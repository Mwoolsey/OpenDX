/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dxmods/slab.c,v 1.6 2006/01/03 17:02:25 davidt Exp $
 */

#include <dxconfig.h>

#if defined(HAVE_STRING_H)
#include <string.h>
#endif
 
#include <dx/dx.h>
#include <ctype.h>
#include <math.h>

#define argblk slabargblk

/* struct for passing parameters around
 */ 
struct argblk {
    Object toslab;              /* object to process */
    int slabdim;                /* dimension to make slabs along */
    int slabcount;              /* number of slabs to make */
    int slabthickness;          /* thickness (in connection numbers) */
    int *slabplace;             /* connection numbers at which to make slabs */
    int numdim;                 /* dimensionality of input object */
    Object compositeobj;        /* if !NULL, multiple slabs through CF */
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

 
 
static Object Object_Slab(Object o, struct argblk *a);
static Object Field_Slab(Field o, struct argblk *a);
static Array Array_SlabP(Array a, int dim, int pos, int thick, int num, 
			 int *counts);
static Array Array_SlabC(Array a, int dim, int pos, int thick, int num, 
			 int *counts);
static Error Slab_Wrapper(Pointer a);
static Error Slab_Wrapper2(Pointer a);
static Error Slab_Wrapper3(Pointer a);

static Error todouble(Type t, Pointer v1, int i1, double *v2);
static Error fromdouble(Type t, double v1, Pointer v2, int i2);

static Error numslabs(CompositeField f, struct argblk *a);
static Error fixtype(Series s);
extern void _dxfPermute(int dim, Pointer out, int *ostrides, int *counts, /* from libdx/permute.c */
			int size, Pointer in, int *istrides);

/* the source for these is in partreg.c */
extern Field _dxf_has_ref_lists(Field f, int *pos, int *con); /* from libdx/partreg.c */
extern Error _dxf_make_map_template(Array a, Array *map); /* from libdx/partreg.c */
extern Error _dxf_fix_map_template(Array map, Array newmap); /* from libdx/partreg.c */
extern Array _dxf_remap_by_map(Array a, Array map); /* from libdx/partreg.c */



#define MAXSHAPE   128        /* XXX - system constant */
#define MAXDIM     8          /* XXX - system constant */


#define IsEmpty(o)  (DXGetObjectClass(o)==CLASS_FIELD && DXEmptyField((Field)o))


/* 0 = input
 * 1 = dim number
 * 2 = thickness
 * 3 = position(s)
 *
 * 0 = output
 */
int
m_Slab(Object *in, Object *out)
{
    char *cp, cdim;
    struct argblk a;

 
    /* initialize anything which will be allocated eventually.
     *  it makes the error exit code simpler.  
     */
    a.compositeobj = NULL;		/* object */
    a.compositepositions = NULL;	/* memory */
    a.slabplace = NULL;			/* memory */
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
 
    /* dimension to slab along.  defaults to 0 */
    if (in[1]) {
        /* accept either an integer or the letters x,y or z */
	if (!DXExtractInteger(in[1], &a.slabdim)) {
            if (DXExtractString(in[1], &cp)) {
                cdim = isupper(cp[0]) ? tolower(cp[0]) : cp[0];
                if (cdim < 'x' || cdim > 'z') {
                    DXSetError(ERROR_BAD_PARAMETER, "bad dimension letter");
		    return ERROR;
		}
                a.slabdim = (int)(cdim - 'x');
            } else {
		DXSetError(ERROR_BAD_PARAMETER, "#10620", "dimension number");
		return ERROR;
	    }
        }
        if (a.slabdim < 0) {
            DXSetError(ERROR_BAD_PARAMETER, "#10020", "dimension number");
	    return ERROR;
	}
    } else
	a.slabdim = 0;
    
    
    /* where to make the slab(s).  either a single connection number
     *  or a list of connection numbers along the dimension where the
     *  slabs are to be taken.  the default is to pass a count of 0
     *  and a NULL pointer into the worker routine, which will make a
     *  series of slabs, one at each connection of the input object.
     */
    if (in[2]) {
	if (!DXQueryParameter(in[2], TYPE_INT, 1, &a.slabcount)) {
	    DXSetError(ERROR_BAD_PARAMETER, "#10050", "positions");
	    return ERROR;
	}
	
	if (a.slabcount <= 0) {
	    DXSetError(ERROR_BAD_PARAMETER, "#10050", "positions");
	    return ERROR;
	}
        
        if (!(a.slabplace = (int *)DXAllocate(sizeof(int) * a.slabcount)))
            return ERROR;
        
        if (!DXExtractParameter(in[2], TYPE_INT, 1, 
			      a.slabcount, (Pointer)a.slabplace)) {
            DXSetError(ERROR_BAD_PARAMETER, "#10050", "positions");
            goto error;
        }
        
    } else
	a.slabcount = 0;
    
    /* the thickness of each slab.  defaults to 0 if user gives a position,
     *  else defaults to 1.
     */
    if (in[3]) {
	if (!DXExtractInteger(in[3], &a.slabthickness)) {
	    DXSetError(ERROR_BAD_PARAMETER, "#10030", "thickness");
            goto error;
	}
 
	if (a.slabthickness < 0) {
	    DXSetError(ERROR_BAD_PARAMETER, "#10030", "thickness");
            goto error;
	}
 
    } else
	a.slabthickness = in[2] ? 0 : 1;


    /* allow empty fields to pass thru unchanged */
    if (IsEmpty(in[0])) {
	out[0] = in[0];
	return OK;
    }

    /* make sure there is a valid field in the input somewhere */
    if(!DXExists(in[0], "positions")) {
	DXSetError(ERROR_DATA_INVALID, "#10190", "input");
	return ERROR;
    }
    
    /* ok, try to slab */
    out[0] = Object_Slab(in[0], &a);

    /* if we want to make it so that if the slab location is outside
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
    DXFree((Pointer)a.slabplace);

    return OK;

  error:
    DXDelete((Object)a.compositeobj);
    DXFree((Pointer)a.compositepositions);
    DXFree((Pointer)a.slabplace);
    DXDelete((Object)a.outgroup);

    return ERROR;
}



/* recursive worker routine.
 *  input is an object and a filled in parameter block.
 */
static Object Object_Slab(Object o, struct argblk *a)
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
             *  number of slabs == 0 (slab at all locations)
             *  number of slabs == 1 (create a single slab)
             *  number of slabs > 1 (multiple slabs at user selected locations)
             */


	    if (!numslabs((CompositeField)o, a))
		return NULL;


            /* compute the number of connections in the composite field,
             *  and their actual location.  input is an object divided
             *  into fields based on the locations of the positions of the
             *  points.  output is a series object, where each field is a
             *  partitioned slab.  the series position is the original 
             *  location of the origin of the slab.  eventually this will
             *  run in parallel, so create a series group with initially empty
             *  composite fields to hold the slabs here.
             */

            if (a->slabcount == 1)
                goto normalgroup;


            a->compositeobj = (Object)DXNewSeries();
            if (!a->compositeobj)
                return NULL;

            /* for each new slab */
	    for (i=0; i < a->slabcount; i++) {
                newo = DXCopy(o, COPY_ATTRIBUTES);
                if (!newo)
                    return NULL;

                if (!DXSetSeriesMember((Series)a->compositeobj, i,
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
		    a->toslab = subo;
		    if (!DXAddTask(Slab_Wrapper, (Pointer)a, 
				 sizeof(struct argblk), 1.0)) {
			DXAbortTaskGroup();
			return NULL;
		    }
		    
		} else if (!Field_Slab((Field)subo, a))
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
	    if (!fixtype((Series)a->compositeobj))
		return NULL;

            o = a->compositeobj;
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
		    a->toslab = subo;
		    a->membername = name;
		    
		    if (!DXAddTask(Slab_Wrapper2, (Pointer)a, 
				 sizeof(struct argblk), 1.0)) {
			DXAbortTaskGroup();
			return NULL;
		    }

		} else {
		    if (!(newo = Object_Slab(subo, a))) {
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
		    a->toslab = subo;
		    a->seriesplace = position;
		    a->membernum = i;
		    
		    if (!DXAddTask(Slab_Wrapper3, (Pointer)a, 
				 sizeof(struct argblk), 1.0)) {
			DXAbortTaskGroup();
			return NULL;
		    }

		} else {
		    if (!(newo = Object_Slab(subo, a))) {
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
	
	o = Field_Slab((Field)o, a);
	break;
 
 
      case CLASS_XFORM:
	if (!DXGetXformInfo((Xform)o, &subo, &m))
	    return NULL;
 
	if (!(newo=Object_Slab(subo, a)))
	    return NULL;
 
	o = (Object)DXNewXform(newo, m);
	break;
 
 
      case CLASS_SCREEN:
	if (!DXGetScreenInfo((Screen)o, &subo, &fixed, &z))
	    return NULL;
 
	if (!(newo=Object_Slab(subo, a)))
	    return NULL;
 
	o = (Object)DXNewScreen(newo, fixed, z);
	break;
 
 
      case CLASS_CLIPPED:
	if (!DXGetClippedInfo((Clipped)o, &subo, &subo2))
	    return NULL;
 
	if (!(newo=Object_Slab(subo, a)))
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
static Error Slab_Wrapper(Pointer a)
{
    Object o;

    o = Field_Slab((Field)((struct argblk *)a)->toslab, (struct argblk *)a);
    return (o ? OK : ERROR);
}

/*
 */
static Error Slab_Wrapper2(Pointer a)
{
    struct argblk *ap = (struct argblk *)a;
    struct argblk a2;
    Object o;

    memcpy((char *)&a2, (char *)a, sizeof(struct argblk));

    o = Object_Slab(a2.toslab, &a2);

    if (!o) 
	return ((DXGetError() == ERROR_NONE) ? OK : ERROR);

    if (IsEmpty(o))
	return OK;
    
    DXSetMember((Group)ap->outgroup, ap->membername, o);
    
    return OK;
}

/*
 */
static Error Slab_Wrapper3(Pointer a)
{
    struct argblk *ap = (struct argblk *)a;
    struct argblk a2;
    Object o;

    memcpy((char *)&a2, (char *)a, sizeof(struct argblk));

    o = Object_Slab(a2.toslab, &a2);

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
static Object Field_Slab(Field o, struct argblk *a)
{
    Series s = NULL;
    Object newo, subo;
    Array ac = NULL;
    Array ap = NULL;
    Array ao;
    Field f = NULL;
    char *name, *attr;
    int i, j, k, n;
    int justone = 0;
    int start, slab;
    int stride[MAXDIM];
    int counts[MAXDIM], ccounts[MAXDIM];
    int gridoffset[MAXDIM], ngridoffset[MAXDIM];
    float origins[MAXDIM], deltas[MAXDIM*MAXDIM];
    float worldposition, scratch[MAXDIM];
    float *fp;
    ArrayHandle ah;
    int refpos, refcon;
    Array posmap = NULL;
    Array conmap = NULL;  /* temp for now - should use algorithm */

    
 
    /* have to do positions & connections first, so we know the
     *  original shape.  then other components can be slabbed.
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
    
    if (a->slabdim >= a->numdim) {
	DXSetError(ERROR_BAD_PARAMETER, "#11360", a->slabdim, "input", 
		                                             a->numdim-1);
	return NULL;
    }

    /* all fields have offsets.  if the input isn't partitioned, it still
     *  returns 0.  but if this field was extracted from a composite field,
     *  then the mesh offsets don't make sense - set them to zero.
     */
    if (a->compositepositions) {
	if (!DXGetMeshOffsets((MeshArray)ao, gridoffset))
	    return NULL;
    } else
	memset((char *)gridoffset, '\0', sizeof(int) * MAXDIM);

    /* if the input isn't partitioned, set the limits from this single field.
     */
    if (a->compositecount == 0)
	a->compositecount = ccounts[a->slabdim];

    /* check the thickness against the actual number of connections 
     */
    if ((a->slabthickness+1) < 0) {
	DXSetError(ERROR_BAD_PARAMETER, "#10040", 
		 "thickness", 0, a->compositecount - 1);
	return NULL;
    }
    
    /* if they gave us a position list, check it for out of range values 
     */
    for (i=0; i<a->slabcount; i++) {
	if (a->slabplace[i] < 0 || a->slabplace[i] >= a->compositecount) {
	    if (a->slabcount > 1) {
		DXSetError(ERROR_BAD_PARAMETER, "#10040",
			 "position item %d", 0, a->compositecount - 1, i+1);
		return NULL;
	    } else {
		DXSetError(ERROR_BAD_PARAMETER, "#10040", 
		 "position", 0, a->compositecount - 1);
		return NULL;
	    }
	}
	
	/* check start+thick to see if it extends out of volume 
	 */
	if ((a->slabplace[i]+a->slabthickness+1) > a->compositecount) {
	    slab = a->compositecount - a->slabplace[i] - 1;
	    if (a->slabcount > 1)
		DXWarning("slab %d extends out of data, thickness set to %d",
			i+1, slab);
	    else 
		DXWarning("slab extends out of data, thickness set to %d", slab);
	}
    }
    
    /* if they didn't give us a list, initialize a positions list,
     *  making a position every N==thickness connections.
     */
    if (a->slabcount == 0) {
	
	if (a->slabthickness > 0) 
	    a->slabcount = (a->compositecount-1 + a->slabthickness-1) / 
		                                              a->slabthickness;
	else
	    a->slabcount = a->compositecount;
	
	if (!(a->slabplace = (int *)DXAllocate(a->slabcount * sizeof(int))))
	    return NULL;
	
	if (a->slabthickness > 0)
	    for (i=0, slab=0; i<a->slabcount; i++, slab += a->slabthickness)
		a->slabplace[i] = slab;
	else
	    for (i=0; i<a->slabcount; i++)
		a->slabplace[i] = i;
    }
 
 
    /* if the input is partitioned, then an output field has already been
     *  initialized.  if there is more than one slab/slice, then the output
     *  will be a new series, with each slice in a separate field.  otherwise
     *  the input is a simple field and the output is a simple field - make
     *  a copy of the input a little further down and work on it.
     */
    if (!a->compositeobj) {
	if (a->slabcount != 1) {
	    if (!(s = DXNewSeries()))
		return NULL;
	} else
	    justone++;
    }
 
    /* loop here for each position */
    for (i=0; i<a->slabcount; i++) {
	
	/* check here to see if the slab/slice is inside this partition.
	 *  if slices, each partition excludes the starting face, except
         *  for the very first partition, to prevent creating 2 slices at
         *  the same location because of internal partition boundaries, but
         *  includes the ending face. 
	 *  if slabs, each partition includes all endpoints, but slabs
         *  cannot begin at the exact end of a partition.
	 */

	/* these are start and end position number, relative to this
	 *  partition.
	 */	
	start = a->slabplace[i] - gridoffset[a->slabdim];
	slab = start + a->slabthickness;

	/* completely outside?
         */
	if ((start < 0 && slab < 0)
	||  (start >= ccounts[a->slabdim] && slab >= ccounts[a->slabdim]))
	    continue;

	/* if slices, don't include partition minimum.  if slabs, don't
         *  start a slab at the last position in a partition, or end a
         *  slab at the first position in a partition.
	 */
	if (a->slabthickness == 0 &&
	    a->slabplace[i] != 0 && start <= 0 && slab <= 0)
		continue;
	if (a->slabthickness > 0 && start+1 >= ccounts[a->slabdim])
		continue;
	if (a->slabthickness > 0 && slab <= 0)
		continue;


	/* if there is only 1 slab, use a copy of the existing field.  
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

	/* reset slab so it is now the thickness instead of the ending
         *  position number.
	 */ 
	slab = a->slabthickness + 1;

	/* if the start of this slab is before this partition, adjust the
	 *  thickness first (start is negative, so this makes slab smaller),
	 *  and then set the start to the beginning of this partition.
	 */
	if (start < 0) {
	    slab += start;
	    start = 0;
	}

	/* if a slab, and thickness extends past this partition, decrease
         *  the thickness.
	 */
	if (a->slabthickness > 0 && start+slab > ccounts[a->slabdim])
	    slab = ccounts[a->slabdim]-start;
	
	counts[a->slabdim] = slab;
	if (ngridoffset[a->slabdim] > 0) {
	    if (ngridoffset[a->slabdim] <= a->slabplace[i])
		ngridoffset[a->slabdim] = 0;
	    else
		ngridoffset[a->slabdim] -= a->slabplace[i];
	}

 
	/* if thickness is 0, then object goes down 1 dimension in
         *  connection shape, but stays 3D in positions.  treat lines going to
 	 *  a point as a special case.  points have no connections component.
	 */
	if (slab == 1) {
	    if (a->numdim > 1) {
		for (j=a->slabdim+1; j<a->numdim; j++) {
		    counts[j-1] = ccounts[j];
		    ngridoffset[j-1] = gridoffset[j];
		}

		/* makegridconnections sets elemtype now */
		if (!(ac = DXMakeGridConnectionsV(a->numdim-1, counts))
		|| !DXSetStringAttribute((Object)ac, "ref", "positions")
		|| !DXSetMeshOffsets((MeshArray)ac, ngridoffset)) {
		    DXDelete((Object)ac);
		    goto error;
		}

	    }
 
	} else {
	    if(!(ac = DXMakeGridConnectionsV(a->numdim, counts))
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
 
	if (slab != 1 || a->numdim != 1) {
	    if (!DXSetComponentValue(f, "connections", (Object)ac))
		goto error;
	    
	    /* do this so it doesn't get deleted twice on error */
	    ac = NULL;
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
	 *  connection number.  then slab the positions array.
	 */
 
	/* if regular, compute position.  if irregular, index into data
	 *  array to find it.  if the dimensionality of the positions doesn't
	 *  match the dimensionality of the connections (e.g. quads in 
	 *  three space), then try to account for the missing dimension.
	 */
	if (DXQueryGridPositions(ap, &n, NULL, origins, deltas)) {
	    k = a->slabdim;
	    if (n > a->numdim)
		for (j=0; j<n; j++)
		    if (ccounts[j] <= 1 && k >= j) 
                        k++;
 
	    worldposition = origins[k];
	    for (j=0; j<n; j++)
	        worldposition += start * deltas[n*j + k];

	} else {
	    k = 1;
	    for (j=a->numdim-1; j>=0; j--) {
		stride[j] = k;
		k *= ccounts[j];
	    }
	    /* borrow the counts array temporarily, to look at shape[0] */
	    if (!DXGetArrayInfo(ap, NULL, NULL, NULL, NULL, counts))
		return NULL;
	    k = a->slabdim;
	    if (counts[0] > a->numdim)
		for (j=0; j<counts[0]; j++)
		    if (ccounts[j] <= 1 && k >= j)
			k++;
#if 1 /* new */
	    ah = DXCreateArrayHandle(ap);
	    if (!ah)
		goto error;
	    fp = (float *)DXGetArrayEntry(ah, start*stride[k], 
					  (Pointer)scratch);
	    worldposition = fp[k];
#if 0  /* very wrong, and in the dx 3.1.0 release.  sigh */
	    worldposition = *(float *)DXGetArrayEntry(ah, stride[k], 
						      (Pointer)scratch);
#endif
	    DXFreeArrayHandle(ah);
#else
	    /* bad.  this expands partially regular data in the input object */
	    worldposition = ((float *)DXGetArrayData(ap))[stride[k]];
#endif
	}
	
	
	ap = Array_SlabP(ap, a->slabdim, start, slab, a->numdim, ccounts);
	if (!ap) {
	    DXDelete((Object)ac);
	    goto error;
	}
 
	
	if (!DXSetComponentValue(f, "positions", (Object)ap)
	||  (justone && !DXChangedComponentValues(f, "positions"))) {
	    DXDelete((Object)ac);
	    DXDelete((Object)ap);
	    goto error;
	}


	/* check to see if we will need to make a map for any ref components.
	 *  since we know that the connections are regular (you can't get into
	 *  this code otherwise), we don't have to build a connections map,
	 *  but the positions may not be regular, so we have to build a map
	 *  for them.  (the following code does make a map for the connections
	 *  anyway, as a quick hack to get the function into the system).
	 */
	
	_dxf_has_ref_lists(o, &refpos, &refcon);
	
	/* make a refpos map (and a refcon map for now until i get a 
	 * map routine which keeps the map compact) if needed.
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
	    
	    tmap = Array_SlabP(posmap, a->slabdim, 
			       start, slab, a->numdim, ccounts);
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
	    
	    ac = (Array)DXGetComponentValue(o, "connections");
	    if (!ac)
		goto error;
	    
	    if (!_dxf_make_map_template(ac, &conmap))
		goto error;
	    
	    tmap = Array_SlabC(conmap, a->slabdim, 
			       start, slab, a->numdim, ccounts);
	    if (!tmap)
		goto error;
	    
	    if (!_dxf_fix_map_template(conmap, tmap)) {
		DXDelete((Object) tmap);
		goto error;
	    }
	    
	    DXDelete((Object) tmap);
	    /* conmap now captures where the connections have moved to */
	}
	
 
	
	/* for each of the remaining components:
	 *   skip connections & positions - we've already slabbed them
	 *   either skip or delete derived components
	 *   if it doesn't dep on positions or connections;
         *    for a single slab (therefore a single output field), we've
	 *    already made a copy of all the components, so just continue
	 *    for a series, each slab is a separate field, so the component
	 *    must be copied to the new field.
	 *   if it does dep on positions or connections:
	 *    slab the new component and add it to the field.  for single
	 *    slabs, this replaces the old component.  for series of slabs,
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
	    
	    /* try dep first */
	    if (DXGetStringAttribute(subo, "dep", &attr)) {
		
		if (!strcmp("positions", attr)) {	
		    
		    newo = (Object)Array_SlabP((Array)subo, a->slabdim, 
					       start, slab, 
					       a->numdim, ccounts);
		    
		} else if (!strcmp("connections", attr)) {
		    
		    if (slab <= 1) {
			DXSetError(ERROR_NOT_IMPLEMENTED, 
	          "cannot make 0 thickness slab in connection dependent data");
			goto error;
		    }
		    newo = (Object)Array_SlabC((Array)subo, a->slabdim, 
					       start, slab, 
					       a->numdim, ccounts);
		    
		} else {
		    /* dep on something we don't understand - copy thru */
		    if (!justone)
			DXSetComponentValue(f, name, subo);
		    continue;
		}
	    } else if (DXGetStringAttribute(subo, "ref", &attr)) {
		if (!strcmp(attr, "positions")) {
		    newo = (Object) _dxf_remap_by_map((Array)subo, posmap);

		} else if (!strcmp(attr, "connections")) {
		    newo = (Object) _dxf_remap_by_map((Array)subo, conmap);
		    
		} else {
		    /* ref on something we don't understand - copy thru */
		    if (!justone)
			DXSetComponentValue(f, name, subo);
		    continue;
		}
		
	    } else {
		/* not dep or ref anything - pass a copy thru */
		if (!justone)
		    DXSetComponentValue(f, name, subo);
		continue;
	    }
		
	    if (!newo)
		goto error;

	    if (!DXSetComponentValue(f, name, newo)
		|| !DXChangedComponentValues(f, name)
		|| !DXCopyAttributes(newo, subo)) {
		DXDelete(newo);
		goto error;
	    }

	}
	
	
	if (!DXEndField(f))
	    goto error;

	/* if the output is a series of composite fields, get the matching
         *  group member and add this new field to it.
	 * else if the input is a simple field and there is more than one
         *  slab, add this field as part of a series.
         * else the input is a simple field and the output is only a single
	 *  slice - there is nothing extra to do here.
	 */
	if (a->compositeobj) {
	    subo = DXGetSeriesMember((Series)a->compositeobj, i, 
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


    DXDelete((Object)posmap);
    DXDelete((Object)conmap);
    
    if (a->compositeobj)
	return (Object)a->compositeobj;
    else if (!justone)
	return (Object)s;
    else
	return (Object)f;
 
error:
    DXDelete((Object)posmap);
    DXDelete((Object)conmap);
    DXDelete((Object)s);
    DXDelete((Object)f);
    return NULL;
}
 
/* slab an array dep on positions.  args are:
 *  array to slab
 *  connection dimension number along which to slab
 *  connection offset along that dimension at which to start slabbing
 *  number of connections to include in the slab
 *  number of original dimensions in the connections
 *  number of counts along the original dimensions in the connections
 */
static Array 
Array_SlabP(Array a, int dim, int pos, int thick, int num, int *ccounts)
{
    Array na = NULL, terms[MAXDIM];
    int nitems, itemsize;
    Type t;
    Category c;
    double o_x[MAXDIM], d_x[MAXDIM*MAXDIM];
    Pointer origins = o_x, deltas = d_x;
    int rank, shape[MAXSHAPE], counts[MAXDIM], ncounts[MAXDIM];
    int ostride[MAXDIM], nstride[MAXDIM];
    double tvalue1, tvalue2;
    Pointer op, np;
    int i, j, k;
    
 
    /* check here for fully regular array and shortcut the following code */
 
    if (DXQueryGridPositions(a, &i, counts, 
			     (float *)origins, (float *)deltas)) {
	
	int temp = 0;
	int dimtemp = dim;

	/* dim of this array less than connections? like quads in 1 space? 
	 * expand it to get the right correspondance.  same if the slab
	 * dim is already greater than this array dim.
	 */
	if (i < num || dim > i)
	    goto expanded;

        /* dim of this array larger than connections (like quads in 3 space).
	 * try to see if the number of flat dimensions plus the number of
	 * connection dimensions equals this number of dimensions.  if so,
	 * skip over the flat dimensions.  make sure the remaining connections
	 * match the connection dimensions in length - otherwise expand them.
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
	    
	    /* make sure the dimensions match before you try to slab
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
         */
	counts[dim] = thick;
	for (j=0; j<i; j++)
	    ((float *)origins)[j] += ((float *)deltas)[dim*i + j] * pos;
	
	na = DXMakeGridPositionsV(i, counts, (float *)origins, (float *)deltas);
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
	for (i=num-1; i>=0; i--) {
	    ostride[i] = j;
	    j *= ccounts[i];
	}
 
	for (i=0; i<num; i++)
	    ncounts[i] = ccounts[i];
 
	/* set the starting point at the first corner of the new slab */
	op = (Pointer)((char *)op + pos*ostride[dim] * itemsize);
 
	ncounts[dim] = thick;
 
	/* new strides */
	j = 1;
	for (i=num-1; i>=0; i--) {
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
	_dxfPermute(num, np, nstride, ncounts, itemsize, op, ostride);
	
	break;
	
      case CLASS_REGULARARRAY:
	/* the output slab can't still be regular unless
         *  you are slabbing along the slowest varying dimension...
	 */
	if (dim != 0)
	    goto expanded;
	
	/* need type and shape[0] from this */
	if (!DXGetArrayInfo(a, NULL, &t, NULL, &rank, shape) ||
	    !DXGetRegularArrayInfo((RegularArray)a, &nitems, origins, deltas))
	    return NULL;

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
	nitems = nitems / ccounts[dim] * thick;

	na = (Array)DXNewRegularArray(t, shape[0], nitems, origins, deltas);
	break;
 
      case CLASS_CONSTANTARRAY:
	if (!DXGetArrayInfo(a, &nitems, &t, &c, &rank, shape))
	    return NULL;

        nitems = nitems / ccounts[dim] * thick;
	na = (Array)DXNewConstantArrayV(nitems, DXGetConstantArrayData(a),
                                       t, c, rank, shape);
	break;
 
      case CLASS_PRODUCTARRAY:
	DXGetProductArrayInfo((ProductArray)a, &j, terms);

	/* make sure this product array matches the positions shape
	 *  exactly before trying to keep it as a regular array.
	 *  if any of the terms aren't the same, go to the expanded case
	 */

	if (j != num || dim > j) 
	    goto expanded;
	for (i=0; i<j; i++) {
	    if (!DXGetArrayInfo(terms[i], &nitems, NULL, NULL, NULL, NULL))
		return NULL;
	    
	    if (nitems != ccounts[i])
		goto expanded;
	}
	    
	terms[dim] = Array_SlabP(terms[dim], 0, pos, thick, 1, &ccounts[dim]);
	na = (Array)DXNewProductArrayV(j, terms);
	break;
	
	
      case CLASS_MESHARRAY:
	DXGetMeshArrayInfo((MeshArray)a, &j, terms);

	if (j != num || dim > j) 
	    goto expanded;
	for (i=0; i<j; i++) {
	    if (!DXGetArrayInfo(terms[i], &nitems, NULL, NULL, NULL, NULL))
		return NULL;
	    
	    if (nitems != ccounts[i])
		goto expanded;
	}
	    
	terms[dim] = Array_SlabP(terms[dim], 0, pos, thick, 1, &ccounts[dim]);
	na = (Array)DXNewMeshArrayV(j, terms);
	break;
	
      case CLASS_PATHARRAY:
	na = (Array)DXNewPathArray(thick);
	break;
	    
	
    }
    
    return na;
    
}
 
 
/* slab an array dep on connections.  args are:
 *  array to slab
 *  connection dimension number along which to slab
 *  connection offset along that dimension at which to start slabbing
 *  number of connections to include in the slab
 *  number of original dimensions in the connections
 *  number of counts along the original dimensions in the connections
 */
static Array 
Array_SlabC(Array a, int dim, int pos, int thick, int num, int *ccounts)
{
    Array na = NULL, terms[MAXDIM];
    int nitems, itemsize;
    Type t;
    Category c;
    double o_x[MAXDIM], d_x[MAXDIM*MAXDIM];
    Pointer origins = o_x, deltas = d_x;
    int rank, shape[MAXSHAPE], ncounts[MAXDIM];
    int ostride[MAXDIM], nstride[MAXDIM];
    double tvalue1, tvalue2;
    Pointer op, np;
    int i, j, k;
    
 
    /* check here for fully regular array and shortcut the following code */
 
    if (DXQueryGridConnections(a, &i, ncounts)) {
	
	if (num != i) {
            DXSetError(ERROR_BAD_PARAMETER, "#11390", "slab dimension", i);
            return NULL;
        }
 
	/* change the slabbed count.
	 */
	ncounts[dim] = thick - 1;
	
	na = DXMakeGridConnectionsV(num, ncounts);
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
	for (i=num-1; i>=0; i--) {
	    ostride[i] = j;
	    j *= ccounts[i] - 1;
	}
 
	for (i=0; i<num; i++)
	    ncounts[i] = ccounts[i] - 1;
 
	/* set the starting point at the first corner of the new slab */
	op = (Pointer)((char *)op + pos*ostride[dim] * itemsize);
 
	ncounts[dim] = thick - 1;
 
	/* new strides */
	j = 1;
	for (i=num-1; i>=0; i--) {
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
	_dxfPermute(num, np, nstride, ncounts, itemsize, op, ostride);
	
	break;
	
      case CLASS_REGULARARRAY:
	/* the output slab can't still be regular unless
         *  you are slabbing along the slowest varying dimension...
	 */
	if (dim != 0)
	    goto expanded;
	
	/* need type and shape[0] from this */
	if (!DXGetArrayInfo(a, NULL, &t, NULL, &rank, shape) ||
	    !DXGetRegularArrayInfo((RegularArray)a, &nitems, origins, deltas))
	    return NULL;

	/* compute the new origin and correct the counts 
	 */
	k = 1;
	for (i=num-1; i>0; --i)
	    k *= (ccounts[i]-1);
	k *= pos;

	for (j=0; j<shape[0]; j++) {
	    todouble(t, deltas, j, &tvalue1);
	    todouble(t, origins, j, &tvalue2);
	    
	    tvalue2 += tvalue1 * k;
	    
	    fromdouble(t, tvalue2, origins, j);
	}
	nitems = nitems / (ccounts[dim]-1) * (thick-1);
	
	na = (Array)DXNewRegularArray(t, shape[0], nitems, origins, deltas);
	break;
	
      case CLASS_CONSTANTARRAY:
	if (!DXGetArrayInfo(a, &nitems, &t, &c, &rank, shape))
	    return NULL;

        nitems = nitems / (ccounts[dim]-1) * (thick-1);
	na = (Array)DXNewConstantArrayV(nitems, DXGetConstantArrayData(a),
                                       t, c, rank, shape);
	break;
 
      case CLASS_PRODUCTARRAY:
	DXGetProductArrayInfo((ProductArray)a, &j, terms);

	/* see the comment about this code in the Array_SlabP routine */
	if (j != num || dim > j)
	    goto expanded;
	for (i=0; i<j; i++) {
	    if (!DXGetArrayInfo(terms[i], &nitems, NULL, NULL, NULL, NULL))
		return NULL;
	    
	    if (nitems != ccounts[i]-1)
		goto expanded;
	}
	    
	terms[dim] = Array_SlabC(terms[dim], 0, pos, thick, 1, &ccounts[dim]);
	na = (Array)DXNewProductArrayV(j, terms);
	break;
	
	
      case CLASS_MESHARRAY:
	DXGetMeshArrayInfo((MeshArray)a, &j, terms);

	/* see the comment about this code in the Array_SlabP routine */
	if (j != num || dim > j)
	    goto expanded;
	for (i=0; i<j; i++) {
	    if (!DXGetArrayInfo(terms[i], &nitems, NULL, NULL, NULL, NULL))
		return NULL;
	    
	    if (nitems != ccounts[i]-1)
		goto expanded;
	}

	terms[dim] = Array_SlabC(terms[dim], 0, pos, thick, 1, &ccounts[dim]);
	na = (Array)DXNewMeshArrayV(j, terms);
	break;
	
      case CLASS_PATHARRAY:
	na = (Array)DXNewPathArray(thick-1);
	break;
	    
    }
    
    return na;
    
}
 
 
/* if input is a CF and there will be more than 1 slab/slice, you need to
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
numslabs(CompositeField cf, struct argblk *ap)
{
    int i, j, k, l, m;
    Field f;
    Array arrc, arrp=NULL;
    int ccounts[MAXSHAPE]; 
    int stride[MAXDIM], gridoffsets[MAXDIM];
    float origins[MAXDIM], deltas[MAXDIM*MAXDIM], *pos;
    int startconn, endconn;
    int *sorted = NULL;
    struct gridlist {
	int seenoffset;
	int ccounts;
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
    if (!f || DXGetObjectClass((Object)f) != CLASS_FIELD) {
	DXSetError(ERROR_DATA_INVALID, "compositefield member must be field");
	goto error;
    }

    
    arrc = (Array)DXGetComponentValue(f, "connections");
    
    if (!DXQueryGridConnections(arrc, &ap->numdim, ccounts)) {
	DXSetError(ERROR_BAD_PARAMETER, "#10610", "input");
	goto error;
    }


    /* before we compute the slice positions, we have to collect all the 
     *  partitions along the slice axis and compute the total number of slices.
     */
    
    for (i=0; (f=(Field)DXGetEnumeratedMember((Group)cf, i, NULL)); i++) {
	
	arrc = (Array)DXGetComponentValue(f, "connections");
	
	if (!DXQueryGridConnections(arrc, &ap->numdim, ccounts)) {
	    DXSetError(ERROR_BAD_PARAMETER, "#10610", "input");
	    goto error;
	}
	
	if (!DXGetMeshOffsets((MeshArray)arrc, gridoffsets))
	    goto error;
	
	/* only keep the partitions which are along the origin in the
	 *  other dimensions, so that later when the positions are
	 *  computed, if the data is deformed, the position will be
	 *  the value on the ap->slabdim axis.
	 */
	for (j=0; j<ap->numdim; j++) {
	    if (j == ap->slabdim)
		continue;
	    
	    if (gridoffsets[j] != 0)
		break;
	}
	if (j != ap->numdim)
	    continue;
	
	for (j=0; j<seen; j++)
	    if (gridoffsets[ap->slabdim] == gridlist[j].seenoffset)
		break;
	
	if (j != seen)
	    continue;
	
	if (seen >= beenallocated) {
	    beenallocated = (int)(16+seen * 1.5);
	    
	    gridlist = (struct gridlist *)DXReAllocate((Pointer)gridlist, 
				     beenallocated * sizeof(struct gridlist));
	    if (!gridlist)
		goto error;
	    
	}
	
	gridlist[seen].seenoffset = gridoffsets[ap->slabdim];
	gridlist[seen].ccounts = ccounts[ap->slabdim];
	gridlist[seen].partno = i;
	seen++;
	/* allow for duplicated faces */
	ap->compositecount += ccounts[ap->slabdim] - 1;
		
    }
    /* take into account the last face which isn't duplicated.
     *  this is now a positions count.  num of elements equals positions-1.
     */
    ap->compositecount++;
    
 
    /* if the user gave an explicit list, go through the components and
     *  find the starting positions which correspond to those slices.
     */
    if (ap->slabcount >= 1) {
 
	ap->compositepositions = (float *)
	                         DXAllocateZero(ap->slabcount * sizeof(float));
	if (!ap->compositepositions)
	    goto error;
 
	for (i=0; (f=(Field)DXGetEnumeratedMember((Group)cf, i, NULL)); i++) {
 
	    arrc = (Array)DXGetComponentValue(f, "connections");
 
	    if (!DXQueryGridConnections(arrc, &ap->numdim, ccounts)) {
		DXSetError(ERROR_BAD_PARAMETER, "#10610", "input");
		goto error;
	    }
    
	    if (!DXGetMeshOffsets((MeshArray)arrc, gridoffsets))
		goto error;
	    
	    /* only keep the partitions which are along the origin in the
             *  other dimensions, so that later when the positions are
             *  computed, if the data is deformed, the position will be the
	     *  value on the ap->slabdim axis.
             */
	    for (j=0; j<ap->numdim; j++) {
		if (j == ap->slabdim)
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
	    for (j=0; j<ap->slabcount; j++) {

		startconn = ap->slabplace[j];
		endconn = ap->slabplace[j] + ap->slabthickness;

		if (startconn < 0 || startconn >= ap->compositecount) {
		    DXSetError(ERROR_DATA_INVALID, "#10040", 
			       "slab position", 0, ap->compositecount-1);
		    goto error;
		}
		
		/* if this slab is entirely outside this partition, skip it.
		 */
		if (((startconn < gridoffsets[ap->slabdim])
		&&   (endconn < gridoffsets[ap->slabdim]))
		||  ((startconn > gridoffsets[ap->slabdim]+ccounts[ap->slabdim])
		&&   (endconn > gridoffsets[ap->slabdim]+ccounts[ap->slabdim])))
		    continue;

		if (startconn < gridoffsets[ap->slabdim])
		    startconn = 0;

		if (!arrp)
		    arrp = (Array)DXGetComponentValue(f, "positions");
		
		/* if regular, compute position.  if irregular, index into data
		 *  array to find it.
		 */
		if (DXQueryGridPositions(arrp, &k, NULL, origins, deltas))
		    ap->compositepositions[j] = origins[ap->slabdim] + 
		              startconn * deltas[k*ap->slabdim + ap->slabdim];
		else {
		    k = 1;
		    for (l=ap->slabdim; l>=0; l--) {
			stride[l] = k;
			k *= ccounts[l];
		    }
		    pos = (float *)DXGetArrayData(arrp);
		    if (!pos)
			goto error;
		    ap->compositepositions[j] = 
			              pos[startconn * stride[ap->slabdim]];
		}
	    }
	}
 
	goto done;
    }
 
    
    /* if you get here, the rangelist was not set - the default is to 
     *  make a slice at each position.
     */

    if (ap->compositecount <= 0) {
	DXSetError(ERROR_BAD_PARAMETER, "#10030", "number of slabs");
	goto error;
    }

    if (ap->slabthickness > 0) 
	ap->slabcount = (ap->compositecount-1 + ap->slabthickness-1) / 
	                                                    ap->slabthickness;
    else
	ap->slabcount = ap->compositecount;
    
    if (!(ap->slabplace = (int *)DXAllocate(ap->slabcount * sizeof(int))))
	goto error;
    
    if (ap->slabthickness > 0)
	for (i=0, m=0; i<ap->slabcount; i++, m += ap->slabthickness)
	    ap->slabplace[i] = m;
    else
	for (i=0; i<ap->slabcount; i++)
	    ap->slabplace[i] = i;

    ap->compositepositions = (float *)
	                     DXAllocateZero(ap->slabcount * sizeof(float));
    if (!ap->compositepositions)
	goto error;
 

	
    /* sort the partitions so the slabs/slices will be created in numeric
     *  order (if the user gave an explicit positions list, that will
     *  define the order of the output slabs and this isn't necessary)
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

    for (i=0; i < ap->slabcount; i++) {

	if (!f || ap->slabplace[i] > 
	               gridoffsets[ap->slabdim] + gridlist[sorted[k]].ccounts) {

	    k++;    
	    f = (Field)DXGetEnumeratedMember((Group)cf, 
					   gridlist[sorted[k]].partno, NULL);
 
	    arrc = (Array)DXGetComponentValue(f, "connections");
	    if (!DXGetMeshOffsets((MeshArray)arrc, gridoffsets))
		goto error;
	    
	    arrp = (Array)DXGetComponentValue(f, "positions");
	}
	
	startconn = ap->slabplace[i] - gridoffsets[ap->slabdim];

	/* if regular, compute position.  if irregular, index into data
	 *  array to find it.
	 */
	if (DXQueryGridPositions(arrp, &l, NULL, origins, deltas))
	    ap->compositepositions[i] = origins[ap->slabdim] + 
		              startconn * deltas[l*ap->slabdim + ap->slabdim];
	else {
	    l = 1;
	    for (m=ap->slabdim; m>=0; m--) {
		stride[m] = l;
		l *= ccounts[m];
	    }
	    pos = (float *)DXGetArrayData(arrp);
	    if (!pos)
		goto error;
	    ap->compositepositions[i] = pos[startconn * stride[ap->slabdim]];
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


/* if the result of slab is a series of composite fields, the data type
 *  information is set on the composite fields when the first member
 *  with data is added, but this information isn't propagated up to the
 *  group.   this routine looks for the first non-empty field in the
 *  group and sets the group type based on it.
 */
static Error
fixtype(Series s)
{
    int i;
    Object subo, subo2;
    Type t;
    Category c;
    int rank;
    int shape[MAXDIM];

    for (i=0; (subo=DXGetEnumeratedMember((Group)s, i, NULL)); i++) {
	if (DXGetObjectClass(subo) != CLASS_GROUP ||
	    DXGetGroupClass((Group)subo) != CLASS_COMPOSITEFIELD)
	    return OK;

	for (i=0; (subo2=DXGetEnumeratedMember((Group)subo, i, NULL)); i++) {
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

    /* if there are no fields in the series, return without setting anything.
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
