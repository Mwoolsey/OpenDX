/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/


#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#ifndef _DXI_ERROR_H_
#define _DXI_ERROR_H_

/* TeX starts here. Do not remove this comment. */

/*
\section{Error handling}
\label{errorsec}

In general, the routines in the Data Explorer library return either a
pointer, such as an object reference, or an integer error indication.
Success is indicated by returning a non-null pointer or by returning
the non-zero integer constant {\tt OK}.  Failure is indicated by
returning {\tt NULL}, or by returning {\tt ERROR} (which is defined to
be zero).

In case of failure, the library routine may have set the error code by
calling {\tt DXSetError()}, in which case the calling routine generally
should just return {\tt NULL} or {\tt ERROR} to propagate the error
up.  On the other hand, the library routine may not have set the error
code.  In this case it is up to the calling routine to decide whether
the null return indicates an error, in which case the calling routine
should set the error code by calling {\tt DXSetError()}, and then return
{\tt NULL} or {\tt ERROR}; or whether the null return was not an
error, in which case the calling routine should proceed.  This manual
documents for each routine whether it sets the error code when it
returns null.

The error codes are defined as follows: \index{errorcodes}
*/

typedef enum errorcode {
    ERROR_NONE,
    ERROR_INTERNAL,
    ERROR_UNEXPECTED,
    ERROR_ASSERTION,
    ERROR_NOT_IMPLEMENTED,
    ERROR_NO_MEMORY,
    ERROR_BAD_CLASS,
    ERROR_BAD_TYPE,
    ERROR_NO_CAMERA,
    ERROR_MISSING_DATA,
    ERROR_DATA_INVALID,
    ERROR_BAD_PARAMETER,
    ERROR_NO_HARDWARE_RENDERING,
    ERROR_MAX
} ErrorCode;

typedef int Error;
#ifndef ERROR
#define ERROR 0
#endif
#ifndef OK
#define OK 1
#endif

typedef void *Pointer;

#ifndef NULL
#define NULL 0
#endif

Error DXSetError(ErrorCode e, char *message, ...);
#define DXErrorReturn(e,s) {DXSetError(e,s); return ERROR;}
#define DXErrorGoto(e,s) {DXSetError(e,s); goto error;}
#define DXErrorGoto2(e,s1,s2) {DXSetError(e,s1,s2); goto error;}
#define DXErrorGoto3(e,s1,s2,s3) {DXSetError(e,s1,s2,s3); goto error;}


#define ASSERT           _dxdAssert
#define DXASSERT         _dxdAssert
#define DXASSERTGOTO     _dxdAssert
#define DXDATAASSERTGOTO _dxdAssert

#define _dxdAssert(expression)						\
{ if (!(expression)) { 							\
	DXMessage("Internal error detected at \"%s\":%d.\n",		\
        __FILE__, __LINE__);						\
        abort(); } }



/**
\index{DXSetError}\index{DXErrorReturn}\index{DXErrorGoto}\index{ASSERT}
Sets the error code plus an explanatory {\tt message}.  The {\tt
message} may be a {\tt printf()} format string, in which case
additional arguments as required by the format string must be
specified.  This is used internally to the Data Explorer library, and
a breakpoint may be set on this routine to determine more specifically
the cause of the error.  Always returns {\tt ERROR}.  See the Usage
Notes below.
**/

Error DXAddMessage(char *message, ...);
#define DXMessageReturn(s) {DXAddMessage(s); return ERROR;}
#define DXMessageGoto(s) {DXAddMessage(s); goto error;}
/**
\index{DXAddMessage}\index{DXMessageReturn}\index{DXMessageGoto}
Concatenates {\tt message} onto the error message already recorded.
This is used to provide more information about an error detected in a
low level routine.  The {\tt message} may be a {\tt printf()} format
string, in which case additional arguments as required by the format
string must be specified.  Always returns {\tt ERROR}.  See the Usage
Notes below.
**/

ErrorCode DXGetError(void);
/**
\index{DXGetError}
Returns the error code for the last error that occurred, or {\tt ERROR\_NONE}
if no error code has been set.
**/

char *DXGetErrorMessage(void);
/**
\index{DXGetErrorMessage}
Returns the current error message.  This is a pointer to a static buffer
in local memory, so it must be copied if it is to be used outside the
scope of the calling routine.
**/

void DXResetError(void);
/**
\index{DXResetError}
Resets the error state.  This should be used after correcting an error so
that subsequent queries of the error state do not return an incorrect
indication.
**/

void DXWarning(char *message, ...);
/**
\index{DXWarning}
Presents a warning message to the user.  The message string should not
contain newline characters, because the {\tt DXWarning()} routine formats
the message in a manner appropriate to the output medium.  For terminal
output, this includes prefixing the message with the processor identifier 
and appending a newline.  The {\tt message} may be a {\tt printf()} format
string, in which case additional arguments may be needed.
**/

void DXMessage(char *message, ...);
void DXUIMessage(char *who, char *message, ...);
/**
\index{DXMessage}\index{DXUIMessage}
Presents an informative message to the user.  The message string should not
contain newline characters, because the {\tt DXMessage()} routine formats
the message in a manner appropriate to the output medium.  For terminal
output, this includes prefixing the message with the processor identifier 
and appending a newline.  The {\tt message} may be a {\tt printf()} format
string, in which case additional arguments may need to be specified.
**/

void DXBeginLongMessage(void);
void DXEndLongMessage(void);
/**
\index{DXBeginLongMessage}\index{DXEndLongMessage}
The {\tt DXMessage()} routine as documented above is suitable for
relatively short, unformatted messages.  For long, multiple line
messages, you may enclose a series of calls to {\tt DXMessage()} between
{\tt DXBeginLongMessage()} and {\tt DXEndLongMessage()}.  In this case,
newlines are not automatically appended to each message, and it is
your responsibility to indicate appropriate line breaks by including
newline characters in the various {\tt DXMessage()} calls.  For example,
multiple calls to {\tt DXMessage()} may be printed on the same line, or
one call to {\tt DXMessage()} may contain multiple lines.
**/

void DXExpandMessage(int enable);
/**
The DXExpandMessage routine enables and disables the substitution of
messages by number from the messages file.  if the text of a message
starts with #xxx, the text corresponding to that number in the messages
file is substituted for the number.  setting enable to 0 disables this
feature.  it is on by default.
**/

void DXDebug(char *classes, char *message, ...);
void DXEnableDebug(char *classes, int enable);
int DXQueryDebug(char *classes);
/**
\index{DXDebug}\index{DXEnableDebug}\index{DXQueryDebug}
The {\tt DXDebug()} routine generates a debugging message.  The {\tt message}
may be a {\tt printf()} format string, in which case additional arguments
may need to be specified.  The {\tt classes} argument is a pointer to 
a string of characters; each character identifies a debugging class to
which this message belongs.  Each message may belong to more than one
debugging class.  The {\tt DXEnableDebug()} routine enables ({\tt enable=1})
or disables ({\tt enable=0}) one or more classes of debugging messages
specified by the {\tt classes} string.  Normally modules should not
directly use {\tt DXEnableDebug()}, but should allow the user to enable or
disable debugging messages at run time by using the {\tt Trace} module.
{\tt DXQueryDebug} returns 1 if any of the debug classes specified by
the {\tt classes} argument are enabled, 0 if none of them are enabled.
**/

/*
\paragraph{Usage notes.}
Generally, functions signal an error by returning null, in which case
the function should set an error code and an error message.  This may
be done in one of three ways:
\begin{program}
    DXErrorReturn(errorcode, message);
    DXMessageReturn(message);
    return ERROR;
\end{program}
The first method sets the error code and the error message, then
returns null; this should be done by the lowest level routine that
detects the error.  The second method appends its own message to the
message already recorded; this should be done by routines 1) that have
detected an error return from a lower level routine that has already
set the error code, and 2) that have useful information to add to the
message.  The third method is used when an error return is detected
from a lower level routine, and when the current routine has nothing
useful to add to the message.

Alternatively, if cleanup is required before returning, you may use
{\tt DXErrorGoto} or {\tt DXMessageGoto} instead of {\tt DXErrorReturn} or
{\tt DXMessageReturn}. When using {\tt DXErrorGoto} or {\tt DXMessageGoto}, 
you must have an {\tt error:} label, which does cleanup and then returns null.
\begin{program}
    error:
        ... cleanup goes here ...
        return ERROR;
\end{program}
*/


void DXSetErrorExit(int t);
/**
\index{DXSetErrorExit}
Controls the behavior of the program when an error occurs.  If {\tt
t=0}, the error code is set and execution continues. If {\tt t=1},
which is the default, the error code is set, an error message is
printed, and execution continues. If {\tt t=2}, the program prints an
error message and exits.  This should only be used by standalone
programs, not by modules that are included as part of the Data
Explorer application.
**/

void DXErrorExit(char *s);
/**
\index{DXErrorExit}
Prints an error message describing the last error(s) that occurred,
and the calls {\tt exit()} with the error code.  This should only be
used by standalone programs, not by modules that are included as part
of the Data Explorer application.
**/

void DXPrintError(char *s);
/**
\index{NULL}\index{OK}\index{DXPrintError}
Prints an error message describing the last error(s) that occurred.
This should only be used by standalone programs, not by modules that
are included as part of the Data Explorer application.
**/

#endif /* _DXI_ERROR_H_ */

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
