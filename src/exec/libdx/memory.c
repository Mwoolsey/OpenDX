/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#define DX_MEMORY_C

#include <stdio.h>

#define NO_STD_H
#include <dx/dx.h>
#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#if ibmpvs
#include <sys/svs.h>
#endif

#if sgi
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sysmacros.h>
#include <sys/param.h>
#include <sys/shm.h>
#include <invent.h>
#endif

#if sun4
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <string.h>
#endif

#if aviion
#include <sys/m88kbcs.h>
#endif

#if solaris
#include <sys/types.h>
#include <sys/param.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <string.h>
#endif

#if hp700
#include <sys/pstat.h>
#endif

#if os2
#include <stdlib.h>
#include <string.h>
#endif

#if alphax
#include <string.h>
#endif

#if linux
#include <linux/kernel.h>
#include <linux/sys.h>
#include <sys/sysinfo.h>
#endif

#if freebsd
#include <sys/types.h>
#include <sys/sysctl.h>
#endif

#if defined(macos)
#include <mach/mach.h>
#include <sys/param.h>
extern mach_port_t host_self(void);
#endif

#if DXD_HAS_RLIMIT && ! DXD_IS_MP
#include <sys/time.h>
#include <sys/resource.h>
#endif

extern int _dxd_exRemoteSlave;
extern void _dxfemergency(void);

/*
 * Every memory block starts with a struct block.  The user's portion
 * starts USER bytes into the block, after the next_block and prev_block
 * pointers.  Every memory block starts on an align boundary, and is
 * at least MINBLOCK bytes long.  The blocks contain pointers to the next
 * and previous block in memory, enabling us to find our neighbors in order
 * to merge when we are freed (if desired).  The size is the next_block
 * pointer minus the address of this block.  The low order bit of the
 * next_block pointer is set if this block is on a free list.  The next_free
 * and prev_free pointers are only valid if this block is on a free list;
 * otherwise these pointers are part of the user's area.  The prev_block/
 * next_block list is in sorted order; the prev_free/next_free list is
 * in random order.
 */

#define USER         (sizeof(struct block *)*2)	/* offset of user area */
#define MINBLOCK     sizeof(struct block) /* minimum block size */
#define MINPOOL      (2*MINBLOCK)	/* minimum size of pool block */

#define SMALL_ALIGN  16			/* alignment boundary of small arena */
#define SMALL_SHIFT  4			/* log base 2 of SMALL_ALIGN */
#if ibmpvs				/* to satisfy hippi alignment */
#define LARGE_ALIGN  1024		/* alignment boundary of large arena */
#define LARGE_SHIFT  10			/* log base 2 of LARGE_ALIGN */
#else
#define LARGE_ALIGN  16			/* alignment boundary of large arena */
#define LARGE_SHIFT  4			/* log base 2 of LARGE_ALIGN */
#endif
#define LOCAL_ALIGN  16			/* alignment boundary of local arena */
#define LOCAL_SHIFT  4			/* log base 2 of LOCAL_ALIGN */

struct block {
    struct block *prev_block;	/* previous block in memory */
    struct block *next_block;	/* next block in memory; low bit set if free */
    struct block *prev_free;	/* prevous block on free list, if free */
    struct block *next_free;	/* next block on free list, if free */
};


/*
 * Here are some macros to help manipulate blocks.
 *
 * ISFREE    tells whether a block is on the free list
 * SETFREE   sets the free bit, indicating block is on free list
 * CLEARFREE clears the free bit, indicating block is off free list
 * SIZE      returns the size of a block that is not on a free list
 * FSIZE     returns the size of a block that may or may not be on a free list
 * USIZE     returns the size of the block given user pointer x
 * NEXT      returns the next pointer of a block that may be on a free list
 * ROUND     rounds x up to boundary n
 * CHECK     checks a pointer for corruption
 *
 * Note - use caution with ISFREE, SETFREE, and CLEARFREE; in particular,
 * you should only change free-list related information when the appropriate
 * free list is locked.  You should only believe free list information for
 * a block you don't own (e.g. your next and prev blocks) after you have
 * locked the relevant free list.
 */
#ifdef USIZE
#undef USIZE
#endif

#define ISFREE(b)   ((((ulong)(b->next_block))&1))
#define SETFREE(b)  (b->next_block=(struct block *)(((ulong)(b->next_block))|1))
#define CLEARFREE(b)(b->next_block=(struct block *)(((ulong)(b->next_block))&~1))
#define SIZE(b)	    ((ulong)((b)->next_block)-(ulong)b)
#define FSIZE(b)    ((ulong)NEXT(b)-(ulong)b)
#define USIZE(x)    SIZE((struct block *)((ulong)x-USER))
#define NEXT(b)     ((struct block *)(((ulong)(b->next_block))&~1))
#define ROUND(x,n)  (((x)+(n)-1)&~((n)-1))
#define ROUNDDOWN(x,n)((x)&~((n)-1))


/*
 * Performance vs. debugging macros
 */

#if DEBUGGED
#define CHECK(b) DXASSERT(b->next_block->prev_block==b)
#define FOUND(x) (x==find_me)
#define COUNT(x) ((x)++)
#define AFILL(a,b) memset(a, 0xAB, b)   /* new allocation fill pattern */
#define FFILL(a,b) memset(a, 0xEF, b)   /* freed memory fill pattern */
#else
#define CHECK(b) if (b->next_block->prev_block!=b) {\
		    DXSetError(ERROR_INTERNAL, "#12140"); \
		    return ERROR; }
#define FOUND(x) (x==find_me)
#define COUNT(x) /* not debugged */
#define AFILL(a,b) /* not debugged */
#define FFILL(a,b) /* not debugged */
#endif

#if ibmpvs && OPTIMIZED
#define DXlock(l,v)	simple_lock(l,v+1)
#define DXunlock(l,v)	simple_unlock(l)
#endif


/*
 * The arena is organized as a bunch of separate free lists of roughly
 * exponentially increasing size.  Each free list contains blocks in
 * a range of sizes.  The number of free lists is determined by the
 * free list granularity parameter G: to simplify, there are G ranges of
 * sizes for each power of 2.  More precisely: each free list contains
 * blocks greater than or equal to some minimum size, but smaller than
 * the minimum size for the next larger free list.  For each binary decade
 * G*(2^n)*align to 2*G*(2^n)*align there are G free lists with minimum sizes
 * of G*(2^n)*align, (G+1)*(2^n)*align, ... , (2*G-1)*(2^n)*align.
 * The flist() routine below computes the free list that a block belongs on
 * given its size, and the alist() routine computes the minimum free list
 * such that any block on that free list is guaranteed to be large enough
 * to satisfy the request.
 *
 * The oflo field, if non-null, indicates that overflow of this arena is
 * not to be considered an error because the caller will attempt
 * to allocate from another arena if this one fills up.  The value of the
 * oflo field is the name of the overflow arena for use in a warning message.
 */

#define G   16			/* free list granularity parameter */
#define NL  (32*G)		/* number of lists based on that parameter */

static ulong maxFreedBlock = 0;/* size of largest freed block */

struct arena {

    char *name;			/* character string id */
    struct block *pool;		/* big block of unused storage */
    ulong pool_size;		/* bytes in pool in excess of MINPOOL */
    int pool_satisfied;		/* number satisfied from the pool */
    lock_type pool_lock;	/* lock for pool */
    int merge;			/* whether to merge blocks on free */
    struct list *max_list;	/* highest used list */
    int align;			/* alignment boundary for this arena */
    int shift;			/* log base 2 of align */
    char *oflo;			/* name of overflow arena (for messages) */
    int overflowed;		/* number of overflows */
    ulong max_user;		/* largest user size we allow */
    int scavenger_running;      /* set if we are trying to reclaim memory */
    int fill1[3];		/* fill to 8 int boundary  !?not right on axp*/

    struct list {		/* per-free list info */
	lock_type list_lock;	/* the lock */
	struct block block;	/* dummy block for head of list */
	ulong min;		/* minimium size of blocks on this list */
	int requested;		/* blocks requested from this list */
	int satisfied;		/* blocks satisfied from this list */
    } lists[NL];		/* max of NL free lists */

    struct block *blocks;	/* list of all blocks */
    Pointer (*get)(Pointer, ulong); /* how to get more memory */
    ulong size;		     /* current size of arena */
    ulong incr;		     /* multiple for increments */
    ulong max_size;		/* max size to allow arena to grow to */
    int dead_size;              /* amount of space in dead blocks */
};


/*
 * This routine computes the free list that a block belongs on given its size.
 */

static struct list *
flist(struct arena *a, ulong n)
{
    struct list *l = a->lists;
    n >>= a->shift;
    while (n > 2*G) {
	l += G;
	n /= 2;
    };
    return l + n;
}

/*
 * This routine computes the minimum free list such that any block on that
 * free list is guaranteed to be large enough to satisfy the request.  
 */

static struct list *
alist(struct arena *a, ulong n)
{
    return flist(a, n - a->align) + 1;
}


/*
 * This routine creates and initializes an arena based on the parameters
 * passed to it.
 */

static struct arena *
acreate(char *name, ulong base, ulong size, ulong max_user, 
	Pointer (*get)(Pointer,ulong), ulong incr, ulong max_size, 
	int merge, int align, int shift, char *oflo)
{
    struct arena *a;
    int i, j, min, delta;
    ulong l;

    a = (struct arena *)get((Pointer)base,size);/* get segment for the arena */
    if (!a)				/* get access to it? */
	return NULL;			/* no, return error */
    memset(a, 0, sizeof(struct arena));	/* zero arena header */
    a->name = name;			/* remember name */	
    a->merge = merge;			/* whether to merge blocks on free */
    a->max_user = max_user;		/* largest user size we allow */
    a->align = align;			/* alignment boundary for this arena */
    a->shift = shift;			/* log base 2 of align */
    a->oflo = oflo;			/* name of overflow arena */
    a->overflowed = 0;			/* whether we've overflowed */

    a->max_list=alist(a,max_user+USER);	/* largest possible free list */
    a->get = get;			/* routine to get more memory */
    a->size = size;			/* initial size of arena */
    a->incr = incr;			/* size increment */
    a->max_size = max_size;		/* maximum size for arena */

    /* intialize the locks */
    for (i=0; i<NL; i++)
	DXcreate_lock(&a->lists[i].list_lock, "free list");
    DXcreate_lock(&a->pool_lock, "pool");

    /* initialize the min size number for each list */
    for (i=0, min=0, j=0; j<G; j++, min++, i++)
	a->lists[i].min = min << shift;
    for (delta=1; i<NL; delta*=2)
	for (j=0; j<G; j++, min+=delta, i++)
	    a->lists[i].min = min << shift;

    l = (ulong)a + sizeof(struct arena);	/* end of arena, */
    l = ROUND(l+USER, align)-USER;	/* aligned, */
    a->pool = (struct block *)l;	/* is the pool block */
    a->pool_size = (ulong)a+size-l-MINPOOL;/* pool_size is excess over MINPOOL*/
    a->pool->prev_block = NULL;		/* no prev */
    a->pool->next_block = NULL;		/* no next */
    a->blocks = a->pool;		/* remember the list of all blocks */

    return a;
}


/*
 * Here's a routine that tries to expand the arena, enlarging the pool
 * to accomodate n bytes.  It assumes that we have already locked the
 * pool, if necessary.  If the new block is contiguous with the arena,
 * we just add it to the end of the pool.  If not, we create a "dead"
 * block containing the region to be skipped over, the added segment
 * becomes the new pool, and the old pool becomes just another free block.
 * In this case we may need to expand again, because our original estimate
 * of the amount we needed to add was discounted by the size of the
 * existing pool.
 *
 * Note that for now we take non-expandable to be equivalent to producing
 * just a warning of overflow (the first time), assuming the overflow will
 * automatically be handled by another arena (e.g. small to large).  These
 * two notions (non-expandable and overflow-handled) could be separated.
 */

#define EMessage _dxfemergency(),DXMessage
#define EDebug   _dxfemergency(),DXDebug
#define EWarning _dxfemergency(),DXWarning

/* struct for returning information about each arena, instead
 * of printing out the actual values
 */
struct printvals {
    ulong size;
    ulong header;
    ulong used;
    ulong free;
    ulong pool;
};

static void insfree(struct list *, struct block *);
static Error aprint(struct arena *a, int how, struct printvals *v);
static Error acheck(struct arena *a);

int _dxf_GetPhysicalProcs();

static Error
expand(struct arena *a, ulong n)
{
    ulong base, add, new;
    struct block *b, *d;
    struct list *l;

    add = n - a->pool_size;		/* this is the deficit */
    add = ROUND(add, a->incr);		/* add in increments of a->incr */

    if (a->size+add > a->max_size+a->dead_size	/* will we exceeded limit? */
      || !a->incr) {			/* or are we not allowed to expand? */
	if (!a->oflo) {			/* no overflow arena, it's an error */
	    _dxfemergency();
	    DXSetError(ERROR_NO_MEMORY,
		     "reached limit of %d in %s arena", a->max_size, a->name);
	    return ERROR;
	} else {			/* print warning about overflow */
	    int overflowed = a->overflowed;
	    a->overflowed = overflowed+1;
	    if (!overflowed) {
		EDebug("M", "%s arena overflowed to %s arena",
			a->name, a->oflo);
		aprint(a, 0, NULL);
	    }
	    return ERROR;
	}
    }

    base = (ulong)a + a->size;		/* ideal base of new segment */
    new = (ulong)a->get((Pointer)base, add); /* get the new segment? */
    if (!new)				/* got it? */
	return ERROR;			/* no; return failure */
    a->dead_size += new-base;           /* size of dead region, if any */
    a->size += add + new-base;		/* amount we added, plus dead region */

    if (new==base)			/* contiguous? */
	a->pool_size += add;		/* yes, just add it to pool */
    else {				/* non-contiguous case: */
	b = a->pool;			/* pool becomes just regular block */
	d = (struct block *)(base-USER);/* dead block */
	a->pool = (struct block *)new;	/* new pool */
	b->next_block = d;		/* dead block comes after b */
	l = flist(a, SIZE(b));		/* free list b belongs on */
	insfree(l, b);			/* put b on a free list */
	d->prev_block = b;		/* b comes before dead block */
	d->next_block = a->pool;	/* pool comes after dead block */
	a->pool->prev_block = d;	/* dead block comes before pool */
	a->pool->next_block = NULL;	/* new pool has no next block */
	a->pool_size = add - MINPOOL;	/* new pool size */
	if (a->pool_size < n)		/* is there room in the pool for n? */
	    if (!expand(a, n))		/* no; can we expand some more? */
		return ERROR;		/* no; return failure */
	EDebug("M", 
	       "could not extend %s arena; separate block obtained", 
	       a->name);
    }
    if (!_dxd_exRemoteSlave)
        EDebug("M", "%s arena is now %d bytes", a->name, a->size);
    return OK;
}



/*
 * Here's a routine to print out information about an arena.  The
 * level of detail is controlled by the how parameter: 0 means just
 * summary information for whole arena, 1 means also print out allocated 
 * blocks, 2 means print out every block plus lists.
 * CHANGED - 2 prints out lists. 
 *           3 prints out lists and free blocks on lists
 *           4 prints out every allocated block plus lists.
 *           5 prints out every block plus lists.
 */

static Error
aprint(struct arena *a, int how, struct printvals *v)
{
    struct block *b;
    float sumreq=0, sumsat=0;
    int i, m, free=0, used=0;
    ulong n=0, header, pool, size;
    struct list *l;
    int lused[NL];

    if (how>1)
	EMessage("%s arena:", a->name);

    for (i=0; i<NL; i++)
	lused[i] = 0;

    for (b=a->blocks; NEXT(b); b=NEXT(b)) {
	if (NEXT(b)->prev_block!=b || (b->prev_block&&NEXT(b->prev_block)!=b))
	    EMessage("corrupt block at 0x%lx (user 0x%lx)", b, (ulong)b+USER);
	if (how==4 && !ISFREE(b))
	    EMessage("alloc block of size %ld at user 0x%lx", 
		     FSIZE(b), (ulong)b+USER);
	if (how>=5)
	    EMessage("%s block of size %ld at 0x%lx (user 0x%lx)",
		     ISFREE(b)? "free" : "alloc", FSIZE(b), b, (ulong)b+USER);
	n = FSIZE(b);
	if (ISFREE(b))
	    free += n;
	else {
	    l = flist(a, FSIZE(b));
	    lused[l-a->lists]++;
	    used += n;
	}
    }

    for (i=0; i<NL; i++) {
	struct list *l = &(a->lists[i]);
	if (l->block.next_free || l->requested || l->satisfied) {
	    if (how>=2) {
		for (m=0, b=l->block.next_free; b; b=b->next_free)
		    m++;
		EMessage("list %d: %d min, %d req, %d sat, %d free, %d used",
			 i, l->min, l->requested, l->satisfied, m, lused[i]);
		if (how>=3)
		    for (b=l->block.next_free; b; b=b->next_free)
		        EMessage("    block of size %ld at 0x%lx", FSIZE(b), b);
	    }
	    sumreq += i*l->requested;
	    sumsat += i*l->satisfied;
	    n += l->requested;
	}
    }
    if (how>=2)
	EMessage("pool: %d satisfied", a->pool_satisfied);
    header = (ulong)a->blocks - (ulong)a;
    pool = a->pool_size + MINPOOL;
    size = a->size;
	
    /* only print old message if not returning values */
    if (!v) {
	if (a->dead_size == 0) {
        EMessage(a->overflowed>0?
	   "%s: %d = hdr %d + used %d + free %d + pool %d (limit %d); %d ofl" :
	   "%s: %d = hdr %d + used %d + free %d + pool %d (limit %d)",
	     a->name, size, header, used, free, pool, 
	     a->max_size, a->overflowed);
	} else {
        EMessage(a->overflowed>0?
	   "%s: %d = hdr %d + used %d + free %d + pool %d + dead %d (limit %d); %d ofl" :
	   "%s: %d = hdr %d + used %d + free %d + pool %d + dead %d (limit %d)",
	     a->name, size, header, used, free, pool, a->dead_size,
	     a->max_size, a->overflowed);
	}

    } else {
         v->size = size;
         v->header = header;
         v->used = used;
         v->free = free;
         v->pool = pool;
    }

    if (size!=header+used+free+pool)
	EWarning("things don't add up in the %s arena", a->name);

    return OK;
}

/* don't print, but return whether memory is corrupted or not.
 * should this use assert to stop immediately?
 */
static Error
acheck(struct arena *a)
{
    struct block *b;

    for (b=a->blocks; NEXT(b); b=NEXT(b)) {
	if (NEXT(b)->prev_block != b
	    || (b->prev_block && NEXT(b->prev_block) != b)) {
	    EMessage("corrupt block at 0x%lx (user 0x%lx)", b, (ulong)b+USER);
	    return ERROR;
	}
    }

    return OK;
}

/*
 * this routine allows a user-supplied routine to examine/print/evaluate
 * allocated memory arenas.
 */

static Error
adebug(struct arena *a, int blocktype, MemDebug m, Pointer p)
{
    struct block *b;
    Error rc;

    /* block which contains the memory manager data structs */
    if (blocktype & MEMORY_PRIVATE) {
	rc = (*m)(MEMORY_PRIVATE, (Pointer)a, sizeof(struct arena), p);
	if (rc == ERROR)
	    return rc;
    }

    /* normal user blocks, either allocated or free */
    for (b=a->blocks; NEXT(b); b=NEXT(b)) {

	if ((blocktype & MEMORY_ALLOCATED) && ! ISFREE(b)) {
	    rc = (*m)(MEMORY_ALLOCATED, (Pointer)((ulong)b+USER), 
		      (FSIZE(b)-USER), p);
	    if (rc == ERROR)
		return rc;
	}
	if ((blocktype & MEMORY_FREE) && ISFREE(b)) {
	    rc = (*m)(MEMORY_FREE, (Pointer)((ulong)b+USER), 
		      (FSIZE(b)-USER), p);
	    if (rc == ERROR)
		return rc;
	}
    }

    return OK;
}


/*
 * The following routines are a set of basic building blocks for
 * manipulating the blocks of memory.  All the locking is done here.
 * 
 * Changing free list information (next_free, prev_free, and free bit
 * of next_block) is done only under protection of a lock on the appropriate
 * free list.  This information is believed only after locking the
 * appropriate free list.
 *
 * Changing the block list information for a given block (next_block of
 * that block, prev_block of the next block) is only done if you own the
 * block.  You gain ownership of a block by removing it from the free
 * list, or by acting on behalf of someone else who has (e.g. in afree).
 * You lose ownership of a block by putting it back on its free list.
 * Note that the prev_block pointer is owned by the previous block, not
 * the block that the pointer is in.  Thus, the prev_block pointer can only
 * be believed after locking the free list you think the block must be on,
 * thus guaranteeing that its ownership will not change.  Similarly, the
 * free bit of the next block can only be believed after locking the free
 * list you think it must be on.
 *
 * insfree   Puts a block on the appropriate free list.  You must own the
 *           block at this point, and you then lose ownership.
 * 
 * getfree   Gets the first block from a given free list.  You gain
 *           ownership of the block.
 * 
 * getpool   Chop a block off the pool.
 * 
 * split     Split a block, and return the extra piece to its free list.
 *           You must own the block to do this.
 * 
 * mergeprev Try to merge the given block with the previous block.  Note
 *           that this requires carefully ascertaining the free status of
 *           the previous block.  Returns the address of the merged block
 *           if successful, the address of the given block if not.
 * 
 * mergenext Try to merge the given block with the next block.  This requires
 *           carefully ascertaining the free status of the next block.
 *
 * mergepool Try to merge the given block with the pool.  Returns 1 for
 *           success, in which case the given block is no longer a valid
 *           block, or returns 0 for failure.
 */

static void
insfree(struct list *l, struct block *b)
{
    struct block *nf;
    DXlock(&l->list_lock, 0);		/* lock the list */
    nf = l->block.next_free;		/* nf is the previous first block */
    b->next_free = nf;			/* before current first block */
    b->prev_free = &l->block;		/* after fake block at head of list */
    SETFREE(b);				/* set the free bit */
    if (nf)				/* if there was a first block */
	nf->prev_free = b;		/* we are now before it */
    l->block.next_free = b;		/* and we are now the first block */
    DXunlock(&l->list_lock, 0);		/* DXunlock the list */
}


static struct block *
getfree(struct list *l)
{
    struct block *b, *nf;
    DXlock(&l->list_lock, 0);		/* lock the list */
    b = l->block.next_free;		/* b is first block on list */
    if (!b) {				/* is there still one there? */
	DXunlock(&l->list_lock, 0);	/* no, so unlock */
	return NULL;			/* and return failure */
    }
    nf = b->next_free;			/* nf is the next free block */
    l->block.next_free = nf;		/* it's now the first block */
    if (nf)				/* if there was a next block */
	nf->prev_free = &(l->block);	/* set its prev field */
    CLEARFREE(b);			/* b is no longer on free list */
    COUNT(l->satisfied);		/* statistics */
    DXunlock(&l->list_lock, 0);		/* unlock the list */
    return b;
}


static struct block *
getpool(struct arena *a, ulong n, int exp)
{
    struct block *b, *pool;

    DXlock(&a->pool_lock, 0);		/* lock the pool */
    if (a->pool_size < n) {		/* is pool big enough? */
	if (!exp || !expand(a, n)) {	/* no; can we expand? */
	    DXunlock(&a->pool_lock, 0);	/* no; unlock */
	    return NULL;		/* return failure */
	}
    }
    b = a->pool;			/* take from beginning of pool */
    pool = (struct block *)((ulong)b+n);/* new pool block */
    a->pool = pool;			/* record the new pool */
    a->pool_size -= n;			/* record new size */
    COUNT(a->pool_satisfied);		/* statistics */
    b->next_block = pool;		/* new pool comes after b */
    pool->prev_block = b;		/* b comes before new pool */
    pool->next_block = NULL;		/* nothing comes after */
    DXunlock(&a->pool_lock, 0);		/* unlock the pool */
    return b;
}


static struct block *
mergeprev(struct arena *a, struct block *b)
{
    struct block *pb, *nb, *nf, *pf;
    struct list *l;
    ulong size;

    pb = b->prev_block;			/* pb is our previous block */
    nb = b->next_block;			/* nb is our next block */
    if (!ISFREE(pb)) return b;		/* quick check before locking */
    l = flist(a, (ulong)b-(ulong)pb);	/* l is the free list it may be on */
    DXlock(&l->list_lock, 0);		/* lock that list */
    if (b->prev_block!=pb || !ISFREE(pb)){ /* has pb changed? */
	DXunlock(&l->list_lock, 0);	/* yes, unlock list */
	return b;			/* and return original block */
    }					/* it's ok, we locked the right list */
    nf = pb->next_free;			/* nf is next free block */
    pf = pb->prev_free;			/* pf is previous free block */
    pf->next_free = nf;			/* new next free block */
    if (nf)				/* if there was a next free block */
	nf->prev_free = pf;		/* it has a new prev free block */
    pb->next_block = nb;		/* this also does CLEARFREE */
    nb->prev_block = pb;		/* and our next block's prev block */
    DXunlock(&l->list_lock, 0);		/* unlock the free list */

    if (maxFreedBlock < (size = FSIZE(pb)))
	maxFreedBlock = size;

    return pb;				/* p is new merged block */
}


static void
mergenext(struct arena *a, struct block *b)
{
    struct block *nb, *nf, *pf, *nn;
    struct list *l;
    ulong size, n;

    nb = b->next_block;			/* nb is our next block */
    nn = NEXT(nb);			/* nn is our next-next block */
    n = (ulong)nn - (ulong)nb;	/* n is its size */
    l = flist(a, n);			/* l is the free list it may be on */
    DXlock(&l->list_lock, 0);		/* lock that free list */
    if (NEXT(nb)!=nn || !ISFREE(nb)) {	/* has nb changed? */
	DXunlock(&l->list_lock, 0);	/* yes, unlock list */
	return;				/* and return original block */
    }					/* it's ok, we locked the right list */
    nf = nb->next_free;			/* nf is next free block */
    pf = nb->prev_free;			/* pf is previous free block */
    pf->next_free = nf;			/* new next free block */
    if (nf)				/* if there was a next free block */
	nf->prev_free = pf;		/* it has a new prev free block */
    nn->prev_block = b;			/* it has a new previous block */
    b->next_block = nn;			/* we have a new next block */
    DXunlock(&l->list_lock, 0);		/* unlock the free list */

    if (maxFreedBlock < (size = FSIZE(b)))
	maxFreedBlock = size;

    return;				/* and return merged block */
}


static struct block *
mergepool(struct arena *a, struct block *b)
{
    struct block *nb;

    nb = b->next_block;			/* nb is our next block */
    DXlock(&a->pool_lock, 0);		/* lock the pool */
    if (nb != a->pool) {		/* is the pool our next? */
	DXunlock(&a->pool_lock, 0);	/* no, unlock list */
	return NULL;			/* and return failure */
    }					/* otherwise, remove from free list */
    a->pool_size += SIZE(b);		/* add to pool size */
    a->pool = b;			/* we are now new pool */
    b->next_block = NULL;		/* so no next block */
    DXunlock(&a->pool_lock, 0);		/* unlock the free list */

    if (maxFreedBlock < a->pool_size)
	maxFreedBlock = a->pool_size;

    return b;				/* and return merged block */
}


static void
split(struct arena *a, struct block *b, ulong n, int merge)
{
    struct block *nf, *nn;
    struct list *l;
    ulong m;

    m = SIZE(b) - n;			/* m size of trimmed-off piece */
    if (m < MINBLOCK)			/* will that satisfy min? */
	return;				/* no, don't split */
    nf = (struct block *) ((ulong)b + n);/* nf is our new next block */
    nn = b->next_block;			/* nn will be our next-next block */
    nf->prev_block = b;			/* new next blocks prev is b */
    nf->next_block = nn;		/* new next blocks next is nn */
    b->next_block = nf;			/* our new next block is nf */
    nn->prev_block = nf;		/* next-next blocks prev is nf */
    if (merge && a->merge) {		/* try to merge in split-off blocks */
	if (mergepool(a, nf))		/* merge with pool? */
	    return;			/* yes, we're done */
	mergenext(a, nf);		/* try to merge with next block */
	m = SIZE(nf);			/* its new merged size */
    }
    l = flist(a, m);			/* l is the free list for new piece */

    insfree(l, nf);			/* put the new block on free list */
}


/*
 * Here are the basic arena malloc, realloc, and free routines,
 * built using the free list manipulation routines above.
 */

static char *
amalloc(struct arena *a, ulong n)
{
    struct block *b = NULL;
    struct list *rl, *l, *last;
    int align;

    if (n > a->max_user) {
	DXSetError(ERROR_NO_MEMORY,
		   "block of %u bytes is too large for %s arena",
		   n, a->name);
	return NULL;
    }

    n = n + USER;			/* add overhead */
    align = a->align;			/* alignment boundary for this arena */
    if (n<MINBLOCK) n = MINBLOCK;	/* minimum size */
    n = ROUND(n, align);		/* round up to align */

    rl = alist(a, n);			/* our list to start from */
    COUNT(rl->requested);		/* statistics */
    b = getfree(rl);			/* try to get one there */
    if (b) {				/* got one? */
	AFILL((char *)b+USER, n-USER);  /* for debug, fill w/ pattern */
	return (char *)b + USER;	/* yes, return user's portion */
    }
    n = rl->min;			/* so block goes to rl when freed */

    last = a->max_list;			/* last list to look at */
    for (l=rl+1; l<=last; l++) {	/* search the free lists */
	if (l->block.next_free) {	/* is there a block to try for? */
	    b = getfree(l);		/* try to get a block */
	    if (b) {			/* got one? */
		split(a, b, n, 0);	/* yes, split off extra */
		AFILL((char *)b+USER, n-USER); /* for debug, fill w/ pattern */
		return (char *)b+USER;	/* return user's portion */
	    }
	}
    }

    b = getpool(a, n, 1);		/* no free block; try the pool */
    if (b) {				/* got one? */
	AFILL((char *)b+USER, n-USER);  /* for debug, fill w/ pattern */
	return (char *)b + USER;	/* yes, return user's portion */
    }

    return NULL;			/* return failure, ecode already set */
}


static Error
afree(struct arena *a, char *x)
{
    struct list *l;
    struct block *b;
    ulong size;

    b = (struct block *) (x - USER);	/* convert user to block */
    CHECK(b);				/* check for corruption */
    FFILL(x, SIZE(b)-USER);	        /* for debug, fill w/ pattern */
    if (a->merge) {			/* merge? */
	b = mergeprev(a, b);		/* try to merge with prev block */
	if (mergepool(a, b))		/* merge with pool? */
	    return OK;			/* yes, we're done */
	mergenext(a, b);		/* try to merge with next block */
    }

    size = SIZE(b);
    if (size > maxFreedBlock)
	maxFreedBlock = size;

    l = flist(a, size);			/* free list to put it on */
    insfree(l, b);			/* put it there */
    return OK;
}


static char *
arealloc(struct arena *a, char *x, ulong n)
{
    struct block *b, *nb;
    char *y;
    int align;
    ulong m, bs;

    b = (struct block *) (x - USER);	/* convert user to block */
    CHECK(b);				/* check for corruption */
    /*s = SIZE(b);*/                    /* initial size of b */
    m = n + USER;			/* add overhead */
    if (m<MINBLOCK) m = MINBLOCK;	/* minimum size */
    align = a->align;			/* alignment boundary for this arena */
    m = ROUND(m, align);		/* round up to align */

    for (;;) {
	bs = SIZE(b);			/* current size of block b */
	if (m<=bs) {			/* if already big enough */
#if DEBUGGED
	    if (m - s > 0)              /* for debug, if growing fill mem */
		AFILL((char *)b + s, m - s); /*        with alloc pattern */
	    else if (s - m > 0)         /* for debug, if shrinking fill mem */
		FFILL((char *)b + m, s - m); /*           with free pattern */
#endif
	    split(a, b, m, 1);		/* split */
	    return x;			/* and return */
	}
	nb = b->next_block;		/* look at next block: */
	if (!ISFREE(nb))		/* is it free? */
	    break;			/* no - go malloc/free */
	if (bs+FSIZE(nb) < m)		/* is it big enough? */
	    break;			/* no - go malloc/free */
	mergenext(a, b);		/* try to merge it */
    }					/* and check our size again */

    y = amalloc(a, n);			/* allocate a block the right size */
    if (y) {				/* success? */
	memcpy(y, x, bs-USER);		/* yes, copy the data */
	afree(a, x);			/* free old block */
    }
    return y;				/* return the user's new block */
}


/*
 * Setup for the various architectures.  There are a number of possible
 * variations for the starting point of each arena, the initial size,
 * the maximum user block size, the routine to get more memory, the
 * memory chunk increment, and the maximum arena size.
 */

#define K   *1024
#define MEG K K
                                        /* the following are in mem.c:       */
extern Error _dxfsetmem(ulong limit);	/*             set shared mem size   */
extern Error _dxfinitmem();		/*             initialize shared mem */

extern Pointer _dxfgetmem(Pointer base, ulong n); /*   expand shared segment */
extern Pointer _dxfgetbrk(Pointer base, ulong n); /*   use sbrk to get mem   */

extern int _dxfinmem(Pointer x);        /* returns true if mem in arena      */

static int threshhold = 1 K;		/* default small vs large threshhold */
static ulong small_size = 0;		/* 0 means compute at run time */
static ulong large_size = 0;		/* 0 means compute at run time */
static ulong total_size = 0;		/* 0 means compute at run time */
static int sm_lg_ratio = 0;		/* 0 means compute at run time */

#define SMALL_INIT	small_size	/* potentially user specifiable */
#define SMALL_MAX_USER	threshhold	/* potentially user specifiable */
#define SMALL_INCR	0		/* can't enlarge */
#define SMALL_MAX_SIZE	SMALL_INIT	/* can't enlarge past initial size */
#define SMALL_MERGE	0		/* don't merge blocks as freed */
#define LARGE_BASE	(ulong)small + small_size /* large is just past small */
#define LARGE_MAX_USER	large_size	/* this is largest possible block */
#define LARGE_MERGE	1		/* do merge blocks as they're freed */


#if ibmpvs
#define initvalues
extern int end;				/* linker-provided end of used data  */
#define SMALL_BASE	SVS_sh_base	/* start at shared base */
#define SMALL_GET	_dxfgetmem	/* just returns ok */
#define LARGE_GET	_dxfgetmem	/* just returns ok */
#define LARGE_INIT	4 MEG		/* doesn't matter much */
#define LARGE_INCR	4 MEG		/* doesn't matter much */
#define LOCAL_BASE	(ulong)&end	/* local arena is in data segment */
#define LOCAL_INIT	1 MEG		/* need relatively fine granularity */
#define LOCAL_MAX_USER	LOCAL_MAX_SIZE	/* largest possible block */
#define LOCAL_GET	_dxfgetbrk	/* uses sbrk to get more memory */
#define LOCAL_INCR	1 MEG		/* need relatively fine granularity */
#define LOCAL_MAX_SIZE	12 MEG		/* may be too conservative */
#define LOCAL_MERGE	1		/* merge local blocks when freed */
#define SIZE_ROUND	4 MEG		/* doesn't matter much */
#define MALLOC_LOCAL	1		/* provide malloc from local arena */
#define SMALL(x) ((ulong)x>=(ulong)small && (ulong)x<(ulong)large)
#define LARGE(x) ((ulong)x>=(ulong)large)   /* assume local is below large */
#endif

/* 
 * SHMLBA on sgi is unsigned; cast it to signed before using it
 * in comparisons.
 */
#if sgi || solaris      /* MP capable archs */
#define initvalues
#ifndef SHMLBA
#define SHMLBA          2 MEG
#endif
#define SMALL_BASE	0		/* let system assign address */ 
#define SMALL_GET	_dxfgetmem	/* shared mem or data segment */
/* #define SMALL_INIT	4 MEG 	        -* potentially user specifiable */ 
#define LARGE_GET	_dxfgetmem	/* shared mem or data segment */
#define LARGE_INIT	16 MEG          /* initial size if data segment */
/* #define LARGE_INIT	large_size      -* allocate all space up front */ 
#define LARGE_INCR	8 MEG 		/* size to grow data seg by */
#define SIZE_ROUND	SHMLBA		/* min alignment requirement */
                                        /* don't set any malloc_xxx flags */
#define SMALL(x) ((ulong)x>=(ulong)small && (ulong)x<(ulong)small+small->size)
/* #define LARGE(x) ((ulong)x>=(ulong)large && (ulong)x<(ulong)large+large->size) */
#define LARGE(x)        _dxfinmem(x)    /* turns into a func now, ugh. */
#endif

#if ibm6000             /* MP capable arch */
#define initvalues
#ifndef SHMLBA
#define SHMLBA          2 MEG
#endif
#define SMALL_BASE	0		/* let system assign address */ 
#define SMALL_GET	_dxfgetmem	/* expanding data segment */
#define LARGE_GET	_dxfgetmem	/* expanding data segment */
#define LARGE_INIT	2 MEG           /* initial size if data seg */
/* #define LARGE_INIT	large_size      -* allocate all space up front */
#define LARGE_INCR	2 MEG 		/* size to grow by */
#define SIZE_ROUND	SHMLBA		/* min alignment requirement */
#define MALLOC_GLOBAL   1               /* provide malloc from global arena */
#define SMALL(x) ((ulong)x<(ulong)large)  /* nothing outside global mem */
#define LARGE(x) ((ulong)x>=(ulong)large)
#endif


#if os2
#define initvalues
#define SMALL_BASE    0               /* use data segment */
#define SMALL_GET     _dxfgetmem      /* expand by using DosSetMem */
#define LARGE_GET     _dxfgetmem      /* expand by using DosSetMem */
#define LARGE_INIT    2 MEG           /* doesn't matter; consistent w/ sgi */
#define LARGE_INCR    2 MEG           /* doesn't matter; consistent w/ sgi */
#define SIZE_ROUND    2 MEG           /* doesn't matter; consistent w/ sgi */
#define MALLOC_NONE   1               /* co-exist with system malloc */
#define SMALL(x) ((ulong)x<(ulong)large)
#define LARGE(x) ((ulong)x>=(ulong)large)
#endif

#if defined(intelnt) || defined(WIN32)
#define initvalues
#define SMALL_BASE    0               /* use data segment */
#define SMALL_GET     _dxfgetmem      /* expand by using DosSetMem */
#define LARGE_GET     _dxfgetmem      /* expand by using DosSetMem */
#define LARGE_INIT    2 MEG           /* doesn't matter; consistent w/ sgi */
#define LARGE_INCR    2 MEG           /* doesn't matter; consistent w/ sgi */
#define SIZE_ROUND    2 MEG           /* doesn't matter; consistent w/ sgi */
#define MALLOC_NONE   1               /* provide malloc from global arena */
#define SMALL(x) ((ulong)x<(ulong)large)
#define LARGE(x) ((ulong)x>=(ulong)large)
#endif

#ifdef	linux
#define initvalues
#define SMALL_BASE    0               /* use data segment */
#define SMALL_GET     _dxfgetmem      /* expand by using DosSetMem */
#define LARGE_GET     _dxfgetmem      /* expand by using DosSetMem */
#define LARGE_INIT    2 MEG           /* doesn't matter; consistent w/ sgi */
#define LARGE_INCR    2 MEG           /* doesn't matter; consistent w/ sgi */
#define SIZE_ROUND    2 MEG           /* doesn't matter; consistent w/ sgi */
#define MALLOC_NONE   1               /* provide malloc from global arena */
#define SMALL(x) ((ulong)x<(ulong)large)
#define LARGE(x) ((ulong)x>=(ulong)large)
#endif

#ifdef freebsd
#define initvalues
#define SMALL_BASE    0               /* use data segment */
#define SMALL_GET     _dxfgetmem      /* expand by using DosSetMem */
#define LARGE_GET     _dxfgetmem      /* expand by using DosSetMem */
#define LARGE_INIT    2 MEG           /* doesn't matter; consistent w/ sgi */
#define LARGE_INCR    2 MEG           /* doesn't matter; consistent w/ sgi */
#define SIZE_ROUND    2 MEG           /* doesn't matter; consistent w/ sgi */
#define MALLOC_NONE   1               /* provide malloc from global arena */
#define SMALL(x) ((long)x<(long)large)
#define LARGE(x) ((long)x>=(long)large)
#endif

#ifdef	cygwin
#define initvalues
#define SMALL_BASE    0               /* use data segment */
#define SMALL_GET     _dxfgetmem      /* expand by using DosSetMem */
#define LARGE_GET     _dxfgetmem      /* expand by using DosSetMem */
#define LARGE_INIT    2 MEG           /* doesn't matter; consistent w/ sgi */
#define LARGE_INCR    2 MEG           /* doesn't matter; consistent w/ sgi */
#define SIZE_ROUND    2 MEG           /* doesn't matter; consistent w/ sgi */
#define MALLOC_NONE   1               /* provide malloc from global arena */
#define SMALL(x) ((ulong)x<(ulong)large)
#define LARGE(x) ((ulong)x>=(ulong)large)
#endif

#if alphax              /* single processor, but can't replace malloc */
#define initvalues
#define SMALL_BASE	0               /* normally use end of data seg */
#define SMALL_GET	_dxfgetmem	/* option to use shared mem */
#define LARGE_GET	_dxfgetmem	/* option to use shared mem */
#define LARGE_INIT	2 MEG           /* initial data segment size */
/* #define LARGE_INIT	large_size      -* get all space at startup */
#define LARGE_INCR	2 MEG		/* min size to expand by */
#define SIZE_ROUND	2 MEG		/* min alignment requirement */
#define MALLOC_NONE   1                 /* co-exist with system malloc */
#define SMALL(x) ((ulong)x<(ulong)large)
#define LARGE(x) ((ulong)x>=(ulong)large)
#endif

#if hp700              /* single processor, NO option to use shmem */
#define initvalues
#define SMALL_BASE	0               /* normally end of data seg */
#define SMALL_GET	_dxfgetbrk	/* NO option to use shared mem */
#define LARGE_GET	_dxfgetbrk	/* NO option to use shared mem */
#define LARGE_INIT	2 MEG           /* initial data segment size */
/* #define LARGE_INIT	large_size      -* get all space at startup */
#define LARGE_INCR	2 MEG		/* min size to expand by */
#define SIZE_ROUND	2 MEG		/* min alignment requirement */
#define MALLOC_NONE   1                 /* co-exist with system malloc */
#define SMALL(x) ((ulong)x<(ulong)large)
#define LARGE(x) ((ulong)x>=(ulong)large)
#endif

#if defined(macos)
#define initvalues
#define SMALL_BASE    0               /* use data segment */
#define SMALL_GET     _dxfgetmem      /* expand by using DosSetMem */
#define LARGE_GET     _dxfgetmem      /* expand by using DosSetMem */
#define LARGE_INIT    2 MEG           /* doesn't matter; consistent w/ sgi */
#define LARGE_INCR    2 MEG           /* doesn't matter; consistent w/ sgi */
#define SIZE_ROUND    2 MEG           /* doesn't matter; consistent w/ sgi */
#define MALLOC_NONE   1               /* provide malloc from global arena */
#define SMALL(x) ((ulong)x<(ulong)large)
#define LARGE(x) ((ulong)x>=(ulong)large)
#endif

#if !defined(initvalues)		/* default for other platforms */
extern int end;				/* linker-provided end of used data  */
#define SMALL_BASE	(ulong)&end	/* start at end of data segment */
#define SMALL_GET	_dxfgetbrk	/* expand by using sbrk */
#define LARGE_GET	_dxfgetbrk	/* expand by using sbrk */
#define LARGE_INIT	2 MEG           /* initial data segment size */
/* #define LARGE_INIT	large_size      -* get all space at startup */
#define LARGE_INCR	2 MEG		/* min size to expand by */
#define SIZE_ROUND	2 MEG		/* min alignment requirement */
#define MALLOC_GLOBAL	1		/* provide malloc from global arena */
#define SMALL(x) ((ulong)x<(ulong)large)
#define LARGE(x) ((ulong)x>=(ulong)large)
#endif


/*
 * The initialization routine _initmemory uses the parameters defined
 * above to create up to three arenas: small and large global arenas,
 * and a local arena.  The SMALL and LARGE macros enable the users of
 * the arenas to tell which arena a given pointer belongs to.
 *
 */ 

static struct arena *small = NULL;
static struct arena *large = NULL;
static struct arena *local = NULL;

#if linux

static int
getProcMemInfo()
{
    long int memsize = 0, swapsize = 0;
    FILE *fp = NULL;

    fp = fopen("/proc/meminfo", "r");
    if (fp)
    {
	char str[256];

	while ((memsize <= 0 || swapsize <= 0) && fscanf(fp, "%s", str) == 1)
	{
	    if (! strcmp(str, "MemTotal:"))
	    {
		fscanf(fp, "%lu", &memsize);
		fscanf(fp, "%s", str);
		if (!strcmp(str, "kB"))
		    memsize >>= 10;
		else
		    memsize = 0;
	    }
	    else if (! strcmp(str, "SwapFree:"))
	    {
		fscanf(fp, "%lu", &swapsize);
		fscanf(fp, "%s", str);
		if (!strcmp(str, "kB"))
		    swapsize >>= 10;
		else
		    swapsize = 0;
	    }
	}

	fclose(fp);
    }

    if (memsize > swapsize)
        memsize = swapsize;

    return memsize;
}

#endif

/* cygwin: 
   to obtain available memory size in cygwin, the standard method seems to  
   be sysconf(), but it doesn't work in Win98.  Instead, we use WIN32 way.  
   notes: 
   (1) GlobalMemoryStatus() reports a rather small value as *available*  
   physical memory (11MB or so); we don't use that value.   
   (2) cygwin has 384MB limit by default.  See cygwin User's Guide.   
*/ 
  
#ifdef cygwin 
/* include <windows.h> causes errors; put minimum defs here for memory api */ 
#define WINAPI __stdcall 
#define FALSE 0 
#define MEM_FREE            0x10000 
typedef void VOID; 
typedef void *PVOID; 
typedef const void *LPCVOID; 
typedef unsigned long DWORD; 
typedef unsigned long ULONG; 
typedef struct _MEMORY_BASIC_INFORMATION { 
       PVOID BaseAddress; 
       PVOID AllocationBase; 
       DWORD AllocationProtect; 
       DWORD RegionSize; 
       DWORD State; 
       DWORD Protect; 
       DWORD Type; 
} MEMORY_BASIC_INFORMATION,*PMEMORY_BASIC_INFORMATION; 
typedef struct _MEMORYSTATUS { 
       DWORD dwLength; 
       DWORD dwMemoryLoad; 
       DWORD dwTotalPhys; 
       DWORD dwAvailPhys; 
       DWORD dwTotalPageFile; 
       DWORD dwAvailPageFile; 
       DWORD dwTotalVirtual; 
       DWORD dwAvailVirtual; 
} MEMORYSTATUS,*LPMEMORYSTATUS; 
DWORD WINAPI VirtualQuery(LPCVOID,PMEMORY_BASIC_INFORMATION,DWORD); 
VOID WINAPI GlobalMemoryStatus(LPMEMORYSTATUS); 
#endif 

#if defined(intelnt) || defined(WIN32) || defined(cygwin)
Error _dxfLocateFreeMemory (ulong *size) {
	/* Basically with Windows we need to walk the memory tree to locate
	   a range of free addresses that are left in the 2GB address
	   space and then return that to allocate it for the arena. */
	ULONG initial;
	MEMORY_BASIC_INFORMATION mbi;

	initial = 0x0000FFFF; /* Addresses less than this are reserved for protection */
	*size = 0;

	while(initial <  0x7FFF0000) { /* Addresses greater are reserved for protection */
		if(VirtualQuery((LPCVOID)initial, &mbi, sizeof(MEMORY_BASIC_INFORMATION)) == 0) 
			return FALSE;
		if(mbi.State == MEM_FREE) {
			if(mbi.RegionSize >= *size) {
				*size = mbi.RegionSize;
			}
		}
		initial = (ULONG) mbi.RegionSize + (ULONG) mbi.BaseAddress;
	}

	*size -= 2097152; /* Make sure to leave at least 2M for other allocs */

	return OK;
}
#endif


int _dxf_initmemory(void)
{
    char *s;
    uint physmem = 0;  /* if total_size unset, find physical memsize in Meg */
    uint othermem = 8; /* min amount of physmem we leave for other procs */
    int sl_ratio;      /* these 2 used for small vs large arena calculations */
    int sl_offset;    	 
    float po_ratio = 1.0/8.0;   /* default is use 7/8 of physical memory */
#if DXD_HAS_RLIMIT && ! DXD_IS_MP
    struct rlimit r;
#endif
    /* these are here and are NOT included from header files because
     * all the standard header files also define malloc() which interferes
     * with other things in this file. 
     */
    extern char *getenv(GETENV_ARG);
    extern double atof(char *);

    if (small)            /* set if we've been here before */
	return OK;

#if DEBUGGED
    /* this used to be in all builds.  i changed it to be only in the
     *  debuggable build.   nsc 23oct92
     */
    s = (char *) getenv("FIND_ME");
    if (s) {
	Pointer x;
	sscanf(s, "0x%x", &x);
	DXFindAlloc(x);
    }
#endif

    /* if the total memory size hasn't already been set (using the DXmemsize()
     * call), determine the physical memory size of this machine and base
     * our memory use on some percentage of it.  nsc 23oct92
     *
     * this used to computes both physmem and othermem in total numbers of
     * bytes.  we now have customers with 4G memory machines, which means
     * we have to do careful math using only uints, never signed ints.
     * rather than try to ensure we never go signed, i changed the code to
     * compute these values in megabytes.  that gives us a 2e20 factor
     * before we overflow 32bit ints.  seems safe for now.
     */

#if defined(intelnt) || defined(WIN32) || defined(cygwin)
	if(total_size) {
		ULONG avail;

		_dxfLocateFreeMemory(&avail);
		if(avail < total_size) total_size = avail;
	}
#endif

    if (!total_size) {

#if ibm6000
        float m;
	FILE *f = popen("/usr/sbin/lsattr -E -l sys0 | grep realmem", "r");
	if (!f)
	    goto nomem;
	if (!fscanf(f, "realmem %g", &m)) {
	    pclose(f);
	    goto nomem;
	}

	physmem = (uint)(m / 1024.);
	pclose(f);
#endif

#if solaris
	physmem = (uint)((double)sysconf(_SC_PAGESIZE) / 1024. *
			 sysconf(_SC_PHYS_PAGES) / 1024.);
#endif

#if sun4
	int fd[2], n;
	char buf[1000], *b;
	pipe(fd);
	if ((n=fork())>0) {
	    close(fd[1]);
	    physmem = 0;
	    while ((n = read(fd[0], buf, sizeof(buf)-1)) > 0) {
		if (physmem == 0) {
		    buf[n] = 0;
		    b = strstr(buf, "mem =");
		    if (b)
			physmem = (uint)(atoi(b+6) / 1024.);
		}
	    }
	    close(fd[0]);
	    wait(0);
	} else if (n==0) {
	    close(1);
	    dup(fd[1]);
	    execl("/etc/dmesg", "/etc/dmesg", 0);
	    exit(0);
	} else {
	    fprintf(stderr, "can't start another process\n");
	    exit(0);
	}
#endif

#if aviion
	physmem = (uint)(sysconf(_SC_AVAILMEM) / 1024.);
#endif

#if hp700
	union pstun pstun;
	struct pst_static pst; 
	
	pstun.pst_static = &pst;
	if (pstat(PSTAT_STATIC, pstun, sizeof(pst), 1, 0) < 0)
	    goto nomem;
	
	physmem = (uint)((double)pst.physical_memory / 1024. *
			 pst.page_size / 1024.);
#endif

#if alphax
	/* apparently this can't be determined at runtime.
	 * pick a default here; should match our recommendation for machine
	 * memory requirements.
	 */
	physmem = 32;
#endif

#if defined(intelnt) || defined(WIN32) || defined(cygwin)
	{
		MEMORYSTATUS memStatus;
		ULONG avail;
		
		memset(&memStatus, 0, sizeof(memStatus));
		memStatus.dwLength = sizeof(memStatus);
		GlobalMemoryStatus(&memStatus);
		
		if(memStatus.dwTotalPhys > 0)
			physmem = memStatus.dwTotalPhys;
		else /* Whoa boy big memory - limit to 4GB*/
			physmem = 4294967295U;
		
		_dxfLocateFreeMemory(&avail);
		if(avail < physmem) physmem = avail;
		
		physmem = (uint)((double)physmem / 1024.0 / 1024.0); /* This is MEG now */		
	}
#endif
	
#if os2
	physmem = 16;
#endif

#if sgi     
	/* includes crimson, indigo and onyx */
	inventory_t *inv;

#  if defined(_SYSTYPE_SVR4) || defined(SYSTYPE_SVR4)
	uint physmem2 = 0;

	while (inv=getinvent()) {
	    if (inv->inv_class==INV_MEMORY)
		if (inv->inv_type==INV_MAIN_MB) {
		    physmem = inv->inv_state;
		}
		else if (inv->inv_type==INV_MAIN) {
		    physmem2 = (uint)((double)inv->inv_state / 1024. / 1024.);
		}
	}
	if ( physmem == 0 )
	  physmem = physmem2;   /*  If no MEM/MAIN_MB, fall back to MEM/MAIN  */
#  else
	while (inv=getinvent()) {
	    if (inv->class==INV_MEMORY && inv->type==INV_MAIN) {
		physmem = (uint)((double)inv->inv_state / 1024. / 1024.);
		break;
	    }
	}
#  endif  /* SVR4 */

#  ifndef ENABLE_LARGE_ARENAS
	/*  If not running with large arena support, tell DX to forget about */
	/*    any physmem it sees about 2GB.  With an n32 dxexec, you can    */
	/*    only grab ~1.5GB of shared mem, and the default size computat. */
	/*    below will fall beneath this threshold if given 2GB physmem.   */
	if ( physmem > 2 K /* 2GB */ )
	    physmem = 2 K;
#  endif


#endif  /* sgi */
	
#if ibmpvs
	/* use all of global memory */
	physmem = SVS_sh_len / (1024*1024);
	othermem = 0;
#endif

#if linux
	if ((physmem = getProcMemInfo()) <= 0)
	{
	    struct sysinfo si;
	    int err = sysinfo(&si);
	    if(!err)
		physmem = si.totalram/(1024*1024);
	}
#endif

#if freebsd
  int mib[2],hw_physmem;
  size_t len;
  
  mib[0]=CTL_HW;
  mib[1]=HW_PHYSMEM;
  len=sizeof(hw_physmem);
  sysctl(mib,2,&hw_physmem,&len,NULL,0);
  physmem=hw_physmem/(1024*1024);
#endif

#if defined(macos)
	kern_return_t		ret;
	struct host_basic_info	basic_info;
	unsigned int		count=HOST_BASIC_INFO_COUNT;
	
	ret=host_info(host_self(), HOST_BASIC_INFO,
		(host_info_t)&basic_info, &count);
	if(ret != KERN_SUCCESS) {
		mach_error("host_info() call failed", ret);
		physmem = 32;
	} else
		physmem = (uint)((double)basic_info.memory_size/(1024.0*1024.0));
#endif
	
#if ibm6000 || hp700
      nomem:
#endif
	/* we should only get here without physmem set if one of the above
	 * cases failed, or if an architecture is added without adding an
	 * #ifdef for it.   nsc23oct92
	 */
	if (!physmem) {
	    physmem = 32;
	    EMessage("couldn't determine physical memory size; assuming %d Megabytes",
		     physmem);
	}
	
#if 1 /* debug */
	if (getenv("DX_DEBUG_MEMORY_INIT")) {
	    char buf[132];
	    
	    sprintf(buf, "initial: physmem %d, othermem %d, po_ratio %g, physical procs %d, nproc %d\n",
		    physmem, othermem, po_ratio, _dxf_GetPhysicalProcs(), DXProcessors(0));
	    write(2, buf, strlen(buf));
	}
#endif
	
#if 0 /* old code */
	/* if this is an MP machine, make the default be to leave 2/3 of the
	 * memory on the machine available to other tasks.
	 */
	if (_dxf_GetPhysicalProcs() > 1)
	    othermem = (uint)(physmem * 0.66);
#else
	/* if this is an MP machine, leave up to 5/8 of physical memory free
         * for other processes.
	 */
	if (_dxf_GetPhysicalProcs() > 1)
	    po_ratio = MIN(5.0, (float)_dxf_GetPhysicalProcs()) / 8.0;
#endif

	/* allow for the fact that on all systems but pvs, there are other
	 * jobs running, like at least the X server and dxui.  nsc 23oct92
	 */
	if (othermem && ((physmem * po_ratio) > othermem))
	    othermem = (uint)(physmem * po_ratio);

	/* and finally, don't allow more then 2G, unless ENABLE_LARGE_ARENAS.  
	 * is set.  we may support 64bit pointers, but we have other places
	 * where we compute sizes of things in bytes (like array items, etc) 
	 * and they are still int in places.  when we find and get them all
	 * converted to long or uint, then allow up to 4G, but after that
	 * we have to go to a 64 bit architecture to support this.  
	 *
	 * ENABLE_LARGE_ARENAS indicates we'll allow more than 2G arenas,
	 * because we are on a 64-bit capable machine, with 64-bit pointers
	 * and at least 64-bit ulongs.  However this "does not" mean individual
	 * memory blocks (and thus DX arrays) can be larger than 2G yet.
	 */
	total_size = (ulong)(physmem - othermem) MEG ;

#ifndef ENABLE_LARGE_ARENAS
	if ((physmem - othermem) >= 2048)
		total_size = 2047L MEG ;
#endif

#if 1  /* debug */
	if (getenv("DX_DEBUG_MEMORY_INIT")) {
	    char buf[132];
	    
	    sprintf(buf, "final: physmem %d, othermem %d, total_size %ld (%ldM)\n",
		    physmem, othermem, total_size, total_size >> 20);
	    write(2, buf, strlen(buf));
	}
#endif
    }

#if ibmpvs   /* must be less than physical; virtual memory not supported */
    if (total_size > SVS_sh_len) {
	EMessage("Requested %.1f, using %.1f Megabytes of memory", 
		 (float) (total_size >> 20), (float) (SVS_sh_len >> 20));
	total_size = SVS_sh_len;
    }
#endif


    /* at this point, total_size is set; either explicitly by the user,
     * or by the code above which bases it on the physical memory size.
     */

#if DXD_HAS_RLIMIT && ! DXD_IS_MP
    if (getrlimit(RLIMIT_DATA, &r) < 0)
	return ERROR;
    if (r.rlim_cur < total_size) {
	DXSetError(ERROR_BAD_PARAMETER, 
		   "cannot allocate %u bytes; exceeds system limit", total_size);
	return ERROR;
    }
#endif

    /*
     * Small and large arena default size computation.
     */

    /* ratio between small arena and large arena.  for small machines, give
     * a relatively larger percentage for small arena.
     */
    if (sm_lg_ratio > 0) {
	sl_ratio = sm_lg_ratio;
	sl_offset = 0;
    } else {
	if (total_size < 32L MEG) {
	    sl_ratio = 16;
	    sl_offset = 2 MEG;
	} else if (total_size < 128L MEG) {
	    sl_ratio = 24;
	    sl_offset = 3 MEG;
	} else {
	    sl_ratio = 16;
	    sl_offset = 0;
	}
    }

    /* now, if env var is set, multiply the effective size of the
     * small arena.  this is a temporary measure -- at least one user
     * has reported that he can't continue to run because the large
     * arena is fragmented by small arena overflow objects.
     */
    if ((s = getenv("DX_SMALL_ARENA_FACTOR")) != NULL) {
	float f = atof(s);

	/* enforce reasonable limits here */
	if (f > 0.0 && f < 20.0)
	    sl_ratio = (int)(sl_ratio / f);
    }

    small_size = total_size / sl_ratio + sl_offset;
    small_size = ROUNDDOWN(small_size, SIZE_ROUND); 
    if (small_size < 2 MEG)
	small_size = 2 MEG;

    large_size = total_size - small_size;
    large_size = ROUNDDOWN(large_size, SIZE_ROUND);
    if (large_size < (signed)SIZE_ROUND) {
	DXSetError(ERROR_NO_MEMORY, "memory must be larger than %.1f MB",
		    (float) ((small_size + SIZE_ROUND) >> 20));
	return ERROR;
    }

    /* set size before calling getmem() routine */
    if (_dxfsetmem(total_size) != OK)
	return ERROR;

    /* create small arena */
    small = acreate("small", SMALL_BASE, SMALL_INIT, SMALL_MAX_USER,
		    SMALL_GET, SMALL_INCR, SMALL_MAX_SIZE, SMALL_MERGE,
		    SMALL_ALIGN, SMALL_SHIFT, "large");
    if (!small)
	return ERROR;

    /* create large arena */
    large = acreate("large", LARGE_BASE, LARGE_INIT, LARGE_MAX_USER,
		    LARGE_GET, LARGE_INCR, large_size, LARGE_MERGE,
		    LARGE_ALIGN, LARGE_SHIFT, NULL);
    if (!large)
	return ERROR;

#ifdef LOCAL_BASE
    /* create local arena */
    local = acreate("local", LOCAL_BASE, LOCAL_INIT, LOCAL_MAX_USER,
		    LOCAL_GET, LOCAL_INCR, LOCAL_MAX_SIZE, LOCAL_MERGE,
		    LOCAL_ALIGN, LOCAL_SHIFT, NULL);
    if (!local)
	return ERROR;
#endif

    /* initialize after arenas are created, if necessary */
    if (_dxfinitmem() != OK)
	return ERROR;

    /* tell the user */
#if 0
    EMessage("Large arena limit is %.1f MB", (float)(large_size >> 20));
    EMessage("Small arena limit is %.1f MB", (float)(small_size >> 20));
#else
    if (!_dxd_exRemoteSlave) {
        char tmpbuf[80];
        sprintf(tmpbuf, 
          "Memory cache will use %ld MB (%ld for small items, %ld for large)\n\n", 
	  ((large_size + small_size) >> 20),
	  (small_size >> 20), (large_size >> 20));
        write(fileno(stdout), tmpbuf, strlen(tmpbuf)); 
    }
#endif

    return OK;
}


/*
 * There are four possible relationships with the system malloc,
 * which we control via some #defines:
 *
 * MALLOC_GLOBAL
 *   We provide a malloc replacement, and it operates by allocating
 *   out of the global arena.  DXAllocateLocal calls this malloc.  Net
 *   effect: local=global and we replace system malloc.
 *
 * MALLOC_LOCAL
 *   We provide a malloc replacement, and it operates by allocating
 *   out of the local arena.  DXAllocateLocal calls this malloc.  Net
 *   effect: local!=global and we replace system malloc with malloc
 *   from local arena.
 *
 * MALLOC_NONE
 *   We neither replace nor use the system malloc.  DXAllocateLocal operates
 *   by calling DXAllocate.  Net effect: local==global and we neither replace
 *   nor use system malloc, but rather co-exist with it.
 *
 * (none of the above)
 *   We do not replace system malloc, but we do call it from DXAllocateLocal.
 *   This is used on sgi to simulate local memory by calling malloc, which
 *   uses process-specific data segment.
 *
 * Note that because of the potential dependency
 * of printf on malloc, it may not be possible to produce any messages
 * during initialization before the memory system is initialized.
 */

#if !MALLOC_LOCAL && !MALLOC_GLOBAL && !MALLOC_NONE
extern char *malloc(unsigned int n);
extern char *realloc(char *x, unsigned int n);
extern int free(char *x);
#endif

#if MALLOC_LOCAL
char *malloc(unsigned int n)
{
    if (!local && !DXinitdx())
	return NULL;
    return amalloc(local, n);
}
char *realloc(char *x, unsigned int n)	{return arealloc(local, x, n);}
int free(char *x)			{return afree(local, x);}
#endif

#if MALLOC_GLOBAL
char *malloc(unsigned int n)		{return DXAllocate(n);}
char *realloc(char *x, unsigned int n)	{return DXReAllocate(x, n);}
int free(char *x)			{return DXFree(x);}
#endif

#if MALLOC_NONE
#define malloc(n)			DXAllocate(n)
#define realloc(x,n)			DXReAllocate(x,n)
#define free(x)				DXFree(x)
#endif


/*
 * These are the libsvs ancillary external routines for controlling
 * parameters, debugging, setting the scavenger, etc.
 */

Error DXmemsize(ulong size)    /* obsolete */
{
    return DXSetMemorySize(size, 0);
}

Error DXSetMemorySize(ulong size, int ratio)
{
#if DXD_HAS_RLIMIT && ! DXD_IS_MP
    struct rlimit r;
#endif

    if (small) {
	DXWarning("can't set size after initialization");
	return ERROR;
    }

    if (size > 0)
        total_size = size;

    if (ratio > 0)
	sm_lg_ratio = ratio;

#if DXD_HAS_RLIMIT && ! DXD_IS_MP
    if (getrlimit(RLIMIT_DATA, &r) < 0)
	return ERROR;
    if (r.rlim_cur < total_size) {
	DXSetError(ERROR_BAD_PARAMETER, 
		   "cannot allocate %u bytes; exceeds system limit", size);
	return ERROR;
    }
#endif

    return OK;
}

Error DXGetMemorySize(ulong *sm, ulong *lg, ulong *lo)
{
    if (sm)
	*sm = small ? small->max_size : 0;

    if (lg)
	*lg = large ? large->max_size : 0;

    if (lo)
	*lo = local ? local->max_size : 0;

    return OK;
}

Error DXGetMemoryBase(Pointer *sm, Pointer *lg, Pointer *lo)
{
    if (sm)
	*sm = (Pointer)small;

    if (lg)
	*lg = (Pointer)large;

    if (lo)
	*lo = (Pointer)local;

    return OK;
}


static int trace = 0;
static int every = 0;
Error DXTraceAlloc(int t)
{
    if (every++ < trace) {
	if (trace != t)
	    trace = t;
	return OK;
    }

    if (!acheck(small) || !acheck(large))
	return ERROR;

    if (local && !acheck(local))
	return ERROR;

    if (trace != t)
	trace = t;

    every = 0;
    return OK;
}

/* new code */

/* executive routines - currently internal entry points.  these should
 *  be formalized and exposed as part of libdx
 */
extern Error _dxf_ExRunOn(int pid, Error (*func)(), Pointer arg, int size);
extern Error _dxf_ExRunOnAll(Error (*func)(), Pointer arg, int size);


/*
 * request the memory manager to call the callback routine with one or more
 *  of the following types of memory blocks:  free, allocated, private.
 */
Error
DXDebugAlloc(int arena, int blocktype, MemDebug m, Pointer p)
{
    Error rc;

    if (small && (arena & MEMORY_SMALL)) {
	rc = adebug(small, blocktype, m, p);
	if (rc == ERROR)
	    return rc;
    }
    if (large && (arena & MEMORY_LARGE)) {
	rc = adebug(large, blocktype, m, p);
	if (rc == ERROR)
	    return rc;
    }
    return OK;
}

struct dbparms {
    int blocktype;
    MemDebug m;
    Pointer p;
};


static Error
adebug_local_wrapper(Pointer p)
{
    struct dbparms *d = (struct dbparms *)p;

    adebug(local, d->blocktype, d->m, d->p);
    return OK;
}

Error
DXDebugLocalAlloc(int which, int blocktype, MemDebug m, Pointer p)
{ 
    struct dbparms d;

    if (!local)
	return OK;

    if (which >= DXProcessors(0))
	return OK;

    d.blocktype = blocktype;
    d.m = m;
    d.p = p;

    /* arrange to run on correct processor(s) here */
    if (which < 0) {
	_dxf_ExRunOnAll(adebug_local_wrapper, (Pointer)&d, 
			sizeof(struct dbparms));
	return OK;
    }

    _dxf_ExRunOn(which+1, adebug_local_wrapper, (Pointer)&d, 
		 sizeof(struct dbparms));
    return OK;
}


/*
 * print information about the shared memory arenas.
 * 
 * how = 0xAB
 *  where A = 0x00 for all, 0x10 for small, 0x20 for large, 0x40 for local
 *        B = 0x0 for summary, 0x1 for blocks and 0x2 for all blocks and lists
 */
void
DXPrintAlloc(int how)
{
    int maxsize = 0;
    int used = 0; 
    int free = 0; 
    int overflow = 0;
    struct printvals v;

    /* new, user friendly memory message */
    if (how == 0) {
	if (small) {
	    aprint(small, how & 0x0F, &v);
	    maxsize += small->max_size;
	    overflow += small->overflowed;
	    used += v.used + v.header;
	    free += v.free + v.pool + (small->max_size - v.size);
	}

	if (large) {
	    aprint(large, how & 0x0F, &v);
	    maxsize += large->max_size;
	    overflow += large->overflowed;
	    used += v.used + v.header;
	    free += v.free + v.pool + (large->max_size - v.size);
	}

	EMessage("%u bytes total: %u in use, %u free",
		 maxsize, used, free);

	if (overflow > 0)
	    EMessage("%d objects have been forced to overflow to another area", 
		overflow);

	return;
    }


    /* else look at high byte of how to see what arena(s) are of interest */
    switch (how & 0xF0) {
      case 0:
	if (small) aprint(small, how & 0x0F, NULL);
	if (large) aprint(large, how & 0x0F, NULL);
	break;

      case MEMORY_SMALL:
	if (small) aprint(small, how & 0x0F, NULL);
	break;

      case MEMORY_LARGE:
	if (large) aprint(large, how & 0x0F, NULL);
	break;

      case MEMORY_LOCAL:
	if (local) aprint(local, how & 0x0F, NULL);
	break;

    }
}

static Error
aprint_local_wrapper(int *how)
{
    aprint(local, *how, NULL);
    return OK;
}

/*
 * same as DXPrintAlloc(), but for local memory.  parms are processor
 *  number and amount of information to print.
 */
void
DXPrintLocalAlloc(int which, int how)
{
    if (!local)
	return;

    if (which >= DXProcessors(0))
	return;

    /* arrange to run on correct processor here */
    if (which < 0) {
	_dxf_ExRunOnAll(aprint_local_wrapper, &how, sizeof(int));
	return;
    }

    _dxf_ExRunOn(which+1, aprint_local_wrapper, &how, sizeof(int));
}


static Pointer find_me = 0;

void
DXFindAlloc(Pointer f)
{
    find_me = f;
}

void
DXFoundAlloc(void)
{
    /* this routine is just for setting breakpoint on dbx */
}

/*
 * Scavengers
 */

int nscavengers = 0;
static Scavenger scavengers[10];

#define NORECURSE 0

#if NORECURSE
int *already_scavenging = NULL;
#endif

Scavenger DXRegisterScavenger(Scavenger s)
{
    if (nscavengers >= 10)
	return NULL;
    scavengers[nscavengers++] = s;
    return s;
}

Error
_dxfscavenge(ulong n)
{
    int i;
    for (i=0; i<nscavengers; i++)
	if (scavengers[i](n))
	    return OK;
    return ERROR;
}

int nlscavengers = 0;
static Scavenger lscavengers[10];

Scavenger DXRegisterScavengerLocal(Scavenger s)
{
    if (nlscavengers >= 10)
	return NULL;
    lscavengers[nlscavengers++] = s;
    return s;
}

int
_dxflscavenge(ulong n)
{
    int i;
    for (i=0; i<nlscavengers; i++)
	if (lscavengers[i](n))
	    return OK;
    return ERROR;
}


/*
 * Here are the actual libsvs allocation routines, including
 * intialization, multiple-arena handling, scavenging, and
 * find_me debugging.
 */

Pointer 
DXAllocate(unsigned int n)
{
    Pointer x;

    if (!small && !DXinitdx())
	return NULL;
    if (trace && !DXTraceAlloc(trace))
	return NULL;

    for (;;) {
	
	if (n <= small->max_user) {
	    if ((x=amalloc(small, n))!=NULL)
		break;
	    DXResetError();
	}
	if ((x=amalloc(large, n))!=NULL)
	    break;

#if NORECURSE
	if (small->scavenger_running)
	    return NULL;
#endif
	small->scavenger_running++;

	if (!_dxfscavenge(n)) {
	    small->scavenger_running = 0;
	    if (DXGetError() == ERROR_NONE)
		DXSetError(ERROR_NO_MEMORY, "cannot allocate %d bytes", n);
	    return NULL;
	}

	small->scavenger_running = 0;
	DXResetError();
    }   
 
    if (FOUND(x))
	DXFoundAlloc();

    return x;
}

Pointer
DXAllocateZero(unsigned int n)
{
    Pointer x;

    x = DXAllocate(n);
    if (!x)
	return NULL;

    memset(x, 0, n);

    return x;
}

Pointer 
DXAllocateLocalOnly(unsigned int n)
{
    Pointer x;

    if (!small && !DXinitdx())
	return NULL;
    if (trace && !DXTraceAlloc(trace))
	return NULL;

    for (;;) {

	if ((x=malloc(n?n:1))!=NULL)
	    break;

#if NORECURSE
	if (small->scavenger_running)
	    return NULL;
#endif
	small->scavenger_running++;

	if (!_dxflscavenge(n)) {
	    small->scavenger_running = 0;
	    if (DXGetError() == ERROR_NONE)
		DXSetError(ERROR_NO_MEMORY, "allocate of %d bytes failed", n);
	    return NULL;
	}

	small->scavenger_running = 0;
	DXResetError();
    }

    if (FOUND(x))
	DXFoundAlloc();

    return x;
}

Pointer
DXAllocateLocalOnlyZero(unsigned int n)
{
    Pointer x;

    x = DXAllocateLocalOnly(n);
    if (!x)
	return NULL;

    memset(x, 0, n);

    return x;
}

Pointer
DXAllocateLocal(unsigned int n)
{
    Pointer x;

    x = DXAllocateLocalOnly(n);
    if (!x) {
	DXResetError();

	x = DXAllocate(n);
	if (!x)
	    return NULL;
    }

    return x;
}

Pointer
DXAllocateLocalZero(unsigned int n)
{
    Pointer x;

    x = DXAllocateLocal(n);
    if (!x)
	return NULL;

    memset(x, 0, n);

    return x;
}

Pointer 
DXReAllocate(Pointer x, unsigned int n)
{
    Pointer y;
    Scavenger s;

    if (FOUND(x))
	DXFoundAlloc();
    if (!x)
	return DXAllocate(n);
    if (trace && !DXTraceAlloc(trace))
	return NULL;

    for (;;) {

	if (SMALL(x)) {
	    if (n <= small->max_user)
		if ((y=arealloc(small, x, n))!=NULL)
		    break;
	    if ((y=amalloc(large, n))!=NULL) {
		memcpy(y, x, USIZE(x)-USER);
		afree(small, x);
		break;
	    }
	    s = _dxfscavenge;

	} else if (LARGE(x)) {
	    if ((y=arealloc(large, x, n))!=NULL)
		break;
	    s = _dxfscavenge;

	} else { /* assume local */
	    if ((y=realloc(x, n?n:1))!=NULL)
		break;
#if !MALLOC_LOCAL && !MALLOC_GLOBAL
	    DXSetError(ERROR_NO_MEMORY, "realloc of %d bytes failed", n);
#endif
	    s = _dxflscavenge;
	}

#if NORECURSE
	if (small->scavenger_running)
	    return NULL;
#endif
	small->scavenger_running++;
	if (!s(n)) {
	    small->scavenger_running = 0;
	    if (DXGetError() != ERROR_NONE)
		DXSetError(ERROR_NO_MEMORY, "realloc of %d bytes failed", n);

	    return NULL;
	}
	small->scavenger_running = 0;
	DXResetError();
    }

    if (FOUND(y))
	DXFoundAlloc();

    return y;
}


/* malloc and free get redefined depending on how the MALLOC_xxx variables */
/* get set. See comments at beginning of this file.                        */

Error
DXFree(Pointer x)
{
    if (!x)
	return OK;

    if (FOUND(x))
	DXFoundAlloc();
    if (trace && !DXTraceAlloc(trace))
	return ERROR;

    if (SMALL(x))
	return afree(small, x);

    else if (LARGE(x))
	return afree(large, x);

    else /* assume local */
#if (!MALLOC_NONE && !MALLOC_GLOBAL) || MALLOC_LOCAL
	free(x);
#else
        return ERROR;
#endif

    return OK;
}


void
DXInitMaxFreedBlock()
{
    maxFreedBlock = 0;
}

ulong
DXMaxFreedBlock()
{
    return maxFreedBlock;
}


int _dxf_GetPhysicalProcs()
{
	/* return the number of physical processors on the system.
	*  this is different from the number of processes the user
	*  asked us to use with -pN
	*/

	int nphysprocs = 0;

#if defined(linux) && (ENABLE_SMP_LINUX == 0)
    nphysprocs = 1;

#elif HAVE_SYSMP
    nphysprocs = sysmp (MP_NPROCS);	/* find the number of processors */

#elif DXD_HAS_LIBIOP
    nphysprocs = nprocs = SVS_n_cpus;

#elif HAVE_SYSCONF && defined(_SC_NPROCESSORS_ONLN)
    nphysprocs = sysconf(_SC_NPROCESSORS_ONLN);

#elif HAVE_SYS_SYSCONFIG_NCPUS
    nphysprocs = _system_configuration.ncpus; /* In Kernel space */

#elif defined(intelnt) || defined(WIN32)
#if 0
/* Not ready to deal with multiple processor Windows boxes. */
    SYSTEM_INFO sysinfo;
	GetSystemInfo(&sysinfo);
	nphysprocs = sysinfo.dwNumberOfProcessors;
#endif
    nphysprocs = 1;
#else
    nphysprocs = 1;
#endif

    return nphysprocs;
}

