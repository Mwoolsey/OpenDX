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

#include <sys/types.h>
#include <sys/stat.h>

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif
#include <signal.h>
#include <errno.h>
#include <stdlib.h>

#include "../base/DXStrings.h"
#include "../base/MainWindow.h"
#include "../base/WarningDialogManager.h"
#include "../base/HelpWin.h"
#include "GARApplication.h"
#include "GARMainWindow.h"
#include "GARChooserWindow.h"
#include "ListIterator.h"

#include "../base/CommandScope.h"
#include "NoUndoGARAppCommand.h"

#include "../widgets/Number.h"
#define XK_MISCELLANY
#include <X11/keysym.h>

#ifndef DXD_NON_UNIX_ENV_SEPARATOR
#define SEP_CHAR ':'
#else
#define SEP_CHAR ';'
#endif

#if defined(HAVE_XINERAMA)
#include <X11/extensions/Xinerama.h>
#endif

GARApplication* theGARApplication = NUL(GARApplication*);

boolean    GARApplication::GARApplicationClassInitialized = FALSE;
GARResource GARApplication::resource;

static
XrmOptionDescRec _GAROptionList[] =
{
    { "-full", 	   "*GARFull", 		XrmoptionNoArg,  "True" },
    { "-help", 	   "*Help", 		XrmoptionNoArg,  "True" },
    { "-file",     "*file", 		XrmoptionSepArg,  NULL },
    { "-port", 	   "*port", 		XrmoptionSepArg,  NULL },
    { "-exec", 	   "*exec", 		XrmoptionSepArg,  NULL },
    { "-uidebug",  "*uidebug",		XrmoptionNoArg,   "True" },
    { "-data",	   "*data",		XrmoptionSepArg,  NULL },
    { "-netdir",   "*netdir", 		XrmoptionSepArg,  NULL },
    { "-limited",  "*limited",		XrmoptionNoArg,   "True" },
};


static
XtResource _GARResourceList[] =
{
    {
	"GARInsensitiveColor", "Color", XmRPixel, sizeof(Pixel),
	XtOffset(GARResource*, insensitiveColor), XmRString, (XtPointer)"#888"
    },
    {
	"GARFull", "Flag", XmRBoolean, sizeof(Boolean),
	XtOffset(GARResource*, full), XmRString, (XtPointer)"False"
    },
    {
	"Help", "Flag", XmRBoolean, sizeof(Boolean),
	XtOffset(GARResource*, help), XmRString, (XtPointer)"False"
    },
    {
	"file", "file", XmRString, sizeof(String),
	XtOffset(GARResource*, file), XmRString, (XtPointer)NULL
    },
    {
	"port", "Port", XmRInt, sizeof(int),
	XtOffset(GARResource*, port), XmRInt, (XtPointer)0
    },
    {
	"exec", "Exec", XmRString, sizeof(String),
	XtOffset(GARResource*, exec), XmRString,(XtPointer)"dx -noAnchorAtStartup -wizard"
    },
    {
	"netdir", "NetDir", XmRString, sizeof(String),
	XtOffset(GARResource*, net_dir), XmRString, 
	(XtPointer)NULL
    },
    {
	"data", "Data", XmRString, sizeof(String),
	XtOffset(GARResource*, data_file), XmRString, (XtPointer)NULL,
    },
    {
	"uidebug", "Uidebug", XmRBoolean, sizeof(Boolean),
	XtOffset(GARResource*, debugging), XmRImmediate, (XtPointer)False
    },
    {
	"limited", "Limited", XmRBoolean, sizeof(Boolean),
	XtOffset(GARResource*, limited_use), XmRImmediate, (XtPointer)False
    },
};

static
const String _defaultGARResources[] =
{
    "*background:              #b4b4b4b4b4b4",
    "*XmText.background:       #a2a2a2",
    "*XmTextField.background:  #a2a2a2",
    "*foreground:              black",

    NULL
};


GARApplication::GARApplication(char* className): IBMApplication(className)
{
    //
    // Set the global GAR application pointer.
    //
    theGARApplication = this;
    this->is_dirty = FALSE;
    this->mainWindow = NULL;
    this->chooserWindow = NULL;
    this->dataFileSource = NULL;
}


GARApplication::~GARApplication()
{
    //
    // Set the flag to terminate the event processing loop.
    //

#ifdef __PURIFY__
    if (this->chooserWindow)
      delete this->chooserWindow;
    if (this->mainWindow)
      delete this->mainWindow;
#endif


    theGARApplication = NULL;
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

    // adding a signal handler for SIGABRT does not catch us if
    // an assert fails.  The function abort does not return.
    if (!getenv ("DXUINOCATCHERROR")) {
	signal (SIGSEGV, GARApplication_HandleCoreDump);
	signal (SIGINT, GARApplication_CleanUp);
#if  !defined(DXD_WIN) && !defined(OS2)
	signal (SIGBUS, GARApplication_HandleCoreDump);
#else
	signal (SIGILL, GARApplication_HandleCoreDump);
#endif
    }
}            

boolean GARApplication::initialize(unsigned int* argcp,
			       char**        argv)
{
    ASSERT(argcp);
    ASSERT(argv);

    if (!this->IBMApplication::initializeWindowSystem(argcp,argv))
	return FALSE;

    if (!this->IBMApplication::initialize(argcp,argv))
	return FALSE;

    InitializeSignals();

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

    this->parseCommand(argcp, argv, _GAROptionList, XtNumber(_GAROptionList));
    this->setDefaultResources(this->getRootWidget(), 
				_defaultGARResources);	
    this->setDefaultResources(this->getRootWidget(), 
				IBMApplication::DefaultResources);	

    //
    // Get application resources.
    //
    if (NOT GARApplication::GARApplicationClassInitialized)
    {
	this->getResources((XtPointer)&GARApplication::resource,
			   _GARResourceList,
			   XtNumber(_GARResourceList));
	GARApplication::GARApplicationClassInitialized = TRUE;
    }

    //
    // If the user just asked for a list of command line args
    //
    if (this->resource.help) {
	printf ("dx -prompter \n");
	printf ("\t-full         :   open the full prompter\n");
	printf ("\t-help         :   print this message\n");
#if 1
	printf ("\t-uidebug      :   print messages between garui and the exec\n");
#endif
	printf ("\t-file <file>  :   open the prompter using on a .general file\n");
	printf ("\t-port <port>  :   connect to a running exec on <port>\n");
	printf ("\t-exec <cmd>   :   use <cmd> to start data explorer (default: dx -image)\n");
	printf ("\t-data <file>  :   use <file> as a data file\n");
	printf ("\t-netdir <dir> :   search <dir> for ezstart .net files (default: $DXROOT or /usr/local/dx\n");
	exit(0);
    }
    
    this->postChooserWindow();
    if (resource.full)
	this->postMainWindow((unsigned long)GMW_FULL);

    if (resource.file) {
	if (!this->mainWindow)
	    this->postMainWindow(GARMainWindow::getMode(resource.file));
	this->mainWindow->openGAR(resource.file);
    }

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

    //
    // If both -netdir and -port were specified and the netdir option
    // does not begin with '/' then issue a warning because the exec
    // was probably started somewhere else.
    //

#if !defined(DXD_OS_NON_UNIX)
    //
    // Register an input handler which will tell us if stdin goes away.
    // Then we'll just exit assuming that our startupui parent died and
    // we must die also.  In order to make this work we need a heartbeat.
    // Reason: NoStdinCB will only get called if we go into a select and
    // not if we're already there.  (I don't understand that.)  So each
    // time our heart beats, we'll basically recheck our stout
    //
    boolean startup_flag = FALSE;
    if (getenv("DXSTARTUP")) startup_flag = TRUE;
#if defined(DXD_LICENSED_VERSION)
    this->TemporaryLicense::initialize();
#endif
    if ((startup_flag) && (getenv("DXTRIALKEY"))) {
	XtAppAddInput (this->getApplicationContext(), fileno(stdin), 
	    (XtPointer)XtInputReadMask,
	    (XtInputCallbackProc)GARApplication_StdinCB, (XtPointer)this);
	XtAppAddInput (this->getApplicationContext(), fileno(stdin), 
	    (XtPointer)XtInputExceptMask,
	    (XtInputCallbackProc)GARApplication_NoStdinCB, (XtPointer)this);
	XtAppAddWorkProc (this->getApplicationContext(),  (XtWorkProc)
	    GARApplication_CheckStdoutWP, (XtPointer)this);
    }
#endif

    return TRUE;
}

void GARApplication::postChooserWindow ()
{
    if (!this->chooserWindow)
	this->chooserWindow = new GARChooserWindow;

    this->chooserWindow->manage();
}

GARMainWindow *GARApplication::getMainWindow (boolean create)
{
    if ((create == TRUE) && (this->mainWindow == NUL(GARMainWindow*))) {
	this->mainWindow = new GARMainWindow(GMW_FULL);
	this->mainWindow->initialize();
    }

    return this->mainWindow;
}

void GARApplication::postMainWindow(unsigned long mode)
{
#ifdef DEBUG
    // In general we can assert this, but in the case were a user
    // double-clicks (instead of single clicks) on the 'ok' or 'full'
    // buttons of the SelectModeDialog, we can get here twice, the
    // second time we will now won't create the window the 2nd time.
    ASSERT(this->mainWindow == NULL);
#endif
    if (!this->mainWindow) {
		this->mainWindow = new GARMainWindow(mode);
	}
    this->mainWindow->manage();
}

//
// The old mode may be the same as the new mode (if we are simply opening
// a new file.
//
void GARApplication::changeMode(unsigned long mode, std::istream *tmpstr)
{
    GARMainWindow *old_mw = NULL;
    Boolean change_it = (mode != this->mainWindow->getMode());
    if(mode & GMW_FULL && this->mainWindow->getMode() & GMW_FULL) 
	change_it = False;
    
    // Put up watch cursor
    XDefineCursor(XtDisplay(theGARApplication->getRootWidget()),
                  XtWindow(theGARApplication->getRootWidget()),
                  Application::BusyCursor);
    XFlush(XtDisplay(theGARApplication->getRootWidget()));


    if(change_it)
    {
	old_mw = this->mainWindow;
	this->mainWindow = NULL;
	this->postMainWindow(mode);
    }
    if(!this->mainWindow->openGAR(tmpstr))
	WarningMessage("Internal error: In changeMode");
    this->mainWindow->manage();

    if(change_it)
    {
	//
	// Do this after the open so the changes are'nt undone
	//
	if(old_mw->getFilename())
	    this->mainWindow->setFilename(old_mw->getFilename());
    }
    if(old_mw)
	this->dumpObject(old_mw);
    XUndefineCursor(XtDisplay(theGARApplication->getRootWidget()),
                    XtWindow(theGARApplication->getRootWidget()));
    XFlush(XtDisplay(theGARApplication->getRootWidget()));
}

//
// Return the name of the application (i.e. 'Data Explorer',
// 'Data Prompter', 'Medical Visualizer'...).
//
const char *GARApplication::getInformalName()
{
    return "Data Prompter"; 
}
//
// Return the formal name of the application (i.e.
// 'Open Visualization Data Explorer', 'Open Visualization Data Prompter'...)
//
const char *GARApplication::getFormalName()
{
   return "Open Visualization Data Prompter";
}
//
// Get the applications copyright notice, for example...
// "Copyright International Business Machines Corporation 1991-1993
// All rights reserved"
//
const char *GARApplication::getCopyrightNotice()
{
    return DXD_COPYRIGHT_STRING;
}


void GARApplication::handleEvents()
{
    XEvent event;

    //
    // Process events while the application is running.
    //
    while (True)
    {
	XtAppNextEvent(this->applicationContext, &event);
	this->handleEvent(&event);
        this->destroyDumpedObjects();
    }
}


const char *GARApplication::getHelpDirFileName()
{
    return "GARHelpDir";
}
void GARApplication::addHelpCallbacks(Widget w, const char *widget_name_key)
{

    //
    // Add the callback for this one...
    //
    XtAddCallback(w, XmNhelpCallback, (XtCallbackProc)GARApplication_HelpCB, 
	(XtPointer)widget_name_key);

    //
    // Add the callback for any descendants
    //
    if(XtIsComposite(w))
    {
	WidgetList	children;
	Cardinal	nchild;
	int		i;

	XtVaGetValues(w, XmNchildren, &children, XmNnumChildren, &nchild, NULL);
	for(i = 0; i < nchild; i++)
	{
	    GARApplication::addHelpCallbacks(children[i], widget_name_key);
	}
    }
}
extern "C" void GARApplication_HelpCB(Widget w, XtPointer clientData, XtPointer)
{
    const char *widget_name_key = (const char *)clientData;
    if (widget_name_key)
	theApplication->helpOn(widget_name_key);
    else
	theApplication->helpOn(XtName(w));
}
const char *GARApplication::getStartTutorialCommandString()
{
    // At some point we may want to define this to be the data prompter
    // specific tutorial.
    return this->IBMApplication::getStartTutorialCommandString();
}
void GARApplication::dumpObject(Base *object)
{
     this->dumpedObjects.appendElement((void*)object);
}


void GARApplication::destroyDumpedObjects()
{
     Base *object;
     ListIterator li(this->dumpedObjects);

     while(object = (Base*)li.getNext())
        delete object;

     this->dumpedObjects.clear();
}

#ifdef DXD_OS_NON_UNIX
#define ISGOOD(a) ((a&_S_IFREG)&&((a&_S_IFDIR)==0))
#else
#define ISGOOD(a) ((a&S_IFREG)&&((a&(S_IFDIR|S_IFCHR|S_IFBLK))==0))
#endif

char *GARApplication::FileFound (const char *fname, const char *ext)
{
struct STATSTRUCT statb;
char tmpstr[256];

    if ((STATFUNC(fname, &statb) != -1) && (ISGOOD(statb.st_mode)))
	return DuplicateString(fname);

    if ((ext) && (ext[0])) {
	if (ext[0] == '.')
	    sprintf (tmpstr, "%s%s", fname, ext);
	else
	    sprintf (tmpstr, "%s.%s", fname, ext);

	if ((STATFUNC(tmpstr, &statb) != -1) && (ISGOOD(statb.st_mode)))
	    return DuplicateString(tmpstr);
    }
    return NUL(char *);
}

//
//  Accept the file if it's regular AND !(directory | char spec | block spec)
//
char *GARApplication::resolvePathName(const char *str, const char *ext)
{

    //
    // File found
    //
    char *cp = GARApplication::FileFound (str, ext);
    if (cp) return cp;

    //
    // File can't be found because it begins with '/'
    //
    if (str[0] == '/') return NUL(char *);

    //
    // Try prepending the path of the header file
    //
    if (this->mainWindow) {
	char *dirname = GetDirname(this->mainWindow->getFilename());
	if(dirname) {
	    char *newstr = new char[STRLEN(str) + STRLEN(dirname) + 3];
	    strcpy(newstr, dirname);
	    strcat(newstr, "/"); 
	    strcat(newstr, str); 
	    char *cp = GARApplication::FileFound (newstr, ext);
	    delete newstr;
	    if (cp) return cp;
	}
    }
	
    //
    // Try looking in DXDATA
    //
    char *datadir = (char *)getenv("DXDATA");
    if (!datadir) return NUL(char*);

    char *newstr = new char[STRLEN(str) + STRLEN(datadir) + 2];
    char *newstrHead = newstr;
    while (datadir) {

	strcpy(newstr, datadir);
	char *cp;
	if ((cp = strchr(newstr, SEP_CHAR)) != NULL)
	    *cp = '\0';
	strcat(newstr, "/");
	strcat(newstr, str);

	cp = GARApplication::FileFound (newstr, ext);
	if (cp) {
	    delete newstrHead;
	    return cp;
	}

	datadir = strchr(datadir, SEP_CHAR);
	if (datadir)
	    datadir++;
    }
    delete newstrHead;
    return NUL(char*);
}

void GARApplication::setDirty()
{
    if (this->is_dirty) return ;
    this->is_dirty = TRUE;
    if (this->chooserWindow) this->chooserWindow->setCommandActivation();
}

void GARApplication::setClean()
{
    if (this->is_dirty == FALSE) return ;
    this->is_dirty = FALSE;
    if (this->chooserWindow) this->chooserWindow->setCommandActivation();
}


void GARApplication::destroyMainWindow()
{
    if (this->mainWindow) {
	//this->dumpedObjects.appendElement((void*)this->mainWindow);
	delete this->mainWindow;
	this->mainWindow = NUL(GARMainWindow*);
     }
}


void GARApplication::openGeneralFile(const char *filenm)
{
char *cp = DuplicateString(filenm);

    if (!this->mainWindow) 
	this->postMainWindow(GARMainWindow::getMode(cp));
    this->mainWindow->openGAR(cp);
    if (!this->mainWindow->isManaged())
	this->mainWindow->manage();

    delete cp;
}


const char *
GARApplication::getResourcesNetDir()
{
    if ((this->resource.net_dir) && (this->resource.net_dir[0]))
	return this->resource.net_dir;
    if (this->getUIRoot())
	return this->getUIRoot();
    if (getenv("DXROOT"))
	return getenv("DXROOT");
    return "/usr/local/dx";
}



// Signal handlers can be called anytime they're installed regardless of the
// lifespan of the object.  The check for !theGARApplication really means:
// am I being called after GARApplication::~GARApplication() ?
// If you're building on IRIX 6 or later, then remove the test for defined(sgi)
extern "C" void
#if defined(sun4) || defined(sgi) && !( __mips > 1)
GARApplication_HandleCoreDump(int , ... )
#else
GARApplication_HandleCoreDump(int )
#endif
{
    if (!theGARApplication) exit(1);

    fprintf (stderr, "\n%s has experienced an internal error and will terminate.\n\n",
	theApplication->getInformalName());

    fflush(stderr);
    fflush(stdout);

    exit(1);
}

extern "C" void FileContents_CleanUp();
extern "C" void
#if defined(sun4)|| defined(sgi) && !( __mips > 1)
GARApplication_CleanUp(int , ... )
#else
GARApplication_CleanUp(int )
#endif
{
    if (theGARApplication) 
	FileContents_CleanUp();
    exit(0);
}

extern "C" void
GARApplication_NoStdinCB (XtPointer , int* , XtInputId* )
{
    exit(1);
}

extern "C" void
GARApplication_StdinCB (XtPointer , int* , XtInputId* )
{
    char buf[1024];
    fread (buf, 1, 1024, stdin);
    if ((ferror(stdin)) || (feof(stdin))) {
	GARApplication_NoStdinCB (0,0,0);
    }
}

extern "C" Boolean
GARApplication_CheckStdoutWP (XtPointer cData)
{
    GARApplication* gap = (GARApplication*)cData;
    ASSERT(gap);
    XtAppAddTimeOut (gap->getApplicationContext(), (unsigned long)2000,
	(XtTimerCallbackProc)GARApplication_CheckStdoutTO, (XtPointer)gap);
    return TRUE;
}

extern "C" void
GARApplication_CheckStdoutTO (XtPointer cData, XtIntervalId* )
{
    GARApplication* gap = (GARApplication*)cData;
    ASSERT(gap);
    XtAppAddWorkProc (gap->getApplicationContext(),  (XtWorkProc)
	GARApplication_CheckStdoutWP, (XtPointer)gap);
}
