/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"

#if defined(HAVE_IOSTREAM)
#  include <iostream>
#else 
#  if defined(HAVE_IOSTREAM_H)
#  include <iostream.h>
#  endif
#  if defined(HAVE_STREAM_H)
#  include <stream.h>
#  endif
#endif

#include <Xm/RowColumn.h>
#include <Xm/CascadeB.h>
#include <Xm/Separator.h>
#include "DXWindow.h"
#include "DXApplication.h"
//#include "QuitCommand.h"
#include "CommandInterface.h"
#include "CommandScope.h"
#include "Command.h"
#include "ButtonInterface.h"
#include "ToggleButtonInterface.h"
#include "Network.h"
#include "ProcessGroupManager.h"
#include "ControlPanelAccessDialog.h"
#include "ErrorDialogManager.h"
#include "NoUndoDXWindowCommand.h"
#include "anchor.bm"
#include "OpenFileCommand.h"
#include "ListIterator.h"
#include "CascadeMenu.h"

Symbol DXWindow::lastMsg = 0;
const void *DXWindow::lastMsgData = NULL;

#include "DXWDefaultResources.h"

DXWindow::DXWindow(const char*   name,
		   boolean isAnchor, boolean usesMenuBar): 
		IBMMainWindow(name, usesMenuBar)
{
    this->anchor       = isAnchor;
    this->startup      = FALSE;
    this->anchorPixmap = NUL(Pixmap);
    this->anchorButton = NUL(Widget);

    this->executeMenu    	= NUL(Widget);
    this->executeMenuPulldown   = NUL(Widget);
    this->executeOnceOption     = NUL(CommandInterface*);
    this->executeOnChangeOption = NUL(CommandInterface*);
    this->endExecutionOption    = NUL(CommandInterface*);
    this->sequencerOption       = NUL(CommandInterface*);

    this->connectionMenu 	= NUL(Widget);
    this->connectionMenuPulldown = NUL(Widget);
    this->startServerOption          = NUL(CommandInterface*);
    this->disconnectFromServerOption = NUL(CommandInterface*);
    this->resetServerOption          = NUL(CommandInterface*);
    this->processGroupAssignmentOption = NUL(CommandInterface*);

    this->panelAccessDialog = NULL;

    this->toggleWindowStartupCmd =  NULL;
    this->toggleWindowStartupOption = NULL;

    this->helpTutorialOption = NUL(CommandInterface*);

    ASSERT(!usesMenuBar || this->commandScope);
}

DXWindow::~DXWindow()
{
    //
    // Cause the window to be taken of the display immediately.
    // This is useful when reading in a new network when we don't
    // get to the X main loop until after deleting the current net
    // and reading in the next so that windows from the last network
    // can be left up until after the new network is read in.
    //
    if (this->getRootWidget() && XtIsRealized(this->getRootWidget()))
	XtUnmapWidget(this->getRootWidget());

    if (this->anchorPixmap && this->menuBar)
    {
	XFreePixmap(XtDisplay(this->menuBar), this->anchorPixmap);
    }

    //
    // Execute menu
    //
    if (this->executeOnceOption) delete this->executeOnceOption;
    if (this->executeOnChangeOption) delete this->executeOnChangeOption;
    if (this->endExecutionOption) delete this->endExecutionOption;
    if (this->sequencerOption) delete this->sequencerOption;
   
    //
    // Connection menu
    //
    if (this->startServerOption) delete this->startServerOption;
    if (this->disconnectFromServerOption) 
			delete this->disconnectFromServerOption;
    if (this->resetServerOption) delete this->resetServerOption;
    if (this->processGroupAssignmentOption) 
			delete this->processGroupAssignmentOption;

    if (this->panelAccessDialog) delete this->panelAccessDialog;

    if (this->helpTutorialOption) delete this->helpTutorialOption;

    if (this->toggleWindowStartupOption)
	delete this->toggleWindowStartupOption;

    if (this->toggleWindowStartupCmd)
        delete this->toggleWindowStartupCmd;

    //
    // file history menu
    //
    ListIterator iter(this->file_history_buttons);
    ButtonInterface* bi;
    while ((bi=(ButtonInterface*)iter.getNext())) delete bi;
    iter.setList(this->file_history_commands);
    Command* cmd;
    while ((cmd=(Command*)iter.getNext())) delete cmd;
}
void DXWindow::beginExecution()
{
    // Make sure the widget present
    if (this->executeMenu != NULL) {
        XtVaSetValues(this->executeMenu,
                  XmNforeground,
                  theDXApplication->getExecutionHighlightForeground(),
                  NULL);
    }
}
void DXWindow::standBy()
{
    // Make sure the widget present
    if (this->executeMenu != NULL) {
        XtVaSetValues(this->executeMenu,
                  XmNforeground,
                  theDXApplication->getBackgroundExecutionForeground(),
                  NULL);
    }
}
void DXWindow::endExecution()
{
    // Make sure the widget present
    if (this->executeMenu != NULL) {
        XtVaSetValues(this->executeMenu,
                  XmNforeground,
                  theDXApplication->getForeground(),
                  NULL);
    }
}
void DXWindow::serverDisconnected()
{
    this->endExecution();
}


//
// Install the default resources for this class.
//
void DXWindow::installDefaultResources(Widget baseWidget)
{
    this->setDefaultResources(baseWidget, DXWindow::DefaultResources);
    this->IBMMainWindow::installDefaultResources(baseWidget);
}


void DXWindow::createMenuBar(Widget parent)
{
    this->IBMMainWindow::createMenuBar(parent);
    this->setAnchor(this->anchor);
}

//
// Create the anchor button widget and add the pixmap if desired and not
// already present
//
void DXWindow::createAnchor(boolean addPixmap)
{
    ASSERT(this->menuBar);

    if (!this->anchorButton) {
	this->anchorButton = XtVaCreateWidget
				("anchorButton",
				 xmCascadeButtonWidgetClass,
				 this->menuBar,
				 XmNlabelType,	XmPIXMAP,
				 NULL);
	XtUninstallTranslations (this->anchorButton);
    }
    //
    // If an anchor pixmap has not yet been created, do it now.
    //  (This code should be executed only once.)
    //
    if (addPixmap && !this->anchorPixmap)
    {
	Pixel foreground;
	Pixel background;
	int   depth;
        //
        // Create the anchor button.
        //
        XtVaGetValues (this->menuBar,
             XmNdepth,      &depth,
             NULL);

	//
	// It's bad to use a color name here. 
	// I made the pixmap placement happen earlier to avoid the
	// flashing which resulted when doing it at a convenient time.
	// If you fetch colors from this->menuBar now, then on an hp700 you
	// wind up with workspace default colors because that's what hpvue wants.
	//
#     if defined(hp700) && 0
	XrmValue from, toinout;
	from.addr = "#b4b4b4b4b4b4";
	from.size = 1+strlen(from.addr);
	toinout.addr = (XPointer)&background;
	toinout.size = sizeof(Pixel);
	XtConvertAndStore(this->menuBar, XmRString, &from, XmRPixel, &toinout);
#     else
	XtVaGetValues (this->getRootWidget(), XmNbackground, &background, NULL);
#     endif
	foreground = theDXApplication->getForeground();

	Window wind;
	Screen *scr = XtScreen(this->menuBar);
	wind = RootWindowOfScreen(scr);
        this->anchorPixmap =
            XCreatePixmapFromBitmapData
                (XtDisplay(this->menuBar),
                 wind,
                 anchor_bits,
                 anchor_width,
                 anchor_height,
                 foreground,
                 background,
                 depth);

        XtVaSetValues
            (this->anchorButton,
             XmNlabelType,              XmPIXMAP,
             XmNlabelPixmap,            this->anchorPixmap,
             XmNlabelInsensitivePixmap, this->anchorPixmap,
             NULL);

    }

}

void DXWindow::closeWindow()
{
    if (theDXApplication->appAllowsExitOptions()) {
	if (this->anchor) 
	    theDXApplication->exitCmd->execute();
	else
	    this->unmanage();
    } else {
	this->iconify();
    }
}

void DXWindow::manage()
{

    this->IBMMainWindow::manage();

    //
    // Get notified with the last message.
    //
    this->notify(DXWindow::lastMsg,DXWindow::lastMsgData);

    this->setAnchor(this->anchor);
}

void DXWindow::notify(const Symbol message, const void *data, const char *msg)
{
    if(NOT message)   // no previouse message.
	return;

    if (message == DXApplication::MsgExecute)
    {
    	DXWindow::lastMsgData = data;
    	DXWindow::lastMsg = message;
	this->beginExecution();
    }
    else if (message == DXApplication::MsgStandBy)
    {
    	DXWindow::lastMsgData = data;
    	DXWindow::lastMsg = message;
	this->standBy();
    }
    else if (message == DXApplication::MsgExecuteDone)
    {
    	DXWindow::lastMsgData = data;
    	DXWindow::lastMsg = message;
	this->endExecution();
    }
    else if (message == DXApplication::MsgServerDisconnected) 
    {
  	this->serverDisconnected();
    }
    else if (message == Command::MsgBeginExecuting)
    {
	this->beginCommandExecuting();
    }
    else if (message == Command::MsgEndExecuting)
    {
	this->endCommandExecuting();
    }
    else if (message == DXApplication::MsgPanelChanged)
    {
	if (this->panelAccessDialog && this->panelAccessDialog->isManaged()) {
	    this->panelAccessDialog->update();
	    if (this->title) {
		char *s = new char[STRLEN(this->title) + 32];
		sprintf(s,"Control Panel Access:%s",this->title);
		this->panelAccessDialog->setDialogTitle(s);
		delete s;
	    } else
                this->panelAccessDialog->setDialogTitle(
					"Control Panel Access...");
  	}
    }
    else
	this->IBMMainWindow::notify(message,data,msg);
}

//
// Virtual function called at the beginning of Command::execute.
//
void DXWindow::beginCommandExecuting()
{
    return;
}
//
// Virtual function called at the end of Command::execute.
//
void DXWindow::endCommandExecuting()
{
    return;
}

void DXWindow::createExecuteMenu(Widget parent)
{
    ASSERT(parent);
    Widget            pulldown;

    if (!theDXApplication->appAllowsExecuteMenus())
	return;

    //
    // Create "Execute" menu and options.
    //
    pulldown =
	this->executeMenuPulldown =
	    XmCreatePulldownMenu
		(parent, "executeMenuPulldown", NUL(ArgList), 0);

    this->executeMenu =
	XtVaCreateManagedWidget
	    ("executeMenu",
	     xmCascadeButtonWidgetClass,
	     parent,
	     XmNsubMenuId, pulldown,
	     NULL);

    this->executeOnceOption =
	new ButtonInterface(pulldown,
			    "executeOnceOption",
			    theDXApplication->executeOnceCmd);

    this->executeOnChangeOption =
	new ButtonInterface(pulldown,
			    "executeOnChangeOption",
			    theDXApplication->executeOnChangeCmd);

    this->endExecutionOption =
	new ButtonInterface(pulldown,
			    "endExecutionOption",
			    theDXApplication->endExecutionCmd);

    XtVaCreateManagedWidget
	    ("optionSeparator", xmSeparatorWidgetClass, pulldown, NULL);

    this->sequencerOption =
	new ButtonInterface(pulldown, 
			    "sequencerOption", 
			    theDXApplication->openSequencerCmd);
#if USE_REMAP	// 6/14/93
    XtVaCreateManagedWidget
	    ("optionSeparator", xmSeparatorWidgetClass, pulldown, NULL);

    Network *network = theDXApplication->network;
    //this->sequencerOption =
	new ToggleButtonInterface(pulldown, 
			    "remapInteractorOutputsOption", 
			    theDXApplication->toggleRemapInteractorsCmd,
			    (network ? network->isRemapInteractorOutputMode() :
				DEFAULT_REMAP_INTERACTOR_MODE));
#endif
}


void DXWindow::createHelpMenu(Widget parent)
{
    boolean addHelp = theDXApplication->appAllowsDXHelp();
    // this->createBaseHelpMenu(parent, addHelp, addHelp && this->isAnchor()); 
    // if (theDXApplication->appAllowsDXHelp() && this->isAnchor()) {
    this->createBaseHelpMenu(parent, addHelp, addHelp); 
    if (addHelp) {
	XtVaCreateManagedWidget("separator", xmSeparatorWidgetClass,
                                        this->helpMenuPulldown,
                                        NULL);
        this->helpTutorialOption =
            new ButtonInterface(this->helpMenuPulldown, "helpTutorialOption",
                theDXApplication->helpTutorialCmd);
    }

}


void DXWindow::createConnectionMenu(Widget parent)
{
    ASSERT(parent);
    ASSERT(this->menuBar);

    Widget            pulldown;

    if (!theDXApplication->appAllowsConnectionMenus())
	return;

    //
    // Create "Connection" menu and options.
    //
    pulldown =
        this->connectionMenuPulldown =
            XmCreatePulldownMenu
                (parent, "connectionMenuPulldown", NUL(ArgList), 0);
    this->connectionMenu =
        XtVaCreateManagedWidget
            ("connectionMenu",
             xmCascadeButtonWidgetClass,
             parent,
             XmNsubMenuId, pulldown,
             NULL);

    XtAddCallback(pulldown,
                  XmNmapCallback,
                  (XtCallbackProc)DXWindow_ConnectionMenuMapCB, 
                  (XtPointer)this);

    this->startServerOption =
        new ButtonInterface(pulldown, "startServerOption",
                            theDXApplication->connectToServerCmd);

    this->disconnectFromServerOption =
        new ButtonInterface(pulldown, "disconnectFromServerOption",
                            theDXApplication->disconnectFromServerCmd);

    this->resetServerOption =
        new ButtonInterface(pulldown, "resetServerOption",
                            theDXApplication->resetServerCmd);


    if (theDXApplication->appAllowsPGroupAssignmentChange()) {
	XtVaCreateManagedWidget("optionSeparator",
			    xmSeparatorWidgetClass, pulldown, NULL);

	this->processGroupAssignmentOption =
	    new ButtonInterface(pulldown, "processGroupAssignmentOption",
                            theDXApplication->assignProcessGroupCmd);

    }

}

extern "C" void DXWindow_ConnectionMenuMapCB(Widget w,
                                 XtPointer clientdata,
                                 XtPointer calldata)
{
#if WORKSPACE_PAGES
    GroupManager *gmgr = theDXApplication->getProcessGroupManager();
    if(gmgr->getGroupCount())
#else
    if(theDXApplication->PGManager->getGroupCount())
#endif
        theDXApplication->assignProcessGroupCmd->activate();
    else
        theDXApplication->assignProcessGroupCmd->deactivate();
}

//
// Post the panel access dialog with this window PanelAccessManager info.
//
void DXWindow::postPanelAccessDialog( PanelAccessManager *pam)
{

    if (!this->panelAccessDialog) {
	ASSERT(pam);
	this->panelAccessDialog = new ControlPanelAccessDialog(
			this->getRootWidget(),
			//theDXApplication->getRootWidget(),
			pam);	
    }

    this->panelAccessDialog->post();
    if (this->title) {
        char *s = new char[STRLEN(this->title) + 32];
	sprintf(s,"Control Panel Access:%s",this->title);
        this->panelAccessDialog->setDialogTitle(s);
        delete s;
    } else 
        this->panelAccessDialog->setDialogTitle("Control Panel Access...");
}
boolean DXWindow::printComment(FILE *f) 
{ 
    int xsize, ysize, xpos, ypos;

    if (!this->getGeometry(&xpos, &ypos, &xsize,&ysize))
	return TRUE;

    if (!UIComponent::PrintGeometryComment(f,xpos,ypos,xsize,ysize))
	return FALSE;

    return TRUE;
}
boolean DXWindow::parseComment(const char *line, const char *file,
				int lineno) 
{ 
    int items, xsize=0, ysize=0, xpos=0, ypos=0;
    float norm_xsize, norm_ysize, norm_xpos, norm_ypos;
    int display_xsize, display_ysize;

	
    if (!EqualSubstring(line," window: position =",19)) {
#if DX_MAJOR_VERSION == 3 && DX_MINOR_VERSION == 0 &&  DX_MICRO_VERSION == 0
        if (EqualSubstring(line," window: pos=(",14)) {
	    ErrorMessage("Bad comment found in file '%s' line %d.\n"
	      "This comment is only found in unreleased versions of DX and\n"
	      "so is not being supported.  You can fix your .cfg by deleting\n"
	      "the offending line. \n"
	      "(Customers should never get this message)", file,lineno);
		}
#endif
	return FALSE;
    }

#if INCLUDE_FLAGS_COMMENT	// Not used as of version 2.1
    int flags;
    items = sscanf(line," window: position = (%f,%f), size = %fx%f, "
		   "flags = 0x%x\n", 
		&norm_xpos,&norm_ypos,&norm_xsize,&norm_ysize, &flags);
			
    if (items == 5) {
		if ((norm_xsize < 3) && (norm_ysize < 3)) {
	    	display_xsize = DisplayWidth(theApplication->getDisplay(),0); 
	    	display_ysize = DisplayHeight(theApplication->getDisplay(),0); 
	    	xpos  = (int) (display_xsize * norm_xpos  + .5);
	    	ypos  = (int) (display_ysize * norm_ypos  + .5);
	    	xsize = (int) (display_xsize * norm_xsize + .5);
	    	ysize = (int) (display_ysize * norm_ysize + .5);
		}
#else

	if (UIComponent::ParseGeometryComment(line, file, lineno, &xpos, &ypos,
		&xsize, &ysize, NULL)) {
		
#endif
#if INCLUDE_FLAGS_COMMENT	// Not used as of version 2.1
	if (flags & 1)
	    this->setStartup(TRUE);
	else 
	    this->setStartup(FALSE);
#endif
    } else {
	ErrorMessage("Bad comment found in file '%s' line %d (ignoring)",
					file,lineno);
	return TRUE;
    }

    if (theDXApplication->applyWindowPlacements()) {
	this->setGeometry(xpos, ypos, xsize,ysize);
    }

    return TRUE;
}
//
// Reset the window to use the default settings for the state that is
// printed by the printComment() method. 
//
void DXWindow::useDefaultCommentState()
{
    this->setStartup(FALSE);
}


//
// Add a toggle button interface that toggles the startup up state of this
// window to the given parent widget.
//
Widget DXWindow::addStartupToggleOption(Widget parent)
{
    if (!this->toggleWindowStartupCmd)
	this->toggleWindowStartupCmd = 
		new NoUndoDXWindowCommand("toggleWindowStartup",
                                  this->commandScope, TRUE,
                                  this,
                                  NoUndoDXWindowCommand::ToggleWindowStartup);


    this->toggleWindowStartupOption =
	new ToggleButtonInterface
	    (parent, "toggleWindowStartupOption",
			this->toggleWindowStartupCmd, this->startup);

    return this->toggleWindowStartupOption->getRootWidget();

}
//
// Changes whether or not this window is supposed to open up automatically 
// on startup.
//
void DXWindow::toggleWindowStartup()
{
   this->setStartup(!this->startup);
}
//
// Change whether or not this window is an startup window.
//
void DXWindow::setStartup(boolean setting)
{
    this->startup = setting;
    
    if (this->toggleWindowStartupOption) 
    	this->toggleWindowStartupOption->setState(setting);
}

//
// Change whether or not this window is an anchor window.
//
void DXWindow::setAnchor(boolean isAnchor)
{
    this->anchor = isAnchor;
 
    if (isAnchor && this->hasMenuBar) {
	this->createAnchor(TRUE);
	XtManageChild(this->anchorButton);
    } else if (this->anchorButton) { 
	XtUnmanageChild(this->anchorButton);
    }
	
}


void DXWindow::getGeometryNameHierarchy(String names[], int* count, int max)
{
    int cnt = *count;
    if (cnt >= (max-1)) {
	this->IBMMainWindow::getGeometryNameHierarchy(names, count, max);
	return ;
    }

#ifdef DXD_NON_UNIX_DIR_SEPARATOR
    char fsep = '\\';
#else
    char fsep = '/';
#endif

    const char* fname = theDXApplication->network->getFileName();
    //
    // Remove the extension and all leading names, leaving 
    // only WindVorticity for examples
    //
    if ((!fname) || (!fname[0])) {
	this->IBMMainWindow::getGeometryNameHierarchy(names, count, max);
	return ;
    }

    int len = strlen(fname);
    int i;
    int last_slash = -1;
    for (i=len-1; i>=0; i--) {
	if ((fname[i] == fsep) || (fname[i] == '/')) {
	    last_slash = i;
	    break;
	}
    }
    last_slash++;
    char* file;
    file = DuplicateString(&fname[last_slash]);

    len = strlen(file);
    for (i=len-1; i>=0; i--) if (file[i]=='.') {file[i] = '\0'; break; }

    if (file[0]) {
	names[cnt++] = file;
	*count = cnt;
    }

    this->IBMMainWindow::getGeometryNameHierarchy(names, count, max);
}


void DXWindow::getGeometryAlternateNames(String* names, int* count, int max)
{
    int cnt = *count;
    if (cnt < (max-1)) {
	char* name = DuplicateString(this->name);
	names[cnt++] = name;
	*count = cnt;
    }
    this->IBMMainWindow::getGeometryAlternateNames(names, count, max);
}

void DXWindow::createFileHistoryMenu (Widget parent)
{
    if (!this->isAnchor()) return ;

    //
    // if there is no history, and we don't have the ability to
    // store history, then don't bother offering the menu.
    //
    char fname[256];
    if (!theIBMApplication->getApplicationDefaultsFileName(fname)) {
	List recent_nets;
	theDXApplication->getRecentNets(recent_nets);
	if (recent_nets.getSize()==0) {
	    return ;
	}
    }

    this->file_history_cascade = new CascadeMenu("fileHistory", parent);

    //
    // put the callback on the menu parent in which we create the cascade
    // because that allows us to grey out the cascade button before the
    // user has a chance to click on it.
    //
    XtAddCallback(parent, XmNmapCallback,
	(XtCallbackProc)DXWindow_FileHistoryMenuMapCB, (XtPointer)this);
}

void DXWindow::buildFileHistoryMenu()
{
    if (!this->file_history_cascade) return ;

    ListIterator iter(this->file_history_buttons);
    ButtonInterface* bi;
    while ((bi=(ButtonInterface*)iter.getNext())) {
	bi->unmanage();
	delete bi;
    }
    this->file_history_buttons.clear();

    iter.setList(this->file_history_commands);
    Command* cmd;
    while ((cmd=(Command*)iter.getNext())) delete cmd;
    this->file_history_commands.clear();

    Widget menu_parent = this->file_history_cascade->getMenuItemParent();

    List recent_nets;
    theDXApplication->getRecentNets(recent_nets);
    if (recent_nets.getSize()==0) {
	this->file_history_cascade->deactivate();

	//
	// make a 1 button menu if there is no content available.
	// I don't think this necessary, but it won't hurt.  No one
	// will see it.
	//
	const char* cp = "(null)";
	Symbol s = theSymbolManager->registerSymbol(cp);
	cmd = new OpenFileCommand(s);
	this->file_history_commands.appendElement(cmd);
	bi = new ButtonInterface(menu_parent, "openFile", cmd);
	bi->setLabel(cp);
	this->file_history_buttons.appendElement(bi);
	cmd->deactivate();
    } else {
	this->file_history_cascade->activate();
	//
	// Stick each button's label into this list, then before making
	// new buttons, ensure that the button's label is unique.  If
	// it isn't unique, then use the file's full path instead of
	// just the base name. This list actually serves 2 purposes.
	// It also records allocated memory so that we free it before
	// finishing.
	//
	List baseNames;
	iter.setList(recent_nets);
	Symbol s;
	while ((s=(Symbol)(long)iter.getNext())) {
	    cmd = new OpenFileCommand(s);
	    this->file_history_commands.appendElement(cmd);
	    bi = new ButtonInterface(menu_parent, "openFile", cmd);
	    this->file_history_buttons.appendElement(bi);

	    const char* fullpath = theSymbolManager->getSymbolString(s);
	    char* cp = GetFileBaseName(fullpath,0);
	    baseNames.appendElement(cp);
	    boolean unique = TRUE;
	    ListIterator biter(baseNames);
	    const char* cmprtr;
	    while ((cmprtr = (const char*)biter.getNext())) {
		if ((cmprtr!=cp) && (EqualString(cmprtr, cp))) {
		    unique = FALSE;
		    break;
		}
	    }
	    if (!unique) {
		cp = DuplicateString(fullpath);
		baseNames.appendElement(cp);
	    }
	    bi->setLabel(cp);
	}
	iter.setList(baseNames);
	char* cp;
	while ((cp=(char*)iter.getNext())) delete cp;
    }
}

extern "C" void DXWindow_FileHistoryMenuMapCB(Widget , XtPointer clientdata, XtPointer )
{
    DXWindow* dxw = (DXWindow*)clientdata;
    dxw->buildFileHistoryMenu();
}

