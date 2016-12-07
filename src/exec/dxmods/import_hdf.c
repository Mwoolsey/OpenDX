/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>

#if defined(HAVE_CTYPE_H)
#include <ctype.h>
#endif

#include <stdio.h>
#include <dx/dx.h>
#include "import.h"

#if (defined(WIN32) && defined(NDEBUG)) || defined(_MT)
#define _HDFDLL_
#elif defined(_DEBUG)
#undef _DEBUG
#endif

#if defined(HAVE_LIBDF)

#if defined(linux) && defined(i386)
#define UNIX386
#endif

#if defined(intelnt) || defined(WIN32)
#define F_OK	0
#define R_OK	4
#define INTEL86 
#endif

/* hdfi.h typdefs a set of number types whereas dx defines them
   with configure. In order to compile, it is necessary for us to
   undef them so the hdfi.h header can be included. 
 */
 
#ifdef int8
#undef int8
#endif
#ifdef uint8
#undef uint8
#endif
#ifdef int16
#undef int16
#endif
#ifdef uint16
#undef uint16
#endif
#ifdef int32
#undef int32
#endif
#ifdef uint32
#undef uint32
#endif
#ifdef float32
#undef float32
#endif
#ifdef float64
#undef float64
#endif

#if defined(HAVE_DFSD_H)
#include <dfsd.h>
#else
#include <hdf/dfsd.h>
#endif

#if !defined(macos)
#include <ctype.h>
#endif

/* special access: needs extra include file, and has different constants 
 *  to call stat() to find out if file exists and is readable.
 */
#if DXD_SPECIAL_ACCESS  
#include <sys/access.h>
#else
#include <string.h>
#endif

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#ifndef DXD_NON_UNIX_ENV_SEPARATOR
#define PATH_SEP_CHAR ':'
#else
#define PATH_SEP_CHAR ';'
#endif

#define ABS(x) (x<0 ? -x : x)
#define MAXRANK 8
#define MAXLEN 255

static Error findfile(char *filename, char *pathname);
static Error dxtype(int hdftype,Type *type);
static Array posarray(int rank, int *dimsize, Type);
static Error read_scale(Type type,int dim,int *dimsize,float *pos);
#ifdef PVS
static Error pvsswap(void *data, int numelts, Type type);
#endif

static char *etype(int num, char *str)
{
    switch(num) {
      case 0:
	strcpy(str, "points");
	break;
      case 1:
	strcpy(str, "lines");
	break;
      case 2:
	strcpy(str, "quads");
	break;
      case 3:
	strcpy(str, "cubes");
	break;
      default:
	sprintf(str, "cubes%dD", num);
	break;
    }
    return str;
}



Field
DXImportHDF(char *filename, char *fieldname)
{
    int i;
    int skip = 0;
    int rank, numelts, dimsize[MAXRANK];
    char et[32];
    char pathname[MAXLEN];
    Field f = NULL;
    Array a = NULL, ag = NULL, at = NULL;
    char label[MAXLEN]="", unit[MAXLEN], format[MAXLEN], coordsys[MAXLEN];
    int hdftype;
    Type type;
    int status;
    void *datav;
    /* float deltas[MAXRANK*MAXRANK], origins[MAXRANK]; int j; */
#ifdef PVS
    int badfp=0, denorm=0;
#endif

    if (!findfile(filename, pathname))
	return NULL;

    /* What type of hdf file is it? (SDS,VSET) */

    /* SCIENTIFIC DATA SETS */

    /* is this a good idea? */
    if (fieldname && isdigit(fieldname[0]))
	skip = atoi(fieldname);
    
    DFSDrestart();
    
    /* get dimensions of data */
    while(DFSDgetdims(pathname, &rank, dimsize, MAXRANK) >= 0) {
	
	if(skip > 0) {
	    skip--;
	    continue;
	}
	f = DXNewField();
	if(!f)
	    goto error;

        /* get data type */
        DFSDgetNT(&hdftype);
        if (!dxtype(hdftype,&type))
           goto error;
 
	a = DXNewArray(type, CATEGORY_REAL, 0);
	if(!a)
	    goto error;
	
	/* decide total size */
	for(i=0, numelts=1; i<rank; numelts *= dimsize[i], i++)
	    ;
	
	/* and allocate space for it */
	if(!DXAddArrayData(a, 0, numelts, NULL))
	    goto error;

        /* read data from HDF file and put in the Array */
	datav = (void *)DXGetArrayData(a);
	if (!datav)
	   goto error;
    	status = DFSDgetdata(pathname,rank,dimsize,datav);
	if (status!=0)
	   goto error;

	/* change byte order if PVS and denormalize if float or double */
#ifdef PVS

	_dxfByteSwap(datav,datav,numelts,type);
	if (type == TYPE_FLOAT)
	    _dxffloat_denorm(datav,numelts,&denorm,&badfp);
	else if (type == TYPE_DOUBLE)
	    _dxfdouble_denorm(datav,numelts,&denorm,&badfp);

	if (denorm && badfp)
	    DXWarning("Denormalized and NaN/Infinite floating point numbers");
  	else if (denorm)
	    DXWarning("Denormalized floating point numbers");
	else if (badfp)
	    DXWarning("NaN/Infinite floating point numbers");
#endif

	/* data */
	if (!DXSetStringAttribute((Object)a, "dep", "positions"))
	    goto error;
	if(!DXSetComponentValue(f, "data", (Object)a))
	    goto error;
	a = NULL;
	
	/* connections */
	at = DXMakeGridConnectionsV(rank, dimsize);
	if(!at)
	    goto error;
	
	if (!DXSetStringAttribute((Object)at, "element type", etype(rank, et)))
	    goto error;
	if (!DXSetStringAttribute((Object)at, "ref", "positions"))
	    goto error;
	if (!DXSetComponentValue(f, "connections", (Object)at))
	    goto error;
	at = NULL;

	/* positions */
        ag = posarray(rank,dimsize,type);
        if (!ag)
          goto error;
        /*
	for (i=0; i<rank; i++) {
	    origins[i] = 0.0;
	    for (j=0; j<rank; j++) 
		deltas[i * rank + j] = (i==j) ? 1.0 : 0.0;
	}
	ag = DXMakeGridPositionsV(rank, dimsize, origins, deltas);
	if(!ag)
	    goto error;
        */

	if(!DXSetComponentValue(f, "positions", (Object)ag))
	    goto error;
	ag = NULL;

        /* read label if exists and set an attribute */
        if (DFSDgetdatastrs(label,unit,format,coordsys) >= 0){
          if (!DXSetStringAttribute((Object)f,"name",label)) 
             goto error;
        }
 
	/* 'rewind' HDF file */
	DFSDrestart();

#ifdef DO_ENDFIELD
	f = DXEndField(f);
	if(!f)
	    goto error;
#endif
	    
	/* post-import, preprocessing section */

	/* filtering */
	/* data reduction pyramids */
	/* partition */
	
	return (f);
	
    } /* per hdf set */

    DXSetError(ERROR_DATA_INVALID, 
	       "can't skip %d datasets in this file", atoi(fieldname));
    /* fall thru */

  error:
    DXDelete((Object)f);
    DXDelete((Object)a);
    DXDelete((Object)ag);
    DXDelete((Object)at);
    DFSDrestart();
    return NULL;
}



static Error 
findfile(char *filename, char *pathname)
{
    int foundfile = 0;
    char *foundname = NULL;
    char *dir = NULL;
    char *label;
    char *cp;

    /* assume the worst */
    pathname[0] = '\0';

    if (!filename) {
	DXSetError(ERROR_BAD_PARAMETER, "#10200", filename);
	return ERROR;
    }

    if (strlen(filename) >= MAXLEN) {
	DXSetError(ERROR_BAD_PARAMETER, 
	   "filename `%s' too long; HDF filename limit is 255 characters", 
		   filename);
	return ERROR;
    }
    
#define XTRA 8 /* room for trailing /, .hdf, NULL, plus some slop */
    
    dir = (char *)getenv("DXDATA");
    foundname = (char *)DXAllocateLocalZero((dir ? strlen(dir) : 0) +
					    strlen(filename) + XTRA);
    if (!foundname)
	return ERROR;

    strcpy(foundname, filename);

    /* test if HDF file */
    if(Hishdf(foundname) != 0)
	goto done;

    label = "file '%s' not found";
#ifndef DXD_NON_UNIX_DIR_SEPARATOR
    if (foundname[0] == '/')
       goto error;
#else
    if (foundname[0] == '/' || foundname[1] == ':' || foundname[0] == '\\')
       goto error;
#endif

    
#if DXD_SPECIAL_ACCESS
    if (access(foundname, E_ACC) >= 0 && !foundfile) {
	foundfile++;
	if (access(foundname, R_ACC) < 0)
	    label = "filename '%s' not readable";
	else
	    label = "filename '%s' not HDF dataset";
    }
#else
    if (access(foundname, F_OK) >= 0 && !foundfile) {
	foundfile++;
	if (access(foundname, R_OK) < 0)
	    label = "filename '%s' not readable";
	else
	    label = "filename '%s' not HDF dataset";
    }
#endif
	

    /* if you couldn't open the filename and there isn't a DXDATA
     *  search path, you are out of options.
     */
    if (!dir && !foundfile)
	goto error;
    

    /* go through each directory in the path:path:path list and
     *  try to find the file.
     */
    while (dir) {

	strcpy(foundname, dir);
	if ((cp = strchr(foundname, PATH_SEP_CHAR)) != NULL)
	    *cp = '\0';
	strcat(foundname, "/");
	strcat(foundname, filename);
	
	/* test if HDF file */
	if (Hishdf(foundname) != 0)
	    goto done;
	
#if DXD_SPECIAL_ACCESS
	if (access(foundname, E_ACC) >= 0 && !foundfile) {
	    foundfile++;
	    if (access(foundname, R_ACC) < 0)
		label = "filename '%s' not readable";
	    else
		label = "filename '%s' not HDF dataset";
	}
#else
	if (access(foundname, F_OK) >= 0 && !foundfile) {
	    foundfile++;
	    if (access(foundname, R_OK) < 0)
		label = "filename '%s' not readable";
	    else
		label = "filename '%s' not HDF dataset";
	}
#endif
	
	dir = strchr(dir, PATH_SEP_CHAR);
	if (dir)
	    dir++;
	
    }

  error:
    DXSetError(ERROR_BAD_PARAMETER, label, filename);
    DXFree((Pointer)foundname);
    return ERROR;

  done:
    if (strlen(foundname) > MAXLEN) {
	DXSetError(ERROR_BAD_PARAMETER, 
	    "filename `%s' too long; HDF filename limit is %d characters", 
		   foundname, MAXLEN);
	DXFree((Pointer)foundname);
	return ERROR;
    }
    
    strcpy(pathname, foundname);
    DXFree((Pointer)foundname);
    return OK;
}


ImportStatReturn
_dxfstat_hdf(char *filename)
{
    char pathname[MAXLEN];
    
    if (findfile(filename, pathname) == OK)
	return IMPORT_STAT_FOUND;
    
    DXResetError();
    return IMPORT_STAT_NOT_FOUND;
}

int _dxfget_hdfcount(char *filename)
{
    int i,rank;
    int dimsize[MAXRANK];
    char pathname[MAXLEN];

    if (!findfile(filename, pathname))
	return 0;
    DFSDrestart();

    /* get dimensions of data */
    for (i=0; DFSDgetdims(pathname, &rank, dimsize, MAXRANK) >= 0; i++)
       ;
    return i++;
} 

int _dxfwhich_hdf(char *filename, char *fieldname)
{
    int i,rank;
    int dimsize[MAXRANK];
    char pathname[MAXLEN];
    char label[MAXLEN]="", unit[MAXLEN], format[MAXLEN], coordsys[MAXLEN];

    if (!findfile(filename,pathname))
       return (-1);
    DFSDrestart();

    /* look for filename */
    for (i=0; DFSDgetdims(pathname, &rank, dimsize, MAXRANK) >= 0; i++){
        if (DFSDgetdatastrs(label,unit,format,coordsys) >= 0){
	   if (!strcmp(fieldname,label))
	      return i;
	}
    } 
    DXSetError(ERROR_BAD_PARAMETER,"can't find fieldname %s",fieldname);
    return (-1);
}

static
Error dxtype(int hdftype,Type *type)
{
   switch(hdftype){
   case(5):
      *type = TYPE_FLOAT;
      break;
   case(6):
      *type = TYPE_DOUBLE;
      break;
   case(4):
   case(20):
      *type = TYPE_BYTE;
      break;
   case(3):
   case(21):
      *type = TYPE_UBYTE;
      break;
   case(22):
      *type = TYPE_SHORT;
      break;
   case(23):
      *type = TYPE_USHORT;
      break;
   case(24):
      *type = TYPE_INT;
      break;
   case(25):
      *type = TYPE_UINT;
      break;
   default:
      DXSetError(ERROR_DATA_INVALID,"#10320","hdftype");
      return ERROR;
   }
   return OK;
} 

static
Array posarray(int rank, int *dimsize,Type type)
{
   Array a[MAXRANK];
   float *scale,d,r,v,*pos;
   float origins[MAXRANK],deltas[MAXRANK];
   int i,j,dim,notreg,ret;
  
 
   /* are there positions? (DFSDgetdimscale) */
   for (dim=0; dim<rank; dim++){
      scale = (float *)DXAllocateLocal(sizeof(float)*dimsize[dim]);
      ret=read_scale(type, dim, dimsize, scale);
      if (!ret)
         goto error;
      else if (ret==-1){
         /* if no scale info make a regular array */
         DXFree(scale);
         for (i=0; i<rank; i++) {
            origins[i] = 0.0;
            deltas[i] = (i==dim) ? 1.0 : 0.0;
         }
         a[dim] = (Array)DXNewRegularArray(TYPE_FLOAT, rank, dimsize[dim],
                   origins, deltas);
         if (!a[dim])
            goto error;
      }
      else{
         /* is scale an evenly spaced array */
         d = scale[1]-scale[0];
         notreg=0;
         for (i=1; i<dimsize[dim]-1; i++){
	   r = scale[i+1]-scale[i];
	   v = ABS((d-r)/d);
           if (d!=r && v >.00001){
              notreg=1;
              break;
           }
         }
         if (notreg==0){
            for (i=0; i<rank; i++) {
               origins[i] = (i==dim) ? scale[0] : 0.0;
               deltas[i] = (i==dim) ? d : 0.0;
            }
            DXFree(scale);
            a[dim] = (Array)DXNewRegularArray(TYPE_FLOAT, rank, 
                     dimsize[dim], origins, deltas);
            if (!a[dim])
               goto error;
         }
         else{
            a[dim] = DXNewArray(TYPE_FLOAT,CATEGORY_REAL,1,rank);
            if (!a[dim])
               goto error;
            a[dim] = DXAddArrayData(a[dim],0,dimsize[dim],NULL);
            if (!a[dim])
               goto error;
            pos = (float *)DXGetArrayData(a[dim]);
            for (i=0; i<dimsize[dim]; i++){
               for (j=0; j<rank; j++)
                  pos[i*rank +j] = (j==dim) ? scale[i] : 0.0;
            }
            DXFree(scale);
         }
      }
   }

   /* make product array */
   return ((Array)DXNewProductArrayV(rank,a));

error:
   return NULL; 
}

static
int read_scale(Type type,int dim,int *dimsize,float *pos)
{
   float *data_f;
   int *data_i;
   double *data_d;
   short *data_s;
   byte *data_b;
   uint *data_ui;
   ushort *data_us;
   ubyte *data_ub;
   int i;

   switch(type){
   case(TYPE_FLOAT):
      data_f = (float *)DXAllocateLocal(sizeof(float)*dimsize[dim]);
      if ((DFSDgetdimscale(dim+1,dimsize[dim],(Pointer)data_f))!=0)
         return -1;
      else{
#ifdef PVS
	pvsswap((void *)data_f,dimsize[dim],type);
#endif
         /* convert to float */
         for (i=0; i<dimsize[dim]; i++)
            pos[i] = (float)data_f[i];
      }
      DXFree(data_f);
      break;
   case(TYPE_BYTE):
      data_b = (byte *)DXAllocateLocal(sizeof(byte)*dimsize[dim]);
      if ((DFSDgetdimscale(dim+1,dimsize[dim],data_b))!=0)
         return -1;
      else{
#ifdef PVS
	pvsswap((void *)data_b,dimsize[dim],type);
#endif
         /* convert to float */
         for (i=0; i<dimsize[dim]; i++)
            pos[i] = (float)data_b[i];
      }
      DXFree(data_b);
      break;
   case(TYPE_INT):
      data_i = (int *)DXAllocateLocal(sizeof(int)*dimsize[dim]);
      if ((DFSDgetdimscale(dim+1,dimsize[dim],data_i))!=0)
         return -1;
      else{
#ifdef PVS
	pvsswap((void *)data_i,dimsize[dim],type);
#endif
         /* convert to float */
         for (i=0; i<dimsize[dim]; i++)
            pos[i] = (float)data_i[i];
      }
      DXFree(data_i);
      break;
   case(TYPE_DOUBLE):
      data_d = (double *)DXAllocateLocal(sizeof(double)*dimsize[dim]);
      if ((DFSDgetdimscale(dim+1,dimsize[dim],data_d))!=0)
         return -1;
      else{
#ifdef PVS
	pvsswap((void *)data_d,dimsize[dim],type);
#endif
         /* convert to float */
         for (i=0; i<dimsize[dim]; i++)
            pos[i] = (float)data_d[i];
      }
      DXFree(data_d);
      break;
   case(TYPE_SHORT):
      data_s = (short *)DXAllocateLocal(sizeof(short)*dimsize[dim]);
      if ((DFSDgetdimscale(dim+1,dimsize[dim],data_s))!=0)
         return -1;
      else{
#ifdef PVS
	pvsswap((void *)data_s,dimsize[dim],type);
#endif
         /* convert to float */
         for (i=0; i<dimsize[dim]; i++)
            pos[i] = (float)data_s[i];
      }
      DXFree(data_s);
      break;
   case(TYPE_UBYTE):
      data_ub = (ubyte *)DXAllocateLocal(sizeof(ubyte)*dimsize[dim]);
      if ((DFSDgetdimscale(dim+1,dimsize[dim],data_ub))!=0)
         return -1;
      else{
#ifdef PVS
	pvsswap((void *)data_ub,dimsize[dim],type);
#endif
         /* convert to float */
         for (i=0; i<dimsize[dim]; i++)
            pos[i] = (float)data_ub[i];
      }
      DXFree(data_ub);
      break;
   case(TYPE_UINT):
      data_ui = (uint *)DXAllocateLocal(sizeof(uint)*dimsize[dim]);
      if ((DFSDgetdimscale(dim+1,dimsize[dim],data_ui))!=0)
         return -1;
      else{
#ifdef PVS
	pvsswap((void *)data_ui,dimsize[dim],type);
#endif
         /* convert to float */
         for (i=0; i<dimsize[dim]; i++)
            pos[i] = (float)data_ui[i];
      }
      DXFree(data_ui);
      break;
   case(TYPE_USHORT):
      data_us = (ushort *)DXAllocateLocal(sizeof(ushort)*dimsize[dim]);
      if ((DFSDgetdimscale(dim+1,dimsize[dim],data_us))!=0)
         return -1;
      else{
#ifdef PVS
	pvsswap((void *)data_us,dimsize[dim],type);
#endif
         /* convert to float */
         for (i=0; i<dimsize[dim]; i++)
            pos[i] = (float)data_us[i];
      }
      DXFree(data_us);
      break;
   default:
      DXSetError(ERROR_DATA_INVALID,"#10320","scales");
      return 0;
   }
   return 1;
}

	/* change byte order if PVS and denormalize if float or double */
#ifdef PVS
static Error pvsswap(void *data, int numelts, Type type)
{
	int badfp=0,denorm=0;

	_dxfByteSwap((void *)data,(void *)data,numelts,type);
	if (type == TYPE_FLOAT)
	    _dxffloat_denorm(data,numelts,&denorm,&badfp);
	else if (type == TYPE_DOUBLE)
	    _dxfdouble_denorm(data,numelts,&denorm,&badfp);

	if (denorm && badfp)
	    DXWarning("Denormalized and NaN/Infinite floating point numbers");
  	else if (denorm)
	    DXWarning("Denormalized floating point numbers");
	else if (badfp)
	    DXWarning("NaN/Infinite floating point numbers");
	return OK;
}
#endif

#else

ImportStatReturn
_dxfstat_hdf(char *filename)
{
    return IMPORT_STAT_NOT_FOUND;
}

int _dxfget_hdfcount(char *filename)
{ DXSetError(ERROR_NOT_IMPLEMENTED, "HDF libs not included"); return ERROR; }
int _dxfwhich_hdf(char *filename, char *fieldname)
{ DXSetError(ERROR_NOT_IMPLEMENTED, "HDF libs not included"); return ERROR; }
Field DXImportHDF(char *filename, char *fieldname) 
{ DXSetError(ERROR_NOT_IMPLEMENTED, "HDF libs not included"); return ERROR; }

#endif /* DXD_LACKS_HDF_FORMAT */
