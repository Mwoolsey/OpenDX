/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#include <dx/dx.h>
#include "exobject.h"
#include "config.h"
#include "log.h"
#include "graph.h"
#include "parse.h"

/* DXFree lists of GLOBAL objects only */
typedef struct {
    lock_type	lock;
    gvar 	*gvarHead;
    gvar 	*gvarTail;
} FreeList;
static FreeList *freeList;

static node *FuncFreeList	= NULL;

#define INIT_GVARS	512	/* Number of gvars to allocate initially */

PFIP _dxd_EXO_default_methods[] =
{
    _dxf__EXO_delete
};

static lock_type *allocLock;

#ifdef LEAK_DEBUG
    EXObj head;
#endif


/*
 * Initialize the executive's object facility.
 */

Error
_dxf_EXO_init (void)
{
#if 0
    int i;
    gvar *gvars;
#endif

    freeList = (FreeList *)DXAllocateZero (sizeof (FreeList));
    if (freeList == NULL)
	return (ERROR);
    if (DXcreate_lock (&freeList->lock, "exobj freeList") != OK)
	return (ERROR);
    allocLock = (lock_type *)DXAllocateZero (sizeof (lock_type));
    if (DXcreate_lock (allocLock, "exobj freeList allocation") != OK)
	return (ERROR);

#if 0
    gvars = freeList->gvarHead = (gvar *)DXAllocateZero (INIT_GVARS * sizeof (gvar));
    if (gvars == NULL)
	return (ERROR);
    for (i = 0; i < INIT_GVARS - 1; ++i)
    {
	if (DXcreate_lock (&gvars[i].object.lock, "exobj gvar") != OK)
	    return (ERROR);
	gvars[i].next = &gvars[i+1];
    }
    freeList->gvarTail = &gvars[INIT_GVARS-1];
    if (DXcreate_lock (&gvars[i].object.lock, "exobj gvar") != OK)
	return (ERROR);
#endif
    
#ifdef LEAK_DEBUG
    head = (EXObj) DXAllocateZero (sizeof (exo_object));
    head->DBGnext = head->DBGprev = head;
#endif

    return (OK);
}


/*
 * DXFree the executive's object locking structures.
 */

Error
_dxf_EXO_cleanup (void)
{
    return (OK);
}

int
_dxf_EXO_compact (void)
{
    int			result;
    EXO_Object		obj;

    DXlock (allocLock, 0);
    DXlock (&freeList->lock, exJID);
    result = freeList->gvarHead != NULL;
    while (freeList->gvarHead)
    {
	if (freeList->gvarHead->next == NULL)
	{
	    if (freeList->gvarHead->next == NULL)
		freeList->gvarTail = NULL;
	}
	obj = (EXO_Object) freeList->gvarHead;
	freeList->gvarHead = ((gvar *) obj)->next;
	DXFree((Pointer) obj);
    }
    DXunlock (allocLock, 0);
    DXunlock (&freeList->lock, exJID);

    if (!result && exJID == 1)
    {
	result = FuncFreeList != NULL;
	while (FuncFreeList != NULL)
	{
	    obj = (EXObj) FuncFreeList;
	    FuncFreeList = FuncFreeList->next;
	    DXFree((Pointer) obj);
	}
    }
    return (result);
}

/*
 * Check a piece of data to make sure that it has the proper tag to be
 * an executive object.
 */

Error _dxf_EXOCheck (EXO_Object obj)
{
    if (obj && !((long)obj & 0x03) && obj->tag == EXO_TAG)
    {
	switch (obj->exclass)
	{
	    case EXO_CLASS_DELETED:
	    case EXO_CLASS_FUNCTION:
	    case EXO_CLASS_GVAR:
	    case EXO_CLASS_TASK:
		return (OK);

	    default:
		break;
	}
    }
    
    DXWarning ("#4610", obj);

    DXqflush ();
    abort();			/* Die here please */

    return (ERROR);
}

/*
 * Create a new object and use default method vector if new methods
 * not specified
 */

static EXO_Object
EXO_create_object_worker (EXO_Class exclass, int size, PFIP *methods, int local)
{
    EXO_Object		obj;
    int			locked;

    switch (exclass)
    {
	case EXO_CLASS_FUNCTION:
	    if (FuncFreeList == NULL)
		obj = (EXObj) DXAllocateLocal (size);
	    else
	    {
		obj = (EXObj) FuncFreeList;
		FuncFreeList = FuncFreeList->next;
	    }
	    break;

	case EXO_CLASS_GVAR:
	    DXlock (allocLock, 0);
	    if (freeList->gvarHead)
	    {
		locked = freeList->gvarHead->next == NULL;
		if (locked)
		{
		    DXlock (&freeList->lock, exJID);
		    if (freeList->gvarHead->next == NULL)
			freeList->gvarTail = NULL;
		}
		obj = (EXO_Object) freeList->gvarHead;
		freeList->gvarHead = ((gvar *) obj)->next;
		if (locked)
		    DXunlock (&freeList->lock, exJID);
		DXunlock (allocLock, 0);
	    }
	    else 
	    {
		DXunlock (allocLock, 0);
		obj = (EXO_Object) DXAllocate (size);
	    }
	    break;

	default:
	    obj = (EXO_Object) (local ? DXAllocateLocal (size)
				      : DXAllocate (size));
	    break;
    }


    if (obj == NULL)
	_dxf_ExDie ("_dxf_EXO_create_object:  can't DXAllocate");

    if (! local && DXcreate_lock (&obj->lock, "exec object") != OK)
	_dxf_ExDie ("_dxf_EXO_create_object: can't create lock\n");

    ExZero (obj + 1, size - sizeof (exo_object));

    obj->tag	   = EXO_TAG;
    obj->exclass   = exclass;
    obj->local	   = local;
    obj->refs	   = 0;
    obj->m.copy    = FALSE;
    obj->m.methods = methods ? methods : _dxd_EXO_default_methods;
    obj->lastref  = 0;

#ifdef LEAK_DEBUG
    if (!local) {
	obj->DBGnext = head->DBGnext;
	obj->DBGprev = head;
	head->DBGnext->DBGprev = obj;
	head->DBGnext = obj;
    }
#endif

    return (obj);
}


EXO_Object _dxf_EXO_create_object (EXO_Class exclass, int size, PFIP *methods)
{
    return (EXO_create_object_worker (exclass, size, methods, FALSE));
}


EXO_Object _dxf_EXO_create_object_local (EXO_Class exclass, int size, PFIP *methods)
{
    return (EXO_create_object_worker (exclass, size, methods, TRUE));
}


/*
 * DXDelete an object.  Note that the delete macro has already decremented the
 * reference count and decided the object should go away
 */
int _dxf_EXO_delete (EXO_Object obj)
{
    int		del	= FALSE;
    if (obj->refs == 0)
    {
#ifdef LEAK_DEBUG
	if (!obj->local) {
	    obj->DBGnext->DBGprev = obj->DBGprev;
	    obj->DBGprev->DBGnext = obj->DBGnext;
	}
#endif
	switch (obj->exclass)
	{
	    case EXO_CLASS_FUNCTION:
		((node *) obj)->next = FuncFreeList;
		FuncFreeList = (node *) obj;
		break;

	    case EXO_CLASS_GVAR:
		((gvar *) obj)->next = NULL;
		DXlock (&freeList->lock, exJID);
		if (freeList->gvarTail)
		    freeList->gvarTail->next = (gvar *) obj;
		else
		    freeList->gvarHead = (gvar *) obj;
		freeList->gvarTail = (gvar *) obj;
		DXunlock (&freeList->lock, exJID);
		break;

	    default:
		del = TRUE;
		break;
	}

	obj->exclass = EXO_CLASS_DELETED;
	if (obj->m.copy)
	    DXFree ((Pointer) obj->m.methods);
	if (! obj->local)
	    DXdestroy_lock (&obj->lock);
	if (del)
	    DXFree ((Pointer) obj);
    }
    else
    {
	DXWarning ("#4620", obj, obj->refs);
	DXqflush ();
	if (_dxd_exDebug)
	    abort();		/* Die here please */
    }

    return (0);
}

/*
 * Default delete routine does nothing
 */
int _dxf__EXO_delete (EXO_Object obj)
{
    return (OK);
}


#ifdef LEAK_DEBUG
void
PrintEXObj()
{
    EXObj o;

    for (o = head->DBGnext; o != head; o = o->DBGnext) {
	switch (o->exclass) {
	case EXO_CLASS_DELETED:
	    printf ("0x%08x (refs %d) EXO_CLASS_DELETED\n", o, o->refs);
	    break;
	case EXO_CLASS_FUNCTION:
	    printf ("0x%08x (refs %d) EXO_CLASS_FUNCTION\n", o, o->refs);
	    break;
	case EXO_CLASS_GVAR:
	    printf ("0x%08x (refs %d) EXO_CLASS_GVAR, type is %d\n",
		o, o->refs, ((gvar*)o)->type);
	    break;
	case EXO_CLASS_TASK:
	    printf ("0x%08x (refs %d) EXO_CLASS_TASK\n", o, o->refs);
	    break;
	case EXO_CLASS_UNKNOWN:
	    printf ("0x%08x (refs %d) EXO_CLASS_UNKNOWN\n", o, o->refs);
	    break;
	default:
	    printf ("0x%08x (refs %d) of unknown class %d\n", o, o->refs, o->exclass);
	    break;
	}
    }
}
#endif
