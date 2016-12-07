/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
#ifndef _StartupWindow_h
#define _StartupWindow_h

#include <dxconfig.h>
#include "../base/defines.h"



#include <dxl.h>
#include "IBMMainWindow.h"

//
// Class name definition:
//
#define ClassStartupWindow	"StartupWindow"

//
//
class CommandInterface;
class TimedInfoDialog;
class NetFileDialog;

extern "C" void StartupWindow_BrokenConnCB(DXLConnection*, void*);
extern "C" void StartupWindow_ErrorMsgCB(DXLConnection*, const char*, const void*);
extern "C" void StartupWindow_RefreshTO(XtPointer, XtIntervalId*);

#if !defined(DXD_OS_NON_UNIX)
extern "C" void StartupWindow_ResetGAR_CB (XtPointer , int*, XtInputId*);
extern "C" void StartupWindow_ReadGAR_CB (XtPointer , int*, XtInputId*);
extern "C" void StartupWindow_ResetTutor_CB (XtPointer , int*, XtInputId*);
extern "C" void StartupWindow_ReadTutor_CB (XtPointer , int*, XtInputId*);
extern "C" void StartupWindow_WaitPidTO(XtPointer, XtIntervalId*);
#endif


//
// StartupWindow class definition:
//				

class StartupWindow : public IBMMainWindow
{

  private:
    //
    // Private member data:
    //
    static boolean ClassInitialized;
    static String  DefaultResources[];

#if !defined(DXD_OS_NON_UNIX)
    //
    // Unix uses popen, pclose, fork, exec, pipe.  The pc uses spawnlp
    //
    static FILE*   TutorConn;
    static XtInputId GarErrorId;
    static XtInputId TutorErrorId;
    static XtInputId GarReadId;
    static XtInputId TutorReadId;
    static int	     GarPid;
#endif

    void        postTimedDialog(const char *msg, int millis);
    boolean	entering_image_mode;
    boolean	started_in_image_mode;
    char*	loaded_file;

    NetFileDialog* netFileDialog;
    NetFileDialog* demoNetFileDialog;

    static DXLConnection* Connection;
    static char* PendingCommand;
    static char* PendingNetFile;

    friend void StartupWindow_BrokenConnCB(DXLConnection*, void*);
    friend void StartupWindow_ErrorMsgCB(DXLConnection*, const char*, const void*);
    friend void StartupWindow_RefreshTO(XtPointer, XtIntervalId*);

#if !defined(DXD_OS_NON_UNIX)
    friend void StartupWindow_ResetGAR_CB (XtPointer , int*, XtInputId*);
    friend void StartupWindow_ReadGAR_CB (XtPointer , int*, XtInputId*);
    friend void StartupWindow_ResetTutor_CB (XtPointer , int*, XtInputId*);
    friend void StartupWindow_ReadTutor_CB (XtPointer , int*, XtInputId*);
    friend void StartupWindow_WaitPidTO(XtPointer, XtIntervalId*);
    void resetTutorConnection();
    void resetGarConnection();
#endif

    XtIntervalId command_timer;


  protected:
    //
    // Install the default resources for this class and then call the
    // same super class method to get the default resources from the
    // super classes.
    //
    virtual void installDefaultResources(Widget baseWidget);

    virtual Widget createWorkArea (Widget parent);
    virtual Widget createCommandArea (Widget parent);

    char *formCommand (const char *cmd, char **args, int nargs, boolean enc = TRUE);

    CommandInterface *startImageOption;
    CommandInterface *startPrompterOption;
    CommandInterface *startEditorOption;
    CommandInterface *emptyVPEOption;
    CommandInterface *startTutorialOption;
    CommandInterface *startDemoOption;
    CommandInterface *quitOption;
    CommandInterface *helpOption;

    Command* startImageCmd;
    Command* startPrompterCmd;
    Command* startEditorCmd;
    Command* emptyVPECmd;
    Command* startTutorialCmd;
    Command* startDemoCmd;
    Command* quitCmd;
    Command* helpCmd;

  public:
    boolean startImage();
    boolean startPrompter();
    boolean startEditor();
    boolean startTutorial();
    boolean startDemo();
    boolean startEmptyVPE();
    boolean quit();
    boolean help();
    void    startNetFile (const char *);


    //
    // Constructor:
    //
    StartupWindow();

    //
    // Destructor:
    //
    ~StartupWindow();

    virtual void closeWindow();

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName() { return ClassStartupWindow; }
};


#endif // _StartupWindow_h
