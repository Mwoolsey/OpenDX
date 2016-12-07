/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


//#include <Xm/DialogS.h>

#include "DXApplication.h"
#include "SequencerWindow.h"
#include "SequencerNode.h"

#include "../widgets/VCRControl.h"

Boolean SequencerWindow::ClassInitialized = FALSE;

String SequencerWindow::DefaultResources[] = {
	".title:			      Sequence Control",
        ".iconName:                           Sequencer",
	"*XmVCRControl.currentColor:          #7e7ec9c90000",
	"*XmVCRControl.nextColor:             #ffffffff7e7e",
        "*XmFrameControl.accelerators:        #augment\n"
        "<Key>Return:                         BulletinBoardReturn()",
	NULL
};

Widget SequencerWindow::createWorkArea(Widget parent)
{
    
    SequencerNode *snode = this->node;

    this->vcr = XtVaCreateManagedWidget("VCR",xmVCRControlWidgetClass, parent,
        XmNwidth,                280,
        XmNheight,               80,
	XmNresizePolicy,	 XmRESIZE_ANY,
	XmNminimum,		 snode->getMinimumValue(),
	XmNmaximum,		 snode->getMaximumValue(),
	XmNstart,		 snode->getStartValue(),
	XmNstop,		 snode->getStopValue(),
	XmNcurrent,		 snode->current,
	XmNcurrentVisible,	 snode->current_defined,
	XmNnext,		 snode->next,
	XmNincrement,		 snode->getDeltaValue(),
	XmNloopButtonState,	 snode->loop,
	XmNstepButtonState,	 snode->step,
	XmNpalindromeButtonState,snode->palindrome,
	NULL);

    //
    // Install the help callback for the vcr 
    //
    this->installComponentHelpCallback(this->vcr);

    //
    // Install the correct state from the node.
    //
    this->handleStateChange(FALSE);

#if 0
    //
    // Add some callbacks. 
    //
    XtAddCallback(shell,
		      XmNpopdownCallback,
		      (XtCallbackProc)SequencerWindow_PopdownCB,
		      (XtPointer)this);
#endif

    XtAddCallback(this->vcr,
		      XmNactionCallback,
		      (XtCallbackProc)SequencerWindow_VcrCB,
		      (XtPointer)this);

    XtAddCallback(this->vcr,
		      XmNframeCallback,
		      (XtCallbackProc)SequencerWindow_FrameCB,
		      (XtPointer)this);

    return this->vcr;
}


SequencerWindow::SequencerWindow(SequencerNode* node) 
                       		: DXWindow("sequencerWindow", FALSE, FALSE)
{
    this->node = node;
    this->vcr = NULL;
    this->handlingStateChange = FALSE;

    //
    // Install the default resources for THIS class (not the derived classes)
    //
    if (NOT SequencerWindow::ClassInitialized)
    {
	ASSERT(theApplication);
        SequencerWindow::ClassInitialized = TRUE;
	this->installDefaultResources(theApplication->getRootWidget());
    }
}


SequencerWindow::~SequencerWindow()
{

#if 0
    XtRemoveCallback(this->getRootWidget(),
		      XmNpopdownCallback,
		      (XtCallbackProc)SequencerWindow_PopdownCB,
		      (XtPointer)this);
#endif

}


//
// Install the default resources for this class.
//
void SequencerWindow::installDefaultResources(Widget baseWidget)
{
    this->setDefaultResources(baseWidget, 
			SequencerWindow::DefaultResources);
    this->DXWindow::installDefaultResources(baseWidget);
}

void SequencerWindow::reset()
{
    Arg    arg[3];

    XtSetArg(arg[0], XmNforwardButtonState,  FALSE);
    XtSetArg(arg[1], XmNbackwardButtonState, FALSE);
    XtSetArg(arg[2], XmNframeSensitive,      TRUE);

    XtSetValues(this->vcr, arg, 3);

}

void SequencerWindow::mapRaise()
{
        XMapRaised(XtDisplay(this->getRootWidget()), 
                   XtWindow(this->getRootWidget()));
	XtManageChild(this->vcr);

}


extern "C" void SequencerWindow_VcrCB(Widget    widget,
                                XtPointer clientData,
                                XtPointer callData)
{
    VCRCallbackStruct* vcr = (VCRCallbackStruct*) callData;

    switch (vcr->action)
    { 
	case VCR_FORWARD:
	case VCR_BACKWARD:

    	     theDXApplication->getExecCtl()->vcrExecute(vcr->action);
	     break;

	default:

    	     theDXApplication->getExecCtl()->vcrCommand(vcr->action,vcr->on);
    }
}

extern "C" void SequencerWindow_FrameCB(Widget    widget,
                                XtPointer clientData,
                                XtPointer callData)
{
    SequencerWindow *dialog = (SequencerWindow*) clientData;
    dialog->frameCallback(callData);
}

void SequencerWindow::frameCallback(XtPointer callData)
{
    char   command[64];
    Arg    arg[3];

    SequencerNode *node = (SequencerNode*) this->node;
    VCRCallbackStruct* vcr = (VCRCallbackStruct*) callData;

    //
    // Turn off this call back when installing state which we're sure
    // is consistennt (i.e. min >= start >= max and min >= stop >= max and
    // start <= stop). Also see the comment in this->handleStateChange(). 
    //
    if (this->handlingStateChange)
	return;

    switch (vcr->which)
    {
      case XmCR_NEXT:
	node->next = vcr->detent;
        sprintf(command, "@nextframe = %d;\n", node->next);
        break;

      case XmCR_START:
	node->setStartValue(vcr->detent);
        sprintf(command, "@startframe = %d;\n", node->getStartValue());
        break;

      case XmCR_STOP:
	node->setStopValue(vcr->detent);
        sprintf(command, "@endframe = %d;\n", node->getStopValue());
        break;

      case XmCR_INCREMENT:
	node->setDeltaValue(vcr->detent);
        sprintf(command, "@deltaframe = %d;\n", node->getDeltaValue());
        break;

      case XmCR_MIN:
	node->setMinimumValue(vcr->value);
	/*
	 * If the minimum value has been changed, change the start
	 * value as well.
	 */
	node->setStartValue(vcr->value);
	XtSetArg(arg[0], XmNstart, node->getStartValue());
	XtSetValues(this->vcr, arg, 1);

        sprintf(command, "@startframe = %d;\n", node->getStartValue());
        break;

      case XmCR_MAX:
	node->setMaximumValue(vcr->value);
	/*
	 * If the maximum value has been changed, change the stop
	 * value as well.
	 */
	node->setStopValue(vcr->value);
	XtSetArg(arg[0], XmNstop, node->getStopValue());
	XtSetValues(this->vcr, arg, 1);

        sprintf(command, "@endframe = %d;\n", node->getStopValue());
        break;

      default:

        ASSERT(FALSE);
        break;
    }

    /*
     * Send the command.
     */
    theDXApplication->getExecCtl()->vcrFrameSet(command);

}

extern "C" void SequencerWindow_PopdownCB(Widget    widget,
                                XtPointer clientData,
                                XtPointer callData)
{
#if 0
    int i;
    Arg arg[1];

    ASSERT(clientData);

    SequencerWindow *data = (SequencerWindow*) clientData;

    XtSetArg(arg[0], XmNcountButtonState, False);
    XtSetValues(data->vcr, arg, 1);

#endif
}

//
// Update all state of the VCR with info from the Node.
//
void SequencerWindow::handleStateChange(boolean unmanage)
{
    SequencerNode *snode = this->node;

    if (!this->vcr)
	return;

    //
    // Turn off the FrameCB callback when installing state which we're sure
    // is consistent (i.e. min >= start >= max and min >= stop >= max and
    // start <= stop). Specifically, we do this because when we are changing
    // the range of min and max the vcr wants to make callbacks adjusting
    // its internal start and stop values to be within range of the new
    // min and max.  For example, assume that min==start and max==stop, then
    // when moving from min=6, max=8 to min=2, max=4, the vcr causes
    // callbacks on start=4, next=4 and stop=4 when installing the new min 
    // and max, but before the new start and stop are installed.  The 
    // problem is that it is clamping start to 4 (the new max), which does
    // not seem unreasonable since the old start was greater than the new max.
    // So, to get around this, we just turn off the FrameCB when installing
    // this state. NOTE, that this assumes the executive has the same values
    // that we are installing, otherwise the executive and the vcr get out
    // of sync. 
    //
    this->handlingStateChange = TRUE;

    XtVaSetValues(this->vcr,
	XmNminimum,		snode->getMinimumValue(),
	XmNmaximum,		snode->getMaximumValue(),
	NULL);

    XtVaSetValues(this->vcr,
	XmNstart,		snode->getStartValue(),
	XmNstop,		snode->getStopValue(),
	XmNincrement,		snode->getDeltaValue(),
	XmNcurrent,		snode->current,
	XmNcurrentVisible,	snode->current_defined,
	XmNnext,		snode->next,
	XmNminSensitive,	snode->isMinimumVisuallyWriteable(),
	XmNmaxSensitive,	snode->isMaximumVisuallyWriteable(),
	XmNincSensitive,	snode->isDeltaVisuallyWriteable(),
        //
        // Set the button states.  This is required for DXLink
        //
        XmNstepButtonState,             snode->isStepMode(),
        XmNloopButtonState,             snode->isLoopMode(),
        XmNpalindromeButtonState,       snode->isPalindromeMode(),

        NULL);

    this->handlingStateChange = FALSE;
}
//
// Disable the Frame control. 
//
void SequencerWindow::disableFrameControl()
{
    if (!this->vcr)
	return;

    XtVaSetValues(this->vcr, XmNframeSensitive,      False, NULL);
}
//
// Enable the Frame control. 
//
void SequencerWindow::enableFrameControl()
{
    if (!this->vcr)
	return;

    XtVaSetValues(this->vcr, XmNframeSensitive,      True, NULL);
}
//
// Set the buttons that indicate which way the sequencer is playing.
//
void SequencerWindow::setPlayDirection(SequencerDirection direction)
{
    if (!this->vcr)
	return;

    switch (direction) {
	case SequencerNode::Directionless:
	    XtVaSetValues(this->vcr,
			XmNforwardButtonState,  False,	  
			XmNbackwardButtonState, False,	  
			NULL);
	break;
	case SequencerNode::ForwardDirection:
	    XtVaSetValues(this->vcr,
			XmNforwardButtonState,  True,	  
			XmNbackwardButtonState, False,	  
			NULL);
	break;
	case SequencerNode::BackwardDirection:
	    XtVaSetValues(this->vcr,
			XmNforwardButtonState,  False,	  
			XmNbackwardButtonState, True,	  
			NULL);
	break;
	default:
	    ASSERT(0);
    }

}
//
// Dis/enable frame control and then call the super class method.
//
void SequencerWindow::beginExecution()
{
    this->disableFrameControl();
    this->DXWindow::beginExecution();
}
void SequencerWindow::standBy()
{
    this->enableFrameControl();
    this->DXWindow::standBy();
}
void SequencerWindow::endExecution()
{
    this->enableFrameControl();
    this->DXWindow::endExecution();
}


//
// Control the startup state of the window thru SequencerNode
// Startup values are maintained on our behalf by both DXWindow and
// SequencerNode.  It's possible that the 2 values will disagree, but
// SequencerNode must have one so that Network can determine the 
// necessity of putting up the vcr.
//
void SequencerWindow::manage()
{
    this->DXWindow::manage();
    this->setStartup(TRUE);
}

void SequencerWindow::unmanage()
{
    this->DXWindow::unmanage();
    this->setStartup(FALSE);
}

void SequencerWindow::setStartup (boolean flag)
{
    this->DXWindow::setStartup(flag);
    this->node->setStartup(flag);
}
