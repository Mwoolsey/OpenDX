/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


/*
 *  Header: 
 *
 *  Log:    
 */

#include <string.h>
#include <dx/dx.h>

/*
 * We will create a hash table to associate neighbor partitions.
 * This table associates each partition with each of its corners in 
 * IJK space.  Given this table, a partition process can have access 
 * to its neighbors by using its corners as keys into this hash table.
 *
 * The table will be keyed by the IJK point of a partition corner
 * and a tag indicating which corner this is.  This tag contains one bit
 * for each of the dimensions of the data that indicates whether the
 * corresponding delta has been added onto the origin to get the corner
 * point. Thus the origin corner of a 3-D partition is tagged 000, the 
 * corner adjacent along the 0-th dimension is tagged 001, the corner
 * adjacent in the 1-st dimension is tagged 010, the opposite corner in
 * the (0,1) plane is 011 and so on.  
 */
struct partHash
{
    int  	base[3];
    int 	tag;
    Field	partition;
};

/*
 * Structure holding the overlap lengths along an axis
 */
struct lohi
{
    int lo, hi;
};
typedef struct lohi *LoHi;

/*
 * Looping structure for multi-dimensional looping
 */

struct loop
{
    int      length;	/* iteration limit 				      */
    int      inc;	/* iteration count 				      */

    int      srcStart;	/* index of start in source along this dimension      */
    int      srcSkip;	/* source bump size for this dimension		      */
    byte     *srcPtr;	/* source current pointer			      */

    int      dstStart;	/* index of start in destination along this dimension */
    int      dstSkip;	/* destination bump size for this dimension	      */
    byte     *dstPtr;	/* destination current pointer			      */

};

static PseudoKey	PartHash(Key);
static int		PartCmp(Key, Key);

static Error AddPartition(HashTable, Field);
static Error AddPartitionEntry(HashTable, int *, int, Field);
static Error GetIJKBox(Field, int *, int *);
static Field FindNeighbor(HashTable, int *, int);
static Error DestroyPartitionHash(HashTable);

static Array GrowRegularGrid(Array, LoHi);
static Array InitGrowIrregArray(Array, int *, int, int);
static Error GrowPartition(Pointer);
static Field GrowPartition1(HashTable, Field, int, char **, Pointer);
static Field GrowPartition2(HashTable, Field, int, char **, Pointer);
static Field GrowPartition3(HashTable, Field, int, char **, Pointer);
static Error AddOverlapData(Field, int *, int *, Field, int *,
				    LoHi, LoHi, LoHi, int, char **);
static Error FillEmptyOverlap(Field, int *, LoHi, int, char **, Pointer);

static Error ShrinkPartition(Pointer);
static Array ShrinkArray(Array, int *, int *, int *, int, int);
static Array ShrinkRegularArray(RegularArray, int, int, int);
static Array ShrinkIrregularArray(Array, int *, int *, int *, int, int);
static Array ShrinkProductArray(ProductArray, int *, int *, int *, int, int);
extern Array _dxfReRef(Array, int);
static int   DoIt(char *, char **);
static Error GetDepRef(Array, int *, int *);

#if 0
static Array GrowRegularArray(Array, int *, int, int);
#endif

extern Error _dxf_RemoveDupReferences(Field);

#define DEP_ON_POSITIONS	1
#define DEP_ON_CONNECTIONS	2
#define NOT_DEP			3

#define REFS_POSITIONS		1
#define REFS_CONNECTIONS	2
#define NOT_REFS		3

Field DXQueryOriginalMeshExtents(Field, int *, int *);

#define GetEArray(f,i,n) \
	(Array)DXGetEnumeratedComponentValue((f), (i), (n))

#define GetAttr(f,n,a) \
    DXGetComponentAttribute((f), (n), (a)) ?                              \
        DXGetString((String)DXGetComponentAttribute((f), (n), (a))) :       \
        NULL;

#define GetEAttr(f,i,n,a) \
    DXGetEnumeratedComponentAttribute((f), (i), (n), (a))) ?              \
	DXGetString((String)DXGetEnumeratedComponentAttribute((f),(i),(n),(a))) : \
        NULL;

#define SetOrigName(buf, name)  sprintf((buf), "original %s", (name))

struct regGrowTask
{
    Field 	partition;
    HashTable	hashTable;
    int		nRings;
    Array	compArray;
    Pointer	fill;
};

Object
_dxfRegGrow(Object object, int nRings, Pointer fill,  Array compArray)
{
    int 		i;
    Group		group;
    Field		child;
    HashTable		hashTable;
    struct regGrowTask  task;
    Object		hashCopy;

    hashCopy  = NULL;
    hashTable = NULL;

    if (DXGetObjectClass(object) != CLASS_GROUP)
    {
	DXSetError(ERROR_DATA_INVALID, 
			"_dxfRegGrow called with object of wrong class");
	return NULL;
    }

    group = (Group)object;

    hashCopy = DXCopy(object, COPY_STRUCTURE);
    if (! hashCopy)
	goto error;

    hashTable = DXCreateHash(sizeof(struct partHash), PartHash, PartCmp);
    if (! hashTable)
	return NULL;

    i = 0;
    while (NULL != (child = 
		(Field)DXGetEnumeratedMember((Group)hashCopy, i++, NULL)))
	if (! AddPartition(hashTable, child))
	    goto error;

    group = (Group)object;

    if (! DXCreateTaskGroup())
	goto error;

    task.hashTable   = hashTable;
    task.nRings	     = nRings;
    task.compArray   = compArray;
    task.fill        = fill;

    i = 0; 
    while (NULL != (child = (Field)DXGetEnumeratedMember(group, i++, NULL)))
    {
	task.partition = (Field)child;

	if (! DXAddTask(GrowPartition, (Pointer)&task, sizeof(task), 1.0))
	{
	    DXAbortTaskGroup();
	    goto error;
	}
    }

    if (! DXExecuteTaskGroup() || DXGetError() != ERROR_NONE)
	goto error;
    
    DXDelete(hashCopy);

    DestroyPartitionHash(hashTable);
    return (Object)object;

error:
    DXDelete(hashCopy);
    DestroyPartitionHash(hashTable);
    return NULL;
}

struct regShrinkTask
{
    Field 	partition;
};

Object
_dxfRegShrink(Object object)
{
    int 		 i;
    Group		 group;
    Field		 child;
    struct regShrinkTask task;

    switch(DXGetObjectClass(object))
    {
	case CLASS_FIELD:
	    if (ShrinkPartition((Pointer)&object) == OK)
		return object;
	    else
		return NULL;

	case CLASS_GROUP:
	    if (DXGetGroupClass((Group)object) == CLASS_COMPOSITEFIELD)
		break;
	    
	default:
	{
	    DXSetError(ERROR_DATA_INVALID, 
			"_dxfRegGrow called with object of wrong class");
	    return NULL;
	}
    }

    group = (Group)object;

    if (! DXCreateTaskGroup())
	goto error;

    i = 0; 
    while (NULL != (child = (Field)DXGetEnumeratedMember(group, i++, NULL)))
    {
	task.partition = (Field)child;

	if (! DXAddTask(ShrinkPartition, (Pointer)&task, sizeof(task), 1.0))
	{
	    DXAbortTaskGroup();
	    goto error;
	}
    }

    if (! DXExecuteTaskGroup() || DXGetError() != ERROR_NONE)
	goto error;
    
    return (Object)object;

error:

    return NULL;
}

static Error
DestroyPartitionHash(HashTable ht)
{
    struct partHash *ph;

    if (ht == NULL)
	return OK;

    DXInitGetNextHashElement(ht);
    while (NULL != (ph = (struct partHash *)DXGetNextHashElement(ht)))
	DXDelete((Object)(ph->partition));
    
    DXDestroyHash(ht);

    return OK;
}

static Error
GrowPartition(Pointer ptr)
{
    HashTable ht;
    Field     partition;
    int       nRings;
    Object    attr;
    char      *str;
    char      *components[256];
    Pointer   fill;
    Array     compArray;

    ht 	        = ((struct regGrowTask *)ptr)->hashTable;
    partition   = ((struct regGrowTask *)ptr)->partition;
    nRings      = ((struct regGrowTask *)ptr)->nRings;
    compArray   = ((struct regGrowTask *)ptr)->compArray;
    fill        = ((struct regGrowTask *)ptr)->fill;

    if (compArray)
    {
	char *s;
	int i, nComponents, len;

	DXGetArrayInfo(compArray, &nComponents, NULL, NULL, NULL, &len);
	s = (char *)DXGetArrayData(compArray);
	for (i = 0; i < nComponents; i++)
	{
	    components[i] = s;
	    s += len;
	}
	components[i] = NULL;
    }
    else
	components[0] = NULL;

    if (DXEmptyField(partition))
	return OK;

    attr = DXGetComponentAttribute(partition, "connections", "element type");
    if (! attr)
    {
	DXSetError(ERROR_MISSING_DATA, "no element type specified");
	goto error;
    }

    str = DXGetString((String)attr);
    if (! str)
    {
	DXSetError(ERROR_DATA_INVALID, "bad element type specification");
	goto error;
    }

    if (! strcmp(str, "lines"))
    {
	if (! GrowPartition1(ht, partition, nRings, components, fill))
	    goto error;
    }
    else if (! strcmp(str, "quads"))
    {
	if (! GrowPartition2(ht, partition, nRings, components, fill))
	    goto error;
    }
    else if (! strcmp(str, "cubes"))
    {
	if (! GrowPartition3(ht, partition, nRings, components, fill))
	    goto error;
    }
    else
    {
	DXSetError(ERROR_NOT_IMPLEMENTED, 
	    "Cannot grow regular connections of element type %s", str);
	goto error;
    }

    if (! _dxf_RemoveDupReferences(partition))
	goto error;
    
    if (! DXEndField(partition))
	goto error;

    return OK;

error:

    return ERROR;
}

static int cornerIndices1[3] = {0, 0, 4};
static int tags1[3]          = {4, 0, 0};

static Field
GrowPartition1(HashTable ht, Field field, 
		int nRings, char **components, Pointer fill)
{
    int   nCount, oCount;
    Array oArray;
    int   i;
    int   box[8][3];
    Field neighbors[3];
    struct lohi growth, overlap, extra;
    int   cornerIndex, tag;
    int   *corner;
    int   meshOrigin;
    int   index[1];

    if (DXEmptyField(field))
	return field;
    
    if (! GetIJKBox(field, (int *)box, NULL))
	return NULL;

    oArray = (Array)DXGetComponentValue(field, "connections");
    if (! DXQueryGridConnections(oArray, NULL, &oCount))
    {
	DXSetError(ERROR_INTERNAL, "regular connections expected");
	return NULL;
    }

    /*
     * Get pointers to neighbors
     */
    overlap.lo = overlap.hi = 0;
    for (i = 0; i < 3; i++)
    {
	cornerIndex = cornerIndices1[i];
	corner      = (int *)(box + cornerIndex);	
	tag	    = tags1[i];

	neighbors[i] = FindNeighbor(ht, corner, tag);

	if (neighbors[i])
	{
	    Array cA;
	    int   count;

	    cA = (Array)DXGetComponentValue(neighbors[i], "connections");
	    if (! cA)
		return ERROR;

	    if (! DXQueryGridConnections(cA, NULL, &count))
		return ERROR;
		
	    if (i == 0)
	    {
		if (count-1 < nRings)
			overlap.lo = count-1;
		    else
			overlap.lo = nRings;
	    }
	    else if (i == 2)
	    {
		if (count-1 < nRings)
		    overlap.hi = count-1;
		else
		    overlap.hi = nRings;
	    }

	}
    }
    
    /*
     * If fill is not NULL, then we generate complete growth area.
     * Otherwise, only generate overlap area where it overlaps a neighbor.
     */
    if (fill)
    {
	extra.lo = nRings - overlap.lo;
	extra.hi = nRings - overlap.hi;
	growth.lo = nRings;
	growth.hi = nRings;
    }
    else
    {
	extra.lo = 0;
	extra.hi = 0;
	growth.lo = overlap.lo;
	growth.hi = overlap.hi;
    }

    nCount     = oCount + (growth.lo + growth.hi);
    meshOrigin = box[0][0] - growth.lo;
    index[0]   = 1;

    if (! AddOverlapData(field, &nCount, &meshOrigin, neighbors[1], index,
			    &growth, &overlap, &extra, 1, components))
	goto GrowPartition1_error;

    neighbors[1] = NULL;
    
    /*
     * Now for each neighboring partition, add in the irregular data from that
     * partition
     */
    for (i = 0; i < 3; i++)
    {
	if (neighbors[i])
	    if (! AddOverlapData(field, &nCount, &meshOrigin, neighbors[i], &i,
				    &growth, &overlap, &extra, 1, components))
		goto GrowPartition1_error;
    }

    /*
     * If fill was requested, now's the time.
     */
    if (fill != NULL && fill != GROW_NOFILL)
    {

	if (extra.lo)
	{
	    i = 0; 
	    FillEmptyOverlap(field, &i, &extra, 1, components, fill);
	}
	if (extra.hi)
	{
	    i = 2; 
	    FillEmptyOverlap(field, &i, &extra, 1, components, fill);
	}
    }

    DXDeleteComponent(field, "data statistics");

    return field;

GrowPartition1_error:

    return NULL;
}

static int cornerIndices2[3][3] = { {0, 0, 2}, {0, 0, 2}, {4, 4, 6} };
static int tags2[3][3]          = { {6, 4, 4}, {2, 0, 0}, {2, 0, 0} };

static Field
GrowPartition2(HashTable ht, Field field, 
		int nRings, char **components, Pointer fill)
{
    int   nCounts[2], oCounts[2];
    int	  indices[2];
    Array oArray;
    int   i, j;
    int   box[8][3];
    Field neighbors[3][3];
    struct lohi growth[2], overlap[2], extra[2];
    int   cornerIndex, tag;
    int   *corner;
    int   meshOrigins[2];

    if (DXEmptyField(field))
	return field;
    
    if (! GetIJKBox(field, (int *)box, NULL))
	return NULL;

    oArray = (Array)DXGetComponentValue(field, "connections");
    if (! DXQueryGridConnections(oArray, NULL, oCounts))
    {
	DXSetError(ERROR_INTERNAL, "regular connections expected");
	return NULL;
    }

    /*
     * Get pointers to neighbors
     */
    overlap[0].lo = overlap[0].hi = 0;
    overlap[1].lo = overlap[1].hi = 0;
    for (i = 0; i < 3; i++)
	for (j = 0; j < 3; j++)
	{
	    cornerIndex = cornerIndices2[(i)][(j)];
	    corner      = (int *)(box + cornerIndex);	
	    tag	        = tags2[(i)][(j)];

	    neighbors[i][j] = FindNeighbor(ht, corner, tag);

	    if (neighbors[i][j])
	    {
		Array cA;
		int   counts[2];

		cA = (Array)DXGetComponentValue(neighbors[i][j], "connections");
		if (! cA)
		    return ERROR;

		if (! DXQueryGridConnections(cA, NULL, counts))
		    return ERROR;
		
		if (i == 0)
		{
		    if (counts[0]-1 < nRings)
			overlap[0].lo = counts[0]-1;
		    else
			overlap[0].lo = nRings;
		}
		else if (i == 2)
		{
		    if (counts[0]-1 < nRings)
			overlap[0].hi = counts[0]-1;
		    else
			overlap[0].hi = nRings;
		}

		if (j == 0)
		{
		    if (counts[1]-1 < nRings)
			overlap[1].lo = counts[1]-1;
		    else
			overlap[1].lo = nRings;
		}
		else if (j == 2)
		{
		    if (counts[1]-1 < nRings)
			overlap[1].hi = counts[1]-1;
		    else
			overlap[1].hi = nRings;
		}
	    }
	}
    
    /*
     * If fill is not NULL, then we generate complete growth area.
     * Otherwise, only generate overlap area where it overlaps a neighbor.
     */
    if (fill)
    {
	for (i = 0; i < 2; i++)
	{
	    extra[i].lo = nRings - overlap[i].lo;
	    extra[i].hi = nRings - overlap[i].hi;
	    growth[i].lo = nRings;
	    growth[i].hi = nRings;
	}
    }
    else
    {
	for (i = 0; i < 2; i++)
	{
	    extra[i].lo = 0;
	    extra[i].hi = 0;
	    growth[i].lo = overlap[i].lo;
	    growth[i].hi = overlap[i].hi;
	}
    }

    for (i = 0; i < 2; i++)
    {
	nCounts[i] = oCounts[i] + (growth[i].lo + growth[i].hi);
	meshOrigins[i] = box[i][i] - growth[i].lo;
    }

    indices[0] = indices[1] = 1;
    if (! AddOverlapData(field, nCounts, meshOrigins,
			    neighbors[1][1], indices,
			    growth, overlap, extra, 2, components))
	goto GrowPartition2_error;
    
    neighbors[1][1] = NULL;

    /*
     * Now for each neighboring partition, add in the irregular data from that
     * partition
     */
    for (i = 0; i < 3; i++)
    {
	indices[0] = i;

	for (j = 0; j < 3; j++)
	{
	    indices[1] = j;

	    if ((i != 1 || j != 1) && neighbors[i][j])
		if (! AddOverlapData(field, nCounts, meshOrigins,
					neighbors[i][j], indices,
					growth, overlap, extra, 2, components))
		    goto GrowPartition2_error;
	}
    }

    /*
     * If fill was requested, now's the time.
     */
    if (fill != NULL && fill != GROW_NOFILL)
    {
	if (extra[0].lo)
	{
	    indices[0] = 0; indices[1] = 1;
	    FillEmptyOverlap(field, indices, extra, 2, components, fill);
	}
	if (extra[0].hi)
	{
	    indices[0] = 2; indices[1] = 1;
	    FillEmptyOverlap(field, indices, extra, 2, components, fill);
	}
	if (extra[1].lo)
	{
	    indices[0] = 1; indices[1] = 0;
	    FillEmptyOverlap(field, indices, extra, 2, components, fill);
	}
	if (extra[1].hi)
	{
	    indices[0] = 1; indices[1] = 2;
	    FillEmptyOverlap(field, indices, extra, 2, components, fill);
	}
    }

    DXDeleteComponent(field, "data statistics");

    return field;

GrowPartition2_error:


    return NULL;
}

static int cornerIndices3[3][3][3] =
    {
       { {0, 0, 1}, {0, 0, 1}, {2, 2, 3} },
       { {0, 0, 1}, {0, 0, 1}, {2, 2, 3} },
       { {4, 4, 5}, {4, 4, 5}, {6, 6, 7} }
    };

static int tags3[3][3][3] =
    {
	{ {7, 6, 6}, {5, 4, 4}, {5, 4, 4} },
	{ {3, 2, 2}, {1, 0, 0}, {1, 0, 0} },
	{ {3, 2, 2}, {1, 0, 0}, {1, 0, 0} }
    };

static Field
GrowPartition3(HashTable ht, Field field, int nRings, 
			char **components, Pointer fill)
{
    int   nCounts[3], oCounts[3];
    int	  indices[3];
    Array oArray;
    int   i, j, k;
    int   box[8][3];
    Field neighbors[3][3][3];
    struct lohi growth[3], overlap[3], extra[3];
    int   cornerIndex, tag;
    int   *corner;
    int	  meshOrigins[3];

    if (DXEmptyField(field))
	return field;
    
    if (! GetIJKBox(field, (int *)box, NULL))
	return NULL;

    oArray = (Array)DXGetComponentValue(field, "connections");
    if (! DXQueryGridConnections(oArray, NULL, oCounts))
    {
	DXSetError(ERROR_INTERNAL, "regular connections expected");
	return NULL;
    }

    /*
     * Get pointers to neighbors
     */
    overlap[0].lo = overlap[0].hi = 0;
    overlap[1].lo = overlap[1].hi = 0;
    overlap[2].lo = overlap[2].hi = 0;
    for (i = 0; i < 3; i++)
	for (j = 0; j < 3; j++)
	    for (k = 0; k < 3; k++)
	    {
		cornerIndex = cornerIndices3[i][j][k];
		corner      = (int *)(box + cornerIndex);	
		tag	    = tags3[i][j][k];

		neighbors[i][j][k] = FindNeighbor(ht, corner, tag);
		if (neighbors[i][j][k])
		{
		    Array cA;
		    int   counts[3];

		    cA = (Array)DXGetComponentValue(neighbors[i][j][k], 
							"connections");
		    if (! cA)
			return ERROR;

		    if (! DXQueryGridConnections(cA, NULL, counts))
			return ERROR;
		
		    if (i == 0)
		    {
			if (counts[0]-1 < nRings)
			    overlap[0].lo = counts[0]-1;
			else
			    overlap[0].lo = nRings;
		    }
		    else if (i == 2)
		    {
			if (counts[0]-1 < nRings)
			    overlap[0].hi = counts[0]-1;
			else
			    overlap[0].hi = nRings;
		    }

		    if (j == 0)
		    {
			if (counts[1]-1 < nRings)
			    overlap[1].lo = counts[1]-1;
			else
			    overlap[1].lo = nRings;
		    }
		    else if (j == 2)
		    {
			if (counts[1]-1 < nRings)
			    overlap[1].hi = counts[1]-1;
			else
			    overlap[1].hi = nRings;
		    }

		    if (k == 0)
		    {
			if (counts[2]-1 < nRings)
			    overlap[2].lo = counts[2]-1;
			else
			    overlap[2].lo = nRings;
		    }
		    else if (k == 2)
		    {
			if (counts[2]-1 < nRings)
			    overlap[2].hi = counts[2]-1;
			else
			    overlap[2].hi = nRings;
		    }
		}
	    }

    /*
     * If fill is not NULL, then we generate complete growth area.
     * Otherwise, only generate overlap area where it overlaps a neighbor.
     */
    if (fill)
    {
	for (i = 0; i < 3; i++)
	{
	    extra[i].lo = nRings - overlap[i].lo;
	    extra[i].hi = nRings - overlap[i].hi;
	    growth[i].lo = nRings;
	    growth[i].hi = nRings;
	}
    }
    else
    {
	for (i = 0; i < 3; i++)
	{
	    extra[i].lo = 0;
	    extra[i].hi = 0;
	    growth[i].lo = overlap[i].lo;
	    growth[i].hi = overlap[i].hi;
	}
    }

    for (i = 0; i < 3; i++)
    {
	nCounts[i]     = oCounts[i] + (growth[i].lo + growth[i].hi);
	meshOrigins[i] = box[i][i] - growth[i].lo;
	indices[i]     = 1;
    }

    if (! AddOverlapData(field, nCounts, meshOrigins,
			neighbors[1][1][1], indices,
			growth, overlap, extra, 3, components))
	goto GrowPartition3_error;
    
    neighbors[1][1][1] = NULL;

    /*
     * Now for each neighboring partition, add in the irregular data from that
     * partition
     */
    for (i = 0; i < 3; i++)
    {
	indices[0] = i;

	for (j = 0; j < 3; j++)
	{
	    indices[1] = j;

	    for (k = 0; k < 3; k++)
	    {
		indices[2] = k;

		if (neighbors[i][j][k])
		    if (! AddOverlapData(field, nCounts, meshOrigins,
					neighbors[i][j][k], indices,
					growth, overlap, extra, 3, components))
		    goto GrowPartition3_error;
	    }
	}
    }

    /*
     * If fill was requested, now's the time.
     */
    if (fill != NULL && fill != GROW_NOFILL)
    {
	if (extra[0].lo)
	{
	    indices[0] = 0; indices[1] = 1; indices[2] = 1;
	    FillEmptyOverlap(field, indices, extra, 3, components, fill);
	}
	if (extra[0].hi)
	{
	    indices[0] = 2; indices[1] = 1; indices[2] = 1;
	    FillEmptyOverlap(field, indices, extra, 3, components, fill);
	}
	if (extra[1].lo)
	{
	    indices[0] = 1; indices[1] = 0; indices[2] = 1;
	    FillEmptyOverlap(field, indices, extra, 3, components, fill);
	}
	if (extra[1].hi)
	{
	    indices[0] = 1; indices[1] = 2; indices[2] = 1;
	    FillEmptyOverlap(field, indices, extra, 3, components, fill);
	}
	if (extra[2].lo)
	{
	    indices[0] = 1; indices[1] = 1; indices[2] = 0;
	    FillEmptyOverlap(field, indices, extra, 3, components, fill);
	}
	if (extra[2].hi)
	{
	    indices[0] = 1; indices[1] = 1; indices[2] = 2;
	    FillEmptyOverlap(field, indices, extra, 3, components, fill);
	}
    }
    DXDeleteComponent(field, "data statistics");

    return field;

GrowPartition3_error:

    return NULL;
}

#define INDICES(ref, indices, strides, ndim)		\
{							\
    int _i, _j = (ref);					\
    for (_i = 0; _i < (ndim)-1; _i++) {			\
	(indices)[_i] = _j / (strides)[_i];		\
	_j = _j % (strides)[_i];			\
    }							\
    (indices)[_i] = _j;					\
}

#define REFERENCE(indices, ref, strides, ndim) 		\
{							\
    int _i, _j = 0;					\
    for (_i = 0; _i < (ndim)-1; _i++)			\
	_j += (indices)[_i]*(strides)[_i];		\
    (ref) = _j + (indices)[(ndim)-1];			\
}

#define OFFSET(indices, offsets, ndim)			\
{							\
    int _i;						\
    for (_i = 0; _i < (ndim); _i++)			\
	(indices)[_i] += (offsets)[_i];			\
}

#define IS_IN(in, indices, min, max, ndim)		\
{							\
    int _i;						\
    for ((in) = 1, _i = 0; (in) && _i < (ndim); _i++)	\
	(in) = ((indices)[_i] >= (min)[_i]) && 		\
		    ((indices)[_i] <= (max)[_i]);	\
}

static Error
ShrinkPartition(Pointer ptr)
{
    Field       partition;
    Array       array, oArray, nArray = NULL, rArray = NULL;
    int   	rOffsets[8], oOffsets[8], pSizes[8], gSizes[8];
    int		oLength = 0, length;
    int	  	nPositions, nConnections;
    char 	*name;
    int 	i, nDim;
    char  	origName[128];
    char  	*attr;
    int		scstrides[3], spstrides[3], *sstrides = NULL;
    int		dcstrides[3], dpstrides[3], *dstrides = NULL;
    int		pmaxi[3], cmaxi[3], *maxi = NULL;
    int		dep, ref;
    char	*toShrink[256], *toReplace[256];
    int 	 nS, nR;

    partition  = ((struct regShrinkTask *)ptr)->partition;

    if (DXEmptyField(partition))
	return OK;

    array = (Array)DXGetComponentValue(partition, "connections");
    if (! array)
    {
	DXSetError(ERROR_MISSING_DATA, "connections missing");
	goto error;
    }

    if (! DXQueryGridConnections(array, &nDim, gSizes))
    {
	DXSetError(ERROR_MISSING_DATA, "connections should be regular");
	goto error;
    }

    if (! DXQueryOriginalMeshExtents(partition, oOffsets, pSizes))
	return OK;
    
    if (! DXQueryOriginalSizes(partition, &nPositions, &nConnections))
	return ERROR;
    
    scstrides[nDim-1] = 1;
    spstrides[nDim-1] = 1;
    dcstrides[nDim-1] = 1;
    dpstrides[nDim-1] = 1;
    for (i = nDim-2; i >= 0; i --)
    {
	scstrides[i] = scstrides[i+1]*(gSizes[i+1]-1);
	spstrides[i] = spstrides[i+1]*gSizes[i+1];
	dcstrides[i] = dcstrides[i+1]*(pSizes[i+1]-1);
	dpstrides[i] = dpstrides[i+1]*pSizes[i+1];
    }

    for (i = 0; i < nDim; i++)
    {
	cmaxi[i] = pSizes[i] - 1;
	pmaxi[i] = pSizes[i];

	rOffsets[i] = -oOffsets[i];
    }

    /*
     * Go through the components of the input field.  For each component
     * determine if we are to shrink it, replace it with the pre-grown
     * original, or ignore it altogether.
     */
    nS = nR = 0;
    for (i = 0; NULL != (array = GetEArray(partition, i, &name)); i++)
	if (strncmp(name, "original", 8))
	{
		if(nR > 255 || nS > 255)
		{
			DXSetError(ERROR_INTERNAL, "reggrow out of memory");
			goto error;
		}
	    SetOrigName(origName, name);
	    if (DXGetComponentValue(partition, origName))
	    {
		toReplace[nR++] = name;
	    }
	    else if (DXGetAttribute((Object)array, "ref") ||
					DXGetAttribute((Object)array, "dep"))
	    {
		toShrink[nS++] = name;
	    }
	}
    
    /*
     * Replace the ones that have "originals" - IF the "original" has 
     * non-zero entries.
     */
    for (i = 0; i < nR; i++)
    {
	int nItems;

	SetOrigName(origName, toReplace[i]);

	array = (Array)DXGetComponentValue(partition, origName);
	if (! array)
	    goto error;

	DXGetArrayInfo(array, &nItems, NULL, NULL, NULL, NULL);
	if (nItems == 0)
	    array = NULL;

	DXSetComponentAttribute(partition, toReplace[i], "ref", NULL);
	DXSetComponentAttribute(partition, toReplace[i], "dep", NULL);
	if (! DXSetComponentValue(partition, toReplace[i], (Object)array))
	    goto error;
	
	DXDeleteComponent(partition, origName);
    }
    
    /*
     * Now shrink the one that require it.
     */
    for (i = 0; i < nS; i++)
    {
	name = toShrink[i];

	oArray = (Array)DXGetComponentValue(partition, name);
	if (! oArray)
	{
	    DXSetError(ERROR_INTERNAL, "component not found");
	    goto error;
	}

	nArray = (Array)DXReference((Object)oArray);

	attr = GetAttr(partition, name, "dep");
	if (! attr)
	{
	    dep = NOT_DEP;
	}
	else
	{
	    if (! strcmp(attr, "positions"))
	    {
		oLength = nPositions;
		dep = DEP_ON_POSITIONS;
	    }
	    else if (! strcmp(attr, "connections"))
	    {
		oLength = nConnections;
		dep = DEP_ON_CONNECTIONS;
	    }
	    else
	    {
		DXSetError(ERROR_DATA_INVALID, "illegal dependency: %s", 
					    DXGetString((String)attr));
		goto error;
	    }
	}

	attr = GetAttr(partition, name, "ref");
	if (! attr)
	{
	    ref = NOT_REFS;
	}
	else
	{
	    if (! strcmp(attr, "positions"))
	    {
		sstrides = spstrides;
		dstrides = dpstrides;
		maxi     = pmaxi;
		ref = REFS_POSITIONS;
	    }
	    else if (! strcmp(attr, "connections"))
	    {
		sstrides = scstrides;
		dstrides = dcstrides;
		maxi     = cmaxi;
		ref = REFS_CONNECTIONS;
	    }
	    else
		ref = NOT_REFS;
	}

	DXGetArrayInfo(oArray, &length, NULL, NULL, NULL, NULL);

	/*
	 * If the lengths mismatch, truncate the array.  Note that this
	 * creates a new array.
	 */
	if (dep != NOT_DEP)
	    if (length != oLength)
	    {
		Array tmp;
		
		tmp = ShrinkArray(nArray, oOffsets, pSizes, gSizes, nDim, dep);
		if (! tmp)
		    goto error;
		    
		DXDelete((Object)nArray);
		nArray = (Array)DXReference((Object)tmp);
	    }

	/*
	 * If the component references something, we need to 
	 * re-reference it to reflect to portion of the referenced 
	 * array that has been removed.
	 */
	if (ref != NOT_REFS)
	{
	    int nItemsIn;
	    Type t; Category c; int r, s[32];
	    int *sPtr, *dPtr;
	    int nRefsPerElt = 0;

	    DXGetArrayInfo(nArray, &nItemsIn, &t, &c, &r, s);

	    /*
	     * Two possibilities: either the input is dep on
	     * soneting, in which case we just renumber it, or it 
	     * doesn't, in which case we remove elements that contain
	     * no references into the original portion of the partition.
	     */
	    if (dep == DEP_ON_POSITIONS)
	    {
		length = nPositions;
	    }
	    else if (dep == DEP_ON_CONNECTIONS)
	    {
		length = nConnections;
	    }
	    else
	    {
		/*
		 * Count the ones that will remain.
		 */
		int i, j, in;

		nRefsPerElt = DXGetItemSize(nArray) / sizeof(int);
		sPtr = (int *)DXGetArrayData(nArray);

		length = 0;
		for (i = 0; i < nItemsIn; i++, sPtr += nRefsPerElt)
		{
		    for (in = 1, j = 0; in && j < nRefsPerElt; j++)
		    {
			int k, indices[3];

			INDICES(*sPtr, indices, sstrides, nDim);
			OFFSET(indices, rOffsets, nDim);
			for (k = 0; in && k < nDim; k++)
			    if (indices[k] < 0 || indices[k] >= maxi[k])
				in = 0;
			
			if (in)
			    length++;
		    }
		}
	    }

	    /*
	     * If the length is zero, we delete the nArray reference and
	     * null it out.  Then the SetComponentValue will zip it
	     * out of the field.  Otherwise, we shrink it.
	     */
	    if (length == 0)
	    {
	       DXDelete((Object)nArray);
	       nArray = NULL;
	    }
	    else
	    {
		/*
		 * If the new array is not the original array and the 
		 * counts match, we can avoid creating a new array.  
		 * Otherwise...
		 */
		if (nArray == oArray || length != nItemsIn)
		{
		    rArray = DXNewArrayV(t, c, r, s);
		    if (! rArray)
			goto error;
		    
		    if (! DXAddArrayData(rArray, 0, length, NULL))
			goto error;
		}
		else
		{
		    rArray = (Array)DXReference((Object)nArray);
		}

		sPtr = (int *)DXGetArrayData(nArray);
		dPtr = (int *)DXGetArrayData(rArray);

		for (i = 0; i < nItemsIn; i++)
		{
		    int j, k, keep;
		    int refs[32];

		    for (keep = 0, j = 0; j < nRefsPerElt; j++, sPtr++)
		    {
			int indices[3], in;

			INDICES(*sPtr, indices, sstrides, nDim);
			OFFSET(indices, rOffsets, nDim);

			for (in = 1, k = 0; in && k < nDim; k++)
			    if (indices[k] < 0 || indices[k] >= maxi[k])
				in = 0;
			
			if (in)
			{
			    REFERENCE(indices, refs[j], dstrides, nDim);
			    keep = 1;
			}
			else
			    refs[j] = -1;
		    }

		    if (keep)
		    {
			memcpy((char *)dPtr, (char *)refs,
					nRefsPerElt*sizeof(int));
			dPtr += nRefsPerElt;
		    }
		}

		DXDelete((Object)nArray);
		nArray = (Array)DXReference((Object)rArray);
	    }
	}

	if (! DXSetComponentValue(partition, name, (Object)nArray))
	    goto error;

	DXDelete((Object)nArray);
	nArray = NULL;
    }

    if (! DXEndField(partition))
	goto error;

    return OK;

error:
    DXDelete((Object)nArray);

    return ERROR;
}

static Array
ShrinkArray(Array array, int *oOffs, int *oCnts, int *gCnts, int nDim, int dep)
{
    Array a;

    /*
     * The possibilities are: regular, product, and irregular.
     */
    if (DXGetArrayClass(array) == CLASS_REGULARARRAY)
    {
        a = ShrinkRegularArray((RegularArray)array, *oOffs, *oCnts, dep);
    }
    else if (DXGetArrayClass(array) == CLASS_PRODUCTARRAY)
    {
	a = ShrinkProductArray((ProductArray)array, oOffs,
					    oCnts, gCnts, nDim, dep);
    }
    else
    {
	a = ShrinkIrregularArray(array, oOffs, oCnts, gCnts, nDim, dep);
    }

    return a;
}

static Array
ShrinkProductArray(ProductArray array, 
		int *oOffs, int *oCnts, int *gCnts, int nDims, int dep)
{
    int 	nTerms, i;
    Array	inTerms[32], outTerms[32];

    DXGetProductArrayInfo(array, &nTerms, inTerms);

    /*
     * If we have one term per dimension, we can retain product array-hood.
     */
    if (nTerms != nDims)
    {
	return ShrinkIrregularArray((Array)array, oOffs,
					oCnts, gCnts, nDims, dep);
    }
    else
    {
	for (i = 0; i < nTerms; i++)
	{
	    outTerms[i] = ShrinkArray(inTerms[i], oOffs+i,
						oCnts+i, gCnts+i, 1, dep);
	    if (NULL == outTerms[i])
	    {
		for (i--; i >= 0; i--)
		    DXDelete((Object)outTerms[i]);
		return NULL;
	    }
	}
    }

    return (Array)DXNewProductArrayV(nTerms, outTerms);
}

static Array
ShrinkRegularArray(RegularArray array, int off, int len, int dep)
{
    Type   t;
    double aOrg[32], aDelta[32];
    int    i, shape;

    DXGetArrayInfo((Array)array, NULL, &t, NULL, NULL, &shape);
    DXGetRegularArrayInfo(array, NULL, (Pointer)aOrg, (Pointer)aDelta);

    if (dep == DEP_ON_CONNECTIONS)
	len -= 1;

    /*
     * We need to bump origin by offset times the delta.
     */
    if (t == TYPE_FLOAT)
    {
	float *fOrg, *fDelta;
	int   i;

	fOrg   = (float *)aOrg;
	fDelta = (float *)aDelta;
	for (i = 0; i < shape; i++)
	    *fOrg++ += off * *fDelta++;
    }
    else if (t == TYPE_DOUBLE)
    {
	double *dOrg, *dDelta;

	dOrg   = (double *)aOrg;
	dDelta = (double *)aDelta;
	for (i = 0; i < shape; i++)
	    *dOrg++ += off * *dDelta++;
    }

    return (Array)DXNewRegularArray(t, shape, len, (Pointer)aOrg, (Pointer)aDelta);
}
       
static Array
ShrinkIrregularArray(Array sArray, int *offs, int *oCnts, 
					int *gCnts, int nDim, int dep)
{
    int      i, j, k, itemSize, nItems;
    struct   loop loop[3];
    Array    dArray;
    byte     *srcData, *dstData;
    Type     type;
    Category cat;
    int	     rank, shape[32];
    int	     oK[8], gK[8];

    if (dep == DEP_ON_POSITIONS)
	for (i = 0; i < nDim; i++)
	{
	    oK[i] = oCnts[i];
	    gK[i] = gCnts[i];
	}
    else
	for (i = 0; i < nDim; i++)
	{
	    oK[i] = oCnts[i] - 1;
	    gK[i] = gCnts[i] - 1;
	}

    dArray = NULL;

    nItems = 1;
    for (i = 0; i < nDim; i++)
	nItems *= oK[i];

    /*
     * Set up generic looping.  Note that we invert the looping order
     * here.
     */
    for (i = 0; i < nDim; i++)
    {
	loop[i].srcStart = offs[(nDim-1)-i];
	loop[i].dstStart = 0;
	loop[i].length   = oK[(nDim-1)-i];

	if (i == 0)
	{
	    loop[i].srcSkip = 1;
	    loop[i].dstSkip = 1;
	}
	else
	{
	    loop[i].srcSkip = gK[nDim-i] * loop[i-1].srcSkip;
	    loop[i].dstSkip = oK[nDim-i] * loop[i-1].dstSkip;
	}
    }

    DXGetArrayInfo(sArray, NULL, &type, &cat, &rank, shape);

    dArray = DXNewArrayV(type, cat, rank, shape);
    if (! dArray)
	return NULL;
	
    if (! DXAddArrayData(dArray, 0, nItems, NULL))
    {
	DXDelete((Object)dArray);
	return NULL;
    }
	
    itemSize = DXGetItemSize(sArray);

    srcData = (byte *)DXGetArrayData(sArray);
    dstData = (byte *)DXGetArrayData(dArray);

    /*
     * Initialize looping.  Note: dst start is always [0][0]...[0]
     * For the source, we need to skip the overlap data
     */
    for (j = 0; j < nDim; j++)
	srcData += (loop[j].srcStart * loop[j].srcSkip * itemSize);

    for (j = 0; j < nDim; j++)
    {
	loop[j].inc    = 0;
	loop[j].srcPtr = srcData;
	loop[j].dstPtr = dstData;
    }

    /*
     * Now loop
     */
    for ( ;; )
    {
	memcpy(loop[0].dstPtr, loop[0].srcPtr, loop[0].length*itemSize);

	for (j = 1; j < nDim; j++)
	{
	    loop[j].srcPtr += (loop[j].srcSkip * itemSize);
	    loop[j].dstPtr += (loop[j].dstSkip * itemSize);

	    loop[j].inc ++;
	    if (loop[j].inc < loop[j].length)
		break;
	}

	if (j == nDim)
	    break;

	for (k = (j - 1); k >= 0; k--)
	{
	    loop[k].inc = 0;
	    loop[k].srcPtr = loop[j].srcPtr;
	    loop[k].dstPtr = loop[j].dstPtr;
	}
    }

    return dArray;
}


/*
 * Note that it is critical to call AddOverlapData with the central
 * partition first so that the new positions and connections components
 * get set up correctly.
 */
static Error
AddOverlapData(Field dstField, int *dstCounts, int *meshOffsets,
			Field srcField, int *indices, LoHi growth,
			LoHi overlap, LoHi extra, int nDim, char **components)
{
    int    i, j, k, itemSize;
    struct loop pdep_loop[3];
    struct loop cdep_loop[3];
    struct loop *loop;
    Array  srcArray, dstArray, origArray = NULL;
    int    srcCounts[3];
    char   *name;
    int    spmin[3], spmax[3];
    int    scmin[3], scmax[3];
    /*int    dpmin[3], dpmax[3];*/
    /*int    dcmin[3], dcmax[3];*/
    int    *smin = NULL, *smax = NULL;
    int    scstride[3], spstride[3], *sstride = NULL;
    int    dcstride[3], dpstride[3], *dstride = NULL;
    int	   offset[3];
    int	   ddep, dref, nRefs, doPositions = 0;
    int    sdep, sref;
    char   origName[64];

    if (DXEmptyField(srcField))
	return OK;

    /*
     * First deal with connections.  Get the original and stick it into
     * "original connections".  Then create the new one, install the
     * approproate attributes and stick it in "connections"
     */
    srcArray = (Array)DXGetComponentValue(srcField, "connections");
    if (! srcArray)
	return OK;
    
    if (! DXQueryGridConnections(srcArray, NULL, srcCounts))
    {
	DXSetError(ERROR_MISSING_DATA, "irregular connections");
	return ERROR;
    }

    if (! DXGetComponentValue(dstField, "original connections"))
    {
	if (! DXSetComponentValue(dstField, "original connections",
							(Object)srcArray))
	    goto error;

	dstArray = DXMakeGridConnectionsV(nDim, dstCounts);
	if (! dstArray)
	    return ERROR;

	if (! DXSetMeshOffsets((MeshArray)dstArray, meshOffsets))
	{
	    DXDelete((Object)dstArray);
	    goto error;
	}

	if (! DXCopyAttributes((Object)dstArray, (Object)srcArray))
	{
	    DXDelete((Object)dstArray);
	    goto error;
	}

	if (! DXSetComponentValue(dstField, "connections", (Object)dstArray))
	{
	    DXDelete((Object)dstArray);
	    goto error;
	}
    }
    /*
     * Given a reference in the source component, we need to
     *
     * a) determine whether it lies in the overlap region, and 
     * b) what the index in the grown field will be. 
     *
     * In the following loop we determine the index ranges
     * of the included portions of the source field, with
     * spmin[i] and spmax[i] representing the min and max 
     * position indices along the i-th dimension and, similarly,
     * scmin[i] and scmax[i] representing the min and max 
     * connection indices along the i-th dimension.  Later
     * this info will enable us to determine whether an element
     * referred to in the source will be copied into the destination.
     *
     * We also get the offset of the source grid into the destination
     * grid.  Given this we can translate a source reference's 
     * indices into a destination's indices, from where we can get 
     * the destinations reference.
     */
    for (i = 0; i < nDim; i++)
    {
	switch(indices[i])
	{
	    case 0:
		spmin[i]  = (srcCounts[i] - 1) - overlap[i].lo;
		spmax[i]  = (srcCounts[i] - 1);
		offset[i] = -((srcCounts[i] - 1) - overlap[i].lo);
		break;

	    case 1:
		spmin[i]  = 0;
		spmax[i]  = (srcCounts[i] - 1);
		offset[i] = overlap[i].lo;
		break;

	    case 2:
		spmin[i]  = 0;
		spmax[i]  = overlap[i].hi;
		offset[i] = (dstCounts[i] - 1) - overlap[i].hi;
		break;
	}

	scmin[i] = spmin[i];
	scmax[i] = spmax[i] - 1;

	/*dpmin[i] = 0;*/
	/*dpmax[i] = dstCounts[i] - 1;*/
	/*dcmin[i] = 0;*/
	/*dcmax[i] = dpmax[i] - 1;*/

    }

    /*
     * Now determine the positions and connections strides in both the 
     * source and destination
     */
    scstride[nDim-1] = 1;
    spstride[nDim-1] = 1;
    dcstride[nDim-1] = 1;
    dpstride[nDim-1] = 1;
    for (i = nDim-2; i >= 0; i--)
    {
	scstride[i] = scstride[i+1]*(srcCounts[i+1]-1);
	spstride[i] = spstride[i+1]*srcCounts[i+1];
	dcstride[i] = dcstride[i+1]*(dstCounts[i+1]-1);
	dpstride[i] = dpstride[i+1]*dstCounts[i+1];
    }

    /*
     * Set up generic looping.  Note that we invert the looping order
     * here.
     */
    for (i = 0; i < nDim; i++)
    {
	switch(indices[(nDim-1)-i])
	{
	    case 0:
		pdep_loop[i].srcStart = (srcCounts[(nDim-1)-i] -
						overlap[(nDim-1)-i].lo) - 1;
		pdep_loop[i].dstStart = extra[(nDim-1)-i].lo;
		pdep_loop[i].length   = overlap[(nDim-1)-i].lo;

		cdep_loop[i].srcStart = ((srcCounts[(nDim-1)-i]-1) -
						overlap[(nDim-1)-i].lo);
		cdep_loop[i].dstStart = extra[(nDim-1)-i].lo;
		cdep_loop[i].length   = overlap[(nDim-1)-i].lo;
		break;
	    case 1:
		pdep_loop[i].srcStart = 0;
		pdep_loop[i].dstStart = 
			(extra[(nDim-1)-i].lo + overlap[(nDim-1)-i].lo);
		pdep_loop[i].length   = dstCounts[(nDim-1)-i] - 
		    ((overlap[(nDim-1)-i].lo + overlap[(nDim-1)-i].hi) +
		     (extra[(nDim-1)-i].lo + extra[(nDim-1)-i].hi));

		cdep_loop[i].srcStart = 0;
		cdep_loop[i].dstStart = 
			(extra[(nDim-1)-i].lo + overlap[(nDim-1)-i].lo);
		cdep_loop[i].length   = (dstCounts[(nDim-1)-i]-1) - 
		    ((overlap[(nDim-1)-i].lo + overlap[(nDim-1)-i].hi) +
		     (extra[(nDim-1)-i].lo + extra[(nDim-1)-i].hi));
		break;
	    case 2:
		pdep_loop[i].srcStart = 1;
		pdep_loop[i].dstStart = dstCounts[(nDim-1)-i] -
				(overlap[(nDim-1)-i].hi + extra[(nDim-1)-i].hi);
		pdep_loop[i].length   = overlap[(nDim-1)-i].hi;

		cdep_loop[i].srcStart = 0;
		cdep_loop[i].dstStart = (dstCounts[(nDim-1)-i]-1) -
				(overlap[(nDim-1)-i].hi + extra[(nDim-1)-i].hi);
		cdep_loop[i].length   = overlap[(nDim-1)-i].hi;
		break;
	}

	if (i == 0)
	{
	    pdep_loop[i].srcSkip = 1;
	    pdep_loop[i].dstSkip = 1;
	    cdep_loop[i].srcSkip = 1;
	    cdep_loop[i].dstSkip = 1;
	}
	else
	{
	    pdep_loop[i].srcSkip = srcCounts[nDim-i] * pdep_loop[i-1].srcSkip;
	    pdep_loop[i].dstSkip = dstCounts[nDim-i] * pdep_loop[i-1].dstSkip;

	    cdep_loop[i].srcSkip = 
		(srcCounts[nDim-i]-1) * cdep_loop[i-1].srcSkip;
	    cdep_loop[i].dstSkip = 
		(dstCounts[nDim-i]-1) * cdep_loop[i-1].dstSkip;
	}
    }
    
    /*
     * Now deal with positions
     */

    srcArray = (Array)DXGetComponentValue(srcField, "positions");
    dstArray = (Array)DXGetComponentValue(dstField, "positions");
    if (dstArray)
    {
	/*
	 * If its regular, then we handle it separately.  Otherwise,
	 * handle it just like any other irregular component.
	 */
	if (DXQueryGridPositions(dstArray, NULL, NULL, NULL, NULL))
	{
	    doPositions = 0;

	    /*
	     * If there isn't an "original" version yet, this is the
	     * first time through and we go ahead and grow it.
	     */
	    if (! DXGetComponentValue(dstField, "original positions"))
	    {
		if (! DXSetComponentValue(dstField, "original positions",
							    (Object)dstArray))
		    goto error;
		
		dstArray = GrowRegularGrid(srcArray, growth);
		if (! dstArray)
		    goto error;

		if (! DXCopyAttributes((Object)dstArray, (Object)srcArray))
		{
		    DXDelete((Object)dstArray);
		    goto error;
		}

		if (! DXSetComponentValue(dstField, "positions",
							(Object)dstArray))
		{
		    DXDelete((Object)dstArray);
		    goto error;
		}
	    }
	}
	else
	    doPositions = 1;
    }

    i = 0;
    while (NULL != (srcArray = GetEArray(srcField, i++, &name)))
    {
	if (! DoIt(name, components))
	    continue;
	
	if (! strcmp(name, "connections"))
	    continue;
	
	if (! strcmp(name, "positions") && ! doPositions)
	    continue;
	
	if (! GetDepRef(srcArray, &sdep, &sref))
	    goto error;

	if (sdep == NOT_DEP && sref == NOT_REFS)
	    continue;

	/*
	 * Get the destination array
	 */
	dstArray = (Array)DXGetComponentValue(dstField, name);
	if (! dstArray)
	{
	    /*
	     * OK.  There's no equivalent in the destination field to
	     * a component in the source field.  Assume that a referential
	     * component will suffice.   Better be an invalid component.
	     */

	    if (strncmp(name, "invalid", 7))
	    {
		DXSetError(ERROR_DATA_INVALID, "mismatching components: %s",
							name);
		goto error;
	    }

	    dstArray = DXNewArray(TYPE_INT, CATEGORY_REAL, 0);

	    if (sdep == DEP_ON_POSITIONS || sref == REFS_POSITIONS)
	    {
		if (! DXSetAttribute((Object)dstArray, "ref",
					(Object)DXNewString("positions")))
		{
		    DXDelete((Object)dstArray);
		    goto error;
		}
		dref = REFS_POSITIONS;
	    }
	    else
	    {
		if (! DXSetAttribute((Object)dstArray, "ref",
					(Object)DXNewString("connections")))
		{
		    DXDelete((Object)dstArray);
		    goto error;
		}
		dref = REFS_CONNECTIONS;
	    }

	    ddep = NOT_DEP;

	    if (! DXSetComponentValue(dstField, name, (Object)dstArray))
	    {
		DXDelete((Object)dstArray);
		goto error;
	    }
	}
	else
	{
	    if (! GetDepRef(dstArray, &ddep, &dref))
		goto error;
	}

	if (ddep == DEP_ON_POSITIONS || sdep == DEP_ON_POSITIONS)
	    loop = pdep_loop;
	else if (ddep == DEP_ON_CONNECTIONS || sdep == DEP_ON_CONNECTIONS)
	    loop = cdep_loop;
	else
	    loop = NULL;

	/*
	 * Make sure there's already a saved copy
	 */
	SetOrigName(origName, name);
	if (! DXGetComponentValue(dstField, origName))
	{
	    Type type;
	    Category cat;
	    int knt, rank, shape[32];
	    
	    /*
	     * If not, this is the first time we've encountered
	     * this component.  Place dstArray in "original..." and
	     * create a copy in "..."
	     */

	    origArray = dstArray;

	    DXGetArrayInfo(origArray, &knt, &type, &cat, &rank, shape);

	    dstArray = InitGrowIrregArray(origArray, dstCounts, nDim, sdep);
	    if (! dstArray)
		goto error;

	    if (! DXCopyAttributes((Object)dstArray, (Object)origArray))
	    {
		DXDelete((Object)dstArray);
		dstArray = NULL;
		goto error;
	    }
	    
	    if (! DXSetComponentValue(dstField, origName, (Object)origArray))
		goto error;

	    if (! DXSetComponentValue(dstField, name, (Object)dstArray))
		goto error;
	    
	    origArray = NULL;
	}

	if (sref == REFS_POSITIONS || dref == REFS_POSITIONS)
	{
	    sstride = spstride;
	    dstride = dpstride;
	    smin    = spmin;
	    smax    = spmax;
	    /*dmin    = dpmin;*/
	    /*dmax    = dpmax;*/
	}
	else if (sref == REFS_CONNECTIONS || dref == REFS_CONNECTIONS)
	{
	    sstride = scstride;
	    dstride = dcstride;
	    smin    = scmin;
	    smax    = scmax;
	    /*dmin    = dcmin;*/
	    /*dmax    = dcmax;*/
	}
	
	itemSize = DXGetItemSize(srcArray);
	nRefs = itemSize / sizeof(int); /* just in case */

	if (sdep != NOT_DEP && ddep != NOT_DEP) /* case 0 */
	{
	    byte *srcData = (byte *)DXGetArrayData(srcArray);
	    byte *dstData = (byte *)DXGetArrayData(dstArray);

	    /*
	     * Both are dependent.  Destination array must already exist.
	     *
	     * Initialize looping
	     */
	    for (j = 0; j < nDim; j++)
	    {
		srcData += (loop[j].srcStart * loop[j].srcSkip) * itemSize;
		dstData += (loop[j].dstStart * loop[j].dstSkip) * itemSize;
	    }

	    for (j = 0; j < nDim; j++)
	    {
		loop[j].inc    = 0;
		loop[j].srcPtr = srcData;
		loop[j].dstPtr = dstData;
	    }

	    /*
	     * Now loop
	     */
	    for ( ;; )
	    {
		if (sref == NOT_REFS)
		{
		    memcpy(loop[0].dstPtr, loop[0].srcPtr,
						loop[0].length*itemSize);
		}
		else
		{
		    int i;
		    int *sPtr = (int *)loop[0].srcPtr;
		    int *dPtr = (int *)loop[0].dstPtr;
		    for (i = 0; i < loop[0].length*nRefs; i++, sPtr++, dPtr++)
		    {
			int indices[3], in;

			if (*sPtr == -1)
			    *dPtr = -1;
			else
			{
			    INDICES(*sPtr, indices, sstride, nDim);
			    IS_IN(in, indices, smin, smax, nDim);
			    if (in)
			    {
				OFFSET(indices, offset, nDim);
				REFERENCE(indices, *dPtr, dstride, nDim);
			    }
			    else
				*dPtr = -1;
			}
		    }
		}
		
		for (j = 1; j < nDim; j++)
		{
		    loop[j].srcPtr += (loop[j].srcSkip * itemSize);
		    loop[j].dstPtr += (loop[j].dstSkip * itemSize);

		    loop[j].inc ++;
		    if (loop[j].inc < loop[j].length)
			break;
		}

		if (j == nDim)
		    break;

		for (k = (j - 1); k >= 0; k--)
		{
		    loop[k].inc = 0;
		    loop[k].srcPtr = loop[j].srcPtr;
		    loop[k].dstPtr = loop[j].dstPtr;
		}
	    }
	}
	else if (sdep == NOT_DEP && ddep != NOT_DEP) /* case 1 */
	{
	    /*
	     * source component contains references while the destination
	     * component is dependent.  Must be an invalid component.
	     * Dependent destination component must already exist.
	     */
	    int  *srcData = (int  *)DXGetArrayData(srcArray);
	    byte *dstData = (byte *)DXGetArrayData(dstArray);
	    int  nRefs;

	    if ((ddep == DEP_ON_CONNECTIONS && sref != REFS_CONNECTIONS) ||
		(ddep == DEP_ON_POSITIONS   && sref != REFS_POSITIONS  ))
	    {
		DXSetError(ERROR_DATA_INVALID, "depref mismatch");
		goto error;
	    }

	    DXGetArrayInfo(srcArray, &nRefs, NULL, NULL, NULL, NULL);
	    dstData = (byte *)DXGetArrayData(dstArray);

	    for (j = 0; j < nRefs; j++)
	    {
		int indices[10];
		int dIndex, in;

		INDICES(srcData[j], indices, sstride, nDim);
		IS_IN(in, indices, smin, smax, nDim);
		if (in)
		{
		    OFFSET(indices, offset, nDim);
		    REFERENCE(indices, dIndex, dstride, nDim);
		    dstData[dIndex] = 1;
		}
	    }

	}
	else if (sdep != NOT_DEP && ddep == NOT_DEP) /* case 2 */
	{
	    /*
	     * destination component contains references while the source
	     * component is dependent.  Must be an invalid component.
	     */
	    byte *srcData;
	    int  *dstData;
	    int  nOldRefs, nNewRefs;

	    if ((sdep == DEP_ON_CONNECTIONS && dref != REFS_CONNECTIONS) ||
		(sdep == DEP_ON_POSITIONS   && dref != REFS_POSITIONS  ))
	    {
		DXSetError(ERROR_DATA_INVALID, "depref mismatch");
		goto error;
	    }

	    /*
	     * Initialize looping to count references
	     */
	    srcData = (byte *)DXGetArrayData(srcArray);
	    for (j = 0; j < nDim; j++)
		srcData += loop[j].srcStart * loop[j].srcSkip;

	    for (j = 0; j < nDim; j++)
	    {
		loop[j].inc    = 0;
		loop[j].srcPtr = srcData;
	    }

	    /*
	     * Now loop, counting references
	     */
	    nNewRefs = 0;
	    for ( ;; )
	    {
		byte *ptr = loop[0].srcPtr;

		for (j = 0; j < loop[0].length; j++)
		    if (*ptr++) 
			nNewRefs++;
		
		for (j = 1; j < nDim; j++)
		{
		    loop[j].srcPtr += loop[j].srcSkip;

		    loop[j].inc ++;
		    if (loop[j].inc < loop[j].length)
			break;
		}

		if (j == nDim)
		    break;

		for (k = (j - 1); k >= 0; k--)
		{
		    loop[k].inc = 0;
		    loop[k].srcPtr = loop[j].srcPtr;
		}
	    }

	    DXGetArrayInfo(dstArray, &nOldRefs, NULL, NULL, NULL, NULL);

	    if (! DXAddArrayData(dstArray, nOldRefs, nNewRefs, NULL))
		goto error;

	    dstData = ((int  *)DXGetArrayData(dstArray)) + nOldRefs;

	    for (j = 0; j < nDim; j++)
	    {
		loop[j].inc    = 0;
		loop[j].srcPtr = srcData;
	    }

	    for ( ;; )
	    {
		byte *ptr = loop[0].srcPtr;

		for (j = 0; j < loop[0].length; j++)
		    if (*ptr++) 
		    {
			int indices[32], k;
			for (k = 0; k < nDim-1; k++)
			    indices[k] = loop[(nDim-1)-k].dstStart
							+ loop[(nDim-1)-k].inc;
			indices[nDim-1] = loop[0].dstStart + j;
			REFERENCE(indices, k, dstride, nDim);
			*dstData++ = k;
		    }
		
		for (j = 1; j < nDim; j++)
		{
		    loop[j].srcPtr += loop[j].srcSkip;

		    loop[j].inc ++;
		    if (loop[j].inc < loop[j].length)
			break;
		}

		if (j == nDim)
		    break;

		for (k = (j - 1); k >= 0; k--)
		{
		    loop[k].inc = 0;
		    loop[k].srcPtr = loop[j].srcPtr;
		}
	    }
	}
	else if (sdep == NOT_DEP && ddep == NOT_DEP)
	{
	    /*
	     * Not a dependent array, but contains refs
	     */
	    Type type; Category cat; int rank, shape[32];
	    int *dPtr, *sPtr = (int *)DXGetArrayData(srcArray);
	    int  nDstItems = 0, newDstItems = 0, nSrcItems, in;

	    DXGetArrayInfo(srcArray, &nSrcItems, &type, &cat, &rank, shape);

	    /*
	     * Count refs that will be copied into the overlap region
	     */
	    for (j = 0, newDstItems = 0; j < nSrcItems; j++, sPtr += nRefs)
	    {
		for (k = 0, in = 0; !in && k < nRefs; k++)
		{
		    int indices[3];

		    if (sPtr[k] == -1)
			continue;

		    INDICES(sPtr[k], indices, sstride, nDim);
		    IS_IN(in, indices, smin, smax, nDim);
		}

		if (in)
		    newDstItems ++;
	    }

	    if (newDstItems)
	    {
		if (! dstArray)
		{
		    dstArray = DXNewArrayV(type, cat, rank, shape);
		    if (! dstArray)
			goto error;
		    
		    if (dref == REFS_POSITIONS)
		    {
			if (! DXSetAttribute((Object)dstArray, "ref",
					    (Object)DXNewString("positions")))
			    goto error;
		    }
		    else
		    {
			if (! DXSetAttribute((Object)dstArray, "ref",
					    (Object)DXNewString("connections")))
			    goto error;
		    }
		    
		    if (! DXSetComponentValue(dstField, name, (Object)dstArray))
		    {
			DXDelete((Object)dstArray);
			goto error;
		    }
		    
		    nDstItems = 0;
		}
		else
		    DXGetArrayInfo(dstArray, &nDstItems, NULL,NULL,NULL,NULL);

		if (! DXAddArrayData(dstArray, nDstItems, newDstItems, NULL))
		    return ERROR;
	    
		sPtr = (int *)DXGetArrayData(srcArray);
		dPtr = ((int *)DXGetArrayData(dstArray))+nDstItems;

		/*
		 * Count refs that will be copied into the overlap region
		 */
		for (j = 0, newDstItems = 0; j < nSrcItems; j++, sPtr += nRefs)
		{
		    for (k = 0, in = 0; !in && k < nRefs; k++)
		    {
			int indices[3];

			if (sPtr[k] == -1)
			    continue;

			INDICES(sPtr[k], indices, sstride, nDim);
			IS_IN(in, indices, smin, smax, nDim);
		    }

		    if (in)
		    {
			for (k = 0, in = 0; !in && k < nRefs; k++)
			{
			    int indices[3];

			    if (sPtr[k] == -1)
				dPtr[k] = -1;

			    INDICES(sPtr[k], indices, sstride, nDim);
			    IS_IN(in, indices, smin, smax, nDim);
			    if (in)
			    {
				OFFSET(indices, offset, nDim);
				REFERENCE(indices, dPtr[k], dstride, nDim);
			    }
			}

			dPtr += nRefs;
		    }
		}
	    }
	}
    }

    return OK;

error:
    DXDelete((Object)origArray);
    return ERROR;
}

static Array
InitGrowIrregArray(Array oArray, int *nCounts, int nDim, int dep)
{
    Type     type;
    Category cat;
    int      rank, shape[32];
    int	     i, nItems;
    Array    nArray;

    if (! DXGetArrayInfo(oArray, NULL, &type, &cat, &rank, shape))
	return NULL;
    
    nArray = DXNewArrayV(type, cat, rank, shape);
    if (! nArray)
	return NULL;
    
    if (dep != NOT_DEP)
    {
	if (dep == DEP_ON_POSITIONS)
	{
	    nItems = 1;
	    for (i = 0; i < nDim; i++)
		nItems *= nCounts[i];
	}
	else
	{
	    nItems = 1;
	    for (i = 0; i < nDim; i++)
		nItems *= (nCounts[i]-1);
	}
    
	if (! DXAddArrayData(nArray, 0, nItems, NULL))
	{
	    DXDelete((Object)nArray);
	    return NULL;
	}

	memset(DXGetArrayData(nArray), 0, nItems);
    }

    return nArray;
}

static Array
GrowRegularGrid(Array oArray, LoHi growth)
{
    int   oCounts[3], nCounts[3];
    float oOrigin[3], nOrigin[3];
    float deltas[9], *d;
    Array nArray;
    int   nDim, oDim, i, j;

    DXQueryGridPositions(oArray, &nDim, oCounts, oOrigin, deltas);

    /*
     * Origin is original, minus growth[i].lo*deltas;
     */
    for (i = 0; i < nDim; i++)
	nOrigin[i] = oOrigin[i];
    
    d = deltas; oDim = 0;
    for (i = 0; i < nDim; i++)
    {
	if (oCounts[i] > 1)
	{
	    /*
	     * DXAdd in counts for growth at either end of this axis, as
	     * required by the growth array
	     */
	    nCounts[i] = oCounts[i] + (growth[oDim].lo+growth[oDim].hi);

	    /*
	     * If we grew along the lower end of this axis, we need to 
	     * back the origin off appropriately.
	     */
	    if (growth[oDim].lo)
		for (j = 0; j < nDim; j++)
		    nOrigin[j] -= growth[oDim].lo * *d++;
	    else
		d += nDim;
	    
	    oDim ++;
	}
	else
	{   
	    nCounts[i] = 1;
	    d += nDim;
	}
    }
    
    nArray = DXMakeGridPositionsV(nDim, nCounts, nOrigin, deltas);
    return nArray;
}


#if 0
static Array
GrowRegularArray(Array oArray, int *counts, int nDim, int dep)
{
    int len, i, n;
    float o[32];
    float d[32];
    Type type;

    if (! DXGetRegularArrayInfo((RegularArray)oArray, 
					NULL, (Pointer)o, (Pointer)d))
	return NULL;

    DXGetArrayInfo(oArray, NULL, &type, NULL, NULL, &len);

    if (type != TYPE_FLOAT)
	return NULL;

    for (i = 0; i < len; i++)
	if (d[i] != 0.0)
	    return NULL;

    if (dep == DEP_ON_POSITIONS)
    {
	n = 1;
	for (i = 0; i < nDim; i++)
	    n *= counts[i];
    }
    else
    {
	n = 1;
	for (i = 0; i < nDim; i++)
	    n *= (counts[i]-1);
    }
    
    return (Array)DXNewRegularArray(type, len, n, (Pointer)o, (Pointer)d);
}
#endif

static Error
AddPartition(HashTable ht, Field partition)
{
    int   i, nDim, inc;
    int   box[8][3];

    inc = 0;

    if (DXEmptyField(partition))
	return OK;
    
    if (!GetIJKBox(partition, (int *)box, &nDim))
	return ERROR;

    if (nDim == 1)
	inc = 4;
    else if (nDim == 2)
	inc = 2;
    else if (nDim == 3)
	inc = 1;

    for (i = 0; i < 8; i += inc)
    {
	if (! AddPartitionEntry(ht, (int *)(box + i), i, partition))
	    return ERROR;
    }

    return OK;
}

static Error
GetIJKBox(Field partition, int *box, int *nDim)
{
    int   i, j, n;
    int   counts[3], offsets[3];
    Array array;

    array = (Array)DXGetComponentValue(partition, "connections");
    if (! array)
    {
	DXSetError(ERROR_MISSING_DATA, "no connections component");
	return ERROR;
    }

    if (!DXQueryGridConnections(array, &n, counts)    || 
	 n > 3   					||
	!DXGetMeshOffsets((MeshArray)array, offsets))
    {
	DXSetError(ERROR_DATA_INVALID,"regular connections of dim <= 3 expected");
	return ERROR;
    }

    for (i = n; i < 3; i++)
	counts[i] = offsets[i] = 0;

    /*
     * Generate (i,j,k) coordinates of corners of partition based on
     * the origin of the original partitioned field.  Use the three bits
     * of the corner index i to select which counts we add in.  Theres a 
     * reversal happening here: if i == 0x1, we add in the Z count while
     * if i == 0x4, we dd in X count
     */
    for (i = 0; i < 8; i++, box += 3)
    {
	box[0] = offsets[0];
	box[1] = offsets[1];
	box[2] = offsets[2];

	for (j = 0; j < 3; j++)
	    if (i & (1<<j))
		box[2-j] += (counts[2-j] - 1);
    }

    if (nDim)
	*nDim = n;

    return OK;
}

static Error
AddPartitionEntry(HashTable ht, int *ijk, int tag, Field partition)
{
    struct partHash p;

    p.base[0]   = *ijk++;
    p.base[1]   = *ijk++;
    p.base[2]   = *ijk++;
    p.tag       = tag;
    p.partition = (Field)DXReference((Object)partition);

    return DXInsertHashElement(ht, (Element)&p);
}

static Field
FindNeighbor(HashTable ht, int *ijk, int tag)
{
    struct partHash p, *ptr;

    p.base[0]   = *ijk++;
    p.base[1]   = *ijk++;
    p.base[2]   = *ijk++;
    p.tag       = tag;

    ptr = (struct partHash *)DXQueryHashElement(ht, (void *)&p);

    if (! ptr)
	return NULL;
    else
	return ptr->partition;
}

static PseudoKey
PartHash(Key key)
{
    struct partHash *p;
    register long l;

    p = (struct partHash *)key;
    
    l = p->base[0]*17 + p->base[1]*107 + p->base[2]*557 + p->tag*1039;

    return (PseudoKey) l;
}

static int
PartCmp(Key k0, Key k1)
{
    struct partHash *p0, *p1;

    p0 = (struct partHash *)k0;
    p1 = (struct partHash *)k1;

    if (p0->base[0] != p1->base[0]) return 1;
    if (p0->base[1] != p1->base[1]) return 1;
    if (p0->base[2] != p1->base[2]) return 1;
    if (p0->tag     != p1->tag) return 1;
    return 0;
}

Field
DXQueryOriginalMeshExtents(Field f, int *offsets, int *sizes) 
{
    int   oOffsets[32], grownOffsets[32];
    Array orig, grown;
    int	  nDim, i;

    orig = (Array)DXGetComponentValue(f, "original connections");
    if (! orig)
	return NULL;

    if (sizes || offsets)
    {
	if (! DXQueryGridConnections(orig, &nDim, sizes))
	{
	    DXSetError(ERROR_DATA_INVALID, "regular connections required");
	    return NULL;
	}
    }
    
    if (offsets)
    {
	grown = (Array)DXGetComponentValue(f, "connections");
	if (! grown)
	{
	    DXSetError(ERROR_DATA_INVALID,
		"grown connections component not found");
	    return NULL;
	}

	if (!DXGetMeshOffsets((MeshArray)orig, oOffsets))
	{
	    DXSetError(ERROR_DATA_INVALID, 
		"error retrieving original mesh offset");
	    return NULL;
	}

	if (!DXGetMeshOffsets((MeshArray)grown, grownOffsets))
	{
	    DXSetError(ERROR_DATA_INVALID, 
		"error retrieving grown mesh offset");
	    return NULL;
	}

	for (i = 0; i < nDim; i++)
	    offsets[i] = oOffsets[i] - grownOffsets[i];
    }

    return f;
}

static Error
FillEmptyOverlap(Field field, int *indices, LoHi growth,
				int nDim, char **comp, Pointer fill)
{
    int    i, j, k, itemSize = 0;
    struct loop cdep_loop[3];
    struct loop pdep_loop[3];
    struct loop *loop;
    Array  cArray, array;
    int    counts[3], cdep_sizes[3], pdep_sizes[3];
    byte   *srcData, *dstData;
    char   *name;
    int	   doit;
    char   **cmp;
    int    cstFill;
    Object attr;
    char   *dep;

    cstFill = (fill > (Pointer)0x3);

    cArray = (Array)DXGetComponentValue(field, "connections");
    if (! cArray)
	return ERROR;
    
    if (! DXQueryGridConnections(cArray, NULL, counts))
    {
	DXSetError(ERROR_MISSING_DATA, "FillEmptyOverlap: irregular connections");
	return ERROR;
    }

    cdep_sizes[nDim-1] = pdep_sizes[nDim-1] = 1;
    for (i = nDim-2; i >= 0; i--)
    {
	cdep_sizes[i] = cdep_sizes[i+1] * (counts[i+1] - 1);
	pdep_sizes[i] = pdep_sizes[i+1] * counts[i+1];
    }

    /*
     * Set up generic looping.  
     */
    for (i = 0, j = 0; i < nDim; i++)
    {
	switch(indices[i])
	{
	    case 0:
		pdep_loop[nDim-1].srcStart = growth[i].lo * pdep_sizes[i];
		pdep_loop[nDim-1].dstStart = 0;
		pdep_loop[nDim-1].length   = growth[i].lo;
		pdep_loop[nDim-1].srcSkip  = pdep_sizes[i];

		cdep_loop[nDim-1].srcStart = growth[i].lo * cdep_sizes[i];
		cdep_loop[nDim-1].dstStart = 0;
		cdep_loop[nDim-1].length   = growth[i].lo;
		cdep_loop[nDim-1].srcSkip  = cdep_sizes[i];
		break;
	    case 1:
		pdep_loop[j].srcStart = 0;
		pdep_loop[j].dstStart = 0;
		pdep_loop[j].length   = counts[i];
		pdep_loop[j].srcSkip  = pdep_sizes[i];

		cdep_loop[j].srcStart = 0;
		cdep_loop[j].dstStart = 0;
		cdep_loop[j].length   = (counts[i] - 1);
		cdep_loop[j].srcSkip  = cdep_sizes[i];
		j++;
		break;
	    case 2:
		pdep_loop[nDim-1].srcStart = ((counts[i] -
			    growth[i].hi) - 1) * pdep_sizes[i];
		pdep_loop[nDim-1].dstStart = (counts[i] -
			    growth[i].hi) * pdep_sizes[i];
		pdep_loop[nDim-1].length   = growth[i].hi;
		pdep_loop[nDim-1].srcSkip  = pdep_sizes[i];

		cdep_loop[nDim-1].srcStart = (((counts[i] - 1) -
			    growth[i].hi) - 1) * cdep_sizes[i];
		cdep_loop[nDim-1].dstStart = ((counts[i] - 1) -
			    growth[i].hi) * cdep_sizes[i];
		cdep_loop[nDim-1].length   = growth[i].hi;
		cdep_loop[nDim-1].srcSkip  = cdep_sizes[i];

		break;
	}
    }

    i = 0;
    while (NULL != (array = GetEArray(field, i++, &name)))
    {
	doit = 0;

	if (! strcmp(name, "positions"))
	    doit = 1;
	
	if (comp)
	    for (cmp = comp; !doit && *cmp; cmp++)
		if (! strcmp(*cmp, name))
		{
		    doit = 1;
		    break;
		}

	if (! doit)
	    continue;
	
	attr = DXGetComponentAttribute(field, name, "dep");
	if (! attr)
	    continue;

	if (DXGetObjectClass(attr) != CLASS_STRING)
	{
	    DXSetError(ERROR_MISSING_DATA, "invalid dependency attribute");
	    return ERROR;
	}

	dep = DXGetString((String)attr);
	if (! strcmp(dep, "connections"))
	    loop = cdep_loop;
	else if (! strcmp(dep, "positions"))
	    loop = pdep_loop;
	else
	{
	    DXSetError(ERROR_MISSING_DATA, "invalid dependency attribute");
	    return ERROR;
	}

	if (DXQueryGridPositions(array, NULL, NULL, NULL, NULL))
	    continue;

	/*
	 * If its a regular array and we are replicating the edge value,
	 * then we are already done.  If its regular, then if we are inserting
	 * a fill value, we may need to irregularize the array.
	 */
	if (DXGetArrayClass(array) == CLASS_REGULARARRAY)
	{ 
	    Type  type;
	    Category cat;
	    int rank, shape[32];
	    int nItems;
	    int size, ok=0;

	    if (! cstFill)
		continue;

	    size = DXGetItemSize(array);
	    DXGetArrayInfo(array, &nItems, &type, &cat, &rank, shape);

#define CHECK(type, ok)							\
{									    		\
    type *o = NULL, *d = NULL;					\
    int  i;								    	\
									    		\
    o = (type *)DXAllocate(size); 				\
    d = (type *)DXAllocate(size);				\
    if (! o|| ! d)						    	\
    {									    	\
	DXFree((Pointer)o);						    \
	DXFree((Pointer)d);						    \
	goto error;							    	\
    }									    	\
									    		\
    DXGetRegularArrayInfo((RegularArray)array, NULL, (Pointer)o, (Pointer)d); \
									    		\
    for (i = 0, ok = 1; i < shape[0] && ok; i++)\
    {									    	\
	if (d[i] != (type)0)						\
	    ok = 0;							    	\
	if (o[i] != ((type *)fill)[i])				\
	    ok = 0;							    	\
    }									    	\
									    		\
    DXFree((Pointer)o);							\
    DXFree((Pointer)d);							\
}
	    switch(type)
	    {
		case TYPE_DOUBLE:
		    CHECK(double, ok);
		    break;

		case TYPE_FLOAT:
		    CHECK(float, ok);
		    break;

		case TYPE_INT:
		    CHECK(int, ok);
		    break;

		case TYPE_SHORT:
		    CHECK(short, ok);
		    break;

		case TYPE_UBYTE:
		    CHECK(ubyte, ok);
		    break;
	    }
	
	    if (ok)
		continue;
	    
#define IRREGULARIZE(dx_type, type)					\
{									    			\
    type *o = NULL, *d = NULL,	*ptr;				\
    Array nArray = NULL;					        \
    int   i, j;								    	\
									    			\
    nArray = DXNewArrayV(dx_type, cat, rank, shape);\
    if (! nArray)							    	\
	goto error;							    		\
									    			\
    if (! DXAddArrayData(nArray, 0, nItems, NULL))	\
    {									    		\
	DXDelete((Object)nArray);						\
	goto error;							    		\
    }									    		\
									    			\
    ptr = (type *)DXGetArrayData(nArray) + loop[nDim-1].srcStart*itemSize; \
    if (! ptr)								    	\
    {									    		\
	DXDelete((Object)nArray);						\
	goto error;							    		\
    }									    		\
									    			\
    o = (type *)DXAllocate(size); 					\
    d = (type *)DXAllocate(size);					\
    if (! o || ! d)							    	\
    {									    		\
	DXFree((Pointer)o);						    	\
	DXFree((Pointer)d);						    	\
	goto error;							    		\
    }									    		\
									    			\
    DXGetRegularArrayInfo((RegularArray)array, NULL, (Pointer)o, (Pointer)d); \
									    			\
    for (i = 0; i < nItems - loop[nDim-1].srcStart; i++) \
	for (j = 0; j < shape[0]; j++)					\
	    *ptr++ = o[j] + i*d[j];					    \
									    			\
									    			\
    DXSetComponentValue(field, name, (Object)nArray);\
									    			\
    DXFree((Pointer)o);							    \
    DXFree((Pointer)d);							    \
}

	    switch(type)
	    {
		case TYPE_DOUBLE:
		    IRREGULARIZE(TYPE_DOUBLE, double);
		    break;

		case TYPE_FLOAT:
		    IRREGULARIZE(TYPE_FLOAT, float);
		    break;

		case TYPE_INT:
		    IRREGULARIZE(TYPE_INT, int);
		    break;

		case TYPE_SHORT:
		    IRREGULARIZE(TYPE_SHORT, short);
		    break;

		case TYPE_UBYTE:
		    IRREGULARIZE(TYPE_UBYTE, ubyte);
		    break;
	    }
	}
	
	itemSize = DXGetItemSize(array);

	dstData = srcData = (byte *)DXGetArrayData(array);
	dstData += loop[nDim-1].dstStart * itemSize;

	if (cstFill)
	    srcData = (byte *)fill;
	else
	    srcData += loop[nDim-1].srcStart * itemSize;

	for (j = 0; j < nDim; j++)
	{
	    loop[j].inc    = 0;
	    loop[j].srcPtr = srcData;
	    loop[j].dstPtr = dstData;
	}

	/*
	 * Now loop
	 */
	for ( ;; )
	{
	    if (! cstFill)
		srcData = loop[0].srcPtr;

	    dstData = loop[0].dstPtr;

	    for (j = 0; j < loop[0].length; j++)
	    {
		memcpy(dstData, srcData, itemSize);

		if (! cstFill)
		    srcData += loop[0].srcSkip * itemSize;

		dstData += loop[0].srcSkip * itemSize;
	    }
	    
	    for (j = 1; j < nDim; j++)
	    {
		if (j < (nDim-1))
		    loop[j].srcPtr += (loop[j].srcSkip * itemSize);

		loop[j].dstPtr += (loop[j].srcSkip * itemSize);

		loop[j].inc ++;
		if (loop[j].inc < loop[j].length)
		    break;
	    }

	    if (j == nDim)
		break;

	    for (k = (j - 1); k >= 0; k--)
	    {
		loop[k].inc = 0;
		loop[k].srcPtr = loop[j].srcPtr;
		loop[k].dstPtr = loop[j].dstPtr;
	    }
	}
    }

    return OK;

error:
    return ERROR;
}


static int DoIt(char *name, char **components)
{
    if (! strcmp(name, "positions") 		||
	! strcmp(name, "invalid positions")	||
        ! strcmp(name, "connections")  		||
	! strcmp(name, "invalid connections"))
	return 1;
	
    if (components)
	while (*components)
	{
	    if (! strcmp(*components, name))
		return 1;
	
	    components ++;
	}
    
    return 0;
}

static Error
GetDepRef(Array array, int *dep, int *ref)
{
    Object attr;

    attr = DXGetAttribute((Object)array, "ref");
    if (attr)
    {
	if (DXGetObjectClass(attr) != CLASS_STRING)
	{
	    DXSetError(ERROR_DATA_INVALID, "#10200", "ref attribute");
	    return ERROR;
	}

	if (!strcmp(DXGetString((String)attr), "positions"))
	    *ref = REFS_POSITIONS;
	else if (! strcmp(DXGetString((String)attr), "connections"))
	    *ref = REFS_CONNECTIONS;
	else
	    *ref = NOT_REFS;
    }
    else
	*ref = NOT_REFS;
	
    attr = DXGetAttribute((Object)array, "dep");
    if (attr)
    {
	if (DXGetObjectClass(attr) != CLASS_STRING)
	{
	    DXSetError(ERROR_DATA_INVALID, "#10200", "dep attribute");
	    return ERROR;
	}

	if (!strcmp(DXGetString((String)attr), "positions"))
	    *dep = DEP_ON_POSITIONS;
	else if (! strcmp(DXGetString((String)attr), "connections"))
	    *dep = DEP_ON_CONNECTIONS;
	else
	    *dep = NOT_DEP;
    }
    else
	*dep = NOT_DEP;

    return OK;
}
	

typedef struct
{
    int index;
    int limit;
    int base;
    int stride;
} Loop;

static Error 
DupTask(Pointer p)
{
    Field field = (Field)p;
    int   mo[32], mc[32], stride[32], i, j, k, nd, done;
    InvalidComponentHandle ich = NULL;
    Loop loop[32];
    Array a;

    a = (Array)DXGetComponentValue(field, "invalid positions");
    if (a)
    {
	if (! DXSetComponentValue(field, "saved invalid positions", (Object)a))
	    goto error;
    }

    a = (Array)DXGetComponentValue(field, "connections");
    if (! a)
	return OK;

    if (! DXGetMeshOffsets((MeshArray)a, mo))
	goto error;

    if (! DXQueryGridConnections(a, &nd, mc))
	goto error;
    
    stride[nd-1] = 1;
    for (i = nd-2; i >= 0; i--)
	stride[i] = stride[i+1]*mc[i+1];

    ich = DXCreateInvalidComponentHandle((Object)field, NULL, "positions");
    if (! ich)
	goto error;

    if (nd == 1)
    {
	if (mc[0] != 0)
	    if (! DXSetElementInvalid(ich, 0))
		goto error;
    }
    else
    {
	for (i = 0; i < nd; i++)
	{
	    if (mo[i] != 0)
	    {
		for (j = k = 0; j < nd; j++)
		    if (i != j)
		    {
			loop[k].index  = 0;
			loop[k].limit  = mc[j];
			loop[k].base   = 0;
			loop[k].stride = stride[j];
			k++;
		    }
		
		done = 0;
		while (! done)
		{
		    int b = loop[0].base;
		    for (j = 0; j < loop[0].limit; j++, b += loop[0].stride)
			if (! DXSetElementInvalid(ich, b))
			    goto error;
		    
		    for (j = 1; j < (nd-1); j++)
			if (++loop[j].index < loop[j].limit)
			{
			    loop[j].base += loop[j].stride;
			    for (k = j-1; k >= 0; k--)
			    {
				loop[k].base = loop[j].base;
				loop[k].index = 0;
			    }
			    break;
			}
		    
		    done = (j == (nd-1));
		}
	    }
	}
    }

    if (! DXSaveInvalidComponent(field, ich))
	goto error;
    
    DXFreeInvalidComponentHandle(ich);

    return OK;

error:
    if (ich)
	DXFreeInvalidComponentHandle(ich);

    return ERROR;
}

Error
_dxfRegInvalidateDupBoundary(CompositeField cf)
{
    Field f;
    int i;

    if (! DXCreateTaskGroup())
	goto error;

    i = 0;
    while (NULL != (f = (Field)DXGetEnumeratedMember((Group)cf, i++, NULL)))
        if (! DXAddTask(DupTask, f, 0, 1.0))
	    goto error;
    
    if (! DXExecuteTaskGroup() || DXGetError() != ERROR_NONE)
	goto error;
    
    return OK;

error:
    return ERROR;
}
	
    








	


