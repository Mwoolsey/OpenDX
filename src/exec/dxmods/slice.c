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
#include <ctype.h>
#include <math.h>

/* struct for passing parameters around
 */ 
struct argblk {
    Object toslice;             /* object to process */
    int slicedim;               /* dimension to make slices along */
    int slicecount;             /* number of slices to make */
    int *sliceplace;            /* connection nums at which to make slice */
    int numdim;                 /* dimensionality of input object */
    Object compositeseries;     /* if !NULL, multiple slices through CF */
    int compositecount;         /* number of connections in complete CF */
    float *compositepositions;  /* positions list for complete CF */
    Object outgroup;            /* if !NULL, parent group for slices */
    char *membername;		/* if !NULL, slice name */
    float seriesplace;		/* if series, series position */
    int membernum;		/* if group, enumerated member */
    int goparallel;		/* true/false flag */
    int childtasks;		/* count of parallel child tasks */
    int nprocs;			/* 4 * (number of processors - 1) */
};
 
 
static Object Object_Slice(Object o, struct argblk *a);
static Object Field_Slice(Field o, struct argblk *a);
static Array Array_Slice(Array a, int dim, int pos, int num, int *counts, 
			 int flag);
static Error Slice_Wrapper(Pointer a);
static Error Slice_Wrapper2(Pointer a);
static Error Slice_Wrapper3(Pointer a);

static Error todouble(Type t, Pointer v1, int i1, double *v2);
static Error fromdouble(Type t, double v1, Pointer v2, int i2);

static Error numslices(CompositeField f, struct argblk *a);
static Error fixtype(Series s);
extern void _dxfPermute(int dim, Pointer out, int *ostrides, int *counts,  /* from libdx/permute.c */
		    int size, Pointer in, int *istrides);

/* the source for these is in partreg.c */
extern Field _dxf_has_ref_lists(Field f, int *pos, int *con); /* from libdx/partreg.c */
extern Error _dxf_make_map_template(Array a, Array *map); /* from libdx/partreg.c */
extern Error _dxf_fix_map_template(Array map, Array newmap); /* from libdx/partreg.c */
extern Array _dxf_remap_by_map(Array a, Array map); /* from libdx/partreg.c */
 
#define IsEmpty(o)  (DXGetObjectClass(o)==CLASS_FIELD && DXEmptyField((Field)o))

 
#define MAXSHAPE   128        /* XXX - system constant */
#define MAXDIM     8          /* XXX - system constant */


/* 0 = input
 * 1 = dim number
 * 2 = position(s)
 *
 * 0 = output
 */
int
m_Slice(Object *in, Object *out)
{
    char *cp, cdim;
    struct argblk a;

 
    /* initialize things which need it - especially anything which may be 
     *  allocated - it makes the error exit code simpler.  
     */
    a.compositeseries = NULL;		/* object */
    a.compositepositions = NULL;	/* memory */
    a.sliceplace = NULL;		/* memory */
    a.compositecount = 0;		/* integer */
    a.outgroup = NULL;			/* object */
    a.childtasks = 0;			/* integers */
    a.goparallel = 1;
    a.childtasks = 0;
    a.nprocs = (DXProcessors(0) - 1) * 4;

    /* input object */
    if (!in[0])  {
	DXSetError(ERROR_BAD_PARAMETER, "#10000", "input");
	return ERROR;
    }
 
    /* dimension to slice along.  defaults to 0 */
    if (in[1]) {
        /* accept either an integer or the letters x,y or z */
	if (!DXExtractInteger(in[1], &a.slicedim)) {
            if (DXExtractString(in[1], &cp)) {
                cdim = isupper(cp[0]) ? tolower(cp[0]) : cp[0];
                if (cdim < 'x' || cdim > 'z') {
                    DXSetError(ERROR_BAD_PARAMETER, "bad dimension letter");
		    return ERROR;
		}
                a.slicedim = (int)(cdim - 'x');
            } else {
		DXSetError(ERROR_BAD_PARAMETER, "#10620", "dimension number");
		return ERROR;
	    }
        }
        if (a.slicedim < 0) {
            DXSetError(ERROR_BAD_PARAMETER, "#10020", "dimension number");
	    return ERROR;
	}
    } else
	a.slicedim = 0;
    
    
    /* where to make the slice(s).  either a single connection number
     *  or a list of connection numbers along the dimension where the
     *  slices are to be taken.  the default is to pass a count of 0
     *  and a NULL pointer into the worker routine, which will make a
     *  series of slices, one at each connection of the input object.
     */
    if (in[2]) {
	if (!DXQueryParameter(in[2], TYPE_INT, 1, &a.slicecount)) {
	    DXSetError(ERROR_BAD_PARAMETER, "#10050", "positions");
	    return ERROR;
	}
	
	if (a.slicecount <= 0) {
	    DXSetError(ERROR_BAD_PARAMETER, "#10050", "positions");
	    return ERROR;
	}
        
        if (!(a.sliceplace = (int *)DXAllocate(sizeof(int) * a.slicecount)))
            return ERROR;
        
        if (!DXExtractParameter(in[2], TYPE_INT, 1, 
			      a.slicecount, (Pointer)a.sliceplace)) {
            DXSetError(ERROR_BAD_PARAMETER, "#10050", "positions");
            goto error;
        }
        
    } else
	a.slicecount = 0;
    
    
    /* allow empty fields to pass thru unchanged */
    if (IsEmpty(in[0])) {
	out[0] = in[0];
	return OK;
    }

    /* see if there is at least one field in the input somewhere */
    if(!DXExists(in[0], "positions")) {
	DXSetError(ERROR_DATA_INVALID, "#10190", "input");
	return ERROR;
    }
    
    out[0] = Object_Slice(in[0], &a);

    /* if we want to make it so that if the slice location is outside
     *  the object, that it isn't an error, but just returns an empty
     *  object, then add this code here.
     */
#if 1    /* EMPTY_NOT_AN_ERROR */
    if (!out[0] && DXGetError()==ERROR_NONE)
        out[0] = (Object)DXEndField(DXNewField());
#endif

    if (!out[0])
        goto error;

    DXFree((Pointer)a.compositepositions);
    DXFree((Pointer)a.sliceplace);
    return OK;

  error:
    DXDelete((Object)a.compositeseries);
    DXFree((Pointer)a.compositepositions);
    DXFree((Pointer)a.sliceplace);
    DXDelete((Object)a.outgroup);

    return ERROR;
}



/* recursive worker routine.
 *  input is an object and a filled in parameter block.
 */
static Object Object_Slice(Object o, struct argblk *a)
{
    Object newo, newo2;
    Object subo, subo2;
    char *name;
    Matrix m;
    float position;
    int fixed, z;
    int i, newi, numparts = 0;
    int goparallel = a->goparallel;

    if (a->childtasks >= a->nprocs)
	goparallel = 0;
 
    switch(DXGetObjectClass(o)) {
 
      case CLASS_GROUP:
	switch (DXGetGroupClass((Group)o)) {
	  case CLASS_COMPOSITEFIELD:   /* partitioned data */

            /* three different cases:
             *  number of slices == 0 (create slices at all locations)
             *  number of slices == 1 (create a single slice)
             *  number of slices > 1 (multiple slices at user selected locations)
             */


	    if (!numslices((CompositeField)o, a))
		return NULL;


            /* compute the number of connections in the composite field,
             *  and their actual location.  input is an object divided
             *  into fields based on the locations of the positions of the
             *  points.  output is a series object, where each field is a
             *  partitioned slice.  the series position is the original 
             *  location of the origin of the slice.  eventually this will
             *  run in parallel, so create a series group with initially empty
             *  composite fields to hold the slices here.
             */

            if (a->slicecount == 1)
                goto normalgroup;


            a->compositeseries = (Object)DXNewSeries();
            if (!a->compositeseries)
                return NULL;

            /* for each new slice */
	    for (i=0; i < a->slicecount; i++) {
                newo = DXCopy(o, COPY_ATTRIBUTES);
                if (!newo)
                    return NULL;

                if (!DXSetSeriesMember((Series)a->compositeseries, i,
                                     (double)a->compositepositions[i], newo)) {
                    DXDelete(newo);
                    return NULL;
                }
            }
	    
	    if (goparallel) {
		if (!DXCreateTaskGroup())
		    return NULL;

		/* add the total number of subtasks which will be added to the
		 *  task queue here, so when the child tasks decide whether
		 *  they can go parallel, they know how many previous tasks
		 *  have been started.
		 */
		DXGetMemberCount((Group)o, &numparts);
		a->childtasks += numparts;
	    }


            /* for each original partition */
            for (i=0; (subo=DXGetEnumeratedMember((Group)o, i, &name)); i++) {
                if (DXEmptyField((Field)subo))
                    continue;
		
		if (goparallel) {
		    a->toslice = subo;
		    if (!DXAddTask(Slice_Wrapper, (Pointer)a, 
				 sizeof(struct argblk), 1.0)) {
			DXAbortTaskGroup();
			return NULL;
		    }

		} else if (!Field_Slice((Field)subo, a))
		    return NULL;
		    
	    }

	    if (goparallel) {
		if (!DXExecuteTaskGroup())
		    return NULL;

		a->childtasks -= numparts;
	    }
		
	    /* and now bubble the data type up from the first non-empty
             *  composite field and set the series group type.
	     */
	    if (!fixtype((Series)a->compositeseries))
		return NULL;

            o = a->compositeseries;
	    break;
 
	  default: 	    /* normal groups */

          normalgroup:
            newo2 = DXCopy(o, COPY_ATTRIBUTES);
            if (!newo2)
                return NULL;

            /* for each member */
	    if (goparallel) {
		a->outgroup = newo2;
		if (!DXCreateTaskGroup())
		    return NULL;

		/* add the total number of subtasks which will be added to the
		 *  task queue here, so when the child tasks decide whether
		 *  they can go parallel, they know how many previous tasks
		 *  have been started.
		 */
		DXGetMemberCount((Group)o, &numparts);
		a->childtasks += numparts;
	    }

            newi = 0;
	    for (i=0; (subo=DXGetEnumeratedMember((Group)o, i, &name)); i++) {
		if (goparallel) {
		    a->toslice = subo;
		    a->membername = name;
		    
		    if (!DXAddTask(Slice_Wrapper2, (Pointer)a, 
				 sizeof(struct argblk), 1.0)) {
			DXAbortTaskGroup();
			return NULL;
		    }

		} else {
		    if (!(newo = Object_Slice(subo, a))) {
			if (DXGetError()==ERROR_NONE)
			    continue;
			
			DXDelete(newo2);
			return NULL;
		    }
		    
		    /* add new object to copy of input, with the same name if
		     *  input is named - else add as an unnamed member.
		     */
		    name ? DXSetMember((Group)newo2, name, newo) :
			   DXSetEnumeratedMember((Group)newo2, newi, newo);

                newi++;
		}
	    }

	    if (goparallel) {
		if (!DXExecuteTaskGroup())
		    return NULL;
		a->childtasks -= numparts;
	    }
            o = newo2;
	    break;
 
	  case CLASS_SERIES:
            newo2 = DXCopy(o, COPY_HEADER);
            if (!newo2)
                return NULL;

            /* for each member */
	    if (goparallel) {
		a->outgroup = newo2;
		if (!DXCreateTaskGroup())
		    return NULL;

		/* add the total number of subtasks which will be added to the
		 *  task queue here, so when the child tasks decide whether
		 *  they can go parallel, they know how many previous tasks
		 *  have been started.
		 */
		DXGetMemberCount((Group)o, &numparts);
		a->childtasks += numparts;
	    }

            /* for each member */
	    for (i=0; (subo=DXGetSeriesMember((Series)o, i, &position)); i++) {
		if (goparallel) {
		    a->toslice = subo;
		    a->seriesplace = position;
		    a->membernum = i;
		    
		    if (!DXAddTask(Slice_Wrapper3, (Pointer)a, 
				 sizeof(struct argblk), 1.0)) {
			DXAbortTaskGroup();
			return NULL;
		    }

		} else {
		    if (!(newo = Object_Slice(subo, a))) {
			if (DXGetError()==ERROR_NONE)
			    /* for a series, have to create a placeholder */
			    newo = (Object)DXEndField(DXNewField());
			else {
			    /* error code was set.  delete copy, return error.
			     */
			    DXDelete(newo2);
			    return NULL;
			}
			
		    }

		    if(!DXSetSeriesMember((Series)newo2, i, position, newo)) {
			DXDelete(newo2);
			return NULL;
		    }
		}
		
	    }
	    if (goparallel) {
		if (!DXExecuteTaskGroup())
		    return NULL;
		a->childtasks -= numparts;
	    }
	    o = newo2;
	    break;
	}
	
        /* end of group objects */
	break;
	
 
      case CLASS_FIELD:
	if (DXEmptyField((Field)o))
	    break;
	
	o = Field_Slice((Field)o, a);
	break;
 
 
      case CLASS_XFORM:
	if (!DXGetXformInfo((Xform)o, &subo, &m))
	    return NULL;
 
	if (!(newo=Object_Slice(subo, a)))
	    return NULL;
 
	o = (Object)DXNewXform(newo, m);
	break;
 
 
      case CLASS_SCREEN:
	if (!DXGetScreenInfo((Screen)o, &subo, &fixed, &z))
	    return NULL;
 
	if (!(newo=Object_Slice(subo, a)))
	    return NULL;
 
	o = (Object)DXNewScreen(newo, fixed, z);
	break;
 
 
      case CLASS_CLIPPED:
	if (!DXGetClippedInfo((Clipped)o, &subo, &subo2))
	    return NULL;
 
	if (!(newo=Object_Slice(subo, a)))
	    return NULL;
 
	o = (Object)DXNewClipped(newo, subo2);
	break;
 
	
      default:
	/* any remaining objects can't contain fields, so we can safely
	 *  pass them through unchanged - just make an extra copy.
	 */
        o = DXCopy(o, COPY_STRUCTURE);
	break;
 
    }
 
    return o;
}
 
 
/*
 */
static Error Slice_Wrapper(Pointer a)
{
    Object o;

    o = Field_Slice((Field)((struct argblk *)a)->toslice, (struct argblk *)a);
    return (o ? OK : ERROR);
}

/*
 */
static Error Slice_Wrapper2(Pointer a)
{
    struct argblk *ap = (struct argblk *)a;
    struct argblk a2;
    Object o;

    memcpy((char *)&a2, (char *)a, sizeof(struct argblk));

    o = Object_Slice(a2.toslice, &a2);

    if (!o)
	return ((DXGetError() == ERROR_NONE) ? OK : ERROR);

    if (IsEmpty(o))
	return OK;
    
    DXSetMember((Group)ap->outgroup, ap->membername, o);
    
    return OK;
}

/*
 */
static Error Slice_Wrapper3(Pointer a)
{
    struct argblk *ap = (struct argblk *)a;
    struct argblk a2;
    Object o;

    memcpy((char *)&a2, (char *)a, sizeof(struct argblk));

    o = Object_Slice(a2.toslice, &a2);

    if (!o && (DXGetError() != ERROR_NONE))
	return ERROR;

    if (!o || (IsEmpty(o)))
	o = (Object)DXEndField(DXNewField());
    
    DXSetSeriesMember((Series)ap->outgroup, ap->membernum, ap->seriesplace, o);
    
    return OK;
}

/*
 * 
 */
static Object Field_Slice(Field o, struct argblk *a)
{
    Series s = NULL;
    Object newo, subo;
    Array ac = NULL;
    Array ap = NULL;
    Array ao;
    Field f = NULL;
    char *name, *dattr, *rattr;
    int i, j, k, n;
    int justone = 0;
    int start;
    int rank = 0;
    int stride[MAXDIM];
    int counts[MAXDIM], ccounts[MAXDIM], shape[MAXDIM];
    int gridoffset[MAXDIM], ngridoffset[MAXDIM];
    float origins[MAXDIM], deltas[MAXDIM*MAXDIM];
    float worldposition, scratch[MAXDIM];
    float *fp;
    ArrayHandle ah;
    int refpos, refcon;
    Array posmap = NULL;
    
 
    /* have to do positions & connections first, so we know the
     *  original shape.  then other components can be sliced.
     *  do a quick check on connections to be sure they're regular
     *  before we start.
     */
    ao = (Array)DXGetComponentValue(o, "connections");
    if (!ao) {  
	DXSetError(ERROR_MISSING_DATA, "#10240", "connections");
	return NULL;
    }
    
    if (!DXQueryGridConnections(ao, &a->numdim, ccounts)) {
	DXSetError(ERROR_BAD_TYPE, "#10610", "input");
	return NULL;
    }
    
    if (a->slicedim >= a->numdim) {
	DXSetError(ERROR_BAD_PARAMETER, "#11360", a->slicedim, "input", 
		 a->numdim-1);
	return NULL;
    }
    
    /* all fields have offsets.  if the input isn't partitioned, it still
     *  returns 0 - but if you extract a field from a composite field, it
     *  may still have offsets.  set them to zero if you didn't get a CF.
     */
    if (a->compositepositions) {
	if (!DXGetMeshOffsets((MeshArray)ao, gridoffset))
	    return NULL;
    } else
	memset((char *)gridoffset, '\0', sizeof(int)*MAXDIM);

    /* if the input isn't partitioned, set the limits from this single field.
     */
    if (a->compositecount == 0)
	a->compositecount = ccounts[a->slicedim];

    
    /* if they gave us a position list, check it for out of range values 
     */
    for (i=0; i<a->slicecount; i++) {
	if (a->sliceplace[i] < 0 || a->sliceplace[i] >= a->compositecount) {
	    if (a->slicecount > 1) {
		DXSetError(ERROR_BAD_PARAMETER, "#10041",
			 "position item", i+1, 0, a->compositecount - 1);
		return NULL;
	    } else {
		DXSetError(ERROR_BAD_PARAMETER, "#10040", 
			 "position", 0, a->compositecount - 1);
		return NULL;
	    }
	}
	
    }
    
    /* if they didn't give us a list, initialize a positions list.
     *  making a slice at every connection.
     */
    if (a->slicecount == 0) {
	
	a->slicecount = a->compositecount;
	
	if (!(a->sliceplace = (int *)DXAllocate(a->slicecount * sizeof(int))))
	    return NULL;
	
	for (i=0; i<a->slicecount; i++)
	    a->sliceplace[i] = i;
    }
 
 
    /* if the input is partitioned, then an output field has already been
     *  initialized.  if there is more than one slice, then the output
     *  will be a new series, with each slice in a separate field.  otherwise
     *  the input is a simple field and the output is a simple field - make
     *  a copy of the input a little further down and work on it.
     */
    if (!a->compositeseries) {
	if (a->slicecount != 1) {
	    if (!(s = DXNewSeries()))
		return NULL;
	} else
	    justone++;
    }
 
    /* loop here for each position */
    for (i=0; i<a->slicecount; i++) {
	
	/* check here to see if the slice is inside this partition.
	 *  if slices, each partition excludes the starting face, except
         *  for the very first partition, to prevent creating 2 slices at
         *  the same location because of internal partition boundaries, but
         *  includes the ending face. 
	 */

	/* these are start and end position number, relative to this
	 *  partition.
	 */	
	start = a->sliceplace[i] - gridoffset[a->slicedim];

	/* completely outside?
         */
	if (start < 0 ||  (start >= ccounts[a->slicedim]))
	    continue;
	
	/* don't include partition minimum.
	 */
	if (a->sliceplace[i] != 0 && start <= 0)
	    continue;


	/* if there is only 1 slice, use a copy of the existing field.  
	 *  else, create a new Field to be added to the Series.
	 */
	if (justone)
	    f = (Field)DXCopy((Object)o, COPY_STRUCTURE);
	else {
	    f = DXNewField();
	    DXCopyAttributes((Object)f, (Object)o);
	}
	if (!f)
	    goto error;
	
	/*
	 * do the connections 
	 */
	for (j=0; j<a->numdim; j++) {
	    counts[j] = ccounts[j];
	    ngridoffset[j] = gridoffset[j];
	}

	/* object goes down 1 dimension in connection shape.
         *  treat lines going to a point as a special case - points don't
	 *  have a connections component.
	 */
	if (a->numdim > 1) {
	    for (j=a->slicedim+1; j<a->numdim; j++) {
		counts[j-1] = ccounts[j];
		ngridoffset[j-1] = gridoffset[j];
	    }
	    
	    if (!(ac = DXMakeGridConnectionsV(a->numdim-1, counts))
		|| !DXSetStringAttribute((Object)ac, "ref", "positions")
		|| !DXSetMeshOffsets((MeshArray)ac, ngridoffset)) {
		DXDelete((Object)ac);
		goto error;
	    }
	}
	
	/* if the new connections type would be points, either remove the
	 *  existing connections component if just one field, or for a
	 *  series, don't add a connections component.
	 */
	if (justone)
	    if (!DXDeleteComponent(f, "connections"))
		goto error;
	
	if (a->numdim > 1) {
	    if (!DXSetComponentValue(f, "connections", (Object)ac))
		goto error;
	    
	    if (justone)
		if (!DXChangedComponentValues(f, "connections"))
		    goto error;
	}
	
    
	/* now check the positions component */
	ap = (Array)DXGetComponentValue(o, "positions");
	if (!ap) {
	    DXSetError(ERROR_MISSING_DATA, "#10240", "positions");
	    return NULL;
	}
    
    
	/* new positions.  get the actual position value for the starting
	 *  connection number.  then slice the positions array.
	 */
 
	/* if regular, compute position.  if irregular, index into data
	 *  array to find it.
	 */
	if (DXQueryGridPositions(ap, &n, NULL, origins, deltas)) {
	    if (n != a->numdim) {
		DXSetError(ERROR_DATA_INVALID, "#11161", 
			   "connection dimensions", a->numdim,
			   "position dimensions", n);
		return NULL;
	    }
	    worldposition = origins[a->slicedim];
	    for (j=0; j<n; j++)
	        worldposition += start * deltas[n*j + a->slicedim];
	} else {
            DXGetArrayInfo(ap, NULL, NULL, NULL, &rank, shape);
	    if (rank != 1 || shape[0] != a->numdim) {
		DXSetError(ERROR_DATA_INVALID, "#11161", 
			   "connection dimensions", a->numdim,
			   "position dimensions", shape[0]);
		return NULL;
	    }
	    k = 1;
	    for (j=a->numdim-1; j>=0; j--) {
		stride[j] = k;
		k *= ccounts[j];
	    }

	    ah = DXCreateArrayHandle(ap);
	    if (!ah)
		goto error;
            fp = (float *)DXGetArrayEntry(ah, start*stride[a->slicedim], 
					  (Pointer)scratch);
	    worldposition = fp[a->slicedim];

#if 0  /* very broken, and in dx 3.1.0 */
	    worldposition = *(float *)DXGetArrayEntry(ah, stride[a->slicedim],
						      (Pointer)scratch);
#endif
	    DXFreeArrayHandle(ah);
	}
	
	/* check for 1d point */
	if (a->numdim > 1) {
	    ap = Array_Slice(ap, a->slicedim, start, a->numdim, ccounts, 1);
	    if (!ap) {
		DXDelete((Object)ac);
		goto error;
	    }
	} else {
	    ap = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, 1);
	    if (!DXAddArrayData(ap, 0, 1, (Pointer)&worldposition)) {
		DXDelete((Object)ac);
		goto error;
	    }
	}
 
	
	if (!DXSetComponentValue(f, "positions", (Object)ap)
	||  (justone && !DXChangedComponentValues(f, "positions"))) {
	    DXDelete((Object)ac);
	    DXDelete((Object)ap);
	    goto error;
	}



	/* check to see if we will need to make a map for any ref components.
	 *  build a map for positions.
	 */
	
	_dxf_has_ref_lists(o, &refpos, &refcon);

	/* can't do invalid connections - can't slice connection dep data.
	 */
	if (refcon) {
	    DXSetError(ERROR_NOT_IMPLEMENTED, 
		       "cannot slice components which `ref' connections");
	    goto error;
	}

	/* make a refpos map.
	 * the map is made by making a regular array of 0-N items and then
	 * permuting them the same way we are going to permute the positions.
	 */
	if (refpos) {
	    Array tmap = NULL;
	    
	    ap = (Array)DXGetComponentValue(o, "positions");
	    if (!ap)
		goto error;
	    
	    if (!_dxf_make_map_template(ap, &posmap))
		goto error;
	    
	    tmap = Array_Slice(posmap, a->slicedim, start,
			       a->numdim, ccounts, 0);
	    
	    if (!tmap)
		goto error;
	    
	    if (!_dxf_fix_map_template(posmap, tmap)) {
		DXDelete((Object) tmap);
		goto error;
	    }
	    
	    DXDelete((Object) tmap);
	    /* posmap now captures where the positions have moved to */
	}
 
	
	/* for each of the remaining components:
	 *   skip connections & positions - we've already sliced them
	 *   if it doesn't dep on positions or connections;
         *    for a single slice (therefore a single output field), we've
	 *    already made a copy of all the components, so just continue.
	 *    for a series, each slice is a separate field, so the component
	 *    must be copied to the new field.
	 *   if it does dep on positions or connections:
	 *    slice the new component and add it to the field.  for single
	 *    slices, this replaces the old component.  for series of slices,
	 *    this adds the component for the first time.
	 */
	for (j=0; (subo=DXGetEnumeratedComponentValue(o, j, &name)); j++) {
	    
	    if (!strcmp("positions", name) || !strcmp("connections", name))
		continue;
	    
	    /* skip derived components */
	    if (DXGetStringAttribute(subo, "der", NULL) != NULL) {
		if (justone)
		    DXDeleteComponent(f, name);
		
		continue;
	    }
	    
	    /* if dep on positions, or ref positions, slice.
	     */
	    if (!DXGetStringAttribute(subo, "dep", &dattr))
		dattr = NULL;
	    if (!DXGetStringAttribute(subo, "ref", &rattr))
		rattr = NULL;

	    if (dattr && !strcmp("positions", dattr)) {
		
		newo = (Object)Array_Slice((Array)subo, a->slicedim, start,
					   a->numdim, ccounts, 0);
		
	    }
	    else if (dattr && !strcmp("connections", dattr)) {
		
                DXSetError(ERROR_NOT_IMPLEMENTED, 
			   "cannot slice connection-dependent %s", name);
		goto error;
		
	    }
	    else if (rattr && !strcmp("positions", rattr)) {
		
		newo = (Object) _dxf_remap_by_map((Array)subo, posmap);

	    } else if (rattr && !strcmp("connections", rattr)) {

		DXSetError(ERROR_NOT_IMPLEMENTED, 
			   "cannot slice components which `ref' connections");
		goto error;

	    } else {

		if (!justone)
		    f = DXSetComponentValue(f, name, subo);
		
		continue;
		
	    }
	    
	    if (!newo
	    || !DXSetComponentValue(f, name, newo)
	    || !DXChangedComponentValues(f, name)
	    || !DXCopyAttributes(subo, newo)) {
		DXDelete(newo);
		goto error;
	    }

	}
	
	
	if (!DXEndField(f))
	    goto error;

	/* if the output is a series of composite fields, get the matching
         *  group member and add this new field to it.
	 * else if the input is a simple field and there is more than one
         *  slice, add this field as part of a series.
         * else the input is a simple field and the output is only a single
	 *  slice - there is nothing extra to do here.
	 */
	if (a->compositeseries) {
	    subo = DXGetSeriesMember((Series)a->compositeseries, i, 
				                               &worldposition);
	    if (!subo)
		goto error;

	    if (!DXSetMember((Group)subo, NULL, (Object)f))
		goto error;

	} else if (!justone) {
	    if (!DXSetSeriesMember(s, i, worldposition, (Object)f))
		goto error;
 
	}
    }

    
    if (a->compositeseries)
	return (Object)a->compositeseries;
    else if (!justone)
	return (Object)s;
    else
	return (Object)f;
 
error:
    DXDelete((Object)s);
    DXDelete((Object)f);
    return NULL;
}
 
/* slice an array.  args are:
 *  array to slice
 *  connection dimension number along which to slice
 *  connection offset along that dimension at which to start slicing
 *  number of original dimensions in the connections
 *  number of counts along the original dimensions in the connections
 *  if flag set, slice contents of array also (e.g. for positions)
 */
static Array 
Array_Slice(Array a, int dim, int pos, int num, int *ccounts, int flag)
{
    Array na = NULL, na2 = NULL, terms[MAXDIM];
    int nitems, itemsize;
    Type t;
    Category c;
    double o_x[MAXDIM], d_x[MAXDIM*MAXDIM];
    Pointer origins = o_x, deltas = d_x;
    float *ofp, *ifp;
    int rank, shape[MAXSHAPE], counts[MAXDIM], ncounts[MAXDIM];
    int ostride[MAXDIM], nstride[MAXDIM];
    int dims[MAXDIM], tempdims[MAXDIM];
    double tvalue1, tvalue2;
    Pointer op, np;
    int i, j, k, l;
    
    
    /* check here for fully regular array and shortcut the following code */
    
    if (DXQueryGridPositions(a, &i, counts, 
			     (float *)origins, (float *)deltas)) {
	
	int temp = 0;
	int dimtemp = dim;
	
	/* dim of this array less than connections? like quads in 1 space? 
	 * expand it to get the right correspondance.  same if the slice
	 * dim is already greater than the dim of this array.
	 */
	if (i < num || dim >= i)
	    goto expanded;

	/* if you are going to slice the contents of each array item in
	 * addition to slicing the total number of items, you can't use
	 * this code.
	 */
	if (flag == 1 && i != num)
	    goto expanded;

        /* if dim of array doesn't match dim of connections, expand them
         *  to be sure you get the right ones.
         */
        if (i > num) {
	    for (j=0; j<i; j++) {
		if (counts[j] <= 1) {
		    temp++;
		    if (dimtemp >= j) 
			dimtemp++;
		} else {
		    if (counts[j] != ccounts[j-temp])
			goto expanded;
		}
	    }
	    
	    /* make sure the dimensions match before you try to slice
	     * something which might not really be the same shape.
	     */
	    if (i != num+temp)
		goto expanded;
	    
	    dim = dimtemp;
	} else {
	    /* even if the dims match, check the counts to be sure the
	     * shapes are the same.
	     */
	    for (j=0; j<i; j++)
		if (counts[j] != ccounts[j])
		    goto expanded;
	}
	
	   
	/* it should be safe here to assume this array matches the connections
	 * and we can leave it regular.
	 *
	 * remove the sliced dimension 
	 */
	for (j=dim; j<i-1; j++) {
	    counts[j] = counts[j+1];
	    ((float *)origins)[j] = ((float *)origins)[j+1];
	}
	
	if (flag == 1) {
	    int realdelta;
	    int whichdim;

	    /*
	     * if only one axis varies in this dim, cut it out.
	     * (this is new code and may not match exactly what
	     *  happens if the data is irregularized first, but
	     *  it is a requested function, so i'll try it out...)
	     */
	    realdelta = 0;
	    whichdim = -1;
	    for (j=0; j<i; j++) {
		if (((float *)deltas)[j*i + dim] != 0.0) {
		    if (realdelta > 0)
			goto expanded;
		    whichdim = j;
		    realdelta++;
		}
	    }
	    if (whichdim < 0)
		whichdim = dim;
	    for (j=0; j<i; j++) {
		if (j == dim)
		    continue;
		if(((float *)deltas)[whichdim*i + j] != 0.0)
		    goto expanded;
	    }
	    
	    ifp = ofp = (float *)deltas;
	    for(j=0; j<i; j++) {
		if(j == whichdim)
		    ofp += i;
		
		else for(k=0; k<i; k++)
		    if(k == dim) 
			ofp++;
		    else
			*ifp++ = *ofp++;
	    }
	}
	
	na = DXMakeGridPositionsV(i-1, counts, (float *)origins, (float *)deltas);
	DXCopyAttributes((Object)na, (Object)a);
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

	na = DXNewArrayV(t, c, rank, shape);
	DXCopyAttributes((Object)na, (Object)a);
	
	np = DXGetArrayData(DXAddArrayData(na, 0, nitems/ccounts[dim], NULL));
	if(!np)
	    goto error;

	if (num <= 1) {
	    /* don't bother with Permute */
	    op = (Pointer)((char *)op + pos * itemsize);
	    memcpy(np, op, itemsize);
	    return na;
	}

	/* try for one general routine, and be prepared to optimize
	 * on slicedim == numdim, and itemsize == sizeof(float)
	 */
	
	k = 0;
	for(i=0; i<dim; i++)
	    dims[k++] = i;
	for(i=dim+1; i<num; i++)
	    dims[k++] = i;
	
	
	/* old strides - w/ sliced dim */
	k = 1;
	for (i=num-1; i>=0; i--) {
	    tempdims[i] = k;
	    k *= ccounts[i];
	}
	
	/* rearrange dims */
	for (i=num-2; i>=0; i--) {
	    ostride[i] = tempdims[dims[i]];	
	    ncounts[i] = ccounts[dims[i]];
	}
	
	/* new strides - w/o sliced dim */
	k = 1;
	for (i=num-1; i>dim; i--) {
	    nstride[i-1] = k;
	    k *= ncounts[i-1];
	}
	for (i=dim-1; i>=0; i--) {
	    nstride[i] = k;
	    k *= ncounts[i];
	}
	
	op = (Pointer)((char *)DXGetArrayData(a) + pos*tempdims[dim]*itemsize);
	np = (Pointer)DXGetArrayData(na);
	if (!op || !np)
	    goto error;

#if 0
	DXMarkTimeLocal("begin permute");
#endif
	_dxfPermute(num-1, np, nstride, ncounts, itemsize, op, ostride);
#if 0
	DXMarkTimeLocal("end permute");
#endif
	
	if(!flag)
	    return na;

	/* take care of shape here, because i don't think the other
	 *  cases need to worry about it.  this is inefficient, since
	 *  we are traversing the data array twice - once to cut out
	 *  the items in the sliced dimension, and once to remove the
	 *  sliced dimension from each data item.
	 */
	DXGetArrayInfo(na, &nitems, &t, &c, &rank, shape);
	if(rank != 1) {
	    DXSetError(ERROR_DATA_INVALID, 
		"array contents shape must be 1");
	    goto error;
	}
        if (shape[0] != num) {
	    DXSetError(ERROR_DATA_INVALID,
		 "array contents dimensions must match connection dimensions");
	    goto error;
	}
        if (shape[0] <= 1) {
	    DXSetError(ERROR_DATA_INVALID,
		 "can't slice scalar array contents");
	    goto error;
	}

	--shape[0];
	
	na2 = DXNewArrayV(t, c, rank, shape);
	DXCopyAttributes((Object)na2, (Object)na);
	if(!na2)
	    goto error;
	
	if (t != TYPE_FLOAT) {
	    DXSetError(ERROR_NOT_IMPLEMENTED, "only float contents");
	    goto error;
	}

	ifp = (float *)DXGetArrayData(na);
	ofp = (float *)DXGetArrayData(DXAddArrayData(na2, 0, nitems, NULL));
	if(!ofp)
	    goto error;
	
	
	/* delete a column from the array (copy, not in place) */
	for(k=0; k < nitems; k++) {
            
	    for(l=0; l<dim; l++)
		ofp[l] = ifp[l];
	    
	    for(l=dim; l<shape[0]; l++)
		ofp[l] = ifp[l+1];
	    
	    ofp += shape[0];
	    ifp += shape[0] + 1;
	}
	
	DXDelete((Object)na);
	na = na2;

	break;

	
      case CLASS_REGULARARRAY:
	/* the output slice can't still be regular unless
         *  you are slicing along the slowest varying dimension...
	 */
	if (dim != 0)
	    goto expanded;

	/* need shape[0] from this */
	DXGetArrayInfo(a, NULL, &t, NULL, NULL, shape);
	DXGetRegularArrayInfo((RegularArray)a, &nitems, origins, deltas);

        if (shape[0] != num && flag == 1) {
	    DXSetError(ERROR_DATA_INVALID,
		 "array contents dimensions must match connection dimensions");
	    return NULL;
	}

	/* compute the new origin and correct the counts 
	 */
	k = 1;
	for (i=num-1; i>0; --i)
	    k *= ccounts[i];
	k *= pos;
	
	for (j=0; j<shape[0]; j++) {
	    todouble(t, deltas, j, &tvalue1);
	    todouble(t, origins, j, &tvalue2);
	    
	    tvalue2 += tvalue1 * k;
	    
	    fromdouble(t, tvalue2, origins, j);
	}
	nitems /= ccounts[dim];

	if (flag == 0) {
	    na = (Array)DXNewRegularArray(t, shape[0], nitems, origins, deltas);
	    DXCopyAttributes((Object)na, (Object)a);
	} else {
	    if (shape[0] == 1) {
		DXSetError(ERROR_DATA_INVALID, "can't slice scalar array");
		return NULL;
	    }
	    
#define Plus(type, var) ((type *)var)[j] = ((type *)var)[j+1];

	    switch (t) {
	      case TYPE_UBYTE:
		for (j=dim; j<shape[0]-1; j++) {
		    Plus(ubyte, deltas); Plus(ubyte, origins);
		}
		break;
		
	      case TYPE_BYTE:
		for (j=dim; j<shape[0]-1; j++) {
		    Plus(byte, deltas); Plus(byte, origins);
		}
		break;
		
	      case TYPE_USHORT:
		for (j=dim; j<shape[0]-1; j++) {
		    Plus(ushort, deltas); Plus(ushort, origins);
		}
		break;

	      case TYPE_SHORT:
		for (j=dim; j<shape[0]-1; j++) {
		    Plus(short, deltas); Plus(short, origins);
		}
		break;

	      case TYPE_UINT:
		for (j=dim; j<shape[0]-1; j++) {
		    Plus(uint, deltas); Plus(uint, origins);
		}
		break;

	      case TYPE_INT:
		for (j=dim; j<shape[0]-1; j++) {
		    Plus(int, deltas); Plus(int, origins);
		}
		break;

	      case TYPE_FLOAT:
		for (j=dim; j<shape[0]-1; j++) {
		    Plus(float, deltas); Plus(float, origins);
		}
		break;

	      case TYPE_DOUBLE:
		for (j=dim; j<shape[0]-1; j++) {
		    Plus(double, deltas); Plus(double, origins);
		}
		break;

	      default:
		DXSetError(ERROR_NOT_IMPLEMENTED, "unrecognized data type");
		return NULL;
	    }

	    na = (Array)DXNewRegularArray(t, shape[0]-1, nitems, origins, 
					  deltas);
	    DXCopyAttributes((Object)na, (Object)a);
	}
	break;
 
      case CLASS_CONSTANTARRAY:
	DXGetArrayInfo(a, &nitems, &t, &c, &rank, shape);

        if ((rank != 1 || shape[0] != num) && flag == 1) {
	    DXSetError(ERROR_DATA_INVALID,
		 "array contents dimensions must match connection dimensions");
	    return NULL;
	}

	nitems /= ccounts[dim];

	if (flag == 0) {
	    na = (Array)DXNewConstantArrayV(nitems, DXGetConstantArrayData(a),
                                           t, c, rank, shape);
	    DXCopyAttributes((Object)na, (Object)a);
            break;
	} else {
	    if (rank != 1) {
		DXSetError(ERROR_DATA_INVALID, 
                    "array contents dimensions must match connection dimensions");
		return NULL;
	    }


	    memcpy(origins, DXGetConstantArrayData(a), DXGetItemSize(a));

	    /* inherit Plus macro from above */
	    switch (t) {
	      case TYPE_UBYTE:
		for (j=dim; j<shape[0]-1; j++) Plus(ubyte, origins); break;
		
	      case TYPE_BYTE:
		for (j=dim; j<shape[0]-1; j++) Plus(byte, origins); break;
		
	      case TYPE_USHORT:
		for (j=dim; j<shape[0]-1; j++) Plus(ushort, origins); break;

	      case TYPE_SHORT:
		for (j=dim; j<shape[0]-1; j++) Plus(short, origins); break;

	      case TYPE_UINT:
		for (j=dim; j<shape[0]-1; j++) Plus(uint, origins); break;

	      case TYPE_INT:
		for (j=dim; j<shape[0]-1; j++) Plus(int, origins); break;

	      case TYPE_FLOAT:
		for (j=dim; j<shape[0]-1; j++) Plus(float, origins); break;

	      case TYPE_DOUBLE:
		for (j=dim; j<shape[0]-1; j++) Plus(double, origins); break;

	      default:
		DXSetError(ERROR_NOT_IMPLEMENTED, "unrecognized data type");
		return NULL;
	    }

	    shape[0]--;

	    na = (Array)DXNewConstantArrayV(nitems, origins, t, c, 
                                            rank, shape);
	    DXCopyAttributes((Object)na, (Object)a);
	}
	break;
 
      case CLASS_PRODUCTARRAY:
	if (flag == 1)
	    goto expanded;

	/* before slicing this like the connections, make sure it is
         *  shaped exactly like the connections.  otherwise, go to the
	 *  irregular case to be sure we get the right items.
	 */
	DXGetProductArrayInfo((ProductArray)a, &j, terms);
	if (j != num || dim > j) 
	    goto expanded;
	for (i=0; i<j; i++) {
	    if (!DXGetArrayInfo(terms[i], &nitems, NULL, NULL, NULL, NULL))
		return NULL;
	    
	    if (nitems != ccounts[i] + 1)
		goto expanded;
	}

	switch(j) {
	  case 0:
	    return NULL;
	  case 1:
	    na = Array_Slice(terms[0], dim, pos, num, ccounts, flag);
	    break;
	  case 2:
	    na = terms[dim];
	    break;
	  default:
	    for (i=dim; i<num-1; i++)
		terms[i] = terms[i+1];

	    na = (Array)DXNewProductArrayV(j-1, terms);
	    DXCopyAttributes((Object)na, (Object)a);
	    break;
	}
	break;
	
	
      case CLASS_MESHARRAY:
	if (flag == 1)
	    goto expanded;

	/* see the comment above in the product array case */
	DXGetMeshArrayInfo((MeshArray)a, &j, terms);
	if (j != num || dim > j) 
	    goto expanded;
	for (i=0; i<j; i++) {
	    if (!DXGetArrayInfo(terms[i], &nitems, NULL, NULL, NULL, NULL))
		return NULL;
	    
	    if (nitems != ccounts[i] + 1)
		goto expanded;
	}

	switch(j) {
	  case 0:
	    return NULL;
	  case 1:
	    na = Array_Slice(terms[0], dim, pos, num, ccounts, flag);
	    break;
	  case 2:
	    na = terms[dim];
	    break;
	  default:
	    for (i=dim; i<num-1; i++)
		terms[i] = terms[i+1];
	    
	    na = (Array)DXNewMeshArrayV(j-1, terms);
	    DXCopyAttributes((Object)na, (Object)a);
	    break;
	}
	break;
	
      case CLASS_PATHARRAY:
	na = (Array)DXNewPathArray(1);   /* single point */
	DXCopyAttributes((Object)na, (Object)a);
	break;
	
	
    }
    
    return na;
    
  error:
    DXDelete((Object)na);
    DXDelete((Object)na2);
    return NULL;
}
 

/* if input is a CF and there will be more than 1 slice, you need to
 *  compute the positions for the entire output series now, so you can
 *  create the output series before you start creating any individual
 *  slices.  this is because you don't want to iterate through each incoming
 *  partition once per input slice - you want to compute all slices which
 *  cross this partition at the same time.  this is a bit awkward, since
 *  the partial slices each go into a different output composite field, so
 *  at the end, each outgoing CF will contain a complete single slice.
 *  all the positions are computed here, in order, and then the slices
 *  are added to the already existing CF's as they are computed.
 */
 
static Error 
numslices(CompositeField cf, struct argblk *ap)
{
    int i, j, k, l, m;
    Field f;
    Array arrc, arrp=NULL;
    int shape[MAXSHAPE]; 
    int stride[MAXDIM], counts[MAXDIM], gridoffsets[MAXDIM];
    float origins[MAXDIM], deltas[MAXDIM*MAXDIM], *pos;
    int startconn;
    int *sorted = NULL;
    struct gridlist {
	int seenoffset;
	int shape;
	int partno;
    } *gridlist = NULL;
    int beenallocated = 0;
    int seen = 0;
 
 
    /* clear these, so in case we return with an error, they are set */
    ap->compositecount = 0;
    ap->compositepositions = NULL;
 
 
    /* do some initial checks on the first partition member.
     */
    f = (Field)DXGetEnumeratedMember((Group)cf, 0, NULL);
    if (!f || DXGetObjectClass((Object)f) != CLASS_FIELD)
	DXErrorReturn(ERROR_DATA_INVALID, "compositefield member must be field");

    
    arrc = (Array)DXGetComponentValue(f, "connections");
    
    if (!DXQueryGridConnections(arrc, &ap->numdim, shape)) {
	DXSetError(ERROR_BAD_PARAMETER, "#10610", "input");
	return ERROR;
    }

    if (ap->slicedim >= ap->numdim) {
	DXSetError(ERROR_BAD_PARAMETER, "#11360", ap->slicedim, "input", 
		 ap->numdim-1);
	return ERROR;
    }
    
    /* before we compute the slice positions, we have to collect all the 
     *  partitions along the slice axis and compute the total number of slices.
     */
    
    for (i=0; (f=(Field)DXGetEnumeratedMember((Group)cf, i, NULL)); i++) {
	
	arrc = (Array)DXGetComponentValue(f, "connections");
	
	if (!DXQueryGridConnections(arrc, &ap->numdim, shape)) {
	    DXSetError(ERROR_BAD_PARAMETER, "#10610", "input");
	    return ERROR;
	}
	
	if (!DXGetMeshOffsets((MeshArray)arrc, gridoffsets))
	    return ERROR;
	
	/* only keep the partitions which are along the origin in the
	 *  other dimensions, so that later when the positions are
	 *  computed, if the data is deformed, the position will be
	 *  the value on the ap->slicedim axis.
	 */
	for (j=0; j<ap->numdim; j++) {
	    if (j == ap->slicedim)
		continue;
	    
	    if (gridoffsets[j] != 0)
		break;
	}
	if (j != ap->numdim)
	    continue;
	
	for (j=0; j<seen; j++)
	    if (gridoffsets[ap->slicedim] == gridlist[j].seenoffset)
		break;
	
	if (j != seen)
	    continue;
	
	if (seen >= beenallocated) {
	    beenallocated = (int)(16+seen * 1.5);
	    
	    gridlist = (struct gridlist *)DXReAllocate((Pointer)gridlist, 
				     beenallocated * sizeof(struct gridlist));
	    if (!gridlist)
		return ERROR;
	    
	}
	
	gridlist[seen].seenoffset = gridoffsets[ap->slicedim];
	gridlist[seen].shape = shape[ap->slicedim];
	gridlist[seen].partno = i;
	seen++;
	/* allow for duplicated faces */
	ap->compositecount += shape[ap->slicedim] - 1;
		
    }
    /* take into account the last face which isn't duplicated.
     *  this is now a positions count.  num of elements equals positions-1.
     */
    ap->compositecount++;
    
 
    /* if the user gave an explicit list, go through the components and
     *  find the starting positions which correspond to those slices.
     */
    if (ap->slicecount >= 1) {
 
	ap->compositepositions = (float *)
	                         DXAllocateZero(ap->slicecount * sizeof(float));
	if (!ap->compositepositions)
	    goto error;
 
	for (i=0; (f=(Field)DXGetEnumeratedMember((Group)cf, i, NULL)); i++) {
 
	    arrc = (Array)DXGetComponentValue(f, "connections");
 
	    if (!DXQueryGridConnections(arrc, &ap->numdim, shape)) {
		DXSetError(ERROR_BAD_PARAMETER, "#10610", "input");
		goto error;
	    }
    
	    if (!DXGetMeshOffsets((MeshArray)arrc, gridoffsets))
		goto error;
	    
	    /* only keep the partitions which are along the origin in the
             *  other dimensions, so that later when the positions are
             *  computed, if the data is deformed, the position will be the
	     *  value on the ap->slicedim axis.
             */
	    for (j=0; j<ap->numdim; j++) {
		if (j == ap->slicedim)
		    continue;
		
		if (gridoffsets[j] != 0)
		    break;
	    }
	    if (j != ap->numdim)
		continue;
	    
	    /* if any slices are inside this partition, set their actual
             *  positions.
	     */
	    arrp = NULL;
	    for (j=0; j<ap->slicecount; j++) {

		startconn = ap->sliceplace[j];

		if (startconn < 0 || startconn >= ap->compositecount) {
		    DXSetError(ERROR_DATA_INVALID, "#10040", 
			           "slice position", 0, ap->compositecount-1);
		    goto error;
		}
		
		/* if this slice is outside this partition, skip it.
		 */
		if ((startconn < gridoffsets[ap->slicedim])
		|| (startconn > gridoffsets[ap->slicedim]+shape[ap->slicedim]))
		    continue;


		if (!arrp)
		    arrp = (Array)DXGetComponentValue(f, "positions");
		
		/* if regular, compute position.  if irregular, index into data
		 *  array to find it.
		 */
		if (DXQueryGridPositions(arrp, &k, NULL, origins, deltas))
		    ap->compositepositions[j] = origins[ap->slicedim] + 
			startconn * deltas[k*ap->slicedim + ap->slicedim];
		else {
		    k = 1;
		    for (l=ap->slicedim; l>=0; l--) {
			stride[l] = k;
			k *= counts[l];
		    }
		    pos = (float *)DXGetArrayData(arrp);
		    if (!pos)
			goto error;
		    ap->compositepositions[j] = 
			              pos[startconn * stride[ap->slicedim]];
		}
	    }
	}
 
	goto done;
    }
 
    
    /* if you get here, the rangelist was not set - the default is to 
     *  make a slice at each position.
     */

    if (ap->compositecount <= 0) {
	DXSetError(ERROR_BAD_PARAMETER, "#10030", "number of slices");
	goto error;
    }

    ap->slicecount = ap->compositecount;
    
    if (!(ap->sliceplace = (int *)DXAllocate(ap->slicecount * sizeof(int))))
	return ERROR;
    
    for (i=0; i<ap->slicecount; i++)
	ap->sliceplace[i] = i;

    ap->compositepositions = (float *)
	                     DXAllocateZero(ap->slicecount * sizeof(float));
    if (!ap->compositepositions)
	goto error;
 

	
    /* sort the partitions so the slices/slices will be created in numeric
     *  order (if the user gave an explicit positions list, that will
     *  define the order of the output slices and this isn't necessary)
     */
    sorted = (int *)DXAllocate(seen * sizeof(int));
    if (!sorted)
	goto error;
    
    for (i=0; i < seen; i++)
	sorted[i] = i;
    
    for (i=0; i < seen-1; i++) {
	for (j=i+1; j < seen; j++)
	    if (gridlist[sorted[j]].seenoffset < 
		gridlist[sorted[i]].seenoffset) {
		k = sorted[i];
		sorted[i] = sorted[j];
		sorted[j] = k;
	    }
    }
    
    /* compute the actual positions.
     */
    k = -1;
    f = NULL;

    for (i=0; i < ap->slicecount; i++) {

	if (!f || ap->sliceplace[i] > 
	               gridoffsets[ap->slicedim] + gridlist[sorted[k]].shape) {

	    k++;    
	    f = (Field)DXGetEnumeratedMember((Group)cf, 
					   gridlist[sorted[k]].partno, NULL);
 
	    arrc = (Array)DXGetComponentValue(f, "connections");
	    if (!DXGetMeshOffsets((MeshArray)arrc, gridoffsets))
		goto error;
	    
	    arrp = (Array)DXGetComponentValue(f, "positions");
	}
	
	startconn = ap->sliceplace[i] - gridoffsets[ap->slicedim];

	/* if regular, compute position.  if irregular, index into data
	 *  array to find it.
	 */
	if (DXQueryGridPositions(arrp, &l, NULL, origins, deltas))
	    ap->compositepositions[i] = origins[ap->slicedim] + 
		startconn * deltas[l*ap->slicedim + ap->slicedim];
	else {
	    l = 1;
	    for (m=ap->slicedim; m>=0; m--) {
		stride[m] = l;
		l *= counts[m];
	    }
	    pos = (float *)DXGetArrayData(arrp);
	    if (!pos)
		goto error;
	    ap->compositepositions[i] = pos[startconn * stride[ap->slicedim]];
	}
	
    }


 done: 
    DXFree((Pointer)gridlist);
    DXFree((Pointer)sorted);
    return OK;
 
  error:
    DXFree((Pointer)gridlist);
    DXFree((Pointer)sorted);
    return ERROR;
}


/* if the result of slice is a series of composite fields, the data type
 *  information is set on the composite fields when the first member
 *  with data is added, but this information isn't propagated up to the
 *  series.   this routine looks for the first non-empty field in the
 *  series and sets the group type based on it.
 */
static Error
fixtype(Series s)
{
    int i, j;
    Object subo, subo2;
    Type t;
    Category c;
    int rank;
    int shape[MAXDIM];

    for (i=0; (subo=DXGetEnumeratedMember((Group)s, i, NULL)); i++) {
	if (DXGetObjectClass(subo) != CLASS_GROUP ||
	    DXGetGroupClass((Group)subo) != CLASS_COMPOSITEFIELD) {
	    DXErrorReturn(ERROR_INTERNAL, "missing composite field");
	}

	for (j=0; (subo2=DXGetEnumeratedMember((Group)subo, j, NULL)); j++) {
	    if (DXGetObjectClass(subo2) != CLASS_FIELD || 
		DXEmptyField((Field)subo2))
		continue;
	    
	    /* no data? */
	    if (!DXGetType(subo2, &t, &c, &rank, shape))
		continue;

	    if (!DXSetGroupTypeV((Group)s, t, c, rank, shape))
		return ERROR;

	    return OK;
	}
    }

    /* if there are no fields in the series, i guess it's ok to return
     *  without setting the type - because there isn't one.
     */
    return OK;
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
      default: break;
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
      default: break;
    }

    DXSetError(ERROR_NOT_IMPLEMENTED, "unrecognized data type");
    return ERROR;
}

/* check if value1 when cast to double equals 0.0.  1 is true, 0 is false
 */
#if 0
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
    }

    return 0;
}
#endif 

