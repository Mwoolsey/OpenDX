/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#include "DXStrings.h"
#include "DXApplication.h"
#include "StartOptionsDialog.h"
#include "XmUtility.h"

#include <ctype.h>

#include <Xm/DialogS.h>
#include <Xm/Form.h>
#include <Xm/Label.h>
#include <Xm/Text.h>
#include <Xm/Separator.h>
#include <Xm/PushB.h>
#include <Xm/ToggleB.h>
#include <../widgets/XmDX.h>
#include <../widgets/Number.h>
#include <../widgets/Stepper.h>

Boolean StartOptionsDialog::ClassInitialized = FALSE;
String StartOptionsDialog::DefaultResources[] = {
    ".dialogTitle:			Options...",
    ".accelerators:			#augment\n"
#if 1
	"<Key>Return:			BulletinBoardReturn()",
#else
	"<Key>Return:			BulletinBoardReturn()\n"
	"<Btn2Down>,<Btn2Up>:		HELP!",
#endif
    "*execLabel.labelString:		Executive:",
    "*execText.width:			265",
    "*cwdLabel.labelString:		Working Directory:",
    "*memLabel.labelString:		Memory Size (Mb):",
    "*optionsLabel.labelString:		Options:",
    "*portLabel.labelString:		Port:",
    "*toggleButton.labelString:		Connect to already running server",
    "*okButton.labelString:		OK",
    "*cancelButton.labelString:		Cancel",
    NULL
};


extern "C" void StartOptionsDialog_ToggleCB(Widget w, XtPointer clientData, XtPointer)
{
    StartOptionsDialog *d = (StartOptionsDialog *)clientData;
    ASSERT(w == d->connectButton);
    Boolean b;
    XtVaGetValues(w, XmNset, &b, NULL);

    Pixel color = b ? theDXApplication->getInsensitiveColor()
		    : theDXApplication->getForeground();

    XtSetSensitive(d->portStepper,  b);
    SetTextSensitive(d->execText,    !b, color);
    SetTextSensitive(d->cwdText,     !b, color);
    SetTextSensitive(d->memText,     !b, color);
    SetTextSensitive(d->optionsText, !b, color);
}

extern "C" void StartOptionsDialog_NumberTextCB(Widget w,
				      XtPointer clientData,
				      XtPointer callData)
{
    XmTextVerifyCallbackStruct *cs = (XmTextVerifyCallbackStruct*)callData;

    char *s = XmTextGetString(w);

    char newS[1000];
    strncpy(newS, s, cs->startPos);
    newS[cs->startPos] = '\0';
    if (cs->text->length != 0)
	strncat(newS, cs->text->ptr, cs->text->length);
    newS[cs->startPos + cs->text->length] = '\0';
    strcat(newS, s+cs->endPos);

    // Ensure that the string an integer, possibly surrounded by white spaces.
    // Any (and all) of the fields may be empty.
    char *p = newS;
    while (isspace(*p))
	++p;
    while (isdigit(*p))
	++p;
    while (isspace(*p))
	++p;
    cs->doit = *p == '\0'? True: False;

    XtFree(s);
}

boolean StartOptionsDialog::okCallback(Dialog *clientData)
{
    StartOptionsDialog      *d = (StartOptionsDialog*)clientData;

    Boolean b;
    XtVaGetValues(d->connectButton, XmNset, &b, NULL);

    int autoStart = !b;

    char *exec =    XmTextGetString(d->execText);
    char *cwd =     XmTextGetString(d->cwdText);
    char *options = XmTextGetString(d->optionsText);
    char *sMem =    XmTextGetString(d->memText);
    int  mem = 0;
    sscanf(sMem, "%d", &mem);
    int port;
    XtVaGetValues(d->portStepper, XmNiValue, &port, NULL);

    const char *server;
    theDXApplication->getServerParameters(NULL,
					  &server,
					  NULL,
					  NULL,
					  NULL,
					  NULL,
					  NULL);


    theDXApplication->setServerParameters(autoStart,
		  STRLEN(server)  == 0? (const char *)NULL: (const char *)server,
		  STRLEN(exec)    == 0? (const char *)NULL: (const char *)exec,
		  STRLEN(cwd)     == 0? (const char *)NULL: (const char *)cwd,
		  STRLEN(options) == 0? (const char *)NULL: (const char *)options,
		  port, mem);
    XtFree(exec);
    XtFree(cwd);
    XtFree(options);
    XtFree(sMem);

    return TRUE;
}

Widget StartOptionsDialog::createDialog(Widget parent)
{
    Arg wargs[10];

    XtSetArg(wargs[0], XmNautoUnmanage,	False);

    //
    // Use our own function instead of XmCreateFormDialog()
    // to work around the auto-execution problem on indigo.
    //
    Widget w = this->CreateMainForm(parent, name, wargs, 1);

    Widget execLabel = XtVaCreateManagedWidget("execLabel",
	xmLabelWidgetClass,
	w,
	XmNtopAttachment,	XmATTACH_FORM,
	XmNtopOffset,		10,
	XmNleftAttachment,	XmATTACH_FORM,
	XmNleftOffset,		5,
	NULL);

    this->execText = XtVaCreateManagedWidget("execText",
	xmTextWidgetClass,
	w,
	XmNtopAttachment,	XmATTACH_FORM,
	XmNtopOffset,		10,
	XmNleftAttachment, 	XmATTACH_POSITION,
	XmNleftPosition,	40,
	XmNrightAttachment,	XmATTACH_FORM,
	XmNrightOffset,		5,
	XmNeditMode,		XmSINGLE_LINE_EDIT,
	NULL);

    Widget cwdLabel = XtVaCreateManagedWidget("cwdLabel",
	xmLabelWidgetClass,
	w,
	XmNtopAttachment,	XmATTACH_WIDGET,
	XmNtopWidget,		this->execText,
	XmNtopOffset,		10,
	XmNleftAttachment,	XmATTACH_FORM,
	XmNleftOffset,		5,
	NULL);

    this->cwdText = XtVaCreateManagedWidget("cwdText",
	xmTextWidgetClass,
	w,
	XmNtopAttachment,	XmATTACH_WIDGET,
	XmNtopWidget,		this->execText,
	XmNtopOffset,		10,
	XmNleftAttachment, 	XmATTACH_POSITION,
	XmNleftPosition,	40,
	XmNrightAttachment,	XmATTACH_FORM,
	XmNrightOffset,		5,
	XmNeditMode,		XmSINGLE_LINE_EDIT,
	NULL);

    Widget memLabel = XtVaCreateManagedWidget("memLabel",
	xmLabelWidgetClass,
	w,
	XmNtopAttachment,	XmATTACH_WIDGET,
	XmNtopWidget,		this->cwdText,
	XmNtopOffset,		10,
	XmNleftAttachment,	XmATTACH_FORM,
	XmNleftOffset,		5,
	NULL);

    this->memText = XtVaCreateManagedWidget("memText",
	xmTextWidgetClass,
	w,
	XmNtopAttachment,	XmATTACH_WIDGET,
	XmNtopWidget,		this->cwdText,
	XmNtopOffset,		10,
	XmNleftAttachment, 	XmATTACH_POSITION,
	XmNleftPosition,	40,
	XmNwidth,		100,
	NULL);
    XtAddCallback(this->memText,
		  XmNmodifyVerifyCallback, 
		  (XtCallbackProc)StartOptionsDialog_NumberTextCB,
		  (XtPointer)this);

    Widget optionsLabel = XtVaCreateManagedWidget("optionsLabel",
	xmLabelWidgetClass,
	w,
	XmNtopAttachment,	XmATTACH_WIDGET,
	XmNtopWidget,		this->memText,
	XmNtopOffset,		10,
	XmNleftAttachment,	XmATTACH_FORM,
	XmNleftOffset,		5,
	NULL);

    this->optionsText = XtVaCreateManagedWidget("optionsText",
	xmTextWidgetClass,
	w,
	XmNtopAttachment,	XmATTACH_WIDGET,
	XmNtopWidget,		this->memText,
	XmNtopOffset,		10,
	XmNleftAttachment, 	XmATTACH_POSITION,
	XmNleftPosition,	40,
	XmNrightAttachment,	XmATTACH_FORM,
	XmNrightOffset,		5,
	XmNeditMode,		XmSINGLE_LINE_EDIT,
	NULL);

    Widget separator1 = XtVaCreateManagedWidget("optionsSeparator1", 
	xmSeparatorWidgetClass,
	w,
	XmNleftAttachment,	XmATTACH_FORM,
	XmNleftOffset,		2,
	XmNrightAttachment,	XmATTACH_FORM,
	XmNrightOffset,		2,
	XmNtopOffset,		10,
	XmNtopAttachment,	XmATTACH_WIDGET,
	XmNtopWidget,		optionsText,
	NULL);

    this->connectButton = XtVaCreateManagedWidget("toggleButton", 
	xmToggleButtonWidgetClass,
	w,
	XmNtopOffset,		10,
	XmNtopAttachment,	XmATTACH_WIDGET,
	XmNtopWidget,		separator1,
	XmNleftAttachment,	XmATTACH_FORM,
	XmNleftOffset,		5,
	XmNindicatorType,	XmN_OF_MANY,
	XmNshadowThickness,	0,
	XmNset,			0,
	NULL);

    XtAddCallback(this->connectButton,
		  XmNvalueChangedCallback, 
		  (XtCallbackProc)StartOptionsDialog_ToggleCB,
		  (XtPointer)this);

    Widget portLabel = XtVaCreateManagedWidget("portLabel",
	xmLabelWidgetClass,
	w,
	XmNtopAttachment,	XmATTACH_WIDGET,
	XmNtopWidget,		this->connectButton,
	XmNtopOffset,		10,
	XmNleftAttachment,	XmATTACH_FORM,
	XmNleftOffset,		5,
	NULL);

    this->portStepper = XtVaCreateManagedWidget("portStepper",
	xmStepperWidgetClass,
	w,
	XmNtopAttachment,	XmATTACH_WIDGET,
	XmNtopWidget,		this->connectButton,
	XmNtopOffset,		10,
	XmNleftAttachment, 	XmATTACH_POSITION,
	XmNleftPosition,	40,
	XmNrightAttachment,	XmATTACH_FORM,
	XmNrightOffset,		5,
        XmNdataType       , INTEGER,
        XmNdigits         , 5,
        XmNrecomputeSize  , False,
        XmNeditable       , True,
        XmNiValue         , 1900,
        XmNiMinimum       , 1,
        XmNiMaximum       , 10000,
	NULL);

    Widget separator2 = XtVaCreateManagedWidget("optionsSeparator2", 
	xmSeparatorWidgetClass,
	w,
	XmNleftAttachment,	XmATTACH_FORM,
	XmNleftOffset,		2,
	XmNrightAttachment,	XmATTACH_FORM,
	XmNrightOffset,		2,
	XmNtopOffset,		10,
	XmNtopAttachment,	XmATTACH_WIDGET,
	XmNtopWidget,		this->portStepper,
	NULL);

    this->ok = XtVaCreateManagedWidget("okButton",
	xmPushButtonWidgetClass,
	w,
	XmNwidth,		70,
	XmNtopAttachment,	XmATTACH_WIDGET,
	XmNtopWidget,		separator2,
	XmNtopOffset,		10,
	XmNleftAttachment,	XmATTACH_FORM,
	XmNleftOffset,		10,
	XmNbottomAttachment,	XmATTACH_FORM,
	XmNbottomOffset,	10,
	XmNuserData,	     (XtPointer)this,
	NULL);
    

    this->cancel = XtVaCreateManagedWidget("cancelButton",
	xmPushButtonWidgetClass,
	w,
	XmNwidth,		70,
	XmNtopAttachment,	XmATTACH_WIDGET,
	XmNtopWidget,		separator2,
	XmNtopOffset,		10,
        XmNrightAttachment,	XmATTACH_FORM,
        XmNrightOffset,		10,
	XmNbottomAttachment,	XmATTACH_FORM,
	XmNbottomOffset,	10,
	XmNuserData,		(XtPointer)this,
	NULL);

    XtVaSetValues(w, XmNresizePolicy, XmRESIZE_NONE, NULL);
    return w;
}


StartOptionsDialog::StartOptionsDialog(Widget parent) : 
    Dialog("startOptionsDialog", parent)
{

    if (NOT StartOptionsDialog::ClassInitialized)
    {
        StartOptionsDialog::ClassInitialized = TRUE;
	this->installDefaultResources(theApplication->getRootWidget());
    }

}

//
// Install the default resources for this class.
//
void StartOptionsDialog::installDefaultResources(Widget  baseWidget)
{
    this->setDefaultResources(baseWidget, StartOptionsDialog::DefaultResources);
    this->Dialog::installDefaultResources( baseWidget);
}

void StartOptionsDialog::manage()
{
    int 	 autoStart;
    const char    *server;
    const char    *executive;
    const char    *workingDirectory;
    const char    *executiveFlags;
    int	 port;
    int	 memorySize;

    theDXApplication->getServerParameters(
	&autoStart,
	&server,
	&executive,
	&workingDirectory,
	&executiveFlags,
	&port,
	&memorySize);


    XmTextSetString(this->execText,    (char *)executive);
    XmTextSetString(this->cwdText,     (char *)workingDirectory);
    if (memorySize > 0)
    {
	char s[100];
	sprintf(s, "%d", memorySize);
	XmTextSetString(this->memText, s);
    }
    else
	XmTextSetString(this->memText, "");
    XmTextSetString(this->optionsText, (char *)executiveFlags);

    XtVaSetValues(this->portStepper, XmNiValue, port, NULL);
    XtVaSetValues(this->connectButton, XmNset, !autoStart, NULL);

    XtSetSensitive(this->portStepper, !autoStart);
    XtSetSensitive(this->execText, autoStart);
    XtSetSensitive(this->cwdText, autoStart);
    XtSetSensitive(this->memText, autoStart);
    XtSetSensitive(this->optionsText, autoStart);

    this->Dialog::manage();
}
