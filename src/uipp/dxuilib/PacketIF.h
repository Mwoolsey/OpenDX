/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _PacketIF_h
#define _PacketIF_h

#include <stdio.h>
#include <X11/Intrinsic.h>

#include "Base.h"
#include "PacketHandler.h"
#include "LinkHandler.h"
#include "List.h"
#include "QueuedPackets.h"

#if defined(intelnt)  && defined(ERROR)
#undef ERROR
#endif


//
// Class name definition:
//
#define ClassPacketIF	"PacketIF"

//
// XtCallbackProc (*CB), XtEventHandler (*EH) and XtActionProc (*AP)
// DialogCallback (*DCB), XtInputCallbackProc (*ICP), XtWorkProc (*WP)
// functions for this and derived classes
//
extern "C" void PacketIF_InputIdleTimerTCP(XtPointer, XtIntervalId*);
extern "C" Boolean PacketIF_InputIdleWP(XtPointer);
extern "C" Boolean PacketIF_QueuedPacketWP(XtPointer);
extern "C" void PacketIF_ProcessSocketInputICB(XtPointer, int*, XtInputId*);

#if defined(DXD_IBM_OS2_SOCKETS)  || defined(DXD_HAS_WINSOCKETS)
int UxSend(int s, const char *ExternalBuffer, int TotalBytesToSend, int Flags);
int UxRecv(int s, char *ExternalBuffer, int BuffSize, int Flags);
#endif


typedef void (*PacketIFCallback)(void *clientData, char *echoString);
typedef boolean (*StallingHandler)(void *clientData);

//
// PacketIF class definition:
//				
class PacketIF : public Base
{

    //
    // Allow the LinkHandlers to send packets.
    //
    friend void LinkHandler::sendPacket(int , int , const char *, int);
    friend void QueuedPacket::send(PacketIF*);
    friend void QueuedBytes::send(PacketIF*);
    friend void QueuedImmediate::send(PacketIF*);

  private:
    //
    // Private member data:
    //

    //
    // I hate this... on some AIX systems, it seems that the work proc can
    // get called even after the PacketIF deletion method is called.  When this
    // happens, the work proc receives a deleted PacketIF, and all hell breaks 
    // loose.  Instead, I'm going to use double indirection... I'll allocate a
    // pointer when the PacketIF is created, set it to point to the PacketIF
    // itselfkeep a pointer to it in the PacketIF, and pass its address as the
    // work proc client data.  When the PacketIF is deleted, the pointer is set
    // to NULL, but not freed.  So if the pointer that the work proc receives as 
    // its client data points to NULL, we know that the PacketIF has been deleted
    // and the work proc returns immediately.  Unfortunately, this leaks space 
    // for a pointer for each packetIF created.
    //
    PacketIF	     **wpClientData;

    int      	     error;
    int      	     socket;
    FILE    	     *stream;
    XtInputId 	     inputHandlerId;
    boolean	     deferPacketHandling;
    XtWorkProcId     workProcId;      
    XtIntervalId     workProcTimerId;  
    int      	     line_length;
    int      	     alloc_line_length;

    friend void      PacketIF_InputIdleTimerTCP(XtPointer, XtIntervalId*);
    friend Boolean   PacketIF_InputIdleWP(XtPointer clientData);
    friend Boolean   PacketIF_QueuedPacketWP(XtPointer clientData);
    friend void	     PacketIF_ProcessSocketInputICB(XtPointer clientData, 
						       int *socket,
						       XtInputId *id);
    PacketIFCallback echoCallback;
    void             *echoClientData;

    PacketIFCallback errorCallback;
    void             *errorClientData;

    boolean          endReceiving;
    void             *endReceiveData;	// Data passed back by

    StallingHandler   stallingWorker;
    void 	     *stallingWorkerData;

    /*
     * Open a socket port and wait for a client to connect.
     * This opens 2 sockets (except on the server), one internet domain, and
     * one unix domain.  It then selects on both sockets to see to which one the
     * app connects, and returns the new connection, closing the one not
     * selected.
     */
    void connectAsServer(int pport);

    void connectAsClient(const char *host, int port, boolean local);

    // 
    // Control whether or not the callbacks to handle messages on the
    // wire will be called. 
    //
    void installInputHandler();
    void removeInputHandler();

    // 
    // These methods are used to make the handling of packets a lower 
    // priority than the events coming from the display.  Basically, 
    // we don't enable the handling of a message until the WorkProc 
    // is called (indicating that there are no other events to handle).
    //
    void installWorkProc();
    void removeWorkProc();
    void installWorkProcTimer();
    void removeWorkProcTimer();


  protected:
    //
    // Protected member data:
    //

    List     	     handlers;		// List of PacketHandler's.

    virtual void     parsePacket();
    char    	     *line;


    //
    // Handle stream read/write errors.  We handle SIGPIPE/EPIPE specially.
    // SIGPIPE is ignored and then if we get an EPIPE on a write, then we
    // know the server or other connect has gone away.
    // The errnum is passed in for portability, but is ignored on UNIX
    // systems.
    //
    void handleStreamError(int errnum, const char *msg);

    //
    // Receive a packet.
    // If readSocket is FALSE, then we just process packets that are still
    // in the line buffer.
    //
    void packetReceive(boolean readSocket = TRUE);

    //
    // Routines to handle PacketIF messages.
    //
    static void ProcessMessage(void *clientData, int id, void *line);
    static void ProcessInformation(void *clientData, int id, void *line);
    static void ProcessError(void *clientData, int id, void *line);
    static void ProcessErrorWARNING(void *clientData, int id, void *line);
    static void ProcessErrorERROR(void *clientData, int id, void *p);
    static void ProcessCompletion(void *clientData, int id, void *line);
    static void ProcessInterrupt(void *clientData, int id, void *line);
    static void ProcessBeginExecNode(void *clientData, int id, void *line);
    static void ProcessEndExecNode(void *clientData, int id, void *line);
    static void ProcessLinkCommand(void *clientData, int id, void *p);
    static void HandleError(void *clientData, char *message);

    LinkHandler      *linkHandler;
    virtual void      installLinkHandler();

    //
    // Queue outgoing messages in order to avoid deadlock with dxexec
    //
    void _sendBytes(const char *string);
    void _sendImmediate(const char *string);
    void _sendPacket(int type, int packetId, const char *data = NULL, int length = 0);
    Boolean sendQueuedPackets();
    boolean isSocketInputReady();
    List output_queue;
    int output_queue_wpid;
    void sendPacket(int type, int packetId, const char *data = NULL, int length = 0);

  public:

    static const char* PacketTypes[];

    enum {
	INTERRUPT        = 1,
	SYSTEM           = 2,
	ACK              = 3,
	MACRODEF         = 4,
	FOREGROUND       = 5,
	BACKGROUND       = 6,
	PKTERROR         = 7,
	MESSAGE          = 8,
	INFORMATION      = 9,
	LINQUIRY         = 10,
	LRESPONSE        = 11,
	LDATA            = 12,
	SINQUIRY         = 13,
	SRESPONSE        = 14,
	SDATA            = 15,
	VINQUIRY         = 16,
	VRESPONSE        = 17,
	VDATA            = 18,
	COMPLETE         = 19,
	IMPORT           = 20,
	IMPORTINFO       = 21,
	LINK	         = 22
    };
	
    //
    // Constructor:
    //
    PacketIF(const char *server, int port, boolean local, boolean asClient);

    //
    // Destructor:
    //
    ~PacketIF();

    //
    // Initializer
    //
    virtual void initializePacketIO();

    int inError() {return this->error;}

    void setHandler(int type,
		    PacketHandlerCallback callback,
		    void *clientData,
		    const char *matchString = NULL);

    //
    // Error and echo handling callbacks.  Echos are not performed if no
    // callback has been set up.  Errors are dumped to stderr if nothing
    // has been set up.
    //
    void setErrorCallback(PacketIFCallback callback, void *clientData);
    PacketIFCallback getErrorCallback(void **clientData = NULL);
    void setEchoCallback(PacketIFCallback callback, void *clientData);
    PacketIFCallback getEchoCallback(void **clientData = NULL);

    void sendBytes(const char *string);
    void sendImmediate(const char *string);

    //
    // Return TRUE if there was NOT a packet error.
    // If returning TRUE and data!=NULL, then *data is set to the value 
    // passed to the endReceiveContinuous() call.
    //
    boolean receiveContinuous(void **data = NULL);

    //
    // Terminates receiveContinuous() and causes it to set its *data to
    // the data value passed in.
    //
    void endReceiveContinuous(void *data  = NULL);

    //
    // This is the file descriptor.  It may ONLY be used to write a
    // network to the executive between sendMacroStart and sendMacroEnd.
    // Anything written to this FILE must be also sent to the routine
    // accessed via "getEchoCallback".
    FILE *getFILE() { return this->stream; }


    void         executeLinkCommand(const char *c, int id);

    //
    // Defer handing of messages until the function h is called (periodically)
    // and returns TRUE.  At that point handling is reenabled.
    // If stalling was not already enabled and there are no other problems,
    // then we return TRUE, otherwise FALSE.
    // Use isPacketHandlingStalled() to determine if handling is currently 
    // stalled.
    //
    boolean stallPacketHandling(StallingHandler h, void *data);
    boolean isPacketHandlingStalled();

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassPacketIF;
    }
};
#endif // _PacketIF_h
