/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#include <Xm/Xm.h>
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/PushB.h>
#include <Xm/Text.h>
#include <Xm/SeparatoG.h>
#include <Xm/ScrollBar.h>
#include <Xm/ScrolledW.h>
#include <Xm/Label.h>

#include "Application.h"
#include "TextEditDialog.h"
#include "ButtonInterface.h"


//
// These resources are NOT installed for THIS class because this is an
// abstract class.  It is up to the subclasses to install these resources.
//
String TextEditDialog::DefaultResources[] =
{
	".resizePolicy:			XmRESIZE_NONE",
	"*closeButton.labelString:	Close",
	"*okButton.labelString:		OK",
	"*cancelButton.labelString:	Cancel",
	"*applyButton.labelString:	Apply",
	"*restoreButton.labelString:	Restore",
	"*editorText.editMode:   	XmMULTI_LINE_EDIT",
	"*editorText.rows:		12",
	"*editorText.columns:		65",
	"*editorText.wordWrap:		True",
	"*editorText.scrollHorizontal:	False",
	"*editorText.shadowThickness:	1",
	"*closeButton.width: 		60",
	"*closeButton.topOffset: 	10",
	"*okButton.width: 		60",
	"*okButton.topOffset: 		10",
	"*cancelButton.width: 		60",
	"*cancelButton.topOffset: 	10",
	"*restoreButton.width: 		60",
	"*restoreButton.topOffset: 	10",
	"*applyButton.width: 		60",
	"*applyButton.topOffset: 	10",

// If the user defines osfActivate (normally to be the return key) in the 
// virtual bindings (usually in ~/.motifbind), then the text widget doesn't handle
// the return key properly.  The return key triggers the widget's activate()
// method rather than the self-insert() or insert-string() methods.
// (Caution: If the translation is split into 2 lines, it breaks.)
// - Martin
 "*editorText.translations: #override None<Key>osfActivate: insert-string(\"\\n\")",

        NULL
};

TextEditDialog::TextEditDialog(const char *name, 
			Widget parent, boolean readonly, boolean wizard) 
                       		: Dialog(name, parent)
{
   this->editorText = NULL;
   this->dialogModality = XmDIALOG_MODELESS;
   this->readOnly = readonly; 
   this->wizard = wizard;
}

TextEditDialog::~TextEditDialog()
{
}

void TextEditDialog::installDefaultResources(Widget  baseWidget)
{
    this->setDefaultResources(baseWidget, TextEditDialog::DefaultResources);
    this->Dialog::installDefaultResources( baseWidget);
}

Widget TextEditDialog::createDialog(Widget parent)
{
    Widget separator, label, form;
    Arg wargs[6];

    this->initialize();

    int n = 0;
    XtSetArg(wargs[n], XmNdialogStyle, this->dialogModality); n++;
    XtSetArg(wargs[n], XmNautoUnmanage, False); n++;

    form = this->CreateMainForm(parent, this->name, wargs, n);

    label = XtVaCreateManagedWidget("nameLabel",xmLabelWidgetClass, form,
		    XmNtopAttachment,     XmATTACH_FORM,
		    XmNleftAttachment,    XmATTACH_FORM,
		    XmNalignment,	  XmALIGNMENT_BEGINNING,
		    XmNtopOffset,	  10,
		    XmNleftOffset,	  10,
		    NULL);

    if (this->readOnly) {
	this->ok = XtVaCreateManagedWidget("closeButton",
		    xmPushButtonWidgetClass, form,
		    XmNleftAttachment,    XmATTACH_FORM,
		    XmNbottomAttachment,  XmATTACH_FORM,
		    XmNleftOffset,        5,
		    XmNbottomOffset,      10,
		    NULL);
    } else {
	this->ok = XtVaCreateManagedWidget("okButton",
		    xmPushButtonWidgetClass, form,
		    XmNleftAttachment,    XmATTACH_FORM,
		    XmNbottomAttachment,  XmATTACH_FORM,
		    XmNleftOffset,        5,
		    XmNbottomOffset,      10,
		    NULL);

	// the Ok call back is installed during post() in the super class.

	Widget apply = XtVaCreateManagedWidget("applyButton",
		    xmPushButtonWidgetClass, form,
		    XmNleftAttachment,    XmATTACH_WIDGET,
		    XmNleftWidget,        this->ok,
		    XmNbottomAttachment,  XmATTACH_FORM,
		    XmNleftOffset,        10,
		    XmNbottomOffset,      10,
		    NULL);

	XtAddCallback(apply,XmNactivateCallback,
			(XtCallbackProc)TextEditDialog_ApplyCB, (XtPointer)this);

	this->cancel = XtVaCreateManagedWidget("cancelButton",
		    xmPushButtonWidgetClass, form,
		    XmNrightAttachment,   XmATTACH_FORM,
		    XmNbottomAttachment,  XmATTACH_FORM,
		    XmNrightOffset,       5,
		    XmNbottomOffset,      10,
		    NULL);

	// the Cancel call back is installed during post() in the super class.

	Widget restore = XtVaCreateManagedWidget("restoreButton",
		    xmPushButtonWidgetClass, form,
		    XmNrightAttachment,   XmATTACH_WIDGET,
		    XmNrightWidget,       this->cancel,
		    XmNbottomAttachment,  XmATTACH_FORM,
		    XmNrightOffset,       10,
		    XmNbottomOffset,      10,
		    NULL);
	XtAddCallback(restore,XmNactivateCallback,
				(XtCallbackProc)TextEditDialog_RestoreCB, (XtPointer)this);

    } 

    separator = XtVaCreateManagedWidget("",xmSeparatorGadgetClass, form,
		    XmNbottomAttachment,  XmATTACH_FORM,
		    XmNleftAttachment,    XmATTACH_FORM,
		    XmNbottomAttachment,  XmATTACH_WIDGET,
		    XmNbottomWidget,      this->ok,
		    XmNrightAttachment,   XmATTACH_FORM,
		    XmNleftOffset,        2,
		    XmNrightOffset,       2,
		    XmNbottomOffset,   	  10,
		    NULL);

#ifndef AS_OLD_UI

    this->editorText = XmCreateScrolledText(form,"editorText",NULL,0);
    XtVaSetValues(XtParent(this->editorText),
		    XmNtopAttachment,   XmATTACH_WIDGET,
		    XmNtopWidget,       label,
		    XmNbottomAttachment,XmATTACH_WIDGET,
		    XmNbottomWidget,    separator,
		    XmNleftAttachment,  XmATTACH_FORM,
		    XmNrightAttachment, XmATTACH_FORM,
		    XmNshadowType,  	XmSHADOW_ETCHED_IN,
		    XmNshadowThickness, 0,
		    XmNmarginWidth,   	3,
		    XmNmarginHeight,  	3,
		    XmNleftOffset,    	5,
		    XmNrightOffset,   	5,
		    XmNtopOffset,     	5,
		    XmNbottomOffset,  	10,
		    NULL);

    XtVaSetValues(this->editorText, 
		XmNeditable, (this->readOnly ? False : True), NULL);
    XtManageChild(this->editorText);

#else
    
    Widget frame = XtVaCreateManagedWidget("frame",xmFrameWidgetClass, form,
        XmNshadowThickness,   2,
	XmNtopAttachment,     XmATTACH_WIDGET,
	XmNtopWidget,         label,
        XmNleftAttachment,    XmATTACH_FORM,
        XmNrightAttachment,   XmATTACH_FORM,
        XmNbottomAttachment,  XmATTACH_WIDGET,
        XmNbottomWidget,      separator,
        XmNshadowType,        XmSHADOW_ETCHED_IN,
        XmNshadowThickness,   0,
        XmNleftOffset,    5,
        XmNrightOffset,   5,
        XmNtopOffset,     10,
        XmNbottomOffset,  10,
	XmNmarginWidth,   3,
	XmNmarginHeight,  3,
        NULL);

    Widget sw = XtVaCreateManagedWidget("scrolledWindow",
                                xmScrolledWindowWidgetClass,
                                frame,
				XmNscrollHorizontal,False,
                                XmNscrollingPolicy, XmAPPLICATION_DEFINED,
                                XmNvisualPolicy,    XmVARIABLE,
				XmNscrollBarDisplayPolicy, XmAUTOMATIC,
                                XmNshadowThickness, 0,
                                NULL);

    this->editorText = XtVaCreateManagedWidget("editorText",
                                  xmTextWidgetClass,
                                  sw,
				  XmNeditMode, XmMULTI_LINE_EDIT,
                                  NULL);

    XtVaSetValues(sw, XmNworkWindow, this->editorText, NULL);

#endif

    //
    // Turn off the scroll bar highlighting.
    //
    Widget vsb, hsb;
    Pixel  backcolor;

    XtVaGetValues(form, XmNbackground, &backcolor, NULL);

#ifdef AS_OLD_UI

    XtVaGetValues(sw,
		  XmNverticalScrollBar,&vsb,
		  XmNhorizontalScrollBar,&hsb,
		  NULL);

#else

    XtVaGetValues(XtParent(this->editorText),
		  XmNverticalScrollBar,&vsb,
		  XmNhorizontalScrollBar,&hsb,
		  NULL);

#endif

    if (vsb)
        XtVaSetValues(vsb, 
		      XmNhighlightThickness, 0,
		      XmNforeground,	     backcolor, 
		      NULL);
    if (hsb)
	XtVaSetValues(hsb, XmNhighlightThickness, 0, NULL);

    return form;
}




extern "C" void TextEditDialog_ApplyCB(Widget    widget,
                                XtPointer clientData,
                                XtPointer callData)
{
    ASSERT(clientData);
    TextEditDialog *data = (TextEditDialog*) clientData;

    data->applyCallback(widget, callData);

}
void TextEditDialog::applyCallback(Widget    widget, XtPointer callData)
{
    this->saveEditorText();
}


extern "C" void TextEditDialog_RestoreCB(Widget    widget,
                                XtPointer clientData,
                                XtPointer callData)
{
    ASSERT(clientData);
    TextEditDialog *data = (TextEditDialog*) clientData;

    data->restoreCallback(widget, callData);
}
void TextEditDialog::restoreCallback(Widget    widget, XtPointer callData)
{
    this->installEditorText();
}
void TextEditDialog::installEditorText(const char *text)
{
    ASSERT(this->editorText);
    const char *s = text; 
    if (!s)  {
        s = this->getText();
        if (!s)
	    s = "";
    }
    XmTextSetString(this->editorText, (char*)s);
}
boolean TextEditDialog::saveEditorText()
{
boolean return_value;

    ASSERT(this->editorText);
    char *s = XmTextGetString(this->editorText);
    return_value = this->saveText(s);
    XtFree(s); 
    return return_value;
}

boolean TextEditDialog::okCallback(Dialog *d)
{
    if (!this->readOnly)
        return this->saveEditorText();
    return TRUE;
}
void TextEditDialog::manage()
{
    //
    // Don't display the wizard on top of the parent.
    //
    Dimension dialogWidth;
    if (!XtIsRealized(this->getRootWidget()))
	XtRealizeWidget(this->getRootWidget());

    XtVaGetValues(this->getRootWidget(), XmNwidth, &dialogWidth, NULL);

    Position x;
    Position y;
    Dimension width;
    XtVaGetValues(parent, XmNx, &x, XmNy, &y, XmNwidth, &width, NULL);

    if (x > dialogWidth + 25) x -= dialogWidth + 25;
    else x += width + 25;

    Display *dpy = XtDisplay(this->getRootWidget());
    if (x > WidthOfScreen(DefaultScreenOfDisplay(dpy)) - dialogWidth)
	x = WidthOfScreen(DefaultScreenOfDisplay(dpy)) - dialogWidth;
    XtVaSetValues(this->getRootWidget(), XmNdefaultPosition, False, NULL);
    XtVaSetValues(XtParent(this->getRootWidget()), XmNx, x, XmNy, y, NULL);

    this->Dialog::manage();
    this->updateDialogTitle();
    this->installEditorText();

    XmProcessTraversal(this->editorText, XmTRAVERSE_CURRENT);
    XmProcessTraversal(this->editorText, XmTRAVERSE_CURRENT);
}

//
// The title to be applied the newly managed dialog.
// The returned string is freed by the caller (TextEditDialog::manage()).
//
char *TextEditDialog::getDialogTitle()
{
   return NULL;
}
//
// Ask for the presumably new title from the derived class (usually) and 
// install it. 
//
void TextEditDialog::updateDialogTitle()
{
   char *title = this->getDialogTitle();
   if (title) {
	this->setDialogTitle(title);
	delete title;
   }
}
boolean TextEditDialog::saveText(const char *s) { return TRUE; }
const char *TextEditDialog::getText() { return NULL; }

