/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _ErrorDialogManager_h
#define _ErrorDialogManager_h


#include "DialogManager.h"


//
// Class name definition:
//
#define ClassErrorDialogManager	"ErrorDialogManager"

void ErrorMessage(const char *fmt, ...);

void ModalErrorMessage(Widget parent, const char *fmt, ...);

//
// ErrorDialogManager class definition:
//				
class ErrorDialogManager : public DialogManager
{
  protected:
    //
    // Implementation of createDialog() for this class:
    //   Creates the dialog.  Intended to be called by superclass getDialog().
    //
    Widget createDialog(Widget parent);

  public:
    //
    // Constructor:
    //
    ErrorDialogManager(char* name);

    //
    // Destructor:
    //
    ~ErrorDialogManager(){}

    //
    // A variation on the superclass post() function:
    //   Only the message variable parameter is specifiable.
    //   All other parameters are defaulted.
    //
    virtual void post(Widget parent,
		      char* message,
		      char* = NULL,
		      void* = NULL,
		      DialogCallback = NULL,
		      DialogCallback = NULL,
		      DialogCallback = NULL,
                      char*          /* okLabel      */  = NULL,
                      char*          /* cancelLabel  */  = NULL,
                      char*          /* helpLabel    */  = NULL,
                      int            /* cancelBtnNum */  = 2
	)

    {
	this->DialogManager::post(parent, message, "Error");
    }

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassErrorDialogManager;
    }
};


extern ErrorDialogManager* theErrorDialogManager;


#endif // _ErrorDialogManager_h
