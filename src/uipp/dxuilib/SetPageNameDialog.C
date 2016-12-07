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
#include <Xm/Label.h>
#include <Xm/PushB.h>
#include <Xm/Separator.h>
#include <Xm/Text.h>
#include <Xm/ToggleB.h>

#include "../widgets/Number.h"
#include "../widgets/Stepper.h"
#include <ctype.h> // for isalnum

#include "DXStrings.h"
#include "lex.h"
#include "SetPageNameDialog.h"
#include "PageSelector.h"
#include "EditorWorkSpace.h"

#include "DXApplication.h"
#include "ErrorDialogManager.h"

#include "DictionaryIterator.h"
#include "XmUtility.h"

boolean SetPageNameDialog::ClassInitialized = FALSE;
String SetPageNameDialog::DefaultResources[] =
{
    "*nameLabel.labelString:            Page Name:",
    "*nameLabel.foreground:             SteelBlue",
    "*positionLabel.labelString:        Tab Position:",
    "*positionLabel.foreground:         SteelBlue",
    "*psToggle.shadowThickness:		0",
    "*psToggle.labelString:		",
    "*psLabel.labelString:		Included in Postscript Output:",
    "*psLabel.foreground:               SteelBlue",
#if defined(AUTO_REPOSITION)
    "*scrollToggle.shadowThickness:	0",
    "*scrollToggle.labelString:		",
    "*scrollLabel.labelString:		Automatically Reposition Page:",
    "*scrollLabel.foreground:           SteelBlue",
#endif
    "*position.editable:		True",
    "*okButton.labelString:             OK",
    "*okButton.width:                   60",
    "*applyButton.labelString:          Apply",
    "*applyButton.width:                60",
    "*cancelButton.labelString:         Cancel",
    "*cancelButton.width:               60",
    "*accelerators:           		#augment\n"
    "<Key>Return:                   	BulletinBoardReturn()",
    NULL
};


SetPageNameDialog::SetPageNameDialog(Widget parent, PageSelector* psel):
    Dialog("setPageNameDialog", parent)
{
    this->selector = psel;
    this->page_name_widget = NUL(Widget);
    this->page_name = NUL(char*);
    this->ps_toggle = NUL(Widget);
    this->auto_scroll_toggle = NUL(Widget);
    this->incl = FALSE;
    this->position = 1;
    this->position_number = NUL(Widget);
    this->stop_updates = FALSE;
    if (NOT SetPageNameDialog::ClassInitialized)
    {
        SetPageNameDialog::ClassInitialized = TRUE;
	this->installDefaultResources(theApplication->getRootWidget());
    }
}

//
// Install the default resources for this class.
//
void SetPageNameDialog::installDefaultResources(Widget  baseWidget)
{
    this->setDefaultResources(baseWidget, SetPageNameDialog::DefaultResources);
    this->Dialog::installDefaultResources( baseWidget);
}

SetPageNameDialog::~SetPageNameDialog()
{
    if (this->page_name) delete this->page_name;
}


Widget SetPageNameDialog::createDialog(Widget parent)
{
    Arg arg[10];
    int n = 0;
    //XtSetArg(arg[n], XmNdialogStyle, XmDIALOG_PRIMARY_APPLICATION_MODAL); n++;
    XtSetArg(arg[n], XmNautoUnmanage, False); n++;
    XtSetArg(arg[n], XmNminWidth, 250); n++;
    XtSetArg(arg[n], XmNminHeight, 155); n++;
    XtSetArg(arg[n], XmNwidth, 250); n++;
    Widget dialog = this->CreateMainForm(parent, this->name, arg, n);

    XtVaSetValues(XtParent(dialog), XmNtitle, "Page Configuration...", NULL);

    Widget w = XtVaCreateManagedWidget("nameLabel", 
	xmLabelWidgetClass, dialog,
        XmNtopAttachment   , XmATTACH_FORM,
        XmNtopOffset       , 10,
        XmNleftAttachment  , XmATTACH_FORM,
        XmNleftOffset      , 5,
    NULL);

    this->page_name_widget = XtVaCreateManagedWidget(
	"name", xmTextWidgetClass, dialog,
        XmNtopAttachment   , XmATTACH_FORM,
        XmNtopOffset       , 7,
        XmNrightAttachment , XmATTACH_FORM,
        XmNrightOffset     , 10,
	XmNmaxLength	   , 12,
	XmNcolumns	   , 12,
    NULL);

    w = XtVaCreateManagedWidget ("psLabel",
	xmLabelWidgetClass, dialog,
	XmNtopAttachment, XmATTACH_WIDGET,
	XmNtopWidget, this->page_name_widget,
	XmNtopOffset, 10,
	XmNleftAttachment, XmATTACH_FORM,
	XmNleftOffset, 5,
    NULL);

    this->ps_toggle = XtVaCreateManagedWidget("psToggle",
	xmToggleButtonWidgetClass, dialog,
	XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
	XmNtopWidget, w,
	XmNtopOffset, 0,
        XmNrightAttachment , XmATTACH_FORM,
        XmNrightOffset     , 10,
	XmNspacing, 0,
    NULL);

#if defined(AUTO_REPOSITION)
    w = XtVaCreateManagedWidget ("scrollLabel",
	xmLabelWidgetClass, dialog,
	XmNleftAttachment, XmATTACH_FORM,
	XmNleftOffset, 5,
	XmNtopAttachment, XmATTACH_WIDGET,
	XmNtopWidget, this->ps_toggle,
	XmNtopOffset, 10,
    NULL);

    this->auto_scroll_toggle = XtVaCreateManagedWidget("scrollToggle",
	xmToggleButtonWidgetClass, dialog,
	XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
	XmNtopWidget, w,
	XmNtopOffset, 0,
        XmNrightAttachment , XmATTACH_FORM,
        XmNrightOffset     , 10,
	XmNspacing, 0,
    NULL);
#endif

    w = XtVaCreateManagedWidget ("positionLabel",
	xmLabelWidgetClass, dialog,
	XmNleftAttachment, XmATTACH_FORM,
	XmNleftOffset, 5,
	XmNtopAttachment, XmATTACH_WIDGET,
#if defined(AUTO_REPOSITION)
	XmNtopWidget, this->auto_scroll_toggle,
#else
	XmNtopWidget, this->ps_toggle,
#endif
	XmNtopOffset, 10,
    NULL);

    this->position_number = XtVaCreateManagedWidget ("position",
	xmStepperWidgetClass, dialog,
	XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
	XmNtopWidget, w,
	XmNtopOffset, 0,
        XmNrightAttachment , XmATTACH_FORM,
        XmNrightOffset     , 10,
    NULL);

    Widget separator = XtVaCreateManagedWidget(
        "separator", xmSeparatorWidgetClass, dialog,
        XmNtopAttachment   , XmATTACH_WIDGET,
        XmNtopWidget       , this->position_number,
        XmNtopOffset       , 10,
        XmNleftAttachment  , XmATTACH_FORM,
        XmNleftOffset      , 0,
        XmNrightAttachment , XmATTACH_FORM,
        XmNrightOffset     , 0,
    NULL);

    this->ok = XtVaCreateManagedWidget(
        "okButton", xmPushButtonWidgetClass, dialog,
        XmNtopAttachment   , XmATTACH_WIDGET,
        XmNtopWidget       , separator,
        XmNtopOffset       , 10,
        XmNleftAttachment  , XmATTACH_FORM,
        XmNleftOffset      , 10,
	NULL);

    this->apply = XtVaCreateManagedWidget(
        "applyButton", xmPushButtonWidgetClass, dialog,
        XmNtopAttachment   , XmATTACH_WIDGET,
        XmNtopWidget       , separator,
        XmNtopOffset       , 10,
        XmNleftAttachment  , XmATTACH_WIDGET,
        XmNleftOffset      , 10,
        XmNleftWidget      , this->ok,
	NULL);

    this->cancel = XtVaCreateManagedWidget(
        "cancelButton", xmPushButtonWidgetClass, dialog,
        XmNtopAttachment   , XmATTACH_WIDGET,
        XmNtopWidget       , separator,
        XmNtopOffset       , 10,
        XmNrightAttachment , XmATTACH_FORM,
        XmNrightOffset     , 10,
	NULL);

    XtAddCallback (this->apply, XmNactivateCallback, (XtCallbackProc)
	SetPageNameDialog_ApplyCB, (XtPointer)this);
    XtAddCallback (this->page_name_widget, XmNmodifyVerifyCallback, (XtCallbackProc)
	SetPageNameDialog_ModifyNameCB, (XtPointer)this);


    return dialog;
}

//
// Must call this prior to manage()
//
void SetPageNameDialog::setWorkSpace(EditorWorkSpace* ews, 
    const char* page_name, int pos, int count)
{
    this->workspace = ews;
    if (this->page_name) delete this->page_name;
    this->page_name = DuplicateString(page_name);
    this->position = pos;
    this->max = count;
    this->update();
}

void SetPageNameDialog::update()
{
    if (this->stop_updates) return ;
    if ((this->getRootWidget()) && (this->isManaged())) {
	//
	// Initialize the name
	//
	Boolean sens = (this->workspace != NUL(EditorWorkSpace*));
	XtSetSensitive (this->getRootWidget(), sens);
	XmTextSetString (this->page_name_widget, (char*)
	    (this->page_name?this->page_name:""));

	//
	// Initialize the position
	//
	XtVaSetValues (this->position_number,
	    XmNiMinimum, 1,
	    XmNiMaximum, max,
	    XmNiValue, this->position,
	NULL);
	XtSetSensitive (this->position_number, (1!=max));

	//
	// Initialize the postscript toggle
	//
	if (this->workspace) 
	    this->incl = this->workspace->getPostscriptInclusion();
	else 
	    this->incl = FALSE;
	XmToggleButtonSetState (this->ps_toggle, this->incl, False);

#if defined(AUTO_REPOSITION)
	//
	// Initialize the auto_scroll toggle
	//
	if (this->workspace)
	    this->auto_scroll = this->workspace->getUsesRecordedPositions();
	else
	    this->auto_scroll = FALSE;
	XmToggleButtonSetState (this->auto_scroll_toggle, this->auto_scroll, False);
#endif
    }
}

void SetPageNameDialog::manage()
{
    //
    // Manage first, then update so that update can check managed state to avoid
    // unnecessary work.
    //
    this->Dialog::manage();
    this->update();
}


boolean SetPageNameDialog::okCallback(Dialog *)
{
    return this->applyCallback(this);
}

boolean SetPageNameDialog::applyCallback(Dialog *)
{
    this->stop_updates = TRUE;
    //
    // Set the new name
    //
    boolean retVal = TRUE;
    char *name = PageSelector::GetTextWidgetToken(this->page_name_widget);
    if (name) {
	if (!EqualString(name, this->page_name)) {
	    char errMsg[256];
	    boolean retVal = this->selector->verifyPageName(name, errMsg);
	    if (!retVal) {
		XmTextSetString (this->page_name_widget, (char*)
		    (this->page_name?this->page_name:""));
		ErrorMessage (errMsg);
		this->stop_updates = FALSE;
		this->update();
		return FALSE;
	    }
	    this->selector->changePageName (this->workspace, name);
	}
	if (this->page_name) delete this->page_name;
	this->page_name = name;
    }

    //
    // Set the new position
    //
    int newpos;
    XtVaGetValues (this->position_number, XmNiValue, &newpos, NULL);
    if (newpos != this->position) {
	PageTab* pt = (PageTab*)this->selector->page_buttons->getElement(newpos);
	this->selector->changeOrdering (pt, this->page_name, FALSE);
    }

    //
    // Set the new postscript value.
    //
    boolean incl = XmToggleButtonGetState (this->ps_toggle);
    if (incl != this->incl) {
	this->workspace->setPostscriptInclusion(incl);
    }

#if defined(AUTO_REPOSITION)
    //
    // Set the auto-scroll feature.
    //
    boolean auto_scroll = XmToggleButtonGetState (this->auto_scroll_toggle);
    if (this->auto_scroll != auto_scroll) {
	this->workspace->useRecordedPositions (auto_scroll);
    }
#endif

    this->stop_updates = FALSE;
    this->update();
    return retVal;
}


extern "C" {
void
SetPageNameDialog_ApplyCB (Widget, XtPointer clientData, XtPointer)
{
    SetPageNameDialog* spn = (SetPageNameDialog*)clientData;
    ASSERT(spn);
    spn->applyCallback(spn);
}

//
// FIXME: this is a copy of code from PageTab.C.  The PageTab should make
// a public static routine for performing this verification.
//
void SetPageNameDialog_ModifyNameCB (Widget , XtPointer clientData, XtPointer cbs)
{
    SetPageNameDialog* psel = (SetPageNameDialog*)clientData;
    ASSERT(psel);
    XmTextVerifyCallbackStruct *tvcs = (XmTextVerifyCallbackStruct*)cbs;
    boolean good_chars = TRUE;
    XmTextBlock tbrec = tvcs->text;
    int i;
    for (i=0; i<tbrec->length; i++) {
	if ((tbrec->ptr[i] != '_') && 
	    (tbrec->ptr[i] != ' ') &&
	    (!isalnum((int)tbrec->ptr[i]))) {
	    good_chars = FALSE;
	    break;
	} else if ((i == 0) && (tvcs->startPos == 0) && (tbrec->ptr[i] == ' ')) {
	    good_chars = FALSE;
	    break;
	}
    }
    tvcs->doit = (Boolean)good_chars;
}

}
