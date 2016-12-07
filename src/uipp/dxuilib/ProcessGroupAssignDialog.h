/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#ifndef _ProcessGroupAssignDialog_h
#define _ProcessGroupAssignDialog_h


#include "Dialog.h"
#include "ProcessGroupOptionsDialog.h"

class DXApplication;
class List;
class Dictionary;
class DXPacketIF;

class ProcessGroupOptionsDialog;

//
// Class name definition:
//
#define ClassProcessGroupAssignDialog     "ProcessGroupAssignDialog"

//
// XtCallbackProc (*CB), XtEventHandler (*EH) and XtActionProc (*AP)
// DialogCallback (*DCB), XtInputCallbackProc (*ICP), XtWorkProc (*WP)
// functions for this and derived classes
//
extern "C" void ProcessGroupAssignDialog_ListButtonDownEH(Widget, XtPointer, XEvent*, Boolean*);
extern "C" void ProcessGroupAssignDialog_DefaultActionCB(Widget, XtPointer, XtPointer);
extern "C" void ProcessGroupAssignDialog_TextCB(Widget, XtPointer, XtPointer);
extern "C" void ProcessGroupAssignDialog_SelectListCB(Widget, XtPointer, XtPointer);
extern "C" void ProcessGroupAssignDialog_PopdownCB(Widget, XtPointer, XtPointer);
extern "C" void ProcessGroupAssignDialog_OptionCB(Widget, XtPointer, XtPointer);

class ProcessGroupAssignDialog : public Dialog
{
  //friend class ProcessGroupCreateDialog;
  friend class ProcessGroupOptionsDialog;

  private:
    //
    // Private member data:
    //
    static boolean ClassInitialized;
    static String  DefaultResources[];

    boolean dirty;
    boolean i_caused_it;

    DXApplication *app;
    ProcessGroupOptionsDialog *pgod;

    List	groupList;

    Widget	grouplist, hostlist;
    Widget	hosttext, hostlabel;
    Widget	option_pb;

    int	   lastIndex;

    void   makeList(int item);
    void   addCallbacks();

    virtual boolean okCallback(Dialog*);
 
  protected:
    //
    // Protected member data:
    //
    friend void ProcessGroupAssignDialog_OptionCB(Widget, XtPointer, XtPointer);
    friend void ProcessGroupAssignDialog_PopdownCB(Widget, XtPointer, XtPointer);
    friend void ProcessGroupAssignDialog_SelectListCB(Widget, XtPointer, XtPointer);
    friend void ProcessGroupAssignDialog_TextCB(Widget, XtPointer, XtPointer);
    friend void ProcessGroupAssignDialog_ListButtonDownEH(Widget widget, XtPointer clientData, 
			       XEvent *event, Boolean *ctd);
    // deal with the double clicking
    friend void ProcessGroupAssignDialog_DefaultActionCB(Widget, XtPointer, XtPointer);


    virtual Widget createDialog(Widget parent);

    //
    // Constructor: for derived classes.
    //
    ProcessGroupAssignDialog(const char *name, DXApplication*); 

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
    ProcessGroupAssignDialog(DXApplication*); 

    //
    // Destructor:
    //
    ~ProcessGroupAssignDialog();

    void refreshDisplay() { this->makeList(0); }
    virtual void manage();

};

#endif // _ProcessGroupAssignDialog_h

