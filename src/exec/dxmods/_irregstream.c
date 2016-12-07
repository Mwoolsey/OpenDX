/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dxmods/_irregstream.c,v 1.9 2006/01/03 17:02:21 davidt Exp $
 */

#include <dxconfig.h>

#if defined(HAVE_STRING_H)
#include <string.h>
#endif

#include <dx/dx.h>
#include <math.h>
#include "stream.h"
#include "vectors.h"
#include "_divcurl.h"
#include "_partnbrs.h"

typedef struct neighborsHandle
{
    Array array;
    int   *neighbors;
    int   nPerE, nDim, wrap;
    int   gcounts[10];
    int   nstrides[10];
} *NeighborsHandle;

/*
 ***********************************
 * Irreg interface
 ***********************************
 */

typedef struct irreg_InstanceVars
{
    struct instanceVars i;
    int    	        cp;		/* current partition index  	     */
    int    	        ce;		/* current element index 	     */
    int    	        ct;		/* current primitive         	     */
    int		        flags;		/* flags for zero-area faces	     */
    float	        w[4];		/* current weights                   */
    float	        planes[16];	/* four face planar equations        */
    int		        face;		/* intersection face		     */
    float	        edges[18];	/* primitive edge vectors	     */
    ArrayHandle	        pHandle;
    ArrayHandle         cHandle;
    NeighborsHandle     nHandle;
} *Irreg_InstanceVars;

/*
 * The following table contains an entry for each face of each of the six 
 * tetras within the element.  If the face is external to the element, nelt is
 * the index of the element neighbor pointer for that element face, -1
 * otherwise. nprim is the index of the containing tetra within either
 * the neighbor element (if nelt != -1) or the current tetra (if nelt == -1).
 */
typedef struct {
    int nelt;
    int nprim;
} NbrTable;

typedef struct irreg_VectorGrp  *Irreg_VectorGrp;
typedef struct irreg_VectorPart *Irreg_VectorPart;

/*
 * defines for flag field
 *    PARITY_KNOWN     		the parity of the element list is known
 *    CURRENT_DEGENERATE	the current element is degenerate
 *    
 * the low 16 bits contain flags denoting whether the corresponding
 * face's planar equation must be reversed.
 */
#define PARITY_KNOWN     	0x0100
#define CURRENT_DEGENERATE	0x0200

#define IsParityKnown(i) \
	(((Irreg_InstanceVars)(i))->flags & PARITY_KNOWN)
#define SetParityKnown(i) \
	(((Irreg_InstanceVars)(i))->flags |= PARITY_KNOWN)

#define IsCurrentDegenerate(i) \
	(((Irreg_InstanceVars)(i))->flags & CURRENT_DEGENERATE)
#define SetCurrentDegenerate(i) \
	(((Irreg_InstanceVars)(i))->flags |= CURRENT_DEGENERATE)
#define ResetCurrentDegenerate(i) \
	(((Irreg_InstanceVars)(i))->flags &= ~CURRENT_DEGENERATE)

#define IsFaceNReversed(i,n) \
	(((Irreg_InstanceVars)(i))->flags & (1 << n))
#define SetFaceNReversed(i,n) \
	(((Irreg_InstanceVars)(i))->flags |= (1 << n))

struct irreg_VectorGrp
{
    struct vectorGrp  P;
    int      	      *primTable;
    NbrTable 	      *nbrTable;
    int		      (*InitElement)(Irreg_InstanceVars);
    int		      (*FaceWeights)(Irreg_InstanceVars, POINT_TYPE *);
    int	     	      primsPerElement;
    int		      nbrsPerElement;
    int		      vrtsPerElement;
    int		      nbrsPerPrim;
    int		      nDimensions;
};

struct irreg_VectorPart
{
    struct vectorPart 	   p;
    int    	      	   nElements;
    int    	      	   nPositions;
    int	   	      	   *elements;
    int	   	      	   *neighbors;
    int	   	      	   *partitionNeighbors;
    float  	      	   *points;
    int			   constantData;
    float  	      	   *vectors;
    byte		   *visited;
    byte	      	   gridConnections;
    int		      	   nd;
    float	      	   origin[3];
    int		      	   pcounts[3];
    float	      	   deltas[9];
    int		      	   gcounts[3];
    int		      	   gstrides[3];
    int		     	   gbumps[8];
    int		      	   nstrides[3];
    int		      	   pstrides[3];
    int		      	   wrapFlags;
    InvalidComponentHandle invElements;
    Array	      	   pArray, cArray, nArray;
};

#define SetElementVisited(ip, i) 			\
{							\
    if (i < 0 || i > (ip)->nElements)			\
	DXSetError(ERROR_INTERNAL, "invalid visited");  \
    (ip)->visited[i] = 1;				\
}

#define IsElementVisited(ip, i) ((ip)->visited[i] == 1)

/*
 ***********************************
 * Tetrahedra definition
 ***********************************
 */

static int tetrahedra_primTable[] = 
{
    0, 1, 2, 3
};

static NbrTable tetrahedra_nbrTable[] =
{
    {0, 0}, {1, 0}, {2, 0}, {3, 0}
};

#define TETRAHEDRA_N_PRIMS	1
#define TETRAHEDRA_N_NBRS	4
#define TETRAHEDRA_N_VRTS	4
#define TETRAHEDRA_N_DIMS	3
#define TETRAHEDRA_N_PRIM_NBRS	4

/*
 ***********************************
 * Triangle definition
 ***********************************
 */

static int triangles_primTable[] = 
{
    0, 1, 2
};

static NbrTable triangles_nbrTable[] =
{
    {0, 0}, {1, 0}, {2, 0}
};

#define TRIANGLES_N_PRIMS	1
#define TRIANGLES_N_NBRS	3
#define TRIANGLES_N_VRTS	3
#define TRIANGLES_N_DIMS	2
#define TRIANGLES_N_PRIM_NBRS	3

/*
 ***********************************
 * Irregular cubes definition
 ***********************************
 */

static int icubes_primTable[] =
{
    0, 3, 5, 1,
    5, 2, 7, 3,
    5, 4, 7, 2,
    5, 3, 0, 2,
    5, 2, 0, 4,
    4, 2, 6, 7
};

#define ICUBES_N_PRIMS		6
#define ICUBES_N_NBRS		6
#define ICUBES_N_VRTS		8
#define ICUBES_N_DIMS		3
#define ICUBES_N_PRIM_NBRS	4

static NbrTable icubes_nbrTable[] =
{
    { 5, 4}, { 2, 1}, { 0, 2}, {-1, 3},
    { 3, 0}, { 5, 5}, {-1, 3}, {-1, 2},
    {-1, 5}, {-1, 1}, {-1, 4}, { 1, 0},
    { 0, 5}, {-1, 4}, {-1, 1}, {-1, 0},
    { 4, 0}, { 2, 5}, {-1, 2}, {-1, 3},
    { 3, 4}, { 1, 3}, {-1, 2}, { 4, 1}
};

/*
 ***********************************
 * Irregular quads definition
 ***********************************
 */

static int iquads_primTable[] =
{
    0, 3, 1,
    0, 2, 3
};

#define IQUADS_N_PRIMS		2
#define IQUADS_N_NBRS		4
#define IQUADS_N_VRTS		4
#define IQUADS_N_DIMS		2
#define IQUADS_N_PRIM_NBRS	3

static NbrTable iquads_nbrTable[] =
{
    { 3, 1}, { 0, 1}, {-1, 1},
    { 1, 0}, {-1, 0}, { 2, 0}
};

/*
 ***********************************
 * Element definition table
 ***********************************
 */
typedef struct
{
    char     	      *elementType;
    int      	      *primTable;
    NbrTable 	      *nbrTable;
    int	     	      primsPerElement;
    int		      nbrsPerElement;
    int		      vrtsPerElement;
    int		      nDimensions;
    int		      nbrsPerPrim;
} ElementDef;

static ElementDef   elementTable[] =
{
    {
	"tetrahedra",
	tetrahedra_primTable,
	tetrahedra_nbrTable, 
	TETRAHEDRA_N_PRIMS,
	TETRAHEDRA_N_NBRS,
	TETRAHEDRA_N_VRTS,
	TETRAHEDRA_N_DIMS,
	TETRAHEDRA_N_PRIM_NBRS
    },
    {
	"triangles",
	triangles_primTable,
	triangles_nbrTable, 
	TRIANGLES_N_PRIMS,
	TRIANGLES_N_NBRS,
	TRIANGLES_N_VRTS,
	TRIANGLES_N_DIMS,
	TRIANGLES_N_PRIM_NBRS
    },
    {
	"cubes",
	icubes_primTable,
	icubes_nbrTable, 
	ICUBES_N_PRIMS,
	ICUBES_N_NBRS,
	ICUBES_N_VRTS,
	ICUBES_N_DIMS,
	ICUBES_N_PRIM_NBRS
    },
    {
	"quads",
	iquads_primTable,
	iquads_nbrTable, 
	IQUADS_N_PRIMS,
	IQUADS_N_NBRS,
	IQUADS_N_VRTS,
	IQUADS_N_DIMS,
	IQUADS_N_PRIM_NBRS
    },
    {
	NULL,
	NULL,
	NULL, 
	-1,
	-1,
	-1,
	-1
    }
};

static VectorPart   Irreg_InitVectorPart(Field, Irreg_VectorGrp);
static Error        Irreg_DestroyVectorPart(VectorPart);
static int          Irreg_FindElement_VectorPart(Irreg_InstanceVars,
							int, POINT_TYPE *);
static int          Irreg_FindMultiGridContinuation_VectorPart(
					Irreg_InstanceVars, int, POINT_TYPE *);
static InstanceVars Irreg_NewInstanceVars(VectorGrp);
static Error        Irreg_FreeInstanceVars(InstanceVars);
static int          Irreg_FindElement(InstanceVars, POINT_TYPE *);
static int          Irreg_FindMultiGridContinuation(InstanceVars, POINT_TYPE *);
static int          Irreg_Interpolate(InstanceVars, POINT_TYPE *,
					VECTOR_TYPE *);
static Error        Irreg_StepTime(InstanceVars, double,
					VECTOR_TYPE *, double *);
static Error        Irreg_Walk(InstanceVars, POINT_TYPE *,
					VECTOR_TYPE *, POINT_TYPE *);
static Error        Irreg_FindBoundary(InstanceVars, POINT_TYPE *,
					VECTOR_TYPE *, double *);
static int          Irreg_Neighbor(InstanceVars, VECTOR_TYPE *);
static Error        Irreg_Delete(VectorGrp);
static Error        Irreg_CurlMap(VectorGrp, MultiGrid);
static Error        Irreg_CurlMap_Task(Pointer);
static Error        Irreg_CurlMap_Field(Irreg_VectorGrp,
					Irreg_VectorPart, Group, Field *);
static Error	    Irreg_ResetVectorGrp(VectorGrp);
static Error	    Irreg_ResetVectorPart(VectorPart);
static int	    Irreg_Ghost(InstanceVars I, POINT_TYPE *p);

static int Tetras_Weights(InstanceVars, POINT_TYPE *);
static int Tetras_FaceWeights(InstanceVars, POINT_TYPE *);
static int Tetras_InitElement(Irreg_InstanceVars);
static int Tris_Weights(InstanceVars, POINT_TYPE *);
static int Tris_FaceWeights(InstanceVars, POINT_TYPE *);
static int Tris_InitElement(Irreg_InstanceVars);

static NeighborsHandle
DXCreateNeighborsHandle(Field field)
{
    Array cA;
    NeighborsHandle handle=NULL;

    cA = (Array)DXGetComponentValue(field, "connections");
    if (! cA)
    {
	DXSetError(ERROR_MISSING_DATA, "no connections component");
	goto error;
    }

    handle = (NeighborsHandle)DXAllocateZero(sizeof(struct neighborsHandle));
    if (! handle)
	goto error;

    handle->wrap = 0;
    if (DXGetAttribute((Object)cA, "wrap I")) handle->wrap |= 0x1;
    if (DXGetAttribute((Object)cA, "wrap J")) handle->wrap |= 0x2;
    if (DXGetAttribute((Object)cA, "wrap K")) handle->wrap |= 0x4;

    if (DXQueryGridConnections(cA, &handle->nDim, handle->gcounts))
    {
	int i;

	for (i = 0; i < handle->nDim; i++)
	    handle->gcounts[i] -= 1;

	handle->nstrides[handle->nDim - 1] = 1;
	for (i = handle->nDim-2; i >= 0; i--)
	    handle->nstrides[i] = handle->nstrides[i+1] * handle->gcounts[i+1];
    }
    else
    {
	handle->array = DXNeighbors(field);
	if (! handle->array)
	    goto error;
	
	DXReference((Object)(handle->array));
	
	DXGetArrayInfo(handle->array, NULL, NULL, NULL, NULL, &handle->nPerE);

	handle->neighbors = (int *)DXGetArrayData(handle->array);
    }

    return handle;

error:
    if (handle->array)
	DXDelete((Object)handle->array);
    DXFree((Pointer)handle);
    return NULL;
}

static void
DXFreeNeighborsHandle(NeighborsHandle handle)
{
    if (handle)
    {
	DXDelete((Object)handle->array);
	DXFree((Pointer)handle);
    }
}

#define GET_NEIGHBORS(handle, index, buf)		\
    (handle->neighbors ? 				\
	(handle->neighbors + handle->nPerE*(index)) :	\
	_dxfGetNeighbors(handle, index, buf))

static Error
_dxfGetIndices(int offset, int nDim, int *strides, int *indices)
{
    int i;

    for (i = 0; i < nDim-2; i++)
    {
	indices[i] = offset / strides[i];
	offset = offset % strides[i];
    }

    indices[nDim-2] = offset / strides[nDim-2];
    indices[nDim-1] = offset % strides[nDim-2];

    return OK;
}

static int
_dxfGetOffset(int nDim, int *strides, int *indices)
{
    int i, j;

    for (i = j = 0; i < nDim; i++)
        j += indices[i]*strides[i];
    
    return j;
}

int *
_dxfGetNeighbors(NeighborsHandle handle, int index, int *buf)
{
    int i, indices[10];

    _dxfGetIndices(index, handle->nDim, handle->nstrides, indices);

    for (i = 0; i < handle->nDim; i++)
    {
	if (indices[i] == 0)
	    if (handle->wrap & (1 << i))
		buf[i<<1] = index + 
			(handle->gcounts[i]-1)*handle->nstrides[i];
	    else
		buf[i<<1] = -1;
	else
	    buf[i<<1] = index - handle->nstrides[i];

	if (indices[i] == (handle->gcounts[i]-1))
	    if (handle->wrap & (1 << i))
		buf[(i<<1)+1] = index - 
			(handle->gcounts[i]-1)*handle->nstrides[i];
	    else
		buf[(i<<1)+1] = -1;
	else
	    buf[(i<<1)+1] = index + handle->nstrides[i];
    }

    return buf;
}

static InstanceVars
Irreg_NewInstanceVars(VectorGrp p)
{
    Irreg_InstanceVars iI;

    iI = (Irreg_InstanceVars)DXAllocate(sizeof(struct irreg_InstanceVars));
    if (! iI)
	return NULL;

    memset(iI, -1, sizeof(struct irreg_InstanceVars));
    iI->flags = 0;
    iI->pHandle = NULL;
    iI->cHandle = NULL;
    iI->nHandle = NULL;

    iI->i.currentVectorGrp = p;
    iI->i.currentPartition = NULL;
    iI->i.isRegular = 0;

    return (InstanceVars)iI;
}

static Error
Irreg_FreeInstanceVars(InstanceVars I)
{
    Irreg_InstanceVars iI = (Irreg_InstanceVars)I;

    if (iI)
    {
	if (I->currentPartition)
	    DXDelete(I->currentPartition);

	if (iI->pHandle)
	{
	    DXFreeArrayHandle(iI->pHandle);
	    iI->pHandle = NULL;
	}
	if (iI->cHandle)
	{
	    DXFreeArrayHandle(iI->cHandle);
	    iI->pHandle = NULL;
	}
	if (iI->nHandle)
	{
	    DXFreeNeighborsHandle(iI->nHandle);
	    iI->nHandle = NULL;
	}
	
	DXFree((Pointer)I);
    }
    return OK;
}

VectorGrp
_dxfIrreg_InitVectorGrp(Object object, char *elementType)
{
    Irreg_VectorGrp   P = NULL;
    ElementDef	      *eDef;
    int                i, nP;

    /*
     * Look in the table to see if we have the info necessary
     * to operate on this element type
     */
    for (eDef = elementTable; eDef->elementType != NULL; eDef ++)
	if (! strcmp(eDef->elementType, elementType))
	    break;
    
    if (eDef->elementType == NULL)
    {
	DXSetError(ERROR_DATA_INVALID,
		"unknown element type: %s", elementType);
	return NULL;
    }

    if (! _dxfPartitionNeighbors(object))
	 return NULL;

    if (DXGetObjectClass(object) == CLASS_FIELD)
	nP = 1;
    else
	for (nP = 0; DXGetEnumeratedMember((Group)object, nP, NULL); nP++);
    
    P = (Irreg_VectorGrp)DXAllocate(sizeof(struct irreg_VectorGrp));
    if (! P)
	goto error;
    
    P->P.n = nP;

    P->P.p = (VectorPart *)DXAllocateZero(nP * sizeof(struct irreg_VectorPart));
    if (! P->P.p)
	goto error;
    
    /*
     * Attach generic irregular element type methods to 
     * return object
     */
    P->P.NewInstanceVars   	   = Irreg_NewInstanceVars;
    P->P.FreeInstanceVars  	   = Irreg_FreeInstanceVars;
    P->P.FindElement       	   = Irreg_FindElement;
    P->P.FindMultiGridContinuation = Irreg_FindMultiGridContinuation;
    P->P.FindElement       	   = Irreg_FindElement;
    P->P.Interpolate       	   = Irreg_Interpolate;
    P->P.StepTime          	   = Irreg_StepTime;
    P->P.FindBoundary      	   = Irreg_FindBoundary;
    P->P.Neighbor          	   = Irreg_Neighbor;
    P->P.CurlMap           	   = Irreg_CurlMap;
    P->P.Delete            	   = Irreg_Delete;
    P->P.Reset            	   = Irreg_ResetVectorGrp;
    P->P.Walk            	   = Irreg_Walk;
    P->P.Ghost            	   = Irreg_Ghost;

    /*
     * Attach element type-dependent info to return object
     */
    P->primTable           = eDef->primTable;
    P->nbrTable            = eDef->nbrTable;
    P->primsPerElement     = eDef->primsPerElement;
    P->nbrsPerElement      = eDef->nbrsPerElement;
    P->vrtsPerElement      = eDef->vrtsPerElement;
    P->nbrsPerPrim         = eDef->nbrsPerPrim;
    P->nDimensions         = eDef->nDimensions;

    if ((P->P.nDim = eDef->nDimensions) == 3)
    {
	P->InitElement   = Tetras_InitElement;
	P->P.Weights     = Tetras_Weights;
	P->P.FaceWeights = Tetras_FaceWeights;
    }
    else
    {
	P->InitElement   = Tris_InitElement;
	P->P.Weights     = Tris_Weights;
	P->P.FaceWeights = Tris_FaceWeights;
    }

    if (DXGetObjectClass(object) == CLASS_FIELD)
    {
	P->P.p[0] = Irreg_InitVectorPart((Field)object, P);
	if (! P->P.p[0])
	    goto error;

	P->P.p[0]->field = (Field)object;
    }
    else
    {
	Group g = (Group)object;
	Field f;

	i = 0;
	while (NULL != (f = (Field)DXGetEnumeratedMember(g, i, NULL)))
	{
	    P->P.p[i] = Irreg_InitVectorPart(f, P);
	    if (! P->P.p[i] && DXGetError() != ERROR_NONE)
	        goto error;
	    
	    if (P->P.p[i])
		P->P.p[i]->field = f;
	    
	    i++;
	}
    }

    return (VectorGrp)P;

error:
    Irreg_Delete((VectorGrp)P);
    return NULL;
}

static VectorPart
Irreg_InitVectorPart(Field f, Irreg_VectorGrp iP)
{
    Irreg_VectorPart  ip = NULL;
    Array 	      array;
    int 	      i;

    if (DXEmptyField(f))
	return NULL;
    
    ip = (Irreg_VectorPart)DXAllocate(sizeof(struct irreg_VectorPart));
    if (! ip)
	goto error;

    if (! _dxfInitVectorPart((VectorPart)ip, f))
        goto error;
    
    ip->visited = NULL;

    ip->cArray = (Array)DXGetComponentValue(f, "connections");
    if (! ip->cArray)
	goto error;

    if (DXQueryGridConnections(ip->cArray, &ip->nd, ip->gcounts))
    {
        ip->gridConnections = 1;

        ip->gstrides[ip->nd-1] = 1;
        ip->gcounts[ip->nd-1] -= 1;
        for (i = ip->nd-2; i >= 0; i--)
        {
 	    ip->gcounts[i] -= 1;
	    ip->gstrides[i] = ip->gstrides[i+1]*ip->gcounts[i+1];
	}
    }
    else
	ip->gridConnections = 0;

    if (! DXTypeCheck(ip->cArray, TYPE_INT,
			CATEGORY_REAL, 1, iP->vrtsPerElement))
    {
	DXSetError(ERROR_DATA_INVALID, "connections");
	return NULL;
    }

    ip->wrapFlags = 0;
    if (DXGetAttribute((Object)ip->cArray, "wrap I")) ip->wrapFlags |= 0x1;
    if (DXGetAttribute((Object)ip->cArray, "wrap J")) ip->wrapFlags |= 0x2;
    if (DXGetAttribute((Object)ip->cArray, "wrap K")) ip->wrapFlags |= 0x4;
    
    DXGetArrayInfo(ip->cArray, &ip->nElements, NULL, NULL, NULL, NULL);

    if (! DXInvalidateConnections((Object)f))
	goto error;
    
    array = (Array)DXGetComponentValue(f, "invalid connections");
    if (! array)
    {
	ip->invElements = NULL;
    }
    else
    {
	ip->invElements = DXCreateInvalidComponentHandle((Object)f, NULL,
							"connections");

	if (! ip->invElements)
	    goto error;
    }

    /*
     * Also use an invalid component handle to keep track of the 
     * elements the streamline visits.  This one will be initialized to 
     * all invalid, so we ignore the incoming.
     */
    ip->visited = (byte *)DXAllocateZero(ip->nElements*sizeof(byte));
    if (! ip->visited) 
	goto error;
    
    ip->pArray = (Array)DXGetComponentValue(f, "positions");
    if (! ip->pArray)
	goto error;

    DXGetArrayInfo(ip->pArray, &ip->nPositions, NULL, NULL, NULL, NULL);

    if (! DXTypeCheck(ip->pArray, TYPE_FLOAT, CATEGORY_REAL, 1, iP->P.nDim))
    {
	DXSetError(ERROR_DATA_INVALID, "positions");
	return NULL;
    }
    
    ip->nArray = DXNeighbors(f);

    array = (Array)DXGetComponentValue(f, "partition neighbors");
    if (array)
	ip->partitionNeighbors = (int *)DXGetArrayData(array);
    else
	ip->partitionNeighbors = NULL;
    
    array = (Array)DXGetComponentValue(f, "data");
    if (! array)
    {
	DXSetError(ERROR_MISSING_DATA, "no data component found");
	goto error;
    }

    if (! DXTypeCheck(array, TYPE_FLOAT, CATEGORY_REAL, 1, iP->P.nDim))
    {
	DXSetError(ERROR_DATA_INVALID, "data");
	return NULL;
    }

    if (DXQueryConstantArray(array, NULL, NULL))
    {
	ip->constantData = 1;
	ip->vectors = (float *)DXGetConstantArrayData(array);
    }
    else
    {
	ip->constantData = 0;
	ip->vectors = (float *)DXGetArrayData(array);
    }
    
    return (VectorPart)ip;

error:
    if (ip)
	DXFree((Pointer)ip);
    return NULL;
}
    
static Error
Irreg_ResetVectorGrp(VectorGrp P)
{
    int i;

    for (i = 0; i < P->n; i++)
	if (! Irreg_ResetVectorPart(P->p[i]))
	    return ERROR;
    
    return OK;
}

static Error
Irreg_ResetVectorPart(VectorPart p)
{
    Irreg_VectorPart ip = (Irreg_VectorPart)p;

    if (ip && ip->visited)
	memset(ip->visited, 0, ip->nElements);

    return OK;
}
    
static Error
Irreg_Delete(VectorGrp P)
{
    Irreg_VectorGrp  iP = (Irreg_VectorGrp)P;

    if (iP)
    {
	int i;

	for (i = 0; i < iP->P.n; i++)
	    Irreg_DestroyVectorPart(iP->P.p[i]);

	DXFree((Pointer)iP->P.p);
	DXFree((Pointer)iP);
    }

    return OK;
}

static Error 
Irreg_DestroyVectorPart(VectorPart p)
{
    Irreg_VectorPart ip = (Irreg_VectorPart)p;

    if (ip)
    {
	DXFree((Pointer)ip->visited);
	DXFree((Pointer)ip);
    }

    return OK;
}

static int
Irreg_FindElement(InstanceVars I, POINT_TYPE *point)
{
    int i, last, r = 0;
    Irreg_InstanceVars iI = (Irreg_InstanceVars)I;
    Irreg_VectorGrp    iP = (Irreg_VectorGrp)(iI->i.currentVectorGrp);

    last = iI->cp;

    for (i = 0; i < iP->P.n; i++)
	if (last != i && iP->P.p[i] != NULL 
			&& _dxfIsInBox(iP->P.p[i], point, iP->P.nDim, 0))
	{
	    if ((r = Irreg_FindElement_VectorPart(iI, i, point)) != 0)
		break;
	}
    
    if (r == -1)
	return -1;
    else if (i == iP->P.n)
	return 0;
    else
    {
        if (I->currentPartition)
	     DXDelete(I->currentPartition);
	I->currentPartition = DXReference((Object)iP->P.p[i]->field);
	return 1;
    }
}

static int
Irreg_FindMultiGridContinuation(InstanceVars I, POINT_TYPE *point)
{
    int i, last, r = 0;
    Irreg_InstanceVars iI = (Irreg_InstanceVars)I;
    Irreg_VectorGrp    iP = (Irreg_VectorGrp)(iI->i.currentVectorGrp);

    last = iI->cp;

    for (i = 0; i < iP->P.n; i++)
	if (last != i && iP->P.p[i] != NULL 
			&& _dxfIsInBox(iP->P.p[i], point, iP->P.nDim, 0))
	{
	    if ((r = Irreg_FindElement_VectorPart(iI, i, point)) != 0)
		break;
	}
    
    /*
     * If we don't find an element containing the point, look
     * for an entry point into the volume - that is, an external
     * face of an element that the point lies in (within fuzz) and
     * where the interpolated vector moves into the element.
     */
    if (i == iP->P.n)
    {
	for (i = 0; i < iP->P.n; i++)
	    if (last != i && iP->P.p[i] != NULL 
			&& _dxfIsInBox(iP->P.p[i], point, iP->P.nDim, 1))
	{
	    r = Irreg_FindMultiGridContinuation_VectorPart(iI, i, point);
	    if (r != 0)
		break;
	}
    }

    if (r == -1)
	return -1;
    else if (i == iP->P.n)
	return 0;
    else
	return 1;
}

#define ADD_MINMAX(p, i)		 		\
{							\
    if ((p)[i] < min[i]) min[i] = (p)[i];		\
    if ((p)[i] > max[i]) max[i] = (p)[i];		\
}

static int
Irreg_FindMultiGridContinuation_VectorPart(Irreg_InstanceVars iI,
						int np, POINT_TYPE *point)
{
    Irreg_VectorGrp    iP = (Irreg_VectorGrp)(iI->i.currentVectorGrp);
    int *elt, i, j, k, l, found;
    Irreg_VectorPart ip;
    int  ebuf[8], nBuf[8], *nbrs;
    int nPrimFaces;
    NbrTable *nptr;

    if (iI->pHandle != NULL)
    {
	DXFreeArrayHandle(iI->pHandle);
	iI->pHandle = NULL;
    }

    if (iI->cHandle != NULL)
    {
	DXFreeArrayHandle(iI->cHandle);
	iI->cHandle = NULL;
    }

    if (iI->nHandle)
    {
	DXFreeNeighborsHandle(iI->nHandle);
	iI->nHandle = NULL;
    }

    iI->cp = np;

    ip = (Irreg_VectorPart)iP->P.p[np];
    if (! ip)
	goto error;
    
    iI->pHandle = DXCreateArrayHandle(ip->pArray);
    iI->cHandle = DXCreateArrayHandle(ip->cArray);
    iI->nHandle = DXCreateNeighborsHandle(ip->p.field);
    if (! iI->pHandle || ! iI->cHandle || ! iI->nHandle)
	goto error;
    
    /*
     * The following is the number of faces of primitive elements
     * within an element of the current type.  There are P.nDim+1
     * vertices (and therefore faces) if a primitive element of
     * that dimensionality.
     */
    nPrimFaces = iP->P.nDim + 1;

    for (i = 0, found = 0;
	 ! found && i < ip->nElements;
	 i++, elt += iP->vrtsPerElement)
    {
	float min[3];
	float max[3];

	if (ip->invElements && DXIsElementInvalid(ip->invElements, i))
	    continue;

	nbrs = GET_NEIGHBORS(iI->nHandle, i, nBuf);
	for (j = 0; j < iP->nbrsPerElement; j++)
	    if (nbrs[j] < 0 || (ip->invElements &&
			DXIsElementInvalid(ip->invElements, nbrs[j])))
		break;
	
	if (j == iP->vrtsPerElement)
	    continue;

	elt = (int *)DXGetArrayEntry(iI->cHandle, i, (Pointer)ebuf);

	/*
	 * First check the bounding box with fuzz
	 */
	min[0] = min[1] = min[2] =  DXD_MAX_FLOAT;
	max[0] = max[1] = max[2] = -DXD_MAX_FLOAT;


	for (j = 0; j < iP->vrtsPerElement; j++)
	{
	    float *p;
	    float pbuf[3];
	    
	    p = (float *)DXGetArrayEntry(iI->pHandle,
					elt[j], (Pointer)pbuf);

	    for (k = 0; k < iP->P.nDim; k++)
		ADD_MINMAX(p, k);
	}

	for (k = 0; k < iP->P.nDim; k++)
	{
	    float f = 0.0001 * (max[k] - min[k]);
	    if (point[k] < (min[k]-f) || point[k] > (max[k]+f))
		break;
	}
	
	if (k != iP->P.nDim)
	    continue;

	/*
	 * Now do it the hard way
	 */

	iI->cp = np;
	iI->ce = i;

	/*
	 * for each external face of a primitive element, determine
	 * whether it lies in the external boundary of the partition.
	 */
	nptr = iP->nbrTable;
	for (j = 0; ! found && j < iP->primsPerElement; j++)
	{
	    int first = 1;
	    iI->ct = j;

	    /*
	     * for each face of the primitive, if its external to the
	     * element and the element has no neighbor in that direction,
	     * then the primitive face lies in the boundary of the 
	     * partition.
	     */
	    for (k = 0; k < nPrimFaces; k++, nptr++)
	    {
		/*
		 * if the primitive face is internal to the 
		 * element, ignore it.
		 */
		if (nptr->nelt < 0)
		    continue;
		
		/*
		 * If there's a valid neighbor in that direction, ignore it.
		 */
		if (nbrs[nptr->nelt] > 0)
		    if (!ip->invElements ||
			 !DXIsElementInvalid(ip->invElements, nbrs[nptr->nelt]))
			continue;
		
		/*
		 * Only do the primitive interpolation once.
		 */
		if (first)
		{
		    first = 0;

		    (*iP->InitElement)(iI);
		    if (IsCurrentDegenerate(iI))
			continue;

		    (*iP->P.Weights)((InstanceVars)iI, point);
		}

		if (iI->w[k] < 0.0 && iI->w[k] > -0.01)
		{
		    VECTOR_TYPE v[5];
		    double dot;
		    float *plane = iI->planes + 4*k;

		    iI->w[k] = 0.0;
		    iI->face = k;
		    
		    if (! Irreg_Interpolate((InstanceVars)iI, NULL, v))
			goto error;
		    
		    dot = 0.0;
		    for (l = 0; l < iP->P.nDim; l++)
			dot += plane[l] * v[l];
		    
		    if (dot < 0.0)
			found = 1;
		}
	    }
	}
    }

    return found;

error:
    return -1;
}

static int
Irreg_FindElement_VectorPart(Irreg_InstanceVars iI, int np, POINT_TYPE *point)
{
    Irreg_VectorGrp    iP = (Irreg_VectorGrp)(iI->i.currentVectorGrp);
    int *elt, i, j, k, found;
    Irreg_VectorPart ip;
    int  ebuf[8];

    if (iI->pHandle != NULL)
    {
	DXFreeArrayHandle(iI->pHandle);
	iI->pHandle = NULL;
    }

    if (iI->cHandle != NULL)
    {
	DXFreeArrayHandle(iI->cHandle);
	iI->cHandle = NULL;
    }

    if (iI->nHandle)
    {
	DXFreeNeighborsHandle(iI->nHandle);
	iI->nHandle = NULL;
    }

    iI->cp = np;

    ip = (Irreg_VectorPart)iP->P.p[np];
    if (! ip)
	goto error;
    
    iI->pHandle = DXCreateArrayHandle(ip->pArray);
    iI->cHandle = DXCreateArrayHandle(ip->cArray);
    if (! iI->pHandle || ! iI->cHandle)
	goto error;

    for (i = 0, found = 0;
	 ! found && i < ip->nElements;
	 i++, elt += iP->vrtsPerElement)
    {
	float min[3];
	float max[3];

	if (ip->invElements && DXIsElementInvalid(ip->invElements, i))
	    continue;

	elt = (int *)DXGetArrayEntry(iI->cHandle, i, (Pointer)ebuf);

	/*
	 * First check the bounding box
	 */
	min[0] = min[1] = min[2] =  DXD_MAX_FLOAT;
	max[0] = max[1] = max[2] = -DXD_MAX_FLOAT;


	for (j = 0; j < iP->vrtsPerElement; j++)
	{
	    float *p;
	    float pbuf[3];
	    
	    p = (float *)DXGetArrayEntry(iI->pHandle,
					elt[j], (Pointer)pbuf);

	    for (k = 0; k < iP->P.nDim; k++)
		ADD_MINMAX(p, k);
	}

	for (k = 0; k < iP->P.nDim; k++)
	    if (point[k] < min[k] || point[k] > max[k])
		break;
	
	if (k != iP->P.nDim)
	    continue;

	/*
	 * Now do it the hard way
	 */

	iI->cp = np;
	iI->ce = i;

	for (j = 0; ! found && j < iP->primsPerElement; j++)
	{
	    iI->ct = j;

	    (*iP->InitElement)(iI);
	    if (! IsCurrentDegenerate(iI))
		if ((*iP->P.Weights)((InstanceVars)iI, point))
		    found = 1;
	}
    }

    if (found)
    {
       iI->nHandle = DXCreateNeighborsHandle(ip->p.field);
       if (! iI->nHandle)
	   goto error;
    }

    return found;

error:
    return -1;
}

#define CROSS2PLANE(x, p)					\
{								\
    (x)[3] = -((x)[0]*(p)[0] + (x)[1]*(p)[1] + (x)[2]*(p)[2]);  \
}

#define DOT(a,b)        ((a)[0]*(b)[0] + (a)[1]*(b)[1] + (a)[2]+(b)[2])
#define POINTPLANE(a,b) ((a)[0]*(b)[0] + (a)[1]*(b)[1] + (a)[2]*(b)[2] + a[3])
#define NEGATEPLANE(a)		\
{				\
    (a)[0] = -(a)[0];		\
    (a)[1] = -(a)[1];		\
    (a)[2] = -(a)[2];		\
    (a)[3] = -(a)[3];		\
}

static int
Tetras_Weights(InstanceVars I, POINT_TYPE *pt)
{
    Irreg_InstanceVars iI = (Irreg_InstanceVars)I;
    Irreg_VectorGrp    iP = (Irreg_VectorGrp)(iI->i.currentVectorGrp);
    int   e, t;
    Irreg_VectorPart ip;
    float vPp[3], vPs[3];
    float V, nV;
    float *p, *s;
    float pbuf[3], sbuf[3];
    float *xqsr = iI->planes +  0;
    float *xrps = iI->planes +  4;
    float *xspq = iI->planes +  8;
    float *xqpr = iI->planes + 12;
    float *w    = iI->w;
    int   i, *elt, *tet;
    int   ebuf[8];

    if (IsCurrentDegenerate(iI))
	return 0;

    ip = (Irreg_VectorPart)iP->P.p[iI->cp];

    /*np = iI->cp;*/
    e  = iI->ce;
    t  = iI->ct;

    if (ip->invElements && DXIsElementInvalid(ip->invElements, e))
	return 0;

    elt = (int *)DXGetArrayEntry(iI->cHandle, e, (Pointer)ebuf);

    tet = iP->primTable + 4*t;

    p = (float*)DXGetArrayEntry(iI->pHandle,elt[tet[0]],(Pointer)pbuf);
    /*q = (float*)DXGetArrayEntry(iI->pHandle,elt[tet[1]],(Pointer)qbuf);*/
    /*r = (float*)DXGetArrayEntry(iI->pHandle,elt[tet[2]],(Pointer)rbuf);*/
    s = (float*)DXGetArrayEntry(iI->pHandle,elt[tet[3]],(Pointer)sbuf);

    /*
     * DXCross products of first three faces have apexes at p, last has
     * apex at s.
     */
    vPp[0] = pt[0] - p[0]; vPp[1] = pt[1] - p[1]; vPp[2] = pt[2] - p[2];
    vPs[0] = pt[0] - s[0]; vPs[1] = pt[1] - s[1]; vPs[2] = pt[2] - s[2];

    w[0] =  vPs[0]*xqsr[0] + vPs[1]*xqsr[1] + vPs[2]*xqsr[2];
    w[1] =  vPp[0]*xrps[0] + vPp[1]*xrps[1] + vPp[2]*xrps[2];
    w[2] =  vPp[0]*xspq[0] + vPp[1]*xspq[1] + vPp[2]*xspq[2];
    w[3] =  vPp[0]*xqpr[0] + vPp[1]*xqpr[1] + vPp[2]*xqpr[2];

    V = w[0] + w[1] + w[2] + w[3];

    /*
     * If volume is zero, element was degenerate.  Otherwise,
     * check for a boundary condition: total negative volume must
     * be no more than 0.0001 of total volume for the point to be 
     * considered inside.
     */
    if (V == 0)
	return 0;
    else if (V > 0)
    {
	nV = 0;
	for (i = 0; i < 4; i++)
	    if (w[i] < 0)
		nV += w[i];
    }
    else
    {
	nV = 0;
	for (i = 0; i < 4; i++)
	    if (w[i] > 0)
		nV += w[i];
    }

    V = 1.0 / V;

    for (i = 0; i < 4; i++)
	w[i] *= V;

    if (nV != 0.0)
	return 0;

    return 1;
}

static int
Tetras_FaceWeights(InstanceVars I, POINT_TYPE *pt)
{
    Irreg_InstanceVars iI = (Irreg_InstanceVars)I;
    Irreg_VectorGrp    iP = (Irreg_VectorGrp)(iI->i.currentVectorGrp);
    int                np, e, t;
    Irreg_VectorPart   ip;
    float              w[3];
    int                i, j, *elt, *tet;
    int                ebuf[8];
    float              V, *pts[3], pbuf[3][3];
    Vector             vecs[3], cross;
    float	       fpt[3];
    int		       npos;

    fpt[0] = pt[0];
    fpt[1] = pt[1];
    fpt[2] = pt[2];

    np = iI->cp;
    e  = iI->ce;
    t  = iI->ct;

    ip = (Irreg_VectorPart)iP->P.p[np];

    if (ip->invElements && DXIsElementInvalid(ip->invElements, e))
	return 0;

    elt = (int *)DXGetArrayEntry(iI->cHandle, e, (Pointer)ebuf);

    tet = iP->primTable + 4*t;

    for (i = j = 0; i < 4; i++)
    {
	if (i != iI->face)
	{
	    pts[j] = (float *)DXGetArrayEntry(iI->pHandle,
					    elt[tet[j]], (Pointer)pbuf[j]);
	    j++;
	}
    }

    _dxfvector_subtract_3D((Vector *)pts[0], (Vector *)fpt, &vecs[0]);
    _dxfvector_subtract_3D((Vector *)pts[1], (Vector *)fpt, &vecs[1]);
    _dxfvector_subtract_3D((Vector *)pts[2], (Vector *)fpt, &vecs[2]);

    _dxfvector_cross_3D(&vecs[1], &vecs[2], &cross);
    w[0] = _dxfvector_length_3D(&cross);

    w[1] = _dxfvector_cross_3D(&vecs[2], &vecs[0], &cross);
    w[1] = _dxfvector_length_3D(&cross);

    w[2] = _dxfvector_cross_3D(&vecs[0], &vecs[1], &cross);
    w[2] = _dxfvector_length_3D(&cross);

    V = w[0] + w[1] + w[2];

    V = 1.0 / V;

    for (i = j = npos = 0; i < 4; i++)
    {
	if (i == iI->face)
	    iI->w[i] = 0.0;
	else
	    iI->w[i] = w[j++] * V;

	if (iI->w[i] >= 0.0)
	    npos ++;
    }

    return npos == 0 || npos == 4;
}

static int
Tetras_InitElement(Irreg_InstanceVars iI)
{
    Irreg_VectorGrp iP = (Irreg_VectorGrp)(iI->i.currentVectorGrp);
    Irreg_VectorPart ip;
    float *p, *q, *r, *s, *v;
    float pbuf[3], qbuf[3], rbuf[3], sbuf[3];
    float *vqp  = iI->edges +   0;
    float *vrp  = iI->edges +   3;
    float *vsp  = iI->edges +   6;
    float *vqs  = iI->edges +   9;
    float *vrs  = iI->edges +  12;
    float *vrq  = iI->edges +  15;
    float *xqsr = iI->planes +  0;
    float *xrps = iI->planes +  4;
    float *xspq = iI->planes +  8;
    float *xqpr = iI->planes + 12;
    int   i,  *elt, ebuf[8];
    int   np, e, t, *tet;

    np = iI->cp;
    e  = iI->ce;
    t  = iI->ct;

    ip = (Irreg_VectorPart)iP->P.p[np];

    elt = (int *)DXGetArrayEntry(iI->cHandle, e, (Pointer)ebuf);

    tet = iP->primTable + 4*t;

    SetElementVisited(ip, iI->ce);

    p = (float*)DXGetArrayEntry(iI->pHandle,elt[tet[0]],(Pointer)pbuf);
    q = (float*)DXGetArrayEntry(iI->pHandle,elt[tet[1]],(Pointer)qbuf);
    r = (float*)DXGetArrayEntry(iI->pHandle,elt[tet[2]],(Pointer)rbuf);
    s = (float*)DXGetArrayEntry(iI->pHandle,elt[tet[3]],(Pointer)sbuf);

    /*
     * edge vectors
     */
    vqp[0] = q[0] - p[0]; vqp[1] = q[1] - p[1]; vqp[2] = q[2] - p[2];
    vrp[0] = r[0] - p[0]; vrp[1] = r[1] - p[1]; vrp[2] = r[2] - p[2];
    vsp[0] = s[0] - p[0]; vsp[1] = s[1] - p[1]; vsp[2] = s[2] - p[2];
    vqs[0] = q[0] - s[0]; vqs[1] = q[1] - s[1]; vqs[2] = q[2] - s[2];
    vrs[0] = r[0] - s[0]; vrs[1] = r[1] - s[1]; vrs[2] = r[2] - s[2];
    vrq[0] = r[0] - q[0]; vrq[1] = r[1] - q[1]; vrq[2] = r[2] - q[2];
    
    /*
     * DXCross product terms for the four faces based on vertices p and s
     */
    xqsr[0] = vrs[1]*vqs[2] - vrs[2]*vqs[1];
    xqsr[1] = vrs[2]*vqs[0] - vrs[0]*vqs[2];
    xqsr[2] = vrs[0]*vqs[1] - vrs[1]*vqs[0];
    CROSS2PLANE(xqsr, s);

    xrps[0] = vrp[1]*vsp[2] - vrp[2]*vsp[1];
    xrps[1] = vrp[2]*vsp[0] - vrp[0]*vsp[2];
    xrps[2] = vrp[0]*vsp[1] - vrp[1]*vsp[0];
    CROSS2PLANE(xrps, p);

    xspq[0] = vsp[1]*vqp[2] - vsp[2]*vqp[1];
    xspq[1] = vsp[2]*vqp[0] - vsp[0]*vqp[2];
    xspq[2] = vsp[0]*vqp[1] - vsp[1]*vqp[0];
    CROSS2PLANE(xspq, p);

    xqpr[0] = vqp[1]*vrp[2] - vqp[2]*vrp[1];
    xqpr[1] = vqp[2]*vrp[0] - vqp[0]*vrp[2];
    xqpr[2] = vqp[0]*vrp[1] - vqp[1]*vrp[0];
    CROSS2PLANE(xqpr, p);

    /*
     * normalize the edge vectors 
     */
    ResetCurrentDegenerate(iI);
    for (i = 0, v = iI->edges; i < 6; i++, v += 3)
    {
	float d = v[0]*v[0] + v[1]*v[1] + v[2]*v[2];

	if (d == 0.0)
	{
	    SetCurrentDegenerate(iI);
	    v[0] = 0.0;
	    v[1] = 0.0;
	    v[2] = 0.0;
	}
	else
	{
	    d = 1.0 / d;
	    v[0] *= d;
	    v[1] *= d;
	    v[2] *= d;
	}
    }

    /*
     * Note that the streamline will be found in some non-degenerate element
     * before any degenerate elements have to be handled.
     */
    if (! IsParityKnown(iI) && ! IsCurrentDegenerate(iI))
    {
	if (POINTPLANE(xqsr, p) > 0.0)
	    SetFaceNReversed(iI, 0);
	if (POINTPLANE(xrps, q) > 0.0)
	    SetFaceNReversed(iI, 1);
	if (POINTPLANE(xspq, r) > 0.0)
	    SetFaceNReversed(iI, 2);
	if (POINTPLANE(xqpr, s) > 0.0)
	    SetFaceNReversed(iI, 3);
	
	SetParityKnown(iI);
    }

    if (IsParityKnown(iI))
    {
        int i;
	float *p = iI->planes;
	
	for (i = 0; i < 4; i++, p += 4)
	    if (IsFaceNReversed(iI, i))
		NEGATEPLANE(p);
    }

    return 1;
}

static int
Tris_Weights(InstanceVars I, POINT_TYPE *pt)
{
    Irreg_InstanceVars iI = (Irreg_InstanceVars)I;
    Irreg_VectorGrp iP = (Irreg_VectorGrp)(iI->i.currentVectorGrp);
    int   e, t;
    float *p, *q, *r;
    float pbuf[3], qbuf[3], rbuf[3];
    float x, y, x0, y0, x1, y1, x2, y2;
    float A, aInv, nA;
    float *w    = iI->w;
    int   i, *elt, *tri, ebuf[8];

    if (IsCurrentDegenerate(iI))
	return 0;

    /*np = iI->cp;*/
    e  = iI->ce;
    t  = iI->ct;

    /*ip = (Irreg_VectorPart)iP->P.p[np];*/

    elt = (int *)DXGetArrayEntry(iI->cHandle, e, (Pointer)ebuf);

    tri = iP->primTable + 3*t;

    p = (float*)DXGetArrayEntry(iI->pHandle,elt[tri[0]],(Pointer)pbuf);
    q = (float*)DXGetArrayEntry(iI->pHandle,elt[tri[1]],(Pointer)qbuf);
    r = (float*)DXGetArrayEntry(iI->pHandle,elt[tri[2]],(Pointer)rbuf);

    x = pt[0]; y = pt[1];

    x0 = p[0]; y0 = p[1];
    x1 = q[0]; y1 = q[1];
    x2 = r[0]; y2 = r[1];

    A  = (x0*y1 + x1*y2 + x2*y0 - y0*x1 - y1*x2 - y2*x0) * 0.5;
    if (A == 0)
	return 0;
    
    aInv = 1.0 / A;

    w[0] = 0.5 * (x *y1 + x1*y2 + x2*y  - y *x1 - y1*x2 - y2*x ) * aInv;
    w[1] = 0.5 * (x0*y  + x *y2 + x2*y0 - y0*x  - y *x2 - y2*x0) * aInv;
    w[2] = 0.5 * (x0*y1 + x1*y  + x *y0 - y0*x1 - y1*x  - y *x0) * aInv;

    nA = 0;
    for (i = 0; i < 3; i++)
	if (w[i] < 0)
	    nA -= w[i];

    if (nA > 0.0001)
	return 0;

    return 1;
}

static int
Tris_FaceWeights(InstanceVars I, POINT_TYPE *pt)
{
    Irreg_InstanceVars iI = (Irreg_InstanceVars)I;
    Irreg_VectorGrp iP = (Irreg_VectorGrp)(iI->i.currentVectorGrp);
    int   e, t;
    float w[2];
    int   npos, i, j, *elt, *tri, ebuf[8];
    float *pts[3], pbuf[3][2];

    /*np = iI->cp;*/
    e  = iI->ce;
    t  = iI->ct;

    /*ip = (Irreg_VectorPart)iP->P.p[np];*/

    elt = (int *)DXGetArrayEntry(iI->cHandle, e, (Pointer)ebuf);

    tri = iP->primTable + 3*t;

    for (i = j = 0; i < 3; i++)
	if (i != iI->face)
	{
	    pts[j] = (float *)DXGetArrayEntry(iI->pHandle,
					    elt[tri[i]], (Pointer)pbuf[j]);
	    j++;
	}
    
    if (pts[0][0] != pts[1][0])
    {
	w[1] = (pt[0] - pts[0][0]) / (pts[1][0] - pts[0][0]);
	w[0] = 1.0 - w[1];
    }
    else if (pts[0][1] != pts[1][1])
    {
	w[1] = (pt[1] - pts[0][1]) / (pts[1][1] - pts[0][1]);
	w[0] = 1.0 - w[1];
    }
    else
    {
	DXSetError(ERROR_INTERNAL, "zero length edge");
	return 0;
    }

    for (i = j = npos = 0; i < 3; i++)
    {
	if (i == iI->face)
	    iI->w[i] = 0.0;
	else
	    iI->w[i] = w[j++];
	
	if (iI->w[i] >= 0.0)
	    npos ++;
    }

    return npos == 0 || npos == 3;
}

#define POINTLINE(a,b) ((a)[0]*(b)[0] + (a)[1]*(b)[1] + (a)[2])
#define NEGATELINE(a)		\
{				\
    (a)[0] = -(a)[0];		\
    (a)[1] = -(a)[1];		\
    (a)[2] = -(a)[2];		\
}

static int
Tris_InitElement(Irreg_InstanceVars iI)
{
    Irreg_VectorGrp iP = (Irreg_VectorGrp)(iI->i.currentVectorGrp);
    Irreg_VectorPart ip;
    float *p, *q, *r, *v;
    float pbuf[3], qbuf[3], rbuf[3];
    float *vqp = iI->edges +   0;
    float *vrq = iI->edges +   3;
    float *vpr = iI->edges +   6;
    float *xrq = iI->planes +  0;
    float *xpr = iI->planes +  4;
    float *xqp = iI->planes +  8;
    int   i,  *elt, ebuf[8];
    int   np, e, t, *tri;

    np = iI->cp;
    e  = iI->ce;
    t  = iI->ct;

    ip = (Irreg_VectorPart)iP->P.p[np];

    elt = (int *)DXGetArrayEntry(iI->cHandle, e, (Pointer)ebuf);

    tri = iP->primTable + 3*t;

    SetElementVisited(ip, iI->ce);

    p = (float*)DXGetArrayEntry(iI->pHandle,elt[tri[0]],(Pointer)pbuf);
    q = (float*)DXGetArrayEntry(iI->pHandle,elt[tri[1]],(Pointer)qbuf);
    r = (float*)DXGetArrayEntry(iI->pHandle,elt[tri[2]],(Pointer)rbuf);

    /*
     * edge vectors
     */
    vqp[0] = q[0] - p[0]; vqp[1] = q[1] - p[1];
    vrq[0] = r[0] - q[0]; vrq[1] = r[1] - q[1];
    vpr[0] = p[0] - r[0]; vpr[1] = p[1] - r[1];
    
    xqp[0] =  vqp[1];
    xqp[1] = -vqp[0];
    xqp[2] = -(xqp[0]*p[0] + xqp[1]*p[1]);

    xrq[0] =  vrq[1];
    xrq[1] = -vrq[0];
    xrq[2] = -(xrq[0]*q[0] + xrq[1]*q[1]);

    xpr[0] =  vpr[1];
    xpr[1] = -vpr[0];
    xpr[2] = -(xpr[0]*r[0] + xpr[1]*r[1]);

    /*
     * normalize the edge vectors 
     */
    ResetCurrentDegenerate(iI);
    for (i = 0, v = iI->edges; i < 3; i++, v += 3)
    {
	float d = v[0]*v[0] + v[1]*v[1];

	if (d == 0.0)
	{
	    SetCurrentDegenerate(iI);
	    v[0] = 0.0;
	    v[1] = 0.0;
	}
	else
	{
	    d = 1.0 / d;
	    v[0] *= d;
	    v[1] *= d;
	}
    }

    /*
     * Note that the streamline will be found in some non-degenerate element
     * before any degenerate elements have to be handled.
     */
    if (! IsParityKnown(iI) && ! IsCurrentDegenerate(iI))
    {
	if (POINTLINE(xqp, r) > 0.0)
	    SetFaceNReversed(iI, 0);
	if (POINTLINE(xrq, p) > 0.0)
	    SetFaceNReversed(iI, 1);
	if (POINTLINE(xpr, q) > 0.0)
	    SetFaceNReversed(iI, 2);
	
	SetParityKnown(iI);
    }

    if (IsParityKnown(iI))
    {
        int i;
	float *p = iI->planes;
	
	for (i = 0; i < 3; i++, p += 4)
	    if (IsFaceNReversed(iI, i))
		NEGATELINE(p);
    }

    return 1;
}

static Error
Irreg_FindBoundary(InstanceVars I, POINT_TYPE *p, VECTOR_TYPE *v, double *t)
{
    Irreg_InstanceVars iI = (Irreg_InstanceVars)I;
    Irreg_VectorGrp    iP = (Irreg_VectorGrp)(iI->i.currentVectorGrp);
    double tmin, tface;
    int i, j, imin;
    float *plane;
    double d0, d1, dmax;

    plane = iI->planes; imin = -1; tmin = DXD_MAX_FLOAT; dmax = 0.0;
    for (i = 0; i < iP->P.nDim+1; i++, plane += 4)
    {
	d0 = d1 = 0.0;

	for (j = 0; j < iP->P.nDim; j++)
	{
	   d0 += (double)plane[j]*v[j];
	   d1 += (double)plane[j]*p[j];
	}

	d1 += plane[iP->P.nDim];

	if (d0 <= 0.0)
	    continue;

	if (d1 > 0.0)
	    tface = 0;
	else
	    tface = -d1 / d0;

	if (tface >= 0)
	    if ((tface == tmin && d0 > dmax) || tface < tmin)
	    {
		dmax = d0;
		tmin = tface;
		imin = i;
	    }
    }


    if (imin == -1)
    {
	DXSetError(ERROR_INTERNAL, "can't find intersection");
	return ERROR;
    }

    iI->face = imin;
    *t = tmin;

    return OK;
}

static int
Irreg_Interpolate(InstanceVars I, POINT_TYPE *pt, VECTOR_TYPE *vec)
{
    Irreg_VectorPart   ip;
    Irreg_VectorGrp    iP;
    Irreg_InstanceVars iI;
    int		        *elt, i, j;
    float	        *vdata;
    int			*prim;
    int			ebuf[8];

    iI = (Irreg_InstanceVars)I;
    iP = (Irreg_VectorGrp)(iI->i.currentVectorGrp);

    ip = (Irreg_VectorPart)iP->P.p[iI->cp];

    if (ip->constantData)
    {
        for (i = 0; i < iP->P.nDim; i++)
	    vec[i] = (VECTOR_TYPE)ip->vectors[i];
	return OK;
    }

    if (ip->p.dependency == DEP_ON_POSITIONS)
    {
	prim = iP->primTable + (iP->P.nDim+1)*iI->ct;

	elt = (int*)DXGetArrayEntry(iI->cHandle,iI->ce,(Pointer)ebuf);

	vec[0] = 0.0; vec[1] = 0.0; vec[2] = 0.0;

	for (i = 0; i < iP->P.nDim+1; i++)
	{
	    vdata = ip->vectors + iP->P.nDim*elt[prim[i]];
	    for (j = 0; j < iP->P.nDim; j++)
		vec[j] += iI->w[i]*vdata[j];
	}
    }
    else
    {
	vdata = ip->vectors + iP->P.nDim*iI->ce;
	for (j = 0; j < iP->P.nDim; j++)
	    vec[j] = vdata[j];
    }

    return OK;
}

static Error
Irreg_StepTime(InstanceVars I, double c, VECTOR_TYPE *vec, double *t)
{
    Irreg_InstanceVars iI = (Irreg_InstanceVars)I;
    Irreg_VectorGrp    iP = (Irreg_VectorGrp)(iI->i.currentVectorGrp);
    int i, j, nEdges;
    double tmin, iprime;
    float *edge;

    /*
     * We are given c, a limit on |t*Vx|/|X| where t is the unknown time step,
     * Vx is the projection of V on edge X, and |X| is the length of X.
     * For all edges X, find the smallest t such that 
     *
     *  c > |t*Vx| / |X| (= t|Vx|/|X|)
     * 
     * For each edge X, we solve for t': the maximum allowable length along
     * the edge X:
     *
     *  t' = c|X|/|Vx|
     *
     * To get Vx,  the projection of the vector along the double, we
     * scale the vector by the cosine of the angle separating them:
     *
     *  |Vx| = |V| cos theta 
     *
     * where theta is the angle between the edge and the vector.  We also
     * have that:
     *
     *  cos theta = (V dot X) / |X| |V|
     *
     * We therefore get:
     *
     *  |Vx| = (V dot X) / |X|
     *
     * which we plug back into the above:
     *
     *  t' = c|X|/|Vx| = c|X|/((V dot X)/|X|) = c|X|^2 / (V dot X)
     *
     * But since our edge vectors are normalized, 
     * 
     *     = c / (V dot X)
     */

    if (IsCurrentDegenerate(iI))
    {
	*t = 0.0;
	return OK;
    }

    if (iP->P.nDim == 2)
	nEdges = 3;
    else 
	nEdges = 6;

    tmin = DXD_MAX_FLOAT;
    edge = iI->edges;
    for (i = 0; i < nEdges; i++, edge += 3)
    {
	float dot;

	dot = 0;
	for (j = 0; j < iP->P.nDim; j++)
	    dot += vec[j]*edge[j];

	if (dot == 0.0)
	    continue;
	
	if (dot < 0.0)
	    dot = -dot;
	
	iprime = c / dot;

	if (iprime < tmin)
	    tmin = iprime;
    }

    *t = tmin;

    return OK;
}
	
static int
_Irreg_Walk(Irreg_InstanceVars iI, Irreg_VectorGrp iP,
	int *nbrs, /* The element neighbors of elt iI->ce	*/
	int NPerP, /* The number of nbrs of a primitive	*/
	int nDim,  /* The dimensionality of the space */
	POINT_TYPE *start, VECTOR_TYPE *vector, POINT_TYPE *target)
{
    int      nbuf[6];
    int      i, j;
    float    *plane;
    int      ne;
    NbrTable *nt;
    /*
     * Check the current element/primitive.  If its in, we're done.
     */
    if ((*iP->P.Weights)((InstanceVars)iI, target))
        return WALK_FOUND;

#if 0
    plane = iI->planes;
    for (i = 0; i < NPerP; i++)
    {
	float l = 0.0;
	for (j = 0; j < NPerP-1; j++)
	    l += (plane[j]*plane[j]);
    
	if (l == 0)
	    continue;

	l = 1.0 / sqrt(l);

	for (j = 0; j < NPerP; j++)
	    plane[j] *= l;

	plane += NPerP;
    }
#endif

    /*
     * Find the exit point for the vector.  To do so, intersect
     * the vector with the plane of each face, then test the
     * intersection point against each other plane.
     */
    plane = iI->planes;
    for (i = 0; i < NPerP; i++)
    {
        float VDotP = 0.0; /* vector dot plane		      */
	float SDotP = 0.0; /* start dot plane                 */
	float t;           /* intersection dist along vector  */
	POINT_TYPE xit[3]; /* intersection point              */

	VDotP = 0.0;
	for (j = 0; j < nDim; j++)
	{
	    VDotP += (vector[j] * *plane);
	    SDotP += (start[j] * *plane);
	    plane ++;
	}
	SDotP += *plane;
	plane++;

	/*
	 * if VDotP <= 0.0 then the vector points away
	 * from the element face
	 */
	if (VDotP <= 0)
	    continue;

	t = -SDotP / VDotP;
	if (t > 1.0)
	    continue;

	for (j = 0; j < nDim; j++)
	    xit[j] = start[j] + t*vector[j];

	/*
	 * See if the intersection point (xit) actually lies in the
	 * face.
	 */
	iI->face = i;
	if (! (*iP->P.FaceWeights)((InstanceVars)iI, xit))
	    continue;

	if (j == NPerP)
	    return WALK_ERROR;

	nt = iP->nbrTable + NPerP*iI->ct + iI->face;

	/*
	 * If there is no neighbor, then we've found the exit.  This
	 * is the case if the primitive face corresponds to an element
	 * face (nt->nelt != -1) and there's no corresponding  element
	 * neighbor.
	 *
	 * ne is the next element.  Its either the same as this one,
	 * if this is an internal face, or the neighbor if there
	 * is one.  If this face is external to the partition, then 
	 * we'll be returning before its used. 
	 */
	ne = iI->ce; 
	if (nt->nelt != -1 && (ne = nbrs[nt->nelt]) == -1)
	{
	    for (j = 0; j < nDim; j++)
	        target[j] = xit[j];
	    return WALK_EXIT;
	}

	iI->ct = nt->nprim;
	if (ne != iI->ce)
	{
	    iI->ce = ne;
	    nbrs = GET_NEIGHBORS(iI->nHandle, iI->ce, nbuf);
	}

	(*iP->InitElement)(iI);

	if (IsCurrentDegenerate(iI))
	{
	    for (j = 0; j < nDim; j++)
	        target[j] = xit[j];
	    return WALK_EXIT;
	}

	return _Irreg_Walk(iI, iP, nbrs, NPerP, nDim, start, vector, target);
    }

    /*
     * If we fall out, then we didn't find the exit face.
     */
    return WALK_ERROR;
}
    
static int
Irreg_Walk(InstanceVars I, 
	POINT_TYPE *start, VECTOR_TYPE *vector, POINT_TYPE *target)
{
    Irreg_InstanceVars iI = (Irreg_InstanceVars)I;
    Irreg_VectorGrp    iP = (Irreg_VectorGrp)(iI->i.currentVectorGrp);
    int nbuf[6], *nbrs;

    nbrs = GET_NEIGHBORS(iI->nHandle, iI->ce, nbuf);

    return _Irreg_Walk(iI, iP, nbrs,
    	iP->nbrsPerPrim, iP->nDimensions, start, vector, target);
}

static int
Irreg_Neighbor(InstanceVars I, VECTOR_TYPE *v)
{
    Irreg_InstanceVars iI = (Irreg_InstanceVars)I;
    Irreg_VectorGrp    iP = (Irreg_VectorGrp)(iI->i.currentVectorGrp);
    Irreg_VectorPart  ip;
    int nbr, nindex, *nbrs, nBuf[6];
    float *plane, dot;
    int   i;

    plane = iI->planes + 4*iI->face;

    dot = 0;
    for (i = 0; i < iP->P.nDim; i++)
	dot += *v++ * *plane++;
    if (dot < 0)
	return 1;

    ip = (Irreg_VectorPart)iP->P.p[iI->cp];

    nindex = iP->nbrTable[(iP->P.nDim+1)*iI->ct + iI->face].nelt;
    iI->ct = iP->nbrTable[(iP->P.nDim+1)*iI->ct + iI->face].nprim;

    if (nindex == -1)
    {
	(*iP->InitElement)(iI);
	return 1;
    }

    nbrs = GET_NEIGHBORS(iI->nHandle, iI->ce, nBuf);
    nbr = nbrs[nindex];

    if (nbr < 0)
    {
	nbr = -nbr;

	if (ip->partitionNeighbors)
	{
	    Irreg_VectorPart np;

	    if (ip->gridConnections)
	    {
		/*
		 * if grid connections, then the partition neighbors 
		 * gives us a pointer to the neighboring partition.
		 * We figure out the offset from that partitions grid
		 * strides and the munged indices of the current element.
		 */
		int indices[10], axis, npartition;

		npartition = ip->partitionNeighbors[nindex];
		if (npartition == -1)
		    return 0;

		/*
		 * OK.  there's a neighboring partition.  Determine the
		 * indices of the current element in the current partition
		 * and from that determine the indicies in the neighbor.
		 */
		iI->cp = npartition;
		
		np = (Irreg_VectorPart)iP->P.p[npartition];
		if (np == NULL)
		    return 0;

		_dxfGetIndices(iI->ce, ip->nd, ip->gstrides, indices);

		axis  = nindex >> 1;

		if (nindex & 0x1)
		{
		    /*
		     * Then the current element is at the top end of the 
		     * partition's range, so it'll be at the bottom end
		     * of the neighbor's.
		     */
		    indices[axis] = 0;
		}
		else
		{
		    /*
		     * Then the current element is at the bottom end of
		     * the partition's range, so it'll be at the top of the
		     * neighbor's.
		     */
		    indices[axis] = np->gcounts[axis]-1;
		}

		iI->ce = _dxfGetOffset(ip->nd, np->gstrides, indices);
	    }
	    else
	    {
		if (ip->partitionNeighbors[nbr<<1] == -1)
		    return 0;

		iI->cp = ip->partitionNeighbors[nbr<<1];
		iI->ce = ip->partitionNeighbors[(nbr<<1)+1];
		np = (Irreg_VectorPart)iP->P.p[iI->cp];
	    }


	    if (iI->pHandle)
		DXFreeArrayHandle(iI->pHandle);
	    iI->pHandle = DXCreateArrayHandle(np->pArray);

	    if (iI->cHandle)
		DXFreeArrayHandle(iI->cHandle);
	    iI->cHandle = DXCreateArrayHandle(np->cArray);

	    if (iI->nHandle)
		DXFreeNeighborsHandle(iI->nHandle);
	    iI->nHandle = DXCreateNeighborsHandle(np->p.field);

	    if (!iI->pHandle || ! iI->nHandle || ! iI->cHandle)
		return -1;

	    (*iP->InitElement)(iI);
	    return 1;
	}
	else
	    return 0;
    }
    else
    {
	if (ip->invElements && DXIsElementInvalid(ip->invElements, nbr))
	    return 0;

	iI->ce = nbr;
	(*iP->InitElement)(iI);
	return 1;
    }
}

typedef struct
{ 
    Irreg_VectorGrp  grp;
    Irreg_VectorPart part;
    CompositeField   group;
} CurlMap_Args;
    
static Error
Irreg_CurlMap(VectorGrp P, MultiGrid mg)
{
    Irreg_VectorGrp  iP = (Irreg_VectorGrp)P;
    Object           curl  = NULL;
    int		     i;
    CurlMap_Args     args;
    CompositeField   cf = NULL;
    Field	     cfield = NULL;

    if (iP->P.n == 1)
    {
	if (iP->P.p[0]->dependency == DEP_ON_CONNECTIONS)
	{
	    DXSetError(ERROR_DATA_INVALID,
	      "cannot compute curl when data is dependent on connections");
	    goto error;
	}

	cfield = NULL;

	if (!  Irreg_CurlMap_Field(iP, (Irreg_VectorPart)iP->P.p[0], NULL, &cfield))
	    goto error;
	
	if (cfield)
	{
	    if (! _dxfDivCurl((Object)cfield, NULL, &curl))
		goto error;
	
	    if (! curl)
		goto error;
	}
	
	DXDelete((Object)cfield);
	cfield = NULL;
    }
    else
    {
	if (! DXCreateTaskGroup())
	    goto error;
	
	cf = args.group = DXNewCompositeField();
	if (! args.group)
	    goto error;

	args.grp   = iP;

	for (i = 0; i < iP->P.n; i++)
	{
	    args.part = (Irreg_VectorPart)iP->P.p[i];

	    if (args.part->p.dependency == DEP_ON_CONNECTIONS)
	    {
		DXSetError(ERROR_DATA_INVALID,
		  "cannot compute curl when data is dependent on connections");
		DXAbortTaskGroup();
		goto error;
	    }

	    if (! DXAddTask(Irreg_CurlMap_Task,
				(Pointer)&args, sizeof(args), 1.0))
	    {
		DXAbortTaskGroup();
		goto error;
	    }
	}
	
	if (! DXExecuteTaskGroup() || DXGetError() != ERROR_NONE)
	    goto error;

	DXGetMemberCount((Group)cf, &i);

	if (i != 0)
	{
	    if (! _dxfDivCurl((Object)cf, NULL, &curl))
		goto error;
	    
	    if  (! curl)
		goto error;
	}
	    
	DXDelete((Object)cf);
	cf = NULL;
    }

    if (curl)
	if (! DXSetMember((Group)mg, NULL, curl))
	    goto error;

    return OK;

error:

    DXDelete((Object)cf);
    DXDelete((Object)curl);
    DXDelete((Object)cfield);
    return ERROR;
}

static Error
Irreg_CurlMap_Task(Pointer ptr)
{
    CurlMap_Args *args = (CurlMap_Args *)ptr;
    return Irreg_CurlMap_Field(args->grp, args->part, (Group)args->group, NULL);
}

static Error
Irreg_CurlMap_Field(Irreg_VectorGrp iP, Irreg_VectorPart ip, Group g, Field *f)
{
    Field newf = NULL;
    int *elt;
    int i, j;
    int ebuf[8];
    InvalidComponentHandle activePositions = NULL;
    InvalidComponentHandle activeConnections = NULL;
    ArrayHandle handle = NULL;

    handle = DXCreateArrayHandle(ip->cArray);
    if (! handle)
	goto error;
    
    newf = (Field)DXCopy((Object)(ip->p.field), COPY_STRUCTURE);
    if (! newf)
	goto error;

    activePositions = DXCreateInvalidComponentHandle((Object)newf,
							NULL, "positions");
    if (! activePositions)
	goto error;
    
    DXSetAllInvalid(activePositions);

    activeConnections = DXCreateInvalidComponentHandle((Object)newf,
							NULL, "connections");
    if (! activeConnections)
	goto error;

    DXSetAllInvalid(activeConnections);

    /*
     * Mark the vertices of the alive elements alive
     */
    for (i = 0; i < ip->nElements; i++)
    {
	if (IsElementVisited(ip, i))
	{
	    elt = (int *)DXGetArrayEntry(handle, i, (Pointer)ebuf);
	
	    for (j = 0; j < iP->vrtsPerElement; j++)
		DXSetElementValid(activePositions, elt[j]);
	    
	    DXSetElementValid(activeConnections, i);
	}
    }

    
    /*
     * For each dead element, if it contains an alive vertex,
     * resurrect it UNLESS it was dead to begin with
     */
    for (i = 0; i < ip->nElements; i++)
    {
	/*
	 * If element i is already active, continue
	 */
	if (IsElementVisited(ip, i))
	    continue;

	/*
	 * If element i is not currently active but not invalid, then
	 * activate it if it has at least one active position
	 */
	if (!ip->invElements || DXIsElementValid(ip->invElements, i))
	{
	    elt = (int *)DXGetArrayEntry(handle, i, (Pointer)ebuf);

	    for (j = 0; j < iP->vrtsPerElement; j++)
		if (DXIsElementValid(activePositions, elt[j]))
		{
		    DXSetElementValid(activeConnections, i);
		    break;
		}
	}
    }

    /*
     * Now go back through, turning on all the vertices of all
     * the elements that touch a vertex that an initially alive
     * element touches.
     */
    for (i = 0; i < ip->nElements; i++)
    {
	if (DXIsElementValid(activeConnections, i))
	{
	    elt = (int *)DXGetArrayEntry(handle, i, (Pointer)ebuf);
	    for (j = 0; j < iP->vrtsPerElement; j++)
		DXSetElementValid(activePositions, elt[j]);
	}
    }

    DXFreeArrayHandle(handle);
    handle = NULL;
    
    if (! DXSaveInvalidComponent(newf, activeConnections))
	goto error;
    
    if (! DXSaveInvalidComponent(newf, activePositions))
	goto error;
    
    if (DXGetComponentValue(newf, "neighbors"))
	DXDeleteComponent(newf, "neighbors");

    if (! DXEndField(newf))
	goto error;

    if (! DXCull((Object)newf))
	goto error;
    
    if (! DXEmptyField(newf))
    {
	if (g)
	{
	    if (! DXSetMember((Group)g, NULL, (Object)newf))
		goto error;
	}
	else if (f)
	{
	    *f = newf;
	}
	else
	{
	    DXSetError(ERROR_INTERNAL, "no place to return the curl field");
	    DXDelete((Object)newf);
	    return ERROR;
	}
    }
    else 
	DXDelete((Object)newf);
    
    if (activePositions)
	DXFreeInvalidComponentHandle(activePositions);

    if (activeConnections)
	DXFreeInvalidComponentHandle(activeConnections);

    return OK;

error:
    if (activePositions)
	DXFreeInvalidComponentHandle(activePositions);
    if (activeConnections)
	DXFreeInvalidComponentHandle(activeConnections);
    DXFreeArrayHandle(handle);
    DXDelete((Object)newf);

    return ERROR;
}

static int
Irreg_Ghost(InstanceVars I, POINT_TYPE *p)
{
    Irreg_InstanceVars iI = (Irreg_InstanceVars)I;
    Irreg_VectorGrp    iP = (Irreg_VectorGrp)(iI->i.currentVectorGrp);
    Irreg_VectorPart   vp = (Irreg_VectorPart)iP->P.p[iI->cp];
    return vp->p.ghosts[iI->ce];
}

