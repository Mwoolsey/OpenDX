/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#include <Xm/RowColumn.h>
#include <Xm/Form.h>
#include <Xm/ScrolledW.h>
#include <Xm/PushB.h>
#include <Xm/DrawingA.h>
#include <Xm/Label.h>
#include <Xm/Separator.h>

//
// Is it a good idea to provide a button which will sort up/down the ticks
// according to the tick value?  Pro: The user doesn't have the ability to 
// move individual entries around in the list and the AutoAxes module seems
// to care about sorted order.  Con:  The module should be able to deal with
// the entries in any order, sort them if it wants to, and echo them back in
// sorted order.  The ui should just respond to that and not get cluttered up
// with sorting code.
//
// If you use the SORT_TICKS code, then touch TickLabelList.h so that Makefile
// does the right thing.
//
//#define UI_SHOULD_SORT_TICKS 1

#include "TickLabelList.h"
#include "TickLabel.h"
#include "ListIterator.h"
#include "DXStrings.h"
#include "DXApplication.h"


//
//
String TickLabelList::DefaultResources[] = {
    "*tllMenuB.width:			20",
    "*tllMenuB.labelString:		...",
    "*tllMenuB.recomputeSize:		False",

    "*listRC.marginWidth:		1",
    "*listRC.marginHeight:		1",
    "*listRC.spacing:			0",

    "*newTop.labelString:		Insert at top",
    "*deleteAll.labelString:		Delete all items",
    "*aasBut.labelString:		Append after selection",
    "*iasBut.labelString:		Insert above selection",
    "*dsBut.labelString:		Delete Selection",
    "*suBut.labelString:		Sort in Ascending Order",
    "*sdBut.labelString:		Sort in Descending Order",
    NULL
};

boolean TickLabelList::SortUp = TRUE;

boolean TickLabelList::ClassInitialized = FALSE;

#define DIRTY_APPEND 1
#define DIRTY_DELETE 2

TickLabelList::TickLabelList (const char *header, TickListModifyCB tlmcb, 
	void *clientData): UIComponent ("tickList")
{
   this->header = DuplicateString(header);
   this->dirty = 0;

   this->header_label = this->header_button = this->listRC = NULL;
   this->popupMenu = NULL;

   this->oldDvals = NULL;
   this->oldString = NULL;

   this->highest_set_number = 0;

   this->tlmcb = tlmcb;
   this->clientData = clientData;
}

void TickLabelList::initialize()
{
    //
    // Initialize default resources (once only).
    //
    if (NOT TickLabelList::ClassInitialized) {
	this->setDefaultResources(theApplication->getRootWidget(),
	    TickLabelList::DefaultResources);
	TickLabelList::ClassInitialized = TRUE;
    }
}

TickLabelList::~TickLabelList()
{
    if (this->header) delete this->header;

    ListIterator it(this->ticks);
    TickLabel *tl;

    while ( (tl = (TickLabel*)it.getNext()) ) 
	delete tl;
    this->ticks.clear();

    if (this->oldDvals) delete this->oldDvals;

    if (this->oldString) 
	delete this->oldString;
}


void
TickLabelList::createList (Widget parent)
{
int n;
Arg args[25];

    this->initialize();

    Widget form = XtVaCreateWidget (this->name,
	xmFormWidgetClass, 	parent,
	XmNshadowThickness,	2,
	XmNshadowType,		XmSHADOW_ETCHED_IN,
    NULL);

    n = 0;
    XtSetArg (args[n], XmNtopAttachment,	XmATTACH_FORM); n++;
    XtSetArg (args[n], XmNtopOffset,		50); n++;
    XtSetArg (args[n], XmNleftAttachment,	XmATTACH_FORM); n++;
    XtSetArg (args[n], XmNrightAttachment,	XmATTACH_FORM); n++;
    XtSetArg (args[n], XmNbottomAttachment,	XmATTACH_FORM); n++;
    XtSetArg (args[n], XmNleftOffset,		2); n++;
    XtSetArg (args[n], XmNrightOffset,		2); n++;
    XtSetArg (args[n], XmNbottomOffset,		2); n++;
    XtSetArg (args[n], XmNheight,		150); n++;
    XtSetArg (args[n], XmNshadowThickness, 	0); n++;
    XtSetArg (args[n], XmNspacing, 		0); n++;
    XtSetArg (args[n], XmNscrollingPolicy, 	XmAUTOMATIC); n++;
    Widget sw = XmCreateScrolledWindow (form, "ticksSW", args, n);
    XtManageChild (sw);
    Widget sb = NULL;
    XtVaGetValues (sw, XmNhorizontalScrollBar, &sb, NULL);
    if (sb) XtUnmanageChild (sb);
    
    n = 0;
    XtSetArg (args[n], XmNresizeWidth,		False); n++;
    this->listRC = XmCreateRowColumn (sw, "listRC", args, n);
    XtManageChild (this->listRC);
    XmScrolledWindowSetAreas (sw, NULL, NULL, this->listRC);

    //
    // Title line... 
    //
    XmString xmstr = XmStringCreate (this->header, "bold");
    Widget xlab = XtVaCreateManagedWidget ("listHeader",
	xmLabelWidgetClass,	form,
	XmNbottomAttachment,	XmATTACH_WIDGET,
	XmNbottomWidget,	sw,
	XmNbottomOffset,	20,
	XmNleftAttachment,	XmATTACH_POSITION,
	XmNleftPosition,	50,
	XmNleftOffset,		-50,
	XmNalignment,		XmALIGNMENT_CENTER,
	XmNlabelString,		xmstr,
    NULL);
    XmStringFree (xmstr);

    //
    // Labels over the numbers and the text.
    //
    xmstr = XmStringCreate ("Tick Location", "small_bold");
    XtVaCreateManagedWidget ("tickTitle",
	xmLabelWidgetClass,	form,
	XmNbottomAttachment,	XmATTACH_WIDGET,
	XmNbottomWidget,	sw,
	XmNbottomOffset,	1,
	XmNleftAttachment,	XmATTACH_FORM,
	XmNleftOffset,		4,
	XmNlabelString,		xmstr,
    NULL);
    XmStringFree(xmstr);
    xmstr = XmStringCreate ("Tick Label", "small_bold");
    this->labelLabel = XtVaCreateManagedWidget ("textTitle",
	xmLabelWidgetClass,	form,
	XmNbottomAttachment,	XmATTACH_WIDGET,
	XmNbottomWidget,	sw,
	XmNbottomOffset,	1,
	XmNrightAttachment,	XmATTACH_FORM,
	XmNrightOffset,		4,
	XmNlabelString,		xmstr,
    NULL);
    XmStringFree(xmstr);
	

    //
    // button for list ops... 
    //
    Widget button = XtVaCreateManagedWidget ("tllMenuB",
	xmPushButtonWidgetClass,form,
	XmNbottomAttachment,	XmATTACH_OPPOSITE_WIDGET,
	XmNbottomWidget,	xlab,
	XmNbottomOffset,	0,
	XmNleftAttachment,	XmATTACH_WIDGET,
	XmNleftWidget,		xlab,
	XmNleftOffset,		10,
    NULL);
    XtUninstallTranslations (button);
    XtAddEventHandler (button, ButtonPressMask|ButtonReleaseMask, False,
	(XtEventHandler)TickLabelList_ButtonEH, (XtPointer)this);

    n = 0;
    XtSetArg (args[0], XmNmenuPost, "Shift<Btn4Down>");
    this->popupMenu = XmCreatePopupMenu(form, "tllPopup", args, 1);

    button = this->ntiButton = XtVaCreateManagedWidget ("newTop",
	xmPushButtonWidgetClass,	this->popupMenu,
	XmNuserData,			0,
    NULL);
    XtAddCallback (button, XmNactivateCallback,
	(XtCallbackProc)TickLabelList_AppendCB, (XtPointer)this);

    button = this->daButton = XtVaCreateManagedWidget ("deleteAll",
	xmPushButtonWidgetClass,	this->popupMenu,
    NULL);
    XtAddCallback (button, XmNactivateCallback,
	(XtCallbackProc)TickLabelList_DeleteAllCB, (XtPointer)this);

    XtVaCreateManagedWidget ("sep", xmSeparatorWidgetClass, this->popupMenu, NULL);

    button = this->iasButton = XtVaCreateManagedWidget ("iasBut",
	xmPushButtonWidgetClass,	this->popupMenu,
    NULL);
    XtAddCallback (button, XmNactivateCallback,
	(XtCallbackProc)TickLabelList_InsertAboveSelCB, (XtPointer)this);

    button = this->aasButton = XtVaCreateManagedWidget ("aasBut",
	xmPushButtonWidgetClass,	this->popupMenu,
    NULL);
    XtAddCallback (button, XmNactivateCallback,
	(XtCallbackProc)TickLabelList_AppendAfterSelCB, (XtPointer)this);

    button = this->dsButton = XtVaCreateManagedWidget ("dsBut",
	xmPushButtonWidgetClass,	this->popupMenu,
    NULL);
    XtAddCallback (button, XmNactivateCallback,
	(XtCallbackProc)TickLabelList_DeleteSelCB, (XtPointer)this);

#if UI_SHOULD_SORT_TICKS
    XtVaCreateManagedWidget ("sep", xmSeparatorWidgetClass, this->popupMenu, NULL);

    button = this->suButton = XtVaCreateManagedWidget("suBut",
	xmPushButtonWidgetClass,	this->popupMenu,
    NULL);
    XtAddCallback (button, XmNactivateCallback,
	(XtCallbackProc)TickLabelList_SortUpCB, (XtPointer)this);

    button = this->sdButton = XtVaCreateManagedWidget("sdBut",
	xmPushButtonWidgetClass,	this->popupMenu,
    NULL);
    XtAddCallback (button, XmNactivateCallback,
	(XtCallbackProc)TickLabelList_SortDownCB, (XtPointer)this);
#endif

    //
    // Create a dummy drawing area in order to catch resize events.  This is
    // necessary for scrolled windows.  Their contents' must be manually stretched
    // horizontally when the window is resized.
    //
    n = 0;
    XtSetArg (args[n], XmNtopAttachment,	XmATTACH_FORM); n++;
    XtSetArg (args[n], XmNtopOffset,		35); n++;
    XtSetArg (args[n], XmNleftAttachment,	XmATTACH_FORM); n++;
    XtSetArg (args[n], XmNrightAttachment,	XmATTACH_FORM); n++;
    XtSetArg (args[n], XmNbottomAttachment,	XmATTACH_FORM); n++;
    XtSetArg (args[n], XmNleftOffset,		2); n++;
    XtSetArg (args[n], XmNrightOffset,		2); n++;
    XtSetArg (args[n], XmNbottomOffset,		2); n++;
    XtSetArg (args[n], XmNmappedWhenManaged,	False); n++;
    Widget drawa = XmCreateDrawingArea (form, "junk", args, n);
    XtManageChild (drawa);
    XtAddCallback (drawa, XmNresizeCallback, 
	(XtCallbackProc)TickLabelList_ResizeTicksCB, (XtPointer)this);

    this->setRootWidget(form);

    ListIterator it(this->ticks);
    TickLabel *tl;
    while ( (tl = (TickLabel*)it.getNext()) ) 
	tl->createLine (this->listRC);
}


void
TickLabelList::createLine (double dval, const char *str, int pos)
{
    TickLabel *tl = new TickLabel (dval, str, pos, 
	(TickSelectCB)TickLabelList::SelectCB, (void*)this);
    ASSERT (pos);
    ASSERT (pos <= (this->ticks.getSize() + 1));
    this->ticks.insertElement((void *)tl, pos);
    this->dirty|= DIRTY_APPEND;
    if (this->listRC)
	tl->createLine(this->listRC);

    int newpos = this->ticks.getPosition((void*)tl);
    ASSERT (newpos == pos);
    this->highest_set_number = MAX(pos, this->highest_set_number);

    this->resizeCallback();
}

void
TickLabelList::SelectCB (TickLabel* tlab, void* clientData)
{
    ASSERT(tlab);
    if (!tlab->isSelected()) return ;

    ASSERT(clientData);
    TickLabelList *tll = (TickLabelList*)clientData;

    ListIterator it(tll->ticks);
    TickLabel *nextTL;
    while ( (nextTL = (TickLabel*)it.getNext()) ) {
	if ((nextTL != tlab) && (nextTL->isSelected())) {
	    nextTL->setSelected (FALSE, FALSE);
	}
    }

    if (tll->tlmcb)
	tll->tlmcb (tll, tll->clientData);
}

boolean
TickLabelList::isModified()
{
    if (this->dirty) return TRUE;

    TickLabel *tl;
    ListIterator it(this->ticks);

    while ( (tl = (TickLabel*)it.getNext()) )
	if (tl->isModified()) return TRUE;
    return FALSE;
}

boolean
TickLabelList::isNumberModified()
{
    if (this->dirty) return TRUE;

    TickLabel *tl;
    ListIterator it(this->ticks);

    while ( (tl = (TickLabel*)it.getNext()) )
	if (tl->isNumberModified()) return TRUE;
    return FALSE;
}

boolean
TickLabelList::isTextModified()
{
    if (this->dirty) return TRUE;

    TickLabel *tl;
    ListIterator it(this->ticks);

    while ( (tl = (TickLabel*)it.getNext()) )
	if (tl->isTextModified()) return TRUE;
    return FALSE;
}

//
// Caller must not free the memory.
//
double *
TickLabelList::getTickNumbers()
{
    if (!this->ticks.getSize()) return NULL;

    if (this->oldDvals) {
	delete this->oldDvals;
	this->oldDvals = NULL;
    }

    ListIterator it(this->ticks);
    TickLabel *tl;
    double *dvals = new double[this->ticks.getSize()];

    int i = 0;
    while ( (tl = (TickLabel*)it.getNext()) ) 
	dvals[i++] = tl->getNumber();

    return (this->oldDvals = dvals);
}

//
// Caller must not free the memory.
//
String 
TickLabelList::getTickTextString()
{

    int size = this->ticks.getSize();
    if (!size) return NULL;

    if ((!this->isTextModified())&&(this->oldString)) 
	return this->oldString;

    if (this->oldString) {
	delete this->oldString;
	this->oldString = NULL;
    }
	
    boolean is_any_text_set = FALSE;
    ListIterator it(this->ticks);
    TickLabel *tl;
    int len = 0;
    const char *cp;
    while ( (tl = (TickLabel*)it.getNext()) ) {
	cp = tl->getText();
	if (!cp) continue;
	len+= strlen(cp);
	if ((len!=2)&&(len!=0)) is_any_text_set = TRUE;
	else if ((len == 2) && (strcmp(cp, "\"\""))) is_any_text_set = TRUE;
    }

    if (!is_any_text_set) {
	return NULL;
    }

    int newSize = len+10 + (5*size);
    char *buf = this->oldString = new char[newSize];
    strcpy (buf, "{ ");
    int bufLen = strlen(buf);
    boolean first_loop = TRUE;
    boolean add_quotes;
    
    it.setList(this->ticks);
    while ( (tl = (TickLabel*)it.getNext()) ) {
	if (!first_loop) {
	    strcpy (&buf[bufLen], ", "); bufLen+= 2;
	}
	cp = tl->getText();
	len = strlen(cp);
	add_quotes = TRUE;
	if ((cp[0] == '"') && (cp[len-1] == '"')) add_quotes = FALSE;
	if (add_quotes) buf[bufLen++] = '"';
	strcpy (&buf[bufLen], cp); bufLen+= len;
	if (add_quotes) buf[bufLen++] = '"'; 

	first_loop = FALSE;
    }

    ASSERT (newSize > bufLen);
    strcpy (&buf[bufLen], " }"); bufLen+= 2;
    return this->oldString;
}

void
TickLabelList::markClean()
{
    this->dirty = 0;
    TickLabel *tl;
    ListIterator it(this->ticks);

    while ( (tl = (TickLabel*)it.getNext()) ) {
	tl->setText();
	tl->setNumber();
    }
}

void
TickLabelList::markNumbersClean()
{
    TickLabel *tl;
    ListIterator it(this->ticks);

    while ( (tl = (TickLabel*)it.getNext()) ) {
	tl->setNumber();
    }
}

void
TickLabelList::markTextClean()
{
    TickLabel *tl;
    ListIterator it(this->ticks);

    while ( (tl = (TickLabel*)it.getNext()) ) {
	tl->setText();
    }
}

void
TickLabelList::clear()
{
    ListIterator it(this->ticks);
    TickLabel *tl;
    while ( (tl = (TickLabel*)it.getNext()) ) 
	delete tl;
    this->ticks.clear();

    ASSERT (this->ticks.getSize() == 0);
    this->dirty = 0;
}

void
TickLabelList::setNumber (int pos, double dval)
{
int size = this->ticks.getSize();

    if (pos > size)
	this->setListSize (pos);

    TickLabel *tl = (TickLabel*)this->ticks.getElement(pos);
    ASSERT(tl);
    tl->setNumber(dval);

    this->highest_set_number = MAX(pos, this->highest_set_number);
}

void
TickLabelList::setText (int pos, char *str)
{
int size = this->ticks.getSize();

    if (pos > size)
	this->setListSize (pos);

    TickLabel *tl = (TickLabel*)this->ticks.getElement(pos);
    ASSERT(tl);
    tl->setText(str);
}

void
TickLabelList::setListSize (int new_size)
{
int i, current_size = this->ticks.getSize();
TickLabel *tl;

    if (new_size == current_size) {
    } else if (new_size > current_size) {
	for (i=current_size+1; i<=new_size; i++) 
	    this->createLine (0.0, "", i);
    } else {
	List tmpList;
	for (i=new_size+1; i<=current_size; i++) {
	    tl = (TickLabel*)this->ticks.getElement(i);
	    ASSERT(tl);
	    tmpList.appendElement((void*)tl);
	}
	ListIterator it(tmpList);
	while ( (tl = (TickLabel*)it.getNext()) ) {
	    this->ticks.removeElement((void*)tl);
	    delete tl;
	}
    }
}

void TickLabelList::resizeCallback ()
{
    Dimension width, height, rch;
    Widget sw = this->listRC;

    while ((sw) && (XtClass(sw)!=xmScrolledWindowWidgetClass))
	sw = XtParent(sw);

    XtVaGetValues (sw, XmNwidth, &width, XmNheight, &height, NULL);
    XtVaGetValues (this->listRC, 
	XmNheight, &rch, 
    NULL);

    boolean scrollbar_in_use;
    if (rch > height) scrollbar_in_use = TRUE;
    else scrollbar_in_use = FALSE;

    if (scrollbar_in_use) {
	XtVaSetValues (this->listRC, XmNwidth, width-20, NULL);
	XtVaSetValues (this->labelLabel, XmNrightOffset, 26, NULL);
    } else {
	XtVaSetValues (this->listRC, XmNwidth, width-2, NULL);
	XtVaSetValues (this->labelLabel, XmNrightOffset, 8, NULL);
    }
}

//
// Using the list of TickLabel objects, create an array.  Sort the array.
// Then clear the original list and rebuild it using the contents of the array.
//
void TickLabelList::sortList (boolean up)
{
#if UI_SHOULD_SORT_TICKS
    int size = this->ticks.getSize();
    TickLabelList::SortUp = up;
    ASSERT (size > 1);
    int i;
    TickLabel **tla = new TickLabel*[size];
    for (i=1; i<=size; i++) 
	tla[i-1] = (TickLabel*)this->ticks.getElement(i);

    qsort (tla, size, sizeof(TickLabel*), TickLabelList_SortFunc);

    this->ticks.clear();

    XtUnmanageChild (this->listRC);
    for (i=0; i<size; i++) {
	this->ticks.appendElement((void*)tla[i]);
	tla[i]->destroyLine();
    }
    for (i=0; i<size; i++) {
	tla[i]->setPosition(i+1);
	tla[i]->createLine(this->listRC);
    } 
    XtManageChild (this->listRC);

    this->dirty|= DIRTY_APPEND;

    delete tla;
#endif
}

extern "C" {

int 
TickLabelList_SortFunc (const void* e1, const void* e2)
{
#if UI_SHOULD_SORT_TICKS
    TickLabel** tlp1 = (TickLabel**)e1;
    TickLabel** tlp2 = (TickLabel**)e2;
    TickLabel* tl1 = *tlp1;
    TickLabel* tl2 = *tlp2;
    double dval1 = tl1->getNumber();
    double dval2 = tl2->getNumber();

    if (TickLabelList::SortUp) {
	if (dval1 > dval2) return 1;
	if (dval1 < dval2) return -1;
    } else {
	if (dval1 < dval2) return 1;
	if (dval1 > dval2) return -1;
    }
    ASSERT (dval1 == dval2);
#endif
    return 0;
}

void TickLabelList_ButtonEH(Widget , XtPointer clientData,
                                        XEvent *e, Boolean *)
{
    TickLabelList *tlab = (TickLabelList *)clientData;
    ASSERT(tlab);
    XmMenuPosition(tlab->popupMenu , (XButtonPressedEvent *)e);
 
    if (e->type == ButtonRelease) XtManageChild(tlab->popupMenu);
    else XtAppAddTimeOut (theApplication->getApplicationContext(), 100,
        (XtTimerCallbackProc)XtManageChild, (XtPointer)tlab->popupMenu);

    //
    // Enable/Disable certain items based on list selection.
    //
    TickLabel *tl;
    ListIterator it(tlab->ticks);
    boolean any_selected = FALSE;
    while ((!any_selected) && (tl = (TickLabel*)it.getNext())) 
	if (tl->isSelected()) any_selected = TRUE;
    XtSetSensitive (tlab->aasButton, (Boolean)any_selected);
    XtSetSensitive (tlab->iasButton, (Boolean)any_selected);
    XtSetSensitive (tlab->dsButton,  (Boolean)any_selected);

    XtSetSensitive (tlab->daButton, (Boolean)(tlab->ticks.getSize() != 0));

#if UI_SHOULD_SORT_TICKS
    XtSetSensitive (tlab->suButton, (Boolean)(tlab->ticks.getSize() > 1));
    XtSetSensitive (tlab->sdButton, (Boolean)(tlab->ticks.getSize() > 1));
#endif
}


//
// Horizontally resize the contents of the 3 scrolled windows in TicksLabels.
//
void TickLabelList_ResizeTicksCB(Widget , XtPointer clientData, XtPointer)
{
    ASSERT(clientData);
    TickLabelList *tll = (TickLabelList *)clientData;
    tll->resizeCallback();
}

void TickLabelList_DeleteAllCB(Widget , XtPointer clientData, XtPointer)
{
    ASSERT(clientData);
    TickLabelList *tll = (TickLabelList *)clientData;


    tll->clear();
    tll->dirty|= DIRTY_DELETE;

    tll->highest_set_number = 0;

    tll->resizeCallback();
    if (tll->tlmcb)
	tll->tlmcb (tll, tll->clientData);
}

void TickLabelList_AppendCB(Widget w, XtPointer clientData, XtPointer)
{
    ASSERT(clientData);
    TickLabelList *tll = (TickLabelList *)clientData;

    //
    // Inovke the callback only if adding the first iterm.
    //
    boolean call_callback = FALSE;
    int size = tll->ticks.getSize();
    if (size == 0) call_callback = TRUE;

    int newTail;
    XtVaGetValues (w, XmNuserData, &newTail, NULL);

    int newPos;
    if (newTail) 
	newPos = 1 + size;
    else
	newPos = 1;

    tll->createLine (0.0, "", newPos);
    if ((tll->tlmcb) && (call_callback))
	tll->tlmcb (tll, tll->clientData);
}

void TickLabelList_AppendAfterSelCB(Widget , XtPointer clientData, XtPointer)
{
int newPos;

    ASSERT(clientData);
    TickLabelList *tll = (TickLabelList *)clientData;

    TickLabel *tl, *sel;
    ListIterator it(tll->ticks);
    sel = NULL;
    newPos = 1;
    while ( (tl = (TickLabel*)it.getNext()) ) {
	if (tl->isSelected()) {
	    sel = tl;
	    break;
	}
	newPos++;
    }
    ASSERT(sel);

    tll->createLine (0.0, "", newPos+1);
}

void TickLabelList_InsertAboveSelCB(Widget , XtPointer clientData, XtPointer)
{
int newPos;

    ASSERT(clientData);
    TickLabelList *tll = (TickLabelList *)clientData;

    TickLabel *tl, *sel;
    ListIterator it(tll->ticks);
    sel = NULL;
    newPos = 1;
    while ( (tl = (TickLabel*)it.getNext()) ) {
	if (tl->isSelected()) {
	    sel = tl;
	    break;
	}
	newPos++;
    }
    ASSERT(sel);

    tll->createLine (0.0, "", newPos);
}

void TickLabelList_DeleteSelCB(Widget , XtPointer clientData, XtPointer)
{
    ASSERT(clientData);
    TickLabelList *tll = (TickLabelList *)clientData;

    TickLabel *tl, *sel;
    ListIterator it(tll->ticks);
    sel = NULL;
    while ( (tl = (TickLabel*)it.getNext()) ) {
	if (tl->isSelected()) {
	    sel = tl;
	    break;
	}
    }
    ASSERT(sel);

    tll->ticks.removeElement((void*)sel);
    delete sel;

    tll->resizeCallback();
}

void TickLabelList_SortUpCB(Widget , XtPointer clientData, XtPointer)
{
    ASSERT(clientData);
    TickLabelList *tll = (TickLabelList *)clientData;
    tll->sortList(TRUE);
}

void TickLabelList_SortDownCB(Widget , XtPointer clientData, XtPointer)
{
    ASSERT(clientData);
    TickLabelList *tll = (TickLabelList *)clientData;
    tll->sortList(FALSE);
}

} // extern "C"
