/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _DialogManager_h
#define _DialogManager_h


#include <Xm/Xm.h>
#include "UIComponent.h"


//
// Class name definition:
//
#define ClassDialogManager	"DialogManager"

//
// XtCallbackProc (*CB), XtEventHandler (*EH) and XtActionProc (*AP)
// DialogCallback (*DCB) functions for this and derived classes
//
extern "C" void DialogManager_HelpCB(Widget, XtPointer, XtPointer);
extern "C" void DialogManager_CancelCB(Widget, XtPointer, XtPointer);
extern "C" void DialogManager_OkCB(Widget, XtPointer, XtPointer);
extern "C" void DialogManager_DestroyDialogCB(Widget, XtPointer, XtPointer);
extern "C" void DialogManager_DestroyCB(Widget, XtPointer, XtPointer);

class DialogData;

//
// DialogCallback type definition:
//
typedef void (*DialogCallback)(void*);

//
// DialogManager class definition:
//				
class DialogManager : public UIComponent
{
  private:
    friend void DialogManager_DestroyDialogCB(Widget widget,
				      XtPointer,
				      XtPointer);

    friend void DialogManager_DestroyCB(Widget widget,
				      XtPointer,
				      XtPointer);

    friend void DialogManager_OkCB(Widget    widget,
			   XtPointer clientData,
			   XtPointer);

    friend void DialogManager_CancelCB(Widget    widget,
			       XtPointer clientData,
			       XtPointer);

    friend void DialogManager_HelpCB(Widget    widget,
			     XtPointer clientData,
			     XtPointer);

  protected:
    DialogData  *data;
    Widget	lastParent;

    virtual Widget createDialog(Widget parent) = 0;

    //
    // Constructor:
    //   Protected to prevent direct instantiation.
    //
    DialogManager(char* name);

    Widget getDialog(Widget parent);

  public:
    //
    // Destructor:
    //
    ~DialogManager(){}

    static void CleanUp(Widget      dialog,
			DialogData* data);

    DialogData *getData()
    {
	return this->data;
    }

    //
    // Posts a dialog as a child of the application's root window.
    //
    virtual void post(Widget parent,
		      char*          message        = NULL,
		      char*          title          = NULL,
		      void*          clientData     = NULL,
		      DialogCallback okCallback     = NULL,
		      DialogCallback cancelCallback = NULL,
		      DialogCallback helpCallback   = NULL,
		      char*	     okLabel	    = NULL,
		      char*	     cancelLabel    = NULL,
		      char*	     helpLabel	    = NULL,
		      int	     cancelBtnNum   = 2
		);

    //
    // Post a modal dialog.
    //
    void modalPost(Widget parent,
		      char*          message        = NULL,
		      char*          title          = NULL,
		      void*          clientData     = NULL,
		      DialogCallback okCallback     = NULL,
		      DialogCallback cancelCallback = NULL,
		      DialogCallback helpCallback   = NULL,
		      char*	     okLabel	    = NULL,
		      char*	     cancelLabel    = NULL,
		      char*	     helpLabel	    = NULL,
		      int	     style	    = XmDIALOG_FULL_APPLICATION_MODAL,
		      int	     cancelBtnNum   = 2
	);

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassDialogManager;
    }
};


#endif // _DialogManager_h
