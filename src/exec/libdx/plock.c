/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#if defined(linux) && (ENABLE_SMP_LINUX == 1)

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include "plock.h"
#include <string.h>

#undef DEBUG_PLOCKS

#if defined(__GNU_LIBRARY__) && !defined(_SEM_SEMUN_UNDEFINED)
/* union semun is defined by including <sys/sem.h> */
#else
/* according to X/OPEN we have to define it ourselves */
union semun {
int val;                    /* value for SETVAL */
struct semid_ds *buf;       /* buffer for IPC_STAT, IPC_SET */
unsigned short int *array;  /* array for GETALL, SETALL */
struct seminfo *__buf;      /* buffer for IPC_INFO */
};
#endif

#define SEM_FLAGS  	 (IPC_CREAT | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)
#define SHM_FLAGS  	 (IPC_CREAT | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)
#define OPEN_FLAGS 	 (O_CREAT | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)
#define IS_MINE(a)	 (locks->_owner[a] == getpid())
#define SET_OWNER(a)	 (locks->_owner[a] = getpid())
#define CLEAR_OWNER(a)	 (locks->_owner[a] = 0)
#define IS_LOCKED(a)     (locks->_owner[a] != 0)
#define IS_UNLOCKED(a)   (! IS_LOCKED(a))
#define SEM_FLG 	 SEM_UNDO
#define MAGIC		 329430943

#define LOCKS_PER_SEMID	    250
#define SEM_BUFS	    256
#define MAXLOCKS	    (LOCKS_PER_SEMID * SEM_BUFS)
#define LOCK_SEMID(a)	    ((a)/LOCKS_PER_SEMID)
#define LOCK_OFFSET(a)	    ((a)%LOCKS_PER_SEMID)
#define CREATE_LOCK_ID(b,o) (((b)*LOCKS_PER_SEMID) + (o))

typedef struct
{
    int _magic;
    int _shmid;

    /*
     * locking the whole ball of wax
     */
    int _locklock;

#if defined(DEBUG_PLOCKS)
    /*
     * for debugging
     */
    int _findlock;
#endif

    int _totAllocated;
    int _totFreed;

    /*
     * index of next bunch of semaphores to allocate
     */
    int _nbuf;

    /*
     * For transfer of an inter-process message
     */
    int _value;
    int _sendlock;
    int _sendwait;

    /*
     * If a lock is locked, which process has it locked?
     */
    int _owner[MAXLOCKS];
    int _allocated[MAXLOCKS];

    /*
     * list of free locks
     */
    int _free[MAXLOCKS];
    int _nfree;

    /*
     * list of semaphore buffers
     */
    struct
    {
	int _nalloc;
	int _semid;
    } _sbufs[SEM_BUFS];

} _locks;

static _locks  *locks = NULL;
static void    _lock(int);
static void    _unlock(int);

#if defined(DEBUG_PLOCKS)
void
foundlock(int i, char *s)
{
    fprintf(stderr, "foundlock %s: %d on %d\n", s, i, getpid());
}
#endif

#define LOCKFILE "/tmp/P.locks"
#define PROJ	 'a'

static void
bump(int l, int v)
{
    struct sembuf sops;
    int sid, soff;

    sid = LOCK_SEMID(l);
    soff = LOCK_OFFSET(l);

    sops.sem_num = soff;
    sops.sem_op = v;
    sops.sem_flg = SEM_FLG;

    errno = 0;

    while ((semop(locks->_sbufs[sid]._semid, &sops, 1) != 0) && (errno == EINTR));

    if (errno)
        perror("error bumping lock");
}

static void
_lock(int l)
{
#if defined(DEBUG_PLOCKS)
    int n0, n1;
    int sid, soff;

    if (l == locks->_findlock)
    {
	sid = LOCK_SEMID(l);
	soff = LOCK_OFFSET(l);

	n0 = semctl(locks->_sbufs[sid]._semid, soff, GETVAL);
    }
#endif

    bump(l, -1);

#if defined(DEBUG_PLOCKS)
    if (l == locks->_findlock)
    {
        char buf[1024];

	n1 = semctl(locks->_sbufs[sid]._semid, soff, GETVAL);

	sprintf(buf, "_lock: %d %d", n0, n1);
        foundlock(locks->_findlock, buf);
    }
#endif

}

static void
_unlock(int l)
{
#if defined(DEBUG_PLOCKS)
    int n0, n1;
    int sid, soff;

    if (l == locks->_findlock)
    {
	sid = LOCK_SEMID(l);
	soff = LOCK_OFFSET(l);

	n0 = semctl(locks->_sbufs[sid]._semid, soff, GETVAL);
    }
#endif

    bump(l, 1);

#if defined(DEBUG_PLOCKS)
    if (l == locks->_findlock)
    {
        char buf[1024];

	n1 = semctl(locks->_sbufs[sid]._semid, soff, GETVAL);

	sprintf(buf, "_unlock: %d %d", n0, n1);
        foundlock(locks->_findlock, buf);
    }
#endif

}

static void
_cleanup()
{
    if (locks)
    {
	struct shmid_ds buf;
	int shmid = locks->_shmid;

	if (shmctl(shmid, IPC_STAT, &buf) == 0)
	    if (buf.shm_nattch == 1)
	    {
	        int i;
		for (i = 0; i < locks->_nbuf; i++)
		    semctl(locks->_sbufs[i]._semid, 0, IPC_RMID, NULL);
		shmctl(shmid, IPC_RMID, 0);
	    }

	shmdt(locks);
	locks = NULL;
    }
}

static int
_alloc()
{
    if (locks->_nbuf == SEM_BUFS)
    {
        fprintf(stderr, "too many locks created\n");
	return 0;
    }
    
    locks->_sbufs[locks->_nbuf]._nalloc = 0;

    locks->_sbufs[locks->_nbuf]._semid = semget(IPC_PRIVATE, LOCKS_PER_SEMID, SEM_FLAGS);
    if (-1 == locks->_sbufs[locks->_nbuf]._semid)
    {
	perror("error creating sem");
	abort();
    }

    locks->_nbuf++;

    return 1;
}

int
PLockInit()
{
    int shmid;
    struct stat statbuf;
    key_t k;

    if (locks)
    {
        fprintf(stderr, "initLocks: locks already initted\n");
	return 0;
    }

    if (stat(LOCKFILE, &statbuf))
    {
        int fd = open(LOCKFILE, OPEN_FLAGS);
	if (fd < 0)
	{
	    fprintf(stderr, "initLocks: error accessing locks shared block: open\n");
	    return 0;
	}
	close(fd);
    }
        
    k = ftok(LOCKFILE, PROJ);

    shmid = shmget(k, sizeof(_locks), SHM_FLAGS);
    if (shmid == 0)
    {
        fprintf(stderr, "initLocks: error accessing locks shared block: shmget\n");
	return 0;
    }

    locks = (_locks *)shmat(shmid, 0, 0);
    if (locks == (_locks *)-1)
    {
        fprintf(stderr, "initLocks: error accessing locks shared block: shmat\n");
	return 0;
    }

    if (locks->_magic != MAGIC)
    {
	locks->_magic  	     = MAGIC;
	locks->_shmid  	     = shmid;
	locks->_nbuf   	     = 0;
	locks->_nfree  	     = 0;
	locks->_totAllocated = 0;
	locks->_totFreed 	     = 0;
	memset(locks->_allocated, 0, sizeof(locks->_allocated));

#if defined(DEBUG_PLOCKS)

        s = getenv("PLOCK_DEBUG");
	if (s)
	{
	    locks->_findlock = atoi(s);
	    fprintf(stderr, "plock: looking for %d\n", locks->_findlock);
	}
	else
	    locks->_findlock = -1;
#endif

	if (! _alloc())
	    return 0;

	/*
	 * lock lock is allocated unlocked
	 */
	locks->_locklock = 0;
	_unlock(locks->_locklock);
	locks->_sbufs[0]._nalloc ++;

	/*
	 * now we can use the standard lock allocator
	 *
	 * initialize snd/rcv.  
	 */
	locks->_sendlock = PLockAllocateLock(1);
	locks->_sendwait = PLockAllocateLock(1);
    }

    atexit(_cleanup);

    return 1;
}

int
PLockAllocateLock(int locked)
{
    int i = -1;
    int sid, soff;
    union semun sbuf;

    _lock(locks->_locklock);

    locks->_totAllocated ++;

    if (locks->_nfree)
    {
	locks->_nfree --;
        i = locks->_free[locks->_nfree];

	sid  = locks->_sbufs[LOCK_SEMID(i)]._semid;
	soff = LOCK_OFFSET(i);
    }
    else
    {
        if (locks->_sbufs[locks->_nbuf - 1]._nalloc == LOCKS_PER_SEMID)
	    if (! _alloc())
	        return -1;

	sid  = locks->_sbufs[locks->_nbuf - 1]._semid;
	soff = locks->_sbufs[locks->_nbuf - 1]._nalloc;
	i = CREATE_LOCK_ID(locks->_nbuf - 1, soff);
	locks->_sbufs[locks->_nbuf - 1]._nalloc ++;

    }

    if (locked)
        sbuf.val = 0;
    else
        sbuf.val = 1;

    if (semctl(sid, soff, SETVAL, sbuf) == -1)
        perror("PLockAllocateLock");

    SET_OWNER(i);
    locks->_allocated[i] = 1;

#if defined(DEBUG_PLOCKS)
    if (i == locks->_findlock)
        foundlock(locks->_findlock, "PLockAllocateLock");
#endif

    _unlock(locks->_locklock);

    return i;
}

void
PLockFreeLock(int l)
{
    int n;
    int sid, soff;

    _lock(locks->_locklock);

    locks->_totFreed++;

#if defined(DEBUG_PLOCKS)
    if (l == locks->_findlock)
        foundlock(locks->_findlock, "PLockFreeLock");
#endif

    sid = LOCK_SEMID(l);
    soff = LOCK_OFFSET(l);

    n = semctl(locks->_sbufs[sid]._semid, soff, GETVAL);
    if (n < 0)
    {
	fprintf(stderr, "PLockFreeLock: invalid lock: %d on process %d\n", l, getpid());
	goto out;
    }

    locks->_owner[l] = 0;
    locks->_allocated[l] = 0;

    locks->_free[locks->_nfree] = l;
    locks->_nfree ++;

out:
    _unlock(locks->_locklock);
}

int
PLockLock(int l)
{
    _lock(locks->_locklock);

#if defined(DEBUG_PLOCKS)
    if (l == locks->_findlock)
        foundlock(locks->_findlock, "PLockLock entry");
#endif

    if (l < 3)
    {
        fprintf(stderr, "PLockLock: invalid lock: %d on process %d\n", l, getpid());
	goto out;
    }

    _unlock(locks->_locklock);
    _lock(l);
    _lock(locks->_locklock);

    SET_OWNER(l);

#if defined(DEBUG_PLOCKS)
    if (l == locks->_findlock)
        foundlock(locks->_findlock, "PLockLock exit");
#endif

    _unlock(locks->_locklock);

    return 1;

out:
    _unlock(locks->_locklock);
    return 0;
}

int
PLockTryLock(int l)
{
    _lock(locks->_locklock);

#if defined(DEBUG_PLOCKS)
    if (l == locks->_findlock)
        foundlock(locks->_findlock, "PLockTryLock");
#endif

    if (l < 3)
    {
        fprintf(stderr, "PLockTryLock: invalid lock: %d on process %d\n", l, getpid());
	goto out;
    }

    if (locks->_owner[l])
        goto out;

    while (IS_LOCKED(l))
    {
	_unlock(locks->_locklock);
	_lock(l);
	_lock(locks->_locklock);
    }

    SET_OWNER(l);

    _unlock(locks->_locklock);

    return 1;

out:
    _unlock(locks->_locklock);
    return 0;
}

int
PLockUnlock(int l)
{
    _lock(locks->_locklock);

#if defined(DEBUG_PLOCKS)
    if (l == locks->_findlock)
        foundlock(locks->_findlock, "PLockUnlock");
#endif

    if (l < 3)
    {
        fprintf(stderr, "PLockUnlock: invalid lock: %d on process %d \n", l, getpid());
	goto out;
    }

    CLEAR_OWNER(l);
    _unlock(l);

    _unlock(locks->_locklock);

    return 1;

out:
    _unlock(locks->_locklock);
    return 0;
}

int
PLockAllocateWait()
{
    return PLockAllocateLock(1);
}

void
PLockFreeWait(int l)
{
    return PLockFreeLock(l);
}

int
PLockWait(int w, int l)
{
    _lock(locks->_locklock);

    if (w < 3)
    {
        fprintf(stderr, "PLockWait: invalid wait: %d on process %d\n", w, getpid());
	goto out;
    }

    if (l < 3)
    {
        fprintf(stderr, "PLockWait: invalid lock: %d on process %d\n", l, getpid());
	goto out;
    }

    if (! IS_LOCKED(l))
    {
        fprintf(stderr, "PLockWait: lock is not locked: %d on process %d\n", l, getpid());
	goto out;
    }

    if (!IS_MINE(l))
    {
        fprintf(stderr, "PLockWait: attempting to unlock someone else's lock: %d pid %d\n", l, getpid());
	goto out;
    }

    CLEAR_OWNER(l);
    _unlock(l);

    SET_OWNER(w);
    _unlock(locks->_locklock);
    bump(w, -1);
    _lock(l);
    _lock(locks->_locklock);

    SET_OWNER(l);

    _unlock(locks->_locklock);

    return 1;

out:
    _unlock(locks->_locklock);
    return 0;
}

int 
PLockSignal(int w)
{
    _lock(locks->_locklock);

    if (IS_LOCKED(w))
	bump(w, 1);
    
    _unlock(locks->_locklock);

    return 1;
}

void
PLockStats()
{
    fprintf(stderr, "%d locks allocated, %d locks freed, %d locks outstanding\n",
    		locks->_totAllocated, locks->_totFreed, 
    		locks->_totAllocated - locks->_totFreed);
}

#endif
