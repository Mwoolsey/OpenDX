/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/* 
 * (C) COPYRIGHT International Business Machines Corp. 1992, 1997
 * All Rights Reserved
 * Licensed Materials - Property of IBM
 * 
 * US Government Users Restricted Rights - Use, duplication or
 * disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
 */

#include <dxconfig.h>

#if defined(HAVE_CTYPE_H)
#include <ctype.h>
#endif

#if defined(HAVE_STRING_H)
#include <string.h>
#endif

#include <dx/dx.h>
#include <math.h>

/* things to add - components w/ no dep/ref/der - copy first.
 *  handle renumbering of ref components 
 */



/* generic groups can be handled three different ways:
 *  1. if group members are fields, create a 0 .. N set of series pos
 *     and stack the members and output a single field.
 *  2. if the members are different series, stack each one separately
 *     and output a group of stacked fields.
 *  3. since a single field coming into stack gets the positions up'd by 1D.
 *     a group of fields could also be interpreted as a request to up each
 *     field by 1d and output a group of fields.
 * given my druthers i guess i'd either prefer always #1, or look at the
 * members and do either 1 or 2 based on what the first member type is.
 *
 * and how should multigrids be treated?  for now the same as generic groups,
 * whatever i decide to do with them.
 */

typedef struct stackparms { /* input from the user */
    Object inobj;        /* original top level object */
    int stackdim;        /* connection dimension to stack on */
    int newposdim;       /* new dimension to add to pos if connD != posD */
} StackParms;

typedef struct stackinfo {  /* stuff specific to each group/series/field */
    Object thisobj;      /* the thing currently being stacked */
    Class inclass;       /* kind (class) of input object */
    int stackflags;      /* flags - set for handling special cases */
    int posdim;          /* dimensionality of positions */
    int conndim;         /* dimensionality of connections */
    int membercnt;       /* number of members in the series */
    Field f0;            /* first field, used to compare to rest of stack */
    Array ap0;           /* first positions array */
    Array ac0;           /* first connections array */
    int *countlist;      /* connections counts in stacked obj */
    char **compname;     /* list of component names in every member */
    int *compflags;      /* flags per component name */
    int compcount;       /* count of components in list */
/* should stackparms pointer be here as well? */
} StackInfo;

/* bitmasks for stackflags - not all used */
#define PARTITIONED  0x1    /* input is partitioned */
#define PD_NE_CD     0x2    /* connD < positionD e.g. lines in 2space */
#define PARTIALCOMP  0x4    /* some fields have components missing in others */
#define DIFFDELTAS   0x8    /* inputs are regular but have different deltas */
#define JUSTPOS      0x10   /* input is a field; just stack positions */
#define CREATESERIES 0x20   /* input is group; create artificial series */
#define CONTAINER    0x40   /* top obj is container; needs traverse */
#define EMPTYOUT     0x80   /* entire input is empty fields */
#define INVALIDS     0x100  /* input has some sort of invalid component: */
#define INVPOS       0x200  /* invalid positions */
#define INVCON       0x400  /*    "    connections */
#define HASREFS      0x800  /* has at least one "ref" component */
#define POINTS       0x1000 /* input is a series of arrays */

/* bitmasks for compflags */
#define SKIP         0x1    /* skip this component while stacking */
#define WARNEDDCON   0x2    /* warning already printed for dep conn */
#define WARNEDMISS   0x4    /* warning printed for comp not in all fields */
#define ISREF        0x8    /* is ref another component and needs renumbering */
#define DOFIRST      0x10   /* if set, copy only first occurance of component */ 
#define DONEONE      0x20   /* set when one copy has been done */


/* general bit operators */
#define SET(flag, mask)    (flag |= mask)
#define CLR(flag, mask)    (flag &= ~mask)
#define ISSET(flag, mask)  ((flag & mask) == mask)
#define ISCLR(flag, mask)  ((flag & mask) == 0)

#define INTERNALERROR DXSetError(ERROR_INTERNAL, "at line %d, file %s", \
				 __LINE__, __FILE__)

#define NORMFUZZ   (DXD_FLOAT_EPSILON*20.0)
				/* sort of a normalized fuzz.  on normalized
				 * numbers (around 1.0), accept a difference 
				 * of 2 in the second least significant digit 
				 * when taking the difference of two deltas.
			         */




static Error parseparms(Object *in, StackParms *sp);
static Object doStack(StackParms *sp, StackInfo *si);
static Error setupwork(StackParms *sp, StackInfo *si);
static Object dowork(StackParms *sp, StackInfo *si);
static Object stackpos(StackParms *sp, StackInfo *si, float where);
static Array add1posdim(Array oldpos, int newdim, float where);
static Array addNposdim(StackInfo *si, int nflag, int oflag, int newdim, Array addpos);
static Array addpositions(StackParms *sp, StackInfo *si);
static Array addcondim(StackInfo *si, Array oldcon, int newdim, int newcount);
static Array addarrays(StackParms *sp, StackInfo *si, char *component, int flags);
static Array addirregarrays(StackInfo *si, char *component, int stackdim);
static Array addrefarrays(StackInfo *si, char *component, int stackdim);
static Array makerefs(StackInfo *si, int stackdim);
static Error dorenumber(Array newarr, Array renumber);
static Array addcnstarrays(StackParms *sp, StackInfo *si, char *component);
static Array addseriesdim(StackInfo *si, Array oldpos, Array addpos, int stackdim);
static Error newdimregular(StackParms *sp, StackInfo *si, Array *newarr, int *regflag);
static Error allposregular(StackInfo *si, int *regflag);
static Error seriesregular(StackParms *sp, StackInfo *si, Array *newpos);
static Array seriesspacing(StackParms *sp, StackInfo *si);
static Object stackgroup(StackParms *sp, StackInfo *si);
static Field getfirstfield(StackInfo *si);
static Error checksample(StackParms *sp, StackInfo *si);
static Error checkall(StackParms *sp, StackInfo *si);
static Error arraymatch(Array a1, Array a2, int countflag, char *name);
static Error ispartitioned(StackInfo *si);
static Error isstackable(StackParms *sp, StackInfo *si);
static Object invertobj(StackInfo *si);
static Object convertseries(StackInfo *si);
static Field setinvalids(StackInfo *si, Field f);
static void freespace(StackInfo *si);
static double nonzeromax(double a, double b);
static Array foolInvalids(Array a, char *name, Field f, int seriesmember);
extern void _dxfPermute(int dim, Pointer out, int *ostrides, int *counts,  /* from libdx/permute.c */
 		        int size, Pointer in, int *istrides);

/*
 *  inputs:  series, group or field
 *           dimension in which to stack
 *           position dim if stacking nD conns in (n+1)D pos space
 *  output:  stacked data
 */ 
Error
m_Stack(Object *in, Object *out)
{
    StackParms sp;
    StackInfo si;

    memset((Pointer)&sp, '\0', sizeof(StackParms));
    memset((Pointer)&si, '\0', sizeof(StackInfo));


    if (!parseparms(in, &sp))
	goto error;


    si.thisobj = sp.inobj;

    out[0] = doStack(&sp, &si);
    if (out[0] == NULL)
	goto error;


    return OK;

  error:

    out[0] = NULL;
    return ERROR;
}


/*
 * check input parms and set defaults.  allow x, y, z strings as
 * aliases for 0, 1, 2 as dim numbers.  there are two dimension
 * numbers in the case that connD != posD.  you need to know
 * what connD to stack in, and then the posD can be a different value.
 * think about stacking 2D lines into 3D quads.  you can stack
 * the lines horizonally or vertically, and then you can make the 
 * resulting sheet of quads be located parallel to the x, y or z axis.
 * if connD == posD, the third input is ignored (actually is an error to be
 * set to something different than the second input).
 */
static Error 
parseparms(Object *in, StackParms *sp)
{
    char *cp;

    if (!in[0]) {
	DXSetError(ERROR_BAD_PARAMETER, "#10000", "input");
	goto error;
    }
    sp->inobj = in[0];

    if (!in[1])
	sp->stackdim = 0;
    else {
	if (DXExtractInteger(in[1], &sp->stackdim)) {
	    if (sp->stackdim < 0) {
		DXSetError(ERROR_BAD_PARAMETER, "#10030", "dimension");
		goto error;
	    }
	} else if (DXExtractString(in[1], &cp)) {
	    if (isupper(*cp))
	        *cp = tolower(*cp);
	    if (*cp == 'x')
	        sp->stackdim = 0;
	    else if (*cp == 'y')
	        sp->stackdim = 1;
	    else if (*cp == 'z')
	        sp->stackdim = 2;
	    else {
		DXSetError(ERROR_BAD_PARAMETER, "#10211",
			   "dimension", "x, y, z");
		goto error;
	    }
	} else {
	    DXSetError(ERROR_BAD_PARAMETER, "#10650", "dimension");
	    goto error;
	}
    }
    
#if 1
    sp->newposdim = sp->stackdim;
#else
    if (!in[2])
	sp->newposdim = 0;
    else {
	if (DXExtractInteger(in[2], &sp->newposdim)) {
	    if (sp->newposdim < 0) {
		DXSetError(ERROR_BAD_PARAMETER, "#10030", "positionsdim");
		goto error;
	    }
	} else if (DXExtractString(in[2], &cp)) {
	    if (strcmp(cp, "x"))
		sp->newposdim = 0;
	    else if (strcmp(cp, "y"))
		sp->newposdim = 1;
	    else if (strcmp(cp, "z"))
		sp->newposdim = 2;
	    else {
		DXSetError(ERROR_BAD_PARAMETER, "#10211",
			   "dimension", "x", "y", "z");
		goto error;
	    }
	} else {
	    DXSetError(ERROR_BAD_PARAMETER, "#10650", "positionsdim");
	    goto error;
	}
    }
#endif

    return OK;

  error:
    return ERROR;
}

/* recursive routine.  stacks when it comes to a series, a
 * group or a field.
 */

static Object 
doStack(StackParms *sp, StackInfo *si)
{

    Object tostack, subo;
    Object stacked = NULL;
    Object returnobj = NULL;
    int i;
    Class inclass;

    tostack = si->thisobj;

    inclass = DXGetObjectClass(tostack);
    if (inclass == CLASS_GROUP)
	inclass = DXGetGroupClass((Group)tostack);

    si->inclass = inclass;

    switch (inclass) {

      /* GOOD */

      case CLASS_SERIES:
      case CLASS_FIELD:

	/* "normal" cases */

	/* free any pending space first before clearing this struct */
	freespace(si);
 
	memset(si, '\0', sizeof(StackInfo));
	si->thisobj = tostack;
	si->inclass = inclass;

	/* handle series of composite fields by restructuring the
         * input into a compositefield of series of fields, then stack
	 * each one into a single field.
	 */
	if (ispartitioned(si)) {
	    returnobj = invertobj(si);
	    if (!returnobj)
		goto error;

	    si->thisobj = returnobj;

	    stacked = doStack(sp, si);
	    if (!stacked)
		goto error;

	    DXDelete(returnobj);   /* delete temporary structures */
	    returnobj = stacked;
	    break;
	}
	
	if (!setupwork(sp, si))
	    goto error;

	if (!(returnobj = dowork(sp, si)))
	    goto error;

	freespace(si);
	break;

      case CLASS_GROUP:

	/* first check to see if the members are compatible fields.
	 * if so, stack them with 0 .. n series positions.  else
	 * assume they are separate objs and try to stack each one
	 * individually.
	 * 
	 * given that it's now easy to convert a generic group into
	 * a series (ChangeGroupType) i'm not sure this code should
	 * be here - maybe we should force the user to make a series
	 * themselves before coming into stack.  if so, just have the
	 * class_group case be the same as mg and cf and skip this code.
	 */
     
	if (isstackable(sp, si)) {

	    returnobj = convertseries(si);
	    if (!returnobj)
		goto error;
	    
	    si->thisobj = returnobj;
	    
	    stacked = doStack(sp, si);
	    if (!stacked)
		goto error;
	    
	    DXDelete(returnobj);   /* delete temporary structures */
	    returnobj = stacked;
	    break;
	}

	/* if not compatible objs, FALL THROUGH! */

      case CLASS_MULTIGRID:       /* ??? what should happen to one of these? */
      case CLASS_COMPOSITEFIELD:  

	returnobj = DXCopy(tostack, COPY_STRUCTURE);
	if (!returnobj)
	    goto error;

	for (i=0; (subo=DXGetEnumeratedMember((Group)returnobj, i, NULL)); i++) {
	    si->thisobj = subo;

	    stacked = doStack(sp, si);
	    if (!stacked)
		goto error;

	    if (!DXSetEnumeratedMember((Group)returnobj, i, stacked))
		goto error;
	}
	    
	break;

      /* container objs */
      case CLASS_XFORM:
	DXGetXformInfo((Xform)tostack, (Object *)&subo, NULL);
	si->thisobj = subo;

	stacked = doStack(sp, si);
	if (!stacked)
	    goto error;

	returnobj = DXCopy(tostack, COPY_STRUCTURE);
	if (!returnobj)
	    goto error;

	if (!DXSetXformObject((Xform)returnobj, stacked))
	    goto error;

	break;

      case CLASS_SCREEN:
	DXGetScreenInfo((Screen)tostack, (Object *)&subo, NULL, NULL);
	si->thisobj = subo;

	stacked = doStack(sp, si);
	if (!stacked)
	    goto error;

	returnobj = DXCopy(tostack, COPY_STRUCTURE);
	if (!returnobj)
	    goto error;

	if (!DXSetScreenObject((Screen)returnobj, stacked))
	    goto error;

	break;

      case CLASS_CLIPPED:
	DXGetClippedInfo((Clipped)tostack, (Object *)&subo, NULL);
	si->thisobj = subo;

	stacked = doStack(sp, si);
	if (!stacked)
	    goto error;

	returnobj = DXCopy(tostack, COPY_STRUCTURE);
	if (!returnobj)
	    goto error;

	if (!DXSetClippedObjects((Clipped)returnobj, stacked, NULL))
	    goto error;

	break;

      /* BAD */
      case CLASS_STRING:
      case CLASS_ARRAY:
      case CLASS_CAMERA:
      case CLASS_LIGHT:
      default:
	DXSetError(ERROR_BAD_PARAMETER, "#10190", "input");
	goto error;
    }


  /* done */
    freespace(si);
    return returnobj;

  error:
    freespace(si);
    DXDelete(returnobj);
    return NULL;
}


/* things which need to be checked before the actual bytes 
 *  are moved around.
 */
static Error
setupwork(StackParms *sp, StackInfo *si)
{
    if (!checksample(sp, si))
	return ERROR;
    
    if (ISSET(si->stackflags, JUSTPOS) || ISSET(si->stackflags, EMPTYOUT) 
	|| ISSET(si->stackflags, POINTS))
	return OK;
    
    if (!checkall(sp, si))
	return ERROR;

    return OK;
}



#define MAXRANK 8  /* largest inline rank supported.
		     * will handle larger by alloc'ing space for it 
		     */


/* pull out the first member and check things we can tell from
 * our selected victim.
 */
static Error 
checksample(StackParms *sp, StackInfo *si)
{
    Field testf;
    Array ap, ac, aa;
    int i, j;
    int invs;
    int comps;
    int count;
    int thiscomp;
    Type t;
    Category c;
    int rank, hereshape[MAXRANK], *genshape = NULL;
    char *cp, *ccp, *acp;


    invs = 0;
    /* mark that we'll have to deal with these at some point.  you
     * shouldn't have both, but it has been seen before.  can cause
     * strange things to happen.
     */
    if (DXExists(si->thisobj, "invalid positions")) {
	SET(si->stackflags, INVALIDS);
	SET(si->stackflags, INVPOS);
	invs++;
    }
    if (DXExists(si->thisobj, "invalid connections")) {
	SET(si->stackflags, INVALIDS);
	SET(si->stackflags, INVCON);
	invs++;
    }


    /* get the first nonempty field in the input, if there is one */

    if (si->inclass == CLASS_FIELD) {
	SET(si->stackflags, JUSTPOS);
	si->membercnt = 1;
	testf = (Field)si->thisobj;
    } else {
	if (!DXGetMemberCount((Group)si->thisobj, &si->membercnt))
	    goto error;
	testf = getfirstfield(si);
    }

    if (!testf) {
	if (ISSET(si->stackflags, POINTS)) {
	    si->conndim = 0;
	    goto done;
	}

	goto error;
    }
    
    if (DXEmptyField(testf)) {
	SET(si->stackflags, EMPTYOUT);
	goto done;
    }

    
    /* now we have it; dissect it. 
     */
    ap = (Array)DXGetComponentValue(testf, "positions");
    if (!ap) {
	/* shouldn't happen - we've tested for empty fields above */
	INTERNALERROR;
	goto error;
    }
    if ((!DXTypeCheckV(ap, TYPE_FLOAT, CATEGORY_REAL, 0, NULL)) &&
	(!DXTypeCheckV(ap, TYPE_FLOAT, CATEGORY_REAL, 1, NULL))) {
	DXSetError(ERROR_DATA_INVALID, "#11383");
	goto error;
    }
    
    if (!DXGetArrayInfo(ap, &count, &t, &c, &rank, hereshape))
	goto error;
    
    if (rank == 0)
	si->posdim = 0;
    else
	si->posdim = hereshape[0];

    ac = (Array)DXGetComponentValue(testf, "connections");
    if (!ac)
	si->conndim = 0;   /* no connections == scattered pts */
    else {
	/* get the dimensionality and make sure it's regular */
	if ((!DXTypeCheckV(ac, TYPE_INT, CATEGORY_REAL, 0, NULL)) &&
	    (!DXTypeCheckV(ac, TYPE_INT, CATEGORY_REAL, 1, NULL))) {
	    DXSetError(ERROR_DATA_INVALID, "#11382");
	    goto error;
	}
	
	if (!DXGetStringAttribute((Object)ac, "element type", &cp)) {
	    DXSetError(ERROR_DATA_INVALID, "#13070");
	    goto error;
	}

	if (!DXQueryGridConnections(ac, &rank, NULL)) {
	    DXSetError(ERROR_DATA_INVALID, "connections must be fully regular");
	    goto error;
	}

	/* think about tris here -> prisms */
	if (strcmp(cp, "points") == 0)
	    si->conndim = 0;
	else if (strcmp(cp, "lines") == 0)
	    si->conndim = 1;
	else if (strcmp(cp, "quads") == 0)
	    si->conndim = 2;
	else if (strcmp(cp, "cubes") == 0)
	    si->conndim = 3;
	else if (strncmp(cp, "cubes", 5) == 0) {
	    if (sscanf(cp+5, "%d", &si->conndim) != 1) {
		DXSetError(ERROR_DATA_INVALID, "#10610", "input");
		goto error;
	    }
	} else {
	    DXSetError(ERROR_DATA_INVALID, "#10610", "input");
	    goto error;
	}
	
    }

    if (sp->stackdim > si->conndim) {
	DXSetError(ERROR_BAD_PARAMETER, "#11360", sp->stackdim, "input",
		   si->conndim);
	goto error;
    }
    if (si->conndim != si->posdim) {
	if (sp->newposdim > si->posdim) {
	    DXSetError(ERROR_BAD_PARAMETER, "#11361", sp->newposdim, "input",
		       si->posdim);
	    goto error;
	}
    } else if (sp->stackdim != sp->newposdim) {
	DXSetError(ERROR_BAD_PARAMETER, "positiondimension can only be specified for lines in 2D or 3D, or surfaces in 3D");
	goto error;
    }
    
    
    /* save information for comparison with other fields in the group.
     */
    si->f0 = testf;
    si->ap0 = ap;
    si->ac0 = ac;

    /* and save the component names in a list. we can only stack
     * the ones found in all the fields.
     */
    comps = 0;
    for (i=0; DXGetEnumeratedComponentValue(testf, i, &cp); i++) {
	/* skip any here for now */
	if (strncmp("invalid", cp, strlen("invalid")) == 0)
	    continue;
	comps++;
    }

    si->compname = (char **)DXAllocateZero((comps + invs) * sizeof(char *));
    if (!si->compname)
	goto error;
    si->compflags = (int *)DXAllocateZero((comps + invs) * sizeof(int));
    if (!si->compflags)
	goto error;

    si->compcount = 0;
    for (i=0; (aa=(Array)DXGetEnumeratedComponentValue(testf, i, &ccp)); i++) {
	/* skip derived components */
	if (DXGetAttribute((Object)aa, "der"))
	    continue;

	/* pos & conn will be handled separately */
	if (!strcmp("positions", ccp) || !strcmp("connections", ccp))
	    continue;

	/* handle these separately below */
	if (strncmp("invalid", ccp, strlen("invalid")) == 0)
		continue;

	si->compname[si->compcount] = (char *)DXAllocate(strlen(ccp)+1);
	if (!si->compname[si->compcount])
	    goto error;
	strcpy(si->compname[si->compcount], ccp);
	thiscomp = si->compcount;
	si->compcount++;

	
	/* rest of components */
	if (DXGetAttribute((Object)aa, "dep") || 
	    DXGetAttribute((Object)aa, "ref")) {

	    /* fail if it's data, warn if any other comp */
            if (DXGetStringAttribute((Object)aa, "dep", &acp)) {
		
		if (!strcmp("connections", acp)) {
		    if (!strcmp("data", ccp)) {
			DXSetError(ERROR_NOT_IMPLEMENTED, 
				   "cannot stack data which is dependent on connections");
			goto error;
		    } else {
			if (ISCLR(si->compflags[thiscomp], WARNEDDCON)) {
			    DXWarning("cannot stack components which are dependent on connections; skipping `%s' component", ccp);
			    SET(si->compflags[thiscomp], SKIP);
			    SET(si->compflags[thiscomp], WARNEDDCON);
			}
		    }
		}
	    }

	    if (DXGetStringAttribute((Object)aa, "ref", &acp)) {
		SET(si->compflags[thiscomp], ISREF);
	    }
	} else {
	    /* if no dep, set a flag and only do the first */
	    SET(si->compflags[thiscomp], DOFIRST);
	}
    }

    /* now look through list and if anything is marked ref, see if the
     * target is skipped.  if so, we don't have to renumber the ref.
     */
    for (i=0; i<si->compcount; i++) {
	if (ISCLR(si->compflags[i], ISREF))
	    continue;

	aa = (Array)DXGetComponentValue(testf, si->compname[i]);
	DXGetStringAttribute((Object)aa, "ref", &acp);
	if (!acp) {
	    CLR(si->compflags[i], ISREF);
	    continue;
	}
	for(j=0; j<si->compcount; j++) {
	    if (strcmp(acp, si->compname[j]))
		continue;
	    /* target of ref */
	    if (ISSET(si->compflags[j], SKIP))
		CLR(si->compflags[i], ISREF);
	}
    }

    /* finally, if there are invalid pos or con, add in those components,
     * but they will be treated specially since they can be either dep
     * or ref.
     */
    for (i=0; i<invs; i++) {
	if (ISSET(si->stackflags, INVPOS)) {
	    si->compname[si->compcount] = 
		(char *)DXAllocate(strlen("invalid positions")+1);
	    if (!si->compname[si->compcount])
		goto error;
	    strcpy(si->compname[si->compcount], "invalid positions");
	    si->compcount++;
	}
	if (ISSET(si->stackflags, INVCON)) {
	    si->compname[si->compcount] = 
		(char *)DXAllocate(strlen("invalid connections")+1);
	    if (!si->compname[si->compcount])
		goto error;
	    strcpy(si->compname[si->compcount], "invalid connections");
	    si->compcount++;
	}
    }


  done:
    if (genshape)
	DXFree((Pointer)genshape);
    return OK;

  error:
    if (genshape)
	DXFree((Pointer)genshape);
    return ERROR;
}



/* find the first non-empty field in the input.  the input is at this point
 * only going to be a series or generic group.
 */
static Field 
getfirstfield(StackInfo *si)
{
    Object testf;
    Field retval=NULL;
    int i;
    int empty;
    
    
    if ((si->inclass != CLASS_SERIES) && (si->inclass != CLASS_GROUP)) {
	INTERNALERROR;
	goto error;
    }
    
    for (i=0, empty=1; 
	 empty && (testf=DXGetEnumeratedMember((Group)si->thisobj, i, NULL));
	 i++) {
	
	/* series of points can be stacked */
	if (DXGetObjectClass(testf) == CLASS_ARRAY) {
	    SET(si->stackflags, POINTS);
	    return NULL;
	}

	/* might get here with a CF --  check this XXX */
	if (DXGetObjectClass(testf) != CLASS_FIELD) {
	    DXSetError(ERROR_BAD_CLASS, 
		       "members of the series must be fields");
	    goto error;
	}
	
	retval = (Field)testf;
	
	if (DXEmptyField((Field)testf))
	    continue;
	
	empty = 0;
    }

    return retval;

  error:
    return NULL;
}




/* based on what the first member was, now look at the rest of the
 * series members and complain about inconsistent things (e.g. quads
 * in first field and cubes in second).
 */
static Error 
checkall(StackParms *sp, StackInfo *si)
{
    Field testf;
    Array ap, ac, aa;
    int i, j, tmp;
    char *cp;

    
    if ((si->inclass != CLASS_SERIES) && (si->inclass != CLASS_GROUP)) {
	INTERNALERROR;
	goto error;
    }
    
    for (i=0; 
	 (testf=(Field)DXGetEnumeratedMember((Group)si->thisobj, i, NULL));
	 i++) {
	
	if (DXGetObjectClass((Object)testf) != CLASS_FIELD) {
	    DXSetError(ERROR_BAD_CLASS, "members of the series must be fields");
	    goto error;
	}
	
	if (DXEmptyField(testf)) {
	    DXSetError(ERROR_BAD_CLASS, "series cannot contain Empty Fields");
	    goto error;
	}
	
	
	
	/* compare each to the first field
	 */
	ap = (Array)DXGetComponentValue(testf, "positions");
	if (!ap) {
	    /* shouldn't happen - we've tested for empty fields above */
	    INTERNALERROR;
	    goto error;
	}
	if ((!DXTypeCheckV(ap, TYPE_FLOAT, CATEGORY_REAL, 0, NULL)) &&
	    (!DXTypeCheckV(ap, TYPE_FLOAT, CATEGORY_REAL, 1, NULL))) {
	    DXSetError(ERROR_DATA_INVALID, "#11383");
	    goto error;
	}
	

	if (!arraymatch(ap, si->ap0, 1, "positions"))
	    goto error;
	
	
	ac = (Array)DXGetComponentValue(testf, "connections");
	if (!ac) {
	    if (si->conndim != 0)  /* all have to be scattered */
		goto error;
	} else {
	    /* get the dimensionality and make sure it's regular */
	    if ((!DXTypeCheckV(ac, TYPE_INT, CATEGORY_REAL, 0, NULL)) &&
		(!DXTypeCheckV(ac, TYPE_INT, CATEGORY_REAL, 1, NULL))) {
		DXSetError(ERROR_DATA_INVALID, "#11382");
		goto error;
	    }
	    
	    if (!DXGetStringAttribute((Object)ac, "element type", &cp)) {
		DXSetError(ERROR_DATA_INVALID, "#13070");
		goto error;
	    }
	    
	    if (!arraymatch(ac, si->ac0, 1, "connections"))
		goto error;
	
	    
	    /* think about tris here -> prisms */
	    if ((si->conndim == 0) && (strcmp(cp, "points") == 0))
		;  /* ok */
	    else if ((si->conndim == 1) && (strcmp(cp, "lines") == 0))
		;
	    else if ((si->conndim == 2) && (strcmp(cp, "quads") == 0))
		;
	    else if ((si->conndim == 3) && (strcmp(cp, "cubes") == 0))
		;
	    else if ((si->conndim > 3) && (strncmp(cp, "cubes", 5) == 0) &&
		     (sscanf(cp+5, "%d", &tmp) == 1) && tmp == si->conndim)
		;
	    else {
		DXSetError(ERROR_DATA_INVALID, "#10610", "input");
		goto error;
	    }
	    
	}


	/* make sure components match.
	 */


	/* need to beware of invalids - some might be dep & some ref
         * with different counts & types!
	 */
	for (j=0; j < si->compcount; j++) {
	    /* skip invalids here - they need to be on the list, but not 
	     * checked here.
	     */

	    if (strncmp("invalid", si->compname[j], strlen("invalid")) == 0)
		continue;

	    /* make this a warning instead of error? */
	    if (!(aa=(Array)DXGetComponentValue(testf, si->compname[j]))) {
		DXSetError(ERROR_DATA_INVALID, 
			   "at least one field is missing a %s component",
			   si->compname[j]);
		goto error;
	    }
	    
	    if (!arraymatch(aa, 
			    (Array)DXGetComponentValue(si->f0, si->compname[j]), 
			    1, si->compname[j]))
		goto error;
	}
    }
    

  /* done */
    return OK;

  error:
    return ERROR;
}

/* return ok if arrays match identically (plus rank 0 matches
 * r1, shape 1 and visa versa).  if countflag == 1, counts have
 * to match.  
 */
static Error 
arraymatch(Array a1, Array a2, int countflag, char *name)
{
    int i;
    int count1, count2;
    Type t1, t2;
    Category c1, c2;
    int rank1, rank2;
    int hereshape1[MAXRANK], *genshape1 = NULL;
    int hereshape2[MAXRANK], *genshape2 = NULL;

    
    if (!DXGetArrayInfo(a1, &count1, &t1, &c1, &rank1, NULL))
	goto error;
    
    if (rank1 < MAXRANK) {
	if (!DXGetArrayInfo(a1, &count1, &t1, &c1, &rank1, hereshape1))
	    goto error;
    } else {
	genshape1 = (int *)DXAllocate(rank1 * sizeof(int));
	if (!genshape1)
	    goto error;
	if (!DXGetArrayInfo(a1, &count1, &t1, &c1, &rank1, genshape1))
	    goto error;
	
    }
    
    if (!DXGetArrayInfo(a2, &count2, &t2, &c2, &rank2, NULL))
	goto error;
    
    if (rank2 < MAXRANK) {
	if (!DXGetArrayInfo(a2, &count2, &t2, &c2, &rank2, hereshape2))
	    goto error;
    } else {
	genshape2 = (int *)DXAllocate(rank2 * sizeof(int));
	if (!genshape2)
	    goto error;
	if (!DXGetArrayInfo(a2, &count2, &t2, &c2, &rank2, genshape2))
	    goto error;
	
    }
    
    if (countflag && (count1 != count2)) {
	DXSetError(ERROR_DATA_INVALID, 
		   "%s arrays are not the same length", name);
	goto error;
    }
    
    if (t1 != t2) {
	DXSetError(ERROR_DATA_INVALID, 
		   "%s arrays are not the same type", name);
	goto error;
    }
    
    if (c1 != c2) {
	DXSetError(ERROR_DATA_INVALID, 
		   "%s arrays are not the same category", name);
	goto error;
    }
    
    if (rank1 != rank2) {
	if ((rank1 != 0 && rank1 != 1) ||
	    (rank2 != 0 && rank2 != 1)) {
	    DXSetError(ERROR_DATA_INVALID, 
		       "%s arrays are not the same rank", name);
	    goto error;
	}
	if (rank1 == 1 && hereshape1[0] != 1) {
	    DXSetError(ERROR_DATA_INVALID, 
		       "%s arrays are not the same rank", name);
	    goto error;
	}
	if (rank2 == 1 && hereshape2[0] != 1) {
	    DXSetError(ERROR_DATA_INVALID, 
		       "%s arrays are not the same rank", name);
	    goto error;
	}
    }
    
    if (rank1 < MAXRANK) {
	for (i=0; i<rank1; i++)
	    if (hereshape1[i] != hereshape2[i]) {
		DXSetError(ERROR_DATA_INVALID, 
			   "%s arrays are not the same shape", name);
		goto error;
	    }
    } else {
	for (i=0; i<rank1; i++)
	    if (genshape1[i] != genshape2[i]) {
		DXSetError(ERROR_DATA_INVALID, 
			   "%s arrays are not the same shape", name);
		goto error;
	    }
    }
    
    /* they match */
    

	
  /* done */
    if (genshape1)
	DXFree((Pointer)genshape1);
    if (genshape2)
	DXFree((Pointer)genshape2);
    return OK;

  error:
    if (genshape1)
	DXFree((Pointer)genshape1);
    if (genshape2)
	DXFree((Pointer)genshape2);
    return ERROR;
}


/* if input is a series of composite fields, return OK
 * otherwise return error but do NOT set an error code.
 */
static Error 
ispartitioned(StackInfo *si)
{
    Object subo;
    
    if (si->inclass != CLASS_SERIES)
	goto error;
    
    subo = DXGetSeriesMember((Series)si->thisobj, 0, NULL);
    if (!subo)
	goto error;
    
    if (DXGetObjectClass(subo) == CLASS_GROUP && 
	DXGetGroupClass((Group)subo) == CLASS_COMPOSITEFIELD)
	return OK;

  error:
    /* set no error code */
    return ERROR;
}


/* if input is a generic group of fields which match, then
 * return ok, otherwise return error.  the error code MIGHT
 * be set - reset it here before returning.
 */
static Error 
isstackable(StackParms *sp, StackInfo *si)
{
    if (setupwork(sp, si) != ERROR)
	return OK;

    /* return with NO error code set */
    DXResetError();
    return ERROR;
}

/* input obj is series of composite fields.  output is a
 * composite field of series.  use meshoffsets to match up
 * corresponding members.
 */

static Object 
invertobj(StackInfo *si)
{
    CompositeField newtop = NULL;
    CompositeField first, next;
    Series news = NULL;
    float seriespos;
    Object subo;
    int i, ii, j, k;
    int cfcount;
    Array conn;
    int *meshoffsets = NULL;
    int meshoffsets2[MAXRANK];
    int meshsize, meshsize2;
    Array meshattr;

    /* lots o loops here */
    
    /* how about:  count up the number of partitions in series member 0
     *  make a cf and add each member to the cf as the first member of
     *  a new series.  record the mesh offsets at the series level.
     *  now for each other series member, loop through each partition,
     *  find the cf # which matches the mesh offsets and add that partition
     *  as the next series member.  draw pictures here - this is confusing.
     */

    newtop = DXNewCompositeField();
    if (!newtop)
	goto error;

    /* first composite field in the series */
    first = (CompositeField)DXGetSeriesMember((Series)si->thisobj, 0, &seriespos);

    if (!DXGetMemberCount((Group)first, &cfcount))
	goto error;

    /* first field in the composite field */
    subo = DXGetEnumeratedMember((Group)first, 0, NULL);
    if (!subo)
	goto done;
    
    /* figure out dimensionality of the connections and allocate space
     * for ALL the mesh offsets for all the partitions at once 
     */
    conn = (Array)DXGetComponentValue((Field)subo, "connections");
    if (!conn) {
	DXSetError(ERROR_DATA_INVALID, "cannot stack partitioned data if some fields do not have a 'connections' component");
	goto error;
    }
    
    if (!DXGetMeshArrayInfo((MeshArray)conn, &meshsize, NULL)) {
	DXSetError(ERROR_DATA_INVALID, 
		   "cannot stack partitioned data if all fields do not havea completely regular 'connections' component");
	goto error;
    }
    
    meshoffsets = (int *)DXAllocate(sizeof(int) * meshsize * cfcount);
    if (!meshoffsets)
	goto error;
    

    for (i=0; (subo=DXGetEnumeratedMember((Group)first, i, NULL)); i++) {
	news = DXNewSeries();
	if (!news)
	    goto error;
	
	if (!DXSetSeriesMember(news, 0, (double)seriespos, subo))
	    goto error;

	conn = (Array)DXGetComponentValue((Field)subo, "connections");
	if (!conn) {
	    DXSetError(ERROR_DATA_INVALID, "cannot stack partitioned data if some fields do not have a 'connections' component");
	    goto error;
	}
	
	if (!DXGetMeshArrayInfo((MeshArray)conn, &meshsize, NULL)) {
	    DXSetError(ERROR_DATA_INVALID, 
		       "cannot stack partitioned data if all fields do not havea completely regular 'connections' component");
	    goto error;
	}
	
	if (meshsize >= MAXRANK) {
	    INTERNALERROR;
	    goto error;
	}
	
	if (!DXGetMeshOffsets((MeshArray)conn, meshoffsets+(i*meshsize))) {
	    DXSetError(ERROR_DATA_INVALID, 
		       "cannot stack partitioned data if all fields do not havemesh offsets");
	    goto error;
	}
	
	meshattr = DXNewArray(TYPE_INT, CATEGORY_REAL, 0);
	if (!meshattr)
	    goto error;
	
	if (!DXAddArrayData(meshattr, 
			    0, meshsize, 
			    (Pointer)(meshoffsets+(i*meshsize))))
	    goto error;
	
	if (!DXSetAttribute((Object)news, "mesh offsets", (Object)meshattr))
	    goto error;
	
	
	if (!DXSetMember((Group)newtop, NULL, (Object)news))
	    goto error;
	
	news = NULL;
	
    }
    
    
    for (k=1; 
	 (next=(CompositeField)DXGetSeriesMember((Series)si->thisobj, k, &seriespos)); 
	 k++) {
	for (j=0; (subo=DXGetEnumeratedMember((Group)next, j, NULL)); j++) {
	    
	    
	    /* get each partition, find the mesh offsets, match them
             * up with the right cf member, and set it as the next member
	     * of that series.
	     */
	    
	    conn = (Array)DXGetComponentValue((Field)subo, "connections");
	    if (!conn) {
		DXSetError(ERROR_DATA_INVALID, "cannot stack partitioned data if some fields do not have a 'connections' component");
		goto error;
	    }
	    
	    if (!DXGetMeshArrayInfo((MeshArray)conn, &meshsize2, NULL)) {
		DXSetError(ERROR_DATA_INVALID, 
			   "cannot stack partitioned data if all fields do not havea completely regular 'connections' component");
		goto error;
	    }

	    /* shouldn't happen if we've gotten this far */
	    if (meshsize2 != meshsize) {
		INTERNALERROR;
		goto error;
	    }
	    
	    if (!DXGetMeshOffsets((MeshArray)conn, meshoffsets2)) {
		DXSetError(ERROR_DATA_INVALID, 
			   "cannot stack partitioned data if all fields do not havemesh offsets");
		goto error;
	    }

	    for (i=0; i<cfcount; i++) {
		for (ii=0; ii<meshsize; ii++) {
		    if (meshoffsets2[ii] != meshoffsets[i*meshsize + ii])
			break;
		}
		if (ii == meshsize)
		    break;
	    }
	    if (i == cfcount) {
		DXSetError(ERROR_DATA_INVALID,
			   "cannot stack partitioned data if all fields do not havemesh offsets");
		goto error;
	    }
	   
	    news = (Series)DXGetEnumeratedMember((Group)newtop, i, NULL);
	    if (!news) {
		INTERNALERROR;
		goto error;
	    }
	    
	    if (!DXSetSeriesMember(news, k, (double)seriespos, subo))
		goto error;
	}
    }
    

  done:
    DXFree((Pointer)meshoffsets);
    return (Object)newtop;

  error:
    DXFree((Pointer)meshoffsets);
    DXDelete((Object)newtop);
    DXDelete((Object)news);
    return NULL;
    
}

/* take a generic group and convert the members into
 * a series with series pos 0 .. n
 */
static Object 
convertseries(StackInfo *si)
{
    Series newtop = NULL;
    Object subo;
    int i;
    double seriespos = 0.0;


    newtop = DXNewSeries();
    if (!newtop)
	goto error;

    for (i=0, seriespos = 0.0; 
	 (subo=DXGetEnumeratedMember((Group)si->thisobj, i, NULL)); 
	 i++, seriespos += 1.0) 

	if (!DXSetSeriesMember(newtop, i, seriespos, subo))
	    goto error;



  /* done */
    return (Object)newtop;

  error:
    DXDelete((Object)newtop);
    return NULL;
    
}



/* actually do the stack
 */
static Object 
dowork(StackParms *sp, StackInfo *si)
{

    /* if the input is entirely empty, return an empty field
     */
    if (ISSET(si->stackflags, EMPTYOUT))
	return (Object)DXEndField(DXNewField());

    /* if the input is a single field, stack the positions only
     * and return.
     */
    if (DXGetObjectClass(si->thisobj) == CLASS_FIELD)
	return (stackpos(sp, si, 0.0));

    /* normal case - input is a series.  stack into a single
     * field and return.
     */
    return (stackgroup(sp, si));
}


/* add the requested dim to the positions component
 */
static Object 
stackpos(StackParms *sp, StackInfo *si, float where)
{
    Field retval = NULL;
    Array oldpos;
    Array newpos = NULL;

    retval = (Field)DXCopy(si->thisobj, COPY_STRUCTURE);
    if (!retval)
	return NULL;
    
    oldpos = (Array)DXGetComponentValue(retval, "positions");
    if (!oldpos)
	goto error;

    
    newpos = add1posdim(oldpos, sp->newposdim, where);
    if (!newpos)
	goto error;
    

    if (!DXSetComponentValue(retval, "positions", (Object)newpos))
	goto error;
    newpos = NULL;
    
    if (!DXChangedComponentValues(retval, "positions"))
	goto error;
    
    if (!DXEndField(retval))
	goto error;

    return (Object)retval;

  error:
    DXDelete((Object)retval);
    DXDelete((Object)newpos);
    return NULL;
}


/* add a fixed dimension to a position array.  we know these
 * have to be rank 1 and float, so we can cut corners 
 * in the array handling. 
 */
static Array 
add1posdim(Array oldpos, int newdim, float where)
{
    Array newpos = NULL;
    int i, j;
    Type t;
    Category c;
    int items, tcount;
    int rank, shape;
    Array terms[MAXRANK], *genterms = NULL;
    int counts[MAXRANK], *gencounts = NULL;
    float orig[MAXRANK], *genorig = NULL;
    float delta[MAXRANK*MAXRANK], *gendelta = NULL;
    float *ifp, *ofp;

#define FREEPTR(alloc, static)  if (alloc && (alloc != static)) \
                                        DXFree((Pointer)alloc)


    /* fully regular case */
    if (DXQueryGridPositions(oldpos, &shape, NULL, NULL, NULL)) {
	if (shape+1 < MAXRANK) {
	    if (!DXQueryGridPositions(oldpos, NULL, counts, orig, delta))
		goto error;
	    gencounts = counts;
	    genorig = orig;
	    gendelta = delta;
	} else {
	    items = sizeof(float) * (shape+1);
	    genorig = (float *)DXAllocate(items);
	    gencounts = (int *)DXAllocate(items);
	    
	    items *= (shape+1);
	    gendelta = (float *)DXAllocate(items);
	    if (!gencounts || !genorig || !gendelta)
		goto error;
	    
	    if (!DXQueryGridPositions(oldpos, NULL, gencounts, genorig, gendelta))
		goto error;
	}

	for (i=shape; i > newdim; --i) {
	    genorig[i] = genorig[i-1];
	    gencounts[i] = gencounts[i-1];
	}
	
	genorig[newdim] = where;
	gencounts[newdim] = 1;
	
	ifp = gendelta + shape * shape;
	ofp = gendelta + (shape+1) * (shape+1);
	
	for (i=shape; i >= 0; --i)
	    for (j=shape; j >= 0; --j) {
		if (i == newdim)
		    *--ofp = 0.0;
		else if (j == newdim)
		    *--ofp = 0.0;
		else
		    *--ofp = *--ifp;
	    }

	newpos = DXMakeGridPositionsV(shape+1, gencounts, genorig, gendelta);
	if (!newpos)
	    goto error;

	goto done;
    }
    

    /* not fully regular.  handle the compact forms when we can */
    DXGetArrayInfo(oldpos, &items, &t, &c, &rank, &shape);
    if (rank == 0)
	shape = 1;
    
    
    switch (DXGetArrayClass(oldpos)) {
      case CLASS_ARRAY:
      /* expanded */
	if (!(ifp = DXGetArrayData(oldpos)))
	    goto error;
	if (!(newpos = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, shape+1)))
	    goto error;
	if (!DXCopyAttributes((Object)newpos, (Object)oldpos))
	    goto error;
	if (!DXAddArrayData(newpos, 0, items, NULL))
	    goto error;
	if (!(ofp = DXGetArrayData(newpos)))
	    goto error;
	
	for (i=0; i<items; i++) 
	    for (j=0; j<shape+1; j++) {
		if (j == newdim)
		    *ofp++ = where;
		else
		    *ofp++ = *ifp++;
	    }
	
	break;
	
	
      case CLASS_REGULARARRAY:
	if (shape+1 < MAXRANK) {
	    if (!DXGetRegularArrayInfo((RegularArray)oldpos, &items, 
				       (Pointer)orig, (Pointer)delta))
		goto error;
	    genorig = orig;
	    gendelta = delta;
	} else {
	    genorig = (float *)DXAllocate(sizeof(float) * (shape+1));
	    gendelta = (float *)DXAllocate(sizeof(float) * (shape+1));
	    if (!genorig || !gendelta)
		goto error;
	    
	    if (!DXGetRegularArrayInfo((RegularArray)oldpos, &items, 
				       (Pointer)genorig, (Pointer)gendelta))
		goto error;
	}
	
	for (i=shape; i > newdim; --i) {
	    genorig[i] = genorig[i-1];
	    gendelta[i] = gendelta[i-1];
	}
	
	genorig[newdim] = where;
	gendelta[newdim] = 0.0;
	
	if (!(newpos = (Array)DXNewRegularArray(TYPE_FLOAT, shape+1, items,
	 			        (Pointer)genorig, (Pointer)gendelta)))
	    goto error;
	
	break;
	
      case CLASS_PRODUCTARRAY:

	if (!DXGetProductArrayInfo((ProductArray)oldpos, &tcount, NULL))
	    goto error;

	if (tcount+1 < MAXRANK) {
	    if (!DXGetProductArrayInfo((ProductArray)oldpos, &tcount, terms))
		goto error;
	    genterms = terms;
	} else {
	    genterms = (Array *)DXAllocate(sizeof(Array *) * (tcount+1));
	    if (!genterms)
		goto error;
	    if (!DXGetProductArrayInfo((ProductArray)oldpos, &tcount, genterms))
		goto error;
	}

	for (i=0; i<tcount; i++)
	    genterms[i] = add1posdim(genterms[i], newdim, where);

	for (i=tcount; i>newdim; i--)
	    genterms[i] = genterms[i-1];


	if (shape+1 < MAXRANK) {
	    genorig = orig;
	    gendelta = delta;
	} else {
	    genorig = (float *)DXAllocate(sizeof(float) * (shape+1));
	    gendelta = (float *)DXAllocate(sizeof(float) * (shape+1));
	    if (!genorig || !gendelta)
		goto error;
	}

	for (i=0; i<shape+1; i++) {
	    if (i == newdim)
		genorig[i] = where;
	    else
		genorig[i] = 0.0;
	    gendelta[i] = 0.0;
	}
	    
	if (!(terms[newdim] = (Array)DXNewRegularArray(TYPE_FLOAT, shape+1, 
						       items,
						       (Pointer)genorig, 
						       (Pointer)gendelta)))
	    goto error;
	
        
        if (!(newpos = (Array)DXNewProductArrayV(tcount+1, terms)))
	    goto error;

	break;


      /* degenerate, but perhaps useful after being computed on? */
      case CLASS_CONSTANTARRAY:
	if (shape+1 < MAXRANK) {
	    if (!DXQueryConstantArray(oldpos, &items, (Pointer)orig))
		goto error;
	    genorig = orig;
	} else {
	    genorig = (float *)DXAllocate(sizeof(float) * (shape+1));
	    if (!genorig)
		goto error;
	    if (!DXQueryConstantArray(oldpos, &items, (Pointer)genorig))
		goto error;
	}

	for (i=shape; i > newdim; --i)
	    genorig[i] = genorig[i-1];
	
	genorig[newdim] = where;
	    
	if (!(newpos = (Array)DXNewConstantArray(items, (Pointer)genorig, 
						 TYPE_FLOAT, CATEGORY_REAL,
						 1, shape+1)))
	    goto error;

	break;

      /* these are integer types - can't be used as the pos component */
      case CLASS_MESHARRAY:
      case CLASS_PATHARRAY:
      default:
	DXSetError(ERROR_BAD_CLASS, "Unrecognized array type in positions");
	goto error;
    }


  done:

    FREEPTR(gencounts, counts);
    FREEPTR(genorig, orig);
    FREEPTR(gendelta, delta);
    FREEPTR(genterms, terms);

    return newpos;
    
  error: 
    FREEPTR(gencounts, counts);
    FREEPTR(genorig, orig);
    FREEPTR(gendelta, delta);
    FREEPTR(genterms, terms);

    DXDelete((Object)newpos);
    return NULL;

}

/* add a new array to the positions array.  we know positions
 * have to be rank 1 and float, so this can be less general.
 * nflag is set if the new positions dimension is regular,
 * oflag is set if the entire series has a regular and compatible
 * set of series positions so the output object can be completely
 * regular in that dimension.  
 */
static Array 
addNposdim(StackInfo *si, int nflag, int oflag, int newdim, Array addpos)
{
    Array oldpos = NULL;
    Array newpos = NULL;
    Array newterm = NULL;
    int i, j;
    Type t;
    Category c;
    int items, tcount, rcount;
    int rank, shape;
    Array terms[MAXRANK], *genterms = NULL;
    int counts[MAXRANK], *gencounts = NULL;
    float orig[MAXRANK], *genorig = NULL;
    float delta[MAXRANK*MAXRANK], *gendelta = NULL;
    float rorig[MAXRANK], *genrorig = NULL;
    float rdelta[MAXRANK], *genrdelta = NULL;
    float *ifp, *ofp;
    Array tmppos = NULL;



    /* NEEDS CODE CHANGE HERE!!! XXX   the reg test should be:
     * are all the pos the same.  if so, then do the query on each
     * of the inputs and if not, go to the expanded case w/o the
     * error out.
     */

    /* fully regular case - both existing dim and new stacked dim */
    if (nflag && oflag) {
	oldpos = si->ap0;
	if (!DXQueryGridPositions(oldpos, &shape, NULL, NULL, NULL)) {
	    INTERNALERROR;
	    goto error;
	}
	if (!DXGetArrayClass(addpos) == CLASS_REGULARARRAY) {
	    INTERNALERROR;
	    goto error;
	}

	if (shape+1 < MAXRANK) {
	    if (!DXQueryGridPositions(oldpos, NULL, counts, orig, delta))
		goto error;
	    gencounts = counts;
	    genorig = orig;
	    gendelta = delta;
	    if (!DXGetRegularArrayInfo((RegularArray)addpos, &rcount, 
				       (Pointer)rorig, (Pointer)rdelta))
		goto error;
	    genrorig = rorig;
	    genrdelta = rdelta;
	} else {
	    items = sizeof(float) * (shape+1);
	    genorig = (float *)DXAllocate(items);
	    gencounts = (int *)DXAllocate(items);
	    genorig = (float *)DXAllocate(items);
	    gendelta = (float *)DXAllocate(items);

	    items *= (shape+1);
	    gendelta = (float *)DXAllocate(items);
	    if (!gencounts || !genorig || !gendelta || !genorig || !gendelta)
		goto error;
	    
	    if (!DXQueryGridPositions(oldpos, NULL, gencounts, genorig, gendelta))
		goto error;
	    if (!DXGetRegularArrayInfo((RegularArray)addpos, &rcount, 
				       (Pointer)genrorig, (Pointer)genrdelta))
		goto error;
	}

	for (i=shape; i > newdim; --i) {
	    genorig[i] = genorig[i-1];
	    gencounts[i] = gencounts[i-1];
	}
	
	genorig[newdim] = genrorig[newdim];
 	gencounts[newdim] = rcount;
	
	ifp = gendelta + shape * shape;
	ofp = gendelta + (shape+1) * (shape+1);
	
	for (i=shape; i >= 0; --i)
	    for (j=shape; j >= 0; --j) {
		if (i == newdim)
		    *--ofp = genrdelta[j];
		else if (j == newdim)
		    *--ofp = 0.0;
		else
		    *--ofp = *--ifp;
	    }

	newpos = DXMakeGridPositionsV(shape+1, gencounts, genorig, gendelta);
	if (!newpos)
	    goto error;

	goto done;
    }
    


    /* if oflag is not set, the old dim does not have compatible position
     * components and has to be completely expanded - all the series members.
     */
    if (!oflag) {
	oldpos = addirregarrays(si, "positions", newdim);
	if (!oldpos)
	    goto error;

	tmppos = oldpos;  /* to make sure this gets deleted at the end */
    } else
	oldpos = si->ap0;



    DXGetArrayInfo(oldpos, &items, &t, &c, &rank, &shape);
    if (rank == 0)
	shape = 1;
    
    
    switch (DXGetArrayClass(oldpos)) {
      case CLASS_ARRAY:
      /* expanded */
	newpos = addseriesdim(si, oldpos, addpos, newdim);
	if (!newpos)
	    goto error;
	
	break;
	
	
      case CLASS_REGULARARRAY:
	if (shape+1 < MAXRANK) {
	    if (!DXGetRegularArrayInfo((RegularArray)oldpos, &items, 
				       (Pointer)orig, (Pointer)delta))
		goto error;
	    genorig = orig;
	    gendelta = delta;
	} else {
	    genorig = (float *)DXAllocate(sizeof(float) * (shape+1));
	    gendelta = (float *)DXAllocate(sizeof(float) * (shape+1));
	    if (!genorig || !gendelta)
		goto error;
	    
	    if (!DXGetRegularArrayInfo((RegularArray)oldpos, &items, 
				       (Pointer)genorig, (Pointer)gendelta))
		goto error;
	}
	
	for (i=shape; i > newdim; --i) {
	    genorig[i] = genorig[i-1];
	    gendelta[i] = gendelta[i-1];
	}
	
	genorig[newdim] = 0.0;
	gendelta[newdim] = 0.0;
	
	if (!(newterm = (Array)DXNewRegularArray(TYPE_FLOAT, shape+1, items,
					(Pointer)genorig, (Pointer)gendelta)))
	    goto error;
	
	/* if the old pos were only a reg array, the old conn had to be 1d.
         * the only reasonable options for stackdim here are 0 or 1.  
	 */
	switch (newdim) {
	  case 0:
	    terms[0] = addpos;
	    terms[1] = newterm;
	    break;
	  case 1:
	    terms[0] = newterm;
	    terms[1] = addpos;
	    break;
	  default:
	    INTERNALERROR;
	    goto error;
	}

        if (!(newpos = (Array)DXNewProductArrayV(2, terms)))
	    goto error;
	
	break;
	
      case CLASS_PRODUCTARRAY:

	if (!DXGetProductArrayInfo((ProductArray)oldpos, &tcount, NULL))
	    goto error;

	if (tcount+1 < MAXRANK) {
	    if (!DXGetProductArrayInfo((ProductArray)oldpos, &tcount, terms))
		goto error;
	    genterms = terms;
	} else {
	    genterms = (Array *)DXAllocate(sizeof(Array *) * (tcount+1));
	    if (!genterms)
		goto error;
	    if (!DXGetProductArrayInfo((ProductArray)oldpos, &tcount, genterms))
		goto error;
	}

	for (i=0; i<tcount; i++)
	    genterms[i] = add1posdim(genterms[i], newdim, 0.0);

	for (i=tcount; i>newdim; i--)
	    genterms[i] = genterms[i-1];

	genterms[newdim] = addpos;
        
        if (!(newpos = (Array)DXNewProductArrayV(tcount+1, terms)))
	    goto error;

	break;


      /* degenerate, but perhaps useful after being computed on? */
      case CLASS_CONSTANTARRAY:

	/* can only do this if the addpos array matches exactly.
         *
	 * NEED CODE HERE !!! XXX
	 */

	if (shape+1 < MAXRANK) {
	    if (!DXQueryConstantArray(oldpos, &items, (Pointer)orig))
		goto error;
	    genorig = orig;
	} else {
	    genorig = (float *)DXAllocate(sizeof(float) * (shape+1));
	    if (!genorig)
		goto error;
	    if (!DXQueryConstantArray(oldpos, &items, (Pointer)genorig))
		goto error;
	}

	for (i=shape; i > newdim; --i)
	    genorig[i] = genorig[i-1];
	
	genorig[newdim] = 1.0;
	    
	if (!(newpos = (Array)DXNewConstantArray(items, (Pointer)genorig, 
						 TYPE_FLOAT, CATEGORY_REAL,
						 1, shape+1)))
	    goto error;

	break;

      /* these are integer types - can't be used as the pos component */
      case CLASS_MESHARRAY:
      case CLASS_PATHARRAY:
      default:
	DXSetError(ERROR_BAD_CLASS, "Unrecognized array type in positions");
	goto error;
    }



  done:

    FREEPTR(gencounts, counts);
    FREEPTR(genorig, orig);
    FREEPTR(gendelta, delta);
    FREEPTR(genrorig, rorig);
    FREEPTR(genrdelta, rdelta);
    FREEPTR(genterms, terms);
    DXDelete((Object)tmppos);
    return newpos;
    
  error: 
    FREEPTR(gencounts, counts);
    FREEPTR(genorig, orig);
    FREEPTR(gendelta, delta);
    FREEPTR(genrorig, rorig);
    FREEPTR(genrdelta, rdelta);
    FREEPTR(genterms, terms);
    DXDelete((Object)tmppos);
    DXDelete((Object)newpos);
    return NULL;

}


static Array 
addpositions(StackParms *sp, StackInfo *si)
{
    Array newseriesdim = NULL;
    Array newpos = NULL;
    int newposreg = 0;
    int allserreg = 0;

    /* decide if posD is already > conD - if so, then just
     * concatinate the pos together like other data arrays
     * and return.  that lets 2d lines become 2d quads (normal)
     * and 3d lines becomes 3d quads (ok as well).  don't add 
     * another dim to the positions in this case.
     */
    
    if (si->posdim > si->conndim)
	return addarrays(sp, si, "positions", 0);
    
    /* there is a 2x2 decision matrix here.  the new series axis can
     * be regular or not.  the existing series position members can
     * be regular and consistent or not.  take advantage of any regularity
     * you can.  newposreg is the flag for former, allserreg for latter.
     */

    if (!newdimregular(sp, si, &newseriesdim, &newposreg))
	goto error;
    
    
    /* decide if the positions in the series are regular or not.
     * if regular, be sure they all have identical origin/count/deltas.
     */
    if (!allposregular(si, &allserreg))
	goto error;


    /* ok based on the flags, decide how to construct the new positions
     * component.
     */
    newpos = addNposdim(si, newposreg, allserreg, sp->stackdim, newseriesdim);
    if (!newpos)
	goto error;


    return newpos;

  error:
    return NULL;
}

static Array 
addcondim(StackInfo *si, Array oldcon, int newdim, int newcount)
{
    Array newcon = NULL;
    int shape;
    int i;
    int counts[MAXRANK], *gencounts = NULL;

    /* originally scattered pts?  connect into lines */
    if (!oldcon) {
	newcon = DXMakeGridConnections(1, newcount);
	if (!newcon)
	    goto error;

	si->countlist = (int *)DXAllocate(sizeof(int));
	if (!si->countlist)
	    goto error;

	si->countlist[0] = newcount;
	goto done;
    }
    

    /* normal case - get old connections array and up it by 1D */
    if (!DXQueryGridConnections(oldcon, &shape, NULL))
	goto error;

    if (shape+1 < MAXRANK) {
	if (!DXQueryGridConnections(oldcon, NULL, counts))
	    goto error;
	gencounts = counts;
	
    } else {
	gencounts = (int *)DXAllocate(sizeof(int) * (shape+1));
	if (!gencounts)
	    goto error;

	if (!DXQueryGridConnections(oldcon, NULL, gencounts))
	    goto error;
    }

    for (i=shape; i>newdim; --i)
	gencounts[i] = gencounts[i-1];

    gencounts[i] = newcount;


    newcon = DXMakeGridConnectionsV(shape+1, gencounts);
    if (!newcon)
	goto error;

    si->countlist = (int *)DXAllocate(sizeof(int) * (shape+1));
    if (!si->countlist)
	goto error;
    
    for (i=0; i<shape+1; i++)
	si->countlist[i] = gencounts[i];
    
  done:
    FREEPTR(gencounts, counts);
    return newcon;
    
  error: 
    FREEPTR(gencounts, counts);

    DXDelete((Object)newcon);
    return NULL;
}

/* try to keep some things compact - constant & reg arrays w/0 deltas.
 */
static Array 
addarrays(StackParms *sp, StackInfo *si, char *component, int flags)
{
    Array newarr;

    /* if array is constant or regular w/ 0 deltas, try to keep
     * it compact.
     */
    if ((newarr=addcnstarrays(sp, si, component)))
	return newarr;
    
    /* else fall into irreg case */
    if (ISSET(flags, ISREF))
	return addrefarrays(si, component, sp->stackdim);

    return addirregarrays(si, component, sp->stackdim);
}

static Array 
addcnstarrays(StackParms *sp, StackInfo *si, char *component)
{
    Array newarr = NULL;
    Array a;
    int j;
    Object o;
    Type t0;
    Category c0;
    int rank0;
    int shape0[MAXRANK];
    int size, items;
    Pointer *dvalue0 = NULL;
    Pointer *dvalue1 = NULL;


    /* for each member of the series, extract the array and
     * see if it's a constant value across the entire series.
     */
    for (j=0; (o=DXGetSeriesMember((Series)si->thisobj, j, NULL)); j++) {
	
	a = (Array)DXGetComponentValue((Field)o, component);
	
	if ((DXGetArrayClass(a) != CLASS_CONSTANTARRAY) &&
	    (DXGetArrayClass(a) != CLASS_REGULARARRAY))
	    return NULL;
	
    }

    /* ok, might be worthwhile to try */

    a = (Array)DXGetComponentValue(si->f0, component);
    if (DXGetArrayClass(a) == CLASS_CONSTANTARRAY) {
	
	if (!DXGetArrayInfo(a, &items, &t0, &c0, &rank0, NULL))
	    return NULL;
	
	if (rank0 >= MAXRANK)
	    return NULL;   /* bail here */
	
	if (!DXGetArrayInfo(a, NULL, NULL, NULL, &rank0, shape0))
	    return NULL;
	
	size = DXGetItemSize(a);
	
	dvalue0 = (Pointer)DXAllocate(size);
	dvalue1 = (Pointer)DXAllocate(size);
	if (!dvalue0 || !dvalue1)
	    return NULL;
	
	if (!DXQueryConstantArray(a, NULL, (Pointer)dvalue0))
	    goto error;
	
	for (j=1; (o=DXGetSeriesMember((Series)si->thisobj, j, NULL)); j++) {
	    
	    a = (Array)DXGetComponentValue((Field)o, component);
	    
	    if (DXGetArrayClass(a) != CLASS_CONSTANTARRAY)
		goto error;
	    
	    if (!DXQueryConstantArray(a, NULL, (Pointer)dvalue1))
		goto error;
	    
	    if (memcmp(dvalue0, dvalue1, size) != 0)
		goto error;
	}
	
	/* bingo. (success) */
	newarr = (Array)DXNewConstantArrayV(items * si->membercnt, 
					    (Pointer)dvalue0,
					    t0, c0, rank0, shape0);
	if (!newarr)
	    goto error;

	goto done;
    }


    if (DXGetArrayClass(a) == CLASS_REGULARARRAY) {
	/* see if the deltas are 0, and if the values match. */
	
	/* less likely of happening, but some old code might still
	 * be using reg arrays instead of constants
	 */
	
	/* NEED CODE HERE, but ok for now w/o it */
	return NULL;
    }
    
    
  done:
    return newarr;
    
  error:
    DXFree(dvalue0);
    DXFree(dvalue1);
    DXDelete((Object)newarr);
    return NULL;
}

/* this component refs another. call addirregarrays first to
 * make it an expanded array, and then set up the same size
 * array to see where the numbers go.  then renumber the array.
 * this probably needs cases for constant arrays (and a test in
 * addcnstarrays() because the code doesn't get here in that case)
 * and regular arrays.  for now just assume it's irregular.
 */
static Array 
addrefarrays(StackInfo *si, char *component, int stackdim)
{
    Array newarr;
    Array renumber = NULL;

    /* have to renumber the original values first - add
     * offsets to each stack so all the numbers are unique.
     * !! need to know if original target was stacked!!  if
     * not (no dep attrs) then we don't have to renumber anything!!
     */
    newarr = addirregarrays(si, component, stackdim);
    if (!newarr)
	goto error;

    renumber = makerefs(si, stackdim);
    if (!renumber)
	goto error;

    if (!dorenumber(newarr, renumber))
	goto error;

    DXDelete((Object)renumber);
    return newarr;

  error:
    DXDelete((Object)newarr);
    DXDelete((Object)renumber);
    return NULL;
}


static Array 
makerefs(StackInfo *si, int stackdim)
{
    int i, j;
    int items, totalcnt;
    int *ip;
    Series news = NULL;
    Field newf = NULL;
    Array newa = NULL;
    Array retval = NULL;

    totalcnt = 0;

    items = 1;
    for (i=0; i<si->conndim; i++)
	items *= si->countlist[i];

    news = DXNewSeries();
    if (!news)
	goto error;

    for (i=0; i<si->membercnt; i++) {
	newf = DXNewField();
	if (!newf)
	    goto error;

	newa = DXNewArray(TYPE_INT, CATEGORY_REAL, 0);
	if (!newa)
	    goto error;

	if (!DXAddArrayData(newa, 0, items, NULL))
	    goto error;

	ip = (int *)DXGetArrayData(newa);
	if (!ip)
	    goto error;

	for (j = 0; j < items; j++)
	    *ip++ = totalcnt++;

	if (!DXSetComponentValue(newf, "numbers", (Object)newa))
	    goto error;
	newa = NULL;

	if (!DXSetSeriesMember(news, i, (double)i, (Object)newf))
	    goto error;
	news = NULL;
    }

    if (!(retval = addirregarrays(si, "numbers", stackdim)))
	goto error;

    DXDelete((Object)news);
    return retval;

  error:
    DXDelete((Object)news);
    DXDelete((Object)newf);
    DXDelete((Object)newa);
    DXDelete((Object)retval);
    return NULL;

}
    
/* renumber the array in place - we just created it
 * so it's not a read only input.
 */
static Error 
dorenumber(Array newarr, Array renumber)
{
    int i;
    int items;
    int *ip, *rip;
    int *mip = NULL;

    /* ref arrays should be able to be ubyte or int.  for now
     * force them to be int.
     */
    if (!DXTypeCheckV(newarr, TYPE_INT, CATEGORY_REAL, 0, NULL)) {
	DXSetError(ERROR_DATA_INVALID, "ref arrays must be type integer");
	return ERROR;
    }

    if (!DXGetArrayInfo(newarr, &items, NULL, NULL, NULL, NULL))
	goto error;

    ip = (int *)DXGetArrayData(newarr);
    rip = (int *)DXGetArrayData(renumber);
    mip = (int *)DXAllocate(sizeof(int) * items);
    if (!ip || !rip || !mip)
	goto error;

    for(i=0; i<items; i++)
	mip[i] = -1;

    for(i=0; i<items; i++)
	mip[rip[i]] = i;

    for(i=0; i<items; i++)
	ip[i] = mip[ip[i]];

    DXFree((Pointer)mip);
    return OK;

  error:
    DXFree((Pointer)mip);
    return ERROR;
}

/* this irregularizes everything.  we should at least look 
 * for constant arrays or regular arrays w/ deltas == 0. 
 * colors are often like this.  (constant arrays now handled;
 * reg w/ delta=0 is NOT yet.)
 * if component == null, input is series of arrays.  don't look
 * up the componentt, work directly on the series member.
 */
static Array 
addirregarrays(StackInfo *si, char *component, int stackdim)
{
    int i, j, k;
    Object o;
    Type t;
    Category c;
    int rank, shape[MAXRANK];
    Array a, na = NULL;
    int ncounts[MAXRANK];
    int dims[MAXRANK], temp[MAXRANK];
    int nstride[MAXRANK], ostride[MAXRANK];
    int nitems, itemsize;
    Pointer np, op;

    /* for each member of the series, extract the array and
     * add it into one array.
     */
    
    if (component) {
	a = (Array)DXGetComponentValue(si->f0, component);
	
	a = foolInvalids(a, component, si->f0, 0);
	if (!a)
	    return NULL;
    } else
	a = (Array)DXGetSeriesMember((Series)si->thisobj, 0, NULL);

    if (!DXGetArrayInfo(a, &nitems, &t, &c, &rank, shape))
	return NULL;
    
    na = DXNewArrayV(t, c, rank, shape);
    if (!na)
	goto error;
    if (!DXCopyAttributes((Object)na, (Object)a))
	goto error;
    np = DXGetArrayData(DXAddArrayData(na, 0, nitems*si->membercnt, NULL));
    if (!np)
	goto error;
    
    itemsize = DXGetItemSize(a);

    for (i=0; i<si->conndim+1; i++)
	dims[i] = i;

    /* old strides - w/o stacked dim*/
    k = 1;
    for (i=si->conndim; i>stackdim; i--) {
	temp[i] = k;
	k *= si->countlist[i];
    }
    temp[stackdim] = 0;
    for (i=stackdim-1; i>=0; i--) {
	temp[i] = k;
	k *= si->countlist[i];
    }
    
    /* rearrange dims */
    for (i=si->conndim; i>=0; i--) {
	ostride[i] = temp[dims[i]];	
	ncounts[i] = si->countlist[dims[i]];
    }
    
    /* new strides - w/ stacked dim */
    k = 1;
    for (i=si->conndim; i>=0; i--) {
	nstride[i] = k;
	k *= ncounts[i];
    }

    ncounts[stackdim] = 1;
 
    
    for (j=0; (o=DXGetSeriesMember((Series)si->thisobj, j, NULL)); j++) {

	if (component) {
	    a = (Array)DXGetComponentValue((Field)o, component);
	    a = foolInvalids(a, component, (Field)o, j);
	    if (!a)
		goto error;
	} else
	    a = (Array)o;

	op = DXGetArrayData(a);
	np = (char *)DXGetArrayData(na) + j*nstride[stackdim]*itemsize;

	_dxfPermute(si->conndim+1, np, nstride, ncounts, itemsize, op, ostride);
	
    }

    return na;
    
  error:
    DXDelete((Object)na);
    return NULL;
}



/* take a positions component with all the previous positions
 * stacked into one array, plus the new series array (both irregular)
 * and permute them into a single array.
 */
static Array 
addseriesdim(StackInfo *si, Array oldpos, Array addpos, int stackdim)
{
    int i, j, k;
    int reps, skips;
    Type t;
    Category c;
    int rank, shape[MAXRANK];
    Array newpos = NULL;
    int nitems;
    float *ofp, *nfp;

    /* oldpos is the old positions, combined into a single array
     * but without the new position.  addpos is the new positions
     * to add.  use permute to get them into the right order.
     */
    
    
    if (!DXGetArrayInfo(oldpos, &nitems, &t, &c, &rank, shape))
	return NULL;
    ofp = (float *)DXGetArrayData(oldpos);
    if (!ofp)
	goto error;
    
    if (rank == 0) {
	rank = 1;
	shape[0] = 1;
    }
    shape[0] += 1;
    newpos = DXNewArrayV(t, c, rank, shape);
    if (!newpos)
	goto error;
    if (!DXCopyAttributes((Object)newpos, (Object)addpos))
	goto error;
    nfp = (float *)DXGetArrayData(DXAddArrayData(newpos, 0, nitems, NULL));
    if (!nfp)
	goto error;
    
    /* make space for new dim */
    for (i=0; i<nitems; i++) 
	for (j=0; j<shape[0]; j++) {
	    if (j == stackdim)
		*nfp++ = 0.0;
	    else
		*nfp++ = *ofp++;
	}
    

    /* now permute the new array into the slot left in the pos array.
     */

    /* compute offset to first and skip between items.
     *  if stackdim = maxdim, skip = 1, offset = skip*j;  varies fastest 
     *  if stackdim = max-1, skip = countlist[maxdim], offset = skip*j
     *  if stackdim = max-2, skip = countlist[max*max-1], etc 
     */
    /* reps & skips */
    reps = 1;
    skips = nitems/si->membercnt;
    for (i=si->conndim; i>stackdim; --i) {
	reps *= si->countlist[i];
	skips /= si->countlist[i];
    }
    
    nfp = (float *)DXGetArrayData(newpos);
    ofp = (float *)DXGetArrayData(addpos);
    if (!nfp || !ofp)
	goto error;
    
    
    /* outer loop - once per data item */
    for (i=0; i<si->membercnt; i++) {
	nfp = (float *)DXGetArrayData(newpos);
	nfp += (reps * i) * shape[0];
	for (j=0; j<skips; j++) {
	    for (k=0; k<reps; k++) {
		nfp[k*shape[0] + stackdim] = ofp[i*shape[0] + stackdim];
	    }
	    nfp += shape[0] * si->membercnt * reps;
	}
    }
    
   return newpos;
    
  error:
    DXDelete((Object)newpos);
    return NULL;
}


/*
 * check the array - to stack invalids i have to have a real instantiated
 * array even if there are no invalids in one or more of the individual
 * series members.  so if there isn't an invalid array here
 * create one filled with 0s (all valid).  if there is one but it's ref,
 * create one which is dep.  if this array has nothing to do with invalids,
 * just return the original incoming array.
 */
static Array 
foolInvalids(Array a, char *name, Field f, int seriesmember)
{
    int i;
    Type t;
    Category c;
    int rank, shape[MAXRANK];
    Array na;
    int nitems;
    int *ip;
    Pointer np;
    char *attr;

    if (!a) {
        if (strncmp(name, "invalid ", strlen("invalid ")) != 0) {
            DXSetError(ERROR_DATA_INVALID, 
                       "series member %d does not contain component `%s'", 
                       seriesmember, name);
            return NULL;
        }
        
        /* prepare an invalid array filled with all valid values.  
         */
        a = (Array)DXGetComponentValue(f, name+strlen("invalid "));
        if (!a) {
            DXSetError(ERROR_DATA_INVALID, 
                       "series member %d does not contain component `%s'", 
                       seriesmember, name+strlen("invalid "));
            return NULL;
        }
        if (!DXGetArrayInfo(a, &nitems, &t, &c, &rank, shape))
            return NULL;
        
        a = DXNewArray(TYPE_UBYTE, CATEGORY_REAL, 0);
        if (!a)
            return NULL;
        
        if (!DXAddArrayData(a, 0, nitems, NULL)) {
            DXDelete((Object)a);
            return NULL;
        }
        
        np = DXGetArrayData(a);
        if (!np) {
            DXDelete((Object)a);
            return NULL;
        }
        
        memset(np, '\0', nitems);
        
        if (!DXSetStringAttribute((Object)a, "dep", "positions")) {
            DXDelete((Object)a);
            return NULL;
        }

        /* and now continue like there was an invalid array there 
         * from the beginning... 
         */
        
        return a;
    } 
    
    if (strncmp(name, "invalid ", strlen("invalid ")) == 0) {
        
        if (DXGetStringAttribute((Object)a, "ref", &attr) != NULL) {
            
            /* if the `name' array did already exist, and it's a ref list and
             * not a dep list, then we have to expand it into a dep list.
             */
            
            /* prepare an invalid array filled with all valid values.  
             */
            na = (Array)DXGetComponentValue(f, name+strlen("invalid "));
            if (!na) {
                DXSetError(ERROR_DATA_INVALID, 
                           "series member %d does not contain component `%s'", 
                           seriesmember, name+strlen("invalid "));
                return NULL;
            }
            if (!DXGetArrayInfo(na, &nitems, &t, &c, &rank, shape))
                return NULL;
            
            na = DXNewArray(TYPE_UBYTE, CATEGORY_REAL, 0);
            if (!na)
                return NULL;
            
            if (!DXAddArrayData(na, 0, nitems, NULL)) {
                DXDelete((Object)na);
                return NULL;
            }
            
            np = DXGetArrayData(na);
            if (!np) {
                DXDelete((Object)na);
                return NULL;
            }
            
            memset(np, '\0', nitems);
            
            /* and now go thru the ref list and fill in the invalids.
             * get the original count of the ref list
             */
            if (!DXGetArrayInfo(a, &nitems, &t, &c, &rank, shape))
                return NULL;
            
            ip = (int *)DXGetArrayData(a);
            
            for (i=0; i<nitems; i++)
                ((ubyte *)np)[ip[i]] = (ubyte)1;
            
            /* now pretend this new array is what we originally had */
            a = na;

            if (!DXSetStringAttribute((Object)a, "dep", "positions")) {
                DXDelete((Object)a);
                return NULL;
            }

        }
        
    }
    
    /* either the original, or the newly constructed one */
    return a;
}



/* decide if the series position is regular.  if not, return ok
 * but leave newpos set to NULL.  if a real error occurred, return
 * error and the value of newpos is ignored.
 */
static Error 
seriesregular(StackParms *sp, StackInfo *si, Array *newpos)
{
    int i;
    float s0, sN, sL;
    double del0, delN, nmax;
    float orig[MAXRANK], *genorig = NULL;
    float delta[MAXRANK], *gendelta = NULL;
    
    if (si->inclass != CLASS_SERIES)
	return OK;

    if (!DXGetSeriesMember((Series)si->thisobj, 0, &s0))
	goto error;
    
    if (!DXGetSeriesMember((Series)si->thisobj, 1, &sN))
	goto error;
    
    del0 = sN - s0;
    if (fabs(del0) < DXD_MIN_FLOAT) {
	DXSetError(ERROR_DATA_INVALID, "#11075");
	goto error;
    }
    
    sL = sN;
    for (i=2; DXGetSeriesMember((Series)si->thisobj, i, &sN); i++) {
	delN = sN - sL;
	
	/* fuzzy compare to see if del == ndel */
	nmax = nonzeromax(sN, s0);
	if (fabs(nmax) > DXD_MIN_FLOAT && fabs((delN-del0)/nmax) < NORMFUZZ)
	    ; /* close enough to be equal */
	else
	    goto done;   /* no error, just not regularly spaced */
	
	if ((del0 > 0 && delN < 0) || (del0 < 0 && delN > 0) || (delN == 0.0)) {
	    DXSetError(ERROR_DATA_INVALID, "#11075");
	    goto error;
	}

	sL = sN;
    }
	
    /* if the series spacing is regular, construct an N+1 D
     * regular array to combine with the other part of the positions.
     */
    if (si->posdim+1 < MAXRANK) {
	genorig = orig;
	gendelta = delta;
    } else {
	genorig = (float *)DXAllocate(sizeof(float) * (si->posdim+1));
	gendelta = (float *)DXAllocate(sizeof(float) * (si->posdim+1));
	if (!genorig || !gendelta)
	    goto error;
    }

    for (i=0; i<si->posdim+1; i++) {
	if (i == sp->stackdim) {
	    genorig[i] = s0;
	    gendelta[i] = del0;
	} else {
	    genorig[i] = 0.0;
	    gendelta[i] = 0.0;
	}
    }
	
    *newpos = (Array)DXNewRegularArray(TYPE_FLOAT, si->posdim+1, si->membercnt,
				      genorig, gendelta);
    if (!*newpos)
	goto error;
	 
	 
  done:
    FREEPTR(genorig, orig);
    FREEPTR(gendelta, delta);
    return OK;

  error:
    FREEPTR(genorig, orig);
    FREEPTR(gendelta, delta);
    DXDelete((Object)*newpos);
    *newpos = NULL;
    return ERROR;
}



/* decide if the series spacing is regular.  if so, construct a
 * regular grid of the proper dim.  seriesregular() returns error only
 * for a real error.  if the series is regular, it returns a reg array
 * in newseriesdim.  if not, it returns OK but null for the new array
 * and you have to call seriesspacing() to construct an expanded array.
 */
static Error 
newdimregular(StackParms *sp, StackInfo *si, Array *newarr, int *regflag)
{

    if (!seriesregular(sp, si, newarr))
	goto error;
    
    if (*newarr)
	(*regflag)++;
    else {
	*newarr = seriesspacing(sp, si);
	if (! *newarr)
	    goto error;
    }

    return OK;

  error:
    return ERROR;
}

/* this really isn't the question.  the question is: is the
 * positions component on the all series members identical,
 * whatever it is?  if so, then we can use the first one as
 * a template and make a product array with the other dim.
 * if it's not, we have to expand it completely no matter
 * what the original form was.
 */
static Error 
allposregular(StackInfo *si, int *regflag)
{
    Field subo;
    Array pos;
    int i, j;
    int items;
    int shape0, shape;
    int counts0[MAXRANK], *gencounts0 = NULL;
    float orig0[MAXRANK], *genorig0 = NULL;
    float delta0[MAXRANK*MAXRANK], *gendelta0 = NULL;
    int counts[MAXRANK], *gencounts = NULL;
    float orig[MAXRANK], *genorig = NULL;
    float delta[MAXRANK*MAXRANK], *gendelta = NULL;


    /* look at first member as a reference */
    if (!DXQueryGridPositions(si->ap0, &shape0, NULL, NULL, NULL)) {
	*regflag = 0;
	goto done;
    }
    
    if (shape0 < MAXRANK) {
	if (!DXQueryGridPositions(si->ap0, NULL, counts0, orig0, delta0))
	    goto error;
	gencounts0 = counts0;
	genorig0 = orig0;
	gendelta0 = delta0;
	gencounts = counts;
	genorig = orig;
	gendelta = delta;
    } else {
	items = sizeof(float) * shape0;
	genorig0 = (float *)DXAllocate(items);
	gencounts0 = (int *)DXAllocate(items);
	genorig = (float *)DXAllocate(items);
	gencounts = (int *)DXAllocate(items);
	
	items *= shape0;
	gendelta0 = (float *)DXAllocate(items);
	gendelta = (float *)DXAllocate(items);
	if (!gencounts0 || !genorig0 || !gendelta0 || !gencounts || !genorig || !gendelta)
	    goto error;
	
	if (!DXQueryGridPositions(si->ap0, NULL, gencounts0, genorig0, gendelta0))
	    goto error;
    }



    for (i=1; (subo=(Field)DXGetSeriesMember((Series)si->thisobj, i, NULL)); i++) {
	if (DXGetObjectClass((Object)subo) != CLASS_FIELD)
	    goto error;

	pos = (Array)DXGetComponentValue(subo, "positions");
	if (!pos)
	    goto error;

	if (!DXQueryGridPositions(pos, &shape, NULL, NULL, NULL)) {
	    *regflag = 0;
	    goto done;
	}

	if (shape != shape0) {
	    *regflag = 0;
	    goto done;
	}
    
	if (!DXQueryGridPositions(pos, NULL, gencounts, genorig, gendelta))
	    goto error;
	
	for (j=0; j<shape; j++) {
	    if (gencounts[j] != gencounts0[j]) {
		*regflag = 0;
		goto done;
	    }
	    if (genorig[j] != genorig0[j]) {
		*regflag = 0;
		goto done;
	    }
	    if (gendelta[j] != gendelta0[j]) {
		*regflag = 0;
		goto done;
	    }
	}
	
	
    }

    *regflag = 1;

  done:
    FREEPTR(gencounts0, counts0);
    FREEPTR(genorig0, orig0);
    FREEPTR(gendelta0, delta0);
    FREEPTR(gencounts, counts);
    FREEPTR(genorig, orig);
    FREEPTR(gendelta, delta);
    return OK;

  error:
    FREEPTR(gencounts0, counts0);
    FREEPTR(genorig0, orig0);
    FREEPTR(gendelta0, delta0);
    FREEPTR(gencounts, counts);
    FREEPTR(genorig, orig);
    FREEPTR(gendelta, delta);
    return ERROR;

}

static Array 
seriesspacing(StackParms *sp, StackInfo *si)
{
    int i;
    int shape, index;
    float *fp = NULL;
    float pos;
    Array newarr = NULL;


    /* if the series positions are NOT regular, construct an
     * N+1 D generic positions array, which will either be used
     * as a product term, or combined into an expanded array
     * depending on the rest of the positions.
     */

    shape = si->posdim + 1;
    index = sp->stackdim;

    newarr = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, shape);
    if (!newarr)
	goto error;

    if (!DXAddArrayData(newarr, 0, si->membercnt, NULL))
	goto error;

    fp = (float *)DXGetArrayData(newarr);
    if (!fp)
	goto error;

    memset((Pointer)fp, '\0', sizeof(float) * shape * si->membercnt);



    for (i=0; DXGetSeriesMember((Series)si->thisobj, i, &pos); i++)
	fp[shape*i + index] = pos;

    
    return newarr;

  error:
    return NULL;
}


static Object 
stackgroup(StackParms *sp, StackInfo *si)
{
    Field retval = NULL;
    Object subo;
    float seriespos;
    Class class;
    Array newpos = NULL;
    Array newcon = NULL;
    Array newarr = NULL;
    int i;


    /* if series only has 1 member and it is a field, stack it alone */
    if (si->membercnt == 1) {
	if (si->inclass == CLASS_SERIES) {
	    if (!(subo=DXGetSeriesMember((Series)si->thisobj, 0, &seriespos)))
		goto error;
	    
	    class = DXGetObjectClass(subo);
	    if (class == CLASS_FIELD) {
		
		si->thisobj = subo;
		si->inclass = class;
		
		retval = (Field)stackpos(sp, si, seriespos);
		if (!retval)
		    goto error;
		
		goto done;
	    }
	} else {
	    if (!(subo = DXGetEnumeratedMember((Group)si->thisobj, 0, NULL)))
		goto error;
	    
	    class = DXGetObjectClass(subo);
	    if (class == CLASS_FIELD) {
		si->thisobj = subo;
		si->inclass = class;
		
		retval = (Field)stackpos(sp, si, 0.0);
		if (!retval)
		    goto error;
		
		goto done;
	    }
	}
    }


    /* return field */
    retval = DXNewField();
    if (!retval)
	goto error;

    
    /* if the members of the series are arrays, stack them into lines */
    if (ISSET(si->stackflags, POINTS)) {
	newcon = DXMakeGridConnections(1, si->membercnt);
	if (!newcon)
	    goto error;

	if (!DXSetComponentValue(retval, "connections", (Object)newcon))
	    goto error;
	newcon = NULL;
	
	newpos = addirregarrays(si, NULL, sp->stackdim);
	if (!newpos)
	    goto error;

	if (!DXSetComponentValue(retval, "positions", (Object)newpos))
	    goto error;
	newpos = NULL;

	if (!DXEndField(retval))
	    goto error;

	goto done;
    }
    
    /* "normal" case of a series of fields */

    /* stack the connections */
    newcon = addcondim(si, si->ac0, sp->stackdim, si->membercnt);
    if (!newcon)
	goto error;

    if (!DXSetComponentValue(retval, "connections", (Object)newcon))
	goto error;
    newcon = NULL;


    /* stack the positions */
    newpos = addpositions(sp, si);
    if (!newpos)
	goto error;

    if (!DXSetComponentValue(retval, "positions", (Object)newpos))
	goto error;
    newpos = NULL;

    /* stack each of the members */
    for (i=0; i<si->compcount; i++) {
	if (ISSET(si->compflags[i], SKIP))
	    continue;

	if (strcmp("connections", si->compname[i]) == 0)
	    continue;
	if (strcmp("positions", si->compname[i]) == 0)
	    continue;

	if (ISSET(si->compflags[i], DOFIRST)) {
	    /* loop thru members looking for a field w/ component? */
	    newarr = (Array)DXGetComponentValue(si->f0, si->compname[i]);
	    if (!newarr)
		continue;
	    SET(si->compflags[i], SKIP);

	} else {

	    newarr = addarrays(sp, si, si->compname[i], si->compflags[i]);
	    if (!newarr)
		goto error;
	}

	if (!DXSetComponentValue(retval, si->compname[i], (Object)newarr))
	    goto error;
	newarr = NULL;

    }

    if (ISSET(si->stackflags, INVALIDS) && !setinvalids(si, retval))
	goto error;

    if (!DXEndField(retval))
	goto error;

  done:
    return (Object)retval;

  error:
    DXDelete((Object)retval);
    DXDelete((Object)newpos);
    DXDelete((Object)newcon);
    DXDelete((Object)newarr);
    return NULL;
}

static Field setinvalids(StackInfo *si, Field f)
{
    InvalidComponentHandle ich;

    if (ISSET(si->stackflags, INVPOS))
	ich = DXCreateInvalidComponentHandle((Object)f, 
					     NULL, "invalid positions");
    else
	ich = DXCreateInvalidComponentHandle((Object)f, 
					     NULL, "invalid connections");

    if (!ich)
	return NULL;

    if (!DXSaveInvalidComponent(f, ich))
	return NULL;

    if (!DXFreeInvalidComponentHandle(ich))
	return NULL;

    return f;
}

static void
freespace(StackInfo *si)
{
    int i;

    if (!si)
	return;
    
    if (si->countlist) {
	DXFree((Pointer)si->countlist);
	si->countlist = NULL;
    }
    if (si->compflags) {
	DXFree((Pointer)si->compflags);
	si->compflags = NULL;
    }
    if (si->compname) {
	for (i=0; i<si->compcount; i++) 
	    DXFree((Pointer)si->compname[i]);
	DXFree((Pointer)si->compname);
	si->compname = NULL;
    }
    si->compcount = 0;
}


/* return the max of a and b unless the max is zero, then
 * return the other value.
 */
static double 
nonzeromax(double a, double b)
{
    if (a == 0.0 && b != 0.0)
	return b;
    if (b == 0.0 && a != 0.0)
	return a;
    
    if (a != 0.0 && b != 0.0)
	return MAX(a, b);

    return NORMFUZZ;
}


#if 0
static int 
isZero(Type t, Pointer value)
{
    switch (t) {
      case TYPE_BYTE:   return (byte)0     == *(byte *)value;
      case TYPE_UBYTE:  return (ubyte)0    == *(ubyte *)value;
      case TYPE_SHORT:  return (short)0    == *(short *)value;
      case TYPE_USHORT: return (ushort)0   == *(ushort *)value;
      case TYPE_INT:    return (int)0      == *(int *)value;
      case TYPE_UINT:   return (uint)0     == *(uint *)value;
      case TYPE_FLOAT:  return (float)0.0  == *(float *)value;
      case TYPE_DOUBLE: return (double)0.0 == *(double *)value;
      default:          return 0;
    }	

    /* not reached */
}
#endif

