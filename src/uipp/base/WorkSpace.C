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
#include <Xm/ScrolledW.h>
#include <X11/cursorfont.h>

#ifdef DXD_WIN
#include "../widgets/XmDX.h"
#else
#include "XmDX.h"
#endif
#include "WorkSpace.h" 
#include "WorkSpaceInfo.h" 
#include "Application.h" 
#include "List.h" 
#include "ListIterator.h" 
#include <../widgets/WorkspaceW.h>


String WorkSpace::DefaultResources[] =
{
     ".traversalOn:		True",
     ".marginWidth:		0",
     ".marginHeight:		0",
     ".allowResize:		True",
     ".sensitive:		True",
     ".allowMovement:		True",
     ".allowOverlap:		False",
     ".button1PressMode:    	False",
     ".resizePolicy:        	RESIZE_GROW",
     ".accentPolicy:        	ACCENT_BACKGROUND",
     ".horizontalDrawGrid:  	DRAW_HASH",
     ".verticalDrawGrid:    	DRAW_HASH",
     ".horizontalAlignment: 	ALIGNMENT_CENTER",
     ".verticalAlignment:   	ALIGNMENT_CENTER",
     ".inclusionPolicy:     	INCLUDE_ALL",
     ".outlineType:         	OUTLINE_EACH",
     ".selectionPolicy:     	EXTENDED_SELECT",
     ".sortPolicy:          	ALIGNMENT_BEGINNING",
     NUL(char*)

};

WorkSpace::WorkSpace( const char* name, Widget parent, WorkSpaceInfo *wsinfo) : 
			UIComponent(name)
{
    this->info = wsinfo;
    this->parent = parent;
    this->info->associateWorkSpace(this);
    this->activeManyPlacements = 0;
}

WorkSpace::~WorkSpace()
{
    if (this->isRoot())
	XtRemoveEventHandler(this->parent, StructureNotifyMask, False,
	    (XtEventHandler)WorkSpace_ResizeHandlerEH, (XtPointer)this);
    if (this->info)
	this->info->associateWorkSpace(NULL);
}

void WorkSpace::manage()
{
     if (!this->getRootWidget())
	this->initializeRootWidget(FALSE);

     this->UIComponent::manage();
}

void WorkSpace::initializeRootWidget(boolean fromFirstIntanceOfADerivedClass)
{
    int              w, h, n;
    Arg              warg[20];
    boolean	     prevent_overlap;

    if (fromFirstIntanceOfADerivedClass) {
        ASSERT(theApplication);
        this->setDefaultResources(theApplication->getRootWidget(),
                                  WorkSpace::DefaultResources);
    }
    n = 0;
    XtSetArg(warg[n],XmNwidth, this->info->getWidth()); n++;
    XtSetArg(warg[n],XmNheight, this->info->getHeight()); n++;

    //
    // Info from grid spec
    //
    WorkSpaceInfo *info = this->info;
    info->getGridSpacing(w,h);
    prevent_overlap = info->getPreventOverlap();
    XtSetArg(warg[n],XmNgridWidth, w); n++;
    XtSetArg(warg[n],XmNgridHeight, h); n++;
    XtSetArg(warg[n],XmNsnapToGrid, 
			(info->isGridActive() ? True : False)); n++;
    XtSetArg(warg[n],XmNverticalAlignment,   info->getGridYAlignment()); n++;
    XtSetArg(warg[n],XmNhorizontalAlignment, info->getGridXAlignment()); n++;

    XtSetArg(warg[n],XmNhorizontalDrawGrid, 
	(info->getGridXAlignment() == XmALIGNMENT_NONE) &&
	(info->getGridYAlignment() == XmALIGNMENT_CENTER) ? 
		XmDRAW_NONE : XmDRAW_HASH); n++;
    XtSetArg(warg[n],XmNverticalDrawGrid, 
	(info->getGridYAlignment() == XmALIGNMENT_NONE) && 
	(info->getGridXAlignment() == XmALIGNMENT_CENTER) ? 
		XmDRAW_NONE : XmDRAW_HASH); n++;
    XtSetArg(warg[n], XmNpreventOverlap, prevent_overlap); n++;
    

    this->setRootWidget(
        XtCreateWidget
            (name,
             xmWorkspaceWidgetClass,
             this->parent,
	     warg,
	     n));

    this->display = XtDisplay(this->getRootWidget());

    XtAddCallback(this->getRootWidget(),
                      XmNdefaultActionCallback,
                      (XtCallbackProc)WorkSpace_DefaultActionCB,
                      (XtPointer)this);

    XtAddCallback(this->getRootWidget(),
                  XmNbackgroundCallback,
                  (XtCallbackProc)WorkSpace_BackgroundCB,
                  (XtPointer)this);

    XtAddCallback(this->getRootWidget(),
                  XmNselectionCallback,
                  (XtCallbackProc)WorkSpace_SelectionCB,
                  (XtPointer)this);

    XtAddCallback(this->getRootWidget(),
                  XmNpositionChangeCallback,
                  (XtCallbackProc)WorkSpace_PosChangeCB,
                  (XtPointer)this);

    //
    // Record initial lineDrawing, overlap, and manhattan values so that
    // they can be toggle later, on behalf of pages.
    //
    this->saveWorkSpaceParams(NUL(WorkSpace*));

    if (this->isRoot())
	XtAddEventHandler(this->parent, StructureNotifyMask, False,
	    (XtEventHandler)WorkSpace_ResizeHandlerEH, (XtPointer)this);
}

extern "C" void WorkSpace_PosChangeCB(Widget widget,
                XtPointer clientData,
                XtPointer callData)
{
    WorkSpace *ws = (WorkSpace*) clientData;
    ws->doPosChangeAction(widget, callData);
}

extern "C" void WorkSpace_DefaultActionCB(Widget widget,
                XtPointer clientData,
                XtPointer callData)
{
    WorkSpace *ws = (WorkSpace*) clientData;
    ws->doDefaultAction(widget, callData);
}

extern "C" void WorkSpace_BackgroundCB(Widget widget,
                XtPointer clientData,
                XtPointer calldata)
{
    WorkSpace *ws = (WorkSpace*) clientData;

    ws->doBackgroundAction(widget,calldata);
}

extern "C" void WorkSpace_SelectionCB(Widget widget,
                XtPointer clientData,
                XtPointer calldata)
{
    WorkSpace *ws = (WorkSpace*) clientData;

    ws->doSelectionAction(widget,calldata);
}


void WorkSpace::setCursor(int cursorType)
{
    Widget widget;
    int    n;
    Arg    arg[5];


       static int _type[] =
    {
        XC_ul_angle,
        XC_fleur,
        XC_watch,
        XC_left_ptr
    };

    static Cursor _cursor[4] =
    {
        NUL(Cursor),
        NUL(Cursor),
        NUL(Cursor),
        NUL(Cursor)
    };

    widget = this->getRootWidget();
    ASSERT(widget);

    n = 0;
    XtSetArg(arg[n], XmNbutton1PressMode, True); n++;
    XtSetValues(widget, arg, n);

    if (!XtIsRealized(widget))
	return;

    ASSERT(cursorType >= 0 AND cursorType < 4);

    if (_cursor[cursorType] == NUL(Cursor))
    {
        _cursor[cursorType] =
            XCreateFontCursor(XtDisplay(widget), _type[cursorType]);
    }

    XDefineCursor(XtDisplay(widget), XtWindow(widget), _cursor[cursorType]);

    XFlush(XtDisplay(widget));
}

//
// reset the WorkSpace cursor to the default.
//
void WorkSpace::resetCursor()
{
    Widget widget;
    int n;
    Arg arg[2];

    widget = this->getRootWidget();
    ASSERT(widget);

    n = 0;
    XtSetArg(arg[n], XmNbutton1PressMode, False); n++;
    XtSetValues(widget, arg, n);

    if (!XtIsRealized(widget))
	return;

    XUndefineCursor(XtDisplay(widget), XtWindow(widget));

    XFlush(XtDisplay(widget));
}

//
// Set the Height and Width of the work space.
//
void WorkSpace::setWidth(int val)
{
    this->info->setWidth(val);
    if (!this->getRootWidget()) 
	return;
    XtVaSetValues(this->getRootWidget(),XmNwidth, val, NULL);
}
void WorkSpace::setHeight(int val )
{
    this->info->setHeight(val);
    if (!this->getRootWidget()) 
	return;
    XtVaSetValues(this->getRootWidget(),XmNheight, val, NULL);
}
void WorkSpace::setWidthHeight(int width, int height )
{
    this->info->setWidth(width);
    this->info->setHeight(height);
    if (!this->getRootWidget()) 
	return;
    XtVaSetValues(this->getRootWidget(),
		  XmNwidth, width, 
		  XmNheight, height, 
		  NULL);
}
void WorkSpace::getMaxWidthHeight(int *w, int *h)
{
    XmWorkspaceGetMaxWidthHeight(this->getRootWidget(), w, h);
}
extern "C" void WorkSpace_ResizeHandlerEH(Widget widget,
                              XtPointer clientdata,
                              XEvent*,
			      Boolean*)

{
    WorkSpace * ws = (WorkSpace*) clientdata;

    ws->resize();
}

//
// Install the new grid in the WorkSpace
//
void WorkSpace::installInfo(WorkSpaceInfo *new_info)
{
    int n, w,h;
    Arg warg[20];
    boolean prevent_overlap;

    if (new_info) {
	this->info->disassociateWorkSpace();
        this->info = new_info;
	this->info->associateWorkSpace(this);
    } 
    
    prevent_overlap = this->info->getPreventOverlap();

    //
    // Install the grid info in the work space. 
    //
    n = 0;
    this->info->getGridSpacing(w,h);
    XtSetArg(warg[n],XmNgridWidth, w); n++;
    XtSetArg(warg[n],XmNgridHeight, h); n++;
    XtSetArg(warg[n],XmNsnapToGrid, 
			(this->info->isGridActive() ? True : False)); n++;
    XtSetArg(warg[n],XmNverticalAlignment,   this->info->getGridYAlignment()); n++;
    XtSetArg(warg[n],XmNhorizontalAlignment, this->info->getGridXAlignment()); n++;

    XtSetArg(warg[n],XmNhorizontalDrawGrid, 
	(this->info->getGridXAlignment() == XmALIGNMENT_NONE) &&
	(this->info->getGridYAlignment() == XmALIGNMENT_CENTER) ? 
		XmDRAW_NONE : XmDRAW_HASH); n++;
    XtSetArg(warg[n],XmNverticalDrawGrid, 
	(this->info->getGridYAlignment() == XmALIGNMENT_NONE) && 
	(this->info->getGridXAlignment() == XmALIGNMENT_CENTER) ? 
		XmDRAW_NONE : XmDRAW_HASH); n++;

    this->info->getXYSize(&w,&h);
    XtSetArg(warg[n],XmNwidth, w); n++;
    XtSetArg(warg[n],XmNheight, h); n++;
    XtSetArg(warg[n],XmNpreventOverlap, prevent_overlap); n++;
    XtSetValues(this->getRootWidget(), warg, n);
}    
//
// Optimized the work space for a bunch of placements 
//
void WorkSpace::beginManyPlacements()
{
   ASSERT(this->getRootWidget());

   this->activeManyPlacements++;
   if (this->activeManyPlacements == 1) {
	XtVaSetValues(this->getRootWidget(),
		XmNlineDrawingEnabled, False,
		XmNallowOverlap, True,
		NULL);
    }
}
void WorkSpace::endManyPlacements()
{
    ASSERT(this->getRootWidget());
    this->activeManyPlacements--;
    if (this->activeManyPlacements == 0) {
	Boolean overlap = this->wasAllowOverlap;
	Boolean drawing = this->wasLineDrawingEnabled;
	if (this->getRootWidget())
	    XtVaSetValues(this->getRootWidget(),
		XmNlineDrawingEnabled, drawing,
		XmNallowOverlap, overlap,
	    NULL);
    }
}

void WorkSpace::setPlacementCount(int count)
{
    boolean active = (count==0);
    boolean was_active = (this->activeManyPlacements==0);
    if (active != was_active) {
	Boolean overlap = (active?this->wasAllowOverlap:True);
	Boolean drawing = (active?this->wasLineDrawingEnabled:False);
	if (this->getRootWidget())
	    XtVaSetValues(this->getRootWidget(),
		XmNlineDrawingEnabled, drawing,
		XmNallowOverlap, overlap,
	    NULL);
    }
    this->activeManyPlacements = count;
}


//
// This logic is supplied by WorkSpaceRoot with the addition of workspace pages.
// This version of resize() is in use only for ControlPanelWorkSpace which isn't
// paged as of 12/96.  As soon as ControlPanelWorkSpace is converted to use
// WorkSpaceRoot and WorkSpacePage, WorkSpace::resize should become pure virtual.
//
void WorkSpace::resize()
{
    Dimension width, height;
    Dimension sw_width, sw_height;
    Dimension vsb_width, hsb_height;
    int max_w, max_h;
    Widget wid, vsb, hsb;
    Widget widget = XtParent(XtParent(this->getRootWidget()));

    XmWorkspaceGetMaxWidthHeight(this->getRootWidget(), &max_w, &max_h);

    vsb_width = 0;
    hsb_height = 0;
    if(XmIsScrolledWindow(widget))
    {
	Position x, y;

	XtVaGetValues(widget,
		      XmNhorizontalScrollBar, &hsb,
		      XmNverticalScrollBar, &vsb,
		      NULL);
	XtVaGetValues(widget, XmNwidth, &sw_width, XmNheight, &sw_height, NULL);

	//
	// Vertical ScrollBar
	//
	XtVaGetValues(vsb, XmNx, &x, XmNy, &y, NULL);
	if((x < sw_width) && (y < sw_height))
	    XtVaGetValues(vsb, XmNwidth, &vsb_width, NULL);

	//
	// Horizontal ScrollBar
	//
	XtVaGetValues(hsb, XmNx, &x, XmNy, &y, NULL);
	if((x < sw_width) && (y < sw_height))
	    XtVaGetValues(hsb, XmNheight, &hsb_height, NULL);

    }
       
    if(XmIsScrolledWindow(widget))
	XtVaGetValues(widget, XmNclipWindow, &wid, NULL);
    else
	wid = widget;

    XtVaGetValues(wid,
		  XmNwidth, &width,
		  XmNheight,&height,
		  NULL);

    width += vsb_width;
    height += hsb_height;
    this->setWidthHeight(MAX(width, max_w), MAX(height, max_h));
}

//
// This will probably be supplied in a virtual version in a subclass unless
// there is some reason to have pages with different values.  The reason this
// routine is required is to make it convenient to have all WorkSpaces use the
// same values even though we can't make all WorkSpaces recognize the same
// Xdefault setting.  So we want to be able to install a parent's values into
// a page if that's what the subclass wants.
//
void WorkSpace::saveWorkSpaceParams(WorkSpace *ws)
{
    Boolean overlap, drawing, manhattan;

    if (ws) {
	this->wasAllowOverlap = ws->wasAllowOverlap;
	this->wasLineDrawingEnabled = ws->wasLineDrawingEnabled;
	this->wasManhattanEnabled = ws->wasManhattanEnabled;

	//
	// Deal with manhattanRoute in a special way.  We never toggle its value.
	// We normally turn on/off lineDrawing.  So if manhattanRoute wants to be
	// off, unset it now and don't ever touch it again.
	//
	if (!this->wasManhattanEnabled)
	    XtVaSetValues (this->getRootWidget(), XmNmanhattanRoute, False, NULL);
    } else {
	XtVaGetValues(this->getRootWidget(),
	    XmNlineDrawingEnabled, &drawing,
	    XmNallowOverlap, &overlap,
	    XmNmanhattanRoute, &manhattan,
	NULL);
	this->wasAllowOverlap = overlap;
	this->wasLineDrawingEnabled = drawing;
	this->wasManhattanEnabled = manhattan;
    }
}

boolean WorkSpace::isEmpty (int x, int y, int width, int height)
{
    return XmWorkspaceRectangleEmpty (this->getRootWidget(), x,y,width,height);
}
