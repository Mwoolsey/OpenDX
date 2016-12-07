/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dxmods/transpose.c,v 1.6 2006/01/03 17:02:26 davidt Exp $
 */

#include <dxconfig.h>

#if defined(HAVE_STRING_H)
#include <string.h>
#endif

#if defined(HAVE_CTYPE_H)
#include <ctype.h>
#endif

#include <dx/dx.h>

static Object Object_Transpose(Object o, int num, int *dimlist);
static Field Field_Transpose(Field f, int num, int *dimlist);
static Array Item_Transpose(Array a, int num, int *dimlist, char *comp);
static Array Connect_Transpose(Array a, int num, char *element);


#define STR(x)    (Object)DXNewString(x)

#define MAXRANK   16
#define MAXSHAPE 128



#define PARALLEL 1

 
#if PARALLEL
static Field Transpose_Wrapper(Field, char *, int);
struct argblk {
    int ndim;
    int dim[MAXRANK];
};
#endif

#define is_empty(o)  ((DXGetObjectClass(o)==CLASS_FIELD) && DXEmptyField((Field)o))


/* 0 = input
 * 1 = new dimension order
 *
 * 0 = output
 */
int
m_Transpose(Object *in, Object *out)
{
    int i, ndim;
    char cdim, *cp;
    int dim[MAXRANK];

    if(!in[0]) {
	DXSetError(ERROR_BAD_PARAMETER, "#10000", "input");
	return ERROR;
    }

    if(!in[1] || is_empty(in[0])) {
	out[0] = in[0];
	return OK;
    }

    /* input can either be a list of integers, or a list of strings.
     */
    if (DXQueryParameter(in[1], TYPE_STRING, 1, &ndim)) {
	
	/* list of strings - since there isn't a get-number-of-strings call,
	 *  check first to be sure there aren't too many, and then loop 
	 *  until there aren't any more.
	 */
	if(DXExtractNthString(in[1], MAXRANK, &cp)) {
	    DXSetError(ERROR_BAD_PARAMETER, "bad number of dimensions in list");
	    return ERROR;
	}
	for(i=0; i<MAXRANK; i++) {
	    if(!DXExtractNthString(in[1], i, &cp))
		break;
	    else {
		cdim = isupper(cp[0]) ? tolower(cp[0]) : cp[0];
		if(cdim < 'x' || cdim > 'z') {
		    DXSetError(ERROR_BAD_PARAMETER, "bad dimension letter");
		    return ERROR;
		}
		dim[i] = (int)(cdim - 'x');
	    }
	}
	ndim = i;
	if(ndim <= 1  || ndim > MAXRANK) {
	    DXSetError(ERROR_BAD_PARAMETER, "bad number of dimensions in list");
	    return ERROR;
	}
	
    } else if(DXQueryParameter(in[1], TYPE_INT, 1, &ndim)) {
	
	/* list of ints */
	if(ndim <= 1  || ndim > MAXRANK) {
	    DXSetError(ERROR_BAD_PARAMETER, "bad number of dimensions in list");
	    return ERROR;
	}
	if(!DXExtractParameter(in[1], TYPE_INT, 1, ndim, (Pointer)dim)) {
	    DXSetError(ERROR_BAD_PARAMETER, "bad dimension list");
	    return ERROR;
	}
	
    } else {
	/* neither integers or strings */
	DXSetError(ERROR_BAD_PARAMETER, "bad dimension list");
	return ERROR;
    }



#if PARALLEL
    { struct argblk argblk;
      int i;
      
      argblk.ndim = ndim;
      for (i=0; i<ndim; i++)
	  argblk.dim[i] = dim[i];

      out[0] = DXProcessParts(in[0], Transpose_Wrapper, (char *)&argblk, 
			    sizeof(argblk), 1, 1);
    
    }
#else

    if (!(out[0] = DXCopy(in[0], COPY_STRUCTURE))) {
	DXSetError(ERROR_BAD_PARAMETER, "bad input object");
	return ERROR;
    }
	    
    out[0] = Object_Transpose(out[0], ndim, dim);
#endif
    
    return(out[0] ? OK : ERROR);


}

#if PARALLEL
/*
 */ 
static Field Transpose_Wrapper(Field f, char *argblk, int size)
{
    return (Field)Object_Transpose((Object)f, 
				   ((struct argblk *)argblk)->ndim,
				   ((struct argblk *)argblk)->dim);
}
#endif

/*
 */
static Object Object_Transpose(Object o, int num, int *dims)
{
    Object subo;
    char *name;
    int i;

    switch(DXGetObjectClass(o)) {
      case CLASS_GROUP:
	/* for each member */
	for(i=0; (subo=DXGetEnumeratedMember((Group)o, i, &name)); i++) {
	    if(!Object_Transpose(subo, num, dims))
		return NULL;
	}
	break;

      case CLASS_FIELD:
#if ! PARALLEL
        if(DXEmptyField((Field)o))
            break;

#else
        if(DXEmptyField((Field)o))
            return NULL;

	o = DXCopy(o, COPY_STRUCTURE);
	if(!o)
	    return NULL;
#endif
	if (!Field_Transpose((Field)o, num, dims)) {
	    DXDelete(o);
	    o = NULL;
	}
	break;
	
      default:
        /* ignore things we don't understand */
        break;
    }
    
    return o;
}




/*
 */
static Field Field_Transpose(Field f, int num, int *dims)
{
    Object newo, subo;
    char *name, *attr;
    int i, j, n, nitems, rank;
    int inverted = 0;
    int ndims[MAXRANK];
    int pcounts[MAXRANK], ccounts[MAXRANK];
    float origins[MAXRANK], norigins[MAXRANK];
    float deltas[MAXRANK*MAXRANK], ndeltas[MAXRANK*MAXRANK];


    /* transpose positions.
     */
    subo = DXGetComponentValue(f, "positions");
    if(!subo) {
        DXSetError(ERROR_MISSING_DATA, "#10240", "positions");
	return ERROR;
    }
    
    if(!DXTypeCheckV((Array)subo, TYPE_FLOAT, CATEGORY_REAL, 1, NULL)) {
        DXSetError(ERROR_DATA_INVALID, "positions must be type float, rank 1"); 
        return NULL;
    }
    
    /* make sure all the dims are valid before proceeding */
    DXGetArrayInfo((Array)subo, &nitems, NULL, NULL, &rank, pcounts);
    if(rank != 1) {
        if(rank == 0)
            pcounts[0] = 1;    /* rank 0 == rank 1, shape 1 */
        else
            DXErrorReturn(ERROR_DATA_INVALID, "bad position component");
    }
    
    if(num != pcounts[0]) {
        DXSetError(ERROR_BAD_PARAMETER, "#11365", num, pcounts[0]);
        return NULL;
    }
    
    /* initialize a use-counts list.  in the second loop, if we find the 
     *  same dimension more than once in the list, it's an error.
     */
    for(i=0; i<pcounts[0]; i++)
        ndims[i] = 0;
    
    for(i=0; i<pcounts[0]; i++) {
        /* out of range?
	 */
        if(dims[i] < 0  || dims[i] >= num) {
            DXSetError(ERROR_BAD_PARAMETER, "#11361", dims[i], "input", num-1);
            return NULL;
        }
        /* duplicates?
	 */
        if(++ndims[dims[i]] > 1) {
            DXSetError(ERROR_BAD_PARAMETER, 
                     "dimension %d appears more than once in list",
                     dims[i]);
            return NULL;
        }
    }
    
    
    /* dim list should be ok.  now rearrange positions 
     */
    if(DXQueryGridPositions((Array)subo, &n, pcounts, origins, deltas)) {
        /* regular grid 
	 */
        for(i=0; i<n; i++)
            norigins[i] = origins[dims[i]];
	
        for(i=0; i<n; i++)
            for(j=0; j<n; j++)
                ndeltas[i*n + j] = deltas[i*n + dims[j]];

	
        newo = (Object)DXMakeGridPositionsV(n, pcounts, norigins, ndeltas);
        

    } else {
        /* irregular positions, have to reorder them inside each point.
	 */
        newo = (Object)Item_Transpose((Array)subo, num, dims, "positions");
        
        inverted = 1;
        for(i=0; i<num; i++)
            if(dims[i] != i)
                inverted = !inverted;
        
    }

    if (!newo)
	return ERROR;

    if (!DXSetComponentValue(f, "positions", (Object)newo)
	|| !DXChangedComponentValues(f, "positions"))
	return ERROR;
    
    
    /* connections, if they exist and need to be inverted.
     */
    subo = DXGetComponentValue(f, "connections");
    if (subo && inverted) {
	
	if (!DXTypeCheckV((Array)subo, TYPE_INT, CATEGORY_REAL, 1, NULL)) {
	    DXSetError(ERROR_DATA_INVALID, "bad connections component"); 
	    return NULL;
	}
	
	/* if the connections are irregular, and we've inverted the axes,
	 *  then we have to reorder the verticies of the connections.
	 */
	if (!DXQueryGridConnections((Array)subo, &n, ccounts)) {
	    
	    DXGetArrayInfo((Array)subo, &nitems, NULL, NULL, &rank, ccounts);
	    if (rank != 0  &&  rank != 1) {
		DXSetError(ERROR_DATA_INVALID, "connections component not rank 1");
		return NULL;
	    }
	    
	    attr = DXGetString((String)DXGetAttribute((Object)subo, "element type"));
	    if (!attr) {
		DXSetError(ERROR_DATA_INVALID, "connections have no element type");
		return NULL;
	    }
	    
	    newo = (Object)Connect_Transpose((Array)subo, ccounts[0], attr);
	    if (!newo)
		return NULL;
	    
	
	    if (!DXSetComponentValue(f, "connections", (Object)newo) ||
		!DXChangedComponentValues(f, "connections"))
		return ERROR;

	}
    }
	

    /* for each of the rest of the components, see if they need 
     *  to be transposed.
     */
    for(i=0; (subo=DXGetEnumeratedComponentValue(f, i, &name)); i++) {

        /* skip positions & connections.
	 */
        if (!strcmp("positions", name) || !strcmp("connections", name))
            continue;
        
	/* for now, special case normals.  as soon as we can agree,
	 *  skip any component which doesn't have the 'XXX' attribute set.
	 */
        attr = DXGetString((String)DXGetAttribute(subo, "geometric"));
	
	if (!attr && strcmp(name, "normals"))
	    continue;

	newo = (Object)Item_Transpose((Array)subo, num, dims, name);

	if (!newo ||
	    !DXSetComponentValue(f, name, (Object)newo) ||
	    !DXCopyAttributes(subo, newo))
	    return ERROR;
	
    }
    
    DXEndField(f);
    return f;

}


/*
 * reorder the connections in an array, if necessary
 */
static Array Connect_Transpose(Array a, int num, char *element)
{
    Array na = a;
    int i, ndims[MAXRANK];

    if (!strcmp(element, "triangles") || !strcmp(element, "tetrahedra")) {
	
	/* construct a new dim array with 0<->1, and call item_tr() */
	ndims[0] = 1;
	ndims[1] = 0;
	for(i=2; i<num; i++)
	    ndims[i] = i;
	
	na = Item_Transpose(a, num, ndims, "connections");
	
    } 
    else if (!strcmp(element, "cubes"))
	;
    else if (!strcmp(element, "quads"))
	;
    else if (!strcmp(element, "lines"))
	;
    else if (!strcmp(element, "points"))
	;
    else {
	DXWarning("unrecognized element type; connections not inverted");
    }

    return na;
}


/* 
 * for each vector item in an array, reorder the items inside the vector
 */
static Array Item_Transpose(Array a, int num, int *dims, char *comp)
{
    Type t;
    Category c;
    int rank, shape[MAXRANK];
    float origins[MAXRANK], norigins[MAXRANK];
    float deltas[MAXRANK], ndeltas[MAXRANK];
    Array na;
    Array terms[MAXRANK], nterms[MAXRANK];
    int nitems, typesize;
    Pointer op, np;
    float *ofp, *nfp;
    int i, j;
    

    /* regardless of the array type, these checks apply */
    if(!DXGetArrayInfo(a, &nitems, &t, &c, &rank, shape))
	return NULL;
    
    if(rank == 0 || (rank == 1 && shape[0] == 1))
	return a;        /* transpose of single wide vector is itself */
    
    if(rank != 1) {
	DXSetError(ERROR_DATA_INVALID, "can only transpose vector data");
	return NULL;     /* only transposes rank 1 arrays */
    }
    
    if(num != shape[0]) {
	DXSetError(ERROR_DATA_INVALID, 
		   "number of dimensions doesn't match shape of '%s'", comp);
	return NULL;
    }
	

    switch(DXGetArrayClass(a)) {
	
      default:
      case CLASS_ARRAY:
	/* normal expanded array */
	
	na = DXNewArrayV(t, c, rank, shape);
        if(!na)
	    return NULL;
	
	np = DXGetArrayData(DXAddArrayData(na, 0, nitems, NULL));
	op = DXGetArrayData(a);

	nitems *= DXCategorySize(c);

	/* for speed, special case floats */
	if(t == TYPE_FLOAT && c == CATEGORY_REAL) {
	    ofp = (float *)op;
	    nfp = (float *)np;

	    for(i=0; i<nitems; i++) {
		for(j=0; j<shape[0]; j++)
		    nfp[j] = ofp[dims[j]];

		nfp += shape[0];
		ofp += shape[0];
	    }
	} else {
	    typesize = DXTypeSize(t);
	    for(i=0; i<nitems; i++) {

		for(j=0; j<shape[0]; j++)
		    memcpy((char *)np + dims[j]*typesize, 
			   (char *)op + j*typesize,
			   typesize);

		np = (Pointer)((char *)np + typesize*shape[0]);
		op = (Pointer)((char *)op + typesize*shape[0]);
	    }
	}

	break;

      case CLASS_CONSTANTARRAY:

	np = DXAllocate(DXGetItemSize(a));
	op = DXGetArrayData(a);
	if (!np || !op)
	    return NULL;

	typesize = DXTypeSize(t) * DXCategorySize(c);
	for(j=0; j<shape[0]; j++)
	    memcpy((char *)np + dims[j]*typesize, 
		   (char *)op + j*typesize,
		   typesize);
	    

	na = (Array) DXNewConstantArrayV(nitems, np, t, c, rank, shape);
	DXFree(np);
	break;

      case CLASS_REGULARARRAY:

	if(!DXGetRegularArrayInfo((RegularArray)a, &i, 
				(Pointer)origins, (Pointer)deltas))
	    return NULL;

	for(j=0; j<shape[0]; j++) {
	    norigins[j] = origins[dims[j]];
	    ndeltas[j] = deltas[dims[j]];
	}
	na = (Array)DXNewRegularArray(t, shape[0], i, 
				    (Pointer)norigins, (Pointer)ndeltas);
	break;

      case CLASS_PATHARRAY:
	/* doesn't change */

	na = a;
	break;

      case CLASS_PRODUCTARRAY:
	/* recurse until you get to a more fundamental type of array */

	DXGetProductArrayInfo((ProductArray)a, &j, terms);
	for(i=0; i<j; i++)
	    nterms[i] = Item_Transpose(terms[i], num, dims, comp);

	na = (Array)DXNewProductArrayV(j, nterms);
	break;
	
      case CLASS_MESHARRAY:
	/* recurse until you get to a more fundamental type of array */

	DXGetMeshArrayInfo((MeshArray)a, &j, terms);
	for(i=0; i<j; i++)
	    nterms[i] = Item_Transpose(terms[i], num, dims, comp);

	na = (Array)DXNewMeshArrayV(j, nterms);
	break;
	
    }
    
    return na;
    
}

