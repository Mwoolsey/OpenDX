/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include <dx/dx.h>

#if defined(DDX)
#if defined(DXD_EXEC_WAIT_PROCESS)
#undef DXD_EXEC_WAIT_PROCESS
#endif
#define DXD_EXEC_WAIT_PROCESS   0
#define DXD_MASTER_IS_P0	1
#endif

#if defined(HAVE_WINIOCTL_H)
#include <winioctl.h>
#endif
#if defined(HAVE_SYS_IOCTL_H)
#include <sys/ioctl.h>
#endif
#if defined(HAVE_SYS_TYPES_H)
#include <sys/types.h>
#endif
#if defined(HAVE_SYS_TIMES_H)
#include <sys/times.h>
#endif
#if defined(HAVE_SYS_PARAM_H)
#include <sys/param.h>
#endif
#if defined(HAVE_SYS_TIMEB_H)
#include <sys/timeb.h>
#endif
#if defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#endif
#if defined(HAVE_TIME_H)
#include <time.h>
#endif
#if defined(HAVE_SYS_SIGNAL_H)
#include <sys/signal.h>
#endif
#if defined(HAVE_CTYPE_H)
#include <ctype.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif
#if defined(HAVE_CONIO_H)
#include <conio.h>
#endif
#if defined(HAVE_ERRNO_H)
#include <errno.h>
#endif
#if defined(HAVE_NETDB_H)
#include <netdb.h>
#endif
#if defined(HAVE_SYS_FILIO_H)
#include <sys/filio.h>
#endif
#if defined(HAVE_SYS_WAIT_H)
#include <sys/wait.h>
#endif
#if defined(HAVE_LIMITS_H)
#include <limits.h>
#endif
#if defined(HAVE_FCNTL_H)
#include <fcntl.h>
#endif

#if defined(HAVE_SYS_RESOURCES_H)
#include <sys/resource.h>
#endif

#if DXD_HAS_LIBIOP

#ifndef WSTOPSIG
#define WSTOPSIG(status)	((status).w_stopsig)
#define WTERMSIG(status)	((status).w_termsig)
#define WEXITSTATUS(status)	((status).w_retcode)
#endif

#endif

#if defined(HAVE_SYS_SELECT_H)
#include <sys/select.h>
#endif

#if defined(HAVE_SYS_SYSMP_H)
#include <sys/sysmp.h>
#endif
#if defined(HAVE_SYS_SYSTEMCFG_H)
# include <sys/systemcfg.h>
#endif

#if DXD_HAS_LIBIOP
#include <sys/svs.h>
#endif

/* On solaris exit seems to not work properly mp, the parent hangs around and never exits. */
#if solaris
#define exit(x) _exit(x)
#endif

#define	READ_I_THRESHHOLD	0.05		/* how often to ioctl  fd */
/*#define	READ_S_THRESHHOLD	5.0*/	/* how often to select fd */
#define MAIN_TIMING		1

#define	CHECK_INIT(_i, what)\
    if ((_i) != OK) ExInitFailed (what)

#include "dxmain.h"
#include "config.h"
#include "background.h"
#include "parse.h"
#include "d.h"
#include "graph.h"
#include "rq.h"
#include "graphqueue.h"
#include "status.h"
#include "log.h"
#include "packet.h"
#include "exobject.h"
#include "sysvars.h"
#include "version.h"
#include "vcr.h"
#include "swap.h"
#include "_macro.h"
#include "_variable.h"
#include "parsemdf.h"
#include "command.h"
#include "pendingcmds.h"
#include "userinter.h"
#include "nodeb.h"
#include "lex.h"
#include "evalgraph.h"
#include "function.h"
#include "rih.h"
#include "task.h"
#include "remote.h"
#include "socket.h"
#include "../libdx/diskio.h"
#include "instrument.h"

#ifdef DXD_LICENSED_VERSION
#include "license.h"
static int exLicenseSelf = FALSE;
lictype _dxd_exForcedLicense    = NOLIC; /* Is the given license forced */
int		_dxd_ExHasLicense	= FALSE;
#endif

#ifndef OPEN_MAX
#define OPEN_MAX  	sysconf(_SC_OPEN_MAX)
#endif

static int extestport = -1;
static char extesthost[80];

/* Functions defined in libdx or programatically defined */

extern int   DXConnectToServer(char *host, int pport); /* from libdx/client.c */
extern void  _dxfTraceObjects(int d); /* from libdx/object.c */
extern void  _dxf_user_modules(); /* from libdx/ */
extern void  _dxf_private_modules(); /* from libdx/ */
extern Error user_cleanup(); /* from libdx/userinit.c */

extern void _dxfcleanup_mem(); /* from libdx/mem.c */
extern int _dxf_GetPhysicalProcs(); /* from libdx/memory.c */
extern Error ExHostToFQDN( const char host[], char fqdn[MAXHOSTNAMELEN] );
/* from remote.c */
extern Error user_slave_cleanup(); /* from userinit.c */

/*
 * How often to check for registered input handlers
 */

#define	EX_RIH_FREQUENCY	10.0
#define	EX_RIH_INTERVAL		1.0 / EX_RIH_FREQUENCY


static int	exParent = FALSE;
static int      exParent_RQread_fd;
static int      exParent_RQwrite_fd;
static int	exChParent_RQread_fd;

static double	read_i_threshhold = READ_I_THRESHHOLD;
/*static double	read_s_threshhold = READ_S_THRESHHOLD;*/

static int maxMemory	= 0;	/* in MB -- 0 implies library default */

static int nprocs;
static int nphysprocs;

static int processor_status_on	= FALSE;

PATHTAG            _dxd_pathtags = {0, 0, NULL};
DPGRAPHSTAT        _dxd_dpgraphstat = {0, 0, NULL};
DPHOSTS            *_dxd_dphosts = NULL;
#if defined(DXD_USE_MUTEX_LOCKS) && DXD_USE_MUTEX_LOCKS==1
lock_type          _dxd_dphostslock;
#else
lock_type          _dxd_dphostslock = 0;
#endif
PGASSIGN           _dxd_pgassign = {0, 0, NULL};
;
SLAVEPEERS         _dxd_slavepeers = {0, 0, NULL};
;

int _dxd_exCacheOn 		  = TRUE;		/* use cache */
int _dxd_exIntraGraphSerialize    = TRUE;
int _dxd_exDebug		  = FALSE;
int _dxd_exGoneMP		  = FALSE;   /* set true after forking */
int _dxd_exRemote		  = FALSE;
int _dxd_exRemoteSlave            = FALSE;
int _dxd_exRunningSingleProcess   = TRUE;    /* set with nphysprocs/nprocs */
int _dxd_exShowTiming		  = FALSE;
int _dxd_exShowTrace		  = FALSE;
int _dxd_exShowBells		  = FALSE;
int _dxd_exSkipExecution	  = FALSE;
int _dxd_exRshInput		  = FALSE;
int _dxd_exIsatty		  = FALSE;

SFILE           *_dxd_exSockFD	  = NULL;
static SFILE	*_pIfd		  = NULL;

static int logcache		  = FALSE;
static int logerr                 = 0;
int _dxd_exDebugConnect           = FALSE;
int _dxd_exRemoteUIMsg		  = FALSE;

char *_dxd_exHostName = NULL;
int  _dxd_exPPID = 0;		 /* parent's process id */
int *_dxd_exTerminating = NULL;	 /* flag set when dx is exiting */
int _dxd_exSelectReturnedInput = FALSE;  /* flag set when select returned from yyin */
Context *_dxd_exContext = NULL;         /* structure for context information */
int  _dxd_exMyPID = 0;		 /* pid of the process */
int  _dxd_exMasterfd = -1;       /* slave to master file descriptor */
int  _dxd_exSlaveId = 0;             /* slave number */
int  _dxd_exSwapMsg = 0;             /* do we need to swap msg from peer? */
/* startup of slave as finished, send msgs OK */
int  *_dxd_exNSlaves = NULL;		 /* number of distributed slaves */
int  *_dxd_extoplevelloop = NULL;	 /* looping at top level of graph */

int            _dxd_exErrorPrintLevel = 3;

int _dxd_exEnableDebug	        = 0;
long _dxd_exMarkMask		= 0;	      /* DXMarkTime enable mask	*/
static int MarkModules		= FALSE;

int _dxd_exParseAhead	        = TRUE;
int _dxd_exSParseAhead = 0;

static char	*_pIstr	= "stdin";

static char	**exenvp	= NULL;

#define MDF_MAX  20
static char     *mdffiles[MDF_MAX];
static int      nmdfs           = 0;


/* Common routines added for distributed processing */

/*
 * All the main helper functions.
 */

/*
 * This one's used externally in DODX RunOnSlaves 
 */
int  ExCheckInput(void);
void ExQuit(void);
void _dxf_ExPromptSet(char *var, char *val);

static int	ExCheckGraphQueue	(int);
static int	ExCheckRunqueue		(int graphId);
static void	ExCheckTerminate	(void);
static void	ExChildProcess		(void);
static void	ExCleanup		(void);
static void	ExConnectInput		(void);
static void ExCopyright 		(int);
static int	ExFindPID		(int pid);
static void	ExForkChildren		(void);
static void	ExInitialize		(void);
static void	ExInitFailed		(char *message);
static void	ExMainLoop		(void);
static void	ExMainLoopMaster	(void);
static void	ExMainLoopSlave		(void);
static void	ExParallelMaster	(void);
static void	ExProcessArgs		(int argc, char **argv);
static void	ExSettings		(void);
static void	ExUsage			(char *name);
static void	ExVersion		(void);
static int  ExFromMasterInputHndlr  (int fd, Pointer p);

#if !defined(intelnt) && !defined(WIN32)
extern int   DXForkChild(int);
#endif

#if DXD_EXEC_WAIT_PROCESS
static void	ExParentProcess		(void);
#endif

#if HAVE_SIGQUIT
static void	ExSigQuit(int);
#endif
#if HAVE_SIGPIPE
static void	ExSigPipe(int);
#endif
#if HAVE_SIGDANGER
static void	ExSigDanger		(int);
#endif
#if DXD_EXEC_WAIT_PROCESS
static void	ExKillChildren(void);
#endif

EXDictionary	_dxd_exGlobalDict = NULL;

static struct child
{
    int pid;
    int RQwrite_fd;
}
*children;

lock_type *childtbl_lock = NULL;

static volatile int *exReady;

int DXmain (int argc, char **argv, char **envp)
{
    int		save_errorlevel=0;
#if HAVE_SYS_CORE_H

    struct rlimit	rl;
#endif

    exenvp = envp;

#if HAVE_SYS_CORE_H

    getrlimit (RLIMIT_CORE, &rl);
    rl.rlim_cur = 0;
    setrlimit (RLIMIT_CORE, &rl);
#endif

	nphysprocs = _dxf_GetPhysicalProcs();

	if(nphysprocs > 3)
        nprocs = (int)(nphysprocs / 2);
    else
        nprocs = (nphysprocs > 1) ? 2 : 1;

#if HAVE_SIGDANGER
    signal (SIGDANGER, ExSigDanger);
#endif

#if HAVE_SIGPIPE
    signal(SIGPIPE, ExSigPipe);
#endif

#if HAVE_SIGQUIT
    signal(SIGQUIT, ExSigQuit);
#endif

    ExProcessArgs (argc, argv);

    /* boolean: if UP machine, or user asked for -p1 on MP machine */
    _dxd_exRunningSingleProcess = (nphysprocs == 1 || nprocs == 1);

    /* if running single-process, we don't need the overhead of locking */
    DXenable_locks( !_dxd_exRunningSingleProcess );

    /* we will turn off parse ahead when we get a sync */
    /* so save original state */
    _dxd_exSParseAhead = _dxd_exParseAhead;
    if (_dxd_exRemoteSlave) {
        /* turn off messages during initialization */
        save_errorlevel = _dxd_exErrorPrintLevel;
        _dxd_exErrorPrintLevel = -1;
    } else
        ExCopyright (! _dxd_exRemote);

    ExInitialize ();		       /* perform all shared initializations */

#ifdef DXD_LICENSED_VERSION

    if(!_dxd_exRemoteSlave)
        ExLicenseFinish();    /* finish license stuff with libDX initialized */

#endif /* DXD_LICENSED_VERSION */

    if(_dxd_exRemoteSlave) {
        /* turn messages back on */
        _dxd_exErrorPrintLevel = save_errorlevel;
    }

#ifdef DXD_LICENSED_VERSION

    if (!_dxd_exRemote || exLicenseSelf) {
        if (_dxd_exRemoteSlave)
            _dxd_ExHasLicense = TRUE;
        else if (!ExGetPrimaryLicense()) {
            DXMessage ("Could not get a license\n");
            exit (-1);
        }
    }
#endif /* DXD_LICENSED_VERSION */

    if(logerr > 0)
        _dxf_ExLogError(logerr);

    ExForkChildren ();			/* create subprocesses */

#if DXD_EXEC_WAIT_PROCESS
    /*
     * This is the parent waiter process.  It waits for children to die
     * and either kills the others off if there was an error, or exits
     * gracefully.
     */
    if ((nprocs > 1 && _dxd_exPPID == getpid ()) || _dxd_exMyPID == -1)
        ExParentProcess ();

    if (nprocs == 1 || _dxd_exPPID != getpid ())
        ExChildProcess ();
#else

    ExChildProcess ();
#endif


    return (0);
}


char **_dxf_ExEnvp (void)
{
    return (exenvp);
}

#if DXD_EXEC_WAIT_PROCESS
static void ExParentProcess ()
{
    int 		pid;
    int			wstatus;
    int			fpid;

    exParent = TRUE;
    *exReady = TRUE;

    /*
     * Stop here to add child processes for debugging!
     */

wait_on_child:
    /* wait for a child to die */
    while ((pid  = wait (&wstatus)) < 0)
        if (errno != EINTR)
            break;

#ifdef DXD_LICENSED_VERSION
    /* Getting a license causes one or two more children to be created.
     * on an MP system we will get two licenses and have 2 dxshadows running 
     * for nodelock licenses dxshadow exits immediately. This caused the 
     * exec to think a child had terminated. We need to see if this child
     * was a dxshadow process and if it was a nodelock license then it's
     * OK and we should go back to waiting on our children. In the case that
     * dxshadow died for a concurrent license we will print out an error
     * message and terminate. */
    fpid = _dxfCheckLicenseChild(pid);
    if(fpid == 0) /* we had a node locked license, continue to wait on child */
        goto wait_on_child;
    if(fpid == -1)
#endif

        fpid = ExFindPID (pid);
    /* process not in child table so use pid in error messages */
    if(fpid < 0)
        fpid = pid;

    /* child died, now figure out why */
    if (WIFSTOPPED (wstatus)) {
        printf ("child process %d (%d) stopped; stop signal = %d\n",
                fpid, pid, WSTOPSIG (wstatus));
    } else if (WIFSIGNALED (wstatus)) {
        if (WTERMSIG (wstatus) != 9)
            printf ("child process %d (%d) killed by signal = %d\n",
                    fpid, pid, WTERMSIG (wstatus));
    } else if (WIFEXITED (wstatus)) {
        printf ("child process %d (%d) exited, status = %d\n",
                fpid, pid, WEXITSTATUS (wstatus));
    } else {
        printf ("child process %d (%d) broke wait\n", fpid, pid);
    }

    ExCleanup ();

    printf ("\nparent exiting\n");
    fflush (stdout);

    exit (0);
}
#endif


static void ExChildProcess ()
{
    /*
     * Wait for all the children to appear and the parent to signal OK to
     * start processing.
     */
    _dxf_ExInitTaskPerProc();

    /* don't send out worker messages for slaves */
    if(!_dxd_exRemoteSlave)
        DXMessage ("#1060", getpid ());

    while ((nprocs > 1) && (! *exReady))
        ;

    ExMainLoop ();
}



static void ExMainLoop ()
{
    if (_dxd_exMyPID == 0 || nprocs == 1)
        ExMainLoopMaster ();
    else
        ExMainLoopSlave ();
}


static void ExMainLoopSlave ()
{
    int			ret = TRUE;
    int 		RQ_fd;
    char		c;

    set_status (PS_EXECUTIVE);

    RQ_fd = exParent_RQread_fd;

    while (! *_dxd_exTerminating) {
        ret = _dxf_ExRQPending () && ExCheckRunqueue (0);
        if(! ret) {
            if (_dxd_exMyPID == 1) {
                if(_dxf_ExIsExecuting())
                    _dxf_ExCheckRIH ();
                else
                    _dxf_ExCheckRIHBlock (-1);
            } else
                read(RQ_fd, &c, 1);
        }
    }
    user_slave_cleanup();
}


/*
 * ExMainLoopMaster -- This is the main loop executed by the master processor.
 * The loop is different depending upon if this is the only processor (nprocs
 * == 1) or if it is one of several, and thus the chief delegator.  In the 
 * multiprocessor case, the principal task of the master is to make modules
 * available to run (put them in the run queue) as quickly as possible, and
 * only if there is nothing else for it to do, to try to run something.  
 * The function is different in the uniprocessor case, where this is the only
 * loop being run.  In this case, one tries to empty the run queue, then the
 * module queue, then the graph queue, ....
 *
 * Note that all things that are only needed by the master should be inited
 * here so that we don't waste the slaves' local memory.
 */

#define	MAIN_LOOP_CONTINUE	{naptime = 0; goto main_loop_continue;}

#if TIMEOUT_DX
/* right now this only works for sgi, it's not being used now but it should
   be changed to run on all machines if you want to do a timeout            */
/* if idle for 30 minutes, kill it */
#define NAPTIME		(CLK_TCK >> 2)
#define NAPDEAD		(30 * 60 * CLK_TCK)
#endif

static void ExMainLoopMaster ()
{
#if defined(DX_NATIVE_WINDOWS)
    MSG msg;
#endif
    /*
     * Module definitions should be put into the dictionary before
     * macro definitions which may occur in _dxf_ExParseInit (.dxrc files).
     */

    _dxf_user_modules ();
    _dxf_private_modules ();
    _dxf_ExInitRemote ();
    _dxf_ExFunctionDone ();
    CHECK_INIT (_dxf_ExParseInit (_pIstr, _pIfd), "reading .dxrc init file");

#if sgi

    if (nprocs > 1)
        sleep (1);
#endif

    set_status (PS_EXECUTIVE);

    if(_dxd_exRemoteSlave) {
        int fd;
        if(extestport < 0)
            _dxf_ExDie("No port specified for slave to connect to");
        fd = DXConnectToServer(extesthost, extestport);
        if(fd < 0)
            _dxf_ExDie("couldn't connect to socket %d\n", extestport);
        printf("connected to %s:%4d\n", extesthost, extestport);
        _dxd_exMasterfd = fd;
        DXRegisterInputHandler(ExFromMasterInputHndlr, fd, NULL);
        for (;;) {
loop_slave_continue:
            ExCheckTerminate ();
            if (_dxf_ExRQPending () && ExCheckRunqueue (0)) {
                /* always check rih so socket doesn't get blocked */
                _dxf_ExCheckRIH ();
                goto loop_slave_continue;
            }
            if(nprocs == 1)
                _dxf_ExCheckRIHBlock (_dxd_exMasterfd);
            else {
                if(_dxf_ExCheckRIH ())
                    goto loop_slave_continue;
#if sgi

                else {
                    set_status (PS_NAPPING);
                    sginap (0);
                    set_status (PS_EXECUTIVE);
                }
#endif

            }
        }
    } else
        _dxd_exSlaveId = 0;

    while (1)  /*  naptime  */
    {

        ExMarkTimeLocal (4, "main:top");
        DXqflush ();

        IFINSTRUMENT (++exInstrument[_dxd_exMyPID].numMasterTry);

        /*
         * have we achieved termination condition
         */
        ExCheckTerminate ();

        if (nprocs > 1)
            ExParallelMaster ();
        else
        {
            _dxf_ExCheckRIH ();
            ExMarkTimeLocal (4, "main:chrq");
            if (_dxf_ExRQPending () && ExCheckRunqueue (0)) {
                if (_dxd_exParseAhead)
                    ExCheckInput ();
                continue;
            }

            _dxf_RunPendingCmds();

            if (_dxd_exParseAhead) {
                ExMarkTimeLocal (4, "main:chin");
                if (ExCheckInput ())
                    continue;
            }

            ExMarkTimeLocal (4, "main:chgq");
            if (ExCheckGraphQueue (-1))
                continue;

            if (! _dxd_exParseAhead) {
                /* if we get here there is nothing in the queues */
                /* restore parse ahead in case it was changed */
                _dxd_exParseAhead = _dxd_exSParseAhead;
                ExMarkTimeLocal (4, "main:chin");
                if (ExCheckInput ())
                    continue;
            }

            ExMarkTimeLocal (4, "main:chbg");
            if (_dxf_ExCheckBackground (_dxd_exGlobalDict, FALSE))
                continue;

            ExMarkTimeLocal (4, "main:chVCR");
            if (_dxf_ExCheckVCR (_dxd_exGlobalDict, FALSE))
                continue;

#if defined(intelnt) || defined(WIN32)
            SleepEx(100, TRUE);
#endif
#if defined(DX_NATIVE_WINDOWS)

            while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
#endif
#if defined(DDX)
            {
                extern Error SlaveBcastLoop(int, Pointer);
                extern int GetMPINodeId();
                if (GetMPINodeId() == 0)
                    _dxf_ExCheckRIHBlock (SFILEfileno (yyin));
                else {
                    SlaveBcastLoop(0, NULL);
                    _dxf_ExCheckRIH ();
                }
            }
#else
#ifndef DXD_NOBLOCK_SELECT
            ExMarkTimeLocal (4, "main:chRIH");
            _dxf_ExCheckRIHBlock (SFILEfileno (yyin));
#endif
#endif

        }

        /* main_loop_continue: */
        continue;
    }

    _dxf_CleanupPendingCmdList();
}

#if DXD_EXEC_WAIT_PROCESS
/*
 * Let all of the remaining children know they are to die.
 */
static void ExKillChildren ()
{
    int		i;

    if (! (_dxd_exMyPID == 0 && nprocs == 1))
        for (i = 0; i < nprocs && i < nphysprocs; i++)
            kill (children[i].pid, SIGKILL);

#if DXD_PROCESSOR_STATUS

    if (_dxd_exStatusPID)
        kill (_dxd_exStatusPID, SIGKILL);
#endif
}
#endif


#define EMESS "Error during Initialization\n"

static void ExInitFailed (char *message)
{
    char *cp;

    /*
     * print a message before exiting saying why we can't start up.
     */

    write(fileno (stdout), EMESS, strlen(EMESS));

    cp = DXGetErrorMessage();
    if (cp)
        write (fileno (stdout), cp, strlen (cp));
    write(fileno (stdout), "\n", 1);

    if (message)
        write(fileno (stdout), message, strlen (message));
    write(fileno (stdout), "\n", 1);

    exit (0);
}


void _dxf_ExDie (char *format, ...)
{
    char buffer[1024];
    va_list arg;

    /* don't add a space before format in the next line or it won't
     * compile with the metaware compiler.
     */
    va_start(arg,format);
    vsprintf(buffer, format, arg);
    va_end(arg);

    if(_dxd_exRemoteSlave && _dxd_exMasterfd >= 0)
        DXUIMessage("ERROR", buffer);
    else {
        fputs  (buffer, stdout);
        fputs  ("\n",   stdout);
        fflush (stdout);
    }

    exit (-1);
}

#if DXD_IS_MP
#define	VALID_ARGS	"BC:cdDE:F:H:hi:L:lmM:p:PrRsStTUuvVX"
#else
#define	VALID_ARGS	"BC:cdDE:F:H:hi:L:lmM:p:rRsStTUuvVX"
#endif

/*
 * Process command line arguments.
 */
static void ExProcessArgs (int argc, char **argv)
{
    int		opt;

    /*
     * Loop over the command line looking for arguments.
     */
    while ((opt = getopt (argc, argv, VALID_ARGS)) != EOF) {
        switch (opt) {
        case 'B':
            _dxd_exShowBells = TRUE;
            break;
        case 'C':
            extestport = atoi(optarg);
            break;
        case 'D':
            _dxd_exDebugConnect = TRUE;
            break;
        case 'E':
            _dxd_exErrorPrintLevel = atoi (optarg);
            break;
        case 'F':
            if (optarg == NULL) {
                fprintf(stdout, "missing MDF filename");
                ExUsage (argv[0]);
            }
            if (nmdfs >= MDF_MAX) {
                fprintf(stdout, "too many -F files specified");
                ExUsage (argv[0]);
            }
            mdffiles[nmdfs++] = optarg;
            break;
        case 'H':
            strcpy(extesthost, optarg);
            break;
        case 'L':
#ifdef DXD_LICENSED_VERSION

            if (optarg && !strcmp(optarg,"runtime"))
                _dxd_exForcedLicense = RTLIC;
            else if (optarg && !strcmp(optarg,"develop"))
                _dxd_exForcedLicense = DXLIC;
            else if (optarg && !strcmp(optarg,"self"))
                exLicenseSelf = TRUE;
            else {
                fprintf (stdout,
                         "Invalid license specification '%s'\n",
                         optarg ? optarg : "");
                ExUsage (argv[0]);
            }
#else
#if 0 /* This causes a malloc() which causes problems on the PVS */
            fprintf (stdout,
                     "L option ignored on non-license managed hosts\n");
#endif
#endif /* DXD_LICENSED_VERSION */

            break;
        case 'M':
            if (optarg == NULL      ||
                    ! isdigit (*optarg) ||
                    (maxMemory = atoi (optarg)) == 0) {
                fprintf (stdout,
                         "Invalid memory specification '%s'\n",
                         optarg);
                ExUsage (argv[0]);
            }
#ifndef ENABLE_LARGE_ARENAS
            {
                int mlim = (0x7fffffff >> 20);	/* divide by 1 meg */
                if (maxMemory > mlim)
                    maxMemory = mlim;
            }
#endif

            /*
             * NOTE:  If we don't call DXmemsize then the memory allocator
             * will default to something appropriate for the machine that we
             * are currently running on.
             */

            if (maxMemory > 0)
                while (DXmemsize (MEGA ((ulong)maxMemory)) != OK)
                    maxMemory--;
            break;

#if DXD_PROCESSOR_STATUS

        case 'P':
            processor_status_on = TRUE;
            break;
#endif

        case 'R':
            _dxd_exRshInput = TRUE;
            /*read_s_threshhold = READ_I_THRESHHOLD;*/
            break;
        case 'S':
            _dxd_exIntraGraphSerialize = FALSE;
            break;
        case 'T':
            _dxd_exShowTrace = TRUE;
            break;
        case 'U':
            _dxd_exRemoteUIMsg = TRUE;
            break;
        case 'X':
            _dxd_exSkipExecution = TRUE;
            break;
        case 'V':
            _dxd_exEnableDebug = TRUE;
            break;

        case 'c':
            _dxd_exCacheOn = FALSE;
            break;
        case 'd':
            _dxd_exDebug = TRUE;
            break;
        case 'h':
            ExUsage (argv[0]);
            break;
        case 'i':
            read_i_threshhold = atof (optarg);
            break;
        case 'l':
            logcache = TRUE;
            break;
        case 'm':
            MarkModules = TRUE;
            break;
        case 'p':
            if (optarg == NULL      ||
                    ! isdigit (*optarg) ||
                    (nprocs = atoi (optarg)) == 0) {
                fprintf (stdout,
                         "Invalid processor specification '%s'\n",
                         optarg);
                ExUsage (argv[0]);
            }
            break;

        case 'r':
            _dxd_exRemote = TRUE;
            break;
        case 's':
            _dxd_exRemoteSlave = TRUE;
            break;
        case 't':
            _dxd_exShowTiming = 1;
            break;
        case 'u':
            _dxd_exParseAhead = FALSE;
            break;
        case 'v':
            ExVersion ();
            break;

        default :
            ExUsage (argv[0]);
            break;
        }
    }
}


static void ExCopyright (int p)
{
    if (p) {
        write (fileno (stdout), DXD_COPYRIGHT_STRING,
               strlen (DXD_COPYRIGHT_STRING));
        write (fileno (stdout), "\n", 1);
    }
}


static void ExUsage (char *name)
{
    ExCopyright (TRUE);
    fprintf (stdout, "usage: %s ", name);
    fprintf (stdout, "[-B]");
    fprintf (stdout, "[-c]");
    fprintf (stdout, "[-d]");
    fprintf (stdout, "[-E #]");
    fprintf (stdout, "[-F file]");
    fprintf (stdout, "[-i #]");
    fprintf (stdout, "[-l]");
    fprintf (stdout, "[-m]");
    fprintf (stdout, "[-M #]");
#if DXD_IS_MP

    fprintf (stdout, "[-p #]");
#endif
#if DXD_PROCESSOR_STATUS

    fprintf (stdout, "[-P]");
#endif

    fprintf (stdout, "[-r]");
    fprintf (stdout, "[-R]");
    fprintf (stdout, "[-S]");
    fprintf (stdout, "[-t]");
    fprintf (stdout, "[-T]");
    fprintf (stdout, "[-u]");
    fprintf (stdout, "[-v]");
    fprintf (stdout, "\n");

    fprintf (stdout, "  -B    enable UI node highlighting\n");
    fprintf (stdout, "  -c    disable lookaside cache\n");
    fprintf (stdout, "  -d    enable memory debug\n");
    fprintf (stdout, "  -E    set error print level        (default = %d)\n",
             _dxd_exErrorPrintLevel);
    fprintf (stdout, "  -F    load a module definition file\n");
    fprintf (stdout, "  -i    set read delay threshhold    (default = %g)\n",
             read_i_threshhold);
    fprintf (stdout, "  -l    toggle logging to dx.log     (default = %s)\n",
             logcache ? "on" : "off");
    fprintf (stdout, "  -L    force a license type (runtime or develop)\n");

    fprintf (stdout, "  -m    mark module execution times\n");
    fprintf (stdout, "  -M    limit global memory           (default = %d)\n",
             maxMemory);

#if DXD_IS_MP

    fprintf (stdout, "  -p    number of processors         (default = %d)\n",
             nprocs);
#endif
#if DXD_PROCESSOR_STATUS

    fprintf (stdout, "  -P    toggle processor status      (default = %s)\n",
             processor_status_on ? "on" : "off");
#endif

    fprintf (stdout, "  -r    turn on remote execution\n");
    fprintf (stdout, "  -R    started with rsh but not in remote mode\n");

    fprintf (stdout, "  -S    intra graph serialization    (default = %s)\n",
             _dxd_exIntraGraphSerialize ? "on" : "off");

    fprintf (stdout, "  -t    enable exec timing & printing\n");
    fprintf (stdout, "  -T    trace module executions\n");
    fprintf (stdout, "  -u    disable parse ahead (for leak detection)\n");

    fprintf (stdout, "  -v    display executive version number\n");
    fprintf (stdout, "  -V    enables printing of executive DXDebug messages\n");
    fflush  (stdout);

    exit (2);
}


static void ExVersion ()
{
    char buf[128];

    /*
     * On the sun, using fprintf() makes _initmemory() get called, which 
     * causes unwanted memory messages about arena sizes.  So use write()
     * directly.  this is what we are putting together:
     *    printf("%s, version %02d.%02d.%04d (%s, %s)\n"
     *
     * or
     *    printf("%s, version %02d.%02d.%04d%c (%s, %s)\n"
     *  (note the letter following the revision number)
     */
    fflush(stdout);
    strcpy(buf, EX_NAME);
    strcat(buf, ", version ");
    strcat(buf, DXD_VERSION_STRING);

    strcat(buf, " (");
    strcat(buf, __TIME__);
    strcat(buf, ", ");
    strcat(buf, __DATE__);
    strcat(buf, ")\n");
    write(fileno(stdout),buf,strlen(buf));

    exit (0);
}

void _dxf_ExInitPromptVars()
{
    _dxf_ExPromptSet(PROMPT_ID_PROMPT,EX_PROMPT);
    _dxf_ExPromptSet(PROMPT_ID_CPROMPT,EX_CPROMPT);
}


void _dxf_ExInitSystemVars ()
{
    _dxf_ExInitVCRVars ();
    if(!_dxd_exRemoteSlave)
        _dxf_ExInitPromptVars ();
}


/*
 * Perform all initializations necessary to run the executive.
 */
static void ExConnectInput ()
{
    int		port;
    SFILE 	*_dxf_ExInitServer(int);

    if (_dxd_exRemote) {
        _dxf_ExGetSocket (NULL, &port);
        _dxd_exSockFD = _dxf_ExInitServer (port);

        if(_dxf_ExInitPacket() == ERROR)
            ExInitFailed ("can't make UI connection");

        if (_dxd_exSockFD == NULL)
            ExInitFailed ("can't make UI connection");
        _pIfd  = _dxd_exSockFD;
        _pIstr = "UI";
        _dxd_exIsatty = 0;
    } else {
        FILE *fptr;
        extern void GetBaseConnection(FILE **fptr, char **str);
        GetBaseConnection(&fptr, &_pIstr);
        _pIfd = FILEToSFILE(fptr);
        _dxd_exIsatty = SFILEisatty(_pIfd);
    }
}


static void ExInitialize ()
{
    int			i;
    int			n;
    int			nasked;
    char		*mm;

    _dxd_exPPID = getpid ();

    if (logcache)
        logerr = _dxf_ExLogOpen ();

    /*
     * Set up the library
     */

    /* save this until we initialize the library.  then we can use
     * the message code to warn the user if they asked for more procs
     * than are available
     */
    nasked = nprocs;

#if DXD_IS_MP

    if (nprocs <= 0 || nprocs > nphysprocs) {
        nprocs = nphysprocs;
    }
#else
    if (nprocs > 1) {
        nprocs = 1;
    }
#endif

#if DXD_LICENSED_VERSION

    if(nprocs > 1) {
        /* we call LicenseFinish later when it's safe */
        if (!ExGetLicense(MPLIC,FALSE))
            nprocs = 1;

    }

#endif /* DXD_LICENSED_VERSION */



    DXProcessors (nprocs);		/* set number of processors before */
    /* initializing the library        */

    CHECK_INIT (DXinitdx (), "cannot initialize DX library");

    /* connect to server BEFORE rest of inits which can produce messages */
    ExConnectInput ();

    if((_dxd_exContext = (Context *)DXAllocate(sizeof(Context))) == NULL)
        ExInitFailed ("can't allocate memory");
    _dxd_exContext->graphId = 0;
    _dxd_exContext->userId = 0;
    _dxd_exContext->program = NULL;
    _dxd_exContext->subp = NULL;

    if((_dxd_exHostName = (char *)DXAllocate(MAXHOSTNAMELEN)) == NULL)
        ExInitFailed ("can't allocate memory");
    gethostname(_dxd_exHostName, MAXHOSTNAMELEN);
    if ( ExHostToFQDN(_dxd_exHostName, _dxd_exHostName ) == ERROR )
        ExInitFailed ("ExHostToFQDN failed");

    /* now that lib is initialized, we can use DXMessage() if needed */
#if DXD_IS_MP

    if (nasked <= 0 || nasked > nphysprocs) {
        if(!_dxd_exRemoteSlave)
            DXUIMessage ("WARNING MSGERRUP",
                         "requested %d, using %d processors",
                         nasked, nphysprocs);
    }
#else
    if (nasked > 1) {
        if(!_dxd_exRemoteSlave)
            DXMessage ("#1080");
    }
#endif

    fflush  (stdout);

    DXSetErrorExit (0);			/* don't allow error exits */
    DXEnableDebug ("0", _dxd_exShowTrace);
    _dxfTraceObjects (0);			/* don't be verbose about objects */
    if (_dxd_exShowTiming > 1)
        DXTraceTime (TRUE);
    DXRegisterScavenger (_dxf_ExReclaimMemory);

    if (MarkModules)
        _dxd_exMarkMask = 0x20;
    else {
        if ((mm = (char *) getenv ("EXMARKMASK")) != NULL)
            _dxd_exMarkMask = strtol (mm, 0, 0);
        else
            _dxd_exMarkMask = 0x3;
    }

    CHECK_INIT (_dxf_initdisk (), "cannot initialize external disk array");

    ExSettings ();

    CHECK_INIT (_dxf_EXO_init (), "cannot initialize executive object dictionary");

    if ((exReady = (volatile int *) DXAllocate (sizeof (volatile int))) == NULL)
        ExInitFailed ("can't allocate memory");
    *exReady = FALSE;

    if ((children = (struct child *)
                    DXAllocate (sizeof (struct child) * DXProcessors (0) + 1)) == NULL)
        ExInitFailed ("can't DXAllocate");
    if ((childtbl_lock = (lock_type *)DXAllocate(sizeof(lock_type))) == NULL)
        ExInitFailed ("can't DXAllocate");
    DXcreate_lock(childtbl_lock, "lock for child table");

    if ((_dxd_exTerminating = (int *) DXAllocate (sizeof(int))) == NULL)
        ExInitFailed ("can't allocate memory");
    *_dxd_exTerminating = FALSE;

    if ((_dxd_exNSlaves = (int *) DXAllocate (sizeof(int))) == NULL)
        ExInitFailed ("can't allocate memory");
    *_dxd_exNSlaves = 0;

    if ((_dxd_extoplevelloop = (int *) DXAllocate (sizeof(int))) == NULL)
        ExInitFailed ("can't allocate memory");
    *_dxd_extoplevelloop = FALSE;

    _dxd_exSlaveId = -1;
    _dxd_exSwapMsg = FALSE;
    _dxf_InitDPtableflag();

    CHECK_INIT (_dxf_ExInitTask (nprocs), "cannot initialize task structures");

#if DXD_PROCESSOR_STATUS

    CHECK_INIT (_dxf_ExInitStatus (nprocs, processor_status_on),
                "cannot initialize processor status display");
#endif

    n = MAXGRAPHS;

    CHECK_INIT (_dxf_ExInitMemoryReclaim (),
                "cannot initialize memory reclaimation routines");

    /* create the run queue */
    CHECK_INIT (_dxf_ExRQInit (), "cannot initialize the run queue");

    n = MAXGRAPHS;
    /* don't allocate more graph slots than processors */
    n = (n > nprocs) ? nprocs : n;
    CHECK_INIT (_dxf_ExGQInit (n), "cannot initialize the graph queue");

    /* locks for module symbol table */
    CHECK_INIT (_dxf_ModNameTablesInit(), "cannot initialize symbol table");

    /* get root dictId before fork */
    _dxd_exGlobalDict = _dxf_ExDictionaryCreate (2048, TRUE, FALSE);

    _dxd_dphosts =
        (DPHOSTS *)DXAllocate(sizeof(LIST(dphosts)));
    if(_dxd_dphosts == NULL)
        ExInitFailed ("can't allocate memory for distributed table");
    INIT_LIST(*_dxd_dphosts);
    DXcreate_lock (&_dxd_dphostslock, "HostTable's Lock");
    INIT_LIST(_dxd_pgassign);
    INIT_LIST(_dxd_slavepeers);

    /* must happen after dictionary */
    CHECK_INIT (_dxf_ExInitVCR (_dxd_exGlobalDict), "cannot initialize the Sequencer");
    CHECK_INIT (_dxf_ExInitBackground (), "cannot initialize background processes");
    CHECK_INIT (_dxf_ExQueueGraphInit (), "cannot initialize for graph execution");

#if YYDEBUG != 0

    yydebug = 0;				/* don't bug me */
#endif

    CHECK_INIT (_dxf_ExCacheInit (), "cannot initialize the object cache");
    CHECK_INIT (_dxf_ExMacroInit (), "cannot initialize the macro dictionary");
    INIT_LIST(_dxd_pathtags);

    /* this does NOT use CHECK_INIT because it shouldn't be a fatal
     * error to not find an mdf file.  the loadmdf routine will set an
     * error message; if set, DXPrintError() will make it appear.
     * libdx is initialized at this point, so calling SetError, PrintError
     * and ResetError should be ok.
     */
    for (i=0; i<nmdfs; i++)
        if (DXLoadMDFFile (mdffiles[i]) == ERROR) {
            DXPrintError("MDF file");
            DXResetError();
        }

    if (! _dxfLoadDefaultUserInteractors())
        ExInitFailed("Error loading user interactor files");



    /*
     * System variables should be set before .dxrc processing too.
     */

    _dxf_ExInitSystemVars ();

}


static void ExSettings ()
{
    ExDebug ("*1", "intra graph serialize    %s",
             _dxd_exIntraGraphSerialize ? "ON" : "OFF");
    ExDebug ("*1", "lookaside caching        %s",
             _dxd_exCacheOn        ? "ON" : "OFF");
    ExDebug ("*1", "processors               %d", nprocs);
    ExDebug ("*1", "status display           %s",
             processor_status_on       ? "ON" : "OFF");
    ExDebug ("*1", "execution log            %s", logcache ? "ON" : "OFF");
    ExDebug ("*1", "");
}


/*
 * Have conditions for termination been met yet?
 */

static void ExCheckTerminate ()
{
    int		n = 3, i;

    if (! (*_dxd_exTerminating))
        return;

    /*
     * Make sure that nothing slipped in on us.  Particularly from the
     * VCR operating on another processor.
     */

    while (n--) {
        if (_dxf_ExRQPending ())
            return;
        if (! _dxf_ExGQAllDone ())
            return;
        if (_dxf_ExVCRRunning ())
            return;
    }

    /*
     * signal childen to loop so they will see the terminate flag
     */
    for (i = 1; i < nprocs; i++)
        write(children[i].RQwrite_fd, "a", 1);

    _dxf_ExCacheFlush (TRUE);
    _dxf_ExDictionaryPurge (_dxd_exGlobalDict);

    /*
     * send out exit message before ExCleanup, ExCleanup
     * can get called multiple times. If a child exits first
     * ExCleanup gets called a second time by the parent
     * to clean up the rest of the children.
     */
    if(!_dxd_exRemoteSlave) {
        int peerwait = 0;
        _dxf_ExDistributeMsg(DM_EXIT, (Pointer)&peerwait, 0, TOSLAVES);
    } else
        close(_dxd_exMasterfd);
    ExCleanup ();
    exit(0);
}


/*
 * Error quit
 */
void ExQuit()
{
    int i;

    (*_dxd_exTerminating) = 1;

    /*
     * signal childen to loop so they will see
     * the terminate flag (if they are still there)
     */
#if HAVE_SIGQUIT

    for (i = 1; i < nprocs; i++)
        kill(children[i].pid, SIGQUIT);
#endif

    _dxf_ExCacheFlush (TRUE);
    _dxf_ExDictionaryPurge (_dxd_exGlobalDict);

    /*
     * send out exit message before ExCleanup, ExCleanup
     * can get called multiple times. If a child exits first
     * ExCleanup gets called a second time by the parent
     * to clean up the rest of the children.
     */
    if(!_dxd_exRemoteSlave) {
        int peerwait = 0;
        _dxf_ExDistributeMsg(DM_EXIT,
                             (Pointer)&peerwait, 0, TOSLAVES);
    } else
        close(_dxd_exMasterfd);

    ExCleanup ();
    exit(1);
}

/*
 * Perform any cleanups necessary to free up system resources.
 */
static void ExCleanup ()
{
    int		ok;
    int         i,limit;
    dpgraphstat *index;
    PGassign	*pgindex;
    SlavePeers  *sindex;

    user_cleanup();

    if (MarkModules)
        DXPrintTimes ();
    ExDebug ("*1", "in ExCleanup");

#ifdef DXD_LICENSED_VERSION

    _dxf_ExReleaseLicense();
#endif

    /* for MP machines it is possible that someone will be running only 1 */
    /* processor but will have the status window turned on. The status    */
    /* windowing creates a new process so we have to clean that up right. */
    ok = ((exParent && _dxd_exMyPID==-1) || (nprocs == 1 && _dxd_exMyPID == 0 && ! processor_status_on));

    if (! ok)
        exit (0);

    FREE_LIST(_dxd_pathtags);

    for (i = 0, limit = SIZE_LIST(_dxd_dpgraphstat); i < limit; ++i) {
        index = FETCH_LIST(_dxd_dpgraphstat, i);
        DXFree(index->prochostname);
        DXFree(index->procusername);
        if(index->options)
            DXFree(index->options);
		if(index->procfd > 0)
	        close(index->procfd);
    }
    FREE_LIST(_dxd_dpgraphstat);

    for (i = 0, limit = SIZE_LIST(_dxd_pgassign); i < limit; ++i) {
        pgindex = FETCH_LIST(_dxd_pgassign, i);
        DXFree(pgindex->pgname);
        pgindex->hostindex = -1;
    }
    FREE_LIST(_dxd_pgassign);

    for (i = 0, limit = SIZE_LIST(_dxd_slavepeers); i < limit; ++i) {
        sindex = FETCH_LIST(_dxd_slavepeers, i);
        DXFree(sindex->peername);
        close(sindex->sfd);
    }
    FREE_LIST(_dxd_slavepeers);

    _dxf_exitdisk ();

    if(!_dxd_exRemoteSlave)
        DXMessage ("#1090");

#ifdef INSTRUMENT

    ExPrintInstrument ();
    ExFreeInstrument();
#endif

    DXqflush ();

    DXFree ((Pointer) exReady);

    /* make sure there are no shared memory segments still waiting to */
    /* be attached to by other processes. */
    _dxfcleanup_mem();

    /*
     * make sure other kids go away before we really start blowing things
     * away
     */

#if DXD_EXEC_WAIT_PROCESS

    ExKillChildren ();
#endif

    if (logcache)
        _dxf_ExLogClose ();

    return;
}

/*
 * Fork off the child processes which will migrate to different physical
 * processors to give us our multi-processor support.
 */
static void ExForkChildren ()
{
    int		pid;
    int		i;

#ifdef INSTRUMENT

    ExAllocateInstrument (nprocs);
#endif

    children[0].pid = getpid ();

    /*
     * Don't fork if we are running uni-processor and were not creating
     * any other processes by creating a status window.
     */

    if (nprocs == 1 && ! processor_status_on) {
        _dxd_exMyPID = ExFindPID (children[0].pid);
        if(_dxd_exMyPID < 0)
            _dxf_ExDie("Fork Children unable to get child id %d",
                       children[0].pid);

        return;
    }

#if !defined(intelnt) && !defined(WIN32)

    /* set this before we fork and create separate data spaces. */
    _dxd_exGoneMP = TRUE;

#if DXD_HAS_LIBIOP

    {
        int         *GI = NULL;
        lock_type   *LI = NULL;
        GI = (int *)DXAllocate(sizeof(int));
        LI = (lock_type *)DXAllocate(sizeof(lock_type));
        if(GI == NULL || LI == NULL)
            _dxf_ExDie("pfork setup failed");
        *GI = 0;
        if(!DXcreate_lock(LI, "pfork"))
            _dxf_ExDie("pfork create lock failed");

        /* fork all processes at once */
        pid = pfork (nprocs - 1);

        /* this code is forked and is run on all processes. *GI holds an */
        /* index into array of process ids. Lock and then increment      */
        /* counter before filling in that array entry for each processor */
        DXlock(LI, 0);
        i = *GI;
        *GI += 1;
        DXunlock(LI, 0);
        children[i].pid = getpid ();
    }
#else
    /* fork off processes for each of the processors */
#if DXD_EXEC_WAIT_PROCESS
    for (i = 0; i < nprocs; i++)
#else

    for (i = 1; i < nprocs; i++)
#endif

    {
        int p;

#if DXD_EXEC_WAIT_PROCESS && !defined(DXD_MASTER_IS_P0)

        if (i == nprocs-1)
            p = 0;
        else
            p = i + 1;
#else

    p = i;
#endif

        /* flush all output files prior to forking */
        fflush (stdout);
        fflush (stderr);

        pid = DXForkChild(p);

        if (pid == 0)
        {
            break;
        }

        if (pid == -1)
            perror ("main: fork failed");
    }
#endif

#if DXD_EXEC_WAIT_PROCESS
    /* don't pin parent process */
    if (_dxd_exPPID != getpid ()) {
#endif
        DXlock(childtbl_lock, 0);
        _dxd_exMyPID = ExFindPID (getpid ());
        DXunlock(childtbl_lock, 0);

        if(_dxd_exMyPID < 0)
            _dxf_ExDie("Fork Children unable to get child id %d",
                       getpid());

#if DXD_EXEC_WAIT_PROCESS
        /* The original process (the master) is also the "exParent", and
         * can allow the other processes to start now
         */
        if (_dxd_exMyPID == 0) {
            *exReady = TRUE;
            exParent = TRUE;
        }
#else
        *exReady = TRUE;
#endif

#if DXD_HAS_SYSMP

        if (nprocs > 1 && nphysprocs == nprocs) {
            i = sysmp (MP_MUSTRUN, _dxd_exMyPID % nphysprocs);
            if (i < 0) {
                char buffer[256];

                sprintf(buffer, "%d:  MP_MUSTRUN failed", _dxd_exMyPID);
                perror(buffer);
            }
        }
#endif
#if DXD_EXEC_WAIT_PROCESS

    } else
        _dxd_exMyPID = -1;
#endif
#endif
}

/* return the number of physical processors on the system.
 *  this is different from the number of processes the user
 *  asked us to use with -pN
 */
int _dxfPhysicalProcs()
{
    return nphysprocs;
}

void _dxf_lock_childpidtbl()
{
    DXlock(childtbl_lock, 0);
}

static int ExReadCharFromRQ_fd (int fd, Pointer p)
{
    int ret;
    char c;

#if DEBUGMP

    if(_dxd_exMyPID == 0)
        DXMessage("starting read from rq %d", fd);
#endif

    ret = read(fd, &c, 1);
    if(ret != 1) {
        /*
               DXMessage("Error reading from request queue pipe: %d %d", errno, fd);
        */
        DXRegisterInputHandler(NULL, fd, NULL);
    }

#if DEBUGMP
    if(_dxd_exMyPID == 0)
        DXMessage("-----------finished read from rq");
#endif

    return 0;
}

void _dxf_update_childpid(int i, int pid, int writefd)
{
    /* table should already be locked before this call */

#if DEBUGMP
    DXMessage("child %d, writefd %d", i, writefd);
#endif

    children[i].pid = pid;
    children[i].RQwrite_fd = writefd;
    DXunlock(childtbl_lock, 0);
}

void _dxf_set_RQ_ReadFromChild1(int readfd)
{
    exChParent_RQread_fd = readfd;
}

/*
 * Set up fd that slaves block on waiting for the master to
 * put work in the run queue
 */
void _dxf_set_RQ_reader(int fd)
{
    exParent_RQread_fd = fd;
    DXRegisterInputHandler(ExReadCharFromRQ_fd, fd, NULL);
}

/*
 * On slave #1 set up fd for writing to the master
 */
void
_dxf_set_RQ_writer(int fd)
{
    exParent_RQwrite_fd = fd;
}

Error _dxf_parent_RQ_message()
{
    int ret;

#if DEBUGMP

    DXMessage("writing to parent %d", exParent_RQwrite_fd);
#endif

    ret = write(exParent_RQwrite_fd, "a", 1);
    if(ret != 1)
        _dxf_ExDie("Write Erroring notifying parent of job request, write returns %d, error number %d", ret, errno);
    return ERROR;
}

/* Send a message to the child so they know there is work on the queue. */
/* If there is a jobid then only notify the child that the job is for,  */
/* otherwise notify all children and the first to get the job wins.     */
Error _dxf_child_RQ_message(int *jobid)
{
    int i;
    int procid;
    int ret;

#if !defined(HAVE__ERRNO)

    errno = 0;
#endif

    procid = *jobid - 1;

    if(procid == 0) {
        if(_dxd_exMyPID != 0)
            DXWarning("Ignoring rq message to parent");
        return ERROR;
    }

    if(procid > 0) {
#if DEBUGMP
        DXMessage("send job request to %d, %d", procid, children[procid].RQwrite_fd);
#endif

        ret = write(children[procid].RQwrite_fd, "a", 1);
        if(ret != 1)
            _dxf_ExDie("Write Erroring notifying child of job request, write returns %d, error number %d", ret, errno);
#if DEBUGMP

        else
            DXMessage("successful write to %d", children[procid].RQwrite_fd);
#endif

    } else { /* if procid == -1 then send message to all children */
        for (i = 1; i < nprocs; i++) {
#if DEBUGMP
            DXMessage("send job request to %d, %d", i, children[i].RQwrite_fd);
#endif

            ret = write(children[i].RQwrite_fd, "a", 1);
            if(ret != 1)
                _dxf_ExDie("Write Erroring notifying child of job request, write returns %d, error number %d", ret, errno);
#if DEBUGMP

            else
                DXMessage("successful write to %d", children[i].RQwrite_fd);
#endif

        }
    }
    return ERROR;
}

/*
 * Processors are identified by their index into the table of child
 * processes.
 */
static int ExFindPID (int pid)
{
    int i;

    for (i = 0; i < nprocs; i++)
        if (pid == children[i].pid)
            return (i);

    DXWarning ("#4510", pid);
    return (-1);
}


static int OKToRead (SFILE *fp)
{
    if (ExCheckParseBuffer())
        return 1;

    return SFILECharReady(fp);
}

void _dxf_ExPromptSet(char *var, char *val)
{
    gvar	*gv;
    String	pmpt;

    pmpt = DXNewString(val);
    gv = _dxf_ExCreateGvar (GV_DEFINED);
    _dxf_ExDefineGvar (gv, (Object)pmpt);
    _dxf_ExVariableInsert (var, _dxd_exGlobalDict, (EXObj)gv);
}

char * _dxf_ExPromptGet(char *var)
{
    gvar	*gv;
    char	*val;

    if ((gv = (gvar*)_dxf_ExVariableSearch (var, _dxd_exGlobalDict)) == NULL)
        return (NULL);
    if (DXExtractString((Object)gv->obj, &val) == NULL)
        val = NULL;
    ExDelete (gv);
    return (val);
}

static int ExFromMasterInputHndlr (int fd, Pointer p)
{
    Program		*graph = NULL;
    DistMsg             pcktype;
    int			b, peerwait;
    int 		graphId;
    DictType		whichdict;
    int			cacheall, namelen;
    char		name[1024];
    dpversion 		dpv;

    if ((IOCTL(_dxd_exMasterfd, FIONREAD, (char *)&b) < 0) || (b <= 0)) {
        printf("Connect to Master closed\n");
        ExCleanup();
        exit(0);
    }

    if(_dxf_ExReceiveBuffer(_dxd_exMasterfd, &pcktype, 1, TYPE_INT,
                            _dxd_exSwapMsg) < 0) {
        DXUIMessage("ERROR", "bad distributed packet type");
        ExCleanup();
        exit(0);
    }

    if(pcktype == DPMSG_SIGNATURE || pcktype == DPMSG_SIGNATUREI) {
        if(_dxd_exDebug)
            printf("signature %x\n", pcktype);
        if(pcktype == DPMSG_SIGNATUREI)
            _dxd_exSwapMsg = TRUE;
        else
            _dxd_exSwapMsg = FALSE;
        if(_dxf_ExReceiveBuffer(_dxd_exMasterfd, &pcktype, 1, TYPE_INT,
                                _dxd_exSwapMsg) < 0) {
            DXUIMessage("ERROR", "bad distributed packet type");
            ExCleanup();
            exit(0);
        }
    }

    if(_dxd_exDebug)
        printf("packet type %d\n", pcktype);
    else
        ExDebug("7", "packet type %d", pcktype);

    switch(pcktype) {
    case DM_PARSETREE:
        _dxd_exParseTree = _dxf_ExReadTree(_dxd_exMasterfd, _dxd_exSwapMsg);
        if (_dxd_exParseTree != NULL) {
            set_status (PS_GRAPHGEN);
            _dxf_ExGraphInit ();
            graph = _dxf_ExGraph (_dxd_exParseTree);
            if (graph != NULL) {
                graph->origin = GO_FOREGROUND;
                set_status (PS_GRAPHQUEUE);
#ifdef MAIN_TIMING
                DXMarkTimeLocal ("pre  gq_enq");
#endif

                _dxf_ExGQEnqueue (graph);
            }
        }
        if (_dxd_exParseTree != NULL) {
            if (graph == NULL && !_dxd_exRemoteSlave)
                _dxf_ExSPack (PACK_COMPLETE, _dxd_exContext->graphId,
                              "Complete", 8);

            _dxf_ExPDestroyNode (_dxd_exParseTree);
        }
        break;
    case DM_INSERTMDICT: {
            node *n;
            n = _dxf_ExReadTree(_dxd_exMasterfd, _dxd_exSwapMsg);
            _dxf_ExMacroInsert (n->v.macro.id->v.id.id, (EXObj) n);
        }
        break;
    case DM_INSERTGDICT:
        _dxf_ExRecvGDictPkg(_dxd_exMasterfd, _dxd_exSwapMsg, 0);
        break;
    case DM_INSERTGDICTNB:
        _dxf_ExRecvGDictPkg(_dxd_exMasterfd, _dxd_exSwapMsg, 1);
        break;
    case DM_EVICTCACHE:
        break;
    case DM_KILLEXECGRAPH:
        *_dxd_exKillGraph = TRUE;
    case DM_EXECGRAPH:
        if(_dxf_ExReceiveBuffer(_dxd_exMasterfd, &graphId, 1, TYPE_INT,
                                _dxd_exSwapMsg) < 0) {
            DXUIMessage("ERROR", "bad graph id");
            *_dxd_exKillGraph = TRUE;
        }
        ExCheckGraphQueue(graphId);
        ExCheckRunqueue(0);
        _dxf_ResumePeers();
        break;
    case DM_SLISTEN:
        _dxf_ExSlaveListen();
        break;
    case DM_SCONNECT:
        _dxf_ExSlaveConnect();
        break;
    case DM_SLAVEID:
        if(_dxf_ExReceiveBuffer(_dxd_exMasterfd, &_dxd_exSlaveId, 1,
                                TYPE_INT, _dxd_exSwapMsg) < 0) {
            DXUIMessage("ERROR", "bad peer id");
        }
        ExDebug("7", "My Slave Number is %d", _dxd_exSlaveId);
        if(_dxf_ExReceiveBuffer(_dxd_exMasterfd, &namelen, 1,
                                TYPE_INT, _dxd_exSwapMsg) < 0) {
            DXUIMessage("ERROR", "bad name length for slave hostname");
        }
        if(_dxf_ExReceiveBuffer(_dxd_exMasterfd, _dxd_exHostName, namelen,
                                TYPE_UBYTE, _dxd_exSwapMsg) < 0) {
            DXUIMessage("ERROR", "error receiving slave hostname");
        }
        break;
    case DM_VERSION:
        if(_dxf_ExReceiveBuffer(_dxd_exMasterfd, &dpv.version, 1,
                                TYPE_INT, _dxd_exSwapMsg) < 0) {
            DXUIMessage("ERROR", "bad version number for distributed");
        }
        ExDebug("7", "DPVERSION is %d", dpv.version);
        break;
    case DM_FLUSHGLOBAL:
        _dxf_ExFlushGlobal();
        break;
    case DM_FLUSHMACRO:
        _dxf_ExFlushMacro();
        break;
    case DM_FLUSHCACHE:
        if(_dxf_ExReceiveBuffer(_dxd_exMasterfd, &cacheall, 1, TYPE_INT,
                                _dxd_exSwapMsg) < 0)
            DXUIMessage("ERROR", "flush cache: bad parameter");
        _dxf_ExCacheFlush(cacheall);
        break;
    case DM_FLUSHDICT:
        if(_dxf_ExReceiveBuffer(_dxd_exMasterfd, &whichdict, 1,
                                TYPE_INT, _dxd_exSwapMsg) < 0)
            DXUIMessage("ERROR", "bad dictionary type");
        switch(whichdict) {
        case DICT_GLOBAL:
            _dxf_ExDictionaryPurge (_dxd_exGlobalDict);
            break;
        case DICT_MACRO:
            _dxf_ExDictionaryPurge (_dxd_exMacroDict);
            break;
        case DICT_GRAPH:
            _dxf_ExDictionaryPurge (_dxd_exGraphCache);
            break;
        default:
            break;
        }
        break;
    case DM_GRAPHDELETE:
        _dxf_SuspendPeers();
        _dxf_ExGQDecrement(NULL);
        _dxf_ExDistributeMsg(DM_GRAPHDELETECONF, NULL, 0, TOMASTER);
        break;
    case DM_PERSISTDELETE: {
            DelRemote drpkg;

#define GETPER(what, len, whattype) \
 if (_dxf_ExReceiveBuffer(_dxd_exMasterfd, what, len, whattype,  \
      _dxd_exSwapMsg) < 0) \
      goto perout_error

            GETPER (&drpkg.del_namelen, 1, TYPE_INT);
            GETPER (drpkg.del_name, drpkg.del_namelen, TYPE_UBYTE);
            GETPER (&drpkg.del_instance, 1, TYPE_INT);

            if (_dxf_ExDeleteRemote(drpkg.del_name, drpkg.del_instance) == ERROR)
                DXUIMessage("ERROR", "error deleting persistent outboard module");

            break;

perout_error:
            DXUIMessage("ERROR",
                        "bad request to delete persistent outboard module");
            break;
        }
    case DM_LOADMDF:
        if (_dxf_ExRecvMdfPkg(_dxd_exMasterfd, _dxd_exSwapMsg) == ERROR) {
            DXUIMessage("ERROR", "error loading additional mdf entries");
        }
        break;
    case DM_EXIT:
        if(_dxf_ExReceiveBuffer(_dxd_exMasterfd, &peerwait, 1, TYPE_INT,
                                _dxd_exSwapMsg) < 0) {
            DXUIMessage("ERROR", "bad peer wait value for exit");
        }
        if(peerwait)
            _dxf_ExWaitForPeers();
        ExCleanup();
        exit(0);
    case DM_DELETEPEER:
        if(_dxf_ExReceiveBuffer(_dxd_exMasterfd, &namelen, 1, TYPE_INT,
                                _dxd_exSwapMsg) < 0) {
            DXUIMessage("ERROR", "bad name length for peer name");
        }
        if(_dxf_ExReceiveBuffer(_dxd_exMasterfd, name, namelen,
                                TYPE_UBYTE, _dxd_exSwapMsg) < 0) {
            DXUIMessage("ERROR", "error receiving peer host name");
        }
        /* delete peer and close socket */
        _dxf_ExDeletePeerByName(name, 1);
        break;
    default:
        DXUIMessage("ERROR", "bad message type %d", pcktype);
        ExCleanup();
        exit(1);
    }
    return (0);
}

int ExCheckInput ()
{
    Program		*graph;
    static int 		prompted = FALSE;
    char		*prompt;
    Context		savedContext;
    extern SFILE        *_dxd_exBaseFD;


    /* don't read anymore input if we are exiting */
    if (*_dxd_exTerminating)
        return (0);

    _dxf_ExCheckPacket(NULL, 0);

    /* If this is the terminal, and the user hasn't typed anything yet,
     * prompt him.
     */
    if ((SFILEisatty(yyin) || (_dxd_exRshInput && yyin == _dxd_exBaseFD)) &&
            !prompted && _dxf_ExGQAllDone() && !SFILECharReady(yyin)) {
        prompt = _dxf_ExPromptGet(PROMPT_ID_PROMPT);
        printf (prompt? prompt: EX_PROMPT);
        fflush (stdout);
        prompted = TRUE;
    }

    /* If we have input */
    if (OKToRead (yyin)) {
        ExDebug ("*1", "input");
        prompted = FALSE;
        set_status (PS_PARSE);
        /* save the values from _dxd_exContext in savedContext */
        _dxfCopyContext(&savedContext, _dxd_exContext);
        DXqflush ();
        _dxf_ExBeginInput ();
        yyparse ();				/* parse a command */
        _dxf_ExEndInput ();
        DXqflush ();

        /* restore the values in _dxd_exContext from savedContext */
        _dxfCopyContext(_dxd_exContext, &savedContext);
        graph = NULL;

        if (_dxd_exParseTree != NULL) {
            set_status (PS_GRAPHGEN);
            _dxf_ExGraphInit ();
            graph = _dxf_ExGraph (_dxd_exParseTree);
            /* we are the master, send a copy of the parse tree
             * to all slaves 
             */
            _dxf_ExSendParseTree(_dxd_exParseTree);
            if (graph != NULL) {
                graph->origin = GO_FOREGROUND;
                set_status (PS_GRAPHQUEUE);
#ifdef MAIN_TIMING
                DXMarkTimeLocal ("pre  gq_enq");
#endif

                _dxf_ExGQEnqueue (graph);
            }
        }

        set_status (PS_EXECUTIVE);

        /*
         * Tell remote that immediate statements are complete and clean
         * up the parse tree.
         */

        if (_dxd_exParseTree != NULL) {
            if (graph == NULL)
                _dxf_ExSPack (PACK_COMPLETE, _dxd_exContext->graphId, "Complete", 8);

            _dxf_ExPDestroyNode (_dxd_exParseTree);
        }

#ifdef MAIN_TIMING
        DXMarkTimeLocal ("post destro");
#endif

        return (1);
    }

    return (0);
}

/*
 * See if there are any tasks ready to be executed.
 */
static int ExCheckRunqueue (int graphId)
{
    return (_dxf_ExRQDequeue (0));
}

/*
 * See if there is a graph ready for execution
 */
static int ExCheckGraphQueue (int newGraphId)
{
    Program	*graph;
    graph = _dxf_ExGQDequeue ();

    if (graph == NULL)
        return (FALSE);

#ifdef DXD_LICENSED_VERSION

    if (!_dxd_ExHasLicense) {
        DXUIMessage("LICENSE","NO LICENSE\n");
        return (FALSE);
    }

#endif /* DXD_LICENSED VERSION */

    if (newGraphId >= 0)
        graph->graphId = newGraphId;

    _dxd_exContext->graphId = graph->graphId;

    /*
     * Schedule graph nodes which are ready for execution.
     */
#ifdef GQ_TIMING

    DXMarkTimeLocal ("pre _dxf_ExQueueGraph");
#endif

    set_status (PS_GRAPHQUEUE);
    _dxf_ExQueueGraph (graph);
    set_status (PS_EXECUTIVE);
#ifdef GQ_TIMING

    DXMarkTimeLocal ("post _dxf_ExQueueGraph");
#endif

    _dxd_exContext->graphId = 0;

    return (TRUE);
}


#define	EX_LOOP_TRIES	1000	/* limit sizing iterations		*/
#define	EX_LOOP_PER_SEC	30	/* non-quiescent input check frequency	*/

#if ibmpvs
#define	EX_SELECT	128
#define	EX_INCREMENT	0x1
static int EX_LIMIT	= 0;
#elif sgi || ibm6000
#define	EX_SELECT	16
#define	EX_INCREMENT	0x1
#define	EX_LIMIT	0x100
#elif solaris
#define	EX_SELECT	0
#define	EX_INCREMENT	0x1
#define	EX_LIMIT	0x100
#else
#define	EX_SELECT	1024
#define	EX_INCREMENT	0x100
#define	EX_LIMIT	0x100000
#endif


static int ExInputAvailable (SFILE *fp)
{
    static int		iters	= 0;
    extern SFILE        *_dxd_exBaseFD;

    if (ExCheckParseBuffer())
        return TRUE;

    _dxf_ExCheckPacket(NULL, 0);

    if (SFILECharReady(fp))
        return TRUE;

    if (fp != _dxd_exBaseFD)
        return TRUE;

    if (++iters < EX_SELECT) {
        int ret, n, fd = SFILEfileno (fp);
        ret = IOCTL (fd, FIONREAD, (char *) &n);
        return (n > 0 || ret < 0);
    } else
        return 0;
}


#define	ISSUE_PROMPT()\
{\
    char	*prompt;\
    if (! prompted &&\
        !SFILECharReady(yyin) &&\
	(_dxd_exRshInput || _dxd_exIsatty) &&\
	_dxf_ExGQAllDone ())\
    {\
	prompt = _dxf_ExPromptGet (PROMPT_ID_PROMPT);\
	printf (prompt ? prompt : EX_PROMPT);\
	fflush (stdout);\
	prompted = TRUE;\
    }\
}

static void ExRegisterRQ_fds()
{
    DXRegisterInputHandler(ExReadCharFromRQ_fd, exChParent_RQread_fd, NULL);
}

static void ExParallelMaster ()
{
    Program		*graph;
    Context             savedContext;
    int			tries		= 0;
    int			limit		= 0;
    static int		prompted	= FALSE;
#if defined(ibmpvs)

    int			reading		= FALSE;
    int			cnt		= 0;
    double		start		= SVS_double_time ();
#else
#define			reading		TRUE
#endif

    _dxd_exParseTree = NULL;

    ExRegisterRQ_fds();

    for (;;) {
        if (++tries > limit) {
            /*
             * If this is the terminal, and the user hasn't typed anything
             * yet then prompt him.
             */

            ISSUE_PROMPT ();
            _dxf_ExCheckRIH();

            while (reading && ! *_dxd_exTerminating &&
                    (ExInputAvailable (yyin) || _dxd_exSelectReturnedInput)
                    && _dxd_exParseAhead) {
                limit = -EX_INCREMENT;

                _dxd_exSelectReturnedInput = FALSE;
                prompted = FALSE;
                set_status (PS_PARSE);
                /* save the values from _dxd_exContext in savedContext */
                _dxfCopyContext(&savedContext, _dxd_exContext);
                _dxf_ExBeginInput ();
                if (reading)
                    yyparse ();
                _dxf_ExEndInput ();
                /* restore the values in _dxd_exContext from savedContext */
                _dxfCopyContext(_dxd_exContext, &savedContext);
                if (_dxd_exParseTree) {
                    set_status (PS_GRAPHGEN);
                    _dxf_ExGraphInit ();
                    graph = _dxf_ExGraph (_dxd_exParseTree);
                    /* we are the master, send a copy of the parse tree
                     * to all slaves 
                     */
                    _dxf_ExSendParseTree(_dxd_exParseTree);

                    if (graph) {
                        graph->origin = GO_FOREGROUND;
                        set_status (PS_GRAPHQUEUE);
                        _dxf_ExGQEnqueue (graph);
                        ExCheckGraphQueue (-1);
                        if (_dxf_ExCheckBackground (_dxd_exGlobalDict, TRUE) ||
                                _dxf_ExCheckVCR (_dxd_exGlobalDict, TRUE))
                            ExCheckGraphQueue (-1);
                    } else {
                        _dxf_ExSPack (PACK_COMPLETE, _dxd_exContext->graphId, "Complete", 8);
                    }

                    _dxf_ExPDestroyNode (_dxd_exParseTree);
                }

                set_status (PS_EXECUTIVE);
            }

#if defined(DDX)
            {
                extern Error SlaveBcastLoop(int, Pointer);
                extern int GetMPINodeId();
                if (GetMPINodeId() == 0)
                    _dxf_ExCheckRIHBlock (SFILEfileno (yyin));
                else {
                    SlaveBcastLoop(0, NULL);
                    _dxf_ExCheckRIH ();
                }
            }
#endif

            limit += EX_INCREMENT;
            if (limit > EX_LIMIT)
                limit = EX_LIMIT;

            tries = 0;
        }

        if (ExCheckGraphQueue (-1))
            continue;

        if (_dxf_ExGQAllDone ())
            _dxf_RunPendingCmds();

        if (_dxf_ExCheckBackground (_dxd_exGlobalDict, TRUE) ||
                _dxf_ExCheckVCR (_dxd_exGlobalDict, TRUE)) {
            if (ExCheckGraphQueue (-1))
                continue;
        }

        /*
         * If we run a job here then we immediately want to check to
         * see whether any new input has come in.
         */

        if (_dxf_ExRQPending () && _dxf_ExRQDequeue (0)) {
#if DEBUGMP
            DXMessage("got something");
#endif

            tries = limit;
            continue;
        }

        ExCheckTerminate ();

        /*
         * Since 'os' can't handle blocking reads without blocking the 
         * entire system, but it can handle blocking selects, we must use
         * the later to block on input so that I/O processing destined
         * for other processors, specifically that done by RIH (e.g.
         * X window expose events, and status window updates, is not also
         * blocked.
         *
         * $$$$$
         * For now this seems to make things worse so we'll leave it out.
         * $$$$$
         */

        if (reading && _dxf_ExGQAllDone () && ! _dxf_ExVCRRunning ()) {
            ISSUE_PROMPT ();
            if (! _dxd_exParseAhead) {
                /* if we get here there is nothing in the queues */
                /* restore parse ahead in case it was changed */
                _dxd_exParseAhead = _dxd_exSParseAhead;
            }

#if defined(DDX)
            if(!_dxf_ExIsExecuting() && !ExInputAvailable(yyin)) {
                extern Error SlaveBcastLoop(int, Pointer);
                extern int GetMPINodeId();
                if (GetMPINodeId() == 0)
                    _dxf_ExCheckRIHBlock (SFILEfileno (yyin));
                else {
                    SlaveBcastLoop(0, NULL);
                    _dxf_ExCheckRIH ();
                }
            }
#else
#ifndef DXD_NOBLOCK_SELECT
            if(!_dxf_ExIsExecuting() && !ExInputAvailable(yyin))
                _dxf_ExCheckRIHBlock (SFILEfileno (yyin));
#endif
#endif

        }

#if sgi
        set_status (PS_NAPPING);
        sginap (0);
        set_status (PS_EXECUTIVE);
#endif

    }
}

#if defined(HAVE_SIGPIPE)
static void ExSigPipe(int signo)
{
    /*
     * If I am a slave, send a quit signal to the master.
     * Otherwise, just quit.
     */
    if (_dxd_exMyPID < 0 || DXProcessorId() != 0) {
#if 0
        fprintf(stderr, "ExSigPipe: slave received %d\n", signo);
#endif
#if HAVE_SIGQUIT

        kill(children[0].pid, SIGQUIT);
#endif

    } else {
#if 0
        fprintf(stderr, "ExSigPipe: master received %d\n", signo);
#endif

        ExQuit();
    }
}
#endif

#if defined(HAVE_SIGQUIT)
static void ExSigQuit(int signo)
{
    /*
     * Received by the master from a slave that
     * was told to quit, due to either a SIGPIPE
     * or SIGDANGER signal.  ExQuit will then send
     * SIGUSR2 to the children.
     */
#if 0
    fprintf(stderr, "ExSigQuit: %s receive %d\n",
            DXProcessorId() == 0 ? "master" : "slave", signo);
#endif

    if (DXProcessorId() == 0)
        ExQuit();
    else
        exit(0);
}
#endif

#if defined(HAVE_SIGDANGER)
static void ExSigDanger (int signo)
{
    DXSetError (ERROR_INTERNAL, "#8300");
    DXPrintError (NULL);
    ExQuit();
    exit(1);
}
#endif

#ifdef DXD_WIN
int DXWinFork()
{
    return -1;
}

#endif

#if !defined(intelnt) && !defined(WIN32)

int DXForkChild(int i)
{
    int pid, master2slave[2], slave2master[2];

    _dxf_lock_childpidtbl();

    /*
     * The slaves always need to hear from the master
     */
    if (pipe(master2slave))
      return ERROR;

    /*
     * The master only needs to hear from slave[1]
     */
    if (i == 1)
      if (pipe(slave2master))
          return ERROR;

    pid = fork();

    /*
     * The right thing to do depends on whether we're the parent 
     * or child process.  The parent is always the master, and the
     * child is always a slave.
     */
    if (pid)
    {
        /*
         * The master writes to master2slave[1].
         */
        close(master2slave[0]);
        _dxf_update_childpid(i, pid, master2slave[1]);

        /*
         * The master only needs to hear from slave[1]
         */
        if (i == 1)
        {
            close(slave2master[1]);
            _dxf_set_RQ_ReadFromChild1(slave2master[0]);
        }
    }
    else
    {
        /*
         * The child is a slave.  It needs to listen to the
         * master. Note that the slave2master pipe is only
         * required for node 1, and that the master (if we
	 * forked it at all) should not watch an master2slave
         */
        close(master2slave[1]);

	if (i != 0)
	    _dxf_set_RQ_reader(master2slave[0]);
  
        if (i == 1)
        {
              close(slave2master[0]);
            _dxf_set_RQ_writer(slave2master[1]);
        }
    }

    return  pid;
}
#endif

#if defined(DXD_WIN_SMP)  && defined(THIS_IS_COMMENTED)
static int MyChildProc()
{
    int i;
    static int iCount = 0;
    static __declspec(thread)  int tls_td;

    iCount++;

    tls_td = GetCurrentThreadId();

    _dxf_lock_childpidtbl();
    _dxf_update_childpid(iCount, tls_td, -1);

    i = tls_td;
    _dxd_exMyPID = ExFindPID (DXGetPid ());
    i = _dxd_exMyPID;

    printf("In New child Thread   %d \n",tls_td);

    ExChildProcess();

    /*
    _dxf_ExInitTaskPerProc();
    ExMainLoopSlave ();
    */

    printf("End Thread   %d \n",tls_td);
    return 1;
}

int DXWinFork()
{
    int i;

    i = _beginthread(MyChildProc, 0, NULL);
    return i;
}

DXGetPid()
{
    int i;
    i = GetCurrentThreadId();
    return i;
}


#endif


