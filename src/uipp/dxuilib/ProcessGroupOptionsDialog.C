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
#include "ProcessGroupOptionsDialog.h"
#include "ProcessGroupManager.h"

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
#include "lex.h"

Boolean ProcessGroupOptionsDialog::ClassInitialized = FALSE;
String ProcessGroupOptionsDialog::DefaultResources[] = {
    "*dialogTitle:			Options...",
    ".accelerators:			#augment\n"
#if 1
	"<Key>Return:			BulletinBoardReturn()",
#else
	"<Key>Return:			BulletinBoardReturn()\n"
	"<Btn2Down>,<Btn2Up>:		HELP!",
#endif
    ".execLabel.labelString:		Executive:",
    ".cwdLabel.labelString:		Working Directory:",
    ".memLabel.labelString:		Memory Size (Mb):",
    ".optionsLabel.labelString:		Options:",
    ".portLabel.labelString:		Port:",
    ".connectButton.labelString:	Connect to already running server",
    ".okButton.labelString:		OK",
    ".cancelButton.labelString:		Cancel",
    NULL
};

static XrmOptionDescRec opTable[] = {
{"-exec",	"*exec",	XrmoptionSepArg,	(char *) NULL},
{"-directory",	"*directory",	XrmoptionSepArg,	(char *) NULL},
{"-memory",	"*memory",	XrmoptionSepArg,	(char *) NULL},
};

extern "C" void ProcessGroupOptionsDialog_ToggleCB(Widget w, XtPointer clientData, XtPointer)
{
    ProcessGroupOptionsDialog *d = (ProcessGroupOptionsDialog *)clientData;
    ASSERT(w == d->connectButton);
    Boolean b;
    XtVaGetValues(w, XmNset, &b, NULL);

    XtSetSensitive(d->portStepper,  b);
    XtSetSensitive(d->execText,    !b);
    XtSetSensitive(d->cwdText,     !b);
    XtSetSensitive(d->memText,     !b);
    XtSetSensitive(d->optionsText, !b);
}

extern "C" void ProcessGroupOptionsDialog_NumberTextCB(Widget w,
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

boolean ProcessGroupOptionsDialog::okCallback(Dialog *clientData)  // AJ
{
    ProcessGroupOptionsDialog      *d = (ProcessGroupOptionsDialog*)clientData;
#if WORKSPACE_PAGES
    ProcessGroupManager *pgm = theDXApplication->getProcessGroupManager();
#else
    ProcessGroupManager *pgm = theDXApplication->PGManager;
#endif

    char *exec =    XmTextGetString(d->execText);
    char *cwd =     XmTextGetString(d->cwdText);
    char *options = XmTextGetString(d->optionsText);
    char *sMem =    XmTextGetString(d->memText);
    int  mem = 0;

    if(sMem)
	sscanf(sMem, "%d", &mem);

    char *name = XmTextGetString(d->pgad->hosttext);
    if(name)
    {
	char args[1024];
	args[0] = '\0';
	if(exec && !IsBlankString(exec))
	{
	    strcat(args, "-exec ");
	    strcat(args, exec);
	    strcat(args, " ");
	}
	if(cwd && !IsBlankString(cwd))
	{
	    strcat(args, "-directory ");
	    if(strchr(cwd, ' ') != NULL) {
	    	strcat(args, "\"");
	    	strcat(args, cwd);
	    	strcat(args, "\"");
	    } else 
	        strcat(args, cwd);

	    strcat(args, " ");
	}
	if(sMem && !IsBlankString(sMem))
	{
	    strcat(args, " -memory ");
	    strcat(args, sMem);
	    strcat(args, " ");
	}
	if(options && !IsBlankString(options))
	{
	    strcat(args, options);
	}
	pgm->assignArgument(name, args);
	d->pgad->dirty = TRUE;
    }
    if(exec) XtFree(exec);
    if(cwd) XtFree(cwd);
    if(options) XtFree(options);
    if(sMem) XtFree(sMem);
    if(name) XtFree(name);

    return TRUE;
}

Widget ProcessGroupOptionsDialog::createDialog(Widget parent)
{
    Arg arg[10];
    int n;

    n = 0;
    XtSetArg(arg[n], XmNautoUnmanage,	False); n++;
    XtSetArg(arg[n], XmNwidth,		450); n++;
    XtSetArg(arg[n], XmNheight,		211); n++;
    XtSetArg(arg[n], XmNdialogStyle,	XmDIALOG_FULL_APPLICATION_MODAL);n++;
//    Widget w = XmCreateFormDialog(parent, name, arg, n);
    Widget w = this->CreateMainForm(parent, name, arg, n);

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
		  (XtCallbackProc)ProcessGroupOptionsDialog_NumberTextCB,
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

    this->ok = XtVaCreateManagedWidget("okButton",
	xmPushButtonWidgetClass,
	w,
	XmNwidth,		70,
	XmNtopAttachment,	XmATTACH_WIDGET,
	XmNtopWidget,		separator1,
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
	XmNtopWidget,		separator1,
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


ProcessGroupOptionsDialog::ProcessGroupOptionsDialog( 
			   ProcessGroupAssignDialog *pgad) : 
			   Dialog("PGOD", pgad->app->getRootWidget())
{

    if (NOT ProcessGroupOptionsDialog::ClassInitialized)
    {
        ProcessGroupOptionsDialog::ClassInitialized = TRUE;
	this->installDefaultResources(theApplication->getRootWidget());
    }
    this->pgad = pgad;
}

void ProcessGroupOptionsDialog::manage()
{
    this->Dialog::manage();
}

void ProcessGroupOptionsDialog::setArgs(const char *args)
{
    int         i;
    int         j;
    int         argc;
    XrmDatabase	db;
    XrmValue    value;
    char        *str_type[20];
    char	*argv[100];
    char	*other = new char[512];

    XmTextSetString(this->execText, "");
    XmTextSetString(this->cwdText, "");
    XmTextSetString(this->memText, "");
    XmTextSetString(this->optionsText, "");

    if(args == NULL) return;

    //
    // Otherwise, parse the string
    //
    argc = 0;
    int ndx = 0;
    argv[0] = "PGOD";
    
    i = 1;
    while(args[ndx])
    {
	argv[i] = (char *)CALLOC(STRLEN(&args[ndx]), sizeof(char));
	SkipWhiteSpace(args, ndx);
	j = 0;
	while(!IsWhiteSpace(args, ndx) && args[ndx])
	    argv[i][j++] = args[ndx++];
	i++;
    }
    argv[i] = NULL;
    argc = i;

    db = NULL;
    XrmParseCommand(&db, opTable, XtNumber(opTable), "PGOD", &argc, argv);

    if(XrmGetResource(db, "PGOD.exec", "*Exec", str_type, &value))
	XmTextSetString(this->execText, value.addr);
    if(XrmGetResource(db, "PGOD.directory", "*Directory", str_type, &value))
	XmTextSetString(this->cwdText, value.addr);
    if(XrmGetResource(db, "PGOD.memory", "*Number", str_type, &value))
	XmTextSetString(this->memText, value.addr);

    i = 1;
    other[0] = '\0';
    while(argv[i])
    {
	strcat(other, argv[i]);
	strcat(other, " ");
	delete(argv[i++]);
    }
    if(STRLEN(other) > 0)
	XmTextSetString(this->optionsText, other);

    delete other;
}
//
// Install the default resources for this class.
//
void ProcessGroupOptionsDialog::installDefaultResources(Widget  baseWidget)
{
    this->setDefaultResources(baseWidget,
				ProcessGroupOptionsDialog::DefaultResources);
    this->Dialog::installDefaultResources( baseWidget);
}
