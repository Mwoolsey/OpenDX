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
#include "ToggleAttrDialog.h"
#include "ToggleInstance.h"
#include "Application.h"
#include "ErrorDialogManager.h"
//#include "DXValue.h"	// For IsTensor() and IsVector().


boolean ToggleAttrDialog::ClassInitialized = FALSE;
String  ToggleAttrDialog::DefaultResources[] =
{
    "*accelerators:		#augment\n"
	"<Key>Return:			BulletinBoardReturn()",
    ".width: 		425", 	     // HP may have trouble with this 
    ".height: 		140", 	     // HP may have trouble with this 

    "*setLabel.labelString:	Button down (set) value:",
    "*resetLabel.labelString:   Button up (unset) value:",
    "*cancelButton.labelString:Cancel", 
    "*okButton.labelString:OK", 

    NULL
};


ToggleAttrDialog::ToggleAttrDialog(Widget 	  parent,
				     const char* title,
				     ToggleInstance *si) :
    SetAttrDialog("toggleAttr", parent, title, (InteractorInstance*)si)

{
    this->setText = NULL;
    this->resetText = NULL;

    if (NOT ToggleAttrDialog::ClassInitialized)
    {
        ToggleAttrDialog::ClassInitialized = TRUE;
	this->installDefaultResources(theApplication->getRootWidget());
    }
}

ToggleAttrDialog::~ToggleAttrDialog()
{
}


//
// Called before the root widget is created.
//
//
// Install the default resources for this class.
//
void ToggleAttrDialog::installDefaultResources(Widget  baseWidget)
{
    this->setDefaultResources(baseWidget, ToggleAttrDialog::DefaultResources);
    this->SetAttrDialog::installDefaultResources( baseWidget);
}

//
// Desensitize the data-driven attributes in the dialog.
//
void ToggleAttrDialog::setAttributeSensitivity()
{
    ToggleInstance *ti = (ToggleInstance*)this->interactorInstance;
    ToggleNode *tnode = (ToggleNode*)ti->getNode();
    Boolean s;

    if (tnode->isSetAttributeVisuallyWriteable())
	s = True;
    else
	s = False;
    XtSetSensitive(this->setText, s);

    if (tnode->isResetAttributeVisuallyWriteable())
	s = True;
    else
	s = False;
    XtSetSensitive(this->resetText, s);
}

//
// Read the current attribute settings from the dialog and set them in  
// the given InteractorInstance indicated with this->interactorInstance 
//
boolean ToggleAttrDialog::storeAttributes()  
{
    ToggleInstance *ti = (ToggleInstance*)this->interactorInstance;
    ToggleNode *tnode = (ToggleNode*)ti->getNode();
    char *setval = NULL, *resetval = NULL;

    if (tnode->isSetAttributeVisuallyWriteable())
    	setval = XmTextGetString(this->setText);
    if (tnode->isResetAttributeVisuallyWriteable())
     	resetval = XmTextGetString(this->resetText);

    if (!tnode->setToggleValues(setval,resetval))  {
	ModalErrorMessage(this->getRootWidget(),
			"Set and Unset values are incompatible");
	return FALSE;
    }

    tnode->sendValues();

    if (setval) XtFree (setval);
    if (resetval) XtFree (resetval);
    return TRUE;
    
}
//
// Display the options stored in the node. 
//
void ToggleAttrDialog::updateDisplayedAttributes()
{
    ToggleInstance *ti = (ToggleInstance*)this->interactorInstance;
    ToggleNode *tnode = (ToggleNode*)ti->getNode();

    ASSERT(ti);

    char *setval = tnode->getSetValue();
    char *resetval = tnode->getResetValue();
   
    ASSERT(setval && resetval);

    XmTextSetString(this->setText, setval);
    XmTextSetString(this->resetText, resetval);

    delete setval;
    delete resetval;
}

//
// This is a noop since all attributes are global and changes made in
// the dialog are saved inside the widgets (i.e. updateDisplayedAttributes()
// gets all its information from the node directly).
//
void ToggleAttrDialog::loadAttributes()
{
    return;
}

void ToggleAttrDialog::createAttributesPart(Widget mainForm)
{

    //
    // The set value label
    //
    Widget setLabel = XtVaCreateManagedWidget("setLabel",
                        xmLabelWidgetClass ,    mainForm,
                        XmNtopAttachment,       XmATTACH_FORM,
                        XmNtopOffset,           10,
                        XmNleftAttachment,      XmATTACH_FORM,
                        XmNleftOffset,          5,
                        NULL);

    //
    // The set value text window
    //
    this->setText = XtVaCreateManagedWidget("setText",
                        xmTextWidgetClass , mainForm,
                        XmNtopAttachment,       XmATTACH_FORM,
                        XmNtopOffset,           10,
                        XmNrightAttachment,      XmATTACH_FORM,
                        XmNrightOffset,          5,
                        XmNcolumns,             14,
                        NULL);
 
    //
    // The reset value label
    //
    Widget resetLabel = XtVaCreateManagedWidget("resetLabel",
                        xmLabelWidgetClass ,    mainForm,
                        XmNtopAttachment,       XmATTACH_WIDGET,
                        XmNtopWidget,       	setLabel,
                        XmNtopOffset,           10,
                        XmNleftAttachment,      XmATTACH_FORM,
                        XmNleftOffset,          5,
                        NULL);

    //
    // The resset value text window
    //
    this->resetText = XtVaCreateManagedWidget("resetText",
                        xmTextWidgetClass , mainForm,
                        XmNtopAttachment,       XmATTACH_WIDGET,
                        XmNtopWidget,       	this->setText,
                        XmNtopOffset,           10,
                        XmNrightAttachment,      XmATTACH_FORM,
                        XmNrightOffset,          5,
                        XmNcolumns,             14,
                        NULL);
 
    //
    // The OK and CANCEL buttons.
    //
    Widget separator = XtVaCreateManagedWidget(
        "separator", xmSeparatorWidgetClass, mainForm,
                        XmNtopAttachment   , XmATTACH_WIDGET,
                        XmNtopWidget       , this->resetText,
                        XmNtopOffset       , 10,
                        XmNleftAttachment  , XmATTACH_FORM,
                        XmNleftOffset      , 2,
                        XmNrightAttachment , XmATTACH_FORM,
                        XmNrightOffset     , 2,
                        NULL);


    this->ok =  XtVaCreateManagedWidget(
        "okButton", xmPushButtonWidgetClass, mainForm,
                        XmNtopAttachment   , XmATTACH_WIDGET,
                        XmNtopWidget       , separator,
                        XmNtopOffset       , 10,
                        XmNleftAttachment  , XmATTACH_FORM,
                        XmNleftOffset      , 5,
                        XmNbottomAttachment, XmATTACH_FORM,
                        XmNbottomOffset    , 10,
                        XmNrecomputeSize   , False,
                        XmNwidth           , 75,
                        NULL);


    this->cancel =  XtVaCreateManagedWidget(
        "cancelButton", xmPushButtonWidgetClass, mainForm,
                        XmNtopAttachment   , XmATTACH_WIDGET,
                        XmNtopWidget       , separator,
                        XmNtopOffset       , 10,
                        XmNrightAttachment , XmATTACH_FORM,
                        XmNrightOffset     , 5,
                        XmNbottomAttachment, XmATTACH_FORM,
                        XmNbottomOffset    , 10,
                        XmNrecomputeSize   , False,
                        XmNwidth           , 75,
                        NULL);


    this->updateDisplayedAttributes();
}

