/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#include <stdio.h>
#include <string.h>

#include <dx/dx.h>

#include "config.h"
#include "crc.h"
#include "d.h"
#include "graph.h"
#include "utils.h"
#include "sysvars.h"
#include "distp.h"

#define	DICT_LOCK(l,d)		if (l) DXlock (&(d)->lock, exJID)
#define	DICT_UNLOCK(l,d)	if (l) DXunlock (&(d)->lock, exJID)

typedef struct _EXDictElement 	   	   *EXDictElement;
typedef struct _EXDictElementSorted	   *EXDictElementSorted;

typedef struct _EXDictElement
{
    EXDictElement		next;	/* the element chain		*/
    EXDictElement		prev;
    char			*key;	/* symbolic key			*/
    Pointer			hash;	/* hash of key			*/
    EXObj			obj;	/* data with the key		*/
} _EXDictElement;

typedef struct _EXDictElementSorted
{
    struct _EXDictElement       de;
    EXDictElementSorted		newer;	/* the sort chain		*/
    EXDictElementSorted		older;
} _EXDictElementSorted;

typedef struct _EXDictHead
{
    EXDictElement		e;	/* element pointer		*/
} _EXDictHead, *EXDictHead;

typedef struct	 _EXDictionary
{
    int				items;	/* number of in table		*/
    int				iterIndex;	/* current iterator index */
    EXDictElement		iterItem;	/* current iterator item */
    int				mask;	/* bit mask for size		*/
    int				locking;/* is locking required		*/
    int				size;	/* size of table		*/
    int				local;	/* is this a local dictionary	*/
    EXDictElement		free;	/* element free list		*/
    lock_type			lock;	/* lock if necessary		*/
    int				sorted; /* is a sort list kept?		*/
    EXDictElementSorted		newest; /* sort list head 		*/
    EXDictElementSorted		oldest;	/* sort list tail 		*/
    int				reverse;
    _EXDictHead			h[1];	/* the elements in the table	*/
} _EXDictionary;

static void _dxf_ExRemoveFromSortList(EXDictionary d, EXDictElement de);
static void _dxf_ExMakeNewest(EXDictionary d, EXDictElement de);

static EXDictionary __ExDictionaryCreate (int size, int local,
						int locking, int sorted)
{
    int			n;
    EXDictionary	d;
    Error		l;

#ifdef DICT_TIMING
DXMarkTimeLocal("D create");
#endif
    /*
     * Adjust the inputs so that they match our notion of things.  Round
     * up to a power of 2 so that the masking will work.  If a dictionary
     * is in local memory then it doesn't need locking.
     */

    size    = (size <= 0) ? 1 : _dxf_ExNextPowerOf2 (size);
    locking = local ? FALSE : locking;

    /*
     * OK, now let's construct the dictionary and fill in the blanks.
     */

    n  = sizeof (_EXDictionary);
    n += sizeof (_EXDictHead) * size;

    d = (EXDictionary) (local ? DXAllocateLocal (n) : DXAllocate (n));
    if (! d)
	_dxf_ExDie ("_dxf_ExDictionaryCreate:  DXAllocate failed");
    
    ExZero (d, n);

    d->mask	= size - 1;
    d->locking	= locking;
    d->size	= size;
    d->local	= local;

    d->sorted   = sorted;
    d->newest   = NULL;
    d->oldest   = NULL;

    if (locking)
    {
	l = DXcreate_lock (&d->lock, "Dictionary");
	if (l != OK)
	    _dxf_ExDie ("_dxf_ExDictionaryCreate:  DXcreate_lock failed");
    }

#ifdef DICT_TIMING
DXMarkTimeLocal("-D create");
#endif
    return (d);
}

/*
 * Constructs a new dictionary based upon the following characteristics:
 *
 * size		Number of buckets to create
 * local	Whether dictionary should reside in local memory
 * locking	Whether dictionary requires locking
 */
EXDictionary _dxf_ExDictionaryCreate (int size, int local, int locking)
{
    return __ExDictionaryCreate(size, local, locking, 0);
}

EXDictionary _dxf_ExDictionaryCreateSorted (int size, int local, int locking)
{
    return __ExDictionaryCreate(size, local, locking, 1);
}


/*
 * Destroys a dictionary.  We need not bother with locking here since we
 * assume that a dictionary will only be destroyed when no-one else is 
 * going to use it.
 */

Error _dxf_ExDictionaryDestroy (EXDictionary d)
{
    int			i;
    EXDictHead		h;
    EXDictElement	e, ne;

#ifdef DICT_TIMING
DXMarkTimeLocal("D destroy");
#endif
    if (! d)
	return (OK);
    
    /*
     * Get rid of the free element list.
     */

    for (e = d->free; e; e = ne)
    {
	ne = e->next;
	DXFree ((Pointer) e);
    }

    /*
     * Destroy all of the records in the table.
     */

    for (i = d->size, h = d->h; i--; h++)
    {
	for (e = h->e; e; e = ne)
	{
	    ne = e->next;
	    ExDelete (e->obj);			/* delete its value	*/
	    DXFree ((Pointer) e->key);		/* destroy the key	*/
	    DXFree ((Pointer) e);			/* get rid of the block	*/
	}
    }

    if (d->locking)
	DXdestroy_lock (&d->lock);
    
    DXFree ((Pointer) d);

#ifdef DICT_TIMING
DXMarkTimeLocal("-D destroy");
#endif
    return (OK);
}

/* Releases all unneeded memory, returned TRUE if any memory is freed */
int _dxf_ExDictionaryCompact (EXDictionary d)
{
    EXDictElement	e, ne;
    int			result;

#ifdef DICT_TIMING
DXMarkTimeLocal("D compact");
#endif
    if (! d)
	return (FALSE);
    
    DICT_LOCK (d->locking, d);

    result = d->free != NULL;
    /*
     * Get rid of the free element list.
     */
    for (e = d->free; e; e = ne)
    {
	ne = e->next;
	DXFree ((Pointer) e);
    }
    d->free = NULL;

    DICT_UNLOCK (d->locking, d);

#ifdef DICT_TIMING
DXMarkTimeLocal("-D compact");
#endif
    return (result);
}

/*
 * Empties a dictionary.  Returns true if any memory has been freed.
 */

int _dxf_ExDictionaryPurge (EXDictionary d)
{
    int			i;
    EXDictHead		h;
    EXDictElement	e, ne;
    int			result;

#ifdef DICT_TIMING
DXMarkTimeLocal("D purge");
#endif
    if (! d)
	return (FALSE);
     
    if(!_dxd_exRemoteSlave) 
        _dxf_ExDistributeMsg(DM_FLUSHDICT, (Pointer)d, 0, TOSLAVES);

    DICT_LOCK (d->locking, d);

    /*
     * Destroy all of the records in the table.
     */
    for (result = FALSE, i = d->size, h = d->h; i--; h++)
    {
	for (e = h->e, h->e = NULL; e; e = ne)
	{
	    ne = e->next;

	    /* add node to free list	*/
	    e->next = d->free;
	    d->free = e;

	    if (d->sorted)
		_dxf_ExRemoveFromSortList(d, e);

	    ExDelete (e->obj);
	    DXFree ((Pointer) e->key);

	    result = TRUE;
	}
    }

    DICT_UNLOCK (d->locking, d);

#ifdef DICT_TIMING
DXMarkTimeLocal("-D purge");
#endif
    return (result);
}


/*
 * Searches for the dictionary element associated with the given key.
 * If it is found then it is returned, otherwise NULL is returned.
 *
 * NOTE:  If the dictionary requires locking then the locking should be
 * performed outside of this routine.
 */

static EXDictElement
ExDictionarySearchE (EXDictionary d, int b, char *key, Pointer hash)
{
    EXDictElement	e;

    for (e = d->h[b].e; e; e = e->next)
	if ((hash == e->hash) && ! strcmp (key, e->key))
	{
	    if (d->sorted)
		_dxf_ExMakeNewest(d, e);
	    return (e);
	}

    return (NULL);
}


/*
 * Searches the given dictionary for an object associated with the given key.
 * If an entry is found then its value is returned, otherwise NULL is returned.
 */

EXObj _dxf_ExDictionarySearch (EXDictionary d, char *key)
{
    Pointer		h;			/* hash value		*/
    int			b;			/* bucket id		*/
    int			l;			/* locking?		*/
    EXDictElement	e;			/* the found element	*/
    EXObj		o;			/* the found object	*/

#ifdef DICT_TIMING
DXMarkTimeLocal("D search");
#endif
    if (d->items == 0)
	return (NULL);

    h = (Pointer) _dxf_ExCRCString (EX_INITIAL_CRC, key);
    b = (long) h & d->mask;
    l = d->locking;

    if (! d->h[b].e)				/* a quick out		*/
	return (NULL);

    DICT_LOCK (l, d);
    e = ExDictionarySearchE (d, b, key, h);
    if (e)
    {
	o = e->obj;
	ExReference (o);
    }
    else
	o = NULL;

    DICT_UNLOCK (l, d);

#ifdef DICT_TIMING
DXMarkTimeLocal("-D search");
#endif

    return (o);
}


static EXDictElement 
_dxf_ExAllocateNewDictionaryElement(EXDictionary d)
{
    int size;

    if (d->sorted)
	size = sizeof(struct _EXDictElementSorted);
    else
	size = sizeof(struct _EXDictElement);
	
    return d->local ?
	    (EXDictElement)DXAllocateLocalZero(size) :
	    (EXDictElement)DXAllocateZero(size);
}

/*
 * Inserts a new element into a dictionary.
 */

Error _dxf_ExDictionaryInsert (EXDictionary d, char *key, EXObj obj)
{
    Pointer		h;
    int			b;
    int			l;
    Pointer		k;
    EXDictElement	e, ne;
    EXObj		o;

#ifdef DICT_TIMING
DXMarkTimeLocal("D insert");
#endif

    ExReference (obj);

    h = (Pointer) _dxf_ExCRCString (EX_INITIAL_CRC, key);
    b = (long) h & d->mask;
    l = d->locking;

    DICT_LOCK (l, d);

    e = d->h[b].e ? ExDictionarySearchE (d, b, key, h) : NULL;

    /*
     * If we find something with the given key then just replace the object.
     * If not then we need to construct a new dictionary element, fill out
     * its fields and link it into the structure.  Just to be sure, however,
     * we redo the search to make sure that no-one snuck something with the
     * same key in on us.
     */
    if (e)
    {
	o      = e->obj;			/* remember for delete	*/
	e->obj = obj;				/* replace object	*/
	DICT_UNLOCK (l, d);
	ExDelete (o);

	if (_dxf_ExIsDictionarySorted(d))
	    _dxf_ExMakeNewest(d, e);

    }
    else
    {
	/*
	 * Get an element block.  If we can find one from the free list the
	 * make use of it, otherwise go ahead and allocate one.  Once we
	 * have the block then go ahead and fill in its fields.
	 */

	if (d->free)
	{
	    ne = d->free;
	    d->free = ne->next;
	    DICT_UNLOCK (l, d);
	}
	else
	{
	    DICT_UNLOCK (l, d);

	    ne = _dxf_ExAllocateNewDictionaryElement(d);
	    if (! ne)
		_dxf_ExDie ("_dxf_ExDictionaryInsert:  can't DXAllocate");
	}

	ne->prev = NULL;
	ne->key  = d->local ? _dxf_ExCopyStringLocal (key) : _dxf_ExCopyString (key);
	ne->obj  = obj;
	ne->hash = h;

	/*
	 * Redo the search.
	 */

	if (l)
	{
	    DICT_LOCK (l, d);
	    e = d->h[b].e ? ExDictionarySearchE (d, b, key, h) : NULL;

	    /*
	     * If someone snuck one in on us then just replace the data and
	     * get rid of the one we created.
	     */

	    if (e)
	    {
		o        = e->obj;		/* remember for delete	*/
		e->obj   = obj;			/* replace object	*/
		k        = ne->key;		/* remember for delete	*/
		ne->next = d->free;		/* free element block	*/
		d->free  = ne;

		if (_dxf_ExIsDictionarySorted(d))
		    _dxf_ExMakeNewest(d, e);

		DICT_UNLOCK (l, d);

		ExDelete (o);			/* delete old object	*/
		DXFree ((Pointer) k);		/* delete new key	*/
	    }
	    else
	    {
		if (d->h[b].e)			/* fill in back pointer	*/
		    d->h[b].e->prev = ne;
		ne->next  = d->h[b].e;		/* link in new element	*/
		d->h[b].e = ne;
		d->items++;			/* bump item count	*/

		if (_dxf_ExIsDictionarySorted(d))
		    _dxf_ExMakeNewest(d, ne);

		DICT_UNLOCK (l, d);
	    }
	}
	else
	{
	    if (d->h[b].e)			/* fill in back pointer	*/
		d->h[b].e->prev = ne;
	    ne->next  = d->h[b].e;		/* link in new element	*/
	    d->h[b].e = ne;
	    d->items++;				/* bump item count	*/
	}
    }

#ifdef DICT_TIMING
DXMarkTimeLocal("-D insert");
#endif
    return (OK);
}


/*
 * Deletes an element from a dictionary
 */

Error _dxf_ExDictionaryDelete (EXDictionary d, Pointer key)
{
    Pointer		h;
    int			b;
    int			l;
    Pointer		k;
    EXDictElement	e;
    EXObj		o;

#ifdef DICT_TIMING
DXMarkTimeLocal("D delete");
#endif

    if (d->items == 0)
	return (ERROR);

    h = (Pointer) _dxf_ExCRCString (EX_INITIAL_CRC, key);
    b = (long) h & d->mask;
    l = d->locking;

    DICT_LOCK (l, d);
    e = ExDictionarySearchE (d, b, key, h);

    if (! e)					/* a quick out		*/
    {
	DICT_UNLOCK (l, d);
	return (ERROR);
    }

    o = e->obj;					/* remember for delete	*/
    k = e->key;					/* remember for delete	*/

    if (e->next)				/* unlink 		*/
	e->next->prev = e->prev;

    if (e->prev)
	e->prev->next = e->next;
    else
	d->h[b].e = e->next;

    e->next = d->free;				/* add to free list	*/
    d->free = e;
    d->items--;

    if (d->sorted)
	_dxf_ExRemoveFromSortList(d, e);

    DICT_UNLOCK (l, d);

    ExDelete (o);
    DXFree ((Pointer) k);

#ifdef DICT_TIMING
DXMarkTimeLocal("-D delete");
#endif

    return (OK);
}


/*
 * Deletes an element from a dictionary.  This ignores locking because
 * it is intended to be called while iterating through the dictionary
 * using the ExDictionary*Iterate functions.
 */

Error _dxf_ExDictionaryDeleteNoLock (EXDictionary d, Pointer key)
{
    Pointer		h;
    int			b;
    Pointer		k;
    EXDictElement	e;
    EXObj		o;

#ifdef DICT_TIMING
DXMarkTimeLocal("D del NL");
#endif

    if (d->items == 0)
	return (ERROR);

    h = (Pointer) _dxf_ExCRCString (EX_INITIAL_CRC, key);
    b = (long) h & d->mask;

    e = ExDictionarySearchE (d, b, key, h);

    if (! e)					/* a quick out		*/
    {
	return (ERROR);
    }

    o = e->obj;					/* remember for delete	*/
    k = e->key;					/* remember for delete	*/

    if (e->next)				/* unlink 		*/
	e->next->prev = e->prev;

    if (e->prev)
	e->prev->next = e->next;
    else
	d->h[b].e = e->next;

    if (d->iterItem == e)
	d->iterItem = e->next;

    e->next = d->free;				/* add to free list	*/
    d->free = e;
    d->items--;

    if (d->sorted)
	_dxf_ExRemoveFromSortList(d, e);

    ExDelete (o);
    DXFree ((Pointer) k);

#ifdef DICT_TIMING
DXMarkTimeLocal("-D del NL");
#endif

    return (OK);
}

/* A set of routines to iterate the dictionary */
Error _dxf_ExDictionaryBeginIterate (EXDictionary d)
{
    if (!d)
	return (OK);

    DICT_LOCK (d->locking, d);

    d->iterIndex = -1;
    d->iterItem = NULL;

    return (OK);
}

EXObj _dxf_ExDictionaryIterate (EXDictionary d, char **key)
{
    EXObj		result;
    EXDictElement	e = d->iterItem;
    int			i = d->iterIndex;

    while (e == NULL) 
    {
	++i;
	if (i == d->size)
	{
	    *key = NULL;
	    return NULL;
	}
	e = d->h[i].e;
    }

    result = e->obj;
    *key = e->key;

    d->iterItem = e->next;
    d->iterIndex = i;

    return (result);
}


int _dxf_ExDictionaryIterateMany (EXDictionary d, char **key, EXObj *obj, int n)
{
    EXDictElement	e = d->iterItem;
    int			i = d->iterIndex;
    int			cnt = 0;

    if (i >= d->size)
	return (0);

    for (cnt = 0; cnt < n; )
    {
	while (e == NULL) 
	{
	    ++i;
	    if (i == d->size)
	    {
		d->iterItem  = NULL;
		d->iterIndex = i;
		return (cnt);
	    }
	    e = d->h[i].e;
	}

	*key++ = e->key;
	*obj++ = e->obj;
	cnt++;
	e = e->next;
    }

    d->iterItem = e;
    d->iterIndex = i;
    return (cnt);
}

Error _dxf_ExDictionaryEndIterate (EXDictionary d)
{
    DICT_UNLOCK (d->locking, d);
    return (OK);
}

Error _dxf_ExDictPrint (EXDictionary d)
{
    int			l, i, colision;
    EXDictElement	e;


    if (!d)
	return (OK);
    l = d->locking;

    DICT_LOCK (l, d);

    for (i = 0, e = d->free; e; ++i, e = e->next)
	;

    DXMessage ("Dictionary 0x%08x, size %d, %slocking, freelist size %d",
                 d, d->size, l? "": "non-", i);
    DXMessage("%25s | %4s | %2s | %10s",  "name", "bin", "cl", "object");
    for (i = 0; i < d->size; ++i) 
    {
	for (colision = 0, e = d->h[i].e; e != NULL; e = e->next, ++colision)
	{
            DXMessage ("%25s | %4d | %2d | 0x%08x", e->key, i, colision, e->obj);
	}
    }

    DICT_UNLOCK (l, d);
    return (OK);
}


void 
_dxf_ExPrintSortList(EXDictionary d)
{
    if (! d->sorted) 
	DXMessage("dictionary is not sorted");
    else
    {
	EXDictElementSorted a = d->oldest, b = d->newest;


	DXMessage("o->n    n->o");
	while (a || b)
	{
	    DXMessage("0x%08lx 0x%08lx", a, b);
	    a = a->newer; b = b->older;
	}
    }
}

int
_dxf_ExIsDictionarySorted(EXDictionary d)
{
    return d->sorted;
}

static void
_dxf_ExRemoveFromSortList(EXDictionary d, EXDictElement de)
{
    if (d->sorted)
    {
	EXDictElementSorted sde = (EXDictElementSorted)de;

	if (sde->older)
	    sde->older->newer = sde->newer;
	if (sde->newer) 
	    sde->newer->older = sde->older;
	
	if (d->newest == sde)
	    d->newest = sde->older;
	
	if (d->oldest == sde)
	    d->oldest = sde->newer;
	
	sde->newer = sde->older = NULL;
    }
}

static void
_dxf_ExMakeNewest(EXDictionary d, EXDictElement de)
{
    if (d->sorted)
    {
	EXDictElementSorted sde = (EXDictElementSorted)de;

	if (d->newest == sde)
	    return;

	if (sde->newer || sde->older)
	    _dxf_ExRemoveFromSortList(d, (EXDictElement)sde);
	
	if (NULL != (sde->older = d->newest))
	    sde->older->newer = sde;

	sde->newer = NULL;

	d->newest  = sde;

	if (d->oldest == NULL)
	    d->oldest = sde;
    }
}


Error
_dxf_ExDictionaryBeginIterateSorted(EXDictionary d, int reverse)
{
    if (! d->sorted)
    {
	DXSetError(ERROR_INTERNAL,
		"attempting to use _dxf_ExDictionaryBeginIterateSorted"
		"on an unsorted dictionary");
	return ERROR;
    }

    if (reverse)
    {
	d->reverse  = 1;
	d->iterItem = (EXDictElement)d->newest;
    }
    else
    {
	d->reverse  = 0;
	d->iterItem = (EXDictElement)d->oldest;
    }

    return OK;
}

EXObj
_dxf_ExDictionaryIterateSorted(EXDictionary d, char **key)
{
    EXDictElementSorted	e = (EXDictElementSorted)d->iterItem;

    if (! d->sorted)
    {
	DXSetError(ERROR_INTERNAL,
		"attempting to use _dxf_ExDictionaryIterateSorted"
		"on an unsorted dictionary");
	return ERROR;
    }

    if (e != NULL) 
    {
	if (d->reverse)
	    d->iterItem = (EXDictElement)(e->older);
	else
	    d->iterItem = (EXDictElement)(e->newer);
	
	*key = e->de.key;
	return e->de.obj;
    }
    else
    {
	*key = 0;
	return NULL;
    }
}

int
_dxf_ExDictionaryIterateSortedMany (EXDictionary d, char **key, EXObj *obj, int n)
{
    int nn;

    for (nn = 0; nn < n; nn++)
	if (NULL == (*obj++ = _dxf_ExDictionaryIterateSorted(d, key++)))
	    break;

    return nn;
}

