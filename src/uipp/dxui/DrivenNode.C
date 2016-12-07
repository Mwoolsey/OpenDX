/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#include <string.h> 

#include "DXApplication.h"
#include "DrivenNode.h"
#include "Parameter.h"
#include "AttributeParameter.h"
#include "ErrorDialogManager.h"

// for java stuff
#include "Ark.h" 
#include "ListIterator.h"

//
// Constructor 
//
DrivenNode::DrivenNode(NodeDefinition *nd,
			Network *net, int instance) :
                        ModuleMessagingNode(nd, net, instance)
{
    this->visualNotificationDeferrals = 0;
    this->visualsNeedNotification = FALSE;
    this->handlingLongMessage = FALSE;
}

//
// Update any inputs that are being updated by the server (i.e. the
// module that is doing the data-driven operations).  These inputs
// are updated in a special way since we don't want to change the
// defaulting/set status of the parameter, we don't want it sent back
// to the server, and we don't want to mark it as dirty.
//
Type DrivenNode::setInputAttributeFromServer(int index, 
				const char *val, Type t)
{

    Parameter *p = this->getInputParameter(index);
    ASSERT(p->isA(ClassAttributeParameter));
    boolean was_dirty = p->isDirty();
#if 11 
    boolean r = this->setInputAttributeParameter(index,val);
#else   // We now do this to fix GRESH883, with the thinking being that when
	// we get a value from the server, the real parameter (not just the 
	// attribute parameter) should be updated, regardless of whether or
	// not it type matches with the attribute. 
	// Also, see AttributeParamter::setAttributeValue().
    boolean r = this->setInputAttributeParameter(index,val, TRUE);
#endif
    if (!was_dirty)
	this->clearInputDirty(index);
    ASSERT(r);
    if (t == DXType::UndefinedType) {
	AttributeParameter *ap = (AttributeParameter*)p;
	t = ap->getAttributeValueType();
    }
    return t;
}

//
// Check all input parameters to see if any are connected.
// If connected, then we consider the ModuleMessagingNode to be data-driven 
// (at this level of the the class hierarchy) and return TRUE, 
// otherwise FALSE.
//
boolean DrivenNode::isDataDriven()
{
    int i, icnt = this->getInputCount();
    boolean driven = FALSE; 

    for (i=1 ; !driven && i<=icnt ; i++)  {
	if (this->isInputViewable(i)) 
#if 0	// 8/9/93
	    driven = this->isInputConnected(i) || !this->isInputDefaulting(i);
#else
	    driven = !this->isInputDefaulting(i);
#endif
    }

    return driven;
}
//
// Notify anybody that needs to know that a parameter has changed. 
// At the level we notify all instances that the label has changed.
//
void DrivenNode::ioParameterStatusChanged(boolean input, int index, 
					NodeParameterStatusChange status)
{
    if ((input) && (status & Node::ParameterArkChanged)) {
	//
	// If we become un data driven, then we must make sure the
	// outputs get sent on the next execution.
	//
	boolean driven = this->isDataDriven();
	if (driven) {
	    int i, ocnt = this->getOutputCount();
	    for (i=1 ; i<=ocnt ; i++)	 
		this->setOutputDirty(i);
	    // Don't need to send them because the network will get marked
	    // dirty as a result of an arc change.
	} 
    }

    this->ModuleMessagingNode::ioParameterStatusChanged(input,index, status);

    //
    // Update the UI visuals for this node when ever any of our viewable
    // input arcs or values change, which may lead to a sensitivity change 
    //  on one of the UI visuals for this node. 
    //
    if (input && ((status & Node::ParameterVisibilityChanged) == 0) &&
		  this->isInputViewable(index)) 
	this->notifyVisualsOfStateChange();
}
//
// Set the message id parameter for a data-driven node.  
// We assume that the id parameter is always parameter found in
// the parameter indexed by this->getMessageIdParamNumber().
// Returns TRUE/FALSE.  If FALSE, an error message is given. 
//
boolean DrivenNode::setMessageIdParameter(int id_index)
{
    const char *id = this->getModuleMessageIdString();

    if (id_index == 0) {
	id_index = this->getMessageIdParamNumber();
    	if (id_index == 0)
	    return TRUE;
    }

    if (this->setInputValue(id_index, id, DXType::StringType, FALSE) == 
						DXType::UndefinedType) {
        ErrorMessage(
        "Error setting message id string for node %s, check ui.mdf\n",
                this->getNameString());
	return FALSE;
    }
    return TRUE;
}
#if 0
//
// Called at the end of a long message. 
// This can be overridden by those derived classes that  expect long messages.
// At this level, this method is a no-op.
//
void DrivenNode::completeLongMessage()
{
}
#endif
//
//
// Called when a message is received from the executive after
// this->ExecModuleMessageHandler() is registered in
// this->Node::netPrintNode() to receive messages for this node.
// We parse all the common information and then the class specific.
// If any relevant info was found then we call this->reflectStateChange(). 
//
void DrivenNode::execModuleMessageHandler(int id, const char *line)
{
    const char *p;
    int  values;
    boolean do_notify;

#ifdef DEBUG 
    printf("%s: receiving message #%d...\n", this->getNameString(), id);
    printf("%s\n",line);
#endif

    this->deferVisualNotification();

    //
    // Parse the attributes specific to the derived class
    //
    p = strchr(line,':');	// Skip over '%d:'
    ASSERT(p);
    p = strchr(p+1,':');	// Skip over 'Node-name_%d:'
    ASSERT(p);
	if (!this->handlingLongMessage && EqualString(p,":  begin")) {
		//
		// Begin handling a long message.
		//
		this->handlingLongMessage = TRUE;
		//
		// Don't notify visuals during a long message. 
		//
		do_notify = FALSE;
	} else if (EqualString(p,":  end")) {
		//
		// End of long message  encountered. 
		//
		this->handlingLongMessage = FALSE;
		//
		// Always notify visuals after a long message.
		//
		do_notify = TRUE;
	} else {
		//
		// Handle both long and short messages. 
		// Only do notification on short messages.
		//
		values = this->handleNodeMsgInfo(line);
		do_notify = (!this->handlingLongMessage && values > 0);
	}

	//
	// Notify the visuals if requested. 
	//
	if (do_notify)
		this->notifyVisualsOfStateChange();

	this->undeferVisualNotification(TRUE);
}
//
// Return TRUE/FALSE, indicating whether or not we expect to receive
// a message from the UI when our module executes in the executive.
// By default, a module only executes in the executive for data-driven
// nodes and so we don't expect messages unless the node 
// is data-driven.
//
boolean DrivenNode::expectingModuleMessage()
{
    return this->isDataDriven();
}

//
// Notify all UI visuals that the attributes have changed.
// This calls this->reflectStateChange() if notification is not
// deferred.
//
void DrivenNode::notifyVisualsOfStateChange(boolean unmanage)
{
    if (!this->isVisualNotificationDeferred()) {
        this->visualsNeedNotification = FALSE;
        this->reflectStateChange(unmanage);
    } else {
        this->visualsNeedNotification = TRUE;
    }
}
//
// Determine if the give attribute/paramater is visually writeable (i.e.
// settable from something other than the CDB, like the SetAttrDialog).
//
boolean DrivenNode::isAttributeVisuallyWriteable(int input_index)
{
    if (!this->isDataDriven())
	return TRUE;

    Parameter *p = this->getInputParameter(input_index);
    ASSERT(p->isA(ClassAttributeParameter));
    AttributeParameter *ap = (AttributeParameter*)p;
    return ap->isAttributeVisuallyWriteable(); 
}
//
// Initialize the value of an attribute parameter.
//
boolean DrivenNode::initInputAttributeParameter(int index, const char *val)
{
    AttributeParameter *p = (AttributeParameter*) 
				this->getInputParameter(index);
    return p->initAttributeValue(val);
}
//
// Set the value of an attribute parameter.
// If forceSync is TRUE (default = FALSE), then force the primary
// parameters of the AttributeParameter to be updated regardless of
// whether or not the types match.
//
boolean DrivenNode::setInputAttributeParameter(int index, 
			const char *val, boolean forceSync)
{
    AttributeParameter *p = (AttributeParameter*) 
				this->getInputParameter(index);
    return p->setAttributeValue(val,forceSync);
}
//
// Get the value of an attribute parameter.
//
const char *DrivenNode::getInputAttributeParameterValue(int index)
{
    AttributeParameter *p = (AttributeParameter*) 
				this->getInputParameter(index);
    return p->getAttributeValueString();
}
//
// Determine if this node is of the given class.
//
boolean DrivenNode::isA(Symbol classname)
{
    Symbol s = theSymbolManager->registerSymbol(ClassDrivenNode);
    if (s == classname)
	return TRUE;
    else
	return this->ModuleMessagingNode::isA(classname);
}
//
//  Sync all the attribute values with the actual parameter values.
//
void DrivenNode::syncAttributeParameters()
{
    int i, cnt = this->getInputCount();
    
   
    for (i=1 ; i<=cnt ; i++) {
    	Parameter *p = this->getInputParameter(i);
	ASSERT(p);
	if (p->isA(ClassAttributeParameter)) {
	    AttributeParameter *ap = (AttributeParameter*)p;
	    ap->syncAttributeValue();
	}
    }
}

int DrivenNode::assignNewInstanceNumber()
{
    int t = this->ModuleMessagingNode::assignNewInstanceNumber();
    int index = this->getMessageIdParamNumber();
    if (index > 0)
	this->setMessageIdParameter(index);
    return t;
}




