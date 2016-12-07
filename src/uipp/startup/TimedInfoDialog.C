/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#include "TimedInfoDialog.h"

#include <Xm/Xm.h>
#include <Xm/MessageB.h>
#include <X11/cursorfont.h>
#include "DXStrings.h"

Widget TimedInfoDialog::createDialog (Widget parent)
{
    XmString message = XmStringCreateLtoR (this->msg, "bold");
#if defined(alphax)
    XmString title = XmStringCreateLtoR (this->title, XmSTRING_DEFAULT_CHARSET);
#else
    XmString title = XmStringCreateLtoR (this->title, "bold");
#endif

    int n = 0;
    Arg args[10];
    XtSetArg (args[n],	XmNminimizeButtons, 	False); n++;
    XtSetArg (args[n],	XmNautoUnmanage,	True); n++;
    XtSetArg (args[n],	XmNdialogType,		XmDIALOG_INFORMATION); n++;
    XtSetArg (args[n],	XmNmessageString,	message); n++;
    XtSetArg (args[n],	XmNdialogTitle,		title); n++;
    XtSetArg (args[n],	XmNdialogStyle,		XmDIALOG_PRIMARY_APPLICATION_MODAL); n++;
    Widget box = XmCreateInformationDialog (parent, "timedInfo", args, n);

    XmStringFree(message);
    XmStringFree(title);

    XtUnmanageChild (XmMessageBoxGetChild (box, XmDIALOG_CANCEL_BUTTON));
    XtUnmanageChild (XmMessageBoxGetChild (box, XmDIALOG_HELP_BUTTON));

    return box;
}

void TimedInfoDialog::setTitle (const char *title)
{
    if (this->title) delete this->title;
    this->title = DuplicateString(title);
    Widget w = this->getRootWidget();
    if ((w) && (this->title)) {
#if defined(alphax)
	XmString xmstr = XmStringCreateLtoR (this->title, XmSTRING_DEFAULT_CHARSET);
#else
	XmString xmstr = XmStringCreateLtoR (this->title, "bold");
#endif
	XtVaSetValues (w, XmNdialogTitle, xmstr, NULL);
	XmStringFree(xmstr);
    }
}

void TimedInfoDialog::setMessage (const char *msg)
{
    if (this->msg) delete this->msg;
    this->msg = DuplicateString(msg);
    Widget w = this->getRootWidget();
    if ((w) && (this->msg)) {
	XmString xmstr = XmStringCreateLtoR (this->msg, "bold");
	XtVaSetValues (w, XmNmessageString, xmstr, NULL);
	XmStringFree(xmstr);
    }
}

