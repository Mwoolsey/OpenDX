/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include <defines.h>




#include <Xm/Xm.h>
#include <Xm/MessageB.h>

#include "QuestionDialogManager.h"
#include "Application.h"


QuestionDialogManager* theQuestionDialogManager =
    new QuestionDialogManager("questionDialog");


QuestionDialogManager::QuestionDialogManager(char* name): DialogManager(name)
{
    //
    // No op.
    //
}

void QuestionDialogManager::post(Widget parent,
				char*          message,
                                char*          title,
                                void*          clientData,
                                DialogCallback okCallback,
                                DialogCallback cancelCallback,
                                DialogCallback helpCallback,
                                char*          okLabel,
                                char*          cancelLabel,
                                char*          helpLabel,
				int	       cancelBtnNum)
{
    Widget dialog = this->getDialog(parent);

    //
    // Remove the help button.
    //
    if(NOT helpLabel)
        XtUnmanageChild(XmMessageBoxGetChild(dialog, XmDIALOG_HELP_BUTTON));
    else
        XtManageChild(XmMessageBoxGetChild(dialog, XmDIALOG_HELP_BUTTON));

    this->DialogManager::post(parent, message, title, clientData,
                              	okCallback, cancelCallback, helpCallback,
                              	(okLabel ? okLabel : (char *)"Yes"), 
				(cancelLabel ? cancelLabel : (char *)"No"), helpLabel,
				cancelBtnNum
	);

}


Widget QuestionDialogManager::createDialog(Widget parent)
{
    Widget dialog;

    //
    // Create the question dialog.  Realize it because of a bug on SGI and
    // perhaps elsewhere.
    //
    dialog = XmCreateQuestionDialog(parent, this->name, NUL(ArgList), 0);
    XtRealizeWidget(dialog);

    //
    // Return the created dialog.
    //
    return dialog;
}


int QuestionDialogManager::userQuery (Widget parent, char* message, 
	char* title, char* okLabel, char* cancelLabel, char* helpLabel,
	int cancelBtnNum)
{
int users_response = QuestionDialogManager::NoResponse;
XEvent event;
XtAppContext app = theApplication->getApplicationContext();

    this->modalPost (parent, message, title, (void*)&users_response,
	QueryOK, QueryCancel, QueryHelp, okLabel, cancelLabel, helpLabel,
	XmDIALOG_FULL_APPLICATION_MODAL, cancelBtnNum);

    //
    // How do I know we'll ever exit the loop?  If any of the three callbacks is
    // invoked, then we'll exit.  The window mgr won't dismiss the dialog box
    // with its own close button, so the only way to get the thing off the screen
    // is with one of the buttons on the dialog box.  
    //
    while (users_response == QuestionDialogManager::NoResponse) {
	XtAppNextEvent (app, &event);
	theApplication->passEventToHandler (&event);
    }

    return users_response;
}

void QuestionDialogManager::QueryOK (void *cdata)
{
int *response = (int*)cdata;
    *response = QuestionDialogManager::OK;
}

void QuestionDialogManager::QueryCancel (void *cdata)
{
int *response = (int*)cdata;
    *response = QuestionDialogManager::Cancel;
}

void QuestionDialogManager::QueryHelp (void *cdata)
{
int *response = (int*)cdata;
    *response = QuestionDialogManager::Help;
}

