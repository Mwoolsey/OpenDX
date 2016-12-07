
/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


/*
 * Header: /usr/people/gresh/code/svs/src/libdx/RCS/invalid.c,v 5.1 93/01/20 14:26:14 gda Exp Locker: gresh 
 *
 */

#include <stdio.h>
#include <string.h>
#include <dx/dx.h>

static Array   CullArray(Array, int, int, InvalidComponentHandle);

static Array   _dxfReRef(Array, int *);
static int    *MakeMap(InvalidComponentHandle, int, int *);

static Object  C_Field(Field);
static Object  C_Field_Standard(Field);
static Object  C_Field_PE(Field);
static Object  C_Field_FLE(Field);

static Object  IC_Field(Field);
static Object  IC_Field_Standard(Field);
static Object  IC_Field_PE(Field);
static Object  IC_Field_FLE(Field);

static Object  IUP_Field(Field);
static Object  IUP_Field_Standard(Field);
static Object  IUP_Field_PE(Field);
static Object  IUP_Field_FLE(Field);

/* --FIXME 
 * Never defined --remove after determining not used
 *

static Object  C_Object(Object);
static Object  IC_Object(Object);
static Object  IUP_Object(Object);

 */

static Field DeleteFieldContents(Field);

/*
 * The following checks to find the maximum point reference in the
 * connections array and whether the references skip any in the
 * range.  This is a fast out for InvalidateUnreferencedPositions...
 * if the max. reference matches the  length of the positions array
 * and no holes were found, then no points are unreferenced.
 */
#define ERROR_OCCURRED	-1
#define HOLES_FOUND	-2
static int GridSize(Array);

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

/*
 * DEP ARRAY lower-bound thresholds
 *   DEP_STORAGE_THRESHOLD - When to switch to a DEP ARRAY when writing a field
 *                           component (< -- SORTED_LIST; >= -- DEP_ARRAY)
 *   DEP_MEM_THRESHOLD     - When to use a DEP for in-memory 
 *                           (i.e. InvalidComponentHandle) operations
 *
 *   These are different because one should favor storage size, while the
 *   other should favors fast processing.  
 */
#define DEP_STORAGE_THRESHOLD(nItems) ( 0.20*(nItems) )
#define DEP_MEM_THRESHOLD(nItems)     ( 1024 )


#define TRAVERSE(objectMethod, fieldMethod)				\
{									\
    switch(DXGetObjectClass(object))					\
    {									\
	case CLASS_FIELD:						\
	    return (fieldMethod)((Field)object);			\
									\
	case CLASS_GROUP:						\
	{								\
	    int i = 0;							\
	    Group g = (Group)object;					\
	    Object c;							\
	    while(NULL != (c = DXGetEnumeratedMember(g, i++, NULL)))	\
	    {								\
		if (NULL == (objectMethod)(c))				\
		    return NULL;					\
	    }								\
	    return object;						\
	}								\
									\
	case CLASS_XFORM:						\
	{								\
	    Object c;							\
	    if (! DXGetXformInfo((Xform)object, &c, NULL))		\
		return NULL;						\
	    c = (objectMethod)(c);					\
	    if (! c)							\
		return NULL;						\
	    return object;						\
	}								\
									\
	case CLASS_CLIPPED:						\
	{								\
	    Object c;							\
	    if (! DXGetClippedInfo((Clipped)object, &c, NULL))		\
		return NULL;						\
	    c = (objectMethod)(c);					\
	    if (! c)							\
		return NULL;						\
	    return object;						\
	}								\
									\
	default:							\
	    return object;						\
    }									\
}


Object
DXCull(Object object)
{
    TRAVERSE(DXCull, C_Field);
}

Object
DXInvalidateConnections(Object object)
{
    TRAVERSE(DXInvalidateConnections, IC_Field);
}


Object
DXInvalidateUnreferencedPositions(Object object)
{
    TRAVERSE(DXInvalidateUnreferencedPositions, IUP_Field);
}

Object
DXCullConditional(Object object)
{
    return DXCull(object);
}

static Object
C_Field(Field field)
{
    if (DXGetComponentValue(field, "polylines"))
    {
	return C_Field_PE(field);
    }
    else if (DXGetComponentValue(field, "faces"))
    {
	return C_Field_FLE(field);
    }
    else
    {
	return C_Field_Standard(field);
    }
}

static Object
C_Field_PE(Field field)
{
    Array			pArray,   plArray, eArray;
    Array			child, 	  newChild;
    int				nPositions, nPolylines, nEdges, nInvP, nInvPl;
    int				nNewPositions, nNewPolylines;
    int				i, j, k, knt;
    int				*validPositionsMap, *validPolylinesMap;
    Object			attr;
    char			*reference, *dependence;
    char			*name;
    char			*toDelete[32];
    int				nDelete = 0;
    InvalidComponentHandle 	vpHandle = NULL;
    InvalidComponentHandle	vplHandle = NULL;
    Array			nPlArray = NULL, nEArray = NULL;
    int				*polylines, *npolylines;
    int				*edges, *nedges;

    if (DXEmptyField(field))
	return (Object)field;

    vplHandle 	        = NULL;
    vpHandle 	        = NULL;
    validPolylinesMap = NULL;
    validPositionsMap   = NULL;
    child 	        = NULL;

    pArray = (Array)DXGetComponentValue(field, "positions");
    if (! pArray)
    {
	DXSetError(ERROR_MISSING_DATA, "positions component");
	goto error;
    }

    if (! DXGetArrayInfo(pArray, &nPositions, NULL, NULL, NULL, NULL))
	goto error;

    plArray = (Array)DXGetComponentValue(field, "polylines");
    if (plArray)
    {
	if (! DXGetArrayInfo(plArray, &nPolylines, NULL, NULL, NULL, NULL))
	    goto error;
    }
    else
	nPolylines = 0;
    
    eArray = (Array)DXGetComponentValue(field, "edges");
    if (eArray)
    {
	if (! DXGetArrayInfo(eArray, &nEdges, NULL, NULL, NULL, NULL))
	    goto error;
    }
    else
	nEdges = 0;
    
    vpHandle = DXCreateInvalidComponentHandle((Object)field, NULL, "positions");
    if (! vpHandle)
	goto error;
    
    nInvP = DXGetInvalidCount(vpHandle);
    if (nInvP)
    {
	if (NULL == (validPositionsMap = 
		    MakeMap(vpHandle, nPositions, &nNewPositions)))
	    goto error;
	
	if (nNewPositions == 0)
	{
	    if (! (Object)DeleteFieldContents(field))
		goto error;
	    
	    goto done;
	}
    }
    else
    {
	DXFreeInvalidComponentHandle(vpHandle);
	vpHandle = NULL;
    }

    if (plArray)
    {
	vplHandle = DXCreateInvalidComponentHandle((Object)field, NULL,
						"polylines");
	if (! vplHandle)
	    goto error;

	nInvPl = DXGetInvalidCount(vplHandle);
    }
    else
	nInvPl = 0;

    if (nInvPl)
    {
	if (NULL == (validPolylinesMap = 
		    MakeMap(vplHandle, nPolylines, &nNewPolylines)))
	    goto error;
    }
    else
    {
	DXFreeInvalidComponentHandle(vplHandle);
	vplHandle = NULL;
    }

    if (nInvP == 0 && nInvPl == 0)
    {
	DXDeleteComponent(field, "invalid polylines");
	DXDeleteComponent(field, "invalid positions");
	return (Object)field;
    }

    /*
     * Handle polylines and edges components
     */
    polylines = (int *)DXGetArrayData(plArray);
    edges = (int *)DXGetArrayData(eArray);

    /*
     * count the resulting edges
     */
    for (i = 0, knt = 0; i < nPolylines; i++)
    {
	if (DXIsElementValid(vplHandle, i))
	{
	    int start = polylines[i];
	    int end   = (i == nPolylines-1) ? nEdges : polylines[i+1];

	    knt += end - start;
	}
    }

    nPlArray = DXNewArray(TYPE_INT, CATEGORY_REAL, 0);
    nEArray  = DXNewArray(TYPE_INT, CATEGORY_REAL, 0);
    if (! nPlArray || ! nEArray)
	goto error;
    
    if (! DXAddArrayData(nPlArray, 0, (nPolylines-nInvPl), NULL))
	goto error;

    if (! DXAddArrayData(nEArray, 0, knt, NULL))
	goto error;
    
    npolylines = (int *)DXGetArrayData(nPlArray);
    nedges     = (int *)DXGetArrayData(nEArray);

    for (i = 0, j = 0, k = 0; i < nPolylines; i++)
    {
	if (DXIsElementValid(vplHandle, i))
	{
	    int start = polylines[i];
	    int end   = (i == nPolylines-1) ? nEdges : polylines[i+1];
	    int n     = end - start;
	    int ii;

	    npolylines[j++] = k;

	    for (ii = 0; ii < n; ii++)
		nedges[k + ii] = edges[start + ii];

	    k += n;
	}
    }

    if (! DXSetAttribute((Object)nPlArray, "ref",
			(Object)DXNewString("edges")))
	goto error;

    if (! DXSetComponentValue(field, "polylines", (Object)nPlArray))
	goto error;
    nPlArray = NULL;

    if (! DXSetAttribute((Object)nEArray, "ref",
			(Object)DXNewString("positions")))
	goto error;

    if (! DXSetComponentValue(field, "edges", (Object)nEArray))
	goto error;
    nEArray = NULL;

    i = 0;
    while (NULL != 
	(child = (Array)DXGetEnumeratedComponentValue(field, i, &name)))
    {
	if (! strncmp(name, "invalid ", 8))
	{
	    toDelete[nDelete++] = name;
	    i++;
	    continue;
	}

	if (! strcmp(name, "polylines"))
	{
	    i++;
	    continue;
	}

	attr = DXGetEnumeratedComponentAttribute(field, i, NULL, "der");
	if (attr && strcmp(name, "neighbors"))
	{
	    toDelete[nDelete++] = name;
	    i++;
	    continue;
	}
	
	DXReference((Object)child);

	dependence = NULL;

	if (! strcmp(name, "positions"))
	    dependence = "positions";
	else if (! strcmp(name, "polylines"))
	    dependence = "polylines";
	else
	{
	    attr = DXGetEnumeratedComponentAttribute(field, i, NULL, "dep");
	    if (attr)
		dependence = DXGetString((String)attr);
	}

	/*
	 * If the component is dependent on something, then do to the
	 * component whatever is appropriate for the component on which
	 * it is dependent.
	 */
	if (dependence)
	{
	    /*
	     * If its dependent on positions...
	     */
	    if (! strcmp(dependence, "positions"))
	    {
		/*
		 *  and there are invalidated positions, cull the component
		 */
		if (vpHandle)
		{
		    if (nNewPositions == 0)
		    {
			toDelete[nDelete++] = name;
			DXDelete((Object)child);
			child = NULL;
			i++;
			continue;
		    }
		    else
		    {
			newChild = CullArray(child, nPositions, 
						nNewPositions, vpHandle);
			if (! newChild)
			    goto error;

			DXDelete((Object)child);
			child = (Array)DXReference((Object)newChild);
		    }
		}
	    }
	    /*
	     * Else if its dependent on polylines...
	     */
	    else if (! strcmp(dependence, "polylines"))
	    {
		if (! plArray)
		{
		    DXSetError(ERROR_DATA_INVALID, 
			    "component dependent on nonexistent polylines");
		    goto error;
		}
			    
		/*
		 *  and there is a validPolylines array, cull the component.
		 */
		if (vplHandle)
		{
		    if (nNewPolylines == 0)
		    {
			toDelete[nDelete++] = name;
			DXDelete((Object)child);
			child = NULL;
			i++;
			continue;
		    }
		    else
		    {
			newChild = CullArray(child, nPolylines, 
					nNewPolylines, vplHandle);
			
			if (! newChild)
			    goto error;

			DXDelete((Object)child);
			child = (Array)DXReference((Object)newChild);
		    }
		}
	    }
	}

	/*
	 * Now we consider components that ref.  If the component they ref
	 * has been culled, we need to renumber the references to reflect
	 * the culled referenced array.
	 */
	attr = DXGetEnumeratedComponentAttribute(field, i, NULL, "ref");
	if (attr)
	{
	    reference = DXGetString((String)attr);

	    /*
	     * If it references polylines...
	     */
	    if (!strcmp(reference, "polylines"))
	    {
		if (! plArray)
		{
		    DXSetError(ERROR_DATA_INVALID, 
			    "component refers to nonexistent polylines");
		    goto error;
		}

		/*
		 * And polylines have been culled, renumber the references.
		 */
		if (validPolylinesMap)
		{
		    newChild = _dxfReRef(child, validPolylinesMap);
		    if (! newChild)
			goto error;

		    DXDelete((Object)child);
		    child = (Array)DXReference((Object)newChild);
		}
	    }
	    /*
	     * Else if it references positions...
	     */
	    else if (!strcmp(reference, "positions"))
	    {
		/*
		 * And positions have been culled, renumber the references.
		 */
		if (validPositionsMap)
		{
		    newChild = _dxfReRef(child, validPositionsMap);
		    if (! newChild)
			goto error;

		    DXDelete((Object)child);
		    child = (Array)DXReference((Object)newChild);
		}
	    }
	}

	if (! DXSetComponentValue(field, name, (Object)child))
	    goto error;

	/*
	 * Remove local reference
	 */
	DXDelete((Object)child);
	child = NULL;
    
	i++;
    }

    for (nDelete--; nDelete >= 0; nDelete--)
	DXDeleteComponent(field, toDelete[nDelete]);

    field = DXEndField(field);

done:
    DXFree((Pointer)validPolylinesMap);
    DXFree((Pointer)validPositionsMap);
    DXFreeInvalidComponentHandle(vpHandle);
    DXFreeInvalidComponentHandle(vplHandle);

    return (Object)field;

error:
    DXDelete((Object)nPlArray);
    DXDelete((Object)nEArray);
    DXDelete((Object)child);
    DXFreeInvalidComponentHandle(vpHandle);
    DXFreeInvalidComponentHandle(vplHandle);
    DXFree((Pointer)validPolylinesMap);
    DXFree((Pointer)validPositionsMap);

    return NULL;
}

static Object
C_Field_FLE(Field field)
{
    Array			pArray, fArray, lArray, eArray;
    Array			child, newChild;
    int				nPositions, nFaces, nLoops;
    int				nEdges, nInvP, nInvF;
    int				nNewPositions, nNewFaces;
    int				i, f, nfknt, nlknt, neknt;
    int				*validPositionsMap, *validFacesMap;
    Object			attr;
    char			*reference, *dependence;
    char			*name;
    char			*toDelete[32];
    int				nDelete = 0;
    InvalidComponentHandle 	vpHandle = NULL;
    InvalidComponentHandle	vfHandle = NULL;
    Array			nLArray = NULL, nFArray = NULL, nEArray = NULL;
    int				*faces, *nfaces;
    int				*loops, *nloops;
    int				*edges, *nedges;

    if (DXEmptyField(field))
	return (Object)field;

    vfHandle 	        = NULL;
    vpHandle 	        = NULL;
    validFacesMap       = NULL;
    validPositionsMap   = NULL;
    child 	        = NULL;

    pArray = (Array)DXGetComponentValue(field, "positions");
    if (! pArray)
    {
	DXSetError(ERROR_MISSING_DATA, "positions component");
	goto error;
    }

    if (! DXGetArrayInfo(pArray, &nPositions, NULL, NULL, NULL, NULL))
	goto error;

    fArray = (Array)DXGetComponentValue(field, "faces");
    if (fArray)
    {
	if (! DXGetArrayInfo(fArray, &nFaces, NULL, NULL, NULL, NULL))
	    goto error;
    }
    else
	nFaces = 0;
    
    lArray = (Array)DXGetComponentValue(field, "loops");
    if (lArray)
    {
	if (! DXGetArrayInfo(lArray, &nLoops, NULL, NULL, NULL, NULL))
	    goto error;
    }
    else
	nLoops = 0;
    
    eArray = (Array)DXGetComponentValue(field, "edges");
    if (eArray)
    {
	if (! DXGetArrayInfo(eArray, &nEdges, NULL, NULL, NULL, NULL))
	    goto error;
    }
    else
	nEdges = 0;
    
    vpHandle = DXCreateInvalidComponentHandle((Object)field, NULL, "positions");
    if (! vpHandle)
	goto error;
    
    nInvP = DXGetInvalidCount(vpHandle);
    if (nInvP)
    {
	if (NULL == (validPositionsMap = 
		    MakeMap(vpHandle, nPositions, &nNewPositions)))
	    goto error;
	
	if (nNewPositions == 0)
	{
	    if (! (Object)DeleteFieldContents(field))
		goto error;
	    
	    goto done;
	}
    }
    else
    {
	DXFreeInvalidComponentHandle(vpHandle);
	vpHandle = NULL;
    }

    if (fArray)
    {
	vfHandle = DXCreateInvalidComponentHandle((Object)field, NULL,
						"faces");
	if (! vfHandle)
	    goto error;

	nInvF = DXGetInvalidCount(vfHandle);
    }
    else
	nInvF = 0;

    if (nInvF)
    {
	if (NULL == (validFacesMap = 
		    MakeMap(vfHandle, nFaces, &nNewFaces)))
	    goto error;
    }
    else
    {
	DXFreeInvalidComponentHandle(vfHandle);
	vfHandle = NULL;
    }

    if (nInvP == 0 && nInvF == 0)
    {
	DXDeleteComponent(field, "invalid faces");
	DXDeleteComponent(field, "invalid positions");
	return (Object)field;
    }

    /*
     * Handle polylines and edges components
     */
    faces = (int *)DXGetArrayData(fArray);
    loops = (int *)DXGetArrayData(lArray);
    edges = (int *)DXGetArrayData(eArray);

    /*
     * count the resulting loops and edges
     */
    for (f = 0, nfknt = nlknt = neknt = 0; f < nFaces; f++)
    {
	if (!vfHandle || DXIsElementValid(vfHandle, f))
	{
	    int lstart = faces[f];
	    int lend   = (f == nFaces-1) ? nLoops : faces[f+1];
	    int l;

	    for (l = lstart; l < lend; l++)
	    {
		int estart = loops[l];
		int eend   = (l == nLoops-1) ? nEdges : loops[l+1];
		neknt += eend - estart ;
	    }

	    nlknt += lend - lstart;
	    nfknt ++;
	}
    }

    nFArray = DXNewArray(TYPE_INT, CATEGORY_REAL, 0);
    nLArray = DXNewArray(TYPE_INT, CATEGORY_REAL, 0);
    nEArray = DXNewArray(TYPE_INT, CATEGORY_REAL, 0);
    if (! nFArray || ! nEArray || ! nLArray)
	goto error;
    
    if (! DXAddArrayData(nFArray, 0, nfknt, NULL))
	goto error;

    if (! DXAddArrayData(nLArray, 0, nlknt, NULL))
	goto error;
    
    if (! DXAddArrayData(nEArray, 0, neknt, NULL))
	goto error;
    
    nfaces = (int *)DXGetArrayData(nFArray);
    nloops = (int *)DXGetArrayData(nLArray);
    nedges = (int *)DXGetArrayData(nEArray);

    for (f = 0, nfknt = neknt = nlknt = 0; f < nFaces; f++)
    {
	if (!vfHandle || DXIsElementValid(vfHandle, f))
	{
	    int lstart = faces[f];
	    int lend   = (f == nFaces-1) ? nLoops : faces[f+1];
	    int l;

	    nfaces[nfknt++] = nlknt;

	    for (l = lstart; l < lend; l++)
	    {
		int estart = loops[l];
		int eend   = (l == nLoops-1) ? nEdges : loops[l+1];
		int e;

		nloops[nlknt++] = neknt;

		for (e = estart; e < eend; e++)
		    nedges[neknt++] = edges[e];
	    }
	}
    }

    if (! DXSetAttribute((Object)nFArray, "ref",
			(Object)DXNewString("loops")))
	goto error;

    if (! DXSetComponentValue(field, "faces", (Object)nFArray))
	goto error;
    nFArray = NULL;

    if (! DXSetAttribute((Object)nLArray, "ref",
			(Object)DXNewString("edges")))
	goto error;

    if (! DXSetComponentValue(field, "loops", (Object)nLArray))
	goto error;
    nLArray = NULL;

    if (! DXSetAttribute((Object)nEArray, "ref",
			(Object)DXNewString("positions")))
	goto error;

    if (! DXSetComponentValue(field, "edges", (Object)nEArray))
	goto error;
    nEArray = NULL;

    i = 0;
    while (NULL != 
	(child = (Array)DXGetEnumeratedComponentValue(field, i, &name)))
    {
	if (! strncmp(name, "invalid ", 8))
	{
	    toDelete[nDelete++] = name;
	    i++;
	    continue;
	}

	if (! strcmp(name, "faces") ||
	    ! strcmp(name, "loops"))
	{
	    i++;
	    continue;
	}

	attr = DXGetEnumeratedComponentAttribute(field, i, NULL, "der");
	if (attr && strcmp(name, "neighbors"))
	{
	    toDelete[nDelete++] = name;
	    i++;
	    continue;
	}
	
	DXReference((Object)child);

	dependence = NULL;

	if (! strcmp(name, "positions"))
	    dependence = "positions";
	else if (! strcmp(name, "faces"))
	    dependence = "faces";
	else
	{
	    attr = DXGetEnumeratedComponentAttribute(field, i, NULL, "dep");
	    if (attr)
		dependence = DXGetString((String)attr);
	}

	/*
	 * If the component is dependent on something, then do to the
	 * component whatever is appropriate for the component on which
	 * it is dependent.
	 */
	if (dependence)
	{
	    /*
	     * If its dependent on positions...
	     */
	    if (! strcmp(dependence, "positions"))
	    {
		/*
		 *  and there are invalidated positions, cull the component
		 */
		if (vpHandle)
		{
		    if (nNewPositions == 0)
		    {
			toDelete[nDelete++] = name;
			DXDelete((Object)child);
			child = NULL;
			i++;
			continue;
		    }
		    else
		    {
			newChild = CullArray(child, nPositions, 
						nNewPositions, vpHandle);
			if (! newChild)
			    goto error;

			DXDelete((Object)child);
			child = (Array)DXReference((Object)newChild);
		    }
		}
	    }
	    /*
	     * Else if its dependent on faces...
	     */
	    else if (! strcmp(dependence, "faces"))
	    {
		if (! fArray)
		{
		    DXSetError(ERROR_DATA_INVALID, 
			    "component dependent on nonexistent faces");
		    goto error;
		}
			    
		/*
		 *  and there is a validFaces array, cull the component.
		 */
		if (vfHandle)
		{
		    if (nNewFaces == 0)
		    {
			toDelete[nDelete++] = name;
			DXDelete((Object)child);
			child = NULL;
			i++;
			continue;
		    }
		    else
		    {
			newChild = CullArray(child, nFaces, 
					nNewFaces, vfHandle);
			
			if (! newChild)
			    goto error;

			DXDelete((Object)child);
			child = (Array)DXReference((Object)newChild);
		    }
		}
	    }
	}

	/*
	 * Now we consider components that ref.  If the component they ref
	 * has been culled, we need to renumber the references to reflect
	 * the culled referenced array.
	 */
	attr = DXGetEnumeratedComponentAttribute(field, i, NULL, "ref");
	if (attr)
	{
	    reference = DXGetString((String)attr);

	    /*
	     * If it references faces...
	     */
	    if (!strcmp(reference, "faces"))
	    {
		if (! fArray)
		{
		    DXSetError(ERROR_DATA_INVALID, 
			    "component refers to nonexistent faces");
		    goto error;
		}

		/*
		 * And faces have been culled, renumber the references.
		 */
		if (validFacesMap)
		{
		    newChild = _dxfReRef(child, validFacesMap);
		    if (! newChild)
			goto error;

		    DXDelete((Object)child);
		    child = (Array)DXReference((Object)newChild);
		}
	    }
	    /*
	     * Else if it references positions...
	     */
	    else if (!strcmp(reference, "positions"))
	    {
		/*
		 * And positions have been culled, renumber the references.
		 */
		if (validPositionsMap)
		{
		    newChild = _dxfReRef(child, validPositionsMap);
		    if (! newChild)
			goto error;

		    DXDelete((Object)child);
		    child = (Array)DXReference((Object)newChild);
		}
	    }
	}

	if (! DXSetComponentValue(field, name, (Object)child))
	    goto error;

	/*
	 * Remove local reference
	 */
	DXDelete((Object)child);
	child = NULL;
    
	i++;
    }

    for (nDelete--; nDelete >= 0; nDelete--)
	DXDeleteComponent(field, toDelete[nDelete]);

    field = DXEndField(field);

done:
    DXFree((Pointer)validFacesMap);
    DXFree((Pointer)validPositionsMap);
    DXFreeInvalidComponentHandle(vpHandle);
    DXFreeInvalidComponentHandle(vfHandle);

    return (Object)field;

error:
    DXDelete((Object)nFArray);
    DXDelete((Object)nLArray);
    DXDelete((Object)nEArray);
    DXDelete((Object)child);
    DXFreeInvalidComponentHandle(vpHandle);
    DXFreeInvalidComponentHandle(vfHandle);
    DXFree((Pointer)validFacesMap);
    DXFree((Pointer)validPositionsMap);

    return NULL;
}

static Object
C_Field_Standard(Field field)
{
    Array			pArray,   cArray;
    Array			child, 	  newChild;
    int				nPositions, nConnections, nInvP, nInvC;
    int				nNewPositions, nNewConnections;
    int				i;
    int				*validPositionsMap, *validConnectionsMap;
    Object			attr;
    char			*reference, *dependence;
    char			*name;
    char			*toDelete[32];
    int				nDelete = 0;
    InvalidComponentHandle 	vpHandle = NULL;
    InvalidComponentHandle	vcHandle = NULL;

    if (DXEmptyField(field))
	return (Object)field;

    vcHandle 	        = NULL;
    vpHandle 	        = NULL;
    validConnectionsMap = NULL;
    validPositionsMap   = NULL;
    child 	        = NULL;

    pArray = (Array)DXGetComponentValue(field, "positions");
    if (! pArray)
    {
	DXSetError(ERROR_MISSING_DATA, "positions component");
	goto error;
    }

    if (! DXGetArrayInfo(pArray, &nPositions, NULL, NULL, NULL, NULL))
	goto error;

    cArray = (Array)DXGetComponentValue(field, "connections");
    if (cArray)
    {
	if (! DXGetArrayInfo(cArray, &nConnections, NULL, NULL, NULL, NULL))
	    goto error;
    }
    else
	nConnections = 0;
    
    vpHandle = DXCreateInvalidComponentHandle((Object)field, NULL, "positions");
    if (! vpHandle)
	goto error;
    
    nInvP = DXGetInvalidCount(vpHandle);
    if (nInvP)
    {
	if (NULL == (validPositionsMap = 
		    MakeMap(vpHandle, nPositions, &nNewPositions)))
	    goto error;
	
	if (nNewPositions == 0)
	{
	    if (! (Object)DeleteFieldContents(field))
		goto error;
	    
	    goto done;
	}
    }
    else
    {
	DXFreeInvalidComponentHandle(vpHandle);
	vpHandle = NULL;
    }

    if (cArray)
    {
	vcHandle = DXCreateInvalidComponentHandle((Object)field, NULL,
						"connections");
	if (! vcHandle)
	    goto error;

	nInvC = DXGetInvalidCount(vcHandle);
    }
    else
	nInvC = 0;

    if (nInvC)
    {
	if (NULL == (validConnectionsMap = 
		    MakeMap(vcHandle, nConnections, &nNewConnections)))
	    goto error;
    }
    else
    {
	DXFreeInvalidComponentHandle(vcHandle);
	vcHandle = NULL;
    }
    
    if (nInvP == 0 && nInvC == 0)
    {
	DXDeleteComponent(field, "invalid connections");
	DXDeleteComponent(field, "invalid positions");
	return (Object)field;
    }
    
    i = 0;
    while (NULL != 
	(child = (Array)DXGetEnumeratedComponentValue(field, i, &name)))
    {
	if (! strncmp(name, "invalid ", 8))
	{
	    toDelete[nDelete++] = name;
	    i++;
	    continue;
	}

	attr = DXGetEnumeratedComponentAttribute(field, i, NULL, "der");
	if (attr && strcmp(name, "neighbors"))
	{
	    toDelete[nDelete++] = name;
	    i++;
	    continue;
	}
	
	DXReference((Object)child);

	dependence = NULL;

	if (! strcmp(name, "positions"))
	    dependence = "positions";
	else if (! strcmp(name, "connections"))
	    dependence = "connections";
	else
	{
	    attr = DXGetEnumeratedComponentAttribute(field, i, NULL, "dep");
	    if (attr)
		dependence = DXGetString((String)attr);
	}

	/*
	 * If the component is dependent on something, then do to the
	 * component whatever is appropriate for the component on which
	 * it is dependent.
	 */
	if (dependence)
	{
	    /*
	     * If its dependent on positions...
	     */
	    if (! strcmp(dependence, "positions"))
	    {
		/*
		 *  and there are invalidated positions, cull the component
		 */
		if (vpHandle)
		{
		    if (nNewPositions == 0)
		    {
			toDelete[nDelete++] = name;
			DXDelete((Object)child);
			child = NULL;
			i++;
			continue;
		    }
		    else
		    {
			newChild = CullArray(child, nPositions, 
						nNewPositions, vpHandle);
			if (! newChild)
			    goto error;

			DXDelete((Object)child);
			child = (Array)DXReference((Object)newChild);
		    }
		}
	    }
	    /*
	     * Else if its dependent on connections...
	     */
	    else if (! strcmp(dependence, "connections"))
	    {
		if (! cArray)
		{
		    DXSetError(ERROR_DATA_INVALID, 
			    "component dependent on nonexistent connections");
		    goto error;
		}
			    
		/*
		 *  and there is a validConnections array, cull the component.
		 */
		if (vcHandle)
		{
		    if (nNewConnections == 0)
		    {
			toDelete[nDelete++] = name;
			DXDelete((Object)child);
			child = NULL;
			i++;
			continue;
		    }
		    else
		    {
			newChild = CullArray(child, nConnections, 
					nNewConnections, vcHandle);
			
			if (! newChild)
			    goto error;

			DXDelete((Object)child);
			child = (Array)DXReference((Object)newChild);
		    }
		}
	    }
	}

	/*
	 * Now we consider components that ref.  If the component they ref
	 * has been culled, we need to renumber the references to reflect
	 * the culled referenced array.
	 */
	attr = DXGetEnumeratedComponentAttribute(field, i, NULL, "ref");
	if (attr)
	{
	    reference = DXGetString((String)attr);

	    /*
	     * If it references connections...
	     */
	    if (!strcmp(reference, "connections"))
	    {
		if (! cArray)
		{
		    DXSetError(ERROR_DATA_INVALID, 
			    "component refers to nonexistent connections");
		    goto error;
		}

		/*
		 * And connections have been culled, renumber the references.
		 */
		if (validConnectionsMap)
		{
		    newChild = _dxfReRef(child, validConnectionsMap);
		    if (! newChild)
			goto error;

		    DXDelete((Object)child);
		    child = (Array)DXReference((Object)newChild);
		}
	    }
	    /*
	     * Else if it references positions...
	     */
	    else if (!strcmp(reference, "positions"))
	    {
		/*
		 * And positions have been culled, renumber the references.
		 */
		if (validPositionsMap)
		{
		    newChild = _dxfReRef(child, validPositionsMap);
		    if (! newChild)
			goto error;

		    DXDelete((Object)child);
		    child = (Array)DXReference((Object)newChild);
		}
	    }
	}

	if (! DXSetComponentValue(field, name, (Object)child))
	    goto error;

	/*
	 * Remove local reference
	 */
	DXDelete((Object)child);
	child = NULL;
    
	i++;
    }

    for (nDelete--; nDelete >= 0; nDelete--)
	DXDeleteComponent(field, toDelete[nDelete]);

    field = DXEndField(field);

done:
    DXFree((Pointer)validConnectionsMap);
    DXFree((Pointer)validPositionsMap);
    DXFreeInvalidComponentHandle(vpHandle);
    DXFreeInvalidComponentHandle(vcHandle);

    return (Object)field;

error:
    DXDelete((Object)child);
    DXFreeInvalidComponentHandle(vpHandle);
    DXFreeInvalidComponentHandle(vcHandle);
    DXFree((Pointer)validConnectionsMap);
    DXFree((Pointer)validPositionsMap);

    return NULL;
}

static Array
CullArray(Array oldArray, int nOld, int nNew, InvalidComponentHandle handle)
{
    Array       newArray;
    Type        type;
    Category    cat;
    int         rank;
    int         shape[100];
    Pointer     buf = NULL;
    ArrayHandle aHandle = NULL;

    DXGetArrayInfo(oldArray, NULL, &type, &cat, &rank, shape);

    if (DXQueryConstantArray(oldArray, NULL, NULL))
    {
	newArray = (Array)DXNewConstantArrayV(nNew, 
	    DXGetConstantArrayData((Array)oldArray), type, cat, rank, shape);
    }
    else
    {
	int   itemSize, i, nKnt;
	ubyte *nPtr;

	itemSize = DXGetItemSize(oldArray);
	buf      = DXAllocate(itemSize);
	aHandle  = DXCreateArrayHandle(oldArray);
	newArray = DXNewArrayV(type, cat, rank, shape);
	if (! buf || ! aHandle || ! newArray)
	    goto error;
	
	if (! DXAddArrayData(newArray, 0, nNew, NULL))
	    goto error;

	nPtr = (ubyte *)DXGetArrayData(newArray);

	for (i = 0, nKnt = 0; i < nOld; i++)
	    if (! DXIsElementInvalidSequential(handle, i))
	    {
		memcpy(nPtr, DXGetArrayEntry(aHandle, i, buf), itemSize);
		nPtr += itemSize;
		if (++nKnt > nNew)
		{
		    DXSetError(ERROR_INTERNAL, "invalid count mismatch");
		    goto error;
		}
	    }
	
	DXFree(buf); buf = NULL;
	DXFreeArrayHandle(aHandle); aHandle = NULL;
    }

    return newArray;

error:
    DXFree(buf);
    DXFreeArrayHandle(aHandle);
    DXDelete((Object)newArray);
    return NULL;
}

static Array
_dxfReRef(Array oldArray, int *map)
{
    Array 	newArray;
    Type	type;
    Category	cat;
    int		rank, shape[32];
    int		*oldPtr, *newPtr;
    int		i, itemSize;
    int  	nElements;

    if (! DXGetArrayInfo(oldArray, &nElements, &type, &cat, &rank, shape))
	return NULL;

    if (type != TYPE_INT || cat != CATEGORY_REAL)
    {
	DXSetError(ERROR_DATA_INVALID, "invalid data array is not int/real");
	return NULL;
    }

    itemSize = 1;
    for(i = 0; i < rank; i++)
	itemSize *= shape[i];

    if (NULL == (oldPtr = (int *)DXGetArrayData(oldArray)))
	return NULL;

    if (NULL == (newArray = DXNewArrayV(type, cat, rank, shape)))
	return NULL;
    
    if (! DXAddArrayData(newArray, 0, nElements, NULL))
    {
	DXDelete((Object)newArray);
	return NULL;
    }
    
    if (NULL == (newPtr = (int *)DXGetArrayData(newArray)))
    {
	DXDelete((Object)newArray);
	return NULL;
    }
    
    if (NULL == (oldPtr = (int *)DXGetArrayData(oldArray)))
    {
	DXDelete((Object)newArray);
	return NULL;
    }

    for (i = 0; i < nElements*itemSize; i++)
    {
	if (*oldPtr != -1)
	    *newPtr = map[*oldPtr];
	else
	    *newPtr = -1;

	oldPtr++;
	newPtr++;
    }

    return newArray;
}

static int *
MakeMap(InvalidComponentHandle handle, int nIn, int *nNew)
{
    int *map;
    int i, j;

    map = (int *)DXAllocate(nIn * sizeof(int));
    if (! map)
	return NULL;

    for (i = 0, j = 0; i < nIn; i++)
	if (DXIsElementInvalidSequential(handle, i))
	    map[i] = -1;
	else
	    map[i] = j++;
    
    *nNew = j;

    return map;
}

/*
 * Any connections referencing invalidated positions are invalidated.
 */
static Object
IC_Field(Field field)
{
    if (DXGetComponentValue(field, "polylines"))
    {
	return IC_Field_PE(field);
    }
    else if (DXGetComponentValue(field, "faces"))
    {
	return IC_Field_FLE(field);
    }
    else
    {
	return IC_Field_Standard(field);
    }
}

/*
 * Any polylines referencing invalidated positions are invalidated.
 */
static Object
IC_Field_PE(Field field)
{
    Array	           plArray, eArray;
    InvalidComponentHandle vp = NULL, vplIn = NULL, vplOut = NULL;
    int		           i, j;
    int			   nPl, nE;
    int			   *polylines = NULL, *edges = NULL;

    if (DXEmptyField(field))
	return (Object)field;
    
    /* 
     * If there are no polylines, then there are none to invalidate
     */
    if (NULL == (plArray = (Array)DXGetComponentValue(field, "polylines")))
	return (Object)field;

    if (! DXGetArrayInfo(plArray, &nPl, NULL, NULL, NULL, NULL))
	goto error;

    
    if (nPl == 0)
	return (Object)field;
    
    eArray = (Array)DXGetComponentValue(field, "edges");
    if (! eArray)
    {    
	DXSetError(ERROR_DATA_INVALID, "polylines with no edges");
	goto error;
    }

    if (! DXGetArrayInfo(eArray, &nE, NULL, NULL, NULL, NULL))
	goto error;
    
    vplIn  = NULL;
    vplOut = NULL;

    /*
     * Get the valid positions array.  If there isn't one, then we
     * assume all positions are valid and there's nothing to do.
     */
    vp = DXCreateInvalidComponentHandle((Object)field, NULL,
						"positions");
    if (! vp)
	goto error;

    if (DXGetInvalidCount(vp) == 0)
    {
	DXFreeInvalidComponentHandle(vp);
	return (Object)field;
    }
    
    vplIn  = DXCreateInvalidComponentHandle((Object)field, NULL,
						"polylines");
    if (! vplIn)
	goto error;

    if (DXGetInvalidCount(vplIn) == 0)
    {
	DXFreeInvalidComponentHandle(vplIn);
	vplIn = NULL;
    }

    polylines = (int *)DXGetArrayData(plArray);
    edges = (int *)DXGetArrayData(eArray);

    /* 
     * For each connections element, consider each referent.  If its
     * invalid, invalidate the connections element.
     */
    for (i = 0; i < nPl; i++)
    {
	int valid = !vplIn || !DXIsElementInvalidSequential(vplIn, i);

	if (valid)
	{
	    int start = polylines[i];
	    int end   = (i == nPl-1) ? nE - 1 : polylines[i+1] - 1;

	    for (j = start; j <= end  && valid; j++)
		valid = ! DXIsElementInvalid(vp, edges[start+j]);
	}

	if (! valid)
	{
	    /*
	     * If we haven't invalidated anything yet,
	     * we need to set up the invalid connections 
	     * array, creating one and initializing it
	     * if necessary.
	     */
	    if (! vplOut)
	    {
		vplOut = DXCreateInvalidComponentHandle((Object)field, NULL,
						    "polylines");
		
		if (! vplOut)
		    goto error;
	    }

	    if (! DXSetElementInvalid(vplOut, i))
		goto error;
	}
    }

    if (vplOut)
    {
	if (! DXSaveInvalidComponent(field, vplOut))
	    goto error;
    }

    DXFreeInvalidComponentHandle(vp);
    DXFreeInvalidComponentHandle(vplIn);
    DXFreeInvalidComponentHandle(vplOut);

    return (Object)field;

error:

    DXFreeInvalidComponentHandle(vp);
    DXFreeInvalidComponentHandle(vplIn);
    DXFreeInvalidComponentHandle(vplOut);

    return NULL;
}

/*
 * Any faces referencing invalidated positions are invalidated.
 */
static Object
IC_Field_FLE(Field field)
{
    Array	           fArray, lArray, eArray;
    InvalidComponentHandle vp = NULL, vfIn = NULL, vfOut = NULL;
    int		           face;
    int			   nF, nL, nE;
    int			   *faces = NULL, *loops = NULL, *edges = NULL;

    if (DXEmptyField(field))
	return (Object)field;
    
    /* 
     * If there are no faces, then there are none to invalidate
     */
    if (NULL == (fArray = (Array)DXGetComponentValue(field, "faces")))
	return (Object)field;

    if (! DXGetArrayInfo(fArray, &nF, NULL, NULL, NULL, NULL))
	goto error;

    if (nF == 0)
	return (Object)field;
    
    lArray = (Array)DXGetComponentValue(field, "loops");
    if (! lArray)
    {    
	DXSetError(ERROR_DATA_INVALID, "faces with no loops");
	goto error;
    }

    if (! DXGetArrayInfo(lArray, &nL, NULL, NULL, NULL, NULL))
	goto error;
    
    eArray = (Array)DXGetComponentValue(field, "edges");
    if (! eArray)
    {    
	DXSetError(ERROR_DATA_INVALID, "faces with no edges");
	goto error;
    }

    if (! DXGetArrayInfo(eArray, &nE, NULL, NULL, NULL, NULL))
	goto error;
    
    vfIn  = NULL;
    vfOut = NULL;

    /*
     * Get the valid positions array.  If there isn't one, then we
     * assume all positions are valid and there's nothing to do.
     */
    vp = DXCreateInvalidComponentHandle((Object)field, NULL,
						"positions");
    if (! vp)
	goto error;

    if (DXGetInvalidCount(vp) == 0)
    {
	DXFreeInvalidComponentHandle(vp);
	return (Object)field;
    }
    
    vfIn  = DXCreateInvalidComponentHandle((Object)field, NULL,
						"faces");
    if (! vfIn)
	goto error;

    if (DXGetInvalidCount(vfIn) == 0)
    {
	DXFreeInvalidComponentHandle(vfIn);
	vfIn = NULL;
    }

    faces = (int *)DXGetArrayData(fArray);
    loops = (int *)DXGetArrayData(lArray);
    edges = (int *)DXGetArrayData(eArray);

    /* 
     * For each connections element, consider each referent.  If its
     * invalid, invalidate the connections element.
     */
    for (face = 0; face < nF; face++)
    {
	int valid = !vfIn || !DXIsElementInvalidSequential(vfIn, face);

	if (valid)
	{
	    int lstart = faces[face];
	    int lend   = (face == nF-1) ? nL : faces[face+1];
	    int loop;

	    for (loop = lstart; loop < lend  && valid; loop++)
	    {
		int estart = loops[loop];
		int eend   = (loop == nL-1) ? nE : loops[loop+1];
		int edge;

		for (edge = estart; edge < eend  && valid; edge++)
		    valid = ! DXIsElementInvalid(vp, edges[edge]);
	    }
	}

	if (! valid)
	{
	    /*
	     * If we haven't invalidated anything yet,
	     * we need to set up the invalid connections 
	     * array, creating one and initializing it
	     * if necessary.
	     */
	    if (! vfOut)
	    {
		vfOut = DXCreateInvalidComponentHandle((Object)field, NULL,
						    "faces");
		
		if (! vfOut)
		    goto error;
	    }

	    if (! DXSetElementInvalid(vfOut, face))
		goto error;
	}
    }

    if (vfOut)
    {
	if (! DXSaveInvalidComponent(field, vfOut))
	    goto error;
    }

    DXFreeInvalidComponentHandle(vp);
    DXFreeInvalidComponentHandle(vfIn);
    DXFreeInvalidComponentHandle(vfOut);

    return (Object)field;

error:

    DXFreeInvalidComponentHandle(vp);
    DXFreeInvalidComponentHandle(vfIn);
    DXFreeInvalidComponentHandle(vfOut);

    return NULL;
}

/*
 * Any connections referencing invalidated positions are invalidated.
 */
static Object
IC_Field_Standard(Field field)
{
    Array	           cArray;
    InvalidComponentHandle vp = NULL, vcIn = NULL, vcOut = NULL;
    int		           ptsPerPrim;
    int		           *cPtr = NULL;
    int		           i, j, nCons;
    ArrayHandle		   cHandle = NULL;
    Pointer		   cBuf = NULL;

    if (DXEmptyField(field))
	return (Object)field;
    
    /* 
     * If there are no connections, then there are none to invalidate
     */
    if (NULL == (cArray = (Array)DXGetComponentValue(field, "connections")))
	return (Object)field;

    vcIn  = NULL;
    vcOut = NULL;

    /*
     * Get the valid positions array.  If there isn't one, then we
     * assume all positions are valid and there's nothing to do.
     */
    vp = DXCreateInvalidComponentHandle((Object)field, NULL,
						"positions");
    if (! vp)
	goto error;

    if (DXGetInvalidCount(vp) == 0)
    {
	DXFreeInvalidComponentHandle(vp);
	return (Object)field;
    }
    
    vcIn  = DXCreateInvalidComponentHandle((Object)field, NULL,
						"connections");
    if (! vcIn)
	goto error;

    if (DXGetInvalidCount(vcIn) == 0)
    {
	DXFreeInvalidComponentHandle(vcIn);
	vcIn = NULL;
    }

    cBuf = DXAllocate(DXGetItemSize(cArray));
    if (! cBuf)
	goto error;

    cHandle = DXCreateArrayHandle(cArray);
    if (! cHandle)
	goto error;

    if (! DXGetArrayInfo(cArray, &nCons, NULL, NULL, NULL, &ptsPerPrim))
	goto error;

    /* 
     * For each connections element, consider each referent.  If its
     * invalid, invalidate the connections element.
     */
    for (i = 0; i < nCons; i++)
    {
	int valid = !vcIn || !DXIsElementInvalidSequential(vcIn, i);

	if (valid)
	{
	    cPtr = (int *)DXGetArrayEntry(cHandle, i, (Pointer)cBuf);
	
	    for (j = 0; j < ptsPerPrim && valid; j++)
		valid = ! DXIsElementInvalid(vp, cPtr[j]);
	}

	if (! valid)
	{
	    /*
	     * If we haven't invalidated anything yet,
	     * we need to set up the invalid connections 
	     * array, creating one and initializing it
	     * if necessary.
	     */
	    if (! vcOut)
	    {
		vcOut = DXCreateInvalidComponentHandle((Object)field, NULL,
						    "connections");
		
		if (! vcOut)
		    goto error;
	    }

	    if (! DXSetElementInvalid(vcOut, i))
		goto error;
	}
    }

    if (vcOut)
    {
	if (! DXSaveInvalidComponent(field, vcOut))
	    goto error;
    }

    DXFree(cBuf);
    DXFreeArrayHandle(cHandle);
    DXFreeInvalidComponentHandle(vp);
    DXFreeInvalidComponentHandle(vcIn);
    DXFreeInvalidComponentHandle(vcOut);

    return (Object)field;

error:

    DXFree(cBuf);
    DXFreeArrayHandle(cHandle);
    DXFreeInvalidComponentHandle(vp);
    DXFreeInvalidComponentHandle(vcIn);
    DXFreeInvalidComponentHandle(vcOut);

    return NULL;
}

/*
 * Any connections referencing invalidated positions are invalidated.
 */
static Object
IUP_Field(Field field)
{
    if (DXGetComponentValue(field, "polylines"))
    {
	return IUP_Field_PE(field);
    }
    else if (DXGetComponentValue(field, "faces"))
    {
	return IUP_Field_FLE(field);
    }
    else
    {
	return IUP_Field_Standard(field);
    }
}

static Object
IUP_Field_Standard(Field field)
{
    Array       	   pArray,  cArray;
    InvalidComponentHandle vpHandle = NULL, vcHandle = NULL;
    int         	   *refs = NULL, *cons;
    int	        	   i, j, ptsPerPrim, nCons, nPts;
    int         	   unReffedPositions;
    ArrayHandle 	   cHandle = NULL;
    Pointer     	   cBuf = NULL;

    if (DXEmptyField(field))
	return (Object)field;

    if (NULL == (pArray = (Array)DXGetComponentValue(field, "positions")))
        return (Object)field;

    if (! DXGetArrayInfo(pArray, &nPts, NULL, NULL, NULL, NULL))
	goto error;
    
    /*
     * Allow for no connections component... If none are present, all
     * positions are invalidated
     */
    if (NULL == (cArray = (Array)DXGetComponentValue(field, "connections")))
    {
	Array invPos;
	byte valid;

	valid =  DATA_INVALID;

	invPos = (Array)DXNewConstantArray(nPts, (Pointer)&valid,
					    TYPE_UBYTE, CATEGORY_REAL, 0);
	if (! invPos)
	    goto error;
	
	if (! DXSetComponentValue(field, "invalid positions", (Object)invPos))
	    goto error;
	
	if (! DXSetComponentAttribute(field, "invalid positions", "dep",
				(Object)DXNewString("positions")))
	    goto error;
	
	goto done;
    }

    /*
     * Get the valid connections information
     */
    vcHandle = DXCreateInvalidComponentHandle((Object)field, NULL,
						"connections");
    if (! vcHandle)
	goto error;
    
    /*
     * If no connections are invalidated, we have regular positions and
     * connections and the counts match, all are valid - take the fast 
     * path.
     */
    if (DXGetInvalidCount(vcHandle) == 0)
    {
	int nRef = GridSize(cArray);
	if (nRef == ERROR_OCCURRED)
	    goto error;
	else if (nRef != HOLES_FOUND && nRef == nPts)
	    goto done;
    }

    if (! DXGetArrayInfo(cArray, &nCons, NULL, NULL, NULL, &ptsPerPrim))
	return NULL;

    cHandle = DXCreateArrayHandle(cArray);
    if (! cHandle)
	goto error;
    
    cBuf = DXAllocate(DXGetItemSize(cArray));
    if (! cBuf)
	goto error;

    if (NULL == (refs = (int *)DXAllocateZero(nPts*sizeof(int))))
	return NULL;
    
    /*
     * Assume that we have already invalidated the connections.
     * If there's no valid connections input info, assume all connection 
     * elements are valid.  Otherwise, test on an element by element basis
     */
    for (i = 0; i < nCons; i++)
    {
	if (! DXIsElementInvalidSequential(vcHandle, i))
	{
	    cons = (int *)DXGetArrayEntry(cHandle, i, cBuf);

	    for (j = 0; j < ptsPerPrim; j++)
		refs[*cons++]++;
	}
    }
    
    /*
     * See if we have any unreffed positions
     */
    for (i = 0; i < nPts; i++)
	if (! refs[i])
	    break;
    
    if (i != nPts)
	unReffedPositions = 1;
    else
	unReffedPositions = 0;
    
    vpHandle = DXCreateInvalidComponentHandle((Object)field, NULL,
						"positions");
    if (! vpHandle)
	goto error;

    /*
     * Invalidate unreferenced positions
     */
    if (unReffedPositions)
    {
	for (i = 0; i < nPts; i++)
	    if (! refs[i])
	        DXSetElementInvalid(vpHandle, i);
    }

    if (! DXSaveInvalidComponent(field, vpHandle))
	goto error;
    
done:
    DXFreeInvalidComponentHandle(vcHandle);
    DXFreeInvalidComponentHandle(vpHandle);
    DXFreeArrayHandle(cHandle);
    DXFree(cBuf);
    DXFree((Pointer)refs);

    return (Object)field;

error:
    DXFreeInvalidComponentHandle(vcHandle);
    DXFreeInvalidComponentHandle(vpHandle);
    DXFreeArrayHandle(cHandle);
    DXFree(cBuf);
    DXFree((Pointer)refs);
    return NULL;
}

static Object
IUP_Field_PE(Field field)
{
    Array       	   pArray,  plArray, eArray;
    InvalidComponentHandle vpHandle = NULL, vplHandle = NULL;
    ubyte         	   *refs = NULL;
    int	        	   i, j, nPls, nEs, nPts;
    int         	   unReffedPositions;
    ArrayHandle 	   plHandle = NULL, eHandle = NULL;

    if (DXEmptyField(field))
	return (Object)field;

    if (NULL == (pArray = (Array)DXGetComponentValue(field, "positions")))
        return (Object)field;

    if (! DXGetArrayInfo(pArray, &nPts, NULL, NULL, NULL, NULL))
	goto error;
    
    /*
     * We wouldn't get here unless we found a polylines component
     */
    plArray = (Array)DXGetComponentValue(field, "polylines");

    eArray = (Array)DXGetComponentValue(field, "edges");
    if (! eArray)
    {
	DXSetError(ERROR_DATA_INVALID, "polylines with no edges component");
	goto error;
    }

    plHandle = DXCreateArrayHandle(plArray);
    eHandle  = DXCreateArrayHandle(eArray);
    if (! plHandle || ! eHandle)
	goto error;

    DXGetArrayInfo(plArray, &nPls, NULL, NULL, NULL, NULL);
    DXGetArrayInfo(eArray, &nEs, NULL, NULL, NULL, NULL);
    DXGetArrayInfo(pArray, &nPts, NULL, NULL, NULL, NULL);

    refs = (ubyte *)DXAllocateZero(nPts*sizeof(ubyte));
    if (! refs)
	goto error;

    /*
     * Get the valid connections information
     */
    vplHandle = DXCreateInvalidComponentHandle((Object)field, NULL,
						"polylines");
    if (! vplHandle)
	goto error;
    
    /*
     * Assume that we have already invalidated the connections.
     * If there's no valid connections input info, assume all connection 
     * elements are valid.  Otherwise, test on an element by element basis
     */
    for (i = 0; i < nPls; i++)
    {
	if (! DXIsElementInvalidSequential(vplHandle, i))
	{
	    int startBuf, *start;
	    int endBuf, *end;
	    
	    start = (int *)DXGetArrayEntry(plHandle, i, (Pointer)&startBuf);
	    if (i == (nPls-1))
		end = &nEs;
	    else
	        end = (int *)DXGetArrayEntry(plHandle, i+1, (Pointer)&endBuf);

	    for (j = *start; j < *end; j++)
	    {
		int ptrBuf, *ptr;
		
		ptr = (int *)DXGetArrayEntry(eHandle, j, (Pointer)&ptrBuf);
		refs[*ptr] = 1;
	    }
	}
    }
    
    /*
     * See if we have any unreffed positions
     */
    for (i = 0; i < nPts; i++)
	if (! refs[i])
	    break;
    
    if (i != nPts)
	unReffedPositions = 1;
    else
	unReffedPositions = 0;
    
    vpHandle = DXCreateInvalidComponentHandle((Object)field, NULL,
						"positions");
    if (! vpHandle)
	goto error;

    /*
     * Invalidate unreferenced positions
     */
    if (unReffedPositions)
    {
	for (i = 0; i < nPts; i++)
	    if (! refs[i])
	        DXSetElementInvalid(vpHandle, i);
    }

    if (! DXSaveInvalidComponent(field, vpHandle))
	goto error;
    
/* done: */
    DXFreeInvalidComponentHandle(vpHandle);
    DXFreeInvalidComponentHandle(vplHandle);
    DXFreeArrayHandle(plHandle);
    DXFreeArrayHandle(eHandle);
    DXFree((Pointer)refs);

    return (Object)field;

error:
    DXFreeInvalidComponentHandle(vpHandle);
    DXFreeInvalidComponentHandle(vplHandle);
    DXFreeArrayHandle(plHandle);
    DXFreeArrayHandle(eHandle);
    DXFree((Pointer)refs);

    return NULL;
}

static Object
IUP_Field_FLE(Field field)
{
    Array       	   pArray,  fArray, lArray, eArray;
    InvalidComponentHandle vpHandle = NULL, vfHandle = NULL;
    ubyte         	   *refs = NULL;
    int	        	   f, e, i, nFs, nLs, nEs, nPts;
    int         	   unReffedPositions;
    ArrayHandle 	   fHandle = NULL, lHandle = NULL, eHandle = NULL;

    if (DXEmptyField(field))
	return (Object)field;

    if (NULL == (pArray = (Array)DXGetComponentValue(field, "positions")))
        return (Object)field;

    if (! DXGetArrayInfo(pArray, &nPts, NULL, NULL, NULL, NULL))
	goto error;
    
    /*
     * We wouldn't get here unless we found a polylines component
     */
    fArray = (Array)DXGetComponentValue(field, "faces");

    lArray = (Array)DXGetComponentValue(field, "loops");
    eArray = (Array)DXGetComponentValue(field, "edges");
    if (! eArray || ! lArray)
    {
	DXSetError(ERROR_DATA_INVALID, 
		"faces with no edges or loops component");
	goto error;
    }

    fHandle = DXCreateArrayHandle(fArray);
    lHandle = DXCreateArrayHandle(lArray);
    eHandle  = DXCreateArrayHandle(eArray);
    if (! fHandle || ! lHandle || ! eHandle)
	goto error;

    DXGetArrayInfo(fArray, &nFs, NULL, NULL, NULL, NULL);
    DXGetArrayInfo(lArray, &nLs, NULL, NULL, NULL, NULL);
    DXGetArrayInfo(eArray, &nEs, NULL, NULL, NULL, NULL);
    DXGetArrayInfo(pArray, &nPts, NULL, NULL, NULL, NULL);

    refs = (ubyte *)DXAllocateZero(nPts*sizeof(ubyte));
    if (! refs)
	goto error;

    /*
     * Get the valid connections information
     */
    vfHandle = DXCreateInvalidComponentHandle((Object)field, NULL,
						"faces");
    if (! vfHandle)
	goto error;
    
    /*
     * Assume that we have already invalidated the connections.
     * If there's no valid connections input info, assume all connection 
     * elements are valid.  Otherwise, test on an element by element basis
     */
    for (f = 0; f < nFs; f++)
    {
	int lStartBuf, *lStart;
	int lEndBuf, *lEnd;
	int l;

	if (DXIsElementInvalidSequential(vfHandle, f))
	    continue;

	lStart = (int *)DXGetArrayEntry(fHandle, f, (Pointer)&lStartBuf);
	if (f == (nFs-1))
	    lEnd = &nLs;
	else
	    lEnd = (int *)DXGetArrayEntry(fHandle, f+1, (Pointer)&lEndBuf);
	
	for (l = *lStart; l < *lEnd;  l++)
	{
	    int eStartBuf, *eStart;
	    int eEndBuf, *eEnd;
	
	    eStart = (int *)DXGetArrayEntry(lHandle, l, (Pointer)&eStartBuf);
	    if (l == (nLs-1))
		eEnd = &nEs;
	    else
		eEnd = (int *)DXGetArrayEntry(lHandle, l+1, (Pointer)&eEndBuf);

	    for (e = *eStart; e < *eEnd; e++)
	    {
		int ptrBuf, *ptr;
		
		ptr = (int *)DXGetArrayEntry(eHandle, e, (Pointer)&ptrBuf);
		refs[*ptr] = 1;
	    }
	}
    }
    
    /*
     * See if we have any unreffed positions
     */
    for (i = 0; i < nPts; i++)
	if (! refs[i])
	    break;
    
    if (i != nPts)
	unReffedPositions = 1;
    else
	unReffedPositions = 0;
    
    vpHandle = DXCreateInvalidComponentHandle((Object)field, NULL,
						"positions");
    if (! vpHandle)
	goto error;

    /*
     * Invalidate unreferenced positions
     */
    if (unReffedPositions)
    {
	for (i = 0; i < nPts; i++)
	    if (! refs[i])
	        DXSetElementInvalid(vpHandle, i);
    }

    if (! DXSaveInvalidComponent(field, vpHandle))
	goto error;
    
/* done: */
    DXFreeInvalidComponentHandle(vpHandle);
    DXFreeInvalidComponentHandle(vfHandle);
    DXFreeArrayHandle(fHandle);
    DXFreeArrayHandle(lHandle);
    DXFreeArrayHandle(eHandle);
    DXFree((Pointer)refs);

    return (Object)field;

error:
    DXFreeInvalidComponentHandle(vpHandle);
    DXFreeInvalidComponentHandle(vfHandle);
    DXFreeArrayHandle(fHandle);
    DXFreeArrayHandle(lHandle);
    DXFreeArrayHandle(eHandle);
    DXFree((Pointer)refs);

    return NULL;
}

#define TYPE      int
#define LT(a,b)   ((*(a))<(*(b)))
#define GT(a,b)   ((*(a))>(*(b)))
#define QUICKSORT refsort

#include "qsort.c"

static Error SetMark(InvalidComponentHandle, int);
static Error RemoveMark(InvalidComponentHandle, int);
static int   RemoveContents(InvalidComponentHandle);
static Error IsElementMarked(InvalidComponentHandle, int);
static Error MakeSortList(InvalidComponentHandle);
static int   GetNextMarked(InvalidComponentHandle);

InvalidComponentHandle
DXCreateInvalidComponentHandle(Object o, Array iarray, char *name)
{
    Field field = NULL;
    Array array;
    int   nItems, nInv;
    InvalidComponentHandle handle = NULL;

    if (o)
    {
	if (DXGetObjectClass(o) == CLASS_FIELD)
	{
	    char sbuf[128];

	    field = (Field)o;

	    if (! name)
	    {
		DXSetError(ERROR_BAD_PARAMETER, "null invalid component name");
		goto error;
	    }

	    if (name == NULL)
	    {
		DXSetError(ERROR_BAD_PARAMETER, "component name required");
		goto error;
	    }

	    array = (Array)DXGetComponentValue(field, name);
	    if (! array)
	    {
		DXSetError(ERROR_BAD_PARAMETER, 
		    "cannot create invalid component handle for component %s",
		    name);
		goto error;
	    }

	    sprintf(sbuf, "invalid %s", name);

	    if (! iarray)
		iarray = (Array)DXGetComponentValue(field, sbuf);

	}
	else if (DXGetObjectClass(o) == CLASS_ARRAY)
	{
	    array = (Array)o;
	}
	else
	{
	    DXSetError(ERROR_BAD_CLASS, "unknown object class");
	    goto error;
	}

	DXGetArrayInfo(array, &nItems, NULL, NULL, NULL, NULL);
    }
    else
    {
	nItems = -1;
    }


    handle = (InvalidComponentHandle)
			DXAllocateZero(sizeof(struct invalidComponentHandle));
    if (! handle)
	goto error;

    handle->nItems = nItems;
    handle->sense  = IC_MARKS_INDICATE_INVALID;

    if (name)
    {
	handle->iName = (char *)DXAllocate(strlen(name) + 1);
	if (! handle->iName)
	    goto error;

	sprintf(handle->iName, "%s", name);
    }
    else
	handle->iName = NULL;

    /*
     * If there's any pre-existing invalid data, initialize with it.
     */
    if (iarray)
    {
	Type type;
	Category cat;
	int rank, shape[100];
	int i;
	ubyte *dPtr;

	DXGetArrayInfo(iarray, &nInv, &type, &cat, &rank, shape);

	/*
	 * If there was no original array, then we can get a 
	 * dependency size here.
	 */
	nItems = nInv;

	/*
	 * If so, is is dependent or referential?
	 */
	if (DXGetAttribute((Object)iarray, "dep"))
	{
	    if ( nInv != nItems      			  ||
		(type != TYPE_UBYTE && type != TYPE_BYTE) ||
		 cat  != CATEGORY_REAL 			  ||
		 rank != 0)
	    {
		DXSetError(ERROR_DATA_INVALID, 
		    "dependent invalid data component must be scalar bytes");
		goto error;
	    }

	    /*
	     * If its a constant array, then its either all valid or all
	     * invalid.
	     */
	    if (DXGetArrayClass(iarray) == CLASS_CONSTANTARRAY)
	    {
		ubyte v = *(ubyte *)DXGetConstantArrayData(iarray);

		handle->array = NULL;

		if (v == DATA_INVALID)
		{
		    handle->nMarkedItems = nInv;
		    handle->type = IC_ALL_MARKED;
		}
		else if (v == DATA_VALID)
		{
		    handle->nMarkedItems = 0;
		    handle->type = IC_ALL_UNMARKED;
		}
		else
		{
		    DXSetError(ERROR_DATA_INVALID, 
			    "invalid component array");
		    goto error;
		}

	    }
	    else
	    {
		handle->type = IC_DEP_ARRAY;

		handle->array  = (Array)DXReference((Object)iarray);
		handle->data   = (byte *)DXGetArrayData(iarray);

		handle->nMarkedItems = 0;
		dPtr = (ubyte *)(handle->data);
		for (i = 0; i < nInv; i++)
		    if (*dPtr++ == DATA_INVALID)
			 handle->nMarkedItems ++;
	    }
	}
	else if (DXGetAttribute((Object)iarray, "ref"))
	{
	    int *iPtr, i;

	    if ((type != TYPE_INT && type != TYPE_UINT) ||
		cat  != CATEGORY_REAL 			||
		rank != 0)
	    {
		DXSetError(ERROR_DATA_INVALID, 
		    "referential invalid data component is wrong type, %s",
		    "category, or not scalar");
		goto error;
	    }

	    DXGetArrayInfo(iarray, &nInv, NULL, NULL, NULL, NULL);

	    if (nInv == 0)
	    {
		handle->type = IC_ALL_UNMARKED;
		handle->nMarkedItems = 0;
	    }
	    else
	    {
		/*  Accessing the elements of a large IC_SORTED_LIST via   */
		/*    binary search is hideously slow.  Use a HASH or DEP  */
		/*    internally.                                          */
		if (handle->nItems != -1 && 
		    nInv >= DEP_MEM_THRESHOLD(handle->nItems))
		{
		    ubyte *data = (ubyte *)DXAllocate(
			                     handle->nItems*sizeof(ubyte));
		    if (!data)
			goto error;
		    memset(data, IC_ELEMENT_UNMARKED, handle->nItems);

		    handle->nMarkedItems = 0;
		    iPtr = (int *)DXGetArrayData(iarray);
		    for (i = 0; i < nInv; i++, iPtr++) 
		    {
			if (data[*iPtr] == IC_ELEMENT_UNMARKED)
			    handle->nMarkedItems++;
			data[*iPtr] = IC_ELEMENT_MARKED;
		    }

		    handle->type         = IC_DEP_ARRAY;
		    handle->data         = data;
		}
		else 
		{
		    long      li;
		    HashTable hash = DXCreateHash(sizeof(long), NULL, NULL);
		    if (!hash)
			goto error;

		    iPtr = (int *)DXGetArrayData(iarray);
		    for (i = 0; i < nInv; i++, iPtr++) {
			li = *iPtr;
			if (! DXInsertHashElement(hash, (Element)&li))
			    goto error;
		    }

		    handle->nMarkedItems = 0;
		    DXInitGetNextHashElement(hash);
		    while (DXGetNextHashElement(hash))
		        handle->nMarkedItems++;

		    handle->type         = IC_HASH;
		    handle->hash         = hash;
		}
	    }
	}
	else
	{
	    DXSetError(ERROR_DATA_INVALID,
		"invalid component has neither dep nor ref attribute");
	    goto error;
	}
    }
    else
    {
	/*
	 * Otherwise, the everything so far is assumed to be valid.
	 */
        handle->type = IC_ALL_UNMARKED;
    }

    return handle;

error:
    DXFreeInvalidComponentHandle(handle);
    return NULL;
}

Error
DXFreeInvalidComponentHandle(InvalidComponentHandle handle)
{
    if (handle)
    {
	if (handle->iName)
	    DXFree((Pointer)handle->iName);
	if (handle->array)
	    DXDelete((Object)handle->array);
	if (handle->hash)
	    DXDestroyHash(handle->hash);
	if (handle->data && !handle->array)
	    DXFree((Pointer)handle->data);
	if (handle->seglist)
	    DXDeleteSegList(handle->seglist);
	DXFree((Pointer)handle->sortList);

	DXFree((Pointer)handle);
    }

    return OK;
}

Error
DXSaveInvalidComponent(Field field, InvalidComponentHandle handle)
{
    Array array;
    char  sbuf[128];

    if (! handle->iName)
    {
	DXSetError(ERROR_INTERNAL, "unknown invalid component name");
	return ERROR;
    }

    sprintf(sbuf, "invalid %s", handle->iName);

    if (DXGetComponentValue(field, sbuf))
	DXDeleteComponent(field, sbuf);

    if (handle->type  == IC_ALL_UNMARKED &&
	handle->sense == IC_MARKS_INDICATE_INVALID)
    {
	return OK;
    }

    if (handle->type  == IC_ALL_MARKED &&
	handle->sense == IC_MARKS_INDICATE_VALID)
    {
	return OK;
    }

    array = DXGetInvalidComponentArray(handle);
    if (! array)
	return ERROR;
    
    if (! DXSetComponentValue(field, sbuf, (Object)array))
	return ERROR;

    return OK;
}

#define DEP_ARRAY 1
#define REF_ARRAY 2

Array
DXGetInvalidComponentArray(InvalidComponentHandle handle)
{
    Array  array = NULL;
    Object attr;
    int    nInvalid;
    int    type = 0;
    long   li;

    if (! handle)
	goto error;

    if (handle->array)
	return handle->array;

    if (handle->sense == IC_MARKS_INDICATE_VALID)
	nInvalid = handle->nItems - handle->nMarkedItems;
    else
	nInvalid = handle->nMarkedItems;
    
    if ((nInvalid == handle->nItems)				             ||
	((handle->type == IC_ALL_MARKED &&
		handle->sense == IC_MARKS_INDICATE_INVALID)) 		     ||
	((handle->type == IC_ALL_UNMARKED &&
		handle->sense == IC_MARKS_INDICATE_VALID)))
    {
	ubyte value = DATA_INVALID;

	type = DEP_ARRAY;

	array = (Array)DXNewConstantArray(handle->nItems, &value,	
					    TYPE_UBYTE, CATEGORY_REAL, 0);
	if (! array)
	    goto error;
    }
    else if (nInvalid == 0						     ||
	((handle->type == IC_ALL_MARKED &&
		handle->sense == IC_MARKS_INDICATE_VALID)) 		     ||
	((handle->type == IC_ALL_UNMARKED &&
		handle->sense == IC_MARKS_INDICATE_INVALID)))
    {
	ubyte value = DATA_VALID;

	type = DEP_ARRAY;

	array = (Array)DXNewConstantArray(handle->nItems, &value,	
					    TYPE_UBYTE, CATEGORY_REAL, 0);
	if (! array)
	    goto error;
    }
    else if (handle->type == IC_DEP_ARRAY)
    {
	
	/*
	 * If there are enough actually invalid, output a dep array.
	 * Otherwise, create a sort list.  
	 */
	if (handle->nItems != -1 && 
	    nInvalid >= DEP_STORAGE_THRESHOLD(handle->nItems))
	{
	    type = DEP_ARRAY;

	    array = DXNewArray(TYPE_UBYTE, CATEGORY_REAL, 0);
	    if (! array)
		goto error;

	    /*
	     * We may have to invert the byte vector to get the right sense.
	     */
	    if (handle->sense == IC_MARKS_INDICATE_VALID)
	    {
		int i;
		ubyte *ptr;

		ptr = (ubyte *)handle->data;
		for (i = 0; i < handle->nItems; i++, ptr++)
		    if (*ptr == IC_ELEMENT_MARKED)
			*ptr = IC_ELEMENT_UNMARKED;
		    else
			*ptr = IC_ELEMENT_MARKED;
		
		handle->sense = IC_MARKS_INDICATE_INVALID;
	    }

	    if (! DXAddArrayData(array, 0, handle->nItems, handle->data))
		goto error;
	}
	else
	{
	    ubyte target;
	    int i;
	    ubyte *bptr;
	    int *iptr;

	    type = REF_ARRAY;

	    array = DXNewArray(TYPE_INT, CATEGORY_REAL, 0);
	    if (! array)
		goto error;
	    
	    if (!DXAddArrayData(array, 0, nInvalid, NULL))
		goto error;
	    
	    if (handle->sense == IC_MARKS_INDICATE_VALID)
	        target = IC_ELEMENT_UNMARKED;
	    else
	        target = IC_ELEMENT_MARKED;
	    
	    iptr = (int *)DXGetArrayData(array);
	    bptr = (ubyte *)handle->data;
	    for (i = 0; i < handle->nItems; i++)
		if (*bptr++ == target)
		   *iptr++ = i;
	}
    }
    else if (handle->type == IC_SORTED_LIST)
    {
	type = REF_ARRAY;

	array = DXNewArray(TYPE_INT, CATEGORY_REAL, 0);
	if (! array)
	    goto error;
	
	if (!DXAddArrayData(array, 0, nInvalid, handle->data))
	    goto error;
    }
    else if (handle->type == IC_HASH)
    {
	/*
	 * If its a hash table, then we may return either a dep
	 * or ref result, depending on which will be larger. 
	 * This depends on the number actually invalid, regardless
	 * of the sense of the hash table.
	 */
	int *dPtr;

	if (handle->nItems != -1 && 
	    nInvalid >= DEP_STORAGE_THRESHOLD(handle->nItems))
	{
	    ubyte *data;
	    long *sPtr;

	    type = DEP_ARRAY;

	    array = DXNewArray(TYPE_UBYTE, CATEGORY_REAL, 0);
	    if (! array)
		goto error;
		    
	    if (! DXAddArrayData(array, 0, handle->nItems, NULL))
		goto error;
		    
	    data = (ubyte *)DXGetArrayData(array);
		    
	    /*
	     * If marks indicate invalid, then initialize the array
	     * to valid, then traverse the hash list setting the
	     * array entries corresponding to the hash table
	     * contents to INVALID.  Otherwise, do it the opposite
	     * way.
	     */
	    if (handle->sense == IC_MARKS_INDICATE_INVALID)
	    {
		memset(data, DATA_VALID, handle->nItems*sizeof(ubyte));

		DXInitGetNextHashElement(handle->hash);
		while(NULL !=
		   	  (sPtr = (long *)DXGetNextHashElement(handle->hash)))
		{
		    data[*sPtr] = DATA_INVALID;
		}
	    }
	    else
	    {
		memset(data, DATA_INVALID, handle->nItems*sizeof(ubyte));

		DXInitGetNextHashElement(handle->hash);
		while(NULL !=
		   	  (sPtr = (long *)DXGetNextHashElement(handle->hash)))
		{
		    data[*sPtr] = DATA_VALID;
		}
	    }
	}
	else
	{
	    long *sPtr;

	    type = REF_ARRAY;

	    array = DXNewArray(TYPE_INT, CATEGORY_REAL, 0);
	    if (! array)
		goto error;

	    if (! DXAddArrayData(array, 0, nInvalid, NULL))
		goto error;
	
	    dPtr = (int *)DXGetArrayData(array);
	
	    /*
	     * If the marks indicate valid, then we need to
	     * determine the elements that DO NOT reside in the
	     * hash table.  We do this by trying all of them.
	     * This ain't great, but nothing better comes to
	     * mind.  If the marks indicate invalid, then we have
	     * the easy case.
	     */
	    if (handle->sense == IC_MARKS_INDICATE_VALID)
	    {
		for (li = 0; li < handle->nItems; li++)
		{
		    if (! DXQueryHashElement(handle->hash, (Key)&li))
			*dPtr++ = li;
		}
	    }
	    else
	    {
		DXInitGetNextHashElement(handle->hash);
		while(NULL !=
			  (sPtr = (long *)DXGetNextHashElement(handle->hash)))
		{
		    *dPtr++ = *sPtr;
		}
	    }
	}
    }
    else
    {
	DXSetError(ERROR_INTERNAL, "bad invalid component handle");
	goto error;
    }

    if (handle->iName)
    {
	attr = (Object)DXNewString(handle->iName);
	if (! attr)
	     goto error;

	if (type == DEP_ARRAY)
	    DXSetAttribute((Object)array, "dep", attr);
	else if (type == REF_ARRAY)
	    DXSetAttribute((Object)array, "ref", attr);
	else
	{
	    DXSetError(ERROR_INTERNAL, "unknown invalid array type");
	    goto error;
	}
    }

    return array;

error:
    DXDelete((Object)array);
    return NULL;
}

int
DXIsElementInvalid(InvalidComponentHandle handle, int index)
{
    if (handle->sense == IC_MARKS_INDICATE_VALID)
	return ! IsElementMarked(handle, index);
    else
	return IsElementMarked(handle, index);
}

int
DXIsElementValid(InvalidComponentHandle handle, int index)
{
    if (handle->sense == IC_MARKS_INDICATE_INVALID)
	return ! IsElementMarked(handle, index);
    else
	return IsElementMarked(handle, index);
}

static int
IsElementMarked(InvalidComponentHandle handle, int index)
{
    switch(handle->type)
    {
	case IC_ALL_MARKED:
	{
	    return TRUE;
	}

	case IC_ALL_UNMARKED: 
	{
	    return FALSE;
	}
	
	case IC_DEP_ARRAY:
	{
	    if (((ubyte *)(handle->data))[index] == IC_ELEMENT_MARKED)
		return TRUE;
	    else
		return FALSE;
	}
	
	case IC_HASH:
	{
	    long li = index;

	    if (DXQueryHashElement(handle->hash, (Key)&li))
		return TRUE;
	    else
		return FALSE;
	}

	case IC_SORTED_LIST:
	{
	    int *base = (int *)handle->data;
	    int max = handle->nMarkedItems-1;
	    int min = 0, mid;
	
	    if (handle->nMarkedItems == 0 	||
		base[0] > index 		||
		base[max] < index)
	    {
		return FALSE;
	    }

	    for (mid = (max + min) >> 1; mid != min; mid = (min + max) >> 1)
		if (base[mid] > index)
		    max = mid;
		else if (base[mid] < index)
		    min = mid;
		else
		    return TRUE;
	    
	    if (base[min] == index || base[max] == index) 
		return TRUE;
	    else
		return FALSE;
	}
    }
    /* Function should always short circuit, but just incase */
    return FALSE;
}

Error
DXSetElementInvalid(InvalidComponentHandle handle, int index)
{
    if (handle->sense == IC_MARKS_INDICATE_VALID)
	return RemoveMark(handle, index);
    else
	return SetMark(handle, index);
}

Error
DXSetElementValid(InvalidComponentHandle handle, int index)
{
    if (handle->sense == IC_MARKS_INDICATE_VALID)
	return SetMark(handle, index);
    else
	return RemoveMark(handle, index);
}

static Error
SetMark(InvalidComponentHandle handle, int index)
{
    long li;

    if (handle->type == IC_ALL_MARKED)
	return OK;

    /*
     * If this is the first invalidation, then we need to set up
     * a hash table to swallow invalidations until the threshold
     */
    if (handle->type == IC_ALL_UNMARKED)
    {
	handle->nMarkedItems = 0;

	handle->type = IC_HASH;

	/*
	 * Hashing will use a straight key
	 */
	handle->hash = DXCreateHash(sizeof(long), NULL, NULL);
	if (! handle->hash)
	    goto error;
    }

    /*
     * Now save the invalidation
     */
    switch(handle->type)
    {
	case IC_DEP_ARRAY:
	{
	    if (handle->array)
	    {
		ubyte *tmp = DXAllocate(handle->nItems*sizeof(ubyte));
		if (! tmp)
		    goto error;
		
		memcpy(tmp, handle->data, handle->nItems*sizeof(ubyte));

		DXDelete((Object)handle->array);
		handle->array = NULL;
		handle->data = tmp;
	    }

	    if (((ubyte *)(handle->data))[index] != IC_ELEMENT_MARKED)
	    {
		((ubyte *)(handle->data))[index] = IC_ELEMENT_MARKED;
		handle->nMarkedItems ++;
	    }
	    break;
	}
	
	case IC_HASH:
	{
	    li = index;

	    if (! DXQueryHashElement(handle->hash, (Key)&li))
	    {
		if (! DXInsertHashElement(handle->hash, (Element)&li))
		    goto error;
		handle->nMarkedItems ++;
	    }
	    break;
	}

	case IC_SORTED_LIST:
	{
	    /*
	     * If the thing is currently a sorted list and we are
	     * doing an insert, transform to a hash table
	     */
	    int i, *iPtr;
	    HashTable hash = DXCreateHash(sizeof(long), NULL, NULL);
	    if (! hash)
		goto error;
	    
	    for (i = 0, iPtr = (int *)handle->data; i < handle->nMarkedItems; i++)
	    {
		li = *iPtr++;

		if (! DXInsertHashElement(hash, (Element)&li))
		    goto error;
	    }
	    
	    if (handle->array)
	    {
		DXDelete((Object)handle->array);
		handle->array = NULL;
		handle->data  = NULL;
	    }
	    else
	    {
		DXFree((Pointer)handle->data);
		handle->data = NULL;
	    }

	    handle->hash = hash;
	    handle->type = IC_HASH;

	    /*
	     * Now add to hash table
	     */
	    li = index;
	    if (! DXQueryHashElement(handle->hash, (Key)&li))
	    {
		if (! DXInsertHashElement(handle->hash, (Element)&li))
		    goto error;
		handle->nMarkedItems ++;
	    }
	    break;
	}

	default:
	{
	    DXSetError(ERROR_INTERNAL, "unknown handle type");
	    goto error;
	}
    }

    /*
     * If not already a dependent array and we have passed the threshold,
     * convert to a dependent array. IF we know the overall size.
     */
    if ((handle->type != IC_DEP_ARRAY) && (handle->nItems != -1) &&
	(handle->nMarkedItems >= DEP_MEM_THRESHOLD(handle->nItems)))
    {
	ubyte *depArray = (ubyte *)DXAllocate(handle->nItems*sizeof(ubyte));
	if (! depArray)
	    goto error;
	
	memset(depArray, IC_ELEMENT_UNMARKED, handle->nItems);

	/*
	 * At this point, if we came in with a sorted list we will already
	 * have converted to a hash table, so if its not dependent, it
	 * must be hash.
	 */
	if (handle->type == IC_HASH)
	{
	    long *iPtr;

	    DXInitGetNextHashElement(handle->hash);
	    while(NULL != (iPtr = (long *)DXGetNextHashElement(handle->hash)))
	       depArray[*iPtr] = IC_ELEMENT_MARKED;
		
	    DXDestroyHash(handle->hash);
	    handle->hash = NULL;
	}
	else
	{ 
	    DXSetError(ERROR_INTERNAL,
		"attempting to expand an invalid type of invalid component");
	    goto error;
	}

	handle->data = depArray;
	handle->array = NULL;
	handle->type = IC_DEP_ARRAY;
    }

    return OK;

error:
    return ERROR;
}

static Error
RemoveMark(InvalidComponentHandle handle, int index)
{
    if (handle->type == IC_ALL_UNMARKED)
	return OK;

    /*
     * If this is the first invalidation, then we need to set up
     * a hash table to swallow invalidations until the threshold
     */
    if (handle->type == IC_ALL_MARKED)
    {
	handle->type = IC_HASH;
	handle->nMarkedItems = 0;
	handle->sense = !handle->sense;

	/*
	 * Hashing will use a straight key
	 */
	handle->hash = DXCreateHash(sizeof(long), NULL, NULL);
	if (! handle->hash)
	    goto error;
	
	if (! SetMark(handle, index))
	    goto error;
	
	return OK;
    }

    /*
     * Now save the invalidation
     */
    switch(handle->type)
    {
	case IC_DEP_ARRAY:
	{
	    if (handle->array)
	    {
		ubyte *tmp = DXAllocate(handle->nItems*sizeof(ubyte));
		if (! tmp)
		    goto error;
		
		memcpy(tmp, handle->data, handle->nItems*sizeof(ubyte));

		DXDelete((Object)handle->array);
		handle->array = NULL;
		handle->data = tmp;
	    }

	    if (((ubyte *)(handle->data))[index] != IC_ELEMENT_UNMARKED)
	    {
		((ubyte *)(handle->data))[index] = IC_ELEMENT_UNMARKED;
		handle->nMarkedItems --;
	    }
	    break;
	}
	
	case IC_HASH:
	{
	    long li = index;

	    if (DXQueryHashElement(handle->hash, (Key)&li))
	    {
		if (! DXDeleteHashElement(handle->hash, (Element)&li))
		    goto error;
		handle->nMarkedItems --;
	    }
	    break;
	}

	case IC_SORTED_LIST:
	{
	    /*
	     * If the thing is currently a sorted list and we are
	     * doing an deletion, transform to a hash table
	     */
	    int i, *iPtr;
	    long li;

	    HashTable hash = DXCreateHash(sizeof(long), NULL, NULL);
	    if (! hash)
		goto error;
	    
	    iPtr = (int *)handle->data;
	    for (i = 0; i < handle->nMarkedItems; i++)
	    {
		li = *iPtr++;
		if (! DXInsertHashElement(hash, (Element)&li))
		    goto error;
	    }
	    
	    if (handle->array)
	    {
		DXDelete((Object)handle->array);
		handle->array = NULL;
		handle->data  = NULL;
	    }
	    else
	    {
		DXFree((Pointer)handle->data);
		handle->data = NULL;
	    }

	    handle->hash = hash;
	    handle->type = IC_HASH;

	    /*
	     * Now add to hash table
	     */
	    li = index;
	    if (DXQueryHashElement(handle->hash, (Key)&li))
	    {
		if (! DXDeleteHashElement(handle->hash, (Element)&li))
		    goto error;
		handle->nMarkedItems --;
	    }
	    break;
	}

	default:
	{
	    DXSetError(ERROR_INTERNAL, "unknown handle type");
	    goto error;
	}
    }

    /*
     * If not already a dependent array and we have passed the threshold,
     * convert to a dependent array.
     */
    if ((handle->type != IC_DEP_ARRAY) && (handle->nItems != -1) &&
	(handle->nMarkedItems >= DEP_MEM_THRESHOLD(handle->nItems)))
    {
	ubyte *depArray = (ubyte *)DXAllocate(handle->nItems*sizeof(ubyte));
	if (! depArray)
	    goto error;
	
	memset(depArray, IC_ELEMENT_UNMARKED, handle->nItems);

	/*
	 * At this point, if we came in with a sorted list we will already
	 * have converted to a hash table, so if its not dependent, it
	 * must be hash.
	 */
	if (handle->type == IC_HASH)
	{
	    long *lPtr;

	    DXInitGetNextHashElement(handle->hash);
	    while(NULL != (lPtr = (long *)DXGetNextHashElement(handle->hash)))
	       depArray[*lPtr] = IC_ELEMENT_MARKED;
		
	    DXDestroyHash(handle->hash);
	    handle->hash = NULL;
	}
	else
	{ 
	    DXSetError(ERROR_INTERNAL,
		"attempting to expand an invalid type of invalid component");
	    goto error;
	}

	handle->data = depArray;
	handle->array = NULL;
	handle->type = IC_DEP_ARRAY;
    }

    return OK;

error:
    return ERROR;
}

int
DXIsElementInvalidSequential(InvalidComponentHandle handle, int index)
{
    /*
    if (handle->type == IC_ALL_UNMARKED)
	return FALSE;
    else if (handle->type == IC_ALL_MARKED)
	return TRUE;
    else if (handle->type == IC_SORTED_LIST)
    {
	int *data;

	if (index == 0)
	    handle->next = 0;
	
	if (handle->next >= handle->nMarkedItems)
	    return FALSE;

	data = (int *)(handle->data);

	do
	{
	    if (index < data[handle->next])
		return FALSE;
	    if (index == data[handle->next])
		return TRUE;
	}
	while (++(handle->next) < handle->nMarkedItems);

	return FALSE;
    }
    else
    */
	if(handle)
		return DXIsElementInvalid(handle, index);
	return 0;
}


int
DXIsElementValidSequential(InvalidComponentHandle handle, int index)
{
    /*
    if (handle->type == IC_ALL_UNMARKED)
	return FALSE;
    else if (handle->type == IC_ALL_MARKED)
	return TRUE;
    else if (handle->type == IC_SORTED_LIST)
    {
	int *data;

	if (index == 0)
	    handle->next = 0;
	
	if (handle->next >= handle->nMarkedItems)
	    return FALSE;

	data = (int *)(handle->data);

	do
	{
	    if (index < data[handle->next])
		return FALSE;
	    if (index == data[handle->next])
		return TRUE;
	}
	while (++(handle->next) < handle->nMarkedItems);

	return FALSE;
    }
    else
    */
	return DXIsElementValid(handle, index);
}

int
DXGetInvalidCount(InvalidComponentHandle handle)
{
    if (handle->sense == IC_MARKS_INDICATE_VALID)
	return handle->nItems - handle->nMarkedItems;
    else
	return handle->nMarkedItems;
}

int
DXGetValidCount(InvalidComponentHandle handle)
{
    if (handle->sense == IC_MARKS_INDICATE_INVALID)
	return handle->nItems - handle->nMarkedItems;
    else
	return handle->nMarkedItems;
}

static Field
DeleteFieldContents(Field f)
{
    char *cNames[1000];
    int  n;

    for (n = 0; DXGetEnumeratedComponentValue(f, n, cNames+n); n++);

    while (--n >= 0)
	DXDeleteComponent(f, cNames[n]);
    
    return DXEndField(f);
}

Error
DXSetAllValid(InvalidComponentHandle handle)
{
    if (! RemoveContents(handle))
	return ERROR;
    
    handle->type  = IC_ALL_UNMARKED;
    handle->sense = IC_MARKS_INDICATE_INVALID;

    return OK;
}

Error
DXSetAllInvalid(InvalidComponentHandle handle)
{
    if (! RemoveContents(handle))
	return ERROR;
    
    handle->type  = IC_ALL_UNMARKED;
    handle->sense = IC_MARKS_INDICATE_VALID;

    return OK;
}

static Error
RemoveContents(InvalidComponentHandle handle)
{
    switch(handle->type)
    {
	case IC_ALL_UNMARKED: 
	case IC_ALL_MARKED:
	{
	    break;
	}

	case IC_DEP_ARRAY:
	case IC_SORTED_LIST:
	{
	    if (handle->array)
	    {
		DXDelete((Object)handle->array);
		handle->array = NULL;
		handle->data = NULL;
	    }
	    else
	    {
		DXFree((Pointer)handle->data);
		handle->data = NULL;
	    }

	    break;
	}

	case IC_HASH:
	{
	    DXDestroyHash(handle->hash);
	    handle->hash = NULL;
	   
	    break;
	}

	default:
	    DXSetError(ERROR_INTERNAL,
		"unknown invalid component handle type");
	    goto error;
    }

    return OK;

error:
    return ERROR;
}

Error
DXInitGetNextInvalidElementIndex(InvalidComponentHandle handle)
{
    handle->nextCand  = 0;
    handle->nextSlot  = 0;
    handle->nextMarkI = -1;

    if (handle->type == IC_HASH)
	if (! MakeSortList(handle))
	    goto error;

    handle->nextMark = GetNextMarked(handle);
     
    return OK;
     
error:
    return ERROR;
}
    
Error
DXInitGetNextValidElementIndex(InvalidComponentHandle handle)
{
    return DXInitGetNextInvalidElementIndex(handle);
}

int
DXGetNextValidElementIndex(InvalidComponentHandle handle)
{
    if (handle->sense == IC_MARKS_INDICATE_VALID)
    {
	int next = handle->nextMark;
	handle->nextMark = GetNextMarked(handle);
	return next;
    }
    else
    {

	/*
	 * While there remain marks and our next candidate is marked, bump
	 * them both.
	 */
	while (handle->nextMark != -1 && handle->nextCand == handle->nextMark)
	{
	    handle->nextCand ++;
	    handle->nextMark = GetNextMarked(handle);
	}

	/*
	 * If there are no remaining marks and we are not done...
	 */
	if (handle->nextCand < handle->nItems)
	    return handle->nextCand++;
	else
	    return -1;
    }
}


int
DXGetNextInvalidElementIndex(InvalidComponentHandle handle)
{
    if (handle->sense == IC_MARKS_INDICATE_INVALID)
    {
	int next = handle->nextMark;
	handle->nextMark = GetNextMarked(handle);
	return next;
    }
    else
    {

	/*
	 * While there remain marks and our next candidate is marked, bump
	 * them both.
	 */
	while (handle->nextMark != -1 && handle->nextCand == handle->nextMark)
	{
	    handle->nextCand ++;
	    handle->nextMark = GetNextMarked(handle);
	}

	/*
	 * If there are no remaining marks and we are not done...
	 */
	if (handle->nextCand < handle->nItems)
	    return handle->nextCand++;
	else
	    return -1;
    }
}

Error
DXInvertValidity(InvalidComponentHandle handle)
{
    if (handle->sense == IC_MARKS_INDICATE_INVALID)
	handle->sense = IC_MARKS_INDICATE_VALID;
    else
	handle->sense = IC_MARKS_INDICATE_INVALID;
    
    return OK;
}

static Error
MakeSortList(InvalidComponentHandle handle)
{
    int i;
    long *p;

    if (handle->sortListSize != handle->nMarkedItems)
    {
	DXFree((int *)handle->sortList);
	handle->sortList = (int *)DXAllocate(handle->nMarkedItems*sizeof(int));
	if (! handle->sortList)
	    goto error;
	handle->sortListSize = handle->nMarkedItems;
    }

    DXInitGetNextHashElement(handle->hash); i = 0;
    while (NULL != (p = (long *)DXGetNextHashElement(handle->hash)))
    {
	if (i == handle->sortListSize)
	{
	    DXSetError(ERROR_INTERNAL, "sort list too short!");
	    goto error;
	}

	handle->sortList[i++] = *p;
    }

    if (i != handle->sortListSize)
    {
	DXSetError(ERROR_INTERNAL, "sort list wrong size!");
	goto error;
    }

    refsort(handle->sortList, handle->sortListSize);

    return OK;

error:
    return ERROR;
}

static int
GetNextMarked(InvalidComponentHandle handle)
{
    if (handle->nextMarkI >= handle->nItems)
	return -1;

    switch(handle->type)
    {
	case IC_ALL_MARKED:    
	{
	    if (handle->nextMarkI >= handle->nItems - 1)
		return -1;
	    else
	        return (++(handle->nextMarkI));
	}
	    
	case IC_ALL_UNMARKED:
	{
	    return -1;
	}

	case IC_SORTED_LIST:
	{
	    if (handle->nextSlot == handle->nMarkedItems)
		return -1;
	    else
		return ((int *)(handle->data))[handle->nextSlot++];
	}

	case IC_HASH:
	{
	    if (handle->nextSlot == handle->sortListSize)
		return -1;
	    else
		return ((int *)(handle->sortList))[handle->nextSlot++];
	}

	case IC_DEP_ARRAY:
	{
	    int i;
	    ubyte *ptr = (ubyte *)handle->data + handle->nextMarkI + 1;

	    for (i = handle->nextMarkI+1; i < handle->nItems; i++)
		if (*ptr++ == IC_ELEMENT_MARKED)
		{
		    handle->nextMarkI = i;
		    return i;
		}
	    
	    return -1;
	}
    }
    /* Function should be already returned, but just incase */
   return -1;
}


static int
GridSize(Array grid)
{
    switch(DXGetArrayClass(grid))
    {
	case CLASS_PATHARRAY:
	{
	    int count;

	    DXGetPathArrayInfo((PathArray)grid, &count);
	    return count;
	}
	
	case CLASS_ARRAY:
	{
	    byte *map = NULL;
	    int max, i, n, *elts, *e, nElements, nVerts, r;
	    Type t;
	    Category c;

	    DXGetArrayInfo((Array)grid, &nElements, &t, &c, &r, &nVerts);

	    if (t != TYPE_INT || c != CATEGORY_REAL || r != 1)
	    {
	        DXSetError(ERROR_DATA_INVALID, "connections");
		return -1;
	    }

	    elts = (int *)DXGetArrayData(grid);

	    n = nElements*nVerts;

	    for (i = 0, e = elts, max = -1; i < n; i++, e++)
		if (*e > max)
		    max = *e;

	    map = (byte *)DXAllocateZero((max+1)*sizeof(byte));
	    if (! map)
		return ERROR_OCCURRED;

	    for (i = 0, e = elts; i < n; i++, e++)
		if (*e >= 0)
		    map[*e] = 1;
	    
	    for (i = 0; i < max; i++)
		if (map[i] == 0)
		{
		    DXFree((Pointer)map);
		    return HOLES_FOUND;
		}

	    DXFree((Pointer)map);
	    return max + 1;
	}

	case CLASS_MESHARRAY:
	{
	    int nTerms, i, max;
	    Array terms[100];
	    int rtn;

	    DXGetMeshArrayInfo((MeshArray)grid, &nTerms, terms);

	    for (max = 1, i = 0; i < nTerms; i++)
	    {
		rtn = GridSize(terms[i]);
		if (rtn < 0)
		    return rtn;
		max *= rtn;
	    }

	    return max;
	}

	default:
	{
	    DXSetError(ERROR_DATA_INVALID, "connections component");
	    return ERROR_OCCURRED;
	}
    }
}
