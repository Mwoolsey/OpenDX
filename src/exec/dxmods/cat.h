/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>

#ifndef _CAT_H_
#define _CAT_H_

#include <dx/dx.h>
#include <math.h>

#define ICH InvalidComponentHandle   

/* max supported is 32 dimensional */
#define MAXRANK 32

/* number of parallel tasks * (number of processors - 1) */
#define PFACTOR 4

#define LISTJUMP 64

#define DEFAULTCOMP "data"
#define STR_DATA "data"
#define STR_POSITIONS "positions"

/* only used when computing categorize groups in parallel */
typedef struct {
    Object h;		/* the categorize itself */
    char *name;		/* the group member name */
    int didwork;	/* global copy of didwork flag */
    /* slicecnt here? */
} parinfo;

/* values for the 'XXXset' flags below - 
 *  indicates whether or how the values were determined.
 */
#define NOTSET   0	/* no value yet */
#define EXPLICIT 1	/* user-given input parameter to module */
#define BYOBJECT 2	/* statistics of input object used */

#define CAT_SCALAR    0
#define CAT_STRING    1
#define CAT_ARBITRARY 2

#define CAT_I_CASE	1
#define CAT_I_SPACE	2
#define CAT_I_LSPACE	4
#define CAT_I_RSPACE	8
#define CAT_I_LRSPACE	16
#define CAT_I_PUNCT	32

/* parm list */
typedef struct _catinfo{
    Object o;		/* use component of this object */
    char *comp_name;	/* current component name in use */
    char **comp_list;	/* component list to use */
    int sort;  /* 0 - don't sort lookup, 1 - sort lookup (default) */
    int comp_rank;
    int comp_shape[30];
    Array comp_array;	/* component array */
    Pointer comp_data;	/* array data */
    int free_data;	/* data was copied so must be freed */
    int cat_type;	/* scalar, string, arbitrary */
    Type comp_type;	/* component type */
    uint num;		/* number in list */
    int obj_size;	/* component object size, including shape and rank */
    int count;          /* number of constituent components */
    int intsize;	/* number of integers in object size */
    int remainder;	/* bytes remaining after integer division */
    Array new_comp;	/* New array component */
    Error (*cmpcat)(struct _catinfo *cp, Pointer s, Pointer t);
    int (*hashcmp)(Key searckey, Element element);
    PseudoKey (*catkey) (Key key);
    uint *cind;		/* component indices into built list. Pre-allocd full size */
    uint ncats;		/* number of categories */
    Array unique_array;	/* unique array of component values*/
    int maxparallel;    /* max number of parallel tasks; based on nproc */
    int goneparallel;	/* number of parallel tasks already spawned */
    ICH invalid;        /* if set, there are invalid data values */
    struct parinfo *p;	/* if computing in parallel, where to put the output */
} catinfo;

typedef struct {
    PseudoKey key;
    uint index;
    Pointer p;
    catinfo *cp;
} hashelement;

typedef struct {
    hashelement *ph;
    uint sortindex;
    catinfo *cp;
} sortelement;

typedef struct {
    Pointer lookup;
    Pointer value;
    uint sortindex;
    catinfo *cp;
} table_entry;

Object _dxfCategorize(Object o, char **comp_list);

Error _dxf_cat_FindCompType(catinfo *cp);
int _dxf_cat_cmp_char(catinfo *cp, Pointer s, Pointer t);
int _dxf_cat_cmp_ubyte(catinfo *cp, Pointer s, Pointer t);
int _dxf_cat_cmp_short(catinfo *cp, Pointer s, Pointer t);
int _dxf_cat_cmp_ushort(catinfo *cp, Pointer s, Pointer t);
int _dxf_cat_cmp_float(catinfo *cp, Pointer s, Pointer t);
int _dxf_cat_cmp_int(catinfo *cp, Pointer s, Pointer t);
int _dxf_cat_cmp_uint(catinfo *cp, Pointer s, Pointer t);
int _dxf_cat_cmp_double(catinfo *cp, Pointer s, Pointer t);
int _dxf_cat_cmp_str(catinfo *cp, Pointer s, Pointer t);
int _dxf_cat_cmp_any(catinfo *cp, Pointer s, Pointer t);

int _dxf_cat_ignore(char *s, int ignore);

#define INTERNALERROR DXSetError(ERROR_INTERNAL, \
		      "unexpected error, file %s line %d",  __FILE__, __LINE__)


typedef struct {
    catinfo *data;
    catinfo *lookup;
    catinfo *value;
    catinfo *dest;
    table_entry *lut;
    int ignore;
    Pointer nfValue;  /* allow explicit return val for unfound */
    int nfLen;
} lookupinfo;

#endif /* _CAT_H_ */
