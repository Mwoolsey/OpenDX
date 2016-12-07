/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



// DrivenNode.h -					
//                                            
// Definition for the DrivenNode class.			    
//
// There general support for data-driven nodes.  These
// are nodes that have inputs and make an executive module call and
// then expect a UImessage from that module concerning the state of the
// UI visuals for the Node (i.e. mininum, maximum, increment, label...).
// this->Node::netPrintNode() determines if the ModuleMessagingNode is 
// data-driven with this->expectingModuleMessage() via this->isDataDriven() 
// and arranges for this->execModuleMessageHandler() to be called when a 
// message for this ModuleMessagingNode is found.
//
// All DrivenNodes (if they have NodeDefinitions that derived from 
// DrivenDefinition) have AttributeParameters instead of Parameters for
// the inputs.  When arcs are changed or a message
// from the executive is received, the UI Visual is asked to updates its 
// state through this->reflectStateChange() which is called by 
// this->notifyVisualsOfStateChange() .  Calls to reflectStateChange()
// can be deferred through the use of [un]deferVisualNotification().  This
// can be useful during message reception.
//
// At this level isDataDriven() indicates that the node is data-driven if
// any non-private inputs are connected.
//                                     


#ifndef _DrivenNode_h
#define _DrivenNode_h


#include "ModuleMessagingNode.h"


//
// Class name definition:
//
#define ClassDrivenNode	"DrivenNode"


//
// DrivenNode class definition:
//				
class DrivenNode : public ModuleMessagingNode
{
  private:
    //
    // Private member data:
    //
    int 	visualNotificationDeferrals;
    boolean 	visualsNeedNotification;

    //
    // Set the label used for the data driven node.  Generally
    // only called by the message handler.
    //
    void setInteractorLabel(const char *label);

  protected:
    //
    // Protected member data:
    //

    //
    // This is set when the execModuleMessageHandler() starts accepting
    // a long message.
    //
    boolean handlingLongMessage;

    //
    // Called when a message is received from the executive after
    // this->ExecModuleMessageHandler() is registered in
    // this->Node::netPrintNode() to receive messages for this node.
    // We parse all the common information and then the class specific.
    // If any relevant info was found then we call this->reflectStateChange(). 
    //
    // If any input or output values are to be changed, don't send them 
    // because the module backing up the node will have just executed 
    // and if the UI is in 'execute on change' mode then there will be an 
    // extra execution.
    //
    virtual void execModuleMessageHandler(int id, const char *line);

    //
    // Parse the node specific info from an executive message. 
    // Returns the number of attributes parsed.
    //
    virtual int  handleNodeMsgInfo(const char *line) = 0;
 
    //
    // Update any UI visuals that may be based on the state of this
    // node.   Among other times, this is called after receiving a message
    // from the executive.
    //
    virtual void reflectStateChange(boolean unmanage) = 0;

    //
    // Return TRUE/FALSE, indicating whether or not we expect to receive
    // a message from the UI when our module executes in the executive.
    // By default, a module only executes in the executive for data-driven
    // nodes and so we don't expect messages unless the node 
    // is data-driven.
    //
    virtual boolean expectingModuleMessage();

    //
    // Set the message id parameter for a data-driven node.  
    // We assume that the id parameter is always parameter the value
    // returned by getMessageIdParamNumber().
    // Returns TRUE/FALSE.  If FALSE, an error message is given.
    //
    boolean setMessageIdParameter(int id_index = 0);

    //
    // Get the index of the parameter that has a message id token.
    // Returns 0, if there is not message id parameter for the node.
    //
    virtual int getMessageIdParamNumber() { return 1; }

    //
    // Notify anybody that needs to know that a parameter has changed its 
    // arcs or values.
    //
    virtual void ioParameterStatusChanged(boolean input, int index, 
			NodeParameterStatusChange status);

    //
    // Update any inputs that are being updated by the server (i.e. the
    // module that is doing the data-driven operations).  These inputs
    // are updated in a special way since we don't want to change the
    // defaulting/set status of the parameter, we don't want it sent back
    // to the server, and we don't want to mark it as dirty.
    //
    Type setInputAttributeFromServer(int index, const char *val,
                                        Type t = DXType::UndefinedType);

    //
    // Initialize/set the value of an attribute parameter.
    // If forceSync is TRUE (default = FALSE), then force the primary
    // parameters of the AttributeParameter to be updated regardless of
    // whether or not the types match.
    boolean setInputAttributeParameter(int index, const char *val, 
						boolean forceSync = FALSE);
    boolean initInputAttributeParameter(int index, const char *val);
    const char *getInputAttributeParameterValue(int index);
    //
    // Determine if the give attribute/paramater is visually writeable (i.e.
    // settable from something other than the CDB, like the SetAttrDialog).
    // It may be ok to make this public, but erring on the side of caution
    // for now.
    //
    virtual boolean isAttributeVisuallyWriteable(int input_index);

    //
    //  Sync all the attribute values with the actual parameter values.
    //
    void syncAttributeParameters();


  public:
    //
    // Constructor:
    //
    DrivenNode(NodeDefinition *nd, Network *net, int instnc);

    //
    // Destructor:
    //
    ~DrivenNode(){}

    //
    // Check all input parameters to see if any are connected.
    // If connected, then we consider the ModuleMessagingNode to be data-driven
    // (at this level of the the class hierarchy) and return TRUE,
    // otherwise FALSE.
    //
    virtual boolean isDataDriven();

    //
    // Notify all UI visuals that the attributes have changed.
    // If 'unmanage' is TRUE, then it is recommended that the visual
    // be unmanage making the changes.  This calls this->reflectStateChange()
    // when undefered.
    //
    void notifyVisualsOfStateChange(boolean unmanage = FALSE);

    //
    // in addition to superclass' work, change the node name parameter.
    //
    virtual int assignNewInstanceNumber();

    //
    // Provide methods to delay notifyVisualsOfStateChange().
    //
    boolean isVisualNotificationDeferred()   
			{ return this->visualNotificationDeferrals != 0;}
    void deferVisualNotification()   {this->visualNotificationDeferrals++;}
    void undeferVisualNotification(boolean unmanage = FALSE)
                        {  ASSERT(this->visualNotificationDeferrals > 0);
                           if ((--this->visualNotificationDeferrals == 0) &&
                               this->visualsNeedNotification)
                                this->notifyVisualsOfStateChange(unmanage);
			}

    //
    // Determine if this node is a node of the given class
    //
    virtual boolean isA(Symbol classname);

    virtual boolean hasJavaRepresentation() { return TRUE; }
    virtual const char* getJavaNodeName() { return "DrivenNode"; }

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassDrivenNode;
    }
};


#endif // _DrivenNode_h
