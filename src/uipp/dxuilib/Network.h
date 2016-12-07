/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _Network_h
#define _Network_h

#include "enums.h"
#include "Base.h"
#include "List.h"
#include "NodeList.h"
#include "Parse.h"
#include "SymbolManager.h"
#include "PacketIF.h"
#include "WorkSpaceInfo.h"

typedef long CPNodeStatus;	// Also defined in ControlPanel.h

//
// Class name definition:
//
#define ClassNetwork	"Network"


//
// Defines the default output remapping mode.
// When FALSE, outputs are not to be remapped by the executive.
// This is however only advisor, it is up to the DrivenInteractorNodes and
// their derived classes to read their Network's remap mode an use it. 
//
#define DEFAULT_REMAP_INTERACTOR_MODE	FALSE

//
// Referenced classes:
//

class Dictionary;
class EditorWindow;
class GetSetConversionDialog;
class ImageWindow;
class ControlPanel;
class CommandScope;
class Command;
class Node;
class StandIn;
class SaveAsDialog;
class Dialog;
class SequencerNode;
class DeferrableAction;
class EditorWorkSpace;
class TransmitterNode;

class MacroDefinition;
class MacroParameterNode;
class ParameterDefinition;
class PanelGroupManager;
class PanelAccessManager;
class DXApplication;
class CascadeMenu;
class Decorator;
class UniqueNameNode;

#define FOR_EACH_NETWORK_NODE(network, node, iterator) \
 for (iterator.setList(network->nodeList) ; (node = (Node*)iterator.getNext()) ; )
#define FOR_EACH_NETWORK_PANEL(network, cp, iterator) \
 for (iterator.setList(network->panelList) ; \
	(cp = (ControlPanel*)iterator.getNext()) ; )
#define FOR_EACH_NETWORK_DECORATOR(network, decor, iterator) \
 for (iterator.setList(network->decoratorList); (decor = (Decorator*)iterator.getNext()); )


//
// Network class definition:
//				
class Network : public Base
{
    friend class DXApplication;	// For the constructor

  private:
    //
    // Methods for extern "C" functions reference by yacc. 
    //
    void parseComment(char *comment);
    void parseFunctionID(char *name);
    void parseArgument(char *name, const boolean isVarname);
    void parseLValue(char *name);
    void parseRValue(char *name);
    void parseStringAttribute(char *name, char *value);
    void parseIntAttribute(char *name, int value);
    void parseEndOfMacroDefinition();

    //
    // Parse all .net file comments.
    //
    void parseVersionComment(const char* comment, boolean netfile);
    boolean netParseComments(const char* comment, const char *file, 
							int lineno);
    void netParseMacroComment(const char* comment);
    void netParseMODULEComment(const char* comment);
    void netParseVersionComment(const char* comment);
    void netParseCATEGORYComment(const char* comment);
    void netParseDESCRIPTIONComment(const char* comment);
    void netParseCommentComment(const char* comment);
    void netParseNodeComment(const char* comment);

    //
    // Parse all .cfg file comments.
    //
    boolean cfgParseComments(const char* comment, const char *file, 
							int lineno);
    void cfgParsePanelComment(const char* comment);
    void cfgParseInteractorComment(const char* comment);
    void cfgParseNodeComment(const char* comment);
    void cfgParseVersionComment(const char* comment);
    ControlPanel *selectionOwner;


    //
    // Parsing routines used by yacc.
    //
    friend void ParseComment(char* comment);
    friend void ParseFunctionID(char* name);
    friend void ParseArgument(char* name, const boolean isVarname);
    friend void ParseLValue(char* name);
    friend void ParseRValue(char* name);
    friend void ParseIntAttribute(char* name, int value);
    friend void ParseStringAttribute(char* name, char* value);
    friend void ParseEndOfMacroDefinition();
    friend void yyerror(char *, ...);

    int visitNodes(Node *n);
    boolean markAndCheckForCycle(Node *srcNode, Node *dstNode);


    //
    // Static methods used by the DeferrableAction
    // class member below.
    //
    static void AddNodeWork(void *staticData, void *requestData);
    static void RemoveNodeWork(void *staticData, void *requestData);

    //
    // Static method used by the sendNetwork DeferrableAction
    // class member below.
    //
    static void SendNetwork(void *staticData, void *requestData);

    //
    // CommandInterfaces that use these two commands must arrange to set 
    // the instance number of the desired panel in the local data using
    // UIComponent::setLocalData().  Further, these are never de/activated()
    // and are created activated.  These are set up this way, because
    // anything that allows opening specific panels will have to be dynamic
    // (given an editor and the fact that the indices are kept in the
    // CommandInterface).  The CommandInterfaces that use these are created
    // in this->fillPanelCascade(). 
    //
    Command*		openPanelByInstanceCmd;
    Command*		openPanelGroupByIndexCmd;

    //
    // Used by prepareFor/completeNewNetwork() and addNode() to defer
    // notifying the editor of the new nodes.  Instead we notify the editor
    // after the network is complete. 
    //
    boolean             readingNetwork;

    //
    // Add the given panel this network and if the instance number is 0
    // then allocate a new instance number for the panel.
    //
    boolean addPanel(ControlPanel *p, int instance = 0);

    //
    // Check version numbers using rules taken from ParseVersionComment
    // If there is a problem post on Info Message
    //
    boolean versionMismatchQuery (boolean netfile, const char *file);

    //
    // If the net came from a file and the file was encrypted, 
    // then this is set to TRUE.
    //
    boolean netFileWasEncoded;

    static Decorator* lastObjectParsed;
    // Print decorator comments in the .net file.
    static void SetOwner(void*);
    static void DeleteSelections(void*);
    static void Select(void*);
    virtual boolean parseDecoratorComment (const char *comment,
                                const char *filename, int lineno);

    //
    // This is a helper function for optimizeNodeOutputCacheability() that
    // returns a list of Nodes that are connected to the given output of the
    // given node.  This would be trivial, except for Transmitters and 
    // Receivers, In which case we need to traverse all Receivers for a 
    // given Transmitter and all output arcs for each receiver.
    // If destList is given, then we append the Nodes to that list, 
    // otherwise we allocate one internally.  In either case, we return a 
    // list.  In the latter case, the user should delete the returned list.
    //
    static List *GetDestinationNodes(Node *src, int output_index, 
			List *destList, boolean rcvrsOk = FALSE);

    //
    // Do a recursive depth-first search of this network for macro nodes.
    // and append their MacroDefinitions to the given list.  The list will
    // come back in depth-first order and will NOT contain duplicates.
    // This has the side effect of loading the network bodies for all the
    // returned MacroDefinitions.
    //
    void getReferencedMacros(List *macros, List *visted = NULL);

public:
    class ParseState {
      public:
	//
	// This is state that is defined over a whole .net or .cfg file
	//
	static Network 		*network;
	static char		*parse_file;
	static boolean		error_occurred;
	static boolean 		stop_parsing;
	static boolean 		issued_version_error;
	static boolean 		ignoreUndefinedModules;
	static Dictionary	*undefined_modules;
	static Dictionary	*redefined_modules;
	static int		parse_mode;
	static boolean 		main_macro_parsed;

	ParseState(); 
	~ParseState(); 
	void initializeForParse(Network *n, int mode, const char *filename);
	void cleanupAfterParse();

	//
	// This is state that is set for parsing a macro within a .net file
	//
	int	parse_state;

#if WORKSPACE_PAGES
	//
	// What did we just parse?  Was it a node or a decorator?  There are
	// stateful comments which could apply to either and we must know
	// wether to parseDecoratorComment or node->netParseComment.
	// group comments belong to either node or decorator.
	//
	int     parse_sub_state;
#endif

	Node	*node;
	int	input_index;
	int	output_index;
	boolean	node_error_occurred;
	ControlPanel *control_panel;

    } parseState;

private:

    //
    // Keep track if whether or not the .net had a Get or a Set in it.
    //
    boolean fileHadGetOrSetNodes;	// Set during parse of .net file

    //
    // See fixupTransmitter() below for comment.
    //
    static List* RenameTransmitterList;

#if WORKSPACE_PAGES
    Dictionary* groupManagers;
#endif

  protected:
    friend class StandIn;
    friend class EditorWindow;
    friend class GetSetConversionDialog; // for getReferencedMacros

    //
    // Protected member data:
    //
    EditorWindow*	editor;		// editor for this network
    List		imageList;	// image window list
    List		panelList;	// control panel list
    NodeList		nodeList;	// network node list

    List		decoratorList;  // LabelDecorators in the vpe
#if 00
    List                pickList;       // List of pick nodes
    List                transmitterList;// Contains transmitters so that
					// receivers can find them.
    List                receiverList;   // Contains receivers without 
					// transmitters so that
					// new transmitters can find them.
    List		probeList;	// list of all probe nodes.
#endif

    char 		*description;
    char 		*comment;
    Symbol		category;
    Symbol		name;
    char 		*prefix;
    boolean		remapInteractorOutputs;

    DeferrableAction	*deferrableRemoveNode;
    DeferrableAction	*deferrableAddNode;
    DeferrableAction    *deferrableSendNetwork;


    struct {
	short		major;
	short		minor;
	short		micro;
    } 			netVersion;
    struct {
	short		major;
	short		minor;
	short		micro;
    } 			dxVersion;


    CommandScope*	commandScope;
    Command*		openAllPanelsCmd;
    Command*		closeAllPanelsCmd;
    Command*		newCmd;
    Command*		saveAsCmd;
    Command*		saveCmd;
    Command*		saveCfgCmd;
    Command*		openCfgCmd;
    Command*		setNameCmd;
    Command*		helpOnNetworkCmd;

    boolean		parse(FILE *f);
    boolean		dirty;
    boolean		fileDirty;
    char*               fileName;

    boolean             macro;
    MacroDefinition     *definition;
    boolean		deleting;		// True during destructor.

    SaveAsDialog	*saveAsDialog;
    Dialog		*saveCfgDialog;
    Dialog		*openCfgDialog;
    Dialog		*setNameDialog;
    Dialog		*setCommentDialog;
    Dialog		*helpOnNetworkDialog;

    //
    // Parse any network specific info out of the .cfg comments.
    //
    boolean cfgParseComment(const char *name, const char *file, int lineno);

    //
    // Used to allocate uniq ids for control panels in this network.
    //
    int 	lastPanelInstanceNumber;	

    PanelGroupManager* panelGroupManager;
    PanelAccessManager* panelAccessManager;

    //
    // This will reset the image count for isLastImage();
    int  imageCount;
    void resetImageCount();

    // 
    // Deletes a given panel from the panel list.
    // Should you be using this->closeControlPanel(...).
    //
    boolean deletePanel(ControlPanel* panel, boolean destroyPanelsNow = FALSE);

    //
    // Do any work that is necessary to read in a new .cfg file and/or
    // clear the network of any .cfg information.
    //
    void clearCfgInformation(boolean destroyPanelsNow = FALSE);

    //
    // The work space grid to be used by the editor.
    //
    WorkSpaceInfo	workSpaceInfo;


    // 
    // Ask various windows to reset their titles.
    // 
    void resetWindowTitles();



    //
    // Do whatever is necessary to speed up the process of reading in a
    // new network.  This includes letting the EditorWindow know by calling
    // editor->prepareFor/completeNewNetwork().
    //
    void prepareForNewNetwork();
    void completeNewNetwork();

#ifdef  DXUI_DEVKIT
    //
    // Print the visual program as a .c file that uses libDX calls.
    // An error message is printed if there was an error.
    //
    boolean printAsCCode(FILE *f);
#endif // DXUI_DEVKIT

    //
    // Print comments and 'include' statements for the macros that are
    // referenced in this network.  If nested is TRUE, then we don't print
    // the 'include' statements.  Return TRUE on sucess or FALSE on failure.
    // nested and processedMacros args are only really intended to be 
    // used in the recursive calls (i.e. not by generic users).
    //
    virtual boolean printMacroReferences(FILE *f, boolean inline_define,
                        PacketIFCallback echoCallback, void *echoClientData);

    //
    // Constructor:
    //  THIS CONSTRUCTOR SHOULD NEVER BE CALLED FROM ANYWHERE BUT
    //  DXApplication::newNetwork OR A SUBCLASS OF NETWORK.  ANY
    //  SUBCLASSES CONSTRUCTOR SHOULD ONLY BE CALLED FROM A VIRTUAL
    //  REPLACEMENT OF DXApplication::newNetwork.
    //
    Network();


    //
    // For editing operations which automatically chop arcs and replace
    // them with transmitters/receivers.
    //
    boolean	chopInputArk(Node*, int, Dictionary*, Dictionary*);

  public:
    
    SequencerNode* sequencer;	// FIXME! does this need to be public. 

    //
    // Destructor:
    //
    ~Network();

    //
    // Adds a given image window to the image window list.
    // Deletes a given image window from the image window list.
    //
    boolean addImage(ImageWindow* image);
    List   *getImageList() { return &this->imageList; }
    int		getImageCount() { return this->imageList.getSize(); }
    boolean removeImage(ImageWindow* image);

    //
    // Posts the sequencer
    void    postSequencer();

    void    setRemapInteractorOutputMode(boolean val = TRUE);
    boolean isRemapInteractorOutputMode()
		{ return this->remapInteractorOutputs; }


#if 00
    // Maintain the Transmitter and Receiver lists.
    // The transmitter list contains all transmitters, and the receiver
    // list contains only "transmitter-less" receivers.
    boolean addTransmitter(Node *n);
    boolean removeTransmitter(Node *n);
    Node   *getTransmitter(int i);
    int     getTransmitterCount();

    boolean addReceiver(Node *n);
    boolean removeReceiver(Node *n);
    Node   *getReceiver(int i);
    int     getReceiverCount();
#endif

    //
    // Probe management:
    //
#if 00
    boolean addProbe(Node*);
    boolean deleteProbe(Node*);
    boolean changeProbe(Node*);
    int	    getProbeListSize();
    List    *getProbeList();
#endif

    //
    // Pick management:
    //
#if 00
    boolean addPick(Node*);
    boolean deletePick(Node*);
    boolean changePick(Node*);
    int	    getPickListSize();
    List    *getPickList();
#endif

    //
    // Find a panel by index (indexed from 1). 
    //
    ControlPanel *getPanelByIndex(int index)
		{ ASSERT(index > 0); 
		  return (ControlPanel*) this->panelList.getElement(index); }
    int getPanelCount() { return  this->panelList.getSize(); }
    ControlPanel *getPanelByInstance(int instance);
    List *getNonEmptyPanels();

    //
    // Close the given panel indicated by panelInstance.
    // If panelInstance is 0, unmanage all panels.
    // If the given panel(s) contain 0 interactors, then also call 
    // this->deletePanel() on the panel. 
    // If do_unmanage is true, then unmanage the control panel(s).
    // do_unmanage should be FALSE when call from the control panel's
    // close callback.
    //
    void closeControlPanel(int panelInstance, boolean do_unmanage = TRUE);
    //
    // Open the given panel indicated by panelInstance.
    // If panelInstance is 0, manage all panels.
    //
    void openControlPanel(int panelInstance);
    //
    // Open the given panel group indicated by groupIndex.
    // Group indices are 1 based.
    //
    void openControlPanelGroup(int groupIndex);

    //
    // Access routines:
    //
    EditorWindow* getEditor()
    {
	return this->editor;
    }

    Command* getCloseAllPanelsCommand()
    {
	return this->closeAllPanelsCmd;
    }
    Command* getOpenAllPanelsCommand()
    {
	return this->openAllPanelsCmd;
    }
    Command* getNewCommand()
    {
	return this->newCmd;
    }
    Command* getSaveAsCommand()
    {
	return this->saveAsCmd;
    }
    Command* getSaveCommand()
    {
	return this->saveCmd;
    }

    //
    // These don't really belong in Network, however their inclusion
    // here obviates the need to include JavaNet.h in other places
    //
    virtual Command* getSaveWebPageCommand() { return NUL(Command*); }
    virtual Command* getSaveAppletCommand() { return NUL(Command*); }
    virtual Command* getSaveBeanCommand() { return NUL(Command*); }

    Command* getSaveCfgCommand()
    {
	return this->saveCfgCmd;
    }
    Command* getOpenCfgCommand()
    {
	return this->openCfgCmd;
    }
    Command* getSetNameCommand()
    {
	return this->setNameCmd;
    }
    Command* getHelpOnNetworkCommand()
    {
	return this->helpOnNetworkCmd;
    }
    boolean  clear(boolean destroyPanelsNow = FALSE);


    //
    // Net file format version access functions 
    //
    int	getDXMajorVersion() { return this->dxVersion.major; }
    int	getDXMinorVersion() { return this->dxVersion.minor; }
    int	getDXMicroVersion() { return this->dxVersion.micro; }
    //
    // Net file format version access functions 
    //
    int	getNetMajorVersion() { return this->netVersion.major; }
    int	getNetMinorVersion() { return this->netVersion.minor; }
    int	getNetMicroVersion() { return this->netVersion.micro; }
    void getVersion(int &maj, int& min, int& mic) 
			{ maj = this->netVersion.major; 
			  min = this->netVersion.minor; 
			  mic = this->netVersion.micro; }

    //
    // Name and Category manipulations 
    //
    void    setDescription(const char *description, boolean markDirty = TRUE);
    Symbol  setCategory(const char *cat, boolean markDirty = TRUE);
    Symbol  setName(const char *n);
    const char *getDescriptionString();
    const char *getCategoryString() 
		{ return theSymbolManager->getSymbolString(category); }
    Symbol  getCategorySymbol() { return category; }
    const char *getNameString() 
		{ return theSymbolManager->getSymbolString(name); }
    Symbol  getNameSymbol() { return name; }
    const char *getPrefix(); 
    const char *getNetworkComment() { return this->comment; }
    virtual void setNetworkComment(const char *s);


    //
    // Printing/sending script representations of this network 
    //
    virtual boolean printNetwork(FILE *f,
				PrintType dest);
    virtual boolean printHeader(FILE *f,
				PrintType dest,
				PacketIFCallback echoCallback = NULL,
				void *echoClientData = NULL);
    virtual boolean printBody(FILE *f,
			      PrintType dest,
			      PacketIFCallback echoCallback = NULL,
			      void *echoClientData = NULL);
    virtual boolean printTrailer(FILE *f,
				 PrintType dest,
				 PacketIFCallback echoCallback = NULL,
				 void *echoClientData = NULL);
    virtual boolean printValues(FILE *f, PrintType dest);
    virtual boolean sendNetwork();
    virtual boolean sendValues(boolean force = FALSE);

    //
    // Convert a filename (with or without corresponding extensions)
    // to a filename with the correct extension.
    //
    static char *FilenameToNetname(const char *filename);
    static char *FilenameToCfgname(const char *filename);

    //
    // Determine if the network is in a state that can be saved to disk.
    // If so, then return TRUE, otherwise return FALSE and issue an error.
    //
    boolean isNetworkSavable();

    //
    // Save the network as a file in the .net and .cfg files
    //
    virtual boolean saveNetwork(const char *name, boolean force = FALSE);

    boolean openCfgFile(const char *name, boolean openStartup = FALSE, 
					  boolean send = TRUE);
    boolean saveCfgFile(const char *name);

    //
    // markNetwork sets the flag marked[j] (0 based) if note j
    // is connected to node i (0 based) in the d direction where d < 0
    // means above (to its inputs), and d > 0 means below (to its outputs),
    // and 0 means in both directions.
    int     connectedNodes(boolean *marked, int i, int d);
    void    sortNetwork();

    boolean checkForCycle(Node *srcNode, Node *dstNode);
    virtual boolean readNetwork(const char *netfile, const char *cfgfile = NULL,
		boolean ignoreUndefinedModules = FALSE);

    // These are called by readNetwork() and any other functions that want to
    // READ a .net file. Returns FILE* and deals with encoded networks.
    // The static version is provided for others to open and read .net files,
    // but they must save and pass the wasEncoded value into 
    // CloseNetworkFILE(). 
    // NOTE that encoding is done outside dxui (by dxencode).  
    //
    static FILE* OpenNetworkFILE(const char *netfile, 
				boolean *wasEncoded, char **errmsg = NULL);
    FILE* openNetworkFILE(const char *netfile, char **errmsg = NULL);
    //
    // Close the .net file that was opened with [Oo]penNetworkFILE().
    //
    static void CloseNetworkFILE(FILE *f, boolean wasEncoded);
    void closeNetworkFILE(FILE *f);


    //
    // Save the network as a file in the .net and .cfg files and any
    // per Node files (auxiliary)
    //
    boolean netPrintNetwork(const char *name);
    boolean cfgPrintNetwork(const char *name, PrintType dest = PrintFile);
    void setCPSelectionOwner (ControlPanel *cp) { this->selectionOwner = cp; }
    ControlPanel *getSelectionOwner () { return this->selectionOwner; }
    boolean auxPrintNetwork();

    boolean isDirty() { return this->dirty; }
    void    setDirty();
    void    clearDirty() { this->dirty = FALSE; }
    void    setFileDirty(boolean val = TRUE) { this->fileDirty = val; }
    void    clearFileDirty() { this->fileDirty = FALSE; }
    //
    // Is the network different than that on disk.  Before using this,
    // be careful, to be sure you shouldn't be using saveToFileRequired() 
    // instead.
    //
    boolean isFileDirty() { return this->fileDirty; }

    //
    // Return true if the network is different from that on disk, and the
    // application allows saving of .net and .cfg files.
    //
    boolean saveToFileRequired();


    // Add a node to the network's node list.
    void addNode(Node *node, EditorWorkSpace *where = NULL);
    void deleteNode(Node *node, boolean undefer=TRUE);
    Node *findNode(Symbol name, int instance);

    Node *findNode(const char* name, int* startpos = NULL, boolean byLabel = FALSE);

    void postSaveAsDialog(Widget parent, Command* cmd = NULL);
    void postSaveCfgDialog(Widget parent);
    void postOpenCfgDialog(Widget parent);

    boolean postNameDialog();
    void editNetworkComment();
    void postHelpOnNetworkDialog();

    //
    // isMacro returns true if this network can be considered a macro.
    // canBeMacro returns true if there is nothing in the net that prevents
    // it from being a macro.
    // makeMacro makes this network a macro if do is true, else it makes it
    // be a "normal" net.
    boolean isMacro();
    boolean canBeMacro();
    boolean makeMacro(boolean make);

    ParameterDefinition *getInputDefinition(int i);
    ParameterDefinition *getOutputDefinition(int i);
    int getInputCount();
    int getOutputCount();

    boolean moveInputPosition(MacroParameterNode *n, int index);
    boolean moveOutputPosition(MacroParameterNode *n, int index);

    void setDefinition(MacroDefinition *md);
    MacroDefinition *getDefinition();

    //
    // Colormap Management functions
    //
    void    openColormap(boolean openAll);

    Node   *getNode(const char *name, int instance);

    //
    // Manipulate the panel instance numbers for panels in this network.
    //
    void   resetPanelInstanceNumber()
		{ this->lastPanelInstanceNumber = 0; }
    void   setLastPanelInstanceNumber(int last)
		{  ASSERT(last >= 0); 
		this->lastPanelInstanceNumber = last; }
    int	   getLastPanelInstanceNumber()
		{ return this->lastPanelInstanceNumber; }
    int	   newPanelInstanceNumber()
		{ return ++this->lastPanelInstanceNumber; }
    //
    // Allocate a new control panel that belongs to this network.
    //
    ControlPanel *newControlPanel(int instance = 0);

    PanelAccessManager* getPanelAccessManager()
    {
	return this->panelAccessManager;
    }

    PanelGroupManager* getPanelGroupManager()
    {
	return this->panelGroupManager;
    }

    const char* getFileName()
    {
	return this->fileName;
    }

    WorkSpaceInfo *getWorkSpaceInfo()
    {
	return &this->workSpaceInfo;
    }

    //
    // Returns true if the caller is the last element of the image list.
    // It is assumed that each member of this list will determine, during
    // Node::prepareToSendNode, will call this to see if it's the last one.
    boolean isLastImage();

    //
    // Given lists of old and new NodeDefinitions, redefine any nodes 
    // in the current network.
    //
    boolean redefineNodes(Dictionary *newdefs, Dictionary *olddefs);

    //
    // Find the first free index (indices start from 1) for nodes with the
    // given name.
    // Currently, this only works for Input/Output nodes.
    //
    int findFreeNodeIndex(const char *nodename);

    boolean isDeleted() {return this->deleting;}

    int getNodeCount();

    //
    // Place menu items (ButtonInterfaces) in the CascadeMenu, that when 
    // executed cause individually named panels and panel groups to be opened.
    // If a PanelAccessManager, then only those ControlPanels defined to be 
    // accessible are placed in the CascadeMenu.
    // NOTE: to optimize this method, if the menu is returned deactivated()
    //   it may not contain the correct contents and so should no be activated()
    //   except through subsequent calls to this method.
    //
    void fillPanelCascade(CascadeMenu *menu, PanelAccessManager *pam);

    //
    // Merge two nets together, resolving instance collisions.  Delete the
    // net being passed in.
    //
    // The stitch param (added 11/8/02) indicates what should be done when the
    // incoming network has type/instance number conflicts with the existing
    // network.  In the stitch case, the conflicts should be resolved by
    // deleting the node in the incoming network and shifting its wires over
    // to the conflicted node in the existing network.  The use of this is
    // undoing a deletion of a set of nodes.
    //
    boolean mergeNetworks(Network *, List *panels, boolean all, boolean stitch=FALSE);
    boolean mergePanels (Network *);

    //
    // The decorators are held in this->decoratorList (no interactors in the list)
    //
    void addDecoratorToList (void *e); 
    void removeDecoratorFromList (void *e); 
    const List* getDecoratorList() { return &this->decoratorList; }

    //
    // get and set x,y,width,height of a net in the vpe
    //
    void setTopLeftPos(int, int);
    void getBoundingBox (int *x1, int *y1, int *x2, int *y2);

    //
    // look through the network list and make a node list of the given class.  
    // If classname == NULL, then all nodes are included in the list.
    // If classname != NULL, and includeSubclasses = FALSE, then nodes must
    // be of the given class and not derived from the given class.
    // If no nodes were found then NULL is returned.
    // The returned List must be deleted by the caller.
    //
    List *makeClassifiedNodeList(const char *classname,
				 boolean includeSubclasses = TRUE);

    List *makeNamedControlPanelList(const char *name);
    List *makeLabelledNodeList(const char *label);

    //
    // Search the network for a node of the given class and return TRUE
    // if found.
    //
    boolean containsClassOfNode(const char *classname);

    //
    // look through the network list and make a list of the nodes with
    // the give name.
    // If no nodes were found then NULL is returned.
    // The returned List must be deleted by the caller.
    //
    List *makeNamedNodeList(const char *nodename);

    boolean isReadingNetwork() { return this->readingNetwork; }

#ifdef DXUI_DEVKIT
    //
    // Print the visual program as a .c file that uses libDX calls.
    // If the filename does not have a '.c' extension, one is added.
    // An error message is printed if there was an error.
    //
    boolean saveAsCCode(const char *filename);

#endif // DXUI_DEVKIT

    //static void        Network::GetDestinationNodes (Node*, int, List*, List*);



    //
    // Optimally set the output cacheability of all nodes within the network.
    // We only operate on those that have writable cacheability.
    //
    void optimizeNodeOutputCacheability();


    boolean wasNetFileEncoded() { return this->netFileWasEncoded;}

    //
    // See if the given label is unique among nodes requiring uniqueness.
    // Return NUL(char*) if unique.  Otherwise return the node name of the
    // guy with whom we have a conflict.
    //
    const char* nameConflictExists(UniqueNameNode* passed_in, const char* label);
    const char* nameConflictExists(const char* label);


    //
    // Maintain a list of transmitters which need fixing up after a network read.
    // Older versions of dx weren't restrictive about name conflicts.  Now it
    // you aren't allowed name conflicts among Transmitter,Receiver,Input,Output.
    // So when reading a net with a name conflict, let it pass.  Just maintain a list
    // of the conflicts and go back and fix them after the read.
    // It's only necessary to fix the transmitters because changing them automatically
    // fixes the receivers. (See renameTransmitters(), and RenameList.)
    // If/when fixes are applied, we'll form a message string to show the user and
    // store the string in a list.  EditorWindow can show the list if/when there
    // is a vpe for the network.  No vpe/no display messages.
    //
    void fixupTransmitter(TransmitterNode*);
    void renameTransmitters();
    List* editorMessages;
    void addEditorMessage(const char* msg);
    void showEditorMessages();

    //
    // Called by Remove/AddNodeWork(), but can also be called directly.
    // This had always been protected but is now public because we need to
    // revisit command activation for reasons other than adding/removing nodes.
    //
    virtual void changeExistanceWork(Node *n, boolean adding);

#if WORKSPACE_PAGES
    Dictionary* getGroupManagers() { return this->groupManagers; }
    void	copyGroupInfo (Node* , List* );
    void	copyGroupInfo (Node* , Node* );
    boolean	chopArks(List*, Dictionary*, Dictionary*);
    boolean	replaceInputArks(List*, List*);
#endif

    virtual boolean isJavified() { return FALSE; }

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassNetwork;
    }
};


#endif // _Network_h

