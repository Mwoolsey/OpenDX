/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>



#ifndef _StartupApplication_h
#define _StartupApplication_h

#if defined(HAVE_SSTREAM)
#include <sstream>
#elif defined(HAVE_STRSTREAM_H)
#include <strstream.h>
#elif defined(HAVE_STRSTREA_H)
#include <strstrea.h>
#endif

#include <Xm/Xm.h>

#include "../base/MainWindow.h"
#include "../base/Dialog.h"
#include "../base/IBMApplication.h"
#if !defined(DXD_OS_NON_UNIX) && defined(DXD_LICENSED_VERSION)
#include "../base/TemporaryLicense.h"
#endif
#include "../base/List.h"


class StartupWindow;
class Command;

//
// Class name definition:
//
#define ClassStartupApplication	"StartupApplication"

//
// XtCallbackProc (*CB), XtEventHandler (*EH) and XtActionProc (*AP)
// DialogCallback (*DCB), XtInputCallbackProc (*ICP), XtWorkProc (*WP)
// functions for this and derived classes
//
extern "C" void StartupApplication_XtWarningHandler(char*);
extern "C" void StartupApplication_HelpCB(Widget, XtPointer, XtPointer);
extern "C" int	StartupApplication_XErrorHandler(Display *display, 
						XErrorEvent *event);


#if defined(DXD_LICENSED_VERSION)
class StartupApplication : public IBMApplication, public TemporaryLicense
#else
class StartupApplication : public IBMApplication
#endif
{

  private:
    //
    // Private class data:
    //
    static boolean    StartupApplicationClassInitialized; // class initialized?
    friend void StartupApplication_XtWarningHandler(char *message);
    friend int	      StartupApplication_XErrorHandler(Display *display, 
						XErrorEvent *event);

    void shutdownApplication() {};

    StartupWindow	*mainWindow;

    List                dumpedObjects;

    List		argList;

    friend void StartupApplication_HelpCB(Widget, XtPointer , XtPointer);

    void destroyDumpedObjects();

  protected:
    //
    // Overrides the Application class version:
    //   Initializes Xt Intrinsics with option list (switches).
    //
    virtual boolean initialize(unsigned int* argcp, char**argv);

    CommandScope       *commandScope;   // command scope

    virtual const char* getRootDir() { return this->getUIRoot(); }

  public:

    void postStartupWindow();
    StartupWindow *getMainWindow(){return (StartupWindow *)this->mainWindow;};


    static void addHelpCallbacks(Widget);

    List * getCommandLineArgs() { return &this->argList; }

    //
    // Constructor:
    //
    StartupApplication(char* className);

    //
    // Destructor:
    //
    ~StartupApplication();

    //
    // Return the name of the application (i.e. 'Data Explorer',
    // 'Data Prompter', 'Medical Visualizer'...).
    //
    virtual const char *getInformalName() { return "Startup"; }

    //
    // Return the formal name of the application (i.e.
    // 'Open Visualization Data Explorer', 'Open Visualization Data Prompter'...)
    //
    virtual const char *getFormalName() { return "Open Visualization Startup"; }

    //
    // Get the applications copyright notice, for example...
    // "Copyright International Business Machines Corporation 1991-1993
    // All rights reserved"
    //
    virtual const char *getCopyrightNotice();


    void dumpObject(Base *object) { this->dumpedObjects.appendElement((void*)object); }

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
        return ClassStartupApplication;
    }
};


extern StartupApplication* theStartupApplication;

#endif /*  _StartupApplication_h */

