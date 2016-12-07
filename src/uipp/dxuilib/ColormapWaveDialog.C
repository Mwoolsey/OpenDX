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
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/PushB.h>
#include <Xm/SeparatoG.h>
#include <Xm/Separator.h>
#include <Xm/ScrolledW.h>
#include <Xm/BulletinB.h>
#include <Xm/RowColumn.h>
#include <Xm/Label.h>

#include "Application.h"
#include "ColormapWaveDialog.h"
#include "ColormapEditor.h"
#include "ColormapNode.h"

#include "step.bm"
#include "saw_sor.bm"
#include "saw_sol.bm"
#include "square_sor.bm"
#include "square_sol.bm"

#include "../widgets/Stepper.h"
#include "../widgets/ColorMapEditor.h"


boolean ColormapWaveDialog::ClassInitialized = FALSE;
String ColormapWaveDialog::DefaultResources[] = {
	"*dialogTitle:			Generate Wave Forms",
	"*XmPushButton.height:		30",
	"*closebtn.labelString:		Close",
	"*applybtn.labelString:		Apply",
	"*waveFormLabel.labelString:	Waveform:",
	"*stepsLabel.labelString:	Steps:",
    	"*accelerators:          	#augment\n"
          "<Key>Return:                   BulletinBoardReturn()",
	NULL
};

ColormapWaveDialog::ColormapWaveDialog(Widget parent,
					   ColormapEditor* editor) 
                       			   : Dialog("generateWaveforms", parent)
{
    this->editor = editor;

    if (NOT ColormapWaveDialog::ClassInitialized)
    {
        ColormapWaveDialog::ClassInitialized = TRUE;
	this->installDefaultResources(theApplication->getRootWidget());
    }
}

ColormapWaveDialog::~ColormapWaveDialog()
{

}

//
// Install the default resources for this class.
//
void ColormapWaveDialog::installDefaultResources(Widget  baseWidget)
{
    this->setDefaultResources(baseWidget, ColormapWaveDialog::DefaultResources);
    this->Dialog::installDefaultResources( baseWidget);
}

Widget ColormapWaveDialog::createDialog(Widget parent)
{
    Widget 	sep;
    Arg		args[20];
    int		n;
    Widget	pulld;
    Pixmap	pixmap;
    Pixel	bg;
    Display	*display;
    Window	root;
    Pixel	black;
    int		depth;
    XmString	xms;


    n = 0;
    XtSetArg(args[n], XmNresizePolicy, XmRESIZE_GROW); n++;
    XtSetArg(args[n], XmNwidth,          177); n++;
    XtSetArg(args[n],XmNheight,           160); n++;
    XtSetArg(args[n],XmNautoUnmanage,    False); n++;
    XtSetArg(args[n],XmNdialogStyle,     XmDIALOG_MODELESS); n++;
//    Widget dialog = XmCreateFormDialog(parent, this->name, args, n);
    Widget dialog = this->CreateMainForm(parent, this->name, args, n);

	XtVaCreateManagedWidget("waveFormLabel", xmLabelWidgetClass, dialog,
		    XmNtopAttachment, XmATTACH_FORM,
		    XmNleftAttachment, XmATTACH_FORM,
		    XmNtopOffset, 10,
		    XmNleftOffset, 10,
		    NULL);

    n = 0;
    pulld = XmCreatePulldownMenu(dialog, "Pulldown", args, n);

    n = 0;
    XtSetArg(args[n], XmNsubMenuId, pulld); n++;
    XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
    XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
    XtSetArg(args[n], XmNtopOffset, 10); n++;
    XtSetArg(args[n], XmNrightOffset, 10); n++;
    this->waveoption = XmCreateOptionMenu(dialog, "Option", args, n);

    //
    // Some useful things to keep around.
    //
    XtVaGetValues(this->waveoption, XmNbackground, &bg, NULL);
    display = XtDisplay(this->waveoption);
    root = RootWindowOfScreen(XtScreen(this->waveoption));
    black = BlackPixel(display,0);
    depth = DefaultDepthOfScreen(XtScreen(this->waveoption));

    //
    // Create the pixmap buttons
    //
    pixmap = XCreatePixmapFromBitmapData(
		display, root, 
		(char *)step_bits, step_width, step_height,
		black, bg, depth);
    this->step = XtVaCreateManagedWidget("step", xmPushButtonWidgetClass, pulld,
			XmNlabelType, XmPIXMAP,
			XmNlabelPixmap, pixmap,
			NULL);

    pixmap = XCreatePixmapFromBitmapData(
		display, root, 
		(char *)square_sol_bits, square_sol_width, square_sol_height,
		black, bg, depth);
    this->square_sol = 
	XtVaCreateManagedWidget("square_sol", xmPushButtonWidgetClass, pulld,
			XmNlabelType, XmPIXMAP,
			XmNlabelPixmap, pixmap,
			NULL);

    pixmap = XCreatePixmapFromBitmapData(
		display, root, 
		(char *)square_sor_bits, square_sor_width, square_sor_height,
		black, bg, depth);
    this->square_sor = 
	XtVaCreateManagedWidget("square_sor", xmPushButtonWidgetClass, pulld,
			XmNlabelType, XmPIXMAP,
			XmNlabelPixmap, pixmap,
			NULL);

    pixmap = XCreatePixmapFromBitmapData(
		display, root, 
		(char *)saw_sol_bits, saw_sol_width, saw_sol_height,
		black, bg, depth);
    this->saw_sol = 
	XtVaCreateManagedWidget("saw_sol", xmPushButtonWidgetClass, pulld,
			XmNlabelType, XmPIXMAP,
			XmNlabelPixmap, pixmap,
			NULL);

    pixmap = XCreatePixmapFromBitmapData(
		display, root, 
		(char *)saw_sor_bits, saw_sor_width, saw_sor_height,
		black, bg, depth);
    this->saw_sor = 
	XtVaCreateManagedWidget("square_sor", xmPushButtonWidgetClass, pulld,
			XmNlabelType, XmPIXMAP,
			XmNlabelPixmap, pixmap,
			NULL);
    XtManageChild(this->waveoption);

    XtVaCreateManagedWidget("Range:", xmLabelWidgetClass, dialog,
    	    XmNtopAttachment, XmATTACH_WIDGET,
    	    XmNtopWidget, this->waveoption,
    	    XmNleftAttachment, XmATTACH_FORM,
    	    XmNtopOffset, 10,
    	    XmNleftOffset, 10,
    	    NULL);

    n = 0;
    pulld = XmCreatePulldownMenu(dialog, "Pulldown", args, n);

    n = 0;
    XtSetArg(args[n], XmNsubMenuId, pulld); n++;
    XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNtopWidget, this->waveoption); n++;
    XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
    XtSetArg(args[n], XmNtopOffset, 10); n++;
    XtSetArg(args[n], XmNrightOffset, 10); n++;
    this->rangeoption = XmCreateOptionMenu(dialog, "Option", args, n);

    xms = XmStringCreateSimple("Full");
    this->rangefull = 
	XtVaCreateManagedWidget("Full", xmPushButtonWidgetClass, pulld,
			XmNlabelString, xms,
			NULL);
    XmStringFree(xms);

    xms = XmStringCreateSimple("Selected");
    this->rangeselected = 
	XtVaCreateManagedWidget("Selected", xmPushButtonWidgetClass, pulld,
			XmNlabelString, xms,
			NULL);
    XmStringFree(xms);

    XtManageChild(this->rangeoption);

   /*
    * Steppers etc.
    */
	XtVaCreateManagedWidget("stepsLabel", xmLabelWidgetClass, dialog,
			XmNtopOffset, 10,
			XmNleftOffset, 10,
			XmNbottomOffset, 10,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, this->rangeoption,
			XmNleftAttachment, XmATTACH_FORM,
			NULL);

    this->nstep = 
	XtVaCreateManagedWidget("nstep", xmStepperWidgetClass, dialog,
			XmNtopOffset, 10,
			XmNleftOffset, 10,
			XmNrightOffset, 10,
			XmNbottomOffset, 10,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, this->rangeoption,
			XmNrightAttachment, XmATTACH_FORM,
			XmNdataType, INTEGER,
			XmNiMinimum, 2,
			XmNiMaximum, 100,
			NULL);

    sep = XtVaCreateManagedWidget("sep", xmSeparatorWidgetClass, dialog,
			XmNtopOffset, 10,
			XmNleftOffset, 2,
			XmNrightOffset, 2,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, this->nstep,
			XmNleftAttachment, XmATTACH_FORM,
			XmNrightAttachment, XmATTACH_FORM,
			NULL);

    this->applybtn = 
	XtVaCreateManagedWidget("applybtn", xmPushButtonWidgetClass, dialog,
			XmNwidth, 70,
			XmNtopOffset, 10,
			XmNleftOffset, 10,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, sep,
			XmNtopOffset, 10,
			XmNbottomAttachment, XmATTACH_FORM,
			XmNbottomOffset, 10,
			XmNleftAttachment, XmATTACH_FORM,
			NULL);

    this->closebtn = 
	XtVaCreateManagedWidget("Close", xmPushButtonWidgetClass, dialog,
			XmNwidth, 70,
			XmNtopOffset, 10,
			XmNleftOffset, 10,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, sep,
			XmNtopOffset, 10,
			XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, this->applybtn,
			XmNbottomAttachment, XmATTACH_FORM,
			XmNbottomOffset, 10,
			XmNrightAttachment, XmATTACH_FORM,
			XmNrightOffset, 10,
			NULL);
    XtAddCallback(XtParent(dialog),
		      XmNpopdownCallback,
		      (XtCallbackProc)ColormapWaveDialog_PopdownCB,
		      (XtPointer)this);

    XtAddCallback(this->closebtn,
		      XmNactivateCallback,
		      (XtCallbackProc)ColormapWaveDialog_CloseCB,
		      (XtPointer)this);

    XtAddCallback(this->applybtn,
		      XmNactivateCallback,
		      (XtCallbackProc)ColormapWaveDialog_AddCB,
		      (XtPointer)this);

    XtAddCallback(this->rangeselected,
		      XmNactivateCallback,
		      (XtCallbackProc)ColormapWaveDialog_RangeCB,
		      (XtPointer)this);

    XtAddCallback(this->rangefull,
		      XmNactivateCallback,
		      (XtCallbackProc)ColormapWaveDialog_RangeCB,
		      (XtPointer)this);
    return dialog;
}



extern "C" void ColormapWaveDialog_AddCB(Widget    widget,
                                 XtPointer clientData,
                                 XtPointer callData)
{
    int 	nstep;
    Boolean 	start_on_left;
    Boolean 	use_selected;
    Widget	wave_widget;
    Widget	range_widget;

    ASSERT(clientData);
    ColormapWaveDialog *data = (ColormapWaveDialog*) clientData;
    XtVaGetValues(data->waveoption, XmNmenuHistory, &wave_widget, NULL);

    XtVaGetValues(data->rangeoption, XmNmenuHistory, &range_widget, NULL);

    if ( (wave_widget == data->saw_sol) ||
	 (wave_widget == data->square_sol) )
	start_on_left = True;
    else
	start_on_left = False;

    if(range_widget == data->rangefull)
	use_selected = False;
    else
	use_selected = True;

    XtVaGetValues(data->nstep, XmNiValue, &nstep, NULL);

    if(wave_widget == data->step)
    {
	CMEStep(data->editor->getEditor(), nstep, use_selected);
    }
    else if( (wave_widget == data->square_sol) ||
	     (wave_widget == data->square_sor) )
    {
	CMESquareWave(data->editor->getEditor(), 
		nstep, start_on_left, use_selected);
    }
    else if( (wave_widget == data->saw_sol) ||
	     (wave_widget == data->saw_sor) )
    {
	CMESawToothWave(data->editor->getEditor(),
			nstep, start_on_left, use_selected);
    }
}

extern "C" void ColormapWaveDialog_CloseCB(Widget    widget,
                                   XtPointer clientData,
                                   XtPointer callData)
{
    ASSERT(clientData);
    ColormapWaveDialog *data = (ColormapWaveDialog*) clientData;

    XtUnmanageChild(data->getRootWidget());
}

extern "C" void ColormapWaveDialog_PopdownCB(Widget    widget,
                                XtPointer clientData,
                                XtPointer callData)
{
    ASSERT(clientData);
}
extern "C" void ColormapWaveDialog_RangeCB(Widget    widget,
                                 XtPointer clientData,
                                 XtPointer callData)
{

    ASSERT(clientData);
    ColormapWaveDialog *data = (ColormapWaveDialog*) clientData;
    data->rangeCallback(widget);
}

void ColormapWaveDialog::rangeCallback(Widget    widget)
{
    ColormapEditor* editor = this->editor;

    if(widget == this->rangefull)
    {
	XtSetSensitive(this->applybtn, True);
    }
    if(widget == this->rangeselected)
    {
	if((editor->hue_selected < 2) &&
	   (editor->sat_selected < 2) &&
	   (editor->val_selected < 2) &&
	   (editor->op_selected  < 2))
	    XtSetSensitive(this->applybtn, False);
	else
	    XtSetSensitive(this->applybtn, True);
    }
}

