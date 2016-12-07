/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>



#include <sys/types.h>
#include <string.h>
#include <dx/dx.h>
#include <stdio.h>

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif
#if defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#endif
#if defined(HAVE_SYS_TIMES_H)
#include <sys/times.h>
#endif
#if defined(HAVE_SYS_PARAM_H)
#include <sys/param.h>
#endif
#if defined(HAVE_TIME_H)
#include <time.h>
#endif


#ifndef HZ
#define HZ 60
#endif

#define EVENTS 2000

static
struct ti {
    int trace;				/* whether to trace times or not */
    struct _proc {
	struct event {			/* per event info: */
	    double user, sys, real;	/* time of this event */
	    char label[16];		/* this event's label */
	    int id;			/* processor on which it ran */
	    int global;			/* whether it is global or local */
	} events[EVENTS];		/* one per event */
	int n;				/* number of events this processor */
	int printed;			/* count for printing */
	struct last {			/* last printed event time */
	    double user, sys, real;	/* user, system, real times */
	} last;				/* per processor */
    } *procs;				/* one per processor */
} *ti = NULL;				/* ptr to struct in global memory */


void
DXTraceTime(int t)
{
    static int been_here;
    if (!been_here) {
	DXinitdx();
	been_here = 1;
    }
    if (ti)
	ti->trace = t;
}


/*
 * Internal routine: get a time split, measured in
 * seconds since the first time split
 */

#if ibmpvs && !paxc
static void
_times_svs(double *user, double *sys, double *real)
{
    static struct tms tms0;
    struct tms tms1;
    static double time0;
    double time1;
    extern double double_time();
    static int fast;

    if (!time0) {
	times(&tms0);
	time0 = double_time();
	fast = getenv("SLOW")? 0 : 1;
    }
    if (!fast && (user || sys))
	times(&tms1);
    time1 = double_time();
    if (user)
	*user = fast? 0 : (float)(tms1.tms_utime-tms0.tms_utime) / HZ;
    if (sys)
	*sys = fast? 0 : (float)(tms1.tms_stime-tms0.tms_stime) / HZ;
    if (real)
	*real = time1-time0;
}
#endif

#if defined(os2)  || defined(intelnt) || defined(WIN32)

static void
_times_unix(double *user, double *sys, double *real)
{
    static clock_t time0  = 0;
    static time_t  rtime0 = 0;
    clock_t time1;
    time_t  rtime1;
    if (!time0)
    {
      time0  = clock();
      rtime0 = time(0);
    }
    time1  = clock();
    rtime1 = time(0);
    if (user)
        *user = (float)(time1) / CLOCKS_PER_SEC;
    if (sys)
        *sys = (float)(time1) / CLOCKS_PER_SEC;
    if (real)
        *real = (float)(rtime1-rtime0);
}

#else

static void
_times_unix(double *user, double *sys, double *real)
{
    static struct tms tms0;
    struct tms tms1;
    static int time0;
    int time1;
    if (!time0)
        time0 = times(&tms0);
    time1 = times(&tms1);
    if (user)
        *user = (float)(tms1.tms_utime/*-tms0.tms_utime*/) / HZ;
    if (sys)
        *sys = (float)(tms1.tms_stime/*-tms0.tms_stime*/) / HZ;
    if (real)
        *real = (float)(time1-time0) / HZ;

}
#endif

#if ibm6000 || sun4 || solaris
static void
_times_ibmsun4(double *user, double *sys, double *real)
{
    static struct tms tms0;
    struct tms tms1;
    struct timeval tp;
    struct timezone tzp;
    static double time0;
    double time1;

    if (!time0) {
	times(&tms0);
        gettimeofday(&tp, &tzp);
        time0 = (double)tp.tv_sec + ((double)(unsigned)tp.tv_usec)*.000001;
    }
    if (user || sys)
	times(&tms1);
    gettimeofday(&tp, &tzp);
    time1 = (double)tp.tv_sec + ((double)(unsigned)tp.tv_usec)*.000001;
    if (user)
	*user = (float)(tms1.tms_utime/*-tms0.tms_utime*/) / HZ;
    if (sys)
	*sys = (float)(tms1.tms_stime/*-tms0.tms_stime*/) / HZ;
    if (real)
	*real = time1 - time0;
}
#endif

static void
_times(double *user, double *sys, double *real)
{
#if !ibm6000 && !sun4 && !solaris
#if ibmpvs && !paxc
    static int been_here, svs;

    if (!been_here) {
	svs = getenv("GENERICI860")? 0 : 1;
	been_here = 1;
    }

    if (svs)
	_times_svs(user, sys, real);
    else
#endif
	_times_unix(user, sys, real);
#else
    _times_ibmsun4(user, sys, real);  
#endif
}


double
DXGetTime(void)
{
    double t;
    _times(NULL, NULL, &t);
    return t;
}

void
DXWaitTime(double seconds)
{
#if defined(intelnt) || defined(WIN32)
    Sleep(seconds*1000);
#else
#if ibmpvs || hp700 || aviion

    double old;
    old = DXGetTime();

    while (DXGetTime() - old < seconds)
	continue;

#elif ibm6000 || sun4 || macos || linux || freebsd || cygwin

    usleep((int)(1000000.0 * seconds));

#elif sgi

    sginap((int)(CLK_TCK * seconds));

#else

    sleep((int)seconds);   /* this will only sleep in whole seconds */

#endif
#endif
}

/*
 * time marking
 */

Error
_dxf_inittiming(void)
{
    if (!ti) {
	ti = (struct ti *) DXAllocateZero(sizeof(struct ti));
	if (!ti)
	    return ERROR;
	ti->procs = (struct _proc *)
	    DXAllocateZero(DXProcessors(0) * sizeof(struct _proc));
	if (!ti->procs) {
	    DXFree((Pointer)ti);
	    return ERROR;
	}
	_times(NULL, NULL, NULL);
    }
    return OK;
}

static
void
mark(char *s, int global)
{
    static int been_here;
    struct _proc *proc;
    struct event *event;
    int id = DXProcessorId();

    if (!been_here) {
	DXinitdx();
	been_here = 1;
    }
    proc = &ti->procs[id];
    if (proc->n >= EVENTS)
	return;
    event = &proc->events[proc->n++];
    _times(&event->user, &event->sys, &event->real);
    event->global = global;
    strncpy(event->label, s, sizeof(event->label)-1);
    event->label[sizeof(event->label)-1] = '\0';
}


void DXMarkTime       (char *s) {if (ti->trace) mark(s, 1);}
void DXMarkTimeLocal  (char *s) {if (ti->trace) mark(s, 0);}
void DXMarkTimeX      (char *s) {               mark(s, 1);}
void DXMarkTimeLocalX (char *s) {               mark(s, 0);}


void
DXPrintTimes(void)
{
    int i, id=0;
    static struct last glob;
    struct _proc *proc = NULL, *p;
    struct event *event;
    struct last *last;
    DXBeginLongMessage();

    for (;;) {

	/* find latest event not yet printed */
	event = NULL;
	for (i=0, p=ti->procs;  i<DXProcessors(0);  i++, p++) {
	    if (p->printed < p->n) {
		struct event *e = &p->events[p->printed];
		if (!event || e->real < event->real)
		    id = i, proc = p, event = e;
	    }
	}
	if (!event)
	    break;
	proc->printed += 1;
	last = &proc->last;

	if (event->global) {
	    /*
	     * Measure global real time delta from last global event.
	     * Measure user and system delta from last event on
	     * this processor.
	     */
	    DXMessage("--: %16s %10.6f+%10.6f=%10.6f (%6.2fu, %6.2fs)\n",
		    event->label, glob.real,event->real-glob.real, event->real,
		    event->user-last->user, event->sys-last->sys);
	    glob.real = event->real;
	} else {
	    /*
	     * Measure real local time delta from last event on this processor
	     * or last global event, whichever is more recent.  Measure user
	     * and sys time delta from last event on this processor.
	     */
	    double real = last->real>glob.real? last->real : glob.real;
	    DXMessage("%2d: %16s %10.6f+%10.6f=%10.6f (%6.2fu, %6.2fs)\n",
		    id, event->label, real, event->real-real, event->real,
		    event->user-last->user, event->sys-last->sys);
	}
	last->user = event->user;
	last->sys = event->sys;
	last->real = event->real;
    }

    DXEndLongMessage();

    /* reset all per-processor information to 0 */
    for (i=0, p=ti->procs; i < DXProcessors(0);  i++, p++)
	p->n = p->printed = 0;
}
