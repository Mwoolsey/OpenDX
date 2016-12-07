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

#include "SliderInteractor.h"
#include "SetScalarAttrDialog.h"
#include "InteractorStyle.h"

#include "ScalarNode.h"

#include "ScalarInstance.h"
#include "Application.h"
#include "ErrorDialogManager.h"
#include "../widgets/Slider.h"

boolean SliderInteractor::SliderInteractorClassInitialized = FALSE;

String SliderInteractor::DefaultResources[] =  {
    ".allowHorizontalResizing:	True",
    "*form*horizontalSpacing:	5",
    "*form*verticalSpacing:	5",
    "*Offset:			5",
    "*form.resizable:		False",
    "*recomputeSize: 		False",
    "*bboard*shadowThickness:   0",
    "*slider*topOffset:		2",
    "*slider*bottomOffset:	2",
    "*slider*leftOffset:	2",
    "*slider*rightOffset:	2",
    "*wwLeftOffset:		0",
    "*wwTopOffset:		0",

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

SliderInteractor::SliderInteractor(const char *name,
                      InteractorInstance *ii) : ScalarInteractor(name,ii)
{
    this->componentForm = NULL;
}
//
// One time class initializations.
//
void SliderInteractor::initialize()
{
    //
    // Initialize default resources (once only).
    //
    if (NOT SliderInteractor::SliderInteractorClassInitialized)
    {
        ASSERT(theApplication);
        this->setDefaultResources(theApplication->getRootWidget(),
                                  SliderInteractor::DefaultResources);
        this->setDefaultResources(theApplication->getRootWidget(),
                                  Interactor::DefaultResources);
        SliderInteractor::SliderInteractorClassInitialized = TRUE;
    }
}


static Widget CreateSliderComponent(Widget,  boolean, double, double,
				     double, double, int,   XtCallbackProc,   
				     int, void *) ;

//
// Allocate a slider for the given instance.
//
Interactor *SliderInteractor::AllocateInteractor(const char *name,
				InteractorInstance *ii)
{

    SliderInteractor *si = new SliderInteractor(name,ii);
    return (Interactor*)si;
}

//
// Perform anything that needs to be done after the parent of 
// this->interactivePart has been managed.
//
void SliderInteractor::completeInteractivePart()
{
    if (this->componentForm)
	this->passEvents(this->componentForm, TRUE);
}
//
// Build a slider  for the given instance.
//
Widget SliderInteractor::createInteractivePart(Widget form)
{
    ScalarNode	*node;
    ScalarInstance		*si = (ScalarInstance*)
					this->interactorInstance;

    ASSERT(si);

    node = (ScalarNode*)si->getNode();

    ASSERT(form);
    ASSERT(node);

	/*
	 * Set up and create the Slider component of this interactor
	 */
    this->sliderWidget = CreateSliderComponent(
		    form,
		    si->isIntegerTypeComponent(),
		    si->getMinimum(1),
		    si->getMaximum(1),
		    si->getComponentValue(1),
		    si->getDelta(1),
		    si->getDecimals(1),
		    (XtCallbackProc)SliderInteractor_SliderCB,
		    1,
		    (void *)this);

    return this->sliderWidget;
}

//
// Build a slider component with the given attributes.
//
static Widget CreateSliderComponent(Widget  parent, 
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
    if(isInteger)
    {
    	XtSetArg(wargs[n], XmNdataType,	INTEGER); n++;
    	XtSetArg(wargs[n], XmNdecimalPlaces, 0);  n++;
    }
    else
    {
    	XtSetArg(wargs[n], XmNdataType,	    DOUBLE);          n++;
    	XtSetArg(wargs[n], XmNdecimalPlaces, decimalPlaces);  n++;
    }

    DoubleSetArg(wargs[n], XmNcurrent,      value);         n++;
    DoubleSetArg(wargs[n], XmNminimum,      min);           n++;
    DoubleSetArg(wargs[n], XmNmaximum,      max);           n++;
    DoubleSetArg(wargs[n], XmNincrement,    delta);         n++;
    XtSetArg(wargs[n], XmNuserData,         comp_index);     n++;
    XtSetArg(wargs[n], XmNnoResize,         False);   	     n++;
    XtSetArg(wargs[n], XmNtopAttachment,    XmATTACH_FORM);  n++;
    XtSetArg(wargs[n], XmNbottomAttachment, XmATTACH_FORM);  n++;
    XtSetArg(wargs[n], XmNleftAttachment,   XmATTACH_FORM);  n++;
    XtSetArg(wargs[n], XmNrightAttachment,  XmATTACH_FORM);  n++;
 
    widget = XmCreateSlider(parent, "slider", wargs, n);

    XtManageChild(widget);

    XtAddCallback(widget,
		  XmNvalueCallback, 
		  valueChangedCallback,
		  clientData);

    XmSliderAddWarningCallback(widget, (XtCallbackProc)ScalarInteractor_NumberWarningCB, 
						(XtPointer)clientData);
    return widget;
}

//
// Call the virtual callback for a change in value. 
//
extern "C" void SliderInteractor_SliderCB(Widget                  widget,
	       XtPointer		clientData,
 	       XtPointer		callData)
{
    SliderInteractor *si = (SliderInteractor*)clientData;
    int            component; 

    ASSERT(widget);

    component = (int)(long)GetUserData(widget);
    ASSERT(component > 0);
    si->sliderCallback(widget, component, callData);
}

//
// Accepts value changes and reflects them into other interactors, cdbs
// and off course the interactor node output.
//
void SliderInteractor::sliderCallback(Widget                  widget,
		int	component,
	        XtPointer	callData)
{
    XmSliderCallbackStruct *cb = (XmSliderCallbackStruct*)callData;
    ScalarInstance *si = (ScalarInstance*)this->interactorInstance;

    ASSERT(callData);
    ASSERT(si);
    
    /*
     * Wait for relevant value (i.e. the button has been released or
     * there is continuous update), then get the interactor value, store it,
     * and send it.
     */
    boolean send = (cb->reason != XmCR_DRAG) || si->isContinuous();
    if (send) {
        double value = cb->value;
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
void SliderInteractor::updateDisplayedInteractorValue()
{
    this->updateSliderValue();
}
//
// Update the displayed values for the slider(s).
//
void SliderInteractor::updateSliderValue()
{
    Arg		   wargs[10];
    int		   i,components,n=0;
    // FIXME: should check to make sure we have the correct class of node.
    ScalarInstance *si = (ScalarInstance*)this->interactorInstance;
    ASSERT(si);

    /*
     * For all components.
     */
    components = si->getComponentCount();

    for (i=1 ; i<=components; i++) {
	double value = si->getComponentValue(i);

	DoubleSetArg(wargs[n], XmNcurrent, value); n++;
	XtSetValues(this->sliderWidget, wargs, n);
    }

}

//
// Make sure the attributes match the resources for the widgets.
//
void SliderInteractor::handleInteractivePartStateChange(
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
	 * Update the Slider.
	 */
	n = 0;
        if (NOT si->isIntegerTypeComponent()) 
	{
	    XtSetArg(wargs[n], XmNdecimalPlaces, decimals); n++;
	}

	DoubleSetArg(wargs[n], XmNmaximum, max); 		n++;
	DoubleSetArg(wargs[n], XmNminimum, min); 		n++;
	DoubleSetArg(wargs[n], XmNincrement,    delta);   n++;

	XtSetValues(this->sliderWidget, wargs, n);
    }

    this->updateSliderValue();
}

