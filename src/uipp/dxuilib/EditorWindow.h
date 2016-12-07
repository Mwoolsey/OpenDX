/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#ifndef _EditorWindow_h
#define _EditorWindow_h

#include "List.h"
#include "Command.h"
#include "DXWindow.h"
#include "ControlPanel.h"
#include "Cacheability.h"
#include "InsertNetworkDialog.h"
#include "Stack.h"

typedef long NodeStatusChange;

//
// Class name definition:
//
#define ClassEditorWindow	"EditorWindow"

//
// XtCallbackProc (*CB), XtEventHandler (*EH) and XtActionProc (*AP)
// DialogCallback (*DCB), XtInputCallbackProc (*ICP), XtWorkProc (*WP)
// functions for this and derived classes
//
extern "C" void EditorWindow_EditMenuMapCB(Widget, XtPointer, XtPointer);
extern "C" void EditorWindow_WindowMenuMapCB(Widget, XtPointer, XtPointer);
extern "C" void EditorWindow_FileMenuMapCB(Widget, XtPointer, XtPointer);
extern "C" void EditorWindow_OptionsMenuMapCB(Widget, XtPointer, XtPointer);
extern "C" void EditorWindow_SetDecoratorStyleCB(Widget, XtPointer, XtPointer);
extern "C" boolean EditorWindow_ConvertSelectionCB(Widget, Atom *, Atom *, Atom *,
	XtPointer *, unsigned long *, int *);
extern "C" void EditorWindow_LoseSelectionCB(Widget, Atom *);
extern "C" void EditorWindow_SelectionDoneCB(Widget, Atom *, Atom *);
extern "C" void EditorWindow_SelectionReadyCB (Widget , XtPointer , Atom *,
	Atom *, XtPointer , unsigned long *, int *);


//
// Referenced classes:
//
class Network;
class CommandScope;
class CommandInterface;
class ControlPanel;
class NodeDefinition;
class ToolSelector;
class EditorWorkSpace;
class WorkSpaceComponent;
class VPERoot;
class Ark;
class Node;
class StandIn;
class FindToolDialog;
class GridDialog;
class CreateMacroDialog;
class InsertNetworInsertNetworkkDialog;
class List;
class NoUndoEditorCommand; 
class DeleteNodeCommand; 
class PanelAccessManager;
class PanelGroupManager;
class ControlPanelGroupDialog;
class ProcessGroupCreateDialog;
class ProcessGroupAssignDialog;
class DXApplication;
class DeferrableAction;
class CascadeMenu;
class Dialog;
class Decorator;
class DecoratorStyle;
class GetSetConversionDialog;
class TransmitterNode;
class GraphLayout;
class VPEAnnotator;
class XHandler;
class UndoableAction;

#if WORKSPACE_PAGES
class GroupManager;
class GroupRecord;
class PageGroupManager;
class AnnotationGroupManager;
class Dictionary;
class PageSelector;
#endif

//
// EditorWindow class definition:
//				
class EditorWindow : public DXWindow
{
  friend class DXApplication;	// For the constructor 
  friend class NoUndoEditorCommand; 
  friend class DeleteNodeCommand; 
  friend class UndoDeletion; // for createNetFileFromSelection
  friend class PageSelector; // for setCommandActivation() in PageSelector::selectPage()
  friend ControlPanel::~ControlPanel();

  private:
    //
    // Private class data:
    //
    static boolean ClassInitialized;
    static String DefaultResources[];
    static String SequenceNet[];
    friend void EditorWindow_FileMenuMapCB(Widget, XtPointer, XtPointer);
    friend void EditorWindow_OptionsMenuMapCB(Widget, XtPointer, XtPointer);
    friend void EditorWindow_WindowMenuMapCB(Widget, XtPointer, XtPointer);
    friend void EditorWindow_EditMenuMapCB(Widget, XtPointer, XtPointer);
    friend void EditorWindow_SetDecoratorStyleCB(Widget, XtPointer, XtPointer);
    friend boolean EditorWindow_ConvertSelectionCB(Widget, Atom *, Atom *, Atom *,
	XtPointer *, unsigned long *, int *);
    friend void EditorWindow_LoseSelectionCB(Widget, Atom *);
    friend void EditorWindow_SelectionDoneCB(Widget, Atom *, Atom *);
    friend void EditorWindow_SelectionReadyCB (Widget , XtPointer , Atom *,
	Atom *, XtPointer , unsigned long *, int *);
    static boolean KeyHandler(XEvent *event, void *clientData);

    static void SetOwner(void*);
    static void DeleteSelections(void*);
    static void Select(void*);

    boolean	   initialNetwork;
    FindToolDialog *findToolDialog;
    Dialog 	   *printProgramDialog;
    Dialog 	   *saveAsCCodeDialog;
    GridDialog     *gridDialog;
    ProcessGroupCreateDialog *processGroupCreateDialog;
    ProcessGroupAssignDialog *processGroupAssignDialog;
    ControlPanelGroupDialog  *panelGroupDialog;
    CreateMacroDialog	     *createMacroDialog;
    InsertNetworkDialog	     *insertNetworkDialog;
    Network	   *pendingPaste;
    char	   *copiedNet;
    char 	   *copiedCfg;

    void postProcessGroupCreateDialog();
    void postPanelGroupDialog();
    void postGridDialog();
    void postFindToolDialog();
    void postPrintProgramDialog();
    void postSaveAsCCodeDialog();
    void postCreateMacroDialog();
    void postInsertNetworkDialog();
    void selectNodes(int how);

    //
    // Used to defer the calls to setCommandActivation() when used with
    // this->deferrableCommandActivation.
    // We allow for the ability to defer the activation when 
    // it is known that an operation will cause an activation.  For example
    // when many selections are done, it is sufficient to to a single 
    // activation at the end of the operation instead of for each selection.
    //
    //
    static void	SetCommandActivation(void *editor, void *requestData);

    //
    // Origin of the workspace window, used for error node finding
    // and for 'restore' in the find tool dialog.
    //
    int    Ox, Oy;
    char*  find_restore_page;

    void   openSelectedINodes();

    //
    // Virtual function called at the beginning/end of Command::execute 
    // We must turn off tool selection and make sure the cursor is
    // back to the normal cursor (i.e. not the Tool placement cursor).
    //
    virtual void beginCommandExecuting();


    //
    // This is a helper method for this->areSelectedNodesMacrofiable().
    // It recursively descends the connections from the given node to determine
    // if it connected to a selected node.  If ignoreDirectConnect is TRUE,
    // then we do not care about selected nodes that are directly connected
    // to selected nodes.
    // Returns TRUE if there is a downstream selected node, otherwise FALSE.
    //
    boolean isSelectedNodeDownstream(Node *srcNode,boolean ignoreDirectConnect);

    //
    // Just call Network's method to optimize output cacheability.
    //
    void optimizeNodeOutputCacheability();

#ifndef FORGET_GETSET
    //
    // Convert nets to use GetLocal/Global and SetLocal/Global
    // instead of Get and Set (nodes).
    //
           void postGetSetConversionDialog();
    static void ConvertToLocal();
    static void ConvertToGlobal(boolean global = TRUE);

    void convertToGlobal(boolean global);

    static Command*  SelectedToGlobalCmd;
    static Command*  SelectedToLocalCmd;

    CascadeMenu		*programVerifyCascade;	

    Command		*toGlobalCmd;
    Command		*toLocalCmd;
    Command		*postGetSetCmd;

    static GetSetConversionDialog* GetSetDialog;
#endif

    //
    // This is the last TransmitterNode that was seen to go through
    // handleNodeStatusChange().  We use this to allow the ReceiverNode
    // to hook itself to if non-NULL.
    //
    TransmitterNode *lastSelectedTransmitter;

    //
    // Record each node as it's highlighted during and execution.  Later on we
    // can change the label colors to show what executed.  The list should be
    // cleared whenever the net gets marked dirty.
    // set transmitters_added to TRUE after examining executed_nodes for connections
    // to transmitters/receivers.  It would be possible to add transmitters/recievers
    // as the executed_nodes list is built up, however it saves time to do it only
    // if the user requests the operation, so transmitters_added tells us if we've
    // done it already or not.
    //
    // Record the executing node because if it's in a page which hasn't been populated
    // yet, it won't turn green.  If you populate the page after its module started
    // executing, then need some way to turn the standin green.  This is provided
    // as a member field and not static because you could (I think) have 2 standins
    // green at the same time - if 1 were a MacroNode and the other were in the the
    // vpe for that macro.
    //
    List* executed_nodes;
    List* errored_standins;
    Node* executing_node;
    boolean transmitters_added;
    void resetExecutionList();
    void resetErrorList(boolean reset_all=TRUE);
    void resetColorList() { this->resetExecutionList(); this->resetErrorList(); }

    //
    // Automatic graph layout
    //
    GraphLayout* layout_controller;

    //
    // Record moved StandIns and decorators so that movement
    // can be undone. ...on behalf of the new graph layout operation.
    //
    Stack undo_list;
    boolean moving_many_standins;
    boolean performing_undo;
    boolean creating_new_network;
    void clearUndoList();

  protected:
    //
    // State information:
    //
    Network*			network;
    ControlPanel		*currentPanel;
    boolean			panelVisible;
    boolean			hit_detection;

    //
    // Used to defer the calls to setCommandActivation(). 
    //
    DeferrableAction	*deferrableCommandActivation;

    //
    // Editor commands:
    //
    Command*		undoCmd;
    Command*		toolPanelCmd;
    Command*		hitDetectionCmd;
    Command*		showExecutedCmd;
    Command*		newControlPanelCmd;
    Command*		openControlPanelCmd;
    Command*            valuesCmd;
    Command*            addInputTabCmd;
    Command*            removeInputTabCmd;
    Command*            addOutputTabCmd;
    Command*            removeOutputTabCmd;
    Command*            hideAllTabsCmd;
    Command*            revealAllTabsCmd;
    Command*            deleteNodeCmd;
    Command*            cutNodeCmd;
    Command*            copyNodeCmd;
    Command*            pasteNodeCmd;
    Command*            selectAllNodeCmd;
    Command*            selectConnectedNodeCmd;
    Command*            selectUnconnectedNodeCmd;
    Command*            selectUpwardNodeCmd;
    Command*            selectDownwardNodeCmd;
    Command*            deselectAllNodeCmd;
    Command*		selectUnselectedNodeCmd;
    Command*		editMacroNameCmd;
    Command*		editCommentCmd;
    Command*            findToolCmd;
    Command*            insertNetCmd;
    Command*            addAnnotationCmd;
    Command*            macroifyCmd;
#if WORKSPACE_PAGES
    Command*            pagifyCmd;
    Command*            pagifySelectedCmd;
    Command*            moveSelectedCmd;
    Command*            autoChopSelectedCmd;
    Command*            autoFuseSelectedCmd;
    Command*            deletePageCmd;
    Command*            configurePageCmd;
#endif
    Command*		javifyNetCmd;
    Command*		unjavifyNetCmd;
    Command*		reflowGraphCmd;
    Command*            gridCmd;
    Command*		setPanelGroupCmd;
    Command*		setPanelAccessCmd;
    Command*		closeCmd;
    Command*		printProgramCmd;
    Command*		saveAsCCodeCmd;
    Command*		createProcessGroupCmd;
    Command*		openImageCmd;
    Command*		openMacroCmd;
    Command*		openColormapCmd;
    Command*		cacheAllOutputsCmd;
    Command*		cacheLastOutputsCmd;
    Command*		cacheNoOutputsCmd;
    Command*		showCacheAllOutputsCmd;
    Command*		showCacheLastOutputsCmd;
    Command*		showCacheNoOutputsCmd;
    Command*		optimizeCacheabilityCmd;

    //
    // Editor components:
    //
    // 
    ToolSelector	*toolSelector;	
#if WORKSPACE_PAGES
    PageSelector	*pageSelector;
#endif
    VPERoot		*workSpace;	
    Widget		scrolledWindow;

    //
    // Menus & pulldowns:
    //
    Widget		fileMenu;
    Widget		editMenu;
    Widget		windowsMenu;
    Widget		optionsMenu;

    Widget		fileMenuPulldown;
    Widget		editMenuPulldown;
    Widget		windowsMenuPulldown;
    Widget		optionsMenuPulldown;

    //
    // File menu options:
    //
    CommandInterface*	newOption;
    CommandInterface*	openOption;
    CommandInterface*	loadMacroOption;
    CommandInterface*	loadMDFOption;
    CommandInterface*	saveOption;
    CommandInterface*	saveAsOption;
    CascadeMenu*	settingsCascade;
    CommandInterface*	saveCfgOption;
    CommandInterface*	openCfgOption;
    CommandInterface*	printProgramOption;
    CommandInterface*	saveAsCCodeOption;
    CommandInterface*	quitOption;
    CommandInterface*	closeOption;

    //
    // Edit menu options:
    //
    CommandInterface*	valuesOption;
    CommandInterface*	findToolOption;
    CascadeMenu		*editTabsCascade;	
    CommandInterface*	addInputTabOption;
    CommandInterface*	removeInputTabOption;
    CascadeMenu		*editSelectCascade;	
    CascadeMenu		*outputCacheabilityCascade;	
    CascadeMenu		*editOutputCacheabilityCascade;	
#if WORKSPACE_PAGES
    CascadeMenu		*pageCascade;	
#endif
    CascadeMenu		*javaCascade;	
    CommandInterface*	deleteOption;
    CommandInterface*	cutOption;
    CommandInterface*	copyOption;
    CommandInterface*	pasteOption;
    CommandInterface*	macroNameOption;
    CommandInterface*	reflowGraphOption;
    CommandInterface*	undoOption;
    CommandInterface*	insertNetworkOption;
    CommandInterface*	addAnnotationOption;
    CommandInterface*	createMacroOption;
    CommandInterface*	commentOption;
    CommandInterface*	createProcessGroupOption;
    CommandInterface*	assignProcessGroupOption;

    //
    // Windows menu options:
    //
    CommandInterface*	newControlPanelOption;
    CommandInterface*	openControlPanelOption;
    CommandInterface*	openAllControlPanelsOption;
    CommandInterface*	openControlPanelByNameOption;
    CascadeMenu		*openControlPanelByNameMenu;
    CommandInterface*	openMacroOption;
    CommandInterface*	openImageOption;
    CommandInterface*	openColormapEditorOption;
    CommandInterface*   messageWindowOption;

    //
    // Options menu options:
    //
    CommandInterface*	toolPalettesOption;
    CommandInterface*	hitDetectionOption;
    CommandInterface*	gridOption;
    CommandInterface*	panelGroupOption;
    CommandInterface*	panelAccessOption;
    CommandInterface*	showExecutedOption;

    //
    // Help menu options:
    //
    CommandInterface*	onVisualProgramOption;

#if 00
    void newNetwork();
#endif
    void drawArks();


    //
    // Creates the editor window workarea (as required by MainWindow class).
    //
    virtual Widget createWorkArea(Widget parent);

    //
    // Creates the editor window menus (as required by DXWindow class).
    //
    virtual void createMenus(Widget parent);

    //
    // Creation routine for each of the editor menus:
    //   Intended to be used in derived class createMenus(), if needed,
    //   to better control the menu creation, or to be overriden in
    //   the derived classes.
    //
    virtual void createFileMenu(Widget parent);
    virtual void createEditMenu(Widget parent);
    virtual void createWindowsMenu(Widget parent);
    virtual void createOptionsMenu(Widget parent);
    virtual void createHelpMenu(Widget parent);

    
    //
    // De/Activate any commands that have to do with editing.
    // Should be called when a node is added or deleted or there is a selection 
    //
    // Called through deferrableCommandActivation via SetCommandActivation(),
    // but this can also be called directly.  It is used to set the activation
    // state of all commands owned by the Editor.
    // FIXME: should this be virtual?
    //
    void setCommandActivation();

    //
    // Perform various command functions.
    //
    boolean selectDownwardNodes();
    boolean selectUpwardNodes();
    boolean selectConnectedNodes();
    boolean selectUnconnectedNodes();
    boolean setSelectedNodeTabVisibility(boolean v);
    boolean selectConnection(int direction, boolean connected);
    //
    // Implements the 'Add/Remove Input/Output Tab' commands.
    // If 'adding' is TRUE, we ADD either inputs or outputs, otherwise
    // REMOVE.
    // If 'input' is true, we operate on the input parameters, otherwise
    // the outputs.
    //
    boolean editSelectedNodeTabs(boolean adding, boolean input);

    //
    // Methods that are used by the DeleteCommand to help implement
    // undelete.
    //
    void	removeSelectedNodes();
#ifdef OBSOLETE
    void 	restoreRemovedNodes(List *toRestore);
#endif
    void 	deleteNodes(List *toDelete);

    virtual void serverDisconnected();

    //
    // Edit/{Cut,Copy,Paste} operations
    //
    boolean	cutSelectedNodes();
    boolean	copySelectedNodes(boolean delete_property = FALSE);
    boolean	pasteCopiedNodes();
    char*       createNetFileFromSelection(int& net_len, char** cfg_out, int& cfg_len);
    boolean     setPendingPaste (const char* net_file_name,
	    const char* cfg_file_name, boolean ignoreUndefinedModules=FALSE);

    //
    // When the user clicks the Edit/Add Decorator option, we create a new
    // decorator, stick it in a list.  Then a callback in EditorWorkSpace
    // will let us know when the user clicks in the workspace.  The
    // decorator(s) is the list will be added to the workspace at that location.
    // Any decorator in this list is not yet managed.
    // This list increases is size in an extern C callback, not thru NoUndoEditorCommand
    // because it is modal in nature.  The coding is copied from ControlPanel which
    // does the same thing when adding either interactors or decorators.
    //
    List *addingDecorators;

    //
    // Constructor:
    //  THIS CONSTRUCTOR SHOULD NEVER BE CALLED FROM ANYWHERE BUT
    //  DXApplication::newNetworkEditor OR A SUBCLASS OF NETWORK.  ANY
    //  SUBCLASSES CONSTRUCTOR SHOULD ONLY BE CALLED FROM A VIRTUAL
    //  REPLACEMENT OF DXApplication::newNetworkEditor.
    //
    EditorWindow(boolean  isAnchor, Network* network);

    //
    // Install the default resources for this class and then call the
    // same super class method to get the default resources from the
    // super classes.
    //
    virtual void installDefaultResources(Widget baseWidget);

#if WORKSPACE_PAGES
    EditorWorkSpace *getPage(const char *name);
    Dictionary      *pageDictionary;
    GroupRecord	    *getGroupOfWorkSpace(EditorWorkSpace*);
    int		     getPageMemberCount(WorkSpace*);
    void	     destroyStandIns(List*);
#endif

    //
    // Do what ever is necessary just before changing the current network.
    // To make things run faster we turn off line routing.  This should
    // be followed by a call to endNetworkChange(), which will re-enable
    // line routing.
    //
    void beginNetworkChange();
    void endNetworkChange();

    //
    // Put up a little dialog with page configuration options
    //
    boolean configurePage();

    //
    // Prepare to place a new decorator (vpe annotation)
    //
    boolean placeDecorator();

    //
    // In addition to setting command activation based on undo_list size,
    // also set the text of the undo button label.
    //
    void setUndoActivation();

    // work around for a motif bug - hitting pg up,down in the vpe crashes dxui
    XHandler* pgKeyHandler;

    boolean keyHandler(XEvent* event);

  public:

    //
    // Flags for highlighting nodes.
    //
    enum {
	   ERRORHIGHLIGHT,
	   EXECUTEHIGHLIGHT,
	   REMOVEHIGHLIGHT 
	 };

    //
    // Destructor:
    //
    ~EditorWindow();

    //
    // this->workSpace subclasses off EditorWorkSpace but is no longer an EditorWorkSpace
    //
    EditorWorkSpace *getWorkSpace() { return (EditorWorkSpace*)this->workSpace; }

    //
    // look through the network list and make a list of all selected  nodes.
    // If no selected interactors were found then NULL is returned.
    // The return list must be freed by the caller.
    //
    List *makeSelectedNodeList(const char *classname = NULL);
    List *makeSelectedDecoratorList();

    //
    // Look through the node list and make a list of existing module names.
    //
    List *makeModuleList();

    //
    // EditorWorkSpace needs access to the object to support drag-n-drop
    //
    ToolSelector *getToolSelector() { return this->toolSelector; };

    //
    // Properly deal with a nodes status change. 
    // Currently, this only include selection and deselection.
    //
    void handleNodeStatusChange(Node *n, NodeStatusChange status);
    void handleDecoratorStatusChange() { this->setCommandActivation(); }

    //
    // Determine the number of selected nodes
    //
    int getNodeSelectionCount(const char *classname = NULL);

    //
    // Highlighting node.
    //
    void highlightNodes(int flag, List *l = NULL);
    void highlightNode(Node *n, int flag);
    void highlightNode(const char* name, int instance, int flag);
    boolean selectNode(Node *node, boolean select, boolean moveto = TRUE);
    boolean selectNode(char* name, int instance, boolean select);
    boolean selectDecorator (VPEAnnotator* dec, boolean select, boolean moveto=TRUE);
    void selectUnselectedNodes();
    void deselectAllNodes(); 
    void selectAllNodes();

    //
    // Process group fuctions.
    //
#if WORKSPACE_PAGES
    void setGroup(GroupRecord *grec, Symbol groupID);
    void resetGroup(const char* name, Symbol groupID);
    void selectGroup(const char* name, Symbol groupID);
    void clearGroup(const char* name, Symbol groupID);
    boolean changeGroup (const char* old_group, const char* new_name, Symbol groupID);
#else
    void setProcessGroup(const char* name);
    void resetProcessGroup(const char* name);
    void selectProcessGroup(const char* name);
    void clearProcessGroup(const char* name);
#endif

    //
    // Handle client messages (not from the exec).
    //
    virtual void notify
	(const Symbol message, const void *msgdata=NULL, const char *msg=NULL);

    //
    // Sets the tool panel to be visible or invisible
    // based on the current state.
    //
    void toggleToolPanel();
    void doSelectedNodesDefaultAction();

    //
    // Change the label color of standins whose nodes ran in the most recent
    // execution
    //
    boolean showExecutedNodes();

    //
    // look through the network list for selected nodes
    // and open the configuration dialog associated with
    // the node.
    //
    void openSelectedNodesCDB();

    //
    // Implements the 'Values' command.
    //
    void showValues();

    // EditorWindow cursor functions
    void setCursor(int cursorType);
    void resetCursor();

    //
    // add the node selected in the toolList to the vpe at x,y
    // Added stitch 11/8/02.  When it's set to TRUE, we're going to handle
    // conflicts of node type/inst # by replacing the copy in the merging
    // network with the copy in the real network.  This is useful in 
    // implemented Undo of Cut/Delete.  For this we need to copy not only
    // selected nodes, but also nodes they're wired to so that we can
    // restore the wires that were broken as a result of the cut.
    //
    void addCurrentNode(int x, int y, EditorWorkSpace *where, boolean stitch=FALSE);
    void addNode (NodeDefinition *, int x, int y, EditorWorkSpace *where);

    //
    // Move the workspace window if the selected error node is not shown 
    //
    void moveWorkspaceWindow(UIComponent*);

    //
    // Move the workSpace inside the scrolled window so that the given x,y
    // position is at the upper left corner of the scrolled window unless
    // centered is TRUE in which case x,y is the center of the scrolled
    // window.  Currently only implmented for x=y=0 && centered == FALSE.
    //
    void moveWorkspaceWindow(int x, int y, boolean centered = TRUE);

    //
    // Supply scrollbar positions so that they can be recorded/reset when
    // switching pages.
    //
    void getWorkspaceWindowPos (int *x, int *y);


    //
    // Reset the origin flag
    //
    void resetOrigin()
    {
	this->Ox = -1;
    }

    //
    // Data access routines:
    //
    Network* getNetwork()
    {
	return this->network;
    }

    //
    // Return a pointer to the editor's notion of the 'current' ControlPanel.
    // The return panel may or may not be managed.
    // 
    virtual ControlPanel *getCurrentControlPanel(void);

   
    Widget  getScrolledWindow()
    {
	return this->scrolledWindow;
    }

    boolean isPanelVisible()
    {
	return this->panelVisible;
    }

    void newNode(Node *n, EditorWorkSpace *where);
    void newDecorator(Decorator* dec, EditorWorkSpace* where=NUL(EditorWorkSpace*));

    //
    // notify the a standIn that an arc has been added/deleted to/from the node
    //
    void notifyArk(Ark *a, boolean added=TRUE);

    FindToolDialog* getFindDialog()
    {
	return (findToolDialog);
    }

    virtual void manage();

    void installWorkSpaceInfo(WorkSpaceInfo *info);

    Command *getDeleteNodeCmd(){return this->deleteNodeCmd;};
#ifndef FORGET_GETSET
    static Command* GetGlobalCmd() 
	{ return EditorWindow::SelectedToGlobalCmd; };
    static Command* GetLocalCmd() 
	{ return EditorWindow::SelectedToLocalCmd; };
#endif


    //
    // Do what ever is necessary just before and after reading a new network
    // These should be called in pairs. 
    //
    void prepareForNewNetwork();
    void completeNewNetwork();

    //
    // Notify control panel related dialogs of the change of panel list.
    //
    void notifyCPChange(boolean newList = TRUE);

    //
    // Adjust the name of the editor window based on the current network
    // name.
    //
    void resetWindowTitle();

    void    openSelectedMacros();
    void    openSelectedImageWindows();
    boolean anySelectedNodes(const char *classname);
    boolean anySelectedColormaps();
    boolean anySelectedMacros();
    boolean anySelectedDisplayNodes();

    //
    // Turn selected nodes into a macro
    //
    virtual boolean macroifySelectedNodes(const char *name,
					  const char *cat,
					  const char *desc,
					  const char *fileName);

    //
    // Turn selected nodes into a vpe page
    //
#if WORKSPACE_PAGES
    virtual boolean pagifySelectedNodes(boolean require_selected_nodes=FALSE);
    virtual boolean pagifySelectedNodes(EditorWorkSpace* );
    virtual boolean postMoveSelectedDialog();
    virtual boolean autoChopSelectedNodes();
    virtual boolean autoFuseSelectedNodes();
	    Command *getMoveSelectedCmd() { return this->moveSelectedCmd; }
	    boolean pagifyNetNodes
		(Network* tmpnet, EditorWorkSpace*, boolean try_current_page = FALSE );
    virtual boolean deletePage(const char* page_name=NUL(char*));
    void moveStandIns 
	(EditorWorkSpace *page, List *selectedNodes, int xoff, int yoff, List *dl);
    void moveDecorators 
	(EditorWorkSpace *page, List *seldec, int xoff, int yoff, List *nl);
#endif
    EditorWorkSpace *getNodesBBox 
	(int *minx, int *miny, int *maxx, int *maxy, List *l, List *dl);

    //
    // Change the decorator style of the selected decorator(s)
    //
    void setDecoratorStyle (List *decors, DecoratorStyle *newds);

#if 11 
    void newNetwork();
#endif

    //
    // Print the visual program as a PostScript file using geometry and not
    // bitmaps.  We set up the page so that it maps to the current workspace 
    // and then as the StandIns and ArkStandIns to print themselves.  
    // If the scale allows and the label_parameters arg is set, then 
    // label the parameters and display the values.
    // We return FALSE and issue and error message if an error occurs.
    //
    boolean printVisualProgramAsPS(const char *filename,
				    float x_pagesize, float y_pagesize,
				    boolean label_parameters);
    boolean printVisualProgramAsPS(FILE*, const char* filename, float x_pagesize, 
	float y_pagesize, boolean label_parameters, EditorWorkSpace*, int, int);

    //
    // Set the cacheability of all outputs of the selected tools to one of
    // OutputFullyCached, OutputNotCached or OutputCacheOnce.
    // A warning is issued on those nodes that do not have writeable 
    // cacheability.
    //
    void setSelectedNodesOutputCacheability(Cacheability c);

    //
    // Show (select) the nodes with given cacheability
    // OutputFullyCached, OutputNotCached or OutputCacheOnce.
    //
    void selectNodesWithOutputCacheability(Cacheability c);

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
    boolean areSelectedNodesMacrofiable();

#if WORKSPACE_PAGES
    //
    // Determine if the currently selected set of nodes is pagifiable. 
    // This means that no selected node has an arc to an unselected node.
    // The only way data flow into or out of a page is thru Input, Output,
    // Transmitter, Receiver nodes.
    // The report flag tells if we should report failure to the user via
    // an ErrorMessage.
    boolean areSelectedNodesPagifiable(boolean report=FALSE);

    void    populatePage(EditorWorkSpace* ews);
    PageSelector* getPageSelector() { return this->pageSelector; }

    //
    // Help for Network::mergeNetworks().  It's wrong to use {begin,end}NetworkChnage
    // because that would operate on all pages.  They operate only on the current page.
    // Aside: the current page better not change in between calls to begin, end.
    // It would be just as dangerous to record the page upon which the operation was
    // initiated because the page could be deleted.
    //
    void beginPageChange();
    void endPageChange();
    void	removeNodes(List* toDelete);
#endif

    TransmitterNode *getMostRecentTransmitterNode();

#if EXPOSE_SET_SELECTION
    //
    // On behalf of VPEAnnotators which have the ability to snarf whole nets and
    // set them back into the selection via XtOwnSelection...
    //
    void setCopiedNet(const char*);
#endif

    //
    // Change a resource in the Workspace widget that lets the user know
    // if things will overlap - during the course of moving
    //
    boolean toggleHitDetection();

    //
    // Create a special java page and fill it up.
    //
    boolean javifyNetwork();
    boolean unjavifyNetwork();

    //
    // Use GraphLayout to modify the flow of the graph.
    //
    boolean reflowEntireGraph();

    //
    // register undo information
    //
    void  saveLocationForUndo (UIComponent* uic, boolean mouse=FALSE, boolean same_event=FALSE);
    void  saveAllLocationsForUndo (UndoableAction* gridding);
    void  beginMultipleCanvasMovements();
    void  endMultipleCanvasMovements() { this->moving_many_standins = FALSE; }

    //
    // a mechanism for Node to notify us that his definition has changed.  We
    // throw out the undo list because this will probably involve changes that
    // we can't work around i.e. types of io tabs will be different and attempts
    // to reconnect tabs will fail.
    //
    void notifyDefinitionChange(Node* n) { this->clearUndoList(); }

    //
    // perform undo operation
    //
    boolean undo();

    //
    // Receive a notification that the net has been saved and marked clean
    //
    void notifySaved() { this->clearUndoList(); }

    //
    // FindToolDialog requires support in order to move the canvas back
    // the original location before the first find operation.  This used
    // to happen via a side effect in moveWorkspaceWindow() which was
    // broken with the addition of vpe pages.
    //
    void checkPointForFindDialog(FindToolDialog*);

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassEditorWindow;
    }
};


#endif // _EditorWindow_h
