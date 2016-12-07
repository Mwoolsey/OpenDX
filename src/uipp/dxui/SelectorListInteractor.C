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
#include <Xm/List.h>
#include <Xm/Frame.h>
#include <Xm/Form.h>
#include <Xm/Separator.h>
#include "XmUtility.h"

#include "SelectorListInteractor.h"
#include "InteractorStyle.h"

#include "SelectorNode.h"
#include "SelectorListNode.h"
#include "DXApplication.h"
#include "SelectorListInstance.h"

boolean SelectorListInteractor::ClassInitialized = FALSE;

String SelectorListInteractor::DefaultResources[] =  {
    "*allowHorizontalResizing:		True",
    "*pinLeftRight:                     True",
    "*pinTopBottom:                     True",
    "*allowVerticalResizing:		True",
    "*selectorList.height:		150",
    "*selectorList.width:		150",
    
    NUL(char*) 
};

SelectorListInteractor::SelectorListInteractor(const char *name,
		 InteractorInstance *ii) : Interactor(name,ii)
{
    //
    // We can connect to either subclass of SelectionNode: Selector or SelectorList
    //
    SelectionNode* sn = (SelectionNode*)this->getNode();
    if (EqualString (ClassSelectorListNode, sn->getClassName())) {
	this->single_select = FALSE;
    } else if (EqualString (ClassSelectorNode, sn->getClassName())) {
	this->single_select = TRUE;
    } else
	ASSERT(0);

    //
    // Initialize default resources (once only).
    //
    if (NOT SelectorListInteractor::ClassInitialized)
    {
        ASSERT(theApplication);
        this->setDefaultResources(theApplication->getRootWidget(),
			      SelectorListInteractor::DefaultResources);
        this->setDefaultResources(theApplication->getRootWidget(),
			      Interactor::DefaultResources);
        SelectorListInteractor::ClassInitialized = TRUE;
    }
}


//
// Build the selector interactor option menu. 
// Note that we also use this to rebuild the list of options seen in the menu
// In this case the  
//
Widget SelectorListInteractor::createInteractivePart(Widget form)
{
    ASSERT(form);

    SelectorListInstance* si = (SelectorListInstance*)this->interactorInstance;
    ASSERT(si);
    
    Widget frame = XtVaCreateManagedWidget ("listFrame",
	xmFrameWidgetClass,	form,
	XmNshadowThickness,	2,
	XmNshadowType,		XmSHADOW_ETCHED_IN,
	XmNtopAttachment,	XmATTACH_FORM,
	XmNleftAttachment,	XmATTACH_FORM,
	XmNrightAttachment,	XmATTACH_FORM,
	XmNbottomAttachment,	XmATTACH_FORM,
	XmNtopOffset,		2,
	XmNleftOffset,		2,
	XmNrightOffset,		2,
	XmNbottomOffset,	2,
	XmNmarginWidth,		2,
	XmNmarginHeight,	2,
    NULL);

    Arg args[20];
    int n = 0;
    XtSetArg (args[n], XmNscrollBarDisplayPolicy, XmAS_NEEDED); n++;
    if (this->single_select) {
	XtSetArg (args[n], XmNselectionPolicy, XmSINGLE_SELECT); n++;
    } else {
	XtSetArg (args[n], XmNselectionPolicy, XmMULTIPLE_SELECT); n++;
    }
    XtSetArg (args[n], XmNlistSizePolicy, XmCONSTANT); n++;
    this->list_widget = XmCreateScrolledList (frame, "selectorList", args, n);
    XtManageChild (this->list_widget);

    this->reloadListOptions();

    return form;
}

void SelectorListInteractor::enableCallbacks (boolean enab)
{
    if (enab) {
	XtAddCallback (this->list_widget, XmNmultipleSelectionCallback, 
	    (XtCallbackProc) SelectorListInteractor_SelectCB, (XtPointer)this);
	XtAddCallback (this->list_widget, XmNsingleSelectionCallback, 
	    (XtCallbackProc) SelectorListInteractor_SelectCB, (XtPointer)this);
	XtAddCallback (this->list_widget, XmNdefaultActionCallback, 
	    (XtCallbackProc) SelectorListInteractor_SelectCB, (XtPointer)this);
    } else {
	XtRemoveCallback (this->list_widget, XmNmultipleSelectionCallback, 
	    (XtCallbackProc) SelectorListInteractor_SelectCB, (XtPointer)this);
	XtRemoveCallback (this->list_widget, XmNsingleSelectionCallback, 
	    (XtCallbackProc) SelectorListInteractor_SelectCB, (XtPointer)this);
	XtRemoveCallback (this->list_widget, XmNdefaultActionCallback, 
	    (XtCallbackProc) SelectorListInteractor_SelectCB, (XtPointer)this);
    }
}

//
// [Re]load the options into this->pulldown.
//
void 
SelectorListInteractor::reloadListOptions()
{
    if (!this->list_widget) return ;

    this->disableCallbacks();

    XmListDeleteAllItems (this->list_widget);

    SelectorListInstance* si = (SelectorListInstance*)this->interactorInstance;
    int count = si->getOptionCount();
    XmStringTable strtab = new XmString[count];

    int i;
    for (i=1; i<=count; i++) {
	char *optname = (char*)si->getOptionNameString(i);
	ASSERT(optname);
	strtab[i-1] = XmStringCreateLtoR(optname, "canvas");
	delete optname;
    }

    XmListAddItemsUnselected (this->list_widget, strtab, count, 1);

    SelectionNode* snode = (SelectionNode*)si->getNode();
    for (i=1; i<=count; i++) {
	if (snode->isOptionSelected(i))
	    XmListSelectPos (this->list_widget, i, False);
    }

    for (i=0; i<count; i++)
	XmStringFree(strtab[i]);
    delete strtab;

    this->enableCallbacks();
}

//
// Accepts value changes and reflects them into other interactors, cdbs
// and off course the interactor node output.
//
void SelectorListInteractor::applyCallback()
{
    SelectorListInstance* si = (SelectorListInstance*)this->interactorInstance;
    SelectionNode* snode = (SelectionNode*)si->getNode();
    ASSERT(snode);
 
    int *posList;
    int posCnt;
    if (XmListGetSelectedPos (this->list_widget, &posList, &posCnt) == False) 
	posCnt = 0;
    posCnt = MIN(posCnt, 1024);

    //
    // In the case of SelectorNode && the user deselected the 1 selected item,
    // force it to be reselected because we cannot handle 0 selections.
    //
    if ((posCnt == 0) && (this->single_select)) {
	SelectorNode* rn = (SelectorNode*)snode;
	int selctd = rn->getSelectedOptionIndex();
	XmListSelectPos (this->list_widget, selctd, False);
    } else {
	snode->setSelectedOptions(posList,posCnt);
    }
    if (posList)
	XtFree((char*)posList);
}

//
// Update the displayed values for this interactor.
//
void SelectorListInteractor::updateDisplayedInteractorValue()
{
    SelectorListInstance* si = (SelectorListInstance*)this->interactorInstance;
    ASSERT(si);
    SelectionNode* snode = (SelectionNode*)si->getNode();
    ASSERT(snode);

    this->disableCallbacks();
    XmListDeselectAllItems (this->list_widget);

    int i;
    int	count = si->getOptionCount();
    for (i=1; i<=count; i++) {
	if (snode->isOptionSelected(i)) {
	    XmListSelectPos (this->list_widget, i, False);
	}
    }
    this->enableCallbacks();
}

//
// Make sure the attributes match the resources for the widgets.
//
void 
SelectorListInteractor::handleInteractivePartStateChange(InteractorInstance *,
					boolean major_change)
{
    if (major_change) this->unmanage();
    this->reloadListOptions();
    if (major_change) this->manage();
}

void SelectorListInteractor::setAppearance(boolean developer_style)
{
    Pixel fg,bg,ts,bs,arm;
    Screen *screen;
    Colormap cmap;

    this->Interactor::setAppearance(developer_style);
    if (developer_style) {
    	   XtVaGetValues (this->getRootWidget(), XmNcolormap, &cmap, XmNscreen, &screen, NULL);

	   bg = theDXApplication->getStandInBackground();
	   XmGetColors (screen, cmap, bg, &fg, &ts, &bs, &arm);

	XtVaSetValues( this->list_widget,
	    XmNselectColor, bs,
	    NULL);
    }
}

extern "C" {
//
//
void 
SelectorListInteractor_SelectCB(Widget, XtPointer cdata, XtPointer)
{
    SelectorListInteractor *slt = (SelectorListInteractor*)cdata;
    ASSERT(slt);
    slt->applyCallback();
}

} // end extern "C"
