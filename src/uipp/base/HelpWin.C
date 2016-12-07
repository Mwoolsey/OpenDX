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
#include "HelpWin.h"

#include "Application.h"
#include "../dxuilib/DXApplication.h"
#include "../dxuilib/MsgWin.h"
#include "ButtonInterface.h"
#include "NoUndoHelpCmd.h"
#include "DictionaryIterator.h"
#include "StartWebBrowser.h"

#include "help.h"

#include <stdio.h>

#include <Xm/PushB.h>
#include <Xm/CascadeB.h>
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/Label.h>
#include <Xm/MenuShell.h>
#include <Xm/PanedW.h>
#include <Xm/RowColumn.h>
#include <Xm/ScrolledW.h>
#include <Xm/Separator.h>
#include "../widgets/MultiText.h"

boolean HelpWin::ClassInitialized = FALSE;
boolean HelpWin::UseWebBrowser = FALSE;
String  HelpWin::DefaultResources[] = {
    ".title:				Help",
    ".iconName:				Help",
    ".width:				750",
    ".height:				750",
    "*fileMenu.labelString:		File",
    "*fileMenu.mnemonic:		F",
    "*helpCloseOption.labelString:	Close",
    "*helpCloseOption.width:		80",
    "*helpCloseOption.height:		25",

//    "*background:			#dddddddddddd",
//    "*topShadowColor:			#ffffffffffff",
//    "*bottomShadowColor:		#666666666666",


//    "*helpForm.width:			845",
//    "*helpForm.height:			636",

    "*Text1.fontList:			6x10",
    "*Text1.value:			demo.main",

    "*Text2.fontList:			6x10",
    "*Text2.value:			footnote",

    "*HistoryMenuLabel.labelString:	Path",

    "*goBackButton.labelString:		Go Back",
    "*goBackButton.width:		80",
    "*goBackButton.height:		25",

    "*scroll.borderWidth:		0",
//    "*scroll*vScrollBar.background:	#dddddddddddd",
//    "*scroll*vScrollBar.foreground:	#dddddddddddd",

    "*FootText.width:			600",

    "*PopupMenu*background:		#7e7eb4b4b4b4",
    "*HistoryMenuLabel*background:	#7e7eb4b4b4b4",

    "*scroll*scrollBarDisplayPolicy:	STATIC",
    "*scroll*scrollingPolicy:		AUTOMATIC",
    "*scroll*visualPolicy:		CONSTANT",

    "*helpText*scaleDPSpercent:		160",
    "*helpText*frameMakerFix:		false",
    "*helpText*waitCursorCount:		8",

    NULL
};

//
// Routines in the help C system.
extern "C" Widget HelpOn(Widget, int, char *, char *, int);

//
// Constructors:
//
HelpWin::HelpWin() : MainWindow("helpWindow", FALSE)
{

   char *webApp = getenv("DX_WEB_BROWSER");
   if(webApp) UseWebBrowser = TRUE;

   if(!UseWebBrowser) {
	this->init();

	//
    	// Install the default resources for THIS class (not the derived classes)
    	//
    	if (NOT HelpWin::ClassInitialized)
    	{
	    ASSERT(theApplication);
            HelpWin::ClassInitialized = TRUE;
	    this->installDefaultResources(theApplication->getRootWidget());
    	}
    }
}

HelpWin::HelpWin(const char *name, boolean hasMenuBar) :
		    MainWindow(name, hasMenuBar)
{
   this->init();
}

//
// Encapsulate initialization for multiple constructors
//
void HelpWin::init()
{
    this->fileMenu = NULL;
    this->historyPopup = NULL;
    this->closeCmd = new NoUndoHelpCmd("close", this->commandScope, TRUE, 
	this, NoUndoHelpCmd::Close);
}
//
// Destructor:
//
HelpWin::~HelpWin()
{
    DictionaryIterator di(this->topicToFileMap);
    char *file;
    while((file=(char*)di.getNextDefinition()))
	delete file;
    this->topicToFileMap.clear();

    delete this->closeOption;
    delete this->closeCmd;

    UserData *userdata;
    XtVaGetValues(this->multiText, XmNuserData, &userdata, NULL);
    XtFree((char*)userdata);
}

//
// Install the default resources for this class.
//
void HelpWin::installDefaultResources(Widget baseWidget)
{
    this->setDefaultResources(baseWidget, HelpWin::DefaultResources);
    this->MainWindow::installDefaultResources(baseWidget);
}

void HelpWin::initialize()
{
    FILE *helpDir;
    
    if(UseWebBrowser) {
	char helpDirFileName[512];
	strcpy(helpDirFileName, GetHTMLDirectory());
	strcat(helpDirFileName, "/");
	strcat(helpDirFileName, GetHTMLDirFileName());
	helpDir = fopen(helpDirFileName, "r");
     } else { /* !UseWebBrowser */
	this->MainWindow::initialize();

	char helpDirFileName[512];
	strcpy(helpDirFileName, GetHelpDirectory());
	strcat(helpDirFileName, "/");
	strcat(helpDirFileName, GetHelpDirFileName());
	helpDir = fopen(helpDirFileName, "r");
    }

    if (!helpDir)
	return;
    char line[1000];
    while(fgets(line, 1000, helpDir))
    {
	if (strchr(line, '!'))
	    *strchr(line, '!') = '\0';
	char topic[250];
	char file[250];
	if (sscanf(line, "%s %s", topic, file) == 2)
	{
	    char *fileCopy = DuplicateString(file);
	    this->topicToFileMap.addDefinition(topic, fileCopy);
	}
    }
    fclose(helpDir);
}

extern "C"  void LinkCB(Widget, XtPointer, XtPointer);
extern "C"  void GoBackCB(Widget, XtPointer, XtPointer);
extern "C"  SpotList *NewList();
extern "C"  void ResizeTheWindow(Widget, XtPointer, XEvent*, Boolean*);

Widget HelpWin::createWorkArea(Widget parent)
{
    Widget	frame;
    Widget      form;
    Widget      panedWindow;
    Widget      scrolledWindow;
    Widget      hbar;
    Widget      vbar;
    Widget      goBackButton;
    Widget      menuShell;
    UserData*   userdata;
    Dimension   width;
    Dimension   height;
    Dimension   thickness;
    int         argcnt;

    Arg         args[64];
    
    frame =
	XtVaCreateManagedWidget
	    ("helpFrame",
	     xmFrameWidgetClass,
	     parent,
	     XmNshadowType,   XmSHADOW_OUT,
	     XmNmarginHeight, 5,
	     XmNmarginWidth,  5,
	     NULL);

    /*
     * Create the top-level form widget.
     */
    form = XtVaCreateManagedWidget("helpForm", xmFormWidgetClass, frame, NULL);

    /*
     * Create the "Go Back" button.
     */
    goBackButton =
	XtVaCreateManagedWidget ("goBackButton", xmPushButtonWidgetClass, form,
	    XmNsensitive,        False,
	    XmNrecomputeSize,    False,
	    XmNbottomAttachment, XmATTACH_FORM,
	    XmNleftAttachment,   XmATTACH_FORM,
	    NULL);
    this->closeOption = 
	new ButtonInterface(form, "helpCloseOption", this->closeCmd);

    XtVaSetValues(this->closeOption->getRootWidget(),
	    XmNrecomputeSize,    False,
	    XmNbottomAttachment, XmATTACH_FORM,
	    XmNrightAttachment,  XmATTACH_FORM,
	    NULL);
    
    /*
     * Create a paned window.
     */
    panedWindow =
	XtVaCreateManagedWidget("panedWin",
	    xmPanedWindowWidgetClass,
	    form,
	    XmNtopAttachment,    XmATTACH_FORM,
	    XmNleftAttachment,   XmATTACH_FORM,
	    XmNrightAttachment,  XmATTACH_FORM,
	    XmNbottomAttachment, XmATTACH_WIDGET,
	    XmNbottomWidget,     goBackButton,
	    XmNbottomOffset,     5,
	    NULL);

    /*
     * Create a scrolled window.
     */
    argcnt = 0;
    XtSetArg(args[argcnt], XmNscrollingPolicy, XmAUTOMATIC); argcnt++;
    XtSetArg(args[argcnt], XmNallowResize,     True);        argcnt++;

    scrolledWindow =
	XtCreateManagedWidget
	    ("scrolledWin", 
	     xmScrolledWindowWidgetClass,
	     panedWindow,
	     args,
	     argcnt);

    /*
     * Get the horizotal/vertical scrollbar widget ID's.
     */
    XtSetArg(args[0], XmNhorizontalScrollBar, &hbar);
    XtSetArg(args[1], XmNverticalScrollBar,   &vbar);
    XtGetValues(scrolledWindow, args, 2);
    
    /*
     * Override the highlight thickness on the horizontal scrollbar.
     */
    if (hbar)
    {
	XtSetArg(args[0], XmNhighlightThickness, &thickness);
	XtSetArg(args[1], XmNheight,             &height);
	XtGetValues(hbar, args, 2);

	height -= thickness * 2;

	XtSetArg(args[0], XmNhighlightThickness, 0);
	XtSetArg(args[1], XmNheight,             height);
	XtSetValues(hbar, args, 2);
    }

    /*
     * Override the highlight thickness on the vertical scrollbar.
     */
    if (vbar)
    {
	XtSetArg(args[0], XmNhighlightThickness, &thickness);
	XtSetArg(args[1], XmNwidth,              &width);
	XtGetValues(vbar, args, 2);

	width -= thickness * 2;
	XtSetArg(args[0], XmNhighlightThickness, 0);
	XtSetArg(args[1], XmNwidth,              width);
	XtSetValues(vbar, args, 2);
    }

    /*
     * Create the MultiText widget.
     */
    XtVaGetValues(scrolledWindow, XmNwidth, &width, NULL);
    this->multiText =
	XtVaCreateManagedWidget(
	    "helpText", xmMultiTextWidgetClass, scrolledWindow,
	    XmNshowCursor, False,
	    XmNwidth, width,
	    NULL);

#ifdef OS2
    XtAddCallback(this->multiText, XmNlinkCallback, (XtCallbackProc)LinkCB, NULL);
#else
    XtAddCallback(this->multiText, XmNlinkCallback, LinkCB, NULL);
#endif

    /*
     * Register a callback to find out when the window is resized.
     */
#ifdef OS2
    XtAddEventHandler
	(panedWindow, StructureNotifyMask, False, (XtEventHandler)ResizeTheWindow, (XtPointer)this->multiText);
#else
    XtAddEventHandler
        (panedWindow, StructureNotifyMask, False, ResizeTheWindow, (XtPointer)this->multiText);
#endif

#ifdef OS2
    XtAddCallback
	(goBackButton, XmNactivateCallback, (XtCallbackProc)GoBackCB, (XtPointer)this->multiText);
#else
    XtAddCallback
	(goBackButton, XmNactivateCallback, GoBackCB, (XtPointer)this->multiText);
#endif

    /*
     * Create a popup menu (only if it's not Sun's X system, since pop-up's
     * tend to hang the session).
     */
    if (strstr(ServerVendor(XtDisplay(scrolledWindow)), "X11/NeWS"))
    {
	menuShell = NULL;
    }
    else
    {
	argcnt = 0;
	XtSetArg(args[argcnt], XmNwidth,  1); argcnt++;
	XtSetArg(args[argcnt], XmNheight, 1); argcnt++;
	menuShell = DXCreatePopupMenu(scrolledWindow);
    }

    /*
     * Save info in the client data.
     */
    userdata = (UserData*)XtMalloc(sizeof(UserData));
    userdata->filename[0] = '\0';
    userdata->label[0] = '\0';
    userdata->swin = scrolledWindow;
    userdata->hbar = hbar;
    userdata->vbar = vbar;
    userdata->toplevel = parent;
    userdata->goback = goBackButton;
    userdata->historylist = NewHistoryList();
    userdata->spotlist = NewList();
    userdata->fontstack = NULL;
    userdata->colorstack = NULL;
    userdata->linkType = NOOP;
    userdata->menushell = menuShell;
    userdata->getposition = FALSE;
    userdata->mapped = TRUE;

    /*
     * Save away the client data.
     */
    argcnt = 0;
    XtSetArg(args[argcnt], XmNuserData,userdata); argcnt++;

    XtSetValues(this->multiText, args, argcnt);

    return frame;
}

void HelpWin::loadTopicFile(const char *topic, const char *file)
{
    if(UseWebBrowser) {
    	//Start web browser with file
    	char url[520];
    	strcpy(url, "file://");
    	strcat(url, GetHTMLDirectory());
    	strcat(url, "/");
	if(strcmp(topic, file) == 0)
	    strcat(url, "notfound.htm");
	else
    	    strcat(url, file);
    	if(!_dxf_StartWebBrowserWithURL(url)) {
		// Couldn't start browser
	    // Can't run standard help without initializing it as so. 
             fprintf(stderr, "Error: Unable to launch web browser, defaulting back to standard help.\n");

		UseWebBrowser=FALSE;

		this->init();

		//
    	// Install the default resources for THIS class (not the derived classes)
    	//
    	if (NOT HelpWin::ClassInitialized)
    	{
	    ASSERT(theApplication);
            HelpWin::ClassInitialized = TRUE;
	    this->installDefaultResources(theApplication->getRootWidget());
    	}
    	topicToFileMap.clear();
    	helpOn(topic);
	}
    } else /* !UseWebBrowser */
	HelpOn(this->multiText, LINK, (char*)file, (char*)topic, 0);
}
void HelpWin::helpOn(const char *topic)
{
#ifdef DEBUG
    printf("HelpWin::helpOn(%s)\n", topic);
#endif
    if(UseWebBrowser) {
    	initialize();    	    
    } else { /* !UseWebBrowser */
        //
        // Manage must be done first in case initialize() hasn't been 
        // called yet (the first manage() results in an initialize() call).
        //
	this->manage();
    }

    char *file;
    if ((file = (char*)this->topicToFileMap.findDefinition(topic)) == 0)
	file = (char*)topic;

    this->loadTopicFile(topic,file);
}


extern "C" const char *GetHelpDirectory()
{
    return theApplication->getHelpDirectory();
}
extern "C" const char *GetHelpDirFileName()
{
    return theApplication->getHelpDirFileName();
}

extern "C" const char *GetHTMLDirectory()
{
    return theApplication->getHTMLDirectory();
}
extern "C" const char *GetHTMLDirFileName()
{
    return theApplication->getHTMLDirFileName();
}


