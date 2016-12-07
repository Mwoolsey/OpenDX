/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#ifndef _Dialog_h
#define _Dialog_h


#include "UIComponent.h"


//
// DialogCallback type definition:
//
typedef void (*DialogCallback)(void*);

//
// Class name definition:
//
#define ClassDialog	"Dialog"

//
// XtCallbackProc (*CB), XtEventHandler (*EH) and XtActionProc (*AP)
// DialogCallback (*DCB) functions for this and derived classes
//
extern "C" void Dialog_HelpCB(Widget, XtPointer, XtPointer);
extern "C" void Dialog_CancelCB(Widget, XtPointer, XtPointer);
extern "C" void Dialog_OkCB(Widget, XtPointer, XtPointer);

//
// Dialog class definition:
//				
class Dialog : public UIComponent
{
  private:
    //
    // Private member data:
    //
    static boolean ClassInitialized;

  protected:
    //
    // Protected member data:
    //
    friend void Dialog_OkCB(Widget widget,
			   XtPointer clientData,
			   XtPointer);
    friend void Dialog_CancelCB(Widget widget,
			       XtPointer clientData,
			       XtPointer);
    friend void Dialog_HelpCB(Widget widget,
			     XtPointer clientData,
			     XtPointer);

    //
    // This is to create a top level form contained by a shell widget.
    // Most of the dialogs should use this method, instead of 
    // XmCreateFormDialog(), to avoid the X bug on some platforms
    // like SGI indigo.
    //
    static Widget CreateMainForm(Widget parent, String name, 
				 ArgList arg, Cardinal argcount);

    boolean managed;

    Widget parent;
    Widget ok;
    Widget cancel;
    Widget help;

    virtual boolean okCallback(Dialog * /* clientData */) {return TRUE;}
    virtual void helpCallback(Dialog * /* clientData */) {}
    virtual void cancelCallback(Dialog * /* clientData */) {}

    virtual Widget createDialog(Widget parent);

    //
    // One time class initializations.
    //
    virtual void initialize();

    //
    // Constructor for the subclasses:
    //
    Dialog(const char *name, Widget parent);

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
    ~Dialog(){}

    virtual void post();

    virtual void manage();
    virtual void unmanage();
    virtual boolean isManaged();

    //
    // Set the title of the dialog after this->root has been created.
    // Ignored if this->root has not been created yet.
    //
    void setDialogTitle(const char *title);

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassDialog;
    }
};


#endif // _Dialog_h
