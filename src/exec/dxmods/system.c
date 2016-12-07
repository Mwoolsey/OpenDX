/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dxmods/system.c,v 1.6 2006/01/03 17:02:25 davidt Exp $
 */

#include <dxconfig.h>

#if defined(HAVE_STRING_H)
#include <string.h>
#endif

#include <dx/dx.h>

int
m_System(Object *in, Object *out)
{
    static char format[] = "/bin/sh -c \"%s\" </dev/null";
    char *cmd;
#if defined(intelnt) || defined(WIN32)
#define MAX_CMD_ARGS 30
    char *p[MAX_CMD_ARGS];
    char *s;
    int i, j;
#endif

    if (DXExtractString(in[0], &cmd)) {
	char *buf = (char *)DXAllocate(sizeof(format) + strlen(cmd) + 1);
#if defined(intelnt) || defined(WIN32)
	char *buf2 = (char *)DXAllocate(sizeof(format) + strlen(cmd) + 1);
	int quote = 0;
	strcpy(buf, cmd);
	s = buf;
	for (i = 0; *s && (i < MAX_CMD_ARGS - 1); s++, i++) {
        p[i] = s;
	    if(*s == '"') { 
	    	quote = 1; 
	    	s++;
	    }
	    p[i+1] = NULL;
		if (quote == 1) {
			for ( ; *s && *s != '"'; s++);
			quote = 0;
		}
		else
	    	for ( ; *s && isspace(*s) && *s != '"'; s++);
	    for ( ; *s && !isspace(*s); s++);
	    if (*s)
		*s = '\0';
	      else
	        break;
	}		
	for (s = *p; s && *s; s++)
	    if (*s == '/')
			*s = '\\';

	strcpy(buf2, p[0]);
	s = buf2;
	if(*s == '"') {
	    s++;
	    buf2[strlen(buf2)-1] = '\0';
	}
	
	DXDebug("S", "cmd: %s", s);
	
	for(j = 0; j <= i; j++)
		DXDebug("S", "p[%d]: %s", j, p[j]);

	spawnvp(_P_WAIT, s, p);
#else
	sprintf(buf, format, cmd);
	system(buf);
#endif
	DXFree(buf);
    } else
	DXErrorReturn(ERROR_BAD_PARAMETER, "bad command string");

    return OK;
}
