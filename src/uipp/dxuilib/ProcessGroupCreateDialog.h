/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#ifndef _ProcessGroupCreateDialog_h
#define _ProcessGroupCreateDialog_h


#include "Dialog.h"

class EditorWindow;
class List;

//
// Class name definition:
//
#define ClassProcessGroupCreateDialog     "ProcessGroupCreateDialog"

//
// XtCallbackProc (*CB), XtEventHandler (*EH) and XtActionProc (*AP)
// DialogCallback (*DCB), XtInputCallbackProc (*ICP), XtWorkProc (*WP)
// functions for this and derived classes
//
extern "C" void ProcessGroupCreateDialog_TextChangeCB(Widget, XtPointer, XtPointer);
extern "C" void ProcessGroupCreateDialog_TextVerifyCB(Widget, XtPointer, XtPointer);
extern "C" void ProcessGroupCreateDialog_SelectCB(Widget, XtPointer, XtPointer);
extern "C" void ProcessGroupCreateDialog_ShowCB(Widget, XtPointer, XtPointer);
extern "C" void ProcessGroupCreateDialog_CloseCB(Widget, XtPointer, XtPointer);
extern "C" void ProcessGroupCreateDialog_DeleteCB(Widget, XtPointer, XtPointer);
extern "C" void ProcessGroupCreateDialog_RemoveCB(Widget, XtPointer, XtPointer);
extern "C" void ProcessGroupCreateDialog_CreateCB(Widget, XtPointer, XtPointer);
extern "C" void ProcessGroupCreateDialog_AddCB(Widget, XtPointer, XtPointer);

class ProcessGroupCreateDialog : public Dialog
{

  private:
    //
    // Private member data:
    //
    static boolean ClassInitialized;
    static String  DefaultResources[];

    EditorWindow  *editor;

    List	groupList;

    Widget	list;
    Widget	text;
    Widget	addbtn, createbtn,removebtn, deletebtn, closebtn, showbtn;

    int	   lastIndex;

    void   makeList(int item);
    void   addCallbacks();
    void   setButtonsSensitive(Boolean setting);
 
  protected:
    //
    // Protected member data:
    //
    friend void ProcessGroupCreateDialog_AddCB(Widget, XtPointer, XtPointer);
    friend void ProcessGroupCreateDialog_CreateCB(Widget, XtPointer, XtPointer);
    friend void ProcessGroupCreateDialog_RemoveCB(Widget, XtPointer, XtPointer);
    friend void ProcessGroupCreateDialog_DeleteCB(Widget, XtPointer, XtPointer);
    friend void ProcessGroupCreateDialog_CloseCB(Widget, XtPointer, XtPointer);
    friend void ProcessGroupCreateDialog_ShowCB(Widget, XtPointer, XtPointer);
    friend void ProcessGroupCreateDialog_SelectCB(Widget, XtPointer, XtPointer);
    friend void ProcessGroupCreateDialog_TextVerifyCB(Widget, XtPointer, XtPointer);
    friend void ProcessGroupCreateDialog_TextChangeCB(Widget, XtPointer, XtPointer);

    virtual Widget createDialog(Widget parent);

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
    ProcessGroupCreateDialog(EditorWindow*);

    //
    // Destructor:
    //
    ~ProcessGroupCreateDialog();

    virtual void manage();

};

#endif // _ProcessGroupCreateDialog_h

