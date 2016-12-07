/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _InfoDialogManager_h
#define _InfoDialogManager_h


#include "DialogManager.h"


//
// Class name definition:
//
#define ClassInfoDialogManager	"InfoDialogManager"

//
// Display an info message.
//
void InfoMessage(const char *fmt, ...);
void ModalInfoMessage(Widget parent, const char *fmt, ...);


//
// InfoDialogManager class definition:
//				
class InfoDialogManager : public DialogManager
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
    InfoDialogManager(char* name);

    //
    // Destructor:
    //
    ~InfoDialogManager(){}

    //
    // A variation on the superclass post() function:
    //   Only the message variable parameter is specifiable.
    //   All other parameters are defaulted.
    //
    virtual void post(Widget parent,
		      char* message,
		      char *title = "Information",
		      void* = NULL,
		      DialogCallback = NULL,
		      DialogCallback = NULL,
		      DialogCallback = NULL,
                      char*          /*okLabel*/       = NULL,
                      char*          /*cancelLabel*/   = NULL,
                      char*          /*helpLabel*/     = NULL,
                      int            /*cancelBtnNum*/  = 2
	);
    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassInfoDialogManager;
    }
};


extern InfoDialogManager* theInfoDialogManager;


#endif // _InfoDialogManager_h


