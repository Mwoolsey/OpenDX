/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>



#ifndef _TutorApplication_h
#define _TutorApplication_h

//#include <Xm/Xm.h>

//#include "TutorWindow.h"
#include "IBMApplication.h"

class Command;
class CommandScope;
class TutorWindow;
class Dictionary;

//
// Class name definition:
//
#define ClassTutorApplication	"TutorApplication"

//
// XtCallbackProc (*CB), XtEventHandler (*EH) and XtActionProc (*AP)
// DialogCallback (*DCB), XtInputCallbackProc (*ICP), XtWorkProc (*WP)
// functions for this and derived classes
//
extern "C" void TutorApplication_XtWarningHandler(char*);
extern "C" int	TutorApplication_XErrorHandler(Display *display, XErrorEvent *event);


typedef struct
{
    Pixel	insensitiveColor;
    String 	UIRoot;	
    String 	tutorFile;	
} TutorResource;

class TutorApplication : public IBMApplication
{
    friend class TutorWindow;

  private:
    //
    // Private class data:
    //
    static boolean    TutorApplicationClassInitialized; // class initialized?
    friend void TutorApplication_XtWarningHandler(char *message);
    friend int	      TutorApplication_XErrorHandler(Display *display, XErrorEvent *event);

    void shutdownApplication() {};


  protected:

    CommandScope *commandScope;
    Command	*quitCmd;
    Command	*gotoHelpTextCmd;
 
    //
    // Overrides the Application class version:
    //   Initializes Xt Intrinsics with option list (switches).
    //
    virtual boolean initialize(unsigned int* argcp,
			    char**        argv);

    //
    // Handle Xt Warnings (called by TutorApplication_XtWarningHandler, static, above)
    // Handle X Errors (called by XErrorHandler, static, above)
    virtual void handleXtWarning(char *message);
    virtual int  handleXError(Display *display, XErrorEvent *event);

    //
    // Override of superclass handleEvents() function:
    //   Handles application events.
    //   This routine should be called by main() only.
    //
    virtual void handleEvents();

    static TutorResource	resource;

    void loadTutorDbase();

  public:


    //
    // Constructor:
    //
    TutorApplication(char* className);

    //
    // Destructor:
    //
    ~TutorApplication();

    TutorWindow	*mainWindow;

    virtual const char *getFormalName();
    virtual const char *getInformalName();
    virtual const char *getCopyrightNotice();

    virtual const char *getHelpDirectory();
    virtual void helpOn(const char *topic);

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
        return ClassTutorApplication;
    }
};


extern TutorApplication* theTutorApplication;

#endif /*  _TutorApplication_h */

