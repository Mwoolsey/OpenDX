/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#include "DXApplication.h"

#if defined(windows) && defined(HAVE_WINSOCK_H)
#include <winsock.h>
#elif defined(HAVE_CYGWIN_SOCKET_H)
#include <cygwin/socket.h>
#elif defined(HAVE_SYS_SOCKET_H)
#include <sys/socket.h>
#endif

#include <Xm/RepType.h>

#if USE_PROFILING_ON_ALPHA
#ifdef alphax // for monitoring software
extern "C" {
extern void __start();
extern unsigned long _etext;
}
#include <sys/syslimits.h>
#include <mon.h>
#endif
#endif

#include <stdio.h>
#include <Xm/Xm.h>          /* Motif Toolkit */

// #include <Mrm/MrmPubli.h>    /* Mrm */

#ifdef intelnt
#include <stdlib.h>        /* HCL - exit prototype               */
#include <X11/XlibXtra.h>          /* HCL - HCLXmInit prototype          */
#endif

//
// Used by the assert macro.
//
const char *AssertMsgString = "Internal error detected at \"%s\":%d.\n";
 
int main(unsigned int argc,
	  char**       argv)
{
#if defined(HAVE_HCLXMINIT)
// The following is not needed with the recent Exceed XDK. If you're
// using an XDK < 11.0 then uncomment the following command.
//    HCLXmInit();
#endif

#ifdef intelnt
    WSADATA *wsadata = new WSADATA;
    WSAStartup(MAKEWORD(1,1),wsadata);
    delete wsadata;
#endif

#ifdef DXD_IBM_OS2_SOCKETS
    sock_init();
#endif

#if USE_PROFILING_ON_ALPHA
    //
    // For machines with built-in profiling.  This mechanim profiles everything
    // in the executable except shared libs.  (That's everything between __start
    // and _etext.  Smaller ranges are possible by using other names.)
    // Invoking moncontrol(0) / moncontrol(1) provides a mechanism to start and stop
    // profiling -- you could add a menubar button for engineers only in order
    // to profile a chunk of code, or you could shut of profiling after startup.
    //
    // The PROFDIR env variable is a directory name to hold the results.  It is used
    // by the monitoring software, not by dx.
    //
#ifdef alphax
    boolean profiling_on = FALSE;
    char *pdir = (char *)getenv ("PROFDIR");
    if ((pdir)&&(pdir[0])) {
	if (monstartup ((void *)__start, (void *)&_etext)==-1) {
	    perror("monstartup");
	} else {
	    profiling_on = TRUE;
	}
    }
#endif
#endif

    //
    // Initialize Xt Intrinsics, build all the windows, and enter event loop.
    // Note that all the windows are created elsewhere (<Application>App.C),
    // and managed in the application initialization routine.
    //
    if (NOT theApplication)
    {
	theApplication = new DXApplication("DX");
    }

    // add *tearOffModel:: XmTEAR_OFF_ENABLED/XmTEAR_OFF_DISABLED
    XmRepTypeInstallTearOffModelConverter();
    if (!theApplication->initialize(&argc, argv))
	exit(1);

    theApplication->handleEvents();

    //
    // This is a very nasty hack for SunOS 4.1 when displaying on a display
    // other than the console (e.g. ibm6000).  When exiting, if the application
    // is deleted, we get a core dump.  I haven't verified that the problem
    // goes away with X11R4, but want to make us try and deal with this then.
    //
#if !defined(sun4) || (defined(XlibSpecificationRelease) && XlibSpecificationRelease > 4)
    delete theApplication;
#endif

#if USE_PROFILING_ON_ALPHA
#ifdef alphax
    if (profiling_on) {
	monitor(0);
    }
#endif
#endif

#ifdef DXD_WINSOCK_SOCKETS    //SMH cleanup Win Sockets
    CloseXlibConnection();
    WSACleanup();
#endif

    return 0;
}
