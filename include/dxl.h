/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/* 
 * (C) COPYRIGHT International Business Machines Corp. 1995
 * All Rights Reserved
 * Licensed Materials - Property of IBM
 * 
 * US Government Users Restricted Rights - Use, duplication or
 * disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
 *
 */


#ifndef _DXL_H
#define _DXL_H

#ifdef OS2
#define INCL_DOS
#include <os2.h>
#define WM_DXL_HANDLE_PENDING_MESSAGE WM_USER+3000
#endif

#ifdef DXD_WIN
#define WM_DXL_HANDLE_PENDING_MESSAGE WM_USER+3000
#endif

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

typedef int DXLError;   /* Will be one of ERROR or OK */
#ifndef ERROR
#define ERROR 0
#endif
#ifndef OK
#define OK 1
#endif

/*
 * Connection specific structure.
 */
typedef struct DXLConnection DXLConnection;


/*
 * These are the routines that most applications would use to start DX and
 * establish a DXL connection.  string is the string used by dxl to start
 * the dx application, and the correct hostname and port number are appended
 * to this string as " -appPort 1234 -appHost hostname".  DXLStartDX
 * returns NULL and sets the error code on error.
 */
DXLConnection  *DXLStartDX(const char *string, const char *host);
void            DXLCloseConnection(DXLConnection *connection);
void            DXLSetSynchronization(DXLConnection *conn, const int onoff);

/*
 * This is used primarily for debugging to connect to a server (i.e. dxexec
 *  or dxui) that has already been started externally and is awaiting
 *  a connection.  This will cause a connection to be created between
 * the process at the given port on the given host.  If host is given
 * as NULL, then "localhost" is used.  port must be given as >= 0.
 * On error NULL is returned, otherwise a pointer to a DXLConnection
 * structure.
 */
DXLConnection *DXLConnectToRunningServer(int port, const char *host);

/*
 * This turns on/off message debugging. If message debugging is enabled,
 * then messages are printed on the terminal window as they are sent
 * and received.   The return value indicates the previous state of
 * message debugging. 
 */
int   	DXLSetMessageDebugging(DXLConnection *c, int on);

/*
 * Get the socket this is the used for communications.
 */
int DXLGetSocket(DXLConnection *conn);

/*
 * Determine if there are message that need to be handled.  If so, the
 * user should arrange to have DXLHandlePendingMessages() called.
 * Return 0 if no messages pending, non-zero otherwise.
 */
int DXLIsMessagePending(DXLConnection *conn);

/*
 * Parse a message that is waiting to be processed.  This results the
 * installed message handlers being called when appropriate  messages
 * are received.  This is called automatically if DXLInitializeXMainLoop()
 * is used.
 */
DXLError DXLHandlePendingMessages(DXLConnection *conn);
DXLError DXLWaitForEvent(DXLConnection *conn);

/*
 * Initialize the X11 window system so that calls to XtAppMainLoop() will
 * cause messages to be processed.
 */
#ifdef X_H
DXLError DXLInitializeXMainLoop(XtAppContext app, DXLConnection *conn);
DXLError DXLUninitializeXMainLoop(DXLConnection *conn);
#endif

/*
 * Install callback to be called back when the connection to DX is broken
 */
void
DXLSetBrokenConnectionCallback(DXLConnection *conn, void (*proc)(DXLConnection *, void *), void *data);

/*
 * Functions/typedefs for generic message handling 
 * See the DXLMessage module.
 */
typedef void (*DXLMessageHandler)(DXLConnection *conn,
                                        const char *msg, void *data);

/*
 * Message (packet) types
 */
enum DXLPacketType {
    PACK_INTERRUPT      = 1,
    PACK_MACRODEF       = 4,
    PACK_FOREGROUND     = 5,
    PACK_BACKGROUND     = 6,
    PACK_ERROR          = 7,
    PACK_MESSAGE        = 8,
    PACK_INFO           = 9,
    PACK_LINQUIRY       = 10,
    PACK_LRESPONSE      = 11,
    PACK_COMPLETE       = 19,
    PACK_LINK           = 22
};
typedef enum DXLPacketType DXLPacketTypeEnum;


/*
 * This routine sets the error handler, the routine called when an error
 * (usually a dxui generated error) occurs.
 * The default error handler prints the message and exits.
 */
DXLError DXLSetErrorHandler(DXLConnection *connection, DXLMessageHandler h,
                                   const void *data);

/*
 * Functions/typedefs for generic message handling
 * See the DXLMessage module.
 */
 
DXLError        DXLSetMessageHandler(DXLConnection *conn,
			DXLPacketTypeEnum type, const char *matchstr,
			DXLMessageHandler h, const void *data);
DXLError        DXLRemoveMessageHandler(DXLConnection *conn,
			DXLPacketTypeEnum type, const char *matchstr,
			DXLMessageHandler h);

/*
 * Functions/typedefs to receive values from the server.
 * See the DXLOutput module. 
 */
typedef void (*DXLValueHandler)(DXLConnection *conn, const char *name,
                                                const char *value, void *data);
DXLError DXLSetValueHandler(DXLConnection *c, const char *name,
                                        DXLValueHandler h, const void *data);
DXLError DXLRemoveValueHandler(DXLConnection *c, const char *name);

/*
 * Functions that are used to set values in the DX server's global dictionary.
 * See the DXLInput tool.
 */
DXLError        DXLSetValue(DXLConnection *conn,
                        const char *varname,
                        const char *value);
DXLError        DXLSetInteger(DXLConnection *conn,
                        const char *varname,
                        const int value);
DXLError        DXLSetScalar(DXLConnection *conn,
                        const char *varname,
                        const double value);
DXLError        DXLSetString(DXLConnection *conn,
                        const char *varname,
                        const char *value);

/*
 * These are higher level functions to control DX.
 */
DXLError        DXLLoadVisualProgram(DXLConnection *connection,
                        const char *file);

DXLError	DXLLoadMacroDirectory(DXLConnection *connection, const char *dir);

DXLError        exDXLLoadScript(DXLConnection *connection,
                        const char *file);
DXLError        exDXLBeginMacroDefinition(DXLConnection *connection,
                                          const char *mhdr);
DXLError        exDXLEndMacroDefinition(DXLConnection *connection);

/*
 * This is a function that can be used to send a message through dxl
 * and return a response to the message. It returns the length of the
 * respsonse if successfull, otherwise -1 and sets the error code (in errno).
 */
int             DXLQuery(DXLConnection *connection,
                         const char *msg, const int length, char *response);

#if !defined(DX_NATIVE_WINDOWS)
DXLError        uiDXLLoadConfig(DXLConnection *connection, const char *file);
DXLError        uiDXLSyncExecutive(DXLConnection *conn);
DXLError        uiDXLOpenVPE(DXLConnection *conn);
DXLError        uiDXLCloseVPE(DXLConnection *conn);
DXLError        uiDXLOpenSequencer(DXLConnection *conn);
DXLError        uiDXLCloseSequencer(DXLConnection *conn);
DXLError        uiDXLOpenAllImages(DXLConnection *conn);
DXLError        uiDXLCloseAllImages(DXLConnection *conn);

DXLError             uiDXLConnectToRunningServer(DXLConnection *connection,
                        const int port);
DXLError             uiDXLStartServer(DXLConnection *connection);
DXLError                uiDXLDisconnect(DXLConnection *conn);

/*
 * UI/Windowing system specific functions
 */
DXLError           uiDXLSaveVisualProgram(DXLConnection *connection,
			const char *file);
/* need save config */

DXLError           uiDXLOpenImageByLabel(DXLConnection *conn, char *label);
DXLError           uiDXLOpenImageByTitle(DXLConnection *conn, char *title);

DXLError           uiDXLCloseImageByLabel(DXLConnection *conn, char *label);
DXLError           uiDXLCloseImageByTitle(DXLConnection *conn, char *title);

DXLError           uiDXLOpenAllColorMapEditors(DXLConnection *conn);
DXLError           uiDXLOpenColorMapEditorByLabel(DXLConnection *conn, char *label);
DXLError           uiDXLOpenColorMapEditorByTitle(DXLConnection *conn, char *title);

DXLError           uiDXLCloseAllColorMapEditors(DXLConnection *conn);
DXLError           uiDXLCloseColorMapEditorByLabel(DXLConnection *conn, char *label);
DXLError           uiDXLCloseColorMapEditorByTitle(DXLConnection *conn, char *title);

/* this doesn't need to exist - we have close all, close by title
 *  and close by label.  what would a generic close do?
 */
#if 0 
/* DXLError           uiDXLCloseColorMapEditors(DXLConnection *conn); */
#endif

/*
 * Effectively "reset" the server when we are dxui-connected.  
 * The UI will stop execution, flush the cache and reload all the 
 * referenced variables.  
 */
DXLError uiDXLResetServer(DXLConnection *conn);

/*
 * This routine allows DXLink apps to switch between h/w & s/w rendering
 * when connected to the ui
 * hwrender == 0 : s/w else h/w rendering
 */
DXLError        uiDXLSetRenderMode(DXLConnection *conn, char *title, 
			int hwrender);

#endif /* !DX_NATIVE_WINDOWS */

/*
 * Initialize the Presentation manager window system so that...
 */
#ifdef OS2
DXLError DXLInitializePMMainLoop(/* HWND PMAppWindow, DXLConnection *conn */);
#endif


DXLError        DXLSync(DXLConnection *conn);


DXLError        DXLGetExecutionStatus(DXLConnection *conn, int *state);

DXLError        DXLExecuteOnce(DXLConnection *conn);
DXLError        DXLExecuteOnChange(DXLConnection *conn);

DXLError        exDXLExecuteOnceNamed(DXLConnection *conn, char *name);
DXLError        exDXLExecuteOnceNamedWithArgs(DXLConnection *conn, char *name, ...);
DXLError        exDXLExecuteOnceNamedWithArgsV(DXLConnection *conn, char *name, char **args);
DXLError        exDXLExecuteOnChangeNamed(DXLConnection *conn, char *name);
DXLError 	exDXLExecuteOnChangeNamedWithArgs(DXLConnection *conn, char *name, ...);
DXLError 	exDXLExecuteOnChangeNamedWithArgsV(DXLConnection *conn, char *name, char **args);

DXLError        DXLEndExecution(DXLConnection *conn);
DXLError        DXLEndExecuteOnChange(DXLConnection *conn);


typedef enum {
    SeqPlayForward,
    SeqPlayBackward,
    SeqPause,
    SeqStep,
    SeqStop,
    SeqPalindromeOn,
    SeqPalindromeOff,
    SeqLoopOn,
    SeqLoopOff
} DXLSequencerCtlEnum;
DXLError DXLSequencerCtl(DXLConnection *conn, DXLSequencerCtlEnum action);


DXLError        DXLExitDX(DXLConnection *conn);

/*
 * This is a function that can be used to send a mesage through dxl.
 * It returns OK if successfull, otherwise ERROR and sets the
 * error code (in errno).
 */
DXLError        DXLSend(DXLConnection *connection, const char *msg);



#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
