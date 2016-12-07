/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#include <sys/types.h>
#include <time.h>
#include <sys/stat.h>
#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif


#include "TypeChoice.h"
#include "GARChooserWindow.h"
#include "GARMainWindow.h"
#include "GARApplication.h"
#include "NoUndoChoiceCommand.h"
#include "ButtonInterface.h"
#include "ToggleButtonInterface.h"
#include "DXStrings.h"
#include "WarningDialogManager.h"
#include "MsgDialog.h"

#include <Xm/Form.h>
#include <Xm/Label.h>
#include <Xm/Separator.h>

MsgDialog *TypeChoice::Messages = NUL(MsgDialog*);

DXLConnection* TypeChoice::ExecConnection = NUL(DXLConnection*);
DXLConnection* TypeChoice::DxuiConnection = NUL(DXLConnection*);

String TypeChoice::DefaultResources[] = 
{
    ".leftOffset:			30",
    ".rightOffset:			2",

    "*Header.labelString:		Settings",
    "*Trailer.labelString:		Operations",
    "*Header.foreground:		#26a",
    "*Trailer.foreground:		#26a",

    "*browseOption.labelString:		Browse Data...",
    "*browseOption.topOffset:		10",
    "*browseOption.width:		135",

    "*verifyOption.labelString:		Test Import...",
    "*verifyOption.topOffset:		2",
    "*verifyOption.width:		135",

    "*visualizeOption.labelString:	Visualize Data...",
    "*visualizeOption.topOffset:	2",
    "*visualizeOption.width:		135",

    "*prompterOption.labelString:	Full Prompter...",
    "*prompterOption.width:		135",
    "*prompterOption.topOffset:		2",

    "*simplePrompterOption.labelString:		Describe Data...",
    "*simplePrompterOption.width:		135",
    "*simplePrompterOption.topOffset:		2",

    NUL(char*)
};

XtInputId TypeChoice::InputId = 0;
XtWorkProcId TypeChoice::SeekerWP = 0;

TypeChoice::TypeChoice (
    const char*	name, 
    boolean	browsable, boolean	testable,
    boolean	visualizable, boolean	prompterable,
    GARChooserWindow *gcw,
    Symbol sym
) : UIComponent(name)
{
    this->browsable = browsable;
    this->testable = testable;
    this->visualizable = visualizable;
    this->prompterable = prompterable;
    ASSERT(gcw);
    this->gcw = gcw;
    this->browseCmd = this->visualizeCmd = NUL(Command*);
    this->verifyDataCmd = this->prompterCmd = NUL(Command*);
    this->simplePrompterCmd = NUL(Command*);
    this->choice_id = sym;
    this->net_to_run = NUL(char*);
    this->previous_data_file = NUL(char*);
    this->connection_mode = 0;

    //
    // Create all commands active and set them inactive if appropriate inside
    // setCommandActivation() so that the inactive help msg is associated.
    //
    // These commands could really be static since the ui never makes 2
    // different browse buttons available at the same time, however it's
    // nice to have the this pointer around when the command is executed.
    //
    if (this->browsable)
	this->browseCmd =
	    new NoUndoChoiceCommand ("browse", this->gcw->commandScope,
		TRUE, this, NoUndoChoiceCommand::Browse);

    if (this->visualizable)
	this->visualizeCmd =
	    new NoUndoChoiceCommand ("visualize", this->gcw->commandScope,
		TRUE, this, NoUndoChoiceCommand::Visualize);

    if (this->testable)
	this->verifyDataCmd =
	    new NoUndoChoiceCommand ("verifyData", this->gcw->commandScope,
		TRUE, this, NoUndoChoiceCommand::VerifyData);

    if (this->prompterable) {
	this->prompterCmd =
	    new NoUndoChoiceCommand ("fullPrompter", this->gcw->commandScope,
		TRUE, this, NoUndoChoiceCommand::Prompter);
	this->simplePrompterCmd =
	    new NoUndoChoiceCommand ("simplePrompter", this->gcw->commandScope,
		TRUE, this, NoUndoChoiceCommand::SimplePrompter);
    }

    this->setChoiceCmd =
	new NoUndoChoiceCommand ("prompter", this->gcw->commandScope,
	    TRUE, this, NoUndoChoiceCommand::SetChoice);
}

TypeChoice::~TypeChoice()
{
    if (this->prompterCmd) delete this->prompterCmd;
    if (this->verifyDataCmd) delete this->verifyDataCmd;
    if (this->visualizeCmd) delete this->visualizeCmd;
    if (this->browseCmd) delete this->browseCmd;
    if (this->setChoiceCmd) delete this->setChoiceCmd;
    if (this->net_to_run) delete this->net_to_run;
    if (this->previous_data_file) delete this->previous_data_file;
}


void TypeChoice::createChoice (Widget notebook)
{
    this->initialize();

    this->radio_button = 
	new ToggleButtonInterface (notebook, "typeOption", this->setChoiceCmd,
	    FALSE, this->getActiveHelpMsg());

    char *cp = DuplicateString(this->getFormalName());
    XmString xmstr = XmStringCreateLtoR(cp, "bold");
    delete cp;
    XtVaSetValues (this->radio_button->getRootWidget(),
	XmNleftAttachment,	XmATTACH_FORM,
	XmNrightAttachment,	XmATTACH_OPPOSITE_FORM,
	XmNlabelString,		xmstr,
	XmNrightOffset,		-360,
    NULL);
    XmStringFree(xmstr);

    this->expand_form = XtVaCreateWidget (this->name,
	xmFormWidgetClass,      notebook,
	XmNleftAttachment,      XmATTACH_FORM,
	XmNrightAttachment,     XmATTACH_FORM,
	XmNtopAttachment,	XmATTACH_WIDGET,
	XmNtopWidget,		this->radio_button->getRootWidget(),
	XmNtopOffset,		0,
	XmNshadowThickness,	2,
	XmNshadowType,		XmSHADOW_ETCHED_IN,
    NULL);
    this->setRootWidget(this->expand_form);

    Widget sep = NUL(Widget);
#if LOOSEN_UP
    if (this->hasSettings()) {
	XtVaCreateManagedWidget ("Header",
	    xmLabelWidgetClass,     this->expand_form,
	    XmNrightAttachment,     XmATTACH_FORM,
	    XmNrightOffset,         20,
	    XmNtopAttachment,       XmATTACH_FORM,
	    XmNtopOffset,           0,
	NULL);

	sep = XtVaCreateManagedWidget ("sep",
	    xmSeparatorWidgetClass, this->expand_form,
	    XmNleftAttachment,      XmATTACH_FORM,
	    XmNrightAttachment,     XmATTACH_FORM,
	    XmNtopAttachment,       XmATTACH_FORM,
	    XmNleftOffset,          0,
	    XmNrightOffset,         0,
	    XmNtopOffset,           12,
	NULL);
    }
#endif

    Widget attach = this->createBody(this->expand_form, sep);
    if (!attach) {
	attach = sep;
	XtVaSetValues (this->expand_form,
	    XmNshadowThickness,		0,
	NULL);
    }

#if LOOSEN_UP
    XtVaCreateManagedWidget ("Trailer",
	xmLabelWidgetClass,	this->expand_form,
	XmNrightAttachment,	XmATTACH_FORM,
	XmNrightOffset,		20,
	XmNtopAttachment,	(attach?XmATTACH_WIDGET:XmATTACH_FORM),
	XmNtopWidget,		attach,
	XmNtopOffset,		2,
    NULL);
#endif

    sep = XtVaCreateManagedWidget ("sep",
	xmSeparatorWidgetClass,	this->expand_form,
	XmNleftAttachment,	XmATTACH_FORM,
	XmNrightAttachment,	XmATTACH_FORM,
	XmNtopAttachment,	(attach?XmATTACH_WIDGET:XmATTACH_FORM),
	XmNtopWidget,		attach,
	XmNleftOffset,		14,
	XmNrightOffset,		0,
#if LOOSEN_UP
	XmNtopOffset,		4,
#else
	XmNtopOffset,		(attach?0:2),
	XmNshadowThickness,	0,
#endif
    NULL);


    ButtonInterface *option[6];
    Widget top = sep;
    int num_buts = 0;

    if (this->browsable) {
	option[num_buts] = new 
	    ButtonInterface(this->expand_form, "browseOption", this->browseCmd,
		"View the ascii data file.");
	this->setAttachments(option, num_buts, top);
	num_buts++;
    }

    if (this->testable) {
	option[num_buts] = new ButtonInterface
	    (this->expand_form,"verifyOption", this->verifyDataCmd,
	   "Test importation and report the structure of your data file.");
	this->setAttachments(option, num_buts, top);
	num_buts++;
    }

    if (this->visualizable) {
	option[num_buts] = new ButtonInterface
	    (this->expand_form,"visualizeOption", this->visualizeCmd,
		"Run a sample visual program using your data?");
	this->setAttachments(option, num_buts, top);
	num_buts++;
    }

    if (this->prompterable) {
	//option[num_buts] = new ButtonInterface
	//    (this->expand_form,"prompterOption", this->prompterCmd,
	//	"Open the full data prompter window.");
	//this->setAttachments(option, num_buts, top);
	//num_buts++;

	option[num_buts] = new ButtonInterface
	    (this->expand_form,"simplePrompterOption", this->simplePrompterCmd,
		"Open a new window which allows you to describe your data.");
	this->setAttachments(option, num_buts, top);
	num_buts++;
    }
}

void TypeChoice::setAttachments(ButtonInterface *option[], int num_buts, Widget top)
{
    if ((num_buts & 0x1)==0) {
	XtVaSetValues (option[num_buts]->getRootWidget(),
	    XmNleftAttachment, 	XmATTACH_FORM,
	    XmNleftOffset,	14,
	    XmNbottomAttachment,XmATTACH_FORM,
	    XmNbottomOffset,	6,
	NULL);
    } else {
	XtVaSetValues (option[num_buts]->getRootWidget(),
	    XmNleftAttachment, 	XmATTACH_WIDGET,
	    XmNleftWidget,	option[num_buts-1]->getRootWidget(),
	    XmNleftOffset,	10,
	    XmNbottomAttachment,XmATTACH_FORM,
	    XmNbottomOffset,	6,
	NULL);
    }
    if (num_buts < 2) {
	XtVaSetValues (option[num_buts]->getRootWidget(),
	    XmNtopAttachment,	XmATTACH_WIDGET,
	    XmNtopWidget,	top,
	    XmNtopOffset,	8,
	NULL);
    } else {
	XtVaSetValues (option[num_buts]->getRootWidget(),
	    XmNtopAttachment,	XmATTACH_WIDGET,
	    XmNtopWidget,	option[num_buts-2]->getRootWidget(),
	    XmNtopOffset,	4,
	NULL);
    }
}



boolean TypeChoice::selected()
{
    return this->radio_button->getState();
}

void TypeChoice::setSelected (boolean state)
{
    if (state == this->selected()) return ;
    this->radio_button->setState(state, TRUE);
}

Widget TypeChoice::getOptionWidget()
{
    return this->radio_button->getRootWidget();
}

void TypeChoice::hideList()
{
    if (!TypeChoice::Messages) return ;
    if (!TypeChoice::Messages->getRootWidget()) return ;

    TypeChoice::Messages->unmanage();
}

int TypeChoice::getMessageCnt()
{
    if (!TypeChoice::Messages) return  0;
    return TypeChoice::Messages->getMessageCount();
}

void TypeChoice::showList()
{
    if (!TypeChoice::Messages) 
	TypeChoice::Messages = new MsgDialog(this->gcw->getRootWidget(), this);
    TypeChoice::Messages->post();
}

void TypeChoice::appendMsg (const char *msg, boolean is_info)
{
XmString xmstr;
boolean popup_the_dialog = (is_info == FALSE);

    if (is_info) {
	char *tmp = DuplicateString(msg);
	if (!tmp) return ;
	int len = strlen(tmp);
	if ((len) && (tmp[len-1] == '\n')) tmp[len-1] = '\0';
	xmstr = XmStringCreateLtoR (tmp, "normal");
	delete tmp;
    } else {
	boolean is_warning = FALSE;
	char *tmp = DuplicateString(msg);
	char *cp = (char *) strstr(msg, "ERROR");
	XmString xmstr2;
	if (cp) {
	    cp+= strlen("ERROR");
	    xmstr2 = XmStringCreateLtoR (cp, "oblique");
	} else {
	    cp = (char *) strstr(msg, "WARNING");
	    if (cp) {
		cp+= strlen("WARNING");
		xmstr2 = XmStringCreateLtoR (cp, "oblique");
		is_warning = TRUE;
		popup_the_dialog = FALSE;
	    } else {
		xmstr2 = XmStringCreateLtoR (tmp, "oblique");
	    }
	}
	if (EqualString(msg, "Connection to DX broken"))
	    popup_the_dialog = FALSE;
	delete tmp;
	XmString xmstr1;
	if (is_warning) 
	    xmstr1 = XmStringCreateLtoR ("WARNING: ", "bold");
	else if (popup_the_dialog == FALSE)
	    xmstr1 = XmStringCreateLtoR ("STATUS: ", "oblique");
	else
	    xmstr1 = XmStringCreateLtoR ("ERROR: ", "bold");
	xmstr = XmStringConcat (xmstr1, xmstr2);
	XmStringFree(xmstr1);
	XmStringFree(xmstr2);
    }
    if (!TypeChoice::Messages)
	TypeChoice::Messages = new MsgDialog(this->gcw->getRootWidget(), this);
    TypeChoice::Messages->addItem(xmstr, 0);
    XmStringFree(xmstr);
    if (popup_the_dialog)
	this->showList();
}




boolean TypeChoice::browse()
{
    GARMainWindow *gmw = theGARApplication->getMainWindow(TRUE);
    ASSERT(gmw);

    char *fname = NULL;
    const char *cp = this->gcw->getDataFilename();

    if ((cp) && (cp[0])) {
	fname = DuplicateString(cp);
	gmw->postBrowser(fname);
	delete fname;
    } else
	this->setCommandActivation();

    return TRUE;
}

boolean
TypeChoice::connect(int mode)
{
#define DXLINK_SUPPORTS_REUSE 1
#if DXLINK_SUPPORTS_REUSE
    //
    // This chunk of code hangs onto old DXLConnections so that we can reuse them.
    // It's a big performance improvement.  I originally opted not to use it because
    // of purify problems.  I believe the purify problems went away when I removed
    // all handling of dxlink events from the message handler (TypeChoice_NetSeekerCB)
    // Instead it queues something to happen down the road.  Otherwise dxlink gets
    // totally confused.  And so I put the code back in.
    //
    ASSERT ((this->gcw->connection == NUL(DXLConnection*)) ||
	    (this->gcw->connection == TypeChoice::ExecConnection) ||
	    (this->gcw->connection == TypeChoice::DxuiConnection));
    if (mode != this->connection_mode) {
	switch (mode) {
	    case TypeChoice::ImageMode:
	    case TypeChoice::EditMode:
	    case TypeChoice::UserChoice:
		this->gcw->connection = TypeChoice::DxuiConnection;
		break;

	    case TypeChoice::ExecMode:
		this->gcw->connection = TypeChoice::ExecConnection;
		break;
	}
	if (this->gcw->connection)
	    this->connection_mode = mode;
    }
#endif

    //
    // Must decide what to do with the dxui/dxexec if we need to run in
    // a mode other than the one in which we most recently started.
    //
    if ((this->gcw->connection) && (mode != this->connection_mode)) {
	boolean must_exit = FALSE;
	switch(mode) {
	    case TypeChoice::ImageMode:
		if (this->connection_mode == TypeChoice::ExecMode)
		    must_exit = TRUE;
		else if (this->connection_mode == TypeChoice::EditMode) 
		    uiDXLCloseVPE(this->gcw->connection);
		break;
	    case TypeChoice::EditMode:
		if (this->connection_mode == TypeChoice::ImageMode) {
		    uiDXLOpenVPE(this->gcw->connection);
		} else {
		    must_exit = TRUE;
		}
		break;
	    case TypeChoice::ExecMode:
		must_exit = TRUE;
		break;
	    case TypeChoice::UserChoice:
		must_exit = TRUE;
		break;
	    default:
		ASSERT(0);
	}
	if (must_exit) {
	    XtRemoveInput(TypeChoice::InputId);
	    TypeChoice::InputId = 0;
	    DXLExitDX(this->gcw->connection);
	    if (this->gcw->connection == TypeChoice::ExecConnection)
		TypeChoice::ExecConnection = NUL(DXLConnection*);
	    else if (this->gcw->connection == TypeChoice::DxuiConnection)
		TypeChoice::DxuiConnection = NUL(DXLConnection*);
	    this->gcw->connection = NUL(DXLConnection*);
	}
    }

    if (!this->gcw->connection) {
	int port = theGARApplication->getResourcesPort(); 
	if (port) {
	    this->gcw->connection = DXLConnectToRunningServer (port, NULL);
	} else {
	    char* delete_mem_please = NUL(char*);
	    const char *exec = theGARApplication->getResourcesExec();
	    switch (mode) {
		//
		// It's really necessary to use -menubar here.  The program just
		// won't work if it actually tries to start -image.  Reasons:
		// 1) decision.net makes things look crude if you load it -image.
		// 2) Window placement will break kdb accelerators if you load
		// a net after you started -image.
		case TypeChoice::ImageMode:
		    exec = 
			"dx -processors 1 -menubar -wizard "
			"-xrm DX.dxAnchor.geometry:410x56+0+0 "
			"-xrm DX.editorWindow.geometry:200x150-0+0";
#if !defined(DXD_OS_NON_UNIX) && defined(DXD_LICENSED_VERSION)
		    if (!cp) cp = theGARApplication->getResourcesDxuiArgs();
		    if (!cp) cp = theGARApplication->getDxuiArgs();
		    if (cp) {
			char* newe = new char[4 + strlen(exec) + strlen(cp)];
			sprintf (newe, "%s %s", exec, cp);
			exec = delete_mem_please = newe;
		    }
#endif
		    break;
		case TypeChoice::EditMode:
		    exec = "dx -uionly ";
		    break;
		case TypeChoice::ExecMode:
		    exec = "dx -execonly -optimize memory -processors 1";
		    break;
		case TypeChoice::UserChoice:
		    break;
		default:
		    ASSERT(0);
	    }
	    this->gcw->connection = DXLStartDX (exec, NULL);
	    if (delete_mem_please) delete delete_mem_please;
	    this->connection_mode = mode;
#if DXLINK_SUPPORTS_REUSE
	    if (mode == TypeChoice::ExecMode)
		TypeChoice::ExecConnection = this->gcw->connection;
	    else
		TypeChoice::DxuiConnection = this->gcw->connection;
#endif
	}
    }

    //
    // Must remove/reset these message handlers as the type
    // is picked in GARChooserWindow.  If you don't then the only TypeChoice
    // object which will recieve messages will be the one which was first
    // to try and connect
    //
    if (this->gcw->connection) {
	if (TypeChoice::InputId) {
	    XtRemoveInput (TypeChoice::InputId);
	    TypeChoice::InputId = 0;
	}
	TypeChoice::InputId =
	    XtAppAddInput (theApplication->getApplicationContext(), 
		DXLGetSocket(this->gcw->connection),
#ifndef DXD_WIN
		(XtPointer)XtInputReadMask,
#else
		(XtPointer)XtInputReadWinsock,
#endif
		(XtInputCallbackProc)TypeChoice_HandleMessagesCB,
		(XtPointer)this->gcw);
	DXLSetBrokenConnectionCallback (this->gcw->connection,
	    TypeChoice_BrokenConnCB, (void*)this);
	DXLSetMessageHandler (this->gcw->connection, PACK_INFO, NUL(char*), 
	    (DXLMessageHandler)TypeChoice_InfoMsgCB, (const void *)this);
	//
	// Currently, the error handler is being called by dxlink to service warnings.
	//
	DXLSetErrorHandler (this->gcw->connection, 
	    (DXLMessageHandler)TypeChoice_ErrorMsgCB, (const void *)this);
	DXLSetValueHandler (this->gcw->connection, "net_to_run",
	    (DXLValueHandler)TypeChoice_NetSeekerCB, (const void*)this);
    }

    if (!this->gcw->connection) {
	WarningMessage ("Could not connect to dxexec.");
	return FALSE;
    }

    if (theGARApplication->getResourcesDebug()) {
	DXLSetMessageDebugging (this->gcw->connection, 1);
    } else {
	DXLSetMessageDebugging (this->gcw->connection, 0);
    }
    return TRUE;
}

void TypeChoice::manage()
{
    this->UIComponent::manage();
    if (this->gcw->connection) {
	DXLRemoveValueHandler (this->gcw->connection, "net_to_run");
	    
	if (TypeChoice::InputId) {
	    XtRemoveInput(TypeChoice::InputId);
	    TypeChoice::InputId = 0;
	}

	//
	// Currently, the error handler is being called by dxlink to service warnings.
	//
	DXLRemoveMessageHandler (this->gcw->connection, PACK_ERROR, "Error",
	    (DXLMessageHandler)TypeChoice_ErrorMsgCB);

	DXLSetValueHandler (this->gcw->connection, "net_to_run",
	    (DXLValueHandler)TypeChoice_NetSeekerCB, (const void*)this);
	DXLSetErrorHandler (this->gcw->connection, 
	    (DXLMessageHandler)TypeChoice_ErrorMsgCB, (const void *)this);

	TypeChoice::InputId =
	    XtAppAddInput (theApplication->getApplicationContext(), 
		DXLGetSocket(this->gcw->connection),
#ifndef DXD_WIN
		(XtPointer)XtInputReadMask,
#else
		(XtPointer)XtInputReadWinsock,
#endif
		(XtInputCallbackProc)TypeChoice_HandleMessagesCB,
		(XtPointer)this->gcw);
    }
}

void TypeChoice::unmanage()
{
    this->UIComponent::unmanage();
}

boolean TypeChoice::verify(const char *seek)
{
    if (!this->connect(TypeChoice::ImageMode)) return FALSE;

    const char *net_dir = theGARApplication->getResourcesNetDir();
    char net_file[512];
    sprintf (net_file, "%s/ui/decision.net", net_dir);
    DXLConnection* conn = this->gcw->getConnection();
    DXLLoadVisualProgram (conn, net_file);

    const char *cp = this->gcw->getDataFilename();

    DXLSetString (conn, "test_file", cp);
    DXLSetString (conn, "test_format", this->getImportType());
    if (!seek) {
	DXLSetValue (conn, "DescribeOrVisualize", "2");
	//
	// Commeneted out because we're running in ImageMode and dxui MsgWin
	// will get the messages.  We won't.
	//this->gcw->showMessages();
    } else {
	DXLSetValue (conn, "DescribeOrVisualize", seek);
    }

    DXLExecuteOnce (conn);

    return TRUE;
}

boolean TypeChoice::visualize()
{
    char net_file[512];
    const char *net_dir = theGARApplication->getResourcesNetDir();
    if (!this->net_to_run) {
	return this->verify("1");
    }

    if (!this->connect(TypeChoice::ImageMode)) return FALSE;
    DXLConnection* conn = this->gcw->getConnection();

    sprintf (net_file, "%s/ui/%s", net_dir, this->net_to_run);
    char *cp = GARApplication::FileFound(net_file, NUL(char*));
    if ((cp) && (cp[0])) {
	char* args[4];
	args[0] = "_filename_";
	args[1] = DuplicateString(this->gcw->getDataFilename());
	args[2] = "_filetype_";
	args[3] = DuplicateString(this->getImportType());
	this->gcw->loadAndSet (conn, net_file, args, 4);
	delete args[1];
	delete args[3];
	uiDXLOpenAllImages (conn);

	this->hideList();
	DXLExecuteOnce(conn);
	delete cp;
	return TRUE;
    } else {
	WarningMessage ("Could not find %s", net_file);
	return FALSE;
    }
}


//
// file_checked means that the caller has looked for the file and found
// that is exists.
//
void TypeChoice::setCommandActivation(boolean file_checked)
{
boolean file_ready = FALSE;
char *fname = NUL(char*);

    //
    // Does the text widget at the top contain a valid data file name?
    //
    if (!file_checked) {
	const char *cp = this->gcw->getDataFilename();
	if ((cp) && (cp[0])) {
	    fname = theGARApplication->resolvePathName(cp, this->getFileExtension());
	    if (fname) {
		file_ready = TRUE;
	    }
	}
    } else
	file_ready = TRUE;

    GARMainWindow *gmw = theGARApplication->getMainWindow();
    if (file_ready) {
        if (this->isBrowsable()) {
            this->browseCmd->activate();
        } 
        if (this->isTestable()) {
	    this->verifyDataCmd->activate();
        } 

        if (this->visualizable) 
	    this->visualizeCmd->activate();

	//
	// If the file changed, then toss out net_to_run
	//
	ASSERT(fname);
	
	boolean file_changed = FALSE;
	if (!this->previous_data_file)
	    file_changed = TRUE;
	else if (!EqualString (this->previous_data_file, fname)) 
	    file_changed = TRUE;

	if (file_changed) {
	    if (this->net_to_run) delete this->net_to_run;
	    this->net_to_run = NUL(char*);
	    if (this->previous_data_file) delete this->previous_data_file;
	    this->previous_data_file = NUL(char*);
	}
    } else {
	if (this->net_to_run) {
	    delete this->net_to_run;
	    this->net_to_run = NUL(char*);
	    if (this->previous_data_file) delete this->previous_data_file;
	    this->previous_data_file = NUL(char*);
	}
	if (this->isBrowsable()) 
	    this->browseCmd->deactivate ("You must specify a data file name, first.");
	if (this->isTestable()) 
	    this->verifyDataCmd->deactivate ("You must specify a data file name, first.");
	if (this->isVisualizable()) 
	    this->visualizeCmd->deactivate ("You must specify a data file name, first.");
    }
    if (fname) {
	if (this->previous_data_file) delete this->previous_data_file;
	this->previous_data_file = DuplicateString(fname);
	delete fname;
    }
}

CommandScope*
TypeChoice::getCommandScope() { return this->gcw->commandScope; }


boolean
TypeChoice::setChoice()
{
    if (this->net_to_run) {
	delete this->net_to_run;
	this->net_to_run = NUL(char*);
    }
    if (this->previous_data_file) {
	delete this->previous_data_file;
	this->previous_data_file = NUL(char*);
    }
    this->gcw->setChoice(this);
    return TRUE;
}

int
TypeChoice::getHeightContribution()
{
    if (this->radio_button->getState()) 
	return this->expandedHeight();
    return 30;
}

void
TypeChoice::netFoundCB(const char *value)
{
boolean had_net = (this->net_to_run != NUL(char*));

    if (value) {
	if (this->net_to_run) delete this->net_to_run;
	this->net_to_run = NUL(char*);
	this->net_to_run = DuplicateString(value);
	if (!had_net)
	    this->setCommandActivation(FALSE);
    } else {
	this->setCommandActivation(FALSE);
    }

    if (this->net_to_run)
	this->visualizeCmd->execute();
}

extern "C" void
TypeChoice_InfoMsgCB (DXLConnection *, const char *msg, const void *data)
{
    TypeChoice *tc = (TypeChoice*)data;
    if (!tc->isTestable()) return ;
    tc->appendMsg(msg, TRUE);
}

extern "C" void
TypeChoice_NetSeekerCB (DXLConnection *, const char* , const char* value, void *data)
{
TypeChoice *tc = (TypeChoice*)data;
char *str = NUL(char*);

    ASSERT(tc);
    if (value) {
	// remove trailing blanks.
	str = DuplicateString(value);
	int i = strlen(str) - 1;
	while (str[i] == ' ') str[i--] = '\0';
    }
    if (TypeChoice::SeekerWP) {
	XtRemoveWorkProc (TypeChoice::SeekerWP);
	TypeChoice::SeekerWP = 0;
    }
    if (tc->net_to_run) delete tc->net_to_run;
    tc->net_to_run = str;
    if ((str) && (str[0])) {
	XtAppContext apcxt = theApplication->getApplicationContext();
	TypeChoice::SeekerWP =
	    XtAppAddWorkProc (apcxt, (XtWorkProc)TypeChoice_SeekerWP, (XtPointer)tc);
    }
}

extern "C" Boolean TypeChoice_SeekerWP (XtPointer clientData)
{
    TypeChoice* tc = (TypeChoice*)clientData;
    ASSERT(tc);
    TypeChoice::SeekerWP = 0;
    tc->netFoundCB ();
    return True;
}

extern "C" void
TypeChoice_ErrorMsgCB (DXLConnection *, const char *msg, const void *data)
{
    TypeChoice *tc = (TypeChoice*)data;
    if (!tc->isTestable()) return ;
    //
    // in addition to removing this snippet, remove "ERROR" (happens later)
    //
    const char *cp = strstr(msg, "main:0/");
    if (cp) {
	cp+= 7;
	tc->appendMsg(cp, FALSE);
    } else {
	tc->appendMsg(msg, FALSE);
    }
}

extern "C" void
TypeChoice_BrokenConnCB (DXLConnection* , void* clientData)
{
    TypeChoice* tc = (TypeChoice*)clientData;
    ASSERT(tc);
    if (tc->gcw->connection == TypeChoice::ExecConnection)
	TypeChoice::ExecConnection = NUL(DXLConnection*);
    else if (tc->gcw->connection == TypeChoice::DxuiConnection)
	TypeChoice::DxuiConnection = NUL(DXLConnection*);
    tc->gcw->connection = NUL(DXLConnection*);
    if (TypeChoice::InputId) {
	XtRemoveInput(TypeChoice::InputId);
	TypeChoice::InputId = 0;
    }
}


extern "C" void
TypeChoice_HandleMessagesCB (XtPointer cData, int* ,  XtInputId* )
{
    if (TypeChoice::InputId == 0) return ;
    GARChooserWindow* gcw = (GARChooserWindow*)cData;
    DXLHandlePendingMessages(gcw->connection);
}
