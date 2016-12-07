/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#include "WorkSpace.h"
#include "WorkSpaceRoot.h" 
#include "WorkSpaceInfo.h" 
#include "ListIterator.h" 
#include "SymbolManager.h"


#include <Xm/Xm.h>
#include "WorkspaceW.h"
#include <Xm/ScrolledW.h>


//
//  This routine is called when the scrolled window is resized.  It tries
//  to keep the Workspace the same size as the scrolled window.  If the
//  ScrolledWindow is displaying ScrollBars (we can detect this by looking
//  at the position of the ScrollBars), then we take the ScrollBar
//  dimensions into account when resizing the Workspace widget.  This routine
//  is also called from WorkSpaceInfo::setDefaultConfiguration (so we can
//  get rid of the ScrollBars when the user does a "New"). 
//
void WorkSpaceRoot::resizeWorkSpace()
{
    Dimension width, height;
    Dimension sw_width, sw_height;
    Dimension vsb_width, hsb_height;
    Widget wid, vsb, hsb;
    int max_w, max_h;

    Widget widget = XtParent(XtParent(this->getWorkSpaceWidget()));

    int page = this->getCurrentPage();
    if (page) {
	WorkSpace* ws = this->getElement(page);
	XmWorkspaceGetMaxWidthHeight(ws->getRootWidget(), &max_w, &max_h);
    } else {
	XmWorkspaceGetMaxWidthHeight(this->getWorkSpaceWidget(), &max_w, &max_h);
    }


    vsb_width = 0;
    hsb_height = 0;
    if(XmIsScrolledWindow(widget)) {
	Position x, y;

	XtVaGetValues(widget,
		      XmNhorizontalScrollBar, &hsb,
		      XmNverticalScrollBar, &vsb,
		      NULL);
	XtVaGetValues(widget, XmNwidth, &sw_width, XmNheight, &sw_height, NULL);

	//
	// Vertical ScrollBar
	//
	if (XtIsManaged(vsb)) {
	    XtVaGetValues(vsb, XmNx, &x, XmNy, &y, NULL);
	    if ((x < sw_width) && (y < sw_height))
		XtVaGetValues(vsb, XmNwidth, &vsb_width, NULL);
	}
	
	//
	// Horizontal ScrollBar
	//
	if (XtIsManaged(hsb)) {
	    XtVaGetValues(hsb, XmNx, &x, XmNy, &y, NULL);
	    if ((x < sw_width) && (y < sw_height))
		XtVaGetValues(hsb, XmNheight, &hsb_height, NULL);
	}

	XtVaGetValues(widget, XmNclipWindow, &wid, NULL);
	
	XtVaGetValues(wid, XmNwidth, &width, XmNheight,&height, NULL);

	width += vsb_width;
	height += hsb_height;
	int mw = MAX(width, max_w);
	int mh = MAX(height, max_h);
	this->setDimensions(mw,mh);

	if ((this->current_page) && (this->current_page != UNDETERMINED_PAGE)) {
	    WorkSpace *ws = this->getElement(this->current_page);
	    ws->setWidthHeight(mw,mh);
	}
    } 
}


WorkSpace*
WorkSpaceRoot::addPage()
{
    int max_w, max_h;
    this->getObjectXYSize (&max_w, &max_h);
    WorkSpace *ws = this->newPage(max_w,max_h);
    this->page_list.appendElement ((const void*)ws);
    ws->setPlacementCount(this->getPlacementCount());

    WorkSpaceInfo *wsinfo = this->getWorkSpaceInfo();
    unsigned int x_align = wsinfo->getGridXAlignment();
    unsigned int y_align = wsinfo->getGridYAlignment();
    int w,h;
    wsinfo->getGridSpacing(w, h);

    WorkSpaceInfo *newinfo = ws->getInfo();
    newinfo->setGridAlignment (x_align, y_align);
    newinfo->setGridSpacing(w, h);
    newinfo->setGridActive(wsinfo->isGridActive());
    newinfo->setPreventOverlap(wsinfo->getPreventOverlap());
    ws->installInfo(NUL(WorkSpaceInfo*));

    return ws;
}

//
// Maintain the current_page index by getting the old position, then checking
// for existing after removal from the list.  If we're deleting other than the
// current_page, then the index should decrement by 1 or remain unchanged.
//
void WorkSpaceRoot::removePage (const void* ws)
{
    const void* curws = NUL(void*);
    if ((this->current_page) && (this->current_page != UNDETERMINED_PAGE)) {
	curws = this->page_list.getElement(this->current_page);
	ASSERT(curws);
    }

    ASSERT(this->page_list.removeElement(ws));

    //
    // If the WorkSpace which used to be current is still in the list, 
    // then its index in the list is the new current_page.
    //
    if (curws) {
	int new_cur_pos = this->page_list.getPosition(curws);
	if (new_cur_pos) {
	    this->current_page = new_cur_pos;
	    ASSERT (curws != ws);
	} else {
	    this->current_page = UNDETERMINED_PAGE;
	    ASSERT (curws == ws);
	}
    }
}

//
//
void WorkSpaceRoot::showWorkSpace(int page)
{
    if (page == 0) {
	this->showRoot();
	this->current_page = 0;
	return ;
    }

    WorkSpace *manage_this = NUL(WorkSpace*);
    WorkSpace *unmanage_this = NUL(WorkSpace*);

    ASSERT(page);

    //
    // By turning off mappedWhenManaged for children of the root workspace widget,
    // we make sure that nothing can ever peek around the edge of a page in the
    // event that our size calculations are off by a couple of pixels.
    //
    if (this->current_page == 0) {
	Widget *kids;
	int nkids,i;
	Widget wsw = this->getWorkSpaceWidget();
	XtVaGetValues (wsw, XmNchildren, &kids, XmNnumChildren, &nkids, NULL);
	for (i=0; i<nkids; i++) {
	    Widget w = kids[i];
	    if (XtClass(w) == xmWorkspaceWidgetClass) continue;
	    XtVaSetValues (w, XmNmappedWhenManaged, False, NULL);
	}
    }

    if (page == this->current_page) return ;
    if ((this->current_page) && (this->current_page != UNDETERMINED_PAGE))
	unmanage_this = this->getElement(this->current_page);
    manage_this = this->getElement(page);

    if (manage_this) {
	Dimension w,h;
	if (this->getWorkSpaceWidget()) {
	    XtVaSetValues(this->getWorkSpaceWidget(),
		XmNlineDrawingEnabled, False,
		XmNallowOverlap, True,
	    NULL);
	    XtVaGetValues (this->getWorkSpaceWidget(),
		XmNwidth, &w, XmNheight, &h, NULL);
	}
	manage_this->manage();
	Widget wsr = manage_this->getRootWidget();
	if (wsr) {
	    Dimension old_w, old_h;
	    /*
	    XtVaSetValues(wsr, 
		XmNlineDrawingEnabled, manage_this->getLineDrawing(),
		XmNallowOverlap, manage_this->getOverlap(),
	    NULL);
	    */
	    XtVaGetValues (wsr, XmNwidth, &old_w, XmNheight, &old_h, NULL);

	    if ((old_w < w) && (old_h < h))
		XtVaSetValues (wsr, XmNwidth, old_w, XmNheight, old_h, NULL);
	    else if (old_w < w)
		XtVaSetValues (wsr, XmNwidth, old_w, NULL);
	    else if (old_h < h)
		XtVaSetValues (wsr, XmNheight, old_h, NULL);

	    XMapRaised (XtDisplay(wsr), XtWindow(wsr));
	}
	XSync(XtDisplay(this->getWorkSpaceWidget()), False);
    }

    //
    // Found one to take down from the screen.
    //
    if (unmanage_this) {
	unmanage_this->unmanage();
    }

    this->current_page = page;
}

void WorkSpaceRoot::showWorkSpace(WorkSpace* page)
{
    int pos = this->page_list.getPosition((void*)page);
    this->showWorkSpace(pos);
}

void WorkSpaceRoot::showRoot()
{
    if ((this->current_page) && (this->current_page != UNDETERMINED_PAGE)) {
	WorkSpace* ws = this->getElement(this->current_page);
	ASSERT(ws);
	ws->unmanage();
    }
    Widget wsw = this->getWorkSpaceWidget();
    ASSERT(wsw);
    if (this->getPlacementCount() == 0) {
	XtVaSetValues(wsw,
	    XmNlineDrawingEnabled, this->getLineDrawingEnabled(),
	    XmNallowOverlap, this->getOverlapEnabled(),
	NULL);
    }

    this->current_page = 0;

    //
    // By turning off mappedWhenManaged for children of the root workspace widget,
    // we make sure that nothing can ever peek around the edge of a page in the
    // event that our size calculations are off by a couple of pixels.
    //
    Widget *kids;
    int nkids,i;
    XtVaGetValues (wsw, XmNchildren, &kids, XmNnumChildren, &nkids, NULL);
    for (i=0; i<nkids; i++) {
	Widget w = kids[i];
	if (XtClass(w) == xmWorkspaceWidgetClass) continue;
	XtVaSetValues (w, XmNmappedWhenManaged, True, NULL);
    }
}

//
// Determine if this WorkSpace is a WorkSpace of the given class
//
boolean WorkSpaceRoot::isA(const char *classname)
{
    Symbol s = theSymbolManager->registerSymbol(classname);
    return this->isA(s);
}
//
// Determine if this WorkSpace is of the given class.
//
boolean WorkSpaceRoot::isA(Symbol classname)
{
    Symbol s = theSymbolManager->registerSymbol(ClassWorkSpaceRoot);
    return (s == classname);
}


//
// P E R F O R M    T H E    S A M E    O P E R A T I O N    O N    P A G E S
// P E R F O R M    T H E    S A M E    O P E R A T I O N    O N    P A G E S
// P E R F O R M    T H E    S A M E    O P E R A T I O N    O N    P A G E S
// P E R F O R M    T H E    S A M E    O P E R A T I O N    O N    P A G E S
// P E R F O R M    T H E    S A M E    O P E R A T I O N    O N    P A G E S
// P E R F O R M    T H E    S A M E    O P E R A T I O N    O N    P A G E S
//
WorkSpaceRoot::~WorkSpaceRoot()
{
    WorkSpace *ws;
    ListIterator it(this->page_list);
    while ( (ws = (WorkSpace*)it.getNext()) )
	delete ws;
}
void WorkSpaceRoot::setPageCursor (int cursorType)
{
    WorkSpace *ws;
    ListIterator it(this->page_list);
    while ( (ws = (WorkSpace*)it.getNext()) )
	ws->setCursor(cursorType);
}
void WorkSpaceRoot::resetPageCursor()
{
    WorkSpace *ws;
    ListIterator it(this->page_list);
    while ( (ws = (WorkSpace*)it.getNext()) )
	ws->resetCursor();
}
void  WorkSpaceRoot::installPageInfo (WorkSpaceInfo* new_info)
{
int w,h;

    WorkSpaceInfo* info = (new_info?new_info: this->getWorkSpaceInfo());

    unsigned int x_align = info->getGridXAlignment();
    unsigned int y_align = info->getGridYAlignment();
    boolean is_active = info->isGridActive();
    boolean prevent_overlap = info->getPreventOverlap();
    info->getGridSpacing(w,h);

    ListIterator it(this->page_list);
    WorkSpace *ws = NULL;
    while  ( (ws = (WorkSpace*)it.getNext()) ) { 
	WorkSpaceInfo *page_info = ws->getInfo();
	page_info->setGridAlignment (x_align, y_align);
	page_info->setGridActive (is_active);
	page_info->setGridSpacing (w,h);
	page_info->setPreventOverlap(prevent_overlap);
	ws->installInfo(NULL);
    }
}
void WorkSpaceRoot::beginManyPagePlacements()
{
    ListIterator it(this->page_list);
    WorkSpace *ws = NULL;
    while  ( (ws = (WorkSpace*)it.getNext()) ) 
	ws->beginManyPlacements();
}
void WorkSpaceRoot::endManyPagePlacements()
{
    ListIterator it(this->page_list);
    WorkSpace *ws = NULL;
    while  ( (ws = (WorkSpace*)it.getNext()) ) 
	ws->endManyPlacements();
}


