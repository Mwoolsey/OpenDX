/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include <defines.h>





#include <stdarg.h>
#include <Xm/Xm.h>
#include <Xm/MessageB.h>

#include "InfoDialogManager.h"
#include "Application.h"
#include "DXStrings.h"


InfoDialogManager* theInfoDialogManager =
    new InfoDialogManager("informationDialog");


InfoDialogManager::InfoDialogManager(char* name): DialogManager(name)
{
    //
    // No op.
    //
}


Widget InfoDialogManager::createDialog(Widget parent)
{
    Widget dialog;
    Widget button;

    //
    // Create the information dialog.  Realize it because of a bug on SGI and
    // perhaps elsewhere.
    //
    dialog = XmCreateInformationDialog(parent, this->name, NUL(ArgList), 0);
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

void InfoDialogManager::post(
		      Widget parent,
                      char *message,
                      char *title,
                      void *,
                      DialogCallback ,
                      DialogCallback ,
                      DialogCallback ,
                      char*          ,
                      char*          ,
                      char*          ,
                      int            )
{
    this->DialogManager::post(parent, message, title);
}

void
ModalInfoMessage(Widget parent, const char *fmt, ...)
{
        va_list ap;
        char buffer[1024];      // FIXME: how to allocate this

        va_start(ap, fmt);

        vsprintf(buffer,(char*)fmt,ap);

        theInfoDialogManager->modalPost(parent, buffer);

        va_end(ap);
}


//
// Display an info message.
//
void
InfoMessage(const char *fmt, ...)
{
        va_list ap;
        char *p, buffer[1024];      
	int len = STRLEN(fmt);

  	if (len > 512) 
	    p = new char[len * 2];
	else
	    p = buffer;
	
        va_start(ap, fmt);

        vsprintf(p,(char*)fmt,ap);

        theInfoDialogManager->post(NULL,p);

	if (p != buffer)
	    delete p;

        va_end(ap);
}

