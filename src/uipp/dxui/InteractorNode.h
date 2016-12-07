/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



// InteractorNode.h -							   
//                                                                        
// Definition for the InteractorNode class.				 
//
// The InteractorNode is represented by the StandIn that is visible in the
// Editor work space.  Each InteractorNode has a family of 
// InteractorInstance/Interactor pairs.  This family is of unlimited size,
// and different pairs can reside in different control panels.
// The InteractorNode maintains a list of InteractorInstances (which in turn
// reference their associated Interactor).
// 
// We redefine this->openDefaultWindow() to (by default) open the control
// panel that the InteractorInstance(s) are contained in.  If you want
// a different action for you derived class, redefine this.
// 
// We also implement this->reflectStateChange()
// which calls InteractorInstance::handleInteractorStateChange() on each of
// the InteractorInstances in the list of associated instances.  
// 
// this->newInteractorInstance() and this->addInstance() are used during
// .cfg file parsing to allocate a new InteractorInstance for (what is
// usually) a derived class and add the instance to this->instanceList.
// Redefine newInteractorInstance() if you need other than an
// InteractorInstance class instance for your InteractorNode derived
// class.
// 
// Apart from the above we (re)define the standard parsing and printing
// methods.  We also define setOutputValues() so that all
// InteractorInstances  associated with this InteractorNode have their
// displayed value updated by calling
// this->setOutputAndOtherInteractorValues().  Typically, an Interactor
// will change it's nodes output value which results in all other
// Interactors having their displayed value change.
//
// All InteractorNodes are by default data-driven as defined by 
// DrivenNode::isDataDriven().  See the NondrivenInteractorNode sub-class
// for interactor nodes that are not data-driven.  A general discussion of
// data-driven interactors follows...
//
// Data-driven interactors are interactors that have inputs and make an 
// executive module call and then expect a UImessage from that module 
// concerning the state of the Interactor (i.e. mininum, maximum, increment, 
// label...).  this->Node::netPrintNode() determines if the InteractorNode is 
// data-driven with this->DrivenNode::expectingModuleMessage() via 
// this->DrivenNode::isDataDriven() and arranges
// for this->execModuleMessageHandler() to be called when a message for
// this InteractorNode is found.
//
// The following inputs are assumed at this level...
//      Input 1: Unique Id string identifying the instance of this node.
//      Input 2: Field/group input object.
//      Input 3: Current output value.
//         . . .
//      Input N: The label for this interactor (this is the last input).
//
// Because this class is derived from the ShadowedOutputNode class,
// when this->setOutputValue() is called shadowing inputs are updated
// to set to the values of any inputs that may be shadowing the given
// output.  Input to Output shadowing is defined in the virtual function
// this->getShadowingInput().  A one to one mapping of input to output is
// assumed in the overall architecture and by default output 1 is shadowed
// by input 3 when this->isDataDriven() returns TRUE.
//
//
//
//////////////////////////////////////////////////////////////////////////////

#ifndef _InteractorNode_h
#define _InteractorNode_h


#include "ShadowedOutputNode.h"
#include "InteractorStyle.h"

typedef long Type;

class Network;
class InteractorInstance;
class ComponentAttributes;
class ControlPanel;

//
// Class name definition:
//
#define ClassInteractorNode	"InteractorNode"

//
// InteractorNode class definition:
//				
class InteractorNode : public ShadowedOutputNode
{
    friend class ControlPanel;	// Needs to be able to delete instances.
    friend class SetAttrDialog; // Needs to un/deferVisualNotification().
    friend class InteractorInstance; // Needs direct access to instanceList.

  private:
    //
    // Private member data:
    //
    char* java_variable;

  protected:
    //
    // Protected member data:
    //

    //
    // Used by getInteractorLabel() so it can return a const char *.
    //
    char 	*lastInteractorLabel;	

    //
    // numComponents is the value parsed from the .cfg file and may
    // be interpretted differently depending upon the derived class.
    //
    int		numComponents;

    List	instanceList;	// List of InteractorInstances for this node.

    virtual boolean cfgParseInteractorComment(const char* comment,
                		const char* filename, int lineno);
    virtual boolean cfgParseInstanceComment(const char* comment,
                		const char* filename, int lineno);
    virtual boolean cfgParseLabelComment(const char* comment,
                		const char* filename, int lineno);

    virtual boolean cfgPrintInteractor(FILE *f);
    virtual boolean cfgPrintInteractorComment(FILE *f);
    virtual boolean cfgPrintInteractorAuxInfo(FILE * /* f */) { return TRUE; };
    virtual boolean cfgPrintInteractorInstances(FILE *f, PrintType dest);
    virtual boolean cfgPrintInstanceComment(FILE *f,
						 InteractorInstance *ii);
    virtual boolean cfgPrintInstanceLabelComment(FILE *f, 
						 InteractorInstance *ii);
    virtual boolean cfgPrintInstanceAuxInfo(FILE * /* f */, 
						InteractorInstance * /* ii */)
			{ return TRUE; }
    
    boolean	appendInstance(InteractorInstance *ii)
		  { return this->instanceList.appendElement((void*)ii); }
    //
    // Get a new interactor instance for this class.
    // Derived classes can override this to allocated subclasses of 
    // InteractorInstance which may be specific to the derived class.
    // For example, ScalarNode uses a ScalarInstance instead
    // of an InteractorInstance which incorporates local modes.
    //
    virtual	InteractorInstance *newInteractorInstance();


    // 
    // Delete and free an instance from the list of instances.
    // This may be called by a ControlPanel.
    //
    boolean	deleteInstance(InteractorInstance *ii);

    // 
    // Create a new interactor instance (using newInteractorInstance()) 
    // giving it the given position and style and assigning it to the given 
    // ContorlPanel, if given.  This includes adding the instance to the 
    // control panels list of instances.  
    // Returns the InteractorInstance on success, NULL otherwise.
    //
    InteractorInstance *addInstance(int x, int y, 
				InteractorStyle *is,
				ControlPanel *cp = NULL,
				int width = 0, int height = 0);
    //
    //  Get the index'th interactor instance for this Interactor.
    //  index is 1 based.
    //
    InteractorInstance *getInstance(int index) 
		{
		    ASSERT(index > 0);
		    return (InteractorInstance*)
				this->instanceList.getElement(index);
		}
    //
    // Called when a message is received from the executive after
    // this->ExecModuleMessageHandler() is registered in
    // this->Node::netPrintNode() to receive messages for this node.
    // We parse all the common information and then the class specific.
    // We return the number of items in the message.
    //
    virtual int handleNodeMsgInfo(const char *line);


    //
    // Parse the interactor specific info from an executive message.
    // Returns the number of attributes parsed.
    //
    virtual int  handleInteractorMsgInfo(const char *line) = 0;

    //
    // Parse attributes that are common to all data-driven interactors.
    // Parses and sets the label from a message string.
    // Returns the number of recognized message items.
    //
    int handleCommonMsgInfo(const char *line);

    //
    // Update all interactor instances that may be based on the state of this
    // node.   Among other times, this is called after receiving a message
    // from the executive.
    //
    virtual void reflectStateChange(boolean unmanage);

    //
    // Define the mapping of inputs that shadow outputs.
    // By default, all data driven interactors, have a single output that is
    // shadowed by the third input.
    // Returns an input index (greater than 1) or 0 if there is no shadowing
    // input for the given output index.
    //
    virtual int getShadowingInput(int output_index);

    //
    // Get the index of the label parameter.
    // Be default, it is always the last parameter.
    //
    virtual int getLabelParameterIndex();

    //
    // Set all shadowing inputs to use the default value.
    // This is most likely used during initialization after setting the outputs
    // (which sets the shadowing inputs).
    //
    void setShadowingInputsDefaulting(boolean send = FALSE);

    //
    // Notify anybody that needs to know that a parameter has changed its arcs.
    // At this class level, we just check changes in output arcs that may
    // change the label associated with the Interactor.  If it may have 
    // changed the label, then we notify all instances with 
    // notifyVisualsOfStateChange().
    //
    virtual void ioParameterStatusChanged(boolean input, int index, 
				NodeParameterStatusChange status);

    //
    // Print the script representation of the call for interactors.
    // For interactors, there is no executive call, unless we are being
    // data driven.  The work done here is to determine if we are acting
    // as a data-driven interactor and then to do the correct action.
    // If we are not data-driven, return "" since there is no work for
    // the executive to do for us.
    // If we are data-driven, then we generate the call to the correct
    // executive module by calling the superclass' method.
    //
    virtual char *netNodeString(const char *prefix);

    //
    // Change the dimensionality of the vector;
    // At this level, we don't allow dimensionality changes so we always
    // return FALSE;
    //
    boolean changeDimensionality(int new_dim);
    virtual boolean doDimensionalityChange(int new_dim);

#if 0	// 8/9/93
    //
    // Determine if the given input parameter is writeable as an attribute.
    //
    virtual boolean isAttributeVisuallyWriteable(int input_index);
#endif

  public:
    //
    // Constructor:
    //
    InteractorNode(NodeDefinition *nd, Network *net, int instnc);

    //
    // Destructor:
    //
    ~InteractorNode();

    //
    // Return a pointer to a string representing the global name that can
    // be used to label interactors.   This can be superseded by a local label
    // string maintained in InteractorInstance. 
    // The algorithm we use is as follows...
    //  If there are no output arcs  or more than 1 use "Value:" 
    //	If there is a single output arc, use the name of the destination 
    //	    node and parameter.
    //
    char *getOutputDerivedLabel();
    virtual const char *getInteractorLabel();

    //
    // Set the global label for all instances of this interactor node.
    // DrivenInteractors keep their labels in the label parameter.
    // If strip_quotes is TRUE, then remove leading and trail double quotes
    // which are expected to be present.
    //
    void saveInteractorLabel(const char *label, boolean strip_quotes = FALSE);


    //
    // Get the current type of the given output.
    // Be default, this is the type of current output value or if no value is
    // currently set, then the first (and only?) type in the parameters type 
    // list. If an interactor can have different outputs, that class should
    // override this.
    //
    Type  getTheCurrentOutputType(int index);

    //
    // Redefine this so that we call the super-class method and then
    // check for the input comment for the label parameter value (if we have
    // one).  If we see the label parameter value then save it.
    //
    virtual boolean netParseComment(const char* comment,
                                const char *file, int lineno);

    virtual boolean cfgParseComment(const char* comment,
                		const char* filename, int lineno);

    //
    // Routine for printing the .cfg file contents for this interactor.
    //
    virtual boolean cfgPrintNode(FILE *f, PrintType dest); 

    
    //
    // Get the number of instances for this interactor. 
    //
    int	getInstanceCount()	{ return instanceList.getSize(); }

    int getComponentCount()	{ return this->numComponents; }

    //
    // Calls setOutputAndOtherInteractorValues to update all interactor
    // instances.
    //
    virtual Type setOutputValue(int index,
                                const char *value,
                                Type t = DXType::UndefinedType,
                                boolean send = TRUE);


    //
    // Indicates whether this node has outputs that can be remapped by the
    // server.
    //
    virtual boolean hasRemappableOutput();
    //
    // Do what ever is necessary to enable/disable remapping of output values
    // by the server.
    //
    virtual void setOutputRemapping(boolean val);

    //
    // The default action for interactors is to open the control panels
    // associated with the instances.  This overrides Node::openDefaultWindow()
    //
    virtual void openDefaultWindow(Widget parent);

    //
    // Let the caller of openDefaultWindow() know what kind of window she's getting.
    // This is intended for use in EditorWindow so that we can sanity check the number
    // of cdbs were going to open before kicking off the operation and so that we
    // don't question the user before opening large numbers of interactors.
    // A name describing the type of window can be written into window_name in order
    // to enable nicer warning messages.
    //
    virtual boolean defaultWindowIsCDB(char* window_name = NULL)
	{ if (window_name) strcpy (window_name, "Interactor"); return FALSE; }
    //
    // If the node does not currently have any InteractorInstances, then
    // add them to the editor's/network's notion of the current control panel.
    //
    void openControlPanels(Widget parent);

    //
    // Does this node support dimensionality changes.
    // By default all interactors do NOT support this.
    //
    virtual boolean hasDynamicDimensionality(boolean ignoreDataDriven = FALSE);

    //
    // Determine if this node is a node of the given class
    //
    virtual boolean isA(Symbol classname);


    //
    // just like deleteInstance except don't delete ii, just remove it from the list.
    //
    boolean removeInstance (InteractorInstance *ii);

    //
    // Return TRUE if this node has state that will be saved in a .cfg file.
    //
    virtual boolean hasCfgState();

    virtual boolean printAsJava(FILE* );
    virtual boolean printJavaValue(FILE*);
    virtual const char* getJavaVariable();
    virtual const char* getJavaNodeName() { return "ValueNode"; }

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassInteractorNode;
    }
};


#endif // _InteractorNode_h
