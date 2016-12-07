/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#include <Xm/Form.h>
#include <Xm/Label.h>
#include <Xm/PushB.h>
#include <Xm/RowColumn.h>
#include <Xm/Scale.h>
#include <Xm/Separator.h>
#include <Xm/ToggleB.h>
#include "../widgets/Number.h"
#include "../widgets/Stepper.h"

#include "SetAttrDialog.h"
#include "SetScalarAttrDialog.h"
#include "ScalarInstance.h"
#include "InteractorStyle.h"
#include "LocalAttributes.h"
#include "Application.h"
#include "WarningDialogManager.h"

#ifdef ABS_IN_MATH_H
# define abs __Dont_define_abs
#endif
#include <math.h>
#ifdef ABS_IN_MATH_H
# undef abs
#endif


boolean SetScalarAttrDialog::ClassInitialized = FALSE;
String  SetScalarAttrDialog::DefaultResources[] =
{
    "*accelerators:		#augment\n"
#if 1
	"<Key>Return:			BulletinBoardReturn()",
#else
	"<Key>Return:			BulletinBoardReturn()\n"
	"<Btn2Down>,<Btn2Up>:		HELP!",
#endif
//    ".width: 		350", 	     // HP may have trouble with this 
 //   ".height: 		220", 	     // HP may have trouble with this 
//    "*resizePolicy: 	RESIZE_NONE",// HP may have trouble with this 

    "*maxLabel.labelString:Maximum:",
    "*maxLabel.topOffset:	10", 
    "*maxLabel.leftOffset:	10", 

    "*maxNumber.rightOffset:	10", 
    "*maxNumber.topOffset:	10", 
    "*maxNumber.editable:	True", 
    "*maxNumber.fixedNotation:	False", 

    "*minLabel.labelString:Minimum:",
    "*minLabel.topOffset:	10", 
    "*minLabel.leftOffset:	10", 

    "*minNumber.rightOffset:	10", 
    "*minNumber.topOffset:	10", 
    "*minNumber.editable:	True", 
    "*minNumber.fixedNotation:	False", 

    "*incrementOptions.topOffset: 	10",
    "*incrementOptions.rightOffset: 	20",
    "*incrementOptions.leftOffset: 	1",
    "*incrementPulldown*localIncrement*labelString: Local Increment",
    "*incrementPulldown*globalIncrement*labelString: Global Increment",

    "*incrementNumber.rightOffset:	10", 
    "*incrementNumber.topOffset:	10", 
    "*incrementNumber.editable:		True", 
    "*incrementNumber.fixedNotation:	False", 

    "*decimalsLabel.labelString:Decimal Places:",
    "*decimalsLabel.topOffset:		10", 
    "*decimalsLabel.leftOffset:		10", 

    "*decimalsStepper.topOffset:	10", 
    "*decimalsStepper.rightOffset:	10", 
    "*decimalsStepper.recomputeSize:	False", 
    "*decimalsStepper.editable:		True", 
    "*decimalsStepper.fixedNotation:	False", 

    "*updateOptions.topOffset: 		10",
    "*updateOptions.bottomOffset: 	5",
    "*updateOptions.leftOffset: 	1",
    "*updatePulldown*localUpdate*labelString:Local Update",
    "*updatePulldown*globalUpdate*labelString:Global Update",

    "*updateToggle.labelString:		Continuously", 
    "*updateToggle.topOffset:		10", 
    "*updateToggle.bottomOffset:	10", 
    "*updateToggle.rightOffset:		20", 
    "*updateToggle.shadowThickness: 	0",	
    "*updateToggle.set: 		False",	

    "*cancelButton.labelString:		Cancel", 
    "*cancelButton.topOffset:		10", 
    "*cancelButton.bottomOffset:	10", 
    "*cancelButton.rightOffset:		10", 
    "*cancelButton.recomputeSize:	False", 
    "*cancelButton.width: 		60",


    "*okButton.labelString:OK", 
    "*okButton.topOffset:		10", 
    "*okButton.bottomOffset:		10", 
    "*okButton.leftOffset:		10", 
    "*okButton.recomputeSize:		False", 
    "*okButton.width: 			60",

#if defined(aviion)
    "*incrementOptions.labelString:",
    "*updateOptions.labelString:",
#endif

    NULL
};

#if 0   // Use XmChangeNumberValue instead
static void SetNumberWidgetValue(Widget w, double val)
{
    short type;
    XtArgVal dx_l;

    XtVaGetValues(w, XmNdataType, &type, NULL);
    if (type == DOUBLE)
        XtVaSetValues(w, XmNdValue, DoubleVal(val, dx_l), NULL);
    else 
        XtVaSetValues(w, XmNiValue, (int)val, NULL);
    
}
static double GetNumberWidgetValue(Widget w)
{
    short type;
    int ivalue;
    double dvalue;

    XtVaGetValues(w, XmNdataType, &type, NULL);

    if (type == DOUBLE) {
        XtVaGetValues(w, XmNdValue, &dvalue, NULL);
    } else {
        XtVaGetValues(w, XmNiValue, &ivalue, NULL);
	dvalue = (double) ivalue;
    }
    return dvalue;
}
#endif

SetScalarAttrDialog::SetScalarAttrDialog(Widget 	  parent,
				     const char* title,
				     ScalarInstance *si):
    SetAttrDialog("scalarAttr", parent, title, (InteractorInstance*)si)
{
   this->initInstanceData(si);

    if (NOT SetScalarAttrDialog::ClassInitialized)
    {
        SetScalarAttrDialog::ClassInitialized = TRUE;
	this->installDefaultResources(theApplication->getRootWidget());
    }
}

SetScalarAttrDialog::SetScalarAttrDialog(const char *name,
					Widget 	  parent,
				     const char* title,
				     ScalarInstance *si):
    SetAttrDialog(name, parent, title, (InteractorInstance*)si)
{
   this->initInstanceData(si);
}


void SetScalarAttrDialog::initInstanceData(ScalarInstance *si)
{
    ScalarNode *sinode = ( ScalarNode*)si->getNode();
    int ncomp;
    // FIXME: should we assert that we have a ScalarNode?

    this->maxLabel = this->maxNumber = NULL;	
    this->minLabel = this->minNumber = NULL;	
    this->incrementOptions = this->incrementNumber = NULL;	
    this->globalIncrement = this->localIncrement = NULL;
    this->decimalsLabel = this->decimalsStepper = NULL;
    this->updateOptions = this->updateToggle = NULL;
    this->globalUpdate  = this->localUpdate = NULL;
    this->numComponents = ncomp = sinode->getComponentCount(); 
    ASSERT(ncomp > 0);
    this->attributes = new _ScalarAttr[ncomp];

}

SetScalarAttrDialog::~SetScalarAttrDialog()
{
    delete this->attributes;
}


//
// Called before the root widget is created.
//
//
// Install the default resources for this class.
//
void SetScalarAttrDialog::installDefaultResources(Widget  baseWidget)
{
    this->setDefaultResources(baseWidget,
				SetScalarAttrDialog::DefaultResources);
    this->SetAttrDialog::installDefaultResources( baseWidget);
}
//
// Desensitize the data-driven attributes in the dialog.
//
void SetScalarAttrDialog::setAttributeSensitivity()
{
    ScalarInstance *si = (ScalarInstance*) this->interactorInstance;
    ScalarNode *snode = (ScalarNode*)si->getNode();

    Boolean s = (snode->isMaximumVisuallyWriteable() ? True : False);
    XtSetSensitive(this->maxNumber,s);

    s = (snode->isMinimumVisuallyWriteable() ? True : False);
    XtSetSensitive(this->minNumber,s);

    InteractorStyleEnum style = si->getStyle()->getStyleEnum();
     
    if (si->isIntegerTypeComponent() || (style == TextStyle)) {
	s = False;
    } else {
        s = (snode->isDecimalsVisuallyWriteable() ? True : False);
    }
    XtSetSensitive(this->decimalsStepper,s);

    //
    // Get the current increment options menu selection (local or global).
    //
    Widget current_selection;
    XtVaGetValues(this->incrementOptions,XmNmenuHistory,
					&current_selection,NULL);

    if (style == TextStyle) {
	s = False;
    } else if (current_selection == this->localIncrement) {
	s = True; // The local increment is always enabled.
    } else {
        s = (snode->isDeltaVisuallyWriteable() ? True : False);
    }
    XtSetSensitive(this->incrementNumber,s);

    if (style == TextStyle) {
        XtSetSensitive(this->incrementOptions,False);
        XtSetSensitive(this->updateOptions,False);
        XtSetSensitive(this->updateToggle,False);
    } else {
        XtSetSensitive(this->incrementOptions,True);
        XtSetSensitive(this->updateOptions,True);
        XtSetSensitive(this->updateToggle,True);
    }
}

//
// Get the current component number that we are working on.
// Be default we always work on the first component (assumes a
// Scalar/Integer interactor).
//
int SetScalarAttrDialog::getCurrentComponentNumber()
{
   return 1;
}
//
// Indicate whether to update all attributes when a dialog attribute
// is changed.  At this level we return FALSE, but provide this 
// to allow others (i.e. SetVectorAttrDialog) to indicate that
// all values are to be updated. 
//
boolean SetScalarAttrDialog::isAllAttributesMode()
{
   return FALSE;
}

//
// Copy all the values from the given components dialog saved attributes 
// into the other dialog saved attributes.
// 
void SetScalarAttrDialog::updateAllAttributes(int component_index)
{
    int i;
    struct _ScalarAttr attr;

    ASSERT(this->numComponents >= component_index);
    ASSERT(component_index > 0);

    attr = this->attributes[component_index-1];
    for (i=1 ; i<=this->numComponents ; i++)
	if (i != component_index)
          this->attributes[i-1] = attr;
}

//
// Read the current attribute settings from the dialog and set them in  
// the given InteractorInstance indicated with this->interactorInstance and
// this->componentIndex.
//
boolean SetScalarAttrDialog::storeAttributes()
{
    int component_index, ncomp = this->numComponents;
    ScalarNode *sinode; 
    ScalarInstance *si = (ScalarInstance*) this->interactorInstance;
    double *mins = NULL;
    double *maxs = NULL; 

    ASSERT(si);
    sinode = (ScalarNode*)si->getNode();
    ASSERT(sinode);
    ASSERT(this->getRootWidget());

    //
    // The ScalarNode is somehow setting the real parameter values when
    // we install the new ranges.  So, lets just not install the new limit
    // if it not sensitive (which is when we don't want to override the set
    // value).
    //
    if (XtIsSensitive(this->minNumber))
	mins = new double[ncomp];
    if (XtIsSensitive(this->maxNumber))
    	maxs = new double[ncomp];

    for (component_index=1 ; component_index<=ncomp ; component_index++) {
	struct _ScalarAttr *attr = &this->attributes[component_index - 1];

	if (mins)
	    mins[component_index-1]  = attr->minimum;
	if (maxs)
	    maxs[component_index-1]  = attr->maximum;

	if (XtIsSensitive(this->decimalsStepper))
	    si->useGlobalDecimals(component_index, attr->decimals); 

	//
	// Get the Increment and whether or not it is applied locally. 
	//
	if (attr->usingGlobalIncrement) {
	    si->setLocalDelta(component_index,  attr->localIncrementValue);
	    si->useGlobalDelta(component_index, attr->globalIncrementValue);
	} else {
	    si->setGlobalDelta(component_index, attr->globalIncrementValue);
	    si->useLocalDelta(component_index,  attr->localIncrementValue);
	}

	//
	// Get the 'Continuousness' and see if it is applied locally. 
	// Yes, we do this for every component even when we don't need to.
	//
	if (this->usingGlobalContinuous) {
	    si->setLocalContinuous(this->localContinuousValue == True ?
					    TRUE : FALSE);
	    si->useGlobalContinuous(this->globalContinuousValue == True ?
					    TRUE : FALSE);
	} else {
	    si->setGlobalContinuous(this->globalContinuousValue == True ?
					    TRUE : FALSE);
	    si->useLocalContinuous(this->localContinuousValue == True ?
					TRUE : FALSE);
	}
    }
    //
    // Set these all at once as advised by ScalarNode so that we send a 
    // clamped value only once (for Vector, *List and VectorList). 
    //
    sinode->setAllComponentRanges(mins,maxs);
    if (mins) delete mins;
    if (maxs) delete maxs;

    return TRUE;
}
//
// Display the current saved attribute settings 
//
void SetScalarAttrDialog::updateDisplayedAttributes()
{
     int i = this->getCurrentComponentNumber();
     this->updateDisplayedComponentAttributes(i);
}

void SetScalarAttrDialog::loadAttributes()
{
    int i, ncomp = this->numComponents;
    ScalarInstance *si = (ScalarInstance*) this->interactorInstance;

    for (i=1 ; i<= ncomp ; i++) {
	struct _ScalarAttr *attr = &this->attributes[i-1];

	attr->maximum  = si->getMaximum(i); 
	attr->minimum  = si->getMinimum(i); 
	attr->decimals = si->getDecimals(i);

	attr->usingGlobalIncrement = !si->isLocalDelta(i);
	attr->globalIncrementValue = si->getGlobalDelta(i);  
	attr->localIncrementValue = si->getLocalDelta(i);

    }
    this->usingGlobalContinuous = si->usingGlobalContinuous();
    this->localContinuousValue  = (si->getLocalContinuous()  ? 
					    True : False);
    this->globalContinuousValue = (si->getGlobalContinuous() ? 
					    True : False);

}
void SetScalarAttrDialog::updateDisplayedComponentAttributes(
						int component_index)
{
    ScalarInstance *si = (ScalarInstance*) this->interactorInstance;
    ScalarNode *sinode; 
    struct _ScalarAttr *attr = &this->attributes[component_index - 1];
    double min, max, delta; 
    int	   decimals;

    ASSERT(si);
    ASSERT((component_index > 0) && (this->numComponents >= component_index));
    ASSERT( (sinode = (ScalarNode*)si->getNode()) &&
    	    (component_index <= sinode->getComponentCount()));

    //
    // Install the values in the widgets
    //
    ASSERT(this->maxLabel);

    boolean   is_integer = si->isIntegerTypeComponent();
    max = attr->maximum;
    min = attr->minimum;
    decimals = attr->decimals;


    if (attr->usingGlobalIncrement)
	delta = attr->globalIncrementValue;
    else
	delta = attr->localIncrementValue;
    

    if (is_integer) {
	int imin   = (int)min;
	int imax   = (int)max;
	int idelta = (int)delta;
        XtVaSetValues(this->maxNumber,       XmNdataType,  INTEGER,
        				     XmNiValue,    imax, NULL);
        XtVaSetValues(this->minNumber,       XmNdataType,  INTEGER,
				   	     XmNiValue,    imin, NULL);
        XtVaSetValues(this->incrementNumber, XmNdataType,  INTEGER,
				   	     XmNiValue,    idelta, 
					     NULL);

    } else {
#ifdef PASSDOUBLEVALUE
	XtArgVal dx_l;
#endif
        XtVaSetValues(this->maxNumber,       XmNdataType, DOUBLE, 
        				     XmNdValue,   DoubleVal(max, dx_l), 
        				     XmNdecimalPlaces,  decimals, 
					     NULL);
        XtVaSetValues(this->minNumber,       XmNdataType,  DOUBLE, 
				   	     XmNdValue,    DoubleVal(min, dx_l),
        				     XmNdecimalPlaces,  decimals, 
					     NULL);
        XtVaSetValues(this->incrementNumber, XmNdataType,  DOUBLE, 
				   	     XmNdValue,    DoubleVal(delta, dx_l), 
        				     XmNdecimalPlaces,  decimals, 
					     NULL);
    }
    XtVaSetValues(this->decimalsStepper, XmNiValue,    decimals, NULL);

    //
    // Set the increment pull down menu option.
    //
    XtVaSetValues(this->incrementOptions,
	XmNmenuHistory, (attr->usingGlobalIncrement ? 
			 this->globalIncrement : this->localIncrement),
	NULL);

    //
    // Set the continuous update toggle button. 
    //
    Boolean continuous;
    if (this->usingGlobalContinuous) {
	continuous = this->globalContinuousValue; 
    } else {
	continuous = this->localContinuousValue; 
    }
    XtVaSetValues(this->updateToggle, XmNset, continuous , NULL);

    //
    // Set the update pull down menu option.
    //
    XtVaSetValues(this->updateOptions,
	XmNmenuHistory, (this->usingGlobalContinuous ? 
			 this->globalUpdate : this->localUpdate),
	NULL);

    //
    // Set the sensitivity of the widgets.
    //
    this->setAttributeSensitivity();

    //
    // If the node is one of min or max is not visually writeable
    // (i.e. it will be grayed-out) and the other is not, the set the
    // appropriate bound on the ungrayed-out one. 
    //
    boolean min_gray = !XtIsSensitive(this->minNumber); 
    boolean max_gray = !XtIsSensitive(this->maxNumber); 
    double min_of_min, min_of_max, max_of_min, max_of_max;
    XmGetNumberBounds((XmNumberWidget)this->minNumber, 
				&min_of_min, &max_of_min);
    XmGetNumberBounds((XmNumberWidget)this->maxNumber, 
				&min_of_max, &max_of_max);
    XmUnlimitNumberBounds((XmNumberWidget)this->minNumber); 
    XmUnlimitNumberBounds((XmNumberWidget)this->maxNumber); 
    if (min_gray && !max_gray) {
	// Limit minimum of max 
	min_of_max = XmGetNumberValue((XmNumberWidget)this->minNumber);
	XmSetNumberBounds((XmNumberWidget)this->maxNumber, 
				min_of_max, max_of_max);
    } else if (max_gray && !min_gray) {
	// Limit maximum of min 
	max_of_min = XmGetNumberValue((XmNumberWidget)this->maxNumber);
	XmSetNumberBounds((XmNumberWidget)this->minNumber, 
				min_of_min, max_of_min);
    } 
    this->adjustIncrementBounds();
}

//
// Make sure the range of values accepted in the increment number
// widget is goes from 0 to (current_max - current_min);  
//
void SetScalarAttrDialog::adjustIncrementBounds()
{
    double min = XmGetNumberValue((XmNumberWidget)this->minNumber);
    double max = XmGetNumberValue((XmNumberWidget)this->maxNumber);
    double incr = XmGetNumberValue((XmNumberWidget)this->incrementNumber);
    double max_incr = max - min;
    if (incr > max_incr)
	XmChangeNumberValue((XmNumberWidget)this->incrementNumber,max_incr);
    XmSetNumberBounds((XmNumberWidget)this->incrementNumber, 0.0, max_incr);
}
//
// Make sure the display increment value matches the options menu selection. 
//
extern "C" void SetScalarAttrDialog_IncrementOptionsCB(Widget w, XtPointer clientData,
				XtPointer callData)
{
    SetScalarAttrDialog *sad = (SetScalarAttrDialog*)clientData;
    double nextValue; 
    Widget current_selection;
    struct _ScalarAttr *attr = 
			&sad->attributes[sad->getCurrentComponentNumber()-1];

    ASSERT((w == sad->globalIncrement) || (w == sad->localIncrement));

    //
    // Get the current increment options menu selection (local or global). 
    //
    XtVaGetValues(sad->incrementOptions, 
			XmNmenuHistory, &current_selection, NULL);

    //
    // Get the increment value that should be displayed for this menu selection.
    //
    Boolean sensitive;
    if (w == sad->globalIncrement)  {
	ScalarInstance *si = (ScalarInstance*)sad->interactorInstance;
        ScalarNode *snode = (ScalarNode*)si->getNode();
        attr->usingGlobalIncrement = TRUE; 
	nextValue = attr->globalIncrementValue;
	sensitive = (snode->isDeltaVisuallyWriteable() ? True : False); 
    } else  {
        attr->usingGlobalIncrement = FALSE; 
	nextValue = attr->localIncrementValue;
	sensitive = True;
    }
    
    //
    // Change the increment value. 
    //
    XmChangeNumberValue((XmNumberWidget)sad->incrementNumber, nextValue);
    XtSetSensitive(sad->incrementNumber, sensitive);
    sad->adjustIncrementBounds();

    //
    // If we are supposed to update all attributes then do so. 
    //
    if (sad->isAllAttributesMode())
	sad->updateAllAttributes(sad->getCurrentComponentNumber());

}
extern "C" void SetScalarAttrDialog_NumberCB(Widget w, XtPointer clientData,
				XtPointer callData)
{
    double min, max, inc;
    SetScalarAttrDialog *sad = (SetScalarAttrDialog*)clientData;
    struct _ScalarAttr *attr = 
			&sad->attributes[sad->getCurrentComponentNumber()-1];

    ASSERT((w == sad->maxNumber) || 
	   (w == sad->minNumber) ||
	   (w == sad->incrementNumber));

    min = XmGetNumberValue((XmNumberWidget)sad->minNumber);
    max = XmGetNumberValue((XmNumberWidget)sad->maxNumber);
    inc = XmGetNumberValue((XmNumberWidget)sad->incrementNumber);

    if (w == sad->maxNumber) {
	if (max <= min) {
	   attr->minimum = max-inc;
	   XmChangeNumberValue((XmNumberWidget)sad->minNumber,attr->minimum);
	}
	attr->maximum = max;
	sad->adjustIncrementBounds();
    } else if (w == sad->minNumber) {
	if (max <= min) {
	   attr->maximum = min+inc;
	   XmChangeNumberValue((XmNumberWidget)sad->maxNumber,attr->maximum);
	}
	attr->minimum = min;
	sad->adjustIncrementBounds();
    } else if (w == sad->incrementNumber) {
	//
 	// Store the change for IncrementOptionsCB(). 
 	//
	Widget current_selection;
        XtVaGetValues(sad->incrementOptions, 
			XmNmenuHistory, &current_selection, NULL);

	ASSERT((sad->globalIncrement == current_selection) || 
	       (sad->localIncrement == current_selection));

        if (current_selection == sad->globalIncrement) {
	    attr->globalIncrementValue = inc;
        } else {
	    attr->localIncrementValue = inc;
	}

    } 

    //
    // If we are supposed to update all attributes then do so. 
    //
    if (sad->isAllAttributesMode()) {
	sad->updateAllAttributes(sad->getCurrentComponentNumber());
    }
}
//
// Swap the saved and displayed toggle values.
//
extern "C" void SetScalarAttrDialog_UpdateOptionsCB(Widget w, XtPointer clientData,
				XtPointer callData)
{
    SetScalarAttrDialog *sad = (SetScalarAttrDialog*)clientData;
    Boolean  currentValue, nextValue;

    ASSERT((w == sad->globalUpdate) || (w == sad->localUpdate));

    //
    // Get the toggle value that should be displayed for this menu selection.
    //
    if (w == sad->globalUpdate)  {
	sad->usingGlobalContinuous = TRUE;
	nextValue = sad->globalContinuousValue;
    } else {
	sad->usingGlobalContinuous = FALSE;
	nextValue = sad->localContinuousValue;
    }
    
    //
    // Change the toggle value if it needs to be changed. 
    //
    XtVaGetValues(sad->updateToggle, XmNset, &currentValue, NULL);
    if (currentValue != nextValue)  {
	XtVaSetValues(sad->updateToggle, XmNset, nextValue, NULL);
     }

}
extern "C" void SetScalarAttrDialog_ToggleCB(Widget w, XtPointer clientData,
				XtPointer callData)
{
    Boolean set;
    Widget current_selection;
    SetScalarAttrDialog *sad = (SetScalarAttrDialog*)clientData;

    ASSERT(sad->updateToggle == w);

    XtVaGetValues(sad->updateOptions, 
			XmNmenuHistory, &current_selection, NULL);

    ASSERT((sad->globalUpdate == current_selection) || 
	   (sad->localUpdate  == current_selection));

    XtVaGetValues(sad->updateToggle, XmNset, &set, NULL);

    if (current_selection == sad->globalUpdate)  {
	sad->globalContinuousValue = set;
    } else {
	sad->localContinuousValue = set;
    }

}

extern "C" void SetScalarAttrDialog_StepperCB(Widget w, XtPointer clientData,
				XtPointer callData)
{
    SetScalarAttrDialog *sad = (SetScalarAttrDialog*)clientData;
    struct _ScalarAttr *attr = 
		&sad->attributes[sad->getCurrentComponentNumber()-1];
    int decimals = (int)XmGetNumberValue((XmNumberWidget)sad->decimalsStepper);

    attr->decimals = decimals;
    XtVaSetValues(sad->maxNumber,       XmNdecimalPlaces, decimals, NULL);
    XtVaSetValues(sad->minNumber,       XmNdecimalPlaces, decimals, NULL);
    XtVaSetValues(sad->incrementNumber, XmNdecimalPlaces, decimals,
					NULL);

    //
    // If we are supposed to update all attributes then do so. 
    //
    if (sad->isAllAttributesMode())
	sad->updateAllAttributes(sad->getCurrentComponentNumber());
}


boolean SetScalarAttrDialog::okCallback(Dialog* dialog)
{
   boolean r = this->SetAttrDialog::okCallback(dialog);
#ifdef DEBUG
   printf("Writing resources to resources.dx\n");
   Application::DumpApplicationResources("resources.dx");
#endif
   return r;
}

Widget SetScalarAttrDialog::createIncrementPulldown(Widget parent,
							const char *name)
{
    Widget pulldown = XmCreatePulldownMenu(parent, (char*)name, NULL, 0);

    this->localIncrement = XtVaCreateManagedWidget(
		"localIncrement",xmPushButtonWidgetClass,pulldown,
		NULL);
    this->globalIncrement = XtVaCreateManagedWidget(
		"globalIncrement",xmPushButtonWidgetClass,pulldown,
		NULL);

    XtAddCallback(this->localIncrement,XmNarmCallback,
		(XtCallbackProc)SetScalarAttrDialog_IncrementOptionsCB, (XtPointer)this);
    XtAddCallback(this->globalIncrement,XmNarmCallback,
		(XtCallbackProc)SetScalarAttrDialog_IncrementOptionsCB, (XtPointer)this);
    return pulldown;
}

Widget SetScalarAttrDialog::createUpdatePulldown(Widget parent, 
						const char *name)
{
    Widget pulldown = XmCreatePulldownMenu(parent,(char *)name,NULL,0);

    this->localUpdate = XtVaCreateManagedWidget(
		"localUpdate",xmPushButtonWidgetClass,pulldown,
		NULL);

    this->globalUpdate = XtVaCreateManagedWidget(
		"globalUpdate",xmPushButtonWidgetClass,pulldown,
		NULL);
			
    XtAddCallback(this->localUpdate, XmNarmCallback,
			(XtCallbackProc)SetScalarAttrDialog_UpdateOptionsCB, (XtPointer)this);
    XtAddCallback(this->globalUpdate, XmNarmCallback,
			(XtCallbackProc)SetScalarAttrDialog_UpdateOptionsCB, (XtPointer)this);

    return pulldown;
}



void SetScalarAttrDialog::createAttributesPart(Widget mainForm)
{
    ScalarInstance *si = (ScalarInstance*) this->interactorInstance;

    /////////////////////////////////////////////////////////////
    // Create the widgets from left to right and top to bottom.//
    /////////////////////////////////////////////////////////////

    //
    // The Maximum label and number widget 
    //
    this->maxLabel = XtVaCreateManagedWidget(
	"maxLabel", xmLabelWidgetClass, mainForm,NULL);

    this->maxNumber = 
	XtVaCreateManagedWidget(
       "maxNumber", xmNumberWidgetClass, mainForm, 
	XmNdataType, (si->isIntegerTypeComponent() ? INTEGER : DOUBLE),
	XmNcharPlaces, 18,		// Apparently has to be hard-code 
	XmNrecomputeSize, False,	// Apparently has to be hard-code 
	NULL);

    XtAddCallback(this->maxNumber, XmNactivateCallback,
	(XtCallbackProc)SetScalarAttrDialog_NumberCB, (XtPointer)this);
    XtAddCallback(this->maxNumber,
                  XmNwarningCallback,
                  (XtCallbackProc)SetScalarAttrDialog_NumberWarningCB,
                  (XtPointer)this);

    //
    // The Minimum label and number widget 
    //
    this->minLabel = XtVaCreateManagedWidget( 
	"minLabel", xmLabelWidgetClass, mainForm,  NULL);

    this->minNumber = XtVaCreateManagedWidget(
       "minNumber", xmNumberWidgetClass, mainForm,
	XmNdataType, (si->isIntegerTypeComponent() ? INTEGER : DOUBLE),
	XmNcharPlaces, 18,		// Apparently has to be hard-code 
	XmNrecomputeSize, False,	// Apparently has to be hard-code 
	NULL);

    XtAddCallback(this->minNumber, XmNactivateCallback,
	(XtCallbackProc)SetScalarAttrDialog_NumberCB, (XtPointer)this);
    XtAddCallback(this->minNumber,
                  XmNwarningCallback,
                  (XtCallbackProc)SetScalarAttrDialog_NumberWarningCB,
                  (XtPointer)this);

    //
    // The Increment pulldown and number widget
    //
    Widget pulldown = this->createIncrementPulldown(mainForm,
						"incrementPulldown");
    this->incrementOptions = XtVaCreateManagedWidget(
		"incrementOptions", xmRowColumnWidgetClass, mainForm, 
		XmNentryAlignment  , XmALIGNMENT_CENTER,
		XmNrowColumnType   , XmMENU_OPTION,
		XmNsubMenuId       , pulldown,
		NULL);

	
    this->incrementNumber = XtVaCreateManagedWidget(
       "incrementNumber", xmNumberWidgetClass, mainForm, 
	XmNdataType, (si->isIntegerTypeComponent() ? INTEGER : DOUBLE),
	XmNcharPlaces, 18,		// Apparently has to be hard-code 
	XmNrecomputeSize, False,	// Apparently has to be hard-code 
	NULL);

    XtAddCallback(this->incrementNumber, XmNactivateCallback,
	(XtCallbackProc)SetScalarAttrDialog_NumberCB, (XtPointer)this);
    XtAddCallback(this->incrementNumber,
                  XmNwarningCallback,
                  (XtCallbackProc)SetScalarAttrDialog_NumberWarningCB,
                  (XtPointer)this);

    //
    // The Decimal places label and stepper widget
    //
    this->decimalsLabel = XtVaCreateManagedWidget( 
	"decimalsLabel", xmLabelWidgetClass, mainForm, NULL);


    this->decimalsStepper = XtVaCreateManagedWidget(
	"decimalsStepper", xmStepperWidgetClass, mainForm, 
		XmNdataType      ,	INTEGER, 	
		XmNiValue        ,	2, 
		XmNiMinimum      ,	0, 
		XmNiMaximum      ,	6, 
		XmNdecimalPlaces,	0, 
		NULL);



    XtAddCallback(this->decimalsStepper, XmNactivateCallback,
	(XtCallbackProc)SetScalarAttrDialog_StepperCB, (XtPointer)this);
    XtAddCallback(this->decimalsStepper,
                  XmNwarningCallback,
                  (XtCallbackProc)SetScalarAttrDialog_NumberWarningCB,
                  (XtPointer)this);


    //
    // The Update pulldown and toggle button 
    //
    pulldown = this->createUpdatePulldown(mainForm, "updatePulldown");
    this->updateOptions = XtVaCreateManagedWidget(
		"updateOptions", xmRowColumnWidgetClass, mainForm, 
		XmNentryAlignment  , XmALIGNMENT_CENTER,
		XmNrowColumnType   , XmMENU_OPTION,
		XmNsubMenuId       , pulldown,
		NULL);

    this->updateToggle = XtVaCreateManagedWidget("updateToggle",
        xmToggleButtonWidgetClass, mainForm, NULL);

    XtAddCallback(this->updateToggle,
                  XmNvalueChangedCallback,
                  (XtCallbackProc)SetScalarAttrDialog_ToggleCB,
                  (XtPointer)this);

    this->loadAttributes();
    this->updateDisplayedAttributes();

    //
    // The OK and CANCEL buttons. 
    //
    Widget separator = XtVaCreateManagedWidget(
        "separator", xmSeparatorWidgetClass, mainForm, NULL);


    this->ok =  XtVaCreateManagedWidget(
        "okButton", xmPushButtonWidgetClass, mainForm, NULL);


    this->cancel =  XtVaCreateManagedWidget(
        "cancelButton", xmPushButtonWidgetClass, mainForm, NULL);


    ///////////////////////////////////
    // And now build the attachments //
    ///////////////////////////////////


    XtVaSetValues(this->maxLabel,
	XmNtopAttachment   , XmATTACH_FORM,
        XmNleftAttachment  , XmATTACH_FORM,
	NULL);
    XtVaSetValues(this->maxNumber,
	XmNtopAttachment   , XmATTACH_FORM,
        XmNrightAttachment , XmATTACH_FORM,
	NULL);
    XtVaSetValues(this->minLabel,
	XmNtopAttachment   , XmATTACH_WIDGET,
	XmNtopWidget       , this->maxNumber,
        XmNleftAttachment  , XmATTACH_FORM,
	NULL);
    XtVaSetValues(this->minNumber,
	XmNtopAttachment   , XmATTACH_WIDGET,
	XmNtopWidget       , this->maxNumber,
        XmNrightAttachment , XmATTACH_FORM,
	NULL);
    XtVaSetValues(this->incrementOptions,
	XmNtopAttachment   , XmATTACH_WIDGET,
	XmNtopWidget       , this->minNumber,
        XmNleftAttachment  , XmATTACH_FORM,
        XmNrightAttachment , XmATTACH_WIDGET,
        XmNrightWidget     , this->incrementNumber,
	NULL);
    XtVaSetValues(this->incrementNumber,
	XmNtopAttachment   , XmATTACH_WIDGET,
	XmNtopWidget       , this->minNumber,
        XmNrightAttachment , XmATTACH_FORM,
	NULL);
    XtVaSetValues(this->decimalsLabel,
	XmNtopAttachment   , XmATTACH_WIDGET,
	XmNtopWidget       , this->incrementNumber,
        XmNleftAttachment  , XmATTACH_FORM,
	NULL);
    XtVaSetValues(this->decimalsStepper,
	XmNrightAttachment , XmATTACH_FORM,
	XmNtopAttachment   , XmATTACH_WIDGET,
	XmNtopWidget       , this->incrementNumber,
	NULL);
    XtVaSetValues(this->updateOptions,
	XmNtopAttachment   , XmATTACH_WIDGET,
	XmNtopWidget       , this->decimalsStepper,
        XmNleftAttachment  , XmATTACH_FORM,
	NULL);
    XtVaSetValues(this->updateToggle,
        XmNtopAttachment   , XmATTACH_WIDGET,
        XmNtopWidget       , this->decimalsStepper, 
        XmNrightAttachment , XmATTACH_FORM,
        NULL);
    XtVaSetValues(separator,
        XmNtopAttachment   , XmATTACH_WIDGET,
        XmNtopWidget       , this->updateOptions,
        XmNleftAttachment  , XmATTACH_FORM,
        XmNrightAttachment , XmATTACH_FORM,
        NULL);
    XtVaSetValues(this->ok,
        XmNtopAttachment   , XmATTACH_WIDGET,
        XmNtopWidget       , separator,
        XmNleftAttachment  , XmATTACH_FORM,
        XmNbottomAttachment, XmATTACH_FORM,
        NULL);
    XtVaSetValues(this->cancel,
        XmNtopAttachment   , XmATTACH_WIDGET,
        XmNtopWidget       , separator,
        XmNrightAttachment , XmATTACH_FORM,
        XmNbottomAttachment, XmATTACH_FORM,
        NULL);

}

extern "C" void SetScalarAttrDialog_NumberWarningCB(Widget w,
                        XtPointer clientData, XtPointer callData)
{
    SetScalarAttrDialog *sad = (SetScalarAttrDialog*)clientData;
    XmNumberWarningCallbackStruct* warning;
    warning = (XmNumberWarningCallbackStruct*)callData;

    if(w != sad->incrementNumber)
    {
        ModalWarningMessage(sad->getRootWidget(), warning->message);
        return;
    }

    //
    // Insert "increment" in the message for the increment number widget.
    //
    char *message = new char[STRLEN(warning->message) + 15];
    strcpy(message, warning->message);
    char *colon   = strchr(message, ':');

    if(colon)
    {
        colon[0] = '\0';
        strcat(message, " increment");
        colon = strchr(warning->message, ':');
        strcat(message, colon);
    }

    ModalWarningMessage(sad->getRootWidget(), message);

    delete message;
}

