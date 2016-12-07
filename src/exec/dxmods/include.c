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


struct argblk {
    int justcull;	/* if set, don't take stats - just cull */
    int reallycull;	/* if set, call cull instead of leaving inv comps */ 
    int shape;		/* rank 1 and either shape == 1 or shape == data */
    float *min;		/* pointer to allocated list of mins */
    float *max;		/* pointer to allocated list of maxs */
    int exclude;	/* 0 = include, 1 = exclude */
    int noconnect;      /* 1 = remove connections first before doing include */
};
static Object Object_Include(Object o, struct argblk *);
static Field Field_Include(Field f, int justcull, int shape, 
			   float *min, float *max, int exclude, 
			   int reallycull, int removeconnect);

static Error VectorLength(Object o, int *len);
static Error VectorExtract(Object o, int len, float *ptr);

extern Object _dxfDXEmptyObject(Object o); /* from libdx/component.c */
#define IsEmpty(o) _dxfDXEmptyObject(o)

extern Array DXScalarConvert(Array a); /* from libdx/stats.h */


static Field Include_Wrapper(Field f, char *argblk, int size);
static Array Include_Array(Array a, struct argblk b);


/* 0 = input
 * 1 = min val
 * 2 = max val
 * 3 = exclude flag
 * 4 = really cull?
 * 5 = pointwise flag
 *
 * 0 = output
 */
int
m_Include(Object *in, Object *out)
{
    struct argblk argblk;
    int msize, i;
    int minset = 0;
    int maxset = 0;
    int simple_array = 0;

    /* arg defaults */
    argblk.justcull = 0;
    argblk.reallycull = 1;
    argblk.shape = 1;
    argblk.min = NULL;
    argblk.max = NULL;
    argblk.exclude = 0;
    argblk.noconnect = 0;


    out[0] = NULL;

    if (!in[0]) {
	DXSetError(ERROR_BAD_PARAMETER, "#10000", "input");
	return ERROR;
    }


    if (DXGetObjectClass(in[0]) == CLASS_ARRAY)
	simple_array++;
    
    
    /* if the caller doesn't set min, max or exclude, then the only useful
     *  thing we can be doing is cull. 
     */
    if (!in[1] && !in[2] && !in[3])
	argblk.justcull = 1;
    
    if (in[1]) {
	/* DXQueryParameter is useless here.
	 * i want 1 float vector and to know the shape, not give
	 * the shape and get the count... 
	 */
	if (!VectorLength(in[1], &argblk.shape)) {
	    DXSetError(ERROR_BAD_PARAMETER, "#10640", "minimum value");
	    return ERROR;
	}
	
	argblk.min = (float *)DXAllocate(argblk.shape * sizeof(float));
	if (!argblk.min)
	    goto error;
	
	if (!VectorExtract(in[1], argblk.shape, argblk.min))
	    goto error;
	
	minset++;

    }

    if (in[2]) {
	/* same here */
	if (!VectorLength(in[2], &msize)) {
	    DXSetError(ERROR_BAD_PARAMETER, "#10640", "maximum value");
	    return ERROR;
	}

	if (minset) {
	    if (msize != argblk.shape) {
		DXSetError(ERROR_BAD_PARAMETER, 
			   "shape of max must match shape of min");
		goto error;
	    }
	} else
	    argblk.shape = msize;

	argblk.max = (float *)DXAllocate(argblk.shape * sizeof(float));
	if (!argblk.max)
	    goto error;
	
	if (!VectorExtract(in[2], argblk.shape, argblk.max))
	    goto error;

	maxset++;

    }

    /* the 4 possibilities here are: 
     *   neither min or max are set - shape is 1, limits are minfloat/maxfloat
     *   min is set but not max - set max to same shape, all lims maxfloat
     *   max is set but not min - set min to same shape, all lims minfloat
     *   both min and max are already set - do nothing
     */
    if (!minset && !maxset) {
	argblk.min = (float *)DXAllocate(sizeof(float));
	if (!argblk.min)
	    goto error;

	*argblk.min = -DXD_MAX_FLOAT;

	argblk.max = (float *)DXAllocate(sizeof(float));
	if (!argblk.max)
	    goto error;

	*argblk.max = DXD_MAX_FLOAT;
    }
    else if (!minset && maxset) {
	argblk.min = (float *)DXAllocate(argblk.shape * sizeof(float));
	if (!argblk.min)
	    goto error;

	for (i=0; i<argblk.shape; i++)
	    argblk.min[i] = -DXD_MAX_FLOAT;

    }
    else if (minset && !maxset) {
	argblk.max = (float *)DXAllocate(argblk.shape * sizeof(float));
	if (!argblk.max)
	    goto error;

	for (i=0; i<argblk.shape; i++)
	    argblk.max[i] = DXD_MAX_FLOAT;
    }




    for (i=0; i<argblk.shape; i++)
	if (argblk.min[i] > argblk.max[i]) {
	    DXSetError(ERROR_BAD_PARAMETER, "min greater than max");
	    goto error;
	}

	
    if (in[3]) {
	if (!DXExtractInteger(in[3], &argblk.exclude) ||
	    (argblk.exclude != 0 && argblk.exclude != 1)) {
	    DXSetError(ERROR_BAD_PARAMETER, "#10070", "exclude flag");
	    goto error;
	}

    } 

    if (in[4]) {
	if (!DXExtractInteger(in[4], &argblk.reallycull) ||
	    (argblk.reallycull != 0 && argblk.reallycull != 1)) {
	    DXSetError(ERROR_BAD_PARAMETER, "#10070", "cull flag");
	    goto error;
	}
	if (simple_array && (argblk.reallycull == 0)) {
	    DXSetError(ERROR_BAD_PARAMETER, "input is an array so invalid data points must be culled out");
	    goto error;
	}
    }

    if (in[5]) {
	if (!DXExtractInteger(in[5], &argblk.noconnect) ||
	    (argblk.noconnect != 0 && argblk.noconnect != 1)) {
	    DXSetError(ERROR_BAD_PARAMETER, "#10070", "pointwise flag");
	    goto error;
	}
    }


    /* allow empty objects to pass thru unchanged */
    if (!simple_array && IsEmpty(in[0])) {
	DXWarning("#4000", "input");
	out[0] = in[0];
	goto done;
    }


    /* if the user didn't set max or min, then all they can want us to
     * do is remove invalid and/or unreferenced points.  if the input
     * is a simple array, there's nothing to do. 
     */
    if (argblk.justcull && simple_array) {
	out[0] = in[0];
	goto done;
    }
    
    /* if we aren't just culling, then at least one of the fields in the
     * input has to have a data component.
     */
    if (!argblk.justcull && !simple_array && !DXExists(in[0], "data")) {
	DXSetError(ERROR_DATA_INVALID, "#10240", "data");
	goto error;
    }
    
    /* make sure the stats are computed for the whole object before
     * starting the traversal.  this makes sure the stats are right
     * for composite fields.
     */
    if (!argblk.justcull && !simple_array && !DXStatistics((Object)in[0], 
					      "data", NULL, NULL, NULL, NULL)) 
	goto error;


    if (simple_array)
	out[0] = (Object)Include_Array((Array)in[0], argblk);
    else
	out[0] = DXProcessParts(in[0], Include_Wrapper, (char *)&argblk, 
				sizeof(argblk), 1, 1);

    
    /* handle the case where all the data is excluded, and the wrapper
     *  has returned an empty field, but process parts has culled it
     *  out so the return is null (but there is no error).
     */
    if(!out[0] && DXGetError()==ERROR_NONE)
	out[0] = (Object)DXEndField(DXNewField());

  done:
    DXFree((Pointer)argblk.min);
    DXFree((Pointer)argblk.max);
    
    return(out[0] ? OK : ERROR);

  error:
    DXFree((Pointer)argblk.min);
    DXFree((Pointer)argblk.max);

    out[0] = NULL;
    return ERROR;
}


/*
 */ 
static Field Include_Wrapper(Field f, char *a, int size)
{
    return (Field)Object_Include((Object)f, (struct argblk *)a);
}

static Object Object_Include(Object o, struct argblk *ap)
{
    Object newo, subo;
    Matrix m;
    Object subo2;
    int fixed, z;
    int i;
    

    switch(DXGetObjectClass(o)) {
      case CLASS_GROUP:
	/* for each member */
	for (i=0; (subo = DXGetEnumeratedMember((Group)o, i, NULL)); i++) {
	    if (!Object_Include(subo, ap))
		continue;
	}
	break;
	
      case CLASS_FIELD:
	/* even empty fields have to be copied */
	if (DXEmptyField((Field)o))
	    return DXCopy(o, COPY_STRUCTURE);
	
	if (!(newo = DXCopy(o, COPY_STRUCTURE)))
            return NULL;

	if (!(o = (Object)Field_Include((Field)newo, ap->justcull, ap->shape,
					ap->min, ap->max, 
					ap->exclude, ap->reallycull,
					ap->noconnect))) {
	    DXDelete(newo);
	    return NULL;
	}
	break;

      case CLASS_XFORM:
	if (!DXGetXformInfo((Xform)o, &subo, &m))
	    return NULL;

	if (!(newo=Object_Include(subo, ap)))
	    return NULL;

	o = (Object)DXNewXform(newo, m);
	break;


      case CLASS_SCREEN:
	if (!DXGetScreenInfo((Screen)o, &subo, &fixed, &z))
	    return NULL;

	if (!(newo=Object_Include(subo, ap)))
	    return NULL;

	o = (Object)DXNewScreen(newo, fixed, z);
	break;


      case CLASS_CLIPPED:
	if (!DXGetClippedInfo((Clipped)o, &subo, &subo2))
	    return NULL;

	if (!(newo=Object_Include(subo, ap)))
	    return NULL;

	o = (Object)DXNewClipped(newo, subo2);
	break;

	
      default:
	/* any remaining objects can't contain fields, so we can safely
	 *  pass them through unchanged.
	 */
	break;

    }

    return o;

}




static Field 
Field_Include(Field f, int justcull, int shape, float *min, float *max, 
	      int exclude, int reallycull, int removeconnect)
{
    int items;
    Object o = NULL;
    Array adata = NULL;
    Array afloat = NULL;
    char *attr;
    char inv_attr[64];   /* at least long enough for "invalid xxx" */
    float tmin, tmax;
    float *fp;
    Pointer dp;
    int i, j;
    int dopositions = 0;
    int doconnections = 0;
    int wasconnections = 0;
    int wasinvalid = 0;
    InvalidComponentHandle icHandle = NULL;


    /* see if there is a connections component - if not, don't cull out
     *  unreferenced positions at end.  but first, make sure they do want
     *  us to use the component.  
     */
    if (DXGetComponentValue(f, "connections") ||
	DXGetComponentValue(f, "faces")  ||
	DXGetComponentValue(f, "polylines")) {
	if (removeconnect) {
	    DXDeleteComponent(f, "connections");
	    DXDeleteComponent(f, "faces");
	    DXDeleteComponent(f, "polylines");
	} else
	    wasconnections++;
    }
    

    /* look at the data component.
     */
    if ((adata = (Array)DXGetComponentValue(f, "data"))) {
    
	if (!DXGetArrayInfo(adata, &items, NULL, NULL, NULL, NULL))
	    return NULL;

	/* make sure you don't reuse attr - it gets used later... */
	attr = DXGetString((String)DXGetAttribute((Object)adata, "dep"));

	if (attr && !strcmp(attr, "positions"))
	    dopositions++;
	else if (attr && !strcmp(attr, "connections"))
	    doconnections++;
	else if (attr && !strcmp(attr, "faces"))
	    doconnections++;
	else if (attr && !strcmp(attr, "polylines"))
	    doconnections++;
	else
	    DXErrorReturn(ERROR_DATA_INVALID, 
		     "data must be dependent on either positions or connections");

	/* ...and so does this. */
	if (dopositions) {
	    if (DXGetComponentValue(f, "connections"))
		strcpy(inv_attr, "invalid connections");
	    else if (DXGetComponentValue(f, "faces"))
		strcpy(inv_attr, "invalid faces");
	    else if (DXGetComponentValue(f, "polylines"))
		strcpy(inv_attr, "invalid polylines");
	} else {
	    strcpy(inv_attr, "invalid positions");
	    if (removeconnect) {
		DXErrorReturn(ERROR_BAD_PARAMETER,
			      "cannot set pointwise flag to ignore connections because data is dependent on connections");
	    }
	}
    } else {  /* no data component */
	if (!justcull)
		return NULL;

	attr = "positions";
	dopositions++;
	strcpy(inv_attr, "invalid connections");
    }

    /* this used to be sooner, but now we need to know what the data
     * is dep on before going to the end.
     */
    if (justcull)
	goto cullonly;
    

    /* do some simple checks first - if there is going to be no data,
     *  or all the data will remain, then there is no need to look at
     *  all the points.  if the cull flag is set, we can cull out
     *  unreferenced points without having to compute the statistics.
     */

    if (shape == 1) {
	if (!DXStatistics((Object)f, "data", &tmin, &tmax, NULL, NULL)) 
	    return NULL;

	
	/* include and all points in range, or exclude & all out of range.
	 *  keep all data.  still call cull to remove unreferenced points.
	 */
	if ((!exclude && (tmin >= *min && tmax <= *max))
	    || (exclude && (tmin >  *max || tmax <  *min)))
	    goto cullonly;
	
	
	/* include and all points out of range, or exclude & all in range.
	 *  delete the old field and just make an empty new one.
	 *  if the flag is set so we can't remove invalid data, we have
	 *  to make an invalid component instead.
	 */
	if ((!exclude && (tmax <  *min || tmin >  *max))
	    || (exclude && (tmin >= *min && tmax <= *max))) {
	    
	    if (reallycull) {
		o = (Object)DXNewField();
		if (!o)
		    goto error;
		DXCopyAttributes (o, (Object)f);
		DXDelete ((Object)f);
		return (DXEndField((Field)o));
	    } else {
		byte d = 1;
		o = (Object)DXNewConstantArray(items, (Pointer)&d,
					       TYPE_BYTE, CATEGORY_REAL,
					       0);
		if (dopositions) {
		    DXDeleteComponent(f, "invalid positions");
		    DXSetStringAttribute(o, "dep", "positions");
		    if (!DXSetComponentValue(f, "invalid positions", o))
			goto error;
		} else if (doconnections) {
		    DXDeleteComponent(f, inv_attr);
		    DXSetStringAttribute(o, "dep", attr);
		    if (!DXSetComponentValue(f, inv_attr, o))
			goto error;
		}
		goto cullonly;
	    }
	}

	/* convert the data to scalar float if it is anything else.
	 *  this is the same routine the statistics code uses.
	 */
	if (!DXTypeCheck(adata, TYPE_FLOAT, CATEGORY_REAL, 0)) {
	    if ((afloat = DXScalarConvert(adata)) == NULL)
		return NULL;
	    
	    if(!(dp = DXGetArrayData(afloat)))
		goto error;
	    
	} else {
	    
	    if(!(dp = DXGetArrayData(adata)))
		goto error;
	}
    } else {
	/* shape != 1, can't do simple rejection with statistics 
	 */
	if (!DXTypeCheck(adata, TYPE_FLOAT, CATEGORY_REAL, 1, shape)) {
	    if ((afloat = DXArrayConvert(adata, TYPE_FLOAT, CATEGORY_REAL,
					 1, shape)) == NULL)
		return NULL;
	    
	    if(!(dp = DXGetArrayData(afloat)))
		goto error;
	    
	} else {
	    
	    if(!(dp = DXGetArrayData(adata)))
		goto error;
	}
    }

    /* for exclude, find out if the field originally came in with 
     *  invalid components.  see about 20 lines below for more info.
     */
    if (exclude) {
	if ( (dopositions   && DXGetComponentValue(f, "invalid positions"))
	  || (doconnections && DXGetComponentValue(f, "invalid connections"))
	  || (doconnections && DXGetComponentValue(f, "invalid faces"))
	  || (doconnections && DXGetComponentValue(f, "invalid polylines")))
	    wasinvalid++;
    }

    /* create invalid data array to mark which data is being discarded,
     *  or get handle to existing invalid array if it already exists.
     */
    
    if (dopositions)
	icHandle = DXCreateInvalidComponentHandle((Object)f,
						NULL, "positions");
    else if (doconnections)
	icHandle = DXCreateInvalidComponentHandle((Object)f, NULL, attr);

    /* if test is exclude instead of include, we are going to mark all
     *  the items to keep and then invert the sense of the invalid array
     *  at the end.  that means we have to invert the sense of any 
     *  existing invalid items now before we start.
     */
    if (wasinvalid) {
	DXInvertValidity(icHandle);

	/* now iterate through data.  if it's marked invalid now, it used
         * to be valid.  see if any invalid things need to be validated.
	 * then at the end the array will be inverted.
	 */
	if (shape == 1) {
	    for (i=0, fp=(float *)dp; i<items; i++, fp++) {
		if (DXIsElementInvalid(icHandle, i) && (*fp >= *min && *fp <= *max))
		    DXSetElementValid(icHandle, i);
	    }
	} else {
	    for (i=0, fp=(float *)dp; i<items; i++, fp += shape) {
		if (DXIsElementValid(icHandle, i))
		    continue;
		for (j=0; j<shape; j++) {
		    if (*(fp+j) <  *(min+j) || *(fp+j) >  *(max+j))
			continue;
		}
		if (j == shape)
		    DXSetElementValid(icHandle, i);
	    }
	}

    } else {  /* no invalid components before */

	/* iterate through data.  keep either the positions or connections
	 *  which correspond to good data.  then at the end, cull out 
	 *  everything which has been made invalid.
	 */
	if (shape == 1) {
	    for (i=0, fp=(float *)dp; i<items; i++, fp++) {
		if (*fp <  *min || *fp >  *max)
		    DXSetElementInvalid(icHandle, i);
	    }
	} else {
	    for (i=0, fp=(float *)dp; i<items; i++, fp += shape) {
		for (j=0; j<shape; j++) {
		    if (*(fp+j) <  *(min+j) || *(fp+j) >  *(max+j)) {
			DXSetElementInvalid(icHandle, i);
			break;
		    }
		}
	    }
	}
    }
    
    if (exclude)
	DXInvertValidity(icHandle);

    if (! DXSaveInvalidComponent(f, icHandle))
	goto error;
    DXFreeInvalidComponentHandle(icHandle);
    icHandle = NULL;
    
  cullonly:
    /* see what's been invalidated.
     *  for each thing which deps data, remove the same items.  for
     *  anything which refs it, renumber it.  for anything which refs
     *  something which defs it, remove before renumbering.
     */
    if (wasconnections) {

        if (!DXInvalidateConnections((Object)f))
            goto error;
        
        if (!DXInvalidateUnreferencedPositions((Object)f))
            goto error;
     
    }

    /* the default is to cull out the invalid items by really removing them
     *  from the field.  if the caller turns off the cull option, be sure
     *  any components which are derived from the data component are deleted
     *  (DXCull does this for you), and remove the opposite invalid component
     *  which may have been created while processing the field.  the output
     *  should only have an "invalid xxx" component (if you aren't culling), 
     *  where 'xxx' matches what the data is dep on.
     */
    if (reallycull) {
	if (!DXCull((Object)f))
	    goto error;
    } else {
	DXChangedComponentValues(f, "data");
	if (dopositions)
	    DXDeleteComponent(f, inv_attr);
	else if (doconnections)
	    DXDeleteComponent(f, "invalid positions");
    }
    
    DXEndField(f);
    DXDelete((Object)afloat);

    return f;
    
  error:
    DXDelete((Object)afloat);
    DXFreeInvalidComponentHandle(icHandle);
    return NULL;
}


static Array
Include_Array(Array a, struct argblk b)
{
    int items;
    Array na = NULL;
    Array afloat = NULL;
    float tmin, tmax;
    float *fp;
    Pointer dp, odp;
    int i, j, bytes, id;
    Type t;
    Category c;
    int rank, shape[32];


    if (!DXGetArrayInfo(a, &items, &t, &c, &rank, shape))
	return NULL;


    /* do some simple checks first - if there is going to be no data,
     *  or all the data will remain, then there is no need to look at
     *  all the points.
     */

    if (b.shape == 1) {
	if (!DXStatistics((Object)a, "data", &tmin, &tmax, NULL, NULL)) 
	    return NULL;

	
	/* include and all points in range, or exclude & all out of range.
	 *  keep all data, which means returning the input array is fine.
	 */
	if ((!b.exclude && (tmin >= *b.min && tmax <= *b.max))
	    || (b.exclude && (tmin >  *b.max || tmax <  *b.min)))
	    return a;
	
	/* include and all points out of range, or exclude & all in range.
	 * return an empty array of the same type as the input.
	 */
	if ((!b.exclude && (tmax <  *b.min || tmin >  *b.max))
	    || (b.exclude && (tmin >= *b.min && tmax <= *b.max)))
	    return DXNewArrayV(t, c, rank, shape);
	    

	/* convert the data to scalar float if it is anything else.
	 *  this is the same routine the statistics code uses.
	 */
	if (!DXTypeCheck(a, TYPE_FLOAT, CATEGORY_REAL, 0)) {
	    if ((afloat = DXScalarConvert(a)) == NULL)
		return NULL;
	    
	    if(!(dp = DXGetArrayData(afloat)))
		goto error;
	    
	} else {
	    
	    if(!(dp = DXGetArrayData(a)))
		goto error;
	}
    } else {
	/* shape != 1, can't do simple rejection with statistics 
	 */
	if (!DXTypeCheck(a, TYPE_FLOAT, CATEGORY_REAL, 1, b.shape)) {
	    if ((afloat = DXArrayConvert(a, TYPE_FLOAT, CATEGORY_REAL,
					 1, b.shape)) == NULL)
		return NULL;
	    
	    if(!(dp = DXGetArrayData(afloat)))
		goto error;
	    
	} else {
	    
	    if(!(dp = DXGetArrayData(a)))
		goto error;
	}
    }


    id = 0;
    odp = DXGetArrayData(a);    /* this expands the input if it's regular */
    bytes = DXGetItemSize(a);
    na = DXNewArrayV(t, c, rank, shape);
    if (!odp || !na || bytes <= 0)
	goto error;

    /* now iterate through data.  if the data is in range copy it to the output
     *  array for include, out of range for exclude.
     */
    if (!b.exclude) {
	if (b.shape == 1) {
	    for (i=0, fp=(float *)dp; i<items; i++, fp++) {
		if (*fp >= *b.min && *fp <= *b.max)
		    DXAddArrayData(na, id++, 1, 
				   (Pointer)((char *)odp + i*bytes));
	    }
	} else {
	    for (i=0, fp=(float *)dp; i<items; i++, fp += b.shape) {
		for (j=0; j<b.shape; j++) {
		    if (*(fp+j) < *(b.min+j) || *(fp+j) > *(b.max+j))
			break;
		}
		if (j == b.shape)
		    DXAddArrayData(na, id++, 1, 
				   (Pointer)((char *)odp + i*bytes));
	    }
	}
    } else {  /* exclude */
	if (b.shape == 1) {
	    for (i=0, fp=(float *)dp; i<items; i++, fp++) {
		if (*fp < *b.min || *fp > *b.max)
		    DXAddArrayData(na, id++, 1, 
				   (Pointer)((char *)odp + i*bytes));
	    }
	} else {
	    for (i=0, fp=(float *)dp; i<items; i++, fp += b.shape) {
		for (j=0; j<b.shape; j++) {
		    if (*(fp+j) >= *(b.min+j) && *(fp+j) <= *(b.max+j))
			break;
		}
		if (j == b.shape)
		    DXAddArrayData(na, id++, 1, 
				   (Pointer)((char *)odp + i*bytes));
	    }
	}
    }

    return na;
    
  error:
    DXDelete((Object)afloat);
    DXDelete((Object)na);
    return NULL;
}


static Error VectorLength(Object o, int *len)
{
    int nitems;
    int rank, shape[100];
    Type t;
    Category c;

    if (DXGetObjectClass(o) != CLASS_ARRAY)
	return ERROR;
    
    if (!DXGetArrayInfo((Array)o, &nitems, &t, &c, &rank, shape))
	return ERROR;

    if (nitems > 1 || c != CATEGORY_REAL || rank >=2)
	return ERROR;

    if (rank == 0)
	shape[0] = 1;

    if (len)
	*len = shape[0];
    
    return OK;
}

static Error VectorExtract(Object o, int len, float *ptr)
{
    Array a = NULL;
    int i;
    float *fp;

    if (!(a = DXArrayConvert((Array)o, TYPE_FLOAT, CATEGORY_REAL,
			     1, len)))
	return ERROR;
    
    fp = (float *)DXGetArrayData(a);
    if (!fp)
	goto error;

    for (i=0; i<len; i++)
	*ptr++ = *fp++;
    
    DXDelete((Object)a);
    return OK;
    
  error:
    DXDelete((Object)a);
    return ERROR;
}
