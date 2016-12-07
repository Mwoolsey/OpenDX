/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#ifdef OS2
#include <stdlib.h>
#include <types.h>
#endif
#include <time.h>
#include <stdio.h>	// For fprintf(), fseek() and rename()

#if defined(DXD_WIN) || defined(OS2)          //SMH get correct name for unlink for NT in stdio.h
#define unlink _unlink

#if 0
#if defined(windows) && defined(HAVE_WINSOCK_H)
#include <winsock.h>
#elif defined(HAVE_CYGWIN_SOCKET_H)
#include <cygwin/socket.h>
#elif defined(HAVE_SYS_SOCKET_H)
#include <sys/socket.h>
#endif
#endif

#undef send             //SMH without stepping on things
#undef FLOAT
#undef PFLOAT
#else
#include <unistd.h>	// For unlink()
#endif
#include <errno.h>	// For errno
#include <sys/stat.h>

#include "lex.h"
#include "DXStrings.h"
#include "Network.h"
#include "List.h"
#include "ListIterator.h"
#include "Parameter.h"

#include "CommandScope.h"
#include "ControlPanel.h"
//#include "InfoDialogManager.h"
#include "ErrorDialogManager.h"
#include "WarningDialogManager.h"
#include "InfoDialogManager.h"

#include "AccessNetworkPanelsCommand.h"
#include "NewCommand.h"
#include "NoUndoNetworkCommand.h"

#include "SaveAsDialog.h"
#include "SaveCFGDialog.h"
#include "OpenCFGDialog.h"
#include "SetMacroNameDialog.h"

#include "Parse.h"
#include "MacroDefinition.h"
#include "NodeDefinition.h"
#include "Node.h"
#include "InteractorNode.h"
#include "DXLInputNode.h"
#include "DXLOutputNode.h"
#include "TransmitterNode.h"
#include "ReceiverNode.h"
#include "MacroNode.h"
#include "Ark.h"
#include "SymbolManager.h"
#include "DXPacketIF.h"
#include "ApplicIF.h"
#include "DXVersion.h"
#include "EditorWindow.h"
#include "ImageWindow.h"
#include "DXAnchorWindow.h" // for resetWindowTitle()
#include "ImageNode.h"
#include "StandIn.h" 
#include "SequencerNode.h"
#include "ProcessGroupManager.h"
#if WORKSPACE_PAGES
#include "PageGroupManager.h"
#include "AnnotationGroupManager.h"
#include "GroupStyle.h"
#endif
#include "PanelGroupManager.h"
#include "PanelAccessManager.h"
#include "MacroParameterNode.h"

#include "DXApplication.h"

#include "ColormapNode.h"
#include "DisplayNode.h"
#include "UniqueNameNode.h"

#include "Dictionary.h"
#include "NDAllocatorDictionary.h"
#include "SetNetworkCommentDialog.h"
#include "HelpOnNetworkDialog.h"
#include "DeferrableAction.h"
#include "DictionaryIterator.h"
#include "VPEAnnotator.h"
#include "DecoratorStyle.h"
#include "DecoratorInfo.h"
#include "EditorWorkSpace.h"

#include "CascadeMenu.h"
#include "ButtonInterface.h"
#include "QuestionDialogManager.h"

#include "MsgWin.h"

#ifdef	DXD_WIN
#include <process.h>
#define	getpid	_getpid
#define	popen	_popen
#define pclose	_pclose
#endif

//
// These are the file name extensions assumed for the network file and
// the configuration file.  The code assumes that they are 4 characters
// in length
//
static const char NetExtension[] = ".net";
static const char CfgExtension[] = ".cfg";

#ifdef DXD_OS_NON_UNIX           //SMH allow upper case extensions as well
static const char NetExtensionUpper[] = ".NET";
static const char CfgExtensionUpper[] = ".CFG";
#endif

static void GenerateModuleMessage(Dictionary *nodes, char *msg, boolean error);

#define DEFAULT_MAIN_NAME "main"
/***
 *** Constants:
 ***/

#define _PARSE_CONFIGURATION	1	// Currently parsing the .cfg file
#define _PARSE_NETWORK		2	// Currently parsing the .net file

#define _LEXER_RUNNING		0	// Initialized and lexing
#define _LEXER_INITIALIZE	1	// Set up the lexer for a new run

#define _PARSED_NONE		0	
#define _PARSED_NODE		1	// Parsing the node's cfg info
#define _PARSED_PANEL		2	// Parsing the panel's info
#define _PARSED_INTERACTOR	3	// Parsing the interactor's info
#define _PARSING_NETWORK_CFG	4	// Parsing the network's cfg info 
#define _PARSED_VCR		5	// Parsing the vcr's info 
#define _PARSED_WORKSPACE	6	// Parsing the editor workspace info 
#define _PARSED_VERSION		7	// Parsing .cfg application comments

#if WORKSPACE_PAGES
#define _SUB_PARSED_NONE	0	
#define _SUB_PARSED_NODE	1	// Parsed a node most recently
#define _SUB_PARSED_DECORATOR	2       // Parsed a node most recently
#endif

Network          *Network::ParseState::network;
char             *Network::ParseState::parse_file;
boolean          Network::ParseState::error_occurred;
boolean          Network::ParseState::stop_parsing;
boolean          Network::ParseState::issued_version_error;
boolean          Network::ParseState::ignoreUndefinedModules;
Dictionary       *Network::ParseState::undefined_modules;
Dictionary       *Network::ParseState::redefined_modules;
int              Network::ParseState::parse_mode;
boolean          Network::ParseState::main_macro_parsed;

List*		Network::RenameTransmitterList = NUL(List*);

Network::ParseState::ParseState()
{
    this->parse_file = NULL;
}
Network::ParseState::~ParseState()
{
}
void Network::ParseState::cleanupAfterParse() 
{
    if (this->parse_file)
	delete this->parse_file;
    this->parse_file = NULL;
}
void Network::ParseState::initializeForParse(Network *n, int mode, 
						const char *filename)
{
    this->network = n;
    this->parse_mode = mode;
    if (this->parse_file)
	delete this->parse_file;
    this->parse_file = DuplicateString(filename);
    this->node                = NULL;
    this->parse_state         = _PARSED_NONE;
#if WORKSPACE_PAGES
    this->parse_sub_state     = _SUB_PARSED_NONE;
#endif
    this->main_macro_parsed   = FALSE;
    this->error_occurred      = FALSE;
    this->node_error_occurred = FALSE;
    this->stop_parsing        = FALSE;
    this->control_panel       = NUL(ControlPanel*);

    if (mode == _PARSE_NETWORK) {

        this->issued_version_error = FALSE;

	//
	// Initialize the undefined/redefined modules dictionaries.
	//
	if (Network::ParseState::undefined_modules)
	    Network::ParseState::undefined_modules->clear();
	else
	    Network::ParseState::undefined_modules = new Dictionary;

	if (Network::ParseState::redefined_modules)
	    Network::ParseState::redefined_modules->clear();
	else
	    Network::ParseState::redefined_modules = new Dictionary;
    }
}

Network::Network()
{
    this->deleting	= FALSE;

    this->editor       	= NUL(EditorWindow*);
    this->saveAsDialog 	= NUL(SaveAsDialog*);
    this->saveCfgDialog = NUL(Dialog*);
    this->openCfgDialog = NUL(Dialog*);
    this->setNameDialog = NUL(Dialog*);
    this->setCommentDialog 	= NULL;
    this->helpOnNetworkDialog 	= NULL;
    this->prefix 	= NUL(char*);
    this->description 	= NULL;
    this->comment 	= NULL;
    this->sequencer    	= NUL(SequencerNode*);
    this->fileName     	= NUL(char *);
    this->readingNetwork = FALSE; 
    this->definition 	= NULL;
    this->remapInteractorOutputs = FALSE;

    this->commandScope = new CommandScope;
    this->openAllPanelsCmd =
	new AccessNetworkPanelsCommand
	    ("openAllControlPanels", this->commandScope, FALSE,
		this, AccessNetworkPanelsCommand::OpenAllPanels);

    //
    // CommandInterfaces that use this must arrange to set the instance
    // number of the desired panel in the local data using 
    // UIComponent::setLocalData();
    //
    this->openPanelByInstanceCmd =
	new AccessNetworkPanelsCommand
	    ("openPanelByInstance", this->commandScope, TRUE,
		this, AccessNetworkPanelsCommand::OpenPanelByInstance);
    //
    // CommandInterfaces that use this must arrange to set the instance
    // number of the desired panel in the local data using 
    // UIComponent::setLocalData();
    //
    this->openPanelGroupByIndexCmd =
	new AccessNetworkPanelsCommand
	    ("openPanelGroupByIndex", this->commandScope, TRUE,
		this, AccessNetworkPanelsCommand::OpenPanelGroupByIndex);

    this->closeAllPanelsCmd =
	new AccessNetworkPanelsCommand
	    ("closeAllControlPanels", this->commandScope, FALSE, 
		this, AccessNetworkPanelsCommand::CloseAllPanels);

    this->newCmd =
	new NewCommand ("new", this->commandScope, FALSE, this,
			NULL /* We don't have an anchor window yet */);

    this->saveAsCmd =
        new NoUndoNetworkCommand("saveAsCommand", this->commandScope, 
			FALSE,
			this, NoUndoNetworkCommand::SaveNetworkAs);

    this->saveCmd =
        new NoUndoNetworkCommand("saveCommand", this->commandScope, 
			FALSE, this, NoUndoNetworkCommand::SaveNetwork);
    this->saveCfgCmd =
        new NoUndoNetworkCommand("saveCfgCommand", this->commandScope, 
			FALSE, this, NoUndoNetworkCommand::SaveConfiguration);
    this->openCfgCmd =
        new NoUndoNetworkCommand("openCfgCommand", this->commandScope, 
			FALSE, this, NoUndoNetworkCommand::OpenConfiguration);
    this->setNameCmd =
        new NoUndoNetworkCommand("setNameCommand", this->commandScope, 
			TRUE, this, NoUndoNetworkCommand::SetNetworkName);
    this->helpOnNetworkCmd =
        new NoUndoNetworkCommand("helpOnNetworkCommand", this->commandScope,
			FALSE,this,NoUndoNetworkCommand::HelpOnNetwork);

    this->panelGroupManager = new PanelGroupManager(this);
    this->panelAccessManager = new PanelAccessManager(this);

    //
    // Set the activation of commands that depend on execution.
    // Remember to remove them in the destructor.
    //
    theDXApplication->executingCmd->autoDeactivate(this->newCmd);
    theDXApplication->notExecutingCmd->autoActivate(this->newCmd);
    this->newCmd->autoActivate(theDXApplication->executeOnChangeCmd);

    this->deferrableRemoveNode = new DeferrableAction(
				Network::RemoveNodeWork, (void*)this);
    this->deferrableAddNode = new DeferrableAction(
				Network::AddNodeWork, (void*)this);
    this->deferrableSendNetwork = new DeferrableAction(
				Network::SendNetwork, (void*)this);

    this->selectionOwner = NUL(ControlPanel*);

#if WORKSPACE_PAGES
    this->groupManagers = BuildTheGroupManagerDictionary(this);
#endif

    this->editorMessages = NUL(List*);

    this->clear();
}


Network::~Network()
{

#ifndef __PURIFY__
    ASSERT(this != theDXApplication->network);
#endif

    this->deleting = TRUE;

    // TRUE means delete the panels now... don't add them to the dump list.
    this->clear(TRUE);

    //
    // This must go after clear() in case the Nodes have items in the
    // editor. 
    //
    if (this->editor) {
	if (!this->editor->isAnchor())
	    delete this->editor;
	this->editor = NULL;
    }

    //
    // Remove the auto de/activated commands from the 'server' commands.
    //
    theDXApplication->executingCmd->removeAutoCmd(this->newCmd);
    theDXApplication->notExecutingCmd->removeAutoCmd(this->newCmd);


    delete this->openAllPanelsCmd;
    delete this->openPanelByInstanceCmd;
    delete this->openPanelGroupByIndexCmd;
    delete this->closeAllPanelsCmd;
    delete this->newCmd;
    delete this->saveAsCmd;
    delete this->saveCmd;
    if (this->saveCfgCmd) delete this->saveCfgCmd;
    if (this->openCfgCmd) delete this->openCfgCmd;
    delete this->setNameCmd;
    delete this->helpOnNetworkCmd; 

    //
    // Delete the command scope after the commands.
    //
    delete this->commandScope;

    //
    // Delete the dialogs.
    //
    if (this->setCommentDialog) delete this->setCommentDialog;
    if (this->helpOnNetworkDialog) delete this->helpOnNetworkDialog;
    if (this->saveAsDialog) 	delete this->saveAsDialog;
    if (this->saveCfgDialog) 	delete this->saveCfgDialog;
    if (this->openCfgDialog) 	delete this->openCfgDialog;
    if (this->setNameDialog) 	delete this->setNameDialog;

    //
    // Delete miscellaneous objects 
    //
    delete this->deferrableSendNetwork; 
    delete this->deferrableRemoveNode; 
    delete this->deferrableAddNode; 
    delete this->panelGroupManager;
    delete this->panelAccessManager;

    ListIterator it(this->decoratorList);
    Decorator *dec;
    while ( (dec = (Decorator*)it.getNext()) ) {
	this->decoratorList.removeElement((void*)dec);
        delete dec;
    }

#if WORKSPACE_PAGES
    //
    // Delete the dictionary for group managers and every GroupManager
    //
    GroupManager *gmgr;
    DictionaryIterator di(*this->groupManagers);
    while ( (gmgr = (GroupManager*)di.getNextDefinition()) )
	delete gmgr;
    delete this->groupManagers;
#endif

    if (this->editorMessages) {
	ListIterator ml(*this->editorMessages);
	char *msg;
	while ( (msg = (char*)ml.getNext()) ) 
	    delete msg;
	delete this->editorMessages;
    }
}

//
// Do any work that is necessary to read in a new .cfg file and/or
// clear the network of any .cfg information.  
//
void Network::clearCfgInformation(boolean destroyPanelsNow)
{
    ControlPanel *p;
    while ( (p = (ControlPanel*) this->panelList.getElement(1)) ) 
	this->deletePanel(p, destroyPanelsNow);
    this->resetPanelInstanceNumber();
    this->panelGroupManager->clear();
    this->panelAccessManager->clear();

    Node *node;
    ListIterator l;
    FOR_EACH_NETWORK_NODE(this, node, l) 
	node->setDefaultCfgState();

}

boolean Network::clear(boolean destroyPanelsNow)
{
    boolean was_deleting = this->deleting;

    this->deleting = TRUE;

    this->prepareForNewNetwork();

    //
    // Reset the macro name, category, and description
    //
    this->name = theSymbolManager->registerSymbol(DEFAULT_MAIN_NAME);
    if (this->description) {
	delete this->description;
        this->description 	= NULL;
    }
    if (this->fileName) delete this->fileName;
    this->fileName = NUL(char*);
    this->setNetworkComment(NULL);
    if (this->prefix) 
    { 
	delete this->prefix;
        this->prefix = NUL(char*);
    }
 

    //
    // Delete the most basic elements first, since these may muck
    // around with some of the others (i.e. InteractorNodes access their
    // ControlPanels).  Delete each node from the network list before we
    // delete it so that there are not references to deleted nodes during
    // subsequent node deletions.
    //
    Node *n;
    while ( (n = (Node*) this->nodeList.getElement(1)) ) {
	this->deleteNode(n);
    }
    Decorator *dec;
    while ( (dec = (Decorator*)this->decoratorList.getElement(1)) ) {
	this->decoratorList.removeElement((void*)dec);
	delete dec;
    }

    //
    // Now clean up the control panels and other information in the .cfg .
    //
    this->clearCfgInformation(destroyPanelsNow);

    //
    // Delete all the image windows except the anchor.
    //
    ImageWindow *iw, *anchor = NULL;
    while( (iw = (ImageWindow*)imageList.getElement(1)) ) {
	this->imageList.deleteElement(1);
	if (iw->isAnchor()) {	
	    ASSERT(anchor == NULL);
	    anchor = iw;
	} else {
	    delete iw;
	}
    }
    if (anchor)
	this->imageList.appendElement((void*)anchor);
    

    if (this->helpOnNetworkDialog) 
        this->helpOnNetworkDialog->unmanage();
    
    if (this->setCommentDialog) 
        this->setCommentDialog->unmanage();
    

    //
    // Clear the process group manager if it's the main network.
    //
#if WORKSPACE_PAGES
    GroupManager *gmgr;
    DictionaryIterator di(*this->groupManagers);
    while ( (gmgr = (GroupManager*)di.getNextDefinition()) )
	gmgr->clear();
#else
    if (this == theDXApplication->network)
    	theDXApplication->PGManager->clear();
#endif

    //
    // Reset the node instance numbers only if there are no other nodes
    // currently in the system.  This currently means that there are no
    // macros defined.
    //
    if ((this == theDXApplication->network) &&
				(theDXApplication->macroList.getSize() == 0))
        NodeDefinition::ResetInstanceNumbers(theNodeDefinitionDictionary);

    this->category = 0;
    this->macro = FALSE;
    this->dirty = TRUE;
    this->fileDirty = FALSE;

    if ((this->definition) && (this->definition->isReadingNet() == FALSE)) {
	delete this->definition;
	this->definition = NULL;
    }

    //
    // Assume all networks are 0.0.0 unless the version comment in the
    // .net file indicates otherwise.  This assumes that when we write
    // out the .net file we use NETFILE_*_VERSION instead of these.
    //
    this->netVersion.major = 0;	
    this->netVersion.minor = 0;
    this->netVersion.micro = 0;

    //
    // Assume all networks were written with dx version 1.0.0 unless the 
    // version comment in the .net file indicates otherwise.  This assumes 
    // that when we write out the .net file we use DX_*_VERSION instead of 
    // these.
    //
    this->dxVersion.major = 0;	
    this->dxVersion.minor = 0;
    this->dxVersion.micro = 0;


    this->completeNewNetwork();

    //
    // Go back to the default work space configuration.  This must be done
    // AFTER the workspace widgets have been marked as being_destroyed in
    // order for the scrollbars to be removed.
    //
    this->workSpaceInfo.setDefaultConfiguration();

    this->netFileWasEncoded = FALSE;

    if (!was_deleting)
	this->deleting = FALSE;

    this->changeExistanceWork(NUL(Node*), FALSE);
    return TRUE;
}
//
// Do whatever is necessary to speed up the process of reading in a 
// new network.  This includes letting the EditorWindow know by calling
// editor->prepareForNewNetwork().
//

void Network::prepareForNewNetwork()
{
    EditorWindow *editor = this->editor;

    if (Network::RenameTransmitterList) 
	Network::RenameTransmitterList->clear();

    this->deferrableAddNode->deferAction();
    this->deferrableRemoveNode->deferAction();

    //
    // Let the editor know that the new network is coming. 
    //
    if (editor) 
	editor->prepareForNewNetwork();	

    this->fileHadGetOrSetNodes = FALSE;
}
//
// Do whatever is necessary to speed up the process of reading in a 
// new network.  This includes letting the EditorWindow know by calling
// editor->completeNewNetwork().
//
void Network::completeNewNetwork()
{
    //
    // This is to accomadate an optimization in addNode() in which nodes
    // are placed on the beginning of the list instead of the end.  This
    // should make finding connected nodes quicker, but leaves 
    // this->nodeList unsorted.
    //
    if (this->readingNetwork)
	this->sortNetwork();

    //
    // Let the editor know that the new network is done. 
    //
    if (editor) {
	editor->installWorkSpaceInfo(&this->workSpaceInfo);
	editor->completeNewNetwork();
    }

    this->resetWindowTitles();

    //
    // Undefer the calls to changeNodeExistanceWork(), which may result in
    // a call to AddNodeWork().
    //
    this->deferrableAddNode->undeferAction();
    this->deferrableRemoveNode->undeferAction();
}

void Network::AddNodeWork(void *staticData, void *requestData)
{
     Network *n = (Network*)staticData;
     n->changeExistanceWork((Node*)requestData, TRUE);
}
void Network::RemoveNodeWork(void *staticData, void *requestData)
{
     Network *n = (Network*)staticData;
     n->changeExistanceWork((Node*)requestData, FALSE);
}
void Network::changeExistanceWork(Node *n, boolean adding)
{
    List *l;
    ListIterator iter;
    boolean hasCfg = FALSE;

    //
    // If this isn't the 'main' network in dxui, then don't modify
    // the commands of DXApplication.  We arrive at this point in the
    // code whenever temporary networks are modified.  That shouldn't
    // change activation of application commands.  ...bug106
    //
    boolean modify_application_commands = (this == theDXApplication->network);

	if (adding) {
		List dummy;
		if (n) {
			dummy.appendElement((void*)n);
			iter.setList(dummy);
		} else {
			iter.setList(this->nodeList);
		}
		while ( (n = (Node*)iter.getNext()) ) {
			if (!this->sequencer && n->isA(ClassSequencerNode))	
				this->sequencer = (SequencerNode*)n;	
			n->initializeAfterNetworkMember();
			if (modify_application_commands) {
				if (!theDXApplication->openAllColormapCmd->isActive() &&
					n->isA(ClassColormapNode))
					theDXApplication->openAllColormapCmd->activate();	
			}
			if (!hasCfg && n->hasCfgState())
				hasCfg = TRUE;
		}
		this->newCmd->activate();
		if (theDXApplication->appAllowsSavingNetFile(this))
			this->saveAsCmd->activate();
		if (this->fileName) {
			if (theDXApplication->appAllowsSavingNetFile(this)) 
				this->saveCmd->activate();
			if (hasCfg) {
				if (theDXApplication->appAllowsSavingCfgFile() &&  
					this->saveCfgCmd) 
					this->saveCfgCmd->activate();
				if (this->openCfgCmd) this->openCfgCmd->activate();
			}
		}
	} else {		// Not adding
		if (n) {
			if (this->sequencer && n->isA(ClassSequencerNode))
				this->sequencer = NULL;
			if (theDXApplication->openAllColormapCmd->isActive() &&
				n->isA(ClassColormapNode))
				if (modify_application_commands) {
					if (!this->containsClassOfNode(ClassColormapNode))
						theDXApplication->openAllColormapCmd->deactivate();
				}
		} else {
			if (modify_application_commands) {
				if (theDXApplication->openAllColormapCmd->isActive() &&
					!this->containsClassOfNode(ClassColormapNode))
					theDXApplication->openAllColormapCmd->deactivate();
			}
			if (this->sequencer && 
				!this->containsClassOfNode(ClassSequencerNode))
				this->sequencer = NULL;	
		}

		iter.setList(this->nodeList); 
		while (!hasCfg && (n = (Node*)iter.getNext()))  
			hasCfg = n->hasCfgState();

		int count = this->nodeList.getSize();
		int decorCount = this->decoratorList.getSize();
		int groupCnt = 0;
		GroupManager *gmgr;
		DictionaryIterator di(*this->groupManagers);
		while ( (gmgr = (GroupManager*)di.getNextDefinition()) )
			groupCnt+= gmgr->getGroupCount();
		if ((count == 0) && (decorCount == 0) && (groupCnt == 0)) {
			this->newCmd->deactivate();
			this->saveCmd->deactivate();
			this->saveAsCmd->deactivate();
		} 
		if ((count == 0) || !hasCfg) {
			if (this->saveCfgCmd) this->saveCfgCmd->deactivate();
			if (this->openCfgCmd) this->openCfgCmd->deactivate();
		}
	} 

	// FIXME : where does this go? Does it belong in DisplayNode
	//
	// Adjust which DisplayNode is "last"
	//
	l = this->makeClassifiedNodeList(ClassDisplayNode);

	if(l)
	{
		ListIterator li(*l);
		Node        *node;

		while ( (node = (Node*)li.getNext()) )
		{
			// If this is not the last DisplayNode...
			if(li.getPosition()-1 != l->getSize())
			{
				if(((DisplayNode *)(node))->isLastImage())
					((DisplayNode *)(node))->setLastImage(FALSE);
			}
			else
			{
				if(!((DisplayNode *)(node))->isLastImage())
					((DisplayNode *)(node))->setLastImage(TRUE);
			}
		}
		delete l;
	}
}

//
// Allocate a new control panel that belongs to this network.
//
ControlPanel *Network::newControlPanel(int instance)
{
    ASSERT(instance >= 0);

    //
    // Allocate and initialize the panel.
    //
    ControlPanel *panel = theDXApplication->newControlPanel(this);
    ASSERT(panel);

    //
    // Initialization is done when creating the window.
    //
    // panel->initialize();

    this->addPanel(panel, instance);

    return panel;
}
boolean Network::addPanel(ControlPanel *panel, int instance)
{
    //
    // Add it to the network's list of panels.
    //
    boolean result = this->panelList.appendElement(panel);
    ASSERT(result);

    //
    // Allocate an instance number for the panel.
    //
    if (instance == 0) {
	panel->setInstanceNumber(this->newPanelInstanceNumber());
    } else {
	int last_instance = this->getLastPanelInstanceNumber();
	ASSERT(instance > last_instance);
	panel->setInstanceNumber(instance);
	this->setLastPanelInstanceNumber(instance); 
    }
    //
    // Make sure the relevant commands are activated.
    //
    this->openAllPanelsCmd->activate();
    this->closeAllPanelsCmd->activate();

    //
    // Notify the DXWindow's (which includes updating their panelAccessDialog). 
    //
    theDXApplication->notifyPanelChanged();

    //
    // Notify the editor (which includes the PanelGroupDialog).
    //
    EditorWindow *editor = this->getEditor();
    if (editor)
    {
        editor->notifyCPChange();
    }
   
    return TRUE;
}

ControlPanel *Network::getPanelByInstance(int instance)
{
    ListIterator iterator(this->panelList);
    ControlPanel *cp;

    while ( (cp = (ControlPanel*)iterator.getNext()) ) {
        if (cp->getInstanceNumber() == instance)
            return cp;
    }
    return NULL;
}


void Network::postSequencer()
{
    ASSERT(this->sequencer);

    this->sequencer->openDefaultWindow(NUL(Widget));

}

//
// Tell all data-driven interactors that they should ask the executive to
// remap their outputs.
//
void Network::setRemapInteractorOutputMode(boolean val)
{
    Node *node;
    ListIterator l;
     
    if (this->remapInteractorOutputs == val)
	return;

    FOR_EACH_NETWORK_NODE(this, node, l) {
	if (node->isA(ClassInteractorNode)) {
	    InteractorNode *inode = (InteractorNode*)node;
	    if (inode->hasRemappableOutput())
		    inode->setOutputRemapping(val);
	}
    }

    this->remapInteractorOutputs = val;

}

//
// Get the prefix that is prepended to identifiers when communicating with 
// the Executive.
//
const char *Network::getPrefix()
{
    const char *s;

    if (!this->prefix) {
       ASSERT(this->name != 0);
       s = theSymbolManager->getSymbolString(this->name);
       ASSERT(s != NUL(const char*));
       this->prefix = new char[STRLEN(s) + 2];
       strcpy(this->prefix,s);
       strcat(this->prefix,"_");
    }
    return this->prefix;
}

boolean Network::deletePanel(ControlPanel* panel, boolean destroyPanelsNow)
{
    boolean result;
    int     position;

    ASSERT(panel);

    if ( (position = this->panelList.getPosition(panel)) )
    {
	//
	// Delete the panel from the list.
	//
	if ( (result = this->panelList.deleteElement(position)) )
	{
	    if (this->panelList.getSize() == 0)
	    {
		//
		// If no more panels, deactivate the
		// "Open All Control Panels" command.
		//
		this->openAllPanelsCmd->deactivate();
		this->closeAllPanelsCmd->deactivate();
	    }
	    //
	    // Move it into the 'dumpedList' and delete it later, but make
	    // sure it is removed from the display by unmanaging it.
	    //
	    // 3/31 - don't always add them to the dump list because it causes
	    // free-memory-read problems when clearing the dump list following
	    // destruction of a net.
	    //
	    if (panel->isManaged())
		panel->unmanage();
	    if (!destroyPanelsNow) theDXApplication->dumpObject(panel);

	    //
	    // Remove all the InteractorInstances now, so that (probably
	    // among other things) the Interactors are not left associated 
	    // with the Nodes when reading in a new .cfg file.
	    //
       	    panel->clearInstances(); 

	    //
	    // Notify the DXWindow's (which includes updating their
	    // panelAccessDialog). 
	    //
	    theDXApplication->notifyPanelChanged();

	    //
	    // Notify the editor (which includes the PanelGroupDialog).
	    //
	    if(this->getEditor())
                this->getEditor()->notifyCPChange();

	    if (destroyPanelsNow) delete panel;
	}
    }
    else
    {
	result = FALSE;
    }

    return result;
}

boolean Network::addImage(ImageWindow* image)
{
    ASSERT(image);

    return this->imageList.appendElement(image);
}


boolean Network::removeImage(ImageWindow* image)
{
    int position;

    if ( (position = this->imageList.getPosition(image)) )
    {
	return this->imageList.deleteElement(position);
    }
    else
    {
	return FALSE;
    }
}

void Network::addNode(Node *node, EditorWorkSpace *where)
{
    this->setDirty();

    //
    // This should make finding connected nodes quicker, but leaves 
    // this->nodeList unsorted (see this->completeNewNetwork()).
    //
    if (this->readingNetwork)
	this->nodeList.insertElement(node,1);
    else
	this->nodeList.appendElement(node);

    if (this->editor)
        this->editor->newNode(node, where);


    this->deferrableAddNode->requestAction((void*)node);
}

void Network::setDirty()
{  
    this->dirty = TRUE;
    this->fileDirty = TRUE;

    DXExecCtl *execCtl = theDXApplication->getExecCtl();

    execCtl->terminateExecution();
}


void Network::deleteNode(Node *node, boolean undefer)
{
    //
    // Disconnect the node from other nodes in the network first.
    // This helps us when a node's destructor results in a call to visitNodes()
    // in which arcs from other nodes are followed to the deleted node, 
    // resulting in the node being put back in the Network.
    //
    node->disconnectArks();

    this->nodeList.removeElement(node);

    if (undefer) this->deferrableRemoveNode->requestAction((void*)node);

    delete node;        // Marks network dirty in ~Node()...
    //
    // No confirmation dialog asks the user to save the empty network.
    //
    if(this->nodeList.getSize() == 0)
        this->clearFileDirty();

}

Node *Network::findNode(Symbol name, int instance)
{
    ListIterator li(this->nodeList);
    Node        *n;

    for(n = (Node*)li.getNext(); n != NULL; n = (Node*)li.getNext())
    {
	if ((n->getNameSymbol() == name) && 
	    ((n->getInstanceNumber() == instance) OR (instance == 0)) )
	    return n; 
    }
    return NUL(Node*);
}
boolean Network::containsClassOfNode(const char *classname)
{
    ListIterator iterator; 
    Node        *node;

    FOR_EACH_NETWORK_NODE(this, node, iterator) {
        if (EqualString(node->getClassName(),classname)) 
	    return TRUE;
    }
    return FALSE; 

}
//
// look through the network list and make a list of the given class.  
// If classname == NULL, then all nodes are included in the list.
// If classname != NULL, and includeSubclasses = FALSE, then nodes must
// be of the given class and not derived from the given class.
// If no nodes were found then NULL is returned.
// The returned List must be deleted by the caller.
//
List *Network::makeClassifiedNodeList(const char *classname,
					boolean includeSubclasses)
{
    Node *node;
    ListIterator iterator;
    List        *l = NUL(List*);

    if ((classname != NUL(char*)) && (includeSubclasses == FALSE)) 
	return this->nodeList.makeClassifiedNodeList(classname);

    FOR_EACH_NETWORK_NODE(this, node, iterator) {
        if ((classname == NULL) || 
	    (includeSubclasses && node->isA(classname)) ||
	    EqualString(node->getClassName(),classname)) {
            if (!l)
                l = new List;
            l->appendElement((const void*) node);
        }
    }
    return l;
}
//
// look through the network list and make a list of the nodes with
// the give name. 
// If no nodes were found then NULL is returned.
// The returned List must be deleted by the caller.
//
List *Network::makeNamedNodeList(const char *nodename)
{
    Node *node;
    ListIterator iterator;
    List        *l = NUL(List*);


    FOR_EACH_NETWORK_NODE(this, node, iterator) {
        if (EqualString(node->getNameString(),nodename)) {
            if (!l)
                l = new List;
            l->appendElement((const void*) node);
        }
    }
    return l;
}

//
// Close the given panel indicated by panelInstance.
// If panelInstance is 0, unmanage all panels. 
// If the given panel(s) do not contain any interactors, then delete them
// from the network. 
//
void Network::closeControlPanel(int panelInstance, boolean do_unmanage)
{
    ListIterator  iterator(this->panelList);
    List	   toDelete, toUnmanage;
    ControlPanel* panel;

    while ( (panel = (ControlPanel*)iterator.getNext()) ) {
	if ((panelInstance == 0) || 
	    (panel->getInstanceNumber() == panelInstance)) {
	    // getComponentCount is InstanceCount + DecoratorCount
	    if (panel->getComponentCount() == 0)
	        toDelete.appendElement((void*)panel);
	    if (do_unmanage && panel->isManaged())
	        toUnmanage.appendElement((void*)panel);
	}
    }

    //
    // Now that we aren't iterating the panelList, we can do 
    // this->deletePanel() and panel->unmanage(). 
    //
    iterator.setList(toDelete);
    while ( (panel = (ControlPanel*)iterator.getNext()) )
	this->deletePanel(panel);
    iterator.setList(toUnmanage);
    while ( (panel = (ControlPanel*)iterator.getNext()) )
	panel->unmanage();
    
}
//
// Open the given panel indicated by panelInstance.
// If panelInstance is 0, manage all panels. 
// If panelInstance is <0 , manage all panels that are marked as startup panels.
//
void Network::openControlPanel(int panelInstance)
{
	ControlPanel* panel;

	if (panelInstance < 0) {
		ListIterator iterator(this->panelList);
		while ( (panel = (ControlPanel*) iterator.getNext()) ) {
			if (panel->isStartup())
				panel->manage();
		}
	} else if (panelInstance == 0 ) {
		ListIterator  iterator(this->panelList);
		while ( (panel = (ControlPanel*)iterator.getNext()) )
			panel->manage();
	} else {
		panel = this->getPanelByInstance(panelInstance);
		if (panel)
			panel->manage();
	}
}
//
// Open the given panel group indicated by groupIndex.
// Group indices are 1 based.
//
void Network::openControlPanelGroup(int groupIndex)
{
    List plist;
    int panelInstance;

    if (this->panelGroupManager->getPanelGroup(groupIndex,&plist)) {
    	ListIterator  iterator(plist);
    	while ( (panelInstance = (int)(long)iterator.getNext()) ) 
	    this->openControlPanel(panelInstance);
    }
}



/***
 *** External variables:
 ***/

extern "C"
{
extern
FILE* yyin;			/* parser input stream	  */

#if !defined(YYLINENO_DEFINED)
int yylineno;			/* flex line number */
#else
extern int yylineno;			/* lexer line number      */
#endif

extern int yylexerstate;

extern int yyparse();
}

/***
 *** Static variables:
 ***/
#ifdef NoParseState

static
Dictionary *_undefined_modules = NUL(Dictionary*);
				/* undefined module list  */

static
Dictionary *_redefined_modules = NUL(Dictionary*);
				/* redefined module list  */


static
boolean _ignoreUndefinedModules;
static
boolean _main_macro_parsed;	/* main macro parsed?	  */

static
int _parse_mode;		/* parsing mode		  */

static
int _parse_state;		/* parse state		  */

static
const char* _parse_file;		/* parse file string	  */

static
Network* _network;		/* the network		  */
static
Node* _node;			/* the current Node	  */
ControlPanel *_control_panel;	/* the current control panel */

static
boolean _error_occurred;	/* error found ?	  */

static
boolean _stop_parsing;		/* All parse functions return immediately */
static 
boolean _issued_version_error; 

static
boolean _node_error_occurred;	/* node error flag	  */

static
int _input_index;		/* current input param index	  */

// The name of the current module (during argument parsing).
static char _current_module[64];

static
int _output_index;		/* current output param index	  */
#endif // NoParseState


//
// Open a .net file and a .cfg file (if it exists) and read the network.
// 'netfile' contains the name of network file and should include the .net
// extension.
//
boolean Network::readNetwork(const char *netFile, const char *cfgFile,
							 boolean ignoreUndefinedModules)
{
	Decorator *dec;
	ListIterator it;

	FILE *f;
	int	namesize;
	char *netfile, *cfgfile;

	Network::ParseState::ignoreUndefinedModules = ignoreUndefinedModules;

	netfile = Network::FilenameToNetname(netFile);

	f = this->openNetworkFILE(netfile);

	//
	// If there is an editor and this net file is encoded, then don't 
	// allow reading the .net.  Note, to catch all cases, we must also 
	// not allow creating of Editor's for encoded nets that have already
	// been read in (i.e. in image mode).  
	// See DXApplication::newNetworkEditor() and/or EditorWindow::manage().
	//
	if (this->getEditor() && this->wasNetFileEncoded()) {
		ErrorMessage("This visual program is encoded and is "
			"therefore not viewable with the VPE");
		if (f) {
			this->closeNetworkFILE(f);	
			f = NULL;
		}
	}

	if (!f) {
		delete netfile;
		this->clear();
		return FALSE;
	}

	if (this->fileName != NULL)
		delete this->fileName;
	this->fileName = netfile;

#ifdef NoParseState
	//
	// Initialize the undefined/redefined modules dictionaries.
	//
	if (_undefined_modules)
		_undefined_modules->clear();
	else
		_undefined_modules = new Dictionary;
	if (_redefined_modules)
		_redefined_modules->clear();
	else
		_redefined_modules = new Dictionary;
#endif

	//
	// Parse the .net file
	//
	this->readingNetwork = TRUE;
	this->prepareForNewNetwork();
#ifdef NoParseState
	_parse_mode = _PARSE_NETWORK;
	_parse_file = netfile;
#else
	this->parseState.initializeForParse(this,_PARSE_NETWORK,netfile);
#endif
	boolean parse_terminated =  this->parse(f);
	this->completeNewNetwork();
	this->readingNetwork = FALSE;
	this->closeNetworkFILE(f);	
#ifdef NoParseState
#else
	this->parseState.cleanupAfterParse();
#endif

	if (parse_terminated)  {
		this->clear();
		return FALSE;
	}

	//
	// Parse the .cfg file if it exists.
	//
	if (!cfgFile) {
		namesize = STRLEN(netfile);
		cfgfile = DuplicateString(netfile);
#ifndef DXD_OS_NON_UNIX          //SMH for NT we'll should handle upper case as well
		ASSERT(netfile[namesize-4] == NetExtension[0]); 
		cfgfile[namesize-4]  = CfgExtension[0];
		ASSERT(netfile[namesize-3] == NetExtension[1]); 
		cfgfile[namesize-3]  = CfgExtension[1];
		ASSERT(netfile[namesize-2] == NetExtension[2]); 
		cfgfile[namesize-2]  = CfgExtension[2];
		ASSERT(netfile[namesize-1] == NetExtension[3]); 
		cfgfile[namesize-1]  = CfgExtension[3];
#else
		ASSERT(netfile[namesize-4] == NetExtension[0]);
		cfgfile[namesize-4]  = CfgExtension[0];
		if (netfile[namesize-3] == NetExtension[1])
		{
			cfgfile[namesize-3]  = CfgExtension[1];
			ASSERT(netfile[namesize-2] == NetExtension[2]);
			cfgfile[namesize-2]  = CfgExtension[2];
			ASSERT(netfile[namesize-1] == NetExtension[3]);
			cfgfile[namesize-1]  = CfgExtension[3];
		}
		else if (netfile[namesize-3] == NetExtensionUpper[1])
		{
			cfgfile[namesize-3]  = CfgExtensionUpper[1];
			ASSERT(netfile[namesize-2] == NetExtensionUpper[2]);
			cfgfile[namesize-2]  = CfgExtensionUpper[2];
			ASSERT(netfile[namesize-1] == NetExtensionUpper[3]);
			cfgfile[namesize-1]  = CfgExtensionUpper[3];
		}
		else
		{
			fprintf(stderr,"Internal error detected at \"%s\":%d.\n",
				__FILE__,__LINE__);
		}
#endif
	} else
		cfgfile = (char*)cfgFile;

	this->openCfgFile(cfgfile, FALSE, FALSE);

	if (cfgfile != cfgFile)
		delete cfgfile;

	//
	// If we are in image mode then open control panels that are marked
	// as 'startup' control panels and the sequencer if it exists.
	//
	if (!theDXApplication->inEditMode() && 
		!theDXApplication->appSuppressesStartupWindows()) {
			ListIterator iterator;

#if USE_WINDOW_STARTUP
			//
			// Do the image windows. 
			//
			ImageWindow *iw;
			iterator.setList(this->imageList);
			while(iw = (ImageWindow*)iterator.getNext()) {
				if (iw->isStartup())
					iw->manage();
			}
#endif

			//
			// Do the startup control panels.
			//
			this->openControlPanel(-1);

			//
			// Do the sequencer. 
			//
			if ((this->sequencer) && (this->sequencer->isStartup()))
				this->postSequencer();
	}

	if (this->editor) {
		//
		// turn line routing off before adding multiple vpe comments because
		// each one can cause a line reroute.
		//
		boolean hasdec = (this->decoratorList.getSize() > 0);
		if (hasdec) {
			this->editor->beginPageChange();
			FOR_EACH_NETWORK_DECORATOR (this, dec, it) {
				this->editor->newDecorator(dec);
			}
			this->editor->endPageChange();
		}
	}

	this->setDirty();


	//
	// Mark the network file dirty if there were redefined modules.
	//
	if (Network::ParseState::redefined_modules->getSize() > 0)
		this->setFileDirty();
	else
		this->clearFileDirty();

	/*
	* Generate one collective error message for redefined modules.
	*/
	GenerateModuleMessage(Network::ParseState::redefined_modules, 
		"The following tools have been redefined",
		FALSE);

	/*
	* Generate one collective error message for undefined modules.
	*/
	char msg[1024];
	if (!strcmp(this->getNameString(), "main")) 
		SPRINTF(msg,
		"The main visual program contains undefined tools.\n"
		"Reload the visual program after loading the following tools");
	else
		SPRINTF(msg,
		"Macro %s contains undefined tools.\n"
		"Reload macro %s after loading the following tools",
		this->getNameString(), this->getNameString());
	GenerateModuleMessage(Network::ParseState::undefined_modules, msg, TRUE);

	//
	// These three are 0.0.0 unless they were parsed out of the .net
	// which doesn't contain DX version numbers until DX 2.0.2
	//
	int dx_major = this->getDXMajorVersion();
	int dx_minor = this->getDXMinorVersion();
	int dx_micro = this->getDXMicroVersion();
	int dx_version =   VERSION_NUMBER( dx_major, dx_minor, dx_micro); 
	//
	// Version checking was added after version 2.0.2 of Data Explorer
	//
	if (!ParseState::issued_version_error && 
		(dx_version > VERSION_NUMBER(2,0,2))) {
			int net_major = this->getNetMajorVersion();
			int net_minor = this->getNetMinorVersion();
			// int net_micro = this->getNetMicroVersion();
			// int net_version =   VERSION_NUMBER( net_major, net_minor, net_micro); 
			if (VERSION_NUMBER(net_major, net_minor,0) > 
				VERSION_NUMBER(NETFILE_MAJOR_VERSION,NETFILE_MINOR_VERSION,0)) {
					const char *name = theDXApplication->getInformalName();
					WarningMessage(
						"%s%s was saved in a format defined by\n"
						"a release of %s (version %d.%d.%d) that is\n"
						"more recent than this version (%d.%d.%d).\n"
						"Contact your support center if you would like to obtain a\n"
						"version of %s that can fully support this visual program." ,
						(this->isMacro()?"The macro " : "The visual program "),
						(this->isMacro()?this->getDefinition()->getNameString():""),
						name,dx_major,dx_minor,dx_micro,
						DX_MAJOR_VERSION,DX_MINOR_VERSION,DX_MICRO_VERSION,
						name);
			} 
	}

	//
	// Fixup name conflicts with Transmitters/Receivers read in from old nets.
	// (Must be done after readingNetwork is reset to FALSE, and dirty bit is cleared.)
	//
	this->renameTransmitters();

	return TRUE;

}

//
// Reset all the relevant window titles.
// FIXME: this should be done by sending all DXWindows (i.e. clients
// of the application) a MsgNetworkChanged message, but that requires
// move newCmd, saveCmd and saveAsCmd up into DXApplication where
// the belong anyway.
//
void Network::resetWindowTitles()
{
DXWindow *anchor = theDXApplication->getAnchor();
boolean anchor_reset = FALSE;

    if (this->editor) {
	if (this->editor == anchor) anchor_reset = TRUE;
	this->editor->resetWindowTitle();
    }

    ListIterator li(this->imageList);
    ImageWindow *iw;
    while( (iw = (ImageWindow*)li.getNext()) ) {
	if (iw == anchor) anchor_reset = TRUE;
	iw->resetWindowTitle();
    }

    // 
    // ... and to deal with kiosk in a most clumsy way.  
    // FIXME: Only 3 of the guys who subclass from DXWindow have resetWindowTitle().
    // 
    if ((!anchor_reset)&&(anchor)) {
	if (strcmp(anchor->getClassName(), ClassDXAnchorWindow) == 0) {
	    DXAnchorWindow *dxa = (DXAnchorWindow*)anchor;
	    dxa->resetWindowTitle();
	}
    }

}
/*****************************************************************************/
/* Parse -								     */
/*                                                                           */
/*                                                                           */
/*****************************************************************************/

#if defined(USING_BISON)
extern "C" int yychar;
#endif

//
// Returns TRUE if the parse was prematurely terminated.
//
boolean Network::parse(FILE*    input)
{
    boolean  result;
    


    ASSERT(input);

#ifdef NoParseState
    /*
     * Save the network. 
     */
    _network = this;
#endif

    /*
     * Initialize...
     */
    yyin                 = input;
    yylineno             = 1;
    yylexerstate	 = _LEXER_INITIALIZE;
#ifdef NoParseState
    _node		 = NULL;
    _parse_state         = _PARSED_NONE;
    _main_macro_parsed   = FALSE;
    _error_occurred      = FALSE;
    _node_error_occurred = FALSE;
    _stop_parsing 	 = FALSE;
    _issued_version_error = FALSE;
    _control_panel       = NUL(ControlPanel*);
#endif

    result = TRUE;

#if defined(USING_BISON)
    for (;;)
#else
    while (NOT feof(yyin))
#endif
    {
	/*
	 * If parse failed...
	 */
	if (yyparse()) {
	    result = FALSE;
	    break;
	}

#if defined(USING_BISON)
	if (yychar == 0)
	    break;
#endif

	/*
	 * If error found...
	 */
	if (Network::ParseState::stop_parsing || Network::ParseState::error_occurred) {
	    result = FALSE;
	    break;
	}
    }

    // Only nodes found in the .net file are added to the network.
    if (result && this->parseState.parse_mode == _PARSE_NETWORK && 
				this->parseState.node != NULL) {
	this->addNode(this->parseState.node);
	this->parseState.node = NUL(Node*);
#if 1
    //
    // If there's an error, then parseState.node is being cast adrift and in fact,
    // might sail its way back and wind up in a Network at some point.  This chunk
    // might be a little destabilizing.  I don't know what all the ramifications might
    // be.
    //
    } else if ((this->parseState.node) && (this->parseState.parse_mode==_PARSE_NETWORK)) {
	this->addNode(this->parseState.node);
	this->parseState.node = NUL(Node*);
#endif
    }

    return Network::ParseState::stop_parsing;
}

//
// Post either an error or a warning dialog with the given msg followed
// by the names of the modules found in the dictionary. 
// If the dictionary is empty, no dialog is posted.
//
static void GenerateModuleMessage(Dictionary *nodes, char *msg, boolean error)
{
    int i, count;
    char *p, *message;

    /*
     * Generate one collective error message. 
     */
    count = nodes->getSize();
    if (count == 0)
	return;

    //
    // sanity check.  Otherwise we'll dump core in ErrorMessage because of
    // a fixed buffer.
    //
    count = MIN(count, 20);

    message = new char [STRLEN(msg) + 128 * count];

    sprintf(message, "%s:\n",msg);
    p = message + STRLEN(message);

    for (i = 1; i <= count; i++, p += STRLEN(p))
    {
	if (i == count)
	    sprintf(p,"     %s.",nodes->getStringKey(i));
	else
	    sprintf(p,"     %s,\n",nodes->getStringKey(i));
    }

    if (error)
	ErrorMessage(message);
    else
	WarningMessage(message);
    delete message;
}
/*****************************************************************************/
/* ParseMODULEComment -						     */
/*                                                                           */
/* Parses "MODULE" comment.						     */
/*                                                                           */
/*****************************************************************************/

void Network::netParseMODULEComment(const char* comment)
{
    int  items_parsed;
    char name[1024];

    ASSERT(comment);

    items_parsed = sscanf(comment, " MODULE %[^\n]", name);

    if (items_parsed != 1)
    {
	ErrorMessage("Invalid MODULE comment at %s:%d", 
				Network::ParseState::parse_file, yylineno);
	Network::ParseState::error_occurred = TRUE;
	return;
    }

//    cout << "Module comment: " << name << ":\n";
//    cout.flush();


    this->name = theSymbolManager->registerSymbol(name);
    if (this->prefix)
	delete this->prefix;
    this->prefix = NULL;
}

void Network::netParseMacroComment(const char* comment)
{
    int  items_parsed;
    char type[1024],name[1024],path[1024];

    ASSERT(comment);

    //
    // Comments are one of the following: 
    //
    //    // macro reference (direct) : MACRO_NAME MACRO_PATH
    //    // macro reference (indirect) : MACRO_NAME MACRO_PATH
    //    // macro definition : MACRO_NAME 
    //
    items_parsed = sscanf(comment, " macro %[^:]: %[^ ] %[^\n]", type, name, path);

    if (items_parsed != 3)
    {
	ErrorMessage("Invalid macro comment at %s:%d(ignoring)", 
				Network::ParseState::parse_file, yylineno);
	return;
    } else if (strstr(type,"reference")) {
	NodeDefinition *nd = 
	    (NodeDefinition*)theNodeDefinitionDictionary->findDefinition(name);

	if (!nd) {	//  Try to load it for them
	    char *errmsg = NULL;
	    char buf[2048];
	    SPRINTF(buf,"Attempting to load undefined macro %s from file %s"
			" while reading %s", 
				name,path, Network::ParseState::parse_file); 
	    theDXApplication->getMessageWindow()->addInformation(buf);
	    if (!MacroDefinition::LoadMacro(path,&errmsg)) {
		Network::ParseState::undefined_modules->addDefinition(
								name,NULL);
		SPRINTF(buf,"Attempt to load %s failed: %s", name,errmsg);
	        theDXApplication->getMessageWindow()->addInformation(buf);
	        delete errmsg; 
	    } else {
		SPRINTF(buf,"Attempt to load %s was successful",name);
	        theDXApplication->getMessageWindow()->addInformation(buf);
	    }
	    theDXApplication->getMessageWindow()->manage();
	}

    }	// else we ignore 'macro' comments
    return;
}

//
// userQuery is a new method in QuestionDialogManager specially designed for the
// temporally challenged.  It doesn't return until the user has responded - it
// contains its own mini event loop (which is a mini loop for events, not a loop 
// for mini events).
//
// If the destination file doesn't currently exist don't bother doing anything.
//
boolean Network::versionMismatchQuery (boolean netfile, const char *file)
{
boolean want_to_proceed = TRUE;
int response;

    if ((file) && (file[0])) {
	struct STATSTRUCT statbuf;
	if (STATFUNC(file, &statbuf) == -1) return TRUE;
    }

    if ((this->netVersion.major >= 2) && netfile) {
	char buf[1024];
	if (VERSION_NUMBER(this->netVersion.major, 0, 0) <
	    VERSION_NUMBER(NETFILE_MAJOR_VERSION, 0, 0)) {
	    if (this->dxVersion.major > 0) {
		sprintf (buf, "Saving Data Explorer %d.%d.%d program file in "
			"Data Explorer %d.%d.%d format.\n"
			"NOTE: Programs saved by this version of Data Explorer "
			"will NOT be readable by Data Explorer %d.%d.%d.\n"
			"Do you want to proceed?",
			this->dxVersion.major, 
			this->dxVersion.minor,
			this->dxVersion.micro,
			DX_MAJOR_VERSION,
			DX_MINOR_VERSION,
			DX_MICRO_VERSION,
			this->dxVersion.major,
			this->dxVersion.minor,
			this->dxVersion.micro);
		response = theQuestionDialogManager->userQuery (
		    theDXApplication->getRootWidget(), 
		    (char *)buf, (char *)"Version Mismatch",
		    (char *)"Yes", (char *)"No", (char *)NULL);
		switch (response) {
		    case QuestionDialogManager::OK:
			want_to_proceed = TRUE;
			break;
		    default:
			want_to_proceed = FALSE;
			break;
		}
	    } else {
		sprintf (buf, "Saving Data Explorer program file in "
			"Data Explorer %d.%d.%d format.\n"
			"NOTE: Programs saved by this version of Data Explorer "
			"will NOT be readable by the version of\nData Explorer "
			"that wrote this program.\n"
			"Do you want to proceed?",
			DX_MAJOR_VERSION,
			DX_MINOR_VERSION,
			DX_MICRO_VERSION);
		response = theQuestionDialogManager->userQuery (
		    theDXApplication->getRootWidget(), buf, "Version Mismatch",
		    "Yes", "No", NULL);
		switch (response) {
		    case QuestionDialogManager::OK:
			want_to_proceed = TRUE;
			break;
		    default:
			want_to_proceed = FALSE;
			break;
		}
	    }
	} 
    }
   return want_to_proceed;
}


/*****************************************************************************/
/* parseVersionComment -						     */
/*                                                                           */
/* Parses "version" comment.						     */
/*                                                                           */
/*****************************************************************************/

void Network::parseVersionComment(const char* comment, boolean netfile)
{
	int  items_parsed;
	int  net_major=0;
	int  net_minor=0;
	int  net_micro=0;
	int  dx_major=0;
	int  dx_minor=0;
	int  dx_micro=0;

	ASSERT(comment);

	items_parsed =
		sscanf(comment, " version: %d.%d.%d (format), %d.%d.%d", 
		&net_major, &net_minor, &net_micro,
		&dx_major, &dx_minor, &dx_micro);

	if (netfile) {
		if (items_parsed != 6)
		{
			items_parsed =
				sscanf(comment, " version: %d.%d.%d", 
				&net_major, &net_minor, &net_micro);
			if (items_parsed != 3)
			{

				ErrorMessage("Invalid version comment at %s:%d", 
					Network::ParseState::parse_file, 
					yylineno);
				Network::ParseState::error_occurred = TRUE;
				return;
			}
		} else {
			//
			// Some sample .net file went out marked as DX version 2.1.100.
			// This happened when we decided to make the rcs 7.0.2 branch,
			// into the refresh release 2.1.5.  At the time the 7.0.2 branch 
			// was a "developer's" branch and thus marked as
			// version 2.1.100.  This fixes the information message that
			// pops up with version 3.x and later versions of dxui that
			// tells what version of DX the .net was saved with.
			//
			if ((dx_major == 2) && (dx_minor == 1) && (dx_micro == 100))
				dx_micro = 5;
			this->dxVersion.major = dx_major;
			this->dxVersion.minor = dx_minor;
			this->dxVersion.micro = dx_micro;
		}
		this->netVersion.major = net_major;
		this->netVersion.minor = net_minor;
		this->netVersion.micro = net_micro;
	} else {
		if (items_parsed != 6) {
			ErrorMessage("Invalid version comment at %s:%d", 
				Network::ParseState::parse_file, yylineno);
			Network::ParseState::error_occurred = TRUE;
			return;
		}
	}


    //
    // Check all version 2 and later .nets and .cfg against the
    // current NETFILE version number.  If the .net or .cfg has 
    // a later major version number then don't attempt to parse
    // the file.
    //
    if ((net_major > 2) && (net_major > NETFILE_MAJOR_VERSION)) {
	const char *name = theDXApplication->getInformalName();
        char buf[1024];
        if (netfile) {
            if (this->isMacro()) {
                MacroDefinition *md = this->getDefinition();
                sprintf(buf,"The macro %s", md->getNameString());
            } else
                strcpy(buf,"This visual program");
        } else {
            strcpy(buf,"This configuration file");
        }
        ErrorMessage(
           "%s was saved in an incompatible format by\n"
           "a release of %s (version %d.%d.%d) that is\n"
           "more recent than this version (%s %d.%d.%d).\n"
           "Contact your support center if you would like to obtain a\n"
           "version of %s that can support this visual program." ,
                   buf,
                   name,dx_major,dx_minor,dx_micro,
                   name,DX_MAJOR_VERSION,DX_MINOR_VERSION,DX_MICRO_VERSION,
                   name);
	ParseState::issued_version_error = TRUE;
	Network::ParseState::stop_parsing = TRUE;
    } 
}

void Network::cfgParseVersionComment(const char* comment)
{
    Network::parseVersionComment(comment,FALSE);
}
void Network::netParseVersionComment(const char* comment)
{
    Network::parseVersionComment(comment,TRUE);
}

/*****************************************************************************/
/* ParseCATEGORYComment -						     */
/*                                                                           */
/* Parses "CATEGORY" comment.						     */
/*                                                                           */
/*****************************************************************************/

void Network::netParseCATEGORYComment(const char* comment)
{
    int  items_parsed;
    char category[1024];

    ASSERT(comment);

    items_parsed = sscanf(comment, " CATEGORY %[^\n]", category);

    if (items_parsed != 1)
    {
	ErrorMessage("Invalid CATEGORY comment at %s:%d", 
				Network::ParseState::parse_file, yylineno);
	Network::ParseState::error_occurred = TRUE;
	return;
    }

    this->setCategory(category, FALSE);
}


/*****************************************************************************/
/* ParseDESCRIPTIONComment -					     */
/*                                                                           */
/* Parses "DESCRIPTION" comment.					     */
/*                                                                           */
/*****************************************************************************/

void Network::netParseDESCRIPTIONComment(const char* comment)
{
    int  items_parsed;
    char description[1024];

    ASSERT(comment);

    items_parsed = sscanf(comment, " DESCRIPTION %[^\n]", description);

    if (items_parsed != 1)
    {
	ErrorMessage("Invalid DESCRIPTION comment at %s:%d", 
					Network::ParseState::parse_file, yylineno);
	Network::ParseState::error_occurred = TRUE;
	return;
    }

    this->setDescription(description, FALSE);
}


/*****************************************************************************/
/* ParseCommentComment -						     */
/*                                                                           */
/* Parses "comment" comment.						     */
/*                                                                           */
/*****************************************************************************/

void Network::netParseCommentComment(const char* comment)
{

    if (Network::ParseState::stop_parsing)
	return;

    ASSERT(comment);

    /*
     * Save network program comments.
     */
    if (this->comment == NUL(char*))
    {
	this->comment = DuplicateString(comment + STRLEN(" comment: "));
    }
    else
    {
	this->comment =
	    (char*)REALLOC
		(this->comment,
		 (2 + STRLEN(this->comment) + STRLEN(comment) -
		 STRLEN(" comment: ")) * sizeof(char));
	strcat(this->comment, "\n");
	strcat(this->comment, comment + STRLEN(" comment: "));
    }
    this->setNetworkComment(this->comment);

}

/*****************************************************************************/
/* ParseNodeComment -						             */
/*                                                                           */
/* Parses "node" comment.						     */
/*                                                                           */
/*****************************************************************************/

void Network::netParseNodeComment(const char* comment)
{
    int        items_parsed;
    int        instance;
    char       name[1024];
    
    ASSERT(comment);

    /*
     * Ignore comments that we do not recognize
     */
    if (strncmp(" node ",comment,6))
        return;

    if (this->parseState.node != NULL)
	this->addNode(this->parseState.node);

    this->parseState.node = NULL;
    
    this->parseState.node_error_occurred = FALSE;

    items_parsed = sscanf(comment, " node %[^[][%d]:", name, &instance);
    if (items_parsed != 2) {
	ErrorMessage("Can't parse 'node' comment (file %s, line %d)",
					Network::ParseState::parse_file, yylineno);
	this->parseState.node_error_occurred = TRUE;
	Network::ParseState::error_occurred = TRUE;
	return;
    }
    NodeDefinition *nd = 
	(NodeDefinition*)theNodeDefinitionDictionary->findDefinition(name);

    if (nd == NULL)
    {
	Network::ParseState::undefined_modules->addDefinition(name, NULL);
	this->parseState.node_error_occurred = TRUE;
	Network::ParseState::error_occurred = TRUE;
	return;
    }
    this->parseState.node = nd->createNewNode(this, instance);
    if (this->parseState.node == NULL)
    {
        Network::ParseState::error_occurred      = TRUE;
	this->parseState.node_error_occurred = TRUE;
	return;
    }
    // 
    //  Determine if the node definition has changed.
    // 
    const char *p = strstr(comment,"inputs =");
    if (p) {
	p += STRLEN("inputs =");
	int inputs = atoi(p);
	if (nd->isUserTool() &&
	    !nd->isInputRepeatable() && (inputs != nd->getInputCount())) {
	    const char *name = nd->getNameString();
#if 0	// Only warn about changes in user modules now 5/18/94.
	    //
	    // Receivers are the only nodes that have 'inputs = 0' (in
	    // old nets) but really do have a single input.
	    //
	    if (!((inputs == 0) && EqualString(name,"Receiver")))
#endif
		Network::ParseState::redefined_modules->addDefinition(name, NULL);
	}
    }
    p = strstr(comment,"outputs =");
    if (p) {
	p += STRLEN("outputs =");
	int outputs = atoi(p);
	if (nd->isUserTool() && 
	    !nd->isOutputRepeatable() && (outputs != nd->getOutputCount())) {
	    const char *name = nd->getNameString();
	    Network::ParseState::redefined_modules->addDefinition(name, NULL);
	}
    }

    Symbol sym = nd->getNameSymbol();
    if ((sym == NDAllocatorDictionary::GetNodeNameSymbol) || 
        (sym == NDAllocatorDictionary::SetNodeNameSymbol))
	this->fileHadGetOrSetNodes = TRUE;
  

    this->parseState.input_index = 0;
    this->parseState.output_index = 0;

    /*
     * 'node' comment parsed successfully:  set the parse state.
     */
    this->parseState.parse_state = _PARSED_NODE;

    //
    // Now ask the node to parse the rest of the information.
    //
    if (!this->parseState.node->netParseComment(comment,
				Network::ParseState::parse_file, yylineno)){
        WarningMessage("Unrecognized comment at line %d of file %s"
                        " (ignoring)", yylineno, Network::ParseState::parse_file);

    }
}
/*****************************************************************************/
/* ParseNodeComment -						             */
/*                                                                           */
/* Parses "node" comment.						     */
/*                                                                           */
/*****************************************************************************/

void Network::cfgParseNodeComment(const char* comment)
{
    int        items_parsed;
    int        instance;
    char       name[1024];
    
    ASSERT(comment);

    /*
     * Ignore comments that we do not recognize
     */
    if (strncmp(" node",comment,5))
        return;

    this->parseState.node = NULL;
    this->parseState.node_error_occurred = FALSE;

    items_parsed = sscanf(comment, " node %[^[][%d]:", name, &instance);
    if (items_parsed != 2) {
	ErrorMessage("Can't parse 'node' comment (file %s, line %d)",
					Network::ParseState::parse_file, yylineno);
	this->parseState.node_error_occurred = TRUE;
	Network::ParseState::error_occurred = TRUE;
	return;
    }
    NodeDefinition *nd = 
	(NodeDefinition*)theNodeDefinitionDictionary->findDefinition(name);

    if (nd == NULL)
    {
	Network::ParseState::undefined_modules->addDefinition(name, NULL);
	this->parseState.node_error_occurred = TRUE;
	Network::ParseState::error_occurred = TRUE;
	return;
    }
    Symbol namesym = theSymbolManager->getSymbol(name);
    this->parseState.node = this->findNode(namesym, instance);
    if (this->parseState.node == NULL)
    {
	WarningMessage("The '%s' tool is referenced at line %d in file %s,\n"
			"but is not found in the program.",
			name,yylineno,Network::ParseState::parse_file);
        Network::ParseState::error_occurred      = TRUE;
	this->parseState.node_error_occurred = TRUE;
	return;
    }

    /*
     * 'node' comment parsed successfully:  set the parse state.
     */
    this->parseState.parse_state = _PARSED_NODE;

    //
    // Now ask the node to parse the rest of the information.
    //
    if (!this->parseState.node->cfgParseComment(comment,
			Network::ParseState::parse_file, yylineno)) {
        WarningMessage("Unrecognized comment at line %d of file %s"
                              " (ignoring)",  yylineno, Network::ParseState::parse_file);
    }
}


//
// Parse all comments out of the .net file
//
boolean Network::netParseComments(const char *comment, const char *filename,
					int lineno)
{

    /*
     * if .net node comment...
     * This signifies the beginning of the node comments
     */
    if (EqualSubstring(comment, " node", 5))
    {
	Network::netParseNodeComment(comment);
    }
    else if (EqualSubstring(comment, " version", 8))
    {
	Network::netParseVersionComment(comment);
    }
    /*
     * If .net MODULE comment...
     */
    else if (EqualSubstring(comment, " MODULE", 7))
    {
	Network::netParseMODULEComment(comment);
    }
    /*
     * If .net CATEGORY comment...
     */
    else if (EqualSubstring(comment, " CATEGORY", 9))
    {
	Network::netParseCATEGORYComment(comment);
    }
    /*
     * If .net pgroup comment...
     */
#if WORKSPACE_PAGES
    else if (EqualSubstring(comment, " pgroup assignment", 18))
    {
	Dictionary *dict = this->groupManagers;
	GroupManager *gmgr = (GroupManager*)dict->findDefinition(PROCESS_GROUP);
	if (gmgr)
	    gmgr->parseComment(comment, Network::ParseState::parse_file, yylineno, this);
    }
    /*
     * If .net page group comment...
     */
    else if (EqualSubstring(comment, " page assignment", 16))
    {
	Dictionary *dict = this->groupManagers;
	GroupManager *gmgr = (GroupManager*)dict->findDefinition(PAGE_GROUP);
	gmgr->parseComment(comment, Network::ParseState::parse_file, yylineno, this);
    }
#else
    else if (EqualSubstring(comment, " pgroup assignment", 18))
    {
	theDXApplication->PGManager->parseComment(comment, 
			Network::ParseState::parse_file, yylineno, this);	
    }
#endif
    /*
     * If .net DESCRIPTION comment...
     */
    else if (EqualSubstring(comment, " DESCRIPTION", 12))
    {
	Network::netParseDESCRIPTIONComment(comment);
    }
    /*
     * If .net comment comment...
     */
    else if (EqualSubstring(comment, " comment: ", 10))
    {
	this->netParseCommentComment(comment);
    }
    /*
     * If .net comment comment...
     */
    else if (EqualSubstring(comment, " macro ", 7))
    {
	this->netParseMacroComment(comment);
    }
    /*
     * If .net network: end of macro body comment...
     */
    else if (EqualString(comment, " network: end of macro body"))
    {
	this->parseState.main_macro_parsed = TRUE;
	this->parseState.parse_state = _PARSED_NONE;
    }
    /*
     * If decorator comment...
     * FIXME: must handle stateful resource comments, but this won't work.
     * It will do for now, only because only decorators have resource comments.
     */
    else if ((EqualSubstring(comment, " decorator", 10)) ||
	     (EqualSubstring(comment, " annotation", 11)) ||
	     (EqualSubstring(comment, " resource", 9)))
    {
	if (!this->parseDecoratorComment(comment, 
				Network::ParseState::parse_file, yylineno)) {
	    Network::ParseState::error_occurred = TRUE;
	} else {
#if WORKSPACE_PAGES
	    this->parseState.parse_sub_state = _SUB_PARSED_DECORATOR;
#endif
	}
    }
#if WORKSPACE_PAGES
    else if ((strstr(comment, " group:")) &&
	       (this->parseState.parse_sub_state == _SUB_PARSED_DECORATOR)) 
    {
	if (!this->parseDecoratorComment(comment, 
				Network::ParseState::parse_file, yylineno)) {
	    Network::ParseState::error_occurred = TRUE;
	}
    }
    else if ((this->parseState.parse_state == _PARSED_NODE) && 
	     (this->parseState.parse_sub_state == _SUB_PARSED_NODE) &&
	     (strstr(comment, " group:")) &&
	     (this->parseState.node_error_occurred == FALSE)) {
	if (!this->parseState.node->netParseComment(comment,
				Network::ParseState::parse_file, yylineno)) {
	    WarningMessage("Unrecognized comment at line %d of file %s"
			   " (ignoring)",
			lineno,filename);
	}
    }
#endif
    /*
     * If .net workspace comment...
     */
	else if (EqualSubstring(comment, " workspace", 10)) 
	{
		if (!this->workSpaceInfo.parseComment(comment, 
			Network::ParseState::parse_file, yylineno)) {
				Network::ParseState::error_occurred      = TRUE;
		}
		this->parseState.parse_state = _PARSED_WORKSPACE;
	}
	else if (this->parseState.parse_state == _PARSED_NODE && 
		this->parseState.node_error_occurred == FALSE) {
			if (!this->parseState.node->netParseComment(comment,
				Network::ParseState::parse_file, yylineno)) {
					WarningMessage("Unrecognized comment at line %d of file %s"
						" (ignoring)",
						lineno,filename);
			} else {
#if WORKSPACE_PAGES
				this->parseState.parse_sub_state = _SUB_PARSED_NODE;
#endif
			}
	} 
	else if (this->parseState.parse_state == _PARSED_WORKSPACE) 
	{
		if (!this->workSpaceInfo.parseComment(comment, 
			Network::ParseState::parse_file, yylineno)) {
				WarningMessage("Unrecognized comment at line %d of file %s"
					" (ignoring)",
					lineno,filename);
		}
	}
	else if (this->parseState.parse_state == _PARSED_NODE) {
		WarningMessage("Unrecognized comment at line %d of file %s (ignoring)",
			lineno,filename);
		//this->parseState.node_error_occurred = TRUE;
		//this->parseState.error_occurred      = TRUE;
	}

    return (Network::ParseState::error_occurred == FALSE);

}

//
// Parse all comments out of the .cfg file
//
boolean Network::cfgParseComments(const char *comment, const char *filename,
					int lineno)
{
    
    /*
     * if a .cfg interactor comment...
     * This signifies the beginning of the interactor comments.
     */
    if (EqualSubstring(comment, " interactor", 11))
    {
	this->cfgParseInteractorComment(comment);
    }
    /*
     * if a .cfg panel comment (this signifies the begining of the panel
     * comments)
     */
    else if (EqualSubstring(comment, " panel[", 7))
    {
	this->cfgParsePanelComment(comment);
    }
    /*
     * If version comment(in the .cfg file)...
     */
    else if (EqualSubstring(comment, " version", 8))
    {
	this->cfgParseVersionComment(comment);
    }
    /*
     * If vcr comment(in the .cfg file)...
     * In version 3, we started using the '// node Sequencer[%d]:' comments
     */
    else if ((this->getNetMajorVersion() < 3) && 
		EqualSubstring(comment, " vcr", 4))
    {
#if 00
	Node *vcr= this->findNode(
				theSymbolManager->getSymbol("Sequencer"),0);
#else
	Node *vcr= this->sequencer;
	ASSERT(vcr);
#endif
	if (vcr && vcr->cfgParseComment(comment,filename, lineno))
	    this->parseState.parse_state 	 = _PARSED_VCR;
    	else
	    this->parseState.error_occurred      = TRUE;
    }
    else if (EqualSubstring(comment, " node", 5))
    {
	this->cfgParseNodeComment(comment);
    }
    else if ((this->parseState.parse_state == _PARSED_NODE) || 
	     (this->parseState.parse_state == _PARSED_INTERACTOR))
    {
	if ((this->parseState.node_error_occurred == FALSE) &&
	    !this->parseState.node->cfgParseComment(comment,filename, lineno)) {
	    WarningMessage("Unrecognized comment at line %d of file %s"
			   " (ignoring)", lineno,filename);
	}
    } 
    else if (this->parseState.parse_state == _PARSED_PANEL) 
    {
	ASSERT(this->parseState.control_panel);
	if (!this->parseState.control_panel->cfgParseComment(comment,
							filename, lineno)) {
	    WarningMessage("Unrecognized comment at line %d of file %s"
			   " (ignoring)", lineno,filename);
	}
    } 
    else if (this->parseState.parse_state == _PARSING_NETWORK_CFG) 
    {
	if (!this->cfgParseComment(comment,filename,lineno)) {
	    WarningMessage("Unrecognized comment at line %d of file %s"
			   " (ignoring)", lineno,filename);
	}
    }
    /*
     * The 'time:' comment (in the .cfg file) signifies 
     * the beginning of the application's cfg comments.
     */
    else if (EqualSubstring(comment, " time:", 6))
    {
	 this->parseState.parse_state = _PARSING_NETWORK_CFG;
    }

    return (this->parseState.error_occurred == FALSE);
 
}
/*****************************************************************************/
/* uinParseComment -							     */
/*                                                                           */
/* Parses comment.							     */
/*                                                                           */
/*****************************************************************************/

extern "C"
void ParseComment(char* comment)
{
    ASSERT(Network::ParseState::network);
    Network::ParseState::network->parseComment(comment);
}

void Network::parseComment(char* comment)
{
    ASSERT(comment);

    /*
     * Suppress further processing if first macro has already been
     * parsed.
     */
    if (this->parseState.main_macro_parsed)
	    return;

    /*
     * If blank comment...
     */
    if (IsBlankString(comment))
    {
	return;
    }

    /*
     * Handle either a .cfg or a .net comment 
     */
    if (this->parseState.parse_mode == _PARSE_NETWORK)
	this->netParseComments(comment, 
				this->parseState.parse_file, yylineno);
    else if (this->parseState.parse_mode == _PARSE_CONFIGURATION)
	this->cfgParseComments(comment, 
				this->parseState.parse_file, yylineno);
}

/*****************************************************************************/
/* uinParseFunctionID -							     */
/*                                                                           */
/* Parses function name.						     */
/*                                                                           */
/*****************************************************************************/

extern "C"
void ParseFunctionID(char* name)
{
    ASSERT(Network::ParseState::network);
    Network::ParseState::network->parseFunctionID(name);
}
void Network::parseFunctionID(char *name)
{

    if (this->parseState.stop_parsing)
	return;


    /*
     * Suppress further processing if first macro has already been
     * parsed.
     */
    if (this->parseState.main_macro_parsed)
	return;

    ASSERT(name);
#if 000
    strcpy(_current_module,name);
#endif
    this->parseState.output_index = 0;
}


/*****************************************************************************/
/* uinParseArgument -							     */
/*                                                                           */
/* Parses function arguments, i.e., uses arguement variable names to	     */
/* generate connections (arcs) between nodes.				     */
/*                                                                           */
/*****************************************************************************/

extern "C"
void ParseArgument(char* name, const boolean isVarname)
{
    ASSERT(Network::ParseState::network);
    Network::ParseState::network->parseArgument(name,isVarname);
}
void Network::parseArgument(char* name, const boolean isVarname)
{
    int        parsed_items;
    int        instance;
    int        parameter;
    char       type[128];
    char       module[512];
    boolean parse_error = FALSE;
    char macro[250];

    if (this->parseState.stop_parsing || this->parseState.main_macro_parsed)
	return;

    ASSERT(name);
    /*
     * If the destination node has not yet been specified....
     */
    if (this->parseState.node == NULL)
	return;

    /*
     * Increment the parameter number and save it as the parameter index
     * of the destination node.
     */
    this->parseState.input_index++;


    if (!isVarname)
	return;

    /*
     * Scan the name for components.
     */
    parsed_items =
	sscanf(name,
	       "%[^_]_%[^_]_%d_%[^_]_%d",
	       macro,
	       module,
	       &instance,
	       type,
	       &parameter);

    if (parsed_items != 5) {
	parsed_items =
	    sscanf(name,
		   "%[^_]_%d_%[^_]_%d",
		   module,
		   &instance,
		   type,
		   &parameter);
	/*
	 * If parse failed...
	 */
	if (parsed_items != 4)
	    parse_error = TRUE;

    } else if (!EqualString(macro, this->getNameString())) {
	parse_error = TRUE;
    } 

    if (parse_error && !EqualString("NULL", name)) {
	ErrorMessage( "Error parsing tool input %s (in %s, line %d)",
	    name, this->parseState.parse_file, yylineno);
	this->parseState.error_occurred = TRUE;
	return;
    }

    /*
     * if not "out" or "in"...
     */
    if (NOT EqualString(type, "out"))
    {
	if (NOT EqualString(type, "in"))
	{
	    ErrorMessage( "Error parsing tool input %s (in %s, line %d)",
		name, this->parseState.parse_file, yylineno);
	    this->parseState.error_occurred = TRUE;
	}
#if 000	// Start of changes for expression into Compute 2/23/95
	//
	// If a tool implements itself with a series of module calls, 
	// don't bump the index when variable name does not match the current
	// module.  This input will  not be connected to anything, but throws
	// off the other input connections if we don't decrement the index.
	// For example, don't bump the index on 'Compute_1_in_5' in
	// the following: 
	//
	//  Compute_1_out_1 = List(Compute_1_in_5,foo_1_out_1);
	//  Compute_1_out_1 = Compute(Compute_1_out_1,bar_1_out_1);
	//
	if (!EqualString(module,_current_module)) 
	    this->parseState.input_index--;
#endif
	return;
    }


    //
    // If this node does not have enough inputs, and they were not added 
    // earlier because this is a .net without the number of inputs in the 
    // comment, then add a set of input repeats.
    //
    if (this->parseState.input_index > 
				this->parseState.node->getInputCount()) {	
	if (this->parseState.node->isInputRepeatable() && 
				this->getNetMajorVersion() == 0) {
	    this->parseState.node->addInputRepeats();
	} else {
	    ErrorMessage(
		"Node %s had unexpected number of inputs, file %s, line %d",
		this->parseState.node->getNameString(),
					this->parseState.parse_file, yylineno);
	    this->parseState.error_occurred = TRUE;
	    return;
	}
    }

    // The following is for 'out' input parameters ONLY!

    Symbol s_of_out = theSymbolManager->registerSymbol(module);
    // 
    //  Skip any nodes that are 'connected to themselves' such as
    //  the Colormap module which does something like the following... 
    //    Colormap_1_out_1 = f(a);
    //    Colormap_1_out_1 = g(Colormap_1_out_1);
    //		....
    //  If the names and instances are the same then we assume we have
    //  the above situation.
    // 
    if ((this->parseState.node->getNameSymbol() == s_of_out) &&
	(this->parseState.node->getInstanceNumber() == instance))
	return;

    Node *n;
    boolean found = FALSE;
#if 00
    ListIterator l;
    FOR_EACH_NETWORK_NODE(this, n, l)
    {
	if (n->getNameSymbol() == s_of_out && 
	    n->getInstanceNumber() == instance)
#else
	if ( (n = this->findNode(s_of_out, instance)) )
#endif
	{
	    if (n->getOutputCount() < parameter)
		ErrorMessage("Cannot connect output %d of %s to %s, "
			     "too many outputs (in %s, line %d)", 
			 parameter, module, 
			 this->parseState.node->getNameString(),
			 this->parseState.parse_file, yylineno);
	    else if (this->parseState.node->getInputCount() < 
			    this->parseState.input_index)
		ErrorMessage("Cannot connect input %d of %s to %s, "
			     "too many inputs (in %s, line %d)", 
			 this->parseState.input_index, 
			 this->parseState.node->getNameString(),
			 module,
			 this->parseState.parse_file, yylineno);
	    else
	    {
		// Connect output parameter to input of this->parseState.node 
		new Ark(n, parameter, this->parseState.node, 
				this->parseState.input_index);
		this->parseState.node->setInputVisibility(
			this->parseState.input_index,TRUE);
		n->setOutputVisibility(parameter,TRUE);
	    }
	    found = TRUE;
#if 00
	    break;
	}
#endif
    }
    if (!found && !this->parseState.ignoreUndefinedModules)
    {
	this->parseState.undefined_modules->addDefinition(module, NULL);
	this->parseState.error_occurred = TRUE;
	return;
    }
}


/*****************************************************************************/
/* uinParseLValue -							     */
/*                                                                           */
/*                                                                           */
/*****************************************************************************/

extern "C"
void ParseLValue(char* name)
{
    ASSERT(Network::ParseState::network);
    Network::ParseState::network->parseLValue(name);
}

void Network::parseLValue(char* name)
{
    if (this->parseState.stop_parsing)
	return;

    /*
     * Suppress further processing if first macro has already been
     * parsed.
     */
    if (this->parseState.main_macro_parsed)
	return;

    ASSERT(name);

    //
    // Only bump the output index when given an output parameter name.
    //
    if (strstr(name,"_out_"))
        ++this->parseState.output_index;

}

extern "C"
void ParseStringAttribute(char* name, char* value)
{
    ASSERT(Network::ParseState::network);
    Network::ParseState::network->parseStringAttribute(name,value);
}

void Network::parseStringAttribute(char* name, char *value)
{
    if (this->parseState.stop_parsing)
	return;

    if(NOT EqualString(name, "pgroup"))
	return;

#if WORKSPACE_PAGES
    this->parseState.node->addToGroup(value, theSymbolManager->getSymbol(PROCESS_GROUP));
#else
    this->parseState.node->addToGroup(value);
#endif
}

extern "C"
void ParseIntAttribute(char* name, int value)
{
    ASSERT(Network::ParseState::network);
    Network::ParseState::network->parseIntAttribute(name, value);
}

void Network::parseIntAttribute(char* name, int value)
{
    if (!this->parseState.node || this->parseState.stop_parsing)
	return;
    //
    // Determine if this should be associated with an output
    if (this->parseState.node != NULL && this->parseState.output_index > 0)
    {
	if (EqualString("cache", name))
	{
	    switch (value) {
	    case 0:
		this->parseState.node->setOutputCacheability(
				this->parseState.output_index, 
				ModuleNotCached);
		break;
	    case 1:
		this->parseState.node->setOutputCacheability(
				this->parseState.output_index, 
				ModuleFullyCached);
		break;
	    case 2:
		this->parseState.node->setOutputCacheability(
				this->parseState.output_index, 
				ModuleCacheOnce);
		break;
	    }
	}
    }
    //
    // associate with the node
    else if ((this->parseState.node != NULL) && 
	     (this->parseState.parse_state == _PARSED_NODE))
    {
	if (EqualString("cache", name))
	{
	    switch (value) {
	    case 0:
		this->parseState.node->setNodeCacheability( ModuleNotCached);
		break;
	    case 1:
		this->parseState.node->setNodeCacheability( ModuleFullyCached);
		break;
	    case 2:
		this->parseState.node->setNodeCacheability( ModuleCacheOnce);
		break;
	    }
	}
    }
}


/*****************************************************************************/
/* uinParseRValue -							     */
/*                                                                           */
/*                                                                           */
/*****************************************************************************/

extern "C"
void ParseRValue(char* name)
{
    ASSERT(Network::ParseState::network);
    Network::ParseState::network->parseRValue(name);
}

void Network::parseRValue(char* name)
{
    int        parsed_items;
    int        instance;
    int        parameter;
    char       type[128];
    char       string[256];
    char       macro[256];
    boolean	parse_error = FALSE;

    if (!this->parseState.node || this->parseState.stop_parsing)
	return;

    ASSERT(name);

    /*
     * Suppress further processing if first macro has already been
     * parsed.
     */
    if (this->parseState.main_macro_parsed)
	return;

// cout << "ParseRValue: " << name << "\n";
// cout.flush();


    /*
     * Scan the name for components.
     */
    parsed_items =
	sscanf(name,
	       "%[^_]_%[^_]_%d_%[^_]_%d",
	       macro,
	       string,
	       &instance,
	       type,
	       &parameter);

    if (parsed_items != 5)  {
	/*
	 * If parse failed...try the old (pre 2.0) style
	 */
	parsed_items =
	    sscanf(name,
		   "%[^_]_%d_%[^_]_%d",
		   string,
		   &instance,
		   type,
		   &parameter);

	if (parsed_items != 4)
	    parse_error = TRUE;
    } else if (!EqualString(macro, this->getNameString())) {
	parse_error = TRUE;
    }

    if (parse_error || !EqualString(type, "out"))
	return;

    Node *n;
    boolean found = FALSE;
    Symbol s_of_out = theSymbolManager->registerSymbol(string);
#if 00
    ListIterator l;
    FOR_EACH_NETWORK_NODE(this, n, l)
    {
	if (n->getNameSymbol() == s_of_out && 
	    n->getInstanceNumber() == instance)
#else
	if ( (n = this->findNode(s_of_out, instance)) )
#endif
	{
	    if (n->getOutputCount() < parameter)
		ErrorMessage("Cannot connect output %d of %s to %s, "
			  "too many outputs (in %s, line %d)", 
			  parameter, string, 
			  this->parseState.node->getNameString(),
			  this->parseState.parse_file, yylineno);
	    else if (this->parseState.node->getInputCount() < 1)
		ErrorMessage("Cannot connect input %d of %s to %s, "
			  "too many inputs (in %s, line %d)", 
			  this->parseState.input_index, 
			  this->parseState.node->getNameString(),
			  string,
			  this->parseState.parse_file, yylineno);
	    else
	    {
		// Connect output parameter to input of this->parseState.node
		new Ark(n, parameter, 
			this->parseState.node, 1);
		this->parseState.node->setInputVisibility(1,
						TRUE);
		n->setOutputVisibility(parameter,TRUE);
	    }
	    found = TRUE;
#if 00
	    break;
	}
#endif
    }
    if (!found && !this->parseState.ignoreUndefinedModules)
    {
	this->parseState.undefined_modules->addDefinition(string, NULL);
	this->parseState.error_occurred = TRUE;
	return;
    }
}


/*****************************************************************************/
/* uinParseEndOfMacroDefinition -					     */
/*                                                                           */
/*                                                                           */
/*****************************************************************************/

extern "C"
void ParseEndOfMacroDefinition()
{
    ASSERT(Network::ParseState::network);
    Network::ParseState::network->parseEndOfMacroDefinition();
}

void Network::parseEndOfMacroDefinition()
{
    if (this->parseState.stop_parsing)
	return;
    this->parseState.main_macro_parsed = TRUE;
}


/*****************************************************************************/
/* yyerror -								     */
/*                                                                           */
/* Parser error routine.                                                     */
/*                                                                           */
/*****************************************************************************/

extern "C"
void yyerror(char* , ...)
{
    ErrorMessage("Syntax error encountered (%s file, line %d).",
	    Network::ParseState::network->parseState.parse_file, yylineno);
    Network::ParseState::network->parseState.error_occurred = TRUE;
}

//
// Static method that will...
// Build a network (.net) filename (handling semantics of file extension) 
// from the given name.  If the given name does not have the required 
// extension, then add one.  
// 
// A newly allocated string is always returned containing the appropriate name.
//
char *Network::FilenameToNetname(const char *filename)
{
    const char *ext;
    char *file;
    int flen = STRLEN(filename);
    int len = STRLEN(NetExtension);

    file = new char[flen + len + 1];
    strcpy(file, filename);

    // 
    // Build/add the extension
    // 
#ifdef DXD_OS_NON_UNIX    //SMH cover upper case extensions
    ext = strrstr(file, (char *)NetExtensionUpper);
    if (ext) return file;
#endif
    ext = strrstr(file, (char *)NetExtension);
    if (!ext || (STRLEN(ext) != len))
	strcat(file,NetExtension);

    ASSERT((flen + len + 1) > strlen(file));
    return file;
}
//
// Static method that will...
// Build a config (.cfg) filename (handling semantics of file extension) 
// from the given name.  If the given name does not have the required 
// extension, then add one.  
// 
// A newly allocated string is always returned containing the appropriate name.
//
char *Network::FilenameToCfgname(const char *filename)
{
    const char *ext;
    char *file;
    int len = STRLEN(CfgExtension);

    file = new char[STRLEN(filename) + len + 1];
    strcpy(file, filename);

    // 
    // Build/add the extension
    // 
#ifdef DXD_OS_NON_UNIX    //SMH cover upper case extensions
    ext = strrstr(file, (char *)CfgExtensionUpper);
    if (ext) return file;
#endif
    
#if 0
    //
    // Remove the .net extension if present
    // FIXME
    // This code was placed here ifdef-ed out.  We want the code in, but it's left
    // out only because of nearness of release date.  When this chunk is enabled,
    // go to SaveCFGDialog.C and remove what should look like a dup of this chunk.
    //
    const char *netext = strrstr(file, (char *)NetExtension);
    int netExtLen = STRLEN(NetExtension);
    if ((netext) && (STRLEN(netext) == netExtLen)) {
	int filelen = STRLEN(file);
	file[filelen-netExtLen] = '\0';
    }
#endif

    ext = strrstr(file, (char *)CfgExtension);
    if (!ext || (STRLEN(ext) != len)) {
	strcat(file,CfgExtension);
    }

    return file;
}

//
// Determine if the network is in a state that can be saved to disk.
// If so, then return TRUE, otherwise return FALSE and issue an error dialog. 
//
boolean Network::isNetworkSavable()
{
    if (this->isMacro() && EqualString(this->getNameString(),DEFAULT_MAIN_NAME))
    {
	ErrorMessage("Macros must have a name other than '%s'.\n"
		     "To change the name, use the Macro Name option\n"
		     "available in the Edit menu.",
					DEFAULT_MAIN_NAME);
	return FALSE;
    }
    return TRUE;
}


boolean Network::saveNetwork(const char *name, boolean force)
{
    char *ext, *buf;
 

    if (!this->isNetworkSavable() && !force)
	return FALSE;

    buf = DuplicateString(name);
    ext = (char*)strrstr(buf,(char *)NetExtension);
    if (!ext)
	ext = (char*)strrstr(buf,(char *)CfgExtension);
    if (ext)
	*ext = '\0'; 

    char *fname = this->fileName;	
    char *fullName = new char[STRLEN(buf) + STRLEN(NetExtension) + 1];
    strcpy(fullName, buf);
    strcat(fullName, NetExtension);

    //
    // Version checking will return control here after the user has responded
    // to a question dialog if it's necessary to ask her a question.
    // If fullName doesn't exist there can't be a version mismatch.
    //
    if (!this->versionMismatchQuery (TRUE, fullName))
	return FALSE;

    this->fileName = fullName;

    //
    // Change MacroDefinition's file name.
    //
    MacroDefinition *md = this->getDefinition();
    if (md)
        md->setFileName(fullName);

    boolean ret =  this->netPrintNetwork(buf) &&
	           this->cfgPrintNetwork(buf) &&
	           this->auxPrintNetwork();

    if (theDXApplication->dxlAppNotifySaveNet())
    {
        ApplicIF *applic = theDXApplication->getAppConnection();
        if (applic)
        {
            char *buf = new char [strlen("savednet ") +
                                        strlen(this->fileName) + 1];
            strcpy(buf, "savednet ");
            strcat(buf, this->fileName);

            applic->send(PacketIF::SYSTEM, buf);
        }
    }

    if (ret)
    {
	if (fname)
	    delete fname;
	//
	// Activate these now that the user has give the net a name.
	//
	if (theDXApplication->appAllowsSavingNetFile(this)) 
	    this->saveCmd->activate();
	if (theDXApplication->appAllowsSavingCfgFile()) 
	    if (this->saveCfgCmd) this->saveCfgCmd->activate();
	if (this->openCfgCmd) this->openCfgCmd->activate();
        this->resetWindowTitles();
        this->clearFileDirty();

	//
	// Change the version info in the current net now that saving has
	// succeeded.
	//
	this->dxVersion.major = DX_MAJOR_VERSION;
	this->dxVersion.minor = DX_MINOR_VERSION;
	this->dxVersion.micro = DX_MICRO_VERSION;
	this->netVersion.major = NETFILE_MAJOR_VERSION;;
	this->netVersion.minor = NETFILE_MINOR_VERSION;;
	this->netVersion.micro = NETFILE_MICRO_VERSION;;

	//
	// The editor may have something to do if the network is clean.
	// Clearing the undo stack, perhaps?
	//
	if (this->editor) {
	    this->editor->notifySaved();
	}
	theDXApplication->appendReferencedFile(this->fileName);
    }
    else
    {
	delete fullName;
	this->fileName = fname;
    }


    delete buf;

     
    return ret;
}

boolean Network::openCfgFile(const char *name, 
				boolean openStartup, boolean send)
{
    char *cfgfile;
    boolean ret;
    FILE *f;

    if (this->isMacro())
	return TRUE;
    
    cfgfile = Network::FilenameToCfgname(name);

    f = fopen (cfgfile, "r");
    if (f != NULL)  {
    	this->clearCfgInformation();
#ifdef NoParseState
        _parse_mode = _PARSE_CONFIGURATION;
        _parse_file = cfgfile;
#else
	this->parseState.initializeForParse(this,_PARSE_CONFIGURATION,cfgfile);
#endif
        this->readingNetwork = TRUE;
        boolean stopped_parsing = this->parse(f);
	if (openStartup)
	    this->openControlPanel(-1);
	if (send)
	    this->sendValues(FALSE);
	ret = !stopped_parsing;	
        this->readingNetwork = FALSE;
#ifdef NoParseState
#else
	this->parseState.cleanupAfterParse();
#endif
    }
    else {
	ret = FALSE;	
    }

    if (f) fclose(f);
    delete cfgfile;

    return ret;
}


boolean Network::saveCfgFile(const char *name)
{
    return this->cfgPrintNetwork(name);

}

//
// Print the .net portion of a network to a file with the given name.
// The extension used is NetExtension, and will be added if not present in
// the filename.
//
boolean Network::netPrintNetwork(const char *filename)
{
    char *file, *tmpfile, *netfile;
    FILE *f;
    boolean ret;

    netfile = Network::FilenameToNetname(filename);
#if HAS_RENAME
    file = tmpfile = UniqueFilename(netfile);
    if (!file) {
	file = netfile;
# define WRITE_PERM_FIX 1
# ifdef  WRITE_PERM_FIX 
    } else {
	//
	// Make sure we can open the netfile for write.  rename() seems to
 	// allow us to overright 444 files if the directory is writable.
	// FIXME: should we do this .cm files?
	//
#ifdef DXD_WIN
	f = fopen(netfile,"a+");
#else
	f = fopen(netfile,"a");
#endif
	if (!f) {
	    ErrorMessage("Can not open file %s for writing", netfile); 
	    ret = FALSE;
	    goto out;
	}
	fclose(f);
#if defined(OS2) || defined(DXD_WIN)  // otherwise rename later would fail since file exists
        unlink(netfile);
#endif
# endif // WRITE_PERM_FIX
    }
#else
    tmpfile = NULL;
    file = netfile;
#endif
 
    f = fopen (file, "w");

    if (f == NULL)  {
        ErrorMessage("Can not open file %s for writing", netfile); 
	ret = FALSE;
    } else if (this->printNetwork(f, PrintFile) != TRUE) {
        ErrorMessage("Error writing file %s",netfile);
        fclose(f);
	unlink(file);
	ret = FALSE;
    } else  { 
        fclose(f);
	ret = TRUE;
#if HAS_RENAME
	if (tmpfile) {
	    rename(tmpfile,netfile);
	}
#endif	// HAS_RENAME
    }
    
#if HAS_RENAME
out:
#endif
    if (netfile) delete netfile;
    if (tmpfile) delete tmpfile;

    return ret;
}
//
// Print the .cfg portion of a network to a file with the given name.
// The extension used is found in CfgExtension, and will be added if not 
// present in the filename.
//
boolean Network::cfgPrintNetwork(const char *filename, PrintType dest)
{
    char *file, *tmpfile, *cfgfile; 
    FILE *f = NULL;
    boolean ret, rmfile = TRUE;

    cfgfile = Network::FilenameToCfgname(filename);

    if (this->isMacro()) {
	unlink(cfgfile);
	tmpfile = NULL;
	ret = TRUE;
	goto out;
    }

#if HAS_RENAME
    file = tmpfile = UniqueFilename(cfgfile);
    if (!file) {
	file = cfgfile;
# define WRITE_PERM_FIX 1
# ifdef  WRITE_PERM_FIX
    } else {
        //
        // Make sure we can open the cfgfile for write.  rename() seems to
        // allow us to overright 444 files if the directory is writable.
        //
#ifdef DXD_WIN
        f = fopen(cfgfile,"a+");
#else
        f = fopen(cfgfile,"a");
#endif
        if (!f) {
            ErrorMessage("Can not open file %s for writing", cfgfile);
            ret = FALSE;
            goto out;
        }
        fclose(f);
#if defined(OS2) || defined(DXD_WIN)                // otherwise rename later would fail since file exists
        unlink(cfgfile);
#endif
# endif // WRITE_PERM_FIX
    }
#else
    tmpfile = NULL;
    file = cfgfile;
#endif	// HAS_RENAME
	
    f = fopen (file, "w");

    ret = TRUE;
    if (f == NULL)  {
        ErrorMessage("Can not open file %s for writing", file); 
	ret = FALSE;
    } else { 
	Node *n;
	ControlPanel *cp;
	ListIterator l;
	time_t t;
	long int fsize;

	t = time((time_t*) 0);		
	
	//
  	// Demarcates the beginning of the network cfg comments.
	//
	if (fprintf(f, "//\n// time: %s//\n", ctime(&t)) < 0) {
	    ret = FALSE;
	    goto write_failed;
	}

	if (fprintf(f, 
#ifdef BETA_VERSION
		"// version: %d.%d.%d (format), %d.%d.%d (DX Beta)\n//\n",
#else
		"// version: %d.%d.%d (format), %d.%d.%d (DX)\n//\n",
#endif
			NETFILE_MAJOR_VERSION,
			NETFILE_MINOR_VERSION,
			NETFILE_MICRO_VERSION,
			DX_MAJOR_VERSION,
			DX_MINOR_VERSION,
			DX_MICRO_VERSION) < 0) {
	    ret = FALSE;
	    goto write_failed;
	}

	//
	// Get the current size of the file. If does not change with next 
	// few operations, then unlink it later.
	//
	fflush(f);
	fsize = ftell(f);

	//
	// Ask the application to print any relevant comments first.
	//
	if(dest == PrintExec || dest == PrintFile) {
	    if (!this->isMacro() && !theDXApplication->printComment(f)) {
		ret = FALSE;
		goto write_failed;
	    }
	}
	//
	// Then the panel group and acess information.
	//
	if (!this->panelGroupManager->cfgPrintComment(f) ||
	    !this->panelAccessManager->cfgPrintInaccessibleComment(f)) {
	    ret = FALSE;
	    goto write_failed;
	}


#define FIX_LLOYDT96 1
#if defined(FIX_LLOYDT96)
	//
	// PrintCut is dnd on nodes in the vpe
	//    I think you want to avoid all cfg info because you don't
	//    want any new panels. ???
	// PrintCPBuffer is dnd within a control panel.
	//    Certainly need whatever cfg info there is, but only on
	//    selected things.
	// (If you drag from a vpe to a panel, then it's not writing a file.)
	//
	if (dest==PrintCut) 
	{
	} 
	else if (dest==PrintCPBuffer)
#else
	if (dest==PrintCPBuffer)
#endif
	{
	    ControlPanel *ownerCp = this->selectionOwner;
	    if (!(ownerCp->cfgPrintPanel(f, PrintCPBuffer))) {
		ret = FALSE;
		goto write_failed;
	    }

	    //
	    // don't really want to loop over all nodes in network.
	    // Really just want all nodes referenced by instanceList
	    // of ownerCp.
	    //
	    FOR_EACH_NETWORK_NODE(this, n, l) {
		if (ownerCp->nodeIsInInstanceList (n)) {
		    if (!n->cfgPrintNode(f, dest)) {
			ret = FALSE;
			goto write_failed;
		    }
		}
	    }
	}
	else
	{
	    FOR_EACH_NETWORK_PANEL(this, cp, l) {
		if (!cp->cfgPrintPanel(f)) {
		    ret = FALSE;
		    goto write_failed;
		}
	    }
	    FOR_EACH_NETWORK_NODE(this, n, l) {
		boolean selected = FALSE;
		if ((n->getStandIn()) && (n->getStandIn()->isSelected()))
		    selected = TRUE;
		if(dest == PrintCut && selected ||
		    (dest == PrintCPBuffer && selected) ||
		    dest == PrintExec || dest == PrintFile) {
		    if (!n->cfgPrintNode(f, dest)) {
			ret = FALSE;
			goto write_failed;
		    }
		}
	    }
	}

	//
	// Check to see if the above wrote anything to the file.
	// If not, then we don't need to keep it.
	//
	fflush(f);
	if (ftell(f) != fsize)
	    rmfile = FALSE;
    }

write_failed:
    if (f) {
        fclose(f);
	if (!ret) {
	    ErrorMessage("Error writing file %s",cfgfile);
	} else {
	    ASSERT(cfgfile);
	    if (rmfile) {
		unlink(cfgfile);
		if (tmpfile)
		    unlink(tmpfile);
	    } 
#if HAS_RENAME
	    else if (tmpfile)
		rename(tmpfile, cfgfile);
#endif	// HAS_RENAME
	}
    }

out:
    if (cfgfile) delete cfgfile;
    if (tmpfile) delete tmpfile;

    return ret;
}
//
// Print any files that are created on a per node basis. 
//
boolean Network::auxPrintNetwork()
{
    boolean ret = TRUE;
    Node *n;
    ListIterator l;

    FOR_EACH_NETWORK_NODE(this, n, l) {	
	if (!n->auxPrintNodeFile()) {
	    // Nodes are expected to indicate any errors that occur.
	    ret = FALSE;
	}
    }

    return ret;
}

boolean Network::printNetwork(FILE *f, PrintType dest)
{
    if (!this->printHeader(f, dest))
	return FALSE;
    if (!this->printBody(f, dest))
	return FALSE;
    if (!this->printTrailer(f, dest))
	return FALSE;
    if (!this->printValues(f, dest))
	return FALSE;

#if WORKSPACE_PAGES
    //
    // Process group info is only stored in a network if this is the main network.
    //
    if((dest == PrintFile)||(dest == PrintExec)) {
	DictionaryIterator di(*this->groupManagers);
	GroupManager *gmgr;
	while ( (gmgr = (GroupManager*)di.getNextDefinition()) ) {
	    if (!gmgr->printAssignment(f))
		return FALSE;
	}
    }
#endif
    if (!this->isMacro())
    {
#if WORKSPACE_PAGES
#else
	if((dest == PrintFile)||(dest == PrintExec))
	    if (!theDXApplication->PGManager->printAssignment(f))
		return FALSE;
#endif
	
	if (! theDXApplication->dxlAppNoNetworkExecute())
	{

	    if (fprintf(f, "Executive(\"product version %d %d %d\");\n" 
			"$sync\n",
			DX_MAJOR_VERSION, 
			DX_MINOR_VERSION, 
			DX_MICRO_VERSION) < 0)
		return FALSE;

	//
	// Don't print the stuff that causes execution of this is a DXLink
	// program, as it may used in DXLLoadVisualProgram() or  
	// exDXLLoadScript() which would cause executions when talking to
	// the executive.
	// FIXME: The Node's should be telling us if this is a DXLink program. 
	//
	    char *comment;
	    if (this->containsClassOfNode(ClassDXLInputNode) ||
		this->containsClassOfNode(ClassDXLOutputNode))  {
		if(fprintf(f,
"// This network contains DXLink tools.  Therefore, the following line(s)\n"
"// that would cause an execution when run in script mode have been \n"
"// commented out.  This will facilitate the use of the DXLink routines\n"
"// exDXLLoadScript() and DXLLoadVisualProgram() when the DXLink\n"
"// application is connected to an executive.\n") < 0)
		    return FALSE;
		comment = "// ";
	    } else
		comment = "";

	    if (this->sequencer)
	    {
		if ((fprintf(f, "%s\nsequence %s();\n", comment,
				this->getNameString()) < 0) ||
		    (fprintf(f, "%splay;\n",comment) < 0))
		    return FALSE;
	    }
	    else if (fprintf(f, "%s%s();\n", comment,this->getNameString()) < 0)
		return FALSE;	
	}
    }

    return TRUE;  	
}

boolean Network::printHeader(FILE *f,
			     PrintType dest,
			     PacketIFCallback echoCallback,
				 void *echoClientData)
{
	DXPacketIF *pif = theDXApplication->getPacketIF();
	char s[1000];
	if (dest == PrintFile || dest == PrintCut || dest == PrintCPBuffer)
	{
		SPRINTF(s, "//\n");
		if (fputs(s, f) < 0) return FALSE;
		if (echoCallback)
			(*echoCallback)(echoClientData, s);
		time_t t = time((time_t*)NULL);
		SPRINTF(s, "// time: %s", ctime(&t));
		if (fputs(s, f) < 0) return FALSE;
		if (echoCallback)
			(*echoCallback)(echoClientData, s);
		SPRINTF(s, "//\n");
		if (fputs(s, f) < 0) return FALSE;
		if (echoCallback)
			(*echoCallback)(echoClientData, s);
		SPRINTF(s, 
#ifdef BETA_VERSION
			"// version: %d.%d.%d (format), %d.%d.%d (DX Beta)\n//\n",
#else
			"// version: %d.%d.%d (format), %d.%d.%d (DX)\n//\n",
#endif
			NETFILE_MAJOR_VERSION,
			NETFILE_MINOR_VERSION,
			NETFILE_MICRO_VERSION,
			DX_MAJOR_VERSION,
			DX_MINOR_VERSION,
			DX_MICRO_VERSION);
		if (fputs(s, f) < 0) return FALSE;
		if (echoCallback)
			(*echoCallback)(echoClientData, s);
		SPRINTF(s, "//\n");
		if (fputs(s, f) < 0) return FALSE;
		if (echoCallback)
			(*echoCallback)(echoClientData, s);

		//
		// Print the referenced 
		//
		int inline_define = FALSE;
#ifdef DEBUG
		if (getenv("DXINLINE"))
			inline_define = TRUE;
#endif
		if ((dest == PrintFile) &&
			!this->printMacroReferences(f, inline_define,
			echoCallback,echoClientData))
			return FALSE;

		if (this->isMacro())
		{
			SPRINTF(s, "// Begin MDF\n");
			if (fputs(s, f) < 0) return FALSE;
			if (echoCallback)
				(*echoCallback)(echoClientData, s);
		}

		SPRINTF(s, "// MODULE %s\n", theSymbolManager->getSymbolString(this->name));
		if (fputs(s, f) < 0) return FALSE;
		if (echoCallback)
			(*echoCallback)(echoClientData, s);
		if (this->category)
		{
			SPRINTF(s, "// CATEGORY %s\n", this->getCategoryString());
			if (fputs(s, f) < 0) return FALSE;
			if (echoCallback)
				(*echoCallback)(echoClientData, s);
		}
		if (this->description && STRLEN(this->description) > 0)
		{
			SPRINTF(s, "// DESCRIPTION %s\n", 
				this->getDescriptionString() ? this->getDescriptionString() : " ");
			if (fputs(s, f) < 0) return FALSE;
			if (echoCallback)
				(*echoCallback)(echoClientData, s);
		}
		if (this->isMacro())
		{
			int i;
			for (i = 1; i <= this->getInputCount(); ++i)
			{
				ParameterDefinition *pd = this->getInputDefinition(i);
				{
					const char * const *strings = pd->getTypeStrings();
					const char *visattr;
					int length = 0;
					int j;
					for (j = 0; strings[j] != NULL; ++j)
						length += STRLEN(strings[j]) + 5;
					char *types = new char[length];
					strcpy(types, strings[0]);
					for (j = 1; strings[j] != NULL; ++j)
					{
						strcat(types, " or ");
						strcat(types, strings[j]);
					}

					const char *dflt = pd->getDefaultValue();
					if (pd->isRequired())
						dflt = "(none)";
					else if (!dflt || (*dflt == '\0') || 
						EqualString(dflt,"NULL"))
						dflt = "(no default)";
					if (!pd->isViewable()) {
						visattr="[visible:2]";
					} else {
						if (pd->getDefaultVisibility())
							visattr="";
						else
							visattr="[visible:0]";
					}
					SPRINTF(s, "// INPUT %s%s; %s; %s; %s\n",
						pd->getNameString(),
						visattr,
						types,
						dflt? dflt: "(no default)",
						pd->getDescription() ? pd->getDescription() : " ");
					delete types;
					if (fputs(s, f) < 0) return FALSE;
					if (echoCallback)
						(*echoCallback)(echoClientData, s);

					// If the input parameter has option values...
					const char *const *options = pd->getValueOptions();
					if (options && options[0]) {
						int option_count = 0;
						while (options[option_count]) option_count++;
						strcpy (s, "// OPTIONS");
						int slen = strlen(s);
						int one_less = option_count - 1;
						for (int i=0; i<one_less; i++) {
							SPRINTF (&s[slen], " %s ;", options[i]);
							slen+= strlen(options[i]) + 3;
							option_count++;
						}
						SPRINTF (&s[slen], " %s\n", options[one_less]);
						if (fputs(s, f) < 0) return FALSE;
						if (echoCallback)
							(*echoCallback)(echoClientData, s);
					}
				}
			}
			for (i = 1; i <= this->getOutputCount(); ++i)
			{
				ParameterDefinition *pd = this->getOutputDefinition(i);
				{
					const char * const *strings = pd->getTypeStrings();
					const char *visattr;
					int length = 0;
					int j;
					for (j = 0; strings[j] != NULL; ++j)
						length += STRLEN(strings[j]) + 5;
					char *types = new char[length];
					strcpy(types, strings[0]);
					for (j = 1; strings[j] != NULL; ++j)
					{
						strcat(types, " or ");
						strcat(types, strings[j]);
					}

					if (!pd->isViewable()) {
						visattr="[visible:2]";
					} else {
						if (pd->getDefaultVisibility())
							visattr="";
						else
							visattr="[visible:0]";
					}
					SPRINTF(s, "// OUTPUT %s%s; %s; %s\n",
						pd->getNameString(),
						visattr,
						types,
						pd->getDescription() ? pd->getDescription() : " ");
					delete types;
				}
				if (fputs(s, f) < 0) return FALSE;
				if (echoCallback)
					(*echoCallback)(echoClientData, s);
			}
			SPRINTF(s, "// End MDF\n");
			if (fputs(s, f) < 0) return FALSE;
			if (echoCallback)
				(*echoCallback)(echoClientData, s);
		}
		//
		// Print comments 
		// FIXME: do we care that annotated stuff does not go through 
		//	the echoCallback?
		//
		if (this->comment) {
			if (fprintf(f,"//\n// comment: ") < 0)
				return FALSE;
			int i, len=STRLEN(this->comment);
			for (i=0 ; i<len ; i++) {
				char c = this->comment[i];
				if (putc(c, f) == EOF)
					return FALSE;
				if ((c == '\n') && (i+1 != len)) {
					if (fprintf(f,"// comment: ") < 0)
						return FALSE;
				}
			}
			if (fprintf(f,"\n") < 0)
				return FALSE;
		}

#if WORKSPACE_PAGES
		// Print all the group assignments.
		// Handle the process group specially
		// FIXME: do we care that annotated stuff does not go through
		//	the echoCallback?
		//
		DictionaryIterator di(*this->groupManagers);
		GroupManager *gmgr;
		Symbol psym = theSymbolManager->getSymbol(PROCESS_GROUP);
		while ( (gmgr = (GroupManager*)di.getNextDefinition()) ) {
			if (psym == gmgr->getManagerSymbol()) {
				if ((dest == PrintExec) || (dest == PrintFile)) 
					if (!this->isMacro()) 
						if (!gmgr->printComment(f))
							return FALSE;
			} else {
				if (!gmgr->printComment(f))
					return FALSE;
			}
		}
#else
		//
		// Print the process group assignment.
		//
		if ((dest == PrintExec) || (dest == PrintFile))
			if (!this->isMacro() && 
				!theDXApplication->PGManager->printComment(f))
				return FALSE;
#endif

		//
		// Print workspace information 
		//
		this->workSpaceInfo.printComments(f);
		SPRINTF(s, "//\n");
		if (fputs(s, f) < 0) return FALSE;
		if (echoCallback)
			(*echoCallback)(echoClientData, s);

	}
	//SMH all sprintfs in the remainder of this routine were changed to
	//    remember length of formatted string in variable l

	int l = SPRINTF(s, "macro %s(\n",
		theSymbolManager->getSymbolString(this->name));

	if (dest == PrintFile || dest == PrintCut || dest == PrintCPBuffer) {
		if (fputs(s, f) < 0) return FALSE;
		if (echoCallback) (*echoCallback)(echoClientData, s);
	} else {
		ASSERT(dest==PrintExec);
		pif->sendBytes(s);
	}

	int i;
	for (i = 1; this->isMacro() && i <= this->getInputCount(); ++i)
	{
		ParameterDefinition *param = this->getInputDefinition(i);
		if (param->getNameSymbol() == -1)
			l = SPRINTF(s, "%c%s\n", (i == 1? ' ': ','), "dummy");
		else if (param->isDefaultDescriptive() ||
			param->isRequired() ||
			param->getDefaultValue() == NULL ||
			EqualString(param->getDefaultValue(), "NULL"))
			l = SPRINTF(s, "%c%s\n", (i == 1? ' ': ','), param->getNameString());
		else
			l = SPRINTF(s, "%c%s = %s\n", (i == 1? ' ': ','),
			param->getNameString(), param->getDefaultValue());
		if (dest == PrintFile || dest == PrintCut || dest == PrintCPBuffer) {
			if (fputs(s, f) < 0) return FALSE;
			if (echoCallback) (*echoCallback)(echoClientData, s);
		} else {
			ASSERT(dest==PrintExec);
			pif->sendBytes(s);
		}
	}
	l = SPRINTF(s, ") -> (\n");

	if (dest == PrintFile || dest == PrintCut || dest == PrintCPBuffer) {
		if (fputs(s, f) < 0) return FALSE;
		if (echoCallback) (*echoCallback)(echoClientData, s);
	} else {
		ASSERT(dest==PrintExec);
		pif->sendBytes(s);
	}

	for (i = 1; this->isMacro() && i <= this->getOutputCount(); ++i)
	{
		ParameterDefinition *param = this->getOutputDefinition(i);
		if (param->getNameSymbol() == -1)
			l = SPRINTF(s, "%c%s\n", (i == 1? ' ': ','), "dummy");
		else
			l = SPRINTF(s, "%c%s\n", (i == 1? ' ': ','), param->getNameString());

		if (dest == PrintFile || dest == PrintCut || dest == PrintCPBuffer) {
			if (fputs(s, f) < 0) return FALSE;
			if (echoCallback) (*echoCallback)(echoClientData, s);
		} else {
			ASSERT(dest==PrintExec);
			pif->sendBytes(s);
		}

	}
	l = SPRINTF(s, ") {\n");
	if (dest == PrintFile || dest == PrintCut || dest == PrintCPBuffer) {
		if (fputs(s, f) < 0) return FALSE;
		if (echoCallback) (*echoCallback)(echoClientData, s);
	} else {
		ASSERT(dest==PrintExec);
		pif->sendBytes(s);
	}


	Node *n;
	ListIterator li;
	const char *prefix = this->getPrefix();
	FOR_EACH_NETWORK_NODE(this, n, li)
	{
		if (!n->netPrintBeginningOfMacroNode(f, dest, prefix,
			echoCallback, echoClientData))
			return FALSE;
	}

	return TRUE;
}
boolean Network::printBody(FILE *f,
			   PrintType dest,
			   PacketIFCallback echoCallback,
			   void *echoClientData)
{
    Node *n;
    ListIterator l;
    const char *prefix; 
    prefix = this->getPrefix();

    if (this->editor && this->isDirty())
	this->sortNetwork();

    this->resetImageCount();

    FOR_EACH_NETWORK_NODE(this, n, l)
    {
	StandIn* si = n->getStandIn();
	if((dest == PrintFile) || 
	   (dest == PrintExec) ||
	   (dest == PrintCPBuffer && (this->selectionOwner->nodeIsInInstanceList(n))) ||
	   (dest == PrintCut && si && si->isSelected()))
	{
	    if (!n->netPrintNode(f, dest, prefix, echoCallback, 
				 echoClientData))
		return FALSE;
	}
    }

    Decorator *dec;
    FOR_EACH_NETWORK_DECORATOR(this, dec, l)
    {
	if((dest == PrintFile) || 
	   (dest == PrintCut && dec->isSelected()))
	{
	    if (!dec->printComment (f))
		return FALSE;
	}
    }

    return TRUE;
}
boolean Network::printTrailer(FILE *f,
			      PrintType dest,
			      PacketIFCallback echoCallback,
			      void *echoClientData)
{
    if (dest == PrintFile || dest == PrintCut || dest == PrintCPBuffer)
    {
        if (fprintf(f, "// network: end of macro body\n") < 0)
	    return FALSE;
    }

    Node *n;
    ListIterator l;
    const char *prefix = this->getPrefix();
    FOR_EACH_NETWORK_NODE(this, n, l)
    {
	if (!n->netPrintEndOfMacroNode(f, dest, prefix,
					echoCallback, echoClientData))
	    return FALSE;
    }

    const char* s = "}\n";
    DXPacketIF* pif = theDXApplication->getPacketIF();
    if (dest == PrintFile || dest == PrintCut || dest == PrintCPBuffer) {
	if (fputs(s, f) < 0) return FALSE;
	if (echoCallback) (*echoCallback)(echoClientData, (char*)s);
    } else {
	ASSERT(dest==PrintExec);
	pif->sendBytes(s);
    }

    return TRUE;
}
boolean Network::printValues(FILE *f, PrintType ptype)
{
    Node *n;
    ListIterator l;
    const char *prefix;

    if (ptype == PrintCPBuffer) return TRUE;

    prefix = this->getPrefix();

    FOR_EACH_NETWORK_NODE(this, n, l)
    {
	// FIXME: ignoring annotate, requires minor fix to n->printValues().
#if WORKSPACE_PAGES
	StandIn* si = n->getStandIn();
	boolean is_selected = (si?si->isSelected():FALSE);
#endif
	if ((ptype != PrintCut) || (is_selected))
	    if (!n->printValues(f, prefix, ptype))
		return FALSE;
    }
    return TRUE;
}
void Network::SendNetwork(void * staticData, void * /*requestData */)
{
     Network *n = (Network*)staticData;
     n->sendNetwork();
}
boolean Network::sendNetwork()
{
    if (this->deferrableSendNetwork->isActionDeferred()) {
      this->deferrableSendNetwork->requestAction(NULL);
      return TRUE;
    }

    DXPacketIF *pi = theDXApplication->getPacketIF();
    if (!pi)
	return TRUE;

    void *cbData;
    PacketIFCallback cb;

    //
    // Only display the network contents if the file that the net came from
    // (if it came from a file) was not encrypted.
    //
    if (this->netFileWasEncoded)
        cb = NULL;
    else
        cb = pi->getEchoCallback(&cbData);

    pi->sendMacroStart();
    if (!this->printHeader(pi->getFILE(), PrintExec, cb, cbData))
	return FALSE;
    if (!this->printBody(pi->getFILE(), PrintExec, cb, cbData))
	return FALSE;
    if (! this->printTrailer(pi->getFILE(), PrintExec, cb, cbData))
	return FALSE;
    pi->sendMacroEnd();
    
    this->clearDirty();

    return TRUE;
}
boolean Network::sendValues(boolean force)
{
    Node *n;
    ListIterator l;

    FOR_EACH_NETWORK_NODE(this, n, l)
    {
	if (!n->sendValues(force))
	{
	    return FALSE;
	}
    }
    return TRUE;
}


boolean Network::checkForCycle(Node *srcNode, Node *dstNode)
{
    Node *n;
    ListIterator l;

    FOR_EACH_NETWORK_NODE(this, n, l)
	n->clearMarked();

    return this->markAndCheckForCycle(srcNode,dstNode);
}
boolean Network::markAndCheckForCycle(Node *srcNode, Node *dstNode)
{
    int  i;

    if (srcNode == dstNode)
        return TRUE;  // A cycle has been detected

    //
    // We've already been to this node (and not found a cycle).
    //
    if (dstNode->isMarked())
	return FALSE;

    //
    // Follow the destination Node's output params.
    // If they go back to the source Node then adding an 
    // Ark would create a cycle.
    //
    int  numParam = dstNode->getOutputCount();

    for (i = 1; i <= numParam; ++i)
    {
        if (dstNode->isOutputConnected(i))
        {
            Ark *a;
	    int j;
            List *arcs = (List *)dstNode->getOutputArks(i);
            for (j = 1; (a = (Ark*)arcs->getElement(j)); ++j)
            {
                int paramInd;
                Node *dstPtr = a->getDestinationNode(paramInd);
                if (this->markAndCheckForCycle(srcNode, dstPtr)) {
                       return TRUE;  // A cycle has been detected
                }
            }
         }
    }
    dstNode->setMarked();
    return FALSE; // No cycle has been found 
}

  
int
Network::connectedNodes(boolean *marked, int ind, int d)
{
    int		markedNodes = 0;
    int		i;
    Node       *n;
    int		j;

    if (marked[ind])
	return markedNodes;

    marked[ind] = TRUE;
    ++markedNodes;
    n = (Node*)this->nodeList.getElement(ind+1);

    if (d <= 0)
    {
	int numParam = n->getInputCount();
	for (i = 1; i <= numParam; ++i)
	{
	    if (n->isInputConnected(i) && n->isInputVisible(i))
	    {
		Ark *a;
		List *arcs = (List *)n->getInputArks(i);
		for (j = 1; (a = (Ark*)arcs->getElement(j)); ++j)
		{
		    int paramInd;
		    Node *n2 = a->getSourceNode(paramInd);
		    markedNodes += this->connectedNodes(
					marked,
					this->nodeList.getPosition((void*)n2)-1,
					d);
		}
	    }
	}
    }
    if (d >= 0)
    {
	int numParam = n->getOutputCount();
	for (i = 1; i <= numParam; ++i)
	{
	    if (n->isOutputConnected(i) && n->isOutputVisible(i))
	    {
		Ark *a;
		List *arcs = (List *)n->getOutputArks(i);
		for (j = 1; (a = (Ark*)arcs->getElement(j)); ++j)
		{
		    int paramInd;
		    Node *n2 = a->getDestinationNode(paramInd);
		    markedNodes += this->connectedNodes(
					marked,
					this->nodeList.getPosition((void*)n2)-1,
					d);
		}
	    }
	}
    }

    return markedNodes;
}

int Network::visitNodes(Node *n)
{
    int		i;

    int numParam = n->getInputCount();

    for (i = 1; i <= numParam; ++i)
    {
	if (n->isInputConnected(i))
	{
	    //
	    // Visit each upstream node. 
	    // Note, we don't need to sort the arcs to get a deterministic 
	    // sort, because we are going upstream instead of downstream
	    // (i.e. there is one arc per input).
	    //
	    Ark *a;
	    List *arcs = (List *)n->getInputArks(i);
	    ListIterator iter(*arcs);
	    while ( (a = (Ark*)iter.getNext()) ) 
	    {
		int paramInd;
		Node *n2 = a->getSourceNode(paramInd);
		if (!n2->isMarked())
		    this->visitNodes(n2);
	    }
	}
    }

    n->setMarked();
    this->nodeList.appendElement((void*)n);

    return 0;
}

//
// Sort in alphabetic, 
// This will make sure, that when there is no ordering between say 
// AutoAxes and Compute, AutoAxes will get written out first.  
// Also, handle reference numbers similarly so that
// last placed tools (of the same name), get executed last.
// If v > ref return less than 0, else if v < ref return greater than 0, 
// else return 0;
//
static int CompareNodeName(const void *v, const void *ref)
{
   Node *refNode = (Node*)ref;
   Node *node = (Node*)v;
   int r = STRCMP(node->getNameString(),refNode->getNameString());
   if (r == 0)
	r = node->getInstanceNumber() - refNode->getInstanceNumber();

   return r;
}

void Network::sortNetwork()
{
    int	     i;
    int      numNodes = this->nodeList.getSize();
    ListIterator iterator;
    Node	*n;

    if (numNodes <= 1)
	return;

    //
    // Sort the nodes so the visitation is deterministic.
    //
    this->nodeList.sort(CompareNodeName);    
    List tmpNodeList;
    iterator.setList(this->nodeList);
    for (i=0 ; (n = (Node*)iterator.getNext()) ; i++) {
	tmpNodeList.appendElement((void*)n);
        n->clearMarked();
    }
    this->nodeList.clear();

    //
    // Now that we have a copy of the sorted list, visit all the nodes.
    //
    iterator.setList(tmpNodeList);
    while ( (n = (Node*)iterator.getNext()) ) {
	if (!n->isMarked())
	    this->visitNodes(n);
    }
  
}

void Network::cfgParseInteractorComment(const char* comment)
{
    Symbol namesym;
    int		items_parsed, instance;
    char	interactor_name[1024];	// FIXME: allocate this

    ASSERT(comment);

    this->parseState.control_panel = NUL(ControlPanel*);

    items_parsed =
        sscanf(comment, " interactor %[^[][%d]:",
               interactor_name,
               &instance);

    /*
     * If all items found...
     */
    if (items_parsed != 2)
    {
        ErrorMessage("Bad interactor comment file %s line %d\n",
                                        Network::ParseState::parse_file,yylineno);
	Network::ParseState::error_occurred = TRUE;
	return;
    }

    /*
     * Get the symbol for the name of this node.
     * If not found in the symbol table it is an error.
     */
    namesym = theSymbolManager->getSymbol(interactor_name);
    if (!namesym) {
        ErrorMessage(
            "Interactor %s at line %d file %s not found in the symbol table",
                        interactor_name, yylineno, Network::ParseState::parse_file);
	Network::ParseState::error_occurred = TRUE;
	return;
    }


    /*
     * Find this node in the current network.
     * If not found in the network it is an error.
     */
    this->parseState.node = this->findNode(namesym,instance);
    if (!this->parseState.node) {
        ErrorMessage(
           "Node %s (instance %d) not found in the program (file %s, line %d)",
                        interactor_name, instance, 
					Network::ParseState::parse_file, yylineno);
	Network::ParseState::error_occurred = TRUE;
	return;
    }

    if (!this->parseState.node->cfgParseComment(comment, 
			Network::ParseState::parse_file, yylineno)) {
         WarningMessage("Unrecognized comment at line %d of file %s"
                         " (ignoring)",  yylineno, Network::ParseState::parse_file);
    }
    this->parseState.parse_state = _PARSED_INTERACTOR;
}

//
// Parse any network specific comments found in the .cfg file.
//
boolean Network::cfgParseComment(const char* comment, 
					const char *file, int lineno)
{
    ASSERT(this->panelGroupManager);
    ASSERT(this->panelAccessManager);
    return theDXApplication->parseComment(comment,file,lineno) ||
	   this->panelGroupManager->cfgParseComment(comment,file,lineno) ||
	   this->panelAccessManager->cfgParseInaccessibleComment(comment,
							file,lineno);
}

void Network::cfgParsePanelComment(const char* comment)
{
    int i;

    ASSERT(comment);

    sscanf(comment, " panel[%d]", &i);
    this->parseState.control_panel = this->getPanelByInstance(i+1);
    if (NOT this->parseState.control_panel)
    	this->parseState.control_panel = this->newControlPanel(i+1);

    if (!this->parseState.control_panel->cfgParseComment(comment, 
				Network::ParseState::parse_file, yylineno)) {
          WarningMessage("Unrecognized comment at line %d of file %s"
                        " (ignoring)",  yylineno, Network::ParseState::parse_file);
    }

    this->parseState.parse_state = _PARSED_PANEL;
}


//
// Find a node with a specific name and return its instance number
// as well as its position in the list.
//
Node *Network::findNode(const char* name, int* startPos, boolean byLabel)
{
    ListIterator li(this->nodeList);
    Node        *n;

    if (startPos) {
	if (*startPos > (this->nodeList).getSize())
	    return NULL;
	li.setPosition(*startPos);
    }

    for(n = (Node*)li.getNext(); n; n = (Node*)li.getNext())
    {
	if (startPos)
	    (*startPos)++;
	if (EqualString(n->getLabelString(),name) AND byLabel
            OR EqualString(n->getNameString(),name) AND !byLabel)
             return(n);
    }

    return NULL;
}

//
// Search the network for a node of the given class and return TRUE
// if found.
//


void Network::postSaveCfgDialog(Widget parent)
{
    if (this->saveCfgDialog == NULL) 
        this->saveCfgDialog = new SaveCFGDialog(parent, this);

    this->saveCfgDialog->post();
}
void Network::postOpenCfgDialog(Widget parent)
{
    if (this->openCfgDialog == NULL) 
        this->openCfgDialog = new OpenCFGDialog(parent, this);

    this->openCfgDialog->post();
}
void Network::postSaveAsDialog(Widget parent, Command *cmd)
{
    if (this->saveAsDialog == NULL) {
        this->saveAsDialog = new SaveAsDialog(parent, this);
    }

    this->saveAsDialog->setPostCommand(cmd);
    this->saveAsDialog->post();
}


boolean Network::isMacro()
{
    return this->definition != NULL;
}
// FIX me
boolean Network::canBeMacro()
{
    Node *n;
    ListIterator li;
    FOR_EACH_NETWORK_NODE(this, n, li)
	if (!n->isAllowedInMacro())
	    return FALSE;
    
    return TRUE;
}
boolean Network::makeMacro(boolean make)
{
    if (make && !this->canBeMacro())
	return FALSE;
    else if (make && this->isMacro())
	return TRUE;
    // FIXME: be sure to return false if (!make && I have inputs)

    if (make)
    {
	const char *name = this->getNameString();
	MacroDefinition *md = new MacroDefinition();
	md->setName(name);
	md->setCategory(this->getCategorySymbol());

	this->definition = md;
    }
    // Make this a non-macro iff it's the main network.  Otherwise, it's
    // got to stay a macro.
    else if (this == theDXApplication->network)
    {
	delete this->definition;
	this->definition = NULL;
    }
    
    return TRUE;
}

//
// Find the first free index (indices start from 1) for nodes with the 
// given name. 
// Currently, this only works for Input/Output nodes.
//
int Network::findFreeNodeIndex(const char *nodename)
{
    int retindex;

    // MacroParameterNodes are the only ones that have an index (currently).
    ASSERT(EqualString(nodename,"Input") ||
     	   EqualString(nodename,"Output"));

    List *l = this->makeNamedNodeList(nodename);
    if (!l) {
	retindex = 1;
    } else {
	int i, j, nodecnt = l->getSize();
	//
	// The following algorithm assumes that all nodes of the
	// given class in the network all have valid indices. 
	//
	    
	//
	// Bubble sort the nodes by index (nodes[0] has smallest index).
	//
	int *indices= new int[nodecnt];
	for (i=0 ; i<nodecnt ; i++) {
	    MacroParameterNode *n = (MacroParameterNode*)l->getElement(i+1);
	    indices[i] = n->getIndex();
	}
	for (i=0 ; i<nodecnt ; i++) {
	    for (j=i+1 ; j<nodecnt ; j++) {
		if (indices[j] < indices[i]) {
		    int tmp  = indices[i];
		    indices[i] = indices[j];
		    indices[j] = tmp; 
		}
	    }
	}
	retindex = 0;
	for (i=1 ; !retindex && i<=nodecnt ; i++) {
	    if (indices[i-1] != i)
		retindex = i;
	}
	if (!retindex)
	    retindex = nodecnt + 1; 
	delete indices;
	delete l;
    }	
    return retindex;
}

boolean Network::moveInputPosition(MacroParameterNode *n, int index)
{
	ASSERT(this->isMacro());
	ParameterDefinition *pd = n->getParameterDefinition();
	int inputCount = this->definition->getInputCount();
	int oldPos = 0;
	for (int i = 1; i <= inputCount; ++i)
	{
		ParameterDefinition *oldPd = this->definition->getInputDefinition(i);
		if (pd == oldPd)
		{
			oldPos = i;
			break;
		}
	}
	ASSERT(oldPos != 0);

	if (oldPos == index)
		return TRUE;

	boolean return_val = TRUE;
	this->deferrableSendNetwork->deferAction();
	//
	// At this point, oldPos is where the parameter was, and index is where
	// we want it to be.  First, we put a dummy where the parameter was
	// (unless it was at the end), then we put the parameter where it should
	// be.
	if (oldPos == inputCount)
	{
		this->definition->removeInput(pd);
		inputCount--;
	}
	else
	{
		ParameterDefinition *dummy = new ParameterDefinition(-1);
		dummy->setDummy(TRUE);
		dummy->setName("input");
		dummy->markAsInput();
		dummy->setDefaultVisibility();
		dummy->addType(new DXType(DXType::ObjectType));
		this->definition->replaceInput(dummy, pd);
	}
	if (index > inputCount)
	{
		for (int i = inputCount + 1; i < index; ++i)
		{
			ParameterDefinition *dummy = new ParameterDefinition(-1);
			dummy->setDummy(TRUE);
			dummy->setName("input");
			dummy->markAsInput();
			dummy->setDefaultVisibility();
			dummy->addType(new DXType(DXType::ObjectType));
			dummy->setDescription("Dummy parameter");
			this->definition->addInput(dummy);
		}
		this->definition->addInput(pd);
	}
	else
	{
		ParameterDefinition *targetPd =
			this->definition->getInputDefinition(index);
		List *l = this->makeNamedNodeList(n->getNameString());
		MacroParameterNode *mpn = NULL;
		if (l)
		{
			ListIterator li(*l);
			while( (mpn = (MacroParameterNode*)li.getNext()) )
			{
				if (mpn->getIndex() == index)
					break;
			}
			delete l;
		}
		if (mpn == NULL)
		{
			this->definition->replaceInput(pd, targetPd);
			delete targetPd;
		}
		else
		{
			if (oldPos == inputCount+1)
				this->definition->addInput(pd);
			else
			{
				ParameterDefinition *dummyPd =
					this->definition->getInputDefinition(oldPos);
				this->definition->replaceInput(pd, dummyPd);
				delete dummyPd;
			}
			return_val =  FALSE;
		}
	}

	if (return_val)
		n->setIndex(index);

	this->deferrableSendNetwork->undeferAction();

	return return_val;
}
boolean Network::moveOutputPosition(MacroParameterNode *n, int index)
{
	ASSERT(this->isMacro());
	ParameterDefinition *pd = n->getParameterDefinition();
	int outputCount = this->definition->getOutputCount();
	int oldPos = 0;
	for (int i = 1; i <= outputCount; ++i)
	{
		ParameterDefinition *oldPd = this->definition->getOutputDefinition(i);
		if (pd == oldPd)
		{
			oldPos = i;
			break;
		}
	}
	ASSERT(oldPos != 0);

	if (oldPos == index)
		return TRUE;

	this->deferrableSendNetwork->deferAction();
	boolean return_val = TRUE;

	if (oldPos == outputCount)
	{
		this->definition->removeOutput(pd);
		outputCount--;
	}
	else
	{
		ParameterDefinition *dummy = new ParameterDefinition(-1);
		dummy->setDummy(TRUE);
		dummy->setName("output");
		dummy->markAsOutput();
		dummy->setDefaultVisibility();
		dummy->addType(new DXType(DXType::ObjectType));
		dummy->setDescription("Dummy parameter");
		this->definition->replaceOutput(dummy, pd);
	}
	if (index > outputCount)
	{
		for (int i = outputCount + 1; i < index; ++i)
		{
			ParameterDefinition *dummy = new ParameterDefinition(-1);
			dummy->setDummy(TRUE);
			dummy->setName("output");
			dummy->markAsOutput();
			dummy->setDefaultVisibility();
			dummy->addType(new DXType(DXType::ObjectType));
			dummy->setDescription("Dummy parameter");
			this->definition->addOutput(dummy);
		}
		this->definition->addOutput(pd);
	}
	else
	{
		ParameterDefinition *targetPd =
			this->definition->getOutputDefinition(index);
		List *l = this->makeNamedNodeList(n->getNameString());
		MacroParameterNode *mpn = NULL;
		if (l)
		{
			ListIterator li(*l);
			while( (mpn = (MacroParameterNode*)li.getNext()) )
			{
				if (mpn->getIndex() == index)
					break;
			}
			delete l;
		}
		if (mpn == NULL)
		{
			this->definition->replaceOutput(pd, targetPd);
			delete targetPd;
		}
		else
		{
			if (oldPos == outputCount+1)
				this->definition->addOutput(pd);
			else
			{
				ParameterDefinition *dummyPd =
					this->definition->getOutputDefinition(oldPos);
				this->definition->replaceOutput(pd, dummyPd);
				delete dummyPd;
			}
			return_val = FALSE;
		}
	}
	if (return_val)
		n->setIndex(index);

	this->deferrableSendNetwork->undeferAction();

	return return_val;
}

void Network::setDefinition(MacroDefinition *md)
{
    this->definition = md;
}
MacroDefinition *Network::getDefinition()
{
    return this->definition;
}
ParameterDefinition *Network::getInputDefinition(int i)
{
    return this->getDefinition()->getInputDefinition(i);
}
ParameterDefinition *Network::getOutputDefinition(int i)
{
    return this->getDefinition()->getOutputDefinition(i);
}
int Network::getInputCount()
{
    MacroDefinition *md = this->definition; 
    Node *node;
    ListIterator l;

    if (md)
	return md->getInputCount();
   
    int count = 0;
    FOR_EACH_NETWORK_NODE(this, node, l) {
	if (node->isA(ClassMacroParameterNode)) {
	    MacroParameterNode *mpn = (MacroParameterNode*)node;
	    if (mpn->isInput() && mpn->getIndex() > count)
		count = mpn->getIndex();
	}
    }
    return count;
}
int Network::getOutputCount()
{
    MacroDefinition *md = this->definition; 
    Node *node;
    ListIterator l;

    if (md)
	return md->getOutputCount();
   
    int count = 0;
    FOR_EACH_NETWORK_NODE(this, node, l) {
	if (node->isA(ClassMacroParameterNode)) {
	    MacroParameterNode *mpn = (MacroParameterNode*)node;
	    if (!mpn->isInput() && mpn->getIndex() > count)
		count = mpn->getIndex();
	}
    }
    return count;
}

void Network::openColormap(boolean openAll)
{
    ColormapNode        *n;
    ListIterator              li;
    List *cmaps = NULL;

    if (openAll) {
	cmaps = this->makeClassifiedNodeList(ClassColormapNode);
	if (!cmaps)
	    return;
    } else {
	EditorWindow *e = this->editor;
	ASSERT(e);
	cmaps = e->makeSelectedNodeList(ClassColormapNode);
    }
    li.setList(*cmaps);

    while( (n = (ColormapNode*)li.getNext()) )
        n->openDefaultWindow(theDXApplication->getRootWidget());

    delete cmaps;
}

void Network::setDescription(const char *description, boolean markDirty)
{
    if (this->description)
	delete this->description;
    this->description = DuplicateString(description);
    if (markDirty)
	this->setFileDirty();
}
Symbol Network::setCategory(const char *cat, boolean markDirty)
{
    if ((cat) && (cat[0]))
	this->category = theSymbolManager->registerSymbol(cat);
    else
	this->category = 0;
    if (markDirty)
	this->setFileDirty();
    return this->category;
}

const char *Network::getDescriptionString()
{
    return this->description;
}

boolean Network::postNameDialog()
{
    if (!this->setNameDialog)
    {
	// FIXME: should this be an editor command.
        EditorWindow *editor = this->getEditor();
	Widget parent;
	if (editor)
	    parent = editor->getRootWidget();
	else
	    parent = theDXApplication->getAnchor()->getRootWidget();
	this->setNameDialog = new SetMacroNameDialog(parent,this);
    }
    this->setNameDialog->post();
    return TRUE;
}

Node *Network::getNode(const char *name, int instance)
{
    Symbol s = theNDAllocatorDictionary->getSymbolManager()->
		registerSymbol(name);

    return this->findNode(s,instance);
}
void Network::editNetworkComment()
{
     if (!this->setCommentDialog) {
        this->setCommentDialog = new SetNetworkCommentDialog(
                                this->getEditor()->getRootWidget(),
                                FALSE, this);
     }
     this->setCommentDialog->post();
}
void Network::postHelpOnNetworkDialog()
{
     if (!this->helpOnNetworkDialog) {
	Widget parent = theDXApplication->getRootWidget();
        this->helpOnNetworkDialog = new HelpOnNetworkDialog(
				parent, this);
     }
     this->helpOnNetworkDialog->post();
}
//
// Set the comment of this network.
//
void Network::setNetworkComment(const char *comment)
{

	if (comment != this->comment) {
		this->setDirty();
		if (this->comment)
			delete this->comment;
		if (comment && STRLEN(comment)) {
			this->comment = DuplicateString(comment);
		} else {
			this->comment = NULL; 
		}
	}

	if (this->comment && STRLEN(this->comment))
		this->helpOnNetworkCmd->activate();
	else 
		this->helpOnNetworkCmd->deactivate();
}

//
// This will reset the image count for isLastImage();
void Network::resetImageCount()
{
    this->imageCount = 0;
}

//
// Returns true if the caller is the last element of the image list.
// It is assumed that each member of this list will determine, during
// Node::prepareToSendNode, will call this to see if it's the last one.
boolean Network::isLastImage()
{
    return ++this->imageCount == this->imageList.getSize();
}
Symbol Network::setName(const char *n) 
{
    Symbol newName = theSymbolManager->registerSymbol(n);
    boolean needsValues = this->name != newName && this->prefix != NULL;
    this->setDirty();
    if (this->prefix)
	delete this->prefix;
    this->prefix = NULL;
    this->name = newName;
    if (needsValues && theDXApplication->getPacketIF() != NULL)
	this->sendValues(TRUE);
    return this->name;
}
//
// Given lists of old and new NodeDefinitions, redefine any nodes
// in the current network.
//
boolean Network::redefineNodes(Dictionary *newdefs, Dictionary *olddefs)
{
    Node *node;
    ListIterator l;
 
    FOR_EACH_NETWORK_NODE(this, node, l) {
	const char *name = node->getNameString();
	if (olddefs->findDefinition(name)) {
	    NodeDefinition *newdef = (NodeDefinition*)
					newdefs->findDefinition(name);
	    if (newdef) {	// Should always be non-zero
		node->setDefinition(newdef);
		node->updateDefinition();
	    }
	}
    }
    return TRUE;
}

int Network::getNodeCount()
{
    return this->nodeList.getSize();
}

//
// Place menu items (ButtonInterfaces) in the CascadeMenu, that when executed
// cause individually named panels and panel groups to be opened.
// If a PanelAccessManager, then only those ControlPanels defined to be 
// accessible are placed in the CascadeMenu.
// NOTE: to optimize this method, if the menu is returned deactivated()
//	it may not contain the correct contents and so should no be activated()
//	except through subsequent calls to this method.
//
void Network::fillPanelCascade(CascadeMenu *menu, PanelAccessManager *pam)
{
    ButtonInterface *bi;
    Widget parent;
    char *name;
    char buttonName[32];
    int i, size;
    boolean hasChildren = FALSE;


    if ((size = this->getPanelCount()) != 0) {

        menu->clearComponents();

        parent = menu->getMenuItemParent();

        ASSERT(pam);

        //
        // Build the list of panels by name
        //
        for(i = 0; i<size; i++) {
            ControlPanel *cp = this->getPanelByIndex(i+1);
            ASSERT(cp);

            long inst = cp->getInstanceNumber();
            if (pam) {
                if (NOT pam->isAccessiblePanel(inst) OR
                        pam->getControlPanel() == cp)
                    continue;
            }


	    sprintf(buttonName,"panel%d",inst);
            ButtonInterface *bi = new ButtonInterface(parent, buttonName,
                                        this->openPanelByInstanceCmd);

            name = (char*)cp->getPanelNameString();
            if(IsBlankString(name))
                name = "Control Panel";
            bi->setLabel(name);
            bi->setLocalData((void*)inst);
            menu->appendComponent(bi);
            hasChildren = TRUE;
        }

        //
        // Now add panel groups
        //
        PanelGroupManager *pgm = this->panelGroupManager;
        size = pgm->getGroupCount(); 
        if (size > 0) {
            for (i=1; i<=size; i++) {
                name = (char*)pgm->getPanelGroup(i, NULL);
                if (pam && pam->isAccessibleGroup(name)) {
                    hasChildren = TRUE;
	    	    sprintf(buttonName,"group%d",i);
                    bi = new ButtonInterface(parent, buttonName,
                                        this->openPanelGroupByIndexCmd);

                    bi->setLabel(name);
                    bi->setLocalData((void*)i);
                    menu->appendComponent(bi);
                }
            }
        }

    }

    if (hasChildren)
        menu->activate();
    else
        menu->deactivate();

}

#ifdef NOTYET
//
// look through the network list and make a count of the nodes with
// the give name.
// If nodename is NULL then count all of them.
//
int Network::countNodesByName(const char *nodename)
{
    Node *node;
    ListIterator iterator;
    int count;

    if (nodename) {
        count = 0;
        FOR_EACH_NETWORK_NODE(this, node, iterator) {
            if (EqualString(node->getNameString(),nodename))
                count++;
        }
    } else {
        count = this->getNodeCount();
    }
    return count;
}
#endif // NOTYET
//
// Return true if the network is different from that on disk, and the
// application allows saving of .net and .cfg files.
//
boolean Network::saveToFileRequired()
{
    int count = this->getNodeCount() + this->decoratorList.getSize();
#if WORKSPACE_PAGES
    DictionaryIterator di(*this->groupManagers);
    GroupManager* gmgr;
    while ( (gmgr = (GroupManager*)di.getNextDefinition()) ) 
	count+= gmgr->getGroupCount();
#endif
    return this->isFileDirty() && (count > 0) && 
	theDXApplication->appAllowsSavingNetFile(this) &&
	theDXApplication->appAllowsSavingCfgFile() &&
	!this->netFileWasEncoded;
}

//
// Merge the new network into this network.
//
boolean Network::mergeNetworks(Network *new_net, List *panels, boolean allNodes, boolean stitch)
{
Node		*cur_node;
Node		*new_node;
ListIterator	cur_l;
ListIterator	new_l;
ControlPanel	*new_cp;
ListIterator	new_cp_l;
List		cpDeleteList;
List		removedNodes;
List		removedDecs;
List		swapFromNodes; 	  // nodes in new_net that aren't going to be
List		swapToNodes;	  // transferred to this network.
				

    //
    // make sure we don't merge an encoded (unviewable) network into 
    // a network that has an editor and that would allow viewing of the
    // encoded program.
    //
    if (this->getEditor() && new_net->wasNetFileEncoded()) {
	ErrorMessage("Encoded visual programs are not viewable.\n"
		     "Attempt to merge encoded program aborted.");
	return FALSE;
    }

    //
    // Delete the image windows: Their network pointers are all wrong.
    // They are re-created in DisplayNode::switchNetwork()
    //
    ImageWindow *iw;
    while ( (iw = (ImageWindow *)new_net->imageList.getElement(1)) )
	delete iw;

    FOR_EACH_NETWORK_NODE(new_net, new_node, new_l) 
    {
	if (allNodes ||
	    (new_node->getStandIn() && new_node->getStandIn()->isSelected()))
	    removedNodes.appendElement(new_node);
    }

    Decorator *new_dec;
    FOR_EACH_NETWORK_DECORATOR(new_net, new_dec, new_l) 
    {
	if (allNodes || new_dec->isSelected())
	    removedDecs.appendElement(new_dec);
    }

    for (new_l.setList(removedNodes); (new_node = (Node*)new_l.getNext()); )
    {
	Symbol new_name = new_node->getNameSymbol();
	long   new_inst = new_node->getInstanceNumber();

	//
	// See if we have a name/instance collision
	//
	FOR_EACH_NETWORK_NODE(this, cur_node, cur_l) 
	{
	    if (!stitch) {
		if((cur_node->getNameSymbol() == new_name) &&
		   (cur_node->getInstanceNumber() == new_inst))
		{
		    new_node->assignNewInstanceNumber();
		}
	    } else {
		// we're helping to undo a node deletion in the vpe.
		// We want to figure out the arcs that were incident on
		// the current node in the incoming network and shift 
		// those arcs over to the node in the existing network.
		if((cur_node->getNameSymbol() == new_name) &&
		   (cur_node->getInstanceNumber() == new_inst))
		{
		    swapFromNodes.appendElement (new_node);
		    swapToNodes.appendElement (cur_node);
		}
	    }
	}
	new_net->nodeList.removeElement(new_node);
	// new_net->deferrableRemoveNode->requestAction((void*)new_node);
    }

    //
    // Check to see if there are any problems with moving things over.  If
    // there are, abort the whole process.
    //
    for (new_l.setList(removedNodes); (new_node = (Node*)new_l.getNext()); )
    {
	if (!new_node->canSwitchNetwork(new_net,this))
	{
	    return FALSE;
	}
    }

#if WORKSPACE_PAGES
    //
    // Deal with group information.  
    // Rules:
    //	1) Throw out all group info from GroupManagers who say they can't
    //	survive a network merge.
    //	2) Throw out all group info from the first group from GroupManagers who
    //	say they can survive a merge and keep group info from remaining groups.
    //  caveat: only throw out the first group if its name doesn't match a name
    //  already in the current network.
    //	3) When keeping group info, if the group doesn't already exist in the 
    //	current net, then create the group otherwise just cruise along.
    //
    // I wanted to put this code into a GroupManager::switchNetwork method, but
    // it requires scanning the list of removedNodes and it requires knowledge
    // of the set of groups rather than just knowledge internal to the group.
    //
    if (this->editor) this->editor->beginPageChange();
    Dictionary* mergingGroup = new_net->getGroupManagers();
    DictionaryIterator di(*mergingGroup);
    GroupManager* gmgr;
    while ( (gmgr = (GroupManager*)di.getNextDefinition()) ) {
	Symbol gmgr_sym = gmgr->getManagerSymbol();
	const char* first_group = NUL(char*);
	//
	// The following check on allNodes assumes that the incoming network
	// originated in a single page.  See EditorWindow::macroify...
	//
	if ((gmgr->survivesMerging() == FALSE) || (allNodes == FALSE)) {
	    for (new_l.setList(removedNodes); (new_node = (Node*)new_l.getNext()); ) {
		const char* group_name = new_node->getGroupName(gmgr_sym);
		if (group_name) {
		    new_node->setGroupName(NUL(GroupRecord*), gmgr_sym);
		}
	    }
	    for (new_l.setList(removedDecs); (new_dec = (Decorator*)new_l.getNext()); ) {
		VPEAnnotator* vpea = (VPEAnnotator*)new_dec;
		const char* group_name = vpea->getGroupName(gmgr_sym);
		if (group_name) {
		    vpea->setGroupName(NUL(GroupRecord*), gmgr_sym);
		}
	    }
	} else {
	    GroupManager* local_gmgr = (GroupManager*)
		this->groupManagers->findDefinition(gmgr_sym);
	    for (new_l.setList(removedNodes); (new_node = (Node*)new_l.getNext()); ) {
		GroupRecord* grec = NUL(GroupRecord*);
		const char* group_name = new_node->getGroupName(gmgr_sym);
		if (group_name) {
		    if (local_gmgr)
			grec = local_gmgr->getGroup(group_name);

		    if ((grec == NUL(GroupRecord*)) && (first_group == NUL(char*)))
			first_group = group_name;

		    if ((first_group) && (EqualString(first_group, group_name))) {
			new_node->setGroupName(NUL(GroupRecord*), gmgr_sym);
		    } else {
			if (grec == NUL(GroupRecord*)) {
			    boolean group_created = 
				local_gmgr->createGroup (group_name, this);
			    if (!group_created)
				new_node->setGroupName(NUL(GroupRecord*), gmgr_sym);
			    else {
				grec = local_gmgr->getGroup(group_name);
				new_node->setGroupName(grec, gmgr_sym);
			    }
			} else {
			    new_node->setGroupName(grec, gmgr_sym);
			}
		    }
		}
	    }
	    for (new_l.setList(removedDecs); (new_dec = (Decorator*)new_l.getNext()); ) {
		VPEAnnotator* vpea = (VPEAnnotator*)new_dec;
		const char* group_name = vpea->getGroupName(gmgr_sym);
		GroupRecord* grec = NUL(GroupRecord*);
		if (group_name) {
		    if (local_gmgr)
			grec = local_gmgr->getGroup(group_name);

		    if ((grec == NUL(GroupRecord*)) && (first_group == NUL(char*)))
			first_group = group_name;

		    if ((first_group) && (EqualString(first_group, group_name))) {
			vpea->setGroupName(NUL(GroupRecord*), gmgr_sym);
		    } else {
			if (grec == NUL(GroupRecord*)) {
			    boolean group_created = 
				local_gmgr->createGroup (group_name, this);
			    if (!group_created)
				vpea->setGroupName(NUL(GroupRecord*), gmgr_sym);
			    else {
				grec = local_gmgr->getGroup(group_name);
				vpea->setGroupName(grec, gmgr_sym);
			    }
			} else {
			    vpea->setGroupName(grec, gmgr_sym);
			}
		    }
		}
	    }
	} 
    }
#endif

    //
    // Move them over to the current network
    //
    this->deferrableAddNode->deferAction();
    for (new_l.setList(removedNodes); (new_node = (Node*)new_l.getNext()); )
    {
	boolean silently = FALSE;
	if (stitch) {
	    // don't complain about problems with nodes that are
	    // about to go away.
	    if (swapFromNodes.isMember(new_node))
		silently = TRUE;
	}
	new_node->switchNetwork(new_net,this,silently);
	new_net->nodeList.removeElement(new_node);
	this->addNode(new_node);
    }

    //
    // If we're stitching - a result of undoing a deletion -
    // then for every arc that connects to a member of deleteNodes
    // from a member of new_net that isn't in deleteNodes, throw
    // out the arc and replace the arc with one that connects to
    // the existing counter part to the member of deleteNodes.
    //
    if (stitch) {
	boolean input_tab_problem_detected = FALSE;
	boolean output_tab_problem_detected = FALSE;
	int missing_tab_count = 0;
	int swap_count = swapFromNodes.getSize();
	for (int i=1; i<=swap_count; i++) {
	    Node* fromNode = (Node*)swapFromNodes.getElement(i);
	    const char* cp = fromNode->getLabelString();
	    Node* toNode = (Node*)swapToNodes.getElement(i);
	    ASSERT (fromNode->getNameSymbol() == toNode->getNameSymbol());
	    int input_count = fromNode->getInputCount();
	    int input_count2 = toNode->getInputCount();
	    for (int input=1; input<=input_count; input++) {
		if (!fromNode->isInputVisible(input)) continue;
		List* arcs = (List*)fromNode->getInputArks(input);
		ListIterator iter(*arcs);
		Ark* arc;
		while ((arc=(Ark*)iter.getNext())) {
		    int output;
		    Node* source = arc->getSourceNode(output);
		    // if source is remaining...
		    if (swapFromNodes.isMember(source)) continue;
		    delete arc;
		    if ((input<=input_count2) && (toNode->isInputVisible(input))) {
			if (toNode->isInputConnected(input)) {
			    // can't create the arc because someone else
			    // already wired the input we wanted to use.
			    input_tab_problem_detected = TRUE;
			    missing_tab_count++;
			} else {
			    StandIn* si = toNode->getStandIn();
			    ASSERT (si);
			    si->addArk (this->editor, 
				new Ark(source, output, toNode, input));
			}
		    } else {
			// the tab was missing
			input_tab_problem_detected = TRUE;
			missing_tab_count++;
		    }
		}
	    }
	    int output_count = fromNode->getOutputCount();
	    int output_count2 = toNode->getOutputCount();
	    for (int output=1; output<=output_count; output++) {
		if (!fromNode->isOutputVisible(output)) continue;
		List* arcs = (List*)fromNode->getOutputArks(output);
		ListIterator iter(*arcs);
		Ark* arc;
		while ((arc=(Ark*)iter.getNext())) {
		    int input;
		    Node* dest = arc->getDestinationNode(input);
		    if (swapFromNodes.isMember(dest)) continue;
		    delete arc;
		    if ((output<=output_count2) && (toNode->isOutputVisible(output))) {
			if (dest->isInputConnected(input)) {
			    // can't create the arc because someone else
			    // already wired the input we wanted to use.
			    output_tab_problem_detected = TRUE;
			    missing_tab_count++;
			} else {
			    StandIn* si = toNode->getStandIn();
			    ASSERT (si);
			    si->addArk (this->editor, 
				new Ark (toNode, output, dest, input));
			}
		    } else {
			// the tab was missing
			output_tab_problem_detected = TRUE;
			missing_tab_count++;
		    }
		}
	    }
	    //delete fromNode;
	}
	//swapFromNodes.clear();
	if ((input_tab_problem_detected) || (output_tab_problem_detected)) {
	    char *cp;
	    if ((input_tab_problem_detected) && (!output_tab_problem_detected)) {
		cp = "input";
	    } else if ((!input_tab_problem_detected) && (output_tab_problem_detected)) {
		cp = "output";
	    } else {
		cp = "input and output";
	    }
	    WarningMessage ( 
		"The Undo was not completed because %d\n"
		"%s tab(s) had been hidden, removed, or used", 
		missing_tab_count, cp);
	}
    }

    for (new_l.setList(swapFromNodes); (new_node = (Node*)new_l.getNext()); )
    {
	this->deleteNode(new_node, FALSE);
    }
    swapFromNodes.clear();


    this->deferrableAddNode->undeferAction();

    if (panels)
    {
	//
	// Resolve any instance number collisions among control panels
	//
	for (new_cp_l.setList(*panels);
		(new_cp = (ControlPanel*)new_cp_l.getNext()); )
	{	
	    new_cp->switchNetwork(new_net,this);
	    new_net->panelList.removeElement(new_cp);
	    this->addPanel(new_cp,0);
	}
    }

    //
    // FIXME: Please write a switchNetworks() method for Decorators.
    //
    for (new_l.setList(removedDecs); (new_dec = (Decorator*)new_l.getNext());)
    {
	DecoratorInfo* dnd = new DecoratorInfo (this, (void*)this,
	    (DragInfoFuncPtr)Network::SetOwner,
	    (DragInfoFuncPtr)Network::DeleteSelections,
	    (DragInfoFuncPtr)Network::Select);
	new_dec->setDecoratorInfo (dnd);
	EditorWindow *ew = this->editor;

#if WORKSPACE_PAGES
	if (new_dec->getRootWidget()) {
	    new_dec->unmanage();
	    new_dec->uncreateDecorator();
	}
	if (ew) ew->newDecorator(new_dec);
#else
	EditorWorkSpace *ews = ew->getWorkSpace();
	new_dec->manage(ews);
#endif
	new_net->decoratorList.removeElement((void*)new_dec);
	this->addDecoratorToList ((void*)new_dec);
    }

    this->setDirty();
#if WORKSPACE_PAGES
    if (this->editor) this->editor->endPageChange();
#endif
    return TRUE;
}

//
// Set the upper left hand corner of the bounding box of the network nodes
// to a new position, moving all nodes the same amount
//
void Network::setTopLeftPos(int x, int y)
{
Node		*node;
ListIterator	l;
int		minx, miny;

    minx = miny = 1000000;
    FOR_EACH_NETWORK_NODE(this, node, l)
    {
	minx = MIN(minx, node->getVpeX());
	miny = MIN(miny, node->getVpeY());
    }
    Decorator *dec;
    int decx, decy;
    FOR_EACH_NETWORK_DECORATOR(this, dec, l)
    {
	dec->getXYPosition(&decx, &decy);
	minx = MIN(minx, decx);
	miny = MIN(miny, decy);
    }
    FOR_EACH_NETWORK_NODE(this, node, l)
    {
	node->setVpePosition(node->getVpeX() - minx + x,
			     node->getVpeY() - miny + y);
    }
    FOR_EACH_NETWORK_DECORATOR(this, dec, l)
    {
	dec->getXYPosition(&decx,&decy);
	dec->setXYPosition(decx-minx+x, decy-miny+y);
    }
}

//
// Return the x,y coords of the south most and east most
// nodes in this network.  ... used by InsertNetworkDialog so that
// he can plop in a new net without having it land on top of the 
// existing net.  This isn't really the bounding box because
// "southeast most" excludes width,height of the standin.
//
void Network::getBoundingBox (int *x1, int *y1, int *x2, int *y2)
{
int x,y;    // minima
int mx,my;  // maxima
boolean emptyNet = TRUE;
Node		*n;
ListIterator	l;

    mx = my = 0;
    x = y = 5000; // no way can we seen screen coords bigger than this
    FOR_EACH_NETWORK_NODE (this, n, l) {
        x = MIN(x, n->getVpeX());
        y = MIN(y, n->getVpeY());
        mx = MAX(mx, n->getVpeX());
        my = MAX(my, n->getVpeY());
        emptyNet = FALSE;
    }
    if (emptyNet) {
	*x1 = *y1 = *x2 = *y2 = 0;
    } else {
	*x1 = x; *y1 = y;
	*x2 = mx; *y2 = my;
    }
}

List *Network::getNonEmptyPanels()
{
    List		*result = new List;
    ControlPanel	*cp;
    ListIterator	 li;

    //
    // Get rid of empty panels
    //
    FOR_EACH_NETWORK_PANEL(this, cp, li)
    {	
	// getComponentCount is InstanceCount + DecoratorCount
	if(cp->getComponentCount() != 0)
	{
	    result->appendElement(cp);
	}
    }

    return result;
}

List *Network::makeNamedControlPanelList(const char *name)
{
    ControlPanel *cp;
    ListIterator iterator;
    List        *l = NUL(List*);


    FOR_EACH_NETWORK_PANEL(this, cp, iterator) {
        if ((name == NULL) || EqualString(name, "") ||
				EqualString(cp->getPanelNameString(), name))
        {
            if (!l)
                l = new List;
            l->appendElement((const void*) cp);
        }
    }
    return l;
}

List *Network::makeLabelledNodeList(const char *label)
{
    Node *node;
    ListIterator iterator;
    List        *l = NUL(List*);

    FOR_EACH_NETWORK_NODE(this, node, iterator) {
        if ((label == NULL) || EqualString(node->getLabelString(), label))
      {
            if (!l)
                l = new List;
            l->appendElement((const void*) node);
        }
    }
    return l;
}
#ifndef DXD_LACKS_POPEN
//
// Try and start the network decoder with the given key to decode the
// file given by netfile.  If the envirinment variable DXDECODER does
// not contain the name of the decoder file, then use 
// $DXROOT/bin_$ARCH/dxdecode.
// Returns a valid FILE pointer if the key can decode the network, 
// otherwise a NULL and error message.  This will exit under certain
// conditions that might indicate someone is trying to break the
// security mechanisms of encoded networks.
//
#if DXD_HAS_CRYPT             //CRYPTKEY
static FILE *OpenForDecoding(const char *netfile, const char *key)
{
    char buf[1024];
    char cmd[1024];
    char envbuf[1024];
    FILE *f;
    unsigned int code;
    char *decoder;

    decoder = getenv("DXDECODER");
    if (!decoder) {
	sprintf(buf,"%s/bin_%s/dxdecode", 
		theDXApplication->getUIRoot(),DXD_ARCHNAME);
	decoder = buf;
    }
    sprintf(cmd,"eval \"_$$=%s\";export _$$;%s %s", key, decoder, netfile );
    sprintf(envbuf,"__=%d", getpid());
    putenv(envbuf);
    f = popen (cmd, "r");
    if (!f) {
        ErrorMessage("Could not start %s to decode networks", decoder);
	return NULL;
    }

    putenv("__=");
    code = atoi(fgets(buf,80,f)); //check my pid is first thing in mesg
    if ( ((unsigned int)(getpid()^1234)) != code ){
        fprintf(stderr,"Invalid data from %s, exiting\n", decoder);
        exit(1);
    }

    // for now we will just check for the first / cause we can
    // unget this

    if (fgetc(f) != '/') {
        ErrorMessage("Incorrect key for encoded network %s: %s", 
					netfile , strerror(errno));
	pclose(f);
	f = NULL;
    } else {
	ungetc('/',f);
    }

    return f;
}
#endif // DXD_HAS_CRYPT
#endif // DXD_LACKS_POPEN

//
// Close the .net file that was opened with openNetworkFILE().
//
void Network::closeNetworkFILE(FILE *f)
{
    this->CloseNetworkFILE(f,this->wasNetFileEncoded());
}
void Network::CloseNetworkFILE(FILE *f, boolean wasEncoded)
{
#ifndef DXD_LACKS_POPEN
    if (wasEncoded)
	pclose(f);
    else
#endif
	fclose(f);

}
FILE *Network::openNetworkFILE(const char *netFileName, 
					char **errmsg)
{
    return this->OpenNetworkFILE(netFileName, &this->netFileWasEncoded, errmsg);
}
FILE *Network::OpenNetworkFILE(const char *netFileName, 
					boolean *wasEncoded,
					char **errmsg)
{

    FILE *f;
    char errbuf[1024];
#if ! DXD_HAS_CRYPT             //CRYPTKEY
    char *netfile = DuplicateString(netFileName);
    *wasEncoded = FALSE;
#else
    char buf[1024];
    char cmd[1024];
    const char *key = theDXApplication->getCryptKey();
    boolean forceEncryption = theDXApplication->appForcesNetFileEncryption(); 
    char *netfile = DuplicateString(netFileName);

    ASSERT(wasEncoded);
    *wasEncoded = FALSE;
    errbuf[0] = '\0';

    if (!key && forceEncryption) { 
	SPRINTF(errbuf,
                "%s must be provided a key to open ecnrypted program files",
                theDXApplication->getInformalName());
	goto error;
    }


    if (key != NULL) {

        // Deal with .ntz (compressed) filenames

        char *p = (char*)strrstr(netfile,".ntz");
        if (p != NULL){
            strcpy(p,".ntz");
        }
        else{
            p = (char *)strstr(netfile,".net");
            ASSERT(p);
        }

        f = fopen (netfile, "r");

        if (f == NULL)  {
            SPRINTF(errbuf,"Error opening file %s: %s", 
					netfile, strerror(errno));
	    goto error;
        }

        // Determine if net is encoded by looking for #* in header

        if ((fread(buf,sizeof(unsigned char),2,f)) != 2) {
            SPRINTF(errbuf,"Error reading file %s: %s", 
					netfile, strerror(errno));
	    goto error;
        }

        buf[2] = '\0';

        if (strcmp(buf,"#*") == 0) {

	    fclose(f);

            // net is encrypted lets see if our key is good.

	    f = OpenForDecoding(netfile, key); 
		
	    if (f)
		*wasEncoded = TRUE;
        } else if (forceEncryption) {
	    SPRINTF(errbuf,"Network file %s must be encoded", netfile);
	    goto error;
	} else {
            fseek(f,0,0);
	}
    } else
#endif
	f = fopen (netfile, "r");


    if (f == NULL)  {
        SPRINTF(errbuf,"Error opening file %s: %s", netfile, strerror(errno));
	goto error;
    } 

    delete[] netfile;
    return f;

error:
    if (netfile) delete netfile;
    if (errbuf[0]) {
	if (errmsg)
	   *errmsg = DuplicateString(errbuf);
	else
	    ErrorMessage(errbuf);
    }
    return NULL;

}

#ifdef DXUI_DEVKIT 
//
// Print the visual program as a .c file that uses libDX calls.
// If the filename does not have a '.c' extension, one is added.
// An error message is printed if there was an error.
//
boolean Network::saveAsCCode(const char *filename)
{
    const char *p = strrstr(filename,".c");
    char *nf;

    if (!p) p = strrstr(filename,".C");
   
    if (!p) {
	nf = new char[STRLEN(filename) + 3];
	sprintf(nf,"%s.c",filename);
    } else {
	nf = (char*)filename;
    }

    FILE *f = fopen(nf,"w");
    boolean rcode; 
    if (f) {
	rcode = this->printAsCCode(f);
	fclose(f);
	if (!rcode) {
	    ErrorMessage("Error printing file to %s\n",nf);
	    unlink(nf);
	} 
    } else {
	ErrorMessage("Could not open file %s for writing\n",nf);
	rcode = FALSE;
    }
    if (nf != filename) delete nf;

    return rcode; 
}
//
// Print the visual program as a .c file that uses libDX calls.
// An error message is printed if there was an error.
//
boolean Network::printAsCCode(FILE *f)
{
    Node *n;
    ListIterator l;
    const char *unsup_tools[] = { "Switch", "Route", "Colormap", "Transmitter",
				  "Image", "Receiver", "Sequencer", NULL };
    const char *unsup_nodeclass[] = { "InteractorNode", "ProbeNode", NULL };
    const char **p; 
    
    if (this->isMacro()) {
	ErrorMessage("Can not generate C code from macros");
	return FALSE;
    }

    for (p = unsup_tools ; *p ; p++) {
	List *list = this->makeNamedNodeList(*p);
	if (list) {
	    ErrorMessage("Can not generate C code from programs containing"
			" %s tools",*p);
	    delete list;
	    return FALSE;
	}
    }

    for (p = unsup_nodeclass ; *p ; p++) {
	List *list = this->makeClassifiedNodeList(*p);
	if (list) {
	    Node *n = (Node*)list->getElement(1);
	    ErrorMessage("Can not generate C code from programs containing"
			" %s tools",n->getNameString());
	    delete list;
	    return FALSE;
	}
    }

    if (this->editor && this->isDirty())
	this->sortNetwork();

    this->resetImageCount();

    if (fprintf(f,"#include \"dx/dx.h\"\n\n"
		  "%s()\n{\n"
		  "\n",
		this->getNameString()) <= 0)
	return FALSE;

    FOR_EACH_NETWORK_NODE(this, n, l)
    {
	if (!n->beginDXCallModule(f))
	    return FALSE;
    }

    fprintf(f, "\n    DXInitModules();\n\n");

    FOR_EACH_NETWORK_NODE(this, n, l)
    {
	if (!n->callDXCallModule(f))
	    return FALSE;
    }

    // Include a semi-colon statement incase no statements follow
    if (fprintf(f,"\n\nerror: ;\n\n") <= 0)
	return FALSE;

    FOR_EACH_NETWORK_NODE(this, n, l)
    {
	if (!n->endDXCallModule(f))
	    return FALSE;
    }

    if (fprintf(f,"\n}\n") <= 0)
	return FALSE;

    return TRUE;

}
#endif	// DXUI_DEVKIT

//
// This is a help function for optimizeNodeOutputCacheability() that 
// returns a list of Nodes that are connected to the given output of the
// given node.  This would be trivial, except for Transmitters and Receivers, 
// In which case we need to traverse all Receivers for a given Transmitter 
// and all output arcs for each receiver.
// If destList is given, then we append the Nodes to that list, otherwise we
// allocate one internally.  In either case, we return a list.  In the latter
// case, the user should delete the returned list.
//
List *Network::GetDestinationNodes(Node *src, int output_index, 
				List *destList, boolean recvrsOk)
{
    Ark *a;
 
    if (!destList)
	destList = new List;

    List *arcs = (List *)src->getOutputArks(output_index);

    ListIterator iter(*arcs);
    while ( (a = (Ark*)iter.getNext()) ) {
	int paramInd;
	Node *dest = a->getDestinationNode(paramInd);
	const char *destClassName = dest->getClassName();
	if (EqualString(destClassName,ClassTransmitterNode)) {
	    //
	    // This is a Transmitter, so get a list of all the Receivers.
	    //
	    List *recvrs = Network::GetDestinationNodes(dest,1,NULL, TRUE);
	    if (recvrs) {
		//
		// For each Receiver, find the connected nodes.
		//
		ListIterator iter(*recvrs);
		Node *rcvr;
		while ( (rcvr = (Node*)iter.getNext()) ) {
		    ASSERT(EqualString(rcvr->getClassName(),ClassReceiverNode));
		    Network::GetDestinationNodes(rcvr,1,destList);	
		}
		delete recvrs;
	    }
	} else {
	    ASSERT(recvrsOk || !EqualString(destClassName,ClassReceiverNode));
	    if (!destList->isMember((void*)dest))
		destList->appendElement((void*)dest);
	}
    }
	    
    return destList;

}
//
// Optimally set the output cacheability of all nodes within the network.
// We only operate on those that have writable cacheability.
//
void Network::optimizeNodeOutputCacheability()
{
    ListIterator iterator;
    Node        *src;
    EditorWindow *editor = this->getEditor(); 

    //
    // Need to check destination nodes to see if they're driven.
    //
    Symbol driven_sym = 
	theSymbolManager->registerSymbol(ClassDrivenNode);

    iterator.setList(this->nodeList);
    while ( (src = (Node*)iterator.getNext()) ) {
	int i, ocnt = src->getOutputCount();
 	if (editor)
	    editor->selectNode(src,FALSE,FALSE);
	//
	// Don't bother with Transmitters or Receivers 
	//
	const char *srcClassName = src->getClassName();
	if (EqualString(srcClassName,ClassTransmitterNode) ||
	    EqualString(srcClassName,ClassReceiverNode))
	    continue; 
	
	//
	// Follow each output arc to see if we need to cache this output
	//
	for (i=1 ; i<=ocnt ; i++) {		// For each node output
	    boolean cache_output = FALSE; 
	    if (src->isOutputCacheabilityWriteable(i) == FALSE) continue;

	    if (src->isOutputConnected(i)) { // If the output is connected 
		Node *dest; 
		List dests; 
		Network::GetDestinationNodes(src,i,&dests);
		ListIterator destIter(dests); 
		while (!cache_output && (dest = (Node*)destIter.getNext())) {
		    //
		    // If the destination node does not have any outputs,
		    // is a side-effect module or is the Image tool, then 
		    // cache this output (i.e. cache all outputs that
		    // feed modules without any outputs).
		    //
		    if ((dest->getOutputCount() == 0) ||
			EqualString(dest->getClassName(), ClassImageNode) || 	
			dest->getDefinition()->isMDFFlagSIDE_EFFECT()) {
			cache_output = TRUE;
			break;
		    }
		    //
		    // If the destination node is a looping tool, then cache this
		    // output.  Looping tools will request their inputs each time
		    // thru the loop.
		    //
		    if (dest->getDefinition()->isMDFFlagLOOP()) {
			cache_output = TRUE;
			break;
		    }

		    //
		    // If the destination node is a macro, then cache this output.
		    // Some macros run every time but we don't which ones.  It would
		    // be better to turn on caching only preceeding those macros
		    // which are side-effect but we don't have that information.
		    //
		    if (dest->isA(ClassMacroNode)) {
			cache_output = TRUE;
			break;
		    }

		    //
		    // If this output feeds a DrivenNode then cache this output.
		    // Reason:  In spite of the cache setting on the DrivenNode,
		    // the module will check its inputs for changes in order to
		    // determine if it must run.  To allow it to check we had better
		    // cache this output.
		    // But, doesn't the input value affect the behavior of a driven
		    // node?  And what does caching on Receivers mean?
		    //
		    if (dest->isA(driven_sym)) {
			cache_output = TRUE;
			break;
		    }

		    //
		    // If this output feeds a module that takes input from  
		    // other than this module, then cache this output. 
		    //
		    int  k;
		    for (k=1 ; !cache_output && k<=dest->getInputCount() ; k++){
			Ark *a2;
			List *dest_in_arcs = (List*)dest->getInputArks(k);
			ListIterator iter(*dest_in_arcs);
			while ( (a2 = (Ark*)iter.getNext()) ) {
		    	    int paramInd;
			    Node *inode = a2->getSourceNode(paramInd);
			    if (src != inode) { 
				//
				// Cache the output if this input is not 
				// connected to a Receiver that is connect 
				// to the src node directly from the
				// corresponding Transmitter.
				//
				cache_output = 
					!EqualString(inode->getClassName(),
							ClassReceiverNode) ||
					((ReceiverNode*)inode)->
						getUltimateSourceNode() != src;
				break;
			    }
			}
		    }
		}
	    } 
	    //
	    // I moved the cache setting outside the loop.  Meaning: turn off caching
	    // for outputs which aren't connected.
	    //
	    if (cache_output) {
		src->setOutputCacheability(i,OutputFullyCached);
		if (editor)
		    editor->selectNode(src,TRUE,FALSE);
	    } else 
		src->setOutputCacheability(i,OutputNotCached);
	}
    }

}
Decorator* Network::lastObjectParsed = NUL(Decorator*);
//
// Parse the 'decorator' comments in the .net file.
//
//
// Parse the 'resource' comments in the .net file for DynamicResource
// ControlPanel saves state in order to handle resource comments because
// each DynamicResource must belong to a decorator or interactor and it will
// always be the most recently parsed decorator or interactor.
//
boolean Network::parseDecoratorComment (const char *comment,
				const char *filename, int lineno)
{
int items_parsed;
char decoType[128];
char stylename[128];
 
    ASSERT(comment);
 
    if ((strncmp(" decorator",comment,10))&&
	(strncmp(" resource",comment,9))&&
	(strncmp(" annotation",comment,11))&&
	(!strstr(comment, " group:")))
	return FALSE;
 
    // The following is a special case for stateful resource comments.
    // Currently they are associated only with Decorators but that
    // will change.
	if (!strncmp(" resource",comment,9)) {
		if (!lastObjectParsed)
			return FALSE;
		return lastObjectParsed->parseResourceComment (comment, filename, lineno);
	} else if (!strncmp(" annotation", comment, 11)) {
		if (!lastObjectParsed)
			return FALSE;
		return lastObjectParsed->parseComment (comment, filename, lineno);
	}

#if WORKSPACE_PAGES
	if (strstr(comment, " group:"))   {
		int i;
		int count = this->groupManagers->getSize();
		boolean group_comment = FALSE;
		GroupManager *gmgr = NUL(GroupManager*);
		for (i=1; i<=count; i++) {
			gmgr = (GroupManager*)this->groupManagers->getDefinition(i);
			const char *mgr_name = gmgr->getManagerName();
			char buf[128];
			sprintf (buf, " %s group:", mgr_name);
			if (EqualSubstring (buf, comment, strlen(buf))) {
				group_comment = TRUE;
				break;
			}
		}
		if (group_comment)
			return lastObjectParsed->parseComment (comment, filename, lineno);
	}
#endif

 
    //
    // Decorator comments in 3.2 and earlier included only a stylename and not
    // the the pair: stylen name / type name.  You need both to handle a look up
    // in a dictionary of dictionaries (which is what DecoratorStyle is).  Prior
    // to 3.2 stylename was always equal to its dictionary key string and there
    // was only 1 style for each type.
    //
    int junk;
    boolean parsed = FALSE;
    items_parsed =
	sscanf (comment, " decorator %[^\t]\tpos=(%d,%d) size=%dx%d style(%[^)])",
	    stylename, &junk,&junk,&junk,&junk, decoType);
    if (items_parsed == 6) {
	parsed = TRUE;
    } else {
	items_parsed =

// See Decorator::parseComment for an explanation
#ifndef ERASE_ME
	    sscanf (comment, " decorator %[^\t]\tstyle(%[^)]", stylename, decoType);
#else
	    0;
#endif
	if (items_parsed == 2) {
	    parsed = TRUE;
	} else {
	    items_parsed = sscanf(comment, " decorator %[^\t]", stylename);
	    if (items_parsed == 1)
		parsed = TRUE;
	}
    }


	Dictionary *dict;
	DecoratorStyle *ds;
	if (!parsed) {
		ErrorMessage("Unrecognized 'decorator' comment (file %s, line %d)",
			filename, lineno);
		return FALSE;
	} 
	dict = DecoratorStyle::GetDecoratorStyleDictionary (stylename);
	DictionaryIterator di(*dict);

	if (items_parsed == 1) {
		ds = (DecoratorStyle *)di.getNextDefinition();
		if (!ds) {
			ErrorMessage("Unrecognized 'decorator' type (file %s, line %d)",
				filename, lineno);
			return FALSE;
		}
	} else {
		while ( (ds = (DecoratorStyle*)di.getNextDefinition()) ) {
			if (EqualString (decoType, ds->getNameString())) {
				break;
			}
		}
		if (!ds) {
			ErrorMessage("Unrecognized 'decorator' type (file %s, line %d)",
				filename, lineno);

			di.setList(*dict);
			ds = (DecoratorStyle*)di.getNextDefinition();
			if (!ds) return FALSE;
		}
	}

	ASSERT(ds);
	Decorator *d;
	d = ds->createDecorator (TRUE);
	d->setStyle (ds);
	DecoratorInfo* dnd = new DecoratorInfo (this, (void*)this,
		(DragInfoFuncPtr)Network::SetOwner,
		(DragInfoFuncPtr)Network::DeleteSelections,
		(DragInfoFuncPtr)Network::Select);
	d->setDecoratorInfo (dnd);

	if (!d->parseComment (comment, filename, lineno))
		ErrorMessage("Unrecognized 'decorator' comment (file %s, line %d)",
		filename, lineno);

	this->addDecoratorToList ((void *)d);
	lastObjectParsed = d;

	return TRUE;
}

void Network::SetOwner(void *b)
{
Network *netw = (Network*)b;
    netw->setCPSelectionOwner(NUL(ControlPanel*));
}
 
void Network::DeleteSelections(void *b)
{
Network *netw = (Network*)b;
Command *cmd = netw->editor->getDeleteNodeCmd();
 
    cmd->execute();
}

void Network::Select(void *b)
{
Network *netw = (Network*)b;
    if (netw->editor) netw->editor->handleDecoratorStatusChange();
}

//
// Do a recursive depth-first search of this network for macro nodes. 
// and append their MacroDefinitions to the given list.  The list will
// come back in depth-first order and will NOT contain duplicates. 
// This has the side effect of loading the network bodies for all the 
// returned MacroDefinitions.
//
void Network::getReferencedMacros(List *macros, List *visited)
{
    List *l = this->makeClassifiedNodeList(ClassMacroNode, FALSE);

    if (l) {
	MacroNode *mn;
	boolean wasVisitNull;  
	if (!visited) {
	    visited = new List();
	    wasVisitNull = TRUE;  
	} else
	    wasVisitNull = FALSE;  

	ListIterator iter(*l);
	while ( (mn = (MacroNode*)iter.getNext()) ) {
	    MacroDefinition *md = (MacroDefinition*)mn->getDefinition();
	    //
	    // Search all unvisited macros for other macros
	    //
	    if (!visited->isMember((void*)md)) {
		visited->appendElement((void*)md);
		md->loadNetworkBody();
		Network *md_network = md->getNetwork();;
		md_network->getReferencedMacros(macros,visited);
	    } 
	    //
	    // If this macro wasn't found yet, put it on the end of the list. 
	    //
	    if (!macros->isMember((void*)md)) 
		macros->appendElement((void*)md);
	}

	if (wasVisitNull)
	    delete visited;

	delete l;
    }

}
//
// Print comments and 'include' statements for the macros that are 
// referenced in this network.  If nested is TRUE, then we don't print
// the 'include' statements.  Return TRUE on sucess or FALSE on failure.
//
boolean Network::printMacroReferences(FILE *f, boolean inline_define,
			PacketIFCallback echoCallback, void *echoClientData)
			
{

    List *l = this->makeClassifiedNodeList(ClassMacroNode);


    if (l) {

	MacroNode *mn;
	const char *comment_type;

	if (inline_define) 
	    comment_type = "definition";
	else  
	    comment_type = "location ";

	ListIterator iter(*l);	
	List topLevelMacros;
	MacroDefinition *md; 
	while ( (mn = (MacroNode*)iter.getNext()) )  {
	    md = (MacroDefinition*)mn->getDefinition();
	    topLevelMacros.appendElement((void*)md);
	}

	List refMacros;
	this->getReferencedMacros(&refMacros);

	iter.setList(refMacros);	
	while ( (md = (MacroDefinition*)iter.getNext()) ) {
	    boolean top;
    	    char s[1024];
	    md->loadNetworkBody();
	    Network *md_net = md->getNetwork();
	    const char *name = md->getNameString();
	    char *path = (char*)md_net->getFileName();
	    path = GetFullFilePath(path);	
	
	    ASSERT(name && path);

	    if (topLevelMacros.isMember((void*)md)) 
		top = TRUE;
	    else
		top = FALSE;

	    if (inline_define) 
		comment_type = "definition";
	    else if (top)
		comment_type = "reference (direct)";
	    else
		comment_type = "reference (indirect)";

	    SPRINTF(s, 
		"//\n"
//		"// macro reference: %s first referenced (%s) by macro %s\n"
		"// macro %s: %s %s\n", 
//				name, ref_kind,this->getNameString(),
				comment_type, name, path);

	    if (!inline_define && top) {
	        char *basename = GetFileBaseName(path,NULL);
		ASSERT(basename);
		char s2[1024];
		SPRINTF(s2, "include \"%s\"\n", basename);
		strcat(s,s2);
		delete basename;
	    } 
	    delete path;

	    if (fputs(s, f) < 0)
		goto error; 
	    if (echoCallback)
		(*echoCallback)(echoClientData, s);
	    
	    //
	    // Now print the inline definition if requested. 
	    //
	    if (inline_define && !md_net->printNetwork(f, PrintFile))
		goto error;
	}
	delete l;

	if (fputs("//\n", f) < 0)
	    goto error; 
    }

    return TRUE;

error:
    if (l)
	delete l;
    return FALSE;
}

//
// See if the given label is unique among nodes requiring uniqueness.
// Return a node's type name if there is a conflict, 0 otherwise.
// (So that the caller can produce a decent error message)
//
const char * Network::nameConflictExists (UniqueNameNode *passed_in, const char *label)
{
    List* cnl = this->makeClassifiedNodeList (ClassUniqueNameNode);
    if (cnl) {
	ListIterator it(*cnl);
	UniqueNameNode* existing;
	while ( (existing = (UniqueNameNode*)it.getNext()) ) {
	    if ((passed_in != existing) && 
		(passed_in->namesConflict
		    (existing->getUniqueName(), label, existing->getClassName()))) {
		delete cnl;
		return existing->getNameString();
	    }
	}
	delete cnl;
    }

    //
    // Do the loop over again in order to account for DXLOutputs.  They aren't
    // ClassUniqueNameNodes because they're DrivenNodes.
    //
    cnl = this->makeClassifiedNodeList (ClassDXLOutputNode, FALSE);
    if (cnl) {
	ListIterator it(*cnl);
	Node* existing;
	while ( (existing = (Node*)it.getNext()) ) {
	    if (passed_in->namesConflict
		(existing->getLabelString(), label, existing->getClassName())) {
		delete cnl;
		return existing->getNameString();
	    }
	}
	delete cnl;
    }

    return NUL(char*);
}


const char * Network::nameConflictExists (const char *label)
{
    List* cnl = this->makeClassifiedNodeList (ClassUniqueNameNode);
    if (cnl) {
	ListIterator it(*cnl);
	UniqueNameNode* existing;
	while ( (existing = (UniqueNameNode*)it.getNext()) ) {
	    if (existing->namesConflict
		    (existing->getUniqueName(), label, ClassDXLOutputNode)) {
		delete cnl;
		return existing->getNameString();
	    }
	}
	delete cnl;
    }

    return NUL(char*);
}

//
// Called by ReceiverNode during parsing.  Scenario:  an old network has a name
// conflict between Transmitter/Receiver pair on one hand, and Input,Output,of
// DXLinkInput on the other hand.  Such a Receiver would mark the transmitter for
// fixup which must be done after the network has been read in so that connections
// among Transmitter/Reciever(s) aren't lost.  The corollary to marking them for
// fixup is calling renameTransmitters().  That method is called after reading
// is finished.  This code was added because the ui has become more restrictive
// about naming things which use user supplied names in code generation.  This
// lets existing nets which were created before these new restrictions, live on.
//
void Network::fixupTransmitter(TransmitterNode* tn)
{
    if (!Network::RenameTransmitterList)
	Network::RenameTransmitterList = new List;

    if (Network::RenameTransmitterList->isMember(tn)) return ;

    Network::RenameTransmitterList->appendElement((const void*)tn);
}

void Network::renameTransmitters()
{

    if (!Network::RenameTransmitterList) return ;
    int size = Network::RenameTransmitterList->getSize();
    if (!size) return ;

    //
    // Form a reasonable warning message.
    //
    char* msg = new char[(size+1)*256];
    char tbuf[128];
    int offs = 0;
    if (size > 1) {
	if (this->isMacro())
	    sprintf (msg, 
		"The following Transmitters in macro %s\n"
		"were renamed due to name conflicts:", this->getNameString());
	else
	    strcpy(msg, "The following Transmitters were renamed due to name conflicts:");
	offs = strlen(msg);
    }

    ListIterator it(*Network::RenameTransmitterList);
    TransmitterNode* tn;
    while ( (tn = (TransmitterNode*)it.getNext()) ) {
	char new_name[128];
	sprintf (new_name, "%s_xcvr", tn->getLabelString());
	if (size == 1) {
	    if (this->isMacro())
		sprintf (msg, "Transmitter \"%s\" in macro %s was\n"
			      "renamed \"%s\" due to a name conflict.",
		    tn->getLabelString(), this->getNameString(), new_name);
	    else 
		sprintf(msg, 
		    "Transmitter \"%s\" was renamed \"%s\"\ndue to a name conflict.",
		    tn->getLabelString(), new_name);
	} else {
	    sprintf (tbuf, "\n   \"%s\" is now \"%s\"", tn->getLabelString(), new_name);
	    strcpy (&msg[offs], tbuf);
	    offs+= strlen(tbuf);
	}

	tn->setLabelString(new_name);
    }

    this->setDirty();

    this->addEditorMessage(msg);
    delete msg;
    Network::RenameTransmitterList->clear();
}

void Network::addEditorMessage (const char* msg)
{
    if (!this->editorMessages)
	this->editorMessages = new List;
    if (this->editor) {
	InfoMessage (msg);
    } else {
	char* cp = DuplicateString(msg);
	this->editorMessages->appendElement((void*)cp);
    }
}

void Network::showEditorMessages ()
{
    if (!this->editorMessages) return ;

    boolean notify = theDXApplication->appAllowsSavingNetFile(this); 

    ListIterator it(*this->editorMessages);
    char* msg;
    while ( (msg = (char*)it.getNext()) ) {
	if (notify) InfoMessage (msg);
	delete msg;
    }
    this->editorMessages->clear();
}

void Network::addDecoratorToList (void* e)
{
    this->decoratorList.appendElement(e);
    this->setFileDirty();
    if (this->readingNetwork == FALSE);
	this->changeExistanceWork(NUL(Node*), TRUE);
}

void Network::removeDecoratorFromList (void* e)
{
    this->decoratorList.removeElement(e);
    this->setFileDirty();
    this->changeExistanceWork(NUL(Node*), FALSE);
}

void Network::copyGroupInfo (Node* src, Node* dest)
{
    List destList;
    destList.appendElement((void*)dest);
    this->copyGroupInfo(src, &destList);
}

void Network::copyGroupInfo (Node* src, List* destList)
{
    Dictionary* group_mgrs = this->getGroupManagers();
    DictionaryIterator di(*group_mgrs);
    GroupManager* gmgr = NUL(GroupManager*);
    GroupRecord* grec = NUL(GroupRecord*);
    while ( (gmgr = (GroupManager*)di.getNextDefinition()) ) {
	Symbol gmgr_sym = gmgr->getManagerSymbol();
	const char* group_name = src->getGroupName(gmgr_sym);
	ListIterator it(*destList);
	Node* n;
	while ( (n = (Node*)it.getNext()) ) {
	    grec = gmgr->getGroup(group_name);
	    n->setGroupName(grec, gmgr_sym);
	}
    }
}


//
// If any Node in the list has an arc to any Node not in the list,
// then break the arc, replacing it with Transmitter,Receiver.
//
boolean Network::chopArks(List* selected, Dictionary* tmits, Dictionary* rcvrs)
{
int i,j;

    ListIterator it(*selected);
    Node* seln;
    while ( (seln = (Node*)it.getNext()) ) {
	//
	// Chop input arcs
	//
	int count = seln->getInputCount();
	for (i=1; i<=count; i++) {
	    List* orig = (List*)seln->getInputArks(i);
	    if ((orig == NUL(List*)) || (orig->getSize() == 0)) continue;
	    List* conns = orig->dup();
	    int acnt = conns->getSize();
	    ASSERT (acnt == 1);
	    Ark* a = (Ark*)conns->getElement(1);
	    int pno;
	    Node* src = a->getSourceNode(pno);
	    if (selected->isMember(src) == FALSE) 
		this->chopInputArk (seln, i, tmits, rcvrs);
	    delete conns;
	}

	//
	// Chop output arcs
	//
	count = seln->getOutputCount();
	for (i=1; i<=count; i++) {
	    List* orig = (List*)seln->getOutputArks(i);
	    if ((orig == NUL(List*)) || (orig->getSize() == 0)) continue;
	    List* conns = orig->dup();
	    int acnt = conns->getSize();
	    for (j=1; j<=acnt; j++) {
		Ark* a = (Ark*)conns->getElement(j);
		int pno;
		Node* dest = a->getDestinationNode(pno);
		if (selected->isMember(dest) == FALSE) 
		    this->chopInputArk (dest, pno, tmits, rcvrs);
	    }
	    delete conns;
	}
    }
    Node* n;
    DictionaryIterator di;
    di.setList(*rcvrs);
    while ( (n = (Node*)di.getNextDefinition()) )
       this->addNode(n);

    return TRUE;
}

boolean Network::chopInputArk (Node* n, int pno, Dictionary* tmits, Dictionary* rcvrs)
{
int vpex_src, vpey_src;
int vpex_dest, vpey_dest;
char newname[64];
char tmp[64];
int dummy;

    //
    // Receivers have input arcs but they're not visible so
    // we don't chop them.  Transmitters have output arcs which
    // likewise aren't visible so we don't chop them either.
    //
    if ((n->isA(ClassReceiverNode)) && (pno == 1))
	return TRUE;

    NodeDefinition* tmit_nd = (NodeDefinition*)
	theNodeDefinitionDictionary->findDefinition("Transmitter");
    ASSERT(tmit_nd);
    NodeDefinition* rcvr_nd = (NodeDefinition*)
	theNodeDefinitionDictionary->findDefinition("Receiver");
    ASSERT(rcvr_nd);

    ASSERT (pno <= n->getInputCount());
    List* orig = (List*)n->getInputArks(pno);
    if ((orig == NUL(List*)) || (orig->getSize()==0)) return TRUE;
    int acnt = orig->getSize();
    ASSERT (acnt <= 1);

    int src_pno;
    Ark* a = (Ark*)orig->getElement(1);
    Node* src = a->getSourceNode(src_pno);
    delete a;
    src->getVpePosition(&vpex_src, &vpey_src);
    n->getVpePosition(&vpex_dest, &vpey_dest);
    int minx = MIN(vpex_src, vpex_dest);
    int maxx = MAX(vpex_src, vpex_dest);
    int diffx = maxx - minx;
    int diffy = vpey_dest - vpey_src;

    src->getOutputNameString(src_pno, tmp);
    sprintf (newname, "%s_%ld_%s", src->getNameString(), 
	src->getInstanceNumber(), tmp);

    Node* tmit = (Node*)tmits->findDefinition(newname);
    const char* actual_name = newname;

    //
    // If the src/param already has a transmitter
    // then we don't need to make another one.
    //
    if (tmit == NUL(Node*)) {
	List* orig = (List*)src->getOutputArks(src_pno);
	if ((orig) && (orig->getSize())) {
	    ListIterator oi(*orig);
	    Ark* a;
	    while ( (a = (Ark*)oi.getNext()) ) {
		Node* dest = a->getDestinationNode(dummy);
		if (dest->isA(ClassTransmitterNode) == FALSE) continue;
		tmit = dest;
		break;
	    }
	}
    }

    if (tmit == NUL(Node*)) {
	//
	// If the src/param is already a receiver, then we
	// don't need to make a new transmitter.
	//
	if (src->isA(ClassReceiverNode)) {
	    actual_name = src->getLabelString();
	} else {
	    tmit = tmit_nd->createNewNode(this);
	    tmits->addDefinition(newname, tmit);
	    if ((diffy > 0) && (diffy < 140) && (diffx < 180))
		tmit->setVpePosition(vpex_src+100, vpey_src + 80);
	    else
		tmit->setVpePosition(vpex_src, vpey_src + 80);
	    tmit->setLabelString (newname);
	    actual_name = tmit->getLabelString();
	    this->copyGroupInfo(src, tmit);
	    new Ark (src, src_pno, tmit, 1);
	    this->addNode(tmit);
	}
    } else {
	actual_name = tmit->getLabelString();
    }

    Node* rcvr = (Node*)rcvrs->findDefinition(actual_name);
    if (rcvr == NUL(Node*)) {
	rcvr = rcvr_nd->createNewNode(this);
	rcvrs->addDefinition(actual_name, rcvr);
	if ((diffy > 0) && (diffy < 140) && (diffx < 180))
	    rcvr->setVpePosition(vpex_dest+100, MAX(0,vpey_dest-80));
	else 
	    rcvr->setVpePosition(vpex_dest, MAX(0,vpey_dest-80));
	rcvr->setLabelString (actual_name);
	this->copyGroupInfo(src, rcvr);
    }

    new Ark (rcvr, 1, n, pno);

    return TRUE;
}

//
// For each receiver in the list...
//    remove the receiver and replace it with an arc.
//    the source of the arc is found by looking at a 
//    matching transmitter.
//
boolean Network::replaceInputArks(List* selected, List* )
{

//
// Delete nodes outside the loop because we're iterating over the list.
//
List nodes_to_delete;

    if (!selected) return TRUE;
    if (selected->getSize() == 0) return TRUE;
    Dictionary erased_rcvrs;
    Dictionary* group_mgrs = this->getGroupManagers();
    GroupManager* gmgr = (GroupManager*)group_mgrs->findDefinition (PAGE_GROUP);
    Symbol gmgr_sym = gmgr->getManagerSymbol();

    ListIterator it(*selected);
    Node* no;
    while ( (no = (Node*)it.getNext()) ) {
	//
	// Pay attention only to Receiver nodes
	//
	if (no->isA(ClassReceiverNode) == FALSE) continue;
	ReceiverNode* rcvr = (ReceiverNode*)no;

	//
	// The Receiver's input is the Transmitter
	//
	List* orig_in = (List*)rcvr->getInputArks(1);
	if ((orig_in == NUL(List*)) || (orig_in->getSize() == 0)) continue;
	ASSERT(orig_in->getSize() == 1);

	//
	// The Transmitter's input is one node/param we want to find
	// We'll look at both ultimate source and immediate source
	// because ultimate source might be null or on a different page
	// in which case we'll use immediate source.
	//
	int pno;
	Node* ultimate_src = rcvr->getUltimateSourceNode(&pno);
	const char* src_page = NUL(char*);
	const char* rcvr_page = rcvr->getGroupName(gmgr_sym);
	if (ultimate_src) src_page = ultimate_src->getGroupName(gmgr_sym);
	boolean same_page = FALSE;
	if ((rcvr_page == NUL(char*)) && (src_page == NUL(char*)) && (ultimate_src)) {
	    same_page = TRUE;
	} else if ((rcvr_page) && (src_page) && (EqualString(rcvr_page,src_page))) {
	    same_page = TRUE;
	}
	if (same_page == FALSE) {
	    List* ia = (List*)rcvr->getInputArks(1);
	    if ((!ia) || (ia->getSize() < 1)) continue;

	    Node* tmit;
	    Ark* a = (Ark*)ia->getElement(1);
	    int dummy;
	    tmit = a->getSourceNode(dummy);
	    if (!tmit) continue;
	    ia = (List*)tmit->getInputArks(1);
	    if ((!ia) || (ia->getSize() < 1)) continue;
	    a = (Ark*)ia->getElement(1);
	    ultimate_src = a->getSourceNode(pno);
	}
	if (ultimate_src == NUL(Node*)) continue;
	src_page = ultimate_src->getGroupName(gmgr_sym);

	//
	// Loop over the nodes connected to the Receiver in order
	// to attach each one the real source node/param.
	// During the loop, don't delete arcs, just gather them up
	// for later deletion.
	//
	List* orig_out = (List*)rcvr->getOutputArks(1);
	int arc_count = 0;
	if (orig_out) arc_count = orig_out->getSize();
	int i;
	List delete_arcs;
	for (i=1; i<=arc_count; i++) {
	    Ark* rcvr_arc =(Ark*)orig_out->getElement(i);
	    int out_pno = 0;
	    Node* dest = rcvr_arc->getDestinationNode(out_pno);
	    const char* dest_page = dest->getGroupName(gmgr_sym);
	    //
	    // If the page group of ultimate source is the same as the
	    // page group for dest, then we can reconnect.
	    //
	    boolean same_page = FALSE;
	    if ((dest_page == NUL(char*)) && (src_page == NUL(char*))) {
		same_page = TRUE;
	    } else if ((dest_page) && (src_page) && (EqualString(dest_page,src_page))) {
		same_page = TRUE;
	    }
	    if (same_page) 
		delete_arcs.appendElement((void*)rcvr_arc);
	}

	//
	// Go back and delete arcs now.
	//
	ListIterator it(delete_arcs);
	Ark* a;
	while ( (a=(Ark*)it.getNext()) ) {
	    int out_pno = 0;
	    Node* dest = a->getDestinationNode(out_pno);
	    //Node* src = a->getSourceNode(in_pno);
	    delete a;
	    ASSERT (ultimate_src->isA(ClassTransmitterNode) == FALSE);
	    ASSERT (dest->isA(ClassReceiverNode) == FALSE);
	    Ark* new_arc = new Ark (ultimate_src, pno, dest, out_pno);
	    if (this->editor)
		this->editor->notifyArk(new_arc);
	}

	//
	// If the Receiver has no more arcs, then delete it too.
	//
	orig_out = (List*)rcvr->getOutputArks(1);
	if ((orig_out==NUL(List*)) || (orig_out->getSize() == 0)) {
	    const char* label = rcvr->getLabelString();
	    erased_rcvrs.addDefinition(label, (void*)1111);
	    nodes_to_delete.appendElement(rcvr);
	}
    }
    if (nodes_to_delete.getSize() > 0) {
	if (this->editor) 
	    this->editor->removeNodes(&nodes_to_delete);
	else {
	    ListIterator it(nodes_to_delete);
	    Node* n;
	    while ( (n=(Node*)it.getNext()) ) this->deleteNode(n);
	}
	nodes_to_delete.clear();
    }

    //
    // For each label in the erasure dictionary, check corresponding
    // transmitters.  If we have reduced any transmitter's receiver count
    // to 0, then erase the transmitter also.
    //
    int i = 1;
    int size = erased_rcvrs.getSize();
    List* tmits = NUL(List*);
    if (size > 0)
	tmits = this->makeClassifiedNodeList(ClassTransmitterNode, FALSE);
    if (tmits) {
	while (i<=size) {
	    const char* key = erased_rcvrs.getStringKey(i);
	    it.setList(*tmits);
	    Node* tmit;
	    while ( (tmit = (Node*)it.getNext()) ) {
		const char* label = tmit->getLabelString();
		if (EqualString(label, key)) {
		    List* orig = (List*)tmit->getOutputArks(1);
		    if ((!orig) || (orig->getSize() == 0))
			nodes_to_delete.appendElement(tmit);
		}
	    }

	    i++;
	}
	delete tmits;
    }

    if (nodes_to_delete.getSize() > 0) {
	if (this->editor)
	    this->editor->removeNodes(&nodes_to_delete);
	else {
	    ListIterator it(nodes_to_delete);
	    Node* n;
	    while ( (n=(Node*)it.getNext()) ) this->deleteNode(n);
	}
	nodes_to_delete.clear();
    }

    return TRUE;
}

