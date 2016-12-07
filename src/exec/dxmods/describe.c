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

#include <ctype.h>
#include <dx/dx.h>

#define Return_Internal_Error \
    DXSetError(ERROR_INTERNAL, "at file %s, line %d",  __FILE__, __LINE__); \
    return ERROR


/*******

the general idea:

1) what is this object?
     if a group, list some of the member names ( < 10? )
     if a series, list some of the series pos (")
     if a list, list some of the values (")

2) if this is a potentially renderable thing (field, group, etc)
     what is the bbox?
     what is the data max/min?
     how many data values, what kind?
     what kind of connections are there?  Ndim?
     *** what kind of colors (float/byte, full list/delayed) ***

3) is it renderable at this point?
     if not, why not?
     if so, what is going to be rendered?


*******/
   



/* reasons why an object might not be renderable.
 */
struct renderinfo {
    int nfields;        /* need to be more than one to be renderable */
    int nlights;        /* if > 1, default lighting disabled */
    int badobjs;        /* if > 1, will get error from render */
    Class *badclass;    /* list of bad obj types */
    int badpos;         /* pos <= 0 or > 3d, error */
    int nocolors;       /* if missing colors, error from render */
};
struct nopicture {
    int nodep;          /* no errors, but no picture either */
    int novalid;
    int nopos;          /* no positions component (empty field) */
};
struct nD {
    int c0;             /* types of connections in the input */
    int c1;
    int c2;
    int c3;             /* points, lines, surfaces, volumes */
    int cplus;          /* more than 3d connections */
    int cbad;           /* an unrecognized type or malformed component */
    int cmissing;       /* missing elem type attribute */
};
struct nColors {
    int cubyte;         /* types of color representations in the input */
    int cfloat; 
    int cbytemap;
    int cunknown;
};
			   

struct info {
    struct renderinfo ri;
    struct nopicture np;
    struct nD nd;
    struct nColors nc;
};


static Error saytype(Object o, char *opt);
static Error saydata(Object o, char *opt);
static Error sayrender(Object o, char *opt);

static Error sayclass(Object o, char *intro, int descend);
static Error saymembers(Group g);
static Error traverse(Object o, struct info *ip, int nosee);
static Error validfield(Field f, struct info *ip, int nosee);
#if 0
static int queryNDconnections(Field f, int n);
#endif
static Error validcount(Field f, char *component, int *icount);
#if 0
static Error atleast1field(Object o);
static Error atleast1image(Object o);
static Error atleast1contype(Object o, int n);
#endif
static int qimage(Object o);

#define MEMBERCOUNT  10    /* print info about this many members */

/* figure a way to add a constant string to the front of this */
#define DescribeMsg DXMessage
    


/* 0 = input
 * 1 = string flags: "structure", "details", "render", "all"
 *     (default "all")
 *
 * which mean:    tell me about the input structure
 *                tell me about the geometry (positions, connections, bbox)
 *                tell me about the renderability
 */
int
m_Describe(Object *in, Object *out)
{
    char *opt = "all";


    if (!in[0]) {
	DescribeMsg("Object Description:");
	DescribeMsg("No input object was specified, or the input object is NULL.");
	return OK;
    }
    
    if (in[1] && !DXExtractString(in[1], &opt)) {
	DXErrorReturn(ERROR_BAD_PARAMETER, "Options string must be one of 'structure', `details', `render', or `all'");
    }
    
    /* ???  i should lcase opt, but have to make a copy first. */
    if (strcmp(opt, "all") && strcmp(opt, "structure") && 
	strcmp(opt, "details") && strcmp(opt, "render")) {
	
	DXErrorReturn(ERROR_BAD_PARAMETER, "Options string must be one of 'structure', `details', `render', or `all'");
    }
    
    DXBeginLongMessage();

    if (!strcmp(opt, "all"))
	DescribeMsg("Object Description:\n");

    if (!strcmp(opt, "all") || !strcmp(opt, "structure")) {
	if (!strcmp(opt, "structure"))
	    DescribeMsg("Object Structure:\n");
	if (!saytype(in[0], opt))
	    goto done;
    }

    if (!strcmp(opt, "all") || !strcmp(opt, "details")) {
	if (!strcmp(opt, "details"))
	    DescribeMsg("Object Details:\n");
	if (!saydata(in[0], opt))
	    goto done;
    }

    if (!strcmp(opt, "all") || !strcmp(opt, "render")) {
	if (!strcmp(opt, "render"))
	    DescribeMsg("Object Renderability:\n");
	if (!sayrender(in[0], opt))
	    goto done;
    }

    DescribeMsg("\n");
    DXEndLongMessage();

  done:
    return (DXGetError()==ERROR_NONE ? OK : ERROR);
}


#define IS    "is a"
#define IS_V  "is an"


static Error saytype(Object o, char *opt)
{
    return sayclass(o, "Input object", 1);
}

static Error sayclass(Object o, char *intro, int descend)
{
    int count;

    switch (DXGetObjectClass(o)) {
	
      case CLASS_GROUP:
	if (!DXGetMemberCount((Group)o, &count))
	    return ERROR;
	
	switch(DXGetGroupClass((Group)o)) {
	    
	  case CLASS_GROUP:
	    DescribeMsg("%s %s Group which contains %d member%s.\n", 
		      intro, IS, count, (count==1 ? "" : "s"));
	    break;
	    
	  case CLASS_COMPOSITEFIELD:
	    DescribeMsg("%s %s Partitioned Field which contains %d partition%s.\n", 
		      intro, IS, count, (count==1 ? "" : "s"));
	    break;
	    
	  case CLASS_MULTIGRID:
	    DescribeMsg("%s %s Multigrid Group which contains %d member%s.\n", 
		      intro, IS, count, (count==1 ? "" : "s"));
	    break;
	    
	  case CLASS_SERIES:
	    DescribeMsg("%s %s Series Group which contains %d series member%s.\n", 
		      intro, IS, count, (count==1 ? "" : "s"));
	    DescribeMsg("To work with one member at a time use the `Select' module\n");

	    break;
	
	  default:
	    DescribeMsg("%s %s Group Object but of unrecognized Group Type.\n",
		      intro, IS);
	    break;
	}
	
	if (descend)
	    saymembers((Group)o);
	break;
	
      case CLASS_FIELD:
        DescribeMsg("%s %s Field, the basic data carrying structure in DX.\n", 
		  intro, IS);
        break;

      case CLASS_ARRAY:
	DescribeMsg("%s %s Array Object, containing a list of data values.\n",
		  intro, IS_V);

	switch (DXGetArrayClass((Array)o)) {
	    
	  case CLASS_ARRAY:
	    break;

	  case CLASS_PRODUCTARRAY:
	  case CLASS_MESHARRAY:
	  case CLASS_PATHARRAY:
	  case CLASS_CONSTANTARRAY:
	  case CLASS_REGULARARRAY:
	    DescribeMsg("These values are stored in a compact format to save space.\n");
	    break;
	
	  default:
	    DescribeMsg("The Array is of an unrecognized type.\n");
	    break;
	}

	/* ??? add code here ??? - print the first N values? */
	break;
	
      case CLASS_STRING:
	DescribeMsg("%s %s String, a list of character values\n",
		  intro, IS);
	DescribeMsg("It contains the value `%s'\n", DXGetString((String)o));
	break;
	
      case CLASS_CAMERA:
	DescribeMsg("%s %s Camera, used to set the viewpoint for Rendering.\n",
		  intro, IS);
	break;

      case CLASS_XFORM:
	DescribeMsg("%s %s Transformed object, meaning that a transformation will be applied to the final object before rendering.\n",
		  intro, IS);
	/* sayclass of xformed obj? */
	break;

      case CLASS_OBJECT:
	DescribeMsg("%s %s Generic Object, of unrecognized type.\n", intro, IS);
	break;

      case CLASS_LIGHT:
	DescribeMsg("%s %s Light, used to set the Lighting parameters for Rendering.\n",
		  intro, IS);
	break;
	
      case CLASS_CLIPPED:
	DescribeMsg("%s %s Clipped Object, which means that at Rendering time part of the object will be invisible.\n",
		    intro, IS);
	/* sayclass of clipped obj? */
	break;
	
      case CLASS_INTERPOLATOR:
	DescribeMsg("%s %s Interpolator. Internal use only.\n", intro, IS);
	break;
	
      case CLASS_SCREEN:
	DescribeMsg("%s %s Screen Object, meaning this object stays aligned with the screen.\n", 
		  intro, IS);
	/* say something about the screen obj? */
	break;
	
      case CLASS_MAX:
	DescribeMsg("Unrecognized Object.  Object type above Class Max.\n");
	break;
	
      case CLASS_MIN:
	DescribeMsg("Unrecognized Object.  Object type below Class Min.\n");
	break;

      case CLASS_PRIVATE:
	DescribeMsg("%s %s Private object, defined by a user.\n", intro, IS);
	break;

      case CLASS_DELETED:
	DescribeMsg("%s %s Deleted Object. Should not happen.\n", intro, IS);
	break;

      default:
	DescribeMsg("%s %s Unrecognized object.  Bad return from GetObjectType().\n",
		  intro, IS);
	break;
    }

    return OK;
}

/* print something about each member, up to MEMBERCOUNT items */
static Error saymembers(Group g)
{
    int i, count;
    char *name;
    Object subo, firstsubo;
    int same;
    Class grouptype;
    float position;
    
    
    if (!DXGetMemberCount(g, &count))
	return ERROR;
    
    /* no members, nothing to do here */
    if (count <= 0)
	return OK;

    /* save group type; we'll use it later on */
    grouptype = DXGetGroupClass(g);
    switch (grouptype) {
	
      case CLASS_GROUP:
	if (count <= MEMBERCOUNT) {
	    for (i=0; i<count; i++) {
		if (!(subo = DXGetEnumeratedMember(g, i, &name)))
		    return ERROR;
		if (name)
		    DescribeMsg(" member %d is named %s\n", i, name);
	    }
	} else {
	    for (i=0; i<MEMBERCOUNT/2; i++) {
		if (!(subo = DXGetEnumeratedMember(g, i, &name)))
		    return ERROR;
		if (name)
		    DescribeMsg(" member %d is named %s\n", i, name);
	    }
	    DescribeMsg(" ...\n");
	    for (i=count-MEMBERCOUNT/2; i<count; i++) {
		if (!(subo = DXGetEnumeratedMember(g, i, &name)))
		    return ERROR;
		if (name)
		    DescribeMsg(" member %d is named %s\n", i, name);
	    }
	}
	break;
	
      case CLASS_COMPOSITEFIELD:
	break;
	
      case CLASS_MULTIGRID:
	/* bbox? */
	break;
	
      case CLASS_SERIES:
	if (count <= MEMBERCOUNT) {
	    for (i=0; i<count; i++) {
		if (!(subo = DXGetSeriesMember((Series)g, i, &position)))
		    return ERROR;
		DescribeMsg(" member %d is at position %g\n", i, position);
	    }
	} else {
	    for (i=0; i<MEMBERCOUNT/2; i++) {
		if (!(subo = DXGetSeriesMember((Series)g, i, &position)))
		    return ERROR;
		DescribeMsg(" member %d is at position %g\n", i, position);
	    }
	    DescribeMsg(" ...\n");
	    for (i=count-MEMBERCOUNT/2; i<count; i++) {
		if (!(subo = DXGetSeriesMember((Series)g, i, &position)))
		    return ERROR;
		DescribeMsg(" member %d is at position %g\n", i, position);
	    }
	    
	}
	break;
	
      default:
	break;
    }

    /* if only one member, say what it is and return */
    if (count <= 1) {
	subo = DXGetEnumeratedMember(g, 0, NULL);
	return sayclass(subo, "The member", 0);
    }
	
    /* if there is more than one member of this group, check out
     * whether the group members are all the same type of object.
     * this has to be true for a composite field and should be true
     * for a multigrid, and usually for a series.
     */
    same = 1;
    firstsubo = DXGetEnumeratedMember(g, 0, NULL);
    for (i=1; i<count; i++) {
	if (!(subo = DXGetEnumeratedMember(g, i, NULL)))
	    return ERROR;
	if (DXGetObjectClass(firstsubo) != DXGetObjectClass(subo)) {
	    same = 0;
	    break;
	}
    }
    
    switch (grouptype) {
      case CLASS_GROUP:
	if (same)
	    sayclass(firstsubo, "Each group member", 0);
	else 
	    DescribeMsg("The group contains members of different object types\n");
	break;
	
      case CLASS_COMPOSITEFIELD:
	if (!same || (DXGetObjectClass(firstsubo) != CLASS_FIELD)) {
	    DescribeMsg("This is not a well-formed Partitioned Field.\n");
	    DescribeMsg("Members should all be Field objects and they are not.\n");
	    if (!same) 
		DescribeMsg("It contains members of more than one object type.\n");
	    if ((DXGetObjectClass(firstsubo) != CLASS_FIELD))
		DescribeMsg("It contains members which are not Field objects.\n");
	}
	break;
	
      case CLASS_MULTIGRID:
	/* what about partitioned members? i think they are ok */
	break;
#if 0
	if (!same || (DXGetObjectClass(firstsubo) != CLASS_FIELD)) {
	    DescribeMsg("This is not a well-formed MultiGrid Group.\n");
	    DescribeMsg("Members should all be Field objects and they are not.\n");
	    if (!same) 
		DescribeMsg("It contains members of more than one object type.\n");
	    if ((DXGetObjectClass(firstsubo) != CLASS_FIELD))
		DescribeMsg("It contains members which are not Field objects.\n");
	}
	break;
#endif
	
      case CLASS_SERIES:
	if (same)
	    sayclass(firstsubo, "Each series member", 0);
	else 
	    DescribeMsg("The series contains members of different object types\n");
	break;
	
      default:
	break;
    }
    
    return OK;
}


static void pinfo(int isitems, int items, int vitems, 
		  Type type, Category category, int rank, int *shape)
{
    int i;
    char tbuf[32];  /* temp buf */
    char mbuf[256]; /* message buf */

    mbuf[0] = '\0';

    if (isitems) {
	if (items != vitems)
	    sprintf(mbuf, "There %s %d data item%s (%d valid), each is of type ", 
		    (items==1 ? "is" : "are"), items, (items==1 ? "" : "s"), 
		    vitems);
	else
	    sprintf(mbuf, "There %s %d data item%s, each is of type ", 
		    (items==1 ? "is" : "are"), items, (items==1 ? "" : "s") );
    } else
	strcpy(mbuf, "Each item data type is ");

    switch (rank) {
      case 0:
	/* strcat(mbuf,  "scalar, "); */
	break;
      case 1:
	sprintf(tbuf, "%d-vector, ", shape[0]);
	strcat(mbuf, tbuf);
	break;
      case 2:
	sprintf(tbuf,  "%d by %d matrix, ", shape[0], shape[1]);
	strcat(mbuf, tbuf);
	break;
      default:
	sprintf(tbuf,  "%d ", shape[0]);
	strcat(mbuf, tbuf);
	for(i=1; i < rank; i++) {
	    sprintf(tbuf,  "by %d ", shape[i]);
	    strcat(mbuf, tbuf);
	}
	strcat(mbuf,  "tensor, ");
	break;
    }

    switch (category) {
      case CATEGORY_REAL:
	break;
      case CATEGORY_COMPLEX:
	strcat(mbuf,  "complex, "); break;
      case CATEGORY_QUATERNION:
	strcat(mbuf,  "quaternion, "); break;
      default:
	strcat(mbuf,  "unrecognized, "); break;
    }
    
    switch (type) {
      case TYPE_STRING:
	strcat(mbuf,  "string (a list of characters).\n"); break;
      case TYPE_SHORT:
	strcat(mbuf,  "short integer (2-byte or int*2).\n"); break;
      case TYPE_INT:
	strcat(mbuf,  "integer (4-byte or int*4).\n"); break;
      case TYPE_HYPER:
	strcat(mbuf,  "hyperlong (8-byte or int*8).\n"); break;
      case TYPE_FLOAT:
	strcat(mbuf,  "float (4-byte or real*4).\n"); break;
      case TYPE_DOUBLE:
	strcat(mbuf,  "double (8-byte or real*8).\n"); break;
      case TYPE_UINT:
	strcat(mbuf,  "unsigned integer (4-byte or int*4).\n"); break;
      case TYPE_USHORT:
	strcat(mbuf,  "unsigned short (2-byte or int*2).\n"); break;
      case TYPE_UBYTE:
	strcat(mbuf,  "unsigned byte (1-byte or int*1).\n"); break;
      case TYPE_BYTE:
	strcat(mbuf,  "byte (1-byte or int*1).\n"); break;
      default:
	strcat(mbuf,  "unrecognized.\n"); break;
    }

    DescribeMsg(mbuf);
    return;
}


#define MAXRANK 100

static Error saydata(Object o, char *opt)
{
    Array a;
    int nitems;
    Type type;
    Category category;
    int rank;
    int shape[MAXRANK];
    int valids;
    Point boxlist[8];
    Point vboxlist[8];
    float min, max, avg;

    if (DXGetObjectClass(o) == CLASS_ARRAY) {
	DXGetArrayInfo((Array)o, &nitems, &type, &category, &rank, shape);
	pinfo(1, nitems, nitems, type, category, rank, shape);
    } else if (DXGetObjectClass(o) == CLASS_FIELD) {
	a = (Array)DXGetComponentValue((Field)o, "positions");
	if (!a) {
	    DescribeMsg("The Field does not contain any positions.  It will not cause an error, but will be\n");
	    DescribeMsg("silently ignored by most modules since positions are required.\n");
	    DescribeMsg("Use the `Mark' or `Replace' module to create a `positions' component.\n");
	}
	a = (Array)DXGetComponentValue((Field)o, "data");
	if (!a) {
	    DescribeMsg("The Field contains nothing marked as data items.\n");
            DescribeMsg("If the Field does contain data under another name, use the `Mark' module\n");
            DescribeMsg("to select what should be used as data.\n");
	} else {
	    DXGetArrayInfo(a, &nitems, &type, &category, &rank, shape);
	    if (!validcount((Field)o, "data", &valids))
		return ERROR;
	    pinfo(1, nitems, valids, type, category, rank, shape);
	}
    } else if (!DXGetType((Object)o, &type, &category, &rank, shape)) {
	/* DescribeMsg("no data type information\n"); */
	DXResetError();
    } else
	pinfo(0, 0, 0, type, category, rank, shape);

    /* get bbox and print it in a non-threatening format */
    if (!DXBoundingBox(o, boxlist)) {
	/* DescribeMsg("Cannot get information about the positions of the data\n"); */
	DXResetError();
    } else {
	DescribeMsg("The positions are enclosed within the box defined by the corner points:\n");
	DescribeMsg("[ %g %g %g ] and [ %g %g %g ]\n", 
		  boxlist[0].x, boxlist[0].y, boxlist[0].z,
		  boxlist[7].x, boxlist[7].y, boxlist[7].z);
    }
    if (!DXValidPositionsBoundingBox(o, vboxlist)) {
	/* DescribeMsg("Cannot get information about the valid positions of the data\n"); */
	DXResetError();
    } else {
	if ((boxlist[0].x != vboxlist[0].x) || (boxlist[7].x != vboxlist[7].x)
	 || (boxlist[0].y != vboxlist[0].y) || (boxlist[7].y != vboxlist[7].y)
	 || (boxlist[0].z != vboxlist[0].z) || (boxlist[7].z != vboxlist[7].z)) {
	    DescribeMsg("The VALID positions are enclosed within the box defined by the corner points:\n");
	    DescribeMsg("[ %g %g %g ] and [ %g %g %g ]\n", 
		      vboxlist[0].x, vboxlist[0].y, vboxlist[0].z,
		      vboxlist[7].x, vboxlist[7].y, vboxlist[7].z);
	}
    }

    /* something about data stats */
    if (!DXStatistics(o, "data", &min, &max, &avg, NULL)) {
	/* DescribeMsg("Cannot get information about the data values\n"); */
	DXResetError();
    } else {
	DescribeMsg("Data range is:\n");
	DescribeMsg("minimum = %g, maximum = %g, average = %g\n", 
		    min, max, avg);
	if (rank > 0) {
	    /* ??? call vectorstats here if rank >= 1 
	     * look at the code in histogram; does rank=1, nothing
	     * about larger.
	     */
	    DescribeMsg("(These are the scalar statistics which will be used by modules\n");
	    DescribeMsg("which need scalar values.  The length is computed for vectors and\n");
	    DescribeMsg("the determinant for matricies.)\n");
	}
    }
    return OK;
	
}


#define NOTREADY  "Input is not ready to be rendered because"
#define READY     "Input is ready to be rendered"



static Error sayrender(Object o, char *opt)
{
    struct info info;
    int i;

    /* zero out the struct */
    memset((char *)&info, '\0', sizeof(struct info));

    /* and fill it with info */
    if (!traverse(o, &info, 0))
	goto error;


    /* sort out what we've found.  first report on things which
     * will prevent rendering.
     */
    if (info.ri.badpos > 0) {
	DescribeMsg("%s %d Field(s) in the Input object have bad positions (less than 1D or greater than 3D).\n",
		    NOTREADY, info.ri.badpos);
	DescribeMsg("If > 3D, use the `Slice' module or `Mark' and `Compute' to reduce them to 3D or less.\n");
	goto done;
    }

    if (info.ri.nocolors > 0) {
	if (info.ri.nocolors == 1)
	    DescribeMsg("%s the Field does not have colors yet.\n", 
			NOTREADY);
	else
	    DescribeMsg("%s %d Fields in the Input object do not have colors yet.\n", 
			NOTREADY, info.ri.nocolors);
	
	DescribeMsg("Use the `AutoColor', `AutoGreyScale', or `Color' modules to add colors.\n");
	goto done;
    }

    if (info.np.nodep > 0) {
	DescribeMsg("%s the `colors' component does not have a `dep' attribute\n",
		    NOTREADY);
	DescribeMsg("to indicate whether they are per vertex or per cell\n");
	DescribeMsg("Use the modules in the Structuring category to add the attribute.\n");
	goto done;
    }

    if (info.nc.cunknown > 0) {
	DescribeMsg("%s the Input contained Fields with an unsupported `color' component format.\n", NOTREADY);
	DescribeMsg("Use the `Color', `AutoColor', or `AutoGreyScale' to create a valid `color' component.\n");
	goto done;
    }

    /* warn about empty fields and fields were all items are invalid */
    if (info.np.novalid > 0) {
	if (info.ri.nfields == 0) { 
	    DescribeMsg("%s no valid items were found in any of the Field(s) in the input.\n", NOTREADY);
	    goto done;
	} else {
	    DescribeMsg("%d Fields(s) contain no valid items and will be ignored.\n",
			info.np.novalid);
	}
    }
    if (info.np.nopos > 0) {
	if (info.ri.nfields == 0) {	
	    DescribeMsg("%s none of the Field(s) in the Input object have a `positions' component.\n", NOTREADY);
	    DescribeMsg("Fields without positions are intentionally ignored without error by almost all modules.\n");
	    DescribeMsg("Use the `Mark' or `Replace' modules to create positions.\n");
	    goto done;
	} else {
	    DescribeMsg("%d Field(s) contain no positions and will be ignored.\n",
			info.np.nopos);
	}
    }


    if (info.ri.badobjs > 0) {
	for (i=0; i<info.ri.badobjs; i++) {
	    switch (info.ri.badclass[i]) {
	      case CLASS_STRING:
		DescribeMsg("A string cannot be rendered directly.  It can be converted\n");
		DescribeMsg("into text with the `Caption' or `Text' modules.\n");
		break;

	      case CLASS_ARRAY:
		DescribeMsg("A list of values cannot be rendered directly.  It can be converted\n");
		DescribeMsg("into geometry with the `AutoGlyph' module or into text with\n");
		DescribeMsg("the `Format' and then `Caption' or `Text' modules.\n");
		break;

	      case CLASS_CAMERA:
		DescribeMsg("A Camera cannot be part of the rendered object.\n");
		DescribeMsg("There is a separate Camera input on the `Render' and `Display' modules.\n");
		DescribeMsg("It is not required for the `Image' module but can be specified to control the initial or reset viewpoint.\n");
		break;

	      case CLASS_INTERPOLATOR:
	      case CLASS_MAX:
	      case CLASS_MIN:
	      case CLASS_PRIVATE:
	      case CLASS_DELETED:
	      default:
		DescribeMsg("An unexpected (and unrenderable) object was found in the input.\n");
		break;
	    }
	}
	goto done;  /* ??? or fall thru? */
    }
    
    
    /* check here for bad connection types? */
    if (info.nd.cbad > 0) {
	if (info.nd.cbad == 1)
	    DescribeMsg("%s the Field has a bad `connections' component.\n",
			NOTREADY);
	else
	    DescribeMsg("%s %d Fields in the Input object have a bad `connections' component.\n",
			NOTREADY, info.nd.cbad);
	goto done;
    }
    
    if (info.nd.cmissing > 0) {
	if (info.nd.cmissing == 1)
	    DescribeMsg("%s the Field has no `element type' attribute on the `connections' component.\n", NOTREADY);
	else
	    DescribeMsg("%s %d Fields have no `element type' attribute on the `connections' component.\n",
			NOTREADY, info.nd.cmissing);
	DescribeMsg("The renderer must know what type of geometry is in each Field (e.g. `triangles', `lines').\n");
	DescribeMsg("This attribute can be added using the modules in the Structuring category.\n");
	goto done;
    }
    
    if (info.nd.cplus > 0) {
	if (info.nd.cplus == 1)
	    DescribeMsg("%s the Field has a `connections' component of more than 3D.\n", NOTREADY);
	else
	    DescribeMsg("%s %d Fields have a `connections' component of more than 3D.\n", NOTREADY, info.nd.cplus);
	DescribeMsg("The `Slice' module can be used to reduce the input to 3D or lower.\n");
	goto done;
    }

    /* ok, we've checked for bad things.  how about nothing? */
    if (info.ri.nfields <= 0) {
	DescribeMsg("There is nothing to render in the input because it does not contain any Field objects.\n");
	DescribeMsg("Field objects are usually created by the `Import' module\n");
	DescribeMsg("or by the modules in the `Realization' category.\n");
	goto done;
    }

    
    
    /* ok, praps it can be rendered after all */
    
    /* warn about disabling the default lighting. */
    if (info.ri.nlights > 0)
	DescribeMsg("%d Light%s found in the Input; the default lighting will not be used.\n",
		    info.ri.nlights, info.ri.nlights > 1 ? "s were" : " was");
    
    
    
    /* if already image, suggest Display (w/no camera???) */
    if (qimage(o)) {
	DescribeMsg("Input is already an Image.  It can be displayed fastest using the `Display' module with no `Camera' input.\n");
	DescribeMsg("Use the `Image' module, which will be slower, if you want to resize the image, rotate it, zoom in or out,\n");
	DescribeMsg("or combine the image with other renderable items.\n");
    }
    
    /* connections?  points, lines, surfaces, volumes */
    if (info.nd.c0 > 0) {
	DescribeMsg("Input contains a set of scattered points.  They will be rendered\n");
	DescribeMsg("as individual pixels.  Use the `AutoGlyph' or `Glyph'\n");
	DescribeMsg("modules to make larger objects, or use the `Connect' module\n");
	DescribeMsg("for 2D data or the `AutoRegrid' or `Regrid' modules for 3D data.\n");
    }

    /* lines? */
    if (info.nd.c1 > 0) {
	DescribeMsg("Input contains a set of lines.  They will be rendered as a single\n");
	DescribeMsg("pixel-wide line.  Use the `Tube' or `Ribbon' modules\n");
	DescribeMsg("to make larger objects.\n");
    }
    
    /* surfaces */
    if (info.nd.c2 > 0) {
	DescribeMsg("Input contains a surface.  Planar (flat) surfaces exactly aligned\n");
	DescribeMsg("along the viewpoint will NOT be visible.  To see the\n");
	DescribeMsg("surface move the viewpoint slightly off-axis.\n");
	/* something about front/back colors and hardware rendering */
    }
	
    /* volumes */
    if (info.nd.c3) {
	DescribeMsg("Input contains a volume and will be rendered using volume rendering techniques.\n");
	DescribeMsg("These are the slowest types of objects to render, and they produce\n");
	DescribeMsg("images with `fuzzy cloud' effects.  To get a sharp image, convert\n");
	DescribeMsg("part of the volume into a surface using the `ShowBoundary',\n");
	DescribeMsg("`MapToPlane', or `Isosurface' modules.   The software renderer usually\n");
	DescribeMsg("gives better results for volumes since most hardware cards do not support volume rendering.\n");
    }

    /* and last, tell them about colors */
    if (info.nc.cbytemap > 0) {
	DescribeMsg("Input contains Fields with Delayed colors.  Delayed colors use a 256-entry lookup table of colors,\n");
	DescribeMsg("with the data values as the index into the table.\n"); 
	DescribeMsg("Data must be of type `ubyte' to use Delayed colors.\n");
	DescribeMsg("Each entry in the lookup table itself is a 3-float triplet specifying the color.\n");
	DescribeMsg("Valid color values are between 0 and 1 for each of Red, Green and Blue.\n");
    }
    if (info.nc.cubyte > 0) {
	DescribeMsg("Input contains Fields with Byte colors.  Byte colors use 3-byte triplets per data value.\n");
	DescribeMsg("Valid color values are between 0 and 255 for each of Red, Green and Blue.\n");
    }
    if (info.nc.cfloat > 0) {
	DescribeMsg("Input contains Fields with Float colors.  Float colors use 3-float triplets per data value.\n");
	DescribeMsg("Valid color values are between 0 and 1 for each of Red, Green and Blue.\n");
    }

  done:
    DXFree((Pointer)info.ri.badclass);
    return OK;

  error:
    DXFree((Pointer)info.ri.badclass);
    return ERROR;

}

#if 0
/* return all fields which aren't empty.
 */
static Field getpartplus(Object o, int *i, int *count)
{
    int again = 1;
    Field f;

    while (again) {
	f = DXGetPart(o, *i);
	if (!f)
	    return NULL;
	
	(*i)++;

	if (DXEmptyField(f))
	    continue;

	(*count)++;
	return f;
    }

    /* not reached */
}


/* unlike Inquire, say yes if you find any */
static int queryNDconnections(Field f, int n)
{
    char *cp;
    Object con;
    
    
    if (!(con = DXGetComponentValue(f, "connections"))) {
	if (n == 0)
	    return 1;
	return 0;
    }
    
    if (DXGetObjectClass(con) != CLASS_ARRAY)
	return 0;
    
    if (!DXGetStringAttribute(con, "element type", &cp))
	return 0;
    
    if (!strcmp(cp, "lines") && (n == 1))
	return 1;
    
    if ((!strcmp(cp, "triangles") || !strcmp(cp, "quads")) && (n == 2))
	return 1;
    
    if ((!strcmp(cp, "tetrahedra") || !strcmp(cp, "cubes")) && (n == 3))
	return 1;

    if (!strncmp(cp, "cubes", 5) && (strlen(cp) > 5) && (n > 3))
	return 1;

    return 0;
}
#endif

/* return how many items in this field are valid */
static Error validcount(Field f, char *component, int *icount)
{
    Array a = NULL;
    char *dep = NULL;
    char *invalid = NULL;
    InvalidComponentHandle ih;

    /* assume none and be happily surprised when it finds some */
    *icount = 0;

    /* this must be called at the field level.  if you want to recurse
     * through a hierarchy, do the recursion above this subroutine.
     */
    if (DXGetObjectClass((Object)f) != CLASS_FIELD) {
	Return_Internal_Error;
    }
    
    a = (Array)DXGetComponentValue(f, component);
    if (!a) {
	DXSetError(ERROR_DATA_INVALID, "field has no `%s' component",
		   component);
	return ERROR;
    }

    /* follow a dependency. */
    if (!DXGetStringAttribute((Object)a, "dep", &dep)) {
	DXSetError(ERROR_DATA_INVALID, 
		   "The `%s' component does not have a `dep' attribute",
		   component);
	return ERROR;
    }
    
    if (!DXGetArrayInfo(a, icount, NULL, NULL, NULL, NULL))
	return ERROR;

#define INVLEN 10     /* strlen("invalid "); */    
    
    if (!(invalid = (char *)DXAllocate(strlen(dep) + INVLEN)))
	return ERROR;

    strcpy(invalid, "invalid ");
    strcat(invalid, dep);

    /* if no invalid component, all valid */
    if (!DXGetComponentValue(f, invalid)) {
	DXFree((Pointer)invalid);
	return OK;
    }
    DXFree((Pointer)invalid);

    ih = DXCreateInvalidComponentHandle((Object)f, NULL, dep);
    if (!ih) 
	return ERROR;

    *icount = DXGetValidCount(ih);
    DXFreeInvalidComponentHandle(ih);

    return OK;
}



/* make sure the field contains colors and that > 1 are valid.
 * i guess we can look to see that the positions are 1D <= p <= 3D
 * as well.  and we could look at connections here as well.
 */
static Error validfield(Field f, struct info *ip, int nosee)
{
    int icount = 0;
    Array colors, pos, conn;
    char *elem = NULL;
    char *dep = NULL;
    char *invalid = NULL;
    InvalidComponentHandle ih;

    
    /* check for fields - recurse above this */
    if (DXGetObjectClass((Object)f) != CLASS_FIELD) {
	Return_Internal_Error;
    }

    /* check pos first */
    pos = (Array)DXGetComponentValue(f, "positions");
    if (!pos) {
	ip->np.nopos++;
	return OK;
    }

    if (!DXGetArrayInfo(pos, &icount, NULL, NULL, NULL, NULL))
	return ERROR;

    if (icount <= 0) {
	ip->np.nopos++;
	return OK;
    }

    /* pos must be vector, rank 1,  1 <= shape <= 3 */
    if ((!DXTypeCheck(pos, TYPE_FLOAT, CATEGORY_REAL, 1, 1)) &&
	(!DXTypeCheck(pos, TYPE_FLOAT, CATEGORY_REAL, 1, 2)) &&
	(!DXTypeCheck(pos, TYPE_FLOAT, CATEGORY_REAL, 1, 3))) {
	ip->ri.badpos++;
	return OK;
    }

    /* check Nd of conn here.  if "nosee" is true, don't count the
     * Nd of the connections because normal modules won't "see" these
     * as normal fields.
     */
    
    if (!nosee) {
	conn = (Array)DXGetComponentValue(f, "connections");
	if (!conn)
	    ip->nd.c0++;
	else if (DXGetObjectClass((Object)conn) != CLASS_ARRAY)
	    ip->nd.cbad++;
	else if (!DXGetStringAttribute((Object)conn, "element type", &elem))
	    ip->nd.cmissing++;
	else if (!strcmp(elem, "lines"))
	    ip->nd.c1++;
	else if (!strcmp(elem, "triangles") || !strcmp(elem, "quads"))
	    ip->nd.c2++;
	else if (!strcmp(elem, "tetrahedra") || !strcmp(elem, "cubes"))
	    ip->nd.c3++;
	else if (!strncmp(elem, "cubes", 5) && (strlen(elem) > 5))
	    ip->nd.cplus++;
	else
	    ip->nd.cbad++;
    }

   
    /* expect colors and follow dependency */
    colors = (Array)DXGetComponentValue(f, "colors");
    if (!colors) {
	ip->ri.nocolors++;
	return OK;
    } else {
	/* check for colormap component */
	if (DXGetComponentValue(f, "color map") != NULL)
	    ip->nc.cbytemap++;
	else if (DXTypeCheck(colors, TYPE_UBYTE, CATEGORY_REAL, 1, 3))
	    ip->nc.cubyte++;	
	else if (DXTypeCheck(colors, TYPE_FLOAT, CATEGORY_REAL, 1, 3))
	    ip->nc.cfloat++;
	else
	    ip->nc.cunknown++; 
    }

    /* follow colors dependency.  it will be unusual to find no dep. */
    if (!DXGetStringAttribute((Object)colors, "dep", &dep)) {
	ip->np.nodep++;
	return OK;
    }
    
    if (!DXGetArrayInfo(colors, &icount, NULL, NULL, NULL, NULL))
	return ERROR;

    if (icount == 0) {
	ip->np.novalid++;
	return OK;
    }
	
#define INVLEN 10     /* strlen("invalid "); */    
    
    if (!(invalid = (char *)DXAllocate(strlen(dep) + INVLEN)))
	return ERROR;

    strcpy(invalid, "invalid ");
    strcat(invalid, dep);

    /* if no invalid component, all valid */
    if (!DXGetComponentValue(f, invalid)) {
	DXFree((Pointer)invalid);
	ip->ri.nfields++;
	return OK;
    }
    DXFree((Pointer)invalid);

    ih = DXCreateInvalidComponentHandle((Object)f, NULL, dep);
    if (!ih) 
	return ERROR;

    icount = DXGetValidCount(ih);
    DXFreeInvalidComponentHandle(ih);

    if (icount == 0) {
	ip->np.novalid++;
	return OK;
    }

    ip->ri.nfields++;
    return OK;
}


/* accumulate info about what's what
 */
static Error traverse(Object o, struct info *ip, int nosee)
{
    Object subo;
    Class curclass;
    int i;

    switch (curclass = DXGetObjectClass(o)) {
      case CLASS_FIELD:
	return validfield((Field)o, ip, nosee);
	
      case CLASS_GROUP:
	/* traverse members */
	for (i=0; (subo = DXGetEnumeratedMember((Group)o, i, NULL)); i++) {
	    if (!traverse(subo, ip, nosee))
		return ERROR;
	}
	break;
	
      case CLASS_SCREEN:
	if (!DXGetScreenInfo((Screen)o, &subo, NULL, NULL))
	    return ERROR;

	return traverse(subo, ip, 1);

      case CLASS_XFORM:
	if (!DXGetXformInfo((Xform)o, &subo, NULL))
	    return ERROR;

	return traverse(subo, ip, nosee);

      case CLASS_CLIPPED:
	if (!DXGetClippedInfo((Clipped)o, &subo, NULL))
	    return ERROR;

	return traverse(subo, ip, nosee);  /* is this right? */

      case CLASS_LIGHT:
	ip->ri.nlights++;
	break;
	
      case CLASS_INTERPOLATOR:
      case CLASS_MAX:
      case CLASS_MIN:
      case CLASS_PRIVATE:
      case CLASS_DELETED:
      case CLASS_STRING:
      case CLASS_ARRAY:
      case CLASS_CAMERA:	
      default:
	for (i=0; i<ip->ri.badobjs; i++) {
	    if (ip->ri.badclass[i] == curclass)
		return OK;
	}
	ip->ri.badobjs++;
	ip->ri.badclass = (Class *)DXReAllocate((Pointer)ip->ri.badclass, 
						sizeof(Class) * ip->ri.badobjs);
	ip->ri.badclass[ip->ri.badobjs - 1] = curclass;

	break;
    }

    return OK;
}

static int qimage(Object o)
{
    int n;
    Array a;
    char *s;
    
    /* an image has got to be a field or a composite field */
    if (DXGetObjectClass(o) != CLASS_FIELD) {
	if (DXGetObjectClass(o) != CLASS_GROUP)
	    return 0;
	if (DXGetGroupClass((Group)o) != CLASS_COMPOSITEFIELD)
	    return 0;
	o = DXGetEnumeratedMember((Group)o, 0, NULL);
    }

    /* check to see if it's the right shape, etc */

    /* check positions */
    a = (Array) DXGetComponentValue((Field)o, "positions");
    if (!a || !DXQueryGridPositions(a, &n, NULL, NULL, NULL) || n!=2)
	return 0;
    
    /* check connections */
    a = (Array) DXGetComponentValue((Field)o, "connections");
    if (!a || !DXQueryGridConnections(a, &n, NULL) || n!=2)
	return 0;

    /* is there a colors component */
    a = (Array) DXGetComponentValue((Field)o, "colors");
    if (!a)
	return 0;

    /* check dep */
    s = DXGetString((String)DXGetAttribute((Object)a, "dep"));
    if (s && strcmp(s, "positions"))
	return 0;
    
    return 1;
}


#if 0
static Error atleast1field(Object o)
{
    Object subo, old;
    int i;

    switch (DXGetObjectClass(o)) {
      case CLASS_FIELD:
	return OK;

      case CLASS_GROUP:
	/* traverse members */
	for (i=0; subo = DXGetEnumeratedMember((Group)o, i, NULL); i++)
	    if (atleast1field(subo))
		return OK;
	break;
    
      case CLASS_SCREEN:
	if (!DXGetScreenInfo((Screen)o, &subo, NULL, NULL))
	    return ERROR;

	return atleast1field(subo);

      case CLASS_XFORM:
	if (!DXGetXformInfo((Xform)o, &subo, NULL))
	    return ERROR;

	return atleast1field(subo);

      case CLASS_CLIPPED:
	if (!DXGetClippedInfo((Clipped)o, &subo, NULL))
	    return ERROR;

	return atleast1field(subo);

      default:
	break;
    }

    return ERROR;
}


static Error atleast1image(Object o)
{
    Object subo, old;
    int i;

    switch (DXGetObjectClass(o)) {
      case CLASS_FIELD:
	if (qimage(o))
	    return OK;
	break;

      case CLASS_GROUP:
	/* traverse members */
	for (i=0; subo = DXGetEnumeratedMember((Group)o, i, NULL); i++)
	    if (atleast1image(subo))
		return OK;
	break;
    
      case CLASS_SCREEN:
	if (!DXGetScreenInfo((Screen)o, &subo, NULL, NULL))
	    return ERROR;

	return atleast1image(subo);

      case CLASS_XFORM:
	if (!DXGetXformInfo((Xform)o, &subo, NULL))
	    return ERROR;

	return atleast1image(subo);

      case CLASS_CLIPPED:
	if (!DXGetClippedInfo((Clipped)o, &subo, NULL))
	    return ERROR;

	return atleast1image(subo);

      default:
	break;
    }

    return ERROR;
}


static Error atleast1contype(Object o, int n)
{
    Object subo, old;
    int i;

    switch (DXGetObjectClass(o)) {
      case CLASS_FIELD:
	if (queryNDconnections((Field)o, n))
	    return OK;
	break;

      case CLASS_GROUP:
	/* traverse members */
	for (i=0; subo = DXGetEnumeratedMember((Group)o, i, NULL); i++)
	    if (atleast1contype(subo, n))
		return OK;
	break;
    
      case CLASS_SCREEN:
	/* these don't act like other objs;  don't count them for this */
	break;

      case CLASS_XFORM:
	if (!DXGetXformInfo((Xform)o, &subo, NULL))
	    return ERROR;

	return atleast1contype(subo, n);

      case CLASS_CLIPPED:
	if (!DXGetClippedInfo((Clipped)o, &subo, NULL))
	    return ERROR;

	return atleast1contype(subo, n);

      default:
	break;
    }

    return ERROR;
}
#endif


