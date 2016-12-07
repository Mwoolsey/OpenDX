/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
#ifndef _EchoNode_h
#define _EchoNode_h

#include <dxconfig.h>
#include "../base/defines.h"



#include "ModuleMessagingNode.h"


//
// Class name definition:
//
#define ClassEchoNode	"EchoNode"

//
// Referenced Classes
//
class NodeDefinition;
class Network;

//
// EchoNode class definition:
//				
class EchoNode : public ModuleMessagingNode
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
    EchoNode(NodeDefinition *nd, Network *net, int instnc);

    //
    // Destructor:
    //
    ~EchoNode();

    //
    // Determine if this node is a node of the given class
    //
    virtual boolean isA(Symbol classname);

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassEchoNode;
    }
};


#endif // _EchoNode_h
