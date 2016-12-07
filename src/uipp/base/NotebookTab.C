/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
#include "NotebookTab.h"
#include <Xm/DrawnB.h>
#include "DXStrings.h"
#include "Application.h"
#include <X11/Xlib.h>

int NotebookTab::CornerSize = 6;
boolean NotebookTab::ClassInitialized = FALSE;
XmFontList NotebookTab::FontList = 0;

XPoint NotebookTab::LeftCorner[] = {
    { 0, 5 },
    { 1, 5 },
    { 1, 4 },
    { 1, 3 },
    { 2, 3 },
    { 2, 2 },
    { 3, 2 },
    { 3, 1 },
    { 4, 1 },
    { 5, 0 },
};

//
// CoordModePrevious.  The user of these points must assign a valid
// coordinate to the x coord of the 0th element.  The x coord should be
// replaced with (width-CornerSize)
//
XPoint NotebookTab::TopCorner[] = {
    { 0, 1 },
    { 1, 0 },
    { 1, 1 },
};

//
// CoordModePrevious.  The user of these points must assign a valid
// coordinate to the x coord of the 0th element.  The x coord should
// be replaced with (width+3-CornerSize)
//
XPoint NotebookTab::RightCorner[] = {
    { 3, 3 },
    { 1, 1 },
    { 0, 1 },
};

NotebookTab::NotebookTab(const char* name) : UIComponent("notebookTab")
{
    if (!NotebookTab::ClassInitialized) {
	NotebookTab::ClassInitialized = TRUE;
    }
    this->set = FALSE;
    this->label = XmStringCreateLtoR((char*)name, "small_bold");
    this->label_str = DuplicateString(name);
    this->dirty = TRUE;
    this->dbl_buffer = 0;
    this->armed = FALSE;
    this->multiclick_timer = 0;
    this->repaint_timer = 0;
}

NotebookTab::~NotebookTab()
{
    if (this->multiclick_timer) XtRemoveTimeOut(this->multiclick_timer);
    if (this->repaint_timer) XtRemoveTimeOut(this->repaint_timer);
    if (this->label) XmStringFree(this->label);
    if (this->label_str) delete this->label_str;
    if (this->dbl_buffer) {
	XFreePixmap(XtDisplay(this->getRootWidget()), this->dbl_buffer);
    }
    Widget w = this->getRootWidget();
    if (w) {
	XtRemoveCallback (w, XmNexposeCallback, (XtCallbackProc)
	    NotebookTab_ExposeCB, (XtPointer)this);
    }
}

void NotebookTab::createButton(Widget parent)
{
    Widget w = XtVaCreateWidget (this->label_str, xmDrawnButtonWidgetClass, parent, 
	XmNmultiClick, XmMULTICLICK_KEEP,
	XmNpushButtonEnabled, FALSE,
	XmNshadowThickness, 0,
	XmNrecomputeSize, FALSE,
    NULL);
    this->setRootWidget(w);

    XtAddCallback (w, XmNexposeCallback, (XtCallbackProc)
	NotebookTab_ExposeCB, (XtPointer)this);
    XtAddCallback (w, XmNarmCallback, (XtCallbackProc)
	NotebookTab_ArmCB, (XtPointer)this);
    XtAddCallback (w, XmNdisarmCallback, (XtCallbackProc)
	NotebookTab_DisarmCB, (XtPointer)this);
    XtAddCallback (w, XmNactivateCallback, (XtCallbackProc)
	NotebookTab_ActivateCB, (XtPointer)this);

    //
    // Use an event handler for double clicks because we want to
    // prevent the multiclick from causing a button-pull-out
    //
    XtAddEventHandler (w, ButtonPressMask|ButtonReleaseMask,
	False, (XtEventHandler)NotebookTab_ButtonEH, (XtPointer)this);
}

void NotebookTab::activate()
{
    this->armed = FALSE;
    this->setState(this->getState()==FALSE);
}

void NotebookTab::arm()
{
    this->armed = TRUE;
    this->setDirty(TRUE,FALSE);
}

void NotebookTab::disarm()
{
    this->armed = FALSE;
    this->setDirty(TRUE,FALSE);
}

void NotebookTab::expose()
{
    if (!XtIsRealized(this->getRootWidget())) return ;
    if (this->isDirty()) this->repaint();
    if (this->repaint_timer) {
	XtRemoveTimeOut(this->repaint_timer);
	this->repaint_timer = 0;
    }
    Dimension width,height;
    Widget w = this->getRootWidget();
    XtVaGetValues (w, XmNwidth, &width, XmNheight, &height, NULL);
    Window win = XtWindow(w);
    Display* d = XtDisplay(w);
    XGCValues values;
    GC gc = XtGetGC (w, 0, &values);
    XCopyArea (d, this->dbl_buffer, win, gc, 0, 0, width, height, 0, 0);
}

void NotebookTab::setLabel(const char* str)
{
    if (this->label_str) delete this->label_str;
    if (str) this->label_str = DuplicateString(str);
    else this->label_str = 0;
    if (this->label) XmStringFree(this->label);
    if (str) {
	this->label = XmStringCreateLtoR((char*)str, (char*)this->getFont());
    } else {
	this->label = 0;
    }
    this->setDirty(TRUE,TRUE);
}

void NotebookTab::setState(boolean s)
{
    if (s == this->set) return ;
    this->set = s;
    this->setDirty(TRUE, TRUE);
}

void NotebookTab::setDirty(boolean d, boolean repaint)
{
    this->dirty = d;
    if (this->repaint_timer) XtRemoveTimeOut(this->repaint_timer);
    if (repaint) {
	this->expose();
	this->repaint_timer = 0;
    } else {
	XtAppContext apcxt = theApplication->getApplicationContext();
	int millis = 100;
	this->repaint_timer = XtAppAddTimeOut(apcxt, millis, (XtTimerCallbackProc)
	    NotebookTab_RepaintTO, (XtPointer)this);
    }
}

void NotebookTab::repaint() 
{
    Widget w = this->getRootWidget();
    Window win = XtWindow(w);
    if (!win) return ;

    Pixel fg,ts,bs,bg;
    int depth;
    Dimension width,height;

    XtVaGetValues (w,
	XmNwidth, &width,
	XmNheight, &height,
	XmNtopShadowColor, &ts,
	XmNbottomShadowColor, &bs,
	XmNforeground, &fg,
	XmNbackground, &bg,
	XmNdepth, &depth,
    NULL);


    int s = NotebookTab::CornerSize;
    Display* d = XtDisplay(w);
    if (!this->dbl_buffer) {
	this->dbl_buffer = XCreatePixmap(d, win, width, height, depth);
    }
    XGCValues values;

    values.foreground = bg;
    GC gc = XtGetGC (w, GCForeground, &values);
    XFillRectangle (d, this->dbl_buffer, gc, 0,0,width, height); 
    XtReleaseGC(w,gc);

    //
    // Draw left edge
    //
    boolean pushed_in = 
	((this->getState()) && (!this->isArmed()) ||
         (!this->getState()) && (this->isArmed()));
    if (pushed_in) {
	values.foreground = bs;
	values.background = bg;
    } else {
	values.foreground = ts;
	values.background = bg;
    }
    gc = XtGetGC (w, GCForeground|GCBackground, &values);
    XDrawLine (d, this->dbl_buffer, gc, 0,s,0,height-1);

    //
    // Draw top edge
    //
    XDrawLine (d, this->dbl_buffer, gc, s,0,width-(s+1),0);
    XtReleaseGC (w, gc);

    //
    // Draw right edge
    //
    if (pushed_in) {
	values.foreground = ts;
	values.background = bg;
    } else {
	values.foreground = bs;
	values.background = bg;
    }
    gc = XtGetGC (w, GCForeground|GCBackground, &values);
    XDrawLine (d, this->dbl_buffer, gc, width-1,s,width-1,height-1);
    XtReleaseGC(w, gc);

    //
    // Draw top left corner
    //
    if (pushed_in) {
	values.foreground = bs;
    } else {
	values.foreground = ts;
    }
    gc = XtGetGC (w, GCForeground, &values);

    int n = sizeof(NotebookTab::LeftCorner) / sizeof(XPoint);
    XDrawPoints (d, this->dbl_buffer, gc, NotebookTab::LeftCorner, n, CoordModeOrigin);

    //
    // Draw top portion of right corner
    //
    n = sizeof(NotebookTab::TopCorner) / sizeof(XPoint);
    NotebookTab::TopCorner[0].x = width - NotebookTab::CornerSize;
    XDrawPoints (d, this->dbl_buffer, gc, NotebookTab::TopCorner, n, CoordModePrevious);
    XtReleaseGC(w, gc);

    //
    // Draw bottom portion of right corner
    //
    if (pushed_in) {
	values.foreground = ts;
    } else {
	values.foreground = bs;
    }
    gc = XtGetGC (w, GCForeground, &values);
    n = sizeof(NotebookTab::RightCorner) / sizeof(XPoint);
    NotebookTab::RightCorner[0].x = width + 3 - NotebookTab::CornerSize;
    XDrawPoints (d, this->dbl_buffer, gc, NotebookTab::RightCorner, n, CoordModePrevious);
    XtReleaseGC(w, gc);

    //
    // Draw string
    //
    if (!NotebookTab::FontList) {
	XmFontList fontList;
	XtVaGetValues (w, XmNfontList, &fontList, NULL);
	NotebookTab::FontList = XmFontListCopy(fontList);
    }

    values.foreground = fg;
    values.background = bg;
    gc = XtGetGC (w, GCForeground|GCBackground, &values);

    Dimension strw, strh;
    XmStringExtent (NotebookTab::FontList, this->label, &strw, &strh);
    int sx = (width>>1) - (strw>>1);
    int sy = (height>>1) - (strh>>1);
    XmStringDraw (d, this->dbl_buffer, NotebookTab::FontList,
	this->label, gc, sx,sy, width, XmALIGNMENT_BEGINNING,
	XmSTRING_DIRECTION_L_TO_R, NUL(XRectangle*));
    XtReleaseGC(w, gc);

    this->dirty = FALSE;
}

void NotebookTab::multiClickTimer()
{
    if (this->multiclick_timer) {
	XtRemoveTimeOut(this->multiclick_timer);
	this->multiclick_timer = 0;
	this->multiClick();
    } else {
	XtAppContext apcxt = theApplication->getApplicationContext();
	int millis = XtGetMultiClickTime(XtDisplay(this->getRootWidget()));
	this->multiclick_timer = XtAppAddTimeOut(apcxt, millis, (XtTimerCallbackProc)
		NotebookTab_MultiClickTO, (XtPointer)this);
    }
}

extern "C" {

void NotebookTab_ExposeCB (Widget w, XtPointer clientData, XtPointer)
{
    NotebookTab* nt = (NotebookTab*)clientData;
    nt->expose();
}

void NotebookTab_ArmCB (Widget w, XtPointer clientData, XtPointer)
{
    NotebookTab* nt = (NotebookTab*)clientData;
    nt->arm();
}

void NotebookTab_DisarmCB (Widget w, XtPointer clientData, XtPointer)
{
    NotebookTab* nt = (NotebookTab*)clientData;
    nt->disarm();
}

void NotebookTab_ActivateCB (Widget w, XtPointer clientData, XtPointer callData)
{
    NotebookTab* nt = (NotebookTab*)clientData;
    XmDrawnButtonCallbackStruct* cbs = (XmDrawnButtonCallbackStruct*)callData;
    if (cbs->click_count == 1)
	nt->activate();
}

void NotebookTab_ButtonEH(Widget w, XtPointer clientData, XEvent* xev, Boolean *doit)
{
    *doit = TRUE;
    if (!xev) return ;
    if ((xev->type != ButtonPress) && (xev->type != ButtonRelease)) return ;
    if (xev->xbutton.button != 1) return ;
    NotebookTab* nt = (NotebookTab*)clientData;
    ASSERT(nt);
    if (nt->getState()) *doit = FALSE;
    if (xev->type == ButtonRelease)
	nt->multiClickTimer();
}

void NotebookTab_MultiClickTO (XtPointer clientData, XtIntervalId*)
{
    NotebookTab* nt = (NotebookTab*)clientData;
    nt->multiclick_timer = 0;
}

void NotebookTab_RepaintTO (XtPointer clientData, XtIntervalId*)
{
    NotebookTab* nt = (NotebookTab*)clientData;
    nt->repaint_timer = 0;
    nt->expose();
}

} // extern C
