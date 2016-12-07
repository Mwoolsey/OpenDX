/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>



#include <string.h>
#include <stdarg.h>
#include "groupClass.h"


static Group _CopyGroup(Group new, Group old, enum _dxd_copy copy);

#define RANKTEST(rval) \
    if (rank>=100) { \
	DXSetError(ERROR_DATA_INVALID, "cannot support rank >= 100"); \
	return rval; \
    }
		     
/*
 * XXX - create and move to util.c
 */

static
char *
CopyString(char *s)
{
    if (s) {
	int n = strlen(s) + 1;
	char *c = DXAllocate(n);
	if (c)
	    strcpy(c, s);	
	return c;
    } else
	return NULL;
}


Group
_dxf_NewGroup(struct group_class *class)
{
    struct group *g = (Group) _dxf_NewObject((struct object_class *)class);
    if (!g)
	return NULL;
    if (!DXcreate_lock(&g->lock, "group"))
	return NULL;
    /* everything else is initialized to 0 */
    return g;
}


Group
DXNewGroup(void)
{
    return _dxf_NewGroup(&_dxdgroup_class);
}


/*
 * DXCopy.  DXWarning: this code is essentially the same as the
 * code for Series_Copy, until the loop at the end that does the copy.
 * If you change anything here, check that code also.
 */


Object
_dxfGroup_Copy(Group old, enum _dxd_copy copy)
{
    Group new;

    new = DXNewGroup();
    if (!new)
	return NULL;

    /*
     * XXX - copy the class here, so this routine suffices
     * for all subclasses.  Is this really OK?
     */
    new->object.class = old->object.class;
    new->object.class_id = old->object.class_id;
    
    /* XXX - check return code and delete new */
    if (! _CopyGroup(new, old, copy))
    {
	DXDelete((Object)new);
	return NULL;
    }

    return (Object)new;
}


static Group
_CopyGroup(Group new, Group old, enum _dxd_copy copy)
{
    int i;
    char *name;
    Object val;

    /* copy superclass */
    if (!_dxf_CopyObject((Object)new, (Object)old, copy))
	return NULL;

    /* done? */
    if (copy==COPY_ATTRIBUTES)
	return new;

    /*
     * XXX - should COPY_ATTRIBUTES copy the type also?  Since 
     * there is no way to reset it, it is inconvenient if it does so.
     * It seems unlikely that anyone would want to create an empty
     * group and copy the type.
     */
    new->typed = old->typed;
    new->type = old->type;
    new->category = old->category;
    new->rank = old->rank;
    if (old->shape) {
	new->shape = (int *) DXAllocate(old->rank * sizeof(*(old->shape)));
	if (! new->shape)
	    return NULL;
	for (i=0; i<old->rank; i++)
	    new->shape[i] = old->shape[i];
    } else
	new->shape = NULL;

    /* copy the members */
    for (i=0; (val = DXGetEnumeratedMember(old, i, &name)); i++) {
	if (copy!=COPY_HEADER) {
	    val = DXCopy(val, copy);
	    if (!val)
		return NULL;
	}
	if (! DXSetMember(new, name, val))
	    return NULL;
    }
    return new;
}


static
Error
type_check(Group g, Object value)
{
    Type type;
    Category category;
    int i, rank, shape[100];

    if (!DXGetType(value, &type, &category, &rank, shape)) {
	if (DXGetError() != ERROR_NONE)
	    return ERROR;
	return OK;
    }
    RANKTEST(ERROR);
    if (type!=g->type || category!=g->category || rank!=g->rank) {
	DXSetError(ERROR_BAD_PARAMETER, "#11090");
	return ERROR;
    }
    for (i=0; i<rank; i++)
	if (shape[i]!=g->shape[i]) {
	    DXSetError(ERROR_BAD_PARAMETER, "#11090");
	    return ERROR;
	}
    return OK;
}

Group
DXSetMember(Group g, char *name, Object value)
{
    return _dxfSetMember(g, name, value);
}


Group
_dxfGroup_SetMember(Group g, char *name, Object value)
{
    int i, nmembers;
    struct member *m = NULL;

    CHECK(g, CLASS_GROUP);

    /* get the lock */
    DXlock(&g->lock, 0);

    /* type check if appropriate */
    if (g->typed && value && !type_check(g, value))
	goto error;

    /* is member there? */
    nmembers = g->nmembers;
    if (name) {
	for (i=0, m=g->members; i<nmembers; i++, m++)
	    if (m->name && strcmp(m->name, name)==0)
		break;
    } else {
	/* null name => new member */
	i = nmembers;
    }

    /* member is not there - add it */
    if (i >= nmembers) {
	/* re-alloc if necessary */
	if (nmembers >= g->alloc) {
	    m = (struct member *) DXReAllocate((Pointer)g->members,
		(g->alloc=g->alloc*2+1) * sizeof(struct member));
	    if (!m)
		goto error;
	    g->members = m;
	}
	m = g->members + nmembers;
	g->nmembers = ++nmembers;
	m->name = CopyString(name);
	m->value = NULL;
    }

    /* put value in */
    DXReference(value);		/* do first in case value==c->comp_value */
    DXDelete(m->value);
    m->value = value;
    m->position = i;

    /* copy other members down if deleting */
    if (!value) {
	for (i=i+1; i<nmembers; i++)
	    g->members[i-1] = g->members[i];
	g->nmembers = --nmembers;
    }

    DXunlock(&g->lock, 0);
    return g;

error:
    DXunlock(&g->lock, 0);
    return NULL;
}


Object
DXGetMember(Group g, char *name)
{
    int i;
    struct member *m;

    CHECK(g, CLASS_GROUP);

    if (!name)
	DXErrorReturn(ERROR_BAD_PARAMETER, "DXGetMember given null name");

    /* is member there? */
    for (i=0, m=g->members; i<g->nmembers; i++, m++)
	if (m->name && strcmp(m->name, name)==0)
	    break;
    
    /* no */
    if (i >= g->nmembers)
	return NULL;

    /* yes */
    return m->value;
}

Group
DXGetMemberCount(Group g, int *n)
{
    CHECK(g, CLASS_GROUP);
    if (n)
        *n = g->nmembers;
    return g;
}


Object
_dxf_GetEnumeratedMember(Group g, int n, float *position, char **name)
{
    CHECK(g, CLASS_GROUP);

    /* is member there? */
    if (n < 0 || n >= g->nmembers)
	return NULL;

    /* yes */
    if (position)
	*position = g->members[n].position;
    if (name)
	*name = g->members[n].name;
    return g->members[n].value;
}


Object
DXGetEnumeratedMember(Group g, int n, char **name)
{
    return _dxf_GetEnumeratedMember(g, n, NULL, name);
}


Group
_dxf_SetEnumeratedMember(Group g, int i, double position, Object value)
{
    int nmembers;

    CHECK(g, CLASS_GROUP);

    /* get the lock */
    DXlock(&g->lock, 0);

    /* type check if appropriate */
    if (g->typed && value && !type_check(g, value))
	goto error;

    /* is member there? */
    nmembers = g->nmembers;
    if (i > nmembers)
	DXErrorGoto(ERROR_BAD_PARAMETER, "Non-contiguous enumerated member");

    /* if one past last member, add it */
    if (i == nmembers) {
	if (nmembers >= g->alloc) {
	    struct member *m = (struct member *)DXReAllocate((Pointer)g->members,
		(g->alloc=g->alloc*2+1) * sizeof(struct member));
	    if (!m)
		goto error;
	    g->members = m;
	}
	g->nmembers = ++nmembers;
	g->members[i].name = NULL;
	g->members[i].value = NULL;
    }

    /* put value in */
    DXReference(value);
    DXDelete(g->members[i].value);
    g->members[i].value = value;
    g->members[i].position = position;

    /* copy other members down if deleting */
    if (!value) {
	for (i=i+1; i<nmembers; i++)
	    g->members[i-1] = g->members[i];
	g->nmembers = --nmembers;
    }

    DXunlock(&g->lock, 0);
    return g;

error:
    DXunlock(&g->lock, 0);
    return NULL;
}

Group
DXSetEnumeratedMember(Group g, int n, Object value)
{
    return _dxfSetEnumeratedMember(g, n, value);
}

Group
_dxfGroup_SetEnumeratedMember(Group g, int n, Object value)
{
    return _dxf_SetEnumeratedMember(g, n, n, value);
}


/*
 * We lock the group in _SetType because it can be called from the
 * various DXSetMember/DXSetEnumeratedMember routines, which we wish to
 * support in parallel.  Note that we check again after locking to see
 * if somebody else has beat us to setting the type.
 */

Error
_dxf_SetType(Group g, Object o)
{
    Type type;
    Category category;
    int rank, shape[100];

    if (!DXGetType(o, &type, &category, &rank, shape)) {
	DXResetError();
	return OK;
    }
    RANKTEST(ERROR);

    DXlock(&g->lock, 0);
    if (!g->typed)
	if (!DXSetGroupTypeV(g, type, category, rank, shape)) {
	    DXunlock(&g->lock, 0);
	    return ERROR;
	}
    DXunlock(&g->lock, 0);
    return OK;
}


/*
 * SetPartClass() is implemented by calling _SetPartClass(), which is
 * the same except the part number n is passed by reference.  If
 * o has that many parts, the part is set and *n is set to -1.
 * Otherwise, *n is decremented by the number of parts that o has.
 * _SetPartClass(o,n,part,class) returns o if o is a group, or returns
 * part if o is a has class class.
 */

static Object
_SetPartClass(Object o, int *n, Object part, Class class)
{
    int i;
    Object new, m;

    if (DXGetObjectClass(o)==CLASS_GROUP) {

	Group g = (Group) o;

	for (i=0, new=NULL; *n>=0 && (m=DXGetEnumeratedMember(g, i, NULL)); i++)
	    new = _SetPartClass(m, n, part, class);

	/*
	 * _SetPartClass(m,n,part,class) will have returned part if member m
	 * has class class, or m if m is a group, so that the following
	 * statement does the right thing
	 */
	if (*n<0 && new==part)
	    DXSetEnumeratedMember(g, i-1, new);

	/* sic - this is how we tell if o was a group */
	return (Object)g;

    } else if (DXGetObjectClass(o)==class) {

	*n -= 1;
	return part;
	
    } else
	return NULL;
}



static Object
SetPartClass(Object o, int n, Object part, Class class)
{
    _SetPartClass(o, &n, part, class);
    if (n<0)
	return o;
    else
	DXErrorReturn(ERROR_BAD_PARAMETER, "DXSetPart of non-existent part");
}


Object
DXSetPart(Object o, int n, Field f)
{
    return SetPartClass(o, n, (Object)f, CLASS_FIELD);
}


/*
 * DXGetPartClass() is implemented by calling _GetPartClass(), which is
 * the same except the part number n is passed by reference.  If
 * o has that many parts, the part is returned and *n is set to -1.
 * Otherwise, *n is decremented by the number of parts that o has.
 */

static Object
_GetPartClass(Object o, int *n, Class class)
{
    int i;
    Object m, part;
    Class oclass = DXGetObjectClass(o);

    if (oclass==class) {

	if ((*n)-- == 0)
	    return o;
	else
	    return NULL;

    } else if (oclass==CLASS_GROUP) {

	part = NULL;
	for (i=0; *n>=0 && (m=DXGetEnumeratedMember((Group)o, i, NULL)); i++)
	    part = _GetPartClass(m, n, class);
	return part;

    } else if (oclass==CLASS_XFORM) {

	DXGetXformInfo((Xform)o, &o, NULL);
	return _GetPartClass(o, n, class);

    } else if (oclass==CLASS_CLIPPED) {

	DXGetClippedInfo((Clipped)o, &o, NULL);
	return _GetPartClass(o, n, class);

    } else if (oclass==CLASS_SCREEN) {

	DXGetScreenInfo((Screen)o, &o, NULL, NULL);
	return _GetPartClass(o, n, class);

    } else
	return NULL;

}


Object
DXGetPartClass(Object o, int n, Class class)
{
    /* sigh - nn required to work around AIX 3.2 cc -O bug */
    int nn = n;
    return _GetPartClass(o, &nn, class);
}


Field
DXGetPart(Object o, int n)
{
    return (Field) DXGetPartClass(o, n, CLASS_FIELD);
}


/*
 * DXDelete group
 */

int
_dxfGroup_Delete(Group g)
{
    int i;

    /* delete the members */
    DXdestroy_lock(&g->lock);
    for (i=0; i<g->nmembers; i++) {
	DXFree(g->members[i].name);
	DXDelete(g->members[i].value);
    }
    DXFree((Pointer)g->members);
    DXFree((Pointer)g->shape);

    return OK;
}


Group
DXSetGroupTypeV(Group g, Type t, Category c, int rank, int *shape)
{
    int i;

    CHECK(g, CLASS_GROUP);

    /* record type */
    g->typed = 1;
    g->type = t;
    g->category = c;
    g->rank = rank;
    DXFree((Pointer)g->shape);
    g->shape = (int *) DXAllocate(rank*sizeof(*shape));
    if (!g->shape)
	return NULL;
    for (i=0; i<rank; i++)
	g->shape[i] = shape[i];

    /* check type of existing members */
    for (i=0; i<g->nmembers; i++)
	if (!type_check(g, g->members[i].value))
	    return NULL;

    return g;
}


Group
DXSetGroupType(Group g, Type t, Category c, int rank, ...)
{
    int shape[100];
    int i;
    va_list arg;

    RANKTEST(NULL);

    va_start(arg,rank);
    for (i=0; i<rank; i++)
	shape[i] = va_arg(arg, int);
    va_end(arg);

    return DXSetGroupTypeV(g, t, c, rank, shape);
}


Group
DXUnsetGroupType(Group g)
{
    CHECK(g, CLASS_GROUP);

    g->typed = 0;

    DXFree((Pointer)g->shape);
    g->shape = NULL;

    return g;
}


Object
_dxfGroup_GetType(Group g, Type *t, Category *c, int *rank, int *shape)
{
    int i;

    CHECK(g, CLASS_GROUP);

    if (!g->typed)
	return NULL;

    if (t)
	*t = g->type;
    if (c)
	*c = g->category;
    if (rank)
	*rank = g->rank;
    if (shape)
	for (i=0; i<g->rank; i++)
	    shape[i] = g->shape[i];

    return (Object) g;
}
