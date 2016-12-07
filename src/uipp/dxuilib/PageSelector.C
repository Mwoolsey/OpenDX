/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#include "PageSelector.h"
#include "EditorWindow.h"
#include "VPERoot.h"
#include "ListIterator.h"
#include "DXStrings.h"
#include "DXApplication.h"
#include "ErrorDialogManager.h"
#include "Network.h"
#include "PageGroupManager.h"
#include "PageTab.h"
#include "SetPageNameDialog.h"
#include "MoveNodesDialog.h"
#include "lex.h"

#include <X11/IntrinsicP.h>
#include <X11/Composite.h>
#include <X11/ShellP.h>
#include <Xm/Form.h>
#include <Xm/Label.h>
#include <Xm/ToggleB.h>
#include <Xm/PushB.h>
#include <Xm/Text.h>
#include <Xm/RowColumn.h>
#include <Xm/List.h>
#include <Xm/MenuShell.h>
#include <Xm/ScrolledW.h>
#include <X11/cursorfont.h>
#include <X11/Shell.h>

#include <ctype.h> // for isalnum

#define BUTTON_WIDTH 90
#define ELLIPSIS_WIDTH 23
#define SELECTOR_HEIGHT 23
#define MAX_VISIBLE 12

//
// Provide a translation table for the page_name_prompt widget.  We want to use
// a grab (XtAddGrab) when the widget is displayed because we want to keep all
// keyboard events going into the widget while it's on the screen so that you
// can type into the thing with the mouse outside the widget and so that the 
// delete key doesn't erase selected nodes.
//
XtTranslations PageSelector::PnpTranslations = 0;

String PageSelector::PnpTranslationText = "\
 <Key>osfCancel:			process-cancel() ungrab-kbd()\n\
 <Key>Escape:				process-cancel() ungrab-kbd()\n\
 <EnterWindow>:				enter() keeping-grab()\n\
 <LeaveWindow>:				leave() losing-grab()\n\
 ~Ctrl  Shift ~Meta ~Alt<Btn1Down>:	extend-start() monitor-grab()\n\
  Ctrl ~Shift ~Meta ~Alt<Btn1Down>:	move-destination() monitor-grab()\n\
 ~Ctrl ~Shift ~Meta ~Alt<Btn1Down>:	grab-focus() monitor-grab()\n\
 <Btn2Down>:				ungrab-kbd()\n\
 <Btn3Down>:				ungrab-kbd()\n\
";

XtActionsRec PageSelector::PnpActions[] = {
 { "ungrab-kbd",		(XtActionProc)PageSelector_UngrabAP 	},
 { "keeping-grab",		(XtActionProc)PageSelector_KeepingAP 	},
 { "losing-grab",		(XtActionProc)PageSelector_LosingAP 	},
 { "monitor-grab",		(XtActionProc)PageSelector_MonitoringAP	}
};

Cursor PageSelector::GrabCursor = 0;

//
// The color #ddddddddd is chosen because it will both stand out, and go easy
// on the colormap since it's the same as WorkSpace.selectColor
//
String PageSelector::DefaultResources[] = {
    "*popupMenu.borderWidth: 0",
    "*popupMenu.allowShellResize: True",
    "*pagePrompt.fontList: -adobe-helvetica-bold-r-normal--12-*iso8859-1",
    "*pagePrompt.background:		#ddddddddd",
    "*pagePrompt.topShadowColor:	#ddddddddd",
    "*pagePrompt.bottomShadowColor:	#ddddddddd",
    "*pagePrompt.shadowThickness:	0",
    "*pagePrompt.borderWidth:		1",
    "*pagePrompt.borderColor:		black",
    "*pagePrompt.marginWidth:		2",
    "*pagePrompt.marginHeight:		2",
    "*diagButton.labelString:		...",
    "*diagButton.recomputeSize:		False",
    "*diagButton.indicatorOn:		False",
    "*diagButton.topOffset: 0",
    "*diagButton.leftOffset: 0",
    "*diagButton.rightOffset: 0",
    "*diagButton.bottomOffset: 0",
    NUL(char*)
};

boolean PageSelector::ClassInitialized = FALSE;

PageSelector::PageSelector 
    (EditorWindow* editor, Widget parent, Network* net) : 
    UIComponent ("pageSelector"),  Dictionary (FALSE, FALSE)
{
    this->editor = editor;
    this->parent = parent;
    this->net = net;
    this->root = NUL(VPERoot*);
    this->page_name_prompt = NUL(Widget);
    this->page_buttons = new List;
    this->selecting_page = FALSE;
    this->num_pages_when_empty = 0;
    this->button_release_timer = 0;
    this->diag_button = NUL(Widget);
    this->popupMenu = NUL(Widget);
    this->popupList = NUL(Widget);
    this->is_grabbed = FALSE;
    this->starting_button = 1;
    this->action_hook = 0;
    this->vsb = this->hsb = NUL(Widget);
    this->old_event = NUL(XEvent*);
    this->page_dialog = NUL(SetPageNameDialog*);
    this->move_dialog = NUL(MoveNodesDialog*);
    this->remove_hook_wpid = NUL(XtWorkProcId);

    if (!PageSelector::ClassInitialized) {
	PageSelector::ClassInitialized = TRUE;
	this->setDefaultResources(theDXApplication->getRootWidget(), 
	    PageSelector::DefaultResources);

	XtAppContext apcxt = theApplication->getApplicationContext();
	XtAppAddActions (apcxt, PageSelector::PnpActions, 
	    XtNumber(PageSelector::PnpActions));

	PageSelector::PnpTranslations = 
	    XtParseTranslationTable (PageSelector::PnpTranslationText);
    }

    this->buildSelector();
}

PageSelector::~PageSelector()
{
    if (this->page_buttons) {
	// The superclass destructor calls clear() but I'm confused
	// about what the virtual call from the destructor is going
	// to do.  So I'll add the needed statements.
	ListIterator it (*this->page_buttons);
	PageTab* page_button;
	while ( (page_button = (PageTab*)it.getNext()) ) {
	    page_button->unmanage();
	    delete page_button;
	}
	delete this->page_buttons;
	this->page_buttons = NUL(List*);
    }
    if (this->button_release_timer)
	XtRemoveTimeOut(this->button_release_timer);
    if (this->is_grabbed)
	this->ungrab();
    if (this->old_event)
	delete this->old_event;
    if (this->page_dialog)
	delete this->page_dialog;
    if (this->move_dialog)
	delete this->move_dialog;
    if (this->action_hook)
	XtRemoveActionHook (this->action_hook);
    if (this->remove_hook_wpid)
	XtRemoveWorkProc (this->remove_hook_wpid);
}

void PageSelector::clear()
{
    if (this->page_name_prompt) {
	XtDestroyWidget (this->page_name_prompt);
	this->page_name_prompt = NUL(Widget);
    }
    if (this->page_buttons) {
	ListIterator it (*this->page_buttons);
	PageTab* page_button;
	while ( (page_button = (PageTab*)it.getNext()) ) {
	    page_button->unmanage();
	    delete page_button;
	}
	delete this->page_buttons;
	this->page_buttons = new List;
    }
    this->starting_button = 1;
    this->Dictionary::clear();
    this->addDefinition ("Untitled", this->root);
    this->num_pages_when_empty = 1;
    if (this->is_grabbed)
	this->ungrab();
    if (this->move_dialog)
	this->move_dialog->unmanage();
    if (this->page_dialog)
	this->page_dialog->unmanage();
}

void PageSelector::buildSelector()
{

    ASSERT(this->parent);
    Widget form = XtVaCreateManagedWidget ("pageSelector",
	xmFormWidgetClass,	this->parent,
	XmNshadowThickness,	2,
	XmNheight,		SELECTOR_HEIGHT,
	XmNresizable,		False,
	XmNshadowType,		XmSHADOW_ETCHED_OUT,
    NULL);
    this->setRootWidget(form);

    XtVaCreateManagedWidget ("coverUp",
	xmLabelWidgetClass,	form,
	XmNtopAttachment,	XmATTACH_FORM,
	XmNleftAttachment,	XmATTACH_FORM,
	XmNrightAttachment,	XmATTACH_FORM,
	XmNbottomAttachment,	XmATTACH_FORM,
	XmNlabelType,		XmPIXMAP,
	XmNlabelPixmap,		XmUNSPECIFIED_PIXMAP,
	XmNtopOffset,		0,
	XmNleftOffset,		0,
	XmNrightOffset,		0,
	XmNbottomOffset,	1,
    NULL);

#if !defined(aviion)
    this->buildPageMenu();
#endif

    XtAddEventHandler(form, StructureNotifyMask, False, (XtEventHandler)
	PageSelector_ResizeHandlerEH, (XtPointer)this);
}

//
// When the window is resized, add or remove buttons for existing pages
// based on the available space.  
//
void PageSelector::resizeCallback()
{
int i;
PageTab* pbut; 
PageTab* prev = NUL(PageTab*);
int selected_button = 1;

    if (!this->page_buttons) return ;
    if (this->page_buttons->getSize() == 0) return ;

    Dimension totWidth;
    ASSERT (this->getRootWidget());
    XtVaGetValues (this->getRootWidget(), XmNwidth, &totWidth, NULL);
    totWidth-= ELLIPSIS_WIDTH;

    int max_buttons = totWidth/BUTTON_WIDTH;
    int tot_buttons = this->page_buttons->getSize();
    max_buttons = MIN(max_buttons, tot_buttons);

    //
    // If we can fit in the last button by excluding the ellipsis button...
    //
    if (tot_buttons == (max_buttons+1)) {
	int tw = totWidth + ELLIPSIS_WIDTH;
	int mw = tw/BUTTON_WIDTH;
	if (mw == tot_buttons)
	    max_buttons = mw;
    }

    //
    // Attempt to make the selected button in the set of managed buttons.
    //
    boolean reset_top = FALSE;
    for (i=1; i<=tot_buttons; i++) {
	pbut = (PageTab*)this->page_buttons->getElement(i);
	if (pbut->getState()) {
	    selected_button = i;

	    //
	    // If the selected button is outside the set of visible buttons
	    //
	    if (i < this->starting_button)  {
		reset_top = TRUE;
	    } else if ((this->starting_button + max_buttons) <= i) {
		reset_top = TRUE;
	    } 
	    break;
	}
    }

    //
    // There will be no selected_button if we just deleted the selected button.
    // It's wrong to just go and select some other button, because we want to avoid
    // selecting some bigpig page.
    //
    //ASSERT(selected_button);


    int hidden_buttons = tot_buttons - max_buttons;
    if ((reset_top) || (!hidden_buttons)) {
	this->starting_button = selected_button - (max_buttons>>1);
    }
    this->starting_button = MIN(this->starting_button, 1+(tot_buttons - max_buttons));
    this->starting_button = MAX(1,this->starting_button);

    boolean need_diag_button = FALSE;
    for (i=1; i<this->starting_button; i++) {
	pbut = (PageTab*)this->page_buttons->getElement(i);
	pbut->unmanage();
	need_diag_button = TRUE;
    }
    int ending_button = this->starting_button + max_buttons;

    boolean first = TRUE;
    for (i=this->starting_button; i<ending_button; i++) {
	pbut = (PageTab*)this->page_buttons->getElement(i);
	ASSERT(pbut);
	XtVaSetValues (pbut->getRootWidget(), 
	    XmNleftAttachment,	(first?XmATTACH_FORM:XmATTACH_WIDGET),
	    XmNleftWidget,	(first?NUL(Widget):prev->getRootWidget()),
	NULL);
	pbut->manage();
	prev = pbut;
	first = FALSE;
    }

    //
    // If there is any remaining button(s), then display the ... button which
    // leads to a dialog for showing the others.
    //
    for (i=ending_button; i<=tot_buttons; i++) {
	pbut = (PageTab*)this->page_buttons->getElement(i);
	pbut->unmanage();
	need_diag_button = TRUE;
    }

    ASSERT (need_diag_button == (max_buttons<tot_buttons));
    boolean have_diag_button = (this->diag_button?XtIsManaged (this->diag_button):FALSE);
    if (need_diag_button) {
	if (!this->diag_button) {
	    this->diag_button = XtVaCreateWidget ("diagButton",
		xmToggleButtonWidgetClass, this->getRootWidget(),
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNwidth, ELLIPSIS_WIDTH,
	    NULL);
	    XtUninstallTranslations (this->diag_button);
	    XtAddEventHandler (this->diag_button, ButtonPressMask, False,
		(XtEventHandler)PageSelector_EllipsisEH, (XtPointer)this);
	}
	XtVaSetValues (this->diag_button, 
	    XmNleftAttachment, XmATTACH_WIDGET,
	    XmNleftWidget,	prev->getRootWidget(),
	NULL);
	if (!have_diag_button) {
	    XtManageChild (this->diag_button);
	}
    } else if (have_diag_button) {
	XtDestroyWidget (this->diag_button);
	this->diag_button = NUL(Widget);
    }
}


void PageSelector::addButton (const char* name, const void* definition)
{
    int size = this->page_buttons->getSize();
    boolean first = (boolean)(size == 0);

    PageTab* new_button = new PageTab (this, (WorkSpace*)definition, name);

    PageGroupRecord* prec = this->getRecordOf(name);
    if (prec) {
	new_button->createButton(this->getRootWidget(), prec);
	new_button->setGroup(prec);
    } else {
	new_button->createButton(this->getRootWidget());
    }

    XtVaSetValues(new_button->getRootWidget(),
	XmNtopAttachment,		XmATTACH_FORM,
	XmNbottomAttachment,		XmATTACH_FORM,
	XmNwidth,			BUTTON_WIDTH,
    NULL);
    if (first) new_button->setState();

    this->appendButton(new_button);
    ASSERT (this->getSize() == this->page_buttons->getSize());

    //
    // To keep the button ordering up to date, visit each button.  You would think
    // that we would only need to set the order number for the current button,
    // but what we're really doing is just maintaining an ordering, not assigning
    // an order number which equals the positions down from the top.  This way
    // it isn't necessary to update the order numbers when removing a button.
    //
    int i,buts = this->getSize();
    for (i=1; i<=buts; i++) {
	PageTab* pbut = (PageTab*)this->page_buttons->getElement(i);
	pbut->setPosition(i);
    }

    new_button->manage();
    this->resizeCallback();
    this->updateDialogs();
}

PageGroupRecord* 
PageSelector::getRecordOf (const char* name)
{
    Dictionary* dict = this->net->getGroupManagers();
    Symbol psym = theSymbolManager->getSymbol(PAGE_GROUP);
    PageGroupManager* pmgr = (PageGroupManager*)dict->findDefinition (psym);
    PageGroupRecord *prec = (PageGroupRecord*)pmgr->getGroup(name);
    return prec;
}

//
// Using the information in the PageGroupRecords, try to put this button in
// the proper order in the list.
//
void PageSelector::appendButton (PageTab *button)
{
    if (!this->page_buttons)
	this->page_buttons = new List;

    //
    // Now compare button_num with the same peice of info for the
    // other buttons already in the list.
    //
    boolean position_found = FALSE;
    if (button->hasDesiredPosition()) {
	int bpos = button->getDesiredPosition();
	ListIterator it(*this->page_buttons);
	PageTab* pbut;
	while ( (pbut = (PageTab*)it.getNext()) ) {
	    if (pbut == button) continue;
	    int rove_pos = (pbut->hasDesiredPosition()?
		pbut->getDesiredPosition():pbut->getPosition());
	    if (bpos < rove_pos) {
		position_found = TRUE;
		button->setPosition(this->page_buttons->getPosition((void*)pbut));
		break;
	    }
	}
    }
    if (!position_found)
	button->setPosition(this->page_buttons->getSize()+1);
    this->page_buttons->insertElement((void*)button, button->getPosition());
}

void PageSelector::removeButton (const char* name)
{
    ASSERT(this->page_buttons);
    ListIterator it(*this->page_buttons);
    PageTab* destroy_me = NUL(PageTab*);
    PageTab* pbut;
    PageTab* prev = NUL(PageTab*);

    //
    // Search for the button to throw out, and then adjust attachments
    // of trailing buttons.
    //
    while ( (pbut = (PageTab*)it.getNext()) ) {
	if (destroy_me) {
	    XtVaSetValues (pbut->getRootWidget(), 
		XmNleftAttachment,	(prev?XmATTACH_WIDGET:XmATTACH_FORM),
		XmNleftWidget,		(prev?prev->getRootWidget():NUL(Widget)),
	    NULL);
	    prev = pbut;
	} else if (EqualString (pbut->getGroupName(), name)) {
	    ASSERT(destroy_me == NUL(PageTab*));
	    destroy_me = pbut;
	} else {
	    prev = pbut;
	}
    }
    ASSERT(destroy_me);
    boolean was_set = destroy_me->getState();
    this->page_buttons->removeElement((const void*)destroy_me);
    destroy_me->unmanage();

    //
    // If you just destroyed the button which was pushed in, then make
    // sure that some other button gets pushed in.
    //
    if (was_set) {
	if (this->getSize() == 0)
	    this->clear();
	else {
	    PageTab* pt = (PageTab*)this->page_buttons->getElement(1);
	    this->selectPage (pt);
	}
    }

    this->resizeCallback();
    delete destroy_me;
}

void PageSelector::selectPage (Widget toggleBut)
{
    PageTab* pt= NUL(PageTab*);
    ListIterator it(*this->page_buttons);
    while ( (pt = (PageTab*)it.getNext()) )
	if (pt->getRootWidget() == toggleBut) {
	    this->selectPage (pt);
	    break;
	}
}

void PageSelector::selectPage (PageTab* tab)
{
    if (this->selecting_page) return ;
    this->selecting_page = TRUE;

    this->editor->deselectAllNodes();
    this->hidePageNamePrompt();

    //
    // Show the associated page...
    //
    if (tab->getState()) {
	EditorWorkSpace* page = (EditorWorkSpace*)tab->getWorkSpace();
	if (page) {
	    this->root->showWorkSpace(page);
	    this->editor->populatePage(page);
	} else
	    this->root->showRoot();

	//
	// ...then enforce radio button style behavior
	//
	ASSERT(this->page_buttons);
	ListIterator it(*this->page_buttons);
	PageTab* pbut;
	while ( (pbut = (PageTab*)it.getNext()) ) {
	    if (pbut != tab) {
		pbut->setState(FALSE);
	    }
	}
    } else {
	this->root->showRoot();
    }
    this->root->resize();
    this->selecting_page = FALSE;
    this->updateDialogs();

    //
    // PageSelector is a friend of EditorWindow so that this protected
    // method may be used.  The regular mechanism for setting command
    // activation in EditorWindow uses the map callback of the edit
    // menu pane.  The problem with that is that it doesn't handle
    // activation of commands that are associated with buttons that
    // use accelerators.
    //
    this->editor->setCommandActivation();
}

//
// Make sure that the page which is shown has its corresponding
// button pushed in.
//
void PageSelector::selectPage (EditorWorkSpace* ews)
{
    ASSERT(this->page_buttons);
    if (!this->page_buttons->getSize())
	this->clear();

    ListIterator it(*this->page_buttons);
    PageTab* pbut;
    while ( (pbut = (PageTab*)it.getNext()) ) {
	if (pbut->getState() == (pbut->getWorkSpace() == ews)) continue;
	if (pbut->getWorkSpace() == ews) {
	    pbut->setState(TRUE);
	    this->selectPage(pbut);
	    if (pbut->isManaged() == FALSE)
		this->resizeCallback();
	}
    }
    this->updateDialogs();
}

//
// Initialize the dialog using values from whatever tab is currently pushed in
//
void PageSelector::updatePageNameDialog()
{
    if (this->page_dialog) {
	ListIterator it(*this->page_buttons);
	PageTab* pt;
	int pos = 1;
	while ( (pt = (PageTab*)it.getNext()) ) {
	    if (pt->getState()) {
		break;
	    }
	    pos++;
	}
	//
	// pt can be null in the case where we're deleting a page whose tab button
	// was pushed in.
	//
	if (pt) {
	    EditorWorkSpace* ews = (EditorWorkSpace*)pt->getWorkSpace();
	    const char* name = pt->getGroupName();
	    int count = this->page_buttons->getSize();
	    this->page_dialog->setWorkSpace (ews, name, pos, count);
	} else {
	    this->page_dialog->setWorkSpace(NUL(EditorWorkSpace*),
		NUL(char*), 1, 1);
	}
    }
}

void PageSelector::updateMoveNodesDialog()
{
    if (this->move_dialog) {
	ListIterator it(*this->page_buttons);
	PageTab* pt;
	int pos = 1;
	while ( (pt = (PageTab*)it.getNext()) ) {
	    if (pt->getState()) {
		break;
	    }
	    pos++;
	}
	//
	// pt can be null in the case where we're deleting a page whose tab button
	// was pushed in.
	//
	if (pt) {
	    // EditorWorkSpace* ews = (EditorWorkSpace*)pt->getWorkSpace();
	    const char* name = pt->getGroupName();
	    this->move_dialog->setWorkSpace (name);
	} else {
	    this->move_dialog->setWorkSpace(NUL(char*));
	}
    }
}

void PageSelector::updateDialogs()
{
    this->updatePageNameDialog();
    this->updateMoveNodesDialog();
}

void PageSelector::postMoveNodesDialog()
{
    if (!this->move_dialog)
	this->move_dialog = new MoveNodesDialog (this->getRootWidget(), this);
    this->updateMoveNodesDialog();
    this->move_dialog->post();
}
void PageSelector::postPageNameDialog()
{
    if (!this->page_dialog)
	this->page_dialog = new SetPageNameDialog (this->getRootWidget(), this);
    this->updatePageNameDialog();
    this->page_dialog->post();
}

void PageSelector::postPageNamePrompt(PageTab* pt)
{
    if (!this->page_name_prompt) {
	this->page_name_prompt = XtVaCreateWidget ("pagePrompt",
	    xmTextWidgetClass,	this->getRootWidget(),
	    XmNtopOffset, 0,
	    XmNleftOffset, 0,
	    XmNrightOffset, 0,
	    XmNbottomOffset, 0,
	    XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
	    XmNrightAttachment, XmATTACH_OPPOSITE_WIDGET,
	    XmNtopAttachment, XmATTACH_FORM,
	    XmNbottomAttachment, XmATTACH_FORM,
	    XmNmaxLength, 12,
	NULL);
	XtOverrideTranslations (this->page_name_prompt,
	    PageSelector::PnpTranslations);
	XtAddCallback (this->page_name_prompt, XmNactivateCallback, (XtCallbackProc)
	    PageSelector_CommitNameChangeCB, (XtPointer)this);
	XtAddCallback (this->page_name_prompt, XmNmodifyVerifyCallback, (XtCallbackProc)
	    PageSelector_ModifyNameCB, (XtPointer)this);
    }

    XtVaSetValues (this->page_name_prompt,
	XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
	XmNrightAttachment, XmATTACH_OPPOSITE_WIDGET,
	XmNleftWidget, pt->getRootWidget(),
	XmNrightWidget, pt->getRootWidget(),
	XmNvalue, pt->getGroupName(),
	XmNuserData, this,
    NULL);

    this->name_change_in_progress = (EditorWorkSpace*)pt->getWorkSpace();
    this->mouse_inside_name_prompt = FALSE;
    XtAppContext apcxt = theApplication->getApplicationContext();
    XSync (XtDisplay(this->page_name_prompt), False);
    XtAppAddWorkProc (apcxt, PageSelector_PostNamePromptWP, (XtPointer)this);
}

void PageSelector::postPageNamePrompt()
{
    ASSERT (this->page_name_prompt);

    if (!XtIsManaged(this->page_name_prompt)) {
	XtManageChild (this->page_name_prompt);

	if (!PageSelector::GrabCursor) {
	    PageSelector::GrabCursor = 
		XCreateFontCursor (XtDisplay(this->page_name_prompt), XC_arrow);
	}
	XDefineCursor (XtDisplay(this->page_name_prompt), 
	    XtWindow(this->editor->getRootWidget()), PageSelector::GrabCursor);

	XtWidgetGeometry req;
	req.request_mode = CWStackMode;
	req.stack_mode = Above;
	XtMakeGeometryRequest (this->page_name_prompt, &req, NULL);

	XtAppContext apcxt = theApplication->getApplicationContext();
	XSync (XtDisplay(this->page_name_prompt), False);
	XtAppAddWorkProc (apcxt, PageSelector_PostNamePromptWP, (XtPointer)this);
    } else {
	char* str = XmTextGetString (this->page_name_prompt);
	if ((str) && (str[0])) {
	    XtAddGrab (this->page_name_prompt, True, True);
	    Time stamp = XtLastTimestampProcessed(XtDisplay(this->page_name_prompt));
	    XmTextSetSelection (this->page_name_prompt,
		(XmTextPosition)0, (XmTextPosition)strlen(str), stamp);
	}
	if (str) XtFree(str);
    }
}

void PageSelector::hidePageNamePrompt()
{
    if ((this->page_name_prompt) && (XtIsManaged(this->page_name_prompt))) {
	XtUnmanageChild (this->page_name_prompt);
	XtVaSetValues (this->page_name_prompt,
	    XmNleftAttachment, XmATTACH_FORM,
	    XmNrightAttachment, XmATTACH_NONE,
	    XmNvalue, "",
	NULL);

	XtRemoveGrab (this->page_name_prompt);
	this->name_change_in_progress = NUL(EditorWorkSpace*);

	XUndefineCursor (XtDisplay(this->page_name_prompt), 
	    XtWindow(this->editor->getRootWidget()));
    }
}


void PageSelector::highlightTab (EditorWorkSpace* ews, int flag)
{

    //
    // Locate the tab for the referenced workspace.
    //
    boolean is_root = (ews == this->root);
    ListIterator it(*this->page_buttons);
    PageTab* pbut;
    PageTab* found_but = NUL(PageTab*);
    while ( (pbut = (PageTab*)it.getNext()) ) {
	EditorWorkSpace* page = (EditorWorkSpace*)pbut->getWorkSpace();
	if ((page == NUL(EditorWorkSpace*)) && (is_root)) {
	    found_but = pbut;
	    break;
	} else if (page == ews) {
	    found_but = pbut;
	    break;
	}
    }
    ASSERT(found_but);

    //
    // Now get a pixel for the tab.
    //
    Screen* scrptr;
    XtVaGetValues (this->getRootWidget(), XmNscreen, &scrptr, NULL);
    Pixel color = BlackPixelOfScreen(scrptr);
    switch(flag) {
	case EditorWindow::ERRORHIGHLIGHT:
	    color = theDXApplication->getErrorNodeForeground();
	    break;
	case EditorWindow::EXECUTEHIGHLIGHT :
	    color = theDXApplication->getExecutionHighlightForeground();
	    break;
    }
    found_but->setColor(color);
}

boolean PageSelector::verifyPageName (const char* new_name, char* errMsg)
{
    //
    // There is no need to check for a string containing only spaces because we
    // prevent the user for entering a space in the modifyVerifyCallback.
    //
    if ((!new_name) || (!new_name[0])) {
	sprintf (errMsg, "Page names cannot be blank");
	return FALSE;
    }

    Dictionary* dict = this->net->getGroupManagers();
    GroupManager* page_mgr = (GroupManager*)dict->findDefinition(PAGE_GROUP);
    GroupRecord* grec = page_mgr->getGroup(new_name);
    if (grec) {
	sprintf (errMsg, "Page name %s is already in use.", new_name);
	return FALSE;
    }

    if (EqualString(new_name, "Untitled")) {
	sprintf (errMsg, "The name \'Untitled\' is reserved.");
	return FALSE;
    }
    return TRUE;
}


void PageSelector::changePageName(EditorWorkSpace* ews, const char* new_name)
{
    Dictionary* dict = this->net->getGroupManagers();
    GroupManager* page_mgr = (GroupManager*)dict->findDefinition(PAGE_GROUP);
    const char* old_name=NULL;
    Symbol gsym = page_mgr->getManagerSymbol();
    EditorWorkSpace* def;
    int dsize = this->getSize();
    int i;
    boolean found = FALSE;;
    for (i=1; i<=dsize; i++) {
	def = (EditorWorkSpace*)this->getDefinition(i);
	if (def == ews) {
	    old_name = this->getStringKey(i);
	    found = TRUE;
	    break;
	}
    }
    ASSERT(found);
    Symbol delete_symbol = this->getSymbol(i);
    if (EqualString(old_name, new_name)) {
	//
	// do nothing the if name is not being changed.  This has a consequence:
	// The first time thru here, nodes in Untitled belong to no group.  So,
	// if we abort the operation because old_name == new_name, then they still
	// won't belong to a group.
	//
    } else if (this->editor->changeGroup(old_name, new_name, gsym)) {
	this->Dictionary::removeDefinition (delete_symbol);
	this->Dictionary::addDefinition (new_name, ews);
	GroupRecord* grec = page_mgr->getGroup(new_name);
	ASSERT(grec);
	ListIterator it(*this->page_buttons);
	PageTab* pbut;
	found = FALSE;
	while ( (pbut = (PageTab*)it.getNext()) ) {
	    if (ews == pbut->getWorkSpace()) {
		pbut->setGroup ((PageGroupRecord*)grec);
		found = TRUE;
		break;
	    }
	}
	ASSERT(found);
	this->updateDialogs();
    }
}

PageTab* PageSelector::getPageTabOf(const char* name)
{
    ASSERT(this->page_buttons);
    ListIterator it(*this->page_buttons);
    PageTab* pt;
    boolean found = FALSE;
    while ( (pt = (PageTab*)it.getNext()) ) {
	if (EqualString(name, pt->getGroupName())) {
	    found = TRUE;
	    break;
	}
    }
    if (!found) return NUL(PageTab*);
    return pt;
}

//
// Find the PageTab corresponding to move_name.  If it's currently earlier than
// fixed, make it 1 later.  If it's currently later, make it 1 earlier.
// Added the boolean dnd_op so that this method could be available for the page_dialog
// to call without having to force a redraw of 2 buttons.
//
boolean
PageSelector::changeOrdering (PageTab* fixed, const char* move_name, boolean dnd_op)
{
    PageTab* pt = this->getPageTabOf(move_name);
    if (!pt) return FALSE;

    int fixed_position = this->page_buttons->getPosition ((void*)fixed);
    int move_position = this->page_buttons->getPosition ((void*)pt);
    if ((fixed_position == 0)||(move_position == 0))
	return FALSE;

    PageTab* mover = pt;

    boolean retVal = TRUE;
    if (fixed_position == move_position) {
	retVal = FALSE;
    } else {
	const void* def = this->page_buttons->getElement(move_position);
	ASSERT(def == (void*)pt);
	this->page_buttons->deleteElement(move_position);
	this->page_buttons->insertElement(def, fixed_position);

	ListIterator it(*this->page_buttons);
	int position = 1;
	while ( (pt = (PageTab*)it.getNext()) )
	    pt->setPosition (position++, ((pt==fixed)||(pt==mover)));
    }

    this->resizeCallback();

#if !defined(MOTIF_TOGGLES_WORK_AS_DOCUMENTED)
    if (dnd_op) {
	//
	// A workaround for a motif bug.  The problem:  Toggle buttons
	// with XmNindicatorOn==False, redraw themselves incorrectly following a drop.
	// This has nothing to do with making the button a drag source/drop site.  The
	// same bug is evident in other places.  (Try autoaxes dialog.)
	//
	XtAppContext apcxt = theApplication->getApplicationContext();
	Widget w = fixed->getRootWidget();
	XtAppAddTimeOut (apcxt, 100, (XtTimerCallbackProc)
		PageSelector_UndrawTO, (XtPointer)w);
	XtAppAddTimeOut (apcxt, 200, (XtTimerCallbackProc)
		PageSelector_RedrawTO, (XtPointer)w);
	if (retVal) {
	    w = mover->getRootWidget();
	    XtAppAddTimeOut (apcxt, 100, (XtTimerCallbackProc)
		    PageSelector_UndrawTO, (XtPointer)w);
	    XtAppAddTimeOut (apcxt, 200, (XtTimerCallbackProc)
		    PageSelector_RedrawTO, (XtPointer)w);
	}
    }
#endif
    this->updateDialogs();
    return retVal;
}

void PageSelector::buildPageMenu()
{
    Arg args[20];
    int n = 0;
    Widget parent = this->popupMenu = 
	XtCreatePopupShell ("popupMenu", overrideShellWidgetClass, 
	    this->getRootWidget(), args, n);

    parent = XtVaCreateManagedWidget ("rcForm",
	xmBulletinBoardWidgetClass, parent,
	XmNshadowThickness, 1,
	XmNshadowType, XmSHADOW_OUT,
	XmNresizePolicy, XmRESIZE_ANY,
	XmNmarginWidth, 2,
	XmNmarginHeight, 2,
    NULL);

    n = 0;
    XtSetArg(args[n], XmNvisualPolicy, XmCONSTANT); n++;
    XtSetArg(args[n], XmNscrollingPolicy, XmAUTOMATIC); n++;
    XtSetArg(args[n], XmNshadowThickness, 0); n++;
    XtSetArg(args[n], XmNscrollBarDisplayPolicy, XmAS_NEEDED); n++;
    Widget sw = XmCreateScrolledWindow(parent, "pageListSW", args, n);
    XtManageChild (sw);
    n = 0;
    XtSetArg(args[n], XmNshadowThickness, 0); n++;
    XtSetArg(args[n], XmNhighlightThickness, 0); n++;
    this->popupList = XmCreateList (sw, "pageList", args, n);
    XtManageChild (this->popupList);
    XtAddCallback (this->popupList, XmNsingleSelectionCallback,
	(XtCallbackProc)PageSelector_SelectCB, (XtPointer)this);
    XtAddCallback (this->popupList, XmNbrowseSelectionCallback,
	(XtCallbackProc)PageSelector_SelectCB, (XtPointer)this);
    XtAddCallback (this->popupList, XmNdefaultActionCallback,
	(XtCallbackProc)PageSelector_SelectCB, (XtPointer)this);
    XtAddEventHandler(this->popupList, ButtonReleaseMask, False, (XtEventHandler)
	PageSelector_RemoveGrabEH, (XtPointer)this);
    XtAddEventHandler(this->popupList, EnterWindowMask, False, (XtEventHandler)
	PageSelector_ProcessOldEventEH, (XtPointer)this);
    XtVaGetValues (sw, 
	XmNverticalScrollBar, &this->vsb,
	XmNhorizontalScrollBar, &this->hsb,
    NULL);
}

//
// Put an entry into the scrolled list for each entry in the dictionary.
//
void PageSelector::updateList()
{
    XmFontList xmf; 
    Dimension maxWidth = 0;

#if defined(aviion)
    if (this->popupMenu == NUL(Widget)) 
	this->buildPageMenu();
#endif

    //
    // Clear the list
    //
    XmListDeleteAllItems(this->popupList);

    if (!this->page_buttons) return ;
    if (!this->page_buttons->getSize()) return ;

    //
    // Fill in the list with the contents of our PageTabs
    //
    XtVaGetValues (this->popupList, XmNfontList, &xmf, NULL);
    XmString* strTable = new XmString[this->page_buttons->getSize()];
    int next = 0;
    ListIterator it(*this->page_buttons);
    PageTab* pt;
    int select_this_item = 0;
    while ( (pt = (PageTab*)it.getNext()) ) {
	const char* name = pt->getGroupName();
	if (!name) continue;
	strTable[next] = XmStringCreateLtoR ((char*)name, "small_bold");
        Dimension strWidth = XmStringWidth(xmf, strTable[next]);
	maxWidth = (maxWidth>strWidth?maxWidth:strWidth);
	next++;
	if (pt->getState() == TRUE) select_this_item = next;
    }
    maxWidth = (maxWidth>30?maxWidth:30);

    XtVaSetValues(this->popupList, XmNvisibleItemCount, next, NULL);
    int numDisp = next<=MAX_VISIBLE?next:MAX_VISIBLE;

   // If fixing the scroll lists width, we must also set its
   // height. This means finding out how tall 1 line is and multiplying
   // it by the number of items.

    Dimension swmarginH, swshadowT;
    XtVaGetValues (XtParent(this->popupList), XmNmarginHeight, &swmarginH,
		XmNshadowThickness, &swshadowT, NULL);

    Dimension fh = XmStringHeight (xmf, strTable[0]);

    XtVaSetValues (XtParent(XtParent(this->popupList)),
	XmNheight, ((fh+2)*numDisp)+(2*(swmarginH+swshadowT)), 
	NULL);

    // Now set the width -- may need room for the scrollbar.
    if(next <= MAX_VISIBLE)
	XtVaSetValues (XtParent(XtParent(this->popupList)),
		XmNwidth, maxWidth,
		NULL);
    else
	XtVaSetValues (XtParent(XtParent(this->popupList)),
		XmNwidth, maxWidth + 18, 
		NULL);


    //
    // Show the page as selected in the list and make sure that position is showing.
    //
    XmListAddItemsUnselected (this->popupList, strTable, next, 1);
    XmListSelectPos (this->popupList, select_this_item, False);

    if(select_this_item >= MAX_VISIBLE) {
	int bofs = next - select_this_item;
	if(bofs>MAX_VISIBLE/2) bofs = MAX_VISIBLE/2; 
	XmListSetBottomPos(this->popupList, select_this_item+bofs); 
    }

    int i;
    for (i=0; i<next; i++)
	XmStringFree (strTable[i]);
    delete strTable;
}

void PageSelector::ungrab()
{
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
void PageSelector::grab(XEvent* e)
{
    if (!PageSelector::GrabCursor) {
	PageSelector::GrabCursor = 
	    XCreateFontCursor (XtDisplay(this->popupMenu), XC_arrow);
    }

    XtPopup (this->popupMenu, XtGrabNone);
    XtAppContext apcxt = theApplication->getApplicationContext();
    this->action_hook = XtAppAddActionHook (apcxt, (XtActionHookProc)
	PageSelector_PopupListAH, (XtPointer)this);
    XtGrabPointer (this->popupList, True, 
	ButtonPressMask | ButtonReleaseMask , GrabModeAsync, 
	GrabModeAsync, None, PageSelector::GrabCursor, e->xbutton.time);

    this->is_button_release_grabbed = FALSE;
    if (this->button_release_timer)
	XtRemoveTimeOut (this->button_release_timer);

    this->button_release_timer = XtAppAddTimeOut (apcxt, 500, (XtTimerCallbackProc)
	PageSelector_GrabReleaseTO, (XtPointer)this);

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

//
// the initial workspace should correspond to a PageTab whose GroupRecord
// has a showing flag turned on.
//
EditorWorkSpace* PageSelector::getInitialWorkSpace()
{
    ListIterator it(*this->page_buttons);
    PageTab* pt;
    while ( (pt = (PageTab*)it.getNext()) ) {
	if (pt->getDesiredShowing())
	    return (EditorWorkSpace*)pt->getWorkSpace();
    }
    if (this->getSize())
	return (EditorWorkSpace*)this->getDefinition(1);

    return NUL(EditorWorkSpace*);
}

List* PageSelector::getSortedPages()
{
    List *l = new List;

    ListIterator it(*this->page_buttons);
    PageTab* pt;
    while ( (pt = (PageTab*)it.getNext()) ) {
	l->appendElement((void*)pt->getWorkSpace());
    }
    return l;
}

void PageSelector::togglePage(PageTab* pt)
{
    if (pt->getState()) this->selectPage(pt);
}

extern "C"  {

void 
PageSelector_ResizeHandlerEH(Widget, XtPointer clientData, XEvent*, Boolean* keep_going)
{
    *keep_going = True;
    PageSelector* psel = (PageSelector*)clientData;
    psel->resizeCallback();
}

void PageSelector_SelectCB(Widget , XtPointer clientData, XtPointer cbs)
{
    PageSelector* psel = (PageSelector*)clientData;
    ASSERT(psel);

    psel->ungrab();

    //
    // Error check hacking...  If the mouse event was outside the list widget,
    // then ignore it.  
    //
    XmListCallbackStruct *lcs = (XmListCallbackStruct*)cbs;
    XEvent* xev = lcs->event;
    boolean inside = TRUE;
    if ((xev->type == ButtonPress) || (xev->type == ButtonRelease)) {
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
	if ((pos >= 1) && (pos <= psel->page_buttons->getSize())) {
	    PageTab* pt = (PageTab*)psel->page_buttons->getElement(lcs->item_position);
	    if (pt) {
		EditorWorkSpace* ews = (EditorWorkSpace*)pt->getWorkSpace();
		psel->selectPage(ews);
		if (pt->isManaged() == FALSE)
		    psel->resizeCallback();
	    }
	}
    }
}

Boolean PageSelector_PostNamePromptWP (XtPointer clientData)
{
    PageSelector* psel = (PageSelector*)clientData;
    ASSERT(psel);
    psel->postPageNamePrompt();
    return True;
}

void PageSelector_GrabReleaseTO(XtPointer clientData, XtIntervalId* )
{
    PageSelector* psel = (PageSelector*)clientData;
    ASSERT(psel);
    psel->is_button_release_grabbed = TRUE;
    psel->button_release_timer = 0;
}

void PageSelector_CommitNameChangeCB (Widget w, XtPointer clientData, XtPointer)
{
    PageSelector* psel = (PageSelector*)clientData;
    ASSERT(psel);
    EditorWorkSpace* ews = (EditorWorkSpace*)psel->name_change_in_progress;
    char *cp = PageSelector::GetTextWidgetToken(w);
    // remove trailing blanks
    char errMsg[256];
    if (psel->verifyPageName(cp, errMsg)) {
	psel->changePageName (ews, cp);
	psel->hidePageNamePrompt();
    } else {
	//
	// You must take the text widget off the screen before calling ErrorMessage
	// because the text widget has a grab which will be broken by ErrorMessage.
	//
	psel->hidePageNamePrompt();
	ErrorMessage (errMsg);
    }
    if (cp) delete cp ;
}

void PageSelector_ModifyNameCB (Widget w, XtPointer clientData, XtPointer cbs)
{
    PageSelector* psel = (PageSelector*)clientData;
    ASSERT(psel);
    XmTextVerifyCallbackStruct *tvcs = (XmTextVerifyCallbackStruct*)cbs;
    boolean good_chars = TRUE;
    XmTextBlock tbrec = tvcs->text;
    int i;
    for (i=0; i<tbrec->length; i++) {
	if ((tbrec->ptr[i] != '_') && 
	    (tbrec->ptr[i] != ' ') && 
	    (!isalnum((int)tbrec->ptr[i]))) {
	    good_chars = FALSE;
	    break;
	} else if ((i == 0) && (tvcs->startPos == 0) && (tbrec->ptr[i] == ' ')) {
	    good_chars = FALSE;
	    break;
	}
    }
    tvcs->doit = (Boolean)good_chars;
}

void PageSelector_UndrawTO(XtPointer clientData, XtIntervalId* )
{
    Widget w = (Widget)clientData;
    XtVaSetValues (w, XmNmappedWhenManaged, False, NULL);
}

void PageSelector_RedrawTO(XtPointer clientData, XtIntervalId* )
{
    Widget w = (Widget)clientData;
    XtVaSetValues (w, XmNmappedWhenManaged, True, NULL);
}

void PageSelector_ProcessOldEventEH(Widget , XtPointer clientData, XEvent *e, Boolean *)
{
    PageSelector* psel = (PageSelector*)clientData;
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

void PageSelector_RemoveGrabEH(Widget w, XtPointer clientData, XEvent *, Boolean *)
{
    PageSelector* psel = (PageSelector*)clientData;
    ASSERT(psel);
    if (psel->is_button_release_grabbed)
	psel->ungrab();
    if (psel->button_release_timer) {
	XtRemoveTimeOut (psel->button_release_timer);
	psel->button_release_timer = 0;
    }
    
    psel->is_button_release_grabbed = TRUE;
}
void PageSelector_EllipsisEH(Widget , XtPointer clientData, XEvent *e, Boolean *)
{
    PageSelector* psel = (PageSelector*)clientData;
    ASSERT(psel);

    if(psel->is_grabbed) {
	psel->ungrab();
	return;
    }

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

    psel->grab(e);
}

//
// If the widget of the event is not the popuplist or one of its scrollbars,
// then popdown the list.
//
void PageSelector_PopupListAH (Widget w, XtPointer clientData, String, XEvent* xev,
String*, Cardinal*)
{
    PageSelector* psel = (PageSelector*)clientData;
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
		PageSelector_RemoveHookWP, (XtPointer)psel);
	}
    }
}

Boolean PageSelector_RemoveHookWP (XtPointer clientData)
{
    PageSelector* psel = (PageSelector*)clientData;
    ASSERT(psel);
    if (psel->action_hook)
	XtRemoveActionHook (psel->action_hook);
    psel->action_hook = 0;
    psel->remove_hook_wpid = NUL(XtWorkProcId);
    return TRUE;
}

} // extern "C"



//       D I C T I O N A R Y        W O R K
//       D I C T I O N A R Y        W O R K
//       D I C T I O N A R Y        W O R K
//       D I C T I O N A R Y        W O R K
//       D I C T I O N A R Y        W O R K
//
// We won't intervene in dictionary operations in any way, we just use
// these as a means of ensuring that the selector is always in sync with
// the dictionary.
//
boolean PageSelector::addDefinition(const char *name, const void *definition)
{
    if (!this->Dictionary::addDefinition(name, definition))
	return FALSE;
    this->addButton(name, definition);
    return TRUE;
}
boolean PageSelector::addDefinition(Symbol key, const void *definition)
{
    if (!this->Dictionary::addDefinition(key, definition))
	return FALSE;
    SymbolManager* sm = this->getSymbolManager();
    this->addButton(sm->getSymbolString(key), definition);
    return TRUE;
}
void *PageSelector::removeDefinition(Symbol findkey)
{
    void* retval = this->Dictionary::removeDefinition(findkey);
    SymbolManager* sm = this->getSymbolManager();
    this->removeButton(sm->getSymbolString(findkey));
    if ((EditorWorkSpace*)retval == this->root)
	this->num_pages_when_empty = 0;
    return retval;
}
void *PageSelector::removeDefinition(const void * def)
{
    EditorWorkSpace* ews;
    EditorWorkSpace* remove_ews = (EditorWorkSpace*) def;
    int i,dsize = this->getSize();
    boolean found = FALSE;
    for (i=1; i<=dsize; i++) {
	ews = (EditorWorkSpace*)this->getDefinition(i);
	if (ews == remove_ews) {
	    found = TRUE;
	    break;
	}
    }
    if (found) {
	Symbol findkey = this->getSymbol(i);
	this->removeDefinition(findkey);
	if (remove_ews == this->root) 
	    this->num_pages_when_empty = 0;
	return (void*)def;
    } else
	return NUL(void*);
}

//
// Get the text from a text widget and clip off the leading and
// trailing white space.
// The return string must be deleted by the caller.
//
char *PageSelector::GetTextWidgetToken(Widget textWidget)
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

Dialog* PageSelector::getMoveNodesDialog()
{
    return this->move_dialog;
}

extern "C" {
void PageSelector_UngrabAP (Widget w, XEvent*, String*, Cardinal* )
{
    PageSelector* psel;
    XtVaGetValues (w, XmNuserData, &psel, NULL);
    ASSERT(psel);
    psel->hidePageNamePrompt();
}

void PageSelector_KeepingAP (Widget w, XEvent*, String*, Cardinal* )
{
    PageSelector* psel;
    XtVaGetValues (w, XmNuserData, &psel, NULL);
    ASSERT(psel);
    psel->mouse_inside_name_prompt = TRUE;
}

void PageSelector_LosingAP (Widget w, XEvent*, String*, Cardinal* )
{
    PageSelector* psel;
    XtVaGetValues (w, XmNuserData, &psel, NULL);
    ASSERT(psel);
    psel->mouse_inside_name_prompt = FALSE;
}

void PageSelector_MonitoringAP (Widget w, XEvent* xev, String*, Cardinal* )
{
    PageSelector* psel;
    XtVaGetValues (w, XmNuserData, &psel, NULL);
    ASSERT(psel);
    if (psel->mouse_inside_name_prompt == FALSE)
	psel->hidePageNamePrompt();
}

}

