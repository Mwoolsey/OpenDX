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

#include "DialInteractor.h"
#include "SetScalarAttrDialog.h"
#include "InteractorStyle.h"

#include "ScalarNode.h"

#include "ScalarInstance.h"
#include "Application.h"
#include "ErrorDialogManager.h"
#include "../widgets/Dial.h"
#include "../widgets/Number.h"

boolean DialInteractor::DialInteractorClassInitialized = FALSE;

String DialInteractor::DefaultResources[] =  {
    ".AllowResizing:            True",
    "*form*horizontalSpacing:	5",
    "*pinTopBottom:		True",
    "*pinLeftRight:		True",
    "*form*verticalSpacing:	5",
    "*form*Offset:		5",
    "*dial.shadowThickness:	0",

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

DialInteractor::DialInteractor(const char *name,
                      InteractorInstance *ii) : ScalarInteractor(name,ii)
{
    this->componentForm = NULL;
}
//
// One time class initializations.
//
void DialInteractor::initialize()
{
    //
    // Initialize default resources (once only).
    //
    if (NOT DialInteractor::DialInteractorClassInitialized)
    {
        ASSERT(theApplication);
        this->setDefaultResources(theApplication->getRootWidget(),
                                  DialInteractor::DefaultResources);
        this->setDefaultResources(theApplication->getRootWidget(),
                                  Interactor::DefaultResources);
        DialInteractor::DialInteractorClassInitialized = TRUE;
    }
}


static Widget CreateDialComponent(Widget,  boolean, double, double,
				     double, double, int,   XtCallbackProc,   
				     int, void *) ;

//
// Allocate a dial for the given instance.
//
Interactor *DialInteractor::AllocateInteractor(const char *name, 
						InteractorInstance *ii)
{

    DialInteractor *si = new DialInteractor(name, ii);
    return (Interactor*)si;
}

//
// Perform anything that needs to be done after the parent of 
// this->interactivePart has been managed.
//
void DialInteractor::completeInteractivePart()
{
    if (this->componentForm)
	this->passEvents(this->componentForm, TRUE);
}
//
// Build a dial and a number for the given instance.
//
Widget DialInteractor::createInteractivePart(Widget form)
{
    ScalarNode	*node;
    ScalarInstance		*si = (ScalarInstance*)
					this->interactorInstance;

    ASSERT(si);

    node = (ScalarNode*)si->getNode();

    ASSERT(form);
    ASSERT(node);

    this->componentForm = form;

    this->numberWidget = createNumberComponent(
		    this->componentForm,
		    si->isIntegerTypeComponent(),
		    si->getMinimum(1),
		    si->getMaximum(1),
		    si->getComponentValue(1),
		    si->getDecimals(1),
		    (XtCallbackProc)DialInteractor_DialCB,
		    1,
		    (void *)this);
    XtVaSetValues(this->numberWidget,
		XmNleftAttachment, 	XmATTACH_FORM,
		XmNleftOffset, 		5,
		XmNbottomAttachment, 	XmATTACH_FORM,
		XmNtopAttachment,	XmATTACH_NONE,
		XmNrightAttachment,	XmATTACH_FORM,
		XmNrightOffset, 	5,
        	XmNcharPlaces, 		10,
		NULL);
    /*
     * Set up and create the Dial component of this interactor
     */
    this->dialWidget = CreateDialComponent(
		    this->componentForm,
		    si->isIntegerTypeComponent(),
		    si->getMinimum(1),
		    si->getMaximum(1),
		    si->getComponentValue(1),
		    si->getDelta(1),
		    si->getDecimals(1),
		    (XtCallbackProc)DialInteractor_DialCB,
		    1,
		    (void *)this);
    XtVaSetValues(this->dialWidget,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		XmNtopAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNbottomWidget, this->numberWidget,
		NULL);

    return this->componentForm;

}

//
// Build a dial component with the given attributes.
//
static Widget CreateDialComponent(Widget  parent, 
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
    Arg    wargs[11];

    ASSERT(parent);
    ASSERT(valueChangedCallback);

    n = 0;
    DoubleSetArg(wargs[n], XmNposition,      value);      n++;
    DoubleSetArg(wargs[n], XmNminimum,       min);           n++;
    DoubleSetArg(wargs[n], XmNmaximum,       max);           n++;
    DoubleSetArg(wargs[n], XmNincrement,     delta);     n++;
    XtSetArg(wargs[n], XmNdecimalPlaces, decimalPlaces); n++;
    XtSetArg(wargs[n], XmNuserData,      comp_index);       n++;
    XtSetArg(wargs[n], XmNcharPlaces,    8);       n++;
 
    widget = XmCreateDial(parent, "dial", wargs, n);

    XtManageChild(widget);

    XtAddCallback(widget,
		  XmNselectCallback, 
		  valueChangedCallback,
		  clientData);

    return widget;
}

Widget createNumberComponent(Widget  parent,
			     boolean isInteger,
                             double  min,
                             double  max,
                             double  value,
                             int     decimalPlaces,
			     XtCallbackProc valueChangedCallback,
			     int     comp_index,
			     void * clientData)
{
    Widget widget;
    int    n;
    Arg    wargs[10];

    ASSERT(parent);
    ASSERT(valueChangedCallback);

    n = 0;

    if (isInteger) 	// An integer interactor
    {
    	XtSetArg(wargs[n], XmNdataType,      INTEGER);  n++;
    	XtSetArg(wargs[n], XmNdecimalPlaces, 0);        n++;
    	XtSetArg(wargs[n], XmNiMinimum,      (int)min);      n++;
    	XtSetArg(wargs[n], XmNiMaximum,      (int)max);      n++;
    	XtSetArg(wargs[n], XmNiValue,        (int)value);    n++;
    }
    else
    {
    	XtSetArg(wargs[n], XmNdataType,      DOUBLE);        n++;
    	XtSetArg(wargs[n], XmNdecimalPlaces, decimalPlaces); n++;
    	DoubleSetArg(wargs[n], XmNdMinimum,  min);          n++;
    	DoubleSetArg(wargs[n], XmNdMaximum,  max);          n++;
    	DoubleSetArg(wargs[n], XmNdValue,    value);        n++;
    }
    XtSetArg(wargs[n], XmNeditable,      True);          n++;
    XtSetArg(wargs[n], XmNuserData,      comp_index);      n++;
    XtSetArg(wargs[n], XmNfixedNotation, False);     n++;

    widget = XmCreateNumber(parent, "numberComponent", wargs, n);

    XtManageChild(widget);

    XtAddCallback(widget,
                  XmNactivateCallback,
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
extern "C" void DialInteractor_DialCB(Widget                  widget,
	       XtPointer		clientData,
 	       XtPointer		callData)
{
    DialInteractor *si = (DialInteractor*)clientData;
    int            component; 

    ASSERT(widget);

    component = (int)(long)GetUserData(widget);
    ASSERT(component > 0);
    si->dialCallback(widget, component, callData);
}

//
// Accepts value changes and reflects them into other interactors, cdbs
// and off course the interactor node output.
//
void DialInteractor::dialCallback(Widget                  widget,
		int	component,
	        XtPointer	callData)
{
    double value;

    ScalarInstance *si = (ScalarInstance*)this->interactorInstance;

    ASSERT(callData);
    ASSERT(si);
    
    boolean send;
    if(widget == this->numberWidget) {
    	value = ((XmDoubleCallbackStruct*)callData)->value;
   	send = TRUE;
    } else {
	XmDialCallbackStruct *dcb = (XmDialCallbackStruct*)callData;
    	value = dcb->position;
        send = (dcb->reason != XmCR_DRAG) || si->isContinuous();
	XmChangeNumberValue((XmNumberWidget)this->numberWidget, value);
    }

    if (send) {
        ScalarNode *node       = (ScalarNode*)si->getNode(); 
	si->setComponentValue(component, value);
	char *s = si->buildValueFromComponents();
	node->setOutputValue(1,s,DXType::UndefinedType, TRUE);
	delete s;

    }
}

//
// Update the displayed values for this interactor.
//
void DialInteractor::updateDisplayedInteractorValue()
{
    this->updateDialValue();
}
//
// Update the displayed values for the dial(s).
//
void DialInteractor::updateDialValue()
{
    Arg		   wargs[10];
    int		   i,components,n=0;
#ifdef PASSDOUBLEVALUE
XtArgVal dx_l;
#endif

    // FIXME: should check to make sure we have the correct class of node.
    ScalarInstance *si = (ScalarInstance*)this->interactorInstance;
    ASSERT(si);

    /*
     * For all components.
     */
    components = si->getComponentCount();

    for (i=1 ; i<=components; i++) {
	double value = si->getComponentValue(i);
        if (si->isIntegerTypeComponent())  {
	    XtSetArg(wargs[n], XmNiValue, (int)value); n++;
	} else {
	    DoubleSetArg(wargs[n], XmNdValue, value); n++;
	}
	/*
	 * Update the Dial.
	 */
	XtSetValues(this->numberWidget, wargs, n);
	XtVaSetValues(this->dialWidget, XmNposition, DoubleVal(value, dx_l), NULL);
    }

}

//
// Make sure the attributes match the resources for the widgets.
//
void DialInteractor::handleInteractivePartStateChange(
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

	/*
	 * Update the Number.
	 */
	n = 0;
        if (si->isIntegerTypeComponent())  {
	    XtSetArg(wargs[n], XmNiMaximum, (int)max); n++;
	    XtSetArg(wargs[n], XmNiMinimum, (int)min); n++;
	    XtSetArg(wargs[n], XmNiValueStep,    (int)delta);        n++;
	} else {
	    DoubleSetArg(wargs[n], XmNdMaximum, max); n++;
	    DoubleSetArg(wargs[n], XmNdMinimum, min); n++;
	    XtSetArg(wargs[n], XmNdecimalPlaces, decimals); n++;
	    DoubleSetArg(wargs[n], XmNdValueStep, delta);   n++;

	}
	XtSetValues(this->numberWidget, wargs, n);

	/*
	 * Update the Dial.
	 */
    	n = 0;
    	DoubleSetArg(wargs[n], XmNminimum,       min);           n++;
    	DoubleSetArg(wargs[n], XmNmaximum,       max);           n++;
    	DoubleSetArg(wargs[n], XmNincrement,     delta);     n++;
	XtSetValues(this->dialWidget, wargs, n);
    }

    this->resizeCB();
    this->updateDialValue();
}

