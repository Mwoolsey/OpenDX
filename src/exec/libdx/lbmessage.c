/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#include <dx/dx.h>
#include <fcntl.h>
#include "internals.h"

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(HAVE_SYS_ERRNO_H)
#include <sys/errno.h>
#endif

#if defined(HAVE_ERRNO_H)
#include <errno.h>
#endif

#if defined(HAVE_IO_H)
#include <io.h>
#endif

/*
 * DXMessage system state.
 */

static struct state {		/* message system state */

    int error_exit;		/* 2=exit, 1=print, 0=nothing, on error */
    int trace;			/* enable/disable messages */
    int enabled[256];		/* enabled debug classes */
    
    int nmessages;		/* number of messages */
    int alloc;			/* allocate size of messages array */
    char **messages;		/* the messages */
    char *buf;			/* the buffer for the messages */
    int translate;              /* use the messages file if available */

} *state;


Error
_dxf_initmessages(void)
{
    char *file, *s, name[100];
    int fd, mno, size;

    if (state)
	return OK;

#if pgcc
    atexit(DXqflush);
#endif

    /* initialize state vector */
    state = (struct state *) DXAllocateZero(sizeof(struct state));
    if (!state)
	return ERROR;
    state->error_exit = 1;
    state->trace = 1;
    state->translate = 1;
#ifdef DEBUGGED	/* Turn this off for production code, it confuses users */
    DXEnableDebug("M", 1);
#endif
    if (!_dxf_initmemqueue())
	return ERROR;

    /* read and parse message file */
    file = getenv("DXMESSAGES");
    if (file)
	sprintf(name, "%s", file);
    else {
	char *root = getenv("DXEXECROOT");
	if (!root) root = getenv("DXROOT");
	if (!root) root = "/usr/local/dx";
	sprintf(name, "%s/lib/messages", root);
    }
    fd = open(name, O_RDONLY);
    if (fd<0) {
	DXWarning("message file %s could not be opened", name);
	state->translate = 0;
	return OK;
    }
    size = lseek(fd, 0, 2);
    lseek(fd, 0, 0);
    state->buf = DXAllocate(size+1);
    if (!state->buf)
	return ERROR;
    if (read(fd, state->buf, size) < size) {
	DXWarning("message file %s could not be read", name);
	state->translate = 0;
	close(fd);
	return OK;
    }
    close(fd);
    state->buf[size] = '\0';
    s = state->buf;
    while (*s!='#' && *s)
	s++;
    while (*s) {
	*s = '\0';
	s++;
	mno = atoi(s);
	if (mno >= state->alloc) {
	    int n = 2*mno + 1;
	    char **m = (char **) DXReAllocate((Pointer)state->messages,
					   n * sizeof(char*));
	    if (!m)
		return ERROR;
	    state->messages = m;
	    state->alloc = n;
	}
	while ('0'<=*s && *s<='9')
	    s++;
	while (*s==' ' || *s=='\t' || *s=='\n')
	    s++;
	state->messages[mno] = s;
	if (mno > state->nmessages)
	    state->nmessages = mno;
	while (*s!='#' && *s) {
	    if (*s=='\n')
		*s = ' ';
	    s++;
	}
    }
    return OK;
}

static char *
translate(char *s, int *messfile)
{
    int i;
    if (!state || !s || *s!='#' || !state->translate) {
	*messfile = 0;
	return s;
    }
    i = atoi(s+1);
    if (0<=i && i<=state->nmessages && state->messages[i]) {
	*messfile = 1;
	return state->messages[i];
    } else {
	*messfile = 0;
	return s;
    }
}


static void
aqmessage(char *who, char *message, ...)
{
    va_list arg;
    va_start(arg,message);
    DXqmessage(who, message, arg);
    va_end(arg);
}



/*
 * Error handling.
 */

static ErrorCode _ErrorCode = ERROR_NONE;
static char _ErrorMessage[2000] = "";

void
DXSetErrorExit(int t)
{
    if (!state && !DXinitdx())
	return;
    state->error_exit = t;
}

int DXGetErrorExit()
{
    if(state)
        return(state->error_exit);
    else return(0);
}

/* mark the messages which don't come from the messages file, so we
 * can eventually get them all out of the code.  debug version only.
 */
#define TAG "(!file) "
#define TAGLEN 8

Error
DXSetError(ErrorCode code, char *message, ...)
{
    va_list arg;
    int usedmessfile;
    int offset = 0;

    message = translate(message, &usedmessfile);

    /* error code */
    _ErrorCode = code;

    /* message */
#if DEBUGGED
    if (!usedmessfile) {
        strcpy(_ErrorMessage, TAG);
	offset = TAGLEN;
    }
#endif
    va_start(arg,message);
    vsprintf(_ErrorMessage+offset, message, arg);
    va_end(arg);

    /* action */
    if (!state || state->error_exit == 1)
	DXPrintError(NULL);
    else if (state->error_exit >= 2)
	DXErrorExit(NULL);

    return ERROR;
}


Error
DXAddMessage(char *message, ...)
{
    char buf[2000];
    va_list arg;
    int usedmessfile;

    message = translate(message, &usedmessfile);

    /* message */
    va_start(arg,message);
    vsprintf(buf, message, arg);
    va_end(arg);
    strcat(_ErrorMessage, " / ");
    strcat(_ErrorMessage, buf);

    /* action */
    if (!state || state->error_exit)
	DXMessage("    (%s)", buf);

    return ERROR;
}


ErrorCode
DXGetError(void)
{
    return _ErrorCode;
}


char *
DXGetErrorMessage(void)
{
    return _ErrorMessage;
}


void
DXResetError(void)
{
#if !defined(HAVE__ERRNO)
    errno = 0;
#endif
    _ErrorCode = ERROR_NONE;
    _ErrorMessage[0] = 0;
}


void
DXPrintError(char *s)
{
    char *msg, *errnomsg;
    static int been_here = 0;
    static char *messages[(int)ERROR_MAX];

    if (!been_here) {
	messages[(int)ERROR_NONE] = "Error code not set";
	messages[(int)ERROR_INTERNAL] = "Internal error";
	messages[(int)ERROR_UNEXPECTED] = "Unexpected error";
	messages[(int)ERROR_ASSERTION] = "Assertion failed";
	messages[(int)ERROR_NOT_IMPLEMENTED] = "Operation not implemented";
	messages[(int)ERROR_NO_MEMORY] = "Out of memory";
	messages[(int)ERROR_BAD_CLASS] = "Bad class";
	messages[(int)ERROR_BAD_TYPE] = "Bad type";
	messages[(int)ERROR_NO_CAMERA] = "No camera";
	messages[(int)ERROR_MISSING_DATA] = "Missing data";
	messages[(int)ERROR_DATA_INVALID] = "Invalid data";
	messages[(int)ERROR_BAD_PARAMETER] = "Bad parameter";
	been_here = 1;
    }

    if ((int)_ErrorCode<0
	|| (int)_ErrorCode>=ERROR_MAX
	|| !messages[(int)_ErrorCode])
	msg = "Bad error code";
    else
	msg = messages[(int)_ErrorCode];

    /* print error message */
    if (_ErrorCode==ERROR_NONE && errno) {
    	errnomsg = (char *) strerror(errno); /* sys_errlist[errno]; */
	if (s)
	    aqmessage("ERROR", "%s: %s", s, errnomsg);
	else
	    aqmessage("ERROR", "%s", errnomsg);
    } else {
	if (s)
	    aqmessage("ERROR", "%s: %s: %s", s, msg, _ErrorMessage);
	else
	    aqmessage("ERROR", "%s: %s", msg, _ErrorMessage);
    }
}

void
DXErrorExit(char *s)
{
    DXPrintError(s);
    exit(DXGetError());
}


/*
 * DXMessage, DXWarning, DXUIMessage
 */
#if 0
void
_dxfTraceMessage(int t)
{
    if (!state && !DXinitdx())
	return;
    state->trace = t;
}
#endif

#define LARGE 2000
#define SAFE 1000
static char long_buf[LARGE];
static int long_n = 0;
static int long_message = 0;


void
DXBeginLongMessage(void)
{
    long_message = 1;
}


void
DXEndLongMessage(void)
{
    if (long_n > 0)
	aqmessage("*", "%s", long_buf);
    long_n = 0;
    long_message = 0;
}


void
DXMessage(char *message, ...)
{
    char *p;
    va_list arg;
    int usedmessfile;

    if (state && !state->trace)
	return;
    message = translate(message, &usedmessfile);

    if (long_message) {
	va_start(arg,message);
	vsprintf(long_buf+long_n, message, arg);
	va_end(arg);
	for (p=long_buf+long_n; *p; ) {
	    if (*p=='\n') {
		*p = '\0';
		aqmessage("*", "%s", long_buf);
		strcpy(long_buf, p+1);
		p = long_buf;
	    }
	    else
		p++;
	}	
	long_n = p-long_buf;
	if (long_n > SAFE) {
	    *p = '\0';
	    aqmessage("*", "%s", long_buf);
	    long_n = 0;
	}
    } else {
	va_start(arg,message);
	DXqmessage(NULL, message, arg);
	va_end(arg);
    }
}


void
DXWarning(char *message, ...)
{
    va_list arg;
    int usedmessfile;

    message = translate(message, &usedmessfile);
    va_start(arg,message);
    DXqmessage("WARNING", message, arg);
    va_end(arg);
}


void
DXUIMessage(char *who, char *message, ...)
{
    va_list arg;
    int usedmessfile;

    message = translate(message, &usedmessfile);
    if (state && !state->trace)
	return;
    va_start(arg,message);
    DXqmessage(who, message, arg);
    va_end(arg);
}


void 
DXExpandMessage(int enable)
{
    state->translate = enable;
}

/*
 * DXDebug mechanism
 */

void
DXEnableDebug(char *classes, int enable)
{
    int i;
    if (!state && !DXinitdx())
	return;
    if (!classes)
	for (i=0; i<256; i++)
	    state->enabled[i] = enable;
    else
	for (i=0; classes[i]; i++)
	    state->enabled[(unsigned char)classes[i]] = enable;
}


void
DXDebug(char *classes, char *message, ...)
{
    int i;
    int usedmessfile;

    if (!state && !DXinitdx())
	return;
    message = translate(message, &usedmessfile);
    for (i=0; classes[i]; i++) {
	if (!state || state->enabled[(unsigned char)classes[i]]) {
	    va_list arg;
	    va_start(arg,message);
	    DXqmessage(NULL, message, arg);
	    va_end(arg);
	    return;
	}
    }
}

int
DXQueryDebug(char *classes)
{
    int c, i;
    if (!state && !DXinitdx())
	return 0;
    for (i=0; (c=classes[i])!=0; i++)
	if (state->enabled[c])
	    return 1;
    return 0;
}
