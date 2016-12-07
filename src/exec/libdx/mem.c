/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


/* this file is compiled into all versions, but the functions are 
 * really only needed for multiprocessor architectures or machines
 * which are using shared memory.  there are stubs defined at the end
 * of this file so if none of the special architectures are defined
 * default routines are used.  the init code is really needed; if not
 * using shared memory, it has to grow the data segment.
 */

/* 
 * the following is an old comment - some of the items aren't relevant anymore.
 * the original strategy on some architectures was to allocate a small
 * shared memory area, and then grow it even after the child processes
 * had been forked.  this required signals and some synchronization between
 * children to be sure they had a consistent view of the global space before
 * doing an allocate.  the current code is simpler in that we just allocate
 * the entire shared mem segment at initialization time, before forking,
 * and then each child has a consistent view of the shared space when they
 * start.
 *
 *
 * System-independent shared memory segment management.  Most of the
 * complication here is due to System V shared memory segments, which
 * are rather difficult to use for this purpose.  This file provides:
 *
 * Error _dxfsetmem(ulong limit)
 *     Called before getmem or getbrk with the total of the small/large 
 *     arena sizes in bytes.
 *
 * Error _dxfinitmem()
 *     Called after the first getmem/getbrk but before any forks;
 *     can be used to initialize lock tables.
 *
 * Pointer _dxfgetmem(Pointer base, ulong size)
 *     Request for a shared memory segment starting at base of the given
 *     size.  Guaranteed to be initialized to zeroes. Calls signal_allprocs
 *     so each process can attach to the new memory segment. Returns 
 *     actual address of the segment, or NULL for error
 *
 * Pointer _dxfgetbrk(Pointer base, ulong size)
 *     Alternative to _dxfgetmem.  Calls the brk() system call to expand
 *     the existing data segment.  Defined at the end of the file instead
 *     of in the system-dependent sections because it's the same for all.
 *
 * Error DXsyncmem()
 *     This routine should no longer be needed. 
 *     Used to Synchronize shared memory segments between processors. This
 *     call is no longer needed because we changed _dxfgetmem to send a signal 
 *     to all processors so they can attach to the new segment. This routine
 *     used to be called at "appropriate" points in execution.
 *
 * Error DXmemfork(int)
 *     Replacement for fork that causes this process to participate in
 *     the shared memory pool.
 */

#if defined(HAVE_WINDOWS_H)
#include <windows.h>
#endif

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#if defined(HAVE_ERRNO_H)
#include <errno.h>
#endif

#include <string.h>
#include <dx/dx.h>
#include "../dpexec/dxmain.h"

/* prototypes - not in a system header file because these are only
 *  called in a couple places, mostly in memory.c but also from the
 *  exec, and they are basically private routines which shouldn't
 *  be exposed.
 */
Error _dxfsetmem(ulong limit);
Error _dxfinitmem();
Pointer _dxfgetmem(Pointer base, ulong size);
Pointer _dxfgetbrk(Pointer base, ulong size);
int   _dxfinmem(Pointer x);
Error DXsyncmem();
Error DXmemfork(int);

/* moved out of memstats conditional section.  i'm going to use this to tell
 * what kind of memory we are using - shared or dataseg - so when we are
 * called to extend the segment we can do the right thing.  it's getting 
 * more complicated to tell which one we are using - MP or UP, small vs
 * large on ibm6000, environment variable override, etc.
 */
typedef enum memtype {
    MEM_NOTINIT,       /* memory init routines not called yet */
    MEM_DATASEG,       /* data segment extended with sbrk() */
    MEM_SHARED         /* separate shared memory segment obtained */
} Memtype;

static Memtype m = MEM_NOTINIT;

static int num_sbrk_calls = 0;
static int num_failed_calls = 0;

#define MAX_CHUNKS 256
static Pointer alloc_addr_start[MAX_CHUNKS];
static Pointer alloc_addr_end[MAX_CHUNKS];
static int alloc_segments[MAX_CHUNKS];
static int alloc_chunks;

/* some compilers don't let you do math on void *, so cast
 * them to char * first
 */
#define ADD_INT(a, b)  ((Pointer)((byte *)(a) + (b)))
#define SUB_INT(a, b)  ((Pointer)((byte *)(a) - (b)))
#define SUB_PTR(a, b)  ((Pointer)((byte *)(a) - (byte *)(b)))
#define ERR_PTR        ((Pointer)(-1))


/* routines common to all archs - just stubs now 
 */
Error _dxfinitmem() { return OK; }
Error DXsyncmem() { return OK; }
/* is this called from anywhere? */
void _dxfcleanup_mem() { }




#if ibm6000 || sgi || solaris || alphax || hp700 || linux

#define memroutines

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/sysmacros.h>
#include <sys/param.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <fcntl.h>

/* no resizing; allocate enough space initially. 
 *
 * the address space needs to be contiguous, so allow system to pick
 * addr of first segment and then use explicit addresses after that.
 *
 * on solaris, by default the system picks the last available segment, so
 * if multiple segments are necessary, request them in decreasing memory
 * address order - as long as they really are contiguous, it's ok.
 *
 */

#if ibm6000
#define DATASEG_LIMIT (255 * 1024L * 1024L)
#endif

ulong
getSHMMAX()
{
    ulong shmmax = 0;

#if linux
    FILE *fd = fopen("/proc/sys/kernel/shmmax", "r");
    if (fd)
    {
        if( fscanf(fd, "%lu", &shmmax) != 1)
            shmmax = 0x2000000;
	fclose(fd);
    }
    else
    {
        /*
	 * A problem here ... SHMMAX is defined in linux/shm.h,
	 * but if we include that, all hell breaks loose with 
	 * redefined structures.
	 */
        shmmax = 0x2000000;
    }
#endif

    if (shmmax == 0)
    {
#if defined(SHMMAX)
	shmmax = SHMMAX;
#else
	/* wishful thinking... 256 Meg */
	shmmax = (1 << 28); 
#endif
    }

    return shmmax;
}


extern int _dxd_exRunningSingleProcess;   /* boolean supplied by the exec */

/* move this logic out of dxfsetmem() into its own routine.  it's
 * gotten too complicated.
 */
static Error whatkindofmem(ulong limit)
{
    int force = 0;    /* set if the user requested a particular choice */
    char *cp = NULL;

    /* start by checking to see if we are running SMP.  if so, we have to
     * use shared memory, end of story.  the child processes all are attached
     * to the same segment(s) and so can share data.  the data segment itself
     * is private to each process and cannot be shared.
     *
     * otherwise, see if there's another reason to use shared mem.  if not,
     * use brk() to expand the data segment.  this avoids problems with having
     * to reconfigure the solaris because the shared mem limit is too low
     * and it doesn't use up shared memory resources which aren't needed
     * if you aren't going to run multiprocessor.
     *
     * on the ibm6000 only, if the requested arena size is > 256 Mb, then
     * even UP use shared memory - there is a system-imposed limit of
     * 256 mb in the data segment unless you use some special link flags
     * (see man ld, look for -bmaxdata) which cause problems with other
     * runtime-loaded code like the opengl libs.
     *
     * (sgi note) the following description of DX behavior is still the
     * case but the allegations about IRIX vfork aren't quite accurate.  On
     * IRIX, fork()/vfork() are like BSD vfork() but the pages are marked
     * MAP_PRIVATE rather than MAP_SHARED, so they don't get copied unless
     * modified.  However, the page table does get duplicated.
     * Performance-wise this is similar to the BSD vfork().
     *
     * on the sgi only, always use shared memory.  all other platforms
     * have vfork(), which does NOT copy the current process on a fork.
     * this is good because you are turning around and execing a second
     * (smaller) process in most cases.  on the sgi, there is no vfork()
     * and dx with a huge data segment cannot fork because it can't copy
     * the data segment even though we don't want it for any reason.
     * neither fork() nor vfork() copy shared memory, so by using that
     * instead of expanding the data segment, ! filters and system() and
     * other things which require a fork/exec have a chance of working.
     * */

    /* user override - force using shared memory or the data segment.  
     * set the value, but go ahead and check to see if we'd have to set it
     * to the other value.  if override not set, default to using the data
     * segment and then see if we need to be using shared memory.
     */
    if ((cp = getenv("DXSHMEM")) != NULL) {
	/* atoi() unfortunately returns 0 on error.  this means "fred",
	 * 0, and NULL all look the same.  sigh.  i could start in with
	 * skipping spaces then looking for isdigit(), but sheesh.
	 * the rule is going to be -1 == data segment; anything else
	 * is shared memory.
	 */
	m = (atoi(cp) == -1) ? MEM_DATASEG : MEM_SHARED;
	force++;

    } else
	m = MEM_DATASEG;
    
    
    /* if running MP, you have to use shared memory */
    if (!_dxd_exRunningSingleProcess) {
	if (force && (m == MEM_DATASEG)) {
	    DXSetError(ERROR_BAD_PARAMETER, 
		       "DXSHMEM cannot be -1; multiprocessing requires using shared memory");
	    return ERROR;
	}
	m = MEM_SHARED;
	return OK;
    }
	
#if ibm6000
    /* avoid the problem of not being able to grow the dataseg more
     * than 256M by using shared memory anyway
     */
    if (limit >= DATASEG_LIMIT) {
	if (force && (m == MEM_DATASEG)) {
	    DXSetError(ERROR_BAD_PARAMETER, 
		       "DXSHMEM cannot be -1; more than %u Mbytes requires using shared memory", 
		       (uint)(DATASEG_LIMIT/(1024L*1024L)));
	    return ERROR;
	}
	m = MEM_SHARED;
	return OK;
    }
#endif

#if sgi
    /* avoid the problem of not being able to fork ! filters because
     * sgi doesn't have a real vfork().  always use shmem because the
     * data segment is copied on fork but shmem is not.  here's a tricky
     * one - i guess go ahead and let the user override the default.
     * it may cause them fork() headaches, but it's their dime.
     */
    if (force)
	return OK;

    m = MEM_SHARED;
    return OK;

#else   /* !sgi */

    /* apparently no reason not to use the data segment, or whatever
     * the user picked as an override.
     */
    return OK;

#endif  
}

/* we could make limit be Mbytes instead of real bytes.  that would
 * buy us a factor of e6 in size in an int.
 */
Error
_dxfsetmem(ulong limit)
{
    ulong size, maxsegsize = getSHMMAX();
    int id, nchunk, nextseg;
    int i, j;
    int first = 1;
    Pointer sh_base[MAX_CHUNKS]; 
    ulong gotten[MAX_CHUNKS];
    int    nsegs [MAX_CHUNKS];
    ulong totalsize = 0;
    Pointer tbase, tend;
    int tcount;
    extern int errno;
    char *cp;

    /* decide on shared mem or the data segment */
    if (whatkindofmem(limit) == ERROR)
	return ERROR;

    /* if we decided to use the data segment instead of shared memory,
     * just return.  we'll extended the data segment on demand, from
     * the dxfgetmem() routine.
     */
    if (m == MEM_DATASEG)
	return OK;

    /* allow user to tell us the system limit for the size of one shared memory
     * segment in megabytes.  this should only be needed if they haven't
     * set the system limits file like we tell them to.
     */
    if ((cp = getenv("DXSHMEMSEGMAX")) != NULL) {
	maxsegsize = atoi(cp);
#ifdef ENABLE_LARGE_ARENAS
	if (maxsegsize <= 0)
#else
	if (maxsegsize <= 0 || maxsegsize > 2048)
#endif
	{
	    DXSetError(ERROR_BAD_PARAMETER, 
      "bad shared memory segment size limit, DXSHMEMSEGMAX = %d Megabytes",
			  maxsegsize);
	    return ERROR;
	} 
	maxsegsize *= 1024L*1024L;
    }
    
    nchunk = 0;
    nsegs[nchunk] = 0;
    gotten[nchunk] = 0;
    sh_base[nchunk] = NULL;
    nextseg = 1;
    
    while (totalsize < limit) {
	/* the default segment size, unless we are near the end and only
	 * need a partial segment to finish out the required space.
	 */
	size = ((limit-totalsize) > maxsegsize) ? maxsegsize : (limit-totalsize);
	
	/* get the memory segment from the system. 
	 */
	id = shmget(IPC_PRIVATE, size, IPC_CREAT|IPC_EXCL|S_IRUSR|S_IWUSR);
	if (id < 0) {
	    DXSetError(ERROR_NO_MEMORY, "cannot get shared memory segment");
	    DXAddMessage("failed on segment %d of %ld needed segments", 
		nextseg, ((long)limit + maxsegsize - 1) / maxsegsize);
	    DXAddMessage("last call to shmget returned errno %d", errno);
	    return ERROR;
	}
	

	/* if we are working on an existing chunk, try to add in a
	 * contiguous manner either at the end or directly before it.
	 */
	if (sh_base[nchunk] != NULL) {
	    /* try after */
	    tbase = shmat(id, ADD_INT(sh_base[nchunk], gotten[nchunk]), 0);
	    if (tbase != ERR_PTR) {
	        gotten[nchunk] += size;
	        nsegs[nchunk]++;
		goto got_one;
	    } 

	    /* try before */
	    tbase = shmat(id, SUB_INT(sh_base[nchunk], size), 0);
	    if (tbase != ERR_PTR) {
		sh_base[nchunk] = SUB_INT(sh_base[nchunk], size);
	        gotten[nchunk] += size;
		nsegs[nchunk]++;
		goto got_one;
	    } 
	}
	/* have to start a new chunk - either this is the first time through
	 * or we couldn't attach on either side of the last chunk.
	 */
	tbase = shmat(id, NULL, 0);
	if (tbase == ERR_PTR) {
	    /* error */
	    DXSetError(ERROR_NO_MEMORY, "cannot attach shared memory segment");
	    DXAddMessage("failed on segment %d of %ld needed segments", 
			 nextseg, ((long)limit + maxsegsize - 1) / maxsegsize);
	    DXAddMessage("last call to shmat returned errno = %d", errno);
	    shmctl(id, IPC_RMID, 0);
	    return ERROR;
	}
	
	/* don't bump the chunk number the first time through */
	if (first) 
	    first = 0;
	else
	    nchunk++;
	
	/* check for too many chunks */
	if (nchunk >= MAX_CHUNKS-1) {
	    DXSetError(ERROR_NO_MEMORY,
		       "too many disjoint memory segments (more than %d)",
		       MAX_CHUNKS);
	    DXAddMessage("start again with -memory set to a smaller value or with $DXSHMEMSEGMAX to a larger value");
	    return ERROR;
	}

	/* set the basic info for this chunk */
	sh_base[nchunk] = tbase;
	gotten[nchunk] = size;
	nsegs[nchunk] = 1;
	
      got_one:
	/* mark segment for deletion upon process termination */
	if (shmctl(id, IPC_RMID, 0) < 0)
	    DXWarning("cannot mark shared memory segment to be deleted at process exit.  id = %d", id);
	
	totalsize += size;
	nextseg++;
    }

	 
    /* at this point we no longer care about how many segments there are per
     * chunk, only number of discontiguous chunks we got.  this is a different
     * use of these structs than in the past.
     */
	    
    for (i=0; i <= nchunk; i++) {
	alloc_addr_start[i] = sh_base[i];
	alloc_addr_end[i] = ADD_INT(sh_base[i], gotten[i]);
	alloc_segments[i] = nsegs[i];
    }
    alloc_chunks = nchunk + 1;

    if (nchunk > 0) 
	/* bubble sort here - there shouldn't be many chunks */
	for (i=0; i <= nchunk; i++) 
	    for (j=i; j <= nchunk; j++) 
		if (alloc_addr_start[j] < alloc_addr_start[i]) {
		    tbase = alloc_addr_start[i];
		    tend = alloc_addr_end[i];
		    tcount = alloc_segments[i];
		    
		    alloc_addr_start[i] = alloc_addr_start[j];
		    alloc_addr_end[i] = alloc_addr_end[j];
		    alloc_segments[i] = alloc_segments[j];

		    alloc_addr_start[j] = tbase;
		    alloc_addr_end[j] = tend;
		    alloc_segments[j] = tcount;
		}


    /* this is old - it's more complicated than this now */
    DXDebug("M", "shared memory will start at 0x%08x", sh_base[0]);
    return OK;
}


Pointer
_dxfgetmem(Pointer base, ulong size)
{
    int i;

    /* if using the data segment, extend it the requested number of bytes */
    if (m == MEM_DATASEG)
	return _dxfgetbrk(base, size);

    /* if using shared memory, then all the space is already allocated.
     * the create for the small arena calls this with 0, so this returns
     *  the start of the shared memory segment which contains both the
     *  small and large arena.  the create for the large arena calls this
     *  with small_base + small_size, which is exactly where the large
     *  arena should start, so just return that same value.
     */

    if (base == 0)
	return (Pointer)alloc_addr_start[0];

    /* search for the enclosing segment and return the same chunk base
     * if there is still space in it - otherwise return the next chunk.
     * ah hell.  i've rewritten this 5 times now.  how hard can this be?
     * i've found many creative ways to do this wrong.  and it's damn slow
     * to get to this point during testing.
     */

    /* find the largest chunk starting address which is still below base.
     * quit at the index of the last valid chunk, not one past.
     */
    for (i=0; i<alloc_chunks && base > alloc_addr_start[i]; i++)
	;

    /* this has got to be one too far, so back it up */
    i--;
    
    /* if base to base+size is completely within this segment, we're ok */
    if (base > alloc_addr_start[i]  &&  ADD_INT(base, size) <= alloc_addr_end[i])
	return base;

    /* we are either completely out of space, or we need to return the start of the
     * next chunk.  if we are already at the last chunk, fail. 
     */
    if (i == alloc_chunks-1)
	return NULL;

    /* we've gone past the end of the current chunk - return the start of the next one */
    return alloc_addr_start[i+1];
}

int
_dxfinmem(Pointer x)
{
    int i;

    for (i=0; i<alloc_chunks; i++) 
        if ((x >= alloc_addr_start[i]) && (x < alloc_addr_end[i]))
	    return 1;

    return 0;
}

#endif   /* ibm6000, solaris, sgi, alpha, hp700 */

#if defined(macos)
#define memroutines

/* Still need to write shared memory routines */

void *sh_base = NULL;   /* starting virtual address */
Error _dxfsetmem(ulong limit)
{
    sh_base = malloc(limit);
    if (sh_base == NULL) {
	DXErrorReturn(ERROR_NO_MEMORY, "getmem can't commit memory");
    }
    else {
	/*   memset(sh_base, 0, limit);   */
	return OK;
    }
}

Pointer _dxfgetmem(Pointer base, ulong size)
{
    if (!base)	
	base = (Pointer) sh_base;

    return base;
}
#endif /* defined(macos) */


#if defined(intelnt) || defined(WIN32)

#if defined(USE_AWE)

/*****************************************************************
   LoggedSetLockPagesPrivilege: a function to obtain, if possible, or
   release the privilege of locking physical pages.

   Inputs:

       HANDLE hProcess: Handle for the process for which the
       privilege is needed

       BOOL bEnable: Enable (TRUE) or disable?

   Return value: TRUE indicates success, FALSE failure.

*****************************************************************/
BOOL
LoggedSetLockPagesPrivilege ( HANDLE hProcess,
							 BOOL bEnable)
{
	struct {
		DWORD Count;
		LUID_AND_ATTRIBUTES Privilege [1];
	} Info;
	
	HANDLE Token;
	BOOL Result;
	
	Result = OpenProcessToken ( hProcess,
		TOKEN_ADJUST_PRIVILEGES,
		& Token);
	
	if( Result != TRUE ) {
		DXSetError(ERROR_NO_MEMORY, "Cannot open process token.\n" );
		return FALSE;
	}
	
	Info.Count = 1;
	if( bEnable ) 
	{
		Info.Privilege[0].Attributes = SE_PRIVILEGE_ENABLED;
	} 
	else 
	{
		Info.Privilege[0].Attributes = 0;
	}
	
	Result = LookupPrivilegeValue ( NULL,
		SE_LOCK_MEMORY_NAME,
		&(Info.Privilege[0].Luid));
	
	if( Result != TRUE ) 
	{
		DXSetError(ERROR_NO_MEMORY, "Cannot get privilege value for %s.\n", SE_LOCK_MEMORY_NAME );
		return FALSE;
	}
	
	Result = AdjustTokenPrivileges ( Token, FALSE,
		(PTOKEN_PRIVILEGES) &Info,
		0, NULL, NULL);
	
	if( Result != TRUE ) 
	{
		DXSetError(ERROR_NO_MEMORY, "Cannot adjust token privileges, error %u.\n", GetLastError() );
		return FALSE;
	} 
	else 
	{
		if( GetLastError() != ERROR_SUCCESS ) 
		{
			DXSetError(ERROR_NO_MEMORY, "Cannot enable SE_LOCK_MEMORY privilege, please check the local policy.\n");
			return FALSE;
		}
	}
	
	CloseHandle( Token );
	
	return TRUE;
}

#endif

#ifndef DXD_WIN_SHARE_MEMORY
#define memroutines
void *sh_base;   /* starting virtual address */


DWORD GetPageSize(void) {
	SYSTEM_INFO SystemInfo;
	GetSystemInfo(&SystemInfo);
	return SystemInfo.dwPageSize;
}

Error _dxfsetmem(ulong limit)
{
	/* Determine if we should use the AWE extensions and lock physical
	   memory. Can give us significant speed up. Only available with
	   Win2K or better and user must have special priveledges. See AWE
	   extensions in MSDN. 
	*/
#if defined(USE_AWE)
	HANDLE hProcess = GetCurrentProcess();
	ULONG_PTR pnPages;
	PULONG_PTR ppPFN;
	int PFNArraySize;

	pnPages = limit / GetPageSize();
	if (limit % GetPageSize() ) { pnPages += 1; }
	
	PFNArraySize = sizeof(ULONG_PTR) * pnPages;

	ppPFN = (PULONG_PTR) HeapAlloc (GetProcessHeap(), 0, PFNArraySize); /* Allocate PFN array */
	
	if( ! LoggedSetLockPagesPrivilege( GetCurrentProcess(), TRUE ) ) {
		DXErrorReturn(ERROR_NO_MEMORY, "Cannot get priveledges to lock physical memory.");
	}

	if(AllocateUserPhysicalPages(hProcess, &pnPages, ppPFN) ) {
		limit = pnPages * GetPageSize(); /* May return fewer pages than asked for */
		sh_base = VirtualAlloc(NULL, limit, MEM_RESERVE | MEM_PHYSICAL, PAGE_READWRITE);
		if(sh_base == 0) {
			LPVOID lpMsgBuf;
			FormatMessage( 
				FORMAT_MESSAGE_ALLOCATE_BUFFER | 
				FORMAT_MESSAGE_FROM_SYSTEM | 
				FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL,
				GetLastError(),
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), /* Default language */
				(LPTSTR) &lpMsgBuf,
				0,
				NULL 
				);
			DXErrorReturn(ERROR_NO_MEMORY, (char *) lpMsgBuf);
			LocalFree( lpMsgBuf );
		}
		MapUserPhysicalPages(sh_base, pnPages, ppPFN);
	} else {
		LPVOID lpMsgBuf;
		FormatMessage( 
			FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			FORMAT_MESSAGE_FROM_SYSTEM | 
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), /* Default language */
			(LPTSTR) &lpMsgBuf,
			0,
			NULL 
			);
		DXErrorReturn(ERROR_NO_MEMORY, (char *) lpMsgBuf);
		LocalFree( lpMsgBuf );
	}

#else
	sh_base = VirtualAlloc(NULL, limit, MEM_COMMIT, PAGE_READWRITE);
	if(sh_base == 0) {
		LPVOID lpMsgBuf;
		FormatMessage( 
			FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			FORMAT_MESSAGE_FROM_SYSTEM | 
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), /* Default language */
			(LPTSTR) &lpMsgBuf,
			0,
			NULL 
			);
		DXErrorReturn(ERROR_NO_MEMORY, (char *) lpMsgBuf);
		LocalFree( lpMsgBuf );
	}
#endif
	return OK;
}

Pointer _dxfgetmem(Pointer base, ulong size)
{
    if (!base)	
	base = (Pointer) sh_base;

    return base;
}

Error DXmemfork(int i)
{
#if 0
    return fork(); I hope this is not needed -- GDA
#endif
		return 0;
}


#else   /*   DXD_WIN_SHARE_MEMORY    */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <windows.h>
#include <string.h>
#include <winbase.h>



#define memroutines

void *sh_base;   /* starting virtual address */

/*  #define		DX_MAP_FILE	"dxmap.map"   */
#define		DX_DAT_FILE	"dxmap.tmp"

HANDLE MapFileHandle, MapHandle;
LPVOID MappedPointer;
static  ulong DXLimit;


Error _dxfsetmem(ulong limit)
{
    /*  Create Map file   */
    char DX_MAP_FILE[32];

    DXLimit = limit;
    sprintf(DX_MAP_FILE, "dx%d.map", _getpid()); 

    MapFileHandle= CreateFile(DX_DAT_FILE, 
                              GENERIC_READ | GENERIC_WRITE,
                              FILE_SHARE_READ | FILE_SHARE_WRITE,
                              NULL,
                              CREATE_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL | FILE_FLAG_DELETE_ON_CLOSE /* | STANDARD_RIGHTS_REQUIRED |
                              FILE_MAP_WRITE | FILE_MAP_READ */,
                              NULL);

    MapHandle= CreateFileMapping(MapFileHandle,
                                 NULL,
                                 PAGE_READWRITE,
                                 0,
                                 (DWORD) limit,
                                 DX_MAP_FILE);
    if(MapHandle == NULL)
    {
	CloseHandle(MapHandle);
	return NULL;
    }


    MappedPointer= MapViewOfFile(MapHandle,
		FILE_MAP_WRITE | FILE_MAP_READ,
		0,
		0,
		(DWORD) limit);
    if(MappedPointer)
    {
	memset(MappedPointer, 0, limit);
    }
    else
    {
	return NULL;
    }
	return OK;

}


Pointer _dxfgetmem(Pointer base, ulong size)
{
    /*  static HANDLE MapFileHandle, MapHandle;   */
    char DX_MAP_FILE[32];
    LPVOID MappedPointer;
    HANDLE hAMap;

    if (base) return base;

    sprintf(DX_MAP_FILE, "dx%d.map", _getpid()); 

    hAMap= OpenFileMapping(FILE_MAP_READ | FILE_MAP_WRITE,
                           TRUE,
                           DX_MAP_FILE);

    MappedPointer= MapViewOfFile(hAMap,
                                 FILE_MAP_WRITE | FILE_MAP_READ,
                                 0,
                                 0,
                                 DXLimit);

   base = (Pointer) MappedPointer; 
}


Error DXmemfork(int i)
{
    return fork();
}

#endif   /*   DXD_WIN_SHARE_MEMORY    */

#endif   

#if os2
#define memroutines

static PVOID sh_base;   /* starting virtual address */

Error
_dxfsetmem(ulong limit)
{

    if (DosAllocSharedMem(&sh_base, "\\sharemem\\dx.dat",
	(int)limit, PAG_READ | PAG_WRITE)) {
	DXErrorReturn(ERROR_NO_MEMORY, "setmem can't allocate memory");
    } else
        return OK ;
}

Pointer
_dxfgetmem(Pointer base, ulong size)
{
    if (!base) 
	base = (Pointer)sh_base;

    if (DosSetMem((PVOID)base, size, PAG_COMMIT | PAG_DEFAULT)) {
	DXErrorReturn(ERROR_NO_MEMORY, "getmem can't commit memory");
    } else
	return base ;
}

Error
DXmemfork(int i)
{
    return fork();
}

#endif

 
#if ibmpvs

#define memroutines
#include <sys/svs.h>

Error _dxfsetmem(ulong limit) { return OK; }
Error DXmemfork(int i) { return fork(); }
Pointer _dxfgetmem(Pointer base, ulong size) { return base; }

#endif /* ibmpvs */


#if !defined(os2) && !defined(intelnt) && !defined(WIN32)

#if !defined(cygwin) && !defined(macos)
extern int end;   /* filled in by linker */
#endif

/* extend the existing data segment
 */
Pointer _dxfgetbrk(Pointer base, ulong n)
{
    Pointer x = (Pointer)sbrk(n);

    if (x == ERR_PTR)
	num_failed_calls++;
    else if (num_sbrk_calls < MAX_CHUNKS) {
	alloc_addr_start[num_sbrk_calls] = x;
	alloc_addr_end[num_sbrk_calls] = ADD_INT(x, n);
	alloc_chunks++;
    }
    num_sbrk_calls++;
    

    if (m == MEM_NOTINIT)
	m = MEM_DATASEG;

    if (x == ERR_PTR) {
#if !(defined(cygwin) || defined (macos))
	unsigned int i;
#endif
	x = (Pointer)sbrk(0);
#if defined(cygwin) || defined (macos)
	DXSetError(ERROR_NO_MEMORY, 
		"cannot expand the data segment by %u bytes", n);
#else
	i = (unsigned int)((char *)x - (char *)&end);
	DXSetError(ERROR_NO_MEMORY, 
	"cannot expand the data segment by %u bytes, current size is %u bytes",
		   n, i);
#endif
	return NULL;
    } else
	return x;

}
#endif


/* the default case for all platforms which aren't MP or
 * aren't using shared memory.
 */

#if !defined(memroutines)

#if 0
/* if we run across a platform where we want to use sbrk
 * but ask for all the memory at once, i think this first section
 * will do it.  this is not tested, however.
 */
extern int end;  /* filled in by linker */
static Pointer after_end = NULL;

Error _dxfsetmem(ulong limit)
{
    after_end = _dxfgetbrk(limit);
}

Pointer _dxfgetmem(Pointer base, ulong size)
{
    return ((base == NULL) ? after_end : base);
}

#else
Error _dxfsetmem(ulong limit)
{
    return OK;
}

Pointer _dxfgetmem(Pointer base, ulong size)
{
    return _dxfgetbrk(base, size);
}
#endif


Error DXmemfork(int i)
{
    return fork();
}

#endif




#define MEMSTATS 1  /* change to 0 to comment out this code.  (needs a
                     * corresponding change in usage.m if you do.)
		     */

/* the NT linker doesn't define 'end' as the end of the data segment, 
 * so the calculations about the size of the data segment can't be done 
 * on the NT.  thus this section is disabled on that platform.
 */
#if defined(intelnt) || defined(WIN32)
#if defined(MEMSTATS)
#undef MEMSTATS
#endif
#define MEMSTATS 0
#endif

#if MEMSTATS   /* memory stats */

/* information about what was allocated & where.  it's hard to print this
 * out at startup time because the memory arenas aren't initialized yet and 
 * calling printf() can cause malloc to be called internally by the libc
 * routine for formatting, which on some architectures is caught and replaced
 * by a call to the dx memory manager, which isn't initialized yet...
 * so save it in global structs and you can look at it with the debugger
 * in case something bad happens.  or print it out with Usage("memory info");
 */

#if !defined(cygwin) && !defined(macos)
extern int end;   /* filled in by linker */
#endif

#define min(a,b)  ((a) < (b) ? (a) : (b))

/* public */
void DXPrintMemoryInfo()
{
    uint i;
    ulong total;

    switch (m) {
      case MEM_NOTINIT:
	DXMessage("memory not initialized yet");
	return;

      case MEM_DATASEG:
	DXMessage("using data segment for arenas; sbrk called %d times", 
		  num_sbrk_calls);
	if (num_failed_calls > 0) {
	    DXMessage("sbrk failed %d times", num_failed_calls);
	    num_sbrk_calls -= num_failed_calls;
	}
	for (i=0; i<min(num_sbrk_calls, MAX_CHUNKS); i++)
	    DXMessage("sbrk %d returned 0x%08x, %lu bytes allocated", 
		      i+1, alloc_addr_start[i], 
		      (ulong)SUB_PTR(alloc_addr_end[i], alloc_addr_start[i]));
	
#if !defined(cygwin) && !defined(macos)
	DXMessage("end address = 0x%08x, data segment extended by %lu bytes", 
		  alloc_addr_end[i-1],
		  (ulong)SUB_PTR(alloc_addr_end[i-1], &end));
#endif
	return;
	
      case MEM_SHARED:
	DXMessage("using %d shared memory chunks(s) for arenas", alloc_chunks);
	total = 0;
	for (i=0; i<min(alloc_chunks, MAX_CHUNKS); i++) {
	    DXMessage("chunk %d starts at 0x%08x, %lu bytes long (%d segments)", 
		      i+1, alloc_addr_start[i], 
		      (ulong)SUB_PTR(alloc_addr_end[i], alloc_addr_start[i]), 
		      alloc_segments[i]);
	    total += (ulong)SUB_PTR(alloc_addr_end[i], alloc_addr_start[i]);
	}
	if (i > 1)
	    DXMessage("end address = 0x%08x, total shared memory = %lu bytes", 
		      alloc_addr_end[i-1], total);
	else
	    DXMessage("end address = 0x%08x", alloc_addr_end[0]);
	return;
    }
    return;
}
#endif  /* memory stats */

