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
#include <Xm/PushB.h>
#include <Xm/ToggleB.h>
#include <Xm/Text.h>
#include <Xm/Label.h>
#include <Xm/Separator.h>
#include <Xm/BulletinB.h>

#include "DXStrings.h"
#include "OptionsDialog.h"
#include "MBMainWindow.h"

boolean OptionsDialog::ClassInitialized = FALSE;

String  OptionsDialog::DefaultResources[] = {
	"*dialogTitle:		      	Options",
	"*mainForm*okPB.labelString:		      OK",
	"*mainForm*cancelPB.labelString:	      Cancel",
	"*topOffset:                                  15",
	"*bottomOffset:                               15",
	"*leftOffset:                                 10",
	"*rightOffset:                                10",
        "*pinToggle.labelString:          PIN - Always run module on the same"
                                      " processor (if MP)",
        "*sideEffectToggle.labelString:   SIDE_EFFECT - Run module on every"
                                      " execution", 
        NULL
};


Widget OptionsDialog::createDialog(Widget parent)
{
    int n;
    Widget shell, sep;
    boolean set;
    XmString xms;

    shell = XmCreateDialogShell(parent, this->name, NULL, 0);
    XtVaSetValues(shell, 
		  XmNtitle, 		"Options...",
		  XmNallowShellResize,	True,
		  XmNwidth,		425,
		  NULL);

    this->mainform = 
	XtVaCreateWidget("mainForm",
		xmFormWidgetClass, 	shell,
		XmNdialogStyle,		XmDIALOG_APPLICATION_MODAL,
		XmNautoUnmanage,        FALSE,
		XmNwidth,		425,
		NULL);

    this->pin_tb = 
	XtVaCreateManagedWidget("pinToggle",
		xmToggleButtonWidgetClass, this->mainform,
		XmNleftAttachment,   	XmATTACH_FORM,
		XmNtopAttachment,  	XmATTACH_FORM,
		NULL);

    this->side_effect_tb= 
	XtVaCreateManagedWidget("sideEffectToggle",
		xmToggleButtonWidgetClass, this->mainform,
		XmNleftAttachment,   	XmATTACH_FORM,
		XmNtopAttachment,  	XmATTACH_WIDGET,
		XmNtopWidget,      	this->pin_tb,
		NULL);


   sep = 
	XtVaCreateManagedWidget("sep",
		xmSeparatorWidgetClass, this->mainform,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNleftOffset,		3,
		XmNrightAttachment,	XmATTACH_FORM,
		XmNrightOffset,		3,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		this->side_effect_tb,
		NULL);

    this->ok= 
	XtVaCreateManagedWidget("okPB",
		xmPushButtonWidgetClass, this->mainform,
		XmNleftAttachment,	XmATTACH_POSITION,
		XmNleftPosition,	5,
		XmNrightAttachment,	XmATTACH_POSITION,
		XmNrightPosition,	25,
		XmNbottomAttachment,  	XmATTACH_FORM,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		sep,
		NULL);

    this->cancel= 
	XtVaCreateManagedWidget("cancelPB",
		xmPushButtonWidgetClass, this->mainform,
		XmNleftAttachment,	XmATTACH_POSITION,
		XmNleftPosition,	75,
		XmNrightAttachment,	XmATTACH_POSITION,
		XmNrightPosition,	95,
		XmNbottomAttachment,  	XmATTACH_FORM,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		sep,
		NULL);

    XtManageChild(this->mainform);
    return this->mainform;
}

OptionsDialog::OptionsDialog(Widget parent, MBMainWindow *mbmw) 
			: Dialog("optionsD", parent)
{
    Widget shell;
     
    //
    // Install the default resources for THIS class (not the derived classes)
    //
    if (NOT OptionsDialog::ClassInitialized)
    {
        OptionsDialog::ClassInitialized = TRUE;
	this->installDefaultResources(parent);
    }

    this->mbmw = mbmw;
    this->pin = FALSE;
    this->side_effect = FALSE;
}

OptionsDialog::~OptionsDialog()
{
       if(this->host)
        delete this->host;

}

//
// Install the default resources for this class.
//
void OptionsDialog::installDefaultResources(Widget baseWidget)
{
    this->setDefaultResources(baseWidget, OptionsDialog::DefaultResources);
    this->Dialog::installDefaultResources(baseWidget);
}

void OptionsDialog::post()
{
    boolean set;

    this->Dialog::post();
    XmToggleButtonSetState(this->pin_tb, this->pin, False);
    XmToggleButtonSetState(this->side_effect_tb, this->side_effect, False);
}

boolean OptionsDialog::okCallback(Dialog *d)
{
    OptionsDialog *optionsDialog = (OptionsDialog*)d;

    optionsDialog->pin = 
	XmToggleButtonGetState(optionsDialog->pin_tb);
    optionsDialog->side_effect = 
	XmToggleButtonGetState(optionsDialog->side_effect_tb);
    return True;
}

void OptionsDialog::cancelCallback(Dialog *d)
{
    OptionsDialog *optionsDialog = (OptionsDialog*)d;

    XmToggleButtonSetState(optionsDialog->pin_tb, 
			   optionsDialog->pin, False);
    XmToggleButtonSetState(optionsDialog->side_effect_tb, 
			   optionsDialog->side_effect, False);

}
void OptionsDialog::getValues(boolean &pin, 
			    boolean &side_effect)
{
    pin = this->pin;
    side_effect = this->side_effect;

}
void OptionsDialog::setPin(boolean pin)
{
    this->pin = pin;
}
void OptionsDialog::setSideEffect(boolean side_effect)
{
    this->side_effect = side_effect;
}

