/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _StartServerDialog_h
#define _StartServerDialog_h


#include "Dialog.h"


//
// Class name definition:
//
#define ClassStartServerDialog	"StartServerDialog"

//
// XtCallbackProc (*CB), XtEventHandler (*EH) and XtActionProc (*AP)
// DialogCallback (*DCB), XtInputCallbackProc (*ICP), XtWorkProc (*WP)
// functions for this and derived classes
//
extern "C" void StartServerDialog_FocusEH(Widget, XtPointer, XEvent*, Boolean*);
extern "C" void StartServerDialog_TextCB(Widget, XtPointer, XtPointer);
extern "C" void StartServerDialog_OptionsCB(Widget, XtPointer, XtPointer);

class StartOptionsDialog;

//
// StartServerDialog class definition:
//				
class StartServerDialog : public Dialog
{
  private:
    //
    // Private member data:
    //
    static Boolean ClassInitialized;
    static String  DefaultResources[];

  protected:
    //
    // Protected member data:
    //
    virtual Widget createDialog(Widget parent);
    Widget text;
    Widget options;

    StartOptionsDialog *optionsDialog;

    friend void StartServerDialog_OptionsCB(Widget widget,
			        XtPointer clientData,
			        XtPointer);

    friend void StartServerDialog_TextCB(Widget widget,
			        XtPointer clientData,
			        XtPointer);

    friend void StartServerDialog_FocusEH(Widget widget,
			        XtPointer clientData,
			        XEvent* event,
				Boolean *cont);

    virtual boolean okCallback(Dialog *clientData);
    virtual void cancelCallback(Dialog *clientData);
    virtual void optionsCallback(StartServerDialog *clientData);

    //
    // For classes that wish to sub-class this dialog.
    //
    StartServerDialog(char *name, Widget parent);

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
    StartServerDialog(Widget parent);

    //
    // Destructor:
    //
    ~StartServerDialog();

    void manage();
    void unmanage();

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassStartServerDialog;
    }
};


#endif // _StartServerDialog_h
