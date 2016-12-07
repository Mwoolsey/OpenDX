/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#include <float.h>

#if defined(HAVE_TYPES_H)
#include <types.h>
#endif

#if defined(HAVE_SYS_TYPES_H)
#include <sys/types.h>
#endif

#if defined(windows) && defined(HAVE_WINSOCK_H)
#include <winsock.h>
#elif defined(HAVE_CYGWIN_SOCKET_H)
#include <cygwin/socket.h>
#elif defined(HAVE_SYS_SOCKET_H)
#include <sys/socket.h>
#endif

#include "dxlP.h"

#ifdef OS2
VOID _Optlink PMCheckInputThread(void *Voidconn);
#else
void PMCheckInputThread(void *Voidconn);
#endif


#if 0
static void _dxl_InputHandler(void *clientData)
{
    DXLConnection *conn = (DXLConnection *)clientData;
    DXLHandlePendingMessages(conn);
}
#endif

    /* FIXME: arrange for _dxl_InputHandler to be called when input available
     * on the connection, or deliver a message to the main loop that must 
     * recognize this message and call DXLHandlePendingMessages(). 
     */

#ifdef OS2
DXLError DXLInitializePMMainLoop(HWND PMAppWindow, DXLConnection *conn)
{
    static  TID tidPMCheckInputThread;
    APIRET      rc;

    conn->PMAppWindow = PMAppWindow;
    rc = DosCreateEventSem(NULL,&conn->PMHandlerSemaphore,DC_SEM_SHARED,0);
    tidPMCheckInputThread = _beginthread(PMCheckInputThread,NULL,4096,conn);
    return TRUE;
}

#else

#if defined(intelnt) || defined(WIN32)

DXLError DXLInitializePMMainLoop(HWND PMAppWindow, DXLConnection *conn)	/*     ajay   */
{
    int rc;

    conn->PMAppWindow = PMAppWindow;
    conn->PMHandlerSemaphore = CreateEvent(NULL, 0, 1000, NULL);
    rc = CreateThread(NULL, 0, 
	    (LPTHREAD_START_ROUTINE) PMCheckInputThread, 
	    (LPVOID) NULL, 0, (LPVOID) conn);
   return TRUE;
}

#else

DXLError DXLInitializePMMainLoop()
{
   return FALSE;
}
#endif
#endif



/* ******************************************************************* */
/*                    Routines for PMCheckInput thread                 */
/* ******************************************************************* */

#ifdef OS2
VOID _Optlink PMCheckInputThread(void *Voidconn)
{
  DXLConnection *conn;
  int i,j;
  HAB habt;
  HMQ hmqt;
  QMSG qmsgt;
  int rc;
  ULONG SemPostCount;

  _fpreset();

  habt=WinInitialize(0);
  hmqt=WinCreateMsgQueue(habt,0);

  conn = (DXLConnection *)Voidconn;

  for (;;)
  {
    rc = select(&conn->fd,1,0,0,-1); 
    if (rc!=1)
      {
        WinDestroyMsgQueue(hmqt);
        WinTerminate(habt);
        DosExit(0,0);
      }
    rc = WinPostMsg(conn->PMAppWindow,WM_DXL_HANDLE_PENDING_MESSAGE,(ULONG)conn,0L);
    rc = DosWaitEventSem(conn->PMHandlerSemaphore,-1);
    rc = DosResetEventSem(conn->PMHandlerSemaphore, &SemPostCount);
  }
}
#else

#ifdef DXD_HAS_WINSOCKETS
void PMCheckInputThread(void *Voidconn)
{
  int rc;
  fd_set   fds;
  DXLConnection *conn;
  struct timeval  tv;

  _fpreset();

    conn = (DXLConnection *)Voidconn;

    tv.tv_usec = 1;
    tv.tv_sec = 0;

  FD_ZERO(&fds);
  FD_SET(conn->fd, &fds);
  for(;;)
  {
    rc = select(conn->fd,&fds,0,0,&tv); 
    if (rc!=1)
    {
	PostQuitMessage(WM_QUIT);
	exit(0);
    }
    rc = PostMessage(conn->PMAppWindow, WM_DXL_HANDLE_PENDING_MESSAGE, (WPARAM) conn, NULL);
    rc = WaitForSingleObject(conn->PMHandlerSemaphore, INFINITE);
    rc = ResetEvent(conn->PMHandlerSemaphore);
  }



}

#else

void PMCheckInputThread(void *Voidconn)
{
}

#endif
#endif

/*****************************************************************************\

  The following are patches to allow IBM OS/2 sockets to send messages that
  cross a segment boundary (which *could* happen for any message longer than
  4 bytes).  This is necessary because the OS/2 socket calls are still
  16-bit code.  When the library goes 32-bit, this should no longer be
  needed.  suits 6/94.

\*****************************************************************************/

/*  This code is written for non-Berkeley select calls.  Thus must override */
/*  the select macro if it has been defined.                                */

#ifdef select
#undef select
#endif
#ifdef BSD_SELECT
#undef BSD_SELECT
#endif

int _dxl_os2_send(int s, char *ExternalBuffer, int TotalBytesToSend, int Flags)
{
#ifdef OS2
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
      if (send(s, &ExternalBuffer[BuffPtr], BytesToSend, Flags)<1)
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
#endif
#ifdef DXD_HAS_WINSOCKETS
    int BuffPtr;

    BuffPtr = send(s, ExternalBuffer, TotalBytesToSend, Flags);
    return BuffPtr;

#endif
  return (int) -1;
}

int _dxl_os2_recv(int s, char *ExternalBuffer, int BuffSize, int Flags)
{
#ifdef OS2
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
#endif
#ifdef DXD_HAS_WINSOCKETS
    int BuffPtr;

    BuffPtr = recv(s, ExternalBuffer, BuffSize, Flags);
    return (int) BuffPtr;
#endif

  return (int) -1;
}
