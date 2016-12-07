/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include <X11/StringDefs.h>
#include <Xm/Xm.h>
#include <Xm/DialogS.h>
#include <Xm/List.h>
#include <Xm/Label.h>
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/PushB.h>
#include <Xm/SeparatoG.h>
#include <Xm/Separator.h>
#include <Xm/ScrolledW.h>
#include <Xm/BulletinB.h>

#include "Application.h"
#include "ColormapAddCtlDialog.h"
#include "ColormapEditor.h"
#include "ColormapNode.h"
#include "WarningDialogManager.h"

#include "../widgets/Stepper.h"
#include "../widgets/ColorMapEditor.h"

Boolean ColormapAddCtlDialog::ClassInitialized = FALSE;

String ColormapAddCtlDialog::DefaultResources[] = {
	"*dialogTitle:			Add Control Points",
	"*addButton.labelString:	Add",
	"*closeButton.labelString:	Close",
    	"*accelerators:          	#augment\n"
          "<Key>Return:                   BulletinBoardReturn()",
	NULL
};

ColormapAddCtlDialog::ColormapAddCtlDialog(Widget parent,
					   ColormapEditor* editor) 
                       			   : Dialog("addControlPoints", parent)
{
    this->editor = editor;

    if (NOT ColormapAddCtlDialog::ClassInitialized)
    {
        ColormapAddCtlDialog::ClassInitialized = TRUE;
	this->installDefaultResources(theApplication->getRootWidget());
    }
}

ColormapAddCtlDialog::~ColormapAddCtlDialog()
{

}


//
// Install the default resources for this class.
//
void ColormapAddCtlDialog::installDefaultResources(Widget  baseWidget)
{
    this->setDefaultResources(baseWidget,
				ColormapAddCtlDialog::DefaultResources);
    this->Dialog::installDefaultResources( baseWidget);
}
Widget ColormapAddCtlDialog::createDialog(Widget parent)
{
    Arg arg[10];
    Widget sep;
    double  min = 0.0;
    double  max = 1.0;
    double  inc = 0.05;
    double  min_level = 0.0;
    int     n = 0;

    XtSetArg(arg[n], XmNdialogStyle,	  XmDIALOG_MODELESS); n++;
    XtSetArg(arg[n], XmNwidth,            450);   n++;
    XtSetArg(arg[n], XmNheight,           120);   n++;
    XtSetArg(arg[n], XmNresizePolicy,     XmRESIZE_GROW);   n++;
    XtSetArg(arg[n], XmNautoUnmanage,     False);   n++;

    Widget form = this->CreateMainForm(parent, this->name, arg, n);
#ifdef PASSDOUBLEVALUE
	XtArgVal dx_l1, dx_l2, dx_l3, dx_l4;
#endif

    this->levellabel = 	
	XtVaCreateManagedWidget("dataValueLabel", 
			xmLabelWidgetClass, form,
				XmNtopOffset, 		10,
				XmNleftOffset, 		10,
				XmNbottomOffset, 	10,
				XmNtopAttachment, 	XmATTACH_FORM,
				XmNleftAttachment, 	XmATTACH_FORM,
				XmNalignment, 		XmALIGNMENT_BEGINNING,
				NULL);

    this->levelstepper = 	
	XtVaCreateManagedWidget("lStepper", xmStepperWidgetClass, form,
				XmNtopOffset, 		10,
				XmNleftOffset, 		10,
				XmNrightOffset, 	10,
				XmNbottomOffset, 	10,
				XmNtopAttachment, 	XmATTACH_FORM,
				XmNrightAttachment, 	XmATTACH_FORM,
				XmNdataType, 		DOUBLE,
				XmNdMinimum, 		DoubleVal(min, dx_l1),
				XmNdMaximum, 		DoubleVal(max, dx_l2),
				XmNdValueStep, 		DoubleVal(inc, dx_l3),
				XmNdecimalPlaces, 	5,
				XmNdValue, 		DoubleVal(min_level, dx_l4),
				XmNfixedNotation,	False,
				NULL);

    // pin the labels to the steppers so that longer text isn't munged
    // gresh560
    XtVaSetValues (this->levellabel, XmNrightAttachment, XmATTACH_WIDGET,
	XmNrightWidget, this->levelstepper, XmNrightOffset, 10, NULL);

    this->valuelabel = 	
	XtVaCreateManagedWidget("valueLabel", 
				xmLabelWidgetClass, form,
				XmNtopOffset, 		10,
				XmNleftOffset, 		10,
				XmNbottomOffset, 	10,
				XmNtopAttachment, 	XmATTACH_WIDGET,
				XmNtopWidget, 		this->levelstepper,
				XmNleftAttachment, 	XmATTACH_FORM,
				XmNrightAttachment, 	XmATTACH_OPPOSITE_WIDGET,
				XmNrightWidget, 	this->levellabel,
				XmNalignment, 		XmALIGNMENT_BEGINNING,
				NULL);

    this->valuestepper = 	
	XtVaCreateManagedWidget("vStepper", xmStepperWidgetClass, form,
				XmNtopOffset, 		10,
				XmNleftOffset, 		10,
				XmNrightOffset,		10,
				XmNbottomOffset, 	10,
				XmNtopAttachment, 	XmATTACH_WIDGET,
				XmNtopWidget, 		this->levelstepper,
				XmNrightAttachment, 	XmATTACH_FORM,
				XmNdataType, 		DOUBLE,
				XmNdMinimum, 		DoubleVal(min, dx_l1),
				XmNdMaximum, 		DoubleVal(max, dx_l2),
				XmNdValueStep, 		DoubleVal(inc, dx_l3),
				XmNdecimalPlaces, 	5,
				XmNdValue, 		DoubleVal(min_level, dx_l4),
				XmNfixedNotation,	False,
				NULL);


    sep = XtVaCreateManagedWidget("sep:", xmSeparatorWidgetClass, form, 
				XmNtopOffset, 		10,
				XmNleftOffset, 		2,
				XmNrightOffset, 	2,
				XmNtopAttachment, 	XmATTACH_WIDGET,
				XmNtopWidget, 		this->valuestepper,
				XmNleftAttachment, 	XmATTACH_FORM,
				XmNrightAttachment, 	XmATTACH_FORM,
				NULL);

    this->addbtn = 	
	XtVaCreateManagedWidget("addButton", 
				xmPushButtonWidgetClass, form,
				XmNwidth, 		70,
				XmNtopOffset, 		10,
				XmNleftOffset, 		10,
				XmNtopAttachment, 	XmATTACH_WIDGET,
				XmNtopWidget, 		sep,
				XmNtopOffset, 		10,
				XmNleftAttachment, 	XmATTACH_FORM,
				XmNbottomAttachment, 	XmATTACH_FORM,
				XmNbottomOffset, 	10,
				NULL);

    this->cancel = 	
	XtVaCreateManagedWidget("closeButton", 
			xmPushButtonWidgetClass, form,
				XmNwidth, 		70,
				XmNtopOffset, 		10,
				XmNrightOffset, 	10,
				XmNtopAttachment, 	XmATTACH_WIDGET,
				XmNtopWidget, 		sep,
				XmNtopOffset, 		10,
				XmNrightAttachment, 	XmATTACH_FORM,
				XmNbottomAttachment, 	XmATTACH_FORM,
				XmNbottomOffset, 	10,
				NULL);

    XtAddCallback(this->addbtn,
		      XmNactivateCallback,
		      (XtCallbackProc)ColormapAddCtlDialog_AddCB,
		      (XtPointer)this);

    XtAddCallback(this->valuestepper,
		      XmNwarningCallback,
		      (XtCallbackProc)ColormapAddCtlDialog_ValueRangeCB,
		      (XtPointer)this);

    XtAddCallback(this->levelstepper,
		      XmNwarningCallback,
		      (XtCallbackProc)ColormapAddCtlDialog_LevelRangeCB,
		      (XtPointer)this);


    this->setStepper();
    this->setFieldLabel(HUE);

    return form;
}

void ColormapAddCtlDialog::setStepper()
{
    double        step, min, max;
    ColormapNode* node;
    char          str[100];

    node = this->editor->getColormapNode();

    max = node->getMaximumValue();
    min = node->getMinimumValue();

    step = (max - min)/100;

#ifdef PASSDOUBLEVALUE
	XtArgVal dx_l1, dx_l2, dx_l3;
#endif

    XtVaSetValues(this->levelstepper,
		  XmNdMinimum, DoubleVal(min, dx_l1),
		  XmNdMaximum, DoubleVal(max, dx_l2),
		  XmNdValueStep, DoubleVal(step, dx_l3),
		  NULL);

    sprintf(str, "Data value(%g to %g):", min, max);
    XmString xmstr = XmStringCreateSimple(str);
    XtVaSetValues(this->levellabel, XmNlabelString, xmstr, NULL);
    XmStringFree(xmstr);
}

void ColormapAddCtlDialog::setFieldLabel(int selected_area)
{
    XmString xmstr = NULL;

    switch(selected_area)
    {
      case HUE:
	xmstr = XmStringCreateSimple("Hue value(0.0 to 1.0):");
	break;
      case SATURATION:
	xmstr = XmStringCreateSimple("Saturation value(0.0 to 1.0):");
	break;
      case VALUE:
	xmstr = XmStringCreateSimple("Value value(0.0 to 1.0):");
	break;
      case OPACITY:
	xmstr = XmStringCreateSimple("Opacity value(0.0 to 1.0):");
	break;
    }
    XtVaSetValues(this->valuelabel,
		  XmNlabelString, xmstr,
		  NULL);
    XmStringFree(xmstr);
}

extern "C" void ColormapAddCtlDialog_AddCB(Widget    widget,
                                 XtPointer clientData,
                                 XtPointer callData)
{
    double	level;
    double	value;

    ASSERT(widget);
    ASSERT(callData);
    ASSERT(clientData);
    ColormapAddCtlDialog *data = (ColormapAddCtlDialog*) clientData;

    XtVaGetValues(data->levelstepper, XmNdValue, &level, NULL);
    XtVaGetValues(data->valuestepper, XmNdValue, &value, NULL);
    CMEAddControlPoint(data->editor->getEditor(), level, value, True);
}

extern "C" void ColormapAddCtlDialog_ValueRangeCB(Widget    widget,
				        XtPointer clientData,
				        XtPointer callData)
{
    ASSERT(widget);
    ASSERT(clientData);

    WarningMessage("Values must be in the range 0.0 to 1.0");
}

extern "C" void ColormapAddCtlDialog_LevelRangeCB(Widget    widget,
				        XtPointer clientData,
				        XtPointer callData)
{
    ASSERT(widget);
    ASSERT(clientData);
    ColormapAddCtlDialog *dialog = (ColormapAddCtlDialog*) clientData;
    ColormapNode* node;

    node = dialog->editor->getColormapNode();

    WarningMessage("Values must be in the range %g to %g",
	node->getMinimumValue(), node->getMaximumValue());
}
