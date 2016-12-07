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
#include "DXAnchorWindow.h"
#include "ButtonInterface.h"
#include "CloseWindowCommand.h"
#include "Network.h"
#include "CascadeMenu.h"
#include "NoUndoAnchorCommand.h"
#include "DXStrings.h"
#include <Xm/Label.h>
#include <Xm/RowColumn.h>
#include <Xm/CascadeB.h>
#include <Xm/PushB.h>
#include <Xm/Separator.h>
#include <Xm/Form.h>
#include <X11/keysym.h>

boolean DXAnchorWindow::ClassInitialized = FALSE;

#define MENU_BAR_HEIGHT 32

String DXAnchorWindow::DefaultResources[] = {

    "*frame.shadowType:     		XmSHADOW_OUT",
    "*frame.marginHeight:   		2",
    "*frame.marginWidth:    		2",
    "*frame.shadowThickness:   	     	1",

    "*copyright.topOffset: 		2",
    "*copyright.leftOffset: 		2",
    "*copyright.rightOffset: 		2",
    "*copyright.bottomOffset: 		2",
    "*copyright.topAttachment: 		XmATTACH_FORM",
    "*copyright.leftAttachment: 	XmATTACH_FORM",
    "*copyright.rightAttachment: 	XmATTACH_FORM",
    "*copyright.bottomAttachment: 	XmATTACH_FORM",

    "*logoMessage.topOffset:		2",
    "*logoMessage.leftOffset:		2",
    "*logoMessage.rightOffset:		2",
    "*logoMessage.bottomOffset:		2",
    "*logoMessage.leftAttachment: 	XmATTACH_FORM",
    "*logoMessage.rightAttachment: 	XmATTACH_FORM",
    "*logoMessage.bottomAttachment: 	XmATTACH_FORM",

    "*logoButton.topOffset:		2",
    "*logoButton.bottomOffset:		2",
    "*logoButton.labelType:	 	XmPIXMAP",
    "*logoButton.showAsDefault: 	True",

    ".iconName:				DX",
    "*allowResize:			False",

    "*fileMenu.labelString:		File",
    "*fileMenu.mnemonic:		F",
    "*fileOpenOption.labelString:	Open...",
    "*fileOpenOption.mnemonic:		O",
    "*fileMenuPulldown.tearOffModel:	XmTEAR_OFF_DISABLED",

    "*fileSaveOption.labelString:       Save Program",
    "*fileSaveOption.mnemonic:          S",
    "*fileSaveOption.accelerator:       Ctrl<Key>S",
    "*fileSaveOption.acceleratorText:   Ctrl+S",
    "*fileSaveAsOption.labelString:     Save Program As...",
    "*fileSaveAsOption.mnemonic:        a",

    "*windowsMenu.labelString:		Windows",
    "*windowsMenu.mnemonic:		W",
    "*windowsMenuPulldown.tearOffModel:		    XmTEAR_OFF_DISABLED",
    "*openVPEOption.labelString:    		    Open Visual Program Editor",
    "*openVPEOption.mnemonic:       		    V",
    "*openAllControlPanelsOption.labelString:       Open All Control Panels",
    "*openAllControlPanelsOption.mnemonic:          A",
    "*openAllControlPanelsOption.accelerator:       Ctrl<Key>P",
    "*openAllControlPanelsOption.acceleratorText:   Ctrl+P",
    "*openControlPanelByNameOption.labelString:     Open Control Panel by Name",
    "*openControlPanelByNameOption.mnemonic:        N",
    "*openAllColormapEditorsOption.labelString:     Open All Colormap Editors",
    "*openAllColormapEditorsOption.mnemonic:        E",
    "*openAllColormapEditorsOption.accelerator:     Ctrl<Key>E",
    "*openAllColormapEditorsOption.acceleratorText: Ctrl+E",
    "*messageWindowOption.labelString:              Open Message Window",
    "*messageWindowOption.mnemonic:                 M",

    // FIXME:  It might be considered wrong to put this here because 
    // MainWindow subclasses may/may not be anchor windows and therefore may/may not
    // use the word "Quit" instead of "Close" which of course affects the choice of
    // accelerator.  The problem is that some people install the accelerator in
    // a MapCB which is way wrong.  Reason: accelerators operate without/before a
    // MapCB would be called.
    "*fileCloseOption.accelerator:	Ctrl<Key>Q",
    "*fileCloseOption.acceleratorText:  Ctrl+Q",


    "*fileLoadMacroOption.labelString:	Load Macro...",
    "*fileLoadMacroOption.mnemonic:	L",
    "*fileLoadMDFOption.labelString:	Load Module Description(s)...",
    "*fileLoadMDFOption.mnemonic:	M",
    "*settingsCascade.labelString:	Program Settings",
    "*settingsCascade.mnemonic:		r",
    "*openCfgOption.labelString:	Load...",
    "*openCfgOption.mnemonic:		L",	
    "*saveCfgOption.labelString:	Save As...",
    "*saveCfgOption.mnemonic:		S",

    // FIXME: this should be in DXWindow.C
    "*dxAnchorOnVisualProgramOption.labelString:	Application Comment...",
    "*dxAnchorOnVisualProgramOption.mnemonic:		A",

    NULL
};

DXAnchorWindow::DXAnchorWindow(const char *name, boolean isAnchor, boolean usesMenuBar):
	DXWindow (name, isAnchor, usesMenuBar)
{
    //
    // Initialize member data.
    //
    this->fileMenu    = NUL(Widget);
    this->fileMenuPulldown    = NUL(Widget);

    this->openOption         = NUL(CommandInterface*);
    this->saveOption         = NUL(CommandInterface*);
    this->saveAsOption       = NUL(CommandInterface*);
    this->loadMacroOption    = NUL(CommandInterface*);
    this->loadMDFOption      = NUL(CommandInterface*);
    this->closeOption        = NUL(CommandInterface*);

    this->settingsCascade    = NUL(CascadeMenu*);
    this->saveCfgOption      = NUL(CommandInterface*);
    this->openCfgOption      = NUL(CommandInterface*);

    this->closeCmd =
	new CloseWindowCommand("close",this->commandScope,TRUE,this);


    if (theDXApplication->appAllowsEditorAccess()) {
	this->openVPECmd = new NoUndoAnchorCommand ("openVPE",
		this->commandScope, TRUE, this, NoUndoAnchorCommand::OpenVPE);
    } else {
	this->openVPECmd = NUL(Command*);
    }

    this->openVPEOption                = NUL(CommandInterface*);
    this->openAllControlPanelsOption   = NUL(CommandInterface*);
    this->openAllColormapEditorsOption = NUL(CommandInterface*);
    this->messageWindowOption          = NUL(CommandInterface*);
    this->onVisualProgramOption = NUL(CommandInterface*);


    // Widgets
    this->form = NUL(Widget);
    this->label = NUL(Widget);
    this->logoButton = NUL(Widget);
    this->logoMessage = NUL(Widget);

    this->timeoutId = 0;

    //
    // Install the default resources for THIS class (not the derived classes)
    //
    if (NOT DXAnchorWindow::ClassInitialized)
    {
	ASSERT(theApplication);
        DXAnchorWindow::ClassInitialized = TRUE;
	this->installDefaultResources(theApplication->getRootWidget());
    }
}

DXAnchorWindow::~DXAnchorWindow()
{
    if (this->openOption) delete this->openOption;
    if (this->saveOption) delete this->saveOption;
    if (this->saveAsOption) delete this->saveAsOption;
    if (this->closeOption) delete this->closeOption;
    if (this->loadMacroOption) delete this->loadMacroOption;
    if (this->loadMDFOption) delete this->loadMDFOption;

    if (this->settingsCascade) delete this->settingsCascade;
    if (this->saveCfgOption) delete this->saveCfgOption;
    if (this->openCfgOption) delete this->openCfgOption;

    if (this->openVPECmd) delete this->openVPECmd;

    if (this->openVPEOption) delete this->openVPEOption;
    if (this->openAllControlPanelsOption) delete this->openAllControlPanelsOption;
    if (this->openAllColormapEditorsOption) delete this->openAllColormapEditorsOption;
    if (this->messageWindowOption) delete this->messageWindowOption;

    if (this->onVisualProgramOption) delete this->onVisualProgramOption;

    delete this->closeCmd;
    if (this->timeoutId) XtRemoveTimeOut(this->timeoutId);
}


//
// Install the default resources for this class.
//
void DXAnchorWindow::installDefaultResources(Widget baseWidget)
{
    this->setDefaultResources(baseWidget, DXAnchorWindow::DefaultResources);
    this->DXWindow::installDefaultResources(baseWidget);
}
//
// Create the work area for this window.  
//
Widget DXAnchorWindow::createWorkArea (Widget parent)
{
int n;
Arg args[20];
XmString xmstr;

    n = 0;
    this->form = XmCreateForm (parent, "frame", args, n);
    XtManageChild (this->form);

    //const char *cp = theDXApplication->getCopyrightNotice();
    const char *cp = "";
    char buf[256];
    strcpy (buf, cp);
    xmstr = XmStringCreateLtoR (buf, "bold");

    n = 0;
    XtSetArg (args[n], XmNlabelString, xmstr); n++;
    this->label = XmCreateLabel (this->form, "copyright", args, n);
    XtManageChild (this->label);

    XmStringFree(xmstr);

    this->resetWindowTitle();

    return form;
}

void
DXAnchorWindow::createMenus (Widget parent)
{
    this->createFileMenu(parent);
    this->createExecuteMenu(parent);
    this->createConnectionMenu(parent);
    this->createWindowsMenu(parent);
    this->createHelpMenu(parent);
    //
    // Right justify the help menu (if it exists).
    //
    if (this->helpMenu)
    {
	XtVaSetValues(parent, XmNmenuHelpWidget, this->helpMenu, NULL);
    }

}


void DXAnchorWindow::createFileHistoryMenu(Widget parent)
{
    if (theDXApplication->appAllowsImageRWNetFile()) 
	this->DXWindow::createFileHistoryMenu(parent);
}

void 
DXAnchorWindow::createFileMenu(Widget parent)
{
    ASSERT(parent);
    int buttons = 0;
    Widget pulldown;

    //
    // Create "File" menu and options.
    //
    pulldown =
	this->fileMenuPulldown =
	    XmCreatePulldownMenu(parent, "fileMenuPulldown", NUL(ArgList), 0);
    this->fileMenu =
	XtVaCreateManagedWidget
	    ("fileMenu",
	     xmCascadeButtonWidgetClass,
	     parent,
	     XmNsubMenuId, pulldown,
	     NULL);

    //
    // Net file options. 
    //
    
    if (theDXApplication->appAllowsImageRWNetFile())  {
	this->openOption =
	   new ButtonInterface(pulldown, "fileOpenOption", 
			theDXApplication->openFileCmd);

	this->createFileHistoryMenu(pulldown);

	Network* net = theDXApplication->network;
	this->saveOption = 
	   new ButtonInterface(pulldown, "fileSaveOption", 
			net->getSaveCommand());
	this->saveAsOption = 
	   new ButtonInterface(pulldown, "fileSaveAsOption", 
			net->getSaveAsCommand());
	buttons = 1;
    }

    if (theDXApplication->appAllowsRWConfig()) {
	Command *openCfgCmd = theDXApplication->network->getOpenCfgCommand();
	Command *saveCfgCmd = theDXApplication->network->getSaveCfgCommand();
	if (openCfgCmd && saveCfgCmd) {
	    this->settingsCascade = new CascadeMenu("settingsCascade",pulldown);
	    Widget menu_parent = this->settingsCascade->getMenuItemParent();
	    this->saveCfgOption = new ButtonInterface(menu_parent, 
					"saveCfgOption", saveCfgCmd);
	    this->openCfgOption = new ButtonInterface(menu_parent, 
					"openCfgOption", openCfgCmd);
	    buttons = 1;
	} else if (openCfgCmd) {
	    this->openCfgOption = new ButtonInterface(pulldown, 
					"openCfgOption", openCfgCmd);
	    buttons = 1;
	} else if (saveCfgCmd) {
	    this->saveCfgOption = 
		new ButtonInterface(pulldown, 
					"saveCfgOption", saveCfgCmd);
	    buttons = 1;
	}
    }
 
    
    //
    // Macro/mdf options.
    //
    if (buttons) {
	XtVaCreateManagedWidget
		("optionSeparator", xmSeparatorWidgetClass, pulldown, NULL);
	buttons = 0;
    }
    if (theDXApplication->appAllowsImageLoad()) {
	this->loadMacroOption =
	    new ButtonInterface(pulldown, "fileLoadMacroOption", 
			theDXApplication->loadMacroCmd);
	this->loadMDFOption =
	    new ButtonInterface(pulldown, "fileLoadMDFOption", 	
			theDXApplication->loadMDFCmd);
	buttons = 1;
    }


    //
    // Close/quite
    //
    if (buttons) 
	XtVaCreateManagedWidget
		("optionSeparator", xmSeparatorWidgetClass, pulldown, NULL);

    this->closeOption = new ButtonInterface(pulldown,"fileCloseOption",this->closeCmd);

    XtAddCallback(pulldown,
		  XmNmapCallback,
		  (XtCallbackProc)DXAnchorWindow_FileMenuMapCB,
		  (XtPointer)this);
}


//
// FIXME: You can't do this sort of stuff here.  This problem is not unique to
// this class either.  Look at ImageWindow.  You can't set accelerator in a MapCB
// because the accelerator is supposed to work before (or without ever) mapping
// the menu pane.
//
extern "C" void DXAnchorWindow_FileMenuMapCB(Widget ,
				   XtPointer clientdata,
				   XtPointer )
{
    DXAnchorWindow *filew = (DXAnchorWindow*)clientdata;
    XmString accText;
    KeySym mnem;

    if (theDXApplication->appAllowsExitOptions() && filew->isAnchor()) 
    {
	ASSERT(filew->closeOption);
	if((theDXApplication->network)&&(theDXApplication->network->saveToFileRequired()))
	    ((ButtonInterface*)filew->closeOption)->setLabel("Quit...");
	else
	    ((ButtonInterface*)filew->closeOption)->setLabel("Quit");
	accText = XmStringCreateSimple("Ctrl+Q");
	mnem = XK_Q;
	XtVaSetValues(filew->closeOption->getRootWidget(),
		    XmNmnemonic, mnem,
		    XmNaccelerator, "Ctrl<Key>Q",
		    XmNacceleratorText,accText,
		    NULL);
	XmStringFree(accText);
    }
    else 
    {
	((ButtonInterface*)filew->closeOption)->setLabel("Close");
	mnem = XK_C;
	XtVaSetValues(filew->closeOption->getRootWidget(),
		    XmNmnemonic, mnem,
		    NULL);
    }

    if (!filew->isAnchor()) 
    {
	if (filew->openOption)
	    filew->openOption->deactivate();
	if (filew->loadMacroOption)
	    filew->loadMacroOption->deactivate();
	if (filew->loadMDFOption)
	    filew->loadMDFOption->deactivate();
	if (filew->openCfgOption)
	    filew->openCfgOption->deactivate();
	if (filew->saveCfgOption)
	    filew->saveCfgOption->deactivate();
    }

}

void DXAnchorWindow::createWindowsMenu(Widget parent)
{
    ASSERT(parent);
 
    Widget	    pulldown;
 
    //
    // Create "Windows" menu and options.
    //
    pulldown =
	this->windowsMenuPulldown =
	    XmCreatePulldownMenu
		(parent, "windowsMenuPulldown", NUL(ArgList), 0);
    this->windowsMenu =
	XtVaCreateManagedWidget
	    ("windowsMenu",
	     xmCascadeButtonWidgetClass,
	     parent,
	     XmNsubMenuId, pulldown,
	     NULL);
 
    if (theDXApplication->appAllowsEditorAccess())
	this->openVPEOption =
	    new ButtonInterface
		(pulldown, "openVPEOption", this->openVPECmd);
 
    if (theDXApplication->appAllowsPanelAccess()) {
	if (theDXApplication->appAllowsOpenAllPanels())
	    this->openAllControlPanelsOption =
		new ButtonInterface
		    (pulldown,
		     "openAllControlPanelsOption",
		     theDXApplication->network->getOpenAllPanelsCommand());
  
	this->openControlPanelByNameMenu =
		new CascadeMenu("openControlPanelByNameOption",pulldown);
 
	// Builds the list of control panels
	XtAddCallback(pulldown,
		      XmNmapCallback,
		      (XtCallbackProc)DXAnchorWindow_WindowsMenuMapCB,
		      (XtPointer)this);
    }
 
    this->openAllColormapEditorsOption =
	new ButtonInterface
		(pulldown, "openAllColormapEditorsOption",
			theDXApplication->openAllColormapCmd);
 
    this->messageWindowOption =
	new ButtonInterface
	    (pulldown, "messageWindowOption",
	     theDXApplication->messageWindowCmd);
 
 
}
 

boolean DXAnchorWindow::postVPE()
{
Network *net = theDXApplication->network;

    if(NOT net)
	return FALSE;
 
    DXWindow* editor = (DXWindow*)net->getEditor();
 
    if (NOT editor)
	editor = (DXWindow*)theDXApplication->newNetworkEditor(net);

    if (editor) {
	editor->manage();
	XMapRaised(XtDisplay(editor->getRootWidget()),
	    XtWindow(editor->getRootWidget()));
    }
 
     return TRUE;
}
 

extern "C" void DXAnchorWindow_WindowsMenuMapCB(Widget ,
				 XtPointer clientdata,
				 XtPointer )
{
    PanelAccessManager *panelMgr;
    DXAnchorWindow *fw = (DXAnchorWindow*)clientdata;
 
    if (fw->openControlPanelByNameMenu) {
	Network *network = theDXApplication->network;
	CascadeMenu *menu = fw->openControlPanelByNameMenu;
	if (!network) {
	    menu->deactivate();
	} else {
	    panelMgr = network->getPanelAccessManager();
	    network->fillPanelCascade(menu, panelMgr);
	}
    }
}

void DXAnchorWindow::createHelpMenu(Widget parent)
{
    ASSERT(parent);

    this->DXWindow::createHelpMenu(parent);

    XtVaCreateManagedWidget("separator", xmSeparatorWidgetClass,
                                        this->helpMenuPulldown,
                                        NULL);

    this->onVisualProgramOption =
        new ButtonInterface(this->helpMenuPulldown, 
			"dxAnchorOnVisualProgramOption",
                        theDXApplication->network->getHelpOnNetworkCommand());
}

//
// A mix of {EditorWindow,ImageWindow::resetWindowTitles. 
// Differs from EditorWindow becuase it doesn't use the full path name. It just
// uses getFileName() which I believe will be whatever the user supplied and not
// what NFS thinks the file is.  The vpe uses the full nfs path.
// Differs from ImageWindow because it doesn't look for a node name or user supplied
// window title.
//
void DXAnchorWindow::resetWindowTitle()
{
    const char *vpe_name = theDXApplication->getInformalName();
    char *t = NULL;
    const char *title, *file = theDXApplication->network->getFileName();
 
    if (file) {
	t = new char[STRLEN(file) + STRLEN(vpe_name) +3];
	sprintf(t,"%s: %s", vpe_name, file);
	title = t;
    } else {
	title = vpe_name;
    }
    ASSERT(title);
    this->setWindowTitle(title);
    if (t) delete t;
}


//
// Handle the copyright notice responsibilities.  This could be called at any
// time but it makes sense to create widgets anyway because the logo is only
// available for a short time so this function is useless after startup.
//
void DXAnchorWindow::postCopyrightNotice()
{
Arg args[10];

    ASSERT (DXAnchorWindow::ClassInitialized);
    ASSERT (this->getRootWidget());
    const char *c = theDXApplication->getCopyrightNotice();
    if (!c) return;

    Pixmap logo_pmap = theDXApplication->getLogoPixmap(TRUE);

    Dimension diagWidth;
    XtVaGetValues (this->getRootWidget(),
	XmNwidth, &diagWidth,
    NULL);

    XtVaSetValues (this->label,
	XmNbottomAttachment, XmATTACH_NONE,
    NULL);

    unsigned int logow, logoh, bw;
    logow = logoh = bw = 0;
    if ((logo_pmap) && (logo_pmap != XtUnspecifiedPixmap)) {
	long junk;
	XGetGeometry (XtDisplay(this->label), logo_pmap, (Window*)&junk,
	    (int*)&junk, (int*)&junk, &logow, &logoh, &bw, (unsigned int*)&junk);
	logow+= 4; //button shadows and margins
	int half_logo_size = (logow+(bw<<1))>>1;
	half_logo_size = -half_logo_size;

	int n = 0;
	XtSetArg (args[n], XmNlabelPixmap, logo_pmap); n++;
	XtSetArg (args[n], XmNtopWidget, this->label); n++;
	XtSetArg (args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
	XtSetArg (args[n], XmNleftAttachment, XmATTACH_POSITION); n++;
	XtSetArg (args[n], XmNleftPosition, 50); n++;
	XtSetArg (args[n], XmNleftOffset, half_logo_size); n++; 
	this->logoButton = XmCreatePushButton (this->form, "logoButton", args, n);
	XtManageChild (this->logoButton);

	XtAddCallback (this->logoButton, XmNactivateCallback, 
	    (XtCallbackProc)DXAnchorWindow_LogoCB, (XtPointer)this);
    }

    char *tmpstr = new char[1+strlen(c)];
    strcpy (tmpstr, c);
    XmString xmstr = XmStringCreateLtoR (tmpstr, "bold");
    delete tmpstr;

    int n = 0;
    XtSetArg (args[n], XmNlabelString, xmstr); n++;
    if (this->logoButton)
	{ XtSetArg (args[n], XmNtopWidget, this->logoButton); n++; }
    else
	{ XtSetArg (args[n], XmNtopWidget, this->label); n++; }
    XtSetArg (args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
    this->logoMessage = XmCreateLabel (this->form, "logoMessage", args, n);
    XtManageChild (this->logoMessage);

    XmStringFree(xmstr);

    int topw, toph, botw, both, newh;
    DXAnchorWindow::LabelExtent (this->label, &topw, &toph);
    DXAnchorWindow::LabelExtent (this->logoMessage, &botw, &both);
    newh = 10 + toph + both + logoh + MENU_BAR_HEIGHT;
    botw+= 4; //fudge factor;

    if (botw > diagWidth) {
	XtVaSetValues (this->getRootWidget(),
	    XmNmaxHeight, newh,
	    XmNheight, newh,
	    XmNwidth, botw,
	NULL);
    } else {
	XtVaSetValues (this->getRootWidget(),
	    XmNmaxHeight, newh,
	    XmNheight, newh,
	NULL);
    }

    //
    // Set a timer to remove the message after 5000 milliseconds.
    //
    this->timeoutId = XtAppAddTimeOut (theDXApplication->getApplicationContext(), 5000,
	(XtTimerCallbackProc)DXAnchorWindow_TimeoutTO, (XtPointer)this);
}

extern "C" void
DXAnchorWindow_TimeoutTO (XtPointer cdata, XtIntervalId*)
{
    DXAnchorWindow *dxa = (DXAnchorWindow*)cdata;
    dxa->deleteCopyright();
    dxa->timeoutId = 0;
}

//
// Try getting rid of 1 piece at a time
//
// this->logoButton might legitimately be NUL if the logo pixmap wasn't available.
//
void DXAnchorWindow::deleteCopyright()
{
Boolean lButton, lMessage;

    if ((!this->logoMessage) && (!this->logoButton)) return ;

    if (this->logoButton)
	XtVaGetValues (this->logoButton, XmNmappedWhenManaged, &lButton, NULL);
    else
	lButton = False;

    XtVaGetValues (this->logoMessage, 
	XmNmappedWhenManaged, &lMessage,
    NULL);

    if (lButton) {
	XtVaSetValues (this->logoButton,
	    XmNmappedWhenManaged, False,
	NULL);
	this->timeoutId = 
	    XtAppAddTimeOut (theDXApplication->getApplicationContext(), 200,
		(XtTimerCallbackProc)DXAnchorWindow_TimeoutTO, (XtPointer)this);
    } else if (lMessage) {
	XtVaSetValues (this->logoMessage,
	    XmNmappedWhenManaged, False,
	NULL);
	this->timeoutId = 
	    XtAppAddTimeOut (theDXApplication->getApplicationContext(), 200,
		(XtTimerCallbackProc)DXAnchorWindow_TimeoutTO, (XtPointer)this);
    } else {

	// Look in IBMApplication.C for the list of available font names.
	this->setKioskMessage(theDXApplication->getInformalName(), "huge_bold");

	XtUnmanageChild (this->logoMessage);
	if (this->logoButton) {
	    theDXApplication->cleanupLogo();
	    XtDestroyWidget (this->logoButton);
	    this->logoButton = NUL(Widget);
	}

	XtVaSetValues (this->label,
	    XmNbottomAttachment, XmATTACH_FORM,
	NULL);

	XtDestroyWidget (this->logoMessage);
	this->logoMessage = NUL(Widget);
    }
}

void DXAnchorWindow::LabelExtent (Widget label, int *ww, int *wh)
{
XmFontList 	xmfl;
XmString 	xmstr;
Dimension 	labh, labw;
Dimension   	mw,mh,mt,mb,ml,mr,ht,st;
int		to,lo,ro,bo;

	mw = mh = mt = mb = ml = mr = ht = st = lo = to = bo = ro = 0;
	XtVaGetValues (label,
	    XmNfontList, &xmfl, XmNmarginWidth, &mw,
	    XmNmarginHeight, &mh, XmNmarginWidth, &mw, 
	    XmNmarginHeight, &mh, XmNmarginLeft, &ml, 
	    XmNmarginRight, &mr, XmNmarginTop, &mt,
	    XmNmarginBottom, &mb, XmNshadowThickness, &st, 
	    XmNhighlightThickness, &ht, XmNleftOffset, &lo, 
	    XmNrightOffset, &ro, XmNtopOffset, &to,
	    XmNbottomOffset, &bo, XmNlabelString, &xmstr,
	NULL);
	XmStringExtent (xmfl, xmstr, &labw, &labh);
	*ww = labw + lo + ro + (2*(mw + ht + st)) + ml + mr;
	*wh = labh + to + bo + (2*(mh + ht + st)) + mt + mb;
	XmStringFree (xmstr);
}

extern "C" void
DXAnchorWindow_LogoCB (Widget logoButton, XtPointer cdata, XtPointer)
{
DXAnchorWindow *dxa = (DXAnchorWindow*)cdata;

    XtRemoveCallback (logoButton, XmNactivateCallback,
	(XtCallbackProc)DXAnchorWindow_LogoCB, cdata);
    if (dxa->timeoutId) {
	XtRemoveTimeOut(dxa->timeoutId);
	dxa->timeoutId = 0;
    }
    dxa->deleteCopyright();
}


//
// Set a new text string and adjust the window size accordingly.  Setting
// msg==NUL means don't change the message just update the sizes.
//
void DXAnchorWindow::setKioskMessage (const char *msg, const char *font)
{
Dimension diagWidth;
int newh, neww;
Arg args[8];
int n = 0;

    XtVaGetValues (this->getRootWidget(),
	XmNwidth, &diagWidth,
    NULL);

    if (msg) {
	ASSERT(font);  //font[0] can be '\0' but font can't be NUL
	char *tmpmsg = new char[strlen(msg) + 1]; strcpy (tmpmsg, msg);
	char *tmpfont = new char[strlen(font) + 1]; strcpy (tmpfont, font);
	XmString xmstr = XmStringCreateLtoR (tmpmsg, tmpfont);
	XtVaSetValues (this->label, XmNlabelString, xmstr, NULL);
	XmStringFree(xmstr);
	delete tmpmsg;
	delete tmpfont;
    }

    DXAnchorWindow::LabelExtent (this->label, &neww, &newh);
    newh+= MENU_BAR_HEIGHT;

    //
    // Reduce the size of the window only if the difference is really big.
    // (Removed the "only if" on behalf of ezstart so that the kiosk window
    // would fit beside the startup window comortably.)
    //
    neww = MAX(neww, 410); // can't make menubar smaller than 400 or it wraps
    int diff = diagWidth - neww;
    if (diff > /* 150 */0) // diff can't be negative
    {XtSetArg (args[n],XmNwidth, neww); n++;}
     XtSetArg (args[n],XmNminHeight, MENU_BAR_HEIGHT); n++;
     XtSetArg (args[n],XmNmaxHeight, newh); n++;
     XtSetArg (args[n],XmNheight, newh); n++;
     XtSetArg (args[n],XmNminWidth, neww); n++;
    XtSetValues (this->getRootWidget(), args, n);
}
