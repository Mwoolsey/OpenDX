/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dxmods/genimp.h,v 1.4 2000/08/24 20:04:30 davidt Exp $
 */

#include <dxconfig.h>

#ifndef _GENIMP_H_
#define _GENIMP_H_

#include  <stdio.h>
#include  <stdlib.h>
#include  <string.h>
#include  <ctype.h>
#include  <dx/dx.h>
#include  "import.h"

#define   MAX_FNAME	80	/* max length of a file name */
#define   MAX_VARS	16	/* max no. of variables */
#define   MAX_NC_DIMS   20
#define   MAX_COMPS     40      /* max no. of components */
#define   MAX_DATA_DIM	9	/* Maximum number of data dimensions */
#define   MAX_POS_DIMS	MAX_DATA_DIM	/* for field locations keyword */	
#define   MAX_DSTR	256	/* max length of data string */
#define   MAX_MARKER	80	/* Size of marker string */
#define   ATTR_LEN	128	/* max string length of an attribute */

#define   OFF		0       /* definition for flag */
#define   ON		1

#define   REAL          0       /* DEFINITIONS FOR CATEGORY */
#define   COMPLEX       1
#define   QUATERNION    2

#define   ASCII		0
#define   BINARY	1

#define   FREE		0
#define   FIX		1

#define   ROW		0
#define   COL		1

#define DEFAULT_BYTEORDER LOCAL_BYTEORDER 

typedef enum {
	IT_UNKNOWN = 0,
	IT_FIELD = 1,
	IT_RECORD = 2,
	IT_RECORD_VECTOR = 3,	
	IT_VECTOR = 4	
} interleave_type;
#define DEFAULT_INTERLEAVE IT_RECORD_VECTOR

typedef enum {
	DEP_UNKNOWN = 0,
	DEP_POSITIONS = 1,
	DEP_CONNECTIONS = 2
} dep_type;

/*
 * Retrieve the index'th type variable relative to the given pointer.
 */
#define DREF(TYPE,PTR,INDEX) (((TYPE*)(PTR))[INDEX])


typedef enum {
	REGULAR = 0,
	IRREGULAR = 1,
	REGULAR_PRODUCT = 2,
	IRREGULAR_PRODUCT = 3,
	MIXED_PRODUCT = 4,
	FIELD_PRODUCT = 5,
	DEFAULT_PRODUCT = 6
} position_type;

typedef enum {
	CONN_GRID = 0,
	CONN_POINTS = 1,
	CONN_UNKNOWN = 2
} conn_type;

typedef enum {
	SKIP_NOTHING = 0,
	SKIP_LINES = 1,
	SKIP_BYTES = 2,
	SKIP_MARKER = 3
} header_type;

struct header {
	int	size;		/* size of header (bytes or lines) */
 	header_type  type;	/* type of header */
	char marker[MAX_MARKER]; /* marker for header */
};

struct variable {          
	char	name[100]; /* field name 				   */
	int	varid;     /* fieldid , variable id  			   */
	int	dimid[20]; /* dimension id 				   */
	Type	datatype;  /* Type of values making of data    	   */
	int	databytes;  /* Size of datatype in bytes */
	dep_type datadep;  /* dependency of data */
	int	width;     /* # bytes per data. _dxd_gi_asciiformat == FIX only    */
	int	leading;   /* # of leading blanks per data line 	   */
	int	elements;  /* # of elements per data line 		   */
 struct header  rec_separat[MAX_DATA_DIM]; /* record separator per component */
	int	datadim;   /* rank                                         */
};

struct infoplace {
	struct header	begin;
	int *		skip;
	int *		width;
	Type		type;
	void *		data;
	int 		index;
	int 		format;
	ByteOrder	byteorder;
};

struct place_state {
	FILE *	fp;
	int	lines;
	int 	bytes;
};

Error _dxf_gi_InputInfoTable   (char *, char *,FILE **);
Error _dxf_gi_OpenFile         (char *, FILE **, int);
Error _dxd_gi_extract		(struct place_state *);

Object _dxf_gi_get_gen(struct parmlist *);
int _dxf_gi_read_file(void **,FILE **);
int _dxf_gi_importDatafile(void **,FILE **);
int _dxfnumGridPoints(int );
Error _dxf_gi_extract(struct place_state *,int *);

extern char *_dxd_gi_filename; /* input data file name (w/wo full paths)*/
extern int _dxd_gi_numdims; 			/* dimensionality */
extern int _dxd_gi_dimsize[MAX_POS_DIMS]; 	/* dimension sizes */
extern int _dxd_gi_series; 			/* total number of time steps */
extern int _dxd_gi_numflds;			/* total number of fields */
extern int _dxd_gi_format;			/* either ASCII or BINARY */
extern int _dxd_gi_asciiformat;	  /* either FREE or FIX for ascii file format */
extern int _dxd_gi_majority;			/* either ROW or COL */
extern interleave_type _dxd_gi_interleaving; /* type of data layout */	
extern position_type _dxd_gi_positions; /* either REGULAR or IRREGULAR or COMPACT */

extern float *_dxd_gi_dataloc ;      	 /* Position locations */
extern struct variable **_dxd_gi_var;/* pointers to variables */
extern int *_dxd_gi_whichser;              /* Series numbers to be imported */
extern int *_dxd_gi_whichflds;             /* Field numbers to be imported */
extern float *_dxd_gi_seriesvalues;        /* Series values */
extern int _dxd_gi_loc_index;              /* Index of field locations in XF array */
extern int _dxd_gi_pos_type[MAX_POS_DIMS]; /* Position type of each dimension */
extern conn_type _dxd_gi_connections;      /* Connections, grid or point keyword */
extern dep_type _dxd_gi_dependency;	      /* Data dependency */
extern ByteOrder _dxd_gi_byteorder;

extern struct header _dxd_gi_header;    /* header for header keyword */
extern struct header _dxd_gi_serseparat; /* header for series separator */
extern struct infoplace **_dxd_gi_fromfile; /* hold headerinfo placement */

FILE *_dxfopen_dxfile(char *name,char *auxname,char **outname,char *ext);
Error _dxfclose_dxfile(FILE *fp,char *filename);

#endif /* _GENIMP_H_ */
