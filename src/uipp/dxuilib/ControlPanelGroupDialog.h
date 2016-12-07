/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#ifndef _ControlPanelGroupDialog_h
#define _ControlPanelGroupDialog_h


#include "Dialog.h"
#include "EditorWindow.h"

//
// Class name definition:
//
#define ClassControlPanelGroupDialog	"ControlPanelGroupDialog"

//
// XtCallbackProc (*CB), XtEventHandler (*EH) and XtActionProc (*AP)
// DialogCallback (*DCB), XtInputCallbackProc (*ICP), XtWorkProc (*WP)
// functions for this and derived classes
//
extern "C" void ControlPanelGroupDialog_TextVerifyCB(Widget, XtPointer, XtPointer);
extern "C" void ControlPanelGroupDialog_OpenPanelCB(Widget, XtPointer, XtPointer);
extern "C" void ControlPanelGroupDialog_SelectCB(Widget, XtPointer, XtPointer);
extern "C" void ControlPanelGroupDialog_CloseCB(Widget, XtPointer, XtPointer);
extern "C" void ControlPanelGroupDialog_ChangeCB(Widget, XtPointer, XtPointer);
extern "C" void ControlPanelGroupDialog_DeleteCB(Widget, XtPointer, XtPointer);
extern "C" void ControlPanelGroupDialog_AddCB(Widget, XtPointer, XtPointer);

class PanelGroupManager;

//
// ControlPanelGroupDialog class definition:
//				
class ControlPanelGroupDialog : public Dialog
{
  friend  void EditorWindow::notifyCPChange(boolean);

  private:
    //
    // Private member data:
    //
    static boolean ClassInitialized;
    static String  DefaultResources[];

    int    lastIndex;

    Widget sform;
    Widget addbtn;
    Widget deletebtn;
    Widget changebtn;
    Widget text;
    Widget list;

    List   toggleList[2];

    PanelGroupManager* panelManager;
 
  protected:
    //
    // Protected member data:
    //
    friend void ControlPanelGroupDialog_AddCB(Widget, XtPointer , XtPointer);
    friend void ControlPanelGroupDialog_DeleteCB(Widget, XtPointer , XtPointer);
    friend void ControlPanelGroupDialog_ChangeCB(Widget, XtPointer , XtPointer);
    friend void ControlPanelGroupDialog_CloseCB(Widget, XtPointer , XtPointer);
    friend void ControlPanelGroupDialog_SelectCB(Widget, XtPointer , XtPointer);
    friend void ControlPanelGroupDialog_OpenPanelCB(Widget, XtPointer , XtPointer);
    friend void ControlPanelGroupDialog_TextVerifyCB(Widget, XtPointer , XtPointer);

    Widget createDialog(Widget);
    void   makeToggles();
    void   setToggles();
    void   makeGroupList(int item = 0);
   
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
    ControlPanelGroupDialog(Widget parent);
    //
    // Destructor:
    //
    ~ControlPanelGroupDialog();

    void manage();

    //
    // Set user data.
    //
    void setData(PanelGroupManager*);

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassControlPanelGroupDialog;
    }
};

extern ControlPanelGroupDialog* theCPGroupDialog;
extern void SetControlPanelGroup(PanelGroupManager*);

#endif // _ControlPanelGroupDialog_h
