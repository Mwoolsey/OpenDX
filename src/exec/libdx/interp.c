/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>



#include <stdio.h>
#include <math.h>
#include <string.h>
#include <dx/dx.h>
#include "interpClass.h"

typedef long CRC;

Interpolator _dxfNewGroupInterpolator(Group, 
			enum interp_init, float, Matrix *);

Interpolator _dxfSelectFieldInterpolator(Field, 
			enum interp_init, float, Matrix *);

static Error _dxfValidInterpolatorMap(Object);


static struct PrimTypes {
    char 	*name;
    int		dimensionality;
    int		ptsPerPrimitive;
} _dxdprimTypes[] = 
{
    { "lines",		1,	2 },
    { "triangles",	2,	3 },
    { "quads",		2,	4 },
    { "cubes",		3,	8 },
    { "tetrahedra",	3,	4 },
    { NULL,		0,	0 }
};

Interpolator
DXNewInterpolator(Object o, enum interp_init initType, float fuzz)
{
    Interpolator interp = NULL;
    Object copy = NULL;

    if (fuzz < 0.0)
	fuzz = FUZZ;

    if (! _dxfValidInterpolatorMap(o))
	goto error;

    copy = DXCopy(o, COPY_HEADER);
    if (! copy)
	goto error;

    if (initType == INTERP_INIT_PARALLEL)
	DXCreateTaskGroup();
    
    interp = _dxfNewInterpolatorSwitch(copy, initType, fuzz, NULL);
    if (! interp)
    {
	if (initType == INTERP_INIT_PARALLEL)
	    DXAbortTaskGroup();

	goto error;
    }

    if (initType == INTERP_INIT_PARALLEL)
	if (! DXExecuteTaskGroup() || DXGetError() != ERROR_NONE)
	    goto error;

    interp->fuzz = fuzz;

    interp->rootObject = DXReference(copy);

    return interp;

error:
    DXDelete((Object)interp);
    DXDelete((Object)copy);
    return NULL;
}

Interpolator
_dxfNewInterpolatorSwitch(Object o, 
		enum interp_init initType, float fuzz, Matrix *stack)
{
    Interpolator interp = NULL;
    Class        class;
    Object       child;
    Matrix	 mat, prod;

    class = DXGetObjectClass(o);
    /*
    if (class == CLASS_GROUP)
	class = DXGetGroupClass((Group)o);
    */
    
    /*
     * Determine class of input and call appropriate Interpolator
     * sub-class method
     */
    switch (class)
    {
	case CLASS_XFORM:
	    if (stack)
	    {
		if (! DXGetXformInfo((Xform)o, &child, &mat))
		    goto error;
	    
		prod = DXConcatenate(mat, *stack);
	    }
	    else
		if (! DXGetXformInfo((Xform)o, &child, &prod))
		    goto error;

	    interp = _dxfNewInterpolatorSwitch(child,
						initType, fuzz, &prod);

	    if (! interp)
		goto error;
	    
	    break;
	    
	case CLASS_GROUP:
	    interp = (Interpolator)_dxfNewGroupInterpolator((Group)o,
						    initType, fuzz, stack);
	    if (! interp)
		goto error;

	    break;
	case CLASS_FIELD:
	    interp = (Interpolator)_dxfSelectFieldInterpolator((Field)o,
						    initType, fuzz, stack);
	    if (! interp)
		goto error;

	    break;
	default:
	    DXSetError(ERROR_BAD_CLASS, "#11381", "map");
	    interp = NULL;
    }

    return interp;

error:
    DXDelete((Object)interp);
    return NULL;
}

Interpolator
_dxf_NewInterpolator(struct interpolator_class *class, Object o)
{
    Interpolator i;

   i =  (Interpolator)_dxf_NewObject((struct object_class *)class);
   if (! i)
       return NULL;

    i->dataObject = o;
    i->rootObject = NULL;

    memset(&i->matrix, 0, sizeof(Matrix));

    return i;
}

int
_dxfInterpolator_Delete(Interpolator interp)
{
    if (interp->rootObject)
	DXDelete(interp->rootObject);
    return OK;
}

Interpolator
DXInterpolate(Interpolator interp, int *n, float *p, Pointer v)
{
    int nInterpolated, nAtStart;

    /*
     * Try to interpolate a list of points.  First, try interpolation
     * without fuzz; if points remain, we'll have to try interpolation
     * with fuzz.
     */
    if (! _dxfInterpolate(interp, n, &p, &v, NULL, FUZZ_OFF))
	return NULL;
    
    /*
     * If points remain, loop.  When the fuzz flag is turned on, the
     * first point will be interpolated with fuzz.  If this succeeds,
     * subsequent interpolations will proceed without fuzz until a point
     * cannot be interpolated.  At this point, control passes back up here
     * and we start again with fuzz.  This loop terminates when no point is
     * interpolated with or without fuzz or when all points have been 
     * interpolated.
    */
    if (interp->fuzz > 0.0 && *n)
	do
	{
	    nAtStart = *n;

	    if (! _dxfInterpolate(interp, n, &p, &v, NULL, FUZZ_ON))
		return NULL;
	
	    nInterpolated = nAtStart - *n;

	} while (nInterpolated && *n);

    return interp;
}

Object
_dxfInterpolator_GetType(Interpolator interp, Type *type, 
			Category *category, int *rank, int *shape)
{
    if (interp->rootObject == NULL)
	return NULL;
    else
	return DXGetType(interp->rootObject, type, category, rank, shape);
}

Interpolator
_dxf_CopyInterpolator(Interpolator new, Interpolator src)
{
    int 	 i;

    if (src->rootObject)
	new->rootObject = DXReference(src->rootObject);
    else
	new->rootObject = NULL;

    new->dataObject = src->dataObject;

    new->fuzz = src->fuzz;

    new->nDim = src->nDim;
    for (i = 0; i < src->nDim; i++)
    {
	new->max[i] = src->max[i];
	new->min[i] = src->min[i];
    }

    new->type     = src->type;
    new->category = src->category;
    new->rank     = src->rank;
    new->matrix   = src->matrix;

    for (i = 0; i < src->rank; i++)
	 new->shape[i] = src->shape[i];
	
    return new;
}

static Error
_dxfValidInterpolatorMap(Object map)
{
    char *name;
    Class class;

    name = NULL;

    class = DXGetObjectClass(map);
    if (class == CLASS_GROUP)
	class = DXGetGroupClass((Group)map);

    if (class == CLASS_COMPOSITEFIELD || class == CLASS_MULTIGRID)
    {
	Object  child;
	int	i;

	i = 0;
	while (NULL != (child = DXGetEnumeratedMember((Group)map, i++, NULL)))
	    if (! _dxfValidInterpolatorMap(child))
		return ERROR;
	
	return OK;
    }
    else if (class == CLASS_XFORM)
    {
	Object c;

	if (! DXGetXformInfo((Xform)map, &c, NULL))
	    return ERROR;
	
	return _dxfValidInterpolatorMap(c);
    }
    else if (class == CLASS_FIELD)
    {
	struct PrimTypes *prim;
	Array  		 array;
	int   		 rank, shape[32];
	int		 nPrims;
	Object		 at;

	/*
	 * Allow empty fields and fields with no connections
	 */
	if (DXEmptyField((Field)map))
	    return OK;
	
	array = (Array)DXGetComponentValue((Field)map, "connections");
	if (! array)
	    return OK;
	
	DXGetArrayInfo(array, &nPrims, NULL, NULL, NULL, NULL);
	if (nPrims < 0)
	    return OK;

	at = DXGetComponentAttribute((Field)map, "connections", "element type");
	if (! at || DXGetObjectClass(at) != CLASS_STRING)
	{
	    DXSetError(ERROR_MISSING_DATA, "#10255", "connections", 
							"element type");
	    return ERROR;
	}

	name = DXGetString((String)at);

	for (prim = _dxdprimTypes; prim->name != NULL; prim ++)
	    if (! strcmp(prim->name, name))
		break;

	if (prim->name == NULL)
	{
	    DXSetError(ERROR_DATA_INVALID, "#11380", "name");
	    return ERROR;
	}

	array = (Array)DXGetComponentValue((Field)map, "positions");
	if (! array)
	{
	    DXSetError(ERROR_MISSING_DATA, "#10240", "map", "positions");
	    return ERROR;
	}

	DXGetArrayInfo(array, NULL, NULL, NULL, NULL, shape);

	if (shape[0] != prim->dimensionality)
	{
	    DXSetError(ERROR_DATA_INVALID, "#11003",
				    prim->name, prim->dimensionality);
	    return ERROR;
	}

	array = (Array)DXGetComponentValue((Field)map, "connections");
	DXGetArrayInfo(array, NULL, NULL, NULL, &rank, shape);

	if (rank != 1 || shape[0] != prim->ptsPerPrimitive)
	{
	    DXSetError(ERROR_DATA_INVALID, "#11004",
			prim->name, prim->ptsPerPrimitive);
	    return ERROR;
	}

	return OK;
    }
    else
    {
	DXSetError(ERROR_DATA_INVALID, "#11381");
	return ERROR;
    }
}

Interpolator
DXLocalizeInterpolator(Interpolator o)
{
    return _dxfLocalizeInterpolator(o);
}
