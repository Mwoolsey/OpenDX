/*
 * main routine for Visualization Data Explorer outboard modules.
 *
 * see below for the comment about defining USERMODULE. 
 *  then compile this file and link with libDXlite.a and any other libraries
 *  (-lm, etc) which are necessary.
 *
 */



#include <dx/dx.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


/* 
 * uncomment this next line and substitute your module name here,
 * or compile this file with the -DUSERMODULE=m_MyModule flag
 */

/* #define USERMODULE   m_MyModule    */

extern Error USERMODULE (Object *in, Object *out);

extern int   DXConnectToServer (char *host, int port);
extern int   DXInputAvailable (int dxfd);
extern Error DXCallOutboard (Error (*module)(Object *, Object *), int dxfd);



main (int argc, char **argv)
{
    int dxfd;

    /* 
     * this program gets called with two arguments: the host name
     *  and socket number which it should connect to.
     */
    if (argc != 3) {
	fprintf (stderr, 
		 "error in execution: hostname and port number not set\n");
	exit (-1);
    }

    /*
     * connect to the DX Executive which started the outboard.
     */
    dxfd = DXConnectToServer (argv[1], atoi(argv[2]));
    if (dxfd < 0) {
	fprintf (stderr, "couldn't connect to socket %d on host %s\n", 
		 atoi(argv[2]), argv[1]);
	exit (-1);
    }

    /*
     * while the socket is open, read requests for work, call the
     * module to execute them, and send the data back.
     *
     * if the module is a one-time module, DXCallOutboard closes the
     * socket.  if the module is persistent, the socket remains open.
     */
    while(1) {

        if (DXInputAvailable (dxfd) <= 0)
	    break;

	DXCallOutboard (USERMODULE, dxfd);

    }

    close (dxfd);
    exit (0);
}

