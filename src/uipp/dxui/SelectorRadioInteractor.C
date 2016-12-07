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
#include <Xm/Form.h>
#include <Xm/ToggleB.h>
#include "XmUtility.h"

#include "SelectorRadioInteractor.h"
#include "InteractorStyle.h"

#include "SelectorNode.h"

#include "SelectorInstance.h"
#include "DXApplication.h"
#include "ListIterator.h"
#include "ErrorDialogManager.h"

boolean SelectorRadioInteractor::SelectorRadioInteractorClassInitialized = FALSE;

String SelectorRadioInteractor::DefaultResources[] =  {
	"*XmToggleButton.shadowThickness:	0",
        "*XmToggleButton.selectColor:         White",
	"*allowHorizontalResizing:			True",
	"*toggleRadio.topAttachment: 	XmATTACH_FORM",
	"*toggleRadio.leftAttachment: 	XmATTACH_FORM",
	"*toggleRadio.rightAttachment: 	XmATTACH_FORM",
	"*toggleRadio.bottomAttachment: XmATTACH_FORM",
	"*toggleRadio.topOffset: 	4",
	"*toggleRadio.bottomOffset: 	4",
	"*toggleRadio.leftOffset: 	4",
	"*toggleRadio.rightOffset: 	4",

// NOTE: Do not install any accelerators here (or at all), otherwise
// 	the XtDestroyWidget() calls may cause core dumps because of
//	a bug in Motif/X11 code having to do with destroying unrealized
//	widgets.
//
	NUL(char*) 
};

SelectorRadioInteractor::SelectorRadioInteractor(const char *name,
		 InteractorInstance *ii) : Interactor(name,ii)
{
    this->toggleRadio = NULL;
}

//
// One time class initializations.
//
void SelectorRadioInteractor::initialize()
{
    //
    // Initialize default resources (once only).
    //
    if (NOT SelectorRadioInteractor::SelectorRadioInteractorClassInitialized)
    {
        ASSERT(theApplication);
        this->setDefaultResources(theApplication->getRootWidget(),
                                  SelectorRadioInteractor::DefaultResources);
        this->setDefaultResources(theApplication->getRootWidget(),
                                  Interactor::DefaultResources);
        SelectorRadioInteractor::SelectorRadioInteractorClassInitialized = TRUE;
    }
}


//
// Allocate an interactor 
//
Interactor *SelectorRadioInteractor::AllocateInteractor(
				const char *name, InteractorInstance *ii)
{

    SelectorRadioInteractor *si = new SelectorRadioInteractor(name,ii);
    return (Interactor*)si;
}

//
// Accepts value changes and reflects them into other interactors, cdbs
// and off course the interactor node output.
//
extern "C" void SelectorRadioInteractor_SelectorToggleCB(Widget                  widget,
               XtPointer                clientData,
               XtPointer                callData)
{
    SelectorRadioInteractor *si = (SelectorRadioInteractor*)clientData;

    ASSERT(widget);
    ASSERT(si);


    int optnum = (int)(long)GetUserData(widget);
    si->toggleCallback(widget, optnum, callData);
}

//
// Perform anything that needs to be done after the parent of 
// this->interactivePart has been managed.
//
void SelectorRadioInteractor::completeInteractivePart()
{
    //SelectorInstance	*si = (SelectorInstance*)this->interactorInstance;
}
//
// Build the selector interactor option menu. 
// Note that we also use this to rebuild the list of options seen in the menu
// In this case the  
//
Widget SelectorRadioInteractor::createInteractivePart(Widget form)
{
    SelectorNode	*node;
    SelectorInstance	*si = (SelectorInstance*)this->interactorInstance;
    Pixel bg,fg;
    int n;
    Arg wargs[5];

    ASSERT(si);

    node = (SelectorNode*)si->getNode();

    //ASSERT(si->getStyle()->getStyleEnum() == SelectorStyle);
    ASSERT(form);
    ASSERT(node);
    ASSERT(EqualString(node->getClassName(), ClassSelectorNode));

    this->form = form;
    XtVaGetValues (this->form, XmNbackground, &bg, XmNforeground, &fg, NULL);
    n = 0;
    XtSetArg (wargs[n], XmNbackground, bg); n++;
    XtSetArg (wargs[n], XmNforeground, bg); n++;
    this->toggleRadio = XmCreateRadioBox(form,"toggleRadio", wargs, n);
    XtManageChild(this->toggleRadio);

    // 
    // Build the option menu 
    //
    this->reloadMenuOptions();

    return this->toggleRadio;
}
void SelectorRadioInteractor::reloadMenuOptions()
{
    SelectorInstance *si = (SelectorInstance*) this->interactorInstance;
    SelectorNode     *snode = (SelectorNode*)si->getNode();
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
    XtVaGetValues (this->toggleRadio, XmNforeground, &fg, XmNbackground, &bg, NULL);
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
                xmToggleButtonWidgetClass, this->toggleRadio, wargs,n);

        XtAddCallback(button, XmNvalueChangedCallback,
		 (XtCallbackProc)SelectorRadioInteractor_SelectorToggleCB,
		 (XtPointer)this);

        this->appendOptionWidget(button);
        delete optname;
        XmStringFree(xmstr);
    }
    this->resetUserDimensions();
    XtVaSetValues (XtParent(this->toggleRadio), XmNheight, 0, NULL);
}

//
// Accepts value changes and reflects them into other interactors, cdbs
// and off course the interactor node output.
//
void SelectorRadioInteractor::toggleCallback(Widget w, int optnum, XtPointer cb)
{
    SelectorInstance *si = (SelectorInstance*)this->interactorInstance;
    XmToggleButtonCallbackStruct *tbcb = (XmToggleButtonCallbackStruct *)cb;
    
    ASSERT(tbcb);

    //
    // With radio boxes we get two callbacks, one for the set and one
    // for the unset.  Wait for the callback that is for setting a toggle. 
    //
    if (!tbcb->set)
	return;
	
    ASSERT(w);
    ASSERT(optnum > 0);

    si->setSelectedOptionIndex(optnum, TRUE);

#if 0
    ListIterator li(this->toggleWidgets);
    Widget toggle;
    while (toggle = (Widget)li.getNext()) {
	if (toggle != w) {
	    Boolean isset;
	    XtVaGetValues(toggle, XmNset, &isset, NULL);	
	    if (isset)
	        XtVaSetValues(toggle, XmNset, False, NULL);	
	}
    }
#endif
}

//
// Update the displayed values for this interactor.
//
void SelectorRadioInteractor::updateDisplayedInteractorValue()
{
    // FIXME: should check to make sure we have the correct class of node.
    SelectorInstance *si = (SelectorInstance*)this->interactorInstance;
    ASSERT(si);

    //
    // There are no options, so there is nothing to update.
    //
    if (si->getOptionCount() <= 0)
	return;

#if 11 
    ListIterator li(this->toggleWidgets);
    Widget toggle;
    while ( (toggle = (Widget)li.getNext()) ) {
	Boolean isset;
	XtVaGetValues(toggle, XmNset, &isset, NULL);	
	if (isset) {
	    XtVaSetValues(toggle, XmNset, False, NULL);	
	    break;
	}
    }
#endif

    /*
     * Just display the currently selected option. 
     */
    int option = si->getSelectedOptionIndex();
    if (option == 0)
	return;

    Widget w = this->getOptionWidget(option);
    ASSERT(w);
    XtVaSetValues(w, XmNset, True, NULL); 
}

//
// Make sure the attributes match the resources for the widgets.
//
void SelectorRadioInteractor::handleInteractivePartStateChange(
					InteractorInstance *src_ii,
					boolean major_change)
{
    if (major_change)
	this->unmanage();

    this->reloadMenuOptions();

    if (major_change)
	this->manage();
}

void SelectorRadioInteractor::layoutInteractorVertically()
{
    XtVaSetValues(this->toggleRadio,
		  XmNleftAttachment,	XmATTACH_FORM,
		  XmNrightAttachment,	XmATTACH_FORM,
		  NULL);
    Interactor::layoutInteractorVertically();
}
void SelectorRadioInteractor::layoutInteractorHorizontally()
{
    XtVaSetValues(this->toggleRadio,
		  XmNleftAttachment,	XmATTACH_NONE,
		  XmNrightAttachment,	XmATTACH_FORM,
		  NULL);
    Interactor::layoutInteractorHorizontally();
}

