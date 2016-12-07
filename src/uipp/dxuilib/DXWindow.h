/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



// DXWindow.h -								    //
//                                                                          //
// Definition for the DXWindow class.					    //
//                                                                          //
// DXWindow class is a subclass of IBMMainWindow, which implements an 
// optional menu bar with an optional anchor pixmap on the left side of the 
// menu bar if the window is an "anchor" window and a Help menu pulldown.  
// The subclasses are expected to create the menus via createMenus() call.
//                                                                          //

#ifndef _DXWindow_h
#define _DXWindow_h


#include "List.h"
#include "IBMMainWindow.h"

#define ClassDXWindow "DXWindow"

extern "C" void DXWindow_ConnectionMenuMapCB(Widget    widget,
                              XtPointer clientData,
                              XtPointer callData);
extern "C" void DXWindow_FileHistoryMenuMapCB(Widget, XtPointer , XtPointer );

class Command;
class CommandInterface;
class ToggleButtonInterface;
class ControlPanelAccessDialog;
class PanelAccessManager;
class CascadeMenu;

//
// DXWindow class definition:
//				
class DXWindow : public IBMMainWindow
{
  private:
    friend void DXWindow_ConnectionMenuMapCB(Widget    widget,
                              XtPointer clientData,
                              XtPointer callData);
    friend void DXWindow_FileHistoryMenuMapCB(Widget, XtPointer , XtPointer );

  protected:
    //
    // Protected member data:
    //

    //
    // These resources are expected to be loaded by the derived classes.
    //
    static String DefaultResources[];

    Widget  anchorButton;	// optional first menu item

    boolean startup;		// Is this supposed to be opened at startup. 
    boolean anchor;		// is this an anchor window?
    Pixmap  anchorPixmap;	// anchor pixmap
 
    // 
    //  Execute option pulldown support
    // 
    Widget              executeMenu;
    Widget              executeMenuPulldown;
    CommandInterface*   executeOnceOption;
    CommandInterface*   executeOnChangeOption;
    CommandInterface*   endExecutionOption;
    CommandInterface*   sequencerOption;

    // 
    //  Connect option pulldown support
    // 
    Widget              connectionMenu;
    Widget              connectionMenuPulldown;
    CommandInterface*   startServerOption;
    CommandInterface*   disconnectFromServerOption;
    CommandInterface*   resetServerOption;
    CommandInterface*	processGroupAssignmentOption;

    //
    // Options menu
    //
    Command		*toggleWindowStartupCmd;    
    ToggleButtonInterface *toggleWindowStartupOption;

    // 
    //  Adds help option pulldown support
    // 
    CommandInterface	*helpTutorialOption;

    //
    // Override IBMMainWindow to put anchor up
    //
    virtual void createMenuBar(Widget parent);
    
    //
    // These two routines provide the basic methods for build the execute
    // and Help menu pulldowns, but can be overridden by the derived classes.
    //
    virtual void createHelpMenu(Widget parent);
    virtual void createExecuteMenu(Widget parent);
    virtual void createConnectionMenu(Widget parent);

    //
    // Children should implement these virtual functions to hightlight their
    // "Execute" menu pull down if the have one.
    //
    virtual void beginExecution();
    virtual void standBy();
    virtual void endExecution();
    virtual void serverDisconnected();

    //
    // Virtual function called at the beginning/end of Command::execute 
    //
    virtual void beginCommandExecuting();
    virtual void endCommandExecuting();

    //
    // Remember the last message notified so that a new window can get
    // notified at the creation time.
    //
    static Symbol lastMsg;
    static const void *lastMsgData;

    //
    // Not every window uses this, but enough do that we put it here.
    //
    ControlPanelAccessDialog	*panelAccessDialog;

    //
    // Add a toggle button interface that toggles the startup up state of this
    // window to the given parent widget.
    //
    Widget addStartupToggleOption(Widget parent);

    //
    // Change whether or not this window is an anchor window.
    // Update any visual state or anything else that is required.  At this
    // level we handle the anchor pixmap if initialized.
    //
    virtual void setAnchor(boolean isAnchor);
    //
    // Create the anchor button widget and add the pixmap if desired and not
    // already present
    //
    void createAnchor(boolean addPixmap);

    //
    // Constructor for the subclasses:
    //
    DXWindow(const char*   name,
	     boolean isAnchor, 
	     boolean usesMenuBar = TRUE);

    //
    // Install the default resources for this class and then call the
    // same super class method to get the default resources from the
    // super classes.
    //
    virtual void installDefaultResources(Widget baseWidget);

    //
    // Allow subclasses to supply a string for the XmNgeometry resource
    // (i.e. -geam 90x60-0-0) which we'll use instead of createX,createY,
    // createWidth, createHeight when making the new window.  If the string
    // is available then initialize() won't call setGeometry.
    //
    virtual void getGeometryNameHierarchy(String*, int*, int);
    virtual void getGeometryAlternateNames(String*, int*, int);

    //
    // build a file history button and menu
    //
    virtual void createFileHistoryMenu (Widget parent);
    virtual void buildFileHistoryMenu();
    CascadeMenu* file_history_cascade;
    List file_history_buttons;
    List file_history_commands;

  public:

    //
    // Destructor:
    //
    ~DXWindow();

    //
    // Implementation of notify() function for this class:
    //
    virtual void notify
	(const Symbol message, const void *msgdata=NULL, const char *msg=NULL);

    boolean isAnchor() { return this->anchor; }

    //
    // Post the panel access dialog with this window PanelAccessManager info.
    // If pam is not NULL, use it when creating the dialog otherwise use
    // this->panelAccessManager.
    // Not every window uses this, but enough do that we put it here.
    //
    void postPanelAccessDialog(PanelAccessManager *pam);

    //
    // Overrides the MainWindow class manage() function.
    //  Calls the superclass manage() first then finishes building the
    //  anchor symbol.
    //
    virtual void manage();

    virtual boolean printComment(FILE *f);
    virtual boolean parseComment(const char *line, const char *file,
                                int lineno); 
    //
    // Reset the window to use the default settings for the state that is
    // printed by the printComment() method.
    //
    virtual void useDefaultCommentState();

    //
    // Determine if this window is a startup window.
    //
    boolean isStartup() { return this->startup; }
    //
    // Virtual because SequencerWindow must set a startup flag in his node
    // at the same time.
    //
    virtual void setStartup(boolean set = TRUE);

    //
    // Called by the MainWindow CloseCallback
    //
    virtual void closeWindow();

    //
    // Changes whether or not this window is supposed to open up automatically 
    // on startup.
    //
    virtual void toggleWindowStartup();

    const char * getClassName() { return ClassDXWindow; }
};


#endif // _DXWindow_h
