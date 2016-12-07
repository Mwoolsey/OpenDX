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
#include <string.h> // for strerror
#include <errno.h> // for errno
#include <Xm/CascadeB.h>
#include <Xm/DrawingA.h>
#include <Xm/Frame.h>
#include <Xm/Form.h>
#include <Xm/Label.h>
#include <Xm/List.h>
#include <Xm/PushB.h>
#include <Xm/RowColumn.h>
#include <Xm/ScrolledW.h>
#include <Xm/Separator.h>
#include <Xm/AtomMgr.h>
#if !defined(ibm6000)
#define class ____class
#define new ____new
#endif
#include <Xm/ScrolledWP.h>
#if !defined(ibm6000)
#undef class
#undef new
#endif
#include <Xm/ScrollBar.h>

#include <X11/cursorfont.h>


#include "../widgets/WorkspaceW.h"
#include "EditorWindow.h"
#include "Ark.h"
#include "ButtonInterface.h"
#include "ColormapNode.h"
#include "ImageNode.h"
#include "ConfigurationDialog.h"
#include "ControlPanel.h"
#include "EditorToolSelector.h"
#include "VPERoot.h"
#include "ErrorDialogManager.h"
#include "DXApplication.h"
#include "DeleteNodeCommand.h"
#include "Dictionary.h"
#include "DictionaryIterator.h"
#include "InteractorNode.h"
#include "InteractorInstance.h"
#include "Network.h"
#include "NoUndoEditorCommand.h"
#include "NoUndoDXAppCommand.h"
#include "NodeDefinition.h"
#include "Node.h"
#include "Parameter.h"
#include "StandIn.h"
#include "ArkStandIn.h"
#include "ToggleButtonInterface.h"
#include "ToolPanelCommand.h"
#include "PanelAccessManager.h"
#include "PanelGroupManager.h"
#include "CloseWindowCommand.h"
#include "List.h"
#include "ListIterator.h"
#include "ProcessGroupCreateDialog.h"
#include "ProcessGroupAssignDialog.h"
#include "ControlPanelGroupDialog.h"
#include "ControlPanelAccessDialog.h"
#include "CreateMacroDialog.h"
#include "WarningDialogManager.h"
#include "InfoDialogManager.h"
#include "FindToolDialog.h"
#include "GridDialog.h"
#include "DXStrings.h"
#include "MacroNode.h"
#include "MacroDefinition.h"
#include "MacroParameterDefinition.h"
#include "MacroParameterNode.h"
#include "DeferrableAction.h"
#include "QuestionDialogManager.h"
#include "Decorator.h"
#include "LabelDecorator.h"
#include "VPEAnnotator.h"
#include "DecoratorStyle.h"
#include "DecoratorInfo.h"
#include "PrintProgramDialog.h"
#include "TransferAccelerator.h"
#if WORKSPACE_PAGES
#include "PageGroupManager.h"
#include "AnnotationGroupManager.h"
#include "PageSelector.h"
#endif

#ifdef DXUI_DEVKIT
#include "SaveAsCCodeDialog.h"
#endif // DXUI_DEVKIT

#include "CascadeMenu.h"
#include "TransmitterNode.h"
#include "ReceiverNode.h"
#include "GlobalLocalNode.h"
#include "DXLInputNode.h"
#include "GraphLayout.h"
#include "UndoMove.h"
#include "UndoDeletion.h"
#include "UndoRepeatableTab.h"
#include "UndoAddArk.h"
#include "Stack.h"
#include "XHandler.h"

#ifndef FORGET_GETSET
#include "GetSetConversionDialog.h"
Command *EditorWindow::SelectedToGlobalCmd = NULL;
Command *EditorWindow::SelectedToLocalCmd = NULL;
GetSetConversionDialog *EditorWindow::GetSetDialog = NULL;
#endif

#ifdef DXD_WIN
#define unlink _unlink
#endif

//
// Transmitter/Receiver for the execution/loop sequence numbers
#define JAVA_SEQUENCE "java_sequence"

// If you change this, you break existing nets
#define JAVA_SEQ_PAGE "java tools"

#include "EWDefaultResources.h"

#define NET_ATOM "NET_CONTENTS"
#define CFG_ATOM "CFG_CONTENTS"

boolean EditorWindow::ClassInitialized = FALSE;

EditorWindow::EditorWindow(boolean  isAnchor, Network* network) : 
			   DXWindow("editorWindow", isAnchor)
{
    ASSERT(network);

    //
    // Save associated network and designate self as the editor
    // for this network.
    //
    this->network = network;
    this->initialNetwork = TRUE;
    this->lastSelectedTransmitter = NULL;

    ASSERT(this->network->editor == NUL(EditorWindow*));
    this->network->editor = this;
    this->network->showEditorMessages();

    //
    // The panel is visible initially.
    //
    this->panelVisible = TRUE;

    //
    // Initially set hit detection to FALSE.  It's not saved in 
    // the net file currently.
    //
    this->hit_detection = FALSE;

    //
    // Set the origin flag
    //
    this->resetOrigin();

    this->findToolDialog 	   = NULL;
    this->printProgramDialog 	   = NULL;
    this->saveAsCCodeDialog 	   = NULL;
    this->panelGroupDialog 	   = NULL;
    this->gridDialog		   = NULL;
    this->processGroupCreateDialog = NULL;
    this->processGroupAssignDialog = NULL;
    this->createMacroDialog        = NULL;
    this->insertNetworkDialog      = NULL;

    //
    // Initialize member data.
    //
    this->currentPanel   = NUL(ControlPanel*);

    this->fileMenu       = NUL(Widget);
    this->editMenu       = NUL(Widget);
    this->windowsMenu    = NUL(Widget);
    this->optionsMenu    = NUL(Widget);

    this->fileMenuPulldown       = NUL(Widget);
    this->editMenuPulldown       = NUL(Widget);
    this->windowsMenuPulldown    = NUL(Widget);
    this->optionsMenuPulldown    = NUL(Widget);
	
    this->newOption          = NUL(CommandInterface*);
    this->openOption         = NUL(CommandInterface*);
    this->loadMacroOption    = NUL(CommandInterface*);
    this->loadMDFOption    = NUL(CommandInterface*);
    this->saveOption         = NUL(CommandInterface*);
    this->saveAsOption       = NUL(CommandInterface*);
    this->settingsCascade    = NULL;
    this->saveCfgOption	     = NULL;
    this->openCfgOption	     = NULL;
    this->printProgramOption = NUL(CommandInterface*);
    this->saveAsCCodeOption  = NUL(CommandInterface*);
    this->quitOption         = NUL(CommandInterface*);
    this->closeOption         = NUL(CommandInterface*);

    this->undoOption              = NUL(CommandInterface*);
    this->valuesOption            = NUL(CommandInterface*);
    this->findToolOption          = NUL(CommandInterface*);
    this->Ox			  = -1;
    this->find_restore_page       = NUL(char*);
#ifndef FORGET_GETSET
    this->programVerifyCascade    = NUL(CascadeMenu*);
#endif
    this->editTabsCascade	  = NULL;
    this->addInputTabOption	  = NUL(CommandInterface*);
    this->removeInputTabOption	  = NUL(CommandInterface*);
    this->editSelectCascade	  = NULL;
    this->outputCacheabilityCascade	    = NULL;
    this->editOutputCacheabilityCascade	    = NULL;
#if WORKSPACE_PAGES
    this->pageCascade	          = NULL;
#endif
    this->javaCascade	          = NULL;
    this->deleteOption            = NUL(CommandInterface*);
    this->cutOption            	  = NUL(CommandInterface*);
    this->copyOption              = NUL(CommandInterface*);
    this->pasteOption             = NUL(CommandInterface*);
    this->macroNameOption         = NUL(CommandInterface*);
    this->reflowGraphOption       = NUL(CommandInterface*);
    this->createMacroOption	  = NULL;
    this->insertNetworkOption     = NUL(CommandInterface*);
    this->addAnnotationOption     = NUL(CommandInterface*);
    this->createProcessGroupOption= NUL(CommandInterface*);
    this->assignProcessGroupOption= NUL(CommandInterface*);
    this->commentOption           = NUL(CommandInterface*);

    this->newControlPanelOption        = NUL(CommandInterface*);
    this->openControlPanelOption       = NUL(CommandInterface*);
    this->openAllControlPanelsOption   = NUL(CommandInterface*);
    this->openControlPanelByNameMenu   = NULL;
    this->openMacroOption              = NUL(CommandInterface*);
    this->openImageOption              = NUL(CommandInterface*);
    this->openColormapEditorOption     = NUL(CommandInterface*);

    this->toolPalettesOption  = NUL(CommandInterface*);
    this->panelGroupOption    = NUL(CommandInterface*);
    this->gridOption          = NUL(CommandInterface*);

    this->onVisualProgramOption = NUL(CommandInterface*);
    this->messageWindowOption   = NUL(CommandInterface*);
    this->workSpace = NULL;
    this->addingDecorators = NUL(List*);
    this->pendingPaste = NUL(Network*);
    this->copiedNet = NUL(char*);
    this->copiedCfg = NUL(char*);

    //
    // Create the commands.
    //

    //
    // File menu commands
    //
    this->closeCmd =
        new CloseWindowCommand("close",this->commandScope,TRUE,this);

    this->printProgramCmd =
	new NoUndoEditorCommand 
	    ("printProgram", this->commandScope, FALSE, 
		this, NoUndoEditorCommand::PrintProgram, NoUndoEditorCommand::Ignore);

#ifdef DXUI_DEVKIT
    this->saveAsCCodeCmd =
	new NoUndoEditorCommand 
	    ("saveAsCCode", this->commandScope, FALSE, 
		this, NoUndoEditorCommand::SaveAsCCode, NoUndoEditorCommand::Ignore);
#else
    this->saveAsCCodeCmd = NULL;
#endif

    //
    // Edit menu commands
    //
    this->undoCmd = 
	new NoUndoEditorCommand
	    ("undo", this->commandScope, FALSE,
	        this, NoUndoEditorCommand::Undo, NoUndoEditorCommand::Ignore);
    this->valuesCmd =
	new NoUndoEditorCommand 
	    ("values", this->commandScope, FALSE, 
		this, NoUndoEditorCommand::ShowConfiguration,
		NoUndoEditorCommand::Ignore);

    this->findToolCmd =
	new NoUndoEditorCommand 
	    ("findTool", this->commandScope, FALSE, 
		this, NoUndoEditorCommand::OpenFindTool,
		NoUndoEditorCommand::Ignore);

    this->addInputTabCmd =
	new NoUndoEditorCommand 
	    ("addInputTab", this->commandScope, FALSE, 
		this, NoUndoEditorCommand::AddInputTab,
		NoUndoEditorCommand::AffectsUndo|NoUndoEditorCommand::CanBeUndone);

    this->removeInputTabCmd =
	new NoUndoEditorCommand 
	    ("removeInputTab", this->commandScope, FALSE, 
		this, NoUndoEditorCommand::RemoveInputTab,
		NoUndoEditorCommand::AffectsUndo|NoUndoEditorCommand::CanBeUndone);
    
    this->addOutputTabCmd =
	new NoUndoEditorCommand 
	    ("addOutputTab", this->commandScope, FALSE, 
		this, NoUndoEditorCommand::AddOutputTab,
		NoUndoEditorCommand::AffectsUndo|NoUndoEditorCommand::CanBeUndone);

    this->removeOutputTabCmd =
	new NoUndoEditorCommand 
	    ("removeOutputTab", this->commandScope, FALSE, 
		this, NoUndoEditorCommand::RemoveOutputTab,
		NoUndoEditorCommand::AffectsUndo|NoUndoEditorCommand::CanBeUndone);
    
    this->hideAllTabsCmd =
	new NoUndoEditorCommand 
	    ("hideAllTabs", this->commandScope, FALSE, 
		this, NoUndoEditorCommand::HideAllTabs);

    this->revealAllTabsCmd =
	new NoUndoEditorCommand 
	    ("revealAllTabs", this->commandScope, FALSE, 
		this, NoUndoEditorCommand::RevealAllTabs);

#ifndef FORGET_GETSET
    this->postGetSetCmd =
	new NoUndoEditorCommand 
	    ("postGetSet", this->commandScope, TRUE, 
		this, NoUndoEditorCommand::PostGetSet,
		NoUndoEditorCommand::Ignore);
    this->toLocalCmd = 
	new NoUndoEditorCommand 
	    ("setToLocal", this->commandScope, FALSE, 
		this, NoUndoEditorCommand::SelectedToLocal);
    this->toGlobalCmd = 
	new NoUndoEditorCommand 
	    ("setToGlobal", this->commandScope, FALSE, 
		this, NoUndoEditorCommand::SelectedToGlobal);
#endif

    this->deleteNodeCmd =
	new DeleteNodeCommand
	    ("deleteNode", this->commandScope, FALSE, this);

    this->cutNodeCmd =
	new NoUndoEditorCommand
	    ("cutNode", this->commandScope, FALSE, 
		this, NoUndoEditorCommand::CutNode,
		NoUndoEditorCommand::AffectsUndo|NoUndoEditorCommand::CanBeUndone);

    this->copyNodeCmd =
	new NoUndoEditorCommand
	    ("copyNode", this->commandScope, FALSE, 
		this, NoUndoEditorCommand::CopyNode,
		NoUndoEditorCommand::Ignore);

    this->pasteNodeCmd =
	new NoUndoEditorCommand
	    ("pasteNode", this->commandScope, TRUE, 
		this, NoUndoEditorCommand::PasteNode);

    this->selectAllNodeCmd =
	new NoUndoEditorCommand 
	    ("selectAllNode", this->commandScope, FALSE, 
		this, NoUndoEditorCommand::SelectAll,
		NoUndoEditorCommand::Ignore);

    this->selectConnectedNodeCmd =
	new NoUndoEditorCommand 
	    ("selectConnectedNode", this->commandScope, FALSE, 
		this, NoUndoEditorCommand::SelectConnected,
		NoUndoEditorCommand::Ignore);

    this->selectUnconnectedNodeCmd =
	new NoUndoEditorCommand 
	    ("selectUnconectedNode", this->commandScope, FALSE, 
		this, NoUndoEditorCommand::SelectUnconnected,
		NoUndoEditorCommand::Ignore);

    this->selectUpwardNodeCmd =
	new NoUndoEditorCommand 
	    ("selectUpwardNode", this->commandScope, FALSE, 
		this, NoUndoEditorCommand::SelectUpward,
		NoUndoEditorCommand::Ignore);

    this->selectDownwardNodeCmd =
	new NoUndoEditorCommand 
	    ("selectDownwardNode", this->commandScope, FALSE, 
		this, NoUndoEditorCommand::SelectDownward,
		NoUndoEditorCommand::Ignore);

    this->deselectAllNodeCmd =
	new NoUndoEditorCommand 
	    ("deselectAllNode", this->commandScope, FALSE, 
		this, NoUndoEditorCommand::DeselectAll,
		NoUndoEditorCommand::Ignore);

    this->selectUnselectedNodeCmd =
	new NoUndoEditorCommand 
	    ("selectUnselectedNode", this->commandScope, FALSE, 
		this, NoUndoEditorCommand::SelectUnselected,
		NoUndoEditorCommand::Ignore);

    this->showExecutedCmd = 
	new NoUndoEditorCommand
	    ("showExecuted", this->commandScope, FALSE, 
		this, NoUndoEditorCommand::ShowExecutedNodes,
		NoUndoEditorCommand::Ignore);

#ifndef FORGET_GETSET
    if (!EditorWindow::SelectedToGlobalCmd) {
	EditorWindow::SelectedToGlobalCmd = new NoUndoEditorCommand 
	    ("SelectedToGlobal", this->commandScope, FALSE, 
		this, NoUndoEditorCommand::ToGlobal);

	EditorWindow::SelectedToLocalCmd = new NoUndoEditorCommand 
	    ("SelectedToLocal", this->commandScope, FALSE, 
		this, NoUndoEditorCommand::ToLocal);
    }
#endif

    this->cacheAllOutputsCmd =
	new NoUndoEditorCommand 
	    ("cacheAllOutputsCmd", this->commandScope, TRUE, 
		this, NoUndoEditorCommand::SetOutputsFullyCached,
		NoUndoEditorCommand::Ignore);
    this->cacheLastOutputsCmd =
	new NoUndoEditorCommand 
	    ("cacheLastOutputsCmd", this->commandScope, TRUE, 
		this, NoUndoEditorCommand::SetOutputsCacheOnce,
		NoUndoEditorCommand::Ignore);
    this->cacheNoOutputsCmd =
	new NoUndoEditorCommand 
	    ("cacheNoOutputsCmd", this->commandScope, TRUE, 
		this, NoUndoEditorCommand::SetOutputsNotCached,
		NoUndoEditorCommand::Ignore);

    this->optimizeCacheabilityCmd =
	new NoUndoEditorCommand 
	    ("optimizeCacheabilityCmd", this->commandScope, TRUE, 
		this, NoUndoEditorCommand::OptimizeOutputCacheability,
		NoUndoEditorCommand::Ignore);

    this->showCacheAllOutputsCmd =
	new NoUndoEditorCommand 
	    ("showCacheAllOutputsCmd", this->commandScope, TRUE, 
		this, NoUndoEditorCommand::ShowOutputsFullyCached,
		NoUndoEditorCommand::Ignore);
    this->showCacheLastOutputsCmd =
	new NoUndoEditorCommand 
	    ("showCacheLastOutputsCmd", this->commandScope, TRUE, 
		this, NoUndoEditorCommand::ShowOutputsCacheOnce,
		NoUndoEditorCommand::Ignore);
    this->showCacheNoOutputsCmd =
	new NoUndoEditorCommand 
	    ("showCacheNoOutputsCmd", this->commandScope, TRUE, 
		this, NoUndoEditorCommand::ShowOutputsNotCached,
		NoUndoEditorCommand::Ignore);


    this->editMacroNameCmd =
	new NoUndoEditorCommand 
	    ("editMacroName", this->commandScope, TRUE, 
		this, NoUndoEditorCommand::EditMacroName);

    this->editCommentCmd =
	new NoUndoEditorCommand 
	    ("editComment", this->commandScope, TRUE, 
		this, NoUndoEditorCommand::EditComment,
		NoUndoEditorCommand::Ignore);

    this->createProcessGroupCmd =
	new NoUndoEditorCommand 
	    ("createProcessGroup", this->commandScope, isAnchor, 
		this, NoUndoEditorCommand::CreateProcessGroup,
		NoUndoEditorCommand::Ignore);

    this->insertNetCmd =
	new NoUndoEditorCommand 
	    ("insertNet", this->commandScope, TRUE, 
		this, NoUndoEditorCommand::InsertNetwork);

    this->addAnnotationCmd =
	new NoUndoEditorCommand 
	    ("addAnnotation", this->commandScope, TRUE, 
		this, NoUndoEditorCommand::AddAnnotation);

    this->macroifyCmd =
	new NoUndoEditorCommand 
	    ("macroify", this->commandScope, FALSE, 
		this, NoUndoEditorCommand::Macroify);

#if WORKSPACE_PAGES
    this->pagifyCmd =
	new NoUndoEditorCommand 
	    ("pagify", this->commandScope, TRUE, 
		this, NoUndoEditorCommand::Pagify);

    this->pagifySelectedCmd =
	new NoUndoEditorCommand 
	    ("pagifySelected", this->commandScope, FALSE, 
		this, NoUndoEditorCommand::PagifySelected);

    this->autoChopSelectedCmd =
	new NoUndoEditorCommand 
	    ("autoChopSelected", this->commandScope, FALSE, 
		this, NoUndoEditorCommand::AutoChopSelected);

    this->autoFuseSelectedCmd =
	new NoUndoEditorCommand 
	    ("autoFuseSelected", this->commandScope, FALSE, 
		this, NoUndoEditorCommand::AutoFuseSelected);

    this->deletePageCmd =
	new NoUndoEditorCommand 
	    ("deletePage", this->commandScope, FALSE, 
		this, NoUndoEditorCommand::DeletePage);

    this->configurePageCmd =
	new NoUndoEditorCommand 
	    ("configurePage", this->commandScope, TRUE, 
		this, NoUndoEditorCommand::ConfigurePage,
		NoUndoEditorCommand::Ignore);

    this->moveSelectedCmd =
	new NoUndoEditorCommand 
	    ("moveSelectedNodes", this->commandScope, TRUE, 
		this, NoUndoEditorCommand::MoveSelected);
#endif

    if (this->network->isMacro() == FALSE) {

	this->javifyNetCmd = new NoUndoEditorCommand
	    ("javifyNetwork", this->commandScope, FALSE,
		this, NoUndoEditorCommand::JavifyNetwork);
	this->unjavifyNetCmd = new NoUndoEditorCommand
	    ("unJavifyNetwork", this->commandScope, TRUE,
		this, NoUndoEditorCommand::UnjavifyNetwork);
    } else {
	this->javifyNetCmd = NUL(Command*);
	this->unjavifyNetCmd = NUL(Command*);
    }
    this->reflowGraphCmd = 
	new NoUndoEditorCommand ("reflowGraph", this->commandScope, FALSE,
		this, NoUndoEditorCommand::ReflowGraph,
		NoUndoEditorCommand::AffectsUndo|NoUndoEditorCommand::CanBeUndone);

    //
    // Window's menu commands
    //
    this->openMacroCmd =
        new NoUndoEditorCommand("openMacroCommand", this->commandScope,
                        FALSE,this, NoUndoEditorCommand::OpenSelectedMacros,
			NoUndoEditorCommand::Ignore);

    this->openColormapCmd =
        new NoUndoEditorCommand("openColormapCmd",
                        this->commandScope,
                        FALSE,
                        this,
                        NoUndoEditorCommand::OpenSelectedColormaps,
			NoUndoEditorCommand::Ignore);
    this->openImageCmd =
        new NoUndoEditorCommand("openImageCommand", this->commandScope,
                        FALSE,this,
			NoUndoEditorCommand::OpenSelectedImageWindows,
			NoUndoEditorCommand::Ignore);

    this->newControlPanelCmd =
	new NoUndoEditorCommand 
	    ("newControlPanel", this->commandScope, TRUE, 
		this, NoUndoEditorCommand::NewControlPanel,
		NoUndoEditorCommand::Ignore);

    this->openControlPanelCmd =
	new NoUndoEditorCommand 
	    ("openControlPanel", this->commandScope, FALSE, 
		this, NoUndoEditorCommand::OpenControlPanel,
		NoUndoEditorCommand::Ignore);

    //
    // Options's menu commands
    //
    this->toolPanelCmd =
	new ToolPanelCommand
	    ("toolPalettes", this->commandScope, TRUE, this);

    this->hitDetectionCmd =
	new NoUndoEditorCommand
	    ("hitDetector", this->commandScope, TRUE, 
		this, NoUndoEditorCommand::HitDetection,
		NoUndoEditorCommand::Ignore);

    this->setPanelAccessCmd =
	new NoUndoEditorCommand 
	    ("setAccess", this->commandScope, FALSE, 
		this, NoUndoEditorCommand::SetCPAccess,
		NoUndoEditorCommand::Ignore);

    this->setPanelGroupCmd =
	new NoUndoEditorCommand 
	    ("setGroup", this->commandScope, FALSE, 
		this, NoUndoEditorCommand::SetPanelGroup,
		NoUndoEditorCommand::Ignore);

    this->gridCmd =
	new NoUndoEditorCommand 
	    ("grid", this->commandScope, TRUE, 
		this, NoUndoEditorCommand::OpenGrid,
		NoUndoEditorCommand::Ignore); 

    this->deferrableCommandActivation = new DeferrableAction(
				EditorWindow::SetCommandActivation,
				(void*)this);

    this->addInputTabCmd->autoActivate(this->removeInputTabCmd);
    this->addOutputTabCmd->autoActivate(this->removeOutputTabCmd);
    this->revealAllTabsCmd->autoActivate(this->hideAllTabsCmd); 
    this->hideAllTabsCmd->autoActivate(this->revealAllTabsCmd); 

    this->resetWindowTitle();

    //
    // List of executed nodes
    //
    this->executed_nodes = NUL(List*);
    this->executing_node = NUL(Node*);
    this->transmitters_added = FALSE;
    this->errored_standins = NUL(List*);

    //
    // Graph layout
    //
    this->layout_controller = NULL;
    
    //
    // Undo list management
    //
    this->moving_many_standins = FALSE;
    this->performing_undo = FALSE;
    this->creating_new_network = FALSE;

    // work around for a motif bug
    this->pgKeyHandler = NUL(XHandler*);

    //
    // Install the default resources for THIS class (not the derived classes)
    //
    if (NOT EditorWindow::ClassInitialized)
    {
	ASSERT(theApplication);
        EditorWindow::ClassInitialized = TRUE;
	this->installDefaultResources(theApplication->getRootWidget());
    }
}


EditorWindow::~EditorWindow()
{

    //
    // Delete the dialogs.
    //
    if(this->findToolDialog)
	delete this->findToolDialog;
    if(this->printProgramDialog)
	delete this->printProgramDialog;
    if(this->saveAsCCodeDialog)
	delete this->saveAsCCodeDialog;
    if(this->panelGroupDialog)
	delete this->panelGroupDialog;
    if(this->gridDialog)
	delete this->gridDialog;
    if(this->processGroupCreateDialog)
	delete this->processGroupCreateDialog;
    if(this->processGroupAssignDialog)
	delete this->processGroupAssignDialog;
    if(this->createMacroDialog)
	delete this->createMacroDialog;
    if(this->insertNetworkDialog)
	delete this->insertNetworkDialog;

    //
    // File menu options
    //
    if (this->newOption) delete this->newOption;
    if (this->openOption) delete this->openOption;
    if (this->loadMacroOption) delete this->loadMacroOption;
    if (this->loadMDFOption) delete this->loadMDFOption;
    if (this->saveOption) delete this->saveOption;
    if (this->saveAsOption) delete this->saveAsOption;
    if (this->settingsCascade) delete this->settingsCascade;
    if (this->saveCfgOption) delete this->saveCfgOption;
    if (this->openCfgOption) delete this->openCfgOption;
    if (this->printProgramOption) delete this->printProgramOption; 
    if (this->saveAsCCodeOption) delete this->saveAsCCodeOption; 
    if (this->quitOption) delete this->quitOption;
    if (this->closeOption) delete this->closeOption;

    //
    // Edit menu options
    //
    if (this->undoOption) delete this->undoOption;
    if (this->valuesOption) delete this->valuesOption;
    if (this->findToolOption) delete this->findToolOption;
#ifndef FORGET_GETSET
    if (this->programVerifyCascade) delete this->programVerifyCascade;
#endif
    if (this->editTabsCascade) delete this->editTabsCascade;
    //if (this->addInputTabOption) delete this->addInputTabOption;
    //if (this->removeInputTabOption) delete this->removeInputTabOption;
    if (this->editSelectCascade) delete this->editSelectCascade;
    if (this->editOutputCacheabilityCascade) delete this->editOutputCacheabilityCascade;
    if (this->outputCacheabilityCascade) delete this->outputCacheabilityCascade;
#if WORKSPACE_PAGES
    if (this->pageCascade) delete this->pageCascade;
#endif
    if (this->javaCascade) delete this->javaCascade;
    if (this->deleteOption) delete this->deleteOption;
    if (this->cutOption) delete this->cutOption;
    if (this->copyOption) delete this->copyOption;
    if (this->pasteOption) delete this->pasteOption;
    if (this->macroNameOption) delete this->macroNameOption;
    if (this->reflowGraphOption) delete this->reflowGraphOption;
    if (this->createMacroOption) delete this->createMacroOption;
    if (this->insertNetworkOption) delete this->insertNetworkOption;
    if (this->addAnnotationOption) delete this->addAnnotationOption;
    if (this->createProcessGroupOption) delete this->createProcessGroupOption;
    if (this->commentOption) delete this->commentOption;


    //
    // Windows options
    //
    if (this->newControlPanelOption) delete this->newControlPanelOption;
    if (this->openControlPanelOption) delete this->openControlPanelOption;
    if (this->openAllControlPanelsOption) delete this->openAllControlPanelsOption;
    if (this->openControlPanelByNameMenu) delete this->openControlPanelByNameMenu;
    if (this->openMacroOption) delete this->openMacroOption;
    if (this->openImageOption) delete this->openImageOption;
    if (this->openColormapEditorOption) delete this->openColormapEditorOption;
    if (this->messageWindowOption) delete this->messageWindowOption;

    //
    // Options options
    //
    if (this->toolPalettesOption) delete this->toolPalettesOption;
    if (this->gridOption) delete this->gridOption;
    if (this->panelGroupOption) delete this->panelGroupOption;
    if (this->panelAccessOption) delete this->panelAccessOption;

    if (this->onVisualProgramOption) delete this->onVisualProgramOption;

    if (this->workSpace) delete this->workSpace;
	
    //
    // Delete the commands after the command interfaces.
    //
    delete this->toolPanelCmd;
    delete this->hitDetectionCmd;
    delete this->showExecutedCmd;
    delete this->newControlPanelCmd;
    delete this->openControlPanelCmd;
    delete this->undoCmd;
    delete this->valuesCmd;
    delete this->addInputTabCmd;
    delete this->removeInputTabCmd;
    delete this->addOutputTabCmd;
    delete this->removeOutputTabCmd;
    delete this->hideAllTabsCmd;
    delete this->revealAllTabsCmd;
#ifndef FORGET_GETSET
    delete this->postGetSetCmd;
    delete this->toLocalCmd;
    delete this->toGlobalCmd;
#endif
    delete this->deleteNodeCmd;
    delete this->cutNodeCmd;
    delete this->copyNodeCmd;
    delete this->pasteNodeCmd;
    delete this->selectAllNodeCmd;
    delete this->selectConnectedNodeCmd;
    delete this->selectUnconnectedNodeCmd;
    delete this->selectUnselectedNodeCmd;
    delete this->selectUpwardNodeCmd;
    delete this->selectDownwardNodeCmd;
    delete this->cacheAllOutputsCmd;
    delete this->cacheLastOutputsCmd;
    delete this->cacheNoOutputsCmd;
    delete this->showCacheAllOutputsCmd;
    delete this->showCacheLastOutputsCmd;
    delete this->showCacheNoOutputsCmd;
    delete this->optimizeCacheabilityCmd;
    delete this->deselectAllNodeCmd;
    delete this->insertNetCmd;
    delete this->addAnnotationCmd;
    delete this->macroifyCmd;
#if WORKSPACE_PAGES
    delete this->pagifyCmd;
    delete this->pagifySelectedCmd;
    delete this->autoChopSelectedCmd;
    delete this->autoFuseSelectedCmd;
    delete this->deletePageCmd;
    delete this->configurePageCmd;
    delete this->moveSelectedCmd;
#endif
    if (this->javifyNetCmd) delete this->javifyNetCmd;
    if (this->unjavifyNetCmd) delete this->unjavifyNetCmd;
    delete this->reflowGraphCmd;
    delete this->editMacroNameCmd;
    delete this->editCommentCmd;
    delete this->findToolCmd;
    delete this->gridCmd;
    delete this->setPanelAccessCmd;
    delete this->setPanelGroupCmd;
    delete this->createProcessGroupCmd;
    delete this->openImageCmd;
    delete this->openColormapCmd;
    delete this->openMacroCmd;
    delete this->closeCmd;
    delete this->printProgramCmd;
    if (this->saveAsCCodeCmd) delete this->saveAsCCodeCmd;

    this->network->editor = NUL(EditorWindow*);

    delete this->deferrableCommandActivation;

    //
    // Delete the category/tool lists for this editor.
    //
    delete this->toolSelector;
#if WORKSPACE_PAGES
    delete this->pageSelector;
#endif

    if (this->addingDecorators) delete this->addingDecorators;
    if (this->pendingPaste) delete this->pendingPaste;
    if (this->copiedNet) delete this->copiedNet;
    if (this->copiedCfg) delete this->copiedCfg;
    if (this->executed_nodes) delete this->executed_nodes;
    if (this->errored_standins) delete this->errored_standins;
    if (this->layout_controller) delete this->layout_controller;
    this->clearUndoList();

    if (this->find_restore_page) delete this->find_restore_page;

    if (this->pgKeyHandler) delete this->pgKeyHandler;;
}

//
// Install the default resources for this class.
//
void EditorWindow::installDefaultResources(Widget baseWidget)
{
    this->setDefaultResources(baseWidget, EditorWindow::DefaultResources);
    this->DXWindow::installDefaultResources(baseWidget);
}

void EditorWindow::SetCommandActivation(void *editor, void *requestData)
{
    ((EditorWindow*)editor)->setCommandActivation();
}
//
// De/Activate any commands that have to do with editing.
// Should be called when a node is added or deleted or there is a selection
// status change.
//
void EditorWindow::setCommandActivation()
{
    int nselected;
 
    int nodes = this->getNetwork()->getNodeCount();

    int decors = 0;
    int dselected = 0;
    decors = this->getNetwork()->decoratorList.getSize();
    ListIterator it;
    Decorator *decor;
    FOR_EACH_NETWORK_DECORATOR(this->getNetwork(),decor,it) {
	if (decor->isSelected()) dselected++;
    }

    //
    // Are execution results available?
    //
    if ((this->executed_nodes) && (this->executed_nodes->getSize())) {
	this->showExecutedCmd->activate();
    } else {
	this->showExecutedCmd->deactivate();
    }

    //
    // count the nodes in the current page
    //
    WorkSpace *current_ws = this->workSpace;
    int page = this->workSpace->getCurrentPage();
    if (page) current_ws = this->workSpace->getElement(page);
    int nodes_in_current_page = 0;
    Node *n;
    ListIterator iterator;
    FOR_EACH_NETWORK_NODE(this->network, n, iterator) {
	StandIn* si = n->getStandIn();
	if (!si) continue;
	if (si->getWorkSpace() == current_ws)
	    nodes_in_current_page++;
    }

    //
    // Is undo available?  ...also sets button label
    //
    this->setUndoActivation();

    //
    // Handle commands that depend on whether there are nodes in the current page 
    //
    if (nodes_in_current_page==0) {
	this->reflowGraphCmd->deactivate();
    } else {
	this->reflowGraphCmd->activate();
    }

    //
    // Handle commands that depend on whether there are nodes 
    //
    if (nodes == 0) {
	if (decors == 0)
	    this->selectAllNodeCmd->deactivate();
	else
	    this->selectAllNodeCmd->activate();
	this->findToolCmd->deactivate();
	this->printProgramCmd->deactivate();
	if (this->saveAsCCodeCmd)
	    this->saveAsCCodeCmd->deactivate();
	nselected = 0;
	this->outputCacheabilityCascade->deactivate();
    } else {
	this->selectAllNodeCmd->activate();
	this->findToolCmd->activate();
	this->printProgramCmd->activate();
	if (this->saveAsCCodeCmd)
	    this->saveAsCCodeCmd->activate();
	nselected = this->getNodeSelectionCount();
	this->outputCacheabilityCascade->activate();
    } 

    //
    // Set Activation of commands that depend on whether or not the 
    // network is a macro.
    // FIXME: should ASSERT(net), but just before 2.0 release we'll be
    //	      VERY careful.
    //
    Network *net = this->getNetwork();
    if ((nodes != 0) && net && !net->isMacro()) {
        this->createProcessGroupCmd->activate();
    } else { 
        this->createProcessGroupCmd->deactivate();
    }

#ifndef FORGET_GETSET
    EditorWindow *ew = 
	((EditorWindow::GetSetDialog && EditorWindow::GetSetDialog->isManaged())? 
	  EditorWindow::GetSetDialog->getActiveEditor(): 
	  NULL);
    if ((EditorWindow::GetSetDialog) && 
	(EditorWindow::GetSetDialog->isManaged()) && (!ew)) {
	EditorWindow::GetSetDialog->setActiveEditor(this);
	ew = EditorWindow::GetSetDialog->getActiveEditor();
    }
#endif

    //
    // Handle commands that depend on whether there are populated pages
    //
#if WORKSPACE_PAGES
    if (this->pageSelector->getSize() > 1) {
	this->deletePageCmd->activate();
    } else {
	// does it have a group record
	int page_num = this->workSpace->getCurrentPage();
	EditorWorkSpace* ews = this->workSpace;
	if (page_num) 
	    ews = (EditorWorkSpace*)this->workSpace->getElement(page_num);
	GroupRecord* grec = this->getGroupOfWorkSpace(ews);
	if (grec)
	    this->deletePageCmd->activate();
	else
	    this->deletePageCmd->deactivate();
    }
#endif

    //
    // command depends on whether selected nodes are pagifiable
    //
    boolean mscmd_activate = FALSE;
    if ((nselected) || (dselected)) {
	Dialog* diag = this->pageSelector->getMoveNodesDialog();
	if ((diag) && (diag->isManaged())) {
	    if (this->pageSelector->getSize() > 1)
		mscmd_activate = this->areSelectedNodesPagifiable(FALSE);
	    else
		mscmd_activate = FALSE;
	} else {
	    mscmd_activate = TRUE;
	}
    }
    if (mscmd_activate)
	this->moveSelectedCmd->activate();
    else
	this->moveSelectedCmd->deactivate();

    //
    // Handle commands that depend on whether there are nodes selected
    //
    if (nselected == 0) {
	this->valuesCmd->deactivate();
	this->addInputTabCmd->deactivate();
	this->removeInputTabCmd->deactivate();
	this->addOutputTabCmd->deactivate();
	this->removeOutputTabCmd->deactivate();
	this->hideAllTabsCmd->deactivate();
	this->revealAllTabsCmd->deactivate();
	this->autoChopSelectedCmd->deactivate();
	this->autoFuseSelectedCmd->deactivate();
	if (dselected) {
	    this->deleteNodeCmd->activate();
	    this->cutNodeCmd->activate();
	    this->copyNodeCmd->activate();
	    this->pagifySelectedCmd->activate();
	} else {
	    this->cutNodeCmd->deactivate();
	    this->copyNodeCmd->deactivate();
	    this->deleteNodeCmd->deactivate();
	    this->pagifySelectedCmd->deactivate();
	}
	this->selectDownwardNodeCmd->deactivate();
	this->selectUpwardNodeCmd->deactivate();
	this->deselectAllNodeCmd->deactivate();
	this->selectConnectedNodeCmd->deactivate();
#ifndef FORGET_GETSET
	if ((EditorWindow::GetSetDialog) && 
	    (EditorWindow::GetSetDialog->isManaged()) &&
	    (this == EditorWindow::GetSetDialog->getActiveEditor())) {
	    this->SelectedToGlobalCmd->deactivate();
	    this->SelectedToLocalCmd->deactivate();
	}
	this->toLocalCmd->deactivate();
	this->toGlobalCmd->deactivate();
#endif
	this->selectUnconnectedNodeCmd->deactivate();
	this->selectUnselectedNodeCmd->deactivate(); // Same as selectAll
	this->macroifyCmd->deactivate();
	this->openColormapCmd->deactivate();
	this->openMacroCmd->deactivate();
	this->openImageCmd->deactivate();
	this->openControlPanelCmd->deactivate();
	this->editOutputCacheabilityCascade->deactivate();
    } else {
	//
	// Commands that can only work selected nodes, but depend on the Node.
	//
	boolean add_input  = FALSE, remove_input  = FALSE;
	boolean add_output = FALSE, remove_output = FALSE;
	boolean hide_all   = FALSE, reveal_all    = FALSE;
	boolean cacheable = FALSE;
#ifndef FORGET_GETSET
	boolean only_gets_n_sets = TRUE;
	boolean convertible = TRUE;
#endif
	Node *n;
	ListIterator iterator;
	FOR_EACH_NETWORK_NODE(this->network, n, iterator) {
	    if ((n->getStandIn()) && (n->getStandIn()->isSelected())) 
	    {
#ifndef FORGET_GETSET
		const char *cp = n->getNameString();
		if (!n->isA(ClassGlobalLocalNode)) 
		    convertible = only_gets_n_sets = FALSE;
		else if ((!EqualString(cp, "Get")) && (!EqualString(cp, "Set")))
		    only_gets_n_sets = FALSE;
#endif

		if (!add_input && n->isInputRepeatable())  
		    add_input = TRUE;
		if (!remove_input && n->hasRemoveableInput())
		    remove_input = TRUE;
		if (!add_output && n->isOutputRepeatable()) 
		    add_output = TRUE;
		if (!remove_output && n->hasRemoveableOutput())
		    remove_output = TRUE;
		if (!hide_all && 
		    (n->hasHideableInput() || n->hasHideableOutput()))
		    hide_all = TRUE;
		if (!reveal_all &&
		    (n->hasExposableInput() || n->hasExposableOutput()))
		    reveal_all = TRUE;
		int count = n->getOutputCount();
		for (int i=1 ; !cacheable && i<=count  ; i++)
		    cacheable = n->isOutputCacheabilityWriteable(i);
	    }
	}
	(add_input ? 	this->addInputTabCmd->activate() : 
			this->addInputTabCmd->deactivate());	
	(remove_input ? this->removeInputTabCmd->activate() : 
			this->removeInputTabCmd->deactivate());	
	(add_output ? 	this->addOutputTabCmd->activate() : 
			this->addOutputTabCmd->deactivate());	
	(remove_output? this->removeOutputTabCmd->activate() : 
			this->removeOutputTabCmd->deactivate());	
	(reveal_all ? 	this->revealAllTabsCmd->activate() : 
			this->revealAllTabsCmd->deactivate());	
	(hide_all ? 	this->hideAllTabsCmd->activate() : 
			this->hideAllTabsCmd->deactivate());	
	(cacheable ?	this->editOutputCacheabilityCascade->activate() :
			this->editOutputCacheabilityCascade->deactivate());
	//
	// Commands that can only work if there are unselected nodes 
	//
	if (nodes > nselected) {
	    this->selectAllNodeCmd->activate();
	    this->selectConnectedNodeCmd->activate();
	    this->selectUnconnectedNodeCmd->activate();
	    this->selectUnselectedNodeCmd->activate();
	    this->selectDownwardNodeCmd->activate();
	    this->selectUpwardNodeCmd->activate();
	} else {	// There are no unselected nodes
	    this->selectAllNodeCmd->deactivate();
	    this->selectConnectedNodeCmd->deactivate();
	    this->selectUnconnectedNodeCmd->deactivate();
	    this->selectUnselectedNodeCmd->deactivate();
	    this->selectDownwardNodeCmd->deactivate();
	    this->selectUpwardNodeCmd->deactivate();
	}
	//
	// And commands that depend on the class of node that is selected.
	//
        List *l = this->makeSelectedNodeList(NULL);
	boolean selected_macro = FALSE, selected_image = FALSE;
	boolean selected_colormap = FALSE;
	boolean selected_interactor = FALSE;
  	if (l) {
	    ListIterator  iterator(*l);
	    Node *n;
	    while ( (n = (Node*)iterator.getNext()) ) {
		if (!selected_colormap && n->isA(ClassColormapNode))
		    selected_colormap = TRUE;
		if (!selected_macro && n->isA(ClassMacroNode))
		    selected_macro = TRUE;
		if (!selected_image && n->isA(ClassDisplayNode))
		    selected_image = TRUE;
		if (!selected_interactor && n->isA(ClassInteractorNode))
		    selected_interactor = TRUE;
	    }
	    delete l;
	}
	(selected_macro ? 	this->openMacroCmd->activate() :
			  	this->openMacroCmd->deactivate());
	(selected_image ? 	this->openImageCmd->activate() :
	    		  	this->openImageCmd->deactivate());
	(selected_colormap ? 	this->openColormapCmd->activate() :
	    		     	this->openColormapCmd->deactivate());
	(selected_interactor ? 	this->openControlPanelCmd->activate() :
	    			this->openControlPanelCmd->deactivate());
	//
	// And...commands that can only work if there are selected nodes 
	//
	this->valuesCmd->activate();
	this->macroifyCmd->activate();
	this->deleteNodeCmd->activate();
	this->cutNodeCmd->activate();
	this->copyNodeCmd->activate();
	this->deselectAllNodeCmd->activate();
	this->pagifySelectedCmd->activate();
	this->autoChopSelectedCmd->activate();
	this->autoFuseSelectedCmd->activate();

	//
	// Convert Get/Set node commands.  Requires that 1 or more nodes selected and
	// every node selected is a Get or Set.
	//
#ifndef FORGET_GETSET
	if ((only_gets_n_sets) && (EditorWindow::GetSetDialog) &&
	    (this != ew) && (EditorWindow::GetSetDialog->isManaged())) {
	    EditorWindow::GetSetDialog->setActiveEditor(this);
	    ew = EditorWindow::GetSetDialog->getActiveEditor();
	}
	if (convertible) {
	    this->toLocalCmd->activate();
	    this->toGlobalCmd->activate();
	} else {
	    this->toLocalCmd->deactivate();
	    this->toGlobalCmd->deactivate();
	}
	if ((EditorWindow::GetSetDialog) && 
	    (EditorWindow::GetSetDialog->isManaged()) && (this == ew)) {
	    if (only_gets_n_sets) {
		if (!ew) EditorWindow::GetSetDialog->setActiveEditor(this);
		this->SelectedToGlobalCmd->activate();
		this->SelectedToLocalCmd->activate();
	    } else {
		this->SelectedToGlobalCmd->deactivate();
		this->SelectedToLocalCmd->deactivate();
	    }
	}
#endif
    }

    //
    // If there is at least 1 image node
    //
    if (this->javifyNetCmd) {
	boolean activate = FALSE;
	List* bns = this->network->makeClassifiedNodeList(ClassImageNode);
	if ((bns) && (bns->getSize() > 0)) {
	    activate = TRUE;
	}
	if (bns) delete bns;
	if (activate) {
	    this->javifyNetCmd->activate();
	    this->unjavifyNetCmd->activate();
	} else {
	    this->javifyNetCmd->deactivate();
	    this->unjavifyNetCmd->deactivate();
	}
    }

    this->editTabsCascade->setActivationFromChildren();
    this->editSelectCascade->setActivationFromChildren();
}

//
// Do any work that is required when a Node's status changes (generally 
// called by a StandIn).  Mostly what needs to be done is handle command 
// activation and deactivation. 
//
void EditorWindow::handleNodeStatusChange(Node *n, NodeStatusChange status)
{

    switch (status) {
	case Node::NodeSelected:
	case Node::NodeDeselected:
	    // FIXME: should we do this more quickly?
            this->deferrableCommandActivation->requestAction(NULL); 
            if (n->isA(ClassTransmitterNode)) 
                this->lastSelectedTransmitter = (TransmitterNode*)n;

	    break;
	default:
	    ASSERT(0);
    }
    
}
Widget EditorWindow::createWorkArea(Widget parent)
{
    Widget    form;
    Widget    hBar;
    Widget    vBar;
    Dimension height;
    Dimension width;
    Dimension thickness;

    ASSERT(parent);

    Widget outer_form = XtVaCreateManagedWidget ("workAreaFrame", xmFormWidgetClass, 
	    parent, 
	XmNshadowType, XmSHADOW_OUT,
	XmNshadowThickness, 1,
    NULL);

    form = XtVaCreateManagedWidget ("workArea", xmFormWidgetClass, outer_form, 
	XmNtopAttachment, XmATTACH_FORM,
	XmNleftAttachment, XmATTACH_FORM,
	XmNrightAttachment, XmATTACH_FORM,
	XmNbottomAttachment, XmATTACH_FORM,
	XmNtopOffset, 6,
	XmNleftOffset, 6,
	XmNrightOffset, 6,
	XmNbottomOffset, 6,
	XmNshadowThickness, 0,
    NULL);


    //
    // Create the category/tool lists as a child of the above form. 
    //

    this->toolSelector = new EditorToolSelector("toolSelector", this);
    if (!this->toolSelector->initialize(form,theNodeDefinitionDictionary))
	ASSERT(0);

    XtVaSetValues (this->toolSelector->getRootWidget(),
        XmNtopAttachment,    XmATTACH_FORM,
        XmNleftAttachment,   XmATTACH_FORM,
        XmNbottomAttachment, XmATTACH_FORM,
	XmNtopOffset, 0,
	XmNleftOffset, 0,
	XmNbottomOffset, 0,
    NULL);

#if WORKSPACE_PAGES
    this->pageSelector = new PageSelector (this, form, this->network);
    XtVaSetValues (this->pageSelector->getRootWidget(),
	XmNtopAttachment,	XmATTACH_FORM,
	XmNleftAttachment,      XmATTACH_WIDGET,
	XmNleftWidget,          this->toolSelector->getRootWidget(),
	XmNleftOffset,          5,
	XmNrightAttachment,     XmATTACH_FORM,
	XmNrightOffset,		20,
    NULL);
#endif

    //
    // Create the scrolled window.
    //
    this->scrolledWindow =
	XtVaCreateManagedWidget
	    ("scrolledWindow",
	     xmScrolledWindowWidgetClass,
	     form,
#if WORKSPACE_PAGES
	     XmNtopAttachment,          XmATTACH_WIDGET,
	     XmNtopWidget,          	this->pageSelector->getRootWidget(),
	     XmNtopOffset,		-1,
#else
	    XmNtopAttachment,	XmATTACH_FORM,
#endif
	     XmNleftAttachment,         XmATTACH_WIDGET,
	     XmNleftWidget,             this->toolSelector->getRootWidget(),
	     XmNleftOffset,             5,
	     XmNrightAttachment,        XmATTACH_FORM,
	     XmNbottomAttachment,       XmATTACH_FORM,
	     XmNscrollingPolicy,        XmAUTOMATIC,
	     XmNvisualPolicy,           XmVARIABLE,
	     XmNscrollBarDisplayPolicy, XmAS_NEEDED,
	     NULL);
    //
    // Create the workspace object.
    //

    this->workSpace = new VPERoot("vpeCanvas", this->scrolledWindow, 
				this->network->getWorkSpaceInfo(),
				this, this->pageSelector);
    this->workSpace->initializeRootWidget();
    this->workSpace->manage();
    this->pageSelector->setRootPage((VPERoot*)this->workSpace);

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

#if ((XmVERSION==2)&&(XmREVISION>=2))
    //
    // Pg {Up,Down} is crashing the vpe.  If this bug goes away
    // then remove this code.  Finding a bug fix for Motif isn't good
    // enough since we build with shared libraries.  Another way
    // to fix this might have been to make a translation table
    // entry.  I did try that but when I invoked the widget's
    // action routine, I got the same crash.
    // 
    // Event handling in libXt is more complex than I what I can write
    // code for at this level.  The 0 passed to this constructor is 
    // supposed to be a window id. 0 means wildcard i.e. we match any
    // event.  In KeyPress() we'll try to figure out if the widget
    // that belongs to this event is in the hierarchy of this window.
    // 
    if (!this->pgKeyHandler) {
	this->pgKeyHandler = new XHandler(KeyPress, 0, KeyHandler, (void*)this);
    }
#endif
    //
    // Return the topmost widget of the work area.
    //
    return outer_form;
}


void EditorWindow::createFileMenu(Widget parent)
{
    ASSERT(parent);

    Widget pulldown;
    Command *cmd;

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

    this->newOption =
	new ButtonInterface(pulldown,
			    "vpeNewOption",
			    this->network->getNewCommand());

    this->openOption =
	new ButtonInterface(pulldown, 
                            "vpeOpenOption", theDXApplication->openFileCmd);

    this->createFileHistoryMenu(pulldown);

    if ( (cmd = this->network->getSaveCommand()) ) {
    	if(this->network->isMacro() == FALSE) 
	    this->saveOption = new ButtonInterface(pulldown, "vpeSaveOption", cmd);
	else
	    this->saveOption = new ButtonInterface(pulldown, "vpeSaveMacroOption", cmd);
    }

    if ( (cmd = this->network->getSaveAsCommand()) ) {
        if(this->network->isMacro() == FALSE)
	    this->saveAsOption = new ButtonInterface(pulldown, 
						"vpeSaveAsOption", cmd);
	else
	    this->saveAsOption = new ButtonInterface(pulldown, 
						"vpeSaveMacroAsOption", cmd);
    }

   
    Command *openCfgCmd = this->network->getOpenCfgCommand();
    Command *saveCfgCmd = this->network->getSaveCfgCommand();
    if (openCfgCmd && saveCfgCmd) {
        this->settingsCascade = new CascadeMenu("vpeSettingsCascade",pulldown);
	Widget menu_parent = this->settingsCascade->getMenuItemParent();
	this->saveCfgOption = new ButtonInterface(menu_parent, 
						"vpeSaveCfgOption", saveCfgCmd);
	this->openCfgOption = new ButtonInterface(menu_parent, 
						"vpeOpenCfgOption", openCfgCmd);
    } else if (openCfgCmd) {
	this->openCfgOption = new ButtonInterface(pulldown, 
						"vpeOpenCfgOption", openCfgCmd);
    } else if (saveCfgCmd) {
	this->saveCfgOption = new ButtonInterface(pulldown, 
						"vpeSaveCfgOption", saveCfgCmd);
    } 


    XtVaCreateManagedWidget
	    ("optionSeparator", xmSeparatorWidgetClass, pulldown, NULL);

    this->loadMacroOption =
	new ButtonInterface(pulldown, 
                            "vpeLoadMacroOption", theDXApplication->loadMacroCmd);
    this->loadMDFOption =
	new ButtonInterface(pulldown, 
                            "vpeLoadMDFOption", theDXApplication->loadMDFCmd);

    XtVaCreateManagedWidget
	    ("optionSeparator", xmSeparatorWidgetClass, pulldown, NULL);

    this->printProgramOption = new ButtonInterface(pulldown, 
                                             "vpePrintProgramOption", 
                                             this->printProgramCmd);
    if (this->saveAsCCodeCmd)
	this->saveAsCCodeOption = new ButtonInterface(pulldown, 
                                             "vpeSaveAsCCodeOption", 
                                             this->saveAsCCodeCmd);
    XtVaCreateManagedWidget
	    ("optionSeparator", xmSeparatorWidgetClass, pulldown, NULL);

    if (this->isAnchor() && theDXApplication->appAllowsExitOptions())
    	this->quitOption =
	   new ButtonInterface(pulldown,"quitOption",theDXApplication->exitCmd);
    else 
    	this->closeOption =
	   new ButtonInterface(pulldown,"vpeCloseOption",this->closeCmd);

    XtAddCallback(pulldown,
                  XmNmapCallback,
                  (XtCallbackProc)EditorWindow_FileMenuMapCB,
                  (XtPointer)this);
}


void EditorWindow::createEditMenu(Widget parent)
{
    ASSERT(parent);

    Widget pulldown;
    CascadeMenu *cascade_menu;
    Widget menu_parent;
    CommandInterface *ci;

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

    this->undoOption = new ButtonInterface (pulldown, "vpeUndoOption", this->undoCmd);
    XtVaCreateManagedWidget ("optionSeparator", xmSeparatorWidgetClass, pulldown, NULL);

    //
    // Module level functions 
    //
    this->valuesOption =
	new ButtonInterface(pulldown, "vpeValuesOption", this->valuesCmd);

    this->findToolOption =
	new ButtonInterface(pulldown, "vpeFindToolOption", this->findToolCmd);


    //
    // Add/removing tabs 
    //
#if 0
    XtVaCreateManagedWidget("optionSeparator", xmSeparatorWidgetClass, 
						pulldown, NULL);
#endif
    cascade_menu = this->editTabsCascade = 
		new CascadeMenu("vpeEditTabsCascade",pulldown);
    menu_parent = cascade_menu->getMenuItemParent();


    this->addInputTabOption = new ButtonInterface(menu_parent, "vpeAddInputTabOption",
					this->addInputTabCmd);
    cascade_menu->appendComponent(this->addInputTabOption);

    this->removeInputTabOption = new ButtonInterface(menu_parent, "vpeRemoveInputTabOption", 
					this->removeInputTabCmd);
    cascade_menu->appendComponent(this->removeInputTabOption);
    ci = new ButtonInterface(menu_parent, "vpeAddOutputTabOption",
					this->addOutputTabCmd);
    cascade_menu->appendComponent(ci);

    ci = new ButtonInterface(menu_parent, "vpeRemoveOutputTabOption", 
					this->removeOutputTabCmd);
    cascade_menu->appendComponent(ci);

    ci = new ButtonInterface(menu_parent, "vpeRevealAllTabsOption",
					this->revealAllTabsCmd);
    cascade_menu->appendComponent(ci);

    ci = new ButtonInterface(menu_parent, "vpeHideAllTabsOption",
					this->hideAllTabsCmd);
    cascade_menu->appendComponent(ci);

#ifndef FORGET_GETSET
    //
    // GetSet Conversion operation
    //
    cascade_menu = this->programVerifyCascade = 
		new CascadeMenu("programVerifyCascade",pulldown);
    menu_parent = cascade_menu->getMenuItemParent();

    ci = new ButtonInterface (menu_parent, "getSetConversion", this->postGetSetCmd);
    cascade_menu->appendComponent(ci);

    ci = new ButtonInterface (menu_parent, "setToLocal", this->toLocalCmd);
    cascade_menu->appendComponent(ci);

    ci = new ButtonInterface (menu_parent, "setToGlobal", this->toGlobalCmd);
    cascade_menu->appendComponent(ci);
#endif

    //
    // Module selection methods 
    //
#if 0
    XtVaCreateManagedWidget("optionSeparator", xmSeparatorWidgetClass, 
						pulldown, NULL);
#endif

    this->editSelectCascade = 
    cascade_menu = new CascadeMenu("vpeEditSelectCascade",pulldown);
    menu_parent = cascade_menu->getMenuItemParent();

    ci = new ButtonInterface(menu_parent, "vpeSelectAllOption", 
						this->selectAllNodeCmd);
    cascade_menu->appendComponent(ci);

    ci = new ButtonInterface(menu_parent, "vpeSelectConnectedOption", 
					this->selectConnectedNodeCmd);
    cascade_menu->appendComponent(ci);

    ci = new ButtonInterface(menu_parent, "vpeSelectUnconnectedOption", 
					this->selectUnconnectedNodeCmd);
    cascade_menu->appendComponent(ci);

    ci = new ButtonInterface(menu_parent, "vpeSelectUpwardOption", 
					this->selectUpwardNodeCmd);
    cascade_menu->appendComponent(ci);

    ci = new ButtonInterface(menu_parent, "vpeSelectDownwardOption", 
					this->selectDownwardNodeCmd);
    cascade_menu->appendComponent(ci);

    ci = new ButtonInterface(menu_parent, "vpeDeselectAllOption", 
					this->deselectAllNodeCmd);
    cascade_menu->appendComponent(ci);

    ci = new ButtonInterface(menu_parent, "vpeSelectUnselectedOption", 
					this->selectUnselectedNodeCmd);
    cascade_menu->appendComponent(ci);

    ci = new ButtonInterface (menu_parent, "vpeShowExecutedOption", 
					this->showExecutedCmd);
    cascade_menu->appendComponent(ci);

    //
    // Output Cacheability (two cascade menus) 
    //
    this->outputCacheabilityCascade = 
    cascade_menu = new CascadeMenu("vpeOutputCacheabilityCascade",pulldown);
    menu_parent = cascade_menu->getMenuItemParent();

    ci =  new ButtonInterface(menu_parent, "vpeOptimizeCacheability",
					this->optimizeCacheabilityCmd);
    cascade_menu->appendComponent(ci);

    // Set 
    this->editOutputCacheabilityCascade = 
    cascade_menu = new CascadeMenu("vpeEditOutputCacheabilityCascade",
							menu_parent);
    menu_parent = cascade_menu->getMenuItemParent();

    ci = new ButtonInterface(menu_parent, "vpeCacheAllOutputsOption", 
						this->cacheAllOutputsCmd);
    cascade_menu->appendComponent(ci);

    ci = new ButtonInterface(menu_parent, "vpeCacheLastOutputsOption", 
						this->cacheLastOutputsCmd);
    cascade_menu->appendComponent(ci);

    ci = new ButtonInterface(menu_parent, "vpeCacheNoOutputsOption", 
						this->cacheNoOutputsCmd);
    cascade_menu->appendComponent(ci);
    // Show 
    menu_parent = this->outputCacheabilityCascade->getMenuItemParent();
    cascade_menu = new CascadeMenu("vpeShowOutputCacheabilityCascade",
							menu_parent);
    this->outputCacheabilityCascade->appendComponent(cascade_menu); 
    menu_parent = cascade_menu->getMenuItemParent();

    ci = new ButtonInterface(menu_parent, "vpeShowCacheAllOutputsOption", 
						this->showCacheAllOutputsCmd);
    cascade_menu->appendComponent(ci);

    ci = new ButtonInterface(menu_parent, "vpeShowCacheLastOutputsOption", 
						this->showCacheLastOutputsCmd);
    cascade_menu->appendComponent(ci);

    ci = new ButtonInterface(menu_parent, "vpeShowCacheNoOutputsOption", 
						this->showCacheNoOutputsCmd);
    cascade_menu->appendComponent(ci);

    //
    // Delete, Cut, Copy, Paste
    //
    XtVaCreateManagedWidget("optionSeparator", xmSeparatorWidgetClass, 
						pulldown, NULL);
    this->deleteOption =
	new ButtonInterface(pulldown, "vpeDeleteOption", this->deleteNodeCmd);

    if ((theDXApplication->appAllowsSavingNetFile()) &&
	(theDXApplication->appAllowsSavingCfgFile()) &&
	(theDXApplication->appAllowsEditorAccess())) {
	this->copyOption =
	    new ButtonInterface(pulldown, "vpeCopyOption", this->copyNodeCmd);

	this->cutOption =
	    new ButtonInterface(pulldown, "vpeCutOption", this->cutNodeCmd);

	this->pasteOption =
	    new ButtonInterface(pulldown, "vpePasteOption", this->pasteNodeCmd);
    }

    //
    // Annotation
    //
    XtVaCreateManagedWidget("optionSeparator", xmSeparatorWidgetClass, 
						pulldown, NULL);

    this->addAnnotationOption = new ButtonInterface(pulldown, "vpeAddDecorator",
	this->addAnnotationCmd);

    //
    // Miscellaneous 
    //
    this->insertNetworkOption =
	new ButtonInterface(pulldown, "vpeInsertNetOption",
			    this->insertNetCmd);

    this->createMacroOption =
	new ButtonInterface(pulldown, "vpeCreateMacroOption",
			    this->macroifyCmd);

#if WORKSPACE_PAGES
    this->pageCascade = cascade_menu = new CascadeMenu("vpePageCascade", pulldown);
    menu_parent = cascade_menu->getMenuItemParent();

    ci = new ButtonInterface(menu_parent, "vpeCreatePageOption", this->pagifyCmd);
    cascade_menu->appendComponent(ci);

    ci=new ButtonInterface(menu_parent, "vpeSelectedPageOption", this->pagifySelectedCmd);
    cascade_menu->appendComponent(ci);

    ci = new ButtonInterface(menu_parent, "vpeDeletePageOption", this->deletePageCmd);
    cascade_menu->appendComponent(ci);

    XtVaCreateManagedWidget("optionSeparator", xmSeparatorWidgetClass, menu_parent, NULL);

    ci=new ButtonInterface(menu_parent, "vpeChopPageOption", this->autoChopSelectedCmd);
    cascade_menu->appendComponent(ci);

    ci=new ButtonInterface(menu_parent, "vpeFusePageOption", this->autoFuseSelectedCmd);
    cascade_menu->appendComponent(ci);

    XtVaCreateManagedWidget("optionSeparator", xmSeparatorWidgetClass, menu_parent, NULL);

    ci = new ButtonInterface(menu_parent,"vpeConfigurePageOption",this->configurePageCmd);
    cascade_menu->appendComponent(ci);

    ci = new ButtonInterface(menu_parent, "vpeMoveToPageOption", this->moveSelectedCmd);
    cascade_menu->appendComponent(ci);
#endif

    if (this->javifyNetCmd) {
	this->javaCascade = cascade_menu = new CascadeMenu("vpeJavaCascade", pulldown);
	menu_parent = cascade_menu->getMenuItemParent();

	ci = new ButtonInterface(menu_parent, 
	    "vpeJavifyNetOption", this->javifyNetCmd);
	cascade_menu->appendComponent(ci);

	ci = new ButtonInterface(menu_parent, 
	    "vpeUnjavifyNetOption", this->unjavifyNetCmd);
	cascade_menu->appendComponent(ci);

	Command* cmd = NUL(Command*);
	cmd = this->network->getSaveWebPageCommand();
	if (cmd) {
	    XtVaCreateManagedWidget("optionSeparator", 
		xmSeparatorWidgetClass, menu_parent, NULL);
	    ci = new ButtonInterface(menu_parent, "vpeSaveWebPageOption", cmd);
	    cascade_menu->appendComponent(ci);
	}

	cmd = this->network->getSaveAppletCommand();
	if (cmd) {
	    ci = new ButtonInterface(menu_parent, "vpeSaveAppletOption", cmd);
	    cascade_menu->appendComponent(ci);
	}

	cmd = this->network->getSaveBeanCommand();
	if (cmd) {
	    ci = new ButtonInterface(menu_parent, "vpeSaveBeanOption", cmd);
	    cascade_menu->appendComponent(ci);
	}

    }

    this->macroNameOption =
	new ButtonInterface(pulldown, "vpeMacroNameOption",
			    this->network->getSetNameCommand());

    this->reflowGraphOption =
	new ButtonInterface(pulldown, "vpeReflowGraphOption",
			    this->reflowGraphCmd);

    this->createProcessGroupOption =
	new ButtonInterface(pulldown, "vpeCreateProcessGroupOption", 
			    this->createProcessGroupCmd);

    this->commentOption =
	new ButtonInterface(pulldown, "vpeCommentOption", this->editCommentCmd);

    XtAddCallback(pulldown,
                  XmNmapCallback,
                  (XtCallbackProc)EditorWindow_EditMenuMapCB,
                  (XtPointer)this);
}


void EditorWindow::createWindowsMenu(Widget parent)
{
    ASSERT(parent);

    Widget            pulldown;

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

    this->newControlPanelOption =
	new ButtonInterface
	    (pulldown, "vpeNewControlPanelOption", this->newControlPanelCmd);

    this->openControlPanelOption =
	new ButtonInterface(pulldown, "vpeOpenControlPanelOption", 
		this->openControlPanelCmd);

    this->openAllControlPanelsOption =
        new ButtonInterface
            (pulldown,
             "vpeOpenAllControlPanelsOption",
             this->network->getOpenAllPanelsCommand());

    this->openControlPanelByNameMenu =
                new CascadeMenu("vpePanelCascade",pulldown);
	
    XtAddCallback(pulldown,
                  XmNmapCallback,
                  (XtCallbackProc)EditorWindow_WindowMenuMapCB,
                  (XtPointer)this);

#ifdef PANEL_GROUP_SEPARATED
    this->panelGroupPulldown =
            XmCreatePulldownMenu
                (pulldown, "panelGroupPulldown", NUL(ArgList), 0);

    this->panelGroupCascade =
    cascade =  XtVaCreateManagedWidget
            ("vpePanelGroupCascade",
             xmCascadeButtonWidgetClass,
             pulldown,
             XmNsubMenuId, this->panelGroupPulldown,
	     XmNsensitive, TRUE,
             NULL);
#endif

    XtVaCreateManagedWidget("optionSeparator", 
				xmSeparatorWidgetClass, pulldown, NULL);

    this->openMacroOption =
	new ButtonInterface(pulldown, "vpeOpenMacroOption",
			    this->openMacroCmd);

    this->openImageOption =
	new ButtonInterface(pulldown, "vpeOpenImageOption",
			    this->openImageCmd);

    this->openColormapEditorOption =
	new ButtonInterface
	    (pulldown, "vpeOpenColormapEditorOption", this->openColormapCmd);

    this->messageWindowOption =
        new ButtonInterface
            (pulldown, "vpeMessageWindowOption",
	     theDXApplication->messageWindowCmd);


}


void EditorWindow::createOptionsMenu(Widget parent)
{
    ASSERT(parent);

    Widget            pulldown;
    
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

    this->toolPalettesOption =
	new ToggleButtonInterface
	    (pulldown,
	     "vpeToolPalettesOption",
	     this->toolPanelCmd,
	     this->panelVisible);

    this->hitDetectionOption =
	new ToggleButtonInterface
	    (pulldown,
	     "vpeHitDetectionOption",
	     this->hitDetectionCmd,
	     this->hit_detection);

    this->panelAccessOption =
	new ButtonInterface(pulldown, "vpePanelAccessOption", this->setPanelAccessCmd);

    this->panelGroupOption =
	new ButtonInterface(pulldown, "vpePanelGroupOption", this->setPanelGroupCmd);

    this->gridOption =
	new ButtonInterface(pulldown, "vpeGridOption", this->gridCmd);

    XtAddCallback(pulldown, XmNmapCallback, (XtCallbackProc)
	EditorWindow_OptionsMenuMapCB, (XtPointer)this);
}

void EditorWindow::createHelpMenu(Widget parent)
{
    ASSERT(parent);

    this->DXWindow::createHelpMenu(parent);

    XtVaCreateManagedWidget("separator", xmSeparatorWidgetClass, 
                                        this->helpMenuPulldown,
                                        NULL);

    this->onVisualProgramOption =
        new ButtonInterface(this->helpMenuPulldown, "vpeOnVisualProgramOption", 
				this->network->getHelpOnNetworkCommand());
}


void EditorWindow::createMenus(Widget parent)
{
    ASSERT(parent);

    //
    // Create the menus.
    //
    this->createFileMenu(parent);
    this->createEditMenu(parent);
    this->createExecuteMenu(parent);
    this->createWindowsMenu(parent);
    this->createConnectionMenu(parent);
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


void EditorWindow::toggleToolPanel()
{
    Widget toolpanel = this->toolSelector->getRootWidget();

    this->panelVisible = NOT this->panelVisible;
    if (!this->panelVisible) {
	//
	// Unmanage the control panel.
	//
	XtUnmanageChild(toolpanel);
	XtVaSetValues (this->scrolledWindow,
	     XmNleftAttachment, XmATTACH_FORM,
	     XmNleftOffset,     0,
	NULL);
#if WORKSPACE_PAGES
	XtVaSetValues (this->pageSelector->getRootWidget(),
	    XmNleftAttachment, XmATTACH_FORM, XmNleftOffset, 0, NULL);
#endif
    } else {
	//
	// Manage the control panel.
	//
	XtVaSetValues (this->scrolledWindow,
	     XmNleftAttachment, XmATTACH_WIDGET,
	     XmNleftWidget,     toolpanel,
	     XmNleftOffset,     5,
	     NULL);
	XtManageChild(toolpanel);
#if WORKSPACE_PAGES
	XtVaSetValues (this->pageSelector->getRootWidget(),
	    XmNleftAttachment, XmATTACH_WIDGET, XmNleftOffset, 5,
	    XmNleftWidget, toolpanel, NULL);
#endif
    }
}

extern "C" void EditorWindow_EditMenuMapCB(Widget widget,
                                   XtPointer clientdata,
                                   XtPointer calldata)
{

    // 
    // We do this here, because the editor does not get notified
    // when a node has the number repeatable inputs reduced to zero
    // and so can't grey out the 'Remove input' option.  This same argument
    // works for other add/remove repeatable input/output options.
    // 
    EditorWindow *editor = (EditorWindow*)clientdata;
    editor->deferrableCommandActivation->requestAction(NULL); 

    //
    // {de}activate the paste option based on selection ownership.
    // setCommandActivation() isn't really the right place for these
    // because there is no event or notification received when some other
    // vpe establishes or loses selection ownership.
    //
    Display *d = XtDisplay(widget);
    Atom file_atom = XmInternAtom (d, NET_ATOM, True);
    if (file_atom) {
	Window win = XGetSelectionOwner (d, file_atom);
	if (win == None) editor->pasteNodeCmd->deactivate();
	else editor->pasteNodeCmd->activate();
    } else 
	editor->pasteNodeCmd->deactivate();
}
extern "C" void EditorWindow_FileMenuMapCB(Widget widget,
                                   XtPointer clientdata,
                                   XtPointer calldata)
{
    EditorWindow *editor = (EditorWindow*)clientdata;

    if(editor->isAnchor() )
    {
	if (editor->quitOption) {
	    if(editor->network->saveToFileRequired())
		((ButtonInterface*)editor->quitOption)->setLabel("Quit...");
	    else
		((ButtonInterface*)editor->quitOption)->setLabel("Quit");
 	}
    }
    else
    {
        editor->newOption->deactivate();
        editor->openOption->deactivate();
        editor->loadMDFOption->deactivate();
        editor->loadMacroOption->deactivate();
    }

}

extern "C" void EditorWindow_WindowMenuMapCB(Widget widget,
                                   XtPointer clientdata,
                                   XtPointer calldata)
{
     EditorWindow *editor = (EditorWindow*)clientdata;

    Network *network = editor->network;
    CascadeMenu *menu = editor->openControlPanelByNameMenu;
    PanelAccessManager *panelMgr = network->getPanelAccessManager();
    network->fillPanelCascade(menu, panelMgr);
}

extern "C" 
void EditorWindow_OptionsMenuMapCB(Widget widget, XtPointer clientdata, XtPointer )
{
    EditorWindow *editor = (EditorWindow*)clientdata;
    EditorWorkSpace* ews = editor->workSpace;
    WorkSpaceInfo* info = ews->getInfo();
    ToggleButtonInterface* tbi = (ToggleButtonInterface*)
	editor->hitDetectionOption;
    editor->hit_detection = info->getPreventOverlap();
    tbi->setState(editor->hit_detection);
}

void EditorWindow::openSelectedINodes()
{
     Node *node;
     List *snlist = this->makeSelectedNodeList();

     if(NOT snlist)
	return;

     ListIterator li(*snlist);

     while( (node = (Node*)li.getNext()) )
	if(node->isA(ClassInteractorNode))
	    ((InteractorNode*)node)->openControlPanels(this->getRootWidget());

     delete snlist;
}

#if WORKSPACE_PAGES
EditorWorkSpace*
EditorWindow::getPage (const char *pageName)
{
    EditorWorkSpace *page = 
	(EditorWorkSpace*)this->pageSelector->findDefinition(pageName);

    //
    // If the page hasn't been created yet, then find its info from the
    // page manager.
    //
    if (!page) {
	PageGroupManager *pmgr = (PageGroupManager*)
	    this->network->getGroupManagers()->findDefinition(PAGE_GROUP);
	PageGroupRecord  *prec = (PageGroupRecord*)pmgr->getGroup (pageName);
	ASSERT(prec);
	page = (EditorWorkSpace*)this->workSpace->addPage();
	ASSERT (this->pageSelector->addDefinition (pageName, page));
    }

    return page;
}
#endif

void EditorWindow::newNode(Node *n, EditorWorkSpace *where)
{
    StandIn      *si;
    List         *arcs;
    Ark          *a;
    int           icnt, i, j;
    const char   *cp;

    si = n->getStandIn();

    //
    // We don't want to always create a new StandIn, because when 
    // undeleting a node, the standin already exists. 
    //
    if (!si) {
	if (where) {
	    n->newStandIn(where);
#if WORKSPACE_PAGES
	    GroupRecord* grec = this->getGroupOfWorkSpace(where);
	    Symbol page_sym = theSymbolManager->getSymbol(PAGE_GROUP);
	    if (grec) n->setGroupName(grec, page_sym);
#endif
	}
#if WORKSPACE_PAGES
	else if ( (cp = n->getGroupName(theSymbolManager->getSymbol(PAGE_GROUP))) ) {
	    EditorWorkSpace *page = this->getPage(cp);
	    ASSERT(page);
	    //
	    // If page is the current page, then make a standin.
	    //
	    int page_num = this->workSpace->getCurrentPage();
	    EditorWorkSpace* ews = this->workSpace;
	    if (page_num) 
		ews = (EditorWorkSpace*)this->workSpace->getElement(page_num);
	    if (ews == page) 
		n->newStandIn(page);
	    else
		page->setMembersInitialized(FALSE);
	} else {
	    int page_num = this->workSpace->getCurrentPage();
	    EditorWorkSpace* page = this->workSpace;
	    if (page_num) 
		page = (EditorWorkSpace*)this->workSpace->getElement(page_num);
	    GroupRecord* grec = this->getGroupOfWorkSpace(page);
	    Symbol page_sym = theSymbolManager->getSymbol(PAGE_GROUP);
	    if (grec) n->setGroupName(grec, page_sym);
	    n->newStandIn(page);
	}
#else
	else
	    n->newStandIn(this->workSpace);
#endif
        si = n->getStandIn();
	if (si) {
	    icnt = n->getInputCount();
	    for (i=1 ; i<=icnt ; i++) {
		arcs = (List *)n->getInputArks(i);
		for (j = 1; (a = (Ark*) arcs->getElement(j)); j++) 
		    this->notifyArk(a);
		si->drawTab(i, FALSE);     
	    }
	}
    } else {
	si->manage();
	si->drawArks(n);
    }

    if (this->workSpace->isManaged())
        this->deferrableCommandActivation->requestAction(NULL); 
}


void EditorWindow::newNetwork()
{
    Node *node;
    ListIterator iterator;

    this->prepareForNewNetwork();

    // This is in case this is the first program being displayed.
    if (this->workSpace->isManaged())
	this->workSpace->unmanage();

    FOR_EACH_NETWORK_NODE(this->network, node, iterator) {
	this->newNode(node, NUL(EditorWorkSpace*));
    }

    this->deferrableCommandActivation->requestAction(NULL); 

    this->workSpace->manage();

    this->completeNewNetwork();
}

void EditorWindow::manage()
{
    this->DXWindow::manage();
    if (this->initialNetwork)
    {
	this->newNetwork();
	this->initialNetwork = FALSE;
    }
    Decorator *dec;
    ListIterator it;


    FOR_EACH_NETWORK_DECORATOR(this->network,dec,it) {
#if WORKSPACE_PAGES
	const char* cp;
	VPEAnnotator *vpea = (VPEAnnotator*)dec;
	if ( (cp = vpea->getGroupName(theSymbolManager->getSymbol(PAGE_GROUP))) ) {
	    EditorWorkSpace *page = this->getPage(cp);
	    ASSERT(page);
	} else
#endif
	{
	    int page_num = this->workSpace->getCurrentPage();
	    EditorWorkSpace* page = this->workSpace;
	    if (page_num) 
		page = (EditorWorkSpace*)this->workSpace->getElement(page_num);
	    if (!dec->isManaged())
		dec->manage (page);
	}
    }
}

//
// Return a pointer to the editor's notion of the 'current' ControlPanel. 
// The return panel may or may not be managed.
// 
ControlPanel *EditorWindow::getCurrentControlPanel(void)
{
    if (!this->currentPanel) 
        this->currentPanel = this->network->newControlPanel();

    return this->currentPanel;
}


//
// Implements the 'Values' command. 
//
void EditorWindow::showValues()
{
    this->openSelectedNodesCDB();
}
//
// Implements the 'Add/Remove Input/Output Tab' commands.
// If 'adding' is TRUE, we ADD either inputs or outputs, otherwise 
// REMOVE.
// If 'input' is true, we operate on the input parameters, otherwise 
// the outputs.
//
boolean EditorWindow::editSelectedNodeTabs(boolean adding, boolean input)
{
    List *l = this->makeSelectedNodeList();
    if (l == NULL || l->getSize() == 0) {
	if (l) delete l;
	return TRUE;
    }

    Node *node; 
    boolean r = TRUE;
    ListIterator li(*l);

    boolean added_separator = FALSE;
    while ( (node = (Node*)li.getNext()) )
    {
	if (!this->performing_undo) {
	    if (!added_separator) {
		this->undo_list.push (new UndoSeparator(this));
		added_separator = TRUE;
	    }
	    this->undo_list.push (new UndoRepeatableTab(this, node, adding, input));
	}
	if (adding) {
	    if (input && node->isInputRepeatable()) {
		r = r && node->addInputRepeats();
	    } else if (!input && node->isOutputRepeatable()) {
		r = r && node->addOutputRepeats();
	    }
	} else {
	    if (input && node->hasRemoveableInput()) {
		r = r && node->removeInputRepeats();
	    } else if (!input && node->hasRemoveableOutput()) {
		r = r && node->removeOutputRepeats();
	    }
	}
    }

    if (r && !input)
        this->deferrableCommandActivation->requestAction(NULL); 

    delete l;
 
    // this->setCommandActivation() doesn't run often enough to deal
    // with accelerators.
    if ((added_separator) && (!this->undoCmd->isActive()))
	this->setUndoActivation();

    return r;
}

boolean EditorWindow::setSelectedNodeTabVisibility(boolean v)
{
    List *l = this->makeSelectedNodeList();
    if (l == NULL || l->getSize() == 0) {
	if (l) delete l;
	return TRUE;
    }

    ListIterator li(*l);
    Node *node; 
#if WORKSPACE_PAGES
    this->beginPageChange();
#else
    this->beginNetworkChange();
#endif
    boolean added_separator = FALSE;
    while( (node = (Node*)li.getNext()) )
    {
	node->setAllInputsVisibility(v);
	node->setAllOutputsVisibility(v);
    }
    delete l;
#if WORKSPACE_PAGES
    this->endPageChange();
#else
    this->endNetworkChange();
#endif
 
    return TRUE;
}

//
// Do node selection.
//
boolean EditorWindow::selectConnection(int dir, boolean connected)
{
    List *l = this->makeSelectedNodeList();

    if (l == NULL || l->getSize() == 0) {
	if (l) delete l;
	return FALSE;
    }

    ListIterator li(*l);
    ListIterator iterator;
    Node *node, *snode;
    List *nlist = &this->network->nodeList;
    int nsize = nlist->getSize();
    int i, pos;
    boolean *mark = new boolean[nsize];

    if (connected)
	this->deselectAllNodes();
    else
	this->selectAllNodes();

    while((snode = (Node*)li.getNext())
	  AND (pos = nlist->getPosition((const void*)snode)))
    {
      	for(i=0; i<nsize; i++)
             mark[i] = FALSE;

	/*mnodes =*/ this->network->connectedNodes(mark, pos-1, dir);

	pos = 0;
	FOR_EACH_NETWORK_NODE(this->network, node, iterator)
	{
	     if(mark[pos++])
	     	node->getStandIn()->setSelected(connected);
	}
    }
    delete mark;
    delete l;

    return TRUE;

}


//
// Implements the 'Select Downward' command.
//
boolean EditorWindow::selectDownwardNodes()
{
    return this->selectConnection(1, TRUE);
}
//
// Implements the 'Select Upward' command.
//
boolean EditorWindow::selectUpwardNodes()
{
    return this->selectConnection(-1, TRUE);
}
//
// Implements the 'Select Connected' command.
//
boolean EditorWindow::selectConnectedNodes()
{
    return this->selectConnection(0, TRUE);
}
//
// Implements the 'Selected Unconnected' command.
//
boolean EditorWindow::selectUnconnectedNodes()
{
    return this->selectConnection(0, FALSE);
}

//
// look through the network list for selected nodes
// and open the configuration dialog associated with
// the node.
// 
#define MAX_SANE_CDBS 5
void EditorWindow::openSelectedNodesCDB()
{
    Node *node;
    StandIn *standIn;
    ListIterator iterator;
    int selected_count = 0;

    //
    // The sanity check is to prevent opening too many cdbs at once.  This path
    // results from Ctrl-F in the vpe and is seldom used incorrectly.  The real
    // problem is double clicking on a node in the vpe.  But when we go down that
    // path in the code, we don't know if we'll open a cdb or something else so
    // it's hard to count them up first.
    //
    FOR_EACH_NETWORK_NODE(this->network, node, iterator) {
        standIn = node->getStandIn();
#if WORKSPACE_PAGES
	if (!standIn) continue;
#endif
        if (standIn->isSelected()) 
	    selected_count++;
    }
    //
    // Sanity check
    //
    if (selected_count > MAX_SANE_CDBS) {
	char msg[128];
	sprintf (msg, "Really open %d configuration dialogs?", selected_count);
	int response = theQuestionDialogManager->userQuery(this->getRootWidget(),
	    msg, "Confirm Open CDB", "Yes", "No", NULL, 2
	);
	if (response != QuestionDialogManager::OK) return ;
    }

    FOR_EACH_NETWORK_NODE(this->network, node, iterator) {
        standIn = node->getStandIn();
#if WORKSPACE_PAGES
	if (!standIn) continue;
#endif
        if (standIn->isSelected()) 
	    node->openConfigurationDialog(this->getRootWidget()); 
        
    }

}
//
// look through the network list for selected nodes
// and ask the node to do its default action. 
// 
void EditorWindow::doSelectedNodesDefaultAction()
{
    Node *node;
    StandIn *standIn;
    ListIterator iterator;

    //
    // The sanity check is to prevent opening too many cdbs at once.  This path
    // results from a double click in the vpe and is often used incorrectly.  
    //
    int cdb_count = 0;
    FOR_EACH_NETWORK_NODE(this->network, node, iterator) {
        standIn = node->getStandIn();
#if WORKSPACE_PAGES
	if (!standIn) continue;
#endif
        if (standIn->isSelected()) 
	    if (node->defaultWindowIsCDB())
		cdb_count++;
    }
    //
    // Sanity check
    //
    if (cdb_count > MAX_SANE_CDBS) {
	char msg[128];
	sprintf (msg, "Really open %d configuration dialogs?", cdb_count);
	int response = theQuestionDialogManager->userQuery(this->getRootWidget(),
	    msg, "Confirm Open CDB", "Yes", "No", NULL, 2
	);
	if (response != QuestionDialogManager::OK) return ;
    }

    // 
    // If there are selected interactor nodes, then deselect all 
    // InteractorInstances in the existing control panels.
    //
    FOR_EACH_NETWORK_NODE(this->network, node, iterator) {
        if (node->isA(ClassInteractorNode))  {
	    int count = network->getPanelCount();
	    if (count != 0) {
		int i;
		for (i=1; i<=count ; i++) {
		    ControlPanel *cp = network->getPanelByIndex(i);
		    cp->selectAllInstances(FALSE);
		}
	    }
	    break;
	}
    }


    //
    // Erase any notion of the current control panel so that selected 
    // InteractorNodes can add themselves to what will be a newly created
    // ControlPanel via this->getCurrentControlPanel().
    //
    this->currentPanel = NULL;

    theApplication->setBusyCursor(TRUE);
    FOR_EACH_NETWORK_NODE(this->network, node, iterator) {
        standIn = node->getStandIn();
#if WORKSPACE_PAGES
	if (!standIn) continue;
#endif
        if (standIn->isSelected())  {
	    node->openDefaultWindow(this->getRootWidget()); 
	    ConfigurationDialog *cdb = node->getConfigurationDialog();
	    if (cdb) {
	    	Widget cdbWidget = cdb->getRootWidget();
	    	if (cdbWidget) {
	    		// Transfer Accelerations
	    		TransferAccelerator(cdbWidget, 
	    			this->saveOption->getRootWidget(), "ArmAndActivate");
	    		TransferAccelerator(cdbWidget, 
	    			this->addInputTabOption->getRootWidget(), "ArmAndActivate");
	    		TransferAccelerator(cdbWidget, 
	    			this->removeInputTabOption->getRootWidget(), "ArmAndActivate");
	    		TransferAccelerator(cdbWidget, 
	    			this->executeOnceOption->getRootWidget(), "ArmAndActivate");
	    		TransferAccelerator(cdbWidget, 
	    			this->executeOnChangeOption->getRootWidget(), "ArmAndActivate");
	    		TransferAccelerator(cdbWidget, 
	    			this->endExecutionOption->getRootWidget(), "ArmAndActivate");
	    	}
	    }
	}
    }


    //
    // Open the default windows for any selected decorators.
    //
    Decorator *decor;
    FOR_EACH_NETWORK_DECORATOR(this->network, decor, iterator) {
	if ((decor->isSelected()) && (decor->hasDefaultWindow()))
	    decor->openDefaultWindow();
    }


    theApplication->setBusyCursor(FALSE);

}

int EditorWindow::getNodeSelectionCount(const char *classname)
{
    Node	*node;
    StandIn	*standIn;
    ListIterator iterator;
    int		selected = 0;

    FOR_EACH_NETWORK_NODE(this->network, node, iterator) {
	standIn = node->getStandIn();
	if ((standIn) && (standIn->isSelected() &&
	    ((classname == NULL) || node->isA(classname))))
	    selected++;
    }
    return selected;
}
//
// look through the network list and make a list of selected nodes  of
// the given class.  If classname == NULL, then get all selected nodes.
// If no selected interactors were found then NULL is returned.
// 
List *EditorWindow::makeSelectedNodeList(const char *classname)
{
    List	*l; 
    List *selected = NULL;;

    if (classname == NULL)
	l = &this->network->nodeList;
    else
	l = this->network->makeClassifiedNodeList(classname);

    if (l) {
	ListIterator iterator(*l);
        Node *node;
  	while ( (node = (Node*)iterator.getNext()) ) {
            StandIn *standIn = node->getStandIn();
	    if (standIn && standIn->isSelected()) {
		if (!selected)
		    selected = new List();
	        selected->appendElement((const void*) node);
	    }
	}
	if (classname != NULL)
	    delete l;
    }
    return selected;
}


List *
EditorWindow::makeSelectedDecoratorList()
{
Decorator *decor;
ListIterator it;

    List *selected = NULL;

    FOR_EACH_NETWORK_DECORATOR(this->network, decor, it) {
	if (decor->isSelected()) {
	    if (!selected) selected = new List;
	    selected->appendElement((const void*)decor);
	} 
    }
    return selected;
}

//
// Make a sorted list of modules existing in the VPE.
//
List *EditorWindow::makeModuleList()
{
    Node *node;
    ListIterator li,iterator;
    List   	*l = NUL(List*);
    char *name, *module;
    int  r, i;

    FOR_EACH_NETWORK_NODE(this->network, node, iterator)
    {
	if (!l)
	    l = new List;

	name = (char*)node->getNameString();
	li.setList(*l);
	i = 1; r = 1;
	while( (module = (char*)li.getNext()) )
	{
	    r = strcmp(module, name);
	    if(r >= 0)	break;
	    i++;
	}
	if(r != 0)
	{
	    name = DuplicateString(node->getNameString()); 
	    l->insertElement((const void*)name, i);
	}
    }
    return l;
}

//
// Notify the source and destination standIn that an arc 
// has been added.
//

void EditorWindow::notifyArk(Ark *a, boolean added)
{
    StandIn *standIn;
    Node    *n;
    int      param;

    n = a->getSourceNode(param);
    standIn = n->getStandIn();
#if WORKSPACE_PAGES
    if (!standIn) return ;
#endif
    if (added) {
	standIn->addArk(this, a);
    }
    if ((!this->performing_undo) && (!this->creating_new_network)) {
	this->undo_list.push (new UndoSeparator(this));
	this->undo_list.push (new UndoAddArk(this, a, added));
	// this->setCommandActivation() doesn't run often enough to deal
	// with accelerators.
	if (!this->undoCmd->isActive())
	    this->setUndoActivation();
    }
}

//
// Move the workSpace inside the scrolled window so that the given x,y
// position is at the upper left corner of the scrolled window unless
// centered is TRUE in which case x,y is the center of the scrolled
// window.
//
void EditorWindow::moveWorkspaceWindow(int x, int y, boolean centered)
{
    ASSERT(!centered);// Not implemented yet.

    XmScrolledWindowWidget scrollWindow;
    int         value;
    int         size;
    int         max;
    int         min;
    int         delta;
    int         pdelta;
    int		newValue;

    /*
     * Convert scrolled window widget to its internal form.
     */
    scrollWindow = (XmScrolledWindowWidget)this->getScrolledWindow();

    //
    // Set ScrollBar's value.
    // ...but ensure that the values are legal, making any necessary
    // adjustments.  That way callers (like InsertNetworkDialog) don't 
    // need carnal knowledge of the scrollbars to1 change screen positioning.  
    //
    XmScrollBarGetValues((Widget)scrollWindow->swindow.hScrollBar,
                         &value, &size, &delta, &pdelta);
    XtVaGetValues ((Widget)scrollWindow->swindow.hScrollBar, 
	XmNmaximum, &max, XmNminimum, &min, NULL);

    if (x>= (max-size))
	newValue = max-(size+1);
    else
	newValue = x;
    newValue = MAX(min,newValue);


    XmScrollBarSetValues((Widget)scrollWindow->swindow.hScrollBar,
	newValue, size, delta, pdelta, True);

    XmScrollBarGetValues((Widget)scrollWindow->swindow.vScrollBar,
                         &value, &size, &delta, &pdelta);
    XtVaGetValues ((Widget)scrollWindow->swindow.vScrollBar, 
	XmNmaximum, &max, XmNminimum, &min, NULL);

    if (y>= (max-size))
	newValue = max-(size+1);
    else
	newValue = y;
    newValue = MAX(min,newValue);

    XmScrollBarSetValues((Widget)scrollWindow->swindow.vScrollBar,
	newValue, size, delta, pdelta, True);
}

void EditorWindow::getWorkspaceWindowPos(int *x, int *y)
{
    XmScrolledWindowWidget scrollWindow;
    int         value;
    int         size;
    int         delta;
    int         pdelta;

    scrollWindow = (XmScrolledWindowWidget)this->getScrolledWindow();
    XmScrollBarGetValues((Widget)scrollWindow->swindow.hScrollBar,
			 &value, &size, &delta, &pdelta);
    *x = value;
    XmScrollBarGetValues((Widget)scrollWindow->swindow.vScrollBar,
			 &value, &size, &delta, &pdelta);
    *y = value;
    return ;
}

//
// moveWorkspaceWindow() used to do some side effect stuff on behalf
// of the FindToolDialog.  I've moved that into a separate method.  I
// hope it's clearer.  The stuff in moveWorkspaceWindow was broken when
// I added vpe pages because the timing of the operations changed.
//
void EditorWindow::checkPointForFindDialog(FindToolDialog* dialog)
{
    XmScrolledWindowWidget scrollWindow;
    Arg                    arg[8];
    Cardinal               n;
    int                    hScrollValue;
    int                    vScrollValue;

    ASSERT (dialog->isManaged());

    int page = this->workSpace->getCurrentPage();
    EditorWorkSpace *current_ews = this->workSpace;
    if (page) current_ews = (EditorWorkSpace*)this->workSpace->getElement(page);
    GroupRecord* grec = this->getGroupOfWorkSpace(current_ews);
    if (this->find_restore_page)
	delete this->find_restore_page;
    if (grec)
	this->find_restore_page = DuplicateString(grec->getName());
    else
	this->find_restore_page = NUL(char*);

    /*
     * Convert scrolled window widget to its internal form.
     */
    scrollWindow = (XmScrolledWindowWidget)this->getScrolledWindow();
    /*
     * Get drawing window resources.
     */
    n = 0;
    XtSetArg(arg[n], XmNvalue, &hScrollValue); n++;
    XtGetValues((Widget)scrollWindow->swindow.hScrollBar, arg, n);

    n = 0;
    XtSetArg(arg[n], XmNvalue, &vScrollValue); n++;
    XtGetValues((Widget)scrollWindow->swindow.vScrollBar, arg, n);

    /*
     * Store the original position.  The original vpe page
     * was recorded earlier in this method just prior to 
     * selecting the vpe page of the destination node.
     */
    this->Ox = hScrollValue;
    this->Oy = vScrollValue;
}

void EditorWindow::moveWorkspaceWindow(UIComponent* si)
{
    XmScrolledWindowWidget scrollWindow;
    Arg                    arg[8];
    Cardinal               n;
    int                    hSliderSize;
    int                    vSliderSize;
    int                    hScrollValue;
    int                    vScrollValue;
    int                    hScrollMax;
    int                    vScrollMax;
    int                    newOx;
    int                    newOy;


#if WORKSPACE_PAGES
    //
    // Present the proper page for this standin
    //
    // It's also possible to find the proper page using Node methods but we
    // don't ordinarily know the node behind a standin because that's protected info.
    //
    if (si) {
	Widget page_parent = XtParent(si->getRootWidget());
	if (page_parent != this->workSpace->getRootWidget()) {
	    DictionaryIterator di(*this->pageSelector);
	    EditorWorkSpace* ews;
	    while ( (ews = (EditorWorkSpace*)di.getNextDefinition()) ) {
		if (ews->getRootWidget() == page_parent) {
		    ews->unsetRecordedScrollPos();
		    this->workSpace->showWorkSpace(ews);
		    break;
		}
	    }
	}
    }
#endif

    /*
     * Convert scrolled window widget to its internal form.
     */
    scrollWindow = (XmScrolledWindowWidget)this->getScrolledWindow();
    /*
     * Get drawing window resources.
     */
    n = 0;
    XtSetArg(arg[n], XmNvalue, &hScrollValue); n++;
    XtSetArg(arg[n], XmNsliderSize, &hSliderSize); n++;
    XtSetArg(arg[n], XmNmaximum, &hScrollMax); n++;
    XtGetValues((Widget)scrollWindow->swindow.hScrollBar, arg, n);

    n = 0;
    XtSetArg(arg[n], XmNvalue, &vScrollValue); n++;
    XtSetArg(arg[n], XmNsliderSize, &vSliderSize); n++;
    XtSetArg(arg[n], XmNmaximum, &vScrollMax); n++;
    XtGetValues((Widget)scrollWindow->swindow.vScrollBar, arg, n);

    /*
     * Check if the module is already in the clip window.
     */
     
     int xpos, ypos, width, height;
     if (si) {
	 si->getXYSize(&width, &height);
	 si->getXYPosition(&xpos, &ypos);
	 if ( (xpos + width < hSliderSize + hScrollValue) && 
              (ypos + height< vSliderSize + vScrollValue) &&
              (xpos > hScrollValue) &&
              (ypos > vScrollValue))
             return;
    }

    /*
     * Calculate new origin.
     */
    if (!si)                 /* RESTORE */
    {
        newOx = this->Ox;
        newOy = this->Oy;
        this->Ox = -1;
	if (this->find_restore_page) {
	    EditorWorkSpace* ews = (EditorWorkSpace*)
		this->pageSelector->findDefinition (this->find_restore_page);
	    if (ews) this->workSpace->showWorkSpace(ews);
	} else {
	    // It could be that the original was the "Untitled" page in
	    // which case find_restore_page would legitimatly be NUL.
	    // In that situation we can show the root workspace but only
	    // if we first ensure that the user hasn't deleted it.
	    // Anytime the user has deleted the page we're looking for, we
	    // just silently ignore the problem.
	    EditorWorkSpace* ews = (EditorWorkSpace*)
		this->pageSelector->findDefinition ("Untitled");
	    if (ews) this->workSpace->showWorkSpace(ews);
	}
    	this->find_restore_page = NUL(char*);
    }
    else                       /* FIND */
    {
        if ((newOx = xpos + width/2-hSliderSize/2) < 0)
                newOx = 0;
        if ((newOy = ypos + height/2-vSliderSize/2) < 0)
                newOy = 0;
    }

    if (newOx + hSliderSize > hScrollMax)
                newOx = hScrollMax - hSliderSize;

    if (newOy + vSliderSize > vScrollMax)
                newOy = vScrollMax - vSliderSize;

    /*
     * Reset the scrolled window's (x,y) origin.(??????????????)
     */
    scrollWindow->swindow.hOrigin = newOx;
    scrollWindow->swindow.vOrigin = newOy;

    /*
     * Reset the scrolled bars physically.
     */
    XtSetArg(arg[0], XmNvalue, newOx);
    XtSetValues((Widget)scrollWindow->swindow.hScrollBar, arg, 1);
    XtSetArg(arg[0], XmNvalue, newOy);
    XtSetValues((Widget)scrollWindow->swindow.vScrollBar, arg, 1);

    /*
     * Move the work window to reflect the new state.
     */
    XtMoveWidget(scrollWindow->swindow.WorkWindow,-newOx,-newOy);

}

#if WORKSPACE_PAGES
GroupRecord* EditorWindow::getGroupOfWorkSpace (EditorWorkSpace* where)
{
    //
    // If adding to a page, then find out the page name in order to
    // set the group record in the added object.  First fetch the group
    // manager, then find the name of the workspace page by looping over
    // the page dictionary, then fetch the group record from the manager
    // using the name, then set the group record into the object.
    //
    GroupRecord *grec = NUL(GroupRecord*);
    Symbol page_sym = theSymbolManager->getSymbol(PAGE_GROUP);

    const char *page_name = NUL(char*);
    GroupManager *page_mgr = NUL(GroupManager*);
    ASSERT(this->pageSelector);
    int count = this->pageSelector->getSize();
    int i;
    for (i=1; i<=count; i++) {
	if (where == this->pageSelector->getDefinition(i)) {
	    page_name = this->pageSelector->getStringKey(i);
	    break;
	}
    }
    page_mgr = (GroupManager*)
	this->network->getGroupManagers()->findDefinition(page_sym);
    ASSERT(page_mgr);
    if (page_name) {
	if (where != this->workSpace) {
	    ASSERT(page_name);
	}

	//
	// grec could be NULL for the root page when the name is still Untitled.
	//
	grec = page_mgr->getGroup (page_name);
	if (where != this->workSpace) {
	    ASSERT(grec);
	}
    }

    return grec;
}
#endif

//
// Add the current ToolSelector node to the workspace
//
void EditorWindow::addCurrentNode(int x, int y, EditorWorkSpace *where, boolean stitch)
{
    NodeDefinition *nodeDef = this->toolSelector->getCurrentSelection();

    if (nodeDef) {
	this->addNode(nodeDef, x, y, where);
    } else if (this->addingDecorators) {

	// Deselect all currently selected nodes
	this->deselectAllNodes();

	ListIterator it(*this->addingDecorators);
	VPEAnnotator *dec;
	while ( (dec = (VPEAnnotator*)it.getNext()) ) {
	    dec->setXYPosition(x,y);
	    dec->manage(where);
	    dec->setSelected(TRUE);
	    this->network->addDecoratorToList((void*)dec);
#if WORKSPACE_PAGES
	    GroupRecord* grec = this->getGroupOfWorkSpace(where);
	    Symbol page_sym = theSymbolManager->getSymbol(PAGE_GROUP);
	    if (grec)
		dec->setGroupName(grec, page_sym);
#endif
	}
	this->resetCursor();
	this->setCommandActivation();
    } else if (this->pendingPaste) {
	// because of stitching, we might be adding nodes to a page that
	// isn't the current page.  So, make sure the destination page
	// is set to the current page.
	if (stitch) {
	    List *all_nodes = this->pendingPaste->makeClassifiedNodeList(ClassNode);
	    Symbol psym = theSymbolManager->getSymbol(PAGE_GROUP);
	    Node* any_node = NUL(Node*);
	    const char* group_name = NUL(const char*);
	    if ((all_nodes) && (all_nodes->getSize() >= 1)) {
		any_node = (Node*)all_nodes->getElement(1);
		group_name = any_node->getGroupName(psym);
	    }
	    if (!any_node) {
		List* all_decs = (List*)this->pendingPaste->getDecoratorList();
		VPEAnnotator* any_dec = NUL(VPEAnnotator*);
		if ((all_decs) && (all_decs->getSize() >= 1)) {
		    any_dec = (VPEAnnotator*)all_decs->getElement(1);
		    group_name = any_dec->getGroupName(psym);
		}
	    }
	    if (group_name) {
		EditorWorkSpace* ews = (EditorWorkSpace*)
		    this->pageSelector->findDefinition(group_name);
		if (ews != NUL(EditorWorkSpace*)) 
		    this->pageSelector->selectPage(ews);
	    }
	    delete all_nodes;
	}

	// Deselect all currently selected nodes
	this->deselectAllNodes();

	this->pendingPaste->setTopLeftPos (x,y);
	List *l = this->pendingPaste->getNonEmptyPanels();
	this->pagifyNetNodes (this->pendingPaste, where, TRUE);
	boolean tmp = this->creating_new_network;
	this->creating_new_network = TRUE;
	this->network->mergeNetworks (this->pendingPaste, l, TRUE, stitch);
	this->creating_new_network = tmp;
	if (l) delete l;
	this->resetCursor();
	this->setCommandActivation();
    }
    
    //
    // Check these 2 fields for deletion apart from the other if blocks
    // because if you're adding a new node, then you still need to come
    // here and delete these objects.
    //
    if (this->addingDecorators) {
	delete this->addingDecorators;
	this->addingDecorators = 0;
    }
    
    if (this->pendingPaste) {
	delete this->pendingPaste;
	this->pendingPaste = NUL(Network*);
    }
}

//
// Add a node of the specified type to the workspace
//
void EditorWindow::addNode (NodeDefinition *nodeDef, int x, int y, EditorWorkSpace *where)
{
    Node *node;
 
    // Reset the cursor if further placements are not allowed.
    if (!this->toolSelector->isSelectionLocked()) {
	this->resetCursor();	
	this->toolSelector->deselectAllTools();
    }

    node = nodeDef->createNewNode(this->network);
    if (node == NULL)
	return;

#if WORKSPACE_PAGES
    GroupRecord* grec;
    if (where)
	grec = this->getGroupOfWorkSpace(where);
    else {
	EditorWorkSpace *ews;
	int page = this->workSpace->getCurrentPage();
	if (!page) 
	    ews = this->workSpace;
	else
	    ews = (EditorWorkSpace*)this->workSpace->getElement(page);
	grec = this->getGroupOfWorkSpace(ews);
    }
    Symbol page_sym = theSymbolManager->getSymbol(PAGE_GROUP);
    if (grec)
	node->setGroupName(grec, page_sym);
#endif
    
    node->setVpePosition(x,y);
    this->network->addNode(node, where);

    // Deselect all currently selected nodes
    this->deselectAllNodes();

    // Select the added node 
    StandIn *si = node->getStandIn();
    ASSERT(si);
    si->setSelected(TRUE);

    if(this->findToolDialog AND this->findToolDialog->isManaged())
    	this->findToolDialog->changeList();

    this->resetExecutionList();
}

void EditorWindow::setCursor(int cursorType)
{

// Just ask the workspace to set the cursor type

    this->workSpace->setCursor(cursorType);
}

//
// reset the EditorWindow cursor to the default.
//
void EditorWindow::resetCursor()
{
    this->workSpace->resetCursor();
}


//
// Create a list of nodes to delete, and call deleteNodes with the list.
//
void EditorWindow::removeSelectedNodes()
{
    Node         *node;
    List         *toDelete = NULL;
    StandIn      *si;
    ListIterator iterator;

    this->deferrableCommandActivation->deferAction();

    //
    // Make a copy of the deletions so that the operation can be undone
    //
    if (!this->performing_undo) {
	this->undo_list.push (new UndoSeparator(this));
	this->undo_list.push(
	    new UndoDeletion (this, 
		this->makeSelectedNodeList(NUL(const char*)),
		this->makeSelectedDecoratorList())
	);
	// this->setCommandActivation() doesn't run often enough to deal
	// with accelerators.
	if (!this->undoCmd->isActive())
	    this->setUndoActivation();
    }

    //
    // Build a list of nodes and unmanage the standIns that go with them.
    //
    FOR_EACH_NETWORK_NODE(this->network, node, iterator) {
        si = node->getStandIn();
#if WORKSPACE_PAGES
	if (!si) continue;
#endif
        if (si->isSelected()) {
	    if (!toDelete)
		toDelete = new List;
            toDelete->appendElement(node);
	}
    }
    if (toDelete)
    {
	this->deleteNodes(toDelete);
	delete toDelete;
    }

    //
    // Delete selected decorators.  A separate list isn't required.
    //
    Decorator *decor;
    FOR_EACH_NETWORK_DECORATOR (this->network, decor, iterator) {
	if (decor->isSelected()) {
	    this->network->decoratorList.removeElement((void*)decor);
	    delete decor;
	}
    }

    this->deferrableCommandActivation->undeferAction();

    //this->setUndoActivation();
}

//
// A public interface to deleteNodes... used by Network::replaceInputArks
// so that it can let us do important work when deleting a bunch of nodes.
//
void EditorWindow::removeNodes(List* toDelete)
{
    this->deleteNodes(toDelete);
}

//
// Called by the DeleteCommand to REALLY delete the nodes.
//
void EditorWindow::deleteNodes(List *toDelete)
{
    Node         *node;
    ListIterator iterator;
    Node         *nodePtr;
    int		 deleteCnt = toDelete->getSize();

    //
    // don't toggle line drawing if erasing a small number of nodes.
    // Would it be good to check for lines entering/leaving the selected set?
    // If there were no such lines, then would it be unnecessary to toggle line
    // routing?
    int		threshold = 5;

    //
    // turn off line routing if necessary
    //
    if (deleteCnt > threshold)
	this->beginPageChange();

    //
    // In order to maintain consistency between the yellowing of standins and 
    // page tabs after a run time error, check each deleted node to see if its
    // standin is showing an error.
    //
    boolean errored_node_deleted = FALSE;

    //
    // Perform the deletion
    //
    iterator.setList(*toDelete);
    nodePtr = (Node*)iterator.getNext();
    while (nodePtr != NULL) {
        node = nodePtr;
        nodePtr = (Node*)iterator.getNext();
	StandIn* si = node->getStandIn();
	if ((si) && (this->errored_standins)) 
	    errored_node_deleted|= this->errored_standins->removeElement((void*)si);
        this->network->deleteNode(node);	
    }

    //
    // restart line routing if necessary
    //
    if (deleteCnt > threshold)
	this->endPageChange();
    this->lastSelectedTransmitter = NULL;

    if (errored_node_deleted) {
	//
	// resetErrorList with arg==FALSE means update colors but don't 
	// toss out the list.
	//
	this->resetErrorList(FALSE);
    }

    //
    // Update the find tool dialog
    //
    if(this->findToolDialog AND this->findToolDialog->isManaged())
    	this->findToolDialog->changeList();
}

void EditorWindow::highlightNodes(int h, List *l)
{
    Node *n;
    if (!l)
	l = &this->network->nodeList;

    ListIterator li(*l);
    while ( (n = (Node*)li.getNext()) ) 
	this->highlightNode(n,h);
}

void EditorWindow::highlightNode(const char* name, int instance, int flag)
{
    Node        *node;

    node = this->network->findNode(theSymbolManager->getSymbol(name),instance); 
    if (NOT node)
	return;

    this->highlightNode(node,flag);
}
 
void EditorWindow::highlightNode(Node *n, int flag)
{
    Pixel       color;

    this->executing_node = NUL(Node*);
    StandIn* si = n->getStandIn();
    switch (flag)
    {
	case EditorWindow::ERRORHIGHLIGHT :
	 	color = theDXApplication->getErrorNodeForeground();
		if (!this->errored_standins) this->errored_standins = new List;
		if ((si) && (this->errored_standins->isMember((void*)si) == FALSE))
		    this->errored_standins->appendElement((void*)si);
		break;

	case EditorWindow::EXECUTEHIGHLIGHT :
	 	color = theDXApplication->getExecutionHighlightForeground();
		if (this->executed_nodes == NUL(List*)) 
		    this->executed_nodes = new List;
		if (!this->executed_nodes->isMember((const void*)n))
		    this->executed_nodes->appendElement((const void*)n);
		this->showExecutedCmd->activate();
		this->executing_node = n;
		break;

	case EditorWindow::REMOVEHIGHLIGHT :
	 	color = standInGetDefaultForeground();
		break;

        default:

	        ASSERT(FALSE);
    }

#if WORKSPACE_PAGES
    //
    // Notify the pageSelector to highlight the tab of the page containing
    // the node whose standin we're trying to hightlight.
    //
    Symbol psym = theSymbolManager->getSymbol(PAGE_GROUP);
    EditorWorkSpace *ews = this->workSpace;
    const char* group_name = n->getGroupName(psym);
    if (group_name) {
	EditorWorkSpace *page =
		(EditorWorkSpace*)this->pageSelector->findDefinition(group_name);
	ASSERT(page);
	ews = page;
    }
    this->pageSelector->highlightTab (ews, flag);

    // 
    // If the flag is ERROR, then make sure that the standin does exist
    // because we're going to be moving the vpe there anyway and we need
    // the feedback.
    //
    if ((!si) && (flag == EditorWindow::ERRORHIGHLIGHT)) {
	this->pageSelector->selectPage(ews);
	si = n->getStandIn();
	if (si) {
	    if (!this->errored_standins) this->errored_standins = new List;
	    if (this->errored_standins->isMember((void*)si) == FALSE)
		this->errored_standins->appendElement((void*)si);
	}
    }
    if (si) 
#endif
	si->setLabelColor(color);
}

boolean EditorWindow::selectDecorator (VPEAnnotator* dec, boolean select, boolean warp)
{
    if (dec->getNetwork()->getEditor() != this)
	return FALSE;
    //
    // Select the proper page.
    //
    Symbol psym = theSymbolManager->getSymbol(PAGE_GROUP);
    const char* group_name = dec->getGroupName(psym);
    if ((select) && (warp)) {
	if (group_name) {
	    EditorWorkSpace *page =
		    (EditorWorkSpace*)this->pageSelector->findDefinition(group_name);
	    ASSERT(page);
	    page->unsetRecordedScrollPos();
	    this->pageSelector->selectPage(page);
	    this->populatePage(page);
	    this->workSpace->resize();
	} else {
	    this->workSpace->unsetRecordedScrollPos();
	    this->pageSelector->selectPage(this->workSpace);
	}
    } else {
	//
	// If we're not going to bring the selected node into view,
	// then make sure that the node's page is the current page.
	// If it's not, then silently refuse to select the node.
	//
	boolean wrong_page = FALSE;
	EditorWorkSpace *page = NUL(EditorWorkSpace*);
	if (group_name)
	    page = (EditorWorkSpace*)this->pageSelector->findDefinition(group_name);
	else
	    page = this->workSpace;
	int page_no = this->workSpace->getCurrentPage();
	EditorWorkSpace* current_ews = this->workSpace;
	if (page_no) current_ews = (EditorWorkSpace*)this->workSpace->getElement(page_no);
	if (page != current_ews) wrong_page = TRUE;

	if (wrong_page) 
	    return TRUE;
    }
    dec->setSelected(select);
    if (select && warp)
	this->moveWorkspaceWindow(dec);
    return TRUE;
}

boolean EditorWindow::selectNode(Node *node, boolean select, boolean warp)
{
    StandIn     *standIn;
    ASSERT(node);
    if (node->getNetwork()->getEditor() != this)
	return FALSE;

    standIn = node->getStandIn();
#if WORKSPACE_PAGES
    //
    // Select the proper page.
    //
    Symbol psym = theSymbolManager->getSymbol(PAGE_GROUP);
    const char* group_name = node->getGroupName(psym);
    if ((select) && (warp)) {
	if (group_name) {
	    EditorWorkSpace *page =
		    (EditorWorkSpace*)this->pageSelector->findDefinition(group_name);
	    ASSERT(page);
	    page->unsetRecordedScrollPos();
	    this->pageSelector->selectPage(page);
	    this->populatePage(page);
	    this->workSpace->resize();
	} else {
	    this->workSpace->unsetRecordedScrollPos();
	    this->pageSelector->selectPage(this->workSpace);
	    ASSERT(standIn);
	}
	if (!standIn) {
	    standIn = node->getStandIn();
	}
	ASSERT(standIn);
    } else if (!standIn) {
	return TRUE;
    } else {
	//
	// If we're not going to bring the selected node into view,
	// then make sure that the node's page is the current page.
	// If it's not, then silently refuse to select the node.
	//
	boolean wrong_page = FALSE;
	EditorWorkSpace *page = NUL(EditorWorkSpace*);
	if (group_name)
	    page = (EditorWorkSpace*)this->pageSelector->findDefinition(group_name);
	else
	    page = this->workSpace;
	int page_no = this->workSpace->getCurrentPage();
	EditorWorkSpace* current_ews = this->workSpace;
	if (page_no) current_ews = (EditorWorkSpace*)this->workSpace->getElement(page_no);
	if (page != current_ews) wrong_page = TRUE;

	if (wrong_page) 
	    return TRUE;
    }
    ASSERT(standIn);
#endif
    standIn->setSelected(select);
    if (select && warp)
	this->moveWorkspaceWindow(standIn);
    return TRUE;
    
}

boolean EditorWindow::selectNode(char* name, int instance, boolean select)
{
    Node        *node;

    node = this->network->findNode(theSymbolManager->getSymbol(name),instance); 
    if (NOT node)
	return (FALSE);
    return this->selectNode(node, select, TRUE);
}

#define SELECT_ALL		0
#define DESELECT_ALL		1
#define SELECT_UNSELECTED	2
#define SELECT_ALL_IN_PAGE	3

void EditorWindow::selectUnselectedNodes()
{
    this->selectNodes(SELECT_UNSELECTED);
}
void EditorWindow::selectAllNodes()
{
#if WORKSPACE_PAGES
    this->selectNodes(SELECT_ALL_IN_PAGE);
#else
    this->selectNodes(SELECT_ALL);
#endif
}
void EditorWindow::deselectAllNodes()
{
    this->selectNodes(DESELECT_ALL);
}

void EditorWindow::selectNodes(int how)
{
    ListIterator li(this->network->nodeList);
    Node        *n;
    EditorWorkSpace* page = this->workSpace;
#if WORKSPACE_PAGES
    if (this->network->isDeleted()) return ;
    int page_num = this->workSpace->getCurrentPage();
    if (page_num) page = (EditorWorkSpace*)this->workSpace->getElement(page_num);
#endif

    this->deferrableCommandActivation->deferAction();

    for(n = (Node*)li.getNext(); n; n = (Node*)li.getNext()) {
	StandIn *si = n->getStandIn();
	if (!si) continue;
	switch (how) {
	    case SELECT_ALL:   si->setSelected(TRUE); break;
	    case DESELECT_ALL: si->setSelected(FALSE); break;
	    case SELECT_UNSELECTED: si->setSelected(!si->isSelected()); break;
	    case SELECT_ALL_IN_PAGE: if (si->getWorkSpace() == page) 
					si->setSelected(TRUE); break;
	}
    }

    // Decorators
    Decorator *decor;
    ListIterator iterator;
    FOR_EACH_NETWORK_DECORATOR (this->network, decor, iterator) {
	switch (how) {
	    case SELECT_ALL: 	decor->setSelected(TRUE); break;
	    case DESELECT_ALL: 	decor->setSelected(FALSE); break;
	    case SELECT_UNSELECTED:	decor->setSelected(!decor->isSelected()); break;
	    case SELECT_ALL_IN_PAGE: if (decor->getWorkSpace() == page) 
					decor->setSelected(TRUE); break;
	}
    }
    this->deferrableCommandActivation->undeferAction();
}



void EditorWindow::serverDisconnected()
{
    Pixel runColor = theDXApplication->getExecutionHighlightForeground();
    Pixel defaultColor = standInGetDefaultForeground();

    // For each node, if the standin's forground color is "execute", reset it.
    Node *n;
    ListIterator li(this->network->nodeList);
    while ( (n = (Node*)li.getNext()) ) {
	StandIn *si = n->getStandIn();
#if WORKSPACE_PAGES
	if (!si) continue;
#endif
	if (si->getLabelColor() == runColor)
	    si->setLabelColor(defaultColor);
    }


    this->DXWindow::serverDisconnected();
    this->executing_node = NUL(Node*);
    this->resetColorList();
}

void EditorWindow::postPanelGroupDialog()
{
    if(NOT this->panelGroupDialog)
	this->panelGroupDialog
	    = new ControlPanelGroupDialog(this->getRootWidget());
    this->panelGroupDialog->setData(this->network->getPanelGroupManager());

    this->panelGroupDialog->post();
}

void EditorWindow::postProcessGroupCreateDialog()
{
    Network *net = this->getNetwork();
    Node *n;
    ListIterator iter;
    boolean hasloop = FALSE;

    FOR_EACH_NETWORK_NODE(net,n,iter) {
        NodeDefinition *nd = n->getDefinition();
        hasloop = nd->isMDFFlagLOOP();
        if (hasloop)
            break;
        
    }   

    if (hasloop) {
        InfoMessage("Execution groups, and thus distributed execution, can\n"
		    "not be used in programs that have looping tools at the\n"
		    "top level of the program. If you would like to use \n"
		    "distributed execution, encapsulate your loops within\n"
		    "macros and then create the execution groups.");
    } else {
        if(NOT this->processGroupCreateDialog)
            this->processGroupCreateDialog
                = new ProcessGroupCreateDialog(this);

        this->processGroupCreateDialog->post();
    }
}

#if WORKSPACE_PAGES
void EditorWindow::setGroup(GroupRecord *grec, Symbol groupID)
{
    //
    // Do all Nodes
    //
    List *nlist = this->makeSelectedNodeList();
 
    if (nlist) {
	ListIterator li(*nlist);
	Node *node;

	while( (node = (Node*)li.getNext()) )
	    node->setGroupName(grec, groupID);

	delete nlist;
    }

    //
    // Do all Decorators
    //
    List *seldec = this->makeSelectedDecoratorList();
    if (seldec) {
	ListIterator li(*seldec);
	VPEAnnotator *vpea;
	while ( (vpea = (VPEAnnotator*)li.getNext()) )
	    vpea->setGroupName(grec, groupID);

	delete seldec;
    }

}

void EditorWindow::resetGroup(const char* name, Symbol groupID)
{
    Node *node;
    ListIterator li;

    FOR_EACH_NETWORK_NODE(this->network, node, li)
    {
  	const char *s = node->getGroupName(groupID);
	if(s && EqualString(name, s))
	    node->setGroupName(NUL(GroupRecord*), groupID);
    }

    //
    // Do all Decorators
    //
    Decorator* dec;
    FOR_EACH_NETWORK_DECORATOR(this->network, dec, li) {
	VPEAnnotator *vpea = (VPEAnnotator*)dec;
  	const char *s = vpea->getGroupName(groupID);
	if(s && EqualString(name, s))
	    vpea->setGroupName(NUL(GroupRecord*), groupID);
    }

}

void EditorWindow::selectGroup(const char* name, Symbol groupID)
{
    Node *node, *firstNode = NUL(Node*);
    ListIterator li;

    this->deselectAllNodes();

    FOR_EACH_NETWORK_NODE(this->network, node, li)
    {
  	const char *s = node->getGroupName(groupID);
        if(s && EqualString(name, s))
        {
            if (firstNode == NUL(Node*)) {
            	this->selectNode(node, TRUE, TRUE);
		firstNode = node;
	    } else
            	this->selectNode(node, TRUE, FALSE);
        }
    }

    //
    // Do all Decorators
    //
    Decorator* dec;
    FOR_EACH_NETWORK_DECORATOR(this->network, dec, li) {
	VPEAnnotator *vpea = (VPEAnnotator*)dec;
	const char *group_name = vpea->getGroupName(groupID);
	if ((group_name) && (EqualString(group_name, name))) {
	    vpea->setSelected(TRUE);
	} else {
	    vpea->setSelected(FALSE);
	}
    }

    if(NOT firstNode)
        WarningMessage("Process group %s is empty.", name);
}

void EditorWindow::clearGroup(const char* name, Symbol groupID)
{
    Node *node;
    ListIterator li;

    FOR_EACH_NETWORK_NODE(this->network, node, li)
	node->setGroupName(NULL, groupID);

    //
    // Do all Decorators
    //
    Decorator* dec;
    FOR_EACH_NETWORK_DECORATOR(this->network, dec, li) {
	VPEAnnotator *vpea = (VPEAnnotator*)dec;
	vpea->setGroupName(NUL(GroupRecord*), groupID);
    }
}

//
// Look for the definition of the old group and change dictionary entries so
// that members of that group belong to the new group.  If old_group doesn't
// currently exist, then treat nodes as if they belong if they have no group name.
//
boolean 
EditorWindow::changeGroup (const char* old_group, const char* new_group, Symbol groupID)
{
    GroupManager *gmgr = (GroupManager*)
	this->network->getGroupManagers()->findDefinition(groupID);
    GroupRecord* old_grec = gmgr->getGroup(old_group);

    //
    // If there is an existing group with this name then we'll fail here, but that
    // should have been tested for earlier.
    //
    GroupRecord* new_grec = gmgr->getGroup(new_group);
    if (new_grec) return FALSE;

    //
    // The call to GroupManager::createGroup()  will produce a call
    // to EditorWindow::setGroup().  So, if you're changing a group and something
    // is already selected in the vpe, then that thing will be in the new group
    // as soon as we come back from createGroup.
    //
    if (!gmgr->createGroup (new_group, this->network)) return FALSE;
    ASSERT (new_grec = gmgr->getGroup(new_group));

    Node* node;
    ListIterator li;
    FOR_EACH_NETWORK_NODE(this->network, node, li) {
	const char* group_name = node->getGroupName(groupID);
	if ((group_name) && (EqualString(group_name, old_group))) {
	    node->setGroupName(new_grec, groupID);
	} else if ((!group_name) && (!old_grec)) {
	    node->setGroupName(new_grec, groupID);
	}
    }
    Decorator* dec;
    FOR_EACH_NETWORK_DECORATOR(this->network, dec, li) {
	VPEAnnotator* vpea = (VPEAnnotator*)dec;
	const char* group_name = vpea->getGroupName(groupID);
	if ((group_name) && (EqualString(group_name, old_group))) {
	    vpea->setGroupName(new_grec, groupID);
	} else if ((!group_name) && (!old_grec)) {
	    vpea->setGroupName(new_grec, groupID);
	}
    }
    if (old_grec) {
	ASSERT(gmgr->removeGroup (old_group, this->network));
    }
    return TRUE;
}

#else
void EditorWindow::setProcessGroup(const char* name)
{
    List *nlist = this->makeSelectedNodeList();
 
    if(NOT nlist) return;

    ListIterator li(*nlist);
    Node *node;

    while(node = (Node*)li.getNext())
	node->setGroupName(name);

    delete nlist;
}

void EditorWindow::resetProcessGroup(const char* name)
{
    Node *node;
    ListIterator li;

    FOR_EACH_NETWORK_NODE(this->network, node, li)
    {
  	const char *s = node->getGroupName();
	if(s && EqualString(name, s))
	    node->setGroupName(NULL);
    }
}

void EditorWindow::selectProcessGroup(const char* name)
{
    Node *node, *firstNode = NULL;
    ListIterator li;

    this->deselectAllNodes();

    FOR_EACH_NETWORK_NODE(this->network, node, li)
    {
  	const char *s = node->getGroupName();
        if(s && EqualString(name, s))
        {
            if(firstNode)
            	this->selectNode(node, TRUE, FALSE);
	    else
		firstNode = node; 
        }
    }

    if(NOT firstNode)
        WarningMessage("Process group %s is empty.", name);
    else
	this->selectNode(firstNode, TRUE, TRUE);
}

void EditorWindow::clearProcessGroup(const char* name)
{
    Node *node;
    ListIterator li;

    FOR_EACH_NETWORK_NODE(this->network, node, li)
	node->setGroupName(NULL);
}
#endif

void EditorWindow::installWorkSpaceInfo(WorkSpaceInfo *info)
{
    ASSERT(this->workSpace);
    this->workSpace->installInfo(info);
}

#if WORKSPACE_PAGES
void EditorWindow::beginPageChange()
{
    int page = this->workSpace->getCurrentPage();
    WorkSpace *current_ews = this->workSpace;
    if (page) current_ews = this->workSpace->getElement(page);
    current_ews->beginManyPlacements();
    this->deferrableCommandActivation->deferAction();
    this->resetExecutionList();
}

void EditorWindow::endPageChange()
{
    int page = this->workSpace->getCurrentPage();
    WorkSpace *current_ews = this->workSpace;
    if (page) current_ews = this->workSpace->getElement(page);
    current_ews->endManyPlacements();
    //
    // Don't resize if we need less space.  It's too visually jarring.
    //
    int reqw, w, reqh, h;
    this->workSpace->getMaxWidthHeight(&reqw, &reqh);
    this->workSpace->getXYSize(&w,&h);
    if ((reqw > w) || (reqh > h)) this->workSpace->resize();

    this->deferrableCommandActivation->requestAction(NULL);
    this->deferrableCommandActivation->undeferAction();
}
#endif

//
// Do what ever is necessary just before changing the current network.
// To make things run faster we turn off line routing.  This should
// be followed by a call to endNetworkChange(), which will re-enable
// line routing.
//
void EditorWindow::beginNetworkChange()
{
    WorkSpace *ws = this->workSpace;
    ASSERT(ws);
    ws->beginManyPlacements();
    this->lastSelectedTransmitter = NULL;
    this->deferrableCommandActivation->deferAction();
    this->resetColorList();
}
//
// Do what ever is necessary just after changing to the current network.
// To make things run faster we turn off line routing in beginNetworkChange()
// which should be called before this is called.  
//
void EditorWindow::endNetworkChange()
{
    WorkSpace *ws = this->workSpace;
    ASSERT(ws);
    this->lastSelectedTransmitter = NULL;
    ws->endManyPlacements();

#if WORKSPACE_PAGES
    //
    // All non-empty pages will be created automatically.  Now go and look
    // for empty pages and create them.
    //
    PageGroupManager *pmgr = (PageGroupManager*)
	this->network->getGroupManagers()->findDefinition(PAGE_GROUP);
    int gcnt = pmgr->getGroupCount();
    if (gcnt) {
	int i;
	for (i=1; i<=gcnt; i++) {
	    const char *pageName = pmgr->getGroupName(i);
	    if (this->pageSelector->findDefinition (pageName)) continue;
	    PageGroupRecord  *prec = (PageGroupRecord*)pmgr->getGroup (pageName);
	    ASSERT(prec);
	    EditorWorkSpace *page = (EditorWorkSpace*) this->workSpace->addPage();
	    page->resetCursor();
	    ASSERT (this->pageSelector->addDefinition (pageName, page));
	}
	//
	// If the root page has no members and its name is still Untitled, then
	// remove the it from the page selector since there's no longer
	// a need for it.
	// On the other hande, if the root page has members, then make sure that
	// it's in the PageSelector.
	//
	int cnt = this->getPageMemberCount (this->workSpace);
	if (!cnt) {
	    if (this->network->getFileName()) {
		EditorWorkSpace* ews = (EditorWorkSpace*)
		    this->pageSelector->findDefinition("Untitled");
		if (this->workSpace == ews) {
		    this->pageSelector->removeDefinition ((void*)this->workSpace);

		    //
		    // Try to get the leftmost tab's workspace.
		    //
		    EditorWorkSpace* ews = (EditorWorkSpace*)
			this->pageSelector->getInitialWorkSpace();
		    this->workSpace->showWorkSpace(ews);
		}
	    }
	} else {
	    int i,dsize = this->pageSelector->getSize();
	    boolean found = FALSE;
	    for (i=1; i<=dsize; i++) {
		EditorWorkSpace* ews = (EditorWorkSpace*)
		    this->pageSelector->getDefinition(i);
		if (ews == this->workSpace) {
		    found = TRUE;
		    break;
		}
	    }
	    if (!found) 
		this->pageSelector->addDefinition ("Untitled", this->workSpace);
	}
    }
#endif

    this->deferrableCommandActivation->requestAction(NULL);
    this->deferrableCommandActivation->undeferAction();
}
//
// Do what ever is necessary just before and after reading a new network
// These should be called in pairs.
//
void EditorWindow::prepareForNewNetwork()
{

    if(this->findToolDialog)
	this->findToolDialog->unmanage();   

    if(this->printProgramDialog)
	this->printProgramDialog->unmanage();   

    if(this->saveAsCCodeDialog)
	this->saveAsCCodeDialog->unmanage();   

    if(this->processGroupCreateDialog)
	this->processGroupCreateDialog->unmanage();   

    if(this->panelGroupDialog)
        this->panelGroupDialog->unmanage();

    if(this->panelAccessDialog)
        this->panelAccessDialog->unmanage();

#ifndef FORGET_GETSET
    if ((EditorWindow::GetSetDialog) && (this->isAnchor()) && (this->network))
	EditorWindow::GetSetDialog->unmanage();
#endif

    this->beginNetworkChange();

#if WORKSPACE_PAGES
    ListIterator it(this->network->decoratorList);
    Decorator* dec;
    while ( (dec = (Decorator*)it.getNext()) ) {
	if ((dec->getRootWidget()) && (dec->isManaged())) {
	    dec->unmanage();
	    dec->uncreateDecorator();
	}
    }
    this->destroyStandIns(&this->network->nodeList);
    if (this->pageSelector) {
	int i, pcnt = this->pageSelector->getSize();
	    this->network->getGroupManagers()->findDefinition(PAGE_GROUP);
	for (i=1; i<=pcnt; i++) {
	    EditorWorkSpace *ews = (EditorWorkSpace*)
		this->pageSelector->getDefinition(i);
	    if (ews != this->workSpace) {
		WorkSpaceInfo* info = ews->getInfo();
		delete ews;
		delete info;
	    }
	}
	this->pageSelector->clear();
    }
    this->workSpace->setMembersInitialized(FALSE);
    this->workSpace->showRoot();
#endif

    // reset the workspace
    // FIXME: shouldn't this be in completeNewNetwork()
    this->moveWorkspaceWindow(0,0, FALSE);

    this->creating_new_network = TRUE;
}
void EditorWindow::completeNewNetwork()
{
    this->endNetworkChange();
    this->clearUndoList();
    this->creating_new_network = FALSE;
    theDXApplication->refreshErrorIndicators();

}

#ifndef FORGET_GETSET
void
EditorWindow::postGetSetConversionDialog()
{
    if (!EditorWindow::GetSetDialog)
	EditorWindow::GetSetDialog = new GetSetConversionDialog();

    EditorWindow::GetSetDialog->post();

    List *glns =  this->network->makeNamedNodeList ("Get");
    if (!glns) glns = this->network->makeNamedNodeList ("Set");
    if (glns) {
	EditorWindow::GetSetDialog->setActiveEditor(this);
	delete glns;
    }
}
#endif // FORGET_GETSET

//
// Open the grid dialog
//
void EditorWindow::postGridDialog()
{
    if (NOT this->gridDialog )
	this->gridDialog = new GridDialog(this->getRootWidget(),
				(WorkSpace*)this->workSpace);
    this->gridDialog->post();
}
//
// Open the find tool dialog
//
void EditorWindow::postFindToolDialog()
{
    if (NOT this->findToolDialog )
       this->findToolDialog = new FindToolDialog(this);
    this->findToolDialog->post();
}
//
// Open the print program dialog
//
void EditorWindow::postPrintProgramDialog()
{
    if (NOT this->printProgramDialog)
       this->printProgramDialog = new PrintProgramDialog(this);
    this->printProgramDialog->post();
}

//
// Virtual function called at the beginning/end of Command::execute 
// We must turn off tool selection and make sure the cursor is
// back to the normal cursor (i.e. not the Tool placement cursor).
//
void EditorWindow::beginCommandExecuting()
{
    this->toolSelector->deselectAllTools();
    this->workSpace->resetCursor();
}

void EditorWindow::notifyCPChange(boolean newList)
{
    if(this->panelGroupDialog)
    {
        if(newList)
             this->panelGroupDialog->makeToggles();
        this->panelGroupDialog->setToggles();
    }

}

void EditorWindow::notify(const Symbol message, const void *msgdata, const char *msg)
{
    if (message == DXApplication::MsgPanelChanged) {
	//
	// Set the command activations that depend on the number of panels.
	//
	int panelCount = this->network->getPanelCount();
	if (panelCount == 0) {
	    this->openControlPanelByNameMenu->deactivate();
	    this->setPanelAccessCmd->deactivate();
	    this->setPanelGroupCmd->deactivate();
	} else if (panelCount == 1) {
	    this->openControlPanelByNameMenu->activate();
	    this->setPanelAccessCmd->activate();
	    this->setPanelGroupCmd->activate();
	} // else if panelCount == 2 the commands were already activated.
    } else if (message == DXApplication::MsgExecute) {
	this->resetColorList();
    }
    this->DXWindow::notify(message, msgdata, msg);
}


//
// Adjust the name of the editor window based on the current network
// name.
//
void EditorWindow::resetWindowTitle()
{
    const char *vpe_name = "Visual Program Editor";
    char *p = NULL, *t = NULL;
    const char *title, *file = this->network->getFileName();

    if (file) {
	p = GetFullFilePath(file);
	t = new char[STRLEN(p) + STRLEN(vpe_name) +3];
	sprintf(t,"%s: %s", vpe_name, p);
#ifdef OS2
        if (STRLEN(t)>60)
          memmove(t, &t[STRLEN(t)-60],61);
#endif
	title = t;

#ifndef FORGET_GETSET
	if (EditorWindow::GetSetDialog)
	    EditorWindow::GetSetDialog->notifyTitleChange(this, title);
#endif

    } else {
	title = vpe_name;
    }
    this->setWindowTitle(title);
    if (p) delete p;
    if (t) delete t;
}

//
// Determine if there are any nodes of the give class that are selected.
//
boolean EditorWindow::anySelectedNodes(const char *classname)
{
    boolean r;
    List *l = this->makeSelectedNodeList(classname);
    if (!l)
	return FALSE;

    if (l->getSize() != 0)
	r = TRUE;
    else
	r = FALSE;
    delete l;
    return r;
}

//
// Return TRUE if there are selected Macros nodes.
//
boolean EditorWindow::anySelectedMacros()
{
    return this->anySelectedNodes(ClassMacroNode);
}
boolean EditorWindow::anySelectedDisplayNodes()
{
    return this->anySelectedNodes(ClassDisplayNode);
}
boolean EditorWindow::anySelectedColormaps()
{
    return this->anySelectedNodes(ClassColormapNode);
}
void EditorWindow::openSelectedMacros()
{
    List *l = this->makeSelectedNodeList(ClassMacroNode);
    if (!l)
	return;
    ListIterator li(*l);

    MacroNode *n;
    while( (n = (MacroNode *)li.getNext()) ) {
        n->openMacro();
	MacroDefinition* md = (MacroDefinition*)n->getDefinition();
	Network* net = md->getNetwork();
	theDXApplication->appendReferencedFile (net->getFileName());
    }
    delete l;
}
void EditorWindow::openSelectedImageWindows()
{
    List *l = this->makeSelectedNodeList(ClassDisplayNode);
    if (!l)
	return;
    ListIterator li(*l);

    DisplayNode *n;
    while( (n = (DisplayNode *)li.getNext()) )
        n->openImageWindow(TRUE);
    delete l;
}

//
// Print the visual program as a PostScript file using geometry and not
// bitmaps.  We set up the page so that it maps to the current workspace 
// and then as the StandIns and ArkStandIns to print themselves.  
// If the scale allows and the label_parameters arg is set, then 
// label the parameters and display the values. 
// We return FALSE and issue and error message if an error occurs.
//
boolean EditorWindow::printVisualProgramAsPS(const char *filename,
				float x_pagesize, float y_pagesize,
				boolean label_parameters)
{
    FILE *fout = fopen(filename,"w");
    if (!fout) {
	ErrorMessage("Can't open file %s for writing",filename);
	return FALSE;
    }

    //
    // Record the current page so that we can flip back there after the
    // print operation completes.  (This because we're going to have to visit
    // each uninitialized page.)
    //
    int page = this->workSpace->getCurrentPage();
    WorkSpace *current_ews = this->workSpace;
    if (page) current_ews = this->workSpace->getElement(page);

    //
    // Loop over the dictionary to get the pages to print.  FIXME: this doesn't
    // print them in the order in which they appear on the screen.  To do that,
    // you must build a new dictionary with the order_number as the key.
    //
    List* sorted_pages = this->pageSelector->getSortedPages();
    ASSERT(sorted_pages);
    ListIterator li(*sorted_pages);;
    EditorWorkSpace* ews;
    int pages_to_print = 0;
    int page_num = 1;

    while ( (ews = (EditorWorkSpace*)li.getNext()) ) 
	if (ews->getPostscriptInclusion()) pages_to_print++;

    if (pages_to_print == 0) {
	ErrorMessage (
	    "You have removed every Visual Program page from the\n"
	    "Postscript output.  (See Edit/Page/Configure Page.)\n"
	    "No output created."
	);
	return FALSE;
    }

    boolean current_ews_changed = FALSE;
    li.setList(*sorted_pages);
    while ( (ews = (EditorWorkSpace*)li.getNext()) ) {
	if (ews->getPostscriptInclusion()) {
	    if (ews->membersInitialized() == FALSE) {
		this->workSpace->showWorkSpace(ews);
		current_ews_changed = TRUE;
	    }
	    if (!this->printVisualProgramAsPS(fout, filename, x_pagesize,y_pagesize,
		label_parameters, ews, page_num, pages_to_print)) {
		    fclose (fout);
		    ErrorMessage("Error writing to file %s", filename);
		    return FALSE;
		}
	    page_num++;
	}
    }
    fclose (fout);

    delete sorted_pages;

    if (current_ews_changed)
	this->workSpace->showWorkSpace(current_ews);

    return TRUE;
}

boolean EditorWindow::printVisualProgramAsPS(FILE* fout, const char* filename, 
    float x_pagesize, float y_pagesize, boolean label_parameters, 
    EditorWorkSpace* current_ews, int page_number, int of_howmany)
{
    boolean landscape = FALSE;
    Node *n;
    float border_percent = .075;
    float printable_percent = 1.0 - 2*(border_percent);
    int xsize, ysize;
    time_t t;
    char *tod;
    int begx, begy;
    int endx, endy;
    List *arcs;
    StandIn *standIn;
    int i, icnt;
    int xpos, ypos, width, height;
    int xmin=1000000,xmax=0,ymin=1000000,ymax=0; 
    Ark *a;
    // We allocate these ourselves because the hp gives the following message
    // sorry, not implemented: label in block with destructors (2048)
    ListIterator *iterator = new ListIterator, *iter2 = new ListIterator;
    Decorator *dec;
    VPEAnnotator *vpea;
    Symbol vpeas = theSymbolManager->registerSymbol(ClassVPEAnnotator);
    GroupRecord* grec;
    const char* group_name;


    xmin = 0; 	// Always use the upper left corner of the workspace as origin
    ymin = 0; 	// 			ditto
    FOR_EACH_NETWORK_NODE(this->network, n, (*iterator)) { 
	standIn = n->getStandIn();
	if (!standIn) 
	    continue;
#if WORKSPACE_PAGES
	if (standIn->getWorkSpace() != current_ews) continue;
#endif
	standIn->getGeometry(&xpos, &ypos, &width, &height);
	xmax = MAX(xpos + width,xmax);	
	ymax = MAX(ypos + height,ymax);	
	
    }

    FOR_EACH_NETWORK_DECORATOR (this->network, dec, (*iterator)) {
#if WORKSPACE_PAGES
	if (dec->getWorkSpace() != current_ews) continue;
#endif
	dec->getXYPosition (&xpos, &ypos);
	dec->getXYSize (&width, &height);
	xmax = MAX(xpos + width, xmax);
	ymax = MAX(ypos + height,ymax);	
    }

    xsize = xmax - xmin;
    ysize = ymax - ymin;

    if (xsize > ysize)
	landscape = TRUE;

    /* Required for EPSF-3.0  */
    t = time(0);
    tod = ctime(&t);
    if (page_number == 1) {
	if (fprintf(fout, "%s", "%!PS-Adobe-3.0 EPSF-3.0\n") <= 0) goto error;
	if (fprintf(fout, "%%%%Creator: IBM/Data Explorer\n") <= 0) goto error;
	if (fprintf(fout, "%%%%CreationDate: %s", tod) <= 0) goto error;
	if (fprintf(fout, "%%%%Title:  %s\n",  filename) <= 0) goto error;
	if (fprintf(fout, "%%%%Pages: %d\n", of_howmany) <= 0) goto error;

	/* Put a Bounding Box specification, required for EPSF-3.0  */
	begx = (int) ((x_pagesize * border_percent) * 72);
	begy = (int) ((y_pagesize * border_percent) * 72);
	endx = (int) ((x_pagesize * 72) - begx);
	endy = (int) ((y_pagesize * 72) - begy);
	if (fprintf(fout, "%%%%BoundingBox: %d %d %d %d\n",
			    begx, begy, endx,endy) <= 0) 
	    goto error;

	//
	// Begin the Setup section
	//
	if (fprintf(fout,"%%%%BeginSetup\n\n") < 0) goto error;
	if (fprintf(fout, "%% Build a temporary dictionary\n%d dict begin\n\n",
						    25) < 0) goto error;
	if (!StandIn::PrintPostScriptSetup(fout))	goto error;
	if (fprintf(fout,"%%%%EndSetup\n\n") < 0) goto error;
    }
	

    //
    // Begin the Page Setup section
    //
    if (fprintf(fout,"\n%%%%Page: %d %d\n\n", page_number, page_number) <= 0) goto error;

    if (fprintf(fout,"%%%%BeginPageSetup\n\n") < 0) goto error;


    if ((fprintf(fout,"%% Number of units/inch (don't change this one)\n"
                      "/upi 72 def\n") <= 0)  			||
        (fprintf(fout,"%% Size of output page in inches\n"
			"/pwidth %f def\n"
			"/pheight %f def\n",
                                x_pagesize,y_pagesize) <= 0)           	||
        (fprintf(fout,"%% Size of printable output page in inches\n"
			"/width  %f pwidth  mul def\n"
			"/height %f pheight mul def\n",
			printable_percent, printable_percent) <= 0)   	|| 
        (fprintf(fout,"%% Extents of the used portion of the workspace\n"
                        "/xmin %d def\n"
                        "/xmax %d def\n"
                        "/ymin %d def\n"
                        "/ymax %d def\n",
                        xmin, xmax,ymin,ymax) <= 0)      	||
        (fprintf(fout,"%% Size of the used portion of the workspace\n"
                        "/xsize xmax xmin sub def\n"
                        "/ysize ymax ymin sub def\n") <= 0))
        goto error;


    if (landscape) {
        if ((fprintf(fout,"%% Rotate and translate into Landscape mode\n")
                                                        <= 0) ||
            (fprintf(fout,"90 rotate\n0 pwidth neg upi mul translate\n\n")
                                                        <= 0) ||
            (fprintf(fout,"%% Swap the width and height variables\n") 
							<= 0) ||
            (fprintf(fout,"/tmp width def\n"
			  "/width height def\n"
			  "/height tmp def\n") 		<= 0) ||
            (fprintf(fout,"/tmp pwidth def\n"
			  "/pwidth pheight def\n"
			  "/pheight tmp def\n") 	<= 0)) 
            goto error;
    }

    if (fprintf(fout,"%% Translate canvas inside page borders\n"
			"%f pwidth mul upi mul dup translate\n",
			border_percent ) <= 0)
	goto error;

    if (fprintf(fout,"%% Scale canvas to the page with origin in upper left\n"
			"/max {	%% Usage : a b max\n" 
			"	2 dict begin\n"
		 	"	/a 2 1 roll def\n"
		 	"	/b 2 1 roll def\n"
			"	a b gt\n"
			"	{ a } { b } ifelse\n"
			"	end\n"
			"} def\n"
			"0 height upi mul translate\n"
			"xsize width div ysize height div max\n"
			"upi max\n"		// Use at least 72 dpi
			"upi 2 1 roll div "
			"dup neg "
		 	"scale\n"
			"%% Now Translate to (xmin,ymin)\n"
		 	"xmin neg ymin neg translate\n"	) <= 0)
	goto error;
			
    
    if (fprintf(fout,"%%%%EndPageSetup\n\n") <= 0) goto error;


    //
    // And finally send the visual program. 
    //
    FOR_EACH_NETWORK_NODE(this->network, n, (*iterator)) { 
	standIn = n->getStandIn();
	if (!standIn) 
	    continue;
#if WORKSPACE_PAGES
	if (standIn->getWorkSpace() != current_ews) continue;
#endif
	standIn->printPostScriptPage(fout, label_parameters && (xsize < 1500));
	icnt = n->getInputCount();
	for (i=1 ; i<=icnt ; i++) {
	    if (n->isInputConnected(i) && 
		n->isInputViewable(i) && 
		n->isInputVisible(i)) {
		arcs = (List*)n->getInputArks(i);
		ASSERT(arcs);
		iter2->setList(*arcs);
		while ( (a = (Ark*)iter2->getNext()) ) 
		    a->getArkStandIn()->printAsPostScript(fout);
	    }
	}
    }

    FOR_EACH_NETWORK_DECORATOR (this->network, dec, (*iterator)) {
#if WORKSPACE_PAGES
	if (dec->getWorkSpace() != current_ews) continue;
#endif
	if (dec->isA(vpeas)) {
	    vpea = (VPEAnnotator*)dec;
	    vpea->printPostScriptPage(fout); 
	}
    }

    //
    // Oh, and one more thing.  ...Page %s: %d of %d
    //
    char footer[128];
    grec = this->getGroupOfWorkSpace((EditorWorkSpace*)current_ews);
    group_name = (grec? grec->getName(): "<Untitled>");
    sprintf (footer, "%s - page %d of %d", group_name, page_number, of_howmany);
    if (fprintf (fout, 
	"%% Reset translation, rotation, and scaling for making a page footer\n"
	"initmatrix\n"
	"/Helvetica-Bold findfont\n"
	"[ 10.0 0 0 10.0 0 0 ] makefont setfont\n"
	"%% (page width/2) - (string width/2) for centering the footer\n"
	"(%s) stringwidth pop 0.5 mul neg\n"
	"%f 0.5 mul upi mul add\n"
	"%% 0.5 inches up from the bottom of the page\n"
	"36 moveto\n"
	"(%s) show\n",
	footer, x_pagesize, footer) <=0) goto error;

    //
    // Force the page out of the printer 
    //
    if (fprintf(fout,"\nshowpage\n") <= 0) goto error;

    if (page_number == of_howmany)
	if (fprintf(fout,"\nend\n") <= 0) 
	    goto error;

    delete iterator;
    delete iter2;
    return TRUE;

error:
    delete iterator;
    delete iter2;
    return FALSE;
}

boolean EditorWindow::macroifySelectedNodes(const char *name,
				    const char *cat,
				    const char *desc,
				    const char *fileName)
{
    ListIterator li;
    Node *node; 

    //
    // Try and verify that the selected nodes are macrofiable. 
    //
    if (!this->areSelectedNodesMacrofiable())
	return FALSE;

    //
    // Get the set of selected nodes.  If there are none, we're done.
    //
    List *l = this->makeSelectedNodeList();
    if (l == NULL || l->getSize() == 0) {
	if (l) delete l;
	return TRUE;
    }

    //
    // Check to see if all these nodes are allowed in macros, get the
    // bounding box of the nodes on the vpe.
    //
    int x = 0, y = 0;
    node = (Node*)l->getElement(1);
    node->getVpePosition(&x, &y);
    int bboxXmin = x;
    int bboxYmin = y;
    int bboxYmax = y;
    for (li.setList(*l); (node = (Node*)li.getNext()); )
    {

	node->getVpePosition(&x, &y);
	if(x < bboxXmin)
	    bboxXmin = x;
	if(y < bboxYmin)
	    bboxYmin = y;
	if(y > bboxYmax)
	    bboxYmax = y;
    }

    //
    // Create and setup the new network
    //
    Network *net = theDXApplication->newNetwork();

    net->setName(name? name: "NewMacro");
    net->setCategory(cat? cat: "Macros");
    net->setDescription(desc? desc: "new macro");

    if (!net->makeMacro(TRUE))
    {
	delete net;
	delete l;
	return FALSE;
    }

    MacroDefinition *nd = net->getDefinition();
    nd->setNetwork(net);
    theDXApplication->macroList.appendElement(net);

    theNodeDefinitionDictionary->replaceDefinition(nd->getNameString(), nd);
    ToolSelector::AddTool(nd->getCategorySymbol(),
			  nd->getNameSymbol(),
			  (void *)nd);
    ToolSelector::UpdateCategoryListWidget();

    //
    // For each arc, if the arc crosses the line between selected and
    // unselected nodes, delete it and set up a macro input.
    // if it is only in the new macro, delete and recreate the arc.
    //
    struct connection {
	Node *n;
	int   param;
	Node *pn;
    };
    List newInputs;
    List newOutputs;
    connection *con;
    for (li.setList(*l); (node = (Node*)li.getNext()); )
    {
 	List *orig;
	int i;
	for (i = 1; i <= node->getInputCount(); ++i)
	{
	    orig = (List*)node->getInputArks(i);
	    List *inputs = orig->dup();
	    ListIterator li(*inputs);
	    Ark *a;
	    while ( (a = (Ark*)li.getNext()) )
	    {
		int param;
		Node *source = a->getSourceNode(param);
		StandIn *si = source->getStandIn();
		source->deleteArk(a);
		si->deleteArk(a);

		if (!si->isSelected()) {
		    ListIterator connections(newInputs);
		    while ( (con = (connection *)connections.getNext()) ) {
			if (source == con->n && param == con->param)
			    break;
		    }
		    if (con == NULL) {
			con = new connection;
			con->n = source;
			con->param = param;

			MacroParameterDefinition *inputDef =
			    (MacroParameterDefinition*)
			    theNodeDefinitionDictionary->
			    findDefinition("Input");
			con->pn = inputDef->createNewNode(net);
			net->addNode(con->pn);
			newInputs.appendElement(con);
		    }
		    new Ark(con->pn, 1, node, i);
		}
		else {
		    new Ark(source, param, node, i);
		}
	    }
	    delete inputs;
	}

	for (i = 1; i <= node->getOutputCount(); ++i)
	{
	    orig = (List*)node->getOutputArks(i);
	    List *outputs = orig->dup();
	    ListIterator li(*outputs);
	    Ark *a;
	    while ( (a = (Ark*)li.getNext()) )
	    {
		int param;
		Node *dest = a->getDestinationNode(param);
		StandIn *si = dest->getStandIn();
		dest->deleteArk(a);
		si->deleteArk(a);

		if (!si->isSelected()) {
		    ListIterator connections(newOutputs);
		    while ( (con = (connection*)connections.getNext()) ) {
			if (dest == con->n && param == con->param)
			    break;
		    }
		    if (con == NULL) {
			con = new connection;
			con->n = dest;
			con->param = param;

			MacroParameterDefinition *outputDef =
			    (MacroParameterDefinition*)
			    theNodeDefinitionDictionary->
			    findDefinition("Output");
			con->pn = outputDef->createNewNode(net);
			con->pn->setVpePosition(0,bboxYmax-bboxYmin+2*80);
			net->addNode(con->pn);
			newOutputs.appendElement(con);
		    }
		    new Ark(node, i, con->pn, 1);
		}
		else {
		    new Ark(node, i, dest, param);
		}
	    }
	    delete outputs;
	}
    }

    //
    // Switch the nodes to the newly created network.
    //
    if (!net->mergeNetworks (this->network, NULL, FALSE)) {
	delete l;
	return FALSE;
    }

    XmUpdateDisplay(this->getRootWidget());

    //
    // Move the nodes to the upper right hand corner after switching them
    // to the new net.  This is the last use of "l".
    //
    for (li.setList(*l); (node = (Node*)li.getNext()); )
    {
	node->getVpePosition(&x,&y);
	node->setVpePosition(x-bboxXmin, y-bboxYmin+80);
    }
    delete l;

    net->sortNetwork();

    node = nd->createNewNode(this->network);
    if (!node)
    {
	delete net;
	return FALSE;
    }
    node->setVpePosition(bboxXmin, bboxYmin);

    this->network->addNode(node);


    int i;
    for (i = 1; (con = (connection*)newInputs.getElement(1)); ++i) {
	node->getStandIn()->addArk(this, new Ark(con->n, con->param, node, i));
	delete con;
	newInputs.deleteElement(1);
    }
    newInputs.clear();
    for (i = 1; (con = (connection*)newOutputs.getElement(1)); ++i) {
	node->getStandIn()->addArk(this, new Ark(node, i, con->n, con->param));
	delete con;
	newOutputs.deleteElement(1);
    }
    newOutputs.clear();

    if (fileName)
	net->saveNetwork(fileName, TRUE);

#ifndef FORGET_GETSET
    if ((EditorWindow::GetSetDialog) && (EditorWindow::GetSetDialog->isManaged()))
	EditorWindow::GetSetDialog->update();
#endif

    return TRUE;
}

void EditorWindow::postInsertNetworkDialog()
{
    if (this->insertNetworkDialog == NULL)
	this->insertNetworkDialog = new InsertNetworkDialog(this->getRootWidget());
    this->insertNetworkDialog->post();
}
void EditorWindow::postCreateMacroDialog()
{
    if (this->createMacroDialog == NULL)
	this->createMacroDialog = new CreateMacroDialog(
					    this->getRootWidget(),
					    this);
    this->createMacroDialog->post();
}
void EditorWindow::optimizeNodeOutputCacheability()
{
   this->network->optimizeNodeOutputCacheability();
}
//
// Search all nodes and select those with any outputs that match the
// give cachability.
//
void EditorWindow::selectNodesWithOutputCacheability(Cacheability c)
{
    Node *node;

    //
    // Scan the tools and select them if any of their outputs have the 
    // given cachability. 
    //
    ListIterator iterator;
    FOR_EACH_NETWORK_NODE(this->network, node, iterator) {
	int i, cnt =  node->getOutputCount();
	boolean select = FALSE;
	for (i=1 ; i<=cnt && !select ; i++)  {
	    if (node->getOutputCacheability(i) == c)
		select = TRUE;
	}
	this->selectNode(node, select, FALSE);
    }

}
//
// Set the cacheability of all outputs of the selected tools to one of 
// OutputFullyCached, OutputNotCached or OutputCacheOnce.
// A warning is issued on those nodes that do not have writeable cacheability.
//
void EditorWindow::setSelectedNodesOutputCacheability(Cacheability c)
{
#define MAX_IGNORES 256
     int i;
     Dictionary ignored;
     Node *node;
     List *snlist = this->makeSelectedNodeList();

     ASSERT((c == OutputFullyCached) || (c == OutputNotCached) ||
	    (c == OutputCacheOnce));

     if (NOT snlist)
	return;

     //
     // Scan the list of selected tools and change their outputs' cacheability
     // if it is writeable. 
     //
     ListIterator li(*snlist);
     while ( (node = (Node*)li.getNext()) ) {
	int i, cnt =  node->getOutputCount();
	for (i=1 ; i<=cnt ; i++)  {
	    if (node->isOutputCacheabilityWriteable(i)) {
		node->setOutputCacheability(i,c);
            } else { 
		const char *name = node->getNameString();
		if (!ignored.findDefinition(name))
		    ignored.addDefinition(name,name);
	    }
	}
     }

     //
     // Notify the user about any modules that were skipped above
     //
     int num_ignored = ignored.getSize();
     if (num_ignored > 0) {
	char *name, *p, *tools = new char[num_ignored * 64];
	tools[0] = '\0';
	p = tools;	
	DictionaryIterator di(ignored);
	for (i=0 ; (name = (char*)di.getNextDefinition()); i++, p+=STRLEN(p)) {
	    sprintf(p,"%s  ",name);
	    if ((i != 0) && ((i & 3) == 0)) 
		strcat(p,"\n");	// 4 tool names per line of text.
	}
	WarningMessage("One or more of the outputs of the following tools has "
			"a read-only cacheability setting\n"
		"(operation ignored for those outputs):\n\n%s", tools);
	delete tools;
     }
     delete snlist;
}
void EditorWindow::postSaveAsCCodeDialog()
{
#ifdef DXUI_DEVKIT
    if (NOT this->saveAsCCodeDialog)
       this->saveAsCCodeDialog = new SaveAsCCodeDialog(
			this->getRootWidget(),this->getNetwork());
    this->saveAsCCodeDialog->post();
#endif // DXUI_DEVKIT
}

//
// This is a helper method for this->areSelectedNodesMacrofiable().
// It recursively descends the connections from the given node to determine
// if it connected to a selected node.  If ignoreDirectConnect is TRUE,
// then we do not care about selected nodes that are directly connected
// to selected nodes. 
// Returns TRUE if there is a downstream selected node, otherwise FALSE.
//
boolean EditorWindow::isSelectedNodeDownstream(Node *srcNode,
                                boolean ignoreDirectConnect)
{
    int         i, j;
    StandIn     *srcStandIn = srcNode->getStandIn();

    if (!srcStandIn)
        return FALSE;

    boolean     srcSelected = srcStandIn->isSelected();

    ASSERT(!srcSelected || ignoreDirectConnect);

    int numParam = srcNode->getOutputCount();

    for (i = 1; i <= numParam; ++i)
    {
        if (srcNode->isOutputConnected(i))
        {
            Ark *a;
            List *arcs = (List *)srcNode->getOutputArks(i);
            for (j = 1; (a = (Ark*)arcs->getElement(j)); ++j)
            {
                int paramInd;
                Node *destNode = a->getDestinationNode(paramInd);
		if (destNode->isMarked())
		    continue;
                StandIn *destStandIn = destNode->getStandIn();
		if (!destStandIn)
		    continue;
		boolean destSelected = destStandIn->isSelected();
		if (srcSelected && !destSelected) {
		    if (this->isSelectedNodeDownstream(destNode,FALSE))
			return TRUE;
		} else if (srcSelected && destSelected) {
		    if (!ignoreDirectConnect)
			return TRUE;
		} else if (!srcSelected && destSelected) {
		    return TRUE;
		} else {    // !srcSelected && !destSelected
		    if (this->isSelectedNodeDownstream(destNode,FALSE)) 
			return TRUE;
                }
		if (!destSelected)
		    destNode->setMarked();
            }
        }
    }

    return FALSE;
}
//
// Determine if the currently selected set of nodes is macrofiable.  In
// particular, we check for selections that would result in an output
// being connected to on input of the newly created macro.  To do this
// we look at each selected node and determine if its immediate downstream
// unselected nodes are connected (possibly through intermediate nodes)
// to another of the selected nodes.
// Returns TRUE if macrofiable, else returns FALSE and issues and error
// message.
//
boolean EditorWindow::areSelectedNodesMacrofiable()
{

    int      i;
    Network  *network = this->network;
    int      numNodes = network->nodeList.getSize();
    List     tmpNodeList;
    ListIterator iterator;
    Node        *n;
    boolean	saw_tmtr = FALSE, saw_rcvr = FALSE;
    List 	*tmtrs = NULL, *rcvrs = NULL; 
   
    if (numNodes == 0)
        return TRUE;

    iterator.setList(network->nodeList);
    for (i=0 ; (n = (Node*)iterator.getNext()) ; i++)
        n->clearMarked();

//
// The HP gives the following message if I try to use a goto 
// CC: "EditorWindow.C", line 3510: sorry, not implemented: label in 
//  	block with destructors (2048)
//
#define RETURN_FALSE    if (tmtrs) delete tmtrs;	\
			if (rcvrs) delete rcvrs;	\
			return FALSE;

    iterator.setList(network->nodeList);
    while ( (n = (Node*)iterator.getNext()) ) {
        StandIn *si = n->getStandIn();
        if (si && si->isSelected()) {
	    if (this->isSelectedNodeDownstream(n,TRUE)) {
		ErrorMessage(
		"The currently selected tools can not be macrofied because the"
		"\nresulting macro would have an output connected to an input."
		"\nThis is due to the fact that a data path that leaves the "
		"\nselected tools later reenters the selected tools.");
		RETURN_FALSE;
	    } else if (!n->isAllowedInMacro())  {
		ErrorMessage("Tool %s is not allowed in a macro",
		    n->getNameString());
		RETURN_FALSE;
	    } else if (n->isA(ClassMacroParameterNode)) {
		ErrorMessage(
		    "Tool %s is not allowed in an automatically created macro",
		    n->getNameString());
		RETURN_FALSE;
	    } else if (n->isA(ClassTransmitterNode)) {
		if (!saw_tmtr) {
		    rcvrs = network->makeClassifiedNodeList(ClassReceiverNode);
		    saw_tmtr = TRUE;
		}
		if (rcvrs) {
		    ListIterator iter(*rcvrs);
		    const char *tlabel = n->getLabelString();
		    Node *r;
		    while ( (r = (Node*)iter.getNext()) ) {
			StandIn *rsi = r->getStandIn();
			if (rsi && !rsi->isSelected() && 
				    EqualString(r->getLabelString(),tlabel)) {
			    ErrorMessage(
			    "The currently selected tools can not be macrofied "
			    "because the selected\nTransmitter %s has a "
			    "corresponding Receiver that is not selected.\n",
			    tlabel);
			    RETURN_FALSE;
			}
		    }
		}
	    } else if (n->isA(ClassReceiverNode)) {
		if (!saw_rcvr) {
		    tmtrs = 
			network->makeClassifiedNodeList(ClassTransmitterNode);
		    saw_rcvr = TRUE;
		}
		if (tmtrs) {
		    ListIterator iter(*tmtrs);
		    const char *rlabel = n->getLabelString();
		    Node *t;
		    while ( (t = (Node*)iter.getNext()) ) {
			StandIn *tsi = t->getStandIn();
			if (tsi && !tsi->isSelected() && 
				    EqualString(t->getLabelString(),rlabel)) {
			    ErrorMessage(
			    "The currently selected tools can not be macrofied "
			    "because the selected\nReceiver %s has a "
			    "corresponding Transmitter that is not selected.\n",
			    rlabel);
			    RETURN_FALSE;
			}
		    }
		}
	    }
	}
    }

    if (tmtrs) delete tmtrs;
    if (rcvrs) delete rcvrs;
    return TRUE;

}


#ifndef FORGET_GETSET
//
// convert selected nodes (guaranteed to be either Get or Set) to
// Global
//
void
EditorWindow::ConvertToGlobal(boolean global)
{
    EditorWindow* e = EditorWindow::GetSetDialog->getActiveEditor();
    ASSERT(e);
    e->convertToGlobal(global);
}

void
EditorWindow::convertToGlobal(boolean global)
{
    List *glnlist = this->makeSelectedNodeList();

    // The command can't be active if there is not selected node in the vpe.
    ASSERT(glnlist);
    ListIterator it(*glnlist);
    GlobalLocalNode *gln;

    while ( (gln = (GlobalLocalNode*)it.getNext()) ) {
	ASSERT (((Node*)gln)->isA(ClassGlobalLocalNode));
	// Both of the following return FALSE if the node has never been set
	// Taken together, these 3 ASSERTS mean that everything in the list
	// is either a Get or a Set.
	//ASSERT (!gln->isGlobalNode());
	//ASSERT (!gln->isLocalNode());

	if (global) gln->setAsGlobalNode();
	else gln->setAsLocalNode();
    }
    delete glnlist;

    this->setCommandActivation();
    if (EditorWindow::GetSetDialog)
	EditorWindow::GetSetDialog->setCommandActivation();

}

void
EditorWindow::ConvertToLocal()
{
    EditorWindow::ConvertToGlobal(FALSE);
}

#endif


#if WORKSPACE_PAGES
//
// Turn the selected nodes into a page.
//  Any arc of a selected node must go to a node which is also selected.
//  The Edit/Select/Selected Connected command works well for selecting pagifiable nodes.
//
boolean
EditorWindow::pagifySelectedNodes(boolean include_selected_nodes)
{
char page_name[64];

    //
    // See if the selected nodes are pagifiable. 
    // Creating the group automatically includes selected nodes, so if we don't
    // want them included, deselect them.
    //
    List *l = NUL(List*);
    List *dl = NUL(List*);
    boolean pagifiable = this->areSelectedNodesPagifiable(include_selected_nodes);
    if ((!pagifiable) && (include_selected_nodes)) {
	return FALSE;
    } else if ((pagifiable) && (include_selected_nodes)) {
	l = this->makeSelectedNodeList();
	dl = this->makeSelectedDecoratorList();
    } else {
	this->deselectAllNodes();
    }

    //
    // Find an unused name for a new page.
    //
    boolean page_name_in_use = TRUE;
    int next_page_no = 1;
    while (page_name_in_use) {
	sprintf (page_name, "Untitled_%d", next_page_no++);
	const void* def_found = this->pageSelector->findDefinition (page_name);
	if (def_found == NUL(void*))
	    page_name_in_use = FALSE;
    }

    //
    // Create a new page group in the network.
    // Creating the new group also automatically marks selected nodes as belonging.
    //
    GroupManager *page_mgr = (GroupManager*)
	this->network->getGroupManagers()->findDefinition(PAGE_GROUP);
    ASSERT(page_mgr->createGroup (page_name, this->network));

    //
    // Make the new WorkSpace page. 
    //
    EditorWorkSpace *page = (EditorWorkSpace*)this->workSpace->addPage();
    ASSERT (this->pageSelector->addDefinition (page_name, page));
    PageGroupRecord *grec = (PageGroupRecord*)page_mgr->getGroup (page_name);
    grec->setComponents (NUL(UIComponent*), page);

    //
    // Move the standins and decorators out their old page
    //
    if ((include_selected_nodes) && ((l) || (dl))) {

	//
	// prepare to turn off/on line drawing  if moving standins
	//
	EditorWorkSpace *selectedWS = NUL(EditorWorkSpace*);
	if ((l) && (l->getSize())) {
	    Node* n = (Node*)l->getElement(1);
	    StandIn* si = (StandIn*)n->getStandIn();
	    selectedWS = (EditorWorkSpace*)si->getWorkSpace();
	} else if ((dl) && (dl->getSize())) {
	    Decorator* dec = (Decorator*)dl->getElement(1);
	    selectedWS = (EditorWorkSpace*)dec->getWorkSpace();
	}

	if (selectedWS) selectedWS->beginManyPlacements(); 
	this->moveDecorators (page, dl, 20, 20, l);
	this->moveStandIns (page, l, 20, 20, dl);
	if (selectedWS) selectedWS->endManyPlacements();
    }

    if (l) delete l;
    if (dl) delete dl;
    return TRUE;
}
#endif



#if WORKSPACE_PAGES
//
// Delete the page and everything inside the page and the page group.
// If the group_name passed in is NULL, then delete the current page.
//
boolean
EditorWindow::deletePage(const char* to_delete)
{
    //
    // Find the workspace we're deleting.  We also need to know if we're deleting
    // the current workspace.  If current is being deleted, then we'll have to
    // present some other workspace after deletion.
    //
    EditorWorkSpace* ews = NUL(EditorWorkSpace*);
    if (to_delete) ews = (EditorWorkSpace*)this->pageSelector->findDefinition (to_delete);
    int page = this->workSpace->getCurrentPage();
    EditorWorkSpace* current_ews = this->workSpace;
    if (page) current_ews = (EditorWorkSpace*)this->workSpace->getElement(page);
    if (ews == NUL(EditorWorkSpace*)) ews = current_ews;
    boolean deleted_was_current = (ews == current_ews);

    //
    // Find all the guys whose group name is the same as the group name of
    // the workspace we're deleting.  Somewhat roundabout, but it works for
    // both situations... a) deleting a named page, b) deleting the current page.
    //
    boolean first = TRUE;
    const char *group_name = NUL(char*);
    Node *node;
    ListIterator iter;
    List nodeList;
    List decorList;
    Decorator *decor;
    nodeList.clear();
    decorList.clear();
    FOR_EACH_NETWORK_NODE(this->network, node, iter) {
	StandIn *si = node->getStandIn();
	if (!si) continue;
	WorkSpace* nws = si->getWorkSpace();
	if (nws == ews) {
	    nodeList.appendElement((void*)node);
	    if (first) {
		group_name = node->getGroupName(theSymbolManager->getSymbol(PAGE_GROUP));
		first = FALSE;
	    }
	}
    }
    FOR_EACH_NETWORK_DECORATOR(this->network, decor, iter) {
	WorkSpace* nws = decor->getWorkSpace();
	if (nws == ews) {
	    decorList.appendElement((void*)decor);
	}
    }

    //
    // Now all the nodes/decorators are gathered up.  Delete them.
    //
    this->deleteNodes(&nodeList);
    nodeList.clear();
    ListIterator it(decorList);
    while ( (decor = (Decorator*)it.getNext()) ) {
	this->network->decoratorList.removeElement((void*)decor);
	delete decor;
    }

    if ((to_delete) && (group_name)) {
	ASSERT(EqualString(to_delete, group_name));
    }

    //
    // If there was a node in the group then we'll have the name otherwise
    // we don't know the name and we'll have to scan the Dictionary
    //
    GroupManager *page_mgr = (GroupManager*)
	this->network->getGroupManagers()->findDefinition (PAGE_GROUP);
    if (to_delete) {
	this->pageSelector->removeDefinition ((const char*)to_delete);
	page_mgr->removeGroup(to_delete, this->network);
    } else if (group_name) {
	this->pageSelector->removeDefinition ((const char*)group_name);
	page_mgr->removeGroup(group_name, this->network);
    } else {
	boolean found = FALSE;
	int i, pcnt = this->pageSelector->getSize();
	for (i=1; i<=pcnt; i++) {
	    EditorWorkSpace *def = (EditorWorkSpace*)
		this->pageSelector->getDefinition(i);
	    if (def == ews) {
		group_name = this->pageSelector->getStringKey(i);
		this->pageSelector->removeDefinition ((const char*)group_name);
		found = TRUE;
		break;
	    }
	}
	ASSERT(found);
	page_mgr->removeGroup(group_name, this->network);
    }

    //
    // If it was really a subpage and not the root page.
    //
    if (ews != this->workSpace) {
	WorkSpaceInfo* info = ews->getInfo();
	delete ews;
	delete info;
    }

    //
    // We've deleted what was the visible page.  Now we must find a different page
    // to present to the user.  Walk the list of pages looking for one which has
    // had standins created. (We don't want to accidently present a mamoth page
    // which will take a long time to fill.)  If none is found with standins, then
    // try to put up one which contains fewest members.  Else just put up page 0.
    // Putting up page 0 might require adding "Untitled" to the pageSelector.
    //
    if (deleted_was_current) {
	boolean found = FALSE;
	DictionaryIterator di(*this->pageSelector);
	int member_count = 9999999;
	EditorWorkSpace* contains_few_members = NUL(EditorWorkSpace*);
	while ( (ews = (EditorWorkSpace*)di.getNextDefinition()) ) {
	    if (ews->membersInitialized()) {
		found = TRUE;
		this->pageSelector->selectPage(ews);
		break;
	    } else {
		int current_count = this->getPageMemberCount(ews);
		if ((current_count > 0) && (current_count < member_count)) {
		    member_count = current_count;
		    contains_few_members = (EditorWorkSpace*)ews;
		}
		/*if (ews == this->workSpace)
		  contains_root = TRUE;*/
	    }
	}
	if (!found) {
	    if (contains_few_members) {
		this->pageSelector->selectPage(contains_few_members);
	     } else if (this->pageSelector->getSize()) {
		EditorWorkSpace* ws = (EditorWorkSpace*)
		    this->pageSelector->getDefinition(1);
		ASSERT(ws);
		this->pageSelector->selectPage(ws);
	    } else {
		this->pageSelector->addDefinition ("Untitled", this->workSpace);
		this->pageSelector->selectPage(this->workSpace);
	    }
	}
    }

    return TRUE;
}
#endif


EditorWorkSpace *
EditorWindow::getNodesBBox(int *minx, int *miny, int *maxx, int *maxy, List *l, List *dl)
{
    //
    // get the bounding box of the nodes on the vpe.
    //
    int x = 0, y = 0;
    int bboxXmin = 0, bboxYmin = 0;
    int bboxXmax = 1, bboxYmax = 1;
    ListIterator li;
    int width, height;
    EditorWorkSpace *selectedWorkSpace = NULL;
    boolean first = TRUE;

    if (l) {
	li.setList(*l);
	Node *node;
	while ( (node = (Node*)li.getNext()) ) {
	    node->getVpePosition(&x, &y);
	    if (first) {
		bboxXmin = x;
		bboxYmin = y;
		first = FALSE;
	    }
	    StandIn *si = node->getStandIn();

	    if (si) {
		si->getXYSize (&width, &height);
		if (!selectedWorkSpace) 
		    selectedWorkSpace = (EditorWorkSpace*)si->getWorkSpace();
		ASSERT (selectedWorkSpace == (EditorWorkSpace*)si->getWorkSpace());
	    } else {
		width = height = 0;
	    }

	    if(x < bboxXmin)
		bboxXmin = x;

	    if((x+width) > bboxXmax)
		bboxXmax = x + width;

	    if(y < bboxYmin)
		bboxYmin = y;

	    if((y+height) > bboxYmax)
		bboxYmax = y + height;
	}
    }

    if (dl) {
	li.setList(*dl);
	Decorator *dec = NULL;
	while ( (dec = (Decorator*)li.getNext()) ) {
	    dec->getXYPosition (&x, &y);
	    if (first) {
		bboxXmin = x;
		bboxYmin = y;
		first = FALSE;
	    }
	    dec->getXYSize (&width, &height);

	    if(x < bboxXmin)
		bboxXmin = x;

	    if((x+width) > bboxXmax)
		bboxXmax = x + width;

	    if(y < bboxYmin)
		bboxYmin = y;

	    if((y+height) > bboxYmax)
		bboxYmax = y + height;
	}
    }

    *minx = bboxXmin;
    *miny = bboxYmin;
    *maxx = bboxXmax;
    *maxy = bboxYmax;
    return selectedWorkSpace;
}

#if WORKSPACE_PAGES

//
// Destroy all standins in the list
//
void EditorWindow::destroyStandIns(List* selectedNodes)
{
boolean error_node = FALSE;
int i;
ListIterator li(*selectedNodes);
Node *node;

    while ( (node = (Node*)li.getNext()) ) {
	//
	// Destroy the input arcs
	//
	int count = node->getInputCount();
	for (i=1; i<=count; i++) {
	    Ark *a;
	    List *orig = (List*)node->getInputArks(i);
	    List *conns = orig->dup();
	    ListIterator ai(*conns);
	    while ( (a = (Ark*)ai.getNext()) ) {
		ArkStandIn *asi = a->getArkStandIn();
		a->setArkStandIn(NULL);
		delete asi;
	    }
	    delete conns;
	}

	//
	// Destroy the output arcs
	//
	count = node->getOutputCount();
	for (i=1; i<=count; i++) {
	    Ark *a;
	    List *orig = (List*)node->getOutputArks(i);
	    List *conns = orig->dup();
	    ListIterator ai(*conns);
	    while ( (a = (Ark*)ai.getNext()) ) {
		ArkStandIn *asi = a->getArkStandIn();
		a->setArkStandIn(NULL);
		delete asi;
	    }
	    delete conns;
	}


	//
	// Destroy the existing StandIn for the node, making sure to remove it
	// from the error list if necessary.
	//
	StandIn *si = node->getStandIn();
	if (si) {
	    si->unmanage();
	    if (this->errored_standins) 
		error_node|= this->errored_standins->removeElement((void*)si);
	    delete si;
	}
    }

    if (error_node)
	this->resetErrorList(FALSE);
}

void
EditorWindow::moveStandIns(EditorWorkSpace *page, List* selectedNodes, int xoff, int yoff, List *dl)
{
int minx, miny, maxx, maxy;

    if (!selectedNodes) return ;

    this->getNodesBBox (&minx, &miny, &maxx, &maxy, selectedNodes, dl);

    this->destroyStandIns(selectedNodes);

    Node *node;
    ListIterator li(*selectedNodes);
    while ( (node = (Node*)li.getNext()) ) {
	int x,y;
	//
	// Record bbox relative position, incoming, outgoing arcs
	//
	node->getVpePosition(&x, &y);

	//
	// Make a new StandIn using the saved position info.
	//
	node->setVpePosition (xoff + x - minx, yoff + y - miny);
    }
    page->setMembersInitialized(FALSE);

}
#endif


#if WORKSPACE_PAGES
void
EditorWindow::moveDecorators(EditorWorkSpace *page, List* seldec, int xoff, int yoff, List*nl)
{
int minx, miny, maxx, maxy;
Decorator *dec;
int x,y;

    if (!seldec) return ;

    this->getNodesBBox (&minx, &miny, &maxx, &maxy, nl, seldec);

    Widget page_w = page->getRootWidget();
    Window page_wnd = (page_w?XtWindow(page_w):NUL(Window));

    ListIterator it(*seldec);
    while ( (dec = (Decorator*)it.getNext()) ) {
	dec->getXYPosition (&x, &y);
	dec->unmanage();
	dec->uncreateDecorator();
	dec->setXYPosition (xoff + x - minx, yoff + y - miny);
	if ((page_w) && (page_wnd))
	    dec->manage(page);
	else
	    page->setMembersInitialized(FALSE);
    }
}

//
// If the standin is selected, then any arcs must go only to a selected standin
// and not to an unselected standin.
//
// This is where nodes might automatically be replaced with Transmitter/Reciever
// pairs.  
//
boolean
EditorWindow::areSelectedNodesPagifiable(boolean report)
{
boolean retVal = TRUE;
EditorWorkSpace *ws = NULL;


    List *l = this->makeSelectedNodeList();
    if ((!l) || (!l->getSize())) {
	if (l) delete l;
	return TRUE;
    }

    ListIterator snl(*l);
    Node *node;
    while ((retVal) && (node = (Node*)snl.getNext())) {
	int i,count = node->getInputCount();
	List *conns = NULL;

	for (i=1; ((retVal) && (i<=count)); i++) {
	    Ark *a;
	    List *orig = (List*)node->getInputArks(i);
	    conns = orig->dup();
	    ListIterator ai(*conns);
	    while ( (a = (Ark*)ai.getNext()) ) {
		int fromParam, toParam;
		Node *source = a->getSourceNode (fromParam);
		Node *dest = a->getDestinationNode (toParam);
		ASSERT (dest == node);
		StandIn *ssi = source->getStandIn();
		if ((ssi) && (!ssi->isSelected())) {
		    if ((dest->isA(ClassReceiverNode)) && 
			(source->isA(ClassTransmitterNode)) &&
			(EqualString (dest->getLabelString(),source->getLabelString()))) {
		    } else {
			retVal = FALSE;
			if (report) {
			    char msg[512];
			    sprintf (msg, 
				"Tool %s connects to unselected tool %s.\nSuggestion: ",
				dest->getNameString(), source->getNameString());
			    strcat (msg, 
				"Try the \'Edit/Select Tools/Select Connected\' option");
			    ErrorMessage(msg);
			}
			break;
		    }
		} else if (ssi) {
		    if (!ws) {
			ws = (EditorWorkSpace*)ssi->getWorkSpace();
		    } else if (ws != ssi->getWorkSpace()) {
			retVal = FALSE;
			if (report)
			    ErrorMessage("Tool %s occupies a different page from %s.",
				dest->getNameString(), source->getNameString());
			break;
		    }
		}
	    }
	    delete conns;
	}


	count = node->getOutputCount();
	for (i=1; ((retVal) && (i<=count)); i++) {
	    Ark *a;
	    List *orig = (List*)node->getOutputArks(i);
	    conns = orig->dup();
	    ListIterator ai(*conns);
	    while ( (a = (Ark*)ai.getNext()) ) {
		int fromParam, toParam;
		Node *source = a->getSourceNode (fromParam);
		Node *dest = a->getDestinationNode (toParam);
		ASSERT (source == node);
		StandIn *dsi = dest->getStandIn();
		if ((dsi) && (!dsi->isSelected())) {
		    if ((dest->isA(ClassReceiverNode)) && 
			(source->isA(ClassTransmitterNode)) &&
			(EqualString (dest->getLabelString(),source->getLabelString()))) {
		    } else {
			retVal = FALSE;
			if (report) {
			    char msg[512];
			    sprintf (msg, 
				"Tool %s has a connection from unselected tool %s.\n",
				source->getNameString(), dest->getNameString());
			    strcat (msg, "Suggestion: "
				"Try the \'Edit/Select Tools/Select Connected\' option");
			    ErrorMessage(msg);
			}
			break;
		    }
		} else if (dsi) {
		    if (!ws) {
			ws = (EditorWorkSpace*)dsi->getWorkSpace();
		    } else if (ws != dsi->getWorkSpace()) {
			retVal = FALSE;
			if (report)
			    ErrorMessage("Tool %s occupies a different page from %s.",
				source->getNameString(), dest->getNameString());
			break;
		    }
		}
	    }
	    delete conns;
	}
    }

    delete l;
    return retVal;
}
#endif

//
// Prepare for a new decorator, but don't managed it yet.
//
boolean EditorWindow::placeDecorator()
{
    if (!this->addingDecorators)
	this->addingDecorators = new List;

    DecoratorStyle *ds = 
	DecoratorStyle::GetDecoratorStyle("Annotate", DecoratorStyle::DefaultStyle);
    ASSERT(ds);
    Decorator *d = ds->createDecorator(TRUE);
    ASSERT(d);
    d->setStyle (ds);
 
    DecoratorInfo *dnd = new DecoratorInfo (this->network,
	(void*)this,
	(DragInfoFuncPtr)EditorWindow::SetOwner,
	(DragInfoFuncPtr)EditorWindow::DeleteSelections,
	(DragInfoFuncPtr)EditorWindow::Select);
    d->setDecoratorInfo(dnd);

    this->addingDecorators->appendElement((void*)d);
    this->toolSelector->deselectAllTools();

    //
    // Set the cursor to indicate placement
    //
    ASSERT(this->workSpace);
    this->workSpace->setCursor(UPPER_LEFT);
    return TRUE;
}

//
// Change the style of the selected decorator(s)
//
extern "C" void 
EditorWindow_SetDecoratorStyleCB (Widget w, XtPointer clientData, XtPointer)
{
EditorWindow *editor = (EditorWindow*)clientData;
List *decors = editor->makeSelectedDecoratorList();
long ud;

    XtVaGetValues (w, XmNuserData, &ud, NULL);
    editor->setDecoratorStyle (decors, (DecoratorStyle*)ud);
    if (decors) delete decors;
}

void EditorWindow::setDecoratorStyle (List *decors, DecoratorStyle *ds)
{
    if (!decors) return;
    ASSERT(ds);

    //
    // For interactors, there is InteractorInstance which handles style
    // changes but, for decorators there is nothing.  So, fetch the data,
    // make a new decorator, then replace the data.
    //
    ListIterator it(*decors);
    Decorator *dec;
    Symbol s = theSymbolManager->registerSymbol(ClassLabelDecorator);
    while ( (dec = (Decorator*)it.getNext()) ) {
	if (!dec->isA(s)) continue;

	LabelDecorator *oldlab = (LabelDecorator*)dec;
	Dialog* sdtDiag = oldlab->getDialog();
	oldlab->associateDialog(NUL(Dialog*));
	WorkSpace *parent = oldlab->getWorkSpace();
	oldlab->unmanage();
	const char *val = oldlab->getLabelValue();
	int x,y;
	oldlab->getXYPosition (&x, &y);
	this->network->removeDecoratorFromList((void*)oldlab);

	LabelDecorator *newlab = (LabelDecorator*)ds->createDecorator(TRUE);
	ASSERT(newlab);
	newlab->setStyle(ds);

	DecoratorInfo *dnd = new DecoratorInfo (this->network,
	    (void*)this,
	    (DragInfoFuncPtr)EditorWindow::SetOwner,
	    (DragInfoFuncPtr)EditorWindow::DeleteSelections,
	    (DragInfoFuncPtr)EditorWindow::Select);
	newlab->setDecoratorInfo(dnd);

	if ((val) && (val[0]))
	    newlab->setLabel(val);
	newlab->setXYPosition(x,y);
	this->network->addDecoratorToList ((void*)newlab);

	//
	// Transfer resource settings.
	//
	oldlab->transferResources(newlab);
	newlab->setFont(oldlab->getFont());
	newlab->associateDialog(sdtDiag);
	this->newDecorator (newlab, (EditorWorkSpace*)parent);

	delete oldlab;
    }

    this->network->setFileDirty();
}

//
// Edit/{Cut,Copy} means produce the chunk of .net/.cfg file the same as
// would happen with drag-n-drop and store that chunk somewhere so that it
// can be supplied to a requestor.
//
boolean EditorWindow::cutSelectedNodes()
{
    if (this->copySelectedNodes(TRUE) == FALSE) {
	return FALSE;
    }
    this->removeSelectedNodes();
    this->setUndoActivation();
    return TRUE;
}

#ifdef DXD_OS_NON_UNIX
#define RD_FLAG "rb"
#else
#define RD_FLAG "r"
#endif

boolean EditorWindow::copySelectedNodes(boolean delete_property)
{
char msg[128];
Display *d = XtDisplay(this->getRootWidget());
Atom file_atom = XmInternAtom (d, NET_ATOM, False);
Atom delete_atom = XmInternAtom (d, "DELETE", False);
Screen *screen = XtScreen(this->getRootWidget());
Window root = RootWindowOfScreen(screen);

    int net_len, cfg_len;
    char* cfg_buffer;
    char* net_buffer = this->createNetFileFromSelection(net_len, &cfg_buffer, cfg_len);
    if (!net_buffer) return FALSE;

    //
    // Safeguard:  I don't know if this is necessary.  Maybe calling XtOwnSelection
    // can have unintended consequences if you're already the selection owner?
    // Maybe libXt will tell me I lost the selection because someone took it?
    // So, I'll disown the selection before reasserting ownership.
    //
    Widget w = XtNameToWidget (this->getRootWidget(), "*optionSeparator");
    ASSERT (w);
    XtVaSetValues (w, XmNuserData, this, NULL);
    Time tstamp = XtLastTimestampProcessed(d);
    Window win = XGetSelectionOwner (d, file_atom);
    if ((win != None) && (XtWindow(w) == win)) 
	XtDisownSelection (w, file_atom, tstamp);

    unsigned char del_buf[16];
    if (delete_property) strcpy ((char*)del_buf, "TRUE");
    else strcpy ((char*)del_buf, "FALSE");
    XChangeProperty (d, root, delete_atom, XA_STRING, 8, PropModeReplace,
	del_buf, strlen((char*)del_buf));

    //
    // XtOwnSelection registers 3 callbacks but there is no clientData to pass.
    // Therefore I need some other way to get back the this pointer inside
    // these 3 callbacks.  I am using XmNuserData in the widget for this purpose.
    // I picked such an obscure widget because I suspect that if the widget is the
    // root of some UIComponent, the XmNuserData slot will already be in use.
    //
    boolean retVal;
    if (XtOwnSelection (w, file_atom, tstamp,
	(XtConvertSelectionProc)EditorWindow_ConvertSelectionCB,
	(XtLoseSelectionProc)EditorWindow_LoseSelectionCB,
	(XtSelectionDoneProc)EditorWindow_SelectionDoneCB)) {
	this->pasteNodeCmd->activate();
	retVal = TRUE;
    } else {
	this->pasteNodeCmd->deactivate();
	retVal = FALSE;
    }

    //
    // Assign to copied{Net,Cfg} after owning the selection in case owning also
    // makes you lose it.
    //
    if (this->copiedNet) delete this->copiedNet;
    this->copiedNet = (char *)net_buffer;
    if (this->copiedCfg) delete this->copiedCfg;
    this->copiedCfg = (char *)cfg_buffer;

    return retVal;
}

//
// caller must free the returned memory
//
char* EditorWindow::createNetFileFromSelection(int& net_len, char** cfg_out, int& cfg_len)
{
char netfilename[256];
char cfgfilename[256];
FILE *netf;
char msg[128];

    net_len = cfg_len = 0;
    if (cfg_out) *cfg_out = NUL(char*);

    //
    // It should be unnecessary to perform this check because command activation
    // should prevent being here if both counts are 0, however kbd accelerators
    // can get us here and we might not have any way of preempting that.
    //
    int nselected = this->getNodeSelectionCount();
    int dselected = 0; 
    ListIterator it;
    Decorator *decor;
    FOR_EACH_NETWORK_DECORATOR(this->getNetwork(),decor,it) 
	if (decor->isSelected()) dselected++;
    if ((dselected == 0) && (nselected == 0)) return NUL(char*);

    //
    // Get 2 temp file names for the .net and .cfg files
    // FIXME:  We need a DXApplication::getTmpFile().  The code
    // to form tmp file names off the tmp directory is used in several places.
    //
    const char *tmpdir = theDXApplication->getTmpDirectory();
    int tmpdirlen = STRLEN(tmpdir);
    if (!tmpdirlen) return FALSE;
    if (tmpdir[tmpdirlen-1] == '/') {
	sprintf(netfilename, "%sdx%d.net", tmpdir, getpid());
	sprintf(cfgfilename, "%sdx%d.cfg", tmpdir, getpid());
    } else {
	sprintf(netfilename, "%s/dx%d.net", tmpdir, getpid());
	sprintf(cfgfilename, "%s/dx%d.cfg", tmpdir, getpid());
    }
    unlink (netfilename);
    unlink (cfgfilename);
 
    if ((netf = fopen(netfilename, "a+")) == NULL) {
	sprintf (msg, "Copy failed: %s", strerror(errno));
	WarningMessage(msg);
	return NUL(char*);
    }
 
    //
    // Save .net/.cfg into the temp directory;
    //
    this->network->printNetwork (netf, PrintCut);
    this->network->cfgPrintNetwork (cfgfilename, PrintCut);
    fclose(netf);
 

    //
    // Read the .net and .cfg back it, and save their contents
    //
    netf = NUL(FILE*);
    if ((netf = fopen(netfilename, RD_FLAG)) == NULL) {
	sprintf (msg, "Copy failed (fopen): %s", strerror(errno));
	WarningMessage(msg);
	return NUL(char*);
    }

    unsigned char *cfg_buffer = NULL;

    struct STATSTRUCT statb;
    if (cfg_out) {
	FILE *cfgf = fopen(cfgfilename, RD_FLAG);
	if ((cfgf) && (STATFUNC(cfgfilename, &statb) != -1)) {
	    cfg_len = (unsigned int)statb.st_size;
	    cfg_buffer = (unsigned char *)(cfg_len?new char[1+cfg_len]:NULL);

	    if (fread((char*)cfg_buffer, sizeof(char), cfg_len, cfgf) != cfg_len) {
		sprintf (msg, "Copy failed (fread): %s", strerror(errno));
		WarningMessage(msg);
		fclose(cfgf);
		delete cfg_buffer;
		return NUL(char*);
	    }
	    cfg_buffer[cfg_len] = '\0';
	    fclose(cfgf);
	    cfgf = NUL(FILE*);
	    unlink (cfgfilename);
	}
    }

    if (STATFUNC(netfilename, &statb) == -1) {
	sprintf (msg, "Copy failed (stat): %s", strerror(errno));
	WarningMessage(msg);
	fclose(netf);
        return NUL(char*);
    }
    net_len = (unsigned int)statb.st_size;
    if (!net_len) return NUL(char*);
    if (net_len > 63000) {
	WarningMessage ("Too much data. Try transferring in pieces.");
	fclose(netf);
	return NUL(char*);
    }
    unsigned char *net_buffer = (unsigned char *)(net_len?new char[1+net_len]:NULL);

    if (fread((char*)net_buffer, sizeof(char), net_len, netf) != net_len) {
	sprintf (msg, "Copy failed (fread): %s", strerror(errno));
	WarningMessage(msg);
	fclose(netf);
	delete net_buffer;
        return NUL(char*);
    }
    net_buffer[net_len] = '\0';
    fclose(netf);
    unlink (netfilename);

    if (cfg_out) *cfg_out = (char*)cfg_buffer;
    else if (cfg_buffer) delete cfg_buffer;
    return (char*)net_buffer;
}

//
// Edit/Paste means request the saved chunk of .net/.cfg text and do an
// insertNetwork operation.
//
boolean EditorWindow::pasteCopiedNodes()
{
Display *d = XtDisplay(this->getRootWidget());
Atom file_atom = XmInternAtom (d, NET_ATOM, False);
boolean proceed = TRUE;

    //
    // command activation is handled clumsily for cut/copy/paste because
    // there is no notification when someone gains/loses selection ownership.
    //
    if (file_atom) {
	Window win = XGetSelectionOwner (d, file_atom);
	if (win == None) proceed = FALSE;
    } else
	proceed = FALSE;

    if (!proceed) {
	this->pasteNodeCmd->deactivate();
	return FALSE;
    }

    XtGetSelectionValue (this->getRootWidget(), file_atom, XA_STRING,
	(XtSelectionCallbackProc)EditorWindow_SelectionReadyCB, (XtPointer)this,
	XtLastTimestampProcessed(d));
    return TRUE;
}
void EditorWindow::SetOwner(void *b)
{
EditorWindow *ew = (EditorWindow*)b;
Network *netw = ew->getNetwork();
    netw->setCPSelectionOwner(NUL(ControlPanel*));
}
 
void EditorWindow::DeleteSelections(void *b)
{
EditorWindow *ew = (EditorWindow*)b;
Command *cmd = ew->getDeleteNodeCmd();
 
    cmd->execute();
}

void EditorWindow::Select(void *b)
{
EditorWindow *ew = (EditorWindow*)b;
    ew->setCommandActivation();
}

#if EXPOSE_SET_SELECTION
void EditorWindow::setCopiedNet(const char* text)
{
Display *d = XtDisplay(this->getRootWidget());
Atom file_atom = XmInternAtom (d, NET_ATOM, False);
Atom delete_atom = XmInternAtom (d, "DELETE", False);
Atom cfg_atom = XmInternAtom (d, CFG_ATOM, False);
Screen *screen = XtScreen(this->getRootWidget());
Window root = RootWindowOfScreen(screen);
Time tstamp = XtLastTimestampProcessed(d);

    Widget w = XtNameToWidget (this->getRootWidget(), "*optionSeparator");
    ASSERT (w);

    if (this->copiedNet) {
	delete this->copiedNet;
	this->copiedNet = NUL(char*);
	this->pasteNodeCmd->deactivate();
	XtDisownSelection (w, file_atom, tstamp);
    }
    if (this->copiedCfg) {
	delete this->copiedCfg;
	this->copiedCfg = NUL(char*);
    }

    if ((text) && (text[0])) {
	XtVaSetValues (w, XmNuserData, this, NULL);
	if (XtOwnSelection (w, file_atom, tstamp,
	    (XtConvertSelectionProc)EditorWindow_ConvertSelectionCB,
	    (XtLoseSelectionProc)EditorWindow_LoseSelectionCB,
	    (XtSelectionDoneProc)EditorWindow_SelectionDoneCB))
	    this->pasteNodeCmd->activate();
	else
	    this->pasteNodeCmd->deactivate();

	this->copiedNet = DuplicateString(text);

	unsigned char del_buf[16];
	strcpy ((char*)del_buf, "FALSE");
	XChangeProperty (d, root, delete_atom, XA_STRING, 8, PropModeReplace,
	    del_buf, strlen((char*)del_buf));
    }
}
#endif

boolean EditorWindow_ConvertSelectionCB(Widget w, Atom *selection, Atom *target, 
    Atom *type, XtPointer *value, unsigned long *length, int *format)
{ 
Display *d = XtDisplay(w);
Atom file_atom = XmInternAtom (d, NET_ATOM, False);
Atom cfg_atom = XmInternAtom (d, CFG_ATOM, False);
Screen *screen = XtScreen(w);
Window root = RootWindowOfScreen(screen);

    //
    // The this pointer was supposed to be in userData.  If anyone else is using
    // userData on the widget, then this scheme won't work
    // because these callbacks lack a clientData argument.
    //
    XtPointer ud;
    XtVaGetValues (w, XmNuserData, &ud, NULL);
    EditorWindow *editor = (EditorWindow*)ud;
    ASSERT(editor);

    //
    // FIXME:  there was an ASSERT in place of this if but the ASSERT often failed.
    // It's meaning was:  If this editor received a request for the selection,
    // then it must have asserted ownership of the selection.  If it did that, then
    // it must have stored copiedNet.  I don't understand how that can fail.
    // Maybe I'm doing something wrong in LoseSelectionCB, I don't know.
    //
    // I may have fixed it by disowning the selection prior to owning it.  It turned
    // out that asking to own the selection if I already owned it, would produce
    // a call to my LoseSelection callback which would throw out selection data.
    //
    ASSERT (*selection == file_atom);
    if (editor->copiedNet) {


	*value = editor->copiedNet;
	*length = strlen(editor->copiedNet);
	*format = 8;
	*type = XA_STRING;

	if (editor->copiedCfg)
	    XChangeProperty (d, root, cfg_atom, XA_STRING, 8, PropModeReplace,
		(unsigned char*)editor->copiedCfg, strlen(editor->copiedCfg));

	return TRUE;
    } else {
	XtDisownSelection (w, *selection, XtLastTimestampProcessed(XtDisplay(w)));
	editor->pasteNodeCmd->deactivate();
	return FALSE;
    }
}

void EditorWindow_LoseSelectionCB(Widget w, Atom *)
{
    //
    // The this pointer was supposed to be in userData.  If anyone else is using
    // userData on the widget, then this scheme won't work
    // because these callbacks lack a clientData argument.
    //
    XtPointer ud;
    XtVaGetValues (w, XmNuserData, &ud, NULL);
    EditorWindow *editor = (EditorWindow*)ud;
    ASSERT(editor);

    if (editor->copiedNet) {
	delete editor->copiedNet;
	editor->copiedNet = NUL(char*);
    }
    if (editor->copiedCfg) {
	delete editor->copiedCfg;
	editor->copiedCfg = NUL(char*);
    }
    editor->pasteNodeCmd->deactivate();
}

void EditorWindow_SelectionDoneCB(Widget w, Atom *, Atom *) {}

void EditorWindow_SelectionReadyCB (Widget w, XtPointer clientData, Atom *selection,
    Atom *type, XtPointer value, unsigned long *length, int *format)
{
EditorWindow *editor = (EditorWindow*)clientData;
int status;
FILE *netf;
Display *d = XtDisplay(editor->getRootWidget());
Atom file_atom = XmInternAtom (d, NET_ATOM, False);
Atom cfg_atom = XmInternAtom (d, CFG_ATOM, False);
Screen *screen = XtScreen(editor->getRootWidget());
Window root = RootWindowOfScreen(screen);
unsigned long bytes_after;
char *net_buffer;
Atom actual_type;
int actual_format;
unsigned long n_items;
char msg[128];

    //
    // Get temp file names
    //
    char net_file_name[256];
    char cfg_file_name[256];
    //
    // It's good that this file name is formed using getTmpDirectory()
    // because later we're going to check to see if the file name
    // begins with the tmp directory and if so, leave it out of
    // the list of recently-referenced files.
    //
    const char *tmpdir = theDXApplication->getTmpDirectory();
    int tmpdirlen = STRLEN(tmpdir);
    if (!tmpdirlen) return ;
    if (tmpdir[tmpdirlen-1] == '/') {
	sprintf(net_file_name, "%sdx%d.net", tmpdir, getpid());
	sprintf(cfg_file_name, "%sdx%d.cfg", tmpdir, getpid());
    } else {
	sprintf(net_file_name, "%s/dx%d.net", tmpdir, getpid());
	sprintf(cfg_file_name, "%s/dx%d.cfg", tmpdir, getpid());
    }
    unlink (net_file_name);
    unlink (cfg_file_name);


    //
    // Put the file contents into the files
    //
    if ((netf = fopen(net_file_name, "w")) == NULL) {
	sprintf (msg, "Paste failed (fopen): %s", strerror(errno));
	WarningMessage(msg);
	return ;
    }
    fwrite ((char*)value, sizeof(char), *length, netf);
    fclose(netf);
    XtFree((char*)value);

    status = XGetWindowProperty (d, root, cfg_atom, 0,
	    63000>>2, False, XA_STRING, &actual_type, 
	    &actual_format, &n_items, &bytes_after, 
	    (unsigned char **)&net_buffer);

    if ((status == Success) && (net_buffer)) {
	if ((netf = fopen(cfg_file_name, "w")) == NULL) {
	    sprintf (msg, "Paste failed (fopen): %s", strerror(errno));
	    WarningMessage(msg);
	    unlink (cfg_file_name);
	    return ;
	}
	fwrite (net_buffer, sizeof(char), n_items, netf);
	fclose(netf);
	XFree(net_buffer);
	unlink (cfg_file_name);
    }


    editor->setPendingPaste (net_file_name, NUL(char*), TRUE);

    unlink (net_file_name);

    unsigned char *del_buf;
    Atom delete_atom = XmInternAtom (d, "DELETE", False);
    status = XGetWindowProperty (d, root, delete_atom, 0, 4, False, XA_STRING,
	&actual_type, &actual_format, &n_items, &bytes_after, &del_buf);
    if ((status == Success) && (del_buf) && (!strcmp((char*)del_buf, "FALSE"))) {
    } else {
	XDeleteProperty (d, root, file_atom);
	XDeleteProperty (d, root, cfg_atom);
	//
	// Force no one to own the selection.  XtDisown ought to be superfluous,
	// but I'm not sure how/if the intrinsics find out about things like this
	// done thru Xlib.  We're trying to make sure nobody has the selection,
	// and also to let our own intrinsics know we don't.
	//
	Time tstamp = XtLastTimestampProcessed(d);
	XtDisownSelection (w, file_atom, tstamp);
	XSetSelectionOwner (d, file_atom, None, CurrentTime);
	editor->pasteNodeCmd->deactivate();
    }
    if (del_buf) XFree(del_buf);


    editor->workSpace->setCursor(UPPER_LEFT);
}

//
// Broken out of SelectionReadyCB so that it can be reused by UndoDeletion
//
boolean EditorWindow::setPendingPaste (const char* net_file_name,
	const char* cfg_file_name, boolean ignoreUndefinedModules)
{
    //
    // Prepare a new network
    //
    if (this->addingDecorators) {
	ListIterator it(*this->addingDecorators);
	Decorator *dec;
	while ( (dec = (Decorator*)it.getNext()) )  delete dec;
	this->addingDecorators->clear();
	delete this->addingDecorators;
	this->addingDecorators = NULL;
    }

    if (this->pendingPaste) delete this->pendingPaste;
    this->pendingPaste = theDXApplication->newNetwork(TRUE);
    ASSERT(this->pendingPaste);
    if (!this->pendingPaste->readNetwork (net_file_name, NULL, TRUE)) {
	delete this->pendingPaste;
	this->pendingPaste = NULL;
	WarningMessage ("Paste failed");
	unlink (net_file_name);
	return FALSE;
    }
    this->toolSelector->deselectAllTools();
    return TRUE;
}

//
// Find the last selected transmitter node.  If it no longer exists,
// then find the most recently placed one.  This is useful for the 
// ReceiverNode, which needs to no the most recent Transmitter to attach
// to.
//
TransmitterNode *EditorWindow::getMostRecentTransmitterNode()
{
    Node *node = NULL; 

    //
    // Find all transmitters 
    //
    List *l = this->network->makeClassifiedNodeList(ClassTransmitterNode);

    //
    // Find 
    //   1) the one that is in lastSelectedTransmitter, or
    //   2) the one with the highest instance number.
    //
    if (l) {
        Node *last=NULL;
        ListIterator iter(*l);

	//
	// Look for one that matches the last selected node.
	// Note, that we aren't sure that lastSelectedTransmitter is still
	// in the network.
	//
	if (this->lastSelectedTransmitter) {
	    while ( (last = (Node*)iter.getNext()) ) {
		if (last == this->lastSelectedTransmitter) {
		    node = last;
		    break;	
		}
	    }
	}
	if (!node) {	// If none found that match lastSelectedTransmitter
	    int maxinst = 0;
	    iter.setList(*l);
	    while ( (node = (Node*)iter.getNext()) ) {
		    int newinst = node->getInstanceNumber();
		    if ((maxinst == 0) || (newinst > maxinst)) {
			maxinst = newinst;
			last = node;
		    }
	    }
	    node = last;
        }
        delete l;
    }

    return (TransmitterNode*)node;

}

//
// Visit the most recently executed nodes changing their label color.
// Check every addition to the list for a connection to Transmitter.  If
// found add the Transmitter to the list.  Then add all corresponding
// Receivers to the list also.
//
boolean EditorWindow::showExecutedNodes()
{
    this->deselectAllNodes();

    if (!this->executed_nodes) return FALSE;
    if (!this->executed_nodes->getSize()) return FALSE;

    ListIterator it;
    Node* n;

    //
    // See if the network contains any moduless nodes
    //
    List modless;
    Symbol sym;
    Symbol interactor_sym;
    if (!this->transmitters_added) {
	//
	// Search the network for nodes which aren't backed up by modules.  
	// These will have be to selected depending on what else executed.
	// It does no good to add DXLOutputNode to the list because we're doing
	// a bottom-up search.  DXLOutputNode has no outputs.  I tried adding
	// ClassNondrivenInteractorNode instead of InteractorNode.  The problem
	// there is that some InteractorNodes (subclassing off DrivenNode and not
	// subclassing off NondrivenInteractorNode) are really not driven, so you
	// have to test each one using isDataDriven().
	//
	sym = theSymbolManager->registerSymbol(ClassMacroParameterNode);
	modless.appendElement((void*)sym);
	sym = theSymbolManager->registerSymbol(ClassTransmitterNode);
	modless.appendElement((void*)sym);
	sym = theSymbolManager->registerSymbol(ClassReceiverNode);
	modless.appendElement((void*)sym);
	sym = theSymbolManager->registerSymbol(ClassDXLInputNode);
	modless.appendElement((void*)sym);
	interactor_sym = sym = theSymbolManager->registerSymbol(ClassInteractorNode);
	modless.appendElement((void*)sym);


	ListIterator it(modless);
	boolean found_some = FALSE;
	while ((found_some == FALSE) && (sym = (Symbol)(long)it.getNext())) {
	    const char* cp = theSymbolManager->getSymbolString(sym);
	    ASSERT ((cp) && (cp[0]));
	    List* nodes = this->network->makeClassifiedNodeList(cp);
	    if ((nodes) && (nodes->getSize()))
		found_some = TRUE;
	    if (nodes) 
		delete nodes;
	}
	if (found_some == FALSE)
	    this->transmitters_added = TRUE;
    }

    //
    // Loop until no change to the list of executed_nodes is made.  Must loop
    // because if one moduleless node is added, that guy could be connected to
    // others who should be added, but we won't know until the next pass.
    //
    // FIXME: would someone please rewrite this loop in the form of a recursive
    // subroutine call. 
    //
    boolean change_made = TRUE;
    List appendables;
    List recent1;
    List recent2;
    boolean first_time = TRUE;
    while ((!this->transmitters_added) && (change_made)) {
	int i, j;

	change_made = FALSE;

	//
	// First time thru the loop look at every executed node.  On subsequent 
	// iterations, look only at those nodes which were added on the previous 
	// iteration.
	//
	if (first_time) it.setList(*this->executed_nodes);
	else it.setList(recent1);

	while ( (n = (Node*)it.getNext()) ) {
	    int numParam = n->getInputCount();
	    for (i = 1; i <= numParam; ++i) {
		if (n->isInputConnected(i)) {
		    Ark *a;
		    List *arcs = (List *)n->getInputArks(i);
		    for (j = 1; (a = (Ark*)arcs->getElement(j)); ++j) {
			int paramInd;
			Node *srcNode = a->getSourceNode(paramInd);
			boolean moduleless_node = FALSE;

			ListIterator mit(modless);
			while ( (sym = (Symbol)(long)mit.getNext()) ) {
			    if (srcNode->isA(sym)) {
				if (sym == interactor_sym) {
				    InteractorNode* in = (InteractorNode*)srcNode;
				    if (in->isDataDriven() == FALSE) {
					moduleless_node = TRUE;
					break;
				    }
				} else {
				    moduleless_node = TRUE;
				    break;
				}
			    }
			}

			if (moduleless_node) {
			    if (!appendables.isMember((void*)srcNode)) {
				appendables.appendElement((void*)srcNode);
				recent2.appendElement((void*)srcNode);
				change_made = TRUE;
			    }
			}
		    }
		}
	    }
	}
	recent1.clear();
	it.setList(recent2);
	while ( (n = (Node*)it.getNext()) ) 
	    recent1.appendElement((void*)n);
	recent2.clear();
	first_time = FALSE;
    }
    it.setList(appendables);
    while ( (n = (Node*)it.getNext()) )
	this->executed_nodes->appendElement((void*)n);
    this->transmitters_added = TRUE;

    it.setList(*this->executed_nodes);
    while ( (n = (Node*)it.getNext()) ) 
	this->selectNode(n, TRUE, FALSE);

    return TRUE;
}

//
// A counterpart to newNode().  Used only for vpe annotation.
//
void EditorWindow::newDecorator (Decorator* dec, EditorWorkSpace* where)
{

#if WORKSPACE_PAGES
    const char* cp;
    VPEAnnotator *vpea = (VPEAnnotator*)dec;
    if ( (cp = vpea->getGroupName(theSymbolManager->getSymbol(PAGE_GROUP))) ) {
	EditorWorkSpace *page = this->getPage(cp);
	ASSERT(page);
	//
	// If page is equal to the current page, then manage the decorator.
	//
	int page_num = this->workSpace->getCurrentPage();
	EditorWorkSpace* ews = this->workSpace;
	if (page_num) 
	    ews = (EditorWorkSpace*)this->workSpace->getElement(page_num);
	if (ews == page)
	    vpea->manage(page);
	else
	    page->setMembersInitialized(FALSE);
    } else {
	EditorWorkSpace* current_page = where;
	if (!current_page) {
	    int page = this->workSpace->getCurrentPage();
	    if (page)
		current_page = (EditorWorkSpace*)this->workSpace->getElement(page);
	    else
		current_page = this->workSpace;
	}
	GroupRecord* grec = this->getGroupOfWorkSpace(current_page);
	Symbol page_sym = theSymbolManager->getSymbol(PAGE_GROUP);
	if (grec) vpea->setGroupName (grec, page_sym);
	if (current_page->getRootWidget())
	    vpea->manage(current_page);
	else
	    current_page->setMembersInitialized(FALSE);
    }
#else
    if ((where) && (where->getRootWidget()))
	dec->manage(where);
    else if (this->workSpace->getRootWidget())
	dec->manage(this->workSpace);
#endif
}

//
// EditorWindow maintains a list of greened nodes so that we can go back
// sometime later and see what executed.  Several steps are required to 
// unset the list and the lis must be unset in several situations so
// encapsulate it here.
//
void EditorWindow::resetExecutionList()
{
    if (this->executed_nodes) this->executed_nodes->clear();
    this->showExecutedCmd->deactivate();
    this->transmitters_added = FALSE;
}

#if WORKSPACE_PAGES
void EditorWindow::resetErrorList(boolean reset_all)
{
    if (reset_all) {
	if (this->errored_standins) delete this->errored_standins;
	this->errored_standins = NUL(List*);
    } else if (this->errored_standins) {
	if (this->errored_standins->getSize() == 0) {
	    delete this->errored_standins;
	    this->errored_standins = NUL(List*);
	}
    }

    //
    // reset all page tab colors keeping those yellow which 
    // still have yellow standins
    //
    List yellow_tabs;
    if (this->errored_standins) {
	ListIterator it(*this->errored_standins);
	StandIn* si;
	while ( (si = (StandIn*)it.getNext()) ) {
	    WorkSpace* page = si->getWorkSpace();
	    yellow_tabs.appendElement((void*)page);
	}
    }

    DictionaryIterator di(*this->pageSelector);
    EditorWorkSpace* ews;
    while ( (ews = (EditorWorkSpace*)di.getNextDefinition()) ) 
	if (yellow_tabs.isMember((void*)ews) == FALSE)
	    this->pageSelector->highlightTab (ews, EditorWindow::REMOVEHIGHLIGHT);
}
#endif

#if WORKSPACE_PAGES

int EditorWindow::getPageMemberCount(WorkSpace* page)
{
    const char* page_name = NUL(char*);
    int i,count = this->pageSelector->getSize();
    for (i=1; i<=count; i++) {
	if (page == this->pageSelector->getDefinition(i)) {
	    page_name = this->pageSelector->getStringKey(i);
	    break;
	}
    }
    if (page != this->workSpace) {
	ASSERT(page_name);
    }

    int kidCnt = 0;
    ListIterator it;
    Node* node;
    Decorator* dec;
    Symbol psym = theSymbolManager->getSymbol(PAGE_GROUP);
    FOR_EACH_NETWORK_NODE (this->network, node, it) {
	const char* nodes_page_name = node->getGroupName(psym);
	if ((!nodes_page_name) && ((!page) || (page == this->workSpace))) {
	    kidCnt++;
	} else {
	    if (EqualString(page_name, nodes_page_name)) {
		kidCnt++;
	    }
	}
    }

    FOR_EACH_NETWORK_DECORATOR (this->network, dec, it) {
	VPEAnnotator* vpea = (VPEAnnotator*)dec;
	const char* nodes_page_name = vpea->getGroupName(psym);
	if ((!nodes_page_name) && ((!page) || (page == this->workSpace))) {
	    kidCnt++;
	} else {
	    if (EqualString(page_name, nodes_page_name)) {
		kidCnt++;
	    }
	}
    }
    return kidCnt;
}

void EditorWindow::populatePage(EditorWorkSpace* ews)
{
    Symbol psym = theSymbolManager->getSymbol(PAGE_GROUP);
    GroupRecord* grec = this->getGroupOfWorkSpace(ews);

    //
    // There is a page button associated with a workspace and that workspace
    // has no group?  Shouldn't happen unless you just haven't made the group yet.
    //
    //if (!grec) return ;

    const char* page_name = (grec?grec->getName():NUL(char*));
    Node* node;
    ListIterator it;
    Decorator* dec;

    //
    // Loop over nodes looking for those belonging to the page.  When found
    // check to see if the nodes have standins.  This tells us if we need to
    // do work to populate the page.
    //
    boolean standins_created = FALSE;
    if (ews->membersInitialized() == FALSE) {
	FOR_EACH_NETWORK_NODE (this->network, node, it) {
	    const char* cp = node->getGroupName(psym);
	    if (((cp == NUL(char*)) && (page_name == NUL(char*))) || 
		(EqualString(cp, page_name))) {
		StandIn* si = node->getStandIn();
		if (!si) {
		    if (!standins_created)
			ews->beginManyPlacements();
		    node->newStandIn(ews);
		    standins_created = TRUE;
		    if (node == this->executing_node) 
			this->highlightNode(node, EditorWindow::EXECUTEHIGHLIGHT);
		} else {
		    //
		    // I would like to put a break here but I can't assume that if
		    // the page contains 1 standin, that all nodes in the page have
		    // standins.  Reason:  Edit/Insert Visual Program could have
		    // placed new standins here which haven't been created yet.
		    //
		    //break;
		}
	    }
	}

	//
	// This says we're not going to toggle lineRouting/overlap if we're adding
	// only decorators.  Good?
	//
	FOR_EACH_NETWORK_DECORATOR(this->network,dec,it) {
	    if (dec->getRootWidget()) continue;
	    VPEAnnotator* vpea = (VPEAnnotator*)dec;
	    const char* cp = vpea->getGroupName(psym);
	    if (((cp == NUL(char*)) && (page_name == NUL(char*))) || 
		(EqualString(cp, page_name))) {
		vpea->manage(ews);
	    }
	}
    }

    if (standins_created) {
	boolean old_value = this->creating_new_network;
	this->creating_new_network = TRUE;
	FOR_EACH_NETWORK_NODE (this->network, node, it) {
	    const char* cp = node->getGroupName(psym);
	    if (((cp == NUL(char*)) && (page_name == NUL(char*))) || 
		(EqualString(cp, page_name))) {
		StandIn* si = node->getStandIn();
		ASSERT(si);
		int icnt = node->getInputCount();
		int i;
		for (i=1 ; i<=icnt ; i++) {
		    List* arcs = (List *)node->getInputArks(i);
		    Ark* a;
		    int j;
		    for (j = 1; (a = (Ark*) arcs->getElement(j)); j++) {
			int dummy;
			Node* tn = a->getSourceNode(dummy);
			ASSERT(tn);
			if (tn->getStandIn() == NUL(StandIn*)) continue;
			if (a->getArkStandIn() == NUL(ArkStandIn*))
			    this->notifyArk(a);
		    }
		    si->drawTab(i, FALSE); 
		}
	    }
	}
	this->creating_new_network = old_value;
	ews->endManyPlacements();
	ews->setMembersInitialized();
    }
}
#endif

//
// Change all nodes and decorators in tmpnet so that they think they live
// in ews and belong to the corresponding group.  The is used when dropping
// a selection of network onto a page tab.  An apparent wierdness here is that
// we'll change all nodes in tmpnet without regard to their current grouping.
// That should be OK however, because there is no way using drag-n-drop to
// select nodes which live in more than 1 page/group.
// The try_current_page argument means that we should allow the operation to proceed
// if it looks like we're trying to paste into a page with no group operation.  Letting
// it go ahead will put the nodes in the currently selected page.  That's good behavior
// in the case of Edit/Paste but bad behavior when dropping onto a page tab.
//
boolean EditorWindow::pagifyNetNodes 
(Network* tmpnet, EditorWorkSpace* ews, boolean try_current_page)
{
    Symbol groupID = theSymbolManager->getSymbol(PAGE_GROUP);

    //
    // It's not OK for grec to be NULL because if it's NULL, then when
    // Network::mergeNetworks() works on tmpnet, it won't be able to adjust
    // the group pointers. Allow the operation to proceed if there is no
    // group and there is also no workspace.  In that case just erase the
    // group information and let the chips fall where they may.  That should
    // mean that everything will be placed in whatever is the current page.
    //
    GroupRecord* grec = this->getGroupOfWorkSpace(ews);
    const char* group_name = NUL(char*);
    if (grec) {
	group_name = grec->getName();
    }
    if ((!group_name) && (!try_current_page)) return FALSE;


    //
    // Find the page manager from the old net and make a group record for the
    // new group name.
    //
    Dictionary* dict = tmpnet->getGroupManagers();
    GroupManager* page_mgr = (GroupManager*)dict->findDefinition(groupID);
    //
    // This can only fail if the group already exists, which is OK.
    //
    GroupRecord* new_grec = NUL(GroupRecord*);
    if (group_name) {
	page_mgr->createGroup (group_name, tmpnet);
	new_grec = page_mgr->getGroup(group_name);
	if (!new_grec) return FALSE;
    }


    Node* node;
    ListIterator li;
    Decorator* dec;
    FOR_EACH_NETWORK_NODE(tmpnet, node, li) {
	node->setGroupName(new_grec, groupID);
    }
    FOR_EACH_NETWORK_DECORATOR(tmpnet, dec, li) {
	VPEAnnotator* vpea = (VPEAnnotator*)dec;
	vpea->setGroupName(new_grec, groupID);
    }
    return TRUE;
}

boolean EditorWindow::pagifySelectedNodes(EditorWorkSpace* ews)
{
    Symbol groupID = theSymbolManager->getSymbol(PAGE_GROUP);

    GroupRecord* grec = this->getGroupOfWorkSpace(ews);
    const char* group_name = NUL(char*);
    if (grec) group_name = grec->getName();
    if (!group_name) return FALSE;

    List* selno = this->makeSelectedNodeList();
    List* seldec = this->makeSelectedDecoratorList();
    EditorWorkSpace* selectedWS = NUL(EditorWorkSpace*);

    if ((selno) && (selno->getSize())) {
	Node* n = (Node*)selno->getElement(1);
	StandIn* si = (StandIn*)n->getStandIn();
	selectedWS = (EditorWorkSpace*)si->getWorkSpace();
    } else if ((seldec) && (seldec->getSize())) {
	Decorator* dec = (Decorator*)seldec->getElement(1);
	selectedWS = (EditorWorkSpace*)dec->getWorkSpace();
    }


    if (selectedWS) selectedWS->beginManyPlacements(); 
    if (seldec) {
	ListIterator it(*seldec);
	VPEAnnotator* dec;
	while ( (dec = (VPEAnnotator*)it.getNext()) ) 
	    dec->setGroupName(grec, groupID);
	this->moveDecorators (ews, seldec, 20, 20, selno);
    }
    if (selno) {
	ListIterator it(*selno);
	Node* n;
	while ( (n = (Node*)it.getNext()) ) 
	    n->setGroupName(grec, groupID);
	this->moveStandIns (ews, selno, 20, 20, seldec);
    }
    if (selectedWS) selectedWS->endManyPlacements();
    if (selno) delete selno;
    if (seldec) delete seldec;

    this->setCommandActivation();
    return TRUE;
}

boolean EditorWindow::configurePage()
{
    this->pageSelector->postPageNameDialog();
    return TRUE;
}
boolean EditorWindow::postMoveSelectedDialog()
{
    this->pageSelector->postMoveNodesDialog();
    this->setCommandActivation();
    return TRUE;
}

//
// Break any arc which connects a selected node to an unselected node.
// Replace the arc with transmitter/receiver combination.
//
boolean EditorWindow::autoChopSelectedNodes()
{
    List* sellist = this->makeSelectedNodeList();
    if ((sellist == NUL(List*)) || (sellist->getSize() == 0))
	return TRUE;

    Network* net = this->getNetwork();
    Dictionary tmits;
    Dictionary rcvrs;
    boolean status = net->chopArks(sellist, &tmits, &rcvrs);

    DictionaryIterator di(rcvrs);
    Node* n;
    while ( (n = (Node*)di.getNextDefinition()) ) {
        StandIn* si = n->getStandIn();
	if (si) {
	    List* arcs = (List *)n->getOutputArks(1);
	    int j;
	    Ark* a;
	    for (j = 1; (a = (Ark*) arcs->getElement(j)) ; j++) 
		this->notifyArk(a);
	    si->drawTab(1, FALSE);     
	}
    }
    delete sellist;

    if (this->selectConnectedNodeCmd->isActive())
	this->selectConnectedNodeCmd->execute();

    return status;
}

boolean EditorWindow::autoFuseSelectedNodes()
{
ListIterator it;
Node* n;
List rcvrs;
int dummy;

    List* sellist = this->makeSelectedNodeList();
    if ((sellist == NUL(List*)) || (sellist->getSize() == 0))
	return TRUE;

    Network* net = this->getNetwork();

    it.setList(*sellist);
    while ( (n = (Node*)it.getNext()) )
	if (n->isA(ClassReceiverNode))
	    rcvrs.appendElement((void*)n);

    //
    // If the original list contains only a transmitter(s), then augment the
    // list by adding all the receivers they're feeding.  In other words, if
    // the user selects a transmitter and asks to rewire it, then assume
    // she wants to get rid of all corresponding receivers.
    //
    if (rcvrs.getSize() == 0) {
	it.setList(*sellist);
	while ( (n = (Node*)it.getNext()) ) {
	    if (n->isA(ClassTransmitterNode) == FALSE) continue;
	    List* orig = (List*)n->getOutputArks(1);
	    if ((!orig) || (orig->getSize() == 0)) continue;
	    ListIterator ai(*orig);
	    Ark* a;
	    while ( (a = (Ark*)ai.getNext()) ) {
		Node* rcvr = a->getDestinationNode(dummy);
		ASSERT(rcvr->isA(ClassReceiverNode));
		rcvrs.appendElement((void*)rcvr);
	    }
	}
    }

    //
    // Make a new list by augmenting the original.  For every
    // non-(transmitter/receiver) node , if its got no selected
    // receiver node inputs, then add all its receiver node inputs.
    //
    it.setList(*sellist);
    while ( (n = (Node*)it.getNext()) ) {
	if (n->isA(ClassTransmitterNode)) continue;
	if (n->isA(ClassReceiverNode)) continue;
	int icnt = n->getInputCount();
	int i;
	List toadd;
	boolean unusable_receiver = FALSE;
	for (i=1; i<=icnt; i++) {
	    List* orig = (List*)n->getInputArks(i);
	    if ((!orig) || (orig->getSize() == 0)) continue;
	    ASSERT(orig->getSize() == 1);
	    Ark* a = (Ark*)orig->getElement(1);;
	    Node* src = a->getSourceNode(dummy);
	    if (src->isA(ClassReceiverNode)) {
		List* oa = (List*)src->getOutputArks(1);
		ASSERT(oa);
		if (oa->getSize() == 1) {
		    toadd.appendElement((void*)src);
		} else {
		    ListIterator oi(*oa);
		    Ark* a;
		    boolean included = TRUE;
		    while ( (a = (Ark*)oi.getNext()) ) {
			Node* dest = a->getDestinationNode(dummy);
			if (sellist->isMember((void*)dest) == FALSE) {
			    included = FALSE;
			    break;
			}
		    }
		    if (included)
			toadd.appendElement((void*)src);
		}
		StandIn* si = src->getStandIn();
		if (si) unusable_receiver = si->isSelected();
	    }
	    if (unusable_receiver) break;
	}
	if (unusable_receiver == FALSE) {
	    ListIterator ta(toadd);
	    Node* add;
	    while ( (add = (Node*)ta.getNext()) ) {
		if (rcvrs.isMember(add) == FALSE)
		    rcvrs.appendElement((void*)add);
	    }
	}
    }

    //
    // Network::replaceInputArks() operates stritly on 
    // the basis of the Receviers it encounters
    //
    boolean status = net->replaceInputArks(&rcvrs, NUL(List*));

    delete sellist;

    return status;
}

boolean EditorWindow::toggleHitDetection()
{
    WorkSpaceInfo *wsinfo = this->workSpace->getInfo();
    ToggleButtonInterface* tbi = (ToggleButtonInterface*)
	this->hitDetectionOption;
    this->hit_detection = tbi->getState();
    wsinfo->setPreventOverlap(this->hit_detection);
    this->workSpace->installInfo(NULL);
    return TRUE;
}

String EditorWindow::SequenceNet[] = {
#include "sequence.h"
};

boolean EditorWindow::unjavifyNetwork()
{
    List* rlist = this->network->makeClassifiedNodeList(ClassReceiverNode);
    if (rlist) {
	List to_delete;
	ListIterator it(*rlist);
	Node* n;
	while ( (n = (Node*)it.getNext()) ) {
	    if (EqualString(n->getLabelString(), JAVA_SEQUENCE))
		to_delete.appendElement((void*)n);
	}

	if (to_delete.getSize() > 0)
	    this->deleteNodes(&to_delete);
	delete rlist;
	rlist = NUL(List*);
    }
    //
    // Before asking to delete the page, make sure that the page
    // exists. Reason: deletePage will delete "Untitled" otherwise.
    //
    EditorWorkSpace* ews = (EditorWorkSpace*)
	this->pageSelector->findDefinition(JAVA_SEQ_PAGE);
    if (ews != NUL(EditorWorkSpace*)) {
	this->pageSelector->selectPage(ews);
	this->deletePage(NUL(char*));
    }

    rlist = (List*)this->network->makeNamedNodeList("WebOptions");
    if (rlist) {
	this->deleteNodes(rlist);
	delete rlist;
	rlist = NUL(List*);
    }

    rlist = (List*)this->network->makeClassifiedNodeList(ClassImageNode);
    if (rlist) {
	ListIterator it(*rlist);
	ImageNode* n;
	while ( (n = (ImageNode*)it.getNext()) ) 
	    n->unjavifyNode();
	delete rlist;
	rlist = NUL(List*);
    }

    return TRUE;
}

//
// Add the special java page.  Then merge in a network.
//
boolean EditorWindow::javifyNetwork()
{
    //
    // We must be able to create an instance of the WebOptions macro
    //
    NodeDefinition* wopt_nd = (NodeDefinition*)
	theNodeDefinitionDictionary->findDefinition("WebOptions");

    if (!wopt_nd) {
	// try loading web-options
	char* macros = "/java/server/dxmacros";
	const char* uiroot = theDXApplication->getUIRoot();
	char* jxmacros;
	fprintf(stderr, "WebOptions macro not in DXMACROS path\n");
	fprintf(stderr, "attempting to load from configure-time parameters\n");
    	if (!uiroot)
        	uiroot = "/usr/local/dx";
	jxmacros=(char*)malloc(strlen(uiroot)+strlen(macros)+2);
	strcpy(jxmacros,uiroot);
	strcat(jxmacros,macros);
	MacroDefinition::LoadMacroDirectories(jxmacros,TRUE,NULL,FALSE);
	free(jxmacros);
    	wopt_nd = (NodeDefinition*) theNodeDefinitionDictionary->findDefinition("WebOptions");
    }
    if (!wopt_nd) {
	ErrorMessage (
	    "The definition of macro WebOptions is missing.\n"
	    "Load the macros from /usr/local/dx/java/server/dxmacros.\n"
	    "Otherwise, this visual program will not function properly\n"
	    "under control of DXServer.\n");
	return FALSE;
    }


    //
    // Open a tmp file and write out sequence.net then read it
    // back into a temporary Network.
    //
    GroupManager *page_mgr = (GroupManager*)
	this->network->getGroupManagers()->findDefinition(PAGE_GROUP);
    Symbol gmgr_sym = page_mgr->getManagerSymbol();
    if (this->pageSelector->findDefinition(JAVA_SEQ_PAGE) == NULL) {
	const char* tmpdir = theIBMApplication->getTmpDirectory();
	char uniq_file[512];
	sprintf (uniq_file, "%s/foo.bar", tmpdir);
	char* holder_file = UniqueFilename(uniq_file);
	FILE* holder = fopen(holder_file, "w");
	sprintf (uniq_file, "%s.net", holder_file);
	FILE* uniq_f = fopen(uniq_file, "w");
	
	int ll;
	for (ll = 0; EditorWindow::SequenceNet[ll] != NULL; ll += 1)
		fprintf (uniq_f, "%s", EditorWindow::SequenceNet[ll]);
	fclose(uniq_f);
	uniq_f = NUL(FILE*);

	//
	// Merging in the temporary network puts the nodes into java seqno page.
	//
	boolean non_java_net = TRUE;
	Network* tmpnet = theDXApplication->newNetwork(non_java_net);
	ASSERT(tmpnet);
	boolean status = tmpnet->readNetwork(uniq_file, NULL, TRUE);
	unlink (uniq_file);
	fclose(holder);
	unlink(holder_file);
	delete holder_file;
	if (status == FALSE) {
	    delete tmpnet;
	    return FALSE;
	}

	//
	// Create a new page group in the network.
	// Creating the new group also automatically marks selected nodes as belonging.
	//
	const char* page_name = JAVA_SEQ_PAGE;
	this->deselectAllNodes();
	ASSERT(page_mgr->createGroup (page_name, this->network));

	//
	// Make the new WorkSpace page. 
	//
	EditorWorkSpace *page = (EditorWorkSpace*)this->workSpace->addPage();
	ASSERT (this->pageSelector->addDefinition (page_name, page));
	PageGroupRecord *grec = (PageGroupRecord*)page_mgr->getGroup (page_name);
	grec->setComponents (NUL(UIComponent*), page);
	this->workSpace->showWorkSpace(page);
	gmgr_sym = page_mgr->getManagerSymbol();

	this->network->mergeNetworks(tmpnet, NUL(List*), TRUE);
	delete tmpnet;
    }

    //
    // Look for Image tools.  For each one, connect 2 transmitters - 1 for
    // loop number, the other for sequence number.
    //
    List* imgs = this->network->makeClassifiedNodeList(ClassImageNode);
    if (imgs) {
	int x, y;
	NodeDefinition *rcvr_nd = (NodeDefinition*)
	    theNodeDefinitionDictionary->findDefinition("Receiver");
	ASSERT(rcvr_nd);

	ListIterator it(*imgs);
	ImageNode* bn;
	while ( (bn = (ImageNode*)it.getNext()) ) {
	    Node* rcvr = NUL(Node*);
	    Node* wopt = NUL(Node*);
	    boolean adding_rcvr = TRUE;
	    if (bn->isJavified() == FALSE) {
		bn->getVpePosition(&x,&y);

		List* rlist = this->network->makeClassifiedNodeList(ClassReceiverNode);
		if (rlist) {
		    const char* current_page_name = bn->getGroupName(gmgr_sym);
		    ListIterator it(*rlist);
		    Node* n;
		    while ( (n = (Node*)it.getNext()) ) {
			boolean same_page = FALSE;
			if (EqualString(JAVA_SEQUENCE, n->getLabelString())) {
			    const char* page_name = n->getGroupName(gmgr_sym);
			    if (EqualString(page_name, current_page_name)) {
				same_page = TRUE;
				rcvr = (ReceiverNode*)n;
			    }
			}
			if (same_page) {
			    adding_rcvr = FALSE;
			    break;
			}
		    }
		    delete rlist;
		    rlist = NUL(List*);
		}

		if (adding_rcvr) {
		    rcvr = rcvr_nd->createNewNode(this->network);
		    this->network->copyGroupInfo(bn, rcvr);
		    rcvr->setVpePosition (x + 90, MAX(0,y-250));
		    rcvr->setLabelString (JAVA_SEQUENCE);
		} else {
		    ASSERT(rcvr);
		}

		wopt = wopt_nd->createNewNode(this->network);
		this->network->copyGroupInfo(bn, wopt);

		const char* group_name = bn->getGroupName(page_mgr->getManagerSymbol());
		EditorWorkSpace* ews = 
		    (EditorWorkSpace*)this->pageSelector->findDefinition(group_name);
		if (!ews) {
		    ews = (EditorWorkSpace*)this->workSpace;
		    this->workSpace->showWorkSpace(ews);
		}

		wopt->setVpePosition (x + 40, MAX(0,y-100));

		//this->network->addNode(wopt, ews);
		this->network->addNode(wopt, NUL(EditorWorkSpace*));
		if (adding_rcvr) {
		    //this->network->addNode(rcvr, ews);
		    this->network->addNode(rcvr, NUL(EditorWorkSpace*));
		}
	    }
	    bn->javifyNode(wopt, rcvr);
	}
	delete imgs;
    }

    return TRUE;
}

boolean EditorWindow::reflowEntireGraph()
{
    WorkSpace *current_ws = this->workSpace;
    int page = this->workSpace->getCurrentPage();
    if (page) current_ws = this->workSpace->getElement(page);
    if (!this->layout_controller)
	this->layout_controller = new GraphLayout(this);
    boolean retval = this->layout_controller->entireGraph(
	    current_ws, this->network->nodeList, this->network->decoratorList);

    // This will cause the ui to prompt the user to save.  In the past we
    // always tried to avoid this prompt if repositioning of standIns was
    // the only change.  We'll see if this is nice.
    if (retval) this->network->setFileDirty();
    return retval;
}

//
// ewsc can be a StandIn or a VPEAnnotation.  A callback from the workspace
// widget has told us that the component has been moved.  We fetch its
// location (which is still the old location) and save it so that it can
// be undone.
//
void EditorWindow::saveLocationForUndo (UIComponent* uic, boolean mouse, boolean same_event)
{
    boolean separator_pushed = same_event||this->moving_many_standins;
    if (this->performing_undo) return ;

    boolean found_it = FALSE;
    if ((TRUE)||(mouse)||(this->moving_many_standins)) {
	ASSERT(uic);
	ListIterator iterator;
	Node* n;
	FOR_EACH_NETWORK_NODE(this->network, n, iterator) {
	    if (n->getStandIn() == uic) {
		StandIn* si = (StandIn*)uic;

		// error check
		// can't do this error check because something that affects
		// a standIn's label can cause  bumper cars even if those
		// standIns aren't in the current page
		//WorkSpace *current_ws = this->workSpace;
		//int page = this->workSpace->getCurrentPage();
		//if (page) current_ws = this->workSpace->getElement(page);
		//ASSERT (current_ws == si->getWorkSpace());

		if (!separator_pushed) {
		    this->undo_list.push(new UndoSeparator(this));
		    separator_pushed = TRUE;
		}

		this->undo_list.push ( new UndoStandInMove (
			this, si, n->getNameString(), n->getInstanceNumber()
		    )
		);
		found_it = TRUE;
		break;
	    }
	}
	Decorator* dec;
	FOR_EACH_NETWORK_DECORATOR(this->network, dec, iterator) {
	    if (dec == uic) {

		if (!separator_pushed) {
		    this->undo_list.push(new UndoSeparator(this));
		    separator_pushed = TRUE;
		}

		this->undo_list.push (new UndoDecoratorMove (this, (VPEAnnotator*)dec));
		found_it = TRUE;
		break;
	    }
	}
	// don't do this because if you drop one thing on top of
	// another, someone might unmanage a widget which means
	// we'll not be able to find out what happened.
	//ASSERT(found_it);
    }

    // this->setCommandActivation() doesn't run often enough to deal
    // with accelerators.
    if ((found_it) && (!this->undoCmd->isActive()))
	this->setUndoActivation();
}

void EditorWindow::clearUndoList()
{
    if (this->undo_list.getSize() == 0) return;
    UndoableAction* undoable;
    while ((undoable=(UndoableAction*)this->undo_list.pop())) 
	delete undoable;
    this->undo_list.clear();
    this->setUndoActivation();
}

void EditorWindow::beginMultipleCanvasMovements()
{
    this->moving_many_standins = TRUE;
    this->undo_list.push(new UndoSeparator(this));
}

boolean EditorWindow::undo()
{
    boolean many_placements = TRUE;//(this->undo_list.getSize() >= 2);
    WorkSpace *current_ws = this->workSpace;
    if (many_placements) {
	int page = this->workSpace->getCurrentPage();
	if (page) current_ws = this->workSpace->getElement(page);
	current_ws->beginManyPlacements();
    }
    this->performing_undo = TRUE;
    UndoableAction* undoable;

    Stack short_stack;
    List undo_candidates;
    ListIterator iter;
    const char* couldnt_undo = NUL(const char*);
    int count = 0;
    while ((undoable=(UndoableAction*)this->undo_list.pop())) {
	if (undoable->isSeparator()) break;
	undoable->prepare();
	undo_candidates.appendElement(undoable);
	count++;
    }
    for (int i=count; i>=1; i--) {
	undoable = (UndoableAction*)undo_candidates.getElement(i);
	if (undoable->canUndo()) {
	    short_stack.push(undoable);
	} else {
	    couldnt_undo = undoable->getLabel();
	}
    }


    if (couldnt_undo) {
	if (short_stack.getSize() == 0) {
	    WarningMessage( "An operation could not be undone: %s", couldnt_undo);
	} else {
	    ErrorMessage ("Part of the operation could not be undone: %s", couldnt_undo);
	}
    }

    boolean first_in_list = TRUE;
    while ((undoable=(UndoableAction*)short_stack.pop())) {
	undoable->undo(first_in_list);
	first_in_list = FALSE;
    }

    iter.setList(undo_candidates);
    while (undoable = (UndoableAction*)iter.getNext()) {
	undoable->postpare();
    }

    this->performing_undo = FALSE;
    if (many_placements) {
	current_ws->endManyPlacements();
    }

    this->setUndoActivation();

    return TRUE;
}

void EditorWindow::setUndoActivation()
{
    //if (this->deferrableCommandActivation->isActionDeferred()) return ;

    // These 2 numbers work like the float on your sump pump.  When
    // the level gets really high (up to check_when) the pump runs
    // and gets rid of material until it's low (down to max_stack_prune).
    // how many individual events are we going to keep around?
    int max_stack_prune = 10;
    // how big does the stack get before we check it for pruning.
    int check_when = 30;

    // set up for subsequent undo
    char button_label[64];
    strcpy (button_label, "Undo");
    int current_size = this->undo_list.getSize();
    if (current_size > 0) {

	UndoableAction* undoable;
	if (current_size >= check_when) {
	    int saved = 0;
	    Stack tmpStack;
	    while ((undoable=(UndoableAction*)this->undo_list.pop())) {
		if (undoable->isSeparator()) {
		    saved++;
		    tmpStack.push(undoable);
		} else if (saved < max_stack_prune) {
		    tmpStack.push(undoable);
		} else {
		    break;
		}
	    }
	    while ((undoable=(UndoableAction*)this->undo_list.pop())) 
		delete undoable;

	    while(undoable = (UndoableAction*)tmpStack.pop()) {
		this->undo_list.push(undoable);
	    }
	}

	undoable = (UndoableAction*)this->undo_list.peek();
	const char* cp = undoable->getLabel();
	strcat (button_label, " ");
	strcat (button_label, cp);
	this->undoCmd->activate();
    } else {
	this->undoCmd->deactivate();
    }
    XmString xmstr = XmStringCreateLtoR (button_label, "bold");
    XtVaSetValues (this->undoOption->getRootWidget(), XmNlabelString, xmstr, NULL);
    XmStringFree(xmstr);
}

boolean EditorWindow::KeyHandler(XEvent *event, void *clientData)
{
    EditorWindow *ew = (EditorWindow*)clientData;
    return ew->keyHandler(event);
}

#if !defined(XK_MISCELLANY)
#define XK_MISCELLANY
#endif
#include <X11/keysym.h>
boolean EditorWindow::keyHandler(XEvent* event)
{
    if (event->type != KeyPress) return TRUE;

    KeySym lookedup = XLookupKeysym(((XKeyEvent*)event), 0);
    if ((lookedup!=XK_End) && (lookedup!=XK_Page_Up) && (lookedup!=XK_Page_Down)) 
	return TRUE;

    Widget ww = XtWindowToWidget(XtDisplay(this->getRootWidget()), event->xany.window);
    boolean is_descendant = FALSE;
    //
    // Choice of window is significant.  By using workArea, we're grabbing any
    // event that is in the vpe or tool selector.  I thought it would be OK
    // to get only the events inside the scrolledWindow however, the tool selector
    // crashes when given pg-up/pg-down also.  It would be nice if there were
    // a better way of computing ancestory.
    //
    // Window win = XtWindow(this->scrolledWindow);
    //
    Window win = XtWindow(this->workArea);
    while ((ww) && (!XtIsShell(ww))) {
	if (XtWindow(ww) == win) {
	    is_descendant = TRUE;
	    break;
	}
	ww = XtParent(ww);
    }
    if (!is_descendant) return TRUE;

    XmScrolledWindowWidget scrollWindow;
    int xsize,xdelta,xpdelta,x;
    int ysize,ydelta,ypdelta,y;

    //
    // Here, it would be nice to fetch the vertical scrollbar's max value
    // and use that in the XK_End calculation, but that causes a core dump
    // inside  Motif the same way Page-Up/Page-Down do.
    //
    //int max;
    //XtVaGetValues ((Widget)scrollWindow->swindow.vScrollBar, XmNmaximum, &max, NULL);

    scrollWindow = (XmScrolledWindowWidget)this->getScrolledWindow();
    XmScrollBarGetValues((Widget)scrollWindow->swindow.hScrollBar,
			 &x, &xsize, &xdelta, &xpdelta);
    XmScrollBarGetValues((Widget)scrollWindow->swindow.vScrollBar,
			 &y, &ysize, &ydelta, &ypdelta);
    if (lookedup == XK_Page_Up) {
	y-=ypdelta;
	y=MAX(y,0);
    } else if (lookedup == XK_Page_Down) {
	y+=ypdelta;
    } else if (lookedup == XK_End) {
	//y = max-(ysize+1);
	y+=2*ypdelta;
    }
    this->moveWorkspaceWindow(x,y,FALSE);
    return FALSE;
}

void  EditorWindow::saveAllLocationsForUndo (UndoableAction* gridding)
{
    this->undo_list.push(new UndoSeparator(this));
    Node* n;
    ListIterator iterator;
    FOR_EACH_NETWORK_NODE(this->network, n, iterator) {
	StandIn* si = n->getStandIn();
	if (!si) continue;

	this->undo_list.push ( new UndoStandInMove (
		this, si, n->getNameString(), n->getInstanceNumber()
	    )
	);
    }

    Decorator* dec;
    FOR_EACH_NETWORK_DECORATOR(this->network, dec, iterator) {
	if (dec->getRootWidget())
	    this->undo_list.push (new UndoDecoratorMove (this, (VPEAnnotator*)dec));
    }
    this->undo_list.push(gridding);
 
    // this->setCommandActivation() doesn't run often enough to deal
    // with accelerators.
    if (!this->undoCmd->isActive())
	this->setUndoActivation();
}
