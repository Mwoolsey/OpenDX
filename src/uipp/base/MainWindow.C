/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"

//#define USE_EDITRES 1

#include <Xm/Xm.h>
#include <Xm/MainW.h>
#include <Xm/DialogS.h>
#include <Xm/RowColumn.h>
#include <Xm/AtomMgr.h>
#include <Xm/Protocols.h>

#include "DXStrings.h"
#include "MainWindow.h"
#include "Application.h"
#include "CommandScope.h"

#if defined(HAVE_X11_XMU_EDITRES) && defined(USE_EDITRES)
#include <X11/Xmu/Editres.h>
#endif

#if defined(HAVE_XINERAMA)
#include <X11/extensions/Xinerama.h>
#endif

boolean MainWindow::OffsetsInitialized = FALSE;
boolean MainWindow::IsMwmBroken = FALSE;
boolean MainWindow::IsMwmRunning = FALSE;
int MainWindow::WmOffsetX = 0;
int MainWindow::WmOffsetY = 0;

String MainWindow::DefaultResources[] =
{
    NULL
};
extern "C" void MainWindow_HelpCB(Widget,
				  XtPointer clientData,
				  XtPointer callData)
{
    MainWindowHelpCallbackStruct* callbackData;
    MainWindow*                   window;

    callbackData = (MainWindowHelpCallbackStruct*)callData;
    if (callbackData->reason == DxCR_HELP)
    {
	window = (MainWindow*)clientData;
	window->provideContextHelp((callbackData->widget));
    }
}
extern "C" void MainWindow_CloseCB(Widget, XtPointer clientData, XtPointer)
{
    MainWindow* window;

    ASSERT(clientData);

    window = (MainWindow*)clientData;

    window->closeWindow();
}
extern "C" void MainWindow_PopdownCB(Widget, XtPointer clientData, XtPointer)
{
    MainWindow* window;

    ASSERT(clientData);

    window = (MainWindow*)clientData;

    window->popdownCallback();
}

//
// This method should only be called if this window does not have a 
// menu bar (i.e. this->hasMenuBar == FALSE);
//
void MainWindow::createMenus(Widget)
{
    ASSERT(!this->hasMenuBar);
}


MainWindow::MainWindow(const char* name, boolean usesMenuBar): UIComponent(name)
{
    this->workArea = NUL(Widget);
    this->commandArea = NUL(Widget);
    this->main = NUL(Widget);
    this->hasMenuBar = usesMenuBar;
    
    //
    // By default, MainWindow's can't be resized.
    // and it isn't managed.
    //
    this->resizable = FALSE;
    this->managed   = FALSE;
    this->title = NULL;
    this->menuBar = NULL;
    this->createX = UIComponent::UnspecifiedPosition;
    this->createY = UIComponent::UnspecifiedPosition;
    this->createWidth  = UIComponent::UnspecifiedDimension;
    this->createHeight = UIComponent::UnspecifiedDimension;
    this->screenLeftPos = 0;

    if (usesMenuBar) 
        this->commandScope = new CommandScope();
    else
        this->commandScope = NULL; 

    this->geometry_string = NUL(char*);

    //
    // Register this window with the application object.
    // The application object must exist before any
    // MainWindow object.
    //
    ASSERT(theApplication);
    theApplication->registerClient(this);
}


MainWindow::~MainWindow()
{
    //
    // Unregister this window from the application window list.
    //
    ASSERT(theApplication);
    theApplication->unregisterClient(this);

    if (this->title) delete[] this->title;

    if (this->hasMenuBar && this->commandScope)
	delete this->commandScope;
    
    if (this->geometry_string)
	delete[] this->geometry_string;
}

//
// Install the default resources for this class.
//
void MainWindow::installDefaultResources(Widget baseWidget)
{
    this->setDefaultResources(baseWidget, MainWindow::DefaultResources);
}


void MainWindow::provideContextHelp(Widget)
{
}


void MainWindow::initialize()
{
Arg args[5];
int n = 0;
Display* disp = XtDisplay(theApplication->getRootWidget());

	this->screenWidth = DisplayWidth(disp, DefaultScreen(disp));
	this->screenHeight = DisplayHeight(disp, DefaultScreen(disp));

#if defined(HAVE_XINERAMA)
	// Do some Xinerama Magic if Xinerama is present to
	// store the info on the largest screen.
	int dummy_a, dummy_b;
	int screens;
	XineramaScreenInfo   *screeninfo = NULL;
	int x=0, y=0, width=0, height=0;
	
	if ((XineramaQueryExtension (disp, &dummy_a, &dummy_b)) &&
		(screeninfo = XineramaQueryScreens(disp, &screens))) {
		// Xinerama Detected
		
		if (XineramaIsActive(disp)) {
			int i = dummy_a;
			while ( i < screens ) {
				if(screeninfo[i].width > width) {
					width = screeninfo[i].width;
					height = screeninfo[i].height;
					this->screenLeftPos = screeninfo[i].x_org;
					this->screenWidth = screeninfo[i].width;
					this->screenHeight = screeninfo[i].height;
				}
				i++;
			}
		}
	}
#endif

    const char* geomstr = this->getGeometryString();
    if ((geomstr) && (geomstr[0])) {
		XtSetArg (args[n], XmNgeometry, geomstr); n++; 
		if (!(strchr(geomstr, '-') || strchr(geomstr, '+'))) {
	    	if (this->createX != UIComponent::UnspecifiedPosition ) {
				XtSetArg (args[n], XmNx, this->createX); n++; 
			}
	    	if (this->createY != UIComponent::UnspecifiedPosition ) {
				XtSetArg (args[n], XmNy, this->createY); n++; 
			}
		}
		if (!(strchr(geomstr, 'x') || strchr(geomstr, 'X'))) {
	    	if (this->createWidth != UIComponent::UnspecifiedPosition ) {
				XtSetArg (args[n], XmNwidth, this->createWidth); n++; 
			}
	    	if (this->createHeight != UIComponent::UnspecifiedPosition ) {
				XtSetArg (args[n], XmNheight, this->createHeight); n++; 
			}
		}
    } else {
    	// Initialize the window based on the Xinerama data.
    	if (this->createX == UIComponent::UnspecifiedPosition ) {
    		if (this->screenWidth > 1000) {
				XtSetArg(args[n], XmNx, this->screenLeftPos+10); n++;
			}
		}
		if (this->createY == UIComponent::UnspecifiedPosition ) {
			if (this->screenHeight > 800) {
				XtSetArg(args[n], XmNy, 30); n++;
			}
		}		
    }

    //
    // Create an application shell as a popup shell off the
    // application s root shell widget.
    //

    this->setRootWidget(
	XtCreatePopupShell
	    (this->name,
	     topLevelShellWidgetClass,
	     theApplication->getRootWidget(),
	     args, 
	     n));

#if defined(HAVE_X11_XMU_EDITRES) && defined(USE_EDITRES)
    XtAddEventHandler((Widget)this->getRootWidget(), (EventMask)0, True,
         (XtEventHandler)_XEditResCheckMessages, 0);
#endif

    if(this->title)
    {
    	this->setWindowTitle(this->title);
	// DANZ: use the icon name defined in the default resources.
    	// this->setIconName(this->title);
    }

    //
    // Create the MainWindow widget.
    //
    this->main =
	XtCreateManagedWidget
	    ("mainWindow",
	     xmMainWindowWidgetClass,
	     this->getRootWidget(),
	     NULL,
	     0);

    //
    // Install XmNhelpCallback for the main window.
    //
    XtAddCallback
	(this->main,
	 XmNhelpCallback,
	 (XtCallbackProc)MainWindow_HelpCB,
	 (XtPointer)this);

    XtAddCallback
	(this->getRootWidget(),
	 XmNpopdownCallback,
	 (XtCallbackProc)MainWindow_PopdownCB,
	 (XtPointer)this);

    //
    // Create the work area.
    //
    this->workArea = this->createWorkArea(this->main);
    ASSERT(this->workArea);


    //
    // Create the dialog style buttons
    //
    this->commandArea = this->createCommandArea (this->main);

    //
    // Designate the this->workArea widget as the MainWindow
    // XmNworkArea widget.
    //
    XtVaSetValues
	(this->main,
	 XmNworkWindow, this->workArea,
	 NULL);
    if (this->commandArea) {
	XtVaSetValues (this->main, XmNcommandWindowLocation, XmCOMMAND_BELOW_WORKSPACE,
	    XmNcommandWindow, this->commandArea, NULL);
    }

    //
    // If the work area is not already managed, manage it now.
    //
    if (NOT XtIsManaged(this->workArea))
    {
	XtManageChild(this->workArea);
    }
    if (this->commandArea) {
      if (NOT XtIsManaged(this->commandArea))
      {
	   XtManageChild(this->commandArea);
      }
    }

    //
    // Override the default response to window manager "close" request.
    //
    XtVaSetValues(this->getRootWidget(), XmNdeleteResponse, XmDO_NOTHING, NULL);

    XmAddWMProtocolCallback
	(this->getRootWidget(),
	 XmInternAtom(XtDisplay(this->getRootWidget()),
			"WM_DELETE_WINDOW", FALSE),
	 (XtCallbackProc)MainWindow_CloseCB,
	 (void *)this);

 
    if (this->hasMenuBar)
    {
	this->createMenuBar(this->main);

#ifdef hp700
	//
	// Work around for HP Vue bug.
	//
	Pixel topShadow;
	Pixel bottomShadow;
	Pixel foreground;
	Pixel background;

	XtVaGetValues(this->main,
	    XmNforeground, &foreground,
	    XmNbackground, &background,
	    XmNtopShadowColor, &topShadow,
	    XmNbottomShadowColor, &bottomShadow,
	NULL); 
	XtVaSetValues(this->menuBar,
	    XmNforeground, foreground,
	    XmNbackground, background,
	    XmNtopShadowColor, topShadow,
	    XmNbottomShadowColor, bottomShadow,
	    NULL); 
#endif
	//
	// Call on the subclasses to create the menus.
	//
	this->createMenus(this->menuBar);

	//
	// Manage the menu bar.
	//
	XtManageChild(this->menuBar);
    }

    this->setGeometry(this->createX,this->createY, 
	this->createWidth, this->createHeight);

}

void MainWindow::createMenuBar(Widget parent)
{
#if 11	// Moved from DXWindow.C 7/14/95
    char menubar_name[1024];
    sprintf(menubar_name,"%sMenuBar",this->name);
    this->menuBar = XmCreateMenuBar(parent,menubar_name,NUL(ArgList), 0);
    this->installComponentHelpCallback(this->menuBar);
#else
    this->menuBar = XmCreateMenuBar(parent,"menuBar",NUL(ArgList), 0);
#endif

    XtVaSetValues(this->menuBar, XmNspacing, 8, NULL);
    XtVaSetValues(this->main, XmNmenuBar, this->menuBar, NULL);


}
void MainWindow::manage()
{
    if (!this->isInitialized()) 
	this->initialize();

    //
    // Pop up the shell.
    //
    ASSERT(this->getRootWidget());
    XtPopup(this->getRootWidget(), XtGrabNone);

    //
    // Map the window, in case it is iconified.
    //
    XMapRaised(XtDisplay(this->getRootWidget()),
	       XtWindow(this->getRootWidget()));

    //
    // Turn off any busy cursor.
    //
    XUndefineCursor(XtDisplay(this->main), XtWindow(this->main));

    // mark the window as managed;
    this->managed = TRUE;

    XmUpdateDisplay(this->main);
    XSync(XtDisplay(this->main), 0);
}


void MainWindow::unmanage()
{
    //
    // Pop down the shell.
    //
    ASSERT(this->getRootWidget());
    XtPopdown(this->getRootWidget());

    // mark the window as unmanaged
    this->managed = FALSE;
}

boolean MainWindow::isManaged()
{
    return this->managed;
}


void MainWindow::iconify()
{
    //
    // Set the widget to have an initial iconic state in case
    // the root widget has not yet been initialized.
    //
    ASSERT(this->getRootWidget());
    XtVaSetValues(this->getRootWidget(), XmNiconic, TRUE, NULL);

    //
    // If the shell has already been realized, iconify the window.
    //
    if (XtIsRealized(this->getRootWidget()))
    {
	XIconifyWindow(XtDisplay(this->getRootWidget()),
		       XtWindow(this->getRootWidget()), 0);
    }
}


void MainWindow::notify(const Symbol message, const void *data, const char *)
{
    if (message == Application::MsgManage)
    {
	ASSERT(this->isInitialized());
	this->manage();
    }
    else if (message == Application::MsgUnmanage)
    {
	this->unmanage();
    }
    else if (message == Application::MsgSetBusyCursor)
    {
	if (this->main != NULL && this->isManaged())
	{
	    XmUpdateDisplay(this->main);
	    XDefineCursor
		(XtDisplay(this->main),
		 XtWindow(this->main),
		 Application::BusyCursor);
	    XFlush(XtDisplay(this->main));
	}
    }
    else if (message == Application::MsgResetCursor)
    {
	if (this->main != NULL && this->isManaged())
	{
	    XUndefineCursor(XtDisplay(this->main), XtWindow(this->main));
	    XFlush(XtDisplay(this->main));

	    // HACK ALERT
	    // Somehow I have to figure out the difference between my coords
	    // and the ones imposed on me by mwm.  I can only do that after I
	    // put some window on the screen.  These numbers are used by setGeometry().
	    // I used to fetch these numbers when I needed them which was safe, but
	    // because of a dec/mwm bug, I need to fetch  them in order to set
	    // XmNgeometry which is earlier than the the window shows up.  So this
	    // initialization is here because I just need some relatively safe, 
	    // post-manage place to put it.
	    //
	    if (!MainWindow::OffsetsInitialized)
		MainWindow::InitializeOffsets(this->getRootWidget());
	}
    } 
    else if (message == Application::MsgManageByLeafClassName)
    {
	if (!data || EqualString(this->getClassName(),(char*)data))
	    this->manage();
    } 
    else if (message == Application::MsgUnmanageByLeafClassName)
    {
	if (!data || EqualString(this->getClassName(),(char*)data))
	    this->unmanage();
    } 
    else if (message == Application::MsgCreate)
    {
	if (!this->isManaged()) {
	    if (!this->isInitialized())
		this->initialize();
	    this->manage();
	}
    }


    // else ignore unrecognized messages, since applications can
    // extend the message set and use generic windows that are derived
    // from this class.

}

void MainWindow::allowResize(boolean resizable)
{
    this->resizable = resizable;
    if (this->getRootWidget())
	XtVaSetValues(this->getRootWidget(),
	    XmNallowShellResize, this->resizable? True: False,
	    NULL);
}


//
// Get the name/title of this window.
//
const char *MainWindow::getWindowTitle()
{
    String s;

    if (this->title) {
	return this->title;
    } else if (this->getRootWidget()) {
        XtVaGetValues(this->getRootWidget(), XmNtitle, &s, NULL);
	this->title = DuplicateString(s);
//	delete s;
	return this->title;
    } 
    return NULL; 

}
//
// Set the name/title of this window.
// If check_geometry is on, then check the resource database to see if
// there is a geometry setting matching the new title.
//
void MainWindow::setWindowTitle(const char *name, boolean check_geometry)
{
    ASSERT(name);
    boolean titles_equal = FALSE;

    if (!this->title)
    {
	this->title = DuplicateString(name);
    }
    else if (!EqualString(this->title, name))
    {
	if(this->title)
	    delete[] this->title;
	this->title = DuplicateString(name);
    } else {
	titles_equal = TRUE;
    }

    if ((this->getRootWidget()) && (check_geometry) && (titles_equal == FALSE)) {
	if (check_geometry) {
	    char* old_geom_string = this->geometry_string;
	    this->geometry_string = NUL(char*);
	    const char* new_geom_string = this->getGeometryString();
	    boolean geom_strings_equal = FALSE;
	    if ((new_geom_string) && (old_geom_string))
		geom_strings_equal = EqualString(new_geom_string, old_geom_string);
	    if ((!geom_strings_equal) && (new_geom_string)) {
		int x,y,w,h;
		this->getGeometry(&x,&y,&w,&h);
		this->setGeometry(x,y,w,h);
	    }
	    if (old_geom_string)
		delete[] old_geom_string;
	} else if (this->geometry_string) {
		delete[] this->geometry_string;
	    this->geometry_string = NUL(char*);
	}
    } 


    if (this->getRootWidget()) 
	XtVaSetValues(this->getRootWidget(), XmNtitle, name, NULL);

}
//
// Get the size and dimensions. 
//
boolean MainWindow::getGeometry(int *x, int *y, int *width, int *height)
{
    Position a = UIComponent::UnspecifiedPosition;
    Position b = UIComponent::UnspecifiedPosition;
    Dimension w = UIComponent::UnspecifiedDimension;
    Dimension h = UIComponent::UnspecifiedDimension;

    if (!this->getRootWidget()) {
	a = this->createX;
	b = this->createY;
	w = this->createWidth;
	h = this->createHeight;
    } else {
	Widget wid = this->getRootWidget();
	ASSERT(wid);

	XtVaGetValues(wid,
	    XmNwidth, &w,
	    XmNheight, &h,
	    NULL);

	//
	// The geometry takes the window manager into account.  We don't want to.
	//
	if(XtIsRealized(wid))
	{
	    Window win = XtWindow(wid);
	    Window root, parent, *children;
	    unsigned int n_child;
	    root = 0;
	    XQueryTree(XtDisplay(wid), win, &root, &parent, &children, &n_child);
	    XFree((char *)children); children = 0;
	    if(root != parent)
	    {
		while(root != parent)
		{
		    win = parent;
		    XQueryTree(XtDisplay(wid), win, 
			    &root, &parent, &children, &n_child);
		    XFree((char *)children); children = 0;
		}
	    }

	    XWindowAttributes attr;
	    XGetWindowAttributes(XtDisplay(wid), win, &attr);
	    a = attr.x;
	    b = attr.y;
	}
    }
    *x = a;
    *y = b;
    *width = w;
    *height = h;
    return (a != UIComponent::UnspecifiedPosition) &&
	   (b != UIComponent::UnspecifiedPosition) &&
	   (w != UIComponent::UnspecifiedDimension) &&
	   (h != UIComponent::UnspecifiedDimension);
}
//
// Set the size and dimensions. 
//
// Caveat for when mwm is running:
//	For windows which are already managed, you must compute new coords
//	which are offset a little bit because of the mwm border widths.
//	This is not necessary when setting XmNgeometry.
//
// Caveat for when mwm is running and you're displaying onto an sgi:
//	Fortunately, the sgi/4Dwm combo behaves differently from all others.
//	Assume you're running on it if MainWindow::IsMwmRunning==TRUE &&
//	_SGI__MENU_WINDOW on the root window is readable.  For 4Dwm, you
// 	must add wm offsets for both XmN{x,y} and for XmNgeometry just as with
//	the dec.  It's just harder to figure out if you're using 4Dwm.
//
// Caveat for when mwm is running and you're displaying onto a hp:
//	vue seems to work properly, so you have to treat it differently from
//	other versions of mwm.
//
// Caveat for when mwm is running and you're displaying onto a dec:
//	It appears to me that XmNgeometry and -geom both are broken when used
//	with dec's mwm.  This window mgr requires that I find the offsets and
//	use them for setting both XmNgeometry and XmN{x,y}.  (For other wmgrs
//	you would only apply the offsets to XmN{x,y} and not to XmNgeometry.)
//	I assume I'm displaying on a dec mwm if XmIsMotifWMRunning()==False
//	and the _MOTIF_WM_INFO property is present and readable.
//
// Caveat for when mwm is not running:
//	I tested with twm and olwm and naturally they behave differently from
//	each other.  I distinguish by looking for a property on the root window
//	named _SUN_WM_PROTOCOLS.  It's put there by olwm.  
//	So, for !mwm case, find out what the window mgr imposed offsets might
//	be and use them.  For olwm this is correct.
//	(example: With twm, you need to 0,0 and not 0+horiz offset, 0+vert offset
//	as you do for olwm and for mwm).   The way you find these offsets is different
// 	for mwm versus !mwm.  For !mwm you query the geometry of the window of the
//	dialog, whereas for mwm you query the geometry of the parent window of the
//	window of the dialog.
//
// Caveat for any window mgr:
//	I wrote code to calculate offsets 1 time only.  So if you change Xdefaults
//	and restart the window mgr, the offsets could be wrong, or if you change
//	window mgrs, then the offsets could be way wrong.
//
// CANTFIXME:
// Caveat for using .net files against lots of window mgrs:
//	What do you suppose will happen if the guy who saved the .net file made the
//	windows butt up against one another, and then someone else loads the .net
//	files?  It's unlikely that the new guy's window mgr will provide borders of
//	exactly the same sizes so upon reloading, the windows will probably overlap
//	or have gaps.  I think that means window placement is a bad thing.
//
// - Martin  
// 8/95
//
void MainWindow::setGeometry(int x, int y, int width, int height)
{

    if (!this->isInitialized()) {
	this->createX = x;
	this->createY = y;
	this->createWidth = width;
	this->createHeight = height;
    } else { 
   	Arg wargs[5]; 
	int 	n = 0;
	ASSERT(this->getRootWidget());

	const char* geom_str = this->getGeometryString();

	//
	// exclude those settings which are already in the geometry string
	//
	boolean ignore_size = FALSE;
	boolean ignore_pos = FALSE;
	if (geom_str) {
	    if (strchr(geom_str, 'x') || strchr(geom_str,'X')) ignore_size = TRUE;
	    if (strchr(geom_str, '-') || strchr(geom_str,'+')) ignore_pos = TRUE;
	}

	if (!ignore_pos) {
	    if (x != UIComponent::UnspecifiedPosition ) 
		{ XtSetArg (wargs[n], XmNx, x); n++; }
	    if (y != UIComponent::UnspecifiedPosition ) 
		{ XtSetArg (wargs[n], XmNy, y); n++; }
	}
	if (!ignore_size) {
	    if (width != UIComponent::UnspecifiedDimension ) 
		{ XtSetArg (wargs[n], XmNwidth, width); n++; }
	    if (height != UIComponent::UnspecifiedDimension ) 
		{ XtSetArg (wargs[n], XmNheight, height); n++; }
	}
	if ((n > 0) || (geom_str)) {
	    char geom[256];

	    boolean remanage = this->isManaged() && (geom_str != NUL(char*));
	    if (remanage) this->unmanage();
	    boolean was_managed = this->isManaged();

	    //
	    // You can always move a window if it's currently mapped.
	    // You can always set XmNgeometry if it's unrealized.
	    // For unmanaged + realized windows, you must do unrealize/realize
	    //
	    if (!was_managed) {
		XtUnrealizeWidget(this->getRootWidget());

		int minW, minH, baseW, baseH, wOffset, hOffset;
		int width_spec, height_spec;


		// The instrinsics use {min,base}{Width,Height} in order
		// to calculate the dimensions they'll really use.  The x,y,w,h specified
		// in the geom string is only a starting point.  Surprise!
		// The secret is to decrease {width,height} by the amount in the
		// resource XmN{base,min}{Width,Height}.  baseWidth takes precedence
		// over minWidth.  How about that? -Martin 4/24/95

		// The intrinsics also use XmN{width,height}Inc to interpret the
		// contents of the geometry string. Recall how xterm -geom 90x60
		// works.  That is not accounted for here, so if any dx MainWindow
		// uses XmN{width,height}Inc, there will be trouble.

		minH = minW = baseH = baseW = -1;
		XtVaGetValues (this->getRootWidget(), XmNminWidth, &minW, 
		    XmNminHeight, &minH, XmNbaseWidth, &baseW, 
		    XmNbaseHeight, &baseH, 
		NULL);

		if (baseW != -1) wOffset = baseW;
		else if (minW != -1) wOffset = minW;
		else wOffset = 0;
		if (baseH != -1) hOffset = baseH;
		else if (minH != -1) hOffset = minH;
		else hOffset = 0;

		width_spec = width-wOffset;
		height_spec = height-hOffset;

		// Don't assert because it's possible to get a negative value in the
		// following scenario:
		// Size to min{Width,Height} on 1280x1024 display, save, 
		// reload on 1024x780 screen.  The normalization arithmetic yields
		// width,height less than min{Width,Height} and you get dims < 0
		// Maybe the save routine should have checked for this case.
		//ASSERT (width_spec >= 0);
		//ASSERT (height_spec >= 0);

		// Really use 0 as a minimum value?  I don't know if wmgrs will
		// accept -geom 0x0.  But using a min of say 10, might have 
		// ramifications.
		width_spec = MAX(0,width_spec);
		height_spec = MAX(0,height_spec);

		if (this->geometry_string) {
		    strcpy (geom, this->geometry_string);
		} else if ((MainWindow::OffsetsInitialized) && (MainWindow::IsMwmBroken))
		    //
		    // Because of broken dec mwm (gda75)
		    //
		    sprintf(geom,"%dx%d+%d+%d", width_spec, height_spec, 
			x + MainWindow::WmOffsetX,
			y + MainWindow::WmOffsetY);
		else
		    sprintf(geom,"%dx%d+%d+%d", width_spec, height_spec, x,y);

		//
		// Reset n or not?  It means "do we also set x,y,width,height?" 
		// scenario: on my machines it works fine, but on alphax and irix5.2,
		// the window appears initially with its desired size and then 
		// immediately changes to 640x480.  The problem seemed to go away if
		// I went ahead and set those dimensions and it didn't seem to hurt
		// anyone else.
		//n = 0;
	        XtSetArg(wargs[n],XmNgeometry,geom); n++;
	    } else if (was_managed) {
		if (!MainWindow::OffsetsInitialized)
		    MainWindow::InitializeOffsets (this->getRootWidget());
		if (geom_str)
		    { XtSetArg (wargs[n], XmNgeometry, geom_str); n++; }
	    } else {
		// FIXME
		// We would hit this assert if called on to set only some of 
		// x,y,width,height on an unmanaged window.   The code can't
		// currently cope with this case.
		//ASSERT(0);
	    }
	    XtSetValues(this->getRootWidget(), wargs,n);
	    if (!was_managed) {
		//
		// Fiddle with the menu bar.  The problem is that if the width
		// of the window constrains the menu bar (so that you get a multi-
		// lined menubar) then when the window is rerealized, the menubar
		// tries to stretch itself out to a single line thereby widening
		// the window.
		//
		if (this->menuBar)
		    XtVaSetValues (this->menuBar,
			XmNresizeHeight, False,
			XmNresizeWidth,  False,
		    NULL);
		XtRealizeWidget(this->getRootWidget());
		if (this->menuBar) {
		    XSync (XtDisplay(this->menuBar), False);
		    XtVaSetValues (this->menuBar,
			XmNresizeHeight, True,
			XmNresizeWidth,  True,
		    NULL);
		}
	    }
	    if (remanage) this->manage();
	}
    }
}

//
// Set the name used in the window icon.
// This will not take effect until the window has been realized, but is
// intended to be called by the derived class during initialization 
// (i.e. createWorkArea()).
//
void MainWindow::setIconName(const char *name)
{
   if (this->getRootWidget()) 
       XtVaSetValues(this->getRootWidget(),XmNiconName,name,NULL);
}
// 
// The default action for a close and popdown.
//
void MainWindow::popdownCallback() { } 
void MainWindow::closeWindow()   { this->unmanage(); }


//
// Set up special data items used in window placement.  Note that as of 10/95
// dec mwm seemed broken for both osf2 and osf3.  The problem: XmIsMotifWMRunning
// would always return false (regardless of host).  The other problem: -geom
// and XmNgeometry are broken.
//
void MainWindow::InitializeOffsets( Widget shell )
{
Window root;
unsigned int nkids;
Window win = XtWindow(shell);
Window *kids, parent;
Display *d = XtDisplay(shell);
unsigned int junk;
XTextProperty tprop;

    //
    // X{t}TranslateCoords won't work here.
    // Must get parent of win, then get parent's x,y coords.  Those
    // are the values for WmOffset{x,y}
    //
    XQueryTree (d, win, &root, &parent, &kids, &nkids);
    MainWindow::IsMwmRunning = XmIsMotifWMRunning(shell);
    if (!MainWindow::IsMwmRunning) {
	MainWindow::IsMwmBroken = FALSE;
	Atom mexists = XInternAtom (d, "_MOTIF_WM_INFO", True);
	if (mexists != None) {
	    if (XGetTextProperty (d, root, &tprop, mexists)) {
		MainWindow::IsMwmBroken = TRUE;
	    }
	}
	if (MainWindow::IsMwmBroken) {
	    MainWindow::IsMwmRunning = True;
	} 
    } else {
	Atom sgiexists = XInternAtom (d, "_SGI__MENU_WINDOW", True);
	if (sgiexists != None) {
	    if (XGetTextProperty (d, root, &tprop, sgiexists)) {
		MainWindow::IsMwmBroken = TRUE;
	    }
	} else
	    MainWindow::IsMwmBroken = False;
    }

    if (kids) XFree(kids);
    if (MainWindow::IsMwmRunning) {
	Atom vueexists = XInternAtom (d, "_VUE_SM_WINDOW_INFO", True);
	if (vueexists != None) {
	    MainWindow::WmOffsetX = MainWindow::WmOffsetY = 0;
	} else {
	    XGetGeometry (d, parent, &root, &MainWindow::WmOffsetX, 
		&MainWindow::WmOffsetY, &junk, &junk, &junk, &junk);
	}
    } else if (parent != root) {
	XTextProperty tprop;
	boolean probablyOlwm = FALSE;
	Atom olwm = XInternAtom (d, "_SUN_WM_PROTOCOLS", True);
	if (olwm != None) {
	    if (XGetTextProperty (d, root, &tprop, olwm)) {
		probablyOlwm = TRUE;
	    }
	}
	//
	// could be twm | olwm
	if (probablyOlwm)
	    XGetGeometry (d, win, &root, &MainWindow::WmOffsetX, 
		&MainWindow::WmOffsetY, &junk, &junk, &junk, &junk);
	else
	    MainWindow::WmOffsetX = MainWindow::WmOffsetY = 0;
    } else {
	//
	// non-reparenting window mgr.  Never tested because
	// I couldn't find a copy of uwm.  Did test absence 
	// of window manager.
	MainWindow::WmOffsetX = MainWindow::WmOffsetY = 0;
    }

    MainWindow::OffsetsInitialized = TRUE;
}



//
// resource naming:
//	DX.filebasenamesansext.windowtitle.geometry:
//	DX.filebasenamesansext.windowtitle%d.geometry:
//
const char* MainWindow::getGeometryString()
{
int i;
Widget w = NUL(Widget);
Widget root = NUL(Widget);
int nxtalt = 0;
#define MAXNM 20

    if (this->geometry_string) return this->geometry_string;

    Widget *leaves =  new Widget[MAXNM];
    Widget parent = theApplication->getRootWidget();

    //
    // Create a temporary widget hierarchy so that resource name
    // matching works.  After fetching the resources, we'll destroy
    // the widgets.
    //
    String names[MAXNM];
    int count = 0;
    this->getGeometryNameHierarchy(names, &count, MAXNM);
    for (i=0; i<count; i++) {
	char* name = names[i];
	if (i == 0)
	    root = w = XtCreatePopupShell (name, 
		topLevelShellWidgetClass, parent, NULL, 0);
	else
	    w = XtCreateWidget (name, xmRowColumnWidgetClass, parent, NULL, 0);
	parent = w;
	delete[] name;
    }
    if (i == 0) {
	delete[] leaves;
	return NUL(char*);
    }

    //
    // Create siblings so that alternate name matching is provided
    //
    leaves[nxtalt++] = w;
    parent = XtParent(w);
    String anames[MAXNM];
    count = 0;
    this->getGeometryAlternateNames(anames, &count, MAXNM);
    for (i=0; i<count; i++) {
	char* name = anames[i];
	w = XtCreateWidget (name, xmRowColumnWidgetClass, parent, NULL, 0);
	leaves[nxtalt++] = w;
	delete[] name;
    }

    XtResource xtres;
    xtres.resource_name = "geometry";
    xtres.resource_class = "Geometry";
    xtres.resource_type = XmRString;
    xtres.resource_size = sizeof(String);
    xtres.resource_offset = 0;
    xtres.default_type = XmRString;
    xtres.default_addr = NULL;

    char* resvalue = NUL(char*);
    for (i=0; i<nxtalt; i++) {
	XtGetSubresources (XtParent(leaves[i]), &resvalue, XtName(leaves[i]), 
	    this->getClassName(), &xtres, 1, NUL(ArgList), 0);
	if ((resvalue) && (resvalue[0])) break;
    }
    ASSERT(root);
    XtDestroyWidget(root);
    delete[] leaves; leaves = NULL;

    if (!resvalue) return NUL(char*);
    this->geometry_string = DuplicateString(resvalue);
    return this->geometry_string;
}

void MainWindow::getGeometryNameHierarchy(String names[], int* count, int max)
{
    int cnt = *count;
    if (cnt >= (max-1)) return ;

    const char* title = this->getWindowTitle();
    char* resvalue = NUL(char*);
    //
    // Let's see if we can find out what the title is going to be
    //
    if (!title) {
	XtResource xtres;
	xtres.resource_name = "title";
	xtres.resource_class = "Title";
	xtres.resource_type = XmRString;
	xtres.resource_size = sizeof(String);
	xtres.resource_offset = 0;
	xtres.default_type = XmRString;
	xtres.default_addr = NULL;

	XtGetSubresources (theApplication->getRootWidget(), &resvalue, 
	    this->name, this->getClassName(), &xtres, 1, NUL(ArgList), 0);
    }
    if ((resvalue) && (resvalue[0])) 
	title = resvalue;
    if (!title) title = this->name;

    char* tmp;
    tmp = DuplicateString(title);
    int len = strlen(tmp);
    int i;
    for (i=0; i<len; i++) if (tmp[i]==' ') tmp[i] = '_';
    names[cnt++] = tmp; 
    *count = cnt;
}

void MainWindow::getGeometryAlternateNames(String*, int*, int) { }
