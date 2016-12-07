/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


/*
 * $Header: /src/master/dx/src/exec/dxmods/_partnbrs.c,v 1.5 2000/08/24 20:04:16 davidt Exp $
 */

#include <string.h>
#include <dx/dx.h>
#include "_partnbrs.h"

typedef struct node
{
    struct node *next;
    int		 offset;
} Node;

#define MAXDIM	8
#define MAXNODES MAXDIM*64

static Error RegularPartitionNeighbors(CompositeField);
static Error IrregularPartitionNeighbors(CompositeField);

Error _dxfPartitionNeighbors(Object o)
{
    int i;
    Field f;
    Array  c=NULL;
    Class class;

    class = DXGetObjectClass(o);
    if (class == CLASS_GROUP)
	 class = DXGetGroupClass((Group)o);

    if (class == CLASS_FIELD)
	return OK;
    
    if (class != CLASS_COMPOSITEFIELD)
    {
	DXSetError(ERROR_DATA_INVALID, "input");
	return ERROR;
    }

    i = 0;
    while (NULL != (f = (Field)DXGetEnumeratedMember((Group)o, i++, NULL)))
    {
	if (DXEmptyField(f))
	    continue;

	c = (Array)DXGetComponentValue(f, "connections");

	if (c)
	    break;
    }
    
    if (!f)
	return OK;
    
    if (DXQueryGridConnections(c, NULL, NULL))
	return RegularPartitionNeighbors((CompositeField)o);
    else
	return IrregularPartitionNeighbors((CompositeField)o);
}

static Error
RegularPartitionNeighbors(CompositeField cf)
{
    struct node Nodes[MAXNODES];
    int         nextNode = 0;
    struct node heads[MAXDIM];
    int         nDim;
    Field       f;
    int		meshOffsets[MAXDIM];
    int		counts[MAXDIM];
    int		*indices = NULL;
    Array	conn;
    int		nPartitions;
    Field	*fieldList = NULL;
    int		*grid = NULL;
    int		gridSize;
    int		*iPtr;
    int		strides[MAXDIM];
    int		i, j, k, offset;
    Array	pnbrs;
    int		*nbrs;

    for(i=0; NULL != (f = (Field)DXGetEnumeratedMember((Group)cf, i, NULL)); i++)
    {
	conn = (Array)DXGetComponentValue(f, "connections");
	if (! conn)
	    goto error;

	if (i == 0)
	{
	    if (! DXQueryGridConnections(conn, &nDim, NULL))
		goto error;

	    for (j = 0; j < nDim; j++)
	    {
		heads[j].offset = 0; 
		heads[j].next   = NULL; 
		counts[j]	= 1;
	    }
	}

	if (! DXGetMeshOffsets((MeshArray)conn, meshOffsets))
	    goto error;
	
	for (j = 0; j < nDim; j++)
	{
	    Node *l, *n, *new;

	    l = heads + j;
	    n = l->next;

	    while (n != NULL && n->offset <= meshOffsets[j])
	    {
		l = l->next;
		n = n->next;
	    }

	    if (l->offset == meshOffsets[j])
		continue;
	    
	    if (nextNode == MAXNODES)
	    {
		DXSetError(ERROR_INTERNAL, 
			"ran out of nodes in RegularPartitionNeighbors");
		return ERROR;
	    }

	    new = Nodes + nextNode++;

	    new->offset = meshOffsets[j];
	    new->next = n;
	    l->next = new;

	    counts[j]++;
	}
    }

    nPartitions = i;

    gridSize = counts[0];
    for (i = 1; i < nDim; i++)
	gridSize *= counts[i];
    
    fieldList = (Field *)DXAllocateZero(nPartitions * sizeof(Field));
    if (! fieldList)
	goto error;
    
    grid = (int *)DXAllocate(gridSize * sizeof(int));
    if (! grid)
	goto error;
    
    memset(grid, -1, sizeof(grid));
    
    indices = (int *)DXAllocate(nDim*nPartitions*sizeof(int));
    if (! indices)
	goto error;

    strides[0] = 1;
    for (i = 1; i < nDim; i++)
	 strides[i] = strides[i-1] * counts[i-1];

    iPtr = indices;
    for(i=0; NULL != (f = (Field)DXGetEnumeratedMember((Group)cf, i, NULL)); i++)
    {
	fieldList[i] = f;

	conn = (Array)DXGetComponentValue(f, "connections");
	if (! conn)
	    goto error;

	if (! DXGetMeshOffsets((MeshArray)conn, meshOffsets))
	    goto error;
	
	for (j = 0; j < nDim; j++)
	{
	    Node *l;

	    l = heads + j;
	    for (k = 0; k < counts[j]; k++, l = l->next)
		if (l->offset == meshOffsets[j])
		    break;
	    
	    if (k == counts[j])
	    {
		DXSetError(ERROR_INTERNAL, "mesh error");
		goto error;
	    }

	    iPtr[j] = k;
	}

	offset = iPtr[0];
	for (j = 1; j < nDim; j++)
	    offset += iPtr[j]*strides[j];

	grid[offset] = i;
	iPtr += nDim;
    }

    iPtr = indices;
    for (i = 0; i < nPartitions; i++, iPtr += nDim)
    {
	offset = iPtr[0];
	for (j = 1; j < nDim; j++)
	    offset += iPtr[j]*strides[j];
	
	f = fieldList[grid[offset]];

	pnbrs = DXNewArray(TYPE_INT, CATEGORY_REAL, 1, 2);
	if (! pnbrs)
	    goto error;
	
	if (! DXAddArrayData(pnbrs, 0, nDim, NULL))
	    goto error;
	
	if (NULL == (nbrs = (int *)DXGetArrayData(pnbrs)))
	    goto error;
	
	for (j = 0; j < nDim; j++)
	{
	    if (iPtr[j] == 0)
		*nbrs++ = -1;
	    else
		*nbrs++ = (int)grid[offset - strides[j]];
	    
	    if (iPtr[j] == counts[j]-1)
		*nbrs++ = -1;
	    else
		*nbrs++ = (int)grid[offset + strides[j]];
	}

	if (! DXSetComponentValue(f, "partition neighbors", (Object)pnbrs))
	    goto error;
    }

    DXFree((Pointer)grid);
    DXFree((Pointer)fieldList);
    DXFree((Pointer)indices);

    return OK;

error:

    DXFree((Pointer)grid);
    DXFree((Pointer)fieldList);
    DXFree((Pointer)indices);

    return ERROR;
}
	    
struct IPN_pass1_args
{
    Field 	field;
    HashTable   *hPtr;
};

struct IPN_pass2_args
{
    Field 	*fields;
    HashTable   *hashes;
    int		i, n;
};

static Error IPN_pass1(Pointer);
static Error IPN_pass2(Pointer);

static Error
IrregularPartitionNeighbors(CompositeField cf)
{
    Field     			*fieldList = NULL;
    HashTable 			*hashList = NULL;
    int       			i, nPartitions;
    struct IPN_pass1_args	args1;
    struct IPN_pass2_args	args2;

    for (i = 0; NULL != DXGetEnumeratedMember((Group)cf, i, NULL); i++);

    nPartitions = i;
    
    hashList = (HashTable *)DXAllocateZero(nPartitions * sizeof(HashTable));
    if (! hashList)
	goto error;
    
    fieldList = (Field *)DXAllocateZero(nPartitions * sizeof(Field));
    if (! fieldList)
	goto error;

    if (! DXCreateTaskGroup())
	goto error;

    for (i = 0; i < nPartitions; i++)
    {
	fieldList[i] = (Field)DXGetEnumeratedMember((Group)cf, i, NULL);

	args1.field = fieldList[i];
	args1.hPtr  = hashList + i;

	if (! DXAddTask(IPN_pass1, (Pointer)&args1, sizeof(args1), 1.0))
	{
	    DXAbortTaskGroup();
	    goto error;
	}
    }

    if (! DXExecuteTaskGroup() || DXGetError() != ERROR_NONE)
	goto error;
    
    args2.fields = fieldList;
    args2.hashes = hashList;
    args2.n      = nPartitions;

    if (! DXCreateTaskGroup())
	goto error;

    for (i = 0; i < nPartitions; i++)
    {
	args2.i = i;

	if (! DXAddTask(IPN_pass2, (Pointer)&args2, sizeof(args2), 1.0))
	{
	    DXAbortTaskGroup();
	    goto error;
	}
    }

    if (! DXExecuteTaskGroup() || DXGetError() != ERROR_NONE)
	goto error;
    
    for (i = 0; i < nPartitions; i++)
	if (hashList[i])
	    DXDestroyHash(hashList[i]);
	
    DXFree((Pointer)hashList);
    DXFree((Pointer)fieldList);

    return OK;

error:
    
    if (hashList)
    {
	for (i = 0; i < nPartitions; i++)
	    if (hashList[i])
		DXDestroyHash(hashList[i]);
    }
	
    DXFree((Pointer)hashList);
    DXFree((Pointer)fieldList);

    return OK;
}

typedef struct
{
    float point[3];
    int   element;
    int   face;
} HashItem;

static float primes[] =
    { 143669821.0, 45497241659.0, 1455799321.0};

static PseudoKey
FaceHash(Key key)
{
    long l;
    float *p = ((HashItem *)key)->point;
    float q;

    q = (p[0]*primes[0] + p[1]*primes[1] + p[2]*primes[2]);
    l = *(int *)&q;

    return (PseudoKey)l;
}

static int
FaceCmp(Key k0, Key k1)
{
    float *p0 = ((HashItem *)k0)->point;
    float *p1 = ((HashItem *)k0)->point;

    return p0[0] != p1[0] || p0[1] != p1[1] || p0[2] != p1[2];
}

#define SWAP(a,b) {float t=a; a=b; b=t;}	

#define GET_POINT(index,buf)			\
{						\
    if (points)					\
    {						\
	int _i;					\
						\
	float *_src = points + index*nDim;	\
	for (_i = 0; _i < nDim; _i++)		\
	    buf[_i] = *_src++;			\
    }						\
    else					\
    {						\
	int _indices[MAXDIM], _i, _j;		\
	float *_d; 				\
    						\
	GET_INDICES(index,pCounts,nDim);	\
	_d = pDeltas;				\
	for (_i = 0; _i < nDim; _i++)		\
	    buf[_i] = pOrigin[_i];		\
	for (_i = 0; _i < nDim; _i++)		\
	    for (_j = 0; _j < nDim; _j++)	\
		buf[_j] += _indices[_i]*(*_d++);\
    }						\
}

#define GET_ELEMENT(index)			\
{						\
    if (elements)				\
    {						\
	elt = elements + index*nVerts;		\
    }						\
    else					\
    {						\
	int _indices[MAXDIM], _i;		\
	int _l;					\
    						\
	GET_INDICES(index,cCounts,nDim);	\
	for (_i = _l = 0; _i < nDim; _i++)	\
	    _l += _indices[_i]*pstrides[_i];	\
	for (_i = 0; _i < nVerts; _i++)		\
	    eltBuf[_i] = _l + bumps[_i];	\
	elt = eltBuf;				\
    }						\
}

#define GET_INDICES(index,knts,n)		\
{						\
    int _i, _j;					\
    _j = index;					\
    for (_i = (n-1); _i >= 0; _i--)		\
    {						\
	_indices[_i] = _j % knts[_i];		\
	_j = _j / knts[_i];			\
    }						\
}

static void 
SortBuf(float *buf, int nPts, int nDim)
{
    int i, j, k, l;
    float max;
    float *b;

    b = buf;
    for (l = 0; l < nDim; l++, b++)
    {
	for (i = 0; i < nPts; i++)
	{
	    max = b[i*nDim];
	    j = i;
	    for (k = i+1; k < nPts; k++)
		if (b[k*nDim] > max)
		{
		    max = b[k*nDim];
		    j = k;
		}
	    if (i != j)
		SWAP(b[i*nDim], b[j*nDim]);
	}
    }
}

int tetraFaces[] = 
{
    1, 2, 3,
    2, 3, 0,
    3, 0, 1,
    0, 1, 2
};
int nTetraFaces = 4;
int nTetraFaceVertices = 3;

int triangleFaces[] =
{
    1, 2,
    2, 0,
    0, 1
};
int nTriangleFaces = 3;
int nTriangleFaceVertices = 2;

int quadFaces[] =
{
    0, 1,
    2, 3,
    0, 2,
    1, 3,
};
int nQuadFaces = 4;
int nQuadFaceVertices = 2;

int cubeFaces[] =
{
    0, 1, 2, 3,
    4, 5, 6, 7,
    0, 1, 4, 5,
    2, 3, 6, 7,
    0, 4, 2, 6,
    1, 5, 3, 7,
};
int nCubeFaces = 6;
int nCubeFaceVertices = 4;

static Error
IPN_pass1(Pointer args)
{
    int	      i, j, k, l;
    Field     field;
    HashTable hash=NULL;
    Array     cA, pA, nA;
    int       nDim, nVerts;
    int       *elements, *elt, eltBuf[16];
    int       *neighbors, *nPtr;
    float     *points;
    int	      pCounts[MAXDIM];
    float     pOrigin[MAXDIM], pDeltas[MAXDIM*MAXDIM];
    int       cCounts[MAXDIM], nConnections;
    float     buf[MAXDIM*8], *bufPtr;
    Object    attr;
    char      *str;
    int	      *table, nFaces, nFaceVerts;
    HashItem  item;
    int       nExternals;
    int	      pstrides[3];
    int	      cstrides[3];
    int       bumps[32];

    field = ((struct IPN_pass1_args *)args)->field;

    if (DXEmptyField(field))
	return OK;
    
    attr = DXGetComponentAttribute(field, "connections", "element type");
    if (! attr)
    {
	DXSetError(ERROR_MISSING_DATA, "element type attribute");
	goto error;
    }

    if (DXGetObjectClass(attr) != CLASS_STRING)
    {
	DXSetError(ERROR_MISSING_DATA, "element type attribute");
	goto error;
    }

    str = DXGetString((String)attr);
    if (! strcmp(str, "cubes"))
    {
	table      = cubeFaces;
	nFaces     = nCubeFaces;
	nFaceVerts = nCubeFaceVertices;
    }
    else if (! strcmp(str, "tetrahedra"))
    {
	table      = tetraFaces;
	nFaces     = nTetraFaces;
	nFaceVerts = nTetraFaceVertices;
    }
    else if (! strcmp(str, "triangles"))
    {
	table      = triangleFaces;
	nFaces     = nTriangleFaces;
	nFaceVerts = nTriangleFaceVertices;
    }
    else if (! strcmp(str, "quads"))
    {
	table      = quadFaces;
	nFaces     = nQuadFaces;
	nFaceVerts = nQuadFaceVertices;
    }
    else
    {
	DXSetError(ERROR_NOT_IMPLEMENTED, "superneighbors on %s", str);
	goto error;
    }

    hash = DXCreateHash(sizeof(HashItem), FaceHash, FaceCmp);
    if (! hash)
	goto error;

    pA = (Array)DXGetComponentValue(field, "positions");
    cA = (Array)DXGetComponentValue(field, "connections");
    if (! pA || ! cA)
	goto error;
    
    if (DXQueryGridPositions(pA, &nDim, pCounts, pOrigin, pDeltas))
    {
	points = NULL;
    }
    else
    {
	DXGetArrayInfo(pA, NULL, NULL, NULL, NULL, &nDim);
	if (NULL == (points = (float *)DXGetArrayData(pA)))
	    goto error;
    }

    DXGetArrayInfo(cA, &nConnections, NULL, NULL, NULL, &nVerts);
    if (DXQueryGridConnections(cA, NULL, cCounts))
    {
	int incs[3];
	Object attr;

	for (i = 0; i < nDim; i++)
	{
	    pCounts[i] = cCounts[i];
	    cCounts[i] -= 1;
	}

	pstrides[nDim-1] = cstrides[nDim-1] = 1;
	for (i = nDim-2; i >= 0; i--)
	{
	    cstrides[i] = cstrides[i+1] * cCounts[i+1];
	    pstrides[i] = pstrides[i+1] * pCounts[i+1];
	}
	
	for (i = 0; i < (1 << nDim); i++)
	{
	    bumps[i] = 0;
	    for (j = 0; j < nDim; j++)
		if (i & (1 << j))
		    bumps[i] += pstrides[(nDim-1)-j];
	}

	/*
	 * If its a regular grid connections array, we have a warped regular
	 * array.  We need to create a neighbors array since the standard
	 * interface won't.
	 */
	nA = DXNewArray(TYPE_INT, CATEGORY_REAL, 1, nFaces);
	if (! nA)
	    goto error;
	
	if (! DXSetComponentValue(field, "neighbors", (Object)nA))
	{
	    DXDelete((Object)nA);
	    goto error;
	}

	attr = (Object)DXNewString("connections");
	if (! attr)
	    goto error;

	if (! DXSetComponentAttribute(field, "neighbors", "dep", attr))
	    goto error;
	
	if (! DXSetComponentAttribute(field, "neighbors", "ref", attr))
	    goto error;
	
	if (! DXSetComponentAttribute(field, "neighbors", "der", attr))
	    goto error;
	
	if (! DXAddArrayData(nA, 0, nConnections, NULL))
	    goto error;
	
	neighbors = (int *)DXGetArrayData(nA);

	/* 
	 * For each element, create its neighbors list.  The incs array
	 * accumulates the element indices.
	 */
	incs[0] = incs[1] = incs[2] = 0;
	nPtr = neighbors;
	for (i = 0; i < nConnections; i++)
	{
	    /*
	     * For each dimension in x -> z order, if its the first element
	     * set its down neighbor to -1; otherwise to the element indexed
	     * by the same indicies excepting the j-th entry, which is the
	     * same - 1.  Similarly, if is the last...
	     */
	    for (j = 0; j < nDim; j++)
	    {
		if (incs[j] == 0)
		    *nPtr++ = -1;
		else
		{
		    /*
		     * Decrement that index and determine the offset of the
		     * neighbor.  Then restore the index.
		     */
		    incs[j] -= 1;
		    
		    for (k = l = 0; k < nDim; k++)
			l += incs[k] * cstrides[k];
		    
		    *nPtr++ = l;
		    incs[j] += 1;
		}

		if (incs[j] == cCounts[j] - 1)
		    *nPtr++ = -1;
		else
		{
		    /*
		     * Increment that index and determine the offset of the
		     * neighbor.  Then restore the index.
		     */
		    incs[j] += 1;
		    
		    for (k = l = 0; k < nDim; k++)
			l += incs[k] * cstrides[k];
		    
		    *nPtr++ = l;
		    incs[j] -= 1;
		}
	    }

	    for (j = (nDim - 1); j >= 0; j--)
	    {
		incs[j] ++;

		if (incs[j] == (cCounts[j]))
		{
		    if (j == 0 && i != nConnections-1)
		    {
			DXMessage("loop error");
		    }
		    else
		    {
			incs[j] = 0;
		    }
		}
		else
		    break;
	    }
	}

	elements = NULL;
	elt      = eltBuf;
    }
    else
    {
	if (NULL == (elements = (int *)DXGetArrayData(cA)))
	    goto error;

	nA = DXNeighbors(field);
	if (! nA)
	{
	    if (DXGetError() != ERROR_NONE)
		DXSetError(ERROR_INTERNAL,
			"DXNeighbors failed without setting error");
	    goto error;
	}

	if (NULL == (neighbors = (int *)DXGetArrayData(nA)))
	    goto error;
    }

    for (i = 0; i < 3; i++)
	item.point[i] = 0.0;

    nPtr = neighbors; nExternals = 0;
    for (i = 0; i < nConnections; i++)
    {
	int jj;

	GET_ELEMENT(i);

	item.element = i;

	for (jj = 0; jj < nFaces; jj++, nPtr++)
	{
	    if (*nPtr < 0)
	    {
		*nPtr = -1;

		bufPtr = buf;
		for (k = 0; k < nFaceVerts; k++)
		{
		    GET_POINT(elt[table[jj*nFaceVerts+k]], bufPtr);
		    bufPtr += nDim;
		}
		
		SortBuf(buf, nFaceVerts, nDim);
	    
		bufPtr = buf + nDim;
		for (k = 1; k < nFaceVerts; k++)
		    for (l = 0; l < nDim; l++)
			buf[l] += *bufPtr++;
		
		for (k = 0; k < nDim; k++)
		    item.point[k] = buf[k];
		
		item.face = jj;
		
		if (! DXInsertHashElement(hash, (Element)&item))
		    goto error;

		nExternals ++;
	    }
	}
    }

    *(((struct IPN_pass1_args *)args)->hPtr) = hash;

    return OK;

error:
    if (hash)
	DXDestroyHash(hash);
    
    return ERROR;
}

static Error
MinMaxBox(Field f, float *min, float *max)
{
    float box[24], *b, fuzz;
    int   i, j;

    if (! DXBoundingBox((Object)f, (Point *)box))
	return ERROR;

    min[0] = min[1] = min[2] =  DXD_MAX_FLOAT;
    max[0] = max[1] = max[2] = -DXD_MAX_FLOAT;

    b = box;
    for (i = 0; i < 8; i++)
	for (j = 0; j < 3; j++, b++)
	{
	    if (*b < min[j]) min[j] = *b;
	    if (*b > max[j]) max[j] = *b;
	}
    
    for (i = 0; i < 3; i++)
    {
	fuzz = (max[i] - min[i]) * 0.001;
	max[i] += fuzz;
	min[i] -= fuzz;
    }

    return OK;
}

static int
BoxOverlap(float *m0, float *M0, float *m1, float *M1)
{
    if (M1[0] < m0[0]) return 0;
    if (M1[1] < m0[1]) return 0;
    if (M1[2] < m0[2]) return 0;
    if (m1[0] > M0[0]) return 0;
    if (m1[1] > M0[1]) return 0;
    if (m1[2] > M0[2]) return 0;

    return 1;
}

#define BUFSIZE	16

#define FLUSHBUF					\
{							\
    if (! DXAddArrayData(pA, aKnt, bKnt, (Pointer)exBuf)) \
	goto error;					\
    aKnt += bKnt;					\
    bKnt = 0;						\
}

#define ADDEXTERNAL(field, element)			\
{							\
    if (bKnt == BUFSIZE)				\
	FLUSHBUF;					\
    							\
    exBuf[bKnt<<1]     = (int)field;			\
    exBuf[(bKnt<<1)+1] = element;			\
    bKnt ++;						\
}

static Error
IPN_pass2(Pointer ptr)
{
    Field     *fieldList  = ((struct IPN_pass2_args *)ptr)->fields;
    HashTable *hashList   = ((struct IPN_pass2_args *)ptr)->hashes;
    int       nPartitions = ((struct IPN_pass2_args *)ptr)->n;
    int       myPartition = ((struct IPN_pass2_args *)ptr)->i;
    float     myMin[3],  myMax[3];
    float     hisMin[3], hisMax[3];
    HashTable myHash,  hisHash;
    Field     myField, hisField;
    Array     nA, pA = NULL, nnA = NULL;
    int	      *neighbors;
    HashItem  *myItem, *hisItem;
    int	      exBuf[2*BUFSIZE];
    int	      aKnt, bKnt;
    int	      nFaces, nElements;
    int	      i;

    myHash  = hashList[myPartition];
    if (! myHash)
	return OK;

    myField = fieldList[myPartition];

    if (! MinMaxBox(myField, myMin, myMax))
	goto error;
    
    nA = (Array)DXGetComponentValue(myField, "neighbors");
    if (! nA)
    {
	DXSetError(ERROR_MISSING_DATA, "neighbors in pass2");
	goto error;
    }

    DXGetArrayInfo(nA, &nElements, NULL, NULL, NULL, &nFaces);

    pA = DXNewArray(TYPE_INT, CATEGORY_REAL, 1, 2);
    if (! pA)
	goto error;
    
    nnA = DXNewArray(TYPE_INT, CATEGORY_REAL, 1, nFaces);
    if (! nnA)
	goto error;
    
    if (! DXAddArrayData(nnA, 0, nElements, DXGetArrayData(nA)))
	goto error;
    
    neighbors = (int *)DXGetArrayData(nnA);
    if (! neighbors)
	goto error;

    if (! DXSetComponentValue(myField, "neighbors", (Object)nnA))
	goto error;

    nnA = NULL;

    aKnt = bKnt = 0;

    ADDEXTERNAL(-1,-1);
    ADDEXTERNAL(-1,-1);

    for (i = 0; i < nPartitions; i++)
	if (i != myPartition)
	{
	    hisField = fieldList[i];
	    hisHash = hashList[i];

	    if (! hisHash)
		continue;

	    if (! MinMaxBox(hisField, hisMin, hisMax))
		goto error;
	    
	    if (BoxOverlap(myMin, myMax, hisMin, hisMax))
	    {
		hisHash  = hashList[i];
		DXInitGetNextHashElement(myHash);

		while (NULL != (myItem=(HashItem *)DXGetNextHashElement(myHash)))
		    if (NULL !=
			(hisItem=(HashItem *)DXQueryHashElement(hisHash, 
					(Element)myItem)))
		    {
		       neighbors[myItem->element*nFaces+myItem->face]
							    = -(aKnt+bKnt);
		       ADDEXTERNAL(i, hisItem->element);
		    }
	    }
	}
    
    FLUSHBUF;
    
    if (! DXSetComponentValue(myField, "partition neighbors", (Object)pA))
	goto error;

    return OK;

error:

    DXDelete((Object)pA);
    DXDelete((Object)nnA);

    return ERROR;
}
