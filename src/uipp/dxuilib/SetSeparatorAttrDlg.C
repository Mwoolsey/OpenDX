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
#include "SetSeparatorAttrDlg.h"
#include "SeparatorDecorator.h"
#include "Network.h"
#include <Xm/RowColumn.h>
#include <Xm/PushBG.h>
#include <Xm/Form.h>
#include <Xm/SeparatoG.h>
#include <Xm/Text.h>

boolean SetSeparatorAttrDlg::ClassInitialized = FALSE;

String SetSeparatorAttrDlg::DefaultResources[] =
{
        "*dialogTitle:     		Separator Attributes...\n",
	"*colorOM.labelString:		Separator Color:",
	"*styleOM.labelString:		Line Style:",
	"*XmPushButtonGadget.traversalOn: False",
	"*XmPushButton.traversalOn: 	False",

	"*okButton.labelString:		OK",
	"*applyButton.labelString:	Apply",
	"*restoreButton.labelString:	Restore",
	"*cancelButton.labelString:	Cancel",
	"*okButton.width:		60",
	"*applyButton.width:		60",
	"*restoreButton.width:		60",
	"*cancelButton.width:		60",

        NULL
};


#define DIRTY_COLORS 1
#define DIRTY_STYLE 2

SetSeparatorAttrDlg::SetSeparatorAttrDlg(Widget parent,
				boolean , SeparatorDecorator *dec) : 
				Dialog("SeparatorAttributes",parent)
{
    this->decorator = dec;
    this->dirty = DIRTY_COLORS | DIRTY_STYLE ;

    if (!SetSeparatorAttrDlg::ClassInitialized) {
	SetSeparatorAttrDlg::ClassInitialized = TRUE;
        this->installDefaultResources(theApplication->getRootWidget());
    }
}

SetSeparatorAttrDlg::~SetSeparatorAttrDlg()
{
}

//
// Install the proper values whenever the dialog box is put onto the screen.
//
void
SetSeparatorAttrDlg::post()
{
    this->Dialog::post();
    this->restoreCallback (this);
}

boolean
SetSeparatorAttrDlg::restoreCallback (Dialog * )
{
    //
    // color
    // The default will be kept in kids[nkids-1]
    //
    if (this->dirty & DIRTY_COLORS) {
	int i,nkids;
	Widget *kids;
	const char *color;
	char *cp;
	if (this->decorator->isResourceSet (XmNforeground)) {
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


    if (this->dirty & DIRTY_STYLE) {
	int i,nkids;
	Widget *kids;
	const char *line_style;
	char *cp;
	if (this->decorator->isResourceSet (XmNseparatorType)) {
	    line_style = this->decorator->getResourceString(XmNseparatorType);
	} else {
	    line_style = NULL;
	}
	XtVaGetValues (this->stylePulldown, 
	    XmNnumChildren, &nkids, XmNchildren, &kids, NULL);
	ASSERT (nkids > 0);
	if ((line_style) && (line_style[0])) {
	    for (i=0; i<nkids; i++) {
		XtVaGetValues (kids[i], XmNuserData, &cp, NULL);
		if (!strcmp(cp, line_style)) break;
	    }
	    if (i==nkids) i = nkids - 1;
	} else {
	    i = nkids - 1;
	}

	XtVaSetValues (this->styleOM, XmNmenuHistory, kids[i], NULL);
	this->dirty&= ~DIRTY_STYLE;
    }


    return TRUE;
}

boolean
SetSeparatorAttrDlg::okCallback (Dialog* )
{
Colormap cmap;
Screen *screen;
Display *d = XtDisplay(this->colorOM);
XrmValue toinout, from;
Pixel bg, ts, bs;
XColor cdef;
Boolean use_default = FALSE;

    if (this->dirty) {
	this->decorator->getNetwork()->setFileDirty();
    }

    Widget choice;

    if (this->dirty & DIRTY_COLORS) {
	char *color;
	char tscolor[32], bscolor[32];
	XtVaGetValues (this->colorOM, XmNmenuHistory, &choice, NULL);
	ASSERT(choice);
	XtVaGetValues (choice, XmNuserData, &color, NULL);
	XtVaGetValues (this->colorOM,
	    XmNcolormap, &cmap,
	    XmNscreen, &screen,
	NULL);
	if (!strcmp(XtName(choice), DEFAULT_SETTING)) {
	    color = "#b4b4b4b4b4b4";
	    use_default = TRUE;
	} 

	toinout.size = sizeof(Pixel);
	toinout.addr = (XPointer)&bg;
	from.addr = color;
	from.size = 1+strlen(from.addr);
	Boolean resOK = XtConvertAndStore (this->getRootWidget(), XmRString, 
	    &from, XmRPixel, &toinout);
	ASSERT(resOK);
	XmGetColors (screen, cmap, bg, NULL, &ts, &bs, NULL);

	cdef.pixel = bg;
	XQueryColor (d, cmap, &cdef);
	sprintf (tscolor, "#%4.4x%4.4x%4.4x", cdef.red, cdef.green, cdef.blue);

	cdef.pixel = ts;
	XQueryColor (d, cmap, &cdef);
	sprintf (tscolor, "#%4.4x%4.4x%4.4x", cdef.red, cdef.green, cdef.blue);

	cdef.pixel = bs;
	XQueryColor (d, cmap, &cdef);
	sprintf (bscolor, "#%4.4x%4.4x%4.4x", cdef.red, cdef.green, cdef.blue);

	this->decorator->setResource (XmNtopShadowColor, tscolor); 
	this->decorator->setResource (XmNbottomShadowColor, bscolor); 

	if (use_default) {
	    this->decorator->setResource (XmNtopShadowColor, NUL(const char *));
	    this->decorator->setResource (XmNbottomShadowColor, NUL(const char *));
	    this->decorator->setResource (XmNforeground, "Black");
	    this->decorator->setResource (XmNforeground, NUL(const char *));
	} else {
	    this->decorator->setResource (XmNforeground, color); 
	}

	this->dirty&= ~DIRTY_COLORS;
    }

    if (this->dirty & DIRTY_STYLE) {
        char *line_style;
        XtVaGetValues (this->styleOM, XmNmenuHistory, &choice, NULL);
        ASSERT(choice);
        XtVaGetValues (choice, XmNuserData, &line_style, NULL);
        this->decorator->setResource (XmNseparatorType, line_style);
 
        if (!strcmp(XtName(choice), DEFAULT_SETTING))
            this->decorator->setResource(XmNseparatorType, NUL(const char *));
 
        this->dirty&= ~DIRTY_STYLE;
    }


    return TRUE;
}

Widget
SetSeparatorAttrDlg::createDialog(Widget parent)
{
int n = 0;
Widget form;
Arg args[20];

    n = 0;
    XtSetArg (args[n], XmNautoUnmanage, False); n++;
    XtSetArg (args[n], XmNminWidth, 610); n++;
    form = this->CreateMainForm (parent, this->name, args, n);

    n = 0;
    XtSetArg (args[n], XmNtopAttachment, XmATTACH_FORM); n++;
    XtSetArg (args[n], XmNtopOffset, 10); n++;
    XtSetArg (args[n], XmNleftAttachment, XmATTACH_FORM); n++;
    XtSetArg (args[n], XmNleftOffset, 20); n++;
    XtSetArg (args[n], XmNsubMenuId, this->createColorMenu(form)); n++;
    this->colorOM = XmCreateOptionMenu (form, "colorOM", args, n);
    XtManageChild (this->colorOM);

    n = 0;
    XtSetArg (args[n], XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
    XtSetArg (args[n], XmNtopWidget, this->colorOM); n++;
    XtSetArg (args[n], XmNrightAttachment, XmATTACH_FORM); n++;
    XtSetArg (args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
    XtSetArg (args[n], XmNleftWidget, this->colorOM); n++;
    XtSetArg (args[n], XmNleftOffset, 20); n++;
    XtSetArg (args[n], XmNrightOffset, 20); n++;
    XtSetArg (args[n], XmNsubMenuId, this->createStyleMenu(form)); n++;
    this->styleOM = XmCreateOptionMenu (form, "styleOM", args, n);
    XtManageChild (this->styleOM);

    n = 0;
    XtSetArg (args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
    XtSetArg (args[n], XmNbottomAttachment, XmATTACH_WIDGET); n++;
    XtSetArg (args[n], XmNtopWidget, colorOM); n++;
    XtSetArg (args[n], XmNtopOffset, 10); n++;
    XtSetArg (args[n], XmNbottomOffset, 50); n++;
    XtSetArg (args[n], XmNleftAttachment, XmATTACH_FORM); n++;
    XtSetArg (args[n], XmNrightAttachment, XmATTACH_FORM); n++;
    Widget sep2 = XmCreateSeparatorGadget (form, "separator", args, n);
    XtManageChild (sep2);

    n = 0;
    XtSetArg (args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
    XtSetArg (args[n], XmNtopWidget, sep2); n++;
    XtSetArg (args[n], XmNtopOffset, 14); n++;
    XtSetArg (args[n], XmNleftAttachment, XmATTACH_FORM); n++;
    XtSetArg (args[n], XmNleftOffset, 14); n++;
    this->ok = XmCreatePushButtonGadget (form, "okButton", args, n);
    XtManageChild (this->ok);

    n = 0;
    XtSetArg (args[n], XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
    XtSetArg (args[n], XmNtopWidget, this->ok); n++;
    XtSetArg (args[n], XmNtopOffset, 0); n++;
    XtSetArg (args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
    XtSetArg (args[n], XmNleftWidget, this->ok); n++;
    XtSetArg (args[n], XmNleftOffset, 10); n++;
    Widget apply = XmCreatePushButtonGadget (form, "applyButton", args, n);
    XtManageChild (apply);
    XtAddCallback (apply, XmNactivateCallback, (XtCallbackProc)
	SetSeparatorAttrDlg_ApplyCB, (XtPointer)this);

    n = 0;
    XtSetArg (args[n], XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
    XtSetArg (args[n], XmNtopWidget, this->ok); n++;
    XtSetArg (args[n], XmNtopOffset, 0); n++;
    XtSetArg (args[n], XmNrightAttachment, XmATTACH_FORM); n++;
    XtSetArg (args[n], XmNrightOffset, 14); n++;
    this->cancel = XmCreatePushButtonGadget (form, "cancelButton", args, n);
    XtManageChild (this->cancel);

    n = 0;
    XtSetArg (args[n], XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
    XtSetArg (args[n], XmNtopWidget, this->ok); n++;
    XtSetArg (args[n], XmNtopOffset, 0); n++;
    XtSetArg (args[n], XmNrightAttachment, XmATTACH_WIDGET); n++;
    XtSetArg (args[n], XmNrightWidget, this->cancel); n++;
    XtSetArg (args[n], XmNrightOffset, 10); n++;
    Widget restore = XmCreatePushButtonGadget (form, "restoreButton", args, n);
    XtManageChild (restore);
    XtAddCallback (restore, XmNactivateCallback, (XtCallbackProc)
	SetSeparatorAttrDlg_RestoreCB, (XtPointer)this);

    return form;
}

Widget
SetSeparatorAttrDlg::createColorMenu(Widget parent)
{
Widget button;
// The other createMenu() funcs use 2 arrays.  There's only 1 here because
// we'll just use the button name as the color name.  There's not likely to
// be a difference between what you show the user and what you really use
// for a color name so that will work.
//
const char **btn_names = this->decorator->getSupportedColorNames();
const char **color_values = this->decorator->getSupportedColorValues();


Arg args[9];
int i,n;

    n = 0;
    this->colorPulldown = XmCreatePulldownMenu (parent, "pulldownMenu", args, n);

    i = 0;
    while (btn_names[i]) {
	char *btn_name = new char[1+strlen(btn_names[i])];
	strcpy (btn_name, btn_names[i]);
	char *color_value = new char[1+strlen(color_values[i])];
	strcpy (color_value, color_values[i]);
	XmString xmstr = XmStringCreateLtoR (btn_name, "bold");

	n = 0;
	XtSetArg (args[n], XmNlabelString, xmstr); n++;
	XtSetArg (args[n], XmNuserData, color_values[i]); n++;
	button = XmCreatePushButtonGadget (this->colorPulldown, btn_name, args, n);
	XtManageChild (button);
	XtAddCallback (button, XmNactivateCallback, (XtCallbackProc)
	    SetSeparatorAttrDlg_DirtyColorsCB, (XtPointer)this);

	delete btn_name;
	delete color_value;
	i++;
    }

    return this->colorPulldown;
}

Widget
SetSeparatorAttrDlg::createStyleMenu(Widget parent)
{
Widget button;
#if defined(aviion) || defined(hp700)
static
#endif
char *btn_names[] = {
    "Single Line",		"Double Line",	"Single Dashed Line",
    "Double Dashed Line",	"Etched In",	"Etched Out",
    DEFAULT_SETTING
};

#if defined(aviion) || defined(hp700)
static
#endif
char *type_names[] = {
    "XmSINGLE_LINE",		"XmDOUBLE_LINE",	"XmSINGLE_DASHED_LINE",
    "XmDOUBLE_DASHED_LINE",	"XmSHADOW_ETCHED_IN",	"XmSHADOW_ETCHED_OUT",
    "XmSHADOW_ETCHED_IN"
};

Arg args[9];
int i,n;

    n = 0;
    this->stylePulldown = XmCreatePulldownMenu (parent, "pulldownMenu", args, n);

    for (i=0; i<XtNumber(btn_names); i++) {
	n = 0;
	XtSetArg (args[n], XmNuserData, type_names[i]); n++;
	button = XmCreatePushButtonGadget (this->stylePulldown, btn_names[i], args, n);
	XtManageChild (button);
	XtAddCallback (button, XmNactivateCallback, (XtCallbackProc)
	    SetSeparatorAttrDlg_DirtyStyleCB, (XtPointer)this);
    }

    return this->stylePulldown;
}



extern "C" {

void
SetSeparatorAttrDlg_DirtyColorsCB (Widget , XtPointer cdata, XtPointer)
{
SetSeparatorAttrDlg *sdt = (SetSeparatorAttrDlg *)cdata;

    ASSERT(sdt);
    sdt->dirty|= DIRTY_COLORS;
}

void
SetSeparatorAttrDlg_DirtyStyleCB (Widget , XtPointer cdata, XtPointer)
{
SetSeparatorAttrDlg *sdt = (SetSeparatorAttrDlg *)cdata;

    ASSERT(sdt);
    sdt->dirty|= DIRTY_STYLE;
}


void
SetSeparatorAttrDlg_ApplyCB (Widget , XtPointer cdata, XtPointer)
{
SetSeparatorAttrDlg *sdt = (SetSeparatorAttrDlg *)cdata;

    ASSERT(sdt);
    sdt->okCallback (sdt);
}

void
SetSeparatorAttrDlg_RestoreCB (Widget , XtPointer cdata, XtPointer)
{
SetSeparatorAttrDlg *sdt = (SetSeparatorAttrDlg *)cdata;

    ASSERT(sdt);
    sdt->restoreCallback (sdt);
}


} // extern C


//
// Install the default resources for this class.
//
void SetSeparatorAttrDlg::installDefaultResources(Widget  baseWidget)
{
    this->setDefaultResources(baseWidget, SetSeparatorAttrDlg::DefaultResources);
    this->Dialog::installDefaultResources( baseWidget);
}

