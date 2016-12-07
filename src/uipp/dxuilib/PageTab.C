/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>



#include "PageTab.h"
#include "PageGroupManager.h"
#include "Application.h"
#include "PageSelector.h"
#include "EditorWindow.h"

//
// In order to use EditorWorkSpace::DXDropSite methods
//
#include "EditorWorkSpace.h"

String PageTab::DefaultResources[] = {
    ".recomputeSize:	False",
    ".topOffset:	0",
    ".leftOffset:	0",
    ".bottomOffset:	0",
    NUL(char*)
};

// gethostname is needed by decodeDropType which is part of drag-n-drop
#if defined(NEEDS_GETHOSTNAME_DECL)
extern "C" int gethostname(char *address, int address_len);
#endif

Pixmap PageTab::AnimationPixmap = NUL(Pixmap);
Pixmap PageTab::AnimationMaskPixmap = NUL(Pixmap);
boolean PageTab::ClassInitialized = FALSE;
Dictionary* PageTab::DropTypeDictionary = new Dictionary;
Dictionary* PageTab::DragTypeDictionary = new Dictionary;
Widget PageTab::DragIcon = NUL(Widget);

#include "pagedrag.bm"
#include "pagedragmask.bm"
#include "animation.bm"
#include "anim_mask.bm"

#define DXPAGETRASH "DXPAGETRASH"
#define DXPAGENAME  "DXPAGENAME"
#define DXMODULES "DXMODULES"

PageTab::PageTab(PageSelector* sel, WorkSpace* ws, const char* group) :
    NotebookTab(group)
{
    this->workSpace = ws;
    this->selector = sel;
    this->set = FALSE;
    this->group_rec = NUL(PageGroupRecord*);
    this->position = 0;
    this->has_desired_position = FALSE;
    this->color_timer = 0;
    this->group_name = DuplicateString(group);
}

int PageTab::getDesiredPosition()
{
    ASSERT(this->hasDesiredPosition());
    return this->desired_position;
}

void PageTab::activate()
{
    this->NotebookTab::activate();
    this->selector->togglePage(this);
}

void PageTab::createButton (Widget p, PageGroupRecord* prec)
{
    this->has_desired_position = TRUE;
    this->desired_position = prec->order_in_list;
    this->createButton(p);
    this->setGroup(prec);
}
void PageTab::createButton (Widget p)
{
    this->NotebookTab::createButton(p);
    if (!PageTab::ClassInitialized) {
	this->setDefaultResources (theApplication->getRootWidget(),
	    PageTab::DefaultResources);
        this->DXDropSite::addSupportedType (PageTab::PageName, DXPAGENAME, TRUE);

	this->DXDropSite::addSupportedType (PageTab::Modules, DXMODULES, TRUE);
        this->DragSource::addSupportedType (PageTab::PageName, DXPAGENAME, TRUE);

        PageTab::DragIcon = this->createDragIcon(pagedrag_width, pagedrag_height,
	     (char *)pagedrag_bits, (char *)pagedragmask_bits);
	PageTab::AnimationPixmap = XCreateBitmapFromData(XtDisplay(p),
	      XtWindow(p), animation_bits, animation_width, animation_height);
	      PageTab::AnimationMaskPixmap = XCreateBitmapFromData(XtDisplay(p),
	      XtWindow(p), anim_mask_bits, anim_mask_width, anim_mask_height);
	PageTab::ClassInitialized = TRUE;
    }

    Widget w = this->getRootWidget();

    this->setDragWidget(w);
    this->setDragIcon(PageTab::DragIcon);
    this->setIntraShellBehavior(TRUE);
}

PageTab::~PageTab()
{
    if (this->group_name) delete this->group_name;
    if (this->color_timer) XtRemoveTimeOut(this->color_timer);
}

void PageTab::setState(boolean value)
{
    this->NotebookTab::setState(value);
    if (this->group_rec) this->group_rec->showing = value;
}

void PageTab::setGroup(PageGroupRecord* rec)
{
    if (this->group_name) delete this->group_name;
    this->group_rec = rec;
    const char* rec_name = rec->getName();
    this->group_name = DuplicateString(rec_name);
    this->setLabel(this->group_name);
    this->setPosition (this->position);
}

void PageTab::setPosition (int order, boolean designated_by_user)
{
    this->position = order;

    if (designated_by_user) this->has_desired_position = FALSE;
    if (this->group_rec) this->group_rec->order_in_list = order;
}

int PageTab::getPosition ()
{
    if (this->group_rec) 
	this->position = this->group_rec->order_in_list;
    return this->position;
}

//
// It might be nice to set a timer to set the color because we're really going
// to just walk thru the net turning things on, then off, then moving to the
// next node.  If you're turning lots of nodes in 1 page on/off, then we would
// save some time by not changing the color of the page's tab.
//
void PageTab::setColor (Pixel fg)
{
    if (this->color_timer)
	XtRemoveTimeOut(this->color_timer);

    XtAppContext apcxt = theApplication->getApplicationContext();
    this->pending_fg = fg;
    this->color_timer = 
	XtAppAddTimeOut (apcxt, 100, (XtTimerCallbackProc)
		PageTab_ColorTimerTO, (XtPointer)this);
}

//
// Does the saved info for this group say the page should be visible?
// This is used to put up the same page which was showing when the net
// was saved.
//
boolean PageTab::getDesiredShowing() 
{
    return (this->group_rec?this->group_rec->showing:FALSE);
}

extern "C"
void PageTab_ColorTimerTO (XtPointer clientData, XtIntervalId* )
{
    PageTab* pt = (PageTab*)clientData;
    ASSERT(pt);
    XtVaSetValues (pt->getRootWidget(), XmNforeground, pt->pending_fg, NULL);
    pt->setDirty(TRUE,TRUE);
    pt->color_timer = 0;
}

// Called to consumate a dnd operation
boolean PageTab::decodeDropType 
(int tag, char *type, XtPointer value, unsigned long len, int, int)
{
boolean retVal;
int x,y;
int width,height;
EditorWorkSpace* ews = (EditorWorkSpace*)this->getWorkSpace();

    switch (tag) {

	case PageTab::Modules:

	    //
	    // Put the new nodes below whatever net we already have.
	    //
	    ews->getMaxWidthHeight (&width, &height);
	    if ((height > width) && (width < 400)) {
		y = 0;
		x = width + 10;
	    } else {
		x = 0;
		y = height + 10;
	    }
	    retVal = this->transfer(type, value, len, x,y);
	    break;

	case PageTab::PageName:
	    //
	    // A page name is held in *value.  Notify the PageSelector to move
	    // the corresponding tab right after me.
	    //
	    retVal = this->selector->changeOrdering (this, (char*)value);
	    break;
	default:
	    retVal = FALSE;
	    break;
    }
    return retVal;
}

boolean PageTab::decodeDragType (int tag,
	char *a, XtPointer *value, unsigned long *length, long operation)
{
boolean retVal;

    if (this->isIntraShell() == FALSE) 
	return FALSE;

    const char* gname = this->getGroupName();
    char* cp = DuplicateString(gname);

    switch(tag) {

#if 0
	case PageTab::PageTrash:
	    gname = this->getGroupName();
	    if (!gname) 
		retVal = FALSE;
	    else if (EqualString(gname, "Untitled"))
		retVal = FALSE;
	    else {
		*value = (XtPointer)cp;
		*length = strlen(cp);
		retVal = TRUE;
	    }
	    break;
#endif
	case PageTab::PageName:
	    //
	    // get some space and put a string there.  The string says the name
	    // of the button which is being moved.  The drop site will move me.
	    //
	    *value = (XtPointer)cp;
	    *length = strlen(cp);
	    retVal = TRUE;
	    break;

	default:
	    retVal = FALSE;
	    break;
    }
    return retVal;
}

#if 0
void PageTab::dropFinish(long operation, int tag, unsigned char status)
{
    if (tag == PageTab::PageTrash) {
	const char* gname = this->getGroupName();
	if (gname)
	    this->selector->requestPageDeletion (gname);
    }
}
#endif


void PageTab::manage()
{
    if (this->isManaged()) return ;
    this->setDropWidget(this->getRootWidget(), XmDROP_SITE_SIMPLE,
	PageTab::AnimationPixmap, PageTab::AnimationMaskPixmap);
    this->UIComponent::manage();
}

void PageTab::unmanage()
{
    this->UIComponent::unmanage();
    XmDropSiteUnregister (this->getRootWidget());
}

//
// A drop has deposited a network for placement in the workspace corresponding
// to this tab.  We already set x,y to lead us to empty space in the page.
// In order to get the drop to hit our page, we must remove its page group info
// and replace it with our own.
//
boolean PageTab::mergeNetElements (Network* tmpnet, List* tmppanels, int x, int y)
{
    EditorWorkSpace* ews = (EditorWorkSpace*)this->getWorkSpace();
    EditorWindow* ew = this->selector->getEditor();
    ASSERT(ew);
    if (!ew->pagifyNetNodes (tmpnet, ews)) return FALSE;
    boolean retVal = ews->mergeNetElements (tmpnet, tmppanels, x, y);
    if (retVal) ews->setMembersInitialized(FALSE);
    return retVal;
}

void PageTab::multiClick()
{
    this->selector->postPageNamePrompt(this);
}

