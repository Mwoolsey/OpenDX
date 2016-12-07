/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"
#include "../base/StartWebBrowser.h"

#include <X11/StringDefs.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <limits.h>
#include <Xm/Xm.h>
#include <Xm/DialogS.h>
#include <Xm/Form.h>
#include <Xm/PushB.h>
#include <Xm/ToggleB.h>
#include <Xm/RowColumn.h>
#include <Xm/Label.h>
#include <Xm/Separator.h>
#include <Xm/Text.h>
#ifdef DXD_WIN
#include <Xm/FileSB.h>
#include <process.h>
#endif

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#include "../widgets/Stepper.h"
#include "ErrorDialogManager.h"
#include "InfoDialogManager.h"
#include "DXStrings.h"
#include "ButtonInterface.h"
#include "StartupCommand.h"
#include "StartupWindow.h"
#include "StartupApplication.h"
#include "TimedInfoDialog.h"
#include "ListIterator.h"
#include "NetFileDialog.h"

#include <dxl.h>



//
// Note on XmNgeometry resource in DefaultResources or in a -xrm string:
// The dimensions in use may look strange, but they are actually added to
// minWidth,minHeight to arrive at an actual figure.  (See MainWindow::setGeometry)
//

boolean StartupWindow::ClassInitialized = FALSE;

#if !defined(DXD_OS_NON_UNIX)

//
// On sgi, aviion, and solaris, our use of a pipe lets us know that garui
// went away and that it's ok to start another one when the user asks.
// On these platforms, the pipe doesn't work that way so we'll make a
// polling loop that runs once every few seconds and checks the child pid.
//
#if defined(ibm6000) || defined(hp700) || defined(alphax) || defined(sun4) || defined(linux) || defined(cygwin) || defined(freebsd) || defined(macos)
#define USE_WAIT3 1
#endif

#if USE_WAIT3
#if defined(alphax)
#include <sys/time.h>
#define _BSD
#include <sys/wait.h>
#else
#include <sys/wait.h>
#include <sys/time.h>
#endif
#else
#include <wait.h>
#endif

FILE* StartupWindow::TutorConn = NUL(FILE*);
XtInputId StartupWindow::GarErrorId = 0;
XtInputId StartupWindow::GarReadId = 0;
int StartupWindow::GarPid = -1;
XtInputId StartupWindow::TutorErrorId = 0;
XtInputId StartupWindow::TutorReadId = 0;


#endif // for unix only


//
// Used to store a command for later use by a timeout proc.
//
char* StartupWindow::PendingCommand = NUL(char*);
char* StartupWindow::PendingNetFile = NUL(char*);

DXLConnection* StartupWindow::Connection     = NULL;

String  StartupWindow::DefaultResources[] = {
    ".iconName:                         DX",
    ".title:				Data Explorer",
    ".minWidth:				220",
    ".minHeight:			285",
    ".width:				220",
    ".height:				285",
    ".mainWindow.showSeparator:		False",
    ".mainWindow.shadowThickness:	1",
    "*commandForm.shadowThickness:	1",
    "*quit.labelString:			Quit",
    "*help.labelString:			Help",
    "*startPrompter.labelString:	Import Data...", 
    "*startImage.labelString:		Run Visual Programs...", 
    "*startEditor.labelString:		Edit Visual Programs...", 
    "*emptyVPE.labelString:		New Visual Program...",
    "*startTutorial.labelString:	Run Tutorial...", 
    "*startDemo.labelString:		Samples...", 

    "*startPrompter.height:	29",
    "*startImage.height:	29",
    "*startEditor.height:	29",
    "*startTutorial.height:	29",
    "*startDemo.height:		29",
    "*emptyVPE.height:		29",

    "*startPrompter.recomputeSize:	False",
    "*startImage.recomputeSize:		False",
    "*startEditor.recomputeSize:	False",
    "*startTutorial.recomputeSize:	False",
    "*startDemo.recomputeSize:		False",
    "*emptyVPE.recomputeSize:		False",

#if defined(aviion)
    "*om.labelString:",
#endif

    NULL
};

StartupWindow::StartupWindow() : IBMMainWindow("startupWindow", FALSE)
{
    this->startImageOption 	= NUL(CommandInterface*);
    this->startPrompterOption 	= NUL(CommandInterface*);
    this->startEditorOption 	= NUL(CommandInterface*);
    this->startTutorialOption 	= NUL(CommandInterface*);
    this->startDemoOption 	= NUL(CommandInterface*);
    this->quitOption 		= NUL(CommandInterface*);
    this->helpOption 		= NUL(CommandInterface*);
    this->netFileDialog		= NUL(NetFileDialog*);
    this->demoNetFileDialog	= NUL(NetFileDialog*);
    this->loaded_file		= NUL(char*);
    this->command_timer		= 0;

    this->startImageCmd =
	new StartupCommand ("startImage", this->commandScope, TRUE, 
	    this, StartupCommand::StartImage);
    this->startPrompterCmd =
	new StartupCommand ("startPrompter", this->commandScope, TRUE, 
	    this, StartupCommand::StartPrompter);
    this->startEditorCmd =
	new StartupCommand ("startEditor", this->commandScope, TRUE, 
	    this, StartupCommand::StartEditor);
    this->emptyVPECmd =
	new StartupCommand ("emptyVPE", this->commandScope, TRUE, 
	    this, StartupCommand::EmptyVPE);
    this->startTutorialCmd =
	new StartupCommand ("startTutorial", this->commandScope, TRUE, 
	    this, StartupCommand::StartTutorial);
    this->startDemoCmd =
	new StartupCommand ("startDemo", this->commandScope, TRUE, 
	    this, StartupCommand::StartDemo);
    this->quitCmd =
	new StartupCommand ("quit", this->commandScope, TRUE, 
	    this, StartupCommand::Quit);
    this->helpCmd =
	new StartupCommand ("help", this->commandScope, TRUE, 
	    this, StartupCommand::Help);

    //
    // Install the default resources for THIS class (not the derived classes)
    //
    if (NOT StartupWindow::ClassInitialized)
    {
	ASSERT(theApplication);
        StartupWindow::ClassInitialized = TRUE;
	this->installDefaultResources(theApplication->getRootWidget());
    }
}
//
// Install the default resources for this class.
//
void StartupWindow::installDefaultResources(Widget baseWidget)
{
    this->setDefaultResources(baseWidget, StartupWindow::DefaultResources);
    this->IBMMainWindow::installDefaultResources(baseWidget);
}

StartupWindow::~StartupWindow()
{
    if (this->startImageOption)  	delete this->startImageOption;
    if (this->startPrompterOption)  	delete this->startPrompterOption;
    if (this->startEditorOption)  	delete this->startEditorOption;
    if (this->emptyVPEOption)	  	delete this->emptyVPEOption;
    if (this->startTutorialOption)  	delete this->startTutorialOption;
    if (this->startDemoOption)  	delete this->startDemoOption;
    if (this->quitOption)  		delete this->quitOption;
    if (this->helpOption)  		delete this->helpOption;

    if (this->startImageCmd)  	delete this->startImageCmd;
    if (this->startPrompterCmd) delete this->startPrompterCmd;
    if (this->startEditorCmd)  	delete this->startEditorCmd;
    if (this->startTutorialCmd) delete this->startTutorialCmd;
    if (this->startDemoCmd) 	delete this->startDemoCmd;
    if (this->quitCmd)  	delete this->quitCmd;
    if (this->helpCmd)  	delete this->helpCmd;

    if (this->netFileDialog)	delete this->netFileDialog;
    if (this->demoNetFileDialog)delete this->demoNetFileDialog;
    if (this->loaded_file)	delete this->loaded_file;

    if (this->command_timer)    XtRemoveTimeOut(this->command_timer);
}

Widget StartupWindow::createWorkArea(Widget parent)
{

    Widget form = XtVaCreateManagedWidget("mainForm", xmFormWidgetClass, parent, NULL);

    //
    // S T A R T   P R O M P T E R
    //
    this->startPrompterOption = 
	new ButtonInterface (form, "startPrompter", this->startPrompterCmd);
    Widget button = this->startPrompterOption->getRootWidget();
    XtVaSetValues (button,
	XmNtopAttachment, XmATTACH_FORM,
	XmNleftAttachment, XmATTACH_FORM,
	XmNrightAttachment, XmATTACH_FORM,
	XmNtopOffset,	10,
	XmNleftOffset,	10,
	XmNrightOffset,	10,
    NULL);
    Widget prev_button = button;

    //
    // S T A R T   U I   I N   I M A G E   M O D E
    //
    this->startImageOption = 
	new ButtonInterface (form, "startImage", this->startImageCmd);
    button = this->startImageOption->getRootWidget();
    XtVaSetValues (button,
	XmNtopAttachment, XmATTACH_WIDGET,
	XmNtopWidget, prev_button,
	XmNleftAttachment, XmATTACH_FORM,
	XmNrightAttachment, XmATTACH_FORM,
	XmNtopOffset,	10,
	XmNleftOffset,	10,
	XmNrightOffset,	10,
    NULL);
    prev_button = button;

    //
    // S T A R T   U I   I N   E D I T   M O D E
    //
    this->startEditorOption = 
	new ButtonInterface (form, "startEditor", this->startEditorCmd);
    button = this->startEditorOption->getRootWidget();
    XtVaSetValues (button,
	XmNtopAttachment, XmATTACH_WIDGET,
	XmNtopWidget, prev_button,
	XmNleftAttachment, XmATTACH_FORM,
	XmNrightAttachment, XmATTACH_FORM,
	XmNtopOffset,	10,
	XmNleftOffset,	10,
	XmNrightOffset,	10,
    NULL);
    prev_button = button;

    //
    // S T A R T   U I   W I T H   A N   E M P T Y   V P E
    //
    this->emptyVPEOption = 
	new ButtonInterface (form, "emptyVPE", this->emptyVPECmd);
    button = this->emptyVPEOption->getRootWidget();
    XtVaSetValues (button,
	XmNtopAttachment, XmATTACH_WIDGET,
	XmNtopWidget, prev_button,
	XmNleftAttachment, XmATTACH_FORM,
	XmNrightAttachment, XmATTACH_FORM,
	XmNtopOffset,	10,
	XmNleftOffset,	10,
	XmNrightOffset,	10,
    NULL);
    prev_button = button;

    //
    // S T A R T   T U T O R I A L
    //
    this->startTutorialOption = 
	new ButtonInterface (form, "startTutorial", this->startTutorialCmd);
    button = this->startTutorialOption->getRootWidget();
    XtVaSetValues (button,
	XmNtopAttachment, XmATTACH_WIDGET,
	XmNtopWidget, prev_button,
	XmNleftAttachment, XmATTACH_FORM,
	XmNrightAttachment, XmATTACH_FORM,
	XmNtopOffset,	10,
	XmNleftOffset,	10,
	XmNrightOffset,	10,
    NULL);
    prev_button = button;


    //
    // S T A R T   D E M O
    //
    this->startDemoOption = 
	new ButtonInterface (form, "startDemo", this->startDemoCmd);
    button = this->startDemoOption->getRootWidget();
    XtVaSetValues (button,
	XmNtopAttachment, XmATTACH_WIDGET,
	XmNtopWidget, prev_button,
	XmNleftAttachment, XmATTACH_FORM,
	XmNrightAttachment, XmATTACH_FORM,
	XmNtopOffset,	10,
	XmNleftOffset,	10,
	XmNrightOffset,	10,
    NULL);
    prev_button = button;

    //
    // Change the shadow colors on the parent to get the "dialog box"
    // appearance.
    //
    Pixel ts,bs;
    XtVaGetValues (form,
	XmNtopShadowColor, &ts, 
	XmNbottomShadowColor, &bs, 
    NULL);
    XtVaSetValues (parent, 
	XmNtopShadowColor, bs, 
	XmNbottomShadowColor, ts, 
    NULL);

    return form;
}


Widget StartupWindow::createCommandArea (Widget parent)
{
    Widget form = 
	XtVaCreateWidget ("commandForm",
	    xmFormWidgetClass,		parent,
    NULL);

    this->quitOption = 
	new ButtonInterface (form, "quit", this->quitCmd);
    Widget button = this->quitOption->getRootWidget();
    XtVaSetValues (button, 
	XmNtopAttachment,    	XmATTACH_FORM,
	XmNtopOffset,        	10,
	XmNbottomAttachment,    XmATTACH_FORM,
	XmNbottomOffset,        10,
	XmNleftAttachment,      XmATTACH_POSITION,
	XmNrightAttachment,     XmATTACH_POSITION,
	XmNleftPosition,        10,
	XmNrightPosition,       45,
    NULL);


    this->helpOption = 
	new ButtonInterface (form, "help", this->helpCmd);
    button = this->helpOption->getRootWidget();
    XtVaSetValues (button,
	XmNtopAttachment,    	XmATTACH_FORM,
	XmNtopOffset,        	10,
	XmNbottomAttachment,    XmATTACH_FORM,
	XmNbottomOffset,        10,
	XmNleftAttachment,      XmATTACH_POSITION,
	XmNleftPosition,        55,
	XmNrightAttachment,     XmATTACH_POSITION,
	XmNrightPosition,       90,
    NULL);

    return form;
}

void StartupWindow::postTimedDialog(const char *msg, int millis)
{
    TimedInfoDialog *td = 
	new TimedInfoDialog("startMessage", this->getRootWidget(), millis);

    td->setTitle ("Please Wait...");
    td->setMessage (msg);
    td->post();
}

boolean StartupWindow::startImage()
{
    if (!this->netFileDialog)
	this->netFileDialog = new NetFileDialog(this->getRootWidget(), this);
    this->entering_image_mode = TRUE;
    this->netFileDialog->post();
    this->netFileDialog->showNewOption(FALSE);
    return TRUE;
}

void StartupWindow::startNetFile (const char *file)
{
int argcnt = 0;
char *args[30], *cmdstr;

    //
    // Special Case:
    // The only combination of mixed startup modes and loaded files is the
    // one where we want to run or edit the same file we have
    // already opened.  In that case, we can either open the vpe and return,
    // or else execute once and return.
    //
    if ((StartupWindow::Connection) &&
	(file) && (file[0]) &&
	(this->loaded_file) &&
	(EqualString(this->loaded_file, file))) {
	if (this->entering_image_mode != this->started_in_image_mode) {
	    if (this->entering_image_mode)
		DXLExecuteOnce(StartupWindow::Connection);
	    else
		uiDXLOpenVPE (StartupWindow::Connection);
	} else {
	    if (this->entering_image_mode)
		DXLExecuteOnce(StartupWindow::Connection);
	}
	return ;
    }

    //
    // Sometimes we want edit mode, other times image mode.  There is no way
    // to switch (even using uiDXL... functions).  So if we started in the opposite
    // mode from what we want, then restart.
    //
    if ((StartupWindow::Connection) && 
	(this->entering_image_mode != this->started_in_image_mode)) {
	DXLExitDX(StartupWindow::Connection);
	StartupWindow::Connection = NUL(DXLConnection*);
	if (this->loaded_file) delete this->loaded_file;
	this->loaded_file = NUL(char*);
    }

    //
    // It's not possible to do a File/New.  So if we ever caused a file to
    // load and want to an empty vpe later on, then restart.
    // Note:  The dialog box doesn't make the "New" button available from the
    // "Run visual program" option.
    //
    if ((StartupWindow::Connection) &&
	(this->loaded_file) &&
	((file==NUL(char*))||(file[0]=='\0'))) {
	DXLExitDX(StartupWindow::Connection);
	StartupWindow::Connection = NUL(DXLConnection*);
	delete this->loaded_file;
	this->loaded_file = NUL(char*);
    }


    if (StartupWindow::Connection) {
	DXLEndExecution(StartupWindow::Connection);

        if (this->entering_image_mode)
	    uiDXLCloseVPE(StartupWindow::Connection);


	//
	// net file to use
	//
	if ((file) && (file[0])) {
	    this->postTimedDialog("...loading new visual program.", 4000);
	    DXLLoadVisualProgram (StartupWindow::Connection, file);
	    if (this->loaded_file) delete this->loaded_file;
	    this->loaded_file = DuplicateString(file);
	}
	
	if (this->entering_image_mode)
	    DXLExecuteOnChange(StartupWindow::Connection);
    } else {
	//
	// net file to use
	//
	if ((file) && (file[0])) {
	    if (StartupWindow::PendingNetFile) 
		delete StartupWindow::PendingNetFile; 
	    if (this->loaded_file) delete this->loaded_file;
	    this->loaded_file = NUL(char*);
	    StartupWindow::PendingNetFile = DuplicateString(file);
	    args[argcnt++] = "-wizard";
	}

	//
	// startup mode for dxui
	//
	if (this->entering_image_mode) {
	    args[argcnt++] = "-menubar"; 
//	    args[argcnt++] = "-xrm";
//	    args[argcnt++] = "DX.editorWindow.geometry:200x150-0+0";
//	    args[argcnt++] = "-xrm";
//	    args[argcnt++] = "DX.dxAnchor.geometry:410x56+0+0";
	    this->started_in_image_mode = TRUE;
	} else {
	    args[argcnt++] = "-edit"; 
//	    args[argcnt++] = "-xrm";
//	    args[argcnt++] = "DX.editorWindow.geometry:-0+0";
	    this->started_in_image_mode = FALSE;
	}

	//
	// There aren't any Partition modules in Donna's auto-visualize nets,
	// and ezstart is likely to kick off multiple copies of dxui which makes
	// it hard to run without -p 1 on an mp machine.
	//
	args[argcnt++] = "-processors 1";
#if defined(DXD_LICENSED_VERSION)
	const char* cp = theStartupApplication->getDxuiArgs();
	if (cp) args[argcnt++] = DuplicateString(cp);
#endif

	cmdstr = this->formCommand ("dx", args, argcnt, FALSE);
	if (StartupWindow::PendingCommand) delete StartupWindow::PendingCommand;
	StartupWindow::PendingCommand = DuplicateString(cmdstr);

	XSynchronize (XtDisplay(this->getRootWidget()), True);
	this->postTimedDialog("Starting Data Explorer...", 8000);
	XSynchronize (XtDisplay(this->getRootWidget()), False);

	//
	// The following is a hack to make the TimedDialog update its window.
	// (I believe I tried all the easy stuff: XSync, XmUpdateDisplay,
	// using a work proc, setting the connection to the server in synchronous
	// mode, and putting an event loop here. 
	// So set a timer and continue the work we've started when the timer goes off.
	// That gives the dialog a chance to paint itself.
	//
	if (this->command_timer)
	    XtRemoveTimeOut (this->command_timer);
	this->command_timer = 
	    XtAppAddTimeOut (theApplication->getApplicationContext(), 80,
		(XtTimerCallbackProc)StartupWindow_RefreshTO, (XtPointer)this);
    }
    return ;
}

extern "C" void
StartupWindow_RefreshTO(XtPointer cData, XtIntervalId*  )
{
    StartupWindow* suw = (StartupWindow*)cData;
    ASSERT(suw);
    ASSERT(StartupWindow::PendingCommand);
    suw->command_timer = 0;

    theApplication->setBusyCursor(TRUE);
    XmUpdateDisplay(suw->getRootWidget());
    StartupWindow::Connection = DXLStartDX(StartupWindow::PendingCommand, NULL);
    theApplication->setBusyCursor(FALSE);

    if (!StartupWindow::Connection) {
	ErrorMessage ("Unable to connect to dxexec");
	return ;
    }
    DXLInitializeXMainLoop (theApplication->getApplicationContext(),
	StartupWindow::Connection);
    DXLSetBrokenConnectionCallback(StartupWindow::Connection, 
	StartupWindow_BrokenConnCB, (void*)suw);
    DXLSetErrorHandler (StartupWindow::Connection, (DXLMessageHandler)
	StartupWindow_ErrorMsgCB, (const void*)suw);

    if (StartupWindow::PendingNetFile) {
	suw->loaded_file = StartupWindow::PendingNetFile;
	StartupWindow::PendingNetFile = NUL(char*);
	DXLLoadVisualProgram (StartupWindow::Connection, suw->loaded_file);
	if (suw->entering_image_mode) {
	    DXLExecuteOnChange(StartupWindow::Connection);
	}
    }
    return ;
}


//
// Hack me up into little tiny bits.  sigh. wheeze. 
// There isn't time to do this right, so I won't try.  Use pipe, fork, exec to
// kick off a prompter so that there is a file descriptor linking parent and
// child processes.  This way each one can select on the descriptor as a way of
// knowing when the other goes away.  Parent wants to know, because it needs
// to prevent spawing multiple childs.  Child wants to know because it needs
// to shut itself off if (it got a license from parent && parent goes away).
//
boolean StartupWindow::startPrompter()
{
    int result;

#ifndef DXD_WIN
    if (StartupWindow::GarReadId) {
	ErrorMessage ("You have one prompter running already.");
	return FALSE;
    }
#endif

    this->postTimedDialog("Starting Data Explorer Prompter...", 3000);

#ifndef DXD_WIN
    int fds[2];
    if (pipe(fds) < 0) {
	ErrorMessage ("pipe error");
	return FALSE;
    }
    int child = fork();
    if (child == 0) {
	char path[512];
	sprintf(path, "%s/bin_%s/prompter", theIBMApplication->getUIRoot(), DXD_ARCHNAME);
	close (fds[1]);
	dup2 (fds[0], 0);
#if !defined(DXD_OS_NON_UNIX) && defined(DXD_LICENSED_VERSION)
	if (theStartupApplication->isLimitedUse()) 
	    execl (path, path, "-limited", NUL(char*));
	else
#endif
	execl (path, path, NUL(char*));
	fprintf (stderr, "Data Explorer is unable to exec %s\n", path);
	exit(1);
    } else if (child > 0) {
	close(fds[0]);
	StartupWindow::GarPid = child;
    } else {
	ErrorMessage ("Unable to fork child\n");
	return FALSE;
    }
    int fd = fds[1];
    StartupWindow::GarReadId = XtAppAddInput (theApplication->getApplicationContext(), 
	fd, (XtPointer)XtInputReadMask, (XtInputCallbackProc)StartupWindow_ReadGAR_CB, 
	(XtPointer)this);
    StartupWindow::GarErrorId = XtAppAddInput (theApplication->getApplicationContext(), 
	fd, (XtPointer)XtInputExceptMask, (XtInputCallbackProc)StartupWindow_ResetGAR_CB, 
	(XtPointer)this);
#if defined(USE_WAIT3)
    XtAppAddTimeOut (theApplication->getApplicationContext(), 3000,
	(XtTimerCallbackProc)StartupWindow_WaitPidTO, (XtPointer)this);
#endif
#else // !DXD_WIN
    char* arg0 = "-prompter";
    result = _spawnlp(_P_WAIT, "dx", "dx", arg0, NULL);
    if(result == -1) {
	printf( "error prompter spawning process: %s\n", strerror( errno ) );
    }
      
#endif
    return TRUE;
}

boolean StartupWindow::startEditor()
{
    if (!this->netFileDialog)
	this->netFileDialog = new NetFileDialog(this->getRootWidget(), this);
    this->entering_image_mode = FALSE;
    this->netFileDialog->post();
    this->netFileDialog->showNewOption(TRUE);
    return TRUE;
}

boolean StartupWindow::startEmptyVPE()
{
    this->entering_image_mode = FALSE;
    this->startNetFile(NUL(char*));
    return TRUE;
}

boolean StartupWindow::startTutorial()
{
    char *url = new char[strlen(theIBMApplication->getUIRoot()) + 35];
    strcpy(url, "file://");
    strcat(url, theIBMApplication->getUIRoot());
    strcat(url, "/html/pages/qikgu011.htm");
    if(!_dxf_StartWebBrowserWithURL(url)) {
#ifndef DXD_WIN
		if (StartupWindow::TutorConn) {
			ErrorMessage ("You have one tutorial running already.");
			return FALSE;
		}
#endif
		char *args[5], *cmdstr;
		args[0] = "-tutor"; 
		args[1] = "-xrm"; 
		args[2] = "DXTutor.dxTutorWindow.geometry:280x600+670+0";
		cmdstr = this->formCommand ("dx", args, 3);
		
		this->postTimedDialog("Starting Data Explorer Tutorial...", 3000);
#ifndef DXD_WIN
		StartupWindow::TutorConn = popen(cmdstr, "r");
		if (!StartupWindow::TutorConn) {
			ErrorMessage ("Couldn't spawn the tutorial.");
			return FALSE;
		}
		int fd = fileno(StartupWindow::TutorConn);
		StartupWindow::TutorReadId = XtAppAddInput (theApplication->getApplicationContext(), 
			fd, (XtPointer)XtInputReadMask, 
			(XtInputCallbackProc)StartupWindow_ReadTutor_CB, (XtPointer)this);
		StartupWindow::TutorErrorId = XtAppAddInput (theApplication->getApplicationContext(), 
			fd, (XtPointer)XtInputExceptMask, 
			(XtInputCallbackProc)StartupWindow_ResetTutor_CB, (XtPointer)this);
#else
		_spawnlp(_P_WAIT, "dx", "dx", args[0], NULL);
#endif
    }
    return TRUE;
}

//
// Set the filter in the file selection dialog and then do what start-image does.
//
boolean StartupWindow::startDemo()
{
boolean set_filter = FALSE;

    if (!this->demoNetFileDialog) {
	this->demoNetFileDialog = new NetFileDialog(this->getRootWidget(), this);
	set_filter = TRUE;
    }
    this->entering_image_mode = TRUE;
    this->demoNetFileDialog->post();
    this->demoNetFileDialog->showNewOption(FALSE);

    // 
    // Set the filter.  fsb should not be NULL.
    // 
    Widget fsb = this->demoNetFileDialog->getFileSelectionBox();
    if ((fsb) && (set_filter)) {
	this->demoNetFileDialog->setDialogTitle("Sample Program Selection");
	const char* uir = theIBMApplication->getUIRoot();
	char* ext = "/samples/programs/*.net";
	char* dirspec = new char[strlen(uir) + strlen(ext) + 1];
	sprintf (dirspec, "%s%s", uir, ext);
#ifdef DXD_WIN
/* Convert to Unix if DOS */
    for(int i=0; i<strlen(dirspec); i++)
        if(dirspec[i] == '/') dirspec[i] = '\\';
#endif
	XmString xmstr = XmStringCreateLtoR (dirspec, "bold");
#ifdef DXD_WIN
	Widget filt = XmFileSelectionBoxGetChild(fsb, XmDIALOG_FILTER_TEXT);
	XmTextSetString(filt, dirspec);
#endif
	XtVaSetValues (fsb, XmNdirMask, xmstr, NULL);
	XmStringFree(xmstr);
	delete dirspec;
    }

    return TRUE;
}

//
// All dxlink sessions will magically go away here.  
// If you wanted to add some sort of user-prompt
// prior to quitting, then the way to do that is to use a different class
// of command for the quit command, rather than to put code here.  Look at
// ConfirmedQuitCommand.
//
boolean StartupWindow::quit()
{
    if (StartupWindow::Connection) DXLExitDX(StartupWindow::Connection);
    exit(0);
    return TRUE;
}


boolean StartupWindow::help()
{
    FILE *fp;
    int helpsize;

    const char *dxroot = theIBMApplication->getUIRoot();
    const char *nohelp = "No Help Available";
    
    if (!dxroot) {
       ErrorMessage(nohelp);
       return FALSE;
    }

    char helpfile[1024];
    sprintf(helpfile, "%s/ui/help.txt", dxroot);  
 
    if (!(fp = fopen(helpfile, "r"))) {
       ErrorMessage(nohelp);
       return FALSE;
    }

    struct STATSTRUCT buf;

    if (STATFUNC(helpfile, &buf) != 0) {
       ErrorMessage(nohelp);
       return FALSE;
    }
    helpsize = buf.st_size;

    char *helpstr = new char[helpsize + 1];
    if (!helpstr) {
       ErrorMessage(nohelp);
       return FALSE;
    }
    fread(helpstr, 1, helpsize, fp);
    fclose(fp); 
    helpstr[helpsize] = '\0';

    InfoMessage(helpstr);
    delete(helpstr);
    return TRUE; 
}


void StartupWindow::closeWindow()
{
    this->IBMMainWindow::closeWindow();
    this->quit();
}

//
// Append the args to the command, then add on whatever arguments were on the
// command line excluding the ones already int 'args'.  This isn't really foolproof.
// Some arguments take an additional parameter.  It's not right to check those also.
// The 'enc' arg means enclose the command string in parens.  Why is that necessary?
//
char *StartupWindow::formCommand (const char *cmd, char **args, int nargs, boolean enc)
{
static char cmdstr[2048];
int i, totlen;

    totlen = 0;
    if (enc)
	sprintf (cmdstr, "(%s ", cmd);
    else
	sprintf (cmdstr, "%s ", cmd);
    totlen+= strlen(cmdstr);

    for (i=0; i<nargs; i++) {
	strcpy (&cmdstr[totlen], args[i]);
	totlen+= strlen(args[i]);
	cmdstr[totlen++] = ' '; cmdstr[totlen] = '\0';
    }

    ListIterator it(*theStartupApplication->getCommandLineArgs());
    char *cp;
    while (cp = (char *)it.getNext()) {
	strcpy (&cmdstr[totlen], cp);
	totlen+= strlen(cp);
	cmdstr[totlen++] = ' '; cmdstr[totlen] = '\0';
    }
    if (enc) {
	cmdstr[totlen++] = ' ';
	cmdstr[totlen++] = ')';
	cmdstr[totlen++] = '&';
	cmdstr[totlen] = '\0';
    }
    ASSERT (totlen < 1024);
    return cmdstr;
}

extern "C"
void StartupWindow_BrokenConnCB(DXLConnection*, void* clientData)
{
    StartupWindow* sw = (StartupWindow*)clientData;
    ASSERT(sw);
    StartupWindow::Connection = NUL(DXLConnection*);
    if (sw->loaded_file) delete sw->loaded_file;
    sw->loaded_file = NUL(char*);
    sw->started_in_image_mode = FALSE;
    sw->entering_image_mode = FALSE;
}

extern "C"
void StartupWindow_ErrorMsgCB (DXLConnection*, const char*, const void*)
{
}


//
// The following bunch of subroutinage is excluded on pc builds because the
// pcs are using some form of spawn and unix is using fork, exec, pipe, popen,
// to get children going.  The unix implementation requires that parent/child
// share opposite ends of a communication channel so that both know when
// either goes away.  Licensing is involved and we want to prevent starting
// up too many children.
//
#if !defined(DXD_OS_NON_UNIX)
extern "C"
void StartupWindow_ResetGAR_CB (XtPointer cData, int* fd, XtInputId* )
{
    StartupWindow* suw = (StartupWindow*)cData;
    ASSERT(suw);
    suw->resetGarConnection();
    close(*fd);
}

extern "C"
void StartupWindow_ReadGAR_CB (XtPointer cData, int* fd, XtInputId* )
{
    StartupWindow* suw = (StartupWindow*)cData;
    ASSERT(suw);
    char buf[1024];
    int bread = read (*fd, buf, 1024);
    if (bread == 0) {
	suw->resetGarConnection();
	close(*fd);
    }
}

extern "C"
void StartupWindow_ResetTutor_CB (XtPointer cData, int*, XtInputId* )
{
    StartupWindow* suw = (StartupWindow*)cData;
    ASSERT(suw);
    suw->resetTutorConnection();
}

extern "C"
void StartupWindow_ReadTutor_CB (XtPointer cData, int*, XtInputId* )
{
    StartupWindow* suw = (StartupWindow*)cData;
    ASSERT(suw);
    if ((StartupWindow::TutorConn == NUL(FILE*)) || (feof(StartupWindow::TutorConn))) {
	suw->resetTutorConnection();
	return ;
    }
    char buf[1024];
    fread (buf, 1, 1024, StartupWindow::TutorConn);
}


void StartupWindow::resetTutorConnection()
{
    if (StartupWindow::TutorReadId) {
	XtRemoveInput (StartupWindow::TutorReadId);
	StartupWindow::TutorReadId = 0;
    }
    if (StartupWindow::TutorErrorId) {
	XtRemoveInput (StartupWindow::TutorErrorId);
	StartupWindow::TutorErrorId = 0;
    }
    if (StartupWindow::TutorConn) {
	pclose (StartupWindow::TutorConn);
	StartupWindow::TutorConn = NUL(FILE*);
    }
}
void StartupWindow::resetGarConnection()
{
    if (StartupWindow::GarReadId) {
	XtRemoveInput (StartupWindow::GarReadId);
	StartupWindow::GarReadId = 0;
    }
    if (StartupWindow::GarErrorId) {
	XtRemoveInput (StartupWindow::GarErrorId);
	StartupWindow::GarErrorId = 0;
    }
#if !defined(USE_WAIT3)
    siginfo_t winfo;
    waitid (P_ALL, 0, &winfo, WNOHANG|WEXITED);
#endif
}

//
// On some platforms, we find out asynchronously (iow immediately) when
// a child dies because of our pipe with the child.  On ibm6000, hp700,
// sun4, and alphax, we don't know that the child died.  We find out
// gar is gone because every few seconds we look at its pid.  If it
// died, then we'll shut down our pipe and allow the user to start another
// one.
//
extern "C"
void StartupWindow_WaitPidTO (XtPointer cData, XtIntervalId*  )
{
#if defined(USE_WAIT3)
    if (StartupWindow::GarReadId == 0) return ;

    StartupWindow* suw = (StartupWindow*)cData;
    ASSERT(suw);
    if (wait3 (NULL, WNOHANG, NULL) == StartupWindow::GarPid) {
	StartupWindow::GarPid = -1;
	suw->resetGarConnection();
    } else {
	XtAppAddTimeOut (theApplication->getApplicationContext(), 3000,
	    (XtTimerCallbackProc)StartupWindow_WaitPidTO, (XtPointer)suw);
    }
#endif
}


#endif
