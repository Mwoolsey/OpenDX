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

#include <limits.h>
#include <Xm/Xm.h>
#include <Xm/DialogS.h>
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/PushB.h>
#include <Xm/ToggleB.h>
#include <Xm/ToggleBG.h>
#include <Xm/RowColumn.h>
#include <Xm/SeparatoG.h>
#include <Xm/Label.h>
#include <Xm/BulletinB.h>

#include "GridDialog.h"
#include "Application.h"
#include "WorkSpace.h"
#include "WorkSpaceInfo.h"
#include "WorkSpaceGrid.h"
#include "../widgets/Number.h"
#include "../widgets/Stepper.h"

boolean GridDialog::ClassInitialized = FALSE;

String  GridDialog::DefaultResources[] = {
	"*cancelButton.labelString:	Cancel",
	"*okButton.labelString:		OK",
	"*okButton.width:		60",
	"*cancelButton.width:		60",
	"*llButton.labelString:		Lower Left",
	"*lrButton.labelString:		Lower Right",
	"*ctButton.labelString:		Center",
	"*ulButton.labelString:		Upper Left",
	"*urButton.labelString:		Upper Right",
	"*leftButton.labelString:	Left",
	"*rightButton.labelString:	Right",
	"*upperButton.labelString:	Upper",
	"*lowerButton.labelString:	Lower",
	"*alignLabel.labelString:	Alignment:",
	"*spaceLabel.labelString:	Grid Spacing",
	"*typeLabel.labelString:	Grid Type",
	"*hLabel.labelString:		Horizontal:",
	"*hLabel.alignment:		XmALIGNMENT_BEGINNING",
	"*vLabel.labelString:		Vertical:",
	"*vLabel.alignment:		XmALIGNMENT_BEGINNING",
	"*noneTButton.labelString:	None",
	"*oneDhTButton.labelString:	1D Horizontal",
	"*oneDvTButton.labelString:	1D Vertical",
	"*twoDTButton.labelString:	2D",
        "*accelerators:              	#augment\n"
          "<Key>Return:                   BulletinBoardReturn()",
        NULL
};

GridDialog::GridDialog(Widget parent, WorkSpace *workSpace) 
                       		: Dialog("gridDialog", parent)
{
    this->workSpace = workSpace;

    if (NOT GridDialog::ClassInitialized)
    {
        GridDialog::ClassInitialized = TRUE;
	this->installDefaultResources(theApplication->getRootWidget());
    }
}
GridDialog::GridDialog(char *name, Widget parent,
			WorkSpace *workSpace) 
                       		: Dialog(name, parent)
{
    this->workSpace = workSpace;
}
GridDialog::~GridDialog()
{

}

//
// Install the default resources for this class.
//
void GridDialog::installDefaultResources(Widget  baseWidget)
{
    this->setDefaultResources(baseWidget, GridDialog::DefaultResources);
    this->Dialog::installDefaultResources( baseWidget);
}


Widget GridDialog::createDialog(Widget parent)
{
    Arg arg[20];
    int n = 0;
    Widget form;

    XtSetArg(arg[n], XmNheight,              400);  n++;
    XtSetArg(arg[n], XmNdialogStyle,         XmDIALOG_APPLICATION_MODAL);  n++;

    form = this->CreateMainForm(parent, this->name, arg, n);

    XtVaSetValues(XtParent(form), 
	XmNtitle,	" Grid...",
	NULL);

    this->cancel = XtVaCreateManagedWidget("cancelButton",
	xmPushButtonWidgetClass, form,
        XmNrightAttachment,   XmATTACH_FORM,
        XmNrightOffset,       5,
        XmNbottomAttachment,  XmATTACH_FORM,
        XmNbottomOffset,      10,
        NULL);

    this->ok = XtVaCreateManagedWidget("okButton",
	xmPushButtonWidgetClass, form,
        XmNleftAttachment,    XmATTACH_FORM,
        XmNleftOffset,        5,
        XmNbottomAttachment,  XmATTACH_FORM,
        XmNbottomOffset,      10,
        NULL);

    this->separator1 = XtVaCreateManagedWidget("separator1",
	xmSeparatorGadgetClass, form,
        XmNleftAttachment,    XmATTACH_FORM,
        XmNleftOffset,        5,
        XmNrightAttachment,   XmATTACH_FORM,
        XmNrightOffset,       5,
        XmNbottomAttachment,  XmATTACH_WIDGET,
        XmNbottomWidget,      this->ok,
        XmNbottomOffset,      10,
        NULL);

    Widget label = XtVaCreateManagedWidget("typeLabel",
	xmLabelWidgetClass,   form,
	XmNtopAttachment,  XmATTACH_FORM,
	XmNtopOffset,         5,
	XmNleftAttachment,    XmATTACH_FORM,
	XmNleftOffset,        5,
	XmNrightAttachment,   XmATTACH_FORM,
	XmNrightOffset,       5,
	XmNalignment,	      XmALIGNMENT_CENTER,
	NULL);

    n = 0;
    XtSetArg(arg[n], XmNleftAttachment, XmATTACH_FORM); n++;
    XtSetArg(arg[n], XmNrightAttachment, XmATTACH_FORM); n++;
    XtSetArg(arg[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(arg[n], XmNtopWidget, label); n++;
    XtSetArg(arg[n], XmNleftOffset, 3); n++;
    XtSetArg(arg[n], XmNrightOffset, 3); n++;
    XtSetArg(arg[n], XmNbottomOffset, 10); n++;
    this->gc_rc = XmCreateRadioBox(form,
	"gridType", arg, n);

    this->noneTButton = XtVaCreateManagedWidget("noneTButton",
	xmToggleButtonWidgetClass, this->gc_rc,
	XmNshadowThickness,		0,
	NULL);

    this->oneDhTButton = XtVaCreateManagedWidget("oneDhTButton",
	xmToggleButtonWidgetClass, this->gc_rc,
	XmNshadowThickness,		0,
	NULL);

    this->oneDvTButton = XtVaCreateManagedWidget("oneDvTButton",
	xmToggleButtonWidgetClass, this->gc_rc,
	XmNshadowThickness,		0,
	NULL);

    this->twoDTButton = XtVaCreateManagedWidget("twoDTButton",
	xmToggleButtonWidgetClass, this->gc_rc,
	XmNshadowThickness,		0,
	NULL);

    XtManageChild(this->gc_rc);

    this->separator3 = XtVaCreateManagedWidget("separator3",
	xmSeparatorGadgetClass, form,
        XmNleftAttachment,    XmATTACH_FORM,
        XmNleftOffset,        5,
        XmNrightAttachment,   XmATTACH_FORM,
        XmNrightOffset,       5,
        XmNtopAttachment,  XmATTACH_WIDGET,
        XmNtopWidget,      this->gc_rc,
        XmNtopOffset,      10,
        NULL);

    this->spaceLabel = XtVaCreateManagedWidget("spaceLabel",
	xmLabelWidgetClass,   form,
	XmNtopAttachment,  XmATTACH_WIDGET,
	XmNtopWidget,      this->separator3,
	XmNtopOffset,      5,
	XmNleftAttachment,    XmATTACH_FORM,
	XmNleftOffset,        5,
	XmNrightAttachment,   XmATTACH_FORM,
	XmNrightOffset,       5,
	XmNalignment,	      XmALIGNMENT_CENTER,
	NULL);

    this->vspacing = XtVaCreateManagedWidget("vNumber",
	xmStepperWidgetClass,form,
        XmNtopAttachment, XmATTACH_WIDGET,
	XmNtopWidget,     this->spaceLabel, 
	XmNtopOffset,     10,
	XmNrightAttachment,  XmATTACH_FORM,
	XmNrightOffset,      5,
	XmNdataType,         INTEGER,
	XmNdigits,           3,
	XmNrecomputeSize,    False,
	XmNiMinimum,         5,
	XmNiMaximum,         100,
	XmNiValue,	     50,
	NULL);

    this->vLabel = XtVaCreateManagedWidget("vLabel",
	xmLabelWidgetClass,form,
	XmNtopAttachment,  XmATTACH_OPPOSITE_WIDGET,
	XmNtopWidget,      this->vspacing,
	XmNtopOffset,      0,
	XmNleftAttachment,    XmATTACH_FORM,
	XmNleftOffset,        5,
	XmNrightAttachment,   XmATTACH_WIDGET,
	XmNrightWidget,       this->vspacing,
	XmNrightOffset,        75,
	XmNshadowThickness,   0,
	XmNset,		      True,
	NULL);

    this->hspacing = XtVaCreateManagedWidget("hNumber",
	xmStepperWidgetClass,form,
        XmNtopAttachment, XmATTACH_WIDGET,
	XmNtopWidget,     this->vspacing, 
	XmNtopOffset,     10,
	XmNrightAttachment,  XmATTACH_FORM,
	XmNrightOffset,      5,
	XmNdataType,         INTEGER,
	XmNdigits,           3,
	XmNrecomputeSize,    False,
	XmNiMinimum,         5,
	XmNiMaximum,         100,
	XmNiValue,	     50,
	NULL);

    this->hLabel = XtVaCreateManagedWidget("hLabel",
	xmLabelWidgetClass,form,
	XmNtopAttachment,  XmATTACH_OPPOSITE_WIDGET,
	XmNtopWidget,      this->hspacing,
	XmNtopOffset,      0,
	XmNleftAttachment,    XmATTACH_FORM,
	XmNleftOffset,        5,
	XmNrightAttachment,   XmATTACH_WIDGET,
	XmNrightWidget,       this->hspacing,
	XmNrightOffset,        75,
	XmNshadowThickness,   0,
	XmNset,		      True,
	NULL);

    this->separator2 = XtVaCreateManagedWidget("separator2",
	xmSeparatorGadgetClass, form,
        XmNleftAttachment,    XmATTACH_FORM,
        XmNleftOffset,        5,
        XmNrightAttachment,   XmATTACH_FORM,
        XmNrightOffset,       5,
        XmNtopAttachment,  XmATTACH_WIDGET,
        XmNtopWidget,      this->hspacing,
        XmNtopOffset,      10,
        NULL);

    this->align_form = XtVaCreateWidget("alignForm",
	xmFormWidgetClass,	form,
	XmNbottomAttachment,	XmATTACH_WIDGET,
	XmNbottomWidget,	separator1,
	XmNbottomOffset,	10,
	XmNrightAttachment,	XmATTACH_FORM,
	XmNleftAttachment,	XmATTACH_FORM,
	XmNleftOffset,		5,
	XmNrightOffset,		5,
	XmNtopAttachment,	XmATTACH_WIDGET,
	XmNtopWidget,	this->separator2,
	XmNtopOffset,		10,
	NULL);

    {
    this->alignLabel = XtVaCreateWidget("alignLabel",
	xmLabelWidgetClass,this->align_form,
	XmNtopAttachment,  XmATTACH_FORM,
	XmNleftAttachment,    XmATTACH_FORM,
	XmNleftOffset,        5,
	NULL);

    this->ulbtn = XtVaCreateWidget("ulButton",
	xmToggleButtonWidgetClass, this->align_form,
        XmNtopAttachment,  XmATTACH_WIDGET,
	XmNtopWidget,      this->alignLabel,
        XmNtopOffset,      10,
        XmNleftAttachment,    XmATTACH_FORM,
        XmNleftOffset,        5,
        XmNindicatorType,     XmONE_OF_MANY,
	XmNshadowThickness,   0,
	XmNset,               False,
	NULL);

    this->urbtn = XtVaCreateWidget("urButton",
	xmToggleButtonWidgetClass, this->align_form,
        XmNtopAttachment,  XmATTACH_WIDGET,
	XmNtopWidget,      this->alignLabel,
        XmNtopOffset,      10,
        XmNrightAttachment,   XmATTACH_FORM,
        XmNrightOffset,       5,
        XmNindicatorType,     XmONE_OF_MANY,
	XmNshadowThickness,   0,
	XmNset,               False,
	NULL);

    this->ctbtn = XtVaCreateWidget("ctButton",
	xmToggleButtonWidgetClass, this->align_form,
        XmNtopAttachment,  XmATTACH_WIDGET,
        XmNtopOffset,      10,
	XmNtopWidget,      this->ulbtn,
        XmNleftAttachment,    XmATTACH_POSITION,
        XmNleftPosition,      50,
        XmNleftOffset,        -35,
        XmNindicatorType,     XmONE_OF_MANY,
	XmNshadowThickness,   0,
	XmNset,               False,
        NULL);

    this->llbtn = XtVaCreateWidget("llButton",
	xmToggleButtonWidgetClass, this->align_form,
        XmNtopAttachment,  XmATTACH_WIDGET,
        XmNtopOffset,      10,
        XmNtopWidget,      this->ctbtn,
        XmNleftAttachment,    XmATTACH_FORM,
        XmNleftOffset,        5,
        XmNindicatorType,     XmONE_OF_MANY,
	XmNshadowThickness,   0,
	XmNset,               False,
	NULL);

    this->lrbtn = XtVaCreateWidget("lrButton",
	xmToggleButtonWidgetClass, this->align_form,
        XmNtopAttachment,  XmATTACH_WIDGET,
        XmNtopOffset,      10,
        XmNtopWidget,      this->ctbtn,
        XmNrightAttachment,   XmATTACH_FORM,
        XmNrightOffset,       5,
        XmNindicatorType,     XmONE_OF_MANY,
	XmNshadowThickness,   0,
	XmNset,               False,
	NULL);

    this->upperbtn = XtVaCreateWidget("upperButton",
	xmToggleButtonWidgetClass, this->align_form,
        XmNtopAttachment,  XmATTACH_WIDGET,
	XmNtopWidget,      this->alignLabel,
        XmNtopOffset,      10,
        XmNleftAttachment,    XmATTACH_POSITION,
        XmNleftPosition,      50,
        XmNleftOffset,        -35,
        XmNindicatorType,     XmONE_OF_MANY,
	XmNshadowThickness,   0,
	XmNset,               False,
        NULL);

    this->lowerbtn = XtVaCreateWidget("lowerButton",
	xmToggleButtonWidgetClass, this->align_form,
        XmNtopAttachment,  XmATTACH_WIDGET,
        XmNtopOffset,      10,
        XmNtopWidget,      this->ctbtn,
        XmNleftAttachment,    XmATTACH_POSITION,
        XmNleftPosition,      50,
        XmNleftOffset,        -35,
        XmNindicatorType,     XmONE_OF_MANY,
	XmNshadowThickness,   0,
	XmNset,               False,
        NULL);

    this->leftbtn = XtVaCreateWidget("leftButton",
	xmToggleButtonWidgetClass, this->align_form,
        XmNtopAttachment,  XmATTACH_OPPOSITE_WIDGET,
        XmNtopOffset,      0,
	XmNtopWidget,      this->ctbtn,
        XmNleftAttachment,    XmATTACH_FORM,
        XmNleftOffset,        5,
        XmNindicatorType,     XmONE_OF_MANY,
	XmNshadowThickness,   0,
	XmNset,               False,
        NULL);

    this->rightbtn = XtVaCreateWidget("rightButton",
	xmToggleButtonWidgetClass, this->align_form,
        XmNtopAttachment,  XmATTACH_OPPOSITE_WIDGET,
        XmNtopOffset,      0,
	XmNtopWidget,      this->ctbtn,
        XmNrightAttachment,   XmATTACH_FORM,
        XmNrightOffset,       5,
        XmNindicatorType,     XmONE_OF_MANY,
	XmNshadowThickness,   0,
	XmNset,               False,
        NULL);
    }

    XtAddCallback(this->noneTButton,
		      XmNvalueChangedCallback,
		      (XtCallbackProc)GridDialog_DimensionCB,
		      (XtPointer)this);

    XtAddCallback(this->oneDvTButton,
		      XmNvalueChangedCallback,
		      (XtCallbackProc)GridDialog_DimensionCB,
		      (XtPointer)this);

    XtAddCallback(this->oneDhTButton,
		      XmNvalueChangedCallback,
		      (XtCallbackProc)GridDialog_DimensionCB,
		      (XtPointer)this);

    XtAddCallback(this->twoDTButton,
		      XmNvalueChangedCallback,
		      (XtCallbackProc)GridDialog_DimensionCB,
		      (XtPointer)this);

    XtAddCallback(this->urbtn,
		      XmNvalueChangedCallback,
		      (XtCallbackProc)GridDialog_AlignmentCB,
		      (XtPointer)this);

    XtAddCallback(this->llbtn,
		      XmNvalueChangedCallback,
		      (XtCallbackProc)GridDialog_AlignmentCB,
		      (XtPointer)this);

    XtAddCallback(this->lrbtn,
		      XmNvalueChangedCallback,
		      (XtCallbackProc)GridDialog_AlignmentCB,
		      (XtPointer)this);

    XtAddCallback(this->ulbtn,
		      XmNvalueChangedCallback,
		      (XtCallbackProc)GridDialog_AlignmentCB,
		      (XtPointer)this);

    XtAddCallback(this->ctbtn,
		      XmNvalueChangedCallback,
		      (XtCallbackProc)GridDialog_AlignmentCB,
		      (XtPointer)this);

    XtAddCallback(this->leftbtn,
		      XmNvalueChangedCallback,
		      (XtCallbackProc)GridDialog_AlignmentCB,
		      (XtPointer)this);

    XtAddCallback(this->rightbtn,
		      XmNvalueChangedCallback,
		      (XtCallbackProc)GridDialog_AlignmentCB,
		      (XtPointer)this);

    XtAddCallback(this->upperbtn,
		      XmNvalueChangedCallback,
		      (XtCallbackProc)GridDialog_AlignmentCB,
		      (XtPointer)this);

    XtAddCallback(this->lowerbtn,
		      XmNvalueChangedCallback,
		      (XtCallbackProc)GridDialog_AlignmentCB,
		      (XtPointer)this);

    XtManageChild(form);
    return form;
}

void GridDialog::manage()
{
    WorkSpaceInfo *wsinfo = this->workSpace->getInfo();
    unsigned int x_align = wsinfo->getGridXAlignment();
    unsigned int y_align = wsinfo->getGridYAlignment();
    int   width, height;

    this->Dialog::manage();
    wsinfo->getGridSpacing(width, height);

    XtVaSetValues(this->hspacing,
		       XmNiValue, width,
		       NULL);
    XtVaSetValues(this->vspacing,
		       XmNiValue, height,
		       NULL);

    XtSetSensitive(this->hspacing, x_align != XmALIGNMENT_NONE);
    XtSetSensitive(this->vspacing, y_align != XmALIGNMENT_NONE);

    if ((!wsinfo->grid.isActive())||
	((x_align == XmALIGNMENT_NONE) && (y_align == XmALIGNMENT_NONE)))
	XmToggleButtonSetState(this->noneTButton, True, True);
    else if (y_align == XmALIGNMENT_NONE)
	XmToggleButtonSetState(this->oneDhTButton, True, True);
    else if (x_align == XmALIGNMENT_NONE)
	XmToggleButtonSetState(this->oneDvTButton, True, True);
    else
	XmToggleButtonSetState(this->twoDTButton, True, True);

    this->resetToggleBtn();
    switch(x_align)
    {
      case XmALIGNMENT_BEGINNING:
	switch(y_align)
	{
	  case XmALIGNMENT_BEGINNING:
	     XtVaSetValues(this->ulbtn, XmNset, True, NULL);
	    break;
	  case XmALIGNMENT_CENTER:
	    break;
	  case XmALIGNMENT_END:
	     XtVaSetValues(this->llbtn, XmNset, True, NULL);
	    break;
	  case XmALIGNMENT_NONE:
	     XtVaSetValues(this->leftbtn, XmNset, True, NULL);
	    break;
	}
	break;
      case XmALIGNMENT_CENTER:
	switch(y_align)
	{
	  case XmALIGNMENT_BEGINNING:
	    break;
	  case XmALIGNMENT_CENTER:
	     XtVaSetValues(this->ctbtn, XmNset, True, NULL);
	    break;
	  case XmALIGNMENT_END:
	    break;
	  case XmALIGNMENT_NONE:
	     XtVaSetValues(this->ctbtn, XmNset, True, NULL);
	    break;
	}
	break;
      case XmALIGNMENT_END:
	switch(y_align)
	{
	  case XmALIGNMENT_BEGINNING:
	     XtVaSetValues(this->urbtn, XmNset, True, NULL);
	    break;
	  case XmALIGNMENT_CENTER:
	    break;
	  case XmALIGNMENT_END:
	     XtVaSetValues(this->lrbtn, XmNset, True, NULL);
	    break;
	  case XmALIGNMENT_NONE:
	     XtVaSetValues(this->rightbtn, XmNset, True, NULL);
	    break;
	}
	break;
      case XmALIGNMENT_NONE:
	switch(y_align)
	{
	  case XmALIGNMENT_BEGINNING:
	     XtVaSetValues(this->upperbtn, XmNset, True, NULL);
	    break;
	  case XmALIGNMENT_CENTER:
	     XtVaSetValues(this->ctbtn, XmNset, True, NULL);
	    break;
	  case XmALIGNMENT_END:
	     XtVaSetValues(this->lowerbtn, XmNset, True, NULL);
	    break;
	  case XmALIGNMENT_NONE:
	    break;
	}
	break;
    }
}

extern "C" void GridDialog_AlignmentCB(Widget    widget,
                                XtPointer clientData,
                                XtPointer callData)
{
    Boolean setting;
    GridDialog *dialog = (GridDialog*) clientData;

     XtVaGetValues(widget, XmNset, &setting, NULL);

     if (setting)
     	 dialog->resetToggleBtn();

     XtVaSetValues(widget, XmNset, True, NULL);
}
extern "C" void GridDialog_DimensionCB(Widget    widget,
                             XtPointer clientData,
                             XtPointer callData)
{
    Boolean right, lower, center;
    GridDialog *dialog = (GridDialog*) clientData;

    if(!XmToggleButtonGetState(widget)) return;

    lower = right = center = False;
    if(XmToggleButtonGetState(dialog->lowerbtn)) {lower  = True;};
    if(XmToggleButtonGetState(dialog->rightbtn)) {right  = True;};
    if(XmToggleButtonGetState(dialog->lrbtn))    {lower  = True; right = True;}
    if(XmToggleButtonGetState(dialog->llbtn))    {lower  = True;}
    if(XmToggleButtonGetState(dialog->urbtn))    {right  = True;};
    if(XmToggleButtonGetState(dialog->ctbtn))    {center = True;};

    dialog->resetToggleBtn();
    if(widget == dialog->noneTButton)
    {
	XtSetSensitive(dialog->hspacing, False);
	XtSetSensitive(dialog->vspacing, False);

	XtVaSetValues(XtParent(dialog->ok),
		XmNresizePolicy, XmRESIZE_NONE,
		NULL);
	
	XtUnmanageChild(dialog->align_form);
	XtUnmanageChild(dialog->separator2);
	
	XtVaSetValues(dialog->hspacing, 
	    XmNbottomAttachment, XmATTACH_WIDGET, 
	    XmNbottomOffset, 10, 
	    XmNbottomWidget, dialog->separator1, 
	    NULL);

	XtVaSetValues(XtParent(dialog->ok),
		XmNresizePolicy, XmRESIZE_ANY,
		NULL);
	XtVaSetValues (XtParent(dialog->ok), XmNheight, 0, NULL);

    }
    else if(widget == dialog->oneDvTButton)
    {
	XtSetSensitive(dialog->hspacing, False);
	XtSetSensitive(dialog->vspacing, True);

	XtVaSetValues(XtParent(dialog->ok),
		XmNresizePolicy, XmRESIZE_NONE,
		NULL);

	XtUnmanageChild(dialog->ulbtn);
	XtUnmanageChild(dialog->urbtn);
	XtUnmanageChild(dialog->ctbtn);
	XtUnmanageChild(dialog->llbtn);
	XtUnmanageChild(dialog->lrbtn);
	XtUnmanageChild(dialog->leftbtn);
	XtUnmanageChild(dialog->rightbtn);

	XtManageChild(dialog->upperbtn);
	XtVaSetValues(dialog->ctbtn, 
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,	dialog->upperbtn,
		NULL);
	XtManageChild(dialog->ctbtn);
	XtManageChild(dialog->lowerbtn);

	XtVaSetValues(dialog->hspacing, 
	    XmNbottomAttachment, XmATTACH_NONE,
	    NULL);
	XtManageChild(dialog->separator2);
	if(lower)
	    XmToggleButtonSetState(dialog->lowerbtn, True, False);
	else if(center)
	    XmToggleButtonSetState(dialog->ctbtn, True, False);
	else
	    XmToggleButtonSetState(dialog->upperbtn, True, False);
	XtManageChild(dialog->align_form);
	XtVaSetValues (dialog->align_form, XmNheight, 0, NULL);

	XtVaSetValues(XtParent(dialog->ok),
		XmNresizePolicy, XmRESIZE_ANY,
		NULL);
	XtVaSetValues (dialog->align_form, XmNheight, 0, NULL);
    }
    else if(widget == dialog->oneDhTButton)
    {

	XtSetSensitive(dialog->hspacing, True);
	XtSetSensitive(dialog->vspacing, False);

	XtVaSetValues(XtParent(dialog->ok),
		XmNresizePolicy, XmRESIZE_NONE,
		NULL);

	XtUnmanageChild(dialog->ulbtn);
	XtUnmanageChild(dialog->urbtn);
	XtUnmanageChild(dialog->ctbtn);
	XtUnmanageChild(dialog->llbtn);
	XtUnmanageChild(dialog->lrbtn);
	XtUnmanageChild(dialog->upperbtn);
	XtUnmanageChild(dialog->lowerbtn);

	XtManageChild(dialog->leftbtn);

	XtVaSetValues(dialog->ctbtn, 
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,	dialog->alignLabel,
		NULL);
	XtManageChild(dialog->ctbtn);
	XtManageChild(dialog->rightbtn);

	XtVaSetValues(dialog->hspacing, 
	    XmNbottomAttachment, XmATTACH_NONE,
	    NULL);
	XtManageChild(dialog->separator2);

	if(right)
	    XmToggleButtonSetState(dialog->rightbtn, True, False);
	else if(center)
	    XmToggleButtonSetState(dialog->ctbtn, True, False);
	else
	    XmToggleButtonSetState(dialog->leftbtn, True, False);

	// make the form widget recalc its height requirement.  In the 1dH case,
	// the align_form widget is getting too tall.  Changing from 1dH to either
	// 2d or 1dV, then makes a mess on the screen.
	XtManageChild(dialog->align_form);

	XtVaSetValues(XtParent(dialog->ok),
		XmNresizePolicy, XmRESIZE_ANY,
		NULL);

	XtVaSetValues (dialog->align_form, XmNheight, 0, NULL);
    }
    else if(widget == dialog->twoDTButton)
    {

	XtSetSensitive(dialog->hspacing, True);
	XtSetSensitive(dialog->vspacing, True);

	XtVaSetValues(XtParent(dialog->ok),
		XmNresizePolicy, XmRESIZE_NONE,
		NULL);

	XtUnmanageChild(dialog->leftbtn);
	XtUnmanageChild(dialog->rightbtn);
	XtUnmanageChild(dialog->upperbtn);
	XtUnmanageChild(dialog->lowerbtn);
	XtUnmanageChild(dialog->ctbtn);

	XtManageChild(dialog->ulbtn);
	XtManageChild(dialog->urbtn);

	XtVaSetValues(dialog->ctbtn, 
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,	dialog->ulbtn,
		NULL);
	XtManageChild(dialog->ctbtn);
	XtManageChild(dialog->llbtn);
	XtManageChild(dialog->lrbtn);

	XtVaSetValues(dialog->hspacing, 
	    XmNbottomAttachment, XmATTACH_NONE,
	    NULL);
	XtManageChild(dialog->separator2);
	
	if(lower)
	    if(right)
		XmToggleButtonSetState(dialog->lrbtn, True, False);
	    else
		XmToggleButtonSetState(dialog->llbtn, True, False);
	else if(center)
	    XmToggleButtonSetState(dialog->ctbtn, True, False);
	else
	    if(right)
		XmToggleButtonSetState(dialog->urbtn, True, False);
	    else
		XmToggleButtonSetState(dialog->ulbtn, True, False);
	XtManageChild(dialog->align_form);

	XtVaSetValues(XtParent(dialog->ok),
		XmNresizePolicy, XmRESIZE_ANY,
		NULL);

	XtVaSetValues (dialog->align_form, XmNheight, 0, NULL);
    }
}


boolean GridDialog::okCallback(Dialog *d)
{
    int   width, height;
    Boolean  upper_left, upper_right, center, lower_left, lower_right;
    Boolean  upper, lower, right, left;
    unsigned int x_align; 
    unsigned int y_align;
    WorkSpaceInfo *wsinfo = this->workSpace->getInfo();
    Boolean align_horizontal;
    Boolean align_vertical;

    align_horizontal = align_vertical = False;
    if(XmToggleButtonGetState(this->twoDTButton))
	align_horizontal = align_vertical = True;
    else if(XmToggleButtonGetState(this->oneDvTButton))
	align_vertical = True;
    else if(XmToggleButtonGetState(this->oneDhTButton))
	align_horizontal = True;

    XtVaGetValues(this->ulbtn,    XmNset, &upper_left, NULL);
    XtVaGetValues(this->urbtn,    XmNset, &upper_right, NULL);
    XtVaGetValues(this->ctbtn,    XmNset, &center, NULL);
    XtVaGetValues(this->llbtn,    XmNset, &lower_left, NULL);
    XtVaGetValues(this->lrbtn,    XmNset, &lower_right, NULL);
    XtVaGetValues(this->rightbtn, XmNset, &right, NULL);
    XtVaGetValues(this->leftbtn,  XmNset, &left, NULL);
    XtVaGetValues(this->upperbtn, XmNset, &upper, NULL);
    XtVaGetValues(this->lowerbtn, XmNset, &lower, NULL);

    if(align_horizontal)
    {
	if (upper_left || lower_left || left) 
	    x_align = XmALIGNMENT_BEGINNING;
	else if (center)
	    x_align = XmALIGNMENT_CENTER;
	else
	    x_align = XmALIGNMENT_END;
    }
    else
	x_align = XmALIGNMENT_NONE;

    if(align_vertical)
    {
	if (upper_left || upper_right || upper) 
	    y_align = XmALIGNMENT_BEGINNING;
	else if (center)
	    y_align = XmALIGNMENT_CENTER;
	else
	    y_align = XmALIGNMENT_END;
    }
    else
	y_align = XmALIGNMENT_NONE;


    wsinfo->setGridAlignment(x_align, y_align);
    
    wsinfo->setGridActive(align_horizontal || align_vertical);
    
    XtVaGetValues(this->hspacing, XmNiValue, &width, NULL);
    XtVaGetValues(this->vspacing, XmNiValue, &height, NULL);
    wsinfo->setGridSpacing(width, height);

    this->workSpace->installInfo(NULL);

    return TRUE;
}

void GridDialog::resetToggleBtn()
{
     XtVaSetValues(this->ulbtn,    XmNset, False, NULL);
     XtVaSetValues(this->urbtn,    XmNset, False, NULL);
     XtVaSetValues(this->llbtn,    XmNset, False, NULL);
     XtVaSetValues(this->lrbtn,    XmNset, False, NULL);
     XtVaSetValues(this->ctbtn,    XmNset, False, NULL);
     XtVaSetValues(this->leftbtn,  XmNset, False, NULL);
     XtVaSetValues(this->rightbtn, XmNset, False, NULL);
     XtVaSetValues(this->upperbtn, XmNset, False, NULL);
     XtVaSetValues(this->lowerbtn, XmNset, False, NULL);
}


