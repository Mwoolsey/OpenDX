/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dxmods/keyin.c,v 1.6 2006/01/03 17:02:23 davidt Exp $
 */

#include <dxconfig.h>

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#if defined(HAVE_STRING_H)
#include <string.h>
#endif

/***
MODULE:
 KeyIn 
SHORTDESCRIPTION:
 Stops execution until an Enter keystroke is received.
CATEGORY:
 Debugging
INPUTS:
 string;    String;    Type <ENTER> to continue;  message to print
OUTPUTS:
BUGS:
 this routine only works in completely serial mode (-S).
AUTHOR:
 nancy s collins
END:
***/

#include <dx/dx.h>
#include <fcntl.h>

static char defmessage[] = "Type <ENTER> to continue";


Error
m_KeyIn(Object *in, Object *out)
{
    int fh;
    char c, *s;

    if(in[0]) {
	if(!DXExtractString(in[0], &s)) {
	    DXSetError(ERROR_BAD_PARAMETER, "#10200", "prompt");
	    return ERROR;
	}
    } else
	s = defmessage;

#ifdef DXD_OS_NON_UNIX
    _cprintf("%s", s);
    /*
    {
	char tmpStr[1024];
	 _cgets(tmpStr);
     }
     */
     while(1) {
	c = getche();
	if( c == '\n' || c == 13)
	    break;

     }
     putch('\n');
#else
    fh = open("/dev/tty", 2);
    if (fh < 0) {
	DXSetError(ERROR_DATA_INVALID, "cannot open /dev/tty");
	return ERROR;
    }
    write(fh, s, strlen(s));

    do {
	read(fh, &c, 1);
    } while(c != '\n');

    close(fh);
#endif

    return OK;
}
