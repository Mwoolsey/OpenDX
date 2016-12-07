/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



// ResetNode.h -
//             
// Definition for the ResetNode class.
//
/*
 This node is much like a toggle (in fact it is derived from the
 ToggleNode class), except that it is reset by the executive after a single
 execution.  If it was already reset, then the executive leaves it reset.
 The executive sends a message to the UI when it gets reset, we catch that
 message and reset the displayed toggle value.  The message contains simply 
 the name of the output variable that is acting as the oneshot. Resets have
 a unique assignment syntax as follows:

	oneshot_1_out_1[oneshot:resetval] = setval;

*/
//////////////////////////////////////////////////////////////////////////////

#ifndef _ResetNode_h
#define _ResetNode_h


#include "ToggleNode.h"


//
// Class name definition:
//
#define ClassResetNode	"ResetNode"


//
// ResetNode class definition:
//				
class ResetNode : public ToggleNode
{
  private:
    //
    // Private member data:
    //


  protected:
    //
    // Protected member data:
    //

    //
    // Look for the "'oneshotname': Resetting..." message.
    //
    int handleInteractorMsgInfo(const char *line);

    virtual boolean expectingModuleMessage();

    //
    // Reset output lvalues contain the '[oneshot:value]' attribute if the
    // output is currently set.  So, we first call the super class method and
    // then if this is the #1 output an it is set, then we append the attribute.
    //
    virtual int strcatParameterNameLvalue(char *s,  Parameter *p,
                                const char *prefix, int index);

    //
    // Define the token that we install in the message handler to receive 
    // messages for this module.  We redefine this method to return the name 
    // of the output as the executive knows it.  It uses the name of the 
    // output, to send a message back to the ui (i.e. 'outputname: reset').
    //
    const char *getModuleMessageIdString();

  public:
    //
    // Constructor:
    //
    ResetNode(NodeDefinition *nd, Network *net, int instnc);

    //
    // Destructor:
    //
    ~ResetNode();

    //
    // Called after allocation is complete.
    // The work done here is to assigned default values to the InteractorNode 
    // inputs so that we can use them later when setting the attributes for the
    // Interactor.
    //
    virtual boolean initialize();

    //
    // Determine if this node is a node of the given class
    //
    virtual boolean isA(Symbol classname);

    virtual const char* getJavaNodeName() { return "ResetNode"; }

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassResetNode;
    }
};

#endif // _ResetNode_h
