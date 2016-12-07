/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _SelectionNode_h
#define _SelectionNode_h


#include "InteractorNode.h"
#include "../base/DeferrableAction.h"
#include "List.h"

class Network;

//
// Class name definition:
//
#define ClassSelectionNode	"SelectionNode"


//
// SelectionNode class definition:
//				
class SelectionNode : public InteractorNode 
{
    friend class SelectionInstance;	

  private:
    //
    // Private member data:
    //
    //
    // This is used during parsing of the .cfg, to delay setting of the 
    // selected option.
    //
    static      int OptionsLeftToSet;

    int		optionCount;
    boolean	alwaysEnlistOutputs;

    static void InstallNewOptions(void *staticdata, void *requestData);
    DeferrableAction *deferNewOptionInstallation;

    //
    // Update the output values based on the current selections, then send
    // the values to the server.
    //
    void updateOutputs(boolean fromServer = FALSE);

    //
    // Update the selected option index, and if requested, update the output
    // values to match the values indicated by the index.
    //
    void changeSelectedOptionIndex(int index, boolean set,
                        boolean send = TRUE, boolean update_outputs = TRUE);


    //
    // Update the selected option index, and if requested, update the output
    // values to match the values indicated by the index.
    //
#if 00 // Not used
    void removeSelectedOptionIndex(int index, boolean send = TRUE, 
						boolean update_outputs = TRUE);
#endif
    void addSelectedOptionIndex(int index, boolean send = TRUE, 
						boolean update_outputs = TRUE);
    void clearSelections(boolean send = TRUE, boolean update = TRUE);

  protected:
    //
    // Protected member data:
    //

    List	selectedOptions;

    boolean cfgPrintInteractorAuxInfo(FILE *f);
    boolean cfgPrintSelectionsComment(FILE *f);
    boolean cfgPrintOptionComments(FILE *f);

    //
    // Parse comments the 'option[' comment. 
    //
    boolean cfgParseOptionComment(const char *comment,
					const char *filename, int lineno);

    //
    // Parse comments the 'selections: ' comment. 
    //
    boolean cfgParseSelectionsComment(const char *comment,
					const char *filename, int lineno);

    // The messages we parse can contain one or more of the following...
    //
    //      'value list={...}' or 'string list={...}'
    //
    // If any input or output values are to be changed, don't send them
    // because the module backing up the interactor will have just executed
    // and if the UI is in 'execute on change' mode then there will be an
    // extra execution.
    //
    // Returns the number of attributes parsed.
    //
    virtual int handleInteractorMsgInfo(const char *line);

    //
    // Define the mapping of inputs that shadow outputs.
    // By default, all data driven interactors, have a single output that is
    // shadowed by the third (hidden) input.
    // Returns an input index (greater than 1) or 0 if there is no shadowing 
    // input for the given output index.
    //
    virtual int getShadowingInput(int output_index);

    //
    // If either of the list inputs has changed, we must determine the size
    // of the smaller of the two lists and reset this->optionCount to that 
    // value.  
    // NOTE: If this routine is being called, then one of the values must 
    //    have a set value (even though both are allowed not too).
    //
    void ioParameterStatusChanged(boolean input, int index,
				NodeParameterStatusChange status);

    boolean setValueOptionsAttribute(const char *vlist);
    boolean setStringOptionsAttribute(const char *slist);
    boolean initValueOptionsAttribute(const char *vlist);
    boolean initStringOptionsAttribute(const char *slist);
    const char *getValueOptionsAttribute();
    const char *getStringOptionsAttribute();

    //
    // Get a a SelectorInstance instead of an InteractorInstance.
    //
    InteractorInstance *newInteractorInstance();

    //
    // These define the initial values 
    // Also, nice to force this class to be pure abstract.
    //
    virtual const char *getInitialValueList() = 0;
    virtual const char *getInitialStringList() = 0;

  public:
    //
    // Constructor:
    //
    SelectionNode(NodeDefinition *nd, Network *net, 
				int instnc, 
				boolean alwaysEnlistOutputs = FALSE);

    //
    // Destructor:
    //
    ~SelectionNode();

    //
    // Get the number of components for this interactor.
    //
    int getSelectedOptionCount() { return this->selectedOptions.getSize(); }
    int getOptionCount() { return this->optionCount; }

    //
    // Get the value of the indicated option.
    // The returned string must be deleted by the caller.
    //
    char  *getOptionValueString(int optind);
    //
    // Get the name of the indicated option.
    // The return string is not double quoted unless keep_quotes is TRUE.
    // Ther returned string must be deleted by the caller.
    //
    char  *getOptionNameString(int optind, boolean keep_quotes = FALSE);

    boolean appendOptionPair(const char *val, const char *label);

    //  
    // Use the value list and string list to set the new list of options.
    // Also, notify all instances that the state has changed.
    //
    void installNewOptions(const char *vlist, const char *slist, boolean send);


    //
    // Called once for each class by the allocator in definition. 
    // 
    virtual boolean initialize();

    //
    // Parse comments found in the .cfg that the InteractorNode knows how to
    // parse plus ones that it does not.
    //
    virtual boolean cfgParseComment(const char *comment,
					const char *filename, int lineno);

    //
    // Return true of the given index'ed option is selected.
    //
    boolean isOptionSelected(int index);

    //
    // Determine if this node is a node of the given class
    //
    virtual boolean isA(Symbol classname);


    //
    // Update the selected option index, and if requested, update the output
    // values to match the values indicated by the index.
    //
    void setSelectedOptions(int *setIndices, int count, 
				boolean send = TRUE, 
				boolean update_outputs = TRUE);

    virtual boolean printJavaType(FILE*, const char*, const char*);

    virtual const char* getJavaNodeName() { return "SelectionNode"; }
    virtual boolean printJavaValue(FILE*);

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassSelectionNode;
    }
};


#endif // _SelectionNode_h


