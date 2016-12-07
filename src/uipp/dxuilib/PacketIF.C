/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
#include <dxconfig.h>
#include "../base/defines.h"

#if defined(_AIX41)
#include <strings.h>
#endif

#include "DXStrings.h"

#include "PacketIF.h"
#include "ListIterator.h"
#include "QueuedPackets.h"


#include "DXApplication.h" // for getPacketIF()
#include "Application.h"
#include "ErrorDialogManager.h"
#include "WarningDialogManager.h"

#if defined(HAVE_ERRNO_H)
extern "C" {
#include <errno.h>
}
#else
#include <errno.h>
#endif

#if defined(HAVE_CTYPE_H)
#include <ctype.h>
#endif

#if defined(HAVE_FCNTL_H)
#include <fcntl.h>
#endif

#if defined(HAVE_NETDB_H)
#ifdef alphax
extern "C" {
#include <netdb.h>
}
#else
#include <netdb.h>
#endif
#endif

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#if defined(HAVE_IO_H)
#include <io.h>
#endif

#if defined(HAVE_ARPA_INET_H)
#include <arpa/inet.h>
#endif

#if defined(HAVE_NETINET_IN_H)
#include <netinet/in.h>
#endif

#if defined(sgi)
#include <bstring.h>
#endif
#if defined(HAVE_SYS_SELECT_H)
#include <sys/select.h>
#endif

#ifdef aviion 
extern "C" {
	int getsockname(int,struct sockaddr*, int*);
	int bzero(char*,int);
}
#endif

#if defined(HAVE_TIME_H)
#include <time.h>
#endif

#if defined(HAVE_SYS_STAT_H)
#include <sys/stat.h>
#endif

#if defined(HAVE_SYS_TIMEB_H)
#include <sys/timeb.h>
#endif

#if defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#endif

#if defined(HAVE_SYS_TYPES_H)
#include <sys/types.h>
#endif

#if defined(HAVE_SYS_UN_H)
#include <sys/un.h>
#endif

#if defined(HAVE_SYS_UTSNAME_H)
#include <sys/utsname.h>
#endif

#if defined(HAVE_GETDTABLESIZE)
extern "C" int getdtablesize();
#else
#define getdtablesize() FD_SETSIZE
#endif

#ifdef  USING_WINSOCKS
int DXMessageOnSocket(int s);
int SetSocketMode(int  s, int iMode);
static short sTimeLoop = 1;
#endif

#ifdef alphax
extern "C"   unsigned short htons (unsigned short hostshort) ;
extern "C" int select(
					  int nfds,
					  fd_set *readfds,
					  fd_set *writefds,
					  fd_set *exceptfds,
struct timeval *timeout) ;
extern "C" unsigned short ntohs (unsigned short netshort) ;
#endif

#ifdef OS2
#include <sys/ioctl.h>
#include <net/route.h>
#include <net/if.h>
#include <net/if_arp.h>
#endif


#ifdef DXD_NON_UNIX_SOCKETS  //SMH hack it out hack it in
#undef send
#define fdopen _fdopen
#endif

#define SOCK_QUEUE_LENGTH       1
#define SOCK_ACCEPT_TIMEOUT     60      /* Seconds */

#ifdef USING_WINSOCKS
#define close closesocket
#define read( a, b, c ) UxRecv( a, b, c, 0 )
#endif

#ifdef DXD_IBM_OS2_SOCKETS
#define close soclose
#define read( a, b, c ) UxRecv( a, b, c, 0 )
#define EPIPE SOCEPIPE
#endif

/***
*** Packet types:
***/

const char* PacketIF::PacketTypes[] =
{
	"UNKNOWN",
	"$int",		/* PacketIF::INTERRUPT	*/
	"$sys",		/* PacketIF::SYSTEM		*/
	"$ack",		/* PacketIF::ACK		*/
	"$mac",		/* PacketIF::MACRODEF		*/
	"$for",		/* PacketIF::FOREGROUND	*/
	"$bac",		/* PacketIF::BACKGROUND	*/
	"$err",		/* PacketIF::PKTERROR		*/
	"$mes",		/* PacketIF::MESSAGE		*/
	"$inf",		/* PacketIF::INFO		*/
	"$lin",		/* PacketIF::LINQUIRY		*/
	"$lre",		/* PacketIF::LRESPONSE	*/
	"$dat",		/* PacketIF::LDATA		*/
	"$sin",		/* PacketIF::SINQUIRY		*/
	"$sre",		/* PacketIF::SRESPONSE	*/
	"$dat",		/* PacketIF::SDATA		*/
	"$vin",		/* PacketIF::VINQUIRY		*/
	"$vre",		/* PacketIF::VRESPONSE	*/
	"$dat",		/* PacketIF::VDATA		*/
	"$com",		/* PacketIF::COMPLETE		*/
	"$imp",		/* PacketIF::IMPORT		*/
	"$imi",		/* PacketIF::IMPORTINFO	*/
	"$lnk",		/* PacketIF::LINK		*/
	"UNKNOWN"
};

/***
*** Static variables:
***/

static
int _packet_type_count = sizeof(PacketIF::PacketTypes) /
sizeof(PacketIF::PacketTypes[0]);

/***
*** External functions:
***/

//
// A new public interface to sending packets.  We check for input
// awaiting our attention before we send bytes in order to avoid
// deadlock.
//
void PacketIF::sendPacket(int type, int packetId, const char *data, int length)
{
	// checking the queue size protects us in the case that we've queued
	// something up but in the meantime, we've emptied the input side
	// of the socket.
	if ((this->isSocketInputReady())||(this->output_queue.getSize() > 0)) {
		//
		// Record our information and queue writing it (via a workproc)
		//
		void* item = new QueuedPacket(type, packetId, data, length);
		this->output_queue.appendElement (item);
		if (this->output_queue_wpid == 0)
			this->output_queue_wpid = XtAppAddWorkProc (
			theApplication->getApplicationContext(),
			(XtWorkProc)PacketIF_QueuedPacketWP, this);
		return ;
	}
	this->_sendPacket(type, packetId, data, length);
}


/*****************************************************************************/
/* _uipSendPacket -							     */
/*                                                                           */
/*                                                                           */
/*****************************************************************************/

void PacketIF::_sendPacket(int type, int packetId, const char *data, int length)
{

	if (this->stream == NUL(FILE*))
		return;

	if (data && length <= 0)
		length = STRLEN(data);

#if !defined(DXD_NON_UNIX_SOCKETS)
	if ((fprintf
		(this->stream,
		(length > 0? "|%d|%s|%d|%s|\n": "|%d|%s|%d||\n"),
		packetId,
		PacketIF::PacketTypes[type],
		length,
		data) <= 0) ||
		(fflush(this->stream) == EOF))
	{
		this->handleStreamError(errno,"PacketIF::sendPacket");
	}
#else
	int l;
	char *tempbuf = new char [length+64];
	l = sprintf
		(tempbuf,
		(length > 0? "|%d|%s|%d|%s|\n": "|%d|%s|%d||\n"),
		packetId,
		PacketIF::PacketTypes[type],
		length,
		data);

	if (UxSend (this->socket, tempbuf, l, 0) == -1 )
	{
		this->handleStreamError(errno,"PacketIF::sendPacket");
	}
	delete tempbuf;
#endif

	if (this->echoCallback)
	{
		char *echo_string = new char[length + 32];

		sprintf(echo_string, "%d [%s]: ", packetId, PacketIF::PacketTypes[type]);
		if ((length) && (data))
			strncat(echo_string, data, length);
		(*this->echoCallback)(this->echoClientData, echo_string);

		delete echo_string;
	}
}



/*****************************************************************************/
/* uipPacketCreate -							     */
/*                                                                           */
/*                                                                           */
/*****************************************************************************/

//
// The number of instances of packet interfaces.
//
static int pif_count = 0;

PacketIF::PacketIF(const char *host, int port, boolean local, boolean asClient)
{
	ASSERT(host);

	// see comment in PacketIF.h
	this->wpClientData = new (PacketIF *);
	(*this->wpClientData) = this;

#if defined(HAVE_SYS_UN_H)
	ASSERT(port > 0);
#endif
	//
	// If this is the first instance of a packet interface, then set up
	// to ignore SIGPIPE (but we watch for EPIPE).
	//
#if defined(HAVE_SYS_UN_H)
	if (pif_count++ == 0)
		signal(SIGPIPE, SIG_IGN);
#endif

	this->socket = -1;
	this->error = FALSE;
	this->deferPacketHandling = FALSE;
	this->stream = NULL;
	this->inputHandlerId = 0;
	this->workProcTimerId = 0;
	this->workProcId = 0;
	this->line = (char *)MALLOC(this->alloc_line_length = 2000);
	this->line[0] = '\0';
	this->line_length = 0;

	this->echoCallback = NULL;
	this->echoClientData = NULL;
	this->endReceiving = TRUE;

	this->linkHandler = NULL;
	this->stallingWorker = NULL;

	if (asClient)
		this->connectAsClient(host, port, local);
	else
		this->connectAsServer(port);

	this->output_queue_wpid = 0;


	if (!this->error)
	{

#if !defined(DXD_NON_UNIX_SOCKETS) 
		this->stream = fdopen(this->socket, "w");
#else
		this->stream = (FILE *)this->socket;
#endif

		this->installInputHandler();
	}
}


/*****************************************************************************/
/* uipPacketDisconnect -						     */
/*                                                                           */
/*                                                                           */
/*****************************************************************************/

PacketIF::~PacketIF()
{
	// see comment in PacketIF.h
	(*this->wpClientData) = NULL;

	//
	// If this is the last instance of a packet interface, reset to the
	// default action for SIGPIPE.
	//
#if defined(HAVE_SYS_UN_H)     //SMH  NT shouldn't do this - no stream was opened
	if (--pif_count == 0)
		signal(SIGPIPE, SIG_DFL);

	if (this->stream != NULL)
		fclose(this->stream);
#endif
	close(this->socket);
	if (this->inputHandlerId != 0)
		this->removeInputHandler();

	/*
	* Reset the socket info.
	*/
	this->stream = NUL(FILE*);
	this->socket = -1;

	FREE(this->line);

	if (this->linkHandler)
		delete this->linkHandler;

	PacketHandler *h;
	while ( (h = (PacketHandler*)this->handlers.getElement(1)) ) {
		this->handlers.deleteElement(1);
		delete h;
	}


	this->removeWorkProc();
	this->removeWorkProcTimer();


#if 0
	/*
	* Flush the handler queue.
	*/
	for (p = packet->queue; p != NUL(UipPacketHandler*); p = q)
	{
		q = p->next;
		uiuFree((char*)p);
	}
	packet->queue = NUL(UipPacketHandler*);

	/*
	* Flush the buffer.
	*/
	_line[0]     = NUL(char);
	_line_length = 0;
#endif

	if (this->output_queue_wpid)
		XtRemoveWorkProc(this->output_queue_wpid);
	ListIterator li(this->output_queue);
	QueuedPacket* qp;
	while ((qp=(QueuedPacket*)li.getNext()))
		delete qp;
}

void PacketIF::initializePacketIO()
{
	this->setHandler(PacketIF::MESSAGE,
		PacketIF::ProcessMessage,
		(void*)this);
	this->setHandler(PacketIF::INFORMATION,
		PacketIF::ProcessInformation,
		(void*)this);
	this->setHandler(PacketIF::INTERRUPT,
		PacketIF::ProcessInterrupt,
		(void*)this);
	this->setHandler(PacketIF::PKTERROR,
		PacketIF::ProcessError,
		(void*)this);
	this->setHandler(PacketIF::PKTERROR,
		PacketIF::ProcessErrorWARNING,
		(void*)this,
		"WARNING");
	this->setHandler(PacketIF::INTERRUPT,
		PacketIF::ProcessBeginExecNode,
		(void*)this,
		"begin ");
	this->setHandler(PacketIF::INTERRUPT,
		PacketIF::ProcessEndExecNode,
		(void*)this,
		"end ");
	this->setHandler(PacketIF::PKTERROR,
		PacketIF::ProcessErrorERROR,
		(void*)this,
		"ERROR");
	this->setHandler(PacketIF::LINK,
		PacketIF::ProcessLinkCommand,
		(void*)this);

	this->setErrorCallback(PacketIF::HandleError,
		(void*)this);
}

void PacketIF::installWorkProcTimer()
{
	if (this->workProcTimerId)
		return;

	this->workProcTimerId = XtAppAddTimeOut(
		theApplication->getApplicationContext(),
		1000,       // 1 second
		(XtTimerCallbackProc)PacketIF_InputIdleTimerTCP,
		(XtPointer)this->wpClientData);
}
void PacketIF::removeWorkProcTimer()
{
	if (this->workProcTimerId) {
		XtRemoveTimeOut(this->workProcTimerId);
		this->workProcTimerId = 0;
	}
}
extern "C" void PacketIF_InputIdleTimerTCP(XtPointer clientData,
										   XtIntervalId *id)
{
	PacketIF *p = *(PacketIF**)clientData;
	if (! p)
		return;

	p->workProcTimerId = 0;  // Xt uninstalls this automatically
	if (p->deferPacketHandling)
		PacketIF_InputIdleWP(clientData);
}
void PacketIF::installWorkProc()
{
	if (this->workProcId)
		return;

	this->workProcId = XtAppAddWorkProc(
		theApplication->getApplicationContext(),
		(XtWorkProc)PacketIF_InputIdleWP,
		(XtPointer)this->wpClientData);

}
void PacketIF::removeWorkProc()
{
	if (this->workProcId) {
		XtRemoveWorkProc(this->workProcId);
		this->workProcId = 0;
	}
}
Boolean PacketIF::sendQueuedPackets()
{
	if (this->output_queue.getSize() == 0) {
		this->output_queue_wpid = 0;
		return TRUE;// Yes, remove me from the list
	}

	QueuedPacket* qp = (QueuedPacket*)this->output_queue.getElement(1);
	this->output_queue.deleteElement(1);
	qp->send(this);
	delete qp;

	return FALSE; //No, don't remove me.  Keep on calling me.
}

Boolean PacketIF_QueuedPacketWP(XtPointer clientData)
{
	PacketIF *p = (PacketIF*)clientData;
	return p->sendQueuedPackets();
}

Boolean PacketIF_InputIdleWP(XtPointer clientData)
{
	PacketIF *p = *(PacketIF**)clientData;
	if (! p)
		return True;

	//
	// Check to see if message handling is stalled.  If so, then
	//
	//
	if (p->isPacketHandlingStalled()) {
		ASSERT(p->stallingWorker);
		ASSERT(p->deferPacketHandling);
		if (p->stallingWorker(p->stallingWorkerData)) {
			p->stallingWorker = NULL;
			p->deferPacketHandling = FALSE;
			p->packetReceive(FALSE); // Process read()'d but unhandled packets
		}
	} else
		p->deferPacketHandling = FALSE;

	boolean r = !p->deferPacketHandling;

	if (r)
		p->workProcId = 0;   // Xt will be removing it.

	//
	// We have one workProc for the queued packets that we want
	// to send to the exec.  We have another workProc for a
	// DXLink connection.  The DXLink workProc busy-waits
	// until it finds out that an execution has completed.  If
	// we have queued output for the exec, then we better make
	// sure that it gets sent before we busy-wait.  Otherwise,
	// we might actually be sitting on the command we want to
	// send to the exec to make it start executing in the first
	// place.
	//
	// Here, pif might/might not be the same as p.  But since we
	// have 2 workProcs installed at the same time, we can't be
	// sure which will be called and which will be starved.  Another
	// way to handle this case would be to force a call to
	// other workProcs also, but I don't think there is a way to
	// do that.  So, really what I'm doing here is to ensure that
	// if this workProc gets called, then the effect is the about
	// the same as if both workProcs were called.  The "normal"
	// Xt behavior for workProcs is to keep calling one until
	// it removes itself, and only then start calling the other.
	//
	PacketIF* pif = (PacketIF*)theDXApplication->getPacketIF();
	if (pif->output_queue.getSize() != 0) {
		pif->sendQueuedPackets();
	}

	return r;
}


//
// Some documentation on this somewhat convoluted, but important, way of
// processing socket input is warranted.  The primary goal is to make it
// so that real X events (mouse clicks and motion, and keyboard hits) take
// priority over socket input.  So the way this is done, is by permanantly
// registering the socket input handler (...ProcessSocketInputICB), but
// making it return without reading from the socket unless the idle time
// work procedure has been called since the last packet on the socket was
// handled.  The work procedure (...InputIdleWP) is only called when there
// are no events on the mouse or keyboard and it's primary duty is to
// enable the input handler (ProcessSocketInputICB) to process sockets.
//
// There is one other point to be made and that is that when there are
// multiple socket connections to the UI (i.e. one to dxexec and one to a
// DXLink app), one of the sockets (i.e. DXLink app) can be stalled so
// that the input handler continues to be called on the socket, causing
// the work procedure to not be called on the other (i.e. dxexec) socket.
// This has the nasty effect of dead-locking the sytem.  So, to handle
// this, we install a timer procedure (...InputIdleTimerTCP) if the input
// handler is called but socket input handling is deferred.  The timer
// procedure, when called, just makes a call to the work procedure,
// thereby forcing the input handler to periodically handle packets,
// regardless of the event stream..
//
extern "C" void PacketIF_ProcessSocketInputICB(XtPointer    clientData,
											   int*       /* socket */,
											   XtInputId* /* id */)
{
	PacketIF *p = (PacketIF *)clientData;

	ASSERT(p);

	if (p->deferPacketHandling) {
		if (!p->workProcTimerId)
			p->installWorkProcTimer();
		return;
	}

	if (p->stream == NULL)
		return;

	/*
	* Basically, we hand over the the processing of the socket input
	* completely over to the packet handling routine....
	*/
	p->packetReceive();

	/*
	* If the connection has been severed, handle the error.  If someone else
	* handled the error, we're done.
	*/
	if (p->stream == NULL)
		return;
	if (p->error)
	{
		p->handleStreamError(errno,"(XtInputCallbackProc)PacketIF_ProcessSocketInputICB");
		p->deferPacketHandling = FALSE;
	}
	else
	{
		p->deferPacketHandling = TRUE;
		p->installWorkProc();
	}
}






/*****************************************************************************/
/* uipPacketSetHandler -						     */
/*                                                                           */
/*                                                                           */
/*****************************************************************************/

void
PacketIF::setHandler(int                     type,
					 PacketHandlerCallback callback,
					 void                   *clientData,
					 const char	      *matchString)
{
	PacketHandler *p;

	if (clientData == NULL)
		fprintf(stderr, "null client data\n");

	/*
	* See if the handler is already in the queue.
	*/
	ListIterator li(this->handlers);
	for ( ; (p = (PacketHandler*)li.getNext()); )
		if (p->getType() == type && p->match(matchString))
			break;
	if (callback == NULL)
	{
		if (p)
		{
			this->handlers.deleteElement(li.getPosition() - 1);
			delete p;
		}
	}
	else
	{
		PacketHandler *h = new PacketHandler(TRUE,
			type,
			0,
			callback,
			clientData,
			matchString);
		if (p)
		{
			this->handlers.replaceElement((void *)h, li.getPosition() - 1);
			delete p;
		}
		else
			this->handlers.insertElement((void *)h, 1);
	}
}

boolean PacketIF::isSocketInputReady()
{
	//
	// If messages are waiting to be read from the socket, then
	// postpone handling and read them in order to prevent deadlock
	//
#if defined(USING_WINSOCKS)
	u_long rc;

	if (ioctlsocket(this->socket, FIONREAD, (u_long *) &rc)<0)
		return FALSE;
	else if (rc > 0)
		return FALSE;
	else
		return TRUE;
#else
	int fd = fileno(this->stream);
	fd_set read_fds;
	FD_SET(fd, &read_fds);
	struct timeval no_time;
	no_time.tv_sec = 0;
	no_time.tv_usec = 0;
	int status = select (fd+1, (SELECT_ARG_TYPE*)&read_fds, NULL, NULL, &no_time);
	return (status>=1);
#endif
}

void PacketIF::sendBytes(const char *string)
{
	// checking the queue size protects us in the case that we've queued
	// something up but in the meantime, we've emptied the input side
	// of the socket.
	if ((this->isSocketInputReady())||(this->output_queue.getSize() > 0)) {
		//
		// Record our information and queue writing it (via a workproc)
		//
		void* item = new QueuedBytes(string, STRLEN(string));
		this->output_queue.appendElement (item);
		if (this->output_queue_wpid == 0)
			this->output_queue_wpid = XtAppAddWorkProc (
			theApplication->getApplicationContext(),
			(XtWorkProc)PacketIF_QueuedPacketWP, this);
		return ;
	}
	this->_sendBytes(string);
}

void PacketIF::_sendBytes(const char* string)
{
	if (this->stream == NUL(FILE*))
		return;

	int length = STRLEN(string);

#if !defined(DXD_NON_UNIX_SOCKETS)
	if (fputs(string,this->stream) < 0 ||
		(fflush(this->stream) == EOF))
	{
		this->handleStreamError(errno,"PacketIF::sendBytes");
	}
#else
	if (UxSend (this->socket, string, length, 0) == -1 )
	{
		this->handleStreamError(errno,"PacketIF::sendBytes");
	}
#endif

	if (this->echoCallback)
		(*this->echoCallback)(this->echoClientData, (char*)string);
}

void PacketIF::sendImmediate(const char *string)
{
	// checking the queue size protects us in the case that we've queued
	// something up but in the meantime, we've emptied the input side
	// of the socket.
	if ((this->isSocketInputReady())||(this->output_queue.getSize() > 0)) {
		//
		// Record our information and queue writing it (via a workproc)
		//
		void* item = new QueuedImmediate(string, STRLEN(string));
		this->output_queue.appendElement (item);
		if (this->output_queue_wpid == 0)
			this->output_queue_wpid = XtAppAddWorkProc (
			theApplication->getApplicationContext(),
			(XtWorkProc)PacketIF_QueuedPacketWP, this);
		return ;
	}
	this->_sendImmediate(string);
}

void PacketIF::_sendImmediate(const char *string)
{
	if (this->stream == NUL(FILE*))
		return;

	int length = STRLEN(string) + 2;
	char *newString = new char[length + 1];
	strcpy(newString, "$");
	strcat(newString, string);
	strcat(newString, "\n");

#if !defined(DXD_NON_UNIX_SOCKETS)
	if (fputs(newString,this->stream) < 0 ||
		(fflush(this->stream) == EOF))
	{
		this->handleStreamError(errno,"PacketIF::sendImmediate");
	}
#else
	if (UxSend (this->socket, newString, length, 0) == -1 )
	{
		this->handleStreamError(errno,"PacketIF::sendPacket");
	}
#endif

	if (this->echoCallback)
	{
		char *echo_string = new char[length + 32];

		sprintf(echo_string, "-1: ");
		strncat(echo_string, newString, length);
		(*this->echoCallback)(this->echoClientData, echo_string);

		delete echo_string;
	}

	delete newString;
}

//
// Handle stream read/write errors.  We handle SIGPIPE/EPIPE specially.
// SIGPIPE is ignored and then if we get an EPIPE on a write, then we
// know the server or other connect has gone away.
// The errnum is passed in for portability, but is ignored on UNIX
// systems.
//
void PacketIF::handleStreamError(int errnum, const char *msg)
{
	void *errorData;
	PacketIFCallback cb;
	if ((cb = this->getErrorCallback(&errorData)) != NULL)
		(*cb)(errorData, "Connection to the server has been broken.");
	else
		fprintf(stderr, "Connection to the server has been broken.\n");

	this->error = TRUE;

	if (errnum != EPIPE && errnum != 0) {
		errno = errnum;
		perror(msg);
	}
	errno = 0;

#if defined(HAVE_SYS_UN_H)                 //SMH must do direct socket calls for NT
	if (this->stream != NULL)
		fclose(this->stream);
	this->stream = NULL;
#endif
	close(this->socket);
	if (this->inputHandlerId != 0)
		this->removeInputHandler();
}

/*****************************************************************************/
/* _uipParsePacket -							     */
/*                                                                           */
/* Parses and processes a packet.					     */
/*                                                                           */
/*****************************************************************************/

void
PacketIF::parsePacket()
{
	int               id;
	int               type;
	int               data_length;
	int               i=0;
	int               j;
	int               k;
	char              *token;

	// Guarantee token always long enough.
	token = new char[strlen(this->line)+1];
	
	/*
	* Skip past the first vertical bar.
	*/
	if (this->line[i++] != '|')
	{
		ErrorMessage("Format error (1) encountered in packet.");
		delete token;
		return;
	}

	/*
	* Extract the id number.
	*/
	j = 0;
	while (this->line[i] != NUL(char) && this->line[i] != '|')
	{
		token[j++] = this->line[i++];
	}
	token[j] = NUL(char);

	id = atoi(token);

	/*
	* Skip past the second vertical bar.
	*/
	if (this->line[i++] != '|')
	{
		ErrorMessage("Format error (2) encountered in packet.");
		delete token;
		return;
	}

	/*
	* Extract and verify the packet type.
	*/
	j = 0;
	while (this->line[i] != NUL(char) && this->line[i] != '|')
	{
		token[j++] = this->line[i++];
	}
	token[j] = NUL(char);

	for (type = 1; type < _packet_type_count - 1; type++)
	{
		if (EqualString(token, PacketIF::PacketTypes[type]))
			break;
	}
	if (type >= _packet_type_count - 1)
	{
		ErrorMessage("Unknown packet type \"%s\" encountered.", token);
		delete token;
		return;
	}

	/*
	* Skip past the third vertical bar.
	*/
	if (this->line[i++] != '|')
	{
		ErrorMessage("Format error (3) encountered in packet.");
		delete token;
		return;
	}

	/*
	* Extract the data length.
	*/
	j = 0;
	while (this->line[i] != NUL(char) && this->line[i] != '|')
	{
		token[j++] = this->line[i++];
	}
	token[j] = NUL(char);

	data_length = atoi(token);

	/*
	* Skip past the fourth vertical bar and extract the data.
	*/
	if (this->line[i++] != '|')
	{
		ErrorMessage("Format error (4) encountered in packet.");
		delete token;
		return;
	}

	k = i + data_length;

	/*
	* Extract the data.
	*/
	j = 0;
	while (this->line[i] != NUL(char) && i < k)
	{
		token[j++] = this->line[i++];
	}
	token[j] = NUL(char);

	if (this->line[i] != '|')
	{
		ErrorMessage("Format error (5) encountered in packet.");
		delete token;
		return;
	}

	if (this->echoCallback)
	{
		char *s = new char[STRLEN(token) + STRLEN(PacketIF::PacketTypes[type]) + 50];
		sprintf(s, "Received %s:%d: %s\n",
			PacketIF::PacketTypes[type], id, token);
		(*this->echoCallback)(this->echoClientData, s);
		delete s;
	}

	/*
	* Find the handler to handle this packet.
	*/
	PacketHandler*  h;

	//
	// Skip past the colon and the white spaces.
	const char *matchString = token;
	while(*matchString == ' ')
		++matchString;
	while(isdigit(*matchString))
		++matchString;
	if (*matchString == ':')
		++matchString;
	while(*matchString == ' ')
		++matchString;
	//
	// Check for strings that exactly match the handler's match string
	// (In the case of data-driven interactors, there will never be an
	// exact match.)
	//
	for (ListIterator li(this->handlers);
		(h = (PacketHandler*)li.getNext()); )
	{
#if 111
		int hnumber = h->getNumber();
		int htype = h->getType();
		const char *str = h->getMatch();
		if (str &&
			((hnumber == id && (htype == type || htype == -1)) ||
			(hnumber == 0  && htype == type))                    &&
			h->match(matchString))
#else
		if ((h->getNumber() == id && h->getType() == type &&
			h->matchFirst(matchString)) ||
			(h->getNumber() == id && h->getType() == -1 &&
			h->matchFirst(matchString)) ||
			(h->getNumber() == 0  && h->getType() == type &&
			h->matchFirst(matchString)))

#endif
		{
			break;
		}
	}

#if 111
	//
	// If we didn't find an exact match, then check for strings that match
	// the handler's match string for the length of the match string.
	//
	if (!h) {
		int longest_match_so_far = 0;
		PacketHandler*  best_handler = NUL(PacketHandler*);
		for (ListIterator li(this->handlers);
			(h = (PacketHandler*)li.getNext()); )
		{
			int hnumber = h->getNumber();
			int htype = h->getType();
			const char *str = h->getMatch();
			if (str &&
				((hnumber == id && (htype == type || htype == -1)) ||
				(hnumber == 0  && htype == type))                    &&
				h->matchFirst(matchString))
			{
				int len = strlen(str);
				if (len > longest_match_so_far) {
					best_handler = h;
					longest_match_so_far = len;
				}
			}
		}
		h = best_handler;
	}
	//
	// If we still didn't find an match, then check for handlers that don't
	// have match string specified, implying they match any string.
	//
	if (!h) {
		for (ListIterator li(this->handlers);
			(h = (PacketHandler*)li.getNext()); )
		{
			int hnumber = h->getNumber();
			int htype = h->getType();
			const char *str = h->getMatch();
			if ((str == NULL) &&
				((hnumber == id && (htype == type || htype == -1)) ||
				(hnumber == 0  && htype == type)))
			{
				break;
			}
		}
	}
#endif

	if (h)
	{
		h->callCallback(id, token);

		if (!h->getLinger())
		{
			this->handlers.removeElement((void*)h);
			delete h;
		}
	}
	delete token;
}


/*****************************************************************************/
/* uipPacketReceive -							     */
/*                                                                           */
/* Reads a buffer full of input from the socket and process the packets.     */
/*                                                                           */
/* NOTE:								     */
/*   The amount of input may vary in size; and partial lines must be	     */
/*   expected.  This means that the following code must be able to	     */
/*   handle multiple lines, as well as partial lines.			     */
/*                                                                           */
/*****************************************************************************/

void
PacketIF::packetReceive(boolean readSocket)
{
	int  buflen, length;
	int  i;
	char string[1024];
	char buffer[4096 + 1];

#ifdef	USING_WINSOCKS
	if(DXMessageOnSocket(this->socket) <= 0) return ;
#endif
	buffer[0] = NUL(char);
	buflen = 0;
	if (readSocket) {
		buflen = read(this->socket, buffer, 4096);
		if (buflen <= 0)
		{
			this->error = TRUE;
			return;
		}
		buffer[buflen] = NUL(char);
	} else {
		//
		// Copy the packets that had been saved after being read before
		// a deferPacketHandling was requested.
		// This is not the most efficient way to do things, but it gets
		// the job done.
		//
		ASSERT(!this->deferPacketHandling);
		strcpy(buffer,this->line);
		buflen = STRLEN(buffer);
		this->line[0]     = NUL(char);
		this->line_length = 0;
	}

	i              = 0;
	while (!this->deferPacketHandling)
	{
		length = 0;
		while(length < 1023 &&
			buffer[i] != NUL(char) &&
			buffer[i] != '\n')
		{
			string[length++] = buffer[i++];
		}
		if (buffer[i] == '\n')
		{
			string[length++] = buffer[i++];
		}
		string[length] = NUL(char);

		if (length <= 0)
		{
			break;
		}

		/*
		* Append the retrieved line (fragment) to contents of current line
		* (which may be empty or filled with a partial line).
		*/
		this->line_length += length;
		while (this->line_length > this->alloc_line_length)
			this->line = (char *)REALLOC(this->line,
			this->alloc_line_length += 1000);

		strcat(this->line, string);


		/*
		* If a complete line has been assembled...
		*/
		if (this->line[this->line_length - 1] == '\n')
		{
			/*
			* Remove the trailing newline character.
			*/
			this->line_length--;
			this->line[this->line_length] = NUL(char);

			/*
			* Process the completed line....
			*/
			this->parsePacket();

			/*
			* Reinitialize the line.
			*/
			this->line[0]     = NUL(char);
			this->line_length = 0;
		}
	}

	//
	// If we didn't finish the buffer (because of a stalling of the packet
	// handling), then concatenate onto the existing
	// line for retrieve later (see above).
	//
	if (i != buflen) {
		ASSERT(this->isPacketHandlingStalled());
		int leftover = buflen-i+1;
		ASSERT(leftover > 0);
		ASSERT(this->line_length == 0);
		while (leftover > this->alloc_line_length)
			this->line = (char *)REALLOC(this->line,
			this->alloc_line_length += leftover+1);
		memcpy((void*)this->line, (void*)(buffer+i), leftover);
		this->line_length = leftover;
		this->line[leftover] = 0;
	}
}

void PacketIF::setErrorCallback(PacketIFCallback callback,
								void              *clientData)
{
	this->errorCallback = callback;
	this->errorClientData = clientData;
}
PacketIFCallback PacketIF::getErrorCallback(void **clientData)
{
	if (clientData != NULL)
		*clientData = this->errorClientData;
	return this->errorCallback;
}
void PacketIF::setEchoCallback(PacketIFCallback callback,
							   void              *clientData)
{
	this->echoCallback = callback;
	this->echoClientData = clientData;
}
PacketIFCallback PacketIF::getEchoCallback(void **clientData)
{
	if (clientData != NULL)
		*clientData = this->echoClientData;
	return this->echoCallback;
}

boolean PacketIF::receiveContinuous(void **data)
{
	boolean noerr = TRUE;

	this->endReceiving = FALSE;
	this->endReceiveData = NULL;
	while (!this->endReceiving)
	{
		ASSERT(!this->deferPacketHandling); // Otherwise, we never exit
		this->packetReceive();
		if (this->error)
		{
			this->handleStreamError(errno,"PacketIF::receiveContinuous");
			noerr = FALSE;
			break;
		}
	}
	if (noerr && data)
		*data = this->endReceiveData;
	return noerr;
}

void PacketIF::endReceiveContinuous(void *data)
{
	this->endReceiving = TRUE;
	this->endReceiveData = data;
}

void PacketIF::installLinkHandler()
{
	this->linkHandler = new LinkHandler(this);
}

void PacketIF::executeLinkCommand(const char *c, int id)
{
	if (!this->linkHandler)
		this->installLinkHandler();

	this->linkHandler->executeLinkCommand(c,id);
}

void PacketIF::ProcessMessage(void *, int, void *p)
{
	// char *line = (char *)p;
}

void PacketIF::ProcessInformation(void *, int, void *p)
{
	// char *line = (char *)p;
}

void PacketIF::ProcessError(void *, int, void *p)
{
	// char *line = (char *)p;
}

void PacketIF::ProcessErrorERROR(void *clientData, int id, void *p)
{
	// char *line = (char *)p;
}

void PacketIF::ProcessErrorWARNING(void *, int, void *p)
{
	// char *line = (char *)p, *s;
}

void PacketIF::ProcessCompletion(void *, int , void *)
{
}

void PacketIF::ProcessInterrupt(void *clientData, int id, void *p)
{
	// char *line = (char *)p;
}

void PacketIF::ProcessBeginExecNode(void *clientData, int id, void *p)
{
	// char *line = (char *)p;
}

void PacketIF::ProcessEndExecNode(void *clientData, int id, void *p)
{
	// char *line = (char *)p;
}

void PacketIF::ProcessLinkCommand(void *clientData, int id, void *p)
{
	// char *line = (char *)p;
}

void PacketIF::HandleError(void *clientData, char *message)
{
}


static int
setSockBufSize(int sock)
{
#if defined(HAVE_SETSOCKOPT)
	char *s = getenv("DX_SOCKET_BUFSIZE");
	if (s)
	{
		SOCK_LENGTH_TYPE rq_bufsz, set_bufsz;
		SOCK_LENGTH_TYPE set_len = sizeof(set_bufsz), rq_len = sizeof(rq_bufsz);

		rq_bufsz = atoi(s);
		if (setsockopt(sock, SOL_SOCKET, SO_SNDBUF, (void *)&rq_bufsz, rq_len))
		{
			perror("setsockopt");
			return 0;
		}
		if (getsockopt(sock, SOL_SOCKET, SO_SNDBUF, (void *)&set_bufsz, &set_len))
		{
			perror("getsockopt");
			return 0;
		}
		if (rq_bufsz > set_bufsz + 10)
			WarningMessage("SOCKET bufsize mismatch: send buffer (%d %d)",
			rq_bufsz, set_bufsz);

		if (setsockopt(sock, SOL_SOCKET, SO_RCVBUF, (void *)&rq_bufsz, rq_len))
		{
			perror("setsockopt");
			return 0;
		}
		set_len = sizeof(set_bufsz);
		if (getsockopt(sock, SOL_SOCKET, SO_RCVBUF, (void *)&set_bufsz, &set_len))
		{
			perror("getsockopt");
			return 0;
		}
		if (rq_bufsz > set_bufsz + 10)
			WarningMessage("SOCKET bufsize mismatch: rcv buffer (%d %d)",
			rq_bufsz, set_bufsz);
	}
#endif
	return 1;
}
//
//
//
//
void PacketIF::connectAsClient(const char *host, int port, boolean local)
{
	struct sockaddr_in server;
#if defined(HAVE_SYS_UN_H)
	struct sockaddr_un userver;
#else
	unsigned long locaddr;
#endif
	struct hostent*    hostp;

	ASSERT(host);
#if defined(HAVE_SYS_UN_H)
	ASSERT(port > 0);
#endif

#if defined(HAVE_SYS_UN_H)     //SMH  NT can't local socket AF_UNIX

	if (local)
	{
		memset((char*)&userver, 0, sizeof(userver));
		userver.sun_family = AF_UNIX;
		sprintf(userver.sun_path, "/tmp/.DX-unix/DX%d", port);

		this->socket = ::socket(AF_UNIX, SOCK_STREAM, 0);
		if (this->socket < 0)
		{
			this->error = TRUE;
			return;
		}

		/*
		* If connect fails, then fall through to creating an inet socket.
		* (unless the host is "unix").
		*/
		if (connect(this->socket,
			(struct sockaddr *)&userver,
			sizeof(userver) - sizeof(userver.sun_path) +
			STRLEN(userver.sun_path)) >= 0)
		{
			if (! setSockBufSize(this->socket))
			{
				close(this->socket);
				this->error = TRUE;
			}
			return;
		}
		else
		{
			close(this->socket);
			if (strcmp(host, "unix") == 0)
			{
				this->socket = -1;
				this->error = TRUE;
				return;
			}
		}
	}

	memset((char *)&server, 0, sizeof(server));
#endif // SMH end bypass AF_UNIX Style socket

	server.sin_family = AF_INET;
	server.sin_port   = htons((u_short)port);

	/*

	* Get host address.
	*/
	if (isdigit(host[0]))
	{
#ifdef aviion
		server.sin_addr.s_addr = inet_addr().s_addr;
#else
		server.sin_addr.s_addr = inet_addr((char*)host);
#endif
	}
	else
	{
		/* Trying to prevent calls to name server when host is local */
#ifdef DXD_WIN
		if (local || !_stricmp((char*)host, "localhost") || !_stricmp((char*)host, "localPC"))
		{
			locaddr = inet_addr("127.0.0.1");
			memcpy((void*)&server.sin_addr, &locaddr, sizeof(unsigned long));
		}
		else
		{
			hostp = gethostbyname((char*)host);
			if (hostp == NUL(struct hostent*))
			{
				this->error = TRUE;
				return;
			}
			memcpy((void*)&server.sin_addr, hostp->h_addr, hostp->h_length);
		}
#else
		hostp = gethostbyname((char*)host);
		if (hostp == NUL(struct hostent*))
		{
			this->error = TRUE;
			return;
		}
		memcpy((void*)&server.sin_addr, hostp->h_addr, hostp->h_length);
#endif
	}

	this->socket = ::socket(AF_INET, SOCK_STREAM, 0);
	if (this->socket < 0)
	{
		this->error = TRUE;
		return;
	}


	if (connect(this->socket,
		(struct sockaddr*)&server,
		sizeof(server)) < 0)
	{
		close(this->socket);
		this->socket = -1;
		this->error = TRUE;
		return;
	}

	if (! setSockBufSize(this->socket))
	{
		close(this->socket);
		this->error = TRUE;
	}

#if  defined(DXD_IBM_OS2_SOCKETS)
	int dontblock = 1;
	if (ioctl(this->socket, FIONBIO, (char *) &dontblock, sizeof(dontblock))<0)
	{
		this->error = TRUE;
		return;
	}
	if (select((int *)&this->socket,0,1,0,-1)<=0)
	{
		this->error = TRUE;
		return;
	}
#endif


}

/*
* Open a socket port and wait for a client to connect.
* This opens 2 sockets (except on the server), one internet domain, and
* one unix domain.  It then selects on both sockets to see to which one the
* app connects, and returns the new connection, closing the one not
* selected.
*/
void PacketIF::connectAsServer(int pport)
{
	struct sockaddr_in server;
	int sock = -1;
#if defined(HAVE_SYS_UN_H)
	int usock = -1;
	struct sockaddr_un userver;
	int oldUmask;
#endif
	struct linger sl;
	SOCK_LENGTH_TYPE length;
	ushort port;
	int fd;
	int sts;
	int oldPort;
#if !defined(linux) && !defined(cygwin) && !defined(freebsd) && !defined(macos) && !defined(solaris)
	extern int errno;
#endif
	int tries;
	fd_set fds;
	int  width = getdtablesize();
	struct timeval to;

	port = pport;

retry:
	sock = ::socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0)
	{
		perror ("socket");
		fd = -1;
		goto error;
	}

	sl.l_onoff = 1;
	sl.l_linger = 0;
	setsockopt(sock, SOL_SOCKET, SO_LINGER, (char *)&sl, sizeof(sl));
#if DXD_HAS_IBM_OS2_SOCKETS
	SOCK_SETSOCKET(sock);
#endif

#if defined(HAVE_SYS_UN_H)
	usock = ::socket(AF_UNIX, SOCK_STREAM, 0);
	if (usock < 0)
	{
		perror ("socket");
		fd = -1;
		goto error;
	}

	setsockopt(usock, SOL_SOCKET, SO_LINGER, (char*)&sl, sizeof(sl));
#if DXD_HAS_IBM_OS2_SOCKETS
	SOCK_SETSOCKET(sock);
#endif
#endif

	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(port);
	length = sizeof(struct sockaddr_in);

	/* Try to find a working port, and keep trying if you get EADDRINUSE.
	* If we get some other error, try a few times because we sometimes
	* get bad error numbers on the server.
	*/
	tries = 0;
	while ((sts = bind(sock, (struct sockaddr *)&server, length)) < 0 &&
		(errno == EADDRINUSE || tries++ < 5))
	{
		oldPort = port;
		server.sin_port = htons(++port);
		if (ntohs(server.sin_port) < oldPort)
			break;
	}
	if (sts < 0)
	{
		perror ("bind");
		fd = -1;
		goto error;
	}

	if (getsockname(sock, (struct sockaddr *)&server, &length) < 0)
	{
		perror ("getsockname");
		fd = -1;
		goto error;
	}

#if defined(HAVE_SYS_UN_H)
	userver.sun_family = AF_UNIX;
	sprintf(userver.sun_path, "/tmp/.DX-unix/DX%d", port);
	length = sizeof (userver) - sizeof(userver.sun_path) +
		strlen (userver.sun_path);

	oldUmask = umask(0);
	mkdir("/tmp/.DX-unix", 0777);
	chmod("/tmp/.DX-unix", 0777);
	umask(oldUmask);

	unlink(userver.sun_path);


	/* Try to find a working port, and keep trying if you get EADDRINUSE.
	* If we get some other error, try a few times because we sometimes
	* get bad error numbers on the server.
	*/
	if ((sts = bind(usock, (struct sockaddr *)&userver, length)) < 0 &&
		errno == EADDRINUSE)
	{
		oldPort = port;
		server.sin_port = htons(++port);
		close (sock);
		close (usock);
		sock= -1;
		usock = -1;
		if (ntohs(server.sin_port) > oldPort)
			goto retry;
	}
	if (sts < 0)
	{
		perror ("bind");
		fd = -1;
		goto error;
	}
#endif

	if (listen(sock, SOCK_QUEUE_LENGTH) < 0)
	{
		perror ("listen");
		fd = -1;
		goto error;
	}
#if defined(HAVE_SYS_UN_H)
	if (listen(usock, SOCK_QUEUE_LENGTH) < 0)
	{
		perror ("listen");
		fd = -1;
		goto error;
	}
#endif

	printf ("port = %d\n", ntohs(server.sin_port));
	fflush(stdout);

	/* Wait (in select) for someone to connect.
	* if we get through the error checks, someone must want to connect to
	* our port, accept him/her.  Otherwize, block in accept until someone
	* connects.  If stdin is not a terminal, timeout after SOCK_ACCEPT_TIMEOUT
	* seconds.
	*/
	FD_ZERO(&fds);
	FD_SET(sock, &fds);

#if defined(USING_WINSOCKS)
	if (!isatty(0))
	{
		to.tv_sec = SOCK_ACCEPT_TIMEOUT*1000;
		to.tv_usec = 0;
		sts = select(sock, &fds, NULL, NULL, &to);
	}
	else
	{
		sts = select(sock, &fds, NULL, NULL, NULL);
	}
#else
#if defined(HAVE_SYS_UN_H) 
	FD_SET(usock, &fds);
	width = ((sock > usock) ? sock : usock) + 1;
#else
	width = sock + 1;
#endif
	if (!isatty(0)) {
		to.tv_sec = SOCK_ACCEPT_TIMEOUT;
		to.tv_usec = 0;
		sts = select(width, (SELECT_ARG_TYPE *) &fds, NULL, NULL, &to);
	}
	else
	{
		sts = select(width, (SELECT_ARG_TYPE *) &fds, NULL, NULL, NULL);
	}
#endif
	if (sts < 0) {
		perror("select");
		fd = -1;
		goto error;
	}
	else if (sts == 0) {
		fprintf (stderr, "connection timed out\n");
		fd = -1;
		goto error;
	}

#if defined(HAVE_SYS_UN_H)
	if (FD_ISSET(sock, &fds))
	{
#endif
		if ((fd = accept(sock, (struct sockaddr *)&server, &length)) < 0)
		{
			perror ("accept");
			fd = -1;
			goto error;
		}
#if DXD_HAS_IBM_OS2_SOCKETS
		SOCK_SETSOCKET(fd);
#endif

#if defined(HAVE_SYS_UN_H)
	}
	else
	{
		if ((fd = accept(usock, (struct sockaddr *)&userver, &length)) < 0)
		{
			perror ("accept");
			fd = -1;
			goto error;
		}
	}
#endif

	printf ("server: accepted connection from client\n");

error:
#if defined(HAVE_SYS_UN_H) 
	if (userver.sun_path[0] != '\0')
		unlink (userver.sun_path);
	if (usock >= 0)
		close (usock);
#endif
	if (sock>= 0)
		close (sock);

	if (fd < 0) {
		this->error = TRUE;
	} else {
		this->error = FALSE;
		this->socket = fd;
	}
	return;
}

void PacketIF::installInputHandler()
{
	ASSERT(this->inputHandlerId == 0);
	ASSERT(this->socket >= 0);
	this->inputHandlerId =
		XtAppAddInput(theApplication->getApplicationContext(),
		this->socket,
#if		defined(DXD_WIN)
		(XtPointer)XtInputReadWinsock,
#else
		(XtPointer)XtInputReadMask,
#endif
		(XtInputCallbackProc)PacketIF_ProcessSocketInputICB,
		(XtPointer) this);
}
void PacketIF::removeInputHandler()
{
	ASSERT(this->inputHandlerId != 0);
	XtRemoveInput(this->inputHandlerId);
	this->inputHandlerId = 0;
}
//
// Return true if packet handling is currently stalled.
//
boolean PacketIF::isPacketHandlingStalled()
{
	return this->stallingWorker == NULL ? FALSE : TRUE;
}
//
// Defer handing of messages until the function h is called (periodically)
// and returns TRUE.  At that point handling is reenabled.
// If stalling was not already enabled and there are no other problems,
// then we return TRUE, otherwise FALSE.
// Use isPacketHandlingStalled() to determine if handling is currently stalled.
//
boolean PacketIF::stallPacketHandling(StallingHandler h, void *data)
{
	if (this->isPacketHandlingStalled())
		return FALSE;

	this->deferPacketHandling = TRUE;

	this->stallingWorker = h;
	this->stallingWorkerData = data;

	return TRUE;
}

#if defined(USING_WINSOCKS)

int UxSend(int s, const char *ExternalBuffer, int TotalBytesToSend, int Flags)
{
	int BuffPtr;
	struct timeval to;
	fd_set fds;

	to.tv_sec = 10;
	to.tv_usec = 0;
	FD_ZERO(&fds);
	FD_SET(s, &fds);

	if (select(s, NULL, &fds, NULL, &to) <= 0)
		return -1;

	BuffPtr = 0;
	BuffPtr = ::send(s, ExternalBuffer, TotalBytesToSend, Flags);

	return (int) BuffPtr;
}

int UxRecv(int s, char *ExternalBuffer, int BuffSize, int Flags)
{
	int BuffPtr;
	u_long rc;
	struct timeval to;
	int i;
	fd_set fds;

	BuffPtr  = recv(s, ExternalBuffer, BuffSize, Flags);
	return (int) BuffPtr;
}

int DXMessageOnSocket(int s)
{
	u_long rc;

#if 0
	struct timeval to;
	fd_set fds;

	FD_ZERO(&fds);
	FD_SET(s, &fds);
	if(sTimeLoop == 1)
	{
		sTimeLoop++;
		SetSocketMode(s, 0);   //	BLOCKING MODE
		return 1;   //   There is something
	}

	if(sTimeLoop > 1) {
		sTimeLoop = 0;
		SetSocketMode(s, 1);   //	NON BLOCKING MODE
	}

	if (select(s,NULL,&fds,NULL,&to)<=0)
		return -1;
#endif
	if (ioctlsocket(s, FIONREAD, (u_long *) &rc)<0)
		return -1;

	return (int) rc;

}


int SetSocketMode(int  s, int iMode)
{
	u_long   rc;

	rc = iMode;
	if (ioctlsocket(s, FIONBIO, (u_long *) &rc)<0)	// NON blocking mode
		return -1;
	else
		return 0;

}

#elif  defined(DXD_IBM_OS2_SOCKETS)

/*****************************************************************************\

The following are patches to allow IBM OS/2 sockets to send messages that
cross a segment boundary (which *could* happen for any message longer than
4 bytes).  This is necessary because the OS/2 socket calls are still
16-bit code.  When the library goes 32-bit, this should no longer be
needed.  suits 6/94.

\*****************************************************************************/

int UxSend(int s, char *ExternalBuffer, int TotalBytesToSend, int Flags)
{
#define MAX_BYTES_PER_SEND 32767
#define UX_SEND_SEG_SIZE 65536
#define UX_SEND_TIMEOUT 20000
	int BytesToSend;
	int BuffPtr;
	int BytesRemaining;

	if (select((int *)&s,0,1,0,UX_SEND_TIMEOUT)<=0)
		return -1;
	BytesRemaining = TotalBytesToSend;
	BuffPtr = 0;
	while (BytesRemaining>0)
	{
		BytesToSend = (unsigned short)(-(unsigned long)&ExternalBuffer[BuffPtr]);
		if (BytesToSend == 0)
			BytesToSend = UX_SEND_SEG_SIZE;
		if (BytesToSend>BytesRemaining)
			BytesToSend = BytesRemaining;
		if (BytesToSend > MAX_BYTES_PER_SEND)
			BytesToSend = MAX_BYTES_PER_SEND;
		if (::send(s, &ExternalBuffer[BuffPtr], BytesToSend, Flags)<1)
		{
			if (sock_errno()!=SOCEWOULDBLOCK)
				return -1;
			if (select((int *)&s,0,1,0,5000)<=0)
				return -1;
		}
		else
		{
			BuffPtr += BytesToSend;
			BytesRemaining -= BytesToSend;
		}
	}
	return (int) BuffPtr;
}

int UxRecv(int s, char *ExternalBuffer, int BuffSize, int Flags)
{
#define MAX_BYTES_PER_RECV 32767
#define UX_RECV_SEG_SIZE 65536
#define UX_RECV_TIMEOUT 20000
	int BytesToReceive;
	int BuffPtr;
	int BytesRemaining;
	int NumReceived = -1;

	if (select((int *)&s,1,0,0,UX_RECV_TIMEOUT)<=0)
		return -1;
	BytesRemaining = BuffSize;
	BuffPtr = 0;
	while (NumReceived != 0 && BytesRemaining != 0)
	{
		BytesToReceive = (unsigned short)(-(unsigned long)&ExternalBuffer[BuffPtr]);
		if (BytesToReceive == 0)
			BytesToReceive = UX_RECV_SEG_SIZE;
		if (BytesToReceive>BytesRemaining)
			BytesToReceive = BytesRemaining;
		if (BytesToReceive>MAX_BYTES_PER_RECV)
			BytesToReceive = MAX_BYTES_PER_RECV;
		if ((NumReceived = recv(s, &ExternalBuffer[BuffPtr], BytesToReceive, Flags))<0)
		{
			if (sock_errno()!=SOCEWOULDBLOCK)
				return -1;
			else
				return (int) BuffPtr;
		}
		BuffPtr += NumReceived;
		BytesRemaining -= NumReceived;
	}
	return (int) BuffPtr;
}

#endif

