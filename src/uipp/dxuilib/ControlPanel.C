/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif
#include <Xm/CascadeB.h>
#include <Xm/DrawingA.h>
#include <Xm/Frame.h>
#include <Xm/Form.h>
#include <Xm/Label.h>
#include <Xm/List.h>
#include <Xm/RowColumn.h>
#include <Xm/ScrolledW.h>
#include <Xm/Separator.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h>

#include "ControlPanel.h"
#include "ControlPanelWorkSpace.h"
#include "DXApplication.h"
#include "Network.h"
#include "ButtonInterface.h"
#include "ToggleButtonInterface.h"
// #include "UndoCommand.h"
#include "CloseWindowCommand.h"
#include "ToolPanelCommand.h"
#include "NoUndoPanelCommand.h"
#include "ErrorDialogManager.h"
#include "WarningDialogManager.h"
#include "InteractorInstance.h"
#include "Interactor.h"
#include "EditorWindow.h"
#include "WorkSpace.h"
#include "../widgets/XmDX.h"
#include "../widgets/WorkspaceW.h"
#include "StandIn.h"
#include "SetPanelNameDialog.h"
#include "SetPanelCommentDialog.h"
#include "HelpOnPanelDialog.h"
#include "Dictionary.h"
#include "DictionaryIterator.h"
#include "GridDialog.h"
#include "SetInteractorNameDialog.h"
#include "PanelAccessManager.h"
#include "ControlPanelAccessDialog.h"
#include "Decorator.h"
#include "WorkSpaceComponent.h"
#include "CascadeMenu.h"
#include "DecoratorStyle.h"
#include "DecoratorInfo.h"
#include "LabelDecorator.h"
#include "QuestionDialogManager.h"

#if defined(HAVE_XINERAMA)
#include <X11/extensions/Xinerama.h>
#endif

#define ICON_NAME	"Control Panel"

#include "CPDefaultResources.h"

//
// Matting
//
#define SCRWF_MARGIN 5

boolean ControlPanel::ClassInitialized = FALSE;


ControlPanel::ControlPanel(Network* network) : 
			DXWindow("controlPanel", FALSE)
{
    ASSERT(network);

    //
    // Save the network and add self to the network panel list.
    //
    this->network = network;

    //
    // Initialize member data.
    //
    this->fileMenu    = NUL(Widget);
    this->editMenu    = NUL(Widget);
    this->panelsMenu  = NUL(Widget);
    this->optionsMenu = NUL(Widget);

    this->fileMenuPulldown    = NUL(Widget);
    this->editMenuPulldown    = NUL(Widget);
    this->panelsMenuPulldown  = NUL(Widget);
    this->optionsMenuPulldown = NUL(Widget);
	
    this->closeOption        = NUL(CommandInterface*);
    this->closeAllOption     = NUL(CommandInterface*);

    this->setAttributesOption           = NUL(CommandInterface*);
    this->setInteractorLabelOption      = NUL(CommandInterface*);
    this->verticalLayoutOption          = NUL(CommandInterface*);
    this->horizontalLayoutOption        = NUL(CommandInterface*);
    this->deleteOption                  = NUL(CommandInterface*);
    this->addSelectedInteractorsOption  = NUL(CommandInterface*);
    this->showSelectedInteractorsOption = NUL(CommandInterface*);
    this->showSelectedStandInsOption    = NUL(CommandInterface*);
    this->commentOption                 = NUL(CommandInterface*);

    this->openAllControlPanelsOption   = NUL(CommandInterface*);
    this->openControlPanelByNameMenu = NULL;

    this->changeControlPanelNameOption = NUL(CommandInterface*);
    this->gridOption                   = NUL(CommandInterface*);
    this->styleOption                  = NUL(CommandInterface*);
    this->setAccessOption 	       = NUL(CommandInterface*);

    this->onControlPanelOption  = NUL(CommandInterface*);
    this->onVisualProgramOption = NUL(CommandInterface*);

    this->userHelpOption = NUL(CommandInterface*);

    this->panelAccessManager = new PanelAccessManager(network, this);

    if (theDXApplication->appAllowsPanelAccess())
	this->closeCmd =
	    new CloseWindowCommand("close", this->commandScope, TRUE, this);
    else
	this->closeCmd = NULL;


    if (theDXApplication->appAllowsPanelEdit() &&
        theDXApplication->appAllowsEditorAccess()) {

	this->addSelectedInteractorsCmd =
	    new NoUndoPanelCommand("addSelectedInteractors", 
				  this->commandScope, FALSE, 
				  this, NoUndoPanelCommand::AddInteractors);

	this->showSelectedInteractorsCmd =
	    new NoUndoPanelCommand("showSelectedInteractors", 
				  this->commandScope, FALSE, 
				  this, NoUndoPanelCommand::ShowInteractors);

	this->showSelectedStandInsCmd =
	    new NoUndoPanelCommand("showSelectedStandIns", 
				  this->commandScope, FALSE, 
				  this, NoUndoPanelCommand::ShowStandIns);

	this->deleteSelectedInteractorsCmd =
	    new NoUndoPanelCommand("deleteInteractors", 
				  this->commandScope, FALSE, 
				  this, NoUndoPanelCommand::DeleteInteractors);
    } else {
	this->showSelectedInteractorsCmd = NULL;
	this->addSelectedInteractorsCmd = NULL;
	this->showSelectedStandInsCmd = NULL;
	this->deleteSelectedInteractorsCmd = NULL;
    }

    if (theDXApplication->appAllowsInteractorAttributeChange())  {
	this->setSelectedInteractorAttributesCmd =
	    new NoUndoPanelCommand("setInteractorAttributes", 
				  this->commandScope, FALSE, 
				  this, 
				  NoUndoPanelCommand::SetInteractorAttributes);
    } else {
	this->setSelectedInteractorAttributesCmd = NULL;
    }

    if (theDXApplication->appAllowsPanelEdit() && 
	theDXApplication->appAllowsEditorAccess())  
	this->setPanelCommentCmd =
	    new NoUndoPanelCommand("setPanelComment", 
				  this->commandScope, TRUE, 
				  this, 
				  NoUndoPanelCommand::SetPanelComment);
    else
	this->setPanelCommentCmd = NULL;


    if (theDXApplication->appAllowsPanelOptions())  {

	this->setPanelNameCmd =
	    new NoUndoPanelCommand("setPanelName", 
				  this->commandScope, TRUE, 
				  this, 
				  NoUndoPanelCommand::SetPanelName);
	this->setPanelAccessCmd =
	    new NoUndoPanelCommand("setPanelAccess", 
				  this->commandScope, TRUE, 
				  this, 
				  NoUndoPanelCommand::SetPanelAccess);

	// Don't provide this command in image mode because there's no way 
	// to toggle back to dev style when in image mode.
	this->togglePanelStyleCmd =
	    new NoUndoPanelCommand("togglePanelStyle", 
				  this->commandScope, 
    				  this->allowDeveloperStyleChange(),
				  this, 
				  NoUndoPanelCommand::TogglePanelStyle);
#if 00
	if (theDXApplication->inImageMode()) {
	    this->togglePanelStyleCmd->activate();
	} else {
	    this->togglePanelStyleCmd->deactivate();
	}
#endif

	this->setPanelGridCmd =
	    new NoUndoPanelCommand("setPanelGrid", 
				  this->commandScope, TRUE, 
				  this, 
				  NoUndoPanelCommand::SetPanelGrid);
	this->hitDetectionCmd = 
	    new NoUndoPanelCommand("hitDetection", 
				  this->commandScope, TRUE, 
				  this, 
				  NoUndoPanelCommand::HitDetection);
    } else {
	this->setPanelNameCmd = NULL;
	this->setPanelAccessCmd = NULL;
	this->setPanelGridCmd = NULL;
	this->togglePanelStyleCmd = NULL;
    }


    if (theDXApplication->appAllowsInteractorEdits()) {
	this->verticalLayoutCmd =
	    new NoUndoPanelCommand("verticalLayout", 
				  this->commandScope, TRUE, 
				  this, 
				  NoUndoPanelCommand::SetVerticalLayout);

	this->horizontalLayoutCmd =
	    new NoUndoPanelCommand("horizontalLayout", 
				  this->commandScope, TRUE, 
				  this, 
				  NoUndoPanelCommand::SetHorizontalLayout);
	this->setSelectedInteractorLabelCmd =
	    new NoUndoPanelCommand("setInteractorLabel", 
				  this->commandScope, FALSE, 
				  this, 
				  NoUndoPanelCommand::SetInteractorLabel);
    } else {
	this->verticalLayoutCmd   = NULL;
	this->horizontalLayoutCmd = NULL;
	this->setSelectedInteractorLabelCmd = NULL;
    }

    this->helpOnPanelCmd =
	new NoUndoPanelCommand("helpOnPanel", 
				  this->commandScope, FALSE, 
				  this, 
				  NoUndoPanelCommand::HelpOnPanel);

    this->comment = NUL(char*);
    this->title = NUL(char*); 

    //
    // Set up the default wXh for the panel work space.
    //
    this->workSpaceInfo.setWidth(500);
    this->workSpaceInfo.setHeight(500);
    this->workSpace = NULL;

    this->addingNodes = NUL(List*);

    this->xpos = this->ypos = -1;
    this->startup = TRUE;
    this->setNameDialog = NULL;
    this->setCommentDialog = NULL;
    this->helpOnPanelDialog = NULL;
    this->setLayoutButton = NULL;
    this->gridDialog   = NULL;
    this->setLabelDialog = NULL;
    this->stylePulldown = NULL;
    this->setStyleButton = NULL;
    this->setDimensionalityButton = NULL;
    this->addButton = NULL;

    this->developerStyle = TRUE;
    this->displayedStyle = NeverSet;
    this->develModeDiagWidth = this->develModeDiagHeight = 0;
    this->develModeGridding = FALSE;
    this->lastObjectParsed = NUL(WorkSpaceComponent*);
    // this->updated = TRUE;

    //
    // Install the default resources for THIS class (not the derived classes)
    //
    if (NOT ControlPanel::ClassInitialized)
    {
	ASSERT(theApplication);
        ControlPanel::ClassInitialized = TRUE;
	this->installDefaultResources(theApplication->getRootWidget());
    }

    //
    // Initially set hit detection to FALSE.  It's not saved in
    // the cfg file currently.
    //
    this->hit_detection = FALSE;

    this->java_variable = NUL(char*);
}


ControlPanel::~ControlPanel()
{
    this->clearInstances();

    // Throw out decorators
    ListIterator it(this->addingDecoratorList);
    Decorator *dec;
    while ( (dec = (Decorator *)it.getNext()) ) {
	delete dec;
    }
    this->addingDecoratorList.clear();

    it.setList(this->componentList);
    while ( (dec = (Decorator *)it.getNext()) ) {
	delete dec;
    }
    this->componentList.clear();



    //
    // Delete the File menu command interfaces
    //
    if (this->closeOption) delete this->closeOption;
    if (this->closeAllOption) delete this->closeAllOption;

    //
    // Delete the Edit menu command interfaces
    //
    if (this->verticalLayoutOption) delete this->verticalLayoutOption;
    if (this->horizontalLayoutOption) delete this->horizontalLayoutOption;
    if (this->setAttributesOption) delete this->setAttributesOption;
    if (this->setInteractorLabelOption) delete this->setInteractorLabelOption;
    if (this->deleteOption) delete this->deleteOption;
    if (this->addSelectedInteractorsOption) 
	delete this->addSelectedInteractorsOption;
    if (this->showSelectedInteractorsOption) 
	delete this->showSelectedInteractorsOption;
    if (this->showSelectedStandInsOption) 
	delete this->showSelectedStandInsOption;
    if (this->commentOption) 
	delete this->commentOption;

    //
    // Delete the Panels menu command interfaces
    //
    if (this->openAllControlPanelsOption) 
	delete this->openAllControlPanelsOption;
    if (this->openControlPanelByNameMenu) 
	delete this->openControlPanelByNameMenu;
    if (this->hitDetectionCmd)
	delete this->hitDetectionCmd;

    //
    // Delete the Options menu command interfaces
    //
    if (this->changeControlPanelNameOption) 
	delete this->changeControlPanelNameOption;
    if (this->setAccessOption) delete this->setAccessOption;
    if (this->gridOption) delete this->gridOption;
    if (this->styleOption) delete this->styleOption;

    //
    // Delete the Help menu command interfaces
    //
    if (this->onControlPanelOption) delete this->onControlPanelOption;
    if (this->onVisualProgramOption) delete this->onVisualProgramOption;
    if (this->userHelpOption) delete this->userHelpOption;

    //
    // Delete the strings that were allocated
    //
    if (this->comment) delete this->comment;

    //
    // Delete the commands (after the command interfaces).
    //
    if (this->closeCmd)
	delete this->closeCmd;


    if (this->verticalLayoutCmd)
	delete this->verticalLayoutCmd; 
    if (this->horizontalLayoutCmd)
	delete this->horizontalLayoutCmd; 
    if (this->addSelectedInteractorsCmd)
	delete this->addSelectedInteractorsCmd;
    if (this->showSelectedInteractorsCmd)
	delete this->showSelectedInteractorsCmd;
    if (this->showSelectedStandInsCmd)
	delete this->showSelectedStandInsCmd;
    if (this->deleteSelectedInteractorsCmd)
	delete this->deleteSelectedInteractorsCmd;
    if (this->setSelectedInteractorAttributesCmd)
	delete this->setSelectedInteractorAttributesCmd;
    if (this->setSelectedInteractorLabelCmd)
	delete this->setSelectedInteractorLabelCmd;
    if (this->setPanelCommentCmd)
	delete this->setPanelCommentCmd;

    if (this->togglePanelStyleCmd)
	delete this->togglePanelStyleCmd;
    if (this->setPanelGridCmd)
	delete this->setPanelGridCmd;
    if (this->setPanelNameCmd) 
	delete this->setPanelNameCmd;
    if (this->setPanelAccessCmd)
	delete this->setPanelAccessCmd;

    if (this->helpOnPanelCmd)
	delete this->helpOnPanelCmd;

    delete this->panelAccessManager;

    //
    // Delete self from the network panel list.
    //
    //this->network->deletePanel(this);

    delete this->workSpace;
    
    //
    // Delete the list of selected interactor nodes (selected for placement). 
    //
    if (this->addingNodes) delete this->addingNodes;


    //
    // Delete all dialogs 
    //
    if (this->setNameDialog) delete this->setNameDialog; 
    if (this->setCommentDialog) delete this->setCommentDialog; 
    if (this->helpOnPanelDialog) delete this->helpOnPanelDialog; 
    if (this->gridDialog) delete this->gridDialog;
    if (this->setLabelDialog) delete this->setLabelDialog;

    EditorWindow *editor = this->network->getEditor();
    if (editor && this == editor->currentPanel)
	editor->currentPanel = NULL;

    if (this->java_variable) delete this->java_variable;
}


//
// Install the default resources for this class.
//
void ControlPanel::installDefaultResources(Widget baseWidget)
{
    this->setDefaultResources(baseWidget, ControlPanel::DefaultResources);
    this->DXWindow::installDefaultResources(baseWidget);
}

void ControlPanel::initialize()
{

    if (!this->isInitialized()) {
	//
	// Now, call the superclass initialize().
	//
	this->DXWindow::initialize();
	
	// The Menubar and the CommandArea have both been added at
	// this point and managed; however for an initial CP
	// we don't want the command area initialized--so unmanage it.
	Widget w = this->getCommandArea();
	if (w) XtUnmanageChild(w);
    }
    //
    // FIXME: should this be done in manage() and then we could get
    // rid of this method?
    //
    this->setActivationOfEditingCommands();
}
//
// Get the next x,y position of an interactor being placed during a 
// series of placements into an initially empty control panel and 
// that use this method to get x,y positions of all placed interactors. 
//
// The size of instanceList == 0 prompted us to fetch values for our
// static ws_{width,height} but that meant that this routine could only be called
// on an empty ControlPanel.  Look for a starting value of x==-1 instead.
//
#define XY_SEP 5
void ControlPanel::getNextInteractorPosition(int *x, int *y)
{
    int cnt, next_x = XY_SEP, next_y = XY_SEP;
    static int ws_width, ws_height;

    //
    // Get the position of the last placed interactor.
    // (assumes interactors are appended and not prepended)
    //
    cnt = this->instanceList.getSize();
    if (cnt > 0) {
	int max_width = 0, last_x, last_y = 0, last_width, last_height;
	InteractorInstance *ii;
	ListIterator iterator(this->instanceList);

	last_x = 0;
	while ( (ii = (InteractorInstance*)iterator.getNext()) ) {
	    int xx, yy;
	    ii->getXYPosition(&xx,&yy);
	    if (xx >= last_x) {
		last_x = xx;
		last_y = yy;
		ii->getXYSize(&last_width, &last_height);

		// check for 0 because that's what getXYSize might return in the
		// future for interactors that have never been touched.
		if ((last_width==0) && (last_height==0)) {
		    Interactor *ntr = ii->getInteractor();
		    if (ntr)
			ntr->GetDimensions(ntr->getRootWidget(),&last_height,&last_width);
		}
	    }
	}
	iterator.setList(this->instanceList);
	while ( (ii = (InteractorInstance*)iterator.getNext()) ) {
	    int xx, yy;
	    ii->getXYPosition(&xx,&yy);
	    if (xx == last_x) {
		int width, height;
		ii->getXYSize(&width,&height);
		if ( width > max_width) {
		    max_width = width;
		}
	    }
	}
	next_y = last_y + last_height + XY_SEP;
	if (next_y > (ws_height-100)) { // rough approximation
	    next_x = last_x + max_width + XY_SEP;
	    next_y = XY_SEP;
	} else {
	    next_x = last_x;
	}
    } else {
	//  The first time, make sure the control panel is managed so
	// susequence getXYSize() requests will work.
	//
	// It would be better to use the dimesions of the c.p. instead
	// of the dimensions of the Workspace which are always >= c.p. dimensions
	//
	this->manage();
	this->getWorkSpace()->getXYSize(&ws_width,&ws_height);
    }

    *x = next_x;
    *y = next_y;
}

//
// Add all the editor's selected InteractorNodes to the control panel
//
void ControlPanel::addSelectedInteractorNodes()
{
    EditorWindow *editor = this->network->getEditor();
    List 	*l;

    ASSERT(editor);
    
    l = editor->makeSelectedNodeList(ClassInteractorNode);

    if (l) {
	ListIterator iterator;
	InteractorNode *n;
        int next_x,next_y, ws_width, ws_height;
	//
	// Now place all of the rest in the control panel. 
	// Interactors are placed in columns from top to bottom and when
	// we reach the bottom of the workspace we move over a column to 
	// right, starting at the top again.
	//
	iterator.setList(*l);
	this->getWorkSpace()->getXYSize(&ws_width,&ws_height);
	next_x = next_y = -1;
	while ( (n = (InteractorNode*)iterator.getNext()) ) {
	    this->getNextInteractorPosition(&next_x, &next_y);
	    this->createAndPlaceInteractor(n, next_x, next_y);
	}
 	delete l;	
    }
    
}

//
// We write or own manage so that we can manage the control panel
// and then create the interactors.  Interactor::createFrame()
// needs to be able to get the Display which requires the parent
// be realized.
//
void ControlPanel::manage()
{
    InteractorInstance *ii;

    //
    // Initialize, but don't manage until the interactors are craeated.
    // This makes for a more professional look when the panel comes up
    // the first time (i.e. the interactors are already in the panel when
    // it becomes visible).
    //
    if (!this->isInitialized())
	this->initialize();

    //
    // Why realize now? because if you don't, then the interactors must use
    // width,height values from the .cfg file or else they'll end up with 0,0.
    // If they always use values from the .cfg file, then they look goofy reading
    // in 2.1.5 files with huge interactors.
    //
    if ((!XtIsRealized(this->getRootWidget())) && (!this->developerStyle)) {
	XtRealizeWidget(this->getRootWidget());
    }
   
    //
    // Turn off Workspace space wars while adding interactors, if enabled
    //
    Boolean overlap;
    XtVaGetValues(this->getWorkSpaceWidget(), XmNallowOverlap, &overlap, NULL);
    if(!overlap)
	XtVaSetValues(this->getWorkSpaceWidget(), XmNallowOverlap, True, NULL);
    //
    // Display all the interactors that sit in this control panel. 
    //
    ListIterator iterator(this->instanceList);
    while ( (ii = (InteractorInstance*)iterator.getNext()) ) 
	ii->createInteractor();

    //
    // Display all the decorators that sit in this control panel. 
    // Decorator overrides the manage method.  Widget creation happens there.
    //
    iterator.setList(this->componentList);
    Decorator *dec;
    while ( (dec = (Decorator*)iterator.getNext()) ) 
	dec->manage(this->workSpace);

    //
    // Restore Workspace space wars state, if neccessary
    //
    if(!overlap)
	XtVaSetValues(this->getWorkSpaceWidget(), XmNallowOverlap, False,NULL);


    XtManageChild(this->getWorkSpaceWidget());

    //
    // Things get created using developerStyle==TRUE.  Reason: don't want to
    // know about XmForm attachments necessary for developerStyle==FALSE.
    // Interactors and Decorators in the cfg file have x,y,width,height not
    // XmNattachPosition values, so keep that code separate.
    //
    StyleEnum oldDisplayedStyle = this->displayedStyle;
    if (!this->developerStyle)
	this->setPanelStyle (this->developerStyle);

    this->DXWindow::manage();

    //
    // You can't do this every time thru manage.  You must only do this if the
    // control panel style is actually changin.
    //
#   if !defined(AIX312_MWM_IS_FIXED)
    if ((!this->developerStyle) && (oldDisplayedStyle != this->displayedStyle))
    {
	//
	// I call mwm on the rs6000 broken because you can't unset min{Width,Height}
	// if they were set prior to manage().
	//
	Widget diag = this->getRootWidget();
	Dimension width, height;
	XtVaGetValues (diag, XmNwidth, &width, XmNheight, &height, NULL);
	XtVaSetValues (diag, XmNminWidth, width, XmNminHeight, height, NULL);
    }
#   endif

    if(this->network->getEditor())
        this->network->getEditor()->notifyCPChange(FALSE);
    theDXApplication->notifyPanelChanged();

}

void ControlPanel::unmanage()
{
#ifndef AIX312_MWM_IS_FIXED
    Widget diag = this->getRootWidget();
    XtVaSetValues (diag, 
	XmNminWidth, XtUnspecifiedShellInt, 
	XmNminHeight, XtUnspecifiedShellInt, 
    NULL);
    XSync(XtDisplay(diag), False);
#endif
    this->DXWindow::unmanage();

    if(this->network->getEditor())
        this->network->getEditor()->notifyCPChange(FALSE);
    theDXApplication->notifyPanelChanged();
}

Widget ControlPanel::createWorkArea(Widget parent)
{
    Widget    frame;
    Widget    hBar;
    Widget    vBar;
    Dimension height;
    Dimension width;
    Dimension thickness;

    ASSERT(parent);

    //
    // Create the outer frame and form.
    //
    this->scrolledWindowFrame = frame =
	XtVaCreateManagedWidget
	    ("scrolledWindowFrame",
	     xmFrameWidgetClass,
	     parent,
	     XmNshadowType,   XmSHADOW_OUT,
	     XmNmarginHeight, SCRWF_MARGIN,
	     XmNmarginWidth,  SCRWF_MARGIN,
	     NULL);

    //
    // Create the scrolled window.
    //
    this->scrolledWindow =
	XtVaCreateManagedWidget
	    ("scrolledWindow",
	     xmScrolledWindowWidgetClass,
	     frame,
	     XmNscrollBarDisplayPolicy, XmAS_NEEDED,
	     XmNscrollingPolicy,        XmAUTOMATIC,
	     XmNvisualPolicy,           XmVARIABLE,
	     NULL);

    //
    // Create the canvas.
    //
    this->workSpace = new ControlPanelWorkSpace("panelCanvas", 
					this->scrolledWindow,  
					&this->workSpaceInfo, 
					this);

    this->workSpace->initializeRootWidget();	
    Boolean overlap;
    XtVaGetValues(this->getWorkSpaceWidget(), XmNallowOverlap, &overlap, NULL);
    this->allowOverlap = (boolean)overlap;

    XtVaSetValues(this->scrolledWindow, XmNworkWindow, 
				this->workSpace->getRootWidget(), NULL);

    //
    // Adjust the horizontal/vertical scrollbars to get rid of
    // highlighting feature.
    //
    XtVaGetValues(this->scrolledWindow,
		  XmNhorizontalScrollBar, &hBar,
		  XmNverticalScrollBar,   &vBar,
		  NULL);

    XtVaGetValues(hBar,
		  XmNhighlightThickness, &thickness,
		  XmNheight,             &height,
		  NULL);
    height -= thickness * 2;
    XtVaSetValues(hBar,
		  XmNhighlightThickness, 0,
		  XmNheight,             height,
		  NULL);

    XtVaGetValues(vBar,
		  XmNhighlightThickness, &thickness,
		  XmNwidth,              &width,
		  NULL);
    width -= thickness * 2;
    XtVaSetValues(vBar,
		  XmNhighlightThickness, 0,
		  XmNwidth,              width,
		  NULL);

    //
    // Return the topmost widget of the work area.
    //
    return frame;
}


void ControlPanel::createFileMenu(Widget parent)
{
    ASSERT(parent);

    Widget pulldown;
    int    buttons = 0;

    if (!theDXApplication->appAllowsPanelAccess())
	return;	// The File menu is empty

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

    if (buttons)
	XtVaCreateManagedWidget
	    ("optionSeparator", xmSeparatorWidgetClass, pulldown, NULL);

    //
    // If the application does not allow panel access from the pulldown 
    // menus (and requires that they are popped up at startup), then
    // be sure that the user can't close the window.  
    //
    if (theDXApplication->appAllowsPanelAccess()) {
	this->closeOption =
	    new ButtonInterface(pulldown, "panelCloseOption", this->closeCmd);

	this->closeAllOption =
	    new ButtonInterface(pulldown, "panelCloseAllOption", 
			    this->network->getCloseAllPanelsCommand());
    }
}


void ControlPanel::createEditMenu(Widget parent)
{
    ASSERT(parent);

    Widget pulldown, layoutPulldown;
    int    i, buttons;
    


    //
    // Create "Edit" menu and options.
    //
    pulldown =
	this->editMenuPulldown =
	    XmCreatePulldownMenu(parent, "editMenuPulldown", NUL(ArgList), 0);
    this->editMenu =
	XtVaCreateManagedWidget
	    ("editMenu",
	     xmCascadeButtonWidgetClass,
	     parent,
	     XmNsubMenuId, pulldown,
	     NULL);

    //
    // 'Set the style of an interactor' pulldown option. 
    //
    if (theDXApplication->appAllowsInteractorEdits()) {
	//
	// Style 
	//
	this->stylePulldown =
		XmCreatePulldownMenu
		    (pulldown, "stylePulldown", NUL(ArgList), 0);

	this->setStyleButton =
	    XtVaCreateManagedWidget
		("panelSetStyleButton",
		 xmCascadeButtonWidgetClass,
		 pulldown,
		 XmNsubMenuId, this->stylePulldown,
		 XmNsensitive, TRUE,
		 NULL);

	XtAddCallback(pulldown,
		  XmNmapCallback,
		  (XtCallbackProc)ControlPanel_EditMenuMapCB,
		  (XtPointer)this);

	//
	// Dimensionality 
	//
	Widget dimensionalityPulldown =
		XmCreatePulldownMenu
		    (pulldown, "dimensionalityPulldown", NUL(ArgList), 0);

	this->setDimensionalityButton =
	    XtVaCreateManagedWidget
		("panelSetDimensionalityButton",
		 xmCascadeButtonWidgetClass,
		 pulldown,
		 XmNsubMenuId, dimensionalityPulldown,
		 XmNsensitive, TRUE,
		 NULL);

	for (i=1 ; i<=3 ; i++) {
	    char buf[32];
	    sprintf(buf,"set%dDOption",i);
	    Widget btn = XmCreatePushButton(dimensionalityPulldown, 
						buf, NULL, 0);
	    XtVaSetValues(btn, XmNuserData, i, NULL);
	    XtAddCallback(btn,
		      XmNactivateCallback,
		      (XtCallbackProc)ControlPanel_SetDimensionsCB,
		      (XtPointer)this);
	    this->dimensionalityList.appendElement((void*)btn);
	    XtManageChild(btn);
	}

#if 0
	// Don't install this twice?
	// I would just take it out, but we are just before release 3.1
	XtAddCallback(pulldown,
		  XmNmapCallback,
		  (XtCallbackProc)ControlPanel_EditMenuMapCB,
		  (XtPointer)this);
#endif


	//
	// Layout (horizontal/vertical)
	//
	layoutPulldown =
		XmCreatePulldownMenu
		    (pulldown, "layoutPulldown", NUL(ArgList), 0);

	ASSERT(this->verticalLayoutCmd);
	this->verticalLayoutOption = new ButtonInterface(layoutPulldown, 
			    "panelVerticalOption", this->verticalLayoutCmd);

	ASSERT(this->horizontalLayoutCmd);
	this->horizontalLayoutOption = new ButtonInterface(layoutPulldown, 
			    "panelHorizontalOption", this->horizontalLayoutCmd);

	this->setLayoutButton =
	    XtVaCreateManagedWidget
		("panelSetLayoutButton",
		 xmCascadeButtonWidgetClass,
		 pulldown,
		 XmNsubMenuId, layoutPulldown,
		 XmNsensitive, False,
		 NULL);
    } 

    //
    // 
    //
    buttons = 0;
    if (this->setSelectedInteractorAttributesCmd) {
	this->setAttributesOption =
	    new ButtonInterface(pulldown, "panelSetAttributesOption", 
			    this->setSelectedInteractorAttributesCmd);
  	buttons = 1;
    }

    if (this->setSelectedInteractorLabelCmd) {
	this->setInteractorLabelOption =
	    new ButtonInterface
	    (pulldown, "panelSetInteractorLabelOption", 
			this->setSelectedInteractorLabelCmd);
  	buttons = 1;
    }

    if (this->deleteSelectedInteractorsCmd) {
	this->deleteOption =
	    new ButtonInterface(pulldown, "panelDeleteOption", 
			    this->deleteSelectedInteractorsCmd);
  	buttons = 1;
    }

    if (theDXApplication->appAllowsInteractorEdits()) {
	if (buttons)
	    XtVaCreateManagedWidget
		    ("optionSeparator", xmSeparatorWidgetClass, pulldown, NULL);
	//
	// Add Decorators
	//
	Widget addPulldown =
		XmCreatePulldownMenu
		    (pulldown, "addPulldown", NUL(ArgList), 0);

	Dictionary *dict = DecoratorStyle::GetDecoratorStyleDictionary();
	Dictionary *subdict;
	DictionaryIterator di(*dict);
	while ( (subdict = (Dictionary*)di.getNextDefinition()) ) {
	    Arg args[4];
	    DictionaryIterator subdi(*subdict);
	    DecoratorStyle *ds = (DecoratorStyle*)subdi.getNextDefinition();
	    ASSERT(ds);
	    if (ds->allowedInVPE()) continue;
	    char *stylename = (char*)ds->getNameString();
	    XmString label = XmStringCreateSimple(stylename);
	    int n = 0;
	    XtSetArg (args[n], XmNlabelString, label); n++;
	    XtSetArg (args[n], XmNuserData, ds); n++;
	    Widget btn = XmCreatePushButton(addPulldown, "stylename", args, n);
	    XtAddCallback(btn, XmNactivateCallback,
		(XtCallbackProc)ControlPanel_PlaceDecoratorCB,
		(XtPointer)this);
	    XtManageChild(btn);
	    XmStringFree(label);
	}

	this->addButton =
	    XtVaCreateManagedWidget
		("panelAddButton",
		 xmCascadeButtonWidgetClass,
		 pulldown,
		 XmNsubMenuId, addPulldown,
		 NULL);

	XtVaCreateManagedWidget
		("optionSeparator", xmSeparatorWidgetClass, pulldown, NULL);
	buttons = 0;
    }

    if (buttons &&
	(this->addSelectedInteractorsCmd || 
	 this->showSelectedInteractorsCmd)) {
	XtVaCreateManagedWidget
		("optionSeparator", xmSeparatorWidgetClass, pulldown, NULL);
    }

    buttons = 0;
    if (this->addSelectedInteractorsCmd) {
	this->addSelectedInteractorsOption =
	    new ButtonInterface
		(pulldown, "panelAddSelectedInteractorsOption", 
			    this->addSelectedInteractorsCmd);
  	buttons = 1;
    }



    if (this->showSelectedInteractorsCmd) {
	this->showSelectedInteractorsOption =
	    new ButtonInterface 
		(pulldown, "panelShowSelectedInteractorsOption", 
			    this->showSelectedInteractorsCmd);
  	buttons = 1;
    }

    if (this->showSelectedStandInsCmd) {
	this->showSelectedStandInsOption =
	    new ButtonInterface 
		(pulldown, "panelShowSelectedStandInsOption", 
			    this->showSelectedStandInsCmd);
  	buttons = 1; 
    }

    if (buttons && this->setPanelCommentCmd)
	XtVaCreateManagedWidget
		("optionSeparator", xmSeparatorWidgetClass, pulldown, NULL);

    //
    // Add a comment option pull down
    //
    if (this->setPanelCommentCmd)
	this->commentOption =
	    new ButtonInterface(pulldown, "panelCommentOption",
					this->setPanelCommentCmd);
}


void ControlPanel::createPanelsMenu(Widget parent)
{
    ASSERT(parent);

    Widget            pulldown;

    //
    // Create "Panels" menu and options.
    //
    pulldown =
	this->panelsMenuPulldown =
	    XmCreatePulldownMenu
		(parent, "panelsMenuPulldown", NUL(ArgList), 0);
    this->panelsMenu =
	XtVaCreateManagedWidget
	    ("panelsMenu",
	     xmCascadeButtonWidgetClass,
	     parent,
	     XmNsubMenuId, pulldown,
	     NULL);

    if (theDXApplication->appAllowsOpenAllPanels()) 
	this->openAllControlPanelsOption =
	    new ButtonInterface
		(pulldown,
		 "panelOpenAllControlPanelsOption",
		 this->network->getOpenAllPanelsCommand());

   this->openControlPanelByNameMenu =
                new CascadeMenu("panelPanelCascade",pulldown);

    XtAddCallback(pulldown,
                  XmNmapCallback,
                  (XtCallbackProc)ControlPanel_PanelMenuMapCB,
                  (XtPointer)this);

}


void ControlPanel::createOptionsMenu(Widget parent)
{
    ASSERT(parent);

    Widget            pulldown;
    int		      buttons = 0;

    //
    // Create "Options" menu and options.
    //
    pulldown =
	this->optionsMenuPulldown =
	    XmCreatePulldownMenu
		(parent, "optionsMenuPulldown", NUL(ArgList), 0);
    this->optionsMenu =
	XtVaCreateManagedWidget
	    ("optionsMenu",
	     xmCascadeButtonWidgetClass,
	     parent,
	     XmNsubMenuId, pulldown,
	     NULL);

    if (this->hitDetectionCmd) {
	this->hitDetectionOption =
	    new ToggleButtonInterface(pulldown, "hitDetectionOption", 
			    this->hitDetectionCmd, this->hit_detection);
	buttons++;
    }

    if (this->setPanelNameCmd) {
	this->changeControlPanelNameOption =
	    new ButtonInterface
		(pulldown, "panelChangeControlPanelNameOption", 
			    this->setPanelNameCmd);
	buttons++;
    }

    if (this->setPanelAccessCmd) {
	this->setAccessOption =
	    new ButtonInterface(pulldown, "panelSetAccessOption", 
			    this->setPanelAccessCmd);
	buttons++;
    }

    if (this->setPanelGridCmd) {
	this->gridOption =
	    new ButtonInterface(pulldown, "panelGridOption", 
			    this->setPanelGridCmd);
	buttons++;
    }

    if (this->togglePanelStyleCmd) {
	this->styleOption =
	    new ButtonInterface(pulldown, "panelStyleOption", 
			    this->togglePanelStyleCmd);
	buttons++;
	XtAddCallback(pulldown,
		  XmNmapCallback,
		  (XtCallbackProc)ControlPanel_OptionsMenuMapCB,
		  (XtPointer)this);
    }

    if (theDXApplication->appAllowsPanelOptions())  {
	if (buttons) 
	    XtVaCreateManagedWidget
		    ("optionSeparator", xmSeparatorWidgetClass, pulldown, NULL);
	this->addStartupToggleOption(pulldown);
    }

}


void ControlPanel::createHelpMenu(Widget parent)
{
    ASSERT(parent);

    this->DXWindow::createHelpMenu(parent);

    XtVaCreateManagedWidget("separator", xmSeparatorWidgetClass,
                                        this->helpMenuPulldown,
                                        NULL);

    this->onVisualProgramOption = 
	new ButtonInterface(this->helpMenuPulldown, 
				"panelOnVisualProgramOption", 
				this->network->getHelpOnNetworkCommand());
    this->onControlPanelOption =
	new ButtonInterface(this->helpMenuPulldown, 
				"panelOnControlPanelOption", 
				this->helpOnPanelCmd);
}


void ControlPanel::createMenus(Widget parent)
{
    ASSERT(parent);

    //
    // Create the menus.
    //
    this->createFileMenu(parent);
    if (theDXApplication->appAllowsPanelEdit())
        this->createEditMenu(parent);
    this->createExecuteMenu(parent);
    if (theDXApplication->appAllowsPanelAccess())
	this->createPanelsMenu(parent);
    if (theDXApplication->appAllowsPanelOptions())
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

extern "C" void ControlPanel_PanelMenuMapCB(Widget ,
                                 XtPointer clientdata,
                                 XtPointer )
{
    ControlPanel *cp = (ControlPanel*)clientdata;
    Network *network = cp->network;
    CascadeMenu *menu = cp->openControlPanelByNameMenu;
    PanelAccessManager *panelMgr = cp->panelAccessManager;
    network->fillPanelCascade(menu, panelMgr);
}
extern "C" void ControlPanel_OptionsMenuMapCB(Widget ,
                                 XtPointer clientdata,
                                 XtPointer )
{
    ControlPanel *cp = (ControlPanel*)clientdata;
    Command *cmd;

    if ( (cmd = cp->togglePanelStyleCmd) ) {
	if (cp->allowDeveloperStyleChange()) 
	    cmd->activate();
	else
	    cmd->deactivate();
    }
    
}

extern "C" void ControlPanel_SetDimensionsCB(Widget w, XtPointer clientdata,
                               XtPointer )
{
    ControlPanel *cp = (ControlPanel*)clientdata;
    cp->setDimensionsCallback(w, clientdata);

}
void ControlPanel::setDimensionsCallback(Widget w, XtPointer )
{
    int dim;
    InteractorInstance *ii = NULL;
    boolean only_one_selected =  this->findTheSelectedInteractorInstance(&ii);
    ASSERT(only_one_selected);
    InteractorNode *inode;

    ASSERT(ii);
    inode = (InteractorNode*)ii->getNode();
    ASSERT(inode);

    XtPointer ptr;
    XtVaGetValues(w,XmNuserData,&ptr,NULL);
    dim = (int)(long)ptr;
    ASSERT(dim>0);
    inode->changeDimensionality(dim);
}

extern "C" void ControlPanel_EditMenuMapCB(Widget w,
                                 XtPointer clientdata,
                                 XtPointer calldata)
{
    ControlPanel *cp = (ControlPanel*)clientdata;
    Dictionary *dict;
    char  *stylename;
    InteractorInstance *ii;
    InteractorNode *inode = NULL;
    Widget    btn;

    boolean only_one_selected =  cp->findTheSelectedInteractorInstance(&ii);


    if (ii) 
	inode = (InteractorNode*)ii->getNode();

    if (cp->setStyleButton) {
	ASSERT(cp->stylePulldown);

	if(cp->styleList.getSize())
	{
	    ListIterator li(cp->styleList);
	    while( (btn = (Widget)li.getNext()) )
		 XtDestroyWidget(btn);
	    cp->styleList.clear();
	}

	if (!only_one_selected) {	
	    XtVaSetValues(cp->setStyleButton, XmNsensitive, False, NULL);
	} else { 

	    ASSERT(inode);
	    dict = InteractorStyle::GetInteractorStyleDictionary(
						inode->getNameString());

	    ASSERT(dict);
	    if (dict->getSize() <= 1 && cp->setStyleButton) {
		XtVaSetValues(cp->setStyleButton, XmNsensitive, False, NULL);
	    } else {

		XtVaSetValues(cp->setStyleButton, XmNsensitive, True, NULL);

		DictionaryIterator di(*dict);
		InteractorStyle *is;

		const char *current_style = ii->getStyle()->getNameString();
		XmString label;

		while ( (is = (InteractorStyle*)di.getNextDefinition()) )
		{
		    stylename = (char*)is->getNameString();
		    label = XmStringCreateSimple(stylename);
		    btn = XmCreatePushButton(cp->stylePulldown,
					     "stylename",
					     NULL,
					     0);
		    XtVaSetValues(btn,
				  XmNlabelString, label,
				  NULL);
		    if (strcmp(stylename,current_style)) {
			XtVaSetValues(btn,
				      XmNuserData, is,
				      NULL);
			XtAddCallback(btn,
				      XmNactivateCallback,
				      (XtCallbackProc)ControlPanel_SetStyleCB,
				      (XtPointer)ii);
		    } else {
			XtVaSetValues(btn, XmNsensitive, False, NULL);
		    }
		    cp->styleList.appendElement((void*)btn);
		    XtManageChild(btn);
		    XmStringFree(label);
		}
	    }
   	}
    }   // End of setStyleButton and stylePulldown configuration

    if (cp->setDimensionalityButton) {

	Boolean sensitive;
	if (only_one_selected) {
	    ASSERT(inode);
	    boolean dyno_dim = inode->hasDynamicDimensionality();
	    if (!dyno_dim) {
		sensitive = False;
	    } else {
		sensitive = True;
		int i=0, current_dim = inode->getComponentCount();
		ListIterator iterator(cp->dimensionalityList); 
		while ( (w = (Widget)iterator.getNext()) ) {
		    if (++i == current_dim)
			XtSetSensitive(w,False);
		    else
			XtSetSensitive(w,True);
		}
	    }
	} else { 
	    sensitive = False;
	}
	XtVaSetValues(cp->setDimensionalityButton, XmNsensitive, 
						sensitive, NULL);
    }   // End of setDimensionalityButton and DimensionalityPulldown 
 	// configuration
}

extern "C" void ControlPanel_SetStyleCB(Widget w,
                                 XtPointer clientdata,
                                 XtPointer calldata)
{
    InteractorInstance *ii = (InteractorInstance*)clientdata;
    InteractorStyle    *is;

    XtVaGetValues(w, XmNuserData, &is, NULL);
    ASSERT(is AND ii);

    ii->changeStyle(is);

}

//
// Create a decorator, add it to the addingDecoratorList , 
// its defaultWindow will be opened by the guy who manages it.
// There will be a callback in ControlPanelWorkSpace for the button
// click which places the decorator.  That code is tied to ControlPanel::
// placeSelectedInteractorCallback.  That cp func now checks the
// addingDecoratorList for decorators to add.
//
extern "C" void ControlPanel_PlaceDecoratorCB(Widget w, 
	XtPointer clientdata, XtPointer )
{
ControlPanel *cp = (ControlPanel*)clientdata;
DecoratorStyle *ds;

    ASSERT(cp);
    XtVaGetValues (w, XmNuserData, &ds, NULL);
    ASSERT(ds);

    Decorator *d = ds->createDecorator(cp->developerStyle);
    ASSERT(d);
    d->setStyle (ds);
    DecoratorInfo* dnd = new DecoratorInfo (cp->getNetwork(), (void*)cp, 
	(DragInfoFuncPtr)ControlPanel::SetOwner,
	(DragInfoFuncPtr)ControlPanel::DeleteSelections);
    d->setDecoratorInfo (dnd);
    cp->getNetwork()->setFileDirty();

    // want to do this?
    if (cp->addingDecoratorList.getSize()) {
	Decorator *dec;
	ListIterator it(cp->addingDecoratorList);
	while ( (dec = (Decorator *)it.getNext()) ) {
	    delete dec;
	}
	cp->addingDecoratorList.clear();
    }
    cp->addingDecoratorList.appendElement((void *)d);

    //
    // Set the cursor to indicate placement
    //
    ASSERT(cp->workSpace);
    cp->workSpace->setCursor(UPPER_LEFT);

    //
    // Not necessary, but be less confusing
    //
    if (cp->addingNodes)
        cp->addingNodes->clear();

    //
    // Adding a decorator requires new info in the .cfg file 
    //
    cp->getNetwork()->setFileDirty();
}

//
// Print the information about the control panel in the .cfg file. 
//
boolean ControlPanel::cfgPrintPanel(FILE *f, PrintType dest)
{
    ASSERT(f);
 
    if (fprintf(f,"//\n") < 0)
	return FALSE;

    return this->cfgPrintPanelComment(f, dest)   && 
	   this->cfgPrintTitleComment(f, dest)   && 
	   this->cfgPrintCommentComment(f, dest) && 
	   this->cfgPrintDecoratorComment(f, dest) && 
	   this->panelAccessManager->cfgPrintInaccessibleComment(f) && 
	   this->workSpaceInfo.printComments(f); 
}
//
// Parse the comments in the .cfg file. 
//
boolean ControlPanel::cfgParseComment(const char* comment,
                                const char *filename, int lineno)
{
boolean retVal;

    ASSERT(comment);
    retVal = FALSE;

    /*
     * Try and parse known comments for a control panel. 
     */

    //
    // Parsing gained state with the addition of resource comments.  Currenly
    // only decorators have resource comments.  These new comments are handled
    // by ::cfgParseDecoratorComment().
    //
    if (this->lastObjectParsed) {
	retVal = this->cfgParseDecoratorComment(comment,filename,lineno);
	if (!retVal) {
	    retVal =
		this->cfgParsePanelComment(comment, filename, lineno) ||
		this->cfgParseTitleComment(comment,filename,lineno)   ||
		this->cfgParseCommentComment(comment,filename,lineno) ||
		this->panelAccessManager->cfgParseInaccessibleComment(
					comment,filename,lineno) ||
		this->workSpaceInfo.parseComment(comment,filename,lineno);
	    lastObjectParsed = NUL(WorkSpaceComponent*);
	}
    } else {

        retVal = 
	   this->cfgParsePanelComment(comment, filename, lineno) ||
	   this->cfgParseTitleComment(comment,filename,lineno)   ||
	   this->cfgParseCommentComment(comment,filename,lineno) ||
	   this->cfgParseDecoratorComment(comment,filename,lineno) ||
	   this->panelAccessManager->cfgParseInaccessibleComment(
					comment,filename,lineno) ||
	   this->workSpaceInfo.parseComment(comment,filename,lineno);
    }

    return retVal;
}

//
// Print the 'decorator' set of comments in the .cfg file.
//
boolean ControlPanel::cfgPrintDecoratorComment (FILE *f, PrintType dest)
{
Decorator *dec;
    ASSERT(f);
 
    if (fprintf(f,"//\n") < 0)
	return FALSE;

    ListIterator it(this->componentList);
    while ( (dec = (Decorator *)it.getNext()) ) {
	if (dest!=PrintCPBuffer) {
	    if (!dec->printComment(f)) return FALSE;
	} else if (dec->isSelected()) {
	    if (!dec->printComment(f)) return FALSE;
	}
    }
    return TRUE;
}

//
// Parse the 'decorator' comments in the .cfg file.
//
//
// Parse the 'resource' comments in the .cfg file for DynamicResource
// ControlPanel saves state in order to handle resource comments because
// each DynamicResource must belong to a decorator or interactor and it will
// always be the most recently parsed decorator or interactor.
//
boolean ControlPanel::cfgParseDecoratorComment (const char *comment,
                                const char *filename, int lineno)
{
int items_parsed;
char decoType[128];
char stylename[128];

    ASSERT(comment);

    if ((strncmp(" decorator",comment,10))&&
	(strncmp(" annotation",comment,11))&&
	(strncmp(" resource",comment,9)))
	return FALSE;

    // The following is a special case for stateful resource comments.
    // Currently they are associated only with Decorators but that
    // will change.
    if (EqualSubstring(" resource",comment,9)) {
	if (!this->lastObjectParsed)
	    return FALSE;
	return this->lastObjectParsed->parseResourceComment (comment, filename, lineno);
    } else if (EqualSubstring(" annotation", comment, 11)) {
	if (!this->lastObjectParsed)
	    return FALSE;
	ASSERT (this->lastObjectParsed->isA(ClassLabelDecorator));
	LabelDecorator* lab = (LabelDecorator*)this->lastObjectParsed;
	return lab->parseComment (comment, filename, lineno);
    }

    items_parsed = sscanf(comment, " decorator %[^\t]\tstyle(%[^)]", decoType, stylename);
    if (items_parsed != 2)
	items_parsed = sscanf(comment, " decorator %[^\t]", decoType);

    if ((items_parsed != 1) && (items_parsed != 2)) {
	ErrorMessage("Unrecognized 'decorator' comment (file %s, line %d)",
					    filename, lineno);
	return FALSE;
    }

    DecoratorStyle *ds = NUL(DecoratorStyle*);
    if (items_parsed == 1) {
	ds = DecoratorStyle::GetDecoratorStyle(decoType,DecoratorStyle::DefaultStyle);
	if (!ds) {
	    ErrorMessage("Unrecognized 'decorator' type (file %s, line %d)",
						filename, lineno);
	    return FALSE;
	}
    } else {
	Dictionary *dict = DecoratorStyle::GetDecoratorStyleDictionary(decoType);
	if (!dict) {
	    ErrorMessage("Unrecognized 'decorator' type (file %s, line %d)",
						filename, lineno);
	    return FALSE;
	}
	DictionaryIterator di(*dict);
	while ( (ds = (DecoratorStyle*)di.getNextDefinition()) ) {
	    if (EqualString(stylename, ds->getNameString())) break;
	}
	if (!ds) {
	    ErrorMessage("Unrecognized 'decorator' type (file %s, line %d)",
						filename, lineno);
	    return FALSE;
	}
    }

    ASSERT(ds);
    Decorator *d;
    d = ds->createDecorator (this->developerStyle);
    d->setStyle (ds);
    DecoratorInfo* dnd = new DecoratorInfo (this->getNetwork(), (void*)this, 
	(DragInfoFuncPtr)ControlPanel::SetOwner,
	(DragInfoFuncPtr)ControlPanel::DeleteSelections);
    d->setDecoratorInfo (dnd);

    if (!d->parseComment (comment, filename, lineno))
	ErrorMessage("Unrecognized 'decorator' comment (file %s, line %d)",
					    filename, lineno);

    this->addComponentToList ((void *)d);
    this->lastObjectParsed = d;

    return TRUE;
}

/* 
    ParseGeometryComment actually performs the Window placement and
    Window sizing for shell windows such as the ImageWindow and 
    Control Panels. The network saves the placement as a normalized
    vector of where and what size it was on the authors screen 
    and then tries to replicate that to fit the screen of others. 
    This becomes a real problem when Xinerama comes into play. For example,
    the width/height ratio changes significantly and makes ImageWindows
    very wide when reconstructing them if the Xinerama environment is
    in place. For example DisplayWidth may be 2560 and Height may only
    be 1024. But the original developer was on a system of 1280 x 1024.
    The resulting image doesn't look anything like the authors.

    The fix for placement is to check for Xinerama and replicate on
    the dominate screen. Then this also needs to be fixed in PrintGeometryComment
    as well.
    
    It will not be possible to replicate the multiple screen geometry
    unless the support for Xinerama is turned off. For example: two
    identical screens side by side. If the user builds with the VPE on
    the primary screen, and puts the Image Window on the secondary screen,
    when the Editor is re-opened and executed, the Image Window will
    appear on the primary screen. The only way to fix this would be
    to append the screen number in the GeometryComment
 */

//
// Print the 'panel' comment in the .cfg file. 
//
boolean ControlPanel::cfgPrintPanelComment(FILE *f, PrintType ptype)
{
    int xpos, ypos, xsize, ysize;
    
    this->getGeometry(&xpos, &ypos, &xsize, &ysize);

    if (!this->isInitialized()) {
	//
	// If the window was never displayed, just use the values from
	// the .cfg file.  Note that we don't need to do the same with
	// the height and width as they are always applied to the window
	// and MainWindow saves/restores those values even if the window
	// was not initialized.
	//
		xpos = this->xpos;
		ypos = this->ypos;
    } 

	Display *disp = theApplication->getDisplay();
	
    int width = DisplayWidth(disp,0);
    int height = DisplayHeight(disp,0);
    int x=0, y=0;
    int screen = -1;

 #if defined(HAVE_XINERAMA)
			// Do some Xinerama Magic to use the largest main screen.
			int dummy_a, dummy_b;
			int screens;
			XineramaScreenInfo   *screeninfo = NULL;
	
			if ((XineramaQueryExtension (disp, &dummy_a, &dummy_b)) &&
				(screeninfo = XineramaQueryScreens(disp, &screens))) {
				// Xinerama Detected
		
				if (XineramaIsActive(disp)) {
					width = 0; height = 0;
					int i = dummy_a;
					while ( i < screens ) {
						if(xpos > screeninfo[i].x_org && 
							xpos < screeninfo[i].x_org+screeninfo[i].width &&
							ypos > screeninfo[i].y_org &&
							ypos < screeninfo[i].y_org+screeninfo[i].height ) {
							screen = i;
							width = screeninfo[i].width;
							height = screeninfo[i].height;
							x = screeninfo[i].x_org;
							y = screeninfo[i].y_org;
						}
						i++;
					}
				}
			}
#endif

    float norm_xsize = (float)xsize/width;
    float norm_ysize = (float)ysize/height;
    float norm_xpos  = (float)(xpos - x)/width;
    float norm_ypos  = (float)(ypos - y)/height;

    //
    // Be sure to print 0 for startup if ptype == PrintCPBuffer. (Dnd within
    // a ControlPanel)  Reason:  Network::readNetwork will manage the panel
    // and it looks stupid.
    //

	if(screen != -1) {
	    return fprintf(f, "// panel[%d]: position = (%6.4f,%6.4f), "
		"size = %6.4fx%6.4f, startup = %d, devstyle = %d, screen = %d\n",
	       this->getInstanceNumber() - 1, // 0 indexed in file
	       norm_xpos,
	       norm_ypos,
	       norm_xsize,
	       norm_ysize,
	       ((ptype == PrintCPBuffer)?0:this->startup),
	       this->developerStyle,
	       screen) >= 0;
	}

    return fprintf(f, "// panel[%d]: position = (%6.4f,%6.4f), "
		"size = %6.4fx%6.4f, startup = %d, devstyle = %d\n",
	       this->getInstanceNumber() - 1, // 0 indexed in file
	       norm_xpos,
	       norm_ypos,
	       norm_xsize,
	       norm_ysize,
	       ((ptype == PrintCPBuffer)?0:this->startup),
	       this->developerStyle) >= 0;
}
//
// Parse the 'panel' comment in the .cfg file. 
//
boolean ControlPanel::cfgParsePanelComment(const char* comment,
                                const char *filename, int lineno)
{
    int       items_parsed;
    int       x = 0, y = 0;
    int		  xpos, ypos, xsize, ysize;
    int       width;
    int       height;
    int		  screen = -1;
    int       old_id;
    int	      devstyle;
    int       startup;
    float 	norm_x, norm_y, norm_width, norm_height;

    ASSERT(comment);

    if (strncmp(" panel",comment,6))
	return FALSE;

    items_parsed =
	sscanf(comment,
	      " panel[%d]: position = (%f,%f), size = %fx%f, startup = %d, devstyle = %d, screen = %d",
	       &old_id,
	       &norm_x,
	       &norm_y,
	       &norm_width,
	       &norm_height,
	       &startup,
	       &devstyle,
	       &screen);

    if ((items_parsed == 6)||(items_parsed == 7)||(items_parsed == 8)) {
    	Display *disp = theApplication->getDisplay();
		int width = DisplayWidth(disp,0);
		int height = DisplayHeight(disp,0);

 #if defined(HAVE_XINERAMA)
			// Do some Xinerama Magic to use the largest main screen.
			int dummy_a, dummy_b;
			int screens;
			XineramaScreenInfo   *screeninfo = NULL;
	
			if ((XineramaQueryExtension (disp, &dummy_a, &dummy_b)) &&
				(screeninfo = XineramaQueryScreens(disp, &screens))) {
				// Xinerama Detected
		
				if (XineramaIsActive(disp)) {
					width = 0; height = 0;
					int i = dummy_a;
					if(screen != -1 && screen < screens) {
						width = screeninfo[screen].width;
						height = screeninfo[screen].height;
						x = screeninfo[screen].x_org;
						y = screeninfo[screen].y_org;
					} else
					while ( i < screens ) {
						if(screeninfo[i].width > width) {
							width = screeninfo[i].width;
							height = screeninfo[i].height;
							x = screeninfo[i].x_org;
							y = screeninfo[i].y_org;
						}
						i++;
					}
				}
			}
#endif		
		
		xpos = (int) (width * norm_x + .5 + x);
        ypos = (int) (height * norm_y + .5 + y);
        xsize = (int) (width * norm_width  + .5);
        ysize = (int) (height * norm_height + .5);
		if (items_parsed == 7)
			this->developerStyle = devstyle; // initialized elsewhere
    } else {

	// Try older style comment
	items_parsed =
	    sscanf(comment,
		   " panel[%d]: x = %d, y = %d, width = %d, height = %d,"
		   " startup = %d",
		   &old_id,
		   &xpos,
		   &ypos,
		   &xsize,
		   &ysize,
		   &startup);

	if (items_parsed != 6)	// Try an even older style panel comment
	{
	    items_parsed =
		sscanf(comment,
		       " panel[%d]: x = %d, y = %d, width = %d, height = %d",
		       &old_id,
		       &xpos,
		       &ypos,
		       &xsize,
		       &ysize);

	    if (items_parsed == 5)
	    {
		startup = TRUE;
	    }
	    else
	    {
		ErrorMessage("Unrecognized 'panel' comment (file %s, line %d)",
					    filename, lineno);
		return FALSE;
	    }
	}
    }

    // 
    // Id numbers are zero based in file, but 1 based internally.
    // 
    old_id++;	

    //
    // The following verifies panel instance numbers and relies on the
    // fact that the network resets panel instance numbers before reading
    // a .cfg file.
    //
    if (this != this->network->getPanelByInstance(old_id)) {
        int last_id = this->network->getLastPanelInstanceNumber();
	if (old_id <= last_id) {
	    WarningMessage(
		"Panel instance number not found in current network (file %s, line %d)",
		filename,lineno);
	} else { 
	    this->network->setLastPanelInstanceNumber(old_id);
	    this->instanceNumber = old_id;
	}
    }

    if (theDXApplication->applyWindowPlacements()) {
		this->setGeometry(xpos,ypos,xsize,ysize);
    } else {
	//
	// Always fix the size of the control panel.
	//
		this->setXYSize(xsize,ysize);
    }

    this->xpos    = xpos;
    this->ypos    = ypos;
    this->startup = (boolean)startup;

    return TRUE;
}

//
// Print the 'comment' comment in the .cfg file. 
//
boolean ControlPanel::cfgPrintCommentComment(FILE *f, PrintType)
{
    if (!this->comment)
	return TRUE;


    if (fprintf(f,"// comment: ") < 0)
	return FALSE;

    int i, len = STRLEN(this->comment);
    for (i=0 ; i<len ; i++) {
	char c = this->comment[i];
	if (putc(c, f) == EOF)
	    return FALSE;
	if ((c == '\n') && (i+1 != len)) { 
	    if (fprintf(f,"// comment: ") < 0)
		return FALSE;	
	}
    }
    if (fprintf(f,"\n//\n") < 0)
	return FALSE;	

    return TRUE;
}
//
// Parse the comments in the .cfg file after the panel comment.
//
boolean ControlPanel::cfgParseCommentComment(const char *comment,
                                const char *, int )
{
   
    char *c, *keyword = " comment: ";
    int keyword_size = STRLEN(keyword);
    ASSERT(comment);

    if (strncmp(comment,keyword,keyword_size)) 
	return FALSE;


    if (this->comment != NUL(char*))
    {
	int newsize = 2 + STRLEN(this->comment) + STRLEN(comment) 
					        - keyword_size;
	this->comment =
	    (char*)REALLOC(this->comment,newsize * sizeof(char));
	strcat(this->comment, "\n");
	strcat(this->comment, comment + keyword_size);
	c = this->comment;
    } else
        c = (char*)(comment + keyword_size);

    this->setPanelComment(c);

    return TRUE;
}

//
// Parse a control panel's 'title' comment.
//
boolean ControlPanel::cfgPrintTitleComment(FILE *f, PrintType)
{
    if (this->title) 
        return fprintf(f, "// title: value = %s\n", this->title) >= 0;
    else 
	return TRUE; 
}
//
// Parse a control panel's 'title' comment.
//
boolean ControlPanel::cfgParseTitleComment(const char *comment,
                                const char *filename, int lineno)
{

    int       items_parsed;
    char      title[512];


    if (strncmp(comment," title",6)) 
	return FALSE;

    items_parsed = sscanf(comment, " title: value = %[^\n]", title);

    /*
     * If not parsed correctly, error.
     */
    if (items_parsed != 1)
    {
	ErrorMessage("Bad 'title' panel comment (file %s, line %d)",
				filename, lineno);
 	return FALSE;
    } 
    this->setWindowTitle(title);
    return TRUE;
}

//
// Add/remove an instance of an interactor.
//
boolean ControlPanel::addInteractorInstance(InteractorInstance *ii)
{ 
    if (!this->instanceList.appendElement((void*)ii))
	return FALSE;

    ii->setPanel(this);
 
    this->setActivationOfEditingCommands();

    return TRUE;
}

boolean ControlPanel::removeInteractorInstance(InteractorInstance *ii)
{ 
    boolean r = this->instanceList.removeElement((void*)ii); 

    this->setActivationOfEditingCommands();
    return r;
}

const char* ControlPanel::getInteractorLabel()
{
     InteractorInstance *ii;
     WorkSpaceComponent *dd;
     boolean r = this->findTheSelectedInteractorInstance(&ii);
     boolean l = this->findTheSelectedComponent(&dd);

     if ((r)&&(!l))
            return ii->getInteractorLabel();
     else if ((l)&&(!r))
            return dd->getLabel();
     else
            return NULL;
}


//
// Determine the number of selected Interactors 
//
int ControlPanel::getInteractorSelectionCount()
{
    int         selected = 0;
    ListIterator iterator(this->instanceList);
    InteractorInstance *ii; 

    //
    // Look through the list of interactor instances for ones that are
    // currently selected. 
    //
    while ( (ii = (InteractorInstance*)iterator.getNext()) ) {
	if (ii->isSelected()) 
	   selected++;
    }
    return selected;
}

int ControlPanel::getComponentSelectionCount()
{
    int         selected = 0;
    ListIterator iterator(this->componentList);
    WorkSpaceComponent *wsc; 

    //
    // Look through the list of interactor instances for ones that are
    // currently selected. 
    //
    while ( (wsc = (WorkSpaceComponent*)iterator.getNext()) ) {
	if (wsc->isSelected()) 
	   selected++;
    }
    return selected;
}

//
// De/Activate any commands that have to do with editing.
// Should be called when an Interactor is added or deleted or there is a 
// selection status change.
//
void ControlPanel::setActivationOfEditingCommands()
{
    int nselected, wscselected;
    int interactors = this->instanceList.getSize();
    int components  = this->componentList.getSize();


    //
    // Handle commands that depend on whether there are Interactors present 
    //
    if (interactors == 0) {
	nselected = 0;
    } else {
	nselected = this->getInteractorSelectionCount();
    }
    if (components == 0) {
	wscselected = 0;
    } else {
	wscselected = this->getComponentSelectionCount();
    }

    //
    // Handle commands that depend on whether there are Interactors selected
    //
    if ((nselected == 0)&&(wscselected == 0)) {
	if (this->setSelectedInteractorAttributesCmd)
	    this->setSelectedInteractorAttributesCmd->deactivate();
	if (this->setSelectedInteractorLabelCmd)
	    this->setSelectedInteractorLabelCmd->deactivate();
	if (this->deleteSelectedInteractorsCmd)
	    this->deleteSelectedInteractorsCmd->deactivate();
	if (this->showSelectedStandInsCmd)
	    this->showSelectedStandInsCmd->deactivate();
	if (this->setLayoutButton)
	    XtSetSensitive(this->setLayoutButton, FALSE);
    } else if (nselected==0) {
	WorkSpaceComponent *wsc;
	if (wscselected == 1) {
	    boolean hasWindow = FALSE;
	    this->findTheSelectedComponent(&wsc);
	    ASSERT(wsc);
	    if (wsc->isA(ClassDecorator)) {
		Decorator *dd = (Decorator*)wsc;
		hasWindow = dd->hasDefaultWindow();
	    }
	    if (this->setSelectedInteractorAttributesCmd) {
		if (hasWindow)
		    this->setSelectedInteractorAttributesCmd->activate();
		else
		    this->setSelectedInteractorAttributesCmd->deactivate();
	    }
	} else {
	    if (this->setSelectedInteractorAttributesCmd)
		this->setSelectedInteractorAttributesCmd->deactivate();
	}

	if (this->setSelectedInteractorLabelCmd) 
	    this->setSelectedInteractorLabelCmd->deactivate();
	if (this->deleteSelectedInteractorsCmd)
	    this->deleteSelectedInteractorsCmd->activate();
	if (this->showSelectedStandInsCmd)
	    this->showSelectedStandInsCmd->deactivate();

	// 
	// If there is any thing selected which can't accept a layout change then
	// disable the layout button.  If there are multiple things selected and
	// their layouts aren't all equal, then disable the layout button.
	// If the layout button is enabled, then enable only the vertical or the
	// horizontal button and base the decision on the layout of things currently
	// selected.
	// 
	boolean disableLayout = FALSE;
	boolean layouts = TRUE, first = TRUE;
	ListIterator it(this->componentList);
	while ((!disableLayout)&&(wsc = (WorkSpaceComponent *)it.getNext())) {
	    if (wsc->isSelected()) {
		if (!wsc->acceptsLayoutChanges())
		    disableLayout = TRUE;
		else {
		    if (first) {
			layouts = wsc->verticallyLaidOut();
		    } else if (layouts != wsc->verticallyLaidOut()) {
			disableLayout = TRUE;
		    }
		}
	    }
	}
	if (this->setLayoutButton) {
	    if (disableLayout) {
		XtSetSensitive (this->setLayoutButton, False);
		this->verticalLayoutCmd->deactivate();
		this->horizontalLayoutCmd->deactivate();
	    } else {
		XtSetSensitive (this->setLayoutButton, True);
		if (layouts) {
		    this->verticalLayoutCmd->deactivate();
		    this->horizontalLayoutCmd->activate();
		} else {
		    this->verticalLayoutCmd->activate();
		    this->horizontalLayoutCmd->deactivate();
		}
	    }
	}
    } else {
	//
	// Commands that depend on only one interactor being selected
	//
        if ((nselected == 1)&&(!wscselected)) {
	    if (this->setSelectedInteractorAttributesCmd) {
	    	InteractorInstance *ii;
		this->findTheSelectedInteractorInstance(&ii);
		ASSERT(ii);
	        if (ii->hasSetAttrDialog())
		    this->setSelectedInteractorAttributesCmd->activate();
		else
		    this->setSelectedInteractorAttributesCmd->deactivate();
 	    }
	    if (this->setSelectedInteractorLabelCmd)
		this->setSelectedInteractorLabelCmd->activate();
	    //
	    // Only enable the show StandIns option if there is an editor. 
	    //
	    if (this->getNetwork()->getEditor() && 
		this->showSelectedStandInsCmd)
	        this->showSelectedStandInsCmd->activate();
	} else {	// More than one selected interactor
	    if (this->setSelectedInteractorAttributesCmd)
		this->setSelectedInteractorAttributesCmd->deactivate();
	    if (this->setSelectedInteractorLabelCmd)
		this->setSelectedInteractorLabelCmd->deactivate();
	    if (this->showSelectedStandInsCmd)
		this->showSelectedStandInsCmd->deactivate();
	}
	//
	// Commands that depend on at least one interactor being selected
	//
	if (this->deleteSelectedInteractorsCmd)
	    this->deleteSelectedInteractorsCmd->activate();
	// 
	// If there is any thing selected which can't accept a layout change then
	// disable the layout button.  If there are multiple things selected and
	// their layouts aren't all equal, then disable the layout button.
	// If the layout button is enabled, then enable only the vertical or the
	// horizontal button and base the decision on the layout of things currently
	// selected.
	// 
	boolean disableLayout = FALSE;
	boolean layouts = TRUE, first = TRUE;
	ListIterator it(this->componentList);
	WorkSpaceComponent *wsc;
	while ((!disableLayout)&&(wsc = (WorkSpaceComponent *)it.getNext())) {
	    if (wsc->isSelected()) {
		if (!wsc->acceptsLayoutChanges())
		    disableLayout = TRUE;
		else {
		    if (first) {
			layouts = wsc->verticallyLaidOut();
			first = FALSE;
		    } else if (layouts != wsc->verticallyLaidOut()) {
			disableLayout = TRUE;
		    }
		}
	    }
	}
	it.setList(this->instanceList);
	InteractorInstance *ii;
	while ((!disableLayout)&&(ii = (InteractorInstance*)it.getNext())) {
	    if (ii->isSelected()) {
		Interactor *ntr = ii->getInteractor();
		if (!ntr->acceptsLayoutChanges())
		    disableLayout = TRUE;
		else {
		    if (first) {
			layouts = ntr->verticallyLaidOut();
			first = FALSE;
		    } else if (layouts != ntr->verticallyLaidOut()) {
			disableLayout = TRUE;
		    }
		}
	    }
	}
	if (this->setLayoutButton) {
	    if (disableLayout) {
		XtSetSensitive (this->setLayoutButton, False);
		this->verticalLayoutCmd->deactivate();
		this->horizontalLayoutCmd->deactivate();
	    } else {
		XtSetSensitive (this->setLayoutButton, True);
		if (layouts) {
		    this->verticalLayoutCmd->deactivate();
		    this->horizontalLayoutCmd->activate();
		} else {
		    this->verticalLayoutCmd->activate();
		    this->horizontalLayoutCmd->deactivate();
		}
	    }
	}
    }

    //
    // Enable/Disable any commands that depend on whether there are Nodes
    // that are currently selected.
    //
    EditorWindow *editor = this->network->getEditor();
    if (editor && 
	(editor->getNodeSelectionCount(ClassInteractorNode) != 0)) {
	if (this->addSelectedInteractorsCmd)
	    this->addSelectedInteractorsCmd->activate();
	if (this->showSelectedInteractorsCmd)
	    this->showSelectedInteractorsCmd->activate();
    } else {
	this->concludeInteractorPlacement();
	if (this->addSelectedInteractorsCmd)
	    this->addSelectedInteractorsCmd->deactivate();
	if (this->showSelectedInteractorsCmd)
	    this->showSelectedInteractorsCmd->deactivate();
    }
}

//
// Here is a function with 2 return values.
// return True if exactly 1 decorator is selected... False otherwise.
// set **selected to 0 if no decorator is selected, set it to some
// selected decorator is 1 or more decorators are selected.
//
boolean ControlPanel::findTheSelectedComponent(WorkSpaceComponent **selected)
{
    ListIterator iterator(this->componentList);
    WorkSpaceComponent *d; 
    int cnt;

    *selected = NULL;
    //
    // Look through the list of decorators for ones that are
    // currently selected. 
    //
    cnt = 0;
    while (cnt<2 && (d = (WorkSpaceComponent*)iterator.getNext())) {
	if (d->isSelected()) {
	    if (!cnt) *selected = d;
	    cnt++;
	}
    }
    return (cnt==1);
}

//
// Find THE selected InteractorInstance.
// Return TRUE if a single interactor is selected 
// Return FALSE if none or more than one selected interactor were found 
// Always set *selected to the first selected InteractorInstance that was
// found.  If FALSE is return *selected may be set to NULL if no selected
// InteractorInstances were found. 
//
boolean ControlPanel::findTheSelectedInteractorInstance(
				InteractorInstance **selected)
{
    ListIterator iterator(this->instanceList);
    InteractorInstance *ii; 
    boolean found_multi;

    *selected = NULL;
    //
    // Look through the list of interactor instances for ones that are
    // currently selected. 
    //
    found_multi = FALSE;
    while (!found_multi && (ii = (InteractorInstance*)iterator.getNext())) {
	if (ii->isSelected()) {
	    if (*selected == NULL)
	        *selected = ii;
	    else
		found_multi = TRUE;
	}
    }
    return !found_multi && (*selected != NULL);
}
//
// Change the layout of all selected interactors. 
//
void ControlPanel::setVerticalLayout(boolean vertical)
{
    ListIterator iterator(this->instanceList);
    InteractorInstance *ii;

    //
    // Change the layout of all selected interactors.
    //
    while ( (ii = (InteractorInstance*)iterator.getNext()) ) {
        if (ii->isSelected()) 
	    ii->setVerticalLayout(vertical);
    }

    //
    // Change the layout of all selected decorators.
    //
    WorkSpaceComponent *wsc;
    iterator.setList (this->componentList);
    while ( (wsc = (WorkSpaceComponent *)iterator.getNext()) ) {
	if (wsc->isSelected()) {
	    wsc->setVerticalLayout(vertical);
	}
    }
    this->setActivationOfEditingCommands();
}
//
// Perform the default double click action for all selected interactor
// instances.  Interactors are expected to mark their instances as selected.
//
void ControlPanel::doDefaultWorkSpaceAction()
{
    InteractorInstance *iSelected;
    WorkSpaceComponent *dSelected;
    boolean oneInteractor, oneComponent;

    if (theDXApplication->appAllowsInteractorAttributeChange()) {
	oneInteractor = this->findTheSelectedInteractorInstance(&iSelected);
	oneComponent = this->findTheSelectedComponent(&dSelected);
	if ((oneInteractor)&&(oneComponent))
	    ErrorMessage("Only one type of selection allowed");
	else if ((!oneInteractor)&&(iSelected))
	    ErrorMessage("Only one selected interactor allowed");
	else if ((!oneComponent)&&(dSelected))
	    ErrorMessage("Only one selected decorator allowed");
	else if ((oneInteractor)&&(iSelected))
	    iSelected->getInteractor()->openDefaultWindow();
	else if ((oneComponent)&&(dSelected)) {
	    dSelected->openDefaultWindow();
	}
    }
}

//
// Open the set attributes dialog box of THE selected interactor. 
//
void ControlPanel::openSelectedSetAttrDialog()
{
    InteractorInstance *selected;
    WorkSpaceComponent *dd;
    boolean r = this->findTheSelectedInteractorInstance(&selected);
    boolean l = this->findTheSelectedComponent(&dd);

    if (r&&l)
	ErrorMessage("Only one selection allowed");
    else if ((!r)&&(selected))
	ErrorMessage("Only one selected interactor allowed");
    else if (r)
        selected->openSetAttrDialog();
    else if ((!l)&&(dd))
	ErrorMessage("Only one selected decorator allowed");
    else if (l)
	dd->openDefaultWindow();
    else { /* nothing was selected */ }
}

//
// Do any work that is required when an Interactors state has changed. 
//
void ControlPanel::handleInteractorInstanceStatusChange(InteractorInstance *, 
						InteractorStatusChange status)
{
    switch (status) {
	case Interactor::InteractorSelected:
	case Interactor::InteractorDeselected:
	    this->setActivationOfEditingCommands();
	    break;
	default:
	    ASSERT(0);
    }

}
//
// Do any work that is required when a Node's state has changed. 
// When a node is selected, we remember it and when it is unselected
// it is unremembered.  
// If the number of remembered nodes is 0, disable the 
// 'Add Selected Interactors' command, otherwise enable it.
//
void ControlPanel::handleNodeStatusChange(Node *node, NodeStatusChange status)
{
    switch (status) {
	case Node::NodeSelected:
	    break;
	case Node::NodeDeselected:
	    if (this->addingNodes)
		this->addingNodes->removeElement((const void*)node);
	    break;
	default:
	    ASSERT(0);
    }

    this->setActivationOfEditingCommands();
}
//
//  Deselect all nodes in the VPE, and then highlight each 
//  InteractorNode that has a selected InteractorInstance in this 
//  control panel.
//  Called when this->showSelectedStandInsCmd is used.
//
void ControlPanel::showSelectedStandIns()
{
    EditorWindow *editor = this->getNetwork()->getEditor(); 

    //
    // Control Panels don't get notified when the editor goes away so if
    // there is no editor just return after setting the edit command 
    // activations.
    //
    if (!editor)  {
	this->setActivationOfEditingCommands();
	return;
    }

    //
    // Deselect all the nodes in the VPE
    //
    editor->deselectAllNodes();
    
    //
    // Look through all selected InteractorInstances.
    //
    ListIterator instanceIterator(this->instanceList);
    InteractorInstance *ii;
    boolean first = TRUE;
    while ( (ii = (InteractorInstance*) instanceIterator.getNext()) )  {
	if (ii->isSelected())  {
	    Node *n = ii->getNode();
	    editor->selectNode(n,first);
	    first = FALSE;
	}
    }
}
// 
//  Highlight each InteractorInstances that are in this control panel
//  and have their corresponding Node selected.
//  Called when this->showSelectedInteractorsCmd is used.
//
void ControlPanel::showSelectedInteractors()
{
    InteractorNode *inode;
    InteractorInstance *ii;
    ListIterator instanceIterator(this->instanceList);
     

    //
    // Deselect all other interactors in this panel. 
    //
    while ( (ii = (InteractorInstance*) instanceIterator.getNext()) ) {
	ii->setSelected(FALSE);
    }

    //
    // Look through all selected nodes.
    //
    EditorWindow *editor = this->network->getEditor();
    ASSERT(editor);
    List *selected = editor->makeSelectedNodeList(ClassInteractorNode);
    if (!selected)
	return;
    ListIterator nodeIterator(*selected);
    while ( (inode = (InteractorNode*) nodeIterator.getNext()) ) {
	ASSERT(((Node*)inode)->isA(ClassInteractorNode));
        ListIterator instanceIterator(inode->instanceList);
	//
	// Look at each InteractorInstance of the current node.
	//
        while ( (ii = (InteractorInstance*) instanceIterator.getNext()) ) {
	    //
	    // If the instance belongs to this control panel then highlight it.
	    //
	    if (ii->getControlPanel() == this)
	        ii->setSelected(TRUE);	
	}
    } 
    delete selected;
}

void ControlPanel::clearInstances()
{
    InteractorInstance *ii;
    InteractorNode *inode;

    while( (ii = (InteractorInstance*)this->instanceList.getElement(1)) )
    {
    	inode = (InteractorNode*)ii->getNode();	
    	inode->deleteInstance(ii);
    }

}


//
// Move all InteractorInstances from cp into this.  When finished, cp
// should contain no instances.  The instances in cp think they belong
// to a node in the temporary net and must be switched to the new net.
//
void ControlPanel::mergePanels (ControlPanel *cp, int x, int y)
{
   ListIterator iterator(cp->instanceList);
   InteractorInstance *ii;
   List toDelete;
   int firstx, firsty, newx, newy, offx, offy;
   Boolean first = TRUE;

    while ( (ii = (InteractorInstance*)iterator.getNext()) ) {

	// uncreate the interactor?  During dnd, if in image mode, then
	// Network::readNetwork() would have managed the tmppanel and created the
	// interactor.  We must not keep that interactor, only the instance.
	if (ii->getInteractor()) ii->uncreateInteractor();

	toDelete.appendElement(ii);
 	if (ii->switchNets (this->network)) {
	    this->addInteractorInstance(ii);
	    // should ASSERT that the panel is on the screen ?
	    ii->setSelected(FALSE);
	    if (first) {
		ii->getXYPosition (&firstx, &firsty);
		newx = x; newy = y;
		first = FALSE;
	    } else {
		ii->getXYPosition (&offx, &offy);
		newx = x + (offx - firstx); newy = y + (offy - firsty);
		newx = (newx<0?0:newx); newy = (newy<0?0:newy);
	    }
	    ii->setXYPosition (newx,newy);
	    ii->createInteractor();
	} else {
	    // What to do in case of error?
	    ASSERT(0);
	}
    }

    iterator.setList(toDelete);
    while ( (ii = (InteractorInstance*)iterator.getNext()) ) {
	cp->removeInteractorInstance (ii);
    }
    toDelete.clear();

    // Now move the decorators
    iterator.setList (cp->componentList);
    Decorator *dd;
    while ( (dd = (Decorator*)iterator.getNext()) ) {
	toDelete.appendElement((void*)dd);
	this->addComponentToList((void*)dd);
	dd->setAppearance (this->isDeveloperStyle());
	DecoratorInfo* dnd = new DecoratorInfo (this->network, (void*)this, 
	    (DragInfoFuncPtr)ControlPanel::SetOwner,
	    (DragInfoFuncPtr)ControlPanel::DeleteSelections);
	dd->setDecoratorInfo (dnd);

	if (first) {
	    dd->getXYPosition (&firstx, &firsty);
	    newx = x; newy = y;
	    first = FALSE;
	} else {
	    dd->getXYPosition (&offx, &offy);
	    newx = x + (offx - firstx); newy = y + (offy - firsty);
	    newx = (newx<0?0:newx); newy = (newy<0?0:newy);
	}
	dd->setXYPosition (newx, newy);
	dd->manage(this->workSpace);
    }
    iterator.setList(toDelete);
    while ( (dd = (Decorator*)iterator.getNext())) {
	cp->componentList.removeElement((void *)dd);
    }
    toDelete.clear();
}

//
// Delete all selected InteractorInstances from the control panel.
//
void ControlPanel::deleteSelectedInteractors()
{
   ListIterator iterator(this->instanceList);
   InteractorNode *inode;
   InteractorInstance *ii;
   List	toDelete;
   WorkSpaceComponent *dd;

    //
    // This loop constructs a list of nodes to delete
    // because the inode->deleteInstance() function removes
    // the node from the list.  This causes problems with
    // the list iterator pointing to released memory.
    //
    while ( (ii = (InteractorInstance*)iterator.getNext()) ) {
	if (ii->isSelected()) 
	    toDelete.appendElement((void*)ii);
    }


    // 
    //  Now delete the instances.
    //
    iterator.setList(toDelete);
    while ( (ii = (InteractorInstance*)iterator.getNext()) ) {
	    inode = (InteractorNode*)ii->getNode();	
	    inode->deleteInstance(ii);
    }

    // 
    //  Now gather up the decorators.
    //
    iterator.setList(this->componentList);
    while ( (dd = (WorkSpaceComponent *)iterator.getNext()) ) {
	if (dd->isSelected()) {
	    this->componentList.removeElement((void*)dd);
	    delete dd;
	}
    }

    this->setActivationOfEditingCommands();
    this->getNetwork()->setFileDirty();
}
//
// Set the label of THE selected interactor.
// If there is more than one interactor selected then we pop up an
// error message.
//
void ControlPanel::setSelectedInteractorLabel()
{
     if (!this->setLabelDialog)
     	this->setLabelDialog = new SetInteractorNameDialog(this);
     this->setLabelDialog->post();

}

void ControlPanel::setInteractorLabel(const char *label)
{
    InteractorInstance *ii;
    boolean r = this->findTheSelectedInteractorInstance(&ii);
    ASSERT(r);
    ii->setLocalLabel(label);

    // because if the label is NULL, want to disable set layout menu option
    this->setActivationOfEditingCommands();
}

//
// Edit the comment for this panel.
//
void ControlPanel::editPanelComment()
{
     if (!this->setCommentDialog)
     	this->setCommentDialog = new SetPanelCommentDialog(
					this->getRootWidget(), FALSE, this);
     this->setCommentDialog->post();

}
//
// View the comment for this panel.
//
void ControlPanel::postHelpOnPanel()
{
     if (!this->helpOnPanelDialog)
     	this->helpOnPanelDialog = new HelpOnPanelDialog(this->getRootWidget(), 
							this);
     this->helpOnPanelDialog->post();
}

//
// Edit the name of this panel.
//
void ControlPanel::editPanelName()
{
     if (!this->setNameDialog)
     	this->setNameDialog = new SetPanelNameDialog(
					this->getRootWidget(), this);
     this->setNameDialog->post();

}

//
// Change the work space grid setup
//
void ControlPanel::setPanelGrid()
{
     if (NOT this->gridDialog )
     {
        this->gridDialog = new GridDialog(this->getRootWidget(),
                                          (WorkSpace*)this->getWorkSpace());
     }
     this->gridDialog->post();

     //
     // This isn't quite correct, but at least assures that the user won't
     // accidentally not save the .net/.cfg after changing the grid. 
     // We do this here becase GridDialog does know about panels and networks.
     //
     this->getNetwork()->setFileDirty();

}

//
// Set a particular panel style
// To enter dialog style, you must tell the workspace widget and the objects living
// inside it.  The workspace widget has a resource names XmNautoArrange(boolean).
// When changed to true it tells the workspace widget to act like an XmForm*.
// Form attachments are calculated inside the widget automatically**, however if you
// choose to make them yourself - either thru code or default resource settings or
// editres - then your settings will be honored by the widget.  The objects living
// inside the workspace have a method for setting the style.
//
// *How does a workspace widget act like an XmForm?
// The workspace widget subclasses off XmForm, so it doesn't just act like an XmForm,
// it _is_ an XmForm.  While its XmNautoArrange resource is set to False, it doesn't
// call the basic widget methods of XmForm and it forces attachments to XmATTACH_NONE.
// Therefore the code in XmForm doesn't have the opportunity to move children arround
// or maintain their dimensions.  When it wants to act like an XmForm, it just stands
// aside and lets its XmForm self do the work.  The widget contains no special code
// to do geometry.  So anything you can do with an XmForm widget, you can do with
// a workspace widget.
//
// **How does a workspace widget set attachments automatically?
// First, remember that the workspace widget won't set attachments if you supply
// values yourself***.
// When changing XmNautoArrange to True, the workspace widget observes x,y,width,height
// of each child and sets {top,left}Position of every child.  {top,left}Attachment
// will always be set to XmATTACH_POSITION.  The {top,left}Position values are 
// calculated based on the x,y location of the child relative to the size of the
// workspace widget.  So if its 20 pixels from the top a a 200 pixel tall workspace
// widget, then it will remain 10% of the way from the top following a resize of the
// window.  That's just normal XmForm stuff.  I also added 2 extra constraint resources
// to control what workspace would do with {right,bottom}Attachment.  By default, they
// are left set to XmATTACH_NONE.  Setting XmNpinLeftRight to True means that the
// workspace widget will also set rightAttachment and rightPosition using the same
// logic used on the left.  Similar for XmNpinTopBottom.  These are useful for 
// children which really should always be resized like anything based on a scrolled 
// list.  Caveat:  Setting pinLeftRight,pinTopBottom to True also means that if the
// widget is within a few pixels of the edge of the workspace, that the workspace will
// set the corresponding attachment resource to XmATTACH_FORM instead of 
// XmATTACH_POSITION.  This is important mainly for separator decorators which really
// need to span the window.
//
// ***How do I supply my own attachments for children of a workspace widget.
// The workspace widget will force attachments to XmATTACH_NONE while autoArrange is
// False.  I created a shadowing set of contraints with similar names:
// XmNleftAttachment becomes XmNwwLeftAttachment and so forth.  So in default resources
// you can use *wwLeftAttachment: XmATTACH_FORM or XtSetArg if you want.  Then when
// the workspace enters dialog style, the contents of wwLeftAttachment which defaults
// to XmATTACH_NONE, will be used instead of an automatic calculation.  Once in dialog
// style you're free to adjust XmNleftAttachment or whatever resource in anyway you
// want, just as with any other XmForm widget.  N.B.  If you're in dialog style, and
// you create a new child of the workspace - and this does not 
// currently happen anywhere - then you would have to set XmNleftAttachment and so
// forth yourself, because the automatic calculation of XmN{left,right}Position
// resource values, happens only when you change XmNautoArrange from False to True.
//
// -Martin 10/9/95 (dx 3.1)
//
void ControlPanel::setPanelStyle(boolean developerStyle)
{
Widget ws = this->getWorkSpaceWidget();
Widget diag = this->getRootWidget();
ControlPanelWorkSpace *wsObj = this->getWorkSpace();
boolean isWsManaged;

    ASSERT (XtIsShell(diag));
    ASSERT(ws);

    //
    // Abort if there isn't going to be a style change.  Set the file dirty
    // if there is a style change.
    //
    this->developerStyle = developerStyle;
    if (this->displayedStyle != NeverSet) {
	if ((this->developerStyle) && (this->displayedStyle == Developer))
	    return ;
	if ((!this->developerStyle) && (this->displayedStyle == User))
	    return ;
	this->getNetwork()->setFileDirty();
    }

    isWsManaged = XtIsManaged (ws);
    if (isWsManaged) wsObj->unmanage();

    //
    // Change the appearance of all the interactors
    //
    InteractorInstance *ii; 
    Interactor *ntr;
    ListIterator iterator(this->instanceList);
    while ( (ii = (InteractorInstance*)iterator.getNext()) ) {
	ii->setSelected(FALSE);
	ntr = ii->getInteractor();
	ntr->setAppearance(this->developerStyle);
    }

    //
    // Change the appearance of all the decorators
    //
    WorkSpaceComponent *d;
    iterator.setList(this->componentList);
    while ( (d = (WorkSpaceComponent*)iterator.getNext()) ) {
	d->setSelected(FALSE);
	d->setAppearance(this->developerStyle);
    }

    //
    // (Un)Manage the menubar and the buttons at the bottom.
    //
    Widget w = this->getCommandArea();
    if (this->developerStyle) 
    {
	XtManageChild (this->getMenuBar());
	if (w) XtUnmanageChild (w);
    } 
    else 
    {
	XtUnmanageChild (this->getMenuBar());
	if (w) XtManageChild (w);
    }


    //
    // Turn off dnd in the ControlPanelWorkSpace.
    // Don't need to turn off dnd in the Interactors because it is now
    // impossible to select an interactor.
    //
    wsObj->dropsPermitted(this->developerStyle);

    Widget hBar, vBar;
    if (!this->developerStyle) 
    {

	//
	// Turn off Gridding and record the state so that he
	// doesn't have to bring up GridDialog.
	//
	if (this->workSpaceInfo.isGridActive()) 
	{
	    this->develModeGridding = TRUE;
	    this->workSpaceInfo.setGridActive(FALSE);
	    this->workSpace->installInfo(NULL);
	}

	Dimension maxR, maxB;
	Dimension diffW, diffH;

	XtVaSetValues (this->scrolledWindow, 
	    XmNshadowThickness, 0, 
	    NULL);
	XtVaSetValues (this->scrolledWindowFrame, 
	    XmNmarginWidth, 0, 
	    XmNmarginHeight, 0, 
	    NULL);

	//
	// If I call XtUnmanage on the scrollbars of the scrolledwindow,
	// then no matter what happens, the user will not see them.
	//
	XtVaGetValues (this->scrolledWindow, 
	    XmNhorizontalScrollBar, &hBar,
	    XmNverticalScrollBar, &vBar, 
	    NULL);
	XtUnmanageChild (hBar); XtUnmanageChild (vBar);

	int gmw, gmh;
	XmWorkspaceGetMaxWidthHeight (ws, &gmw, &gmh);
	maxB = MAX(100,gmh);// Start with a reasonable smallest case size;
	maxR = MAX(200,gmw); 

	//
	// develModeDiag{Width,Height} will be used when returning to developer style
	//
	XtVaGetValues(diag, 
	    XmNwidth, &this->develModeDiagWidth, 
	    XmNheight, &this->develModeDiagHeight, 
	    NULL);

	//
	// Need to set dims for both Workspace widget and dialog box.  They're
	// similar but not the same.  Set the dialog to be the size of Workspace
	// plus whatever it takes for window borders, etc horizontally and the size
	// of Close,Help buttons vertically.  I obtained diffH from observation
	// using editres and flash active widget.
	//
	diffW = 2;
	diffH = 40;

	//
	// newW,newH will be the size of the panel.  The Motif widgets need to  
	// recalc their sizes becuase of (un)managing menubar/buttons, so 
	// don't allow new size == old size
	//
	Dimension newW = maxR+diffW;
	Dimension newH = maxB+diffH;
	boolean needTickle = 
	    ((newW==this->develModeDiagWidth)&&(newH==this->develModeDiagHeight)); 
	if (needTickle) newW++;

	//
	// There are side effects from the following XtSetValues.  The Workspace
	// widget turns into an XmForm because of the XmNautoArrange setting.
	// It uses the new width,height to calculate values for XmNattachPosition
	// for each of its children.  Because it's difficult to know how big the
	// Workspace widget will end up being, the Workspace widget on its own
	// will take the width,height specified in this call to XtSetValues and
	// use it for the calculations.  Make sure that space wars are turned off
	// *before* going into XmForm mode.  That requires 2 separate calls to
	// XtSetValues.
	//
	XtVaSetValues (ws, 
	    XmNallowOverlap, True, 
	    NULL);
	XtVaSetValues (ws, 
	    XmNwidth, maxR, 
	    XmNheight, maxB, 
	    XmNautoArrange, True, 
	    NULL);

	//
	// FIXME
	// When displaying on ibm6000 aix325, setting min{Width,Height} before
	// this->manage(), means that min{Width,Height} can never be unset.  When
	// 325 isn't supported||broken then remove some of this stuff and set
	// min{Width,Height} and {width,height} together all the time.
	//
	if (this->isManaged()) 
	{
	    XtVaSetValues (diag, 
		XmNminWidth, newW, 
		XmNminHeight, newH, 
		NULL);
#	if !defined(AIX312_MWM_IS_FIXED)
	    XSync (XtDisplay(diag), False);
#	endif
	    XtVaSetValues (diag, 
		XmNwidth, newW, 
		XmNheight, newH, 
		NULL);
	} 
	else 
	{
	    XtVaSetValues (diag, 
		XmNwidth, newW, 
		XmNheight, newH, 
		NULL);
	}

	wsObj->setSizeDeltas (diffW, diffH);

	this->displayedStyle = User;

	if (needTickle)
	    XtVaSetValues (diag, XmNwidth, newW-1, NULL);

    } 
    else 
    {
	ASSERT (this->develModeDiagWidth && this->develModeDiagHeight);
	//
	// Side effects here:  Setting autoArrange==False changes the XmForm back
	// into a Workspace.  Make sure that allowOverlap is set to False *after*
	// and not at the same time.  Otherwise you can get space wars during the
	// conversion process.
	//
	XtVaSetValues(ws, XmNautoArrange, False, NULL);
	if (!this->allowOverlap)
	    XtVaSetValues(ws, XmNallowOverlap, False, NULL);


	XtVaGetValues (this->scrolledWindow, 
	    XmNhorizontalScrollBar, &hBar,
	    XmNverticalScrollBar, &vBar, 
	    NULL);
	XtManageChild (hBar); XtManageChild (vBar);

	XtVaSetValues (this->scrolledWindow, 
	    XmNshadowThickness, 1, 
	    NULL);
	XtVaSetValues (this->scrolledWindowFrame, 
	    XmNmarginWidth, SCRWF_MARGIN, 
	    XmNmarginHeight, SCRWF_MARGIN, 
	    NULL);

	//
	// The Motif MainWindow widget wants a chance to recalc sizes, so
	// don't allow old size == new size.  The check for greater-than is because
	// incrementing width by 1 does no good if it's still less than minWidth
	// on an rs6000 where unsetting minWidth is unreliable.
	//
	Dimension oldW, oldH;
	XtVaGetValues (diag, XmNwidth, &oldW, XmNheight, &oldH, NULL);
	if ((oldW >= this->develModeDiagWidth)&&
	    (oldH >= this->develModeDiagHeight))  this->develModeDiagWidth = oldW+1;

	XtVaSetValues (diag, 
	    XmNminWidth, XtUnspecifiedShellInt,
	    XmNminHeight, XtUnspecifiedShellInt,
	    NULL);
#ifndef AIX312_MWM_IS_FIXED
	XSync(XtDisplay(diag), False);
#endif
	XtVaSetValues (diag, 
	    XmNwidth, this->develModeDiagWidth, 
	    XmNheight, this->develModeDiagHeight, 
	    NULL);
	this->develModeDiagWidth = this->develModeDiagHeight = 0;

	this->displayedStyle = Developer;

	//
	// Turn Gridding back on if it was in use before the switch.
	//
	if (this->develModeGridding) 
	{
	    this->develModeGridding = FALSE;
	    this->workSpaceInfo.setGridActive(TRUE);
	    this->workSpace->installInfo(NULL);
	}
    }
    if (isWsManaged) wsObj->manage();

    this->getNetwork()->setFileDirty();
}

//
// These 2 callbacks {ControlPanel_DialogHelpCB and ControlPanel_DialogCancelCB} are
// used by the 2 buttons at the bottom of the control panel when in user style.
//
extern "C" { 
void ControlPanel_DialogHelpCB (Widget w, XtPointer clientdata, XtPointer )
{
ControlPanel *cp = (ControlPanel *)clientdata;
    cp->postHelpOnPanel();
}

void ControlPanel_DialogCancelCB (Widget , XtPointer clientdata, XtPointer calldata)
{
ControlPanel *cp = (ControlPanel *)clientdata;
XmPushButtonCallbackStruct *pbcbs = (XmPushButtonCallbackStruct*)calldata;
XEvent *xev = pbcbs->event;
boolean mustAsk = TRUE;
boolean mustRevert = FALSE;

    // The button can be pressed only when this->developerStyle == FALSE
#if 00
    if (theDXApplication->inEditMode()) {
#else
    if (cp->allowDeveloperStyleChange()) {
#endif
	if ((xev->type == KeyPress) || (xev->type == KeyRelease)) {
	    mustAsk = FALSE;
	    mustRevert = FALSE;
	} else if ((xev->type == ButtonPress) || (xev->type == ButtonRelease)) {
	    XButtonEvent *xbe = (XButtonEvent*)xev;
	    if (xbe->state & ShiftMask) {
		mustRevert = TRUE;
		mustAsk = FALSE;
	    } else if (xbe->state & ControlMask) {
		mustRevert = FALSE;
		mustAsk = FALSE;
	    } else {
		mustRevert = FALSE;
		mustAsk = TRUE;
	    }
	}

	if (mustRevert) {
	    cp->setPanelStyle(TRUE);
	} else if (mustAsk) {
	    char *message = 
	    "Revert to developer style?\n\n"
//	    "(Shift+Close direct to dev. style)\n"
	    "(Ctrl+Close avoids this dialog and closes the control panel.\n"
	    "This dialog only appears when the VPE is the anchor window.)";
	    theQuestionDialogManager->modalPost (
		cp->getRootWidget(), message, "Confirm Style Change", (void*)cp,
		(DialogCallback)ControlPanel::ToDevStyle, 
		(DialogCallback)ControlPanel::Unmanage, 
		NULL,
		"Yes", "No", NULL, 
		XmDIALOG_PRIMARY_APPLICATION_MODAL
	    );
	} else {
	    cp->unmanage();
	}
    } else {
	cp->unmanage();
    }
}
} // extern "C"

void ControlPanel::ToDevStyle (XtPointer clientdata)
{
ControlPanel *cp = (ControlPanel *)clientdata;
    cp->setPanelStyle (TRUE);
}

void ControlPanel::Unmanage (XtPointer clientdata)
{
ControlPanel *cp = (ControlPanel *)clientdata;
    cp->unmanage ();
}


//
// Initialize for selected interactor node placement.  Should be called
// when the 'Add Selected Interactors' command is issued.
//
// FIXME: we could install the callback here and remove it in the callback
// when the there or no more interactors to place.
//
void ControlPanel::initiateInteractorPlacement()
{
    EditorWindow *editor = this->network->getEditor();
    ASSERT(editor);
    List *selected = editor->makeSelectedNodeList(ClassInteractorNode);

    ASSERT(selected);
    int selects = selected->getSize();
    ASSERT(selects > 0);

    // 
    // Set the cursor to indicate placement 
    // 
    ASSERT(this->workSpace);
    this->workSpace->setCursor(UPPER_LEFT);

    // 
    // Copy the items from the selected InteractorNode list to 
    // the addingNodes list.
    // 
    if (!this->addingNodes)
	this->addingNodes = new List;
    else
	this->addingNodes->clear();

    const void *v;
    ListIterator i(*selected);
    while ( (v = i.getNext()) )
	this->addingNodes->appendElement(v);

    delete selected;
}

// Does almost the same things as initiateInteractorPlacement(), but since
// it's used by a dnd operation, it must not change the cursor. harumph.
// Be less restrictive about the contents of "selected" because we can't rely
// on command activation in the vpe.
boolean ControlPanel::dndPlacement(int x, int y)
{
Node *node;
int prev_width, prev_height, placex, placey, max_right_edge, vert_cnt;

    EditorWindow *editor = this->network->getEditor();
    ASSERT(editor);
    List *selected = editor->makeSelectedNodeList(ClassInteractorNode);

    if (!selected) return FALSE;
    int selects = selected->getSize();
    if (selects<=0) return FALSE;


    // get a count of selected interactor nodes.  Proceed only if the count
    // is nonzero.  This helps us do things the same as other placement routines
    // and share code.  If there is no selected interactor node then we should
    // return FALSE in order to reject the drop.
    ListIterator i(*selected);
    selects = 0;
    while ( (node = (Node*)i.getNext()) ) {
	if (node->isA(ClassInteractorNode) == FALSE) continue;
	selects++;
    }
    if (selects==0) return FALSE;

    // Deselect all other interactors and decorators.
    ListIterator it(this->instanceList);
    InteractorInstance *ii;
    while ( (ii = (InteractorInstance*)it.getNext()) ) 
	ii->setSelected(FALSE);
    it.setList(this->componentList);
    WorkSpaceComponent *wsc;
    while ( (wsc = (WorkSpaceComponent *)it.getNext()) )
	wsc->setSelected(FALSE);


    i.setList(*selected);
    placex = x; placey = y;
    prev_height = prev_width = 0;
    max_right_edge = vert_cnt = 0;
    while ( (node = (Node*)i.getNext()) ) {
	// If it's not an InteractorNode then skip over it.
	ASSERT(node);

	if (node->isA(ClassInteractorNode) == FALSE) continue;

	placey+= prev_height;
	ii = this->createAndPlaceInteractor((InteractorNode*)node, placex, placey);
	ii->getXYSize (&prev_width, &prev_height); prev_height+= 5;
	max_right_edge = MAX(max_right_edge, (placex+prev_width));

	vert_cnt++;
	if (vert_cnt >= 5) {
	    placex = max_right_edge + 5;
	    vert_cnt = 0;
	    placey = y;
	    prev_height = 0;
	}
    }

    delete selected;

    selected = editor->makeSelectedDecoratorList (); 
    if (selected) {
	i.setList(*selected);
	Decorator *dec;
	Symbol s = theSymbolManager->registerSymbol(ClassLabelDecorator);

	while ( (dec = (Decorator*)i.getNext()) ) {
	    if (dec->isA(s) == FALSE) continue;

	    placey+= prev_height;
	    LabelDecorator *lab = (LabelDecorator*)
		this->createAndPlaceDecorator ("Label", placex, placey);
	    lab->setLabel(((LabelDecorator*)dec)->getLabelValue());
	    lab->manage(this->workSpace);

	    lab->getXYSize (&prev_width, &prev_height); prev_height+= 5;
	    max_right_edge = MAX(max_right_edge, (placex+prev_width));

	    vert_cnt++;
	    if (vert_cnt >= 5) {
		placex = max_right_edge + 5;
		vert_cnt = 0;
		placey = y;
		prev_height = 0;
	    }
	}
	delete selected;
    }

    return TRUE;
}

Decorator *
ControlPanel::createAndPlaceDecorator (const char *type, int x, int y)
{
    DecoratorStyle *ds = 
	DecoratorStyle::GetDecoratorStyle(type, DecoratorStyle::DefaultStyle);
    ASSERT(ds);
    Decorator *d = ds->createDecorator(this->isDeveloperStyle());
    ASSERT(d);
    d->setStyle (ds);
 
    DecoratorInfo *dnd = new DecoratorInfo (this->getNetwork(), (void*)this,
        (DragInfoFuncPtr)ControlPanel::SetOwner,
        (DragInfoFuncPtr)ControlPanel::DeleteSelections);
    d->setDecoratorInfo(dnd);
    d->setXYPosition (x,y);
    this->addComponentToList ((void *)d);
    return d;
}


//
// Normal termination of interactor placement.
// We reset the work space cursor and clear the list of Nodes to place. 
// 
void ControlPanel::concludeInteractorPlacement()
{
    // 
    // Unremember any nodes that were in the process of being added. 
    // 
    if (this->addingNodes)
        this->addingNodes->clear();

    // 
    // Reset the cursor to not indicate placement 
    // 
    if (this->workSpace)
        this->workSpace->resetCursor();

}

//
// Take the first item on this->addingNodes list and place
// it in the ContolPanel data structures and workspace.  
// We get this callback even when the list is empty, so be sure we
// do the right thing when it is empty.
//
void ControlPanel::placeSelectedInteractorCallback(Widget w, XtPointer cb)
{
    InteractorNode *node; 
    XmWorkspaceCallbackStruct *wscb = (XmWorkspaceCallbackStruct*) cb;

    //
    // If there are no selected Nodes, then there is nothing to do.
    //
    int deccnt = this->addingDecoratorList.getSize();
    int intcnt = 0;
    if (this->addingNodes) intcnt = this->addingNodes->getSize();
    if (!intcnt && !deccnt) return ;
    if (intcnt && deccnt) {
	this->addingDecoratorList.clear();
	deccnt = 0;
    }

    ASSERT(w == this->workSpace->getRootWidget());

    //
    // Deselect all other interactors.
    //
    ListIterator iterator(this->instanceList);
    InteractorInstance *ii;
    while ( (ii = (InteractorInstance*)iterator.getNext()) ) 
	    ii->setSelected(FALSE);
    //
    // Deselect all other decorators.
    //
    iterator.setList(this->componentList);
    WorkSpaceComponent *wsc;
    while( (wsc = (WorkSpaceComponent *)iterator.getNext()) )
	wsc->setSelected(FALSE);
    

    //
    // If there are decorators to add...
    // 		If it has a default Window, then open it.
    //		else just manage it.
    // The decorators were created when the menu button was clicked.
    // Now, they're sitting in a list waiting to be added.  Currently
    // the list is cleared before adding new elements (so why a list?).
    // ...because I copied the coding style from the treatment for
    // adding selected interactors.  Add decorators could work the same way.
    // 
    if (deccnt) {
	ListIterator it(this->addingDecoratorList);
	Decorator *dec;
	while ( (dec = (Decorator *)it.getNext()) ) {
	    dec->setXYPosition (wscb->event->xbutton.x, wscb->event->xbutton.y);
	    this->addComponentToList ((void *)dec);
	    dec->manage(this->workSpace);
	    dec->setSelected(TRUE);
	}
	this->addingDecoratorList.clear();

	// intcnt was 0 in order to get here but it need not be.
	if (intcnt == 0) this->concludeInteractorPlacement();
	return ;
    }

    node = (InteractorNode*) this->addingNodes->getElement(1);
    this->addingNodes->deleteElement(1);
    ASSERT(node);
  
    this->createAndPlaceInteractor(node,
                        wscb->event->xbutton.x,wscb->event->xbutton.y);
    //
    // If there are no more InteractorInstances to place, then end
    // the operation.
    //
    if (this->addingNodes->getSize() == 0)
        this->concludeInteractorPlacement();
   
   
}

//
// Create the default style of interactor for the given node and place it
// at the give x,y position.
//
InteractorInstance *ControlPanel::createAndPlaceInteractor(
				InteractorNode *inode, int x, int y)
{
    InteractorInstance *ii = NULL;

    //
    // Get the default interactor style for this type of interactor
    //
    InteractorStyle *is = InteractorStyle::GetInteractorStyle(
				inode->getNameString(),
				DefaultStyle);

    if (!is) {
	ErrorMessage(
		"Can not find default interactor style for a %s interactor",
		inode->getNameString());
    } else {
	
	//
	// Now that we have the position, add the instance to the Node's
	// list of known interactor instances.
	// This also adds the interactor instance to the control panels
	// list of known instances.
	//
	ii = inode->addInstance(x, y, is,this);
	ii->createInteractor();
	ii->setSelected(TRUE);
    }
    return ii;
}


Widget ControlPanel::getWorkSpaceWidget()
{ 
    ASSERT(this->workSpace); 
    return workSpace->getRootWidget();
}

//
// Set the name/title of this panel.
//
void ControlPanel::setPanelComment(const char *comment)
{

    if (this->comment != comment) {
       if (this->comment)
	    delete this->comment;

       if (comment && STRLEN(comment)) {
	    this->comment = DuplicateString(comment);
       } else {
	    this->comment = NULL; 
       }
    }

    if (this->comment && STRLEN(this->comment))	
	this->helpOnPanelCmd->activate(); 
    else
	this->helpOnPanelCmd->deactivate(); 

    this->getNetwork()->setFileDirty();
}

void ControlPanel::setPanelName(const char *name)
{
    this->setWindowTitle(name, TRUE);

    if(this->network->getEditor())
        this->network->getEditor()->notifyCPChange();
    theDXApplication->notifyPanelChanged();
}

//
// Same as the super class, but also sets the icon name.
//
void ControlPanel::setWindowTitle(const char *name, boolean check_geometry)
{
    boolean populated = (this->getComponentCount() > 0); 
    this->DXWindow::setWindowTitle(name, (check_geometry && populated));
    this->DXWindow::setIconName(name);
    if (this->helpOnPanelDialog)
    	this->helpOnPanelDialog->updateDialogTitle();
    if (this->setCommentDialog)
    	this->setCommentDialog->updateDialogTitle();
	
}

//
// Change the dimensionality of the selected interactor.
//
boolean ControlPanel::changeInteractorDimensionality(int new_dim)
{
    InteractorNode *inode;
    InteractorInstance *ii;

    if (!this->findTheSelectedInteractorInstance(&ii))
	return FALSE;

    inode = (InteractorNode*)ii->getNode();

    if (!inode->hasDynamicDimensionality())
	return FALSE;

    return inode->changeDimensionality(new_dim); 

}

//
// called by MainWindow::PopdownCallback().
//
void ControlPanel::popdownCallback()
{
    //
    // If the application does not allow panel access from the pulldown 
    // menus (and requires that they are popped up at startup), then
    // be sure that the user can't close the window.  It is not enough
    // to remove the "Close" and "Close All Control Panels" from the
    // "File" menu as mwm (and others?) provide their own close buttons.
    //
    if (!theDXApplication->appAllowsPanelAccess())
	return;

    this->network->closeControlPanel(this->getInstanceNumber(), FALSE);
}
//
// Virtual function called at the beginning of Command::execute().
// Before command in the system is executed we disable (the active 
// current) interactor placement.
//
void ControlPanel::beginCommandExecuting()
{
    this->concludeInteractorPlacement();
    this->DXWindow::beginCommandExecuting();
}
//
// De/Select all instances in the control panel.
//
void ControlPanel::selectAllInstances(boolean select)
{
    ListIterator li(this->instanceList);
    InteractorInstance *ii;

    while ( (ii = (InteractorInstance*)li.getNext()) )
	 ii->setSelected(select);
}
void ControlPanel::switchNetwork(Network *, Network *to)
{
    if (this->panelAccessManager) 
    	delete this->panelAccessManager;

    this->panelAccessManager = new PanelAccessManager(to, this);

    this->network = to;
}

void ControlPanel::setInstanceNumber(int instance)
{
    ASSERT(instance > 0);
    this->instanceNumber = instance;
}

boolean ControlPanel::nodeIsInInstanceList (Node *n)
{
    ListIterator li(this->instanceList);
    InteractorInstance *ii;

    while ( (ii = (InteractorInstance*)li.getNext()) )
	 if (n == ii->getNode()) return TRUE;

    return FALSE;
}



//
// These are the buttons that go at the bottom of the control panel in user style
//
Widget 
ControlPanel::createCommandArea(Widget parent)
{
Arg args[20];
int n;

    ASSERT(parent);

    n = 0;
    XtSetArg (args[n], XmNshadowThickness, 1); n++;
    XtSetArg (args[n], XmNshadowType, XmSHADOW_OUT); n++;
    Widget form = XmCreateForm (parent, "cpForm", args, n);

// Override the standard Motif translations so that we can use the control
// button press. 

    n = 0;
    XtSetArg (args[n], XmNleftAttachment, XmATTACH_POSITION); n++;
    XtSetArg (args[n], XmNrightAttachment, XmATTACH_POSITION); n++;
    XtSetArg (args[n], XmNleftPosition, 10); n++;
    XtSetArg (args[n], XmNrightPosition, 40); n++;
    XtSetArg (args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
    XtSetArg (args[n], XmNbottomOffset, 8); n++;
    XtSetArg (args[n], XmNtopAttachment, XmATTACH_FORM); n++;
    XtSetArg (args[n], XmNtopOffset, 6); n++;
    Widget pbCancel = XmCreatePushButton (form, "panelCancel", args, n);
    // Now override default.
    char *pbCancelTransStr = "\
	<EnterWindow>: 		Enter()\n\
	<LeaveWindow>: 		Leave()\n\
	<Btn1Down>: 		Arm()\n\
	<Btn1Down>,<Btn1Up>: 	Activate() Disarm()\n\
	<Btn1Down>(2+):		MultiArm()\n\
	<Btn1Up>:		Activate() Disarm()\n\
	:<Key>osfActivate:	PrimitiveParentActivate()\n\
	:<Key>osfCancel:	PrimitiveParentCancel()\n\
	:<Key>osfSelect:	ArmAndActivate()\n\
	:<Key>osfHelp:		Help()\n\
	~s ~m ~a <Key>Return:	PrimitiveParentActivate()\n\
	~s ~m ~a <Key>space:	ArmAndActivate()";
    XtTranslations pbCancelTrans = XtParseTranslationTable(pbCancelTransStr);
    XtVaSetValues (pbCancel, XtNtranslations, pbCancelTrans, NULL);
    XtManageChild (pbCancel);

    Widget pbHelp;
    n = 0;
    XtSetArg (args[n], XmNleftAttachment, XmATTACH_POSITION); n++;
    XtSetArg (args[n], XmNrightAttachment, XmATTACH_POSITION); n++;
    XtSetArg (args[n], XmNleftPosition, 60); n++;
    XtSetArg (args[n], XmNrightPosition, 90); n++;
    XtSetArg (args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
    XtSetArg (args[n], XmNbottomOffset, 8); n++;
    XtSetArg (args[n], XmNtopAttachment, XmATTACH_FORM); n++;
    XtSetArg (args[n], XmNtopOffset, 6); n++;

    ASSERT(this->helpOnPanelCmd);
    this->userHelpOption = new ButtonInterface(form, 
	"userHelpOption", this->helpOnPanelCmd);
    pbHelp = this->userHelpOption->getRootWidget();
    XtSetValues (pbHelp, args, n);

    XtAddCallback (pbCancel, XmNactivateCallback, 
	(XtCallbackProc)ControlPanel_DialogCancelCB, (XtPointer)this);

    return form;
}

void ControlPanel::SetOwner(void *b)
{
ControlPanel *cp = (ControlPanel*)b;
Network *netw = cp->getNetwork();
    netw->setCPSelectionOwner(cp);
}

void ControlPanel::DeleteSelections(void *b)
{
ControlPanel *cp = (ControlPanel*)b;

    cp->deleteSelectedInteractors();
}
//
// Do we allow the user to toggle between developer and dialog style
//
boolean ControlPanel::allowDeveloperStyleChange()
{
    EditorWindow *e = this->getNetwork()->getEditor();
    if (e && e->isManaged())
	return TRUE;
    else
	return FALSE;
}

//
// Supply the NW corner of the box containing all selected interactors
// This is used for drag-n-drop so that the drop site can reset the
// box. 
//
void ControlPanel::getMinSelectedXY(int *minx, int *miny)
{

    if ((!minx) || (!miny)) return ;

    ListIterator it;
    int x,y;
    int mx = *minx;
    int my = *miny;


    // 
    //  The Interactors
    //
    it.setList(this->instanceList);
    InteractorInstance *ii;
    while ( (ii = (InteractorInstance*)it.getNext()) ) {
	if (ii->isSelected()) {
	    Interactor *i = ii->getInteractor();
	    if (!i) continue;
	    i->getXYPosition (&x, &y);
	    mx = MIN(mx,x);
	    my = MIN(my,y);
	}
    }

    // 
    //  The decorators.
    //
    WorkSpaceComponent *dd;
    it.setList(this->componentList);
    while ( (dd = (WorkSpaceComponent *)it.getNext()) ) {
	if (dd->isSelected()) {
	    dd->getXYPosition (&x, &y);
	    mx = MIN(mx,x);
	    my = MIN(my,y);
	}
    }
    *minx = mx;
    *miny = my;
}

boolean ControlPanel::printAsJava(FILE* aplf)
{
    boolean success = TRUE;
    const char* ns = this->getPanelNameString();
    int inst = this->getInstanceNumber();
    if ((ns) && (ns[0]) && (EqualString(ns, "Control Panel") == FALSE))
	fprintf (aplf,
	    "        ControlPanel %s = new ControlPanel(this.network, \"%s\", %d);\n",
	    this->getJavaVariableName(), ns, inst
	);
    else
	fprintf (aplf,
	    "        ControlPanel %s = new ControlPanel(this.network, null, %d);\n",
	    this->getJavaVariableName(), inst
	);
    if (fprintf (aplf, 
	"        this.network.addPanel(%s);\n\n",
	this->getJavaVariableName()
    ) <= 0) success = FALSE;

    int components  = (success?this->componentList.getSize():0);
    if (components > 0) {
	ListIterator it;
	int i = 0;
	it.setList(this->componentList);
	Decorator* dec;
	while ((success == TRUE) && (dec = (Decorator *)it.getNext())) {
	    if (dec->printAsJava(aplf, this->getJavaVariableName(), i++) == FALSE) 
		success = FALSE;
	}
    }

    return success;
}

const char* ControlPanel::getJavaVariableName()
{
    if (this->java_variable == NUL(char*)) {
	this->java_variable = new char[32];
	sprintf (this->java_variable, "cp_%d", this->getInstanceNumber());
    }
    return this->java_variable;
}


boolean ControlPanel::toggleHitDetection()
{
    ToggleButtonInterface* tbi = (ToggleButtonInterface*)
	this->hitDetectionOption;
    WorkSpaceInfo *wsinfo = this->workSpace->getInfo();
    this->hit_detection = tbi->getState();
    wsinfo->setPreventOverlap(this->hit_detection);
    this->workSpace->installInfo(NULL);
    return TRUE;
}

void ControlPanel::getGeometryAlternateNames(String* names, int* count, int max) 
{ 
    int cnt = *count;
    if (cnt < (max-1)) {
	char* name = new char[32];
	sprintf (name, "%s%d", this->name,this->instanceNumber);
	names[cnt++] = name;
	*count = cnt;
    }
    this->DXWindow::getGeometryAlternateNames(names, count, max);
}

void ControlPanel::getWorkSpaceSize(int *width, int *height)
{

    ASSERT ((width) && (height));
    ListIterator it;
    int x,y,w,h;
    int maxw = 0;
    int maxh = 0;


    // 
    //  The Interactors
    //
    it.setList(this->instanceList);
    InteractorInstance *ii;
    while ( (ii = (InteractorInstance*)it.getNext()) ) {
	ii->getXYPosition (&x, &y);
	ii->getXYSize (&w, &h);
	maxw = MAX(maxw,(x+w));
	maxh = MAX(maxh,(y+h));
    }

    // 
    //  The decorators.
    //
    WorkSpaceComponent *dd;
    it.setList(this->componentList);
    while ( (dd = (WorkSpaceComponent *)it.getNext()) ) {
	dd->getXYPosition (&x, &y);
	dd->getXYSize (&w, &h);
	maxw = MAX(maxw,(x+w));
	maxh = MAX(maxh,(y+h));
    }
    *width = maxw;
    *height = maxh;
}
