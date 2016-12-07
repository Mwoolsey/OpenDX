/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#include <X11/StringDefs.h>

#include <Xm/Xm.h>
#include <Xm/DialogS.h>
#include <Xm/Form.h>
#include <Xm/PushB.h>
#include <Xm/ToggleB.h>
#include <Xm/Separator.h>
#include <Xm/Label.h>
#include <Xm/Text.h>
#include <Xm/TextF.h>
#include <Xm/RowColumn.h>
#include <Xm/ToggleBG.h>

#include "DXStrings.h"
#include "lex.h"
#include "AutoAxesDialog.h"
#include "DXType.h"
#include "DXValue.h"
#include "ImageWindow.h"
#include "WarningDialogManager.h"
#include "../base/Application.h"
#include "../base/TextPopup.h"
#include "../widgets/Number.h"
#include "../widgets/Stepper.h"
#include "DrivenNode.h"
#include "TickLabelList.h"
#include "ticks_in.bm"
#include "ticks_out.bm"
#include "ticks_in_ins.bm"
#include "ticks_out_ins.bm"

#define  OK	 0
#define  APPLY   1
#define  RESTORE 2 
#define  CANCEL  3
#define  EXPAND  4

#define DIRTY_LABELS	(1<<0)
#define DIRTY_FRAME	(1<<1)
#define DIRTY_GRID	(1<<2)
#define DIRTY_ADJUST	(1<<3)
#define DIRTY_TICKS	(1<<4)
#define DIRTY_COLORS	(1<<5)
#define DIRTY_CORNERS	(1<<6)
#define DIRTY_CURSOR	(1<<7)
#define DIRTY_FONT	(1<<8)
#define DIRTY_SCALE	(1<<9)
#define DIRTY_ENABLE	(1<<10)
#define DIRTY_XTICK_LOCS	(1<<11)
#define DIRTY_YTICK_LOCS	(1<<12)
#define DIRTY_ZTICK_LOCS	(1<<13)
#define DIRTY_XTICK_LABELS	(1<<14)
#define DIRTY_YTICK_LABELS	(1<<15)
#define DIRTY_ZTICK_LABELS	(1<<16)

#define XTICK_LIST 0
#define YTICK_LIST 1
#define ZTICK_LIST 2
#define MAX_TICK_LABEL_LEN 64

int AutoAxesDialog::MinWidths[] = {
	455,	// MAIN_FORM
	445,	// LABEL_INPUTS
	445,	// FLAG_INPUTS
#if BG_IS_TEXTPOPUP
	510,	// COLOR_INPUTS
#else
	575,	// COLOR_INPUTS
#endif
	465,	// CORNER_INPUTS
	575,	// TICK_INPUTS
	650,	// TICK_LABELS
	445	// BUTTON_FORM
};

Pixmap AutoAxesDialog::TicksIn = NUL(Pixmap);
Pixmap AutoAxesDialog::TicksOut = NUL(Pixmap);
Pixmap AutoAxesDialog::TicksInGrey = NUL(Pixmap);
Pixmap AutoAxesDialog::TicksOutGrey = NUL(Pixmap);

#define OPTION_MENU_WIDTH 65
#define RES_CONVERT(res, str) XtVaTypedArg, res, XmRString, str, STRLEN(str)+1

#define SET_OM_NAME(w, name) {					\
	Widget wtmp;						\
	WidgetList children;					\
	int num_children;					\
	int itmp;						\
	XtVaGetValues(w, XmNsubMenuId, &wtmp, NULL);		\
	XtVaGetValues(wtmp, XmNchildren, &children, 		\
		XmNnumChildren, &num_children, NULL);		\
	for(itmp = 0; itmp < num_children; itmp++)		\
	{							\
	    if(EqualString(XtName(children[itmp]), name))	\
	    {							\
		XtVaSetValues(w, XmNmenuHistory, children[itmp],NULL);\
		break;						\
	    }							\
	}							\
	if(itmp == num_children)				\
	    WarningMessage("Set value failed:%s", name);}


boolean AutoAxesDialog::ClassInitialized = FALSE;

String  AutoAxesDialog::DefaultResources[] = {
	"*dialogTitle:				AutoAxes Configuration...",
	"*All.labelString:			All",
	"*PerAxis.labelString:			Per Axis",
	"*On.labelString:			On",
	"*Off.labelString:			Off",
	"*clear.labelString:			clear",
	"*opaque.labelString:			opaque",
	"*tickDirection.labelString:		Ticks' Direction",

	"*okBut.width:				70",
	"*okBut.recomputeSize:			False",
	"*okBut.labelString:			OK",
	"*cancelBut.width:			70",
	"*cancelBut.recomputeSize:		False",
	"*cancelBut.labelString:		Cancel",
	"*applyBut.width:			70",
	"*applyBut.recomputeSize:		False",
	"*applyBut.labelString:			Apply",
	"*restoreBut.width:			70",
	"*restoreBut.recomputeSize:		False",
	"*restoreBut.labelString:		Restore",

	"*om*XmPushButton.width:		70",
	"*om*XmPushButton.recomputeSize:	False",

	"*XmSeparator.leftOffset:		0",
	"*XmSeparator.rightOffset:		0",
	"*XmNumber.editable:			True",
	"*ticks_number.iValue:			15",
	"*popupButton.width:			20",

	"*flagsButton.width:		140",
	"*colorsButton.width:		140",
	"*cornersButton.width:		140",
	"*ticksButton.width:		140",
	"*labelsButton.width:		140",
	"*tickLabelsButton.width:	140",

	"*xListToggle.width:		100",
	"*yListToggle.width:		100",
	"*zListToggle.width:		100",

	"*xListToggle.indicatorOn:	False",
	"*yListToggle.indicatorOn:	False",
	"*zListToggle.indicatorOn:	False",

	"*xListToggle.labelString:	X Axis Ticks",
	"*yListToggle.labelString:	Y Axis Ticks",
	"*zListToggle.labelString:	Z Axis Ticks",

	"*flagsButton.indicatorOn:	False",
	"*colorsButton.indicatorOn:	False",
	"*cornersButton.indicatorOn:	False",
	"*ticksButton.indicatorOn:	False",
	"*labelsButton.indicatorOn:	False",
	"*tickLabelsButton.indicatorOn:	False",

	"*colorsButton.labelString:	Annotation Colors",
	"*cornersButton.labelString:	Corners / Cursor",
	"*ticksButton.labelString:	Ticks",
	"*labelsButton.labelString:	Axes' Labels",
	"*flagsButton.labelString:	Miscellaneous",
	"*tickLabelsButton.labelString:	Ticks' Values",

	"*labelsHeader.labelString:	Axes' Labels",
	"*labelsHeader.foreground:	SteelBlue",
	"*labelsHeader.rightOffset:	30",
	"*colorsHeader.labelString:	Annotation Colors",
	"*colorsHeader.foreground:	SteelBlue",
	"*colorsHeader.rightOffset:	30",
	"*ticksHeader.labelString:	Ticks",
	"*ticksHeader.foreground:	SteelBlue",
	"*ticksHeader.rightOffset:	30",
	"*flagsHeader.labelString:	Miscellaneous",
	"*flagsHeader.foreground:	SteelBlue",
	"*flagsHeader.rightOffset:	30",
	"*tickLabelsHeader.labelString:	Ticks' Values",
	"*tickLabelsHeader.foreground:	SteelBlue",
	"*tickLabelsHeader.rightOffset:	30",
	"*cornersHeader.labelString:	Corners / Cursor",
	"*cornersHeader.foreground:	SteelBlue",
	"*cornersHeader.rightOffset:	30",
	"*choicesHeader.labelString:	Input Groups",
	"*choicesHeader.foreground:	SteelBlue",
	"*choicesHeader.rightOffset:	30",

	"*tickLabelsForm.resizePolicy:	XmRESIZE_NONE",

	"*flagsForm.leftOffset:		4",
	"*cornersForm.leftOffset:	4",
	"*labelsForm.leftOffset:	4",
	"*buttonForm.leftOffset:	4",
	"*colorsForm.leftOffset:	4",
	"*ticksForm.leftOffset:		4",
	"*tickLabelsForm.leftOffset:	4",
	"*mainForm.leftOffset:		4",

	"*flagsForm.rightOffset:	4",
	"*cornersForm.rightOffset:	4",
	"*labelsForm.rightOffset:	4",
	"*buttonForm.rightOffset:	4",
	"*colorsForm.rightOffset:	4",
	"*ticksForm.rightOffset:	4",
	"*tickLabelsForm.rightOffset:	4",
	"*mainForm.rightOffset:		4",

	"*cornersForm.topOffset:	8",
	"*flagsForm.topOffset:		8",
	"*labelsForm.topOffset:		8",
	"*buttonForm.topOffset:		8",
	"*colorsForm.topOffset:		8",
	"*ticksForm.topOffset:		8",
	"*tickLabelsForm.topOffset:	8",
	"*mainForm.topOffset:		8",

	"*cornersForm.bottomOffset:	8",
	"*labelsForm.bottomOffset:	8",
	"*buttonForm.bottomOffset:	8",
	"*colorsForm.bottomOffset:	8",
	"*ticksForm.bottomOffset:	8",
	"*tickLabelsForm.bottomOffset:	8",
	"*flagsForm.bottomOffset:	8",
	"*mainForm.bottomOffset:	8",

	"*bgColor.columns:		9",
	"*gridColor.columns:		9",
	"*tickColor.columns:		9",
	"*labelColor.columns:		9",

	"*accelerators:             #augment\n"
        "<Key>Return:                   BulletinBoardReturn()",
 
	"*popupButton.labelString:	...",

#if defined(aviion)
	"*om.labelString:",
	"*tdir_om.labelString:",
#endif
        NULL
};

//#define BG_IS_TEXTPOPUP 1
#if defined(BG_IS_TEXTPOPUP)
static const char *BGColors[] = {
			"red",
			"green",
			"blue",
			"yellow", 
			"black",
			"white",
			"clear"
};
#endif

static const char *FontList[] = {
			"area",
			"cyril_d",
			"fixed",
			"gothiceng_t", 
			"gothicger_t",
			"gothicit_t",
			"greek_d",
			"greek_s",
			"italic_d",
			"italic_t",
			 "pitman",
			 "roman_d",
			 "roman_dser",
			 "roman_s",
			 "roman_tser",
			 "roman_ext",
			 "script_d",
			 "script_s",
			 "variable"
};

Widget AutoAxesDialog::createDialog(Widget parent)
{
    int 	n;
    Arg		args[25];

    n = 0;
    XtSetArg(args[n], XmNautoUnmanage,  False); n++;
    this->toplevelform = this->CreateMainForm(parent, this->name, args, n);

    this->subForms[MAIN_FORM] = XtVaCreateManagedWidget("mainForm",
	xmFormWidgetClass, 	this->toplevelform,
	XmNtopAttachment,	XmATTACH_FORM,
	XmNleftAttachment,	XmATTACH_FORM,
	XmNrightAttachment,	XmATTACH_FORM,
        NULL);

    this->enable_tb = XtVaCreateManagedWidget("AutoAxes enabled",
	xmToggleButtonWidgetClass, this->subForms[MAIN_FORM],
	XmNleftAttachment,	XmATTACH_FORM,
	XmNleftOffset,		5,
	XmNtopAttachment,	XmATTACH_FORM,
	XmNindicatorType,	XmN_OF_MANY,
	XmNset,			False,
	XmNshadowThickness,	0,
	XmNuserData,		DIRTY_ENABLE,
	NULL);
    XtAddCallback(this->enable_tb,
		  XmNvalueChangedCallback, 
		  (XtCallbackProc)AutoAxesDialog_DirtyCB, 
		  (XtPointer)this);

    this->subForms[LABEL_INPUTS] = XtVaCreateWidget("labelsForm",
	xmFormWidgetClass, 	this->toplevelform,
	XmNleftAttachment,	XmATTACH_FORM,
	XmNrightAttachment,	XmATTACH_FORM,
    NULL);
    this->layoutLabels(this->subForms[LABEL_INPUTS]);

    this->subForms[TICK_INPUTS] = XtVaCreateWidget("ticksForm",
	xmFormWidgetClass, 	this->toplevelform,
	XmNleftAttachment,	XmATTACH_FORM,
	XmNrightAttachment,	XmATTACH_FORM,
    NULL);
    this->layoutTicks(this->subForms[TICK_INPUTS]);

    this->subForms[CORNER_INPUTS] = XtVaCreateWidget("cornersForm",
	xmFormWidgetClass, 	this->toplevelform,
	XmNleftAttachment,	XmATTACH_FORM,
	XmNrightAttachment,	XmATTACH_FORM,
    NULL);
    this->layoutCorners(this->subForms[CORNER_INPUTS]);

    this->subForms[COLOR_INPUTS] = XtVaCreateWidget("colorsForm",
	xmFormWidgetClass, 	this->toplevelform,
	XmNleftAttachment,	XmATTACH_FORM,
	XmNrightAttachment,	XmATTACH_FORM,
    NULL);
    this->layoutColors(this->subForms[COLOR_INPUTS]);

    this->subForms[FLAG_INPUTS] = XtVaCreateWidget("flagsForm",
	xmFormWidgetClass, 	this->toplevelform,
	XmNleftAttachment,	XmATTACH_FORM,
	XmNrightAttachment,	XmATTACH_FORM,
    NULL);
    this->layoutFlags(this->subForms[FLAG_INPUTS]);

    this->subForms[TICK_LABELS] = XtVaCreateWidget("tickLabelsForm",
	xmFormWidgetClass, 	this->toplevelform,
	XmNleftAttachment,	XmATTACH_FORM,
	XmNrightAttachment,	XmATTACH_FORM,
    NULL);
    this->layoutTickLabels(this->subForms[TICK_LABELS]);

    XtVaCreateManagedWidget ("choicesHeader",
	xmLabelWidgetClass,	this->subForms[MAIN_FORM],
	XmNtopAttachment,	XmATTACH_WIDGET,
	XmNtopWidget,		this->enable_tb,
	XmNrightAttachment,	XmATTACH_FORM,
	XmNtopOffset,		0,
    NULL);
	
    Widget sep = XtVaCreateManagedWidget ("sep",
	xmSeparatorWidgetClass,	this->subForms[MAIN_FORM],
	XmNtopAttachment,	XmATTACH_WIDGET,
	XmNtopWidget,		this->enable_tb,
	XmNleftAttachment,	XmATTACH_FORM,
	XmNrightAttachment,	XmATTACH_FORM,
	XmNtopOffset,		12,
	XmNleftOffset,		0,
	XmNrightOffset,		0,
    NULL);

    Widget button;
    n = 0;
    XtSetArg (args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
    XtSetArg (args[n], XmNtopWidget, sep); n++;
    XtSetArg (args[n], XmNtopOffset, 20); n++;
    XtSetArg (args[n], XmNleftAttachment, XmATTACH_FORM); n++;
    XtSetArg (args[n], XmNleftOffset, 12); n++;
    XtSetArg (args[n], XmNset, False); n++;
    XtSetArg (args[n], XmNuserData, LABEL_INPUTS); n++;
    button = this->notebookToggles[LABEL_INPUTS] =
	XmCreateToggleButton (this->subForms[MAIN_FORM], "labelsButton", args, n);
    XtManageChild (button);
    XtAddCallback (button, XmNvalueChangedCallback, 
	(XtCallbackProc)AutoAxesDialog_NoteBookCB, (XtPointer)this);

    n = 0;
    XtSetArg (args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
    XtSetArg (args[n], XmNtopWidget, this->notebookToggles[LABEL_INPUTS]); n++;
    XtSetArg (args[n], XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
    XtSetArg (args[n], XmNleftWidget, this->notebookToggles[LABEL_INPUTS]); n++;
    XtSetArg (args[n], XmNleftOffset, 0); n++;
    XtSetArg (args[n], XmNrightAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
    XtSetArg (args[n], XmNrightWidget, this->notebookToggles[LABEL_INPUTS]); n++;
    XtSetArg (args[n], XmNrightOffset, 0); n++;
    XtSetArg (args[n], XmNset, False); n++;
    XtSetArg (args[n], XmNuserData, FLAG_INPUTS); n++;
    button = this->notebookToggles[FLAG_INPUTS] =
	XmCreateToggleButton(this->subForms[MAIN_FORM], "flagsButton", args, n);
    XtManageChild (button);
    XtAddCallback (button, XmNvalueChangedCallback, 
	(XtCallbackProc)AutoAxesDialog_NoteBookCB, (XtPointer)this);


    n = 0;
    XtSetArg (args[n], XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
    XtSetArg (args[n], XmNtopWidget, this->notebookToggles[LABEL_INPUTS]); n++;
    XtSetArg (args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
    XtSetArg (args[n], XmNleftWidget, this->notebookToggles[LABEL_INPUTS]); n++;
    XtSetArg (args[n], XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
    XtSetArg (args[n], XmNbottomWidget, this->notebookToggles[LABEL_INPUTS]); n++;
    XtSetArg (args[n], XmNtopOffset, 0); n++;
    XtSetArg (args[n], XmNbottomOffset, 0); n++;
    XtSetArg (args[n], XmNset, False); n++;

    XtSetArg (args[n], XmNuserData, COLOR_INPUTS); n++;
    button = this->notebookToggles[COLOR_INPUTS] = 
	XmCreateToggleButton (this->subForms[MAIN_FORM], "colorsButton", args, n);
    XtManageChild (button);
    XtAddCallback (button, XmNvalueChangedCallback, 
	(XtCallbackProc)AutoAxesDialog_NoteBookCB, (XtPointer)this);

    n = 0;
    XtSetArg (args[n], XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
    XtSetArg (args[n], XmNtopWidget, this->notebookToggles[LABEL_INPUTS]); n++;
    XtSetArg (args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
    XtSetArg (args[n], XmNleftWidget, button); n++;
    XtSetArg (args[n], XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
    XtSetArg (args[n], XmNbottomWidget, this->notebookToggles[LABEL_INPUTS]); n++;
    XtSetArg (args[n], XmNtopOffset, 0); n++;
    XtSetArg (args[n], XmNbottomOffset, 0); n++;
    XtSetArg (args[n], XmNset, False); n++;

    XtSetArg (args[n], XmNuserData, TICK_INPUTS); n++;
    button = this->notebookToggles[TICK_INPUTS] = 
	XmCreateToggleButton (this->subForms[MAIN_FORM], "ticksButton", args,n);
    XtManageChild (this->notebookToggles[TICK_INPUTS]);
    XtAddCallback (this->notebookToggles[TICK_INPUTS], XmNvalueChangedCallback, 
	(XtCallbackProc)AutoAxesDialog_NoteBookCB, (XtPointer)this);

    n = 0;
    XtSetArg (args[n], XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
    XtSetArg (args[n], XmNtopWidget, this->notebookToggles[FLAG_INPUTS]); n++;
    XtSetArg (args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
    XtSetArg (args[n], XmNleftWidget, this->notebookToggles[FLAG_INPUTS]); n++;
    XtSetArg (args[n], XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
    XtSetArg (args[n], XmNbottomWidget, this->notebookToggles[FLAG_INPUTS]); n++;
    XtSetArg (args[n], XmNtopOffset, 0); n++;
    XtSetArg (args[n], XmNbottomOffset, 0); n++;
    XtSetArg (args[n], XmNset, False); n++;

    XtSetArg (args[n], XmNuserData, CORNER_INPUTS); n++;
    button = this->notebookToggles[CORNER_INPUTS] = 
	XmCreateToggleButton (this->subForms[MAIN_FORM], "cornersButton", args, n);
    XtManageChild (button);
    XtAddCallback (button, XmNvalueChangedCallback, 
	(XtCallbackProc)AutoAxesDialog_NoteBookCB, (XtPointer)this);

    n = 0;
    XtSetArg (args[n], XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
    XtSetArg (args[n], XmNtopWidget, this->notebookToggles[FLAG_INPUTS]); n++;
    XtSetArg (args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
    XtSetArg (args[n], XmNleftWidget, button); n++;
    XtSetArg (args[n], XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
    XtSetArg (args[n], XmNbottomWidget, this->notebookToggles[FLAG_INPUTS]); n++;
    XtSetArg (args[n], XmNtopOffset, 0); n++;
    XtSetArg (args[n], XmNbottomOffset, 0); n++;
    XtSetArg (args[n], XmNset, False); n++;

    XtSetArg (args[n], XmNuserData, TICK_LABELS); n++;
    button = this->notebookToggles[TICK_LABELS] =
	XmCreateToggleButton (this->subForms[MAIN_FORM],"tickLabelsButton", args, n);
    XtManageChild (button);
    XtAddCallback (button, XmNvalueChangedCallback, 
	(XtCallbackProc)AutoAxesDialog_NoteBookCB, (XtPointer)this);
    XtAddCallback (this->notebookToggles[TICK_LABELS], XmNvalueChangedCallback, 
	(XtCallbackProc)AutoAxesDialog_TickLabelToggleCB, (XtPointer)this);

    this->subForms[BUTTON_FORM] = XtVaCreateManagedWidget("buttonForm",
	xmFormWidgetClass, 	this->toplevelform,
	XmNleftAttachment,	XmATTACH_FORM,
	XmNrightAttachment,	XmATTACH_FORM,
	XmNbottomAttachment,	XmATTACH_FORM,
	XmNleftOffset,		6,
	XmNrightOffset,		6,
	XmNbottomOffset,	6,
        NULL);
    this->layoutButtons (this->subForms[BUTTON_FORM]);
    XtVaCreateManagedWidget ("sep",
	xmSeparatorWidgetClass,	this->toplevelform,
	XmNleftAttachment,	XmATTACH_FORM,
	XmNrightAttachment,	XmATTACH_FORM,
	XmNbottomAttachment,	XmATTACH_WIDGET,
	XmNbottomWidget,	this->subForms[BUTTON_FORM],
	XmNleftOffset,		0,
	XmNrightOffset,		0,
	XmNbottomOffset,	2,
    NULL);

    XtVaSetValues (this->subForms[MAIN_FORM], 
	XmNbottomAttachment, XmATTACH_WIDGET, 
	XmNbottomWidget, this->subForms[BUTTON_FORM], 
    NULL);


    //
    // Manage at most 2 forms.  Choose the forms based on parameters which
    // have values.
    //
    int managed_forms = 0;

    if ((managed_forms<2) && (this->image->isAutoAxesLabelsSet())) {
	XmToggleButtonSetState (this->notebookToggles[LABEL_INPUTS], True, True);
	managed_forms++;
    }
    if ((managed_forms<2) && 
	( (this->image->isAutoAxesFontSet()) || 
	  (this->image->isAutoAxesLabelScaleSet()) ) ) {
	XmToggleButtonSetState (this->notebookToggles[FLAG_INPUTS], True, True);
	managed_forms++;
    }

    if ((managed_forms<2) && (this->image->isAutoAxesTicksSet())) {
	XmToggleButtonSetState (this->notebookToggles[TICK_INPUTS], True, True);
	managed_forms++;
    }

    if ((managed_forms<2) && 
	((this->image->isAutoAxesXTickLocsSet()) ||
	 (this->image->isAutoAxesYTickLocsSet()) ||
	 (this->image->isAutoAxesZTickLocsSet())) ) {
	XmToggleButtonSetState (this->notebookToggles[TICK_LABELS], True, True);
	managed_forms++;
    }

    if ((managed_forms<2) && (this->image->isAutoAxesColorsSet())) {
	XmToggleButtonSetState (this->notebookToggles[COLOR_INPUTS], True, True);
	managed_forms++;
    }

    if ((managed_forms<2) && 
	((this->image->isAutoAxesCornersSet()) || this->image->isAutoAxesCursorSet())) {
	XmToggleButtonSetState (this->notebookToggles[CORNER_INPUTS], True, True);
	managed_forms++;
    }

    if (!managed_forms) {
	XmToggleButtonSetState (this->notebookToggles[LABEL_INPUTS], True, True);
	XmToggleButtonSetState (this->notebookToggles[FLAG_INPUTS], True, True);
    }

    return this->toplevelform; 
}

//
// L A B E L S    L A Y O U T           L A B E L S    L A Y O U T
// L A B E L S    L A Y O U T           L A B E L S    L A Y O U T
// L A B E L S    L A Y O U T           L A B E L S    L A Y O U T
//
void AutoAxesDialog::layoutLabels (Widget parent)
{
    char	*ticks_label_name[3];
    int		i;

    ticks_label_name[0] = "X"; 
    ticks_label_name[1] = "Y"; 
    ticks_label_name[2] = "Z"; 

    Widget label = XtVaCreateManagedWidget ("labelsHeader",
	xmLabelWidgetClass,	parent,
	XmNrightAttachment,	XmATTACH_FORM,
	XmNtopAttachment,	XmATTACH_FORM,
	XmNtopOffset,		0,
    NULL);

    XtVaCreateManagedWidget ("sep",
	xmSeparatorWidgetClass,	parent,
	XmNleftAttachment,	XmATTACH_FORM,
	XmNrightAttachment,	XmATTACH_FORM,
	XmNtopAttachment,	XmATTACH_FORM,
	XmNleftOffset,		0,
	XmNrightOffset,		0,
	XmNtopOffset,		12,
    NULL);

    for(i = 0; i < 3; i++) {
	this->axes_label[i] = 
	    XtVaCreateManagedWidget("axes_label",
		xmTextFieldWidgetClass,	parent,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		(i == 0)? label: this->axes_label[i-1],
		XmNuserData,		DIRTY_LABELS,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNleftOffset,		30,
		XmNrightAttachment,	XmATTACH_FORM,
		XmNrightOffset,		10,
	    NULL);

	XtAddCallback(this->axes_label[i], XmNmodifyVerifyCallback, 
	    (XtCallbackProc)AutoAxesDialog_DirtyTextCB, (XtPointer)this);

	XtVaCreateManagedWidget(ticks_label_name[i],
	    xmLabelWidgetClass,		parent,
	    XmNrightAttachment,		XmATTACH_WIDGET,
	    XmNrightWidget,		this->axes_label[i],
	    XmNrightOffset,		8,
	    XmNtopAttachment,		XmATTACH_OPPOSITE_WIDGET,
	    XmNtopWidget,		this->axes_label[i],
	    XmNtopOffset,		0,
	    XmNbottomAttachment,	XmATTACH_OPPOSITE_WIDGET,
	    XmNbottomWidget,		this->axes_label[i],
	    XmNbottomOffset,		0,
	NULL);
    }
}

//
// F L A G S    L A Y O U T           F L A G S    L A Y O U T
// F L A G S    L A Y O U T           F L A G S    L A Y O U T
// F L A G S    L A Y O U T           F L A G S    L A Y O U T
//
void AutoAxesDialog::layoutFlags (Widget parent)
{
Widget pulldown_menu;
Arg wargs[25];
int n;


    XtVaCreateManagedWidget ("flagsHeader",
	xmLabelWidgetClass,	parent,
	XmNrightAttachment,	XmATTACH_FORM,
	XmNtopAttachment,	XmATTACH_FORM,
	XmNtopOffset,		0,
    NULL);

    Widget sep = XtVaCreateManagedWidget ("sep",
	xmSeparatorWidgetClass,	parent,
	XmNleftAttachment,	XmATTACH_FORM,
	XmNrightAttachment,	XmATTACH_FORM,
	XmNtopAttachment,	XmATTACH_FORM,
	XmNleftOffset,		0,
	XmNrightOffset,		0,
	XmNtopOffset,		12,
    NULL);

    //
    // Frame
    //
    Widget frame_label = XtVaCreateManagedWidget("Frame",
	xmLabelWidgetClass,	parent,
	XmNtopAttachment,	XmATTACH_WIDGET,
	XmNtopWidget,		sep,
	XmNtopOffset,		14,
	XmNleftAttachment,	XmATTACH_FORM,
	XmNleftOffset,		44,
	XmNalignment,		XmALIGNMENT_END,
    NULL);

    pulldown_menu = this->createFramePulldown(parent);

    n = 0;
    XtSetArg(wargs[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(wargs[n], XmNtopWidget, sep); n++;
    XtSetArg(wargs[n], XmNtopOffset, 10); n++;
    XtSetArg(wargs[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(wargs[n], XmNleftWidget, frame_label); n++;
    XtSetArg(wargs[n], XmNleftOffset, 0); n++;
    XtSetArg(wargs[n], XmNsubMenuId, pulldown_menu); n++;
    this->frame_om = XmCreateOptionMenu(parent, "om", wargs, n);
    XtManageChild(this->frame_om);

    //
    // Grid
    //
    pulldown_menu = this->createGridPulldown(parent);

    n = 0;
    XtSetArg(wargs[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(wargs[n], XmNtopWidget, this->frame_om); n++;
    XtSetArg(wargs[n], XmNtopOffset, 4); n++;
    XtSetArg(wargs[n], XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
    XtSetArg(wargs[n], XmNleftWidget, this->frame_om); n++;
    XtSetArg(wargs[n], XmNsubMenuId, pulldown_menu); n++;
    this->grid_om = XmCreateOptionMenu(parent, "om", wargs, n);
    XtManageChild(this->grid_om);

    XtVaCreateManagedWidget("Grid",
	xmLabelWidgetClass,	parent,
	XmNrightAttachment,	XmATTACH_WIDGET,
	XmNrightWidget,		this->grid_om,
	XmNrightOffset,		0,
	XmNtopAttachment,	XmATTACH_OPPOSITE_WIDGET,
	XmNtopWidget,		this->grid_om,
	XmNtopOffset,		0,
	XmNbottomAttachment,	XmATTACH_OPPOSITE_WIDGET,
	XmNbottomWidget,	this->grid_om,
	XmNbottomOffset,	0,
	XmNalignment,		XmALIGNMENT_END,
	NULL);

    //
    // Fonts
    //
    this->fontSelection = new TextPopup();    
    this->fontSelection->createTextPopup(parent,
				FontList,sizeof(FontList)/sizeof(FontList[0]),
				AutoAxesDialog::FontChanged, 
				AutoAxesDialog::FontChanged, 
				(void*)this);

    Widget fs = this->fontSelection->getRootWidget();
    XtVaSetValues(fs,
	XmNleftAttachment,	XmATTACH_FORM,
	XmNleftOffset,		260,
	XmNwidth,		172,
	XmNtopAttachment,	XmATTACH_OPPOSITE_WIDGET,
	XmNtopWidget,		this->frame_om,
    NULL);
    this->fontSelection->manage();

    XtVaCreateManagedWidget("Font",
	xmLabelWidgetClass,	parent,
	XmNrightAttachment,	XmATTACH_WIDGET,
	XmNrightWidget,		fs,
	XmNrightOffset,		5,
	XmNtopAttachment,	XmATTACH_OPPOSITE_WIDGET,
	XmNtopWidget,		fs,
	XmNtopOffset,		0,
	XmNbottomAttachment,	XmATTACH_OPPOSITE_WIDGET,
	XmNbottomWidget,	fs,
	XmNbottomOffset,	0,
    NULL);


    //
    // Label Scale
    //
#ifdef PASSDOUBLEVALUE
    XtArgVal    dx_l1, dx_l2, dx_l3, dx_l4;
#endif
    double min = 0.0;
    double max = 1000000.0;
    double value = 1.0;
    double inc = 0.1;
    this->label_scale_stepper = XtVaCreateManagedWidget("scaleStepper",
	xmStepperWidgetClass,	parent,
	XmNtopAttachment,	XmATTACH_OPPOSITE_WIDGET,
	XmNtopWidget,		this->grid_om,
	XmNtopOffset,		3,
	XmNleftAttachment,	XmATTACH_FORM,
	XmNleftOffset,		305,
	XmNalignment,		XmALIGNMENT_CENTER,
	XmNdataType, 		DOUBLE,
	XmNdMinimum, 		DoubleVal(min, dx_l1),
	XmNdMaximum, 		DoubleVal(max, dx_l2),
	XmNdValueStep, 		DoubleVal(inc, dx_l3),
	XmNdecimalPlaces, 	3,
	XmNdValue, 		DoubleVal(value, dx_l4),
	XmNfixedNotation,	False,
	XmNuserData,		DIRTY_SCALE,
	NULL);
    XtAddCallback(this->label_scale_stepper,
		XmNactivateCallback,
		(XtCallbackProc)AutoAxesDialog_DirtyCB,
		(XtPointer)this);

    XtVaCreateManagedWidget("Label scale",
	xmLabelWidgetClass,	parent,
	XmNrightAttachment,	XmATTACH_WIDGET,
	XmNrightWidget,		this->label_scale_stepper,
	XmNrightOffset,		5,
	XmNtopAttachment,	XmATTACH_OPPOSITE_WIDGET,
	XmNtopWidget,		this->label_scale_stepper,
	XmNtopOffset,		0,
	XmNbottomAttachment,	XmATTACH_OPPOSITE_WIDGET,
	XmNbottomWidget,	this->label_scale_stepper,
	XmNbottomOffset,	0,
	NULL);
}

//
// C O L O R S    L A Y O U T           C O L O R S    L A Y O U T
// C O L O R S    L A Y O U T           C O L O R S    L A Y O U T
// C O L O R S    L A Y O U T           C O L O R S    L A Y O U T
//
void AutoAxesDialog::layoutColors (Widget parent)
{
Arg args[25];
int n;

    XtVaCreateManagedWidget ("colorsHeader",
	xmLabelWidgetClass,	parent,
	XmNrightAttachment,	XmATTACH_FORM,
	XmNtopAttachment,	XmATTACH_FORM,
	XmNtopOffset,		0,
    NULL);

    Widget sep = XtVaCreateManagedWidget ("sep",
	xmSeparatorWidgetClass,	parent,
	XmNleftAttachment,	XmATTACH_FORM,
	XmNrightAttachment,	XmATTACH_FORM,
	XmNtopAttachment,	XmATTACH_FORM,
	XmNleftOffset,		0,
	XmNrightOffset,		0,
	XmNtopOffset,		12,
    NULL);

    //
    // Annotation
    //
    Widget color_label = XtVaCreateManagedWidget("Color",
	xmLabelWidgetClass,	parent,
	XmNtopAttachment,	XmATTACH_WIDGET,
	XmNtopWidget,		sep,
	XmNtopOffset,		40,
	XmNbottomAttachment,	XmATTACH_FORM,
	XmNbottomOffset,	10,
	XmNleftAttachment,	XmATTACH_FORM,
	XmNleftOffset,		5,
	XmNmarginHeight,	7,
	NULL);

    this->grid_color = XtVaCreateManagedWidget("gridColor",
	xmTextFieldWidgetClass,	parent,
	XmNleftAttachment,	XmATTACH_WIDGET,
	XmNleftWidget,		color_label,
	XmNtopAttachment,	XmATTACH_OPPOSITE_WIDGET,
	XmNtopWidget,		color_label,
	XmNbottomAttachment,	XmATTACH_OPPOSITE_WIDGET,
	XmNbottomWidget,	color_label,
	XmNbottomOffset,	0,
	XmNtopOffset,		0,
	XmNleftOffset,		10,
	XmNuserData,		DIRTY_COLORS,
	NULL);

    this->tick_color = XtVaCreateManagedWidget("tickColor",
	xmTextFieldWidgetClass,	parent,
	XmNleftAttachment,	XmATTACH_WIDGET,
	XmNleftWidget,		this->grid_color,
	XmNtopAttachment,	XmATTACH_OPPOSITE_WIDGET,
	XmNtopWidget,		this->grid_color,
	XmNbottomAttachment,	XmATTACH_OPPOSITE_WIDGET,
	XmNbottomWidget,	this->grid_color,
	XmNbottomOffset,	0,
	XmNtopOffset,		0,
	XmNleftOffset,		10,
	XmNuserData,		DIRTY_COLORS,
	NULL);

    this->label_color = XtVaCreateManagedWidget("labelColor",
	xmTextFieldWidgetClass,	parent,
	XmNleftAttachment,	XmATTACH_WIDGET,
	XmNleftWidget,		this->tick_color,
	XmNtopAttachment,	XmATTACH_OPPOSITE_WIDGET,
	XmNtopWidget,		this->tick_color,
	XmNbottomAttachment,	XmATTACH_OPPOSITE_WIDGET,
	XmNbottomWidget,	this->tick_color,
	XmNbottomOffset,	0,
	XmNtopOffset,		0,
	XmNleftOffset,		10,
	XmNuserData,		DIRTY_COLORS,
	NULL);

#if !defined(BG_IS_TEXTPOPUP)
    this->background_color = XtVaCreateManagedWidget("bgColor",
	xmTextFieldWidgetClass,	parent,
	XmNleftAttachment,	XmATTACH_WIDGET,
	XmNleftWidget,		this->label_color,
	XmNtopAttachment,	XmATTACH_OPPOSITE_WIDGET,
	XmNtopWidget,		this->label_color,
	XmNbottomAttachment,	XmATTACH_OPPOSITE_WIDGET,
	XmNbottomWidget,	this->label_color,
	XmNbottomOffset,	0,
	XmNtopOffset,		0,
	XmNleftOffset,		10,
	XmNuserData,		DIRTY_COLORS,
	NULL);
    XtAddCallback(this->background_color,
		XmNactivateCallback,
		(XtCallbackProc)AutoAxesDialog_BgColorCB,
		(XtPointer)this);
    XtAddCallback(this->background_color,
		XmNmodifyVerifyCallback,
		(XtCallbackProc)AutoAxesDialog_DirtyTextCB,
		(XtPointer)this);
#else
    this->bgColor = new TextPopup();    
    this->bgColor->createTextPopup(parent,
				BGColors,sizeof(BGColors)/sizeof(BGColors[0]),
				AutoAxesDialog::BGroundChanged, 
				AutoAxesDialog::BGroundChanged, 
				(void*)this);

    this->background_color = this->bgColor->getRootWidget();
    XtVaSetValues (this->background_color,
	XmNleftAttachment,	XmATTACH_WIDGET,
	XmNleftWidget,		this->label_color,
	XmNtopAttachment,	XmATTACH_OPPOSITE_WIDGET,
	XmNtopWidget,		this->label_color,
	XmNbottomAttachment,	XmATTACH_OPPOSITE_WIDGET,
	XmNbottomWidget,	this->label_color,
	XmNbottomOffset,	0,
	XmNtopOffset,		0,
	XmNleftOffset,		10,
	XmNwidth,		120,
    NULL);
    bgColor->manage();
#endif

    XtAddCallback(this->grid_color,
		XmNmodifyVerifyCallback,
		(XtCallbackProc)AutoAxesDialog_DirtyTextCB,
		(XtPointer)this);
    XtAddCallback(this->tick_color,
		XmNmodifyVerifyCallback,
		(XtCallbackProc)AutoAxesDialog_DirtyTextCB,
		(XtPointer)this);
    XtAddCallback(this->label_color,
		XmNmodifyVerifyCallback,
		(XtCallbackProc)AutoAxesDialog_DirtyTextCB,
		(XtPointer)this);

    Widget bg_label = XtVaCreateManagedWidget("Background",
	xmLabelWidgetClass,	parent,
	XmNleftAttachment,	XmATTACH_OPPOSITE_WIDGET,
	XmNleftWidget,		this->background_color,
	XmNrightAttachment,	XmATTACH_OPPOSITE_WIDGET,
	XmNrightWidget,		this->background_color,
	XmNbottomAttachment,	XmATTACH_WIDGET,
	XmNbottomWidget,	this->background_color,
	XmNleftOffset,		0,
#if defined(BG_IS_TEXTPOPUP)
	XmNrightOffset,		28,
#else
	XmNrightOffset,		0,
#endif
	XmNbottomOffset,	2,
	NULL);

    Widget pulldown_menu = this->createBackgroundPulldown(parent);
    n = 0;
    XtSetArg(args[n], XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
    XtSetArg(args[n], XmNtopWidget, this->background_color); n++;
    XtSetArg(args[n], XmNtopOffset, 2); n++;
    XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNleftWidget, bg_label); n++;
    XtSetArg(args[n], XmNleftOffset, 2); n++;
    XtSetArg(args[n], XmNsubMenuId, pulldown_menu); n++;
    this->background_om = XmCreateOptionMenu(parent, "om", args, n);
#if !defined(BG_IS_TEXTPOPUP)
    XtManageChild(this->background_om);
#endif
    
    XtVaCreateManagedWidget("Grid",
	xmLabelWidgetClass,	parent,
	XmNleftAttachment,	XmATTACH_OPPOSITE_WIDGET,
	XmNleftWidget,		this->grid_color,
	XmNrightAttachment,	XmATTACH_OPPOSITE_WIDGET,
	XmNrightWidget,		this->grid_color,
	XmNbottomAttachment,	XmATTACH_WIDGET,
	XmNbottomWidget,	this->grid_color,
	XmNleftOffset,		0,
	XmNrightOffset,		0,
	XmNbottomOffset,	2,
	NULL);
    
    XtVaCreateManagedWidget("Ticks",
	xmLabelWidgetClass,	parent,
	XmNleftAttachment,	XmATTACH_OPPOSITE_WIDGET,
	XmNleftWidget,		this->tick_color,
	XmNrightAttachment,	XmATTACH_OPPOSITE_WIDGET,
	XmNrightWidget,		this->tick_color,
	XmNbottomAttachment,	XmATTACH_WIDGET,
	XmNbottomWidget,	this->tick_color,
	XmNleftOffset,		0,
	XmNrightOffset,		0,
	XmNbottomOffset,	2,
	NULL);
    
    XtVaCreateManagedWidget("Labels",
	xmLabelWidgetClass,	parent,
	XmNleftAttachment,	XmATTACH_OPPOSITE_WIDGET,
	XmNleftWidget,		this->label_color,
	XmNrightAttachment,	XmATTACH_OPPOSITE_WIDGET,
	XmNrightWidget,		this->label_color,
	XmNbottomAttachment,	XmATTACH_WIDGET,
	XmNbottomWidget,	this->label_color,
	XmNleftOffset,		0,
	XmNrightOffset,		0,
	XmNbottomOffset,	2,
	NULL);
}


//
// T I C K S    L A Y O U T           T I C K S    L A Y O U T
// T I C K S    L A Y O U T           T I C K S    L A Y O U T
// T I C K S    L A Y O U T           T I C K S    L A Y O U T
//
void AutoAxesDialog::layoutTicks (Widget parent)
{
Arg wargs[25];
int i,n;
char *ticks_label_name[3];

    ticks_label_name[0] = "X"; 
    ticks_label_name[1] = "Y"; 
    ticks_label_name[2] = "Z"; 

    XtVaCreateManagedWidget ("ticksHeader",
	xmLabelWidgetClass,	parent,
	XmNrightAttachment,	XmATTACH_FORM,
	XmNtopAttachment,	XmATTACH_FORM,
	XmNtopOffset,		0,
    NULL);

    Widget sep = XtVaCreateManagedWidget ("sep",
	xmSeparatorWidgetClass,	parent,
	XmNleftAttachment,	XmATTACH_FORM,
	XmNrightAttachment,	XmATTACH_FORM,
	XmNtopAttachment,	XmATTACH_FORM,
	XmNleftOffset,		0,
	XmNrightOffset,		0,
	XmNtopOffset,		12,
    NULL);

    //
    // Ticks
    //
    this->ticks_tb = XtVaCreateManagedWidget("Ticks",
	xmToggleButtonWidgetClass,	parent,
	XmNleftAttachment,	XmATTACH_FORM,
	XmNleftOffset,		5,
	XmNtopOffset, 		30,
	XmNtopAttachment,	XmATTACH_WIDGET,
	XmNtopWidget,		sep,
	XmNshadowThickness,	0,
	XmNuserData,		DIRTY_TICKS,
	NULL);
    XtAddCallback(this->ticks_tb,
		  XmNvalueChangedCallback, 
		  (XtCallbackProc)AutoAxesDialog_TicksCB, 
		  (XtPointer)this);
    XtAddCallback(this->ticks_tb,
		  XmNvalueChangedCallback, 
		  (XtCallbackProc)AutoAxesDialog_DirtyCB, 
		  (XtPointer)this);

    Widget pulldown_menu = this->createTicksPulldown(parent);

    n = 0;
    XtSetArg(wargs[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(wargs[n], XmNtopWidget, sep); n++;
    XtSetArg(wargs[n], XmNtopOffset, 28); n++;
    XtSetArg(wargs[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(wargs[n], XmNleftWidget, this->ticks_tb); n++;
    XtSetArg(wargs[n], XmNleftOffset, 26); n++;
    XtSetArg(wargs[n], XmNsubMenuId, pulldown_menu); n++;
    this->ticks_om = XmCreateOptionMenu(parent, "om", wargs, n);

    XtManageChild(this->ticks_om);

    //
    // Tick direction label
    //
    XtVaCreateManagedWidget ("tickDirection",
	xmLabelWidgetClass,	parent,
	XmNtopAttachment,	XmATTACH_WIDGET,
	XmNtopWidget,		this->ticks_om,
	XmNrightAttachment,	XmATTACH_OPPOSITE_WIDGET,
	XmNrightWidget,		this->ticks_om,
	XmNrightOffset,		0,
	XmNtopOffset,		10,
    NULL);


    for(i = 0; i < 3; i++)
    {
	this->ticks_number[i] = XtVaCreateManagedWidget("ticks_number",
		xmNumberWidgetClass,	parent,
		XmNtopAttachment,	XmATTACH_OPPOSITE_WIDGET,
		XmNtopWidget,		this->ticks_om,
		XmNtopOffset,		4,
		XmNleftAttachment,	XmATTACH_WIDGET,
		XmNleftOffset,		10,
		XmNleftWidget,		(i == 0) ? 
					this->ticks_om : 
					this->ticks_number[i-1],
		XmNrecomputeSize,	False,
		XmNcharPlaces,		5,
		XmNiMinimum,		0,
		XmNuserData,		DIRTY_TICKS,
		NULL);
	XtAddCallback(this->ticks_number[i],
		  XmNactivateCallback, 
		  (XtCallbackProc)AutoAxesDialog_DirtyCB, 
		  (XtPointer)this);

	this->ticks_label[i] = XtVaCreateManagedWidget(ticks_label_name[i],
		xmLabelWidgetClass,	parent,
		XmNbottomAttachment,	XmATTACH_WIDGET,
		XmNbottomWidget,	this->ticks_number[i],
		XmNbottomOffset,	3,
		XmNleftAttachment,	XmATTACH_OPPOSITE_WIDGET,
		XmNleftWidget,		this->ticks_number[i],
		XmNrightAttachment,	XmATTACH_OPPOSITE_WIDGET,
		XmNrightWidget,		this->ticks_number[i],
		XmNrecomputeSize,	False,
		NULL);

	pulldown_menu = this->createTicksDirectionPulldown(parent);
	n = 0;
	XtSetArg(wargs[n], XmNsubMenuId, pulldown_menu); n++;
	XtSetArg(wargs[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(wargs[n], XmNtopWidget, this->ticks_number[i]); n++;
	XtSetArg(wargs[n], XmNtopOffset, 6); n++;
	XtSetArg(wargs[n], XmNrightAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
	XtSetArg(wargs[n], XmNrightWidget, this->ticks_number[i]); n++;
	XtSetArg(wargs[n], XmNrightOffset, -2); n++;
	this->ticks_direction[i] = XmCreateOptionMenu(parent, "tdir_om", wargs, n);
	XtManageChild (this->ticks_direction[i]);
    }

    //
    // Adjust
    //
    pulldown_menu = this->createAdjustPulldown(parent);

    n = 0;
    XtSetArg(wargs[n], XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
    XtSetArg(wargs[n], XmNtopWidget, this->ticks_om); n++;
    XtSetArg(wargs[n], XmNtopOffset, 0); n++;
    XtSetArg(wargs[n], XmNleftAttachment, XmATTACH_FORM); n++;
    XtSetArg(wargs[n], XmNleftOffset, 445); n++;
    XtSetArg(wargs[n], XmNsubMenuId, pulldown_menu); n++;
    this->adjust_om = XmCreateOptionMenu(parent, "om", wargs, n);
    XtManageChild(this->adjust_om);

    XtVaCreateManagedWidget("Adjust",
	xmLabelWidgetClass,	parent,
	XmNrightAttachment,	XmATTACH_WIDGET,
	XmNrightWidget,		this->adjust_om,
	XmNrightOffset,		0,
	XmNtopAttachment,	XmATTACH_OPPOSITE_WIDGET,
	XmNtopWidget,		this->adjust_om,
	XmNtopOffset,		0,
	XmNbottomAttachment,	XmATTACH_OPPOSITE_WIDGET,
	XmNbottomWidget,	this->adjust_om,
	XmNbottomOffset,	0,
	XmNalignment,		XmALIGNMENT_END,
	NULL);
}

//
// C O R N E R S    L A Y O U T           C O R N E R S    L A Y O U T
// C O R N E R S    L A Y O U T           C O R N E R S    L A Y O U T
// C O R N E R S    L A Y O U T           C O R N E R S    L A Y O U T
//
void AutoAxesDialog::layoutCorners (Widget parent)
{
int i;
char *ticks_label_name[3];

    ticks_label_name[0] = "X"; 
    ticks_label_name[1] = "Y"; 
    ticks_label_name[2] = "Z"; 

    XtVaCreateManagedWidget ("cornersHeader",
	xmLabelWidgetClass,	parent,
	XmNrightAttachment,	XmATTACH_FORM,
	XmNtopAttachment,	XmATTACH_FORM,
	XmNtopOffset,		0,
    NULL);

    Widget sep = XtVaCreateManagedWidget ("sep",
	xmSeparatorWidgetClass,	parent,
	XmNleftAttachment,	XmATTACH_FORM,
	XmNrightAttachment,	XmATTACH_FORM,
	XmNtopAttachment,	XmATTACH_FORM,
	XmNleftOffset,		0,
	XmNrightOffset,		0,
	XmNtopOffset,		12,
    NULL);


    this->corners_tb = XtVaCreateManagedWidget("Corners",
	xmToggleButtonWidgetClass,parent,
	XmNleftAttachment,	XmATTACH_FORM,
	XmNleftOffset,		5,
	XmNtopAttachment,	XmATTACH_WIDGET,
	XmNtopWidget,		sep,
	XmNtopOffset,		28,
	XmNshadowThickness,	0,
	XmNuserData,		DIRTY_CORNERS,
	NULL);
    XtAddCallback(this->corners_tb,
		  XmNvalueChangedCallback, 
		  (XtCallbackProc)AutoAxesDialog_CornersCB, 
		  (XtPointer)this);
    XtAddCallback(this->corners_tb,
		  XmNvalueChangedCallback, 
		  (XtCallbackProc)AutoAxesDialog_DirtyCB, 
		  (XtPointer)this);

    Widget min_label = XtVaCreateManagedWidget("Min",
	xmLabelWidgetClass,	parent,
	XmNleftAttachment,	XmATTACH_WIDGET,
	XmNleftWidget,		this->corners_tb,
	XmNleftOffset,		10,
	XmNtopAttachment,	XmATTACH_WIDGET,
	XmNtopWidget,		sep,
	XmNtopOffset,		30,
	NULL);

    XtVaCreateManagedWidget("Max",
	xmLabelWidgetClass,	parent,
	XmNleftAttachment,	XmATTACH_WIDGET,
	XmNleftWidget,		this->corners_tb,
	XmNleftOffset,		10,
	XmNtopAttachment,	XmATTACH_WIDGET,
	XmNtopWidget,		min_label,
	XmNtopOffset,		12,
	NULL);

#ifdef PASSDOUBLEVALUE
    XtArgVal    dx_l1, dx_l2, dx_l3, dx_l4;
#endif
    double inc = 0.1;
    for(i = 0; i < 6; i++)
    {
	this->corners_number[i] = XtVaCreateManagedWidget("cornerStepper",
		xmNumberWidgetClass,	parent,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		(i < 3) ?
					sep :
					this->corners_number[i-3],
		XmNtopOffset,		(i < 3) ? 28 : 10,
		XmNleftAttachment,	XmATTACH_WIDGET,
		XmNleftOffset,		10,
		XmNleftWidget,		(i == 0) || (i == 3) ?
				    	min_label :
				    	this->corners_number[i-1],
		XmNdataType, 		DOUBLE,
		XmNdValueStep, 		DoubleVal(inc, dx_l1),
		XmNdecimalPlaces, 	3,
		XmNfixedNotation,	False,
		XmNuserData,		DIRTY_CORNERS,
		NULL);
	XtAddCallback(this->corners_number[i],
		  XmNactivateCallback, 
		  (XtCallbackProc)AutoAxesDialog_DirtyCB, 
		  (XtPointer)this);

	if(i < 3)
	{
	    XtVaCreateManagedWidget(ticks_label_name[i],
		xmLabelWidgetClass, 	parent,
		XmNbottomAttachment,	XmATTACH_WIDGET,
		XmNbottomWidget,	this->corners_number[i],
		XmNbottomOffset,	3,
		XmNleftAttachment,	XmATTACH_OPPOSITE_WIDGET,
		XmNleftWidget,		this->corners_number[i],
		XmNrightAttachment,	XmATTACH_OPPOSITE_WIDGET,
		XmNrightWidget,		this->corners_number[i],
		XmNrecomputeSize,	False,
		NULL);
	}
    }

    this->cursor_tb = XtVaCreateManagedWidget("Cursor",
	xmToggleButtonWidgetClass,parent,
	XmNleftAttachment,	XmATTACH_FORM,
	XmNleftOffset,		5,
	XmNtopAttachment,	XmATTACH_WIDGET,
	XmNtopWidget,		this->corners_number[5],
	XmNtopOffset,		10,
	XmNshadowThickness,	0,
	XmNuserData,		DIRTY_CURSOR,
	NULL);
    XtAddCallback(this->cursor_tb,
		  XmNvalueChangedCallback, 
		  (XtCallbackProc)AutoAxesDialog_DirtyCB, 
		  (XtPointer)this);
    XtAddCallback(this->cursor_tb,
		  XmNvalueChangedCallback, 
		  (XtCallbackProc)AutoAxesDialog_CursorCB, 
		  (XtPointer)this);

    for(i = 0; i < 3; i++)
    {
	this->cursor_number[i] = XtVaCreateManagedWidget("cursorStepper",
		xmNumberWidgetClass,	parent,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		this->corners_number[5],
		XmNtopOffset,		10,
		XmNleftAttachment,	XmATTACH_WIDGET,
		XmNleftOffset,		10,
		XmNleftWidget,		(i == 0) ?
				    	min_label :
				    	this->corners_number[i-1],
		XmNdataType, 		DOUBLE,
		XmNdValueStep,	 	DoubleVal(inc, dx_l1),
		XmNdecimalPlaces, 	3,
		XmNfixedNotation,	False,
		XmNuserData,		DIRTY_CURSOR,
		NULL);
	XtAddCallback(this->cursor_number[i],
		  XmNactivateCallback, 
		  (XtCallbackProc)AutoAxesDialog_DirtyCB, 
		  (XtPointer)this);
    }
}

//
// T I C K L A B E L S    L A Y O U T           T I C K L A B E L S    L A Y O U T
// T I C K L A B E L S    L A Y O U T           T I C K L A B E L S    L A Y O U T
// T I C K L A B E L S    L A Y O U T           T I C K L A B E L S    L A Y O U T
//
void AutoAxesDialog::layoutTickLabels (Widget parent)
{
Arg args[25];
int n;


    XtVaSetValues (parent, XmNheight, 200, NULL);

    XtVaCreateManagedWidget ("tickLabelsHeader",
	xmLabelWidgetClass,	parent,
	XmNrightAttachment,	XmATTACH_FORM,
	XmNtopAttachment,	XmATTACH_FORM,
	XmNtopOffset,		0,
    NULL);

    Widget sep = XtVaCreateManagedWidget ("sep",
	xmSeparatorWidgetClass,	parent,
	XmNleftAttachment,	XmATTACH_FORM,
	XmNrightAttachment,	XmATTACH_FORM,
	XmNtopAttachment,	XmATTACH_FORM,
	XmNleftOffset,		0,
	XmNrightOffset,		0,
	XmNtopOffset,		12,
    NULL);

    //
    // Create 3 toggle buttons to control {un}managing of the each
    // of the 3 lists.
    //
    Widget xbut = XtVaCreateManagedWidget ("xListToggle",
	xmToggleButtonWidgetClass,	parent,
	XmNleftAttachment,		XmATTACH_FORM,
	XmNleftOffset,			10,
	XmNtopAttachment,		XmATTACH_WIDGET,
	XmNtopWidget,			sep,
	XmNtopOffset,			40,
	XmNset,				False,
	XmNuserData,			XTICK_LIST,
    NULL);
    XtAddCallback (xbut, XmNvalueChangedCallback,
	(XtCallbackProc)AutoAxesDialog_ListToggleCB, (XtPointer)this);

    Widget ybut = XtVaCreateManagedWidget ("yListToggle",
	xmToggleButtonWidgetClass,	parent,
	XmNleftAttachment,		XmATTACH_OPPOSITE_WIDGET,
	XmNleftWidget,			xbut,
	XmNleftOffset,			0,
	XmNrightAttachment,		XmATTACH_OPPOSITE_WIDGET,
	XmNrightWidget,			xbut,
	XmNrightOffset,			0,
	XmNtopAttachment,		XmATTACH_WIDGET,
	XmNtopWidget,			xbut,
	XmNtopOffset,			0,
	XmNset,				False,
	XmNuserData,			YTICK_LIST,
    NULL);
    XtAddCallback (ybut, XmNvalueChangedCallback,
	(XtCallbackProc)AutoAxesDialog_ListToggleCB, (XtPointer)this);

    Widget zbut = XtVaCreateManagedWidget ("zListToggle",
	xmToggleButtonWidgetClass,	parent,
	XmNleftAttachment,		XmATTACH_OPPOSITE_WIDGET,
	XmNleftWidget,			ybut,
	XmNleftOffset,			0,
	XmNrightAttachment,		XmATTACH_OPPOSITE_WIDGET,
	XmNrightWidget,			ybut,
	XmNrightOffset,			0,
	XmNtopAttachment,		XmATTACH_WIDGET,
	XmNtopWidget,			ybut,
	XmNtopOffset,			0,
	XmNset,				False,
	XmNuserData,			ZTICK_LIST,
    NULL);
    XtAddCallback (zbut, XmNvalueChangedCallback,
	(XtCallbackProc)AutoAxesDialog_ListToggleCB, (XtPointer)this);

    //
    // A label above the toggles
    //
    XtVaCreateManagedWidget ("Edit:",
	xmLabelWidgetClass,	parent,
	XmNleftAttachment,	XmATTACH_FORM,
	XmNleftOffset,		10,
	XmNbottomAttachment,	XmATTACH_WIDGET,
	XmNbottomWidget,	xbut,
	XmNbottomOffset,	4,
    NULL);

    TickLabelList *tll;
    tll = this->tickList[XTICK_LIST] = 
	new TickLabelList ("X Axis", AutoAxesDialog::TLLModifyCB, (void*)this);
    tll->createList(parent);
    Widget sw_x = tll->getRootWidget();

    n = 0;
    XtSetArg (args[n], XmNtopAttachment,	XmATTACH_WIDGET); n++;
    XtSetArg (args[n], XmNtopWidget,		sep); n++;
    XtSetArg (args[n], XmNtopOffset,		10); n++;
    XtSetArg (args[n], XmNbottomAttachment,	XmATTACH_FORM); n++;
    XtSetArg (args[n], XmNbottomOffset,		4); n++;
    XtSetValues (sw_x, args, n);
    
    tll = this->tickList[YTICK_LIST] = 
	new TickLabelList ("Y Axis", AutoAxesDialog::TLLModifyCB, (void*)this);
    tll->createList(parent);
    Widget sw_y = tll->getRootWidget();

    n = 0;
    XtSetArg (args[n], XmNtopAttachment,	XmATTACH_WIDGET); n++;
    XtSetArg (args[n], XmNtopWidget,		sep); n++;
    XtSetArg (args[n], XmNtopOffset,		10); n++;
    XtSetArg (args[n], XmNbottomAttachment,	XmATTACH_FORM); n++;
    XtSetArg (args[n], XmNbottomOffset,		4); n++;
    XtSetValues (sw_y, args, n);

    tll = this->tickList[ZTICK_LIST] = 
	new TickLabelList ("Z Axis", AutoAxesDialog::TLLModifyCB, (void*)this);
    tll->createList(parent);
    Widget sw_z = tll->getRootWidget();

    n = 0;
    XtSetArg (args[n], XmNtopAttachment,	XmATTACH_WIDGET); n++;
    XtSetArg (args[n], XmNtopWidget,		sep); n++;
    XtSetArg (args[n], XmNtopOffset,		10); n++;
    XtSetArg (args[n], XmNbottomAttachment,	XmATTACH_FORM); n++;
    XtSetArg (args[n], XmNbottomOffset,		4); n++;
    XtSetValues (sw_z, args, n);

    XmToggleButtonSetState (xbut, True, True);
    XmToggleButtonSetState (ybut, True, True);
}

//
// B U T T O N S    L A Y O U T           B U T T O N S    L A Y O U T
// B U T T O N S    L A Y O U T           B U T T O N S    L A Y O U T
// B U T T O N S    L A Y O U T           B U T T O N S    L A Y O U T
//
void AutoAxesDialog::layoutButtons (Widget parent)
{
    this->okbtn = XtVaCreateManagedWidget("okBut",
	xmPushButtonWidgetClass, parent,
	XmNtopAttachment,	XmATTACH_FORM,
	XmNtopOffset,		6,
        XmNleftAttachment,	XmATTACH_FORM,
        XmNleftOffset,		5,
	XmNuserData,		OK,
        NULL);

    this->cancelbtn = XtVaCreateManagedWidget("cancelBut",
	xmPushButtonWidgetClass, parent,
	XmNtopAttachment,	XmATTACH_OPPOSITE_WIDGET,
	XmNtopWidget,		this->okbtn,
	XmNtopOffset,		0,
        XmNrightAttachment,	XmATTACH_FORM,
        XmNrightOffset,		5,
	XmNuserData,		CANCEL,
        NULL);

    this->applybtn = XtVaCreateManagedWidget("applyBut",
	xmPushButtonWidgetClass, parent,
	XmNtopAttachment,	XmATTACH_OPPOSITE_WIDGET,
	XmNtopWidget,		this->okbtn,
	XmNtopOffset,		0,
        XmNleftAttachment,	XmATTACH_WIDGET,
        XmNleftWidget,		this->okbtn,
        XmNleftOffset,		10,
	XmNuserData,		APPLY,
        NULL);

    this->restorebtn = XtVaCreateManagedWidget("restoreBut",
	xmPushButtonWidgetClass, parent,
	XmNtopAttachment,	XmATTACH_OPPOSITE_WIDGET,
	XmNtopWidget,		this->okbtn,
	XmNtopOffset,		0,
        XmNrightAttachment,	XmATTACH_WIDGET,
        XmNrightWidget,		this->cancelbtn,
        XmNrightOffset,		10,
	XmNuserData,		RESTORE,
        NULL);

    XtAddCallback(this->okbtn, 
		  XmNactivateCallback, 
		  (XtCallbackProc)AutoAxesDialog_PushbuttonCB, 
		  (XtPointer)this);

    XtAddCallback(this->cancelbtn, 
		  XmNactivateCallback, 
		  (XtCallbackProc)AutoAxesDialog_PushbuttonCB, 
		  (XtPointer)this);

    XtAddCallback(this->applybtn, 
		  XmNactivateCallback, 
		  (XtCallbackProc)AutoAxesDialog_PushbuttonCB, 
		  (XtPointer)this);

    XtAddCallback(this->restorebtn, 
		  XmNactivateCallback, 
		  (XtCallbackProc)AutoAxesDialog_PushbuttonCB, 
		  (XtPointer)this);

}

extern "C" void AutoAxesDialog_PushbuttonCB(Widget    widget,
				  XtPointer clientData,
				  XtPointer )
{
    ASSERT(clientData);
    AutoAxesDialog *dialog = (AutoAxesDialog*) clientData;
    XtPointer data;

    XtVaGetValues(widget, XmNuserData, &data, NULL);

    switch((int)(long)data)
    {
      case EXPAND:
	break;
      case RESTORE:
	dialog->update();
	break;
      case OK:
	dialog->installValues();
	dialog->unmanage();
	break;
      case APPLY:
	dialog->installValues();
	break;
      case CANCEL:
	dialog->unmanage();
	break;
      default:
	ASSERT(FALSE);
	break;
    }
}

Widget AutoAxesDialog::createTicksDirectionPulldown(Widget parent)
{
    Widget w;
    int    i;
    char   *name[2];
    Widget pdm = XmCreatePulldownMenu(parent, "pdm", NULL, 0);

    //
    // Don't change these widget names unless you do search/replace 
    // throughout this file.
    //
    name[0] = "In";
    name[1] = "Out";

    Pixmap p_in = AutoAxesDialog::TicksIn;
    Pixmap p_out = AutoAxesDialog::TicksOut;
    Pixmap p_in_g = AutoAxesDialog::TicksInGrey;
    Pixmap p_out_g = AutoAxesDialog::TicksOutGrey;

    for(i = 0; i < 2; i++)
    {
	w = XtVaCreateManagedWidget(name[i],
	    xmPushButtonWidgetClass, pdm, 
	    XmNlabelType,	XmPIXMAP,
	    XmNuserData,	DIRTY_TICKS,
	    XmNlabelPixmap,		(i==0?p_in : p_out),
	    XmNlabelInsensitivePixmap,	(i==0?p_in_g : p_out_g),
	NULL);

	XtAddCallback(w, XmNactivateCallback, (XtCallbackProc)
	    AutoAxesDialog_TicksCB, (XtPointer)this);
	XtAddCallback(w, XmNactivateCallback, (XtCallbackProc)
	    AutoAxesDialog_DirtyCB, (XtPointer)this);
    }
    return pdm;
}
Widget AutoAxesDialog::createTicksPulldown(Widget parent)
{
    Widget w;
    int    i;
    char   *name[3];
    Widget pdm = XmCreatePulldownMenu(parent, "pdm", NULL, 0);

    //
    // Don't change these widget names unless you do search/replace 
    // throughout this file.
    //
    name[0] = "All";
    name[1] = "PerAxis";
    name[2] = "Values";

    for(i = 0; i < 3; i++)
    {
	//
	// Don't mark the dirty bit as DIRTY_TICKS if the user chooses
	// Values from the option menu.
	//
	w = XtVaCreateManagedWidget(name[i],
		    xmPushButtonWidgetClass, pdm, 
		    XmNwidth,		OPTION_MENU_WIDTH,
		    XmNrecomputeSize,	False,
		    XmNuserData,	DIRTY_TICKS,
		    NULL);

	//
	// Use a special callback for the Values button in order to
	// show the Ticks and Labels box when this option menu choice is made.
	//
	if (!strcmp("Values", name[i])) {
	    XtAddCallback(w, XmNactivateCallback, 
		      (XtCallbackProc)AutoAxesDialog_TicksValueCB, 
		      (XtPointer)this);
	}

	XtAddCallback(w, XmNactivateCallback, 
		      (XtCallbackProc)AutoAxesDialog_TicksCB, 
		      (XtPointer)this);
	XtAddCallback(w, XmNactivateCallback, 
		      (XtCallbackProc)AutoAxesDialog_DirtyCB, 
		      (XtPointer)this);
    }
    return pdm;
}

Widget AutoAxesDialog::createFramePulldown(Widget parent)
{
    Widget pdm = XmCreatePulldownMenu(parent, "pdm", NULL, 0);
    Widget w;
    char *name[2];
    int i;

    name[0] = "On";
    name[1] = "Off";
    for(i = 0; i < 2; i++)
    {
	w = XtVaCreateManagedWidget(name[i],
		    xmPushButtonWidgetClass, 	pdm, 
		    XmNwidth,			OPTION_MENU_WIDTH,
		    XmNrecomputeSize,		False,
		    XmNuserData,		DIRTY_FRAME,
		    NULL);
	XtAddCallback(w,
		  XmNactivateCallback, 
		  (XtCallbackProc)AutoAxesDialog_DirtyCB, 
		  (XtPointer)this);
    }

    return pdm;
}
Widget AutoAxesDialog::createGridPulldown(Widget parent)
{
    Widget pdm = XmCreatePulldownMenu(parent, "pdm", NULL, 0);
    Widget w; 

    char *name[2];
    int i;

    name[1] = "On";
    name[0] = "Off";
    for(i = 0; i < 2; i++)
    {
	w = XtVaCreateManagedWidget(name[i],
		xmPushButtonWidgetClass, pdm, 
		XmNwidth,		OPTION_MENU_WIDTH,
		XmNrecomputeSize,	False,
		XmNuserData,		DIRTY_GRID,
		NULL);
	XtAddCallback(w,
		  XmNactivateCallback, 
		  (XtCallbackProc)AutoAxesDialog_DirtyCB, 
		  (XtPointer)this);
    }

    return pdm;
}
Widget AutoAxesDialog::createBackgroundPulldown(Widget parent)
{
    Widget pdm = XmCreatePulldownMenu(parent, "pdm", NULL, 0);

    Widget w; 
    char *name[2];
    int i;

    name[1] = "clear";
    name[0] = "opaque";
    for(i = 0; i < 2; i++)
    {
	w = XtVaCreateManagedWidget(name[i],
		xmPushButtonWidgetClass, pdm, 
		XmNwidth,		OPTION_MENU_WIDTH,
		XmNrecomputeSize,	False,
		XmNuserData,		DIRTY_COLORS,
		NULL);
	XtAddCallback(w,
		  XmNactivateCallback, 
		  (XtCallbackProc)AutoAxesDialog_DirtyCB, 
		  (XtPointer)this);
	XtAddCallback(w,
		  XmNactivateCallback, 
		  (XtCallbackProc)AutoAxesDialog_BgOMCB, 
		  (XtPointer)this);
    }

    return pdm;
}
Widget AutoAxesDialog::createAdjustPulldown(Widget parent)
{
    Widget pdm = XmCreatePulldownMenu(parent, "pdm", NULL, 0);

    Widget w; 
    char *name[2];
    int i;

    name[1] = "Off";
    name[0] = "On";
    for(i = 0; i < 2; i++)
    {
	w = XtVaCreateManagedWidget(name[i],
		xmPushButtonWidgetClass, pdm, 
		XmNwidth,		OPTION_MENU_WIDTH,
		XmNrecomputeSize,	False,
		XmNuserData,		DIRTY_ADJUST,
		NULL);
	XtAddCallback(w,
		  XmNactivateCallback, 
		  (XtCallbackProc)AutoAxesDialog_DirtyCB, 
		  (XtPointer)this);
    }

    return pdm;
}


AutoAxesDialog::AutoAxesDialog(ImageWindow* image) :
    Dialog("autoAxesDialog", image->getRootWidget())
{
    this->image = image;
    this->saved_bg_color = NULL;
    this->fontSelection = NULL; 
    this->dirtyBits = 0;
    this->deferNotify = FALSE;
    this->tickList[XTICK_LIST] = this->tickList[YTICK_LIST] = 
	this->tickList[ZTICK_LIST] = NULL;
    this->bgColor = NULL;

    int i;
    for (i=0; i<=BUTTON_FORM; i++) this->subForms[i] = NULL;

    if (NOT AutoAxesDialog::ClassInitialized)
    {
	//
	// Prepare bitmaps for the tick direction buttons.
	//
	Widget root = image->getRootWidget();
	Pixel bg = 0;
	int depth = 0;
	Screen* scr;
	XtVaGetValues (root, XmNbackground, &bg, 
	    XmNdepth, &depth, XmNscreen, &scr, NULL);
	Pixel fg = BlackPixelOfScreen(scr);
	AutoAxesDialog::TicksIn = XCreatePixmapFromBitmapData (XtDisplay(root), 
	    XtWindow(root), ticks_in_bits, ticks_in_width, ticks_in_height,
	    fg, bg, depth);
	AutoAxesDialog::TicksOut = XCreatePixmapFromBitmapData (XtDisplay(root), 
	    XtWindow(root), ticks_out_bits, ticks_out_width, ticks_out_height,
	    fg, bg, depth);
	AutoAxesDialog::TicksInGrey = XCreatePixmapFromBitmapData (XtDisplay(root), 
	    XtWindow(root), ticks_in_ins_bits, ticks_in_ins_width, ticks_in_ins_height,
	    fg, bg, depth);
	AutoAxesDialog::TicksOutGrey = XCreatePixmapFromBitmapData (XtDisplay(root), 
	    XtWindow(root),ticks_out_ins_bits, ticks_out_ins_width, ticks_out_ins_height,
	    fg, bg, depth);


        AutoAxesDialog::ClassInitialized = TRUE;
	this->installDefaultResources(theApplication->getRootWidget());
    }

}

AutoAxesDialog::~AutoAxesDialog()
{
    if(this->saved_bg_color)
	delete this->saved_bg_color;
    if (this->fontSelection)
	delete this->fontSelection;
#if defined(BG_IS_TEXTPOPUP)
    if (this->bgColor)
	delete this->bgColor;
#endif

    if (this->tickList[XTICK_LIST]) delete this->tickList[XTICK_LIST];
    if (this->tickList[YTICK_LIST]) delete this->tickList[YTICK_LIST];
    if (this->tickList[ZTICK_LIST]) delete this->tickList[ZTICK_LIST];
}

//
// Install the default resources for this class.
//
void AutoAxesDialog::installDefaultResources(Widget  baseWidget)
{
    this->setDefaultResources(baseWidget, AutoAxesDialog::DefaultResources);
    this->Dialog::installDefaultResources( baseWidget);
}

void AutoAxesDialog::unmanage()
{
    this->setMinWidth(FALSE);
    this->Dialog::unmanage();
}

void AutoAxesDialog::manage()
{
    this->Dialog::manage();
    this->setMinWidth(TRUE);
    this->update();
}

void AutoAxesDialog::update()
{
    this->setAutoAxesDialogTicks();
    this->setAutoAxesDialogFrame();
    this->setAutoAxesDialogGrid();
    this->setAutoAxesDialogAdjust();
    this->setAutoAxesDialogLabels();
    this->setAutoAxesDialogLabelScale();
    this->setAutoAxesDialogFont();
    this->setAutoAxesDialogAnnotationColors();
    this->setAutoAxesDialogCorners();
    this->setAutoAxesDialogCursor();
    this->setAutoAxesDialogEnable();
    this->setAutoAxesDialogXTickLocs();
    this->setAutoAxesDialogYTickLocs();
    this->setAutoAxesDialogZTickLocs();
    this->setAutoAxesDialogXTickLabels();
    this->setAutoAxesDialogYTickLabels();
    this->setAutoAxesDialogZTickLabels();
}

void AutoAxesDialog::setAutoAxesDialogTicks()
{
    int 	i, ival[3];
    
    if (this->deferNotify) return ;
    if (this->image->isAutoAxesTicksSet())
    {
	XmToggleButtonSetState(this->ticks_tb, True, False);
	int count = this->image->getAutoAxesTicksCount();
	if (count == 3)
	{
	    this->image->getAutoAxesTicks (&ival[0], &ival[1], &ival[2]);
	    if ((!this->image->isAutoAxesXTickLocsSet()) &&
		(!this->image->isAutoAxesYTickLocsSet()) &&
		(!this->image->isAutoAxesZTickLocsSet())) 
		SET_OM_NAME(this->ticks_om, "PerAxis");

	    for(i = 0; i < 3; i++) {
		if (ival[i] < 0) {
		    SET_OM_NAME(this->ticks_direction[i], "Out");
		    ival[i] = -ival[i];
		} else {
		    SET_OM_NAME(this->ticks_direction[i], "In");
		}
		XtVaSetValues(this->ticks_number[i], XmNiValue, ival[i], NULL);
	    }
	}
	else
	{
	    this->image->getAutoAxesTicks (&ival[0]);
	    if(count == 1)
	    {
		i = 0;
		if (ival[i] < 0) {
		    SET_OM_NAME(this->ticks_direction[i], "Out");
		    ival[i] = -ival[i];
		} else {
		    SET_OM_NAME(this->ticks_direction[i], "In");
		}
		XtVaSetValues(this->ticks_number[0], XmNiValue, ival[0], NULL);
	    }
	    else	// Default
	    {
		for(i = 0; i < 3; i++) {
		    if (ival[i] < 0) {
			SET_OM_NAME(this->ticks_direction[i], "Out");
			ival[i] = -ival[i];
		    } else {
			SET_OM_NAME(this->ticks_direction[i], "In");
		    }
		    XtVaSetValues(this->ticks_number[i], XmNiValue, 15, NULL);
		}
	    }
	    if ((!this->image->isAutoAxesXTickLocsSet()) &&
		(!this->image->isAutoAxesYTickLocsSet()) &&
		(!this->image->isAutoAxesZTickLocsSet())) 
		SET_OM_NAME(this->ticks_om, "All");
	}
	this->setTicksSensitivity(TRUE);
    }
    else if ((this->image->isAutoAxesXTickLocsSet()) ||
		(this->image->isAutoAxesYTickLocsSet()) ||
		(this->image->isAutoAxesZTickLocsSet())) 
    {
	XmToggleButtonSetState(this->ticks_tb, True, False);
	this->setTicksSensitivity(TRUE);
    }
    else
    {
	XmToggleButtonSetState(this->ticks_tb, False, False);
	this->setTicksSensitivity(FALSE);
    }

    this->dirtyBits&= ~DIRTY_TICKS;
}

void AutoAxesDialog::setAutoAxesDialogXTickLocs()
{ 
int i, size;
double *dvals = NULL;

    if (this->deferNotify) return ;

    Widget wid;
    XtVaGetValues(this->ticks_om, XmNmenuHistory, &wid, NULL);
    
    if (this->image->isAutoAxesXTickLocsSet()) {
	this->image->getAutoAxesXTickLocs(&dvals, &size);
	SET_OM_NAME(this->ticks_om, "Values");
	for (i=0; i<size; i++) 
	    this->tickList[XTICK_LIST]->setNumber(i+1, dvals[i]);
	this->tickList[XTICK_LIST]->setListSize (size);
	if (dvals) delete dvals;
	XmToggleButtonSetState(this->ticks_tb, True, False);
	this->setTicksSensitivity(TRUE);
	this->dirtyBits&= ~DIRTY_XTICK_LOCS;
	this->tickList[XTICK_LIST]->markNumbersClean();
    } else {
	this->image->getAutoAxesXTickLocs(&dvals, &size);
	if (size) {
	    for (i=0; i<size; i++) 
		this->tickList[XTICK_LIST]->setNumber(i+1, dvals[i]);
	    this->tickList[XTICK_LIST]->setListSize (size);
	    if (dvals) delete dvals;
	} else {
	    this->tickList[XTICK_LIST]->clear();
	    this->dirtyBits&= ~DIRTY_XTICK_LOCS;
	    this->tickList[XTICK_LIST]->markNumbersClean();
	}
    }
}

//
// ImageNode::getAutoAxesStringList() returns only the count of items
// in the list if you pass NULL for the receiving data area.
//
void AutoAxesDialog::setAutoAxesDialogXTickLabels()
{ 
String *strs;
int i, size;

    if (this->deferNotify) return ;
    if ((this->image->isAutoAxesXTickLabelsSet()) ||
	(!this->image->isAutoAxesTicksSet())) {
	size = this->image->getAutoAxesXTickLabels(NULL);
	int listCnt = this->tickList[XTICK_LIST]->getTickCount();
	if (size) {
	    strs = new String[size];
	    for (i=0; i<size; i++) strs[i] = NULL;
	    ASSERT(size == this->image->getAutoAxesXTickLabels(strs));
	    for (i=0; i<size; i++) {
		this->tickList[XTICK_LIST]->setText(i+1, strs[i]);
		delete strs[i];
	    }
	    delete strs;
	} 
	for (i=size; i<listCnt; i++) {
	    this->tickList[XTICK_LIST]->setText(i+1, "");
	}
    } 
    this->dirtyBits&= ~DIRTY_XTICK_LABELS;
    this->tickList[XTICK_LIST]->markTextClean();
}

void AutoAxesDialog::setAutoAxesDialogYTickLabels()
{ 
String *strs;
int i, size;

    if (this->deferNotify) return ;
    if ((this->image->isAutoAxesYTickLabelsSet()) ||
	(!this->image->isAutoAxesTicksSet())) {
	size = this->image->getAutoAxesYTickLabels(NULL);
	int listCnt = this->tickList[YTICK_LIST]->getTickCount();
	if (size) {
	    strs = new String[size];
	    for (i=0; i<size; i++) strs[i] = NULL;
	    ASSERT(size == this->image->getAutoAxesYTickLabels(strs));
	    for (i=0; i<size; i++) {
		this->tickList[YTICK_LIST]->setText(i+1, strs[i]);
		delete strs[i];
	    }
	    delete strs;
	} 
	for (i=size; i<listCnt; i++) {
	    this->tickList[YTICK_LIST]->setText(i+1, "");
	}
    } 
    this->dirtyBits&= ~DIRTY_YTICK_LABELS;
    this->tickList[YTICK_LIST]->markTextClean();
}

void AutoAxesDialog::setAutoAxesDialogZTickLabels()
{ 
String *strs;
int i, size;

    if (this->deferNotify) return ;
    if ((this->image->isAutoAxesZTickLabelsSet()) ||
	(!this->image->isAutoAxesTicksSet())) {
	size = this->image->getAutoAxesZTickLabels(NULL);
	int listCnt = this->tickList[ZTICK_LIST]->getTickCount();
	if (size) {
	    strs = new String[size];
	    for (i=0; i<size; i++) strs[i] = NULL;
	    ASSERT(size == this->image->getAutoAxesZTickLabels(strs));
	    for (i=0; i<size; i++) {
		this->tickList[ZTICK_LIST]->setText(i+1, strs[i]);
		delete strs[i];
	    }
	    delete strs;
	} 
	for (i=size; i<listCnt; i++) {
	    this->tickList[ZTICK_LIST]->setText(i+1, "");
	}
    } 
    this->dirtyBits&= ~DIRTY_ZTICK_LABELS;
    this->tickList[ZTICK_LIST]->markTextClean();
}

void AutoAxesDialog::setAutoAxesDialogYTickLocs()
{ 
int i, size;
double *dvals = NULL;

    if (this->deferNotify) return ;

    Widget wid;
    XtVaGetValues(this->ticks_om, XmNmenuHistory, &wid, NULL);

    if (this->image->isAutoAxesYTickLocsSet()) {
	SET_OM_NAME(this->ticks_om, "Values");
	this->image->getAutoAxesYTickLocs(&dvals, &size);
	for (i=0; i<size; i++) 
	    this->tickList[YTICK_LIST]->setNumber(i+1, dvals[i]);
	this->tickList[YTICK_LIST]->setListSize (size);
	if (dvals) delete dvals;
	XmToggleButtonSetState(this->ticks_tb, True, False);
	this->setTicksSensitivity(TRUE);
	this->dirtyBits&= ~DIRTY_YTICK_LOCS;
	this->tickList[YTICK_LIST]->markNumbersClean();
    } else {
	this->image->getAutoAxesYTickLocs(&dvals, &size);
	if (size) {
	    for (i=0; i<size; i++) 
		this->tickList[YTICK_LIST]->setNumber(i+1, dvals[i]);
	    this->tickList[YTICK_LIST]->setListSize (size);
	    if (dvals) delete dvals;
	} else {
	    this->tickList[YTICK_LIST]->clear();
	    this->dirtyBits&= ~DIRTY_YTICK_LOCS;
	    this->tickList[YTICK_LIST]->markNumbersClean();
	}
    }
}


void AutoAxesDialog::setAutoAxesDialogZTickLocs()
{ 
int i, size;
double *dvals = NULL;

    if (this->deferNotify) return ;

    Widget wid;
    XtVaGetValues(this->ticks_om, XmNmenuHistory, &wid, NULL);

    if (this->image->isAutoAxesZTickLocsSet()) {
	SET_OM_NAME(this->ticks_om, "Values");
	this->image->getAutoAxesZTickLocs(&dvals, &size);
	for (i=0; i<size; i++) 
	    this->tickList[ZTICK_LIST]->setNumber(i+1, dvals[i]);
	this->tickList[ZTICK_LIST]->setListSize (size);
	if (dvals) delete dvals;
	XmToggleButtonSetState(this->ticks_tb, True, False);
	this->setTicksSensitivity(TRUE);
	this->dirtyBits&= ~DIRTY_ZTICK_LOCS;
	this->tickList[ZTICK_LIST]->markNumbersClean();
    } else {
	this->image->getAutoAxesZTickLocs(&dvals, &size);
	if (size) {
	    for (i=0; i<size; i++) 
		this->tickList[ZTICK_LIST]->setNumber(i+1, dvals[i]);
	    this->tickList[ZTICK_LIST]->setListSize (size);
	    if (dvals) delete dvals;
	} else {
	    this->tickList[ZTICK_LIST]->clear();
	    this->dirtyBits&= ~DIRTY_ZTICK_LOCS;
	    this->tickList[ZTICK_LIST]->markNumbersClean();
	}
    }
}



void AutoAxesDialog::setAutoAxesDialogFrame()
{
    if (this->deferNotify) return ;
    //
    // Frame, Grid, Adjust flags
    //
    if (this->image->isSetAutoAxesFrame ()) SET_OM_NAME(this->frame_om, "On")
    else SET_OM_NAME(this->frame_om, "Off")
    this->setFrameSensitivity();
    this->dirtyBits&= ~DIRTY_FRAME;
}
	 
void AutoAxesDialog::setAutoAxesDialogGrid()
{
    if (this->deferNotify) return ;
    if (this->image->isSetAutoAxesGrid ()) SET_OM_NAME(this->grid_om, "On")
    else SET_OM_NAME(this->grid_om, "Off")
    this->setGridSensitivity();
    this->dirtyBits&= ~DIRTY_GRID;
}

void AutoAxesDialog::setAutoAxesDialogAdjust()
{
    if (this->deferNotify) return ;
    if (this->image->isSetAutoAxesAdjust ()) SET_OM_NAME(this->adjust_om, "On")
    else SET_OM_NAME(this->adjust_om, "Off")
    this->setAdjustSensitivity();
    this->dirtyBits&= ~DIRTY_ADJUST;
}

void AutoAxesDialog::setAutoAxesDialogLabels()
{
    int i;
    String *sval;
    
    if (this->deferNotify) return ;
    //
    // Labels
    //
    if (!this->image->isAutoAxesLabelsSet ()) {
	for(i = 0; i < 3; i++) {
	    XtVaSetValues (this->axes_label[i], XmNvalue, "", NULL);
	}
    } else {
        int count = this->image->getAutoAxesLabels (NULL);
	sval = new String[count+1];
	for (i=0; i<=count; i++) sval[i] = NULL;
        this->image->getAutoAxesLabels (sval);
	for(i = 0; i < 3; i++) {
	    if (i<count)
		XtVaSetValues (this->axes_label[i], XmNvalue, sval[i], NULL);
	    else
		XtVaSetValues (this->axes_label[i], XmNvalue, "", NULL);
	}
	for (i=0; i<count; i++) if (sval[i]) delete sval[i];
	if (sval) delete sval;
	sval = NUL(String*);
    }
    this->setLabelsSensitivity();
    this->dirtyBits&= ~DIRTY_LABELS;
}

void AutoAxesDialog::setAutoAxesDialogFont()
{
    char  	sv[256];
    
    if (this->deferNotify) return ;
    if (!this->image->isAutoAxesFontSet ()) {
	this->fontSelection->setText("");
    } else {
	this->image->getAutoAxesFont(sv);
	this->fontSelection->setText(sv);
    }
    this->setFontSensitivity();
    this->dirtyBits&= ~DIRTY_FONT;
}

void AutoAxesDialog::setAutoAxesDialogLabelScale()
{
    double 	dval[6];
#ifdef PASSDOUBLEVALUE
    XtArgVal    dx_l;
#endif

    if (this->deferNotify) return ;
    if (!this->image->isAutoAxesLabelScaleSet())
	dval[0] = 1.0;
    else
	dval[0] = this->image->getAutoAxesLabelScale();
    XtVaSetValues(this->label_scale_stepper, XmNdValue,
				DoubleVal(dval[0], dx_l), NULL);
    this->setLabelScaleSensitivity();
    this->dirtyBits&= ~DIRTY_SCALE;
}

void AutoAxesDialog::setAutoAxesDialogAnnotationColors()
{
    int		i, n, ncolors;
    char  	*sval[5], sv0[256], sv1[256], sv2[256], sv3[256];
    char  	*color[5], cr0[256], cr1[256], cr2[256], cr3[256];
    boolean     clear = FALSE;
    
    color[0] = cr0; color[1] = cr1; color[2] = cr2; color[3] = cr3;
    color[4] = 0;

    sval[0] = sv0; sval[1] = sv1; sval[2] = sv2; sval[3] = sv3;
    sval[4] = 0;

    if (this->deferNotify) return ;
    //
    // FIXME: Do you really want to supply default color names here?
    // Wouldn't it be just as good to leave the spaces blank?  It is
    // acceptable to the exec to have any number of strings supplied
    // less than or equal to 4.
    //
    if (!this->image->isAutoAxesColorsSet())
    {
#if defined(BG_IS_TEXTPOPUP)
	this->bgColor->setText("grey30");
#else
	XtVaSetValues(this->background_color, XmNvalue, "grey30", NULL);
#endif
	XtVaSetValues(this->grid_color, XmNvalue, "grey5", NULL);
	XtVaSetValues(this->tick_color, XmNvalue, "yellow", NULL);
	XtVaSetValues(this->label_color, XmNvalue, "white", NULL);
    }
    else
    {
	if (this->image->isAutoAxesAnnotationSet())
	    n = this->image->getAutoAxesAnnotation (sval);
	else {
	    sval[0] = "\"all\"";
	    n = 1;
	}
	
	ncolors = this->image->getAutoAxesColors (color);

	if ((n == ncolors) || (ncolors == 1))
	{
	    char *nextColor;
	    for(i = 0; i < n; i++)
	    {
		nextColor = (ncolors == 1? color[0]: color[i]);
		int len = strlen(nextColor);
		if ((nextColor[0]=='\"')&&(len>1)&&(nextColor[len-1]=='\"')) {
		    int j;
		    nextColor[len-1] = '\0'; len--;
		    for (j=0; j<len; j++) nextColor[j] = nextColor[j+1];
		}

		if(EqualString(sval[i], "\"background\""))
		{
#if defined(BG_IS_TEXTPOPUP)
		    this->bgColor->setText(nextColor);
#else
		    XtVaSetValues (this->background_color, XmNvalue, nextColor, NULL);
#endif
		    if (EqualString(nextColor, "clear")) {
			SET_OM_NAME(this->background_om, "clear")
			clear = TRUE;
		    } else
			SET_OM_NAME(this->background_om, "opaque");
		}
		else if(EqualString(sval[i], "\"grid\""))
		    XtVaSetValues (this->grid_color, XmNvalue, nextColor, NULL);
		else if(EqualString(sval[i], "\"ticks\""))
		    XtVaSetValues (this->tick_color, XmNvalue, nextColor, NULL);
		else if(EqualString(sval[i], "\"labels\""))
		    XtVaSetValues (this->label_color, XmNvalue, nextColor, NULL);
		else if((EqualString(sval[i], "\"all\"")) && (n == 1)) {
		    XtVaSetValues (this->label_color, XmNvalue, nextColor, NULL);
		    XtVaSetValues (this->tick_color, XmNvalue, nextColor, NULL);
		    XtVaSetValues (this->grid_color, XmNvalue, nextColor, NULL);
#if defined(BG_IS_TEXTPOPUP)
		    this->bgColor->setText(nextColor);
#else
		    XtVaSetValues (this->background_color, XmNvalue, nextColor, NULL);
#endif
		}
	    }
	}
    }
    this->setColorsSensitivity(clear);
    this->dirtyBits&= ~DIRTY_COLORS;
}

void AutoAxesDialog::setAutoAxesDialogCorners()
{
    int  	i;
#ifdef PASSDOUBLEVALUE
    XtArgVal	dx_l;
#endif
    double 	dval[6];

    if (this->deferNotify) return ;
    if (!this->image->isAutoAxesCornersSet())
    {
	XmToggleButtonSetState(this->corners_tb, False, False);
	this->setCornersSensitivity(FALSE);
    }
    else
    {
	XmToggleButtonSetState(this->corners_tb, True, False);
	this->image->getAutoAxesCorners (dval);
	for (i=0 ; i<6 ; i++)  {
	    XtVaSetValues(this->corners_number[i], 
		XmNdValue, DoubleVal(dval[i], dx_l), NULL);
        }
	this->setCornersSensitivity(TRUE);
    }
    this->dirtyBits&= ~DIRTY_CORNERS;
}

void AutoAxesDialog::setAutoAxesDialogCursor()
{
    int  	i;
    double 	dval[6];
#ifdef PASSDOUBLEVALUE
    XtArgVal    dx_l;
#endif

    if (this->deferNotify) return ;
    if (!this->image->isAutoAxesCursorSet()) {
	XmToggleButtonSetState(this->cursor_tb, False, False);
	this->setCursorSensitivity (FALSE);
    } else {
	XmToggleButtonSetState(this->cursor_tb, True, False);
	this->image->getAutoAxesCursor (&dval[0], &dval[1], &dval[2]);
	for(i = 0; i < 3; i++) {
	    XtVaSetValues(this->cursor_number[i], XmNdValue, 
		DoubleVal(dval[i], dx_l), NULL);
	}
	this->setCursorSensitivity (TRUE);
    }
    this->dirtyBits&= ~DIRTY_CURSOR;
}

void AutoAxesDialog::setAutoAxesDialogEnable()
{
    if (this->deferNotify) return ;
    int i = this->image->getAutoAxesEnable();
    XmToggleButtonSetState(this->enable_tb, (i == 1), False);
    this->setEnableSensitivity();
    this->dirtyBits&= ~DIRTY_ENABLE;
}

boolean
AutoAxesDialog::isDirty (int member)
{
    switch (member) {
	case DIRTY_XTICK_LOCS:
	    return this->tickList[XTICK_LIST]->isNumberModified();
	case DIRTY_YTICK_LOCS:
	    return this->tickList[YTICK_LIST]->isNumberModified();
	case DIRTY_ZTICK_LOCS:
	    return this->tickList[ZTICK_LIST]->isNumberModified();
	case DIRTY_XTICK_LABELS:
	    return this->tickList[XTICK_LIST]->isTextModified();
	case DIRTY_YTICK_LABELS:
	    return this->tickList[YTICK_LIST]->isTextModified();
	case DIRTY_ZTICK_LABELS:
	    return this->tickList[ZTICK_LIST]->isTextModified();
	default:
	    return (this->dirtyBits & member);
    }
}

boolean
AutoAxesDialog::isDirty()
{
    return (
	this->dirtyBits ||
	this->tickList[XTICK_LIST]->isModified() ||
	this->tickList[YTICK_LIST]->isModified() ||
	this->tickList[ZTICK_LIST]->isModified() 
    );
}

void AutoAxesDialog::installValues()
{
    int        i;
    Boolean    set;
    boolean    first;
    Widget     wid;
    char       value[256];
    char       colors[256];
    int        ival[3];
    double     dval[6];
    char       *str;
    int        ndx;
    DrivenNode *dnode;

    boolean    did_we_send = FALSE;

    if (!this->isDirty()) return ;
    this->deferNotify = TRUE;

    //
    // Defer visual notification, so that update is not
    // called (updating this dialog) for each parameter set.
    dnode = (DrivenNode *)this->image->getAssociatedNode();
    if (dnode->Node::isA(ClassDrivenNode))
	dnode->deferVisualNotification();

    //
    // These dirtyBits are special. Since the widgets are owned by other
    // objects, they have no callbacks here.
    //
    if (this->isDirty(DIRTY_XTICK_LOCS)) this->dirtyBits|= DIRTY_XTICK_LOCS;
    if (this->isDirty(DIRTY_YTICK_LOCS)) this->dirtyBits|= DIRTY_YTICK_LOCS;
    if (this->isDirty(DIRTY_ZTICK_LOCS)) this->dirtyBits|= DIRTY_ZTICK_LOCS;
    if (this->isDirty(DIRTY_XTICK_LABELS)) this->dirtyBits|= DIRTY_XTICK_LABELS;
    if (this->isDirty(DIRTY_YTICK_LABELS)) this->dirtyBits|= DIRTY_YTICK_LABELS;
    if (this->isDirty(DIRTY_ZTICK_LABELS)) this->dirtyBits|= DIRTY_ZTICK_LABELS;

    //
    // Ticks om
    //
    if ((this->dirtyBits&DIRTY_TICKS) && (!this->image->isAutoAxesTicksConnected())) {
	if (( this->image->isAutoAxesXTickLocsSet()) && 
	    (!this->image->isAutoAxesXTickLocsConnected()))
	    this->dirtyBits|= DIRTY_XTICK_LOCS;
	if (( this->image->isAutoAxesYTickLocsSet()) &&
	    (!this->image->isAutoAxesYTickLocsConnected()))
	    this->dirtyBits|= DIRTY_YTICK_LOCS;
	if (( this->image->isAutoAxesZTickLocsSet()) &&
	    (!this->image->isAutoAxesZTickLocsConnected()))
	    this->dirtyBits|= DIRTY_ZTICK_LOCS;
	if (( this->image->isAutoAxesXTickLabelsSet()) &&
	    (!this->image->isAutoAxesXTickLabelsConnected()))
	    this->dirtyBits|= DIRTY_XTICK_LABELS;
	if (( this->image->isAutoAxesYTickLabelsSet()) &&
	    (!this->image->isAutoAxesYTickLabelsConnected()))
	    this->dirtyBits|= DIRTY_YTICK_LABELS;
	if (( this->image->isAutoAxesZTickLabelsSet()) &&
	    (!this->image->isAutoAxesZTickLabelsConnected()))
	    this->dirtyBits|= DIRTY_ZTICK_LABELS;
	if(XmToggleButtonGetState(this->ticks_tb)) {
	    XtVaGetValues(this->ticks_om, XmNmenuHistory, &wid, NULL);
	    if (EqualString(XtName(wid), "All")) {
		XtVaGetValues(this->ticks_number[0], XmNiValue, &ival[0], NULL);
		boolean to_send = (this->dirtyBits == DIRTY_TICKS);
		Widget in_or_out;
		XtVaGetValues(this->ticks_direction[0], XmNmenuHistory, &in_or_out, NULL);
		if (EqualString(XtName(in_or_out), "Out"))
		    this->image->setAutoAxesTicks(-ival[0], to_send);
		else
		    this->image->setAutoAxesTicks(ival[0], to_send);
	    } else {
		for(i = 0; i < 3; i++) {
		    XtVaGetValues(this->ticks_number[i], XmNiValue, &ival[i], NULL);
		    Widget in_or_out;
		    XtVaGetValues(this->ticks_direction[i], 
			XmNmenuHistory, &in_or_out, NULL);
		    if (EqualString(XtName(in_or_out), "Out"))
			ival[i] = -ival[i];
		}
		this->image->setAutoAxesTicks(ival[0], ival[1], ival[2],  
		    (this->dirtyBits == DIRTY_TICKS));
	    } 
	    if (EqualString(XtName(wid), "Values")) {
		if (this->tickList[XTICK_LIST]->getTickCount())
		    this->dirtyBits|= DIRTY_XTICK_LOCS | DIRTY_XTICK_LABELS ;
		if (this->tickList[YTICK_LIST]->getTickCount())
		    this->dirtyBits|= DIRTY_YTICK_LOCS | DIRTY_YTICK_LABELS ;
		if (this->tickList[ZTICK_LIST]->getTickCount())
		    this->dirtyBits|= DIRTY_ZTICK_LOCS | DIRTY_ZTICK_LABELS ;
	    }
	} else {
	    this->image->unsetAutoAxesTicks((this->dirtyBits == DIRTY_TICKS));
	}
	did_we_send|= (this->dirtyBits == DIRTY_TICKS);
    }
    this->dirtyBits&= ~DIRTY_TICKS;

    //
    // Ticks Values
    //
    TickLabelList *tll;
    boolean value_mode = FALSE;
    if (XmToggleButtonGetState(this->ticks_tb)) {
	XtVaGetValues(this->ticks_om, XmNmenuHistory, &wid, NULL);
	if(EqualString(XtName(wid), "Values")) value_mode = TRUE;
    }

    // Tick Labels
    // Must do Labels before locations becuase labels might be empty.  If you
    // set send==TRUE for a parameter which is empty and which was empty, then
    // the sendValues doesn't happen.
    for (i=XTICK_LIST; i<=ZTICK_LIST; i++) {
	tll = this->tickList[i];

	int dirty_flag=0;
	boolean iscon=false;
	switch (i) {
	    case XTICK_LIST: 
		dirty_flag = DIRTY_XTICK_LABELS; 
		iscon = this->image->isAutoAxesXTickLabelsConnected();
		break;
	    case YTICK_LIST: 
		dirty_flag = DIRTY_YTICK_LABELS; 
		iscon = this->image->isAutoAxesYTickLabelsConnected();
		break;
	    case ZTICK_LIST: 
		dirty_flag = DIRTY_ZTICK_LABELS; 
		iscon = this->image->isAutoAxesZTickLabelsConnected();
		break;
	}
	boolean send = (this->dirtyBits == dirty_flag);
	if ((this->dirtyBits&dirty_flag) && (!iscon)) {
	    boolean to_be_set = XmToggleButtonGetState(this->ticks_tb) && value_mode ;

	    if (to_be_set) {
		char *cp = tll->getTickTextString();
		switch (i) {
		    case XTICK_LIST:
			if ((!cp)||(!cp[0])) {
			    this->image->setAutoAxesXTickLabels ("NULL", send);
			    this->image->unsetAutoAxesXTickLabels(send);
			} else
			    this->image->setAutoAxesXTickLabels (cp, send);
			break;
		    case YTICK_LIST:
			if ((!cp)||(!cp[0])) {
			    this->image->setAutoAxesYTickLabels ("NULL", send);
			    this->image->unsetAutoAxesYTickLabels(send);
			} else
			    this->image->setAutoAxesYTickLabels (cp, send);
			break;
		    case ZTICK_LIST:
			if ((!cp)||(!cp[0])) {
			    this->image->setAutoAxesZTickLabels ("NULL", send);
			    this->image->unsetAutoAxesZTickLabels(send);
			} else
			    this->image->setAutoAxesZTickLabels (cp, send);
			break;
		    } // switch
	    } else {
		switch (i) {
		    case XTICK_LIST:
			this->image->unsetAutoAxesXTickLabels(send);
			break;
		    case YTICK_LIST:
			this->image->unsetAutoAxesYTickLabels(send);
			break;
		    case ZTICK_LIST:
			this->image->unsetAutoAxesZTickLabels(send);
			break;
		} // switch
	    }
	    did_we_send|= send;
	}
	this->dirtyBits&= ~dirty_flag;
    }

    // Tick Locations
    for (i=XTICK_LIST; i<=ZTICK_LIST; i++) {
	tll = this->tickList[i];

	int dirty_flag=0;
	boolean iscon=false;
	switch (i) {
	    case XTICK_LIST: 
		dirty_flag = DIRTY_XTICK_LOCS; 
		iscon = this->image->isAutoAxesXTickLocsConnected();
		break;
	    case YTICK_LIST: 
		dirty_flag = DIRTY_YTICK_LOCS; 
		iscon = this->image->isAutoAxesYTickLocsConnected();
		break;
	    case ZTICK_LIST: 
		dirty_flag = DIRTY_ZTICK_LOCS; 
		iscon = this->image->isAutoAxesZTickLocsConnected();
		break;
	}
	boolean send = (this->dirtyBits == dirty_flag);
	if ((this->dirtyBits&dirty_flag) && (!iscon)) {
	    boolean to_be_set = XmToggleButtonGetState(this->ticks_tb) && value_mode ;

	    if (to_be_set) {
		switch (i) {
		    case XTICK_LIST:
			this->image->setAutoAxesXTickLocs (
				tll->getTickNumbers(), 
				tll->getTickCount(), 
				send
			);
			if (!tll->getTickCount())
			    this->image->unsetAutoAxesXTickLocs(send);
			break;
		    case YTICK_LIST:
			this->image->setAutoAxesYTickLocs (
				tll->getTickNumbers(), 
				tll->getTickCount(), 
				send
			);
			if (!tll->getTickCount())
			    this->image->unsetAutoAxesYTickLocs(send);
			break;
		    case ZTICK_LIST:
			this->image->setAutoAxesZTickLocs (
				tll->getTickNumbers(), 
				tll->getTickCount(), 
				send
			);
			if (!tll->getTickCount())
			    this->image->unsetAutoAxesZTickLocs(send);
			break;
		    } // switch
	    } else {
		switch (i) {
		    case XTICK_LIST:
			this->image->unsetAutoAxesXTickLocs(send);
			break;
		    case YTICK_LIST:
			this->image->unsetAutoAxesYTickLocs(send);
			break;
		    case ZTICK_LIST:
			this->image->unsetAutoAxesZTickLocs(send);
			break;
		} // switch
	    }
	    did_we_send|= send;
	}
	this->dirtyBits&= ~dirty_flag;
    }
    this->tickList[XTICK_LIST]->markClean();
    this->tickList[YTICK_LIST]->markClean();
    this->tickList[ZTICK_LIST]->markClean();


    //
    // Frame, Grid, Adjust flags
    //
    if ((this->dirtyBits&DIRTY_FRAME) && (!this->image->isAutoAxesFrameConnected())) {
	XtVaGetValues(this->frame_om, XmNmenuHistory, &wid, NULL);
	int state = EqualString(XtName(wid), "On");
	this->image->setAutoAxesFrame(state, (this->dirtyBits == DIRTY_FRAME));
	did_we_send|= (this->dirtyBits == DIRTY_FRAME);
    }
    this->dirtyBits&= ~DIRTY_FRAME;

    if ((this->dirtyBits&DIRTY_GRID) && (!this->image->isAutoAxesGridConnected())) {
	XtVaGetValues(this->grid_om, XmNmenuHistory, &wid, NULL);
	int state = EqualString(XtName(wid), "On");
	this->image->setAutoAxesGrid(state, (this->dirtyBits == DIRTY_GRID));
	did_we_send|= (this->dirtyBits == DIRTY_GRID);
    }
    this->dirtyBits&= ~DIRTY_GRID;

    if ((this->dirtyBits&DIRTY_ADJUST) && (!this->image->isAutoAxesAdjustConnected())) {
	XtVaGetValues(this->adjust_om, XmNmenuHistory, &wid, NULL);
	int state = EqualString(XtName(wid), "On");
	this->image->setAutoAxesAdjust(state, (this->dirtyBits == DIRTY_ADJUST));
	did_we_send|= (this->dirtyBits == DIRTY_ADJUST);
    }
    this->dirtyBits&= ~DIRTY_ADJUST;

    //
    // Labels
    //
    if ((this->dirtyBits&DIRTY_LABELS) && (!this->image->isAutoAxesLabelsConnected())) {
	value[0] = '\0';
	first = TRUE;
	int vndx;
	int j;
	boolean empty_string_list = TRUE;
	vndx = 0;
	for(i = 0; i < 3; i++) {
	    str = XmTextGetString(this->axes_label[i]);

	    if (str) {
		int len = STRLEN(str);
		if ((len != 0) && (len != 2)) 
		    empty_string_list = FALSE;
		else if ((len == 2) && ((str[0] != '"')||(str[1] != '"')))
		    empty_string_list = FALSE;
	    }
 
	    if(!first) value[vndx++] = ',';

	    // If the first or last characters are not "'s,
	    // place "'s around the string
	    boolean quote_item;
	    quote_item = str[0] != '"' || str[STRLEN(str)-1] != '"';

	    int startndx, endndx;

	    if(quote_item) value[vndx++] = '"';

	    if(quote_item) startndx = vndx;
	    else startndx = vndx + 1;

	    for(j = 0; j < STRLEN(str); j++) value[vndx++] = str[j];

	    if(quote_item) value[vndx++] = '"';

	    endndx = vndx - 1;

	    // Check to make sure all quotes (except the leading and ending quotes)
	    // are escaped
	    for(j = startndx; j < endndx; j++) {
		if((value[j] == '"') && (value[j-1] != '\\')) {
		    WarningMessage("Bad Label string format: %s", str);
		    return;
		}
	    }

	    first = FALSE;
	    delete str;
	}
	value[vndx++] = '\0';
	if (empty_string_list)
	    this->image->unsetAutoAxesLabels(this->dirtyBits == DIRTY_LABELS);
	else
	    this->image->setAutoAxesLabels(value, (this->dirtyBits == DIRTY_LABELS));
    }
    did_we_send|= (this->dirtyBits == DIRTY_LABELS);
    this->dirtyBits&= ~DIRTY_LABELS;

    //
    // Corners
    //
    if ((this->dirtyBits&DIRTY_CORNERS) && (!this->image->isAutoAxesCornersConnected())) {
	if(XmToggleButtonGetState(this->corners_tb)) {
	    for (i=0; i<6; i++) 
		XtVaGetValues (this->corners_number[i],   XmNdValue, &dval[i], NULL);
	    this->image->setAutoAxesCorners (dval, (this->dirtyBits == DIRTY_CORNERS));
	} else {
	    this->image->unsetAutoAxesCorners((this->dirtyBits == DIRTY_CORNERS));
	}
	did_we_send|= (this->dirtyBits == DIRTY_CORNERS);
    }
    this->dirtyBits&= ~DIRTY_CORNERS;

    //
    // Cursor
    //
    if ((this->dirtyBits&DIRTY_CURSOR) && (!this->image->isAutoAxesCursorConnected())) {
	if(XmToggleButtonGetState(this->cursor_tb)) {
	    for(i = 0; i < 3; i++)
		XtVaGetValues(this->cursor_number[i], XmNdValue, &dval[i], NULL);
	    this->image->setAutoAxesCursor (dval[0], dval[1], dval[2], 
		(this->dirtyBits == DIRTY_CURSOR));
	} else {
	    this->image->unsetAutoAxesCursor((this->dirtyBits == DIRTY_CURSOR));
	}
	did_we_send|= (this->dirtyBits == DIRTY_CURSOR);
    }
    this->dirtyBits&= ~DIRTY_CURSOR;

    //
    // Label scale
    //
    if ((this->dirtyBits&DIRTY_SCALE)&&(!this->image->isAutoAxesLabelScaleConnected())) {
	XtVaGetValues(this->label_scale_stepper, XmNdValue, &dval[0], NULL);
	this->image->setAutoAxesLabelScale(dval[0], (this->dirtyBits == DIRTY_SCALE));
	did_we_send|= (this->dirtyBits == DIRTY_SCALE);
    }
    this->dirtyBits&= ~DIRTY_SCALE;

    //
    // Font
    //
    if ((this->dirtyBits&DIRTY_FONT) && (!this->image->isAutoAxesFontConnected())) {
	str = this->fontSelection->getText();
	ndx = 0; SkipWhiteSpace(str, ndx);
	if(STRLEN(&str[ndx]) > 0) {
	    this->image->setAutoAxesFont(&str[ndx], (this->dirtyBits == DIRTY_FONT));
	} else {
	    this->image->unsetAutoAxesFont((this->dirtyBits == DIRTY_FONT));
	}
	XtFree(str);
	did_we_send|= (this->dirtyBits == DIRTY_FONT);
    }
    this->dirtyBits&= ~DIRTY_FONT;

    //
    // Annotation
    //
    if ((this->dirtyBits&DIRTY_COLORS) && (!this->image->isAutoAxesColorsConnected())) {
	value[0] = '\0';
	colors[0] = '\0';
	strcat(value, "{");
	strcat(colors, "{");
	first = TRUE;

#if BG_IS_TEXTPOPUP
	str = this->bgColor->getText();
#else
	str = XmTextGetString(this->background_color);
#endif
	ndx = 0; SkipWhiteSpace(str, ndx);
	if(STRLEN(&str[ndx]) > 0) {
	    strcat(value, "\"background\"");
	    strcat(colors, "\"");
	    strcat(colors, str);
	    strcat(colors, "\"");
	    first = FALSE;
	}
	XtFree(str);

	str = XmTextGetString(this->grid_color);
	ndx = 0; SkipWhiteSpace(str, ndx);
	if(STRLEN(&str[ndx]) > 0) {
	    if(!first) {
		strcat(value, ",");
		strcat(colors, ",");
	    }
	    strcat(value, "\"grid\"");
	    strcat(colors, "\"");
	    strcat(colors, str);
	    strcat(colors, "\"");
	    first = FALSE;
	}
	XtFree(str);

	str = XmTextGetString(this->tick_color);
	ndx = 0; SkipWhiteSpace(str, ndx);
	if(STRLEN(&str[ndx]) > 0) {
	    if(!first) {
		strcat(value, ",");
		strcat(colors, ",");
	    }
	    strcat(value, "\"ticks\"");
	    strcat(colors, "\"");
	    strcat(colors, str);
	    strcat(colors, "\"");
	    first = FALSE;
	}
	XtFree(str);

	str = XmTextGetString(this->label_color);
	ndx = 0; SkipWhiteSpace(str, ndx);
	if(STRLEN(&str[ndx]) > 0) {
	    if(!first) {
		strcat(value, ",");
		strcat(colors, ",");
	    }
	    strcat(value, "\"labels\"");
	    strcat(colors, "\"");
	    strcat(colors, str);
	    strcat(colors, "\""); first = FALSE;
	}
	XtFree(str);

	if(!first) {
	    strcat(value, "}");
	    strcat(colors, "}");
	    this->image->setAutoAxesAnnotation(value, FALSE);
	    this->image->setAutoAxesColors(colors, this->dirtyBits == DIRTY_COLORS);
	} else {
	    this->image->unsetAutoAxesAnnotation(FALSE);
	    this->image->unsetAutoAxesColors(this->dirtyBits == DIRTY_COLORS);
	}
	did_we_send|= (this->dirtyBits == DIRTY_COLORS);
    }
    this->dirtyBits&= ~DIRTY_COLORS;
    

    //
    //
    if ((this->dirtyBits&DIRTY_ENABLE) && (!this->image->isAutoAxesEnableConnected())) {
	XtVaGetValues(this->enable_tb, XmNset, &set, NULL);
	this->image->setAutoAxesEnable((set ? 1 : 0), (this->dirtyBits == DIRTY_ENABLE));
	//if (!set) this->flipAllTabsUp(FALSE);
	did_we_send|= (this->dirtyBits == DIRTY_ENABLE);
    }
    this->dirtyBits&= ~DIRTY_ENABLE;

    //
    // Undefer visual notification.  This will cause update
    // to be called unnecessarily, but oh well.
    if (dnode->Node::isA(ClassDrivenNode))
	dnode->undeferVisualNotification();

    ASSERT (did_we_send);
    this->deferNotify = FALSE;
}

void
AutoAxesDialog::flipAllTabsUp(boolean send)
{
    this->image->unsetAutoAxesEnable(FALSE);

    // Miscellaneous
    if (!this->image->isAutoAxesFrameConnected())
	this->image->unsetAutoAxesFrame(send);
    if (!this->image->isAutoAxesGridConnected())
	this->image->unsetAutoAxesGrid(send);
    if (!this->image->isAutoAxesLabelScaleConnected())
	this->image->unsetAutoAxesLabelScale(send);
    if (!this->image->isAutoAxesFontConnected())
	this->image->unsetAutoAxesFont(send);

    // Ticks
    if (!this->image->isAutoAxesAdjustConnected())
	this->image->unsetAutoAxesAdjust(send);
    if (!this->image->isAutoAxesTicksConnected())
	this->image->unsetAutoAxesTicks(send);

    // Corners / Cursor
    if (!this->image->isAutoAxesCornersConnected())
	this->image->unsetAutoAxesCorners(send);
    if (!this->image->isAutoAxesCursorConnected())
	this->image->unsetAutoAxesCursor(send);

    // Annotation Colors
    if (!this->image->isAutoAxesColorsConnected())
	this->image->unsetAutoAxesColors(send);
    if (!this->image->isAutoAxesColorsConnected())
	this->image->unsetAutoAxesAnnotation(send);

    // Tick Locations
    if (!this->image->isAutoAxesXTickLabelsConnected())
	this->image->unsetAutoAxesXTickLabels(send);
    if (!this->image->isAutoAxesYTickLabelsConnected())
	this->image->unsetAutoAxesYTickLabels(send);
    if (!this->image->isAutoAxesZTickLabelsConnected())
	this->image->unsetAutoAxesZTickLabels(send);

    // Tick Labels
    if (!this->image->isAutoAxesXTickLocsConnected())
	this->image->unsetAutoAxesXTickLocs(send);
    if (!this->image->isAutoAxesYTickLocsConnected())
	this->image->unsetAutoAxesYTickLocs(send);
    if (!this->image->isAutoAxesZTickLocsConnected())
	this->image->unsetAutoAxesZTickLocs(send);

    // Labels
    if (!this->image->isAutoAxesLabelsConnected())
	this->image->unsetAutoAxesLabels(send);
}


// A D J U S T M E N T S    T O    S E N S I T I V I T Y
// A D J U S T M E N T S    T O    S E N S I T I V I T Y
// A D J U S T M E N T S    T O    S E N S I T I V I T Y

//
// splain:  encapsulate setting the sensitivity.  4 params are special.  They are:
// 	ticks...	has a toggle and an option menu involved
// 	corners...	has a toggle involved
// 	cursor...	has a toggle involved
// 	colors...	has a an option menu involved
// They had to be encapsulted because they're touched in many places.  The others
// didn't *need* to be but for the sake of maintainability...
//
void AutoAxesDialog::setFrameSensitivity()
{
    XtSetSensitive (this->frame_om , !this->image->isAutoAxesFrameConnected());
}

void AutoAxesDialog::setGridSensitivity()
{
    XtSetSensitive (this->grid_om , !this->image->isAutoAxesGridConnected());
}

void AutoAxesDialog::setAdjustSensitivity()
{
    XtSetSensitive (this->adjust_om , !this->image->isAutoAxesAdjustConnected());
}

void AutoAxesDialog::setLabelsSensitivity()
{
    if (this->image->isAutoAxesLabelsConnected()) {
	XtSetSensitive (this->axes_label[0] , False);
	XtSetSensitive (this->axes_label[1] , False);
	XtSetSensitive (this->axes_label[2] , False);
    } else {
	XtSetSensitive (this->axes_label[0] , True);
	XtSetSensitive (this->axes_label[1] , True);
	XtSetSensitive (this->axes_label[2] , True);
    }
}

void AutoAxesDialog::setFontSensitivity()
{
    XtSetSensitive (this->fontSelection->getRootWidget() , 
	!this->image->isAutoAxesFontConnected());
}

void AutoAxesDialog::setLabelScaleSensitivity()
{
    XtSetSensitive (this->label_scale_stepper , 
	!this->image->isAutoAxesLabelScaleConnected());
}

void AutoAxesDialog::setEnableSensitivity()
{
    XtSetSensitive (this->enable_tb , !this->image->isAutoAxesEnableConnected());
}


//
// Do the clear/opaque option menu also.  They're really the same param.
//
void AutoAxesDialog::setColorsSensitivity(boolean clear)
{
    boolean isCon = this->image->isAutoAxesColorsConnected();
    Boolean sens = ((!isCon) && (!clear)); 

    //
    // One widget depends on connectedness and on an option menu, 
    // the rest depend only on connectedness.
    //
#if defined(BG_IS_TEXTPOPUP)
    XtSetSensitive (this->background_color, !isCon);
#else
    XtSetSensitive (this->background_color, sens);
#endif

    XtSetSensitive (this->background_om, !isCon);
    XtSetSensitive (this->grid_color,    !isCon);
    XtSetSensitive (this->tick_color,    !isCon);
    XtSetSensitive (this->label_color,   !isCon);
}

void AutoAxesDialog::setCornersSensitivity(boolean button_set)
{
int i;

    boolean isCon = this->image->isAutoAxesCornersConnected();
    Boolean sens = ((!isCon) && (button_set)); 
    XtSetSensitive (this->corners_tb, !isCon);
    for(i = 0; i < 6; i++) 
	XtSetSensitive(this->corners_number[i], sens);
}

void AutoAxesDialog::setCursorSensitivity (boolean button_set)
{
int i;

    boolean isCon = this->image->isAutoAxesCursorConnected();
    Boolean sens = ((!isCon) && (button_set)); 
    XtSetSensitive (this->cursor_tb, !isCon);
    for(i = 0; i < 3; i++)
	XtSetSensitive(this->cursor_number[i], sens);
}


void AutoAxesDialog::setTicksSensitivity(boolean button_set)
{
    //
    // There are 3 different modes.
    //
    Widget w;
    XtVaGetValues(this->ticks_om, XmNmenuHistory, &w, NULL);
    boolean isCon = this->image->isAutoAxesTicksConnected();
    Boolean sens_per = 
	((!isCon) && (button_set) && (EqualString(XtName(w), "PerAxis")));
    Boolean sens_all = 
	((!isCon) && (button_set) && 
	 ((EqualString(XtName(w), "All")) || (EqualString(XtName(w), "PerAxis"))) );
    Boolean sens = 
	((!isCon) && (button_set));

    //
    // Ticks' direction sensitivity
    //
    Boolean sens_dir = (!isCon) && button_set;
    XtSetSensitive (this->ticks_direction[0], sens_dir);
    sens_dir = sens_dir && (!EqualString(XtName(w), "All"));
    XtSetSensitive (this->ticks_direction[1], sens_dir);
    XtSetSensitive (this->ticks_direction[2], sens_dir);

    Boolean sens_x_axis = (this->tickList[XTICK_LIST]->getTickCount()==0 ? True: False);
    Boolean sens_y_axis = (this->tickList[YTICK_LIST]->getTickCount()==0 ? True: False);
    Boolean sens_z_axis = (this->tickList[ZTICK_LIST]->getTickCount()==0 ? True: False);
    sens_x_axis&= (!isCon) && (button_set) && EqualString(XtName(w), "Values");
    sens_y_axis&= (!isCon) && (button_set) && EqualString(XtName(w), "Values");
    sens_z_axis&= (!isCon) && (button_set) && EqualString(XtName(w), "Values");

    //
    // If the param isn't connected via an arc, then these are available to the user.
    //
    XtSetSensitive (this->ticks_tb, 	   !isCon);
    XtSetSensitive (this->ticks_number[0], sens_all || sens_x_axis);
    XtSetSensitive (this->ticks_label[0],  sens_all || sens_x_axis);
    XtSetSensitive (this->ticks_om, 	   sens);

    //
    // If the param isn't connected and things are set up for 3 values instead
    // of 1, then the extra 2 fields are available to the user.
    //
    XtSetSensitive(this->ticks_number[1], sens_per || sens_y_axis);
    XtSetSensitive(this->ticks_label[1], sens_per || sens_y_axis);
    XtSetSensitive(this->ticks_number[2], sens_per || sens_z_axis);
    XtSetSensitive(this->ticks_label[2], sens_per || sens_z_axis);

    //
    // If the params isn't connected and things are set up for typing in
    // tick numbers and labels, then the extra lists are available to the user
    //
    Boolean bset;
    XtVaGetValues (this->ticks_tb, XmNset, &bset, NULL);
    Boolean sens_val = ((bset) && (EqualString(XtName(w), "Values")));
    sens = sens_val && !this->image->isAutoAxesXTickLocsConnected();
    XtSetSensitive (this->tickList[XTICK_LIST]->getRootWidget(), sens);
    sens = sens_val && !this->image->isAutoAxesYTickLocsConnected();
    XtSetSensitive (this->tickList[YTICK_LIST]->getRootWidget(), sens);
    sens = sens_val && !this->image->isAutoAxesZTickLocsConnected();
    XtSetSensitive (this->tickList[ZTICK_LIST]->getRootWidget(), sens);

    //
    // Based on the mode we're using, set the label above the first number widget
    // to indicate that it's for all 3 axes or only the x axis.
    //
    XmString xms = NULL;
    if(EqualString(XtName(w), "All"))
	xms = XmStringCreateSimple("All");
    else 
	xms = XmStringCreateSimple("X");
    if (xms) {
	XtVaSetValues(this->ticks_label[0], XmNlabelString, xms, NULL);
	XmStringFree(xms);
    }
}

void
AutoAxesDialog::TLLModifyCB (TickLabelList *, void* clientData)
{
    AutoAxesDialog *dialog = (AutoAxesDialog*)clientData;
    ASSERT(dialog);
    if (!XmToggleButtonGetState (dialog->enable_tb))
	XmToggleButtonSetState (dialog->enable_tb, True, True);
    Boolean set = XmToggleButtonGetState(dialog->ticks_tb);
    dialog->setTicksSensitivity(set);
}

//
// This isn't used by NoteBookCB because in that routine, minWidth and
// width are managed together to make the dialog box look nice.  This
// routine is used just by managed() and unmanage() and only
// to work around a MWM bug on aix312.  The problem is that if minWidth
// is set for a dialog box which is unmanged, then you can never unset
// minWidth.  So AutoAxesDialog::{un}manage() use this guy and unset
// XmNminWidth before their respective activities.
//
void
AutoAxesDialog::setMinWidth(boolean going_up)
{
#ifndef AIX312_MWM_IS_FIXED
int i;

    Widget diag = this->subForms[0];
    while ((diag) && (!XtIsShell(diag)))
	diag = XtParent(diag);
    ASSERT(diag);

    if (going_up) {
	int minWidth = 0;
	for (i=0; i<=BUTTON_FORM; i++) {
	    if ((XtIsManaged(this->subForms[i])) && 
		(AutoAxesDialog::MinWidths[i] > minWidth))
		minWidth = AutoAxesDialog::MinWidths[i];
	}
	XtVaSetValues (diag, 
	    XmNminWidth, minWidth, 
	    XmNwidth, minWidth, 
	NULL);
    } else {
	XtVaSetValues (diag, 
	    XmNminWidth, AutoAxesDialog::MinWidths[0], 
	    XmNwidth, AutoAxesDialog::MinWidths[0], 
	NULL);
    }
#endif
}

// N O T E B O O K    C A L L B A C K          N O T E B O O K    C A L L B A C K
// N O T E B O O K    C A L L B A C K          N O T E B O O K    C A L L B A C K
// N O T E B O O K    C A L L B A C K          N O T E B O O K    C A L L B A C K
extern "C" {
void AutoAxesDialog_NoteBookCB (Widget tb, XtPointer clientData, XtPointer)
{
Boolean set;
int which_form, i;
Widget widget_to_manage;
AutoAxesDialog* dialog = (AutoAxesDialog*)clientData;

    ASSERT (clientData);
    XtPointer ud;
    XtVaGetValues (tb, XmNset, &set, XmNuserData, &ud, NULL);
    which_form = (int)(long)ud;

    ASSERT(which_form > 0);
    ASSERT(which_form < BUTTON_FORM);
    widget_to_manage = dialog->subForms[which_form];

    //
    // set the minWidth for the dialog according to which forms are managed.
    //
    Widget diag = dialog->subForms[0];
    while ((diag) && (!XtIsShell(diag)))
	diag = XtParent(diag);
    ASSERT(diag);
    int minWidth = 0;

    if (dialog->isManaged()) {
	boolean will_be_managed;
	for (i=0; i<=BUTTON_FORM; i++) {
	    will_be_managed = FALSE;
	    if ((dialog->subForms[i] != widget_to_manage) && 
		(XtIsManaged(dialog->subForms[i]))) {
		will_be_managed = TRUE;
	    } else if ((dialog->subForms[i] == widget_to_manage) && (set))
		will_be_managed = TRUE;

	    if ((will_be_managed) && (AutoAxesDialog::MinWidths[i] > minWidth))
		minWidth = AutoAxesDialog::MinWidths[i];
	}

	//
	// If we're currently larger than the new minWidth then prevent shrinking
	// in the case where our width wasn't imposed because of the minWidth
	// of the guy who is now leaving the screen.  This way, when you unmanage
	// the guy who is making the dialog box be big, the dialog box will get small.
	//
	Dimension width;
	XtVaGetValues (diag, XmNwidth, &width, NULL);
	boolean maintain_width = TRUE;
	if ((!set) && (width == AutoAxesDialog::MinWidths[which_form]))
	    maintain_width = FALSE;

	if ((width >= minWidth) && (maintain_width))
	    XtVaSetValues (diag, XmNminWidth, width, NULL);
	else {
	    XtVaSetValues (diag, XmNminWidth, minWidth, NULL);
#ifndef AIX312_MWM_IS_FIXED
	    XSync (XtDisplay(diag), False);
#endif
	    XtVaSetValues (diag, XmNwidth, minWidth, NULL);
	}
    }


    if (set != XtIsManaged(widget_to_manage)) {
	if (set) {
	    Widget topWidget = NULL;
	    for (i=(which_form-1); i>=0; i--) {
		if (XtIsManaged(dialog->subForms[i])) {
		    topWidget = dialog->subForms[i];
		    break;
		}
	    }
	    ASSERT(topWidget);
	    XtVaSetValues (topWidget, XmNbottomAttachment, XmATTACH_NONE, NULL);

	    Widget bottomWidget = NULL;
	    for (i=(which_form+1); i<=BUTTON_FORM; i++) {
		if (XtIsManaged(dialog->subForms[i])) {
		    bottomWidget = dialog->subForms[i];
		    break;
		}
	    }
	    ASSERT(bottomWidget);

	    if (bottomWidget == dialog->subForms[BUTTON_FORM]) {
		XtVaSetValues (widget_to_manage,
		    XmNtopAttachment, 		XmATTACH_WIDGET,
		    XmNtopWidget,		topWidget,
		    XmNbottomAttachment,	XmATTACH_WIDGET,
		    XmNbottomWidget,		bottomWidget,
		NULL);
	    } else {
		XtVaSetValues (widget_to_manage,
		    XmNtopAttachment, 		XmATTACH_WIDGET,
		    XmNtopWidget,		topWidget,
		    XmNbottomAttachment,	XmATTACH_NONE,
		NULL);

		XtVaSetValues (bottomWidget, 
		    XmNtopAttachment, 		XmATTACH_WIDGET,
		    XmNtopWidget,		widget_to_manage,
		NULL);
	    }

	    XtManageChild (widget_to_manage);

	} else {

	    Widget bottomWidget = NULL;
	    for (i=(which_form+1); i<=BUTTON_FORM; i++) {
		if (XtIsManaged(dialog->subForms[i])) {
		    bottomWidget = dialog->subForms[i];
		    break;
		}
	    }
	    ASSERT(bottomWidget);

	    Widget topWidget = NULL;
	    for (i=(which_form-1); i>=0; i--) {
		if (XtIsManaged(dialog->subForms[i])) {
		    topWidget = dialog->subForms[i];
		    break;
		}
	    }
	    ASSERT(topWidget);

	    if (bottomWidget == dialog->subForms[BUTTON_FORM]) {
		XtUnmanageChild (widget_to_manage);
		XtVaSetValues (topWidget, 
		    XmNbottomAttachment, 	XmATTACH_WIDGET,
		    XmNbottomWidget, 		bottomWidget,
		NULL);
	    } else {
		XtVaSetValues (bottomWidget,
		    XmNtopAttachment,		XmATTACH_WIDGET,
		    XmNtopWidget,		topWidget,
		NULL);
		XtUnmanageChild (widget_to_manage);
	    }
	}
    }
}

//
// If you just toggled up TICKS_LABELS, and if the ticks_om
// is set to have this stuff insensitive, then toggle up TICKS_FORM also
//
void AutoAxesDialog_TickLabelToggleCB (Widget tb, XtPointer clientData, XtPointer)
{
Boolean set;
AutoAxesDialog* dialog = (AutoAxesDialog*)clientData;

    ASSERT (dialog);
    XtVaGetValues (tb, XmNset, &set, NULL);
    if (!set) return ;
    Widget w;
    XtVaGetValues(dialog->ticks_om, XmNmenuHistory, &w, NULL);
    Boolean button_set = XmToggleButtonGetState (dialog->ticks_tb);
    Boolean sens = ((button_set) && (EqualString(XtName(w), "Values")));
    if (!sens)
	XmToggleButtonSetState (dialog->notebookToggles[TICK_INPUTS],True, True);
}

} // end of extern "C"

// T E E N Y   T I N Y    C A L L B A C K S    T E E N Y   T I N Y    C A L L B A C K S
// T E E N Y   T I N Y    C A L L B A C K S    T E E N Y   T I N Y    C A L L B A C K S
// T E E N Y   T I N Y    C A L L B A C K S    T E E N Y   T I N Y    C A L L B A C K S

extern "C" {
//
// Tied to the background color text widget.  Background color doesn't mean background
// color for the image.  They can both be used at the same time.  So don't get
// it confused with the bgcolor param of the image macro.
//
void AutoAxesDialog_BgColorCB (Widget , XtPointer clientData, XtPointer)
{
    ASSERT(clientData);
    AutoAxesDialog *dialog = (AutoAxesDialog*) clientData;
    boolean clear;

#if defined(BG_IS_TEXTPOPUP)
    char *str = dialog->bgColor->getText();
#else
    char *str = XmTextGetString(dialog->background_color);
#endif

    if(!EqualString("clear", str)) {
	SET_OM_NAME(dialog->background_om, "opaque");
	clear = FALSE;
    } else 
	clear = TRUE;

    if(str) delete str;

    dialog->setColorsSensitivity(clear);
}

//
// Tied to each button in the clear/opaque option menu.
//
void AutoAxesDialog_BgOMCB (Widget w, XtPointer clientData, XtPointer)
{
    ASSERT(clientData);
    AutoAxesDialog *dialog = (AutoAxesDialog*) clientData;
    boolean clear = EqualString (XtName(w), "clear");

    //
    // If the option menu says "clear", then put that text into the
    // background color text widget.
    //
    if (clear) {
	if(dialog->saved_bg_color) delete dialog->saved_bg_color;
#if defined(BG_IS_TEXTPOPUP)
	dialog->saved_bg_color = dialog->bgColor->getText();
	dialog->bgColor->setText("clear");
#else
	dialog->saved_bg_color = XmTextGetString(dialog->background_color);
	XtVaSetValues (dialog->background_color, XmNvalue, "clear", NULL);
#endif
    } else if(dialog->saved_bg_color) {
#if defined(BG_IS_TEXTPOPUP)
	dialog->bgColor->setText(dialog->saved_bg_color);
#else
	XtVaSetValues (dialog->background_color, XmNvalue, dialog->saved_bg_color, NULL);
#endif
	delete dialog->saved_bg_color;
	dialog->saved_bg_color = NUL(char*);
    }
    dialog->setColorsSensitivity(clear);
}

//
// Called by toggle button and by option menu
//
void AutoAxesDialog_TicksCB(Widget , XtPointer clientData, XtPointer )
{
    ASSERT(clientData);
    AutoAxesDialog *dialog = (AutoAxesDialog*) clientData;
    Boolean set = XmToggleButtonGetState(dialog->ticks_tb);
    dialog->setTicksSensitivity(set);
}

//
// Called by the Values button in the Ticks option menu
//
void AutoAxesDialog_TicksValueCB(Widget , XtPointer clientData, XtPointer )
{
    ASSERT(clientData);
    AutoAxesDialog *dialog = (AutoAxesDialog*) clientData;
    XmToggleButtonSetState (dialog->notebookToggles[TICK_LABELS], True, True);
    dialog->setAutoAxesDialogXTickLocs();
    dialog->setAutoAxesDialogYTickLocs();
    dialog->setAutoAxesDialogZTickLocs();
    dialog->setAutoAxesDialogXTickLabels();
    dialog->setAutoAxesDialogYTickLabels();
    dialog->setAutoAxesDialogZTickLabels();
}

//
// Tied to the corners param's toggle button which lets the user enable/disable
// the batch enmasse.
//
void AutoAxesDialog_CornersCB(Widget , XtPointer clientData, XtPointer )
{
    ASSERT(clientData);
    AutoAxesDialog *dialog = (AutoAxesDialog*) clientData;
    Boolean set = XmToggleButtonGetState(dialog->corners_tb);
    dialog->setCornersSensitivity(set);
}

//
// Tied to the cursor param's toggle button.
//
void AutoAxesDialog_CursorCB(Widget , XtPointer clientData, XtPointer )
{
    ASSERT(clientData);
    AutoAxesDialog *dialog = (AutoAxesDialog*) clientData;
    Boolean set = XmToggleButtonGetState(dialog->cursor_tb);
    dialog->setCursorSensitivity (set);
}

void AutoAxesDialog_DirtyCB(Widget w, XtPointer clientData, XtPointer )
{
XtPointer flag;
    ASSERT(clientData);
    AutoAxesDialog *dialog = (AutoAxesDialog*) clientData;
    XtVaGetValues (w, XmNuserData, &flag, NULL);
    dialog->dirtyBits |= (int)(long)flag;
    if ((w != dialog->enable_tb) && (!XmToggleButtonGetState (dialog->enable_tb))) {
	if ((XtClass(w) == xmToggleButtonWidgetClass) || 
	    (XtClass(w) == xmToggleButtonGadgetClass)) {
	    Boolean set;
	    XtVaGetValues (w, XmNset, &set, NULL);
	    if (set)
		XmToggleButtonSetState (dialog->enable_tb, True, True);
	} else
	    XmToggleButtonSetState (dialog->enable_tb, True, True);
    }
}
void 
AutoAxesDialog_DirtyTextCB(Widget w, XtPointer clientData, XtPointer cbs)
{
XmTextVerifyCallbackStruct *tvcs = (XmTextVerifyCallbackStruct*)cbs;
XtPointer flag;

    ASSERT(clientData);
    AutoAxesDialog *dialog = (AutoAxesDialog*) clientData;
    XtVaGetValues (w, XmNuserData, &flag, NULL);
    dialog->dirtyBits|= (int)(long)flag;
    tvcs->doit = True;
    if (!XmToggleButtonGetState (dialog->enable_tb))
	XmToggleButtonSetState (dialog->enable_tb, True, True);
}

void AutoAxesDialog_ListToggleCB(Widget w, XtPointer clientData, XtPointer)
{
static int leftPos[3][3] = {
    { 20,  0,  0},
    { 20, 58,  0},
    { 20, 44, 73}
};
static int rightPos[3][3] = {
    { 99,  0,  0},
    { 56, 99,  0},
    { 42, 71, 99}
};
    ASSERT(clientData);
    AutoAxesDialog *dialog = (AutoAxesDialog *)clientData;

    XtPointer ud;
    XtVaGetValues (w, XmNuserData, &ud, NULL);
    int axis = (int)(long)ud;
    ASSERT ((axis >= XTICK_LIST) && (axis <= ZTICK_LIST));

    Boolean set;
    XtVaGetValues (w, XmNset, &set, NULL);
    if (set == XtIsManaged(dialog->tickList[axis]->getRootWidget())) return ;

    if (!set) {
	dialog->tickList[axis]->unmanage();
    }
    
    Boolean xset = 
	((axis==XTICK_LIST && set) || (dialog->tickList[XTICK_LIST]->isManaged()));
    Boolean yset = 
	((axis==YTICK_LIST && set) || (dialog->tickList[YTICK_LIST]->isManaged()));
    Boolean zset = 
	((axis==ZTICK_LIST && set) || (dialog->tickList[ZTICK_LIST]->isManaged()));
    int viewable = 0;
    if (xset) viewable++;
    if (yset) viewable++;
    if (zset) viewable++;
    if (!viewable) return ;

    int list = 0;
    if (xset) {
	XtVaSetValues (dialog->tickList[XTICK_LIST]->getRootWidget(),
	    XmNleftAttachment,	(list?XmATTACH_POSITION: XmATTACH_FORM),
	    XmNleftOffset,	(list?0: 120),
	    XmNleftPosition,	leftPos[viewable-1][list],
	    XmNrightAttachment,	((list+1)!=viewable?XmATTACH_POSITION: XmATTACH_FORM),
	    XmNrightOffset,	((list+1)!=viewable?0: 10),
	    XmNrightPosition,	rightPos[viewable-1][list],
	NULL);
	list++;
    }
    ASSERT (list <= viewable);
    if (yset) {
	XtVaSetValues (dialog->tickList[YTICK_LIST]->getRootWidget(),
	    XmNleftAttachment,	(list?XmATTACH_POSITION: XmATTACH_FORM),
	    XmNleftOffset,	(list?0: 120),
	    XmNleftPosition,	leftPos[viewable-1][list],
	    XmNrightAttachment,	((list+1)!=viewable?XmATTACH_POSITION: XmATTACH_FORM),
	    XmNrightOffset,	((list+1)!=viewable?0: 10),
	    XmNrightPosition,	rightPos[viewable-1][list],
	NULL);
        list++;
    }
    ASSERT (list <= viewable);
    if (zset) {
	XtVaSetValues (dialog->tickList[ZTICK_LIST]->getRootWidget(),
	    XmNleftAttachment,	(list?XmATTACH_POSITION: XmATTACH_FORM),
	    XmNleftOffset,	(list?0: 120),
	    XmNleftPosition,	leftPos[viewable-1][list],
	    XmNrightAttachment,	((list+1)!=viewable?XmATTACH_POSITION: XmATTACH_FORM),
	    XmNrightOffset,	((list+1)!=viewable?0: 10),
	    XmNrightPosition,	rightPos[viewable-1][list],
	NULL);
        list++;
    }
    ASSERT (list == viewable);
	
    //
    // Adjust the size of the dialog:
    //	Widen it if we just added the 3rd box.
    //	Shrink it if we just removed the 3rd box and the width is still 800
    //
    Widget diag = dialog->subForms[0];
    while ((diag) && (!XtIsShell(diag)))
	diag = XtParent(diag);
    ASSERT(diag);
    Dimension width;
    XtVaGetValues (diag, XmNwidth, &width, NULL);
    if (viewable == 3) {
	if (width < 800)
	    XtVaSetValues (diag, XmNwidth, 800, NULL);
    } else if ((viewable == 2)&&(!set)) {
	if (width == 800)
	    XtVaSetValues (diag, XmNwidth, AutoAxesDialog::MinWidths[TICK_LABELS],  NULL);
    }

    if (set) {
	dialog->tickList[axis]->manage();
    }
}

} // end of extern "C"

void AutoAxesDialog::FontChanged(TextPopup *tp, const char *font, void *callData)
{
    AutoAxesDialog* dialog = (AutoAxesDialog*)callData;
    dialog->dirtyBits|= DIRTY_FONT;
    if (!XmToggleButtonGetState (dialog->enable_tb))
	XmToggleButtonSetState (dialog->enable_tb, True, True);
}

void AutoAxesDialog::BGroundChanged(TextPopup *tp, const char *font, void *callData)
{
    AutoAxesDialog* dialog = (AutoAxesDialog*)callData;
    dialog->dirtyBits|= DIRTY_COLORS;
    if (!XmToggleButtonGetState (dialog->enable_tb))
	XmToggleButtonSetState (dialog->enable_tb, True, True);
}

