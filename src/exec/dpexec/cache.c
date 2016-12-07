/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#include <time.h>
#include <dx/dx.h>
#include "config.h"
#include "cache.h"
#include "crc.h"
#include "d.h"
#include "graph.h"
#include "log.h"
#include "swap.h"
#include "utils.h"
#include "context.h"
#include "distp.h"
#include "exobject.h"

EXDictionary _dxd_exCacheDict = (EXDictionary) NULL;
static gvar *EmptyGvar;
void _dxf_ExFlushPathTags();

static char *
int32tohex (char *buf, int32 l)
{
    static char *htab = "0123456789abcdef";

    buf[8] = '\0';
    buf[7] = htab[l & 0xf]; l >>= 4;
    buf[6] = htab[l & 0xf]; l >>= 4;
    buf[5] = htab[l & 0xf]; l >>= 4;
    buf[4] = htab[l & 0xf]; l >>= 4;
    buf[3] = htab[l & 0xf]; l >>= 4;
    buf[2] = htab[l & 0xf]; l >>= 4;
    buf[1] = htab[l & 0xf]; l >>= 4;
    buf[0] = htab[l & 0xf];

    return (buf + 8);
}


EXCRC _dxf_ExGenCacheTag(char *function, int key, int n, EXCRC *in)
{
    EXCRC result;

    /* Build a cache string representing the key */
    result = _dxf_ExCRCString(EX_INITIAL_CRC, function);
    result = _dxf_ExCRCInt   (result,	 key);
    result = _dxf_ExCRCIntV  (result, 	 (int32 *)in, n);

    return (EXTAG(result));
}

/* this is what the libdx init code calls, but it is too early for 
 * the exec to init the cache at this point, so it is just a dummy
 * routine.  later in the exec init process, when the exo object pool
 * is set up, then ExCacheInit can be called.
 */
Error _dxf_initcache (void)
{
    return OK;
}

Error _dxf_ExCacheInit (void)
{
    _dxd_exCacheDict = _dxf_ExDictionaryCreateSorted(2048, FALSE, TRUE);
    EmptyGvar = _dxf_ExCreateGvar(GV_UNRESOLVED);
    /* Never want this gvar to get deleted so add an extra reference */
    ExReference(EmptyGvar);
    return (_dxd_exCacheDict == NULL? ERROR: OK);
}


/*
 * Insert an entry into the cache using a reference to an executive-style
 * object.
 */

int _dxf_ExCacheInsert (gvar *obj)
{
    char	 stringKey[8+3];
    int		 ret;

    if ((obj->reccrc & 0x80000000) == 0)
	_dxf_ExDie("Bad executive cache tag 0x%08x", obj->reccrc);

    stringKey[0] = 'X';
    int32tohex(stringKey+1, obj->reccrc);

    ExDebug("2", "DXAdd %s to cache at %f", stringKey, obj->object.lastref);

    obj->obj = DXSetObjectTag(obj->obj, obj->reccrc);

    ExTimestamp((EXO_Object)obj);

    ret = _dxf_ExDictionaryInsert (_dxd_exCacheDict, stringKey, (EXObj)obj);
    if(ret > 0) {
        /* since we are pointing to shared memory for the data we can 
         * send a size of 0.
         */
        _dxf_ExDistributeMsg(DM_INSERTCACHE, (Pointer)&(obj->reccrc), 0, 
                             TOPEERS);
    }
    return(ret);
}

int _dxf_ExCacheInsertRemoteTag (int fd, int swap)
{
    EXCRC ct;
    gvar *cachedGvar;
    char stringKey[8+3];

    if(_dxf_ExReceiveBuffer(fd, &ct, 1, TYPE_INT, swap) < 0) 
        DXUIMessage("ERROR", "bad remote cache tag");

    if ((ct & 0x80000000) == 0)
        _dxf_ExDie("Bad executive cache tag 0x%08x", ct);

    stringKey[0] = 'X';
    int32tohex(stringKey+1, ct);

    /* DPSEND outputs may be already cached */
    if((cachedGvar = (gvar *)
        _dxf_ExDictionarySearch(_dxd_exCacheDict, stringKey)) != NULL) {
        ExDelete(cachedGvar);
        return(OK);
    }
    else {
        ExDebug("2", "DXAdd RemoteTag %s to cache", stringKey);
        return(_dxf_ExDictionaryInsert(_dxd_exCacheDict, stringKey, 
                                       (EXObj)EmptyGvar));
    }
}

/*
 * Delete an entry from the cache.
 */

Error _dxf_ExCacheDelete (EXCRC key)
{
    char stringKey[8+3];
    int	ret;	

    if ((key & 0x80000000) == 0)
	_dxf_ExDie("Bad executive cache tag 0x%08x", key);

    stringKey[0] = 'X';
    int32tohex(stringKey+1, key);

    DXDebug ("1","removing cache entry '%s' in _dxf_ExCacheDelete", stringKey);
    ret = _dxf_ExDictionaryDelete (_dxd_exCacheDict, stringKey);
    if(ret > 0) {
        _dxf_ExDistributeMsg(DM_EVICTCACHE, (Pointer)&key, 
                             sizeof(EXCRC), TOPEERS);
    }
    return(ret);

}

int _dxf_ExCacheDeleteRemoteTag (int fd, int swap)
{
    EXCRC ct;
    char stringKey[8+3];

    if(_dxf_ExReceiveBuffer(fd, &ct, 1, TYPE_INT, swap) < 0) {
        DXUIMessage("ERROR", "bad remote cache tag");
        return(ERROR);
    }

    if ((ct & 0x80000000) == 0)
        _dxf_ExDie("Bad executive cache tag 0x%08x", ct);

    stringKey[0] = 'X';
    int32tohex(stringKey+1, ct);

    DXDebug ("1","removing cache entry '%s' in _dxf_ExCacheDeleteRemoteTag", stringKey);
    return(_dxf_ExDictionaryDelete(_dxd_exCacheDict, stringKey));
}

int _dxf_ExCacheListDeleteRemoteTag(int fd, int swap)
{
    CacheTagList pkg;
    char stringKey[8+3];
    int i;
    
    if(_dxf_ExReceiveBuffer(fd, &(pkg.numtags), 1, TYPE_INT, swap) < 0) {
        DXUIMessage("ERROR", "bad remote cache tag list");
        return(ERROR);
    }

    if(_dxf_ExReceiveBuffer(fd, pkg.ct, pkg.numtags, TYPE_INT, swap) < 0) {
        DXUIMessage("ERROR", "bad remote cache tag");
        return(ERROR);
    }

    stringKey[0] = 'X';
    for(i = 0; i < pkg.numtags; i++) {
        int32tohex(stringKey+1, pkg.ct[i]);
        DXDebug ("1","removing cache entry '%s' in _dxf_ExCacheListDeleteRemoteTag", stringKey);
        if(_dxf_ExDictionaryDelete(_dxd_exCacheDict, stringKey) != OK)
            DXWarning("Cache Tag %s not deleted", stringKey);
    }
    return(OK);
}    

/*
 * Locate an entry in the cache.
 */
gvar *_dxf_ExCacheSearch (EXCRC key)
{
    char	 stringKey[8+3];
    gvar	 *entry;

    if ((key & 0x80000000) == 0)
	_dxf_ExDie("Bad executive cache tag 0x%08x", key);

    stringKey[0] = 'X';
    int32tohex(stringKey+1, key);


    entry =  ((gvar*)_dxf_ExDictionarySearch (_dxd_exCacheDict, stringKey));
    if (entry)
	ExTimestamp((EXO_Object)entry);
    
    return entry;
}

/*
 * Module interface to allow inserting arbitrary data into the cache.
 * The module provides a deletion routine which is called when the
 * entry is removed from the cache.  A cost of CACHE_PERMANENT will
 * insure that the entry is not removed from the cache until the
 * application exits
 */

Error DXCacheInsert (char *id, Pointer data, Error (*pdelete)(Pointer), double cost)
{
    Object o;

    o = (Object)DXNewPrivate(data, pdelete);
    return (DXSetCacheEntry(o, cost, id, 0, 0));
}


/*
 * Module interface to allow deleting specific entries from the cache
 */

Error DXCacheDelete (char *id)
{
    return (DXSetCacheEntry(NULL, 0.0, id, 0, 0));
}

/*
 * Module interface to look up entries in the cache.  The pointer returned
 * is the pointer supplied to DXCacheInsert when the entry was inserted.
 */
Error
DXCacheSearch(char *id, Pointer *data)
{
    Object o;

    *data = NULL;

    o = DXGetCacheEntry(id, 0, 0);
    if (o)
	*data = DXGetPrivateData((Private)o);
    DXDelete (o);

    return(OK);
}


/*
 * Module interface to insert a libdx-style object into the cache.  This
 * routine takes care of referencing the object before inserting it into
 * the cache.  The libdx DXDelete routine is called when the entry is removed
 * from the cache.
 */
Error
DXCacheInsertObject(char *id, Object o, double cost)
{
    Object p;

    p = (Object)DXNewPrivate((Pointer)o, NULL);
    return (DXSetCacheEntry(p, cost, id, 0, 0));
}

/*
 * This routine is responsible for flushing all non-permanent entries from
 * the executive cache.  It walks the dictionary structure and throws
 * away anything that is not marked permanent.
 */

Error
_dxf_ExCacheFlush (int all)
{
    EXObj		obj;
    EXObj		curr;
    gvar		*gv;
    char		*key;

    if (! *_dxd_exTerminating)
        DXDebug ("1", "flushing cache");

    if(!_dxd_exRemoteSlave)
        _dxf_ExDistributeMsg(DM_FLUSHCACHE, (Pointer)&all, sizeof(int), 
                             TOSLAVES);

    _dxf_ExReclaimDisable ();

    _dxf_ExDictionaryBeginIterate (_dxd_exCacheDict);
    while ((obj = _dxf_ExDictionaryIterate (_dxd_exCacheDict, &key)) != NULL)
    {
	curr = NULL;

	switch (EXO_GetClass (obj))
	{
	case EXO_CLASS_GVAR:
	    gv = (gvar *) obj;

	    /* The variable is not currently in use */
	    if (all || gv->cost < CACHE_PERMANENT)
		curr = obj;
	    break;
	
	default:
	    break;
	}

	if (curr)
	{
            DXDebug ("1", "removing cache entry '%s'", key);
	    _dxf_ExDictionaryDeleteNoLock (_dxd_exCacheDict, key);
	}
    }

    _dxf_ExDictionaryEndIterate (_dxd_exCacheDict);
    _dxf_ExFlushPathTags();
    _dxf_ExReclaimEnable ();

    return(OK);
}

Error DXSetCacheEntryV(Object out, double cost, char *function, 
		     int key, int n, Object *in)
{
    char	 tag[8+3];
    gvar	 *entry;
    int		 i;
    EXCRC   	inTags[100];
    EXCRC	outTag;

    
    if (_dxd_exCacheDict == NULL)
	return (OK);

/*---------------------------------------------------------------*/
/* See if user has turned caching off and respect his/her wishes */
/* unless the cost is CACHE_PERMANENT                            */
/*---------------------------------------------------------------*/
    ExDebug("*1","In DXSetCacheEntryV with cost = %g and cache attr = %d",
            cost,_dxd_exCurrentFunc ? _dxd_exCurrentFunc->excache: 1);
    
    if (cost < CACHE_PERMANENT)
      if (_dxd_exCurrentFunc && !_dxd_exCurrentFunc->excache) {
        DXWarning("#4860",function);
        return OK;     /* not an error to not cache .... */
    }

    for (i = 0; i < n && i < 100; ++i)
	inTags[i] = DXGetObjectTag(in[i]);
    outTag = _dxf_ExGenCacheTag(function, key, n, inTags);

    tag[0] = 'U';
    int32tohex(tag+1, outTag);

    if (out == NULL) {
	_dxf_ExDictionaryDelete(_dxd_exCacheDict, tag);
	return (OK);
    }
    else
    {
	entry = _dxf_ExCreateGvar(GV_CACHE);
	if (entry == NULL)
	    return (ERROR);

	_dxf_ExDefineGvar(entry, out);
	entry->cost   = cost;
/* 	entry->reccrc = outTag; */
	entry->procId = exJID;
	ExTimestamp(entry);
	/* DXSetObjectTag(out, outTag); */
	return (_dxf_ExDictionaryInsert (_dxd_exCacheDict, tag, (EXObj)entry));
    }
}

Error DXSetCacheEntry(Object out, double cost, char *function,
		    int key, int n, ...)
{
    Object in[100];
    int i;
    va_list arg;

    va_start(arg,n);
    for (i = 0; i < n && i < 100; ++i)
	in[i] = va_arg(arg, Object);
    va_end(arg);

    return (DXSetCacheEntryV(out, cost, function, key, n, in));
}

Object DXGetCacheEntryV(char *function, int key, int n, Object *in)
{
    char	 tag[8+3];
    gvar	 *entry;
    int		 i;
    EXCRC        inTags[100];
    EXCRC 	outTag;
    Object	out = NULL;

    if (_dxd_exCacheDict == NULL)
	return (NULL);

    for (i = 0; i < n && i < 100; ++i)
	inTags[i] = DXGetObjectTag(in[i]);
    outTag = _dxf_ExGenCacheTag(function, key, n, inTags);

    tag[0] = 'U';
    int32tohex(tag+1, outTag);

    entry = (gvar *)_dxf_ExDictionarySearch (_dxd_exCacheDict, tag);
    if (entry) {
	ExTimestamp(entry);
	out = DXReference (entry->obj);		/* Deleted by user */
	ExDelete(entry);
    }
	
    return (out);
}

Object DXGetCacheEntry(char *function, int key, int n, ...)
{
    Object in[100];
    int i;
    va_list arg;

    va_start(arg,n);
    for (i = 0; i < n && i < 100; ++i)
	in[i] = va_arg(arg, Object);
    va_end(arg);

    return (DXGetCacheEntryV(function, key, n, in));
}
