/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _QuestionDialogManager_h
#define _QuestionDialogManager_h


#include "DialogManager.h"

//
// Class name definition:
//
#define ClassQuestionDialogManager	"QuestionDialogManager"


//
// QuestionDialogManager class definition:
//				
class QuestionDialogManager : public DialogManager
{
  private:
    static void QueryOK (void *);
    static void QueryCancel (void *);
    static void QueryHelp (void *);

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
    QuestionDialogManager(char* name);

    //
    // Destructor:
    //
    ~QuestionDialogManager(){}

    virtual void post(Widget parent,
		      char*          message        = NULL,
                      char*          title          = NULL,
                      void*          clientData     = NULL,
                      DialogCallback okCallback     = NULL,
                      DialogCallback cancelCallback = NULL,
                      DialogCallback helpCallback   = NULL,
                      char*          okLabel        = NULL,
                      char*          cancelLabel    = NULL,
                      char*          helpLabel      = NULL,
		      int	     cancelBtnNum   = 2
		);

    //
    // possible return values from userQuery. 
    //
    enum { NoResponse, OK, Cancel, Help };
    int userQuery (Widget parent, 
	char* message, 
	char* title,
	char* okLabel,
	char* cancelLabel,
	char* helpLabel,
	int   cancelBtnNum   = 2
    );

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassQuestionDialogManager;
    }
};


extern QuestionDialogManager* theQuestionDialogManager;


#endif // _QuestionDialogManager_h


