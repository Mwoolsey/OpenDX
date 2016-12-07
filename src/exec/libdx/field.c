/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>



#include <string.h>
#include "fieldClass.h"
#include "internals.h"


static Field _CopyField(Field new, Field old, enum _dxd_copy copy);

static Field
_NewField(struct field_class *class)
{
    Field f = (Field) _dxf_NewObject((struct object_class *)class);

    if (!f)
	return NULL;

    f->components = f->local;
    f->ncomponents = 0;
    f->comp_alloc = NCOMPONENTS;

    return f;
}


Field
DXNewField()
{
    return _NewField(&_dxdfield_class);
}


Object
_dxfField_GetType(Field f, Type *t, Category *c, int *rank, int *shape)
{
    Array a = (Array) DXGetComponentValue(f, "data");
    if (!a)
	return NULL;
    return DXGetType((Object)a, t, c, rank, shape);
}

static
struct component *
look_component(Field f, char *name, int add)
{
    int i=0, m, n = f->ncomponents;
    struct component *c = NULL;

    /* is component there? */
    if (name) {
	/* quick check for pointer equality first */
	if (!add)
	    for (i=0, c=f->components; i<n; i++, c++)
		if (c->name==name)
		    break;
	/* now check for string equality */
	if (add || i>=n)
	    for (i=0, c=f->components; i<n; i++, c++)
		if (strcmp(c->name, name)==0)
		    break;

    } else {
	/* null name => new component */
	i = n;
    }	

    /* component is not there - add it */
    if (i >= n) {
	if (add) {
	    /* re-alloc if necessary */
	    if (n >= f->comp_alloc) {
		if (f->comp_alloc==NCOMPONENTS) {
		    m = 2*NCOMPONENTS;
		    c = (struct component *)
			DXAllocate(m*sizeof(struct component));
		    if (c)
			memcpy(c, f->local, sizeof(f->local));
		} else {
		    m = f->comp_alloc*2 + 1;
		    c = (struct component *) DXReAllocate((Pointer)f->components,
			m * sizeof(struct component));
		}
		if (!c)
		    return NULL;
		f->components = c;
		f->comp_alloc = m;
	    }
	    c = f->components + n;
	    f->ncomponents = n+1;
	    c->name = _dxfstring(name, 1);
	    c->value = NULL;
	} else
	    c = NULL;
    }
    return c;
}


Field
DXSetComponentValue(Field f, char *name, Object value)
{
    struct component *c;

    /* if value is null, this is a request to delete this component. */
    if (!value)
    {
	DXDeleteComponent(f, name);
	return f;
    }

    CHECK(f, CLASS_FIELD);

    /* is component there? */
    c = look_component(f, name, value? 1: 0);
    if (!c)
	return NULL;

    /* put value in */
    if (value!=c->value) {		/* MP performance */
	DXReference(value);		/* do first in case value==c->value */
	if (value && c->value)
	    DXCopyAttributes(value, c->value);
	DXDelete(c->value);
	c->value = value;
    }

    return f;
}


Field
DXSetComponentAttribute(Field f, char *name, char *attribute, Object value)
{
    Object c;
    c = DXGetComponentValue(f, name);
    if (!c)
	return NULL;
    return DXSetAttribute(c, attribute, value)? f : NULL;
}


Object
DXGetComponentValue(Field f, char *name)
{
    struct component *c;

    CHECK(f, CLASS_FIELD);
    if (!name)
	return NULL;

    /* is component there? */
    c = look_component(f, name, 0);
    if (!c)
	return NULL;

    /* component is there */
    return c->value;
}


Object
DXGetComponentAttribute(Field f, char *name, char *attribute)
{
    Object c;
    c = DXGetComponentValue(f, name);
    if (!c)
	return NULL;
    return DXGetAttribute(c, attribute);
}


Object
DXGetEnumeratedComponentValue(Field f, int n, char **name)
{
    struct component *c;

    CHECK(f, CLASS_FIELD);

    /* is component there? */
    if (n >= f->ncomponents)
	return NULL;
    c = &(f->components[n]);

    /* component is there */
    if (name)
	*name = c->name;
    return c->value;
}


Object
DXGetEnumeratedComponentAttribute(Field f, int n, char **name, char *attribute)
{
    Object c;
    c = DXGetEnumeratedComponentValue(f, n, name);
    if (!c)
	return NULL;
    return DXGetAttribute(c, attribute);
}


static
int
_ComponentXX(Array a, Pointer *data, int *n, int nreq, Type t, int dim,
		 int REQUIRED, int LOCAL)
{
    int nn;
    Type tt;
    Category category;
    int shape;
    int rank;

    /* return 0 in *items, return NULL if component not there */
    if (!a) {
	if (n)
	    *n = 0;
	if (data)
	    *data = NULL;
	if (REQUIRED) {
	    DXErrorReturn(ERROR_MISSING_DATA, "missing component");
	} else
	    return OK;
    }
    
    /* get data */
    if (data)
	*data = LOCAL? DXGetArrayDataLocal(a) : DXGetArrayData(a);

    /* more info */
    DXGetArrayInfo(a, &nn, &tt, &category, &rank, NULL);

    /* return or check number of items */
    if (n)
	*n = nn;
    else if (nreq!=nn) {
	DXSetError(ERROR_DATA_INVALID,
		 "component has %d items, requires %d", nn, nreq);
	return ERROR;
    }

    /* check type and category */
    if (tt!=t || category!=CATEGORY_REAL)
	DXErrorReturn(ERROR_BAD_TYPE,
		    "component has bad type or category");

    /* check shape & rank */
    /* XXX - loosened up the checking, hope this doesn't hurt */
    if (rank==0) {
	if (dim!=0 && dim!=1)
	    DXErrorReturn(ERROR_BAD_TYPE, "component has wrong dimensionality");
    } else if (rank==1) {
	DXGetArrayInfo(a, NULL, NULL, NULL, NULL, &shape);
	if (shape != (dim?dim:1))
	    DXErrorReturn(ERROR_BAD_TYPE, "component has wrong dimensionality");
    } else
	DXErrorReturn(ERROR_BAD_TYPE, "component has wrong dimensionality");

    return OK;
}


int DXComponentReq(Array a, Pointer *data, int *n, int nreq, Type t, int dim)
{
    return _ComponentXX(a, data, n, nreq, t, dim, 1, 0);
}


int DXComponentOpt(Array a, Pointer *data, int *n, int nreq, Type t, int dim)
{
    return _ComponentXX(a, data, n, nreq, t, dim, 0, 0);
}


int DXComponentReqLoc(Array a, Pointer *data, int *n, int nreq, Type t, int dim)
{
    return _ComponentXX(a, data, n, nreq, t, dim, 1, 1);
}


int DXComponentOptLoc(Array a, Pointer *data, int *n, int nreq, Type t, int dim)
{
    return _ComponentXX(a, data, n, nreq, t, dim, 0, 1);
}


Field
DXDeleteComponent(Field f, char *component)
{
    int i, n = f->ncomponents;
    struct component *c;

    CHECK(f, CLASS_FIELD);
    if (!f)
	return NULL;
    if (!component)
	return NULL;

    /* is component there? */
    c = look_component(f, component, 0);
    if (!c)
	return NULL;

    /* free it */
    DXDelete(c->value);

    /* copy remaining components down */
    for (i=c-f->components+1; i<n; i++)
	f->components[i-1] = f->components[i];
    f->ncomponents -= 1;

    return f;
}


int
_dxfField_Delete(Field f)
{
    int i, n;
    struct component *c;

    n = f->ncomponents;
    for (i=0, c=f->components; i<n; i++, c++)
	DXDelete(c->value);
    if (f->components!=f->local)
	DXFree((Pointer)f->components);

    return OK;
}


int
DXEmptyField(Field f)
{
    Array a;
    int n;
    if (f->ncomponents==0)
	return 1;
    a = (Array) DXGetComponentValue(f, POSITIONS);
    if (!a)
	return 1;
    DXGetArrayInfo(a, &n, NULL, NULL, NULL, NULL);
    if (n==0)
	return 1;
    return 0;
}


/*
 * Produces a new field that shares components, etc. with existing
 * field.
 *
 * XXX - should this be done instead by putting a pointer to old
 * field in new field and doing sharing at lookup?  That is, should
 * semantics be to see changes made to old field?
 */

Object
_dxfField_Copy(Field old, enum _dxd_copy copy)
{
    Field new;

    /* create new field */
    new = DXNewField();
    if (!new)
	return NULL;

    /* XXX - check return code and delete - but clean up _CopyField first! */
    return (Object) _CopyField(new, old, copy);
}


static Field
_CopyField(Field new, Field old, enum _dxd_copy copy)
{
    int i, n = old->ncomponents;
    struct component *oc, *nc;

    /* copy superclass */
    if (!_dxf_CopyObject((Object)new, (Object)old, copy))
	return NULL;
    if (copy==COPY_ATTRIBUTES)
	return new;

    /* allocate components */
    if (n<=NCOMPONENTS) {
	nc = new->local;
	new->comp_alloc = NCOMPONENTS;
    } else {
	nc = (struct component *)  DXAllocate(n*sizeof(struct component));
	if (!nc)
	    return NULL;
	new->comp_alloc = n;
    }
    new->components = nc;
    new->ncomponents = n;

    /* fill in components */
    for (i=0, oc=old->components; i<n; i++, oc++, nc++) {
	nc->name = oc->name;
	if (copy==COPY_DATA) {
	    nc->value = DXCopy(oc->value, copy);
	    if (nc->value==NULL) /* XXX - cleanup! */
		return NULL;
	} else
	    nc->value = DXReference(oc->value);
    }	

    return new;
}




