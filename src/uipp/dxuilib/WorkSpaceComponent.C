/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#include <Xm/CascadeB.h>
#include <Xm/CascadeBG.h>

#include "XmUtility.h"
#include "DXStrings.h"
#include "WorkSpaceComponent.h"
#include "DXApplication.h"
#include "../widgets/WorkspaceW.h"
#include <Xm/Form.h>
#include <Xm/Label.h>
#include "List.h"
#include "ListIterator.h"
#include "DynamicResource.h"
#include "Network.h" // for setFileDirty

boolean WorkSpaceComponent::WorkSpaceComponentClassInitialized = FALSE;

#if KEEP_USING_TRANSLATIONS
static XtTranslations wwTranslations = 0;
//
// This translation table makes descendants of a Workspace widget participate
// in selecting, moving, and resizing operations.  This is an alternative to
// the ::PassEvents() method which used to do some Xlib stuff.  PassEvents()
// is now replaced with a call to install this translation table on a widget.
// The call is now ::passEvents -- it's not static and it uses the "this" pointer.
//
static String wwTable = "\
 <Btn1Down>:    select_w()\n\
 <Btn1Motion>:  move_w()\n\
 <Btn1Up>(2+):  select_w() release_w() select_w() release_w()\n\
 <Btn1Up>:      release_w()\n\
";
#else
//
// Stop using translations and use event handlers instead because on some widgets
// the <Btn1Motion> translations doesn't do anything until the mouse leaves the
// the window which is pretty bizarre behavior, but at least the lower level
// mechanism works properly.
//
#endif

long WorkSpaceComponent::NextInstanceNumber = 1;

String WorkSpaceComponent::DefaultResources[] =
{
  NUL(char*)
};


WorkSpaceComponent::WorkSpaceComponent(const char *name, boolean developerStyle) : 
	UIComponent(name)
{
    this->developerStyle = developerStyle;
    this->selected = FALSE;
    this->customPart = 0;
    this->visualsInited = FALSE;
    this->userAssignedWidth = this->userAssignedHeight = 0;
    this->currentLayout = WorkSpaceComponent::NotSet;
    this->setResourceList = NUL(List*);
    this->instance = WorkSpaceComponent::NextInstanceNumber++;

    if (NOT WorkSpaceComponent::WorkSpaceComponentClassInitialized)
    {
        WorkSpaceComponent::WorkSpaceComponentClassInitialized = TRUE;
#if KEEP_USING_TRANSLATIONS
	wwTranslations = XtParseTranslationTable (wwTable);
#endif
    }
}

WorkSpaceComponent::~WorkSpaceComponent()
{
    if (this->setResourceList) {
	ListIterator it(*this->setResourceList);
	DynamicResource *dynres;
	while ( (dynres = (DynamicResource *)it.getNext()) ) {
	    delete dynres;
	}
	this->setResourceList->clear();
	delete this->setResourceList;
    }
}
 
 
//
// Pass button and motion events through to the parent of w.
// The boolean arg is here only because the subclass version of 
// passEvents needs to have one.
//
void WorkSpaceComponent::passEvents(Widget w, boolean )
{
    ASSERT(WorkSpaceComponent::WorkSpaceComponentClassInitialized);
#if KEEP_USING_TRANSLATIONS
    ASSERT(wwTranslations);
    XtOverrideTranslations (w, wwTranslations);
#else
    XtAddEventHandler (w, Button1MotionMask, False, 
	(XtEventHandler)Component_motionEH, (XtPointer)NULL);
    XtAddEventHandler (w, ButtonPressMask, False, 
	(XtEventHandler)Component_buttonPressEH, (XtPointer)NULL);
    XtAddEventHandler (w, ButtonReleaseMask, False, 
	(XtEventHandler)Component_buttonReleaseEH, (XtPointer)NULL);
#endif
}


extern "C" void 
Component_SelectWorkSpaceComponentCB(Widget , XtPointer clientdata, XtPointer callData)
{
    XmWorkspaceChildCallbackStruct* cb =  
				(XmWorkspaceChildCallbackStruct*)callData;
    WorkSpaceComponent	*d = (WorkSpaceComponent*) clientdata;
    ASSERT(d);
    d->setSelected(cb->status);
}



//
// Indicate that the interactor is selected. 
//
void WorkSpaceComponent::setSelected(boolean state)
{
Widget root = this->getRootWidget();

    this->selected = state;
    if (root) {
	Boolean oldState;
	XtVaGetValues(root, XmNselected, &oldState, NULL);
	if (oldState != state) {
	    XtVaSetValues(root, XmNselected, (Boolean)state, NULL);
	}
    }
}

//
// Filter the given label string so that among other things, "\n"
// becomes "<newline>".  The returned string must be freed by the
// caller.
//
char *WorkSpaceComponent::FilterLabelString(const char *label)
{
    char *filtered_string = DeEscapeString(label);
    return filtered_string;
}

//
// Make the interactor "blend into" its environment by turning of
// shadowThickness and setting colors to match those of the parent.
//
void WorkSpaceComponent::setAppearance(boolean developerStyle)
{
static struct {
   Pixel fg,bg,ts,bs,arm;
} cadet;

Pixel fg, bg, ts, bs, arm, white;
static short init = 0;
Screen *screen;
Colormap cmap;
Arg args[20];
Widget root;

    if ((this->visualsInited) && (this->developerStyle == developerStyle)) return ;
    this->visualsInited = TRUE;
    this->developerStyle = developerStyle;
    root = this->getRootWidget();
    if (!root) return ;

    Display *d = XtDisplay(root);
    white = WhitePixelOfScreen (DefaultScreenOfDisplay(d));

    if (developerStyle) {
	if (!init) {
	   init = 1;
	   XtVaGetValues (root, XmNcolormap, &cmap, XmNscreen, &screen, NULL);

	   cadet.bg = bg = theDXApplication->getStandInBackground();
	   XmGetColors (screen,cmap,bg,&cadet.fg, &cadet.ts, &cadet.bs, &cadet.arm);
	   cadet.fg = WhitePixelOfScreen (DefaultScreenOfDisplay(d));
	} 
	bg = cadet.bg; fg = cadet.fg;
	ts = cadet.ts; bs = cadet.bs;
	arm = cadet.arm;
    } else {
	XtVaGetValues (XtParent(root), XmNbackground, &bg, 
	     XmNcolormap, &cmap, XmNscreen, &screen, NULL);
	XmGetColors (screen, cmap, bg, &fg, &ts, &bs, &arm);
    }


    //
    // Perform a test of DynamicResource by using it set fg,bg,ts, and bs.
    //
//#define TESTING_DYNAMIC_RESOURCE_CLASS 1
#if TESTING_DYNAMIC_RESOURCE_CLASS
    if (!this->setResourceList) {
	this->setResourceList = new List;
	DynamicResource *drbg = new DynamicResource (XmNbackground, root);
	DynamicResource *drfg = new DynamicResource (XmNforeground, root);
	DynamicResource *drts = new DynamicResource (XmNtopShadowColor, root);
	DynamicResource *drbs = new DynamicResource (XmNbottomShadowColor, root);
	DynamicResource *drsc = new DynamicResource (XmNselectColor, root);
	DynamicResource *drac = new DynamicResource (XmNarmColor, root);
	DynamicResource *drtc = new DynamicResource (XmNtroughColor, root);

	this->setResourceList->appendElement ((void *)drbg);
	this->setResourceList->appendElement ((void *)drfg);
	this->setResourceList->appendElement ((void *)drts);
	this->setResourceList->appendElement ((void *)drbs);
	this->setResourceList->appendElement ((void *)drsc);
	this->setResourceList->appendElement ((void *)drac);
	this->setResourceList->appendElement ((void *)drtc);

	drbg->setDataDirectly ((void*)bg);
	drfg->setDataDirectly ((void*)fg);
	drts->setDataDirectly ((void*)ts);
	drbs->setDataDirectly ((void*)bs);
	drsc->setDataDirectly ((void*)white);
	drac->setDataDirectly ((void*)arm);
	drtc->setDataDirectly ((void*)arm);

	drbg->addWidgetsToNameList (root);
	drfg->addWidgetsToNameList (root);
	drts->addWidgetsToNameList (root);
	drbs->addWidgetsToNameList (root);
	drsc->addWidgetsToNameList (root);
	drac->addWidgetsToNameList (root);
	drtc->addWidgetsToNameList (root);
    } else {
	ListIterator it(*this->setResourceList);
	DynamicResource *dr;
	while (dr = (DynamicResource*)it.getNext()) {
	    const char *cp = dr->getResourceName();
	    if (!strcmp(cp, XmNbackground))  dr->setDataDirectly ((void*)bg);
	    else if (!strcmp(cp, XmNforeground))  dr->setDataDirectly ((void*)fg);
	    else if (!strcmp(cp, XmNtopShadowColor))  dr->setDataDirectly ((void*)ts);
	    else if (!strcmp(cp, XmNbottomShadowColor))  dr->setDataDirectly ((void*)bs);
	}
    }
    
#else
    // Set bg == ts == bs for root widget instead of setting shadowThickness.
    int n = 0;
    XtSetArg (args[n], XmNbackground, bg); n++;
    if (developerStyle) {
	XtSetArg (args[n], XmNtopShadowColor, ts); n++;
	XtSetArg (args[n], XmNbottomShadowColor, bs); n++;
    } else {
	XtSetArg (args[n], XmNtopShadowColor, bg); n++;
	XtSetArg (args[n], XmNbottomShadowColor, bg); n++;
    }
    XtSetArg (args[n], XmNselectColor, white); n++;
    XtSetArg (args[n], XmNarmColor, arm); n++;
    XtSetArg (args[n], XmNtroughColor, arm); n++;
    XtSetArg (args[n], XmNforeground, fg); n++;
    XtSetArg (args[n], XmNselectable, (developerStyle?True:False) ); n++;
    XtSetValues (root, args, n);

    n = 0;
    XtSetArg (args[n], XmNbackground, bg); n++;
    XtSetArg (args[n], XmNtopShadowColor, ts); n++;
    XtSetArg (args[n], XmNbottomShadowColor, bs); n++;
    XtSetArg (args[n], XmNselectColor, white); n++;
    XtSetArg (args[n], XmNarmColor, arm); n++;
    XtSetArg (args[n], XmNtroughColor, arm); n++;
    XtSetArg (args[n], XmNforeground, fg); n++;
    SetAllChildrenOf (root, args, n);


#endif

    //
    // Restore those DynamicResource settings which might have been affected
    // by the change in modes.  Currently setting them all, but one day it may be more
    // work than is necessary.  The settings in setResourceList might be touched
    // by the SetAllChildrenOf call, i.e. XmNforeground which conflicts with
    // the DynamicResource for Decorators.
    //
    if (this->setResourceList) {
	ListIterator it(*this->setResourceList);
	DynamicResource *dr;
	while ( (dr = (DynamicResource*)it.getNext()) ) {
	    dr->setData();
	}
    }
}

//
// walk the widget tree setting resources.
//
void WorkSpaceComponent::SetAllChildrenOf (Widget w, Arg *args, int n)
{
Widget *kids, menu;
int nkids, i;


    if ((XtIsSubclass (w, xmCascadeButtonWidgetClass))||
	(XtIsSubclass (w, xmCascadeButtonGadgetClass))) {
	menu = 0;
	XtVaGetValues (w, XmNsubMenuId, &menu, NULL);
	if (!menu) return ;
	XtSetValues (menu, args, n);
	SetAllChildrenOf (menu, args, n);
    }
    if (!XtIsComposite (w)) return ;

    XtVaGetValues (w, XmNchildren, &kids, XmNnumChildren, &nkids, NULL);
    for (i=0; i<nkids; i++) {
	if (XtIsShell (kids[i])) continue ;
	XtSetValues (kids[i], args, n);
	SetAllChildrenOf (kids[i], args, n);
    }
}

//
// Determine if this Component is a node of the given class
//
boolean WorkSpaceComponent::isA(const char *classname)
{
    Symbol s = theSymbolManager->registerSymbol(classname);
    return this->isA(s);
}
//
// Determine if this Component is of the given class.
//
boolean WorkSpaceComponent::isA(Symbol classname)
{
    Symbol s = theSymbolManager->registerSymbol(ClassWorkSpaceComponent);
    return (s == classname);
}


void
WorkSpaceComponent::GetDimensions(Widget w, int *height, int *width)
{
Dimension wdim, hdim;

    XtVaGetValues(w, XmNheight, &hdim, XmNwidth, &wdim, NULL);
    *width = wdim; *height = hdim;
}

void WorkSpaceComponent::installResizeHandler()
{
    XmWorkspaceAddCallback(this->getRootWidget(), XmNresizingCallback, 
	(XtCallbackProc)Component_ResizeCB, (XtPointer)this);
}

extern "C" void Component_ResizeCB(Widget , XtPointer clientdata, XtPointer)
{
    WorkSpaceComponent *wcs = (WorkSpaceComponent*) clientdata;
    wcs->GetDimensions (wcs->getRootWidget() , 
	&wcs->userAssignedHeight, &wcs->userAssignedWidth);
    wcs->resizeCB();
    wcs->getNetwork()->setFileDirty();
}

void
WorkSpaceComponent::SetLabelResource (Widget w, const char *labelString)
{
    char        *filtered_string;
    XmString    string;
    XmFontList  xmfl;
    Dimension   neww, newh;
    XtWidgetGeometry req;
    Dimension   mw,mh,mt,mb,ml,mr; // margin sizes
    Dimension   ht,st; // thicknesses
    Dimension   width, height;

    if (!labelString) return ;

// FIXME: is it necessary to free the fontList acquired thru XtGetValues ?

    /*
     * Replace "\n" character sequences with the '\n' character.
     */
    filtered_string = WorkSpaceComponent::FilterLabelString(labelString);
    string = XmStringCreateLtoR((char*)filtered_string, "canvas");

    mw = mh = mt = mb = ml = mr = ht = st = 0;
    XtVaGetValues (w, XmNfontList, &xmfl, XmNmarginWidth, &mw,
	XmNmarginHeight, &mh, XmNmarginWidth, &mw, XmNmarginHeight, &mh,
	XmNmarginLeft, &ml, XmNmarginRight, &mr, XmNmarginTop, &mt,
	XmNmarginBottom, &mb, XmNshadowThickness, &st, XmNhighlightThickness, &ht, 
	XmNwidth, &width, XmNheight, &height, NULL);
    XmStringExtent (xmfl, string, &neww, &newh);
    req.width = neww + (2*(mw + ht + st)) + ml + mr; 
    req.height = newh + (2*(mh + ht + st)) + mt + mb;
    req.request_mode = 0;
    /*if (req.width > width)*/ req.request_mode|= CWWidth;
    /*if (req.height > height)*/ req.request_mode|= CWHeight;
    if (req.request_mode) XtMakeGeometryRequest (w, &req, NULL);
    XtVaSetValues(w,XmNlabelString, string, NULL);

    XmStringFree(string);
    delete filtered_string;
}

//
// If there as never been an explicit size requested by the user, then
// return 0,0 so that the Motif's geometry mgmt will always provide the
// smallest necessary size.  If the user or cfg file did specify a size,
// then use the actual size.  Ironically, we never return the size the
// user explicitly made.
//
void
WorkSpaceComponent::getXYSize (int *width, int *height)
{

#if USE_OWN_SIZES
int uw, uh;
int cw, ch;
    this->getUserDimensions (&uw, &uh);
    if (uw && uh)
	this->UIComponent::getXYSize (width, height);
    else
	*width = *height = 0 ;
#else
	this->UIComponent::getXYSize (width, height);
#endif
}

void
WorkSpaceComponent::setVerticalLayout (boolean vertical)
{
    if (!this->acceptsLayoutChanges()) return ;
    ASSERT (
	((this->currentLayout & WorkSpaceComponent::Vertical) != 
	(this->currentLayout & WorkSpaceComponent::Horizontal))
	||
	(this->currentLayout == WorkSpaceComponent::NotSet)
    );

    if (vertical) {
	this->currentLayout&= ~WorkSpaceComponent::Horizontal;
	this->currentLayout|= WorkSpaceComponent::Vertical;
    } else {
	this->currentLayout&= ~WorkSpaceComponent::Vertical;
	this->currentLayout|= WorkSpaceComponent::Horizontal;
    }
    this->getNetwork()->setFileDirty();
}

boolean
WorkSpaceComponent::verticallyLaidOut()
{
    if (this->currentLayout & WorkSpaceComponent::NotSet) return FALSE;

    return (this->currentLayout & WorkSpaceComponent::Vertical);
}


#if MIGHT_NEED_THIS_LATER
//
// Get the actual value which used to set some XmNresource.  This data would
// have started out as a null-terminated char* and been converted to a value
// thru some type convertor called from DynamicResource.  I think this function
// is not currently in use, but it's purpose is to enable copying resource values
// around.
//
void *
WorkSpaceComponent::getResource (const char *res)
{
ListIterator it;
DynamicResource *dr;

    if (!this->setResourceList) return NUL(void *);

    it.setList (*this->setResourceList);
    while (dr = (DynamicResource *)it.getNext()) {
	if (!strcmp (dr->getResourceName(), res)) {
	    break;
	}
    }
    if (!dr) return NUL(void*);

    return (void*)dr->getData();
}
#endif

//
// Get the null-terminated char* which was used to set some XmNresource.
//
const char *
WorkSpaceComponent::getResourceString (const char *res)
{
ListIterator it;
DynamicResource *dr;

    if (!this->setResourceList) return NUL(const char *);

    it.setList (*this->setResourceList);
    while ( (dr = (DynamicResource *)it.getNext()) ) {
        if (!strcmp (dr->getResourceName(), res)) {
            break;
        }
    }
    if (!dr) return NUL(const char*);
    return (const char *)dr->getStringRepresentation();
}

boolean
WorkSpaceComponent::isResourceSet (const char *res)
{
ListIterator it;
DynamicResource *dr;

    if (!this->setResourceList) return FALSE;

    it.setList (*this->setResourceList);
    while ( (dr = (DynamicResource *)it.getNext()) ) {
        if (!strcmp (dr->getResourceName(), res)) {
	    return TRUE;
        }
    }
    return FALSE;
}

//
// Copy my DynamicResource settings into someone else
//
void WorkSpaceComponent::transferResources (WorkSpaceComponent *wsc)
{
    if (!this->setResourceList) return ;
    if (!wsc->setResourceList)
	wsc->setResourceList = new List;

    ListIterator it (*this->setResourceList);
    DynamicResource *dr;
    while ( (dr = (DynamicResource*)it.getNext()) ) {
	wsc->setResourceList->appendElement((void*)dr);
	if (this->customPart) dr->setRootWidget(this->getRootWidget());
	else dr->setRootWidget(NUL(Widget));
    }
    this->setResourceList->clear();
}

void WorkSpaceComponent::widgetDestroyed ()
{
    if (this->setResourceList) {
	ListIterator it (*this->setResourceList);
	DynamicResource *dr;
	while ( (dr = (DynamicResource*)it.getNext()) ) 
	    dr->setRootWidget(NUL(Widget));
    }
    this->UIComponent::widgetDestroyed ();
}

//
// In addition to superclass work, we'll need to create new workspace
// lines since the start/end points of
// In the case of StandIn, using the superclass doesn't do any good
// since the widget is geometry-managed by the containing widget.  Setting
// XmN{x,y} has no effect.
//
void WorkSpaceComponent::setXYPosition (int x, int y)
{
    this->UIComponent::setXYPosition (x,y);
    if (this->isManaged())
	XmWorkspaceSetLocation (this->getRootWidget(), x, y);
}

// 
// Store the this pointer in the widget's XmNuserData so that we
// can retrieve the Object in a callback in EditorWorkSpace.C
// 
void WorkSpaceComponent::setRootWidget(Widget root, boolean standardDestroy)
{
    this->UIComponent::setRootWidget(root, standardDestroy);
    this->setLocalData(this);
}

extern "C"  {
void Component_motionEH 
(Widget w, XtPointer , XEvent *xev, Boolean *keep_going)
{
    *keep_going = True;
    XtCallActionProc (w, "move_w", xev, NULL, 0);
}
void Component_buttonReleaseEH 
(Widget w, XtPointer , XEvent *xev, Boolean *keep_going)
{
    *keep_going = True;
    if ((xev->xbutton.button != 1)&&(xev->xbutton.button != 2))
	return ;

    if ((xev->xbutton.button == 2) && (xev->xbutton.state & ShiftMask))
	return ;
    XtCallActionProc (w, "release_w", xev, NULL, 0);
}
void Component_buttonPressEH 
(Widget w, XtPointer , XEvent *xev, Boolean *keep_going)
{
    *keep_going = True;
    if ((xev->xbutton.button != 1)&&(xev->xbutton.button != 2))
	return ;

    if ((xev->xbutton.button == 2) && (xev->xbutton.state & ShiftMask))
	return ;
    XtCallActionProc (w, "select_w", xev, NULL, 0);
}
} // extern C
