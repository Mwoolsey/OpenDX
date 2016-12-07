/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#include "ResetNode.h"
#include "Parameter.h"

#if 0	// We get everything from ToggleNode
//
// The following define the mapping of parameter indices to parameter
// functionality in the module that is used to 'data-drive' the
// ResetNode class and Nodes derived from it.
// These MUST match the entries in the ui.mdf file for the interactors
// that expect to be implemented by the ResetInteractor class.
//
#define ID_PARAM_NUM            1       // Id used for UI messages
#define DATA_PARAM_NUM          2       // A list of the 2 set/reset values
#define CVAL_PARAM_NUM          3       // Current output value
#define LABEL_PARAM_NUM         4       // Label 
#define EXPECTED_SELECTOR_INPUTS        LABEL_PARAM_NUM
#endif

//
// Constructor 
//
ResetNode::ResetNode(NodeDefinition *nd,
			Network *net, int instance) :
                        ToggleNode(nd, net, instance)
{
}
ResetNode::~ResetNode()
{
}
//
// Called after allocation is complete.
// The work done here is to assigned default values to the InteractorNode inputs
// so that we can use them later when setting the attributes for the
// Interactor.
//
boolean ResetNode::initialize()
{
    return this->ToggleNode::initialize();
}
//
// Define the token that we install in the message handler to receive messages
// for this module.  We redefine this method to return the name of the output
// as the executive knows it.  It uses the name of the output, to send a message
// back to the ui (i.e. 'outputname: reset').
//
const char *ResetNode::getModuleMessageIdString()
{
   if (!this->moduleMessageId) 
       this->moduleMessageId = this->getNetworkOutputNameString(1); 

   return (const char*)this->moduleMessageId;
}
//
// Look for the "'oneshotname': Resetting oneshot" message.
//
int ResetNode::handleInteractorMsgInfo(const char *line)
{
    int vals = 0;

    if (strstr(line,"Resetting oneshot"))  {
	this->reset(FALSE);	// Don't send value
	this->clearOutputDirty(1);	// Don't send it later either
	vals++;
    }
    return vals;
}
boolean ResetNode::expectingModuleMessage()
{
    return TRUE; 
}

//
// Reset output lvalues contain the '[oneshot:value]' attribute if the
// output is currently set.  So, we first call the super class method and
// then if this is the #1 output an it is set, then we append the attribute. 
//
int ResetNode::strcatParameterNameLvalue(char *s,  Parameter *p,
                                const char *prefix, int index)
{
    int cnt = ToggleNode::strcatParameterNameLvalue(s,p,prefix,index);

    if ((index == 1) && !p->isInput() && this->isSet()) {
	char *reset = this->getResetValue();
	char *p = s;
	p += STRLEN(s);
	sprintf(p,"[oneshot:%s]",reset);
	cnt += STRLEN(p);
	delete reset;
    }

    return cnt;
}

//
// Determine if this node is of the given class.
//
boolean ResetNode::isA(Symbol classname)
{
    Symbol s = theSymbolManager->registerSymbol(ClassResetNode);
    if (s == classname)
	return TRUE;
    else
	return this->ToggleNode::isA(classname);
}
