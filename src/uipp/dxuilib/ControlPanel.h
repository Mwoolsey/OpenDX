/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _ControlPanel_h
#define _ControlPanel_h



#include "DXWindow.h"
#include "WorkSpaceInfo.h"
#include "List.h"
#include "enums.h" // for PrintType

typedef long InteractorStatusChange;
typedef long NodeStatusChange;

// FIXME: these should go away as soon as Network does not use them.
typedef long CPInteractorStatus;
typedef long CPNodeStatus;

//
// Class name definition:
//
#define ClassControlPanel	"ControlPanel"

//
// XtCallbackProc (*CB), XtEventHandler (*EH) and XtActionProc (*AP)
// DialogCallback (*DCB), XtInputCallbackProc (*ICP), XtWorkProc (*WP)
// functions for this and derived classes
//
extern "C" void ControlPanel_OptionsMenuMapCB(Widget, XtPointer, XtPointer);
extern "C" void ControlPanel_PanelMenuMapCB(Widget, XtPointer, XtPointer);
extern "C" void ControlPanel_SetDimensionsCB(Widget, XtPointer, XtPointer);
extern "C" void ControlPanel_SetStyleCB(Widget, XtPointer, XtPointer);
extern "C" void ControlPanel_EditMenuMapCB(Widget, XtPointer, XtPointer);
extern "C" void ControlPanel_DialogCancelCB(Widget, XtPointer, XtPointer);
extern "C" void ControlPanel_DialogHelpCB(Widget, XtPointer, XtPointer);
extern "C" void ControlPanel_PlaceDecoratorCB(Widget, XtPointer, XtPointer);



//
// Referenced classes:
//
class Network;
class CommandInterface;
class InteractorInstance;
class ControlPanelWorkSpace;
class Command;
class InteractorNode;
class Dialog;
class Node;
class NoUndoEditorCommand;	
class SetPanelNameDialog;  	
class SetPanelCommentDialog;  	
class GridDialog;
class SetInteractorNameDialog;
class PanelAccessManager;
class HelpOnPanelDialog;
class CascadeMenu;
class WorkSpaceComponent;
class Decorator;

//
// ControlPanel class definition:
//				
class ControlPanel : public DXWindow
{
    friend class NoUndoEditorCommand;	// For addSelectedInteractorNodes()
    friend class SetPanelNameDialog;  	// For setPanelName();
    friend class SetPanelCommentDialog; // For setPanelComment();
    friend class NoUndoPanelCommand;	// For panelAccessManager

  private:
    //
    // Private class data:
    //
    static boolean ClassInitialized;
    static String  DefaultResources[];
    friend void ControlPanel_EditMenuMapCB(Widget, XtPointer, XtPointer);
    friend void ControlPanel_SetStyleCB(Widget, XtPointer, XtPointer);
    friend void ControlPanel_SetDimensionsCB(Widget, XtPointer, XtPointer);
    friend void ControlPanel_OptionsMenuMapCB(Widget, XtPointer, XtPointer);
    friend void ControlPanel_PanelMenuMapCB(Widget, XtPointer, XtPointer);
    friend void ControlPanel_DialogCancelCB(Widget, XtPointer, XtPointer);
    friend void ControlPanel_DialogHelpCB(Widget, XtPointer, XtPointer);
    friend void ControlPanel_PlaceDecoratorCB(Widget, XtPointer, XtPointer);

    static void ToDevStyle(XtPointer);
    static void Unmanage(XtPointer);
    static void SetOwner(void*);
    static void DeleteSelections(void*);

    void setDimensionsCallback(Widget w, XtPointer clientdata);

    enum StyleEnum {
	NeverSet, Developer, User
    };
    boolean     developerStyle;
    StyleEnum	displayedStyle;
    boolean	allowOverlap;   // save old setting
    int		xpos, ypos;	// Positions saved in .cfg file.
    char	*comment;	// The comment for this control panel.
    List	instanceList;	// List of interactor instances.
    List	*addingNodes;	// List of nodes that are in the process of
				// being placed on the work space.    
    List         styleList;		// Widget list
    List         dimensionalityList;	// Widget list
    List   	 panelNamesList;
    List   	 panelGroupList;
    List	 componentList; // labels and separators
    List	 addingDecoratorList; // decorators not yet managed
    GridDialog   *gridDialog;
    int		instanceNumber;
    SetInteractorNameDialog 	*setLabelDialog;
    PanelAccessManager		*panelAccessManager; 
    Dimension	develModeDiagWidth; // for toggling between user/developer mode
    Dimension   develModeDiagHeight; 
    boolean     develModeGridding;  // true if gridding was on before switching to user
    boolean	hit_detection;

    //
    // called by MainWindow::PopdownCallback().
    //
    virtual void popdownCallback();
 
    //
    // Parsing state:
    //
    // remember what object was most recently parsed so that resource comments
    // which follow can be associated with that object.
    WorkSpaceComponent *lastObjectParsed;

    //
    // Do we allow the user to toggle between developer and dialog style
    //
    boolean allowDeveloperStyleChange();

    char* java_variable;

  protected:
    Network*	network;


    //
    // Control Panel commands:
    //

    Command*		closeCmd;
    Command*		addSelectedInteractorsCmd;
    Command*		showSelectedInteractorsCmd;
    Command*		showSelectedStandInsCmd;
    Command*		deleteSelectedInteractorsCmd;
    Command*		setSelectedInteractorAttributesCmd;
    Command*		setSelectedInteractorLabelCmd;
    Command*		setPanelCommentCmd;
    Command*		setPanelNameCmd;
    Command*		setPanelAccessCmd;
    Command*		setPanelGridCmd;
    Command*		togglePanelStyleCmd;
    Command*		verticalLayoutCmd;
    Command*		horizontalLayoutCmd;
    Command*		helpOnPanelCmd;
    Command*		hitDetectionCmd;

    //
    // Control panel components:
    //
    Widget		scrolledWindow;
    Widget		scrolledWindowFrame;
    WorkSpaceInfo	workSpaceInfo;
    ControlPanelWorkSpace *workSpace;
    
    // collection of pushbuttons
    Widget		commandArea;

    //
    // Menus & pulldowns:
    //
    Widget		fileMenu;
    Widget		editMenu;
    Widget		panelsMenu;
    Widget		optionsMenu;

    Widget		fileMenuPulldown;
    Widget		editMenuPulldown;
    Widget		panelsMenuPulldown;
    Widget		optionsMenuPulldown;

    //
    // File menu options:
    //
    CommandInterface*	closeOption;
    CommandInterface*	closeAllOption;

    //
    // Edit menu options:
    //
    Widget              setStyleButton;
    Widget              stylePulldown;
    Widget              setDimensionalityButton;
    Widget              setLayoutButton;
    Widget              addButton;
    Widget              addPulldown;
    CommandInterface* 	verticalLayoutOption;
    CommandInterface*	horizontalLayoutOption;
    CommandInterface*	setAttributesOption;
    CommandInterface*	setInteractorLabelOption;
    CommandInterface*	deleteOption;
    CommandInterface*	addSelectedInteractorsOption;
    CommandInterface*	showSelectedInteractorsOption;
    CommandInterface*	showSelectedStandInsOption;
    CommandInterface*	commentOption;

    //
    // Panels menu options:
    //
    CommandInterface*	openAllControlPanelsOption;
    CascadeMenu		*openControlPanelByNameMenu;

    //
    // Options menu options:
    //
    CommandInterface*	changeControlPanelNameOption;
    CommandInterface*	setAccessOption;
    CommandInterface*	gridOption;
    CommandInterface*	styleOption;
    CommandInterface*	hitDetectionOption;

    //
    // Help menu options:
    //
    CommandInterface*	onControlPanelOption;
    CommandInterface*	onVisualProgramOption;
    CommandInterface*	userHelpOption;
   
    //
    // Various dialogs. 
    //
    SetPanelNameDialog 	*setNameDialog;
    SetPanelCommentDialog *setCommentDialog;
    HelpOnPanelDialog 	*helpOnPanelDialog;

    //
    // Creates the editor window workarea (as required by MainWindow class).
    //
    virtual Widget createWorkArea(Widget parent);

    //
    // Creates a form,separator, and group of buttons
    //
    virtual Widget createCommandArea (Widget parent);

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
    virtual void createPanelsMenu(Widget parent);
    virtual void createOptionsMenu(Widget parent);
    virtual void createHelpMenu(Widget parent);

    //
    // Parse/Print the 'panel comment in the .cfg file. 
    //
    virtual boolean cfgParsePanelComment(const char *comment, 
				const char *filename, int lineno);
    virtual boolean cfgPrintPanelComment(FILE *f, PrintType dest=PrintFile); 

    // Parse/Print decorator comments in the .cfg file.
    virtual boolean cfgParseDecoratorComment (const char *comment,
				const char *filename, int lineno);
    virtual boolean cfgPrintDecoratorComment(FILE *f, PrintType dest=PrintFile); 

    //
    // Parse/Print the comment comment in the .cfg file. 
    //
    virtual boolean cfgParseCommentComment(const char *comment, 
				const char *filename, int lineno);
    virtual boolean cfgPrintCommentComment(FILE *f, PrintType dest=PrintFile); 

    //
    // Parse/Print a control panel's 'title' comment.
    //
    virtual boolean cfgParseTitleComment(const char *comment,
                                const char *filename, int lineno);
    virtual boolean cfgPrintTitleComment(FILE *f, PrintType dest=PrintFile); 


    //
    // Find THE selected InteractorInstance.
    // Return TRUE if a single interactor is selected 
    // Return FALSE if none or more than one selected interactor were found 
    // Always set *selected to the first selected InteractorInstance that was
    // found.  If FALSE is return *selected may be set to NULL if no selected
    // InteractorInstances were found.
    // 
    boolean findTheSelectedInteractorInstance(InteractorInstance **selected);

    //
    // Same as findTheSelectedInteractorInstance... only for WorkSpaceComponent
    //
    boolean findTheSelectedComponent(WorkSpaceComponent **selected);

    //
    // Determine the number of selected Interactors
    //
    int getInteractorSelectionCount();
    int getComponentSelectionCount();

    //
    // Add all the editor's selected InteractorNodes to the control panel
    //
    void addSelectedInteractorNodes();

    //
    //
    //

    //
    // Create the default style of interactor for the given node and place it
    // at the give x,y position.
    //
    InteractorInstance *createAndPlaceInteractor(InteractorNode *inode, 
							int x, int y);
    Decorator *createAndPlaceDecorator(const char *type, int x, int y);

    //
    // Set the name/title of this panel.
    //
    void setPanelComment(const char *name);
    void setPanelName(const char *name);

    //
    // Virtual function called at the beginning of Command::execute().
    // Before command in the system is executed we disable (the active
    // current) interactor placement.
    //
    virtual void beginCommandExecuting();

    //
    // Install the default resources for this class and then call the
    // same super class method to get the default resources from the
    // super classes.
    //
    virtual void installDefaultResources(Widget baseWidget);

    //
    // Allow subclasses to supply a string for the XmNgeometry resource
    // (i.e. -geam 90x60-0-0) which we'll use instead of createX,createY,
    // createWidth, createHeight when making the new window.  If the string
    // is available then initialize() won't call setGeometry.
    //
    virtual void getGeometryAlternateNames(String*, int*, int);


  public:
    //
    // Constructor:
    // NOTE: This should not be called by anyone but Network::newControlPanel().
    //
    ControlPanel(Network* network);

    //
    // De/Activate any commands that have to do with editing.
    // Should be called when an Interactor is added or deleted or there is a
    // selection status change. Used to be protected but is now public so that
    // ControlPanelWorkSpace can invoke it.
    //
    void setActivationOfEditingCommands();

    //
    // Destructor:
    //
    ~ControlPanel();

    //
    // For cfg file reading
    //
    void    clearInstances();

    //
    // Sets the default resources before calling the superclass
    // initialize() function.
    //
    virtual void initialize();

    //
    //
    //
    Widget	getWorkSpaceWidget();

    ControlPanelWorkSpace* getWorkSpace()
    {
        return this->workSpace;
    }

    void	manage();
    void        unmanage();

    //
    // Add/Remove an instance of an interactor. 
    //
    boolean addInteractorInstance(InteractorInstance *ii);
    boolean removeInteractorInstance(InteractorInstance *ii);

    //
    // Parse the comments in the .cfg file. 
    //
    virtual boolean cfgParseComment(const char *comment, 
				const char *filename, int lineno);

    //
    // Print the information about the control panel in the .cfg file 
    //
    virtual boolean cfgPrintPanel(FILE *f, PrintType dest=PrintFile); 

    //
    // Take the first item on this->selectedNodes list and place
    // it in the ContolPanel data structures and workspace.
    // We get this callback even when the list is empty, so be sure we
    // do the right thing when it is empty.
    //
    virtual void placeSelectedInteractorCallback(Widget w, XtPointer cb);

    //
    //  Highlight each InteractorInstance that is in this control panel
    //  and have their corresponding Node selected.
    //  Called when this->showSelectedInteractorsCmd is used.
    //
    void showSelectedInteractors();
    
    //
    //  Deselect all nodes in the VPE, and then highlight each 
    //  InteractorNode that has a selected InteractorInstance in this 
    //  control panel.
    //  Called when this->showSelectedStandInsCmd is used.
    //
    void showSelectedStandIns();
    
    //
    // Delete all selected InteractorInstances from the control panel. 
    //
    void deleteSelectedInteractors();

    //
    // Open the set attributes dialog box for THE selected interactor. 
    // If there is more than one interactor selected then we pop up an 
    // error message.
    //
    void openSelectedSetAttrDialog();

    //
    // Set the label of THE selected interactor. 
    // If there is more than one interactor selected then we pop up an 
    // error message.
    //
    void setSelectedInteractorLabel();
    void setInteractorLabel(const char *label);

    const char *getInteractorLabel();

    //
    // Edit/get/view the comment for this panel. 
    //
    void editPanelComment();
    const char *getPanelComment() { return this->comment; }
    void postHelpOnPanel();


    //
    // Edit the name of this panel. 
    //
    void editPanelName();

    //
    // Change the work space grid setup 
    //
    void setPanelGrid();

    //
    // User / Developer styles
    //
    void setPanelStyle(boolean developerStyle);
    boolean isDeveloperStyle() { return this->developerStyle; }

    //
    // The decorators are held in this->componentList (no interactors in the list)
    //
    List getComponents () { return this->componentList; }
    void addComponentToList (void *e) { this->componentList.appendElement(e); }

    //
    // This is called by the ControlPanelWorkSpace class to handle
    // double clicks in the workspace.
    //
    virtual void doDefaultWorkSpaceAction();

    //
    // Perform basically the functions of initiateInteractorPlacement(),
    // placeSelectedInteractor, and concluedInteractorPlacement all at once.
    // Use createAndPlaceInteractor for the job.  This happens due to a dnd
    // operation from the vpe to the ControlPanel.
    //
    boolean dndPlacement (int x, int y);
 
    //
    // Initialize for selected interactor node placement.  Should be called
    // when the 'Add Selected Interactors' command is issued.
    // Returns the number of selected interactor nodes that will be allowed 
    // to be placed.
    //
    void initiateInteractorPlacement();

    //
    // Normal termination of interactor placement.
    // There should be no more items in this->selectedNodes.
    // We reset the work space cursor and then deactivate the 
    // 'Add Selected Interactors' pulldown command.
    // 
    void concludeInteractorPlacement();

    //
    // Change the layout of all selected interactors.
    //
    void setVerticalLayout(boolean vertical = TRUE);

    //
    // Do any work that is required when a Node's state has changed.
    // This work may include enable or disabling certain commands.
    //
    void handleNodeStatusChange(Node *node, NodeStatusChange status);

    //
    // Do any work that is required when a Interactors's state has changed.
    // This work may include enable or disabling certain commands.
    //
    void handleInteractorInstanceStatusChange(InteractorInstance *ii, 
					CPInteractorStatus status);

    //
    // G/Set the instance number of this control panel. 
    //
    int getInstanceNumber() { return this->instanceNumber; }

    void setInstanceNumber(int instance);

    Network *getNetwork()
    {
	return this->network;
    }

    //
    // Used in Network::mergeAndDeleteNet
    //
    void switchNetwork(Network *from, Network *to);

    //
    // Get the number of interactor instances / decorators in this control panel. 
    //
    int getInstanceCount()  { return this->instanceList.getSize(); }
    int getDecoratorCount()  { return this->componentList.getSize(); }
    int getComponentCount()  
	{ return (this->getInstanceCount() + this->getDecoratorCount()); }

    const char *getPanelNameString() { return this->getWindowTitle(); }
    const char *getAppletClass();

    //
    // Same as the super class, but also sets the icon name.
    //
    void setWindowTitle(const char *name, boolean check_geometry=FALSE);

    //
    // Change the dimensionality of the selected interactor.
    //
    boolean changeInteractorDimensionality(int new_dim);

    //
    // De/Select all instances in the control panel.
    //
    void selectAllInstances(boolean select);

    //
    // Get the next x,y position of an interactor being placed during a
    // series of placements into an initially empty control panel and
    // that use this method to get x,y positions of all placed interactors.
    // This needs to be public so InteractorNode can call it.
    //
    void getNextInteractorPosition(int *x, int *y);

    //
    // True if n is pointed to by some InteractorInstance in this->instanceList 
    // && that interactor is selected.  Used by Network::cfgPrintSelection 
    // because he wants to visit only certain nodes.
    //
    boolean nodeIsInInstanceList (Node *n);

    //
    // Supply the Position of the the NW most interactor.
    // ...used by Interactor::decideToDrag so that it can compute offsets.
    //
    void getMinSelectedXY(int *minx, int *miny);

    //
    //
    void mergePanels (ControlPanel *cp, int x, int y);

    //
    // J A V A     J A V A     J A V A
    //
    boolean printAsJava(FILE*);
    const char* getJavaVariableName();
    void getWorkSpaceSize(int* w, int *h);

    //
    // Change a resource in the Workspace widget that lets the user know
    // if things will overlap - during the course of moving
    //
    boolean toggleHitDetection();


    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassControlPanel;
    }
};


#endif // _ControlPanel_h
