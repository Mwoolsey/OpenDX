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
#include <stdarg.h>

#include "Application.h"
#include "WarningDialogManager.h"


WarningDialogManager* theWarningDialogManager =
    new WarningDialogManager("warningDialog");


WarningDialogManager::WarningDialogManager(char* name): DialogManager(name)
{
    //
    // No op.
    //
}


Widget WarningDialogManager::createDialog(Widget parent)
{
    Widget dialog;
    Widget button;

    //
    // Create the warning dialog.  Realize it because of a bug on SGI and
    // perhaps elsewhere.
    //
    dialog = XmCreateWarningDialog(parent, this->name, NUL(ArgList), 0);
    XtRealizeWidget(dialog);

    //
    // Remove the cancel and help buttons.
    //
    XtUnmanageChild(XmMessageBoxGetChild(dialog, XmDIALOG_CANCEL_BUTTON));
    XtUnmanageChild(XmMessageBoxGetChild(dialog, XmDIALOG_HELP_BUTTON));

    //
    // Set the ok button to be the default button.
    //
    button = XmMessageBoxGetChild(dialog, XmDIALOG_OK_BUTTON);
    XtVaSetValues(button, XmNshowAsDefault, True, NULL);

    XtVaSetValues(dialog, XmNdefaultButton, button, NULL);

    //
    // Return the created dialog.
    //
    return dialog;
}
//
// Display an error message.
//
void
ModalWarningMessage(Widget parent, const char *fmt, ...)
{
    char buffer[1024];	// FIXME: how to allocate this
    va_list ap;

    va_start(ap, fmt);

    vsprintf(buffer,(char*)fmt,ap);
    theWarningDialogManager->modalPost(parent, buffer);

    va_end(ap);
}
//
// Display an error message.
//
void
WarningMessage(const char *fmt, ...)
{
    char buffer[1024];	// FIXME: how to allocate this
    va_list ap;

    va_start(ap, fmt);

    vsprintf(buffer,(char*)fmt,ap);
    theWarningDialogManager->post(theApplication->getRootWidget(), buffer);

    va_end(ap);
}
void WarningDialogManager::post(Widget parent,
                      char* message,
                      char* title,
                      void* clientData,
                      DialogCallback ok,
                      DialogCallback cancel,
                      DialogCallback help,
                      char* okLabel,
                      char* cancelLabel,
                      char* helpLabel,
                      int   cancelBtnNum)

{
    if (title == NULL)
	title = "Warning";

    this->DialogManager::post(parent,
		message, 
		title,
		clientData,
		ok,
	  	cancel,
		help,
		okLabel,
		cancelLabel,
		helpLabel,
		cancelBtnNum);
}

