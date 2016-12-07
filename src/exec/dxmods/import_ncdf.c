/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
#include <dxconfig.h>



#include <stdio.h>
#include <dx/dx.h>
#include "import.h"

#if defined(HAVE_LIBNETCDF)

#undef int8
#undef uint8
#undef int16
#undef uint16
#undef int32
#undef uint32
#undef float64

#if (defined(WIN32) && defined(NDEBUG)) || defined(_MT)
#define _HDFDLL_
#elif defined(_DEBUG)
#undef _DEBUG
#endif

#include <netcdf.h>
#include <ctype.h>
#include <string.h>
#include <math.h>


#define DEBUG 0
#define REALCODE  0  /* if 0, sends fake data type info for QueryInfo */
#define TESTQUERY 0  /* if 1, send QueryInfo object back for Import */

#define VOID    char   /* prefer void, but char if your compiler chokes */
#define STR(x)  (Object)DXNewString(x)

#define MAX_FILENAME_LEN  MAX_NC_NAME
#define MAX_VARNAME_LEN   MAX_NC_NAME
#define MAXNAME           MAX_NC_NAME
#define MAXHAND           8
#define MAXATTRSTR        16

/* attribute names in netCDF file */
#define SERIESATTRIB      "series"
#define SERIESPOSATTRIB   "seriespositions"
#define FIELDATTRIB       "field"
#define GROUPATTRIB       "group"
#define TOPOATTRIB        "connections"     
#define GEOMATTRIB        "positions"     
#define OTOPOATTRIB       "topology"     /*  "connections" */
#define OGEOMATTRIB       "geometry"     /*  "positions" */
#define COMPATTRIB        "component"    /* general purpose component */
#define ATTRATTRIB        "attribute"    /* general purpose attribute */
#define OATTRATTRIB       "attributes"   /* alternate spelling */
#define MAX_DXATTRIBUTES  9		 /* number of variable dx attributes */

/* component names in Group/Field */
#define TOPCOMPNAME       "connections"
#define GEOCOMPNAME       "positions"
#define DATCOMPNAME       "data"


#define MAXCOMPNAME       128
#define MAXRANK           16     /* number of dimensions in array */
#define MAXSHAPE          128    /* max dim of individual data item */
#define INVALID_VARID     -2
#define INVALID_DIMID	  -2

/* flags for build_poscon */
#define I_NONE            0
#define I_COMPACT_P       1
#define I_PRODUCT_P       2
#define I_IRREGULAR_P     3
#define I_REGULAR_P       4
#define I_COMPACT_C       5 
#define I_PRODUCT_C       6
#define I_IRREGULAR_C     7
#define I_REGULAR_C       8

/* flags for Object_Import */
#define IMPORT_UIQUERY    0
#define IMPORT_DATA       1
#define IMPORT_VARINFO    2

#ifndef DXD_NON_UNIX_ENV_SEPARATOR
#define PATH_SEP_CHAR ':'
#else
#define PATH_SEP_CHAR ';'
#endif

/* for collecting series groups, general groups and composite fields.
 */
struct varinfo {
    Class class;
    char name[MAXCOMPNAME];
    int cdfid;
    char cdfname[MAX_FILENAME_LEN];
    int varid;
    double position;
    int *startframe;
    int *endframe;
    int *deltaframe;
    int recdim;
    struct varinfo *child;
    struct varinfo *next;
};
typedef struct varinfo *Varinfo;    


/* for assembling arrays correctly.
 */    
struct arrayinfo {
    int cdfhandle;              /* for netCDF variable currently being read */
    int varid;
    int recdim;                 /* dim ID for unlimited (record) dimension */
    int data_isrec;             /* whether data uses record dimension */
    char stringattr[MAXNAME];
    int data_ndims;             /* data array shape */
    int datacounts[MAXRANK];
    int array_ndims;            /* current array shape */
    int arraycounts[MAXRANK];
    int arrayrank;              /* current array individual items */
    int arrayshape[MAXSHAPE];
    int arraytype;          
    Varinfo vp;                 /* for getting start/end/delta/current info */
};
typedef struct arrayinfo *Arrayinfo;

static char *dxattributes[] = {
       "field",
       "group",
       "connections",
       "positions",
       "topology",  
       "geometry", 
       "component",
       "attribute",
       "attributes"
};

static char *regularname[] = { 
    "point",        /* rank 0 */
    "lines",        /* rank 1 */ 
    "quads",        /* rank 2 */
    "cubes",        /* rank 3 */
    "cubes4D",      /* rank 4 */
    "cubes5D",      /* rank 5 */
    "cubes6D",      /* rank 6 */
    "cubes7D",      /* rank 7 */
    "cubes8D",      /* rank 8 */
    "cubes9D",      /* rank 9 */
};

/* these prototypes should be moved to import.h - they are publics */
Object _dxfQueryImportInfo(char *filename);
static Object EndImport(Object o);

/* private functions */
static Object QueryNetCDFInfo(char *filename);
static Object Object_Import(char *filename, char **varlist, int *starttime, 
			    int *endtime, int *deltatime, int importflag);

static Object import_field(int cdfhandle, Varinfo vp);
static Object query_field(int cdfhandle, Varinfo vp);
static Varinfo query_var(int cdfhandle, char **varlist, int *starttime,
			 int *endtime, int *deltatime);

static int open_netcdf_file(char *filename);
static void close_netcdf_file(int cdfhandle);
static Varinfo match_vp(Varinfo vp, int varid, char **s, char *stringattr);
static void vp_delete(Varinfo vp);

static Object build_field(int, int);
static Object build_series(int, Varinfo vp);
static Object build_poscon(Arrayinfo ap, int flag, int dim);
static Object build_data(Arrayinfo, int);
static Object build_array(Arrayinfo);
static Object build_regpos(Arrayinfo, int);
static Object build_regpos1D(Arrayinfo, int, int);
static Object build_regcon(Arrayinfo);
static Object build_regcon1D(Arrayinfo, int);

static char *getattr(int hand, int varid, char *attrname,
                    char *stringattr,int maxlen);
static char *getNattr(int hand, int varid, int i, char *attrname, int n, 
                     char *stringattr);
static Error setattr(Arrayinfo ap, Field f, char *compname);
static char *isattr(int hand, int varid, char *attrname);
static Error setuserattr(Arrayinfo ap, Field f, char *compname);
static Error getglobalattr(Object o,Varinfo vp);

static Error check_serieslength(Arrayinfo ap);
static int get_serieslength(Arrayinfo);
static int is_seriesvariable(int hand, int varid, int recdim);
static Error get_seriesvalue(Arrayinfo, float **);

static Error variablename(int hand, int varid, char *cbuf);
static int setrank(char *);
static char *parseit(char *in, int *n, char *out[]);

/* globals defined in xdr_float.c.  xdr_float() increments these counts
 *  if IEEE defined but not-supported-on-the-i860-without-a-trap numbers
 *  are encountered.  they are all set to zero.
 */
#if !defined(DXD_STANDARD_IEEE)
extern int dx_denorm_fp_count;
extern int dx_bad_fp_count;
#endif

/*
 * public entry points: 
 */

/* 
 * inquire about the fields in a file
 *
 */
Object _dxfQueryImportInfo(char *filename)
{
    /* only handle netCDF for now */
    return QueryNetCDFInfo(filename);
}


/* 
 * create a field or group object.  new interface.
 * 
 * input: netCDF dataset name, dependant variable name list, int *starttime,
 *        int *endtime, int *deltatime
 * output: group or field
 *
 */
Object DXImportNetCDF(char *filename, char **variable, int *starttime, 
		    int *endtime, int *deltatime)
{
    return Object_Import(filename, variable, starttime, endtime, 
			 deltatime, IMPORT_DATA);
}


/*
 * do any processing at end of import, e.g. partitioning or statistics
 */
static Object EndImport(Object o)
{
    return o;
}


static Object
QueryNetCDFInfo(char *filename)
{
    return Object_Import(filename, NULL, NULL, NULL, NULL, IMPORT_UIQUERY);
}


static Object 
Object_Import(char *filename, char **varlist, int *starttime, int *endtime,
	      int *deltatime, int importflag)
{
    int cdfhandle;               /* file descriptor */
    Varinfo vp = NULL;           /* number and types of fields to import */
    Object o = NULL;             /* output objects */


    /* open the file, handle default dirs, socket connections, etc.
     *  if return < 0, error code has already been set.
     */
    if((cdfhandle = open_netcdf_file(filename)) < 0)
	return NULL;

#if !defined(DXD_STANDARD_IEEE)

    /* zero out the counts, and look at the end to see if we encountered
     *  any bad floating point values.
     */
    dx_denorm_fp_count = 0;
    dx_bad_fp_count = 0;

#endif
    
    /* fill in the varinfo struct, and return NULL if no
     *  valid fields are found.  sets error code before return.
     */
    if(!(vp = query_var(cdfhandle, varlist, starttime, endtime, deltatime)))
	return NULL;


    /* if query, put information about the field into a string object.
     * if import, import the data and construct the object.
     */
    switch(importflag) {
      case IMPORT_UIQUERY:
	o = query_field(cdfhandle, vp);
	break;

      case IMPORT_DATA:      
	o = import_field(cdfhandle, vp);
 	if( !o )
 	  DXWarning("Nothing has been imported. Did you use a OpenDX-compliant netCDF file convention?");
	else
        getglobalattr(o,vp);
	break;

      case IMPORT_VARINFO:
	close_netcdf_file(cdfhandle);
	return (Object)vp;

      default:
	DXSetError(ERROR_INTERNAL, "bad flag");
    }

    
    /* close the file, sever socket connections, etc.
     */
    close_netcdf_file(cdfhandle);

    vp_delete(vp);

#if !defined(DXD_STANDARD_IEEE)

    if(dx_denorm_fp_count > 0)
	DXWarning("%d denormalized floating point values rounded to zero",
		dx_denorm_fp_count);
    if(dx_bad_fp_count > 0)
	DXWarning("%d NaN or Infinity floating point values changed to zero",
		dx_bad_fp_count);
#endif

    /* hook for pre-processing the imported data, if needed */
    o = EndImport(o);
    return o;

}

/* open the file.  when we start doing distributed things, here is
 *  where the socket connections should be added.
 * look for name.nc instead of name.cdf name not found lumba 3Apr96
 */
static int
open_netcdf_file(char *filename)
{
    int cdfhandle;
    int foundfile = 0;
    char *fname = NULL, *foundname = NULL, *cp;
    char *datadir = NULL;
    int ll;

    /* netCDF library options: on error, don't abort and don't print */
    ncopts = 0;

#define XTRA 8  /* space for trailing /, .nc, trailing 0 and some slop */

    datadir = (char *)getenv("DXDATA");
    ll = strlen(filename) + XTRA;
    if (datadir != NULL)
    	ll += strlen(datadir);
    fname = (char *)DXAllocateLocalZero(ll);
    if (!fname)
	goto error;

    /* try name, name.nc, dir/name and dir/name.nc */
    if((cdfhandle = ncopen(filename, NC_NOWRITE)) < 0) {
	if (ncerr == 0 && !foundfile) {
	    foundfile++;
	    foundname = (char *)DXAllocateLocalZero(strlen(filename)+1);
	    if (!foundname)
		goto error;
	    strcpy(foundname, filename);
	}
	    

	strcpy(fname, filename);
	strcat(fname, ".nc");
	if((cdfhandle = ncopen(fname, NC_NOWRITE)) >= 0) {
	    DXMessage("opening file '%s'", fname);
	    goto file_ok;
	}
	if (ncerr == 0 && !foundfile) {
	    foundfile++;
	    foundname = (char *)DXAllocateLocalZero(strlen(fname)+1);
	    if (!foundname)
		goto error;
	    strcpy(foundname, fname);
	}

	if (!datadir)
	    goto e1;

	while (datadir) {

	    strcpy(fname, datadir);
	    if((cp = strchr(fname, PATH_SEP_CHAR)) != NULL)
	       *cp = '\0';
	    strcat(fname, "/");
	    strcat(fname, filename);
	    if((cdfhandle = ncopen(fname, NC_NOWRITE)) >= 0) {
		DXMessage("opening file '%s'", fname);
		goto file_ok;
	    }
	    if (ncerr == 0 && !foundfile) {
		foundfile++;
		foundname = (char *)DXAllocateLocalZero(strlen(fname)+1);
		if (!foundname)
		    goto error;
		strcpy(foundname, fname);
	    }

	    strcat(fname, ".nc");
	    if((cdfhandle = ncopen(fname, NC_NOWRITE)) >= 0) {
		DXMessage("opening file '%s'", fname);
		goto file_ok;
	    }
	    if (ncerr == 0 && !foundfile) {
		foundfile++;
		foundname = (char *)DXAllocateLocalZero(strlen(fname)+1);
		if (!foundname)
		    goto error;
		strcpy(foundname, fname);
	    }

	    datadir = strchr(datadir, PATH_SEP_CHAR);
	    if (datadir)
		datadir++;
	}

      e1:
	if (!foundfile)
	    DXSetError(ERROR_BAD_PARAMETER, "can't open file '%s'", filename);
	else
	    DXSetError(ERROR_BAD_PARAMETER, "'%s' not a netCDF file", foundname);

      error:
	DXFree((Pointer)fname);
	DXFree((Pointer)foundname);
	return -1;   /* bad filehandle */

    }

file_ok:
    DXFree((Pointer)fname);
    DXFree((Pointer)foundname);
    return cdfhandle;
}


/* close the file.  when we start doing distributed things, here is
 *  where the socket connections should be closed cleanly.
 */
static void
close_netcdf_file(int cdfhandle)
{
    ncclose(cdfhandle);
}


/* see if this netcdf file can be opened.  don't set error, but return 
 *  OK or ERROR.
 */
ImportStatReturn
_dxfstat_netcdf_file(char *filename)
{
    int cdfhandle;
    char *fname = NULL, *cp;
    char *datadir = NULL;

    /* netCDF library options: on error, don't abort and don't print */
    ncopts = 0;

    /* try name, name.cdf, dir/name and dir/name.cdf */
    if((cdfhandle = ncopen(filename, NC_NOWRITE)) >= 0) 
	goto file_ok;
    
    /* XTRA defined above: 
     * space for trailing /, .nc, trailing 0 and some slop 
     */
    
    datadir = (char *)getenv("DXDATA");
    if (!datadir)
	goto error;

    fname = (char *)DXAllocateLocalZero((datadir ? strlen(datadir) : 0) +
					strlen(filename) + XTRA);
    if (!fname)
	goto error;

    strcpy(fname, filename);
    strcat(fname, ".nc");
    if((cdfhandle = ncopen(fname, NC_NOWRITE)) >= 0)
    {
	DXFree((Pointer)fname);
	return IMPORT_STAT_NOT_FOUND;
    }
    
    while (datadir) {
	
	strcpy(fname, datadir);
	if((cp = strchr(fname, PATH_SEP_CHAR)) != NULL)
	    *cp = '\0';
	strcat(fname, "/");
	strcat(fname, filename);
	if((cdfhandle = ncopen(fname, NC_NOWRITE)) >= 0)
	    goto file_ok;

	strcat(fname, ".nc");
	if((cdfhandle = ncopen(fname, NC_NOWRITE)) >= 0)
	    goto file_ok;
	
	datadir = strchr(datadir, PATH_SEP_CHAR);
	if (datadir)
	    datadir++;
    }

  error:
    DXFree((Pointer)fname);
    return IMPORT_STAT_ERROR;
    
  file_ok:
    DXFree((Pointer)fname);
    ncclose(cdfhandle);
    return IMPORT_STAT_FOUND;
}

/* find out how many netcdf variables match the spec, and construct an
 *  information block for each field.  if the caller has given a specific
 *  variable or variable list, restrict it to those; if input list is null, 
 *  count all fields and series attrs in the file.
 * varlist must be null terminated.
 */
static Varinfo 
query_var(int cdfhandle, char **varlist, int *starttime, int *endtime,
	  int *deltatime)
{
    char stringattr[MAXNAME], *cp, *s[MAXATTRSTR], **lp, *attrtext=NULL;
    char *subv[2];
    int ndims, nvars, ngatts, recdim;
    int i, j, k, matched;
    int count = 0;
    int numspec = 0, *found = NULL;
    double position;
    Varinfo vp = NULL;        /* root of tree, to be returned */
    Varinfo vp1, vp2, vp3;    /* temp node pointers */


    /* count the number of netCDF variables in the file
     */
    if ((ncinquire(cdfhandle, &ndims, &nvars, &ngatts, &recdim) < 0)
       || nvars <= 0) {
	DXSetError(ERROR_MISSING_DATA, "no netCDF variables found in file");
	return NULL;
    }
    
    
    /* count the number of fields specified by the user */
    if (varlist != NULL && varlist[0] != NULL) {
	for(lp = varlist; *lp; lp++)
	    numspec++;
	
	if ((found = (int *)DXAllocateLocalZero(sizeof(int) * numspec)) == NULL)
	    return NULL;
    }
    
    /* check for any global series attributes.
     */
    for (k = 0; k < ngatts; k++) {
         /* If a continue has been encountered in the previous iteratation */
         if(attrtext) DXFree((Pointer) attrtext);

         if(!(attrtext=getNattr(cdfhandle, NC_GLOBAL, k, SERIESATTRIB,
                     strlen(SERIESATTRIB), stringattr)))
            continue;
	
	j = MAXATTRSTR;
	cp = parseit(attrtext, &j, s);
	if(j <= 0) {
	    DXSetError(ERROR_DATA_INVALID, "bad attribute for series");
	    goto error;
	}

	matched = 0;
	if(varlist == NULL || varlist[0] == NULL)
	    matched = 1;

	else for(lp = varlist; *lp; lp++) {
	    if(strcmp(s[0], *lp))
		continue;
	    
	    matched = 1;
	    found[lp - varlist]++;
	    break;
	}

	if(!matched)
	    continue;

	/* we are going to process this netCDF variable - add it to the tree
         *  we are building.  if match_vp() returns non-null, it found a
	 *  match, and has already added it as a compositefield member.
	 *
	 * vp = root of tree
	 * vp1 = new series members
	 * vp2 = new series parent node
	 * vp3 = temp node for finding end of next->next->next chain.
	 */
	if (match_vp(vp, NC_GLOBAL, s, attrtext) != NULL)
	    continue;

	/* new series object 
	 */
	vp2 = (Varinfo)DXAllocateLocalZero(sizeof(struct varinfo));
	if(!vp2)
	    goto error;
	vp2->class = CLASS_SERIES;
	strcpy(vp2->name, s[0]);
	vp2->varid = INVALID_VARID;
	vp2->startframe = starttime;
	vp2->endframe = endtime;
	vp2->deltaframe = deltatime;
	vp2->recdim = INVALID_DIMID;
	vp2->cdfid = cdfhandle;
	
	/* multiple files */
	if(s[1] && !strcmp(s[1], "files")) {
	    i = -1;
	    while(1) {
		i++;
		j = MAXATTRSTR;
		cp = parseit(cp, &j, s);
		if(j <= 0)
		    break;
		
		/* if any of start, end, delta or current are set,
		 *  restrict the input to only those in the list.
		 */
		if (starttime && i < *starttime)
		    continue;
		
		if (endtime && i > *endtime)
		    continue;
		
		if (deltatime) {
		    if ((starttime && (i - *starttime) % *deltatime)
		     || (!starttime && i % *deltatime))
			continue;
		}
		
		subv[1] = NULL;
		switch(j) {
		  case 1:
		    /* only filename, no fieldname or series pos */
		    subv[0] = vp2->name;
		    position = i;
		    break;
		    
		  case 2:
		    /* filename plus either fieldname or position */
		    if(isdigit(s[1][0])) {
			subv[0] = vp2->name;
			position = atof(s[1]);
		    } else {
			subv[0] = s[1];
			position = i;
		    }
		    break;
		    
		  case 3:
		    /* filename, fieldname, position */
		    subv[0] = s[1];
		    position = atof(s[2]);
		    break;
		    
		  default:
		    /* more than 3 parms? */
		    DXSetError(ERROR_BAD_PARAMETER, 
			     "bad attribute for series '%s'", 
			     vp2->name);
		    goto error;
		}
		
		/* new series member */
		vp1 = (Varinfo)Object_Import(s[0], subv, starttime, endtime,
					     deltatime, IMPORT_VARINFO);
		if(!vp1) 
		    goto error;
		
		strcpy(vp1->cdfname, s[0]);
		vp1->cdfid = -1;
		vp1->position = position;
		
		/* first series member?  make it the first child */
		if (vp2->child == NULL)
		    vp2->child = vp1;

		else {   /* add to end of series existing list */
		    vp3 = vp2->child;
		    while (vp3->next != NULL)
			vp3 = vp3->next;

		    vp3->next = vp1;
		}
		count++;
	    }
	    
	} else {
	    /* series using variables in this file */
	    i = -1;
	    while(1) {
		i++;
		j = MAXATTRSTR;
		cp = parseit(cp, &j, s);
		if(j <= 0)
		    break;
		
		/* if any of start, end, delta or current are set,
		 *  restrict the input to only those in the list.
		 */
		if (starttime && i < *starttime)
		    continue;
		
		if (endtime && i > *endtime)
		    continue;
		
		if (deltatime) {
		    if ((starttime && (i - *starttime) % *deltatime)
		     || (!starttime && i % *deltatime))
			continue;
		}
		
		/* new series member */
		vp1 = (Varinfo)DXAllocateLocalZero(sizeof(struct varinfo));
		if(!vp1)
		    goto error;
		vp1->class = CLASS_FIELD;
		vp1->varid = ncvarid(cdfhandle, s[0]);
		if(vp1->varid < 0) {
		    DXSetError(ERROR_BAD_PARAMETER, 
			     "field %s not found in series %s",
			     s[0], vp2->name);
		    DXFree((Pointer)vp1);
		    goto error;
		}
		vp1->cdfid = cdfhandle;
		vp1->recdim = INVALID_DIMID;
		strcpy(vp1->name, s[0]);
		
		if(s[1])
		    vp1->position = atof(s[1]);
		else
		    vp1->position = i;

		/* first series member?  make it the first child */
		if (vp2->child == NULL)
		    vp2->child = vp1;

		else {   /* add to end of series existing list */
		    vp3 = vp2->child;
		    while (vp3->next != NULL)
			vp3 = vp3->next;

		    vp3->next = vp1;
		}
		count++;
	    }
	}

	/* vp is root of tree.
	 * vp1 is new group object if needed.
	 * vp2 is series object constructed during this pass
	 * vp3 is temp to add object to end of next->next->next list.
	 */

	/* if first object, make it the root */
	if(!vp)
	    vp = vp2;

	else if(vp->class != CLASS_GROUP) {  /* new group needed */
	    vp1 = (Varinfo)DXAllocateLocalZero(sizeof(struct varinfo));
	    if(!vp1)
		goto error;
	    vp1->class = CLASS_GROUP;
	    vp1->varid = INVALID_VARID;
	    vp1->recdim = INVALID_DIMID;
	    vp1->cdfid = cdfhandle;
	    vp1->child = vp;
	    vp->next = vp2;
	    vp = vp1;

	} else {   /* group already exists.  add to end of list */
	    vp3 = vp->child;
	    while (vp3->next != NULL)
		vp3 = vp3->next;
	    vp3->next = vp2;
	}
	count++;
      DXFree((Pointer) attrtext);
      attrtext=NULL;
    }
    if(attrtext) {
      DXFree((Pointer) attrtext);
      attrtext=NULL;
    }


    /* check each variable looking for 'field' attribute.
     */
    for(i=0; i<nvars; i++) {
	
	if(!getattr(cdfhandle, i, FIELDATTRIB, stringattr, MAXNAME))
	    continue;

	j = MAXATTRSTR;
	cp = parseit(stringattr, &j, s);
	if(j <= 0)
	    continue;
	
	matched = 0;
	if(varlist == NULL || varlist[0] == NULL)
	    matched = 1;
	
	else for(lp = varlist; *lp; lp++) {
	    if(strcmp(s[0], *lp))
		continue;
	    
	    matched = 1;
	    found[lp - varlist]++;
	    break;
	}
	
	if(!matched)
	    continue;

	/* if field name matches an existing one, match_vp returns non-null,
	 *  and has already added it as a member of a composite field.
	 */
	if (match_vp(vp, i, s, stringattr))
	    continue;

	/* new field/implicit series */
	vp2 = (Varinfo)DXAllocateLocalZero(sizeof(struct varinfo));
	if(!vp2)
	    goto error;
	
	/* check here for series keyword */
	if(j > 2 && !strcmp(s[2], "series")) {
	    vp2->class = CLASS_SERIES;
	    strcpy(vp2->name, s[0]);
	    vp2->varid = i;
	    vp2->cdfid = cdfhandle;
	    vp2->startframe = starttime;
	    vp2->endframe = endtime;
	    vp2->deltaframe = deltatime;
	    vp2->recdim = recdim;
	} else {
	    vp2->class = CLASS_FIELD;
	    strcpy(vp2->name, s[0]);
	    vp2->varid = i;
	    vp2->cdfid = cdfhandle;
	    vp2->recdim = INVALID_DIMID;
	}

	/* vp is root of tree.
	 * vp1 is new group node if needed.
	 * vp2 is new field/series
	 * vp3 is temp to add object to end of next->next->next list.
	 */

	/* if first object, make it root */
	if(!vp)
	    vp = vp2;
	
	else if(vp->class != CLASS_GROUP) {  /* create new group */
	    vp1 = (Varinfo)DXAllocateLocalZero(sizeof(struct varinfo));
	    if(!vp1)
		goto error;
	    vp1->class = CLASS_GROUP;
	    vp1->varid = INVALID_VARID;
	    vp1->recdim = INVALID_DIMID;
	    vp1->cdfid = cdfhandle;
	    vp1->child = vp;
	    vp->next = vp2;
	    vp = vp1;

	} else {  /* group already exists.  add object to end */
	    vp3 = vp->child;
	    while (vp3->next != NULL)
		vp3 = vp3->next;

	    vp3->next = vp2;
	}
	count++;
    }
    

    /* if no field attributes found, import netCDF variables as reg grids */
    if (count <= 0) {
	if (numspec == 0)
	    DXWarning("no 'field' attributes; importing all netCDF variables");
	else
	    DXWarning("no 'field' attributes matched list; trying netCDF variable names");
	
	/* treat all variables as having a 'field' attribute.
	 */
	for(i=0; i<nvars; i++) {
	    
	    if(!variablename(cdfhandle, i, stringattr))
		goto error;
	    
	    matched = 0;
	    if(varlist == NULL || varlist[0] == NULL)
		matched = 1;
	    
	    else for(lp = varlist; *lp; lp++) {
		if(strcmp(stringattr, *lp))
		    continue;
		
		matched = 1;
		found[lp - varlist]++;
		break;
	    }
	   
	    if (!strcmp(stringattr,""))	/* dimension=0 so skip this variable */
	      matched = 0;

	    if(!matched)
		continue;
	    
	    
	    /* this can be simpler than above, since netCDF variable names 
             *  must be unique.  therefore, we can't have identical names
	     *  so there can't be composite fields.  also, we don't recognize
             *  series (although if recdim is used, we could, i suppose).
	     */
	    vp2 = (Varinfo)DXAllocateLocalZero(sizeof(struct varinfo));
	    if(!vp2)
		goto error;
	    
	    vp2->class = CLASS_FIELD;
	    strcpy(vp2->name, stringattr);
	    vp2->varid = i;
	    vp2->cdfid = cdfhandle;
	    vp2->recdim = INVALID_DIMID;
	    
	    /* vp is root of tree.
	     * vp1 is new group node if needed.
	     * vp2 is new node to be added.
	     * vp3 is temp to add node at end of next->next->next list
	     */

	    /* first object */
	    if(!vp)
		vp = vp2;
	    
	    else if(vp->class != CLASS_GROUP) {  /* create a group */
		vp1 = (Varinfo)DXAllocateLocalZero(sizeof(struct varinfo));
		if(!vp1)
		    goto error;
		vp1->class = CLASS_GROUP;
		vp1->varid = INVALID_VARID;
		vp1->recdim = INVALID_DIMID;
		vp1->cdfid = cdfhandle;
		vp1->child = vp;
		vp->next = vp2;
		vp = vp1;
		
	    } else {  /* group already exists.  add to end of list */
		vp3 = vp->child;
		while (vp3->next)
		    vp3 = vp3->next;
		vp3->next = vp2;
	    }
	    count++;
	}
	
    }
    
    /* if STILL 0, user specified list and didn't match variables */
    if (count <= 0) {
	DXSetError(ERROR_BAD_PARAMETER, "no matching fields found in file");
	goto error;
    }

    /* warn about field names which didn't match anything in the file */
    if (found) {
	for (i=0; i<numspec; i++) 
	    if (found[i] == 0)
		DXWarning("field '%s' not found in file", varlist[i]);

	DXFree((Pointer)found);
    }

    
    return vp;

  error:
    if(attrtext)
      DXFree((Pointer)attrtext);
    if (found)
	DXFree((Pointer)found);
    if(vp)
        vp_delete(vp);
    return NULL;
}



/* if this routine finds a match, it returns non-null.  if the field
 *  isn't already found, it returns null.
 */
static Varinfo 
match_vp(Varinfo vp, int varid, char **s, char *stringattr)
{
    Varinfo vp1, vp2, vp3;


    /* search down through vp, looking for name matches.  if hit,
     *  this becomes a composite field.
     *  (if varid already in the file, skip the field because it's there as
     *  part of a series?)
     */
    for(vp1 = vp; vp1; vp1 = vp1->next) {
#if 0
        /* already in tree, skip it this time? */
	if(vp1->varid == varid)
	    return vp;
#endif
	
	if(!strcmp(vp1->name, s[0])) {
	    /* make a composite field node - this name matches.
	     *  vp1 is original node.  vp2 is created and the vp1 info is
             *  copied into it.  then vp1 is changed to a composite field node.
	     */
	    if(!vp1->child) {
		/* test here: class should == FIELD */
		vp2 = (Varinfo)DXAllocateLocalZero(sizeof(struct varinfo));
		if(!vp2)
		    return NULL;
		vp2->class = vp1->class;
		vp2->varid = vp1->varid;
		vp2->cdfid = vp1->cdfid;
		vp2->recdim = vp1->recdim;
		vp1->class = CLASS_COMPOSITEFIELD;
		vp1->varid = INVALID_VARID;
		vp1->recdim = INVALID_DIMID;
		vp1->child = vp2;
	    }

	    /* new node for this field */
	    vp2 = (Varinfo)DXAllocateLocalZero(sizeof(struct varinfo));
	    if(!vp2)
		return NULL;
	    vp2->class = CLASS_FIELD;
	    vp2->varid = varid;
	    vp2->recdim = INVALID_DIMID;
	    vp2->cdfid = vp1->cdfid;

	    /* add to end of composite field list */
	    vp3 = vp1->child;
	    while (vp3->next != NULL)
		vp3 = vp3->next;

	    vp3->next = vp2;
	    return vp;
	}
	
	if(vp1->child)
	    return match_vp(vp1->child, varid, s, stringattr);
		

    }

    return NULL;
}


/* build a string which describes a field, put it into a string object
 *  and return it.
 */
static Object
query_field(int cdfhandle, Varinfo vp)
{
    char stringattr[MAXNAME], *cp, *s[MAXATTRSTR];
    int i, rank;
    Varinfo nvp;

    Object o = NULL;
    Object newo = NULL;
    char *sp1 = NULL;


    switch(vp->class) {
      case CLASS_GROUP:
	if(!vp->child)
	    DXErrorReturn(ERROR_INTERNAL, "inconsistant data structs");
	    
	for(nvp = vp->child; nvp; nvp = nvp->next) {
	
	    newo = query_field(cdfhandle, nvp);
	    if(!newo) {
		DXDelete(o);
		return NULL;
	    }
	    if(!o) {
		o = STR("Group: ");
		if(!o) {
		    DXDelete(newo);
		    return NULL;
		}
	    }
	    sp1 = DXAllocate(strlen(DXGetString((String)o)) + 
			   strlen(DXGetString((String)newo)));
	    strcpy(sp1, DXGetString((String)o));
	    strcat(sp1, DXGetString((String)newo));
	    strcat(sp1, ";");
	    DXDelete(o);
	    DXDelete(newo);
	    o = STR(sp1);
	}
	break;
	    
      case CLASS_SERIES:
	if(!vp->child) {
	    /* series object all in one N+1 D netcdf variable */
	    o = STR("Series: ");

            newo = query_field(cdfhandle, vp);
	    if(!newo) {
		DXDelete(o);
		return NULL;
	    }

	    sp1 = DXAllocate(strlen(DXGetString((String)o)) + 
			   strlen(DXGetString((String)newo)));
	    strcpy(sp1, DXGetString((String)o));
	    strcat(sp1, DXGetString((String)newo));
	    DXDelete(o);
	    DXDelete(newo);
	    o = STR(sp1);
	    break;
	}
	    
	for(nvp = vp->child; nvp; nvp = nvp->next) {
	
            newo = query_field(cdfhandle, nvp);
	    if(!newo) {
		DXDelete(o);
		return NULL;
	    }
	    if(!o) {
		o = STR("Series: ");
		if(!o) {
		    DXDelete(newo);
		    return NULL;
		}
	    }
	    sp1 = DXAllocate(strlen(DXGetString((String)o)) + 
			   strlen(DXGetString((String)newo)));
	    strcpy(sp1, DXGetString((String)o));
	    strcat(sp1, DXGetString((String)newo));
	    sprintf(sp1+strlen(sp1), ", position %g;", nvp->position);
	    DXDelete(o);
	    DXDelete(newo);
	    o = STR(sp1);
	}
	break;
	    
      case CLASS_COMPOSITEFIELD:
	if(!vp->child)
	    DXErrorReturn(ERROR_INTERNAL, "inconsistant data structs");
	
	for(nvp = vp->child; nvp; nvp = nvp->next) {
	
	    newo = query_field(cdfhandle, nvp);
	    if(!newo) {
		DXDelete(o);
		return NULL;
	    }
	    if(!o) {
		o = STR("Composite Field: ");
		if(!o) {
		    DXDelete(newo);
		    return NULL;
		}
	    }
	    sp1 = DXAllocate(strlen(DXGetString((String)o)) + 
			   strlen(DXGetString((String)newo)));
	    strcpy(sp1, DXGetString((String)o));
	    strcat(sp1, DXGetString((String)newo));
	    strcat(sp1, ";");
	    DXDelete(o);
	    DXDelete(newo);
	    o = STR(sp1);
	}
	break;
	    
      case CLASS_FIELD:
	if(vp->cdfid < 0 || vp->cdfid != cdfhandle) {
            if(vp->cdfname[0] == '\0')
                return NULL;
            vp->cdfid = open_netcdf_file(vp->cdfname);
            if(vp->cdfid < 0)
                return NULL;
        }
	getattr(vp->cdfid, vp->varid, FIELDATTRIB, stringattr, MAXNAME);
	    
	cp = stringattr;
	i = MAXATTRSTR;
	cp = parseit(cp, &i, s);
	
	if(i <= 0)
	    return NULL;
	
	/* should already be set in varinfo -- 
	 *  strcpy(varname, s[0]); 
	 */
	rank = setrank(s[1]);
	/* etc etc */
	
	    
#if REALCODE
	sp1 = (char *)DXAllocate(strlen(vp->name) + sizeof("name:") + 2);
	strcpy(sp1, "name:");
	strcat(sp1, vp->name);
	strcat(sp1, ";");
#else
	/* misc whitespace added for testing */
	sp1 = (char *)DXAllocate(1024);
	strcpy(sp1, "name:");
	strcat(sp1, vp->name);
	strcat(sp1, ";contype:tetras;");
	strcat(sp1, "datatype: float;\tdatacat:real;");
	strcat(sp1, "datarank:1; datashape:3;");
	strcat(sp1, "  datacount:37000;\n");
	strcat(sp1, "metahistory:original data;");
	strcat(sp1, "metadesc:3-D seismic data from mars;");
#endif   /* !REALCODE */
	    
	o = STR(sp1);
	DXFree(sp1);
        if(cdfhandle != vp->cdfid) {
            close_netcdf_file(vp->cdfid);
            vp->cdfid = -1;
        }
	break;
	    
      default:
	o = NULL;  /* ??? */
	break;
    }
    
    return o;
}




/* build an object based on the varinfo structures, and return it
 */
static Object
import_field(int cdfhandle, Varinfo vp)
{
    int i = 0;
    Varinfo nvp;

    Object o = NULL;
    Object newo = NULL;


    switch(vp->class) {
      case CLASS_GROUP:
	if(!vp->child)
	    DXErrorReturn(ERROR_INTERNAL, "inconsistant data structs");

	for(nvp = vp->child; nvp; nvp = nvp->next) {
	
	    newo = import_field(cdfhandle, nvp);
	    if(!newo) {
		DXDelete(o);
		return NULL;
	    }
	    if(!o) {
		o = (Object)DXNewGroup();
		if(!o) {
		    DXDelete(newo);
		    return NULL;
		}
	    }
	    DXSetMember((Group)o, nvp->name, newo);
	}
	break;
	
      case CLASS_SERIES:
	if(!vp->child) {
            /* series bundled as 1 netcdf variable, using unlimited dim */
            o = build_series(cdfhandle, vp);
            break;
	}

	for(nvp = vp->child; nvp; nvp = nvp->next) {
	    
            newo = import_field(cdfhandle, nvp);
	    if(!newo) {
		DXDelete(o);
		return NULL;
	    }
	    if(!o) {
		o = (Object)DXNewSeries();
		if(!o) {
		    DXDelete(newo);
		    return NULL;
		}
		i = 0;
	    }
	    DXSetSeriesMember((Series)o, i++, nvp->position, newo);
	}
	break;

      case CLASS_COMPOSITEFIELD:
	if(!vp->child)
	    DXErrorReturn(ERROR_INTERNAL, "inconsistant data structs");

	for(nvp = vp->child; nvp; nvp = nvp->next) {
	    
	    newo = import_field(cdfhandle, nvp);
	    if(!newo) {
		DXDelete(o);
		return NULL;
	    }
	    if(!o) {
		o = (Object)DXNewCompositeField();
		if(!o) {
		    DXDelete(newo);
		    return NULL;
		}
	    }
	    DXSetMember((Group)o, NULL, newo);
	}
	break;
	
      case CLASS_FIELD:
	
	if(vp->cdfid < 0 || vp->cdfid != cdfhandle) {
            if(vp->cdfname[0] == '\0')
                return NULL;
            vp->cdfid = open_netcdf_file(vp->cdfname);
            if(vp->cdfid < 0)
                return NULL;
        }

	o = build_field(vp->cdfid, vp->varid);

        if(cdfhandle != vp->cdfid) {
            close_netcdf_file(vp->cdfid);
            vp->cdfid = -1;
        }

	if (o && vp->name)
	    DXSetAttribute(o, "name", (Object)DXNewString(vp->name));

	break;
	
      default:
	DXSetError(ERROR_INTERNAL, "bad class in varinfo struct");
	DXDelete(o);
	return NULL;
    }
   
    return o;
}


/* delete a varinfo tree
 */
static void vp_delete(Varinfo vp)
{
    Varinfo vp1, vp2;

    for(vp2 = vp; vp2; ) {
	if(vp2->child)
	    vp_delete(vp2->child);
	
	vp1 = vp2;
	vp2 = vp2->next;
	DXFree((Pointer)vp1);

    }
}    


/* debug - print a varinfo tree
 */
static void vp_print(Varinfo vp)
{
    Varinfo vp1, vp2;

    for(vp2 = vp; vp2; ) {
	if(vp2->child)
	    vp_print(vp2->child);
	
	vp1 = vp2;
	vp2 = vp2->next;
	printf("%08x: class = %d, varid = %d, name = %s\n",
	       (unsigned int) vp1, vp1->class, vp1->varid, vp1->name);
	printf("          next = %08x, child = %08x\n", (unsigned int) vp1->next, (unsigned int) vp1->child);

    }
}    

/*
 * use netCDF routines to retrieve the named attribute, verify it is a 
 *  string (char), and null terminate it.
 */
static char *
getattr(int hand, int varid, char *attrname, char *stringattr,int maxlen)
{
    nc_type datatype;
    int len;
    char *tmpattr;

    if(ncattinq(hand, varid, attrname, &datatype, &len) < 0)
	return NULL;

    if(datatype != NC_CHAR || len <= 0)
	return NULL;

    tmpattr=(char *)DXAllocate(len+1);
    ncattget(hand, varid, attrname, tmpattr);
    tmpattr[len] = '\0';

    strncpy(stringattr,tmpattr,maxlen);
    stringattr[maxlen-1]='\0';

    if(strncmp(stringattr,tmpattr,maxlen))
      DXWarning("String attribute truncated: `%s' -> `%s'",
              tmpattr,stringattr);

    DXFree((Pointer) tmpattr);
    return stringattr;
}
/*
 * use netCDF routines to retrieve the Nth attribute of a variable and
 *  compare the first len characters for a match.  if matched, get the
 *  complete value of the attribute, verify it is a string (char), and 
 *  null terminate it.
 *  The attribute is returned as a newly allocated string. It is the
 *  caller's responsibility the deallocate it when it is no longer
 *  needed.
 */
static char *
getNattr(int hand, int varid, int n, char *attrname, int len, char *stringattr)
{
    nc_type datatype;
    int i, alen;
    char *attrtext=NULL;

    for(i = n; ; i++) {
        if(ncattname(hand, varid, i, stringattr) < 0)
            return NULL;

        if(strncmp(stringattr, attrname, len))
            continue;

        /* reset attribute name to the one retrieved from netcdf */
        attrname = stringattr;
        break;
    }

    if(ncattinq(hand, varid, attrname, &datatype, &alen) < 0)
	return NULL;

    if(datatype != NC_CHAR || alen <= 0)
	return NULL;

    /* does this work?  we are actually using the same buffer for the
     *  input attribute name and for the returned attribute value.
     ncattget(hand, varid, attrname, stringattr);
     */
    /* No it does not. Make sure the target string has sufficient length */
    attrtext=DXAllocate(alen+1);
    ncattget(hand, varid, attrname, attrtext);
    attrtext[alen] = '\0';

    return attrtext;
}

/*
 * test just to see if there is such a named attribute for this variable.
 */
static char *
isattr(int hand, int varid, char *attrname)
{
    nc_type datatype;
    int len;

    if(ncattinq(hand, varid, attrname, &datatype, &len) < 0)
	return NULL;

    return attrname;

}

/*
 * see if this variable has any ADX attributes, and use them to set 
 *  the component attributes.
 */
static Error setattr(Arrayinfo ap, Field f, char *compname)
{
    char *cp, *s[MAXATTRSTR];
    char savename[MAXCOMPNAME];
    int j;

    /* save the component name now, because it may get overwritten
     *  further down
     */
    strcpy(savename, compname);

    /* see if there is an 'attribute' attribute on this variable
     */
    if(!getattr(ap->cdfhandle, ap->varid, ATTRATTRIB, ap->stringattr,MAXNAME)
     && !getattr(ap->cdfhandle, ap->varid, OATTRATTRIB, ap->stringattr,MAXNAME))
        goto done;

    
    cp = ap->stringattr;
    while(1) {
        j = MAXATTRSTR;
        cp = parseit(cp, &j, s);
        
        /* field:attribute = "a, b";  attribute required.  exactly two
         *  parameters must be present.
         *   a = attribute to set.
         *   b = string to set it to.
         */
        if(j <= 0)
            break;
        if(j != 2) {
            DXSetError(ERROR_BAD_PARAMETER, 
                     "bad attribute value on component '%s'", savename);
            return ERROR;
        }
            
        
        /* and add it to the field with the given component name */
        if(!DXSetComponentAttribute(f, savename, s[0], STR(s[1]))) {
            DXSetError(ERROR_BAD_PARAMETER, 
                     "bad attribute value: '%s' on component '%s'",
                     s[0], savename);
            return ERROR;
            
	}
    }

done:
    /* look for non DX specific attributes and make them component
     * attributes
     */
    setuserattr(ap,f,savename); 
    
    return OK;
}

/* go through each attribute and for each one that is not a
 * DX netcdf attribute, create a DX object and make it a
 * component attribute
 */
static Error setuserattr(Arrayinfo ap, Field f, char *compname)
{
  int i, attr_count=0;
  int alen;
  nc_type datatype;
  Type type;
  Array a;
  char *attr_string;
  void *attr_value;

  while(ncattname(ap->cdfhandle,ap->varid,attr_count,ap->stringattr) !=-1) {
    attr_count++;
    /* is this a DX specific attribute, if yes ignore */
    for (i=0; i<MAX_DXATTRIBUTES; i++){
      if (!strcmp(dxattributes[i],ap->stringattr))
	break;
    }
    if (i < MAX_DXATTRIBUTES) 	/* skip the dx attributes */
      continue;

    if(ncattinq(ap->cdfhandle, ap->varid, ap->stringattr, &datatype, &alen) < 0)
	return ERROR;

    /* convert from netCDF data flags to SVS data flags */
    switch(datatype) {
      case NC_CHAR:   type = TYPE_STRING; break;
      case NC_SHORT:  type = TYPE_SHORT;  break;
      case NC_LONG:   type = TYPE_INT;    break;
      case NC_FLOAT:  type = TYPE_FLOAT;  break;
      case NC_DOUBLE: type = TYPE_DOUBLE; break;
      default:        type = TYPE_UBYTE;  break;   
	/* we need a TYPE_BYTE */
    }
    
    if (type == TYPE_STRING){
      attr_string = (char *)DXAllocate(alen+1);
      if (ncattget(ap->cdfhandle, ap->varid,ap->stringattr,attr_string) < 0) 
        return ERROR;
      attr_string[alen] = '\0';
      DXSetComponentAttribute(f, compname, ap->stringattr, STR(attr_string));
      DXFree((Pointer)attr_string);
    }
    else{
      a = DXNewArray(type,CATEGORY_REAL,0);
      a = DXAddArrayData(a,0,alen,NULL);
      attr_value = DXGetArrayData(a);
      if (ncattget(ap->cdfhandle, ap->varid,ap->stringattr,attr_value) < 0) 
        return ERROR;
      DXSetComponentAttribute(f,compname, ap->stringattr, (Object)a);
    } 
  }
  return OK;
}

/* go through each attribute and for each one that is not a
 * DX netcdf attribute, create a DX object and make it a
 * component attribute
 */
static Error getglobalattr(Object o, Varinfo vp)
{
  int attr_count=0;
  int alen;
  nc_type datatype;
  Type type;
  Array a;
  char stringattr[MAXNAME];
  char *attr_string;
  void *attr_value;
 
	if( !o ) {
		if (DXGetError() == ERROR_NONE)
		DXErrorReturn(ERROR_INTERNAL,"getglobalattr called without a proper object");
	}

  while(ncattname(vp->cdfid,NC_GLOBAL,attr_count,stringattr) !=-1) {
    attr_count++;
    /* is this a DX specific attribute, if yes ignore */
    if (!strcmp(SERIESATTRIB,stringattr))
	continue;

    if(ncattinq(vp->cdfid, NC_GLOBAL, stringattr, &datatype, &alen) < 0)
	return ERROR;

    /* convert from netCDF data flags to SVS data flags */
    switch(datatype) {
      case NC_CHAR:   type = TYPE_STRING; break;
      case NC_SHORT:  type = TYPE_SHORT;  break;
      case NC_LONG:   type = TYPE_INT;    break;
      case NC_FLOAT:  type = TYPE_FLOAT;  break;
      case NC_DOUBLE: type = TYPE_DOUBLE; break;
      default:        type = TYPE_UBYTE;  break;   
	/* we need a TYPE_BYTE */
    }
    
    if (type == TYPE_STRING){
      attr_string = (char *)DXAllocate(alen+1);
      if (ncattget(vp->cdfid, NC_GLOBAL,stringattr,attr_string) < 0) 
        return ERROR;
      attr_string[alen] = '\0';
      DXSetAttribute(o, stringattr, STR(attr_string));
      DXFree((Pointer)attr_string);
    }
    else{
      a = DXNewArray(type,CATEGORY_REAL,0);
      a = DXAddArrayData(a,0,alen,NULL);
      attr_value = DXGetArrayData(a);
      if (ncattget(vp->cdfid, NC_GLOBAL,stringattr,attr_value) < 0) 
        return ERROR;
      DXSetAttribute(o, stringattr, (Object)a);
    } 
  }
  return OK;
}

/* this is where a single netcdf variable is an entire series, with the
 *  unlimited record dimension interpreted as the series value, and the
 *  data as numdims-1.
 */
static Object build_series(int hand, Varinfo vp)
{
    int arraytype = I_NONE;
    int series = 0, varseries;
    int nopos = 0, nocon = 0;
    struct arrayinfo a, *ap = &a;
    char *cp, *s[MAXATTRSTR];
    int i, j, k;
    int varid = vp->varid;
    int recdim = vp->recdim;
    int nterms;
    float value, *valuelist = NULL;
    Field f = NULL;
    Object indata, arr, terms[MAXRANK], *t = NULL;
    Object eltype = NULL;
    Object o = NULL;
    

    /* set up the Arrayinfo struct
     */
    ap->cdfhandle = hand;
    ap->varid = varid;
    ap->recdim = recdim;
    ap->vp = vp;


    /* get the attribute value.
     */
    if(!getattr(hand, varid, FIELDATTRIB, ap->stringattr,MAXNAME))
	return NULL;
    
    cp = ap->stringattr;
    j = MAXATTRSTR;
    cp = parseit(cp, &j, s);
    if(j <= 0)
	return NULL;


    if (!check_serieslength(ap))
	return NULL;

    series = get_serieslength(ap);
    if(series <= 0)
	DXErrorReturn(ERROR_DATA_INVALID, "no members in series dimension");

    
    o = (Object)DXNewSeries();
    
    
    /* 
     * data component
     */
    
    /* decide if it's scalar, vector or matrix data */
    ap->arrayrank = setrank(s[1]);

    varseries = is_seriesvariable(ap->cdfhandle, ap->varid, ap->recdim);
    ap->data_isrec = varseries;

    /* this routine reads a netCDF variable into an Array.
     *  in this case, it's reading in the data component.
     */
    if(varseries) {
	indata = build_data(ap, recdim);
	if(!get_seriesvalue(ap, &valuelist))
	    goto error;
    } else
	indata = build_data(ap, INVALID_DIMID);

    if(!indata)
	goto error;

    for(i=0; i<series; i++) {
	f = DXNewField();
	if(!f)
	    goto error;
	
	arr = varseries ? DXGetSeriesMember((Series)indata, i, &value) 
	                : indata;
	    
	if(!DXSetComponentValue(f, DATCOMPNAME, arr))
	    goto error;
	
	/* look here for more attributes on the data component - to set
	 *  data component attributes.
	 */
	if(!setattr(ap, f, DATCOMPNAME))
	    goto error;
	

	if(varseries && valuelist)
	    value = valuelist[i];
#if 0
	else
	    value = i;
#endif

	o = (Object)DXSetSeriesMember((Series)o, i, value, (Object)f);

    }

    /* if the data was a series-in-a-variable, delete the temporary group
     *  holding the data arrays.
     */
    if(varseries) {
	DXDelete(indata);
	DXFree((Pointer)valuelist);
    }
	
    /*
     * positions component
     */
    
    ap->arrayrank = 1;   /* unless they explicitly say 'scalar' */
    varseries = 0;
    
    /* look for a positions (old points) component.  if the data variable
     *  doesn't have a 'positions' attribute, assume a regularly spaced grid.
     */
    if(!getattr(hand, varid, GEOMATTRIB, ap->stringattr,MAXNAME)
    && !getattr(hand, varid, OGEOMATTRIB, ap->stringattr,MAXNAME)) {
        nterms = 1;
	indata = build_poscon(ap, I_REGULAR_P, 0);
	terms[0] = indata;
    
    } else {
	/* else they gave us a geometry.  parse it. 
	 */
	
	if(isattr(hand, varid, OGEOMATTRIB))
	    DXWarning("'geometry' attribute obsolete, use 'positions' instead");
        
	cp = ap->stringattr;
        nterms = 0;
	while(1) {
	    j = MAXATTRSTR;
	    cp = parseit(cp, &j, s);
	    if(j <= 0)
		break;
            
	    /* positions are vectors unless specifically told scalar */
	    if((j > 1 && !strcmp("scalar", s[1]))
	     ||(j > 2 && !strcmp("scalar", s[2])))
		ap->arrayrank = 0;
            
	    if(!strcmp("none", s[0])) {
		
		/* no positions - should this be ok? */
		nopos++;
		
	    } else if(!strcmp("regular", s[0])) {
		
		/* 'regular' is same as default */
		arraytype = I_REGULAR_P;
		
	    } else if((j > 1 && !strcmp("product", s[1]))
                    ||(j > 2 && !strcmp("product", s[2]))) {
		

		if((k = ncvarid(hand, s[0])) < 0) {
		    DXSetError(ERROR_BAD_PARAMETER,"positions variable missing");
		    goto error;
		}
                ap->varid = k;
		varseries = is_seriesvariable(ap->cdfhandle, ap->varid, 
					      ap->recdim);
		
		/* regular grid, non-zero origin or non-unit spacing */
		if((j > 1 && 
                   (!strcmp("regular", s[1]) || !strcmp("compact", s[1])))
                || (j > 2 && 
                   (!strcmp("regular", s[2]) || !strcmp("compact", s[2])))) {
		    
		    arraytype = I_PRODUCT_P;

		} else {
                    
                    arraytype = I_IRREGULAR_P;
                }

		
	    } else {
		/* else positions isn't regular, or is grid
		 *  but not 0.0 origin or 1.0 deltas.
		 */
		if((k = ncvarid(hand, s[0])) < 0) {
		    DXSetError(ERROR_BAD_PARAMETER,"positions variable missing");
		    goto error;
		}
                ap->varid = k;
		varseries = is_seriesvariable(ap->cdfhandle, ap->varid, 
					      ap->recdim);
		
		/* regular grid, non-zero origin or non-unit spacing */
		if((j > 1 && 
                   (!strcmp("regular", s[1]) || !strcmp("compact", s[1])))
                || (j > 2 && 
                   (!strcmp("regular", s[2]) || !strcmp("compact", s[2])))) {

		    if(j > 2 && !strcmp("product", s[2]))
                        arraytype = I_PRODUCT_P;
                    else
                        arraytype = I_COMPACT_P;

		} else {
                    
                    arraytype = I_IRREGULAR_P;
                }
		
	    }
	    
            if(nopos)
                break;

	    indata = build_poscon(ap, arraytype, nterms);
	    terms[nterms] = indata;
            nterms++;
        }
    }

    /* if there is a positions component */
    if(!nopos) {
	if(!varseries) {
	    if(nterms > 1) {
		indata = (Object)DXNewProductArrayV(nterms, (Array *)terms);
		if(!indata) {
		    DXSetError(ERROR_BAD_PARAMETER, "bad positions attribute");
		    goto error;
		}
	    }
	    
	    for(i=0; i < series; i++) {

		f = (Field)DXGetEnumeratedMember((Group)o, i, NULL);

		/* and add it to field */
		f = DXSetComponentValue(f, GEOCOMPNAME, indata);
		f = DXSetComponentAttribute(f, DATCOMPNAME, 
					  "dep", STR(GEOCOMPNAME));
		if(!f)
		    goto error;
		
		/* and check for attributes if another variable was used */
		if(ap->varid != varid && !setattr(ap, f, GEOCOMPNAME))
		    goto error;
		
	    }

	} else {

	    if(nterms > 1) {
		t = (Object *)DXAllocateLocalZero(series * nterms * sizeof(Array));
		if(!t)
		    goto error;
		
		for(k=0; k < nterms; k++)
		    for(j=0; j < series; j++)
			t[j * nterms + k] = 
			  DXGetEnumeratedMember((Group)terms[k], j, NULL);

	    }

	    for(j=0; j < series; j++) {
		
		f = (Field)DXGetEnumeratedMember((Group)o, j, NULL);

		if(nterms > 1) {
		    arr = (Object)DXNewProductArrayV(nterms, 
						   (Array *)&t[j * nterms]);
		    if(!arr) {
			DXSetError(ERROR_BAD_PARAMETER, 
				 "bad positions attribute");
			goto error;
		    }
		} else
		    arr = DXGetEnumeratedMember((Group)indata, j, NULL);
		
		/* and add it to field */
		f = DXSetComponentValue(f, GEOCOMPNAME, arr);
		f = DXSetComponentAttribute(f, DATCOMPNAME, 
					  "dep", STR(GEOCOMPNAME));
		if(!f)
		    goto error;
		
		/* and check for attributes if another variable was used */
		if(ap->varid != varid && !setattr(ap, f, GEOCOMPNAME))
		    goto error;
	    }

	    if(t)
		DXFree((Pointer)t);
	}

	/* delete anything which is a series-in-a-single-netcdf-variable,
         *  because a temporary Group object was constructed to hold it.
         */
	for(j=0; j<nterms; j++)
	    if(DXGetObjectClass(terms[j]) == CLASS_GROUP)
		DXDelete(terms[j]);
    }
    

    /*
     * connections component 
     */
    varseries = 0;

    /* now look for the 'connections' information.  if there isn't
     *  anything, assume it's a regular grid.  if there is an attribute
     *  for 'connections' (currently 'topology' also still accepted in the 
     *  netCDF file), parse it.  if it's 'none', there isn't a topology
     *  component.  if it's 'regular', set it to a regular grid.  
     *  otherwise, it should be the name of another netCDF variable
     *  which contains the connections information.
     */
    if(!getattr(hand, varid, TOPOATTRIB, ap->stringattr,MAXNAME)
    && !getattr(hand, varid, OTOPOATTRIB, ap->stringattr,MAXNAME)) {
        nterms = 1;
	indata = build_poscon(ap, I_REGULAR_C, 0);
        eltype = STR(regularname[ap->data_ndims]);
	terms[0] = indata;

    } else {
        
	/* a connections attribute.  parse it */
	if(isattr(hand, varid, OTOPOATTRIB))
           DXWarning("'topology' attribute obsolete, use 'connections' instead");

	cp = ap->stringattr;
        nterms = 0;
	while(1) {
	    j = MAXATTRSTR;
	    cp = parseit(cp, &j, s);
	    if(j <= 0)
		break;
            
	    /* no topology */
	    if(!strcmp("none", s[0])) {
                nocon++;

	    } else if(!strcmp("regular", s[0])) {

		/* also accept this as regular grid */
                arraytype = I_REGULAR_C;
                eltype = STR(regularname[ap->data_ndims]);
                
	    } else if(j > 1 && !strcmp("product", s[1])) {
                
		if((k = ncvarid(hand, s[0])) < 0) {
		    DXSetError(ERROR_BAD_PARAMETER, 
                             "connections variable missing");
		    goto error;
		}
                ap->varid = k;
		varseries = is_seriesvariable(ap->cdfhandle, ap->varid, 
					      ap->recdim);
		
                arraytype = I_PRODUCT_C;
                
		/* if irregular, they must specify an element type */
		if(j < 3 || !s[2] || !s[2][0]) {
		    DXSetError(ERROR_BAD_PARAMETER,
			     "'element type' must be specified");
		    goto error;
                } else
                    eltype = STR(s[2]);
	    } else {
                
		/* irregular - look for the other netCDF variable */
		if((k = ncvarid(hand, s[0])) < 0) {
		    DXSetError(ERROR_BAD_PARAMETER, 
                             "connections variable missing");
		    goto error;
		}
                ap->varid = k;
		varseries = is_seriesvariable(ap->cdfhandle, ap->varid, 
					      ap->recdim);
		
                arraytype = I_IRREGULAR_C;
                
		/* if irregular, they must specify an element type */
		if(j < 2 || !s[1] || !s[1][0]) {
		    DXSetError(ERROR_BAD_PARAMETER,
			     "'element type' must be specified");
		    goto error;
                } else
                    eltype = STR(s[1]);
	    }
	    
            if(nocon)
                break;

	    indata = build_poscon(ap, arraytype, nterms);
            terms[nterms] = indata;
            nterms++;
	}

    }


    /* if there is a connections component */
    if(!nocon) {
	if(!varseries) {
	    if(nterms > 1) {
		indata = (Object)DXNewMeshArrayV(nterms, (Array *)terms);
		if(!indata) {
		    DXSetError(ERROR_BAD_PARAMETER, "bad connections attribute");
		    goto error;
		}
	    }
	    
	    for(j=0; j < series; j++) {

		f = (Field)DXGetEnumeratedMember((Group)o, j, NULL);

		/* and add it to field */
		f = DXSetComponentValue(f, TOPCOMPNAME, indata);
		f = DXSetComponentAttribute(f, TOPCOMPNAME, 
					  "element type", eltype);
		f = DXSetComponentAttribute(f, TOPCOMPNAME, 
					  "ref", STR(GEOCOMPNAME));
		if(!f)
		    goto error;

		
		/* and check for attributes if another variable was used */
		if(ap->varid != varid && !setattr(ap, f, TOPCOMPNAME))
		    goto error;
		
	    }

	} else {

	    if(nterms > 1) {
		t = (Object *)DXAllocateLocalZero(series * nterms * sizeof(Array));
		if(!t)
		    goto error;
		
		for(k=0; k < nterms; k++)
		    for(j=0; j < series; j++)
			t[j * nterms + k] = 
			  DXGetEnumeratedMember((Group)terms[k], j, NULL);

	    }

	    for(j=0; j < series; j++) {
		
		f = (Field)DXGetEnumeratedMember((Group)o, j, NULL);

		if(nterms > 1) {
		    arr = (Object)DXNewMeshArrayV(nterms, 
						(Array *)&t[j * nterms]);
		    if(!arr) {
			DXSetError(ERROR_BAD_PARAMETER, 
				 "bad connections attribute");
			goto error;
		    }
		} else
		    arr = DXGetEnumeratedMember((Group)indata, j, NULL);
		
		/* and add it to field */
		f = DXSetComponentValue(f, TOPCOMPNAME, arr);
		f = DXSetComponentAttribute(f, TOPCOMPNAME, 
					  "element type", eltype);
		f = DXSetComponentAttribute(f, TOPCOMPNAME, 
					  "ref", STR(GEOCOMPNAME));
		if(!f)
		    goto error;

		
		/* and check for attributes if another variable was used */
		if(ap->varid != varid && !setattr(ap, f, TOPCOMPNAME))
		    goto error;
	    }

	    if(t)
		DXFree((Pointer)t);
	}

	/* delete anything which is a series-in-a-single-netcdf-variable,
         *  because a temporary Group object was constructed to hold it.
         */
	for(j=0; j<nterms; j++)
	    if(DXGetObjectClass(terms[j]) == CLASS_GROUP)
		DXDelete(terms[j]);

    }
    
    
    
    /* 
     * additional components
     */
    
    /* are there additional components that should be added?
     */
    if(getattr(hand, varid, COMPATTRIB, ap->stringattr,MAXNAME)) {
        
	cp = ap->stringattr;
	while(1) {
	    j = MAXATTRSTR;
	    cp = parseit(cp, &j, s);
	    
	    /* field:component = "a, b, c";  attribute required.  all three
	     *  parameters must be present for additional components.
	     *   a = name of variable.  
	     *   b = component name
	     *   c = rank (scalar, vector, matrix, tensor)
	     */
	    if(j <= 0)
                break;

            if(j < 3) {
                DXSetError(ERROR_BAD_PARAMETER, 
                         "varname, compname, shape required");
		goto error;
            }
            
	    if((k = ncvarid(hand, s[0])) < 0) {
		DXSetError(ERROR_BAD_PARAMETER, 
			 "component variable '%s' not found", s[0]);
		goto error;
	    }
            
            ap->varid = k;
	    ap->arrayrank = setrank(s[2]);
	    varseries = is_seriesvariable(ap->cdfhandle, ap->varid, ap->recdim);
            
	    /* actually read the data into an Array */
	    if(!(indata = build_array(ap)))
		goto error;

            for(i=0; i<series; i++) {

		f = (Field)DXGetEnumeratedMember((Group)o, i, NULL);

		/* and add it to the field with the given component name */
		arr = varseries ? DXGetEnumeratedMember((Group)indata, i, NULL) 
		                : indata;
	    
		if(!DXSetComponentValue(f, s[1], arr))
		    goto error;
	
		/* and check for attributes */
		if(!setattr(ap, f, s[1]))
		    goto error;

	    }

	    if(varseries)
		DXDelete(indata);
	}
        
    }
    
#ifdef DO_ENDFIELD
    for(i=0; i<series; i++)
	DXEndField((Field)DXGetSeriesMember((Series)o, i, &value));
#endif

    return o;

  error:
    DXDelete((Object)f);
    DXDelete(o);
    DXFree((Pointer)t);	    
    return NULL;

}


/* verify that start and end are not out of range.
 */
static Error check_serieslength(Arrayinfo ap)
{
    long size;

    if(ncdiminq(ap->cdfhandle,		/* file descriptor */
		ap->recdim,		/* record dimension id */
		(char *)0,		/* dimension name */
		&size) < 0)		/* record size */
	DXErrorReturn(ERROR_INTERNAL, "netCDF library error");

    if (ap->vp->startframe && *ap->vp->startframe < 0) {
	DXSetError(ERROR_BAD_PARAMETER, "start must be a non-negative integer");
	return ERROR;
    }

    if (ap->vp->endframe && *ap->vp->endframe > size) {
	DXSetError(ERROR_BAD_PARAMETER, "end is larger than series members");
	return ERROR;
    }

    return OK;
}

/* given the dimension number of the unlimited dimension, find out how
 *  long it is.
 */
static int get_serieslength(Arrayinfo ap)
{
    int cnt;
    long size;
    int start, end, delta;

    if(ncdiminq(ap->cdfhandle,		/* file descriptor */
		ap->recdim,		/* record dimension id */
		(char *)0,		/* dimension name */
		&size) < 0)		/* record size */
	DXErrorReturn(ERROR_INTERNAL, "netCDF library error");
    
    start = ap->vp->startframe ? *ap->vp->startframe : 0;
    end = ap->vp->endframe ? *ap->vp->endframe : size-1;
    delta = ap->vp->deltaframe ? *ap->vp->deltaframe : 1;

    if(delta == 0)
	return 0;

    cnt = 0;
    for (size=start; size<=end; size+=delta)
	cnt++;

    return (cnt);
}

/* given a variable and the record dimension id, return whether that variable
 *  uses that id.
 */
static int is_seriesvariable(int hand, int varid, int recdim)
{
    nc_type datatype;
    int natts, ndims;
    int dim[MAX_VAR_DIMS];
    
    /* inquire about the variable - it's size, shape and type */
    if(ncvarinq(hand,                      /* file descriptor */
		varid,                     /* variable ID */ 
		(char *)0,                 /* variable name */
		&datatype,                 /* data item type */
		&ndims,                    /* number of dimensions */
		dim,                       /* array of dimension ID's */
		&natts) < 0)               /* number of attributes */
	DXErrorReturn(ERROR_INTERNAL, "netCDF library error");


    /* if the first variable is the unlimited dimension, this is a series.
     */
    return (dim[0] == recdim);
}


/* given a variable which uses the record dimension, and an index,
 *  find out what number is associated with that record number.
 */
static Error get_seriesvalue(Arrayinfo ap, float **valuelist)
{
#if 0
    return (float)index;
#else
    /* bother.  what makes sense for this is: to have a separate netCDF
     *  variable, a 1D array using the unlimited dimension, which contains
     *  the series value for each dimension.  but how to you specify it?
     *  as a 'var1:series = seriesvar' attribute on the data field?
     */
    nc_type datatype;
    int varid;
    int i, j, natts, ndims;
    int dim[MAX_VAR_DIMS];
    long size, start;
    int delta;
    char *s[MAXATTRSTR];
    

    /* value list comes in as NULL.  if there is no series positions
     *  arrray, just return.  else try to read in the variable, allocate
     *  space for the array, and return with valuelist pointing to it.
     */
    if(!getattr(ap->cdfhandle, ap->varid, SERIESPOSATTRIB, ap->stringattr,MAXNAME))
	return OK;

    j = MAXATTRSTR;
    parseit(ap->stringattr, &j, s);
	
    if(j != 1)
	DXErrorReturn(ERROR_DATA_INVALID, "bad seriesposition attribute");
    
    if((varid = ncvarid(ap->cdfhandle, s[0])) < 0) {
	DXSetError(ERROR_BAD_PARAMETER, 
		    "seriesposition variable '%s' not found", s[0]);
	return ERROR;
    }
    
    
    /* inquire about the variable - it's size, shape and type */
    if(ncvarinq(ap->cdfhandle,          /* file descriptor */
		varid,                  /* variable ID */ 
		(char *)0,              /* variable name */
		&datatype,              /* data item type */
		&ndims,                 /* number of dimensions */
		dim,                    /* array of dimension ID's */
		&natts) < 0)            /* number of attributes */
	DXErrorReturn(ERROR_INTERNAL, "netCDF library error");

    if(dim[0] != ap->recdim)
	DXErrorReturn(ERROR_DATA_INVALID, 
		  "seriespositions array must use unlimited record dimension");

    if(datatype != NC_FLOAT)
	DXErrorReturn(ERROR_DATA_INVALID, 
		    "seriespositions array must be type float");

    if(ndims != 1)
	DXErrorReturn(ERROR_DATA_INVALID, "seriespositions array must be 1D");
	
    size = get_serieslength(ap);
    start = ap->vp->startframe ? *ap->vp->startframe : 0;
    delta = ap->vp->deltaframe ? *ap->vp->deltaframe : 1;

    if(!(*valuelist = (float *)DXAllocateLocal(sizeof(float) * size)))
       return ERROR;

    if(delta == 1) {
	if(ncvarget(ap->cdfhandle,	/* file descriptor */
		    varid,		/* variable ID */
		    &start,		/* array-origin for each dim */
		    &size,              /* array-counts along each dim */
		    (void *)*valuelist) < 0) /* where values are returned */
	    goto error;
    } else {
	for(i=0; i<size; i++) {
	    if(ncvarget1(ap->cdfhandle, varid, &start,
			                (void *)&valuelist[i]) < 0)
		goto error;
	    
	    start += delta;
	}
    }
    return OK;

  error:
    DXFree((Pointer) *valuelist);
    DXErrorReturn(ERROR_INTERNAL, "netCDF library error");
    
#endif
}

/*
 * read a variable into global memory, and build up a field.
 */
static Object build_field(int hand, int varid)
{
    Array indata, terms[MAXRANK];
    int nopos = 0, nocon = 0;
    struct arrayinfo a, *ap = &a;
    int arraytype = I_NONE;
    char *cp, *s[MAXATTRSTR];
    int i, j, k;
    Field f = NULL;
    Object eltype = NULL;
    

    /* set up the Arrayinfo struct
     */
    ap->cdfhandle = hand;
    ap->varid = varid;
    ap->recdim = INVALID_DIMID;


    /* get the attribute value.
     */
    if(getattr(hand, varid, FIELDATTRIB, ap->stringattr,MAXNAME)) {
    
	cp = ap->stringattr;
	j = MAXATTRSTR;
	cp = parseit(cp, &j, s);
	if(j <= 0)
	    return NULL;
    } else {
	if (!variablename(hand, varid, ap->stringattr))
	    return NULL;

	s[0] = ap->stringattr;
	s[1] = NULL;
	s[2] = NULL;
	j = 1;
    }

    
    
    /* 
     * data component
     */
    
    /* decide if it's scalar, vector or matrix data */
    ap->arrayrank = setrank(s[1]);

    /* this routine reads a netCDF variable into an Array.
     *  in this case, it's reading in the data component.
     */
    if(!(indata = (Array)build_data(ap, INVALID_DIMID)))
	    goto error;

    f = DXNewField();
    if(!f)
	goto error;
	
    if(!DXSetComponentValue(f, DATCOMPNAME, (Object)indata))
	goto error;
	
    /* look here for more attributes on the data component - to set
     *  data component attributes.
     */
    if(!setattr(ap, f, DATCOMPNAME))
	goto error;
    
    /*
     * positions component
     */
    
    ap->arrayrank = 1;   /* unless they explicitly say 'scalar' */
    
    /* look for a positions (old points) component.  if the data variable
     *  doesn't have a 'positions' attribute, assume a regularly spaced grid.
     */
    if(!getattr(hand, varid, GEOMATTRIB, ap->stringattr,MAXNAME)
    && !getattr(hand, varid, OGEOMATTRIB, ap->stringattr,MAXNAME)) {
        i = 1;
        indata = (Array)build_poscon(ap, I_REGULAR_P, 0);
    
    } else {
	/* else they gave us a geometry.  parse it. 
	 */
	
	if(isattr(hand, varid, OGEOMATTRIB))
	    DXWarning("'geometry' attribute obsolete, use 'positions' instead");
        
	cp = ap->stringattr;
        i = 0;
	while(1) {
	    j = MAXATTRSTR;
	    cp = parseit(cp, &j, s);
	    if(j <= 0)
		break;
            
	    /* positions are vectors unless specifically told scalar */
	    if((j > 1 && !strcmp("scalar", s[1]))
	     ||(j > 2 && !strcmp("scalar", s[2])))
		ap->arrayrank = 0;
            
	    if(!strcmp("none", s[0])) {
		
		/* no positions - should this be ok? */
		nopos++;
		
	    } else if(!strcmp("regular", s[0])) {
		
		/* 'regular' is same as default */
		arraytype = I_REGULAR_P;
		
	    } else if((j > 1 && !strcmp("product", s[1]))
                    ||(j > 2 && !strcmp("product", s[2]))) {
		

		if((k = ncvarid(hand, s[0])) < 0) {
		    DXSetError(ERROR_BAD_PARAMETER,"positions variable missing");
		    goto error;
		}
                ap->varid = k;
		
		/* regular grid, non-zero origin or non-unit spacing */
		if((j > 1 && 
                   (!strcmp("regular", s[1]) || !strcmp("compact", s[1])))
                || (j > 2 && 
                   (!strcmp("regular", s[2]) || !strcmp("compact", s[2])))) {
		    
		    arraytype = I_PRODUCT_P;

		} else {
                    
                    arraytype = I_IRREGULAR_P;
                }

		
	    } else {
		/* else positions isn't regular, or is grid
		 *  but not 0.0 origin or 1.0 deltas.
		 */
		if((k = ncvarid(hand, s[0])) < 0) {
		    DXSetError(ERROR_BAD_PARAMETER,"positions variable missing");
		    goto error;
		}
                ap->varid = k;
		
		/* regular grid, non-zero origin or non-unit spacing */
		if((j > 1 && 
                   (!strcmp("regular", s[1]) || !strcmp("compact", s[1])))
                || (j > 2 && 
                   (!strcmp("regular", s[2]) || !strcmp("compact", s[2])))) {

		    if(j > 2 && !strcmp("product", s[2]))
                        arraytype = I_PRODUCT_P;
                    else
                        arraytype = I_COMPACT_P;

		} else {
                    
                    arraytype = I_IRREGULAR_P;
                }
		
	    }
	    
            if(nopos)
                break;

            indata = (Array)build_poscon(ap, arraytype, i);
            terms[i] = indata;
            i++;
        }
    }

    /* if there is a positions component */
    if(!nopos) {
        if(i > 1) {
            indata = (Array)DXNewProductArrayV(i, terms);
            if(!indata) {
                DXSetError(ERROR_BAD_PARAMETER, "bad positions attribute");
                goto error;
            }
        }
        
        /* and add it to field */
        f = DXSetComponentValue(f, GEOCOMPNAME, (Object)indata);
        f = DXSetComponentAttribute(f, DATCOMPNAME, "dep", STR(GEOCOMPNAME));
        if(!f)
            goto error;
        
        /* and check for attributes if another variable was used */
        if(ap->varid != varid && !setattr(ap, f, GEOCOMPNAME))
            goto error;
        
    }
    

    /*
     * connections component 
     */

    /* now look for the 'connections' information.  if there isn't
     *  anything, assume it's a regular grid.  if there is an attribute
     *  for 'connections' (currently 'topology' also still accepted in the 
     *  netCDF file), parse it.  if it's 'none', there isn't a topology
     *  component.  if it's 'regular', set it to a regular grid.  
     *  otherwise, it should be the name of another netCDF variable
     *  which contains the connections information.
     */
    if(!getattr(hand, varid, TOPOATTRIB, ap->stringattr,MAXNAME)
    && !getattr(hand, varid, OTOPOATTRIB, ap->stringattr,MAXNAME)) {
        i = 1;
        indata = (Array)build_poscon(ap, I_REGULAR_C, 0);
        eltype = STR(regularname[ap->data_ndims]);

    } else {
        
	/* a connections attribute.  parse it */
	if(isattr(hand, varid, OTOPOATTRIB))
           DXWarning("'topology' attribute obsolete, use 'connections' instead");

	cp = ap->stringattr;
        i = 0;
	while(1) {
	    j = MAXATTRSTR;
	    cp = parseit(cp, &j, s);
	    if(j <= 0)
		break;
            
	    /* no topology */
	    if(!strcmp("none", s[0])) {
                nocon++;

	    } else if(!strcmp("regular", s[0])) {

		/* also accept this as regular grid */
                arraytype = I_REGULAR_C;
                eltype = STR(regularname[ap->data_ndims]);
                
	    } else if(j > 1 && !strcmp("product", s[1])) {
                
		if((k = ncvarid(hand, s[0])) < 0) {
		    DXSetError(ERROR_BAD_PARAMETER, 
                             "connections variable missing");
		    goto error;
		}
                ap->varid = k;
                
                arraytype = I_PRODUCT_C;
                
		/* if irregular, they must specify an element type */
		if(j < 3 || !s[2] || !s[2][0]) {
		    DXSetError(ERROR_BAD_PARAMETER,
			     "'element type' must be specified");
		    goto error;
                } else
                    eltype = STR(s[2]);
	    } else {
                
		/* irregular - look for the other netCDF variable */
		if((k = ncvarid(hand, s[0])) < 0) {
		    DXSetError(ERROR_BAD_PARAMETER, 
                             "connections variable missing");
		    goto error;
		}
                ap->varid = k;
                
                arraytype = I_IRREGULAR_C;
                
		/* if irregular, they must specify an element type */
		if(j < 2 || !s[1] || !s[1][0]) {
		    DXSetError(ERROR_BAD_PARAMETER,
			     "'element type' must be specified");
		    goto error;
                } else
                    eltype = STR(s[1]);
	    }
	    
            if(nocon)
                break;

            indata = (Array)build_poscon(ap, arraytype, i);
            terms[i] = indata;
            i++;
	}

    }
    
    /* if there is a connections component */
    if(!nocon) {
        if(i > 1) {
            indata = (Array)DXNewMeshArrayV(i, terms);
            if(!indata) {
                DXSetError(ERROR_BAD_PARAMETER, "bad connections attribute");
                goto error;
            }
        }
        
        /* and add it to field */
        f = DXSetComponentValue(f, TOPCOMPNAME, (Object)indata);
        f = DXSetComponentAttribute(f, TOPCOMPNAME, "element type", eltype);
        f = DXSetComponentAttribute(f, TOPCOMPNAME, "ref", STR(GEOCOMPNAME));
        if(!f)
            goto error;
        
        /* and check for attributes if another variable was used */
        if(ap->varid != varid && !setattr(ap, f, TOPCOMPNAME))
            goto error;
        
    }
    
    
    /* 
     * additional components
     */
    
    /* are there additional components that should be added?
     */
    if(getattr(hand, varid, COMPATTRIB, ap->stringattr,MAXNAME)) {
        
	cp = ap->stringattr;
	while(1) {
	    j = MAXATTRSTR;
	    cp = parseit(cp, &j, s);
	    
	    /* field:component = "a, b, c";  attribute required.  all three
	     *  parameters must be present for additional components.
	     *   a = name of variable.  
	     *   b = component name
	     *   c = rank (scalar, vector, matrix, tensor)
	     */
	    if(j <= 0)
                break;

            if(j < 3) {
                DXSetError(ERROR_BAD_PARAMETER, 
                         "varname, compname, shape required");
		goto error;
            }
            
	    if((k = ncvarid(hand, s[0])) < 0) {
		DXSetError(ERROR_BAD_PARAMETER, 
			 "component variable '%s' not found", s[0]);
		goto error;
	    }
            
            ap->varid = k;
	    ap->arrayrank = setrank(s[2]);
            
	    /* actually read the data into an Array */
	    if(!(indata = (Array)build_array(ap)))
		goto error;
            
	    /* and add it to the field with the given component name */
	    if(!DXSetComponentValue(f, s[1], (Object)indata))
		goto error;
            
            /* and check for attributes */
            if(!setattr(ap, f, s[1]))
                goto error;
	}
        
    }
    
#ifdef DO_ENDFIELD
    f = DXEndField(f); 
#endif
    return (Object)f;

  error:
    DXDelete((Object)f);
    return NULL;
}


/* single routine for deciding what routine to call to actually construct
 *  either a positions or connections array.  (it's called from more than
 *  one place, so it's here to keep the calls to the routines from being
 *  scattered thoughout the code)
 */
static Object
build_poscon(Arrayinfo ap, int flag, int dim)
{
    Object indata = NULL;
    
    switch(flag) {
        
      case I_NONE:
        break;
       
      case I_COMPACT_P:
        /* regular grid, non-zero origin or non-unit spacing */
        indata = build_regpos(ap, 0);
        break;
        
      case I_REGULAR_P:
        /* origin at 0.0, deltas 1.0, regular grid */
        indata = build_regpos(ap, 1);
        break;
        
      case I_REGULAR_C:
        /* N-dim connected grid */
        indata = build_regcon(ap);
        break;
        
      case I_IRREGULAR_P:
      case I_IRREGULAR_C:
        /* explicit positions or connections list */
        indata = build_array(ap);
        break;

      case I_PRODUCT_P:
        /* indata = build_regpos(ap, 0); */
        indata = build_regpos1D(ap, 0, dim);
        break;

      case I_PRODUCT_C:
        indata = build_regcon1D(ap, dim);
        break;

      default:
        DXSetError(ERROR_INTERNAL, "bad case in build_pos switch statement");
        break;
    }
    
    return indata;
}




/* this code reads in a single variable into an array and also sets
 *  up the AI struct for later.  if recdim is valid, it is the id of
 *  the variable used for the series dimension.  it needs to be the
 *  first variable if it is used.
 */
static Object
build_data(Arrayinfo ap, int recdim)
{
    nc_type datatype;
    int i, j, size, natts, series;
    int start, end, delta;
    int dim[MAX_VAR_DIMS];
    long dstart[MAX_VAR_DIMS];
    long long_datacounts[MAXRANK];
    VOID *dataval;
    Object o = NULL;
    Array adata = NULL;
    long *tempbuf;
    int k,n,ndims=1;
    int *data_int;
    
    /* inquire about the variable - it's size, shape and type */
    if(ncvarinq(ap->cdfhandle,             /* file descriptor */
		ap->varid,                 /* variable ID */ 
		(char *)0,                 /* variable name */
		&datatype,                 /* data item type */
		&ap->data_ndims,           /* number of dimensions */
		dim,                       /* array of dimension ID's */
		&natts) < 0)               /* number of attributes */
	DXErrorReturn(ERROR_INTERNAL, "netCDF library error");

	
    if(ap->data_ndims <= 0){
	DXSetError(ERROR_INTERNAL, "%s variable with no dimensions",
	ap->stringattr);
	return ERROR;
    }
    
    /* figure out how big it is - total number of items, taking into
     *  account that if it is an array of vectors or matricies, the
     *  quickest varying dimension isn't part of the total shape.
     */
    size = 1;
    for (i=0; i<ap->data_ndims; i++) {
	ncdiminq (ap->cdfhandle, dim[i], (char *)0, &long_datacounts[i]);
	ap->datacounts[i] = long_datacounts[i];
	dstart[i] = 0;
	if(i < ap->data_ndims - ap->arrayrank)
	    size *= ap->datacounts[i];
    }
    
    /* convert from netCDF data flags to SVS data flags */
    switch(datatype) {
      case NC_CHAR:   ap->arraytype = TYPE_UBYTE;  break;
      case NC_SHORT:  ap->arraytype = TYPE_SHORT;  break;
      case NC_LONG:   ap->arraytype = TYPE_INT;    break;
      case NC_FLOAT:  ap->arraytype = TYPE_FLOAT;  break;
      case NC_DOUBLE: ap->arraytype = TYPE_DOUBLE; break;
      default:        ap->arraytype = TYPE_UBYTE;  break;   
	/* we need a TYPE_BYTE */
    }
    

    /* if the first variable is the unlimited dimension, this is a series.
     */
    series = (dim[0] == recdim);

    /* create a group to hold the output, and don't count the size of 
     *  the first dimension in the data.
     */
    if(series) {
	o = (Object)DXNewSeries();
	size /= ap->datacounts[0];

	start = ap->vp->startframe ? *ap->vp->startframe : 0;
	end = ap->vp->endframe ? *ap->vp->endframe : ap->datacounts[0]-1;
	delta = ap->vp->deltaframe ? *ap->vp->deltaframe : 1;

	if(delta == 0) {
	    DXSetError(ERROR_DATA_INVALID, "delta is zero");
	    goto error;
	}

	ap->datacounts[0] = 1;
	long_datacounts[0] = 1;
    } else {
	start = 0; 
	end = 0;
	delta = 1;
    }

    for(i=start, j=0; i<=end; i+=delta, j++) {
    
	/* variables are labeled as a vector/matrix/tensor.  the outermost 
	 *  (last) variable must vary fastest.  if not scalar, pass the shape 
	 *  of the vector or matrix into DXNewArray as shape.
	 */  
	adata = DXNewArrayV (ap->arraytype, CATEGORY_REAL, ap->arrayrank, 
			    &ap->datacounts[ap->data_ndims - ap->arrayrank]);
    
	adata = DXAddArrayData (adata, 0, size, NULL);

	if(series)
	    dstart[0] = i;

	/* if data is long, create a temp long buffer to copy 
	   netCDF data to then move to the SVS array buffer */
	if (ap->arraytype == TYPE_INT) {
	   data_int = (int *)DXGetArrayData (adata);
	   if(!data_int)
	       goto error;
	   for (k=0,n=ap->data_ndims - ap->arrayrank; k<ap->arrayrank; k++,n++) 
	      ndims *= ap->datacounts[n];
	   tempbuf = (long *)DXAllocate(ndims * size * sizeof(long));
	   if(ncvarget(ap->cdfhandle,          /* netcdf file handle */
		    ap->varid,              /* variable id */
		    dstart,                 /* array-origin for each dim */
		    long_datacounts,         /* array-counts along each dim */
		    tempbuf) < 0) {         /* memory pointer */
	       DXSetError(ERROR_INTERNAL, "netCDF library error");
	       goto error;
	   }
	   for (k=0; k<size*ndims; k++) 
	      data_int[k] = (int)tempbuf[k];
 	   DXFree((Pointer)tempbuf);
	}
	else {

	dataval = DXGetArrayData (adata);
	if(!dataval)
	    goto error;
	
	/* do the memory copy from netCDF buffer to SVS array buffer */
	if(ncvarget(ap->cdfhandle,          /* netcdf file handle */
		    ap->varid,              /* variable id */
		    dstart,                 /* array-origin for each dim */
		    long_datacounts,         /* array-counts along each dim */
		    dataval) < 0) {         /* memory pointer */
	    DXSetError(ERROR_INTERNAL, "netCDF library error");
	    goto error;
	}
  	}	

	if(series) {
	    if (!DXSetSeriesMember((Series)o, j, (double)i, (Object)adata))
		goto error;
	
	    adata = NULL;
	}
    }
    
    /* account for a vector or matrix at each location, and also series.
     */
    ap->data_ndims -= ap->arrayrank;
    if(series) {
	ap->data_ndims--;
	for(i=0; i < ap->data_ndims; i++){
	    ap->datacounts[i] = ap->datacounts[i+1];
	    long_datacounts[i] = long_datacounts[i+1];
	}
    }
    
    /* if single array instead of group */
    if(!o)
	o = (Object)adata;

    return(o);

  error:
    DXDelete(o);
    DXDelete((Object)adata);
    return NULL;
}


/* this code reads in a single variable into an array, using an AI struct
 *  in which the shape of the data has already been initialized.
 */
static Object
build_array(Arrayinfo ap)
{
    nc_type datatype;
    int i, size, natts, series;
    int start, end, delta;
    int dim[MAX_VAR_DIMS];
    long dstart[MAX_VAR_DIMS];
    long long_arraycounts[MAXRANK];
    VOID *dataval;
    Object o = NULL;
    Array adata = NULL;
    long *tempbuf;
    int k,n,ndims=1;
    int *data_int;

    
    /* inquire about the variable - it's size, shape and type */
    if(ncvarinq(ap->cdfhandle,             /* file descriptor */
		ap->varid,                 /* variable ID */ 
		(char *)0,                 /* variable name */
		&datatype,                 /* data item type */
		&ap->array_ndims,          /* number of dimensions */
		dim,                       /* array of dimension ID's */
		&natts) < 0)               /* number of attributes */
	DXErrorReturn(ERROR_INTERNAL, "netCDF library error");

	
    if(ap->array_ndims < 0)
	DXErrorReturn(ERROR_INTERNAL, "netCDF variable with no dimensions");
    
    /* figure out how big it is - total number of items, taking into
     *  account that if it is an array of vectors or matricies, the
     *  quickest varying dimension isn't part of the total shape.
     */
    size = 1;
    for (i=0; i<ap->array_ndims; i++) {
	ncdiminq (ap->cdfhandle, dim[i], (char *)0, &long_arraycounts[i]);
	ap->arraycounts[i] = long_arraycounts[i];
	dstart[i] = 0;
	if(i < ap->array_ndims - ap->arrayrank)
	    size *= ap->arraycounts[i];
    }
    
    /* convert from netCDF data flags to SVS data flags */
    switch(datatype) {
      case NC_CHAR:   ap->arraytype = TYPE_UBYTE;  break;
      case NC_SHORT:  ap->arraytype = TYPE_SHORT;  break;
      case NC_LONG:   ap->arraytype = TYPE_INT;    break;
      case NC_FLOAT:  ap->arraytype = TYPE_FLOAT;  break;
      case NC_DOUBLE: ap->arraytype = TYPE_DOUBLE; break;
      default:        ap->arraytype = TYPE_UBYTE;  break;   
	/* we need a TYPE_BYTE */
    }
    

    /* if the first variable is the unlimited dimension, this is a series.
     */
    series = (dim[0] == ap->recdim);

    /* create a group to hold the output, and don't count the size of 
     *  the first dimension in the data.
     */
    if(series) {
	o = (Object)DXNewGroup();
	size /= ap->arraycounts[0];

	start = ap->vp->startframe ? *ap->vp->startframe : 0;
	end = ap->vp->endframe ? *ap->vp->endframe : ap->arraycounts[0]-1;
	delta = ap->vp->deltaframe ? *ap->vp->deltaframe : 1;

	if(delta == 0) {
	    DXSetError(ERROR_DATA_INVALID, "delta is zero");
	    goto error;
	}

	ap->arraycounts[0] = 1;
	long_arraycounts[0] =1;
    } else {
	start = 0; 
	end = 0;
	delta = 1;
    }

    for(i=start; i<=end; i+=delta) {
    
	/* variables are labeled as a vector/matrix/tensor.  the outermost 
	 *  (last) variable must vary fastest.  if not scalar, pass the shape 
	 *  of the vector or matrix into DXNewArray as shape.
	 */  
	adata = DXNewArrayV (ap->arraytype, CATEGORY_REAL, ap->arrayrank, 
			    &ap->arraycounts[ap->array_ndims - ap->arrayrank]);
    
    
    
	adata = DXAddArrayData (adata, 0, size, NULL);

	if(series)
	    dstart[0] = i;

	/* if data is long, create a temp long buffer to copy 
	   netCDF data to then move to the SVS array buffer */
	if (ap->arraytype == TYPE_INT) {
	   data_int = DXGetArrayData (adata);
	   if(!data_int)
	       goto error;
	   for (k=0, n=ap->array_ndims - ap->arrayrank; k<ap->arrayrank;k++,n++)
	      ndims *=ap->arraycounts[n];
	   tempbuf = (long *)DXAllocate(size * ndims * sizeof(long));
	   if(ncvarget(ap->cdfhandle,          /* netcdf file handle */
		    ap->varid,              /* variable id */
		    dstart,                 /* array-origin for each dim */
		    long_arraycounts,         /* array-counts along each dim */
		    tempbuf) < 0) {         /* memory pointer */
	       DXSetError(ERROR_INTERNAL, "netCDF library error");
	       goto error;
	   }
	   for (k=0; k<size*ndims; k++) 
	      data_int[k] = (int)tempbuf[k];
 	   DXFree((Pointer)tempbuf);
	}
	else {
	dataval = DXGetArrayData (adata);
	if(!dataval)
	    goto error;
	/* do the memory copy from netCDF buffer to SVS array buffer */
	if(ncvarget(ap->cdfhandle,          /* netcdf file handle */
		    ap->varid,              /* variable id */
		    dstart,                 /* array-origin for each dim */
		    long_arraycounts,        /* array-counts along each dim */
		    dataval) < 0) {         /* memory pointer */
	    DXSetError(ERROR_INTERNAL, "netCDF library error");
	    goto error;
	}
 	}	

	if(series) {
	    if (!DXSetMember((Group)o, NULL, (Object)adata))
		goto error;
	    
	    adata = NULL;
	}
	
    }


    /* if single array instead of group */
    if(!o)
	o = (Object)adata;

    return(o);

  error:
    DXDelete(o);
    DXDelete((Object)adata);
    return NULL;
}


/* 
 * construct a regular Ndim positions component -
 *   either origins 0.0, deltas 1.0, or user specified origins & deltas
 */
static Object build_regpos(Arrayinfo ap, int regular)
{
    int i, j, k, loops;
    long index[3];
    float origins[MAX_VAR_DIMS], deltas[MAXRANK*MAXRANK];
    int start, delta;
    Array indata;
    Object o = NULL;

    memset((char *)deltas, '\0', sizeof(deltas));

    /* setup arrays for each dimension */
    if(regular) {
	
	for (i=0; i<ap->data_ndims; i++) {
	    origins[i] = 0.0;
	    j = ap->data_ndims * i + i;
	    deltas[j] = 1.0;
	}
	
	/* actually make the positions Array */
	indata = DXMakeGridPositionsV(ap->data_ndims, ap->datacounts, 
				    origins, deltas);
	
	return((Object)indata);
    } 

    /* not completely regular */
	
    /* is single positions array shared, or is it a series? */
    if(!is_seriesvariable(ap->cdfhandle, ap->varid, ap->recdim)) {

	for (i=0; i<ap->data_ndims; i++) {
	    
	    /* origins */
	    index[0] = i;  
	    index[1] = 0;
	    if(ncvarget1(ap->cdfhandle, ap->varid, index, &origins[i]) < 0)
		DXErrorReturn(ERROR_INTERNAL, 
			    "netCDF library error getting origins");
	    
	    /* deltas */
	    index[1] = 1;
	    j = ap->data_ndims * i + i;
	    if(ncvarget1(ap->cdfhandle, ap->varid, index, &deltas[j]) < 0)
		DXErrorReturn(ERROR_INTERNAL, 
			    "netCDF library error getting deltas");
	    
	}

	/* actually make the positions Array */
	indata = DXMakeGridPositionsV(ap->data_ndims, ap->datacounts, 
				    origins, deltas);
	
	return((Object)indata);
    }

    /* positions are also series */
    loops = get_serieslength(ap);
    start = ap->vp->startframe ? *ap->vp->startframe : 0;
    delta = ap->vp->deltaframe ? *ap->vp->deltaframe : 1;

    for (j=0; j<loops; j++) {

	memset((char *)deltas, '\0', sizeof(deltas));

	for (i=0; i<ap->data_ndims; i++) {
	    
	    /* origins */
	    index[0] = start;
	    index[1] = i;  
	    index[2] = 0;
	    if(ncvarget1(ap->cdfhandle, ap->varid, index, &origins[i]) < 0)
		DXErrorReturn(ERROR_INTERNAL, 
			    "netCDF library error getting origins");
	    
	    /* deltas */
	    index[2] = 1;
	    k = ap->data_ndims * i + i;
	    if(ncvarget1(ap->cdfhandle, ap->varid, index, &deltas[k]) < 0)
		DXErrorReturn(ERROR_INTERNAL, 
			    "netCDF library error getting deltas");
	    
	}
	
	/* make the positions Array for one timestep */
	indata = DXMakeGridPositionsV(ap->data_ndims, ap->datacounts, 
				  origins, deltas);
	
	if(!o)
	    o = (Object)DXNewGroup();
	o = (Object)DXSetMember((Group)o, NULL, (Object)indata);
	if(!o)
	    return NULL;

	start += delta;
    }
    
    return o;
}



/* 
 * construct a single regular array -
 *   either origin 0.0, delta 1.0, or user specified origin & deltas
 */
static Object build_regpos1D(Arrayinfo ap, int regular, int dim)
{
    int i, j, loops;
    long index[3];
    float origins[MAX_VAR_DIMS], deltas[MAXRANK*MAXRANK];
    int start, delta;
    Array indata;
    Object o = NULL;

    memset((char *)deltas, '\0', sizeof(deltas));

    /* setup an array for one dimension */
    if(regular) {
	
	for (i=0; i<ap->data_ndims; i++)
	    origins[0] = 0.0;
	
	deltas[dim] = 1.0;
	
	indata = (Array)DXNewRegularArray(TYPE_FLOAT, ap->data_ndims,
					ap->datacounts[dim], 
					(Pointer)origins, (Pointer)deltas);

	return((Object)indata);
    }

    /* is the positions variable also a series, or constant? */
    if(!is_seriesvariable(ap->cdfhandle, ap->varid, ap->recdim)) {

	/* origins */
	index[0] = 0;  
	for (i=0; i<ap->data_ndims; i++) {
	    index[1] = i;
	    if(ncvarget1(ap->cdfhandle, ap->varid, index, &origins[i]) < 0)
		DXErrorReturn(ERROR_INTERNAL, 
				"netCDF library error getting origins");
	}
        
	/* deltas */
	index[0] = 1;
	for (i=0; i<ap->data_ndims; i++) {
	    index[1] = i;
	    if(ncvarget1(ap->cdfhandle, ap->varid, index, &deltas[i]) < 0)
		DXErrorReturn(ERROR_INTERNAL, 
			    "netCDF library error getting deltas");
	}
        

	indata = (Array)DXNewRegularArray(TYPE_FLOAT, ap->data_ndims,
					ap->datacounts[dim], 
					(Pointer)origins, (Pointer)deltas);

	return((Object)indata);
    }
    
    /* positions are also series */
    loops = get_serieslength(ap);
    start = ap->vp->startframe ? *ap->vp->startframe : 0;
    delta = ap->vp->deltaframe ? *ap->vp->deltaframe : 1;

    for (j=0; j<loops; j++) {
	for (i=0; i<ap->data_ndims; i++) {
	    
	    /* origins */
	    index[0] = start;  
	    index[1] = 0;  
	    for (i=0; i<ap->data_ndims; i++) {
		index[2] = i;
		if(ncvarget1(ap->cdfhandle, ap->varid, index, &origins[i]) < 0)
		    DXErrorReturn(ERROR_INTERNAL, 
				"netCDF library error getting origins");
	    }
	    
	    /* deltas */
	    index[1] = 1;
	    for (i=0; i<ap->data_ndims; i++) {
		index[2] = i;
		if(ncvarget1(ap->cdfhandle, ap->varid, index, &deltas[i]) < 0)
		    DXErrorReturn(ERROR_INTERNAL, 
				"netCDF library error getting deltas");
	    }
	    
	    
	    indata = (Array)DXNewRegularArray(TYPE_FLOAT, ap->data_ndims,
					    ap->datacounts[dim], 
					    (Pointer)origins, (Pointer)deltas);
	    
	    if(!o)
		o = (Object)DXNewGroup();
	    o = (Object)DXSetMember((Group)o, NULL, (Object)indata);
	    if(!o)
		return NULL;
	    
	    start += delta;
	}
	
    }
    return o;
}


/*
 * completely regular Ndim connections component
 */
static Object build_regcon(Arrayinfo ap)
{
    return (Object)DXMakeGridConnectionsV(ap->data_ndims, ap->datacounts);
}


/*
 * completely regular 1D connections array
 */
static Object build_regcon1D(Arrayinfo ap, int dim)
{
    return (Object)DXNewPathArray(ap->datacounts[dim]);
}

/* given a variable, return it's name
 */
static Error variablename(int hand, int varid, char *cbuf)
{
    nc_type datatype;
    int natts, ndims;
    int dim[MAX_VAR_DIMS];
    
    /* inquire about the variable - it's size, shape and type */
    if(ncvarinq(hand,                      /* file descriptor */
		varid,                     /* variable ID */ 
		cbuf,                      /* variable name */
		&datatype,                 /* data item type */
		&ndims,                    /* number of dimensions */
		dim,                       /* array of dimension ID's */
		&natts) < 0)               /* number of attributes */
	DXErrorReturn(ERROR_INTERNAL, "netCDF library error");

    if (ndims <= 0)	/* no dimensions for this variable */
      *cbuf='\0';

    return OK;
}



/* return whether a char string matches: scalar, vector, matrix or tensor 
 */
static int setrank(char *s)
{
    char *cp;

    if(!s || !s[0])
	return 0;

    for(cp = s; *cp; cp++)
	if(isupper(*cp))
	   *cp = tolower(*cp);

    if(!strcmp("scalar", s))
	return 0;

    if(!strcmp("vector", s))
	return 1;

    if(!strcmp("matrix", s))
	return 2;

    if(!strcmp("tensor", s))
	return 3;

    return 0;
}


/* give it a string in the form: parm, parm, parm; parm, parm, parm; ... 
 *  it will parse up to N parms or ;, whichever occurs first.  it sets
 *  the num found, pointers to each, and returns a pointer to the next set.
 *  THIS IS DESTRUCTIVE TO THE ORIGINAL STRING (it replaces ,'s with NULLS)
 */
static char * parseit(char *in, int *n, char *out[])
{
    int instring = 0;
    int found = 0;
    int i;


    while(*in != '\0') {

	if(isspace(*in)) {
	    in++;
	    continue;
	}

	if(*in == ';') {
	    *in = '\0';
	    in++;
	    break;
	}

	if(*in == ',') {
	    *in = '\0';

	    if(!instring) {
		if(++found > *n) {
		    while(*in && *in != ';')
			in++;
		    if(*in)
			in++;
		    found--;
		    break;
		}
		
		*out = in;
		out++;
	    }
	    
	    instring = 0;
	    in++;
	    continue;
	}

	if(!instring) {
	    if(++found > *n) {
		while(*in && *in != ';')
		    in++;
		if(*in)
		    in++;
		found--;
		break;
	    }
	       
	    *out = in;
	    out++;
	    instring++;
	}

	in++;
    }
    
    for(i = found; i < *n; i++)
	*out++ = NULL;

    *n = found;
    
    return in;
}

#else

ImportStatReturn
_dxfstat_netcdf_file(char *filename)
{ 
    return IMPORT_STAT_NOT_FOUND;
}

Object DXImportNetCDF(char *filename, char **variable, int *starttime,
                    int *endtime, int *deltatime)
{ DXSetError(ERROR_NOT_IMPLEMENTED, "netCDF libs not included"); return ERROR; }
Object _dxfQueryImportInfo(char *filename)
{ DXSetError(ERROR_NOT_IMPLEMENTED, "netCDF libs not included"); return ERROR; }

#endif /* DXD_LACKS_NCDF_FORMAT */
