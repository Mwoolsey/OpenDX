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
#include <Xm/PushB.h> 
#include <Xm/RowColumn.h> 
#include <string.h>

#include "SetVectorAttrDialog.h"
#include "ScalarInstance.h"
#include "ScalarNode.h"
#include "Application.h"
#include "../widgets/Stepper.h"


boolean SetVectorAttrDialog::ClassInitialized = FALSE;

String  SetVectorAttrDialog::DefaultResources[] = {
    "*componentStepper.rightOffset:    10", 
    "*componentStepper.topOffset:      10", 
    "*componentStepper.editable:       True", 
    "*componentStepper.fixedNotation:  False", 
    
    "*componentOptions.topOffset:       10",
    "*componentOptions.leftOffset:      10",
    "*componentPulldown*allComponents*labelString:All Components",
    "*componentPulldown*selectedComponent*labelString:Selected Components",

#if defined(aviion)
    "*componentOptions.labelString:",
#endif

    NULL };



extern "C" void SetVectorAttrDialog_StepperCB(Widget  widget,
                         XtPointer clientData,
                         XtPointer callData)
{
    SetScalarAttrDialog *sad  = (SetScalarAttrDialog*)clientData;
    int i = sad->getCurrentComponentNumber();
    sad->updateDisplayedComponentAttributes(i);
}
extern "C" void SetVectorAttrDialog_ComponentOptionCB(Widget  w,
                         XtPointer clientData,
                         XtPointer callData)
{
    SetVectorAttrDialog *svad  = (SetVectorAttrDialog*)clientData;
    Boolean sensitive = False;

    if (w == svad->allComponents) {
	XmAnyCallbackStruct *cb = (XmAnyCallbackStruct*)callData;
	if (cb->reason == XmCR_ACTIVATE)
	    svad->updateAllAttributes(svad->getCurrentComponentNumber());
	sensitive = False;
    } else if (w == svad->selectedComponent) {
	sensitive = True;
    } else {
	ASSERT(0);
    }
    XtSetSensitive(svad->componentStepper, sensitive);
}

SetVectorAttrDialog::SetVectorAttrDialog(Widget parent,
                        const char *title, ScalarInstance *si) :
			SetScalarAttrDialog("vectorAttr",parent,title,si)
{
    this->allComponents = NULL;
    this->selectedComponent = NULL;
    this->componentOptions = NULL;
    this->componentStepper = NULL;
    // ScalarNode *sinode = (ScalarNode*)si->getNode();

    if (NOT SetVectorAttrDialog::ClassInitialized)
    {
        SetVectorAttrDialog::ClassInitialized = TRUE;
	this->installDefaultResources(theApplication->getRootWidget());
    }

}

SetVectorAttrDialog::~SetVectorAttrDialog()
{
}
//
// Called before the root widget is created.
//
//
// Install the default resources for this class.
//
void SetVectorAttrDialog::installDefaultResources(Widget  baseWidget)
{
    this->setDefaultResources(baseWidget,
				SetVectorAttrDialog::DefaultResources);
    this->SetScalarAttrDialog::installDefaultResources( baseWidget);
}

Widget SetVectorAttrDialog::createComponentPulldown(Widget parent,
                                                        const char *name)
{
    Widget pulldown = XmCreatePulldownMenu(parent, (char*)name, NULL, 0);

    this->selectedComponent = XtVaCreateManagedWidget(
                "selectedComponent",xmPushButtonWidgetClass,pulldown,
                NULL);
    this->allComponents = XtVaCreateManagedWidget(
                "allComponents",xmPushButtonWidgetClass,pulldown,
                NULL);

    XtAddCallback(this->allComponents,XmNarmCallback,
                (XtCallbackProc)SetVectorAttrDialog_ComponentOptionCB, (XtPointer)this);
    XtAddCallback(this->allComponents,XmNactivateCallback,
                (XtCallbackProc)SetVectorAttrDialog_ComponentOptionCB, (XtPointer)this);
    XtAddCallback(this->selectedComponent,XmNarmCallback,
                (XtCallbackProc)SetVectorAttrDialog_ComponentOptionCB, (XtPointer)this);

    return pulldown;
}

void SetVectorAttrDialog::createAttributesPart(Widget parentDialog)
{

    //
    // Add the components pulldown and stepper and then change the attachments. 
    //

    Widget pulldown = this->createComponentPulldown(parentDialog,
                                                "componentPulldown");
    this->componentOptions = XtVaCreateManagedWidget(
                "componentOptions", xmRowColumnWidgetClass, parentDialog,
                XmNentryAlignment  , XmALIGNMENT_CENTER,
                XmNrowColumnType   , XmMENU_OPTION,
                XmNmenuHistory     , this->selectedComponent,
                XmNsubMenuId       , pulldown,
                NULL);


   this->componentStepper = XtVaCreateManagedWidget(
        "componentStepper", xmStepperWidgetClass, parentDialog,
                XmNdataType      ,      INTEGER,
                XmNiValue        ,      1,
                XmNiMinimum      ,      1,
                XmNiMaximum      ,      this->numComponents,
                XmNdecimalPlaces ,      0,
                NULL);

    XtAddCallback(this->componentStepper,XmNactivateCallback,
                (XtCallbackProc)SetVectorAttrDialog_StepperCB, (XtPointer)this);
    //
    // Build the same dialog as for a Scalar interactor.
    // Wait until now so that this->getCurrentComponentNumber() can be called.
    //
    this->SetScalarAttrDialog::createAttributesPart(parentDialog);

    //
    // And now override the attachments set in SetScalarAttrDialog.
    //
    XtVaSetValues(this->maxLabel,
		XmNtopAttachment , XmATTACH_WIDGET,
		XmNtopWidget     , this->componentOptions,
		NULL);
    XtVaSetValues(this->maxNumber,
		XmNtopAttachment , XmATTACH_WIDGET,
		XmNtopWidget     , this->componentOptions,
		NULL);
    XtVaSetValues(this->componentOptions,
		XmNtopAttachment , XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);
    XtVaSetValues(this->componentStepper,
		XmNtopAttachment , XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);
}


// FIXME: this should really use SetScalarAttrDialog::getNumberWidgetValue(),
// which should really be a global utility function.
int SetVectorAttrDialog::getCurrentComponentNumber()
{
    short type;
    int ivalue;
    double dvalue;

    Widget w = this->componentStepper;
    ASSERT(w);
    XtVaGetValues(w, XmNdataType, &type, NULL);

    if (type == DOUBLE) {
        XtVaGetValues(w, XmNdValue, &dvalue, NULL);
        ivalue = (int)dvalue;
    } else {
        XtVaGetValues(w, XmNiValue, &ivalue, NULL);
    }
    return ivalue;
}

//
// Determine if the 'All Components' selection is active.
//
boolean SetVectorAttrDialog::isAllAttributesMode()
{
    Widget w;

    XtVaGetValues(this->componentOptions, XmNmenuHistory, &w, NULL);
    if (this->allComponents == w)
	return TRUE;
    else
	return FALSE;

}
