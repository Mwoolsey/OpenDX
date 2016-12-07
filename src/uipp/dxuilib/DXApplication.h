/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"

#ifndef _DXApplication_h
#define _DXApplication_h

#include <Xm/Xm.h>

#include "IBMApplication.h"
#include "List.h"
#include "Dictionary.h"
#include "DXExecCtl.h"
#include "License.h"

class HelpWin;
class MsgWin;
class ImageWindow;
class ControlPanel;

#define RECENT_NETS "recentNets"

//
// Class name definition:
//
#define ClassDXApplication	"DXApplication"

extern "C" int DXApplication_DXAfterFunction(Display *display);

extern "C" void
#if defined(ALTERNATE_CXX_SIGNAL) 
	DXApplication_HandleCoreDump(int dummy, ... );
#else
	DXApplication_HandleCoreDump(int dummy);
#endif

typedef struct
{
    //
    // options and resources:
    //
    String	server;
    String	executive;
    String	workingDirectory;

    String	program;
    String	cfgfile;
    String	userModules;
    String	macros;
    String      errorPath;

    String	executiveModule;
    String	uiModule;

    Boolean	noWindowPlacement;

    int		port;
    int		memorySize;

    Boolean	echoVersion;
    String	anchorMode;
    Boolean	noAnchorAtStartup;
    Boolean	noConfirmedQuit;
    Boolean	debugMode;
    Boolean	showInstanceNumbers;
    Boolean	runUIOnly;
    Boolean	runLocally;
    Boolean	showHelpMessage;
    Boolean	executeProgram;
    Boolean	executeOnChange;
    Boolean	suppressStartupWindows;
    Boolean	isMetric;
    Boolean	exitAfter;
    Boolean	noExitOptions;
    Boolean	noExecuteMenus;
    Boolean	noConnectionMenus;
    Boolean	noWindowsMenus;

    //
    // Image window image saving/printing control
    //
    String	printImageFormat;
    String	printImagePageSize;	// wxh in inches or cm (see isMetric)
    int		printImageResolution;	// Dots per inch or cm (see isMetric)
    String	printImageSize;		// "8" or "8x" or "x10" or "8x10"
    String	printImageCommand;

    String	saveImageFormat;	
    String	saveImagePageSize;	// wxh in inches or cm (see isMetric)
    int		saveImageResolution;	// Dots per inch or cm (see isMetric)
    String	saveImageSize;		// "8", "8x", "x10" or "8x10" cm or "
 
    //
    // Message Window control
    //
    Boolean	infoEnabled;
    Boolean	warningEnabled;
    Boolean	errorEnabled;
    Boolean	moduleInfoOpensMessage;
    Boolean	infoOpensMessage;
    Boolean	warningOpensMessage;
    Boolean	errorOpensMessage;

    //
    // Configurability level 
    //
    String	restrictionLevel;
    Boolean	useWindowSpecs;
    Boolean	noRWConfig;

    //
    // Panel configurability 
    //
    Boolean	noOpenAllPanels;
    Boolean	noPanelEdit;
    Boolean	noPanelAccess;
    Boolean	noPanelOptions;

    //
    // Interactor/panel  configurability 
    //
    Boolean	noInteractorEdits;
    Boolean	noInteractorAttributes;
    Boolean	noInteractorMovement;

    //
    // Image/Editor window configurability 
    //
    Boolean	noImageMenus;
    Boolean	noImageRWNetFile;
    Boolean	limitedNetFileSelection;
    String	netPath;
    Boolean	noImageLoad;
    Boolean	noEditorAccess;
    Boolean	limitImageOptions;
    Boolean	noImageSaving;
    Boolean	noImagePrinting;
    Boolean     notifySaveNet;
    Boolean     noNetworkExecute;

    //
    // Message window  configurability
    //
    Boolean	noScriptCommands;
    Boolean	noMessageInfoOption;
    Boolean	noMessageWarningOption;
    Boolean	noEditorOnError;

    //
    // ColormapEditor window configurability
    //
    Boolean	noCMapSetNameOption;
    Boolean	noCMapOpenMap;
    Boolean	noCMapSaveMap;


    //
    // Process group defining and host assignments. 
    //
    Boolean	noPGroupAssignment;

    //
    // Global configurability 
    //
    Boolean	noDXHelp;

    String      cryptKey;       // Key used to decrypt the .net and .cfg files
    Boolean     forceNetFileEncryption; // Do we force encryption 
    String      forceFunctionalLicense; // If non-null, 
					// force the given license type 
    Pixel	errorNodeForeground;
    Pixel	executionHighlightForeground;
    Pixel	backgroundExecutionForeground;
    Pixel	foreground;
    Pixel	background;
    Pixel       insensitiveColor;
    Pixel       accentColor;
    Pixel       standInBackground; // This one is shared.

    int		applicationPort;
    String	applicationHost;

    String	oemApplicationName;	// Allow alternate app name if verified 
    String 	oemApplicationNameCode;	// Crypted app name for verification
    String 	oemLicenseCode;		// Used to not require a license 

    String	viewDataFile;		// Data file to use in viewer mode. 

    Boolean 	autoScrollVPEInitVal;	// set to FALSE by default, provides an
					// initial value to EditorWorkSpace for
					// repositioning scrollbars during page changes.
    String	cosmoDir;		// On behalf of java generation
    String	jdkDir;
    String	htmlDir;
    String	dxJarFile;
    String	userHtmlDir;
    String	serverDir;

    //
    // Automatic graph layout
    //
    int		autoLayoutHeight;
    int		autoLayoutGroupSpacing;
    int		autoLayoutNodeSpacing;
} DXResource;


//
// Referenced classes:
//
class Dictionary;
class DXWindow;
class Command;
class CommandScope;
class DXChild;
class DXPacketIF;
class ApplicIF;
class FileDialog;
class Network;
class StartServerDialog;
class LoadMacroDialog;
class LoadMDFDialog;
class OpenNetworkDialog;
class MsgWin;
class DebugWin;
class HelpWin;
class ProcessGroupAssignDialog;
class ProcessGroupManager;
class EditorWindow;

struct DXServerInfo
{
    int 	autoStart;
    char       *server;
    char       *executive;
    char       *workingDirectory;
    char       *userModules; 	// -mdf option
    char       *executiveFlags; // Other flags, as required, for dxexec.
    int		port;
    int		memorySize;

    List    	children;
    List	queuedPackets;
    DXPacketIF *packet;
};

//
// DXApplication class definition:
//				
class DXApplication : public IBMApplication
{
    friend class DXExecCtl;
    friend class DXPacketIF;
    friend class DXLinkHandler;

  private:
    //
    // Private class data:
    //
    static boolean    DXApplicationClassInitialized; // class initialized?
    static void 	DebugEchoCallback(void *clientData, char *echoString);
    static void       QueuedPacketAccept(void *data);
    static void       QueuedPacketCancel(void *data);
    
    static void InitializeSignals();
    friend void
#if defined(ALTERNATE_CXX_SIGNAL) 
    DXApplication_HandleCoreDump(int dummy, ... );
#else
    DXApplication_HandleCoreDump(int dummy);
#endif


    //
    // License manager stuff
    //
    static void       LicenseMessage(void *, int, void *);

    //
    // Dumped object list.
    //
    List	dumpedObjects;
    
    //
    // Should we do a disconnect from the executive next time through the 
    // handleEvents.
    //
    boolean 	serverDisconnectScheduled;

    //
    // Private member data:
    //
    boolean runApplication;	// continue to run the application?

    //
    // Define a mapping of integer levels and the restriction level names and
    // determine if the current restriction level is at the given level.
    // The highest level of restriction is 1 the lowest and +infinity.
    //
    boolean isRestrictionLevel(int level);

    //
    // Called if DIAGNOSTICS is defined
    //
    friend int DXApplication_DXAfterFunction(Display *display);

    //
    // Have we read the first network yet.
    //
    boolean readFirstNetwork;

    //
    // Save all dirty files and return a message string
    //
    void emergencySave (char *msg);

    //
    // Check the oemApplicationName and oemApplicationNameCode resources and see
    // if an alternate application name can be verified.  If so, return it
    // otherwise return NULL.  The return value of the effects the return
    // values of getFormalName() and getInformalName().
    //
    const char *getOEMApplicationName();

    //
    // See if there is an OEM license code (see oemApplicationName and
    // oemLicenseCode resources) and if so see if it is valid.
    // If so return TRUE and the type of functional license (rt/dev) that
    // was granted to the oem.
    //
    boolean verifyOEMLicenseCode(LicenseTypeEnum *);

#if defined(BETA_VERSION) && defined(NOTDEF)
    //
    // Exit if this is a beta version and its old
    //
    void betaTimeoutCheck();
#endif

  protected:
    //
    // Protected class data:
    //
    static DXResource resource;	// resources and options

    static String ListValuedSettings[];

    //
    // Protected member data:
    //
    DXWindow	       *anchor;		// anchor window
    CommandScope       *commandScope;	// command scope
    DXServerInfo	serverInfo;
    DXExecCtl		execCtl;
#ifdef HAS_CLIPNOTIFY_EXTENSION
    boolean		hasClipnotifyExtension;
    int			clipnotifyEventCode;
#endif

    FileDialog         *openFileDialog;

    ApplicIF	       *applicationPacket;

    //
    // Overrides the Application class version:
    //   Initializes Xt Intrinsics with option list (switches).
    //
    virtual boolean initialize(unsigned int* argcp,
			    char**        argv);

    //
    // Override of superclass handleEvents() function:
    //   Handles application events.
    //   This routine should be called by main() only.
    //
    virtual void handleEvents();
    virtual void handleEvent(XEvent *xev);

    //
    // If we're using an anchor window with its own copyright then be satisfied
    // with that.  Otherwise use the superclass method.
    virtual void postCopyrightNotice();

    void highlightNodes(char* path, int highlightType);

    StartServerDialog 	*startServerDialog;
    OpenNetworkDialog	*openNetworkDialog;
    LoadMacroDialog	*loadMacroDialog;
    LoadMDFDialog	*loadMDFDialog;
    MsgWin		*messageWindow;

    LicenseTypeEnum	appLicenseType;		// nodelocked or concurrent
    LicenseTypeEnum	funcLicenseType;	// run-time or development

    //
    // The list of errors for the current (or most recent) execution.
    List errorList;

    //
    // Read the module description file into the Node Definition Dictionary.
    //
    virtual void loadMDF();
    //
    // Read the interactor description file into the Node Definition Dictionary.
    //
    virtual void loadIDF();

    //
    // Destroy the objects that have been dumped.
    //
    void destroyDumpedObjects();

    virtual void clearErrorList(void);
    virtual void addErrorList(char *line);

    //
    // Manage the software licenses 
    //
    virtual void determineUILicense(LicenseTypeEnum *appLicense,
				    LicenseTypeEnum *funcLicense);

    virtual boolean	    verifyServerLicense();

    //
    // Disconnect from Exec next time through the main loop
    //
    void		scheduleServerDisconnect();

    //
    // Send the command to the flush the dictionaries.
    //
    virtual void flushExecutiveDictionaries(DXPacketIF *pif);

    //
    // Handle cancelation of (disconnection from) a packet interface. 
    //
    virtual void packetIFCancel(DXPacketIF *p);
    //
    // Handle acceptance of (connection to) a packet interface. 
    //
    virtual void packetIFAccept(DXPacketIF *p);

    //
    //  Get the functional license that was asked for by the user.
    //
    LicenseTypeEnum getForcedFunctionalLicenseEnum();

    //
    // Install the default resources for this class and then call the
    // same super class method to get the default resources from the
    // super classes.
    //
    virtual void installDefaultResources(Widget baseWidget);

  public:
    //
    // Public static data:
    //
    static Symbol MsgExecute;
    static Symbol MsgStandBy;
    static Symbol MsgExecuteDone;
    static Symbol MsgServerDisconnected;
    static Symbol MsgPanelChanged;

    //
    // The input pointer is a pointer to an Application. 
    //
#if 0
    static void       ShutdownApplication(void *);
#endif

    //
    // Public member data:
    //
    Network	       *network;	// Main network
    List		macroList;	// list of user macros 
    Command*		quitCmd;	// DX application Quit command
    Command*            exitCmd;        // DX application Quit command with
                                        // checking if macros modified
    Command*		openFileCmd;	
    Command*		loadMacroCmd;	
    Command*		executeOnceCmd;
    Command*		executeOnChangeCmd;
    Command*		endExecutionCmd;
    Command*		connectedToServerCmd;
    Command*		disconnectedFromServerCmd;
    Command*		connectToServerCmd;
    Command*		resetServerCmd;
    Command*		disconnectFromServerCmd;
    Command*            messageWindowCmd;
    Command*            openSequencerCmd;
    Command*            openAllColormapCmd;
    Command*		assignProcessGroupCmd;


    //
    // Used to do autoactivation/deactivation of commands that register
    // themselves with this command.
    //
    Command*		executingCmd;
    Command*		notExecutingCmd;

#if USE_REMAP	
    Command*            toggleRemapInteractorsCmd;
#endif
    Command*            loadMDFCmd;
    Command*            toggleInfoEnable;
    Command*            toggleWarningEnable;
    Command*            toggleErrorEnable;

#if !defined(WORKSPACE_PAGES)
    ProcessGroupManager *PGManager;
#endif

    ProcessGroupAssignDialog *processGroupAssignDialog;

    //
    // Constructor:
    //
    DXApplication(char* className);

    //
    // Destructor:
    //
    ~DXApplication();

    void notifyPanelChanged();

    //
    // Add dumped object to the list so as to delete it later.
    //
    void dumpObject(Base *object);

    //
    // return the CommandScope.
    //
    CommandScope *getCommandScope() { return this->commandScope; }

    //
    // Access routines:
    //
    DXWindow* getAnchor()
    {
	return this->anchor;
    }
    void shutdownApplication(); 

    //
    // Functions to manage the server connection.
    int connectToServer(int port, DXChild *c = NULL);
    boolean disconnectFromServer();
    void closeConnection(DXChild *c);
    void completeConnection(DXChild *c);
    DXChild *startServer();
    void setServer(char *server);

    DXPacketIF *getPacketIF() { return this->serverInfo.packet; }
    DXExecCtl  *getExecCtl()  { return &this->execCtl; }

    virtual boolean openFile(const char *netfile, 
				const char *cfgfile = NULL,
				boolean resetTheServer = TRUE);

    virtual boolean saveFile(const char *netfile, 
				boolean force = FALSE);

    virtual boolean resetServer(void);
    boolean postStartServerDialog();
    void postProcessGroupAssignDialog();
    void postOpenNetworkDialog();
    void postLoadMDFDialog();
    void postLoadMacroDialog();
    void getServerParameters(int 	 *autoStart,
			     const char **server,
			     const char **executive,
			     const char **workingDirectory,
			     const char **executiveFlags,
			     int	 *port,
			     int	 *memorySize);
    void setServerParameters(int 	 autoStart,
			     const char *server,
			     const char *executive,
			     const char *workingDirectory,
			     const char *executiveFlags,
			     int	 port,
			     int	 memorySize);
    //
    // Functions to manage the application connection
    //
    virtual void connectToApplication(const char *host, const int port);
    virtual void disconnectFromApplication(boolean terminate);

    //
    // Pre-allocate required ui colors
    //
    //const String *DXAllocateColors(XtResource*, int);

    Pixel getAccentColor()
    {
	return this->resource.accentColor;
    }
    Pixel getStandInBackground()
    {
	return this->resource.standInBackground;
    }
    Pixel getErrorNodeForeground()
    {
	return this->resource.errorNodeForeground;
    }
    Pixel getExecutionHighlightForeground()
    {
	return this->resource.executionHighlightForeground;
    }
    Pixel getBackgroundExecutionForeground()
    {
	return this->resource.backgroundExecutionForeground;
    }
    Pixel getForeground()
    {
	return this->resource.foreground;
    }
    Pixel getInsensitiveColor()
    {
        return this->resource.insensitiveColor;
    }
    boolean inDebugMode() { return this->resource.debugMode; }

    //
    // I added trace on/off buttons to the msgwin and I want to be able to see
    // the instance numbers in the vpe conditionally as well.
    //
    boolean showInstanceNumbers() 
	{ return (this->resource.debugMode || this->resource.showInstanceNumbers); }
    void showInstanceNumbers(boolean on_or_off);

    boolean inEditMode();
    boolean inImageMode();
    boolean inMenuBarMode();

    boolean inDataViewerMode();		// Object viewer, requires ui/viewer.net
    const char *getDataViewerImportFile();  // Data file to view	

    boolean isMetricUnits()
    {
	return this->resource.isMetric;
    }

    MsgWin *getMessageWindow()
    {
	return this->messageWindow;
    }
    const char *getLimitedNetPath()
    {
	return this->resource.netPath;
    }

    //
    // Get the default image printing/saving options specified through
    // command line options or resources.
    //
    String      getPrintImageFormat();
    String      getPrintImageSize();
    String      getPrintImagePageSize();
    int         getPrintImageResolution();
    String      getPrintImageCommand();
    String      getSaveImageFormat();
    String      getSaveImageSize();
    String      getSaveImagePageSize();
    int         getSaveImageResolution();

    //
    // Create a new network editor.  
    // This particular implementation makes the returned editor an anchor
    // if this->anchor is NULL.
    // This may return NULL in which case a message dialog is posted. 
    //
    virtual EditorWindow *newNetworkEditor(Network *n);

    virtual Network *newNetwork(boolean nonJava=FALSE);
    virtual MsgWin *newMsgWin();
    virtual ImageWindow *newImageWindow(Network *n);
    virtual ControlPanel *newControlPanel(Network *n);



    //
    // Mark all the networks owned by this application as dirty.
    //
    void markNetworksDirty(void);

    boolean	isInfoEnabled();
    boolean	isWarningEnabled();
    boolean	isErrorEnabled();
    boolean	doesInfoOpenMessage(boolean fromModule = FALSE);
    boolean	doesWarningOpenMessage();
    boolean	doesErrorOpenMessage();
    int 	doesErrorOpenVpe(Network*);
    void	enableInfo(boolean enable);
    void	enableWarning(boolean enable);
    void	enableError(boolean enable);

    //
    // constants returned by doesErrorOpenVpe()
    //
    enum {
	DontOpenVpe = 0,
	MayOpenVpe  = 1,
	MustOpenVpe = 2
    };

    //
    // Read the user's description file into the Node Definition Dictionary.
    // If dict is not null then fill the given dictionary with the 
    // NodeDefintions found in the given MDF. 
    // If uiLoadedOnly is TRUE, then the user has asked to have these loaded
    // from somewhere other than the command line (in which case the 
    // exec does not know about them yet).
    //
    virtual void loadUDF(const char *fileName, Dictionary *dict = NULL,
				boolean uiLoadedOnly = FALSE);

    //
    // Get selected NodeDefinitoins from the newdefs dictionary and send 
    // their representative MDF specs to the server if they are
    // 	1) outboard 
    // 	2) dynamically loaded 
    // 	3) outboardness or dynmacity changes compared to the possibly 
    //	    existing definition in the current dictionary if provided. 
    //
    void sendNewMDFToServer(Dictionary *newdefs, Dictionary *current = NULL);

    //
    // Define a set of methods that indicate a level of UI capability. 
    //
    virtual boolean appAllowsDXHelp();
    virtual boolean appSuppressesStartupWindows();
    virtual boolean appLimitsNetFileSelection();

    virtual boolean appAllowsPanelEdit();
    virtual boolean appAllowsRWConfig();
    virtual boolean appAllowsPanelAccess();
    virtual boolean appAllowsOpenAllPanels();
    virtual boolean appAllowsPanelOptions();

    virtual boolean appAllowsInteractorEdits();
    virtual boolean appAllowsInteractorSelection();
    virtual boolean appAllowsInteractorMovement();
    virtual boolean appAllowsInteractorAttributeChange();

    virtual boolean appAllowsImageRWNetFile();
    virtual boolean appAllowsSavingNetFile(Network *n = NULL);
    virtual boolean appAllowsSavingCfgFile();
    virtual boolean appAllowsImageLoad();
    virtual boolean appAllowsImageSaving();
    virtual boolean appAllowsImagePrinting();
    virtual boolean appLimitsImageOptions();

    virtual boolean appAllowsEditorAccess();
    virtual boolean appAllowsPGroupAssignmentChange();

    virtual boolean appAllowsMessageInfoOption();
    virtual boolean appAllowsMessageWarningOption();
    virtual boolean appAllowsScriptCommands();
    
    virtual boolean appAllowsCMapSetName();
    virtual boolean appAllowsCMapOpenMap();
    virtual boolean appAllowsCMapSaveMap();
    virtual boolean appForcesNetFileEncryption();
    boolean appAllowsExitOptions();
    boolean appAllowsExecuteMenus();
    boolean appAllowsConnectionMenus();
    boolean appAllowsWindowsMenus();
    boolean appAllowsImageMenus();
    boolean appAllowsConfirmedQuit();

    boolean dxlAppNotifySaveNet()
    { return DXApplication::resource.notifySaveNet; }

    boolean dxlAppNoNetworkExecute()
    { return DXApplication::resource.noNetworkExecute; }





    //
    // Return the name of the application (i.e. 'Data Explorer',
    // 'Data Prompter', 'Medical Visualizer'...).
    //
    virtual const char *getInformalName();

    //
    // Return the formal name of the application (i.e. 
    // 'Open Visualization Data Explorer', 'Open Visualization Data Prompter'...)
    //
    virtual const char *getFormalName();

    //
    // Get the applications copyright notice, for example...
    // "Copyright International Business Machines Corporation 1991-1993
    // All rights reserved"
    //
    virtual const char *getCopyrightNotice();

    //
    // Go through the error list and highlight all nodes that got errors.
    //
    void refreshErrorIndicators();

    virtual boolean printComment(FILE *f);
    virtual boolean parseComment(const char *line, const char *filename, 
					int lineno);

    //
    // Return TRUE if the DXWindows are supposed to use the window placement
    // information saved in the .net or .cfg files.
    //
    virtual boolean applyWindowPlacements();

    virtual const char* getCryptKey() { return this->resource.cryptKey; }

    const char* getCosmoDir() { return this->resource.cosmoDir; }
    const char* getJdkDir() { return this->resource.jdkDir; }
    const char* getHtmlDir() { return this->resource.htmlDir; }
    const char* getServerDir() { return this->resource.serverDir; }
    const char* getDxJarFile() { return this->resource.dxJarFile; }
    const char* getUserHtmlDir() { return this->resource.userHtmlDir; }

    //
    // Should we force the functional license to be that returned by
    // getForcedFunctionalLicenseEnum(). 
    //
    virtual boolean isFunctionalLicenseForced();

    //
    // Get the functional license that was granted.
    //
    LicenseTypeEnum getFunctionalLicenseEnum();

    //
    // Do an emergencySave() and then call the super class method 
    //
    virtual void abortApplication();

    boolean getAutoScrollInitialValue() { return this->resource.autoScrollVPEInitVal; }

#if WORKSPACE_PAGES
    ProcessGroupManager *getProcessGroupManager();
#endif

    ApplicIF            *getAppConnection() { return this->applicationPacket; }

    void appendReferencedFile (const char* file);
    void removeReferencedFile (const char* file);
    void getRecentNets(List& result);


    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassDXApplication;
    }
};


extern DXApplication* theDXApplication;

#endif /*  _DXApplication_h */
