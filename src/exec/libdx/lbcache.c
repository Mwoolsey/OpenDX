/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>



/*
 * very simple cache w/ linear search,
 * lru reclaiming, ignoring cost and target size
 */

#include <string.h>
#include <dx/dx.h>
#include "internals.h"

static struct cache {
    lock_type DXlock;
    int inuse;
    int alloc;
    struct entry {
	Object out;
	float cost;
	char *fun;
	int key;
	int n;
	int *tags;
	struct entry *next, *last;
    } dummy;
} *cache;

/* prototypes */
static int cachescavenger(ulong n);
static struct entry *find(char *fun, int key, int n, Object *in);
static void remove_(struct entry *this); /* '_' resolves os2 name conflict */
static void insert(struct entry *this);
static void delete(struct entry *this);
Error DXSetCacheEntryV(Object out, double cost,
	       char *fun, int key, int n, Object *in);
Error DXSetCacheEntry(Object out, double cost,
		    char *fun, int key, int n, ...);
Object DXGetCacheEntryV(char *fun, int key, int n, Object *in);
Object DXGetCacheEntry(char *fun, int key, int n, ...);


/* cache reclaim routine
 */
static int
cachescavenger(ulong n)
{
    struct entry *this;

    DXMessage("looking for %d bytes from standalone cache", n);

    DXlock(&cache->DXlock, 0);
    for (this=cache->dummy.last; this!=&cache->dummy; this=this->last) {
	if (this->cost < CACHE_PERMANENT) {
	    delete(this);
	    DXunlock(&cache->DXlock, 0);	    
	    return OK;
	}
    }
    DXunlock(&cache->DXlock, 0);	    
    return ERROR;
}

Error
_dxf_initcache(void)
{
    if (cache)
	return OK;

    cache = (struct cache *) DXAllocateZero(sizeof(struct cache));
    if (!cache)
	return ERROR;
    if (!DXcreate_lock(&cache->DXlock, "cache"))
	return ERROR;
    DXRegisterScavenger(cachescavenger);
    cache->dummy.next = &cache->dummy;
    cache->dummy.last = &cache->dummy;

    return OK;
}


static struct entry *
find(char *fun, int key, int n, Object *in)
{
    struct entry *this;
    int i;

    for (this=cache->dummy.next; this!=&cache->dummy; this=this->next) {
	if (this->n==n && strcmp(this->fun,fun)==0 && this->key==key) {
	    for (i=0; i<n; i++)
		if (this->tags[i]!=DXGetObjectTag(in[i]))
		    break;
	    if (i==n)
		break;
	}    
    }
    return this==&cache->dummy? NULL : this;
}


static void
remove_(struct entry *this)
{
    this->last->next = this->next;
    this->next->last = this->last;
}


static void
insert(struct entry *this)
{
    this->next = cache->dummy.next;
    this->last = &cache->dummy;
    cache->dummy.next->last = this;
    cache->dummy.next = this;
}


static void
delete(struct entry *this)
{
    remove_(this);
    DXDelete(this->out);
    DXFree((Pointer)this);
}


Error
DXSetCacheEntryV(Object out, double cost,
	       char *fun, int key, int n, Object *in)
{
    int i;
    struct entry *this;

    if (!cache && !_dxf_initcache())
	return ERROR;

    DXlock(&cache->DXlock, 0);

    this = find(fun, key, n, in);
    if (!this && !out) {
	DXunlock(&cache->DXlock, 0);
	return OK;
    }

    if (!this) {
	int size = sizeof(struct entry) + n*sizeof(int) + strlen(fun) + 1;
	this = (struct entry *) DXAllocateZero(size);
	if (!this)
	    goto error;
	this->tags = (int *) (((char *)this) + sizeof(struct entry));
	this->fun = (char *) (this->tags + n);
	strcpy(this->fun, fun);
	this->out = DXReference(out);
    } else if (!out) {
	delete(this);
	DXunlock(&cache->DXlock, 0);	    
	return OK;
    } else {
	DXReference(out);
	DXDelete(this->out);
	this->out = out;
	remove_(this);
    }

    this->n = n;
    for (i=0; i<n; i++)
	this->tags[i] = DXGetObjectTag(in[i]);
    this->cost = cost;
    this->key = key;
    insert(this);

    DXunlock(&cache->DXlock, 0);
    return OK;

error:
    DXunlock(&cache->DXlock, 0);
    return ERROR;
}

Error DXSetCacheEntry(Object out, double cost,
		    char *fun, int key, int n, ...)
{
    Object in[100];
    int i;
    va_list arg;

    ASSERT(n<100);

    va_start(arg,n);
    for (i=0; i<n; i++)
	in[i] = va_arg(arg, Object);
    va_end(arg);
    return DXSetCacheEntryV(out, cost, fun, key, n, in);
}



Object
DXGetCacheEntryV(char *fun, int key, int n, Object *in)
{
    Object o;
    struct entry *this;

    if (!cache && !_dxf_initcache())
	return ERROR;

    DXlock(&cache->DXlock, 0);
    this = find(fun, key, n, in);
    if (this) {
	remove_(this);
	insert(this);
	o = DXReference(this->out);
    } else
	o = NULL;
    DXunlock(&cache->DXlock, 0);
    return o;
}


Object
DXGetCacheEntry(char *fun, int key, int n, ...)
{
    int i;
    Object in[100];
    va_list arg;

    ASSERT(n<100);

    va_start(arg,n);
    for (i=0; i<n; i++)
	in[i] = va_arg(arg, Object);
    va_end(arg);
    return DXGetCacheEntryV(fun, key, n, in);
}

