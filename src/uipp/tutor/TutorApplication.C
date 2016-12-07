/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <Xm/Xm.h>
#include <Xm/Label.h>
#include <X11/cursorfont.h>
#if 0
#if defined(HAVE_STREAM_H)
#include <stream.h>
#endif

#include <unistd.h>
#include <signal.h>
#include <errno.h>
#endif

#include "DXStrings.h"
#include "MainWindow.h"
#include "TutorApplication.h"
#include "NoUndoTutorAppCommand.h"
#include "TutorWindow.h"
#include "CommandScope.h"

#if (XmVersion > 1001)
# include "../widgets/Number.h"
# define XK_MISCELLANY
# include <X11/keysym.h>
#endif

#ifdef APP_HAS_TUTOR_DBASE
#include "Dictionary.h"
#endif

TutorApplication* theTutorApplication= NUL(TutorApplication*);

boolean    TutorApplication::TutorApplicationClassInitialized = FALSE;
TutorResource TutorApplication::resource;

static
XrmOptionDescRec _TutorOptionList[] =
{
    {
	"-tutorFile",
	"*tutorFile",
	XrmoptionSepArg,
	NULL
    },
    {
        "-uiroot",
        "*UIRoot",
        XrmoptionSepArg,
        NULL
    }
};


static
XtResource _TutorResourceList[] =
{
    {
        "tutorFile",
        "Pathname",
        XmRString,
        sizeof(String),
        XtOffset(TutorResource*, tutorFile),
        XmRString,
        (XtPointer)"Tutorial"
    },
    {
	"TutorInsensitiveColor",
	"Color",
	XmRPixel,
	sizeof(Pixel),
	XtOffset(TutorResource*, insensitiveColor),
	XmRString,
	(XtPointer)"#888"
    },
    {
        "root",
        "Pathname",
        XmRString,
        sizeof(String),
        XtOffset(TutorResource*, UIRoot),
        XmRString,
        (XtPointer) NULL
    }
};

static
const String _defaultTutorResources[] =
{
    "*background:              #b4b4b4b4b4b4",
    "*foreground:              black",
#ifdef sgi
    "*fontList:                 -adobe-helvetica*bold-r*14*=bold\n\
                                -adobe-helvetica*medium-r*14*=normal\n\
                                -adobe-helvetica*medium-o*14*=oblique\n",
#else
    "*fontList:                 -adobe-helvetica*bold-r*14*=bold\
                                -adobe-helvetica*medium-r*14*=normal\
                                -adobe-helvetica*medium-o*14*=oblique",
#endif

    "*keyboardFocusPolicy:     explicit",
    "*highlightOnEnter:	       false",
    "*highlightThickness:      0",

    "*XmNumber.navigationType:     XmTAB_GROUP",

    "*XmScrollBar*foreground:      #b4b4b4b4b4b4",
    "*XmScrollBar.initialDelay:    2000",
    "*XmScrollBar.repeatDelay:     2000",

    "*XmToggleButton.selectColor:  CadetBlue",

    "*XmArrowButton.shadowThickness:         1",
    "*XmCascadeButton.shadowThickness:       1",
    "*XmCascadeButtonGadget.shadowThickness: 1",
    "*XmColorMapEditor*shadowThickness:      1",
    "*XmDrawnButton.shadowThickness:         1",
    "*XmFrame.shadowThickness:               1",
    "*XmList.shadowThickness:                1",
    "*XmMenuBar.shadowThickness:             1",
    "*XmNumber*shadowThickness:              1",
    "*XmPushButton.shadowThickness:          1",
    "*XmRowColumn.shadowThickness:           1",
    "*XmScrollBar.shadowThickness:           1",
    "*XmScrolledList.shadowThickness:        1",
    "*XmScrolledWindow.shadowThickness:      1",
    "*XmText.shadowThickness:                1",
    "*XmToggleButton.shadowThickness:        1",

    NULL
};


TutorApplication::TutorApplication(char* className): IBMApplication(className)
{
    //
    // Set the global Tutor application pointer.
    //
    theTutorApplication = this;
    this->commandScope = new CommandScope();

    this->quitCmd = new NoUndoTutorAppCommand("quitCmd",
			this->commandScope,
			 TRUE, this, NoUndoTutorAppCommand::Quit);

}


TutorApplication::~TutorApplication()
{
    //
    // Set the flag to terminate the event processing loop.
    //
    theTutorApplication= NULL;
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

boolean TutorApplication::initialize(unsigned int* argcp,
			       char**        argv)
{
    ASSERT(argcp);
    ASSERT(argv);

    //
    // Initialize Xt Intrinsics; create the initial shell widget.
    //
    if (!this->IBMApplication::initializeWindowSystem (argcp, argv))
	return FALSE;
    if (!this->IBMApplication::initialize (argcp, argv))
	return FALSE;

    InitializeSignals();
    this->parseCommand(argcp, argv, _TutorOptionList, XtNumber(_TutorOptionList));


    //
    // Get and save the X display structure pointer.
    //
    this->display = XtDisplay(this->getRootWidget());

    //
    // Center the shell and make sure it is not visible.
    //
    XtVaSetValues
	(this->getRootWidget(),
	 XmNmappedWhenManaged, FALSE,
	 XmNx,                 DisplayWidth(this->display, 0) / 2,
	 XmNy,                 DisplayHeight(this->display, 0) / 2,
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

    this->setDefaultResources(this->getRootWidget(), _defaultTutorResources);	

    //
    // Get application resources.
    //
    if (NOT TutorApplication::TutorApplicationClassInitialized)
    {
	this->getResources((XtPointer)&TutorApplication::resource,
			   _TutorResourceList,
			   XtNumber(_TutorResourceList));
	TutorApplication::TutorApplicationClassInitialized = TRUE;
    }

    //
    // setup resources that can be environment varialbles.
    //
    if (TutorApplication::resource.UIRoot == NULL) {
        char *s = getenv("DXROOT");
        if (s)
            // POSIX says we better copy the result of getenv(), so...
            // This will show up as a memory leak, not worth worrying about
            TutorApplication::resource.UIRoot = DuplicateString(s);
        else
            TutorApplication::resource.UIRoot =  "/usr/local/dx";
    }

    this->mainWindow = new TutorWindow();
    this->mainWindow->manage();

    this->setBusyCursor(TRUE);

    //
    // Load the initial tutorial page. 
    //
    this->helpOn(TutorApplication::resource.tutorFile);

    //
    // Refresh the screen.
    //
    XmUpdateDisplay(this->getRootWidget());


    this->setBusyCursor(FALSE);

    return TRUE;
}
const char *TutorApplication::getFormalName()
{
    return "Open Visualization Data Explorer Tutorial";
}

const char *TutorApplication::getInformalName()
{
    return "DX Tutorial";
}

const char *TutorApplication::getCopyrightNotice()
{
#if 1
    return NULL;	// Kills copyright message.
#else

    return "";
#endif

}



void TutorApplication::handleEvents()
{
    XEvent event;


    //
    // Process events while the application is running.
    //
    while (TRUE)
    {
	XtAppNextEvent(this->applicationContext, &event);
	this->handleEvent(&event);
    }
}


extern "C" void TutorApplication_XtWarningHandler(char *message)
{
    if(theTutorApplication)
    	theTutorApplication->handleXtWarning(message);
}
void TutorApplication::handleXtWarning(char *message)
{
   if(strstr(message, "non-existant accelerators"))
	return;

   fprintf(stderr, "XtWarning: %s\n", message);

}

extern "C" int TutorApplication_XErrorHandler(Display *display, XErrorEvent *event)
{
    return theTutorApplication->handleXError(display, event);
}

int TutorApplication::handleXError(Display *display, XErrorEvent *event)
{
    char buffer[BUFSIZ];
    char mesg[BUFSIZ];
    char number[32];
    XGetErrorText(display, event->error_code, buffer, BUFSIZ);
    XGetErrorDatabaseText(display, "XlibMessage", "XError",
	"X Error", mesg, BUFSIZ);
    fprintf(stderr, "%s:  %s\n", mesg, buffer);
    XGetErrorDatabaseText(display, "XlibMessage", "MajorCode",
	"Request Major code %d", mesg, BUFSIZ);
    fprintf(stderr, mesg, event->request_code);
    if (event->request_code < 128) {
        sprintf(number, "%d", event->request_code);
        XGetErrorDatabaseText(display, "XRequest", number, "", buffer, BUFSIZ);
    } else {
	sprintf(buffer, "Extension %d", event->request_code);
    }
    fprintf(stderr, " (%s)\n  ", buffer);
    XGetErrorDatabaseText(display, "XlibMessage", "MinorCode",
	"Request Minor code %d", mesg, BUFSIZ);
    fprintf(stderr, mesg, event->minor_code);
    if (event->request_code >= 128) {
        sprintf(mesg, "Extension %d.%d",
	    event->request_code, event->minor_code);
        XGetErrorDatabaseText(display, "XRequest", mesg, "", buffer, BUFSIZ);
        fprintf(stderr, " (%s)", buffer);
    }
    fputs("\n  ", stderr);
    XGetErrorDatabaseText(display, "XlibMessage", "ResourceID",
	"ResourceID 0x%x", mesg, BUFSIZ);
    fprintf(stderr, mesg, event->resourceid);
    fputs("\n  ", stderr);
    XGetErrorDatabaseText(display, "XlibMessage", "ErrorSerial",
	"Error Serial #%d", mesg, BUFSIZ);
    fprintf(stderr, mesg, event->serial);
    fputs("\n  ", stderr);

#if defined(XlibSpecificationRelease) && XlibSpecificationRelease <= 4
    // R5 does not allow one to get at display->request.
    XGetErrorDatabaseText(display, "XlibMessage", "CurrentSerial",
        "Current Serial #%d", mesg, BUFSIZ);
    fprintf(stderr, mesg, display->request);
    fputs("\n", stderr);
#endif

    if (event->error_code == BadImplementation) return 0;

    return 1;
}

const char *TutorApplication::getHelpDirectory()
{
    static char *buf = NULL;
    if (!buf) {
        const char *root = theTutorApplication->resource.UIRoot;
	int len = STRLEN(root);	
	buf = new char[len + 8];
	sprintf(buf,"%s/help",root);
    }
   return buf;
}

void TutorApplication::helpOn(const char *topic)
{
    ASSERT(this->mainWindow);
    this->mainWindow->helpOn(topic);
}

