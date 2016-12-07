/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"





#include <ctype.h>
#include <errno.h>
#include <string.h>

#include <Xm/CascadeB.h>
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/List.h>
#include <Xm/RowColumn.h>
#include <Xm/Separator.h>

#include "DXStrings.h"
#include "MsgWin.h"

#include "DXApplication.h"
#include "ButtonInterface.h"
#include "EditorWindow.h"
#include "ErrorDialogManager.h"
#include "QuestionDialogManager.h"
#include "ListIterator.h"
#include "MWClearCmd.h"
#include "MWFileDialog.h"
#include "ExecCommandDialog.h"
#include "Network.h"
#include "NoUndoMWCmd.h"
#include "ToggleButtonInterface.h"
#include "RepeatingToggle.h"
#include "DXPacketIF.h"

#include "MWDefaultResources.h"

boolean MsgWin::ClassInitialized = FALSE;

//
// Constructor:
//
MsgWin::MsgWin(): DXWindow("messageWindow", FALSE)
{

    this->fileMenu = NULL;
    this->editMenu = NULL;
    this->executeMenu = NULL;
    this->optionsMenu = NULL;
    this->helpMenu = NULL;

    this->nextErrorOption =  NULL;
    this->prevErrorOption =  NULL;
    this->clearOption = NULL; 
    this->logOption = NULL; 
    this->saveOption = NULL; 
    this->closeOption = NULL; 
    this->infoOption = NULL; 
    this->warningOption = NULL; 
    this->errorOption = NULL; 
    this->execScriptOption = NULL; 

    this->clearCmd = new MWClearCmd("clear", this->commandScope, FALSE, this);
    this->logCmd   = new NoUndoMWCmd("log", this->commandScope, TRUE,
	this, NoUndoMWCmd::Log);
    this->saveCmd  = new NoUndoMWCmd("save", this->commandScope, TRUE, 
	this, NoUndoMWCmd::Save);
    this->closeCmd = new NoUndoMWCmd("close", this->commandScope, TRUE, 
	this, NoUndoMWCmd::Close);
    this->nextErrorCmd = new NoUndoMWCmd("nextError", this->commandScope, TRUE, 
	this, NoUndoMWCmd::NextError);
    this->prevErrorCmd = new NoUndoMWCmd("prevError", this->commandScope, TRUE, 
	this, NoUndoMWCmd::PrevError);

    if (theDXApplication->appAllowsScriptCommands()) {
	this->execScriptCmd = new NoUndoMWCmd("execScript", this->commandScope, 
    			theDXApplication->disconnectFromServerCmd->isActive(),
			this, NoUndoMWCmd::ExecScript);

	theDXApplication->connectedToServerCmd->autoActivate(this->execScriptCmd);
	theDXApplication->disconnectedFromServerCmd->autoDeactivate(this->execScriptCmd);

	this->traceCmd = new NoUndoMWCmd ("traceOn", this->commandScope,
	    TRUE, this, NoUndoMWCmd::Tracing);
#if 0
	theDXApplication->connectedToServerCmd->autoActivate(this->traceCmd);
#endif

	this->memoryCmd = new NoUndoMWCmd ("memory", this->commandScope,
	    FALSE, this, NoUndoMWCmd::Memory);
	theDXApplication->connectedToServerCmd->autoActivate(this->memoryCmd);
	theDXApplication->disconnectedFromServerCmd->autoDeactivate(this->memoryCmd);
    } else {
	this->execScriptCmd = NULL; 
	this->traceCmd = NUL(Command*);
	this->memoryCmd = NUL(Command*);
    }

    this->intervalId = 0;
    this->firstMsg   = FALSE;
    this->executing  = FALSE;
    this->beenManaged= FALSE;

    this->logFile    = NULL;
    this->saveDialog = NULL;
    this->logDialog  = NULL;
    this->execCommandDialog = NULL;


    //
    // Install the default resources for THIS class (not the derived classes)
    //
    if (NOT MsgWin::ClassInitialized)
    {
	ASSERT(theApplication);
        MsgWin::ClassInitialized = TRUE;
	this->installDefaultResources(theApplication->getRootWidget());
    }
}
//
// Destructor:
//
MsgWin::~MsgWin()
{
    this->clear();

    if (this->nextErrorOption)
    	delete this->nextErrorOption;
    if (this->prevErrorOption) 
    	delete this->prevErrorOption;
    if (this->clearOption) 
    	delete this->clearOption;
    if (this->logOption) 
    	delete this->logOption; 
    if (this->saveOption) 
    	delete this->saveOption; 
    if (this->closeOption) 
    	delete this->closeOption; 
    if (this->infoOption) 
    	delete this->infoOption; 
    if (this->warningOption)
    	delete this->warningOption;
    if (this->errorOption)
    	delete this->errorOption;
    if (this->execScriptOption)
    	delete this->execScriptOption;

    delete this->clearCmd;
    delete this->logCmd;
    delete this->saveCmd;
    delete this->closeCmd;
    delete this->nextErrorCmd;
    delete this->prevErrorCmd;
    if (this->execScriptCmd)
        delete this->execScriptCmd;
    if (this->traceCmd)
	delete this->traceCmd;
    if (this->memoryCmd)
	delete this->memoryCmd;
	

    //
    // for now, logDialog and fileDialog are the same.
    //
    if (this->logDialog)
	delete this->logDialog;

    if (this->execCommandDialog)
	delete this->execCommandDialog;
}


//
// Install the default resources for this class.
//
void MsgWin::installDefaultResources(Widget baseWidget)
{
    this->setDefaultResources(baseWidget, MsgWin::DefaultResources);
    this->DXWindow::installDefaultResources(baseWidget);
}

void MsgWin::manage()
{
    this->beenManaged = TRUE;
    this->DXWindow::manage();
}

void MsgWin::createMenus(Widget parent)
{
    this->createFileMenu(parent);
    this->createEditMenu(parent);
    this->createExecuteMenu(parent);
    if (theDXApplication->appAllowsScriptCommands()) 
	this->createCommandsMenu(parent);
    this->createOptionsMenu(parent);
    this->createHelpMenu(parent);

    //
    // Right justify the help menu (if it exists).
    //
    if (this->helpMenu)
    {
        XtVaSetValues(parent, XmNmenuHelpWidget, this->helpMenu, NULL);
    }
}
void MsgWin::createFileMenu(Widget parent)
{
    ASSERT(parent);

    //
    // Create "File" menu and options.
    //
    Widget pulldown =
	this->fileMenuPulldown =
	    XmCreatePulldownMenu(parent, "fileMenuPulldown", NUL(ArgList), 0);
    this->fileMenu =
	XtVaCreateManagedWidget
	    ("fileMenu",
	     xmCascadeButtonWidgetClass,
	     parent,
	     XmNsubMenuId, pulldown,
	     NULL);

    this->logOption = new ToggleButtonInterface(pulldown, "msgLogOption",
	this->logCmd, this->logFile != NULL);
    this->saveOption = new ButtonInterface(pulldown, "msgSaveOption",
	this->saveCmd);

    XtVaCreateManagedWidget
	    ("fileSeparator", xmSeparatorWidgetClass, pulldown, NULL);

    this->closeOption = new ButtonInterface(pulldown, "msgCloseOption",
	this->closeCmd);
}

void MsgWin::createEditMenu(Widget parent)
{
    ASSERT(parent);

    //
    // Create "File" menu and options.
    //
    Widget pulldown =
	this->editMenuPulldown =
	    XmCreatePulldownMenu(parent, "editMenuPulldown", NUL(ArgList), 0);
    this->editMenu =
	XtVaCreateManagedWidget
	    ("editMenu",
	     xmCascadeButtonWidgetClass,
	     parent,
	     XmNsubMenuId, pulldown,
	     NULL);

    this->nextErrorOption = 
		new ButtonInterface(pulldown, 
			"msgNextErrorOption", this->nextErrorCmd);
    this->prevErrorOption = 
		new ButtonInterface(pulldown, 
			"msgPrevErrorOption", this->prevErrorCmd);

    XtVaCreateManagedWidget
	    ("editSeparator", xmSeparatorWidgetClass, pulldown, NULL);

    this->clearOption = new ButtonInterface(pulldown, "msgClearOption",
	this->clearCmd);

}

void MsgWin::createCommandsMenu(Widget parent)
{
    ASSERT(parent);
    if ((!this->traceCmd) && (!this->execScriptCmd) && (!this->memoryCmd)) return ;

    Widget pulldown = this->optionsMenuPulldown =
	XmCreatePulldownMenu(parent, "commandsMenuPulldown", NUL(ArgList), 0);
    this->commandsMenu =
	XtVaCreateManagedWidget
	    ("commandsMenu",
	     xmCascadeButtonWidgetClass,
	     parent,
	     XmNsubMenuId, pulldown,
	     NULL);

    if (this->traceCmd) 
	this->traceOption = new RepeatingToggle (pulldown, "msgTraceOption",
	    this->traceCmd, theDXApplication->showInstanceNumbers());

    if (this->execScriptCmd) 
	this->execScriptOption = new ButtonInterface(pulldown, 
	    "msgExecScriptOption", this->execScriptCmd);

    if (this->memoryCmd)
	this->memoryOption = new ButtonInterface (pulldown,
	    "msgShowMemoryOption", this->memoryCmd);
}

void MsgWin::createOptionsMenu(Widget parent)
{
    ASSERT(parent);

    //
    // Create "File" menu and options.
    //
    Widget pulldown = this->optionsMenuPulldown =
	XmCreatePulldownMenu(parent, "optionsMenuPulldown", NUL(ArgList), 0);
    this->optionsMenu =
	XtVaCreateManagedWidget
	    ("optionsMenu",
	     xmCascadeButtonWidgetClass,
	     parent,
	     XmNsubMenuId, pulldown,
	     NULL);

    if (theDXApplication->appAllowsMessageInfoOption())
	this->infoOption = new ToggleButtonInterface(pulldown, 
		"msgInfoOption", theDXApplication->toggleInfoEnable,
		theDXApplication->isInfoEnabled());

    if (theDXApplication->appAllowsMessageWarningOption())
	this->warningOption = new ToggleButtonInterface(pulldown, 
		"msgWarningOption", theDXApplication->toggleWarningEnable,
		theDXApplication->isWarningEnabled());

    this->errorOption = new ToggleButtonInterface(pulldown, "msgErrorOption",
	theDXApplication->toggleErrorEnable, theDXApplication->isErrorEnabled());
}

Widget MsgWin::createWorkArea(Widget parent)
{
    Widget top = XtVaCreateManagedWidget(
	"workAreaFrame", xmFrameWidgetClass, parent,
	XmNshadowType,		XmSHADOW_OUT,
	XmNmarginWidth,		5,
	XmNmarginHeight,	5,
	NULL);

    Widget form = XtVaCreateManagedWidget(
	"workArea", xmFormWidgetClass, top,
	NULL);

    Widget frame = XtVaCreateManagedWidget(
	"msgFrame", xmFrameWidgetClass, form,
	XmNtopAttachment,	XmATTACH_FORM,
	XmNbottomAttachment,	XmATTACH_FORM,
	XmNleftAttachment,	XmATTACH_FORM,
	XmNrightAttachment,	XmATTACH_FORM,
	NULL);

    Arg arg[10];
    XtSetArg(arg[0], XmNlistSizePolicy, XmCONSTANT);
    XtSetArg(arg[1], XmNselectionPolicy, XmSINGLE_SELECT);
    this->list = XmCreateScrolledList(frame, "msgList", arg, 2);
    this->installComponentHelpCallback(this->list);
    XtAddCallback(this->list,
		  XmNsingleSelectionCallback,
		  (XtCallbackProc)MsgWin_SelectCB,
		  (XtPointer)this);

    XtManageChild(this->list);

    return top;
}

void MsgWin::addInformation(const char *info)
{
    ASSERT(info);
    if (!this->isInitialized())
	this->initialize();
    if (!theDXApplication->isInfoEnabled())
	return;

    if (!info)
	return;

    const char *newLine = strchr(info, '\n');
    char *s = NULL;
    if (newLine)
    {
	s = DuplicateString(info);
	s[newLine - info] = '\0';
	info = s;
    }


    if (executing)
    {
	if (this->intervalId == 0)
	    this->intervalId = XtAppAddTimeOut(
		theApplication->getApplicationContext(),
		5000,
		(XtTimerCallbackProc)MsgWin_FlushTimeoutTO,
		(XtPointer)this);
	if (this->firstMsg)
	{
	    this->firstMsg = FALSE;
	    if (this->logFile)
	    {
		fputs("Begin Execution\n", this->logFile);
		fflush(this->logFile);
	    }
	    XmString s = XmStringCreate("Begin Execution", "oblique");
	    XmListAddItemUnselected(this->list, s, 0);
	    XmStringFree(s);
	    int itemCount;
	    XtVaGetValues(this->list, XmNitemCount, &itemCount, NULL);
	    XmListSetBottomPos(this->list, itemCount);

	    this->clearCmd->activate();
	}
	int vic;
	XtVaGetValues(this->list, XmNvisibleItemCount, &vic, NULL);
	this->batchedLines.appendElement((void*)DuplicateString(info));
	if (vic <= this->batchedLines.getSize())
	    this->flushBuffer();
    }
    else
    {
	if (this->logFile)
	{
	    fputs(info, this->logFile);
	    fputc('\n', this->logFile);
	    fflush(this->logFile);
	}
	XmString s = XmStringCreate((char*)info, "normal");
	XmListAddItemUnselected(this->list, s, 0);
	XmStringFree(s);
	int itemCount;
	XtVaGetValues(this->list, XmNitemCount, &itemCount, NULL);
	XmListSetBottomPos(this->list, itemCount);

	this->clearCmd->activate();
    }

    if (s != NULL)
	delete[] s;

    if (theDXApplication->doesInfoOpenMessage())
    {
	if (!this->isManaged())
	    this->manage();
    }
}

void MsgWin::infoOpen()
{
    if (!this->isInitialized())
	this->initialize();
    if (!theDXApplication->isInfoEnabled())
	return;
    if (!this->isManaged())
	this->manage();
}


void MsgWin::addError(const char *error)
{
	ASSERT(error);
	if (!theDXApplication->isErrorEnabled())
		return;
	if (!error)
		return;
	if (!this->isInitialized())
		this->initialize();

	this->flushBuffer();

	if (this->firstMsg)
	{
		this->firstMsg = FALSE;
		if (this->logFile)
		{
			fputs("Begin Execution\n", this->logFile);
			fflush(this->logFile);
		}
		XmString s = XmStringCreate("Begin Execution", "oblique");
		XmListAddItemUnselected(this->list, s, 0);
		XmStringFree(s);
		int itemCount;
		XtVaGetValues(this->list, XmNitemCount, &itemCount, NULL);
		XmListSetBottomPos(this->list, itemCount);

		this->clearCmd->activate();
	}

	//
	// Manage it before we save the error message, otherwise manage()
	// causes DXWindow::manage() to do a notify(BeginExecution) which clears 
	// the list of lines.
	//
	if (theDXApplication->doesErrorOpenMessage() && !this->isManaged())
		this->manage();

	//
	// If this is a module error, change the format and report it.
	//  0:  ERROR:  2::/main:0/Compute:0: Bad parameter: expression must be a string
	//
	char net[100];
	int  inst;
	const char *topNet = theDXApplication->network->getNameString();
	int l;
	if (sscanf(error, " %*d:  ERROR:  %*d::/%[^:]:%d/%n", net, &inst, &l) == 2 &&
		EqualString(net, topNet))
	{
		char *line = new char[STRLEN(error)+1];
		char *p = line;
		const char *o = error + l;
		boolean names = TRUE;
		boolean skipDigits = FALSE;
		boolean moreNames = TRUE;
		while(names && *o)
		{
			if (!skipDigits && *o == ':')
			{
				skipDigits = TRUE;
				if (moreNames)
				{
					*p++ = *o++;
					*p++ = ' ';
				}
				else
					o++;
			}
			else if (*o == '/')
			{
				skipDigits = FALSE;
				o++;
			}
			else if (skipDigits && isdigit(*o))
				o++;
			else if (skipDigits && *o == ':')
			{
				names = FALSE;
				o++;
			}
			else
			{
				const char *colon = strchr(o, ':');
				if (moreNames)
				{
					char modName[100];
					strncpy(modName, o, colon - o);
					modName[colon - o] = '\0';
					char netName[100];
					if (sscanf(modName, "%*[^_]_%[^_]_%*d", netName) == 1)
					{
						strcpy(p, netName);
						p += STRLEN(netName);
						*p++ = ':';
						moreNames = FALSE;
					}
					else
					{
						strncpy(p, modName, colon-o);
						p += colon-o;
					}
				}
				o = colon;
			}
		}
		*p = '\0';

		if (this->logFile)
		{
			fputs("ERROR: ", this->logFile);
			fputs(line, this->logFile);
			fputs(o, this->logFile);
			fputc('\n', this->logFile);
		}
		XmString errorString = XmStringCreate("ERROR: ", "bold");
		XmString nameString = XmStringCreate((char*)line, "bold");
		XmString text = XmStringCreate((char*)o, "oblique");
		XmString firstHalf = XmStringConcat(errorString, nameString);
		XmString s = XmStringConcat(firstHalf, text);
		XmStringFree(errorString);
		XmStringFree(nameString);
		XmStringFree(firstHalf);
		XmStringFree(text);
		delete[] line;

		XmListAddItemUnselected(this->list, s, 0);
		XmStringFree(s);
		int itemCount;
		XtVaGetValues(this->list, XmNitemCount, &itemCount, NULL);
		XmListSetBottomPos(this->list, itemCount);

		this->clearCmd->activate();

		SelectableLine *l = new SelectableLine;
		XtVaGetValues(this->list, XmNitemCount, &l->position, NULL);
		l->line = DuplicateString(error);
		this->selectableLines.appendElement((void*)l);
	}
	else
	{
		if (this->logFile)
		{
			fputs(error, this->logFile);
			fputc('\n', this->logFile);
		}
		XmString s = XmStringCreate((char*)error, "bold");

		XmListAddItemUnselected(this->list, s, 0);
		XmStringFree(s);
		int itemCount;
		XtVaGetValues(this->list, XmNitemCount, &itemCount, NULL);
		XmListSetBottomPos(this->list, itemCount);

		this->clearCmd->activate();
	}
	if (theDXApplication->doesErrorOpenMessage())
	{
		if (!this->beenBeeped)
		{
			XBell(XtDisplay(this->getRootWidget()), 0);
			XFlush(XtDisplay(this->getRootWidget()));
			this->beenBeeped = TRUE;
		}
		if (!this->isManaged())
			this->manage();
		else
			XRaiseWindow(XtDisplay(this->getRootWidget()),
			XtWindow(this->getRootWidget()));
	}

}
void MsgWin::addWarning(const char *warning)
{
    ASSERT(warning);
    if (!theDXApplication->isWarningEnabled())
	return;
    if (!this->isInitialized())
	this->initialize();
    if (!warning)
	return;

    this->flushBuffer();

    if (this->firstMsg)
    {
	this->firstMsg = FALSE;
	if (this->logFile)
	{
	    fputs("Begin Execution\n", this->logFile);
	    fflush(this->logFile);
	}
	XmString s = XmStringCreate("Begin Execution", "oblique");
	XmListAddItemUnselected(this->list, s, 0);
	XmStringFree(s);
	int itemCount;
	XtVaGetValues(this->list, XmNitemCount, &itemCount, NULL);
	XmListSetBottomPos(this->list, itemCount);

	this->clearCmd->activate();
    }

    if (this->logFile)
    {
	fputs(warning, this->logFile);
	fputc('\n', this->logFile);
	fflush(this->logFile);
    }
    XmString s = XmStringCreate((char*)warning, "normal");
    XmListAddItemUnselected(this->list, s, 0);
    XmStringFree(s);
    int itemCount;
    XtVaGetValues(this->list, XmNitemCount, &itemCount, NULL);
    XmListSetBottomPos(this->list, itemCount);

    if (theDXApplication->doesWarningOpenMessage())
    {
	if (!this->isManaged())
	    this->manage();
	else
	    XRaiseWindow(XtDisplay(this->getRootWidget()),
		   XtWindow(this->getRootWidget()));
    }
}

void MsgWin::beginExecution()
{
    this->DXWindow::beginExecution();

    this->firstMsg  = TRUE;
    this->executing = TRUE;
    this->beenBeeped = FALSE;

    this->clearSelectableLines();
}

void MsgWin::clearSelectableLines()
{
    ListIterator li(this->selectableLines);
    SelectableLine *l;
    while( (l = (SelectableLine*)li.getNext()) )
    {
	delete[] l->line;
	delete l;
    }
    this->selectableLines.clear();
    // XmListDeselectAllItems(this->list);
}

void MsgWin::endExecution()
{
    this->DXWindow::endExecution();

    this->executing = FALSE;
    this->flushBuffer();

    if (this->selectableLines.getSize() > 0)
    {
	this->findNextError(FALSE);
	this->nextErrorCmd->activate();
	this->prevErrorCmd->activate();
    }
    else
    {
	this->nextErrorCmd->deactivate();
	this->prevErrorCmd->deactivate();
    }
}

void MsgWin::standBy()
{
    this->DXWindow::standBy();

    this->executing = FALSE;
    this->flushBuffer();

    if (this->selectableLines.getSize() > 0)
    {
	this->findNextError(FALSE);
	this->nextErrorCmd->activate();
	this->prevErrorCmd->activate();
    }
    else
    {
	this->nextErrorCmd->deactivate();
	this->prevErrorCmd->deactivate();
    }
}

extern "C" void MsgWin_FlushTimeoutTO(XtPointer closure, XtIntervalId*)
{
    MsgWin *mw = (MsgWin*) closure;

    mw->intervalId = 0;
    mw->flushBuffer();
}
void MsgWin::flushBuffer()
{
    int nItems = this->batchedLines.getSize();
    if (nItems == 0)
	return;

    XmString *items = new XmString[nItems];
    ListIterator li(this->batchedLines);
    char *s;
    int i;
    for (i = 0; i < nItems && (s = (char*)li.getNext()); ++i)
    {
	if (this->logFile)
	{
	    fputs(s, this->logFile);
	    fputc('\n', this->logFile);
	}
	items[i] = XmStringCreate(s, "normal");
	delete[] s;
    }

    XmListAddItems(this->list, items, nItems, 0);

    if (this->logFile)
	fflush(this->logFile);

    int itemCount;
    XtVaGetValues(this->list, XmNitemCount, &itemCount, NULL);
    XmListSetBottomPos(this->list, itemCount);

    for (i = 0; i < nItems; ++i)
	XmStringFree(items[i]);
    delete[] items;

    this->batchedLines.clear();

    if (this->intervalId)
    {
	XtRemoveTimeOut(this->intervalId);
	this->intervalId = 0;
    }

    this->clearCmd->activate();
}

boolean MsgWin::clear()
{
    this->flushBuffer();
    if (this->intervalId)
    {
	XtRemoveTimeOut(this->intervalId);
	this->intervalId = 0;
    }

    int nItems = this->batchedLines.getSize();
    char *s;
    ListIterator li(this->batchedLines);
    for (int i = 0; i < nItems && (s = (char*)li.getNext()); ++i)
	delete[] s;
    this->batchedLines.clear();

    this->clearSelectableLines();

    XmListDeleteAllItems(this->list);

    this->clearCmd->deactivate();
    this->beenBeeped = FALSE;

    return TRUE;
}

boolean MsgWin::log(const char *file)
{
    this->flushBuffer();
    if (file == NULL && this->logFile)
    {
	fclose(this->logFile);
	delete[] this->logFileName;
	this->logFile = NULL;
	this->logFileName = NULL;
	this->logOption->setState(FALSE);
    }
    else
    {
	if (this->logFile)
	{
	    fclose(this->logFile);
	    delete[] this->logFileName;
	}
	this->logFile = fopen(file, "w");
	if (this->logFile == NULL)
	{
	    ErrorMessage("Failed to open %s: %s", file, strerror(errno));
	    this->logFileName = NULL;
	    this->logOption->setState(FALSE);
	    return FALSE;
	}
	this->logFileName = DuplicateString(file);
	this->logOption->setState(TRUE);
    }

    return TRUE;
}

boolean MsgWin::save(const char *file)
{
    if (file)
    {
	FILE *f = fopen(file, "w");
	if (f == NULL)
	{
	    ErrorMessage("Failed to open %s: %s", file, strerror(errno));
	    return FALSE;
	}
	int itemCount;
	XmStringTable items;
	XtVaGetValues(this->list,
	    XmNitemCount, &itemCount,
	    XmNitems, &items,
	    NULL);

	for (int i = 0; i < itemCount; ++i)
	{
	    XmStringContext context;
	    if (!XmStringInitContext(&context, items[i]))
		continue;
	    XmStringCharSet tag;
	    XmStringDirection dir;
	    Boolean separator;
	    char *s;
	    while (XmStringGetNextSegment(context, &s, &tag, &dir, &separator))
	    {
		fputs(s, f);
		XtFree(s);
	    }
	    fputc('\n', f);
	}

	fclose(f);
    }

    return TRUE;
}

void MsgWin::postExecCommandDialog()
{
    if (!this->execCommandDialog)
	this->execCommandDialog = 
	    new ExecCommandDialog(this->getRootWidget());
    this->execCommandDialog->post();
}

void MsgWin::postLogDialog()
{
    if (!this->logDialog)
	this->logDialog = this->saveDialog =
	    new MWFileDialog(this->getRootWidget(), this);
    this->logDialog->postAs(TRUE);
}

void MsgWin::postSaveDialog()
{
    if (!this->logDialog)
	this->logDialog = this->saveDialog =
	    new MWFileDialog(this->getRootWidget(), this);
    this->logDialog->postAs(FALSE);
}

extern "C" void MsgWin_SelectCB(Widget w, XtPointer closure, XtPointer callData)
{
    MsgWin *mw = (MsgWin*)closure;
    XmListCallbackStruct *l = (XmListCallbackStruct*)callData;

    ASSERT(l->reason == XmCR_SINGLE_SELECT);


    //
    // An attempt to avoid nagging the user...   if doesErrorOpenVpe() ==
    // DXApplication::MayOpenVpe then selectLine() will check this extra
    // param to see if it should prompt the user.  I would like to prompt only
    // in certain circumstances to avoid nagging.
    //
    boolean prompt = TRUE;
#if 0
    if (l->event) {
	if ((l->event->type == ButtonPress) ||
	    (l->event->type == ButtonRelease) ||
	    (l->event->type == KeyPress) ||
	    (l->event->type == KeyRelease)) {
	    if (l->event->xany.window == XtWindow(w)) {
		prompt = FALSE;
	    }
	}
    } 
#endif
    mw->selectLine(l->item_position, prompt);
}


void MsgWin::selectLine(int pos, boolean promptUser)
{
    ListIterator li(this->selectableLines);
    SelectableLine *l;
    boolean questionPosted = FALSE;

    while( (l = (SelectableLine*)li.getNext()) )
    {
	if (l->position == pos)
	{
	    break;
	}
    }
    if (l == NULL)
	return;
    
    //  0:  ERROR:  2::/main:0/Compute:0: Bad parameter: expression must be a string
    char netName[100];
    char nodeName[100];
    int  inst;
    int  len;
    Network *net = NULL;

    //
    // For each macro (denoted by the /macroname:digit/ sequence), 
    // find it in the net's editor (if there is either a net or an instance),
    // and look up the net.
    const char *line = strchr(l->line, '/');
    char c;
    ASSERT(line);
    while (sscanf(line, "/%[^:]:%d%n%c", netName, &inst, &len, &c) == 3 &&
	c == '/')
    {
	if (net)
	{
	    // Handle "main_Image_3" style macro names.
	    char prefix[100];
	    if (sscanf(netName, "%[^_]_%[^_]_%*d", prefix, nodeName) == 2 &&
		    EqualString(prefix, net->getNameString()))
	    {
		strcpy(netName, nodeName);
		break;
	    }

	    questionPosted = this->openEditorIfNecessary(net,netName,inst, 
						promptUser);
	}
	if (EqualString(netName, theDXApplication->network->getNameString()))
	    net = theDXApplication->network;
	else
	{
	    net = NULL;
	    Network *macro;
	    li.setList(theDXApplication->macroList);
	    while( (macro = (Network*)li.getNext()) )
		if (EqualString(netName, macro->getNameString()))
		    net = macro;
	    if (net == NULL)
		break;
	}
	line += len;
    }
    //
    // Handle the module at the end, which may be an Image.
    // 
    if (sscanf(line, "/%[^:]:%d%n%c", netName, &inst, &len, &c) == 3 &&
	(c == ':' || c == '/'))
    {
	if (net)
	{
	    // Handle "main_Image_3" style macro names.
	    char prefix[100];
	    if (sscanf(netName, "%[^_]_%[^_]_%*d", prefix, nodeName) == 2 &&
		    EqualString(prefix, net->getNameString()))
		strcpy(netName, nodeName);

	    questionPosted |=
			this->openEditorIfNecessary(net,netName,inst,
						promptUser);

	}
    }

    //
    // Raise this window above the editors that were just raised
    //
    if (theDXApplication->doesErrorOpenMessage()) {
        if (!this->isManaged())
            this->manage();
            //
            // If we have put a question dialog, then it might be nice to 
	    // leave it on top instead of the MsgWin.
            //
        else if (!questionPosted)
            XRaiseWindow(XtDisplay(this->getRootWidget()),
               XtWindow(this->getRootWidget()));
    }

}

//
// Make a tiny class for passing the editor window info to some 
// static callbacks.  Martin added this (mid Oct. 1995) in order to 
// provide the ability to abort opening a vpe on error. That process can 
// be very slow for huge nets.  You cannot use 
// theQuestionDialogManager->userQuery() inside selectLine because selectLine()
// is called by notify() and you must not cause any sort of heavily modal 
// pause inside notify() because {un}greening is happening.
//
class EdInfo {
    public:
	char 		*nodeName;
	Network 	*net;
	int 		inst;
	EditorWindow 	*editor;

	EdInfo (const char *nodeName, Network *net, EditorWindow *e, int inst) {
	    this->nodeName= DuplicateString(nodeName);
	    this->net = net;
	    this->editor = e;
	    this->inst = inst;
	};
	~EdInfo () {
	    if (this->nodeName) delete[] this->nodeName;
	};
};
//
// If there is an editor opened on the net, or if there is a resource
// in DXApplication which says to open an editor on the net...
// If the resource says NEVER but the window is already opened, then
// ignore the resource.
// If the application indicates we should prompt the user and the promptUser
// flag is set, then prompt the user with a Yes/No dialog about opening
// the editor.  
//
// Return TRUE, if we posted a dialog to ask the user if we should post
// the window.
//
boolean MsgWin::openEditorIfNecessary(Network *net, 
				const char *nodeName, int inst, 
				boolean promptUser)
{
    EditorWindow *e = net->getEditor();
    EdInfo *edinfo = new EdInfo (nodeName, net, e, inst);
    int show_status = theDXApplication->doesErrorOpenVpe(net);
    boolean questionPosted = FALSE;

    if (show_status == DXApplication::MustOpenVpe) {
	MsgWin::ShowEditor((XtPointer)edinfo);
    } else if (show_status == DXApplication::DontOpenVpe) {
	if (e && e->isManaged()) 
	    MsgWin::ShowEditor((XtPointer)edinfo);
	else
	    MsgWin::NoShowEditor((XtPointer)edinfo);
    } else if (show_status == DXApplication::MayOpenVpe) {
	if (e && e->isManaged()) 
	    MsgWin::ShowEditor((XtPointer)edinfo);
	else if (promptUser) { 
	    //
	    // Must be FULL_APPLICATION_MODAL so that the user can't do 
	    // File/Open while the question dialog is on the screen.  
	    // Must also be a child of theDXApplication and not of 
	    // this becuase if you mwm dismiss the MsgWin then the 
	    // entire ui is hosed.
	    //
	    const char *macro = net->getNameString();
	    const char *file = net->getFileName();
	    char *confMsg = new char[32 + STRLEN(file) + STRLEN(macro)];
	    sprintf (confMsg, "Open editor on %s (%s)?", macro, file);
	    theQuestionDialogManager->modalPost(
		theDXApplication->getAnchor()->getRootWidget(), 
		confMsg, "Open Editor Confirmation",
		(void*)edinfo,
		MsgWin::ShowEditor, MsgWin::NoShowEditor, NULL,
		"Yes", "No", NULL,
		XmDIALOG_FULL_APPLICATION_MODAL
	    );
	    delete[] confMsg;
	    questionPosted = TRUE;
	} 
    } 

    return questionPosted;
}

void MsgWin::ShowEditor (XtPointer cdata)
{
    EdInfo *ei = (EdInfo*)cdata;

    if (ei->editor == NULL)
	ei->editor = theDXApplication->newNetworkEditor(ei->net);
    // 
    //  On the last tool, we open up containing editor for the user.
    // 
    if (ei->editor) {
	ei->editor->manage();
	ei->editor->deselectAllNodes();
	ei->editor->selectNode (ei->nodeName, ei->inst, TRUE);
    }
    delete ei;
}
void MsgWin::NoShowEditor (XtPointer cdata)
{
    EdInfo *ei = (EdInfo*)cdata;

    delete ei;
    Widget w = theQuestionDialogManager->getRootWidget();
    if ((w) && (XtIsManaged(w))) XtUnmanageChild (w);
}

//
// callers are: a menu bar button, and endExecution (which happens quite often
// including during File/New, File/Open).  For the menu bar button, we cycle thru
// error messages ad infinitum.  For internal use we want to visit each member of
// the list at most once.  So if activateSameSelection==FALSE, then don't reset
// pos=0 and do another loop over all selections.
//
void MsgWin::findNextError(boolean activateSameSelection)
{
    int *positions;
    int npos;
    int pos;
    int itemCount;
    int topItem;
    int visibleItems;

    XtVaGetValues(this->list,
	XmNitemCount, &itemCount,
	XmNtopItemPosition, &topItem,
	XmNvisibleItemCount, &visibleItems,
	NULL);

    if (XmListGetSelectedPos(this->list, &positions, &npos))
    {
	pos = positions[npos-1];
	XtFree((char*)positions);
    }
    else
    {
	pos = 0;
    }

    ListIterator li(this->selectableLines);
    SelectableLine *s;
    boolean found = FALSE;
    while( (s = (SelectableLine*)li.getNext()) )
    {
	if (s->position > pos)
	{
	    found = TRUE;
	    XmListSelectPos(this->list, s->position, True);
	    if (s->position < topItem || s->position >= topItem + visibleItems)
		XmListSetBottomPos(this->list, s->position);
	    break;
	}
    }
    if ((!found) && (activateSameSelection))
    {
	li.setPosition(1);
	pos = 0;
	while( (s = (SelectableLine*)li.getNext()) )
	{
	    if (s->position > pos)
	    {
		found = TRUE;
		XmListSelectPos(this->list, s->position, True);
		if (s->position < topItem ||
			s->position >= topItem + visibleItems)
		    XmListSetBottomPos(this->list, s->position);
		break;
	    }
	}
    }
}

void MsgWin::findPrevError()
{
    int *positions;
    int npos;
    int pos;
    int itemCount;
    int topItem;
    int visibleItems;
    XtVaGetValues(this->list,
	XmNitemCount, &itemCount,
	XmNtopItemPosition, &topItem,
	XmNvisibleItemCount, &visibleItems,
	NULL);
    if (XmListGetSelectedPos(this->list, &positions, &npos))
    {
	pos = positions[npos-1];
	XtFree((char*)positions);
    }
    else
    {
	pos = itemCount + 1;
    }

    int i = this->selectableLines.getSize();
    SelectableLine *s;
    boolean found = FALSE;
    for (; i > 0 && (s = (SelectableLine*)this->selectableLines.getElement(i));
	 --i)
    {
	if (s->position < pos)
	{
	    found = TRUE;
	    XmListSelectPos(this->list, s->position, True);
	    if (s->position < topItem || s->position >= topItem + visibleItems)
		XmListSetBottomPos(this->list, s->position);
	    break;
	}
    }
    if (!found)
    {
	i = this->selectableLines.getSize();
	pos = itemCount + 1;
	for (; i > 0 && 
		(s = (SelectableLine*)this->selectableLines.getElement(i));
	     --i)
	{
	    if (s->position < pos)
	    {
		found = TRUE;
		XmListSelectPos(this->list, s->position, True);
		if (s->position < topItem || 
			s->position >= topItem + visibleItems)
		    XmListSetBottomPos(this->list, s->position);
		break;
	    }
	}
    }
}



boolean MsgWin::toggleTracing()
{
boolean on_or_off = this->traceOption->getState();
const char *cmd = (on_or_off? "Trace(\"0\",1);\n" : "Trace(\"0\", 0);\n" );

    //
    // Issue the script command
    //
    DXPacketIF *p = theDXApplication->getPacketIF();
    if (p) {
	theDXApplication->getMessageWindow()->addInformation(cmd);
	p->send(DXPacketIF::FOREGROUND, cmd);
	theDXApplication->showInstanceNumbers(on_or_off);
	return TRUE;
    } else {
	theDXApplication->showInstanceNumbers(on_or_off);
	return FALSE;
    }
}

boolean MsgWin::memoryUse()
{
    //
    // Issue the script command
    //
    char *cmd = "Usage(\"memory\",0);\n"; 
    DXPacketIF *p = theDXApplication->getPacketIF();
    if (p) {
	theDXApplication->getMessageWindow()->addInformation(cmd);
	p->send(DXPacketIF::FOREGROUND, cmd);
	return TRUE;
    }

    return FALSE;
}
