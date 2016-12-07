/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#ifndef _PrintNode_h
#define _PrintNode_h


#include "ModuleMessagingNode.h"


//
// Class name definition:
//
#define ClassPrintNode	"PrintNode"

//
// Referenced Classes
//
class NodeDefinition;
class Network;

//
// PrintNode class definition:
//				
class PrintNode : public ModuleMessagingNode
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
    // Called when a message is received from the executive after
    // this->ExecModuleMessageHandler() is registered in
    // this->Node::netPrintNode() to receive messages for this node.
    // The format of the message coming back is defined by the derived class.
    //
    virtual void execModuleMessageHandler(int id, const char *line);

    //
    // Returns a string that is used to register
    // this->ExecModuleMessageHandler() when this->hasModuleMessageProtocol()
    // return TRUE.  This version, returns an id that is unique to this
    // instance of this node.
    //
    virtual const char *getModuleMessageIdString();


  public:
    //
    // Constructor:
    //
    PrintNode(NodeDefinition *nd, Network *net, int instnc);

    //
    // Destructor:
    //
    ~PrintNode();

    //
    // Determine if this node is a node of the given class
    //
    virtual boolean isA(Symbol classname);

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassPrintNode;
    }
};


#endif // _PrintNode_h
