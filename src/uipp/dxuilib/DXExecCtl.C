/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#include "DXStrings.h"
#include "DXExecCtl.h"
#include "Command.h"
#include "DXApplication.h"
#include "DXPacketIF.h"
#include "List.h"
#include "ListIterator.h"
#include "Network.h"
#include "SequencerNode.h"
#include "ErrorDialogManager.h"
#include "ProcessGroupManager.h"
#include "../widgets/VCRControl.h"


DXExecCtl::DXExecCtl()
{
    this->forceNetworkResend = TRUE;
    this->forceParameterResend = TRUE;
    this->execOnChange = FALSE;
    this->execOnChangeSuspensions = 0;
    this->isCurrentlyExecuting       = FALSE;
    this->vcrIsExecuting    = FALSE;
    this->resumeExecOnChangeAfterExecution = FALSE;
    this->endExecOnChangePending = FALSE;
}

void
DXExecCtl::forceFullResend()
{
    this->forceNetworkResend = TRUE;
    this->forceParameterResend = TRUE;

#if WORKSPACE_PAGES
    GroupManager *pmgr = (GroupManager*)
	theDXApplication->network->getGroupManagers()->findDefinition (PROCESS_GROUP);
    if (pmgr) pmgr->setDirty();
#else
    if(theDXApplication->PGManager)
	theDXApplication->PGManager->setDirty();
#endif

    SequencerNode* sequencer = theDXApplication->network->sequencer;
    if(sequencer)
	sequencer->transmitted = FALSE;
}

void
DXExecCtl::BGBeginMessage(void *clientData, int id, void *p)
{
    DXExecCtl *ctl = (DXExecCtl *)clientData; 
    ctl->beginSingleExecution(FALSE);
}
void
DXExecCtl::BGEndMessage(void *clientData, int id, void *p)
{
    DXExecCtl *ctl = (DXExecCtl *)clientData; 
    ctl->endLastExecution(!ctl->vcrIsExecuting);
}

void
DXExecCtl::HWBeginMessage(void *clientData, int id, void *p)
{
    DXExecCtl *ctl = (DXExecCtl *)clientData; 

    if (!ctl->hwExecuting++ AND !ctl->isExecuting())
        theDXApplication->notifyClients(DXApplication::MsgExecute);
}
void
DXExecCtl::HWEndMessage(void *clientData, int id, void *p)
{
    DXExecCtl *ctl = (DXExecCtl *)clientData; 
    Symbol msg = 0;
    ASSERT(ctl->hwExecuting > 0);

    if (--ctl->hwExecuting <= 0 AND !ctl->isExecuting())
	msg = ctl->inExecOnChange() ? DXApplication::MsgStandBy :	
				      DXApplication::MsgExecuteDone;
#ifdef SET_HW_BUSY_CURSOR
    if (ctl->hwExecuting <= 0 AND ctl->hwBusy)
    {
	theDXApplication->setBusyCursor(FALSE);
	ctl->hwBusy = FALSE;
    }
#endif

    if (msg)
        theDXApplication->notifyClients(msg);
}

void
DXExecCtl::ExecComplete(void *clientData, int id, void *p)
{

    DXExecCtl *ctl = theDXApplication->getExecCtl();
    SequencerNode* sequencer = theDXApplication->network->sequencer;

    //
    // During VCR executing we turn off green lights and pop up
    // the Play button ONLY when we receive "stop" command.
    //
    if (ctl->vcrIsExecuting AND !EqualString((char*)p, "stop"))
	return;

    ctl->endLastExecution(TRUE);

    if (ctl->vcrIsExecuting)
    {
    	if (sequencer)
	    sequencer->setPlayDirection(SequencerNode::Directionless);
	ctl->vcrIsExecuting = FALSE;
    }

    if (!ctl->inExecOnChange() AND ctl->hwExecuting <= 0)
        theDXApplication->notifyClients(DXApplication::MsgExecuteDone);
    // else
    //	MsgStandBy already sent by endLastExecution()
}

void
DXExecCtl::ResumeExecOnChange(void *clientData, int id, void *p)
{

    DXExecCtl *ctl = theDXApplication->getExecCtl();
    if(ctl->isExecOnChangeSuspended())
    	ctl->resumeExecOnChange();
}

void DXExecCtl::newConnection()
{
    DXPacketIF *p = theDXApplication->getPacketIF();

    ASSERT(p);

    p->setHandler(DXPacketIF::INTERRUPT,
                  DXExecCtl::ExecComplete,
                  (void*)theDXApplication->getExecCtl(),
                  "stop");

    //
    // highlighting handlers for HW rendering
    //
    this->hwExecuting = 0;
    this->hwBusy      = FALSE;
    p->setHandler(DXPacketIF::INFORMATION,
		  DXExecCtl::HWBeginMessage,
		  (void *)this,
		  "HW:  begin");
    p->setHandler(DXPacketIF::INFORMATION,
		  DXExecCtl::HWEndMessage,
		  (void *)this,
		  "HW:  end");

    p->setHandler(DXPacketIF::INFORMATION,
		  DXExecCtl::BGBeginMessage,
		  (void *)this,
		  "BG:  begin");
    p->setHandler(DXPacketIF::INFORMATION,
		  DXExecCtl::BGEndMessage,
		  (void *)this,
		  "BG:  end");

    SequencerNode* sequencer = theDXApplication->network->sequencer;
    if (sequencer)
	sequencer->transmitted = FALSE;

    if (this->inExecOnChange())
    {
	this->updateMacros(TRUE);
	this->resumeExecOnChange();
    }
    this->execOnChange 		= FALSE;	// Avoid a resumeExecOnChange()
    this->endLastExecution(FALSE);

    this->forceNetworkResend 	= TRUE;
    this->forceParameterResend 	= TRUE;
    this->execOnChangeSuspensions = 0;	
    this->isCurrentlyExecuting  = FALSE;
    this->vcrIsExecuting    	= FALSE;

}

void DXExecCtl::updateMacros(boolean force)
{
    DXPacketIF *p = theDXApplication->getPacketIF();
    if (p == NULL)
	return;
    boolean forcenet   = force  || this->forceNetworkResend;
    boolean forceparam = force  || this->forceParameterResend;

    boolean resume = FALSE;

    if (this->inExecOnChange() AND !this->isExecOnChangeSuspended())
    {
        this->suspendExecOnChange();
        resume = TRUE;
    }

    this->forceNetworkResend = FALSE;
    this->forceParameterResend = FALSE;

#if WORKSPACE_PAGES
    ProcessGroupManager *pmgr = (ProcessGroupManager*)
	theDXApplication->network->getGroupManagers()->findDefinition (PROCESS_GROUP);
    if(pmgr AND pmgr->isDirty())
	pmgr->sendAssignment(ProcessGroupManager::ATTACH);
#else
    if(theDXApplication->PGManager AND theDXApplication->PGManager->isDirty())
          theDXApplication->PGManager->
                sendAssignment(ProcessGroupManager::ATTACH);
#endif

    ListIterator ii(theDXApplication->macroList);
    Network *n;
    while((n = (Network*)ii.getNext()) != NULL)
    {
	boolean dirty = n->isDirty();
	if (forcenet || dirty)
	    n->sendNetwork();
	if (forceparam || dirty)
	    n->sendValues(forceparam);
    }

    n = theDXApplication->network;
    boolean dirty = n->isDirty();
    if (forcenet || dirty)
	n->sendNetwork();
    if (forceparam || dirty)
	n->sendValues(forceparam);

    SequencerNode* sequencer = n->sequencer;
    if (sequencer && (force || NOT sequencer->transmitted))
	this->vcrTransmit();

    if (resume)
	this->resumeExecOnChange();

}

void DXExecCtl::vcrFrameSet(char* command)
{
    DXPacketIF *p = theDXApplication->getPacketIF();
    void   (*callback)(void*,int,void*) = NULL;

    if (p != NULL)
    {
    	if(this->inExecOnChange())
        {
	     this->suspendExecOnChange();
	     callback = ResumeExecOnChange;
	}

    	p->send(DXPacketIF::FOREGROUND, command, callback);
    }
}

void DXExecCtl::vcrTransmit()
{
    char  command[1024];
    DXPacketIF *p = theDXApplication->getPacketIF();
    SequencerNode* vcr = theDXApplication->network->sequencer;

    if (p == NULL)
   	return;

    sprintf(command, "sequence %s();\n", 
		     theDXApplication->network->getNameString());
    p->send(DXPacketIF::FOREGROUND, command);

    char *s = vcr->getFrameControlString();
    p->send(DXPacketIF::FOREGROUND, s);
    delete s;

    p->send(DXPacketIF::FOREGROUND,
            vcr->isLoopMode() ? "loop on;\n" : "loop off;\n");

    p->send(DXPacketIF::FOREGROUND,
            vcr->isPalindromeMode() ? "palindrome on;\n" : "palindrome off;\n");

    vcr->transmitted = TRUE;

}

void DXExecCtl::vcrExecute(int action)
{
    SequencerNode* sequencer = theDXApplication->network->sequencer;
    DXPacketIF *p = theDXApplication->getPacketIF();

    if (NOT p OR this->vcrIsExecuting OR this->isCurrentlyExecuting )
    {
	SequencerDirection direction;

	if(NOT p)
	    ErrorMessage("No Connection to the server.");
	else
	    ErrorMessage("The program is currently executing.");

	if (action == VCR_FORWARD)
	    direction = (p AND this->vcrIsExecuting ? 
				SequencerNode::BackwardDirection : 
				SequencerNode::Directionless); 
	else
	    direction = (p AND this->vcrIsExecuting ? 
				SequencerNode::ForwardDirection : 
				SequencerNode::Directionless); 

	sequencer->setPlayDirection(direction);
      
   	return;
    }

    this->vcrIsExecuting = TRUE;
    this->beginSingleExecution(TRUE); // May suspend ExecOnChange.

    p->send(DXPacketIF::FOREGROUND,
	    action==VCR_FORWARD ? "forward;\n" : "backward;\n"); 

    p->send(DXPacketIF::FOREGROUND, sequencer->isStepMode() ? 
				    "step;\n" : "play;\n");

}

void DXExecCtl::vcrCommand(int action, boolean pressed)
{
    char  command[30];
    DXPacketIF *p = theDXApplication->getPacketIF();
    SequencerNode* sequencer = theDXApplication->network->sequencer;
    Widget vcr = NULL;
    boolean doEnd = FALSE;

    if (action == VCR_STOP) {
	ASSERT(sequencer);
	vcr = sequencer->getVCRWidget();
    }

    if (p == NULL)
    {
//	this->endLastExecution(!this->vcrIsExecuting);
	this->vcrIsExecuting = FALSE;
        sequencer->setPlayDirection(SequencerNode::Directionless);

	if (action == VCR_STOP)
	    XtVaSetValues(vcr, XmNnext, sequencer->next, NULL);

   	return;
    }

    switch(action)
    {
	 case VCR_PALINDROME:
	     sequencer->setPalindromeMode(pressed);
	     strcpy(command, pressed ? "palindrome on;\n" : "palindrome off;\n");

	     break;

	case VCR_STEP:
	     sequencer->setStepMode(pressed); 
	     sequencer->setPlayDirection(SequencerNode::Directionless);
	     strcpy(command, "pause;\n");
	     doEnd = this->vcrIsExecuting;
    	     this->vcrIsExecuting = FALSE;

	     break;

	case VCR_LOOP:
	     sequencer->setLoopMode(pressed); 
	     strcpy(command, pressed ?  "loop on;\n" : "loop off;\n");

	     break;

	case VCR_PAUSE:

	     strcpy(command, "pause;\n");
	     doEnd = this->vcrIsExecuting;
    	     this->vcrIsExecuting = FALSE;
	     sequencer->setPlayDirection(SequencerNode::Directionless);
	     break;

	case VCR_STOP:

	     strcpy(command, "stop;\n");
	     doEnd = this->vcrIsExecuting;
    	     this->vcrIsExecuting = FALSE;
	     sequencer->setPlayDirection(SequencerNode::Directionless);

	     break;

	default:
	
	     ASSERT(FALSE);
    }

    p->send(DXPacketIF::FOREGROUND,command);

    if (action == VCR_STOP) 
    {
        sequencer->next = sequencer->getStartValue();
        sprintf(command, "@nextframe = %d;\n", sequencer->next);
    	if(this->inExecOnChange()
	   AND !this->isExecOnChangeSuspended())
	     this->suspendExecOnChange();
        p->send(DXPacketIF::FOREGROUND, command, ResetVcrNextFrame);
    }

    if (doEnd)
	this->endLastExecution(action != VCR_STOP);
}

void DXExecCtl::ResetVcrNextFrame(void *clientData, int id, void *p)
{
    DXExecCtl *ctl = theDXApplication->getExecCtl();
    SequencerNode* sequencer = theDXApplication->network->sequencer;
    ASSERT(sequencer);
    Widget vcr = sequencer->getVCRWidget();

    sequencer->next = sequencer->getStartValue();
    XtVaSetValues(vcr, XmNnext, sequencer->next, NULL);

    if(ctl->isExecOnChangeSuspended())
	ctl->resumeExecOnChange();
}

void DXExecCtl::enableExecOnChange()
{
    DXPacketIF *p = theDXApplication->getPacketIF();
    if (!p || this->execOnChange)
	return;
 
    //
    // Set the light green now, so that sending big networks looks like
    // execution.
    //
    theDXApplication->notifyClients(DXApplication::MsgExecute);

    // 
    // Update macros here and not in beginSingleExecution() so that 
    // execOnChange doesn't get suspended.
    // 
    this->updateMacros();	
    this->execOnChange = TRUE;

    // 
    // Go into exec on change mode.
    // 
    this->resumeExecOnChange();

    // 
    // Do what is usually necessary at the beginning of an execution. 
    // 
    this->beginSingleExecution(FALSE);	// updateMacros done above

    // 
    // Now cause the first execution. 
    // 
    char s[100];
    strcpy(s, theDXApplication->network->getNameString());
    strcat(s, "();\n");
    p->send(DXPacketIF::FOREGROUND, s, DXExecCtl::ExecComplete, NULL);
}
//
// Go out of execution on change mode without terminating the current
// graph execution.  If not current executing, then we go ahead and
// go out of eoc mode, otherwise schedule the exit from eoc mode for
// the end of the current graph execution (see endLastExecution()). 
// We return TRUE if we were able to go out of eoc mode now, FALSE if
// we won't be going out until the end of the current execution.
//
boolean DXExecCtl::endExecOnChange()
{
    if (!this->inExecOnChange())
	return TRUE;

    //
    // If the exec is currently executing, then we will wait to go out
    // of execute-on-change mode until the current execution completes.
    // If we don't do this, then the INTERRUPT message to go out of
    // execute on change, will also kill the current graph execution,
    // which we don't want to do. 
    //
    if (this->isCurrentlyExecuting) {
	this->endExecOnChangePending = TRUE;
	return FALSE;
    } 

    if (this->vcrIsExecuting) {
	this->vcrIsExecuting = FALSE;	// Only do one endLastExecution().
        this->vcrCommand(VCR_PAUSE, 0);
    }

    this->endExecOnChangePending = FALSE;
    this->execOnChangeSuspensions = 0; 
    this->execOnChange = FALSE;
    DXPacketIF *p = theDXApplication->getPacketIF();
    if (p)
	p->send(DXPacketIF::INTERRUPT);

    theDXApplication->notifyClients(DXApplication::MsgExecuteDone);

    return TRUE;

}
void DXExecCtl::suspendExecOnChange()
{
    if (!this->inExecOnChange())
	return;

    if (this->execOnChangeSuspensions == 1) 
	return;		// Already suspended

    this->execOnChangeSuspensions = 1; 

    DXPacketIF *p = theDXApplication->getPacketIF();
    if (p) {
	p->send(DXPacketIF::INTERRUPT);
        p->sendImmediate("sync");
    }
}
void DXExecCtl::resumeExecOnChange()
{

    this->execOnChangeSuspensions = 0;
#if 00
    if (!this->inExecOnChange() || this->isExecuting())
	return;
    DXPacketIF *p = theDXApplication->getPacketIF();
    if (p == NULL)
	return;
#else
    DXPacketIF *p = theDXApplication->getPacketIF();
    if (p == NULL)
	return;

    if (!this->inExecOnChange())
	return;
    if (this->isExecuting()) {
	this->resumeExecOnChangeAfterExecution = TRUE;
        this->execOnChangeSuspensions = 1;
	return;
    }
#endif


    //
    // Be sure that all pending assignments/definitions are processed by
    // the executive before going into execute on change.  If we don't do
    // this we can get multiple executions from the pending assigments. 
    // Note that this an unneeded '$sync' is sent when we are in execute-
    // on-change and the user does an 'execute-once' command.  In this
    // case we still send the '$sync', but it is not necessary.
    //
    p->sendImmediate("sync");

    char s[100];
    strcpy(s, theDXApplication->network->getNameString());
    strcat(s, "();\n");
    p->send(DXPacketIF::BACKGROUND, s);
    theDXApplication->notifyClients(DXApplication::MsgStandBy);
}
void DXExecCtl::executeOnce()
{
    DXPacketIF *p = theDXApplication->getPacketIF();
    if (p == NULL)
	return;

#if 0
    if (this->inExecOnChange())
	this->suspendExecOnChange();
#endif

    this->beginSingleExecution(TRUE);

    char s[100];
    strcpy(s, theDXApplication->network->getNameString());
    strcat(s, "();\n");
    p->send(DXPacketIF::FOREGROUND, s, DXExecCtl::ExecComplete, NULL);

}
void DXExecCtl::terminateExecution()
{
#ifdef SET_HW_BUSY_CURSOR
    if (this->hwExecuting > 0 AND NOT this->hwBusy) {
	theDXApplication->setBusyCursor(TRUE);
	this->hwBusy = TRUE;
    }
#endif

    if (!this->isCurrentlyExecuting && !this->execOnChange) 
	return;

    if (this->vcrIsExecuting) {
	this->vcrIsExecuting = FALSE;	// Only do one endLastExecution().
        this->vcrCommand(VCR_PAUSE, 0);
    }

    Symbol msg;
    DXPacketIF *p = theDXApplication->getPacketIF();
    if (p == NULL) {
	msg = DXApplication::MsgExecuteDone;
        //
        // Set activation of execution dependent commands.
        //
        theDXApplication->notExecutingCmd->execute();
    } else {
	msg = 0;
	p->send(DXPacketIF::INTERRUPT);
	p->send(DXPacketIF::FOREGROUND, "Executive(\"nop\");",
				DXExecCtl::ExecComplete, NULL);
        p->sendImmediate("sync");
	theDXApplication->executeOnChangeCmd->activate();
    }

    this->execOnChange = FALSE;
    // FIXME: should use the following but won't right before release...
    this->isCurrentlyExecuting = FALSE;
    if (msg)
        theDXApplication->notifyClients(msg);
}

void DXExecCtl::updateMacro(Network *n)
{
    DXPacketIF *p = theDXApplication->getPacketIF();
    if (p == NULL)
	return;

    boolean resume = FALSE;

    if (n->isDirty())
    {
        if (this->inExecOnChange() AND !this->isExecOnChangeSuspended())
        {
            this->suspendExecOnChange();
            resume = TRUE;
        }

	n->sendNetwork();
	n->sendValues();

	if (resume)
	    this->resumeExecOnChange();
    }
}

//
// This function is called with a boolean that, in essence, indicates
// whether the system is quiescent (updateable) or active (i.e. as a
// result of the BG: begin message, and thus not updateable).
void DXExecCtl::beginSingleExecution(boolean update)
{
    theDXApplication->notifyClients(DXApplication::MsgExecute);

    if (update && this->inExecOnChange() && !this->isExecOnChangeSuspended())
	this->suspendExecOnChange();

    theDXApplication->clearErrorList(); 

    if (update) 
	this->updateMacros();
 
    this->isCurrentlyExecuting = TRUE;
    //
    // Set activation of execution dependent commands.
    //
    theDXApplication->executingCmd->execute();

}

void DXExecCtl::endLastExecution(boolean resume)
{
    this->isCurrentlyExecuting = FALSE;
    SequencerNode* sequencer = theDXApplication->network->sequencer;
    Symbol msg = 0;

    if (this->vcrIsExecuting)
    {
	if (sequencer->isStepMode())
	    this->vcrIsExecuting = FALSE;
    } 

    if ((resume || this->resumeExecOnChangeAfterExecution) AND 
	this->isExecOnChangeSuspended()) {
	this->resumeExecOnChange(); // This may result in MsgStandBy message 
    } else if (this->inExecOnChange() && !this->isExecOnChangeSuspended()) {
	if (this->hwExecuting <= 0)
	    msg = DXApplication::MsgStandBy;
    }

    //
    // If a request to exit ExecOnChange mode was requested during an
    // execution, then do it now.
    //
    if (this->endExecOnChangePending) {
	this->endExecOnChangePending = FALSE;
	this->endExecOnChange();
	msg = 0;
    }

    this->resumeExecOnChangeAfterExecution = FALSE;

    //
    // Set activation of execution dependent commands.
    //
    theDXApplication->notExecutingCmd->execute();

    //
    // Indicate the nodes that had errors.
    // FIXME: only do this if there are errors.
    //
    theDXApplication->refreshErrorIndicators();

    if (msg)
	theDXApplication->notifyClients(msg);
}
