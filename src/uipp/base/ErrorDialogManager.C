/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include <defines.h>





#include <stdio.h>
#include <stdarg.h>
#include <Xm/Xm.h>
#include <Xm/MessageB.h>

#include "ErrorDialogManager.h"
#include "Application.h"


ErrorDialogManager* theErrorDialogManager =
    new ErrorDialogManager("errorDialog");


ErrorDialogManager::ErrorDialogManager(char* name): DialogManager(name)
{
    //
    // No op.
    //
}


Widget ErrorDialogManager::createDialog(Widget parent)
{
    Widget dialog;
    Widget button;

    //
    // Create the error dialog.  Realize it because of a bug on SGI and
    // perhaps elsewhere.
    //
    dialog = XmCreateErrorDialog(parent, this->name, NUL(ArgList), 0);
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

void
ModalErrorMessage(Widget parent, const char *fmt, ...)
{
        va_list ap;
        char buffer[1024];      // FIXME: how to allocate this

        va_start(ap, fmt);

        vsprintf(buffer,(char*)fmt,ap);

        theErrorDialogManager->modalPost(parent, buffer);

        va_end(ap);
}


//
// Display an error message.
//
void
ErrorMessage(const char *fmt, ...)
{
	va_list ap;
	char buffer[1024];	// FIXME: how to allocate this

	va_start(ap, fmt);

	vsprintf(buffer,(char*)fmt,ap);	

	theErrorDialogManager->post(theApplication->getRootWidget(),buffer);

	va_end(ap);
}
