
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
#include <Xm/Label.h>
#include <X11/cursorfont.h>


#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif
#include <signal.h>
#include <errno.h>
#include <stdlib.h>

#include "DXStrings.h"
#include "MainWindow.h"
#include "HelpWin.h"
#include "StartupApplication.h"
#include "StartupWindow.h"
#include "ListIterator.h"

#if defined(HAVE_XINERAMA)
#include <X11/extensions/Xinerama.h>
#endif

#include "../base/CommandScope.h"

StartupApplication* theStartupApplication = NUL(StartupApplication*);

boolean    StartupApplication::StartupApplicationClassInitialized = FALSE;

//
// A set of resources which will be ignored.  Reason:  don't want to accidentally
// pass on something like -tutorial when the intent is to start an editor.
//
static
XrmOptionDescRec OptionList[] = {
    { "-edit", 		"*edit", 	XrmoptionNoArg, "" },
    { "-startup", 	"*startup", 	XrmoptionNoArg, "" },
    { "-tutorial", 	"*tutorial", 	XrmoptionNoArg, "" },
    { "-image", 	"*anchorMode", 	XrmoptionNoArg, "" },
    { "-prompter", 	"*prompter", 	XrmoptionNoArg, "" },
    { "-kiosk"	, 	"*anchorMode", 	XrmoptionNoArg, "" },
    { "-menuBar", 	"*anchorMode", 	XrmoptionNoArg, "" },
};

static
const String _defaultStartupResources[] =
{
    "*background:              #b4b4b4b4b4b4",
    "*foreground:              black",

    NULL
};


StartupApplication::StartupApplication(char* className): IBMApplication(className)
{
    //
    // Set the global application pointer.
    //
    theStartupApplication = this;
    this->mainWindow = NULL;

}


StartupApplication::~StartupApplication()
{
    //
    // Set the flag to terminate the event processing loop.
    //

#ifdef __PURIFY__
    if (this->mainWindow)
      delete this->mainWindow;
#endif


    theStartupApplication = NULL;
}


#if defined (SIGDANGER)
extern "C" {
static void
SigDangerHandler(int dummy)
{
    char *msg = 
#if defined(ibm6000)
    "AIX has notified Data Explorer that the User Interface\nis in"
    " danger of being killed due to insufficient page space.\n";
#else        
    "The operating system has issued a SIGDANGER to the User Interface\n";
#endif       
    write(2, msg, strlen(msg));
    signal(SIGDANGER, SigDangerHandler);
}            
}
#endif
 
static void 
InitializeSignals(void)
{            
#if defined(SIGDANGER)
    signal(SIGDANGER, SigDangerHandler);
#endif       
}            

boolean StartupApplication::initialize(unsigned int* argcp,
			       char**        argv)
{
    ASSERT(argcp);
    ASSERT(argv);

    if (!this->IBMApplication::initializeWindowSystem(argcp,argv))
	return FALSE;

    if (!this->IBMApplication::initialize(argcp,argv))
	return FALSE;

    InitializeSignals();

    this->parseCommand (argcp, argv, OptionList, XtNumber(OptionList));

    //
    // Copy all command line arguments in order to pass them to children
    //
    int i;
    this->argList.clear();
    int n = *argcp;
    for (i=1; i<n; i++) {
	char *cp = DuplicateString (argv[i]);
	this->argList.appendElement((void*)cp);
    }

    //
    // Add Application specific actions.
    //
    this->addActions();

	int xmnx=0, xmny=0;

#if defined(HAVE_XINERAMA)
	// Do some Xinerama Magic if Xinerama is present to
	// center the shell on the largest screen.
	int dummy_a, dummy_b;
	int screens;
	XineramaScreenInfo   *screeninfo = NULL;
	int x=0, y=0, width=0, height=0;
	
	if ((XineramaQueryExtension (this->display, &dummy_a, &dummy_b)) &&
		(screeninfo = XineramaQueryScreens(this->display, &screens))) {
		// Xinerama Detected
		
		if (XineramaIsActive(this->display)) {
			int i = dummy_a;
			while ( i < screens ) {
				if(screeninfo[i].width > width) {
					width = screeninfo[i].width;
					height = screeninfo[i].height;
					x = screeninfo[i].x_org;
					y = screeninfo[i].y_org;
					xmnx = x + width / 2;
					xmny = y + height / 2;
				}
				i++;
			}
		}
	}
	
#endif

	if(xmnx == 0 || xmny == 0) {
		xmnx = DisplayWidth(this->display, 0) / 2;
		xmny = DisplayHeight(this->display, 0) / 2;
	}

    //
    // Center the shell and make sure it is not visible.
    //
    XtVaSetValues
	(this->getRootWidget(),
	 XmNmappedWhenManaged, FALSE,
	 XmNx,                 xmnx,
	 XmNy,                 xmny,
	 XmNwidth,             1,
	 XmNheight,            1,
	 NULL);

    //
    // Since the instance name of this object was set in the UIComponent
    // constructor before the name of the program was visible, delete the
    // old name and set it to argv[0].
    //
    delete this->name;
    this->name = DuplicateString(argv[0]);

    //
    //
    // Force the initial shell window to exist so dialogs popped up
    // from this shell behave correctly.
    //
    XtRealizeWidget(this->getRootWidget());

    // Create the busy status indicator cursor.
    //
    Application::BusyCursor = XCreateFontCursor(this->display, XC_watch);

    this->setDefaultResources(this->getRootWidget(), 
				_defaultStartupResources);	
    this->setDefaultResources(this->getRootWidget(), 
				IBMApplication::DefaultResources);	

    //
    // Get application resources.
    //
    if (NOT StartupApplication::StartupApplicationClassInitialized)
    {
	StartupApplication::StartupApplicationClassInitialized = TRUE;
    }

    this->postStartupWindow();

    this->setBusyCursor(TRUE);

    //
    // Refresh the screen.
    //
    XmUpdateDisplay(this->getRootWidget());

    //
    // Post the copyright message.
    //
    this->postCopyrightNotice();

    this->setBusyCursor(FALSE);

#if !defined(DXD_OS_NON_UNIX)
    if (this->mainWindow) this->mainWindow->deactivate();
#if defined(DXD_LICENSED_VERSION)
    this->TemporaryLicense::initialize();
#endif
    if (this->mainWindow) this->mainWindow->activate();
#endif

    return TRUE;
}

void StartupApplication::postStartupWindow()
{

    this->mainWindow = new StartupWindow;

	int xmnx=0, xmny=0;

#if defined(HAVE_XINERAMA)
	// Do some Xinerama Magic if Xinerama is present to
	// center the shell on the largest screen.
	int dummy_a, dummy_b;
	int screens;
	XineramaScreenInfo   *screeninfo = NULL;
	int x=0, y=0, width=0, height=0;
	
	if ((XineramaQueryExtension (this->display, &dummy_a, &dummy_b)) &&
		(screeninfo = XineramaQueryScreens(this->display, &screens))) {
		// Xinerama Detected
		
		if (XineramaIsActive(this->display)) {
			int i = dummy_a;
			while ( i < screens ) {
				if(screeninfo[i].width > width) {
					width = screeninfo[i].width;
					height = screeninfo[i].height;
					x = screeninfo[i].x_org;
					y = screeninfo[i].y_org;
					xmnx = x + width / 5;
					xmny = y + height / 10;
				}
				i++;
			}
		}
	}
	
#endif

	if(xmnx == 0 || xmny == 0) {
		xmnx = DisplayWidth(this->display, 0) / 5;
		xmny = DisplayHeight(this->display, 0) / 10;
	}

    //
    // Center the shell and make sure it is not visible.
    //
    int cx, cy, cw, ch;
    this->mainWindow->initialize();
    this->mainWindow->getGeometry(&cx, &cy, &cw, &ch);
   	this->mainWindow->setGeometry(xmnx, xmny, cw, ch);
    this->mainWindow->manage();
    
}

//
// Get the applications copyright notice, for example...
// "Copyright International Business Machines Corporation 1991-1993
// All rights reserved"
//
const char *StartupApplication::getCopyrightNotice()
{
    return DXD_COPYRIGHT_STRING;
}


void StartupApplication::destroyDumpedObjects()
{
     Base *object;
     ListIterator li(this->dumpedObjects);

     while(object = (Base*)li.getNext())
        delete object;

     this->dumpedObjects.clear();
}


