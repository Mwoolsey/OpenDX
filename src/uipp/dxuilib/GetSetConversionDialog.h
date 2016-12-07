/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#ifndef _GetSetConversionDialog_h
#define _GetSetConversionDialog_h


#include "Dialog.h"
#include <X11/cursorfont.h>

//
// Class name definition:
//
#define ClassGetSetConversionDialog	"GetSetConversionDialog"

//
//
extern "C" void GetSetConversionDialog_PushbuttonCB(Widget, XtPointer, XtPointer);
extern "C" void GetSetConversionDialog_SelectCB(Widget, XtPointer, XtPointer);
extern "C" void GetSetConversionDialog_BrowseCB(Widget, XtPointer, XtPointer);
extern "C" Boolean EditorWindow_GetSetWP(XtPointer);

class List;
class Network;
class EditorWindow;
class ButtonInterface;
class Command;
class GlobalLocalNode;


//
// GetSetConversionDialog class definition:
//				

class GetSetConversionDialog : public Dialog
{
    friend class EditorWindow;

  private:
    //
    // Private member data:
    //
    static boolean ClassInitialized;
    static String  DefaultResources[];
    static Cursor  WatchCursor;

    List *referenced_macros;

    Widget list; 
    Widget find_next_btn, find_all_btn;
    Widget net_name, net_name_label;

    void layoutButtons (Widget parent);
    void layoutChooser (Widget parent);
    void layoutControls (Widget parent);

    ButtonInterface *global_option;
    ButtonInterface *local_option;

    int next_get_instance;
    int next_set_instance;

    boolean list_includes_main;
    boolean quiet_mode;

    void setFindButton (boolean first, boolean done);

    static char *GetFileName(Network *);

  protected:
    //
    // Protected member data:
    //
    Widget createDialog(Widget);

    //
    // Install the default resources for this class and then call the
    // same super class method to get the default resources from the
    // super classes.
    //
    virtual void installDefaultResources(Widget baseWidget);

    friend void GetSetConversionDialog_PushbuttonCB(Widget, XtPointer, XtPointer);
    friend void GetSetConversionDialog_SelectCB(Widget, XtPointer, XtPointer);
    friend void GetSetConversionDialog_BrowseCB(Widget, XtPointer, XtPointer);
    friend Boolean EditorWindow_GetSetWP(XtPointer);

    void update();
    void unhinge(boolean select_next_editor = FALSE);
    void updateNetName(Network *);

    static void GetSetPlacements
	(Network *topnet, List* maclist, List **gets, List **sets);

    void selectNextNode();
    void selectPartner(GlobalLocalNode*, EditorWindow*);
    void selectAllNodes();

  public:

    //
    // Constructor:
    //
    GetSetConversionDialog();

    virtual void post();

    EditorWindow *getActiveEditor();
    void setActiveEditor(EditorWindow*);
    void setCommandActivation();
    void notifyTitleChange(EditorWindow* ew, const char *title);

    //
    // Destructor:
    //
    ~GetSetConversionDialog();

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassGetSetConversionDialog;
    }
};


#endif // _GetSetConversionDialog_h
