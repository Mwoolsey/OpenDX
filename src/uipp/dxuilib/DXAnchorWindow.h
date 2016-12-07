/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#ifndef _DXANCHOR_WINDOW_H
#define _DXANCHOR_WINDOW_H 1

#include "DXWindow.h"
#include <Xm/XmStrDefs.h> // for XmSTRING_DEFAULT_CHARSET

#define ClassDXAnchorWindow "DXAnchorWindow"


class CommandInterface;
class CascadeMenu;
class Command;
class CommandScope;

extern "C" void DXAnchorWindow_FileMenuMapCB(Widget, XtPointer, XtPointer);
extern "C" void DXAnchorWindow_LogoCB(Widget, XtPointer, XtPointer);
extern "C" void DXAnchorWindow_WindowsMenuMapCB(Widget, XtPointer, XtPointer);
extern "C" void DXAnchorWindow_TimeoutTO (XtPointer , XtIntervalId*);


class NoUndoAnchorCommand;

class DXAnchorWindow: public DXWindow {

    friend class NoUndoAnchorCommand;

  // P R I V A T E   P R I V A T E   P R I V A T E   
  // P R I V A T E   P R I V A T E   P R I V A T E   
  private:

    static boolean ClassInitialized;

    friend void DXAnchorWindow_FileMenuMapCB(Widget, XtPointer, XtPointer);
    friend void DXAnchorWindow_LogoCB(Widget, XtPointer, XtPointer);
    friend void DXAnchorWindow_WindowsMenuMapCB(Widget, XtPointer, XtPointer);
    friend void DXAnchorWindow_TimeoutTO (XtPointer , XtIntervalId*);

    Widget form,label,logoButton,logoMessage;

    XtIntervalId timeoutId;

    static void LabelExtent (Widget, int*, int*);

  // P R O T E C T E D    P R O T E C T E D    P R O T E C T E D    
  // P R O T E C T E D    P R O T E C T E D    P R O T E C T E D    
  protected:

    static String DefaultResources[];

    virtual Widget createWorkArea    (Widget parent);
    virtual void   createMenus       (Widget parent);
    virtual void   createFileMenu    (Widget parent);
    virtual void   createWindowsMenu (Widget parent);
    virtual void   createHelpMenu(Widget parent);


    //
    // Menus & pulldowns:
    //
    Widget              fileMenu;
    Widget              fileMenuPulldown;
    Widget              windowsMenu;
    Widget              windowsMenuPulldown;
 
    //
    // File menu options:
    //
    CommandInterface*   openOption;
    CommandInterface*   loadMacroOption;
    CommandInterface*   loadMDFOption;
    CommandInterface*   saveOption;
    CommandInterface*   saveAsOption;
    CommandInterface*   closeOption;
    CascadeMenu*	settingsCascade;
    CommandInterface*   saveCfgOption;
    CommandInterface*   openCfgOption;
    Command*		closeCmd;

    //
    // Window menu options:
    //
    CommandInterface*   openAllControlPanelsOption;
    CascadeMenu         *openControlPanelByNameMenu;
    CommandInterface*   openAllColormapEditorsOption;
    CommandInterface*   messageWindowOption;
    CommandInterface*   openVPEOption;
    Command*		openVPECmd;

    //
    // Help menu options:
    //
    CommandInterface*   onVisualProgramOption;

  // P U B L I C   P U B L I C   P U B L I C   P U B L I C   
  // P U B L I C   P U B L I C   P U B L I C   P U B L I C   
    //
    // Install the default resources for this class and then call the
    // same super class method to get the default resources from the
    // super classes.
    //
    virtual void installDefaultResources(Widget baseWidget);

    //
    // build a file history button and menu
    //
    virtual void createFileHistoryMenu (Widget parent);

  public:

    boolean 		postVPE();

    //
    // This window will post a copyright notice inside itself as a substitute
    // for the one supplied by {IBM}Application.
    //
    void		postCopyrightNotice();
    void		deleteCopyright();
    void		setKioskMessage (const char* msg, 
			  const char *font = XmSTRING_DEFAULT_CHARSET);

    //
    // Adjust the name of the editor window based on the current network name.
    //
    void resetWindowTitle();

    DXAnchorWindow (const char *name, boolean isAnchor, boolean usesMenuBar = TRUE);
    ~DXAnchorWindow();

    const char *getClassName() { return ClassDXAnchorWindow; }
};

#endif // _DXANCHOR_WINDOW_H
