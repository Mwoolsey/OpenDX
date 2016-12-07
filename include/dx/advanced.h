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

#ifndef _DXI_ADVANCED_H_
#define _DXI_ADVANCED_H_

/* TeX starts here. Do not remove this comment. */

/*
\chapter{Advanced Routines}
This appendix describes advanced routines that are not of interest
to most module writers.  This includes locking routines, which are
not recommended for general use in modules because of the difficulty
of avoiding deadlocks, and system routines that are only of interest
to the executive or standalone programs.
*/

/*
\section{Locking}
Normally, the routines described in this section are not used by a
module, but rather are used only in the implementation of data model routines.
The reason for this is that great care in the use of locks is required to
avoid deadlock in a parallel system.  They are documented here primarily for
internal use within the data model access routines and in the executive.
Thus, they are intentionally given names in a style different from the
rest of the library.
*/
#if defined(DXD_USE_MUTEX_LOCKS) && DXD_USE_MUTEX_LOCKS==1
#include <synch.h>
typedef volatile mutex_t lock_type;
#elif defined(alphax)
#include <sys/mman.h>
typedef msemaphore lock_type;
#else
typedef volatile int lock_type;
#endif

void DXenable_locks(int enable);
/**
\index{enable\_locks}
Enables or disables locks.  On some systems, issuing and releasing locks
takes a significant amount of time.  This overhead is unnecessary when 
multiple execution threads do not exist or are not sharing state.  By
default, DX internal locking is enabled. Calling this routine with an 
{\tt enable} value of 0 causes mutex locking mechanisms to be bypassed and
all DXlock, DXunlock, and DXtry_lock calls to return success immediately.
**/

int DXcreate_lock(lock_type *l, char *name);
/**
\index{create\_lock}
Creates a lock.  In some systems, the lock variable itself is used as
a lock; in other systems, a handle to a lock is put in the lock
variable.  In either case, you must pass the address {\tt l} of the lock
variable to this routine.  Returns zero for failure, non-zero for
success.
**/

int DXdestroy_lock(lock_type *l);
/**
\index{destroy\_lock}
Destroys the lock pointed to by {\tt l}.  The lock must be unlocked
before this call is made.  Returns zero for failure, non-zero for
success.
**/

int DXlock(lock_type *l, int who);
/**
\index{DXlock}
If the lock pointed to by {\tt l} is currently unlocked, it is
locked by the current thread of execution.  If it is currently locked,
execution blocks until it becomes unlocked, and then it is locked by
the current thread of execution.  The {\tt who} variable may be recorded
in the lock for later matching by {\tt DXunlock()}.  Returns zero for failure,
non-zero for success.
**/

int DXtry_lock(lock_type *l, int who);
/**
\index{try\_lock}
If the lock pointed to by {\tt l} is currently unlocked, it is locked
by the current thread of execution.  If it is currently locked, the
routine returns zero immediately.  The {\tt who} variable may be recorded in
the lock for later matching by {\tt DXunlock()}.  Returns zero for failure,
non-zero for success.
**/

int DXunlock(lock_type *l, int who);
/**
\index{DXunlock}
Unlocks the lock pointed to by {\tt l}.  The {\tt who} parameter may be
matched against the {\tt who} parameter of the call that locked the lock.
(To avoid this issue, you may just uniformly specify {\tt who} of zero in 
all calls.)  Returns zero for failure, non-zero for success.
**/

int DXfetch_and_add(int *p, int value, lock_type *l, int who);
/**
\index{DXfetch_and_add}
On machines which have a hardware fetch and add instruction, this routine
increments the value at location {\tt p} by {\tt value}, and the lock and
who parameters are ignored.  On machines without this instruction, the lock
is used to serialize access to location p before incrementing the old value
at location p by value.  In both cases, the previous value is returned.
**/

/*
\paragraph{Multi-processor support.}
Certain considerations apply when using the memory allocator in a
multi-processor environment.  In particular, forking must be done by
calling {\tt DXmemfork(childno)}. This call is intended for use only by 
the executive, and is of no interest to module writers.
*/
#if defined(DXD_WIN) && defined(small)     /*  ajay defined in RPCNDR.h on NT  as "char" ajay  */
#undef	small
#endif

Error DXSetGlobalSize(int, int, int);
Error DXmemsize(ulong size);
/**
\index{DXSetGlobalSize}\index{DXmemsize}
Sets the maximum total size of the small-block global memory arena, the
threshhold block size between the small and large arenas, and the maximum
total size of the large-block global memory arena, in bytes.
**/

Error DXinitdx(void);
/**
\index{initadx}
Initializes Data Explorer.  In general it is not necessary to make this call
becuase as the system will arrange to make it before anything which
requires initialization is done.  However, in a multi-processor
application it may be necessary to call it to ensure proper
initialization before forking (although using {\tt DXmemfork(childno)}
guarantees initialization before forking.)
**/

Error DXsyncmem(void);
/**
\index{DXsyncmem}
In a multi-process environment, this call may be needed at fork/join
points to re-synchronize the shared arena between processors.  This
is intended for use only in the executive, and is of no interest to
module writers.
**/

int DXmemfork(int childno);
/**
\index{DXmemfork}
This call must be used instead of {\tt fork()} to create a new process.
It has the same return codes as {\tt fork()}.  This call
is intended for use only in the executive, and is of no interest to
module writers.
**/

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#include <stdarg.h>

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif


void DXqmessage(char *who, char *message, va_list args);
void DXsqmessage(char *who, char *message, va_list args);
/**
\index{DXqmessage}\index{DXsqmessage}
These routines assume that the caller has already called va\_start and
will subsequently call va\_end (this is the style used by vsprintf).
They will format the message appropriately as per the above discussion
and then call DXqwrite to queue the message.
**/

void DXqwrite(int fd, char *buf, int length);
void DXqflush(void);
/**
\index{DXqwrite}\index{DXqflush}
A message for the given file descriptor is queued.  All messages will
be flushed when DXqflush() is called.  DXqwrite() may do a DXqflush() and
directly output the message if this message is being produced by the
memory allocator; this is considered an emergency situation.
**/

#endif /* _DXI_ADVANCED_H_ */

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
