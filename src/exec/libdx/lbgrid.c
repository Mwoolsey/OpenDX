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
#include "arrayClass.h"


/*
 * General regular grids.  A regular grid in n-space is specified by
 * an n-dimensional origin and a set of n n-dimensional deltas.
 */

Array
DXMakeGridPositionsV(int n, int *counts, float *origin, float *deltas)
{
    Array as[100];
    float zero[100];
    int i;
    
    ASSERT(n<100);

    /* zero */
    for (i=0; i<n; i++)
	zero[i] = 0;

    /* create the terms */
    for (i=0; i<n; i++) {
	as[i] = (Array) DXNewRegularArray(TYPE_FLOAT, n, counts[i],
	    (Pointer)(i==0? origin : zero), (Pointer)&deltas[i*n]);
	if (!as[i])
	    return NULL;
    }

    /* create the product */
    return (Array) DXNewProductArrayV(n, as);
}


Array DXMakeGridPositions(int n, ...)
{
    int counts[100];
    float origin[100], deltas[100];
    int i;
    va_list arg;

    ASSERT(n<100);

    /* collect args */
    va_start(arg,n);
    for (i=0; i<n; i++)
	counts[i] = va_arg(arg, int);
    for (i=0; i<n; i++)
	origin[i] = va_arg(arg, double);
    for (i=0; i<n*n; i++)
	deltas[i] = va_arg(arg, double);
    va_end(arg);

    /* call V version */
    return DXMakeGridPositionsV(n, counts, origin, deltas);
}


Array
DXQueryGridPositions(Array a, int *n, int *counts, float *origin, float *deltas)
{
    Array as[100];
    float orig[100];
    int i, j, nn;

    /* is it a product array? */
    if (DXGetArrayClass(a)!=CLASS_PRODUCTARRAY)
	return NULL;

    /* get and return n */
    if (!DXGetProductArrayInfo((ProductArray)a, &nn, as))
	return NULL;
    ASSERT(nn<100);
    if (n)
	*n = nn;

    /* is it floating point? (implies terms are floating point) */
    /* all other type checking is done by e.g. DXGetProductArrayInfo() */
    if (a->type != TYPE_FLOAT)
	return NULL;

    /* is # terms == dimensionality? */
    if (a->rank!=1 || nn!=a->shape[0])
	return NULL;

    /* zero origin */	
    if (origin)
	for (i=0; i<nn; i++)
	    origin[i] = 0;

    /* are the terms regular arrays? */
    for (i=0; i<nn; i++) {
	ASSERT(as[i]->type==TYPE_FLOAT);
	ASSERT(as[i]->shape[0]==nn);
	ASSERT(as[i]->rank==1);
	if (DXGetArrayClass(as[i])!=CLASS_REGULARARRAY)
	    return NULL;
	if (!DXGetRegularArrayInfo((RegularArray)(as[i]),
				 counts? counts+i: NULL,
				 (Pointer)orig,
				 (Pointer)(deltas? deltas+i*nn : NULL)))
	    return NULL;
	if (origin)
	    for (j=0; j<nn; j++)
		origin[j] += orig[j];
    }	

    return a;
}


/*
 * Aligned grid connections
 */
static char *
elementname(int rank, char *str)
{
    switch(rank) {
      case 0:
	strcpy(str, "points");
	break;
      case 1:
	strcpy(str, "lines");
	break;
      case 2:
	strcpy(str, "quads");
	break;
      case 3:
	strcpy(str, "cubes");
	break;
      default:
	sprintf(str, "cubes%dD", rank);
	break;
    }
    return str;
}


Array
DXMakeGridConnectionsV(int n, int *counts)
{
    Array as[100];
    Array a;
    char et[32];
    int i;
    
    ASSERT(n<100);

    /* create the terms */
    for (i=0; i<n; i++) {
	as[i] = (Array) DXNewPathArray(counts[i]);
	if (!as[i])
	    return NULL;
    }

    /* create the product */
    a = (Array) DXNewMeshArrayV(n, as);
    if (!a)
	return NULL;
    
    /* set the element type */
    DXSetStringAttribute((Object)a, "element type", elementname(n, et));
    return a;
}


Array DXMakeGridConnections(int n, ...)
{
    int counts[100];
    int i;
    va_list arg;

    ASSERT(n<100);

    /* collect args */
    va_start(arg,n);
    for (i=0; i<n; i++)
	counts[i] = va_arg(arg, int);
    va_end(arg);

    /* call V version */
    return DXMakeGridConnectionsV(n, counts);
}


Array
DXQueryGridConnections(Array a, int *n, int *counts)
{
    Array as[100];
    int i, nn;

    /* is it a product? */
    if (DXGetArrayClass(a)!=CLASS_MESHARRAY)
	return NULL;

    /* get and return n */
    if (!DXGetMeshArrayInfo((MeshArray)a, &nn, as))
	return NULL;
    ASSERT(nn<100);
    if (n)
	*n = nn;

    /* are the terms regular connections? */
    for (i=0; i<nn; i++) {
	if (DXGetArrayClass(as[i])!=CLASS_PATHARRAY)
	    return NULL;
	if (!DXGetPathArrayInfo((PathArray)as[i], counts? &(counts[i]) : NULL))
	    return NULL;
    }

    return a;
}
