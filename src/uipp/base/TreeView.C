/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include <defines.h>
#include <ctype.h> // for tolower()

#include <stdio.h>

#include "DXStrings.h"

#include <Xm/Xm.h>
#include <Xm/DrawingA.h>
#include <Xm/Label.h>
#include <X11/cursorfont.h>

#if !defined(XK_MISCELLANY)
#define XK_MISCELLANY
#endif
#if !defined(XK_LATIN1)
#define XK_LATIN1
#endif
#include <X11/keysym.h>

#define TREE_VIEW_PRIVATES
#include "TreeView.h"

#include "TreeNode.h"
#include "Application.h"
#include "List.h"
#include "ListIterator.h"

#include "plus.bm"
#include "minus.bm"

//
// Notes:
// 1) This class has been tested only on data that is 1 level deep and with
// a hidden root - in other words, root + 1 level of categories + 1 level of leaves.
// It ought to work for deeper nesting but I've never tried it so there 
// are probably bugs.
// 2) All outliners draw horizontal/vertical lines to show some sort of parent,
// child,sibling relationships.  The way I've done it here, is different from
// the way it's usually done but I chose this way of doing it because I didn't
// want lots of indentation.  Other outliners indent a lot more than this one does.
// 3) Type-ahead is mostly implemented in TreeView::typeAhead().  It's implemented
// at this level rather than at the tool selector level so that type-ahead can be
// available in other situations in which this object might be used.
//

//
// ToDo:
// 1) Anytime you click on an item you toggle its selection state.  It would
// be better to start a timer to toggle its selection state.  Reason: if you
// double click on an item that's already selected, you're really trying to
// double click it but what you get instead is a deselect then another select.
// 2) Motif provides a flashing ibeam in text widgets and the flash rate is
// settable.  It would be nice if the flash rate used here were the same
// as Motif's flash rate.
// 3) Keyboard focus is problematic.  We always set traversalOn to FALSE,
// not only here but elsewhere in dxui.  I don't know how to fix it.
// 4) Type-ahead has a fixed-size buffer of 32 chars.  I don't know what
// happens if the I type more than that.
//

boolean TreeView::ClassInitialized = FALSE;
static int IBeamTime = 250;

Pixmap TreeView::Plus = XmUNSPECIFIED_PIXMAP;
Pixmap TreeView::Minus = XmUNSPECIFIED_PIXMAP;
Cursor TreeView::Hand = 0;
Cursor TreeView::Typing = 0;

XmFontList TreeView::FontList=0;

#define INDENT 10
#define LEFT_MARGIN 3
#define TOP_MARGIN 3

const String TreeView::DefaultResources[] =
{
    "*shadowThickness: 0",
    NULL
};

TreeView::TreeView(Widget parent, const char* bubbleHelp) : UIComponent ("treeView")
{
    ASSERT(parent);

    if (!TreeView::ClassInitialized) {
	TreeView::ClassInitialized = TRUE;
	this->setDefaultResources (theApplication->getRootWidget(), 
	    TreeView::DefaultResources);
	TreeView::Hand = XCreateFontCursor(XtDisplay(theApplication->getRootWidget()),
	    XC_hand2);
	TreeView::Typing = XCreateFontCursor(XtDisplay(theApplication->getRootWidget()),
	    XC_right_ptr);

    }

    this->buffer = XmUNSPECIFIED_PIXMAP;
    this->cursor_backing = XmUNSPECIFIED_PIXMAP;
    this->initialized = FALSE;
    this->gc = 0;
    this->data_model = NUL(TreeNode*);
    this->selection = NUL(TreeNode*);
    this->selection = NUL(TreeNode*);
    this->containing_marker = NUL(Marker*);
    this->single_click_time = 0;
    this->multi_click_time = 500;
    this->selection_marker = NUL(Marker*);
    this->match_marker = NUL(Marker*);
    this->typing_ahead = FALSE;
    this->matched = NUL(TreeNode*);
    this->typing_count = 0;
    this->ibeam_showing = FALSE;
    this->ibeam_timer = 0;
    this->dirty = TRUE;

    Widget w = XtVaCreateWidget (name, xmDrawingAreaWidgetClass, parent, 
	XmNtraversalOn, FALSE,
    NULL);
    this->setRootWidget(w);

    Colormap colormap;
    Pixel bg;
    XtVaGetValues (w, 
	XmNtopShadowColor, &this->highlight_color, 
	XmNbottomShadowColor, &this->line_color,
	XmNforeground, &this->foreground_color,
	XmNbackground, &bg,
	XmNcolormap, &colormap,
    NULL);

#if USE_DIFFERENT_TYPE_AHEAD_COLOR
    char *color_name = "MediumPurple";
    XColor exact_def;
    XColor screen_def;
    Display *d = XtDisplay(w);
    if (XLookupColor(d, colormap, color_name, &exact_def, &screen_def)) {
	this->type_ahead_color = screen_def.pixel;
    } else {
	this->type_ahead_color = this->highlight_color;
	printf ("%s[%d] Warning: TreeView type ahead color missing\n", __FILE__,__LINE__);
    }
#else
	this->type_ahead_color = this->highlight_color;
#endif

    XtAddCallback (w, XmNexposeCallback, (XtCallbackProc)TreeView_ExposeCB, 
	(XtPointer)this);

    XtAddCallback (w, XmNinputCallback, (XtCallbackProc)TreeView_InputCB, 
	(XtPointer)this);

    XtAddCallback (w, XmNresizeCallback, (XtCallbackProc)TreeView_ResizeCB, 
	(XtPointer)this);

    XtAddEventHandler (w, EnterWindowMask|LeaveWindowMask, False, 
	(XtEventHandler)TreeView_RolloverEH, (XtPointer)this);

    XtAddEventHandler (w, PointerMotionMask, False, 
	(XtEventHandler)TreeView_MotionEH, (XtPointer)this);

    if (bubbleHelp) this->setBubbleHelp (bubbleHelp, w);
}

TreeView::~TreeView()
{
    Display* d = XtDisplay(this->getRootWidget());
    if (this->buffer!=XmUNSPECIFIED_PIXMAP) XFreePixmap(d, this->buffer);
    if (this->cursor_backing!=XmUNSPECIFIED_PIXMAP) XFreePixmap(d, this->cursor_backing);
    if (this->gc) XFreeGC(d, this->gc);
    if (this->data_model) delete this->data_model;
    this->clearMarkers();
    if (this->ibeam_timer) XtRemoveTimeOut(this->ibeam_timer);
    Widget w = this->getRootWidget();
    if (w) {
	XtRemoveCallback (w, XmNexposeCallback, (XtCallbackProc)
	    TreeView_ExposeCB, (XtPointer)this);
    }
}

//
// This method could also have been called setDataModel().  In the case
// of the tool selector, a new model is supplied every time the user 
// loads new macros or changes the set of categories.
//
void TreeView::initialize (TreeNode* model, boolean repaint)
{
    Widget w = this->getRootWidget();
    Display* d = XtDisplay(w);
    if (this->buffer != XmUNSPECIFIED_PIXMAP) XFreePixmap(d, this->buffer);
    this->buffer = XmUNSPECIFIED_PIXMAP;
    if (this->data_model) delete this->data_model;
    this->data_model = model;
    this->initialized = TRUE;
    this->clearMarkers();
    this->auto_expanded.clear();
    this->endTyping();
    this->setDirty(TRUE,repaint);
}

//
// Every displayed string has a corresponding Rectangle stored in
// this->markers.  They're used to translate a mouse click back into
// an TreeNode.
//
void TreeView::clearMarkers()
{
    ListIterator iter(this->markers);
    Marker* marker;
    while (marker = (Marker*)iter.getNext()) {
	delete marker;
    }
    this->markers.clear();
    iter.setList(this->items);
    while (marker = (Marker*)iter.getNext()) {
	delete marker;
    }
    this->items.clear();
    if (this->selection_marker) {
	delete this->selection_marker;
	this->selection_marker = NUL(Marker*);
    }
    this->containing_marker = NUL(Marker*);
}

void TreeView::endTyping()
{
    this->searchable_nodes.clear();
    this->typing_ahead = FALSE;
    if (XtWindow(this->getRootWidget()))
	this->setDefaultCursor();
    this->matched = NUL(TreeNode*);
    if (this->match_marker) delete this->match_marker;
    this->match_marker = NUL(Marker*);
    this->typing_count = 0;
    if (this->ibeam_timer) XtRemoveTimeOut(this->ibeam_timer);
    this->ibeam_timer = 0;
}

void TreeView::resize(int width, int height)
{
    XtVaSetValues (this->getRootWidget(), XmNwidth, width, XmNheight, height, NULL);
    Display* d = XtDisplay(this->getRootWidget());
    if (this->buffer != XmUNSPECIFIED_PIXMAP) XFreePixmap(d, this->buffer);
    this->buffer = XmUNSPECIFIED_PIXMAP;
}

void TreeView::paint()
{
    Widget w = this->getRootWidget();
    Display* d = XtDisplay(w);
    Window win = XtWindow(w);

    boolean selection_drawn = FALSE;

    this->clearMarkers();

    //
    // First step is to adjust the required dimensions
    //
    Dimension unused, strHeight;
    if (!TreeView::FontList) {
	// We're an XmDrawingArea widget which has no XmNfontList resource.  We
	// require an XmNfontList in order to draw text on the screen.  So I
	// obtain one by briefly instantiated a dummy widget.
	Widget lab = XtCreateWidget("foo",xmLabelWidgetClass,this->getRootWidget(), 0,0);
	XmFontList fontList;
	XtVaGetValues (lab, XmNfontList, &fontList, NULL);
	TreeView::FontList = XmFontListCopy(fontList);
	XtDestroyWidget(lab);
    }
    if (this->data_model) {
	XmString tmp = XmStringCreateLtoR("Yy", (char*)this->getFont());
	XmStringExtent (TreeView::FontList, tmp, &unused, &strHeight);
	XmStringFree(tmp);

	if (strHeight < plus_height) strHeight = plus_height + 1;
	int string_count = 0;
	Dimension actual_width = 0;
	//
	// This call to ::paintNode() does now drawing.  It't just a rehearsal.  It
	// has the effect of computing the space required for the whole display.
	// We always make a size request based on this.
	//
	this->paintNode(this->data_model, string_count, 0, (int)strHeight, 
	    actual_width, FALSE, FALSE, selection_drawn);
	int actual_height = (string_count * strHeight) + TOP_MARGIN;
	this->resize(actual_width, actual_height);
    }

    int depth;
    Dimension width, height;
    Pixel fg, bg;
    XtVaGetValues (w, 
	XmNdepth, &depth, XmNwidth, &width, 
	XmNheight, &height, XmNforeground, &fg, 
	XmNbackground, &bg,
    NULL);
    XGCValues values;
    values.foreground = fg;
    values.background = bg;

    if (this->buffer == XmUNSPECIFIED_PIXMAP) {
	this->buffer = XCreatePixmap(d, win, width, height, depth);
	if (this->gc) XFreeGC(d, this->gc);
	unsigned long mask = GCForeground | GCBackground;
	this->gc = XCreateGC(d, this->buffer, mask, &values);
    }

    if (TreeView::Plus == XmUNSPECIFIED_PIXMAP) {
	TreeView::Plus = XCreatePixmapFromBitmapData(d,win,plus_bits,plus_width,
	    plus_height,this->line_color,bg,depth);
	TreeView::Minus = XCreatePixmapFromBitmapData(d,win,minus_bits,minus_width,
	    minus_height,this->line_color,bg,depth);
    }

    values.foreground = bg;
    XChangeGC (d,this->gc,GCForeground,&values);
    XFillRectangle (d, this->buffer, this->gc, 0,0,width, height); 
    this->ibeam_showing = FALSE;
    values.foreground = fg;
    XChangeGC (d,this->gc,GCForeground,&values);

    if (this->data_model) {
	int string_count = 0;
	this->paintNode(this->data_model, string_count, 0, (int)strHeight, 
		width, TRUE, FALSE, selection_drawn);
    }

    if ((this->selection) && (!selection_drawn)) 
	this->select(NUL(TreeNode*), FALSE);
    this->setDirty(FALSE);
}

boolean TreeView::getSelectionLocation (int& x1, int& y1, int& x2, int& y2)
{
    if (!this->selection) return FALSE;
    if (!this->selection_marker) return FALSE;
    this->selection_marker->getLocation(x1,y1,x2,y2);
    return TRUE;
}

boolean TreeView::getMatchLocation (int& x1, int& y1, int& x2, int& y2)
{
    if (!this->matched) return FALSE;
    if (!this->match_marker) return FALSE;
    this->match_marker->getLocation(x1,y1,x2,y2);
    return TRUE;
}

//
// Drawing the contents of the display is a recursive operation: 
// paintNode() on a node, then for each child of that node do paintNode().
//
void TreeView::paintNode(TreeNode* node, int& string_count, int level, int strHeight, Dimension& width, boolean paint_it, boolean last_line, boolean& selection_drawn)
{
    int level_incr = 1;
    int descent = 1;
    int y = (string_count * strHeight) + TOP_MARGIN;
    int x = (level*plus_width) + LEFT_MARGIN + (level>0?(level*INDENT):0);
    Display* d = XtDisplay(this->getRootWidget());
    XGCValues values;
    if ((node->isRoot()) && (!this->isRootVisible())) {
	level_incr = 0;
    } else {
	const char* str = node->getString();
	XmString xmstr = XmStringCreateLtoR((char*)str, (char*)this->getFont());
	if (paint_it) {
	    int sx;
	    if (node->hasChildren())
		sx = x+plus_width+3;
	    else
		sx = x+5;
	    int sy = y+descent;
	    if ((this->selection == node) || (this->matched == node)) {
		if (this->selection == node)
		    values.foreground = this->highlight_color;
		else
		    values.foreground = this->type_ahead_color;
		XChangeGC (d, this->gc, GCForeground, &values);
		XFillRectangle (d, this->buffer, this->gc, sx,sy,width-sx,strHeight);
		if (this->selection == node) {
		    selection_drawn = TRUE;
		    this->selection_marker = new Marker (sx,sy,(width-sx),strHeight,node);
		}
		if (this->matched == node) {
		    this->match_marker = new Marker (sx,sy,(width-sx),strHeight,node);
		}
	    }
	    values.foreground = this->foreground_color;
	    XChangeGC (d, this->gc, GCForeground, &values);
	    XmStringDraw (d, this->buffer, TreeView::FontList, xmstr, this->gc, 
		sx,sy, width, XmALIGNMENT_BEGINNING, 
		XmSTRING_DIRECTION_L_TO_R, NUL(XRectangle*));
	    this->items.appendElement (new Marker (sx,sy,(width-sx),strHeight,node));
	} else {
	    Dimension strWidth, unused;
	    XmStringExtent (TreeView::FontList, xmstr, &strWidth, &unused);
	    int tot_width = x+plus_width+2+strWidth;
	    if (tot_width > width)
		width = tot_width;
	}
	XmStringFree(xmstr);
	string_count++;
    }

    if (node->hasChildren()) {
	List* kids = node->getChildren();
	if (node->isExpanded()) {
	    if ((node->isRoot()==FALSE)||(this->isRootVisible())) {
		if (paint_it) {
		    XCopyArea (d,TreeView::Minus,this->buffer,this->gc,0,0,
			minus_width,minus_height, x,y);
		    this->markers.appendElement(
			new Marker(x,y,minus_width,minus_height,node));
		}
	    }
	    ListIterator li(*kids);
	    TreeNode* kid;
	    int kidCnt = kids->getSize();
	    int cnt = 1;
	    while (kid = (TreeNode*)li.getNext()) {
		this->paintNode(kid, string_count, level+level_incr, strHeight, 
		    width, paint_it, (cnt==kidCnt), selection_drawn);
		cnt++;
	    }
	} else {
	    if (paint_it) {
		if ((node->isRoot()==FALSE)||(this->isRootVisible())) {
		    XCopyArea (d,TreeView::Plus,this->buffer,this->gc,0,0,
			plus_width,plus_height, x,y);
		    this->markers.appendElement(
			new Marker(x,y,plus_width,plus_height,node));
		}
	    }
	}
    } else if (paint_it) {
	int yy = y + 1 + (strHeight>>1);
	int x1 = (level>0?(level*INDENT):0) + LEFT_MARGIN + (plus_width>>1);
	int x2 = x1+1+(plus_width>>1);
	values.foreground = this->line_color;
	XChangeGC (d,this->gc,GCForeground, &values);
	XDrawLine (d, this->buffer, this->gc, x1,yy,x2,yy);
	if (last_line)
	    XDrawLine (d, this->buffer, this->gc, x1,y,x1,y+(strHeight>>1));
	else
	    XDrawLine (d, this->buffer, this->gc, x1,y,x1,y+strHeight);
    }
}

//
// called by the widget's XmNexposeCallback.  Copy from the double-buffer onto
// the screen.
//
void TreeView::redisplay()
{
    if (!XtIsRealized(this->getRootWidget())) return ;
    if (this->isDirty()) this->paint();
    Widget w = this->getRootWidget();
    Dimension width, height;
    XtVaGetValues (w, XmNwidth, &width, XmNheight, &height, NULL);
    Window win = XtWindow(w);
    Display* d = XtDisplay(w);
    XCopyArea (d, this->buffer, win, this->gc, 0, 0, width, height, 0, 0);
}

void TreeView::motion(int x, int y)
{
    boolean need_to_reset_cursor = FALSE;
    if (this->containing_marker) {
	if (this->containing_marker->contains(x,y)) 
	    return ;
	this->containing_marker = NUL(Marker*);
	need_to_reset_cursor = TRUE;
    }
    ListIterator iter(this->markers);
    Marker* marker;
    while (marker = (Marker*)iter.getNext()) {
	if (marker->contains(x,y)) {
	    this->containing_marker = marker;
	    break;
	}
    }
    if (!this->containing_marker) {
	if (need_to_reset_cursor) this->setDefaultCursor();
    } else {
	this->setHandCursor();
    }
}

void TreeView::clear(boolean repaint)
{ 
    ListIterator iter(this->auto_expanded);
    TreeNode* node;
    while (node=(TreeNode*)iter.getNext()) {
	if (node->isExpanded()) node->setExpanded(FALSE);
    }
    this->auto_expanded.clear();
    this->endTyping();
    if (this->selection) this->select(NUL(TreeNode*), FALSE); 
    this->setDirty(TRUE,repaint);
}

void TreeView::select(TreeNode* node, boolean repaint)
{
    if (this->selection == node) return ;
    this->selection = node;

    if (node) {
	TreeNode* parent = node->getParent();
	while (!parent->isRoot()) {
	    if (!parent->isExpanded()) {
		parent->setExpanded(TRUE);
		this->auto_expanded.appendElement(parent);
	    }
	    parent = parent->getParent();
	}
    }
    this->setDirty(TRUE,repaint);
    if ((this->selection) && (repaint)) {
	int x1,y1,x2,y2;
	ASSERT(this->getSelectionLocation(x1,y1,x2,y2));
	this->adjustVisibility(x1,y1,x2,y2);
    }
}

void TreeView::setDefaultCursor()
{
    Widget w = this->getRootWidget();
    XDefineCursor(XtDisplay(w), XtWindow(w), None);
}

void TreeView::setHandCursor()
{
    Widget w = this->getRootWidget();
    XDefineCursor(XtDisplay(w), XtWindow(w), TreeView::Hand);
}

void TreeView::setTypeAheadCursor()
{
    Widget w = this->getRootWidget();
    XDefineCursor(XtDisplay(w), XtWindow(w), TreeView::Typing);
}

void TreeView::setDirty(boolean d, boolean repaint)
{
    this->dirty = d;
    if (d && repaint) this->redisplay();
}

void TreeView::keyPress(XEvent *xev)
{
    if (xev->type != KeyPress) return;
    XKeyEvent* xke = (XKeyEvent*)xev;

    TreeNode* node = this->selection;
    if (!node) node = this->matched;

    KeySym lookedup = XLookupKeysym(xke, 0);
    if ((lookedup >= XK_A) && (lookedup <= XK_ydiaeresis)) {
	this->typeAhead(lookedup);
    } else if (node) {
	ListIterator iter(this->items);
	Marker* prev=NUL(Marker*);
	Marker* next=NUL(Marker*);
	Marker *marker;
	while (marker = (Marker*)iter.getNext()) {
	    if (marker->getNode() == node) {
		next = (Marker*)iter.getNext();
		break;
	    }
	    prev = marker;
	}
	switch (lookedup) {
	    case XK_space:
		if (this->isTyping()) this->endTyping();
		if (this->selection) this->select(NUL(TreeNode*), TRUE);
		this->auto_expanded.clear();
		break;
	    case XK_Up:
		if (prev) {
		    if (this->isTyping()) this->endTyping();
		    this->auto_expanded.clear();
		    this->select(prev->getNode(), TRUE);
		}
		break;
	    case XK_Down:
		if (next) {
		    if (this->isTyping()) this->endTyping();
		    this->auto_expanded.clear();
		    this->select(next->getNode(), TRUE);
		}
		break;
	    case XK_Page_Up:
		break;
	    case XK_Page_Down:
		break;
	    case XK_Tab:
		break;

	    case XK_Escape:
		if (this->isTyping()) this->clear(TRUE);
		break;

	    case XK_Left:
	    case XK_BackSpace:
	    case XK_Delete:
		if (this->isTyping()) {
		    if (this->typing_count) {
			this->typing_count--;
			this->setDirty(TRUE, TRUE);
		    }
		}
		break;

	    case XK_Return:
	    case XK_KP_Enter:
		this->auto_expanded.clear();
		if ((node->isLeaf()==FALSE) && (node->hasChildren())) {
		    node->setExpanded(node->isExpanded() == FALSE);
		    this->setDirty(TRUE,TRUE);
		} else if (this->isTyping()) {
		    TreeNode* tmp = this->matched;
		    this->endTyping();
		    this->select(tmp, TRUE);
		}
		break;
	}
    }
}

//
//
//
void TreeView::typeAhead(KeySym typed)
{
    if (!this->isTyping()) this->beginTyping();
    if (!this->isTyping()) return ;

    // There is a special cursor displayed while typing is active.  This
    // isn't really necessary but it is a nice debugging feature.  The
    // cursor is currently an arrow pointing opposite direction from normal.
    this->setTypeAheadCursor();

    //
    // The ibeam must be invisible before we do anything because
    // if it's visible when we repaint, then the next time it displays
    // it won't make backing store for the ibeam.
    //
    boolean restart_ibeam = FALSE;
    if (this->ibeam_showing) {
	if (this->ibeam_timer) {
	    XtRemoveTimeOut(this->ibeam_timer);
	    this->ibeam_timer = 0;
	    restart_ibeam = TRUE;
	}
	this->ibeam_timer = 0;
	this->toggleIBeam(FALSE);
    }
    this->typing[this->typing_count++] = (char)tolower(typed);
    this->typing[this->typing_count] = '\0';

    //
    // A list of nodes against which we'll match the typing, was formed in
    // TreeView::beginTyping().  Ordinarily it includes only the nodes
    // that are visible i.e. in categories that are expanded.
    //
    ListIterator iter(this->searchable_nodes);
    int matches = 0;

    TreeNode* match;
    TreeNode* very_next = NUL(TreeNode*);
    char* very_next_str = 0;

    TreeNode* selection_parent = NUL(TreeNode*);
    if ((this->selection) && (this->selection->isLeaf()))
	selection_parent = this->selection->getParent();

    this->matched = NUL(TreeNode*);
    while (match=(TreeNode*)iter.getNext()) {
	char* cp = DuplicateString(match->getString());
	int len = strlen(cp);
	for (int i=0;i<len;i++) cp[i] = tolower(cp[i]);
	const char* t = this->typing;
	int cnt = this->typing_count;
	int cmp = strncmp (cp, t, cnt);
	if (cmp == 0) {
	    if (matches == 0) {
		this->matched = match;
	    } else if (((match->getParent() == selection_parent) &&
			(selection_parent) &&
			(this->matched->getParent() != selection_parent))) {
		// Generally, when we're matching we want the 1st match
		// we encounter.  An exception is the case in which we find
		// multiple exact matches but one of them is in the same
		// category as the current selection.  Then we'll give
		// preference to the match that allows us to remain in
		// the same category.
		this->matched = match;
	    }
	    matches++;
	} else if ((cmp > 0) && (match->getParent()->isExpanded())) {
	    //
	    // This is what causes a-u-z to select BSpline.
	    //
	    if (!very_next) {
		very_next = match;
		very_next_str = DuplicateString(cp);
	    } else if (strcmp(cp,very_next_str)==0) {
		if ((selection_parent) && 
		    (match->getParent() == selection_parent) &&
		    (very_next->getParent() != selection_parent)) {
		    very_next = match;
		}
	    } else if (match->getParent()->isSorted()==FALSE) {
		// If the list wasn't sorted, then we have to test each string to 
		// see if it falls between t and very_next in lexical order.
		// It might be better to sort everything in
		// TreeView::getSearchableNodes(List& nodes_to_search)
		// although then we would lose the ordering based on categories.
		if (strcmp(cp,very_next_str)<0) {
		    very_next = match;
		    delete very_next_str;
		    very_next_str = DuplicateString(cp);
		}
	    }
	}
	delete cp;
    }
    if (matches == 0) {
	if (very_next) {
	    this->select(very_next);
	    this->endTyping();
	    restart_ibeam = FALSE;
	} else {
	    this->endTyping();
	    this->typeAhead(typed);
	    restart_ibeam = FALSE;
	}
    } else {
	ASSERT(this->matched);
	this->select(this->matched, TRUE);
    }
    if (restart_ibeam) {
	XtAppContext apcxt = theApplication->getApplicationContext();
	this->ibeam_timer = XtAppAddTimeOut (apcxt, IBeamTime, 
	    (XtTimerCallbackProc) TreeView_IBeamTO, (XtPointer)this);
    }
    if (very_next_str) delete very_next_str;
}

void TreeView::beginTyping()
{
    this->getSearchableNodes(this->searchable_nodes);
    this->typing_ahead = TRUE;
    this->ibeam_showing = FALSE;
    XtAppContext apcxt = theApplication->getApplicationContext();
    if (this->ibeam_timer) XtRemoveTimeOut(this->ibeam_timer);
    this->ibeam_timer = XtAppAddTimeOut (apcxt, IBeamTime, 
	(XtTimerCallbackProc) TreeView_IBeamTO, (XtPointer)this);
    this->select(0,FALSE);
}

void TreeView::buttonPress(XEvent *xev)
{
    Widget w = this->getRootWidget();

    XButtonEvent* xbe = (XButtonEvent*)xev;
    if (xbe->button != 1) return ;
    if (this->isTyping()) this->endTyping();
    this->auto_expanded.clear();

    if (this->containing_marker) {
	//
	// containing_marker is a rectangle containing a pixmap 
	// preceding a category, a plus or minus.
	//
	TreeNode* node = this->containing_marker->getNode();
	if (node->isLeaf()==FALSE) {
	    node->setExpanded(node->isExpanded() == FALSE);
	    this->setDirty(TRUE, TRUE);
	    this->motion(xbe->x, xbe->y);
	    this->single_click_time = 0;
	}
    } else {
	//
	// marker is a rectangle containing a line of text from the 
	// display.
	//
	Marker* marker = this->pick(xbe->x, xbe->y);
	if (marker) {
	    TreeNode* node = marker->getNode();
	    if (node == this->selection) {
		unsigned int diff = xbe->time - this->single_click_time;
		if (diff < this->multi_click_time) {
		    this->multiClick(node);
		    this->single_click_time = 0;
		} else {
		    if (node->hasChildren() == FALSE) {
			this->select(NUL(TreeNode*));
		    }
		    this->single_click_time = xbe->time;
		}
	    } else {
		this->select(node);
		this->single_click_time = xbe->time;
	    }
	} else {
	    this->select(NUL(TreeNode*));
	}
    }
}

Marker* TreeView::pick(int x, int y)
{
    ListIterator iter(this->items);
    Marker* marker;
    boolean found = FALSE;
    while (marker = (Marker*)iter.getNext()) {
	if (marker->contains(x,y)) 
	    return marker;
    }
    return NUL(Marker*);
}

void TreeView::multiClick(TreeNode* node)
{
    if ((node->isLeaf()==FALSE) && (node->hasChildren())) {
	node->setExpanded(node->isExpanded() == FALSE);
	this->setDirty(TRUE,TRUE);
    }
}

void TreeView::getSearchableNodes(List& nodes_to_search)
{
    nodes_to_search.clear();
    if (!this->data_model) return ;
    this->getSearchableNodes(this->data_model, nodes_to_search);
}
void TreeView::getSearchableNodes(TreeNode* subtree, List& nodes_to_search)
{
    if (subtree->isLeaf()) {
	nodes_to_search.appendElement(subtree);
    } else if ((subtree->hasChildren()) && (subtree->isExpanded())) {
	List* kids = subtree->getChildren();
	ListIterator iter(*kids);
	TreeNode* kid;
	while (kid = (TreeNode*)iter.getNext())
	    this->getSearchableNodes(kid, nodes_to_search);
    }
}

void TreeView::toggleIBeam(boolean restart)
{
    if (!this->match_marker) return ;
    ASSERT(this->matched);
    char* match_str = DuplicateString(this->matched->getString());
    match_str[this->typing_count] = '\0';
    XmString xmstr = XmStringCreateLtoR(match_str, (char*)this->getFont());
    Dimension strWidth, strHeight;
    XmStringExtent (TreeView::FontList, xmstr, &strWidth, &strHeight);
    int ibeam_width = 7;
    int ibeam_height = strHeight;
    XmStringFree(xmstr);
    int x1,x2,y1,y2;
    this->match_marker->getLocation(x1,y1,x2,y2);
    int x = x1 + strWidth;
    int y = y1;
    Display* d = XtDisplay(this->getRootWidget());
    if (!this->ibeam_showing) {
	if (this->cursor_backing == XmUNSPECIFIED_PIXMAP) {
	    Window win = XtWindow(this->getRootWidget());
	    int depth;
	    XtVaGetValues (this->getRootWidget(), XmNdepth, &depth, NULL);
	    this->cursor_backing=XCreatePixmap(d, win, ibeam_width, ibeam_height, depth); 
	}
	XCopyArea (d,this->buffer,this->cursor_backing,this->gc,x-(ibeam_width>>1),y,
	    ibeam_width,ibeam_height,0,0);
	XGCValues values;
	values.foreground = this->foreground_color;
	XChangeGC (d,this->gc,GCForeground,&values);
	XDrawLine (d, this->buffer, this->gc, x-1,y1,x-1,y2-1);
	XDrawLine (d, this->buffer, this->gc, x-3,y1,x+1,y1);
	XDrawLine (d, this->buffer, this->gc, x-3,y2-1,x+1,y2-1);
    } else {
	XCopyArea (d,this->cursor_backing,this->buffer,this->gc,0,0,
	    ibeam_width,ibeam_height, x-(ibeam_width>>1),y);
    }
    this->ibeam_showing = this->ibeam_showing==FALSE;
    this->redisplay();
    if (restart) {
	XtAppContext apcxt = theApplication->getApplicationContext();
	this->ibeam_timer = XtAppAddTimeOut (apcxt, IBeamTime, 
	    (XtTimerCallbackProc) TreeView_IBeamTO, (XtPointer)this);
    }

    delete match_str;
}

extern "C" {
void TreeView_ExposeCB(Widget w, XtPointer clientData, XtPointer callData)
{
    XmDrawingAreaCallbackStruct *cbs = (XmDrawingAreaCallbackStruct*)callData;
    ASSERT(clientData);
    TreeView* tv = (TreeView*)clientData;
    tv->redisplay();
}

void TreeView_InputCB(Widget w, XtPointer clientData, XtPointer callData)
{
    XmDrawingAreaCallbackStruct *cbs = (XmDrawingAreaCallbackStruct*)callData;
    ASSERT(clientData);
    TreeView* tv = (TreeView*)clientData;
    XEvent* xev = cbs->event;
    switch (xev->type) {
	case ButtonPress:
	    tv->buttonPress(xev);
	    break;
	case KeyPress:
	    tv->keyPress(xev);
	    break;

	//
	// AFAIK, Motif is never notifying us of focusIn,Out.  I considered
	// using these in order to manage keyboard focus.
	//
	case FocusIn:
	    tv->focusIn();
	    break;
	case FocusOut:
	    tv->focusOut();
	    break;
	default:
	    break;
    }
}

void TreeView_ResizeCB(Widget w, XtPointer clientData, XtPointer callData)
{
    XmDrawingAreaCallbackStruct *cbs = (XmDrawingAreaCallbackStruct*)callData;
    ASSERT(clientData);
    TreeView* tv = (TreeView*)clientData;
}

void TreeView_MotionEH (Widget w, XtPointer cData, XEvent* xev, Boolean* doit)
{
    *doit = True;
    TreeView* tv = (TreeView*)cData;
    XMotionEvent* xmo = (XMotionEvent*)xev;
    tv->motion(xev->xmotion.x, xev->xmotion.y);
}
void TreeView_RolloverEH (Widget w, XtPointer cData, XEvent* xev, Boolean* doit)
{
    *doit = True;
    TreeView* tv = (TreeView*)cData;
    if (xev->type == EnterNotify) {
	tv->enter();
    } else if (xev->type == LeaveNotify) {
	tv->leave();
    }
}

void TreeView_IBeamTO (XtPointer clientData, XtIntervalId* )
{
    TreeView* tv = (TreeView*)clientData;
    tv->toggleIBeam();
}

} // end extern C

