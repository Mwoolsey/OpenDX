/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _ProcessGroupOptionsDialog_h
#define _ProcessGroupOptionsDialog_h


#include "Dialog.h"
#include "ProcessGroupAssignDialog.h"

//
// Class name definition:
//
#define ClassProcessGroupOptionsDialog	"ProcessGroupOptionsDialog"

//
// XtCallbackProc (*CB), XtEventHandler (*EH) and XtActionProc (*AP)
// DialogCallback (*DCB), XtInputCallbackProc (*ICP), XtWorkProc (*WP)
// functions for this and derived classes
//
extern "C" void ProcessGroupOptionsDialog_NumberTextCB(Widget, XtPointer, XtPointer);
extern "C" void ProcessGroupOptionsDialog_ToggleCB(Widget, XtPointer, XtPointer);

class ProcessGroupAssignDialog;

//
// ProcessGroupOptionsDialog class definition:
//				
class ProcessGroupOptionsDialog : public Dialog
{
  private:
    //
    // Private member data:
    //
    static Boolean ClassInitialized;
    ProcessGroupAssignDialog *pgad;

  protected:
    //
    // Protected member data:
    //
    static String  DefaultResources[];
    friend void ProcessGroupOptionsDialog_ToggleCB(Widget w, XtPointer clientData, XtPointer);
    friend void ProcessGroupOptionsDialog_NumberTextCB(Widget w, XtPointer clientData, XtPointer);

    virtual boolean okCallback(Dialog *clientData);

    virtual Widget createDialog(Widget parent);
    Widget execText;
    Widget cwdText;
    Widget memText;
    Widget optionsText;
    Widget connectButton;
    Widget portStepper;

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
    ProcessGroupOptionsDialog(ProcessGroupAssignDialog *pgad);

    //
    // Destructor:
    //
    ~ProcessGroupOptionsDialog(){}


    virtual void manage();

    void setArgs(const char *args);
    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassProcessGroupOptionsDialog;
    }
};


#endif // _ProcessGroupOptionsDialog_h
