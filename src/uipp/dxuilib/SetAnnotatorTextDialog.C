/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#include "DXStrings.h"
#include "Application.h"
#include "SetAnnotatorTextDialog.h"
#include "VPEAnnotator.h"
#include "Dictionary.h"
#include "DecoratorStyle.h"
#include "SymbolManager.h"
#include "EditorWindow.h"
#include "Network.h"
#include <Xm/RowColumn.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/Form.h>
#include <Xm/SeparatoG.h>
#include <Xm/Text.h>

boolean SetAnnotatorTextDialog::ClassInitialized = FALSE;

#define HIDE_TEXT "Hide Text"
#define SHOW_TEXT "Show Text"

String SetAnnotatorTextDialog::DefaultResources[] =
{
    "*dialogTitle:              Visual program comment...\n",
    "*hideShow.width:		80",
    "*hideShow.recomputeSize:	False",
    NUL(char*)
};


SetAnnotatorTextDialog::SetAnnotatorTextDialog
    (Widget parent, boolean , LabelDecorator *dec) : 
    SetDecoratorTextDialog(parent, FALSE, dec, "AnnotatorAttributes")
{
    Dictionary *dict = DecoratorStyle::GetDecoratorStyleDictionary("Annotate");
    ASSERT(dict);

    this->hide_style = (DecoratorStyle*)dict->findDefinition("Marker");
    this->show_style = (DecoratorStyle*)dict->findDefinition("Label");
    ASSERT(this->hide_style);
    ASSERT(this->show_style);

    if (!SetAnnotatorTextDialog::ClassInitialized) {
	SetAnnotatorTextDialog::ClassInitialized = TRUE;
        this->installDefaultResources(theApplication->getRootWidget());
    }
}

SetAnnotatorTextDialog::~SetAnnotatorTextDialog()
{
}

Widget
SetAnnotatorTextDialog::createDialog(Widget parent)
{
    Widget dialog = this->SetDecoratorTextDialog::createDialog(parent);

    XmString xmstr = NUL(XmString);
    ASSERT(this->decorator);
    DecoratorStyle *ds = NUL(DecoratorStyle*);
    if (!EqualString(this->decorator->getClassName(), ClassVPEAnnotator)) {
	xmstr = XmStringCreate (SHOW_TEXT, "bold");
	ds = this->show_style;
    } else {
	xmstr = XmStringCreate (HIDE_TEXT, "bold");
	ds = this->hide_style;
    }

    int n = 0;
    Arg args[9];
    XtSetArg (args[n], XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
    XtSetArg (args[n], XmNtopWidget, this->ok); n++;
    XtSetArg (args[n], XmNtopOffset, 0); n++;
    XtSetArg (args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
    XtSetArg (args[n], XmNleftWidget, this->apply); n++;
    XtSetArg (args[n], XmNleftOffset, 10); n++;
    XtSetArg (args[n], XmNlabelString, xmstr); n++;
    XtSetArg (args[n], XmNuserData, ds); n++;
    this->hide_show = XmCreatePushButton (XtParent(this->ok), "hideShow", args, n);
    XtManageChild (this->hide_show);
    XtAddCallback (this->hide_show, XmNactivateCallback, (XtCallbackProc)
	SetAnnotatorTextDialog_HideShowCB, (XtPointer)this);

    XmStringFree(xmstr);

    return dialog;
}

void SetAnnotatorTextDialog::hideCallback()
{
    this->okCallback(this);

    DecoratorStyle* ds = NUL(DecoratorStyle*);
    XtVaGetValues (this->hide_show, XmNuserData, &ds, NULL);
    ASSERT(ds);
    EditorWindow* ew = this->decorator->getNetwork()->getEditor();
    ASSERT(ew);
    List dl;
    dl.appendElement((void*)this->decorator);
    ew->setDecoratorStyle (&dl, ds);
}

extern "C" {

void
SetAnnotatorTextDialog_HideShowCB (Widget , XtPointer cdata, XtPointer)
{
SetAnnotatorTextDialog *sdt = (SetAnnotatorTextDialog *)cdata;

    ASSERT(sdt);
    sdt->hideCallback();
}

} // extern C


//
// Install the default resources for this class.
//
void SetAnnotatorTextDialog::installDefaultResources(Widget  baseWidget)
{
    this->setDefaultResources(baseWidget, SetAnnotatorTextDialog::DefaultResources);
    this->SetDecoratorTextDialog::installDefaultResources( baseWidget);
}


//
// Connect the dialog to a new decorator.  Update the displayed values.
//
void SetAnnotatorTextDialog::setDecorator(LabelDecorator* new_lab)
{
    this->SetDecoratorTextDialog::setDecorator(new_lab);
    XtSetSensitive (this->hide_show, (this->decorator != NUL(LabelDecorator*)));

    if (this->decorator) {
	DecoratorStyle* ds = NUL(DecoratorStyle*);
	Symbol vpea_sym = theSymbolManager->registerSymbol (ClassVPEAnnotator);
	ASSERT (this->decorator->isA(vpea_sym));
	XmString xmstr = NUL(XmString);
	if (!EqualString(this->decorator->getClassName(), ClassVPEAnnotator)) {
	    xmstr = XmStringCreate (SHOW_TEXT, "bold");
	    ds = this->show_style;
	} else {
	    xmstr = XmStringCreate (HIDE_TEXT, "bold");
	    ds = this->hide_style;
	}
	if (xmstr) {
	    XtVaSetValues (this->hide_show, 
		XmNlabelString, xmstr, 
		XmNuserData, ds,
	    NULL);
	    XmStringFree(xmstr);
	}
    }
}

void SetAnnotatorTextDialog::manage()
{
    if (!this->isManaged()) {
	Dimension dialogWidth;
	if (!XtIsRealized(this->getRootWidget()))
	    XtRealizeWidget(this->getRootWidget());

	XtVaGetValues(this->getRootWidget(), XmNwidth, &dialogWidth, NULL);

	Position x;
	Position y;
	Dimension width;
	Widget tlshell = this->parent;
	while ((tlshell) && (XtIsShell(tlshell) == False))
	    tlshell = XtParent(tlshell);
	if (tlshell) { 
	    XtVaGetValues(tlshell, XmNx, &x, XmNy, &y, XmNwidth, &width, NULL);

	    if (x > dialogWidth + 25) x -= dialogWidth + 25;
	    else x += width + 25;

	    Display *dpy = XtDisplay(this->getRootWidget());
	    if (x > WidthOfScreen(DefaultScreenOfDisplay(dpy)) - dialogWidth)
		x = WidthOfScreen(DefaultScreenOfDisplay(dpy)) - dialogWidth;
	    XtVaSetValues(this->getRootWidget(), XmNdefaultPosition, False, NULL);
	    XtVaSetValues(XtParent(this->getRootWidget()), XmNx, x, XmNy, y, NULL);
	}
    }

    this->SetDecoratorTextDialog::manage();
}

const char* 
SetAnnotatorTextDialog::defaultJustifySetting()
{
static char* def = "XmALIGNMENT_BEGINNING";
    return def;
}

