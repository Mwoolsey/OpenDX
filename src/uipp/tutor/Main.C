/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"

#include "TutorApplication.h"

#if defined(windows) && defined(HAVE_WINSOCK_H)
#define _WINSPOOL_      //SMH prevent name clash from uneeded included inlcudes
#include <winsock.h>
#elif defined(HAVE_CYGWIN_SOCKET_H)
#include <cygwin/socket.h>
#elif defined(HAVE_SYS_SOCKET_H)
#include <sys/socket.h>
#endif

#if defined(HAVE_HCLXMINIT)
extern "C" void HCLXmInit();
#endif



//
// Used by the assert macro.
//
const char *AssertMsgString = "Internal error detected at \"%s\":%d.\n";
 
int main(unsigned int argc,
	  char**       argv)
{
#if defined(HAVE_HCLXMINIT)
    HCLXmInit();
#endif

#ifdef DXD_WINSOCK_SOCKETS    //SMH initialize Win Sockets
    {
        WSADATA *wsadata = new WSADATA;
        WSAStartup(0x0100,wsadata);
        delete wsadata;
    }
#endif
#ifdef DXD_IBM_OS2_SOCKETS
    sock_init();
#endif
    //
    // Initialize Xt Intrinsics, build all the windows, and enter event loop.
    // Note that all the windows are created elsewhere (<Application>App.C),
    // and managed in the application initialization routine.
    //
    if (NOT theApplication)
    {
	theApplication = new TutorApplication("DXTutor");
    }

    if (!theApplication->initialize(&argc, argv))
	exit(1);

    theApplication->handleEvents();

    delete theApplication;

#ifdef DXD_WINSOCK_SOCKETS    //SMH cleanup Win Sockets
    WSACleanup();
#endif

    return 0;
}
