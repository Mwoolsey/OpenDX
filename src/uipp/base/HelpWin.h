/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>

#ifndef _HelpWin_h
#define _HelpWin_h

#include "MainWindow.h"
#include "Dictionary.h"

//
// Class name definition:
//
#define ClassHelpWin	"HelpWin"

//
// XtCallbackProc (*CB), XtEventHandler (*EH) and XtActionProc (*AP)
// DialogCallback (*DCB) functions for this and derived classes
//
extern "C" void HelpWin_SelectCB(Widget, XtPointer, XtPointer);

class Command;
class CommandInterface;

extern "C" { const char *GetHelpDirectory(); }
extern "C" { const char *GetHelpDirFileName(); }
extern "C" { const char *GetHTMLDirectory(); }
extern "C" { const char *GetHTMLDirFileName(); }


//
// HelpWin class definition:
//				
class HelpWin : public MainWindow
{
  friend const char *GetHelpDirectory();
  friend const char *GetHelpDirFileName();
  friend const char *GetHTMLDirectory();
  friend const char *GetHTMLDirFileName();
  private:
    //
    // Private member data:
    //
    static boolean UseWebBrowser;
    static boolean ClassInitialized;
    friend void HelpWin_SelectCB(Widget, XtPointer, XtPointer);

    //
    // Encapsulate initialization for multiple constructors
    //
    void init();

  protected:
    //
    // Protected member data:
    //
    static String DefaultResources[];

    Command *closeCmd;
    CommandInterface *closeOption;

    Widget historyPopup;
    Widget fileMenu;
    Widget fileMenuPulldown;
    Widget multiText;

    Dictionary topicToFileMap;

    //
    // functions to set up menus and the window itself.
    //
    virtual Widget createWorkArea(Widget parent);

    //
    // Constructor for derived classes:
    //
    HelpWin(const char *name, boolean hasMenuBar);

    //
    // Install the default resources for this class and then call the
    // same super class method to get the default resources from the
    // super classes.
    //
    virtual void installDefaultResources(Widget baseWidget);

  public:
    //
    // Constructor for instances of this class:
    //
    HelpWin();

    //
    // Destructor:
    //
    ~HelpWin();

    virtual void initialize();

    virtual void helpOn(const char *topic);

    void loadTopicFile(const char *topic, const char *file);

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassHelpWin;
    }

};



#endif // _HelpWin_h
