/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#define	RECLAIM_TIMING	0

#include <stdio.h>
#include <stdlib.h>
#include <dx/dx.h>
#ifdef DXD_WIN
#include <sys/timeb.h>
#else
#include <sys/times.h>
#endif
#include "cache.h"
#include "graph.h"
#include "utils.h"
#include "swap.h"
#include "d.h"
#include "log.h"
#include "status.h"
#include "distp.h"

#define LOCK_REC() 	 (  have_rec_sem++ ? OK :   DXlock (rec_sem, 0))
#define UNLOCK_REC()	 (--have_rec_sem   ? OK : DXunlock (rec_sem, 0))

static lock_type *rec_sem;

static int		have_rec_sem	= 0;
static EXO_Object	rec_obj		= NULL;		/* for time stamp */
static int		reclaiming_mem  = 0;

extern int _dxd_exEnableDebug;

int
_dxf_ExReclaimingMemory()
{
    return(reclaiming_mem);
}

/*
 * (Un)locks the memory reclaimation so that other processors can't cause it
 * to occur.  This is necessary during some operations which assume that the
 * cache is in a consistent state.  We also set a local flag so that lower
 * level library calls will not cause things to get thrown out of the cache.
 * Note, that _dxf_ExReclaimDisable returns TRUE when someone else has
 * been mucking with the cache, and may have freed memory, which caused us
 * to wait.
 */


int
_dxf_ExReclaimDisable ()
{
    int result;

    if (have_rec_sem++)
	return FALSE;

    result = DXtry_lock(rec_sem, 0);
    if (!result)
	DXlock(rec_sem, 0);

    return(!result);
}


void _dxf_ExReclaimEnable ()
{
    UNLOCK_REC ();
}


/*
 * Initialize the cache memory scavenger.
 */

static float factor = 1.0;

Error _dxf_ExInitMemoryReclaim ()
{
    char *cp;

    if ((rec_sem = (lock_type *) DXAllocate (sizeof (lock_type))) == NULL)
	return (ERROR);

    if (DXcreate_lock (rec_sem, "cache reclamation") != OK)
	return (ERROR);

    if ((rec_obj = (EXO_Object) DXAllocate (sizeof (exo_object))) == NULL)
	return (ERROR);
    
    rec_obj->lastref = 0.0;

    if ((cp = getenv("DX_RECLAIM_FACTOR")) != NULL) {
	factor = atof(cp);
	if (factor <= 0.0 || factor >= 10.0)
	    factor = 1.0;
    }

    return (OK);
}

#if 0
cleanupMemoryReclaim()
{
    DXdestroy_lock (rec_sem);
    DXFree ((Pointer) rec_sem);
    DXFree ((Pointer) rec_obj);
}
#endif

void _dxf_ExSetGVarCost(gvar *gv, double cost)
{
    if (gv == NULL || gv->type == GV_UNRESOLVED)
	return;

    gv->cost = cost;
}

/*
 * THE FIRST COMMENT IS VERY OLD AND DESCRIBES THE ORIGINAL BEHAVIOR
 * OF THIS CODE.  IT DOES NOT WORK LIKE THAT NOW.  SEE THE SECOND
 * PARAGRAPH FOR A MORE UP TO DATE PICTURE OF HOW THIS WORKS.
 * (the description of the function it needs to perform is still valid;
 * it's the approach to how it works which is much better now.)
 *
 * This is the cache memory scavenger.  The global memory allocator calls
 * this routine when it can't allocate a block of memory.  'nbytes' is the
 * size of the block that the allocator needs to satisfy the request.
 * This routine walks the cache and selects the ten best candidates for
 * throwing away.  Currently the selection is based only on the cost of
 * creating the data stored in the cache entry.  An obvious improvement
 * would be to base the selection on the timestamp as well, so things which
 * are frequently used will hang around for a while.  Also the choice of
 * ten items is pretty arbitrary and may be far from optimal.
 *
 * NEW NEW NEW
 * first off, the timestamps on objects used to be the create time and
 * not the time of last reference, so we could not do a real LRU algorithm
 * even if we wanted to.  this was fixed, so the timestamps are the last
 * use.  now we walk the cache in least-recently-used order, throwing 
 * away objects until we have enough space to hold the object we are 
 * trying to make space for.  the memory manager was changed to bookkeep
 * the size of the largest free block during cache scavenging, so this 
 * code no longer picks an arbitrary number of objects to delete.
 *
 */

#define	N_PER_ITER	512

extern void  DXInitMaxFreedBlock(); /* from libdx/memory.c */
extern ulong DXMaxFreedBlock();     /* from libdx/memory.c */

static int nDeleted;

static int
__ExReclaimMemory (ulong nbytes)
{
    EXObj	obj;
    gvar	*gv;
    int		skipped;
    char	*key;
    CacheTagList pkg;
    uint32	 *ctp;

    _dxf_ExDictionaryBeginIterateSorted(_dxd_exCacheDict, 0);

    pkg.numtags = 0;
    ctp = pkg.ct;

    while (NULL !=
	(obj = _dxf_ExDictionaryIterateSorted(_dxd_exCacheDict, &key)))
    {
	gv = (gvar *)obj;

	if (gv->type == GV_UNRESOLVED || gv->cost == CACHE_PERMANENT)
	    continue;
	
	ExDebug ("2", "Free %s from cache", key);

        if(key[0] == 'X') {
            *ctp = strtoul(key+1, NULL, 16);
	    if (_dxf_ExDictionaryDelete (_dxd_exCacheDict, key) == OK)
	    {
                pkg.numtags++;
                ctp++;

		if (pkg.numtags >= N_CACHETAGLIST_ITEMS)
		{
		    _dxf_ExDistributeMsg(DM_EVICTCACHELIST, (Pointer)&pkg, 
                             sizeof(CacheTagList), TOPEERS);
		    pkg.numtags = 0;
		    ctp = pkg.ct;
		}
            }
        }
        else 
	    _dxf_ExDictionaryDelete(_dxd_exCacheDict, key);
	
	nDeleted ++;
	    
	if (nbytes <= DXMaxFreedBlock())
	    break;
    }

    skipped = (obj) ? 1 : 0;

    if(pkg.numtags > 0)
        _dxf_ExDistributeMsg(DM_EVICTCACHELIST, (Pointer)&pkg, 
                             sizeof(CacheTagList), TOPEERS);

    _dxf_ExDictionaryEndIterate (_dxd_exCacheDict);

    return skipped;
}

int _dxf_ExReclaimMemory (ulong nbytes)
{
    int	status;
    int	found;

    /*
     * Sorry, the cache has to stay consistent for a while.  Can't eject
     * anything at the present time.
     * Note, that if _dxf_ExReclaimDisable returns TRUE, then someone else has
     * been mucking with the cache, and may have freed memory, go ahead
     * and say there's more space.
     */

#if RECLAIM_TIMING
    DXMarkTimeLocal ("> RECLAIM");
#endif
    if (_dxf_ExReclaimDisable ())
    {
	_dxf_ExReclaimEnable ();
	return (TRUE);
    }

    reclaiming_mem = 1;

    /*
     * Get the last time this occurred if this is the first time then
     * we need to initialize this for the entire system.  So, let's 
     * assume that the last sweep just occurred.
     */

    DXDebug ("*0M", "Memory reclamation:  looking for %lu bytes", nbytes);

    status = get_status ();
    set_status (PS_RECLAIM);

    nDeleted = 0;

    /* 
     * Now we run over the cache dictionary trying to delete something
     * that gives us a block big enough.
     */
    DXInitMaxFreedBlock();

    DXDebug ("M", " initial max free blocksize %lu bytes", DXMaxFreedBlock());

    /* 
     * EXPERIMENT!!  try reclaiming just a bit more than we need, and
     * if we can't get it than try to get exactly what we do need.  the
     * idea is that we make larger holes in memory if we reclaim too much
     * and perhaps delay fragementation by some amount.
     */


    /*
     * ask for too much.  if this fails, see if we got at least
     * as much as we really need.
     */
    found = __ExReclaimMemory((ulong)(nbytes*factor));
    if (found)
	goto done;
 
    DXDebug ("M", " after deleting %d items max blocksize now %lu", 
	     nDeleted, DXMaxFreedBlock());
    DXDebug ("M", " could not get %lu bytes (%g times %lu needed)", 
	     (ulong)(nbytes*factor), factor, nbytes);

    /*
     * the first request looked at all objects which were valid to
     * discard.  there's no point in traversing the same list again.
     * just look at the largest remaining block and see if it's big enough.
     */
    found = nbytes <= DXMaxFreedBlock();
    if (found)
	goto done;

    DXDebug ("M", " going to try to compact dictionary"); 
    
    /*
     * If we still failed, clean up the dictionary and try again
     */


    if (_dxf_ExDictionaryCompact(_dxd_exCacheDict) ||
	_dxf_ExGraphCompact() ||
	_dxf_EXO_compact())
    {
       found = nbytes <= DXMaxFreedBlock();
       if (! found)
	   found = __ExReclaimMemory(nbytes);
    }

  done:
	       
    DXDebug ("M", "deleted %d items, max blocksize now %lu, found=%d", 
	     nDeleted, DXMaxFreedBlock(), found);

    reclaiming_mem = 0;

    _dxf_ExReclaimEnable ();

    set_status (status);

    return found;
}
