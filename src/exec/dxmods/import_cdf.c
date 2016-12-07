/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


/****************************************************************************

  CDF importer: Each CDF variable is categorized as a data, positions or
  connections component, or series positions coponent. 
  It takes the freedom to accept the first N categorized positions
  variables (N is the # dims) and ignore rest of the categorized positions 
  variable by giving a warning to the user each time it ignores a variable.
  It does the same to series position variables.
  (originally impCDF.c)
*****************************************************************************/ 

#include <stdio.h>
#include <string.h>
#include <dx/dx.h>
#include "import.h"

#if defined(HAVE_LIBCDF)

#include <cdf.h>
#include "impCDF.h"

static Error cdfGetData(Infovar vp, long rec, void *data);
static Error openCDF(char *filename, Infocdf *cdfp, int fill);
static Error conCol2Row(void *des, void *src, Infovar vpdata, Infocdf cdfp);
static Error cdfrotate(Infocdf cdfp, Infovar vpdata, void *data);
static Error queryCDFvars(Infocdf cdfp, Infovar *vpdata, Infovar *vploc,
		   Infovar *vptime, char **varname, 
		   int startframe, int endframe, int deltaframe);
static void freeVar(Infovar vp);
static int cntVars(Infovar vp);
static int categorizeVar(long dims, Infovar vp);
static void addVar(Infovar vp, Infovar *vps);
static Error completeVarsInfo(Infocdf cdfp, Infovar vpdata, 
			      Infovar vploc, Infovar vptime);
static Error cdftype2dxtype( long dataType, Type *type );
static int dxtype2bytes( Type dxtype );
static Error cleanDataVar(Infovar vpdata, Infocdf cdfp);
static Error cleanPosVar(Infovar *vploc, Infocdf cdfp, Infovar *vpdata);
static Error cleanSerVar(Infovar *vptime, Infocdf cdfp, Infovar *vpdata);

static Object impcdfgroup(char *filename, char **varname, int startframe, 
			  int endframe, int deltaframe);
static Object impcdfobject(Infocdf cdfp, Infovar vpdata, double *timeval,
			   Array pos, Array con, int *posdel, int *condel);
static Field impcdffield(int rec, Infocdf cdfp, Infovar vpdata, 
			 Array pos, Array con, int *posdel, int *condel);
static Object impcdf0dim(Infocdf cdfp, Infovar vpdata, Array pos, int *posdel);
static Error loadSerPos(Infocdf cdfp, Infovar vptime,  double *timeval);

static void moveVar(Infovar *vp, Infovar *vp2, int n);
static int getHowMany(char **names);
static Error GetAttribute(Infovar vp, Object f,int var);
static void removeVar(Infovar *vp, int n);
static Error namedVar(Infovar *vpdata,Infovar *vploc,Infovar *vptime,
			char **varname,int numDims);
static void copyVar(Infovar vp, Infovar *vps, int numDims);

Array impcdfpos(Infovar vploc, Infocdf cdfp);

#ifndef DXD_NON_UNIX_ENV_SEPARATOR
#define PATH_SEP_CHAR ':'
#else
#define PATH_SEP_CHAR ';'
#endif

/*
  Top level routine for CDF importer.
  It checks for valid start, end and delta frame values.
  Returns an object if successful in importing or else returns a NULL.
  */

Object DXImportCDF(char* filename, char **fieldlist, int *startframe,
		int *endframe, int *deltaframe)
{
  Object obj = NULL;
  int fframe, lframe, iframe;
  
  if (!startframe)
    fframe = 0;
  else
    {
      fframe = *startframe;
      if (fframe < 0 )
	DXErrorGoto(ERROR_BAD_PARAMETER,
		  "First frame cannot be less than zero");
    }
  
  if (!endframe)
    lframe = -1; 
  else
    {
      lframe = *endframe;
      if (lframe < 0 )
	DXErrorGoto(ERROR_BAD_PARAMETER,
		  "Last frame cannot be less than zero");
      if (lframe < fframe)
	DXErrorGoto(ERROR_BAD_PARAMETER,
		  "Last frame cannot be less than first frame");
    }

  if (!deltaframe)
    iframe = 1;
  else
    {
      if (*deltaframe <= 0)
	{
	  DXSetError(ERROR_BAD_PARAMETER,
		   "Delta value has to be greater than 0");
	  goto error;
	}
      else
	iframe = *deltaframe;
    }

  if(!(obj = impcdfgroup(filename,
			 fieldlist,
			 fframe,
			 lframe,
			 iframe)))
    goto error;
  
  return obj;
  
 error:
  if (obj)
    DXDelete (obj);

  if ( DXGetError() == ERROR_NONE )
    DXSetError(ERROR_INTERNAL, "Error in cdf importer");
  
  return NULL;
}

/*
  This routine is responsible for CDF to dx object conversion.
  According to the number of variables from the CDF data file 
  to be imported returns a (Object)group or an object returned 
  by imcdfobject call.
 */

static Object 
impcdfgroup(char *filename, char **varname, int startframe, 
			  int endframe, int deltaframe)
     
{
  Infocdf cdfp = NULL;
  Infovar vpdata = NULL, vploc = NULL, vptime = NULL, ptr;
  Group  g = NULL;
  Object obj = NULL;
  Array con = NULL, pos = NULL;
  double *timeval;
  int i,posdel=1,condel=1;
  int sizes[CDF_MAX_DIMS];

  
  /* 
    open CDF file and set up the file infomation in a struct infocdf 
    */
  
  timeval = NULL;

  if( openCDF(filename, &cdfp,1) == ERROR )
    goto error;
  
  if( cdfp->maxRec < endframe )
    {
      DXSetError(ERROR_BAD_PARAMETER, "endframe number exceed maximum frame");
      goto error;
    }
  if( cdfp->maxRec < 0 )
    {
      DXSetError(ERROR_BAD_PARAMETER,"maxRec is -1");
      goto error;
    }

  /*
    set required var(s) information in vpdata and location vars in vploc 
    */

  if(queryCDFvars(cdfp, &vpdata, &vploc, &vptime, varname,
		  startframe, endframe, deltaframe) == ERROR)
    goto error;
 
  if (cdfp->maxRec > 0){
    if((timeval=(double *)DXAllocate((cdfp->maxRec+1)*sizeof(double)))==NULL)
      goto error;
    if (loadSerPos(cdfp, vptime, timeval) == ERROR)
      goto error;
  }

  /*
    If number of members in the vpdata link list is more than then
    create a new group.
    */
  
  if (cntVars(vpdata) > 1)
    {
      g = DXNewGroup();
      if(!g)
	goto error;
    }

  /*
    Positions and connections always formed only when number of dimensions
    is greater than 0.  When dimension=0 positions formed
    only when vploc exists (when user supplies positions).
    */

  if (cdfp->numDims >= 1)
    {
      if (!(pos = impcdfpos(vploc, cdfp)))
	goto error;
      for (i=0; i<cdfp->numDims; i++)
	sizes[i] = cdfp->dimSizes[i];
      if (!(con = DXMakeGridConnectionsV((int)(cdfp->numDims),sizes))) 
	goto error;
    }
  else if (cdfp->numDims == 0)
    {
      if (vploc)
        if (!(pos = impcdfpos(vploc, cdfp)))
	  goto error;
    }

  /*
    For each given field build an object and make it an element of
    a group. Which means each object is a field.
    */
  
  if (cdfp->numDims >= 0)
    {
      ptr = vpdata;
      while( ptr )
  	{
 	 obj = impcdfobject(cdfp, ptr, timeval, pos, con, &posdel, &condel);
         if( !obj )
	      goto error;
  	 if (g)
	     {
	      if(!(DXSetMember(g, ptr->name, obj)))
			   goto error;
	      obj = NULL;
	     }
	 ptr = ptr->next;
        }
    }

  /*
    DO NOT USE ANYMORE
    To read data points when number of dimensions is equal to 0 call
    impcdf0dim. It interprets all time series elements to be in the
    same plane. It is done because of the way one has to represent
    irregular grid in cdf data.
    */
    
  else if (cdfp->numDims == -1)
    {
      if (vploc)
	if (!(pos = impcdfpos(vploc, cdfp)))
	  goto error;
      ptr = vpdata;
      while( ptr )
	{
	 obj = impcdf0dim(cdfp, ptr, pos, &posdel);
	 if( !obj )
	    goto error;
	 if (g)
	  {
	    if(!(DXSetMember(g, ptr->name, obj)))
	 	 goto error;
	    obj = NULL;
	  }
	 ptr = ptr->next;
	}
    }
  pos = NULL;
  con = NULL;

  /* Pull Global Attributes from CDF file and convert to DX attribute */
  if (g){
     if (!GetAttribute(vpdata,(Object)g,0))
        goto error;
  }
  else{
     if (!GetAttribute(vpdata,(Object)obj,0))
        goto error;
  }

  /*
    Free up all the unwanted memory once conversion is performed.
    */

  if (cdfp)
    DXFree((Pointer)cdfp);
  if (vpdata)
    freeVar(vpdata);
  if (vploc)
    freeVar(vploc);
  if (vptime)
    freeVar(vptime);
  if (timeval)
    DXFree((Pointer)timeval);

  if (g)
    return (Object)g;
  return obj;
  
 error:
  if (cdfp)
    DXFree((Pointer)cdfp);
  if (vpdata)
    freeVar(vpdata);
  if (vploc)
    freeVar(vploc);
  if (vptime)
    freeVar(vptime);
  if (timeval)
    DXFree((Pointer)timeval);
  if (g)
    DXDelete ((Object)g);
  if (obj)
    DXDelete (obj);
  if (pos && posdel)
    DXDelete ((Object)pos);
  if (con && condel)
    DXDelete ((Object)con);
  return NULL;
}

/*
  This routine loads the array timeval with the series positions 
  values.
  Except for number of dimensions equal to 0 it represents series
  positions value.
  */

static Error loadSerPos(Infocdf cdfp, Infovar vptime, double *timeval)
{
  int i;
  char *d, *dp;
  char msg[CDF_ERRTEXT_LEN+1];
  CDFstatus status;

  d = NULL;
  
  /*
    When vptime is NULL just give the series positions value for
    each record as a record number itself.
    */
  
  if (vptime == NULL && cdfp->maxRec > 0)
    {
      for (i=0; i<=cdfp->maxRec; i++)
			timeval[i] = (double)i;
    }

  /*
    When vptime is not NULL then read in the variable and assign it
    to timeval array.
    */
  
  else if (vptime != NULL && cdfp->maxRec > 0)
    {
      
      /*
	Allocate array d as vptime->numbytes may not be equal to
	sizeof(double).
	*/
      
      if((d = (char *)DXAllocate((cdfp->maxRec+1)* vptime->numbytes))==NULL)
	goto error;
      
      dp = d;
      
      /*
	CDF call to read the time series variable.
	*/

      status = CDFvarHyperGet(vptime->id, vptime->varNum, 0, cdfp->maxRec+1,
			      1, vptime->idx, vptime->cnt, vptime->intval,
			      (void *)d);
      if (status < CDF_OK)
	{
	  CDFerror(status,msg);
	  DXSetError(ERROR_MISSING_DATA,msg);
	  goto error;
	}
  
      /*
	Convert different kind of data type to double and load it to
	timeval array.
	*/
      
      switch(vptime->dxtype)
	{
	case TYPE_BYTE:
	  for (i=0; i<=cdfp->maxRec; i++)
	    {
	      timeval[i] = (double)*((byte *)(dp));
	      ++dp;
	    }
	  break;
 	case TYPE_UBYTE:
	  for (i=0; i<=cdfp->maxRec; i++)
	    {
	      timeval[i] = (double)*((ubyte *)(dp));
	      ++dp;
	    }
          break;
	case TYPE_SHORT:
	  for (i=0; i<=cdfp->maxRec; i++)
	    {
	      timeval[i] = (double)*((short *)(dp));
	      dp+=2;
	    }
	  break;
	case TYPE_USHORT:
	  for (i=0; i<=cdfp->maxRec; i++)
	    {
	      timeval[i] = (double)*((ushort *)(dp));
	      dp+=2;
	    }
	  break;
	case TYPE_INT:
	  for (i=0; i<=cdfp->maxRec; i++)
	    {
	      timeval[i] = (double)*((int *)(dp));
	      dp+=4;
	    }
	  break;
	case TYPE_UINT:
	  for (i=0; i<=cdfp->maxRec; i++)
	    {
	      timeval[i] = (double)*((uint *)(dp));
	      dp+=4;
	    }
	  break;
	case TYPE_FLOAT:
	  for (i=0; i<=cdfp->maxRec; i++)
	    {
	      timeval[i] = (double)*((float *)(dp));
	      dp+=4;
	    }
	  break;
	case TYPE_DOUBLE:
	  for (i=0; i<=cdfp->maxRec; i++)
	    {
	      timeval[i] = *((double *)(dp));
	      dp+=8;
	    }
	  break;
	default:
	  DXErrorGoto(ERROR_BAD_TYPE,"Time series values of bad type");
	  break;
	}
      DXFree((Pointer)d);
    }
  return OK;
  
 error:
  if (d)
    DXFree((Pointer)d);
  return ERROR;
}

/*
  Builds a field or a series according to the users request.
  */

static Object impcdfobject(Infocdf cdfp, Infovar vpdata, double *timeval, 
			   Array pos, Array con, int *posdel,int *condel)
{
  int i, fn;
  Field f=NULL;
  Series s=NULL;
  int maxframe;
  Array a;
  
  /* 
    endframe == -1  means import every time series 
    */
  
  maxframe = (vpdata->endframe == -1) ? cdfp->maxRec : vpdata->endframe ;
  
  /* if maxRec > 0 the cdf file is a series 
   * create a series even if only one time step so the time stamp will exist
   */
  if (cdfp->maxRec > 0)
    { 
      /* 
	more than 1 series 
	*/

      s = DXNewSeries();
      if( !s ) 
	goto error;
    }
  
  for(i=0, fn=vpdata->strframe; fn<=maxframe; fn+=vpdata->dltframe, i++)
    {
      f = impcdffield(fn, cdfp, vpdata, pos, con, posdel,condel);
      if(!f) 
	goto error;

      if( s ) 
	{
	   a = DXNewArray(TYPE_DOUBLE,CATEGORY_REAL,0);
	   if (!a)
		goto error;
	   if (!DXAddArrayData(a,0,1,&timeval[fn]))
		goto error;
	  if (!DXSetAttribute((Object)f,"EPOCH",(Object)a)) 
	     goto error;
	  if(!(DXSetSeriesMember(s, i, (double)fn, (Object)f)))
	    goto error;
	  f = NULL;
	}
    }
  
  if( s ) 
    return (Object)s;
  return (Object)f;
  
 error:
  if (s)
    DXDelete ((Object)s);
  if (f)
    DXDelete ((Object)f);
  return NULL;
}

/*
  This routine is called whenever the cdf number of dimension is defined
  as zero. It reads in each field as an array and if variables 
  LATITUDE and LONGITUDE are present interprets them as positions.
  */

static Object impcdf0dim(Infocdf cdfp, Infovar vpdata, Array pos, int *posdel)
{
  Field f=NULL;
  Array a=NULL;
  void *data;
  Object obj=NULL;
  long recStart, recCount, recInterval;
  CDFstatus status;
  char msg[CDF_ERRTEXT_LEN+1];

  data = NULL;

  recStart = vpdata->strframe;
  recInterval = vpdata->dltframe;
  recCount = vpdata->size;

  if (!(f = DXNewField())) 
    goto error;

  a = DXNewArray(vpdata->dxtype, CATEGORY_REAL, 0);

  if(!a) 
    goto error;

  if ( !(DXAddArrayData(a, 0, vpdata->size, NULL))) 
    goto error;
  
  data = (void *)DXGetArrayData( a );

  if(!data) 
    goto error;

  status = CDFvarHyperGet(vpdata->id, vpdata->varNum, recStart,
			  recCount, recInterval, vpdata->idx,
			  vpdata->cnt, vpdata->intval, (void *)data);
  if (status < CDF_OK)
    {
      CDFerror(status, msg);
      DXSetError(ERROR_MISSING_DATA, msg);
      goto error;
    }
  if(!(DXSetComponentValue(f, "data", (Object)a)))
    goto error;
  a = NULL;

  if (pos)
    {
      if(!(DXSetComponentValue(f, "positions", (Object)pos)))
	goto error;
      obj = NULL;
      *posdel=0;
      if(!(obj=(Object)DXNewString("positions")))
	goto error;
      if(!(DXSetComponentAttribute(f, "data", "dep", obj)))
	goto error;
      obj = NULL;
    }
  
  if (!GetAttribute(vpdata,(Object)f,1))
     goto error; 
  if( !(DXEndField(f)) )
    goto error;
  
  return (Object)f;
  
 error:
  if (f)
    DXDelete ((Object)f);
  if (a)
    DXDelete ((Object)a);
  if (obj)
    DXDelete (obj);
  return NULL;
  
}

/*
  Returns a field if successful.
  After conversion, checks if the data points need to be rotated. This
  is to make sure that the position values of each axis is an ascending
  to descending order. Calls the routine which rotates data points.
  If data is column majority then calls the appropriate routine and 
  converts data from column majority to row majority.

  If includes positions or connections if present and sets data dep positions.
  If neither positions nor connection then just data with no connections
  and positions.
  Pulls attributes on the data varibles from CDF and makes them DX attributes.
 */
static Field impcdffield(int rec, Infocdf cdfp, Infovar vpdata, 
			 Array pos, Array con, int *posdel, int *condel)
{
  Field f=NULL;
  Array a=NULL,posterms[CDF_MAX_DIMS],newpos[CDF_MAX_DIMS];
  Array conterms[CDF_MAX_DIMS],newcon[CDF_MAX_DIMS];
  void *data, *datacm;
  Object obj=NULL;
  int n,i,j=0;

  data = datacm = NULL;

  if (!(f = DXNewField())) 
    goto error;

  if (vpdata->dxtype == TYPE_STRING)
    a = DXNewArray(vpdata->dxtype,CATEGORY_REAL, 1, vpdata->numElements+1);
  else
    a = DXNewArray(vpdata->dxtype, CATEGORY_REAL, 0);
  if(!a) 
    goto error;

  if ( !(DXAddArrayData(a, 0, vpdata->size, NULL))) 
    goto error;
  
  data = (void *)DXGetArrayData( a );
  if(!data) 
    goto error;
  
  /* 
    read the values from CDF file to Array 
    */

  if (cdfp->majority == COL_MAJOR && cdfp->numDims > 1)
    {
      if((datacm = (void *)DXAllocate(vpdata->size * vpdata->numbytes))==NULL)
	goto error;
      if(cdfGetData(vpdata,(long)rec, datacm)==ERROR)
	goto error;
      if (conCol2Row(data, datacm, vpdata, cdfp)==ERROR)
	goto error;
      DXFree((Pointer)datacm);
      datacm = NULL;
    }
  else
    {
      if(cdfGetData(vpdata, (long)rec, data)==ERROR)
	goto error;
    }
  if(cdfp->numDims > 0 && cdfrotate(cdfp, vpdata, data)==ERROR)
    goto error;

  if(!(DXSetComponentValue(f, "data", (Object)a)))
    goto error;
  a = NULL;

  if(!(obj=(Object)DXNewString(vpdata->name)))
    goto error;
  if(!(DXSetComponentAttribute(f, "data", "name", obj)))
    goto error;
  obj = NULL;

  /* if there are positions then there are conn too for dim>0 */
  if (cdfp->numDims > 0 && pos)
    {
      if(cdfp->numDims == vpdata->dataDims){
        if(!(DXSetComponentValue(f, "positions", (Object)pos)))
	  goto error;
        *posdel=0; 
	if(!(DXSetComponentValue(f, "connections", (Object)con)))
	  goto error;
	*condel=0;
        obj = NULL;
        if(!(obj=(Object)DXNewString("positions")))
	  goto error;
        DXSetComponentAttribute(f, "connections", "ref",obj);
        DXSetComponentAttribute(f, "data", "dep", obj);
      }
      /* create positions same dims as the data by using the terms
       * from the product array in the dimension that varys */
      else if (cdfp->numDims > 1 && 
	  DXGetProductArrayInfo((ProductArray)pos,&n,posterms) && 
	  DXGetMeshArrayInfo((MeshArray)con,NULL,conterms) ){
	for (i=0; i<n; i++) {
	  if (vpdata->dimVariances[i] == VARY) {
	    newpos[j] = posterms[i];
	    newcon[j] = conterms[i];
	    j++;
	  }
	}
	if (j > 0 && j == vpdata->dataDims){
	  a = (Array)DXNewProductArrayV(j,newpos);
	  if (!a)
	    goto error;
	  if (!DXSetComponentValue(f,"positions",(Object)a))
	    goto error;
          *posdel=0;
	  a = NULL;
	  a = (Array)DXNewMeshArrayV(j,newcon);
	  if (!a)
	     goto error;
	  if (!DXSetComponentValue(f,"connections",(Object)a))
	    goto error;
	  *condel=0;
	  a = NULL;
	  obj = NULL;
	  if (!(obj=(Object)DXNewString(vpdata->conn)))
	     goto error;
	  DXSetComponentAttribute(f,"connections","element type",obj);
          obj = NULL;
          if(!(obj=(Object)DXNewString("positions")))
	    goto error;
          DXSetComponentAttribute(f, "connections", "ref",obj);
          DXSetComponentAttribute(f, "data", "dep", obj);
        }
	else
	   DXWarning("Can't calculate positions and connections");
      }
    }
  else if (pos)
    {
        if(!(DXSetComponentValue(f, "positions", (Object)pos)))
	  goto error;
        *posdel=0; 
        obj = NULL;
        if(!(obj=(Object)DXNewString("positions")))
	  goto error;
        DXSetComponentAttribute(f, "data", "dep", obj);
    } 

  obj = NULL;
 
  /* get CDF attributes and create DX attributes 
   * only Variable attributes for data are carried through for now
   */
  if (!GetAttribute(vpdata,(Object)f,1))
     goto error; 

  if( !(DXEndField(f)) )
    goto error;
  
  return f;
  
 error:
  if (f)
    DXDelete ((Object)f);
  if (a)
    DXDelete ((Object)a);
  if (obj)
    DXDelete (obj);
  if (datacm)
    DXFree ((Pointer)datacm);
  return NULL;
  
}



/*
  This routine reads data from the CDF file from values in structure
  vp and load the values in array data.
  */

static Error 
cdfGetData(Infovar vp, long rec, void *data)
{
  char msg[CDF_ERRTEXT_LEN+1];
  CDFstatus status;
  void *datacm;
  int i,off1,off2;

  /*
    Read record number rec into array data.
    */

  status = CDFvarHyperGet(vp->id, vp->varNum, rec, 1, 1, vp->idx, 
			  vp->cnt, vp->intval, (void *)data);
  if (status < CDF_OK)
   {
      CDFerror(status,msg);
      DXSetError(ERROR_MISSING_DATA,msg);
      goto error;
   }
  
  if (vp->dxtype == TYPE_STRING)
  {
      if((datacm = (char *)DXAllocate(vp->size * vp->numbytes))==NULL)
        goto error;
      memcpy((char *)datacm,(char *)data,(vp->size * vp->numbytes));
      for (i=0; i<vp->size; i++)
	{
	  off1=vp->numbytes * i;
	  off2=off1 + i +vp->numbytes;
	  memcpy((char *)data+off1+i, (char *)datacm+off1, vp->numbytes);
          memset((char *)data+off2,'\0',1);
	}
      DXFree((Pointer)datacm);
  }

  return OK;

 error:
  return ERROR;
}

/*
  Open the cdf file and load the structure cdfp. Return OK if successful
  and ERROR if failed.
  Looks at the DXDATA environment variable and tries to locate the file
  in all those directories and if it still cannot find the data file it
  then returns an ERROR.
 */

static Error 
openCDF(char *filename, Infocdf *cdfp, int fill)
{
  CDFid id;
  CDFstatus status;
  char msg[CDF_ERRTEXT_LEN+1], *datadir = NULL, *cp; 
  char *outname = NULL;
  int i;

#define XTRA 12

  /*
    Try to open the cdf file in the current directory.
    */

  status = CDFopen(filename, &id);
  if (status >= CDF_OK)
    goto inquire;

  /*
    If it was an absolute path and if couldn't open the file then
    return error.
    */

#ifndef DXD_NON_UNIX_DIR_SEPARATOR
  if (filename[0] == '/')
    goto error;
#else
  if (filename[0] == '/' || filename[1] == ':' || filename[0] == '\\')
    goto error;
#endif

  /*
    Get the data directory paths.
    */

  datadir = (char *)getenv("DXDATA");
  outname = (char *)DXAllocateLocalZero((datadir ? strlen(datadir) : 0) +
			strlen(filename) + XTRA);

  /*
    Try if you can the file in each one of those paths delimited by
    ':'. If not found in any of those paths then return an error.
    */

  while (datadir) 
    {
      strcpy(outname, datadir);
      if((cp = strchr(outname, PATH_SEP_CHAR)) != NULL)
	*cp = '\0';
      strcat(outname, "/");
      strcat(outname, filename);
      status = CDFopen(outname, &id);
      if (status >= CDF_OK)
	goto inquire;
      datadir = strchr(datadir, PATH_SEP_CHAR);
      if (datadir)
	datadir++;
    }
  
  /* couldn't find the filename, return ERROR */
  goto error;

 inquire:

  /* we're done with outname now, so free it here */
  if (outname) {
    DXFree(outname);
    outname = NULL;
  }

  if (!fill)
    return OK;

  if((*cdfp = (struct infocdf *)DXAllocate(sizeof(struct infocdf))) == NULL )
    goto error;
  
  /* 
    set up cdf information 
    */
  status = CDFinquire(id, &((*cdfp)->numDims), (*cdfp)->dimSizes,
		      &((*cdfp)->encoding), &((*cdfp)->majority), 
		      &((*cdfp)->maxRec), &((*cdfp)->numVars), 
		      &((*cdfp)->numAttrs));
  if( status < CDF_OK )
    goto error;

  /* set defaults for rotate */
  for (i=0; i<CDF_MAX_DIMS; i++)
    (*cdfp)->rotate[i] = 0;

  (*cdfp)->id = id;
  return OK;
  
 error:
  if (outname)
    DXFree((Pointer)outname);
  if (status < CDF_OK)
    {
      CDFerror(status, msg);
      DXSetError(ERROR_BAD_PARAMETER, msg);
    }
  return ERROR;
}


/*
  Converts data points in src array which is column major to des array
  which is row major. The dimension size information is acessed from
  cdfp structure while the byte size of the varaibale and other infromation
  is read from vpdata structure.
  Warning: Returns ERROR when number of dimensions is greater than 4.
  */

static Error
conCol2Row(void *des, void *src, Infovar vpdata, Infocdf cdfp)
{
  int  xskip, k, x, y, z, v, offset, off1, off2, off3, off4;
  
  k = 0;
  if ( cdfp->numDims == 1 )
    memcpy((char*)des, (char*)src, vpdata->size*vpdata->numbytes);
  else if (cdfp->numDims == 2)
    {
      xskip = cdfp->dimSizes[1] * vpdata->numbytes;
      for (y = 0; y < cdfp->dimSizes[1]; y++)
        {
          offset = y * vpdata->numbytes;
          for (x = 0; x < cdfp->dimSizes[0]; x++)
            {
              memcpy((char*)des + offset, (char*)src + k, vpdata->numbytes);
              k += vpdata->numbytes;
              offset += xskip;
            }
        }
    }
  else if (cdfp->numDims == 3)
    {
      off1 = cdfp->dimSizes[1] * cdfp->dimSizes[2];
      for (z = 0; z < cdfp->dimSizes[2]; z++)
	{
	  for (y = 0; y < cdfp->dimSizes[1]; y++)
	    {
	      off2 = (y * cdfp->dimSizes[2]) + z;
	      for (x = 0; x < cdfp->dimSizes[0]; x++)
		{ 
		  offset = (x * off1) + off2;
		  offset *= vpdata->numbytes;
	          memcpy((char*)des + offset, (char*)src + k, 
			 vpdata->numbytes);
	          k += vpdata->numbytes;
		}
	    }
	}
    }
  else if (cdfp->numDims == 4)
    {
      off1 = cdfp->dimSizes[1] * cdfp->dimSizes[2] * cdfp->dimSizes[3];
      off2 = cdfp->dimSizes[2]*cdfp->dimSizes[3];
      for (v = 0; v < cdfp->dimSizes[3]; v++)
	{
	  for (z = 0; z < cdfp->dimSizes[2]; z++)
	    {
	      off4 = (z * cdfp->dimSizes[3]) + v;
	      for (y = 0; y < cdfp->dimSizes[1]; y++)
		{
		  off3 = (y * off2) + off4;
		  for (x = 0; x < cdfp->dimSizes[0]; x++)
		    { 
		      offset = ((x * off1) + off3) * vpdata->numbytes;
	              memcpy((char*)des + offset, (char*)src + k, 
			     vpdata->numbytes);
	              k += vpdata->numbytes;
		    }
		}
	    }
	}
    }
  else 
    { 
      DXSetError(ERROR_NOT_IMPLEMENTED,
	       "Cannot change majority of vectors with %d dimensions", 
	       cdfp->numDims);
      goto error;
    }
  return OK;
  
 error:
  return ERROR;
}

/*
  Circular rotation of data points is performed along each axis of data.
  Rotation along dimension is performed only if rotation field is not 
  equal to 0.
  This happens usually in geographical data which might need circular
  rotation.
  */

static Error 
cdfrotate(Infocdf cdfp, Infovar vpdata, void *data)
{
  void *tmparr;
  int off1, off2, i, j, tempi1, tempi2;

  tmparr = NULL;

  if ( cdfp->numDims == 1 )
    {
      if (cdfp->rotate[0] != 0)
	{
	  if((tmparr = (void *)DXAllocate(vpdata->size*vpdata->numbytes))==NULL)
	    goto error;
	  memcpy((char*)tmparr, (char*)data, vpdata->size*vpdata->numbytes);
	  off1 = cdfp->rotate[0] * vpdata->numbytes;
	  off2 = (cdfp->dimSizes[0] - cdfp->rotate[1]) * vpdata->numbytes;
	  memcpy((char*)data, (char*)tmparr+off1, off2);
	  memcpy((char*)data+off2, (char*)tmparr, off1);
	  DXFree((Pointer)tmparr);
	}  
    }
  else if (cdfp->numDims == 2)
    {
      if (cdfp->rotate[0] != 0)
	{
	  if((tmparr = (void *)DXAllocate(vpdata->size*vpdata->numbytes))==NULL)
	    goto error;
	  memcpy((char*)tmparr, (char*)data, vpdata->size*vpdata->numbytes);
	  off1 = cdfp->dimSizes[1] * cdfp->rotate[0] * vpdata->numbytes;
	  off2 = cdfp->dimSizes[1] * (cdfp->dimSizes[0] - cdfp->rotate[0]) *
	    vpdata->numbytes;
	  memcpy((char*)data, (char*)tmparr+off1, off2);
	  memcpy((char*)data+off2, (char*)tmparr, off1);
	  DXFree((Pointer)tmparr);
	}
      if (cdfp->rotate[1] != 0)
	{
	  if((tmparr = (void *)DXAllocate(vpdata->size*vpdata->numbytes))==NULL)
	    goto error;
	  memcpy((char*)tmparr, (char*)data, vpdata->size*vpdata->numbytes);
	  off1 = cdfp->rotate[1] * vpdata->numbytes;
	  off2 = (cdfp->dimSizes[1] - cdfp->rotate[1]) * vpdata->numbytes;
	  tempi1 = 0;
	  tempi2 = cdfp->dimSizes[1] * vpdata->numbytes;
	  for (i=0; i<cdfp->dimSizes[0]; i++)
	    {
	      memcpy((char*)data+tempi1, (char*)tmparr+off1+tempi1, off2);
	      memcpy((char*)data+off2+tempi1, (char*)tmparr+tempi1, off1);
	      tempi1 += tempi2;
	    }
	  DXFree((Pointer)tmparr);
	}
    }
  else if (cdfp->numDims == 3)
    {
      if (cdfp->rotate[0] != 0)
	{
	  if((tmparr = (void *)DXAllocate(vpdata->size*vpdata->numbytes))==NULL)
	    goto error;
	  memcpy((char*)tmparr, (char*)data, vpdata->size*vpdata->numbytes);
	  off1 = cdfp->dimSizes[2] * cdfp->dimSizes[1] * 
	    cdfp->rotate[0] * vpdata->numbytes;
	  off2 = cdfp->dimSizes[2] * cdfp->dimSizes[1] * 
	    (cdfp->dimSizes[0] - cdfp->rotate[0]) * vpdata->numbytes;
	  memcpy((char*)data, (char*)tmparr+off1, off2);
	  memcpy((char*)data+off2, (char*)tmparr, off1);
	  DXFree((Pointer)tmparr);
	}
      if (cdfp->rotate[1] != 0)
	{
	  if((tmparr = (void *)DXAllocate(vpdata->size*vpdata->numbytes))==NULL)
	    goto error;
	  memcpy((char*)tmparr, (char*)data, vpdata->size*vpdata->numbytes);
	  off1 = cdfp->dimSizes[2] * cdfp->rotate[1] * vpdata->numbytes;
	  off2 = cdfp->dimSizes[2] * (cdfp->dimSizes[1] - cdfp->rotate[1])
	    * vpdata->numbytes;
	  tempi1 = 0;
	  tempi2 = cdfp->dimSizes[1] * vpdata->numbytes;
	  for (i=0; i<cdfp->dimSizes[0]; i++)
	    {
	      memcpy((char*)data+tempi1, (char*)tmparr+off1+tempi1, off2);
	      memcpy((char*)data+off2+tempi1, (char*)tmparr+tempi1, off1);
	      tempi1 += tempi2;
	    }
	  DXFree((Pointer)tmparr);
	}
      if (cdfp->rotate[2] != 0)
	{
	  if((tmparr = (void *)DXAllocate(vpdata->size*vpdata->numbytes))==NULL)
	    goto error;
	  memcpy((char*)tmparr, (char*)data, vpdata->size*vpdata->numbytes);
	  off1 = cdfp->rotate[2] * vpdata->numbytes;
	  off2 = (cdfp->dimSizes[2] - cdfp->rotate[2]) * vpdata->numbytes;
	  tempi1 = 0;
	  tempi2 = cdfp->dimSizes[1] * cdfp->dimSizes[2] * vpdata->numbytes;
	  for (i=0; i<cdfp->dimSizes[0]; i++)
	    {
	      for (j=0; j<cdfp->dimSizes[1]; j++)
		{
		  memcpy((char*)data+tempi1, (char*)tmparr+off1+tempi1, off2);
		  memcpy((char*)data+off2+tempi1, (char*)tmparr+tempi1, off1);
		  tempi1 += tempi2;
		}
	    }
	  DXFree((Pointer)tmparr);
	}
    }
  else
    { 
      DXSetError(ERROR_NOT_IMPLEMENTED,
	       "Cannot rotate cdf data with %d dimensions", cdfp->numDims);
      goto error;
    }
  return OK;
  
 error:
  if (tmparr)
    DXFree((Pointer)tmparr);
  return ERROR;
}

/*
  This extracts the attributes from CDF variables and creates 
  DX attributes for components of the field input.
  */
static Error GetAttribute(Infovar vp, Object f,int var)
{
   long i=0;
   int k;
   CDFstatus status;
   char msg[CDF_ERRTEXT_LEN+1],aname[CDF_ATTR_NAME_LEN+1];
   char *attrstring;
   long ascope, numentry,datatype,attrsize,entry;
   void *value;
   Type type;
   Object obj=NULL;
   Array a=NULL;

   /* check if there are any attributes for this variable */
   while((status = CDFattrInquire(vp->id,i,aname,&ascope,&numentry))>=CDF_OK){ 

    if (var==1 && (ascope==VARIABLE_SCOPE || ascope==VARIABLE_SCOPE_ASSUMED))
      entry = CDFvarNum(vp->id,vp->name);
    else if (var==0 && (ascope==GLOBAL_SCOPE || ascope==GLOBAL_SCOPE_ASSUMED))
      entry = numentry;
    else{
      i++;
      continue;
    }
     
      /* place a NULL at first blank character. This will help when
       * selecting an Attribute in DX */
      k=0;
      while (k<strlen(aname) && aname[k] != ' ')
	 k++;
      aname[k] = '\0';

      status = CDFattrEntryInquire(vp->id,i,entry,&datatype,&attrsize);
      if (status >= CDF_OK){
         /* if datatype is char then value is string with length of attrsize */
         if (datatype == CDF_CHAR){
	   attrstring = (char *)DXAllocate(attrsize+1);
	   status = CDFattrGet(vp->id,i,entry,attrstring);
 	   if (status < CDF_OK){
		CDFerror(status,msg);
		DXSetError(ERROR_BAD_PARAMETER,msg);
		goto error;
	   }
	   k = attrsize-1;
	   while (k>0 && (attrstring[k] == ' ' || attrstring[k] == '\0'))
	      k--;
	   attrstring[++k] = '\0';
	   obj = NULL;
	   if (!(obj = (Object)DXNewString(attrstring)))
		goto error;
         }
         else{
	   /* for numeric types create an array object */
	   if (!cdftype2dxtype(datatype,&type))
 		goto error;
	   a = DXNewArray(type,CATEGORY_REAL,0);
	   if (!a)
		goto error;
	   if (!DXAddArrayData(a,0,attrsize,NULL))
		goto error;
	   value = (void *)DXGetArrayData(a);
	   if (!value)
		goto error;
	
	   status = CDFattrGet(vp->id,i,entry,value);
 	   if (status < CDF_OK){
		CDFerror(status,msg);
		DXSetError(ERROR_BAD_PARAMETER,msg);
		goto error;
	   }
	   obj=NULL;
	   obj = (Object)a;
         }
         if (var == 1){
           if (!DXSetAttribute((Object)f,aname,obj))
              goto error;
         }
         else{
	   if (!DXSetAttribute((Object)f,aname,obj))
	      goto error;
         }
      }
      i++;
   }
   return OK;

error:
   if (a)
	   DXDelete((Object)a);
   if (obj)
	   DXDelete((Object)obj);
   return ERROR;

}


/*
  This contains routines pertaining to positions calculations for
  CDF data and returns a positions array.
  (originally posCDF.c)
  */

static Infovar locAxis(int n, Infovar vploc);
static Error buildAxis(int n,Infocdf cdfp,Infovar vp,float *data,float *isreg);
static Error buildAxis2(Infocdf cdfp, Infovar vp, float *data, int cntvar);

/*
  Return an array of positions from the variable names stored in vploc
  structure. if vploc is NULL then generates a default positions with
  origin as 0 and step by 1 in each dimension.
 */

Array impcdfpos(Infovar vploc, Infocdf cdfp)
{
  Array a = NULL, terms[CDF_MAX_DIMS];
  Infovar p;
  int i, j, shape[8], dimsize[CDF_MAX_DIMS];
  float *data,isreg;
  float origin[CDF_MAX_DIMS], deltas[CDF_MAX_DIMS];

  data = NULL;

  for (i=0; i<CDF_MAX_DIMS; i++)
      terms[i] = NULL;

/*
  No positions variables in cdf data but number of dimensions is 
  greater than 1. This case positions are generated with origin 0
  and delta 1 along each dimension.
  */

  if (vploc == NULL && cdfp->numDims >= 1)
    {
      for (i=0; i<cdfp->numDims; i++)
	{
	  origin[i] = 0.0;
	  dimsize[i] = cdfp->dimSizes[i];
	}
      for (i=0; i<cdfp->numDims*cdfp->numDims; i++)
	{
	  deltas[i] = 0.0;
	}
      for (i=0, j=0; i<cdfp->numDims; i++)
        {
          deltas[j] = 1.0;
          j += cdfp->numDims + 1;
        }
      if( !(a = DXMakeGridPositionsV(cdfp->numDims, dimsize,
				   origin, deltas)) )
        goto error;
      goto ok;
    }

/*
  This is for point data where there are no cdf variables which are
  categorized as positions. This case returns a NULL positions array.
  */

  if (vploc == NULL && cdfp->numDims < 1)
    goto ok;

/*
  Get this line only if vploc is not NULL which means positions are
  available. Positions are usually scalar or n-vector for dimensions
  less than 2.
  */

  if (cdfp->numDims <= 1)
    {
      shape[0] = cntVars(vploc);
      a = DXNewArrayV(TYPE_FLOAT, CATEGORY_REAL, 1, shape);
      if(!a) 
	goto error;
      if( !DXAddArrayData(a, 0, vploc->size, NULL) ) 
	goto error;
      data = (float *)DXGetArrayData( a );
      if(!data) 
	goto error;
      if (buildAxis2(cdfp, vploc, data, shape[0])==ERROR)
	goto error;
    }

  /*
    Builds a positions product array for CDF data with more than one 
    dimensions.
    */
  
  else if (cdfp->numDims > 1)
    {
      for(i=0; i<cdfp->numDims; i++)
	{
	  p = locAxis(i, vploc);
	  if(!p) 
	    goto error;
	  
	  shape[0] = cdfp->numDims;
	  
	  a = DXNewArrayV(TYPE_FLOAT, CATEGORY_REAL, 1, shape);
	  
	  if(!a) 
	    goto error;
	  
	  if( !DXAddArrayData(a, 0, p->size, NULL) ) 
	    goto error;
	 
	  data = (float *)DXGetArrayData( a );
	  
	  if(!data) 
	    goto error;
	  
	  if (buildAxis(i, cdfp, p, data,&isreg)==ERROR)
	    goto error;
	 
          if (isreg){
    	    for (j=0; j<cdfp->numDims; j++){
      	       deltas[j] = 0;
	       origin[j] = (float)data[j];
	    } 
    	    deltas[i] = isreg;
	    DXDelete((Object)a);
	    a = NULL;
    	    a=(Array)DXNewRegularArray(TYPE_FLOAT,(int)(cdfp->numDims),p->size,
				origin,deltas);
    	    if (!a)
      	       goto error;
          }

	  terms[i] = a;
	  a = NULL;
	  data = NULL;
	}
      
      a = (Array)DXNewProductArrayV(cdfp->numDims, terms);
      if(!a)
	goto error;
    }
 ok:
  return a;
  
 error:
  if (a)
    DXDelete ((Object)a);
  for (i=0; i<CDF_MAX_DIMS; i++)
    if (terms[i])
      DXDelete ((Object)terms[i]);
  return NULL;
}

/*
  Returns an address of the structure which matches the dimension
  variance to VARY.
 */

static Infovar locAxis(int n, Infovar vploc)
{
  Infovar ptr = vploc;
  
  while( ptr )
    {
      if( ptr->dimVariances[n] == VARY)
	return ptr;
      ptr = ptr->next;
    }
  DXSetError(ERROR_BAD_PARAMETER, "%d_th axis variable invalid.", n);
  return NULL; 
}


/*
  Loads the axis values in array data as dx wants it to
  form a product array.
  The cdfp->rotate field is loaded with the correct rotation values in 
  this subroutine.
  Returns ERROR if it fails.
 */

static Error buildAxis(int n, Infocdf cdfp, Infovar vp, float *data,float *diff)
{
  char *d, *dp;
  float *mydata, *mydata2;
  /* float *delta; */
  int i, numbytes, size, dims, off1, off2;
  Error err;

  d = NULL;
  mydata = NULL;
  dims = cdfp->numDims;
  numbytes = vp->numbytes;
  size = vp->size;
  
  if((d = (char *)DXAllocate(size * numbytes))==NULL)
      goto error;
  if((mydata = (float *)DXAllocate(size * sizeof(float)))==NULL)
      goto error;

  dp = d;
  if (cdfGetData(vp, 0, d)==ERROR)
    goto error;

  memset(data, '\0', (size) * sizeof(float) * dims);

  /*
    Convert different kinds of data types to float for dx positions.
    */
  
  switch(vp->dxtype)
    {
    case TYPE_BYTE:
      for (i=0; i<size; i++)
	{
	  mydata[i] = (float)*((byte *)(dp));
	  ++dp;
	}
      break;
    case TYPE_UBYTE:
      for (i=0; i<size; i++)
	{
	  mydata[i] = (float)*((ubyte *)(dp));
	  ++dp;
	}
      break;
    case TYPE_SHORT:
      for (i=0; i<size; i++)
	{
	  mydata[i] = (float)*((short *)(dp));
	  dp+=2;
	}
      break;
    case TYPE_USHORT:
      for (i=0; i<size; i++)
	{
	  mydata[i] = (float)*((ushort *)(dp));
	  dp+=2;
	}
      break;
    case TYPE_INT:
      for (i=0; i<size; i++)
	{
	  mydata[i] = (float)*((int *)(dp));
	  dp+=4;
	}
      break;
    case TYPE_UINT:
      for (i=0; i<size; i++)
	{
	  mydata[i] = (float)*((uint *)(dp));
	  dp+=4;
	}
      break;
    case TYPE_FLOAT:
      for (i=0; i<size; i++)
	{
	  mydata[i] = *((float *)(dp));
	  dp+=4;
	}
      break;
    case TYPE_DOUBLE:
      for (i=0; i<size; i++)
	{
	  mydata[i] = (float)*((double *)(dp));
	  dp+=8;
	}
      break;
    default:
      DXErrorGoto(ERROR_BAD_TYPE,"Position values of bad type");
      break;
    }
  if (!DXFree((Pointer)d))
     goto error;
  d = NULL;

  /*
    This might need some changes later on . In CDF values are center node 
    unlike dx.
    */

  i = 1;
  if (mydata[0] < mydata[1])
    {
      while ((i<size) && (mydata[i] > mydata[i-1]))
	i++;
    }
  else if (mydata[0] > mydata[1])
    {
      while ((i<size) && (mydata[i] < mydata[i-1]))
	i++;
    }

  if (i != size)
    cdfp->rotate[n] = i;
  else
    cdfp->rotate[n] = 0;

  /*
    Rotate positions values if it need any circular rotation.
    */

  if (cdfp->rotate[n] != 0)
    {
      if((mydata2 = (float *)DXAllocate(size * sizeof(float)))==NULL)
	goto error;
      memcpy((char*)mydata2, (char*)mydata, sizeof(float)*size);
      off1 = cdfp->rotate[n]*sizeof(float);
      off2 = (size - cdfp->rotate[n])*sizeof(float);
      memcpy((char*)mydata, (char*)mydata2+off1, off2);
      memcpy((char*)mydata+off2, (char*)mydata2, off1);
      DXFree((Pointer)mydata2);

    }
  for (i=0; i<size; i++)
    {
      data[(i*dims)+n] = mydata[i];
    }

  /* check for a regular array and create it if exists */
  i=1;
  *diff = mydata[1] - mydata[0];
  while (i < size && (mydata[i] - mydata[i-1])==*diff )
    i++;
  if (i != size){
    *diff = 0;
    /* 
    delta = (float *)DXAllocate(sizeof(float) *dims);
    if (!delta)
      goto error;
    for (i=0; i<dims; i++)
      delta[i] = 0;
    delta[n] = diff;
    a = DXNewRegularArray(TYPE_FLOAT,dims,size,data,delta);
    if (!a)
      goto error;
   */
  }

  if ((err = DXFree((Pointer)mydata))==ERROR)
     goto error;

  return OK;
  
 error:
  if (d)
    DXFree((Pointer)d);
  if (mydata)
    DXFree((Pointer)mydata);
  return ERROR;
}

/*
  This routine is called for CDF data having less than 2 dimensions.
  It doesn't worry about the data dep to connections as this case it
  is data dep positions. It doesn't calculate the positions value 
  for center of a cell.
  */

static Error buildAxis2(Infocdf cdfp, Infovar vp, float *data, int cntvar)
{
  char *d, *dp;
  float *mydata;
  int i, numbytes, size, dims, n = 0;
  long recStart, recCount, recInterval;
  Infovar ptr;
  CDFstatus status;
  char msg[CDF_ERRTEXT_LEN+1];

  d = NULL;
  mydata = NULL;
  dims = cdfp->numDims;
  numbytes = vp->numbytes;
  size = vp->size;
  recStart = vp->strframe;
  recInterval = vp->dltframe;
  recCount = vp->size;

  memset(data, '\0', cntvar * size * sizeof(float));
  
  ptr = vp;
  while (ptr)
    {
      if((d = (char *)DXAllocate(size * numbytes))==NULL)
	goto error;
      if((mydata = (float *)DXAllocate(size * sizeof(float)))==NULL)
	goto error;
      
      dp = d;
      if (dims == 0)
	{
	  status = CDFvarHyperGet(ptr->id, ptr->varNum, recStart,
				  recCount, recInterval, ptr->idx,
				  ptr->cnt, ptr->intval, (void *)d);
	  
	  if (status < CDF_OK)
	    {
	      CDFerror(status, msg);
	      DXSetError(ERROR_MISSING_DATA, msg);
	      goto error;
	    }
	}
      else
	{
	  if (cdfGetData(ptr, 0, d)==ERROR)
	    goto error;
	}
      
      switch(ptr->dxtype)
	{
	case TYPE_BYTE:
	  for (i=0; i<size; i++)
	    {
	      mydata[i] = (float)*((byte *)(dp));
	      ++dp;
	    }
	  break;
	case TYPE_UBYTE:
	  for (i=0; i<size; i++)
	    {
	      mydata[i] = (float)*((ubyte *)(dp));
	      ++dp;
	    }
	  break;
	case TYPE_SHORT:
	  for (i=0; i<size; i++)
	    {
	      mydata[i] = (float)*((short *)(dp));
	      dp+=2;
	    }
	  break;
	case TYPE_USHORT:
	  for (i=0; i<size; i++)
	    {
	      mydata[i] = (float)*((ushort *)(dp));
	      dp+=2;
	    }
	  break;
	case TYPE_INT:
	  for (i=0; i<size; i++)
	    {
	      mydata[i] = (float)*((int *)(dp));
	      dp+=4;
	    }
	  break;
	case TYPE_UINT:
	  for (i=0; i<size; i++)
	    {
	      mydata[i] = (float)*((uint *)(dp));
	      dp+=4;
	    }
	  break;
	case TYPE_FLOAT:
	  for (i=0; i<size; i++)
	    {
	      mydata[i] = *((float *)(dp));
	      dp+=4;
	    }
	  break;
	case TYPE_DOUBLE:
	  for (i=0; i<size; i++)
	    {
	      mydata[i] = (float)*((double *)(dp));
	      dp+=8;
	    }
	  break;
	default:
	  DXErrorGoto(ERROR_BAD_TYPE,"Position values of bad type");
	  break;
	}
      DXFree((Pointer)d);
      
      for (i=0; i<size; i++)
	{
	  data[(i*cntvar)+n] = mydata[i];
	}
      DXFree((Pointer)mydata);
      n++;
      ptr = ptr->next;
    }
  return OK;
  
 error:
  if (d)
    DXFree((Pointer)d);
  if (mydata)
    DXFree((Pointer)mydata);
  return ERROR;
}


/*
  Routines which loads up the Infovar structures from the CDF variable
  are grouped in this file.
  (originally queryCDF.c)
  */


/* 
  This happens to be the heart of cdf importer. This routine queries
  information about each cdf variable. 
  It loads the structure vpdata, vploc and vptime from the varname
  defined by user if given or else from all the variables available
  in the cdf file.
  It returns with vpdata having information about data component,
  vploc having information about positions and connections component,
  and vptime having information about series positions value. It gives
  warnings to user about the variables it ignores while converting
  data.
  If variable names (varname is not NULL) are defined in import level 
  then query for only those variables.
 */

static Error
queryCDFvars(Infocdf cdfp, Infovar *vpdata, Infovar *vploc,
		   Infovar *vptime, char **varname, 
		   int startframe, int endframe, int deltaframe)
{
  int i, k, vtype, tocnvt = 0;
  Infovar ptr;
  char msg[CDF_ERRTEXT_LEN+1];
  CDFstatus status;
  int varcnt=0;

  /*
    Number of variables to be imported will be equal to the number of
    variables defined by the user and if not defined will import all
    the variables.
    */

  /* check for valid variable names */
  if (varname)
    {
       if ((varcnt = getHowMany(varname))<=0)
	{
	  DXSetError(ERROR_BAD_PARAMETER,
		   "Number of variables has to be greater than zero");
	  goto error;
	}
	for (i=0; i<varcnt; i++){
	      if ((status = CDFvarNum(cdfp->id, varname[i])) < 0)
		{
		  CDFerror(status, msg);
		  DXSetError(ERROR_BAD_PARAMETER,"%s : %s", varname[i], msg);
		  goto error;
		}
	 }
    }

  tocnvt = cdfp->numVars;

  for(i=0; i<tocnvt; i++)
    {
      ptr = NULL;
      if((ptr = (struct infovar *)DXAllocate(sizeof(struct infovar))) == NULL )
	goto error;
      
      ptr->id = cdfp->id;
      
      /*
	If variable names are defined then query for only those variables.
      if (varname)
	{
          if (varname[i])
	    {
	      if ((status = CDFvarNum(cdfp->id, varname[i])) < 0)
		{
		  DXFree((Pointer)ptr);
		  CDFerror(status, msg);
		  DXSetError(ERROR_BAD_PARAMETER,"%s : %s", varname[i], msg);
		  goto error;
		}
	      else
		ptr->varNum = (long)status;
	    }
	}
      else
      */

      ptr->varNum = i;
      status = CDFvarInquire(ptr->id, ptr->varNum, ptr->name, 
			     &(ptr->dataType),
			     &(ptr->numElements), &(ptr->recVariance), 
			     ptr->dimVariances);
      if (status < CDF_OK)
	{
	  DXFree((Pointer)ptr);
	  CDFerror(status, msg);
	  DXErrorGoto(ERROR_BAD_PARAMETER,msg);
	}

      /*
	Put a null at the first character encountered as blank.
	This will help if select module is used in a network to select
	a field.
	*/
      
      k = 0;
      while (k<strlen(ptr->name) && ptr->name[k] != ' ')
	k++;
      ptr->name[k] = '\0';

      ptr->strframe = startframe;
      ptr->endframe = endframe;
      ptr->dltframe = deltaframe;
      
      /*
	categorize variable 
	*/
      
      vtype = categorizeVar(cdfp->numDims, ptr);
      
      if(vtype == DATACOMP)
	addVar(ptr, vpdata);
      else if(vtype == POSITIONCOMP)
	addVar(ptr, vploc);
      else if(vtype == SERPOSCOMP)
	addVar(ptr, vptime);
      else
	{
	  DXFree((Pointer)ptr);
	  goto error;
	}
    }

/*
  Keep just the valid positions variable, time series positions variable
  and move the rest to a data variable.
  */
  
  if(cleanPosVar(vploc,cdfp, vpdata) == ERROR)
    goto error;
  if(cleanSerVar(vptime,cdfp,vpdata) == ERROR)
    goto error;
  if(cleanDataVar(*vpdata,cdfp) == ERROR)
    goto error;

/* if variables were specified remove all DATA variables not matching 
   if variable specified is a POSITION or SERIES type then give warning and 
   move to vpdata
   */
   if (varcnt>0){
      if (!namedVar(vpdata,vploc,vptime,varname,cdfp->numDims))
	 goto error;
   }


/*
  Load the link lists with some more information regarding the variables.
  Get the lists ready to be used by the CDF read routines which expect
  the start, end, interval, number of bytes and other details.
  */
  
  if(completeVarsInfo(cdfp, *vpdata, *vploc, *vptime)==ERROR)
    goto error;

  return OK;
  
 error:
  return ERROR;
}

/* 
  How many variable names given
  */
static int getHowMany(char **names)
{
  int i, tocnvt=0;
  for (i=0; names[i]; i++)
    tocnvt++;
  return tocnvt;
}

/* 
  Return with only the Variables specified by the user in varname
  in vpdata, free the others 
  */
static Error namedVar(Infovar *vpdata,Infovar *vploc,Infovar *vptime,
			char **varname,int numDims)
{
   int match = 0,matchcnt=0,n=0,i;
   Infovar ptr;
   int varcnt;

   varcnt = getHowMany(varname);

   ptr = *vpdata;
   while (ptr){
      match = 0;
      for (i=0; i<varcnt; i++){
         if (!strncmp(ptr->name,varname[i],strlen(ptr->name))){
           match = 1;
	   matchcnt++;
           break;
         }
      }
      if (match==0)
         removeVar(vpdata,n);
      else
         n++;
      ptr = ptr->next;
   }
   
   if (matchcnt == varcnt)		/* found all matching names */
      return OK;
   else{			/* var can't be positions variable */
      for (i=0; i<varcnt; i++){
	 ptr = *vpdata;
	 match = 0;
	 while(ptr){
            if (!strncmp(ptr->name,varname[i],strlen(ptr->name))){
	       match=1;
	       break;
	    }
	    ptr = ptr->next;
	 }
         if (match == 0){
           DXSetError(ERROR_BAD_PARAMETER,
	     "%s can't be variable, it is imported as positions or series positions ", varname[i] );
              return ERROR;
         }
      }
   }

   /* DO NOT USE FOR POSITIONS and SERIES 
      causes problems downstream because size of arrays do not match 
      */
   ptr = *vploc;
   n=0;
   while (ptr){
      for (i=0; varname[i]; i++){
	 if (!strncmp(ptr->name,varname[i],strlen(ptr->name))){
	    matchcnt++;
	    copyVar(ptr,vpdata,numDims);
	 }
      }
      ptr = ptr->next;
   }

   if (matchcnt == i)		/* found all matching names */
      return OK;

   ptr = *vptime;
   n=0;
   while (ptr){
      for (i=0; varname[i]; i++){
	 if (!strncmp(ptr->name,varname[i],strlen(ptr->name)))
	    copyVar(ptr,vpdata,numDims);
      }
      ptr = ptr->next;
   }

   return OK;
}

 
/*
  If vpdata is NULL then returns an ERROR.
  */

static Error cleanDataVar(Infovar vpdata, Infocdf cdfp)
{
  if (vpdata == NULL)
    {
      DXSetError(ERROR_MISSING_DATA, "No data component is found");
      goto error;
    }
  return OK;
  
 error:
  return ERROR;
}

/*
  According to the dimensionality of the CDF data updates vploc.
  When something unexpected happens it returns and ERROR.
  */

static Error cleanPosVar(Infovar *vploc, Infocdf cdfp,Infovar *vpdata)
{
  Infovar fvp, ptr;
  int i, n, lat = 0, lon = 0;
  int latlat=0, lonlon=0;

  if (cdfp->numDims > 2)
    {
      if (!*vploc)
	goto ok;
      ptr = *vploc;
      for (i=0; i<cdfp->numDims && ptr; i++)
	{
	  ptr = ptr->next;
	}
      
      /*
	Keep only the first d positions variables and move the rest to 
	data arrays.
	*/
      
      if (i != cdfp->numDims)
	DXErrorGoto(ERROR_BAD_PARAMETER,
        "Number of positions variables are not equal to number of dimensions");

      n = i;
      while( ptr )
	{
	  fvp = ptr;
	  ptr = fvp->next;
	  DXWarning("moved variable '%s' to data cdf import", fvp->name);
	  moveVar(vploc,vpdata,n);
	}
    }
  else if (cdfp->numDims > 0)
    {

      if (!*vploc)
	goto ok;
      ptr = *vploc;
      i = 0;
      while( ptr )
	{
	  if (!strcmp(ptr->name,"LATITUDE"))
	    lat = 1;
	  else if (!strcmp(ptr->name,"LONGITUD"))
	    lon = 1;
	  else if (strstr(ptr->name,"LAT"))
	    latlat = 1;
	  else if (strstr(ptr->name,"LON"))
	    lonlon = 1;
	  i++;
	  ptr = ptr->next;
	}

      if (i >= 2)
	{
	  if (lat == 1 && lon == 1)
	    {
	      n=0;
	      ptr = *vploc;
	      while( ptr )
		{
		  fvp = ptr;
		  ptr = fvp->next;

		  /*
		    If LATITUDE or LONGITUD is the variable name then
		    leave them alone otherwise move that structure
		    to vpdata.
		    */

		  if ((strcmp(fvp->name,"LATITUDE") && 
			strcmp(fvp->name,"LONGITUD"))) 
		    {
		      DXWarning("moved variable '%s' to data cdf import", 
			      fvp->name);
		      moveVar(vploc,vpdata,n);
		    }
          	  else
		    n++;  
		}
	    }
	  else if (latlat == 1 && lonlon == 1)
	    {
	      n=0;
	      ptr = *vploc;
	      while( ptr )
		{
		  fvp = ptr;
		  ptr = fvp->next;
		  
		  /* if LAT or LON are in the variable name then use it 
		   * unless already 2 position variables */
		  if ((!strstr(fvp->name,"LAT") && !strstr(fvp->name,"LON"))
		       || n>=2)
		    { 
		      DXWarning("moved variable '%s' to data cdf import",
				fvp->name);
		      moveVar(vploc,vpdata,n);
		    }
		  else
		    n++;
		}
	    }
	  else
	    {
	      i = 0;
	      ptr = *vploc;
	      while ( ptr )
		{
		  fvp = ptr;
		  ptr = fvp->next;

		  /*
		    Leave the first 2 variables and free the rest of them.
		    */
		  
		  if (i>=2)
		    {
		      DXWarning("moved variable '%s' to data cdf import", 
			      fvp->name);
		      moveVar(vploc,vpdata,i);
		    }
		  i++;
		}
	    }
	}
      else
	{
	  ptr = (*vploc)->next;
	  while ( ptr )
	    {
	      fvp = ptr;
	      ptr = fvp->next;
	      DXWarning("Ignored variable '%s' in cdf import", 
		      fvp->name);
	      DXFree((Pointer)fvp);
	    }
	}
    }

 ok:
  return OK;
  
 error:
  return ERROR;
}

/*
  Updates the structure vptime.
  */

static Error cleanSerVar(Infovar *vptime, Infocdf cdfp, Infovar *vpdata)
{
  Infovar fvp, ptr;
  int n = 0;

  ptr = *vptime;
  if (ptr == NULL)
    return OK;
  
  while(ptr && strcmp(ptr->name,"EPOCH") )
    {
      fvp = ptr;
      ptr = fvp->next;
    }
  
  /*
    ptr = NULL only if no EPOCH variable is found .
    */
  
  if (ptr == NULL)
    {
      /*
	Free all elements of link list vptime except the head.
	*/
      ptr = (*vptime)->next;
      while( ptr )
	{
	  fvp = ptr;
	  ptr = fvp->next;
	  DXWarning("Ignored variable '%s' in cdf import", fvp->name);
	  DXFree((Pointer)fvp);
	}
    }
  else
    {
      /*
	Free all elements of link list vptime except the one which has
	the name field to be EPOCH.
	*/
      ptr = *vptime;
      n = 0;
      while( ptr )
	{
	  fvp = ptr;
	  ptr = fvp->next;
	  if (strcmp(fvp->name,"EPOCH"))
	    {
	      DXWarning("variable '%s' is imported as data field", fvp->name);
	      moveVar(vptime,vpdata,n);
	    }
	  else
	    n++;
	}
    }
  return OK;
}

/*
  By checking the record and dimension variances tries to determine the
  component type of variable. The variable can be "positions" component,
  "series position" component or "data" component. 
  A variable whose value varies only along one dimension but it's value 
  is same for all records is categorized as POSITIONCOMP.
  A variable whose value varies only along records is categorized as a 
  component representing the series element value (SERPOSCOMP).
  A variable whose value varies along each dimension and record is
  categorized as DATACOMP.
  If dims is less than 1 and the variable name is LONGITUD or LATITUDE it
  is categorized as POSITIONCOMP else as DATACOMP.
 */

static int categorizeVar(long dims, Infovar vp)
{
  int i, n, category=0;

  if (dims > 0)
    {
      for(i=0, n=0; i<dims; i++)
	if( vp->dimVariances[i] == VARY )
	  ++n;
      
      if(vp->recVariance==VARY)
	{
	  if (n == 0)
	    category = SERPOSCOMP;
	  else if (n == dims)
	    category = DATACOMP;
	  else 
	    category = DATACOMP;	/* dim less than CDF dimension */
	    /* goto unknown; */ 
	}
      else if( n==1 && vp->recVariance==NOVARY )
	category = POSITIONCOMP;
      else
        category = DATACOMP;
        /*goto unknown;*/ 
    }
  else if (dims == 0)
    {
      if((!strcmp(vp->name,"LATITUDE")) || (!strcmp(vp->name,"LONGITUD")))
	category = POSITIONCOMP;
      else if (!strcmp(vp->name,"EPOCH"))
	category = SERPOSCOMP;
      else
	category = DATACOMP;
    }
  return category;

 /*
 unknown:
  DXSetError(ERROR_BAD_PARAMETER,
	   "Not able to categorize variable %s",vp->name);
  return UNKNOWNCOMP;
  */
}


/*
  Free memory of linklist vp.
  */

static 
void freeVar(Infovar vp)
{
  Infovar tptr, ptr=vp;
  
  while( ptr )
    {
      tptr = ptr;
      ptr = tptr->next;
      DXFree((Pointer)tptr);
    }
}



/*
  Add vp as head to link list vps. 
 */
static void addVar(Infovar vp, Infovar *vps)
{
  Infovar ptr;
  
  ptr = *vps;
  *vps = vp;
  (*vps)->next = ptr;
}

/* 
  Move nth Variable from vp to vp2 
*/
static void moveVar(Infovar *vp, Infovar *vp2, int n)
{
   Infovar last, ptr;
   int i;

   last = *vp;
   if (n==0)
     {
       ptr = last;
       last = last->next;
       *vp = last;
     }
   else
     {
      for (i=0; i<n-1; i++)
	last = last->next;
      ptr = last->next;
      last->next = ptr->next;
     }
   addVar(ptr,vp2);
}

/* 
  Remove nth Variable from vp
  */
static void removeVar(Infovar *vp, int n)
{
   Infovar last,ptr;
   int i;

   last = *vp;
   if (n==0){
      ptr = last;
      last = last->next;
      *vp = last;
   }
   else{
      for(i=0; i<n-1; i++)
         last = last->next;
      ptr = last->next;
      last->next = ptr->next;
   }
   DXFree(ptr);
}

/*
  Copy variable to ptr then add to vps
  */
static void copyVar(Infovar vp, Infovar *vps,int numDims)
{
   Infovar ptr;
   int i;

   /* copy vp to ptr */
   ptr = (struct infovar *)DXAllocate(sizeof(struct infovar));
   
   ptr->id = vp->id;
   ptr->varNum = vp->varNum;
   ptr->dataType = vp->dataType;
   ptr->numElements = vp->numElements;
   ptr->recVariance = vp->recVariance;
   for (i=0; i<numDims; i++)
      ptr->dimVariances[i] = vp->dimVariances[i];
   ptr->strframe = vp->strframe;
   ptr->endframe = vp->endframe;
   ptr->dltframe = vp->dltframe;
   strcpy (ptr->name,vp->name);
   ptr->next = NULL;
   addVar(ptr,vps);
}

/*
  Load structures vpdata, vploc and vptime with the right values to be 
  used by hyperget CDF calls.
 */

static Error completeVarsInfo(Infocdf cdfp, Infovar vpdata, 
			      Infovar vploc, Infovar vptime)
{
  int i, d, s, k;
  Infovar ptr;
  int maxframe;
  static char *nameConnect[] = 
    {
      "point",		/* rank 0 */
      "lines",		/* rank 1 */
      "quads",		/* rank 2 */
      "cubes",		/* rank 3 */
      "hypercubes",	/* rank 4 */
      "hyper5cubes",	/* rank 5 */
      "hyper6cubes",	/* rank 6 */
      "hyper7cubes",	/* rank 7 */
      "hyper8cubes"	/* rank 8 */
      };
  
  ptr = vpdata;
  while( ptr )
    {
      for(i=0, d=0, s=1, k=cdfp->numDims-1; i<cdfp->numDims; i++, k--)
	{
	  if(ptr->dimVariances[i] == VARY)
	    {
	      ptr->shape[d++] = cdfp->dimSizes[i];
	      ptr->cnt[i] = cdfp->dimSizes[i];
	      s *= cdfp->dimSizes[i];
	    }
	  else 
	    ptr->cnt[i] = 1;
	  ptr->idx[i] = 0;
	  ptr->intval[i] = 1;
	}
      if (cdfp->numDims == -1)
	{
	  maxframe = (ptr->endframe == -1) ? cdfp->maxRec : ptr->endframe ;
	  ptr->size = abs((maxframe - ptr->strframe)/ptr->dltframe)+1;
	  ptr->dataDims = 0;
	}
      else
	{
	  ptr->size = s;
	  ptr->dataDims = d;
	  strcpy(ptr->conn,nameConnect[ptr->dataDims]);
	}
      
      for( ; i<CDF_MAX_DIMS; i++)
	ptr->cnt[i] = ptr->idx[i] = ptr->intval[i] = 0;
      if (!cdftype2dxtype(ptr->dataType,&(ptr->dxtype)))
	goto error;
      if (ptr->dxtype == TYPE_STRING)
	ptr->numbytes = ptr->numElements;
      else
        if (!(ptr->numbytes = dxtype2bytes(ptr->dxtype)))
	  goto error;
      ptr = ptr->next;
    }

  ptr = vploc;
  while( ptr )
    {
      for(i=0; i<cdfp->numDims; i++)
	{
	  if(ptr->dimVariances[i] == VARY)
	    {
	      ptr->cnt[i] = cdfp->dimSizes[i];
	      ptr->size = cdfp->dimSizes[i];
	    }
	  else
	    ptr->cnt[i] = 1;
	  
	  ptr->intval[i] = 1;
	  ptr->idx[i] = 0;
	}
      if (cdfp->numDims == -1)
	{
	  maxframe = (ptr->endframe == -1) ? cdfp->maxRec : ptr->endframe ;
	  ptr->size = abs((maxframe - ptr->strframe)/ptr->dltframe)+1;
	  ptr->dataDims = 0;
	}
      else
	{
	  ptr->dataDims = 1;
	}
      if (!cdftype2dxtype(ptr->dataType,&(ptr->dxtype)))
	goto error;
      if (!(ptr->numbytes = dxtype2bytes(ptr->dxtype)))
	goto error;
      for( ; i<CDF_MAX_DIMS; i++)
	ptr->cnt[i] = ptr->idx[i] = ptr->intval[i] = 0;
      ptr = ptr->next;
    }

  ptr = vptime;
  while( ptr )
    {
      for (i=0; i<cdfp->numDims; i++)
	{
	  ptr->cnt[i] = 1;
	  ptr->intval[i] = 1;
	  ptr->idx[i] = 0;
	}
      ptr->dataDims = 1;
      if (!cdftype2dxtype(ptr->dataType,&(ptr->dxtype)))
	goto error;
      if (!(ptr->numbytes = dxtype2bytes(ptr->dxtype)))
	goto error;
      for( ; i<CDF_MAX_DIMS; i++)
	ptr->cnt[i] = ptr->idx[i] = ptr->intval[i] = 0;
      ptr = ptr->next;
    }

  return OK;

 error:
  return ERROR;
}



/*
  Return number of bytes of the dx data type.
  */

static int dxtype2bytes( Type dxtype )
{
  switch(dxtype )
    {
    case TYPE_BYTE:
    case TYPE_UBYTE:
      return 1;
    case TYPE_SHORT:
    case TYPE_USHORT:
      return 2;
    case TYPE_FLOAT:
    case TYPE_INT:
    case TYPE_UINT:
      return 4;
    case TYPE_DOUBLE:
      return 8;
    default:
      DXSetError(ERROR_BAD_TYPE,"Bad dx type data");
      return ERROR;
    }
}
/*
  Figure out dx data type for the given cdf type.
  If there is no equivalent dx data type for the given cdf data type
  then returns an error.
  */

static Error cdftype2dxtype( long cdftype, Type *type )
{

  switch(cdftype )
    {
    case CDF_BYTE:
    case CDF_INT1:
      *type = TYPE_BYTE;
      break;
    case CDF_UINT1:
      *type = TYPE_UBYTE;
      break;
    case CDF_INT2:
      *type = TYPE_SHORT;
      break;
    case CDF_UINT2:
      *type = TYPE_USHORT;
      break;
    case CDF_FLOAT:
    case CDF_REAL4:
      *type = TYPE_FLOAT;
      break;
    case CDF_INT4: 
      *type = TYPE_INT;
      break;
    case CDF_UINT4:
      *type = TYPE_UINT;
      break;
    case CDF_CHAR:
    case CDF_UCHAR:
      *type = TYPE_STRING;
      break;
    case CDF_REAL8:
    case CDF_DOUBLE: 
    case CDF_EPOCH:
      *type = TYPE_DOUBLE;
      break;
    default:
      DXSetError(ERROR_BAD_TYPE,"Bad CDF type data");
      return ERROR;
    }
    return OK;
}

/*
  Return the number of items in link list vp.
 */

static int 
cntVars(Infovar vp)
{
  int cnt=0;
  Infovar ptr = vp;
  
  while (ptr)
    {
      ptr = ptr->next;
      cnt++;
    }
  return cnt;
}

/* called from import.m to test if CDF file */
ImportStatReturn
_dxfstat_cdf(char *filename)
{
   Infocdf cdf; 

   if (!openCDF(filename,&cdf,0)){
     DXResetError();
     return IMPORT_STAT_NOT_FOUND;
   }

   return IMPORT_STAT_FOUND;
}

#else


Object DXImportCDF(char* filename, char **fieldlist, int *startframe,
                int *endframe, int *deltaframe)
{ DXSetError(ERROR_NOT_IMPLEMENTED, "CDF libs not included"); return ERROR; }


ImportStatReturn 
_dxfstat_cdf(char *filename)
{
    return IMPORT_STAT_NOT_FOUND;
}

#endif /* DXD_LACKS_CDF_FORMAT */
