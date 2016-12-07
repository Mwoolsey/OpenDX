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

#include "StepperInteractor.h"
#include "SetScalarAttrDialog.h"
#include "InteractorStyle.h"

#include "ScalarNode.h"

#include "ScalarInstance.h"
#include "Application.h"
#include "ErrorDialogManager.h"
#include "../widgets/Stepper.h"

static Widget CreateStepperComponent(Widget  parent, 
				     boolean isInteger,
				     double  min,
				     double  max,
				     double  value,
				     double  delta,
				     int     decimalPlaces,
				     XtCallbackProc valueChangedCallback,
				     int     comp_index,
				     void * clientData);
				     
boolean StepperInteractor::StepperInteractorClassInitialized = FALSE;

String StepperInteractor::DefaultResources[] =  {
   "*Offset:				0",
   "*allowHorizontalResizing:		True",
   "*recomputeSize: 			True",
   "*XmStepper.alignment:		XmALIGNMENT_CENTER",
   "*stepperComponent.leftOffset:	4",
   "*stepperComponent.rightOffset:	4",
   "*stepperComponent.bottomOffset:	5",
   "*stepperComponent.topOffset:	5",


   "*accelerators:   #augment\n"
#if 0
        "<Btn2Down>,<Btn2Up>:            uicHelpInteractor()",
#endif
        "<Key>Return:                    BulletinBoardReturn()",

	NUL(char*) 
};

//
// One time class initializations.
//

StepperInteractor::StepperInteractor(const char *name,
                      InteractorInstance *ii) : ScalarInteractor(name,ii)
{
    this->componentForm = NULL;
}
//
// One time class initializations.
//
void StepperInteractor::initialize()
{
    //
    // Initialize default resources (once only).
    //
    if (NOT StepperInteractor::StepperInteractorClassInitialized)
    {
        ASSERT(theApplication);
        this->setDefaultResources(theApplication->getRootWidget(),
                                  StepperInteractor::DefaultResources);
        this->setDefaultResources(theApplication->getRootWidget(),
                                  Interactor::DefaultResources);
        StepperInteractor::StepperInteractorClassInitialized = TRUE;
    }
}

//
// Allocate an interactor for the given instance.
//
Interactor *StepperInteractor::AllocateInteractor(const char *name,
				InteractorInstance *ii)
{

    StepperInteractor *si = new StepperInteractor(name,ii);
    return (Interactor*)si;
}

//
// Perform anything that needs to be done after the parent of 
// this->interactivePart has been managed.
//
void StepperInteractor::completeInteractivePart()
{
    if (this->componentForm)
	this->passEvents(this->componentForm, TRUE);
    this->resizeCB();
}


//
// Build an n-vector or scalar stepper for the given instance.
//
Widget StepperInteractor::createInteractivePart(Widget form)
{
    ScalarNode	   	*node;
    int         	components, i;
    Widget		w, last_widget = NULL; /* NULL keeps compiler quiet */
    ScalarInstance	*si = (ScalarInstance*) this->interactorInstance;

    ASSERT(si);

    node = (ScalarNode*)si->getNode();

    ASSERT(form);
    ASSERT(node);

    components = node->getComponentCount();

    this->componentForm = form;

    for (i=1 ; i<=components ; i++) 
    {
	/*
	 * Set up and create the Stepper component of this interactor
	 */
        w = CreateStepperComponent(
		    this->componentForm,
		    si->isIntegerTypeComponent(),
		    si->getMinimum(i),
		    si->getMaximum(i),
		    si->getComponentValue(i),
		    si->getDelta(i),
		    si->getDecimals(i),
		    (XtCallbackProc)StepperInteractor_StepperCB,
		    i,
		    (void *)this);
	this->appendComponentWidget(w);

	/*
	 * Build the attachments for n-vector interactors within the 
	 * form (this->componentForm) created above. 
	 */
	switch (i) 
	{
	case 1: 
	    // The first widget gets attached to the top of the form.
	    XtVaSetValues(w, XmNtopAttachment, XmATTACH_FORM, NULL);
	    break;
	default:
	    // Intermediate widgets get attached to the last widet.
	    XtVaSetValues(w, 
			  XmNtopAttachment, XmATTACH_WIDGET,
	    		  XmNtopWidget, last_widget,
			  NULL);
		
	    break;
	}
	if (i==components) 	
	    XtVaSetValues (w, XmNbottomAttachment, XmATTACH_FORM, NULL);
	last_widget = w;
    }
    return this->componentForm;	
}

//
// Build a stepper component with the given attributes.
//
static Widget CreateStepperComponent(Widget  parent, 
				     boolean isInteger,
				     double  min,
				     double  max,
				     double  value,
				     double  delta,
				     int     decimalPlaces,
				     XtCallbackProc valueChangedCallback,
				     int     comp_index,
				     void * clientData)
				     
{
    Widget widget;
    int    n;
    Arg    wargs[30];

    ASSERT(parent);
    ASSERT(valueChangedCallback);

    n = 0;
    if (isInteger) 	// An integer interactor
    {
	int imin = (int)min, imax = (int)max, ivalue = (int)value, idelta = (int)delta;
	XtSetArg(wargs[n], XmNdataType,      INTEGER);   n++;
	XtSetArg(wargs[n], XmNdecimalPlaces, 0);         n++;
	XtSetArg(wargs[n], XmNiMinimum,      imin);      n++;
	XtSetArg(wargs[n], XmNiMaximum,      imax);      n++;
	XtSetArg(wargs[n], XmNiValue,        ivalue);    n++;
	XtSetArg(wargs[n], XmNiValueStep,    idelta);    n++;
    }
    else 
    {
	XtSetArg(wargs[n], XmNdataType,      DOUBLE);        n++;
	XtSetArg(wargs[n], XmNdecimalPlaces, decimalPlaces); n++;
	DoubleSetArg(wargs[n], XmNdMinimum,      min);          n++;
	DoubleSetArg(wargs[n], XmNdMaximum,      max);          n++;
	DoubleSetArg(wargs[n], XmNdValue,        value);        n++;
	DoubleSetArg(wargs[n], XmNdValueStep,    delta);        n++;
    }
    XtSetArg(wargs[n], XmNuserData,      (XtPointer)comp_index);      n++;
    XtSetArg(wargs[n], XmNfixedNotation, False);         n++;
    XtSetArg(wargs[n], XmNcharPlaces, 8);         n++;
    XtSetArg(wargs[n], XmNleftAttachment, XmATTACH_FORM);         n++;
    XtSetArg(wargs[n], XmNrightAttachment, XmATTACH_FORM);        n++;

    widget = XmCreateStepper(parent, "stepperComponent", wargs, n);
    XtManageChild(widget);

    XtAddCallback(widget,
		  XmNactivateCallback, 
		  valueChangedCallback,
		  clientData);

    XtAddCallback(widget,
		  XmNarmCallback, 
		  valueChangedCallback,
		  clientData);

    XtAddCallback(widget,
		  XmNwarningCallback,
		  (XtCallbackProc)ScalarInteractor_NumberWarningCB,
		  clientData);

    return widget;
}

//
// Call the virtual callback for a change in value. 
//
extern "C" void StepperInteractor_StepperCB(Widget                  widget,
					XtPointer		clientData,
					XtPointer		callData)
{
    StepperInteractor *si = (StepperInteractor*)clientData;
    int            component; 

    ASSERT(widget);

    component = (int)(long)GetUserData(widget);
    ASSERT(component > 0);
    si->stepperCallback(widget, component, callData);
}

//
// Accepts value changes and reflects them into other interactors, cdbs
// and off course the interactor node output.
//
void StepperInteractor::stepperCallback(Widget                  widget,
		int	component,
	        XtPointer	callData)
{
    XmDoubleCallbackStruct *cb = (XmDoubleCallbackStruct*) callData;
    ScalarInstance *si = (ScalarInstance*)this->interactorInstance;

    ASSERT(cb);
    ASSERT(si);
    
    /*
     * Find the node 
     * interactor node associated with this interactor.
     */

    /*
     * Wait for relevant value (i.e. the button has been released or
     * there is continuous update), then get the interactor value, store it,  
     * and send it.
     */
    boolean send = (cb->reason == XmCR_ACTIVATE) || si->isContinuous();
    if (send) {
        ScalarNode *node       = (ScalarNode*)si->getNode(); 
	si->setComponentValue(component, (double)cb->value);
	char *s = si->buildValueFromComponents();
	node->setOutputValue(1,s,DXType::UndefinedType, send);
	delete s;
    }
}

//
// Update the displayed values for this interactor.
//
void StepperInteractor::updateDisplayedInteractorValue()
{
    this->updateStepperValue();
}
//
// Update the displayed values for the stepper(s).
//
void StepperInteractor::updateStepperValue()
{
    Arg		   wargs[10];
    int		   i,components,n;
    // FIXME: should check to make sure we have the correct class of node.
    ScalarInstance *si = (ScalarInstance*)this->interactorInstance;
    ASSERT(si);

    /*
     * For all components.
     */
    components = si->getComponentCount();

    for (i=1 ; i<=components; i++) {
	double value = si->getComponentValue(i);
	Widget comp_stepper = this->getComponentWidget(i);
	n = 0;
        if (si->isIntegerTypeComponent())  {
	    XtSetArg(wargs[n], XmNiValue, (int)value); n++;
	} else {
	    DoubleSetArg(wargs[n], XmNdValue, value); n++;
	}
	/*
	 * Update the Stepper.
	 */
	XtSetValues(comp_stepper, wargs, n);
    }

}

//
// Make sure the attributes match the resources for the widgets.
//
void StepperInteractor::handleInteractivePartStateChange(
					InteractorInstance *src_ii,
					boolean major_change)
{
    Arg		   wargs[8];
    int		   i,components,n;
    // FIXME: should check to make sure we have the correct class of node.
    ScalarInstance *si = (ScalarInstance*)this->interactorInstance;

    /*
     * For all components.
     */
    components = si->getComponentCount();

    for (i=1 ; i<=components; i++) {
	
	int decimals = si->getDecimals(i);
	double delta = si->getDelta(i);
	double max   = si->getMaximum(i);
	double min   = si->getMinimum(i);

	Widget comp_stepper = this->getComponentWidget(i);
	n = 0;
        if (si->isIntegerTypeComponent())  {
	    XtSetArg(wargs[n], XmNiMaximum, (int)max); n++;
	    XtSetArg(wargs[n], XmNiMinimum, (int)min); n++;
	    XtSetArg(wargs[n], XmNiValueStep,    (int)delta);        n++;
	} else {
	    DoubleSetArg(wargs[n], XmNdMaximum, max); n++;
	    DoubleSetArg(wargs[n], XmNdMinimum, min); n++;
	    XtSetArg(wargs[n], XmNdecimalPlaces, decimals); n++;
	    DoubleSetArg(wargs[n], XmNdValueStep,    delta);   n++;

	}
	/*
	 * Update the Stepper.
	 */
	XtSetValues(comp_stepper, wargs, n);
    }

    this->updateStepperValue();

}

void StepperInteractor::layoutInteractorHorizontally()
{
    ScalarInstance *si = (ScalarInstance*) this->interactorInstance;
    ScalarNode *node = (ScalarNode*)si->getNode();
    int i, components = node->getComponentCount();
    Widget stepper;

    ASSERT(this->componentForm);
    
    //
    // Don't layout vectors with XmALIGNMENT_END because they need to be
    // centered horizontally wrt eachother even if the precision of one
    // of the steppers in the group changes.  If there were set to 
    // XmALIGNMENT_END then things would look kind of odd.
    //
    unsigned char align = (components == 1? XmALIGNMENT_END: XmALIGNMENT_CENTER);
    for (i=1 ; i<=components ; i++)  
    {
	stepper = this->getComponentWidget(i);
	XtVaSetValues (stepper, XmNalignment, align, NULL);
    }
    
    this->Interactor::layoutInteractorHorizontally();
}

void StepperInteractor::layoutInteractorVertically()
{
    ScalarInstance *si = (ScalarInstance*) this->interactorInstance;
    ScalarNode *node = (ScalarNode*)si->getNode();
    int i, components = node->getComponentCount();
    Widget stepper;

    ASSERT(this->componentForm);
    
    for (i=1 ; i<=components ; i++)  
    {
	stepper = this->getComponentWidget(i);
	XtVaSetValues (stepper, XmNalignment, XmALIGNMENT_CENTER, NULL);
    }
    
    this->Interactor::layoutInteractorVertically();
}
