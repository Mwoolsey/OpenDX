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
#include "SetDecoratorTextDialog.h"
#include "LabelDecorator.h"
#include "Network.h"
#include <Xm/RowColumn.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/Form.h>
#include <Xm/SeparatoG.h>
#include <Xm/Text.h>
#include <Xm/DrawingA.h>
#include <Xm/DialogS.h>
#include <Xm/ArrowB.h>
#include <Xm/Label.h>

#include <ctype.h> // for ispunct

#define ORIG_MINWIDTH 620
#define ARROW_RIGHTMOST_POS 60

boolean SetDecoratorTextDialog::ClassInitialized = FALSE;

String SetDecoratorTextDialog::DefaultResources[] =
{
        "*dialogTitle:     		Control Panel comment...\n",
	"*justifyOM.labelString:	Justification:",
	"*justifyOM.sensitive:		False",
	"*colorOM.labelString:		Text Color:",
	"*fontOM.labelString:		Font:",
	"*editorText.columns:		40",
	"*XmPushButtonGadget.traversalOn: False",
	"*XmPushButton.traversalOn: 	False",
	"*arrowLabel.marginWidth:	0",
	"*arrowLabel.marginHeight:	0",
	"*arrowLabel.marginTop:		0",
	"*arrowLabel.marginLeft:	0",
	"*arrowLabel.marginRight:	0",
	"*arrowLabel.marginBottom:	0",

	"*okButton.labelString:		OK",
	"*applyButton.labelString:	Apply",
	"*restoreButton.labelString:	Restore",
	"*reformatButton.labelString:	Reformat",
	"*cancelButton.labelString:	Cancel",
	"*okButton.width:		80",
	"*applyButton.width:		80",
	"*restoreButton.width:		80",
	"*cancelButton.width:		80",
	"*reformatButton.width:		80",

// If the user defines osfActivate (normally to be the return key) in the
// virtual bindings (usually in ~/.motifbind), then the text widget doesn't handle
// the return key properly.  The return key triggers the widget's activate()
// method rather than the self-insert() or insert-string() methods.
// (Caution: If the translation is split into 2 lines, it breaks.)
// - Martin
 "*editorText.translations: #override None<Key>osfActivate: insert-string(\"\\n\")",

        NULL
};


#define DIRTY_COLORS 1
#define DIRTY_JUSTIFY 2
#define DIRTY_TEXT 4
#define DIRTY_FONT 8

SetDecoratorTextDialog::SetDecoratorTextDialog(Widget parent, boolean ,
				LabelDecorator *dec, const char* name) : 
				Dialog(name, parent)
{
    this->decorator = dec;
    this->dirty = DIRTY_COLORS | DIRTY_JUSTIFY | DIRTY_TEXT | DIRTY_FONT;
}

SetDecoratorTextDialog::SetDecoratorTextDialog(Widget parent,
				boolean , LabelDecorator *dec) : 
				Dialog("LabelAttributes",parent)
{
    this->decorator = dec;
    this->dirty = DIRTY_COLORS | DIRTY_JUSTIFY | DIRTY_TEXT | DIRTY_FONT;

    if (!SetDecoratorTextDialog::ClassInitialized) {
	SetDecoratorTextDialog::ClassInitialized = TRUE;
        this->installDefaultResources(theApplication->getRootWidget());
    }
}

SetDecoratorTextDialog::~SetDecoratorTextDialog()
{
}

//
// Install the proper values whenever the dialog box is put onto the screen.
//
void
SetDecoratorTextDialog::post()
{
    this->Dialog::post();
    this->restoreCallback (this);
}

//
//
boolean
SetDecoratorTextDialog::restoreCallback (Dialog * )
{
    //
    // text
    //
    if (this->dirty & DIRTY_TEXT) {
	const char* cp = "";
	if (this->decorator) cp = this->decorator->getLabelValue();
	//XtRemoveCallback (this->editorText, XmNmodifyVerifyCallback, (XtCallbackProc)
	//    SetDecoratorTextDialog_DirtyTextCB, (XtPointer)this);
	XmTextSetString (this->editorText, (char*)cp);
	//XtAddCallback (this->editorText, XmNmodifyVerifyCallback, (XtCallbackProc)
	//    SetDecoratorTextDialog_DirtyTextCB, (XtPointer)this);
	this->dirty&= ~DIRTY_TEXT;
    }

    //
    // alignment
    //
    if (this->dirty & DIRTY_JUSTIFY) {
	int i,nkids;
	Widget *kids;
	const char *align;
	char *cp;
	boolean isset = FALSE;
	if (this->decorator) isset = this->decorator->isResourceSet (XmNalignment);
	if (isset) {
	    align = this->decorator->getResourceString(XmNalignment);
	} else {
	    align = NULL;
	}
	XtVaGetValues (this->justifyPulldown, 
	    XmNnumChildren, &nkids, XmNchildren, &kids, NULL);
	ASSERT (nkids > 0);
	if ((align) && (align[0])) {
	    for (i=0; i<nkids; i++) {
		XtVaGetValues (kids[i], XmNuserData, &cp, NULL);
		if (!strcmp(cp, align)) break;
	    }
	    if (i==nkids) i = nkids - 1;
	} else {
	    i = nkids - 1;
	}

	Widget choice = kids[i];
	XtVaSetValues (this->justifyOM, XmNmenuHistory, kids[i], NULL);
	this->dirty&= ~DIRTY_JUSTIFY;

	//
	// If justification is set to left, then enable the reformat button and
	// disable wordwrap.
	//
	ASSERT(choice);
	XtVaGetValues (choice, XmNuserData, &align, NULL);
	boolean enab =  (strcmp (align, "XmALIGNMENT_BEGINNING") == 0);
	XtSetSensitive (this->reformat, enab);
    }

    //
    // color
    // The default will be kept in kids[nkids-1]
    //
    if (this->dirty & DIRTY_COLORS) {
	int i,nkids;
	Widget *kids;
	char *cp;
	const char *color;
	boolean isset = FALSE;
	if (this->decorator) isset = this->decorator->isResourceSet (XmNforeground);
	if (isset) {
	    color = this->decorator->getResourceString(XmNforeground);
	} else {
	    color = NULL;
	}
	XtVaGetValues (this->colorPulldown, 
	    XmNnumChildren, &nkids, XmNchildren, &kids, NULL);
	ASSERT (nkids > 0);
	if ((color) && (color[0])) {
	    for (i=0; i<nkids; i++) {
		XtVaGetValues (kids[i], XmNuserData, &cp, NULL);
		if (!strcmp(cp, color)) break;
	    }
	    if (i==nkids) i = nkids - 1;
	} else {
	    i = nkids - 1;
	}

	XtVaSetValues (this->colorOM, XmNmenuHistory, kids[i], NULL);
	this->dirty&= ~DIRTY_COLORS;
    }


    //
    // font
    // The default will be kept in kids[nkids-1]
    //
    if (this->dirty & DIRTY_FONT) {
	int i,nkids;
	Widget *kids;
	char *cp;
	const char *font_name = NUL(char*);
	if (this->decorator) font_name = this->decorator->getFont();
	XtVaGetValues (this->fontPulldown, 
	    XmNnumChildren, &nkids, XmNchildren, &kids, NULL);
	ASSERT (nkids > 0);
	if ((font_name) && (font_name[0])) {
	    for (i=0; i<nkids; i++) {
		XtVaGetValues (kids[i], XmNuserData, &cp, NULL);
		if (!strcmp(cp, font_name)) break;
	    }
	    if (i==nkids) i = nkids - 1;
	} else {
	    i = nkids - 1;
	}

	XtVaSetValues (this->fontOM, XmNmenuHistory, kids[i], NULL);
	this->dirty&= ~DIRTY_FONT;
    }

    return TRUE;
}

boolean
SetDecoratorTextDialog::okCallback (Dialog* )
{

    if (this->dirty) {
	this->decorator->getNetwork()->setFileDirty();
    }

    if (this->dirty & DIRTY_TEXT) {
	char *s = XmTextGetString (this->editorText);
	this->decorator->setLabel(s);
	XtFree(s);
	this->dirty&= ~DIRTY_TEXT;
	this->decorator->postTextGrowthWork();
    }

    Widget choice;
    if (this->dirty & DIRTY_JUSTIFY) {
	char *align;
	XtVaGetValues (this->justifyOM, XmNmenuHistory, &choice, NULL);
	ASSERT(choice);
	XtVaGetValues (choice, XmNuserData, &align, NULL);
	this->decorator->setResource (XmNalignment, align);

	if (!strcmp(XtName(choice), DEFAULT_SETTING))
	    this->decorator->setResource(XmNalignment, NUL(const char *));

	this->dirty&= ~DIRTY_JUSTIFY;
    }

    if (this->dirty & DIRTY_COLORS) {
	char *color;
	XtVaGetValues (this->colorOM, XmNmenuHistory, &choice, NULL);
	ASSERT(choice);
	XtVaGetValues (choice, XmNuserData, &color, NULL);
	this->decorator->setResource (XmNforeground, color);

	if (!strcmp(XtName(choice), DEFAULT_SETTING)) 
	    this->decorator->setResource (XmNforeground, NUL(const char *));

	this->dirty&= ~DIRTY_COLORS;
    }


    if (this->dirty & DIRTY_FONT) {
	char *font_name;
	XtVaGetValues (this->fontOM, XmNmenuHistory, &choice, NULL);
	ASSERT(choice);
	XtVaGetValues (choice, XmNuserData, &font_name, NULL);
	if (!strcmp(XtName(choice), DEFAULT_SETTING)) {
	    this->decorator->setFont (NUL(const char *));
	} else {
	    this->decorator->setFont (font_name);
	}
	this->dirty&= ~DIRTY_FONT;
    }

    return TRUE;
}

Widget
SetDecoratorTextDialog::createDialog(Widget parent)
{
int n = 0;
Widget form;
Arg args[20];

    n = 0;
    XtSetArg (args[n], XmNautoUnmanage, False); n++;
    XtSetArg (args[n], XmNwidth, ORIG_MINWIDTH); n++;
    XtSetArg (args[n], XmNminWidth, /*290*/ORIG_MINWIDTH); n++;
    form = this->CreateMainForm (parent, this->name, args, n);
    this->negotiated = FALSE;

    n = 0;
    XtSetArg (args[n], XmNtopAttachment, XmATTACH_FORM); n++;
    XtSetArg (args[n], XmNleftAttachment, XmATTACH_FORM); n++;
    XtSetArg (args[n], XmNrightAttachment, XmATTACH_FORM); n++;
    XtSetArg (args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
    XtSetArg (args[n], XmNrows,	6); n++;
    XtSetArg (args[n], XmNcolumns, 40); n++;
    XtSetArg (args[n], XmNbottomOffset,	130); n++;
    XtSetArg (args[n], XmNtopOffset, 10); n++;
    XtSetArg (args[n], XmNleftOffset, 6); n++;
    XtSetArg (args[n], XmNrightOffset, ARROW_RIGHTMOST_POS); n++;
    XtSetArg (args[n], XmNeditMode, XmMULTI_LINE_EDIT); n++;
    XtSetArg (args[n], XmNwordWrap, False); n++;
    XtSetArg (args[n], XmNscrollHorizontal, False); n++;
    XtSetArg (args[n], XmNmappedWhenManaged, False); n++;
    this->editor_magic = XmCreateScrolledText (form, "editorText", args, n);
    XtManageChild (this->editor_magic);

    n = 0;
    XtSetArg (args[n], XmNtopAttachment, XmATTACH_FORM); n++;
    XtSetArg (args[n], XmNleftAttachment, XmATTACH_FORM); n++;
    XtSetArg (args[n], XmNrightAttachment, XmATTACH_FORM); n++;
    XtSetArg (args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
    XtSetArg (args[n], XmNrows,	6); n++;
    XtSetArg (args[n], XmNcolumns, 40); n++;
    XtSetArg (args[n], XmNbottomOffset,	130); n++;
    XtSetArg (args[n], XmNtopOffset, 10); n++;
    XtSetArg (args[n], XmNleftOffset, 6); n++;
    XtSetArg (args[n], XmNrightOffset, 6); n++;
    XtSetArg (args[n], XmNeditMode, XmMULTI_LINE_EDIT); n++;
    XtSetArg (args[n], XmNwordWrap, False); n++;
    XtSetArg (args[n], XmNscrollHorizontal, False); n++;
    this->editorText = XmCreateScrolledText (form, "editorText", args, n);
    XtManageChild (this->editorText);
    XtAddCallback (this->editorText, XmNmodifyVerifyCallback, (XtCallbackProc)
	SetDecoratorTextDialog_DirtyTextCB, (XtPointer)this);


    n = 0;
    XtSetArg (args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
    XtSetArg (args[n], XmNtopWidget, XtParent(this->editorText)); n++;
    XtSetArg (args[n], XmNtopOffset, 30); n++;
    XtSetArg (args[n], XmNleftAttachment, XmATTACH_FORM); n++;
    XtSetArg (args[n], XmNrightAttachment, XmATTACH_FORM); n++;
    Widget sep1 = XmCreateSeparatorGadget (form, "separator", args, n);
    XtManageChild (sep1);

    //
    // Add an arrow button for resizing the text.
    //
    n = 0;
    XtSetArg (args[n], XmNarrowDirection, XmARROW_UP); n++;
    XtSetArg (args[n], XmNshadowThickness, 0); n++;
    XtSetArg (args[n], XmNbottomAttachment, XmATTACH_WIDGET); n++;
    XtSetArg (args[n], XmNbottomWidget, sep1); n++;
    XtSetArg (args[n], XmNbottomOffset, 2); n++;
    XtSetArg (args[n], XmNleftAttachment, XmATTACH_NONE); n++;
    XtSetArg (args[n], XmNrightAttachment, XmATTACH_FORM); n++;
    XtSetArg (args[n], XmNrightOffset, ARROW_RIGHTMOST_POS); n++;
    this->resize_arrow = XmCreateArrowButton (form, "resizeArrow", args, n);
    XtManageChild (this->resize_arrow);
    XtAddEventHandler (this->resize_arrow, Button1MotionMask, False,
	(XtEventHandler)SetDecoratorTextDialog_ArrowMotionEH, (XtPointer)this);
    XtAddCallback (this->resize_arrow, XmNarmCallback, (XtCallbackProc)
	SetDecoratorTextDialog_StartPosCB, (XtPointer)this);
    //XtAddCallback (this->resize_arrow, XmNactivateCallback, (XtCallbackProc)
    //	SetDecoratorTextDialog_EndPosCB, (XtPointer)this);
    XtAddCallback (this->resize_arrow, XmNdisarmCallback, (XtCallbackProc)
	SetDecoratorTextDialog_EndPosCB, (XtPointer)this);

    XmString xmstr = XmStringCreateLtoR ("margin\nposition", "small_bold");
    n = 0;
    XtSetArg (args[n], XmNbottomAttachment, XmATTACH_WIDGET); n++;
    XtSetArg (args[n], XmNbottomWidget, sep1); n++;
    XtSetArg (args[n], XmNbottomOffset, 0); n++;
    XtSetArg (args[n], XmNleftAttachment, XmATTACH_NONE); n++;
    XtSetArg (args[n], XmNrightAttachment, XmATTACH_FORM); n++;
    XtSetArg (args[n], XmNrightOffset, 4); n++;
    XtSetArg (args[n], XmNlabelString, xmstr); n++;
    XtSetArg (args[n], XmNalignment, XmALIGNMENT_END); n++;
    this->resize_arrow_label = XmCreateLabel (form, "arrowLabel", args, n);
    XmStringFree(xmstr);
    XtManageChild (this->resize_arrow_label);


    n = 0;
    XtSetArg (args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
    XtSetArg (args[n], XmNtopWidget, sep1); n++;
    XtSetArg (args[n], XmNtopOffset, 10); n++;
    XtSetArg (args[n], XmNleftAttachment, XmATTACH_FORM); n++;
    XtSetArg (args[n], XmNleftOffset, 6); n++;
    XtSetArg (args[n], XmNsubMenuId, this->createColorMenu(form)); n++;
    this->colorOM = XmCreateOptionMenu (form, "colorOM", args, n);
    XtManageChild (this->colorOM);

    n = 0;
    XtSetArg (args[n], XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
    XtSetArg (args[n], XmNtopWidget, this->colorOM); n++;
    XtSetArg (args[n], XmNtopOffset, 0); n++;
    XtSetArg (args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
    XtSetArg (args[n], XmNleftWidget, this->colorOM); n++;
    XtSetArg (args[n], XmNleftOffset, 10); n++;
    //XtSetArg (args[n], XmNrightAttachment, XmATTACH_WIDGET); n++;
    //XtSetArg (args[n], XmNrightWidget, this->justifyOM); n++;
    //XtSetArg (args[n], XmNrightOffset, 10); n++;
    XtSetArg (args[n], XmNsubMenuId, this->createFontMenu(form)); n++;
    this->fontOM = XmCreateOptionMenu (form, "fontOM", args, n);
    XtManageChild (this->fontOM);

    n = 0;
    XtSetArg (args[n], XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
    XtSetArg (args[n], XmNtopWidget, this->colorOM); n++;
    XtSetArg (args[n], XmNtopOffset, 0); n++;
    XtSetArg (args[n], XmNrightAttachment, XmATTACH_FORM); n++;
    XtSetArg (args[n], XmNrightOffset, 6); n++;
    XtSetArg (args[n], XmNsubMenuId, this->createJustifyMenu(form)); n++;
    this->justifyOM = XmCreateOptionMenu (form, "justifyOM", args, n);
    XtManageChild (this->justifyOM);

    n = 0;
    XtSetArg (args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
    XtSetArg (args[n], XmNtopWidget, colorOM); n++;
    XtSetArg (args[n], XmNtopOffset, 10); n++;
    XtSetArg (args[n], XmNleftAttachment, XmATTACH_FORM); n++;
    XtSetArg (args[n], XmNrightAttachment, XmATTACH_FORM); n++;
    Widget sep2 = XmCreateSeparatorGadget (form, "separator", args, n);
    XtManageChild (sep2);

    n = 0;
    XtSetArg (args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
    XtSetArg (args[n], XmNtopWidget, sep2); n++;
    XtSetArg (args[n], XmNtopOffset, 2); n++;
    XtSetArg (args[n], XmNleftAttachment, XmATTACH_FORM); n++;
    XtSetArg (args[n], XmNleftOffset, 2); n++;
    XtSetArg (args[n], XmNrightAttachment, XmATTACH_FORM); n++;
    XtSetArg (args[n], XmNrightOffset, 2); n++;
    XtSetArg (args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
    XtSetArg (args[n], XmNbottomOffset, 2); n++;
    Widget button_form = XmCreateForm (form, "bForm", args, n);
    XtManageChild (button_form);

    n = 0;
    XtSetArg (args[n], XmNtopAttachment, XmATTACH_FORM); n++;
    XtSetArg (args[n], XmNtopOffset, 12); n++;
    XtSetArg (args[n], XmNleftAttachment, XmATTACH_FORM); n++;
    XtSetArg (args[n], XmNleftOffset, 12); n++;
    this->ok = XmCreatePushButton (button_form, "okButton", args, n);
    XtManageChild (this->ok);

    n = 0;
    XtSetArg (args[n], XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
    XtSetArg (args[n], XmNtopWidget, this->ok); n++;
    XtSetArg (args[n], XmNtopOffset, 0); n++;
    XtSetArg (args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
    XtSetArg (args[n], XmNleftWidget, this->ok); n++;
    XtSetArg (args[n], XmNleftOffset, 10); n++;
    this->apply = XmCreatePushButton (button_form, "applyButton", args, n);
    XtManageChild (this->apply);
    XtAddCallback (this->apply, XmNactivateCallback, (XtCallbackProc)
	SetDecoratorTextDialog_ApplyCB, (XtPointer)this);

    n = 0;
    XtSetArg (args[n], XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
    XtSetArg (args[n], XmNtopWidget, this->ok); n++;
    XtSetArg (args[n], XmNtopOffset, 0); n++;
    XtSetArg (args[n], XmNrightAttachment, XmATTACH_FORM); n++;
    XtSetArg (args[n], XmNrightOffset, 12); n++;
    this->cancel = XmCreatePushButton (button_form, "cancelButton", args, n);
    XtManageChild (this->cancel);

    n = 0;
    XtSetArg (args[n], XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
    XtSetArg (args[n], XmNtopWidget, this->cancel); n++;
    XtSetArg (args[n], XmNtopOffset, 0); n++;
    XtSetArg (args[n], XmNrightAttachment, XmATTACH_WIDGET); n++;
    XtSetArg (args[n], XmNrightWidget, this->cancel); n++;
    XtSetArg (args[n], XmNrightOffset, 10); n++;
    this->restore = XmCreatePushButton (button_form, "restoreButton", args, n);
    XtManageChild (this->restore);
    XtAddCallback (this->restore, XmNactivateCallback, (XtCallbackProc)
	SetDecoratorTextDialog_RestoreCB, (XtPointer)this);

    n = 0;
    XtSetArg (args[n], XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
    XtSetArg (args[n], XmNtopWidget, this->cancel); n++;
    XtSetArg (args[n], XmNtopOffset, 0); n++;
    XtSetArg (args[n], XmNrightAttachment, XmATTACH_WIDGET); n++;
    XtSetArg (args[n], XmNrightWidget, this->restore); n++;
    XtSetArg (args[n], XmNrightOffset, 10); n++;
    this->reformat = XmCreatePushButton (button_form, "reformatButton", args, n);
    XtManageChild (this->reformat);
    XtAddCallback (this->reformat, XmNactivateCallback, (XtCallbackProc)
	SetDecoratorTextDialog_ReformatCB, (XtPointer)this);

    return form;
}

//
// Things inside SHOW_IT_NOW ifdefs will set the corresponding resource value
// into the button so that the user sees it without having to set the value
// into the decorator.  These things are turned off right now because turning
// them on affects widget size which makes the option menus unequal in size.
//
Widget
SetDecoratorTextDialog::createColorMenu(Widget parent)
{
Widget button;
const char **btn_names = this->decorator->getSupportedColorNames();
const char **color_values = this->decorator->getSupportedColorValues();

Arg args[9];
int i,n;

    n = 0;
    this->colorPulldown = XmCreatePulldownMenu (parent, "pulldownMenu", args, n);

#   if SHOW_IT_NOW
    Pixel fg;
    XrmValue from, toinout;
#   endif

    i = 0;
    while (btn_names[i]) {
	char *color_value, *btn_name;
	btn_name = new char[1+strlen(btn_names[i])]; 
	strcpy (btn_name, btn_names[i]);
	color_value = new char[1+strlen(color_values[i])]; 
	strcpy (color_value, color_values[i]);

#	if SHOW_IT_NOW
	from.addr = color_values[i];
	from.size = 1+strlen(from.addr);
        toinout.addr = (XPointer)&fg;
	toinout.size = sizeof(Pixel);
        XtConvertAndStore(parent, XmRString, &from, XmRPixel, &toinout);
#	endif

	XmString xmstr = XmStringCreateLtoR (btn_name, "bold");

	n = 0;
#	if SHOW_IT_NOW
	XtSetArg (args[n], XmNforeground, fg); n++;
#	endif
	XtSetArg (args[n], XmNuserData, color_values[i]); n++;
	XtSetArg (args[n], XmNlabelString, xmstr); n++;
#	if SHOW_IT_NOW
	button = XmCreatePushButton (this->colorPulldown, btn_name, args, n);
#	else
	button = XmCreatePushButtonGadget (this->colorPulldown, btn_name, args, n);
#	endif
	XtManageChild (button);
	XtAddCallback (button, XmNactivateCallback, (XtCallbackProc)
	    SetDecoratorTextDialog_DirtyColorsCB, (XtPointer)this);

	XmStringFree(xmstr);
	delete color_value;
	delete btn_name;

	i++;
    }

    return this->colorPulldown;
}


Widget
SetDecoratorTextDialog::createFontMenu(Widget parent)
{
Widget button;
Arg args[9];
int i,n;
#if defined(aviion) || defined(hp700)
static
#endif
char *btn_names[] = {
    "bold 12 point", 	"bold 14 point", 	"bold 18 point",	"bold 24 point",
    "italic 12 point", 	"italic 14 point", 	"italic 18 point",	"italic 24 point",
    "medium 12 point", 	"medium 14 point", 	"medium 18 point",	"medium 24 point",
    DEFAULT_SETTING
};

// These names must match those used for fontList in IMBApplication.C
#if defined(aviion) || defined(hp700)
static
#endif
char *font_names[] = {
    "small_bold", 	"bold", 	"big_bold",	"huge_bold",
    "small_oblique", 	"oblique", 	"big_oblique",	"huge_oblique",
    "small_normal", 	"normal", 	"big_normal",	"huge_normal",
    XmSTRING_DEFAULT_CHARSET
};

    n = 0;
    this->fontPulldown = XmCreatePulldownMenu (parent, "pulldownMenu", args, n);

    for (i=0; i<XtNumber(btn_names); i++) {
#	if SHOW_IT_NOW
	XmString xmstr = XmStringCreateLtoR (btn_names[i], font_names[i]);
#	endif

	n = 0;
	XtSetArg (args[n], XmNuserData, font_names[i]); n++;
#	if SHOW_IT_NOW
	XtSetArg (args[n], XmNlabelString, xmstr); n++;
#	endif
	button = XmCreatePushButton (this->fontPulldown, btn_names[i], args, n);
	XtManageChild (button);
	XtAddCallback (button, XmNactivateCallback, (XtCallbackProc)
	    SetDecoratorTextDialog_DirtyFontCB, (XtPointer)this);

#	if SHOW_IT_NOW
	XmStringFree(xmstr);
#	endif
    }

    return this->fontPulldown;
}

Widget
SetDecoratorTextDialog::createJustifyMenu (Widget parent)
{
Widget button;
Arg args[9];
int i,n;

#if defined(aviion) || defined(hp700)
static
#endif
char *btn_names[] = {
    "Left", 	"Center", 	"Right",
    DEFAULT_SETTING
};


    static char *align_names[4]; 
    align_names[0] = "XmALIGNMENT_BEGINNING";
    align_names[1] = "XmALIGNMENT_CENTER";
    align_names[2] = "XmALIGNMENT_END";
    align_names[3] = (char*)this->defaultJustifySetting();

    n = 0;
    this->justifyPulldown = XmCreatePulldownMenu (parent, "pulldownMenu", args, n);

    for (i=0; i<XtNumber(btn_names); i++) {
	n = 0;
	XtSetArg (args[n], XmNuserData, align_names[i]); n++;
	button = XmCreatePushButtonGadget (this->justifyPulldown, btn_names[i], args, n);
	XtManageChild (button);
	XtAddCallback (button, XmNactivateCallback, (XtCallbackProc)
	    SetDecoratorTextDialog_DirtyJustifyCB, (XtPointer)this);
    }

    return this->justifyPulldown;
}

const char* 
SetDecoratorTextDialog::defaultJustifySetting()
{
static char* def = "XmALIGNMENT_CENTER";
    return def;
}

extern "C" {

void
SetDecoratorTextDialog_DirtyColorsCB (Widget , XtPointer cdata, XtPointer)
{
SetDecoratorTextDialog *sdt = (SetDecoratorTextDialog *)cdata;

    ASSERT(sdt);
    sdt->dirty|= DIRTY_COLORS;
}


void
SetDecoratorTextDialog_DirtyJustifyCB (Widget , XtPointer cdata, XtPointer)
{
SetDecoratorTextDialog *sdt = (SetDecoratorTextDialog *)cdata;

    ASSERT(sdt);
    sdt->dirty|= DIRTY_JUSTIFY;
//
// ...a workaround for a bug in libXm in aix 3.2.5. If you
// set XmNalignment on anything of the XmLabel variety, the text won't 
// move. But if you set other resources, then the text will move.
//
#if defined(ibm6000)
    sdt->dirty|= DIRTY_TEXT;
#endif


    //
    // If justification is set to left, then enable the reformat button 
    //
    Widget choice;
    char *align;
    XtVaGetValues (sdt->justifyOM, XmNmenuHistory, &choice, NULL);
    ASSERT(choice);
    XtVaGetValues (choice, XmNuserData, &align, NULL);
    boolean enab =  (strcmp (align, "XmALIGNMENT_BEGINNING") == 0);
    XtSetSensitive (sdt->reformat, enab);

    XtVaSetValues (sdt->resize_arrow, XmNmappedWhenManaged, (Boolean)enab, NULL);
    XtVaSetValues (sdt->resize_arrow_label, XmNmappedWhenManaged, (Boolean)enab, NULL);
}

void
SetDecoratorTextDialog_DirtyTextCB (Widget w, XtPointer cdata, XtPointer cbs)
{
SetDecoratorTextDialog *sdt = (SetDecoratorTextDialog *)cdata;
XmTextVerifyCallbackStruct *tvcs = (XmTextVerifyCallbackStruct*)cbs;
XmTextBlockRec *tb = tvcs->text;
boolean enable_it, disable_it;

    ASSERT(sdt);
    sdt->dirty|= DIRTY_TEXT;

    enable_it = disable_it = FALSE;
    boolean isSensitive = XtIsSensitive(sdt->justifyOM);

    //
    // If you're inserting a newline and the menu is insensitive...
    // you're done and you can just turn on the menu. 
    //
    if ((tb->length) && (tb->ptr) && (!isSensitive) &&
	(strchr(tb->ptr, (int)'\n')) &&
	(tvcs->startPos >= tvcs->currInsert)) {
	enable_it = True;

    //
    // else if you're erasing a chunk and the menu is sensitive, then
    // count the lines exclusive of the erased chunk.
    // The chars between startPos and endPos are going away.
    //
    } else if ((tb->length==0) && (isSensitive)) {
	char *cp = XmTextGetString (w);
	int i,len = ((cp&&cp[0])?strlen(cp):0);
	int lines = 0;

	for (i=0; i<len; i++) {
	    if ((i<tvcs->startPos) || (i>=tvcs->endPos))
		if (cp[i] == '\n') lines++;
	    if (lines>=1) break;
	}

	if (lines<1) disable_it = TRUE;
	XtFree(cp);

    //
    // else you're replacing a chunk and you must count lines exclusive
    // of the erasure and inclusive of the insertion.
    //
    } else if (isSensitive) {
	char *cp = XmTextGetString (w);
	int i,len = ((cp&&cp[0])?strlen(cp):0);
	int lines = 0;

	for (i=0; i<len; i++) {
	    if ((i<tvcs->startPos) || (i>=tvcs->endPos))
		if (cp[i] == '\n') lines++;
	}

	for (i=0; i<tb->length; i++) {
	    if (tb->ptr[i] == '\n') lines++;
	    if (lines>=1) break;
	}

	if (lines<1) disable_it = TRUE;
	XtFree(cp);
    }

    ASSERT (!(enable_it && disable_it));
    if (enable_it) {
	XtSetSensitive (sdt->justifyOM, True);

	//
	// If justification is set to left, then enable the reformat button
	//
	Widget choice;
	char *align;
	XtVaGetValues (sdt->justifyOM, XmNmenuHistory, &choice, NULL);
	ASSERT(choice);
	XtVaGetValues (choice, XmNuserData, &align, NULL);
	boolean enab =  (strcmp (align, "XmALIGNMENT_BEGINNING") == 0);
	XtSetSensitive (sdt->reformat, enab);
    } else if (disable_it) {
	int nkids;
	Widget *kids;
	XtVaGetValues (sdt->justifyPulldown, 
	    XmNnumChildren, &nkids, XmNchildren, &kids, NULL);
	ASSERT (nkids > 0);

	XtVaSetValues (sdt->justifyOM, XmNmenuHistory, kids[nkids-1], NULL);
	sdt->dirty|= DIRTY_JUSTIFY;
	XtSetSensitive (sdt->justifyOM, False);
	XtSetSensitive (sdt->reformat, False);
    }

    tvcs->doit = True;
}

void
SetDecoratorTextDialog_DirtyFontCB (Widget , XtPointer cdata, XtPointer)
{
SetDecoratorTextDialog *sdt = (SetDecoratorTextDialog *)cdata;

    ASSERT(sdt);
    sdt->dirty|= DIRTY_FONT;
}


void
SetDecoratorTextDialog_ApplyCB (Widget , XtPointer cdata, XtPointer)
{
SetDecoratorTextDialog *sdt = (SetDecoratorTextDialog *)cdata;

    ASSERT(sdt);
    sdt->okCallback (sdt);
}

void
SetDecoratorTextDialog_RestoreCB (Widget , XtPointer cdata, XtPointer)
{
SetDecoratorTextDialog *sdt = (SetDecoratorTextDialog *)cdata;

    ASSERT(sdt);
    sdt->restoreCallback (sdt);
}


void
SetDecoratorTextDialog_ReformatCB (Widget , XtPointer cdata, XtPointer)
{
SetDecoratorTextDialog *sdt = (SetDecoratorTextDialog *)cdata;

    ASSERT(sdt);
    sdt->reformatCallback (sdt);
}


void 
SetDecoratorTextDialog_ArrowMotionEH 
    (Widget w, XtPointer cdata, XEvent *xev, Boolean *keep_going)
{
SetDecoratorTextDialog *sdt = (SetDecoratorTextDialog *)cdata;

    ASSERT(sdt);
    *keep_going = True;

    if (!xev) return ;
    if (xev->type != MotionNotify) return ;
    XMotionEvent* xmo = (XMotionEvent*)xev;

    int diff = sdt->initial_xpos - xmo->x_root;
    int newofs = sdt->start_pos + diff;
    if ((newofs > 500) && (sdt->start_pos >= 500)) return ;
    if ((newofs < ARROW_RIGHTMOST_POS) && (sdt->start_pos <= ARROW_RIGHTMOST_POS)) return ;
    if (newofs < ARROW_RIGHTMOST_POS) {
	sdt->start_pos = ARROW_RIGHTMOST_POS;
	sdt->initial_xpos = xmo->x_root - (ARROW_RIGHTMOST_POS-newofs);
    } else if (newofs > 500) {
	sdt->start_pos = 500;
	sdt->initial_xpos = xmo->x_root - (500-newofs);
    } else {
	sdt->initial_xpos = xmo->x_root;
	sdt->start_pos = newofs;
    }

    XtVaSetValues (sdt->resize_arrow, XmNrightOffset, sdt->start_pos, NULL);
}


void 
SetDecoratorTextDialog_StartPosCB(Widget , XtPointer cdata, XtPointer cbs)
{
SetDecoratorTextDialog *sdt = (SetDecoratorTextDialog *)cdata;
XmArrowButtonCallbackStruct *abcs = (XmArrowButtonCallbackStruct*)cbs;
XEvent *xev = abcs->event;

    if (!xev) return ;
    if (xev->type != ButtonPress) return ;
    XButtonEvent* xbe = (XButtonEvent*)xev;
    sdt->initial_xpos = xbe->x_root;

    ASSERT(sdt);
    XtVaGetValues (sdt->resize_arrow, XmNrightOffset, &sdt->start_pos, NULL);
    XtVaSetValues (sdt->getRootWidget(), XmNresizePolicy, XmRESIZE_NONE, NULL);
}


void 
SetDecoratorTextDialog_EndPosCB(Widget , XtPointer cdata, XtPointer)
{
SetDecoratorTextDialog *sdt = (SetDecoratorTextDialog *)cdata;

    ASSERT(sdt);
    XtVaSetValues (XtParent(sdt->editor_magic), XmNrightOffset, sdt->start_pos, NULL);
    sdt->reformatCallback(sdt);
}


} // extern C


//
// Install the default resources for this class.
//
void SetDecoratorTextDialog::installDefaultResources(Widget  baseWidget)
{
    this->setDefaultResources(baseWidget, SetDecoratorTextDialog::DefaultResources);
    this->Dialog::installDefaultResources( baseWidget);
}


//
// Connect the dialog to a new decorator.  Update the displayed values.
//
void SetDecoratorTextDialog::setDecorator(LabelDecorator* new_lab)
{
    this->decorator = new_lab;
    this->dirty = DIRTY_COLORS | DIRTY_JUSTIFY | DIRTY_TEXT | DIRTY_FONT;
    this->restoreCallback(this);

    if (!this->decorator) {
	XtSetSensitive (this->ok, False);
	XtSetSensitive (this->restore, False);
	XtSetSensitive (this->apply, False);
    } else {
	XtSetSensitive (this->ok, True);
	XtSetSensitive (this->restore, True);
	XtSetSensitive (this->apply, True);
    }
}

//
// 1) Make a copy of the text.
// 2) Replace all instances of a WhiteSpace which contain only 1 '\n', with one space
// 3) loop over new buffer jumping ahead XmNcolumns chars each time and then
//    back up until work boundary where you insert a '\n'.
// - When backing up, look for only a space.  This is the normal case.  If there is
//   no space, then settle for ispunct()==1 while scanning ahead.
//
// This doesn't account for variable width fonts.  The value in XmNcolumns is just
// an estimate of the number of chars we can accommodate.  There is in fact no such
// number.  In order to do this correctly, we would have to count the pixels used
// by each char on the line.  Motif supplies a mechanism for that sort of behavior.
// It's what you get when you set XmNwordWrap==True.  Unfortunately, there isn't
// any way to query for the results of Motif's wordwrapping.  If there were, then
// we would use that instead.
//
#define IsSpace(q) ((q==' ')||(q=='\t')||(q=='\n'))
boolean SetDecoratorTextDialog::reformatCallback(SetDecoratorTextDialog*)
{
short cols;

    char *spare = XmTextGetString (this->editorText);
    XtVaGetValues (this->editor_magic, XmNcolumns, &cols, NULL);

    cols+= (cols>>2);
    if (!spare) 
	return FALSE;
    int len = strlen(spare);
    if (!len) {
	XtFree(spare);
	return FALSE;
    }

    //
    // compress whitespace converting \n to ' '
    //
    char* no_extra_space = new char[1+len];
    int i;
    boolean was_space = TRUE;
    boolean was_newline = FALSE;
    int nxt = 0;
    for (i=0; i<len; i++) {
	if (IsSpace(spare[i])) {
	    if (was_space == FALSE) {
		no_extra_space[nxt++] = ' ';
	    } else if (was_newline) {
		if (spare[i] == '\n') {
		    no_extra_space[nxt-1] = '\n';
		    no_extra_space[nxt++] = '\n';
		}
	    } else {
	    }
	    was_newline = (spare[i] == '\n');
	    was_space = TRUE;
	} else {
	    no_extra_space[nxt++] = spare[i];
	    was_space = FALSE;
	    was_newline = FALSE;
	}
    }
    no_extra_space[nxt] = '\0';
    XtFree(spare);
    spare = no_extra_space;

    //
    // adding back the \n
    //
    len = nxt;
    no_extra_space = new char[nxt<<1];

    nxt = 0;
    int copied_in_row = 0;
    int most_recent_space = 0;
    for (i=0; i<len; i++) {
	if (copied_in_row == 0) most_recent_space = -1;
	boolean line_break = (copied_in_row == cols);
	if (line_break) {
	    //
	    // How far away is the next space?
	    //
#	    define HOW_FAR_TO_LOOK 8
	    int to_next_space = HOW_FAR_TO_LOOK+1;
	    int regress = most_recent_space-nxt;
#	    define PREFER_BACKWARDS -(HOW_FAR_TO_LOOK+4)
	    if (regress <= PREFER_BACKWARDS) {
		int k = i;
		while ((spare[k]) && 
		    (to_next_space<HOW_FAR_TO_LOOK) && 
		    (IsSpace(spare[k]) == FALSE)) {
		    k++;
		    to_next_space++;
		}
	    }

	    if ((to_next_space < HOW_FAR_TO_LOOK) && 
		(most_recent_space > 0) && (regress <= PREFER_BACKWARDS)) {
	    } else if (most_recent_space > 0) {
		//
		// If we can regress to a space then do
		//
		nxt = most_recent_space;
		i+= regress;
	    } 
#	    undef HOW_FAR_TO_LOOK
#	    undef PREFER_BACKWARDS

	    //
	    // This handles the case of... we've regressed or we must move ahead
	    //
	    while ((spare[i]) && 
		    (IsSpace(spare[i]) == FALSE) &&
		    (ispunct((int)spare[i]) == 0)) {
		no_extra_space[nxt++] = spare[i++];
	    }

	    no_extra_space[nxt++] = '\n';
	    if ((spare[i] != '\n') && (spare[i] != ' ')) {
		no_extra_space[nxt++] = spare[i];
	    }
	    copied_in_row = 0;
	} else {
	    if ((spare[i] == ' ') || (spare[i] == '\t'))
		most_recent_space = nxt;
	    no_extra_space[nxt++] = spare[i];
	    if (spare[i] == '\n')
		copied_in_row = 0;
	    else
		copied_in_row++;
	}
    }
    no_extra_space[nxt] = '\0';
    delete spare;

    //
    // Prepare for kerning
    //
    this->kern_lines = NULL;
    this->kern_lengths = NULL;
    this->line_count = 0;

    boolean default_justification = FALSE;
    Widget choice;
    XtVaGetValues (this->justifyOM, XmNmenuHistory, &choice, NULL);
    ASSERT(choice);

    if (!strcmp(XtName(choice), DEFAULT_SETTING))
	default_justification = TRUE;

    if (default_justification) {
	//
	// Use the font in our menu for calculating padding.
	//
	char *font_name;
	Widget choice;
	XtVaGetValues (this->fontOM, XmNmenuHistory, &choice, NULL);
	ASSERT(choice);
	XtVaGetValues (choice, XmNuserData, &font_name, NULL);
	spare = this->kern(no_extra_space, font_name);
    } else {
	spare = no_extra_space;
    }

    XmTextSetString(this->editorText, spare);
    delete spare;

    if (this->line_count>1) {
	for (i=0; i<this->line_count; i++) 
	    delete this->kern_lines[i];
	delete this->kern_lines;
	delete this->kern_lengths;
	this->kern_lines = NULL;
	this->kern_lengths = NULL;
	this->line_count = 0;
    }

    return TRUE;
}


char* SetDecoratorTextDialog::kern(char* src, char* font)
{
int i;

    if ((!src) || (!src[0])) return src;

    XmFontList xmfl = this->decorator->getFontList();

    //
    // Chop the input into individual lines and measure each one.
    //
    XmString str1 = XmStringCreateLtoR (src, font);
    boolean retVal = this->measureLines(str1, font);
    int max_pixel_len = XmStringWidth (xmfl, str1);
    XmStringFree(str1);
    if (!retVal) return src;


    //
    // Find the size of a ' ' character.
    //
    XmString s1 = XmStringCreateLtoR ("H i", font);
    XmString s2 = XmStringCreateLtoR ("H  i", font);
    Dimension w1 = XmStringWidth (xmfl, s1);
    Dimension w2 = XmStringWidth (xmfl, s2);
    XmStringFree(s1);
    XmStringFree(s2);
    int size_of_1_space = w2 - w1;
    if (size_of_1_space < 1) return src;


    //
    // If the maximum number of extra spaces required is greater than
    // threshold then skip the line.  If the line requires more than half
    // its length in padding, then skip the line.
    //
    float threshhold = max_pixel_len * 0.25;

    //
    // Pad each line appropriately.
    //
    for (i=0; i<this->line_count; i++) {
	int pixel_diff = max_pixel_len - this->kern_lengths[i];
	if (pixel_diff == 0) continue;
	if (pixel_diff > threshhold) continue;
	if (pixel_diff > (this->kern_lengths[i]>>1)) continue;
	int needed_spaces = pixel_diff / size_of_1_space;
	this->kern_lines[i] = 
	    SetDecoratorTextDialog::Pad (this->kern_lines[i], needed_spaces, 2);
    }

    //
    // reconnect the lines
    //
    int totlen = 0;
    for (i=0; i<this->line_count; i++) 
	totlen+= strlen(this->kern_lines[i]);
    char* newsrc = new char[totlen+1];

    totlen = 0;
    for (i=0; i<this->line_count; i++) {
	int len = strlen(this->kern_lines[i]);
	strcpy (&newsrc[totlen], this->kern_lines[i]);
	totlen+= len;
	newsrc[totlen] = '\0';
    }

    delete src;
    return newsrc;
}

char*
SetDecoratorTextDialog::Pad(char* src, int pad, int every_nth)
{
static char sep_chars[] = { 
    ',', '?', ';', '!', '@', '#', '*',
    '$', '&', '+', '{', '}', '[', ']',
    '|', '=', ' ', '\0'
};

    ASSERT (src);
    if (pad == 0) return src;
    ASSERT (pad > 0);
    int pads_added = 0;
    int len = strlen(src);
    char* newsrc = new char[len+1+pad];
    int space_cnt = 1;

    boolean just_added = FALSE;
    int nxt = 0;
    int i;
    for (i=0; i<len; i++) {
	newsrc[nxt++] = src[i];
	if (pads_added < pad) {
	    boolean is_separator = FALSE;
	    if ((just_added == FALSE) && (every_nth)) {
		if (space_cnt == every_nth) {
		    is_separator = (src[i]==' ');
		    space_cnt = 1;
		} else
		    space_cnt++;
	    } else if (just_added == FALSE) {
		int j = 0;
		while ((is_separator == FALSE) && (sep_chars[j])) 
		    is_separator = (src[i] == sep_chars[j++]);
	    }

	    if (is_separator) {
		newsrc[nxt++] = ' ';
		pads_added++;
		just_added = TRUE;
	    } else
		just_added = FALSE;
	}
    }
    newsrc[nxt] = '\0';

    delete src;
    int remaining = pad - pads_added;
    if (pads_added == 0)
	return newsrc;
    else
	return SetDecoratorTextDialog::Pad(newsrc, remaining, (every_nth?every_nth-1:0));
}

//
// null-terminated char * -- the XmNlabelStinrg
//
boolean SetDecoratorTextDialog::measureLines(XmString srcString, char* font)
{
XmStringContext cxt;
char *text, *tag;
XmStringDirection dir;
int i;

    if (!srcString) return FALSE;
    this->line_count = XmStringLineCount(srcString);
    if (this->line_count<=1)
	return FALSE;

    if (!XmStringInitContext (&cxt, srcString)) {
	XtWarning ("Can't convert compound string.");
	return FALSE;
    }

    this->kern_lines = new String[this->line_count];
    this->kern_lengths = new int[this->line_count];
    int next_kern_line = 0;
    for (i=0; i<this->line_count; i++) {
	this->kern_lengths[i] = 0;
	this->kern_lines[i] = NUL(String);
    }

    int os = 0;
    unsigned short ukl = 0;
    unsigned char* ukv = 0;
    XmStringComponentType uktag = 0;
    char* label_buf = NUL(char*);
    tag = NUL(char*);
    while (1) {
	int len;
	XmStringComponentType retVal = 
	    XmStringGetNextComponent (cxt, &text, &tag, &dir, &uktag, &ukl, &ukv);
	if (retVal == XmSTRING_COMPONENT_END) break;
	switch (retVal) {
	    case XmSTRING_COMPONENT_TEXT:
	    case XmSTRING_COMPONENT_LOCALE_TEXT:
		ASSERT (next_kern_line < this->line_count);
		len = strlen(text);
		label_buf = this->kern_lines[next_kern_line++] =
		    new char[len+2];
		strcpy (&label_buf[os], text);
		os+= len;
		XtFree(text),text = NUL(char*);
		break;
	    case XmSTRING_COMPONENT_SEPARATOR:
		ASSERT (next_kern_line <= this->line_count);
		if (!label_buf) {
		    label_buf = this->kern_lines[next_kern_line++] =
			new char[8];
		}
		label_buf[os++] = '\n';
		label_buf[os] = '\0';
		os = 0;
		label_buf = NUL(char*);
		break;
	    default:
		if (tag) XtFree(tag),tag=NUL(char*);
		break;
	}
    }
    XmStringFreeContext(cxt);

    XmFontList xmfl = this->decorator->getFontList();

    for (i=0; i<this->line_count; i++) {
	if (this->kern_lines[i] == NUL(char*)) {
	    this->kern_lines[i] = new char[2];
	    this->kern_lines[i][0]= '\0';
	}

	XmString xmstr = XmStringCreateLtoR (this->kern_lines[i], font);
	this->kern_lengths[i] = XmStringWidth (xmfl, xmstr);
	XmStringFree(xmstr);
    }
    return TRUE;
}

