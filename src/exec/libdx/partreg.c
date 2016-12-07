/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>



/* this routine uses "P" for DXDebug() to print how many partitions are
 *  actually created compared to how many are requested.  for more 
 *  detail on the starts and sizes of each partition, it uses "Q".
 */

#include <stdio.h>
#include <math.h>
#include <string.h>

#include <dx/dx.h>

/* defined in permute.c */
#define PPP unsigned char *
void _dxfPermute( int dim, PPP out, int *ostrides, int *counts, 
                  int size, PPP in, int *istrides );

#define MAXRANK   16
#define MAXSHAPE  128

/*
 * arg block for parallel workers:  this version of partition goes
 *  parallel as soon as we compute how many divisions along each axis
 *  we are going to make.
 */
struct argblock {
    Field parent;		/* original field to partition */
    Field newpart;		/* new composite field member being created */
    int partid;			/* which partition number this one is */
    int rank;			/* the dimensionality of the connections */
    int divisions[MAXRANK];	/* number of parts along each axis */
    int counts[MAXRANK];	/* number of positions on each axis */
};

/* 
 * struct for computing the offsets and lengths along each dimension.
 */
struct cornerinfo {
    int thispart;		/* input, this partition number */
    int rank;			/* input, dimensionality of connections */
    int divisions[MAXRANK];	/* input, num of divisions along each dim */
    int totalcounts[MAXRANK];	/* input, num of positions along each dim */
    int corners[MAXRANK];	/* output, start position numbers per dim */
    int length[MAXRANK];	/* output, number of positions per dim */
    int offset[MAXRANK];	/* output, partition offset each dim */
    int stride[MAXRANK];	/* output, number of partitions to skip */
};

static void computecorners(struct cornerinfo *c);

static Error RegularDivisions(int rank, int *counts, int parts, 
		   int minsize, int *divisions);

static int Partition_Worker(Pointer);
static Array Array_Subset_Pos(Array a, int *startpos, 
			      int **prevcounts, int *offsets,
			      int *thick, int numdims, int *ccounts);
static Array Array_Subset_Con(Array a, int *startpos, 
			      int **prevcounts, int *offsets,
			      int *thick, int numdims, int *ccounts);

Field _dxf_has_ref_lists(Field f, int *pos, int *con);
Error _dxf_make_map_template(Array a, Array *map);
Error _dxf_fix_map_template(Array map, Array newmap);
Array _dxf_remap_by_map(Array a, Array map);

#if 0
static Array remap_by_computation(Array a, int ndims, int *ccounts, 
				int *corners, int *ncounts);
#endif

static Error todouble(Type t, Pointer v1, int i1, double *v2);
static Error fromdouble(Type t, double v1, Pointer v2, int i2);
static int my_iszero (Type t, Pointer v1, int i1);



Group
_dxf_PartitionRegular(Field f, int n, int size)
{
    int i, rank, parts;
    int counts[MAXRANK], divisions[MAXRANK];
    int intask = 0;
    Array conn, data;
    struct argblock arg;
    Field nf = NULL;
    Group g = NULL;


    /* do some error checks to be sure this is an ok field for this
     *  code to work on.  if it's malformed, set an error and return.
     *  if it's just not got regular connections, then return NULL
     *  without setting error and the irregular partitioner will try it.
     */

    /* internal error, shouldn't happen */
    if (DXGetObjectClass((Object)f) != CLASS_FIELD)
	DXErrorReturn(ERROR_BAD_CLASS,
		    "PartitionRegular not called with a field");

    /* it's an error if no positions component, but ok if no connections */
    if (!DXGetComponentValue(f, "positions"))
	DXErrorReturn(ERROR_BAD_PARAMETER, "no positions component found");

    if (!(conn = (Array)DXGetComponentValue(f, "connections")))
	return NULL;

    if (!DXQueryGridConnections(conn, &rank, counts))
	return NULL;


    /* compute the number of divisions along each axis we should make to
     *  generate the closest number of parts to what was requested.
     */
    if (!RegularDivisions(rank, counts, n, size, divisions))
	return NULL;

    /* make sure there is actually some work to do */
    parts = 1;
    for (i=0; i<rank; i++)
	parts *= divisions[i];

    if (parts == 1)
	return (Group)f;

    if (DXQueryDebug("PQ")) {
	int i;
	for (i=0; i<rank; i++)
	    DXDebug("PQ", "%d divisions (%d elements)", divisions[i], 
		   (int)((counts[i]-1)/divisions[i]));

	DXDebug("PQ", "%d partitions total (out of %d requested)", parts, n);
    }


    /* create the composite field and the individual fields in the serial
     *  section, and do the rest of the work after going parallel.
     */
    if (!(g = (Group)DXNewCompositeField()))
	return NULL;

    DXCopyAttributes((Object)g, (Object)f);

    /* if a data component exists, the composite field data type must be
     *  set.  composite fields are only supposed to contain fields which
     *  have the same data type.
     */
    if ((data=(Array)DXGetComponentValue(f, "data"))!=NULL) {
	Type t;
	Category c;
	int rank;
	int shape[MAXRANK];

	if (!DXGetArrayInfo(data, NULL, &t, &c, &rank, shape))
	    goto error;

	if (!DXSetGroupTypeV(g, t, c, rank, shape))
	    goto error;
    }

    /* begin creating worker tasks - they don't start executing until
     *  the DXExecuteTaskGroup() below.
     */
    if (!DXCreateTaskGroup())
	goto error;

    intask++;
    arg.parent = f;
    arg.rank = rank;
    for (i=0; i<rank; i++) {
	arg.divisions[i] = divisions[i];
	arg.counts[i] = counts[i];
    }

#define PRESERVE_ATTRIBUTES 0

    for (i=0; i<parts; i++) {
#if PRESERVE_ATTRIBUTES  
	/* use DXCopy instead of DXNewField() to preserve any attributes */
	if (!(nf = (Field)DXCopy((Object)f, COPY_ATTRIBUTES)))
	    goto error;
#else
	if (!(nf = DXNewField()))
	    goto error;
#endif
    
	if (!DXSetMember(g, NULL, (Object)nf)) {
	    DXDelete((Object)nf);
	    goto error;
	}

	arg.newpart = nf;
	arg.partid = i;

	if (!DXAddTask(Partition_Worker, (Pointer)&arg, sizeof(arg), 1.0))
	    goto error;
    }

    intask = 0;

    /* actually exectute the tasks here 
     */
    if (!DXExecuteTaskGroup())
	goto error;
    
    return g;

  error:
    if (g)
	DXDelete((Object)g);

    if (intask)
	DXAbortTaskGroup();

    return NULL;
}


/* compute the number of pieces each dimension should be divided into.
 *  all but the last parm are inputs;  'divisions' is the return list.
 */
static Error RegularDivisions(int rank, int *counts, int parts, int minsize, 
		     int *divisions)
{
    int i, j;
    double root;
    double elements, points;  /* these are float to catch integer overflows */

    if (parts <= 0) {
	DXSetError(ERROR_BAD_PARAMETER, "#10020", "number of subparts");
	return ERROR;
    }

    /* initialize these in case we decide to return without partitioning */
    for (i=0; i<rank; i++)
	divisions[i] = 1;


    /* compute the total number of connection elements and also positions.
     */
    elements = 1;
    points = 1;
    for (i=0; i<rank; i++) {
	elements *= (counts[i] - 1);
	points *= counts[i];
    }

    /* if this field isn't going to be divided because it is already
     *  smaller than the threshold, return here without error.
     */
    if (parts == 1 || (minsize > 0 && minsize > points))
	return OK;


    /* if user didn't set a min size, use a single primitive as the minimum
     *  element size.  this is fine for testing, but in real use it is 
     *  much too small to be efficient - but i don't know how to compute
     *  what a "reasonable" number of items per partition is.  you have to
     *  look at the modules which will subsequently process this data, and
     *  amortize the overhead of starting a new task for this partition
     *  over the actual time it takes to do the processing - unless the
     *  processing is incredibly expensive per point, it is hard to imagine
     *  that one primitive is reasonable.
     */
    if (minsize == 0)
	minsize = 1;

    /* check for integer wrap if the number of points is too large */
    if (points <= 0 || points >= DXD_MAX_INT || (points / minsize) >= DXD_MAX_INT) {
	DXSetError(ERROR_BAD_PARAMETER, "too many positions");
	return ERROR;
    }

    if ((elements / minsize) < parts)
        parts = elements / minsize;


#if 0
    DXDebug("R", "points %f, minsize %d, parts %d", points, minsize, parts);
#endif

    if (parts == 1)
	return OK;

    if (rank == 1)
	divisions[0] = parts;

    else {
	int divmap[MAXRANK];
	int newdiv[MAXRANK];
	int newdim[MAXRANK];
	int temp, curpts;

	for (i=0; i<rank; i++) {
	    newdim[i] = counts[i]-1;
	    newdiv[i] = 1;
	    divmap[i] = i;
	}

	/* order the dimensions from smallest to largest counts */
	for (i=0; i<rank; i++)
	    for (j=i+1; j<rank; j++)
		if (newdim[i] > newdim[j]) {
		    temp = newdim[j]; newdim[j] = newdim[i]; newdim[i] = temp;
		    temp = divmap[j]; divmap[j] = divmap[i]; divmap[i] = temp;
		}

	
	for (i=0; i<rank; i++) {
	    curpts = 1;
	    for (j=0; j<i; j++)
		curpts *= newdiv[j];
	    for (j=i; j<rank; j++)
		curpts *= newdim[j];

#define F_ERROR 0.0001	/* fuzz factor for floating point round off error */
#define ROUNDUP 0.0000  /* if this close to next integer, round up */

	    /* if you call pow(A, 1.0 / 1.0), you don't get exactly A back,
	     *  which causes problems when you use floor() on the result.
	     */
	    if (rank-i > 1) {
		root = pow((double)parts/curpts, 1.0 / ((double)rank-i));
		newdiv[i] = floor(root * newdim[i] + ROUNDUP + F_ERROR);
	    } else {
#if 1
		root = (double)parts/curpts;
		newdiv[i] = floor(root * newdim[i] + F_ERROR);
#endif
	    }


#if 0
	    DXDebug("R", "%d pts; newdiv = %g, roundup = %g, floor = %d", 
		  newdim[i]+1, root * newdim[i] + F_ERROR, ROUNDUP, newdiv[i]);
#endif
#if 0
	    DXDebug("R", "%d pts; rt %d of %g; __(%g * %d + %f) = %d", 
		  newdim[i]+1, rank-i, (double)parts/curpts, 
		  root, newdim[i], F_ERROR, newdiv[i]);
#endif

	    /* don't allow more parts along an axis than there are elements,
	     *  and don't let the number of partitions along an axis be less
	     *  than one.  one means don't divide along this axis.  zero
             *  screws up other computations because we multiply by it.
	     */
	    newdiv[i] = MIN(newdiv[i], newdim[i]);
	    newdiv[i] = MAX(newdiv[i], 1);

	    divisions[divmap[i]] = newdiv[i];
	}
    }


    return OK; 
}

/* worker task which operates in parallel to create 
 *  a single new partition from the original field.  
 */
static int Partition_Worker(Pointer ptr)
{
    int i, j, n;
    char *name;
    char *dep, *ref, *der;
    struct cornerinfo c, pc;
    int *prevcounts[MAXRANK];
    Array a, na = NULL;
    int refpos, refcon;
    Array posmap = NULL;
    Array conmap = NULL;  /* temp for now - should use algorithm */

    struct argblock *ap = (struct argblock *)ptr;


    /* decide where the origin of the connections is going to be and
     *  how long each side is.  compute both origins and lengths first
     *  as floats, and then round to ints so any unevenness is spread 
     *  throughout the volume, so when you get to the end, you don't leave 
     *  off part of the volume or create empty partitions.
     */
    c.thispart = ap->partid;
    c.rank = ap->rank;
    for (i=0; i<ap->rank; i++) {
	c.divisions[i] = ap->divisions[i];
	c.totalcounts[i] = ap->counts[i];
    }
    computecorners(&c);

    /* now compute the counts in each of the previous partitions. */
    pc.rank = ap->rank;
    for (i=0; i<ap->rank; i++) {
	pc.divisions[i] = ap->divisions[i];
	pc.totalcounts[i] = ap->counts[i];
    }

    for (i=0; i<MAXRANK; i++)
	prevcounts[i] = NULL;
    
    for (i=0; i<ap->rank; i++) {
	if (!(prevcounts[i] = (int *)DXAllocate(sizeof(int) * (c.offset[i]+1))))
	    goto error;
    }

    for (i=0; i<ap->rank; i++) {
	for (j=0; j <= c.offset[i]; j++) {
	    pc.thispart = j * c.stride[i];
	    computecorners(&pc);
	    prevcounts[i][j] = pc.length[i] - 1;
	}
    }

#if 0    
    if (DXQueryDebug("Q")) {
	DXDebug("Q", "part %d:", ap->partid);
	DXDebug("Q", "dim: counts, divisions, offsets, corner, length");
	for (i=0; i<ap->rank; i++) {
	    DXDebug("Q", "%d: %3d, %2d, %2d, %2d, %2d",
		  i, ap->counts[i], ap->divisions[i], c.offset[i], 
		  c.corners[i], c.length[i]);
	    for (j=0; j<c.offset[i]; j++)
		DXDebug("Q", "  partition offset[%d][%d]: %d", 
		      i, j, prevcounts[i][j]);
	}
    }
#endif


    /* create the connections first, because you have to worry
     *  about setting the mesh offsets, and because the shape of
     *  the grid is needed to know how to interpret the rest of the
     *  components.  do all other components with a common routine.
     */

    a = (Array)DXGetComponentValue(ap->parent, "connections");
    if (!a)
	goto error;
    
    na = (Array)DXMakeGridConnectionsV(ap->rank, c.length);
    if (!na)
	goto error;
    
    if (!DXCopyAttributes((Object)na, (Object)a))
	goto error;

    if (!DXSetMeshOffsets((MeshArray)na, c.corners))
	goto error;

    if (!DXSetComponentValue(ap->newpart, "connections", (Object)na))
	goto error;

    na = NULL;

    /* check to see if we will need to make a map for any ref components.
     *  since we know that the connections are regular (you can't get into
     *  this code section otherwise), we don't have to build a connections
     *  map, but the positions may not be regular, so we have to build a map
     *  for them.  (the following code does make a map for the connections
     *  anyway, as a quick hack to get the function into the system).
     */

    _dxf_has_ref_lists(ap->parent, &refpos, &refcon);

    /* make a refpos map (and a refcon map for now until i get a 
     * map routine which keeps the map compact) if needed.
     * the map is made by making a regular array of 0-N items and then
     * permuting them the same way we are going to permute the positions.
     */
    if (refpos) {
	Array tmap = NULL;
	
	a = (Array)DXGetComponentValue(ap->parent, "positions");
	if (!a)
	    goto error;
	
	if (!_dxf_make_map_template(a, &posmap))
	    goto error;
	
	tmap = Array_Subset_Pos(posmap, c.corners, prevcounts, c.offset,
				c.length, ap->rank, ap->counts);
	if (!tmap)
	    goto error;
	
	if (!_dxf_fix_map_template(posmap, tmap)) {
	    DXDelete((Object) tmap);
	    goto error;
	}

	DXDelete((Object) tmap);
	/* posmap now captures where the positions have moved to */
    }
    
    if (refcon) {
	Array tmap = NULL;
	
	a = (Array)DXGetComponentValue(ap->parent, "connections");
	if (!a)
	    goto error;

	if (!_dxf_make_map_template(a, &conmap))
	    goto error;

	tmap = Array_Subset_Con(conmap, c.corners, prevcounts, c.offset,
				c.length, ap->rank, ap->counts);
	if (!tmap)
	    goto error;

	if (!_dxf_fix_map_template(conmap, tmap)) {
	    DXDelete((Object) tmap);
	    goto error;
	}

	DXDelete((Object) tmap);
	/* conmap now captures where the connections have moved to */
    }


    /* for each other component in the field, partition it accordingly.
     */
    for (i = 0; 
	 (a=(Array)DXGetEnumeratedComponentValue(ap->parent, i, &name)); 
	 i++) {

	/* skip connections - we've already done them */
	if (!strcmp(name, "connections"))
	    continue;

	if (!DXGetStringAttribute((Object)a, "dep", &dep))
	    dep = NULL;
	if (!DXGetStringAttribute((Object)a, "ref", &ref))
	    ref = NULL;
	if (!DXGetStringAttribute((Object)a, "der", &der))
	    der = NULL;
	
	/* skip any derived components - they will be recomputed if needed.
	 */
	if (der)
	    continue;
	
	/* if it is not dep or ref anything, just replicate it.
	 */
	if (!dep && !ref) {
	    if (!DXSetComponentValue(ap->newpart, name, (Object)a))
		goto error;
	    
	    continue;
	}

#if 0
	DXDebug("A",
	      "thispart = %d, this comp = %d, corners = %d,%d,%d",
	      c.thispart, i, c.corners[0], c.corners[1], c.corners[2]);
#endif

	/* positions get done here with the rest of the things which
         * are dep on positions.
	 */
	if (dep && !strcmp(dep, "positions")) {
	    na = Array_Subset_Pos(a, c.corners, prevcounts, c.offset,
				  c.length, ap->rank, ap->counts);

	} else if (dep && !strcmp(dep, "connections")) {
	    na = Array_Subset_Con(a, c.corners, prevcounts, c.offset,
				  c.length, ap->rank, ap->counts);

	} else if (ref && !strcmp(ref, "positions")) {
	    na = _dxf_remap_by_map(a, posmap);
	    if (!DXGetArrayInfo(na, &n, NULL, NULL, NULL, NULL))
		goto error;
	    if (n == 0) {
		DXDelete((Object)na);
		continue;
	    }

	} else if (ref && !strcmp(ref, "connections")) {
	    na = _dxf_remap_by_map(a, conmap);

	    if (!DXGetArrayInfo(na, &n, NULL, NULL, NULL, NULL))
		goto error;
	    if (n == 0) {
		DXDelete((Object)na);
		continue;
	    }

	} else {
	    /* not dep or ref something we recognize.  just replicate it.
	     * (this didn't use to be here - these components were skipped).
	     */
	    if (!DXSetComponentValue(ap->newpart, name, (Object)a))
		goto error;
	    
	    continue;
	}

	if (!na)
	    goto error;

	if (!DXCopyAttributes((Object)na, (Object)a))
	    goto error;

	if (!DXSetComponentValue(ap->newpart, name, (Object)na))
	    goto error;

	na = NULL;
    }
    
    if (!DXEndField(ap->newpart))
	goto error;

    for (i=0; i<MAXRANK; i++)
	if (prevcounts[i]) DXFree((Pointer)prevcounts[i]);

    DXDelete((Object)posmap);
    DXDelete((Object)conmap);
    return OK;

  error:
    if (na) DXDelete((Object)na);
#if 0
    /* this gets deleted at the group level */
    if (ap->newpart) DXDelete((Object)ap->newpart);
#endif
    for (i=0; i<MAXRANK; i++)
	if (prevcounts[i]) DXFree((Pointer)prevcounts[i]);

    DXDelete((Object)posmap);
    DXDelete((Object)conmap);
    return ERROR;
}


/* extract a subset of an array.  parms are:
 *  array to subset
 *  origin of this subset
 *  number of connections in each previous partition in each dimension
 *  which partition number this is along each axis
 *  number of connections to include in the subset
 *  number of dimensions in the connections
 *  number of counts along the original dimensions in the connections
 */
static Array Array_Subset_Pos(Array a, int *startpos, int **prevcounts, 
			      int *offset, int *thick, int numdims, 
			      int *ccounts)
{
    Array na, terms[MAXRANK];
    int nitems, itemsize, nterms;
    Type t;
    Category c;
    float origins[MAXRANK], deltas[MAXRANK*MAXRANK];
    float norigins[MAXRANK];
    int rank, shape[MAXSHAPE], counts[MAXRANK], ncounts[MAXRANK];
    int nsp[MAXRANK], ntk[MAXRANK], noffset[MAXRANK];
    int ostride[MAXRANK], nstride[MAXRANK];
    int *nprevcounts[MAXRANK];
    Pointer op, np;
    int i, j, k, p;

    /* check here for fully regular array and shortcut the following code */

    if (DXQueryGridPositions(a, &nterms, counts, origins, deltas)) {
	
        /* if dim of array doesn't match dim of connections, expand them
         *  to be sure you get the right ones.
         */
        if (nterms != numdims) {
	    DXDebug("Q", "partreg: nterms != numdims");
            for (i=0,j=0; j<nterms; j++) {
                if (counts[j] <= 1) {
		    nsp[j] = 0;
		    ntk[j] = counts[j];
		    noffset[j] = 0;
		    nprevcounts[j] = 0;
		} else {
		    nsp[j] = startpos[i];
		    ntk[j] = thick[i];
		    noffset[j] = offset[i];
		    nprevcounts[j] = prevcounts[i];
		    i++;
		}
	    }
	    startpos = nsp;
	    thick = ntk;
	    offset = noffset;
	    prevcounts = nprevcounts;
	}

	
	/* compute new origin and counts */
	for (i=0; i<nterms; i++) {
	    ncounts[i] = thick[i];
	    norigins[i] = origins[i];
	}
	for (i=0; i<nterms; i++) {
	    for (j=0; j<nterms; j++) {
#if 0
		/* this is the more efficient way to compute the
		 *  starts of each partition.  the problem is that
		 *  because of floating point accumulated error, there
		 *  appears to be a gap between the end of the last
		 *  partition and the start of the next if you do it
		 *  this way.  the code we are using below accumulates
		 *  the error the same way a computation would if it
		 *  stepped across a partition.   floating point is a pain.
		 */
		norigins[j] += deltas[i*nterms + j] * startpos[i];
		DXDebug("Q", "startpos = %d, delta = %f", 
		      startpos[i], deltas[i*nterms + j]);
#else
		for (p=0; p<offset[i]; p++) {
		    float f;   /* this works around a compiler bug */
		    f = deltas[i*nterms + j] * prevcounts[i][p];
		    norigins[j] += f;
		    DXDebug("Q", "prevcounts = %d, delta = %f", 
			  prevcounts[i][p], deltas[i*nterms + j]);
		}
#endif
		DXDebug("Q", "%d: origin was %f, now %f", 
		      j, origins[j], norigins[j]);
	    } 
	}
	
	na = DXMakeGridPositionsV(nterms, ncounts, norigins, deltas);
	return na;
    }
    
    /* not a regular grid */

    switch(DXGetArrayClass(a)) {
	
      default:
      case CLASS_ARRAY:
	/* normal expanded array */
expanded:
	if (!DXGetArrayInfo(a, &nitems, &t, &c, &rank, shape))
	    return NULL;

	itemsize = DXGetItemSize(a);
	op = DXGetArrayData(a);
	
	/* old strides */
	j = 1;
	for (i=numdims-1; i>=0; i--) {
	    ostride[i] = j;
	    j *= ccounts[i];
	}

	for (i=0; i<numdims; i++)
	    ncounts[i] = thick[i];

	for (i=0; i<numdims; i++)
	    op = (Pointer)((ubyte *)op + 
			   startpos[i] * ostride[i] * itemsize);

	/* new strides */
	j = 1;
	for (i=numdims-1; i>=0; i--) {
	    nstride[i] = j;
	    j *= ncounts[i];
	}
	nitems = j;
 
	na = DXNewArrayV(t, c, rank, shape);
        if (!na)
	    return NULL;
	
	np = DXGetArrayData(DXAddArrayData(na, 0, nitems, NULL));
	if (!np) {
	    DXDelete((Object)na);
	    return NULL;
	}

	/* call permute to move the data */
	_dxfPermute(numdims, np, nstride, ncounts, itemsize, op, ostride);
	
	break;
	
      case CLASS_REGULARARRAY:
	/* need shape[0] from this */
	DXGetArrayInfo(a, &nitems, &t, &c, &rank, shape);
	DXGetRegularArrayInfo((RegularArray)a, &i, (Pointer)origins, 
			    (Pointer)deltas);

	/* if dim > 1 && all deltas != 0, this can't be regular anymore */
	if (numdims > 1) {
	    for (k=0; k<shape[0]; k++)
		if (!my_iszero(t, (Pointer)deltas, k))
		    goto expanded;
	}

	/* update the new counts and origins.  handle floats special
	 *  because the positions are float and they have to match exactly.
	 *  this means accumulate error instead of doing a single multiply.
	 */
	if (t == TYPE_FLOAT) {
	    for (k=0; k<numdims; k++) {
		nitems = nitems / ccounts[k] * thick[k];
		for (j=0; j<shape[0]; j++) {
#if 0
		    origins[j] += startpos[k] * deltas[j];
#else
		    for (p=0; p<offset[k]; p++) {
			float f;   /* work around sgi optimizer problem */
			f = deltas[j] * prevcounts[k][p];
			origins[j] += f;
		    }
#endif
		}
	    }
	} else {
	    double tvalue1, tvalue2;
	    for (k=0; k<numdims; k++) {
		nitems = nitems / ccounts[k] * thick[k];
		for (j=0; j<shape[0]; j++) {
		    todouble(t, (Pointer)deltas, j, &tvalue1);
		    todouble(t, (Pointer)origins, j, &tvalue2);
		    
		    tvalue2 += tvalue1 * startpos[k];
		    
		    fromdouble(t, tvalue2, (Pointer)origins, j);
		}
	    }
	}

	na = (Array)DXNewRegularArray(t, shape[0], nitems,
					(Pointer)origins, (Pointer)deltas);
	break;

      case CLASS_CONSTANTARRAY:
	DXGetArrayInfo(a, &nitems, &t, &c, &rank, shape);

	/* update the new counts */
	for (k=0; k<numdims; k++)
	    nitems = nitems / ccounts[k] * thick[k];

	na = (Array)DXNewConstantArrayV(nitems, DXGetConstantArrayData(a),
				       t, c, rank, shape);
	break;

      case CLASS_PRODUCTARRAY:
	DXGetProductArrayInfo((ProductArray)a, &j, terms);
	if (j != numdims)
	    goto expanded;
	    /* DXMessage("partreg: j != numdims, product array"); */

	for (i=0; i<j; i++)
	    terms[i] = Array_Subset_Pos(terms[i], &startpos[i], 
					&prevcounts[i], &offset[i],
					&thick[i], 1, &ccounts[i]);

	na = (Array)DXNewProductArrayV(j, terms);
	break;
	
	
      case CLASS_MESHARRAY:
	DXGetMeshArrayInfo((MeshArray)a, &j, terms);
	if (j != numdims)
	    goto expanded;
	    /* DXMessage("partreg: j != numdims, mesh array"); */

	for (i=0; i<j; i++)
	    terms[i] = Array_Subset_Pos(terms[i], &startpos[i], 
					&prevcounts[i], &offset[i],
					&thick[i], 1, &ccounts[i]);

	na = (Array)DXNewMeshArrayV(j, terms);
	break;
	
      case CLASS_PATHARRAY:
	DXGetPathArrayInfo((PathArray)a, ncounts);
	na = (Array)DXNewPathArray(thick[0]);
	break;

    }

    return na;
}



/* same as Array_Subset_Pos above, except it works for arrays where
 *  the contents is dep on the connections instead of the positions.
 */
static Array Array_Subset_Con(Array a, int *startpos, int **prevcounts, 
			   int *offset, int *thick, int numdims, int *ccounts)
{
    Array na, terms[MAXRANK];
    int nitems, itemsize, nterms;
    Type t;
    Category c;
    float origins[MAXRANK], deltas[MAXRANK*MAXRANK];
    int rank, shape[MAXSHAPE], counts[MAXRANK], ncounts[MAXRANK];
    int nsp[MAXRANK], ntk[MAXRANK];
    int ostride[MAXRANK], nstride[MAXRANK];
    Pointer op, np;
    int i, j, k, p;
    

    /* check here for fully regular array and shortcut the following code */

    if (DXQueryGridConnections(a, &nterms, counts)) {
	
        /* if dim of array doesn't match dim of connections, expand them
         *  to be sure you get the right ones.
         */
        if (nterms != numdims) {
	    DXDebug("Q", "partreg: nterms != numdims");
            for (i=0,j=0; j<nterms; j++) {
                if (counts[j] <= 1) {
		    nsp[j] = 0;
		    ntk[j] = counts[j];
		} else {
		    nsp[j] = startpos[i];
		    ntk[j] = thick[i];
		    i++;
		}
	    }
	    startpos = nsp;
	    thick = ntk;
	}

	
	/* compute new counts */
	for (i=0; i<nterms; i++)
	    ncounts[i] = thick[i] - 1;

	na = DXMakeGridConnectionsV(nterms, ncounts);
	return na;
    }
    
    /* not a regular grid */

    switch(DXGetArrayClass(a)) {

      default:
      case CLASS_ARRAY:
	/* normal expanded array */
expanded:
	if (!DXGetArrayInfo(a, &nitems, &t, &c, &rank, shape))
	    return NULL;

	itemsize = DXGetItemSize(a);
	op = DXGetArrayData(a);
	
	/* old strides */
	j = 1;
	for (i=numdims-1; i>=0; i--) {
	    ostride[i] = j;
	    j *= ccounts[i] - 1;
	}

	for (i=0; i<numdims; i++)
	    ncounts[i] = thick[i] - 1;

	for (i=0; i<numdims; i++)
	    op = (Pointer)((ubyte *)op + 
			   startpos[i] * ostride[i] * itemsize);

	/* new strides */
	j = 1;
	for (i=numdims-1; i>=0; i--) {
	    nstride[i] = j;
	    j *= ncounts[i];
	}
	nitems = j;
 
	na = DXNewArrayV(t, c, rank, shape);
        if (!na)
	    return NULL;
	
	np = DXGetArrayData(DXAddArrayData(na, 0, nitems, NULL));
	if (!np) {
	    DXDelete((Object)na);
	    return NULL;
	}

	/* call permute to move the data */
	_dxfPermute(numdims, np, nstride, ncounts, itemsize, op, ostride);
	
	break;
	
      case CLASS_REGULARARRAY:
	/* need shape[0] from this */
	DXGetArrayInfo(a, &nitems, &t, &c, &rank, shape);
	DXGetRegularArrayInfo((RegularArray)a, &i, (Pointer)origins, 
			    (Pointer)deltas);

	/* if dim > 1 && all deltas != 0, this can't be regular anymore */
	if (numdims > 1) {
	    for (k=0; k<shape[0]; k++)
		if (!my_iszero(t, (Pointer)deltas, k))
		    goto expanded;
	}


	/* update the new counts and origins.
	 */
	for (k=0; k<numdims; k++) {
	    double tvalue1, tvalue2;
	    nitems = nitems / (ccounts[k]-1) * (thick[k]-1);
	    for (j=0; j<shape[0]; j++) {
		todouble(t, (Pointer)deltas, j, &tvalue1);
		todouble(t, (Pointer)origins, j, &tvalue2);
		
		for (p=0; p<offset[k]; p++)
		    tvalue2 += tvalue1 * (prevcounts[k][p]-1);
		
		fromdouble(t, tvalue2, (Pointer)origins, j);
	    }
	}

	na = (Array)DXNewRegularArray(t, shape[0], nitems,
					(Pointer)origins, (Pointer)deltas);
	break;

      case CLASS_CONSTANTARRAY:
	DXGetArrayInfo(a, &nitems, &t, &c, &rank, shape);

	/* update the new counts */
	for (k=0; k<numdims; k++)
	    nitems = nitems / (ccounts[k]-1) * (thick[k]-1);

	na = (Array)DXNewConstantArrayV(nitems, DXGetConstantArrayData(a),
				       t, c, rank, shape);
	break;

      case CLASS_PRODUCTARRAY:
	DXGetProductArrayInfo((ProductArray)a, &j, terms);
	if (j != numdims)
	    goto expanded;
	    /* DXMessage("partreg: j != numdims, product array"); */

	for (i=0; i<j; i++)
	    terms[i] = Array_Subset_Con(terms[i], &startpos[i], 
					&prevcounts[i], &offset[i],
					&thick[i], 1, &ccounts[i]);

	na = (Array)DXNewProductArrayV(j, terms);
	break;
	
	
      case CLASS_MESHARRAY:
	DXGetMeshArrayInfo((MeshArray)a, &j, terms);
	if (j != numdims)
	    goto expanded;
	    /* DXMessage("partreg: j != numdims, mesh array"); */

	for (i=0; i<j; i++)
	    terms[i] = Array_Subset_Con(terms[i], &startpos[i], 
					&prevcounts[i], &offset[i],
					&thick[i], 1, &ccounts[i]);

	na = (Array)DXNewMeshArrayV(j, terms);
	break;
	
      case CLASS_PATHARRAY:
	DXGetPathArrayInfo((PathArray)a, ncounts);
	na = (Array)DXNewPathArray(thick[0]-1);
	break;
	
    }

    return na;
}


/* decide where the origin of the connections is going to be and
 *  how long each side is.  compute both origins and lengths first
 *  as floats, and then round to ints so any unevenness is spread 
 *  throughout the volume, so when you get to the end, you don't leave 
 *  off part of the volume or create empty partitions.
 */
static void computecorners(struct cornerinfo *c)
{
    int i, n, part;
    float reallength[MAXRANK];
    float realcorners[MAXRANK];
    float realend;

    /* compute what the fractional number of connections in each partition
     *  should be to evenly divide the field into the requested number of
     *  divisions (e.g. a field with 10 positions (9 connections) divided
     *  into 4 partitions is 9/4 == 2.25
     * also compute the stride in partitions - how many partitions do you
     *  need to skip to move one in the X dimension, etc.
     */
    n = 1;
    for (i=0; i<c->rank; i++) {
	c->stride[i] = n;
	n *= c->divisions[i];
	reallength[i] = (float)(c->totalcounts[i] - 1) / c->divisions[i];
    }

    /* now compute which position number should be picked as the start
     *  of this partition, and how many positions long this partition is.
     *  the offset and lengths are computed as floats, but then are rounded
     *  down to the previous whole integer.  this means some partitions 
     *  may have a length which is 1 more or less than others.
     * the realXXX variables below are floats, the others are integers.
     * also compute the partition offset - the number of partitions
     *  between the origin and this one, along each axis.
     */
    part = c->thispart;
    for (i=c->rank-1; i>=0; i--) {
	realcorners[i] = (part / c->stride[i]) * reallength[i];
	c->corners[i] = (int)(realcorners[i]);
        realend = ((part / c->stride[i]) + 1) * reallength[i];
	c->length[i] = (int)(realend) - c->corners[i] + 1;
	c->offset[i] = part / c->stride[i];
	part %= c->stride[i];
    }

    /* if the last partition extends past the end of the field, adjust
     *  the length.
     */
    for (i=0; i<c->rank; i++) {
	if (c->corners[i] + c->length[i] > c->totalcounts[i])
	    c->length[i] = c->totalcounts[i] - c->corners[i];
    }

    return;
}

/* check to see if this field has components which don't dep
 *  positions or connections but do ref one of them.  in that
 *  case, a map will need to be built to remap the references,
 *  and it might as well be built while the original component is
 *  being partitioned.
 */

Field _dxf_has_ref_lists(Field f, int *pos, int *con)
{
    int i;
    Object comp;
    char *name;
    char *dep, *ref, *der;

    *pos = *con = 0;

    /* check attributes for each component in the field.
     */
    for (i = 0; (comp=DXGetEnumeratedComponentValue(f, i, &name)); i++) {
	
	/* skip positions and connections.
	 */
	if (!strcmp(name, "positions") || !strcmp(name, "connections"))
	    continue;
	
	if (!DXGetStringAttribute(comp, "dep", &dep))
	    dep = NULL;
	if (!DXGetStringAttribute(comp, "ref", &ref))
	    ref = NULL;
	if (!DXGetStringAttribute(comp, "der", &der))
	    der = NULL;
	
	/* skip derived components.
	 */
	if (der)
	    continue;
	
	/* if it is dep positions or connections, it will be handled just
	 * like those components, so no map will be needed.
	 */
	if (dep && (!strcmp(dep, "positions") || !strcmp(dep, "connections")))
	    continue;
	
	/* if it is not dep and not ref anything, it will just be copied
	 * through unchanged, so again, no map will be needed.
	 */
	if (!dep && !ref)
	    continue;

	/* these are the interesting cases.
	 */
	if (ref && !strcmp(ref, "positions")) {
	    (*pos)++;
	    continue;
	}
	
	if (ref && !strcmp(ref, "connections")) {
	    (*con)++;
	    continue;
	}

	/* what here?  either dep on something we don't recognize, or
	 *  ref on something we don't recognize.  i suppose this should
	 *  be handled with either a warning, and/or it should just be 
	 *  passed thru unchanged.
	 */
	
    }

    /* return f if we found components which will need maps built for them.
     * otherwise return NULL.
     */
    return ((*pos || *con) ? f : NULL);

}

/* given a map saying where the old items have ended up and an
 *  array which refs those items, renumber the ref component, and remove
 *  any refs to items which are no longer in the original component.
 */
Array _dxf_remap_by_map(Array a, Array map)
{
    int i;
    int index;
    int nitems;
    int mapitems;
    int newitems;
    Array na = NULL;
    Type t;
    Category c;
    int rank;
    int shape[MAXRANK];
    int *sp, *mp;

    if (!DXGetArrayInfo(a, &nitems, &t, &c, &rank, shape))
	return NULL;

    if (!DXTypeCheck(a, TYPE_INT, CATEGORY_REAL, 0) &&
	!DXTypeCheck(a, TYPE_INT, CATEGORY_REAL, 1, 1)) {
	DXSetError(ERROR_DATA_INVALID, "#11385");
	return NULL;
    }

    if (!DXGetArrayInfo(map, &mapitems, NULL, NULL, NULL, NULL))
	return NULL;

    /*nbytes = sizeof(int);*/

    na = DXNewArray(TYPE_INT, CATEGORY_REAL, 0);
    if (!na)
	return NULL;

    if (!DXCopyAttributes((Object)na, (Object)a))
        goto error;

    if (!DXAllocateArray(na, nitems))
	goto error;

    sp = (int *)DXGetArrayData(a);
    mp = (int *)DXGetArrayData(map);

    if (!sp || !mp)
	goto error;

    newitems = 0;
    for (i=0; i<nitems; i++) {
	index = sp[i];
	if (index < 0 || index >= mapitems)
	    continue;

	if (mp[index] < 0)
	    continue;

	if (!DXAddArrayData(na, newitems, 1, (Pointer)&mp[index]))
	    goto error;

	newitems++;
    }

    DXTrim(na);
    return na;

  error:
    DXDelete((Object)na);
    return NULL;
}

/* this needs to be implemented to save memory.  for now, use remap_by_map()
 */
#if 0
static 
Array remap_by_computation(Array a, int ndims, int *ccounts, 
			 int *corners, int *ncounts)
{
    /* compute where the new points should go */
    return a;
}
#endif

/* this builds a regular array of 0 to N-1 items, which are going
 *  to be partitioned just like the target, so we can see where the
 *  items are going to be moved to.
 */
Error _dxf_make_map_template(Array a, Array *map)
{
    int i, *ip;
    int nitems;

    if (!DXGetArrayInfo(a, &nitems, NULL, NULL, NULL, NULL))
	return ERROR;

    *map = DXNewArray(TYPE_INT, CATEGORY_REAL, 0);
    if (! *map)
	return ERROR;

    if (!DXAddArrayData(*map, 0, nitems, NULL))
	goto error;

    ip = (int *)DXGetArrayData(*map);
    if (!ip)
	goto error;

    for (i=0; i<nitems; i++)
	*ip++ = i;
    
    return OK;
    
  error:
    DXDelete((Object) *map);
    return ERROR;
}

/* this routine takes the original map and the partitioned map and
 *  sees where the bits have been moved to.
 */
Error _dxf_fix_map_template(Array map, Array partmap)
{
    int i, *orig_ip, *part_ip;
    int *np = NULL;
    int orig_items, part_items;

    /* the number of items in the original map and the number of items 
     * in the partitioned map.
     */
    if (!DXGetArrayInfo(map, &orig_items, NULL, NULL, NULL, NULL))
	return ERROR;

    if (!DXGetArrayInfo(partmap, &part_items, NULL, NULL, NULL, NULL))
	return ERROR;

    /* allocate space for another list of the same size.  this will
     *  replace the original list when it contains the new values.
     */
    np = (int *)DXAllocate(sizeof(int) * orig_items);
    if (!np)
	goto error;

    orig_ip = (int *)DXGetArrayData(map);
    if (!orig_ip)
	goto error;

    part_ip = (int *)DXGetArrayData(partmap);
    if (!part_ip)
	goto error;

    /* initialize list of -1's, which means referenced item is no longer
     *  part of the new target array.
     */

    for (i=0; i<orig_items; i++)
	np[i] = -1;

    /* now for the ones which still exist, say where they came from
     */
    for (i=0; i<part_items; i++) {
	/*j = part_ip[i];*/
	np[part_ip[i]] = i;
    }

    /* the data in the map used to be where the original items came from.
     *  now the map will contain where they went to, which is what you
     *  need to look the values up directly.
     */
    memcpy(orig_ip, np, sizeof(int) * orig_items);

    DXFree((Pointer) np);
    return OK;

  error:
    DXFree((Pointer) np);
    return ERROR;
}







#define TYPE_FROM(type, in1, out1, i1) \
{ type *p1;                   \
  p1 = (type *)out1;          \
  p1[i1] = (type) in1;      \
}

#define TYPE_TO(type, in1, i1, out1) \
{ type *p1;		      \
  p1 = (type *)in1;           \
  *out1 = (double) p1[i1];   \
}

#define TYPE_EQ(type, in1, i1, rc) \
{ type *p1;                    \
  p1 = (type *)in1;            \
  rc = (double) p1[i1] == 0.0; \
}


/* add convert a double to a value of any type
 */
static Error fromdouble(Type t, double v1, Pointer v2, int i2)
{
    switch (t) {
      case TYPE_UBYTE:  TYPE_FROM(ubyte,  v1, v2, i2);  return OK;
      case TYPE_BYTE:   TYPE_FROM(byte,   v1, v2, i2);  return OK;
      case TYPE_USHORT: TYPE_FROM(ushort, v1, v2, i2);  return OK;
      case TYPE_SHORT:  TYPE_FROM(short,  v1, v2, i2);  return OK;
      case TYPE_UINT:   TYPE_FROM(uint,   v1, v2, i2);  return OK;
      case TYPE_INT:    TYPE_FROM(int,    v1, v2, i2);  return OK;
      case TYPE_FLOAT:  TYPE_FROM(float,  v1, v2, i2);  return OK;
      case TYPE_DOUBLE: TYPE_FROM(double, v1, v2, i2);  return OK;
	  case TYPE_HYPER: case TYPE_STRING: break;
    }

    DXSetError(ERROR_NOT_IMPLEMENTED, "unrecognized data type");
    return ERROR;
}

/* add convert a value of any type to double.
 */
static Error todouble(Type t, Pointer v1, int i1, double *v2)
{
    switch (t) {
      case TYPE_UBYTE:  TYPE_TO(ubyte,  v1, i1, v2);  return OK;
      case TYPE_BYTE:   TYPE_TO(byte,   v1, i1, v2);  return OK;
      case TYPE_USHORT: TYPE_TO(ushort, v1, i1, v2);  return OK;
      case TYPE_SHORT:  TYPE_TO(short,  v1, i1, v2);  return OK;
      case TYPE_UINT:   TYPE_TO(uint,   v1, i1, v2);  return OK;
      case TYPE_INT:    TYPE_TO(int,    v1, i1, v2);  return OK;
      case TYPE_FLOAT:  TYPE_TO(float,  v1, i1, v2);  return OK;
      case TYPE_DOUBLE: TYPE_TO(double, v1, i1, v2);  return OK;
      case TYPE_HYPER: case TYPE_STRING: break;
    }

    DXSetError(ERROR_NOT_IMPLEMENTED, "unrecognized data type");
    return ERROR;
}

/* check if value1 when cast to double equals 0.0.  1 is true, 0 is false
 */
static int my_iszero (Type t, Pointer v1, int i1)
{
    int rc; 

    switch (t) {
      case TYPE_UBYTE:  TYPE_EQ(ubyte,  v1, i1, rc);  return rc;
      case TYPE_BYTE:   TYPE_EQ(byte,   v1, i1, rc);  return rc;
      case TYPE_USHORT: TYPE_EQ(ushort, v1, i1, rc);  return rc;
      case TYPE_SHORT:  TYPE_EQ(short,  v1, i1, rc);  return rc;
      case TYPE_UINT:   TYPE_EQ(uint,   v1, i1, rc);  return rc;
      case TYPE_INT:    TYPE_EQ(int,    v1, i1, rc);  return rc;
      case TYPE_FLOAT:  TYPE_EQ(float,  v1, i1, rc);  return rc;
      case TYPE_DOUBLE: TYPE_EQ(double, v1, i1, rc);  return rc;
      case TYPE_HYPER: case TYPE_STRING: break;
    }

    return 0;
}

