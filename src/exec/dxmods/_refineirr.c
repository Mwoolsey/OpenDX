/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


/*
 * $Header: /src/master/dx/src/exec/dxmods/_refineirr.c,v 1.6 2002/03/21 02:57:30 rhh Exp $:
 */

#include <string.h>
#include <stdio.h>
#include "math.h"
#include <dx/dx.h>
#include "_refine.h"

/***************************
 * cubes tables
 ***************************/

static int cube_edgeTable[] = 
{
    0, 1, 1, 5, 5, 4, 4, 0,
    2, 3, 3, 7, 7, 6, 6, 2,
    0, 2, 1, 3, 5, 7, 4, 6
};

static int cube_faceTable[] =
{
    0, 1, 2, 3,
    1, 5, 7, 3, 
    5, 4, 6, 7, 
    0, 4, 2, 6,
    1, 5, 4, 0, 
    2, 6, 7, 3
};

static int subcubes[] = 
{
     0,  8, 16, 20, 11, 24, 23, 26,
     8,  1, 20, 17, 24,  9, 26, 21,
    16, 20,  2, 12, 23, 26, 15, 25,
    20, 17, 12,  3, 26, 21, 25, 13,
    11, 24, 23, 26,  4, 10, 19, 22,
    24,  9, 26, 21, 10,  5, 22, 18,
    23, 26, 15, 25, 19, 22,  6, 14,
    26, 21, 25, 13, 22, 18, 14,  7

};

/***************************
 * quads tables
 ***************************/

static int quad_edgeTable[] = 
{
    0, 1, 1, 3, 3, 2, 2, 0
};

static int subquads[] =
{
     0, 4, 7, 8,
     4, 1, 8, 5, 
     7, 8, 2, 6,
     8, 5, 6, 3
};

/***************************
 * lines tables
 ***************************/

static int sublines[] =
{
  0, 2,
  2, 1
};

/***************************
 * tetras tables
 ***************************/

static int tetra_edgeTable[] = 
{
    0, 1, 1, 3, 1, 2,
    2, 3, 0, 2, 0, 3
};

static int subtetras[] =
{
    0, 4, 8, 9,
    4, 1, 6, 5, 
    7, 6, 8, 2,
    9, 5, 7, 3,
    5, 4, 7, 6,
    4, 8, 7, 6,
    4, 7, 8, 9,
    4, 5, 7, 9
};

/***************************
 * triangles tables
 ***************************/

static int tri_edgeTable[] = 
{
    0, 1, 1, 2, 2, 0
};

static int subtriangles[] =
{
    0, 3, 5,
    3, 1, 4,
    3, 4, 5,
    5, 4, 2
};

typedef struct hash
{
    int index;
    int v[4];
} *Hash;

/*
 * One element structure for all hash elements, but
 * each will be sized appropriately.
 */
#define EDGESIZE	3*sizeof(int)
#define QUADSIZE	5*sizeof(int)

/*
 * Tables containing the following information are used
 * to identify the interpolations necessary to create the
 * additional positional information.  The table is a list
 * of nV * knt vertex indices.  For each of knt results,
 * the data associated with nV vertices are averaged.
 */
typedef struct
{
    int *table;	/* list of vertex indices to be interpolated 		*/
    int nV;	/* number of vertices to interpolate for each output	*/
    int knt;	/* number of interpolations to perform			*/
} InterpTable;

static Field RefineICubes(Field, int);
static Field RefineIQuads(Field, int);
static Field RefineILines(Field, int);
static Field RefineITetras(Field, int);
static Field RefineITriangles(Field, int);

static Array RefineCArray(Array, int);
static Array RefineCRefArray(Array, int);
static Array RefinePArray(Array, InterpTable *, int);
static Array CopyArray(Array);
static Error FieldSetup(Field);

static int   *MakeTable(HashTable, int, int);

/*
 * The following is for keeping track of position derivations so
 * that invalid positions references are propagated.
 */
typedef struct listElement
{
    struct listElement *next;
    int			index;
} *ListElement;

typedef struct {
    int                 invalid;     /*  Is this position invalid?  */
    ListElement         head;        /*  List of invalid elements   */
} ListHead;

static Error  AddHashReferences(HashTable, ListHead *, SegList *, int, int);
static Error  AddReferences(int *, int, ListHead *, SegList *, int, int);
static Error  SetupIPTables(Field, int, ListHead **, SegList **);
static Error  DumpIPTables(Field, int, ListHead **, SegList **);

static Error  AddQuadToHash(HashTable, int, int, int, int, int *);
static int    QueryQuad(HashTable, int, int, int, int);
static void   SetupQuad(int, int, int, int, Hash);
static Error  AddEdgeToHash(HashTable, int, int, int *);
static int    QueryEdge(HashTable, int, int);
static void   SetupEdge(int, int, Hash);
static Hash   _QueryHash(HashTable, Hash);

static PseudoKey Hash4(Key);
static int       Cmp2(Key, Key);
static PseudoKey Hash2(Key);
#if 0
static int       Cmp4(Key, Key);
#endif

Field
_dxfRefineIrreg(Field f, int levels)
{
    Object	attr;
    char	*str;

    if (! DXGetComponentValue(f, "connections"))
    {
	DXSetError(ERROR_MISSING_DATA, "#10250", "input", "connections");
	return NULL;
    }

    attr = DXGetComponentAttribute(f, "connections", "element type");
    if (! attr)
    {
	DXSetError(ERROR_MISSING_DATA, "#10255",
				"connections", "element type");
	return NULL;
    }

    if (DXGetObjectClass(attr) != CLASS_STRING)
    {
	DXSetError(ERROR_BAD_CLASS, "#10051", "element type attribute");
	return NULL;
    }

    str = DXGetString((String)attr);

    if (! strcmp(str, "cubes"))
        return RefineICubes(f, levels);
    else if (! strcmp(str, "quads"))
        return RefineIQuads(f, levels);
    else if (! strcmp(str, "lines"))
        return RefineILines(f, levels);
    else if (! strcmp(str, "tetrahedra"))
        return RefineITetras(f, levels);
    else if (! strcmp(str, "triangles"))
        return RefineITriangles(f, levels);
    else
    { 
	DXSetError(ERROR_DATA_INVALID, "#11380", str);
	return NULL;
    }
}
    
static Field
RefineICubes(Field f, int levels)
{
    int         i, j, k, l;
    HashTable   fHash = NULL;
    HashTable   eHash = NULL;
    InterpTable iTable[4];
    Array       inP, inC, outC = NULL;
    int         *c, *inElts, *outElts, nElts, nPoints;
    int  	eOffset, fOffset, cOffset;
    int		nEdges, nFaces;
    Object	old;
    char	*name;
    ListHead    *invTable = NULL;
    SegList     *listElements = NULL;

    iTable[0].table = iTable[1].table = NULL;

    if (! FieldSetup(f))
	goto error;
    
    for (l = 0; l < levels; l++)
    {
	inP = (Array)DXGetComponentValue(f, "positions");
	DXGetArrayInfo(inP, &nPoints, NULL, NULL, NULL, NULL);

	inC = (Array)DXGetComponentValue(f, "connections");
	if (! inC)
	{
	    DXSetError(ERROR_MISSING_DATA, "10250", "input", "connections");
	    goto error;
	}

	inElts = (int *)DXGetArrayData(inC);
	if (! inElts)
	    goto error;
	
	DXGetArrayInfo(inC, &nElts, NULL, NULL, NULL, NULL);

	outC = (Array)DXNewArray(TYPE_INT, CATEGORY_REAL, 1, 8);
	if (! outC)
	    goto error;

	if (! DXAddArrayData(outC, 0, 8*nElts, NULL))
	    goto error;
	
	outElts = (int *)DXGetArrayData(outC);
	if (! outElts)
	    goto error;

	/*
	 * Create hash tables
	 */
	fHash = DXCreateHash(QUADSIZE, Hash4, Cmp2);
	if (! fHash)
	    goto error;

	eHash = DXCreateHash(EDGESIZE, Hash2, Cmp2);
	if (! eHash)
	    goto error;

	/*
	 * Put edges and faces into to the hash tables
	 */
	c = inElts; nEdges = 0; nFaces = 0;
	for (i = 0; i < nElts; i++, c += 8)
	{
	    int *t;

	    t = cube_edgeTable; 
	    for (j = 0; j < 12; j++, t += 2)
		if (! AddEdgeToHash(eHash, c[t[0]], c[t[1]], &nEdges))
		    goto error;

	    t = cube_faceTable;
	    for (j = 0; j < 6; j++, t += 4)
		if (! AddQuadToHash(fHash, c[t[0]], c[t[1]],
						c[t[2]], c[t[3]], &nFaces))
		    goto error;
	}

	/*
	 * for each input cube, create grid containing indices in output
	 * positions component and resulting output cubes.  We will be
	 * producing the input vertices first, the edge vertices next, the 
	 * face vertices next and the center vertices last.
	 */

	eOffset = nPoints;
	fOffset = eOffset + nEdges;
	cOffset = fOffset + nFaces;
	
	c = inElts;
	for (i = 0; i < nElts; i++, c += 8)
	{
	    int grid[27];
	    int v, *t;
	    int *b = subcubes;

	    v = 0;

	    for (j = 0; j < 8; j++)
		grid[v++] = c[j];

	    t = cube_edgeTable; 
	    for (j = 0; j < 12; j++, t += 2)
	    {
		int indx = QueryEdge(eHash, c[t[0]], c[t[1]]);
		
		grid[v++] = indx + eOffset;
	    }

	    t = cube_faceTable; 
	    for (j = 0; j < 6; j++, t += 4)
	    {
		int indx = QueryQuad(fHash, c[t[0]], c[t[1]], c[t[2]], c[t[3]]);
		grid[v++] = indx + fOffset;
	    }

	    grid[v] = cOffset + i;

	    /*
	     * Now the grid is stuffed with the output vertex IDs.  
	     * Generate the output cubes.
	     */
	    for (j = 0; j < 8; j++)
		for (k = 0; k < 8; k++)
		    *outElts++ = grid[*b++];
	}

	if (! SetupIPTables(f, nPoints, &invTable, &listElements))
	    goto error;

	iTable[0].nV    = 2;
	iTable[0].knt   = nEdges;
	iTable[0].table = MakeTable(eHash, nEdges, 2);
	if (! iTable[0].table)
	    goto error;

	if (invTable)
	    if (! AddHashReferences(eHash,
				invTable, listElements, nPoints, 2))
		goto error;


	iTable[1].nV    = 4;
	iTable[1].knt   = nFaces;
	iTable[1].table = MakeTable(fHash, nFaces, 4);
	if (! iTable[1].table)
	    goto error;

	if (invTable)
	    if (! AddHashReferences(fHash, invTable,
					listElements, nPoints+nEdges, 4))
		goto error;

	
	iTable[2].nV    = 8;
	iTable[2].knt   = nElts;
	iTable[2].table = inElts;
	if (! iTable[2].table)
	    goto error;

	if (invTable)
	    if (! AddReferences(inElts, nElts, invTable,
				    listElements, nPoints+nEdges+nFaces, 8))
		goto error;

	iTable[3].table = NULL;

	DXDestroyHash(eHash);
	DXDestroyHash(fHash);
	eHash = fHash = NULL;

	for (i=0; NULL != (old = DXGetEnumeratedComponentValue(f, i, &name)); i++)
	{
	    Object attr;
	    Array new = (Array)old;
	    char *str;

	    if (! strcmp(name, "connections"))
		continue;
	    
	    attr = DXGetComponentAttribute(f, name, "dep");
	    if (attr)
	    {
		int boolean;

		boolean = ! strncmp(name, "invalid", 7);

		if (DXGetObjectClass(attr) != CLASS_STRING)
		{
		    DXSetError(ERROR_BAD_CLASS, "#10051",
					    "dependency attribute");
		    goto error;
		}

		str = DXGetString((String)attr);

		if (! strcmp(str, "positions"))
		{
		    new = RefinePArray((Array)old, iTable, boolean);
		    if (! new)
			goto error;
		}
		else if (! strcmp(str, "connections"))
		{
		    new = RefineCArray((Array)old, 8);
		    if (! new)
			goto error;
		}
	    }
	    else
	    {
		if (! strcmp(name, "invalid connections"))
		{
		    /*
		     * Must be referential
		     */
		    new = RefineCRefArray((Array)old, 8);
		    if (! new)
			goto error;
		}
	    }

	    /*
	     * It is possible that the refinement process created new
	     * arrays.  This is because you cannot extend regular arrays
	     * and they are used for constants.
	     */
	    if ((Object)old != (Object)new)
		if (! DXSetComponentValue(f, name, (Object)new))
		    goto error;
	}


	/*
	 * We wait until now to reset the connections component since it
	 * causes deletion of the old one, whose data is used in iTable[2]
	 * to designate the vertices to be interpolated for each cube-center
	 * point.
	 */
	if (! DXSetComponentValue(f, "connections", (Object)outC))
	    goto error;
	
	outC = NULL;
     
	if (! DumpIPTables(f, nPoints, &invTable, &listElements))
	    goto error;
	
	DXFree((Pointer)iTable[0].table);
	DXFree((Pointer)iTable[1].table);
	iTable[0].table = iTable[1].table = NULL;
    }


    return DXEndField(f);

error:
    if (listElements)
	DXDeleteSegList(listElements);
    if (invTable)
	DXFree((Pointer)invTable);
    if (eHash)
	DXDestroyHash(eHash);
    if (fHash)
	DXDestroyHash(fHash);
    DXFree((Pointer)iTable[0].table);
    DXFree((Pointer)iTable[1].table);
    return NULL;
}

static Field
RefineIQuads(Field f, int levels)
{
    int         i, j, k, l;
    HashTable   eHash = NULL;
    InterpTable iTable[3];
    Array       inP, inC, outC = NULL;
    int         *c, *inElts, *outElts, nElts, nPoints;
    int  	eOffset, cOffset;
    int		nEdges;
    Object	old;
    char	*name;
    ListHead    *invTable = NULL;
    SegList     *listElements = NULL;
    InvalidComponentHandle ich = NULL;

    iTable[0].table = iTable[1].table = NULL;
    
    if (! FieldSetup(f))
	goto error;
    
    for (l = 0; l < levels; l++)
    {
	inP = (Array)DXGetComponentValue(f, "positions");
	DXGetArrayInfo(inP, &nPoints, NULL, NULL, NULL, NULL);

	inC = (Array)DXGetComponentValue(f, "connections");
	if (! inC)
	{
	    DXSetError(ERROR_MISSING_DATA, "10250", "input", "connections");
	    goto error;
	}

	if (DXGetComponentValue(f, "invalid connections"))
	{
	    ich = DXCreateInvalidComponentHandle((Object)f,
							NULL, "connections");
	    if (! ich)
		goto error;
	}

	inElts = (int *)DXGetArrayData(inC);
	if (! inElts)
	    goto error;
	
	DXGetArrayInfo(inC, &nElts, NULL, NULL, NULL, NULL);

	outC = (Array)DXNewArray(TYPE_INT, CATEGORY_REAL, 1, 4);
	if (! outC)
	    goto error;

	if (! DXAddArrayData(outC, 0, 4*nElts, NULL))
	    goto error;
	
	outElts = (int *)DXGetArrayData(outC);
	if (! outElts)
	    goto error;

	eHash = DXCreateHash(EDGESIZE, Hash2, Cmp2);
	if (! eHash)
	    goto error;

	/*
	 * Put edges into to the hash tables
	 */
	c = inElts; nEdges = 0;
	for (i = 0; i < nElts; i++, c += 4)
	{
	    int *t;

	    t = quad_edgeTable; 
	    for (j = 0; j < 4; j++, t += 2)
		if (! AddEdgeToHash(eHash, c[t[0]], c[t[1]], &nEdges))
		    goto error;
	}

	/*
	 * for each input quad, create grid containing indices in output
	 * positions component and resulting output quad.  We will be
	 * producing the input vertices first, the edge vertices next, the 
	 * center vertices last.
	 */

	eOffset = nPoints;
	cOffset = eOffset + nEdges;
	
	c = inElts;
	for (i = 0; i < nElts; i++, c += 4)
	{
	    int grid[9];
	    int *t, v;
	    int *b = subquads;

	    v = 0;

	    for (j = 0; j < 4; j++)
		grid[v++] = c[j];

	    t = quad_edgeTable; 
	    for (j = 0; j < 4; j++, t += 2)
	    {
		int indx = QueryEdge(eHash, c[t[0]], c[t[1]]);
		
		grid[v++] = indx + eOffset;
	    }

	    grid[v] = cOffset + i;

	    /*
	     * Now the grid is stuffed with the output vertex IDs.  
	     * Generate the output quads.
	     */
	    for (j = 0; j < 4; j++)
		for (k = 0; k < 4; k++)
		    *outElts++ = grid[*b++];
	}

	if (! SetupIPTables(f, nPoints, &invTable, &listElements))
	    goto error;

	iTable[0].nV    = 2;
	iTable[0].knt   = nEdges;
	iTable[0].table = MakeTable(eHash, nEdges, 2);
	if (! iTable[0].table)
	    goto error;

	if (invTable)
	    if (! AddHashReferences(eHash,
				invTable, listElements, nPoints, 2))
		goto error;

	iTable[1].nV    = 4;
	iTable[1].knt   = nElts;
	iTable[1].table = inElts;
	if (! iTable[1].table)
	    goto error;
	
	if (invTable)
	    if (! AddReferences(inElts, nElts, invTable,
				    listElements, nPoints+nEdges, 4))
		goto error;

	iTable[2].table = NULL;

	DXDestroyHash(eHash);
	eHash = NULL;

	for (i=0; NULL != (old = DXGetEnumeratedComponentValue(f, i, &name)); i++)
	{
	    Object attr;
	    Array new = (Array)old;
	    char *str;

	    if (! strcmp(name, "connections"))
		continue;
	    
	    attr = DXGetComponentAttribute(f, name, "dep");
	    if (attr)
	    {
		int boolean;

		boolean = ! strncmp(name, "invalid", 7);

		if (DXGetObjectClass(attr) != CLASS_STRING)
		{
		    DXSetError(ERROR_DATA_INVALID,
				"#10051", "dependency attribute");
		    goto error;
		}

		str = DXGetString((String)attr);

		if (! strcmp(str, "positions"))
		{
		    new = RefinePArray((Array)old, iTable, boolean);
		    if (! new)
			goto error;
		}
		else if (! strcmp(str, "connections"))
		{
		    new = RefineCArray((Array)old, 4);
		    if (! new)
			goto error;
		}
	    }
	    else
	    {
		if (! strcmp(name, "invalid connections"))
		{
		    /*
		     * Must be referential
		     */
		    new = RefineCRefArray((Array)old, 4);
		    if (! new)
			goto error;
		}
	    }

	    /*
	     * It is possible that the refinement process created new
	     * arrays.  This is because you cannot extend regular arrays
	     * and they are used for constants.
	     */
	    if ((Object)old != (Object)new)
		if (! DXSetComponentValue(f, name, (Object)new))
		    goto error;
	}

	/*
	 * We wait until now to reset the connections component since it
	 * causes deletion of the old one, whose data is used in iTable[2]
	 * to designate the vertices to be interpolated for each cube-center
	 * point.
	 */
	if (! DXSetComponentValue(f, "connections", (Object)outC))
	    goto error;
	
	outC = NULL;

	if (! DumpIPTables(f, nPoints, &invTable, &listElements))
	    goto error;
	
	DXFree((Pointer)iTable[0].table);
	iTable[0].table = NULL;
    }
     

    return DXEndField(f);

error:
    if (listElements)
	DXDeleteSegList(listElements);
    if (invTable)
	DXFree((Pointer)invTable);
    if (eHash)
	DXDestroyHash(eHash);
    DXFree((Pointer)iTable[0].table);
    return NULL;
}

static Field
RefineILines(Field f, int levels)
{
    int         i, j, k, l;
    InterpTable iTable[2];
    Array       inP, inC, outC = NULL;
    int         *c, *inElts, *outElts, nElts, nPoints;
    int  	cOffset;
    Object	old;
    char	*name;
    ListHead    *invTable = NULL;
    SegList     *listElements = NULL;

    iTable[0].table = iTable[1].table = NULL;

    if (DXEmptyField(f))
	return DXEndField(DXNewField());
    
    if (! FieldSetup(f))
	goto error;
    
    for (l = 0; l < levels; l++)
    {
	inP = (Array)DXGetComponentValue(f, "positions");
	DXGetArrayInfo(inP, &nPoints, NULL, NULL, NULL, NULL);

	inC = (Array)DXGetComponentValue(f, "connections");
	if (! inC)
	{
	    DXSetError(ERROR_MISSING_DATA, "10250", "input", "connections");
	    goto error;
	}

	inElts = (int *)DXGetArrayData(inC);
	if (! inElts)
	    goto error;
	
	DXGetArrayInfo(inC, &nElts, NULL, NULL, NULL, NULL);

	outC = (Array)DXNewArray(TYPE_INT, CATEGORY_REAL, 1, 2);
	if (! outC)
	    goto error;

	if (! DXAddArrayData(outC, 0, 2*nElts, NULL))
	    goto error;
	
	outElts = (int *)DXGetArrayData(outC);
	if (! outElts)
	    goto error;

	/*
	 * for each input line, create grid containing indices in output
	 * positions component and resulting output line.  We will be
	 * producing the input vertices first and the center vertices last.
	 */

	cOffset = nPoints;
	
	c = inElts;
	for (i = 0; i < nElts; i++, c += 2)
	{
	    int grid[3];
	    int v;
	    int *b = sublines;

	    v = 0;

	    for (j = 0; j < 2; j++)
		grid[v++] = c[j];

	    grid[v] = cOffset + i;

	    /*
	     * Now the grid is stuffed with the output vertex IDs.  
	     * Generate the output cubes.
	     */
	    for (j = 0; j < 2; j++)
		for (k = 0; k < 2; k++)
		    *outElts++ = grid[*b++];
	}

	if (! SetupIPTables(f, nPoints, &invTable, &listElements))
	    goto error;

	iTable[0].nV    = 2;
	iTable[0].knt   = nElts;
	iTable[0].table = inElts;
	if (! iTable[0].table)
	    goto error;
	
	iTable[1].table = NULL;

	for (i=0; NULL != (old = DXGetEnumeratedComponentValue(f, i, &name)); i++)
	{
	    Object attr;
	    Array new = (Array)old;
	    char *str;

	    if (! strcmp(name, "connections"))
		continue;
	    
	    attr = DXGetComponentAttribute(f, name, "dep");
	    if (attr)
	    {
		int boolean;

		boolean = ! strncmp(name, "invalid", 7);

		if (DXGetObjectClass(attr) != CLASS_STRING)
		{
		    DXSetError(ERROR_DATA_INVALID, "#10051",
					    "dependency attribute");
		    goto error;
		}

		str = DXGetString((String)attr);

		if (! strcmp(str, "positions"))
		{
		    new = RefinePArray((Array)old, iTable, boolean);
		    if (! new)
			goto error;
		}
		else if (! strcmp(str, "connections"))
		{
		    new = RefineCArray((Array)old, 2);
		    if (! new)
			goto error;
		}
	    }
	    else
	    {
		if (! strcmp(name, "invalid connections"))
		{
		    /*
		     * Must be referential
		     */
		    new = RefineCRefArray((Array)old, 2);
		    if (! new)
			goto error;
		}
	    }

	    /*
	     * It is possible that the refinement process created new
	     * arrays.  This is because you cannot extend regular arrays
	     * and they are used for constants.
	     */
	    if ((Object)old != (Object)new)
		if (! DXSetComponentValue(f, name, (Object)new))
		    goto error;
	}

	/*
	 * We wait until now to reset the connections component since it
	 * causes deletion of the old one, whose data is used in iTable[2]
	 * to designate the vertices to be interpolated for each cube-center
	 * point.
	 */
	if (! DXSetComponentValue(f, "connections", (Object)outC))
	    goto error;
	
	outC = NULL;
    }
     
    return DXEndField(f);

error:
    if (listElements)
    if (listElements)
	DXDeleteSegList(listElements);
    if (invTable)
	DXFree((Pointer)invTable);
	DXDeleteSegList(listElements);
    if (invTable)
	DXFree((Pointer)invTable);
    return NULL;
}

static Field
RefineITetras(Field f, int levels)
{
    int         i, j, k, l;
    HashTable   eHash = NULL;
    InterpTable iTable[3];
    Array       inP, inC, outC = NULL;
    int         *c, *inElts, *outElts, nElts, nPoints;
    int  	eOffset;
    int		nEdges;
    Object	old;
    char	*name;
    ListHead    *invTable = NULL;
    SegList     *listElements = NULL;

    iTable[0].table = iTable[1].table = NULL;

    if (DXEmptyField(f))
	return DXEndField(DXNewField());
    
    if (! FieldSetup(f))
	goto error;
    
    for (l = 0; l < levels; l++)
    {
	inP = (Array)DXGetComponentValue(f, "positions");
	DXGetArrayInfo(inP, &nPoints, NULL, NULL, NULL, NULL);

	inC = (Array)DXGetComponentValue(f, "connections");
	if (! inC)
	{
	    DXSetError(ERROR_MISSING_DATA, "10250", "input", "connections");
	    goto error;
	}

	inElts = (int *)DXGetArrayData(inC);
	if (! inElts)
	    goto error;
	
	DXGetArrayInfo(inC, &nElts, NULL, NULL, NULL, NULL);

	outC = (Array)DXNewArray(TYPE_INT, CATEGORY_REAL, 1, 4);
	if (! outC)
	    goto error;

	if (! DXAddArrayData(outC, 0, 8*nElts, NULL))
	    goto error;
	
	outElts = (int *)DXGetArrayData(outC);
	if (! outElts)
	    goto error;

	eHash = DXCreateHash(EDGESIZE, Hash2, Cmp2);
	if (! eHash)
	    goto error;

	/*
	 * Put edges into to the hash tables
	 */
	c = inElts; nEdges = 0;
	for (i = 0; i < nElts; i++, c += 4)
	{
	    int *t;

	    t = tetra_edgeTable; 
	    for (j = 0; j < 6; j++, t += 2)
		if (! AddEdgeToHash(eHash, c[t[0]], c[t[1]], &nEdges))
		    goto error;
	}

	/*
	 * for each input element, create grid containing indices in output
	 * positions component and resulting output elements.  We will be
	 * producing the input vertices first, the edge vertices next.
	 */

	eOffset = nPoints;
	/*cOffset = eOffset + nEdges;*/
	
	c = inElts;
	for (i = 0; i < nElts; i++, c += 4)
	{
	    int grid[10];
	    int *t, v;
	    int *b = subtetras;

	    v = 0;

	    for (j = 0; j < 4; j++)
		grid[v++] = c[j];

	    t = tetra_edgeTable; 
	    for (j = 0; j < 6; j++, t += 2)
	    {
		int indx = QueryEdge(eHash, c[t[0]], c[t[1]]);

		grid[v++] = indx + eOffset;
	    }

	    /*
	     * Now the grid is stuffed with the output vertex IDs.  
	     * Generate the output tetras.
	     */
	    for (j = 0; j < 8; j++)
		for (k = 0; k < 4; k++)
		    *outElts++ = grid[*b++];
	}

	if (! SetupIPTables(f, nPoints, &invTable, &listElements))
	    goto error;

	iTable[0].nV    = 2;
	iTable[0].knt   = nEdges;
	iTable[0].table = MakeTable(eHash, nEdges, 2);
	if (! iTable[0].table)
	    goto error;
	
	if (invTable)
	    if (! AddHashReferences(eHash,
				invTable, listElements, nPoints, 2))
		goto error;

	iTable[1].table = NULL;

	DXDestroyHash(eHash);
	eHash = NULL;

	for (i=0; NULL != (old = DXGetEnumeratedComponentValue(f, i, &name)); i++)
	{
	    Object attr;
	    Array new = (Array)old;
	    char *str;

	    if (! strcmp(name, "connections"))
		continue;
	    
	    attr = DXGetComponentAttribute(f, name, "dep");
	    if (attr)
	    {
		int boolean;

		boolean = ! strncmp(name, "invalid", 7);

		if (DXGetObjectClass(attr) != CLASS_STRING)
		{
		    DXSetError(ERROR_DATA_INVALID, "#10051", 
					"dependency attribute");
		    goto error;
		}

		str = DXGetString((String)attr);

		if (! strcmp(str, "positions"))
		{
		    new = RefinePArray((Array)old, iTable, boolean);
		    if (! new)
			goto error;
		}
		else if (! strcmp(str, "connections"))
		{
		    new = RefineCArray((Array)old, 8);
		    if (! new)
			goto error;
		}
	    }
	    else
	    {
		if (! strcmp(name, "invalid connections"))
		{
		    /*
		     * Must be referential
		     */
		    new = RefineCRefArray((Array)old, 8);
		    if (! new)
			goto error;
		}
	    }

	    /*
	     * It is possible that the refinement process created new
	     * arrays.  This is because you cannot extend regular arrays
	     * and they are used for constants.
	     */
	    if ((Object)old != (Object)new)
		if (! DXSetComponentValue(f, name, (Object)new))
		    goto error;
	}

	/*
	 * We wait until now to reset the connections component since it
	 * causes deletion of the old one, whose data is used in iTable[2]
	 * to designate the vertices to be interpolated for each cube-center
	 * point.
	 */
	if (! DXSetComponentValue(f, "connections", (Object)outC))
	    goto error;
	
	if (! DumpIPTables(f, nPoints, &invTable, &listElements))
	    goto error;
	
	outC = NULL;
	DXFree((Pointer)iTable[0].table);
	iTable[0].table = NULL;
    }
     

    return DXEndField(f);

error:
    if (listElements)
	DXDeleteSegList(listElements);
    if (invTable)
	DXFree((Pointer)invTable);
    if (eHash)
	DXDestroyHash(eHash);
    DXFree((Pointer)iTable[0].table);
    return NULL;
}

static Field
RefineITriangles(Field f, int levels)
{
    int         i, j, k, l;
    HashTable   eHash = NULL;
    InterpTable iTable[3];
    Array       inP, inC, outC = NULL;
    int         *c, *inElts, *outElts, nElts, nPoints;
    int  	eOffset;
    int		nEdges;
    Object	old;
    char	*name;
    ListHead    *invTable = NULL;
    SegList     *listElements = NULL;

    iTable[0].table = iTable[1].table = NULL;

    if (DXEmptyField(f))
	return DXEndField(DXNewField());
    
    if (! FieldSetup(f))
	goto error;
    
    for (l = 0; l < levels; l++)
    {
	inP = (Array)DXGetComponentValue(f, "positions");
	DXGetArrayInfo(inP, &nPoints, NULL, NULL, NULL, NULL);

	inC = (Array)DXGetComponentValue(f, "connections");
	if (! inC)
	{
	    DXSetError(ERROR_MISSING_DATA, "10250", "input", "connections");
	    goto error;
	}

	inElts = (int *)DXGetArrayData(inC);
	if (! inElts)
	    goto error;
	
	DXGetArrayInfo(inC, &nElts, NULL, NULL, NULL, NULL);

	outC = (Array)DXNewArray(TYPE_INT, CATEGORY_REAL, 1, 3);
	if (! outC)
	    goto error;

	if (! DXAddArrayData(outC, 0, 4*nElts, NULL))
	    goto error;
	
	outElts = (int *)DXGetArrayData(outC);
	if (! outElts)
	    goto error;

	eHash = DXCreateHash(EDGESIZE, Hash2, Cmp2);
	if (! eHash)
	    goto error;

	/*
	 * Put edges into to the hash tables
	 */
	c = inElts; nEdges = 0;
	for (i = 0; i < nElts; i++, c += 3)
	{
	    int *t;

	    t = tri_edgeTable; 
	    for (j = 0; j < 3; j++, t += 2)
		if (! AddEdgeToHash(eHash, c[t[0]], c[t[1]], &nEdges))
		    goto error;
	}

	/*
	 * for each input element, create grid containing indices in output
	 * positions component and resulting output elements.  We will be
	 * producing the input vertices first, the edge vertices next.
	 */

	eOffset = nPoints;
	/*cOffset = eOffset + nEdges;*/
	
	c = inElts;
	for (i = 0; i < nElts; i++, c += 3)
	{
	    int grid[6];
	    int *t, v;
	    int *b = subtriangles;

	    v = 0;

	    for (j = 0; j < 3; j++)
		grid[v++] = c[j];

	    t = tri_edgeTable; 
	    for (j = 0; j < 3; j++, t += 2)
	    {
		int indx = QueryEdge(eHash, c[t[0]], c[t[1]]);
		
		grid[v++] = indx + eOffset;
	    }

	    /*
	     * Now the grid is stuffed with the output vertex IDs.  
	     * Generate the output tetras.
	     */
	    for (j = 0; j < 4; j++)
		for (k = 0; k < 3; k++)
		    *outElts++ = grid[*b++];
	}

	if (! SetupIPTables(f, nPoints, &invTable, &listElements))
	    goto error;

	iTable[0].nV    = 2;
	iTable[0].knt   = nEdges;
	iTable[0].table = MakeTable(eHash, nEdges, 2);
	if (! iTable[0].table)
	    goto error;
	
	if (invTable)
	    if (! AddHashReferences(eHash,
				invTable, listElements, nPoints, 2))
		goto error;

	iTable[1].table = NULL;

	DXDestroyHash(eHash);
	eHash = NULL;

	for (i=0; NULL != (old = DXGetEnumeratedComponentValue(f, i, &name)); i++)
	{
	    Object attr;
	    Array new = (Array)old;
	    char *str;

	    if (! strcmp(name, "connections"))
		continue;
	    
	    attr = DXGetComponentAttribute(f, name, "dep");
	    if (attr)
	    {
		int boolean;

		boolean = ! strncmp(name, "invalid", 7);

		if (DXGetObjectClass(attr) != CLASS_STRING)
		{
		    DXSetError(ERROR_DATA_INVALID, "#10051",
						"dependency attribute");
		    goto error;
		}

		str = DXGetString((String)attr);

		if (! strcmp(str, "positions"))
		{
		    new = RefinePArray((Array)old, iTable, boolean);
		    if (! new)
			goto error;
		}
		else if (! strcmp(str, "connections"))
		{
		    new = RefineCArray((Array)old, 4);
		    if (! new)
			goto error;
		}
	    }
	    else
	    {
		if (! strcmp(name, "invalid connections"))
		{
		    /*
		     * Must be referential
		     */
		    new = RefineCRefArray((Array)old, 4);
		    if (! new)
			goto error;
		}
	    }

	    /*
	     * It is possible that the refinement process created new
	     * arrays.  This is because you cannot extend regular arrays
	     * and they are used for constants.
	     */
	    if ((Object)old != (Object)new)
		if (! DXSetComponentValue(f, name, (Object)new))
		    goto error;
	}

	/*
	 * We wait until now to reset the connections component since it
	 * causes deletion of the old one, whose data is used in iTable[2]
	 * to designate the vertices to be interpolated for each cube-center
	 * point.
	 */
	if (! DXSetComponentValue(f, "connections", (Object)outC))
	    goto error;
	
	if (! DumpIPTables(f, nPoints, &invTable, &listElements))
	    goto error;
	
	outC = NULL;
	DXFree((Pointer)iTable[0].table);
	iTable[0].table = NULL;
    }

    DXFree((Pointer)iTable[0].table);
     
    return DXEndField(f);

error:
    if (listElements)
	DXDeleteSegList(listElements);
    if (invTable)
	DXFree((Pointer)invTable);
    if (eHash)
	DXDestroyHash(eHash);
    DXFree((Pointer)iTable[0].table);
    return NULL;
}

static int *
MakeTable(HashTable h, int knt, int nVerts)
{
    int *table;
    Hash hash;

    table = (int *)DXAllocate(knt*nVerts*sizeof(int));
    if (! table)
	goto error;
    
    if (! DXInitGetNextHashElement(h))
	goto error;
    
    while (NULL != (hash = (Hash)DXGetNextHashElement(h)))
    {
	int i, *tPtr = table + hash->index*nVerts;

	for (i = 0; i < nVerts; i++)
	    *tPtr++ = hash->v[i];
    }

    return table;

error:
    if (table)
	DXFree((Pointer)table);
    return NULL;
}

static Error
FieldSetup(Field f)
{
    char *rmv[64];
    int  nrmv = 0, i;
    Object o;
    Array a;
    char *name;

    for (i = 0; NULL != (o = DXGetEnumeratedComponentValue(f, i, &name)); i++)
    {
	/*
 	 * if its connections, keep it.
	 */
	if (! strcmp(name, "connections"))
	    continue;
	
 	/*
	 * If it is an invalid array, keep it
	 */
	if (! strncmp(name, "invalid", 7))
	    goto keep;
	
	/*
	 * Otherwise, if it refs, get rid of it.
	 */
	if (DXGetComponentAttribute(f, name, "ref"))
	{
	    rmv[nrmv++] = name;
	    continue;
	}
	    
	/*
	 * If its derived, get rid of it.
	 */
	if (DXGetComponentAttribute(f, name, "der"))
	{
	    rmv[nrmv++] = name;
	    continue;
	}
    
keep:
	/*
	 * Otherwise, replace it with a copy.
	 */
	a = CopyArray((Array)o);
	if (! a)
	    goto error;
	
	if (! DXSetComponentValue(f, name, (Object)a))
	    goto error;
    }

    /*
     * Now delete the components we don't want
     */
    for (i = 0; i < nrmv; i++)
	DXDeleteComponent(f, rmv[i]);
	
    return OK;

error:
    return ERROR;
}
static Array
CopyArray(Array in)
{
    int      nIn;
    Array    out=NULL;
    Type     type;
    Category cat;
    int	     rank, shape[32];

    if (DXGetObjectClass((Object)in) != CLASS_ARRAY)
	goto error;
    
    DXGetArrayInfo(in, &nIn, &type, &cat, &rank, shape);

    /*
     * If the array is constant, it will have to be copied 
     * each time through the loop, anyway, so its not necessary to copy
     * it here.
     */
    if (DXQueryConstantArray(in, NULL, NULL))
	return in;

    /*
     * Neither grid positions or constant, go create
     * an irregular output.
     */
    out = DXNewArrayV(type, cat, rank, shape);
    if (! out)
	goto error;

    if (! DXAddArrayData(out, 0, nIn, DXGetArrayData(in)))
	goto error;
    
/* done */
    return out;

error:
    if (out)
	DXDelete((Object)out);

    return NULL;
}

#define AVERAGE(type, itable)						\
{									\
    type *inPtr  = (type *)DXGetArrayData(out);				\
    type *outPtr;							\
    int  i, j, k, t, l, m;						\
									\
    if (! inPtr)							\
	goto error;							\
									\
    outPtr = inPtr + nIn*nVars;						\
    for (t = 0; itable[t].table != NULL; t++)				\
    {									\
	int  nV  = itable[t].nV;					\
	int  knt = itable[t].knt;					\
	int *tab = itable[t].table;					\
	float d = 1.0 / itable[t].nV;					\
									\
	for (i = 0; i < knt; i++, tab += nV)				\
	{								\
	    type *xtab[8];						\
									\
	    for (j = 0; j < nV; j++)					\
		xtab[j] = inPtr + tab[j]*nVars;				\
									\
	    for (k = 0; k < nVars; k++)					\
	    {								\
		*outPtr = 0.0;						\
		for (l = 0; l < nV; l++)				\
		{							\
		    type  max = *xtab[l];				\
		    int   maxi = -1;					\
		    for (m = l+1; m < nV; m++)				\
			if (max < *xtab[m])				\
			{						\
			    maxi = m; 					\
			    max  = *xtab[m];				\
			}						\
		    if (maxi != -1)					\
		    {							\
			type *t;					\
			t       = xtab[maxi];				\
			xtab[maxi] = xtab[l];				\
			xtab[l] = t;					\
		    }							\
		    *outPtr += max;					\
		    xtab[l] += 1;					\
		}							\
		*outPtr++ *= d;						\
	    }								\
	}								\
    }									\
}

#define BOOLEAN(itable)							\
{									\
    ubyte *inPtr  = (ubyte *)DXGetArrayData(out);			\
    ubyte *outPtr;							\
    int  i, j, t;							\
									\
    if (! inPtr)							\
	goto error;							\
									\
    outPtr = inPtr + nIn;						\
    for (t = 0; itable[t].table != NULL; t++)				\
    {									\
	int  nV  = itable[t].nV;					\
	int  knt = itable[t].knt;					\
	int *tab = itable[t].table;					\
									\
	for (i = 0; i < knt; i++, tab += nV, outPtr++)			\
	{								\
	    *outPtr = 0;						\
	    for (j = 0; j < nV; j++)					\
		if (inPtr[tab[j]] == 1) {				\
		    *outPtr = 1;					\
		    break;						\
		}							\
	}								\
    }									\
}

static Array
RefinePArray(Array in, InterpTable *table, int boolean)
{
    int      nIn;
    Array    out=NULL;
    Type     type;
    Category cat;
    int	     rank, shape[32], i;
    int      nNew = 0, nVars, itemSize;

    if (DXGetObjectClass((Object)in) != CLASS_ARRAY)
	goto error;
    
    DXGetArrayInfo(in, &nIn, &type, &cat, &rank, shape);

    itemSize = DXGetItemSize(in);

    for (i = 0; table[i].table != NULL; i ++)
	nNew += table[i].knt;

    if (DXQueryConstantArray(in, NULL, NULL))
    {
	out = (Array)DXNewConstantArrayV(nIn+nNew, DXGetConstantArrayData(in),
							type, cat, rank, shape);
    }
    else
    {
	out = in;

	if (! DXAddArrayData(out, nIn, nNew, NULL))
	    goto error;
	
	nVars = itemSize / DXTypeSize(type);

	if (boolean)
	{
	    if (type != TYPE_BYTE && type != TYPE_UBYTE)
	    {
		DXSetError(ERROR_DATA_INVALID, 
			"invalid data flag must be byte or ubyte");
		goto error;
	    }

	    if (nVars != 1)
	    {
		DXSetError(ERROR_DATA_INVALID, 
			"invalid data flag must be scalar or 1-vector");
		goto error;
	    }

	    BOOLEAN(table);
	}
	else
	{
	    switch(type)
	    {
		case TYPE_UBYTE:
		    AVERAGE(ubyte, table);
		    break;
		case TYPE_BYTE:
		    AVERAGE(byte, table);
		    break;
		case TYPE_USHORT:
		    AVERAGE(ushort, table);
		    break;
		case TYPE_SHORT:
		    AVERAGE(short, table);
		    break;
		case TYPE_UINT:
		    AVERAGE(uint, table);
		    break;
		case TYPE_INT:
		    AVERAGE(int, table);
		    break;
		case TYPE_FLOAT:
		    AVERAGE(float, table);
		    break;
		case TYPE_DOUBLE:
		    AVERAGE(double, table);
		    break;
		default:
		    DXSetError(ERROR_DATA_INVALID, "#10320", "input");
		    goto error;
	    }
	}
    }

    return out;

error:
    if (out)
	DXDelete((Object)out);
    return NULL;
}

static Array
RefineCArray(Array in, int nCopies)
{
    int           nIn;
    Array         out=NULL;
    Type          type;
    Category      cat;
    int	          rank, shape[32], i, j;
    int           itemSize;
    byte          *inPtr, *outPtr;

    if (DXGetObjectClass((Object)in) != CLASS_ARRAY)
	goto error;
    
    DXGetArrayInfo(in, &nIn, &type, &cat, &rank, shape);

    if (DXQueryConstantArray(in, NULL, NULL))
    {
	out = (Array)DXNewConstantArrayV(nIn*nCopies,
		DXGetConstantArrayData(in), type, cat, rank, shape);
    }
    else
    {
	itemSize = DXGetItemSize(in);

	out = DXNewArrayV(type, cat, rank, shape);
	if (! out)
	    goto error;

	if (! DXAddArrayData(out, 0, nIn*nCopies, NULL))
	    goto error;
    
	inPtr  = (byte *)DXGetArrayData(in);
	outPtr = (byte *)DXGetArrayData(out);
	if (! inPtr || ! outPtr)
	    goto error;
    
	for (i = 0; i < nIn; i++, inPtr += itemSize)
	    for (j = 0; j < nCopies; j++, outPtr += itemSize)
		memcpy(outPtr, inPtr, itemSize);
    }

    return out;

error:
    if (out)
	DXDelete((Object)out);
    return NULL;
}

static Hash
_QueryHash(HashTable h, Hash hash)
{
    return (Hash)DXQueryHashElement(h, (Key)hash);
}

static void
SetupEdge(int p, int q, Hash e)
{
   if (p > q)
   {
       e->v[0] = p;
       e->v[1] = q;
   }
   else
   {
       e->v[0] = q;
       e->v[1] = p;
   }
}

static int
QueryEdge(HashTable h, int p, int q)
{
    struct hash hash;
    Hash hptr;

    SetupEdge(p, q, &hash);

    hptr = _QueryHash(h, &hash);
    if (hptr)
	return hptr->index;
    else
	return -1;
}

static Error
AddEdgeToHash(HashTable h, int p, int q, int *nEdges)
{
    struct hash hash;

    SetupEdge(p, q, &hash);

    if (_QueryHash(h, &hash))
	return OK;

    hash.index = *nEdges;
    *nEdges = *nEdges + 1;

    return DXInsertHashElement(h, (Element)&hash);
}

static void
SetupQuad(int p, int q, int r, int s, Hash h)
{
    int i;

    h->v[0] = p; h->v[1] = q; h->v[2] = r; h->v[3] = s;

    for (i = 0; i < 3; i++)
    {
	int max = h->v[i];
	int j, k = -1;
	for (j = i+1; j < 4; j++)
	    if (h->v[j] > max)
	    {
		k = j;
		max = h->v[j];
	    }
	if (k != -1)
	{
	    int t = h->v[i];
	    h->v[i] = h->v[k];
	    h->v[k] = t;
	}
    }
}
    
static int
QueryQuad(HashTable h, int p, int q, int r, int s)
{
    struct hash hash;
    Hash hptr;

    SetupQuad(p, q, r, s, &hash);

    hptr = _QueryHash(h, &hash);
    if (hptr)
	return hptr->index;
    else
	return -1;
}

static Error
AddQuadToHash(HashTable h, int p, int q, int r, int s, int *nQuad)
{
    struct hash hash;

    SetupQuad(p, q, r, s, &hash);

    if (_QueryHash(h, &hash))
	return OK;

    hash.index = *nQuad;
    *nQuad = *nQuad + 1;

    return DXInsertHashElement(h, (Element)&hash);
}

static PseudoKey
Hash4(Key k)
{
    Hash h = (Hash)k;
    return h->v[0]*17 + h->v[1]*53 + h->v[2]*1907 + h->v[3]*2801;
}

#if 0
static int
Cmp4(Key k0, Key k1)
{
    Hash h0 = (Hash)k0;
    Hash h1 = (Hash)k1;

    return ! (h0->v[0]==h1->v[0] &&
	      h0->v[1]==h1->v[1] &&
	      h0->v[2]==h1->v[2] &&
	      h0->v[3]==h1->v[3]);
}
#endif

static PseudoKey
Hash2(Key k)
{
    Hash h = (Hash)k;
    return h->v[0]*17 + h->v[1]*53;
}

static int
Cmp2(Key k0, Key k1)
{
    Hash h0 = (Hash)k0;
    Hash h1 = (Hash)k1;

    return ! (h0->v[0]==h1->v[0] && h0->v[1]==h1->v[1]);
}

static Error
AddHashReferences(HashTable h, ListHead *invTable,
		  SegList *listElements, int base, int nv)
{
    Hash hash;
    int  n = base;

    if (! DXInitGetNextHashElement(h))
	goto error;
    
    while (NULL != (hash = (Hash)DXGetNextHashElement(h)))
    {
	int i;

	for (i = 0; i < nv; i++)
	{
	    int index = hash->v[i];
	    if (invTable[index].invalid)
	    {
		ListElement le = DXNewSegListItem(listElements);
		if (! le)
		    goto error;

		le->index = n;
		le->next  = invTable[index].head;
		invTable[index].head = le;
	    }
	}

	n++;
    }

    return OK;

error:

    return ERROR;
}

static Error
AddReferences(int *list, int knt, ListHead *invTable,
		SegList *listElements, int base, int nv)
{
    int i, j;

    for (i = 0; i < knt; i++)
    {
	for (j = 0; j < nv; j++)
	{
	    int index = *list++;
	    if (invTable[index].invalid)
	    {
		ListElement le = DXNewSegListItem(listElements);
		if (! le)
		    goto error;

		le->index = base + i;
		le->next  = invTable[index].head;
		invTable[index].head = le;
	    }
	}
    }

    return OK;

error:

    return ERROR;
}




static Error
SetupIPTables(Field f, int nPoints,
	      ListHead **invTable, SegList **listElements)
{
    InvalidComponentHandle iph = NULL;
    int i;

    *invTable = NULL;
    *listElements = NULL;

    if (DXGetComponentAttribute(f, "invalid positions", "ref"))
    {
	int index;

	*invTable = (ListHead *)DXAllocateZero(nPoints * sizeof(ListHead));
	if (! *invTable)
	    goto error;

	for (i = 0; i < nPoints; i++ )
	    (*invTable)[i].head = NULL;

	iph = DXCreateInvalidComponentHandle((Object)f, NULL, "positions");
	if (! iph)
	    goto error;
	
	DXInitGetNextInvalidElementIndex(iph);
	while (-1 != (index = DXGetNextInvalidElementIndex(iph)))
	    invTable[index]->invalid = 1;
	
	*listElements = DXNewSegList(sizeof(struct listElement),
				     nPoints*2, nPoints);
	if (! *listElements)
	    goto error;
    
	DXFreeInvalidComponentHandle(iph);
	iph = NULL;

	DXDeleteComponent(f, "invalid positions");
    }

    return OK;

error:
    if (iph)
       DXFreeInvalidComponentHandle(iph);
    if (*invTable)
	DXFree((Pointer)*invTable);
    if (*listElements)
	DXDeleteSegList(*listElements);
    *invTable = NULL;
    *listElements = NULL;

    return ERROR;
}


static Error
DumpIPTables(Field f, int nPoints,
	     ListHead **invTable, SegList **listElements)
{
    InvalidComponentHandle iph = NULL;

    if (*invTable)
    {
	int i;
	ListElement le;

	iph = DXCreateInvalidComponentHandle((Object)f, NULL, "positions");
	if (! iph)
	    goto error;

	for (i = 0; i < nPoints; i++)
	    if ((*invTable)[i].invalid)
	    {
		if (! DXSetElementInvalid(iph, i))
		    goto error;

                le = (*invTable)[i].head;
		do
		{
		    if (! DXSetElementInvalid(iph, le->index))
			goto error;
		    
		    le = le->next;

		} while (le != NULL);
	    }
	    
	if (! DXSaveInvalidComponent(f, iph))
	    goto error;
	
	DXFreeInvalidComponentHandle(iph);
	iph = NULL;

	DXDeleteSegList(*listElements);
	*listElements = NULL;

	DXFree((Pointer)*invTable);
	*invTable = NULL;
    }

    return OK;

error:
    if (iph)
	DXFreeInvalidComponentHandle(iph);

    return ERROR;
}

static Array
RefineCRefArray(Array in, int outPerIn)
{
    Type t;
    Category c;
    int r, s[32], nIn, i, nRefs, *sPtr, *dPtr;
    Array out = NULL;

    if (! DXGetArrayInfo(in, &nIn, &t, &c, &r, s))
	goto error;
    
    if (t != TYPE_INT && t != TYPE_UINT)
    {
	DXSetError(ERROR_DATA_INVALID, "ref components must be int or uint");
	goto error;
    }
    
    out = DXNewArrayV(t, c, r, s);
    if (! out)
	goto error;
    
    if (! DXAddArrayData(out, 0, nIn*outPerIn, NULL))
	goto error;
    
    nRefs = (DXGetItemSize(in) / sizeof(int));

    if (nRefs != 1)
    {
	DXSetError(ERROR_DATA_INVALID,
		"invalid references must be scalar or 1-vector");
	goto error;
    }

    sPtr = (int *)DXGetArrayData(in);
    dPtr = (int *)DXGetArrayData(out);

    for (i = 0; i < nIn; i++, sPtr ++)
    {
	int j;

	for (j = 0; j < outPerIn; j++)
	    *dPtr++ = (*sPtr * outPerIn) + j;
    }

    return out;

error:
    DXDelete((Object)out);
    return NULL;
}
