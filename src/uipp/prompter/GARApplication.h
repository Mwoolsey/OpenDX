/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#ifndef _GARApplication_h
#define _GARApplication_h

#if defined(HAVE_SSTREAM)
#include <sstream>
#elif defined(HAVE_STRSTREAM_H)
#include <strstream.h>
#elif defined(HAVE_STRSTREA_H)
#include <strstrea.h>
#endif

#if 0
#if defined(cygwin)
#include <g++/iostream.h>
#endif
#endif

#include <Xm/Xm.h>

#include "../base/MainWindow.h"
#include "../base/Dialog.h"
#include "../base/IBMApplication.h"
#include "../base/List.h"
#if !defined(DXD_OS_NON_UNIX) && defined(DXD_LICENSED_VERSION)
#include "../base/TemporaryLicense.h"
#endif


class GARMainWindow;
class GARChooserWindow;
class Command;
class GridChoice;

//
// Class name definition:
//
#define ClassGARApplication	"GARApplication"

//
// XtCallbackProc (*CB), XtEventHandler (*EH) and XtActionProc (*AP)
// DialogCallback (*DCB), XtInputCallbackProc (*ICP), XtWorkProc (*WP)
// functions for this and derived classes
//
extern "C" void GARApplication_XtWarningHandler(char*);
extern "C" void GARApplication_HelpCB(Widget, XtPointer, XtPointer);
extern "C" int	GARApplication_XErrorHandler(Display *display, 
						XErrorEvent *event);
extern "C" void GARApplication_NoStdinCB(XtPointer, int*, XtInputId*);
extern "C" void GARApplication_StdinCB(XtPointer, int*, XtInputId*);
extern "C" Boolean GARApplication_CheckStdoutWP (XtPointer );
extern "C" void GARApplication_CheckStdoutTO (XtPointer , XtIntervalId* );


// If you're building on IRIX 6 or later, then remove the test for defined(sgi)
#if defined(sun4) || defined(sgi) && !( __mips > 1)
extern "C" void GARApplication_HandleCoreDump(int dummy, ... );
extern "C" void GARApplication_CleanUp(int dummy, ... );
#else
extern "C" void GARApplication_HandleCoreDump(int dummy);
extern "C" void GARApplication_CleanUp(int dummy);
#endif


typedef struct
{
    Pixel	insensitiveColor;
    Boolean	full;
    Boolean	help;
    String	file;
    int		port;
    String	exec;
    String	net_dir;
    Boolean	debugging;
    String	data_file;
    Boolean	limited_use;
} GARResource;

#if !defined(DXD_OS_NON_UNIX) && defined(DXD_LICENSED_VERSION)
class GARApplication : public IBMApplication, public TemporaryLicense
#else
class GARApplication : public IBMApplication
#endif
{

  private:
    //
    // Private class data:
    //
    static boolean    GARApplicationClassInitialized; // class initialized?
    friend void GARApplication_XtWarningHandler(char *message);
    friend int	      GARApplication_XErrorHandler(Display *display, 
						XErrorEvent *event);
    friend Boolean GARApplication_CheckStdoutWP (XtPointer );
    friend void GARApplication_CheckStdoutTO (XtPointer , XtIntervalId* );

    friend class GARChooserWindow;
    friend class GridChoice;

    void shutdownApplication() {};

    GARMainWindow	*mainWindow;
    GARChooserWindow	*chooserWindow;

    boolean		is_dirty;
    List                dumpedObjects;

    friend void GARApplication_HelpCB(Widget, XtPointer , XtPointer);

    void destroyDumpedObjects();
    void *dataFileSource;

    void destroyMainWindow();

    void	openGeneralFile(const char *);

  protected:
    //
    // Overrides the Application class version:
    //   Initializes Xt Intrinsics with option list (switches).
    //
    virtual boolean initialize(unsigned int* argcp,
			    char**        argv);

    CommandScope       *commandScope;   // command scope

    //
    //   Handles application events.
    //   This routine should be called by main() only.
    //
    virtual void handleEvents();

    static GARResource	resource;

    virtual const char* getRootDir() { return this->getUIRoot(); }

  public:

    static char *FileFound (const char *, const char *);

    void postChooserWindow();
    void postMainWindow(unsigned long);
    GARMainWindow *getMainWindow(boolean create = FALSE);
    GARChooserWindow *getChooserWindow() { return this->chooserWindow; }
    char *resolvePathName(const char *, const char *);

    void *getDataFileSource() { return this->dataFileSource; }
    void  setDataFileSource(void *source) { this->dataFileSource = source; }

    boolean isDirty(){return this->is_dirty;}
    void setDirty();
    void setClean();

    void changeMode(unsigned long, std::istream *);

    const char *getResourcesExec() { return this->resource.exec; }
    int	        getResourcesPort() { return this->resource.port; }
    const char *getResourcesNetDir();
    boolean     getResourcesDebug(){ return this->resource.debugging; }
    const char *getResourcesData() { return this->resource.data_file; }

#if !defined(DXD_OS_NON_UNIX) && defined(DXD_LICENSED_VERSION)
    const char *getResourcesDxuiArgs() { 
	return (this->resource.limited_use?this->getLimitedArgs():NUL(char*));
    }
#endif

    Pixel getInsensitiveColor()
    {
	return this->resource.insensitiveColor;
    }

    static void addHelpCallbacks(Widget, const char *widget_name_key=NULL);

    virtual const char *getHelpDirFileName();

    //
    // Constructor:
    //
    GARApplication(char* className);

    //
    // Destructor:
    //
    ~GARApplication();

    //
    // Return the name of the application (i.e. 'Data Explorer',
    // 'Data Prompter', 'Medical Visualizer'...).
    //
    virtual const char *getInformalName();

    //
    // Return the formal name of the application (i.e.
    // 'Open Visualization Data Explorer', 'Open Visualization Data Prompter'...)
    //
    virtual const char *getFormalName();

    //
    // Get the applications copyright notice, for example...
    // "Copyright International Business Machines Corporation 1991-1993
    // All rights reserved"
    //
    virtual const char *getCopyrightNotice();

    const char *getStartTutorialCommandString();

    void dumpObject(Base *object);

    virtual boolean inWizardMode() { return TRUE; }

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
        return ClassGARApplication;
    }
};


extern GARApplication* theGARApplication;

#endif /*  _GARApplication_h */

