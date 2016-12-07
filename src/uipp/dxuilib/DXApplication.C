/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"

// putenv should come from stdlib.h
// extern "C" int putenv(char*);

#if defined(HAVE_IOSTREAM)
#  include <iostream>
#else
#  if defined(HAVE_IOSTREAM_H)
#  include <iostream.h>
#  endif
#  if defined(HAVE_STREAM_H)
#  include <stream.h>
#  endif
#endif

#if defined(HAVE_IO_H)
#include <io.h>
#endif

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#if defined(HAVE_SIGNAL_H)
#include <signal.h>
#endif

#if defined(HAVE_ERRNO_H)
#include <errno.h>
#endif

#if defined(HAVE_CTYPE_H)
#include <ctype.h>
#endif

#if defined(HAVE_SYS_TYPES_H)
#include <sys/types.h>
#endif

#if defined(HAVE_SYS_STAT_H)
#include <sys/stat.h>
#endif

#if defined(HAVE_DIRECT_H)
#include <direct.h>
#define chdir _chdir
#endif


#include "lex.h"
#include "DXVersion.h"
#include "DXApplication.h"
#include "Network.h"
#include "JavaNet.h"
#include "Client.h"
#include "GraphLayout.h"

#include <Xm/Label.h>
#include <X11/cursorfont.h>


#include "oem.h"
#include "CommandScope.h"
#include "NoUndoDXAppCommand.h"
#include "ConfirmedQuitCommand.h"
#include "ConfirmedExitCommand.h"
#include "NoOpCommand.h"
#include "OpenNetworkDialog.h"
#include "OpenCommand.h"
#include "DisconnectFromServerCommand.h"
#include "LoadMacroDialog.h"
#include "LoadMDFDialog.h"
#include "SaveAsDialog.h"
#include "ToolSelector.h"
#include "DXAnchorWindow.h"

#include "../widgets/findcolor.h"

#include "Dictionary.h"
#include "DictionaryIterator.h"
#include "DXChild.h"
#include "DXExecCtl.h"
#include "DXPacketIF.h"
#include "ApplicIF.h"
#include "EditorWindow.h"
#include "HelpWin.h"
#include "ImageWindow.h"
#include "InfoDialogManager.h"
#include "License.h"
#include "List.h"
#include "ListIterator.h"
#include "QuestionDialogManager.h"
#include "ErrorDialogManager.h"
#include "NDAllocatorDictionary.h"
#include "SIAllocatorDictionary.h"
#include "CDBAllocatorDictionary.h"
#include "ParseMDF.h"
#include "InteractorStyle.h"	// for BuildtheInteractorStyleDictionary()
#include "DecoratorStyle.h"	// for BuildtheDecoratorStyleDictionary()
#include "StandIn.h"
#include "FileDialog.h"
#include "StartServerDialog.h"
#include "MacroDefinition.h"
#include "MsgWin.h"
#include "XHandler.h"
#include "ProcessGroupAssignDialog.h"
#include "ProcessGroupManager.h"
#include "WarningDialogManager.h"
#include "Node.h"
#include "ResourceManager.h"

#ifdef HAS_CLIPNOTIFY_EXTENSION
#include "../widgets/clipnotify.h"
#endif
#if (XmVersion > 1001)
#include "../widgets/Number.h"
#endif


#ifdef DXD_WIN
#define  chdir  _chdir
#define  read _read
#endif
//
// Defines the number of minutes the timed license lasts.
//
#ifdef DEBUG
# define LICENSED_MINUTES       1
#else
# define LICENSED_MINUTES       5
#endif

//
// make the CR key work on the aviion: mapping Return key to Enter key.
//
#if (XmVersion > 1001)
#define XK_MISCELLANY
#include <X11/keysym.h>
#endif


//
// The options that can be used with -forceLicense (dxui) and -license (dx)
// options (.i.e. dx -license runtime) .
//
#define LIC_RT_OPTION "runtime"
#define LIC_DEV_OPTION "develop"
#define LIC_TIMED_OPTION "timed"
#define LIC_VIEWER_OPTION "viewer"

DXApplication* theDXApplication = NUL(DXApplication*);
extern "C" void InstallShutdownTimer(Widget w, 
				XtPointer clientData, XtPointer calldata);

boolean    DXApplication::DXApplicationClassInitialized = FALSE;
DXResource DXApplication::resource;

Symbol DXApplication::MsgExecute = 0;
Symbol DXApplication::MsgStandBy = 0;
Symbol DXApplication::MsgExecuteDone = 0;
Symbol DXApplication::MsgServerDisconnected = 0;
Symbol DXApplication::MsgPanelChanged = 0;

#define IMAGE_ANCHOR_MODE 	"IMAGE"
#define EDIT_ANCHOR_MODE 	"EDIT"
#define MENUBAR_ANCHOR_MODE 	"MENUBAR"

// Resources below
#if 1
static
XrmOptionDescRec _DXOptionList[] =
{
    {
	"-directory",
	"*directory",
	XrmoptionSepArg,
	NULL
    },
    {
	"-edit",
	"*anchorMode",
	XrmoptionNoArg,
 	EDIT_ANCHOR_MODE	
    },
    {
	"-exec",
	"*executive",
	XrmoptionSepArg,
	NULL
    },
    {
	"-execute",
	"*executeProgram",
	XrmoptionNoArg,
	"True"
    },
    {
	"-execute_on_change",
	"*executeOnChange",
	XrmoptionNoArg,
	"True"
    },
    {
	"-help",
	"*printHelpMessage",
	XrmoptionNoArg,
	"True"
    },
    {
	"-host",
	"*host",
	XrmoptionSepArg,
	NULL
    },
    {
	"-image",
	"*anchorMode",
	XrmoptionNoArg,
	IMAGE_ANCHOR_MODE
    },
    {
	"-kiosk",
	"*anchorMode",
	XrmoptionNoArg,
	MENUBAR_ANCHOR_MODE
    },
    {
	"-menubar",
	"*anchorMode",
	XrmoptionNoArg,
	MENUBAR_ANCHOR_MODE
    },
    {
	"-noAnchorAtStartup",
	"*noAnchorAtStartup",
	XrmoptionNoArg,
	"True"
    },
    {
	"-noConfirmedQuit",
	"*noConfirmedQuit",
	XrmoptionNoArg,
	"True"
    },
    {
	"-local",
	"*runLocally",
	XrmoptionNoArg,
	"True"
    },
    {
	"-macros",
	"*macros",
	XrmoptionSepArg,
	NULL
    },
    {
	"-mdf",
	"*userModuleDescriptionFile",
	XrmoptionSepArg,
	NULL
    },
    {
	"-metric",
	"*metric",
	XrmoptionNoArg,
	"True"
    },
    {
	"-dxmdf",
	"*executiveModuleDescriptionFile",
	XrmoptionSepArg,
	NULL
    },
    {
	"-uimdf",
	"*uiModuleDescriptionFile",
	XrmoptionSepArg,
	NULL
    },
    {
	"-memory",
	"*memory",
	XrmoptionSepArg,
	NULL
    },
    {
	"-port",
	"*port",
	XrmoptionSepArg,
	NULL
    },
    {
        "-printImageCommand",
        "*printImageCommand",
        XrmoptionSepArg,
        NULL
    },
    {
        "-printImageFormat",
        "*printImageFormat",
        XrmoptionSepArg,
        NULL
    },
    {
        "-printImagePageSize",
        "*printImagePageSize",
        XrmoptionSepArg,
        NULL
    },
    {
        "-printImageSize",
        "*printImageSize",
        XrmoptionSepArg,
        NULL
    },
    {
        "-printImageResolution",
        "*printImageResolution",
        XrmoptionSepArg,
        NULL
    },
    {
	"-program",
	"*program",
	XrmoptionSepArg,
	NULL
    },
    {
	"-cfg",
	"*cfg",
	XrmoptionSepArg,
	NULL
    },
    {
        "-saveImageFormat",
        "*saveImageFormat",
        XrmoptionSepArg,
        NULL
    },
    {
        "-saveImagePageSize",
        "*saveImagePageSize",
        XrmoptionSepArg,
        NULL
    },
    {
        "-saveImageSize",
        "*saveImageSize",
        XrmoptionSepArg,
        NULL
    },
    {
        "-saveImageResolution",
        "*saveImageResolution",
        XrmoptionSepArg,
        NULL
    },
    {
	"-suppress",
	"*suppressStartupWindows",
	XrmoptionNoArg,
	"True"
    },
    {
	"-uidebug",
	"*debugMode",
	XrmoptionNoArg,
	"True"
    },
    {
	"-showInstanceNumbers",
	"*showInstanceNumbers",
	XrmoptionNoArg,
	"True"
    },
    {
	"-uionly",
	"*runUIOnly",
	XrmoptionNoArg,
	"True"
    },
    {
	"-uimessages",
	"*messages",
	XrmoptionSepArg,
	NULL
    },
    {
	"-version",
	"*DXVersion",
	XrmoptionNoArg,
	"True"
    },

    //
    // Backdoor switches:
    //
    {
	"-restrictionLevel",
	"*restrictionLevel",
	XrmoptionSepArg,
	NULL
    },
    {
	"-appHost",
	"*applicationHost",
	XrmoptionSepArg,
	NULL,
    },
    {
	"-appPort",
	"*applicationPort",
	XrmoptionSepArg,
	NULL,
    },
    {
	"-noDXHelp",
	"*noDXHelp",
	XrmoptionNoArg,
	"True"
    },
    {
	"-noEditorAccess",
	"*noEditorAccess",
	XrmoptionNoArg,
	"True"
    },
    {
	"-notifySaveNet",
	"*notifySaveNet",
	XrmoptionNoArg,
	"True"
    },
    {
	"-noNetworkExecute",
	"*noNetworkExecute",
	XrmoptionNoArg,
	"True"
    },
    {
	"-noImageRWNetFile",
	"*noImageRWNetFile",
	XrmoptionNoArg,
	"True"
    },
    {
	"-noImageLoad",
	"*noImageLoad",
	XrmoptionNoArg,
	"True"
    },
    {
	"-limitImageOptions",
	"*limitImageOptions",
	XrmoptionNoArg,
	"True"
    },
    {
	"-limitedNetFileSelection",// Used to be limitedImageFileSelection
	"*limitedNetFileSelection",
	XrmoptionNoArg,
	"True"
    },
    {
	"-noNetFileSelection",
	"*noNetFileSelection",
	XrmoptionNoArg,
	"True"
    },
    {
	"-noImageSaving",
	"*noImageSaving",
	XrmoptionNoArg,
	"True"
    },
    {
	"-noImagePrinting",
	"*noImagePrinting",
	XrmoptionNoArg,
	"True"
    },
    {
	"-noInteractorEdits",
	"*noInteractorEdits",
	XrmoptionNoArg,
	"True"
    },
    {
	"-noInteractorAttributes",
	"*noInteractorAttributes",
	XrmoptionNoArg,
	"True"
    },
    {
	"-noInteractorMovement",
	"*noInteractorMovement",
	XrmoptionNoArg,
	"True"
    },
    {
	"-noOpenAllPanels",
	"*noOpenAllPanels",
	XrmoptionNoArg,
	"True"
    },
    {
	"-noPanelAccess",
	"*noPanelAccess",
	XrmoptionNoArg,
	"True"
    },
    {
	"-noPanelOptions",
	"*noPanelOptions",
	XrmoptionNoArg,
	"True"
    },
    {
	"-noPanelEdit",
	"*noPanelEdit",
	XrmoptionNoArg,
	"True"
    },
    {
	"-noRWConfig",
	"*noRWConfig",
	XrmoptionNoArg,
	"True"
    },
    {
	"-noScriptCommands",
	"*noScriptCommands",
	XrmoptionNoArg,
	"True"
    },
    {
	"-noMessageInfoOption",
	"*noMessageInfoOption",
	XrmoptionNoArg,
	"True"
    },
    {
	"-noMessageWarningOption",
	"*noMessageWarningOption",
	XrmoptionNoArg,
	"True"
    },
    {
	"-noEditorOnError",
	"*noEditorOnError",
	XrmoptionNoArg,
	"True"
    },
    {
	"-noCMapSetNameOption",
	"*noCMapSetNameOption",
	XrmoptionNoArg,
	"True"
    },
    {
	"-noCMapSaveMap",
	"*noCMapSaveMap",
	XrmoptionNoArg,
	"True"
    },
    {
	"-noWindowPlacement",
	"*noWindowPlacement",
	XrmoptionNoArg,
	"True"
    },
    {
	"-noCMapOpenMap",
	"*noCMapOpenMap",
	XrmoptionNoArg,
	"True"
    },
    {
	"-netPath",
	"*netPath",
	XrmoptionSepArg,
	NULL,
    },
    {
	"-noPGroupAssignment",
	"*noPGroupAssignment",
	XrmoptionNoArg,
	"True"
    },
    {
	"-warning",
	"*warningEnabled",
	XrmoptionNoArg,
	"False"
    },
    {
	"-info",
	"*infoEnabled",
	XrmoptionNoArg,
	"False"
    },
    {
	"-error",
	"*errorEnabled",
	XrmoptionNoArg,
	"False"
    },
    {
	"-forceNetFileEncryption",
	"*forceNetFileEncryption",
	XrmoptionSepArg,
	NULL	
    },
    {
	"-cryptKey",
	"*cryptKey",
	XrmoptionSepArg,
	NULL	
    },
    {
	"-exitAfter",
	"*exitAfter",
	XrmoptionNoArg,
	"True"
    },
    {
	"-forceLicense",
	"*forceLicense",
	XrmoptionSepArg,
	NULL,	
    },
    {
	"-noExecuteMenus",
	"*noExecuteMenus",
	XrmoptionNoArg,
	"True",	
    },
    {
	"-noConnectionMenus",
	"*noConnectionMenus",
	XrmoptionNoArg,
	"True",	
    },
    {
	"-noWindowsMenus",
	"*noWindowsMenus",
	XrmoptionNoArg,
	"True",	
    },
    {
	"-noExitOptions",
	"*noExitOptions",
	XrmoptionNoArg,
	"True",	
    },
    {
	"-noImageMenus",
	"*noImageMenus",
	XrmoptionNoArg,
	"True",	
    },
    {
	"-view",
	"*viewDataFile",
	XrmoptionSepArg,
	NULL,
     },
     {
       "-noAutoScrollVPE",
       "*autoScrollVPE",
       XrmoptionNoArg,
       "False"
     },
     {
       "-autoLayoutHeight",
       "*autoLayoutHeight",
       XrmoptionSepArg,
       NULL
     },
     {
       "-autoLayoutGroupSpacing",
       "*autoLayoutGroupSpacing",
       XrmoptionSepArg,
       NULL
     },
     {
       "-autoLayoutNodeSpacing",
       "*autoLayoutNodeSpacing",
       XrmoptionSepArg,
       NULL
     },
};

static
XtResource _DXResourceList[] =
{
    {
        "standInBackground",
        "StandInBackground",
        XmRPixel,
        sizeof(Pixel),
	XtOffset(DXResource*, standInBackground),
        XmRImmediate,
	(XtPointer)"#5F9EA0" // CadetBlue 
    },
    {
        "executionHighlightForeground",
        "Foreground",
        XmRPixel,
        sizeof(Pixel),
	XtOffset(DXResource*, executionHighlightForeground),
        XmRImmediate,
	(XtPointer)"#00ff7e"
    },
    {
        "backgroundExecutionForeground",
        "Foreground",
        XmRPixel,
        sizeof(Pixel),
	XtOffset(DXResource*, backgroundExecutionForeground),
        XmRImmediate,
	(XtPointer)"#7e7eb4"
    },
    {
        "errorHighlightForeground",
        "Foreground",
        XmRPixel,
        sizeof(Pixel),
	XtOffset(DXResource*, errorNodeForeground),
        XmRImmediate,
	(XtPointer)"#ff9b00"
    },
    {
        "foreground",
        "Foreground",
        XmRPixel,
        sizeof(Pixel),
	XtOffset(DXResource*, foreground),
        XmRImmediate,
	(XtPointer)"Black"
    },
    {
        "background",
        "Background",
        XmRPixel,
        sizeof(Pixel),
	XtOffset(DXResource*, background),
        XmRImmediate,
	(XtPointer)"#b4b4b4"
    },
    {
        "InsensitiveColor",
        "Color",
        XmRPixel,
        sizeof(Pixel),
        XtOffset(DXResource*, insensitiveColor),
        XmRString,
        (XtPointer)"#888888"
    },
    {
	"anchorMode",
	"AnchorMode",
	XmRString,
	sizeof(String),
	XtOffset(DXResource*, anchorMode),
	XmRString,
	(XtPointer)EDIT_ANCHOR_MODE
    },
    {
	"DXVersion",
	"Flag",
	XmRBoolean,
	sizeof(Boolean),
	XtOffset(DXResource*, echoVersion),
	XmRImmediate,
	(XtPointer)False
    },
    {
	"debugMode",
	"Flag",
	XmRBoolean,
	sizeof(Boolean),
	XtOffset(DXResource*, debugMode),
	XmRImmediate,
	(XtPointer)False
    },
    {
	"showInstanceNumbers",
	"Flag",
	XmRBoolean,
	sizeof(Boolean),
	XtOffset(DXResource*, showInstanceNumbers),
	XmRImmediate,
	(XtPointer)False
    },
    {
	"directory",
	"Pathname",
	XmRString,
	sizeof(String),
	XtOffset(DXResource*, workingDirectory),
	XmRString,
	NULL
    },
    {
	"executive",
	"Pathname",
	XmRString,
	sizeof(String),
	XtOffset(DXResource*, executive),
	XmRString,
	NULL
    },
    {
	"executeProgram",
	"Flag",
	XmRBoolean,
	sizeof(Boolean),
	XtOffset(DXResource*, executeProgram),
	XmRImmediate,
	(XtPointer)False
    },
    {
	"executeOnChange",
	"Flag",
	XmRBoolean,
	sizeof(Boolean),
	XtOffset(DXResource*, executeOnChange),
	XmRImmediate,
	(XtPointer)False
    },
    {
	"printHelpMessage",
	"Flag",
	XmRBoolean,
	sizeof(Boolean),
	XtOffset(DXResource*, showHelpMessage),
	XmRImmediate,
	(XtPointer)False
    },
    {
	"host",
	"Host",
	XmRString,
	sizeof(String),
	XtOffset(DXResource*, server),
	XmRString,
	NULL
    },
    {
	"noAnchorAtStartup",
	"Flag",
	XmRBoolean,
	sizeof(Boolean),
	XtOffset(DXResource*, noAnchorAtStartup),
	XmRImmediate,
	(XtPointer)False
    },
    {
	"noConfirmedQuit",
	"Flag",
	XmRBoolean,
	sizeof(Boolean),
	XtOffset(DXResource*, noConfirmedQuit),
	XmRImmediate,
	(XtPointer)False
    },
    {
	"macros",
	"Searchlist",
	XmRString,
	sizeof(String),
	XtOffset(DXResource*, macros),
	XmRString,
	NULL
    },
    {
	"memory",
	"Number",
	XmRInt,
	sizeof(int),
	XtOffset(DXResource*, memorySize),
	XmRInt,
	0
    },
    {
	"metric",
	"Flag",
	XmRBoolean,
	sizeof(Boolean),
	XtOffset(DXResource*, isMetric),
	XmRBoolean,
	(XtPointer)False	
    },
    {
	"messages",
	"Pathname",
	XmRString,
	sizeof(String),
	XtOffset(DXResource*, errorPath),
	XmRString,
	NULL
    },
    {
	"port",
	"Number",
	XmRInt,
	sizeof(int),
	XtOffset(DXResource*, port),
	XmRInt,
	0
    },
    {
        "printImageCommand",
        "PrintCommand",
        XmRString,
        sizeof(String),
        XtOffset(DXResource*, printImageCommand),
        XmRString,
        (XtPointer) "lpr"
    },
    {
        "printImageFormat",
        "ImageFileFormat",
        XmRString,
        sizeof(String),
        XtOffset(DXResource*, printImageFormat),
        XmRString,
        (XtPointer) "PSCOLOR"
    },
    {
        "printImagePageSize",
        "ImagePageSize",
        XmRString,
        sizeof(String),
        XtOffset(DXResource*, printImagePageSize),
        XmRString,
        NULL 
    },
    {
        "printImageSize",
        "ImageSize",
        XmRString,
        sizeof(String),
        XtOffset(DXResource*, printImageSize),
        XmRString,
       	NULL 
    },
    {
        "printImageResolution",
        "ImageResolution",
        XmRInt,
        sizeof(int),
        XtOffset(DXResource*, printImageResolution),
	XmRImmediate,
	(XtPointer)0	// This is 0 (not 300) so that PrintImageDialog can
			// tell if the user specified this option/resource
    },
    {
	"program",
	"Pathname",
	XmRString,
	sizeof(String),
	XtOffset(DXResource*, program),
	XmRString,
	NULL
    },
    {
	"cfg",
	"Pathname",
	XmRString,
	sizeof(String),
	XtOffset(DXResource*, cfgfile),
	XmRString,
	NULL
    },
    {
	"runLocally",
	"Flag",
	XmRBoolean,
	sizeof(Boolean),
	XtOffset(DXResource*, runLocally),
	XmRImmediate,
	(XtPointer)False
    },
    {
	"runUIOnly",
	"Flag",
	XmRBoolean,
	sizeof(Boolean),
	XtOffset(DXResource*, runUIOnly),
	XmRImmediate,
	(XtPointer)False
    },
    {
        "saveImageFormat",
        "ImageFileFormat",
        XmRString,
        sizeof(String),
        XtOffset(DXResource*, saveImageFormat),
        XmRString,
       	NULL, 		// This is NULL (not "PSCOLOR")  so that 
			// SaveImageDialog can tell if the user 
			// specified this option/resource
    },
    {
        "saveImagePageSize",
        "ImagePageSize",
        XmRString,
        sizeof(String),
        XtOffset(DXResource*, saveImagePageSize),
        XmRString,
       	NULL, 		// This is NULL (not "8.5x11")  so that 
			// SaveImageDialog can tell if the user 
			// specified this option/resource
    },
    {
        "saveImageSize",
        "ImageSize",
        XmRString,
        sizeof(String),
        XtOffset(DXResource*, saveImageSize),
        XmRString,
       	NULL, 		
    },
    {
        "saveImageResolution",
        "ImageResolution",
        XmRInt,
        sizeof(int),
        XtOffset(DXResource*, saveImageResolution),
	XmRImmediate,
	(XtPointer)0	// This is 0 (not 300) so that SaveImageDialog can
			// tell if the user specified this option/resource
    },
    {
	"suppressStartupWindows",
	"Flag",
	XmRBoolean,
	sizeof(Boolean),
	XtOffset(DXResource*, suppressStartupWindows),
	XmRImmediate,
	(XtPointer)False
    },
    {
	"userModuleDescriptionFile",
	"Pathname",
	XmRString,
	sizeof(String),
	XtOffset(DXResource*, userModules),
	XmRString,
	NULL
    },
    {
	"executiveModuleDescriptionFile",
	"Pathname",
	XmRString,
	sizeof(String),
	XtOffset(DXResource*, executiveModule),
	XmRString,
	NULL
    },
    {
	"uiModuleDescriptionFile",
	"Pathname",
	XmRString,
	sizeof(String),
	XtOffset(DXResource*, uiModule),
	XmRString,
	NULL
    },
    {
        "noWindowPlacement",
        "WindowPlacement",
	XmRBoolean,
	sizeof(Boolean),
	XtOffset(DXResource*, noWindowPlacement),
	XmRImmediate,
	(XtPointer)False	
    },

    /*
     * Backdoor resources:
     */
    {
	"restrictionLevel",
	"Restriction",
	XmRString,
	sizeof(String),
	XtOffset(DXResource*, restrictionLevel),
	XmRString,
	NULL
    },
    {
	"noRWConfig",
	"Flag",
	XmRBoolean,
	sizeof(Boolean),
	XtOffset(DXResource*, noRWConfig),
	XmRImmediate,
	(XtPointer)False
    },
    {
	"noPanelEdit",
	"Flag",
	XmRBoolean,
	sizeof(Boolean),
	XtOffset(DXResource*, noPanelEdit),
	XmRImmediate,
	(XtPointer)False
    },
    {
	"noInteractorEdits",
	"Flag",
	XmRBoolean,
	sizeof(Boolean),
	XtOffset(DXResource*, noInteractorEdits),
	XmRImmediate,
	(XtPointer)False
    },
    {
	"noInteractorAttributes",
	"Flag",
	XmRBoolean,
	sizeof(Boolean),
	XtOffset(DXResource*, noInteractorAttributes),
	XmRImmediate,
	(XtPointer)False
    },
    {
	"noInteractorMovement",
	"Flag",
	XmRBoolean,
	sizeof(Boolean),
	XtOffset(DXResource*, noInteractorMovement),
	XmRImmediate,
	(XtPointer)False
    },
    {
	"noOpenAllPanels",
	"Flag",
	XmRBoolean,
	sizeof(Boolean),
	XtOffset(DXResource*, noOpenAllPanels),
	XmRImmediate,
	(XtPointer)False
    },
    {
	"noPanelAccess",
	"Flag",
	XmRBoolean,
	sizeof(Boolean),
	XtOffset(DXResource*, noPanelAccess),
	XmRImmediate,
	(XtPointer)False
    },
    {
	"noPanelOptions",
	"Flag",
	XmRBoolean,
	sizeof(Boolean),
	XtOffset(DXResource*, noPanelOptions),
	XmRImmediate,
	(XtPointer)False
    },
    {
	"noMessageInfoOption",
	"Flag",
	XmRBoolean,
	sizeof(Boolean),
	XtOffset(DXResource*, noMessageInfoOption),
	XmRImmediate,
	(XtPointer)False
    },
    {
	"noMessageWarningOption",
	"Flag",
	XmRBoolean,
	sizeof(Boolean),
	XtOffset(DXResource*, noMessageWarningOption),
	XmRImmediate,
	(XtPointer)False
    },
    {
	"noEditorOnError",
	"Flag",
	XmRBoolean,
	sizeof(Boolean),
	XtOffset(DXResource*, noEditorOnError),
	XmRImmediate,
	(XtPointer)False
    },
    {
	"noScriptCommands",
	"Flag",
	XmRBoolean,
	sizeof(Boolean),
	XtOffset(DXResource*, noScriptCommands),
	XmRImmediate,
	(XtPointer)False
    },
    {
	"noPGroupAssignment",
	"Flag",
	XmRBoolean,
	sizeof(Boolean),
	XtOffset(DXResource*, noPGroupAssignment),
	XmRImmediate,
	(XtPointer)False
    },
    {
	"noImageRWNetFile",
	"Flag",
	XmRBoolean,
	sizeof(Boolean),
	XtOffset(DXResource*, noImageRWNetFile),
	XmRImmediate,
	(XtPointer)False
    },
    {
	"limitedNetFileSelection",
	"Flag",
	XmRBoolean,
	sizeof(Boolean),
	XtOffset(DXResource*, limitedNetFileSelection),
	XmRImmediate,
	(XtPointer)False
    },
    {
        "netPath",
        "NetPath",
	XmRString,
	sizeof(String),
	XtOffset(DXResource*, netPath),
	XmRString,
	NULL
    },
    {
	"noImageLoad",
	"Flag",
	XmRBoolean,
	sizeof(Boolean),
	XtOffset(DXResource*, noImageLoad),
	XmRImmediate,
	(XtPointer)False
    },
    {
	"noImageSaving",
	"Flag",
	XmRBoolean,
	sizeof(Boolean),
	XtOffset(DXResource*, noImageSaving),
	XmRImmediate,
	(XtPointer)False
    },
    {
	"noImagePrinting",
	"Flag",
	XmRBoolean,
	sizeof(Boolean),
	XtOffset(DXResource*, noImagePrinting),
	XmRImmediate,
	(XtPointer)False
    },
    {
	"limitImageOptions",
	"Flag",
	XmRBoolean,
	sizeof(Boolean),
	XtOffset(DXResource*, limitImageOptions),
	XmRImmediate,
	(XtPointer)False
    },
    {
        "notifySaveNet",
        "Flag",
        XmRBoolean,
        sizeof(Boolean),
        XtOffset(DXResource*, notifySaveNet),
        XmRImmediate,
        (XtPointer)False
    },
    {
        "noNetworkExecute",
        "Flag",
        XmRBoolean,
        sizeof(Boolean),
        XtOffset(DXResource*, noNetworkExecute),
        XmRImmediate,
        (XtPointer)False
    },
    {
	"noEditorAccess",
	"Flag",
	XmRBoolean,
	sizeof(Boolean),
	XtOffset(DXResource*, noEditorAccess),
	XmRImmediate,
	(XtPointer)False
    },
    {
	"noDXHelp",
	"Flag",
	XmRBoolean,
	sizeof(Boolean),
	XtOffset(DXResource*, noDXHelp),
	XmRImmediate,
	(XtPointer)False
    },
    {
	"noCMapSetNameOption",
	"Flag",
	XmRBoolean,
	sizeof(Boolean),
	XtOffset(DXResource*, noCMapSetNameOption),
	XmRImmediate,
	(XtPointer)False
    },
    {
	"noCMapOpenMap",
	"Flag",
	XmRBoolean,
	sizeof(Boolean),
	XtOffset(DXResource*, noCMapOpenMap),
	XmRImmediate,
	(XtPointer)False
    },
    {
	"noCMapSaveMap",
	"Flag",
	XmRBoolean,
	sizeof(Boolean),
	XtOffset(DXResource*, noCMapSaveMap),
	XmRImmediate,
	(XtPointer)False
    },
    {
        "applicationPort",
        "Number",
	XmRInt,
	sizeof(int),
	XtOffset(DXResource*, applicationPort),
	XmRInt,
	NULL
    },
    {
        "applicationHost",
        "ApplicationHost",
	XmRString,
	sizeof(String),
	XtOffset(DXResource*, applicationHost),
	XmRString,
	NULL
    },
    {
        "infoEnabled",
        "InfoEnabled",
	XmRBoolean,
	sizeof(Boolean),
	XtOffset(DXResource*, infoEnabled),
	XmRImmediate,
	(XtPointer)True
    },
    {
        "warningEnabled",
        "WarningEnabled",
	XmRBoolean,
	sizeof(Boolean),
	XtOffset(DXResource*, warningEnabled),
	XmRImmediate,
	(XtPointer)True
    },
    {
        "errorEnabled",
        "ErrorEnabled",
	XmRBoolean,
	sizeof(Boolean),
	XtOffset(DXResource*, errorEnabled),
	XmRImmediate,
	(XtPointer)True
    },
    {
        "moduleInfoOpensMessage",
        "ModuleInfoOpensMessage",
	XmRBoolean,
	sizeof(Boolean),
	XtOffset(DXResource*, moduleInfoOpensMessage),
	XmRImmediate,
	(XtPointer)True
    },
    {
        "infoOpensMessage",
        "InfoOpensMessage",
	XmRBoolean,
	sizeof(Boolean),
	XtOffset(DXResource*, infoOpensMessage),
	XmRImmediate,
	(XtPointer)False
    },
    {
        "warningOpensMessage",
        "WarningOpensMessage",
	XmRBoolean,
	sizeof(Boolean),
	XtOffset(DXResource*, warningOpensMessage),
	XmRImmediate,
	(XtPointer)False
    },
    {
        "errorOpensMessage",
        "ErrorOpensMessage",
	XmRBoolean,
	sizeof(Boolean),
	XtOffset(DXResource*, errorOpensMessage),
	XmRImmediate,
	(XtPointer)True
    },
    {
        "useWindowSpecs",
        "UseWindowSpecs",
	XmRBoolean,
	sizeof(Boolean),
	XtOffset(DXResource*, useWindowSpecs),
	XmRImmediate,
	(XtPointer)False
    },
    {
        "forceNetFileEncryption",
        "ForceNetFileEncryption",
	XmRBoolean,
	sizeof(Boolean),
	XtOffset(DXResource*, forceNetFileEncryption),
	XmRImmediate,
	(XtPointer)False
    },
    {
        "cryptKey",
        "Cryptkey",
        XmRString,
        sizeof(String),
        XtOffset(DXResource*, cryptKey),
        XmRString,
        NULL
    },
    {
        "exitAfter",
        "ExitAfter",
	XmRBoolean,
	sizeof(Boolean),
	XtOffset(DXResource*, exitAfter),
	XmRImmediate,
	(XtPointer)False
    },
    {
        "forceLicense",
        "License",
        XmRString,
        sizeof(String),
        XtOffset(DXResource*, forceFunctionalLicense),
        XmRString,
        NULL
    },
    {
        "noExecuteMenus",
        "NoExecuteMenus",
	XmRBoolean,
	sizeof(Boolean),
	XtOffset(DXResource*, noExecuteMenus),
	XmRImmediate,
	(XtPointer)False
    },
    {
        "noConnectionMenus",
        "NoConnectionMenus",
	XmRBoolean,
	sizeof(Boolean),
	XtOffset(DXResource*, noConnectionMenus),
	XmRImmediate,
	(XtPointer)False
    },
    {
        "noWindowsMenus",
        "NoWindowsMenus",
	XmRBoolean,
	sizeof(Boolean),
	XtOffset(DXResource*, noWindowsMenus),
	XmRImmediate,
	(XtPointer)False
    },
    {
        "noExitOptions",
        "NoExitOptions",
	XmRBoolean,
	sizeof(Boolean),
	XtOffset(DXResource*, noExitOptions),
	XmRImmediate,
	(XtPointer)False
    },
    {
        "noImageMenus",
        "NoMenus",
	XmRBoolean,
	sizeof(Boolean),
	XtOffset(DXResource*, noImageMenus),
	XmRImmediate,
	(XtPointer)False
    },
    {
        "oemApplicationName",
        "ApplicationName",
	XmRString,
	sizeof(String),
	XtOffset(DXResource*, oemApplicationName),
	XmRString,
	NULL
    },
    {
        "oemApplicationNameCode",
        "ApplicationNameCode",
	XmRString,
	sizeof(String),
	XtOffset(DXResource*, oemApplicationNameCode),
	XmRString,
	NULL
    },
    {
        "oemLicenseCode",
        "LicenseCode",
	XmRString,
	sizeof(String),
	XtOffset(DXResource*, oemLicenseCode),
	XmRString,
	NULL
    },
    {
        "viewDataFile",
        "ViewDataFile",
	XmRString,
	sizeof(String),
	XtOffset(DXResource*, viewDataFile),
	XmRString,
	NULL
    },
     {
       "autoScrollVPE",
       "Flag",
       XmRBoolean,
       sizeof(Boolean),
       XtOffset(DXResource*, autoScrollVPEInitVal),
       XmRImmediate,
       (XtPointer)True
     },
    {
	"autoLayoutHeight",
	"Number",
	XmRInt,
	sizeof(int),
	XtOffset(DXResource*, autoLayoutHeight),
	XmRInt,
	0
    },
    {
	"autoLayoutGroupSpacing",
	"Number",
	XmRInt,
	sizeof(int),
	XtOffset(DXResource*, autoLayoutGroupSpacing),
	XmRInt,
	0
    },
    {
	"autoLayoutNodeSpacing",
	"Number",
	XmRInt,
	sizeof(int),
	XtOffset(DXResource*, autoLayoutNodeSpacing),
	XmRInt,
	0
    },
     //
     // For java
     //
    {
        "cosmoDir",
        "CosmoDir",
	XmRString,
	sizeof(String),
	XtOffset(DXResource*, cosmoDir),
	XmRString,
	(XtPointer) ""
    },
    {
        "jdkDir",
        "JdkDir",
	XmRString,
	sizeof(String),
	XtOffset(DXResource*, jdkDir),
	XmRString,
	(XtPointer) ""
    },
    {
        "htmlDir",
        "HtmlDir",
	XmRString,
	sizeof(String),
	XtOffset(DXResource*, htmlDir),
	XmRString,
	(XtPointer) ""
    },
    {
        "serverDir",
        "ServerDir",
	XmRString,
	sizeof(String),
	XtOffset(DXResource*, serverDir),
	XmRString,
	(XtPointer) ""
    },
    {
        "dxJarFile",
        "DxJarFile",
	XmRString,
	sizeof(String),
	XtOffset(DXResource*, dxJarFile),
	XmRString,
	(XtPointer) ""
    },
    {
        "userHtmlDir",
        "UserHtmlDir",
	XmRString,
	sizeof(String),
	XtOffset(DXResource*, userHtmlDir),
	XmRString,
	(XtPointer) "user"
    },
};
#endif


static
const String _defaultDXResources[] =
{
    "*quitOption.labelString:                Quit",
    "*quitOption.mnemonic:                   Q",
    "*quitOption.accelerator:                Ctrl<Key>Q",
    "*quitOption.acceleratorText:            Ctrl+Q",
    "*standInBackground:              #5F9EA0", // CadetBlue 
    "*executionHighlightForeground:   #00ff7e",
    "*backgroundExecutionForeground:  #7e7eb4",
    "*errorHighlightForeground:       #ff9b00",
    "*InsensitiveColor:     #888888",

// purify <explitive deleted> the bed because of this
//    "*tearOffModel:			     XmTEAR_OFF_ENABLED",
    NULL
};

extern "C" 
void
TurnOffButtonHelp(
    Widget   /* widget */,
    XEvent*   /* event */,
    String*   /* params */,
    Cardinal* /* num_params */)
{
    return;
}

String DXApplication::ListValuedSettings[] = {
    RECENT_NETS,
    EXPANDED_CATEGORIES,
    NULL
};

DXApplication::DXApplication(char* className): IBMApplication(className)
{
#if defined(BETA_VERSION) && defined(NOTDEF)
    //
    // Exit if this is an old BETA copy
    //
    this->betaTimeoutCheck();
#endif

    //
    // Set the global DX application pointer.
    //
    theDXApplication = this;
    this->anchor = NULL;
    this->appLicenseType = UndeterminedLicense;
    this->funcLicenseType = UndeterminedLicense;
    this->serverDisconnectScheduled = FALSE;
    this->network = NULL;
    //
    // Create the local command scope.
    //
    this->commandScope = new CommandScope();

    //
    // Initialize the packet interface.
    //
    this->serverInfo.packet = NULL;
    this->applicationPacket = NULL;
    this->startServerDialog = NULL;
    this->loadMacroDialog = NULL;
    this->openNetworkDialog = NULL;
    this->messageWindow     = NULL;
    this->loadMDFDialog   = NULL;
    this->processGroupAssignDialog = NULL;

    //
    // Create the application commands.
    //
    this->quitCmd =
	new ConfirmedQuitCommand
	     ("quit",
	     this->commandScope,
	     TRUE,
	     this);

    this->exitCmd =
        new ConfirmedExitCommand
             ("exit",
             this->commandScope,
             TRUE,
             this);

    this->openFileCmd =
	new OpenCommand("open",
			this->commandScope,
			TRUE,
			this);

    this->messageWindowCmd = 
	new NoUndoDXAppCommand("messageWindowCmd",
			this->commandScope,
			TRUE,
			this, 
			NoUndoDXAppCommand::OpenMessageWindow);

    this->openSequencerCmd = 
	new NoUndoDXAppCommand("openSequencerCmd",
			this->commandScope,
			FALSE,
			this, 
			NoUndoDXAppCommand::OpenSequencer);

    this->openAllColormapCmd = 
	new NoUndoDXAppCommand("openAllColormapCmd",
			this->commandScope,
			FALSE,
			this, 
			NoUndoDXAppCommand::OpenAllColormaps);

    this->loadMacroCmd =
	new NoUndoDXAppCommand("Load Macro Command",
			this->commandScope,
			TRUE,
			this, 
			NoUndoDXAppCommand::LoadMacro);

    this->executeOnceCmd =
	new NoUndoDXAppCommand("Execute Once",
			this->commandScope,
			FALSE,
			this, 
			NoUndoDXAppCommand::ExecuteOnce);

    this->executeOnChangeCmd =
	new NoUndoDXAppCommand("Execute On Change",
			this->commandScope,
			FALSE,
			this, 
			NoUndoDXAppCommand::ExecuteOnChange);

    this->endExecutionCmd =
	new NoUndoDXAppCommand("End Execution",
			this->commandScope,
			FALSE,
			this, 
			NoUndoDXAppCommand::EndExecution);

    this->connectedToServerCmd =
	new NoOpCommand("Connected To Server", this->commandScope, TRUE);

    this->disconnectedFromServerCmd =
	new NoOpCommand("Disconnected From Server", this->commandScope, TRUE);

    this->executingCmd =
	new NoOpCommand("executingCmd", this->commandScope, TRUE);
    this->notExecutingCmd =
	new NoOpCommand("notExecutingCmd", this->commandScope, TRUE);

    this->connectToServerCmd = 
	new NoUndoDXAppCommand("Start Server",
			this->commandScope,
			TRUE,
			this, 
			NoUndoDXAppCommand::StartServer);

    this->resetServerCmd = 
	new NoUndoDXAppCommand("Reset Server",
			this->commandScope,
			FALSE,
			this, 
			NoUndoDXAppCommand::ResetServer);

    this->disconnectFromServerCmd = 
	new DisconnectFromServerCommand("Disconnect From Server...",
			this->commandScope,
			FALSE);

#if USE_REMAP	// 6/14/93
    this->toggleRemapInteractorsCmd = 
	new NoUndoDXAppCommand("Remap Interactor Outputs...",
			this->commandScope,
			TRUE,
			this, 
			NoUndoDXAppCommand::RemapInteractorOutputs);
#endif
    this->loadMDFCmd = 
	new NoUndoDXAppCommand("Load MDF...",
			this->commandScope,
			TRUE,
			this, 
			NoUndoDXAppCommand::LoadUserMDF);

    this->toggleInfoEnable = 
	new NoUndoDXAppCommand("Enable Information",
			this->commandScope,
			TRUE,
			this, 
			NoUndoDXAppCommand::ToggleInfoEnable);
    this->toggleWarningEnable = 
	new NoUndoDXAppCommand("Enable Warnings",
			this->commandScope,
			TRUE,
			this, 
			NoUndoDXAppCommand::ToggleWarningEnable);
    this->toggleErrorEnable = 
	new NoUndoDXAppCommand("Enable Errors",
			this->commandScope,
			TRUE,
			this, 
			NoUndoDXAppCommand::ToggleErrorEnable);
    this->assignProcessGroupCmd = 
	new NoUndoDXAppCommand("assignProcessGroup",
			this->commandScope,
			TRUE,
			this, 
			NoUndoDXAppCommand::AssignProcessGroup);

    this->connectedToServerCmd->autoActivate(this->executeOnceCmd);
    this->connectedToServerCmd->autoActivate(this->executeOnChangeCmd);
    this->connectedToServerCmd->autoActivate(this->endExecutionCmd);
    this->connectedToServerCmd->autoActivate(this->disconnectFromServerCmd);
    this->connectedToServerCmd->autoActivate(this->resetServerCmd);
    this->connectedToServerCmd->autoDeactivate(this->connectToServerCmd);


    this->disconnectedFromServerCmd->autoActivate(this->connectToServerCmd);
    this->disconnectedFromServerCmd->autoDeactivate(this->executeOnceCmd);
    this->disconnectedFromServerCmd->autoDeactivate(this->executeOnChangeCmd);
    this->disconnectedFromServerCmd->autoDeactivate(this->endExecutionCmd);
    this->disconnectedFromServerCmd->autoDeactivate(this->disconnectFromServerCmd);
    this->disconnectedFromServerCmd->autoDeactivate(this->resetServerCmd);

    //
    // Once in execute on change don't allow it again until the user
    // does an endExecutionCmd
    //
    this->executeOnChangeCmd->autoDeactivate(this->executeOnChangeCmd);
    this->endExecutionCmd->autoActivate(this->executeOnChangeCmd);
    this->openFileCmd->autoActivate(this->executeOnChangeCmd);


    //
    // Set the automatic activation of any commands that depend on whether 
    // or not we are currently executing. 
    //
    this->executingCmd->autoDeactivate(this->openFileCmd);
    this->notExecutingCmd->autoActivate(this->openFileCmd);
    this->executingCmd->autoDeactivate(this->executeOnceCmd);
    this->notExecutingCmd->autoActivate(this->executeOnceCmd);

    //
    // Don't allow execute on change during an execute once.
    //
    this->executeOnceCmd->autoDeactivate(this->executeOnChangeCmd);
    this->notExecutingCmd->autoActivate(this->executeOnChangeCmd);

    //
    // Initialize the NodeDefinition allocator. 
    //
    theNDAllocatorDictionary = new NDAllocatorDictionary;

    this->readFirstNetwork = FALSE;

#if !defined(WORKSPACE_PAGES)
    this->PGManager = new ProcessGroupManager(this);
#endif

}

DXApplication::~DXApplication()
{
#ifdef __PURIFY__ 
// do not free memory just before exiting (makes termination slow).


    //
    // This deletes the panels.  We need to delete the panels before the
    // network is deleted, because the panel's destructor references the
    // network which makes Purify unhappy (referencing freed memory).
    //
    this->network->clear();
    this->destroyDumpedObjects();
    
    //
    // Delete the anchor window after clearing, but before 
    // deleting the network.
    //
    delete this->anchor;
    this->anchor = NULL;

    Network *n;
    ListIterator iterator(this->macroList);
    while (n = (Network*)this->macroList.getElement(1)) {
	this->macroList.deleteElement(1);
        delete n;
    }
    delete this->network;
    this->destroyDumpedObjects();


    //
    // Delete the packet interfaces after the networks
    //
    if (this->serverInfo.packet)
    {
	delete theDXApplication->serverInfo.packet;
	theDXApplication->serverInfo.packet = NULL;
	theDXApplication->disconnectedFromServerCmd->execute();
    }

    if (this->applicationPacket)
	delete this->applicationPacket;
    this->destroyDumpedObjects();

    //
    // Delete objects that were scheduled for deletion. 
    // This must be before any of the DXApplication state.
    //
    this->destroyDumpedObjects();

    // Delete the application windows
    if (this->messageWindow)
	delete this->messageWindow;

    //
    // Delete the application dialogs
    //
    if (this->loadMacroDialog)
	delete this->loadMacroDialog;
    if (this->loadMDFDialog)
	delete this->loadMDFDialog;
    if (this->startServerDialog)
	delete this->startServerDialog;
    if (this->openNetworkDialog)
	delete this->openNetworkDialog;

    //
    // Delete the application commands.
    //
    delete this->quitCmd;
    delete this->exitCmd;
    delete this->openFileCmd;
    delete this->loadMacroCmd;
    delete this->executeOnceCmd;
    delete this->executeOnChangeCmd;
    delete this->endExecutionCmd;
    delete this->connectedToServerCmd;
    delete this->disconnectedFromServerCmd;
    delete this->executingCmd;
    delete this->notExecutingCmd;
    delete this->connectToServerCmd;
    delete this->resetServerCmd;
    delete this->disconnectFromServerCmd;
    delete this->messageWindowCmd;
    delete this->openSequencerCmd;
    delete this->openAllColormapCmd;

    delete this->loadMDFCmd; 
    delete this->toggleInfoEnable;
    delete this->toggleWarningEnable;
    delete this->toggleErrorEnable;
    delete this->assignProcessGroupCmd;

#if !defined(WORKSPACE_PAGES)
    delete this->PGManager;
#endif

    //
    // Delete the command scope.
    //
    delete this->commandScope;

    if (this->serverInfo.server) 
	delete this->serverInfo.server;
    if (this->serverInfo.executive) 
	delete this->serverInfo.executive;
    if (this->serverInfo.workingDirectory) 
	delete this->serverInfo.workingDirectory;
    if (this->serverInfo.userModules) 
	delete this->serverInfo.userModules;


#if 0
    if (theCPGroupDialog) {
	theCPGroupDialog = NULL;
	delete theCPGroupDialog;
    }
    delete theSIAllocatorDictionary; 	theSIAllocatorDictionary = NULL;
    delete theCDBAllocatorDictionary; 	theCDBAllocatorDictionary = NULL;
    delete theDynamicPackageDictionary; theDynamicPackageDictionary = NULL;
    delete theNDAllocatorDictionary; 	theNDAllocatorDictionary = NULL;
    delete theSymbolManager;  		theSymbolManager = NULL;
    delete theInteractorStyleDictionary;theInteractorStyleDictionary =  NULL; 

    DictionaryIterator di(*theNodeDefinitionDictionary);
    NodeDefinition *nd;
    while (nd = (NodeDefinition*)di.getNextDefinition()) {
	delete nd;
    delete theNodeDefinitionDictionary; theNodeDefinitionDictionary = NULL;
#endif

    ListIterator iter(this->saveResourceValues);
    List* resources;
    while (resources=(List*)iter.getNext()) {
	if (resources) delete resources;
    }

#endif	// 0 - do not free memory just before terminating (wastes time).

    //
    // Set the flag to terminate the event processing loop.
    //
    this->runApplication = FALSE;

    theDXApplication = NULL;
}


//
// Read the file that describes modules.
// This is done once at start up and so we load theNodeDefinitionDictionary
// directly with LoadMDFFile.
//
void DXApplication::loadMDF()
{
    //
    // Read the MDF file.
    //
    char s[1000];
    if (*this->resource.executiveModule == '/' || 
	*this->resource.executiveModule == '.')
	strcpy(s, this->resource.executiveModule);
    else
    {
	strcpy(s, this->getUIRoot());
	strcat(s, "/");
	strcat(s, this->resource.executiveModule);
    }
    Dictionary newdefs;
    if (!LoadMDFFile(s,"executive",&newdefs, FALSE)) 
        return;

    //
    // Mark all of these as system-tools.
    //
    NodeDefinition *nd;
    DictionaryIterator di(newdefs);
    while ( (nd = (NodeDefinition*)di.getNextDefinition()) ) {
        Symbol s = nd->getNameSymbol();
        theNodeDefinitionDictionary->addDefinition(s,(const void*)nd);
        nd->setUserTool(FALSE);
    }

}
//
// Read the file that describes interactors.
// This is done once at start up and so we load theNodeDefinitionDictionary
// directly with LoadMDFFile.
//
void DXApplication::loadIDF()
{
    //
    // Read the MDF file.
    //  Use ui++.mdf until we completely switch over to this UI
    //  (from the old one which uses ui.mdf)
    //
    BuildtheInteractorStyleDictionary();

    char s[1000];
    if (*this->resource.uiModule == '/' ||
	*this->resource.uiModule == '.')
	strcpy(s, this->resource.uiModule);
    else
    {
	strcpy(s, this->getUIRoot());
	strcat(s, "/");
	strcat(s, this->resource.uiModule);
    }

    Dictionary newdefs;
    if (!LoadMDFFile(s,"user interface",&newdefs, FALSE)) 
        return;

    //
    // Mark all of these as system-tools.
    //
    NodeDefinition *nd;
    DictionaryIterator di(newdefs);
    while ( (nd = (NodeDefinition*)di.getNextDefinition()) ) {
        Symbol s = nd->getNameSymbol();
        theNodeDefinitionDictionary->addDefinition(s,(const void*)nd);
        nd->setUserTool(FALSE);
    }

}

//
// Read the file that describes interactors.
// If dict is not null then fill the given dictionary with the 
// NodeDefintions found in the given MDF. 
// This is possibly done more than once, so we don't want to load 
// theNodeDefinitionDictionary  directly. Instead, we load a local dictionary,
// then merge it with theNodeDefinitionDictionary.  We then load all the tools
// in the temporary dictionary into the ToolSelector(s).
//
void DXApplication::loadUDF(const char *fileName, Dictionary *dict, 
				boolean uiLoadedOnly)
{
	Dictionary local_dict;
	if (!dict)
		dict = &local_dict;
	if (LoadMDFFile(fileName,"user's",dict, uiLoadedOnly) &&
		dict->getSize() > 0) {
			Dictionary olddefs;

			//
			// Replace the old NodeDefinitions with the new ones. 
			// 
			theNodeDefinitionDictionary->replaceDefinitions(dict, &olddefs);

			//
			// Send any new definitions to the server. 
			//
			this->sendNewMDFToServer(dict,&olddefs);

			//
			// Make sure the tool selectors get updated. 
			// 
			ToolSelector::MergeNewTools(dict);

			//
			// Now ask all the networks to update the node that are contained
			// within the network.
			//
			this->network->redefineNodes(dict,&olddefs);
			ListIterator li(this->macroList);
			Network *net;
			while ( (net = (Network*)li.getNext()) ) 
				net->redefineNodes(dict,&olddefs);

			//
			// And now we can get rid of the old NodeDefinitions.
			//
			DictionaryIterator di(olddefs);
			NodeDefinition *nd;
			while ( (nd = (NodeDefinition*)di.getNextDefinition()) ) 
				delete nd;

	}
}

			       
#if defined (SIGDANGER)
extern "C" {
static void
SigDangerHandler(int dummy)
{
    char msg[1024];
#if defined(ibm6000)
    sprintf(msg,"AIX has notified %s that the User Interface\nis in "
    		"danger of being killed due to insufficient page space.\n",
		theApplication->getInformalName());
#else        
    sprintf(msg,"The operating system has issued a SIGDANGER to %s\n",
		theApplication->getInformalName());
#endif       
    write(2, msg, STRLEN(msg));
    signal(SIGDANGER, SigDangerHandler);
}            
}
#endif
 

// FIXME: We need a place to remove the signal handlers.  It's not good
// to leave them installed because a signal might arrive in the middle of
// a destructor.  If that happens, then the handler will crash.
void 
DXApplication::InitializeSignals(void)
{            
    // adding a signal handler for SIGABRT does not catch us if
    // an assert fails.  The function abort does not return.
    if (!getenv ("DXUINOCATCHERROR")) {
#if defined(HAVE_SIGDANGER)
    signal(SIGDANGER, SigDangerHandler);
#endif       
	signal (SIGSEGV, DXApplication_HandleCoreDump);
#if defined(HAVE_SIGBUS)
	signal (SIGBUS, DXApplication_HandleCoreDump);
#endif
#if defined(HAVE_SIGKILL)
	signal (SIGKILL, DXApplication_HandleCoreDump);
#endif
    }

}            

//
// Install the default resources for this class.
//
void DXApplication::installDefaultResources(Widget baseWidget)
{
    this->setDefaultResources(baseWidget, _defaultDXResources);
    this->IBMApplication::installDefaultResources(baseWidget);
}
boolean DXApplication::initialize(unsigned int* argcp,
								  char**        argv)
{
	boolean wasSetBusy = FALSE;

	if (!this->IBMApplication::initializeWindowSystem(argcp,argv))
		return FALSE;

	//
	// Color preallocation - necessary only because of the logo
	// 
	int j;
	XrmValue from, toinout;
	Pixel tmp_pixel;
	for (j=0; j<XtNumber(_DXResourceList); j++) {
		if (!strcmp(_DXResourceList[j].resource_type, XmRPixel)) {
			from.addr = (XPointer)_DXResourceList[j].default_addr; 
			from.size = strlen((char*)from.addr);
			toinout.addr = (XPointer)&tmp_pixel; toinout.size = sizeof(Pixel);
			XtConvertAndStore (this->getRootWidget(), XmRString, &from,
				XmRPixel, &toinout);
		}
	}

	if (!this->IBMApplication::initialize(argcp,argv))
		return FALSE;

#ifdef DIAGNOSTICS
	/*
	* Set after function for diagnostic purposes only...
	*/
	XSynchronize(this->display, True);
	XSetAfterFunction(this->display, DXApplication_DXAfterFunction);
#endif

	this->InitializeSignals();

	this->parseCommand(argcp, argv, _DXOptionList, XtNumber(_DXOptionList));


	//
	// Get application resources.
	//
	if (NOT DXApplication::DXApplicationClassInitialized)
	{
		this->installDefaultResources(theApplication->getRootWidget());
		this->getResources((XtPointer)&DXApplication::resource,
			_DXResourceList, XtNumber(_DXResourceList));

		DXApplication::MsgExecute = 
			theSymbolManager->registerSymbol("Execute");
		DXApplication::MsgStandBy = 
			theSymbolManager->registerSymbol("StandBy");
		DXApplication::MsgExecuteDone = 
			theSymbolManager->registerSymbol("ExecuteDone");
		DXApplication::MsgServerDisconnected = 
			theSymbolManager->registerSymbol("ServerDisconnected");
		DXApplication::MsgPanelChanged = 
			theSymbolManager->registerSymbol("PanelChanged");
		DXApplication::DXApplicationClassInitialized = TRUE;
	}

	//
	// setup resources that can be environment varialbles.
	//
	if (DXApplication::resource.executiveModule == NULL)
		DXApplication::resource.executiveModule = "lib/dx.mdf";

	if (DXApplication::resource.uiModule == NULL)
		DXApplication::resource.uiModule = "ui/ui.mdf";

	if (DXApplication::resource.userModules == NULL) {
		char *s = getenv("DXMDF");
		if (s)
			// This will show up as a memory leak, not worth worrying about
			DXApplication::resource.userModules = DuplicateString(s);
	} 

	if (DXApplication::resource.macros == NULL) {
		char *s = getenv("DXMACROS");
		if (s)
			// This will show up as a memory leak, not worth worrying about
			DXApplication::resource.macros = DuplicateString(s);
	}

	if (DXApplication::resource.server == NULL) {
		char *s = getenv("DXHOST");
		if (s) 
			// This will show up as a memory leak, not worth worrying about
			DXApplication::resource.server = DuplicateString(s); 
		else
			DXApplication::resource.server = "localhost";
	}
	// Remove the port number if it exists (i.e DXHOST=slope,1920)
	char *p;
	if ( (p = strrchr(DXApplication::resource.server,',')) )
		*p = '\0';


	if (DXApplication::resource.netPath == NULL) {
		char *s = getenv("DXNETPATH");
		if (s)
			// This will show up as a memory leak, not worth worrying about
			DXApplication::resource.netPath = DuplicateString(s); 
	}

	if (DXApplication::resource.cryptKey == 0) {
		char *s = getenv("DXCRYPTKEY");
		if (s) {
			DXApplication::resource.cryptKey = DuplicateString(s);
#ifndef DEBUG
			// Hide the key so debuggers can't get at it.
			s = DuplicateString("DXCRYPTKEY=");
			putenv(s);
#endif
		}
	}

	//
	// If the app does not allow editor access or we are starting up without
	// displaying the anchor window, force one of (image or menubar) mode.
	// Note that we also test this below, to check against the functional
	// license that was acquired.
	//
	if ((this->inEditMode() && !this->appAllowsEditorAccess()) || 
		DXApplication::resource.noAnchorAtStartup) {
			if (this->inMenuBarMode())
				DXApplication::resource.anchorMode = MENUBAR_ANCHOR_MODE; 
			else
				DXApplication::resource.anchorMode = IMAGE_ANCHOR_MODE; 
	}

	//
	// Echo the resources.
	//
	if (DXApplication::resource.debugMode)
	{
		if (DXApplication::resource.port != 0)
			printf("port        = %d\n", DXApplication::resource.port);
		if (DXApplication::resource.memorySize != 0)
			printf("memory size = %d\n", DXApplication::resource.memorySize);

		if (DXApplication::resource.server)
			printf("server = %s\n", DXApplication::resource.server);
		if (DXApplication::resource.executive)
			printf("executive = %s\n", DXApplication::resource.executive);
		if (DXApplication::resource.workingDirectory)
			printf("working directory = %s\n",
			DXApplication::resource.workingDirectory);
		if (DXApplication::resource.netPath)
			printf("net path = %s\n", DXApplication::resource.netPath);
		if (DXApplication::resource.program)
			printf("program = %s\n", DXApplication::resource.program);
		if (DXApplication::resource.cfgfile)
			printf("cfgfile = %s\n", DXApplication::resource.cfgfile);
		if (this->getUIRoot())
			printf("root = %s\n", this->getUIRoot());
		if (DXApplication::resource.macros)
			printf("macros = %s\n", DXApplication::resource.macros);
		if (DXApplication::resource.errorPath)
			printf("error path = %s\n", DXApplication::resource.errorPath);
		if (DXApplication::resource.echoVersion)
			printf("echo version\n");
		if (DXApplication::resource.anchorMode)
			printf("anchor mode = %s\n",DXApplication::resource.anchorMode);
		if (DXApplication::resource.noAnchorAtStartup)
			printf("hiding anchor at startup\n");
		if (DXApplication::resource.debugMode)
			printf("debug mode\n");
		if (DXApplication::resource.runUIOnly)
			printf("run UI only\n");
		if (DXApplication::resource.showHelpMessage)
			printf("show help message\n");
		if (DXApplication::resource.userModules)
			printf("user mdf = %s\n", DXApplication::resource.userModules);
		if (DXApplication::resource.executiveModule)
			printf("executive mdf = %s\n", DXApplication::resource.executiveModule);
		if (DXApplication::resource.uiModule)
			printf("ui mdf = %s\n", DXApplication::resource.uiModule);
		if (DXApplication::resource.suppressStartupWindows)
			printf("suppress startup windows\n");

		if (DXApplication::resource.applicationPort != 0)
			printf("application port = %d\n", DXApplication::resource.applicationPort);
		if (DXApplication::resource.applicationHost)
			printf("application host = %s\n", DXApplication::resource.applicationHost);

		//
		// Image printing resources.
		//
		if (DXApplication::resource.printImageCommand) 
			printf("print image command = '%s'\n",
			DXApplication::resource.printImageCommand);
		if (DXApplication::resource.printImageFormat)
			printf("print image format = '%s'\n",
			DXApplication::resource.printImageFormat);
		if (DXApplication::resource.printImagePageSize)
			printf("print image page size = '%s'\n",
			DXApplication::resource.printImagePageSize);
		printf("print image resolution = %d\n",
			DXApplication::resource.printImageResolution);

		//
		// Image saving resources.
		//
		if (DXApplication::resource.saveImageFormat)
			printf("save image format = '%s'\n",
			DXApplication::resource.saveImageFormat);
		if (DXApplication::resource.saveImagePageSize)
			printf("save image page size = '%s'\n",
			DXApplication::resource.saveImagePageSize);
		printf("save image resolution = %d\n",
			DXApplication::resource.saveImageResolution);

		//
		// UI restrictions 
		//
		if (DXApplication::resource.restrictionLevel)
			printf("restriction level %s\n", 
			DXApplication::resource.restrictionLevel);
		if (DXApplication::resource.noEditorAccess)
			printf("no editor access\n");
		if (DXApplication::resource.limitedNetFileSelection)
			printf("limited network file selection\n");
		if (DXApplication::resource.noImageRWNetFile)
			printf("no net file read/write\n");
		if (DXApplication::resource.noImageSaving)
			printf("no image saving\n");
		if (DXApplication::resource.noImagePrinting)
			printf("no image printing\n");
		if (DXApplication::resource.noImageLoad)
			printf("no image load \n");
		if (DXApplication::resource.limitImageOptions)
			printf("limit image options\n");
		if (DXApplication::resource.noRWConfig)
			printf("no cfg save\n");
		if (DXApplication::resource.noPanelEdit)
			printf("no panel edit\n");
		if (DXApplication::resource.noInteractorEdits)
			printf("no interactor style\n");
		if (DXApplication::resource.noInteractorAttributes)
			printf("no interactor attributes\n");
		if (DXApplication::resource.noInteractorMovement)
			printf("no interactor movement\n");
		if (DXApplication::resource.noOpenAllPanels)
			printf("no open all panels\n");
		if (DXApplication::resource.noPanelAccess)
			printf("no panel access\n");
		if (DXApplication::resource.noPanelOptions)
			printf("no panel options\n");
		if (DXApplication::resource.noMessageInfoOption)
			printf("no message info option\n");
		if (DXApplication::resource.noMessageWarningOption)
			printf("no message warning option\n");
		if (DXApplication::resource.noDXHelp)
			printf("no DX help\n");

		//
		// automatic graph layout
		//
		if (DXApplication::resource.autoLayoutHeight > 0)
			printf("automatic graph layout height = %d\n", 
			DXApplication::resource.autoLayoutHeight);
		if (DXApplication::resource.autoLayoutGroupSpacing > 0)
			printf("automatic graph layout group spacing = %d\n", 
			DXApplication::resource.autoLayoutGroupSpacing);
		if (DXApplication::resource.autoLayoutNodeSpacing > 0)
			printf("automatic graph layout node spacing = %d\n", 
			DXApplication::resource.autoLayoutNodeSpacing);
	}

	if (this->resource.echoVersion)
	{
		printf(
#ifdef BETA_VERSION
			"%s User Interface, version %02d.%02d.%04d Beta (%s, %s)\n",
#else
			"%s User Interface, version %02d.%02d.%04d (%s, %s)\n",
#endif
			theApplication->getFormalName(),
			DX_MAJOR_VERSION, DX_MINOR_VERSION, DX_MICRO_VERSION,
			__TIME__, __DATE__);
		exit (0);
	}

	//
	// If the DXApplication does not allow DX help, then turn off middle
	// mouse button help which is implemented through the actions added
	// in IBMApplication::addActions().  This can't be done in addActions() 
	// because it is called before the options/resources are parsed.
	//
	if (!this->appAllowsDXHelp()) {
		XtActionsRec action;
		action.string = "IBMButtonHelp";
		action.proc   = TurnOffButtonHelp; 
		XtAppAddActions(this->applicationContext, &action, 1);
	}

	//
	// Validate and set automatic graph layout values
	//
	if (DXApplication::resource.autoLayoutHeight > 0) {
		const char* errmsg = 
			GraphLayout::SetHeightPerLevel (DXApplication::resource.autoLayoutHeight);
		if (errmsg) {
			fprintf (stderr, errmsg);
			return FALSE;
		}
	}
	if (DXApplication::resource.autoLayoutGroupSpacing > 0) {
		const char* errmsg = 
			GraphLayout::SetGroupSpacing (DXApplication::resource.autoLayoutGroupSpacing);
		if (errmsg) {
			fprintf (stderr, errmsg);
			return FALSE;
		}
	}
	if (DXApplication::resource.autoLayoutNodeSpacing > 0) {
		const char* errmsg = 
			GraphLayout::SetNodeSpacing (DXApplication::resource.autoLayoutNodeSpacing);
		if (errmsg) {
			fprintf (stderr, errmsg);
			return FALSE;
		}
	}


	//
	// Validate the resources and options
	//
	if (this->inEditMode() && !this->appAllowsEditorAccess()) {
		fprintf(stderr,"-edit and -noEditorAccess options are incompatible.\n");
		return FALSE;
	}


	if (this->appAllowsImageRWNetFile()  && 
		this->appLimitsNetFileSelection()  &&
		(this->resource.netPath == NULL)) {
			fprintf(stderr,
				"The \"limitedNetFileSelection\" or \"noImageRWNetFile\" "
				"option requires\na directory pathname specified by "
				"the \"DXNETPATH\" environment variable, \n"
				"the -netPath command line option, or\n"
				"the *netPath resource.\n");
			return FALSE;
	} 

	//
	// Setup Server Information
	//

	this->serverInfo.autoStart        = DXApplication::resource.port <= 0;
	this->serverInfo.server           = DuplicateString(
		DXApplication::resource.server);
	this->serverInfo.executive        = DuplicateString(
		DXApplication::resource.executive);
	this->serverInfo.workingDirectory = DuplicateString(
		DXApplication::resource.workingDirectory);
	this->serverInfo.userModules = DuplicateString(
		DXApplication::resource.userModules);
	this->serverInfo.port             =
		DXApplication::resource.port == 0? 1900: DXApplication::resource.port;
	this->serverInfo.memorySize       = DXApplication::resource.memorySize;
	this->serverInfo.executiveFlags = NULL;
	int length = 0;
	int i;
	for (i = 1; i < *argcp; ++i)
	{
		length += STRLEN(argv[i]) + 1;
		if (this->serverInfo.executiveFlags == NULL)
		{
			this->serverInfo.executiveFlags = (char *)MALLOC (length);
			this->serverInfo.executiveFlags[0] = '\0';
		}
		else
		{
			this->serverInfo.executiveFlags = 
				(char *)REALLOC(this->serverInfo.executiveFlags, length);
			strcat(this->serverInfo.executiveFlags, " ");
		}
		strcat(this->serverInfo.executiveFlags, argv[i]);
	}


	if (this->inDataViewerMode()) {
		this->appLicenseType = FullyLicensed; 
		this->funcLicenseType = ViewerLicense;
		char *s;
		if ( (s = getenv("DXVIEWERNET")) ) {
			this->resource.program = s; 
		} else {	
			char *buf = new char[1024];
			sprintf(buf,"%s/ui/viewer.net",this->getUIRoot());
			this->resource.program = buf;	
		}
		this->resource.executeOnChange = True;
		this->resource.noImageRWNetFile = True;
		this->resource.noRWConfig = True;
		this->resource.noImageLoad = True;
		this->resource.noDXHelp = True;
		this->resource.noPGroupAssignment = True;
		this->resource.limitImageOptions = True;
		this->resource.noScriptCommands = True;
		this->resource.noConnectionMenus = True;
		this->resource.noWindowsMenus = True;
		this->resource.anchorMode = IMAGE_ANCHOR_MODE;
		this->resource.noAnchorAtStartup= True; 
		this->resource.suppressStartupWindows = True; 
		this->resource.noConfirmedQuit = True; 
		// this->resource.cryptKey = 0x54232419; 
		// this->resource.forceNetFileEncryption = True; 
	} else {
		//
		// Get a license (after parsing resources), and if we can't 
		// then terminate 
		//
		LicenseTypeEnum app_lic, func_lic;
		this->determineUILicense(&app_lic,&func_lic);
		this->appLicenseType = app_lic;
		this->funcLicenseType = func_lic;
		if (this->funcLicenseType == Unlicensed) { 
			if (this->isFunctionalLicenseForced()) {
				fprintf(stderr,"%s could not get the requested license\n",
					this->getInformalName());
				this->funcLicenseType = this->getForcedFunctionalLicenseEnum();
			} else {
				this->funcLicenseType = DeveloperLicense;
			} 
#ifdef GUILT_MESSAGE
			// We do this below after the anchor window is up.
			//WarningMessage(
			//	"You are running an unregistered copy of %s\n"
			//	"Please contact your sales organization to acquire the\n"
			//	"proper license enabling key.",this->getInformalName());
			// this->appLicenseType = FullyLicensed;
		} else if (this->appLicenseType == TimedLicense) {
			this->funcLicenseType = DeveloperLicense;
#endif // GUILT_MESSAGE
			this->appLicenseType = TimedLicense;
			InstallShutdownTimer(NULL,(XtPointer)(LICENSED_MINUTES*60),NULL);
		} 
	}


	i=0;
	ResourceManager::BuildTheResourceManager();
	while (DXApplication::ListValuedSettings[i]) {
		theResourceManager->registerMultiValued(DXApplication::ListValuedSettings[i]);
		i++;
	}


#define START_SERVER_EARLY 0	// Not right before 3.1 release
#if START_SERVER_EARLY 
	DXChild *c = NULL;
	if (!this->resource.runUIOnly)
		c = this->startServer(); 
#endif
	//
	// Create the first/root/anchor network and place it in the
	// network list.
	this->network = this->newNetwork();


	if (this->appAllowsEditorAccess()) {
		//
		// Initialize the ConfigurationDialog allocator for the editor
		//
		theCDBAllocatorDictionary = new CDBAllocatorDictionary;

		//
		// Initialize the StandIn allocator for the editor.
		//
		theSIAllocatorDictionary = new SIAllocatorDictionary;
	} 

	//
	// Move to the indicated directory 
	//
	if (this->serverInfo.workingDirectory && 
		(chdir(this->serverInfo.workingDirectory) < 0)) {
			fprintf(stderr,"Could not change directory to %s",
				this->serverInfo.workingDirectory);
	}


	//
	// Load the MDF files.  If there was a user mdf file (i.e. -mdf foo.mdf)
	// then assume the exec has loaded it itself.
	//
	this->loadMDF();
	this->loadIDF();
	if (this->resource.userModules)
		this->loadUDF(this->resource.userModules, NULL, FALSE);


	// 
	// Decorator Styles
	//
	BuildtheDecoratorStyleDictionary();

#if 0
#if 00 && SYSTEM_MACROS // Net yet, dawood 11/17/94
	//
	// load the set of system macros from $DXROOT/ui/.
	//
	char path[1024];
	sprintf(path,"%s/ui",this->getUIRoot());
	MacroDefinition::LoadMacroDirectories(path, FALSE, NULL, TRUE);
#endif
#endif

	//
	// load the initial set of user macros.
	//
	MacroDefinition::LoadMacroDirectories(this->resource.macros);

	//
	// Create the anchor window.
	//
	if (!this->inEditMode())
	{
		if (this->inImageMode())
			this->anchor = this->newImageWindow(this->network);
		else if (this->inMenuBarMode())
			this->anchor = new DXAnchorWindow ("dxAnchor", TRUE, TRUE);
		else {
			fprintf(stderr,"Unrecognized anchor mode\n");
			ASSERT(0);
		}
		//
		// Initialize the anchor window so it can handle reading in the network
		// (before being managed).
		//
		if (this->applyWindowPlacements() || 
			DXApplication::resource.noAnchorAtStartup)
			this->anchor->initialize();
		else {
			this->anchor->manage();
			this->setBusyCursor(TRUE);
			wasSetBusy = TRUE;
		}
	}
	else
	{
		this->anchor = this->newNetworkEditor(this->network);
		if (!this->anchor)
			return FALSE;    // Expect newNetworkEditor() to issue message.

		//
		// The editor must always be managed before reading .nets
		// (we could do it with some work, but doesn't seem useful especially
		// since we don't want error messages coming up behind the VPE).
		//
		this->anchor->manage();
		this->setBusyCursor(TRUE);
		wasSetBusy = TRUE;
	}


	//
	// Create the message and debug windows.
	//
	this->messageWindow = this->newMsgWin();
	this->messageWindow->initialize();



	//
	// If requested, read in the network.  This is after opening the anchor
	// window because image nodes may wish to bind with the initial image
	// window, etc.
	if (this->resource.program != NULL)
		this->openFile(this->resource.program, this->resource.cfgfile);

	if (this->inDataViewerMode()) {
		Node *n = this->network->findNode("Import");	
		if (!n) 
			ErrorMessage("Can not find Import tool in viewing program"); 
		else  {
			const char *s = this->getDataViewerImportFile();
			ASSERT(s);
			n->setInputValue(1,s);
		}
	}

	//
	// Display the anchor window (after reading the network which may have
	// window sizing information). 
	//
	if (DXApplication::resource.noAnchorAtStartup)
		XtRealizeWidget(this->anchor->getRootWidget());
	else if (!this->anchor->isManaged()) {
		this->anchor->manage();
		this->setBusyCursor(TRUE);
		wasSetBusy = TRUE;
	}

	//
	// Post the copyright message if the anchor window came up.
	//
	if (!DXApplication::resource.noAnchorAtStartup)
		this->postCopyrightNotice();

#ifdef HAS_CLIPNOTIFY_EXTENSION
	//
	// If applicable, see if this is an IBM POWER Visualization Server 
	// Video Controler
	//
	int majorCode;
	int errorCode;
	if (this->hasClipnotifyExtension =
		XQueryExtension
		(this->display,
		CLIPNOTIFY_PROTOCOL_NAME,
		&majorCode,
		&this->clipnotifyEventCode,
		&errorCode))
	{
		XSetWindowAttributes        attributes;
		Window win =
			XCreateWindow
			(this->display,
			RootWindow(this->display, 0),
			0,  0,
			10, 10,
			2,  8,
			CopyFromParent,
			CopyFromParent,
			NULL,
			&attributes);
		XClipNotifyAddWin(this->display, win);
	}
#endif

	//
	// Refresh the screen.
	//
	XmUpdateDisplay(this->getRootWidget());


#ifndef	DXD_WIN
	//
	// Connect to exec first 
	//
	if (!this->resource.runUIOnly)
	{
#if !START_SERVER_EARLY 
		DXChild *c = this->startServer(); 
#else
		ASSERT(c);
#endif
		this->completeConnection(c);
	}
#endif

	//
	// If there is an application to talk to, connect to it.
	//
	if (this->resource.applicationPort != 0)
		this->connectToApplication(this->resource.applicationHost,
		this->resource.applicationPort);


	//
	// If this is a demo license, then post a message after the anchor
	// has been managed.
	//
	if (this->appLicenseType == TimedLicense) {
		ModalWarningMessage(this->anchor->getRootWidget(),
			"You do not have license to run %s.\n"
			"However, you have been given a %d minute demonstration license.\n"
			"\nNote that this license does NOT allow saving visual programs.",
			theDXApplication->getInformalName(), LICENSED_MINUTES);
#ifdef GUILT_MESSAGE
	} else if (this->appLicenseType == Unlicensed) {
		ModalWarningMessage(this->anchor->getRootWidget(),
			"\nYOUR ARE RUNNING AN UNREGISTER COPY OF %s!!!!\n\n"
			"Use of this software is governed by your software license\n"
			"agreement which you, the user, must abide by.  Please contact\n"
			"your sales representative and system administrator to properly\n"
			"register this copy of the software.",
			this->getInformalName());
		this->appLicenseType = FullyLicensed;
#endif // GUILT_MESSAGE
	}

	ASSERT(this->appLicenseType != Unlicensed);

	if (wasSetBusy) this->setBusyCursor(FALSE);

#ifdef	DXD_WIN
	//
	// Connect to exec first 
	//
	if (!this->resource.runUIOnly)
	{
		DXChild *c = this->startServer(); 
		this->completeConnection(c);
	}
#endif

	return TRUE;
}

#if NEED_TO_WATCH_EVENTS
static void
printEvent (Widget w, XtPointer, String actname, XEvent *xev, String *, Cardinal *)
{
    if (xev->type == KeyPress)
	printf ("%s for %s\n", actname, XtName(w));
}
#endif

void DXApplication::handleEvents()
{
    XEvent event;    
    
#if NEED_TO_WATCH_EVENTS
    XtAppAddActionHook (this->applicationContext, (XtActionHookProc)printEvent, NULL);
#endif

    //
    // Process events while the application is running.
    //
    this->runApplication = TRUE;
    while (this->runApplication)
    {
	XtAppNextEvent(this->applicationContext, &event);
	//
	// We check runApplication here because shutdownApplication()
	// delivers a bogus event to kick us out of XtAppNextEvent()
	// when a socket has been closed during a select().
 	// All done for DXLExit().
	//
	if (this->runApplication && XHandler::ProcessEvent(&event))
	{
	    this->handleEvent(&event); 
	}	// Done handling an event

	this->destroyDumpedObjects();

	if (this->serverDisconnectScheduled) {
	    this->serverDisconnectScheduled = FALSE;
	    this->disconnectFromServer();
	}

	//
	// If the user asked the UI to terminate after the first
	// execution (if any) then do so.
	//
	if (this->resource.exitAfter && !this->getExecCtl()->isExecuting())
	    this->shutdownApplication();
    }
}

void
DXApplication::handleEvent (XEvent *xev)
{
#ifdef HAS_CLIPNOTIFY_EXTENSION
    if (this->hasClipnotifyExtension && ((xev->type - this->clipnotifyEventCode) ==
    	    ClipNotifyHdrError))
	{
	char message[1000];
	XClipNotifyHdrErrorEvent *pHdr = (XClipNotifyHdrErrorEvent *)xev;
	sprintf
	    (message,
	     "HPPI header error: "
		"serial# = %d, send_event = %d, window = %d",
	     pHdr->serial, pHdr->send_event, pHdr->window);
	XtWarning(message);
	}
	else if (this->hasClipnotifyExtension &&
	     ((xev->type - this->clipnotifyEventCode) ==
		ClipNotifyDataErrors))
	{
	char message[1000];
	XClipNotifyDataErrorsEvent *pData = (XClipNotifyDataErrorsEvent *) xev;
	sprintf
	    (message, 
	     "HPPI data error:   "
		"serial# = %d, send_event = %d, window = %d,"
		" number of errors = %d, elapsed time in seconds = %d",
	     pData->serial,
	     pData->send_event,
	     pData->window,
	     pData->num_errors,
	     pData->etime);
	XtWarning(message);
        }
#endif
#ifdef sun4
//
// Map Ctl+Delete to Ctl+Del for deleting modules in the VPE
//
    if(xev->type == KeyPress && xev->xkey.keycode == 73  // Delete Key
	&& (xev->xkey.state & ControlMask) != 0 )
	{
	    xev->xkey.keycode = 57;    // Del key
	}
#endif
    this->IBMApplication::handleEvent(xev);
}

static int
testString(char *s)
{
    int r;

    if (!s)
        return 0;

    while(isspace(*s)) s++;

    r = *s != '\0';
    return r;

}

DXChild *DXApplication::startServer()
{

    //
    // Handling Queued events right here, allows the logo screen to become painted
    // before going off to start dxexec.  If it's not painted before fork/execing
    // dxexec, then it will just sit on the screen blank until the exec is done
    // at which point a timeout will immediately remove it from the screen.
    //
    XSync (XtDisplay(this->getRootWidget()), False);
    while (QLength(XtDisplay(this->getRootWidget()))) {
	XEvent event;
	XtAppNextEvent(this->applicationContext, &event);
	switch (event.type) {
	    case ButtonPress:
	    case ButtonRelease:
	    case KeyPress:
	    case KeyRelease:
		break;
	    default:
		this->handleEvent(&event);
		break;
	}
    }
 
    if (this->serverInfo.autoStart)
    {
	char cmd[2048], args[2048];
	if (this->serverInfo.memorySize > 0)
	    sprintf(args, "-exonly -memory %d -local", 
				this->serverInfo.memorySize);
	else
	    strcpy(args, "-exonly -local");

	if (testString(this->serverInfo.executive))
	{
	    strcat(args, " -exec ");
	    strcat(args, this->serverInfo.executive);
	}
	if (testString(this->serverInfo.workingDirectory))
	{
	    strcat(args, " -directory ");
	    if(strchr(this->serverInfo.workingDirectory, ' ') != NULL) {
	    	strcat(args, "\"");
	    	strcat(args, this->serverInfo.workingDirectory);
	    	strcat(args, "\"");
	    } else 
	        strcat(args, this->serverInfo.workingDirectory);
	}
	if (testString(this->serverInfo.userModules))
	{
	    strcat(args, " -mdf ");
	    strcat(args, this->serverInfo.userModules);
	}
#if defined(DXD_LICENSED_VERSION)
	const char *l = NULL;
	if (this->isFunctionalLicenseForced()) {
	    switch (this->getForcedFunctionalLicenseEnum()) {
		case RunTimeLicense:   l = LIC_RT_OPTION; break;
		case DeveloperLicense: l = LIC_DEV_OPTION; break;
	    }	
	} else {
	    switch (this->getFunctionalLicenseEnum()) {
		case RunTimeLicense:   l = LIC_RT_OPTION; break;
		case DeveloperLicense: l = LIC_DEV_OPTION; break;
	    }
	}
	if (l) 
 	{
	    strcat(args, " -license ");
	    strcat(args, l);
	}
#endif
	if (testString(this->serverInfo.executiveFlags))
	{
	    strcat(args, " ");
	    strcat(args, this->serverInfo.executiveFlags);
	}

    sprintf(cmd, "dx %s", args);
	DXChild *c = new DXChild(this->serverInfo.server, cmd, FALSE);
	if (c->failed()) {
	    delete c;
	    sprintf(cmd,"%s/bin/dx %s",this->getUIRoot(),args);
	    c = new DXChild(this->serverInfo.server, cmd, FALSE);
	    // If it still fails, let the connectToServer()  catch it. 
 	}
	
	this->serverInfo.children.insertElement((const void*)c, 1);

	return c;
    }
    else
	return NULL;
}

void DXApplication::QueuedPacketAccept(void *data)
{
    DXPacketIF *p = (DXPacketIF *)data;
    theDXApplication->packetIFAccept(p);
}

void DXApplication::packetIFAccept(DXPacketIF *p)
{
    this->serverInfo.queuedPackets.deleteElement(
	this->serverInfo.queuedPackets.getPosition((const void *)p));
    if (this->serverInfo.packet)
    {
	delete this->serverInfo.packet;
	this->disconnectedFromServerCmd->execute();
    }
    this->serverInfo.packet = p;

    p->initializePacketIO();

    //
    // Once we're all set, do the handshake with the exec to make sure
    // it has the correct license.
    //
    if (this->verifyServerLicense()) {

	//
	// Let the system know that everything's ok.
	this->connectedToServerCmd->execute();
	this->sendNewMDFToServer(theNodeDefinitionDictionary);

	this->getExecCtl()->newConnection();
    }


}


//
// Mark all the networks owned by this application as dirty.
//
void DXApplication::markNetworksDirty()
{
    // This also effectively marks the parameters as dirty.
    this->getExecCtl()->forceFullResend();
}

//
// Utility function to parse the path in the error message.
//
static char* GetErrorNode(char* path, char* name, int*  instance)
{
     char*  nameptr;

     if(sscanf(path,"%[^:]:%d", name, instance) < 2)
     {
	strcpy(name, "\0");
	return NULL;
     }

     if ((nameptr = strchr(name,'_'))
	 AND (strncmp(nameptr,"_Image",6)==0))
		strcpy(name, "Image");

     if ( (path = strchr(path,'/')) )
	path++;

     return (path);
}

void DXApplication::highlightNodes(char* path, int highlightType)
{
    char        name[500], netname[500];
    int         instance;
    Network     *network;
    ListIterator li;
    EditorWindow* editor;

    path = GetErrorNode(path,name,&instance);

    network = this->network;
    if ( (editor = network->getEditor()) )
        editor->highlightNode(name,instance,highlightType);

    while (path)
    {
        li.setList(this->macroList);
	strcpy(netname, name);
	path = GetErrorNode(path,name,&instance);

	for(network = (Network*)li.getNext();
	    network;
	    network = (Network*)li.getNext())
	{
	    if(EqualString(network->getNameString(),netname) &&
	       (editor = network->getEditor()))
	    {
	    	editor->highlightNode(name,instance,highlightType);
	    	break;
	    }
	}
    }
}

void DXApplication::refreshErrorIndicators()
{
    const char *s;
    ListIterator iterator;


    iterator.setList(this->errorList);
    while ( (s = (const char *)iterator.getNext()) )  {
	this->highlightNodes((char*)s,EditorWindow::ERRORHIGHLIGHT);
    }

}
void DXApplication::addErrorList(char *line)

{
	char *newLine = strchr(line, '/');
	char *p, *second_slash;
	//
	// Don't handle errors that result from script commands typed directly to  
	// the executive (i.e. commands without networks) like errors that come
	// from within a network.
	//
	if (newLine && (second_slash = strchr(newLine+1,'/')))
	{
		newLine++;
		const char *name = this->network->getNameString();
		if (!EqualSubstring(newLine,name,STRLEN(name)))
			return;

		char *node_id = second_slash + 1;

		this->errorList.appendElement((void*)DuplicateString(node_id));
		//
		// Don't highlight until after execution.
		// Highlighting is now done in DXExecCtl::endLastExecution() by
		// calling this->refreshErrorIndicators().
		//
		if (!this->getExecCtl()->isExecuting())
			this->highlightNodes(node_id,EditorWindow::ERRORHIGHLIGHT);
	}
	// We don't need to save errors that aren't relevant to a tool placed
	// on the canvas, so comment out the following. 
	//else
	//{
	//j  this->errorList.appendElement((void*)DuplicateString(line));
	//}
	else if ( (p = strstr(line,"PEER ABORT - ")) ) {
		//	
		// One of the distributed peers aborted abnormally.
		// Unhighlight all the modules.
		// FIXME: If we don't unhighlight, the aborted module is still
		// highlighted green, so we could go through the process group
		// and mark the greens modules as error modules.
		//
		Network *n = this->network;
		ListIterator li(this->macroList);
		while (n) {
			EditorWindow *e = n->getEditor();
			if (e)
				e->highlightNodes(EditorWindow::REMOVEHIGHLIGHT);	
			n = (Network*)li.getNext();    
		}
		// this->refreshErrorIndicators();	

		//
		// This is similar to a disconnect, so make sure the user knows.
		//
		ErrorMessage(p+STRLEN("PEER ABORT - "));	
	}

}

void DXApplication::clearErrorList()
{
    char *s;
    ListIterator li(this->errorList);
    while ((s = (char*)li.getNext()))
    {
	this->highlightNodes(s, EditorWindow::REMOVEHIGHLIGHT);
	delete s;
    }
    this->errorList.clear();
}

void DXApplication::QueuedPacketCancel(void *data)
{
    DXPacketIF *p = (DXPacketIF *)data;
    theDXApplication->packetIFCancel(p);
}
void DXApplication::packetIFCancel(DXPacketIF *p)
{
    this->serverInfo.queuedPackets.deleteElement(
	this->serverInfo.queuedPackets.getPosition((const void *)p));
    if (this->serverInfo.packet == p)
    {
	this->serverInfo.packet = NULL;
	this->getExecCtl()->terminateExecution();
	this->disconnectedFromServerCmd->execute();
	this->notifyClients(DXApplication::MsgServerDisconnected);
    }
    this->dumpObject(p);
}

int DXApplication::connectToServer(int port, DXChild *c)
{
    char *server;
    int   wasQueued = FALSE;
    char s[1000];

    if (c)
    {
	server = (char *)c->getServer();
	if ((wasQueued = c->isQueued()) != 0)
	{
	    this->serverInfo.children.deleteElement(
		this->serverInfo.children.getPosition((const void *)c));
	    c->unQueue();
	}
    }
    else
    {
	server = this->serverInfo.server;
    }
    DXPacketIF *p = new DXPacketIF(server, port, DXChild::HostIsLocal(server));

    if (DXApplication::resource.debugMode) 	
        p->setEchoCallback(DXApplication::DebugEchoCallback,(void*)stderr);

    if (p->inError())
    {
	sprintf(s, "Connection to %s failed.", server);
	ErrorMessage(s);

	delete p;
	return FALSE;
    }

    //
    // Now that we have a connection we can clear the error list.
    //
    this->clearErrorList();


    if (this->serverInfo.packet == NULL)
    {
	if (wasQueued)
	{
	    sprintf(s, "Your connection to %s has been accepted.", server);
	    InfoMessage(s); 
	}
	this->packetIFAccept(p);
    }
    else
    {
	this->serverInfo.queuedPackets.insertElement((const void *)p, 1);
	sprintf(s, "Your connection to server %s has been accepted.  Do you want to disconnect from %s and connect to it?", server, this->serverInfo.server);
	theQuestionDialogManager->modalPost(this->getRootWidget(),
					s,
				       NULL,
				       (void *)p,
				       DXApplication::QueuedPacketAccept,
				       DXApplication::QueuedPacketCancel,
				       NULL);
    }
    return (TRUE);
}

void DXApplication::closeConnection(DXChild *c)
{
    if (c->isQueued())
    {
	this->serverInfo.children.deleteElement(
	    this->serverInfo.children.getPosition((const void *)c));
	// From now on we'll queue deletion of c, in a way that ensures it will
	// actually get deleted.  Currently it's cast adrift in normal cases.
	// If there turns out to be a problem with this, then look for
	// DXChild_DeleteObjectWP and stop using it or back out the change to
	// DXChild.C
	//delete c;
    }
}

boolean DXApplication::disconnectFromServer()
{

    if (this->serverInfo.packet != NULL)
	this->packetIFCancel(this->serverInfo.packet);

    //
    // Only clear the error list when we ask to be disconnected
    // (i.e. don't do it in packetIFCancel() which gets called when
    // unexpectedly disconnected from the server). 
    //
    this->clearErrorList();

    //
    // Make sure we don't disconnect on the next connection.
    //
    this->serverDisconnectScheduled = FALSE;

    return TRUE;
}


boolean DXApplication::openFile(const char *netfile, const char *cfgfile,
				boolean resetTheServer)
{
    Network *network = this->network;
    boolean execOnChange = this->getExecCtl()->inExecOnChange();
    boolean result;

    if (execOnChange)
	this->getExecCtl()->suspendExecOnChange();

    if (resetTheServer)
	this->resetServer();

    //
    // We can't do this until after resetting the server, because
    // it has to clean up its windows too.
    //
    network->clear();

    if ((result = network->readNetwork(netfile, cfgfile)))
    {
	if (execOnChange)
	{
	    this->getExecCtl()->updateMacros();
	    this->getExecCtl()->resumeExecOnChange();
	}
	this->appendReferencedFile(netfile);
    }

    this->readFirstNetwork = TRUE;
    return result;
}

boolean DXApplication::saveFile(const char *netfile, boolean force)
{
    const char *name = netfile;
    Network *network = this->network;

    if (! name)
	name = network->getFileName();
    
    if (! name)
    {
	ErrorMessage("unable to save file");
	return FALSE;
    }

    return  network->saveNetwork(name, force);
}

void DXApplication::DebugEchoCallback(void *clientData, char *echoString)
{
    FILE *f = (FILE*)clientData;

    //
    // We use this test for null as a bit of a hack, but if NULL then
    // we assume that the callback is for the ApplicIF and not DXPacketIF.
    //
    if (f)
	fputs(echoString, f);
    else
 	fprintf(stderr,"DXLink::%s",echoString);
}


boolean DXApplication::postStartServerDialog()
{
    if (this->startServerDialog == NULL)
	this->startServerDialog = new StartServerDialog(this->getRootWidget());
    this->markNetworksDirty();
    this->startServerDialog->post();
    return TRUE;
}
//
// This should be moved to "DXExecCtl"
boolean DXApplication::resetServer()
{
    char message[1024];

    this->getExecCtl()->terminateExecution();

    this->clearErrorList();

    this->markNetworksDirty();
    this->notifyClients(DXApplication::MsgServerDisconnected);

    DXPacketIF *pif = this->getPacketIF();
    if (pif) {		// pif should really never be NULL, but...
	

	this->flushExecutiveDictionaries(pif);

	pif->send(DXPacketIF::FOREGROUND, "Trace(\"memory\", 0);\n" );

	sprintf(message,"Executive(\"version %d %d %d\");\n",
	 	   PIF_MAJOR_VERSION,PIF_MINOR_VERSION, PIF_MICRO_VERSION);
	pif->send(DXPacketIF::FOREGROUND, message);

	sprintf(message,"Executive(\"product version %d %d %d\");\n",
                   DX_MAJOR_VERSION, DX_MINOR_VERSION, DX_MICRO_VERSION); 
	pif->send(DXPacketIF::FOREGROUND, message);


    }


    return TRUE;
}

//
// Send the command to the flush the dictionaries. 
//
void DXApplication::flushExecutiveDictionaries(DXPacketIF *pif)
{
    ASSERT(pif);
    pif->send(DXPacketIF::FOREGROUND, "Executive(\"flush dictionary\");\n");
}

void DXApplication::getServerParameters(int 	 *autoStart,
					const char    **server,
					const char    **executive,
					const char    **workingDirectory,
					const char    **executiveFlags,
					int	 *port,
					int	 *memorySize)
{
    if (autoStart)
	*autoStart         = this->serverInfo.autoStart;
    if (server)
	*server            = this->serverInfo.server;
    if (executive)
	*executive         = this->serverInfo.executive;
    if (workingDirectory)
	*workingDirectory  = this->serverInfo.workingDirectory;
    if (executiveFlags)
	*executiveFlags    = this->serverInfo.executiveFlags;
    if (port)
	*port              = this->serverInfo.port;
    if (memorySize)
	*memorySize        = this->serverInfo.memorySize;
}
void DXApplication::setServerParameters(int 	 autoStart,
					const char    *server,
					const char    *executive,
					const char    *workingDirectory,
					const char    *executiveFlags,
					int	 port,
					int	 memorySize)
{
    char *tserver =
	server? DuplicateString(server): NULL;
    char *texecutive =
	executive? DuplicateString(executive): NULL;
    char *tworkingDirectory =
	workingDirectory? DuplicateString(workingDirectory): NULL;
    char *texecutiveFlags =
	executiveFlags? DuplicateString(executiveFlags): NULL;

    if (this->serverInfo.server) 
        delete this->serverInfo.server;
    if (this->serverInfo.executive)
	delete this->serverInfo.executive;
    if (this->serverInfo.workingDirectory)
	delete this->serverInfo.workingDirectory;
    if (this->serverInfo.executiveFlags)
	delete this->serverInfo.executiveFlags;

    this->serverInfo.autoStart        = autoStart;
    this->serverInfo.server	      = tserver;
    this->serverInfo.executive        = texecutive;
    this->serverInfo.workingDirectory = tworkingDirectory;
    this->serverInfo.executiveFlags   = texecutiveFlags;
    this->serverInfo.port             = port;
    this->serverInfo.memorySize       = memorySize;
}

void DXApplication::completeConnection(DXChild *c)
{
    if (c != NULL)
    {
	char s[1000];
	switch(c->waitForConnection()) {
	case -1:
	    sprintf(s, "Connection to server %s failed:\n\n%s", 
		c->getServer(), c->failed());
	    ErrorMessage(s);
	    this->closeConnection(c);
	    break;

	case 0:
	    if (this->resource.executeProgram)
	    {
		this->getExecCtl()->executeOnce();
	    }
	    break;

	case 1:
	    sprintf(s, "Connection to server %s has been queued", 
		    c->getServer());
	    InfoMessage(s);
	    break;
	}
    }
    else if (!this->serverInfo.autoStart)
    {
	this->connectToServer(this->serverInfo.port, NULL);
    }

    //
    // Let the application packet know that a new connection to
    // the server has been created. 
    //
    if (this->applicationPacket)
	this->applicationPacket->handleServerConnection();


    if (DXApplication::resource.executeOnChange)
	this->getExecCtl()->enableExecOnChange();
}

void DXApplication::postOpenNetworkDialog()
{
    if (this->openNetworkDialog == NULL) {
        this->openNetworkDialog = new OpenNetworkDialog(
				this->anchor->getRootWidget());

	if (this->appLimitsNetFileSelection()) {
	    ASSERT(this->resource.netPath);
	    this->openNetworkDialog->setReadOnlyDirectory(
			this->resource.netPath);
	}

    }
    this->openNetworkDialog->post();
}

void DXApplication::postLoadMacroDialog()
{
    if (this->loadMacroDialog == NULL) {
        this->loadMacroDialog = new LoadMacroDialog(
            				this->anchor->getRootWidget());
    }

    this->loadMacroDialog->post();
}


MsgWin *DXApplication::newMsgWin()
{
    return new MsgWin();
}
ImageWindow *DXApplication::newImageWindow(Network *n)
{
    boolean is_anchor;

    ASSERT(n);

    if (this->anchor == NULL)
	is_anchor = TRUE;
    else
	is_anchor = FALSE;

    return new ImageWindow(is_anchor, n);
}

ControlPanel *DXApplication::newControlPanel(Network *n)
{
    return new ControlPanel(n);
}

Network *DXApplication::newNetwork(boolean nonJava)
{
    Network* n;
    if (nonJava)
	n = new Network();
    else
	n = new JavaNet();
    return n;
}

//
// Create a new network editor.  
// This particular implementation, makes the returned editor an anchor
// if this->anchor is NULL. 
// This may return NULL in which case a message dialog is posted to the user.
//
EditorWindow *DXApplication::newNetworkEditor(Network *n)
{
    char *msg = "";

    ASSERT(n);

    if (n->wasNetFileEncoded()) {
	msg = "This visual program is encoded and therefore is not "
			"viewable in the VPE";
	goto error;
    } else if (!this->appAllowsEditorAccess()) {
	msg = "This invocation of Data Explorer does not allow editor "
		"access (-edit).\n"
	      "Try image mode (-image), or forcing a developer license "
		"(-license develop).\n";
	goto error;		
    }

    boolean is_anchor;
    if (this->anchor == NULL)
	is_anchor = TRUE;
    else
	is_anchor = FALSE;

    return new EditorWindow(is_anchor, n);

error:
    if (this->anchor)
	InfoMessage(msg);
    else
	fprintf(stderr,msg);

    return NULL;
}

void DXApplication::connectToApplication(const char *host, const int port)
{
    if (host == NULL)
	host = "localhost";

    this->applicationPacket = new ApplicIF(host, port,
					DXChild::HostIsLocal(host));

    this->applicationPacket->initializePacketIO();

    if (DXApplication::resource.debugMode) 	
        this->applicationPacket->setEchoCallback(
			DXApplication::DebugEchoCallback,(void*)NULL);
}

void DXApplication::disconnectFromApplication(boolean terminate)
{
    if (this->applicationPacket) {
	this->dumpObject(this->applicationPacket);
	this->applicationPacket = NULL;
    }

    if (terminate)
	this->shutdownApplication();
}


//
// Message control functions.
//
boolean	DXApplication::isInfoEnabled()
{
    return this->resource.infoEnabled;
}
boolean	DXApplication::isWarningEnabled()
{
    return this->resource.warningEnabled;
}
boolean	DXApplication::isErrorEnabled()
{
    return this->resource.errorEnabled;
}
boolean	DXApplication::doesInfoOpenMessage(boolean fromModule)
{
    return (this->resource.infoOpensMessage ||
    	    (fromModule && this->resource.moduleInfoOpensMessage)) &&
	   this->isInfoEnabled();
}
boolean	DXApplication::doesWarningOpenMessage()
{
    return this->resource.warningOpensMessage && this->isWarningEnabled();
}
boolean	DXApplication::doesErrorOpenMessage()
{
    return this->resource.errorOpensMessage && this->isErrorEnabled();
}
int DXApplication::doesErrorOpenVpe(Network *net)
{
    if ((!this->isErrorEnabled()) ||
	(net->wasNetFileEncoded()) ||
	(!this->appAllowsEditorAccess())) 
	return DXApplication::DontOpenVpe;

    if (this->resource.noEditorOnError) {
#if DONT_PROMPT_THE_USER
	return DXApplication::DontOpenVpe;
#else
	return DXApplication::MayOpenVpe;
#endif
    }

    return DXApplication::MustOpenVpe;
}
void	DXApplication::enableInfo(boolean enable)
{
    this->resource.infoEnabled = enable;
}
void	DXApplication::enableWarning(boolean enable)
{
    this->resource.warningEnabled = enable;
}
void	DXApplication::enableError(boolean enable)
{
    this->resource.errorEnabled = enable;
}


void DXApplication::dumpObject(Base *object)
{
     this->dumpedObjects.appendElement((void*)object);

}


void DXApplication::destroyDumpedObjects()
{
     Base *object;
     ListIterator li(this->dumpedObjects);

     while( (object = (Base*)li.getNext()) )
	delete object;

     this->dumpedObjects.clear();
}

void DXApplication::postLoadMDFDialog()
{
    if (this->loadMDFDialog == NULL) {
        this->loadMDFDialog = new LoadMDFDialog( this->anchor->getRootWidget(),
					 this);
    }

    this->loadMDFDialog->post();

}

void DXApplication::postProcessGroupAssignDialog()
{
    if (NOT this->processGroupAssignDialog)
	this->processGroupAssignDialog =
	      new ProcessGroupAssignDialog(this);

    this->processGroupAssignDialog->post();
}

//
// Get selected NodeDefinitoins from the new dictionary and send
// their representative MDF specs to the server if they are 
//  1) outboard
//  2) dynamically loaded
//  3) outboardness or dynmacity changes compared to the possibly 
//      existing definition in the current dictionary.
//
void DXApplication::sendNewMDFToServer(Dictionary *newdefs, Dictionary *current)
{
    char *buf = NULL;
    boolean sent_stuff = FALSE;
    DXPacketIF *pif = this->getPacketIF();

    if (!pif)
	return;

    ASSERT(newdefs);
    NodeDefinition *nd;
    DictionaryIterator di(*newdefs);
    while ( (nd = (NodeDefinition*)di.getNextDefinition()) ) {
	char *mdf;
	int  buflen = 0;
	boolean outboard = nd->isOutboard();
  	boolean dynamic = nd->isDynamicallyLoaded();
	// The following requires that the exec sees the -mdf options that
	// the UI sees.  
        boolean send = (outboard || dynamic) && nd->isUILoadedOnly(); 
	//
	// See if a definition already existed and if the outboardness or 
	// the dynamicity changes (e.g. outboard -> inboard) we must resend
	// the new definition.
	//
  	if (!send && current) {
	    Symbol s = nd->getNameSymbol();
	    NodeDefinition *olddef = (NodeDefinition*)
					current->findDefinition(s); 
	    if (olddef) {
		//
		// Resend if the outboardness or the dynmacity of the module
		// changes.
		//
		send =  (olddef->isOutboard() != outboard) ||
			    (olddef->isDynamicallyLoaded() != dynamic);
	    }
	}
	//
	// If we're supposed to send this definition and the node has one,
	// then lets send it.
	//
	if (send && (mdf = nd->getMDFString())) {

	    int mdflen = STRLEN(mdf) + 128;	// 64 for "Executive(..."
	    if (buflen <= mdflen) {
		buf = (char*)REALLOC(buf, mdflen + 1);
		buflen = mdflen + 1;
	    }
	    // 
	    //  Copy mdf into buf, expanding newlines in to "\n" and
	    //  " into \"
	    // 
	    strcpy(buf,"Executive(\"mdf string\",\"");
	    char *p = buf, *m = mdf;
	    p += STRLEN(buf);
	    while (*m) {
		if (*m == '\n') {
		    *p = '\\'; 
		    p++;
		    *p = 'n';
		} else if (*m == '"') {
		    *p = '\\'; 
		    p++;
		    *p = '"';
		} else {
		    *p = *m;
		}
		p++;
		m++;
	    }
	    *p = '\0';
	    strcat(p,"\");\n");
	    pif->send(DXPacketIF::FOREGROUND, buf); 
	    delete mdf;
	    sent_stuff = TRUE;
    	}
    }

    if (buf) {
	// 
	//  Delete the buffer 
	//
	delete buf;
    }

    const char *file;
    int i;
    for (i=1 ; 
	 (file = (const char*)theDynamicPackageDictionary->getStringKey(i)) ;
	 i++) {
	char sbuf[1024];
	sprintf(sbuf,"Executive(\"package\",\"%s\");\n",file);
	pif->send(DXPacketIF::FOREGROUND, sbuf); 
        sent_stuff = TRUE;
    }

    if (sent_stuff) {
	//
	// Make sure the executive processes the Executive() calls
	// before parsing ahead.
	//
	pif->sendImmediate("sync");
    }

}

#define FULLY_RESTRICTED 	1 
#define SOMEWHAT_RESTRICTED  	2	
#define MINIMALLY_RESTRICTED  	3	
//
// Define a set of routines that are used to set the level of customization
// and/or the number of features provided by the user-interface. 
//

//
// Define a mapping of integer levels and the restriction level names and
// determine if the current restriction level is at the given level.
// The highest level of restriction is 1 the lowest and +infinity. 
//
// Currently, we map the following to restriction levels...
// 
//  Level 1 - "C"
//  Level 2 - "B"
//  Level 3 - "A"
// 
//
boolean DXApplication::isRestrictionLevel(int level)
{
    const char *rlev = this->resource.restrictionLevel;

    ASSERT(level > 0);

    if (!rlev)
	return FALSE;

    switch (level) {
	case 3: if (EqualString("minimum",rlev)) return TRUE; 
	case 2: if (EqualString("intermediate",rlev)) return TRUE; 
	case 1: if (EqualString("maximum",rlev)) return TRUE; 
	default:
	    return FALSE;
    }
    return FALSE;

}
boolean DXApplication::appAllowsDXHelp() 
{ 
    return !this->resource.noDXHelp  && !this->getOEMApplicationName();
} 
boolean DXApplication::appAllowsPanelEdit() 
{ 
    return !this->isRestrictionLevel(SOMEWHAT_RESTRICTED) 	&& 
	   !this->resource.noPanelEdit;
} 
boolean DXApplication::appAllowsRWConfig()
{ 
    return !this->isRestrictionLevel(FULLY_RESTRICTED) 	 && 
	   !this->resource.noRWConfig;
}
boolean DXApplication::appAllowsPanelAccess()
{ 
    return !this->isRestrictionLevel(FULLY_RESTRICTED) 	 &&
	   !this->resource.noPanelAccess; 
} 
boolean DXApplication::appAllowsOpenAllPanels()
{ 
    return !this->isRestrictionLevel(SOMEWHAT_RESTRICTED) 	 &&
	   !this->resource.noOpenAllPanels; 
} 
boolean DXApplication::appAllowsPanelOptions()
{ 
    return !this->isRestrictionLevel(SOMEWHAT_RESTRICTED) 	&& 
	   !this->resource.noPanelOptions; 
}

boolean DXApplication::appAllowsInteractorSelection()
{ 
    return this->appAllowsPanelEdit() ||
	   this->appAllowsInteractorAttributeChange() ||
	   this->appAllowsInteractorEdits();
}
boolean DXApplication::appAllowsInteractorMovement()
{ 
    return this->appAllowsPanelEdit() 				&& 
    	   !this->isRestrictionLevel(MINIMALLY_RESTRICTED) 	&& 
    	   !this->resource.noInteractorMovement;
}
boolean DXApplication::appAllowsInteractorAttributeChange()
{ 
    return !this->isRestrictionLevel(SOMEWHAT_RESTRICTED) 	&& 
	   this->appAllowsPanelEdit()  				&&
	   !this->resource.noInteractorAttributes; 
}
boolean DXApplication::appAllowsInteractorEdits()
{ 
    return !this->isRestrictionLevel(MINIMALLY_RESTRICTED) 	&&
    	   this->appAllowsPanelEdit() 				&&
	   !this->resource.noInteractorEdits; 
}

boolean DXApplication::appLimitsNetFileSelection()
{ 
    return this->resource.limitedNetFileSelection; 
}
boolean DXApplication::appAllowsImageRWNetFile()
{ 
    return !this->isRestrictionLevel(SOMEWHAT_RESTRICTED) 	&& 
     	   !this->resource.noImageRWNetFile; 
}
boolean DXApplication::appAllowsSavingNetFile(Network *n)
{
    return this->appAllowsSavingCfgFile() &&
	   (!n || !n->wasNetFileEncoded());
}
boolean DXApplication::appAllowsSavingCfgFile()
{
     return (this->appLicenseType != TimedLicense) &&
	    (this->inEditMode() || this->appAllowsImageRWNetFile());
}
boolean DXApplication::appAllowsImageLoad()
{ 
    return !this->isRestrictionLevel(SOMEWHAT_RESTRICTED) 	&& 
           !this->resource.noImageLoad; 
}
boolean DXApplication::appAllowsImagePrinting()
{ 
    return !this->resource.noImagePrinting;
}
boolean DXApplication::appAllowsImageSaving()
{ 
    return !this->isRestrictionLevel(FULLY_RESTRICTED) 	 &&
    	   !this->resource.noImageSaving;
}
boolean DXApplication::appLimitsImageOptions()
{ 
    return this->isRestrictionLevel(MINIMALLY_RESTRICTED)  	||	
           this->resource.limitImageOptions; 
}


boolean DXApplication::appAllowsEditorAccess()
{ 
    LicenseTypeEnum func = this->getFunctionalLicenseEnum();

    //
    // If the license hasn't yet been acquired, then try the OEM
    // license codes.
    //
    if (func == UndeterminedLicense) 
	this->verifyOEMLicenseCode(&func);

    return !this->isRestrictionLevel(MINIMALLY_RESTRICTED) 	&&
	   !this->resource.noEditorAccess &&
	   (func != RunTimeLicense); 
}

boolean DXApplication::appAllowsPGroupAssignmentChange()
{ 
    return !this->isRestrictionLevel(MINIMALLY_RESTRICTED) 	&&
	    !this->resource.noPGroupAssignment; 
}
boolean DXApplication::appAllowsMessageInfoOption()
{
    return  !this->resource.noMessageInfoOption; 
}
boolean DXApplication::appAllowsMessageWarningOption()
{
    return  !this->resource.noMessageWarningOption; 
}
boolean DXApplication::appAllowsScriptCommands()
{ 
    return !this->isRestrictionLevel(MINIMALLY_RESTRICTED) 	&&
	   !this->resource.noScriptCommands; 
}
boolean DXApplication::appAllowsCMapSetName()
{ 
    return !this->isRestrictionLevel(MINIMALLY_RESTRICTED) 	&& 
           !this->resource.noCMapSetNameOption; 
}
boolean DXApplication::appAllowsCMapOpenMap()
{ 
    return !this->isRestrictionLevel(FULLY_RESTRICTED) 	&& 
    	   !this->resource.noCMapOpenMap; 
}
boolean DXApplication::appAllowsCMapSaveMap()
{ 
    return !this->isRestrictionLevel(FULLY_RESTRICTED) 	&& 
           !this->resource.noCMapSaveMap; 
}
boolean DXApplication::appSuppressesStartupWindows()
{ 
    return this->resource.suppressStartupWindows;
}
boolean DXApplication::appAllowsExitOptions()
{ 
    return !this->resource.noExitOptions;
}
boolean DXApplication::appAllowsExecuteMenus()
{ 
    return !this->resource.noExecuteMenus;
}
boolean DXApplication::appAllowsConnectionMenus()
{ 
    return !this->resource.noConnectionMenus;
}
boolean DXApplication::appAllowsWindowsMenus()
{ 
    return !this->resource.noWindowsMenus;
}
boolean DXApplication::appAllowsImageMenus()
{ 
    return !this->resource.noImageMenus;
}
boolean DXApplication::appAllowsConfirmedQuit()
{ 
    return !this->resource.noConfirmedQuit;
}

//
// See if there is an OEM license code (see oemApplicationName and
// oemLicenseCode resources) and if so see if it is valid.
// If so return TRUE and set *func, otherwise return FALSE and leave
// *func untouched.
//
boolean DXApplication::verifyOEMLicenseCode(LicenseTypeEnum *func)
{
    char cryptbuf[1024];
    boolean r;

    ASSERT(func);

    if (!this->resource.oemLicenseCode || !this->getOEMApplicationName()) 
	return FALSE;

    ScrambleAndEncrypt(this->resource.oemApplicationName,"DEV",cryptbuf);
    if (EqualString(cryptbuf,this->resource.oemLicenseCode)) {
	*func = DeveloperLicense;
        r = TRUE;
    } else {
        ScrambleAndEncrypt(this->resource.oemApplicationName,"RT",cryptbuf);
	if (EqualString(cryptbuf,this->resource.oemLicenseCode)) {
	    *func = RunTimeLicense;
	    r = TRUE;
	} else 
	    r = FALSE;
    }

    return r;
    
}
//
// Check the oemApplicationName and oemApplicationNameCode resources and see
// if an alternate application name can be verified.  If so, return it
// otherwise return NULL. 
//
const char *DXApplication::getOEMApplicationName()
{
    char cryptbuf[1024];

    if (!this->resource.oemApplicationName || 
	!this->resource.oemApplicationNameCode)
	return NULL;
 
    if (STRLEN(this->resource.oemApplicationName) < 3)
	return NULL;

    ScrambleAndEncrypt(this->resource.oemApplicationName,"DX",cryptbuf);
    if (EqualString(cryptbuf,this->resource.oemApplicationNameCode))
	return this->resource.oemApplicationName; 
    else
	return NULL;
}
//
// Return the name of the application (i.e. 'Data Explorer',
// 'Data Prompter', 'Medical Visualizer'...).
// If getOEMApplicationName() return !NULL, then use that. 
//
const char *DXApplication::getInformalName()
{
    const char *s = this->getOEMApplicationName();
    if (!s) s = "Data Explorer";
    return s;
}
//
// Return the formal name of the application (i.e. 
// 'Open Visualization Data Explorer', 'Open Visualization Data Prompter'...)
// If getOEMApplicationName() return !NULL, then use that. 
//
const char *DXApplication::getFormalName()
{
    const char *s = this->getOEMApplicationName();
    if (!s) s = "Open Visualization Data Explorer";
    return s;
}
//

// Get the applications copyright notice, for example...
// "Copyright International Business Machines Corporation 1991-1993
// All rights reserved"
// If getOEMApplicationName() return !NULL, then don't use a copyright
// (i.e. return NULL). 
//


const char *DXApplication::getCopyrightNotice()
{
    const char *s = this->getOEMApplicationName();
    if (!s) 
	return DXD_COPYRIGHT_STRING;
    else
	return NULL;
}

//
// The new 3rd anchor supplies the copyright message itself.  If using such an
// anchor then use only that copyright else use superclass.
//
void DXApplication::postCopyrightNotice()
{
    if ((this->anchor) && 
	(strcmp (this->anchor->getClassName(), ClassDXAnchorWindow) == 0)) {
	DXAnchorWindow *dxa = (DXAnchorWindow*)this->anchor;
	dxa->postCopyrightNotice();
    } else {
	this->IBMApplication::postCopyrightNotice();
    }
}


//
// Get the default image printing/saving options specified through
// command line options or resources.
//
String      DXApplication::getPrintImageFormat() 
{  	return this->resource.printImageFormat; }
String      DXApplication::getPrintImagePageSize()
{	return this->resource.printImagePageSize; }
String      DXApplication::getPrintImageSize()
{	return this->resource.printImageSize; }
int         DXApplication::getPrintImageResolution()
{	return this->resource.printImageResolution; }
String      DXApplication::getPrintImageCommand()
{	return this->resource.printImageCommand; }
String      DXApplication::getSaveImageFormat()
{	return this->resource.saveImageFormat; }
String      DXApplication::getSaveImagePageSize()
{	return this->resource.saveImagePageSize; }
String      DXApplication::getSaveImageSize()
{	return this->resource.saveImageSize; }
int         DXApplication::getSaveImageResolution()
{	return this->resource.saveImageResolution; }

void DXApplication::notifyPanelChanged()
{
    this->notifyClients(DXApplication::MsgPanelChanged);
}

//
// Tell the exec to get a license or not based on the type of license
// that we have. 
//
boolean DXApplication::verifyServerLicense()
{
#if defined(DXD_LICENSED_VERSION)
    DXPacketIF *pif = this->getPacketIF();
    if (!pif)
	return FALSE;

    pif->setHandler(DXPacketIF::INFORMATION,
		  DXApplication::LicenseMessage,
		  (void*)this,
		  "LICENSE:");

    pif->sendImmediate("getkey"); // sends back -> 'LICENSE: abcd'
				  // or 'LICENSE: UNAUTHORIZED 
   
    //
    // Wait until we get the AUTHORIZED or UNAUTHORIZED message.
    //
    void *licensed;
    boolean r;
    if (pif->receiveContinuous(&licensed))
	r = (boolean)licensed;
    else
	r = FALSE;

    return r;
#else
    return TRUE;
#endif

}

#if defined(DXD_LICENSED_VERSION)
//
// Catch the 'LICENSE:' message from the executive and shake hands with
// it to make sure it got the correct license based on the type of
// license that the UI has. 
//
void DXApplication::LicenseMessage(void *, int, void *ptr)
{
    char *p = (char*)ptr;
    static char *lic_token = "LICENSE:";
    DXPacketIF *pif = theDXApplication->getPacketIF();
    

    if (p = strstr(p, lic_token)) {
	p += STRLEN(lic_token);
	SkipWhiteSpace(p);
	if (strstr(p,"AUTHORIZED")) {
	    boolean licensed;
	    if (EqualString(p,"UNAUTHORIZED")) {
		licensed = FALSE;
		theDXApplication->scheduleServerDisconnect();
		InfoMessage("A license for the server is not available.  "
			     "Server disconnected.");
	    } else {
		licensed = TRUE;
	    }
		
	    if (pif) // Force ::verifyServerLicense() to return;
		pif->endReceiveContinuous((void*)licensed);
	} else if (strstr(p,"NO LICENSE")) {
	    ;
	} else if (p) { 	// FIXME: this could be improved
	    // Expecting 'LICENSE: xxxx' in *p
	    LicenseTypeEnum lic = theDXApplication->appLicenseType; 
	    if (lic == TimedLicense)
		lic = ConcurrentLicense;
	    char *outkey = GenerateExecKey(p,lic);
	    if (outkey) {
		char buf[1024];
		sprintf(buf,"license %s",outkey);
		DXPacketIF *pif = theDXApplication->getPacketIF();
		pif->sendImmediate( buf); 
		delete outkey;
	    }
	}
    } else {
	fprintf(stderr,"Unauthorized LICENSE message...exiting.\n");
	exit (1);
    }
    
}

#endif

void DXApplication::scheduleServerDisconnect()
{
    this->serverDisconnectScheduled = TRUE;
}

//
// Manage the software licenses
//
extern "C" void ShutdownTimeout(XtPointer clientData, XtIntervalId *id);
extern "C" void InstallShutdownTimer(Widget w, 
				XtPointer clientData, XtPointer calldata);
extern "C" void ShutdownApplication(Widget w, 
				XtPointer clientData, XtPointer callData);

void DXApplication::determineUILicense(LicenseTypeEnum *app, 
					LicenseTypeEnum *func)
{
#if defined(DXD_LICENSED_VERSION)  && DXD_LICENSED_VERSION!=0

# ifdef DEBUG
    if (!theDXApplication->appAllowsInteractorMovement()) {
	*app = Unlicensed; 
	*func = RunTimeLicense; 
    } else if (!theDXApplication->appAllowsPanelEdit()) {
	*app = ConcurrentLicense; 
	*func = RunTimeLicense; 
    } else if (!theDXApplication->appAllowsPanelAccess()) {
	*app = NodeLockedLicense; 
	*func = RunTimeLicense; 
    } else if (!theDXApplication->appAllowsCMapSaveMap()) {
	*app = TimedLicense;
	*func = RunTimeLicense; 
    } else {
# endif
	LicenseTypeEnum oemLic = Unlicensed;
	*app = Unlicensed;
	if (this->isFunctionalLicenseForced())
	    *func = this->getForcedFunctionalLicenseEnum();
	else
	    *func = Unlicensed;
	if (*func == TimedLicense) {
	    *app = TimedLicense;
	    *func = DeveloperLicense;
	} else if (this->verifyOEMLicenseCode(&oemLic) && 
			((*func == Unlicensed) || (oemLic == *func))) {
	    *app = FullyLicensed;
	} else
	    UIGetLicense(this->getUIRoot(), LostLicense, app,func);
# ifdef DEBUG
    }
# endif

#else
#if !defined(DXD_WIN) || 1
    *app  = FullyLicensed;
    *func = DeveloperLicense;
#else
    UIGetLicense(this->getUIRoot(), LostLicense, app,func);
#endif
#endif // DXD_LICENSED_VERSION
    this->appLicenseType = *app; 
    this->funcLicenseType = *func;
}
boolean DXApplication::isFunctionalLicenseForced()
{ 
    return this->resource.forceFunctionalLicense != NULL;
}
LicenseTypeEnum DXApplication::getFunctionalLicenseEnum()
{
    if (this->funcLicenseType == UndeterminedLicense) {	// not determined yet
	if (this->isFunctionalLicenseForced())
	    return this->getForcedFunctionalLicenseEnum();
	else 
	    return UndeterminedLicense; 
    } else {
	return this->funcLicenseType; 
    }
}
LicenseTypeEnum DXApplication::getForcedFunctionalLicenseEnum()
{

    if (!this->isFunctionalLicenseForced())
	return FullFunctionLicense; 
    const char *s = this->resource.forceFunctionalLicense;
    if (EqualString(s, LIC_RT_OPTION))
	return RunTimeLicense;
    else if (EqualString(s,LIC_DEV_OPTION))
	return DeveloperLicense;
    else if (EqualString(s,LIC_TIMED_OPTION))
	return TimedLicense;
    else if (EqualString(s,LIC_VIEWER_OPTION))
	return ViewerLicense;
    else
	return FullFunctionLicense; 	// User gave unrecognized license name
}


//
// The number of seconds between the first and second warnings about
// losing the license 
//
#ifdef DEBUG
# define MAX_INTERVAL 16	
#else
# define MAX_INTERVAL 120	
#endif
//
// The minimum interval between warnings for which another warning will
// be scheduled instead of saving files automatically. 
//
#ifdef DEBUG
# define MIN_INTERVAL	8	
#else
# define MIN_INTERVAL	30	
#endif
static char *format_seconds(int seconds)
{
    static char buf[64];
    int minutes, frac = seconds % 60;

    if (seconds > 60)
	minutes = seconds/60; 
    else
	minutes = 0;

    if (minutes == 0)
	sprintf(buf,"%d seconds", seconds);
    else if (frac > 0)
	sprintf(buf,"%d minute%s %d seconds",
	    minutes, minutes == 1? "": "s", frac);
    else
	sprintf(buf,"%d minute%s", minutes, minutes == 1? "": "s");

    return buf;
}
//
// If seconds is greater then 30, then we issue a warning
// and install this routine as call back for the ok button so that another
// timeout is installed after the ok is hit. 
// If seconds is positive and less than 30 seconds, then we save the
// user's work and shutdown the application. 
//
extern "C" void ShutdownTimeout(XtPointer clientData, XtIntervalId *id)
{
    char buffer[1024];	
    int seconds = (int)(long) clientData;
    boolean can_save = theDXApplication->appAllowsSavingNetFile();

    if (seconds > MIN_INTERVAL ) {
	seconds = (int)(seconds/2.0);
	sprintf(buffer,
        "You do not have a license to run %s.\n"
	"You have %s %s",
		theApplication->getInformalName(),
		format_seconds(seconds),
		(!can_save ? "before your session is terminated." :
			     "to save your work and exit."));

	theWarningDialogManager->modalPost(
		    theDXApplication->getAnchor()->getRootWidget(),
		    buffer,
		    "WARNING: User Interface License",
		    (void*) seconds,
		    NULL,	// Ok button
		    NULL);	// Close manager options menu
	Widget w = XtParent(theWarningDialogManager->getRootWidget());
	ASSERT(XtHasCallbacks(w,XmNpopdownCallback));
        XtRemoveCallback(w,XmNpopdownCallback, 
			InstallShutdownTimer, (XtPointer)(seconds*2));
	XtAddCallback(w,XmNpopdownCallback, 
			InstallShutdownTimer, (XtPointer)seconds);
    } else if (can_save) {
	ListIterator li;
	int i;
	boolean save_files = FALSE;
	Network *n;
#ifdef DXD_OS_NON_UNIX
	// FIXME: suits should use ENV variables to fine tmp path
        // char *dirs[2] = { "\\tmp", NULL };
	const char *dirs[2];
	dirs[0] = theDXApplication->getTmpDirectory();
	dirs[1] = NULL;
#else
# if defined(aviion) || defined(hp700)
	// These guys won't do aggregate initialization of automatics
	char *dirs[3];
	dirs[0] = "/tmp";
	dirs[1] = "/usr/tmp";
	dirs[2] = NULL; 
# else
	char *dirs[] = { "/tmp", "/usr/tmp" , NULL};
# endif
#endif
	// 
	//  Check to see if there are files (networks) that need saving.
	// 
	if (theDXApplication->network->saveToFileRequired() &&
		theDXApplication->network->getNodeCount() > 0)
		save_files = TRUE;

	if (!save_files) {
	    li.setList(theDXApplication->macroList);
	    while ( (n = (Network*)li.getNext()) )
		if (n->saveToFileRequired() || (n->getNodeCount() > 0))
		    save_files = TRUE;
	}
	
	// 
	// Save the networks 
	// 
	if (save_files) {
	    char msg[4096];
	    char buf[1024];
	    boolean write_error = TRUE;
	    strcpy(msg,"The following files have been written...\n\n");
	    for (i=0 ; write_error && dirs[i] ; i++) {	
		int cnt;
		li.setList(theDXApplication->macroList);
		for (n=theDXApplication->network, cnt=0, write_error = FALSE;
		     	n && !write_error ;
		     	n = (Network*)li.getNext())
		{
		    if (n->saveToFileRequired()) {
			const char *filename = n->getFileName();
			char *basename; 
			if (filename) {
			    basename = GetFileBaseName(filename, NULL);
			} else {
			    basename = new char[32]; 
			    sprintf(basename,"network_%d",++cnt);
			}
			sprintf(buf,"%s/%s",dirs[i],basename);
			strcat(msg,"\t");
			strcat(msg,buf);
			if (!n->saveNetwork(buf, TRUE)) {
			    strcat(msg," (error while writing)");
			    write_error = TRUE;
			}
			strcat(msg,"\n");
			delete basename;
		    }
		}
	    }
	    sprintf(buf,"\n%s will now terminate.",
				theApplication->getInformalName());
	    strcat(msg, buf);
	    
	    theWarningDialogManager->modalPost(
			theDXApplication->getAnchor()->getRootWidget(),
			msg,
			"Terminating",
			(void*) theDXApplication,
		    NULL,	// Ok button
		    NULL);	// Close manager options menu
	Widget w = XtParent(theWarningDialogManager->getRootWidget());
	ASSERT(XtHasCallbacks(w,XmNpopdownCallback));
	XtAddCallback(w,XmNpopdownCallback, 
			ShutdownApplication, (XtPointer)theDXApplication);

	} else {
	    theDXApplication->shutdownApplication();
	}
    } else {
	theDXApplication->shutdownApplication();
    }
}
//
// clientData is interpretted as a number of seconds.  
// Installs the shutdown timeout function to occur after the indicated
// number of seconds.
//
extern "C" void InstallShutdownTimer(Widget w, 
				XtPointer clientData, XtPointer calldata)
{
    int seconds = (int)(long) clientData;

    ASSERT(seconds > 0);

    XtAppAddTimeOut(
	    theApplication->getApplicationContext(),
	    1000*seconds,
	    ShutdownTimeout,
	    (XtPointer)seconds);

}
//
// Called when the license for the user interface has been lost
// This issues a modal warning and installs a callback on the warning, so
// that when the user enters ok, a timer is installed so that the
// system issues more warnings until if finally shuts down. 
//
#if 0
static void LostLicense(XtPointer clientData, int *fid, XtInputId *id)
{
    XtRemoveInput(*id);

    //
    // Make sure the keyboard/pointer is grabbed, so that the user 
    // can respond to a modal dialog.
    //
    XUngrabPointer(theApplication->getDisplay(), CurrentTime);
    XUngrabKeyboard(theApplication->getDisplay(), CurrentTime);

    char buf[1024];

    sprintf(buf,
	"Your %s session has lost its license to run the\n"
	"user interface.  After closing this dialog, you will be given\n"
	"%s with intervening warnings before your session terminates.\n"
	"If you do not save your work, it will be saved for you.",
	theApplication->getInformalName(),
	format_seconds(MAX_INTERVAL));

    theWarningDialogManager->modalPost(
		theDXApplication->getAnchor()->getRootWidget(),
	buf,
	"WARNING: User Interface License",
	(void*) MAX_INTERVAL,
		    NULL,	// Ok button
		    NULL);	// Close manager options menu
	Widget w = XtParent(theWarningDialogManager->getRootWidget());
	ASSERT(XtHasCallbacks(w,XmNpopdownCallback));
	XtAddCallback(w,XmNpopdownCallback, 
			InstallShutdownTimer, (XtPointer)MAX_INTERVAL);
}
#endif

//
// Shutdown the application (typically, after the user has hit OK on
// a dialog).
//
extern "C" void ShutdownApplication(Widget w, 
			XtPointer clientData, XtPointer callData)
{
   Application *app = (Application*)clientData;
   app->shutdownApplication();
}
extern "C" int DXApplication_DXAfterFunction(Display *display)
{
    Window       root;
    Window       child;
    int          root_x;
    int          root_y;
    int          win_x;
    int          win_y;
    unsigned int key_buttons;
    Window       win;

    XSetAfterFunction(display, NULL);

    win  = RootWindow(display,0);
    XQueryPointer(display, win,
                  &root, &child, &root_x, &root_y,
                  &win_x, &win_y, &key_buttons);

    XSetAfterFunction(display, DXApplication_DXAfterFunction);
    return 1;
}

boolean DXApplication::printComment(FILE *f)
{
    if ((this->messageWindow && this->messageWindow->isManaged()) ||
        (this->helpWindow    && this->helpWindow->isManaged())) { 
	if (this->messageWindow->isManaged()) {
	    if ((fprintf(f,"// Message Window:\n") < 0) ||
	        !this->messageWindow->printComment(f))
		return FALSE;
	}
    }

    return TRUE;
}
boolean DXApplication::parseComment(const char *comment, 
				const char *filename, int lineno)
{
    if (strstr(comment,"Message Window:")) {
	if (!this->messageWindow) 
	    this->messageWindow = this->newMsgWin();
	this->messageWindow->useDefaultCommentState();
    }  else if (this->messageWindow) {
	if (!this->messageWindow->parseComment(comment,filename,lineno)) 
	    return FALSE;
    } else {
	return FALSE;
    }

    return TRUE;
}

//
// Return TRUE if the DXWindows are supposed to use the window placement
// information saved in the .net or .cfg files.
//
boolean DXApplication::applyWindowPlacements()
{
    return !this->resource.noWindowPlacement;
}

boolean DXApplication::inEditMode()
{
    return EqualString(this->resource.anchorMode,EDIT_ANCHOR_MODE);
}
boolean DXApplication::inImageMode()
{
    return EqualString(this->resource.anchorMode,IMAGE_ANCHOR_MODE);
}
boolean DXApplication::inMenuBarMode()
{
    return EqualString(this->resource.anchorMode,MENUBAR_ANCHOR_MODE);
}
const char *DXApplication::getDataViewerImportFile()
{
    return this->resource.viewDataFile; 
}
boolean DXApplication::inDataViewerMode()
{
    return this->getDataViewerImportFile() != NULL;
}

void DXApplication::shutdownApplication()
{
    XEvent event;
    Widget widg = this->getRootWidget();
    Window w = XtWindow(widg);

    this->runApplication = FALSE;

    event.type = KeyPress;

    XSendEvent(this->display, w, True, XtBuildEventMask(widg), &event);

    if ((this->inEditMode()) && (this->appAllowsSavingNetFile()))
	theResourceManager->saveResources();
}

//
// Does this application force the .net files that are read in to be
// encrypted.
//
boolean DXApplication::appForcesNetFileEncryption()
{
    return this->resource.forceNetFileEncryption;
}



//
// Write all dirty files into a tmp directory.
//
void DXApplication::emergencySave (char *msg)
{
int i;
boolean write_error = TRUE;

#ifdef DXD_OS_NON_UNIX
#ifdef DXD_WIN
    char *dirs[2];
    dirs[0] = getenv("TMP");
    dirs[1] = NULL;
#else
    // FIXME:
    char *dirs[2] = { "\\tmp", NULL };
#endif
#else
#if defined(aviion) || defined(hp700)
    // These guys won't do aggregate initialization of automatics
    char *dirs[3];
    dirs[0] = "/tmp";
    dirs[1] = "/usr/tmp";
    dirs[2] = NULL;
#else
    char *dirs[] = { "/tmp", "/usr/tmp" , NULL};
#endif
#endif

char *introMsg = "The following files have been written...\n\n";
Boolean introMsgUsed = False;
char buf[1024];

    ASSERT(msg);
    msg[0] = 0;

    for (i=0 ; write_error && dirs[i] ; i++) {
	int cnt;
	ListIterator li(theDXApplication->macroList);
	write_error = FALSE;
	cnt = 0;
	Network *n = theDXApplication->network;
	while ((n) && (!write_error)) {
	    if (n->isFileDirty() && n->saveToFileRequired()) {
		const char *filename = n->getFileName();
		char *basename;
		if (filename) {
		    basename = GetFileBaseName(filename, NULL);
		} else {
		    basename = new char[32];
		    sprintf(basename,"network_%d",++cnt);
		}
	#ifdef DXD_WIN
		sprintf(buf,"%s\\%s",dirs[i],basename);
	#else
		sprintf(buf,"%s/%s",dirs[i],basename);
	#endif
		if (!introMsgUsed) {
		   strcpy (msg, introMsg);
		   introMsgUsed = True;
		}
		strcat(msg,"\t");
		strcat(msg,buf);
		if (!n->saveNetwork(buf, TRUE)) {
		    strcat(msg," (error while writing)");
		    write_error = TRUE;
		}
		strcat(msg,"\n");
		delete basename;
	    }
	    n = (Network*)li.getNext();
	}
    }
    if (!introMsgUsed)
	strcpy(msg,"No files needed to be saved.\n");
}



// Signal handlers can be called anytime they're installed regardless of the 
// lifespan of the object.  The check for !theDXApplication really means:
// am I being called after DXApplication::~DXApplication() ?
void
#if defined(ALTERNATE_CXX_SIGNAL) 
DXApplication_HandleCoreDump(int dummy, ... )
#else
DXApplication_HandleCoreDump(int dummy)
#endif
{
    if (!theDXApplication) exit(1);
    boolean can_save = theDXApplication->appAllowsSavingNetFile();

    fprintf (stderr, "\n%s has experienced an internal error and will terminate.\n\n",
	theApplication->getInformalName());

    if (can_save) {
	char msg[4096];
	fprintf (stderr, "Attempting to save any modified files.\n"
	    "Please check saved files for integrity by reloading them.\n");
 	theDXApplication->emergencySave (msg);
 	fprintf (stderr, msg);
    }

    fprintf(stderr,"The application will now abort.\n");

    fflush(stderr);
    fflush(stdout);

    exit(1);
}
void DXApplication::abortApplication()
{

    if (theDXApplication) {
	char msg[4096];	
	this->emergencySave(msg);
 	fprintf(stderr, msg);
    }

    this->IBMApplication::abortApplication();
}


//
// C H E C K   B E T A   B U I L D   D A T E     C H E C K   B E T A   B U I L D   D A T E
// C H E C K   B E T A   B U I L D   D A T E     C H E C K   B E T A   B U I L D   D A T E
// C H E C K   B E T A   B U I L D   D A T E     C H E C K   B E T A   B U I L D   D A T E
// C H E C K   B E T A   B U I L D   D A T E     C H E C K   B E T A   B U I L D   D A T E
//
#if defined(BETA_VERSION) && defined(NOTDEF)
#include <time.h>
#ifdef DXD_WIN
#include <sys/timeb.h>
#else
#include <sys/time.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#define DAYS_SINCE(year,cumDays) {\
int i;\
cumDays=0;\
for(i=70;i<year;i++){\
if((i&3)==0)cumDays+=366;\
else cumDays+=365;}\
}

#if defined(alphax) 
extern "C" int getdate_err;
extern "C" struct tm *getdate(char *string);
#endif

void DXApplication::betaTimeoutCheck()
{
#define BETA_DAYS 60
#define BETA_SECONDS (BETA_DAYS*24*60*60)

#ifdef DXD_WIN
    char achDate[64], szDay[32], szMonth[32], szYear[32];
    int iMonth, iDaysPassed;
    struct tm  *when;
    time_t now, compile_t;

    time(&now);	//	Get sec. since Jan 1 1970, midnight 00:00:00

    sprintf(achDate, "%s", __DATE__);
    sscanf(achDate, "%s%s%s", szMonth, szDay, szYear);
    iMonth = 0;
    if(strnicmp(szMonth, "Jan", 3) == 0) iMonth = 1;
    if(strnicmp(szMonth, "Feb", 3) == 0) iMonth = 2;
    if(strnicmp(szMonth, "Mar", 3) == 0) iMonth = 3;
    if(strnicmp(szMonth, "Apr", 3) == 0) iMonth = 4;
    if(strnicmp(szMonth, "May", 3) == 0) iMonth = 5;
    if(strnicmp(szMonth, "Jun", 3) == 0) iMonth = 6;
    if(strnicmp(szMonth, "Jul", 3) == 0) iMonth = 7;
    if(strnicmp(szMonth, "Aug", 3) == 0) iMonth = 8;
    if(strnicmp(szMonth, "Sep", 3) == 0) iMonth = 9;
    if(strnicmp(szMonth, "Oct", 3) == 0) iMonth = 10;
    if(strnicmp(szMonth, "Nov", 3) == 0) iMonth = 11;
    if(strnicmp(szMonth, "Dec", 3) == 0) iMonth = 12;
    
    when = localtime(&now);
    when->tm_sec = when->tm_hour = when->tm_min = 0;
    when->tm_mday = atoi(szDay);
    when->tm_mon = iMonth - 1;
    when->tm_year = atoi(szYear) - 1900;

    compile_t = mktime(when);
    if (compile_t == -1) 
	iDaysPassed = BETA_DAYS * 2;
    else
	iDaysPassed = (now - compile_t)/ (24*60*60);

    if (iDaysPassed > BETA_DAYS) {
	fprintf (stderr, "%s\n\texpired %d days after its release.\n", 
	    this->getAboutAppString(), BETA_DAYS);
	exit(1);
    }



#else  // DXD_WIN

#if defined(ibm6000) || defined(sun4)
#define NO_GETDATE
#endif

#if !defined(NO_GETDATE) || !defined(sun4)
#define NORMAL_TIMEZONE_CONVERSION 
#endif

#ifndef NORMAL_TIMEZONE_CONVERSION
//			       J   F   M    A    M    J    J    A    S    O    N 
static int yday[11] =       { 31, 28, 31,  30,  31,  30,  31,  31,  30,  31,  30 };
static int cum_yday[11]; // { 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };
#endif



    struct tm *comp_time = NULL;
    time_t today = time (NULL);
    char *compile_date = __DATE__;
    char *compile_time = __TIME__;
    char date_mask[512];
    boolean abort_the_run = FALSE;

    //
    // just in case
    //
    if (getenv("TCHK_BAIL_OUT")) return ;

#ifndef NORMAL_TIMEZONE_CONVERSION
    int i;
    int current_year = localtime(&today)->tm_year;
    cum_yday[0] = yday[0];
    for (i=1; i<11; i++) {
	if ((i==1) && (current_year%4))
	    cum_yday[i] = cum_yday[i-1] + yday[i] + 1;
	else
	    cum_yday[i] = cum_yday[i-1] + yday[i];
    }
#endif

#if !defined(NO_GETDATE)
    //
    // If the user has a DATEMSK, then save it for later restoration
    //
    const char *od = getenv("DATEMSK");
    char *old_datemsk = NULL;
    if ((od) && (od[0])) old_datemsk = DuplicateString(od);

    // Make the DATEMSK environment variable point to the format file
    // which we'll store in the dx tree.
    const char *cp = /*this->getUIRoot();*/getenv("DXROOT");
    if ((!cp) || (!cp[0]))  cp = "/usr/local/dx";

    char *dx = DuplicateString(cp); 
    strcpy (dx, cp);
    int len = strlen(dx);
    if (dx[len-1] == '/') dx[len-1] = '\0';
    char date_file[512];
    sprintf (date_file, "%s/ui/date.fmt", dx);
    sprintf (date_mask, "DATEMSK=%s", date_file);
    //
    // Check for the date.fmt file.  If it's not found this scheme will fail.
    // The root cause of the problem is an improperly set DXROOT, so tell the
    // user.  The ui won't start.
    //
    char* admin_problem = NUL(char*);
    struct STATSTRUCT statb;
    if (stat (date_file, &statb) == -1) 
	admin_problem = "Your DXROOT environment variable is set incorrectly.";
    delete dx;
#endif

    char date_time[128];
    sprintf (date_time, "%s %s EDT", compile_date, compile_time);

#ifdef NO_GETDATE
    // I don't know of a way to determine if strptime() succeeded or not.
    comp_time = new struct tm;
    strptime(date_time, "%b %d %Y %H:%M:%S", comp_time);
#ifndef NORMAL_TIMEZONE_CONVERSION
    timelocal(comp_time);
#endif
#else
    // This is a leak, but without the DuplicateString, putenv trashes the environ
    putenv (DuplicateString(date_mask)); 
    comp_time = getdate (date_time);

    //
    // Retry the time conversion because some systems don't recognize the time
    // zone EST5EDT.  I don't know why that is.  It isn't necessary to use the
    // time zone.  It provide slightly better accuracy so that a better customer
    // can't continue to run an hour or two after the real expiration.  The real
    // reason for using the timezone in the time calculation is to make it easy
    // to test the code.
    //
    if ((!comp_time) && (admin_problem == NUL(char*))) {
	sprintf (date_time, "%s %s", compile_date, compile_time);
	comp_time = getdate (date_time);
	if (!comp_time)
	    admin_problem = "System time / compile time comparison failed.";
    }
#endif

    int seconds = 0;
    int days;
    if (!comp_time)
	abort_the_run = TRUE;
    else {


#ifdef NORMAL_TIMEZONE_CONVERSION
	DAYS_SINCE(comp_time->tm_year,days); 
	days+= comp_time->tm_yday;
	int hours = (days * 24) + comp_time->tm_hour - (1* (comp_time->tm_isdst>0));
#else
	DAYS_SINCE(comp_time->tm_year,days); 
	days+= (comp_time->tm_mon?cum_yday[comp_time->tm_mon-1]: 0) + comp_time->tm_mday;
	int hours = (days * 24) + comp_time->tm_hour;
#endif
	int minutes = (hours * 60) + comp_time->tm_min;
#ifdef NORMAL_TIMEZONE_CONVERSION
	seconds = minutes * 60 + comp_time->tm_sec + (int)timezone;
#else
	seconds = minutes * 60 + comp_time->tm_sec - (int)comp_time->tm_gmtoff;
#endif
	if ((today - seconds) > BETA_SECONDS) {
	    abort_the_run = TRUE;
	}
#ifdef DEBUG
	int diff_seconds = today-seconds;
	int diff_days = diff_seconds/86400;
	diff_seconds-= diff_days * 86400;
	int diff_hours = diff_seconds/3600;
	diff_seconds-= diff_hours*3600;
	int diff_minutes = diff_seconds/60;
	diff_seconds-= diff_minutes*60;
	fprintf(stdout, "%s age(days H:M:S): %d %d:%d:%d\n",
	    this->getFormalName(), diff_days, diff_hours, diff_minutes, diff_seconds);
#endif
    }
    if (abort_the_run) {
#if !defined(NO_GETDATE)
	if (admin_problem) {
	    fprintf (stderr, "Beta licensing failed for \n%s\n\t%s\n", 
		this->getAboutAppString(), admin_problem);
	} else
#endif
	fprintf (stderr, "%s\n\texpired %d days after its release.\n", 
	    this->getAboutAppString(), BETA_DAYS);
	exit(1);
    }


#ifdef NO_GETDATE
    if (comp_time) delete comp_time;
#else
    //
    // Restore DATEMSK
    //
    if (old_datemsk) {
	putenv(old_datemsk);
	//delete old_datemsk;
    }
#endif

#endif	//	DXD_WIN

#undef BETA_DAYS
#undef BETA_SECONDS
}


#endif


#if WORKSPACE_PAGES
ProcessGroupManager *DXApplication::getProcessGroupManager()
{
    return (ProcessGroupManager*)
	this->network->groupManagers->findDefinition(PROCESS_GROUP);
}
#endif


//
// Conditionally show the nodes' instance numbers in the vpe.  If the setting
// has changed then visit the standins and reshow the standin labels.
//
void DXApplication::showInstanceNumbers(boolean on_or_off)
{
    if (on_or_off == this->resource.showInstanceNumbers) return ;
    this->resource.showInstanceNumbers = on_or_off;
    if (this->inDebugMode()) return ;


    //
    // Visit every vpe.  Start with this->network, then continue with every
    // loaded macro.  Any macro could have a vpe opened.
    //
    int i,netcnt = 1;
    netcnt+= this->macroList.getSize();

    for (i=0; i<netcnt; i++) {
	Network *net = NUL(Network*);
	if (i == 0) 	net = this->network;
	else 		net = (Network*)this->macroList.getElement(i);

	EditorWindow* ew = net->getEditor();
	if (!ew) continue;
	ew->beginNetworkChange();

	Node* node;
	ListIterator it;
	FOR_EACH_NETWORK_NODE (net, node, it) {
	    StandIn* si = node->getStandIn();
	    if (!si) continue;
	    si->notifyLabelChange();
	}
	ew->endNetworkChange();
    }
}

void 
DXApplication::setServer(char *server)
{
    if (this->serverInfo.server) 
	delete this->serverInfo.server;

    this->serverInfo.server = DuplicateString(server);
    fprintf(stderr, "exec server set to %s\n", server);
}

void DXApplication::getRecentNets(List& result)
{
    theResourceManager->getValue(RECENT_NETS, result);
}

// The call to GetFullFilePath is supposed to take
// care of calling Dos2Unix().  These functions could
// also be written with ifdefs for the pc platform,
// but this way I have better assurance that they're
// working right since there aren't platform dependencies.
void DXApplication::appendReferencedFile(const char* file)
{
    //
    // Don't include files that go in /tmp.  These are files
    // that are created as a result of Cut,Copy,Paste and 
    // Drag-n-Drop and shouldn't be recorded here.
    //
    const char *tmpdir = theDXApplication->getTmpDirectory();
    int tmpdirlen = STRLEN(tmpdir);
    if (strncmp (file, tmpdir, tmpdirlen)==0) {
	return ;
    }

    char* cp = GetFullFilePath(file);
    theResourceManager->addValue(RECENT_NETS, cp);
    delete cp;
}
void DXApplication::removeReferencedFile(const char* file)
{
    char* cp = GetFullFilePath(file);
    theResourceManager->removeValue(RECENT_NETS, cp);
    delete cp;
}
