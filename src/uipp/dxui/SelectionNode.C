/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#include "lex.h"
#include "SelectionNode.h"
#include "ErrorDialogManager.h"
#include "AttributeParameter.h"
#include "Parameter.h"
#include "SelectorInstance.h"
#include "ListIterator.h"

//
// Define the input parameter numberings.  These MUST coincide with the mdf.
//
#define ID_PARAM_NUM		1
#define CSTR_PARAM_NUM		2	// Current output string
#define CVAL_PARAM_NUM		3	// Current output value
#define STRLIST_PARAM_NUM	4	// group or stringlist
#define VALLIST_PARAM_NUM	5	// integer or value list
#define CULL_PARAM_NUM		6	// removes zero length strings 
#define LABEL_PARAM_NUM		7

#define EXPECTED_SELECTOR_INPUTS	LABEL_PARAM_NUM

//
// Used to defer setting the outputs when we know we have many options
// to append to the list of options.  Otherwise, appending lots of items
// takes LOTS of time.
//
int SelectionNode::OptionsLeftToSet = 0;

SelectionNode::SelectionNode(NodeDefinition *nd, Network *net, int instnc,
		boolean alwaysEnlistOutputs) :
                InteractorNode(nd, net, instnc) 
		
{ 
    this->optionCount = 0;
    this->alwaysEnlistOutputs = alwaysEnlistOutputs;
    this->deferNewOptionInstallation = 
		new DeferrableAction(SelectionNode::InstallNewOptions, 
			(void*)this);
}
SelectionNode::~SelectionNode()
{
    delete this->deferNewOptionInstallation;
}


void SelectionNode::InstallNewOptions(void *staticData, void *requestData)
{
    SelectionNode *sn = (SelectionNode*)staticData;
    sn->installNewOptions(NULL,NULL,FALSE);
}
//
// Set up the interactor with the default options
//
boolean SelectionNode::initialize()
{
    if (this->getInputCount() != EXPECTED_SELECTOR_INPUTS) {
	fprintf(stderr,
           "Expected %d inputs for %s interactor, please check the mdf file.\n",
                        EXPECTED_SELECTOR_INPUTS,
                        this->getNameString());
	return FALSE;
    }

    //
    // Set the default options
    //
    //this->selectedOption = 0;	// This helps the next 2 lines
    this->initValueOptionsAttribute(this->getInitialValueList());
    this->initStringOptionsAttribute(this->getInitialStringList());
    this->installNewOptions(NULL,NULL,FALSE);
#if 000

    //
    // Set the default output values (must be one of the above options) 
    //
    this->setOutputValue(1,"1", DXType::UndefinedType, FALSE);
    this->setOutputValue(2,"on", DXType::UndefinedType, FALSE);
#else
    this->clearSelections(FALSE, FALSE);
#if 00
    this->addSelectedOptionIndex(1,TRUE);
#else
    this->addSelectedOptionIndex(1,FALSE);
#endif
#endif

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
// If either of the list inputs has changed, we must determine the size
// of the smaller of the two lists and reset this->optionCount to that value.
// NOTE: If this routine is being called, then one of the values must have a 
//       set value (even though both are allowed not too).
//
void SelectionNode::ioParameterStatusChanged(boolean input, int index,
			NodeParameterStatusChange status)
{
    if (input && (status & Node::ParameterValueChanged))  {
	if ((index == VALLIST_PARAM_NUM) || (index == STRLIST_PARAM_NUM)) 
	    this->installNewOptions(NULL,NULL, FALSE);
	
    }
    this->InteractorNode::ioParameterStatusChanged(input, index, status);
}

//
// Use the value list and string list to set the new list of options.
// Also, notify all instances that the state has changed.
// If vlist == NULL, update from the VALLIST parameter.
// If slist == NULL, update from the STRLIST parameter.
//
void SelectionNode::installNewOptions(const char *vlist, const char *slist, 
					boolean send)
{
    static int kloodge = 0;

    if (kloodge != 0)	// Stop installNewOptions -> ioParameterValueChanged ->
	return; 	// installNewOptions loop.

    if (this->deferNewOptionInstallation->isActionDeferred()) {
        this->deferNewOptionInstallation->requestAction((void*)send);
	return;
    }
	

    this->deferVisualNotification();

    if (vlist == NULL)  
	vlist = this->getInputSetValueString(VALLIST_PARAM_NUM);


    kloodge++;
    this->setValueOptionsAttribute(vlist);
    kloodge--;
    vlist = this->getValueOptionsAttribute();

    if (slist == NULL) 
	slist = this->getInputSetValueString(STRLIST_PARAM_NUM);
	

    kloodge++;
    this->setStringOptionsAttribute(slist);
    kloodge--;
    slist = this->getStringOptionsAttribute();

    int vcnt = DXValue::GetListItemCount(vlist, DXType::UndefinedType);
    int scnt = DXValue::GetListItemCount(slist, DXType::UndefinedType);

    this->optionCount = (vcnt < scnt ? vcnt : scnt);

    if (!SelectionNode::OptionsLeftToSet) {

	
#if 000
	if (vcnt && scnt && (this->getOptionCount() == 0))
	    this->addSelectedOptionIndex(1, FALSE, FALSE);
#else
	//
	// Remove any selections that are out of range.
	//
	ListIterator iter(this->selectedOptions);
	int i, pos = 0;
	while ( (i = (int)(long)iter.getNext()) ) {
	    pos++;
	    if (i > this->getOptionCount()) {
		this->selectedOptions.deleteElement(pos);	
		iter.setList(this->selectedOptions); 
		pos = 0;
	    }
	}
	//
	// Make sure there is at least one item selected, otherwise
	// the SelectorInteractor gets upset
	//
	if (vcnt && scnt && this->selectedOptions.getSize() == 0) 
	    this->addSelectedOptionIndex(1, FALSE,FALSE);
#endif

	this->notifyVisualsOfStateChange();

#if 00
	if (this->getOptionCount() >= selection) {
	    //
	    // Update the outputs.
	    //
	    char *vval = this->getOptionValueString(selection); 
	    char *sval = this->getOptionNameString(selection); 
	    ASSERT(vval && sval);
	    this->setOutputValue(1,vval,DXType::UndefinedType,FALSE);
	    this->setOutputValue(2,sval,DXType::UndefinedType,FALSE);
	    delete vval;
	    delete sval;
	}
#else
	this->updateOutputs();
#endif

	if (send)
	    this->sendValues(FALSE);

    }

    this->undeferVisualNotification();
}


//
// Define the mapping of inputs that shadow outputs.
// Returns an input index (greater than 1) or 0 if there is no shadowing input
// for the given output index.
// FIXME: should this be somehow defined in the mdf.
//
int SelectionNode::getShadowingInput(int output_index)
{
    int input_index;

    switch (output_index) {
        case  1: input_index = CVAL_PARAM_NUM;  break;
        case  2: input_index = CSTR_PARAM_NUM;  break;
        default: input_index = 0; break;
    }
    return input_index;

}

boolean SelectionNode::cfgPrintInteractorAuxInfo(FILE *f)
{

    return this->cfgPrintSelectionsComment(f) &&
    	   this->cfgPrintOptionComments(f);
	
}

boolean SelectionNode::cfgParseComment(const char *comment,
                                const char *filename, int lineno)
{
    return this->cfgParseSelectionsComment(comment, filename,lineno)  ||
           this->cfgParseOptionComment(comment, filename, lineno)     ||
           this->InteractorNode::cfgParseComment(comment, 
						filename, lineno);
}

//
// Get the list of all option values 
//
boolean SelectionNode::initValueOptionsAttribute(const char *vlist)
{
    AttributeParameter *p = (AttributeParameter*) this->getInputParameter(
                                                VALLIST_PARAM_NUM);
    return p->initAttributeValue(vlist);
}
boolean SelectionNode::setValueOptionsAttribute(const char *vlist)
{
    AttributeParameter *p = (AttributeParameter*) this->getInputParameter(
                                                VALLIST_PARAM_NUM);
    return p->setAttributeValue(vlist);
}
const char *SelectionNode::getValueOptionsAttribute()
{
    AttributeParameter *p = (AttributeParameter*) this->getInputParameter(
                                                VALLIST_PARAM_NUM);
    return p->getAttributeValueString();
}

//
// Get the list of all option strings 
//
boolean SelectionNode::initStringOptionsAttribute(const char *vlist)
{
    AttributeParameter *p = (AttributeParameter*) this->getInputParameter(
                                                STRLIST_PARAM_NUM);
    return p->initAttributeValue(vlist);
}
boolean SelectionNode::setStringOptionsAttribute(const char *vlist)
{
    AttributeParameter *p = (AttributeParameter*) this->getInputParameter(
                                                STRLIST_PARAM_NUM);
    return p->setAttributeValue(vlist);
}
const char *SelectionNode::getStringOptionsAttribute()
{
    AttributeParameter *p = (AttributeParameter*) this->getInputParameter(
                                                STRLIST_PARAM_NUM);
    return p->getAttributeValueString();
}


//
// Get the value of the indicated option.
// The returned string must be deleted by the caller.
//
char  *SelectionNode::getOptionValueString(int optind)
{
   const char *s = this->getValueOptionsAttribute();
   Type t = DXType::DetermineListItemType(s);
   return DXValue::GetListItem(s,optind,t | DXType::ListType);
}
//
// Get the name of the indicated option.
// The return string is not double quoted if keep_quotes is FALSE.
// The return string must be deleted by the caller.
//
char  *SelectionNode::getOptionNameString(int optind, boolean keep_quotes)
{
   const char *list = this->getStringOptionsAttribute();
   char *value, *s = DXValue::GetListItem(list,optind,DXType::StringListType);

   value = s;
   if (s && !keep_quotes) {
	//
	// Strip off the double quotes.
	//
	char *c, *p;
	p = s;
	c = value = new char [STRLEN(s)];
	p++;
	while (*p != '"') {
	   *c = *p;
	   c++; p++;
	}
	*c = '\0';
	delete s;
   } 
   
   return value;
}


boolean SelectionNode::cfgPrintSelectionsComment(FILE *f)
{
    int count = this->getOptionCount();
    int selectionCount = this->getSelectedOptionCount();

    //
    // If the option list is empty (i.e. count == 0) then make sure the
    // selected option index is reasonable (i.e. >= 0).
    //
    if (fprintf(f, "// selections: maximum = %d, current = ", count) <= 0)
	return FALSE;

    if (count <= 0 || selectionCount <= 0) {
	if (fprintf(f,"-1") <= 0)
	    return FALSE;
    } else {
	int i;
	for (i=1 ; i<=count ; i++)
	    // File uses 0 based indexing
	    if (this->isOptionSelected(i) && (fprintf(f,"%d ",i-1) <= 0))
		return FALSE;
    }
    if (fprintf(f,"\n") <= 0)
	return FALSE;

    return TRUE;
}
//
// 
//
boolean SelectionNode::cfgParseSelectionsComment(const char *comment,
                                const char *filename, int lineno)
{
    int      items_parsed;
    int      nchars,n_options;
    int      option;

    ASSERT(comment);

    if (strncmp(comment," selections",11))
        return FALSE;

    items_parsed =
	sscanf(comment, 
		" selections: maximum = %d, current = %d%n",
	       	&n_options,		
	       	&option,
		&nchars);

    //
    // Indicate that we expect n_options, and that we should not update
    // outputs until this goes back to 0 (i.e. we've seen n_options
    // 'options[...' comments.
    //
    SelectionNode::OptionsLeftToSet = n_options;


    if (items_parsed != 2)
    {
	ErrorMessage("Can't parse 'selections' comment (file %s, line %d)",
					filename, lineno);
	return FALSE;
    }

    /*
     * Set current option(s).
     */
    this->clearSelections(FALSE,FALSE);
    if (option >= 0) {
	// 1 based indexing
	this->addSelectedOptionIndex(option+1, FALSE, FALSE);
	const char *p = comment + nchars;
	while (sscanf(p,"%d%n",&option,&nchars) == 1) {
	    if (option >= 0)
		// 1 based indexing
		this->addSelectedOptionIndex(option+1, FALSE, FALSE);
	    p += nchars;
	}
    }

    return TRUE;
}

boolean SelectionNode::cfgPrintOptionComments(FILE *f)
{
    char *str, *val;
    boolean r = TRUE;

    int i = 0;
    int count = this->getOptionCount();
    while (r && i++ < count) {
	str =	this->getOptionNameString(i);
  	val = this->getOptionValueString(i);
        if (fprintf(f,
		"// option[%d]: name = \"%s\", value = %s\n",
		i-1,		// File uses zero based indexing
		str,	
  	        val) < 0)
	    r = FALSE; 
	delete str;
	delete val;
	
    }
    return r;
}
boolean SelectionNode::cfgParseOptionComment(const char *comment,
                                const char *filename, int lineno)
{
    int      items_parsed;
    int      opt_index;
    char     value[128];
    char     name[512];

    ASSERT(comment);

    if (strncmp(comment," option[",8))
        return FALSE;

    items_parsed =
	sscanf(comment,
	       " option[%d]: name = \"%[^\"]\", value = %[^\n]",
	       &opt_index,
	       name,
	       value);

    // index is 1 based internally
    opt_index++;

    if (items_parsed != 3)
    {
	ErrorMessage("Can't parse 'option' comment (file %s, line %d)",
					filename, lineno);
	return FALSE;
    }


	
    if ((opt_index < 1) || (opt_index > this->getOptionCount()+1))
    {
	ErrorMessage("Bad option index, (file %s, line %d)", filename, lineno);
	return FALSE;
    }

#if 000
    if (this->getSelectedOptionIndex() == opt_index) {
	if (this->setOutputValue(2,name,DXType::StringType,FALSE) == 
							DXType::UndefinedType)
	{
	    ErrorMessage(
	       "Can't set string value of Selection option, (file %s, line %d)", 
		filename, lineno);
	    return FALSE;
	}	
	if (this->setOutputValue(1,value, DXType::UndefinedType,FALSE) == 
							DXType::UndefinedType)
	{
	    ErrorMessage(
	       "Can't set value of Selection option, (file %s, line %d)", 
		filename, lineno);
	    return FALSE;
	}	
    }
#endif

    // 
    // This test should tell us if we are parsing the first of a set of 
    // options.
    // 
    //if (this->getOptionCount() > opt_index) { 
    if (opt_index == 1) {
	this->optionCount = 0;
    }

    boolean r;
    r = this->appendOptionPair(value, name);

    if (!r) {
	ErrorMessage("Can not add option to option list (file %s, line %d)",
				filename, lineno);
	return FALSE;
    }

#if 000
#else
    //
    // At this point we successfully parsed an option, so indicate that 
    // there is one less to set.  If this is the last option, then
    // update and send the new output values.
    //
    if (--SelectionNode::OptionsLeftToSet == 0)  {
	this->updateOutputs();
	this->sendValues(FALSE);
    }
#endif

    return TRUE;

}
//
// Add an option pair to the list of option pairs.
// Do not send the values to the executive or notify CDBs or interactors.
//
boolean SelectionNode::appendOptionPair(const char *value, const char *label)
{
    boolean r;
    const char *slist;
    const char *vlist;
    char *str;

    if (this->getOptionCount() == 0) {
	slist = "{ }";
	vlist = "{ }";
    } else { 
	slist = this->getStringOptionsAttribute();
	vlist = this->getValueOptionsAttribute();
    }

    //
    // Make sure the label has double quotes around it.
    //
    if ( (str = FindDelimitedString(label,'"','"')) ) {
	str = DuplicateString(label);
    } else {
	str = new char [STRLEN(label) + 4]; 
	sprintf(str,"\"%s\"",label);
    }

    char *nvlist = DXValue::AppendListItem(vlist, value); 
    char *nslist = DXValue::AppendListItem(slist, str); 
    delete str;

    if (nvlist && nslist) {
	this->installNewOptions(nvlist,nslist, FALSE);
	r = TRUE; 
    } else
	r = FALSE;

    if (nvlist) delete nvlist;
    if (nslist) delete nslist;

    return r;
}

// The messages we parse can contain one or more of the following...
//
//	'value list={...}' or 'string list={...}' or 'index=%d'
//
// If any input or output values are to be changed, don't send them
// because the module backing up the interactor will have just executed
// and if the UI is in 'execute on change' mode then there will be an
// extra execution.
//
// Returns the number of attributes parsed.
//
int SelectionNode::handleInteractorMsgInfo(const char *line)
{
    int index, values = 0;
    char *p, *vlist = NULL, *slist = NULL;
    
    //
    // Handle the 'value list={...}' part of the message.
    //
    if ( (p = strstr((char*)line,"value list=")) ) {
	vlist = FindDelimitedString(p,'{','}');
    }
    //
    // Handle the 'string list={...}' part of the message.
    //
    if ( (p = strstr((char*)line,"string list=")) ) {
	slist = FindDelimitedString(p,'{','}',NULL,"\"");
    }
    //
    // Install the options if we saw both a value list and a string list
    //
    if (vlist && slist) {
	values++;
	this->deferNewOptionInstallation->deferAction();
	this->setInputAttributeFromServer(VALLIST_PARAM_NUM,vlist,
					DXType::UndefinedType);
	this->setInputAttributeFromServer(STRLIST_PARAM_NUM,slist,
					DXType::UndefinedType);
	//
	// The AttributeParameter's primary value won't get updated unless
	// the types match.  In the case of the value parameter, an integer
	// (or NULL) value will result in a list type attribute, in which
	// case the primary and secondary (attribute) values should not be
	// kept in sync.  However, if the value is currently a list type
	// then it should be ok to install another list type into the 
	// primary values. So, if the value parameter is not an integer and
	// not NULL (i.e. a list type), then force a sync of the attribute 
   	// parameter with the primary value.
	// 
        if (this->getInputSetValueType(VALLIST_PARAM_NUM) & DXType::ListType) {
	    AttributeParameter *ap = (AttributeParameter*)
			this->getInputParameter(VALLIST_PARAM_NUM);
	    ap->syncPrimaryValue(TRUE);
	}
	this->deferNewOptionInstallation->undeferAction();
    }
    if (vlist) delete vlist;
    if (slist) delete slist;


    //
    // Handle the 'index=%d' part of the message.
    //
    if ( (p = strstr((char*)line,"index=")) ) {
	int selections[1024], nselects = 0,nchars;
	values++;
        while (*p != '=') p++;
        p++;
	if (*p == '{') 
	    p++;
		
	while (sscanf(p,"%d%n",&index,&nchars) == 1) {
	    if ((index >= 0) && (index < this->getOptionCount()))  {
		selections[nselects] = index+1;	 
		nselects++;	
	    }
	    p+=nchars;
	}
	//
	// Make sure the internal output value and the shadowed inputs
	// are in sync both internally and in the executive.
	//
	this->setSelectedOptions(selections,nselects,FALSE,FALSE);
	this->updateOutputs(TRUE);

    }


    return values;
}
//
// Get a a SelectorInstance instead of an InteractorInstance.
//
InteractorInstance *SelectionNode::newInteractorInstance()
{
    SelectorInstance *si = new SelectorInstance(this);
    return (SelectorInstance*)si;
}

//
// Determine if this node is of the given class.
//
boolean SelectionNode::isA(Symbol classname)
{
    Symbol s = theSymbolManager->registerSymbol(ClassSelectionNode);
    if (s == classname)
	return TRUE;
    else
	return this->InteractorNode::isA(classname);
}
//
// Update the selected option index, and if requested, update the output
// values to match the values indicated by the index.
//
void SelectionNode::changeSelectedOptionIndex(int index, boolean set,
				boolean send, boolean update_outputs)
{
    int i, pos;
    ASSERT(index > 0);
    // ASSERT(SelectionNode::OptionsLeftToSet || (index <= this->getOptionCount()));

    
    ListIterator iter(this->selectedOptions);
    for (pos=1 ; ((i = (int)(long)iter.getNext()) < index) && (i != 0); pos++)
	;

    if (set) {	
	if (i == index)			// The index is already set
	    return;
    } else if (!i || (i != index))	// The index is not there to remove
	return;

    if (set)
	this->selectedOptions.insertElement((void*)index, pos);
    else
	this->selectedOptions.deleteElement(pos);

    //
    // Update the parameter values if requested. 
    //
    if (update_outputs) {
	this->updateOutputs();
	if (send)
	    this->sendValues(FALSE);
    }

}

static void enlist_items(char **values, int cnt, char *buf, 
			boolean forceList, boolean quoteItems)
{
    int i;

    if (!cnt) {
	strcpy(buf,"NULL");
 	return;
    }

    if (forceList || (cnt > 1))
	strcpy(buf,"{ ");
    else
	buf[0] = '\0';

    char *p = buf + STRLEN(buf);
    for (i=0 ; i<cnt ; p+=STRLEN(p), i++)  {
	if (quoteItems)
	    sprintf(p,"\"%s\" ",values[i]);
	else
	    sprintf(p,"%s ",values[i]);
    }
    if (forceList || (cnt > 1))
	strcpy(p,"}");
}
//
// Update the output values based on the current selections, then send
// the values to the server.
//
void SelectionNode::updateOutputs(boolean fromServer)
{
    char *output;
    int cnt = 0, soptions = this->getSelectedOptionCount();
    char **values = NULL, **names = NULL; 

    //
    // Don't notify the interactors that the value has changed
    // (yes notification is redundant), until after we have set both values.
    //
    this->deferVisualNotification();

    if (soptions > 0) {
	int i, totoptions = this->getOptionCount();
        int vallen=0, namelen=0;

	values = new char*[soptions+1];
	names = new char*[soptions+1];
	
	for (cnt=0, i=1 ; i<= totoptions ; i++) {
	    if (this->isOptionSelected(i)) {
	       values[cnt] = this->getOptionValueString(i);
	       vallen += STRLEN(values[cnt]) + 4;		
	       names[cnt] = this->getOptionNameString(i);
	       namelen += STRLEN(names[cnt]) + 4;		
	       cnt++;
	    }
	}
	ASSERT(cnt>0);

	output = new char[ MAX(namelen,vallen) + 8];

	enlist_items(values,cnt,output,this->alwaysEnlistOutputs,FALSE);


    } else {
	output = new char[8];
	strcpy(output,"NULL");
    }

    if (fromServer)
	this->setShadowedOutputSentFromServer(1,output, DXType::UndefinedType);
    else
	this->setOutputValue(1,output,DXType::UndefinedType, FALSE);

    //
    // Undefer now, so that the next setOutputValue() causes all instances
    // to have their output updated.
    //
    this->undeferVisualNotification();

    if (soptions > 0) 
	enlist_items(names,cnt,output,this->alwaysEnlistOutputs,TRUE);

    if (fromServer)
	this->setShadowedOutputSentFromServer(2,output, DXType::UndefinedType);
    else
	this->setOutputValue(2,output,DXType::UndefinedType, FALSE);

    if (cnt > 0) {
	ASSERT(names && values);
	while (cnt--) {
	    delete names[cnt];
	    delete values[cnt];
	} 
	delete names;
	delete values;
    }

    delete output;

}
void SelectionNode::addSelectedOptionIndex(int index, 
				boolean send, boolean update_outputs)
{
    this->changeSelectedOptionIndex(index,TRUE, send, update_outputs);
}
#if 00 // Not used
void SelectionNode::removeSelectedOptionIndex(int index, 
				boolean send, boolean update_outputs)
{
    this->changeSelectedOptionIndex(index,FALSE, send, update_outputs);
}
#endif
boolean SelectionNode::isOptionSelected(int index)
{
    ASSERT(index > 0);
    ASSERT(index <= this->getOptionCount());
    return this->selectedOptions.isMember((void*)index); 
}
void SelectionNode::clearSelections(boolean send, boolean update)
{
    this->selectedOptions.clear();
    if (update) {
	this->updateOutputs();
	if (send)
	    this->sendValues(FALSE);
    }
}
void SelectionNode::setSelectedOptions(int *setIndices, int count, 
				boolean send, boolean update)
{
    this->clearSelections(FALSE,FALSE);
    int i, options = this->getOptionCount();
    for (i=0 ; i<count ; i++) {
	int index = setIndices[i];
	if (index > 0 && index <= options)
	    this->addSelectedOptionIndex(index,FALSE,FALSE); 
    }

    if (update) {
	this->updateOutputs();
	if (send)
	    this->sendValues(FALSE);
    }
}

boolean SelectionNode::printJavaType(FILE* jf, const char* indent, const char* var)
{
    Parameter *p = this->getOutputParameter(1);
    if (p->hasValue()) {
	Type t = p->getValueType();
	if (t & DXType::IntegerType) {
	    if (t & DXType::ListType)
		fprintf (jf, 
		"%s%s.setOutputType(BinaryInstance.INTEGER|BinaryInstance.LIST);\n", 
		indent,var);
	    else
		fprintf (jf, "%s%s.setOutputType(BinaryInstance.INTEGER);\n", indent,var);
	} else if (t & DXType::ScalarType) {
	    if (t & DXType::ListType)
		fprintf (jf, 
		"%s%s.setOutputType(BinaryInstance.SCALAR|BinaryInstance.LIST);\n", 
		indent,var);
	    else
		fprintf (jf, "%s%s.setOutputType(BinaryInstance.SCALAR);\n", indent,var);
	} 
    } else {
	fprintf (jf, "%s// The output has no value saved in the net\n", indent);
	fprintf (jf, "%s%s.setOutputType(BinaryInstance.STRING);\n", indent,var);
    }
    return TRUE;
}

boolean SelectionNode::printJavaValue (FILE* jf)
{
    const char* indent = "        ";
    const char* var_name = this->getJavaVariable();

    //
    // Load up the items in the SelectorNode into the java interactor
    //
    int ocnt = this->getOptionCount();
    List selection_stmts;
    if (ocnt) {
	fprintf (jf, "%sVector %s_vn = new Vector(%d);\n", indent, var_name, ocnt);
	fprintf (jf, "%sVector %s_vo = new Vector(%d);\n", indent, var_name, ocnt);
	int i;
	for (i=1; i<=ocnt; i++) {
	    const char* optname = this->getOptionNameString(i);
	    char* optvalue = DuplicateString(this->getOptionValueString(i));
	    if (this->isOptionSelected(i)) {
		char* stmt = new char[128];
		sprintf (stmt, "%s%s.selectOption(%d);\n", indent, var_name, i);
		selection_stmts.appendElement((void*)stmt);
	    }
	    char* head = optvalue;
	    int k;
	    for (k=strlen(optvalue)-1; k>=0; k--)
		if ((optvalue[k] == ' ') || (optvalue[k] == '"')) optvalue[k] = '\0';
		else break;
	    while (*optvalue) {
		if ((*optvalue == ' ') || (*optvalue == '"')) optvalue++;
		else break;
	    }

	    fprintf (jf, "%s%s_vn.addElement(\"%s\");\n", indent,var_name,optname);
	    fprintf (jf, "%s%s_vo.addElement(\"%s\");\n", indent,var_name,optvalue);
	    delete head;
	}
	if (fprintf (jf, "%s%s.setValues(%s_vn, %s_vo);\n",
	    indent, var_name, var_name, var_name) <= 0) return FALSE;

	ListIterator it(selection_stmts);
	char* cp;
	while ( (cp = (char*)it.getNext()) ) {
	    fprintf (jf, cp);
	    delete cp;
	}
    }
    return TRUE;
}


