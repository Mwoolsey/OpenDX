/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _WarningDialogManager_h
#define _WarningDialogManager_h


#include "DialogManager.h"


//
// Class name definition:
//
#define ClassWarningDialogManager	"WarningDialogManager"


void WarningMessage(const char *fmt, ...);
void ModalWarningMessage(Widget parent, const char *fmt, ...);

//
// WarningDialogManager class definition:
//				
class WarningDialogManager : public DialogManager
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
    WarningDialogManager(char* name);

    //
    // Destructor:
    //
    ~WarningDialogManager(){}

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
                      char*          okLabel        = NULL,
                      char*          cancelLabel    = NULL,
                      char*          helpLabel      = NULL,
                      int            cancelBtnNum   = 2);

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return "WarningDialogManager";
    }
};


extern WarningDialogManager* theWarningDialogManager;


#endif /*  _WarningDialogManager_h */


