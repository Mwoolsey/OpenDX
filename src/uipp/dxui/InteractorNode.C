/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"

#include <stdio.h>

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif
#include <ctype.h>
 
#include "Application.h"
#include "EditorWindow.h"
#include "ErrorDialogManager.h"
#include "WarningDialogManager.h"
#include "InteractorNode.h"
#include "InteractorInstance.h"
#include "Interactor.h"
#include "InteractorStyle.h"
#include "ControlPanel.h"
#include "DXType.h"
#include "Network.h"
#include "DXApplication.h"
#include "Parameter.h"
#include "Ark.h"
#include "ListIterator.h"
#include "TransmitterNode.h"
#include "ReceiverNode.h"

#define ID_PARAM_NUM 	1
#define DATA_PARAM_NUM 	2


//
// The Constructor... 
//
InteractorNode::InteractorNode(NodeDefinition *nd, Network *net, int instnc) :
                        ShadowedOutputNode(nd, net, instnc)
{ 
    this->numComponents = 1;
    this->lastInteractorLabel = NULL;
    this->java_variable = NUL(char*);
}

//
// Destructure: needs to delete all its instances. 
//
InteractorNode::~InteractorNode()
{
    InteractorInstance *ii;
    ListIterator iterator(this->instanceList);

    //
    // Delete all InteractorInstances associated with this node 
    //
    while ( (ii = (InteractorInstance*)iterator.getNext()) ) 
	this->deleteInstance(ii);

    if (this->lastInteractorLabel) 
        delete this->lastInteractorLabel;
    
    if (this->java_variable)
	delete this->java_variable;
}
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
char *InteractorNode::netNodeString(const char *prefix)
{
    if ((this->getInputCount() != 0) && this->isDataDriven()) {
        // Assuming ShadowedOutputNode is the super class for InteractorNode.
        // What we want here is for a call to the node to be made
        // in the executive.
        return this->ShadowedOutputNode::netNodeString(prefix);
    } else {
	char *string = new char[1];
	string[0] = '\0';
	return string;
    }
}

//
// Notify anybody that needs to know that a parameter has changed its arcs.
// At this class level, we just check changes in output arcs that may
// change the label associated with the Interactor.  If it may have 
// changed the label, then we notify all instances with 
// notifyVisualsOfStateChange().
//
void InteractorNode::ioParameterStatusChanged(boolean input, int index, 
					NodeParameterStatusChange status)
{
    boolean doit = FALSE;

    this->deferVisualNotification();

    if (status & Node::ParameterArkChanged) { 
	if (input) {
	    if (index == this->getLabelParameterIndex())
		doit = TRUE;
	    //
	    // If we become un data driven, then we must make sure the
	    // outputs get sent on the next execution.
	    //
	    if (!this->isDataDriven()) {
		int i, ocnt = this->getOutputCount();
		for (i=1 ; i<=ocnt ; i++)
		    this->setOutputDirty(i);
		// Don't need to send them because the network will get marked
		// dirty as a result of an arc change.
	    }
	} else {
	    //
	    // Count the number of output arcs.
	    //
	    int i, narcs = 0, outputs = this->getOutputCount();
	    for (i=1 ; i<=outputs ; i++) {
		List *arcs = (List*)this->getOutputArks(i);
		narcs += arcs->getSize();
	    }
	    //
	    // If the number of arcs has changed in such a way that the label
	    // may have to change then notify the instances to reset their 
	    // labels.
	    // Labels can change if we ADD an arc and change from having...
	    //	a) 0 to having 1 arc
	    //	b) 1 to having 2 arcs 
	    // or we REMOVE an arc and we change from having...
	    //   	a) 1 to having 0 arcs
	    //	b) 2 to having 1 arc 
	    //
	    boolean added = status == Node::ParameterArkAdded;
	    if ((narcs <= 1) || (added && narcs == 2))
		doit = TRUE;
	} 
	if (doit)
	    this->notifyVisualsOfStateChange();
    }

    this->ShadowedOutputNode::ioParameterStatusChanged(input, index, status);

    this->undeferVisualNotification();
}

//
// Return a pointer to a string representing the global name that can
// be used to label interactors.   This can be superseded by a local label
// string maintained in InteractorInstance. 
// The algorithm we use is as follows...
//  If there are no output arcs  or more than 1 use "Value:"
//  If there is a single output arc, use the name of the destination
//      node and parameter.
// The returned string must be freed by the caller.
//
char *InteractorNode::getOutputDerivedLabel()
{
    Ark *the_arc=NULL;
    char *s = NULL;
    int i, outputs = this->getOutputCount();
    int	total_arcs = 0;


    //
    // Count the total number of arcs and save the first arc found.
    //
    for (i=1 ; i<=outputs ; i++) {
	List *arcs = (List*)this->getOutputArks(i);
	int narcs = arcs->getSize();
	if ((narcs == 1) && total_arcs == 0)
	   the_arc = (Ark*)arcs->getElement(1);
	total_arcs += narcs;
    }

    if (total_arcs == 1) {		
	//
	// Get the name of parameter THE arc is connected to
	//
	const char *nodename;
	char *varname;
	int input_index;
	Node *node = the_arc->getDestinationNode(input_index);
	ASSERT(node);
	nodename = node->getNameString();
	varname = node->getInputNameString(input_index);
	int len = STRLEN(varname) + STRLEN(nodename) + 3;
	s = new char[len];
	sprintf(s,"%s %s:",nodename,varname);
	delete varname;
    } else {
	s = DuplicateString((char*)this->getLabelString());
    }

    return s;
}
//
// Data driven interactors can have their labels driven by another
// node, in which case we don't try and determine the label name based
// on our output connections (i.e. like InteractorNode::getInteractorLabel()).
// Instead we just use the current value or the default label.
//
const char *InteractorNode::getInteractorLabel()
{
    const char *label = NULL;
    int lindex = this->getLabelParameterIndex();
    
    if (lindex > 0) {			// This node as a label input
	if  (!this->isInputDefaulting(lindex) || // Parameter has value
	      this->isInputConnected(lindex)) {	 // Parameter is connected
	    //
	    // Get the value from the last UI/EXEC message.
	    //
	    label = this->lastInteractorLabel;
	} 
    }

    if (!label) {
	//
	// Either there is no label parameter, the label is not data-driven,
	// or we have not yet received a message from the exec telling us 
	// what the value should be or the label parameter does not have 
	// a value.  In any case use the default label.
	//
	char *s = (char*)this->getOutputDerivedLabel();
	if (this->lastInteractorLabel)
	    delete this->lastInteractorLabel;
    	this->lastInteractorLabel = s;
	label = (const char*)s;
    }
    return label;
}

//
// Redefine this so that we call the super-class method and then
// check for the input comment for the label parameter value.  If
// we see the label parameter value then save it.
//
boolean     InteractorNode::netParseComment(const char* comment,
					const char *file, int lineno)
{
    boolean r = this->ShadowedOutputNode::netParseComment(comment,file,lineno);
    int lindex = this->getLabelParameterIndex();
    if (r && (lindex > 0)) { 
	const char *label = this->getInputSetValueString(lindex);
	if (!EqualString(label,"NULL"))      // Parameter has value.
	    this->saveInteractorLabel(label, TRUE);
    }
    return r;
}

//
// Print the information that belongs in the configuration file (.cfg) for 
// this node.
//
boolean InteractorNode::cfgPrintNode(FILE *f, PrintType dest)
{
    if (fprintf(f, "//\n") < 0)
	return FALSE;

    if (!this->Node::cfgPrintNode(f, dest)  	||
        !this->cfgPrintInteractor(f)		||
        !this->cfgPrintInteractorInstances(f,dest))
	return FALSE;

    return TRUE;
}
boolean InteractorNode::cfgParseComment(const char* comment,
                const char* filename, int lineno)
{
    ASSERT(comment);

    /*
     * Try and parse the 'interactor' or 'instance' comment.
     */
    return this->cfgParseInteractorComment(comment, filename, lineno) ||
           this->cfgParseInstanceComment(comment, filename, lineno) ||
           this->cfgParseLabelComment(comment, filename, lineno) ||
	   this->Node::cfgParseComment(comment, filename, lineno);
}

//
// Print general information about an interactor  
//
boolean InteractorNode::cfgPrintInteractorInstances(FILE *f, PrintType dest)
{
    InteractorInstance *ii;
    ListIterator iterator(this->instanceList);
    ControlPanel *ownerCp;

    ownerCp = this->getNetwork()->getSelectionOwner();
    while ( (ii = (InteractorInstance*)iterator.getNext()) ) { 
	if (dest == PrintCPBuffer) {
	    if (!ii->isSelected()) continue;
	    if (ii->getControlPanel() != ownerCp) continue;
	}
        if (!this->cfgPrintInstanceComment(f, ii) 	|| 
	    !this->cfgPrintInstanceLabelComment(f, ii)	||
	    !this->cfgPrintInstanceAuxInfo(f, ii))
		return FALSE;
    }
    return TRUE;
}
//
// Print general information about an interactor  
//
boolean InteractorNode::cfgPrintInteractor(FILE *f)
{
    return this->cfgPrintInteractorComment(f) &&
	   this->cfgPrintInteractorAuxInfo(f);
}
//
// Print the '// interactor...' comment  to the file.
//
boolean InteractorNode::cfgPrintInteractorComment(FILE *f)
{
    return fprintf(f, 
		"// interactor %s[%d]: num_components = 1, value = %s\n",
		this->getNameString(),
		this->getInstanceNumber(),
		this->getOutputValueString(1)) >= 0;
}

//
// The default routine to parse the interactor comment.
// Lookup up the interactor in the current network and set the current output
// value.
//
boolean InteractorNode::cfgParseInteractorComment(const char* comment, 
		const char* filename, int lineno)
{
    int        num_components, instance, items_parsed;
    char       value[4069];
    char       interactor_name[64];
    
    if (strncmp(" interactor",comment,11))
	return FALSE;

    items_parsed =
	sscanf(comment,
	       " interactor %[^[][%d]: num_components = %d, value = %[^\n]",
	       interactor_name,
	       &instance,
	       &num_components,
	       value);

    /*
     * If all items found...
     */
    if (items_parsed != 4)
    {
#if 0
	uiuModelessErrorMessageDialog
	    (_parse_widget, "#10001", "interactor", _parse_file, yylineno);
#else
	ErrorMessage("Bad interactor comment file %s line %d\n", 
					filename,lineno);
#endif
	return TRUE;
    }

    ASSERT(!strcmp(this->getNameString(), interactor_name));
    

#if 1
    if (num_components < 0) {
	ErrorMessage("Bad 'num_components' value, file %s line %d\n",
					filename, lineno);
	return TRUE;
    }

    if (this->numComponents != num_components) { 
	if (!this->hasDynamicDimensionality(TRUE)) {
	    ErrorMessage(
	    "Unmatched num_components and %s does not allow dimensionality"
	    " changes (file %s line %d)\n", 
	    this->getNameString(), filename, lineno);
	    return TRUE;
	}
	if (!this->changeDimensionality(num_components)) {
	    ErrorMessage(
	    "Dimensionality adjustment failed for %s"
	    " (file %s line %d)\n", 
	    this->getNameString(), filename, lineno);
	    return TRUE;
	}
    }
#endif

#if 1
    if (this->setOutputValue(1,value,DXType::UndefinedType, FALSE) == 
				DXType::UndefinedType) {
	ErrorMessage(
		"Encountered an erroneous output value (file %s, line %d).",
			filename, lineno);	
	return TRUE;
   }
	
#else
    tmp_type = uinSetOutputValue(_network, _node_index, 0, value);
    if (tmp_type == MT_NONE)
    {
	uiuModelessErrorMessageDialog
	    (_parse_widget,
	     "Encountered an erroneous output value (%s file, line %d).",
	     _parse_file,
	     yylineno);
	_error_occurred = TRUE;
	return;
    }

    if (tmp_type == MT_OBJECT)
    {
	if (EqualString(_interactor_name, "VectorList"))
	{
	    node->output->value.vector_list.n_tuple = num_components;
	}
    }

    node->output[0].value_in_use = TRUE;

    /*
     * Reset interactor index;
     */
    _interactor_index = -1;

    /*
     * Comment parsed successfully:  set the parse state.
     */
    _parse_state = _PARSED_INTERACTOR;
#endif // 0

    return TRUE;
}

// 
// Print the 'instance: ...' comment for this instance of the InteractorNode. 
//
boolean InteractorNode::cfgPrintInstanceComment(FILE *f, 
					InteractorInstance *ii)
{
    InteractorStyle *is; 
    int            x,y;
    int            width,height;

    ii->getXYPosition(&x, &y);
    ii->getXYSize(&width, &height);
    is = ii->getStyle();

    ASSERT(is);

    return fprintf(f, 
	"// instance: panel = %d, x = %d, y = %d, style = %s, vertical = %d, size = %dx%d\n",
	ii->getControlPanel()->getInstanceNumber() - 1, // 0 indexed in file
	x, 
	y,
	is->getNameString(),
	ii->isVerticalLayout() ? 1 : 0,
	width,
	height) >= 0;

}

// Parse any cfg comment other than the 'interactor' comment.
// In general at this level InteractorNodes are considered not to have 
// additional .cfg comments.  If they do than the subclass must implement
// this routine.
//
boolean InteractorNode::cfgParseInstanceComment(const char* comment, 
		const char* filename, int lineno)
{
    int            items_parsed;
    int            panel_id;
    InteractorStyle *is = NUL(InteractorStyle*);
    InteractorStyleEnum style=UnknownStyle;
    int            vertical, x,y,width,height;
    char           style_name[256];
    InteractorInstance *ii;
 
    if (strncmp(" instance: panel",comment,16))
	return FALSE;

#if 0	// FIXME: This is for temporary backwards compatiblity with .cfg
	// files that were saved with versions of dxui dated between 12/31/93
 	// and 1/15/94 
    items_parsed =
          sscanf(comment,
           " instance: panel = %d, x = %d, y = %d, width = %d, height = %d, style = %[^,], vertical = %d",
               &panel_id,
               &x,
               &y,
	       &width,
	       &height,
               style_name,
	       &vertical);
    if (items_parsed != 7) 
#endif

    items_parsed =
          sscanf(comment,
           " instance: panel = %d, x = %d, y = %d, style = %[^,], vertical = %d, size = %dx%d",
               &panel_id,
               &x,
               &y,
               style_name,
	       &vertical,
	       &width,
	       &height);

    if (items_parsed != 7) {

	items_parsed =
	    sscanf(comment,
	      " instance: panel = %d, x = %d, y = %d, style = %[^,], vertical = %d",
		   &panel_id,
		   &x,
		   &y,
		   style_name,
		   &vertical);
	width = 0;
	height = 0;
	
	if (items_parsed != 5) {
	    
	    items_parsed =
	      sscanf(comment,
	       " instance: panel = %d, x = %d, y = %d, style = %s",
		   &panel_id,
		   &x,
		   &y,
		   style_name);
	    vertical = 1;
	    width = 0;
	    height = 0;
	    
	    if (items_parsed != 4)
	    {
    #if 0
		uiuModelessErrorMessageDialog
		    (_parse_widget, "#10001", "instance", _parse_file, yylineno);
    #else
		ErrorMessage("Bad 'instance' comment (file %s, line %d)",
				    filename,lineno);
    #endif
		return FALSE;
	    }

	}
    }


#if 0
    if (EqualString(style_name, "vector") OR
	EqualString(style_name, "vector_list"))
    {
	n = 3;
    }
    else if (EqualString(style_name, "selector")    OR
	     EqualString(style_name, "string")      OR
	     EqualString(style_name, "string_list") OR
	     EqualString(style_name, "value")       OR
	     EqualString(style_name, "value_list"))
    {
	n = 0;
    }
    else
    {
	n = 1;
    }

    if (n > 0)
    {
	interactor->increment =
	    (struct _uinincrement *)uiuCalloc(n, sizeof(struct _uinincrement));
	interactor->restore_increment = 
	    (struct _uinincrement *)uiuCalloc(n, sizeof(struct _uinincrement));
	interactor->local_increment = 
	    (boolean*)uiuCalloc(n, sizeof(boolean*));
	interactor->restore_local_increment = 
	    (boolean*)uiuCalloc(n, sizeof(boolean*));
    }
#endif

    
    int major = this->getNetwork()->getNetMajorVersion();
    int minor = this->getNetwork()->getNetMinorVersion();
    int micro = this->getNetwork()->getNetMicroVersion();
    if (major < 2) {
	//
	// In version 2.0.0 (9/93) we changed style names
	//
	if (EqualString(style_name, "dial"))
	    style = DialStyle;
	else if (EqualString(style_name, "slider"))
	    style = SliderStyle;
	else if (EqualString(style_name, "stepper"))
	    style = StepperStyle;
	else if (EqualString(style_name, "vector"))
	    // the StepperInteractor implements this now
	    style = StepperStyle;
	else if (EqualString(style_name, "integer_list"))
	    style = IntegerListEditorStyle;
	else if (EqualString(style_name, "scalar_list"))
	    style = ScalarListEditorStyle;
	else if (EqualString(style_name, "vector_list"))
	    style = VectorListEditorStyle;
	else if (EqualString(style_name, "text"))
	    style = TextStyle;
	else if (EqualString(style_name, "option_menu"))
	    style = SelectorOptionMenuStyle;
	else if (EqualString(style_name, "string_list"))
	    style = StringListEditorStyle;
	else if (EqualString(style_name, "value_list"))
	    style = ValueListEditorStyle;
	is = NULL;	// Look up by style below. 
    } else if (major == 2 && minor == 0 && micro == 1 && 
				EqualString(style_name, "Selector")) {
	//
	// Changed name of 'Selector' style to 'Option Menu' style.
	//
	style = SelectorOptionMenuStyle;
    } else {
	// 
	//  Version 2.0.0 and later .net files contain real style names. 
	// 
	is = InteractorStyle::GetInteractorStyle(this->getNameString(),
								style_name);
        style = DefaultStyle;  // In case the above failed.
    }

    if (!is) {
	is = InteractorStyle::GetInteractorStyle(this->getNameString(),style);
	//
	// Apparently, some ValueList interactors were written as 'text' style.
	// in pre 2.0.0 .cfg files.
	//
	if (!is && (style == TextStyle)) {
	    style = TextListEditorStyle;
	    is = InteractorStyle::GetInteractorStyle(this->getNameString(),
			style);
	}
	//
	// If we haven't already tried, try the default style. 
	//
	if (!is && (style != DefaultStyle)) {
	    style = DefaultStyle;
	    is = InteractorStyle::GetInteractorStyle(this->getNameString(),
		    style);
	}
	if (!is ) {
	    ErrorMessage("Could not find default interactor style to replace "
			 "unsupported style (file %s, line %d)", 
			filename, lineno);
	    return FALSE;
	}
    }
    
	
    ControlPanel *cp = this->getNetwork()->getPanelByInstance(panel_id+1);
    if (!cp ) {
	ErrorMessage(
	    "'instance' comment refers to undefined panel (file %s, line %d)", 
	    filename,lineno); 
   	return TRUE;
    }

#if 0
    if(NOT cp->updated)
    {
	cp->clearInstances();
	cp->updated = TRUE;
    }
#endif

    ii = this->addInstance(x, y, is, cp, width, height );
    if (!ii)
	return TRUE;

    ii->setVerticalLayout(vertical == 0 ? FALSE : TRUE);

    if(cp->isManaged())
	ii->createInteractor();

    return TRUE;
}
//
// Print the interactor instance's label comment.
//
boolean InteractorNode::cfgPrintInstanceLabelComment(FILE *f, 
						InteractorInstance *ii)

{
    const char *l = ii->getLocalLabel();

    if (l) {
        if (fprintf(f, "// label: value = ") < 0)
	    return FALSE;
	while (*l) {
	    char c = *l;
	    if (c == '\n') {
		if (fputc('\\', f) == EOF)
		    return FALSE;
		c = 'n';
	    }
	    if (fputc(c, f) == EOF)
		return FALSE;
	    l++;
	}   
	if (fputc('\n', f) == EOF)
	    return FALSE;
    }

    return TRUE;
}
boolean InteractorNode::cfgParseLabelComment(const char* comment, 
				const char *filename, int lineno)
{
    int            items_parsed;
    char           label[256];

    ASSERT(comment);

    if (strncmp(comment," label",6))
	return FALSE;

    items_parsed = sscanf(comment, " label: value = %[^\n]", label);

    /*
     * If parsed ok and node exists, convert value.
     */
    if (items_parsed != 1)
    {
#if 1
	ErrorMessage("Bad .cfg interactor 'label' comment (file %s, line %d)",
			filename, lineno);
	return FALSE;
#else
        uiuModelessErrorMessageDialog
            (_parse_widget,
             "Encountered an erroneous label comment (%s file, line %d).",
             _parse_file,
             yylineno);
        _error_occurred = TRUE;
        return;
#endif
    }

    int instance = this->getInstanceCount();
    InteractorInstance *ii = this->getInstance(instance);
    if (!ii) {
       ErrorMessage("interactor 'label' comment out of order (file %s, line%d)",
                        filename, lineno);
    } else 
        ii->setLocalLabel(label);
    return TRUE;

}
InteractorInstance *InteractorNode::newInteractorInstance()
{
    InteractorInstance *ii;

    ii = new InteractorInstance(this);

    return ii;
}
// 
// Create a new interactor instance giving it the given position and style
// and assigning it to the ContorlPanel if given.  This includes adding the
// instance to the control panels list of instances.
//
InteractorInstance *InteractorNode::addInstance(int x, int y,
		InteractorStyle *is, ControlPanel *cp, int width, int height)

{
    InteractorInstance *ii;

    ii = this->newInteractorInstance();
    if (!ii)
	return NUL(InteractorInstance*);

    ii->setXYPosition(x, y);
    ii->setXYSize(width, height);
    ii->setStyle(is);

    // Add it to the control panel's list of instances.
    if (cp)
    	cp->addInteractorInstance(ii);

    // Add it to this node's list of instances. 
    if (!this->appendInstance(ii))
	return NUL(InteractorInstance*);
 
    this->getNetwork()->setFileDirty();

    return ii;
}

//
// Set the output value of an interactor.  This is the same as for
// Node::setOutputValue(), except that it also updates the interactors
// in this->instanceList with 
//  InteractorInstance->Interactor->updateDisplayedInteractorValue().
//
Type InteractorNode::setOutputValue( 
                                int index,
                                const char *value,
                                Type t,
                                boolean send)
{
    Type type;

    type = this->ShadowedOutputNode::setOutputValue(index, value, t, send);

    if ((type != DXType::UndefinedType) &&	// Set value succeeded.
        !this->isVisualNotificationDeferred()) {

	InteractorInstance *ii;
	ListIterator iterator(this->instanceList);
	//
	// Update all interactors associated with this node.
	//
	while ( (ii = (InteractorInstance*)iterator.getNext()) ) {
	    Interactor  *interactor = ii->getInteractor();
	    if (interactor)	// Only if the interactor/panel have been opened
		interactor->updateDisplayedInteractorValue();
	}
    }
    return type;
}

//
// The default action for interactors is to open the control panels
// associated with the instances.  
// If the node does not currently have any InteractorInstances, then
// add them to the editor's/network's notion of the current control panel.
//
// This overrides Node::openDefaultWindow()
//
void InteractorNode::openDefaultWindow(Widget parent)
{
    this->openControlPanels(parent);
}
//
// If the node does not currently have any InteractorInstances, then
// add them to the editor's/network's notion of the current control panel.
//
void InteractorNode::openControlPanels(Widget parent)
{
    InteractorInstance *ii;

    if (this->instanceList.getSize() > 0) {
	//
	// Open the ControlPanels of each InteractorInstance.
	//
        ListIterator iterator(this->instanceList);
	while ( (ii = (InteractorInstance*) iterator.getNext()) ) {
	    ControlPanel *panel = ii->getControlPanel();
	    panel->manage();	// Raise it even if it is managed.
	    ii->setSelected(TRUE);
	}
    } else {
	//
	// Create a new InteractorInstance for this Node.
	//

	// FIXME: nodes are not supposed to know about editors!!!
	EditorWindow *editor = this->getNetwork()->getEditor();
	ASSERT(editor);
	ControlPanel  *cp = editor->getCurrentControlPanel();
	ASSERT(cp);
	
	InteractorStyle *is = InteractorStyle::GetInteractorStyle(
					this->getNameString(), DefaultStyle);
	if (!is) {
	    ErrorMessage("Can't find default interactor style for '%s'",
						this->getNameString());
	} else {
	    int x,y;
	    cp->getNextInteractorPosition(&x,&y);
	    ii = this->addInstance(x, y, is, cp);
	    //
	    // This shouldn't be necessary every time but if we don't 
	    // have this here the last interactor in a series of 
	    // placements (double click on a set of selected interactors) 
	    // isn't visible.
	    //
	    cp->manage();
	    ASSERT(ii);
	    //ii->setSelected(TRUE);
	} 
    }

}

//
// Indicates whether this node has outputs that can be remapped by the
// server.
//
boolean InteractorNode::hasRemappableOutput()
{
    return FALSE;
}
//
// Do what ever is necessary to enable/disable remapping of output values
// by the server.
// At this level of the class hierarchy, we do nothing since
// hasRemappableOutput() returns FALSE.
//
void InteractorNode::setOutputRemapping(boolean val)
{
    return;
}

void InteractorNode::reflectStateChange( boolean unmanage)
{
    ListIterator iterator(this->instanceList);
    InteractorInstance *ii;

    while ( (ii = (InteractorInstance*) iterator.getNext()) ) 
	ii->handleInteractorStateChange(NULL, unmanage);
}
// 
// Delete and free an instance from the list of instances
// and notify the ControlPanel that the node has been deleted.
// 'delete ii' should take care of removing the Interactor from 
// the ControlPanel's work space.
// This may be called by a ControlPanel.
//
boolean InteractorNode::deleteInstance(InteractorInstance *ii)
{ 
    if (this->removeInstance(ii)) {
	delete ii;
	return TRUE;
    } 
    this->getNetwork()->setFileDirty();
    return FALSE;
}
boolean InteractorNode::removeInstance(InteractorInstance *ii)
{
    return this->instanceList.removeElement((void*)ii); 
}

//
// Get the current type of the given output.
// Be default, this is the type of current output value or if no value is
// currently set, then the first (and only?) type in the parameters type list.
// If an interactor can have different outputs, that class should
// override this. 
//
Type  InteractorNode::getTheCurrentOutputType(int index)
{
    Parameter *p = this->getOutputParameter(index);
    Type t;

    if (p->hasValue()) {
        t = p->getValueType();
	// 
	// Determine which list type we have here. 
	// 
	if (t == DXType::ValueListType) {
	    t = DXType::DetermineListItemType(p->getValueString());
	    ASSERT(t != DXType::UndefinedType);
	    t |= DXType::ListType;
	}
	//
	// The following is done for lists (and should really be in that
 	// class, but...).  Empty lists may get printed to the .net or .cfg
	// files as "NULL", so when they are read in, the type gets set to
	// OjbecType which is not really meaningful.  Instead we use the
	// default type.
	//
	if ((t == DXType::ObjectType) || 
	    EqualString("NULL", p->getValueString()))
	    t = p->getDefaultType();
    } else {
        t = p->getDefaultType();
    }
    return t;
}

//
// Does this node support dimensionality changes.
// By default all interactors do NOT support this.
//
boolean InteractorNode::hasDynamicDimensionality(boolean ignoreDataDriven)
{
    return FALSE;
}
boolean InteractorNode::changeDimensionality(int new_dim)
{
   this->getNetwork()->setFileDirty();
   return this->doDimensionalityChange(new_dim);
}
boolean InteractorNode::doDimensionalityChange(int new_dim)
{
    return FALSE;
}
//
// Called when a message is received from the executive after
// this->ExecModuleMessageHandler() is registered in
// this->Node::netPrintNode() to receive messages for this node.
// We parse all the common information and then the class specific.
// We return the number of items in the message.
//
int InteractorNode::handleNodeMsgInfo(const char *line)
{
    int  values = 0;

    //
    // Parse the attributes specific to this class
    //
    values += this->handleInteractorMsgInfo(line);

    //
    // Parse the attributes specific to this class
    //
    values += this->handleCommonMsgInfo(line);

    return values;
}
//
// Parse and handle messages that are common to all data-driven interactors.
// Parses and sets the label from a message string.
// Returns the number of recognized messages.
//
int InteractorNode::handleCommonMsgInfo(const char *line)
{
    int values = 0;
    char *p;

    //
    // Handle the 'label="%s"' part of the message only if it is being
    // data driven.
    //
    if ( (p = strstr((char*)line,"label=")) ) {
        char buf[1024];
        int label_index = this->getLabelParameterIndex();
	ASSERT(label_index > 0);
        while (*p != '"') p++;
	FindDelimitedString(p,'"','"', buf);
	this->saveInteractorLabel(buf,TRUE);
	this->setInputAttributeFromServer(label_index,
                                               	this->lastInteractorLabel,
						DXType::StringType);
	values++;
    }
    return values;
}
//
// Define the mapping of inputs that shadow outputs.
// By default, all data driven interactors, have a single output that is
// shadowed by the third (hidden) input.
// Returns an input index (greater than 1) or 0 if there is no shadowing input
// for the given output index.
// FIXME: should this be somehow defined in the mdf.
//
int InteractorNode::getShadowingInput(int output_index)
{
    int input_index;

    switch (output_index) {
        case 1: input_index = 3;  break;
        default: input_index = 0; break;
    }
    return input_index;

}
//
// Get the index of the label parameter.
// Be default, it is always the last parameter.
//
int InteractorNode::getLabelParameterIndex()
{
    return this->getInputCount();
}

//
// Set all shadowing inputs to use the default value.
// This is most likely used during initialization after setting the outputs
// (which sets the shadowing inputs).
//
void InteractorNode::setShadowingInputsDefaulting(boolean send)
{
    int i, ocnt = this->getOutputCount();

    for (i=1 ; i<=ocnt ; i++) {
        int input_shadow = this->getShadowingInput(i);
        if (input_shadow)
            this->useDefaultInputValue(input_shadow, send);
    }
}
void InteractorNode::saveInteractorLabel(const char *label,
                                        boolean strip_quotes)
{
    if (this->lastInteractorLabel) 
        delete this->lastInteractorLabel;

    if (!label) {
 	this->lastInteractorLabel = NULL;
	return;
    }

    if (strip_quotes) {
        this->lastInteractorLabel = new char[STRLEN(label)+1];
#if 00
        char *p1 = (char*)label; 
#else
        const char *p1 = (char*)label; 
#endif
	boolean had_quote;
        while (*p1 && *p1 != '"')
                p1++;
	if (*p1 == '"')  {
	    p1++;				// Skip the first "
	    had_quote = TRUE;
	} else 
	    had_quote = FALSE;
#if 00
	char *p2 = this->lastInteractorLabel;
	boolean skipping = FALSE;
	*p2 = '\0';
        ASSERT(*p1);
        while (*p1 && (skipping || *p1 != '"')) {
	    if (!skipping && (*p1 == '\\')) {
		skipping = TRUE;
	    } else {
		skipping = FALSE;
		*p2 = *p1;
		p2++;
	    }
	    p1++;
	}
        ASSERT(*p1);
        // p2++;				// Skip over the last character 
        *p2 = '\0';
#else
	ASSERT(p1 && this->lastInteractorLabel);
	strcpy(this->lastInteractorLabel,(char*)p1);
	if (had_quote) {	// Find and remove trailing double-quote 
	    int len = STRLEN(this->lastInteractorLabel);
	    ASSERT(len > 0);
	    p1 = this->lastInteractorLabel;
	    char *p2 = &this->lastInteractorLabel[len-1];
	    while ((p2 > p1) && *p2 && (*p2 != '"'))
		p2--;
	    if (*p2 == '"')
		*p2 = '\0';
	}
#endif
    } else
        this->lastInteractorLabel = DuplicateString(label);
}
//
// Determine if this node is of the given class.
//
boolean InteractorNode::isA(Symbol classname)
{
    Symbol s = theSymbolManager->registerSymbol(ClassInteractorNode);
    if (s == classname)
	return TRUE;
    else
	return this->ShadowedOutputNode::isA(classname);
}
#if 0	// 8/9/93
boolean InteractorNode::isAttributeVisuallyWriteable(int input_index)
{
     return
        !this->isInputConnected(DATA_PARAM_NUM) &&
         this->ShadowedOutputNode::isAttributeVisuallyWriteable(input_index);
}
#endif

//
// Return TRUE if this node has state that will be saved in a .cfg file.
//
boolean InteractorNode::hasCfgState()
{
    return TRUE;
}


boolean InteractorNode::printAsJava(FILE* aplf)
{
    if (this->ShadowedOutputNode::printAsJava(aplf) == FALSE)
	return FALSE;

    InteractorInstance *ii;
    ListIterator it(this->instanceList);
    while ( (ii = (InteractorInstance*)it.getNext()) ) {
	if (ii->printAsJava(aplf) == FALSE) return FALSE;
	this->printJavaValue(aplf);
    }

    return TRUE;
}

boolean InteractorNode::printJavaValue (FILE* jf)
{
    const char* indent = "        ";
    char var_name[32]; 
    sprintf (var_name, "%s_%d", this->getNameString(), this->getInstanceNumber());

    //
    // Remove leading and trailing quotes and whitespace
    //
    char* value = DuplicateString(this->getOutputValueString(1));
    char* head = value;
    while ( (*value) && ((*value == ' ') || (*value == '"') || (*value == '\t')))
	value++;
    int l = strlen(value);
    int i;
    for (i=l-1; i>=0; i--)
	if ((value[i] == '"')||(value[i] == ' ')||(value[i] == '\t')||(value[i] == '\n'))
	    value[i] = '\0';
	else
	    break;

    //
    // Escape anything which would prevent a clean javac compile
    //
    l = strlen(value);
    int next = 0;
    char* escaped_value = new char[1+ l<<1];
    for (i=0; i<l; i++) {
	char c = value[i];
	if (c == '"')
	    escaped_value[next++] = '\\';
	escaped_value[next++] = c;
    }
    escaped_value[next] = '\0';


    fprintf (jf, "%s%s.setValue(\"%s\");\n", indent, var_name, escaped_value);
    delete head;
    delete escaped_value;

    return TRUE;
}


const char* InteractorNode::getJavaVariable()
{
    if (this->java_variable == NUL(char*)) {
	this->java_variable = new char[32];
	sprintf (this->java_variable, "%s_%d",
	this->getNameString(), this->getInstanceNumber());
    }
    return this->java_variable;
}

