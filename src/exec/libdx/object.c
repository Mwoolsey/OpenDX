/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>



#include <string.h>
#include "objectClass.h"
#include "internals.h"

/*
 * Tracing
 */

#if DEBUGGED
static int trace = 1;
#endif

void
_dxfTraceObjects(int d)
{
#if DEBUGGED
    trace = d;
#endif
}

#if DEBUGGED
static struct table {			/* table of New/DXDelete per class */
    lock_type lock;			/* lock for the table */
    struct counts {			/* the counts */
	struct object_class *class;	/* class vector */
	int new;			/* how many created */
	int deleted;			/* how many deleted */
    } counts[CLASS_MAX];		/* one per class, in global storage */
} *table;

#define EVERY 500			/* how often to print out */
#endif

#define BITS 24				/* this many bits in tag */
static int tag = 1;			/* per-processor */

Error
_dxf_initobjects(void)
{
#if DEBUGGED
    table = (struct table *) DXAllocateZero(sizeof(struct table));
    if (!table)
	return NULL;
    DXcreate_lock(&table->lock, "object statistics table");
#endif

    return OK;
}



/*
 *
 */

#define PERMANENT 999999999

Object _dxf_SetPermanent(Object o)
{
    o->count = PERMANENT;
    return o;
}


#if DEBUGGED
#define ID DXProcessorId()
#else
#define ID 0
#endif

Object DXReference(Object o)
{
    if (!o)
	return NULL;
    if (o->count==PERMANENT)
	return o;

    /* lock & increment count */
    DXfetch_and_add(&o->count, 1, &o->lock, ID);

    return o;
}


Error
DXDelete(Object o)
{
    int rc = 0, i, n;
    Class class;

    if (!o)
	return OK;
    if (o->count==PERMANENT)
	return OK;

    /* sanity checks - up thru dx 2.1.1, this was if DEBUGGED only */
    if (o->count < 0)
	DXErrorReturn(ERROR_DATA_INVALID,
		    "Object deleted too often! (or not an object)");
    class = DXGetObjectClass(o);
    if ((int)class<=(int)CLASS_MIN || (int)class>=(int)CLASS_MAX) {
	DXSetError(ERROR_DATA_INVALID,
		 "Deleting object of unknown class %d! (or not an object)",
		 class);
	return ERROR;
    }

    /* lock & decrement count */
    if (DXfetch_and_add(&o->count, -1, &o->lock, ID) > 1)
	return OK;

#if DEBUGGED
    /* tracing */
    if (trace>=2)
	DXDebug("Q","deleting object class %s at 0x%x", CLASS_NAME(o->class), o);
    if (trace>=1) {
	int n = (int) CLASS_CLASS(o->class);
	DXlock(&table->lock, 0);
	table->counts[n].deleted += 1;
	DXunlock(&table->lock, 0);
    }
#endif

    /*
     * delete attributes
     * XXX - this should by in an _DeleteObject to be called by subclass
     * i.e. _Delete should be handled like New and DXCopy
     */
    for (i=0, n=o->nattributes; i<n; i++)
	if (o->count != PERMANENT)
	    DXDelete(o->attributes[i].value);
    if (o->attributes!=o->local)
	DXFree((Pointer)o->attributes);

    /* user deletion */
    rc = _dxfDelete(o);

    /* in case we mistakenly delete this object again - same as above;
     *  was only if DEBUGGED up through dx 2.1.1
     */
    o->class_id = CLASS_DELETED;
    o->count = -1;
    o->class = NULL;

    /* finish deleting our stuff */
    DXdestroy_lock(&o->lock);
    DXFree((Pointer)o);
    return rc;
}


Error
DXUnreference(Object o)
{
    Class class;

    if (!o)
	return ERROR;
    if (o->count==PERMANENT || o->count==0)
	return OK;

    /* sanity checks  - same comment as in DXDelete */
    if (o->count < 0)
	DXErrorReturn(ERROR_DATA_INVALID,
		    "Object deleted too often! (or not an object)");
    class = DXGetObjectClass(o);
    if ((int)class<=(int)CLASS_MIN || (int)class>=(int)CLASS_MAX) {
	DXSetError(ERROR_DATA_INVALID,
		 "Deleting object of unknown class %d! (or not an object)",
		 class);
	return ERROR;
    }

    /* lock & decrement count */
    DXfetch_and_add(&o->count, -1, &o->lock, ID);

    return OK;
}





Object
_dxf_NewObject(struct object_class *class)
{
    Object o;

    o = (Object) DXAllocate(CLASS_SIZE(class));
    if (!o)
	return NULL;

#if DEBUGGED
    /* tracing */
    if (trace>=2)
	DXDebug("Q", "creating object class %s at 0x%x", CLASS_NAME(class), o);
    if (trace>=1) {
	int n = (int) CLASS_CLASS(class);
	table->counts[n].class = class;
	DXlock(&table->lock, 0);
	table->counts[n].new += 1;
	if (table->counts[n].new%EVERY==0)
	    DXDebug("O", "%d %s objects created, %d deleted, net %d",
		  table->counts[n].new, CLASS_NAME(class),
		  table->counts[n].deleted,
		  table->counts[n].new-table->counts[n].deleted);
	DXunlock(&table->lock, 0);
    }
#endif

    memset(o, 0, CLASS_SIZE(class));
    o->class = class;
    o->class_id = CLASS_CLASS(class);
    DXcreate_lock(&o->lock, "object");
    o->count = 0;
    o->tag = (DXProcessorId()<<BITS) + tag++;
    o->attributes = o->local;
    o->nattributes = 0;
    o->attr_alloc = NATTRIBUTES;
    return o;
}


/*
 * Tags
 */

int
DXGetObjectTag(Object o)
{
    if (!o)
	return 0;
    return o->tag;
}

Object
DXSetObjectTag(Object o, int tag)
{
    if (tag>=0) {
	DXSetError(ERROR_INTERNAL,
		 "tag value %d is illegal: must be less than 0", tag);
	return NULL;
    }
    if (!o)
	return NULL;
    o->tag = tag;
    return o;
}



/*
 * Attributes
 */

Object
DXSetAttribute(Object o, char *name, Object value)
{
    int i, m, n = o->nattributes;
    struct attribute *a;

    if (!o)
	return NULL;

    if (!name)
	DXErrorReturn(ERROR_BAD_PARAMETER, "DXSetAttribute given null name");

    /* don't do quick check: assume usually not there */
    for (i=0, a=o->attributes; i<n; i++, a++)
	if (a->name && strcmp(a->name, name)==0)
	    break;
    
    /* attribute is not there - add it if we are setting a value,
     * or return ok if requesting to delete it.
     */
    if (i >= n) {
	if (!value)
	    return o;
	if (n >= o->attr_alloc) {
	    if (o->attr_alloc==NATTRIBUTES) {
		m = 2*NATTRIBUTES;
		a = (struct attribute *) DXAllocate(m*sizeof(struct attribute));
		if (a)
		    memcpy(a, o->local, sizeof(o->local));
	    } else {
		m = o->attr_alloc*2 + 1;
		a = (struct attribute *) DXReAllocate((Pointer)o->attributes,
					      m * sizeof(struct attribute));
	    }
	    if (!a)
		return NULL;
	    o->attributes = a;
	    o->attr_alloc = m;
	}
	a = o->attributes + n;
	o->nattributes = n+1;
	a->name = _dxfstring(name, 1);
	a->value = NULL;
	
    } else if (!value) {
	
	/* copy remaining attributes down */
	DXDelete(a->value);
	for (i=i+1; i<n; i++)
	    o->attributes[i-1] = o->attributes[i];
	o->nattributes -= 1;
	return o;
	
    }

    /* put value in */
    if (value!=a->value) {		/* MP performance */
	DXReference(value);		/* do first in case value==a->value */
	DXDelete(a->value);
	a->value = value;
    }

    return o;
}


Object
DXDeleteAttribute(Object o, char *name)
{
    int i, n = o->nattributes;
    struct attribute *a;

    if (!o)
	return NULL;

    if (!name)
	DXErrorReturn(ERROR_BAD_PARAMETER, 
		      "DXDeleteAttribute given null name");

    /* quick check for pointer equality */
    for (i=0, a=o->attributes; i<n; i++, a++)
	if (a->name==name)
	    break;

    /* now check for string equality */
    if (i >= n) 
	for (i=0, a=o->attributes; i<n; i++, a++)
	    if (a->name && strcmp(a->name, name)==0)
		break;
    
    /* attribute not there, return ok */
    if (i >= n)
	return o;
    
    /* copy remaining attributes down */
    DXDelete(a->value);
    for (i=i+1; i<n; i++)
	o->attributes[i-1] = o->attributes[i];
    o->nattributes -= 1;
    return o;
}


Object
DXGetAttribute(Object o, char *name)
{
    int i, n;
    struct attribute *a;

    if (!o)
	return NULL;
    if (!name)
	DXErrorReturn(ERROR_BAD_PARAMETER, "DXGetAttribute given null name");

    /* quick check for pointer equality */
    n = o->nattributes;
    for (i=0, a=o->attributes; i<n; i++, a++)
	if (a->name==name)
	    break;

    /* now check for string equality */
    if (i >= n) 
	for (i=0, a=o->attributes; i<n; i++, a++)
	    if (a->name && strcmp(a->name, name)==0)
		break;
    
    /* no */
    if (i >= n)
	return NULL;

    /* yes */
    return a->value;
}


Object
DXGetEnumeratedAttribute(Object o, int n, char **name)
{
    struct attribute *a;
    if (!o)
	return NULL;
    if (n >= o->nattributes)
	return NULL;
    a = o->attributes + n;
    if (name)
	*name = a->name;
    return a->value;
}

Object
DXSetFloatAttribute(Object o, char *name, double x)
{
    Array a;
    float *p;
    a = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 0);
    p = (float *)DXGetArrayData(DXAddArrayData(a, 0, 1, NULL));
    if (!p)
	return NULL;
    *p = x;
    if (!DXSetAttribute(o, name, (Object)a)) {
	DXDelete((Object)a);
	return NULL;
    }
    return o;
}

Object
DXSetIntegerAttribute(Object o, char *name, int x)
{
    Array a;
    int *p;
    a = DXNewArray(TYPE_INT, CATEGORY_REAL, 0);
    p = (int *)DXGetArrayData(DXAddArrayData(a, 1, 0, NULL));
    if (!p)
	return NULL;
    *p = x;
    if (!DXSetAttribute(o, name, (Object)a)) {
	DXDelete((Object)a);
	return NULL;
    }
    return o;
}

Object
DXSetStringAttribute(Object o, char *name, char *x)
{
    String s;
    s = DXNewString(x);
    if (!s)
	return NULL;
    if (!DXSetAttribute(o, name, (Object)s)) {
	DXDelete((Object)s);
	return NULL;
    }
    return o;
}

Object
DXGetFloatAttribute(Object o, char *name, float *x)
{
    Array a;
    float *p;

    if (!(a = (Array)DXGetAttribute(o, name)))
	return NULL;

    if (DXGetObjectClass((Object)a) != CLASS_ARRAY)
	return NULL;
    
    if (!DXTypeCheck(a, TYPE_FLOAT, CATEGORY_REAL, 0))
	return NULL;

    if (!(p = (float *)DXGetArrayData(a)))
	return NULL;

    if (x)
	*x = *p;

    return o;
}

Object
DXGetIntegerAttribute(Object o, char *name, int *x)
{
    Array a;
    int *p;

    if (!(a = (Array)DXGetAttribute(o, name)))
	return NULL;

    if (DXGetObjectClass((Object)a) != CLASS_ARRAY)
	return NULL;
    
    if (!DXTypeCheck(a, TYPE_INT, CATEGORY_REAL, 0))
	return NULL;

    if (!(p = (int *)DXGetArrayData(a)))
	return NULL;

    if (x)
	*x = *p;

    return o;
}


Object
DXGetStringAttribute(Object o, char *name, char **x)
{
    String s;

    if (!(s = (String)DXGetAttribute(o, name)))
	return NULL;

    if (DXGetObjectClass((Object)s) != CLASS_STRING)
	return NULL;
    
    if (x)
	*x = DXGetString(s);

    return o;
}


Object
DXCopyAttributes(Object dst, Object src)
{
    struct attribute *a;
    int i;
    if (!dst)
	return NULL;
    if (!src)
	return dst;
    for (i=0, a=src->attributes; i<src->nattributes; i++, a++)
	if (!DXSetAttribute(dst, a->name, a->value))
	    return NULL;
    return dst;
}


/*
 * Copying
 * XXX - why is copying attributes done both here and in DXCopyAttributes?
 */

Object
_dxf_CopyObject(Object new, Object old, enum _dxd_copy copy)
{
    int i, n = old->nattributes;
    struct attribute *oa, *na;

    /*
     * DXCopy the attributes, no matter what the type of copy
     */

    /* allocate attributes */
    if (n<=NATTRIBUTES) {
	na = new->local;
	new->attr_alloc = NATTRIBUTES;
    } else {
	na = (struct attribute *) DXAllocate(n * sizeof(struct attribute));
	if (!na)
	    return NULL;
	new->attr_alloc = n;
    }
    new->attributes = na;
    new->nattributes = n;

    /* fill in attributes */
    for (i=0, oa=old->attributes; i<n; i++, oa++, na++) {
	na->name = oa->name;
	na->value = DXReference(oa->value);
    }
    return new;
}

Object
DXGetType(Object a, Type* b, Category* c, int* d, int* e)
{
    return _dxfGetType((Object)a,b,c,d,e);
}

Object
DXCopy(Object a, enum _dxd_copy b)
{
    return _dxfCopy((Object)a,b);
}

Error
_dxfObject_Delete(Object o)
{
    return OK;
}

Object
_dxfObject_BoundingBox(Object o, Point *p, Matrix *m, int valid)
{
    return NULL;
}

Error
_dxfObject_Shade(Object o, struct shade *s)
{
    DXSetError(ERROR_BAD_CLASS, "#13880", CLASS_NAME(o->class));
    return ERROR;
}
