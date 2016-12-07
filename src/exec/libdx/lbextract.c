/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>




#include <string.h>
#include <dx/dx.h>


/*
 * simple routines for common cases
 */  
Object DXExtractInteger(Object o, int *ip)
{
    return DXExtractParameter(o, TYPE_INT, 0, 1, (Pointer)ip);
}

Object DXExtractFloat(Object o, float *fp)
{
    return DXExtractParameter(o, TYPE_FLOAT, 0, 1, (Pointer)fp);
}

/* slightly different - it returns a pointer to the start of the string.
 *  the caller doesn't need to allocate space for it.
 */
Object DXExtractString(Object o, char **cp)
{
    char *c;
    int len;
    Type t;

    /* this does the checking to be sure it is either a String object, 
     *  or a TYPE_STRING array.
     */
    if(!DXQueryParameter(o, TYPE_STRING, 1, NULL))
	return NULL;

    
    if(DXGetObjectClass(o) == CLASS_STRING) {
	if (cp)
	    *cp = DXGetString((String)o);
	return o;
    }
    
    DXGetArrayInfo((Array) o, &len, &t, NULL, NULL, NULL);
    
    if (t == TYPE_STRING) {
	if (len > 1)
	    return NULL;

	if (cp)
	    *cp = (char *)DXGetArrayData((Array)o);    
	return o;
    }

    /* for backwards compatibility - TYPE_UBYTE */
    c = (char *)DXGetArrayData((Array)o);
    
    while (--len > 0) {
	
	if (*c++)      /* fail if there are imbedded NULLs... */
	    continue;
	
	return NULL;
    }

    if (*c)            /* ...or if there isn't a trailing NULL */
	return NULL;
    
    if (cp)
	*cp = (char *)DXGetArrayData((Array)o);    
    return o;
}


Object DXExtractNthString (Object o, int n, char **cp)
{
    char *c, *cs;
    int	len;
    Type type;
    int shape;

    if (n < 0)
	return (NULL);

    /* only returns ok if object class String or Array TYPE_STRING or UBYTE */
    if (! DXQueryParameter (o, TYPE_STRING, 1, NULL))
	return (NULL);

    if (DXGetObjectClass(o) == CLASS_STRING) {

	/* String objects are 1 string, so asking for any string 
	 *  but the 0th is bad. 
	 */
	if (n)
	    return (NULL);
	
	if (cp)
	    *cp = DXGetString ((String) o);
	return (o);
    }

    DXGetArrayInfo ((Array) o, &len, &type, NULL, NULL, NULL);
    if (type == TYPE_STRING) {

	/* len is number of strings in array */
	if (n >= len)
	    return NULL;

	/* the Query call above made sure this is rank 1, 
	 * so shape is safe here 
	 */
	DXGetArrayInfo ((Array) o, NULL, NULL, NULL, NULL, &shape);
	cs = (char *) DXGetArrayData ((Array) o);

	if (cp)
	    *cp = cs + (n * shape);
	return o;
    }

    /* here for backward compatibility - 
     * null separated chars in a UBYTE array 
     */
    cs = (char *) DXGetArrayData ((Array) o);
    
    for (c=cs; len--; ) {
	
	if (*c++)
	    continue;
	
	if (n == 0) {
	    if (cp)
		*cp = cs;
	    return (o);
	}
	
	n--;
	cs = c;
    }
    
    return (NULL);
}


/* new code */

#define CONVTYPE(from, to) \
static Error 					\
from##2##to(int count, from *fp, to *tp)  	\
{                                               \
    int i;					\
						\
    for (i=0; i<count; i++) 			\
	*tp++ = (to) *fp++; 			\
						\
    return OK; 					\
}

CONVTYPE(byte, short)
CONVTYPE(byte, int)
CONVTYPE(byte, float)
CONVTYPE(byte, double)
CONVTYPE(ubyte, short )
CONVTYPE(ubyte, ushort)
CONVTYPE(ubyte, int)
CONVTYPE(ubyte, uint)
CONVTYPE(ubyte, float)
CONVTYPE(ubyte, double)
CONVTYPE(short, int)
CONVTYPE(short, float)
CONVTYPE(short, double)
CONVTYPE(ushort, int)
CONVTYPE(ushort, uint)
CONVTYPE(ushort, float)
CONVTYPE(ushort, double)
CONVTYPE(int, float)
CONVTYPE(int, double)
CONVTYPE(uint, float)
CONVTYPE(uint, double)
CONVTYPE(float, double)


struct convtable {
    int conversion;                        /* convert flags */
    Error (*convfunc)();                   /* convert routine */
                     /* parms are: (int count, Pointer from, Pointer to); */
    int hascommon;                         /* has common representation? */
    Type commontype;                       /* smallest common representation */
};

/* conversion flags */
#define CONV_ILLEGAL  0   /* types don't convert without truncation */
#define CONV_ALLOWED  1   /* conversion allowed */
#define CONV_EQUAL    2   /* types already same */
#define CONV_NOTIMPL  3   /* type not implemented */
#define CONV_LOSSY    4   /* conversion may lose precision (not supported) */

/* table indices for conversion function table */
#define NUM_TYPES   11

#define C_BYTE      0
#define C_UBYTE     1
#define C_SHORT     2
#define C_USHORT    3
#define C_INT       4
#define C_UINT      5
#define C_HYPER     6   /* currently unsupported */
#define C_UHYPER    7   /* currently unsupported */
#define C_FLOAT     8
#define C_DOUBLE    9
#define C_LDOUBLE  10   /* currently unsupported */

#define TYPE_XXXX   TYPE_BYTE  /* fill in value for unimplemented types */

/* hascommon flags */
#define NO_COMMON   0
#define HAS_COMMON  1


static char *typename(Type t)
{
    switch (t) {
      case TYPE_BYTE:		return "byte";
      case TYPE_UBYTE:		return "ubyte";
      case TYPE_SHORT:		return "short";
      case TYPE_USHORT:		return "ushort";
      case TYPE_INT:		return "int";
      case TYPE_UINT:		return "uint";
      case TYPE_HYPER:		return "hyper";
/*    case TYPE_UHYPER:		return "uhyper";  */
      case TYPE_FLOAT:		return "float";
      case TYPE_DOUBLE:		return "double";
/*    case TYPE_LDOUBLE:	return "ldouble"; */
      default:			return "unknown";
    }
    /* notreached */
}

static int typeindex(Type t)
{
    switch (t) {
      case TYPE_BYTE:		return C_BYTE;
      case TYPE_UBYTE:		return C_UBYTE;
      case TYPE_SHORT:		return C_SHORT;
      case TYPE_USHORT:		return C_USHORT;
      case TYPE_INT:		return C_INT;
      case TYPE_UINT:		return C_UINT;
      case TYPE_HYPER:		return C_HYPER;
/*    case TYPE_UHYPER:		return C_UHYPER;  */
      case TYPE_FLOAT:		return C_FLOAT;
      case TYPE_DOUBLE:		return C_DOUBLE;
/*    case TYPE_LDOUBLE:	return C_LDOUBLE; */
      default:			return -1;
    }
    /* notreached */
}

static struct convtable typetable[NUM_TYPES][NUM_TYPES] = 
{
 /* from BYTE */			                           /* to ... */ 
 { { CONV_EQUAL,      NULL,   	    HAS_COMMON,     TYPE_BYTE },     /* BYTE */
   { CONV_ILLEGAL,    NULL,  	     NO_COMMON,     TYPE_BYTE },     /* UBYTE */
   { CONV_ALLOWED,    byte2short,   HAS_COMMON,     TYPE_SHORT },    /* SHORT */
   { CONV_ILLEGAL,    NULL,  	     NO_COMMON,     TYPE_BYTE },     /* USHORT */
   { CONV_ALLOWED,    byte2int,     HAS_COMMON,     TYPE_INT },      /* INT */
   { CONV_ILLEGAL,    NULL,  	     NO_COMMON,     TYPE_BYTE },     /* UINT */
   { CONV_NOTIMPL,    NULL,  	    HAS_COMMON,     TYPE_HYPER },    /* HYPER */
   { CONV_NOTIMPL,    NULL,  	     NO_COMMON,     TYPE_BYTE },     /* UHYPER */
   { CONV_ALLOWED,    byte2float,   HAS_COMMON,     TYPE_FLOAT },    /* FLOAT */
   { CONV_ALLOWED,    byte2double,  HAS_COMMON,     TYPE_DOUBLE },   /* DOUBLE */
   { CONV_NOTIMPL,    NULL, 	    HAS_COMMON,     TYPE_XXXX },  }, /* LDOUBLE */

 /* from UBYTE */			                           /* to ... */ 
 { { CONV_ILLEGAL,    NULL,  	     NO_COMMON,     TYPE_BYTE },     /* BYTE */
   { CONV_EQUAL,	    NULL,   	    HAS_COMMON,     TYPE_UBYTE },    /* UBYTE */
   { CONV_ALLOWED,    ubyte2short,    HAS_COMMON,     TYPE_SHORT },    /* SHORT */
   { CONV_ALLOWED,    ubyte2ushort,   HAS_COMMON,     TYPE_USHORT },   /* USHORT */
   { CONV_ALLOWED,    ubyte2int,      HAS_COMMON,     TYPE_INT },      /* INT */
   { CONV_ALLOWED,    ubyte2uint,     HAS_COMMON,     TYPE_UINT },     /* UINT */
   { CONV_NOTIMPL,    NULL,  	    HAS_COMMON,     TYPE_HYPER },    /* HYPER */
   { CONV_NOTIMPL,    NULL,  	    HAS_COMMON,     TYPE_XXXX },     /* UHYPER */
   { CONV_ALLOWED,    ubyte2float,    HAS_COMMON,     TYPE_FLOAT },    /* FLOAT */
   { CONV_ALLOWED,    ubyte2double,   HAS_COMMON,     TYPE_DOUBLE },   /* DOUBLE */
   { CONV_NOTIMPL,    NULL, 	    HAS_COMMON,     TYPE_XXXX },  }, /* LDOUBLE */

 /* from SHORT */			                           /* to ... */ 
 { { CONV_ILLEGAL,    NULL,  	    HAS_COMMON,     TYPE_SHORT },    /* BYTE */
   { CONV_ILLEGAL,    NULL,  	    HAS_COMMON,     TYPE_SHORT },    /* UBYTE */
   { CONV_EQUAL,	    NULL,   	    HAS_COMMON,     TYPE_SHORT },    /* SHORT */
   { CONV_ILLEGAL,    NULL,  	     NO_COMMON,     TYPE_BYTE },     /* USHORT */
   { CONV_ALLOWED,    short2int,      HAS_COMMON,     TYPE_INT },      /* INT */
   { CONV_ILLEGAL,    NULL,  	     NO_COMMON,     TYPE_BYTE },     /* UINT */
   { CONV_NOTIMPL,    NULL,  	    HAS_COMMON,     TYPE_HYPER },    /* HYPER */
   { CONV_NOTIMPL,    NULL,  	     NO_COMMON,     TYPE_BYTE },     /* UHYPER */
   { CONV_ALLOWED,    short2float,    HAS_COMMON,     TYPE_FLOAT },    /* FLOAT */
   { CONV_ALLOWED,    short2double,   HAS_COMMON,     TYPE_DOUBLE },   /* DOUBLE */
   { CONV_NOTIMPL,    NULL, 	    HAS_COMMON,     TYPE_XXXX },  }, /* LDOUBLE */

 /* from USHORT */			                           /* to ... */ 
 { { CONV_ILLEGAL,    NULL,  	     NO_COMMON,     TYPE_BYTE },     /* BYTE */
   { CONV_ILLEGAL,    NULL,  	    HAS_COMMON,     TYPE_USHORT },   /* UBYTE */
   { CONV_ILLEGAL,    NULL,  	     NO_COMMON,     TYPE_BYTE },     /* SHORT */
   { CONV_EQUAL,	    NULL,   	    HAS_COMMON,     TYPE_USHORT },   /* USHORT */
   { CONV_ALLOWED,    ushort2int,     HAS_COMMON,     TYPE_INT },      /* INT */
   { CONV_ALLOWED,    ushort2uint,    HAS_COMMON,     TYPE_UINT },     /* UINT */
   { CONV_NOTIMPL,    NULL,  	    HAS_COMMON,     TYPE_HYPER },    /* HYPER */
   { CONV_NOTIMPL,    NULL,  	    HAS_COMMON,     TYPE_XXXX },     /* UHYPER */
   { CONV_ALLOWED,    ushort2float,   HAS_COMMON,     TYPE_FLOAT },    /* FLOAT */
   { CONV_ALLOWED,    ushort2double,  HAS_COMMON,     TYPE_DOUBLE },   /* DOUBLE */
   { CONV_NOTIMPL,    NULL, 	    HAS_COMMON,     TYPE_XXXX }, },  /* LDOUBLE */

 /* from INT */			                                   /* to ... */ 
 { { CONV_ILLEGAL,    NULL,  	    HAS_COMMON,     TYPE_INT },      /* BYTE */
   { CONV_ILLEGAL,    NULL,  	    HAS_COMMON,     TYPE_INT },      /* UBYTE */
   { CONV_ILLEGAL,    NULL,  	    HAS_COMMON,     TYPE_INT },      /* SHORT */
   { CONV_ILLEGAL,    NULL,  	    HAS_COMMON,     TYPE_INT },      /* USHORT */
   { CONV_EQUAL,	    NULL,   	    HAS_COMMON,     TYPE_INT },      /* INT */
   { CONV_ILLEGAL,    NULL,  	     NO_COMMON,     TYPE_BYTE },     /* UINT */
   { CONV_NOTIMPL,    NULL,  	    HAS_COMMON,     TYPE_HYPER },    /* HYPER */
   { CONV_NOTIMPL,    NULL,  	     NO_COMMON,     TYPE_BYTE },     /* UHYPER */
   { CONV_ALLOWED,    int2float,      HAS_COMMON,     TYPE_FLOAT },    /* FLOAT */
   { CONV_ALLOWED,    int2double,     HAS_COMMON,     TYPE_DOUBLE },   /* DOUBLE */
   { CONV_NOTIMPL,    NULL,	    HAS_COMMON,     TYPE_XXXX },  }, /* LDOUBLE */

 /* from UINT */			                           /* to ... */ 
 { { CONV_ILLEGAL,    NULL,  	     NO_COMMON,     TYPE_BYTE },     /* BYTE */
   { CONV_ILLEGAL,    NULL,  	    HAS_COMMON,     TYPE_UBYTE },    /* UBYTE */
   { CONV_ILLEGAL,    NULL,  	     NO_COMMON,     TYPE_BYTE },     /* SHORT */
   { CONV_ILLEGAL,    NULL,  	    HAS_COMMON,     TYPE_USHORT },   /* USHORT */
   { CONV_ILLEGAL,    NULL,  	     NO_COMMON,     TYPE_BYTE },     /* INT */
   { CONV_EQUAL,	    NULL,   	    HAS_COMMON,     TYPE_UINT },     /* UINT */
   { CONV_NOTIMPL,    NULL,  	    HAS_COMMON,     TYPE_HYPER },    /* HYPER */
   { CONV_NOTIMPL,    NULL,  	    HAS_COMMON,     TYPE_XXXX },     /* UHYPER */
   { CONV_ALLOWED,    uint2float,     HAS_COMMON,     TYPE_FLOAT },    /* FLOAT */
   { CONV_ALLOWED,    uint2double,    HAS_COMMON,     TYPE_DOUBLE },   /* DOUBLE */
   { CONV_NOTIMPL,    NULL, 	    HAS_COMMON,     TYPE_XXXX }, },  /* LDOUBLE */

 /* from HYPER */			                           /* to ... */ 
 { { CONV_NOTIMPL,    NULL,   	    HAS_COMMON,     TYPE_HYPER },    /* BYTE */
   { CONV_NOTIMPL,    NULL,  	    HAS_COMMON,     TYPE_HYPER },    /* UBYTE */
   { CONV_NOTIMPL,    NULL,  	    HAS_COMMON,     TYPE_HYPER },    /* SHORT */
   { CONV_NOTIMPL,    NULL,  	    HAS_COMMON,     TYPE_HYPER },    /* USHORT */
   { CONV_NOTIMPL,    NULL,  	    HAS_COMMON,     TYPE_HYPER },    /* INT */
   { CONV_NOTIMPL,    NULL,  	    HAS_COMMON,     TYPE_HYPER },    /* UINT */
   { CONV_EQUAL,	    NULL,  	    HAS_COMMON,     TYPE_HYPER },    /* HYPER */
   { CONV_NOTIMPL,    NULL,  	     NO_COMMON,     TYPE_BYTE },     /* UHYPER */
   { CONV_NOTIMPL,    NULL,  	    HAS_COMMON,     TYPE_FLOAT },    /* FLOAT */
   { CONV_NOTIMPL,    NULL,  	    HAS_COMMON,     TYPE_DOUBLE },   /* DOUBLE */
   { CONV_NOTIMPL,    NULL, 	    HAS_COMMON,     TYPE_XXXX }, },  /* LDOUBLE */

 /* from UHYPER */			                           /* to ... */ 
 { { CONV_NOTIMPL,    NULL,   	     NO_COMMON,     TYPE_BYTE },     /* BYTE */
   { CONV_NOTIMPL,    NULL,  	    HAS_COMMON,     TYPE_HYPER },    /* UBYTE */
   { CONV_NOTIMPL,    NULL,  	     NO_COMMON,     TYPE_BYTE },     /* SHORT */
   { CONV_NOTIMPL,    NULL,  	    HAS_COMMON,     TYPE_HYPER },    /* USHORT */
   { CONV_NOTIMPL,    NULL,  	     NO_COMMON,     TYPE_BYTE },     /* INT */
   { CONV_NOTIMPL,    NULL,  	    HAS_COMMON,     TYPE_HYPER },    /* UINT */
   { CONV_NOTIMPL,    NULL,  	     NO_COMMON,     TYPE_BYTE },     /* HYPER */
   { CONV_EQUAL,	    NULL,  	    HAS_COMMON,     TYPE_XXXX },     /* UHYPER */
   { CONV_NOTIMPL,    NULL,  	    HAS_COMMON,     TYPE_FLOAT },    /* FLOAT */
   { CONV_NOTIMPL,    NULL,  	    HAS_COMMON,     TYPE_DOUBLE },   /* DOUBLE */
   { CONV_NOTIMPL,    NULL, 	    HAS_COMMON,     TYPE_XXXX }, },  /* LDOUBLE */

 /* from FLOAT */			                           /* to ... */ 
 { { CONV_ILLEGAL,    NULL,   	    HAS_COMMON,     TYPE_FLOAT },    /* BYTE */
   { CONV_ILLEGAL,    NULL,  	    HAS_COMMON,     TYPE_FLOAT },    /* UBYTE */
   { CONV_ILLEGAL,    NULL,  	    HAS_COMMON,     TYPE_FLOAT },    /* SHORT */
   { CONV_ILLEGAL,    NULL,  	    HAS_COMMON,     TYPE_FLOAT },    /* USHORT */
   { CONV_ILLEGAL,    NULL,  	    HAS_COMMON,     TYPE_FLOAT },    /* INT */
   { CONV_ILLEGAL,    NULL,  	    HAS_COMMON,     TYPE_FLOAT },    /* UINT */
   { CONV_NOTIMPL,    NULL,  	    HAS_COMMON,     TYPE_FLOAT },    /* HYPER */
   { CONV_NOTIMPL,    NULL,  	    HAS_COMMON,     TYPE_FLOAT },    /* UHYPER */
   { CONV_EQUAL,	    NULL,  	    HAS_COMMON,     TYPE_FLOAT },    /* FLOAT */
   { CONV_ALLOWED,    float2double,   HAS_COMMON,     TYPE_DOUBLE },   /* DOUBLE */
   { CONV_NOTIMPL,    NULL, 	    HAS_COMMON,     TYPE_XXXX },  }, /* LDOUBLE */

 /* from DOUBLE */			                           /* to ... */ 
 { { CONV_ILLEGAL,    NULL,   	    HAS_COMMON,     TYPE_DOUBLE },   /* BYTE */
   { CONV_ILLEGAL,    NULL,  	    HAS_COMMON,     TYPE_DOUBLE },   /* UBYTE */
   { CONV_ILLEGAL,    NULL,  	    HAS_COMMON,     TYPE_DOUBLE },   /* SHORT */
   { CONV_ILLEGAL,    NULL,  	    HAS_COMMON,     TYPE_DOUBLE },   /* USHORT */
   { CONV_ILLEGAL,    NULL,  	    HAS_COMMON,     TYPE_DOUBLE },   /* INT */
   { CONV_ILLEGAL,    NULL,  	    HAS_COMMON,     TYPE_DOUBLE },   /* UINT */
   { CONV_NOTIMPL,    NULL,  	    HAS_COMMON,     TYPE_DOUBLE },   /* HYPER */
   { CONV_NOTIMPL,    NULL,  	    HAS_COMMON,     TYPE_DOUBLE },   /* UHYPER */
   { CONV_ILLEGAL,    NULL,  	    HAS_COMMON,     TYPE_DOUBLE },   /* FLOAT */ 
   { CONV_EQUAL,	    NULL,  	    HAS_COMMON,     TYPE_DOUBLE },   /* DOUBLE */
   { CONV_NOTIMPL,    NULL, 	    HAS_COMMON,     TYPE_XXXX },  }, /* LDOUBLE */

 /* from LDOUBLE */			                           /* to ... */ 
 { { CONV_NOTIMPL,    NULL,   	    HAS_COMMON,     TYPE_XXXX },     /* BYTE */
   { CONV_NOTIMPL,    NULL,  	    HAS_COMMON,     TYPE_XXXX },     /* UBYTE */
   { CONV_NOTIMPL,    NULL,  	    HAS_COMMON,     TYPE_XXXX },     /* SHORT */
   { CONV_NOTIMPL,    NULL,  	    HAS_COMMON,     TYPE_XXXX },     /* USHORT */
   { CONV_NOTIMPL,    NULL,  	    HAS_COMMON,     TYPE_XXXX },     /* INT */
   { CONV_NOTIMPL,    NULL,  	    HAS_COMMON,     TYPE_XXXX },     /* UINT */
   { CONV_NOTIMPL,    NULL,  	    HAS_COMMON,     TYPE_XXXX },     /* HYPER */
   { CONV_NOTIMPL,    NULL,  	    HAS_COMMON,     TYPE_XXXX },     /* UHYPER */
   { CONV_NOTIMPL,    NULL,  	    HAS_COMMON,     TYPE_XXXX },     /* FLOAT */ 
   { CONV_NOTIMPL,    NULL,  	    HAS_COMMON,     TYPE_XXXX },     /* DOUBLE */
   { CONV_NOTIMPL,    NULL, 	    HAS_COMMON,     TYPE_XXXX },  }, /* LDOUBLE */
};


/* these could probably be macro-ized somehow */
/* THESE MUST WORK INPLACE!!! */

#define INC(ptr, size)  (Pointer)((long)(ptr) + (size))
#define DEC(ptr, size)  (Pointer)((long)(ptr) - (size))

/* conversion is:  a -> a + 0i */
static Error real2complex(int count, Pointer from, Pointer to, int itemsize)
{
    int i;
    Pointer efrom, eto;

    efrom = INC(from, itemsize * (count-1));
    eto = INC(to, 2 * itemsize * (count-1));
    for (i=count; i>0; i--) {
	memset(eto, '\0', itemsize);
	eto = DEC(to, itemsize);
	memcpy(efrom, eto, itemsize);
	efrom = DEC(from, itemsize);
	eto = DEC(to, itemsize);
    }

    return OK;
}

/* conversion is:  a -> a + 0i + 0j + 0k */
static Error real2quatern(int count, Pointer from, Pointer to, int itemsize)
{
    int i;

    /*efrom = INC(from, itemsize * (count-1));*/
    /*eto = INC(to, 4 * itemsize * (count-1));*/
    for (i=count; i>0; i--) {
	memset(to, '\0', 3 * itemsize);
	to = DEC(to, 3 * itemsize);
	memcpy(from, to, itemsize);
	from = DEC(from, itemsize);
	to = DEC(to, itemsize);
    }

    return OK;
}

/* conversion is:  a + bi  ->  a + bi + 0j + 0k */
static Error complex2quatern(int count, Pointer from, Pointer to, int itemsize)
{
    int i;

    /*efrom = INC(from, itemsize * (count-1));*/
    /*eto = INC(from, 2 * itemsize * (count-1));*/
    for (i=count; i>0; i--) {
	memset(to, '\0', itemsize);
	to = DEC(to, itemsize);
	memcpy(from, to, itemsize);
	from = DEC(from, itemsize);
	to = DEC(to, itemsize);
    }

    return OK;
}


struct convtable2 {
    int conversion;
    Error (*convfunc)(int count, Pointer from, Pointer to, int itemsize);
    int hascommon;
    Category commoncat;
};

/* (uses same conversion flags as above) */

#define NUM_CATS    3

#define C_REAL	    0
#define C_COMPLEX   1
#define C_QUATERN   2

static char *catname(Category c)
{
    switch (c) {
      case CATEGORY_REAL:	return "real";
      case CATEGORY_COMPLEX:	return "complex";
      case CATEGORY_QUATERNION:	return "quaternion";
      default:			return "unknown";
    }
    /* notreached */
}

static int catindex(Category c)
{
    switch (c) {
      case CATEGORY_REAL:	return C_REAL;
      case CATEGORY_COMPLEX:	return C_COMPLEX;
      case CATEGORY_QUATERNION:	return C_QUATERN;
      default:			return -1;
    }
    /* notreached */
}


static struct convtable2 cattable[NUM_CATS][NUM_CATS] = 
{
 /* from REAL */			                                /* to ... */ 
 { { CONV_EQUAL,  NULL, 	 HAS_COMMON,  CATEGORY_REAL },          /* REAL */
   { CONV_ALLOWED, real2complex, HAS_COMMON,  CATEGORY_COMPLEX },       /* COMPLEX */
   { CONV_ALLOWED, real2quatern, HAS_COMMON,  CATEGORY_QUATERNION }, }, /* QUATERN */

 /* from COMPLEX */		 	                                /* to ... */ 
 { { CONV_ILLEGAL,  NULL,   	 HAS_COMMON,  CATEGORY_COMPLEX },       /* REAL */
   { CONV_EQUAL,  NULL,  	 HAS_COMMON,  CATEGORY_COMPLEX },       /* COMPLEX */
   { CONV_ALLOWED, complex2quatern, HAS_COMMON, CATEGORY_QUATERNION }, }, /* QUATERN */

 /* from QUATERN */			                                /* to ... */ 
 { { CONV_ILLEGAL,  NULL,   	HAS_COMMON,  CATEGORY_QUATERNION },     /* REAL */
   { CONV_ILLEGAL,  NULL,  	HAS_COMMON,  CATEGORY_QUATERNION },     /* COMPLEX */
   { CONV_EQUAL,  NULL,  	HAS_COMMON,  CATEGORY_QUATERNION }, },  /* QUATERN */
};

/* this modifies shape in place to squeeze out shapes of 1
 *  (e.g. rank 3, shape 3 1 4 ->  rank 2 shape 3 4)
 */
static void SqueezeShape(int *rank, int *shape)
{
    int i, j;

    i = 0;
    while (i < *rank) {
	if (shape[i] > 1)
	    i++;
	else {
            (*rank)--;
            for (j=i; j<*rank; j++)
                shape[j] = shape[j+1];
        } 
    }
  
}

/* this checks to see if it can squeeze the first rank/shape pair
 *  to match the second, and if it can, it modifies them in place
 *  and returns OK.  otherwise it leaves them alone and returns ERROR
 *  and sets the error code. 
 * if it has to expand to match, it adds shape 1 at the end of the list.
 *
 * this routine isn't done yet.
 */

#if 0
static Error SqueezeToShape(int *rank, int *shape, int trank, int *tshape)
{
    int i, j;

    i = 0;
    while (i < *rank) {
	if (shape[i] > 1)
	    i++;
	else {
            (*rank)--;
            for (j=i; j<*rank; j++)
                shape[j] = shape[j+1];
        } 
    }

    return ERROR;
}
#endif

static Error
ConvertArrayContents(Array from, Array to, int squeeze)
{
    int i;
    int fi, ti;
    Pointer fromdp, todp;
    int count;
    int fromcount, tocount;
    Type fromtype, totype;
    Category fromcat, tocat;
    int fromrank, torank;
    int fromshape[100], toshape[100];

    DXGetArrayInfo(from, &fromcount, &fromtype, &fromcat, &fromrank, fromshape);
    DXGetArrayInfo(to, &tocount, &totype, &tocat, &torank, toshape);

    if (fromcount != tocount)
	DXErrorReturn(ERROR_DATA_INVALID, "array sizes don't match");


    /* squeeze out shapes of 1 from both and make sure they match. */
    if (squeeze) {
	SqueezeShape(&fromrank, fromshape);
	SqueezeShape(&torank, toshape);
    }

    if (fromrank != torank) {
	if (squeeze)
	    DXErrorReturn(ERROR_DATA_INVALID, "data ranks don't match");

	return ConvertArrayContents(from, to, 1);
    }

    for (i=0; i<fromrank; i++)
	if (fromshape[i] != toshape[i]) {
	    if (squeeze)
		DXErrorReturn(ERROR_DATA_INVALID, "data shapes don't match");

	    return ConvertArrayContents(from, to, 1);
	}


    if (((fi = typeindex(fromtype)) < 0) || ((ti = typeindex(totype)) < 0))
        DXErrorReturn(ERROR_NOT_IMPLEMENTED, "unsupported data type");
    
    fromdp = DXGetArrayData(from);
    todp = DXGetArrayData(to);

    switch (typetable[fi][ti].conversion) {
      case CONV_EQUAL:
        memcpy(todp, fromdp, fromcount * DXGetItemSize(from));
        break;
        
      case CONV_ALLOWED:
	count = fromcount * (DXGetItemSize(from) / DXTypeSize(fromtype));
        if ((typetable[fi][ti].convfunc)(count, fromdp, todp) == ERROR)
            DXErrorReturn(ERROR_DATA_INVALID, "type conversion failed");
        
        break;
        
      case CONV_NOTIMPL:
      default:
        DXErrorReturn(ERROR_NOT_IMPLEMENTED, "unsupported data type");
        
      case CONV_ILLEGAL:
      case CONV_LOSSY:
        DXSetError(ERROR_DATA_INVALID, "cannot convert %s to %s", 
                   typename(fromtype), typename(totype));
        return ERROR;
        
    }
    
    if (((fi = catindex(fromcat)) < 0) || ((ti = catindex(tocat)) < 0))
        DXErrorReturn(ERROR_NOT_IMPLEMENTED, "unsupported data category");
    
    switch (cattable[fi][ti].conversion) {
      case CONV_EQUAL:
        break;
        
      case CONV_ALLOWED:
	count = fromcount * (DXGetItemSize(from) / DXCategorySize(fromcat));
        if ((cattable[fi][ti].convfunc)(count, todp, todp, 
                                        DXGetItemSize(from)) == ERROR)
            DXErrorReturn(ERROR_DATA_INVALID, "category conversion failed");
        
        break;
        
      case CONV_NOTIMPL:
      default:
        DXErrorReturn(ERROR_NOT_IMPLEMENTED, "unsupported data category");
        
      case CONV_ILLEGAL:
      case CONV_LOSSY:
        DXSetError(ERROR_NOT_IMPLEMENTED, "cannot convert %s to %s", 
                   catname(fromcat), catname(tocat));
        return ERROR;
    }

    return OK;
}


static Array 
ArrayConvert(Array a, Type t, Category c, int rank, int *shape)
{
    int i;
    Array out;

    ASSERT(rank<100);

    out = DXNewArrayV(t, c, rank, shape);
    
    DXGetArrayInfo(a, &i, NULL, NULL, NULL, NULL);
    if (!DXAddArrayData(out, 0, i, NULL))
	return NULL;

    if (!ConvertArrayContents(a, out, 0)) {
	DXDelete((Object)out);
	return NULL;
    }

    return out;
}


Array 
DXArrayConvert(Array a, Type t, Category c, int rank, ... )
{
    int i;
    int shape[100];
    va_list arg;

    ASSERT(rank<100);

    /* collect args */
    va_start(arg,rank);       /* no spaces allowed - this is a weird macro */
    for (i=0; i<rank; i++)
	shape[i] = va_arg(arg, int);
    va_end(arg);

    /* common code with V version. */
    return ArrayConvert(a, t, c, rank, shape);
}


Array 
DXArrayConvertV(Array a, Type t, Category c, int rank, int *shape)
{
    return ArrayConvert(a, t, c, rank, shape);
}


/* sees if there is a common format and returns it.  otherwise returns
 *  NULL but and sets error code.
 */
static Error
QueryArrayCommon(Type *t, Category *c, int *rank, int *shape, int n, 
		 Array *a, int squeeze)
{
    int i, j;
    int ni, cti, cci;
    Type nextt, comt;
    Category nextc, comc;
    int nextrank, comrank;
    int nextshape[100], comshape[100];

    ASSERT(n<100);

    if (n <= 0) {
	DXSetError(ERROR_DATA_INVALID, 
                   "array list count must be a positive integer");
	return ERROR;
    }

    if (!DXGetArrayInfo(a[0], NULL, &comt, &comc, &comrank, comshape))
	return ERROR;

    if (squeeze)
	SqueezeShape(&comrank, comshape);

  
    /* if the values don't match, see if they are compatible, and use
     *  the minimum necessary changes.
     */
    for (i=1; i<n; i++) {

	if (!DXGetArrayInfo(a[i], NULL, &nextt, &nextc, &nextrank, nextshape))
	    return ERROR;
        
	/* data type */
	if (comt != nextt) {
            if ((cti = typeindex(comt)) < 0)
                DXErrorReturn(ERROR_NOT_IMPLEMENTED, "unsupported data type");
    
            if ((ni = typeindex(nextt)) < 0)
                DXErrorReturn(ERROR_NOT_IMPLEMENTED, "unsupported data type");
	
            switch (typetable[cti][ni].hascommon) {
              case HAS_COMMON:
                comt = typetable[cti][ni].commontype;
                break;
	    
              case NO_COMMON:
                DXSetError(ERROR_DATA_INVALID, "no common format for %s and %s", 
                           typename(comt), typename(nextt));
                return ERROR;
            }
            
        }

        /* data category */
        if (comc != nextc) {
            if ((cci = catindex(comc)) < 0)
                DXErrorReturn(ERROR_NOT_IMPLEMENTED, "unsupported data category");
    
            if ((ni = catindex(nextc)) < 0) 
                DXErrorReturn(ERROR_NOT_IMPLEMENTED, "unsupported data category");
            
            switch (cattable[cci][ni].hascommon) {
              case HAS_COMMON:
                comc = cattable[cci][ni].commoncat;	    
                break;
	    
              case NO_COMMON:
                DXSetError(ERROR_DATA_INVALID, "no common format for %s and %s", 
                           catname(comc), catname(nextc));
                return ERROR;
            }
        }
        
        /* data rank -  squeeze out shapes of 1 */
	if (squeeze)
	    SqueezeShape(&nextrank, nextshape);
  
	if (comrank != nextrank) {
	    if (squeeze) {
		DXSetError(ERROR_DATA_INVALID, 
			   "data rank does not match (%d != %d)", 
			   comrank, nextrank);
		return ERROR;
	    }

	    return QueryArrayCommon(t, c, rank, shape, n, a, 1);
	}



	for (j=0; j<comrank; j++) {
	    if (comshape[j] != nextshape[j]) {
		if (squeeze) {
		    DXSetError(ERROR_DATA_INVALID, 
			       "data shape %d does not match (%d != %d)", 
			       j, comshape[j], nextshape[j]);
		    
		    return ERROR;
		}

		
		return QueryArrayCommon(t, c, rank, shape, n, a, 1);
	    }
	    
	}
    }


    /* ok - finally.  return the results */
    if (t)
	*t = comt;
    if (c)
	*c = comc;
    if (rank)
	*rank = comrank;
    if (shape) {
	for (i=0; i<comrank; i++)
	    shape[i] = comshape[i];
    }
    return OK;
}

Error 
DXQueryArrayCommon(Type *t, Category *c, int *rank, int *shape, 
		    int n, Array a, ...) 
{
    int i;
    Array alist[100];
    va_list arg;

    ASSERT(n<100);

    /* collect args */
    va_start(arg,a);       /* no spaces allowed - this is a weird macro */
    for (i=0; i<n; i++)
	alist[i] = va_arg(arg, Array);
    va_end(arg);

    /* common code with V version. */
    return QueryArrayCommon(t, c, rank, shape, n, alist, 0);
}

Error 
DXQueryArrayCommonV(Type *t, Category *c, int *rank, int *shape, 
		     int n, Array *alist)
{
    return QueryArrayCommon(t, c, rank, shape, n, alist, 0);
}



/* sees if the array can be converted to the requested format.  
 *  should this set the error code??  (it does right now)
 */
static Error 
QueryArrayConvert(Array a, Type t, Category c, int rank, int *shape, int squeeze)
{
    int i;
    int ai, rti, rci;
    Type at;
    Category ac;
    int arank;
    int ashape[100], rshape[100];

    if (!DXGetArrayInfo(a, NULL, &at, &ac, &arank, ashape))
	return ERROR;

    /* data type */
    if (at != t) {
        if ((rti = typeindex(t)) < 0)
            DXErrorReturn(ERROR_NOT_IMPLEMENTED, "unsupported data type");
        
        if ((ai = typeindex(at)) < 0)
            DXErrorReturn(ERROR_NOT_IMPLEMENTED, "unsupported data type");
	
        switch (typetable[ai][rti].conversion) {
          case CONV_EQUAL:
          case CONV_ALLOWED:
            break;
	    
          default:
            DXSetError(ERROR_DATA_INVALID, "cannot convert type %s into %s", 
                       typename(at), typename(t));
            return ERROR;
        }
    }
    
    /* data category */
    if (ac != c) {
        if ((rci = catindex(c)) < 0)
            DXErrorReturn(ERROR_NOT_IMPLEMENTED, "unsupported data category");
        
        if ((ai = catindex(ac)) < 0) 
            DXErrorReturn(ERROR_NOT_IMPLEMENTED, "unsupported data category");
        
        switch (cattable[ai][rci].conversion) {
          case CONV_EQUAL:
          case CONV_ALLOWED:
            break;
	    
          default:
            DXSetError(ERROR_DATA_INVALID, "cannot convert category %s into %s", 
                       catname(ac), catname(c));
            return ERROR;
        }
        
    }

    /* get rid of 1's */
    if (squeeze) {
	SqueezeShape(&arank, ashape);
	for (i=0; i<rank; i++)
	    rshape[i] = shape[i];
	SqueezeShape(&rank, rshape);
    }
  
    if (arank != rank) {
	if (squeeze) {
	    DXSetError(ERROR_DATA_INVALID, "data rank does not match (%d > %d)", 
		       arank, rank);
	    return ERROR;
	}

	return QueryArrayConvert(a, t, c, rank, shape, 1);
    }
    
    /* data rank and shape */
    for (i=0; i<rank; i++)
        if (shape[i] != rshape[i]) {
	    if (squeeze) {
		DXSetError(ERROR_DATA_INVALID, 
			   "data shape does not match (%d != %d)", 
			   shape[i], ashape[i]);
		
		return ERROR;
	    }
	    
	    return QueryArrayConvert(a, t, c, rank, shape, 1);
	}

    return OK;
}



Error 
DXQueryArrayConvert(Array a, Type t, Category c, int rank, ...)
{
    int i;
    int shape[100];
    va_list arg;

    ASSERT(rank<100);

    /* collect args */
    va_start(arg,rank);       /* no spaces allowed - this is a weird macro */
    for (i=0; i<rank; i++)
	shape[i] = va_arg(arg, int);
    va_end(arg);

    /* common code with V version. */
    return QueryArrayConvert(a, t, c, rank, shape, 0);
}

Error 
DXQueryArrayConvertV(Array a, Type t, Category c, int rank, int *shape)
{
    return QueryArrayConvert(a, t, c, rank, shape, 0);
}



#define MAXDIM 32

/* type check an object, and return the number if a list.
 */
Object DXQueryParameter(Object o, Type t, int dim, int *count)
{
    int nitems;
    Type at;
    Category ac;
    int ar, as[MAXDIM];

    /* assume the worst */
    if(count)
	*count = 0;

    if(!o)
	return NULL;

    /* slightly special case for char arrays, which might come in
     *  as string objects.
     */
    if ((t == TYPE_STRING) && (DXGetObjectClass(o) == CLASS_STRING)) {
        
        char *cp = DXGetString((String)o);
        
        if(!cp || dim > 1)
            return NULL;
        
        if(dim == 0  &&  strlen(cp) > 1)
            return NULL;
        
        if(count)
            *count = strlen(cp);
        
        return o;
    } 
	

    /* check general characteristics first, before worrying about
     *  specific data types
     */
    if( DXGetObjectClass(o) != CLASS_ARRAY
     || !DXGetArrayInfo((Array)o, &nitems, &at, &ac, &ar, as)
     || ac != CATEGORY_REAL
     || ar > 1
     || nitems == 0)
	return NULL;

    
    /* don't do these checks for strings */
    if (at != TYPE_STRING) {
	/* if dim > 1, rank must be 1 and shape must match dim.
	 * dim 0 or 1 both match rank 1 shape 1 and rank 0
	 */
	if(dim > 1) {
	    if(ar != 1  ||  as[0] != dim)
		return NULL;
	} else {
	    if(ar > 1 || (ar == 1  &&  as[0] != 1))
		return NULL;
	}
    }
	
    if(count)
	*count = nitems;

    /* if types match exactly, you're done */
    if(at == t)
	return o;


    /* see if the types could be promoted ok.  'at' is the actual
     *  array type, 't' is what they want.
     */
    if (dim == 0) {
        if (DXQueryArrayConvert((Array)o, t, CATEGORY_REAL, 0) == OK)
            return o;
    } else {
        if (DXQueryArrayConvert((Array)o, t, CATEGORY_REAL, 1, dim) == OK)
            return o;
    }

    DXResetError();
    return NULL;
}


/* given an object, and a caller-allocated buffer, copy the input
 * to the buffer, converting type if necessary.
 */
Object DXExtractParameter(Object o, Type t, int dim, int count, Pointer p)
{
    Array na;
    char *sp;
    int nitems;
    Type at;
    Category ac;
    int ar, as[MAXDIM];

    if(!o)
	return NULL;

    /* slightly special case for string arrays, which might come in
     *  as string objects.
     */
    if(t == TYPE_STRING  &&  DXGetObjectClass(o) == CLASS_STRING) {

	sp = DXGetString((String)o);

	if(!sp || dim > 1)
	    return NULL;

	if(dim == 0  &&  strlen(sp) > 1)
		return NULL;

	if(count != strlen(sp))
	    return NULL;

	if (p)
	    strcpy((char *)p, sp);

	return o;
    }
	

    /* check general characteristics first, before worrying about
     *  specific data types
     */
    if( DXGetObjectClass(o) != CLASS_ARRAY
     || !DXGetArrayInfo((Array)o, &nitems, &at, &ac, &ar, as)
     || ac != CATEGORY_REAL
     || ar > 1
     || nitems == 0
     || nitems != count)
	return NULL;


    /* if dim > 1, rank must be 1 and shape must match dim.
     * dim 0 or 1 both match rank 1 shape 1 and rank 0
     */
    if(dim > 1) {
	if(ar != 1  ||  as[0] != dim)
	    return NULL;
    } else {
	if(ar > 1 || (ar == 1  &&  as[0] != 1))
	    return NULL;
    }


    /* if types match exactly, just memcpy the number of bytes and return */
    if(at == t) {
	if (p)
	    memcpy(p, DXGetArrayData((Array)o), 
		   nitems * DXGetItemSize((Array)o));
	return o;
    }


    /* types don't match exactly, so see if they can be converted.
     */
    if (dim == 0)
        na = DXArrayConvert((Array)o, t, CATEGORY_REAL, 0);
    else
        na = DXArrayConvert((Array)o, t, CATEGORY_REAL, 1, dim);

    if (!na) {
	DXResetError();
        return NULL;
    }

    if (p)
	memcpy(p, DXGetArrayData(na), nitems * DXGetItemSize(na));

    DXDelete((Object)na);
    return o;

}
