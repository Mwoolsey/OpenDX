/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#include "DXApplication.h"
#include "DXType.h"
#include "Network.h"
#include "ShadowedOutputNode.h"


//
// Update any inputs that are being updated by the server (i.e. the
// module that is doing the data-driven operations).
// We update the values internally, and send the shadowing input
// value back to the executive quietly because if
// we are currently in execute-on-change  mode, the assignment would cause
// extra executions.
//
Type ShadowedOutputNode::setShadowedOutputSentFromServer(
                        int output_index, const char *val, Type t)
{
    int input_index = this->getShadowingInput(output_index);
    Type type;
    //
    // Set the values internally
    // Set the output quietly instead of just setting it and not sending it
    // for the following scenario with out the quietly:
    //
    //   1) Interactor->Echo,	Execute echos 0	 
    //		(references GLOBAL main_Interactor_1_out_1)
    //   2) Set Interactor:min value to 5, Execute echos 5
    //		(references LOCAL  main_Interactor_1_out_1)
    //   3) Tab up Interactor:min, Execute echos 0
    //		(references GLOBAL main_Interactor_1_out_1)
    //
    // So, setting the output quietly makes sure that the global value
    // is up to date.
    //
    type = this->setOutputValueQuietly(output_index, val, t);
    if (type != DXType::UndefinedType) {
        this->clearOutputDirty(output_index);
	this->setInputValueQuietly(input_index, val, type);
    }
    return type;
}
//
// Define the mapping of inputs that shadow outputs.
// By default, all shodowed output nodes, have a single output that is
// shadowed by the third (hidden) input.
// Returns an input index (greater than 1) or 0 if there is no shadowing input
// for the given output index.
//
int ShadowedOutputNode::getShadowingInput(int output_index)
{
    int input_index;

    switch (output_index) {
        case 1: input_index = 3;  break;
        default: input_index = 0; break;
    }
    return input_index;
}
//
// A node that has its first parameter set or its last parameter unset
// must mark the network dirty so that is resent with/without the module
// in the network.  If an arc changes the network gets marked dirty anyway
// so we don't last/first dis/connection. 
// FIXME: this really belongs in an OptionalExecuteNode class between
//	DrivenNode and this class.
//
void ShadowedOutputNode::ioParameterStatusChanged(boolean input, int index,
												  NodeParameterStatusChange status)
{

	if (input && this->isInputViewable(index)) {
		int icnt = this->getInputCount();
		int connections, settabs,i  ;
		boolean became_non_dd = FALSE;
		if ((status & Node::ParameterValueChanged) &&
			(status != Node::ParameterSetValueChanged)) {

				//
				// Count the number of connections and set tabs
				//
				for (connections=0, settabs=0, i=1 ; 
					connections==0 && settabs<2 && i<=icnt ; 
					i++) {
						if (this->isInputViewable(i))  {
							if (this->isInputConnected(i)) 
								connections++; 
							else if (!this->isInputDefaulting(i))
								settabs++;
						}
				}	
				if (!connections && (settabs < 2)) {
					//
					// Either a parameter was just given a set value or just 
					// set to the default value.  If just set to the default value 
					// and the number of set tabs is now 0 then the user just made 
					// the last tab up so mark the network dirty.  If just given a 
					// set value and it is the only (first) parameter with a set 
					// value then mark the network dirty.
					//
					if ((settabs == 0) && 
						(status == Node::ParameterSetValueToDefaulting))  {
							// 
							// Tool went from data-driven to non-data-driven.
							// 
							became_non_dd = TRUE;
					} else if ((settabs == 1) && !this->isInputDefaulting(index)) {
						// 
						// Tool went from non-data-driven to data-driven.
						// 
						this->getNetwork()->setDirty();
					}
				}
		} 

#if USE_INSTANCE_CHANGE	
		//
		// Look for the last arc removed.  When the last arc is removed to
		// make the node non-data-driven we must change the instance number 
		// so the exec cache doesn't get confused (see below) if/when it 
		// becomes data-driven again.
		//
		if ((status & Node::ParameterArkChanged) && 
			(status != Node::ParameterArkAdded)) {
				//
				// Count the number of connections and set tabs
				//
				for (connections=0, settabs=0, i=1 ; 
					connections==0 && settabs==0 && i<=icnt ; 
					i++) {
						if (this->isInputViewable(i))  {
							if (this->isInputConnected(i)) 
								connections++; 
							else if (!this->isInputDefaulting(i))
								settabs++;
						}
				}	
				if ((connections == 0) && (settabs == 0)) 
					became_non_dd = TRUE;
		}
#endif 
		if (became_non_dd) {
#if USE_INSTANCE_CHANGE	
			// This does not work yet as we also have to change the ID parameters
			// and the module message id string. dawood, 5/28/94.
			// 
			// Tool went from non-data-driven to non-data-driven.
			// Change its instance number so the executive does not
			// have cache problems. For example, if a user does the 
			// following to a tool with the VPE
			//	 1) Make tool Data-driven (dd), execute
			//	 2) Make tool non-dd, execute
			//	 3) Make tool dd, execute
			// On the 3rd execution, the executive tries to use cache 
			// tags from the first execution.
			// 
			this->assignNewInstanceNumber();
#else
			this->getNetwork()->setDirty();
#endif
		}
	}

	this->DrivenNode::ioParameterStatusChanged(input,index, status);
}
//
// Set the output value of ShadowedOutputNode.  This is the same as for
// the super class methodd, except that it updates the shadowing inputs 
//
Type ShadowedOutputNode::setOutputValue( 
                                int index,
                                const char *value,
                                Type t,
                                boolean send)
{
    Type type;

    //
    // Defer visual notification in case, we do more then one set*Value().
    //
    this->deferVisualNotification();

    //
    // If this is null the 'if' must be taken, becuase setOutputValue()
    // actually succeeds and return UndefinedType when give a NULL string
    //
    ASSERT(value);

    //
    // When data-driven, outputs that have shadowing inputs are not sent and
    // instead their shadows are sent..
    //
#if 1
    int shadow_input = this->getShadowingInput(index);
    if (shadow_input) {
	type = this->DrivenNode::setOutputValue(index, value, t, FALSE);
	if (type != DXType::UndefinedType)  {
	    value = this->getOutputValueString(index);
	    this->setInputValue(shadow_input, value, type, FALSE);
	    if (send)
		this->sendValues(FALSE);
	}
    } else  {
	type = this->DrivenNode::setOutputValue(index, value, t, send);
    }

#else
    boolean send_shadow, driven = this->isDataDriven();
    int shadow_input = this->getShadowingInput(index);

    if (driven && shadow_input)  {
	type = this->DrivenNode::setOutputValue(index, value, t, FALSE);
	this->clearOutputDirty(index);
	send_shadow = send;
    } else {
	type = this->DrivenNode::setOutputValue(index, value, t, send);
	send_shadow = FALSE;
    }

    //
    // If the setOutputValue() above succeeded and this output has a 
    // shadowing input, then update it.  Only send the value if this 
    // node is data-driven and the input send value was TRUE.
    //
    if ((type != DXType::UndefinedType) && shadow_input) {
	const char *value = this->getOutputValueString(index);
#if 0
	if (!driven || !this->isOutputConnected(index))
	    this->setInputSetValue(shadow_input, value, type, FALSE);
#else
	if (!send_shadow) {
	    // We must always keep the executive up to date in case the
	    // input is needed later (i.e. the node becomes data-driven).
	    this->setInputValueQuietly(shadow_input, value, type);
	    //
	    // But make sure it can cause an execution during a sendValues()
	    // if execute-on-change is being used.
	    //
	    this->setInputDirty(shadow_input);
#endif
	} else
	    this->setInputValue(shadow_input, value, type, TRUE);
    }
#endif // 1

    this->undeferVisualNotification();

    return type;
}
//
// Determine if this node is of the given class.
//
boolean ShadowedOutputNode::isA(Symbol classname)
{
    Symbol s = theSymbolManager->registerSymbol(ClassShadowedOutputNode);
    if (s == classname)
	return TRUE;
    else
	return this->DrivenNode::isA(classname);
}
