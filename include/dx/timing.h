/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

/* TeX starts here. Do not remove this comment. */

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#ifndef _DXI_TIMING_H_
#define _DXI_TIMING_H_

/*
\section{Timing}
This section describes routines that can be used to measure the
performance of the system.  Calls to {\tt DXMarkTime()} and {\tt
DXMarkTimeLocal()} are made at key points in the system.  Time marks are
batched up until a call to {\tt DXPrintTimes()}.  The printing of timing
messages can be enabled by calling {\tt DXTraceTime()}.

For modules linked into the Data Explorer, the Trace module should
be used to enable recording of times and to print the recorded times.
*/

void DXMarkTime(char *s);
void DXMarkTimeLocal(char *s);
/**
\index{DXMarkTime}\index{DXMarkTimeLocal}
{\tt DXMarkTime()}) records a ``global'' time mark relevant to the
system as a whole.  {\tt DXMarkTimeLocal()} records a ``local'' event
relevant just to one processor, as for example during a parallel
section.
**/

void DXPrintTimes(void);
/**
\index{DXPrintTimes}
Prints information about the ``global'' time events recorded by {\tt
DXMarkTime()} and the ``local'' time events recorded by {\tt
DXMarkTimeLocal()} since the last call to {\tt DXPrintTimes()}.  For each
global event, the following are printed: the identifying tag that was
specified by the {\tt DXMarkTime()} call; the time of the last previous
global event; the delta time between that previous event and this
event; and the time of this event.  For each local event, the
following are printed: the processor on which the event occurred; the
time of the last previous local event on this processor or the last
previous global event, whichever occurred later; the delta time
between that previous event and this event; and the time of this
event.  All times are printed in seconds.  (In addition, on some
architectures the delta user and system times are printed, as recorded
by the operating system.  These times are not printed on the IBM Power
Visualization System, and appear as zeroes.)

In addition to the time markings recorded by the modules, the
executive records the time at the beginning and end of each module,
and at the beginning and end of each parallel section.  Some system
modules also record time events; therefore, when using these routines
to time a module, it is advisable to enable recording of time events
via the Trace module just before execution of the module to be timed,
and to disable them immediately afterwards.
**/

void DXTraceTime(int t);
/**
\index{DXTraceTime}
Enables ({\tt t=1}) or disables ({\tt t=0}) the accumulation of time
marks by {\tt DXMarkTime()} and {\tt DXMarkTimeLocal()} and the printing of
timing messages by {\tt DXPrintTimes()}.
**/

double DXGetTime(void);
/**
\index{DXGetTime}
Returns the elapsed time in seconds since some unspecified reference
point.  The reference point may differ between invocations of the
system, but within one invocation the reference point remains fixed.
**/

void DXWaitTime(double seconds);
/**
\index{DXWaitTime}
Returns after the requested number of seconds has elapsed.
**/

#endif /* _DXI_TIMING_H_ */

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
