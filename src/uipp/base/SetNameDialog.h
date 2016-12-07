/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _SetNameDialog_h
#define _SetNameDialog_h


#include "TextEditDialog.h"


//
// Class name definition:
//
#define ClassSetNameDialog	"SetNameDialog"

//
// XtCallbackProc (*CB), XtEventHandler (*EH) and XtActionProc (*AP)
// DialogCallback (*DCB) functions for this and derived classes
//
extern "C" void SetNameDialog_FocusEH(Widget, XtPointer, XEvent*, Boolean*);
extern "C" void SetNameDialog_TextCB(Widget, XtPointer, XtPointer);


//
// SetNameDialog class definition:
//				
class SetNameDialog : public TextEditDialog
{
  private:
    //
    // Private member data:
    //
    friend void SetNameDialog_TextCB(Widget widget,
                                XtPointer clientData,
                                XtPointer);

    friend void SetNameDialog_FocusEH(Widget widget,
                             XtPointer clientData,
                             XEvent* event,
			     Boolean *cont);

    friend void TextEditDialog_ApplyCB(Widget, XtPointer , XtPointer);

    boolean 	hasApply;

  protected:
    //
    // Protected member data:
    //
    static String DefaultResources[];

    virtual Widget createDialog(Widget parent);

    //
    // Constructor: make it protected so we can't directly instantiate it
    //
    SetNameDialog(const char *name, Widget parent, boolean has_apply = FALSE);

    //
    // Install the default resources for this class and then call the
    // same super class method to get the default resources from the
    // super classes.
    //
    virtual void installDefaultResources(Widget baseWidget);

  public:

    //
    // Destructor:
    //
    ~SetNameDialog();

    virtual void manage();

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassSetNameDialog;
    }
};


#endif // _SetNameDialog_h
