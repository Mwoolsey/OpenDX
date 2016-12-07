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

#define CATCH_CLOSE

#ifdef CATCH_CLOSE
#include <Xm/Protocols.h>
#include <Xm/AtomMgr.h>
#endif

#include <Xm/BulletinB.h>
#include <Xm/RowColumn.h>

#include "DialogManager.h"
#include "DialogData.h"
#include "Application.h"

#ifdef DXD_WIN             //SMH define local X routine
extern "C" { extern void ForceUpdate(Widget); }
#endif


extern "C" void DialogManager_DestroyCB(Widget widget,
					  XtPointer clientData,
					  XtPointer)
{
    DialogManager *dm = (DialogManager *)clientData;
    ASSERT(widget);

    // This callback if the widget is destroyed.  This does not normally happen.
    // The case that precipitated the addition of this callback was in the 
    // prompter, when switching "modes".  The main window (the parent of this
    // dialog) is destroyed and re-created.  The dialog widget is thus
    // destroyed.  The solution is to set the root member of UIComponent to
    // NULL, so it is not accessed downstream.
    if(widget == dm->getRootWidget())
	dm->clearRootWidget();
}

extern "C" void DialogManager_DestroyDialogCB(Widget widget,
					  XtPointer,
					  XtPointer)
{
    ASSERT(widget);
    XtDestroyWidget(widget);
}


extern "C" void DialogManager_OkCB(Widget    widget,
			       XtPointer clientData,
			       XtPointer)
{
    DialogData*    data;
    DialogCallback callback;

    ASSERT(widget);
    ASSERT(clientData);

    //
    // If the caller specified an OK callback, call the function.
    //
    data = (DialogData*)clientData;
    if ( (callback = data->getOkCallback()) )
    {
	(*callback)(data->getClientData());
    }

    //
    // Clean up....
    //
    DialogManager::CleanUp(widget, data);
}


//
// Handle both cancelCallbacks and WM_DELETE_WINDOW callbacks as delivered
// by the window manager when the 'Close' menu option is chosen.
// We have to do both in the same callback because I can't seem to get the
// WM_DELETE_WINDOW callback to go anywhere else. 
//
extern "C" void DialogManager_CancelCB(Widget    widget,
				   XtPointer clientData,
				   XtPointer callData)
{
    DialogCallback callback; 
    DialogData*    data = (DialogData*)clientData;
    XmAnyCallbackStruct *cbs = (XmAnyCallbackStruct *)callData;

    ASSERT(widget);
    ASSERT(clientData);

#ifdef CATCH_CLOSE
    //
    // Check to see if this is a real cancel call back or a window manager
    // protocol callback on the WM_DELETE_WINDOW. 
    //
    if (cbs->reason == XmCR_CANCEL)  {
#endif
	//
	// If the caller specified a cancel callback, call the function.
	//
	callback = data->getCancelCallback();
#ifdef CATCH_CLOSE
    } else {
	//
	// According to Section 17.3.2 page 593 of O'Reilly and Assoc. 
	// Motif Programming Manual version 1.1 (Volume 6), the Close
	// manager menu option should be mapped to the Cancel button
	// if one exists, otherwise ok if it exists.  If neither exist,
	// then pop down the dialog (and destroy it).
	//
	callback = data->getDismissCallback();
	widget   = data->getOwnerDialog(); // Get correct widget to CleanUp
	if (!callback)
	    callback = data->getOkCallback();
    }
#endif

    boolean freeData = (callback == data->getCancelCallback());
    if (callback)
	(*callback)(data->getClientData());
    else
	XtUnmanageChild(widget);

    //
    // Clean up....
    //
    if (freeData)
	DialogManager::CleanUp(widget, data);
}


extern "C" void DialogManager_HelpCB(Widget    widget,
				 XtPointer clientData,
				 XtPointer)
{
    DialogData*    data;
    DialogCallback callback;

    ASSERT(widget);
    ASSERT(clientData);

    //
    // If the caller specified a help callback, call the function.
    //
    data = (DialogData*)clientData;
    if ( (callback = data->getHelpCallback()) )
    {
	(*callback)(data->getClientData());
    }
}


void DialogManager::CleanUp(Widget      dialog,
			    DialogData* data)
{
    ASSERT(dialog);
    ASSERT(data);

    //
    // Remove all callbacks to avoid having duplicate callback
    // functions installed.
    //
    XtRemoveCallback
	(dialog,
	 XmNokCallback,
	 (XtCallbackProc)DialogManager_OkCB,
	 (XtPointer)data);
    XtRemoveCallback
	(dialog,
	 XmNcancelCallback,
	 (XtCallbackProc)DialogManager_CancelCB,
	 (XtPointer)data);

    Widget shell = XtParent(dialog);
    Atom WM_DELETE_WINDOW = XmInternAtom(XtDisplay(dialog),
					"WM_DELETE_WINDOW", False);
    XmRemoveWMProtocolCallback(shell, WM_DELETE_WINDOW, 
			       (XtCallbackProc)DialogManager_CancelCB, 
			       caddr_t(data));

    if (data->getHelpCallback() != NUL(DialogCallback))
    {
	Widget helpButton = XmMessageBoxGetChild(dialog, XmDIALOG_HELP_BUTTON);
	XtRemoveCallback
	    (helpButton,
	     XmNactivateCallback,
	     (XtCallbackProc)DialogManager_HelpCB,
	     (XtPointer)data);
    }

    //
    // Make sure the dialog is modeless, in case it was created modal.
    //
    XtVaSetValues(dialog, XmNdialogStyle, XmDIALOG_MODELESS, NULL);

    //
    // Delete the callback data instance for this posting.
    //
    delete data;
}


DialogManager::DialogManager(char* name): UIComponent(name)
{
    //
    // No op.
    //
}


Widget DialogManager::getDialog(Widget parent)
{
    Widget newDialog = NUL(Widget);

    ASSERT(parent);
    //
    // If the permanent dialog exists AND is not in use,
    // return the dialog.
    //
    if (this->getRootWidget() && !XtIsManaged(this->getRootWidget()) && 
		(this->lastParent == parent))
    {
	//
	// If this dialog was posted with modalPost() then we must
	// reset the modality.
	//
	return this->getRootWidget();
    }

    //
    // Otherwise, create a new dialog, using the application shell
    // widget as the parent.
    //
    ASSERT(theApplication);
    this->lastParent = parent;
    newDialog = this->createDialog(parent);


    //
    // If the root widget exists (but in use), install callbacks
    // to destroy it when the user pops it down.
    //
    if (this->getRootWidget())
    {
	// I tried out using XmNunmapCallback (which belongs to bulletin board and
	// to rowcolumn) instead of popdownCallback, but found no difference.
	XtAddCallback
	    (XtParent(this->getRootWidget()),
	     XmNpopdownCallback,
	     (XtCallbackProc)DialogManager_DestroyDialogCB,
	     (XtPointer)this);
    }

    this->setRootWidget(newDialog, FALSE);

    XtAddCallback
	(newDialog,
	 XmNdestroyCallback,
	 (XtCallbackProc)DialogManager_DestroyCB,
	 (XtPointer)this);

    return newDialog;
}


void DialogManager::post(Widget		parent,
			 char*          message,
			 char*          title,
			 void*          clientData,
			 DialogCallback okCallback,
			 DialogCallback cancelCallback,


			 DialogCallback helpCallback,
			 char*		okLabel,
			 char*		cancelLabel,
			 char*		helpLabel,
			 int		cancelBtnNum
	)
{
    Widget      dialog;
    XmString    string,label;
    DialogData* data;

    if (!parent)
	parent = theApplication->getRootWidget();

    dialog = this->getDialog(parent);
    ASSERT(dialog);
    ASSERT(XtIsSubclass(dialog, xmMessageBoxWidgetClass));
    
    //
    // Set the message, if available.
    //
    if (message)
    {
	string = XmStringCreateLtoR(message, XmSTRING_DEFAULT_CHARSET);
	XtVaSetValues(dialog, XmNmessageString, string, NULL);
	XmStringFree(string);
    }

    //
    // Set the title, if available.
    //
    if (title)
    {
	string = XmStringCreateLtoR(title, XmSTRING_DEFAULT_CHARSET);
	XtVaSetValues(dialog, XmNdialogTitle, string, NULL);
	XmStringFree(string);
    }

    //
    // Create a new instance of callback data.
    //
    ASSERT ((cancelBtnNum >= 1) && (cancelBtnNum <= 3));
    this->data = data =
	new DialogData(dialog, clientData,
			okCallback, cancelCallback, helpCallback, cancelBtnNum);

    //
    // Always add callbacks for OK and Cancel buttons.
    //
    XtAddCallback
	(dialog,
	 XmNokCallback,
	 (XtCallbackProc)DialogManager_OkCB,
	 (XtPointer)data);
    XtAddCallback
	(dialog,
	 XmNcancelCallback,
	 (XtCallbackProc)DialogManager_CancelCB,
	 (XtPointer)data);

    //
    // Add callback for help button, if the callback has been specified.
    //
    if (helpCallback)
    {
	Widget helpButton = XmMessageBoxGetChild(dialog, XmDIALOG_HELP_BUTTON);
	XtRemoveAllCallbacks(helpButton, XmNactivateCallback);
	XtAddCallback
	    (helpButton,
	     XmNactivateCallback,
	     (XtCallbackProc)DialogManager_HelpCB,
	     (XtPointer)data);
    }

    //
    // Set the button label strings.
    //
    if(okLabel)
    {
    	label = XmStringCreateSimple(okLabel);
    	XtVaSetValues(dialog, XmNokLabelString, label, NULL);
    	XmStringFree(label);
    }
    if(cancelLabel)
    {
    	label = XmStringCreateSimple(cancelLabel);
    	XtVaSetValues(dialog, XmNcancelLabelString, label, NULL);
    	XmStringFree(label);
    }
    if(helpLabel)
    {
    	label = XmStringCreateSimple(helpLabel);
    	XtVaSetValues(dialog, XmNhelpLabelString, label, NULL);
    	XmStringFree(label);
    }
    //
    // Post the dialog.
    //
#ifdef DXD_WIN                          //SMH make the error message appear
    //	ForceUpdate(dialog);	// Not Req AJ
#endif
    XtManageChild(dialog);

//
// The following code is for the sun only to fix the confirmation dialog size.
//
#ifdef sun4
    Widget sep = XmMessageBoxGetChild(dialog, XmDIALOG_SEPARATOR);
    Dimension width;
    XtVaGetValues(XtParent(sep), XmNwidth, &width, NULL);
    XtVaSetValues(XtParent(sep), XmNwidth, width+1, NULL);
#endif

#ifdef CATCH_CLOSE 
    //
    // Arrange to catch the 'Close' option callback of the 
    // window manager's decoration menu.
    //
    // NOTE: we do this for 3 reasons.  One is so that we don't have 
    // memory leaks.  The second is that it seems like a good idea to
    // map Close to either cancel or ok depending on which is present.
    // The other more important reason, is that the Warning dialogs
    // are used to tell the user that their license to run the UI has 
    // been lost.  When they hit Ok, a timer is installed to begin
    // shutdown of the application.  They must not be allowed to bypass 
    // having that timer installed by selecting Close instead of Ok.
    //
    // The following is taken from O'Rielly Volume 6, page 595.
    //
    Widget shell = XtParent(dialog);
    XtVaSetValues(shell, XmNdeleteResponse, XmDO_NOTHING, NULL);
    Atom WM_DELETE_WINDOW = XmInternAtom(XtDisplay(parent),
					"WM_DELETE_WINDOW", False);
    XmAddWMProtocolCallback(shell, WM_DELETE_WINDOW, 
				(XtCallbackProc)DialogManager_CancelCB, 
				caddr_t(data));
#endif

    parent = dialog;
    while(!XtIsShell(parent))
        parent = XtParent(parent);

    if(XtParent(parent) && (XtParent(parent)!=theApplication->getRootWidget()))
        XMapRaised(XtDisplay(parent), XtWindow(XtParent(parent)));
    XMapRaised(XtDisplay(parent), XtWindow(parent));

    //XmUpdateDisplay(dialog);
    XSync (XtDisplay(dialog), False);
        
}

void DialogManager::modalPost(Widget parent,
			 char*          message,
			 char*          title,
			 void*          clientData,
			 DialogCallback okCallback,
			 DialogCallback cancelCallback,
			 DialogCallback helpCallback,
			 char*		okLabel,
			 char*		cancelLabel,
			 char*		helpLabel,
			 int		style,
			 int		cancelBtnNum
	)
{
    //
    // Ungrab and grabbed pointers or keyboards so the user can respond
    // to the modal dialog.
    //
    XUngrabPointer(theApplication->getDisplay(),CurrentTime);
    XUngrabKeyboard(theApplication->getDisplay(),CurrentTime);

    this->getDialog(parent);
    XtVaSetValues(this->getRootWidget(),
                  XmNdialogStyle, style,
                  NULL);
    this->post(parent, (char*)message,
			 title,
			 clientData,
			 okCallback,
			 cancelCallback,
			 helpCallback,
			 okLabel,
			 cancelLabel,
			 helpLabel,
			 cancelBtnNum);

}

