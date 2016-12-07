/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>



/* on an MP machine, this file defines the lock routines.  on all other
 * machines, there are stubs at the end of this file which do nothing,
 * so this file doesn't need to be edited for single processor architectures.
 */
#include <dx/dx.h>


#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#include <stdlib.h>

#ifdef DEBUGGED
#    define STATUS_LIGHTS 1
#endif
#ifdef STATUS_LIGHTS
#include "../dpexec/status.h"
#include "../dpexec/utils.h"
#endif


static int _dxf_locks_enabled = 1;      /*  Enabled, by default  */

void DXenable_locks(int enable)
{
    char *force_locks;

    if ( (force_locks = getenv( "DX_FORCE_LOCKS" )) != NULL ) {
	if ( !force_locks[0] )
	    _dxf_locks_enabled = 1;
	else
	    _dxf_locks_enabled = atoi(force_locks);
    }
    else if ( enable >= 0 )               /*  -1 only checks env var  */
        _dxf_locks_enabled = enable;
}

#if alphax
#define lockcode

#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/mman.h>

int _dxf_initlocks(void)
{
    DXenable_locks(-1);
    return OK;
}

int DXcreate_lock(lock_type *l, char *name)
{
    static int been_here = 0;
    if (!been_here) {
	been_here = 1;
	if (!_dxf_initlocks())
	    return NULL;
    }

    if(!msem_init(l, MSEM_UNLOCKED))
        DXErrorReturn(ERROR_INTERNAL, "Error creating lock");

    return OK;
}

int DXdestroy_lock(lock_type *l)
{
    if(DXtry_lock(l, 0)) {
       msem_remove(l);
       return OK;
    }
    else
       DXErrorReturn(ERROR_INTERNAL, "attempt to destroy locked lock");
}

int DXlock(lock_type *l, int who)
{
    if ( !_dxf_locks_enabled ) 
	return OK;

    if(msem_lock(l, 0) != 0)
        DXErrorReturn(ERROR_INTERNAL, "Failed attempt to lock lock");

    return OK;
}

int DXtry_lock(lock_type *l, int who)
{
    if ( !_dxf_locks_enabled ) 
	return OK;

    if(msem_lock(l, MSEM_UNLOCKED) != 0)
        return ERROR;

    return OK;
}

int DXunlock(lock_type *l, int who)
{
    if ( !_dxf_locks_enabled ) 
	return OK;

    if(msem_unlock(l, 0) != 0)
        DXErrorReturn(ERROR_INTERNAL, "Failed to unlock lock");
    return OK;
}

int DXfetch_and_add(int *p, int value, lock_type *l, int who)
{
    int old_value;

    DXlock(l, who);
    old_value = *p;
    *p += value;
    DXunlock(l, who);

    return old_value;
}
#endif

/*
 * Locks using Sun's MUTEX
 */
#if DXD_USE_MUTEX_LOCKS && !defined(lockcode)

#define lockcode

#include <synch.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

/*
 * Strangely, with the mutex locks, we've got to replace sleep().
 * Sleep in libthreads.a from Solaris 2.3 doesn't work if you replace
 * the signal handler.  It exits the second time it's called.  The
 * following program quits when compiled cc -o x x.c -lthreads:
 * 
 * #include <unistd.h>
 * #include <stdlib.h>
 * #include <signal.h>
 * 
 * #define SLEEP_LOCKED 1
 * 
 * void main()
 * {
 *     int i;
 *     void (*s)(int) = SIG_IGN;
 * 
 *     for (i = 0; i < 10; ++i) {
 * 	s = signal(SIGALRM, SIG_IGN);
 * 	printf("signal returns 0x%08x\n", s);
 * 	signal(SIGALRM, s);
 * 	printf("sleep(%d)\n", SLEEP_LOCKED);
 * 	sleep(SLEEP_LOCKED);
 * 	printf("finish sleep(%d)\n", SLEEP_LOCKED);
 *     }
 *     printf("exiting thrsig\n");
 * }
 */
static void sleeper(int sig)
{
}
uint sleep(uint secs)
{
    void (*s)(int);

    if (secs == 0)
	return 0;
    s = signal(SIGALRM, sleeper);
    alarm(secs);
    pause();
    signal(SIGALRM, s);

    return 0;
}
    

int _dxf_initlocks(void)
{
    DXenable_locks(-1);
    return OK;
}


int DXcreate_lock(lock_type *l, char *name)
{
    static int been_here = 0;
    lwp_mutex_t *mp = (lwp_mutex_t*) l;
    lwp_mutex_t template = SHAREDMUTEX;

    if (!been_here) {
	been_here = 1;
	if (!_dxf_initlocks())
	    return NULL;
    }

    *mp = template;
    return OK;
}

int DXdestroy_lock(lock_type *l)
{
    return OK;
}

static void wakeupAndSleep(int thing)
{
    signal(SIGALRM, wakeupAndSleep);
}

int DXlock(lock_type *l, int who)
{
    lwp_mutex_t *mp = (lwp_mutex_t*) l;
    int sts;
    struct itimerval tv;
    struct itimerval otv;
    void (*oldSignal)(int);

    if ( !_dxf_locks_enabled ) 
	return OK;

    if ((sts = _lwp_mutex_trylock(mp)) != 0) {
	if (sts == EBUSY) {
#ifdef STATUS_LIGHTS
	    int old_status = get_status();
	    set_status(PS_DEAD);
#endif
	    oldSignal = signal(SIGALRM, wakeupAndSleep);
	    tv.it_value.tv_sec = 1;
	    tv.it_value.tv_usec = 0;
	    tv.it_interval.tv_sec = 1;
	    tv.it_interval.tv_usec = 0;
	    setitimer(ITIMER_REAL, &tv, &otv);
	    if ((sts = _lwp_mutex_lock(mp)) != 0) {
		/* Cancel timer */
		setitimer(ITIMER_REAL, &otv, NULL);
		signal(SIGALRM, oldSignal);

		DXSetError(ERROR_UNEXPECTED, strerror(sts));
#ifdef STATUS_LIGHTS
		set_status(old_status);
#endif
		return ERROR;
	    }
	    else {
		/* Cancel timer */
		setitimer(ITIMER_REAL, &otv, NULL);
		signal(SIGALRM, oldSignal);
#ifdef STATUS_LIGHTS
		set_status(old_status);
#endif
	    }
	}
	else {
	    DXSetError(ERROR_UNEXPECTED, strerror(sts));
	    return ERROR;
	}
    }
    return OK;
}

int DXtry_lock(lock_type *l, int who)
{
    lwp_mutex_t *mp = (lwp_mutex_t*) l;
    int sts;

    if ( !_dxf_locks_enabled ) 
	return OK;

    if ((sts = _lwp_mutex_trylock(mp)) != 0) {
	if (sts != EBUSY) {
	    DXSetError(ERROR_UNEXPECTED, strerror(sts));
	}
	return ERROR;
    }
    return OK;
}

int DXunlock(lock_type *l, int who)
{
    lwp_mutex_t *mp = (lwp_mutex_t*) l;
    int sts;

    if ( !_dxf_locks_enabled ) 
	return OK;

    if ((sts = _lwp_mutex_unlock(mp)) != 0) {
	if (sts != EBUSY) {
	    DXSetError(ERROR_UNEXPECTED, strerror(sts));
	}
	return ERROR;
    }
    return OK;
}

int DXfetch_and_add(int *p, int value, lock_type *l, int who)
{
    int old_value;

    DXlock(l, who);
    old_value = *p;
    *p += value;
    DXunlock(l, who);

    return old_value;
}

#endif
/*
 * Real locks for PVS using OS locks
 */

#if ibmpvs && !defined(lockcode)

#include <sys/svs.h>

#define lockcode

#define LOCKED 0
#define UNLOCKED 1
#define WHO (who+1)


Error _dxf_initlocks(void)
{
    DXenable_locks(-1);
    return OK;
}

int DXcreate_lock(lock_type *l, char *name)
{
    static int been_here = 0;
    if (!been_here) {
	been_here = 1;
	if (!_dxf_initlocks())
	    return NULL;
    }

    simple_lock_init(l);
    return OK;
}

int DXdestroy_lock(lock_type *l)
{
    return OK;
}

int DXlock(lock_type *l, int who)
{
    if ( !_dxf_locks_enabled ) 
	return OK;

#if DEBUGGED
    int v;
    simple_lock(l, WHO);
    if ((v= *l) != WHO) {
	DXWarning("DXlock %x didn't take (expected %d, saw %d)",l,WHO,v);
    }
    return OK;
#else
    simple_lock(l, WHO);
    return OK;
#endif
}

int DXtry_lock(lock_type *l, int who)
{
    if ( !_dxf_locks_enabled ) 
	return OK;

#if DEBUGGED
    int v;
    if (simple_try_lock(l, WHO)) {
	if ((v= *l) != WHO) {
	    DXWarning("DXtry_lock %x didn't take (expected %d, saw %d)", l,WHO,v);
	}
	return OK;
    } else
	return ERROR;
#else
    return simple_try_lock(l, WHO);
#endif
}


int DXunlock(lock_type *l, int who)
{
    if ( !_dxf_locks_enabled ) 
	return OK;

#if DEBUGGED
    if (*l != WHO) {
	DXWarning("invalid DXunlock of %x (expected %d, saw %d)", l, WHO, *l);
    }
#endif
    simple_unlock(l);
    return OK;
}

int DXfetch_and_add(int *p, int value, lock_type *l, int who)
{
    return fetch_and_add(p, value);
}

#endif /* ibmpvs */



/*
 * real MP locks for '6000, for the SMP
 */

#if ibm6000 && !defined (lockcode)

#define lockcode

#define LOCKED    0
#define UNLOCKED  1
#define DEADLOCK -1

int _dxf_initlocks(void)
{
    DXenable_locks(-1);
    return OK;
}

int DXcreate_lock(lock_type *l, char *name)
{
    static int been_here = 0;
    if (!been_here) {
	been_here = 1;
	if (!_dxf_initlocks())
	    return NULL;
    }

    *l = UNLOCKED;
    return OK;
}

int DXdestroy_lock(lock_type *l)
{
    if (*l == LOCKED)
       DXErrorReturn(ERROR_INTERNAL, "attempt to destroy locked lock");
    if (*l == DEADLOCK)
       DXErrorReturn(ERROR_INTERNAL, "attempt to destroy destroyed lock");
    *l = DEADLOCK;
    return OK;
}

int DXlock(lock_type *l, int who)
{
    if ( !_dxf_locks_enabled ) 
	return OK;

    while (cs(l, UNLOCKED, LOCKED)) {
	int v = *l;
	if (v!=UNLOCKED && v!=LOCKED)
	    DXErrorReturn(ERROR_INTERNAL, "bad lock");
	usleep(500 /* microseconds */);    /* spin loop waiting for lock */
    }
    return OK;
}

int DXtry_lock(lock_type *l, int who)
{
    if ( !_dxf_locks_enabled ) 
	return OK;

    if (cs(l, UNLOCKED, LOCKED))
       return ERROR;
    else
       return OK;
}

int DXunlock(lock_type *l, int who)
{
    if ( !_dxf_locks_enabled ) 
	return OK;

    if (*l == DEADLOCK)
       DXErrorReturn(ERROR_INTERNAL, "attempt to unlock destroyed lock");
    *l = UNLOCKED;
    return OK;
}

int DXfetch_and_add(int *p, int value, lock_type *l, int who)
{
    int old_value;

    DXlock(l, who);
    old_value = *p;
    *p += value;
    DXunlock(l, who);

    return old_value;
}

#endif



/*
 * Real locks for sgi MP
 */

#if sgi && !defined(lockcode)

#define lockcode

#if !OLDLOCKS

#include <ulocks.h>

static usptr_t *usptr = NULL;
#define L (*(ulock_t*)l)

/* a pool of real locks */
#define N 16
ulock_t pool[N];

/* manipulating a software lock */
#define CONS(pool,value)  (((pool)<<8) | 0xE | (value))
#define POOL(l)	  	  (((*l)>>8)&0xFF)
#define LOCKPOOL(l)	  ussetlock(pool[POOL(l)])
#define UNLOCKPOOL(l)	  usunsetlock(pool[POOL(l)])
#define LOCK(l)		  ((*l) |= 1)
#define UNLOCK(l)	  ((*l) &= ~1)
#define LOCKED(l)	  ((*l) & 1)
#define VALID(l)	  ((((*l)&~0xFFFF)==0) && (((*l)&0xE)==0xE))

int _dxf_initlocks(void)
{
    static int been_here = 0;
    static char tmp[] = "/tmp/loXXXXXX";
    char *mktemp();
    int i;

    DXenable_locks(-1);

    if (!been_here) {
	been_here = 1;
	mktemp(tmp);
	usconfig(CONF_INITUSERS, 64);
	usptr = usinit(tmp);
	if (!usptr)
	    DXErrorReturn(ERROR_NO_MEMORY, "Can't create us block for locks");
	unlink(tmp);

	/* create a pool of N real locks */
	for (i=0; i<N; i++) {
	    pool[i] = usnewlock(usptr);
	    if (!pool[i])
		DXErrorReturn(ERROR_NO_MEMORY, "can't create lock pool");
	}
    }

    return OK;
}

int DXcreate_lock(lock_type *l, char *name)
{
    static int been_here = 0;
    int i;

    if (!been_here) {
	been_here = 1;
	if (!_dxf_initlocks())
	    return NULL;
    }

    i = ((((long)(l))>>4) ^ (((long)(l))>>12) ^ (((long)(l))>>20)) & (N-1);
    /*DXMessage("new lock off of pool id %d", i);*/
    *l = CONS(i, 0);

    return OK;
}

int DXdestroy_lock(lock_type *l)
{
    *l = -1;
    return OK;
}

int DXlock(lock_type *l, int who)
{
    if ( !_dxf_locks_enabled )
	return OK;

    if (!VALID(l))
	DXErrorReturn(ERROR_BAD_PARAMETER, "bad lock");
    for (;;) {
	while (LOCKED(l))
	    sginap(0);
	LOCKPOOL(l);
	if (LOCKED(l))
	    UNLOCKPOOL(l);
	else
	    break;
    }
    LOCK(l);
    UNLOCKPOOL(l);
    return OK;
}

int DXtry_lock(lock_type *l, int who)
{
    if ( !_dxf_locks_enabled )
	return OK;

    if (!VALID(l))
	DXErrorReturn(ERROR_BAD_PARAMETER, "bad lock");
    if (LOCKED(l))
	return ERROR;
    LOCKPOOL(l);
    if (LOCKED(l)) {
	UNLOCKPOOL(l);
	return ERROR;
    }
    LOCK(l);
    UNLOCKPOOL(l);
    return OK;
}

int DXunlock(lock_type *l, int who)
{
    if ( !_dxf_locks_enabled ) 
	return OK;

    if (!VALID(l))
	DXErrorReturn(ERROR_BAD_PARAMETER, "bad lock");
    LOCKPOOL(l);
    UNLOCK(l);
    UNLOCKPOOL(l);
    return OK;
}

int DXfetch_and_add(int *p, int value, lock_type *l, int who)
{
    int old_value;

    DXlock(l, who);
    old_value = *p;
    *p += value;
    DXunlock(l, who);

    return old_value;
}

#else /* !NEWLOCKS */

#include <ulocks.h>

static usptr_t *usptr = NULL;
#define L (*(ulock_t*)l)

int _dxf_initlocks(void)
{
    static int been_here = 0;
    static char tmp[] = "/tmp/loXXXXXX";
    char *mktemp();

    if (!been_here) {
	been_here = 1;
	DXenable_locks(-1);
	mktemp(tmp);
	usconfig(CONF_INITUSERS, 64);
	usptr = usinit(tmp);
	if (!usptr)
	    DXErrorReturn(ERROR_NO_MEMORY, "Can't create us block for locks");
	unlink(tmp);
    }
    return OK;
}

int DXcreate_lock(lock_type *l, char *name)
{
    static int been_here = 0;

    if (!been_here) {
	been_here = 1;
	if (!_dxf_initlocks())
	    return NULL;
    }

    L = usnewlock(usptr);
    if (!L)
	return NULL;
    return OK;
}

int DXdestroy_lock(lock_type *l)
{
    if (!DXtry_lock(l, 0))
	DXErrorReturn(ERROR_INTERNAL, "Attempt to destroy a locked lock");
    usfreelock(L, usptr);
    return OK;
}

int DXlock(lock_type *l, int who)
{
    if ( !_dxf_locks_enabled ) 
	return OK;

    ussetlock(L);
    return OK;
}

int DXtry_lock(lock_type *l, int who)
{
    if ( !_dxf_locks_enabled ) 
	return OK;

    return uscsetlock(L, _USDEFSPIN);
}

int DXunlock(lock_type *l, int who)
{
    if ( !_dxf_locks_enabled ) 
	return OK;

    usunsetlock(L);
    return OK;
}

int DXfetch_and_add(int *p, int value, lock_type *l, int who)
{
    int old_value;

    DXlock(l, who);
    old_value = *p;
    *p += value;
    DXunlock(l, who);

    return old_value;
}

#endif /* NEWLOCKS */
#endif /* sgi */

/*
 * Real locks for linux IF SMP ENABLED
 */

#if defined(linux) && (ENABLE_SMP_LINUX == 1)

#define lockcode

#include "plock.h"
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/shm.h>

struct _lock
{
    int knt;	/* If its locked, the number of waiters + 1.  Else 0 */
    int wait;
};

#define N_INIT_LOCKS  2048
struct _lock *_initLocks;
static int _nLocks = 0;

static int initted = 0;
static int locklock;
static int lock_shmid;

#define SHM_FLAGS        (IPC_CREAT | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)

int
_dxf_initlocks(void)
{
    if ( !_dxf_locks_enabled )
	return OK;

    if (!  PLockInit())
        abort();

    locklock = PLockAllocateLock(0);
    if (locklock < 0)
         abort();

    lock_shmid = shmget(IPC_PRIVATE, N_INIT_LOCKS*sizeof(struct _lock), SHM_FLAGS);
    if (lock_shmid == 0)
    {
        perror("error allocating shared memory segment for dx locks");
	abort();
    }

    _initLocks = (struct _lock *)shmat(lock_shmid, 0, 0);
    if (_initLocks == (struct _lock *)-1)
    {
        perror("error attaching shared memory segment for dx locks");
	abort();
    }

    shmctl(lock_shmid, IPC_RMID, 0);

    initted = 1;
    return OK;
}

int 
DXcreate_lock(lock_type *l, char *name)
{
    struct _lock *_lock;

    if ( !_dxf_locks_enabled )
	return OK;

    if (! initted)
        if (! _dxf_initlocks())
	    return 0;

    if (_nLocks < N_INIT_LOCKS)
        _lock = _initLocks + _nLocks++;
    else
    {
	_lock = (struct _lock *)DXAllocate(sizeof(struct _lock));
	if (! _lock)
	    return 0;
    }
    
    _lock->knt = 0;
    _lock->wait = -1;

    *l = (lock_type)_lock;
    return OK;
}

int
DXdestroy_lock(lock_type *l)
{
    struct _lock *_lock = *(struct _lock **)l;
    int n;

    if ( !_dxf_locks_enabled )
	return OK;

    if (! initted)
    {
	DXSetError(ERROR_INTERNAL, "trying to destroy a lock before initting locks?");
	return ERROR;
    }

    if (_lock->knt)
    {
	DXSetError(ERROR_INTERNAL, "trying to destroy a locked lock");
	return ERROR;
    }

    if (_lock->wait > 0)
    {
	DXSetError(ERROR_INTERNAL, 
		"why is there a wait associated with a lock thats being destroyed?");
	return ERROR;
    }

    n = _lock - _initLocks;
    if (n < 0 || n > N_INIT_LOCKS)
	DXFree((Pointer)*l);

    return OK;
}

int
DXlock(lock_type *l, int who)
{
    struct _lock *_lock = *(struct _lock **)l;

    if ( !_dxf_locks_enabled )
	return OK;

    if (! initted)
        if (! _dxf_initlocks())
	    return 0;

    PLockLock(locklock);

    /*
     * Indicate my interest
     */
    _lock->knt ++;

    /*
     * If its already locked...
     */
    if (_lock->knt > 1)
    {
        if (_lock->wait == -1)
	    _lock->wait = PLockAllocateWait();

	/*
	 * Wait.
	 */
	PLockWait(_lock->wait, locklock);
    }

    /*
     * Now if noone is waiting and a wait is allocated, free the wait
     */
    if (_lock->knt == 1 && _lock->wait != -1)
    {
	PLockFreeWait(_lock->wait);
	_lock->wait = -1;
    }

    PLockUnlock(locklock);
    return OK;
}

int
DXunlock(lock_type *l, int who)
{
    struct _lock *_lock = *(struct _lock **)l;

    if ( !_dxf_locks_enabled )
	return OK;

    PLockLock(locklock);

    if (_lock->knt == 0)
    {
        DXSetError(ERROR_INTERNAL, "Unlocking an unlocked lock?");
	return;
    }

    /*
     * Release my count
     */
    _lock->knt --;

    /*
     * If there's anyone waiting, kick them.  If not, and there's a 
     * wait allocated, free it.
     */
    if (_lock->knt)
        PLockSignal(_lock->wait);

    PLockUnlock(locklock);

    return OK;
}

int
DXtry_lock(lock_type *l, int who)
{
    int r;
    struct _lock *_lock = *(struct _lock **)l;

    if ( !_dxf_locks_enabled )
	return OK;

    PLockLock(locklock);

    if (_lock->knt == 0)
    {
        _lock->knt ++;
	r = OK;
    }
    else
        r = ERROR;

    PLockUnlock(locklock);
    return r;
}

int
DXfetch_and_add(int *p, int value, lock_type *l, int who)
{
    int old_value;

    if (! initted)
        _dxf_initlocks();

    DXlock(l, who);
    old_value = *p;
    *p += value;
    DXunlock(l, who);

    return old_value;
}

#endif /* linux */


/*
 * fake locks for UP machines
 */


#if defined(DX_NATIVE_WINDOWS)

#define lockcode
#include <windows.h>

#define LOCKED 0
#define UNLOCKED 1

static int find_lock = 10;

int _dxf_initlocks(void)

{
    return OK;
}

static void 

DXFoundLock()

{

}

int DXcreate_lock(lock_type *l, char *name)
{
    *l = (lock_type)CreateMutex(NULL, FALSE, NULL);
	if (*(int *)l == find_lock)
		DXFoundLock();
    return OK;
}

int DXdestroy_lock(lock_type *l)
{
    if (WaitForSingleObject((HANDLE)*l, 0) != WAIT_OBJECT_0)
		DXErrorReturn(ERROR_INTERNAL, "attempt to destroy locked lock");
	ReleaseMutex((HANDLE)*l);
	//CloseHandle((HANDLE)*l);
    return OK;
}

int DXlock(lock_type *l, int who)
{
	if (*(int *)l == find_lock)
		DXFoundLock();

	if (DXWaitForSignal(1, (HANDLE *)l) == WAIT_TIMEOUT)
		return ERROR;
    return OK;
}

int DXtry_lock(lock_type *l, int who)
{
	if (*(int *)l == find_lock)
		DXFoundLock();

    if (WaitForSingleObject((HANDLE)*l, 0) == WAIT_OBJECT_0)
		return OK;
	else
		return ERROR;
}

int DXunlock(lock_type *l, int who)
{
	if (*(int *)l == find_lock)
		DXFoundLock();

	ReleaseMutex((HANDLE)*l);
    return OK;
}

int DXfetch_and_add(int *p, int value, lock_type *l, int who)
{
    int old_value;

    DXlock(l, who);
    old_value = *p;
    *p += value;
    DXunlock(l, who);

    return old_value;
}

#endif /* DX_NATIVE_WINDOWS */


#if !defined(lockcode)

#define LOCKED 0
#define UNLOCKED 1

int _dxf_initlocks(void)
{
    DXenable_locks(-1);
    return OK;
}

int DXcreate_lock(lock_type *l, char *name)
{
    static int been_here = 0;
    if (!been_here) {
	been_here = 1;
	if (!_dxf_initlocks())
	    return 0;
    }

    *l = UNLOCKED;
    return OK;
}

int DXdestroy_lock(lock_type *l)
{
    if (*l==LOCKED)
	DXErrorReturn(ERROR_INTERNAL, "attempt to destroy locked lock");
    return OK;
}

int DXlock(lock_type *l, int who)
{
    if ( !_dxf_locks_enabled ) 
	return OK;

    if (*l==LOCKED)
	DXErrorReturn(ERROR_INTERNAL, "attempt to lock locked lock");
    *l = LOCKED;
    return OK;
}

int DXtry_lock(lock_type *l, int who)
{
    if ( !_dxf_locks_enabled ) 
	return OK;

    if (*l==UNLOCKED){
	*l = LOCKED;
	return OK;
    }
    else
	return ERROR;
}

int DXunlock(lock_type *l, int who)
{
    if ( !_dxf_locks_enabled ) 
	return OK;

    *l = UNLOCKED;
    return OK;
}

int DXfetch_and_add(int *p, int value, lock_type *l, int who)
{
    int old_value;

    DXlock(l, who);
    old_value = *p;
    *p += value;
    DXunlock(l, who);

    return old_value;
}

#endif





#if defined(DXD_WIN_SMP)  && defined(THIS_IS_COMMENTED)

/*
	Define this to enable DXD_WIN    SMP . 
	This locking is not well tested



#define lockcode   1
*/




#define LOCKED 1
#define UNLOCKED 2

#define	DX_MAX_LOCKS	128


struct DXWinLock
{
    int     ISLocked;
    int     ByWhom;
    int    *dxLockPtr;
    int     ThreadID;
    HANDLE  thisMutex, thisEvent;
} ;

static struct DXWinLock  DXLocks[DX_MAX_LOCKS];

HANDLE	evLock, evUnlock, evHandle[10];

static int GetLockID(int *l)
{
     int i, id, tid;

    id = -1;
    tid = DXGetPid();
    for(i = 0; i < DX_MAX_LOCKS; i++)
    {
	if (DXLocks[i].dxLockPtr == l)
	    return i;
    }
    return -1;
}
static int IsLocked(int *l)
{
     int i;

    i = GetLockID(l);
    if(i == -1)
	    return 0;
    else
	    return 1;

    if (DXLocks[i].dxLockPtr == l)
    {
	if(DXLocks[i].ISLocked == LOCKED)
	    return 1;
	else
	    return 0;
    }
    printf("Error in IsLock() %c \n", 7);
    return 0;
}
static int GetFreeLock(int *l)
{
    int i;
    static int IamHere = 0;

    i = DXGetPid();
    while(IamHere)
	Sleep(i);

    IamHere = 1;
    for(i = 0; i < DX_MAX_LOCKS; i++) {
    	if (DXLocks[i].dxLockPtr == NULL) {
    	    DXLocks[i].dxLockPtr = l;
    	    IamHere = 0;
    	    return i;
    	}
    }
    //  DXErrorReturn(ERROR_INTERNAL, "DX_MAX_LOCKS reached ");
    IamHere = 0;
    return -1;

}

int _dxf_initlocks(void)
{
    static int been_here = 0;
    int i;

    if(!been_here)
    {
    	been_here = 1;
	DXenable_locks(-1);
    	for(i = 0; i < DX_MAX_LOCKS; i++) {
    	    DXLocks[i].ISLocked = UNLOCKED;
    	    DXLocks[i].ByWhom = -1;
    	    DXLocks[i].dxLockPtr = NULL;
    	    DXLocks[i].thisMutex = NULL;
    	    DXLocks[i].thisMutex = CreateMutex(NULL, FALSE, NULL);
    	    DXLocks[i].thisEvent = CreateEvent(NULL, 0, 1000, NULL);
    	    SetEvent(DXLocks[i].thisEvent);
    	}
	evLock = CreateEvent(NULL, 0, 1000, NULL);
	evUnlock = CreateEvent(NULL, 0, 1000, NULL);

	SetEvent(evLock);
	SetEvent(ev1c UnlLock);

	for (i = 0; i < 10; i++) {
	    evHandle[i] = CreateEvent(NULL, 0, 1000, NULL);
	    SetEvent(evHandle[i]);
	}
    }

    return OK;
}

int DXdestroy_lock(lock_type *l)
{
    //	if (*l==LOCKED)
//		DXErrorReturn(ERROR_INTERNAL, "attempt to destroy locked lock");
    return OK;
}

int DXtry_lock(lock_type *l, int who)
{
     int id;

    if ( !_dxf_locks_enabled ) 
	return OK;

    /*  if (*l==UNLOCKED){   */
    if(!IsLocked(l)) {
	/*  *l = LOCKED;   */
	DXlock(l, who);
	return OK;
    }
    else
	return ERROR;
}
int DXcreate_lock(lock_type *l, char *name)
{
    int i;
     static  int IamHere = 0;

    static int been_here = 0;
    if (!been_here) {
	been_here = 1;
	if (!_dxf_initlocks())
	    return NULL;

    }

    /*  FIXME: Add locking  */
    return OK;

    /*
    tid = DXGetPid();
    while(IamHere)
	 Sleep(tid);

    IamHere = 1;

    i = GetFreeLock();
    if(i >= 0)
    {
	DXLocks[i].ISLocked = UNLOCKED;
	DXLocks[i].dxLockPtr = l;
    }
    else
    {
	printf("Error in DXcreate_lock()  \n");
	return ERROR;
    }
    *l = UNLOCKED;
    IamHere = 0;
    return OK;
    */
}

int DXlock(int *l, int who)
{
    int tid, id, j, i;
    static int IamHere = 0;

    if ( !_dxf_locks_enabled ) 
	return OK;

    /*
    i = DXGetPid();
    while(IamHere)
	    Sleep(i);
    */
    Sleep(i);
    WaitForSingleObject(evLock, i);
    ResetEvent(evLock);

    tid = IamHere = DXGetPid();

    id = GetLockID(l);
    while (IsLocked(l))
    {
	 /*  locked by myself     */
    	if(tid == DXLocks[id].ThreadID)
    	{
    	    printf("RELOCK COND unlocking %d %d  %c \n", who, *l, 7);
    	    IamHere = 0;
    	    return OK;
    	}

    	/*  locked by someone else     WAIT   */
    	i = WaitForSingleObject(DXLocks[id].thisEvent, tid*10);
    }

    id = GetFreeLock(l);

    ResetEvent(DXLocks[id].thisEvent);

    DXLocks[id].ThreadID = tid;
    DXLocks[id].ByWhom = who;
    DXLocks[id].ISLocked = LOCKED;
    DXLocks[id].dxLockPtr = l;

    *l = LOCKED;

    IamHere = 0;
    return OK;

    
}


int DXunlock(int *l, int who)
{
    int id, tid;
    static short IamHere = 0;

    if ( !_dxf_locks_enabled ) 
	return OK;

    tid = DXGetPid();
    while(IamHere)
	 Sleep(tid);

    tid = IamHere = DXGetPid();;


    id = GetLockID(l);
    if(id < 0) {
    	printf("Error in Unlock %c \n", 7);
    	IamHere = 0;
    	return ERROR;
    }

    if(tid == DXLocks[id].ThreadID) {
    	//  CloseHandle(DXLocks[id].thisMutex);
    	//  DXLocks[id].thisMutex = NULL;
    	DXLocks[id].dxLockPtr = NULL;
    	DXLocks[id].ThreadID = 0;
    	DXLocks[id].ByWhom = 0;
    	DXLocks[id].ISLocked = UNLOCKED;
    	ReleaseMutex(DXLocks[id].thisMutex);
        SetEvent(DXLocks[id].thisEvent);
    }
    else {
	    printf("Someone trying to unlock me %c \n", 7);
    }
    IamHere = 0;
    return OK;
}

int DXfetch_and_add(int *p, int value, lock_type *l, int who)
{
     int old_value;

    DXlock(l, who);
    old_value = *p;
    *p += value;
    DXunlock(l, who);

    return old_value;
}

#endif     /*     DXD_WIN     */
