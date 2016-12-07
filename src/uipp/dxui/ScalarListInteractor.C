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
#include <Xm/Frame.h>
#include <Xm/PushB.h>
#include <Xm/Form.h>
#include <Xm/Label.h>


#ifdef ABS_IN_MATH_H
# define abs __Dont_define_abs
#endif
#include <math.h>
#ifdef ABS_IN_MATH_H
# undef abs
#endif

#include "lex.h"
#include "ScalarListInteractor.h"
#include "ScalarListNode.h"
#include "SetScalarAttrDialog.h"
#include "InteractorStyle.h"

#include "ScalarNode.h"

#include "DXValue.h"
#include "ScalarListInstance.h"
#include "Application.h"
#include "ErrorDialogManager.h"
#include "../widgets/Stepper.h"
#include "../widgets/NumericList.h"

boolean ScalarListInteractor::ScalarListInteractorClassInitialized = FALSE;

String ScalarListInteractor::DefaultResources[] =  {
    ".AllowResizing:			True",
    "*Offset:		   		5",
    "*valueList.shadowThickness:	1",
    "*frame.shadowThickness:		2",
    "*frame.marginWidth:		3",
    "*frame.marginHeight:		3",
    "*addButton.width:       		60",
    "*deleteButton.width:       	60",
    "*addButton.labelString:		Add",
    "*deleteButton.labelString:		Delete",
    "*XmStepper.alignment:		XmALIGNMENT_CENTER",
    "*XmStepper.leftAttachment:		XmATTACH_FORM",
    "*XmStepper.rightAttachment:	XmATTACH_FORM",
    "*steppers_form.stepperComponent.leftOffset:	10",
    "*steppers_form.stepperComponent.rightOffset:	10",
    "*pinLeftRight:			True",
    "*pinTopBottom:			True",
    "*wwLeftOffset:			0",
    "*wwRightOffset:			0",
    "*wwTopOffset:			0",
    "*wwBottomOffset:			0",
    NUL(char*) 
};
#define UNSELECTED_INDEX -1
ScalarListInteractor::ScalarListInteractor(const char *name, 
			InteractorInstance *ii) : StepperInteractor(name,ii) 
{ 
    this->selectedItemIndex = UNSELECTED_INDEX;
    this->addButton = NULL;
    this->deleteButton = NULL;
    this->valueList = NULL;
    this->steppers = NULL;
}

//
// One time class initializations.
//
void ScalarListInteractor::initialize()
{
    //
    // Initialize default resources (once only).
    //
    if (NOT ScalarListInteractor::ScalarListInteractorClassInitialized)
    {
        ASSERT(theApplication);
        this->setDefaultResources(theApplication->getRootWidget(),
                                  ScalarListInteractor::DefaultResources);
        this->setDefaultResources(theApplication->getRootWidget(),
                                  StepperInteractor::DefaultResources);
        this->setDefaultResources(theApplication->getRootWidget(),
                                  Interactor::DefaultResources);
        ScalarListInteractor::ScalarListInteractorClassInitialized = TRUE;
    }
}



/////////////////////////// StepperInteractor methods ////////////////////////

//
// Allocate an n-dimensional stepper for the given instance.
//
Interactor *ScalarListInteractor::AllocateInteractor(const char *name,
					InteractorInstance *ii)
{
    ScalarListInteractor *si = new ScalarListInteractor(name,ii);
    return (Interactor*)si;
}

//
// 
//
void ScalarListInteractor::stepperCallback(Widget widget,
			    int component, XtPointer callData)
{
    ScalarListInstance		*sli = (ScalarListInstance*)
					this->interactorInstance;
    XmDoubleCallbackStruct *cb = (XmDoubleCallbackStruct*) callData;

    /*
     * Get the interactor value, store it.
     */
    ASSERT(sli);
    sli->setComponentValue(component, (double)cb->value);
    
    //
    // Update the list (if an item was selected and the user has released the
    // mouse button) . 
    //
    boolean update = (this->selectedItemIndex != UNSELECTED_INDEX) &&
		      ((cb->reason == XmCR_ACTIVATE) || sli->isContinuous());

    if (update) {
	Vector vec;
        ScalarListNode *sln = (ScalarListNode*)sli->getNode();
	int i, n_tuples = sln->getComponentCount();
	vec = new double[n_tuples];
	for (i=0 ; i<n_tuples ; i++)
	    vec[i] = sli->getComponentValue(i+1);
	
	XmNumericListReplaceSelectedVector((XmNumericListWidget)
							this->valueList,vec);
	delete vec;

	/*
	 * Get the interactor value, store it, and send it.
	 */
	char *s = this->buildDXList();
	sln->setOutputValue(1,s, DXType::UndefinedType, TRUE);
	if (s) delete s;
    }

}

//
// Perform anything that needs to be done after the parent of
// this->interactivePart has been managed.
//
void ScalarListInteractor::completeInteractivePart()
{
    this->passEvents(this->listForm, TRUE);
    this->passEvents(this->steppers, TRUE);
    this->StepperInteractor::completeInteractivePart();
    this->layoutInteractor();
}

//
// Update the displayed values for this interactor (i.e. the list).
//
void ScalarListInteractor::updateDisplayedInteractorValue()
{
    int		i;
    char 	*s;
    VectorList	vectors = NULL;
    Type 	type;
    int 	n_tuples, count, index;
    

    ScalarListInstance *sli = (ScalarListInstance*)this->interactorInstance;
    ASSERT(sli);
    ScalarListNode *sln = (ScalarListNode*)sli->getNode();
    n_tuples = sln->getComponentCount(); 

#if 0
    if (n_tuples > 1) {
	type = DXType::VectorListType;
    } else {
	if (sln->isIntegerTypeComponent())
	    type = DXType::IntegerListType;
	else
	    type = DXType::ScalarListType;
    }
#else
    type = sln->getTheCurrentOutputType(1);
#endif
  
    const char *val = sln->getOutputValueString(1); 	// Get the list itself
    index = -1;
    count = 0;
    while ( (s = DXValue::NextListItem(val, &index, type, NULL)) ) {
	// 
	// Get the individual components out of a vector.
	// This is a very slow, but at least uses DXValue to do the parse.
	// 
	DXValue v;
	v.setValue(s,type & ~DXType::ListType);
	if ((count & 15) == 0)
	    vectors = (VectorList)REALLOC(vectors,(count + 16)*sizeof(Vector));

	vectors[count] = new double[n_tuples];

	// 
	// Get all the (vector) components
	// 
	for (i=1 ; i<=n_tuples ; i++) {
	    double dval = 0; 
	    switch (type) {
	        case DXType::IntegerListType:
			dval = v.getInteger(); break;
	        case DXType::ScalarListType:
			dval = v.getScalar(); break;
	        case DXType::VectorListType:
			dval = v.getVectorComponentValue(i); break;
	    }

#if 0	// 9/22/93 - doRangeCheck...() handles this now.
	    double min = sln->getComponentMinimum(i);
	    double max = sln->getComponentMaximum(i);
	    if (dval > max)
		dval = max;
	    else if (dval < min)
		dval = min;
#endif

	    vectors[count][i-1] = dval;
	}

	// Next vector
	count++;
  	delete s;
    }

    int *decimal_places = new int[n_tuples];
    for (i=0 ; i<n_tuples ; i++) 
	decimal_places[i] = sln->getComponentDecimals(i+1);
    

    XtVaSetValues(this->valueList, 
		    XmNvectorCount,   count,
		    XmNtuples,        n_tuples,
		    XmNdecimalPlaces, decimal_places,
		    XmNvectors,       vectors,
		    NULL);

    delete decimal_places;

    for (i=count-1 ; i>=0 ; i--)
	FREE((void*)vectors[i]);

    FREE((void*)vectors);
    
  
}
//
// Make sure the resources (including the value) match the attributes 
// for the widgets. 
//
void ScalarListInteractor::handleInteractivePartStateChange(
                                        InteractorInstance *src_ii,
					boolean major_change)
{
    int n_tuples, count, j, i, *decimalPlaces;
    ScalarListInstance *sli = (ScalarListInstance*)this->interactorInstance;
    VectorList ro_vectors, vectors;


    if (major_change)
	this->unmanage();

#define NEEDCLAMP 0
#if NEEDCLAMP // Not need after adding ScalarNode::rangeCheckComponentValue 6/3/93
    boolean clamped_vector = FALSE;
#endif
 
    //
    // Update the stepper.
    //
    this->StepperInteractor::handleInteractivePartStateChange(src_ii, FALSE);

    XtVaGetValues(this->valueList,
			XmNreadonlyVectors, &ro_vectors,
			XmNtuples,	&n_tuples,
			XmNvectorCount, &count,
			NULL);

    //
    // Update the min, max and decimals for the list items.
    //
    if (count > 0) {
	vectors = new Vector[count];
	decimalPlaces = new int[n_tuples];
	for (i=0 ; i<count ; i++)
	    vectors[i] = new double[n_tuples];
 
	for (i=0 ; i<n_tuples ; i++) {
	    decimalPlaces[i] = sli->getDecimals(i+1);
	    double max   = sli->getMaximum(i+1);
	    double min   = sli->getMinimum(i+1);
	    for (j=0 ; j<count ; j++) {
		double val = ro_vectors[j][i];
		if (val > max) {
		    val = max;
#if NEEDCLAMP // Not need after adding ScalarNode::rangeCheckComponentValue 6/3/93
		    clamped_vector = TRUE;
#endif
		} else if (val < min) {
		    val = min;
#if NEEDCLAMP // Not need after adding ScalarNode::rangeCheckComponentValue 6/3/93
		    clamped_vector = TRUE;
#endif
		}
		vectors[j][i] = val;
	    }
	}

	XtVaSetValues(this->valueList,
		XmNvectors, vectors,
		XmNtuples,	n_tuples,
		XmNvectorCount, count,
		XmNdecimalPlaces, decimalPlaces,
		NULL);

	for (i=0 ; i<count ; i++)
	    delete vectors[i];
	delete vectors;
	delete decimalPlaces;
    }


#if NEEDCLAMP // Not need after adding ScalarNode::rangeCheckComponentValue 6/3/93
    // 
    // If we change any of the list elements, output value must get adjusted.
    // There are two ways (possibly among others) that this routine can 
    // get called...
    //  1) The user has manually changed the attributes with the Set 
    //	   Attributes dialog.  In this case we always want to send the
    //	   new list value if it has changed.
    //  2) This interactor is being data-driven and executive has sent a 
    // 	   message that changes the attributes.  Messages are only 
    //	   sent after execution of the network. In this case the value should
    //	   not change since the executive will send us the correct list.
    // In addition, this seems like a reasonable, albeit necessary, 
    // optimization.
    //
    ScalarListNode *sln = (ScalarListNode*)sli->getNode();
    if (clamped_vector) {
	char *s = this->buildDXList();
	sln->setOutputValue(1,s);
	if (s) delete s;
    }
#else
    this->updateDisplayedInteractorValue();
#endif 

    if (major_change)
	this->manage();
}

extern "C" void ScalarListInteractor_AddCB(Widget w,
				XtPointer clientData, XtPointer callData)
{
    ScalarListInteractor *sli = (ScalarListInteractor*)clientData;
    sli->addCallback(w, callData);
}

void ScalarListInteractor::addCallback(Widget w, XtPointer callData)
{
    ScalarInstance *si = (ScalarInstance*)this->interactorInstance;
    ScalarNode *node = (ScalarNode*)si->getNode();
    int		pos, i, n_tuples = node->getComponentCount();
    Vector	v = new double[n_tuples];

    //
    // Make sure the item makes it into the list
    //
    
    for (i=0 ; i<n_tuples ; i++) 
	v[i] = si->getComponentValue(i+1);
    pos = this->selectedItemIndex;
    //
    // If no item currently selected, then append to the end of the list. 
    //
    if (pos == UNSELECTED_INDEX)
	pos = -1;
    XmNumericListAddVector((XmNumericListWidget)this->valueList, v,  pos);

    //
    // Now read the list and set the output value. 
    //
    char *val = this->buildDXList();
    node->setOutputValue(1, val);
    if (val) delete val;
    delete v;

}
extern "C" void ScalarListInteractor_DeleteCB(Widget w,
				XtPointer clientData, XtPointer callData)
{
    ScalarListInteractor *sli = (ScalarListInteractor*)clientData;
    sli->deleteCallback(w, callData);
}
void ScalarListInteractor::deleteCallback(Widget w, XtPointer callData)
{
    int pos, count;

    XtVaGetValues(this->valueList, XmNvectorCount, &count, NULL);
    if (count == 0) {
	this->selectedItemIndex = UNSELECTED_INDEX;
    } else if ((pos = this->selectedItemIndex) != UNSELECTED_INDEX) {
	//
	// Only delete selected items.
	//
	ScalarInstance *si = (ScalarInstance*)this->interactorInstance;
	ScalarNode *node = (ScalarNode*)si->getNode();
	//
	// Make sure the item makes it out of the list
	//
	XmNumericListDeleteVector((XmNumericListWidget)this->valueList, pos);
	// 
	//  If that was the last item, take note of it.
	// 
	if (count == 1)
	    this->selectedItemIndex = UNSELECTED_INDEX;
	else if (count == (pos+1))		// pos is zero based.
	    this->selectedItemIndex = pos-1; 	// just deleted last item.
	//
	// Now read the list and set the output value. 
	//
	char *val = this->buildDXList();
	node->setOutputValue(1, val);
	if (val) delete val;
    }
}

//
// Build a DX style list of  the items in the list.
// Returns a string representing the list of items; the string must be
// freed by the caller.
// When the string is empty, the string "NULL" is returned.
//
#define CHUNKSIZE 64
char *ScalarListInteractor::buildDXList()
{
    int i, j, count, n_tuples;
    char *list;
    VectorList vectors;

    XtVaGetValues(this->valueList, XmNvectorCount, &count,
				   XmNtuples, &n_tuples,
				   XmNreadonlyVectors, &vectors,
				   NULL);
	

    if (count > 0 ) {
	ScalarNode *node = (ScalarNode*)this->interactorInstance->getNode();
	boolean ints = node->isIntegerTypeComponent();
        list = new char[n_tuples * count * (1 + 4 + 16)  * sizeof(char)];
	strcpy(list,"{ ");
 	char *p = list;
	for (i=0 ; i<count ; p+=STRLEN(p), i++) {
	    if (node->isVectorType()) 
		strcat(p, "[");

	    for (j=0 ; j<n_tuples ; j++) {
                char s[128];
                if (ints) {
                    int tmp = (int) vectors[i][j];
                    DXValue::FormatDouble((double)tmp, s, 0);
                } else
		    DXValue::FormatDouble((double)vectors[i][j], s);
		strcat(p,s);
		if (n_tuples > 1) strcat(p," ");
	    } 
	    if (node->isVectorType()) 
		strcat(p, "]");
	    if (i == (count-1))
	       strcat(p," ");
	    else
	       strcat(p,", ");
	}
	strcat(p,"}");
    } else {
	list = DuplicateString("NULL");
    }
    return list;

}
/////////////////////////////////////////////////////////////////////////////
//
// If there is a change in selection, then display the new values
// in the value and label windows.
//
extern "C" void ScalarListInteractor_ListCB(Widget w,
				XtPointer clientData, XtPointer callData)
{
    ScalarListInteractor *sli = (ScalarListInteractor*)clientData;
    sli->listCallback(w, callData);

}
void ScalarListInteractor::listCallback(Widget w, XtPointer callData)
{
    ScalarListInstance		*sli = (ScalarListInstance*)
					this->interactorInstance;
    ScalarListNode *sln = (ScalarListNode*)this->getNode();
    XmNumericListCallbackStruct *cb = (XmNumericListCallbackStruct*)callData;
    int i;
    VectorList   vlist = cb->vector_list;

    if (cb->position == -1) {
	this->selectedItemIndex = UNSELECTED_INDEX;
    } else {
	//
	//  A new item has been highlighted.
	//
	this->selectedItemIndex = cb->position;
	int count = sln->getComponentCount();

	for (i=1 ; i<=count ; i++) 
	    sli->setComponentValue(i, vlist[cb->position][i-1]);
	
	//
	// Update the stepper value from the component values set above. 
	//
	this->StepperInteractor::updateDisplayedInteractorValue();
    }
}
//
// Build an n-vector or scalar stepper for the given instance with a list
// editor.
//
Widget ScalarListInteractor::createInteractivePart(Widget form)
{
    //
    // Build a form for both the list
    //
    this->listForm = form;

    this->listFrame = XtVaCreateManagedWidget("frame",
			xmFrameWidgetClass,     this->listForm,
			XmNtopAttachment, 	XmATTACH_FORM,
			XmNleftAttachment,	XmATTACH_FORM,
			XmNrightAttachment,	XmATTACH_FORM,
			XmNleftOffset,		0,
			XmNrightOffset,		0,
			NULL);
 
    this->createListEditor(this->listForm);

    return this->listForm;
}

void ScalarListInteractor::createListEditor(Widget listForm)
{

    //
    // Create the scrolled list (items are added later).
    //
    this->valueList = this->createList(this->listFrame);

    this->addButton = XtVaCreateManagedWidget("addButton",
                        xmPushButtonWidgetClass , listForm,
                        XmNleftAttachment,      XmATTACH_FORM,
			XmNleftOffset,		0,
                        NULL);


    XtAddCallback(this->addButton, XmNactivateCallback,
        (XtCallbackProc)ScalarListInteractor_AddCB, (XtPointer)this);

    //
    // The Delete button
    //
    this->deleteButton = XtVaCreateManagedWidget("deleteButton",
                        xmPushButtonWidgetClass, listForm,
                        XmNrightAttachment,     XmATTACH_FORM,
			XmNrightOffset,		0,
                        NULL);

    XtAddCallback(this->deleteButton, XmNactivateCallback,
        (XtCallbackProc)ScalarListInteractor_DeleteCB, (XtPointer)this);

    //
    // The stepper(s). The name depends on the dimensionality to facilitate
    // setting default resources.
    //

    ScalarListInstance *sli = (ScalarListInstance*)this->interactorInstance;
    ScalarListNode *sln = (ScalarListNode*)sli->getNode();
    int n_tuples = sln->getComponentCount(); 

    char *wname;
    if (n_tuples == 1) wname = "stepper_form";
    else wname = "steppers_form";

    Widget w = this->steppers = XtVaCreateManagedWidget(wname,
			xmFormWidgetClass,	listForm,
			NULL);  
    this->StepperInteractor::createInteractivePart(w);
    
    //
    // Change the attachment so as to attach to the add and delete buttons.
    //
    XtVaSetValues(w,
		  XmNleftAttachment,	XmATTACH_FORM,
		  XmNrightAttachment,	XmATTACH_FORM,
		  XmNtopAttachment,	XmATTACH_NONE,
		  XmNbottomAttachment,	XmATTACH_FORM,
		  NULL);

    XtVaSetValues(this->addButton,
		  XmNbottomAttachment,	XmATTACH_WIDGET,
		  XmNtopAttachment,	XmATTACH_NONE,
		  XmNbottomWidget,	w,
		  NULL);
    XtVaSetValues(this->deleteButton,
		  XmNbottomAttachment,	XmATTACH_WIDGET,
		  XmNtopAttachment,	XmATTACH_NONE,
		  XmNbottomWidget,	w,
		  NULL);
    XtVaSetValues(this->listFrame,
		  XmNbottomAttachment,	XmATTACH_WIDGET,
		  XmNbottomWidget,	this->addButton,
		  NULL);
}

Widget ScalarListInteractor::createList(Widget     frame_parent)
{
    Widget          widget;
    int             n;
    Widget          label;
    XmFontList      font_list;
    XFontStruct    *font;
#if (XmVersion >= 1001)
    XmFontContext   context;
    XmStringCharSet charset;
#endif
    int             dir;
    int             ascent;
    int             descent;
    XCharStruct     overall;
    int             height;
    Arg             wargs[30];
    

    ASSERT(frame_parent);

    /*
     * Calculate the height required for each line.
     */
    label = XmCreateLabel(this->listForm, "temp", wargs, 0);

    XtSetArg(wargs[0], XmNfontList, &font_list);
    XtGetValues(label, wargs, 1);

#if (XmVersion < 1001)
    font = font_list->font;
#else
    XmFontListInitFontContext(&context, font_list);
    XmFontListGetNextFont(context, &charset, &font);
    XmFontListFreeFontContext(context);
#endif

    XTextExtents(font, "temp", 4, &dir, &ascent, &descent, &overall);
    height =  5*(ascent +  descent) + 2;

    //
    // Due to some strange Motif bug, can't destroy this never-managed
    // widget.  Because it was never realized, its accelerators (the
    // BulletinBoardReturn from StepperInteractor) aren't completely
    // installed, but destroyCallbacks from the bulletin board are, to
    // ensure that when it is destroyed, the acclerators are removed.
    // Upon realization of this widget, the acclerators are "really installed."
    // When this widget is destroyed or you call XtUninstallAccelerators, 
    // any "really installed" accelerators are removed and the "source"
    // widget's (the bulletin board's) destroyCallbacks are removed.  If the
    // widget is never realized, then the bulletin board's destroyCallbacks
    // are never removed, and when the bulletin board is destroyed, it
    // causes a "can't remove accelerator from NULL table" warning OR a
    // core dump.
    // 
    // To work around this problem, don't destroy this never-managed
    // label until it is naturally destroyed when the parent is destroyed.
    // To recreate this, uncomment the destroy line, create a VectorList 
    // interactor and put it in a control panel, move it, and change its 
    // style to text.  You'll get a warning about trying to remove 
    // accelerators from a NULL table or you'll get a core dump.
    // XtDestroyWidget(label);


    ScalarInstance *si = (ScalarInstance*)this->interactorInstance;
    int tuples = si->getComponentCount();
    // The following few lines of code were changed between revs 6.2 and 6.3.
    // I didn't see any explaination for what it was supposed to fix, so by 
    // reverting to old code, I'm probably reintroducing a bug.  The problem is
    // that the width calculation (140*tuples) is considerably too large.  The
    // result is a vector list interactor which is way bigger than in 2.1.5.  Recall
    // that in 2.1.5 the .cfg file did not contain interator sizes.  So if customers
    // load an old .cfg file and the interactor is suddenly twice as wide, then
    // the interactors start jumping around like popcorn.
#if WHAT_DID_THIS_FIX
    int width = 140*tuples;
#else
    int width = 75 + 75 * tuples;
    if (width > 300) width = 300;
#endif

    
    n = 0;
    XtSetArg(wargs[n], XmNvectors,              NULL);          n++;
    XtSetArg(wargs[n], XmNvectorCount,          0);     	n++;
    XtSetArg(wargs[n], XmNtuples,               tuples);      	n++;
    XtSetArg(wargs[n], XmNheight,               height);        n++;
    XtSetArg(wargs[n], XmNwidth,                width);         n++;
    XtSetArg(wargs[n], XmNmarginHeight,         0);             n++;
    XtSetArg(wargs[n], XmNmarginWidth,          0);             n++;
    XtSetArg(wargs[n], XmNresizePolicy,         XmRESIZE_ANY);  n++;
    XtSetArg(wargs[n], XmNvectorSpacing,        20);            n++;

    widget = XmCreateScrolledNumericList(frame_parent, "valueList", wargs, n);
    XtManageChild(widget);

    Widget sw = XtParent(widget);
    XtVaSetValues(sw,
		  XmNtopAttachment, XmATTACH_FORM,
		  XmNleftAttachment, XmATTACH_FORM,
		  NULL);

    Widget sb = NULL;
    XtVaGetValues(sw,XmNverticalScrollBar, &sb, NULL);
    if (sb)
        XtVaSetValues(sb, XmNrightAttachment, XmATTACH_FORM, NULL);

    XtVaSetValues(sw, XmNrightAttachment, XmATTACH_FORM, NULL);

    sb = NULL;
    XtVaGetValues(sw,XmNhorizontalScrollBar, &sb, NULL);
    if (sb)
        XtVaSetValues(sb, XmNbottomAttachment, XmATTACH_FORM, NULL);

    XtVaSetValues(sw, XmNbottomAttachment, XmATTACH_FORM, NULL);


    XtAddCallback(widget, XmNselectCallback, (XtCallbackProc)ScalarListInteractor_ListCB, 
					(XtPointer)this);
    return widget;
}

