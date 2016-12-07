/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 2004                                        */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

/* This file is a version of dxmain specifically written to be a thread
 * of a main process such as the User Interface. It is needed because
 * Windows does not share the functionality that X-Windows has of being
 * able to share "Windows" between processes. Thus we must create the 
 * Executive as a thread of the master process (usually the User Interface).

 * This could be extended for use as a thread on UN*X as well, but for
 * now this is written to get the native Window's port going.
 */

#include <dxconfig.h>
#include <dx/dx.h>

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
#if defined(HAVE_SYS_SELECT_H)
#include <sys/select.h>
#endif
#if defined(HAVE_SYS_SYSMP_H)
#include <sys/sysmp.h>
#endif
#if defined(HAVE_SYS_SYSTEMCFG_H)
# include <sys/systemcfg.h>
#endif
#if defined(HAVE_WINDOWS_H)
#include <Windows.h>
#endif

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
#include "dxThreadMain.h"

/* Macro definitions */

#ifndef OPEN_MAX
#define OPEN_MAX  				sysconf(_SC_OPEN_MAX)
#endif

#define MAIN_TIMING				1
#define EMESS					"Error during Initialization\n"

#define	EX_RIH_FREQUENCY		10.0 /* how often to check reg input handlers */
#define	EX_RIH_INTERVAL			1.0 / EX_RIH_FREQUENCY

#define	MAIN_LOOP_CONTINUE		{naptime = 0; goto main_loop_continue;}

#define	EX_LOOP_TRIES	1000	/* limit sizing iterations		*/
#define	EX_LOOP_PER_SEC	30	/* non-quiescent input check frequency	*/

#if TIMEOUT_DX
/* right now this only works for sgi, it's not being used now but it should
   be changed to run on all machines if you want to do a timeout            */
/* if idle for 30 minutes, kill it */
#define NAPTIME					(CLK_TCK >> 2)
#define NAPDEAD					(30 * 60 * CLK_TCK)
#endif

#if sgi || ibm6000
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

#define EXCEPTION_INTERNAL_QUIT 0x60000000
#define EXCEPTION_INTERNAL_ERROR 0xE0000000


#define	CHECK_INIT(_i, what)\
    if ((_i) != OK) InitFailed (what)

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

/* Functions defined in libdx or programatically defined */

extern "C" {

extern int   DXConnectToServer(char *host, int pport); /* from libdx/client.c */
extern void  _dxfTraceObjects(int d); /* from libdx/object.c */
extern void  _dxf_user_modules(); /* from libdx/ */
extern void  _dxf_private_modules(); /* from libdx/ */
extern Error user_cleanup(); /* from libdx/userinit.c */
extern void _dxfcleanup_mem(); /* from libdx/mem.c */
extern Error ExHostToFQDN( const char host[], char fqdn[MAXHOSTNAMELEN] );
/* from remote.c */
extern Error user_slave_cleanup(); /* from userinit.c */
extern SFILE *_dxf_ExInitServer(int); // from socket.c
extern void GetBaseConnection(FILE **fptr, char **str);
extern int _dxf_GetPhysicalProcs(); /* from libdx/memory.c */
extern HWND		DXGetWindowsHandle(const char *where);  /* from libdx/displayw.c */
}

/* Fixme------ */
/* Globals Variables */
/* Global variables are a real problem with a threaded environment.
 * As long as the thread is only created once and used once, then
 * there should be no problem. But as soon as this function is
 * called more than once in a process, then we are in for some
 * serious problems (esp statics). Since we will eventually want to
 * be able to create child threads of the Exec so that the Exec can
 * run on multiple processors, then we need to consider how to write
 * this.
 */

DXExecThread *globalExecThread = NULL;

int            _dxd_exShowBells = FALSE;
int            _dxd_exDebugConnect = FALSE;
int            _dxd_exErrorPrintLevel = 3;
int            _dxd_exRshInput = FALSE;
int            _dxd_exIntraGraphSerialize = TRUE;
int            _dxd_exRemoteUIMsg = FALSE;
int            _dxd_exSkipExecution = FALSE;
int            _dxd_exEnableDebug = TRUE;
int            _dxd_exCacheOn = TRUE;		    /* use cache */
int            _dxd_exDebug = FALSE;
int            _dxd_exRemote = FALSE;
int            _dxd_exRemoteSlave = FALSE;
int            _dxd_exParseAhead = TRUE;
PATHTAG        _dxd_pathtags = {0, 0, NULL};
DPGRAPHSTAT    _dxd_dpgraphstat = {0, 0, NULL};
DPHOSTS        *_dxd_dphosts = NULL;
lock_type      _dxd_dphostslock = 0;
PGASSIGN       _dxd_pgassign = {0, 0, NULL};
SLAVEPEERS     _dxd_slavepeers = {0, 0, NULL};
int            _dxd_exGoneMP = FALSE;           /* set true after forking */
int            _dxd_exRunningSingleProcess = TRUE; /* set with nphysprocs/nprocs */
int            _dxd_exIsatty = FALSE;
SFILE          *_dxd_exSockFD = NULL;
char           *_dxd_exHostName = NULL;
int            _dxd_exPPID = 0;		         /* parent's process id */
int            *_dxd_exTerminating = NULL;	 /* flag set when dx is exiting */
Context        *_dxd_exContext = NULL;       /* structure for context information */
int            _dxd_exMasterfd = -1;         /* slave to master file descriptor */
int            _dxd_exSlaveId = 0;           /* slave number */
int            _dxd_exSwapMsg = 0;           /* do we need to swap msg from peer? */
                                             /* startup of slave as finished, send msgs OK */
int            *_dxd_exNSlaves = NULL;		 /* number of distributed slaves */
int            *_dxd_extoplevelloop = NULL;	 /* looping at top level of graph */
long           _dxd_exMarkMask = 0;	         /* DXMarkTime enable mask	*/
EXDictionary   _dxd_exGlobalDict = NULL;
int            _dxd_exMyPID = 0;		     /* pid of the process */


/* Forward Declarations */

int _dxf_GetPhysicalProcs(void);
static int _dxf_RQ_ReadChar_fd (int, Pointer);
DWORD WINAPI ChildThread(LPVOID);
extern "C" Error _dxf_child_RQ_message(int *jobid);

/*
 * This one's used externally in DODX RunOnSlaves 
 */

extern "C" {
void ExQuit();
int DXGetPid();
}

DWORD WINAPI ExecThread(LPVOID pt) {
	DXExecThread *dx = (DXExecThread *) pt;
	dx->Start();
	return 0;
}

int DXExecThread::SetupArgs(int argc, char **argv) {

	largc = argc;
	largv = (char **) new char*[argc];
	for(int i=0; i<argc; i++) {
		largv[i] = new char[strlen(argv[i])+1];
		strcpy(largv[i], argv[i]);
	}

	lenvp = (char **) GetEnvironmentStrings();

	return OK;
}

DXExecThread::~DXExecThread() {
	if(lenvp)
		FreeEnvironmentStrings((LPTSTR)lenvp);
	if(largv) {
		for(int i=0; i<largc; i++)
			delete [] largv[i];
		delete [] largv;
	}
}

HANDLE DXExecThread::StartAsThread() {
	mainthread = CreateThread(0, 0, ExecThread, this, 0, NULL);
	return mainthread;
}

HANDLE DXExecThread::StartAsThread(int argc, char **argv) {
	SetupArgs(argc, argv);
	return StartAsThread();
}

int DXExecThread::Start(int argc, char **argv) {
	SetupArgs(argc, argv);
	return Start();
}

/* The main function to call as a thread to set up the executive */
int DXExecThread::Start()
{
    int		save_errorlevel=0;

	if(globalExecThread) {
		fprintf(stdout, "Only one main DXExec thread can be running per process.");
		return -1;
	}

	globalExecThread = this;

	/* Still need signals here on UN*X to handle
	 * something thrown at the proc level.
	 */
	try {

#if HAVE_SIGDANGER
    signal (SIGDANGER, ExSigDanger);
#endif

#if HAVE_SIGPIPE
    signal(SIGPIPE, ExSigPipe);
#endif

#if HAVE_SIGQUIT
    signal(SIGQUIT, ExSigQuit);
#endif

	/* Done getting the number of processors */

    ProcessArgs();

    _dxd_exRunningSingleProcess = (numThreads == 1);

    /* if running single-process, we don't need the overhead of locking */
    DXenable_locks( !_dxd_exRunningSingleProcess );

	/* Anytime we are going to change globabl variables with the threaded
	 * version, there is a chance of problems. 
	 * Fixme----
	 */

    /* we will turn off parse ahead when we get a sync */
    /* so save original state */
    savedParseAhead = _dxd_exParseAhead;
    if (_dxd_exRemoteSlave) {
        /* turn off messages during initialization */
        save_errorlevel = _dxd_exErrorPrintLevel;
        _dxd_exErrorPrintLevel = -1;
    } else
        Copyright (! _dxd_exRemote);

    Initialize ();		       /* perform all shared initializations */

    if(_dxd_exRemoteSlave) {
        /* turn messages back on */
        _dxd_exErrorPrintLevel = save_errorlevel;
    }

    if(logerr > 0)
        _dxf_ExLogError(logerr);

	/* Fixme----- */
	/* Some logic needs to be rewritten for threads instead of processes here. 
	 * This needs fixed from here until the end of DXMainThread
	 * Also need to worry about distributed slaves. Not taken care of yet.
	 */

	HANDLE parentEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

#if MULTIPLE_THREADS
	HANDLE *tStartup = new HANDLE[numThreads];
	HANDLE *allThreads = new HANDLE[numThreads];
	for(int i = 0; i< numThreads-1; i++) {
		DWORD tID;
		child[i].shutdown = FALSE;
		tStartup[i] = child[i].running = CreateEvent(NULL, FALSE, FALSE, NULL);
		child[i].parentReady = parentEvent;
		allThreads[i] = child[i].threadHandle = CreateThread(NULL, 0, ChildThread, this, 0, &tID);
		child[i].threadID = tID;
	}
	/* All threads have been started, wait for them all to initialize. */
	WaitForMultipleObjects(numThreads-1, tStartup, TRUE, INFINITE);
#endif

	/* Everything's initialized, now we're ready to start processing. */
	SetEvent(parentEvent);

	/* The parent is always the master. Now call the master loop */
	
	MainLoopMaster();

	/* Finish off the Windows signal processing */
	}
	catch (...) {
		SigDanger(0);
	}

	globalExecThread = NULL;
    return (0);
}


/* Process command line arguments. */
#define	VALID_ARGS				"BC:cdDE:F:H:hi:L:lmM:p:PrRsStTUuvVX"
void DXExecThread::ProcessArgs ()
{
    int		opt;
	ulong     maxMemory = 0;	/* in MB -- 0 implies library default */

    /*
     * Loop over the command line looking for arguments.
     */
    while ((opt = getopt (largc, largv, VALID_ARGS)) != EOF) {
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
                Usage();
            }
            if (nmdfs >= MDF_MAX) {
                fprintf(stdout, "too many -F files specified");
                Usage();
            }
            mdffiles[nmdfs++] = optarg;
            break;
        case 'H':
            strcpy(extesthost, optarg);
            break;
        case 'M':
            if (optarg == NULL      ||
                    ! isdigit (*optarg) ||
                    (maxMemory = atoi (optarg)) == 0) {
                fprintf (stdout,
                         "Invalid memory specification '%s'\n",
                         optarg);
                Usage();
            }
#ifndef ENABLE_LARGE_ARENAS
            {
                ulong mlim = (0x7fffffff >> 20);	/* divide by 1 meg */
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

        case 'R':
            _dxd_exRshInput = TRUE;
            break;
        case 'S':
            _dxd_exIntraGraphSerialize = FALSE;
            break;
        case 'T':
            showTrace = TRUE;
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
            Usage ();
            break;
        case 'l':
            logcache = TRUE;
            break;
        case 'm':
            markModules = TRUE;
            break;
        case 'p':
            if (optarg == NULL      ||
                    ! isdigit (*optarg)) {
                fprintf (stdout,
                         "Invalid processor specification '%s'\n",
                         optarg);
                Usage ();
					} else
						SetNumThreads(atoi(optarg));
            break;

        case 'r':
            _dxd_exRemote = TRUE;
            break;
        case 's':
            _dxd_exRemoteSlave = TRUE;
            break;
        case 't':
            showTiming = 1;
            break;
        case 'u':
            _dxd_exParseAhead = FALSE;
            break;
        case 'v':
            Version ();
            break;

        default :
            Usage();
            break;
        }
    }
}

/* Executive Initialization */
void DXExecThread::Initialize (void)
{
    int			i;
    int			n;
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

    if (numProcs <= 0) {
        numProcs = _dxf_GetPhysicalProcs();
    }

    DXProcessors (numProcs);		/* set number of processors before */
    /* initializing the library        */

    CHECK_INIT (DXinitdx (), "cannot initialize DX library");

    /* connect to server BEFORE rest of inits which can produce messages */
    ConnectInput ();

    if((_dxd_exContext = (Context *)DXAllocate(sizeof(Context))) == NULL)
        InitFailed ("can't allocate memory");
    _dxd_exContext->graphId = 0;
    _dxd_exContext->userId = 0;
    _dxd_exContext->program = NULL;
    _dxd_exContext->subp = NULL;

    if((_dxd_exHostName = (char *)DXAllocate(MAXHOSTNAMELEN)) == NULL)
        InitFailed ("can't allocate memory");
    gethostname(_dxd_exHostName, MAXHOSTNAMELEN);
    if ( ExHostToFQDN(_dxd_exHostName, _dxd_exHostName ) == ERROR )
        InitFailed ("ExHostToFQDN failed");

    /* now that lib is initialized, we can use DXMessage() if needed */
    if (numThreads <= 0 || numThreads > _dxf_GetPhysicalProcs()) {
        if(!_dxd_exRemoteSlave)
            DXUIMessage ("WARNING MSGERRUP",
                         "requested %d, on %d processors",
                         numThreads, _dxf_GetPhysicalProcs());
    }

    fflush(stdout);

    DXSetErrorExit (0);			/* don't allow error exits */
    DXEnableDebug ("0", showTrace);
    _dxfTraceObjects(0);			/* don't be verbose about objects */
    if (showTiming > 1)
        DXTraceTime (TRUE);
    DXRegisterScavenger (_dxf_ExReclaimMemory);

    if (markModules)
        _dxd_exMarkMask = 0x20;
    else {
        if ((mm = (char *) getenv ("EXMARKMASK")) != NULL)
            _dxd_exMarkMask = strtol (mm, 0, 0);
        else
            _dxd_exMarkMask = 0x3;
    }

    CHECK_INIT (_dxf_initdisk (), "cannot initialize external disk array");

    Settings();

    CHECK_INIT (_dxf_EXO_init (), "cannot initialize executive object dictionary");

    if ((exReady = (int *) DXAllocate (sizeof (int))) == NULL)
        InitFailed ("can't allocate memory");
    *exReady = FALSE;

    if ((child = (childThreadTable *)
                    DXAllocate (sizeof (childThreadTable) * numThreads + 1)) == NULL)
        InitFailed ("can't DXAllocate");
    if ((childtbl_lock = (lock_type *)DXAllocate(sizeof(lock_type))) == NULL)
        InitFailed ("can't DXAllocate");
    DXcreate_lock(childtbl_lock, "lock for child table");

    if ((_dxd_exTerminating = (int *) DXAllocate (sizeof(int))) == NULL)
        InitFailed ("can't allocate memory");
    *_dxd_exTerminating = FALSE;

    if ((_dxd_exNSlaves = (int *) DXAllocate (sizeof(int))) == NULL)
        InitFailed ("can't allocate memory");
    *_dxd_exNSlaves = 0;

    if ((_dxd_extoplevelloop = (int *) DXAllocate (sizeof(int))) == NULL)
        InitFailed ("can't allocate memory");
    *_dxd_extoplevelloop = FALSE;

    _dxd_exSlaveId = -1;
    _dxd_exSwapMsg = FALSE;
    _dxf_InitDPtableflag();

    CHECK_INIT (_dxf_ExInitTask (numProcs), "cannot initialize task structures");

    n = MAXGRAPHS;

    CHECK_INIT (_dxf_ExInitMemoryReclaim (),
                "cannot initialize memory reclaimation routines");

    /* create the run queue */
    CHECK_INIT (_dxf_ExRQInit (), "cannot initialize the run queue");

    n = MAXGRAPHS;
    /* don't allocate more graph slots than threads */
    n = (n > numThreads) ? numThreads : n;
    CHECK_INIT (_dxf_ExGQInit (n), "cannot initialize the graph queue");

    /* locks for module symbol table */
    CHECK_INIT (_dxf_ModNameTablesInit(), "cannot initialize symbol table");

    /* get root dictId before fork */
    _dxd_exGlobalDict = _dxf_ExDictionaryCreate (2048, TRUE, FALSE);

    _dxd_dphosts =
		(DPHOSTS *)DXAllocate(sizeof(DPHOSTS));
    if(_dxd_dphosts == NULL)
        InitFailed ("can't allocate memory for distributed table");
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
        InitFailed("Error loading user interactor files");

    /*
     * System variables should be set before .dxrc processing too.
     */

    _dxf_ExInitSystemVars ();
}

DWORD WINAPI ChildThread(LPVOID et) {
	DXExecThread *parent = (DXExecThread *) et;

	//Setup pip between master/slave
	//ExInitTaskPerProcess stuff
	//WaitFor ParentReadySignal
	//ExMainLoop - Slave
	return 0;
}


/* Perform all initializations necessary to run the executive. */
void DXExecThread::ConnectInput()
{
    int		port;

    if (_dxd_exRemote) {
        _dxf_ExGetSocket (NULL, &port);
        _dxd_exSockFD = _dxf_ExInitServer (port);

        if(_dxf_ExInitPacket() == ERROR)
            InitFailed ("can't make UI connection");

        if (_dxd_exSockFD == NULL)
            InitFailed ("can't make UI connection");
        _pIfd  = _dxd_exSockFD;
        _pIstr = "UI";
        _dxd_exIsatty = 0;
    } else {
        FILE *fptr;
        GetBaseConnection(&fptr, &_pIstr);
        _pIfd = FILEToSFILE(fptr);
        _dxd_exIsatty = SFILEisatty(_pIfd);
    }
}





/* Print debugging info on settings */
void DXExecThread::Settings(void)
{
    ExDebug ("*1", "intra graph serialize    %s",
             _dxd_exIntraGraphSerialize ? "ON" : "OFF");
    ExDebug ("*1", "lookaside caching        %s",
             _dxd_exCacheOn        ? "ON" : "OFF");
    ExDebug ("*1", "processors               %d", numProcs);
	ExDebug ("*1", "threads                  %d", numThreads);
    ExDebug ("*1", "execution log            %s", logcache ? "ON" : "OFF");
    ExDebug ("*1", "");
}


/* Child Thread */
/* Fixme-- should probably rename this to ExChildThread */
void DXExecThread::ChildThread ()
{
    /*
     * Wait for all the children to appear and the parent to signal OK to
     * start processing.
     
    _dxf_ExInitTaskPerProc();
*/
    /* don't send out worker messages for slaves */
/*
	if(!_dxd_exRemoteSlave)
        DXMessage ("#1060", getpid ());

    while ((nprocs > 1) && (! *exReady))
        ;

    ExMainLoop ();
*/
	}



/* Fixme-- need a comment */
void DXExecThread::MainLoop ()
{
	/*
    if (_dxd_exMyPID == 0 || nprocs == 1)
        ExMainLoopMaster ();
    else
        ExMainLoopSlave ();
*/
}

/* This is the main loop executed by a slave processor. */
void DXExecThread::MainLoopSlave ()
{
    int			ret = TRUE;
    int 		RQ_fd;
    char		c;

    set_status (PS_EXECUTIVE);

    RQ_fd = exParent_RQread_fd;

    while (! *_dxd_exTerminating) {
        ret = _dxf_ExRQPending () && CheckRunqueue (0);
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

static int ExFromMasterInputHndlr(int fd, Pointer p) {
	DXExecThread *gw = (DXExecThread *) p;
	return gw->FromMasterInputHndlr(fd, p);
}


/* This is the main loop executed by the master processor. */
void DXExecThread::MainLoopMaster (void)
{
	/* The loop is different depending upon if this is the only processor (nprocs
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
		DXRegisterInputHandler(ExFromMasterInputHndlr, fd, this);
        for (;;) {
loop_slave_continue:
            CheckTerminate ();
            if (_dxf_ExRQPending () && CheckRunqueue (0)) {
                /* always check rih so socket doesn't get blocked */
                _dxf_ExCheckRIH ();
                goto loop_slave_continue;
            }
            if(numThreads == 1)
                _dxf_ExCheckRIHBlock (_dxd_exMasterfd);
            else {
                if(_dxf_ExCheckRIH ())
                    goto loop_slave_continue;

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
        CheckTerminate ();

        if (numThreads > 1)
            ParallelMaster ();
        else
        {
            _dxf_ExCheckRIH ();
            ExMarkTimeLocal (4, "main:chrq");
            if (_dxf_ExRQPending () && CheckRunqueue (0)) {
                if (_dxd_exParseAhead)
                    CheckInput ();
                continue;
            }

            _dxf_RunPendingCmds();

            if (_dxd_exParseAhead) {
                ExMarkTimeLocal (4, "main:chin");
                if (CheckInput ())
                    continue;
            }

            ExMarkTimeLocal (4, "main:chgq");
            if (CheckGraphQueue (-1))
                continue;

            if (! _dxd_exParseAhead) {
                /* if we get here there is nothing in the queues */
                /* restore parse ahead in case it was changed */
                _dxd_exParseAhead = savedParseAhead;
                ExMarkTimeLocal (4, "main:chin");
                if (CheckInput ())
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

/* Let all of the remaining children know they are to die. */
void DXExecThread::KillChildren ()
{
  /* Fixme-- with threads we don't use signals, but must kill children somehow */
	/* Could use TerminateThread but that leaves a lot of junk around. */
	/* A much better fix is to add a thread structure to each thread that
	 * has a shutdown flag that the thread itself can check and terminate
	 * if requested. 
	 */

}

/* Fail and Exit after error message */
void DXExecThread::InitFailed (char *message)
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

	globalExecThread = NULL;
    exit (0);
}

/* Have conditions for termination been met yet? */
void DXExecThread::CheckTerminate ()
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
	for (i = 1; i < numThreads; i++) {
		child[i].shutdown = TRUE;
        write(child[i].RQwrite_fd, "a", 1);
	}

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
    Cleanup ();
    exit(0);
}

void DXExecThread::Terminate() {
	*_dxd_exTerminating = 1;
}

/* Error quit */
/* used here and sfile.c */
void ExQuit()
{

	/* To call quit, just go through the child table and set all
	 * the shutdown flags to TRUE.
	 */
	

    (*_dxd_exTerminating) = 1;

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

	if(globalExecThread)
		globalExecThread->Cleanup ();

    exit(1);
}

/* Perform any cleanups necessary to free up system resources. */
void DXExecThread::Cleanup(void)
{
    int		ok;
    int         i,limit;
    dpgraphstat *index;
    PGassign	*pgindex;
    SlavePeers  *sindex;

    user_cleanup();

    if (markModules)
        DXPrintTimes ();
    ExDebug ("*1", "in ExCleanup");

    ok = (exParent && _dxd_exMyPID==-1);

    if (! ok)
        exit (0);

    FREE_LIST(_dxd_pathtags);

    for (i = 0, limit = SIZE_LIST(_dxd_dpgraphstat); i < limit; ++i) {
        index = FETCH_LIST(_dxd_dpgraphstat, i);
        DXFree(index->prochostname);
        DXFree(index->procusername);
        if(index->options)
            DXFree(index->options);
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

	globalExecThread = NULL;
}



/* Check the run queue */
int DXExecThread::CheckRunqueue (int graphId)
{
    return (_dxf_ExRQDequeue (0));
}

/* See if there is a graph ready for execution */
int DXExecThread::CheckGraphQueue (int newGraphId)
{
    Program	*graph;
    graph = _dxf_ExGQDequeue ();

    if (graph == NULL)
        return (FALSE);

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

/* Check and see if any input is available */
int DXExecThread::InputAvailable (SFILE *fp)
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


/* Register the file descriptor input handler */
void DXExecThread::RegisterRQ_fds()
{
    DXRegisterInputHandler(_dxf_RQ_ReadChar_fd, exChParent_RQread_fd, NULL);
}

/* Need a comment here. */
void DXExecThread::ParallelMaster ()
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

    RegisterRQ_fds();

    for (;;) {
        if (++tries > limit) {
            /*
             * If this is the terminal, and the user hasn't typed anything
             * yet then prompt him.
             */

            ISSUE_PROMPT ();
            _dxf_ExCheckRIH();

            while (reading && ! *_dxd_exTerminating &&
                    (InputAvailable (yyin))
                    && _dxd_exParseAhead) {
                limit = -EX_INCREMENT;

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
                        CheckGraphQueue (-1);
                        if (_dxf_ExCheckBackground (_dxd_exGlobalDict, TRUE) ||
                                _dxf_ExCheckVCR (_dxd_exGlobalDict, TRUE))
                            CheckGraphQueue (-1);
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

        if (CheckGraphQueue (-1))
            continue;

        if (_dxf_ExGQAllDone ())
            _dxf_RunPendingCmds();

        if (_dxf_ExCheckBackground (_dxd_exGlobalDict, TRUE) ||
                _dxf_ExCheckVCR (_dxd_exGlobalDict, TRUE)) {
            if (CheckGraphQueue (-1))
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

        CheckTerminate ();

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
                _dxd_exParseAhead = savedParseAhead;
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
            if(!_dxf_ExIsExecuting() && !InputAvailable(yyin))
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

/* Need a comment here. */
Error DXExecThread::FromMasterInputHndlr (int fd, Pointer p)
{
    Program		*graph;
    DistMsg             pcktype;
    int			b, peerwait;
    int 		graphId;
    DictType		whichdict;
    int			cacheall, namelen;
    char		name[1024];
    dpversion 		dpv;

    if ((IOCTL(_dxd_exMasterfd, FIONREAD, (char *)&b) < 0) || (b <= 0)) {
        printf("Connect to Master closed\n");
        Cleanup();
        exit(0);
    }

    if(_dxf_ExReceiveBuffer(_dxd_exMasterfd, &pcktype, 1, TYPE_INT,
                            _dxd_exSwapMsg) < 0) {
        DXUIMessage("ERROR", "bad distributed packet type");
        Cleanup();
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
            Cleanup();
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
        CheckGraphQueue(graphId);
        CheckRunqueue(0);
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
        Cleanup();
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
        Cleanup();
        exit(1);
    }
    return (0);
}

/* Write out copyright notice */
void DXExecThread::Copyright (int p)
{
    if (p) {
        write (fileno (stdout), DXD_COPYRIGHT_STRING,
               strlen (DXD_COPYRIGHT_STRING));
        write (fileno (stdout), "\n", 1);
    }
}
/* Write out Usage info for dxexec */
void DXExecThread::Usage()
{
    Copyright();
    fprintf (stdout, "usage: %s ", largv[0]);
    fprintf (stdout, "[-B]");
    fprintf (stdout, "[-c]");
    fprintf (stdout, "[-d]");
    fprintf (stdout, "[-E #]");
    fprintf (stdout, "[-F file]");
    fprintf (stdout, "[-i #]");
    fprintf (stdout, "[-l]");
    fprintf (stdout, "[-m]");
    fprintf (stdout, "[-M #]");
    fprintf (stdout, "[-p #]");
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
    fprintf (stdout, "  -l    toggle logging to dx.log     (default = %s)\n",
             logcache ? "on" : "off");
    fprintf (stdout, "  -L    force a license type (runtime or develop)\n");
    fprintf (stdout, "  -m    mark module execution times\n");
    fprintf (stdout, "  -M    limit global memory\n");
    fprintf (stdout, "  -p    number of processors         (default = %d)\n",
             numThreads);
    fprintf (stdout, "  -r    turn on remote execution\n");
    fprintf (stdout, "  -R    start with rsh but not in remote mode\n");
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


/* Write out the Version info for dxexec */
void DXExecThread::Version(void)
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
    strcat(buf, ", threaded version ");
    strcat(buf, DXD_VERSION_STRING);

    strcat(buf, " (");
    strcat(buf, __TIME__);
    strcat(buf, ", ");
    strcat(buf, __DATE__);
    strcat(buf, ")\n");
    write(fileno(stdout),buf,strlen(buf));

    exit (0);
}

void DXExecThread::SigPipe(int signo)
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
#if defined(WIN32)
		RaiseException(EXCEPTION_INTERNAL_QUIT,
					EXCEPTION_NONCONTINUABLE,
					0, NULL);
#endif
    } else {
#if 0
        fprintf(stderr, "ExSigPipe: master received %d\n", signo);
#endif
        ExQuit();
    }
}

void DXExecThread::SigQuit(int signo)
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

void DXExecThread::SigDanger (int signo)
{
    DXSetError (ERROR_INTERNAL, "#8300");
    DXPrintError (NULL);
    ExQuit();
    exit(1);
}

/* Send error output to UI or stdout depending on existence of UI */
/* used here and a whole lot of other places */
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
/* Return the executables environment pointer */
char** DXExecThread::getEnvp (void)
{
    return (lenvp);
}

/* used here and command.c */
char** _dxf_ExEnvp(void) {
	return globalExecThread->getEnvp();
}

/* Initialize the prompt variables for command line exec */
void DXExecThread::InitPromptVars()
{
    PromptSet(PROMPT_ID_PROMPT,EX_PROMPT);
    PromptSet(PROMPT_ID_CPROMPT,EX_CPROMPT);
}

/* Initialize the system variables for command line exec */
/* used here and command.c */
void _dxf_ExInitSystemVars ()
{
    _dxf_ExInitVCRVars ();
    if(!_dxd_exRemoteSlave)
        globalExecThread->InitPromptVars ();
}


int DXExecThread::GetNumThreads() {
	int nphysprocs = 0;

	if (numThreads > 0)
		return numThreads;

	nphysprocs = _dxf_GetPhysicalProcs();

	if(nphysprocs > 3)
        numThreads = (int)(nphysprocs / 2);
    else
        numThreads = (nphysprocs > 1) ? 2 : 1;

	return numThreads;
}

void DXExecThread::SetNumThreads(int t) {
	if(t < 1)
		numThreads = 0;

	numThreads = t;
}

/* Lock the child pid table */
void DXExecThread::LockChildpidtbl()
{
    DXlock(childtbl_lock, 0);
}

/* Read character from run queue file descriptor */
/* used here and elsewhere */
static int _dxf_RQ_ReadChar_fd (int fd, Pointer p)
{
    int ret;
    char c;

    ret = read(fd, &c, 1);
    if(ret != 1) {
        /*
               DXMessage("Error reading from request queue pipe: %d %d", errno, fd);
        */
        DXRegisterInputHandler(NULL, fd, NULL);
    }
    return 0;
}

/* update child in child table */
void DXExecThread::UpdateChildpid(int i, int tid, int writefd)
{
    /* table should already be locked before this call */

#if DEBUGMP
    DXMessage("child %d, writefd %d", i, writefd);
#endif

    child[i].threadID = tid;
    child[i].RQwrite_fd = writefd;
    DXunlock(childtbl_lock, 0);
}

/* Need comment */
void DXExecThread::SetRQReadFromChild1(int readfd)
{
    exChParent_RQread_fd = readfd;
}

/*
 * Set up fd that slaves block on waiting for the master to
 * put work in the run queue
 */
void DXExecThread::SetRQReader(int fd)
{
    exParent_RQread_fd = fd;
    DXRegisterInputHandler(_dxf_RQ_ReadChar_fd, fd, NULL);
}

/* On slave #1 set up fd for writing to the master */
void DXExecThread::SetRQWriter(int fd)
{
    exParent_RQwrite_fd = fd;
}

Error _dxf_parent_RQ_message() {
	return globalExecThread->ParentRQMessage();
}

/* Notify parent of run queue request error */ 
Error DXExecThread::ParentRQMessage()
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

/* used here and elsewhere */
Error _dxf_child_RQ_message(int *jobid) {
	return globalExecThread->ChildRQMessage(jobid);
}

/* send child a message about work on the run queue */
Error DXExecThread::ChildRQMessage(int *jobid)
{
	/* Send a message to the child so they know there is work on the queue. */
	/* If there is a jobid then only notify the child that the job is for,  */
	/* otherwise notify all children and the first to get the job wins.     */
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

        ret = write(child[procid].RQwrite_fd, "a", 1);
        if(ret != 1)
            _dxf_ExDie("Write Erroring notifying child of job request, write returns %d, error number %d", ret, errno);
#if DEBUGMP

        else
            DXMessage("successful write to %d", children[procid].RQwrite_fd);
#endif

    } else { /* if procid == -1 then send message to all children */
        for (i = 1; i < numThreads; i++) {
#if DEBUGMP
            DXMessage("send job request to %d, %d", i, children[i].RQwrite_fd);
#endif

            ret = write(child[i].RQwrite_fd, "a", 1);
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

/* Check buffer first then read */
/* only used here */
int DXExecThread::OKToRead (SFILE *fp)
{
    if (ExCheckParseBuffer())
        return 1;

    return SFILECharReady(fp);
}

/* Set the exec prompt */
void DXExecThread::PromptSet(char *var, char *val)
{
    gvar	*gv;
    String	pmpt;

    pmpt = DXNewString(val);
    gv = _dxf_ExCreateGvar (GV_DEFINED);
    _dxf_ExDefineGvar (gv, (Object)pmpt);
    _dxf_ExVariableInsert (var, _dxd_exGlobalDict, (EXObj)gv);
}


/* Return what the prompt is set to */
/* used here and lex.c */
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


/* The following is some stuff that should be useful */
/* only here */
int DXExecThread::MyChildProc()
{
	/*
    int i;
    static int iCount = 0;
    static __declspec(thread)  int tls_td;

    iCount++;

    tls_td = GetCurrentThreadId();

    _dxf_lock_childpidtbl();
    _dxf_update_childpid(iCount, tls_td, -1);

    i = tls_td;
    _dxd_exMyPID = FindPID (DXGetPid ());
    i = _dxd_exMyPID;

    printf("In New child Thread   %d \n",tls_td);

    ChildThread();

    /*
    _dxf_ExInitTaskPerProc();
    ExMainLoopSlave ();

    printf("End Thread   %d \n",tls_td);
    */
    return 1;
}

int DXExecThread::DXWinFork()
{
/*
	int i;

    i = _beginthread(MyChildProc, 0, NULL);
    return i;
*/
	return (-1);
}

/* here and lock.c */
int DXGetPid()
{
    int i;
    i = GetCurrentThreadId();
    return i;
}


int DXExecThread::CheckInput ()
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
 * Processors are identified by their index into the table of child
 * processes.
 */
int DXExecThread::FindPID (int pid)
{
    int i;
	/*
    for (i = 0; i < nprocs; i++)
        if (pid == children[i].pid)
            return (i);

    DXWarning ("#4510", pid);
	*/
	return (-1);

}

HWND DXExecThread::GetImageWindowHandle(const char *name) {
	return DXGetWindowsHandle(name);
}
