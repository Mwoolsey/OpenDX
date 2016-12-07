/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#include "DXApplication.h"
#include "StartServerDialog.h"
#include "StartOptionsDialog.h"
#include "DXChild.h"

#include <Xm/DialogS.h>
#include <Xm/Form.h>
#include <Xm/Label.h>
#include <Xm/Text.h>
#include <Xm/Separator.h>
#include <Xm/PushB.h>

Boolean StartServerDialog::ClassInitialized = FALSE;
String StartServerDialog::DefaultResources[] = 
{
    ".dialogTitle:			Start Server...",
    "*hostLabel.labelString:		Hostname:",
//    "*hostText.width:			200",
    "*connectButton.labelString:	Connect",
    "*cancelButton.labelString:		Cancel",
    "*optionsButton.labelString:	Options...",
//    "*XmPushButton.width:		75",
//    "*XmPushButton.height:		25",
    NULL
};



boolean StartServerDialog::okCallback(Dialog *clientData)   // AJ 
{
    StartServerDialog  *d = (StartServerDialog*)clientData;


    char *s = XmTextGetString(d->text);

    int autoStart;
    const char *server;
    const char *executive;
    const char *workingDirectory;
    const char *executiveFlags;
    int port;
    int memorySize;

    this->unmanage();

    theDXApplication->setBusyCursor(TRUE);

    theDXApplication->getServerParameters(
	&autoStart,
	&server,
	&executive,
	&workingDirectory,
	&executiveFlags,
	&port,
	&memorySize);

    theDXApplication->setServerParameters(
	autoStart,
	s,
	executive,
	workingDirectory,
	executiveFlags,
	port,
	memorySize);

    DXChild *c = theDXApplication->startServer();
    theDXApplication->completeConnection(c);

    theDXApplication->setBusyCursor(FALSE);

    if(s)   XtFree(s);     //   AJ

    return FALSE;	// Unmanaged above
}
void StartServerDialog::cancelCallback(Dialog *)
{
}

extern "C" void StartServerDialog_TextCB(Widget,
					XtPointer clientData,
					XtPointer)
{
    StartServerDialog *d = (StartServerDialog*)clientData;

    d->okCallback(d);
}

extern "C" void StartServerDialog_OptionsCB(Widget,
					XtPointer clientData,
					XtPointer)
{
    StartServerDialog *d = (StartServerDialog*)clientData;

    d->optionsCallback(d);
}

void StartServerDialog::optionsCallback(StartServerDialog *clientData)
{
    StartServerDialog  *d = (StartServerDialog*) clientData;

    if (this->optionsDialog == NULL)
	this->optionsDialog = new StartOptionsDialog(this->getRootWidget());
    d->optionsDialog->post();
}

extern "C" void StartServerDialog_FocusEH(Widget,
					XtPointer clientData,
					XEvent* event,
					Boolean *)
{
    StartServerDialog *d = (StartServerDialog*)clientData;

    XSetInputFocus(XtDisplay(d->text),
		   XtWindow(d->text),
		   RevertToPointerRoot,
		   CurrentTime);
}

Widget StartServerDialog::createDialog(Widget parent)
{
    Arg  arg[5];
    int  n = 0;

    XtSetArg(arg[n], XmNautoUnmanage, False);  n++;

    Widget w = this->CreateMainForm(parent, this->name, arg, n);

    Widget label = XtVaCreateManagedWidget("hostLabel",
	xmLabelWidgetClass,
	w,
	XmNtopAttachment,	XmATTACH_FORM,
	XmNtopOffset,		10,
	XmNleftAttachment,	XmATTACH_FORM,
	XmNleftOffset,		5,
	NULL);

    this->text = XtVaCreateManagedWidget("hostText",
	xmTextWidgetClass,
	w,
	XmNtopAttachment,	XmATTACH_FORM,
	XmNtopOffset,		10,
	XmNleftAttachment, 	XmATTACH_WIDGET,
	XmNleftWidget,		label,
	XmNleftOffset,		10,
	XmNrightAttachment,	XmATTACH_FORM,
	XmNrightOffset,		5,
	XmNeditMode,		XmSINGLE_LINE_EDIT,
	NULL);

    Widget separator = XtVaCreateManagedWidget("connectSeparator", 
	xmSeparatorWidgetClass,
	w,
	XmNleftAttachment,   XmATTACH_FORM,
	XmNleftOffset,       2,
	XmNrightAttachment,  XmATTACH_FORM,
	XmNrightOffset,      2,
	XmNtopOffset,        9,
	XmNtopAttachment,    XmATTACH_WIDGET,
	XmNtopWidget,        this->text,
	NULL);


    this->ok = XtVaCreateManagedWidget("connectButton",
	xmPushButtonWidgetClass,
	w,
	XmNleftAttachment,   XmATTACH_FORM,
	XmNleftOffset,       9,
	XmNtopAttachment,    XmATTACH_WIDGET,
	XmNtopWidget,        separator,
	XmNtopOffset,        9,
	XmNbottomAttachment, XmATTACH_FORM,
	XmNbottomOffset,     9,
	XmNuserData,	     (XtPointer)this,
#ifdef sun4
//	XmNheight,	     40,
//	XmNwidth,	     90,
#endif
	XmNshowAsDefault,    2,
	NULL);

    this->options = XtVaCreateManagedWidget("optionsButton",
	xmPushButtonWidgetClass,
	w,
	XmNleftAttachment,   XmATTACH_POSITION,
	XmNleftPosition,     50,
	XmNleftOffset,       -30,
	XmNtopAttachment,    XmATTACH_WIDGET,
	XmNtopWidget,        separator,
	XmNtopOffset,        17,
	XmNbottomAttachment, XmATTACH_FORM,
	XmNbottomOffset,     9 + 8,
	XmNuserData,	     (XtPointer)this,
	NULL);

    this->cancel = XtVaCreateManagedWidget("cancelButton",
	xmPushButtonWidgetClass,
	w,
        XmNrightAttachment,  XmATTACH_FORM,
        XmNrightOffset,      9,
	XmNtopAttachment,    XmATTACH_WIDGET,
	XmNtopWidget,        separator,
	XmNtopOffset,        17,
	XmNbottomAttachment, XmATTACH_FORM,
	XmNbottomOffset,     9 + 8,
	XmNuserData,	     (XtPointer)this,
	NULL);

    XtAddCallback(this->text,
		  XmNactivateCallback,
		  (XtCallbackProc)StartServerDialog_TextCB,
		  (XtPointer)this);

    XtAddCallback(this->options,
		  XmNactivateCallback,
		  (XtCallbackProc)StartServerDialog_OptionsCB,
		  (XtPointer)this);

    XtAddEventHandler(w,
		  EnterWindowMask, 
		  False,
		  (XtEventHandler)(XtEventHandler)StartServerDialog_FocusEH,
		  (XtPointer)this);

    return w;
}


StartServerDialog::StartServerDialog(char *name, Widget parent) : 
    Dialog(name, parent)
{
    this->optionsDialog = NULL;

}
StartServerDialog::StartServerDialog(Widget parent) : 
    Dialog("startServerDialog", parent)
{
    this->optionsDialog = NULL;

    if (NOT StartServerDialog::ClassInitialized)
    {
        StartServerDialog::ClassInitialized = TRUE;
	this->installDefaultResources(theApplication->getRootWidget());
    }
}

StartServerDialog::~StartServerDialog()
{
    if (this->optionsDialog)
	delete this->optionsDialog;
}

//
// Install the default resources for this class.
//
void StartServerDialog::installDefaultResources(Widget  baseWidget)
{
    this->setDefaultResources(baseWidget, StartServerDialog::DefaultResources);
    this->Dialog::installDefaultResources( baseWidget);
}

void StartServerDialog::manage()
{
    this->Dialog::manage();

    const char *server;
    theDXApplication->getServerParameters(
	NULL,
	&server,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL);
    XmTextSetString(this->text, (char *)server);
}

void StartServerDialog::unmanage()
{
    if (this->optionsDialog)
	this->optionsDialog->unmanage();
    this->Dialog::unmanage();
}
