/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#ifndef _DXLP_H
#define _DXLP_H

#include "dxl.h"
#include "dict.h"

#include <sys/types.h>

#if defined(HAVE_WINSOCK_H)
#include <winsock.h>
#else
# if defined(HAVE_NETINET_IN_H)
# include <netinet/in.h>
# endif
#endif

#if defined(HAVE_SYS_UN_H)
#include <sys/un.h>
#endif

#ifdef OS2
#define INCL_DOS
#include <os2.h>
#endif

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

/*
 * Input handling structure.
 */
#define DXL_EVENT_BUF_SIZE 128
struct DXLEvent {
    int  type;
    int  serial;
    char *contents;
    int  contentsSize;
    char buffer[DXL_EVENT_BUF_SIZE];
    struct DXLEvent *next;
};
typedef struct DXLEvent DXLEvent;


struct handlerEntry
{
    int                 packetId;
    char                *operand;
    int                 operandLength;
    DXLMessageHandler   handler;
    const void          *data;          /* Call data passed to handler */
    struct handlerEntry *next;
};
typedef struct handlerEntry *HandlerEntry;

DXLError _dxl_InitMessageHandlers(DXLConnection *);
DXLError _dxl_FreeMessageHandlers(DXLConnection *);
DXLError _dxl_HandleMessage(DXLConnection *, int, int, const char *);
DXLError _dxl_SetSystemHandlerId(DXLConnection *conn,
                int type, int packetId,
                DXLMessageHandler h, const void *data);
DXLError _dxl_SetSystemHandlerString(DXLConnection *conn,
                int type, const char *str,
                DXLMessageHandler h, const void *data);
DXLError _dxl_SetSystemHandler(DXLConnection *conn,
                int type, int packetId, const char *str,
                DXLMessageHandler h, const void *data);
DXLError _dxl_RemoveSystemHandler(DXLConnection *conn,
		int type, int packetId, const char *str, 
		DXLMessageHandler h);



/*
 * Actually, there are only 21 in dx2.1.1 (no $lnk), but having to many
 * entries should not hurt.
 */
#define PACKET_TYPE_COUNT 22

/*
 * Connection specific structure.
 */
struct DXLConnection {
    int fd;
    int id;
    int nEvents;
#if 00
    int allocdEvents;
#else
    DXLEvent *lastEvent;
#endif
    DXLEvent *events;
    int nLeftOver;
    int allocdLeftOver;
    char *leftOver;
    int synchronous;
    int syncing;                /* DXLSync is in the call stack if true */
    int dxuiConnected;          /* Are we connected to the ui or exec */
    int isExecuting;            /* Only valid when !dxuiConnected */
    int majorVersion;           /* Major version # of remote app */
    int minorVersion;           /* Minor version # of remote app */
    int microVersion;           /* Micro version # of remote app */
    int debugMessaging;         /* Print info about packets sent/received */
    /*
     * remember that packet types are one-origin
     */
    HandlerEntry systemMessageHandlers[PACKET_TYPE_COUNT+1];
    HandlerEntry userMessageHandlers[PACKET_TYPE_COUNT+1];
    _Dictionary  *objectHandlerDict;
    _Dictionary  *valueHandlerDict;
    /* Flag to check macro definition status
     * Aids error checking and reduces user complexity by allowing
     * DXLSend to be used for macro body
     */
    int macroDef;
#ifdef OS2
    HWND PMAppWindow;
    HEV  PMHandlerSemaphore;
    TID  tidCheckInputThread;
#endif
#if defined(intelnt) || defined(WIN32)
    HWND PMAppWindow;
    HANDLE  PMHandlerSemaphore;
    HANDLE  tidCheckInputThread;
#endif
    void (*brokenConnectionCallback)(DXLConnection *, void *);
    void *brokenConnectionCallbackData;
    void (*installIdleEventHandler)(DXLConnection *);
    void (*clearIdleEventHandler)(DXLConnection *);
    void (*removeEventHandler)(DXLConnection *);
    void  *eventHandlerData;

    /*
     * The file descriptors are pipes opened in conn.c.   They're
     * stored here so that they can be closed when the connection
     * goes away.
     */
    int   in, out, err;
};

#define _dxf_InstallIdleEventHandler(c) \
	if ((c) && ((c)->installIdleEventHandler)) \
	    ((c)->installIdleEventHandler)(c);

#define _dxf_ClearIdleEventHandler(c) \
	if ((c) && ((c)->clearIdleEventHandler)) \
	    ((c)->clearIdleEventHandler)(c);

#define _dxf_RemoveEventHandler(c) \
	if ((c) && ((c)->removeEventHandler)) \
	    ((c)->removeEventHandler)(c);

#define SELECT_INTR(n, readlist, writelist, execptlist, timeout,rc) \
        while((rc = select(n, readlist, writelist, execptlist, timeout)) \
          < 0 && errno == EINTR)
		

extern char *_DXLPacketTypes[];
extern int _DXLNumPackets;

/*
 * Internal structure used for the connection functions.
 */
struct DXLConnectionPrototype {
    int sock;
    struct sockaddr_in server;
    int usock;
#if defined(HAVE_SYS_UN_H)
    struct sockaddr_un userver;
#endif
};
typedef struct DXLConnectionPrototype DXLConnectionPrototype;

/*
 * Internal routines, provided for debugging.  These are used by DXLStartDX,
 * and are not normally accessed by a user.  If an application  were to use
 * these routines, it would call DXLInitPrototype, start dx however it
 * wanted to (including by hand in the debugger) and have it connect to the
 * returnedPort.  After dxui is started, the application would call
 * DXLFinishConnection, and "free()" the DXLConnectionPrototype.
 *
 * returnedPort is the port number actually created, and can be passed to
 * DXLStartChild .  forceTimeout is a boolean that makes DXLFinishConnection
 * fail if the DX application didn't connect within some time.
 *
 * DXLInitPrototype and DXLFinishConnection return NULL and set the error code
 * on error.
 */

/*
 * These routines control the inital port tried in creating the connection
 * (defaults to 1920) and the currently used timeout in seconds.
 */
#define DXL_ACCEPT_TIMEOUT      60      /* Seconds */
#define DXL_BASE_PORT           1920
void            DXLSetBasePort(const int port);
int             DXLGetBasePort();
void _dxl_InvalidateSocket(DXLConnection *connection);
#if 0
void            DXLSetHost(const char *host);
char *          DXLGetHost();
#endif
int             DXLIsHostLocal(const char *host);

/*
 * Default error handler and a routine to facilitate calling the
 * current handler.
 */
void _DXLDefaultHandler(DXLConnection *conn, const char *message, void *data);
DXLError _DXLError(DXLConnection *conn, const char *message);

DXLError DXLNextPacket(DXLConnection *conn, DXLEvent *event);
/*
 * Find and read until a packet with the given type and  id is found.
 * Return the found event in the given event, and remove the event
 * from the event list.
 */
DXLError DXLGetPacketId(DXLConnection *conn, DXLPacketTypeEnum type, int id,
                                                DXLEvent *event);
DXLError DXLGetPacketString(DXLConnection *conn, DXLPacketTypeEnum type,
                                        const char *matchstr, DXLEvent *event);
DXLError DXLGetPacketIdString(DXLConnection *conn, DXLPacketTypeEnum type,
                        int requestId, const char *str, DXLEvent *event);
int      DXLCheckPacketString(DXLConnection *conn, DXLPacketTypeEnum type,
                                        const char *matchstr, DXLEvent *event);
DXLError DXLWaitPacketString(DXLConnection *conn, DXLPacketTypeEnum type,
                                        const char *matchstr, DXLEvent *event);
DXLError DXLPeekPacket(DXLConnection *conn, DXLEvent *event);
DXLError DXLReadFromSocket(DXLConnection *conn);
int      DXLGetSocket(DXLConnection *conn);
DXLEvent *DXLNewEvent(int serial, DXLPacketTypeEnum ptype, const char *msg);
DXLError DXLFreeEvent(DXLEvent *event);
DXLError DXLClearEvent(DXLEvent *event);
DXLError DXLCopyEvent(DXLEvent *dst, DXLEvent *src);
DXLError DXLProcessEventList(DXLConnection *conn);

#ifdef NON_ANSI_SPRINTF
#define SPRINTF _DXLsprintf
#else
#define SPRINTF sprintf
#endif

DXLError DXLSendUnpacketized(DXLConnection *conn, const char *string);
DXLError DXLSendImmediate(DXLConnection *conn, const char *string);

DXLError DXLSendPacket(DXLConnection *conn, DXLPacketTypeEnum ptype,
                                        const char *string);


/*
 * Value and Object handler functions that are used to set up functions
 * that are called when values and objects are sent from the DX server.
 */
#ifndef _LIBDX_H
typedef struct object *Object;
#endif  /* dx.h */
typedef void (*DXLObjectHandler)(DXLConnection *conn, const char *name,
                                                Object o, void *data);
DXLError DXLSetObjectHandler(DXLConnection *c, const char *name,
                                        DXLObjectHandler h, const void *data);
DXLError DXLRemoveObjectHandler(DXLConnection *c, const char *name);


/*
 * Setting and getting of input values specified by macro, module,
 * module instance number and parameter index.
 */
DXLError                DXLSetInputValue(DXLConnection *conn,
                        const char *macro,
                        const char *module,
                        const int instance,
                        const int number,
                        const char *value);
DXLError                DXLSetInputInteger(DXLConnection *conn,
                        const char *macro,
                        const char *module,
                        const int instance,
                        const int number,
                        const int value);
DXLError                DXLSetInputScalar(DXLConnection *conn,
                        const char *macro,
                        const char *module,
                        const int instance,
                        const int number,
                        const double value);
DXLError                DXLSetInputString(DXLConnection *conn,
                        const char *macro,
                        const char *module,
                        const int instance,
                        const int number,
                        const char *value);

DXLError                DXLGetInputValue(DXLConnection *conn,
                        const char *macro,
                        const char *module,
                        const int instance,
                        const int number,
                        const int valueLen,
                        char *value);
DXLError                DXLGetInputInteger(DXLConnection *conn,
                        const char *macro,
                        const char *module,
                        const int instance,
                        const int number,
                        int *value);
DXLError                DXLGetInputScalar(DXLConnection *conn,
                        const char *macro,
                        const char *module,
                        const int instance,
                        const int number,
                        double *value);
DXLError                DXLGetInputString(DXLConnection *conn,
                        const char *macro,
                        const char *module,
                        const int instance,
                        const int number,
                        const int valueLen,
                        char *value);

/*
 * Setting and getting of output values specified by macro, module,
 * module instance number and parameter index.
 * Outputs are only settable and queryable for interactors.
 */
DXLError                DXLSetOutputValue(DXLConnection *conn,
                        const char *macro,
                        const char *module,
                        const int instance,
                        const int number,
                        const char *value);
DXLError                DXLSetOutputInteger(DXLConnection *conn,
                        const char *macro,
                        const char *module,
                        const int instance,
                        const int number,
                        const int value);
DXLError                DXLSetOutputScalar(DXLConnection *conn,
                        const char *macro,
                        const char *module,
                        const int instance,
                        const int number,
                        const double value);
DXLError                DXLSetOutputString(DXLConnection *conn,
                        const char *macro,
                        const char *module,
                        const int instance,
                        const int number,
                        const char *value);

DXLError                DXLGetOutputValue(DXLConnection *conn,
                        const char *macro,
                        const char *module,
                        const int instance,
                        const int number,
                        const int valueLen,
                        char *value);
DXLError                DXLGetOutputInteger(DXLConnection *conn,
                        const char *macro,
                        const char *module,
                        const int instance,
                        const int number,
                        int *value);
DXLError                DXLGetOutputScalar(DXLConnection *conn,
                        const char *macro,
                        const char *module,
                        const int instance,
                        const int number,
                        double *value);
DXLError                DXLGetOutputString(DXLConnection *conn,
                        const char *macro,
                        const char *module,
                        const int instance,
                        const int number,
                        const int valueLen,
                        char *value);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif
