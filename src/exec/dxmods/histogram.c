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
#include "histogram.h"

#define OUTPUT_TYPE_FLOAT 0   /* set to 1 to make float */

/* notes to me:
 * in making 2d, 3d and Nd histograms, the following changes are needed.
 *  the num bins, the min, max and ?in/out? flags need to be per dim now
 *  what is the median for a 2d histogram?  median per dim?  then it too
 *  must be a vector now.
 *
 * requested new function:  average bin value.  automatically created?
 * flag to ask for?  add a separate component named "bin average"?
 *
 * related function - user request to bin data values based on one
 * data value and get the average of a second data value - the shape
 * of the histogram will be based on the original data component, but
 * the other value can be marked and used to color the data or display
 * a graph, etc.  can this be done with a 2d input data, where the nbins
 * is normal for X and 1 for Y?  
 *
 * more distantly related function - sum across rows and columns of
 * Nd arrays.  stats as well?  can this be done with a macro which
 * slices data first then stats module?
 *
 */

/* just too long for me to type, sorry */
#define ICH InvalidComponentHandle   

/* for floating point number compares - this is a small number,
 *  but large enough to be a normalized floating point number.
 *  (denormalized numbers cause a floating point trap on the i860.)
 */                                              
#define FUZZ ((float) 1e-37) 

/* max supported is 32 dimensional */
#define MAXRANK 32

/* number of parallel tasks * (number of processors - 1) */
#define PFACTOR 4

/* only used when computing histogram groups in parallel */
struct parinfo {
    Object h;		/* the histogram itself */
    char *name;		/* the group member name */
    int didwork;	/* global copy of didwork flag */
    /* slicecnt here? */
};

/* values for the 'XXXset' flags below - 
 *  indicates whether or how the values were determined.
 */
#define NOTSET   0	/* no value yet */
#define EXPLICIT 1	/* user-given input parameter to module */
#define BYOBJECT 2	/* statistics of input object used */

/* parm list */
struct histinfo {
    Object o;		/* use data component of this object */
    int dim;            /* dimensionality of data/histogram */
    int bins;		/* number of histogram bins */
    int *md_bins;       /* list, used instead of bins if dim > 1 */
    int binset;         /* see flag values above */
    float min;		/* value of minimum of first bin */
    float *md_min;      /* list for dim > 1 */
    int minset;         /* flag */
    float max;		/* value of maximum of last bin */
    float *md_max;      /* list for dim > 1 */
    int maxset;         /* flag */
    int inoutlo;	/* what to do with data below first bin */
    int *md_inoutlo;    /* list for dim > 1 */
    int inoutloset;     /* flag */
    int inouthi;	/* what to do with data above last bin */
    int *md_inouthi;    /* list for dim > 1 */
    int inouthiset;     /* flag */
    int didwork;	/* whether any points fell into any bins */
    float median;	/* the approximate data median */
    float *md_median;   /* vector for dim > 1 */
    int maxparallel;    /* max number of parallel tasks; based on nproc */
    int goneparallel;	/* number of parallel tasks already spawned */
    ICH invalid;        /* if set, there are invalid data values */
    struct parinfo *p;	/* if computing in parallel, where to put the output */
};


/* things which need a list of values, one per dim
 */
struct perdim {
    int *binsplus1;    /* dim */
    float *binsize;
    int *thisbin;
    float *dminlist;
    float *dmaxlist;
    int **slicecnt;
    float *deltas;     /* dim * dim */
};


/* private routines */
static Error Histogram_Wrapper(Pointer);
static Error Histogram_Wrapper2(Pointer);
static Object Object_Histogram(struct histinfo *hp);
static Group Group_Histogram(struct histinfo *hp);
static Field Field_Histogram(struct histinfo *hp);
static Field Array_Histogram(struct histinfo *hp);
static Field EmptyHistogram(struct histinfo *hp);
static Field HistogramND(struct histinfo *hp, int empty);
static Error HasInvalid(Field f, char *, ICH *ih);
static int GetVectorShape(int rank, int *shape);
static Error VectorLength(Object o, int *len, int onlyone);
static Error VectorAllocAndExtract(Object o, Type expected_t, int len, Pointer *ptr);
static Error CopyHistinfo(struct histinfo *src, struct histinfo *dst);
static void FreeHistinfo(struct histinfo *hp);
static Error ObjectVectorStats(Object o, int *vlen, float **minlist, float **maxlist);
static Error VectorStats(Object o, int knownlength, int *vlen, 
			 int initialize, float **minlist, float **maxlist);
static Error init_perdim(struct perdim **pdp, int dim);
static Error bins_perdim(struct perdim *pdp, int dim, int *binsize);
static void free_perdim(struct perdim *pdp, int dim);
static Error FullAllocHistinfo(struct histinfo *hp, int datadim, int setdef);

#if 0
static Field EmptyHistogram1D(struct histinfo *hp);
static Error VectorExtract(Object o, Type expected_t, int len, Pointer ptr);
static Error ScalarAlloc(Pointer inval, Type t, int len, Pointer *retval);
#endif

#define INTERNALERROR DXSetError(ERROR_INTERNAL, \
		      "unexpected error, file %s line %d",  __FILE__, __LINE__)

#define DEFAULTBINS 100


/* 0 = input
 * 1 = bins
 * 2 = min 
 * 3 = max 
 * 4 = in/out
 *
 * 0 = output
 * 1 = median
 */
int
m_Histogram(Object *in, Object *out)
{
    struct histinfo h;
    float tmax=0;
    int multidim;
    int prevdim;
    int i;
    
    /* the idea here is to look at each of the input parms.
     * if they are all integers, this is a simple, 1d histogram.
     * if any are vector (number lists), then all the ones which
     * are vectors must match the vector length of the data component.
     * any which are still given as scalars are ok and apply to all
     * the dimensions.
     */
    
    
    /* initialize histinfo struct to zero, assume 1d and set defaults. */
    memset((char *)&h, '\0', sizeof(h));
    h.dim = 1;
    h.bins = DEFAULTBINS;
    h.min = 0.0;
    h.max = 1.0;
    h.inoutlo = 0;
    h.inouthi = 0;
    h.median = 0.0;
    h.maxparallel = DXProcessors(0);
    if (h.maxparallel > 1)
	h.maxparallel = (h.maxparallel-1) * PFACTOR;

    prevdim = 1;

    /* input object to histogram */
    if (!in[0]) {
	DXSetError(ERROR_BAD_PARAMETER, "#10000", "input");
	return ERROR;
    } 
    else
	h.o = in[0];
    

    /* number of bins */
    if (in[1]) {
	/* simple integer */
	if (DXExtractInteger(in[1], &h.bins)) {
	    if (h.bins <= 0) {
		DXSetError(ERROR_BAD_PARAMETER, "#10525", "number of bins");
		return ERROR;
	    }
	    h.binset = EXPLICIT;
	    
        } else if (VectorLength(in[1], &multidim, 1)) {
	    /* vector list? */
	    if (!VectorAllocAndExtract(in[1], TYPE_INT, multidim, 
				       (Pointer *)&h.md_bins))
		goto error;
	    
	    for (i=0; i<multidim; i++) {
		if (h.md_bins[i] <= 0) {
		    DXSetError(ERROR_BAD_PARAMETER, "#10525", 
			       "number of bins");
		    goto error;
		}
	    }
	    prevdim = multidim;
	    h.binset = EXPLICIT;
	    
	} else if ( 0 ) {
	    /* if they pass an object in for number of bins, what does
             *  that mean?  maybe this is bogus?  this code is currently
	     *  unused (if (0) never executes) but here to remind me to
	     *  look at this sometime.
	     */

	    /* if datatype == BYTE, bins = range, else bins = 100? */
	    h.bins = 256;   /* max - min (which we don't know yet); */
	    h.binset = BYOBJECT;

	} else {
	    DXSetError(ERROR_BAD_PARAMETER, "#10525", "number of bins");
	    goto error;
	}

    }


    /* minimum bin value.  either an explicit number, or a list of numbers,
     *  or an object whose data min will be used.  like autocolor, if min
     *  is set by an object but not max, max is also set by that object.
     */
    if (in[2]) {
	if (DXExtractFloat(in[2], &h.min)) {
	    h.minset = EXPLICIT;
	    
	} else if (VectorLength(in[2], &multidim, 1)) {
	    /* vector list? */
	    if (!VectorAllocAndExtract(in[2], TYPE_FLOAT, multidim, 
				       (Pointer *)&h.md_min))
		goto error;
	    
	    /* if an earlier parm has already set the length of the list,
	     * make sure this one matches in size.  if not, set the length
	     * here and take care of replicating the values for earlier 
	     * parms later.
	     */
	    if (prevdim > 1 && prevdim != multidim) {
		DXSetError(ERROR_BAD_PARAMETER, "#10085", "bin minimum", prevdim);
		goto error;
	    }
	    
	    if (prevdim == 1)
		prevdim = multidim;
	    
	    h.minset = EXPLICIT;
	    
	} else if (ObjectVectorStats(in[2], &multidim, &h.md_min, &h.md_max)) {
	    if (prevdim > 1 && prevdim != multidim) {
		DXSetError(ERROR_BAD_PARAMETER, "#10085", "bin minimum", prevdim);
		goto error;
	    }
	    
	    if (prevdim == 1)
		prevdim = multidim;
	    
	    h.minset = BYOBJECT;
	} else {
	    DXSetError(ERROR_BAD_PARAMETER, "#10562", "bin mininum");
	    goto error;
	}
    } 


    /* max bin value.  either an explicit number, an list, or object whose
     *  data maximum will be used, or the data maximum from the previous
     *  parameter if parm 2 was an object and parm 3 is NULL.
     */
    if (in[3]) {
	if (DXExtractFloat(in[3], &h.max)) {
	    h.maxset = EXPLICIT;
	    
	} else if (VectorLength(in[3], &multidim, 1)) {
	    /* vector list? */
	    if (!VectorAllocAndExtract(in[3], TYPE_FLOAT, multidim, 
				       (Pointer *)&h.md_max))
		goto error;
	    
	    /* if an earlier parm has already set the length of the list,
	     * make sure this one matches in size.  if not, set the length
	     * here and take care of replicating the values for earlier 
	     * parms at the end.
	     */
	    if (prevdim > 1 && prevdim != multidim) {
		DXSetError(ERROR_BAD_PARAMETER, "#10085", "bin maximum", prevdim);
		goto error;
	    }
	    
	    if (prevdim == 1)
		prevdim = multidim;

	    h.maxset = EXPLICIT;
	    
	} else if (ObjectVectorStats(in[3], &multidim, NULL, &h.md_max)) {
	    if (prevdim > 1 && prevdim != multidim) {
		DXSetError(ERROR_BAD_PARAMETER, "#10085", "bin maximum", prevdim);
		goto error;
	    }
	    
	    if (prevdim == 1)
		prevdim = multidim;
	    
	    h.maxset = BYOBJECT;
	} else {
	    DXSetError(ERROR_BAD_PARAMETER, "#10562", "bin maximum");
	    goto error;
	}
    } 
    else if (h.minset == BYOBJECT) {
	h.max = tmax;
	h.maxset = BYOBJECT;
    } 
    
    
    /* whether to ignore counts outside the bins, or add them to the
     *  first and last bins.  if using the data statistics for the
     *  bin max, set inouthi flag to 1, so data equal to the data max
     *  will be included in the last bin.
     */
    if (in[4]) {
	
	if (DXExtractInteger(in[4], &h.inoutlo)) {
	    if (h.inoutlo != 0 && h.inoutlo != 1) {
		DXSetError(ERROR_BAD_PARAMETER, "#10070", "outlier flag");
		goto error;
	    }
	    
	    h.inouthi = h.inoutlo;
	    h.inoutloset = EXPLICIT;
	    h.inouthiset = EXPLICIT;
	    
        } else if (VectorLength(in[4], &multidim, 1)) {
	    /* vector list? */
	    if (!VectorAllocAndExtract(in[4], TYPE_INT, multidim, 
				       (Pointer *)&h.md_inoutlo))
		goto error;
	    
	    for (i=0; i<multidim; i++) {
		if (h.md_inoutlo[i] != 0 && h.md_inoutlo[i] != 1) {
		    DXSetError(ERROR_BAD_PARAMETER, "#10075", "outlier flag");
		    goto error;
		}
	    }
	    
	    /* extract again into hi flag */
	    if (!VectorAllocAndExtract(in[4], TYPE_INT, multidim, 
				       (Pointer *)&h.md_inouthi))
		goto error;
	    
	    /* if an earlier parm has already set the length of the list,
	     * make sure this one matches in size.  if not, set the length
	     * here and take care of replicating the values for earlier 
	     * parms at the end.
	     */
	    if (prevdim > 1 && prevdim != multidim) {
		DXSetError(ERROR_BAD_PARAMETER, "#10076", "outlier flag", prevdim);
		goto error;
	    }
	    
	    if (prevdim == 1)
		prevdim = multidim;
	    
	    h.inoutloset = EXPLICIT;
	    h.inouthiset = EXPLICIT;
	    
	} else {
	    DXSetError(ERROR_BAD_PARAMETER, "#10075", "outlier flag");
	    goto error;
	}
	
    }
    else if (!h.maxset || h.maxset == BYOBJECT) {
	h.inouthi = 1;
	h.inouthiset = BYOBJECT;
    }
    
    multidim = prevdim;
    h.dim = multidim;
    
    if (!FullAllocHistinfo(&h, multidim, 1))
	goto error;
    
    
    /* one last error check - is max smaller or equal to min? */
    if (h.minset && h.maxset) {
	if (h.dim == 1) {
	    if (h.md_min[0] >= h.md_max[0]) {
		DXSetError(ERROR_BAD_PARAMETER, "#10185", "min", "max");
		goto error;
	    }
	} else {
	    for (i=0; i<h.dim; i++)
		/* check min vs corresponding max */
		if (h.md_min[i] >= h.md_max[i]) {
		    DXSetError(ERROR_BAD_PARAMETER, "#10187", 
			       "min", i, "max", i);
		    goto error;
		}
	}
    }
    
    
    
    
    /* do the work here */
    out[0] = Object_Histogram(&h);
    
    
    
    
    /* if no error and the output is one histogram (as opposed to a group
     *  of histograms), set the second output to the estimated median.
     * 
     *  it is inaccurate in proportion to the bin width. 
     */
    if (out[0] && DXGetObjectClass(out[0]) == CLASS_FIELD) {
	if (h.dim == 1)
	    out[1] = (Object)DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 0);
	else
	    out[1] = (Object)DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, h.dim);
	DXAddArrayData((Array)out[1], 0, 1, (Pointer)h.md_median);
    } else
	out[1] = NULL;
    
    /* it would be nice to know if the second output is being used,
     *  because then i'd issue a warning if the input was a group and
     *  the output had no median because of that.  but a warning every
     *  time seems annoying.
     */
     

    /* is it an error if the data is all out of range?  and if not,
     *  then what is the median?  i'm currently returning NULL for
     *  median, but a field for histogram, and not setting error.
     */

    if (out[0] && h.didwork == 0)
	DXWarning("no data values in histogram range");

    FreeHistinfo(&h);
    return (out[0] ? OK : ERROR);

 error:
    FreeHistinfo(&h);
    out[0] = out[1] = NULL;
    return(ERROR);
}


/* general histogram routine.  not used in m_Histogram; it just packages
 *  the parms into a block and calls the internal routine to make it
 *  easier for other libDX users to call the routine.
 */
Object
_dxfHistogram(Object o, int bins, float *min, float *max, int inout)
{
    struct histinfo h;
    Object retval = NULL;
    
    memset((char *)&h, '\0', sizeof(h));
    
    h.o = o;
    h.dim = 1;

    if (bins > 0) {
	h.bins = bins;
	h.binset = EXPLICIT;
    }
    if (min) {
	h.min = *min;
	h.minset = EXPLICIT;
    }
    if (max) {
	h.max = *max;
	h.maxset = EXPLICIT;
    }
    h.inoutlo = inout;
    h.inoutloset = EXPLICIT;
    
    /* what about !max here ? */
    h.inouthi = inout;
    h.inouthiset = EXPLICIT;
    
    if (!FullAllocHistinfo(&h, 1, 1))
	goto done;
    
    retval = Object_Histogram(&h);
    
  done:
    FreeHistinfo(&h);
    return retval;
}


/* if the object is a Series, CompositeField or Field, make one histogram
 *  from it.  if it is a general Group, make one histogram for each member.
 */
static Object
Object_Histogram(struct histinfo *hp)
{
    Object subo, old, cpy;

    switch(DXGetObjectClass(hp->o)) {
      case CLASS_ARRAY:
	return (Object)Array_Histogram(hp);

      case CLASS_FIELD:
	return (Object)Field_Histogram(hp);

      case CLASS_GROUP:
	switch(DXGetGroupClass((Group)hp->o)) {
	  case CLASS_COMPOSITEFIELD:
            /* have to make a copy here */
	    cpy = DXCopy(hp->o, COPY_STRUCTURE);
	    if (!cpy)
		return NULL;

	    old = hp->o;
	    hp->o = cpy;

	    if (!DXInvalidateDupBoundary(hp->o)) {
		/* delete copy and restore original object */
		DXDelete(hp->o);
		hp->o = old;
		return NULL;
	    }

	    subo = (Object)Field_Histogram(hp);

	    DXDelete(cpy);
	    hp->o = old;
	    return subo;

	  case CLASS_MULTIGRID:
	  case CLASS_SERIES:
	    return (Object)Field_Histogram(hp);

	  default:
	    return (Object)Group_Histogram(hp);
	}
	
      case CLASS_SCREEN:
	if (!DXGetScreenInfo((Screen)hp->o, &subo, NULL, NULL))
	    return NULL;

	hp->o = subo;
	return Object_Histogram(hp);

      case CLASS_XFORM:
	if (!DXGetXformInfo((Xform)hp->o, &subo, NULL))
	    return NULL;

	hp->o = subo;
	return Object_Histogram(hp);

      case CLASS_CLIPPED:
	if (!DXGetClippedInfo((Clipped)hp->o, &subo, NULL))
	    return NULL;

	hp->o = subo;
	return Object_Histogram(hp);

      default:
	DXSetError(ERROR_BAD_PARAMETER, "#10190", "input");
	return NULL;
    }

    /* not reached */
}

/* the input is a generic group;  call Histogram for each member.
 */
static Group
Group_Histogram(struct histinfo *hp)
{
    Group g;
    Group newg = NULL;
    Object subo;
    Object h;
    char *name;
    int i, members;
    struct parinfo *pinfo = NULL;

    /* handle empty groups, and groups of only 1 member 
     */
    g = (Group)hp->o;
    for (members=0; (subo = DXGetEnumeratedMember(g, members, &name)); members++)
	;

    if (members == 0)
	return (Group)EmptyHistogram(hp);

    newg = (Group)DXCopy(hp->o, COPY_HEADER);
    if (!newg)
	return NULL;
	
    if (members == 1) {
	subo = DXGetEnumeratedMember(g, 0, &name);
	if (!subo)
	    goto error;
	
	hp->o = subo;
	h = Object_Histogram(hp);
	if (!h)
	    goto error;
	
	/* if the member is named, add it by name, else add by number */
	if (! (name ? DXSetMember(newg, name, h)
	            : DXSetEnumeratedMember(newg, 0, h)))
	    goto error;

	return newg;
    }


    /* if we have groups nested in groups, make sure we don't start
     *  too many parallel tasks.
     */
    if (hp->goneparallel > hp->maxparallel) {
	for (i=0; (subo = DXGetEnumeratedMember(g, i, &name)); i++) {
	    
	    hp->o = subo;
	    h = Object_Histogram(hp);
	    if (!h)
		goto error;
	    
	    /* if the member is named, add it by name, else add by number */
	    if (! (name ? DXSetMember(newg, name, h)
		        : DXSetEnumeratedMember(newg, i, h)))
		goto error;
	    
	}
	return newg;
    }


    /* for each member, construct a histogram in parallel, and
     *  add them to the group later.
     */

    pinfo = (struct parinfo *)DXAllocateZero(members * sizeof(struct parinfo));
    if (!pinfo)
	goto error;
    
    if (!DXCreateTaskGroup())
	goto error;

    for (i=0; (subo = DXGetEnumeratedMember(g, i, &name)); i++) {
	
	hp->o = subo;
	hp->p = &pinfo[i];
	hp->goneparallel++;

	if (!DXAddTask(Histogram_Wrapper, 
		     (Pointer)hp, sizeof(struct histinfo), 0.0))
	    goto error;
	
    }

    if (!DXExecuteTaskGroup())
	goto error;

    hp->goneparallel -= members;

    /* now add accumulated histograms to the parent group */
    for (i=0; i < members; i++) {
	
	/* if the member is named, add it by name, else add by number */
	if (! (pinfo[i].name ? DXSetMember(newg, pinfo[i].name, pinfo[i].h)
	                     : DXSetEnumeratedMember(newg, i, pinfo[i].h)))
	    goto error;

	hp->didwork += pinfo[i].didwork;
    }

    DXFree((Pointer)pinfo);
    return newg;

  error:
    DXDelete((Object)newg);
    DXFree((Pointer)pinfo);
    return NULL;
}

/* for going parallel.  the wrapper routine which calls Object_Histogram
 *  and puts the results in the right place.
 */
static Error
Histogram_Wrapper(Pointer a)
{
    struct histinfo *hp = (struct histinfo *)a;
    struct histinfo h2;

    if (!CopyHistinfo(hp, &h2))
	return ERROR;

    hp->p->h = Object_Histogram(&h2);

    if (!hp->p->h)
	goto error;

    hp->p->didwork += h2.didwork;

    /* DXFreeInvalidComponentHandle(hp->invalid); */  /* not needed here? */
    FreeHistinfo(&h2);
    return OK;

  error:
    FreeHistinfo(&h2);
    return ERROR;
}

/* for going parallel.  the wrapper routine which calls Array_Histogram
 *  and puts the results in the right place.
 */
static Error
Histogram_Wrapper2(Pointer a)
{
    struct histinfo *hp = (struct histinfo *)a;
    struct histinfo h2;

    if (!CopyHistinfo(hp, &h2))
	return ERROR;

    hp->p->h = (Object)Array_Histogram(&h2);

    if (!hp->p->h) 
	goto error;

    hp->p->didwork += h2.didwork;
    
    DXFreeInvalidComponentHandle(hp->invalid);
    FreeHistinfo(&h2);
    return OK;

  error:
    FreeHistinfo(&h2);
    return ERROR;
}


/* this routine takes case of simple fields, and composite fields.  in
 *  both cases, the output is 1 histogram in a simple field.
 *
 *  the code goes parallel on partitioned data.
 */
static Field 
Field_Histogram(struct histinfo *hp)
{
    Object subo;
    Array ao, ao1;
    Type type;
    Group g;
    struct parinfo *pinfo = NULL;
    Field *f = NULL;
    Field h = NULL;
    int isparallel = 0;
    int items;
    int multidim;
    int members, i, j;
    int totalcnt;
#if OUTPUT_TYPE_FLOAT
    float *accum, *add;
#else
    int *accum, *add;
#endif
    float binsize;
    float gather, median;

    
    /* simple fields - histogram the data component */
    if (DXGetObjectClass(hp->o) == CLASS_FIELD) {
	
	/* ignore empty fields/partitions */
	if (DXEmptyField((Field)hp->o))
	    return EmptyHistogram(hp);
	
	ao = (Array)DXGetComponentValue((Field)hp->o, "data");
	if (!ao) {
	    /* no data component */
	    DXSetError(ERROR_MISSING_DATA, "#10240", "data");
	    return NULL;
	}

	/* this has to happen here while o is still the field */
	if (!HasInvalid((Field)hp->o, "data", &hp->invalid))
	    return NULL;

	hp->o = (Object)ao;
	subo = (Object)Array_Histogram(hp);

	DXFreeInvalidComponentHandle(hp->invalid);
	return (Field)subo;
    }
    
    /* not a simple field - a composite field or series.  process each
     *  member in parallel and sum them at the end.
     */

    /* count the members, and return if none */
    if (!DXGetMemberCount((Group)hp->o, &members) || (members == 0))
	return EmptyHistogram(hp);


    /* the default min and max bin values must be for the entire group.
     *  set them here if they aren't already set.
     */
    if (!hp->minset || !hp->maxset) {
	if (!hp->minset && !hp->maxset) {
	    if (!ObjectVectorStats(hp->o, &multidim, &hp->md_min, &hp->md_max))
		return NULL;
	    hp->minset = BYOBJECT;
	    hp->maxset = BYOBJECT;
	}
        if (!hp->minset) {
	    if (!ObjectVectorStats(hp->o, &multidim, &hp->md_min, NULL))
		return NULL;
	    hp->minset = BYOBJECT;
	}
	if (!hp->maxset) {
	    if (!ObjectVectorStats(hp->o, &multidim, NULL, &hp->md_max))
		return NULL;
	    hp->maxset = BYOBJECT;
	}
	if ((hp->dim != multidim) && !FullAllocHistinfo(hp, multidim, 0))
	    return NULL;
    }
    
    /* if the binsize hasn't been set yet, base it on the data type.
     *  byte data default is data range (max 256); everything else is 100 bins.
     */
    if (!hp->binset) {
	if (DXGetType(hp->o, &type, NULL, NULL, NULL) == NULL)
	    return NULL;
	
	for (i=0; i<hp->dim; i++)
	    hp->md_bins[i] = (type != TYPE_UBYTE) ? DEFAULTBINS : 
		hp->md_max[i] - hp->md_min[i] + 1;
	
	hp->binset = BYOBJECT;
    }
    
    /* allocate space to hold the array of histogram fields. 
     * we are going to use the first field to return the data, and
     *  extract the data from the others and add it in and then
     *  discard the rest of the fields.
     */
    pinfo = (struct parinfo *)DXAllocateZero(members * sizeof(struct parinfo));
    if (!pinfo)
	return NULL;

    /* below here, goto error instead of just returning */
    
    f = (Field *)DXAllocateZero(members * sizeof(Field));
    if (!f)
        goto error;
    
    /* iterate thru the members - this can happen in parallel.
     */
    if (hp->goneparallel < hp->maxparallel) {
	if (!DXCreateTaskGroup())
	    goto error;
	else
	    isparallel++;
    }

    g = (Group)hp->o;
    for (i=0; (subo = DXGetEnumeratedMember(g, i, NULL)); i++) {

	hp->o = subo;
	hp->p = &pinfo[i];

	/* if not a field, histogram it with the higher level routine.
	 *  what if it's a group????
	 */
        if (DXGetObjectClass(subo) != CLASS_FIELD) {
	    if (isparallel) {
		if (!DXAddTask(Histogram_Wrapper,
			     (Pointer)hp, sizeof(struct histinfo), 0.0))
		    goto error;
		hp->goneparallel++;
	    } else {
		f[i] = (Field)Object_Histogram(hp);
		if (DXGetError() != ERROR_NONE)
		    goto error;
		
	    }
	    continue;
	}

        /* ignore empty fields */
        if (DXEmptyField((Field)subo))
            continue;
	
        ao = (Array)DXGetComponentValue((Field)subo, "data");
        if (!ao) { 
	    DXSetError(ERROR_MISSING_DATA, "#10240", "data");
	    /* no data component */
	    goto error;
	}

	if (!HasInvalid((Field)subo, "data", &hp->invalid))
	    goto error;
        hp->o = (Object)ao;

	if (isparallel) {
	    if (!DXAddTask(Histogram_Wrapper2,
			 (Pointer)hp, sizeof(struct histinfo), 0.0))
		goto error;
	    hp->goneparallel++;
	} else {
	    f[i] = Array_Histogram(hp);
	    
	    if (DXGetError() != ERROR_NONE)
		goto error;
	    
	    DXFreeInvalidComponentHandle(hp->invalid);
	}
    }
    
    if (isparallel) {
	if (!DXExecuteTaskGroup())
	    goto error;
	
	hp->goneparallel -= members;
	
	/* put histograms where they are expected to be */
	for (i=0; i < members; i++) {
	    f[i] = (Field)pinfo[i].h;
	    hp->didwork += pinfo[i].didwork;
	}
    }
    
    
    /* coalesce the bins into one histogram 
     */
    accum = NULL;
    totalcnt = 0;
    for (i=0; i < members; i++) {
        if (!f[i] || DXEmptyField(f[i]))
            continue;
       	
        ao1 = (Array)DXGetComponentValue(f[i], "data");
        if (!ao1) {
	    INTERNALERROR;
	    goto error;
	}
        
        /* use the data array from the first field to accumulate into */
        if (!accum) {

	    DXGetArrayInfo(ao1, &items, NULL, NULL, NULL, NULL);
#if OUTPUT_TYPE_FLOAT
            accum = (float *)DXGetArrayData(ao1);
#else
            accum = (int *)DXGetArrayData(ao1);
#endif
            if (!accum) {
		INTERNALERROR;
		goto error;
	    }
            
            /* we will accumulate the total histogram into this field.
             *  continue here so we don't accumulate this field twice.
             */
            h = f[i];
            for (j=0; j < items; j++)
                totalcnt += accum[j];

	    /* XXX md - can't recompute totalcnt like this.  need slicecnt
             * for Nd median.
	     */
            
            continue;
        }

#if OUTPUT_TYPE_FLOAT
        add = (float *)DXGetArrayData(ao1);
#else
        add = (int *)DXGetArrayData(ao1);
#endif
        if (!add) {
            INTERNALERROR;
	    goto error;
	}
        
        for (j=0; j < items; j++) {
            accum[j] += add[j];
            totalcnt += add[j];
	    /* XXX md - ditto totalcnt comment.  accum is ok */
        }

    }

    /* delete all but the accumulator field, and recompute the median */
    for (i=0; i < members; i++) {
        if (f[i] && f[i] != h) {
            DXDelete((Object)f[i]);
	    f[i] = NULL;
	}
    }

    if (!h) {
        DXSetError(ERROR_MISSING_DATA, "no histogram data accumulated");
	goto error;
    }
    
    /* if no counts in any bin, no median */
    if (totalcnt == 0)
	goto done;


    /* else, set it */

    if (hp->bins == 0) {
        INTERNALERROR;
	goto error;
    }


    /* XXX start median recalc */

    /* the code in this area is trying to set the binsize, and not
     *  dividing by zero during the process 
     */
    /* XXX md */
    if (fabs(hp->max - hp->min) < FUZZ)
	binsize = 1.0;
    else
	binsize = (hp->max - hp->min) / hp->bins;

    if (binsize == 0) {
        INTERNALERROR;
	goto error;
    }
    

    /* XXX md - have to do this per axis */
    gather = 0;
    for (i=0; i<hp->bins; i++) {
        if ((gather+accum[i]) >= (totalcnt/2.))
            break;
        gather += accum[i];
    }
    if (accum[i] == 0) {
        INTERNALERROR;
	goto error;
    } else
        median = (i*binsize)+hp->min + 
            ((((totalcnt/2.) - gather) / accum[i]) * binsize);

    hp->median = median;

    /* XXX end median calc */


  done:
    if (!DXEndField(h))
	goto error;

    DXFree((Pointer)pinfo);
    DXFree((Pointer)f);
    return h;

  error:
    for (i=0; i < members; i++) {
        if (f && f[i] && f[i] != h)
            DXDelete((Object)f[i]);
    }

    DXDelete((Object)h);
    DXFree((Pointer)pinfo);
    DXFree((Pointer)f);
    return NULL;
}


static Field 
Array_Histogram(struct histinfo *hp)
{
    return HistogramND(hp, 0);
}

static Field
EmptyHistogram(struct histinfo *hp)
{
    return HistogramND(hp, 1);
}




/* allocate and set up space for per/dim items
 */
static Error init_perdim(struct perdim **pdp, int dim)
{
    /* allocate zero so uninitialized pointers are NULL.
     */
    *pdp = (struct perdim *)DXAllocateZero(sizeof(struct perdim));
    if (! *pdp)
	return ERROR;
    
    
    if (((*pdp)->binsplus1 = (int *)DXAllocate(dim * sizeof(int))) == NULL)
	goto error;
    
    if (((*pdp)->binsize = (float *)DXAllocate(dim * sizeof(float))) == NULL)
	goto error;
    if (((*pdp)->thisbin = (int *)DXAllocate(dim * sizeof(int))) == NULL)
	goto error;
    if (((*pdp)->dminlist = (float *)DXAllocate(dim * sizeof(float))) == NULL)
	goto error;
    if (((*pdp)->dmaxlist = (float *)DXAllocate(dim * sizeof(float))) == NULL)
	goto error;
    if (((*pdp)->slicecnt = (int **)DXAllocateZero(dim * sizeof(int *))) == NULL)
	goto error;
    if (((*pdp)->deltas = (float *)DXAllocate(dim * dim * sizeof(float))) == NULL)
	goto error;
    
    return OK;


  error:
    free_perdim(*pdp, dim);
    return ERROR;
}

static Error bins_perdim(struct perdim *pdp, int dim, int *binsize)
{
    int i;

    for (i=0; i<dim; i++)
	if ((pdp->slicecnt[i] = (int *)DXAllocateZero(binsize[i] * sizeof(int)))
	    == NULL)
	    return ERROR;

    return OK;
}

static void free_perdim(struct perdim *pdp, int dim)
{
    int i;

    for (i=0; i<dim; i++)
	if (pdp->slicecnt[i] != NULL)
	    DXFree((Pointer)pdp->slicecnt[i]);
    
    DXFree((Pointer)pdp->slicecnt);
    DXFree((Pointer)pdp->binsplus1);
    DXFree((Pointer)pdp->binsize);
    DXFree((Pointer)pdp->thisbin);
    DXFree((Pointer)pdp->dminlist);
    DXFree((Pointer)pdp->dmaxlist);
    DXFree((Pointer)pdp->deltas); 
    DXFree((Pointer)pdp);
}

/* make sure all lists in histinfo struct are allocated to 
 * the proper length, and if the previous dim was 1 and datadim > 1,
 * reallocate and replicate the existing values.  if allocating
 * this for the first time, fill in the values which work if this 
 * is going to be an empty field.
 */
static Error FullAllocHistinfo(struct histinfo *hp, int datadim, int setdef)
{
    int dimchanged = 0;
    int i;

    if (hp->dim != datadim) {
	if (hp->dim == 1 && datadim > 1) {
	    hp->dim = datadim;
	    dimchanged++;
	} else {
	    INTERNALERROR;
	    return ERROR;
	}
    }

#define DOALLOC(var, varlist, type) \
    if (!varlist) { \
	varlist = (type *)DXAllocate(hp->dim * sizeof(type)); \
	if (!varlist) \
	    return ERROR; \
	if (setdef) \
	    for (i=0; i<hp->dim; i++) \
		varlist[i] = var; \
    } else if (dimchanged) { \
	varlist = (type *)DXReAllocate(varlist, hp->dim * sizeof(type)); \
	if (!varlist) \
	    return ERROR; \
        for (i=0; i<hp->dim; i++) \
	    varlist[i] = varlist[0]; \
    }


    DOALLOC(hp->bins, hp->md_bins, int);
    DOALLOC(hp->min, hp->md_min, float);
    DOALLOC(hp->max, hp->md_max, float);
    DOALLOC(hp->inoutlo, hp->md_inoutlo, int);
    DOALLOC(hp->inouthi, hp->md_inouthi, int);
    DOALLOC(hp->median, hp->md_median, float);

    return OK;
}

static int boundBinIndex(int index, int numbins)
{
	if(index < 0)
		return 0;
	else if(index >= numbins)
		return numbins-1;
	
	return index;
}

static Field
HistogramND(struct histinfo *hp, int empty)
{
    Field f = NULL;
    Array a = NULL;
    Array sa = NULL;
    Array data=NULL;
    float zero = 0.0;
    int i, j, k;
    int bintotal, totalcnt=0;
    int defbins = DEFAULTBINS;
    Type type;
    Category cat;
    int rank;
    int shape[MAXRANK];
    int vshape;
    int nitems;
    int exitearly;
    int index;
#if OUTPUT_TYPE_FLOAT
    float *hd = NULL;    /* histogram data, float */
#else
    int *hd = NULL;      /* histogram data, was float before */
#endif
    float *hsum = NULL;  /* sum of each histogram data bin */
    Pointer od = NULL;   /* original data */
    int accum;
    int outofrange;
    
    struct perdim *pd;


    /* set up parms 
     */
    if (!init_perdim(&pd, hp->dim))
	goto error;


  isempty:
    if (empty) {
	for (i=0; i<hp->dim; i++) {
	    if (!hp->binset)
		hp->md_bins[i] = defbins;
	    
	    if (!hp->minset)
		hp->md_min[i] = 0.0;
	    if (!hp->maxset)
		hp->md_max[i] = 1.0;
	}

    } else {

	if (DXGetObjectClass(hp->o) != CLASS_ARRAY) {
	    INTERNALERROR;
	    goto error;
	}
	data = (Array)hp->o;
	if (!DXGetArrayInfo(data, &nitems, &type, &cat, &rank, shape))
	    goto error;
	
	vshape = GetVectorShape(rank, shape);
	if (cat != CATEGORY_REAL || vshape < 0) {
	    DXSetError(ERROR_DATA_INVALID, "#10511", "data");
	    goto error;
	}

	/* if all user parms were scalar but the data turns out
         * to be vector, expand struct here.
	 */
	if (hp->dim == 1 && vshape != 1) {
	    if (!FullAllocHistinfo(hp, vshape, 1))
		goto error;
	    free_perdim(pd, 1);
	    if (!init_perdim(&pd, hp->dim))
		goto error;
	}

	if (nitems <= 0) {
	    empty++;
	    if (type == TYPE_UBYTE)
		defbins = 256;
	    goto isempty;
	}

	
	/* save these to allow early exit, and set parms if still not set */
	if (hp->dim == 1) {
	    if (!DXStatistics(hp->o, "data", pd->dminlist, pd->dmaxlist, 
			      NULL, NULL))
		goto error;
	} else {
	    if (!VectorStats((Object)data, 1, &hp->dim, 
			     1, &pd->dminlist, &pd->dmaxlist))
		goto error;
	}
	
	
	/* if the binsize hasn't been set yet, base it on the data type.  for
	 * byte data, default is data range (up to a max of 256); everything 
	 * else is 100 bins.
	 */
	for (i=0; i<hp->dim; i++) {
	    if (!hp->minset)
		hp->md_min[i] = pd->dminlist[i];
	    if (!hp->maxset)
		hp->md_max[i] = pd->dmaxlist[i];
	    
	    if (!hp->binset) {
		hp->md_bins[i] = (type != TYPE_UBYTE) ? DEFAULTBINS : 
		    hp->md_max[i] - hp->md_min[i] + 1;
	    }
	}

	if (!bins_perdim(pd, hp->dim, hp->md_bins))
	    goto error;

    }
    
    bintotal = 1;
    for (i=0; i<hp->dim; i++) {
	
	pd->binsplus1[i] = hp->md_bins[i] + 1;
	bintotal *= hp->md_bins[i];
	
	if (fabs(hp->md_max[i] - hp->md_min[i]) < FUZZ)
	    pd->binsize[i] = 1.0;    /* arbitrary number */
	else
	    pd->binsize[i] = (hp->md_max[i] - hp->md_min[i]) / hp->md_bins[i];
	
	for (j=0; j<hp->dim; j++) 
	    pd->deltas[i * hp->dim + j] = (i == j) ? pd->binsize[i] : 0.0;
    }
	
	
    /* create output field.
     */

    f = DXNewField();
    if (!f)
	goto error;
    
    /* array = makegridpos(dims, counts, origins, deltas); */
    if ((a = (Array)DXMakeGridPositionsV(hp->dim, pd->binsplus1,
					  (Pointer)hp->md_min, 
					  (Pointer)pd->deltas)) == NULL)
	goto error;
    
    if (!DXSetComponentValue(f, "positions", (Object)a))
	goto error;
    a = NULL;
    
    
    /* array = makegridconn(dims, counts); */
    if ((a = (Array)DXMakeGridConnectionsV(hp->dim, pd->binsplus1)) == NULL)
	goto error;
    
    if (!DXSetComponentValue(f, "connections", (Object)a))
	goto error;
    a = NULL;

    
    if (empty) {
	
	if (((a = (Array)DXNewConstantArray(bintotal, (Pointer)&zero,
					    TYPE_FLOAT, CATEGORY_REAL, 0)) == NULL)
	    ||  !DXSetStringAttribute((Object)a, "dep", "connections"))
	    goto error;
	
	if (!DXSetComponentValue(f, "data", (Object)a))
	    goto error;
	if (!DXSetComponentValue(f, "sum", (Object)a))
	    goto error;
	a = NULL;
	goto done;

    } 

    /* do the data bins here, plus median */
    
#if OUTPUT_TYPE_FLOAT
    if (((a = (Array)DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 0)) == NULL)
#else
    if (((a = (Array)DXNewArray(TYPE_INT, CATEGORY_REAL, 0)) == NULL)
#endif
	||  !DXSetStringAttribute((Object)a, "dep", "connections"))
	goto error;
	
    if (!DXAddArrayData(a, 0, bintotal, NULL))
	goto error;
#if OUTPUT_TYPE_FLOAT
    if ((hd = (float *)DXGetArrayData(a)) == NULL)
#else
    if ((hd = (int *)DXGetArrayData(a)) == NULL)
#endif
	goto error;
    for (i=0; i<bintotal; i++)
	hd[i] = 0;

    if (!DXSetComponentValue(f, "data", (Object)a))
	goto error;
    a = NULL;

    /* sum of each bin total */
    if (((sa = (Array)DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, hp->dim)) == NULL)
	||  !DXSetStringAttribute((Object)sa, "dep", "connections"))
	goto error;
	
    if (!DXAddArrayData(sa, 0, bintotal, NULL))
	goto error;
    if ((hsum = (float *)DXGetArrayData(sa)) == NULL)
	goto error;
    for (i=0; i<bintotal*hp->dim; i++)
	hsum[i] = 0.0;

    if (!DXSetComponentValue(f, "sum", (Object)sa))
	goto error;
    sa = NULL;



    /* things that allow the accumulation process to be shortcut:
     *  all the data is out of range of the bins
     *  all the data is the same value
     * 
     * for an Nd histogram, this has to be a separate loop from
     * the one which is used if any of the data is in range, because
     * this loop is per dimension, and the other loop is per item.
     * i could make a flag which says skip this dimension, and use
     * it in the per/item loop, but it seems like a small gain for 
     * a lot of code complexity.
     */

#define INDEX(hp, maxdim, thisindex) \
    index = 0; \
    for (j=1; j<maxdim; j++) { \
	index *= hp->md_bins[j]; \
	index += thisindex; \
    }

#define ADDSUM(type) \
    hsum[index*hp->dim + j] += (float)(((type *)od)[i*hp->dim + j]) 

    /* XXX md - fill in slicecnt for median calc if field is partitioned.
     */
    exitearly = 1;
    outofrange = 0;
    if (!hp->invalid) {
	for (i=0; i<hp->dim; i++) {
	    if (pd->dmaxlist[i] < hp->md_min[i]) {
		/* if all below min, put them in bin 0 if i/o flag set */
		if (hp->md_inoutlo[i]) {
		    INDEX(hp, i, 0);
		    hd[index] = nitems;
		    hp->md_median[i] = hp->md_min[i] + (0.5 * hp->md_bins[i]);
		    totalcnt = nitems;
		    pd->thisbin[i] = 0;
		} else
		    outofrange++;
		
	    } else if (pd->dminlist[i] > hp->md_max[i]) {
		/* if all above max, put in maxbin if i/o flag set */
		if (hp->md_inouthi[i]) {
		    INDEX(hp, i, hp->md_bins[i]-1);
		    hd[index] = nitems;
		    hp->md_median[i] = hp->md_max[i] - (0.5 * hp->md_bins[i]);
		    totalcnt = nitems;
		    pd->thisbin[i] = hp->md_bins[i]-1;
		} else
		    outofrange++;
		
	    } else if (pd->dminlist[i] == pd->dmaxlist[i]) {
		/* if all are the same, put them all into one bin enmass */
		pd->thisbin[i] = boundBinIndex( (int)((pd->dmaxlist[i] - hp->md_min[i]) / pd->binsize[i]), 
		    hp->md_bins[i]);
		INDEX(hp, i, pd->thisbin[i]);
		hd[index] = nitems;
		hp->md_median[i] = pd->dminlist[i];
		totalcnt = nitems;
	    } else {
		/* can't use this code, drop down and loop per item */
		exitearly = 0;
		break;
	    }
	}
	
	/* if all the data is in one bin or out of range, compute sum
	 * and then we're done.
	 */
	if (exitearly) {
	    if (outofrange == hp->dim)
		goto done;
	    
	    else if (outofrange == 0) {
			index = pd->thisbin[0];
			for (k=1; k<hp->dim; k++) {
		    	index *= hp->md_bins[k];
		    	index += pd->thisbin[k];
			}
		
		od = DXGetArrayData(data);
		for (i=0; i<nitems; i++) {
		    switch(type) {
		      case TYPE_UBYTE:
			for (j=0; j<hp->dim; j++) ADDSUM(ubyte); break;
		      case TYPE_BYTE:
			for (j=0; j<hp->dim; j++) ADDSUM(byte); break;
		      case TYPE_USHORT:
			for (j=0; j<hp->dim; j++) ADDSUM(ushort); break;
		      case TYPE_SHORT:
			for (j=0; j<hp->dim; j++) ADDSUM(short); break;
		      case TYPE_UINT:
			for (j=0; j<hp->dim; j++) ADDSUM(uint); break;
		      case TYPE_INT:
			for (j=0; j<hp->dim; j++) ADDSUM(int); break;
		      case TYPE_FLOAT:
			for (j=0; j<hp->dim; j++) ADDSUM(float); break;
		      case TYPE_DOUBLE:
			for (j=0; j<hp->dim; j++) ADDSUM(double); break;
		      default:
			DXSetError(ERROR_DATA_INVALID, "#10320", "histogram data");
			goto error;
		    }
		}
		goto done;
	    }
	    /* else fall through into the following code */
	}
    }


    
#define THISVAL(type) (((type *)od)[i*hp->dim + j])
#define THISBIN(type) (boundBinIndex((int)((THISVAL(type) - hp->md_min[j]) \
			/ pd->binsize[j]), hp->md_bins[j]))

    totalcnt = 0;
    od = DXGetArrayData(data);
    for (i=0; i<nitems; i++) {
	if (hp->invalid && DXIsElementInvalidSequential(hp->invalid, i))
	    continue;
	
	for (j=0; j<hp->dim; j++) {
	    
	    /* accumulate.  if bin value is outside data range, check inout
	     *  flag.  if not set, ignore those points.  if set, add points
	     *  to the first or last bin.  keep track of total pointcount 
	     *  for later computation of median.
	     */
	    
	    switch(type) {
	      case TYPE_UBYTE:
		pd->thisbin[j] = THISBIN(ubyte); break;
	      case TYPE_BYTE:
		pd->thisbin[j] = THISBIN(byte); break;
	      case TYPE_USHORT:
		pd->thisbin[j] = THISBIN(ushort); break;
	      case TYPE_SHORT:
		pd->thisbin[j] = THISBIN(short); break;
	      case TYPE_UINT:
		pd->thisbin[j] = THISBIN(uint); break;
	      case TYPE_INT:
		pd->thisbin[j] = THISBIN(int); break;
	      case TYPE_FLOAT:
		pd->thisbin[j] = THISBIN(float); break;
	      case TYPE_DOUBLE:
		pd->thisbin[j] = THISBIN(double); break;
	      default:
		DXSetError(ERROR_DATA_INVALID, "#10320", "histogram data");
		goto error;
	    }
	    
	    /* three cases: below, above, or within range */
	    if (pd->thisbin[j] < 0) {
		if (hp->md_inoutlo[j]) {
		    pd->thisbin[j] = 0;
		} else
		    break;
	    } else if (pd->thisbin[j] >= hp->md_bins[j]) {
		if (hp->md_inouthi[j]) {
		    pd->thisbin[j] = hp->md_bins[j]-1;
		} else
		    break;
	    }
	}

	/* did we exit early because at least one axis was out of range? */
	if (j != hp->dim)
	    continue;

	/* keep track of total counts per slice for each axis; this
         * is needed for median calc later.
	 */
	for (k=0; k<hp->dim; k++)
	    pd->slicecnt[k][pd->thisbin[k]]++;

	index = pd->thisbin[0];
	for (k=1; k<hp->dim; k++) {
	    index *= hp->md_bins[k];
	    index += pd->thisbin[k];
	}

	hd[index]++;
	totalcnt++;


	/* if the data is a scalar great, but otherwise it has to be
         * per component of the vector, which is more than a simple sum.
	 * the sum has to be indexed one more level, shape[0].
	 */
	switch(type) {
	  case TYPE_UBYTE:
	    for (j=0; j<hp->dim; j++) ADDSUM(ubyte); break;
	  case TYPE_BYTE:
	    for (j=0; j<hp->dim; j++) ADDSUM(byte); break;
	  case TYPE_USHORT:
	    for (j=0; j<hp->dim; j++) ADDSUM(ushort); break;
	  case TYPE_SHORT:
	    for (j=0; j<hp->dim; j++) ADDSUM(short); break;
	  case TYPE_UINT:
	    for (j=0; j<hp->dim; j++) ADDSUM(uint); break;
	  case TYPE_INT:
	    for (j=0; j<hp->dim; j++) ADDSUM(int); break;
	  case TYPE_FLOAT:
	    for (j=0; j<hp->dim; j++) ADDSUM(float); break;
	  case TYPE_DOUBLE:
	    for (j=0; j<hp->dim; j++) ADDSUM(double); break;
	  default:
	    DXSetError(ERROR_DATA_INVALID, "#10320", "histogram data");
	    goto error;
	}
    }
    
    
    /* compute median. 
     *  find bin which spans center value & interpolate
     */
    for (i=0; i<hp->dim; i++) {
	accum = 0;

	if (totalcnt == 0) {
	    hp->md_median[i] = 0.0;
	    continue;
	}
	    
	for (j=0; j<hp->md_bins[i]; j++) {
	    if (accum + pd->slicecnt[i][j] >= totalcnt/2.)
		break;
	    accum += pd->slicecnt[i][j];
	}
	if (pd->slicecnt[i][j] == 0) {
	    INTERNALERROR;
	    goto error;
	} else
	    hp->md_median[i] = (j*pd->binsize[i]) + hp->md_min[i] + 
	       (((totalcnt/2.) - accum) / pd->slicecnt[i][j] * pd->binsize[i]);
    }

    
  done:
    if (!DXEndField(f))
       goto error;
    
    hp->didwork += totalcnt;
    free_perdim(pd, hp->dim);
    return f;

  error:
    DXDelete((Object)f);
    DXDelete((Object)a);
    DXDelete((Object)sa);
    free_perdim(pd, hp->dim);
    return NULL;
    
}


static Error HasInvalid(Field f, char *component, ICH *ih)
{
    Error rc = OK;
    char *dep = NULL;
    char *invalid = NULL;

    *ih = NULL;

    if (!DXGetStringAttribute(DXGetComponentValue(f, component), "dep", &dep))
	return OK;
    
#define INVLEN 10     /* strlen("invalid "); */    
    
    if (!(invalid = (char *)DXAllocate(strlen(dep) + INVLEN)))
	return ERROR;

    strcpy(invalid, "invalid ");
    strcat(invalid, dep);

    /* if no component, return NULL */
    if (!DXGetComponentValue(f, invalid))
	goto done;

    *ih = DXCreateInvalidComponentHandle((Object)f, NULL, dep);
    if (!*ih) {
	rc = ERROR;
	goto done;
    }
    
  done:
    DXFree((Pointer)invalid);
    return rc;
}
    

static Error VectorLength(Object o, int *len, int onlyone)
{
    int nitems, value = 0;
    int rank, shape[100];
    Type t;
    Category c;

    if (DXGetObjectClass(o) != CLASS_ARRAY)
	return ERROR;
    
    if (!DXGetArrayInfo((Array)o, &nitems, &t, &c, &rank, shape))
	return ERROR;

    if (onlyone && nitems > 1)
	return ERROR;

    if (c != CATEGORY_REAL)
	return ERROR;

    /* see comment below about what this does */
    value = GetVectorShape(rank, shape);

    if (value < 0)
	return ERROR;

    if (len)
	*len = value;
    
    return OK;
}

/* if rank == 0, scalar.  vector length = 1.
 * if rank == 1, vector length = shape[0].
 * if rank > 1, error unless only one of the ranks has 
 *  a value > 1.
 *
 * returns -1 if error.
 */
static int GetVectorShape(int rank, int *shape)
{
    int i;
    int value = -1;
    
    switch (rank) {
      case 0:
	value = 1;
	break;

      case 1:
	value = shape[0];
	break;

      default:
	for (i=0; i<rank; i++)
	    if (shape[i] > 1) {
		if (value == -1)
		    value = shape[i];
		else
		    return -1;
	    }
	
	if (value == -1)
	    value = 1;

	break;
    }

    return value;
}


#if 0
static Error VectorExtract(Object o, Type expected_t, int len, Pointer d_ptr)
{
    Array a = NULL;
    int i;
    int datasize;
    Pointer s_ptr;

    if (!(a = DXArrayConvert((Array)o, expected_t, CATEGORY_REAL,
			     1, len)))
	return ERROR;
    
    datasize = DXTypeSize(expected_t);

    s_ptr = DXGetArrayData(a);
    if (!s_ptr)
	goto error;

    memcpy(d_ptr, s_ptr, datasize * len);
    
    DXDelete((Object)a);
    return OK;
    
  error:
    DXDelete((Object)a);
    return ERROR;
}
#endif

static Error VectorAllocAndExtract(Object o, Type expected_t, int len, Pointer *d_ptr)
{
    Array a = NULL;
    int datasize;
    Pointer s_ptr;

    if (!(a = DXArrayConvert((Array)o, expected_t, CATEGORY_REAL,
			     1, len)))
	return ERROR;
    
    datasize = DXTypeSize(expected_t);

    s_ptr = DXGetArrayData(a);
    if (!s_ptr)
	goto error;

    if ((*d_ptr = DXAllocate(datasize * len)) == NULL)
	goto error;

    memcpy(*d_ptr, s_ptr, datasize * len);
    
    DXDelete((Object)a);
    return OK;
    
  error:
    DXDelete((Object)a);
    return ERROR;
}

/* currently unused */
#if 0
static Error ScalarAlloc(Pointer inval, Type t, int len, Pointer *retval)
{
    int i;
    int datasize;
    Pointer p;

    datasize = DXTypeSize(t);
    
    if ((*retval = DXAllocate(datasize * len)) == NULL)
	return ERROR;

    p = *retval;
    for (i=0; i<len; i++) {
	memcpy(p, inval, datasize);
	/* the sgi compiler won't let me increment a void * */
	/* p += datasize; */
	p = (Pointer)((byte *)p + datasize);
    }
    
    return OK;
}
#endif


/* used for cleanup - free all storage associated with hist struct.
 */
static void FreeHistinfo(struct histinfo *hp)
{
    /* do not delete (hp->o) -- that is the input object which
     * we read but did not allocate and so should not free.
     * also, do not delete the contents of hp->p nor hp->invalid.
     * they aren't copied by the copy routine are are freed elsewhere.
     */
    
    if (hp->md_bins)
	DXFree((Pointer)hp->md_bins); 
    if (hp->md_min)
	DXFree((Pointer)hp->md_min); 
    if (hp->md_max)
	DXFree((Pointer)hp->md_max); 
    if (hp->md_inoutlo)
	DXFree((Pointer)hp->md_inoutlo); 
    if (hp->md_inouthi)
	DXFree((Pointer)hp->md_inouthi); 
    if (hp->md_median)
	DXFree((Pointer)hp->md_median); 

}

static Error CopyHistinfo(struct histinfo *src, struct histinfo *dst)
{
    /* first do a straight copy of the struct.  
     * then zero all the pointers so they don't point into the src lists.
     * then allocate new space and copy any lists.
     * (if you don't zero the pointers first, you can't do meaningful
     * cleanup on error.)
     */
    memcpy((char *)dst, (char *)src, sizeof(struct histinfo));
    dst->md_bins = NULL;
    dst->md_min = NULL;
    dst->md_max = NULL;
    dst->md_inoutlo = NULL;
    dst->md_inouthi = NULL;
    dst->md_median = NULL;
    
    if (src->md_bins) {
	dst->md_bins = (int *)DXAllocate(src->dim * sizeof(int));
	if (!dst->md_bins)
	    goto error;
	memcpy(dst->md_bins, src->md_bins, src->dim * sizeof(int));
    }
    if (src->md_min) {
	dst->md_min = (float *)DXAllocate(src->dim * sizeof(float));
	if (!dst->md_min)
	    goto error;
	memcpy(dst->md_min, src->md_min, src->dim * sizeof(float));
    }
    if (src->md_max) {
	dst->md_max = (float *)DXAllocate(src->dim * sizeof(float));
	if (!dst->md_max)
	    goto error;
	memcpy(dst->md_max, src->md_max, src->dim * sizeof(float));
    }
    if (src->md_inoutlo) {
	dst->md_inoutlo = (int *)DXAllocate(src->dim * sizeof(int));
	if (!dst->md_inoutlo)
	    goto error;
	memcpy(dst->md_inoutlo, src->md_inoutlo, src->dim * sizeof(int));
    }
    if (src->md_inouthi) {
	dst->md_inouthi = (int *)DXAllocate(src->dim * sizeof(int));
	if (!dst->md_inouthi)
	    goto error;
	memcpy(dst->md_inouthi, src->md_inouthi, src->dim * sizeof(int));
    }
    if (src->md_median) {
	dst->md_median = (float *)DXAllocate(src->dim * sizeof(float));
	if (!dst->md_median)
	    goto error;
	memcpy(dst->md_median, src->md_median, src->dim * sizeof(float));
    }
    return OK;

  error:
    FreeHistinfo(dst);
    return ERROR;
}

static Error
ObjectVectorStats(Object o, int *vlen, float **minlist, float **maxlist)
{
    int i;
    Object subo;

    switch (DXGetObjectClass(o)) {
      case CLASS_ARRAY:
	return VectorStats(o, 0, vlen, 1, minlist, maxlist);
	
      case CLASS_FIELD:
	return VectorStats(o, 0, vlen, 1, minlist, maxlist);
	
      case CLASS_GROUP:
	switch (DXGetGroupClass((Group)o)) {
	  case CLASS_MULTIGRID:
	  case CLASS_SERIES:
	  case CLASS_COMPOSITEFIELD:
	    /* i'm just going for min/max on my stats here,
	     * i can do that w/o worrying about boundary pts.
	     */
	    subo = DXGetEnumeratedMember((Group)o, 0, NULL);
	    if (VectorStats(subo, 0, vlen, 1, minlist, maxlist) == ERROR)
		return ERROR;
	    
	    for (i=1; (subo = DXGetEnumeratedMember((Group)o, i, NULL)); i++) {
		if (VectorStats(subo, 1, vlen, 0, minlist, maxlist) == ERROR)
		    return ERROR;
	    }
	    return OK;
	    
	  default:
	    return ERROR;
	}
	
      case CLASS_SCREEN:
	if (!DXGetScreenInfo((Screen)o, &subo, NULL, NULL))
	    return ERROR;

	return ObjectVectorStats(subo, vlen, minlist, maxlist);

      case CLASS_XFORM:
	if (!DXGetXformInfo((Xform)o, &subo, NULL))
	    return ERROR;

	return ObjectVectorStats(subo, vlen, minlist, maxlist);

      case CLASS_CLIPPED:
	if (!DXGetClippedInfo((Clipped)o, &subo, NULL))
	    return ERROR;

	return ObjectVectorStats(subo, vlen, minlist, maxlist);

      default:
	DXSetError(ERROR_BAD_PARAMETER, "#10190", "input");
	return ERROR;
    }

    /* not reached */
}



/* this routine is getting more complicated than it was originally
 * intended to be.  the first object can either be a simple field or
 * an array.  if a field, the routine operates on the data component.
 * if the knownlength flag is set, the *vlen must match the
 * array or it is an error.  if the flag isn't set, return the vector length
 * in *vlen.  if initialize is set, the min/max lists are initialized to
 * max/min floats, otherwise they must contain previous results and the
 * stats from this field are combined with them.  if one of minlist/maxlist
 * comes in as NULL, don't accumulate that value.  one must be non-null.
 * if either of *minlist or *maxlist are NULL, allocate space for stats.
 * if *minlist or *maxlist are already allocated, use the space which is there.
 */
static Error VectorStats(Object o, int knownlength, int *vlen, 
			 int initialize, float **minlist, float **maxlist)
{
    Array a = NULL, na = NULL;
    int i, j;
    int nitems;
    int domin = 1, domax = 1;
    int allocmin = 0, allocmax = 0;
    float *d_ptr;
    
    if (DXGetObjectClass(o) == CLASS_FIELD) {
	a = (Array)DXGetComponentValue((Field)o, "data");
	if (!a)
	    goto error;
    } else if (DXGetObjectClass(o) == CLASS_ARRAY) {
	a = (Array)o;
    } else {
	INTERNALERROR;
	goto error;
    }

    if (!knownlength && !VectorLength((Object)a, vlen, 0))
	goto error;

    if (minlist == NULL)
	domin = 0;
    if (maxlist == NULL)
	domax = 0;

    if (!domin && !domax)
	goto error;

    if (domin && (*minlist == NULL)) {
	if ((*minlist = (float *)DXAllocate(sizeof(float) * (*vlen))) == NULL)
	    goto error;
	allocmin++;
    }
    if (domax && (*maxlist == NULL)) {
	if ((*maxlist = (float *)DXAllocate(sizeof(float) * (*vlen))) == NULL)
	    goto error;
	allocmax++;
    }
    
    if (!(na = DXArrayConvert((Array)a, TYPE_FLOAT, CATEGORY_REAL, 1, *vlen)))
	return ERROR;
    
    d_ptr = (float *)DXGetArrayData(na);
    if (!d_ptr)
	goto error;
    
    if (!DXGetArrayInfo(na, &nitems, NULL, NULL, NULL, NULL))
	goto error;

    if (initialize)
	for (i=0; i<*vlen; i++) {
	    if (domin)
		(*minlist)[i] = DXD_MAX_FLOAT;
	    if (domax)
		(*maxlist)[i] = DXD_MIN_FLOAT;
	}

    for (i = 0; i < nitems; i++)
	for (j = 0; j < *vlen; j++) {
	    if (domin && (d_ptr[i * (*vlen) + j] < (*minlist)[j]))
		(*minlist)[j] = d_ptr[i * (*vlen) + j];
	    if (domax && (d_ptr[i * (*vlen) + j] > (*maxlist)[j]))
		(*maxlist)[j] = d_ptr[i * (*vlen) + j];
	}

    DXDelete((Object)na);
    return OK;
    
  error:
    for (i = 0; i < *vlen; i++) {
	if (domin)
	    (*minlist)[i] = DXD_MIN_FLOAT;
	if (domax)
	    (*maxlist)[i] = DXD_MAX_FLOAT;
    }

    if (allocmin)
	DXFree((Pointer)minlist);
    if (allocmax)
	DXFree((Pointer)maxlist);
    DXDelete((Object)na);
    return ERROR;
}



#if 0


/* old routine */    
static Field 
Array_Histogram1(struct histinfo *hp)
{
    Field f = NULL;
    Array ao = (Array)hp->o;
    Array ap = NULL;	        /* positions, connections, data, median */
    Array ac = NULL;
    Array ad = NULL;
    float *hd = NULL, *od = NULL; /* histogram data, original data */
    int osbins;                 /* oversample the bins? */
    int i, thisbin, nitems;
    int totalcnt;
    float accum;                /* this could be an integer */
    float binsize, median = 0;
    float binmin, binmax;
    float dmin, dmax;
    Point origin, delta;
    Type type;
    Category cat;
    int rank, shape[MAXRANK];



    if (!DXGetArrayInfo(ao, &nitems, &type, &cat, &rank, shape))
	goto error;
    
    if (cat != CATEGORY_REAL || (rank != 0 && (rank != 1 || shape[0] != 1))) {
	DXSetError(ERROR_DATA_INVALID, "#10080", "data");
	goto error;
    }
    
    if (nitems <= 0)
	goto error;
    
    /* save these to allow early exit, and set parms if still not set */
    if (!DXStatistics((Object)ao, "data", &dmin, &dmax, NULL, NULL))
	goto error;
    
    /* set bin range, and number of bins */
    binmin = hp->minset ? hp->min : dmin;
    binmax = hp->maxset ? hp->max : dmax;

    if (binmin > binmax) {
	DXSetError(ERROR_BAD_PARAMETER, "#10185", "min", "max");
	goto error;
    }
    
    /* if the binsize hasn't been set yet, base it on the data type.
     *  byte data default is data range (max 256); everything else is 100 bins.
     */
    if (!hp->binset) {
	if ((type != TYPE_UBYTE) || 
	    (rank != 0 && (rank != 1 || shape[0] != 1))) {
	    hp->bins = DEFAULTBINS;
	    hp->binset = BYOBJECT;
	} else {
	    hp->bins = binmax - binmin + 1;
	    hp->binset = BYOBJECT;
	}
    }

    osbins = hp->bins;    /* could be multiplier here to make smaller bins */
    if (osbins <= 0) {
	DXSetError(ERROR_DATA_INVALID, "#10020", "number of bins");
	goto error;
    }
    
    
    hd = (float *)DXAllocateLocalZero(osbins * sizeof(float));
    if (!hd)
	goto error;

    if (fabs(binmax - binmin) < FUZZ)
	binsize = 1.0;    /* arbitrary number */
    else
	binsize = (binmax - binmin) / osbins;

    if (binsize <= 0)
	goto error;

    origin.x = binmin;
    origin.y = origin.z = 0.0;
    delta.x = binsize;
    delta.y = delta.z = 0.0;
    
    /* things that allow the accumulation process to be shortcut:
     *  all the data is out of range of the bins
     *  all the data is the same value
     */
    
    totalcnt = 0;
    if (dmax < binmin) {
	if (hp->inoutlo) {
	    hd[0] = nitems;
	    totalcnt = nitems;
	    median = binmin + (0.5 * binsize);
	}
    } else if (dmin > binmax) {
	if (hp->inouthi) {
	    hd[osbins-1] = nitems;
	    totalcnt = nitems;
	    median = binmax - (0.5 * binsize);
	}
    } else if (dmin == dmax) {
	thisbin = (dmax - binmin) / binsize;
	if (thisbin >= 0 && thisbin < osbins) {
	    hd[thisbin] = nitems;
	    totalcnt = nitems;
	    median = dmin;
	} else if (thisbin < 0 && hp->inoutlo) {
	    hd[0] = nitems;
	    totalcnt = nitems;
	    median = binmin + (0.5 * binsize);
	} else if (thisbin >= osbins && hp->inouthi) {
	    hd[osbins-1] = nitems;
	    totalcnt = nitems;
	    median = binmax - (0.5 - binsize);
	}
    } else {
	od = (float *)DXGetArrayData(ao);
	if (!od)
	    goto error;

	/* accumulate.  if bin value is outside data range, check inout
	 *  flag.  if not set, ignore those points.  if set, add points
	 *  to the first or last bin.  keep track of total pointcount 
	 *  for later computation of median.
	 */
	totalcnt = 0;
	
#define THISBIN1(type) (((type *)od)[i] - binmin) / binsize
	
	for (i=0; i<nitems; i++) {
	    if (hp->invalid && DXIsElementInvalidSequential(hp->invalid, i))
		continue;
	    
	    switch(type) {
	      case TYPE_UBYTE:
		thisbin = THISBIN1(ubyte); break;
	      case TYPE_BYTE:
		thisbin = THISBIN1(byte); break;
	      case TYPE_USHORT:
		thisbin = THISBIN1(ushort); break;
	      case TYPE_SHORT:
		thisbin = THISBIN1(short); break;
	      case TYPE_UINT:
		thisbin = THISBIN1(uint); break;
	      case TYPE_INT:
		thisbin = THISBIN1(int); break;
	      case TYPE_FLOAT:
		thisbin = THISBIN1(float); break;
	      case TYPE_DOUBLE:
		thisbin = THISBIN1(double); break;
	      default:
		DXSetError(ERROR_DATA_INVALID, "#10320", "histogram data");
		goto error;
	    }

	    /* three cases: below, above, or within range */
	    if (thisbin < 0) {
		if (hp->inoutlo) {
		    hd[0]++;
		    totalcnt++;
		} /* else ignore */
	    } else if (thisbin >= osbins) {
		if (hp->inouthi) {
		    hd[osbins-1]++;
		    totalcnt++;
		} /* else ignore */
	    } else {
		hd[thisbin]++;
		totalcnt++;
	    }
	}

	/* compute median. 
	 *  find bin which spans center value & interpolate
	 */
	if (totalcnt != 0) {
	    accum = 0;
	    for (i=0; i<osbins; i++) {
		if (accum + hd[i] >= totalcnt/2.)
		    break;
		accum += hd[i];
	    }
	    if (hd[i] == 0) {
		INTERNALERROR;
		goto error;
	    } else
		median = (i*binsize)+binmin + 
		    ((((totalcnt/2.) - accum) / hd[i]) * binsize);
	}
    }
	
    
    /* create output field
     */

    f = DXNewField();
    if (!f)
	goto error;

    if (((ad = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 0)) == NULL)
	||  !DXAddArrayData(ad, 0, osbins, (Pointer)hd)
	||  !DXSetStringAttribute((Object)ad, "dep", "connections"))
	goto error;
    
    if (totalcnt > 0)
	hp->median = median;
    
    if (((ac = (Array)DXNewPathArray(osbins+1)) == NULL)
	||  !DXSetStringAttribute((Object)ac, "ref", "positions")
	||  !DXSetStringAttribute((Object)ac, "element type", "lines"))
	goto error;
    
    if ((ap = (Array)DXNewRegularArray(TYPE_FLOAT, 1, osbins+1,
			     (Pointer)&origin, (Pointer)&delta)) == NULL)
	goto error;
    
    
    /* reset each array ptr, so it won't get deleted twice for errors
     *  which happen after it becomes a member of the field.
     */
    if (!DXSetComponentValue(f, "data", (Object)ad))
	goto error;
    ad = NULL;
    
    if (!DXSetComponentValue(f, "connections", (Object)ac))
	goto error;
    ac = NULL;
    
    if (!DXSetComponentValue(f, "positions", (Object)ap))
	goto error;
    ap = NULL;

    if (!DXEndField(f))
       goto error;

    DXFree((Pointer)hd);
    DXFreeInvalidComponentHandle(hp->invalid);
    hp->didwork += totalcnt;
    return f;

error:
    DXDelete((Object)f);
    DXDelete((Object)ac);
    DXDelete((Object)ap);
    DXDelete((Object)ad);
    DXFree((Pointer)hd);
    DXFreeInvalidComponentHandle(hp->invalid);
    return NULL;
    
}

#endif  /* old routine, unused now */
