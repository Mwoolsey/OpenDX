/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



// NondrivenInteractorNode.h -						    //
//                                                                          //
// Definition for the NondrivenInteractorNode class.			    //
//
// There general support for non-data-driven interactors.
// To support non-data-driven interactor nodes as a sub-class of 
// InteractorNode (which IS data-driven), we redefined isDataDriven()
// to always return FALSE.  For efficiency reasons we also redefine
// hasModuleMessageProtocol() expectingModuleMessage() to turn off
// the messaging.  We also redefined handleNodeMsgInfo() and
// handleInteractorMsgInfo() to fail when called. 
// Also important, we redefined getShadowingInput() so that these 
// nodes do not have any shadowing inputs, which is generally ok since
// the non-data-driven nodes don't have inputs anyway.
// 
//                                                                          //


#ifndef _NondrivenInteractorNode_h
#define _NondrivenInteractorNode_h


#include "InteractorNode.h"


//
// Class name definition:
//
#define ClassNondrivenInteractorNode	"NondrivenInteractorNode"


//
// NondrivenInteractorNode class definition:
//				
class NondrivenInteractorNode : public InteractorNode
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
    // Return TRUE/FALSE, indicating whether or not we support a message
    // protocol between the executive module that runs for this node and the UI.
    // All data-driven interactors have messaging protocol. 
    //
    virtual boolean hasModuleMessageProtocol() { return FALSE; }

    //
    // Return FALSE since non-data-driven interactors never get messages.
    //
    virtual boolean expectingModuleMessage() { return FALSE; }

    //
    // Define the mapping of inputs that shadow outputs.
    // non-data-driven interactors do not have inputs. 
    //
    virtual int getShadowingInput(int output_index);

    //
    // Defined, but expected NOT to be called.
    //
    int handleNodeMsgInfo(const char *line);
    int  handleInteractorMsgInfo(const char *line);


  public:
    //
    // Constructor:
    //
    NondrivenInteractorNode(NodeDefinition *nd, Network *net, int instnc);

    //
    // DrivenNode wants to change an input param when instance numbers change.
    // This is a strange case: we have no input params even though we're driven.
    //
    int assignNewInstanceNumber() 
	{ return this->ModuleMessagingNode::assignNewInstanceNumber(); }

    //
    // Destructor:
    //
    ~NondrivenInteractorNode(){}


    //
    // Always return FALSE;
    //
    virtual boolean isDataDriven();

    //
    // Determine if this node is a node of the given class
    //
    virtual boolean isA(Symbol classname);

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassNondrivenInteractorNode;
    }
};

#endif // _NondrivenInteractorNode_h
