/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#include "Network.h"
#include "DXLOutputNode.h"
#include "ErrorDialogManager.h"

int DXLOutputNode::SettingLabel = 0 ;

#define LABEL_PARMNUM 1
#define VALUE_PARMNUM 2

DXLOutputNode::DXLOutputNode(NodeDefinition *nd, Network *net, int instnc) :
    DrivenNode(nd, net, instnc)
{
}
boolean DXLOutputNode::initialize()
{
    char label[512];

    if (!this->getNetwork()->isReadingNetwork()) {
	sprintf(label,"%s_%d",this->getNameString(),this->getInstanceNumber());
	this->setInputValue(LABEL_PARMNUM,label, DXType::StringType,FALSE);
    }

    return TRUE;
}

DXLOutputNode::~DXLOutputNode()
{
}

//
// Update any UI visuals that may be based on the state of this
// node.   Among other times, this is called after receiving a message
// from the executive.
//
void DXLOutputNode::reflectStateChange(boolean unmanage)
{
   return; // This is empty because setLabelString() takes care of all.
}

//
// Called when a message is received from the executive after
// this->ExecModuleMessageHandler() is registered in
// this->Node::netPrintNode() to receive messages for this node.
// We parse the 'label=....' message to set the label string and 1st 
// parameter. 
// We return the number of items in the message.
//
int DXLOutputNode::handleNodeMsgInfo(const char *line)
{
    const char *p;

    if ( (p = strstr(line,"label=")) ) {
	p+=6;	// skip =.
	this->setLabelString(p);	
    }
    return 0;	// Because relfectStateChange() is a nop.
}

//
// When setting the label, keep the 1st parameter up to date.
//
boolean DXLOutputNode::setLabelString(const char *label)
{
    boolean retval;

    if (!this->DrivenNode::setLabelString(label)) 
	goto error;
   
    if (this->SettingLabel == 0) {
	// char *old_label = DuplicateString(this->getLabelString());
	this->SettingLabel++;
	//
	// Set the label, but don't cause an execution.
	//
	retval = this->setInputAttributeParameter(LABEL_PARMNUM,label) &&
		 this->setInputSetValue(LABEL_PARMNUM,label, 
				DXType::UndefinedType,FALSE);
	if (retval)
	    this->sendValuesQuietly();
	this->SettingLabel--;
    } else {
	retval = TRUE;
    }

    return retval;

error:
    return FALSE;

}
// 
// Keep the label up to date when the 1st parameter changes
// (also see setLabelString()).
//
void DXLOutputNode::ioParameterStatusChanged(boolean input, int index,
                                NodeParameterStatusChange status)
{
    if (input && 
	(index == LABEL_PARMNUM) && 
	(PARAMETER_VALUE_CHANGED & status) &&
	(this->SettingLabel == 0)) { 
	this->SettingLabel++;
	const char *label = this->getInputValueString(LABEL_PARMNUM);
	

	if (!EqualString(label,"NULL")) {
	    //
	    // Strip the double quotes from the parameter value.
	    //
	    char *p, *noquotes = DuplicateString(label);
	    p = noquotes + STRLEN(noquotes) - 1;
	    if (*p == '"') {
		*p = '\0';
		p = noquotes;
		if (*p == '"') p++;
	    } else {
		p = noquotes;
	    }
	    this->setLabelString(p);
	    delete noquotes;
	}
	this->SettingLabel--;
    }
    this->DrivenNode::ioParameterStatusChanged(input,index,status);
}
//
// Determine if this node is of the given class.
//
boolean DXLOutputNode::isA(Symbol classname)
{
    Symbol s = theSymbolManager->registerSymbol(ClassDXLOutputNode);
    if (s == classname)
	return TRUE;
    else
	return this->DrivenNode::isA(classname);
}

//
// We already have a new instance number.
//
boolean DXLOutputNode::canSwitchNetwork (Network *from, Network *to)
{
    const char* label = this->getLabelString();
    const char* conflict = to->nameConflictExists(label);
    if (conflict) {
	ErrorMessage ("A %s with name \"%s\" already exists", conflict, label);
	return FALSE;
    }

    return this->DrivenNode::canSwitchNetwork(from, to);
}
