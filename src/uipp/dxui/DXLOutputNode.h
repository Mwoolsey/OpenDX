/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#ifndef _DXLOutputNode_h
#define _DXLOutputNode_h



#include "DrivenNode.h"


//
// Class name definition:
//
#define ClassDXLOutputNode	"DXLOutputNode"

//
// Referenced Classes

//
// DXLOutputNode class definition:
//				
class DXLOutputNode : public DrivenNode
{
  private:
    //
    // Private member data:
    //
    static int SettingLabel;

  protected:
    //
    // Protected member data:
    //


    //
    // Called when a message is received from the executive after
    // this->ExecModuleMessageHandler() is registered in
    // this->Node::netPrintNode() to receive messages for this node.
    // We parse the 'label=....' message to set the label string and 1st 
    // parameter.
    // We return the number of items in the message.
    //
    virtual int handleNodeMsgInfo(const char *line);

    //
    // This is a nop because setLabelString() does all the work
    //
    virtual void reflectStateChange(boolean unmanage); 

    virtual void ioParameterStatusChanged(boolean input, int index,
                                NodeParameterStatusChange status);

  public:
    //
    // Constructor:
    //
    DXLOutputNode(NodeDefinition *nd, Network *net, int instnc);

    //
    // Destructor:
    //
    ~DXLOutputNode();

    virtual boolean initialize();

    //
    // Determine if this node is a node of the given class
    //
    virtual boolean isA(Symbol classname);

    //
    // Return TRUE if the node can be switched (pasted/merged/moved) from
    // the 'from' net to the 'to' net.
    //
    virtual boolean canSwitchNetwork(Network *from, Network *to);


    //
    // When setting the label, keep the 1st parameter up to date.
    //
    virtual boolean setLabelString(const char *label);

    virtual boolean needsFastSort() { return TRUE; }

    virtual const char* getJavaNodeName() { return "DXLOutputNode"; }

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassDXLOutputNode;
    }
};


#endif // _DXLOutputNode_h
