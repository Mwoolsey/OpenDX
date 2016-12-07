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
#include <Xm/RowColumn.h>
#include <Xm/Frame.h>
#include <Xm/PushB.h>
#include <Xm/Form.h>
#include <Xm/ToggleB.h>
#include <Xm/Separator.h>
#include "XmUtility.h"

#include "SelectorListToggleInteractor.h"
#include "SelectionAttrDialog.h"
#include "InteractorStyle.h"

#include "SelectorListNode.h"

#include "SelectorListInstance.h"
#include "DXApplication.h"
#include "ListIterator.h"
#include "ErrorDialogManager.h"

boolean SelectorListToggleInteractor::ClassInitialized = FALSE;

String SelectorListToggleInteractor::DefaultResources[] =  {
	"*XmToggleButton.shadowThickness:	0",
        "*XmToggleButton.selectColor:           White",
	"*allowHorizontalResizing:		True",
	"*XmForm.resizePolicy:			XmRESIZE_ANY",
// The following 2 settings make the rowColumn widget
// use 1 column at all times.
	"*toggleForm.numColumns:		1",
	"*toggleForm.packing:			XmPACK_COLUMN",
	"*toggleForm.topAttachment:		XmATTACH_FORM",
	"*toggleForm.rightAttachment:		XmATTACH_FORM",
	"*toggleForm.bottomAttachment:		XmATTACH_FORM",

	NUL(char*) 
};

SelectorListToggleInteractor::SelectorListToggleInteractor(const char *name,
		 InteractorInstance *ii) : Interactor(name,ii)
{
    this->toggleForm = NULL;
    this->initialize();
}

//
// One time class initializations.
//
void SelectorListToggleInteractor::initialize()
{
    //
    // Initialize default resources (once only).
    //
    if (NOT SelectorListToggleInteractor::ClassInitialized)
    {
        ASSERT(theApplication);
        this->setDefaultResources(theApplication->getRootWidget(),
			      SelectorListToggleInteractor::DefaultResources);
        this->setDefaultResources(theApplication->getRootWidget(),
			      Interactor::DefaultResources);
        SelectorListToggleInteractor::ClassInitialized = TRUE;
    }
}

//
// Allocate an interactor 
//
Interactor *SelectorListToggleInteractor::AllocateInteractor(
				const char *name, InteractorInstance *ii)
{

    SelectorListToggleInteractor *si = 
				new SelectorListToggleInteractor(name,ii);
    return (Interactor*)si;
}

//
// Toggle button callback, which just sets the apply/cancel buttons sensitive. 
//
extern "C" void SelectorListToggleInteractor_ToggleCB(
		Widget                  widget,
               XtPointer                clientData,
               XtPointer                callData)
{
    SelectorListToggleInteractor *si = 
			(SelectorListToggleInteractor*)clientData;

    ASSERT(widget);
    ASSERT(si);

    si->applyCallback(widget, callData);
}

//
// Perform anything that needs to be done after the parent of 
// this->interactivePart has been managed.
//
void SelectorListToggleInteractor::completeInteractivePart()
{
    //SelectorListInstance *si = (SelectorListInstance*)this->interactorInstance;
}
//
// Build the selector interactor option menu. 
// Note that we also use this to rebuild the list of options seen in the menu
// In this case the  
//
Widget SelectorListToggleInteractor::createInteractivePart(Widget form)
{
    SelectorListNode	*node;
    SelectorListInstance	*si = (SelectorListInstance*)this->interactorInstance;

    ASSERT(si);

    node = (SelectorListNode*)si->getNode();

    ASSERT(form);
    ASSERT(node);
    ASSERT(EqualString(node->getClassName(), ClassSelectorListNode));

    if (this->currentLayout & WorkSpaceComponent::Vertical)
	this->toggleForm = XtVaCreateManagedWidget("toggleForm", xmRowColumnWidgetClass, 
	    form, XmNleftAttachment,  XmATTACH_FORM, NULL);
    else
	this->toggleForm = XtVaCreateManagedWidget("toggleForm", xmRowColumnWidgetClass, 
	    form, XmNleftAttachment,  XmATTACH_NONE, NULL);

    // 
    // Build the option menu 
    //
    this->reloadMenuOptions();

    return form;
}
//
// [Re]load the options into this->pulldown.
//

#if 1
void 
SelectorListToggleInteractor::reloadMenuOptions()
{
    SelectorListInstance *si = (SelectorListInstance*) this->interactorInstance;
    SelectorListNode	*snode = (SelectorListNode*)si->getNode();
    int endi, n, i;
    Arg wargs[10];
    Pixel bg, fg;
    Widget button, w;
    int newOptionCnt = si->getOptionCount();
    int oldOptionCnt = this->toggleWidgets.getSize();
    Boolean setting, oldSetting;
    XmString xmstr, oldstr;
    char *cp;

    // reuse existing buttons
    for (i=1; i<=oldOptionCnt; i++) {
	if (i>newOptionCnt) break;
	w = (Widget)this->toggleWidgets.getElement(i);
	char *optname = (char *)si->getOptionNameString(i);
	ASSERT(optname);
	if (snode->isOptionSelected(i))  setting = True;
	else setting = False;
	XtVaGetValues (w, XmNlabelString, &oldstr, XmNset, &oldSetting, NULL);
	if (oldSetting!=setting) XtVaSetValues (w, XmNset, setting, NULL);
	XmStringGetLtoR (oldstr, XmSTRING_DEFAULT_CHARSET, &cp);
	XmStringFree (oldstr);
	if (!XtIsSensitive (w)) XtSetSensitive(w, True);
	if ((!cp)||(strcmp(cp, optname))) {
	    xmstr = XmStringCreateLtoR(optname, "canvas");
	    XtVaSetValues (w, XmNuserData, i, XmNlabelString, xmstr, NULL);
	    XmStringFree (xmstr);
	} else XtVaSetValues (w, XmNuserData, i, NULL);
	if (cp) XtFree(cp);
	delete optname;
    }

    // deal with existing, unneeded buttons
    endi = i;
    for (i=i; i<=oldOptionCnt; i++) {
	w = (Widget)this->toggleWidgets.getElement(endi);
	if ((i==oldOptionCnt) && (newOptionCnt == 0)) {
	    xmstr = XmStringCreateLtoR("(empty)", "canvas");
	    XtSetSensitive(w, False);
	    XtVaSetValues (w, XmNlabelString, xmstr, NULL);
	    XmStringFree(xmstr);
	    break;
	}
	XtUnmanageChild (w);
	XtDestroyWidget (w);
	this->toggleWidgets.deleteElement(endi);
    }

    // get new buttons if more are needed
    XtVaGetValues (this->toggleForm, XmNforeground, &fg, XmNbackground, &bg, NULL);
    for (i=i; i<=newOptionCnt; i++) {
	char *optname = (char*)si->getOptionNameString(i);
	Boolean setting;
	ASSERT(optname);
	xmstr = XmStringCreateLtoR(optname, "canvas");
	n = 0;
	XtSetArg(wargs[n], XmNuserData, i); n++;
	XtSetArg(wargs[n], XmNlabelString, xmstr ); n++;
	if (snode->isOptionSelected(i))  setting = True;
	else setting = False;
	XtSetArg(wargs[n], XmNset, setting); n++;
	XtSetArg(wargs[n], XmNbackground, bg); n++;
	XtSetArg(wargs[n], XmNforeground, fg); n++;
	button = XtCreateManagedWidget(optname,
		xmToggleButtonWidgetClass, this->toggleForm, wargs,n); 

	XtAddCallback(button, XmNvalueChangedCallback,
		SelectorListToggleInteractor_ToggleCB, (XtPointer)this);

	this->appendOptionWidget(button);
	delete optname;
	XmStringFree(xmstr);
    }
    this->resetUserDimensions();
    XtVaSetValues (XtParent(this->toggleForm), XmNheight, 0, NULL);
}
#else
void SelectorListToggleInteractor::reloadMenuOptions()
{
    SelectorListInstance *si = (SelectorListInstance*) this->interactorInstance;
    SelectorListNode	*snode = (SelectorListNode*)si->getNode();
    int options, n, i;
    Arg wargs[10];
    Pixel bg, fg;
    Widget button;

    /*
     * Destroy all the children in the list
     */
    Widget w;
    while (w = (Widget)this->toggleWidgets.getElement(1)) {
	XtUnmanageChild (w);
	XtDestroyWidget(w);
	this->toggleWidgets.deleteElement(1);
    }


    XtVaGetValues (this->toggleForm, XmNforeground, &fg, XmNbackground, &bg, NULL);

    /*
     * Create the options in the pulldown menu according to specified
     * option list.
     */
    button = 0;
    options = si->getOptionCount();
    XmString xmstr;
    if (options > 0) {
	Widget last_widget = NULL;
	for (i = 1; i <= options; i++)
	{
	    char *optname = (char*)si->getOptionNameString(i);
	    Boolean setting;
	    ASSERT(optname);
	    xmstr = XmStringCreateLtoR(optname, "canvas");
	    n = 0;
	    XtSetArg(wargs[n], XmNuserData, i); n++;
	    XtSetArg(wargs[n], XmNlabelString, xmstr ); n++;
	    if (snode->isOptionSelected(i)) {
		setting = True;
	    } else {
		setting = False;
	    }
	    XtSetArg(wargs[n], XmNset, setting); n++;
	    XtSetArg(wargs[n], XmNbackground, bg); n++;
	    XtSetArg(wargs[n], XmNforeground, fg); n++;
	    button = XtCreateManagedWidget(optname,
		xmToggleButtonWidgetClass, this->toggleForm, wargs,n); 

	    XtAddCallback(button, XmNvalueChangedCallback,
				SelectorListToggleInteractor_ToggleCB,
				(XtPointer)this);

	    this->appendOptionWidget(button);
	    delete optname;
	    XmStringFree(xmstr);
	    last_widget = button;
	}
    } else {
	n = 0;
	xmstr = XmStringCreateLtoR("(empty)", "canvas");
	XtSetArg(wargs[n], XmNlabelString, xmstr); n++;
	XtSetArg(wargs[n], XmNbackground, bg); n++;
	XtSetArg(wargs[n], XmNforeground, fg); n++;
        Widget button = XtCreateManagedWidget("(empty)",
                xmToggleButtonWidgetClass, this->toggleForm, wargs,n);

	XtSetSensitive(button,False);
	XmStringFree(xmstr);
    }
}
#endif

//
// Accepts value changes and reflects them into other interactors, cdbs
// and off course the interactor node output.
//
void SelectorListToggleInteractor::applyCallback(Widget w, XtPointer cb)
{
    SelectorListInstance *si = (SelectorListInstance*)this->interactorInstance;
    SelectorListNode	*snode = (SelectorListNode*)si->getNode();
    ASSERT(snode);
    int	selections[1024], nselects=0, i; 
 
    
    Widget toggle;
    ListIterator li(this->toggleWidgets);
    i = 1;
    while (nselects < 1024 && (toggle = (Widget)li.getNext())) {
	Boolean isset;
	XtVaGetValues(toggle, XmNset, &isset, NULL);	
	if (isset)  {
	    selections[nselects] = i;
	    nselects++;
	}
	i++;
    }
    snode->setSelectedOptions(selections,nselects);
}

//
// Update the displayed values for this interactor.
//
void SelectorListToggleInteractor::updateDisplayedInteractorValue()
{
    int		   i;
    SelectorListInstance *si = (SelectorListInstance*)this->interactorInstance;
    ASSERT(si);
    SelectorListNode	*snode = (SelectorListNode*)si->getNode();
    ASSERT(snode);

    //
    // There are no options, so there is nothing to update.
    //
    int		optcount = si->getOptionCount();
    if (optcount <= 0)
	return;

    ListIterator li(this->toggleWidgets);
    Widget toggle;
    i = 1;
    while ( (toggle = (Widget)li.getNext()) ) {
	Boolean toggle_isset;
	Boolean option_isselected;
	XtVaGetValues(toggle, XmNset, &toggle_isset, NULL);	
	option_isselected = snode->isOptionSelected(i++) ? True : False;
	if (toggle_isset != option_isselected) 
	    XtVaSetValues(toggle, XmNset, option_isselected, NULL);	
    }
}

//
// Make sure the attributes match the resources for the widgets.
//
void SelectorListToggleInteractor::handleInteractivePartStateChange(
					InteractorInstance *src_ii,
					boolean major_change)
{
    if (major_change)
	this->unmanage();

    this->reloadMenuOptions();

    if (major_change)
	this->manage();
}

void
SelectorListToggleInteractor::layoutInteractorVertically()
{
    this->Interactor::layoutInteractorVertically();
    XtVaSetValues(this->toggleForm, XmNleftAttachment,    XmATTACH_FORM, NULL);
}

void
SelectorListToggleInteractor::layoutInteractorHorizontally()
{
    XtVaSetValues(this->toggleForm, XmNleftAttachment,    XmATTACH_NONE, NULL);
    this->Interactor::layoutInteractorHorizontally();
}
