/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>

#if defined(HAVE_STRING_H)
#include <string.h>
#endif

#include <dx/dx.h>
#include <math.h>
#include "stream.h"
#include "_divcurl.h"
#include "_partnbrs.h"

/*
 ***********************************
 * Reg interface
 ***********************************
 */

typedef struct reg_InstanceVars
{
    struct instanceVars i;
    int 	        indices[3];
    VECTOR_TYPE	        ivector[3];
    double	        weights[9];
    int		        face;
    int		        cp;
    int			ce;
} *Reg_InstanceVars;

typedef struct reg_VectorGrp  *Reg_VectorGrp;
typedef struct reg_VectorPart *Reg_VectorPart;

struct reg_VectorGrp
{
    struct vectorGrp  P;
    Object	      object;
    Matrix  	      mInv;
};

struct reg_VectorPart
{
    struct vectorPart      p;
    int	   	           cnts[3];
    int		           strides[3];
    int		           cstrides[3];
    int		           bumps[8];
    float   	           dels[9];
    float	           org[3];
    Matrix  	           mInv;
    float  	           *vectors;
    int			   constantData;
    int	   	           *partitionNeighbors;
    int		           flag;
    InvalidComponentHandle invElements;
};


static VectorPart   Reg_InitVectorPart(Field, Reg_VectorGrp, int);
static Error        Reg_DestroyVectorPart(VectorPart);
static int          Reg_FindElement_VectorPart(Reg_InstanceVars, int,
						POINT_TYPE *);
static int          Reg_FindMultiGridContinuation_VectorPart(Reg_InstanceVars,
						int, POINT_TYPE *);
static InstanceVars Reg_NewInstanceVars(VectorGrp);
static Error        Reg_FreeInstanceVars(InstanceVars);
static int          Reg_FindElement(InstanceVars, POINT_TYPE *);
static int          Reg_FindMultiGridContinuation(InstanceVars, POINT_TYPE *);
static int          Reg_Interpolate(InstanceVars, POINT_TYPE *, VECTOR_TYPE *);
static Error        Reg_StepTime(InstanceVars, double, VECTOR_TYPE *, double *);
static Error        Reg_FindBoundary(InstanceVars, 
				POINT_TYPE *, VECTOR_TYPE *, double *);
static int          Reg_Neighbor(InstanceVars, VECTOR_TYPE *);
static Error        Reg_Delete(VectorGrp);
static int          Reg_Weights(InstanceVars, POINT_TYPE *);
static int          Reg_FaceWeights(InstanceVars, POINT_TYPE *);
static Error        Reg_CurlMap(VectorGrp, MultiGrid);

static Matrix	    InvertN(Matrix, int);
static void	    ApplyN(Matrix, int, POINT_TYPE *, POINT_TYPE *);
static void	    ApplyRotationN(Matrix, int, VECTOR_TYPE *, VECTOR_TYPE *);
static Matrix	    GetXYZtoIJKMatrix(Reg_VectorPart, int, int *);
static int 	    Reg_Ghost(InstanceVars I, POINT_TYPE *p);
static int 	    Reg_ClampToBoundingBox(InstanceVars, POINT_TYPE *);

static InstanceVars
Reg_NewInstanceVars(VectorGrp p)
{
    Reg_InstanceVars iI;

    iI = (Reg_InstanceVars)DXAllocate(sizeof(struct reg_InstanceVars));
    memset(iI, -1, sizeof(struct reg_InstanceVars));

    iI->i.currentVectorGrp = p;
    iI->i.currentPartition = NULL;
    iI->i.isRegular = 1;

    return (InstanceVars)iI;
}

static Error
Reg_FreeInstanceVars(InstanceVars I)
{
    if (I->currentPartition != NULL)
        DXDelete((Pointer)I->currentPartition);

    if (I)
	DXFree((Pointer)I);
    return OK;
}

VectorGrp
_dxfReg_InitVectorGrp(Object object, char *elementType)
{
    Reg_VectorGrp   P = NULL;
    int                i, nP;

    if (! _dxfPartitionNeighbors(object))
    {
	fprintf(stderr, "Partition neighbors error\n");
	return NULL;
    }

    if (DXGetObjectClass(object) == CLASS_FIELD)
	nP = 1;
    else
	for (nP = 0; DXGetEnumeratedMember((Group)object, nP, NULL); nP++);
    
    P = (Reg_VectorGrp)DXAllocate(sizeof(struct reg_VectorGrp));
    if (! P)
    {
	fprintf(stderr, "Allocation error 1\n");
	goto error;
    }

    P->object = object;
    
    P->P.n = nP;

    P->P.p = (VectorPart *)DXAllocateZero(nP * sizeof(struct reg_VectorPart));
    if (! P->P.p)
    {
	fprintf(stderr, "Allocation error 2\n");
	goto error;
    }
    
    /*
     * Attach generic regular element type methods to 
     * return object
     */
    P->P.NewInstanceVars   		= Reg_NewInstanceVars;
    P->P.FreeInstanceVars  		= Reg_FreeInstanceVars;
    P->P.FindElement       		= Reg_FindElement;
    P->P.FindMultiGridContinuation      = Reg_FindMultiGridContinuation;
    P->P.Interpolate       		= Reg_Interpolate;
    P->P.StepTime          		= Reg_StepTime;
    P->P.FindBoundary      		= Reg_FindBoundary;
    P->P.Neighbor          		= Reg_Neighbor;
    P->P.CurlMap           		= Reg_CurlMap;
    P->P.Weights           		= Reg_Weights;
    P->P.FaceWeights       		= Reg_FaceWeights;
    P->P.Delete            		= Reg_Delete;
    P->P.Reset             		= NULL;
    P->P.Ghost             		= Reg_Ghost;
    P->P.ClampToBoundingBox           	= Reg_ClampToBoundingBox;

    if (! strcmp(elementType, "cubes"))
	P->P.nDim = 3;
    else if (! strcmp(elementType, "quads"))
	P->P.nDim = 2;
    else
    {
	fprintf(stderr, "element type error 1\n");
	goto error;
    }

    if (DXGetObjectClass(object) == CLASS_FIELD)
    {
	P->P.p[0] = Reg_InitVectorPart((Field)object, P, 1);
	if (! P->P.p[0])
	{
	    fprintf(stderr, "Reg_InitVectorPart 0 return error 1\n");
	    goto error;
	}

	P->P.p[0]->field = (Field)DXReference(object);
    }
    else
    {
	Group g = (Group)object;
	Field f;

	i = 0;
	while (NULL != (f = (Field)DXGetEnumeratedMember(g, i, NULL)))
	{
	    P->P.p[i] = Reg_InitVectorPart(f, P, (i == 0));
	    if (! P->P.p[i] && DXGetError() != ERROR_NONE)
	    {
		fprintf(stderr, "Reg_InitVectorPart 0 return error 2\n");
	        goto error;
	    }
	    
	    P->P.p[i]->field = (Field)DXReference((Object)f);
	    
	    i++;
	}
    }

    return (VectorGrp)P;

error:
    Reg_Delete((VectorGrp)P);
    return NULL;
}

static VectorPart
Reg_InitVectorPart(Field f, Reg_VectorGrp P, int flag)
{
    Reg_VectorPart    ip = NULL;
    Array 	      array;
    int		      meshOff[3];

    if (DXEmptyField(f))
	return NULL;

    ip = (Reg_VectorPart)DXAllocate(sizeof(struct reg_VectorPart));
    if (! ip)
	goto error;

    if (! _dxfInitVectorPart((VectorPart)ip, f))
    {
	fprintf(stderr, "Reg_InitVectorPart error 1\n");
        goto error;
    }

    array = (Array)DXGetComponentValue(f, "connections");
    if (! array)
    {
	fprintf(stderr, "Reg_InitVectorPart error 2\n");
	goto error;
    }

    if (! DXQueryGridConnections(array, NULL, ip->cnts))
    {
	fprintf(stderr, "Reg_InitVectorPart error 3\n");
	DXSetError(ERROR_DATA_INVALID, "irregular connections");
	goto error;
    }

    if (! DXGetMeshOffsets((MeshArray)array, meshOff))
    {
	fprintf(stderr, "Reg_InitVectorPart error 4\n");
	goto error;
    }

    array = (Array)DXGetComponentValue(f, "positions");
    if (! array)
    {
	fprintf(stderr, "Reg_InitVectorPart error 5\n");
	goto error;
    }
    
    if (! DXQueryGridPositions(array, NULL, ip->cnts, ip->org, ip->dels))
    {
	fprintf(stderr, "Reg_InitVectorPart error 6\n");
	DXSetError(ERROR_DATA_INVALID, "irregular positions");
	goto error;
    }

    if (flag)
	P->mInv = GetXYZtoIJKMatrix(ip, P->P.nDim, meshOff);

    array = (Array)DXGetComponentValue(f, "partition neighbors");
    if (array)
	ip->partitionNeighbors = (int *)DXGetArrayData(array);
    else
	ip->partitionNeighbors = NULL;
    
    array = (Array)DXGetComponentValue(f, "data");
    if (! array)
    {
	fprintf(stderr, "Reg_InitVectorPart error 7\n");
	DXSetError(ERROR_MISSING_DATA, "no data component found");
	goto error;
    }

    if (! DXTypeCheck(array, TYPE_FLOAT, CATEGORY_REAL, 1, P->P.nDim))
    {
	fprintf(stderr, "Reg_InitVectorPart error 8\n");
	DXSetError(ERROR_DATA_INVALID,
	   "dimensionality of positions differs from that of vector field");
	return NULL;
    }

    if (DXQueryConstantArray(array, NULL, NULL))
    {
	ip->vectors = (float *)DXGetConstantArrayData(array);
	ip->constantData = 1;
    }
    else
    {
	ip->vectors = (float *)DXGetArrayData(array);
	ip->constantData = 0;
    }

    
    if (! DXInvalidateConnections((Object)f))
    {
	fprintf(stderr, "Reg_InitVectorPart error 9\n");
	goto error;
    }
    
    DXGetComponentValue(f, "invalid connections");
    if (DXGetComponentValue(f, "invalid connections"))
    {
	ip->invElements = DXCreateInvalidComponentHandle((Object)f,
						    NULL, "connections");
	if (! ip->invElements) {
	    fprintf(stderr, "Reg_InitVectorPart error 10\n");
	    goto error;
	}
    }
    else
	ip->invElements = NULL;
    
    if (P->P.nDim == 3)
    {
	ip->cstrides[0] = (ip->cnts[2] - 1) * (ip->cnts[1] - 1);
	ip->cstrides[1] = (ip->cnts[2] - 1);
	ip->cstrides[2] = 1;
    }
    else
    {
	ip->cstrides[0] = (ip->cnts[1] - 1);
	ip->cstrides[1] = 1;
    }

    if (ip->p.dependency == DEP_ON_CONNECTIONS)
    {
	if (P->P.nDim == 3)
	{
	    ip->strides[0] = (ip->cnts[2] - 1) * (ip->cnts[1] - 1);
	    ip->strides[1] = (ip->cnts[2] - 1);
	    ip->strides[2] = 1;

	    ip->mInv = P->mInv;
	    ip->mInv.b[0] -= meshOff[0];
	    ip->mInv.b[1] -= meshOff[1];
	    ip->mInv.b[2] -= meshOff[2];
	}
	else
	{
	    ip->strides[0] = (ip->cnts[1] - 1);
	    ip->strides[1] = 1;

	    ip->mInv = P->mInv;
	    ip->mInv.b[0] -= meshOff[0];
	    ip->mInv.b[1] -= meshOff[1];
	}
    }
    else
    {
	if (P->P.nDim == 3)
	{
	    ip->strides[0] = ip->cnts[2] * ip->cnts[1];
	    ip->strides[1] = ip->cnts[2];
	    ip->strides[2] = 1;

	    ip->bumps[0] = 0;
	    ip->bumps[1] =                                   ip->strides[0];
	    ip->bumps[2] =                  ip->strides[1];
	    ip->bumps[3] =                  ip->strides[1] + ip->strides[0];
	    ip->bumps[4] = ip->strides[2];
	    ip->bumps[5] = ip->strides[2] +                  ip->strides[0];
	    ip->bumps[6] = ip->strides[2] + ip->strides[1];
	    ip->bumps[7] = ip->strides[2] + ip->strides[1] + ip->strides[0];

	    ip->mInv = P->mInv;
	    ip->mInv.b[0] -= meshOff[0];
	    ip->mInv.b[1] -= meshOff[1];
	    ip->mInv.b[2] -= meshOff[2];
	}
	else
	{
	    ip->strides[0] = ip->cnts[1];
	    ip->strides[1] = 1;

	    ip->bumps[0] = 0;
	    ip->bumps[1] =                  ip->strides[0];
	    ip->bumps[2] = ip->strides[1];
	    ip->bumps[3] = ip->strides[1] + ip->strides[0];

	    ip->mInv = P->mInv;
	    ip->mInv.b[0] -= meshOff[0];
	    ip->mInv.b[1] -= meshOff[1];
	}
    }
	
    ip->flag = 0;

    return (VectorPart)ip;

error:
    if (ip)
	DXFree((Pointer)ip);
    return NULL;
}
    
static Matrix
GetXYZtoIJKMatrix(Reg_VectorPart p, int n, int *mo)
{
    Matrix         m;

    if (n == 3)
    {
	memcpy((float *)m.A, p->dels, 9*sizeof(float));
	memcpy((float *)m.b, p->org, 3*sizeof(float));

	m = InvertN(m, 3);
    }
    else
    {
	m.A[0][0] = p->dels[0];
	m.A[0][1] = p->dels[1];
	m.A[0][2] = 0.0;
	m.A[1][0] = p->dels[2];
	m.A[1][1] = p->dels[3];
	m.A[1][2] = 0.0;
	m.A[2][0] = 0.0;
	m.A[2][1] = 0.0;	
	m.A[2][2] = 0.0;

	m.b[0] = p->org[0];
	m.b[1] = p->org[1];
	m.b[2] = 0.0;

	m = InvertN(m, 2);
    }

    m.b[0] += mo[0];
    m.b[1] += mo[1];
    m.b[2] += mo[2];

    return m;
}
    
static Error
Reg_Delete(VectorGrp P)
{
    Reg_VectorGrp  iP = (Reg_VectorGrp)P;

    if (iP)
    {
	int i;

	for (i = 0; i < iP->P.n; i++)
	    if (iP->P.p[i])
		Reg_DestroyVectorPart(iP->P.p[i]);

	DXFree((Pointer)iP->P.p);
	DXFree((Pointer)iP);
    }

    return OK;
}

static Error 
Reg_DestroyVectorPart(VectorPart p)
{
    Reg_VectorPart ip = (Reg_VectorPart)p;

    if (ip)
    {
        if (p->field)
	    DXDelete((Object)p->field);

	if (ip->invElements)
	    DXFreeInvalidComponentHandle(ip->invElements);

	DXFree((Pointer)ip);
    }

    return OK;
}

static int
Reg_FindElement(InstanceVars I, POINT_TYPE *point)
{
    int i;
    Reg_InstanceVars iI = (Reg_InstanceVars)I;
    Reg_VectorGrp    iP = (Reg_VectorGrp)I->currentVectorGrp;

    for (i = 0; i < iP->P.n; i++)
	if (_dxfIsInBox(iP->P.p[i], point, iP->P.nDim, 0))
	{
	    if (Reg_FindElement_VectorPart(iI, i, point))
		break;
	}
    
    if (i == iP->P.n)
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
Reg_FindMultiGridContinuation(InstanceVars I, POINT_TYPE *point)
{
    int i;
    Reg_InstanceVars iI = (Reg_InstanceVars)I;
    Reg_VectorGrp    iP = (Reg_VectorGrp)I->currentVectorGrp;

    for (i = 0; i < iP->P.n; i++)
	if (_dxfIsInBox(iP->P.p[i], point, iP->P.nDim, 1))
	{
	    if (Reg_FindMultiGridContinuation_VectorPart(iI, i, point))
		break;
	}
    
    if (i == iP->P.n)
	return 0;
    else
	return 1;
}

static int
Reg_FindElement_VectorPart(Reg_InstanceVars iI, int np, POINT_TYPE *point)
{
    Reg_VectorGrp  iP = (Reg_VectorGrp)iI->i.currentVectorGrp;
    Reg_VectorPart ip = (Reg_VectorPart)iP->P.p[np];
    POINT_TYPE	   cpoint[3];
    int		   nd, i, base;
    int		   *strd = ip->cstrides;

    if (! ip)
	return -1;
    
    ApplyN(ip->mInv, (nd = iP->P.nDim), point, cpoint);

    for (i = 0; i < nd; i++)
	if (cpoint[i] < -0.0001 || cpoint[i] >= ((ip->cnts[i]-1) + 0.0001))
	    break;
    
    if (i != nd)
	return 0;
	
    for (i = 0, base = 0; i < nd; i++)
    {
	iI->indices[i] = (int)cpoint[i];
	base += iI->indices[i]*(*strd++);
    }
	
    if (ip->invElements && DXIsElementInvalid(ip->invElements, base))
	    return 0;
    
    iI->cp = np;
    iI->ce = base;

    ((Reg_VectorPart)(iP->P.p[np]))->flag = 1;
    Reg_Weights((InstanceVars)iI, point);
    return 1;
}

static int
Reg_FindMultiGridContinuation_VectorPart(Reg_InstanceVars iI,
					int np, POINT_TYPE *point)
{
    Reg_VectorGrp  iP = (Reg_VectorGrp)iI->i.currentVectorGrp;
    Reg_VectorPart ip = (Reg_VectorPart)iP->P.p[np];
    POINT_TYPE	   cpoint[3];
    int		   nd, i, base;
    int		   *strd = ip->cstrides;

    if (! ip)
	return -1;
    
    ApplyN(ip->mInv, (nd = iP->P.nDim), point, cpoint);

    /*
     * If we passed the bounding box test, we clamp to
     * the grid's boundaries
     */
    for (i = 0; i < nd; i++)
	if (cpoint[i] < 0.0)
	    cpoint[i] = 0.0;
	else if (cpoint[i] >= (ip->cnts[i]-1))
	    cpoint[i] = (ip->cnts[i]-1.00001);
	
    if (i != nd)
	return 0;
	
    for (i = 0, base = 0; i < nd; i++)
    {
	iI->indices[i] = (int)cpoint[i];
	base += iI->indices[i]*(*strd++);
    }
	
    if (ip->invElements && DXIsElementInvalid(ip->invElements, base))
	    return 0;
    
    iI->cp = np;
    iI->ce = base;

    ((Reg_VectorPart)(iP->P.p[np]))->flag = 1;
    Reg_Weights((InstanceVars)iI, point);
    return 1;
}

static int
Reg_Weights(InstanceVars I, POINT_TYPE *pt)
{
    Reg_InstanceVars iI = (Reg_InstanceVars)I;
    Reg_VectorGrp    iP = (Reg_VectorGrp)iI->i.currentVectorGrp;
    Reg_VectorPart   ip = (Reg_VectorPart)iP->P.p[iI->cp];
    POINT_TYPE	     dels[3], ipoint[3];
    int		     nd, i;
    int		     *strd = ip->cstrides;
    int		     base  = 0;
    int		     indices[3];

    if (! ip)
	return -1;
    
    ApplyN(ip->mInv, (nd = iP->P.nDim), pt, ipoint);

    for (i = 0; i < nd; i++)
    {
	if (ipoint[i] < -0.0001 || ipoint[i] > (ip->cnts[i]-1) + 0.0001)
	    break;

	if (ipoint[i] < 0.0)
	    ipoint[i] = 0.0;
	
	if (ipoint[i] >= (ip->cnts[i]-1))
	{
	    ipoint[i] = (ip->cnts[i]-1);
	    indices[i] = (ip->cnts[i]-2);
	    dels[i] = 1.0;
	}
	else
	{
	    indices[i] = (int)(ipoint[i]);
	    dels[i] = ipoint[i] - indices[i];
	}
    }

    if (i < nd)
	return 0;
    
    for (i = 0; i < nd; i++)
	base += ((int)ipoint[i]) * *strd++;
    
    if (base != iI->ce)
#if 0
	return 0;
#else
    {
	iI->ce = base;
	for (i = 0; i < nd; i++)
	    iI->indices[i] = indices[i];
    }
#endif

    if (nd == 3)
    {
	double *w = iI->weights;
	double dx = dels[2];
	double dy = dels[1];
	double dz = dels[0];
	double A  = dx * dy;
	double B  = dx * dz;
	double C  = dy * dz;
	double D  = dx * dy * dz;

	*w++ = 1.0 - dz - dy - dx + A + B + C - D;
	*w++ = dz - B - C + D;
	*w++ = dy - C - A + D;
	*w++ = C - D;
	*w++ = dx - B - A + D;
	*w++ = B - D;
	*w++ = A - D;
	*w++ = D;
    }
    else if (nd == 2)
    {
	double dx   = dels[0];
	double dy   = dels[1];
	double dxdy = dx * dy;
	double *w   = iI->weights;

	*w++ = 1 - dx - dy + dxdy;
	*w++ = dx - dxdy;
	*w++ = dy - dxdy;
	*w++ = dxdy;
    }
    else
    {
	double dx   = dels[0];
	double *w   = iI->weights;

	*w++ = 1 - dx;
	*w++ = dx;
    }
    
    return 1;
}

static int
Reg_FaceWeights(InstanceVars I, POINT_TYPE *pt)
{
    Reg_InstanceVars iI = (Reg_InstanceVars)I;
    Reg_VectorGrp    iP = (Reg_VectorGrp)iI->i.currentVectorGrp;
    Reg_VectorPart   ip = (Reg_VectorPart)iP->P.p[iI->cp];
    POINT_TYPE	     dels[3], ipoint[3];
    int		     nd, i;

    if (! ip)
	return -1;

    ApplyN(ip->mInv, (nd = iP->P.nDim), pt, ipoint);
    
    for (i = 0; i < nd; i++)
    {
	if (iI->face == (i<<1)+0)
	{
	    iI->indices[i] = 0;
	    dels[i]        = 0.0;
	}
	else if (iI->face == (i<<1)+1)
	{
	    iI->indices[i] = ip->cnts[i]-2;
	    dels[i]        = 1.0;
	}
	else
	{
	    if (ipoint[i] < 0.0)
	    {
		iI->indices[i] = 0;
		dels[i] = 0.0;
	    } 
	    else if (ipoint[i] >= (ip->cnts[i]-1))
	    {
		iI->indices[i] = (ip->cnts[i]-1);
		dels[i] = ip->cnts[i]-1;
	    }
	    else
	    {
		iI->indices[i] = (int)ipoint[i];
		dels[i] = ipoint[i] - iI->indices[i];
	    }
	}
    }

    if (i < nd)
	return 0;

    if (nd == 3)
    {
	double *w = iI->weights;
	double dx = dels[2];
	double dy = dels[1];
	double dz = dels[0];
	double A  = dx * dy;
	double B  = dx * dz;
	double C  = dy * dz;
	double D  = dx * dy * dz;

	{
	    double aa;
	    aa = (1.0 + A + B + C) - (dz + dy + dx + D);
	    /*bb = 1.0 - dz - dy - dx + A + B + C - D;*/
	    *w++ = aa;
	}

	*w++ = dz - B - C + D;
	*w++ = dy - C - A + D;
	*w++ = C - D;
	*w++ = dx - B - A + D;
	*w++ = B - D;
	*w++ = A - D;
	*w++ = D;
    }
    else if (nd == 2)
    {
	double dx   = dels[0];
	double dy   = dels[1];
	double dxdy = dx * dy;
	double *w   = iI->weights;

	*w++ = 1 - dx - dy + dxdy;
	*w++ = dx - dxdy;
	*w++ = dy - dxdy;
	*w++ = dxdy;
    }
    else
    {
	double dx   = dels[0];
	double *w   = iI->weights;

	*w++ = 1 - dx;
	*w++ = dx;
    }
    
    return 1;
}

static Error
Reg_FindBoundary(InstanceVars I, POINT_TYPE *p, POINT_TYPE *v, double *t)
{
    Reg_InstanceVars iI = (Reg_InstanceVars)I;
    Reg_VectorGrp    iP = (Reg_VectorGrp)iI->i.currentVectorGrp;
    Reg_VectorPart   ip = (Reg_VectorPart)iP->P.p[iI->cp];
    int   i, nd;
    POINT_TYPE tmin, ipoint[3];
    int   face = -1, f=0;

    ApplyN(ip->mInv, (nd = iP->P.nDim), p, ipoint);
    ApplyRotationN(ip->mInv, iP->P.nDim, v, iI->ivector);

    tmin = DXD_MAX_FLOAT;
    for (i = 0; i < nd; i++)
    {
	double iv = iI->ivector[i];
	double l, t0;

	l  = ipoint[i];

	if (l < 0.0) l = 0.0;
	else if (l > (ip->cnts[i]-1)) l = (ip->cnts[i]-1);

	if (iv < 0)
	{
	    iv = -iv;
	    f  = (i << 1);
	}
	else if (iv > 0)
	{
	    l = (ip->cnts[i]-1) - l;
	    f  = (i << 1) + 1;
	}

	t0 = l / iv;
	if (t0 < tmin)
	{
	    tmin = t0;
	    face = f;
	}
    }

    if (tmin == DXD_MAX_FLOAT || tmin < 0.0 || face == -1)
    {
        tmin = 0.0;
	iI->face = 0;
	return ERROR;
    }

    iI->face = face;
    *t = tmin;

    return OK;
}
	
static int
Reg_Interpolate(InstanceVars I, POINT_TYPE *pt, VECTOR_TYPE *vec)
{
    Reg_InstanceVars iI    = (Reg_InstanceVars)I;
    Reg_VectorGrp    iP    = (Reg_VectorGrp)iI->i.currentVectorGrp;
    Reg_VectorPart   ip    = (Reg_VectorPart)iP->P.p[iI->cp];
    int		     nDim  = iP->P.nDim;
    int		     nVrts = 1 << nDim;
    int		     *ind  = iI->indices;
    int		     *strd = ip->strides;
    int		     *bump = ip->bumps;
    double	     *w    = iI->weights;
    int		     base;
    float	     *vector;
    int		     i, j;

    if (ip->constantData)
    {
	for (i = 0; i < nDim; i++)
	    vec[i] = (VECTOR_TYPE)ip->vectors[i];
	return OK;
    }

    if (ip->p.dependency == DEP_ON_POSITIONS)
    {
	base = 0;
	for (i = 0; i < nDim; i++)
	    base += *ind++ * *strd++;
	
	vec[0] = vec[1] = vec[2] = 0.0;
	for (i = 0; i < nVrts; i++, bump++, w++)
	{
	    vector = ip->vectors + nDim*(base + *bump);
	    for (j = 0; j < nDim; j++)
		vec[j] += *w * *vector++;
	}
    }
    else
    {
	base = 0;
	for (i = 0; i < nDim; i++)
	    base += *ind++ * *strd++;
    
	vector = ip->vectors + nDim*base;
	for (j = 0; j < nDim; j++)
	    vec[j] = *vector++;
    }

    return OK;
}

static Error
Reg_StepTime(InstanceVars I, double c, VECTOR_TYPE *vec, double *t)
{
    Reg_InstanceVars iI = (Reg_InstanceVars)I;
    Reg_VectorGrp    iP = (Reg_VectorGrp)iI->i.currentVectorGrp;
    Reg_VectorPart   ip = (Reg_VectorPart)iP->P.p[iI->cp];
    int              i, nd;
    double 	     l;

    /*
     * determine the step in time that results in a step along
     * the given step vector of length c.
     */
    ApplyRotationN(ip->mInv, iP->P.nDim, vec, iI->ivector);

    nd = iP->P.nDim;
    l = 0.0;
    for (i = 0; i < nd; i++)
	l += (iI->ivector[i] * iI->ivector[i]);
    
    if (l == 0.0)
	*t = 0.0;
    else
	*t = c / sqrt(l);

    return OK;
}
	
static int
Reg_Neighbor(InstanceVars I, VECTOR_TYPE *v)
{
    Reg_InstanceVars iI = (Reg_InstanceVars)I;
    Reg_VectorGrp    iP = (Reg_VectorGrp)iI->i.currentVectorGrp;
    Reg_VectorPart   ip = (Reg_VectorPart)iP->P.p[iI->cp];
    int 	     face, axis, base;
    int 	     *ind;
    int 	     *strd;
    int 	     i, nd;

    axis = iI->face >> 1;
    face = iI->face & 0x1;

    /*
     * Look to see whether this is an internal boundary arising
     * from an invalidated connection or an external boundary.
     */
    if ((face == 0 && iI->indices[axis] == 0) ||
        (face == 1 && iI->indices[axis] == (ip->cnts[axis]-2)))
    {
	int nbr;

	if (! ip->partitionNeighbors)
	    return 0;

	nbr = ip->partitionNeighbors[iI->face];

	if (nbr < 0)
	{
	    return 0;
	}

	iI->cp = nbr;

	ip = (Reg_VectorPart)iP->P.p[iI->cp];

	/*
	 * Find the element of the neighbor we are moving into
	 */
	ind  = iI->indices;
	strd = ip->cstrides;
	nd   = iP->P.nDim;

	if (face == 0)
	    ind[axis] = ip->cnts[axis] - 2;
	else
	    ind[axis] = 0;

	for (base = 0, i = 0; i < nd; i++)
	    base += *ind++ * *strd++;
	    
	/*
	 * Check to see if we will be entering an invalidated element
	 */
	if (ip->invElements)
	    if (DXIsElementInvalid(ip->invElements, base))
		return 0;

	iI->ce = base;
	ip->flag = 1;
	return 1;
    }
    else
    {
	ind  = iI->indices;
	strd = ip->cstrides;
	nd   = iP->P.nDim;

	for (base = 0, i = 0; i < nd; i++)
	    base += *ind++ * *strd++;
	
	if (face == 0)
	{
	    base -= ip->cstrides[axis];
	    iI->indices[axis] -= 1;
	}
	else
	{
	    base += ip->cstrides[axis];
	    iI->indices[axis] += 1;
	}

	if (ip->invElements != NULL)
	    if (DXIsElementInvalid(ip->invElements, base))
		return 0;
	
	iI->ce = base;
	return 1;
    }
}

static Matrix
InvertN(Matrix mIn, int nDim)
{
    if (nDim == 3)
	return DXInvert(mIn);
    else
    {
	Matrix mOut;
	float d;

	d = 1.0 / (mIn.A[0][0]*mIn.A[1][1] - mIn.A[0][1]*mIn.A[1][0]);

	mOut.A[0][0] =  mIn.A[1][1] * d;
	mOut.A[0][1] = -mIn.A[1][0] * d;
	mOut.A[1][0] = -mIn.A[0][1] * d;
	mOut.A[1][1] =  mIn.A[0][0] * d;

	mOut.b[0] = -(mIn.b[0]*mOut.A[0][0] + mIn.b[1]*mOut.A[0][1]);
	mOut.b[1] = -(mIn.b[0]*mOut.A[1][0] + mIn.b[1]*mOut.A[1][1]);

	return mOut;
    }
}

static void
ApplyN(Matrix m, int n, POINT_TYPE *in, POINT_TYPE *out)
{
    int i, j;

    for (i = 0; i < n; i++)
    {
	out[i] = m.b[i];
	for (j = 0; j < n; j++)
	    out[i] += in[j] * m.A[j][i];
	
    }

    return;
}

static void
ApplyRotationN(Matrix m, int n, VECTOR_TYPE *in, VECTOR_TYPE *out)
{
    int i, j;

    for (i = 0; i < n; i++)
    {
	out[i] = 0.0;
	for (j = 0; j < n; j++)
	    out[i] += in[j] * m.A[j][i];
    }

    return;
}

static Error
Reg_CurlMap(VectorGrp P, MultiGrid mg)
{
    int 	   i;
    Object 	   curl;

    if (P->n == 1)
    {
	if (! _dxfDivCurl((Object)P->p[0]->field,
					NULL, &curl))
	    goto error;

	if (! curl)
	    goto error;

	if (! DXSetMember((Group)mg, NULL, curl))
	    goto error;
    }
    else
    {
	CompositeField cf = DXNewCompositeField();
	if (! cf)
	    goto error;

	for (i = 0; i < P->n; i++)
	{
	    Object attr;

	    attr = DXGetComponentAttribute(P->p[i]->field, "data", "dep");
	    if (! attr || DXGetObjectClass(attr) != CLASS_STRING)
	    {
		DXSetError(ERROR_DATA_INVALID, "data dependency attribute");
		DXDelete((Object)cf);
		goto error;
	    }

	    if (!strcmp(DXGetString((String)attr), "connections"))
	    {
		DXSetError(ERROR_DATA_INVALID, 
		    "cannot compute curl when data is dep on connections");
		DXDelete((Object)cf);
		goto error;
	    }

	    if (! DXSetMember((Group)cf, NULL, (Object)P->p[i]->field))
	    {
		DXDelete((Object)cf);
		goto error;
	    }
	}

	if (! _dxfDivCurl((Object)cf, NULL, &curl))
	    goto error;

	if (! curl)
	    goto error;

	if (! DXSetMember((Group)mg, NULL, curl))
	    goto error;
	
	DXDelete((Object)cf);

	if (! DXSetMember((Group)mg, NULL, curl))
	    goto error;
    }

    return OK;

error:
    return ERROR;
}

static int
Reg_Ghost(InstanceVars I, POINT_TYPE *p)
{
    Reg_InstanceVars rI = (Reg_InstanceVars)I;
    Reg_VectorGrp    rP = (Reg_VectorGrp)(rI->i.currentVectorGrp);
    Reg_VectorPart   vp = (Reg_VectorPart)rP->P.p[rI->cp];
    int i, base;

    for (i = 0, base = 0; i <  rP->P.nDim; i++)
	base += rI->indices[i]*vp->cstrides[i];
	
    return vp->p.ghosts[base];
}


static int
Reg_ClampToBoundingBox(InstanceVars I, POINT_TYPE *p)
{
    Reg_InstanceVars rI = (Reg_InstanceVars)I;
    Reg_VectorGrp    rP = (Reg_VectorGrp)rI->i.currentVectorGrp;
    Reg_VectorPart   rp = (Reg_VectorPart)rP->P.p[rI->cp];
    int i;

    for (i = 0; i <  rP->P.nDim; i++)
    {
	if (p[i] < rp->p.min[i]) p[i] = rp->p.min[i];
	if (p[i] > rp->p.max[i]) p[i] = rp->p.max[i];
    }

    return OK;
}

