/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dxmods/_maptoplane.c,v 1.7 2006/01/03 17:02:21 davidt Exp $
 */

#include <dxconfig.h>

#if defined(HAVE_STRING_H)
#include <string.h>
#endif

#include <stdio.h>
#include <math.h>
#include <dx/dx.h>
#include "vectors.h"
#include "cases.h"
#include "_maptoplane.h"

#define FUZZ 0.001

#define POSITIVE		0x001
#define EQ_ZERO			0x010
#define NEGATIVE		0x100

#define SIDE0			0
#define ON_PLANE		1
#define SIDE1			2
#define I_COUNT			3

#define P	        	0x1000
#define Q	        	0x0100
#define R	        	0x0010
#define S	        	0x0001

#define PRIME1 18913
#define PRIME2 10607

#define MULVECTOR(a,b,c) { \
    (c).x = (a).x * (b); \
    (c).y = (a).y * (b); \
    (c).z = (a).z * (b); \
}

#define ADDVECTOR(a,b,c) { \
    (c).x = (a).x + (b).x; \
    (c).y = (a).y + (b).y; \
    (c).z = (a).z + (b).z; \
}

#define ADDMULVECTOR(a, b, c, d) { \
    (d).x = ((a).x * b) + (c).x; \
    (d).y = ((a).y * b) + (c).y; \
    (d).z = ((a).z * b) + (c).z; \
}

#define DOTVECTOR(a,b) \
    ((a).x*(b).x + (a).y*(b).y + (a).z*(b).z)

#define PLANEDISTANCE(pt, pl) \
    (DOTVECTOR((pt), (pl).normal) + (pl).offset)

typedef struct
{
    Vector	normal;
    float	offset;
} Plane;

static Error  MapToPlaneObject(Object, Plane, Vector *);

static Error  MTP_Field(Pointer);
static Field  MTP_tetras(Field, Plane);
static Field  MTP_cubes(Field, Plane, Matrix);
static Field  MTP_cubes_reg(Field, Plane, Matrix);
static Field  MTP_cubes_reg_AxisAligned(Field, Plane, Matrix);
static Field  MTP_cubes_irreg(Field, Plane);

static Object MTP_Cleanup(Object);

static Error  AddNormalsComponent(Field, Vector);
static Error  AddColorsComponent(Field);
static int    DoConnectionsMap(Field);

static int       Cmp(Key, Key);
#if 0
static PseudoKey Hash(Key);
#endif

#define MIS_ALIGNED	2
#define ALIGNED		1
#define INDETERMINATE	0

static int   Orientation(float *, float *, float *, Vector *);
static Error OrientTriangles(Field, Vector *);
static Error MakePlane(Vector *, Vector *, Plane *);
typedef struct 
{
    int   i0, i1;
    int   index;
    float d;
} Intercept;

typedef struct
{
    long      hash;
    Intercept *ptr;
} HashItem;

typedef struct
{
    Intercept x;
    int   axis;
    int   i, j;
    float intercept;
} RegIntercept;

static int  RegAddIntercept(SegList *, int, int, int);
static int  IrregAddIntercept(HashTable, SegList *, int, int);

static Error GetRegRegInfo(Field, Plane, Plane *, Matrix *);
static Error MapArrays(Field, SegList *, SegList *, Matrix *);
static Array MapArrays_PDep(Array, SegList *, Matrix *);
static Array MapArrays_Constant(Array, int);
static Array MapArrays_CDep(Array, SegList *);
static Array MapArrays_CDep_Irreg(Array, SegList *);
static Array MapArrays_PDep_Grid_Reg(Array, SegList *, Matrix *);
static Array MapArrays_PDep_Grid_Irreg(Array, SegList *);
static Array MapArrays_PDep_Irreg(Array, SegList *);

static int IsRegReg(Field);

static Matrix PermuteMatrix(Matrix, int *);

Object
_dxfMapToPlane(Object object, Vector *point, Vector *normal)
{
    Object copy = NULL;
    Object result = NULL;
    Plane  plane;

    if (! MakePlane(normal, point, &plane))
	goto error;
    
    copy = DXCopy(object, COPY_STRUCTURE);
    if (! copy)
	goto error;

    if (! DXCreateTaskGroup())
	goto error;

    if (! MapToPlaneObject(copy, plane, normal))
    {
	DXAbortTaskGroup();
	goto error;
    }
    
    if (! DXExecuteTaskGroup() || DXGetError() != ERROR_NONE)
	goto error;
    
    if (DXGetObjectClass(copy) != CLASS_FIELD)
    {
	result = MTP_Cleanup(copy);
	DXDelete(copy);

	if (DXGetError() != ERROR_NONE)
	    goto error;
    }
    else
	result = copy;

    return result;

error:
    DXDelete((Object)result);
    if (copy != object)
	DXDelete(copy);
    return NULL;
}

static Object
MTP_Cleanup(Object object)
{
    Class class;

    class = DXGetObjectClass(object);
    if (class == CLASS_GROUP)
	class = DXGetGroupClass((Group)object);
    
    switch(class)
    {
	case CLASS_FIELD:
	    if (DXEmptyField((Field)object))
		return NULL;
	    else
		return DXCopy(object, COPY_STRUCTURE);
	
	case CLASS_GROUP:
	case CLASS_MULTIGRID:
	case CLASS_COMPOSITEFIELD:
	{
	    Group  old, new;
	    Object child;
	    int    i, empty = 1;
	    char   *name;

	    old = (Group)object;
	    new = (Group)DXCopy(object, COPY_ATTRIBUTES);
	    if (! new)
		return NULL;

	    i = 0;
	    while (NULL != (child = DXGetEnumeratedMember(old, i++, &name)))
	    {
		child = MTP_Cleanup(child);
		if (child)
		{
		    empty = 0;
		    if (! DXSetMember(new, name, child))
		    {
			DXDelete(child);
			return NULL;
		    }
		}
		else if (DXGetError() != ERROR_NONE)
		{
		    DXDelete((Object)new);
		    return NULL;
		}
	    }
	    
	    if (empty)
	    {
		DXDelete((Object)new);
		return NULL;
	    }

	    return (Object)new;
	}

	case CLASS_SERIES:
	{
	    Series  old, new;
	    Object child;
	    int    i;
	    float  pos;
	    int    empty = 1;

	    old = (Series)object;
	    new = (Series)DXCopy(object, COPY_ATTRIBUTES);
	    if (! new)
		return NULL;

	    i = -1;
	    while (NULL != (child = DXGetSeriesMember(old, ++i, &pos)))
	    {
		child = MTP_Cleanup(child);
		if (! child && DXGetError() != ERROR_NONE)
		{
		    DXDelete((Object)new);
		    return NULL;
		}

		if (child)
		    empty = 0;
		else
		    child = (Object)DXEndField(DXNewField());
		    
		if (! DXSetSeriesMember(new, i, (double)pos, child))
		{
		    DXDelete(child);
		    DXDelete((Object)new);
		    return NULL;
		}
	    }
	    
	    if (empty)
	    {
		DXDelete((Object)new);
		return NULL;
	    }

	    return (Object)new;
	}

	case CLASS_XFORM:
	{
	    Object child;
	    Matrix matrix;
	    Object new;

	    DXGetXformInfo((Xform)object, &child, &matrix);

	    child = MTP_Cleanup(child);
	    if (child)
	    {
		new = (Object)DXNewXform(child, matrix);
		if (! new)
		    DXDelete((Object)child);
		return new;
	    }
	    else
		return NULL;
	}

	case CLASS_CLIPPED:
	{
	    Object clipped;
	    Object child;
	    Object new;

	    DXGetClippedInfo((Clipped)object, &child, &clipped);

	    child = MTP_Cleanup(child);
	    if (child)
	    {
		new = (Object)DXNewClipped(child, clipped);
		if (! new)
		    DXDelete((Object)child);
		return new;
	    }
	    else
		return NULL;
	}

	default:
	     DXSetError(ERROR_BAD_CLASS, "#11381");
	     return ERROR;
    }
}
    
typedef struct
{
    Plane   plane;
    Vector  normal;
    Field   field;
    Matrix  matrix;
} MTPArgs;

static Error
MapToPlaneObject(Object object, Plane plane, Vector *normal)
{
    MTPArgs args, iargs;
    Class class;

    class = DXGetObjectClass(object);
    if (class == CLASS_GROUP)
	class = DXGetGroupClass((Group)object);
    
    args.normal = iargs.normal = *normal;
    
    switch(class)
    {
	case CLASS_FIELD:
	{
	    if (IsRegReg((Field)object))
		GetRegRegInfo((Field)object, plane, &args.plane, &args.matrix);
	    else
		args.plane = plane;
	    
	    args.field = (Field)object;
	    return DXAddTask(MTP_Field, (Pointer)&args, sizeof(args), 1.0);
	}
	
	case CLASS_COMPOSITEFIELD:
	{
	    Group  group = (Group)object;
	    Object child;
	    int    i, firstRegReg = 1;

	    args.plane = plane;

	    i = 0;
	    while (NULL != (child = DXGetEnumeratedMember(group, i++, NULL)))
	    {
		if (DXGetObjectClass(child) != CLASS_FIELD)
		{
		    DXSetError(ERROR_BAD_CLASS, "#11381");
		    return ERROR;
		}

		if (! DXEmptyField((Field)child))
		{
		    if (IsRegReg((Field)child))
		    {
			if (firstRegReg)
			{
			    if (! GetRegRegInfo((Field)child, plane,
						    &iargs.plane, &iargs.matrix))
				return ERROR;
			    firstRegReg = 0;
			}

			iargs.field = (Field)child;

			if (!DXAddTask(MTP_Field, (Pointer)&iargs,
							sizeof(iargs), 1.0))
			    return ERROR;
		    }
		    else
		    {
			args.field = (Field)child;
			if (! DXAddTask(MTP_Field, (Pointer)&args, sizeof(args), 1.0))
			    return ERROR;
		    }
		}
	    }
	    
	    return OK;
	}

	case CLASS_MULTIGRID:
	case CLASS_SERIES:
	case CLASS_GROUP:
	{
	    Group  group = (Group)object;
	    Object child;
	    int    i;

	    i = 0;
	    while (NULL != (child = DXGetEnumeratedMember(group, i++, NULL)))
		if (! MapToPlaneObject(child, plane, normal))
		    return ERROR;
	    
	    return OK;
	}

	case CLASS_XFORM:
	{
	    Matrix matrix;
	    Plane iPlane;
	    Object child;

	    /*
	     * (PtR + O)Pl + d
	     *   = DXPt(RPl) + (OPl + d)
	     */
	    DXGetXformInfo((Xform)object, &child, &matrix);

	    iPlane.normal.x = matrix.A[0][0]*plane.normal.x
				+ matrix.A[0][1]*plane.normal.y
				    + matrix.A[0][2]*plane.normal.z;
	    iPlane.normal.y = matrix.A[1][0]*plane.normal.x
				+ matrix.A[1][1]*plane.normal.y
				    + matrix.A[1][2]*plane.normal.z;
	    iPlane.normal.z = matrix.A[2][0]*plane.normal.x
				+ matrix.A[2][1]*plane.normal.y
				    + matrix.A[2][2]*plane.normal.z;
	    
	    iPlane.offset = matrix.b[0]*plane.normal.x
				+ matrix.b[1]*plane.normal.y 
				    + matrix.b[2]*plane.normal.z
					+ plane.offset;
	    
	    return MapToPlaneObject(child, iPlane, normal);
	}

	case CLASS_CLIPPED:
	{
	    Object child;

	    DXGetClippedInfo((Clipped)object, &child, NULL);
	    return MapToPlaneObject(child, plane, normal);
	}

	default:
	     DXSetError(ERROR_BAD_CLASS, "#11381");
	     return ERROR;
    }
}

static Error
MTP_Field(Pointer ptr)
{
    Plane               plane;
    Field               field;
    Matrix		matrix;
    Vector		normal;
    Object		attr;

    plane  = ((MTPArgs *)ptr)->plane;
    field  = ((MTPArgs *)ptr)->field;
    matrix = ((MTPArgs *)ptr)->matrix;
    normal = ((MTPArgs *)ptr)->normal;

    if (! DXInvalidateConnections((Object)field))
	goto error;
    
    DXDeleteComponent(field, "invalid positions");

    /*
     * Check first whether the input field is empty.
     */
    if (DXEmptyField(field))
        return OK;

    if (!DXGetComponentValue(field,"connections"))
    {
        DXSetError(ERROR_MISSING_DATA,"#10240", "connections");
        goto error;
    }

    attr = DXGetComponentAttribute(field, "connections", "element type");
    if (! attr)
    {
	DXSetError(ERROR_MISSING_DATA, "#10255", "connections", "element type");
	goto error;
    }

    if (DXGetObjectClass(attr) != CLASS_STRING)
    {
	DXSetError(ERROR_DATA_INVALID, "#10200", "element type attribute");
	goto error;
    }

    if (DXGetComponentValue(field, "neighbors"))
	DXDeleteComponent(field, "neighbors");

    if (DXGetComponentValue(field, "data statistics"))
	DXDeleteComponent(field, "data statistics");

    if (! strcmp(DXGetString((String)attr), "tetrahedra"))
    {
	if (! MTP_tetras(field, plane))
	    goto error;
    }
    else if (! strcmp(DXGetString((String)attr), "cubes"))
    {
	if (! MTP_cubes(field, plane, matrix))
	    goto error;
    }
    else
    {
	DXSetError(ERROR_DATA_INVALID, "#11380", DXGetString((String)attr));
	goto error;
    }

    /*
     * Verify the orientation of the triangles
     */
    if (! OrientTriangles(field, &normal))
	goto error;

    if (! AddColorsComponent(field))
	goto error;

    if (! AddNormalsComponent(field, normal))
	goto error;

    if (! DXEndField(field))
	goto error;

    return OK;

error:
    return ERROR;
}

static Field
MTP_cubes(Field field, Plane plane, Matrix matrix)
{
    Array pA, cA;

    cA = (Array)DXGetComponentValue(field, "connections");
    pA = (Array)DXGetComponentValue(field, "positions");

    if (DXQueryGridConnections(cA, NULL, NULL)
	   && DXQueryGridPositions(pA, NULL, NULL, NULL, NULL))
    {
	if (! MTP_cubes_reg(field, plane, matrix))
	    return NULL;
    }
    else
    {
	if (! MTP_cubes_irreg(field, plane))
	    return NULL;
    }

    return field;
}

#define NEXTEDGE					\
    int next = start[(v == (nVerts-1)) ? 0 : v+1];

#define SKIP(i,j)					\
{							\
    if (i == next || j == next) continue;		\
}

static Field
MTP_tetras(Field field, Plane plane)
{
    Array		cArray, pArray;
    int			nTetras, nPts;
    int			*tet;
    float		*hilo=NULL;
    HashTable		ht=NULL;
    int			i;
    int			nTotTris = 0;
    SegList 		*tBuf = NULL;
    SegList 		*iBuf = NULL;
    SegList 		*mBuf = NULL;
    int			nAlloc;
    ArrayHandle		pHandle = NULL;
    Pointer		pScratch = NULL;
    int			size;
    float 		*pPtr=NULL;
    InvalidComponentHandle icHandle = NULL;

    icHandle = DXCreateInvalidComponentHandle((Object)field,
						NULL, "connections");
    if (! icHandle)
	goto error;
    
    hilo = NULL;
    ht   = NULL;

    cArray = (Array)DXGetComponentValue(field, "connections");
    if (! cArray)
    {
	DXSetError(ERROR_MISSING_DATA, "#10240", "connections");
	goto error;
    }

    pArray = (Array)DXGetComponentValue(field, "positions");
    if (! pArray)
    {
	DXSetError(ERROR_MISSING_DATA, "#10240", "positions");
	goto error;
    }

    DXGetArrayInfo (cArray, &nTetras, NULL, NULL, NULL, NULL);
    DXGetArrayInfo (pArray, &nPts, NULL, NULL, NULL, NULL);

    hilo = (float *)DXAllocateLocal(nPts*sizeof(float));
    if (! hilo)
    {
	DXResetError();
	hilo = (float *)DXAllocate(nPts*sizeof(float));
    }
    if (! hilo)
	goto error;

    pHandle = DXCreateArrayHandle(pArray);
    if (! pHandle)
	goto error;
    size = DXGetItemSize(pArray);
    pScratch = DXAllocate(size);
    if (! pScratch)
	goto error;

    for (i = 0; i < nPts; i++)
    {
	float d;

	pPtr = (float *)DXIterateArray(pHandle, i, ((Pointer)pPtr), pScratch);

	d = pPtr[0]*plane.normal.x +
		    pPtr[1]*plane.normal.y +
			  pPtr[2]*plane.normal.z +
				plane.offset;
	hilo[i] = d;
    }

    DXFreeArrayHandle(pHandle);
    pHandle = NULL;
    DXFree(pScratch);
    pScratch = NULL;

    ht = DXCreateHash(sizeof(HashItem), NULL, Cmp);
    if (! ht)
	goto error;
    
    nAlloc = 0.1*nTetras;
    if (nAlloc < 100)
	nAlloc = 100;

    tBuf = DXNewSegList(3*sizeof(int), nAlloc, nAlloc);
    if (! tBuf)
	goto error;
    
    iBuf = DXNewSegList(sizeof(Intercept), nAlloc, nAlloc);
    if (! tBuf)
	goto error;
    
    if (DoConnectionsMap(field))
    {
	mBuf = DXNewSegList(sizeof(int), nAlloc, nAlloc);
	if (! mBuf)
	    goto error;
    }

    /*
     * First, generate the triangles.
     *
     * For each tetrahedron...
     */

    tet = (int *)DXGetArrayData(cArray);
    for (i = 0; i < nTetras; i++)
    {
	int ip, iq, ir, is;
	float fp, fq, fr, fs;
	int v, flags, nVerts, loop, *start;
	int ptIndices[4], nRealVerts, vertex;
	int pos = 0, neg = 0;

	if (DXIsElementInvalid(icHandle, i))
	{
	    tet += 4;
	    continue;
	}

	ip = *tet++;
	iq = *tet++;
	ir = *tet++;
	is = *tet++;

	flags = 0;
	if ((fp = hilo[ip]) > 0.0)
	{
	    flags |= 0x1;
	    pos ++;
	}
	else if (fp < 0.0)
	    neg ++;

	if ((fq = hilo[iq]) > 0.0)
	{
	    flags |= 0x2;
	    pos ++;
	}
	else if (fq < 0.0)
	    neg ++;

	if ((fr = hilo[ir]) > 0.0)
	{
	    flags |= 0x4;
	    pos ++;
	}
	else if (fr < 0.0)
	    neg ++;

	if ((fs = hilo[is]) > 0.0)
	{
	    flags |= 0x8;
	    pos ++;
	}
	else if (fs < 0.0)
	    neg ++;
	
	if (! pos && ! neg)
	    continue;

	loop = tetras_case[flags];
	if (tetras_case[flags+1] == loop)
	    continue;

	nVerts = tetras_loops[loop].knt;
	start  = tetras_edges + tetras_loops[loop].start;

	nRealVerts = 0;
	for (v = 0; v < nVerts; v++)
	{
	    int p0=0, p1=0;
	    NEXTEDGE;

	    switch(start[v])
	    {
		case 0:
		    if (fp == 0.0)
		    {
			SKIP(1,2);
			p0 = ip;
			p1 = -1;
		    }
		    else if (fq == 0.0)
		    {
			SKIP(3,4);
			p0 = -1;
			p1 = iq;
		    }
		    else
		    {
			p0 = ip;
			p1 = iq;
		    }
		    break;

		case 1:
		    if (fp == 0.0)
		    {
			SKIP(0,2)
			p0 = ip;
			p1 = -1;
		    }
		    else if (fr == 0.0)
		    {
			SKIP(3,5);
			p0 = -1;
			p1 = ir;
		    }
		    else
		    {
			p0 = ip;
			p1 = ir;
		    }
		    break;
		
		case 2:
		    if (fp == 0.0)
		    {
			SKIP(0,1)
			p0 = ip;
			p1 = -1;
		    }
		    else if (fs == 0.0)
		    {
			SKIP(4,5);
			p0 = -1;
			p1 = is;
		    }
		    else
		    {
			p0 = ip;
			p1 = is;
		    }
		    break;
		
		case 3:
		    if (fq == 0.0)
		    {
			SKIP(0,4)
			p0 = iq;
			p1 = -1;
		    }
		    else if (fr == 0.0)
		    {
			SKIP(1,5);
			p0 = -1;
			p1 = ir;
		    }
		    else
		    {
			p0 = iq;
			p1 = ir;
		    }
		    break;
		
		case 4:
		    if (fq == 0.0)
		    {
			SKIP(0,3)
			p0 = iq;
			p1 = -1;
		    }
		    else if (fs == 0.0)
		    {
			SKIP(2,5);
			p0 = -1;
			p1 = is;
		    }
		    else
		    {
			p0 = iq;
			p1 = is;
		    }
		    break;
		
		case 5:
		    if (fr == 0.0)
		    {
			SKIP(1,3)
			p0 = ir;
			p1 = -1;
		    }
		    else if (fs == 0.0)
		    {
			SKIP(2,4);
			p0 = -1;
			p1 = is;
		    }
		    else
		    {
			p0 = ir;
			p1 = is;
		    }
		    break;
	    }

	    vertex = IrregAddIntercept(ht, iBuf, p0, p1);

	    if (vertex == -1)
		goto error;

	    ptIndices[nRealVerts++] = vertex;
	}
		
	for (v = 2; v < nRealVerts; v++)
	{
	    int p = ptIndices[0];
	    int q = ptIndices[v-1];
	    int r = ptIndices[v];

	    if (p != q && p != r && q != r)
	    {
		int *tri = (int *)DXNewSegListItem(tBuf);

		if (! tri)
		    goto error;

		*tri++ = p;
		*tri++ = q;
		*tri++ = r;


		if (mBuf)
		{
		    int *m = (int *)DXNewSegListItem(mBuf);

		    if (! m)
			goto error;
				    
		    *m = i;
		}

		nTotTris ++;
	    }
	}
    }

    /*
     * Now generate the points.
     */

    if (nTotTris)
    {
	Intercept *i;
	Array tArray;
	SegListSegment *slist;
	int tot;

	tArray = DXNewArray(TYPE_INT, CATEGORY_REAL, 1, 3);
	if (! tArray)
	    goto error;
	
	if (! DXAllocateArray(tArray, nTotTris))
	{
	    DXDelete((Object)tArray);
	    goto error;
	}
	
	DXInitGetNextSegListSegment(tBuf); tot = 0;
	while (NULL != (slist = DXGetNextSegListSegment(tBuf)))
	{
	    int n = DXGetSegListSegmentItemCount(slist);
	    if (! DXAddArrayData(tArray, tot, n, DXGetSegListSegmentPointer(slist)))
	    {
		DXDelete((Object)tArray);
		goto error;
	    }
	    tot += n;
	}

	DXSetComponentValue(field, "connections", (Object)tArray);

	if (! DXSetComponentAttribute(field, "connections", "element type",
				    (Object)DXNewString("triangles")))
	    goto error;
	
	if (! DXInitGetNextSegListItem(iBuf))
	    goto error;

	while (NULL != (i = (Intercept *)DXGetNextSegListItem(iBuf)))
	{
	    float d0, d1;

	    if (i->i1 != -1)
	    {
		d0 = hilo[i->i0];
		d1 = hilo[i->i1];
		i->d = d0 / (d0 - d1);
	    }
	}

	if (! MapArrays(field, iBuf, mBuf, NULL))
	    goto error;
	
	if (! DXEndField(field))
	    goto error;

    }
    else
    {
	char *name;

	while (DXGetEnumeratedComponentValue(field, 0, &name))
	    DXDeleteComponent(field, name);

	if (!DXEndField(field))
	    goto error;
    }

/* ok: */
    if (ht)
	DXDestroyHash(ht);
    DXFree((Pointer)hilo);
    if (iBuf)
	DXDeleteSegList(iBuf);
    if (tBuf)
	DXDeleteSegList(tBuf);
    if (mBuf)
	DXDeleteSegList(mBuf);
    if (icHandle)
	DXFreeInvalidComponentHandle(icHandle);

    return field;

error:

    if (icHandle)
	DXFreeInvalidComponentHandle(icHandle);
    if (ht)
	DXDestroyHash(ht);
    DXFree((Pointer)hilo);
    if (iBuf)
	DXDeleteSegList(iBuf);
    if (tBuf)
	DXDeleteSegList(tBuf);
    if (mBuf)
	DXDeleteSegList(mBuf);

    return NULL;
}

static Field
MTP_cubes_irreg(Field field, Plane plane)
{
    Array		cArray, pArray;
    int			nCubes, nPts;
    int			*cube=NULL;
    float		*hilo=NULL;
    HashTable		ht=NULL;
    int			i;
    int			nTotTris = 0;
    SegList 		*tBuf = NULL;
    SegList 		*iBuf = NULL;
    SegList 		*mBuf = NULL;
    int			nAlloc;
    ArrayHandle		cHandle = NULL, pHandle = NULL;
    Pointer		cScratch = NULL, pScratch = NULL;
    int			size;
    float 		*pPtr=NULL;
    InvalidComponentHandle icHandle = NULL;

    icHandle = DXCreateInvalidComponentHandle((Object)field,
						NULL, "connections");
    if (! icHandle)
	goto error;

    hilo = NULL;
    ht   = NULL;

    cArray = (Array)DXGetComponentValue(field, "connections");
    if (! cArray)
    {
	DXSetError(ERROR_MISSING_DATA, "#10240", "connectionscomponent");
	goto error;
    }

    pArray = (Array)DXGetComponentValue(field, "positions");
    if (! pArray)
    {
	DXSetError(ERROR_MISSING_DATA, "#10240", "positions");
	goto error;
    }

    DXGetArrayInfo (cArray, &nCubes, NULL, NULL, NULL, NULL);
    DXGetArrayInfo (pArray, &nPts, NULL, NULL, NULL, NULL);

    if (! hilo)
    {
	DXResetError();
	hilo = (float *)DXAllocate(nPts*sizeof(float));
    }
    if (! hilo)
	goto error;

    pHandle = DXCreateArrayHandle(pArray);
    if (! pHandle)
	goto error;
    size = DXGetItemSize(pArray);
    pScratch = DXAllocate(size);
    if (! pScratch)
	goto error;

    for (i = 0; i < nPts; i++)
    {
	float d;

	pPtr = (float *)DXIterateArray(pHandle, i, ((Pointer)pPtr), pScratch);

	d = pPtr[0]*plane.normal.x +
		    pPtr[1]*plane.normal.y +
			  pPtr[2]*plane.normal.z +
				plane.offset;
	hilo[i] = d;
    }

    DXFreeArrayHandle(pHandle);
    pHandle = NULL;
    DXFree(pScratch);
    pScratch = NULL;

    ht = DXCreateHash(sizeof(HashItem), NULL, Cmp);
    if (! ht)
	goto error;
    
    nAlloc = 0.1*nCubes;
    if (nAlloc < 100)
	nAlloc = 100;

    tBuf = DXNewSegList(3*sizeof(int), nAlloc, nAlloc);
    if (! tBuf)
	goto error;
    
    iBuf = DXNewSegList(sizeof(Intercept), nAlloc, nAlloc);
    if (! tBuf)
	goto error;
    
    if (DoConnectionsMap(field))
    {
	mBuf = DXNewSegList(sizeof(int), nAlloc, nAlloc);
	if (! mBuf)
	    goto error;
    }

    /*
     * First, generate the triangles.
     *
     * For each cube...
     */

    cHandle = DXCreateArrayHandle(cArray);
    if (! cHandle)
	goto error;
    size = DXGetItemSize(cArray);
    cScratch = DXAllocate(size);
    if (! cScratch)
	goto error;

    for (i = 0; i < nCubes; i++)
    {
	int ip, iq, ir, is, it, iu, iv, iw;
	float fp, fq, fr, fs, ft, fu, fv, fw;
	int flags, nVerts, loop, *start, nLoops;
	int ptIndices[12], nRealVerts, vertex;
	int v, l;

	cube = (int *)DXIterateArray(cHandle, i, (Pointer)cube, cScratch);
    
	if (DXIsElementInvalid(icHandle, i))
	    continue;

	ip = cube[0];
	iq = cube[1];
	ir = cube[2];
	is = cube[3];
	it = cube[4];
	iu = cube[5];
	iv = cube[6];
	iw = cube[7];

	flags = 0;
	if ((fp = hilo[ip]) > 0.0)
	    flags |= 0x01;
	if ((fq = hilo[iq]) > 0.0)
	    flags |= 0x02;
	if ((fr = hilo[ir]) > 0.0)
	    flags |= 0x04;
	if ((fs = hilo[is]) > 0.0)
	    flags |= 0x08;
	if ((ft = hilo[it]) > 0.0)
	    flags |= 0x10;
	if ((fu = hilo[iu]) > 0.0)
	    flags |= 0x20;
	if ((fv = hilo[iv]) > 0.0)
	    flags |= 0x40;
	if ((fw = hilo[iw]) > 0.0)
	    flags |= 0x80;
	
	loop   = cubes_case[flags];
	nLoops = cubes_case[flags+1] - loop;

	for (l = 0; l < nLoops; l++)
	{
	    nVerts = cubes_loops[loop].knt;
	    start  = cubes_edges + cubes_loops[loop].start;

	    nRealVerts = 0;
	    for (v = 0; v < nVerts; v++)
	    {
		int p0=0, p1=0;
		NEXTEDGE;

		switch(start[v])
		{
		    case 0:
			if (fp == 0.0)
			{
			    SKIP(4,8)
			    p0 = ip;
			    p1 = -1;
			}
			else if (fq == 0.0)
			{
			    SKIP(5,9);
			    p0 = -1;
			    p1 = iq;
			}
			else
			{
			    p0 = ip;
			    p1 = iq;
			}

			break;

		    case 1:
			if (fr == 0.0)
			{
			    SKIP(4,10)
			    p0 = ir;
			    p1 = -1;
			}
			else if (fs == 0.0)
			{
			    SKIP(5,11);
			    p0 = -1;
			    p1 = is;
			}
			else
			{
			    p0 = ir;
			    p1 = is;
			}

			break;

		    case 2:
			if (ft == 0.0)
			{
			    SKIP(6,8)
			    p0 = it;
			    p1 = -1;
			}
			else if (fu == 0.0)
			{
			    SKIP(7,9);
			    p0 = -1;
			    p1 = iu;
			}
			else
			{
			    p0 = it;
			    p1 = iu;
			}

			break;

		    case 3:
			if (fv == 0.0)
			{
			    SKIP(6,10)
			    p0 = iv;
			    p1 = -1;
			}
			else if (fw == 0.0)
			{
			    SKIP(7,11);
			    p0 = -1;
			    p1 = iw;
			}
			else
			{
			    p0 = iv;
			    p1 = iw;
			}

			break;

		    case 4:
			if (fp == 0.0)
			{
			    SKIP(0,8)
			    p0 = ip;
			    p1 = -1;
			}
			else if (fr == 0.0)
			{
			    SKIP(1,10);
			    p0 = -1;
			    p1 = ir;
			}
			else
			{
			    p0 = ip;
			    p1 = ir;
			}

			break;

		    case 5:
			if (fq == 0.0)
			{
			    SKIP(0,9)
			    p0 = iq;
			    p1 = -1;
			}
			else if (fs == 0.0)
			{
			    SKIP(1,11);
			    p0 = -1;
			    p1 = is;
			}
			else
			{
			    p0 = iq;
			    p1 = is;
			}

			break;

		    case 6:
			if (ft == 0.0)
			{
			    SKIP(2,8)
			    p0 = it;
			    p1 = -1;
			}
			else if (fv == 0.0)
			{
			    SKIP(3,10);
			    p0 = -1;
			    p1 = iv;
			}
			else
			{
			    p0 = it;
			    p1 = iv;
			}

			break;

		    case 7:
			if (fu == 0.0)
			{
			    SKIP(2,9)
			    p0 = iu;
			    p1 = -1;
			}
			else if (fw == 0.0)
			{
			    SKIP(3,11);
			    p0 = -1;
			    p1 = iw;
			}
			else
			{
			    p0 = iu;
			    p1 = iw;
			}

			break;

		    case 8:
			if (fp == 0.0)
			{
			    SKIP(0,4)
			    p0 = ip;
			    p1 = -1;
			}
			else if (ft == 0.0)
			{
			    SKIP(2,6);
			    p0 = -1;
			    p1 = it;
			}
			else
			{
			    p0 = ip;
			    p1 = it;
			}

			break;

		    case 9:
			if (fu == 0.0)
			{
			    SKIP(2,7)
			    p0 = iu;
			    p1 = -1;
			}
			else if (fq == 0.0)
			{
			    SKIP(0,5);
			    p0 = -1;
			    p1 = iq;
			}
			else
			{
			    p0 = iu;
			    p1 = iq;
			}

			break;

		    case 10:
			if (fr == 0.0)
			{
			    SKIP(1,4)
			    p0 = ir;
			    p1 = -1;
			}
			else if (fv == 0.0)
			{
			    SKIP(3,6);
			    p0 = -1;
			    p1 = iv;
			}
			else
			{
			    p0 = ir;
			    p1 = iv;
			}

			break;

		    case 11:
			if (fs == 0.0)
			{
			    SKIP(1,5)
			    p0 = is;
			    p1 = -1;
			}
			else if (fw == 0.0)
			{
			    SKIP(3,7);
			    p0 = -1;
			    p1 = iw;
			}
			else
			{
			    p0 = is;
			    p1 = iw;
			}

			break;

		}

		vertex = IrregAddIntercept(ht, iBuf, p0, p1);

		if (vertex == -1)
		    goto error;

		ptIndices[nRealVerts++] = vertex;
	    }
	
		
	    for (v = 2; v < nRealVerts; v++)
	    {
		int p, q, r;

		p = ptIndices[0];
		q = ptIndices[v-1];
		r = ptIndices[v];

		if (p != q && p != r && q != r)
		{
		    int *tri = (int *)DXNewSegListItem(tBuf);

		    if (! tri)
			goto error;

		    *tri++ = p;
		    *tri++ = q;
		    *tri++ = r;

		    if (mBuf)
		    {
			int *m = (int *)DXNewSegListItem(mBuf);

			if (! m)
			    goto error;
					
			*m = i;
		    }

		    nTotTris ++;
		}
	    }
	}
    }

    DXFreeArrayHandle(cHandle);
    cHandle = NULL;
    DXFree(cScratch);
    cScratch = NULL;

    /*
     * Now generate the points.
     */
    
    if (nTotTris)
    {
	Intercept *i;
	Array tArray;
	SegListSegment *slist;
	int tot;

	tArray = DXNewArray(TYPE_INT, CATEGORY_REAL, 1, 3);
	if (! tArray)
	    goto error;
	
	if (! DXAllocateArray(tArray, nTotTris))
	{
	    DXDelete((Object)tArray);
	    goto error;
	}
	
	DXInitGetNextSegListSegment(tBuf); tot = 0;
	while (NULL != (slist = DXGetNextSegListSegment(tBuf)))
	{
	    int n = DXGetSegListSegmentItemCount(slist);
	    if (! DXAddArrayData(tArray, tot, n, DXGetSegListSegmentPointer(slist)))
	    {
		DXDelete((Object)tArray);
		goto error;
	    }
	    tot += n;
	}

	DXSetComponentValue(field, "connections", (Object)tArray);

	if (! DXSetComponentAttribute(field, "connections", "element type",
				    (Object)DXNewString("triangles")))
	    goto error;
	
	if (! DXInitGetNextSegListItem(iBuf))
	    goto error;

	while (NULL != (i = (Intercept *)DXGetNextSegListItem(iBuf)))
	{
	    float d0, d1;

	    if (i->i1 != -1)
	    {
		d0 = hilo[i->i0];
		d1 = hilo[i->i1];
		i->d = d0 / (d0 - d1);
	    }
	}

	if (! MapArrays(field, iBuf, mBuf, NULL))
	    goto error;

	if (! DXEndField(field))
	    goto error;

    }
    else
    {
	char *name;

	while (DXGetEnumeratedComponentValue(field, 0, &name))
	    DXDeleteComponent(field, name);

	if (!DXEndField(field))
	    goto error;
    }
	 
/* ok: */
    if (ht)
	DXDestroyHash(ht);
    DXFree((Pointer)hilo);
    if (tBuf)
	DXDeleteSegList(tBuf);
    if (iBuf)
	DXDeleteSegList(iBuf);
    if (mBuf)
	DXDeleteSegList(mBuf);
    if (cHandle)
	DXFreeArrayHandle(cHandle);
    if (pHandle)
	DXFreeArrayHandle(pHandle);
    if (icHandle)
	DXFreeInvalidComponentHandle(icHandle);

    return field;

error:

    if (icHandle)
	DXFreeInvalidComponentHandle(icHandle);
    if (cHandle)
	DXFreeArrayHandle(cHandle);
    if (pHandle)
	DXFreeArrayHandle(pHandle);
    DXFree(pScratch);
    DXFree(cScratch);

    if (ht)
	DXDestroyHash(ht);
    DXFree((Pointer)hilo);
    if (iBuf)
	DXDeleteSegList(iBuf);
    if (tBuf)
	DXDeleteSegList(tBuf);
    if (mBuf)
	DXDeleteSegList(mBuf);

    return NULL;
}

static int
IrregAddIntercept(HashTable ht, SegList *iBuf, int r, int s)
{
    HashItem  hi, *hi_ptr;
    Intercept i;

    if (r > s)
    {
	i.i0 = r;
	i.i1 = s;
    }
    else
    {
	i.i0 = s;
	i.i1 = r;
    }

    hi.hash = i.i0*PRIME1 + i.i1*PRIME2;
    hi.ptr  = &i;

    if ((hi_ptr = (HashItem *)DXQueryHashElement(ht, (Key)&hi)) != NULL)
	return hi_ptr->ptr->index;
    
    hi.ptr = (Intercept *)DXNewSegListItem(iBuf);
    if (! hi.ptr)
        return -1;
    
    hi.ptr->i0 = i.i0;
    hi.ptr->i1 = i.i1;
    hi.ptr->index = DXGetSegListItemCount(iBuf) - 1;

    if (! DXInsertHashElement(ht, (Element)&hi))
	return -1;

    return hi.ptr->index;
}

#define XAXIS 0
#define YAXIS 1
#define ZAXIS 2

#define LAST 0
#define NEXT 1

/*
 * The following cases define the action taken when the corresponding 
 * edge is determined to intersect the plane in the regular algorithm below.
 */

#define REG_CASE0					\
{							\
    if (i == ixstart && j == jystart)			\
    {							\
	vertex = RegAddIntercept(iBuf,ZAXIS,i,j);	\
	if (vertex == -1) goto error;			\
	state[ZAXIS][LAST][j] = vertex;			\
    }							\
    else						\
	vertex = state[ZAXIS][LAST][j];			\
}

#define REG_CASE1					\
{							\
    if (i == ixstart)					\
    {							\
	vertex = RegAddIntercept(iBuf,ZAXIS,i,j+1);	\
	if (vertex == -1) goto error;			\
	state[ZAXIS][LAST][j+1] = vertex;		\
    }							\
    else						\
	vertex = state[ZAXIS][LAST][j+1];		\
}

#define REG_CASE2					\
{							\
    if (j == jystart)					\
    {							\
	vertex = RegAddIntercept(iBuf,ZAXIS,i+1,j);	\
	if (vertex == -1) goto error;			\
	state[ZAXIS][NEXT][j] = vertex;			\
    }							\
    else						\
	vertex = state[ZAXIS][NEXT][j];			\
}

#define REG_CASE3					\
{							\
    vertex = RegAddIntercept(iBuf,ZAXIS,i+1,j+1);	\
    if (vertex == -1) goto error;			\
    state[ZAXIS][NEXT][j+1] = vertex;			\
}

#define REG_CASE4					\
{							\
    if (i == ixstart && k == kzstart)			\
    {							\
	vertex = RegAddIntercept(iBuf,YAXIS,i,k);	\
	if (vertex == -1) goto error;			\
	state[YAXIS][LAST][k] = vertex;			\
    }							\
    else						\
	vertex = state[YAXIS][LAST][k];			\
}

#define REG_CASE5					\
{							\
    if (i == ixstart)					\
    {							\
	vertex = RegAddIntercept(iBuf,YAXIS,i,k+1);	\
	if (vertex == -1) goto error;			\
	state[YAXIS][LAST][k+1] = vertex;		\
    }							\
    else						\
	vertex = state[YAXIS][LAST][k+1];		\
}

#define REG_CASE6					\
{							\
    if (k == kzstart)					\
    {							\
	vertex = RegAddIntercept(iBuf,YAXIS,i+1,k);	\
	if (vertex == -1) goto error;			\
	state[YAXIS][NEXT][k] = vertex;			\
    }							\
    else						\
	vertex = state[YAXIS][NEXT][k];			\
}

#define REG_CASE7					\
{							\
    vertex = RegAddIntercept(iBuf,YAXIS,i+1,k+1);	\
    if (vertex == -1) goto error;			\
    state[YAXIS][NEXT][k+1] = vertex;			\
}

#define REG_CASE8					\
{							\
    if (j == jystart && k == kzstart)			\
    {							\
	vertex = RegAddIntercept(iBuf,XAXIS,j,k);	\
	if (vertex == -1) goto error;			\
	state[XAXIS][LAST][k] = vertex;			\
    }							\
    else						\
	vertex = state[XAXIS][LAST][k];			\
}

#define REG_CASE9					\
{							\
    if (j == jystart)					\
    {							\
	vertex = RegAddIntercept(iBuf,XAXIS,j,k+1);	\
	if (vertex == -1) goto error;			\
	state[XAXIS][LAST][k+1] = vertex;		\
    }							\
    else						\
	vertex = state[XAXIS][LAST][k+1];		\
}

#define REG_CASE10					\
{							\
    if (k == kzstart)					\
    {							\
	vertex = RegAddIntercept(iBuf,XAXIS,j+1,k);	\
	if (vertex == -1) goto error;			\
	state[XAXIS][NEXT][k] = vertex;			\
    }							\
    else						\
	vertex = state[XAXIS][NEXT][k];			\
}

#define REG_CASE11					\
{							\
    vertex = RegAddIntercept(iBuf,XAXIS,j+1,k+1);	\
    if (vertex == -1) goto error;			\
    state[XAXIS][NEXT][k+1] = vertex;			\
}

/*
 * The following are used when the plane exactly hits 
 * grid vertices.
 */
#define EXACT_HIT_INDEX(J,K) ((J)*count[2]+(K))

/*
 * The logic in the following is as follows:
 *
 *    Since we are not in the axis-aligned case, we know that
 *    the plane is not perpendicular to any one axis and therefore
 *    is single-valued when projected onto any of the planes
 *    perpendicular to an axis.  We can therefore project the exact
 *    match points all along the same axis.
 *
 *    Two buffers are used and are swapped when the outer loop 
 *    bumps.  The buffers correspond to the projection of the
 *    last and next X-faces onto the (X,Y) plane.  Since we have
 *    the single-value property, the X-faces project onto a vertical
 *    line in the (X,Y) plane. 
 *
 *    Exact matching vertices are created when the vertex is first
 *    encountered and retrieved from the buffers when re-encountered.
 *    The following flag = evaluates the condition deciding whether
 *    this is the first time the vertex is encountered.  The c(ijk)flgs
 *    are set if this is the first time through the (ijk)-loops.
 *    For example, vertex 0 is encountered for the first time only
 *    if this is the first cube in all three directions.  Vertex 1
 *    is encountered for the first time if this is the first
 *    time through the i and j loops (since it is on a face shared
 *    by a cube that precedes it in the looping) but regardless
 *    of whether this is the first time through the k-loop.  Vertex
 *    7 is always added, since none of the cubes it is shared by
 *    will have been processed yet.
 *
 *
 *    The last/next buffer is chosen by the vertex number: vertices 
 *    0 - 3 are in the x face (eg. last) while 4 - 7 are on the x+1 face
 *    (eg next). The actual intercept is projected along the Z axis at a 
 *    point (I,J), determined by the (i,j) location of the cube and the
 *    vertex number.
 */

#define REG_EXACT(i, j, k, v)					\
{								\
    int *buf;							\
    int ciflg = (i) == ixstart;					\
    int cjflg = (j) == jystart;					\
    int ckflg = (k) == kzstart;					\
    int flag = (v);						\
    int I, J;							\
								\
    flag = ((v) == 0 && ciflg && cjflg && ckflg) ||		\
           ((v) == 1 && ciflg && cjflg         ) ||		\
	   ((v) == 2 && ciflg          && ckflg) ||		\
	   ((v) == 3 && ciflg                  ) ||		\
	   ((v) == 4          && cjflg && ckflg) ||		\
	   ((v) == 5          && cjflg         ) ||		\
	   ((v) == 6                   && ckflg) ||		\
	   ((v) == 7                           );		\
    								\
    if ((v) > 3) buf = nextExact;				\
    else buf = lastExact;					\
								\
    I = (i) + (((v) & 0x4) ? 1 : 0);				\
    J = (j) + (((v) & 0x2) ? 1 : 0);				\
								\
    if (flag)							\
    {								\
	vertex = RegAddIntercept(iBuf, ZAXIS, I, J);		\
	buf[J] = vertex;					\
    }								\
    else vertex = buf[J];					\
}

#define REG_EXACT_0 REG_EXACT(i, j, k, 0)
#define REG_EXACT_1 REG_EXACT(i, j, k, 1)
#define REG_EXACT_2 REG_EXACT(i, j, k, 2)
#define REG_EXACT_3 REG_EXACT(i, j, k, 3)
#define REG_EXACT_4 REG_EXACT(i, j, k, 4)
#define REG_EXACT_5 REG_EXACT(i, j, k, 5)
#define REG_EXACT_6 REG_EXACT(i, j, k, 6)
#define REG_EXACT_7 REG_EXACT(i, j, k, 7)

#define SNAP(t, fuzz)			\
{					\
    int flag = t < 0;			\
    float d;				\
    if (flag) t = -t;			\
    d = t - (int)t;			\
    if (d < fuzz)			\
	t = (int)t;			\
    else if (d > (1-fuzz))		\
	t = (int)t + 1;			\
    if (flag) t = -t;			\
}


#define SWAP(t, a, b)	\
{			\
    t temp = a;		\
    a = b;		\
    b = temp;		\
}

static Field
MTP_cubes_reg(Field field, Plane plane, Matrix matrix)
{
    int	      i, j, k, v;
    Array     cArray;
    float     c00, c01=0, c10, c11=0;
    int       slabPoints[3];
    int       slabCubes[3];
    int       count[3];
    int       ixstart, jystart, kzstart;
    int       ixend, jyend, kzend;
    float     xstart, ystart, zstart;
    float     xend, yend, zend;
    int       xcube, ycube, zcube;
    int	      flags;
    int	      nTotTris = 0;
    int       *state[3][2];
    int       *stateBuf = NULL;
    int	      ptIndices[16];
    int	      meshOffsets[3];
    int       ix, jy, kz;          
    int	      ixMin, jyMin, kzMin;
    int	      ixMax, jyMax, kzMax;
    int	      ixSiz, jySiz, kzSiz;
    float     aInv, bInv, cInv;
    int	      doit;
    int       xproj;
    int	      planeSize;
    SegList   *iBuf = NULL;
    SegList   *tBuf = NULL;
    SegList   *mBuf = NULL;
    float     a, b, c, d;
    int       permute[3];
    float     xfuzz, yfuzz, zfuzz;
    int	      cubesIgnored = 0;
    InvalidComponentHandle icHandle = NULL;
    RegIntercept *ri;
    int	      *exactHits = NULL, *lastExact, *nextExact;

    /*
     * normalize the planar equation
     */
    d = sqrt(plane.normal.x*plane.normal.x +
	           plane.normal.y*plane.normal.y +
		       plane.normal.z*plane.normal.z);
    
    if (d == 0.0)
    {
	DXSetError(ERROR_BAD_PARAMETER, "degenerate planar equation");
	goto error;
    }

    d = 1.0 / d;
    plane.normal.x *= d;
    plane.normal.y *= d;
    plane.normal.z *= d;
    plane.offset   *= d;

    if (fabs(plane.normal.x) < 0.00001) plane.normal.x = 0.0;
    if (fabs(plane.normal.y) < 0.00001) plane.normal.y = 0.0;
    if (fabs(plane.normal.z) < 0.00001) plane.normal.z = 0.0;
    
    if (plane.normal.x==0.0 || plane.normal.y==0.0 || plane.normal.z==0.0)
	return MTP_cubes_reg_AxisAligned(field, plane, matrix);

    icHandle = DXCreateInvalidComponentHandle((Object)field,
						NULL, "connections");
    if (! icHandle)
	goto error;

    stateBuf = NULL;

    cArray = (Array)DXGetComponentValue(field, "connections");
    DXQueryGridConnections(cArray, NULL, count);
    DXGetMeshOffsets((MeshArray)cArray, meshOffsets);

    slabPoints[0] = count[1]*count[2];
    slabPoints[1] = count[2];
    slabPoints[2] = 1;

    slabCubes[0] = (count[1] - 1)*(count[2] - 1);
    slabCubes[1] = (count[2] - 1);
    slabCubes[2] = 1;

    /*
     * normalize the planar equation
     */
    d = 1.0 / sqrt(plane.normal.x*plane.normal.x +
	           plane.normal.y*plane.normal.y +
	           plane.normal.z*plane.normal.z);
    plane.normal.x *= d;
    plane.normal.y *= d;
    plane.normal.z *= d;
    plane.offset   *= d;
    

    /*
     * Choose the loop ordering so that the innermost loop tends to
     * be as long as possible (remember that we handle each cube that
     * intersects the plane exactly once - this ordering minimizes the 
     * number of loops that have to be initialized).  
     */

    permute[0] = 0; permute[1] = 1; permute[2] = 2;

    a = count[0]*plane.normal.x;
    if (a < 0) a = -a;
    b = count[1]*plane.normal.y;
    if (b < 0) b = -b;
    c = count[2]*plane.normal.z;
    if (c < 0) c = -c;


    if (b > a && b > c)
    {
	SWAP(float, plane.normal.x, plane.normal.y);
	SWAP(int, count[0], count[1]);
	SWAP(int, meshOffsets[0], meshOffsets[1]);
	SWAP(int, slabPoints[0], slabPoints[1]);
	SWAP(int, slabCubes[0], slabCubes[1]);
	SWAP(int, permute[0], permute[1]);
	SWAP(float, a, b);

    }
    else if (c > a && c > b)
    {
	SWAP(float, plane.normal.x, plane.normal.z);
	SWAP(int, count[0], count[2]);
	SWAP(int, meshOffsets[0], meshOffsets[2]);
	SWAP(int, slabPoints[0], slabPoints[2]);
	SWAP(int, slabCubes[0], slabCubes[2]);
	SWAP(int, permute[0], permute[2]);
	SWAP(float, a, c);
    }

    if (c > b)
    {
	SWAP(float, plane.normal.y, plane.normal.z);
	SWAP(int, count[1], count[2]);
	SWAP(int, meshOffsets[1], meshOffsets[2]);
	SWAP(int, slabPoints[1], slabPoints[2]);
	SWAP(int, slabCubes[1], slabCubes[2]);
	SWAP(int, permute[1], permute[2]);
    }

    matrix = PermuteMatrix(matrix, permute);
	
    if (plane.normal.x != 0)
	aInv = -1.0 / plane.normal.x;
    else
	aInv = 0.0;

    if (plane.normal.y != 0)
	bInv = -1.0 / plane.normal.y;
    else
	bInv = 0.0;

    if (plane.normal.z != 0)
	cInv = -1.0 / plane.normal.z;
    else
	cInv = 0.0;

    xfuzz = (plane.normal.x != 0.0) ? FUZZ / plane.normal.x : FUZZ;
    yfuzz = (plane.normal.y != 0.0) ? FUZZ / plane.normal.y : FUZZ;
    zfuzz = (plane.normal.z != 0.0) ? FUZZ / plane.normal.z : FUZZ;

    xfuzz = fabs(xfuzz);
    yfuzz = fabs(yfuzz);
    zfuzz = fabs(zfuzz);
    
    /*
     * An axis either intersects the plane once or is parallel to the plane.
     * The following flags are one if the plane intersects the axis once.
     */
    xproj = (plane.normal.x != 0.0);
    /*yproj = (plane.normal.y != 0.0);*/
    /*zproj = (plane.normal.z != 0.0);*/

    /*
     * Get the extents of this part of the nesh.
     */
    ixSiz = count[0] - 1;
    ixMin = meshOffsets[0];
    ixMax = ixMin + ixSiz;

    jySiz = count[1] - 1;
    jyMin = meshOffsets[1];
    jyMax = jyMin + jySiz;

    kzSiz = count[2] - 1;
    kzMin = meshOffsets[2];
    kzMax = kzMin + kzSiz;

    stateBuf = (int *)DXAllocate((4*count[2]+2*count[1])*sizeof(int));
    if (! stateBuf)
	goto error;
    
    state[XAXIS][LAST] = stateBuf;
    state[XAXIS][NEXT] = state[XAXIS][LAST] + count[2];
    state[YAXIS][LAST] = state[XAXIS][NEXT] + count[2];
    state[YAXIS][NEXT] = state[YAXIS][LAST] + count[2];
    state[ZAXIS][LAST] = state[YAXIS][NEXT] + count[2];
    state[ZAXIS][NEXT] = state[ZAXIS][LAST] + count[1];

    exactHits = (int *)DXAllocate(2*count[1]*sizeof(int));
    if (! exactHits)
	goto error;
    
    lastExact = exactHits;
    nextExact = lastExact + count[1];
    
    if (count[0] >= count[1] && count[0] >= count[2])
    {
	if (count[1] >= count[2])
	    planeSize = count[0]*count[1];
	else
	    planeSize = count[0]*count[2];
    }
    else if (count[1] >= count[0] && count[1] >= count[2])
    {
	    if (count[0] >= count[2])
		planeSize = count[1]*count[0];
	    else
		planeSize = count[1]*count[2];
    }
    else
    {
	    if (count[0] >= count[1])
		planeSize = count[2]*count[0];
	    else
		planeSize = count[2]*count[1];
    }

    iBuf = DXNewSegList(sizeof(RegIntercept), 0.25*planeSize, 0.25*planeSize);
    if (! iBuf)
	goto error;
    
    tBuf = DXNewSegList(3*sizeof(int), 0.5*planeSize, 0.25*planeSize);
    if (! tBuf)
	goto error;

    if (DoConnectionsMap(field))
    {
	mBuf = DXNewSegList(sizeof(int), planeSize, 0.5*planeSize);
	if (! mBuf)
	    goto error;
    }

    /*
     * Determine the slab along the X mesh axis that intersects the 
     * plane.  In the following loops, ix will index the current grid
     * position in the overall (composite field) mesh, while i will index
     * the current position in the local grid.
     */
    if (plane.normal.x == 0.0)
    {
	xstart = ixstart = ixMin;
	xend   = ixend   = ixMax - 1;
	doit   = 1;
    }
    else
    {
	float t;

	if (plane.normal.y == 0.0 && plane.normal.z == 0.0)
	{
	    xstart = xend = plane.offset * aInv;

	    if (xstart == (int)xstart && plane.normal.x < 0.0)
		xstart = xend = xstart - 1;
	}
	else
	{
	    xstart = xend = (jyMin*plane.normal.y +
				 kzMin*plane.normal.z +
				    plane.offset) * aInv;

	    t = (jyMax*plane.normal.y +
		    kzMin*plane.normal.z +
			plane.offset) * aInv;

	    if (t > xend) xend = t;
	    if (t < xstart) xstart = t;

	    t = (jyMin*plane.normal.y +
		    kzMax*plane.normal.z +
			plane.offset) * aInv;

	    if (t > xend) xend = t;
	    if (t < xstart) xstart = t;

	    t = (jyMax*plane.normal.y +
		    kzMax*plane.normal.z +
			plane.offset) * aInv;

	    if (t > xend) xend = t;
	    if (t < xstart) xstart = t;

	}

	SNAP(xend, xfuzz);
	SNAP(xstart, xfuzz);

	if (xstart == xend)
	{
	    xend = ixend = xstart = ixstart = (int)xstart;
	}
	else
	{
	    if (xstart < ixMin)
		xstart = ixMin;
		    
	    if (xend > ixMax)
		xend = ixMax;

	    ixstart = (int)xstart;
	    if (floor(xend) == xend)
		ixend = (int)(xend - 1);
	    else
		ixend = (int)xend;
	}

	if (xstart >= ixMax || xend < ixMin)
	    doit = 0;
	else
	    doit = 1;

    }

    /*
     * Here's the X loop.  For each YZ slab of the intersection area in
     * X, process the slab.
     */
    if (doit)
    {
	ix = ixstart;
	ixstart -= ixMin;
	ixend   -= ixMin;

	xcube = ixstart * slabCubes[0];
	for (i = ixstart; i <= ixend; i++, ix++)
	{
	    if (plane.normal.y == 0.0)
	    {
		ystart = jystart =  jyMin;
		yend   = jyend   =  jyMax - 1;
		doit   = 1;
	    }
	    else
	    {
		float t;

		if (plane.normal.x == 0.0 && plane.normal.z == 0.0)
		{
		    ystart = yend = plane.offset * bInv;

		    if (ystart == (int)ystart && plane.normal.y < 0.0)
			ystart = yend = ystart - 1;
		}
		else
		{

		    t = (ix*plane.normal.x +
			    kzMin*plane.normal.z +
				plane.offset) * bInv;

		    ystart = yend = t;

		    t = (ix*plane.normal.x +
			    kzMax*plane.normal.z +
				plane.offset) * bInv;

		    if (t > yend) yend = t;
		    if (t < ystart) ystart = t;

		    t = ((ix+1)*plane.normal.x +
			    kzMin*plane.normal.z +
				plane.offset) * bInv;

		    if (t > yend) yend = t;
		    if (t < ystart) ystart = t;

		    t = ((ix+1)*plane.normal.x +
			    kzMax*plane.normal.z +
				plane.offset) * bInv;

		    if (t > yend) yend = t;
		    if (t < ystart) ystart = t;
		}

		SNAP(yend, yfuzz);
		SNAP(ystart, yfuzz);

		if (ystart == yend)
		{
		    yend = jyend = ystart = jystart = (int)ystart;
		}
		else
		{
		    if (ystart < jyMin)
			ystart = jyMin;
		    
		    if (yend > jyMax)
			yend = jyMax;

		    jystart = (int)ystart;
		    if (floor(yend) == yend)
			jyend = (int)(yend - 1);
		    else
			jyend = (int)yend;
		}

		if (ystart >= jyMax || yend < jyMin)
		    doit = 0;
		else
		    doit = 1;
	    }

	    /*
	     * Here's the Y loop.  For each row of a YZ slab that
	     * intersects the plane, process the row.  
	     * In the following loops, jy will index the current grid
	     * position in the overall (composite field) mesh, while j
	     * will index the current position in the local grid.
	     */
	    if (doit)
	    {
		jy = jystart;
		jystart -= jyMin;
		jyend   -= jyMin;

		/*
		 * The logic depends on knowing whether this is the first
		 * slab in the intersection volume.  Because the intersection 
		 * along the slab is calculated differently from the
		 * determination of the intersection volume, we may find no	
		 * intersections in this slab.  In this case, this won't be 
		 * the first slab anymore.
		 */
		if (jystart > jyend) /* then no intersections */
		    ixstart ++;
		else
		{
		    ycube = xcube + jystart*slabCubes[1];
		    for (j = jystart; j <= jyend; j++, jy++)
		    {
			if (plane.normal.z == 0.0)
			{
			    float t00, t01, t10, t11;
			    int nVerts;
			    int *start;
			    int non, npos, nneg;
			    int start1[12], nVerts1;
			    int loops, nLoops;

			    kzstart = 0;
			    kzend   = kzSiz - 1;

			    t00 = ix*plane.normal.x +
					    jy*plane.normal.y + plane.offset;
			    SNAP(t00, zfuzz);

			    t10 = (ix+1)*plane.normal.x +
					    jy*plane.normal.y + plane.offset;

			    SNAP(t10, zfuzz);

			    t01 = ix*plane.normal.x +
					(jy+1)*plane.normal.y + plane.offset;
			    SNAP(t01, zfuzz);

			    t11 = (ix+1)*plane.normal.x +
					(jy+1)*plane.normal.y + plane.offset;
			    SNAP(t11, zfuzz);

			    non = npos = nneg = 0;

			    if (t00 == 0.0) non ++;
			    else if (t00 > 0) npos ++;
			    else nneg ++;

			    if (t10 == 0.0) non ++;
			    else if (t10 > 0) npos ++;
			    else nneg ++;

			    if (t01 == 0.0) non ++;
			    else if (t01 > 0) npos ++;
			    else nneg ++;

			    if (t11 == 0.0) non ++;
			    else if (t11 > 0) npos ++;
			    else nneg ++;
			
			    if (non == 1 && (npos == 0 || nneg == 0))
			    {
				jystart ++;
				continue;
			    }

			    flags = 0;
			    if (t00 > 0) flags |= 0x03;
			    if (t10 > 0) flags |= 0x30;
			    if (t01 > 0) flags |= 0x0C;
			    if (t11 > 0) flags |= 0xC0;
			     
			    zcube = ycube + kzstart;

			    loops = cubes_case[flags];
			    nLoops = cubes_case[flags+1] - loops;
			    if (nLoops != 1)
			    {
				DXSetError(ERROR_INTERNAL,
				    "cube case %d", flags);
				goto error;
			    }

			    nVerts = cubes_loops[loops].knt;
			    start  = cubes_edges + cubes_loops[loops].start;

			    for (v = 0, nVerts1 = 0; v < nVerts; v++)
			    {
				NEXTEDGE;

				switch(start[v])
				{
				    case 4:
					if (t00 == 0.0)
					    SKIP(0,8)
					else if (t01 == 0.0)
					    SKIP(1,10);
					break;
					    
				    case 5:
					if (t00 == 0.0)
					    SKIP(0,9)
					else if (t01 == 0.0)
					    SKIP(1,11);
					break;

				    case 6:
					if (t10 == 0.0)
					    SKIP(2,8)
					else if (t11 == 0.0)
					    SKIP(3,10);
					break;

				    case 7:
					if (t10 == 0.0)
					    SKIP(2,9)
					else if (t11 == 0.0)
					    SKIP(3,11);
					break;

				    case 8:
					if (t00 == 0.0)
					    SKIP(0,4)
					else if (t10 == 0.0)
					    SKIP(2,6);
					break;

				    case 9:
					if (t00 == 0.0)
					    SKIP(0,5)
					else if (t10 == 0.0)
					    SKIP(2,7);
					break;

				    case 10:
					if (t01 == 0.0)
					    SKIP(1,4)
					else if (t11 == 0.0)
					    SKIP(3,6);
					break;

				    case 11:
					if (t01 == 0.0)
					    SKIP(1,5)
					else if (t11 == 0.0)
					    SKIP(3,7);
					break;
				}

				start1[nVerts1++] = start[v];
			    }

			    for (k=kzstart; k <= kzend; k++,
						zcube += slabCubes[2])
			    {
				for (v = 0; v < nVerts1; v++)
				{
				    int  vertex=0;

				    /*
				     * Call an appropriate action for the edge.
				     */
				    switch(start1[v])
				    {
					case 4:
					    if (t00 == 0.0 && xproj)
						REG_CASE8
					    else if (t01 == 0.0 && xproj)
						REG_CASE10
					    else
						REG_CASE4;
					    break;
					
					case 5:
					    if (t00 == 0.0 && xproj)
						REG_CASE9
					    else if (t01 == 0.0 && xproj)
						REG_CASE11
					    else
						REG_CASE5;
					    break;
					
					case 6: 
					    if (t11 == 0.0 && xproj)
						REG_CASE10
					    else if (t10 == 0.0 && xproj)
						REG_CASE8
					    else
						REG_CASE6;
					    break;
					
					case 7:
					    if (t11 == 0.0 && xproj)
						REG_CASE11
					    else if (t10 == 0.0 && xproj)
						REG_CASE9
					    else
						REG_CASE7;
					    break;
					
					case 8:
					    if (t00 == 0.0 && !xproj)
						REG_CASE4
					    else if (t10 == 0.0 && !xproj)
						REG_CASE6
					    else
						REG_CASE8;
					    break;
					
					case 9:
					    if (t00 == 0.0 && !xproj)
						REG_CASE5
					    else if (t01 == 0.0 && !xproj)
						REG_CASE7
					    else
						REG_CASE9;
					    break;
					
					case 10:
					    if (t01 == 0.0 && !xproj)
						REG_CASE4
					    else if (t11 == 0.0 && !xproj)
						REG_CASE6
					    else
						REG_CASE10;
					    break;
					
					case 11:
					    if (t10 == 0.0 && !xproj)
						REG_CASE5
					    else if (t11 == 0.0 && !xproj)
						REG_CASE7
					    else
						REG_CASE11;
					    break;
				    }

				    if (vertex < 0)
					goto error;

				    ptIndices[v] = vertex;
				}

				if (DXIsElementInvalid(icHandle, zcube))
				{
				    cubesIgnored ++;
				    continue;
				}

				for (v = 2; v < nVerts; v++)
				{
				    int p = ptIndices[0];
				    int q = ptIndices[v-1];
				    int r = ptIndices[v];

				    if (p != q && p != r && q != r)
				    {

					int *tri =
					    (int *)DXNewSegListItem(tBuf);

					if (! tri)
					    goto error;

					*tri++ = ptIndices[0];
					*tri++ = ptIndices[v-1];
					*tri++ = ptIndices[v];


					if (mBuf)
					{
					    int *m =
						(int *)DXNewSegListItem(mBuf);

					    if (! m)
						goto error;
					    
					    *m = zcube;
					}

					nTotTris ++;
				    }
				}


			    }
			}
			else
			{

			    if (j == jystart)
			    {
				c00 = cInv * (ix*plane.normal.x
					    + jy*plane.normal.y
						    + plane.offset);

#define ZSNAP(c) 		\
{		 		\
    if ((c) < kzMin)		\
	c = kzMin;		\
    else if ((c) > kzMax)	\
	c = kzMax;		\
    else			\
	SNAP(c, zfuzz);		\
}
				SNAP(c00, zfuzz);
				c10 = cInv * ((ix+1)*plane.normal.x
					    + jy*plane.normal.y
						    + plane.offset);
				SNAP(c10, zfuzz);
			    }
			    else
			    {
				c00 = c01;
				c10 = c11;
			    }

			    c01 = cInv * (ix*plane.normal.x
					    + (jy+1)*plane.normal.y
						    + plane.offset);
			    SNAP(c01, zfuzz);

			    c11 = cInv * ((ix+1)*plane.normal.x
					    + (jy+1)*plane.normal.y
						    + plane.offset);
			    SNAP(c11, zfuzz);
			    zstart = zend = c00;

			    if (c10 > zend)   zend = c10;
			    if (c10 < zstart) zstart = c10;
			    if (c01 > zend)   zend = c01;
			    if (c01 < zstart) zstart = c01;
			    if (c11 > zend)   zend = c11;
			    if (c11 < zstart) zstart = c11;

			    if (zstart > kzMax || zend < kzMin)
				continue;
			    

			    if (zstart == zend)
			    {
				zend = kzend = zstart = kzstart = (int)zstart;
			    }
			    else
			    {
				kzstart = (int)zstart;
				if (zstart < kzMin)
				    kzstart = kzMin;

				if (floor(zend) == zend)
				    kzend = (int)(zend - 1);
				else
				    kzend = (int)zend;

				if (kzend > kzMax-1)
				    kzend = kzMax-1;
			    }
			
			    kz = kzstart;
			    kzstart -= kzMin;
			    kzend   -= kzMin;

			    zcube = ycube + kzstart;

			    flags = 0;
			    if (kz < c00)
				flags |= 0x03;
			    if (kz < c01)
				flags |= 0x0C;
			    if (kz < c10)
				flags |= 0x30;
			    if (kz < c11)
				flags |= 0xC0;

			    /*
			     * The logic depends on knowing whether this
			     * is the first row in the intersection slab.
			     * Because the intersection along the row is
			     * calculated differently from the determination
			     * of the intersection slab, we may find no
			     * intersections in this row.  In this case, this
			     * won't be the first row anymore.
			     */
			    if (kzstart > kzend)
				jystart ++;
			    else
			    {
				for (k = kzstart; k <= kzend; k++, kz++,
							zcube += slabCubes[2])
				{
				    int loops, nLoops;
				    int nVerts, nRealVerts;
				    int *start;

				    flags = (flags >> 1) & ~0xaa;

				    if ((kz + 1) < c00)
					flags |= 0x02;
				    if ((kz + 1) < c01)
					flags |= 0x08;
				    if ((kz + 1) < c10)
					flags |= 0x20;
				    if ((kz + 1) < c11)
					flags |= 0x80;

				    nRealVerts = 0;

				    loops = cubes_case[flags];
				    nLoops = cubes_case[flags+1] - loops;
				    if (nLoops != 1)
				    {
					DXSetError(ERROR_INTERNAL,
						"cube case %d", flags);
					goto error;
				    }

				    nVerts = cubes_loops[loops].knt;
				    start  = cubes_edges +
						cubes_loops[loops].start;

				    for (v = 0; v < nVerts; v++)
				    {
					int  vertex=0;
					NEXTEDGE;

					/*
					 * Call an appropriate action for the
					 * edge.  If an edge endpoint lies
					 * precisely in the plane, find an
					 * axis that intersects the plane at
					 * most once and project the vertex
					 * to that plane.
					 */
					switch(start[v])
					{
					    case 0:
						if (kz == c00)
						{
						    SKIP(4,8);
						    REG_EXACT_0;
						}
						else if ((kz+1) == c00)
						{
						    
						    SKIP(5,9);
						    REG_EXACT_1;
						}
						else
						    REG_CASE0;
						break;

					    case 1:
						if (kz == c01)
						{
						    SKIP(4,10);
						    REG_EXACT_2;
						}
						else if ((kz+1) == c01)
						{
						    SKIP(5,11);
						    REG_EXACT_3;
						}
						else
						    REG_CASE1;
						break;
					    
					    case 2:
						if (kz == c10)
						{
						    SKIP(8,6);
						    REG_EXACT_4;
						}
						else if ((kz+1) == c10)
						{
						    SKIP(9,7);
						    REG_EXACT_5;
						}
						else
						    REG_CASE2;

						break;
					    
					    case 3:
						if (kz == c11)
						{
						    SKIP(10,6);
						    REG_EXACT_6;
						}
						else if ((kz+1) == c11)
						{
						    SKIP(11,7);
						    REG_EXACT_7;
						}
						else
						    REG_CASE3;

						break;
					    
					    case 4:
						if (kz == c00)
						{
						    SKIP(0,8);
						    REG_EXACT_0;
						}
						else if (kz == c01)
						{
						    SKIP(1,10);
						    REG_EXACT_2;
						}
						else
						    REG_CASE4;
						break;
					    
					    case 5:
						if ((kz+1) == c00)
						{
						    SKIP(0,9);
						    REG_EXACT_1;
						}
						else if ((kz+1) == c01)
						{
						    SKIP(1,11);
						    REG_EXACT_3;
						}
						else
						    REG_CASE5;
						break;
					    
					    case 6: 
						if (kz == c10)
						{
						    SKIP(2,8);
						    REG_EXACT_4;
						}
						else if (kz == c11)
						{
						    SKIP(3,10);
						    REG_EXACT_6;
						}
						else
						    REG_CASE6;
						break;
					    
					    case 7:
						if ((kz+1) == c10)
						{
						    SKIP(2,9);
						    REG_EXACT_5;
						}
						else if ((kz+1) == c11)
						{
						    SKIP(3,11);
						    REG_EXACT_7;
						}
						else
						    REG_CASE7;
						break;
					    
					    case 8:
						if (kz == c00)
						{
						    SKIP(0,4);
						    REG_EXACT_0;
						}
						else if (kz == c10)
						{
						    SKIP(2,6);
						    REG_EXACT_4;
						}
						else
						    REG_CASE8;
						break;
					    
					    case 9:
						if ((kz+1) == c00)
						{
						    SKIP(0,5);
						    REG_EXACT_1;
						}
						else if ((kz+1) == c10)
						{
						    SKIP(2,7);
						    REG_EXACT_5;
						}
						else
						    REG_CASE9;
						break;
					    
					    case 10:
						if (kz == c01)
						{
						    SKIP(1,4);
						    REG_EXACT_2;
						}
						else if (kz == c11)
						{
						    SKIP(3,6);
						    REG_EXACT_6;
						}
						else
						    REG_CASE10;
						break;
					    
					    case 11:
						if ((kz+1) == c01)
						{
						    SKIP(1,5);
						    REG_EXACT_3;
						}
						else if ((kz+1) == c11)
						{
						    SKIP(3,7);
						    REG_EXACT_7;
						}
						else
						    REG_CASE11;
						break;
					}

					if (vertex < 0)
					    goto error;

					ptIndices[nRealVerts++] = vertex;
				    }

				    if (DXIsElementInvalid(icHandle, zcube))
				    {
					cubesIgnored ++;
					continue;
				    }

				    for (v = 2; v < nRealVerts; v++)
				    {
					int p = ptIndices[0];
					int q = ptIndices[v-1];
					int r = ptIndices[v];

					if (p != q && p != r && q != r)
					{
					    int *tri =
						(int *)DXNewSegListItem(tBuf);

					    if (! tri)
						goto error;

					    *tri++ = p;
					    *tri++ = q;
					    *tri++ = r;

					    if (mBuf)
					    {
						int *m = (int *)
						    DXNewSegListItem(mBuf);

						if (! m)
						    goto error;
						
						*m = zcube;
					    }

					    nTotTris ++;
					}
				    }

				}
			    }
			}

			{
			    int *t = state[XAXIS][LAST];
			    state[XAXIS][LAST] = state[XAXIS][NEXT];
			    state[XAXIS][NEXT] = t;
			}

			ycube += slabCubes[1];

		    }
		}
	    }

	    
	    {
		int *t = state[YAXIS][LAST];
		state[YAXIS][LAST] = state[YAXIS][NEXT];
		state[YAXIS][NEXT] = t;

		t = state[ZAXIS][LAST];
		state[ZAXIS][LAST] = state[ZAXIS][NEXT];
		state[ZAXIS][NEXT] = t;

		t = lastExact;
		lastExact = nextExact;
		nextExact = t;
	    }

	    xcube += slabCubes[0];
	}
    }

    if (nTotTris)
    {
	float a, b, c, d;
	Array tArray;
	SegListSegment *slist;
	int tot;

	tArray = DXNewArray(TYPE_INT, CATEGORY_REAL, 1, 3);
	if (! tArray)
	    goto error;
	
	if (! DXAllocateArray(tArray, nTotTris))
	{
	    DXDelete((Object)tArray);
	    goto error;
	}
	
	DXInitGetNextSegListSegment(tBuf); tot = 0;
	while (NULL != (slist = DXGetNextSegListSegment(tBuf)))
	{
	    int n = DXGetSegListSegmentItemCount(slist);
	    if (! DXAddArrayData(tArray, tot, n,
			    DXGetSegListSegmentPointer(slist)))
	    {
		DXDelete((Object)tArray);
		goto error;
	    }
	    tot += n;
	}

	DXSetComponentValue(field, "connections", (Object)tArray);

	if (! DXSetComponentAttribute(field, "connections", "element type",
				    (Object)DXNewString("triangles")))
	    goto error;
	
	if (! DXInitGetNextSegListItem(iBuf))
	    goto error;
	
	a = plane.normal.x;
	b = plane.normal.y;
	c = plane.normal.z;
	d = plane.offset;
	
	while (NULL != (ri = (RegIntercept *)DXGetNextSegListItem(iBuf)))
	{
	    float t;
	    int it;
	    int I, J;

	    I = ri->i;
	    J = ri->j;

	    switch(ri->axis)
	    {
		case XAXIS:
		    ri->i += meshOffsets[1];
		    ri->j += meshOffsets[2];
		    t = (b*ri->i + c*ri->j + d) * aInv;
		    if (t < meshOffsets[0])
			t = meshOffsets[0];
		    else if (t > (meshOffsets[0]+count[0]-1))
			t = meshOffsets[0]+count[0]-1;
		    ri->intercept = t;
		    if ((it = floor(t)) == t && it != meshOffsets[0])
		    {
			ri->x.d = 1.0;
			it  -= 1;
		    }
		    else
			ri->x.d = t - it;
		    ri->x.i0 = (it-meshOffsets[0])*slabPoints[0]
					+ I*slabPoints[1]
					    + J*slabPoints[2];
		    ri->x.i1 = ri->x.i0 + slabPoints[0];
		    break;

		case YAXIS:
		    ri->i += meshOffsets[0];
		    ri->j += meshOffsets[2];
		    t = (a*ri->i + c*ri->j + d) * bInv;
		    if (t < meshOffsets[1])
			t = meshOffsets[1];
		    else if (t > (meshOffsets[1]+count[1]-1))
			t = meshOffsets[1]+count[1]-1;
		    ri->intercept = t;
		    if ((it = floor(t)) == t && it != meshOffsets[1])
		    {
			ri->x.d = 1.0;
			it  -= 1;
		    }
		    else
			ri->x.d = t - it;
		    ri->x.i0 = I*slabPoints[0]
					+ (it-meshOffsets[1])*slabPoints[1]
					    + J*slabPoints[2];
		    ri->x.i1 = ri->x.i0 + slabPoints[1];
		    break;

		case ZAXIS:
		    ri->i += meshOffsets[0];
		    ri->j += meshOffsets[1];
		    t = (a*ri->i + b*ri->j + d) * cInv;
		    if (t < meshOffsets[2])
			t = meshOffsets[2];
		    else if (t > (meshOffsets[2]+count[2]-1))
			t = meshOffsets[2]+count[2]-1;
		    ri->intercept = t;
		    if ((it = floor(t)) == t && it != meshOffsets[2])
		    {
			ri->x.d = 1.0;
			it  -= 1;
		    }
		    else
			ri->x.d = t - it;
		    ri->x.i0 = I*slabPoints[0]
				    + J*slabPoints[1]
				    + (it-meshOffsets[2])*slabPoints[2];
		    ri->x.i1 = ri->x.i0 + slabPoints[2];
		    break;
	    }
	}

	if (! MapArrays(field, iBuf, mBuf, &matrix))
	    goto error;
	
	if (DXGetComponentValue(field, "invalid positions"))
	    DXDeleteComponent(field, "invalid posititions");

	if (DXGetComponentValue(field, "invalid connections"))
	    DXDeleteComponent(field, "invalid connections");

	if (cubesIgnored)
	{
	    if (! DXInvalidateUnreferencedPositions((Object)field))
		goto error;
	    
	    if (! DXCull((Object)field))
		goto error;
	    
	    if (! DXEndField(field))
		goto error;
	}
    }
    else
    {
	char *name;

	while (DXGetEnumeratedComponentValue(field, 0, &name))
	    DXDeleteComponent(field, name);

	if (!DXEndField(field))
	    goto error;
    }
	 
    DXFree((Pointer)stateBuf);
    DXDeleteSegList(iBuf);
    DXDeleteSegList(mBuf);
    DXDeleteSegList(tBuf);
    DXFree((Pointer)exactHits);
    if (icHandle)
	DXFreeInvalidComponentHandle(icHandle);

    return field;

error:

    if (icHandle)
	DXFreeInvalidComponentHandle(icHandle);
    DXFree((Pointer)stateBuf);
    DXFree((Pointer)exactHits);
    DXDeleteSegList(iBuf);
    DXDeleteSegList(mBuf);
    DXDeleteSegList(tBuf);

    return NULL;
}

static Error
MapArrays(Field field, SegList *iBuf, SegList *mBuf, Matrix *mat)
{
    int i;
    Array old, new = NULL;
    Object attr;
    char *name;

    if (DXGetComponentValue(field, "data statistics"))
	DXDeleteComponent(field, "data statistics");
    
    if (DXGetComponentValue(field, "box"))
	DXDeleteComponent(field, "box");

    /*nIntercepts = DXGetSegListItemCount(iBuf);*/

    i = 0;
    while ((old = (Array)DXGetEnumeratedComponentValue(field, i++, &name))
								    != NULL)
    {
	if (! strcmp(name, "connections"))
	    continue;
	
	if (NULL == (attr = DXGetComponentAttribute(field, name, "dep")))
	    continue;
	
	if (! strcmp(DXGetString((String)attr), "positions"))
	    new = MapArrays_PDep(old, iBuf, mat);
	else
	    new = MapArrays_CDep(old, mBuf);
	
	if (! new)
	    goto error;
	
	if (! DXSetComponentValue(field, name, (Object)new))
	    goto error;
	new = NULL;
    }

    return OK;

error:
    DXDelete((Object)new);
    return ERROR;
}

static Array
MapArrays_PDep(Array old, SegList *iBuf, Matrix *mat)
{
    Array new;

    if (DXQueryGridPositions(old, NULL, NULL, NULL, NULL))
    {
        if (mat)
	{
	    new = MapArrays_PDep_Grid_Reg(old, iBuf, mat);
	}
	else
	{
	    new = MapArrays_PDep_Grid_Irreg(old, iBuf);
	}
    }
    else if (DXQueryConstantArray(old, NULL, NULL))
    {
	new = MapArrays_Constant(old, DXGetSegListItemCount(iBuf));
    }
    else
    {
	new = MapArrays_PDep_Irreg(old, iBuf);
    }

    return new;
}

static Array
MapArrays_PDep_Grid_Reg(Array old, SegList *iBuf, Matrix *mat)
{
    Array        new;
    Vector       *base;
    RegIntercept *x;
    float        A00, A01, A02, A10, A11, A12, A20, A21, A22;
    float        B0, B1, B2;
    float        X=0, Y=0, Z=0;

    new = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, 3);
    if (! new)
	goto error;
    
    if (! DXAddArrayData(new, 0, DXGetSegListItemCount(iBuf), NULL))
	goto error;
    
    base = (Vector *)DXGetArrayData(new);

    if (! DXInitGetNextSegListItem(iBuf))
	goto error;
    
    A00 = mat->A[0][0]; A01 = mat->A[0][1]; A02 = mat->A[0][2];
    A10 = mat->A[1][0]; A11 = mat->A[1][1]; A12 = mat->A[1][2];
    A20 = mat->A[2][0]; A21 = mat->A[2][1]; A22 = mat->A[2][2];

    B0 = mat->b[0]; B1 = mat->b[1]; B2 = mat->b[2];
    
    while (NULL != (x = (RegIntercept *)DXGetNextSegListItem(iBuf)))
    {
	Vector *dst = base + x->x.index;

	switch(x->axis)
	{
	    case XAXIS:
		X = x->intercept;
		Y = x->i;
		Z = x->j;
		break;
	    
	    case YAXIS:
		X = x->i;
		Y = x->intercept;
		Z = x->j;
		break;

	    case ZAXIS:
		X = x->i;
		Y = x->j;
		Z = x->intercept;
		break;
	}

	dst->x = A00*X + A10*Y + A20*Z + B0;
	dst->y = A01*X + A11*Y + A21*Z + B1;
	dst->z = A02*X + A12*Y + A22*Z + B2;
    }

    return new;

error:
    DXDelete((Object)new);
    return NULL;
}
		
static Array
MapArrays_PDep_Grid_Irreg(Array old, SegList *iBuf)
{
    Array        new=NULL;
    Vector       *base;
    Intercept    *ix;
    int		 c[3];
    Vector	 o, d[3];
    int		 zknt, yknt;

    if (! DXQueryGridPositions(old, NULL, c, (float *)&o, (float *)d))
	goto error;
    
    zknt = c[2];
    yknt = c[1];

    new = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, 3);
    if (! new)
	goto error;
    
    if (! DXAddArrayData(new, 0, DXGetSegListItemCount(iBuf), NULL))
	goto error;
    
    base = (Vector *)DXGetArrayData(new);

    if (! DXInitGetNextSegListItem(iBuf))
	goto error;
    
    while (NULL != (ix = (Intercept *)DXGetNextSegListItem(iBuf)))
    {
	Vector p0, p1, *dst = base + ix->index;
	int jj;
	float x, y, z;

	z  = ix->i0  % zknt;
	jj = ix->i0  / zknt;
	y  =    jj % yknt;
	x  =    jj / yknt;

	ADDMULVECTOR(d[0], x,  o, p0);
	ADDMULVECTOR(d[1], y, p0, p0);
	ADDMULVECTOR(d[2], z, p0, p0);

	if (ix->i1 != -1)
	{
	    z  = ix->i1  % zknt;
	    jj = ix->i1  / zknt;
	    y  =    jj % yknt;
	    x  =    jj / yknt;

	    ADDMULVECTOR(d[0], x,  o, p1);
	    ADDMULVECTOR(d[1], y, p1, p1);
	    ADDMULVECTOR(d[2], z, p1, p1);

	    dst->x = p0.x + ix->d*(p1.x - p0.x);
	    dst->y = p0.y + ix->d*(p1.y - p0.y);
	    dst->z = p0.z + ix->d*(p1.z - p0.z);
	}
	else
	    *dst = p0;
    }

    return new;

error:
    DXDelete((Object)new);
    return NULL;
}
		
static Array
MapArrays_Constant(Array old, int n)
{
    int sz = DXGetItemSize(old);
    Pointer a;
    Array new = NULL;
    Type t;
    Category c;
    int r;
    int s[32];

    a = DXAllocate(sz);
    if (! a)
	goto error;

    DXGetArrayInfo(old, NULL, &t, &c, &r, s);
    DXQueryConstantArray(old, NULL, a);

    new = (Array)DXNewConstantArrayV(n, a, t, c, r, s);
    
error:
    DXFree(a);

    return new;
}

#define INTERPOLATE_INTERCEPT(type, rnd)				\
{									\
    type  *nbase = (type *)DXGetArrayData(new);				\
    type  *p0, *p1, *p;							\
    int   i, itemSize;							\
    float d;								\
									\
    if (!nbase)								\
	goto error;							\
									\
    itemSize = DXGetItemSize(old) / DXTypeSize(t);			\
									\
    if (! DXInitGetNextSegListItem(iBuf))				\
	goto error;							\
    									\
    while (NULL != (x = (Intercept *)DXGetNextSegListItem(iBuf)))	\
    {									\
	if (x->i1 != -1)						\
	{								\
	    p0 = (type *)DXGetArrayEntry(oHandle, x->i0, oScratch0);	\
	    p1 = (type *)DXGetArrayEntry(oHandle, x->i1, oScratch1);	\
	    p  = nbase + itemSize*x->index;				\
	    d  = x->d;							\
									\
	    for (i = 0; i < itemSize; i++)				\
	    {								\
		*p = (float)*p0 + d*((float)*p1 - (float)*p0) + rnd;	\
		p++, p0++, p1++;					\
	    }								\
	}								\
	else								\
	{								\
	    p0 = (type *)DXGetArrayEntry(oHandle, x->i0, oScratch0);	\
	    p  = nbase + itemSize*x->index;				\
									\
	    for (i = 0; i < itemSize; i++)				\
	    {								\
		*p = *p0;						\
		p++, p0++;						\
	    }								\
	}								\
    }									\
}
	
static Array
MapArrays_PDep_Irreg(Array old, SegList *iBuf)
{
    Array        new = NULL;
    Intercept    *x;
    Type         t;
    Category     c;
    int          r, s[32];
    ArrayHandle  oHandle = NULL;
    Pointer	 oScratch0 = NULL, oScratch1;

    DXGetArrayInfo(old, NULL, &t, &c, &r, s);

    new = DXNewArrayV(t, c, r, s);
    if (! new)
	goto error;
    
    if (! DXAddArrayData(new, 0, DXGetSegListItemCount(iBuf), NULL))
	goto error;
    
    oHandle = DXCreateArrayHandle(old);
    if (! oHandle)
	goto error;
    
    oScratch0 = DXAllocate(2*DXGetItemSize(old));
    if (! oScratch0)
	goto error;
    
    oScratch1 = (Pointer)((char *)oScratch0 + DXGetItemSize(old));
    
    switch(t)
    {
	case TYPE_DOUBLE:
	     INTERPOLATE_INTERCEPT(double, 0.0);
	     break;
	case TYPE_FLOAT:
	     INTERPOLATE_INTERCEPT(float, 0.0);
	     break;
	case TYPE_INT:
	     INTERPOLATE_INTERCEPT(int, 0.5);
	     break;
	case TYPE_UINT:
	     INTERPOLATE_INTERCEPT(uint, 0.5);
	     break;
	case TYPE_SHORT:
	     INTERPOLATE_INTERCEPT(short, 0.5);
	     break;
	case TYPE_USHORT:
	     INTERPOLATE_INTERCEPT(ushort, 0.5);
	     break;
	case TYPE_BYTE:
	     INTERPOLATE_INTERCEPT(byte, 0.5);
	     break;
	case TYPE_UBYTE:
	     INTERPOLATE_INTERCEPT(ubyte, 0.5);
	     break;
	default:
	     DXSetError(ERROR_DATA_INVALID, "#10320", "data");
	     goto error;
    }
    
    DXFreeArrayHandle(oHandle);
    DXFree(oScratch0);

    return new;

error:
    DXFreeArrayHandle(oHandle);
    DXFree(oScratch0);
    DXDelete((Object)new);
    return NULL;
}
		
static Array
MapArrays_CDep(Array old, SegList *mBuf)
{
    Array new;

    if (DXGetArrayClass(old) == CLASS_REGULARARRAY)
    {
	new = MapArrays_Constant(old, DXGetSegListItemCount(mBuf));
    }
    else
    {
	new = MapArrays_CDep_Irreg(old, mBuf);
    }

    return new;
}

static Array
MapArrays_CDep_Irreg(Array old, SegList *mBuf)
{
    Array          new = NULL;
    Type           t;
    Category       c;
    int            r, s[32];
    int            i, size;
    char           *src, *dst;
    SegListSegment *slist;
    Pointer        oScratch = NULL;
    ArrayHandle    oHandle = NULL;

    DXGetArrayInfo(old, NULL, &t, &c, &r, s);

    new = DXNewArrayV(t, c, r, s);
    if (! new)
	goto error;
    
    if (! DXAddArrayData(new, 0, DXGetSegListItemCount(mBuf), NULL))
	goto error;

    oHandle = DXCreateArrayHandle(old);
    if (! oHandle)
        goto error;
    
    size = DXGetItemSize(old);

    oScratch = DXAllocate(size);
    if (! oScratch)
	goto error;

    dst = (char *)DXGetArrayData(new);

    DXInitGetNextSegListSegment(mBuf);
    while (NULL != (slist = DXGetNextSegListSegment(mBuf)))
    {
	int *m = (int *)DXGetSegListSegmentPointer(slist);
	int  n = DXGetSegListSegmentItemCount(slist);

	for (i = 0; i < n; i++, m++)
	{
	    src = (char *)DXGetArrayEntry(oHandle, (*m), oScratch);
	    memcpy(dst, src, size);
	    dst += size;
	}
    }
    
    DXFreeArrayHandle(oHandle);
    DXFree(oScratch);
    return new;

error:
    DXDelete((Object)new);
    return NULL;
}
	    
static Error
AddNormalsComponent(Field field, Vector normal)
{
    Array array;
    int   n;
    Object attr;
    char *name;

    if (DXEmptyField(field))
	return OK;

    vector_normalize(&normal, &normal);

    name = "colors";
    array = (Array)DXGetComponentValue(field, name);
    if (! array)
    {
	name = "data";
	array = (Array)DXGetComponentValue(field, name);
    }
    if (! array)
    {
	array = (Array)DXGetComponentValue(field, "positions");
	if (! array)
	    return OK;
    
	DXGetArrayInfo(array, &n, NULL, NULL, NULL, NULL);
	attr = (Object)DXNewString("positions");
    }
    else
    {
	attr = DXGetAttribute((Object)array, "dep");
	if (!attr || DXGetObjectClass(attr) != CLASS_STRING)
	{
	    DXSetError(ERROR_INTERNAL, "#10241", name);
	    return ERROR;
	}

	if (! strcmp(DXGetString((String)attr), "positions"))
	{
	    array = (Array)DXGetComponentValue(field, "positions");
	    if (! array)
		return OK;
    
	    DXGetArrayInfo(array, &n, NULL, NULL, NULL, NULL);
	}
	else if (! strcmp(DXGetString((String)attr), "connections"))
	{
	    array = (Array)DXGetComponentValue(field, "connections");
	    if (! array)
		return OK;
	
	    DXGetArrayInfo(array, &n, NULL, NULL, NULL, NULL);
	}
	else
	{
	    DXSetError(ERROR_INTERNAL, "#11250", "data");
	    return ERROR;
	}
    }

    array = (Array)DXNewConstantArray(n, (Pointer)&normal, 
				    TYPE_FLOAT, CATEGORY_REAL, 1, 3);
    if (! array)
	return ERROR;
    
    if (! DXSetComponentValue(field, "normals", (Object)array))
    {
	DXDelete((Object)array);
	return ERROR;
    }

    if (! DXSetComponentAttribute(field, "normals", "dep", attr))
	return ERROR;

    return OK;
}

static Error
AddColorsComponent(Field field)
{

    if (DXEmptyField(field))
	return OK;

    if (! DXGetComponentValue(field, "colors") &&
	! DXGetComponentValue(field, "front colors")) 
    {
	Array array;
	float color[3];
	int   n;
	Object attr;

	if (!DXGetComponentValue(field, "data"))
	{
	    array = (Array)DXGetComponentValue(field, "positions");
	    if (! array)
		return OK;
	
	    DXGetArrayInfo(array, &n, NULL, NULL, NULL, NULL);
	    attr = (Object)DXNewString("positions");
	}
	else
	{
	    attr = DXGetComponentAttribute(field, "data", "dep");
	    if (!attr || DXGetObjectClass(attr) != CLASS_STRING)
	    {
		DXSetError(ERROR_INTERNAL, "#10241", "data");
		return ERROR;
	    }

	    if (! strcmp(DXGetString((String)attr), "positions"))
	    {
		array = (Array)DXGetComponentValue(field, "positions");
		if (! array)
		    return OK;
	    
		DXGetArrayInfo(array, &n, NULL, NULL, NULL, NULL);
	    }
	    else if (! strcmp(DXGetString((String)attr), "connections"))
	    {
		array = (Array)DXGetComponentValue(field, "connections");
		if (! array)
		    return OK;
	    
		DXGetArrayInfo(array, &n, NULL, NULL, NULL, NULL);
	    }
	    else
	    {
		DXSetError(ERROR_INTERNAL, "#11250", "data");
		return ERROR;
	    }
	}

	color[0] = 0.5; color[1] = 0.7; color[2] = 1.0;

	array = (Array)DXNewConstantArray(n, (Pointer)color,
			    TYPE_FLOAT, CATEGORY_REAL, 1, 3);
	if (! array)
	     return ERROR;
    
	if (! DXSetComponentValue(field, "colors", (Object)array))
	{
	    DXDelete((Object)array);
	    return ERROR;
	}

	if (! DXSetComponentAttribute(field, "colors", "dep", attr))
	    return ERROR;
    }

    return OK;
}

#if 0
static PseudoKey
Hash(Key key)
{
    Intercept *i;

    i = *(Intercept **)key;
    return i->i0*PRIME1 + i->i1*PRIME2;
}
#endif

static int
Cmp(Key k0, Key k1)
{
    Intercept *i0 = ((HashItem *)k0)->ptr;
    Intercept *i1 = ((HashItem *)k1)->ptr;

    return ! (i0->i0 == i1->i0 && i0->i1 == i1->i1);
}

static int 
Orientation(float *p, float *q, float *r, Vector *normal)
{
    Vector qp, rp, cross;
    float  d;

    qp.x = q[0] - p[0]; qp.y = q[1] - p[1]; qp.z = q[2] - p[2];
    rp.x = r[0] - p[0]; rp.y = r[1] - p[1]; rp.z = r[2] - p[2];

    _dxfvector_cross_3D(&qp, &rp, &cross);

    d = _dxfvector_dot_3D(&cross, normal);

    if (d == 0.0)
	return INDETERMINATE;
    else if (d > 0)
	return ALIGNED;
    else
	return MIS_ALIGNED;
}

static Error
OrientTriangles(Field field, Vector *n)
{
    Array pA, cA;
    float *points;
    int   *tri, i;
    int   nPoints, nTriangles;

    if (DXEmptyField(field))
	return OK;

    pA = (Array)DXGetComponentValue(field, "positions");
    cA = (Array)DXGetComponentValue(field, "connections");


    if (! DXGetArrayInfo(pA, &nPoints, NULL, NULL, NULL, NULL))
	goto error;

    if (! DXGetArrayInfo(cA, &nTriangles, NULL, NULL, NULL, NULL))
	goto error;
    
    points = (float *)DXGetArrayData(pA);
    tri    = (int *)DXGetArrayData(cA);

    for (i = 0; i < nTriangles; i++, tri += 3)
    {
	float *p = points+3*tri[0];
	float *q = points+3*tri[1];
	float *r = points+3*tri[2];
	if (Orientation(p, q, r, n) == MIS_ALIGNED)
	{
	    int t = tri[0];
	    tri[0] = tri[1];
	    tri[1] = t;
	}
    }

    return OK;

error:
    return ERROR;
}

static int
DoConnectionsMap(Field field)
{
    int i;
    char *name;
    Object attr;

    for (i = 0; DXGetEnumeratedComponentValue(field, i, &name); i++)
    {
	if (!strcmp(name, "connections"))
	    continue;

	attr = DXGetComponentAttribute(field, name, "dep");
	if (attr && !strcmp(DXGetString((String)attr), "connections"))
	    return 1;
    }

    return 0;
}

static Error
MakePlane(Vector *normal, Vector *point, Plane *plane)
{
    if (vector_zero(normal))
    {
	DXSetError(ERROR_BAD_PARAMETER, "#11822", "normal");
	return ERROR;
    }

    if (! vector_normalize(normal, &plane->normal))
	return ERROR;
    
    plane->offset = -vector_dot(&plane->normal, point);

    return OK;
}

static Error
GetRegRegInfo(Field field, Plane plane, Plane *iPlane, Matrix *matrix)
{
    Array     pA;
    Array     cA;
    int       meshOffsets[3];
    Vector    d[3];
    Vector    o;
    double    A, B, C, D, l;

    /*
     * Get the regular positions information
     */
    pA = (Array)DXGetComponentValue(field, "positions");
    if (!DXQueryGridPositions(pA, NULL, NULL, (float *)&o, (float *)d))
	return ERROR;
    
    /*
     * Get the planar equation in mesh coordinates relative to the
     * overall mesh origin.
     */
    cA = (Array)DXGetComponentValue(field, "connections");
    if (! DXQueryGridConnections(cA, NULL, NULL))
	return ERROR;

    DXGetMeshOffsets((MeshArray)cA, meshOffsets);

    ADDMULVECTOR(d[0], -meshOffsets[0], o, o);
    ADDMULVECTOR(d[1], -meshOffsets[1], o, o);
    ADDMULVECTOR(d[2], -meshOffsets[2], o, o);

    /*
     * If I is a point in index space, M = (Mr, Mt) the transformation
     * from index space to positions space defined by a 3x3 matrix (the 
     * deltas) and a 3-vector translation (the offset), and (Pabc, Pd)
     * a planar equation expressed in positions space, then the result
     * of applying the planar equation to the point I is:
     *		  P(IM) = (IM)Pabc + Pd 
     *			= (IMr + Mt)Pabc + Pd
     *			= IMrPabc + (MtPabc + Pd)
     *			= I(MrPabc) + (MtPabc + Pd)
     * In other words, we can define a new planar equation in index space
     * by the 3-vector MrPabc and the offset MtPabc + Pd.
     */
    A = d[0].x*plane.normal.x + d[0].y*plane.normal.y + d[0].z*plane.normal.z;
    B = d[1].x*plane.normal.x + d[1].y*plane.normal.y + d[1].z*plane.normal.z;
    C = d[2].x*plane.normal.x + d[2].y*plane.normal.y + d[2].z*plane.normal.z;
    D = o.x*plane.normal.x + o.y*plane.normal.y +
				o.z*plane.normal.z + plane.offset;

    l = A*A + B*B + C*C;
    if (l == 0.0)
    {
	DXSetError(ERROR_INTERNAL, "#11831", "positions grid");
	return ERROR;
    }

    iPlane->normal.x = A / l;
    iPlane->normal.y = B / l;
    iPlane->normal.z = C / l;
    iPlane->offset   = D / l;

    matrix->A[0][0] = d[0].x; matrix->A[0][1] = d[0].y; matrix->A[0][2] = d[0].z;
    matrix->A[1][0] = d[1].x; matrix->A[1][1] = d[1].y; matrix->A[1][2] = d[1].z;
    matrix->A[2][0] = d[2].x; matrix->A[2][1] = d[2].y; matrix->A[2][2] = d[2].z;
    matrix->b[0]    =    o.x; matrix->b[1]    =    o.y; matrix->b[2]    =    o.z;
    
    return OK;
}

static int
IsRegReg(Field field)
{
    Array a;
    int n;

    a = (Array)DXGetComponentValue(field, "connections");
    if (! a)
	return 0;
    
    if (! DXQueryGridConnections(a, &n, NULL))
	return 0;
    
    if (n != 3)
	return 0;
    
    a = (Array)DXGetComponentValue(field, "positions");
    if (! a)
	return 0;
    
    if (! DXQueryGridPositions(a, NULL, NULL, NULL, NULL))
	return 0;
    
    return 1;
}

static int
RegAddIntercept(SegList *intercepts, int axis, int i, int j)
{
    RegIntercept *intercept;

    intercept = (RegIntercept *)DXNewSegListItem((SegList *)intercepts);
    if (! intercept)
	goto error;

    intercept->axis    = axis;
    intercept->i       = i;
    intercept->j       = j;
    intercept->x.index = DXGetSegListItemCount((SegList *)intercepts) - 1;

    return intercept->x.index;

error:
    return -1;
}

static Matrix
PermuteMatrix(Matrix m, int *permute)
{
    Matrix r;
    int i, j, k;

    i = permute[0];
    j = permute[1];
    k = permute[2];

    r.A[0][0] = m.A[i][0]; r.A[0][1] = m.A[i][1]; r.A[0][2] = m.A[i][2];
    r.A[1][0] = m.A[j][0]; r.A[1][1] = m.A[j][1]; r.A[1][2] = m.A[j][2];
    r.A[2][0] = m.A[k][0]; r.A[2][1] = m.A[k][1]; r.A[2][2] = m.A[k][2];

    r.b[0] = m.b[0];
    r.b[1] = m.b[1];
    r.b[2] = m.b[2];

    return r;
}

#define ADDINTERCEPT(_axis, _i, _j, _knt) 				\
{									\
    RegIntercept *xcept = (RegIntercept *)DXNewSegListItem(iBuf);	\
    if (! xcept)							\
	goto error;							\
    xcept->axis    = _axis;						\
    xcept->i       = _i;						\
    xcept->j       = _j;						\
    xcept->x.index = _knt;						\
}

#define ADDTRIANGLE(_i, _j, _k)						\
{									\
    int *tri = (int *)DXNewSegListItem(tBuf);				\
    if (! tri)								\
	goto error;							\
    *tri++ = _i;							\
    *tri++ = _j;							\
    *tri++ = _k;							\
}

#define ADDMBUF(k)							\
{									\
    int *m;								\
    m = (int *)DXNewSegListItem(mBuf);					\
    if (! m)								\
	goto error;							\
    *m = k;								\
}

#define ADDINTERCEPTROW(_axis, _index, _base)				\
{									\
    int _k;								\
    for (_k = 0; _k < kknt; _k++)					\
	ADDINTERCEPT(_axis, _index, _k, (_base)+_k);			\
}

#define ADDTRIANGLEROW(_pbase, _cbase)					\
{									\
    int _k, _pb = _pbase, _cb = _cbase;					\
    for (_k = 0; _k < (kknt-1); _k++, _pb++, _cb += slabCubes[2])	\
    {									\
	if (DXIsElementInvalid(icHandle, _cb))				\
	{								\
	    cubesIgnored = 1;						\
	    continue;							\
	}								\
	ADDTRIANGLE(_pb, _pb+kknt, _pb+kknt+1);				\
	ADDTRIANGLE(_pb, _pb+kknt+1, _pb+1);				\
	if (mBuf) {							\
	    ADDMBUF(_cb);						\
	    ADDMBUF(_cb);						\
	}								\
    }									\
}

static Field
MTP_cubes_reg_AxisAligned(Field field, Plane plane, Matrix matrix)
{
    int	           i;
    Array          array;
    int            slabPoints[3];
    int            slabCubes[3];
    int            count[3];
    int	           meshOffsets[3];
    int	           ixMin, jyMin;
    int	           ixMax, jyMax;
    double         aInv, bInv;
    SegList        *iBuf = NULL;
    SegList        *tBuf = NULL;
    SegList        *mBuf = NULL;
    int            permute[3];
    double         xfuzz, yfuzz;
    int	           cubesIgnored = 0;
    double         dydx;
    int	           kknt;
    float          x0, x1, y0, y1;
    double         a, b, c, d;
    RegIntercept   *xcept;
    SegListSegment *slist;
    int		   tot;
    InvalidComponentHandle icHandle = NULL;

    icHandle = DXCreateInvalidComponentHandle((Object)field,
						NULL, "connections");
    if (! icHandle)
	goto error;
    
    array = (Array)DXGetComponentValue(field, "connections");
    DXQueryGridConnections(array, NULL, count);
    DXGetMeshOffsets((MeshArray)array, meshOffsets);

    slabPoints[0] = count[1]*count[2];
    slabPoints[1] = count[2];
    slabPoints[2] = 1;

    slabCubes[0] = (count[1] - 1)*(count[2] - 1);
    slabCubes[1] = (count[2] - 1);
    slabCubes[2] = 1;

    /*
     * Choose the loop ordering so that the innermost loop tends to
     * be as long as possible (remember that we handle each cube that
     * intersects the plane exactly once - this ordering minimizes the 
     * number of loops that have to be initialized).  
     */

    permute[0] = 0; permute[1] = 1; permute[2] = 2;

    a = plane.normal.x;
    if (a < 0) a = -a;
    b = plane.normal.y;
    if (b < 0) b = -b;
    c = plane.normal.z;
    if (c < 0) c = -c;


    if (b > a && b >= c)
    {
	SWAP(double, plane.normal.x, plane.normal.y);
	SWAP(int, count[0], count[1]);
	SWAP(int, meshOffsets[0], meshOffsets[1]);
	SWAP(int, slabPoints[0], slabPoints[1]);
	SWAP(int, slabCubes[0], slabCubes[1]);
	SWAP(int, permute[0], permute[1]);
	SWAP(double, a, b);

    }
    else if (c > a && c > b)
    {
	SWAP(double, plane.normal.x, plane.normal.z);
	SWAP(int, count[0], count[2]);
	SWAP(int, meshOffsets[0], meshOffsets[2]);
	SWAP(int, slabPoints[0], slabPoints[2]);
	SWAP(int, slabCubes[0], slabCubes[2]);
	SWAP(int, permute[0], permute[2]);
	SWAP(double, a, c);
    }

    if (c > b)
    {
	SWAP(double, plane.normal.y, plane.normal.z);
	SWAP(int, count[1], count[2]);
	SWAP(int, meshOffsets[1], meshOffsets[2]);
	SWAP(int, slabPoints[1], slabPoints[2]);
	SWAP(int, slabCubes[1], slabCubes[2]);
	SWAP(int, permute[1], permute[2]);
    }

    matrix = PermuteMatrix(matrix, permute);
	
    if (plane.normal.x != 0)
	aInv = -1.0 / plane.normal.x;
    else
	aInv = 0.0;

    if (plane.normal.y != 0)
	bInv = -1.0 / plane.normal.y;
    else
	bInv = 0.0;

    xfuzz = (plane.normal.x != 0.0) ? FUZZ / plane.normal.x : FUZZ;
    yfuzz = (plane.normal.y != 0.0) ? FUZZ / plane.normal.y : FUZZ;

    xfuzz = fabs(xfuzz);
    yfuzz = fabs(yfuzz);
    
    /*
     * Get the extents of this part of the nesh.
     */
    ixMin = meshOffsets[0];
    ixMax = ixMin + (count[0] - 1);

    jyMin = meshOffsets[1];
    jyMax = jyMin + (count[1] - 1);

    kknt = count[2];

    tBuf = DXNewSegList(3*sizeof(int), 2*(kknt-1), 2*(kknt-1));
    if (! tBuf)
	goto error;
    
    iBuf = DXNewSegList(sizeof(RegIntercept), kknt, kknt);
    if (! tBuf)
	goto error;

    if (DoConnectionsMap(field))
    {
	mBuf = DXNewSegList(sizeof(int), 2*(kknt-1), 2*(kknt-1));
	if (! mBuf)
	    goto error;
    }

    /*
     * Get endpoints of line in XY box
     */
    if (plane.normal.x == 0.0)
    {
	x0 = ixMin;
	x1 = ixMax;
	y0 = plane.offset * bInv;
	SNAP(y0, yfuzz);
	y1 = y0;
	if (y0 < jyMin || y0 >= jyMax)
	    goto return_empty;
    }
    else if (plane.normal.y == 0.0)
    {
	y0 = jyMin;
	y1 = jyMax;
	x0 = plane.offset * aInv;
	SNAP(x0, xfuzz);
	x1 = x0;
	if (x0 < ixMin || x0 >= ixMax)
	    goto return_empty;
    }
    else
    {
	/*
	 * clip line against 4 sides in XY plane
	 */
	x0 = ixMin;
	y0 = (plane.normal.x * x0 + plane.offset) * bInv;
	SNAP(y0, yfuzz);
	if (y0 < jyMin) 
	{
	   y0 = jyMin;
	   x0 = (plane.normal.y * y0 + plane.offset) * aInv;
	   SNAP(x0, xfuzz);
	   if (x0 < ixMin || x0 > ixMax)
	       goto return_empty;
        }
        else if (y0 > jyMax)
        {
	   y0 = jyMax;
	   x0 = (plane.normal.y * y0 + plane.offset) * aInv;
	   SNAP(x0, xfuzz);
	   if (x0 < ixMin || x0 > ixMax)
	       goto return_empty;
	}
	    
	x1 = ixMax;
	y1 = (plane.normal.x * x1 + plane.offset) * bInv;
        SNAP(y1, yfuzz);
	if (y1 < jyMin) 
	{
	   y1 = jyMin;
	   x1 = (plane.normal.y * y1 + plane.offset) * aInv;
	   SNAP(x1, xfuzz);
	   if (x1 < ixMin || x1 > ixMax)
	       goto return_empty;
        }
        else if (y1 > jyMax)
        {
	   y1 = jyMax;
	   x1 = (plane.normal.y * y1 + plane.offset) * aInv;
	   SNAP(x1, xfuzz);
	   if (x1 < ixMin || x1 > ixMax)
	       goto return_empty;
	}
    }

    if ((x0 == x1 && 
	    ((plane.normal.x > 0 && x0 >= ixMax) ||
	     (plane.normal.x < 0 && x0 <= ixMin))) ||
	(y0 == y1 && 
	    ((plane.normal.y > 0 && y0 >= jyMax) ||
	     (plane.normal.y < 0 && y0 <= jyMin))) ||
	(x0 == x1 && y0 == y1))
	    goto return_empty;

    if (x0 == x1)
    {
	int pbase = 0;
	int cbase = ((int)x0 - meshOffsets[0])*slabCubes[0];

	/*
	 * if x0 == x1, its a slice parallel to the YZ plane.
	 */
	ADDINTERCEPTROW(XAXIS, jyMin, pbase);
	
	for (i = jyMin+1; i <= jyMax; i++, pbase += kknt, cbase += slabCubes[1])
	{
	    ADDINTERCEPTROW(XAXIS, i, pbase+kknt);
	    ADDTRIANGLEROW(pbase, cbase);

	}
    }
    else
    {
	int pbase = 0;
	int cbase;
	int ix, iy;

	/*
	 * swap endpoints so we increment up the x axis.
	 */
	if (x0 > x1)
	{
	    float t = y0;
	    y0 = y1;
	    y1 = t;
	    t = x0;
	    x0 = x1;
	    x1 = t;
	}


	dydx = (y1 - y0) / (x1 - x0);

	if (y0 == (int)y0 && dydx < 0)
	{
	    ix = x0 - meshOffsets[0];
	    iy = (y0 - meshOffsets[1]) - 1;
	}
	else
	{
	    ix = x0 - meshOffsets[0];
	    iy = y0 - meshOffsets[1];
	}

	cbase = ix*slabCubes[0] + iy*slabCubes[1];

	/*
	 * Add the initial row of intercepts.  The initial point (x0,y0)
	 * is on the boundary of the XY box, so either x0 is ixMin or
	 * y0 is jyMin or jyMax
	 */
	if (x0 == ixMin)
	    ADDINTERCEPTROW(YAXIS, ixMin, pbase)
	else 
	    ADDINTERCEPTROW(XAXIS, (int)y0, pbase);

	/*
	 * We will do an outer loop for each x-axis interval.
	 * From the current point (x0, y0), we determine the end of
	 * the current interval: either the next integral x value or,
	 * if this exceeds x1, then x1.  Then we determine the
	 * y value at the interval end.  
	 */
	while (x0 != x1 && y0 != y1)
	{
	    float xn, yn;

	    xn = floor(x0 + 1);
	    if (xn >= x1)
	    {
		xn = x1;
		yn = y1;
	    }
	    else
	    {
		yn = y0 + (xn - x0)*dydx;
		SNAP(yn, yfuzz);
	    }

	    /*
	     * Now for each integral y value between y0 and yn, set a row of 
	     * intercepts  and triangles, bumping x0 and y0 appropriately.
	     */
	
	    if (dydx > 0.0)
	    {
		for (y0 = floor(y0+1); y0 < yn; y0 += 1, pbase += kknt)
		{
		    ADDINTERCEPTROW(XAXIS, y0, pbase+kknt);
		    ADDTRIANGLEROW(pbase, cbase);
		    cbase += slabCubes[1];
		}
	    }
	    else
	    {
		for (y0 = ceil(y0-1); y0 > yn; y0 -= 1, pbase += kknt)
		{
		    ADDINTERCEPTROW(XAXIS, y0, pbase+kknt);
		    ADDTRIANGLEROW(pbase, cbase);
		    cbase -= slabCubes[1];
		}
	    }

	    /*
	     * Add the intercepts at the X critical point.  
	     */
	    if (xn == (int)xn)
		ADDINTERCEPTROW(YAXIS, (int)xn, pbase+kknt)
	    else
		ADDINTERCEPTROW(XAXIS, (int)yn, pbase+kknt);

	    ADDTRIANGLEROW(pbase, cbase);
	    pbase += kknt;
	    cbase += slabCubes[0];

	    x0 = xn;
	    y0 = yn;
	}
    }

    array = DXNewArray(TYPE_INT, CATEGORY_REAL, 1, 3);
    if (! array)
	goto error;
    
    if (! DXAllocateArray(array, DXGetSegListItemCount(tBuf)))
    {
	DXDelete((Object)array);
	goto error;
    }
    
    DXInitGetNextSegListSegment(tBuf); tot = 0;
    while (NULL != (slist = DXGetNextSegListSegment(tBuf)))
    {
	int n = DXGetSegListSegmentItemCount(slist);
	if (! DXAddArrayData(array, tot, n,
			DXGetSegListSegmentPointer(slist)))
	{
	    DXDelete((Object)array);
	    goto error;
	}
	tot += n;
    }

    DXSetComponentValue(field, "connections", (Object)array);
    array = NULL;

    if (! DXSetComponentAttribute(field, "connections", "element type",
				(Object)DXNewString("triangles")))
	goto error;
    
    if (! DXInitGetNextSegListItem(iBuf))
	goto error;
    
    a = plane.normal.x;
    b = plane.normal.y;
    c = plane.normal.z;
    d = plane.offset;
    
    while (NULL != (xcept = (RegIntercept *)DXGetNextSegListItem(iBuf)))
    {
	double t;
	int it;
	int I, J;


	switch(xcept->axis)
	{
	    case XAXIS:
		I = (xcept->i - meshOffsets[1]);
		J = xcept->j;
		xcept->j += meshOffsets[2];
		t = (b*xcept->i + d) * aInv;
		SNAP(t, xfuzz);
		xcept->intercept = t;
		it = floor(t);
		if ((it = floor(t)) == t)
		{
		    if (it == ixMin)
		    {
			xcept->x.d = 0.0;
		    }
		    else
		    {
			xcept->x.d = 1.0;
			it  -= 1;
		    }
		}
		else
		    xcept->x.d = t - it;
		xcept->x.i0 = (it - meshOffsets[0])*slabPoints[0]
				    + I*slabPoints[1]
					+ J*slabPoints[2];
		SNAP(xcept->x.i0, xfuzz);
		xcept->x.i1 = xcept->x.i0 + slabPoints[0];
		    break;

	    case YAXIS:
		I = (xcept->i - meshOffsets[0]);
		J = xcept->j;
		xcept->j += meshOffsets[2];
		t = (a*xcept->i + d) * bInv;
		SNAP(t, yfuzz);
		xcept->intercept = t;
		if ((it = floor(t)) == t)
		{
		    if (it == jyMin)
		    {
			xcept->x.d = 0.0;
		    }
		    else
		    {
			xcept->x.d = 1.0;
			it  -= 1;
		    }
		}
		else
		    xcept->x.d = t - it;
		xcept->x.i0 = I*slabPoints[0]
				    + (it - meshOffsets[1])*slabPoints[1]
					+ J*slabPoints[2];
		xcept->x.i1 = xcept->x.i0 + slabPoints[1];
		break;
	}
    }

    if (! MapArrays(field, iBuf, mBuf, &matrix))
	goto error;

    if (DXGetComponentValue(field, "invalid positions"))
	DXDeleteComponent(field, "invalid posititions");

    if (DXGetComponentValue(field, "invalid connections"))
	DXDeleteComponent(field, "invalid connections");

    if (cubesIgnored)
    {
	if (! DXInvalidateUnreferencedPositions((Object)field))
	    goto error;
	
	if (! DXCull((Object)field))
	    goto error;
    }
	
    if (! DXEndField(field))
	goto error;
    
    if (icHandle)
	DXFreeInvalidComponentHandle(icHandle);

    if (iBuf)
	DXDeleteSegList(iBuf);
    
    if (mBuf)
	DXDeleteSegList(mBuf);

    if (tBuf)
	DXDeleteSegList(tBuf);

    return field;

return_empty:

    if (icHandle)
	DXFreeInvalidComponentHandle(icHandle);

    if (iBuf)
	DXDeleteSegList(iBuf);
    
    if (mBuf)
	DXDeleteSegList(mBuf);

    if (tBuf)
	DXDeleteSegList(tBuf);


    {
	char *name;

	while (DXGetEnumeratedComponentValue(field, 0, &name))
	    DXDeleteComponent(field, name);

	if (!DXEndField(field))
	    goto error;
    }

    return field;

error:

    if (icHandle)
	DXFreeInvalidComponentHandle(icHandle);
    DXDeleteSegList(iBuf);
    DXDeleteSegList(mBuf);
    DXDeleteSegList(tBuf);

    return NULL;
}
