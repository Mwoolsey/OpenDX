/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include <defines.h>


#include <Xm/PushB.h>
#include <Xm/Text.h>
#include <Xm/List.h>
#include <Xm/MenuShell.h>
#include <Xm/Form.h>
#include <Xm/Separator.h>
#include <X11/cursorfont.h>
#include <X11/Shell.h>

#include "TextSelector.h"
#include "ListIterator.h"
#include "IBMApplication.h"
#include "DXStrings.h"
#include "lex.h"

#include <ctype.h> // for isalnum

#define ELLIPSIS_WIDTH 23
#define SELECTOR_HEIGHT 23
#define MAX_VISIBLE 12

Cursor TextSelector::GrabCursor = 0;
TextSelector* TextSelector::GrabOwner = NUL(TextSelector*);

//
// The color #ddddddddd is chosen because it will both stand out, and go easy
// on the colormap since it's the same as WorkSpace.selectColor
//
String TextSelector::DefaultResources[] = {
    "*popupMenu.borderWidth: 0",
    "*popupMenu.allowShellResize: True",
    "*diagButton.labelString:		...",
    "*diagButton.recomputeSize:		False",
    "*diagButton.indicatorOn:		False",
    "*diagButton.topOffset: 0",
    "*diagButton.leftOffset: 0",
    "*diagButton.rightOffset: 0",
    "*diagButton.bottomOffset: 0",
    NUL(char*)
};

boolean TextSelector::ClassInitialized = FALSE;

TextSelector::TextSelector () :
    UIComponent ("textSelector")
{
    this->parent = NUL(Widget);
    this->button_release_timer = 0;
    this->diag_button = NUL(Widget);
    this->popupMenu = NUL(Widget);
    this->popupList = NUL(Widget);
    this->is_grabbed = FALSE;
    this->starting_button = 1;
    this->action_hook = 0;
    this->vsb = this->hsb = NUL(Widget);
    this->old_event = NUL(XEvent*);
    this->remove_hook_wpid = NUL(XtWorkProcId);
    this->item_list.clear();
    this->selected_items = NUL(int*);
    this->selected_cnt = 0;
    this->auto_fill_wpid = NUL(XtWorkProcId);
    this->case_sensitive = TRUE;

    if (!TextSelector::ClassInitialized) {
	TextSelector::ClassInitialized = TRUE;
	this->setDefaultResources(theIBMApplication->getRootWidget(), 
	    TextSelector::DefaultResources);
    }
}

TextSelector::~TextSelector()
{
    if (this->button_release_timer)
	XtRemoveTimeOut(this->button_release_timer);
    if (this->is_grabbed)
	this->ungrab();
    if (this->old_event)
	delete this->old_event;
    if (this->action_hook)
	XtRemoveActionHook (this->action_hook);
    if (this->remove_hook_wpid)
	XtRemoveWorkProc (this->remove_hook_wpid);
    ListIterator it(this->item_list);
    char* item;
    while ( (item = (char*)it.getNext()) )
	delete item;
    if (this->selected_items)
	XtFree((char*)this->selected_items);
    if (this->auto_fill_wpid)
	XtRemoveWorkProc (this->auto_fill_wpid);
}

void TextSelector::setItems(char* items[], int nitems)
{
    this->enableModifyCB(FALSE);

    //
    // Copy the items;
    //
    int i;
    this->item_list.clear();
    for (i=0; i<nitems; i++) {
	ASSERT (items[i]);
	char* cp = DuplicateString(items[i]);
	this->item_list.appendElement((void*)cp);
    }
    if (this->popupMenu)
	this->updateList();

    if (this->selected_items) 
	XtFree ((char*)this->selected_items);
    this->selected_items = NUL(int*);

    if (nitems)
	this->enableModifyCB(TRUE);
}

void TextSelector::setSelectedItem (int selected)
{
    if (this->selected_items)
	XtFree((char*)this->selected_items);
    this->selected_items = NUL(int*);
    this->selected_cnt = 0;

    if (selected <= 0) return ;
    if (selected > this->item_list.getSize()) return ;

    this->selected_cnt = 1;
    //this->selected_items = new int[this->selected_cnt];
    this->selected_items = (int*)XtMalloc(sizeof(int) * this->selected_cnt);
    this->selected_items[0] = selected;

    char *cp = (char*)this->item_list.getElement(selected);
    this->enableModifyCB(FALSE);
    XmTextSetString (this->textField, cp);
    this->enableModifyCB(TRUE);
}

int TextSelector::getSelectedItem (char* seli)
{
    if (seli != NUL(char*)) seli[0] = '\0';
    if (this->selected_items)
	XtFree((char*)this->selected_items);
    this->selected_items = NUL(int*);
    this->selected_cnt = 0;

    boolean sel = XmListGetSelectedPos 
	(this->popupList, &this->selected_items, &this->selected_cnt);

    if (sel == False)
	return 0;
    if (!this->selected_items) 
	return 0;
    int selected = this->selected_items[0];
    const char* cp = (char*)this->item_list.getElement(selected);
    if ((cp)&&(seli)) 
	strcpy (seli, cp);
    return selected;
}

void TextSelector::enableModifyCB(boolean enab)
{
    if (enab) {
	XtVaSetValues (this->textField, XmNeditable, True, NULL);
	XtAddCallback (this->textField, XmNmodifyVerifyCallback, (XtCallbackProc)
	    TextSelector_ModifyCB, (XtPointer)this);
	XtAddCallback (this->textField, XmNactivateCallback, (XtCallbackProc)
	    TextSelector_ResolveCB, (XtPointer)this);
    } else {
	XtVaSetValues (this->textField, XmNeditable, False, NULL);
	XtRemoveCallback (this->textField, XmNmodifyVerifyCallback, (XtCallbackProc)
	    TextSelector_ModifyCB, (XtPointer)this);
	XtRemoveCallback (this->textField, XmNactivateCallback, (XtCallbackProc)
	    TextSelector_ResolveCB, (XtPointer)this);
    }
}

void TextSelector::createTextSelector(Widget parent, XtCallbackProc cbp, XtPointer cdata, boolean use_button)
{
    ASSERT (this->parent == NUL(Widget));
    this->parent = parent;

    //
    // Layout a text widget and a ... button inside a form.
    //
    if (!use_button) {
	Widget form = XtVaCreateWidget(this->name,xmFormWidgetClass, parent, 
	    XmNshadowThickness, 0,
	    XmNshadowType, XmSHADOW_OUT,
	NULL);
	this->setRootWidget(form);

	this->textField = XtVaCreateManagedWidget("textField",
	    xmTextWidgetClass,form,
	    XmNtopAttachment,      XmATTACH_FORM,
	    XmNleftAttachment,      XmATTACH_FORM,
	    XmNrightAttachment,      XmATTACH_FORM,
	    XmNbottomAttachment,      XmATTACH_FORM,
	    XmNtopOffset,          0,
	    XmNleftOffset,          0,
	    XmNrightOffset,          0,
	    XmNbottomOffset,          0,
	NULL);
    } else {
	Widget form = XtVaCreateWidget(this->name,xmFormWidgetClass, parent, NULL);
	this->setRootWidget(form);

	this->textField = XtVaCreateManagedWidget("textField",
	    xmTextWidgetClass,form,
	    XmNleftAttachment,      XmATTACH_FORM,
	    XmNleftOffset,          2,
	    XmNtopAttachment,       XmATTACH_FORM,
	    XmNtopOffset,           0,
	NULL);

	this->diag_button = XtVaCreateManagedWidget("diagButton",
	    xmPushButtonWidgetClass,form,
	    XmNrightAttachment,     XmATTACH_FORM,
	    XmNrightOffset,         2,
	    XmNtopAttachment,       XmATTACH_FORM,
	    XmNtopOffset,           2,
	    XmNwidth,		ELLIPSIS_WIDTH,
	    XmNheight,		SELECTOR_HEIGHT,
	    XmNshadowThickness,	1,
	NULL);

	XtUninstallTranslations(this->diag_button);
	XtAddEventHandler (this->diag_button, ButtonPressMask, False,
	    (XtEventHandler)TextSelector_EllipsisEH, (XtPointer)this);

	XtVaSetValues(this->textField,
	    XmNrightAttachment,      XmATTACH_WIDGET,
	    XmNrightWidget,          this->diag_button,
	    XmNrightOffset,          10,
	NULL);


	//
	// Create the menu
	//
	Pixel fg,bg;
	XtVaGetValues (this->textField, XmNforeground, &fg, XmNbackground, &bg, NULL);

	Arg args[20];
	int n = 0;
	this->popupMenu = 
	    XtCreatePopupShell ("popupMenu", overrideShellWidgetClass, 
		this->getRootWidget(), args, n);

	Widget rcForm = XtVaCreateManagedWidget ("rcForm",
	    xmFormWidgetClass, this->popupMenu,
	    XmNshadowThickness, 1,
	    XmNshadowType, XmSHADOW_OUT,
	    XmNresizePolicy, XmRESIZE_ANY,
	    XmNforeground, fg,
	    XmNbackground, bg,
	NULL);

	n = 0;
	XtSetArg (args[n], XmNtopAttachment, XmATTACH_FORM); n++;
	XtSetArg (args[n], XmNleftAttachment, XmATTACH_FORM); n++;
	XtSetArg (args[n], XmNrightAttachment, XmATTACH_FORM); n++;
	XtSetArg (args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
	XtSetArg (args[n], XmNtopOffset, 3); n++;
	XtSetArg (args[n], XmNleftOffset, 3); n++;
	XtSetArg (args[n], XmNrightOffset, 3); n++;
	XtSetArg (args[n], XmNbottomOffset, 3); n++;
	XtSetArg (args[n], XmNlistSizePolicy, XmCONSTANT); n++;
	XtSetArg (args[n], XmNvisibleItemCount, MAX_VISIBLE); n++;
	XtSetArg (args[n], XmNselectionPolicy, XmBROWSE_SELECT); n++;
	this->popupList = XmCreateScrolledList (rcForm, "itemList", args, n);
	XtVaSetValues (this->popupList, XmNshadowThickness, 0, NULL);
	XtManageChild (this->popupList);
	XtAddCallback (this->popupList, XmNsingleSelectionCallback,
	    (XtCallbackProc)TextSelector_SelectCB, (XtPointer)this);
	XtAddCallback (this->popupList, XmNbrowseSelectionCallback,
	    (XtCallbackProc)TextSelector_SelectCB, (XtPointer)this);
	XtAddCallback (this->popupList, XmNdefaultActionCallback,
	    (XtCallbackProc)TextSelector_SelectCB, (XtPointer)this);
	XtAddEventHandler(this->popupList, ButtonReleaseMask, False, (XtEventHandler)
	    TextSelector_RemoveGrabEH, (XtPointer)this);
	XtAddEventHandler(this->popupList, EnterWindowMask, False, (XtEventHandler)
	    TextSelector_ProcessOldEventEH, (XtPointer)this);
	XtVaGetValues (XtParent(this->popupList), 
	    XmNverticalScrollBar, &this->vsb,
	    XmNhorizontalScrollBar, &this->hsb,
	NULL);

	if (cbp) {
	    XtAddCallback (this->popupList, XmNsingleSelectionCallback, cbp, cdata);
	    XtAddCallback (this->popupList, XmNbrowseSelectionCallback, cbp, cdata);
	    XtAddCallback (this->popupList, XmNdefaultActionCallback, cbp, cdata);
	}
    }

    this->enableModifyCB(this->item_list.getSize() > 0);
}

//
// Put an entry into the scrolled list for each entry in the dictionary.
//
void TextSelector::updateList()
{

    //
    // Clear the list
    //
    XmListDeleteAllItems(this->popupList);

    //
    // Fill in the list with the contents of our item_list
    //
    XmString* strTable = new XmString[this->item_list.getSize()];
    int next = 0;
    ListIterator it(this->item_list);
    char* name;
    while ( (name = (char*)it.getNext()) ) {
	strTable[next++] = XmStringCreateLtoR (name, "bold");
    }

    XtVaSetValues (this->popupList, XmNvisibleItemCount, 
	(next<=MAX_VISIBLE?next:MAX_VISIBLE), NULL);
    XtVaSetValues (XtParent(this->popupList), 
	XmNwidth, 160, NULL);

    //
    // Show the item as selected in the list and make sure that position is showing.
    //
    XmListAddItemsUnselected (this->popupList, strTable, next, 1);
    if (this->selected_items) {
	int select_this_item = this->selected_items[0];
	XmListSelectPos (this->popupList, select_this_item, False);
	int toppos;
	int maxtoppos = 1 + next - MAX_VISIBLE;
	if (select_this_item <= MAX_VISIBLE) toppos = 1;
	else toppos = select_this_item - 3;
	if (toppos >= maxtoppos) toppos = 0;
	if (toppos)
	    XmListSetPos (this->popupList, toppos);
	else 
	    //
	    // According to the Motif doc passing 0 to this func makes the last
	    // item in the list, the last visible item.  I've never seen it work, though.
	    // The result is that if you've selected the last item, then the list will
	    // never be scrolled so that that item is showing.
	    //
	    XmListSetBottomPos (this->popupList, 0);
    }

    int i;
    for (i=0; i<next; i++)
	XmStringFree (strTable[i]);
    delete strTable;
}

void TextSelector::ungrab()
{
    if (TextSelector::GrabOwner == this)
	TextSelector::GrabOwner = NUL(TextSelector*);
    if (!this->is_grabbed) return ;
    XtUngrabPointer (this->popupList, CurrentTime);
    XtPopdown (this->popupMenu);
    if (this->button_release_timer) {
	XtRemoveTimeOut (this->button_release_timer);
	this->button_release_timer = 0;
    }
    if (this->old_event) {
	delete this->old_event;
	this->old_event = NUL(XEvent*);
    }
    this->is_grabbed = FALSE;
}
void TextSelector::grab(XEvent* e)
{
    if (!TextSelector::GrabCursor) {
	TextSelector::GrabCursor = 
	    XCreateFontCursor (XtDisplay(this->popupMenu), XC_arrow);
    }

    if (TextSelector::GrabOwner != NUL(TextSelector*)) { 
	if (TextSelector::GrabOwner != this)
	    TextSelector::GrabOwner->ungrab();
	else
	    return ;
    }
    TextSelector::GrabOwner = this;

    XtPopup (this->popupMenu, XtGrabNone);
    XtAppContext apcxt = theApplication->getApplicationContext();
    this->action_hook = XtAppAddActionHook (apcxt, (XtActionHookProc)
	TextSelector_PopupListAH, (XtPointer)this);
    XtGrabPointer (this->popupList, True, 
	ButtonPressMask | ButtonReleaseMask , GrabModeAsync, 
	GrabModeAsync, None, TextSelector::GrabCursor, e->xbutton.time);

    this->is_button_release_grabbed = FALSE;
    if (this->button_release_timer)
	XtRemoveTimeOut (this->button_release_timer);

    this->button_release_timer = XtAppAddTimeOut (apcxt, 500, (XtTimerCallbackProc)
	TextSelector_GrabReleaseTO, (XtPointer)this);

    //
    // Squirrel away the XEvent so that it can be processed if the user drags
    // into the list. Use the following line to process it.
    // XtCallActionProc (this->popupList, "ListBeginSelect", e, NULL, 0);
    // You know it needs to be processed if you get EnterWindow on the popupList
    // with the mouse button down.
    //
    if (this->old_event)
	delete this->old_event;
    this->old_event = new XEvent;
    memcpy (this->old_event, e, sizeof(XEvent));

    this->is_grabbed = TRUE;
}

extern "C"  {

void TextSelector_GrabReleaseTO(XtPointer clientData, XtIntervalId* )
{
    TextSelector* psel = (TextSelector*)clientData;
    ASSERT(psel);
    psel->is_button_release_grabbed = TRUE;
    psel->button_release_timer = 0;
}


void TextSelector_ProcessOldEventEH(Widget w, XtPointer clientData, XEvent *e, Boolean *)
{
    TextSelector* psel = (TextSelector*)clientData;
    ASSERT(psel);
    if ((e->type == EnterNotify) && (psel->old_event)) {
	XCrossingEvent* xce = (XCrossingEvent*)e;
	int state = xce->state;
	if (state & Button1Mask) {
	    XtCallActionProc (psel->popupList, "ListBeginSelect", 
		psel->old_event, NULL, 0);
	    delete psel->old_event;
	    psel->old_event = NUL(XEvent*);
	}
    }
    if (psel->old_event) {
	delete psel->old_event;
	psel->old_event = NUL(XEvent*);
    }
}

void TextSelector_RemoveGrabEH(Widget w, XtPointer clientData, XEvent *, Boolean *)
{
    TextSelector* psel = (TextSelector*)clientData;
    ASSERT(psel);
    if (psel->is_button_release_grabbed)
	psel->ungrab();
    if (psel->button_release_timer) {
	XtRemoveTimeOut (psel->button_release_timer);
	psel->button_release_timer = 0;
    }
    
    psel->is_button_release_grabbed = TRUE;
}
void TextSelector_EllipsisEH(Widget , XtPointer clientData, XEvent *e, Boolean *)
{
    TextSelector* psel = (TextSelector*)clientData;
    ASSERT(psel);
    if (psel->is_grabbed == TRUE)
	return ;
    psel->updateList();

    Dimension width, height;
    Screen *scrptr;
    XtVaGetValues (XtParent(psel->popupList), 
	XmNwidth, &width, XmNheight, &height, 
	XmNscreen, &scrptr,
    NULL);

    Position x = e->xbutton.x_root;
    Position y = e->xbutton.y_root;

    x-= e->xbutton.x;
    y-= e->xbutton.y;
    y+= SELECTOR_HEIGHT;

    if ((x+width) > WidthOfScreen(scrptr)) 
	x = WidthOfScreen(scrptr) - (width+4);
    if ((y+height) > HeightOfScreen(scrptr)) 
	y-= (SELECTOR_HEIGHT + height + 7);

    XtVaSetValues (psel->popupMenu, XmNx, x, XmNy, y, NULL);

    e->xbutton.x = e->xbutton.y = 0;
    psel->grab(e);
}

//
// If the widget of the event is not the popuplist or one of its scrollbars,
// then popdown the list.
//
void TextSelector_PopupListAH (Widget w, XtPointer clientData, String, XEvent* xev,
String*, Cardinal*)
{
    TextSelector* psel = (TextSelector*)clientData;
    ASSERT(psel);
    if (psel->is_grabbed) {
	if ((xev->type == ButtonPress) || (xev->type == KeyPress)) {
	    if ((w != psel->popupList) && (w != psel->hsb) && (w != psel->vsb))
		psel->ungrab();
	}
    } else {
	//
	// It would be nice to remove the action hook here, but the intrinsics
	// assume that if you do that inside the action hook proc, then you would
	// like to reference freed memory and core dump.   Usually we prefer not
	// to do that, so we add a work proc and remove the action hook there.
	// It isn't harmful to run the action hook too long, anyway.
	//
	if (psel->remove_hook_wpid == 0) {
	    XtAppContext apcxt = theApplication->getApplicationContext();
	    psel->remove_hook_wpid = XtAppAddWorkProc (apcxt,
		TextSelector_RemoveHookWP, (XtPointer)psel);
	}
    }
}

Boolean TextSelector_RemoveHookWP (XtPointer clientData)
{
    TextSelector* psel = (TextSelector*)clientData;
    ASSERT(psel);
    if (psel->action_hook)
	XtRemoveActionHook (psel->action_hook);
    psel->action_hook = 0;
    psel->remove_hook_wpid = NUL(XtWorkProcId);
    return TRUE;
}

} // extern "C"



//
// Get the text from a text widget and clip off the leading and
// trailing white space.
// The return string must be deleted by the caller.
//
char *TextSelector::GetTextWidgetToken(Widget textWidget)
{
    char *name = XmTextGetString(textWidget);
    ASSERT(name);
    int i,len = STRLEN(name);

    for (i=len-1 ; i>=0 ; i--) {
	if (IsWhiteSpace(name,i))
	    name[i] = '\0';
	else
	    break;
    }

    i=0; SkipWhiteSpace(name,i);

    char *s = DuplicateString(name+i);
    XtFree(name);
    return s;
}

void TextSelector_SelectCB(Widget , XtPointer clientData, XtPointer cbs)
{
    TextSelector* psel = (TextSelector*)clientData;
    ASSERT(psel);

    psel->ungrab();

    //
    // Error check hacking...  If the mouse event was outside the list widget,
    // then ignore it.
    //
    XmListCallbackStruct *lcs = (XmListCallbackStruct*)cbs;
    XEvent* xev = lcs->event;
    boolean inside = TRUE;
    if ((xev) && ((xev->type == ButtonPress) || (xev->type == ButtonRelease))) {
	XButtonEvent* xbe = (XButtonEvent*)xev;
	Dimension width, height;
	XtVaGetValues (psel->popupList, XmNwidth, &width, XmNheight, &height, NULL);
	if ((xbe->x < 0) || (xbe->y < 0) || (xbe->x > width) || (xbe->y > height))
	    inside = FALSE;
    }

    //
    // Make the selection based on list contents
    //
    if ((inside) && (lcs->selected_item_count)) {
	int pos = lcs->item_position;
	if ((pos >= 1) && (pos <= psel->item_list.getSize())) {
	    //
	    // Put a new item into the text widget
	    //
	    char* cp = (char*)psel->item_list.getElement(lcs->item_position);
	    ASSERT(cp);
	    XmTextSetString (psel->textField, cp);

	    //
	    // Remember what's currently selected
	    //
	    if (psel->selected_items) 
		XtFree((char*)psel->selected_items);
	    boolean sel = XmListGetSelectedPos
		(psel->popupList, &psel->selected_items, &psel->selected_cnt);
	    if (sel == False) {
		psel->selected_items = NUL(int*);
		psel->selected_cnt = 0;
	    }
	}
    }
}

void TextSelector::setMenuColors (Arg args[], int n)
{
    XtSetValues (this->popupMenu, args, n);
    XtSetValues (XtParent(this->popupList), args, n);
    XtSetValues (XtParent(XtParent(this->popupList)), args, n);
    XtSetValues (this->popupList, args, n);
    XtSetValues (this->vsb, args, n);
    XtSetValues (this->hsb, args, n);
}

//
// If the proposed modification to the text widget would result in a text
// string which is represented in the list, then allow the modification to
// proceed, else not.
//
extern "C" void
TextSelector_ModifyCB (Widget w, XtPointer cdata, XtPointer callData)
{
    TextSelector* tsel = (TextSelector*)cdata;
    ASSERT (tsel);
    XmTextVerifyCallbackStruct *tvcs = (XmTextVerifyCallbackStruct*)callData;
    boolean good_chars = TRUE;

    XmTextBlock tbrec = tvcs->text;

    if ((tbrec->length) && (tvcs->reason == XmCR_MODIFYING_TEXT_VALUE)) {
	char* cp = XmTextGetString (tsel->textField);
	int plen = (cp?strlen(cp):0) + tbrec->length + 1;
	char* proposed = new char[plen];

	//
	// you might expect cp to be <nil> if there were no chars in the text
	// widget but, it really should be "" in that case, so it should never
	// be <nil>.
	//
	ASSERT (cp);
	strcpy (proposed, cp);
	strcpy (&proposed[tvcs->startPos], tbrec->ptr);
	strcat (proposed, &cp[tvcs->startPos]);

	//
	// Now lookup proposed in the list of items
	//
	ListIterator it(tsel->item_list);
	const char* item;
	int found = 0;
	boolean match = FALSE;
	int i = 1;
	while ( (item = (char*)it.getNext()) ) {
	    if (tsel->equalString (item, cp)) {
		match = TRUE;
		break;
	    } else if (tsel->equalSubstring (item, proposed, plen-1)) {
		found++;
	    }
	    i++;
	}
	if (match) {
	} else if (found == 0)
	    good_chars = FALSE; 
	else if (found == 1) {
	    //
	    // This is the place to complete the entry automatically
	    // It can't be done here because the proposed modification to
	    // the string would still happen after you've updated the contents.
	    //tsel->setSelectedItem (most_recent);
	    if (tsel->auto_fill_wpid)
		XtRemoveWorkProc (tsel->auto_fill_wpid);
	    XtAppContext apcxt = theApplication->getApplicationContext();
	    tsel->auto_fill_wpid = XtAppAddWorkProc (apcxt, (XtWorkProc)
		TextSelector_FillWP, (XtPointer) tsel);
	}

	if (proposed) delete proposed;
    }
    tvcs->doit = (Boolean)good_chars;
    if (!tvcs->doit) 
	XBell(XtDisplay(w), 100);
}

extern "C" Boolean
TextSelector_FillWP (XtPointer cdata)
{
    TextSelector* tsel = (TextSelector*)cdata;
    ASSERT (tsel);
    tsel->auto_fill_wpid = NUL(XtWorkProcId);

    tsel->autoFill();

    return True;
}
boolean TextSelector::autoFill(boolean any_match)
{
    boolean unused;
    return this->autoFill(any_match, unused);
}

// if (any_match) then if we don't find an exact match but
// we do find multiple substring matches, we'll accept the
// first substring match
boolean TextSelector::autoFill(boolean any_match, boolean& unique_match)
{
    boolean retVal = FALSE;
    unique_match = FALSE;
    char* cp = XmTextGetString (this->textField);
    if ((!cp) || (!cp[0])) return FALSE;
    int len = strlen(cp);

    //
    // Now lookup proposed in the list of items
    //
    ListIterator it(this->item_list);
    const char* item;
    int found = 0;
    int most_recent=0;
    int index_of_first_match = -1;
    int i = 1;
    while ( (item = (char*)it.getNext()) ) {
	// we don't need both tests do we?
	if ((this->equalString (item, cp)) || (this->equalSubstring(item,cp,len))) {
	    found++;
	    most_recent = i;
	    if (index_of_first_match == -1)
		index_of_first_match = i;
	}
	i++;
    }
    if (found == 1)  {
	this->setSelectedItem (most_recent);
	if (this->popupList)
	    XmListSelectPos (this->popupList, most_recent, True);
	retVal = TRUE;
	unique_match = TRUE;
    } else if ((found > 1) && (any_match)) {
	ASSERT(cp); // can we have a match with no text?
	int start = strlen(cp);
	this->setSelectedItem (index_of_first_match);
	char *match = (char*)this->item_list.getElement(index_of_first_match);
	int end = strlen(match);
	XmTextSetSelection (this->textField, start, end, 
		XtLastTimestampProcessed(XtDisplay(this->textField)));
	//
	// Some of the text just entered might not be correct since
	// we're accepting the first available match out of several.
	// Therefore, we'll select the text so that further typing
	// overwrites the erroneous portion.
	//
	if (this->popupList)
	    XmListSelectPos (this->popupList, index_of_first_match, True);
	retVal = TRUE;
    }
    XtFree(cp);
    return retVal;
}

boolean TextSelector::equalString(const char* s1, const char* s2)
{
    if (this->case_sensitive) return EqualString(s1,s2);

    int len = STRLEN(s1);
    if (len != STRLEN(s2)) return FALSE;

    boolean retval = TRUE;
    for (int i=0; i<len; i++) {
	if (s1[i] == s2[i]) continue;
	if (tolower(s1[i]) == tolower(s2[i])) continue;
	retval = FALSE;
	break;
    }

    return retval;
}

boolean TextSelector::equalSubstring(const char* s1, const char* s2, int n)
{
    if (this->case_sensitive) return EqualSubstring(s1,s2,n);

    // what's supposed to happen if s2 is ""?
    if (!s2[0]) return FALSE;

    boolean retval = TRUE;

    for (int i=0; i<n; i++) {
	if (s2[i]=='\0') break;
	if (s1[i]=='\0') {
	    retval = FALSE;
	    break;
	}
	if (s1[i] == s2[i]) continue;
	if (tolower(s1[i]) == tolower(s2[i])) continue;
	retval = FALSE;
	break;
    }

    return retval;
}

extern "C" void
TextSelector_ResolveCB (Widget, XtPointer cdata, XtPointer)
{
    TextSelector* tsel = (TextSelector*)cdata;
    ASSERT (tsel);

    if ((tsel->autoFill(TRUE) == FALSE) && (tsel->popupList)) {
	if (tsel->selected_items) {
	    XmListSelectPos (tsel->popupList, tsel->selected_items[0], True);
	} else {
	    XmListDeselectAllItems (tsel->popupList);
	}
    }
}
