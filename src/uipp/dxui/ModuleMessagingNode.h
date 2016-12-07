/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#ifndef _ModuleMessagingNode_h
#define _ModuleMessagingNode_h


#include "Node.h"


//
// Class name definition:
//
#define ClassModuleMessagingNode	"ModuleMessagingNode"

//
// Referenced Classes
//
class NodeDefinition;
class Network;

//
// ModuleMessagingNode class definition:
//				
class ModuleMessagingNode : public Node
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
    // protocol between the executive module that runs for this node and the
    // UI.  
    //
    virtual boolean hasModuleMessageProtocol() { return TRUE; }

    //
    // Return TRUE/FALSE, indicating whether or not we expect to receive
    // a message from the UI when our module executes in the executive.
    //
    virtual boolean expectingModuleMessage() { return TRUE; }

    //
    // Called when a message is received from the executive after
    // this->ExecModuleMessageHandler() is registered in
    // this->Node::netPrintNode() to receive messages for this node.
    // The format of the message coming back is defined by the derived class.
    //
    virtual void execModuleMessageHandler(int id, const char *line) = 0;

    //
    // Returns a string that is used to register
    // this->ExecModuleMessageHandler(). Be default we use the super class' 
    // method.
    //
    // virtual const char *getModuleMessageIdString();


  public:
    //
    // Constructor:
    //
    ModuleMessagingNode(NodeDefinition *nd, Network *net, int instnc)
			: Node(nd,net,instnc)  {}

    //
    // Destructor:
    //
    ~ModuleMessagingNode() {}

    //
    // Determine if this node is a node of the given class
    //
    virtual boolean isA(Symbol classname);

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassModuleMessagingNode;
    }
};


#endif // _ModuleMessagingNode_h
