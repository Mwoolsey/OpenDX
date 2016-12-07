/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _NoUndoEditorCommand_h
#define _NoUndoEditorCommand_h


#include "NoUndoCommand.h"

typedef long EditorCommandType;
typedef long EditorUndoType;

//
// Class name definition:
//
#define ClassNoUndoEditorCommand	"NoUndoEditorCommand"

class   EditorWindow;

//
// NoUndoEditorCommand class definition:
//				
class NoUndoEditorCommand : public NoUndoCommand
{
    
  protected:
    //
    // Protected member data:
    //
    EditorWindow 	*editor;
    EditorCommandType 	commandType;
    EditorUndoType	undoType;
 
    virtual boolean doIt(CommandInterface *ci);

  public:
    //
    // Constructor:
    //
    NoUndoEditorCommand(const char*   name,
                   CommandScope  *scope,
                   boolean       active,
		   EditorWindow  *editor,
		   EditorCommandType comType,
		   EditorUndoType undoType=AffectsUndo);

    //
    // Destructor:
    //
    ~NoUndoEditorCommand(){}

    //
    // Each command has a potential impact on EditorWindow's undo stack.
    // There are really 2 bits in this mask with 3 possible values.
    // Inside the doit(), we'll call on EditorWindow to (maybe) rollback
    // its undo stack.
    //
    enum {
	Ignore = 0,		// example: print
	CanBeUndone = 1,	// example: move a standIn
	AffectsUndo = 2,	// example: delete a node or remove a tab
    };

    // 
    // These are the various operations that the NoUndoEditorCommand can 
    // implement on behalf of an editor .
    // 
    enum {
	ShowConfiguration,		// Open configuration dialog box... 
	SelectAll,			// Select all nodes... 
	DeselectAll,			// Deselect all nodes... 
	AddInputTab,    		// Add repeatable tabs to a node
	RemoveInputTab,    		// Remove repeatable tabs from a node
	AddOutputTab,    		// Add repeatable tabs to a node
	RemoveOutputTab,    		// Remove repeatable tabs from a node
	SelectDownward,			// Select all down stream nodes 
	SelectUpward,			// Select all up stream nodes 
	SelectConnected,		// Select all connected nodes 
	SelectUnconnected,		// Select all unconnected nodes 
	SelectUnselected,		// Select all unselected nodes 
	NewControlPanel,		// Open a new control panel.
	EditComment,			// Edit the network comment
	EditMacroName,			// Edit the network name 
	Macroify,			// Make the selected nodes into a macro
        OpenFindTool,			// Open the find tool dialog
        HideAllTabs,			// Hide hideable tabs
        RevealAllTabs,			// Reveal hidden tabs
        OpenGrid,			// Open the grid dialog
	OpenControlPanel,   		// Open selected interactor nodes.
	SetPanelGroup,   		// Open the dialog to set panel group.
	SetCPAccess,   			// Open the dialog to set panel Access.
	CreateProcessGroup,		// Open Process Group creating dialog.
    	OpenSelectedColormaps,		// Open the Selected Colormaps
    	OpenSelectedMacros,		// Open the Selected Macros
    	OpenSelectedImageWindows,	// Open the Selected Images
	AssignProcessGroup,		// Open Process Group assignment dialog.
	PrintProgram,			// Print the program in postscript 
        SetOutputsNotCached,	// All outputs of selected nodes to uncached
        SetOutputsFullyCached,	// All outputs of selected nodes to cached 
        SetOutputsCacheOnce,  	// All outputs of selected nodes to cache last 
        ShowOutputsNotCached,	// Show nodes that are not cached. 
        ShowOutputsFullyCached,	// Show nodes that are cached. 
        ShowOutputsCacheOnce,  	// Show nodes that are cached once. 
        OptimizeOutputCacheability, // Set cachability optimally. 
        InsertNetwork,  	// merge a new network into vpe
        ShowExecutedNodes,  	// Change the label color of executed standins
#ifndef FORGET_GETSET
	ToGlobal,		// Convert Get/Set modules to Local or Global
	ToLocal,
	SelectedToGlobal,
	SelectedToLocal,
	PostGetSet,
#endif
        CutNode,	  	// Edit/Cut
        CopyNode,	  	// Edit/Copy
        PasteNode,	  	// Edit/Paste
        AddAnnotation,  	// Edit/Add Annotation
#if WORKSPACE_PAGES
        Pagify,		  	// Create a new vpe page
        PagifySelected,	  	// Create a new vpe page including selected tools
	AutoChopSelected,	// Separate selected using Transmitters/Receivers
	AutoFuseSelected,	// Replace matching Transmitters/Receivers with arcs
        DeletePage,	  	// Delete a vpe page
	ConfigurePage,		// Change the name,postscript,position
	MoveSelected,		// Move selected tools to a new page
#endif
	JavifyNetwork,
	UnjavifyNetwork,
	HitDetection,		// change a resource in in the Workspace widget
	ReflowGraph,		// Layout the entire graph automatically
	Undo,			// undo movements in the canvas
        SaveAsCCode
    };
    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassNoUndoEditorCommand;
    }
};


#endif // _NoUndoEditorCommand_h
