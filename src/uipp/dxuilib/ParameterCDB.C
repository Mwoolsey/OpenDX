/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#include "ParameterCDB.h"
#include "TransmitterNode.h"
#include "ReceiverNode.h"


#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/Label.h>
#include <Xm/Text.h>
#include <Xm/ToggleB.h>
#include "../widgets/Stepper.h"

#include "Application.h"
#include "ErrorDialogManager.h"
#include "ListIterator.h"
#include "MacroParameterNode.h"
#include "Network.h"
#include "ParameterDefinition.h"
#include "lex.h"
#include "ParseMDF.h"

boolean ParameterCDB::ClassInitialized = FALSE;
String ParameterCDB::DefaultResources[] =
{

    "*positionLabel.labelString:      Position:",
    "*positionLabel.foreground:       SteelBlue",
    "*nameLabel.labelString:          Name:",
    "*nameLabel.foreground:           SteelBlue",
    "*typeLabel.labelString:          Type:",
    "*typeLabel.foreground:           SteelBlue",
    "*valueLabel.labelString:         Default Value:",
    "*valueLabel.foreground:          SteelBlue",
    "*descriptionLabel.labelString:   Description:",
    "*descriptionLabel.foreground:    SteelBlue",
    "*optionsLabel.labelString:       Options:",
    "*optionValuesLabel.labelString:  Optional Values:",
    "*optionValuesLabel.foreground:   SteelBlue",
    "*optionsLabel.foreground:        SteelBlue",
    "*required.labelString:           required parameter",
    "*descriptive.labelString:        descriptive value",
    "*hidden.labelString:       	hide tab by default",
    "*accelerators:   #augment\n"
        "<Key>Return:                    BulletinBoardReturn()",
    NULL
};

ConfigurationDialog*
ParameterCDB::AllocateConfigurationDialog(Widget parent,
					Node *node)
{
    return new ParameterCDB(parent, node);
}

boolean ParameterCDB::applyCallback(Dialog *)
{
    if (!this->applyValues()) {
        this->saveInitialValues();
	return FALSE;
    }
    this->saveInitialValues();
    return this->ConfigurationDialog::applyCallback(this);
}
void ParameterCDB::restoreCallback(Dialog *clientData)
{
    this->restoreInitialValues();
    this->ConfigurationDialog::restoreCallback(this);
}

Widget ParameterCDB::createParam(Widget parent, Widget )
{
    MacroParameterNode *n = (MacroParameterNode *)this->node;
    ParameterDefinition *pd = n->getParameterDefinition();
    boolean input = pd->isInput();

    Widget form = XtVaCreateManagedWidget(
	"paramSection",
	xmFormWidgetClass,
	parent,
	XmNleftAttachment  , XmATTACH_FORM,
	XmNleftOffset      , 5,
	XmNrightAttachment , XmATTACH_FORM,
	XmNrightOffset     , 5,
	NULL);
    
    Widget positionLabel = XtVaCreateManagedWidget(
	"positionLabel", xmLabelWidgetClass, form,
        XmNtopAttachment    , XmATTACH_FORM,
        XmNtopOffset        , 10,
        XmNleftAttachment   , XmATTACH_FORM,
        XmNleftOffset       , 5,
        XmNrightAttachment  , XmATTACH_POSITION,
        XmNrightPosition    , 25,
        XmNalignment        , XmALIGNMENT_BEGINNING,
	NULL);

    this->position = XtVaCreateManagedWidget(
	"position", xmStepperWidgetClass, form,
        XmNtopAttachment  , XmATTACH_FORM,
        XmNtopOffset      , 10,
        XmNleftAttachment , XmATTACH_POSITION,
        XmNleftPosition   , 25,

        XmNiValue         , 1,
        XmNiMinimum       , 1,
	NULL);

    Widget nameLabel = XtVaCreateManagedWidget(
        "nameLabel", xmLabelWidgetClass, form,
        XmNtopAttachment   , XmATTACH_WIDGET,
        XmNtopWidget       , this->position,
        XmNtopOffset       , 10,
        XmNleftAttachment  , XmATTACH_FORM,
        XmNleftOffset      , 5,
        XmNrightAttachment , XmATTACH_POSITION,
        XmNrightPosition   , 25,
        XmNalignment       , XmALIGNMENT_BEGINNING,
	NULL);

    this->name = XtVaCreateManagedWidget(
        "name", xmTextWidgetClass, form,
        XmNtopAttachment   , XmATTACH_WIDGET,
        XmNtopOffset       , 10,
        XmNtopWidget       , this->position,
        XmNleftAttachment  , XmATTACH_POSITION,
        XmNleftPosition    , 25,
        XmNrightAttachment , XmATTACH_FORM,
        XmNrightOffset     , 5,
        XmNeditMode        , XmSINGLE_LINE_EDIT,
        XmNeditable        , True,
	NULL);

    Widget typeLabel = XtVaCreateManagedWidget(
        "typeLabel", xmLabelWidgetClass, form,
        XmNtopAttachment   , XmATTACH_WIDGET,
        XmNtopWidget       , this->name,
        XmNtopOffset       , 10,
        XmNleftAttachment  , XmATTACH_FORM,
        XmNleftOffset      , 5,
        XmNrightAttachment , XmATTACH_POSITION,
        XmNrightPosition   , 25,
        XmNalignment       , XmALIGNMENT_BEGINNING,
	NULL);

    Widget typeFrame = XtVaCreateManagedWidget(
	"typeFrame", xmFrameWidgetClass, form,
        XmNtopAttachment   , XmATTACH_WIDGET,
        XmNtopWidget       , this->name,
        XmNtopOffset       , 10,
        XmNleftAttachment  , XmATTACH_POSITION,
        XmNleftPosition    , 25,
        XmNrightAttachment , XmATTACH_FORM,
        XmNrightOffset     , 5,
        XmNshadowThickness , 2,
	NULL);

    this->type = XtVaCreateManagedWidget(
        "type", xmTextWidgetClass, typeFrame,
	XmNeditable        , False,
	XmNshadowThickness , 0,
	XmNeditMode        , XmSINGLE_LINE_EDIT,
	NULL);


    Widget valueLabel = XtVaCreateManagedWidget(
        "valueLabel", xmLabelWidgetClass, form,
        XmNtopAttachment   , XmATTACH_WIDGET,
        XmNtopWidget       , typeFrame,
        XmNtopOffset       , 10,
        XmNleftAttachment  , XmATTACH_FORM,
        XmNleftOffset      , 5,
        XmNrightAttachment , XmATTACH_POSITION,
        XmNrightPosition   , 25,
        XmNalignment       , XmALIGNMENT_BEGINNING,
	NULL);

    this->value =  XtVaCreateManagedWidget(
        "value", xmTextWidgetClass, form,
        XmNtopAttachment   , XmATTACH_WIDGET,
        XmNtopWidget       , typeFrame,
        XmNtopOffset       , 10,
        XmNleftAttachment  , XmATTACH_POSITION,
        XmNleftPosition    , 25,
        XmNrightAttachment , XmATTACH_FORM,
        XmNrightOffset     , 5,
        XmNeditMode        , XmSINGLE_LINE_EDIT,
        XmNeditable        , True,
	NULL);
    if (!input)
    {
	XtSetSensitive(valueLabel, False);
	XtSetSensitive(this->value, False);
    }

    Widget descriptionLabel = XtVaCreateManagedWidget(
        "descriptionLabel", xmLabelWidgetClass, form,
        XmNtopAttachment   , XmATTACH_WIDGET,
        XmNtopWidget       , this->value,
        XmNtopOffset       , 10,
        XmNleftAttachment  , XmATTACH_FORM,
        XmNleftOffset      , 5,
        XmNrightAttachment , XmATTACH_POSITION,
        XmNrightPosition   , 25,
        XmNalignment       , XmALIGNMENT_BEGINNING,
	NULL);

    this->description =  XtVaCreateManagedWidget(
        "description", xmTextWidgetClass, form,
        XmNtopAttachment   , XmATTACH_WIDGET,
        XmNtopWidget       , this->value,
        XmNtopOffset       , 10,
        XmNleftAttachment  , XmATTACH_POSITION,
        XmNleftPosition    , 25,
        XmNrightAttachment , XmATTACH_FORM,
        XmNrightOffset     , 5,
        XmNeditMode        , XmSINGLE_LINE_EDIT,
        XmNeditable        , True,
	NULL);

    if (input) {
	XtVaCreateManagedWidget(
	    "optionValuesLabel", xmLabelWidgetClass, form,
	    XmNtopAttachment   , XmATTACH_WIDGET,
	    XmNtopWidget       , this->description,
	    XmNtopOffset       , 10,
	    XmNleftAttachment  , XmATTACH_FORM,
	    XmNleftOffset      , 5,
	    XmNrightAttachment , XmATTACH_POSITION,
	    XmNrightPosition   , 25,
	    XmNalignment       , XmALIGNMENT_BEGINNING,
	    NULL);
	this->optionValues =  XtVaCreateManagedWidget(
	    "optionValues", xmTextWidgetClass, form,
	    XmNtopAttachment   , XmATTACH_WIDGET,
	    XmNtopWidget       , this->description,
	    XmNtopOffset       , 10,
	    XmNleftAttachment  , XmATTACH_POSITION,
	    XmNleftPosition    , 25,
	    XmNrightAttachment , XmATTACH_FORM,
	    XmNrightOffset     , 5,
	    XmNeditMode        , XmSINGLE_LINE_EDIT,
	    XmNeditable        , True,
	    NULL);
    } else {
	// This is to simplify specifying form attachments only.
	// Otherwise it's nonsense to make this assignment.
	// Later on I'll set optionValues back to NULL.
	this->optionValues = this->description;
    }

    Widget optionsLabel = XtVaCreateManagedWidget(
        "optionsLabel", xmLabelWidgetClass, form,
        XmNtopAttachment   , XmATTACH_WIDGET,
        XmNtopWidget       , this->optionValues,
        XmNtopOffset       , 10,
        XmNleftAttachment  , XmATTACH_FORM,
        XmNleftOffset      , 5,
        XmNrightAttachment , XmATTACH_POSITION,
        XmNrightPosition   , 25,
        XmNalignment       , XmALIGNMENT_BEGINNING,
	NULL);

    this->required = XtVaCreateManagedWidget(
        "required", xmToggleButtonWidgetClass, form,
        XmNtopAttachment   , XmATTACH_WIDGET,
        XmNtopWidget       , this->optionValues,
        XmNtopOffset       , 10,
        XmNleftAttachment  , XmATTACH_POSITION,
        XmNleftPosition    , 25,
        XmNshadowThickness , 0,
	NULL);

    this->descriptive = XtVaCreateManagedWidget(
        "descriptive", xmToggleButtonWidgetClass, form,
        XmNtopAttachment   , XmATTACH_WIDGET,
        XmNtopWidget       , this->optionValues,
        XmNtopOffset       , 10,
        XmNleftAttachment  , XmATTACH_WIDGET,
        XmNleftWidget      , this->required,
        XmNleftOffset      , 10,
        XmNshadowThickness , 0,
	NULL);

    this->hidden = XtVaCreateManagedWidget(
        "hidden", xmToggleButtonWidgetClass, form,
        XmNtopAttachment   , XmATTACH_WIDGET,
        XmNtopWidget       , this->optionValues,
        XmNtopOffset       , 10,
        XmNleftAttachment  , XmATTACH_WIDGET,
        XmNleftWidget      , this->descriptive,
        XmNleftOffset      , 10,
        XmNshadowThickness , 0,
	NULL);

    if (!input)
    {
	XtSetSensitive(this->required, False);
	XtSetSensitive(this->descriptive, False);
	XtSetSensitive(this->hidden, False);
	this->optionValues = NULL;
    }

    this->saveInitialValues();
    if (pd->isInput())
	this->changeInput(1);
    else
	this->changeOutput(1);

    return form;
}

Widget ParameterCDB::createInputs(Widget parent, Widget top)
{
    MacroParameterNode *n = (MacroParameterNode*)this->node;
    if (n->getParameterDefinition()->isOutput())
	return this->ConfigurationDialog::createInputs(parent, top);
    else
	return this->createParam(parent, top);
}

Widget ParameterCDB::createOutputs(Widget parent, Widget top)
{
    MacroParameterNode *n = (MacroParameterNode*)this->node;
    if (n->getParameterDefinition()->isInput())
	return this->ConfigurationDialog::createOutputs(parent, top);
    else
	return this->createParam(parent, top);
}


ParameterCDB::ParameterCDB( Widget parent, Node *node):
    ConfigurationDialog("parameterConfigurationDialog", parent, node)
{

    //
    // Install the default resources for THIS class (not the derived classes)
    //
    if (NOT ParameterCDB::ClassInitialized)
    {
	ASSERT(theApplication);
        ParameterCDB::ClassInitialized = TRUE;
	this->installDefaultResources(theApplication->getRootWidget());
    }

    this->initialPosition = 0;
    this->initialName = NULL;
    this->initialValue = NULL;
    this->initialDescription = NULL;
    this->initialRequired = FALSE;
    this->initialDescriptive = FALSE;
    this->initialHidden = FALSE;
    this->initialOptionValues = NULL;
}

ParameterCDB::~ParameterCDB()
{
    if (this->initialName)
	delete this->initialName;
    if (this->initialValue)
	delete this->initialValue;
    if (this->initialDescription)
	delete this->initialDescription;
    if (this->initialOptionValues)
	delete this->initialOptionValues;
}

//
// Install the default resources for this class.
//
void ParameterCDB::installDefaultResources(Widget baseWidget)
{
    this->setDefaultResources(baseWidget, ParameterCDB::DefaultResources);
    this->ConfigurationDialog::installDefaultResources(baseWidget);
}
void ParameterCDB::changeInput(int index)
{
    MacroParameterNode *n = (MacroParameterNode *)this->node;
    ParameterDefinition *pd = n->getParameterDefinition();
    if (pd->isOutput())
    {
	this->ConfigurationDialog::changeInput(index);
	return;
    }
    ASSERT(index == 1);

    XtVaSetValues(this->position, XmNiValue, n->getIndex(), NULL);

    XmTextSetString(this->name, (char *)pd->getNameString());

    const char *const *types = pd->getTypeStrings();
    if (types != NULL && types[0] != NULL)
    {
	int i = 0;
	int len = STRLEN(types[i]) + 1;
	for (++i; types[i] != NULL; ++i)
	    len += 4 + STRLEN(types[i]);
	char *s = new char[len];
	i = 0;
	strcpy(s, types[i]);
	for (++i; types[i] != NULL; ++i)
	{
	    strcat(s, " or ");
	    strcat(s, types[i]);
	}
	XmTextSetString(this->type, s);
	delete s;
    }
    else
	XmTextSetString(this->type, "");

    const char *dflt = pd->getDefaultValue();
    if (EqualString(dflt, "NULL"))
    {
	if (pd->isRequired())
	    pd->setDescriptiveValue("(none)");
	else
	    pd->setDescriptiveValue("(no default)");
    }

    XmTextSetString(this->value, (char *)pd->getDefaultValue());
    XmTextSetString(this->description, (char *)pd->getDescription());

    XtVaSetValues(this->required, XmNset, pd->isRequired(), NULL);
    XtVaSetValues(this->descriptive, XmNset, pd->isDefaultDescriptive(), NULL);
    XtVaSetValues(this->hidden, XmNset, !pd->getDefaultVisibility(), NULL);

    //
    // Gather up option values and put them into the text widget
    //
    if (pd->isInput()) {
	char* cp = this->getOptionValuesString();
	XmTextSetString (this->optionValues, cp);
	delete cp;
    }
}
void ParameterCDB::changeOutput(int index)
{
    MacroParameterNode *n = (MacroParameterNode *)this->node;
    ParameterDefinition *pd = n->getParameterDefinition();

    if (pd->isInput())
    {
	this->ConfigurationDialog::changeOutput(index);
	return;
    }
    ASSERT(index == 1);

    XtVaSetValues(this->position, XmNiValue, n->getIndex(), NULL);

    XmTextSetString(this->name, (char *)pd->getNameString());

    const char *const *types = pd->getTypeStrings();
    if (types != NULL && types[0] != NULL)
    {
	int i = 0;
	int len = STRLEN(types[i]) + 1;
	for (++i; types[i] != NULL; ++i)
	    len += 4 + STRLEN(types[i]);
	char *s = new char[len];
	i = 0;
	strcpy(s, types[i]);
	for (++i; types[i] != NULL; ++i)
	{
	    strcat(s, " or ");
	    strcat(s, types[i]);
	}
	XmTextSetString(this->type, s);
	delete s;
    }
    else
	XmTextSetString(this->type, "");

    XmTextSetString(this->value, (char *)pd->getDefaultValue());
    XmTextSetString(this->description, (char *)pd->getDescription());

    XtVaSetValues(this->required, XmNset, pd->isRequired(), NULL);
    XtVaSetValues(this->descriptive, XmNset, pd->isDefaultDescriptive(), NULL);
    XtVaSetValues(this->descriptive, XmNset, pd->getDefaultVisibility(), NULL);
}

void ParameterCDB::saveInitialValues()
{
    MacroParameterNode *n = (MacroParameterNode *)this->node;
    ParameterDefinition *pd = n->getParameterDefinition();

    this->initialPosition = n->getIndex();
    if (this->initialName)
	delete this->initialName;
    this->initialName = DuplicateString(pd->getNameString());

    if (this->initialValue)
	delete this->initialValue;
    this->initialValue = DuplicateString(pd->getDefaultValue());

    if (this->initialDescription)
	delete this->initialDescription;
    this->initialDescription = DuplicateString(pd->getDescription());

    this->initialRequired = pd->isRequired();
    this->initialDescriptive = pd->isDefaultDescriptive();
    this->initialHidden = !pd->getDefaultVisibility();

    if (this->initialOptionValues) delete this->initialOptionValues;
    this->initialOptionValues = this->getOptionValuesString();
}

//
// Caller must free the returned memory
//
char* ParameterCDB::getOptionValuesString()
{
    MacroParameterNode *n = (MacroParameterNode *)this->node;
    ParameterDefinition *pd = n->getParameterDefinition();
    const char *const *options = pd->getValueOptions();
    char* retval;
    if (options && options[0]) {
	char oval[1024];
	char tbuf[128];
	int len = 0;
	int i=0;
	while (options[i]) {
	    if (i) {
		oval[len++] = ';';
		oval[len++] = ' ';
		oval[len] = '\0';
	    }
	    sprintf (tbuf, " %s ", options[i]);
	    strcpy (&oval[len], tbuf);
	    len+= strlen(tbuf);
	    i++;
	}
	retval = DuplicateString(oval);
    } else {
	retval = new char[1];
	retval[0] = '\0';
    }
    return retval;
}

void ParameterCDB::restoreInitialValues()
{
    MacroParameterNode *n = (MacroParameterNode *)this->node;
    ParameterDefinition *pd = n->getParameterDefinition();

    n->moveIndex(this->initialPosition);

    pd->setName(this->initialName);
    if (this->initialDescriptive)
	pd->setDescriptiveValue(this->initialValue);
    else
	pd->setDefaultValue(this->initialValue);
    pd->setDescription(this->initialDescription);
    if (this->initialRequired)
	pd->setRequired();
    else
	pd->setNotRequired();

    if (this->initialHidden)
	pd->setDefaultVisibility(FALSE);
    else
	pd->setDefaultVisibility(TRUE);

    if (pd->isInput())
	this->changeInput(1);
    else
	this->changeOutput(1);

    if (pd->isInput()) {
	char* cp = this->getOptionValuesString();
	XmTextSetString (this->optionValues, cp);
	delete cp;
    }
}

boolean ParameterCDB::applyValues()
{
    MacroParameterNode *n = (MacroParameterNode *)this->node;
    ParameterDefinition *pd = n->getParameterDefinition();

    int position;
    XtVaGetValues(this->position, XmNiValue, &position, NULL);
    Boolean required;
    XtVaGetValues(this->required, XmNset, &required, NULL);
    Boolean descriptive;
    XtVaGetValues(this->descriptive, XmNset, &descriptive, NULL);
    char *value = XmTextGetString(this->value);
    char *descr = XmTextGetString(this->description);
    char *name = XmTextGetString(this->name);
    char *options = NULL;
    if (pd->isInput()) options = XmTextGetString(this->optionValues);
    boolean return_val = TRUE;

    //
    // Move the index after reading all the CDB values, because this resets 
    // the widgets via changeInput() and changeOutput().
    // moveIndex() will issue an error.
    //
    if (!n->moveIndex(position)) {
	if(name)  XtFree(name);     //	AJ
	if(descr) XtFree(descr);     //	AJ
	if(value)  XtFree(value);     //	AJ
	if(options) XtFree(options);
	return FALSE;
    }

    int   start = 0;
    if (!IsIdentifier(name, start))
    {
	ErrorMessage("Invalid parameter name `%s'.  It must start with a letter"
		     " and contain only letters, numbers, and underscores",
		     name);
	if(name)  XtFree(name);     //	AJ
	if(descr) XtFree(descr);     //	AJ
	if(value)  XtFree(value);     //	AJ
	if(options) XtFree(options);

	return FALSE;
    }
    else
    {
	if (name[start] != '\0')
	    SkipWhiteSpace(name,start);
	if (name[start] == '\0')
	{
	    char *begin, *end; 
	    //
	    // Strip leading and trailing white space
	    //
	    begin = name;
	    SkipWhiteSpace(begin);
	    end = begin;
	    FindWhiteSpace(end); 
	    *end = '\0';
	    
	    //
	    // See if the input is named input_%d or output_%d
	    // and if so, change it to match the possibly new index.
	    //
	    char buf[64];
	    if (n->isInput()) {
		strcpy(buf,"input_");
	    } else {
		strcpy(buf,"output_");
	    }
	    int buflen = strlen(buf);		
	    if (EqualSubstring(begin, buf, buflen)) {
		int endint = 0;
		if (IsInteger(begin+buflen,endint) && 
		    ((begin+buflen+endint) == end)) {
		   	sprintf(buf+buflen,"%d",position); 
			begin = buf;
		}
	    }

	    //
	    // Now make sure the parameter was not given the same name as 
	    // one of the other parameters in the macro.
	    // 
	    Network *net = n->getNetwork();
	    int name_clash=0, i;
	    const char *conflict = 
		net->nameConflictExists((MacroParameterNode*)this->node, begin);

	    if (!conflict) {
		if (n->isInput()) {
		    int count = net->getInputCount();
		    for (i=1 ; !name_clash && (i <= count) ; i++) {
			ParameterDefinition *opd = net->getInputDefinition(i); 
			if ((i != position) && 
			    EqualString(begin,opd->getNameString()))
			    name_clash = i;
		    }
		} else {
		    int count = net->getOutputCount();
		    for (i=1 ; !name_clash && (i <= count) ; i++) {
			ParameterDefinition *opd = net->getOutputDefinition(i); 
			if ((i != position) && 
			    EqualString(begin,opd->getNameString()))
			    name_clash = i;
		    }
		}
	    }

	    //
	    // And finally set the name.
	    //
	    if (conflict) {
		ErrorMessage(
		    "A %s with name \"%s\" already exists.", conflict, begin);
		return_val = FALSE;
	    } else if (name_clash) {
		ErrorMessage(
		"Parameter name `%s' is the same name used by parameter #%d.\n",
		    begin,name_clash);
		return_val = FALSE;
	    } else {
		pd->setName(begin);
		n->getNetwork()->setDirty();
	    }
	}
	else
	{
	    ErrorMessage(
		"Invalid parameter name `%s'.  It must start with a letter"
		" and contain only letters, numbers, and underscores",
		name);
	    return_val = FALSE;
	}
    }

    if (required)
    {
	if (!pd->isRequired())
	{
	    if (EqualString(value, "(no default)") || EqualString(value, ""))
		pd->setDescriptiveValue("(none)");
	    else
		pd->setDescriptiveValue(value);
	    pd->setRequired();
	    n->getNetwork()->setDirty();
	}
	else
	    pd->setDescriptiveValue(value);
    }
    else
    {
	if (descriptive)
	{
	    if (EqualString(value, ""))
		pd->setDescriptiveValue("(no default)");
	    else if (value[0] == '(' && value[STRLEN(value)-1] == ')')
		pd->setDescriptiveValue(value);
	    else
	    {
		char *newValue = new char[STRLEN(value)+3];
		strcpy(newValue, "(");
		strcat(newValue, value);
		strcat(newValue, ")");
		pd->setDescriptiveValue(newValue);
		delete newValue;
	    }
	    n->getNetwork()->setDirty();
	}
	else
	{
	    if (EqualString(value, ""))
	    {
		if (pd->isRequired())
		    pd->setDescriptiveValue("(none)");
		else
		    pd->setDescriptiveValue("(no default)");
		n->getNetwork()->setDirty();
	    }
	    else if (!EqualString(value, pd->getDefaultValue()) ||
		     pd->isDefaultDescriptive())
	    {
		List typeList;
		if (!DXType::ValueToType(value, typeList) ||
		    !DXType::MatchTypeLists(typeList, *pd->getTypes()) ||
		    !pd->setDefaultValue(value))
		{
		    DXType *dxtype;
		    ListIterator li(*pd->getTypes());
		    while ( (dxtype = (DXType*)li.getNext()) )
		    {
			Type type = dxtype->getType();
			char *newVal = DXValue::CoerceValue(value, type);
			if (newVal)
			{
			    if (pd->setDefaultValue(newVal))
			    {
				delete newVal;
				break;
			    }
			    else
				delete newVal;
			}
		    }
		    if (dxtype == NULL)
		    {
			ErrorMessage("`%s' is not a valid value "
				     "for parameter %s.",
			    value? value: "NULL", pd->getNameString());
	    		return_val = FALSE;
		    }
		}
		n->getNetwork()->setDirty();
	    }
	}
	if (pd->isRequired())
	{
	    if (EqualString(pd->getDefaultValue(), "(none)") ||
		EqualString(pd->getDefaultValue(), ""))
		pd->setDescriptiveValue("(no default)");
	    pd->setNotRequired();
	    n->getNetwork()->setDirty();
	}
    }

    //
    // Error check option values.
    // - Try to Coerce each one to our set of types
    //
    ParameterDefinition *pdef = n->getParameterDefinition();
    pdef->removeValueOptions();
    if (options && options[0]) {
	int i=0;
	if (ParseMDFOptions (pdef, options)) {
	    const char *const *ostrings = pdef->getValueOptions();
	    boolean can_coerce = TRUE;
	    if (ostrings && ostrings[0]) {
		List *types = pdef->getTypes();
		while (ostrings[i]) {
		    const char* option = ostrings[i];

		    boolean coerced = FALSE;

		    ListIterator iter;
		    DXType *dxtype;
		    for (iter.setList(*types); (dxtype = (DXType*)iter.getNext()) ; ) {
			char* s = DXValue::CoerceValue (option, dxtype->getType());
			if (s) {
			    delete s;
			    coerced = TRUE;
			    break;
			}
		    }
		    can_coerce&= coerced;
		    if (!coerced) break;
		    i++;
		}
	    }
	    if (!can_coerce) {
		ErrorMessage(
		    "'%s' is not a valid value for Option values.",
		     ostrings[i]);
		return_val = FALSE;
	    }
	} else {
	    ErrorMessage(
		"'%s' is not a valid value for Option values.",
		 options);
	    return_val = FALSE;
	}
    }


    char *p = descr;
    boolean error = FALSE;
    for (; *p; ++p)
	if (*p == '\'')
	{
	    ErrorMessage(
		"Description %s must not contain the \"'\" character.",
		 descr);
	    error = TRUE;
	    return_val = FALSE;
	    break;
	}

    if (!error) 
	pd->setDescription(descr);

    Boolean hidden;
    XtVaGetValues(this->hidden, XmNset, &hidden, NULL);
    pd->setDefaultVisibility(!hidden);

    XmTextSetString(this->value, (char*)pd->getDefaultValue());
    XmTextSetString(this->description, (char*)pd->getDescription());
    XmTextSetString(this->name, (char*)pd->getNameString());
    required = pd->isRequired();
    XtVaSetValues(this->required, XmNset, required, NULL);
    descriptive = pd->isDefaultDescriptive();
    XtVaSetValues(this->descriptive, XmNset, descriptive, NULL);

    XtFree(name);
    XtFree(descr);
    XtFree(value);
    if (options) XtFree(options);

    return return_val;
}
