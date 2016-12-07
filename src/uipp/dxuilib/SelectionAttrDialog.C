/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/Label.h>
#include <Xm/PushB.h>
#include <Xm/Text.h>
#include <Xm/List.h>
#include <Xm/RowColumn.h>
#include <Xm/Separator.h>

#include "lex.h"
#include "SetAttrDialog.h"
#include "SelectionAttrDialog.h"
#include "SelectionInstance.h"
#include "LocalAttributes.h"
#include "Application.h"
#include "ErrorDialogManager.h"
#include "DXValue.h"	// For IsTensor() and IsVector().
#include "DXTensor.h"	
//
// Define the format that we use to put value/label pairs in the option list
//
//#define VALUE_LABEL_SPRINTF_FORMAT  "%s = %s\n"
//#define VALUE_LABEL_SSCANF_FORMAT   "%[^=]= %[^\n]"
#define VALUE_LABEL_SPRINTF_FORMAT  "%s = %s"
#define VALUE_LABEL_SSCANF_FORMAT   "%[^=]= %s"


boolean SelectionAttrDialog::ClassInitialized = FALSE;
String  SelectionAttrDialog::DefaultResources[] =
{

    "*accelerators:		#augment\n"
#if 1
	"<Key>Return:			BulletinBoardReturn()",
#else
	"<Key>Return:			BulletinBoardReturn()\n"
	"<Btn2Down>,<Btn2Up>:		HELP!",
#endif
    ".width: 300",
    "*valueLabel.labelString:Value:",
    "*labelLabel.labelString:Label:",
    "*cancelButton.labelString:Cancel", 
    "*okButton.labelString:OK", 

    NULL
};


SelectionAttrDialog::SelectionAttrDialog( Widget 	  parent,
				     const char* title,
				     SelectionInstance *si) :
    SetAttrDialog("selectionAttr", parent, title, (InteractorInstance*)si), 
    ListEditor(name) 
{
    this->labelText = NULL;
    this->valueText = NULL;
    this->valueType = DXType::DetermineListItemType(
				si->getValueOptionsAttribute());

    if (NOT SelectionAttrDialog::ClassInitialized)
    {
        SelectionAttrDialog::ClassInitialized = TRUE;
	this->installDefaultResources(theApplication->getRootWidget());
    }
}

SelectionAttrDialog::~SelectionAttrDialog()
{
}


//
// Called before the root widget is created.
//
//
// Install the default resources for this class.
//
void SelectionAttrDialog::installDefaultResources(Widget  baseWidget)
{
    this->setDefaultResources(baseWidget,
				SelectionAttrDialog::DefaultResources);
    this->setDefaultResources(baseWidget,
				ListEditor::DefaultResources);
    this->SetAttrDialog::installDefaultResources( baseWidget);
}

// 
// Extract the value and label string from the list item. 
// value must be large enough to hold all of item.
//
static void ParseListItem(const char *item, char *value, char *label)
{
    char *p;
 
    strcpy(value, item);
    p = strstr(value," = ");
    ASSERT(p);
    *p = '\0';	// Terminate v
    label = strcpy(label, p + 3);
}
//
// Desensitize the data-driven attributes in the dialog.
//
void SelectionAttrDialog::setAttributeSensitivity()
{
    SelectionInstance *si = (SelectionInstance*)this->interactorInstance;
    SelectionNode *snode = (SelectionNode*)si->getNode();

    Boolean s = (snode->isDataDriven() == FALSE ? True : False);

    XtSetSensitive(this->labelText, s);
    XtSetSensitive(this->valueText, s);
    // don't desensitize the list because it prevents scrolling.
    //XtSetSensitive(this->valueList, s);
    XtSetSensitive(this->addButton, s);
    XtSetSensitive(this->deleteButton, s);
}


//
// Called by this->listCallback() when a new item is selected in the list.
// This should update the the values displayed in the editor to match
// the values selected (i.e. listItem).
//
void SelectionAttrDialog::updateValueEditor(const char *text)
{
    char        *label, *value;
    int		len = STRLEN(text);

    label = new char [len+1]; 
    value = new char [len+1]; 

    ParseListItem(text,value,label);
    XmTextSetString(this->valueText,value);
    XmTextSetString(this->labelText,label);

    delete label;
    delete value;
}
//
// Get the value in the value editor that should be added to the list.
// The returned string must be freed by the caller.
// Returns NULL if the value in the value editor is not valid and
// can issue an error message in that case. 
//
char *SelectionAttrDialog::getValueEditorValue()
{
    char *label, *value, *buffer = NULL;
    Type	type;
    char *saved_value=NULL;
    SelectionInstance *si;
    SelectionNode *snode;
    int cnt;
    boolean accept_any;

    /*
     * Get the current label text.
     */
    label = XmTextGetString(this->labelText);
    value = XmTextGetString(this->valueText);

    if (IsBlankString(label))
    {
	/*
	 * If the text is blank, we cannot add it to the list.
	 */
	ModalErrorMessage(this->getRootWidget(),
	    "The label field must contain a value.");
	goto error;
    } 
    else if (IsBlankString(value))
    {
	/*
	 * If the value is blank, we cannot add it to the list.
	 */
	ModalErrorMessage(this->getRootWidget(),
	    "The value field must contain a value.");
	goto error;
    }
    else if (strchr(label,'"'))
    {
	/*
	 * The label can not contain double quotes. 
	 */
	ModalErrorMessage(this->getRootWidget(), 
				"The label can not contain double quotes.");
	goto error;
    }

    //
    // Use the output parameter to see if the given value is an acceptable
    // output value.  We must be careful not to send the output value to
    // the server during any of this.
    // FIXME: this could leave a previously clean output dirty.
    //
    si = (SelectionInstance*) this->interactorInstance;
    snode = (SelectionNode*)si->getNode();
    cnt = this->getListItemCount();
    if ((cnt == 0) || ((cnt == 1) && this->getSelectedItemIndex())) {
	// Accept any value
	accept_any = TRUE;
	this->valueType = DXType::UndefinedType;
    } else
	accept_any = FALSE;
    saved_value = DuplicateString(snode->getOutputValueString(1));
    if ((type = snode->setOutputValue(1,value, this->valueType, FALSE)) != 
		DXType::UndefinedType)
    {
	//
	// If the output value is a list type, the items are restricted to
	// the types of items that can be in the list.
	//
	boolean wasList = type & DXType::ListType ? TRUE : FALSE;
	type = type & ~DXType::ListType; 
	//
	// Reset the currently allowed value type
	// FIXME: this depends on knowing that values can be integer, scalar,
	// 	vector or tensor.
	//
	if (this->valueType == DXType::UndefinedType &&
	    type == DXType::ValueType) {
	     if (DXValue::IsValidValue (value,DXType::IntegerType))
		type = DXType::IntegerType;
	     else if (DXValue::IsValidValue(value,DXType::ScalarType))
		type = DXType::ScalarType;
	     else if (DXValue::IsValidValue(value,DXType::VectorType))
		type = DXType::VectorType;
	     else if (DXValue::IsValidValue(value,DXType::TensorType))
		type = DXType::TensorType;
	     else if (DXValue::CoerceValue (value, DXType::VectorType))
		type = DXType::VectorType;
	     else if (DXValue::CoerceValue (value, DXType::TensorType))
		type = DXType::TensorType;
	     else {
	     	ModalErrorMessage(this->getRootWidget(),
			     "'%s' is not a supported type.", value);
		goto error;
	    }

	
	}
	//
	// For, vectors and tensors, verify that the rank and shape are 
	// the same.
	//
	boolean dim_match = TRUE;
	if (!accept_any && 
	    (type == DXType::VectorType || type == DXType::TensorType)) {
	    DXTensor first, proposed;
	    char first_value[1024], label[1024],*text = this->getListItem(1);
	    ASSERT(text);
	    ParseListItem(text, first_value, label);
	    delete text;
	    dim_match = first.setValue(first_value);
	    if (dim_match) {
		proposed.setValue(value);
		if (proposed.getDimensions() != first.getDimensions())
		    dim_match = FALSE;
		int i;
		for (i=1 ; dim_match && (i<=proposed.getDimensions()) ; i++) {
		    if (proposed.getDimensionSize(i) != 
						first.getDimensionSize(i))
		    dim_match = FALSE;
		} 
	    }
	}	
	if (dim_match) {
	    this->valueType = type;
	    /*
	     * generate the value/name pair string.
	     */
	    const char *nvalue1 = snode->getOutputValueString(1);
	    char *nvalue2 ;
	    if (wasList) {
		nvalue2 = DXValue::GetListItem(nvalue1,1,type | DXType::ListType);
		ASSERT(nvalue2);
	    } else {
		nvalue2 = (char*)nvalue1;
	    }
	    XmTextSetString(this->valueText, (char*)nvalue2);
	    buffer = new char [STRLEN(label) + STRLEN(nvalue2) + 
			     STRLEN(VALUE_LABEL_SPRINTF_FORMAT) + 1];
	    sprintf(buffer, VALUE_LABEL_SPRINTF_FORMAT, nvalue2, label);
	    if (nvalue2 != nvalue1)
		delete nvalue2;
	} else {
	    ModalErrorMessage(this->getRootWidget(), 
			"'%s' is not the same dimension as current value(s).", 
			value);
	}
        snode->setOutputValue(1,saved_value,DXType::UndefinedType,FALSE);
    }
    else 
    {
	if (accept_any)
	    ModalErrorMessage(this->getRootWidget(), 
			"'%s' is not a valid value.", value);
	else
	    ModalErrorMessage(this->getRootWidget(), 
			"'%s' is not the same type as current value(s).", 
			value);
    }

error:
    if (saved_value) delete saved_value;
    XtFree(label);
    XtFree(value);
    return buffer;
}

//
// Read the current attribute settings from the dialog and set them in  
// the given InteractorInstance indicated with this->interactorInstance 
//
boolean SelectionAttrDialog::storeAttributes()
{
    int 	i, count;
    char  	*text, value[1024], label[1024];
    SelectionInstance *si = (SelectionInstance*)this->interactorInstance;
 
    count = this->getListItemCount();
    char *vallist = new char[count * 128 + 6];
    char *strlist = new char[count * 256 + 6];

    if (count > 0) {
	strcpy(vallist,"{ ");
	strcpy(strlist,"{ ");

	for (i=1 ; i<=count ; i++) {
	    text = this->getListItem(i);
	    ParseListItem(text, value, label);
	    strcat(vallist,value); 
	    strcat(strlist,"\""); strcat(strlist,label); strcat(strlist,"\""); 
	    if (i < count) {
	       strcat(strlist,", ");
	       strcat(vallist,", ");
	    } else {
	       strcat(strlist," ");
	       strcat(vallist," ");
	    }
	    XtFree(text);
	}
	strcat(vallist,"}");
	strcat(strlist,"}");
    } else {
	strcpy(vallist,"NULL");
	strcpy(strlist,"NULL");
    }

    SelectionNode *snode = (SelectionNode*)si->getNode();
    snode->installNewOptions(vallist, strlist, TRUE);

    delete vallist;
    delete strlist;

    return TRUE;
}
//
// Display the options stored in the node. 
//
void SelectionAttrDialog::updateDisplayedAttributes()
{
    SelectionInstance *si = (SelectionInstance*)this->interactorInstance;
    int options,i;

    ASSERT(si);

    options = si->getOptionCount();

    //
    // Now reload the list. 
    //
    if (options != 0)
    {
        char **strings = new char* [options];
	for (i=0 ; i<options ; i++) {
	    char *value, *name;
	    name  = si->getOptionNameString(i+1);
	    value = si->getOptionValueString(i+1);
	    int len = STRLEN(name) + STRLEN(value) + 
				 STRLEN(VALUE_LABEL_SPRINTF_FORMAT) + 1;
	    strings[i] = new char[len];
	    sprintf(strings[i], VALUE_LABEL_SPRINTF_FORMAT, value, name);
	    delete value;
	    delete name;
	}
	this->replaceListItems((const char**)strings, options);
	for (i=0 ; i<options ; i++) 
	    delete strings[i];
	delete  strings;
    }

    XmTextSetString(this->valueText, "0");
    XmTextSetString(this->labelText, "");
}

//
// This is a noop since all attributes are global and changes made in
// the dialog are saved inside the widgets (i.e. updateDisplayedAttributes()
// gets all its information from the node directly).
//
void SelectionAttrDialog::loadAttributes()
{
    return;
}

void SelectionAttrDialog::createAttributesPart(Widget mainForm)
{
    this->createListEditor(mainForm);
    this->updateDisplayedAttributes();
}

Widget SelectionAttrDialog::createValueEditor(Widget mainForm)
{

    /////////////////////////////////////////////////////////////
    // Create the widgets from left to right and top to bottom.//
    /////////////////////////////////////////////////////////////

    // XtVaSetValues(mainForm,XmNautoUnmanage, False, NULL);

    XtVaSetValues(this->valueList,XmNvisibleItemCount, 10, NULL);

    //
    // The OK and CANCEL buttons. 
    //

    this->ok =  XtVaCreateManagedWidget(
        "okButton", xmPushButtonWidgetClass, mainForm, 
			XmNleftAttachment  , XmATTACH_FORM,
			XmNleftOffset	   , 5,
			XmNbottomAttachment, XmATTACH_FORM,
			XmNbottomOffset	   , 10,
			XmNrecomputeSize   , False,
			XmNwidth	   , 75,
			NULL);
		

    this->cancel =  XtVaCreateManagedWidget(
        "cancelButton", xmPushButtonWidgetClass, mainForm,
			XmNrightAttachment , XmATTACH_FORM,
			XmNrightOffset	   , 5,
			XmNbottomAttachment, XmATTACH_FORM,
			XmNbottomOffset	   , 10,
			XmNrecomputeSize   , False,
			XmNwidth	   , 75,
			NULL);

    Widget separator = XtVaCreateManagedWidget(
        "separator", xmSeparatorWidgetClass, mainForm, 
			XmNbottomAttachment, XmATTACH_WIDGET,
			XmNbottomWidget    , this->ok,
			XmNbottomOffset	   , 10,
			XmNleftAttachment  , XmATTACH_FORM,
			XmNleftOffset	   , 2, 
			XmNrightAttachment , XmATTACH_FORM,
			XmNrightOffset	   , 2, 
			NULL);

    //
    // The value text window 
    //
    this->valueText = XtVaCreateManagedWidget("valueText",
			xmTextWidgetClass , mainForm,
			XmNbottomAttachment, 	XmATTACH_WIDGET,
			XmNbottomWidget, 	separator,
			XmNbottomOffset,	10,
			XmNleftAttachment, 	XmATTACH_FORM,
			XmNleftOffset,		5,
			XmNcolumns,		14,
			NULL); 

    this->installValueEditorCallback(this->valueText, XmNactivateCallback);

    //
    // The label text window 
    //
    this->labelText = XtVaCreateManagedWidget("labelText",
			xmTextWidgetClass , mainForm,
			XmNbottomAttachment, 	XmATTACH_WIDGET,
			XmNbottomWidget, 	separator,
			XmNbottomOffset,	10,
			XmNleftAttachment, 	XmATTACH_WIDGET,
			XmNleftWidget, 		this->valueText,
			XmNleftOffset,		10,
			XmNrightAttachment, 	XmATTACH_FORM,
			XmNrightOffset,		5,
			NULL); 

    this->installValueEditorCallback(this->labelText, XmNactivateCallback);


    //
    // The Value label 
    //
    Widget valueLabel = XtVaCreateManagedWidget("valueLabel",
			xmLabelWidgetClass , 	mainForm,
			XmNbottomAttachment, 	XmATTACH_WIDGET,
			XmNbottomWidget, 	this->valueText,
			XmNbottomOffset,	5,
			XmNleftAttachment, 	XmATTACH_FORM,
			XmNleftOffset,		5,
			NULL); 
    //
    // The Label label 
    //
    Widget labelLabel = XtVaCreateManagedWidget("labelLabel",
			xmLabelWidgetClass, 	mainForm,
			XmNbottomAttachment, 	XmATTACH_WIDGET,
			XmNbottomWidget, 	this->valueText,
			XmNbottomOffset,	5,
			XmNleftAttachment, 	XmATTACH_OPPOSITE_WIDGET,
			XmNleftWidget, 		this->labelText,
			XmNleftOffset, 	 	0,
			NULL); 

    return valueLabel;
}
