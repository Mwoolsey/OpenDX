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
#include <Xm/PushB.h>
#include <Xm/CascadeB.h>
#include <X11/IntrinsicP.h>
#include "XmUtility.h"

#include "SelectorInteractor.h"
#include "InteractorStyle.h"
#include "ListIterator.h"

#include "SelectorNode.h"

#include "SelectorInstance.h"
#include "DXApplication.h"
#include "ErrorDialogManager.h"
#include "../widgets/XmDX.h"

boolean SelectorInteractor::SelectorInteractorClassInitialized = FALSE;

String SelectorInteractor::DefaultResources[] =  {
    "*allowHorizontalResizing:		True",
    "*optionMenu.spacing:		0",
    "*optionMenu.topOffset:		4",
    "*optionMenu.leftOffset:		1",
    "*optionMenu.rightOffset:		1",
    "*optionMenu.bottomOffset:		4",
    "*optionMenu.marginHeight:		1",
    "*optionMenu.marginWidth:		0",
    "*interactive_form.resizePolicy:	XmRESIZE_NONE",
    NUL(char*) 
};


SelectorInteractor::SelectorInteractor(const char *name,
		 InteractorInstance *ii) : Interactor(name,ii)
{
    this->pulldown = NULL;
    this->optionMenu = NULL;
    this->optionForm = NULL;
    this->visinit = FALSE;
}

//
// One time class initializations.
//
void SelectorInteractor::initialize()
{
    //
    // Initialize default resources (once only).
    //
    if (NOT SelectorInteractor::SelectorInteractorClassInitialized)
    {
        ASSERT(theApplication);
        this->setDefaultResources(theApplication->getRootWidget(),
                                  SelectorInteractor::DefaultResources);
        this->setDefaultResources(theApplication->getRootWidget(),
                                  Interactor::DefaultResources);
        SelectorInteractor::SelectorInteractorClassInitialized = TRUE;
    }
}


//
// Allocate an n-dimensional stepper for the given instance.
//
Interactor *SelectorInteractor::AllocateInteractor(const char *name,
						InteractorInstance *ii)
{

    SelectorInteractor *si = new SelectorInteractor(name,ii);
    return (Interactor*)si;
}

//
// Accepts value changes and reflects them into other interactors, cdbs
// and off course the interactor node output.
//
extern "C" void SelectorInteractor_OptionMenuCB(Widget                  widget,
               XtPointer                clientData,
               XtPointer                callData)
{
    SelectorInteractor *si = (SelectorInteractor*)clientData;

    ASSERT(widget);
    ASSERT(si);


    int optnum = (int)(long)GetUserData(widget);
    si->optionMenuCallback(widget, optnum, callData);
}

//
// Perform anything that needs to be done after the parent of 
// this->interactivePart has been managed.
//
void SelectorInteractor::completeInteractivePart()
{
}

//
// Build the selector interactor option menu. 
// Note that we also use this to rebuild the list of options seen in the menu
// In this case the  
//
Widget SelectorInteractor::createInteractivePart(Widget form)
{
    SelectorNode	*node;
    SelectorInstance	*si = (SelectorInstance*)this->interactorInstance;

    ASSERT(si);

    node = (SelectorNode*)si->getNode();

    ASSERT(si->getStyle()->getStyleEnum() == SelectorOptionMenuStyle);
    ASSERT(form);
    ASSERT(node);
    ASSERT(EqualString(node->getClassName(), ClassSelectorNode));

    this->optionForm = form;

    // 
    // Build the option menu 
    //
    this->reloadMenuOptions();

    XtManageChild(this->optionForm);
    return this->optionForm;
}


//
// [Re]load the options into this->pulldown.
//
void SelectorInteractor::reloadMenuOptions()
{
    SelectorInstance *si = (SelectorInstance*) this->interactorInstance;
    Widget sbutton;
    int options, n, i;
    Arg wargs[20];
    Pixel bg, fg;
    unsigned char rsp;
    
    XtVaGetValues (this->optionForm, XmNresizePolicy, &rsp, NULL);
    XtVaSetValues (this->optionForm, XmNresizePolicy, XmRESIZE_GROW, NULL);

    ASSERT(this->optionForm);

    if (this->optionMenu)  {
	XtUnmanageChild (this->optionMenu);
	XtDestroyWidget (this->optionMenu);
	this->optionMenu = NULL;
	XtDestroyWidget (this->pulldown);
    }

    /*
     * The pulldown already exists so remove all its children,
     * which are assumed to be contained in this->optionWidgets. 
     */
    this->optionWidgets.clear();	
    

    //
    // colors change with panel style.  It's better to start out with
    // the proper colors than to switch to ther proper colors.
    //
    XtVaGetValues (this->optionForm, XmNforeground, &fg, XmNbackground, &bg, NULL);
    n = 0;
    XtSetArg (wargs[n], XmNbackground, bg); n++;
    XtSetArg (wargs[n], XmNforeground, fg); n++;
    this->pulldown = XmCreatePulldownMenu(this->optionForm, "pulldownMenu", wargs, n);

    /*
     * Create the options in the pulldown menu according to specified
     * option list.
     */

    Dimension strw, strh;
    int maxw = 0;
    XmFontList xmfl = 0;
    options = si->getOptionCount();
    XmString xmstr;
    if (options > 0) {
	int selectedOption = si->getSelectedOptionIndex();
	ASSERT(selectedOption <= options);
	sbutton = NULL;
	for (i = 1; i <= options; i++)
	{
	    char *optname = (char*)si->getOptionNameString(i);
	    ASSERT(optname);
	    Widget button;
	    xmstr = XmStringCreateLtoR(optname, "canvas");
	    n = 0;
	    XtSetArg(wargs[n], XmNuserData, i); n++;
	    XtSetArg(wargs[n], XmNlabelString, xmstr ); n++;
	    XtSetArg (wargs[n], XmNbackground, bg); n++;
	    XtSetArg (wargs[n], XmNforeground, fg); n++;
	    button = XmCreatePushButton(this->pulldown, optname, wargs, n);

	    if (!xmfl)
		XtVaGetValues (button, 
		    XmNfontList, &xmfl, 
		NULL);

	    XmStringExtent (xmfl, xmstr, &strw, &strh);
	    maxw = MAX(maxw, strw);

	    XtAddCallback
		(button,
		 XmNactivateCallback,
		 (XtCallbackProc)SelectorInteractor_OptionMenuCB,
		 (XtPointer)this);
	    XtManageChild(button);
	    if (i == selectedOption) {
		sbutton = button;
	    }
	    this->appendOptionWidget(button);
	    delete optname;
	    XmStringFree(xmstr);
	}
        ASSERT(sbutton);
	// Certainly not a solution to the problem, but using multi columns
	// makes marginally better those situations in which a huge number of
	// options makes the list go off the screen. 
	// FIXME: MAX_OPTIONS must be determined at run time by comparing the
	// height of the XmStrings with the height of the display.  Besides
	// the number 40 is almost certainly wrong.
#define MAX_OPTIONS 40
	int columns = (options/MAX_OPTIONS) + ((options%MAX_OPTIONS)?1:0);
	if (columns > 1) {
	    XtVaSetValues (this->pulldown,
		XmNpacking, XmPACK_COLUMN,
		XmNnumColumns, columns,
	    NULL);
	}
    } else {
	n = 0;
	xmstr = XmStringCreateLtoR("(empty)", "canvas");
	XtSetArg(wargs[n], XmNlabelString, xmstr); n++;
	sbutton = XmCreatePushButton(this->pulldown, "(empty)", wargs, n);
	XtManageChild(sbutton);
	if (!xmfl) XtVaGetValues (sbutton, XmNfontList, &xmfl, NULL);
	XmStringExtent (xmfl, xmstr, &strw, &strh);
	maxw = MAX(maxw, strw);
	this->appendOptionWidget(sbutton);
	XmStringFree(xmstr);
    }

    /*
     * Create the option menu itself or just reset the current selection
     * if the menu already exists.
     */
    n = 0;
    XtSetArg(wargs[n], XmNmenuHistory, sbutton);		n++;
    if (!this->optionMenu) {
	XtSetArg(wargs[n], XmNsubMenuId,   this->pulldown);         n++;
	XtSetArg(wargs[n], XmNtopAttachment, XmATTACH_FORM);     n++;
	XtSetArg(wargs[n], XmNleftOffset, -(18 + (maxw>>1))); n++;
	XtSetArg(wargs[n], XmNrightAttachment, XmATTACH_FORM);      n++;
	XtSetArg(wargs[n], XmNbottomAttachment, XmATTACH_FORM);     n++;
	if (this->currentLayout & WorkSpaceComponent::Vertical) {
	    XtSetArg (wargs[n], XmNleftAttachment, XmATTACH_POSITION); n++;
	    XtSetArg (wargs[n], XmNleftPosition, 50); n++; 
	} else {
	    XtSetArg (wargs[n], XmNleftAttachment, XmATTACH_NONE); n++;
	}
	XtSetArg(wargs[n], XmNbackground, bg); n++;
	XtSetArg(wargs[n], XmNforeground, fg); n++;
	this->optionMenu = XmCreateOptionMenu(this->optionForm, 
						"optionMenu", wargs, n);
	Widget og = XmOptionButtonGadget(this->optionMenu);
	XtVaSetValues(og,
		XmNbackground, bg,
		XmNforeground, fg,
		NULL);
	//
	// Every motif option menu comes with its own label widget which we
	// never use.  
	// For the sake of dgux, set the labelString to "", because it defaults
	// to "OptionLabel" unlike everywhere else.
	//
	xmstr = XmStringCreateLtoR ("", "canvas");
	Widget labelgadget = XmOptionLabelGadget (this->optionMenu);
	XtVaSetValues (labelgadget, 
	    XmNlabelString, xmstr,
	    XmNbackground, bg,
	    XmNforeground, fg,
	NULL);
	XmStringFree (xmstr);
	XtManageChild(this->optionMenu);
    } 
    if (options <= 0)
	XtSetSensitive(this->optionMenu,False);
    else if (!XtIsSensitive (this->optionMenu))
	XtSetSensitive(this->optionMenu,True);

    XtVaSetValues (this->optionForm, XmNresizePolicy, rsp, NULL);
}

//
// Accepts value changes and reflects them into other interactors, cdbs
// and off course the interactor node output.
//
void SelectorInteractor::optionMenuCallback(Widget w, int optnum, XtPointer cb)
{
    SelectorInstance *si = (SelectorInstance*)this->interactorInstance;

    ASSERT(w);
    ASSERT(optnum > 0);

    si->setSelectedOptionIndex(optnum, TRUE);
}

//
// Update the displayed values for this interactor.
//
void SelectorInteractor::updateDisplayedInteractorValue()
{
    // FIXME: should check to make sure we have the correct class of node.
    SelectorInstance *si = (SelectorInstance*)this->interactorInstance;
    ASSERT(si);

    //
    // There are no options, so there is nothing to update.
    //
    if (si->getOptionCount() <= 0)
	return;

    /*
     * Just display the currently selected option. 
     */
    int option = si->getSelectedOptionIndex();
    if (option == 0)
	return;

    Widget w = this->getOptionWidget(option);
    ASSERT(w);
    XtVaSetValues(this->optionMenu, XmNmenuHistory, w, NULL);
}

//
// Make sure the attributes match the resources for the widgets.
//
void SelectorInteractor::handleInteractivePartStateChange(
					InteractorInstance *src_ii,
					boolean major_change)
{
    this->reloadMenuOptions();
    if (this->currentLayout & WorkSpaceComponent::Horizontal) {
	this->currentLayout|= WorkSpaceComponent::NotSet;
	this->layoutInteractor();
    }
}

void SelectorInteractor::layoutInteractorHorizontally()
{
    this->Interactor::layoutInteractorHorizontally();

    if (!this->optionMenu) return ;
    XtVaSetValues (this->optionMenu, 
	XmNleftAttachment, XmATTACH_NONE,
    NULL);
}

//
// The setting and resetting of marginHeight and topAttachment deals with the 
// absence of a topAttachment.  XmNtopAttachment is NONE because it permits
// the optionmenu to wind up roughly in the middle of its parent even if a change
// in the implementation of the interactor causes us to crave a different height
// than is saved in the .cfg file. The button widget musn't change height but 
// the rowcolumn can.
//
void SelectorInteractor::layoutInteractorVertically()
{
    this->Interactor::layoutInteractorVertically();

    if (!this->optionMenu) return ;
    XtVaSetValues (this->optionMenu, 
	XmNleftAttachment, XmATTACH_POSITION,
	XmNleftPosition, 50,
    NULL);
}


void SelectorInteractor::setAppearance(boolean developer_style)
{
    boolean changing = (developer_style != this->getAppearance());
    this->Interactor::setAppearance(developer_style);
    if ((changing) || (this->visinit == FALSE)) {
	this->reloadMenuOptions();
	this->visinit = TRUE;
    }
}
