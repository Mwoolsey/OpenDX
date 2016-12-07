/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#include "ToggleNode.h"
#include "ToggleInstance.h"
#include "ErrorDialogManager.h"
#include "AttributeParameter.h"

#include "DXApplication.h"
#include "MacroDefinition.h"

//
// The following define the mapping of parameter indices to parameter
// functionality in the module that is used to 'data-drive' the
// ToggleNode class and Nodes derived from it.
// These MUST match the entries in the ui.mdf file for the interactors
// that expect to be implemented by the ToggleInteractor class.
//
#define ID_PARAM_NUM            1       // Id used for UI messages
#define CVAL_PARAM_NUM          2       // The current output 
#define ISSET_PARAM_NUM         3       // is the toggle set 
#define SET_PARAM_NUM           4       // The dd set value 
#define RESET_PARAM_NUM         5       // The dd unset value 
#define LABEL_PARAM_NUM         6       // Label 
#define EXPECTED_SELECTOR_INPUTS        LABEL_PARAM_NUM


#define SEND_NOW	1
#define NO_SEND		0
#define SEND_QUIET 	-1	

//
// Constructor 
//
ToggleNode::ToggleNode(NodeDefinition *nd,
			Network *net, int instance) :
                        InteractorNode(nd, net, instance)
{
    this->outputType = DXType::UndefinedType;
    this->is_set = FALSE;
}
ToggleNode::~ToggleNode()
{
}
//
// Called after allocation is complete.
// The work done here is to assigned default values to the InteractorNode inputs
// so that we can use them later when setting the attributes for the
// Interactor.
//
boolean ToggleNode::initialize()
{

    if (this->getInputCount() != EXPECTED_SELECTOR_INPUTS) {
        fprintf(stderr,
           "Expected %d inputs for %s interactor, please check the mdf file.\n",
                        EXPECTED_SELECTOR_INPUTS,
                        this->getNameString());
        return FALSE;
    }

    //
    // Make the default set/reset values be 1/0 and make the output set.
    //
    if (!this->setToggleValues("1", "0")) {
        ErrorMessage(
        "Error setting default attributes for %s interactor, check ui.mdf\n",
                this->getNameString());
        return FALSE;
    }

    //
    // Make the shadows defaulting (even though we have a current output)
    // so that the executive module can tell when it is executing a just
    // placed module and one that is read in from a .net or .cfg file.
    // When read in, the output will be set again which should make the
    // corresponding shadowing input be non-defaulting.
    //
    this->setShadowingInputsDefaulting();

    // Set the input id for the executive module.
    //
    this->setMessageIdParameter();

    return TRUE;
}
//
// Create a new interactor instance for this class.
//
InteractorInstance *ToggleNode::newInteractorInstance()
{
    return new ToggleInstance(this);
}
boolean ToggleNode::setToggle(boolean setit, boolean send)
{
   return this->setTheToggle(setit,(send ? SEND_NOW : NO_SEND));
}
boolean ToggleNode::setToggleQuietly(boolean setit)
{
   return this->setTheToggle(setit,SEND_QUIET);
}
//
// Set the output and send it if requested.
// If setit is TRUE, then set the output to the first of the toggle values
// otherwise the 2nd of toggle values.
// If how is 1, then send the value.
// If how is 0,  then don't send the value.
// If how is -1, then send the value quietly.
//
boolean ToggleNode::setTheToggle(boolean setit, int how)
{
    boolean r;
    char *val;

    if (setit)
	val = this->getSetValue();
    else
	val = this->getResetValue();

    ASSERT(val);

    //
    // Do this before setting the value so that if/when it succeeds, the 
    // interactors that update their value from this->isSet() will get
    // the correct answer.
    // FIXME: setting the ISSET_PARAM_NUM may be better done in
    //		this->setOutputValue[Quietly](), but since  this is the
    //		only occurence of setOutputValue() in this file, and all
    //		others are either done while parsing (when it doesn't matter)
    //		not done at all and instead use this->set/reset().
    //
    this->is_set = setit;
    r = this->setInputValue(ISSET_PARAM_NUM, setit?"1":"0",
			DXType::UndefinedType, FALSE) &&
         this->setOutputValue(1,val,DXType::UndefinedType, FALSE);
    ASSERT(r);

    if (how == SEND_NOW)
	this->sendValues(FALSE);
    else if (how == SEND_QUIET)
	this->sendValuesQuietly();


    delete val;

    return r; 
}

//
// Set the two potential output values.  The 1st corresponds to the
// set (toggle down) state and the second to the reset state (toggle up).
// If how is 1, then send the values.
// If how is 0,  then don't send the values.
// If how is -1, then send the values quietly.
//
boolean ToggleNode::changeToggleValues(const char *set, 
				const char *reset, int how)
{
    boolean was_set, r = TRUE; 

    was_set = this->isSet();

    if (set) {
	r = this->setInputSetValue(SET_PARAM_NUM,set,
					DXType::UndefinedType,FALSE);
    }
    if (r && reset) {
	r = this->setInputSetValue(RESET_PARAM_NUM,reset,
					DXType::UndefinedType,FALSE);
    }

    if (r) { 
	if (set && was_set) 
	    this->set(FALSE);		
	else if (reset && !was_set) 
	    this->reset(FALSE);		


	if (how == SEND_NOW)
	    this->sendValues(FALSE);
	else if (how == SEND_QUIET)
	    this->sendValuesQuietly();
    }
    return r;
   
}

//
// Initialize the two potential output values.  The 1st corresponds to the
// set (toggle down) state and the second to the reset state (toggle up).
// This (init) is called at creation, where as setToggleValues() is called
// after initialization.
//
boolean ToggleNode::initToggleValues(const char *set, const char *reset)
{
    ASSERT(0);// we should get rid of this method
    return this->changeToggleValues(set,reset,NO_SEND);
}
//
// Set the two potential output values.  The 1st corresponds to the
// set (toggle down) state and the second to the reset state (toggle up).
//
boolean ToggleNode::setToggleValues(const char *set, const char *reset)
{
    return this->changeToggleValues(set,reset,NO_SEND);
}
//
//
// Get the values that are output for the set and reset states.
// The returned string must be freed by the caller.
//
char *ToggleNode::getToggleValue(boolean setval)
{
    int pindex;

    if (setval)
        pindex = SET_PARAM_NUM;
    else
        pindex = RESET_PARAM_NUM;

    return DuplicateString(this->getInputSetValueString(pindex));
}

int  ToggleNode::handleInteractorMsgInfo(const char *line)
{
    int values = 0;
    char *p;

    //
    // Handle the 'unset=...' part of the message.
    //
    if ( (p = strstr((char*)line,"unset=")) ) {
        values++;
        while (*p != '=') p++;
        p++;
	this->changeToggleValues(NULL,p,NO_SEND);
    } else if ( (p = strstr((char*)line,"set=")) ) {
	//
	// Handle the 'set=...' part of the message.
	//
        values++;
        while (*p != '=') p++;
        p++;
	this->changeToggleValues(p,NULL,NO_SEND);
    }

    if (values) {
	//
	// These two params may have gotten marked dirty, but we certainly
	// don't need to send them back to the exec as these parameters
	// are what triggered these messages.
	//
	this->clearInputDirty(SET_PARAM_NUM);
	this->clearInputDirty(RESET_PARAM_NUM);
	//
	// Send the values, but don't cause an execution
	//
	this->sendValuesQuietly();
    }

    return values;
}
//
// Determine if this node is of the given class.
//
boolean ToggleNode::isA(Symbol classname)
{
    Symbol s = theSymbolManager->registerSymbol(ClassToggleNode);
    if (s == classname)
	return TRUE;
    else
	return this->InteractorNode::isA(classname);
}
//
// Print the standard .cfg comment and 
// and the 'toggle: %d, set = %s, reset = %s' comment.
//
boolean ToggleNode::cfgPrintInteractorComment(FILE *f)
{
     return this->InteractorNode::cfgPrintInteractorComment(f) &&
	   this->printToggleComment(f,"", TRUE);
}
//
// Parse the standard .cfg comment and 
// and the 'toggle: %d, set = %s, reset = %s' comment.
//
boolean ToggleNode::cfgParseInteractorComment(const char* comment,
                const char* filename, int lineno)
{
    return this->InteractorNode::cfgParseInteractorComment(
			comment,filename,lineno) ||
	   this->parseToggleComment(comment, filename,lineno, TRUE);
}
//
// Print the standard .net comment and 
// and the 'toggle: %d, set = %s, reset = %s' comment.
//
boolean ToggleNode::netPrintAuxComment(FILE *f)
{
    return this->InteractorNode::netPrintAuxComment(f) &&
	   this->printToggleComment(f,"    ", FALSE);
}
//
// Parse the standard comment and 
// and the 'toggle: %d, set = %s, reset = %s' comment.
//
boolean ToggleNode::netParseAuxComment(const char* comment,
                const char* filename, int lineno)
{
    return this->InteractorNode::netParseAuxComment(comment,filename,lineno) ||
	   this->parseToggleComment(comment, filename,lineno, FALSE);
}
//
// Print the 'toggle: %d, set = %s, reset = %s' comment.
//
boolean ToggleNode::printToggleComment(FILE *f, const char *indent,
						boolean includeValues)
{
    boolean r = TRUE;

    if (fprintf(f,"%s// toggle : %d", indent, this->isSet() ? 1 : 0) <= 0)
	return FALSE;

    if (includeValues) {
	char *setval = this->getSetValue();
	char *resetval = this->getResetValue();
	ASSERT(setval && resetval && indent);
	if (fprintf(f,", set = %s, reset = %s", setval, resetval) <= 0)
	    r = FALSE; 
	delete setval;
	delete resetval;
    } 

    if (r && (fprintf(f,"\n") <= 0)) 
	r = FALSE;

    return r;

}
//
// Parse the 'toggle: %d, set = %s, reset = %s' comment.
//
boolean ToggleNode::parseToggleComment(const char* comment,
                			const char* filename, int lineno,
					boolean includeValues)
{
    int items, set;
    char *setstr, *resetstr, *p, *dup = NULL; 

    if (strncmp(comment," toggle",7))
	return FALSE;

    items = sscanf(comment," toggle : %d",&set);
    if (items != 1)
	goto parse_error;	

    if (includeValues) {
	dup = DuplicateString(comment);

	//
	// Get the set and reset values
	//
	setstr = strstr(dup, "set = "); 
	if (!setstr) goto parse_error;
	setstr += 6;     	// First character of set value
	p = resetstr = strstr(dup, "reset = "); 
	if (!resetstr) goto parse_error;
	while (*p!=',' && p!=dup && *p) p--;	// Skip back to find ','
	if (*p != ',') goto parse_error;
	*p = '\0';	// Now the setstr is set.
	resetstr += 8;	// First character of reset value
	if ( (p = strchr(resetstr,'\n')) )
	    *p = '\0';
	// Now the resetstr is set.

	//
	// Set the two values for the set/reset states. 
	//
	if (!this->setToggleValues(setstr, resetstr))
	    goto parse_error;
    } 
#if 000
else {
	//
	// Get the values that were read in the 'input[]...' comments
 	// and stuff them in to the AttributeParameters.
	//
        this->setToggleValues(this->getInputSetValueString(SET_PARAM_NUM),
			      this->getInputSetValueString(RESET_PARAM_NUM));
    }
#endif

    //
    // Set but don't send since we are in the middle of reading the net.
    //
    if (set == 1)
	this->set(FALSE);	
    else
	this->reset(FALSE);	

    if (dup) delete dup;
    return TRUE;

parse_error:
    if (dup) delete dup;
    ErrorMessage("Can't parse 'toggle : ' comment, file %s, line %d",
				filename, lineno);
    return FALSE;
	
}

int ToggleNode::getShadowingInput(int output_index)
{
    if (output_index == 1)
	return CVAL_PARAM_NUM;
    else
	return 0;
}
boolean ToggleNode::isSetAttributeVisuallyWriteable()
{
    return this->isInputDefaulting(SET_PARAM_NUM);
}
boolean ToggleNode::isResetAttributeVisuallyWriteable()
{
    return this->isInputDefaulting(RESET_PARAM_NUM);
}

#if SYSTEM_MACROS // 3/13/95 - begin work for nodes to use system macros
char *ToggleNode::netNodeString(const char *prefix)
{
    return Node::netNodeString(prefix);
}
boolean ToggleNode::sendValues(boolean ignoreDirty)
{
    const char *macname = this->getExecModuleNameString();
    
    MacroDefinition *md = (MacroDefinition*)
		theNodeDefinitionDictionary->findDefinition(macname);
    if (md && md->getNetwork()->isDirty()) {
	md->updateServer();
	md->getNetwork()->clearDirty();
    }

    return InteractorNode::sendValues(ignoreDirty);
}

boolean ToggleNode::printValues(FILE *f, const char *prefix, PrintType dest)
{
    
    const char *macname = this->getExecModuleNameString();
    
    MacroDefinition *md = (MacroDefinition*)
		theNodeDefinitionDictionary->findDefinition(macname);
    if (md && md->getNetwork()->isDirty()) {
	md->printNetworkBody(f,PrintExec);
	md->getNetwork()->clearDirty();
    }
	

    return InteractorNode::printValues(f,prefix, dest);
}

char *ToggleNode::netBeginningOfMacroNodeString(const char *prefix)
{
    const char *macname = this->getExecModuleNameString();

    MacroDefinition *md = (MacroDefinition*)
		theNodeDefinitionDictionary->findDefinition(macname);
    if (md && md->loadNetworkBody()) 
	md->getNetwork()->setDirty();
    return NULL;
}
#endif // 000



boolean ToggleNode::printJavaValue (FILE* jf)
{
    const char* indent = "        ";
    const char* var_name = this->getJavaVariable();

    //
    // Load items into the java interactor
    //
    fprintf (jf, "%sVector %s_vn = new Vector(2);\n", indent, var_name);
    fprintf (jf, "%sVector %s_vo = new Vector(2);\n", indent, var_name);

    int i;
    for (i=1; i<=2; i++) {
        char* head;
        if (i==1) head = this->getSetValue();
        else head = this->getResetValue();

        char* optvalue = head;
	int k;
	for (k=strlen(optvalue)-1; k>=0; k--)
	    if ((optvalue[k] == ' ') || (optvalue[k] == '"')) optvalue[k] = '\0';
	    else break;
	while (*optvalue) {
	    if ((*optvalue == ' ') || (*optvalue == '"')) optvalue++;
	    else break;
	}


        fprintf (jf, "%s%s_vn.addElement(\"%s\");\n", indent,var_name, "set_value");
        fprintf (jf, "%s%s_vo.addElement(\"%s\");\n", indent,var_name, optvalue);

        delete head;
    }
    fprintf(jf, "%s%s.setValues(%s_vn, %s_vo);\n", indent, var_name, var_name, var_name);

    if (this->isSet())
	if (fprintf (jf, "%s%s.selectOption(1);\n", indent, var_name) <= 0)
	    return FALSE;

    return TRUE;
}

boolean ToggleNode::printJavaType(FILE* jf, const char* indent, const char* var)
{
    Parameter *p = this->getOutputParameter(1);
    if (p->hasValue()) {
	Type t = p->getValueType();
	if (t & DXType::StringType) {
	    fprintf (jf, "%s%s.setOutputType(BinaryInstance.STRING);\n", indent,var);
	} else if (t & DXType::IntegerType) {
	    fprintf (jf, "%s%s.setOutputType(BinaryInstance.INTEGER);\n", indent,var);
	} else if (t & DXType::ScalarType) {
	    fprintf (jf, "%s%s.setOutputType(BinaryInstance.SCALAR);\n", indent,var);
	}
    } else {
	fprintf (jf, "%s// The output has no value saved in the net\n", indent);
	fprintf (jf, "%s%s.setOutputType(BinaryInstance.STRING);\n", indent,var);
    }
    return TRUE;
}

