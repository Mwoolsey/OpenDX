/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>



#include <string.h>
#include <dx/dx.h>


/*
 * Support structure for a linked list used to associate boundary vertices of 
 * adjacent partitions.  Individual links, block of links for efficient 
 * allocation and list header.
 */
struct link
{
    struct link		*next;
    int			v0;	/* index in "my" vertex list */
    int			v1;	/* indexd in neighbor's vertex list */
};
typedef struct link *Link;

#define PLELTS_PER_BLOCK	64

struct linkBlock
{
    struct linkBlock	*nextBlock;
    int			nextFreeElt;
    struct link		links[PLELTS_PER_BLOCK];
};
typedef struct linkBlock *LinkBlock;

struct list
{
    Link		list;
    Link		freeList;
    LinkBlock		listBlocks;
};
typedef struct list *List;
typedef struct list *Boundary;

/*
 * Overlaps contain the information from one partition that must
 * be added onto another.
 */
struct overlap
{
    int				nComponents;
    int				nPoints;
    int				nElements;
    char			**names;
    Pointer			*buffers;
    byte			*dependencies;
    int 			*counts;
    Array 			*arrays;
};
typedef struct overlap *Overlap;

#define DEP_ON_CONNECTIONS	1
#define DEP_ON_POSITIONS	2
#define REF_TO_POSITIONS	3
#define REF_TO_CONNECTIONS	4
#define NO_DEP_OR_REF		5

/*
 * We build a hash table for each partition that identifies its external
 * vertices.  Each element contains the coordinates and the index in the 
 * vertex list, and is keyed by the coordinate.
 */
struct exVert
{
    float 			point[3];
    int				vIndex;
};
typedef struct exVert *ExVert;
typedef HashTable Externals;

/*
 * We use a hash table to store the vertices that have been added to a
 * overlap region.  These elements contain and are keyed by the vertex ID
 * in the current ID and also contain the vertex IDs in the output.
 */
struct ptHash
{
    int				hisIndex;
    int				ring;
};
typedef struct ptHash *PtHash;

/*
 * Three parallel computation control blocks.
 *
 * The logic of this algorithm uses two nParts by nParts arrays:
 *
 *	1.  neighbors: neighbors[i][j] = TRUE if partition i and partition j
 *	    are neighbors.
 *
 *	2.  overlaps: overlaps[i][j] is a field containing the portion of 
 *	    partition i that is to be included into partition j.
 *
 * In fact, one block of memory is reused.  Type ij is used.
 */

union ij
{
    long      			neighbor;
    Overlap    			overlap;
};
typedef union  ij *IJ;

/*
 * First, determine the external vertices for each field and create a 
 * hash table that is keyed by the coordinate point and contains the 
 * vertex ID of the vertex in the field's vertex list.
 */

struct task0Params
{
    Field     			field;
    Externals 			*externals;
};
typedef struct task0Params *Task0Params;

/*
 * Given a partition and its neighbors, produce the partition's
 * boundary sets: the sets of its external vertices that are shared with
 * each of its neighbors.  Each of the arrays here is nParts long;
 * a result for the j-th element of the boundaries list is created if
 * from the j-th external vertices table if neighbors[j] == TRUE
 * (indicating tehat partition j is a neighbor of partition i), NULL
 * otherwise.  In this task neighbors and boundaries are 1-D arrays
 * correspond to the i-th row of the corresponding 2-D arrays.
 *
 * Then, given a partition field and a list of its boundary sets, determine the 
 * sub-fields that contain the overlap seeded by each of the boundary sets.  
 * These fields * are as usual, except that elements may refer to external
 * vertices in the partition that they will be added to by the use of 
 * negative offsets: an element refers to the k-th vertex in the partition 
 * vertex list by referencing vertex -(k+1) (note: the +1 avoids the issue 
 * of 0). Boundary j, containing the region of the field that must be accrued
 * onto partition j, is created if there is a boundary set in boundaries[j].
 * In this task boundaries and overlaps are 1-D arrays corresponding to the
 * i-th rows of the corresponding 2-D arrays.
 */
struct task1Params
{
    int       i;	    /* index of partition in list		     */
    int	      nParts; 	    /* number of partitions in list		     */
    int	      nRings;	    /* number of growth rings			     */
    Field     field;	    /* field of interest			     */
    IJ	      ij;           /* T/F neighbors flags and/or overlap 	     */
    Externals *externals;   /* external vertex hash lists for each partition */
    Array     components;   /* components to be grown			     */
};
typedef struct task1Params *Task1Params;

/*
 * Finally, given a partition and its overlap fields, create the final
 * result.  To do so we accrue onto each partition the portion of its
 * neighbors that itersect its overlap region.  For partition j, this
 * information is held in the j-th column (note: NOT row) of the overlaps
 * array.  The appropriate elements of this array are found at indices
 * j + (n * nParts).
 */
struct task2Params
{
    int       j;
    int	      nParts;
    Field     field;
    IJ        ij;
    Array     components;
};
typedef struct task2Params *Task2Params;


static  Externals	NewExternals();
static  Error		FreeExternals(Externals);
static  ExVert		QueryExternals(Externals, float *);
static  Error		AddExternalVertex(Externals, float *, int, int);

#define NewBoundary	(Boundary)NewList
#define FreeBoundary(B) FreeList((List)(B))
static  Error		AddBoundary(Boundary, int, int);

static  Error	   	MakeExternals(Pointer);
static  Error 		MakeBoundaries(Pointer);
static  Error 		InvalidateBoundaryDuplicates(Pointer);
static  Error 	   	AddOverlap(Pointer);

static  Boundary 	MakeBoundarySet(Externals, Externals);
static  Overlap 	MakeOverlap(Field, Boundary, int, char **components);
static  Error 		FreeOverlap(Overlap);

static  Error		FreeExternalsList(Externals *, int);
static  Error	 	FreeIJ(IJ, int);

static  IJ 	   	FindNeighborPartitions(Field *, int);

static  List		NewList();
static  Error		FreeList(List);
static  Link 		GetFreeLink(List);
static  Error 	   	AddLinkBlock(List);

static  Error		IrregShrinkField(Pointer);
static  Group		IrregShrinkGroup(Group);

extern  Array 		_dxfReRef(Array, int);
static  Array		TruncateArray(Array, int);

extern  Error		_dxf_RemoveDupReferences(Field);

/*
 * Hash and compare functions for external vertex hash tables
 */
static PseudoKey   	VertexHash(Key);
static int		VertexCmp(Key, Key);

#define GetArray(f, n) \
	(Array)DXGetComponentValue((f), (n))

#define GetEArray(f,i,n) \
	(Array)DXGetEnumeratedComponentValue((f), (i), (n))

#define GetAttr(f,n,a) \
    DXGetComponentAttribute((f), (n), (a)) ?				\
	DXGetString((String)DXGetComponentAttribute((f), (n), (a))) :	\
	NULL;
    
#define GetEAttr(f,i,n,a) \
    DXGetEnumeratedComponentAttribute((f), (i), (n), (a))) ?		      \
	DXGetString((String)DXGetEnumeratedComponentAttribute((f),(i),(n),(a))) : \
	NULL;
    
#define SetOrigName(buf, name)	sprintf((buf), "original %s", (name))

static char **GetComponentNames(Array);
static void FreeComponentNames(char **);


Object
_dxfIrregGrow(Group group, int nRings, Array components)
{
    int 	   	  i, j;
    int		   	  nParts = 0, nNonEmptyFields = 0;
    Field		  *fields;
    Externals	   	  *externals;
    IJ		   	  ij;
    struct task0Params 	  task0;
    struct task1Params 	  task1;
    struct task2Params    task2;
    Object		  obj;

    fields = NULL;
    externals = NULL;
    ij = NULL;
    nParts = 0;


    if (DXGetGroupClass(group) != CLASS_COMPOSITEFIELD)
    {
	DXSetError(ERROR_DATA_INVALID, "IrregGrow: object not composite field");
        goto error;
    }

    /*
     * Count the partitions and create appropriately-sized lists for
     * the consituent fields and corresponding hash tables
     */
    nNonEmptyFields = 0; nParts = 0;
    for (j = 0; NULL != (obj = DXGetEnumeratedMember(group, j, NULL)); j++)
    {
	if (DXGetObjectClass(obj) == CLASS_FIELD && !DXEmptyField((Field)obj))
	    nNonEmptyFields ++;

	nParts ++;
    }
    
    fields = (Field *)DXAllocate(nParts*sizeof(Field));
    if (! fields)
        goto error;

    j = 0;
    for (i = 0; i < nParts; i++)
    {
	fields[j] = (Field)DXGetEnumeratedMember(group, i, NULL);
	if (DXGetObjectClass((Object)fields[j]) == CLASS_FIELD 
					&& !DXEmptyField(fields[j]))
	    j++;
	
	/*
	 * Do a little checking on the first non-empty field found
	 */
	if (j == 1)
	{
	    Object attr;
	    char *eltType;
	
	    attr = DXGetComponentAttribute(fields[0], "connections", 
								"element type");
	    if (! attr || NULL == (eltType = DXGetString((String)attr)))
	    {
		DXSetError(ERROR_MISSING_DATA, "missing element type attribute");
		goto error;
	    }

	    if (strcmp(eltType, "tetrahedra") &&
	        strcmp(eltType, "cubes") &&
	        strcmp(eltType, "triangles") &&
	        strcmp(eltType, "quads"))
	    {
		DXFree((Pointer)fields);
		return (Object)group;
	    }
	}
    }
    
    externals = (Externals *)DXAllocateZero(nNonEmptyFields*sizeof(Externals));
    if (! externals)
        goto error;

    if (! DXCreateTaskGroup())
	goto error;

    /*
     * For each component field, create the externals table
     */
    for (i = 0; i < nNonEmptyFields; i++)
    {
	task0.field     = fields[i];
	task0.externals = externals + i;

	if (! DXAddTask(MakeExternals, (Pointer)&task0, sizeof(task0), 1.0))
	{
	    DXAbortTaskGroup();
	    goto error;
	}
    }

    if (! DXExecuteTaskGroup() || DXGetError() != ERROR_NONE)
	goto error;

    ij = FindNeighborPartitions(fields, nNonEmptyFields);
    if (! ij)
	goto error;

    if (! DXCreateTaskGroup())
	goto error;

    for (i = 0; i < nNonEmptyFields; i++)
    {
	task1.i	           = i;
	task1.nParts       = nNonEmptyFields;
	task1.nRings 	   = nRings;
	task1.field 	   = fields[i];
	task1.externals    = externals;
	task1.components   = components;
	task1.ij	   = ij + (i * nNonEmptyFields);
    
	if (! DXAddTask(MakeBoundaries, (Pointer)&task1, sizeof(task1), 1.0))
	{
	    DXAbortTaskGroup();
	    goto error;
	}
    }

    if (! DXExecuteTaskGroup() || DXGetError() != ERROR_NONE)
	goto error;

    if (! DXCreateTaskGroup())
	goto error;

    for (j = 0; j < nNonEmptyFields; j++)
    {
	task2.j	           = j;
	task2.nParts 	   = nNonEmptyFields;
	task2.field 	   = fields[j];
	task2.components   = components;
	task2.ij  	   = ij;
    
	DXAddTask(AddOverlap, (Pointer)&task2, sizeof(task2), 1.0);
    }

    if (! DXExecuteTaskGroup() || DXGetError() != ERROR_NONE)
	goto error;

    DXFree((Pointer)fields);

    if (ij)
	FreeIJ(ij, nNonEmptyFields);

    if (externals)
	FreeExternalsList(externals, nNonEmptyFields);

    return (Object)group;

error:

    DXFree((Pointer)fields);

    if (ij)
	FreeIJ(ij, nNonEmptyFields);

    if (externals)
	FreeExternalsList(externals, nNonEmptyFields);
	
    return NULL;
}

Object
_dxfIrregInvalidateDupBoundary(Group group)
{
    int 	   	  i, j;
    int		   	  nParts = 0, nNonEmptyFields = 0;
    Field		  *fields;
    Externals	   	  *externals;
    IJ		   	  ij;
    struct task0Params 	  task0;
    struct task1Params 	  task1;
    Object		  obj;

    fields = NULL;
    externals = NULL;
    ij = NULL;
    nParts = 0;

    if (DXGetGroupClass(group) != CLASS_COMPOSITEFIELD)
    {
	DXSetError(ERROR_DATA_INVALID, "object not composite field");
        goto error;
    }

    /*
     * Count the partitions and create appropriately-sized lists for
     * the consituent fields and corresponding hash tables
     */
    nNonEmptyFields = 0; nParts = 0;
    for (j = 0; NULL != (obj = DXGetEnumeratedMember(group, j, NULL)); j++)
    {
	if (DXGetObjectClass(obj) == CLASS_FIELD)
	{
	    Array a;

	    if (!DXEmptyField((Field)obj))
		nNonEmptyFields ++;
	
	    a = (Array)DXGetComponentValue((Field)obj, "invalid positions");
	    if (a)
	    {
		if (! DXSetComponentValue((Field)obj,
			"saved invalid positions", (Object)a))
		    goto error;
	    }
	}

	nParts ++;
    }
    
    fields = (Field *)DXAllocate(nParts*sizeof(Field));
    if (! fields)
        goto error;

    j = 0;
    for (i = 0; i < nParts; i++)
    {
	fields[j] = (Field)DXGetEnumeratedMember(group, i, NULL);
	if (DXGetObjectClass((Object)fields[j]) == CLASS_FIELD 
					&& !DXEmptyField(fields[j]))
	    j++;
	
	/*
	 * Do a little checking on the first non-empty field found
	 */
	if (j == 1)
	{
	    Object attr;
	
	    attr = DXGetComponentAttribute(fields[0], "connections", 
							    "element type");
	    /*
	     * If there are no connections, then there won't be any
	     * shared boundary positions
	     */
	    if (! DXGetComponentValue(fields[0], "connections"))
		goto done;

	    if (! attr || NULL == DXGetString((String)attr))
	    {
		DXSetError(ERROR_MISSING_DATA,
			"missing element type attribute");
		goto error;
	    }
	}
    }
    
    externals =
	(Externals *)DXAllocateZero(nNonEmptyFields*sizeof(Externals));
    if (! externals)
        goto error;

    if (! DXCreateTaskGroup())
	goto error;

    /*
     * For each component field, create the externals table
     */
    for (i = 0; i < nNonEmptyFields; i++)
    {
	task0.field     = fields[i];
	task0.externals = externals + i;

	if (! DXAddTask(MakeExternals, (Pointer)&task0, sizeof(task0), 1.0))
	{
	    DXAbortTaskGroup();
	    goto error;
	}
    }

    if (! DXExecuteTaskGroup() || DXGetError() != ERROR_NONE)
	goto error;

    ij = FindNeighborPartitions(fields, nNonEmptyFields);
    if (! ij)
	goto error;

    if (! DXCreateTaskGroup())
	goto error;

    for (i = 0; i < nNonEmptyFields; i++)
    {
	task1.i	           = i;
	task1.nParts       = nNonEmptyFields;
	task1.field 	   = fields[i];
	task1.externals    = externals;
	task1.ij	   = ij + (i * nNonEmptyFields);
    
	if (! DXAddTask(InvalidateBoundaryDuplicates,
			(Pointer)&task1, sizeof(task1), 1.0))
	{
	    DXAbortTaskGroup();
	    goto error;
	}
    }

    if (! DXExecuteTaskGroup() || DXGetError() != ERROR_NONE)
	goto error;

done:
    DXFree((Pointer)fields);

    if (ij)
	FreeIJ(ij, nNonEmptyFields);

    if (externals)
	FreeExternalsList(externals, nNonEmptyFields);

    return (Object)group;

error:

    DXFree((Pointer)fields);

    if (ij)
	FreeIJ(ij, nNonEmptyFields);

    if (externals)
	FreeExternalsList(externals, nNonEmptyFields);
	
    return NULL;
}

struct shrinkTask
{
    Object	object;
};

Object
_dxfIrregShrink(Object object)
{
    int    i;
    Object child;
    struct shrinkTask task;
    Class  class;

    class = DXGetObjectClass(object);

    switch(class)
    {
	case CLASS_FIELD:

	    if (! DXEmptyField((Field)object))
		if (! IrregShrinkField((Pointer)&object))
		    goto error;
	    
	    break;

        case CLASS_GROUP:
   
	    if (! DXCreateTaskGroup())
		goto error;

	    i = 0;
	    while (NULL != 
		(child = DXGetEnumeratedMember((Group)object, i++, NULL)))
	    {
	    
		if (DXGetObjectClass(child), CLASS_FIELD)
		{
		    if (! DXEmptyField((Field)child))
		    {
			task.object = child;
			if (!DXAddTask(IrregShrinkField, 
					(Pointer)&task, sizeof(task),1.0))
			{
			    DXAbortTaskGroup();
			    goto error;
			}
		    }
		    
		}
		else
		{
		    if (! _dxfIrregShrink(child))
		    {
			DXAbortTaskGroup();
			goto error;
		    }
		}
	    }

	    if (! DXExecuteTaskGroup() || DXGetError() != ERROR_NONE)
		return NULL;
	
	    break;
	
	case CLASS_XFORM:

	    if (! DXGetXformInfo((Xform)object, &child, 0))
		goto error;
	    
	    if (! _dxfIrregShrink(child))
		goto error;
	    
	    break;
	
	default:
	    
	    DXSetError(ERROR_DATA_INVALID, "unknown object");
	    goto error;
    }
	    
    return object;

error:
    return NULL;
}

static Group
IrregShrinkGroup(Group group)
{
    int    i;
    Object child;
    struct shrinkTask task;

    i = 0;
    while (NULL != (child = DXGetEnumeratedMember(group, i++, NULL)))
    {
	if (DXGetObjectClass(child) == CLASS_GROUP)
	{
	    if (! IrregShrinkGroup((Group)child))
	    {
		DXAbortTaskGroup();
		return NULL;
	    }
	}
	else if (DXGetObjectClass(child) == CLASS_FIELD)
	{
	    task.object = child;
	    DXAddTask(IrregShrinkField, (Pointer)&task, sizeof(task), 1.0);
	}
    }

    return group;
}

static Error
IrregShrinkField(Pointer ptr)
{
    Field 	field;
    Array 	oArray, nArray = NULL;
    int   	i;
    char  	*attr, *name;
    int   	nPositions, nConnections, dLength, rLength = -1, aLength;
    char   	*oComponents[256];
    char   	*sComponents[256];
    char        origName[256];
    int		oC, sC;

    field = (Field)((struct shrinkTask *)ptr)->object;

    if (! DXQueryOriginalSizes(field, &nPositions, &nConnections))
	return OK;

    DXDeleteComponent(field, "box");
    DXDeleteComponent(field, "data statistics");

    /*
     * Get a list of components to be either replaced by originals or shrunk
     */
    for (i = oC = sC = 0; DXGetEnumeratedComponentValue(field, i, &name); i++)
    {
	if (! strncmp(name, "original", 8))
	    oComponents[oC ++] = name;
	else
	{
	    SetOrigName(origName, name);
	    if (! DXGetComponentValue(field, origName))
		sComponents[sC ++] = name;
	}
    }
	
    /*
     * Now replace them.  UNLESS they are zero length, in which there was no component
     * in this field to correspond to a component present in a neighbor.  In this case 
     * both the original and grown version are removed.
     */
    for (i = 0; i < oC; i++)
    {
	int   nItems;

	oArray = (Array)DXGetComponentValue(field, oComponents[i]);


	DXGetArrayInfo(oArray, &nItems, NULL, NULL, NULL, NULL);

	if (nItems)
	{
	    DXDeleteComponent(field, oComponents[i]+9);

	    if (! DXSetComponentValue(field, oComponents[i]+9, (Object)oArray))
		goto error;
	}
	else
	{
	    DXDeleteComponent(field, oComponents[i]+9);
	}

	DXDeleteComponent(field, oComponents[i]);
    }

    /*
     * Now go through the field shrinking any components that need shrinking.   For a
     * dependent array, we first delete any elements past the pre-grown length of
     * the component the array deps on.  For referential arrays we re-map references
     * outside the new size of the referred-to component to -1.  For arrays that are
     * referential but not dependent, we remove any references that point outside the
     * new size of the referred-to component.
     */
    for (i = 0; i < sC; i++)
    {
	name = sComponents[i];

	oArray = (Array)DXGetComponentValue(field, name);
	if (! oArray)
	{
	    DXSetError(ERROR_INTERNAL, "coundn't re-access %s", name);
	    goto error;
	}

	/*
	 * Otherwise, need to look closely to decide whether explicit
	 * shrinking is required.  Look to see what its dependent on
	 * and compare the counts. If it ain't dependent on something,
	 * get rid of it.
	 */
	if (! strcmp(name, "connections"))
	    dLength = nConnections;
	else
	{
	    attr = GetAttr(field, name, "dep");
	    if (! attr)
		dLength = -1;
	    else if (! strcmp(attr, "positions"))
		dLength = nPositions;
	    else if (! strcmp(attr, "connections"))
		dLength = nConnections;
	}

	attr = GetAttr(field, name, "ref");
	if (! attr)
	    rLength = -1;
	else if (! strcmp(attr, "positions"))
	    rLength = nPositions;
	else if (! strcmp(attr, "connections"))
	    rLength = nConnections;

	DXGetArrayInfo(oArray, &aLength, NULL, NULL, NULL, NULL);

	/*
	 * Doesn't dep anything, doesn't ref anything, whats to do?
	 */
	if (rLength == -1 && dLength == -1)
	    continue;

	/*
	 * If this array does not dep anything, then count the references that will
	 * be valid in the post-shrink world.  Create a new array of the appropriate
	 * length, and copy in the valid re-mapped references.
	 */
	if (rLength != -1 && dLength == -1)
	{
	    int i, j, k, nRefs, nItems, nVItems, *sPtr, *dPtr, shape[32], rank;
	    Type t; Category c;

	    DXGetArrayInfo(oArray, &nItems, &t, &c, &rank, shape);
	    if (t != TYPE_INT)
	    {
		DXSetError(ERROR_INTERNAL, "referential component not type INT");
		goto error;
	    }

	    nRefs = DXGetItemSize(oArray) / sizeof(int);
	    sPtr = (int *)DXGetArrayData(oArray);

	    nVItems = 0;
	    for (i = nVItems = 0; i < nItems; i++, sPtr += nRefs)
	    {
		for (j = k = 0; !k && j < nRefs; j++)
		    if (sPtr[j] < rLength)
			k++;

		if (k)
		    nVItems++;
	    }

	    if (nVItems == 0)
	    {
		nArray = NULL;
	    }
	    else
	    {
		nArray = DXNewArrayV(t, c, rank, shape);
		if (! nArray)
		    goto error;
	    
		if (! DXAddArrayData(nArray, 0, nVItems, NULL))
		    goto error;
		
		sPtr = (int *)DXGetArrayData(oArray);
		dPtr = (int *)DXGetArrayData(nArray);

		for (i = 0; i < nItems; i++, sPtr += nRefs)
		{
		    for (j = k = 0; !k && j < nRefs; j++)
			if (sPtr[j] < rLength)
			    k++;
		    
		    if (k) {
			for (j = 0; j < nRefs; j++)
			    if (sPtr[j] < rLength)
				*dPtr++ = sPtr[j];
			    else
				*dPtr++ = -1;
		    }
		}
	    }
	}
	else
	{
	    nArray = oArray;

	    /*
	     * If the lengths mismatch, truncate the array.  Note that this 
	     * creates a new array.
	     */
	    if (aLength != dLength)
	    {
		nArray = TruncateArray(oArray, dLength);
		if (! nArray)
		    goto error;
	    }

	    /*
	     * Look to see if we need to fix it up
	     */
	    if (rLength != -1)
	    {
		int i, nRefs, *sPtr, *dPtr, shape[32], rank;
		Type t; Category c;

		DXGetArrayInfo(nArray, &dLength, &t, &c, &rank, shape);
		if (t != TYPE_INT)
		{
		    DXSetError(ERROR_INTERNAL, "referential component not type INT");
		    goto error;
		}

		nRefs = (dLength * DXGetItemSize(nArray)) / sizeof(int);

		sPtr = (int *)DXGetArrayData(nArray);

		for (i = 0; i < nRefs; i++)
		    if (*sPtr++ >= rLength)
			break;
		    
		if (i != nRefs)
		{
		    /*
		     * We need to create a new array if we didnt do so
		     * above.
		     */
		    if (nArray == oArray)
		    {
			nArray = DXNewArrayV(t, c, rank, shape);
			if (! nArray)
			    goto error;
			
			if (! DXAddArrayData(nArray, 0, rLength, NULL))
			    goto error;
		    }

		    sPtr = (int *)DXGetArrayData(oArray);
		    dPtr = (int *)DXGetArrayData(nArray);

		    for (i = 0; i < nRefs; i++, sPtr++, dPtr++)
			if (*sPtr >= rLength)
			    *dPtr = -1;
			else
			    *dPtr = *sPtr;
		}
	    }
	}

	if (oArray != nArray)
	    if (! DXSetComponentValue(field, name, (Object)nArray))
		goto error;
    }

    if (! DXEndField(field))
	goto error;

    return OK;

error:
    DXDelete((Object)nArray);

    return ERROR;
}

static Array
TruncateArray(Array in, int length)
{
    Array    out = NULL;
    Pointer  o = NULL, d = NULL;
    int      r, s[32];
    Type     t;
    Category c;

    DXGetArrayInfo(in, NULL, &t, &c, &r, s);

    if (DXQueryConstantArray(in, NULL, NULL))
    {
	out = (Array)DXNewConstantArray(length, DXGetConstantArrayData(in), t, c, r, s);
    }
    else if (DXGetArrayClass(in) == CLASS_REGULARARRAY)
    {
	if (NULL == (o = DXAllocate(DXGetItemSize(in))))
	    goto error;

	if (NULL == (d = DXAllocate(DXGetItemSize(in))))
	    goto error;

	DXGetRegularArrayInfo((RegularArray)in, NULL, o, d);

	out = (Array)DXNewRegularArray(t, s[0], length, o, d);

	DXFree(o);
	DXFree(d);
    }
    else
    {
	out = DXNewArrayV(t, c, r, s);
	if (! out)
	    goto error;

	if (! DXAddArrayData(out, 0, length, DXGetArrayData(in)))
	    goto error;
    }

    return out;

error:
    DXFree(o);
    DXFree(d);
    DXDelete((Object)out);
    return NULL;
}


static Error 
AddOverlap(Pointer ptr)
{
    int       i, j, part, itemSize;
    Field     field;
    int	      nParts;
    Overlap   overlap, *overlapPtr;
    int	      nElements, nPoints, nComponents, newElements, newPoints;
    int	      oElements, oPoints;
    char      origName[256];
    int	      *src, *dst;
    Array     orig, array;
    Type      type;
    Category  cat;
    int       rank, shape[32], nItems;
    char      *names[100];
    Object    obj;
    char      *components[256];
    int       *map = NULL;
    byte      *flags = NULL;
    HashTable hash = NULL;
    Array     cArray;

    nParts 	= ((Task2Params)ptr)->nParts;
    field 	= ((Task2Params)ptr)->field;
    cArray 	= ((Task2Params)ptr)->components;
    overlapPtr  = ((Overlap *)((Task2Params)ptr)->ij) + ((Task2Params)ptr)->j;

    DXDeleteComponent(field, "neighbors");

    /*
     * Place refs to positions and connections into
     * "original ..." in case no neighbors contribute and
     * force it later.
     */
    array = (Array)DXGetComponentValue(field, "connections");
    if (array)
    {
	DXGetArrayInfo(array, &oElements, NULL, NULL, NULL, NULL);
	if (! DXSetComponentValue(field, "original connections", (Object)array))
	    goto error;
    }
    else
	oPoints = 0;

    array = (Array)DXGetComponentValue(field, "positions");
    if (array)
    {
	DXGetArrayInfo(array, &oPoints, NULL, NULL, NULL, NULL);
	if (! DXSetComponentValue(field, "original positions", (Object)array))
	    goto error;
    }
    else
	oPoints = 0;
    
    if (cArray)
    {
	char **list, **l;

	list = GetComponentNames(cArray);
	if (! list)
	    goto error;

	for (l = list; *l != NULL; l++)
	    if (strcmp(*l, "positions") && strcmp(*l, "connections"))
	    {
		Array a = (Array)DXGetComponentValue(field, *l);
		if (a)
		{   
		    SetOrigName(origName, *l);
		    if (! DXSetComponentValue(field, origName, (Object)a))
		    {
			FreeComponentNames(list);
			goto error;
		    }
		}
	    }

	FreeComponentNames(list);
    }
			
    
    nPoints   = oPoints;
    nElements = oElements;

    /*
     * Mark the current contents so we can get rid of the
     * overlap later
     */
    nComponents = 0;
    for (part = 0; part < nParts; part++, overlapPtr += nParts)
    {
	overlap = *overlapPtr;

	if (! overlap)
	    continue;

	/*
	 * Now add the overlap onto the current field
	 */
	for (i = 0; i < overlap->nComponents; i++)
	{
	    byte arrayType = NO_DEP_OR_REF;

	    /*
	     * Get the target component, if it exists
	     */
	    array = (Array)DXGetComponentValue(field, overlap->names[i]);

	    /*
	     * If array doesn't exist, then this is a component present in a neighbor
	     * that was not present in the current partition.  So we create an empty
	     * component of the same type as the overlap and pretend it was there to
	     * begin with.
	     */
	    if (! array)
	    {
		DXGetArrayInfo(overlap->arrays[i], NULL, &type, &cat, &rank, shape);

		array = DXNewArrayV(type, cat, rank, shape);
		if (! array)
		    goto error;
		
		if (! DXCopyAttributes((Object)array, (Object)overlap->arrays[i]))
		{
		    DXDelete((Object)array);
		    goto error;
		}
		
		if (! DXSetComponentValue(field, overlap->names[i], (Object)array))
		{
		    DXDelete((Object)array);
		    goto error;
		}

		arrayType = overlap->dependencies[i];
	    }
	    else
	    {
		/*
		 * what type of component is the target?
		 */
		Object attr = DXGetAttribute((Object)array, "dep");
		if (attr)
		{
		    if (DXGetObjectClass(attr) != CLASS_STRING)
		    {
			DXSetError(ERROR_DATA_INVALID, "#10200", "dep attribute");
			goto error;
		    }

		    if (! strcmp(DXGetString((String)attr), "positions"))
			arrayType = DEP_ON_POSITIONS;
		    else if (! strcmp(DXGetString((String)attr), "connections"))
			arrayType = DEP_ON_CONNECTIONS;
		}

		attr = DXGetAttribute((Object)array, "ref");
		if (attr)
		{
		    /*
		     * Note that connections are special cased, so that connections dep 
		     * connections is OK.
		     */
		    if (arrayType != NO_DEP_OR_REF &&
			strcmp(overlap->names[i], "connections"))
		    {
			DXSetError(ERROR_NOT_IMPLEMENTED,
				"cannot grow componentsthat both dep and ref");
			goto error;
		    }
		    
		    if (DXGetObjectClass(attr) != CLASS_STRING)
		    {
			DXSetError(ERROR_DATA_INVALID, "#10200", "ref attribute");
			goto error;
		    }

		    if (! strcmp(DXGetString((String)attr), "positions"))
			arrayType = REF_TO_POSITIONS;
		    else if (! strcmp(DXGetString((String)attr), "connections"))
			arrayType = REF_TO_CONNECTIONS;
		}
	    }
		
	    /*
	     * Look for a component saved under "original..."
	     */
	    SetOrigName(origName, overlap->names[i]);
	    orig  = (Array)DXGetComponentValue(field, origName);

	    /*
	     * If none exists, then we will be growing this component and need
	     * to save the current contents under the "original" name.
	     */
	    if (! orig || orig == array)
	    {
		Array new;

		orig = array;

		DXGetArrayInfo(orig, &nItems, &type, &cat, &rank, shape);

		new = DXNewArrayV(type, cat, rank, shape);
		if (! new)
		    goto error;
		
		if (nItems)
		    if (! DXAddArrayData(new, 0, nItems, DXGetArrayData(orig)))
		    {
			DXDelete((Object)new);
			goto error;
		    }
		
		if (! DXSetComponentValue(field, origName, (Object)orig))
		{
		    DXDelete((Object)new);
		    goto error;
		}

		if (! DXSetComponentValue(field, overlap->names[i], (Object)new))
		{
		    DXDelete((Object)new);
		    goto error;
		}

		array = new;

		components[nComponents++] = overlap->names[i];
	    }

	    itemSize = DXGetItemSize(array);

	    if (! strcmp(overlap->names[i], "connections"))
	    {
		if (! DXAddArrayData(array, nElements, 
						overlap->nElements, NULL))
		    goto error;

		src = (int *)overlap->buffers[i];
		dst = ((int *)DXGetArrayData(array)) + (nElements*itemSize/4);

		for (j = 0; j < (itemSize/4)*overlap->nElements; j++)
		{
		    if (*src < 0)
			*dst++ = (- *src++) - 1;
		    else
			*dst++ = *src++ + nPoints;
		}
	    }
	    else if (overlap->dependencies[i] == REF_TO_POSITIONS) 
	    {
		int j, nRefs, rank;

		DXGetArrayInfo(array, &nItems, NULL, NULL, &rank, &nRefs);
		if (rank != 0 && !(rank == 1 && nRefs != 1))
		{
		    DXSetError(ERROR_DATA_INVALID, 
			"cannot grow ref components that contain >1 refs per item");
		    goto error;
		}

		if (arrayType == REF_TO_POSITIONS)
		{
		    int *src, *dst;

		    if (! DXAddArrayData(array, nItems, overlap->counts[i], NULL))
			goto error;
		    
		    src = (int *)overlap->buffers[i];
		    dst = ((int *)DXGetArrayData(array)) + nItems;
		    for (j = 0; j < overlap->counts[i]; j++)
			if (*src < 0)
			    *dst++ = (- *src++) - 1;
			else
			    *dst++ = nPoints + *src++;
		}
		else if (arrayType == DEP_ON_POSITIONS)
		{
		    int *src;
		    char *dst;

		    if (! DXAddArrayData(array, nPoints, overlap->nPoints, NULL))
			goto error;
		    
		    dst = ((char *)DXGetArrayData(array)) + nPoints;
		    for (j = 0; j < overlap->nPoints; j++)
			dst[j] = 0;
		    
		    src = (int *)overlap->buffers[i];
		    for (j = 0; j < overlap->counts[i]; j++, src++)
			if (*src >= 0)
			    dst[*src] = 1;
		}
	    }
	    else if (overlap->dependencies[i] == REF_TO_CONNECTIONS) 
	    {
		int j, rank, nRefs;

		DXGetArrayInfo(array, &nItems, NULL, NULL, &rank, &nRefs);
		if (rank != 0 && !(rank == 1 && nRefs == 1))
		{
		    DXSetError(ERROR_DATA_INVALID, 
			"cannot grow ref components that contain >1 refs per item");
		    goto error;
		}

		if (arrayType == REF_TO_CONNECTIONS)
		{
		    int *src, *dst;

		    if (! DXAddArrayData(array, nItems, overlap->counts[i], NULL))
			goto error;
		
		    src = (int *)overlap->buffers[i];
		    dst = ((int *)DXGetArrayData(array)) + nItems;
		    for (j = 0; j < overlap->counts[i]; j++)
			if (*src < 0)
			    *dst++ = (- *src++) - 1;
			else
			    *dst++ = nElements + *src++;
		}
		else if (arrayType == DEP_ON_CONNECTIONS)
		{
		    int *src;
		    char *dst;

		    if (! DXAddArrayData(array, nElements,
					overlap->nElements, NULL))
			goto error;
		    
		    dst = ((char *)DXGetArrayData(array)) + nElements;
		    for (j = 0; j < overlap->nElements; j++)
			dst[j] = 0;
		    
		    src = (int *)overlap->buffers[i];
		    for (j = 0; j < overlap->counts[i]; j++, src++)
			if (*src >= 0)
			    dst[*src] = 1;
		}
	    }
	    else if (overlap->dependencies[i] == DEP_ON_POSITIONS)
	    {
		if (arrayType == DEP_ON_POSITIONS)
		{
		    if (! DXAddArrayData(array, nPoints, 
				    overlap->counts[i], overlap->buffers[i]))
			goto error;
		}
		else if (arrayType == REF_TO_POSITIONS)
		{
		    char *src = (char *)overlap->buffers[i];
		    int  *dst, nRefs, rank;
		    int j, knt;

		    DXGetArrayInfo(array, &nItems, NULL, NULL, &rank, &nRefs);
		    if (rank != 0 && !(rank == 1 && nRefs == 1))
		    {
			DXSetError(ERROR_DATA_INVALID, 
		"cannot grow ref components that contain >1 refs per item");
			goto error;
		    }

		    for (j = knt = 0; j < overlap->nPoints; j++)
		 	if (src[j])
			    knt ++;
		    
		    if (! DXAddArrayData(array, nItems, knt, NULL))
			goto error;
		    
		    dst = ((int *)DXGetArrayData(array)) + nItems;
		    
		    for (j = 0; j < overlap->nPoints; j++)
			if (src[j])
			    *dst++ = nPoints + j;
		}
	    }
	    else if (overlap->dependencies[i] == DEP_ON_CONNECTIONS)
	    {
		if (arrayType == DEP_ON_CONNECTIONS)
		{
		    if (! DXAddArrayData(array, nElements, 
				    overlap->counts[i], overlap->buffers[i]))
			goto error;
		}
		else if (arrayType == REF_TO_CONNECTIONS)
		{
		    char *src = (char *)overlap->buffers[i];
		    int  *dst, nRefs, rank;
		    int j, knt;

		    DXGetArrayInfo(array, &nItems, NULL, NULL, &rank, &nRefs);
		    if (rank != 0 && !(rank == 1 && nRefs == 1))
		    {
			DXSetError(ERROR_DATA_INVALID, 
		"cannot grow ref components that contain >1 refs per item");
			goto error;
		    }

		    for (j = knt = 0; j < overlap->nElements; j++)
		 	if (src[j]) knt ++;
		    
		    if (! DXAddArrayData(array, nItems, knt, NULL))
			goto error;
		    
		    dst = ((int *)DXGetArrayData(array)) + nItems;
		    
		    for (j = 0; j < overlap->nElements; j++)
			if (src[j])
			    *dst++ = nElements + j;
		}
	    }

	}

	nPoints   += overlap->nPoints;
	nElements += overlap->nElements;

	FreeOverlap(overlap);
	*overlapPtr = NULL;
    }

    /*
     * We may have added the same geometrical point more than once, once
     * for each neighbor that contained it.  We need to remove these duplicates
     * so that neighbors works.  Note that its only a problem for vertices
     * that did not lie in the original field.
     */
    newPoints = nPoints - oPoints;
    newElements  = nElements - oElements;
    if (newPoints > 0 || newElements > 0)
    {
	int       	*ePtr;
	int 	  	i, j, nDim, dupKnt;
	struct exVert	nxtVert, *dupPtr;
	float		*points;
	int		nextUnique;
	
	array = (Array)DXGetComponentValue(field, "positions");
	if (! array)
	    goto error;
	
	DXGetArrayInfo(array, NULL, NULL, NULL, NULL, &nDim);
	points = ((float *)DXGetArrayData(array)) + oPoints*nDim;

	map = (int *)DXAllocate(newPoints * sizeof(int));
	flags = (byte *)DXAllocate(newPoints);
	if (! map || ! flags)
	    goto error;

	hash = DXCreateHash(sizeof(struct exVert), VertexHash, VertexCmp);
	if (! hash)
	    goto error;

	for (j = nDim; j < 3; j++)
	    nxtVert.point[j] = 0.0;

	nextUnique = 0;
	for (i = 0, dupKnt = 0; i < newPoints; i++)
	{
	    for (j = 0; j < nDim; j++)
		nxtVert.point[j] = *points++;
	    
	    dupPtr = (struct exVert *)DXQueryHashElement(hash, (Key)&nxtVert);
	    if (dupPtr)
	    {
		flags[i] = 0;
		map[i] = dupPtr->vIndex;
		dupKnt ++;
	    }
	    else
	    {
		flags[i] = 1;
		map[i] = nxtVert.vIndex = oPoints + nextUnique++;
		if (! DXInsertHashElement(hash, (Element)&nxtVert))
		    goto error;
	    }
	}

	DXDestroyHash(hash);
	hash = NULL;

	/*
	 * If there were duplicate positions, then we need to re-map the
	 * referential components and cull the unreferenced positions and 
	 * associated positions dependent data
	 */
	if (dupKnt)
	{
	    /*
	     * Now for each component do the right thing.  If it refs positions, re-map
	     * the references.  If it deps positions, delete the redundant elements.
	     */
	    for (i = 0; i < nComponents; i++)
	    {
		int dep;
		int oKnt = 0, nKnt = 0;

		Array src = GetArray(field, components[i]);
		if (! src)
		    goto error;

		obj = DXGetAttribute((Object)src, "dep");
		if (! obj)
		    continue;
		
		if (DXGetObjectClass(obj) != CLASS_STRING)
		{
		    DXSetError(ERROR_DATA_INVALID, "#10200", "dep attribute");
		    goto error;
		}

		if (! strcmp(DXGetString((String)obj), "positions"))
		{
		    dep = DEP_ON_POSITIONS;
		    oKnt = oPoints;
		    nKnt = newPoints;
		}
		else if (! strcmp(DXGetString((String)obj), "connections"))
		{
		    dep = DEP_ON_CONNECTIONS;
		    oKnt = oElements;
		    nKnt = newElements;
		}
		else
		{
		    dep = 0;
		    SetOrigName(origName, components[i]);
		    orig = (Array)DXGetComponentValue(field, origName);
		    if (! orig)
			oKnt = 0;

		    DXGetArrayInfo(src, &nKnt, NULL, NULL, NULL, NULL);
		}

		obj = DXGetAttribute((Object)src, "ref");
		if (! obj)
		    continue;
		
		if (DXGetObjectClass(obj) != CLASS_STRING)
		{
		    DXSetError(ERROR_DATA_INVALID, "#10200", "dep attribute");
		    goto error;
		}

		if (! strcmp(DXGetString((String)obj), "positions"))
		{
		    int j;

		    DXGetArrayInfo(array, NULL, NULL, NULL, NULL, &nDim);

		    ePtr = ((int *)DXGetArrayData(src)) + oKnt*nDim;

		    for (j = 0; j < nKnt*nDim; j++)
		    {
			if (*ePtr >= oPoints)
			    *ePtr = map[*ePtr - oPoints];
		    
			ePtr ++;
		    }
		}

		if (dep == DEP_ON_POSITIONS)
		{
		    Array dst = NULL;
		    Type t; Category c; int r, s[32];
		    char *dPtr, *sPtr;
		    int iSize, knt;

		    DXGetArrayInfo(src, NULL, &t, &c, &r, s);

		    dst = DXNewArrayV(t, c, r, s);
		    if (! dst)
			goto error;
		    
		    if (! DXAddArrayData(dst, 0, nPoints-dupKnt, NULL))
		    {
			DXDelete((Object)dst);
			goto error;
		    }
		    
		    sPtr = (char *)DXGetArrayData(src);
		    dPtr = (char *)DXGetArrayData(dst);

		    iSize = DXGetItemSize(src);
		    
		    knt = oPoints;
		    for (j = 0; j < newPoints; j++)
			if (! flags[j])
			{
			    memcpy(dPtr, sPtr, knt*iSize);
			    dPtr += knt * iSize;
			    sPtr += (knt+1) * iSize;
			    knt = 0;
			}
			else
			    knt ++;
		    
		    if (knt)
			memcpy(dPtr, sPtr, knt*iSize);
		    
		    if (! DXSetComponentValue(field, names[i], (Object)dst))
		    {
			DXDelete((Object)dst);
			goto error;
		    }
		    dst = NULL;

		}
	    }
	}

	DXFree((Pointer)map);
	DXFree((Pointer)flags);
    }

    if (! _dxf_RemoveDupReferences(field))
	goto error;

    if (!DXEndField(field))
	goto error;

    return OK;

error:

    DXDestroyHash(hash);
    DXFree((Pointer)flags);
    DXFree((Pointer)map);
    return ERROR;
}

static Overlap
MakeOverlap(Field field, Boundary boundary, int nRings, char **components)
{
    int 	  i, j, k, e, v;
    int		  nPoints, nDim, nElts, vPerE, nComponents;
    Overlap	  overlap;
    Array	  pArray, cArray, array;
    int		  *eltHash, *eltPtr;
    PtHash	  ptHash, ptPtr;
    Link	  b;
    int		  *elements, *elt;
    int		  vertKnt, eltKnt;
    int		  ring;
    char	  *name, **cmp;
    int		  doit;

    overlap = NULL;
    eltHash = NULL;
    ptHash  = NULL;

    pArray = (Array)DXGetComponentValue(field, "positions");
    DXGetArrayInfo(pArray, &nPoints, NULL, NULL, NULL, &nDim);
    /*points = (float *)DXGetArrayData(pArray);*/

    cArray = (Array)DXGetComponentValue(field, "connections");
    DXGetArrayInfo(cArray, &nElts, NULL, NULL, NULL, &vPerE);
    elements = (int *)DXGetArrayData(cArray);

    eltHash = (int *)DXAllocateLocal(nElts * sizeof(int));
    if (! eltHash)
    {
	eltHash = (int *)DXAllocate(nElts * sizeof(int));
	if (! eltHash)
	    goto MakeOverlap_error;
    }

    for (i = 0, eltPtr = eltHash; i < nElts; i++)
       *eltPtr++ = -1;

    ptHash = (PtHash)DXAllocateLocal(nPoints * sizeof(struct ptHash));
    if (! ptHash)
    {
	ptHash = (PtHash)DXAllocate(nPoints * sizeof(struct ptHash));
	if (! ptHash)
	    goto MakeOverlap_error;
    }



    /*
     * Initially, included positions are the external vertices
     * from the boundary list.  Include them, and put encoded vertex
     * IDs into the map.
     */
	
    for (ptPtr = ptHash;  ptPtr < ptHash + nPoints; ptPtr++)
	ptPtr->ring = -1;

    for (b = boundary->list; b ; b = b->next)
    {
	ptPtr = ptHash + b->v0;
	ptPtr->hisIndex = -(b->v1 + 1);
	ptPtr->ring = 0;
    }

    vertKnt = 0;
    eltKnt = 0;

    for (ring = 0; ring < nRings; ring ++)
    {
	elt = elements;
	for (e = 0; e < nElts; e++, elt += vPerE)
	{
	    /*
	     * If this element is already included, skip it
	     */
	    if (eltHash[e] >= 0)
		continue;

	    /*
	     * Look to see if any vertices have been included
	     */
	    for (v = 0; v < vPerE; v++)
	    {
		ptPtr = ptHash + elt[v];
		if (ptPtr->ring >= 0 && ptPtr->ring <= ring)
		    break;
	    }
    
	    /*
	     * If not, skip it.
	     */
	    if (v == vPerE)
		continue;

	    /*
	     * If so... 
	     *	include its vertices.  
	     *	include the element.
	     */
	    eltHash[e] = eltKnt;

	    for (v = 0; v < vPerE; v++)
	    {
		/*
		 * If the vertex is already included, don't need to
		 * do it again.  
		 */
		ptPtr = ptHash + elt[v];
		if (ptPtr->ring < 0)
		{
		    ptPtr->hisIndex = vertKnt++;
		    ptPtr->ring     = ring+1;
		}
	    }

	    eltKnt ++;
	}
    }

    /*
     * Now we know which elements and positions lie in the overlap.
     * We need to create the overlap object and add the necessary data 
     * to it.
     *
     * How many components go?
     */
    DXChangedComponentValues(field, "positions");
    DXChangedComponentValues(field, "connections");

    nComponents = 0;
    for (i = 0; DXGetEnumeratedComponentValue(field, i, &name); i++)
    {
	if (!strcmp(name, "positions") 			||
	    !strcmp(name, "invalid positions") 		||
	    !strcmp(name, "connections")		||
	    !strcmp(name, "invalid connections"))
	{
	    nComponents++;
	    continue;
	}

	for (cmp = components; *cmp; cmp++)
	    if (! strcmp(*cmp, name))
	    {
		nComponents++;
		break;
	    }
		
    }

    overlap = (Overlap)DXAllocate(sizeof(struct overlap));
    if (! overlap)
	goto MakeOverlap_error;

    overlap->nComponents = nComponents;
    overlap->nPoints     = vertKnt;
    overlap->nElements   = eltKnt;

    overlap->names        = (char   **)DXAllocateZero(i * sizeof(char *));
    overlap->buffers      = (Pointer *)DXAllocateZero(i * sizeof(Pointer));
    overlap->dependencies = (byte    *)DXAllocateZero(i * sizeof(byte));
    overlap->counts       = (int     *)DXAllocateZero(i * sizeof(int));
    overlap->arrays       = (Array   *)DXAllocateZero(i * sizeof(Array));

    if (!overlap->buffers || !overlap->names
	|| !overlap->dependencies || !overlap->counts)
	goto MakeOverlap_error;

    i = 0; nComponents = 0;
    while (NULL != (array = GetEArray(field, i, &name)))
    {
	int *dst;

	overlap->names[nComponents]  = name;
	overlap->arrays[nComponents] = array;

	/*
	 * We handle components differently because they require renumbering
	 */
	if (!strcmp(name, "connections"))
	{
	    overlap->dependencies[nComponents] = DEP_ON_CONNECTIONS;
	    overlap->buffers[nComponents] = DXAllocate(eltKnt*vPerE*sizeof(int));
	    if (! overlap->buffers[nComponents])
		goto MakeOverlap_error;
	    
	    dst = (int *)overlap->buffers[nComponents];
	    for (j = 0; j < nElts; j++)
	    {
		if (eltHash[j] >= 0)
		{
		    elt = elements + (j * vPerE);
		    for (k = 0; k < vPerE; k++)
			*dst++ = ptHash[*elt++].hisIndex;
		}
	    }
	    overlap->counts[nComponents] = eltKnt;
	    nComponents++;
	}
	else
	{
	    /*
	     * We have done connections.  Now we do positions and any
	     * requested components
	     */
	    doit = 0;

	    if (! strcmp(name, "positions")         ||
		! strcmp(name, "invalid positions") ||
		! strcmp(name, "invalid connections"))
		doit = 1;

	    if (!doit && components)
		for (cmp = components; (*cmp != NULL && !doit); cmp++)
		    if (! strcmp(*cmp, name))
			doit = 1;

	    if (doit)
	    {
		int itemSize;
		Object attr;
		unsigned char *srcBuffer, *src;
		unsigned char *dstBuffer, *dst;

		if (NULL !=
		       (attr = DXGetComponentAttribute(field, name, "dep")))
		{
		    if (DXGetObjectClass(attr) != CLASS_STRING)
		    {
			DXSetError(ERROR_DATA_INVALID,
				    "component dependence attribute");
			goto MakeOverlap_error;
		    }

		    if (! strcmp(DXGetString((String)attr), "positions"))
		    {
			itemSize = DXGetItemSize(array);

			overlap->buffers[nComponents] =
					DXAllocate(vertKnt*itemSize);
			if (! overlap->buffers[nComponents])
			    goto MakeOverlap_error;

			overlap->dependencies[nComponents] = DEP_ON_POSITIONS;
			overlap->counts[nComponents] = vertKnt;
		    
			dstBuffer=(unsigned char *)overlap->buffers[nComponents];
			srcBuffer=(unsigned char *)DXGetArrayData(array);
			for (j = 0; j < nPoints; j++)
			{
			    if (ptHash[j].ring >= 0 && ptHash[j].hisIndex >= 0)
			    {
				src = srcBuffer + j*itemSize;
				dst = dstBuffer + ptHash[j].hisIndex*itemSize;

				memcpy(dst, src, itemSize);
			    }
			}
		    }
		    else if (! strcmp(DXGetString((String)attr), "connections"))
		    {
			itemSize = DXGetItemSize(array);

			overlap->buffers[nComponents] = DXAllocate(eltKnt*itemSize);
			if (! overlap->buffers[nComponents])
			    goto MakeOverlap_error;

			overlap->dependencies[nComponents] = DEP_ON_CONNECTIONS;
			overlap->counts[nComponents] = eltKnt;
			
			dstBuffer=(unsigned char *)overlap->buffers[nComponents];
			srcBuffer=(unsigned char *)DXGetArrayData(array);

			dst = dstBuffer;
			for (j = 0; j < nElts; j++)
			{
			    if (eltHash[j] >= 0)
			    {
				src = srcBuffer + (j * itemSize);
				memcpy(dst, src, itemSize);
				dst += itemSize;
			    }
			}
		    }
		}
		else if 
		   (NULL != (attr = DXGetComponentAttribute(field, name, "ref")))
		{
		    if (! strcmp(DXGetString((String)attr), "positions"))
		    {
			int i, knt, *dst;
			int nItems, nRefs = DXGetItemSize(array) / sizeof(int);
			int *s, *src = (int *)DXGetArrayData(array);

			if (nRefs != 1)
			{
			    DXSetError(ERROR_DATA_INVALID,
			"cannot handle partitioned multi-referential component");
			    goto MakeOverlap_error;
			}

			DXGetArrayInfo(array, &nItems, NULL, NULL, NULL, NULL);

			overlap->dependencies[nComponents] = REF_TO_POSITIONS;

			knt = 0;
			for (i = 0, s = src; i < nItems; i++)
			{
			    if (ptHash[*s++].ring >= 0)
				knt ++;
			}

			overlap->counts[nComponents]  = knt;

			if (knt > 0)
			{
			    overlap->buffers[nComponents] =
				DXAllocate(knt*DXGetItemSize(array));

			    if (! overlap->buffers[nComponents])
				goto MakeOverlap_error;

			    dst = (int *)overlap->buffers[nComponents];

			    for (i = 0, s = src; i < nItems; i++, s++)
			    {
				if (ptHash[*s].ring >= 0)
				    *dst++ = ptHash[*s].hisIndex;
			    }
			}
			else
			    overlap->buffers[nComponents] = NULL;
		    }
		    else if (! strcmp(DXGetString((String)attr), "connections"))
		    {
			int i, knt, *dst;
			int nItems, nRefs = DXGetItemSize(array) / sizeof(int);
			int *s, *src = (int *)DXGetArrayData(array);

			if (nRefs != 1)
			{
			    DXSetError(ERROR_DATA_INVALID,
			"cannot handle partitioned multi-referential component");
			    goto MakeOverlap_error;
			}
			DXGetArrayInfo(array, &nItems, NULL, NULL, NULL, NULL);

			overlap->dependencies[nComponents] = REF_TO_CONNECTIONS;

			knt = 0;
			for (i = 0, s = src; i < nItems; i++)
			{
			    if (eltHash[*s++] >= 0)
				knt ++;
			}

			overlap->counts[nComponents]  = knt;

			if (knt > 0)
			{
			    overlap->buffers[nComponents] =
				    DXAllocate(knt*DXGetItemSize(array));
			    if (! overlap->buffers[nComponents])
				goto MakeOverlap_error;

			    dst = (int *)overlap->buffers[nComponents];

			    for (i = 0, s = src; i < nItems; i++, s++)
			    {
				if (eltHash[*s] >= 0)
				    *dst++ = eltHash[*s];
			    }
			}
			else
			    overlap->buffers[nComponents] = NULL;
		    }
		}
		else
		{
		    DXSetError(ERROR_DATA_INVALID, 
			"no dep or ref attr on %s", name);
		    goto MakeOverlap_error;
		}

		nComponents++;
	    }
	}
	i++;
    }
    
    DXFree((Pointer)ptHash);
    if (eltHash)
	DXFree((Pointer)eltHash);

    return overlap;

MakeOverlap_error:

    FreeOverlap(overlap);
    DXFree((Pointer)ptHash);
    if (eltHash)
	DXFree((Pointer)eltHash);

    return NULL;
}

static Error
FreeOverlap(Overlap overlap)
{
    int i;

    for (i = 0; i < overlap->nComponents; i++)
	if (overlap->buffers[i])
	    DXFree(overlap->buffers[i]);

    DXFree((Pointer)(overlap->names));
    DXFree((Pointer)(overlap->buffers));
    DXFree((Pointer)(overlap->dependencies));
    DXFree((Pointer)(overlap->counts));
    DXFree((Pointer)(overlap->arrays));
    DXFree((Pointer)overlap);

    return OK;
}

/*
 * The mask arrays are used to determine whether a vertex is external.
 * The k-th entry in the array corresponds to the k-th vertex, and 
 * consists of a set of bits corresponding to the faces of the element
 * that are incident on that vertex.
 */
short TriMask[]   = {0x06, 0x05, 0x03};
short TetraMask[] = {0x0E, 0x0D, 0x0B, 0x07};
short CubeMask[]  = {0x15, 0x25, 0x19, 0x29, 0x16, 0x26, 0x1A, 0x2A};
short QuadMask[]  = {0x05, 0x09, 0x06, 0x0A};

/*
 * The following are the number of sides of each element type
 */
short TriSides   = 3;
short TetraSides = 4;
short CubeSides  = 6;
short QuadSides  = 4;

static Error
MakeExternals(Pointer ptr)
{
    Field		field;
    Array		array;
    Externals		externals, *exPtr;
    int			nElts;
    int			*nbrs;
    int			*elts;
    float		*verts;
    int			nDim;
    int			vPerE;
    float		*vertex;
    Object		attr;
    char		*str;
    int   		i, j;
    int   		flags;
    short		*mask;
    short		nSides;
    int			nPts;

    field = ((Task0Params)ptr)->field;
    exPtr = ((Task0Params)ptr)->externals;

    if (NULL == (array = (Array)DXGetComponentValue(field, "connections")))
    {
	DXSetError(ERROR_MISSING_DATA, "no connections component");
	return ERROR;
    }
    DXGetArrayInfo(array, &nElts, NULL, NULL, NULL, &vPerE);

    /*
     * Irregularize if necessary
     */
    if (DXQueryGridConnections(array, NULL, NULL))
    {
	Array irreg = DXNewArray(TYPE_INT, CATEGORY_REAL, 1, vPerE);
	if (! irreg)
	    return ERROR;
	
	if (! DXAddArrayData(irreg, 0, nElts, DXGetArrayData(array)))
	{
	    DXDelete((Object)irreg);
	    return ERROR;
	}
	
	if (! DXSetComponentValue(field, "connections", (Object)irreg))
	{
	    DXDelete((Object)irreg);
	    return ERROR;
	}
	
	array = irreg;
    }

    elts = (int *)DXGetArrayData(array);

    attr = DXGetComponentAttribute(field, "connections", "element type");
    if (! attr)
    {
	DXSetError(ERROR_MISSING_DATA, "no element type attribute");
	return ERROR;
    }
    str = DXGetString((String)attr);
    if (! str)
    {
	DXSetError(ERROR_MISSING_DATA, "bad element type attribute");
	return ERROR;
    }

    if (NULL == (array = (Array)DXGetComponentValue(field, "positions")))
    {
	DXSetError(ERROR_MISSING_DATA, "no positions component");
	return ERROR;
    }
    DXGetArrayInfo(array, &nPts, NULL, NULL, NULL, &nDim);
    verts = (float *)DXGetArrayData(array);

    if (! strcmp(str, "lines"))
    {
	byte *visited = (byte *)DXAllocateZero(nPts*sizeof(byte));
	if (! visited)
	    return ERROR;
	
	for (i = 0; i < 2*nElts; i++)
	    visited[*elts++]++;

	if (NULL == (externals = NewExternals()))
	{
	    DXFree((Pointer)visited);
	    return ERROR;
	}
	
	for (i = 0; i < nPts; i++)
	    if (visited[i] == 1)
	    {
		vertex = verts + (nDim * i);
		if (! AddExternalVertex(externals, vertex, i, nDim)) 
		{
		    DXFree((Pointer)visited);
		    goto MakeExternals_error;
		}
	    }
    }
    else
    {
	if (NULL == (array = DXNeighbors(field)))
	{
	    if (DXGetError() == ERROR_NONE)
		DXSetError(ERROR_MISSING_DATA, 
		    "unable to find or create neighbors component");
	    return ERROR;
	}
	nbrs = (int *)DXGetArrayData(array);

	if (! strcmp(str, "tetrahedra"))
	{
	    mask = TetraMask;
	    nSides = TetraSides;
	}
	else if (! strcmp(str, "triangles"))
	{
	    mask = TriMask;
	    nSides = TriSides;
	}
	else if (! strcmp(str, "cubes"))
	{
	    mask = CubeMask;
	    nSides = CubeSides;
	}
	else if (! strcmp(str, "quads"))
	{
	    mask = QuadMask;
	    nSides = QuadSides;
	}
	else
	{
	    DXSetError(ERROR_NOT_IMPLEMENTED,
		"irregular %s not supported", str);
	    return ERROR;
	}

	if (NULL == (externals = NewExternals()))
	    return ERROR;

	for (i = 0; i < nElts; i++, elts += vPerE)
	{
	    /*
	     * Set up a bit vector: a 1 in bit position k indicates
	     * that there is NO neighbor adjacent to face k and hence
	     * that face is an external face.
	     */
	    flags = 0; 
	    for (j = 0; j < nSides; j++, nbrs++)
		if (*nbrs == -1)
		    flags |= 1 << j;

	    /*
	     * For each vertex, see if it is external.  This is done by
	     * AND-ing the flags word with the mask corresponding to the 
	     * vertex.
	     */
	    for (j = 0; j < vPerE; j++)
	    {
		if (flags & mask[j])
		{
		    vertex = verts + (nDim * elts[j]);
		    if (! AddExternalVertex(externals, vertex, elts[j], nDim)) 
			goto MakeExternals_error;
		    
		}
	    }
	}
    }

    *exPtr = externals;
    return OK;

MakeExternals_error:

    if (externals)
        FreeExternals(externals);
    
    *exPtr = NULL;

    return ERROR;
}

static IJ
FindNeighborPartitions(Field *fields, int nParts)
{
    IJ array;
    Point *maxs, *mins;
    Point box[9], *p;
    Point min, max;
    Point *iMin, *iMax;
    Point *jMin, *jMax;
    int   i, j, k;

    array = NULL;
    mins  = NULL;
    maxs  = NULL;

    array = (IJ     )DXAllocate(nParts*nParts*sizeof(union ij));
    maxs  = (Point *)DXAllocate(nParts*sizeof(Point));
    mins  = (Point *)DXAllocate(nParts*sizeof(Point));
    if ((! array) || (! mins) || (! maxs))
	goto FindNeighborPartitions_error;
    
    for (i = 0; i < nParts; i++)
    {
	if (! DXBoundingBox((Object)fields[i], box))
	    goto FindNeighborPartitions_error;

	min.x = min.y = min.z =  DXD_MAX_FLOAT;
	max.x = max.y = max.z = -DXD_MAX_FLOAT;

	p = box;
	for (k = 0; k < 8; k++, p++)
	{
	    if (p->x > max.x) max.x = p->x;
	    if (p->x < min.x) min.x = p->x;
	    if (p->y > max.y) max.y = p->y;
	    if (p->y < min.y) min.y = p->y;
	    if (p->z > max.z) max.z = p->z;
	    if (p->z < min.z) min.z = p->z;
	}

	maxs[i].x = max.x; maxs[i].y = max.y; maxs[i].z = max.z;
	mins[i].x = min.x; mins[i].y = min.y; mins[i].z = min.z;
    }

    for (i = 0; i < nParts; i++)
    {
	iMax = maxs + i;
	iMin = mins + i;

	array[i*nParts + i].neighbor = 0;

	for (j = i+1; j < nParts; j++)
	{
	    jMax = maxs + j;
	    jMin = mins + j;

	    array[i*nParts + j].neighbor = 0;
	    array[j*nParts + i].neighbor = 0;
	
	    if (iMax->x < jMin->x) continue;
	    if (iMax->y < jMin->y) continue;
	    if (iMax->z < jMin->z) continue;
	    if (iMin->x > jMax->x) continue;
	    if (iMin->y > jMax->y) continue;
	    if (iMin->z > jMax->z) continue;

	    array[i*nParts + j].neighbor = 1;
	    array[j*nParts + i].neighbor = 1;
	}
    }

    DXFree((Pointer)mins);
    DXFree((Pointer)maxs);

    return array;

FindNeighborPartitions_error:

    DXFree((Pointer)array);
    DXFree((Pointer)mins);
    DXFree((Pointer)maxs);

    return NULL;
}

static Error
AddExternalVertex(Externals e, float *point, int vIndex, int nDim)
{
    struct exVert elt;

    elt.point[0] = point[0];
    elt.point[1] = (nDim > 1 ? point[1] : 0.0);
    elt.point[2] = (nDim > 2 ? point[2] : 0.0);
    elt.vIndex	 = vIndex;

    if (! DXInsertHashElement((HashTable)e, (Element)&elt))
	return ERROR;

    return OK;
}

static List
NewList()
{
    List l;

    l = (List)DXAllocate(sizeof(struct list));
    if (l)
    {
	l->list = NULL;
	l->freeList = NULL;
	l->listBlocks = NULL;
    }

    return l;
}

static Error
FreeList(List le)
{
    LinkBlock c, n;

    for (c = le->listBlocks; c; c = n)
    {
	n = c->nextBlock;
	DXFree((Pointer)c);
    }

    DXFree((Pointer)le);

    return OK;
}

static Link
GetFreeLink(List links)
{
    Link free;

    if (NULL != (free = links->freeList))
    {
	links->freeList = free->next;
	return free;
    }

    if ((links->listBlocks == NULL) || 
	(links->listBlocks->nextFreeElt == PLELTS_PER_BLOCK))
    {
	if (! AddLinkBlock(links))
	    return NULL;
    }

    return links->listBlocks->links + links->listBlocks->nextFreeElt++;
}

static Error
AddLinkBlock(List links)
{
    LinkBlock t;

    t = (LinkBlock)DXAllocate(sizeof(struct linkBlock));
    if (! t)
	return ERROR;
    
    t->nextFreeElt = 0;
    t->nextBlock = links->listBlocks;
    links->listBlocks = t;

    return OK;
}

static Error
InitGetNextExternalVertex(Externals e)
{
    if (e == NULL)
	return ERROR;
    
    return DXInitGetNextHashElement((HashTable)e);
}

static ExVert
GetNextExternalVertex(Externals e)
{
    return (ExVert)DXGetNextHashElement((HashTable)e);
}

static Error
MakeBoundaries(Pointer ptr)
{
    IJ        ij;
    Externals *externals;
    int       nParts, nRings;
    int       i, j;
    Field     field;
    Boundary  boundary;
    Array     compArray;
    char      **components = NULL;

    i           = ((Task1Params)ptr)->i;
    nParts	= ((Task1Params)ptr)->nParts;
    nRings	= ((Task1Params)ptr)->nRings;
    field	= ((Task1Params)ptr)->field;
    ij   	= ((Task1Params)ptr)->ij;
    externals   = ((Task1Params)ptr)->externals;
    compArray   = ((Task1Params)ptr)->components;

    components = GetComponentNames(compArray);
    if (! components)
	goto error;

    for (j = 0; j < nParts; j++)
    {
	if (i == j || ij[j].neighbor == 0)
	    continue;
	
	boundary = MakeBoundarySet(externals[i], externals[j]);
	if (! boundary)
	    goto error;

	ij[j].overlap = MakeOverlap(field, boundary, nRings, components);

	FreeBoundary(boundary);

	if (! ij[j].overlap)
	    goto error;
    }

    FreeComponentNames(components);
    
    return OK;

error:
    FreeComponentNames(components);
    return ERROR;
}

static Error
InvalidateBoundaryDuplicates(Pointer ptr)
{
    IJ        ij;
    Externals *externals, ei, ej;
    int       i, j, k, n;
    Field     field;
    byte      *flags = NULL;
    ExVert    xv;
    InvalidComponentHandle ich = NULL;

    /*
     * If this is the 0-th partition, then it "owns" all its external
     * vertices
     */
    if ((i = ((Task1Params)ptr)->i) == 0)
	return OK;

    field	= ((Task1Params)ptr)->field;
    ij   	= ((Task1Params)ptr)->ij;
    externals   = ((Task1Params)ptr)->externals;

    ei = externals[i];

    /*
     * If it has no external vertices (can this happen?) then
     * OK
     */
    InitGetNextExternalVertex(ei);
    for (n = 0; NULL != GetNextExternalVertex(ei); n++);
    if (n == 0)
	 return OK;

    flags = (byte *)DXAllocateZero(n*sizeof(byte));
    if (! flags)
	 goto error;

    for (j = 0; j < i; j++)
    {
	if (ij[j].neighbor == 0)
	    continue;
	
	ej = externals[j];
	
	InitGetNextExternalVertex(ei);
	for (k = 0; NULL != (xv = GetNextExternalVertex(ei)); k++)
	    if (! flags[k] && QueryExternals(ej, xv->point))
		flags[k] = 1;
    }

    ich = DXCreateInvalidComponentHandle((Object)field, NULL, "positions");
    if (! ich)
	goto error;
    
    InitGetNextExternalVertex(ei);
    for (k = 0; NULL != (xv = GetNextExternalVertex(ei)); k++)
	if (flags[k])
	    DXSetElementInvalid(ich, xv->vIndex);
    
    if (! DXSaveInvalidComponent(field, ich))
	goto error;

    DXFree((Pointer)flags);
    DXFreeInvalidComponentHandle(ich);
    
    return OK;

error:
    DXFree((Pointer)flags);
    return ERROR;
}

static Error
FreeExternalsList(Externals *externals, int n)
{
    int i;

    for (i = 0; i < n; i++)
	if (externals[i])
	    FreeExternals(externals[i]);
    
    DXFree((Pointer)externals);

    return OK;
}

static Error
FreeIJ(IJ ij, int n)
{
    int i;

    for (i = 0; i < n*n; i++)
	if (ij[i].neighbor > 1)
	    FreeOverlap(ij[i].overlap);
    
    DXFree((Pointer)ij);

    return OK;
}

static Boundary
MakeBoundarySet(Externals e0, Externals e1)
{
    Boundary b;
    ExVert   xv0, xv1;

    if (NULL == (b = NewBoundary()))
	return NULL;
    
    InitGetNextExternalVertex(e0);
    while (NULL != (xv0 = GetNextExternalVertex(e0)))
	if (NULL != (xv1 = QueryExternals(e1, xv0->point)))
	    if (! AddBoundary(b, xv0->vIndex, xv1->vIndex))
	    {
		FreeBoundary(b);
		return NULL;
	    }
    
    return b;
}

static Externals
NewExternals()
{
    return (Externals)DXCreateHash(sizeof(struct exVert), VertexHash, VertexCmp);
}

static Error
FreeExternals(Externals e)
{
    DXDestroyHash((HashTable)e);

    return OK;
}

static ExVert
QueryExternals(Externals e, float *point)
{
    return (ExVert)DXQueryHashElement((HashTable)e, (Key)point);
}

static Error
AddBoundary(Boundary b, int v0, int v1)
{
    Link l;

    l = GetFreeLink((List)b);
    if (! l)
	return ERROR;
    
    l->v0 = v0;
    l->v1 = v1;

    l->next = b->list;
    b->list = l;

    return OK;
}

static float primes[] =
    { 143669821.0, 45497241659.0, 1455799321.0};

static PseudoKey
VertexHash(Key key)
{
    register int i;
    register long l;
             float j;
    register long k;
    register float *keyPtr;
    register float *primesPtr;

    l = 0;
    keyPtr = (float *)key;
    primesPtr = primes;
    for (i = 0; i < 3; i++)
    {
	j = *primesPtr++ * *keyPtr++;
	k = (*(int *)&j);
	k = (((k ^ (k >> 8))) ^ (k >> 16)) ^ (k >> 24);
	l = l + k;
    }

    return (PseudoKey)l;
}

static int
VertexCmp(Key k0, Key k1)
{
    register int   i;
    register float *p0, *p1;

    p0 = (float *)k0;
    p1 = (float *)k1;

    for (i = 0; i < 3; i++)
	if (*p0++ != *p1++) return 1;
    
    return 0;
}

static char **
GetComponentNames(Array compArray)
{
    char **list = NULL;

    if (! compArray)
    {
	list = (char **)DXAllocate(sizeof(char *));
	if (! list)
	    return ERROR;

	list[0] = NULL;
    }
    else
    {
	char *s;
	int i, nComponents, len;

	DXGetArrayInfo(compArray, &nComponents, NULL, NULL, NULL, &len);

	list = DXAllocate((nComponents+1) * sizeof(char *));
	if (! list)
	    return ERROR;

	s = (char *)DXGetArrayData(compArray);
	for (i = 0; i < nComponents; i++)
	{
	    list[i] = s;
	    s += len;
	}
	list[i] = NULL;
    }

    return list;
}

static void
FreeComponentNames(char **list)
{
    DXFree((Pointer)list);
}


