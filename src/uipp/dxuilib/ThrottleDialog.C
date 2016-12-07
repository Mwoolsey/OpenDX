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
#include <Xm/SeparatoG.h>
#include <Xm/Label.h>
#include <Xm/BulletinB.h>

#include "ThrottleDialog.h"
#include "ImageWindow.h"
#include "../widgets/Number.h"

boolean ThrottleDialog::ClassInitialized = FALSE;

String  ThrottleDialog::DefaultResources[] = {
	"*dialogTitle:			Throttle...",
        "*accelerators:              	#augment\n"
          "<Key>Return:                 BulletinBoardReturn()",
	"*closeButton.labelString:	Close",
	"*closeButton.width:		60",
	"*closeButton.height:		30",
	".width:			280",
	".height:			100",
	"*label.labelString:		Seconds per Frame:",
        NULL
};

ThrottleDialog::ThrottleDialog(char *name, ImageWindow* image) 
                       		: Dialog(name, image->getRootWidget())
{
    this->image = image;
}

ThrottleDialog::~ThrottleDialog()
{
}

void ThrottleDialog::initialize()
{
    if (!ThrottleDialog::ClassInitialized) {
	ThrottleDialog::ClassInitialized = TRUE;
        this->setDefaultResources(image->getRootWidget(), 
			ThrottleDialog::DefaultResources);
    }

}

Widget ThrottleDialog::createDialog(Widget parent)
{
    int n;
    Arg arg[4];
    Widget separator;

    n = 0;
    XtSetArg(arg[n],XmNautoUnmanage,    True); n++;
    XtSetArg(arg[n],XmNnoResize,        True); n++;
//    Widget dialog = XmCreateFormDialog(parent, this->name, arg, n);
    Widget dialog = this->CreateMainForm(parent, this->name, arg, n);
#ifdef PASSDOUBLEVALUE
	XtArgVal dx_l1, dx_l2;
#endif

    this->closebtn = XtVaCreateManagedWidget("closeButton",
	xmPushButtonWidgetClass, dialog, 
        XmNleftAttachment,   XmATTACH_FORM,
        XmNleftOffset,        5,
        XmNbottomAttachment,  XmATTACH_FORM,
        XmNbottomOffset,      10,
        NULL);

    separator = XtVaCreateManagedWidget("separator",
	xmSeparatorGadgetClass, dialog, 
        XmNleftAttachment,    XmATTACH_FORM,
        XmNleftOffset,        2,
        XmNrightAttachment,   XmATTACH_FORM,
        XmNrightOffset,       2,
        XmNbottomAttachment,  XmATTACH_WIDGET,
        XmNbottomWidget,      this->closebtn,
        XmNbottomOffset,      10,
        NULL);


    this->label = XtVaCreateManagedWidget("label",
	xmLabelWidgetClass, dialog,
	XmNbottomAttachment,  XmATTACH_WIDGET,
	XmNbottomWidget,      separator,
	XmNbottomOffset,      10,
	XmNleftAttachment,    XmATTACH_FORM,
	XmNleftOffset,        5,
	XmNtopAttachment,     XmATTACH_FORM,
	XmNtopOffset,         10,
	NULL);

    double min=0.0, max = 100.0;
    this->seconds = XtVaCreateManagedWidget("seconds",
	xmNumberWidgetClass, dialog,
        XmNbottomAttachment, XmATTACH_WIDGET,
	XmNbottomWidget,     separator, 
	XmNbottomOffset,     10,
	XmNrightAttachment,  XmATTACH_FORM,
	XmNrightOffset,      5,
	XmNtopAttachment,    XmATTACH_FORM,
	XmNtopOffset,        10,
	XmNdataType,         DOUBLE,
	XmNdecimalPlaces,    3,
	XmNcharPlaces,       5,
	XmNeditable,         True,
        XmNfixedNotation,    False,
	XmNdMinimum,         DoubleVal(min, dx_l1),
	XmNdMaximum,         DoubleVal(max, dx_l2),
	NULL);

#if 0
    XtAddCallback(dialog,
		      XmNmapCallback,
		      (XtCallbackProc)ThrottleDialog_DoAllCB,
		      (XtPointer)this);
#endif

    XtAddCallback(this->seconds,
		      XmNactivateCallback,
		      (XtCallbackProc)ThrottleDialog_DoAllCB,
		      (XtPointer)this);
    return dialog;
}



void ThrottleDialog::manage()
{
    Dimension dialogWidth;

    if (!XtIsRealized(this->getRootWidget()))
	XtRealizeWidget(this->getRootWidget());

    XtVaGetValues(this->getRootWidget(),
	XmNwidth, &dialogWidth,
	NULL);

    Position x;
    Position y;
    Dimension width;
    XtVaGetValues(parent,
	XmNx, &x,
	XmNy, &y,
	XmNwidth, &width,
	NULL);

    if (x > dialogWidth + 10)
	x -= dialogWidth + 10;
    else
	x += width + 10;

    Display *dpy = XtDisplay(this->getRootWidget());
    if (x > WidthOfScreen(DefaultScreenOfDisplay(dpy)) - dialogWidth)
	x = WidthOfScreen(DefaultScreenOfDisplay(dpy)) - dialogWidth;
    XtVaSetValues(this->getRootWidget(),
	XmNdefaultPosition, False, 
	NULL);
    XtVaSetValues(XtParent(this->getRootWidget()),
	XmNdefaultPosition, False, 
	XmNx, x, 
	XmNy, y, 
	NULL);

    this->Dialog::manage();
    double	value;
    this->image->getThrottle(value);
    XmChangeNumberValue((XmNumberWidget)this->seconds, value);
 
}

void ThrottleDialog::installThrottleValue(double value)
{
    XmChangeNumberValue((XmNumberWidget)this->seconds, value);
}

extern "C" void ThrottleDialog_DoAllCB(Widget    widget,
                                XtPointer clientData,
                                XtPointer callData)
{

    ASSERT(clientData);
    ThrottleDialog *dialog = (ThrottleDialog*) clientData;
    XmAnyCallbackStruct* cbs = (XmAnyCallbackStruct*) callData;
    double	value;

    switch(cbs->reason)
    {
#if 0
	case XmCR_MAP:

	     dialog->image->getThrottle(value);
	     XmChangeNumberValue((XmNumberWidget)dialog->seconds, value);
 
	     break;
#endif

	default:

	     XtVaGetValues(dialog->seconds, XmNdValue, &value, NULL);
	     dialog->image->setThrottle(value);      
	
	     break;

    }

}
