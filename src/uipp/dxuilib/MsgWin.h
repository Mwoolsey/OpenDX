/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _MsgWin_h
#define _MsgWin_h


#include "DXWindow.h"
#include "List.h"


//
// Class name definition:
//
#define ClassMsgWin	"MsgWin"

//
// XtCallbackProc (*CB), XtEventHandler (*EH) and XtActionProc (*AP)
// DialogCallback (*DCB), XtInputCallbackProc (*ICP), XtWorkProc (*WP)
// functions for this and derived classes
//
extern "C" void MsgWin_SelectCB(Widget, XtPointer, XtPointer);
extern "C" void MsgWin_FlushTimeoutTO(XtPointer closure, XtIntervalId*);

class Dialog;
class Command;
class CommandInterface;
class MWFileDialog;
class ToggleButtonInterface;
class Network;

struct SelectableLine {
    int position;
    char *line;
};

//
// MsgWin class definition:
//				
class MsgWin : public DXWindow
{
  private:
    //
    // Private member data:
    //
    static boolean ClassInitialized;
    friend void MsgWin_SelectCB(Widget, XtPointer, XtPointer);

    void clearSelectableLines();

    static void ShowEditor (XtPointer);
    static void NoShowEditor (XtPointer);

    //
    // If there is an editor opened on the net, or if there is a resource
    // in DXApplication which says to open an editor on the net...
    // If the resource says NEVER but the window is already opened, then
    // ignore the resource.
    // If the application indicates we should prompt the user and the promptUser
    // flag is set, then prompt the user with a Yes/No dialog about opening
    // the editor.
    //
    // Return TRUE, if we posted a dialog to ask the user if we should post
    // the window.
    //
    boolean openEditorIfNecessary(Network *net,
                                const char *nodeName, int inst,
                                boolean promptUser);

  protected:
    //
    // Protected member data:
    //
    List selectableLines;
    Widget list;
    boolean firstMsg;
    boolean executing;
    boolean beenManaged;
    boolean beenBeeped;

    char *logFileName;
    FILE *logFile;


    MWFileDialog *logDialog;
    MWFileDialog *saveDialog;
    Dialog	*execCommandDialog;

    Widget fileMenu;
    Widget fileMenuPulldown;
    Command *clearCmd;
    Command *logCmd;
    Command *saveCmd;
    Command *closeCmd;
    CommandInterface *clearOption;
    ToggleButtonInterface *logOption;
    CommandInterface *saveOption;
    CommandInterface *closeOption;

    Widget editMenu;
    Widget commandsMenu;
    Widget editMenuPulldown;
    Command *nextErrorCmd;
    Command *prevErrorCmd;
    CommandInterface *nextErrorOption; 
    CommandInterface *prevErrorOption;

    Widget optionsMenu;
    Widget optionsMenuPulldown;
    CommandInterface *infoOption; 
    CommandInterface *warningOption; 
    CommandInterface *errorOption; 
    CommandInterface *execScriptOption; 
    ToggleButtonInterface *traceOption; 
    CommandInterface *memoryOption; 
    Command *execScriptCmd;
    Command *traceCmd;
    Command *memoryCmd;

    static String DefaultResources[];

    //
    // functions to set up menus and the window itself.
    //
    virtual Widget createWorkArea(Widget parent);
    virtual void createMenus(Widget parent);
    virtual void createFileMenu(Widget parent);
    virtual void createEditMenu(Widget parent);
    virtual void createOptionsMenu(Widget parent);
            void createCommandsMenu(Widget parent);

    //
    // functions to control the message window.  When the system
    // is executing, the line is actually added to batchedLines, instead
    // of being dumped to the window, and a timer is started.  After the
    // timer goes off or when execution ends, flushBuffer is called to put
    // the messages up.  flushBuffer should remove the timeout if intervalId
    // isn't 0.
    virtual void flushBuffer();
    friend void MsgWin_FlushTimeoutTO(XtPointer, XtIntervalId*);
    XtIntervalId intervalId;
    List batchedLines;

    //
    // A function to select a selectable line.
    //
    virtual void selectLine(int, boolean promptUser=TRUE);

    //
    // Install the default resources for this class and then call the
    // same super class method to get the default resources from the
    // super classes.
    //
    virtual void installDefaultResources(Widget baseWidget);

  public:
    //
    // Constructor:
    //
    MsgWin();

    //
    // Destructor:
    //
    ~MsgWin();

    virtual void addError(const char *);
    virtual void addWarning(const char *);
    virtual void addInformation(const char *);
    virtual void infoOpen();

    virtual void beginExecution();
    virtual void endExecution();
    virtual void standBy();

    virtual boolean clear();
    virtual boolean log(const char *);
    virtual boolean save(const char*);

    virtual void postLogDialog();
    virtual void postSaveDialog();
    virtual void postExecCommandDialog();

    FILE    *getLogFile() { return this->logFile; }
    Command *getLogCommand() { return this->logCmd; }
    Command *getSaveCommand() { return this->saveCmd; }
    Command *getClearCommand() { return this->clearCmd; }
    Command *getNextErrorCommand() { return this->nextErrorCmd; }
    Command *getPrevErrorCommand() { return this->prevErrorCmd; }
    ToggleButtonInterface *getLogInterface() { return this->logOption; }

    //
    // activateSameSelection will be passed to XmListSelectPos().  When findNextError
    // is called from code (rather than from a user's menu selection), 
    // activateSameSelection will be set FALSE so that a MsgWin whose selection is 
    // already at the end of the list, won't auto-popup and bring up its associated vpe.
    //
    virtual void findNextError(boolean activateSameSelection = TRUE);
    virtual void findPrevError();

    //
    // override the manage function to know the difference between never
    // managed and closed.
    //
    virtual void manage();

    //
    // Automatically issue script commands turn turn tracing on/off
    //
    boolean toggleTracing();

    //
    // Usage("memory",0); script command
    //
    boolean memoryUse();

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassMsgWin;
    }
};


#endif // _MsgWin_h
