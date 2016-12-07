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
#include "ColormapNBinsDialog.h"
#include "ColormapEditor.h"
#include "ColormapNode.h"

#include "../widgets/Stepper.h"
#include "../widgets/ColorMapEditor.h"

Boolean ColormapNBinsDialog::ClassInitialized = FALSE;

String ColormapNBinsDialog::DefaultResources[] = {
	"*dialogTitle:			Number of Histogram Bins",
	"*okButton.labelString:		OK",
	"*cancelButton.labelString:	Cancel",
        "*accelerators:                 #augment\n"
        "<Key>Return:                   BulletinBoardReturn()",
	NULL
};

ColormapNBinsDialog::ColormapNBinsDialog(Widget parent,
					   ColormapEditor* editor) 
                       			   : Dialog("nBins", parent)
{
    this->editor = editor;

    if (NOT ColormapNBinsDialog::ClassInitialized)
    {
        ColormapNBinsDialog::ClassInitialized = TRUE;
	this->installDefaultResources(theApplication->getRootWidget());
    }
}

ColormapNBinsDialog::~ColormapNBinsDialog()
{
}


//
// Install the default resources for this class.
//
void ColormapNBinsDialog::installDefaultResources(Widget  baseWidget)
{
    this->setDefaultResources(baseWidget,
				ColormapNBinsDialog::DefaultResources);
    this->Dialog::installDefaultResources( baseWidget);
}
Widget ColormapNBinsDialog::createDialog(Widget parent)
{
    Arg arg[10];
    Widget sep;
    int min, max, inc, level;
    int n = 0;

    XtSetArg(arg[n], XmNdialogStyle, XmDIALOG_PRIMARY_APPLICATION_MODAL);  n++;
    XtSetArg(arg[n], XmNwidth,       320);  n++;
    XtSetArg(arg[n], XmNheight,      90);  n++;
    XtSetArg(arg[n], XmNresizePolicy,XmRESIZE_GROW);  n++;
    XtSetArg(arg[n], XmNautoUnmanage,False);  n++;

    Widget form = this->CreateMainForm(parent, this->name, arg, n);

    this->label = 	
	XtVaCreateManagedWidget("Number of histogram bins:", 
			xmLabelWidgetClass, form,
				XmNtopOffset, 		10,
				XmNleftOffset, 		10,
				XmNbottomOffset, 	10,
				XmNtopAttachment, 	XmATTACH_FORM,
				XmNleftAttachment, 	XmATTACH_FORM,
				NULL);

    min = 1; max = 5000; inc = 1; level = 20;
    this->nbinsstepper = 	
	XtVaCreateManagedWidget("nbinsStepper", xmStepperWidgetClass, form,
				XmNtopOffset, 		10,
				XmNleftOffset, 		10,
				XmNrightOffset, 	10,
				XmNbottomOffset, 	10,
				XmNtopAttachment, 	XmATTACH_FORM,
				XmNrightAttachment, 	XmATTACH_FORM,
				XmNdataType, 		INTEGER,
				XmNiMinimum, 		min,
				XmNiMaximum, 		max,
				XmNiValueStep, 		inc,
				XmNiValue, 		level,
				NULL);

    sep = XtVaCreateManagedWidget("sep:", xmSeparatorWidgetClass, form, 
				XmNtopOffset, 		10,
				XmNleftOffset, 		2,
				XmNrightOffset, 	2,
				XmNtopAttachment, 	XmATTACH_WIDGET,
				XmNtopWidget, 		this->nbinsstepper,
				XmNleftAttachment, 	XmATTACH_FORM,
				XmNrightAttachment, 	XmATTACH_FORM,
				NULL);

    this->ok = 	
	XtVaCreateManagedWidget("okButton", 
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
	XtVaCreateManagedWidget("cancelButton", 
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

    this->setStepper();

    return form;
}

void ColormapNBinsDialog::post()
{
    this->Dialog::post();
    this->setStepper();
}

void ColormapNBinsDialog::setStepper()
{
    ColormapNode* node;

    node = this->editor->getColormapNode();

    XtVaSetValues(this->nbinsstepper,
		  XmNiValue, (int)node->getNBinsValue(),
		  NULL);

}


boolean ColormapNBinsDialog::okCallback(Dialog *dialog)
{
    int	nbins;
    ColormapNode* node;

    node = this->editor->getColormapNode();

    XtVaGetValues(this->nbinsstepper, XmNiValue, &nbins, NULL);
    node->setNBinsValue((double)nbins);

    return TRUE;
}
