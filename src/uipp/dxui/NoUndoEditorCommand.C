/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#include "NoUndoEditorCommand.h"
#include "EditorWindow.h"
#include "Cacheability.h"
#include "Network.h"
#include "ControlPanel.h"
#include "ControlPanelGroupDialog.h"
#include "ControlPanelAccessDialog.h"

class PanelGroupManager;

NoUndoEditorCommand::NoUndoEditorCommand(const char*   name,
				       CommandScope* scope,
				       boolean       active,
				       EditorWindow  *editor,
				       EditorCommandType comType,
       				       EditorUndoType undoType) :
	NoUndoCommand(name, scope, active)
{
	this->commandType = comType;
	this->editor = editor;
	this->undoType = undoType;
}


boolean NoUndoEditorCommand::doIt(CommandInterface *ci)
{
    EditorWindow *editor = this->editor;
    Network *n = editor->getNetwork();
    ControlPanel *panel;
    boolean     ret = TRUE;

    ASSERT(editor);

    switch (this->commandType) {
	case NoUndoEditorCommand::ShowConfiguration:
	    editor->showValues();
	    break;
	case NoUndoEditorCommand::SelectAll:
    	    editor->selectAllNodes();
	    break;
	case NoUndoEditorCommand::DeselectAll:
    	    editor->deselectAllNodes();
	    break;
	case NoUndoEditorCommand::NewControlPanel: // Open a new panel 
	    panel = n->newControlPanel();
    	    panel->manage();
	    panel->addSelectedInteractorNodes(); // Wait for panel realization
	    break;
	case NoUndoEditorCommand::OpenControlPanel: //Open selected interactors 
	    editor->openSelectedINodes();  
	    break;
	case NoUndoEditorCommand::OpenSelectedColormaps:
	    editor->network->openColormap(FALSE);
	    break;
	case NoUndoEditorCommand::OpenSelectedMacros:
	    editor->openSelectedMacros();
	    break;
	case NoUndoEditorCommand::OpenSelectedImageWindows:
	    editor->openSelectedImageWindows();
	    break;

	case NoUndoEditorCommand::RemoveInputTab:// Remove repeatable tabs 
	    ret = editor->editSelectedNodeTabs(FALSE,TRUE);
	    break;
	case NoUndoEditorCommand::AddInputTab:
	    ret = editor->editSelectedNodeTabs(TRUE,TRUE);
	    break;
	case NoUndoEditorCommand::RemoveOutputTab:// Remove repeatable tabs 
	    ret = editor->editSelectedNodeTabs(FALSE, FALSE);
	    break;
	case NoUndoEditorCommand::AddOutputTab:
	    ret = editor->editSelectedNodeTabs(TRUE, FALSE);
	    break;
	case NoUndoEditorCommand::HideAllTabs:
	    ret = editor->setSelectedNodeTabVisibility(FALSE);
	    break;
	case NoUndoEditorCommand::RevealAllTabs:
	    ret = editor->setSelectedNodeTabVisibility(TRUE);
	    break;
	case NoUndoEditorCommand::SelectDownward:// Select all down stream nodes
	    ret = editor->selectDownwardNodes();
	    break;
	case NoUndoEditorCommand::SelectUpward: // Select all up stream nodes
	    ret = editor->selectUpwardNodes();
	    break;
	case NoUndoEditorCommand::SelectConnected:// Select all connected nodes
	    ret = editor->selectConnectedNodes();
	    break;
	case NoUndoEditorCommand::SelectUnconnected:// Select all unconnected
	    ret = editor->selectUnconnectedNodes();
	    break;
	case NoUndoEditorCommand::SelectUnselected:// Select all unselected 
	    editor->selectUnselectedNodes();
	    break;
	case NoUndoEditorCommand::SetCPAccess: // Open the Set CP Access dialog
	    editor->postPanelAccessDialog(
			editor->getNetwork()->getPanelAccessManager());
	    break; 
	case NoUndoEditorCommand::SetPanelGroup: // Open the SetGroup dialog
	    editor->postPanelGroupDialog(); 
	    break; 
	case NoUndoEditorCommand::OpenGrid: // Open the Grid dialog 
	    editor->postGridDialog();
	    break;
	case NoUndoEditorCommand::OpenFindTool: // Open the FindTool dialog 
	    editor->postFindToolDialog();
	    break;
	case NoUndoEditorCommand::EditComment: 	// Change the macro comment 
	    n->editNetworkComment();
	    break;
	case NoUndoEditorCommand::CreateProcessGroup:
	    editor->postProcessGroupCreateDialog();
	    break;
	case NoUndoEditorCommand::PrintProgram:
	    editor->postPrintProgramDialog();
	    break;
	case NoUndoEditorCommand::InsertNetwork:
	    editor->postInsertNetworkDialog();
	    break;
	case NoUndoEditorCommand::Macroify:
	    if (this->editor->areSelectedNodesMacrofiable())
		editor->postCreateMacroDialog();
	    else
		ret = FALSE;
	    break;
	case NoUndoEditorCommand::SetOutputsNotCached:
	    editor->setSelectedNodesOutputCacheability(OutputNotCached);
	    break;
	case NoUndoEditorCommand::SetOutputsFullyCached:
	    editor->setSelectedNodesOutputCacheability(OutputFullyCached);
	    break;
	case NoUndoEditorCommand::SetOutputsCacheOnce:
	    editor->setSelectedNodesOutputCacheability(OutputCacheOnce);
	    break;
	case NoUndoEditorCommand::ShowOutputsNotCached:
	    editor->selectNodesWithOutputCacheability(OutputNotCached);
	    break;
	case NoUndoEditorCommand::ShowOutputsFullyCached:
	    editor->selectNodesWithOutputCacheability(OutputFullyCached);
	    break;
	case NoUndoEditorCommand::ShowOutputsCacheOnce:
	    editor->selectNodesWithOutputCacheability(OutputCacheOnce);
	    break;
	case NoUndoEditorCommand::OptimizeOutputCacheability:
	    editor->optimizeNodeOutputCacheability();
	    break;
	case NoUndoEditorCommand::SaveAsCCode:
	    editor->postSaveAsCCodeDialog();
	    break;
#ifndef FORGET_GETSET
	    // editor is not the correct vpe for these commands.
	case NoUndoEditorCommand::ToLocal:
	    EditorWindow::ConvertToLocal();
	    break;
	case NoUndoEditorCommand::ToGlobal:
	    EditorWindow::ConvertToGlobal();
	    break;
	case NoUndoEditorCommand::SelectedToGlobal:
	    editor->convertToGlobal(TRUE);
	    break;
	case NoUndoEditorCommand::SelectedToLocal:
	    editor->convertToGlobal(FALSE);
	    break;
	case NoUndoEditorCommand::PostGetSet:
	    editor->postGetSetConversionDialog();
	    break;
#endif
	case NoUndoEditorCommand::AddAnnotation:
	    editor->placeDecorator();
	    break;
#if WORKSPACE_PAGES
	case NoUndoEditorCommand::Pagify:
	    editor->pagifySelectedNodes((boolean)FALSE);
	    break;
	case NoUndoEditorCommand::PagifySelected:
	    editor->pagifySelectedNodes((boolean)TRUE);
	    break;
	case NoUndoEditorCommand::AutoChopSelected:
	    editor->autoChopSelectedNodes();
	    break;
	case NoUndoEditorCommand::AutoFuseSelected:
	    editor->autoFuseSelectedNodes();
	    break;
	case NoUndoEditorCommand::DeletePage:
	    editor->deletePage();
	    break;
	case NoUndoEditorCommand::ConfigurePage:
	    editor->configurePage();
	    break;
	case NoUndoEditorCommand::MoveSelected:
	    editor->postMoveSelectedDialog();
	    break;
#endif
	case NoUndoEditorCommand::CutNode:
	    editor->cutSelectedNodes();
	    break;
	case NoUndoEditorCommand::CopyNode:
	    editor->copySelectedNodes();
	    break;
	case NoUndoEditorCommand::PasteNode:
	    editor->pasteCopiedNodes();
	    break;
	case NoUndoEditorCommand::HitDetection:
	    editor->toggleHitDetection();
	    break;
	case NoUndoEditorCommand::ShowExecutedNodes:
	    editor->showExecutedNodes();
	    break;
	case NoUndoEditorCommand::JavifyNetwork:
	    editor->javifyNetwork();
	    break;
	case NoUndoEditorCommand::UnjavifyNetwork:
	    editor->unjavifyNetwork();
	    break;
	case NoUndoEditorCommand::ReflowGraph:
	    editor->reflowEntireGraph();
	    break;
	case NoUndoEditorCommand::Undo:
	    editor->undo();
	    break;
	default:
	    ASSERT(0);
    }

    if (this->undoType & NoUndoEditorCommand::AffectsUndo) {
	if ((this->undoType & NoUndoEditorCommand::CanBeUndone) == 0) {
	    editor->clearUndoList();
	}
    }

    return ret;
}


