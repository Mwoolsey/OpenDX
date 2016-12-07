/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dxmods/_refinetopo.c,v 1.7 2006/07/10 21:39:58 davidt Exp $:
 */

#include <dxconfig.h>

#include <math.h>
#include <ctype.h>
#include <dx/dx.h>
#include "vectors.h"
#include <string.h>
#include "_refine.h"
#include "_newtri.h"

#define ELT_NONE		0
#define ELT_LINES		1
#define ELT_QUADS		2
#define ELT_TRIANGLES		3
#define ELT_CUBES		4
#define ELT_TETRAHEDRA		5

#define PAIR(to,from)		(((to)<<16)|(from))

static Object ChgTopologyObject(Object, int);
static Error  ChgTopologyField(Pointer);

typedef Field (*PFF)();

static Field  None2None(Field);

static Error  PolylinesToLines(Field);

static Field  Cubes2Tetrahedra(Field);
static Field  RegCubes2Tetrahedra(Field);
static Field  IrregCubes2Tetrahedra(Field);

static Field  Quads2Triangles(Field);
static Field  RegQuads2Triangles(Field);
static Field  IrregQuads2Triangles(Field);
static int    GetTypeID(char *);
static char   *GetTypeName(int);

Object
_dxfChgTopology(Object object, char *type)
{
    Object copy;
    int    targetType;

    targetType = GetTypeID(type);
    if (targetType == ELT_NONE)
    {
	DXSetError(ERROR_BAD_PARAMETER, "#11380", type);
	return NULL;
    }

    copy = DXCopy(object, COPY_STRUCTURE);
    if (! copy)
	return NULL;
    
    if (! DXCreateTaskGroup())
	goto error;
    
    if (! ChgTopologyObject(copy, targetType))
	goto error;
    
    if (! DXExecuteTaskGroup() || DXGetError() != ERROR_NONE)
	goto error;
    
    return copy;

error:

    DXDelete(copy);
    return NULL;
}
    
typedef struct
{
    Field field;
    int   targetType;
} ChgTopologyTask;

static Object
ChgTopologyObject(Object object, int targetType)
{
    int             i;
    Object          child;
    ChgTopologyTask task;

    task.targetType = targetType;
    
    switch (DXGetObjectClass(object))
    {
	case CLASS_FIELD:

	    task.field = (Field)object;

	    if (! DXAddTask(ChgTopologyField, (Pointer)&task, sizeof(task), 1.0))
		return NULL;

	    return object;
	
	case CLASS_GROUP:

	    i = 0; 
	    while (NULL != (child=DXGetEnumeratedMember((Group)object,i++,NULL)))
		if (! ChgTopologyObject(child, targetType))
		    return NULL;
	    
	    return object;
	
	case CLASS_XFORM:

	    if (! DXGetXformInfo((Xform)object, &child, 0))
		return NULL;
	    
	    if (! ChgTopologyObject(child, targetType))
		return NULL;
	    else
		return object;
    
	case CLASS_CLIPPED:

	    if (! DXGetClippedInfo((Clipped)object, &child, 0))
		return NULL;
	    
	    if (! ChgTopologyObject(child, targetType))
		return NULL;
	    else
		return object;
    
	default:
	    DXSetError(ERROR_DATA_INVALID, "#11381");
	    return NULL;
    }
}

static Error
ChgTopologyField(Pointer ptr)
{
    Field   field;
    PFF	    method;
    Object  attr;
    char    *srcString;
    int	    srcType, targetType;

    field = ((ChgTopologyTask *)ptr)->field;
    targetType = ((ChgTopologyTask *)ptr)->targetType;

    if (DXEmptyField(field))
	return OK;

    attr = DXGetComponentAttribute(field, "connections", "element type");
    if (! attr)
    {
	if (DXGetComponentValue(field, "faces")) 
	{
	    if (targetType != ELT_TRIANGLES)
	    {
		DXSetError(ERROR_DATA_INVALID,
			"can only refine faces/loops/edges data to triangles");
		goto error;
	    }
	    return _dxfTriangulateField(field);
	}
	else if (DXGetComponentValue(field, "polylines"))
	{
	    if (targetType != ELT_LINES)
	    {
		DXSetError(ERROR_DATA_INVALID,
			"can only refine polylines data to lines");
		goto error;
	    }
	    return PolylinesToLines(field);
	}

	DXSetError(ERROR_MISSING_DATA, "#10255",
				"connections", "element type");
	goto error;
    }

    if (! DXExtractString(attr, &srcString))
    {
	DXSetError(ERROR_DATA_INVALID, "#10200", "element type attribute");
	goto error;
    }
	
    srcType = GetTypeID(srcString);
    if (srcType == ELT_NONE)
    {
	DXSetError(ERROR_BAD_PARAMETER, "#11380", srcString);
	goto error;
    }

    if (srcType != targetType)
    {
	switch(PAIR(srcType, targetType))
	{
	    case PAIR(ELT_NONE,ELT_NONE):
		method = None2None;
		break;
	    
	    case PAIR(ELT_CUBES,ELT_TETRAHEDRA):
		method = Cubes2Tetrahedra;
		break;
	    
	    case PAIR(ELT_QUADS,ELT_TRIANGLES):
		method = Quads2Triangles;
		break;
	    
	    default:
		DXSetError(ERROR_BAD_PARAMETER, "#12120",
					srcString, GetTypeName(targetType));
		goto error;
	}

	if (! (*method)(field))
	    goto error;

	DXDeleteComponent(field, "neighbors");
	if (! DXEndField(field))
	    goto error;
    }

    return OK;

error:
    return ERROR;
}


static int
GetTypeID(char *str)
{

    if (! strcmp(str, "lines"))
	return ELT_LINES;
    else if (! strcmp(str, "quads"))
	return ELT_QUADS;
    else if (! strcmp(str, "triangles"))
	return ELT_TRIANGLES;
    else if (! strcmp(str, "cubes"))
	return ELT_CUBES;
    else if (! strcmp(str, "tetrahedra"))
	return ELT_TETRAHEDRA;
    else
	return ELT_NONE;
}

static char *
GetTypeName(int type)
{
    switch(type)
    {
	case ELT_LINES:
            return "lines";
	case ELT_QUADS:
	    return "quads";
	case ELT_TRIANGLES:
	    return "triangles";
	case ELT_CUBES:
	    return "cubes";
	case ELT_TETRAHEDRA:
	    return "tetrahedra";
	default:
	    return "unknown element type";
    }
}

static Field
None2None(Field field)
{
    return field;
}

static Field
Cubes2Tetrahedra(Field field)
{
    Array cArray;

    if (DXEmptyField(field))
	return field;

    cArray = (Array)DXGetComponentValue(field, "connections");
    if (! cArray)
    {
	DXSetError(ERROR_MISSING_DATA, "#10240", "connections");
	return NULL;
    }

    if (DXQueryGridConnections(cArray, NULL, NULL))
	field = RegCubes2Tetrahedra(field);
    else
	field = IrregCubes2Tetrahedra(field);
    
    if (! field)
	return NULL;
    
    if (! DXSetComponentAttribute(field, "connections", "element type", 
				(Object)DXNewString("tetrahedra")))
	return NULL;
    
    return field;
}

#define TETRA(t, a, b, c, d)	\
{						\
    *(t)++ = (a);		\
    *(t)++ = (b);		\
    *(t)++ = (c);		\
    *(t)++ = (d);		\
}

static Field
RegCubes2Tetrahedra(Field field)
{
    MeshArray mArray;
    Array     cArray, dIn, dOut;
    int       i, j, parity, xB, yB, zB, vPerE;
    int       x, y, z, X, Y, Z, xParity, yParity, zParity;
    int       counts[3], offsets[3];
    int       nCubes, nTetras, *tetras;
    int       inElement;
    Object    attr;
    char      *str, *name;
    InvalidComponentHandle iinvalids = NULL, oinvalids = NULL;

    cArray = NULL;
    dOut = NULL;

    mArray = (MeshArray)DXGetComponentValue(field, "connections");
    if (! mArray)
	return field;

    if (! DXTypeCheckV((Array)mArray, TYPE_INT, CATEGORY_REAL, 1, NULL))
    {
	DXSetError(ERROR_DATA_INVALID, "#11382");
	return NULL;
    }

    DXGetArrayInfo((Array)mArray, &nCubes, NULL, NULL, NULL, &vPerE);

    iinvalids = DXCreateInvalidComponentHandle((Object)field,
						NULL, "connections");
    if (! iinvalids)
	goto error;

    if (vPerE != 8)
    {
	DXSetError(ERROR_DATA_INVALID, "#11004", "cubes", 8);
	goto error;
    }

    DXQueryGridConnections((Array)mArray, NULL, counts);
    DXGetMeshOffsets(mArray, offsets);

    nTetras = 5 * nCubes;

    cArray = DXNewArray(TYPE_INT, CATEGORY_REAL, 1, 4);
    if (! cArray)
        goto error;

    if (! DXAddArrayData(cArray, 0, nTetras, NULL))
        goto error;
    
    if (NULL == (tetras = (int *)DXGetArrayData(cArray)))
        goto error;

    oinvalids = DXCreateInvalidComponentHandle((Object)cArray,
						NULL, "connections");
    if (! oinvalids)
	goto error;

    parity = 0;
    for (i = 0; i < 3; i++)
	if (offsets[i] & 0x01)
	    parity = ! parity;

    Z = 1; 
    Y = counts[2];
    X = counts[1] * counts[2];

    inElement = 0;

    xB = 0;
    xParity = parity;
    for (x = 0; x < (counts[0] - 1); x++)
    {
	yB = xB;
	yParity = xParity;
	for (y = 0; y < (counts[1] - 1); y++)
	{
	    zB = yB;
	    zParity = yParity;
	    for (z = 0; z < (counts[2] - 1); z++, inElement++)
	    {
		if (zParity)
		{
		  TETRA(tetras, zB+X+Y, zB,     zB+Y,     zB+Y+Z);
		  TETRA(tetras, zB+Y+Z, zB,     zB+X+Z,   zB+X+Y);
		  TETRA(tetras, zB+X+Z, zB+Y+Z, zB+Z,     zB);
		  TETRA(tetras, zB+X+Y, zB+X+Z, zB+X,     zB);
		  TETRA(tetras, zB+Y+Z, zB+X+Z, zB+X+Y+Z, zB+X+Y);
		}
		else
		{
		  TETRA(tetras, zB+X+Y+Z, zB+Y,     zB+Y+Z, zB+Z);
		  TETRA(tetras, zB+Y,     zB+X+Y+Z, zB+X+Y, zB+X);
		  TETRA(tetras, zB+X+Y+Z, zB+Z,     zB+X+Z, zB+X);
		  TETRA(tetras, zB+X,     zB+Z,     zB+Y,   zB+X+Y+Z);
		  TETRA(tetras, zB+X,     zB+Z,     zB,     zB+Y);
		}

		if (DXIsElementInvalid(iinvalids, inElement))
		{
		    int j, k = 5 * inElement;
		    for (j = 0; j < 5; j++)
			if (! DXSetElementInvalid(oinvalids, k+j))
			    goto error;
		}

		zParity = ! zParity;
		zB += Z;
	    }

	    yParity = ! yParity;
	    yB += Y;
	}
    
	xParity = ! xParity;
	xB += X;
    }

    if (! DXSaveInvalidComponent(field, oinvalids))
	goto error;
    
    DXFreeInvalidComponentHandle(iinvalids);
    DXFreeInvalidComponentHandle(oinvalids);
    iinvalids = oinvalids = NULL;

    if (! DXSetComponentValue(field, "connections", (Object)cArray))
        goto error;
    
    cArray = NULL;

    j = 0;
    while (NULL != (dIn=(Array)DXGetEnumeratedComponentValue(field, j++, &name)))
    {
	if (! strcmp(name, "invalid connections"))
	    continue;

	if (NULL == (attr = DXGetComponentAttribute(field, name, "dep")))
	    continue;
	
	if (DXGetObjectClass(attr) != CLASS_STRING ||
	    NULL == (str = DXGetString((String)attr)))
	{
	    DXSetError(ERROR_MISSING_DATA, "#10200", "dependency attribute");
	    goto error;
	}

	if (!strcmp(str, "connections") && strcmp(name, "connections"))
	{
	    int       i, size, nD, r, s[32];
	    Type      t;
	    Category  c;
	    char      *dataIn, *dataOut;

	    DXGetArrayInfo(dIn, &nD, &t, &c, &r, s);

	    if (nD != nCubes)
	    {
		DXSetError(ERROR_DATA_INVALID, "#10400",
					    name, "connections");
		goto error;
	    }

	    if (DXQueryConstantArray(dIn, NULL, NULL))
	    {
		dOut = (Array)DXNewConstantArrayV(5*nCubes, 
			DXGetConstantArrayData(dIn), t, c, r, s);
	    }
	    else
	    {
		if (NULL == (dOut = DXNewArrayV(t, c, r, s)))
		    goto error;

		if (! DXAddArrayData(dOut, 0, 5*nCubes, NULL))
		    goto error;
	    
		if (0 == (size = DXGetItemSize(dIn)))
		    goto error;
	    
		dataIn  = (char *)DXGetArrayData(dIn);
		dataOut = (char *)DXGetArrayData(dOut);
		if (! dataIn || ! dataOut)
		    goto error;
	    
		for (i = 0; i < nCubes; i++)
		{
		    for (j = 0; j < 5; j++)
		    {
			memcpy(dataOut, dataIn, size);
			dataOut += size;
		    }

		    dataIn += size;
		}
	    }

	    if (! DXSetComponentValue(field, name, (Object)dOut))
		goto error;
	
	    dOut = NULL;
	}
    }

    return field;

error:
    DXFreeInvalidComponentHandle(iinvalids);
    DXFreeInvalidComponentHandle(oinvalids);
    DXDelete((Object)cArray);
    DXDelete((Object)dOut);
    return NULL;
}

static
double _dxf_TetDeterminant ( int p, int q, int r, int s, Point *geom )
{
   /*
    * Calculate the geometric determinate of a tetrahedron.
    * The expression below is the matrix we are 'determining', with 'g'
    * representing geometry, p,q,r,s representing tetrahedral connection indices.
    *
    * +-----------------------+-----------------------------------------+
    * |   a11  a12  a13  a14  |   1.0      1.0      1.0      1.0        |
    * |   a21  a22  a23  a24  |   g[p].x   g[q].x   g[r].x   g[s].x     |
    * |   a31  a32  a33  a34  |   g[p].y   g[q].y   g[r].y   g[s].y     |
    * |   a41  a42  a43  a44  |   g[p].z   g[q].z   g[r].z   g[s].z     |
    * +-----------------------+-----------------------------------------+
    */

    double  a11, a12, a13, a14,   a21, a22, a23, a24, 
            a31, a32, a33, a34,   a41, a42, a43, a44;

    double  d;

    DXASSERTGOTO ( geom != NULL );

    a11=1.0;           a12=1.0;           a13=1.0;           a14=1.0;
    a21=geom[p].x; a22=geom[q].x; a23=geom[r].x; a24=geom[s].x;
    a31=geom[p].y; a32=geom[q].y; a33=geom[r].y; a34=geom[s].y;
    a41=geom[p].z; a42=geom[q].z; a43=geom[r].z; a44=geom[s].z;

    d = a11*(a22*(a33*a44-a43*a34)-a23*(a32*a44-a42*a34)+a24*(a32*a43-a42*a33))
       -a12*(a21*(a33*a44-a43*a34)-a23*(a31*a44-a41*a34)+a24*(a31*a43-a41*a33))
       +a13*(a21*(a32*a44-a42*a34)-a22*(a31*a44-a41*a34)+a24*(a31*a42-a41*a32))
      -a14*(a21*(a32*a43-a42*a33)-a22*(a31*a43-a41*a33)+a23*(a31*a42-a41*a32));
      
    d = ( fabs(d) < DXD_MIN_FLOAT ) ? 0 : d;
    
    return d;
}

#define TETRA2(nt, t, a, b, c, d, points)	\
{						\
    if(!(a==b || b==c || c==d || a==c || a==d || b==d)) {  \
    	if(_dxf_TetDeterminant ( a, b, c, d, points ) != 0) { \
    		nt++;          \
    		*(t)++ = (a);		\
    		*(t)++ = (b);		\
    		*(t)++ = (c);		\
    		*(t)++ = (d);		\
    	} 				\
    }                   \
}

#define TETDATA(dOut, dIn, dSize, a,b,c,d, points) \
{												\
    if(!(a==b || b==c || c==d || a==c || a==d || b==d)) {  \
    	if(_dxf_TetDeterminant ( a,b,c,d, points ) != 0) { \
			memcpy(dOut, dIn, dSize); 			\
			dOut += dSize; 						\
		} 										\
	} 											\
}

static Field
IrregCubes2Tetrahedra(Field field)
{
    Array  pArray, cArrayIn, cArrayOut, dIn, dOut;
    int    i, j, k, vPerE, nDim;
    int    nCubes, *cubes, *cubeStart;
    int    nTetras, *tetras;
    int    dataSize;
    Point  *positions;
    char   *dataIn, *dataOut;
    Object attr;
    char   *str, *name;
    InvalidComponentHandle iinvalids = NULL, oinvalids = NULL;

    dOut      = NULL;
    cArrayOut = NULL;
    tetras    = NULL;
    dataOut   = NULL;

    pArray   = (Array)DXGetComponentValue(field, "positions");
    cArrayIn = (Array)DXGetComponentValue(field, "connections");

    if (!pArray || ! cArrayIn)
	return field;
	
	DXGetArrayInfo(pArray, NULL, NULL, NULL, NULL, &nDim);

    if (! DXTypeCheckV(pArray, TYPE_FLOAT, CATEGORY_REAL, 1, NULL))
    {
	DXSetError(ERROR_DATA_INVALID, "#11383");
	return NULL;
    }

    if (nDim != 3)
    {
	DXSetError(ERROR_DATA_INVALID, "#11002", "cubes");
	return NULL;
    }
    
    positions = (Point *)DXGetArrayData(pArray);

    if (! DXTypeCheckV((Array)cArrayIn, TYPE_INT, CATEGORY_REAL, 1, NULL))
    {
	DXSetError(ERROR_DATA_INVALID, "#11382");
	return NULL;
    }

    DXGetArrayInfo((Array)cArrayIn, &nCubes, NULL, NULL, NULL, &vPerE);

    if (vPerE != 8)
    {
	DXSetError(ERROR_DATA_INVALID, "#11004", "cubes", 8);
	return NULL;
    }

    iinvalids = DXCreateInvalidComponentHandle((Object)field,
						    NULL, "connections");
    if (! iinvalids)
	goto error;

    cubeStart = cubes = (int *)DXGetArrayData(cArrayIn);

    nTetras = nCubes * 6;

    cArrayOut = DXNewArray(TYPE_INT, CATEGORY_REAL, 1, 4);
    if (! cArrayOut)
        goto error;
    
    if (! DXAddArrayData(cArrayOut, 0, nTetras, NULL))
	goto error;

    oinvalids = DXCreateInvalidComponentHandle((Object)cArrayOut,
                                                NULL, "connections");
    if (! oinvalids)
        goto error;
    
    tetras = (int *)DXGetArrayData(cArrayOut);
    if (! tetras)
	goto error;

	nTetras = 0;
    for (i = 0; i < nCubes; i++)
    {
    	int tAdded = nTetras;
    	
        TETRA2(nTetras, tetras, cubes[0], cubes[3], cubes[5], cubes[1], positions);
        TETRA2(nTetras, tetras, cubes[5], cubes[2], cubes[7], cubes[3], positions);
        TETRA2(nTetras, tetras, cubes[5], cubes[4], cubes[7], cubes[2], positions);
        TETRA2(nTetras, tetras, cubes[5], cubes[3], cubes[0], cubes[2], positions);
        TETRA2(nTetras, tetras, cubes[5], cubes[2], cubes[0], cubes[4], positions);
        TETRA2(nTetras, tetras, cubes[4], cubes[2], cubes[6], cubes[7], positions);

		tAdded = nTetras - tAdded;
		
		if (DXIsElementInvalid(iinvalids, i))
		{
			int j;
	
			for (j = nTetras-tAdded; j < nTetras; j++)
			if (! DXSetElementInvalid(oinvalids, j))
				goto error;
		}

      cubes += 8;
    }
    
    cArrayOut = DXTrimItems(cArrayOut, nTetras);

    j = 0;
    while (NULL != (dIn=(Array)DXGetEnumeratedComponentValue(field, j++, &name)))
    {
	if (! strcmp(name, "invalid connections"))
	    continue;

	if (NULL == (attr = DXGetComponentAttribute(field, name, "dep")))
	    continue;

	if (DXGetObjectClass(attr) != CLASS_STRING ||
	    NULL == (str = DXGetString((String)attr)))
	{
	    DXSetError(ERROR_MISSING_DATA, "#10200", "dependency attribute");
	    goto error;
	}

	if (!strcmp(str, "connections") && strcmp(name, "connections"))
	{
	    int       nD, r, s[32];
	    Type      t;
	    Category  c;

	    DXGetArrayInfo(dIn, &nD, &t, &c, &r, s);

	    if (nD != nCubes)
	    {
		DXSetError(ERROR_DATA_INVALID, "#10400",
					    name, "connections");
		goto error;
	    }

	    if (DXQueryConstantArray(dIn, NULL, NULL))
	    {
		dOut = (Array)DXNewConstantArrayV(nTetras, 
			DXGetConstantArrayData(dIn), t, c, r, s);
	    }
	    else
	    {
		if (NULL == (dOut = DXNewArrayV(t, c, r, s)))
		    goto error;

		if (! DXAddArrayData(dOut, 0, nTetras, NULL))
		    goto error;
	    
		if (0 == (dataSize = DXGetItemSize(dIn)))
		    goto error;
	    
		dataIn  = (char *)DXGetArrayData(dIn);
		dataOut = (char *)DXGetArrayData(dOut);
		if (! dataIn || ! dataOut)
		    goto error;
		
		cubes = cubeStart;
		for (i = 0; i < nCubes; i++)
		{
			TETDATA(dataOut, dataIn, dataSize, cubes[0], cubes[3], cubes[5], cubes[1], positions);
			TETDATA(dataOut, dataIn, dataSize, cubes[5], cubes[2], cubes[7], cubes[3], positions);
			TETDATA(dataOut, dataIn, dataSize, cubes[5], cubes[4], cubes[7], cubes[2], positions);
			TETDATA(dataOut, dataIn, dataSize, cubes[5], cubes[3], cubes[0], cubes[2], positions);
			TETDATA(dataOut, dataIn, dataSize, cubes[5], cubes[2], cubes[0], cubes[4], positions);
			TETDATA(dataOut, dataIn, dataSize, cubes[4], cubes[2], cubes[6], cubes[7], positions);
		    dataIn += dataSize;
		    cubes += 8;
		}
	    }

	    if (! DXSetComponentValue(field, name, (Object)dOut))
		goto error;
	}
    }

    if (! DXSaveInvalidComponent(field, oinvalids))
        goto error;

    DXFreeInvalidComponentHandle(iinvalids);
    DXFreeInvalidComponentHandle(oinvalids);
    iinvalids = oinvalids = NULL;

    if (! DXSetComponentValue(field, "connections", (Object)cArrayOut))
	goto error;

    return field;

error:
    DXFreeInvalidComponentHandle(iinvalids);
    DXFreeInvalidComponentHandle(oinvalids);
    DXDelete((Object)dOut);
    DXDelete((Object)cArrayOut);

    return NULL;
}


static Field
Quads2Triangles(Field field)
{
    Array cArray;

    if (DXEmptyField(field))
	return field;

    cArray = (Array)DXGetComponentValue(field, "connections");
    if (! cArray)
    {
	DXSetError(ERROR_MISSING_DATA, "#10240", "connections");
	return NULL;
    }

    if (DXQueryGridConnections(cArray, NULL, NULL))
	field = RegQuads2Triangles(field);
    else
	field = IrregQuads2Triangles(field);
    
    if (! field)
	return NULL;
    
    if (! DXSetComponentAttribute(field, "connections", "element type", 
				(Object)DXNewString("triangles")))
	return NULL;
    
    return field;
}

#define TRIANGLE(t, a, b, c)	\
{				\
    *(t)++ = (a);		\
    *(t)++ = (b);		\
    *(t)++ = (c);		\
}			

static Field
RegQuads2Triangles(Field field)
{
    MeshArray mArray;
    Array     cArray, dIn, dOut;
    int       j, xB, yB;
    int       x, y, X, Y, vPerE;
    int       offsets[3], counts[3];
    int       nQuads, nTris, *tris;
    int       inElement;
    Object    attr;
    char      *str, *name;
    InvalidComponentHandle iinvalids = NULL, oinvalids = NULL;

    cArray = NULL;
    dOut = NULL;

    mArray = (MeshArray)DXGetComponentValue(field, "connections");
    if (! mArray)
	return field;

    if (! DXTypeCheckV((Array)mArray, TYPE_INT, CATEGORY_REAL, 1, NULL))
    {
	DXSetError(ERROR_DATA_INVALID, "#11382");
	return NULL;
    }

    DXGetArrayInfo((Array)mArray, &nQuads, NULL, NULL, NULL, &vPerE);

    if (vPerE != 4)
    {
	DXSetError(ERROR_DATA_INVALID, "#11004", "cubes", 4);
	return NULL;
    }

    iinvalids = DXCreateInvalidComponentHandle((Object)field,
						    NULL, "connections");
    if (! iinvalids)
	goto error;

    DXQueryGridConnections((Array)mArray, NULL, counts);
    DXGetMeshOffsets(mArray, offsets);

    nTris = 2 * nQuads;

    cArray = DXNewArray(TYPE_INT, CATEGORY_REAL, 1, 3);
    if (! cArray)
        goto error;

    if (! DXAddArrayData(cArray, 0, nTris, NULL))
	goto error;

    oinvalids = DXCreateInvalidComponentHandle((Object)cArray,
						    NULL, "connections");
    if (! oinvalids)
	goto error;
    
    if (NULL == (tris = (int *)DXGetArrayData(cArray)))
	goto error;
    
    inElement = 0;

    Y = 1;
    X = counts[1];

    xB = 0;
    for (x = 0; x < (counts[0] - 1); x++)
    {
	yB = xB;
	for (y = 0; y < (counts[1] - 1); y++, inElement++)
	{
	    TRIANGLE(tris, yB, yB+Y+X, yB+Y);
	    TRIANGLE(tris, yB, yB+X,   yB+Y+X);

	    if (DXIsElementInvalid(iinvalids, inElement))
	    {
		if (! DXSetElementInvalid(oinvalids, inElement<<1))
		    goto error;
		if (! DXSetElementInvalid(oinvalids, (inElement<<1)+1))
		    goto error;
	    }

	    yB += Y;
	}
    
	xB += X;
    }

    if (! DXSaveInvalidComponent(field, oinvalids))
	    goto error;

    DXFreeInvalidComponentHandle(iinvalids);
    DXFreeInvalidComponentHandle(oinvalids);
    iinvalids = oinvalids = NULL;

    if (! DXSetComponentValue(field, "connections", (Object)cArray))
	goto error;
    
    cArray = NULL;

    j = 0;
    while (NULL != (dIn=(Array)DXGetEnumeratedComponentValue(field, j++, &name)))
    {
	if (! strcmp(name, "invalid connections"))
	    continue;

	if (NULL == (attr = DXGetComponentAttribute(field, name, "dep")))
	    continue;
	
	if (DXGetObjectClass(attr) != CLASS_STRING ||
	    NULL == (str = DXGetString((String)attr)))
	{
	    DXSetError(ERROR_MISSING_DATA, "#10051", "dependency attribute");
	    goto error;
	}

	if (! strcmp(str, "connections") && strcmp(name, "connections"))
	{
	    int       i, size, nD, r, s[32];
	    Type      t;
	    Category  c;
	    char      *dataIn, *dataOut;

	    DXGetArrayInfo(dIn, &nD, &t, &c, &r, s);

	    if (nD != nQuads)
	    {
		DXSetError(ERROR_DATA_INVALID, "#10400", name, "connections");
		goto error;
	    }

	    if (DXQueryConstantArray(dIn, NULL, NULL))
	    {
		dOut = (Array)DXNewConstantArrayV(2*nD, 
			DXGetConstantArrayData(dIn), t, c, r, s);
	    }
	    else
	    {
		if (NULL == (dOut = DXNewArrayV(t, c, r, s)))
		    goto error;

		if (! DXAddArrayData(dOut, 0, 2*nD, NULL))
		    goto error;
	    
		if (0 == (size = DXGetItemSize(dIn)))
		    goto error;
	    
		dataIn  = (char *)DXGetArrayData(dIn);
		dataOut = (char *)DXGetArrayData(dOut);
		
		for (i = 0; i < nQuads; i++)
		{
		    memcpy(dataOut, dataIn, size);
		    dataOut += size;
		    memcpy(dataOut, dataIn, size);
		    dataOut += size;

		    dataIn += size;
		}
	    }

	    if (! DXSetComponentValue(field, name, (Object)dOut))
		goto error;
	
	    dOut = NULL;
	}
    }

    return field;

error:
    DXFreeInvalidComponentHandle(iinvalids);
    DXFreeInvalidComponentHandle(oinvalids);
    DXDelete((Object)cArray);
    DXDelete((Object)dOut);
    return NULL;
}

static Field
IrregQuads2Triangles(Field field)
{
    Array  pArray, cArrayIn, cArrayOut, dIn, dOut;
    int    i, j;
    int    nQuads, *quads;
    int    nTris, *tris;
    int    nDim, dataSize, vPerE;
    char   *dataIn, *dataOut;
    Object attr;
    char   *str, *name;
    InvalidComponentHandle iinvalids = NULL, oinvalids = NULL;

    dOut      = NULL;
    cArrayOut = NULL;
    tris      = NULL;
    dataOut   = NULL;

    pArray   = (Array)DXGetComponentValue(field, "positions");
    cArrayIn = (Array)DXGetComponentValue(field, "connections");
    if (! pArray || ! cArrayIn)
	return field;

    if (! DXTypeCheckV((Array)cArrayIn, TYPE_INT, CATEGORY_REAL, 1, NULL))
    {
	DXSetError(ERROR_DATA_INVALID, "#11382");
	return NULL;
    }

    DXGetArrayInfo((Array)cArrayIn, &nQuads, NULL, NULL, NULL, &vPerE);

    if (vPerE != 4)
    {
	DXSetError(ERROR_DATA_INVALID, "#11004", "cubes", 4);
	return NULL;
    }

    DXGetArrayInfo(pArray, NULL, NULL, NULL, NULL, &nDim);

    if (! DXTypeCheckV(pArray, TYPE_FLOAT, CATEGORY_REAL, 1, NULL))
    {
	DXSetError(ERROR_DATA_INVALID, "#11383");
	return NULL;
    }

    if (nDim != 2 && nDim != 3)
    {
	DXSetError(ERROR_DATA_INVALID, "#11002", "cubes");
	return NULL;
    }

    quads = (int *)DXGetArrayData(cArrayIn);

    iinvalids = DXCreateInvalidComponentHandle((Object)field,
                                                    NULL, "connections");
    if (! iinvalids)
        goto error;

    nTris = 2*nQuads;

    cArrayOut = DXNewArray(TYPE_INT, CATEGORY_REAL, 1, 3);
    if (! cArrayOut)
        goto error;
    
    if (! DXAddArrayData(cArrayOut, 0, nTris, NULL))
	goto error;

    /*
     * 3 vertices per tris, 2 tris per quads, nQuads
     */
    tris = (int *)DXGetArrayData(cArrayOut);
    if (! tris)
	goto error;

    oinvalids = DXCreateInvalidComponentHandle((Object)cArrayOut,
                                                NULL, "connections");
    if (! oinvalids)
        goto error;
    
    for (i = 0; i < nQuads; i++)
    {
	TRIANGLE(tris, quads[0], quads[3], quads[1]);
	TRIANGLE(tris, quads[0], quads[2], quads[3]);

	if (DXIsElementInvalid(iinvalids, i))
	{
	    if (! DXSetElementInvalid(oinvalids, i<<1) ||
		! DXSetElementInvalid(oinvalids, (i<<1)+1))
		goto error;
	}

	quads += 4;
    }

    if (! DXSaveInvalidComponent(field, oinvalids))
	goto error;

    DXFreeInvalidComponentHandle(iinvalids);
    DXFreeInvalidComponentHandle(oinvalids);
    iinvalids = oinvalids = NULL;

    if (! DXSetComponentValue(field, "connections", (Object)cArrayOut))
	goto error;

    j = 0;
    while(NULL != (dIn=(Array)DXGetEnumeratedComponentValue(field, j++, &name)))
    {

	if (! strcmp(name, "invalid connections"))
	    continue;

	if (NULL == (attr = DXGetComponentAttribute(field, name, "dep")))
	    continue;
	
	if (DXGetObjectClass(attr) != CLASS_STRING ||
	    NULL == (str = DXGetString((String)attr)))
	{
	    DXSetError(ERROR_MISSING_DATA, "#10051", "dependency attribute");
	    goto error;
	}

	if (! strcmp(str, "connections") && strcmp(name, "connections"))
	{
	    int       nD, r, s[32];
	    Type      t;
	    Category  c;

	    DXGetArrayInfo(dIn, &nD, &t, &c, &r, s);

	    if (nD != nQuads)
	    {
		DXSetError(ERROR_DATA_INVALID, "#10400", name, "connections");
		goto error;
	    }

	    if (DXQueryConstantArray(dIn, NULL, NULL))
	    {
		dOut = (Array)DXNewConstantArrayV(nTris,
			DXGetConstantArrayData(dIn), t, c, r, s);
	    }
	    else
	    {
		if (NULL == (dOut = DXNewArrayV(t, c, r, s)))
		    goto error;
	    
		if (0 == (dataSize = DXGetItemSize(dIn)))
		    goto error;
	    
		dataIn  = (char *)DXGetArrayData(dIn);

		if (! DXAddArrayData(dOut, 0, nTris, NULL))
		    goto error;
		
		dataIn  = (char *)DXGetArrayData(dIn);
		dataOut = (char *)DXGetArrayData(dOut);
		if (! dataIn || ! dataOut)
		    goto error;
		
		for (i = 0; i < nQuads; i++)
		{
		    memcpy(dataOut, dataIn, dataSize);
		    dataOut += dataSize;
		    memcpy(dataOut, dataIn, dataSize);
		    dataOut += dataSize;
		    dataIn += dataSize;
		}
	    }

	    if (! DXSetComponentValue(field, name, (Object)dOut))
		goto error;
	}
    }

    return field;

error:
    DXFreeInvalidComponentHandle(iinvalids);
    DXFreeInvalidComponentHandle(oinvalids);
    DXFree((Pointer)tris);
    DXDelete((Object)dOut);
    DXDelete((Object)cArrayOut);

    return NULL;
}

static Error
PolylinesToLines(Field field)
{
    Array  pA, eA, lA = NULL, inA, outA = NULL;
    int    nP, nE, nL, nA;
    int    *polylines, *edges, *lines;
    int    i, k, p, e, l;
    Object o;
    char   *name;
    InvalidComponentHandle invP = NULL, invL = NULL;

    pA = (Array)DXGetComponentValue(field, "polylines");
    eA = (Array)DXGetComponentValue(field, "edges");
    if (! pA || ! eA)
    {
	DXSetError(ERROR_MISSING_DATA, "polylines or edges component");
	goto error;
    }

    DXGetArrayInfo(pA, &nP, NULL, NULL, NULL, NULL);
    DXGetArrayInfo(eA, &nE, NULL, NULL, NULL, NULL);

    nL = nE - nP;

    lA = DXNewArray(TYPE_INT, CATEGORY_REAL, 1, 2);
    if (! lA)
	goto error;
    
    if (! DXAddArrayData(lA, 0, nL, NULL))
	goto error;

    if (! DXSetAttribute((Object)lA, "element type", (Object)DXNewString("lines")))
	goto error;
    
    if (! DXSetComponentValue(field, "connections", (Object)lA))
	goto error;
    
    polylines = (int *)DXGetArrayData(pA);
    edges     = (int *)DXGetArrayData(eA);
    lines     = (int *)DXGetArrayData(lA);

    invP = DXCreateInvalidComponentHandle((Object)field, NULL, "polylines");
    if (invP)
    {
	invL = 
	    DXCreateInvalidComponentHandle((Object)field, NULL, "connections");

	if (! invL)
	    goto error;
    }

    for (p = l = 0; p < nP; p++)
    {
	int start = polylines[p];
	int end = ((p == nP-1) ? nE : polylines[p+1]) - 1;
	int inv = DXIsElementInvalid(invP, p);

	for (e = start; e < end; e++, lines += 2, l++)
	{
	    lines[0] = edges[e];
	    lines[1] = edges[e+1];

	    if (inv)
		if (! DXSetElementInvalid(invL, l))
		    goto error;
	}
    }

    if (invP)
    {
	DXFreeInvalidComponentHandle(invP);
	invP = NULL;
    
	if (! DXSaveInvalidComponent(field, invL))
	    goto error;
	
	DXFreeInvalidComponentHandle(invL);
	invL = NULL;
    }

    for (i = 0; NULL != (o = DXGetEnumeratedComponentValue(field, i, &name)); i++)
    {
	Type t;
	Category c;
	int  r, s[32];

	if (DXGetObjectClass(o) != CLASS_ARRAY)
	    continue;
	
	if (! strcmp(name, "polylines") || ! strcmp(name, "invalid polylines"))
	    continue;
	
	inA = (Array)o;

	o = DXGetAttribute(o, "dep");
	if (! o || 
	    DXGetObjectClass(o) != CLASS_STRING ||
	    strcmp(DXGetString((String)o), "polylines"))
	{
	    continue;
	}

	DXGetArrayInfo(inA, &nA, &t, &c, &r, s);

	if (nA != nP)
	{
	    DXSetError(ERROR_DATA_INVALID, 
		"%s deps polylines but has a different size");
	    goto error;
	}

	if (DXGetArrayClass(inA) == CLASS_CONSTANTARRAY)
	{
	    outA = (Array)DXNewConstantArray(nL, DXGetConstantArrayData(inA), t, c, r, s);
	    if (! outA)
		goto error;
	}
	else
	{
	    int size;
	    ubyte *inData, *outData;

	    outA = DXNewArrayV(t, c, r, s);
	    if (! outA)
		goto error;
	    
	    if (! DXAddArrayData(outA, 0, nL, NULL))
		goto error;

	    size = DXGetItemSize(inA);

	    inData  = (ubyte *)DXGetArrayData(inA);
	    outData = (ubyte *)DXGetArrayData(outA);

	    for (k = 0; k < nP; k++, inData += size)
	    {
		int start = polylines[k];
		int end = ((k == nP-1) ? nE : polylines[k+1]) - 1;
		int j, knt = end - start;
		for (j = 0; j < knt; j++, outData += size)
		    memcpy(outData, inData, size);
	    }
	}

	if (! DXSetComponentValue(field, name, (Object)outA))
	    goto error;

	if (! DXSetComponentAttribute(field, name, "dep",
					(Object)DXNewString("connections")))
	    goto error;
	
    }

    DXDeleteComponent(field, "polylines");
    DXDeleteComponent(field, "edges");
    DXDeleteComponent(field, "invalid polylines");

    return OK;

error:
    if (invP) DXFreeInvalidComponentHandle(invP);
    if (invL) DXFreeInvalidComponentHandle(invL);
    if (outA) DXDelete((Object)outA);
    if (lA)   DXDelete((Object)lA);

    return ERROR;
}
