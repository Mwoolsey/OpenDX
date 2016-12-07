/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#ifndef _ControlPanelAccessDialog_h
#define _ControlPanelAccessDialog_h


#include "Dialog.h"

//
// Class name definition:
//
#define ClassControlPanelAccessDialog	"ControlPanelAccessDialog"

//
// XtCallbackProc (*CB), XtEventHandler (*EH) and XtActionProc (*AP)
// DialogCallback (*DCB), XtInputCallbackProc (*ICP), XtWorkProc (*WP)
// functions for this and derived classes
//
extern "C" void ControlPanelAccessDialog_OpenPanelCB(Widget, XtPointer, XtPointer);

class PanelAccessManager;

//
// ControlPanelAccessDialog class definition:
//				
class ControlPanelAccessDialog : public Dialog
{
  private:
    //
    // Private member data:
    //
    static boolean ClassInitialized;
    static String  DefaultResources[];

    Widget shell;
    Widget mainform;
    Widget sform;
    Widget separator;

    List   toggleList[3];

    PanelAccessManager* panelManager;
 
  protected:
    //
    // Protected member data:
    //
    virtual Widget createDialog(Widget);
    virtual boolean okCallback(Dialog*);

    friend void ControlPanelAccessDialog_OpenPanelCB(Widget, XtPointer , XtPointer);
    void   makeToggles();
   
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
    ControlPanelAccessDialog(Widget parent,PanelAccessManager* pam);
    //
    // Destructor:
    //
    ~ControlPanelAccessDialog();

    void manage();
    void update();

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassControlPanelAccessDialog;
    }
};


#endif // _ControlPanelAccessDialog_h
