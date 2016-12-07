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

#include "DXStrings.h"
#include "MainWindow.h"
#include "MBApplication.h"
#include "MBMainWindow.h"
#include "../widgets/Number.h"
#define XK_MISCELLANY
#include <X11/keysym.h>

MBApplication* theMBApplication = NUL(MBApplication*);

boolean    MBApplication::MBApplicationClassInitialized = FALSE;
MBResource MBApplication::resource;

static
XrmOptionDescRec _MBOptionList[] =
{
    {
	"-warning",
	"*warningEnabled",
	XrmoptionSepArg,
	"False"
    },
};


static
XtResource _MBResourceList[] =
{
    {
	"MBInsensitiveColor",
	"Color",
	XmRPixel,
	sizeof(Pixel),
	XtOffset(MBResource*, insensitiveColor),
	XmRString,
	(XtPointer)"#888"
    },
};

static
const String _defaultMBResources[] =
{
    "*XmScrollBar.initialDelay:    2000",
    "*XmScrollBar.repeatDelay:     2000",

    NULL
};


MBApplication::MBApplication(char* className): IBMApplication(className)
{
    //
    // Set the global MB application pointer.
    //
    theMBApplication = this;
    theMBApplication->is_dirty = FALSE;

}


MBApplication::~MBApplication()
{
    //
    // Set the flag to terminate the event processing loop.
    //
    theMBApplication = NULL;
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

boolean MBApplication::initialize(unsigned int* argcp,
			       char**        argv)
{
    ASSERT(argcp);
    ASSERT(argv);

    if (!this->IBMApplication::initializeWindowSystem(argcp,argv))
        return FALSE;

    if (!this->IBMApplication::initialize(argcp,argv))
        return FALSE;

    InitializeSignals();

    this->parseCommand(argcp, argv, _MBOptionList, XtNumber(_MBOptionList));

    this->setDefaultResources(this->getRootWidget(), _defaultMBResources);

    this->setDefaultResources(this->getRootWidget(), 
				IBMApplication::DefaultResources);

    //
    // Get application resources.
    //
    if (NOT MBApplication::MBApplicationClassInitialized)
    {
	this->getResources((XtPointer)&MBApplication::resource,
			   _MBResourceList,
			   XtNumber(_MBResourceList));
	MBApplication::MBApplicationClassInitialized = TRUE;
    }

    this->mainWindow = new MBMainWindow();
    this->mainWindow->manage();

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

    return TRUE;
}

//
// Return the name of the application (i.e. 'Data Explorer',
// 'Data Prompter', 'Medical Visualizer'...).
//
const char *MBApplication::getInformalName()
{
    return "Module Builder";
}
//
// Return the formal name of the application (i.e.
// 'Open Visualization Data Explorer', 'Open Visualization Data Prompter'...)
//
const char *MBApplication::getFormalName()
{
   return "Open Visualization Module Builder";
}
//
// Get the applications copyright notice, for example...
// "Copyright International Business Machines Corporation 1991-1993
// All rights reserved"
//
const char *MBApplication::getCopyrightNotice()
{
    return DXD_COPYRIGHT_STRING;
}

const char *MBApplication::getHelpDirFileName()
{
    return "MBHelpDir";
}

const char *MBApplication::getStartTutorialCommandString()
{
    // At some point we may want to define this to be the module builder 
    // specific tutorial.
    return this->IBMApplication::getStartTutorialCommandString();
}

