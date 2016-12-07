/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * Header: /usr/people/gresh/code/svs/src/libdx/RCS/print.c,v 5.6 93/02/05 10:59:31 gda Exp Locker: gresh 
 * 
 */

#include <dxconfig.h>


#include <dx/dx.h>
#include <ctype.h>
#include <stdarg.h>
#include <string.h>

#define BYTES 1		/* compute the internal bytes used to store objects */
#define COMPS 1		/* keep statistics on the field components separately */

#if BYTES
#include "objectClass.h"
#include "arrayClass.h"
#include "cameraClass.h"
#include "clippedClass.h"
#include "fieldClass.h"
#include "groupClass.h"
#include "interpClass.h"
#include "lightClass.h"
#include "privateClass.h"
#include "screenClass.h"
#include "stringClass.h"
#include "xformClass.h"
#endif

#define MAXP    50          /* default max array items to print */
#define MAXRANK 16          /* max rank */

#define NEWLINE(l)	DXMessage("\n%*s", l, "")
#define INDENT(l)       DXMessage("%*s", l, "")
#if BYTES
#define PEXP()		if(p->expand) DXMessage("[%08lx] {%x} (+%d) ", \
					        (unsigned long)o, \
						(unsigned int)o->tag, o->count)
#else
#define PEXP()		if(p->expand) DXMessage("[%08x] ", (unsigned int)o)
#endif

struct ap_info {
    char *format;
    char *space;
    int formatwidth;
    int stride;
    int increment;
} ap[10][3] = { 
	/* strings */
      { { "%1s",                      	  "%1s",   1,   64,    1 },
	{ "(%1s,%1s)",                	  "%6s",   6,   12,    2 },
	{ "(%1s,%1s,%1s,%1s)",       	 "%10s",  10,    6,    4 } },
	/* unsigned hex bytes */
      { { "%02x",                     	  "%3s",   3,   20,    1 },
	{ "(%02x,%02x)",              	  "%8s",   8,    8,    2 },
	{ "(%02x,%02x,%02x,%02x)",   	 "%14s",  14,    4,    4 } },
	/* unsigned decimal bytes */
      { { "%3u",                   	  "%4s",   4,   20,    1 },
	{ "(%3u,%3u)",            	 "%10s",  10,    6,    2 },
	{ "(%3u,%3u,%3u,%3u)", 	 	 "%18s",  18,    4,    4 } },
	/* bytes */
      { { "%3d",                   	  "%4s",   4,   20,    1 },
	{ "(%3d,%3d)",            	 "%10s",  10,    6,    2 },
	{ "(%3d,%3d,%15g,%15g)",  	 "%18s",  18,    4,    4 } },
	/* unsigned shorts */
      { { "%6u",                   	 "%7s",    7,    8,    1 },
	{ "(%6u,%6u)",            	 "%16s",  16,    4,    2 },
	{ "(%6u,%6u,%6u,%6u)",	  	 "%32s",  32,    2,    4 } },
	/* shorts */
      { { "%6d",                      	  "%7s",   7,    8,    1 },
	{ "(%6d,%6d)",               	 "%16s",  16,    4,    2 },
	{ "(%6d,%6d,%6d,%6d)",       	 "%32s",  32,    2,    4 } },
	/* unsigned ints */
      { { "%9u",                   	 "%10s",  10,    6,    1 },
	{ "(%9u,%9u)",            	 "%22s",  22,    3,    2 },
	{ "(%9u,%9u,%9u,%9u)", 	         "%42s",  42,    1,    4 } },
	/* ints */
      { { "%9d",                     	 "%10s",  10,    8,    1 },
	{ "(%9d,%9d)",               	 "%22s",  22,    3,    2 },
	{ "(%9d,%9d,%9d,%9d)",       	 "%42s",  42,    1,    4 } },
	/* floats */
      { { "%14.8g",                  	 "%15s",  15,    5,    1 },
	{ "(%14.8g,%14.8g)",         	 "%32s",  32,    3,    2 },
	{ "(%14.8g,%14.8g,%14.8g,%14.8g)", "%62s",  62,    1,    4 } },
	/* doubles */
      { { "%24.18g",                   	 "%24s",  24,    3,    1 },
	{ "(%24.18g,%24.18g)",           	 "%52s",  52,    1,    2 },
	{ "(%24.18g,%24.18g,%24.18g,%24.18g)", "%102s",  102,    1,    4 } },
};

#define NCLASSES 32     /* raise this if the number of classes goes up */
struct sum_info {
    int num;  		/* how many of each object type */
    int min;		/* min, max and avg for members, components, items */
    int max;
    int sum;
    int depth;		/* deepest level in hierarchy this object occurs */
    int bytes;		/* byte count */
    int expanded;	/* for compact arrays, how many have been expanded */
    int expbytes;	/*  and the amount of space comsumed by that expansion */
    struct str_info *c; /* for computing separate stats for components by name */
};

struct str_info {
    char *strname;	/* string name */
    struct sum_info cs; /* stats for just this type of named object */
    struct str_info *l; /* linked list */
};

struct prt_info {
    int recurse;	/* if set, recursively print subobjects */
    int printdata;	/* if 1, print 50 array items; if 2, print all */
    int expand;		/* if 1, print addresses of objects */
    int level;		/* current level of recursion */
    int maxlevel;	/* maximum number of levels to recurse */
    int doattrs;	/* print object attributes */
    int ncomp;		/* if > 0, number of components to print for fields */
    char **comps;	/* the list of component names */
    int *compfound;	/* hit list; used to detect bad component names */
    int didcomp;	/* set if any prints get down to the component level */
    struct sum_info *s; /* if requested, summarize the object structure */
};


static void pinfo(int isitems, int items, Type t, Category c, int rank, 
                  int *shape, int level);
static void pvalue(Type t, Category c, int rank, int *shape, 
		   Pointer value, int comma);
static void pcurse(Type t, Category c, int rank, int *shape, 
		   Pointer value, int offset, int skip, int comma);
static void pstring(char *cp, int newline);
static void pchar(char cp);
static void pcharlist(char *cp, int length);
static void pattrib(Object o, struct prt_info *p);
static void sattrib(Object o, struct prt_info *p);
static void init_sum(struct sum_info *s);
static void del_sum(struct sum_info *s);

static void Object_Print (Object o, struct prt_info *p);
static void Camera_Print (Camera camera, struct prt_info *p);
static void Light_Print (Light light, struct prt_info *p);
static void Array_Print (Array array, struct prt_info *p);
static void Clipped_Print (Clipped clipped, struct prt_info *p);
static void Field_Print (Field field, struct prt_info *p);
static void String_Print (String string, struct prt_info *p);
static void Xform_Print (Xform xform, struct prt_info *p);
static void Group_Print (Group group, struct prt_info *p);
static void Screen_Print (Screen screen, struct prt_info *p);
static void Interpolator_Print (Interpolator interp, struct prt_info *p);

static void Object_Summary (Object o, struct prt_info *p);
static void Camera_Summary (Camera camera, struct prt_info *p);
static void Light_Summary (Light light, struct prt_info *p);
static void Array_Summary (Array array, struct prt_info *p);
static void Clipped_Summary (Clipped clipped, struct prt_info *p);
static void Field_Summary (Field field, struct prt_info *p);
static void String_Summary (String string, struct prt_info *p);
static void Xform_Summary (Xform xform, struct prt_info *p);
static void Group_Summary (Group group, struct prt_info *p);
static void Screen_Summary (Screen screen, struct prt_info *p);
static void Interpolator_Summary (Interpolator interp, struct prt_info *p);
static void Summary_Print(struct sum_info *s);


#if DEBUG
/* private entry points for debugger use */
static void prt(Object o, char *options)
{
    DXPrintV(o, options, 0);
    return;
}

static void prt1(Object o, char *options, char *component)
{
    char *c[2];

    c[0] = component;
    c[1] = NULL;

    DXPrintV(o, options, c);
    return;
}
#endif




Error
DXPrint(Object o, char *options, ... )
{
    char *components[100];
    int i;
    va_list arg;

    va_start(arg,options);
    for (i = 0;  ; i++)
	if (NULL == (components[i] = va_arg(arg, char *)))
	    break;

    va_end(arg);
    
    if (i == 0)
	return DXPrintV(o, options, NULL);
    else
	return DXPrintV(o, options, components);
}

     
Error DXPrintV(Object o, char *options, char **components)
{
    int i;
    int recursion_set = 0;
    int do_print = 0;
    int do_sum = 0;
    char ch;
    struct prt_info p;

    p.recurse = 0;	        /* recurse or not */
    p.printdata = 0;	        /* print data? */
    p.expand = 0;	        /* expanded info */
    p.level = 0;	        /* starting recursion level - always 0 here */
    p.maxlevel = 128;	        /* max recursion level */
    p.doattrs = 1;	        /* print attributes by default */
    p.ncomp = 0;	        /* number of components to print; 0 == all */
    p.comps = components;	/* restrict printing to this list */
    p.compfound = NULL;         /* hit count; allows warnings if bad comp name */
    p.didcomp = 0;	        /* set if prt actually got down to comp level */
    p.s = NULL;		        /* if summary requested, collect stats here */

    if(options) {
    	recursion_set = 0;
    	for (i=0; (ch=options[i])!=0; i++) {
	    switch (ch) {
	      case 'r':
		if (!recursion_set) {
		    recursion_set = 1;
		    p.recurse = 1;
		} else {
		    DXErrorReturn(ERROR_BAD_PARAMETER, "r or o already specified");
		}
		do_print = 1;
		break;
	      case 'o':
		if (!recursion_set) {
		    recursion_set = 1;
		    p.recurse = 0;
		} else {
		    DXErrorReturn(ERROR_BAD_PARAMETER, "r or o already specified");
		}
		do_print = 1;
		break;
	      case 'd':
		p.printdata = 1;
		p.recurse = 1;
		do_print = 1;
		break;
	      case 'D':
		p.printdata = 2;	
		p.recurse = 1;
		do_print = 1;
		break;
	      case 'x':
		p.expand = 1;
		do_print = 1;
		break;
	      case 'a':
		p.doattrs = 0;
		do_print = 1;
		break;
	      case 's':
		do_sum = 1;
		break;
	      default:
		if (isdigit(ch)) {
		    p.maxlevel = ch - '0';
		    if (p.maxlevel < 0) {
			DXErrorReturn(ERROR_BAD_PARAMETER, "bad max level number");
		    }
		    p.recurse = 1;
		    do_print = 1;
		} else {
		    DXSetError(ERROR_BAD_PARAMETER, "bad option \"%c\"", ch); 
		    return ERROR;
		}
	    }
	}
    }

    /* print object contents */
    if (do_print) {
	/* count up number of components - list must be null terminated */
	if (components) {
	    for(p.ncomp=0; components[p.ncomp]; p.ncomp++)
		;
	    p.compfound = (int *)DXAllocateZero(sizeof(int) * p.ncomp);
	    if (!p.compfound)
		return ERROR;
	}
	
	DXUIMessage("PRINT", "");
	DXBeginLongMessage();
	Object_Print(o, &p);
	DXEndLongMessage();
	
	if (p.compfound && p.didcomp) {
	    for (i=0; i<p.ncomp; i++)
		if (p.compfound[i] == 0)
		    DXWarning("component `%s' was not found", components[i]);
	    DXFree((Pointer)p.compfound);
	}
    }

    /* printinfo about the object structure itself */
    if (do_sum) {
	if (!(p.s = (struct sum_info *)
	      DXAllocateLocalZero(sizeof(struct sum_info) * NCLASSES)))
	    return ERROR;
	
	init_sum(p.s);
	
	Object_Summary(o, &p);

	DXUIMessage("PRINT", "");
	DXBeginLongMessage();
	Summary_Print(p.s);
	DXEndLongMessage();

	del_sum(p.s);
    }

    if (DXGetError() != ERROR_NONE)
	return ERROR;

    return OK;
}

/* print objects */

static void Object_Print(Object o, struct prt_info *p)
{

    if(!o) {
        INDENT(p->level);
        DXMessage("Null Object.\n");
        return;
    }

    INDENT(p->level);  PEXP();

    switch(DXGetObjectClass(o)) {

      case CLASS_GROUP:
        Group_Print((Group)o, p);
        break;

      case CLASS_FIELD:
        DXMessage("Field.  ");
        Field_Print((Field)o, p);
        break;

      case CLASS_ARRAY:
        Array_Print((Array)o, p);
        break;

      case CLASS_STRING:
	DXMessage("String.  ");
	String_Print((String)o, p);
	break;
	
      case CLASS_CAMERA:
	DXMessage("Camera.  ");
	Camera_Print((Camera)o, p);
	break;

      case CLASS_XFORM:
	DXMessage("Transform.  ");
	Xform_Print((Xform)o, p);
	break;

      case CLASS_OBJECT:
	DXMessage("Generic Object.\n");
	break;

      case CLASS_LIGHT:
	DXMessage("Light.  "); 
	Light_Print((Light)o, p);
	break;
	
      case CLASS_CLIPPED:
	DXMessage("Clipped Object.\n");
	Clipped_Print((Clipped)o, p);
	break;
	
      case CLASS_INTERPOLATOR:
	DXMessage("Interpolator.  ");
	Interpolator_Print((Interpolator)o, p);
	break;
	
      case CLASS_SCREEN:
	DXMessage("Screen Object.\n");
	Screen_Print((Screen)o, p);
	break;
	
      case CLASS_MAX:
	DXMessage("Unrecognized Object.  Object type above Class Max.\n");
	break;
	
      case CLASS_MIN:
	DXMessage("Unrecognized Object.  Object type below Class Min.\n");
	break;

      case CLASS_PRIVATE:
	{ unsigned long po = (unsigned long)DXGetPrivateData((Private)o);
	  DXMessage("Private Object.  Start of data area = %08lx\n", po);
	  break;
        }

      case CLASS_DELETED:
	DXMessage("Deleted Object.\n");
	break;

      default:
	DXMessage("Unrecognized object.  Bad return from GetObjectType().\n");
	break;
    }

    if (p->doattrs)
	pattrib(o, p);

}


static void Array_Print (Array array, struct prt_info *p)
{
    Type type;
    Category category;
    int items, rank, tshape = 1, shape[MAXRANK];
    Array terms[MAXRANK];
    int pitems = 0, maxp = MAXP;
    int stride;
    Pointer origin, delta, ptr;
    int i, k;
    int numtype, numcat;
    int split = 0;
    char *format, *space;
    char *cp;
    int increment;

    
    switch (DXGetArrayClass(array)) {
	
      case CLASS_ARRAY:
	DXGetArrayInfo(array, &items, &type, &category, &rank, shape);
	DXMessage("Generic Array.  ");
	pinfo(1, items, type, category, rank, shape, p->level);
	
	if(!p->recurse || p->level >= p->maxlevel || !p->printdata)
	    break;
	
	if (items <= 0)
	    break;

	tshape = 1;
	for (i=0; i<rank; i++)
	    tshape *= shape[i];

	if (p->printdata > 1)
	    maxp = items;

	switch(category) {
	  case CATEGORY_REAL:	        numcat = 0; break;
	  case CATEGORY_COMPLEX:	numcat = 1; break;
	  case CATEGORY_QUATERNION:	numcat = 2; break;
	  default:
	    return;
	}

	switch(type) {
	  case TYPE_STRING:	    numtype = 0; break;
	  case TYPE_UBYTE:	    numtype = 1; break;  /* 1=hex, 2=dec */
	  case TYPE_BYTE:	    numtype = 3; break;
	  case TYPE_USHORT:	    numtype = 4; break;
	  case TYPE_SHORT:	    numtype = 5; break;
	  case TYPE_UINT:	    numtype = 6; break;
	  case TYPE_INT:	    numtype = 7; break;
	  case TYPE_FLOAT:	    numtype = 8; break;
	  case TYPE_DOUBLE:	    numtype = 9; break;
	  default:
	    return;
	}

	ptr = DXGetArrayData(array);

	INDENT(p->level);
	if(items > maxp) {
	    pitems = maxp/2;
	    DXMessage("first %d and last %d data values only:\n", 
		    pitems, pitems);
	    split++;
	} else
	    DXMessage("data values:\n");
	
	format = ap[numtype][numcat].format;
	space = ap[numtype][numcat].space;
	stride = (rank > 0 && shape[rank-1] <= ap[numtype][numcat].stride) ? 
	                          shape[rank-1] : ap[numtype][numcat].stride;
	increment = ap[numtype][numcat].increment;

	INDENT(p->level);

#define PR_TYPE(type, dataptr) { \
\
    type *dp = (type *)dataptr; \
    for(i=0; i<items*tshape; i++, dp+=increment) { \
	switch(numcat) { \
	  case 0: \
	    DXMessage(format, *dp); \
	    break; \
	  case 1: \
	    DXMessage(format, *dp, *(dp+1)); \
	    break; \
	  case 2: \
	    DXMessage(format, *dp, *(dp+1), *(dp+2), *(dp+3)); \
	    break; \
	} \
	if(!((i+1)%stride)) { \
	    DXMessage("\n"); \
	    INDENT(p->level); \
	} else \
	    DXMessage(" "); \
\
	if(split && (i+1 == pitems*tshape)) { \
	    if((i+1)%stride) { \
		DXMessage("\n"); \
		INDENT(p->level); \
	    } \
	    DXMessage("...\n"); \
	    INDENT(p->level); \
	    i = (items - pitems) * tshape - 1; \
	    for(k = 0; k < (i+1)%stride; k++) \
		DXMessage(space, " "); \
\
	    dp = (type *)DXGetArrayData(array) + i * increment; \
	} \
    } \
}

	switch(numtype) {
	  case 1:    /* unsigned hex bytes */
          case 2:    /* unsigned decimal bytes */
	    PR_TYPE(ubyte, ptr);
	    break;

	  case 3:   /* signed bytes */
	    PR_TYPE(byte, ptr);
	    break;

	  case 4:    /* unsigned shorts */
	    PR_TYPE(ushort, ptr);
	    break;

	  case 5:    /* short ints */
	    PR_TYPE(short, ptr);
	    break;

	  case 6:    /* unsigned ints */
	    PR_TYPE(uint, ptr);
	    break;

	  case 7:    /* regular ints */
	    PR_TYPE(int, ptr);
	    break;

	  case 8:    /* floats */
	    PR_TYPE(float, ptr);
	    break;

	  case 9:    /* doubles */
	    PR_TYPE(double, ptr);
	    break;

	  case 0:    /* ascii strings */
	    cp = (char *)ptr;
	    if (rank == 0) {
		/* call pcharlist here w/pitems and max-pitems */
		pcharlist(cp, split ? pitems : items);
		
		if(split) {
		    
		    DXMessage("...\n");
		    INDENT(p->level);
		    
		    cp = (char *)DXGetArrayData(array) + (items - pitems);
		    pcharlist(cp, pitems);
		}

	    } else {
		tshape /= shape[rank-1];
		for (i=0; i<items*tshape; i++, cp+=shape[rank-1]) {
		    pstring(cp, 1);
		    INDENT(p->level);
		    
		    if(split && (i+1 == pitems)) {
			
			DXMessage("...\n");
			INDENT(p->level);
			
			i = (items - pitems) * tshape - 1;
			cp = (char *)DXGetArrayData(array) + i * shape[rank-1];
		    }
		}
	    }
	    break;

	}
	DXMessage("\n");
	break;
	
      case CLASS_REGULARARRAY:
	DXGetArrayInfo(array, &items, &type, &category, &rank, shape);
	DXMessage("Regular Array.  ");
	pinfo(1, items, type, category, rank, shape, p->level);
	
	if(!p->recurse || p->level >= p->maxlevel || !p->printdata)
	    break;
	
	origin = delta = NULL;
	origin = DXAllocateLocal(DXGetItemSize(array));
	delta = DXAllocateLocal(DXGetItemSize(array));
	if(!delta || !origin) {
	    if(delta) DXFree(delta);
	    if(origin) DXFree(origin);
	    break;
	}
	
	DXGetRegularArrayInfo((RegularArray)array, &items, origin, delta);
	INDENT(p->level);
	DXMessage("start value ");
	pvalue(type, category, rank, shape, origin, 1);
	
	DXMessage("delta ");
	pvalue(type, category, rank, shape, delta, 1);

	DXMessage("for %d repetitions\n", items);
	DXFree(origin);
	DXFree(delta);
	break;
    
    case CLASS_CONSTANTARRAY:
	DXGetArrayInfo(array, &items, &type, &category, &rank, shape);
	DXMessage("Constant Array.  ");
	pinfo(1, items, type, category, rank, shape, p->level);

	if(!p->recurse || p->level >= p->maxlevel || !p->printdata)
	    break;
	
	INDENT(p->level);
	DXMessage("constant value ");
	pvalue(type, category, rank, shape, DXGetConstantArrayData(array), 0);
	DXMessage("\n", items);
	break;
	
      case CLASS_PRODUCTARRAY:
	DXGetProductArrayInfo((ProductArray)array, &items, terms);
	DXMessage("Product Array.  %d term%s.\n", items, (items==1 ? "" : "s"));
	
	if(!p->recurse || p->level >= p->maxlevel)
	    break;
	
	for(i=0; i<items; i++) {
	    INDENT(p->level);
	    DXMessage("Product term %d: ", i);
	    p->level++;
	    Object_Print((Object)terms[i], p);
	    p->level--;
	}
	break;
	
      case CLASS_MESHARRAY:
	DXGetMeshArrayInfo((MeshArray)array, &items, terms);
	DXMessage("Mesh Array.  %d term%s.\n", items, (items==1 ? "" : "s"));
	if (items > 0 && DXQueryGridConnections(array, NULL, NULL)) {
	    if (!DXGetMeshOffsets((MeshArray)array, shape))
		return;
	    
	    INDENT(p->level);
	    DXMessage("Mesh offset: ");
	    for(i=0; i<items-1; i++)
		DXMessage("%d, ", shape[i]);
	    DXMessage("%d\n", shape[items-1]);
	}
	    
	
	if(!p->recurse || p->level >= p->maxlevel)
	    break;

	for(i=0; i<items; i++) {
	    INDENT(p->level);
	    DXMessage("Mesh term %d: ", i);
	    p->level++;
	    Object_Print((Object)terms[i], p);
	    p->level--;
	}
	break;
	
      case CLASS_PATHARRAY:
	DXGetPathArrayInfo((PathArray)array, &items);
	DXMessage("Path Array.  connects %d item%s\n", items, 
		                                  (items==1 ? "" : "s"));
	break;
	
      default:
	INDENT(p->level);
	DXMessage("Unrecognized Array Type.\n");
	break;
    }

    return;
}


static void Camera_Print (Camera camera, struct prt_info *p)
{

    float fov, world, aspect;
    Vector up;
    Point from, to;
    Matrix tm;
    int height, width;
    RGBColor backgnd;


    DXGetCameraResolution(camera, &width, &height);
    DXMessage("%d pixels wide by %d pixels high.\n", width, height);

    INDENT(p->level);
    if(DXGetOrthographic(camera, &world, &aspect)) {
	DXMessage("orthographic projection, viewport %g units by %g units\n",
	                                  (double)world, (double)world*aspect);
    } else if (DXGetPerspective(camera, &fov, &aspect)) {
	if(fov == 0.0)
	    DXMessage("perspective projection, undefined (illegal) viewport\n");
	else
	    DXMessage("perspective projection, field of view %g\n", fov);

    } else
            DXMessage("bad projection - neither orthographic nor projection\n");
        
    INDENT(p->level);
    DXGetView(camera, &from, &to, &up);
    DXMessage("looking at [%g %g %g] ",
	    (double)to.x, (double)to.y, (double)to.z);
    DXMessage("from [%g %g %g], ",
	    (double)from.x, (double)from.y, (double)from.z);
    DXMessage("up direction is [%g %g %g]\n", 
	    (double)up.x, (double)up.y, (double)up.z);

    INDENT(p->level);
    DXGetBackgroundColor(camera, &backgnd);
    DXMessage("background color: [%g %g %g]\n", backgnd.r, backgnd.g, backgnd.b);

    if(!p->expand)
	return;
    
    tm = DXGetCameraMatrix(camera);
    INDENT(p->level);
    DXMessage("resulting transformation matrix:\n");
    INDENT(p->level);
    DXMessage(" [ %8g %8g %8g ]\n", 
	    (double)tm.A[0][0], (double)tm.A[0][1], (double)tm.A[0][2]);
    INDENT(p->level);
    DXMessage(" [ %8g %8g %8g ]\n", 
	    (double)tm.A[1][0], (double)tm.A[1][1], (double)tm.A[1][2]);
    INDENT(p->level);
    DXMessage(" [ %8g %8g %8g ] ", 
	    (double)tm.A[2][0], (double)tm.A[2][1], (double)tm.A[2][2]);
    /* print on same line */
    DXMessage("+ [ %8g %8g %8g ]\n", 
	    (double)tm.b[0], (double)tm.b[1], (double)tm.b[2]);

    return;
}

static void Clipped_Print (Clipped clipped, struct prt_info *p)
{

    Object render, clipping;

    if(!p->recurse || p->level >= p->maxlevel)
	return;

    DXGetClippedInfo((Clipped) clipped, &render, &clipping);

    INDENT(p->level);
    DXMessage("Object being clipped: ");
    p->level++;
    Object_Print(render, p);
    p->level--;

    INDENT(p->level);
    DXMessage("Object doing clipping: ");
    p->level++;
    Object_Print(clipping, p);
    p->level--;
    
    return;

}

static void Screen_Print (Screen screen, struct prt_info *p)
{
    Object o;
    int position, z;

    if(!p->recurse || p->level >= p->maxlevel)
	return;

    DXGetScreenInfo((Screen)screen, &o, &position, &z);

    INDENT(p->level);

    switch(position) {
      case SCREEN_WORLD:
	DXMessage("world coordinates, ");
	break;
      case SCREEN_VIEWPORT:
	DXMessage("viewport-relative coordinates, ");
	break;
      case SCREEN_PIXEL:
	DXMessage("pixel coordinates, ");
	break;
      case SCREEN_STATIONARY:
	DXMessage("stationary object origin, ");
	break;
      default:
	DXMessage("bad 'position' value, ");
	break;
    }

    switch(z) {
      case -1:
	DXMessage("behind other objects:\n"); break;
      case 0:
	DXMessage("in scene with other objects:\n"); break;
      case 1:
	DXMessage("in front of other objects:\n"); break;
      default:
	DXMessage("bad 'z' value:\n"); break;
    }
    
    p->level++;
    Object_Print(o, p);
    p->level--;

    return;
}

static void Interpolator_Print (Interpolator interp, struct prt_info *p)
{
    Type t;
    Category c;
    int rank, shape[MAXRANK];

    if(!DXGetType((Object)interp, &t, &c, &rank, shape)) {
	DXMessage(" has no type information\n");
	return;
    }

    DXMessage(" for ");
    pinfo(0, 0, t, c, rank, shape, p->level);

    return;
}

static void Field_Print (Field field, struct prt_info *p)
{

    Object subo;
    int i, j;
    char *name;
    
    for(i=0; DXGetEnumeratedComponentValue((Field)field, i, NULL); i++)
	;
    
    DXMessage("%d component%s.\n", i, (i==1 ? "" : "s"));

    if(!p->recurse || p->level >= p->maxlevel)
	return;

    /* p->level++; */
    for(i=0; (subo=DXGetEnumeratedComponentValue((Field)field, i, &name)); i++) {
	INDENT(p->level);
	DXMessage("Component number %d, name '%s':\n", i, name);

	if(p->level >= p->maxlevel)
	    continue;

	/* we got down to the level where we can print components */
	p->didcomp = 1;

	/* if there is a list, see if this component name is on it. */
	for(j=0; j<p->ncomp; j++)
	    if(p->comps[j] && !strcmp(name, p->comps[j])) {
		p->compfound[j]++;
		break;
	    }
	
	/* if they gave a component list, and this component isn't on it,
	 *  don't print it.
	 */
	if(p->ncomp > 0 && j == p->ncomp)
	    continue;
	    
	p->level++;
	Object_Print(subo, p);
	p->level--;
    }
    /* p->level--; */

    return;
}


static void Light_Print (Light light, struct prt_info *p)
{
    Vector v;
    RGBColor c;

    if(DXQueryAmbientLight(light, &c))
	DXMessage("Ambient light, color [%g, %g, %g].\n", c.r, c.g, c.b);

    else if(DXQueryDistantLight(light, &v, &c))
	DXMessage("Distant light, from [%g, %g, %g], color [%g, %g, %g].\n",
	                                    v.x, v.y, v.z, c.r, c.g, c.b);
    else if(DXQueryCameraDistantLight(light, &v, &c))
       DXMessage("Distant light, [%g, %g, %g] from camera, color [%g, %g, %g].\n",
	                                    v.x, v.y, v.z, c.r, c.g, c.b);
    else
	DXMessage("Unknown type.\n");
	
    
    if(!p->expand)
	return;

}

static void String_Print (String string, struct prt_info *p)
{
    pstring((char *)DXGetString(string), 1);
}

static void Xform_Print (Xform xform, struct prt_info *p)
{
    Matrix tm;
    Object subo;
    
    DXGetXformInfo((Xform) xform, &subo, &tm);
    DXMessage("transformation matrix:\n");
    INDENT(p->level);
    DXMessage(" [ %8g %8g %8g ]\n", 
	    (double)tm.A[0][0], (double)tm.A[0][1], (double)tm.A[0][2]);
    INDENT(p->level);
    DXMessage(" [ %8g %8g %8g ]\n", 
	    (double)tm.A[1][0], (double)tm.A[1][1], (double)tm.A[1][2]);
    INDENT(p->level);       
    DXMessage(" [ %8g %8g %8g ] ", 
	    (double)tm.A[2][0], (double)tm.A[2][1], (double)tm.A[2][2]);
    /* same line as last */    
    DXMessage("+ [ %8g %8g %8g ]\n", 
	    (double)tm.b[0], (double)tm.b[1], (double)tm.b[2]);

#if 0    
    if(p->recurse) {
#endif
	INDENT(p->level);
	DXMessage("to be applied to object:\n");
	p->level++;
	Object_Print(subo, p);
	p->level--;
#if 0
    }
#endif
    
    return;
}

static void Group_Print (Group group, struct prt_info *p)
{
    int i, count;
    float position;
    char *name;
    Object subo;
    Type type;
    Category category;
    int rank, shape[MAXRANK];

    if (!DXGetMemberCount(group, &count))
	return;

    switch(DXGetGroupClass((Group)group)) {
	
      case CLASS_GROUP:
	DXMessage("Generic Group.  %d member%s.\n", count, (count==1 ? "" : "s"));

	goto group_common;
	
      case CLASS_COMPOSITEFIELD:
	DXMessage("Composite Field.  %d member%s.\n", count, (count==1 ? "" : "s"));

	INDENT(p->level);
        if(!DXGetType((Object)group, &type, &category, &rank, shape))
	    DXMessage("no data type information\n");
	else {
	    DXMessage("data items are ");
	    pinfo(0, 0, type, category, rank, shape, p->level);
	}

	goto group_common;
	
      case CLASS_MULTIGRID:
	DXMessage("Multigrid Group.  %d member%s.\n", count, (count==1 ? "" : "s"));

	INDENT(p->level);
        if(!DXGetType((Object)group, &type, &category, &rank, shape))
	    DXMessage("no data type information\n");
	else {
	    DXMessage("data items are ");
	    pinfo(0, 0, type, category, rank, shape, p->level);
	}

      group_common:
	if(!p->recurse || p->level >= p->maxlevel)
	    break;

	for(i=0; (subo=DXGetEnumeratedMember((Group)group, i, &name)); i++) {
	    INDENT(p->level);
	    if (name)
		DXMessage("Member %d, name '%s':\n", i, name);
	    else
		DXMessage("Member %d:\n", i);
	    p->level++;
	    Object_Print(subo, p);
	    p->level--;
	}
	break;
	
      case CLASS_SERIES:
	DXMessage("Series Group.  %d member%s.\n", count, (count==1 ? "" : "s"));

	INDENT(p->level);
        if(!DXGetType((Object)group, &type, &category, &rank, shape))
	    DXMessage("no data type information\n");
	else {
	    DXMessage("data items are ");
	    pinfo(0, 0, type, category, rank, shape, p->level);
	}

	if(!p->recurse || p->level >= p->maxlevel)
	    break;

	p->level++;
	for(i=0; (subo=DXGetSeriesMember((Series)group, i, &position)); i++) {
	    INDENT(p->level);
	    DXMessage("Member %d, position %g:\n", i, position);
	    Object_Print(subo, p);
	}
	p->level--;
	break;
	
      default:
	DXMessage("Unrecognized Group Type.\n");
	break;
    }
    
    return;

}

/* summarize object structure 
 */

static void Object_Summary(Object o, struct prt_info *p)
{
    Class class, subclass;

    if(!o)
        return;

    class = DXGetObjectClass(o);
    if (class < CLASS_MIN || class > CLASS_MAX)
	return;

    if (class == CLASS_GROUP) {
	subclass = DXGetGroupClass((Group)o);
	p->s[subclass].num++;
	if (p->level > p->s[subclass].depth)
	    p->s[subclass].depth = p->level;
    } else if (class == CLASS_ARRAY) {
	subclass = DXGetArrayClass((Array)o);
	p->s[subclass].num++;
	if (p->level > p->s[subclass].depth)
	    p->s[subclass].depth = p->level;
    } else {
	p->s[class].num++;
	if (p->level > p->s[class].depth)
	    p->s[class].depth = p->level;
    }

    switch(class) {

      case CLASS_GROUP:
        Group_Summary((Group)o, p);
        break;

      case CLASS_FIELD:
        Field_Summary((Field)o, p);
        break;

      case CLASS_ARRAY:
        Array_Summary((Array)o, p);
        break;

      case CLASS_STRING:
	String_Summary((String)o, p);
	break;
	
      case CLASS_CAMERA:
	Camera_Summary((Camera)o, p);
	break;

      case CLASS_XFORM:
	Xform_Summary((Xform)o, p);
	break;

      case CLASS_LIGHT:
	Light_Summary((Light)o, p);
	break;
	
      case CLASS_CLIPPED:
	Clipped_Summary((Clipped)o, p);
	break;
	
      case CLASS_INTERPOLATOR:
	Interpolator_Summary((Interpolator)o, p);
	break;
	
      case CLASS_SCREEN:
	Screen_Summary((Screen)o, p);
	break;
	
      case CLASS_MAX:
	DXMessage("Unrecognized Object.  Object type above Class Max.\n");
	break;
	
      case CLASS_MIN:
	DXMessage("Unrecognized Object.  Object type below Class Min.\n");
	break;

      case CLASS_PRIVATE:
      case CLASS_DELETED:
      case CLASS_OBJECT:
      default:
	break;
    }

    sattrib(o, p);
    return;

}


static void Array_Summary (Array array, struct prt_info *p)
{
    Type type;
    Category category;
    Class subclass;
    int items, rank, shape[MAXRANK];
    Array terms[MAXRANK];
    int i;

    
    subclass = DXGetArrayClass((Array)array);
    
    if (subclass < CLASS_MIN || subclass > CLASS_MAX)
	return;


    switch(subclass) {
	
      case CLASS_ARRAY:
	DXGetArrayInfo((Array)array, &items, &type, &category, &rank, shape);

	if (items < p->s[subclass].min)
	    p->s[subclass].min = items;
	if (items > p->s[subclass].max)
	    p->s[subclass].max = items;
	p->s[subclass].sum += items;
	
	p->s[subclass].bytes += items * DXGetItemSize((Array)array);
	
	break;
	
      case CLASS_REGULARARRAY:
	DXGetRegularArrayInfo((RegularArray)array, &items, NULL, NULL);

	if (items < p->s[subclass].min)
	    p->s[subclass].min = items;
	if (items > p->s[subclass].max)
	    p->s[subclass].max = items;
	p->s[subclass].sum += items;

#if BYTES
	/* compute expanded bytes here */
	if (array->data) {
	    p->s[subclass].expanded++;
	    p->s[subclass].expbytes += array->items * DXGetItemSize((Array)array);
	}
#endif
	break;
	
      case CLASS_CONSTANTARRAY:
	DXGetArrayInfo((Array)array, &items, &type, &category, &rank, shape);

	if (items < p->s[subclass].min)
	    p->s[subclass].min = items;
	if (items > p->s[subclass].max)
	    p->s[subclass].max = items;
	p->s[subclass].sum += items;
	
#if BYTES
	/* compute expanded bytes here */
	if (array->data) {
	    p->s[subclass].expanded++;
	    p->s[subclass].expbytes += array->items * DXGetItemSize((Array)array);
	}
#endif
	break;
	
      case CLASS_PATHARRAY:
	DXGetPathArrayInfo((PathArray)array, &items);

	if (items < p->s[subclass].min)
	    p->s[subclass].min = items;
	if (items > p->s[subclass].max)
	    p->s[subclass].max = items;
	p->s[subclass].sum += items;

#if BYTES
	/* compute expanded bytes here */
	if (array->data) {
	    p->s[subclass].expanded++;
	    p->s[subclass].expbytes += array->items * DXGetItemSize((Array)array);
	}
#endif
	break;
	
      case CLASS_PRODUCTARRAY:
	DXGetProductArrayInfo((ProductArray)array, &items, terms);

	if (items < p->s[subclass].min)
	    p->s[subclass].min = items;
	if (items > p->s[subclass].max)
	    p->s[subclass].max = items;
	p->s[subclass].sum += items;

	for(i=0; i<items; i++) {
	    p->level++;
	    Object_Summary((Object)terms[i], p);
	    p->level--;
	}

#if BYTES
	/* compute expanded bytes here */
	if (array->data) {
	    p->s[subclass].expanded++;
	    p->s[subclass].expbytes += array->items * DXGetItemSize((Array)array);
	}
#endif
	break;
	
      case CLASS_MESHARRAY:
	DXGetMeshArrayInfo((MeshArray)array, &items, terms);

	if (items < p->s[subclass].min)
	    p->s[subclass].min = items;
	if (items > p->s[subclass].max)
	    p->s[subclass].max = items;
	p->s[subclass].sum += items;

	for(i=0; i<items; i++) {
	    p->level++;
	    Object_Summary((Object)terms[i], p);
	    p->level--;
	}

#if BYTES
	/* compute expanded bytes here */
	if (array->data) {
	    p->s[subclass].expanded++;
	    p->s[subclass].expbytes += array->items * DXGetItemSize((Array)array);
	}
#endif
	break;
	
      default:
	return;
    }

    return;
}


static void Camera_Summary (Camera camera, struct prt_info *p)
{
    return;
}

static void Clipped_Summary (Clipped clipped, struct prt_info *p)
{
    Object render, clipping;

    DXGetClippedInfo((Clipped) clipped, &render, &clipping);

    p->level++;
    Object_Summary(render, p);
    p->level--;

    p->level++;
    Object_Summary(clipping, p);
    p->level--;
    
    return;

}

static void Screen_Summary (Screen screen, struct prt_info *p)
{
    Object o;
    int position, z;

    DXGetScreenInfo((Screen)screen, &o, &position, &z);

    p->level++;
    Object_Summary(o, p);
    p->level--;

    return;
}

static void Interpolator_Summary (Interpolator interp, struct prt_info *p)
{
    return;
}

static void Field_Summary (Field field, struct prt_info *p)
{
    Object subo;
    Class class;
    struct str_info *sp;
    int i;
    char *name;

    class = CLASS_FIELD;

    for(i=0; DXGetEnumeratedComponentValue((Field)field, i, NULL); i++)
	;
    
    if (i < p->s[class].min)
	p->s[class].min = i;
    if (i > p->s[class].max)
	p->s[class].max = i;
    p->s[class].sum += i;

    for(i=0; (subo=DXGetEnumeratedComponentValue((Field)field, i, &name)); i++) {

	p->level++;
	Object_Summary(subo, p);
	p->level--;

#if COMPS  /* new stuff */
	if (p->s[class].c == NULL) {
	    if ((sp = (struct str_info *)
		DXAllocateLocalZero(sizeof (struct str_info))) == NULL)
		return;
	    p->s[class].c = sp;
	    sp->strname = name;
	    sp->cs.num++;
	    sp->l = NULL;
	} else {
	    sp = p->s[class].c;
	    while (sp != NULL) {
		if (!strcmp(sp->strname, name)) {
		    sp->cs.num++;
		    break;
		}
		if (sp->l == NULL) {
		    if ((sp->l = (struct str_info *)
			 DXAllocateLocalZero(sizeof (struct str_info))) == NULL)
			return;
		    sp->l->strname = name;
		    sp->l->cs.num++;
		    sp->l->l = NULL;
		    break;
		}
		sp = sp->l;
	    }
	}
#endif

    }

    return;
}


static void Light_Summary (Light light, struct prt_info *p)
{
    return;

}

static void String_Summary (String string, struct prt_info *p)
{
    char *cp;
    int len = 0;
    Class class;
    
    class = CLASS_STRING;

    cp = (char *)DXGetString((String)string);
    if (cp && *cp) 
	len = strlen(cp);
    
    if (len < p->s[class].min)
	p->s[class].min = len;
    if (len > p->s[class].max)
	p->s[class].max = len;
    p->s[class].sum += len;

    return;

}

static void Xform_Summary (Xform xform, struct prt_info *p)
{
    Matrix tm;
    Object subo;
    
    DXGetXformInfo((Xform) xform, &subo, &tm);

    p->level++;
    Object_Summary(subo, p);
    p->level--;
    
    return;
}

static void Group_Summary (Group group, struct prt_info *p)
{
    int i;
    Object subo;
    Class subclass;
    int items;

    subclass = DXGetGroupClass(group);

    if (subclass < CLASS_MIN || subclass > CLASS_MAX)
	return;

    if (!DXGetMemberCount(group, &items))
	return;

    if (items < p->s[subclass].min)
	p->s[subclass].min = items;
    if (items > p->s[subclass].max)
	p->s[subclass].max = items;
    p->s[subclass].sum += items;
    
    for(i=0; (subo=DXGetEnumeratedMember(group, i, NULL)); i++) {
	p->level++;
	Object_Summary(subo, p);
	p->level--;
    }

    return;

}


static void sattrib(Object o, struct prt_info *p)
{
    Object subo;
    char *name;
    int i;

    for (i=0; (subo=DXGetEnumeratedAttribute(o, i, &name)); i++) {
	p->level++;
	Object_Summary(subo, p);
	p->level--;
    }

}

/* anything which shouldn't start out zero */
static void init_sum(struct sum_info *s)
{
    int i;

    for (i=0; i<NCLASSES; i++)
	s[i].min = DXD_MAX_INT;

    return;
}

static void del_sum(struct sum_info *s)
{
    int i;
    struct str_info *sp, *splink;

    for (i=0; i<NCLASSES; i++) {
#if COMPS
	sp = s[i].c;
	while (sp != NULL) {
	    splink = sp->l;
	    DXFree((Pointer)sp);
	    sp = splink;
	}
#endif
    }
    
    DXFree((Pointer)s);

    return;
}

static void Summary_Print(struct sum_info *s)
{
    int i;
    char linebuf[128];
    struct str_info *sp;
    char sbuf[64], mbuf[64], ebuf[64];

#if BYTES
    char *sformat = "%d %s  (%d header bytes)\n    ";
#define USED(x)  sizeof(x) * s[i].num
#else
    char *sformat = "%d %s%s";
#define USED(x)  " "  
#endif

    char *mformat = "Min, max, avg, total %s: %d, %d, %g, %d";
    char *eformat = "Max depth %d";
    char *lformat = "%s%-55s%-15s\n";

    for (i=0; i<NCLASSES; i++) {
	if (s[i].num <= 0)
	    continue;

	sprintf(mbuf, " ");

	switch ((Class)i) {
	  case CLASS_GROUP:
	    sprintf(sbuf, sformat, s[i].num, "Groups.", USED(struct group));
	    sprintf(mbuf, mformat, "Members", 
		    s[i].min, s[i].max, ((float)s[i].sum)/s[i].num,
		    s[i].sum);
	    break;

	  case CLASS_SERIES:
	    sprintf(sbuf, sformat, s[i].num, "Series.", USED(struct series));
	    sprintf(mbuf, mformat, "Members", 
		    s[i].min, s[i].max, ((float)s[i].sum)/s[i].num,
		    s[i].sum);
	    break;

	  case CLASS_COMPOSITEFIELD:
	    sprintf(sbuf, sformat, s[i].num, "CompositeFields.", 
		    USED(struct compositefield));
	    sprintf(mbuf, mformat, "Members", 
		    s[i].min, s[i].max, ((float)s[i].sum)/s[i].num,
		    s[i].sum);
	    break;

	  case CLASS_MULTIGRID:
	    sprintf(sbuf, sformat, s[i].num, "MultiGrids.", USED(struct multigrid));
	    sprintf(mbuf, mformat, "Members", 
		    s[i].min, s[i].max, ((float)s[i].sum)/s[i].num,
		    s[i].sum);
	    break;

	  case CLASS_FIELD:
	    sprintf(sbuf, sformat, s[i].num, "Fields.", USED(struct field));
	    sprintf(mbuf, mformat, "Components", 
		    s[i].min, s[i].max, ((float)s[i].sum)/s[i].num,
		    s[i].sum);
	    break;

	  case CLASS_ARRAY:
	    sprintf(sbuf, sformat, s[i].num, "Arrays.", USED(struct array));
	    sprintf(mbuf, mformat, "Items", 
		    s[i].min, s[i].max, ((float)s[i].sum)/s[i].num,
		    s[i].sum);
	    break;

	  case CLASS_CONSTANTARRAY:
	    sprintf(sbuf, sformat, s[i].num, "ConstantArrays.",
		    USED(struct constantarray));
	    sprintf(mbuf, mformat, "Items", 
		    s[i].min, s[i].max, ((float)s[i].sum)/s[i].num,
		    s[i].sum);
	    break;

	  case CLASS_REGULARARRAY:
	    sprintf(sbuf, sformat, s[i].num, "RegularArrays.",
		    USED(struct regulararray));
	    sprintf(mbuf, mformat, "Items", 
		    s[i].min, s[i].max, ((float)s[i].sum)/s[i].num,
		    s[i].sum);
	    break;

	  case CLASS_PATHARRAY:
	    sprintf(sbuf, sformat, s[i].num, "PathArrays.",
		    USED(struct patharray));
	    sprintf(mbuf, mformat, "Items", 
		    s[i].min, s[i].max, ((float)s[i].sum)/s[i].num,
		    s[i].sum);
	    break;

	  case CLASS_PRODUCTARRAY:
	    sprintf(sbuf, sformat, s[i].num, "ProductArrays.",
		    USED(struct productarray));
	    sprintf(mbuf, mformat, "Terms", 
		    s[i].min, s[i].max, ((float)s[i].sum)/s[i].num,
		    s[i].sum);
	    break;

	  case CLASS_MESHARRAY:
	    sprintf(sbuf, sformat, s[i].num, "MeshArrays.", 
		    USED(struct mesharray));
	    sprintf(mbuf, mformat, "Terms", 
		    s[i].min, s[i].max, ((float)s[i].sum)/s[i].num,
		    s[i].sum);
	    break;

	  case CLASS_STRING:
	    sprintf(sbuf, sformat, s[i].num, "Strings.", USED(struct string));
	    sprintf(mbuf, mformat, "Length", 
		    s[i].min, s[i].max, ((float)s[i].sum)/s[i].num,
		    s[i].sum);
	    break;
	
	  case CLASS_CAMERA:
	    sprintf(sbuf, sformat, s[i].num, "Cameras.", USED(struct camera));
	    break;

	  case CLASS_XFORM:
	    sprintf(sbuf, sformat, s[i].num, "Transforms.", 
		    USED(struct xform));
	    break;

	  case CLASS_OBJECT:
	    sprintf(sbuf, sformat, s[i].num, "Generic Objects.",
		    USED(struct object));
	    break;

	  case CLASS_LIGHT:
	    sprintf(sbuf, sformat, s[i].num, "Lights.",
		    USED(struct light)); 
	    break;
	
	  case CLASS_CLIPPED:
	    sprintf(sbuf, sformat, s[i].num, "Clipped Objects.",
		    USED(struct clipped));
	    break;
	
	  case CLASS_INTERPOLATOR:
	    sprintf(sbuf, sformat, s[i].num, "Interpolators.",
		    USED(struct interpolator));
	    break;
	
	  case CLASS_SCREEN:
	    sprintf(sbuf, sformat, s[i].num, "Screen Objects.",
		    USED(struct screen));
	    break;
	    
	  case CLASS_PRIVATE:
	    sprintf(sbuf, sformat, s[i].num, "Private Objects.",
		    USED(struct private));
	    break;
	
	  case CLASS_DELETED:
	    sprintf(sbuf, sformat, s[i].num, "Deleted Objects.", 
		    USED(struct object));
	    break;
	    
	  case CLASS_MIN:
	  case CLASS_MAX:
	  default:
	    sprintf(sbuf, sformat, s[i].num, "Unrecognized objects.",
		    USED(struct object));
	    break;
	}

	sprintf(ebuf, eformat, s[i].depth);
	
	sprintf(linebuf, lformat, sbuf, mbuf, ebuf);
	DXMessage(linebuf);

	if (s[i].bytes > 0)
	    DXMessage("    (%d data bytes)\n", s[i].bytes);
	if (s[i].expanded > 0)
	    DXMessage("    (%d expanded arrays containing %d bytes)\n",
		    s[i].expanded, s[i].expbytes);
	
#if COMPS
	if (s[i].c != NULL) {
	    sp = s[i].c;
	    while (sp != NULL) {
		DXMessage("   %d '%s' components\n", sp->cs.num, sp->strname);
		sp = sp->l;
	    }
	}
#endif
	    
    }

    return;
}



/* misc internal routines */

static void pinfo(int isitems, int items, Type type, Category category, 
                  int rank, int *shape, int level)
{
    int i;

    if(isitems)
	DXMessage("%d item%s, ", items, (items==1 ? "" : "s") );

    switch(type) {
      case TYPE_STRING:
	DXMessage("string, "); break;
      case TYPE_SHORT:
	DXMessage("short, "); break;
      case TYPE_INT:
	DXMessage("integer, "); break;
      case TYPE_HYPER:
	DXMessage("hyperlong, "); break;
      case TYPE_FLOAT:
	DXMessage("float, "); break;
      case TYPE_DOUBLE:
	DXMessage("double, "); break;
      case TYPE_UINT:
	DXMessage("unsigned integer, "); break;
      case TYPE_USHORT:
	DXMessage("unsigned short, "); break;
      case TYPE_UBYTE:
	DXMessage("unsigned byte, "); break;
      case TYPE_BYTE:
	DXMessage("byte, "); break;
      default:
	DXMessage("unrecognized, "); break;
    }

    switch(category) {
      case CATEGORY_REAL:
	DXMessage("real, "); break;
      case CATEGORY_COMPLEX:
	DXMessage("complex, "); break;
      case CATEGORY_QUATERNION:
	DXMessage("quaternion, "); break;
      default:
	DXMessage("unrecognized, "); break;
    }
    
    switch(rank) {
      case 0:
	DXMessage("scalar\n");
	break;
      case 1:
	DXMessage("%d-vector\n", shape[0]);
	break;
      case 2:
	DXMessage("%d by %d matrix\n", shape[0], shape[1]);
	break;
      default:
	DXMessage("%d ", shape[0]);
	for(i=1; i < rank; i++)
	    DXMessage("by %d ", shape[i]);
	DXMessage("tensor\n");
	break;
    }

    return;
}

/* try not to use this for TYPE_STRING - pstring() is better */
static void pv(Type t, Pointer value, int offset)
{
    switch (t) {
      case TYPE_STRING:
	pchar(((char *)value)[offset]);
      case TYPE_SHORT:
        DXMessage("%d", ((short *)value)[offset]); break;
      case TYPE_INT:
        DXMessage("%d", ((int *)value)[offset]); break;
      case TYPE_FLOAT:
        DXMessage("%g", ((float *)value)[offset]); break;
      case TYPE_DOUBLE:
        DXMessage("%.18g", ((double *)value)[offset]); break;
      case TYPE_UINT:
        DXMessage("%u", ((uint *)value)[offset]); break;
      case TYPE_USHORT:
        DXMessage("%u", ((ushort *)value)[offset]); break;
      case TYPE_UBYTE:
        DXMessage("%u", ((ubyte *)value)[offset]); break;
      case TYPE_BYTE:
        DXMessage("%d", ((byte *)value)[offset]); break;
      case TYPE_HYPER:
      default:
        DXMessage("?"); break;
    }
}

static void pchar(char c)
{
    /* escape chars according to ansi c */
    switch(c) {
      case '%':  DXMessage("%%");   return;   /* the printf conversion char */
#ifndef DXD_NO_ANSI_ALERT
      case '\a': DXMessage("\\a");  return;   /* attention (bell) */
#endif
      case '\b': DXMessage("\\b");  return;   /* backspace */
      case '\f': DXMessage("\\f");  return;   /* formfeed */
      case '\n': DXMessage("\\n");  return;   /* newline */
      case '\r': DXMessage("\\r");  return;   /* carriage return */
      case '\t': DXMessage("\\t");  return;   /* horizontal tab */
      case '\v': DXMessage("\\v");  return;   /* vertical tab */
      case '\\': DXMessage("\\\\"); return;   /* backslash */
      case '"':  DXMessage("\\\""); return;   /* double quote */
      case '\0': DXMessage("\\0");  return;   /* NULL */
    }

    /* normal printable chars and octal-escape coded chars */
    if (isprint(c))
	DXMessage("%c", c);              /* normal printable char */
    else
	DXMessage("\\%03o", (uint)c);    /* octal (or hex?) ascii code */

    return;
}

static void pcharlist(char *cp, int items)
{
    
    if (!cp) {
	DXMessage("'\\0''");
	return;
    }

    while(items--) {
	DXMessage("'");
	pchar(*cp++);
	DXMessage("' ");
    }
}


static void pstring(char *cp, int newline)
{
    
    if (!cp) {
	DXMessage( "\"(NULL)\"%c", newline ? '\n' : ' ');
	return;
    }

    DXMessage("\"");
    while(*cp)
	pchar(*cp++);

    DXMessage("\"%c", newline ? '\n' : ' ');
}


static void pc(Type t, Pointer value, int offset)
{
    DXMessage("(");
    pv(t, value, offset);
    DXMessage(", ");
    pv(t, value, offset+1);
    DXMessage(")");
}

static void pq(Type t, Pointer value, int offset)
{
    DXMessage("(");
    pv(t, value, offset);
    DXMessage(", ");
    pv(t, value, offset+1);
    DXMessage(", ");
    pv(t, value, offset+2);
    DXMessage(", ");
    pv(t, value, offset+3);
    DXMessage(")");
}

static void pa(Category c, Type t, Pointer value, int offset)
{
    offset *= DXCategorySize(c);
    switch (c) {
      case CATEGORY_REAL:
        pv(t, value, offset);
        break;
      case CATEGORY_COMPLEX:
        pc(t, value, offset);
        break;
      case CATEGORY_QUATERNION:
        pq(t, value, offset);
        break;
    }
}

static void pcurse(Type type, Category category, int rank, int *shape, 
                   Pointer value, int offset, int skip, int comma)
{
    int i, newskip;
    
    switch(rank) {
      case 0:
	if (type == TYPE_STRING)
	    pstring(value, 0);
	else
	    pa(category, type, value, offset);
        DXMessage("%s", comma ? ", " : " ");
        break;

      default:
	DXMessage("[ ");
	--rank;
	newskip = skip / shape[rank];
	for (i=0; i<shape[rank]; i++)
	    pcurse(type, category, rank, shape, value, offset + newskip * i, 
		   newskip, (i+1 < shape[rank]));  /* commas between,not after */

	DXMessage("]%s", comma ? ", " : " ");
    }

}


static void pvalue(Type type, Category category, int rank, int *shape, 
                   Pointer value, int comma)
{
    int skip = 1;
    int i;

    for (i=0; i<rank; i++)
	skip *= shape[i];

    if (type == TYPE_STRING && rank > 0) {
	skip /= shape[rank-1];
	--rank;
    }

    pcurse(type, category, rank, shape, value, 0, skip, comma);
}


static void pattrib(Object o, struct prt_info *p)
{
    Object subo;
    char *name;
    int i;

#if 0 
    int oldr;

    oldr = p->recurse;
    p->recurse = 0;
#endif

    for(i=0; (subo=DXGetEnumeratedAttribute(o, i, &name)); i++) {
	INDENT(p->level);
	DXMessage("Attribute.  Name '%s':\n", name);
	p->level++;
	Object_Print(subo, p);
	p->level--;
    }

#if 0
    p->recurse = oldr;
#endif

}
