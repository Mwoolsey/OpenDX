/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*********************************************************************/
/* (c) COPYRIGHT International Business Machines Corp.  1995         */ 
/*                      ALL RIGHTS RESERVED                          */
/*********************************************************************/

#include <dxconfig.h>

#if defined(HAVE_STRING_H)
#include <string.h>
#endif

#include <dx/dx.h>


/* "how" as a number doesn't seem very easy to use, but
 * i don't have a great alternate suggestion.
 */
typedef struct taskarg {
    Object o;                /* data to work on */
    int how;                 /* 0 to 7, arbitrary reorientations */
    int partitioned;         /* if set, f is part of a partitioned field */
    int maxextent[2];        /* total mesh size for partitioned images */
    int count[2];            /* x and y counts */
    int grid;                /* what original grid was like */
    int altergrid;           /* how the grid needs to be altered */
    int alterdata;           /* ditto for data; see INV_xxx flags below */
    int dotodata;            /* the actual transform to the data - depends on
                                interaction between altergrid & alterdata */
} Rtaskarg;

static Error DoImage (Pointer);
static Error ReorientImage (Rtaskarg *t);

static Error setextents(Object, int *);
static Error dogrid (Rtaskarg *tp);
static Error dodata (Rtaskarg *tp);

#if 0   /* set this if you want to run serial instead of parallel */
#define SERIAL 1
#endif

Error m_Reorient(Object *in, Object *out)
{
    
    Class c;
    Rtaskarg *t;


    if (!in[0]) {
	DXSetError(ERROR_BAD_PARAMETER, "#10000", "image");
	return ERROR;
    }

    c = DXGetObjectClass(in[0]);
    if (c != CLASS_GROUP && c != CLASS_FIELD) {
	DXSetError(ERROR_BAD_PARAMETER, "#10190", "image");
	return ERROR;
    }

    /* initial values all 0 */
    t = (Rtaskarg *)DXAllocateZero(sizeof(Rtaskarg));
    if (!t)
	return ERROR;

    /* default is to ensure that the image is oriented in the manner
     * the Display() module prefers.
     */
    if (in[1] && (!DXExtractInteger(in[1], &t->how) || t->how < 0 || t->how > 7)) {
	DXSetError(ERROR_BAD_PARAMETER, "#10040", "how", 0, 7);
	goto error;
    }


    t->o = (Object)DXCopy(in[0], COPY_STRUCTURE);
    if (!t->o)
	goto error;

#if !SERIAL    
    if (!(DXCreateTaskGroup()))
	goto error;
#endif

    if (!ReorientImage(t))
	goto error;

#if !SERIAL
    if (!(DXExecuteTaskGroup()))
	goto error;
#endif    
    
    /* successful return */
    out[0] = t->o;
    return OK;
    
  error:
    DXDelete(t->o);
    DXFree((Pointer)t);
    return ERROR;
}



static Error 
ReorientImage(Rtaskarg *t)
{
    Object subo;
    Class class;
    int i;
    Rtaskarg nt;
    
    memcpy((char *)&nt, (char *)t, sizeof(Rtaskarg));

    class = DXGetObjectClass(t->o);
    if (class == CLASS_GROUP)
	class = DXGetGroupClass((Group)t->o);
    
    switch (class) {
      case CLASS_COMPOSITEFIELD:
	/* put this in a subroutine */

	if (setextents(t->o, (int *)&nt.maxextent) == ERROR)
	    goto error;
	
	nt.partitioned = 1;
	for (i=0; (subo = DXGetEnumeratedMember((Group)t->o, i, NULL)); i++) {
	    nt.o = subo;
	    if (!ReorientImage(&nt))
		goto error;
	}
	break;
	
      case CLASS_GROUP:
	for (i=0; (subo = DXGetEnumeratedMember((Group)t->o, i, NULL)); i++) {
	    nt.o = subo;
	    if (!ReorientImage(&nt))
		goto error;
	}
	break;
	
      case CLASS_FIELD:
	nt.o = t->o;
#if !SERIAL
	if (!DXAddTask(DoImage, (Pointer)&nt, sizeof(nt), 1.0))
	    goto error;
#else
	if (!DoImage(&nt))
	    goto error;
	
#endif
	break;

      default:
	/* set an error here?  bad object type? */
	break;
	
    }
    
    return OK;

  error:
    return ERROR;
    
}

static Error setextents(Object o, int *e)
{
    Object subo;
    Array a;
    int i, dim;
    int mo[2];
    int counts[2];

    /* gag.  i'm going to have to look for partitioned images here
     * and treat them special.  oh boy.
     */
    for (i=0; (subo = DXGetEnumeratedMember((Group)o, i, NULL)); i++) {

	if (DXGetObjectClass(subo) != CLASS_FIELD) {
	    DXSetError(ERROR_DATA_INVALID, 
		       "member of partitioned field is not a field");
	    return ERROR;
	}

	a = (Array)DXGetComponentValue((Field)subo, "connections");
	if (!a) {
	    DXSetError(ERROR_DATA_INVALID, 
		       "field has no `connections' component");
	    return ERROR;
	}
	if (DXGetObjectClass((Object)a) != CLASS_ARRAY) {
	    DXSetError(ERROR_DATA_INVALID, 
		       "connections component is not an Array object");
	    return ERROR;
	}
     
	if ((DXQueryGridConnections(a, &dim, NULL) == ERROR) ||
	    (dim != 2)) {
	    DXSetError(ERROR_DATA_INVALID, 
		       "field must have regular 2d connections");
	    return ERROR;
	}
	DXQueryGridConnections(a, &dim, counts);

	if (!DXGetMeshOffsets((MeshArray)a, mo))
	    return ERROR;
	
	if (mo[0] + counts[0] > e[0])
	    e[0] = mo[0] + counts[0];
	if (mo[1] + counts[1] > e[1])
	    e[1] = mo[1] + counts[1];
    }

    return OK;
}


/* two issues:  what to do with the data, and what to do
 * with the origins & deltas which define the grid.
 */

/* settings for the "alter" flags below */
#define INV_NOTHING    0x00   /* already correct order */
#define INV_TRANSPOSE  0x01   /* transpose x and y data */                
#define INV_ROWS       0x02   /* invert the data inside each row */  
#define INV_COLS       0x04   /* same w/ cols */                     
#define INV_EXCHANGE   0x08   /* exchange the x and y axis */

/* settings for the grid flag */
#define Y_FASTEST      0x01   /* y variables are contig in memory */
#define NEGATIVE_X     0x02   /* deltas from positions array */
#define NEGATIVE_Y     0x04   /* ditto */

static Error DoImage (Pointer ptr)
{
    Rtaskarg *tp = (Rtaskarg *)ptr;

    if (dogrid(tp) == ERROR)
	return ERROR;

    return OK;
}


static Error dogrid (Rtaskarg *tp)
{
    Field field;
    Array positions, connections;
    Array new_positions=NULL;
    Array new_connections=NULL;

    int yfastest = 0;
    int negative_x = 0;
    int negative_y = 0;
    
    int dim, counts[2], itemp;
    int meshextents[2];
    float origin[2], deltas[4], ftemp;
    int otherdim;


    field = (Field)tp->o;

    /* get partition info here because i'll have to deal
     * with fixing up the mesh offsets as well.
     */
    connections = (Array) DXGetComponentValue(field, "connections");
    if (!connections) {
	DXSetError(ERROR_MISSING_DATA, "field has no `connections' component");
	return ERROR;
    }

    if ((!DXQueryGridConnections(connections, &dim, NULL)) 
	|| (dim != 2)) {
	DXSetError(ERROR_DATA_INVALID, "connections must be a 2D regular grid");
	goto error;
    }
    
    if (!DXGetMeshOffsets((MeshArray)connections, (int *)meshextents))
	return ERROR;
    
    /* now we have the start of this grid in meshextents and the extents
     * of all the partitioned fields combined in tp->maxextents
     */

#if 0
    DXMessage("old extents: %d %d, %d %d", meshextents[0], meshextents[1], 
	      tp->maxextent[0], tp->maxextent[1]);
#endif

    
    /* verify the positions are regular and 2d 
     */
    positions = (Array) DXGetComponentValue(field, "positions");
    if (!positions) {
	DXSetError(ERROR_MISSING_DATA, "field has no `positions' component");
	return ERROR;
    }
    
    if ((!DXQueryGridPositions(positions, &dim, counts, origin, deltas)) 
	|| dim != 2) {
	DXSetError(ERROR_DATA_INVALID, "positions must be a 2D regular grid");
	goto error;
    }

    /* decide what to do with the grid */

    /* figure out what the orientation of the input image was; we want
     * the output deltas to be positive with x varies fastest.  figure
     * out what has to happen to the data to make that happen, and
     * then figure out what additional thing has to happen to the
     * data to change the orientation as the user requested.
     */
    if (deltas[3] != 0) {
	yfastest++;
	tp->grid |= Y_FASTEST;
	tp->altergrid |= INV_EXCHANGE | INV_TRANSPOSE;
    } else if (deltas[2] != 0) {
	yfastest = 0;
    } else {
	DXSetError(ERROR_DATA_INVALID, "image must have an orthogonal grid");
	goto error;
    }

    if (deltas[0] < 0 || deltas[2] < 0) {
	negative_x++;
	tp->grid |= NEGATIVE_X;
	tp->altergrid |= INV_COLS;
    }
    if (deltas[1] < 0 || deltas[3] < 0) {
	negative_y++;
	tp->grid |= NEGATIVE_Y;
	tp->altergrid |= INV_ROWS;
    }

    tp->count[0] = counts[0];
    tp->count[1] = counts[1];


    /* above this line is all setup; below this line the field is changed */


    /* first do the data, then do the grid.
     * this is because depending on what "how" is, this subroutine
     * might change the "altergrid" setting.
     */
    if (dodata(tp) == ERROR)
	goto error;
    

    /* only do the stuff below if there is something to do */
    if (tp->grid == 0 && tp->altergrid == 0 && tp->dotodata == 0) {

	if (!DXEndField(field))
	    return ERROR;

	return OK;
    }


    /* hack because i can't get the logic behind this right.
     */
    otherdim = 0;
/*    DXMessage("grid = %d, how now = %d\n", tp->altergrid, tp->how); */
    switch (tp->altergrid) {
      case 0:
	if (tp->how == 4 || tp->how == 6)
	    otherdim++;
	break;
      case 1:
	if (tp->how == 1 || tp->how == 3)
	    otherdim++;
	break;
      case 2:
	if (tp->how == 0 || tp->how == 2)
	    otherdim++;
	break;
      case 3:
	if (tp->how == 5 || tp->how == 7)
	    otherdim++;
	break;
      case 4:
	if (tp->how == 0 || tp->how == 2)
	    otherdim++;
	break;
      case 5:
	if (tp->how == 5 || tp->how == 7)
	    otherdim++;
	break;
      case 6:
	if (tp->how == 4 || tp->how == 6)
	    otherdim++;
	break;
      case 7:
	if (tp->how == 1 || tp->how == 3)
	    otherdim++;
	break;
    }

    /* now fix the counts and deltas to match the order the
     * Display module is most comfortable with.
     */
    
    if (tp->altergrid & INV_COLS) {
	origin[0] += deltas[0] * (counts[0]-1) + deltas[2] * (counts[1]-1);
	if (deltas[0] != 0.0)
	    deltas[0] = -deltas[0];
	if (deltas[2] != 0.0)
	    deltas[2] = -deltas[2];
    }
    if (tp->dotodata & INV_COLS) {
	if (otherdim)
	    meshextents[1] = tp->maxextent[1] - counts[1] - meshextents[1];
	else
	    meshextents[0] = tp->maxextent[0] - counts[0] - meshextents[0];
    }
    if (tp->altergrid & INV_ROWS) {
	origin[1] += deltas[1] * (counts[0]-1) + deltas[3] * (counts[1]-1);
	if (deltas[1] != 0.0)
	    deltas[1] = -deltas[1];
	if (deltas[3] != 0.0)
	    deltas[3] = -deltas[3];
    } 
    if (tp->dotodata & INV_ROWS) {
	if (otherdim)
	    meshextents[0] = tp->maxextent[0] - counts[0] - meshextents[0];
	else
	    meshextents[1] = tp->maxextent[1] - counts[1] - meshextents[1];
    }
    if (tp->altergrid & INV_TRANSPOSE) {
	ftemp = deltas[2];
	deltas[2] = deltas[0];
	deltas[0] = ftemp;

	ftemp = deltas[3];
	deltas[3] = deltas[1];
	deltas[1] = ftemp;
    }
    if (tp->altergrid & INV_EXCHANGE) {
	itemp = counts[0];
	counts[0] = counts[1];
	counts[1] = itemp;
    }
    if (tp->dotodata & INV_TRANSPOSE) {
	itemp = meshextents[0];
	meshextents[0] = meshextents[1];
	meshextents[1] = itemp;
    }
    

#if 0
    DXMessage("new extents: %d %d", meshextents[0], meshextents[1]);
#endif


    /* redo the positions component */
    new_positions = DXMakeGridPositionsV(2, counts, origin, deltas);
    if (!new_positions)
	goto error;
    
    /* redo the connections component */
    new_connections = DXMakeGridConnectionsV(2, counts);
    if (!new_connections)
	goto error;

   
    /* and for partitioned data, fix the mesh extents */
    if (tp->partitioned) {
	if (!DXSetMeshOffsets((MeshArray)new_connections, (int *)meshextents))
	    return ERROR;
    }
    
    
    /* place in field */
    if (!DXSetComponentValue(field, "positions", (Object)new_positions))
	goto error;
    if (!DXSetComponentValue(field, "connections", (Object)new_connections))
	goto error;
    
    if (!DXChangedComponentValues(field, "positions"))
	goto error;
    if (!DXChangedComponentValues(field, "connections"))
	goto error;

    if (!DXEndField(field))
	goto error;

    return OK;
    
  error:
    DXDelete((Object)new_positions);
    DXDelete((Object)new_connections);
    return ERROR;
}


#define MAXSHAPE 64

static Error dodata (Rtaskarg *tp)
{
    Field field;
    Array a, new_a = NULL;

    int yfastest;
    int loopcount[2];
    int cnum;
    int invert;

    Pointer fromptr, toptr;
    int size;
    ubyte *from1, *to1;
    int *from4, *to4;
    typedef struct {
	ubyte _u[3];
    } threebytes;
    threebytes *from3, *to3;
    typedef struct {
	int _i[3];
    } twelvebytes;
    twelvebytes *from12, *to12;
    int i, ii, j, jj, k;
    
    char *name, *dattr, *rattr;
    int nodeps, norefs;
    int items;
    Type type;
    Category cat;
    int rank, shape[MAXSHAPE];

    int didwork;


    
    field = (Field)tp->o;

    yfastest = tp->grid & Y_FASTEST;
    /*negative_x = tp->grid & NEGATIVE_X;*/
    /*negative_y = tp->grid & NEGATIVE_Y;*/

#if 1    /* in again */   /*xxx  out for now */

#if 0
    DXMessage("yfastest = %d, neg_x = %d, neg_y = %d",
	      yfastest, negative_x, negative_y);
#endif

    /* take care of all the possibilities; inverted axis and
     * different data orientations.  alter tp->how to make it match
     * what happens with the x-fastest, positive x & y deltas.
     */
    if (tp->how != 0) {
	invert  = tp->grid &  Y_FASTEST ? 1 : 0;
	invert += tp->grid & NEGATIVE_X ? 1 : 0;
	invert += tp->grid & NEGATIVE_Y ? 1 : 0;
	switch (invert) {
	  case 2:
	    switch (tp->how) {
	      case 4:
		tp->how = 7;
		break;
	      case 5:
		tp->how = 4;
		break;
	      case 6:
		tp->how = 5;
		break;
	      case 7:
		tp->how = 6;
		break;
	    }
	    break;
	  case 1:
	  case 3:
	    switch (tp->how) {
	      case 1:
		tp->how = 3;
		break;
	      case 3:
		tp->how = 1;
		break;
	      case 5:
		tp->how = 6;
		break;
	      case 6:
		tp->how = 5;
		break;
	      case 4:
		tp->how = 7;
		break;
	      case 7:
		tp->how = 4;
		break;
	    }
	    break;
	}
    }

    /* now that we know something about how the input image is
     * oriented, see how the user wants it to go out.  
     */
    switch (tp->how) {
      case 0:
	/* just make the image in the order the Display module
         * is happiest handling.
	 */
	break;

      case 4:
	tp->alterdata |= (yfastest ? INV_ROWS : INV_COLS);
	break;

      case 6:
	tp->alterdata |= (yfastest ? INV_COLS : INV_ROWS);
	break;

      case 2:
	tp->alterdata |= INV_ROWS;
	tp->alterdata |= INV_COLS;
	break;

      case 7:
	tp->altergrid ^= INV_EXCHANGE;
	tp->alterdata |= INV_TRANSPOSE;
	break;

      case 3:
	tp->altergrid ^= INV_EXCHANGE;
	tp->alterdata |= INV_TRANSPOSE;
	tp->alterdata |= (yfastest ? INV_ROWS : INV_COLS);
	break;

      case 1:
	tp->altergrid ^= INV_EXCHANGE;
	tp->alterdata |= INV_TRANSPOSE;
	tp->alterdata |= (yfastest ? INV_COLS : INV_ROWS);
	break;

      case 5:
	tp->altergrid ^= INV_EXCHANGE;
	tp->alterdata |= INV_TRANSPOSE;
	tp->alterdata |= INV_ROWS;
	tp->alterdata |= INV_COLS;
	break;
    }


    /* now compare what happens to the grid vs data.  if we're
     * moving the grid we can leave the data alone; if we are
     * not moving the grid we have to move the data.
     */
    if ((tp->altergrid & INV_TRANSPOSE) ^ (tp->alterdata & INV_TRANSPOSE))
	tp->dotodata |= INV_TRANSPOSE;
    if ((tp->altergrid & INV_ROWS) ^ (tp->alterdata & INV_ROWS))
	tp->dotodata |= INV_ROWS;
    if ((tp->altergrid & INV_COLS) ^ (tp->alterdata & INV_COLS))
	tp->dotodata |= INV_COLS;
    
    
#else



    switch (tp->how) {
      case 0:
	/* just make the image in the order the Display module
         * is happiest handling.
	 */
	break;

      case 4:
	tp->alterdata |= INV_COLS;
	break;

      case 6:
	tp->alterdata |= INV_ROWS;
	break;

      case 2:
	tp->alterdata |= INV_ROWS | INV_COLS;
	break;

#define FLIP(what) ((tp->altergrid & (what)) ^ (what))

      case 7:
#if 0
	tp->altergrid ^= INV_EXCHANGE;
	tp->alterdata |= FLIP (INV_TRANSPOSE);
#endif
	tp->alterdata |= INV_EXCHANGE | INV_TRANSPOSE | INV_EXCHANGE;
	break;

      case 3:
#if 0
	tp->altergrid ^= INV_EXCHANGE;
	tp->alterdata |= FLIP (INV_TRANSPOSE | INV_COLS);
#endif
	tp->alterdata |= INV_EXCHANGE | INV_TRANSPOSE | INV_EXCHANGE | INV_COLS;
	break;

      case 1:
#if 0
	tp->altergrid ^= INV_EXCHANGE;
	tp->alterdata |= FLIP (INV_TRANSPOSE | INV_ROWS);
#endif
	tp->alterdata |= INV_EXCHANGE | INV_TRANSPOSE | INV_EXCHANGE | INV_ROWS;
	break;

      case 5:
#if 0
	tp->altergrid ^= INV_EXCHANGE;
	tp->alterdata |= FLIP (INV_TRANSPOSE | INV_ROWS | INV_COLS);
#endif
	tp->alterdata |= INV_EXCHANGE | INV_TRANSPOSE | INV_EXCHANGE | INV_ROWS | INV_COLS;
	break;
    }


#if 0
    DXMessage("before grid = 0x%04x, data = 0x%04x", 
	      tp->altergrid, tp->alterdata);
#endif

#define TOGGLE(what, what1, bits)  (what ^= (what1 & bits))

    TOGGLE(tp->altergrid, tp->alterdata, INV_EXCHANGE);
    tp->alterdata &= ~INV_EXCHANGE;
    TOGGLE(tp->alterdata, tp->altergrid, INV_TRANSPOSE);
    TOGGLE(tp->alterdata, tp->altergrid, INV_ROWS);
    TOGGLE(tp->alterdata, tp->altergrid, INV_COLS);

#if 0
    DXMessage("after grid = 0x%04x, data = 0x%04x", 
	      tp->altergrid, tp->alterdata);
#endif


#endif 

#if 0
    DXMessage("altergrid = %d, alterdata = %d, dotodata = %d",
	      tp->altergrid, tp->alterdata, tp->dotodata);
#endif
    
    /* if we don't have to change the data in the components, we're done */
    if (tp->dotodata == INV_NOTHING) 
	return OK;


    for (cnum = 0; 
	 (a = (Array)DXGetEnumeratedComponentValue(field, cnum, &name)); cnum++) {

	if (!strcmp(name, "positions") || !strcmp(name, "connections"))
	    continue;

	/* if no deps and no refs, just pass this component through untouched.
	 */
	nodeps = 0;
	dattr = NULL;
	if (!DXGetStringAttribute((Object)a, "dep", &dattr) || !dattr)
	    nodeps++;
	
	norefs = 0;
	rattr = NULL;
	if (!DXGetStringAttribute((Object)a, "ref", &rattr) || !rattr)
	    norefs++;

	if (nodeps && norefs)
	    continue;
	
	if (dattr && strcmp(dattr, "positions") && strcmp(dattr, "connections"))
	    continue;

	if (nodeps && !norefs) {
	    /* this is not dep anything but is ref something.  to do this
	     * right, i need to reorder the refs the same way i reorder
	     * the target.  i don't have the code to do this written, but
	     * i've got it in slab/slice and other places.
	     */
	    continue;
	}

	
	/* ok, we got a live one */

	if (!strcmp(dattr, "positions")) {
	    loopcount[0] = tp->count[0];
	    loopcount[1] = tp->count[1];
	} else {
	    loopcount[0] = tp->count[0] - 1;
	    loopcount[1] = tp->count[1] - 1;
	}
	    

	size = DXGetItemSize(a);
	fromptr = DXGetArrayData(a);
	if (!fromptr)
	    goto error;

	if (!DXGetArrayInfo(a, &items, &type, &cat, &rank, shape))
	    goto error;

	new_a = DXNewArrayV(type, cat, rank, shape);
	if (!new_a)
	    goto error;

	if (!DXAddArrayData(new_a, 0, items, NULL))
	    goto error;

	toptr = DXGetArrayData(new_a);
	if (!toptr)
	    goto error;

#define MOVEDATA1(op, np) \
 \
     if (tp->dotodata == INV_TRANSPOSE) { \
	 for (i=0; i<loopcount[1]; i++) \
	     for (j=0; j<loopcount[0]; j++) \
		 np[i*loopcount[0] + j] = op[j*loopcount[1] + i]; \
    } \
\
    if (tp->dotodata == INV_ROWS) { \
	for (i=0; i<loopcount[1]; i++) \
	    for (j=0, jj=loopcount[0]-1; j<loopcount[0]; j++, --jj) \
		np[j*loopcount[1] + i] = op[jj*loopcount[1] + i]; \
    } \
\
    if (tp->dotodata == INV_COLS) { \
	for (i=0, ii=loopcount[1]-1; i<loopcount[1]; i++, --ii) \
	    for (j=0; j<loopcount[0]; j++) \
		np[j*loopcount[1] + i] = op[j*loopcount[1] + ii]; \
    } \
\
    if (tp->dotodata == (INV_COLS|INV_ROWS)) { \
	for (i=0, ii=loopcount[1]-1; i<loopcount[1]; i++, --ii) \
	    for (j=0, jj=loopcount[0]-1; j<loopcount[0]; j++, --jj) \
		np[j*loopcount[1] + i] = op[jj*loopcount[1] + ii]; \
    } \
\
    if (tp->dotodata == (INV_TRANSPOSE|INV_COLS)) { \
	for (i=0; i<loopcount[1]; i++) \
	    for (j=0, jj=loopcount[0]-1; j<loopcount[0]; j++, --jj) \
		np[i*loopcount[0] + j] = op[jj*loopcount[1] + i]; \
    } \
\
    if (tp->dotodata == (INV_TRANSPOSE|INV_ROWS)) { \
	for (i=0, ii=loopcount[1]-1; i<loopcount[1]; i++, --ii) \
	    for (j=0; j<loopcount[0]; j++) \
		np[i*loopcount[0] + j] = op[j*loopcount[1] + ii]; \
    } \
\
    if (tp->dotodata == (INV_TRANSPOSE|INV_COLS|INV_ROWS)) { \
	for (i=0, ii=loopcount[1]-1; i<loopcount[1]; i++, --ii) \
	    for (j=0, jj=loopcount[0]-1; j<loopcount[0]; j++, --jj) \
		np[i*loopcount[0] + j] = op[jj*loopcount[1] + ii]; \
    }

	
#define MOVEDATA(op, np, size) \
 \
    if (tp->dotodata == INV_TRANSPOSE) { \
	for (i=0; i<loopcount[1]; i++) \
	    for (j=0; j<loopcount[0]; j++) \
		for (k=0; k<size; k++) \
		    np[(i*loopcount[0] + j)*size + k] = \
			op[(j*loopcount[1] + i)*size + k]; \
    } \
\
    if (tp->dotodata == INV_ROWS) { \
	for (i=0; i<loopcount[1]; i++) \
	    for (j=0, jj=loopcount[0]-1; j<loopcount[0]; j++, --jj) \
		for (k=0; k<size; k++) \
		    np[(j*loopcount[1] + i)*size + k] = \
			op[(jj*loopcount[1] + i)*size + k]; \
    } \
\
    if (tp->dotodata == INV_COLS) { \
	for (i=0, ii=loopcount[1]-1; i<loopcount[1]; i++, --ii) \
	    for (j=0; j<loopcount[0]; j++) \
		for (k=0; k<size; k++) \
		    np[(j*loopcount[1] + i)*size + k] = \
			op[(j*loopcount[1] + ii)*size + k]; \
    } \
\
    if (tp->dotodata == (INV_COLS|INV_ROWS)) { \
	for (i=0, ii=loopcount[1]-1; i<loopcount[1]; i++, --ii) \
	    for (j=0, jj=loopcount[0]-1; j<loopcount[0]; j++, --jj) \
		for (k=0; k<size; k++) \
		    np[(j*loopcount[1] + i)*size + k] = \
			op[(jj*loopcount[1] + ii)*size + k]; \
    } \
\
    if (tp->dotodata == (INV_TRANSPOSE|INV_COLS)) { \
	for (i=0; i<loopcount[1]; i++) \
	    for (j=0, jj=loopcount[0]-1; j<loopcount[0]; j++, --jj) \
		for (k=0; k<size; k++) \
		    np[(i*loopcount[0] + j)*size + k] = \
			op[(jj*loopcount[1] + i)*size + k]; \
    } \
\
    if (tp->dotodata == (INV_TRANSPOSE|INV_ROWS)) { \
	for (i=0, ii=loopcount[1]-1; i<loopcount[1]; i++, --ii) \
	    for (j=0; j<loopcount[0]; j++) \
		for (k=0; k<size; k++) \
		    np[(i*loopcount[0] + j)*size + k] = \
			op[(j*loopcount[1] + ii)*size + k]; \
    } \
\
    if (tp->dotodata == (INV_TRANSPOSE|INV_COLS|INV_ROWS)) { \
	for (i=0, ii=loopcount[1]-1; i<loopcount[1]; i++, --ii) \
	    for (j=0, jj=loopcount[0]-1; j<loopcount[0]; j++, --jj) \
		for (k=0; k<size; k++) \
		    np[(i*loopcount[0] + j)*size + k] = \
			op[(jj*loopcount[1] + ii)*size + k]; \
    }


	if (size == 4) {
	    from4 = (int *)fromptr;
	    to4 = (int *)toptr;
	    MOVEDATA1(from4, to4);
	}
	
	else if (size == 12) {
	    from12 = (twelvebytes *)fromptr;
	    to12 = (twelvebytes *)toptr;
	    MOVEDATA1(from12, to12);
	}
	
	else if (size == 3) {
	    from3 = (threebytes *)fromptr;
	    to3 = (threebytes *)toptr;
	    MOVEDATA1(from3, to3);
	}
	
	else {
	    from1 = (ubyte *)fromptr;
	    to1 = (ubyte *)toptr;
	    MOVEDATA(from1, to1, size);
	}


	if (!DXSetComponentValue(field, name, (Object)new_a))
	    goto error;
	
	new_a = NULL;

    }

    /* we are changing positions & connections - make sure things
     * which are derived from them are deleted.
     */
    DXChangedComponentValues(field, "positions");
    DXChangedComponentValues(field, "connections");



    /* do this as a second pass so the enumeration count doesn't get mangled.
     */
  again:
    didwork = 0;
    for (i=0; (a = (Array)DXGetEnumeratedComponentValue(field, i, &name)); i++) {
	
	if (!strcmp(name, "positions") || !strcmp(name, "connections"))
	    continue;
	
	/* if no deps and no refs, just pass this component through untouched.
	 */
	nodeps = 0;
	dattr = NULL;
	if (!DXGetStringAttribute((Object)a, "dep", &dattr) || !dattr)
	    nodeps++;
	
	norefs = 0;
	rattr = NULL;
	if (!DXGetStringAttribute((Object)a, "ref", &rattr) || !rattr)
	    norefs++;
	
	if (nodeps && norefs)
	    continue;
	
	if (dattr && strcmp(dattr, "positions") && strcmp(dattr, "connections"))
	    continue;

	
	/* the list of things which aren't going to be valid at the end
         * after moving the positions & connections, so nuke them now rather
	 * than passing them through and having them be wrong.
	 */
	if ((rattr && !strcmp(rattr, "positions"))
	    || (rattr && !strcmp(rattr, "connections"))) {
	    DXSetComponentValue(field, name, NULL);  /* screws enum count? */
	    DXChangedComponentValues(field, name);
	    didwork++;
	    goto again;
	}
	
    }   
    
    /* done */
    if (!DXEndField(field))
	return ERROR;
    
    return OK;
    
  error:
    DXDelete((Object)new_a);
    return ERROR;
}






