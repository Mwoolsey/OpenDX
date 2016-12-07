/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dxmods/genimp_io.c,v 1.10 2006/01/04 22:00:51 davidt Exp $
 */

#include <dxconfig.h>


/*
 * There are four variables fields(f), series(s), grid(g) and structure(d).
 * I am representing them from slow to fastest changing variable index.
 * 
 * 1.  f, s, g, d :- vector
 * 2.  f, s, d, g :- vector-record ?
 * 3.  f, d, s, g 
 * 4.  f, d, g, s
 * 5.  f, g, s, d
 * 6.  f, g, d, s
 * 7.  s, f, g, d :- record-vector
 * 8.  s, f, d, g :- record
 * 9.  s, g, f, d :- field
 *10.  g, f, s, d
 *11.  g, s, f, d
 *12.  g, f, d, s
 */

#include <string.h>
#include "genimp.h"

static int  FgetAsciiData(FILE *, void **, int);
static int  FgetBinaryData(FILE *, void **, int);
static int  RgetAsciiData(FILE *, void **, int);
static int  RgetBinaryData(FILE *, void **, int);
static int  VgetAsciiData(FILE *, void **, int);
static int  VgetBinaryData(FILE *, void **, int);
static int  RVgetAsciiData(FILE *, void **, int);
static int  RVgetBinaryData(FILE *, void **, int);

static Error convert_string(char *s, Type t, void *data, int index, int *nl);
static Error read_ascii(FILE *fp, Type t, void *data, int index);
static Error skip_header(FILE *fp,struct header *); 
static Error skip_bdata(FILE *fp, int toskip);
static Error skip_ascii(FILE *fp, int num);
static Error doread(void *p, int size, int cnt, FILE* fp);
static Error skip_numchar( FILE *, int, char **);
static void add_parse_message(char *op, int s, int f, int item, int comp);
static void *reorderData(void *data, int f);
static Pointer locations2float(Pointer from, Type from_type, int cnt);
static Error _dxf_getline(char **, FILE *);
static Error extract_fromline(char str[MAX_DSTR],int which);

#if !defined(DXD_STANDARD_IEEE)
static void _dxfdenormalize(void **);
#endif

static int (* FgetData[])(FILE *, void **, int) = {
  FgetAsciiData,
  FgetBinaryData};
static int (* RgetData[])(FILE *, void **, int) = {
  RgetAsciiData,
  RgetBinaryData};
static int (* VgetData[])(FILE *, void **, int) = {
  VgetAsciiData,
  VgetBinaryData};
static int (* RVgetData[])(FILE *, void **, int) = {
  RVgetAsciiData,
  RVgetBinaryData};

/*
 * DXRead the data file as specified by the general import file.
 * The data for series s and field f is placed in the buffer pointed
 * to by XF[s*_dxd_gi_numflds + f].  After reading in the raw data from the
 * file, we must
 *	1) Change column majority data to row majority.
 *	2) DXSwap byte order if they differ and it was binary data.
 *	3) Denormalize floating point data for the PVS.
 * So what is a returned in each XF[i] buffer is the data for each field
 * of each requested series, in the correct majority and byte order.
 * 
 * See genimp.c:import_Group for a discussion of XF.
 */
int _dxf_gi_read_file(void **XF,FILE **fp)
{
  int size,s,nitems,i,f;

  /* 
   * DXRead the raw data into the XF[i].
   */
  if(!(_dxf_gi_importDatafile(XF,fp)))
    goto error;

  /*size = _dxfnumGridPoints();*/ 

  /* 
   * Change the majority if the input data is in column majority order. 
   */
  if ( _dxd_gi_majority == COL ) {
    for (s=i=0 ; i<_dxd_gi_series ; i++, s+=_dxd_gi_numflds) {
	for (f=0 ; f<_dxd_gi_numflds ; f++) {
	    size = _dxfnumGridPoints(f); 
	    if (XF[s+f]) {
                XF[s+f] = reorderData(XF[s+f],f);
		if (!XF[s+f])
            		goto error;
	    }
	}
    }
  }

  /* 
   * Change the byte order if necessary. 
   */
  if ((_dxd_gi_format == BINARY) && 
      (_dxfLocalByteOrder() != _dxd_gi_byteorder)) {
	for (f=0 ; f<_dxd_gi_numflds ; f++) {
	    if (_dxd_gi_whichflds[f]  == ON) {
	        size = _dxfnumGridPoints(f); 
	        nitems = size * _dxd_gi_var[f]->datadim;
	        for (s=i=0 ; i<_dxd_gi_series ; i++, s+=_dxd_gi_numflds) 
		    if ((_dxd_gi_whichser[i] == ON)  &&
	            	!_dxfByteSwap(XF[s+f], XF[s+f], 
				nitems, _dxd_gi_var[f]->datatype))
			goto error;
	    }
	}
  }

#if !defined(DXD_STANDARD_IEEE)
  /* 
   * Denormalize the IEEE floating point numbers. 
   */
   _dxfdenormalize(XF);
#endif /* !normal ieee */

  /*
   * If user has used the 'locations' keyword and the locations are 
   * not type float, then convert them to floats. 
   * Note that if we convert, then we free the old array.
   */
    if ((_dxd_gi_positions == FIELD_PRODUCT) && 
	(_dxd_gi_var[_dxd_gi_loc_index]->datatype != TYPE_FLOAT)) {
	int nfloats=0;
	Type ltype;
        size = _dxfnumGridPoints(_dxd_gi_loc_index); 
	nfloats = _dxd_gi_var[_dxd_gi_loc_index]->datadim * size;
	ltype = _dxd_gi_var[_dxd_gi_loc_index]->datatype;
	for (s=0 ; s<_dxd_gi_series ; s++) {
	    if (_dxd_gi_whichser[s] == ON) {
		Pointer new_XF;
	        i = s*_dxd_gi_numflds + _dxd_gi_loc_index;
		new_XF = locations2float( XF[i], ltype, nfloats);
		if (!new_XF) goto error;
		DXFree(XF[i]);
		XF[i] = new_XF;
       	    }
	}
	_dxd_gi_var[_dxd_gi_loc_index]->datatype = TYPE_FLOAT;
	_dxd_gi_var[_dxd_gi_loc_index]->databytes = DXTypeSize(TYPE_FLOAT);
   }

   return OK;

error:
  return ERROR;
}

/*
    FN:  open the data file 
*/
Error
_dxf_gi_OpenFile(char *name, FILE **fp, int infofile)
{
  int i;
  char *outname, *f;
  static char infopath[256];	/* Save the path when opening the info file */
 

  /* try to open the filename as given, and save the name 
   *  for error messages later.
   */
  outname = NULL;
  if (!infofile && (strlen(infopath) > 0)) {
      if (!(*fp = _dxfopen_dxfile(name,infopath,&outname,".general")))
	 goto error;
  }
  else{
      if (!(*fp = _dxfopen_dxfile(name,NULL,&outname,".general")))
	 goto error;
      /* 
       * Save the path to the info file so that we can use it to open the
       * the data file if necessary.
       */
      if (infofile) {
	if (strchr(outname,'/'))  {
	    strcpy(infopath,outname);
	    for (i=strlen(outname)-1 ; i>=0 ; i--) {
		if (infopath[i] == '/') {
		    infopath[i+1] = '\0';
		    break;	
		}
	    }
	} else
	    infopath[0] = '\0';
      } 
   }
   DXFree(outname);
   return OK;

error:
  if (outname) DXFree(outname);
  infopath[0] = '\0';
  if (infofile)
	f = "general import file";
  else
	f = "data file";
  DXSetError(ERROR_BAD_PARAMETER, "#10903", f, name);
  return ERROR;
} 


/*
   FN: According to the interleaving technique it calls different getdata
       routines.
   RT: Returns OK if succesful otherwise an ERROR.
*/

int _dxf_gi_importDatafile(void **data,FILE **fp)
{
  int size, gd=ERROR;
  
  /*size = _dxfnumGridPoints();*/
  size = _dxfnumGridPoints(-1);
  
  if(!*fp && !_dxf_gi_OpenFile(_dxd_gi_filename, fp, 0)) /* 0 = data file */
    goto error;

  if ((_dxd_gi_header.type != SKIP_NOTHING)&& !skip_header(*fp,&_dxd_gi_header))
      goto error;

  switch (_dxd_gi_interleaving) {
      case IT_RECORD_VECTOR:
       	if(!(gd = (* RVgetData[_dxd_gi_format])(*fp, data, size)))
	    goto error;
	break;
      case IT_FIELD:
        if(!(gd = (* FgetData[_dxd_gi_format])(*fp, data, size)))
	    goto error;
	break;
      case IT_RECORD:
        if (!(gd = (* RgetData[_dxd_gi_format])(*fp, data, size)))
	    goto error;
	break;
      case IT_VECTOR:
     	if (!(gd = (* VgetData[_dxd_gi_format])(*fp, data, size)))
	    goto error;
	break;
      default:
	DXErrorGoto(ERROR_INTERNAL,"Unexpected interleave");
  }
  
  return(gd);

 error:
  return ERROR;
}

/*
  FN: Field interleaving ascii data read routine.
*/

static int FgetAsciiData(FILE *fp, void **data, int size)
{
  int cnt, f, k, comp, d, dim, width;
  int step, s,nl=0,maxseries,sersize=0;
  char *str, r[MAX_DSTR], *op;
  Type t;

  size = _dxfnumGridPoints(0);
 
  /* what is the max series input */
  for (s=_dxd_gi_series-1; s>=0; s--)
     if (_dxd_gi_whichser[s] == ON) break;
  maxseries = s+1;
  /* how many numbers per series */
  for (f=0; f<_dxd_gi_numflds; f++){
     if (_dxd_gi_var[f]->datatype==TYPE_STRING) dim=1;
     else dim = _dxd_gi_var[f]->datadim;
     sersize += dim;
  }
  sersize = size * sersize;

  if ( _dxd_gi_asciiformat == FREE )
    {
      for (s=step= 0; s<maxseries; s++, step += _dxd_gi_numflds)
	{
	  for (cnt = 0; cnt < size; cnt++) 
	    {
	      for (f = 0; f < _dxd_gi_numflds; f++) 
		{
		  t = _dxd_gi_var[f]->datatype;
		  dim = _dxd_gi_var[f]->datadim;
		  if (_dxd_gi_whichflds[f] == ON && _dxd_gi_whichser[s] == ON)
		    {
		      for (comp = 0; comp < dim; comp++) 
			{
			  d = (cnt*dim) + comp;
			  if (!read_ascii(fp,t,data[step+f],d)) {
			    op = "Reading";
			    goto parse_error;
			  }
			  if (t==TYPE_STRING) comp=dim;
			}
		    }
		  else
		    {
		      for (comp = 0; comp < dim; comp++) 
			{
			  if (!skip_ascii(fp,1)) {
			    op = "Skipping";
			    goto parse_error;
			  }
			  if (t==TYPE_STRING) comp=dim;
			}
		    }
		}  
	    } 
           if ((_dxd_gi_series-1) != s && !skip_header(fp,&_dxd_gi_serseparat)){
               DXAddMessage("error reading series separator %d",s+1);
               goto error;
           }
	}
    }   
  else if ( _dxd_gi_asciiformat == FIX )
    {
      op = "Reading";
      str = (char *)DXAllocate(MAX_DSTR);
      for (s=comp=f=step=0; s < maxseries; s++, step += _dxd_gi_numflds)
	{
	  for (cnt=0; cnt < size; cnt++)
	    {
	      if (!_dxf_getline(&str, fp))
		{
		  DXSetError(ERROR_MISSING_DATA,"#10898","data file"); 
	          f=0; 
		  goto parse_error;
		}
	      if (_dxd_gi_whichser[s] == ON)
		{
	      k = 0;
	      for (f=0; f < _dxd_gi_numflds; f++)
		{
		  k += _dxd_gi_var[f]->leading;
	  	  t = _dxd_gi_var[f]->datatype;
    	  	  dim = _dxd_gi_var[f]->datadim;
	  	  width = _dxd_gi_var[f]->width;
		  if (_dxd_gi_whichflds[f] == ON)
		    {
		      for (comp = 0; comp < dim; comp++)
			{
			  d = (cnt*dim) + comp;
			  strncpy(r, &str[k], width);
			  r[width] = (char)NULL;
			  if (!convert_string(r,t,data[step+f],d,&nl))
				goto parse_error;
			  k += width;
			  if (t==TYPE_STRING) comp=dim;
			} 
		    }
		  else
		    {
		      if (t==TYPE_STRING) dim=1;   
		      k += dim*width;
		    }
		}
		}
	    }
           if ((_dxd_gi_series-1) != s && !skip_header(fp,&_dxd_gi_serseparat)){
               DXAddMessage("error reading series separator %d",s+1);
               goto error;
           }
	}
        DXFree(str);
    }
  else 
    {
      DXSetError(ERROR_INTERNAL,"Unknown ascii format"); 
      goto error;
    }
  return OK;

parse_error:
  add_parse_message(op, s,f,cnt,comp);

error:
  return ERROR;
}

/*
  FN : Field interleaving binary data read routine.
*/

static int FgetBinaryData(FILE *fp, void **data, int size)
{
  int i, j, k, len, step, d, s, type_size;
  char *p=NULL, *op,*tp;
  int maxseries;
 
  size = _dxfnumGridPoints(0);		/* all fields same size */

  for( len=j=0; j<_dxd_gi_numflds; j++)
      len += _dxd_gi_var[j]->databytes * _dxd_gi_var[j]->datadim;
 
  if (!(p = (char *)DXAllocate(len)) )
      goto error;
  
  /* what is the max series input */
  for (s=_dxd_gi_series-1; s>=0; s--)
     if (_dxd_gi_whichser[s] == ON) break;
  maxseries = s+1;
  
  for (s=step = 0; s < maxseries; s++, step += _dxd_gi_numflds)
    {
      for(i=0; i<size; i++)
	{
	  if (_dxd_gi_whichser[s] == OFF)  {
              if (!skip_bdata(fp, len*size)) {
		  op = "skip";
		  goto parse_error;
	      }
	      continue;
	  } else { 
              if (!doread(p, 1, len, fp)) {
		  op = "read";
		  goto parse_error;
	      }
	  }
          tp = p;
	  for(j=0; j<_dxd_gi_numflds; j++)
	    {
	      type_size = _dxd_gi_var[j]->databytes;
	      if (_dxd_gi_whichflds[j] == ON)
		{
		  for(k=0; k<_dxd_gi_var[j]->datadim; k++)
		    {
		      d = (i*_dxd_gi_var[j]->datadim) + k;
		      switch( _dxd_gi_var[j]->datatype )
			{
			case TYPE_UBYTE:
			  DREF(ubyte,data[step+j],d) = DREF(ubyte,tp,0);
			  break;
 			case TYPE_BYTE:
			  DREF(byte,data[step+j],d) = DREF(byte,tp,0);
			  break;
			case TYPE_SHORT:
			  DREF(short,data[step+j],d) = DREF(short,tp,0);
			  break;
			case TYPE_USHORT:
			  DREF(unsigned short,data[step+j],d) = DREF(unsigned                                  short,tp,0);
     			  break;
			case TYPE_INT:
			  DREF(int,data[step+j],d) = DREF(int,tp,0);
			  break;
 			case TYPE_UINT:
			  DREF(unsigned int,data[step+j],d) = DREF(unsigned                                   int,tp,0);
			  break;
			case TYPE_FLOAT:
			  DREF(float,data[step+j],d) = DREF(float,tp,0);
			  break;
			case TYPE_DOUBLE:
			  DREF(double,data[step+j],d) = DREF(double,tp,0);
			  break;
			default:
			  DXSetError(ERROR_INTERNAL,
				   "Unknown data type to read data file");
			  goto error;
			}
		      tp += type_size;
		    }
		}
	      else
		{	/* Skip the data for this field */
		    tp += _dxd_gi_var[j]->datadim * type_size;
		}
	    }
	}
       if ((_dxd_gi_series-1) != s && !skip_header(fp,&_dxd_gi_serseparat)){
          DXAddMessage("error reading series header %d",s+1);
          goto error;
       }
    }

  DXFree((Pointer) p );
  return OK;
  
parse_error:
  if (_dxd_gi_series > 1)
    DXAddMessage("\nTrying to %s item %d from all fields of series %d",op,s+1);
  else
    DXAddMessage("\nTrying to %s item %d from all fields",op);

 error:
  if (p) DXFree((Pointer)p);
  return ERROR;
}

/*
  FN: Record interleaving ascii format read.
*/

static int RgetAsciiData(FILE *fp, void **data, int size)
{
  int cnt=0, f, comp, d, dim, ll=0, kk, elements,leading,width;
  int step, s, nl=0,maxseries;
  char *str, r[MAX_DSTR], *op;
  Type t;
  struct header rec_sep;

  /* what is the max series input */
  for (s=_dxd_gi_series-1; s>=0; s--)
     if (_dxd_gi_whichser[s] == ON) break;
  maxseries = s+1;

  if ( _dxd_gi_asciiformat == FREE )
    {
      for (s=step = 0; s < maxseries; s++, step += _dxd_gi_numflds)
	{
	  for (f = 0; f < _dxd_gi_numflds; f++)
	    {
	      size = _dxfnumGridPoints(f);
	      dim = _dxd_gi_var[f]->datadim;
	      t = _dxd_gi_var[f]->datatype;
	      if (_dxd_gi_whichflds[f] == ON && _dxd_gi_whichser[s] == ON)
		{
		  for (comp = 0; comp < dim; comp++) 
		    {
		      d = comp;
		      for (cnt = 0; cnt < size; cnt++) 
			{
			  if (!read_ascii(fp,t,data[step+f],d))  {
		            op = "Reading";
			    goto parse_error;
			  }
			  d += dim;
			}
	                if (t == TYPE_STRING) dim=1;
                      /* skip according to record separator */
                      rec_sep = _dxd_gi_var[f]->rec_separat[comp];
                      if (!skip_header(fp,&rec_sep)){
                         op = "record separator";
                         goto parse_error;
                      }
		    }
		}
	      else
		{
		  for (comp = 0; comp < dim; comp++) 
		    {
		      if (!skip_ascii(fp,size)) 	/* read one record */
			 goto error;
                      rec_sep = _dxd_gi_var[f]->rec_separat[comp];
                      if (!skip_header(fp,&rec_sep)){
                         op = "record separator";
                         goto parse_error;
                      }
	              if (t == TYPE_STRING) dim=1;
		    }
		}
	    }
           if ((_dxd_gi_series-1) != s && !skip_header(fp,&_dxd_gi_serseparat)){
               DXAddMessage("error reading series separator %d",s+1);
               goto error;
           }
        }
    }
  else if ( _dxd_gi_asciiformat == FIX )
    {
      op = "Reading";
      str = (char *)DXAllocate(MAX_DSTR);
      for (s=step= 0; s < maxseries; s++, step += _dxd_gi_numflds)
	{
	  for (f = 0; f < _dxd_gi_numflds; f++)
	    {
	      size = _dxfnumGridPoints(f);
	      t = _dxd_gi_var[f]->datatype;
    	      dim = _dxd_gi_var[f]->datadim;
	      elements = _dxd_gi_var[f]->elements;
	      leading = _dxd_gi_var[f]->leading;
	      width = _dxd_gi_var[f]->width;
	      for (comp = 0; comp < dim; comp++) 
		{
		  d = comp;
		  for (cnt=0; cnt<size; cnt+=elements) 
		    {
		      if (!(skip_numchar(fp, leading, &str))) 
			goto parse_error;
		      kk = 0;
		      for( ll=0; ll<elements; ll++)
			{
			  if (cnt+ll < size)
			    {
			      strncpy(r, &str[kk], width);
			      r[width] = (char)NULL;
			      if (_dxd_gi_whichflds[f] == ON && 
				  _dxd_gi_whichser[s] == ON)
				{
				  if (!convert_string(r,t,data[step+f],d,&nl))
				    goto parse_error;
 				  if (nl==1){
	                            if (!(skip_numchar(fp, leading, &str)))
                                      goto parse_error;
				    kk=0;
				    nl=0;
				    ll--;
 				  }
				  else{
				     kk+=width;
				     d+=dim;
				  }
				}
			      else
			        {
			         if (r[0] == '\n'){
				   ll--;
				   kk=0;
				   if (!skip_numchar(fp,leading,&str))
				      goto parse_error;
			         }
			         else{
				   d+=dim;
			           kk+=width;
				 }
				}
			    }
			}
		    }
	            if (t == TYPE_STRING) dim=1;
                    rec_sep = _dxd_gi_var[f]->rec_separat[comp];
                    if (!skip_header(fp,&rec_sep)){
                       op = " record separator";
                       goto parse_error;
                    }
		}
	    }
          if ((_dxd_gi_series-1) != s && !skip_header(fp,&_dxd_gi_serseparat)){
            DXAddMessage("error reading series separator %d",s+1);
            goto error;
          }
        } 
        DXFree(str);
    }
  return OK;
  
parse_error:
  add_parse_message(op,s,f,cnt+ll,comp);
 error:
  return ERROR;
}

/*
  FN: Record interleaving binary read.
*/
static int RgetBinaryData(FILE *fp, void **data, int size)
{
  int i, j, k, maxlen, len, step, d, incd, s;
  char *p=NULL, *op;
  struct header rec_sep;
  int maxseries;
  
  for(maxlen=j=0; j<_dxd_gi_numflds; j++) {
	  len = size * _dxd_gi_var[j]->datadim * _dxd_gi_var[j]->databytes;
	  maxlen =  (len > maxlen ? len : maxlen);
  }
  if (!(p = (char *)DXAllocate(maxlen)))
      goto error;

  /* what is the max series input */
  for (s=_dxd_gi_series-1; s>=0; s--)
     if (_dxd_gi_whichser[s] == ON) break;
  maxseries = s+1;
  
  for (s=step=0; s < maxseries; s++, step += _dxd_gi_numflds)
    {
      for(j=0; j<_dxd_gi_numflds; j++) {
	  size = _dxfnumGridPoints(j);
          incd = _dxd_gi_var[j]->datadim;
          for(k=0; k<_dxd_gi_var[j]->datadim; k++) {
	     d = k;
 	     len = size * _dxd_gi_var[j]->databytes;
             if (_dxd_gi_whichser[s]==OFF || _dxd_gi_whichflds[j]==OFF){
                  if(!(skip_bdata(fp, len))) {
                     op = "skip";
                     goto parse_error;
                  }
             }
             else {
                  if (!doread(p, 1, len, fp)) {
                      op = "read";
                      goto parse_error;
                  }
		  switch( _dxd_gi_var[j]->datatype ) {
		    case TYPE_UBYTE:
		      for(i=0; i<size; i++) {
			  DREF(ubyte,data[step+j],d) = DREF(ubyte,p, i);
			  d += incd;
		      }
      		      break;
		    case TYPE_BYTE:
		      for(i=0; i<size; i++) {
			  DREF(byte,data[step+j],d) = DREF(byte,p, i);
			  d += incd;
		      }
		      break;
		    case TYPE_SHORT:
		      for(i=0; i<size; i++) {
			  DREF(short,data[step+j],d) = DREF(short,p,i);
			  d += incd;
		      }
 		      break;
		    case TYPE_USHORT:
		      for(i=0; i<size; i++) {
	                  DREF(unsigned short,data[step+j],d) = DREF(unsigned                                  short,p,i);
			  d += incd;
		      }
		      break;
		    case TYPE_INT:
		      for(i=0; i<size; i++) {
			  DREF(int,data[step+j],d) = DREF(int,p,i);
			  d += incd;
		      }
		      break;
		    case TYPE_UINT:
		      for(i=0; i<size; i++) {
			  DREF(unsigned int,data[step+j],d) = DREF(unsigned                                   int,p,i);
			  d += incd;
		      }
		      break;
		    case TYPE_FLOAT:
		      for(i=0; i<size; i++) {
			  DREF(float,data[step+j],d) = DREF(float,p,i);
			  d += incd;
		      }
		      break;
		    case TYPE_DOUBLE:
		      for(i=0; i<size; i++) {
			  DREF(double,data[step+j],d)=DREF(double,p,i);
			  d += incd;
		      }
		      break;
		    default:
		      DXSetError(ERROR_INTERNAL,
			       "Unknown data type to read data file");
		      goto error;
		    }
                 }   
                 rec_sep = _dxd_gi_var[j]->rec_separat[k];
                 if (!skip_header(fp,&rec_sep)){
                    op = " record separator";
                    goto parse_error;
                 }
           } 
	}
      if ((_dxd_gi_series-1) != s && !skip_header(fp,&_dxd_gi_serseparat)){
          DXAddMessage("error reading series separator %d",s+1);
          goto error;
      }
    }

  DXFree((Pointer) p );
  return OK;
  
parse_error:
  if (_dxd_gi_series > 1)
    DXAddMessage("\nTrying to %s field '%s' of series %d",op,_dxd_gi_var[j]->name,s+1);
  else
    DXAddMessage("\nTrying to %s field '%s'",op,_dxd_gi_var[j]->name);
error:
  if (p) DXFree((Pointer)p);
  return ERROR;
}

/*
  FN: Vector interleaving ascii file format read.
*/

static int VgetAsciiData(FILE *fp, void **data, int size)
{
  int s, f, l=0, ll, kk, dim, elements, leading, width;
  int step, dim_mul_size,nl=0; 
  char *str, r[MAX_DSTR], *op;
  Type t;
  struct header rec_sep;
  int offset = 1, d;

  if ( _dxd_gi_asciiformat == FREE )
    {
      for (f = 0; f < _dxd_gi_numflds; f++)
	{
	  size = _dxfnumGridPoints(f);
	  t = _dxd_gi_var[f]->datatype;
    	  dim = _dxd_gi_var[f]->datadim;
	  offset = 1;
	  if (t == TYPE_STRING) {
	     offset = dim;
	     dim = 1;
	  }
	  for (s = 0; s < _dxd_gi_series; s++)
	    {
	      if (_dxd_gi_whichflds[f] == ON && _dxd_gi_whichser[s] == ON)
		{
		  step = s*_dxd_gi_numflds;
		  for (l = 0, d = 0; l < dim*size; l++) {
		    if (!read_ascii(fp,t,data[step+f],d)) {
      			op = "Reading";
		        goto parse_error;
		    }
		    d += offset;
		  }
		}
	      else
		{
		   if (!skip_ascii(fp,size*dim)) {
      		      op = "Skipping";
		      goto parse_error;
		   }
		}
               if ((_dxd_gi_series-1) != s && !skip_header(fp,&_dxd_gi_serseparat)){
                  DXAddMessage("error reading series separator %d",s+1);
                  goto error;
               }
	    }
                    rec_sep = _dxd_gi_var[f]->rec_separat[0];
                    if (!skip_header(fp,&rec_sep)){
                       op = " record separator";
                       goto parse_error;
                    }
	}
    }
  else if ( _dxd_gi_asciiformat == FIX )
    {
      op = "Reading";
      str = (char *)DXAllocate(MAX_DSTR);
      for (f = 0; f < _dxd_gi_numflds; f++)
	{
	  size = _dxfnumGridPoints(f);
	  t = _dxd_gi_var[f]->datatype;
    	  dim = _dxd_gi_var[f]->datadim;
	  elements = _dxd_gi_var[f]->elements;
	  leading = _dxd_gi_var[f]->leading;
	  width = _dxd_gi_var[f]->width;
	  offset = 1;
	  if (t == TYPE_STRING) {
	     offset = dim;
	     dim=1;
	  }
	  dim_mul_size = dim*size;
	  for (s = 0; s < _dxd_gi_series; s++)
	    {
	      step = s*_dxd_gi_numflds;
	      for (l = 0, d = 0; l < dim_mul_size; l+=elements) 
		{
		  if (!(skip_numchar(fp, leading, &str)))
		    goto parse_error;
		  kk = 0;
		  for( ll=0; ll<elements; ll++)
		    {
		      if ( l+ll < dim_mul_size )
			{
			  if (_dxd_gi_whichflds[f] == ON && _dxd_gi_whichser[s] == ON)
			    {
			      strncpy(r, &str[kk], width);
			      r[width] = (char)NULL;
			      if (!convert_string(r,t,data[step+f],d,&nl))
				goto parse_error;
			      if (nl==1){
				nl=0;
				kk=0;
				ll--;
                                if (!(skip_numchar(fp, leading, &str)))
                                  goto parse_error;
			      }
			      else{
				d += offset;
				kk+=width;
			      }
			    }
			  else
			    {
			      if (r[0] == '\n'){
				 ll--;
				 kk=0;
				 if (!skip_numchar(fp,leading,&str))
				    goto parse_error;
			      }
			      else{
				 d += offset;
			         kk += width;
			      }
			    }
			}
		    }
		}
                if ((_dxd_gi_series-1) != s && !skip_header(fp,&_dxd_gi_serseparat)){
                  DXAddMessage("error reading series separator %d",s+1);
                  goto error;
                }
	    }
                    rec_sep = _dxd_gi_var[f]->rec_separat[0];
                    if (!skip_header(fp,&rec_sep)){
                       op = " record separator";
                       goto parse_error;
                    }
	}
        DXFree(str);
    }
  return OK;
  
parse_error:
  add_parse_message(op,s,f,l/dim,l%dim);
 error:
  return ERROR;
}

/*
  FN: Vector binary file format read.
*/

static int VgetBinaryData(FILE *fp, void **data, int size)
{
  int j, s, len, step;
  char	*op;
  struct header rec_sep;
 
  for(j=0; j<_dxd_gi_numflds; j++)
    {
      size = _dxfnumGridPoints(j);
      for (s = 0; s < _dxd_gi_series; s++)
	{
	  step = s*_dxd_gi_numflds;
	  len = _dxd_gi_var[j]->databytes * _dxd_gi_var[j]->datadim * size;
	  if (_dxd_gi_whichser[s] == OFF || _dxd_gi_whichflds[j] == OFF)
	    {
	      if(!(skip_bdata(fp, len))) {
 	        op = "read";
		goto error;
  	      }
	    }
	  else if(!doread(data[step+j], 1, len, fp)) {
 	    op = "read";
	    goto error;
	  }
          if ((_dxd_gi_series-1) != s && !skip_header(fp,&_dxd_gi_serseparat)){
             op = "read series header";
             goto error;
          }
	}
                    rec_sep = _dxd_gi_var[j]->rec_separat[0];
                    if (!skip_header(fp,&rec_sep)){
                       op = " record separator";
                       goto error;
                    }
    }
  return OK;

error:
  if (_dxd_gi_series > 1)
    DXAddMessage("\nTrying to %s field '%s' of series %d",op,_dxd_gi_var[j]->name,s+1);
  else
    DXAddMessage("\nTrying to %s field '%s'",op,_dxd_gi_var[j]->name);
  return ERROR;
}

/*
  FN: Record Vector ascii file format read.
*/

static int RVgetAsciiData(FILE *fp, void **data, int size)
{
  int f, cnt=0, ll=0, kk, dim,elements,leading,width;
  int step=0, s=0, dim_mul_size,nl=0,maxseries;
  char *str, r[MAX_DSTR], *op;
  Type t;
  struct header rec_sep;
  int d, offset = 1;

  /* what is the max series input */
  for (s=_dxd_gi_series-1; s>=0; s--)
     if (_dxd_gi_whichser[s] == ON) break;
  maxseries = s+1;

  if ( _dxd_gi_asciiformat == FREE )
    {
      for (s = step = 0; s < maxseries; s++, step += _dxd_gi_numflds)
	{
	  for (f = 0; f < _dxd_gi_numflds; f++)
	    {
	      size = _dxfnumGridPoints(f);
	      dim = _dxd_gi_var[f]->datadim;
	      t = _dxd_gi_var[f]->datatype;
	      offset = 1;
	      if (t == TYPE_STRING) {
		 offset = dim;
		 dim=1;
	      }
	      if (_dxd_gi_whichflds[f] == ON && _dxd_gi_whichser[s] == ON)
		{
		  for (cnt = 0, d = 0; cnt < dim*size; cnt++) 
		    {
		      if (!read_ascii(fp,t,data[step+f],d)) {
			op = "Reading";
			goto parse_error;
		      }
		      d += offset;
		    }
		}
	      else
		{
		  if (!skip_ascii(fp,dim*size)) {
		    op = "Skipping";
		    goto parse_error;
		  }
		}
                rec_sep = _dxd_gi_var[f]->rec_separat[0];
                if (!skip_header(fp,&rec_sep)){
                   DXAddMessage("error reading record separator");
                   goto error;
                }
	    }
          if ((_dxd_gi_series-1) != s && !skip_header(fp,&_dxd_gi_serseparat)){
             DXAddMessage("error reading series separator %d",s+1);
             goto error;
          }
	}
    }
  else if ( _dxd_gi_asciiformat == FIX )
    {
      op = "Reading";
      str = (char *)DXAllocate(MAX_DSTR);
      for (s=step = 0; s < maxseries; s++, step += _dxd_gi_numflds)
	{
	  for (f = 0; f < _dxd_gi_numflds; f++)
	    {
	      size = _dxfnumGridPoints(f);
	      dim = _dxd_gi_var[f]->datadim;
	      t = _dxd_gi_var[f]->datatype;
	      elements = _dxd_gi_var[f]->elements;
	      leading = _dxd_gi_var[f]->leading;
	      width = _dxd_gi_var[f]->width;
	      offset = 1;
	      if (t == TYPE_STRING) {
		 offset = dim;
		 dim=1;
	      }
	      dim_mul_size = dim*size;
	      for (cnt = 0, d = 0; cnt < dim_mul_size; cnt+=elements) 
		{
		  if (!(skip_numchar(fp, leading, &str)))
		    goto parse_error;
		  kk = 0;
		  for( ll=0; ll<elements; ll++)
		    {
		      if ( cnt+ll < dim_mul_size )
			{
			  strncpy(r, &str[kk], width);
			  r[width] = (char)NULL;
			  if (_dxd_gi_whichflds[f] == ON && _dxd_gi_whichser[s]==ON)
			    {
			      if (!convert_string(r,t,data[step+f],d,&nl))
				goto parse_error;
			      if (nl==1){
			        ll--;
				nl=0;
				kk=0;
                                if (!(skip_numchar(fp, leading, &str)))
                                  goto parse_error;
			      }
			      else{
				 d += offset;
			         kk+=width;
			      }
			    }
			  else
			    {
			      if (r[0] == '\n'){
				 ll--;
				 kk=0;
				 if (!skip_numchar(fp,leading,&str))
				    goto parse_error;
			      }
			      else{
				 d += offset;
			         kk+=width;
			      }
			    }
			}
		    }
		}
                rec_sep = _dxd_gi_var[f]->rec_separat[0];
                if (!skip_header(fp,&rec_sep)){
                   DXAddMessage("error reading record separator");
                   goto error;
                }
	    }
           if ((_dxd_gi_series-1) != s && !skip_header(fp,&_dxd_gi_serseparat)){
               DXAddMessage("error reading series separator %d",s+1);
               goto error;
           }
	}
        DXFree(str);
    }
  return OK;
  
parse_error:
  add_parse_message(op, s,f,(cnt+ll)/dim,(cnt+ll)%dim);
error:
  return ERROR;
}

/*
  FN: Record Vector binary file format read.
*/

static int RVgetBinaryData(FILE *fp, void **data, int size)
{
  int j, len, step, s;
  char *op;
  struct header rec_sep;
  int maxseries;

  /* what is the max series input */
  for (s=_dxd_gi_series-1; s>=0; s--)
     if (_dxd_gi_whichser[s] == ON) break;
  maxseries = s+1;
  
  for (s=step= 0; s<maxseries; s++, step += _dxd_gi_numflds)
    {
      for(j=0; j<_dxd_gi_numflds; j++)
	{
	  size = _dxfnumGridPoints(j);
	  len = _dxd_gi_var[j]->databytes * _dxd_gi_var[j]->datadim * size;
	  if (_dxd_gi_whichser[s] == OFF || _dxd_gi_whichflds[j] == OFF)
	    {
	      if(!(skip_bdata(fp, len))) {
		op = "skip";
		goto error;
	      }
	    }
	  else if(!doread(data[step+j], 1, len, fp)) {
 	    op = "read";
	    goto error;
	  }
          rec_sep = _dxd_gi_var[j]->rec_separat[0];
          if (!skip_header(fp,&rec_sep)){
             op = " read record separator";
             goto error;
          }
	}
      if ((_dxd_gi_series-1) != s && !skip_header(fp,&_dxd_gi_serseparat)){
         op = "read series separator";
         goto error;
      }
    }
  return OK;

 error:
  if (_dxd_gi_series > 1)
    DXAddMessage("\nTrying to %s field '%s' of series %d",op,_dxd_gi_var[j]->name,s+1);
  else
    DXAddMessage("\nTrying to %s field '%s'",op,_dxd_gi_var[j]->name);
  return ERROR;
}

/*
 * DXRead a value of type 't' from stream fp and put it in the index'th
 * array position relative to data for the given type. 
 * 
 */
static Error
read_ascii(FILE *fp, Type t, void *data, int index)
{
    int r, i;
    char string[80];

    switch (t) {
    case TYPE_BYTE:
	r = fscanf(fp,"%d",&i);
	if (i<DXD_MIN_BYTE || i>DXD_MAX_BYTE) {
	    DXSetError(ERROR_DATA_INVALID,"#10905",i,"byte"); 
	    goto error;
	}
	DREF(byte,data,index) = (byte)i;
	break;
    case TYPE_UBYTE:
	r = fscanf(fp,"%d",&i);
	if (i<0 || i>DXD_MAX_UBYTE) {
	    DXSetError(ERROR_DATA_INVALID,"#10905",i,"ubyte"); 
	    goto error;
	}
	DREF(ubyte,data,index) = (ubyte)i;
	break;
    case TYPE_SHORT:
	r = fscanf(fp,"%d",&i);
	if (i<DXD_MIN_SHORT || i>DXD_MAX_SHORT) {
	    DXSetError(ERROR_DATA_INVALID,"#10905",i,"short");
	    goto error;
	}
	DREF(short,data,index) = (short)i;
	break;
    case TYPE_USHORT:
	r = fscanf(fp,"%d",&i);
	if (i<0 || i>DXD_MAX_USHORT) {
	    DXSetError(ERROR_DATA_INVALID,"#10905",i,"ushort");
	    goto error;
	}
	DREF(unsigned short,data,index) = (unsigned short)i;
	break;
    case TYPE_INT:
	r = fscanf(fp,"%d",&DREF(int,data,index));
	break;
    case TYPE_UINT:
  	r = fscanf(fp,"%u",&DREF(unsigned int,data,index));
	break;
    case TYPE_FLOAT:
	r = fscanf(fp,"%f",&DREF(float,data,index));
	break;
    case TYPE_DOUBLE:
	r = fscanf(fp,"%lf",&DREF(double,data,index));
	break;
    case TYPE_STRING:
	r = fscanf(fp,"%s",&DREF(char,data,index));
	break;
    default:
	DXErrorGoto(ERROR_INTERNAL, 
		"Type not implemented for ascii files");
    }

    if (r == 0){
		fscanf(fp,"%70s",string);
		DXSetError(ERROR_DATA_INVALID, "#10906",string);
        goto error;
    }

    if (r == EOF){
	DXSetError(ERROR_MISSING_DATA, "#10898",_dxd_gi_filename);
        goto error;
    }

    return OK;
error:
    return ERROR;

}

/* skip n numbers */
static Error
skip_ascii(FILE *fp, int num)
{
   int c,prev,curr,found;

   c = getc(fp);
   if (c == ' ' || c == '\n' || c == '\t'){
      prev = 0;
      found = 0;
   }
   else {
      prev = 1;
      found = 1;
   }
   while (1) {
      c = getc(fp);
      if (c == ' ' || c == '\n' || c == '\t') curr = 0;
      else curr = 1;
      if (prev != curr) found++;
      if (found == 2*num) break;
      prev=curr;
   }
   /* put back last whitespace */
   ungetc(c,fp);
   return OK;
}

/*
 * Convert the string of characters to a numeric value of type 't' 
 * and put it in the index'th array position relative to data for the 
 * given type. 
 */
static Error
convert_string(char *s, Type t, void *data, int index,int *nl)
{
    int i;
    float f;
    double d;
	char format[5];

    /* if newline return nl=1 */
    if (s[0]=='\n') {*nl=1; return OK;}
 
    switch (t) {
    case TYPE_BYTE:
        if (sscanf(s,"%d",&i) <= 0) {
                DXSetError(ERROR_DATA_INVALID,"#10907","byte",s);
                goto error;
        }
        if (i<DXD_MIN_BYTE || i>DXD_MAX_BYTE) {
            DXSetError(ERROR_DATA_INVALID,"#10905",i,"byte");
            goto error;
        }
        DREF(byte,data,index) = (byte)i;
        break;
    case TYPE_UBYTE:
	if (sscanf(s,"%d",&i) <= 0) {
		DXSetError(ERROR_DATA_INVALID,"#10907","ubyte",s);
		goto error;
	}
	if (i<0 || i>DXD_MAX_UBYTE) {
	    DXSetError(ERROR_DATA_INVALID,"#10905",i,"ubyte");
	    goto error;
	}
	DREF(ubyte,data,index) = (ubyte)i;
	break;
    case TYPE_SHORT:
        if (sscanf(s,"%d",&i) <= 0) {
                DXSetError(ERROR_DATA_INVALID,"#10907","short",s);
                goto error;
        }
        if (i<DXD_MIN_SHORT || i>DXD_MAX_SHORT) {
            DXSetError(ERROR_DATA_INVALID,"#10905",i,"short");
            goto error;
        }
        DREF(short,data,index) = (short)i;
        break;
    case TYPE_USHORT:
	if (sscanf(s,"%d",&i) <= 0) {
		DXSetError(ERROR_DATA_INVALID,"#10907","ushort",s);
		goto error;
	}
	if (i<0 || i>DXD_MAX_USHORT) {
	    DXSetError(ERROR_DATA_INVALID,"#10905",i,"ushort");
	    goto error;
	}
	DREF(unsigned short,data,index) = (unsigned short)i;
	break;
    case TYPE_INT:
	if (sscanf(s,"%d",&i) <= 0) {
		DXSetError(ERROR_DATA_INVALID,"#10907","int",s);
		goto error;
	}
	DREF(int,data,index) = i; 
	break;
    case TYPE_UINT:
	if (sscanf(s,"%u",&i) <= 0) {
		DXSetError(ERROR_DATA_INVALID,"#10907","uint",s);
		goto error;
	}
	DREF(unsigned int,data,index) = i; 
	break;
    case TYPE_FLOAT:
	if (sscanf(s,"%f",&f) <= 0) {
		DXSetError(ERROR_DATA_INVALID,"#10907","float",s);
		goto error;
	}
	DREF(float,data,index) = f; 
	break;
    case TYPE_DOUBLE:
	if (sscanf(s,"%lf",&d) <= 0) {
		DXSetError(ERROR_DATA_INVALID,"#10907","double",s);
		goto error;
	}
	DREF(double,data,index) = d; 
	break;
    case TYPE_STRING:
	i = strlen(s)+1;
	if (s[i-2] == '\n') 
	   s[i-2] = (char)NULL;
	i = strlen(s) + 1;
	sprintf(format,"%%%dc",i);
	if (sscanf(s,format,&DREF(char,data,index)) <=0){
		DXSetError(ERROR_DATA_INVALID,"#10907","string",s);
		goto error;
	}
	break;
    default:
	DXErrorGoto(ERROR_INTERNAL, "Unexpected type for ascii files");
    }

    return OK;
error:
    return ERROR;

}

/*
 * Skip the header, which can be denoted by either a number of lines
 * or bytes, or a marker string.
 * Returns OK/ERROR, and sets error code on ERROR.
 */
static Error 
skip_header(FILE *fp, struct header *head) 
{
  int i, c;
  char s[MAX_DSTR];


    switch (head->type) {
    case SKIP_MARKER:
        for (i=0;;) {
        c = getc(fp);
        if (c == EOF)  {
            DXSetError(ERROR_DATA_INVALID,"#10898",
		"before encountering header marker");
            goto error;
        } else if ((char)c == head->marker[i])  {
            if (head->marker[++i] == '\0')
                break;
        } else
            i=0;
        }
        break;
    case SKIP_LINES:
        for (i=0; i<head->size; )  {
        if (!fgets(s, MAX_DSTR, fp)){
            DXSetError(ERROR_MISSING_DATA,"#10910","lines");
            goto error;
        }
        if (s[strlen(s)-1] == '\n')
            i++;
        }
        break;
    case SKIP_BYTES:
        if (fseek(fp, head->size, 1) != 0){
          DXSetError(ERROR_MISSING_DATA,"#10910","byte");
          goto error;
        }
        break;
    case SKIP_NOTHING:
        break;
    default:
        DXSetError(ERROR_INTERNAL,"#10910","all");
        goto error;
    }

    return OK;

 error:
  return ERROR;
}

/*
 * Skip some bytes in a file.
 * Returns OK/ERROR, and sets error code on ERROR.
 */
static Error 
skip_bdata(FILE *fp, int toskip)
{

  if (_dxd_gi_format == BINARY && toskip != 0) {
      if (fseek(fp, toskip, 1) != 0) {
          DXSetError(ERROR_MISSING_DATA,"#10911",
			ftell(fp) + toskip);
	  goto error;
	}
    }

  return OK;

 error:
  return ERROR;
}
/*
 * Do fread()s until we are sure we either have all the data or
 * we hit an end of file.
 */
static Error
doread(void *p, int size, int cnt, FILE* fp)
{
    int left,n;

    for (left=cnt*size; left>0 ; ) {
	  n = fread((char*)p, 1, left, fp);
	  if (n == 0) {
	      DXSetError(ERROR_MISSING_DATA,"#10898","data file");
	      goto error;
	  }
	  left -= n;
	  p = (char*)p + n;
    } 

    return OK;

error:
    return ERROR;
}

/*
 * Skip number of characters in a file from the point defined by fp.
 * Returns OK/ERROR and sets error code on ERROR.
 */
static Error 
skip_numchar(FILE *fp, int num_char, char **ret_str) 
{
  int i, c;

  for (i=0; i<num_char; i++) 
    {
      c = getc(fp);
      if (c == EOF)  
	{
	  DXSetError(ERROR_DATA_INVALID,"#10898",
	   "skipping beginning of line characters");
          goto error;
        } 
      else if (c == '\n') 
	{
	  DXSetError(ERROR_DATA_INVALID,"#10899",
		"skipping beginning of line characters");
          goto error;
 	}
    }
  /* if (!fgets(ret_str, MAX_DSTR, fp)) */
  if (!_dxf_getline(ret_str,fp))
     goto error;
  return OK;
  
 error:
  return ERROR;
}

/* getline allocating space as you go 
 */
static
Error _dxf_getline(char **ret_str,FILE *fp)
{
   char str[MAX_DSTR];
   int n=0;
   char *line = *ret_str;

   /* get newline allocating space if ness */
   if ((fgets(str,MAX_DSTR,fp))==NULL){
     DXSetError(ERROR_DATA_INVALID,"#10898",_dxd_gi_filename);
     return ERROR;
   }
   strcpy(line,str);
   while ((int)strlen(str)>MAX_DSTR-2){
      if ((fgets(str,MAX_DSTR,fp))==NULL){
        DXSetError(ERROR_DATA_INVALID,"#10898","data file");
        return ERROR;
      }
      n++;
      line = (char *)DXReAllocate(line,(unsigned int)strlen(str)+n*MAX_DSTR);
      strcat(line,str);
   }
   *ret_str = line;
   return OK;
}

/* search for header info (keeping track of lines and bytes read)
 * and when found place into data
 */
Error
_dxf_gi_extract(struct place_state *dataps,int *read_header)
{
   int i,k;
   int num_sets=0,sets_read=0;
   char str[MAX_DSTR], *pmark;
   int read[MAX_POS_DIMS*3];

   /* open the data file if not already open */
   if(!dataps->fp && !_dxf_gi_OpenFile(_dxd_gi_filename, &dataps->fp, 0))
      return ERROR;
   
   /* how many offset are there? */
   for (i=0; _dxd_gi_fromfile[i]; i++){
      if (_dxd_gi_fromfile[i]->begin.type==SKIP_NOTHING){
	 sets_read++;
	 read[i]=1;
      }
      else
         read[i]=0;
   }
   num_sets=i;

   /* read one line at a time searching for any header */
   while (fgets(str, MAX_DSTR, dataps->fp) != NULL){
      /* are any header lines and if so are the the current line */
      for (i=0; i<num_sets; i++){
         if (_dxd_gi_fromfile[i]->begin.type==SKIP_LINES && 
	    dataps->lines == _dxd_gi_fromfile[i]->begin.size){
	    if (!extract_fromline(str,i))
	       goto error;
	    sets_read++;
	    read[i]=1;
	 }
	 else if (_dxd_gi_fromfile[i]->begin.type==SKIP_BYTES &&
		 dataps->bytes <= _dxd_gi_fromfile[i]->begin.size &&
		 (dataps->bytes+strlen(str))>_dxd_gi_fromfile[i]->begin.size){
		 k = _dxd_gi_fromfile[i]->begin.size - dataps->bytes;
	    if (!extract_fromline(&str[k],i))
	       goto error;
	    sets_read++;
	    read[i]=1;
	 }
	 else if (_dxd_gi_fromfile[i]->begin.type==SKIP_MARKER &&
		 (pmark = strstr(str,_dxd_gi_fromfile[i]->begin.marker))!=NULL){
	    if (read[i] == 1){	/* already hit this marker once */
	      DXWarning("the marker \"%s\" is defined more than one place in this file, check your results",_dxd_gi_fromfile[i]->begin.marker);
	      continue;
	    }
	    for( k=0; k<strlen(_dxd_gi_fromfile[i]->begin.marker);k++)
	       pmark++;
	    if (!extract_fromline(pmark,i))
	       goto error;
	    sets_read++;
	    read[i]=1;
	 }
      }
      dataps->lines++;
      dataps->bytes += strlen(str);
      /* search for header marker if exists and if found return flag */
      if (_dxd_gi_header.type==SKIP_MARKER && 
	  strstr(str,_dxd_gi_header.marker)){
	 if (sets_read == num_sets && 
	      _dxd_gi_header.marker[strlen(_dxd_gi_header.marker-1)]=='\n')
	    _dxd_gi_header.type = SKIP_NOTHING;
	 else
	    *read_header=1;
      }
      if (sets_read == num_sets) break;
   }

   /* all offsets were found? */
   if (sets_read < num_sets){
      for (i=0; i<num_sets; i++){
	 if (read[i]==0){
	    DXSetError(ERROR_DATA_INVALID,"offset for field %d not found",i);
	    goto error;
	 }
      }
   }

   return OK;

error:
   return ERROR;
}

/* read data from a line */
static Error
extract_fromline(char str[MAX_DSTR],int which)
{
   int i,j,k,index,width,nl=0;
   Type t;
   char r[MAX_DSTR];

   i = which;
         t = _dxd_gi_fromfile[i]->type;
         j=0;
         k=0;
	 index=0;
         while (_dxd_gi_fromfile[i]->skip[j] >= 0){
            k += _dxd_gi_fromfile[i]->skip[j];
            width = _dxd_gi_fromfile[i]->width[j];
            strncpy(r, &str[k], width);
            r[width] = (char)NULL;
	    if (!convert_string(r,t,_dxd_gi_fromfile[i]->data,index,&nl))
	       goto error;
	    if (nl){
	       DXSetError(ERROR_DATA_INVALID,"error reading values");
	       goto error;
	    }
            index++;
	    j++;
	    k += width;
         }
	 if (j==0){	/* not skip info read all values on this line */
	    index=0;
	    for (k=0; k<strlen(str); ){
	       while(str[k]==' ' || str[k]=='\t')	/* skip whitespace */
	          k++;
	       if (k>=strlen(str)) break; 
	       if (isdigit(str[k]) || str[k]=='-' || str[k]=='.'){
	          strcpy(r,&str[k]);
	          if (!convert_string(r,t,_dxd_gi_fromfile[i]->data,index,&nl))
	             goto error;
	          if (nl){
	             DXSetError(ERROR_DATA_INVALID,"error reading values");
	             goto error;
	          }
	          _dxd_gi_fromfile[i]->skip[index] = 0;  /*set skip to indicate number*/
	          index++;
	          while(str[k]!=' ' && str[k]!='\t') /* skip number */
	             k++;
	       }
	       else
		  break;
	    }
	    _dxd_gi_fromfile[i]->skip[index]=-1;
	 }
   return OK;

error:
   return ERROR;
}


/*
 * DXAdd a message to the current error message indicating the give
 * series number (s), field name (from f), item number in field (item)
 * and the component of the given item (comp).
 */
static void 
add_parse_message(char *op, int s, int f, int item, int comp) 
{

  item++;		/* Change to a base 1 */

  if (_dxd_gi_series > 1) {
      s++;
      if (_dxd_gi_var[f]->datadim > 1)
          DXAddMessage("\n%s Series %d, Field '%s', Datum %d, Component %d",
				op, s,_dxd_gi_var[f]->name,item,comp+1); 
      else
          DXAddMessage("\n%s Series %d, Field '%s', Datum %d", 
				op, s,_dxd_gi_var[f]->name,item); 
  } else {
     if (_dxd_gi_var[f]->datadim > 1)
          DXAddMessage("\n%s Field '%s', Datum %d, Component %d",
			op, _dxd_gi_var[f]->name,item,comp+1); 
     else
          DXAddMessage("\n%s Field '%s', Datum %d",op,_dxd_gi_var[f]->name,item); 
  }
}

/*
   FN: reorder data arg1 from row majority to column majority.
   EF: arg1 pointer points to a set of reordered data and original space has
       been freed. NULL on error and sets error code.
   Note: handle only 2, 3 and 4 dimensional data.
*/

static void*
reorderData(void *data, int f)
/*reorderData(void *data, int ndatum, int rank, Type type) */
{
  void *d1 = NULL;
  int  xskip, k, x, y, z, v, offset, data_size;
  int dim0,dim1,dim2,dim3,adjust,rank,ndatum;
  Type type;
 
  ndatum = _dxfnumGridPoints(f);
  type = _dxd_gi_var[f]->datatype;
  rank = _dxd_gi_var[f]->datadim;
  if (_dxd_gi_var[f]->datadep==DEP_CONNECTIONS) adjust=1;
  else adjust=0;

  if ( _dxd_gi_numdims == 1 )
      return data;
  
  data_size = DXTypeSize(type) * rank;
  dim0 = _dxd_gi_dimsize[0]-adjust;
  dim1 = _dxd_gi_dimsize[1]-adjust;
 
  if (!(d1 = (void *)DXAllocate(ndatum * data_size)))
      goto error;
  
  k = 0;

  if (_dxd_gi_numdims == 2)
    {
      xskip = dim1 * data_size;
      for (y = 0; y < dim1; y++)
        {
          offset = y * data_size;
          for (x = 0; x < dim0; x++)
            {
              memcpy((char*)d1 + offset, (char*)data + k, data_size);
              k += data_size;
              offset += xskip;
            }
        }
    }
  else if (_dxd_gi_numdims == 3)
    {
      dim2 = _dxd_gi_dimsize[2] - adjust;
      for (z = 0; z < dim2; z++)
	{
	  for (y = 0; y < dim1; y++)
	    {
	      for (x = 0; x < dim0; x++)
		{ 
		  offset = (x*dim1*dim2)+(y*dim2)+z;
		  offset *= data_size;
	          memcpy((char*)d1 + offset, (char*)data + k, data_size);
	          k += data_size;
		}
	    }
	}
    }
  else if (_dxd_gi_numdims == 4)
    {
      dim2 = _dxd_gi_dimsize[2] - adjust;
      dim3 = _dxd_gi_dimsize[3] - adjust;
      for (v = 0; v < dim3; v++)
	{
	  for (z = 0; z < dim2; z++)
	    {
	      for (y = 0; y < dim1; y++)
		{
		  for (x = 0; x < dim0; x++)
		    { 
		      offset = (x*dim1*dim2*dim3) +
			(y*dim2*dim3) + (z*dim3) + v;
		      offset *= data_size;
	              memcpy((char*)d1 + offset, (char*)data + k, data_size);
	              k += data_size;
		    }
		}
	    }
	}
    }
  else 
    { 
      DXSetError(ERROR_NOT_IMPLEMENTED,"#10913",_dxd_gi_numdims);
      goto error;
    }

  DXFree((Pointer) data );
  return d1;

 error:
  if (d1) DXFree((Pointer)d1);

  return NULL;
}

/* 
 * Denormalize any floating point numbers in XF.
 */
#if !defined(DXD_STANDARD_IEEE)
static void
_dxfdenormalize(void **XF)
{
  int size, f, s, i, badfp, denorm, nitems;

  for (f=0 ; f<_dxd_gi_numflds ; f++) {
  size = _dxfnumGridPoints(f); 
	if (_dxd_gi_whichflds[f] == ON) {
 	    nitems = size * _dxd_gi_var[f]->datadim;
	    badfp = denorm = 0;
	    switch  (_dxd_gi_var[f]->datatype) {
		case TYPE_DOUBLE:
		    for (s=i=0 ; i<_dxd_gi_series ; i++, s+=_dxd_gi_numflds)  
			if (_dxd_gi_whichser[i] == ON)  
			    _dxfdouble_denorm(XF[s+f],nitems,&denorm,&badfp);
		    break;
		case TYPE_FLOAT:
		    for (s=i=0 ; i<_dxd_gi_series ; i++, s+=_dxd_gi_numflds)  
			if (_dxd_gi_whichser[i] == ON)  
			    _dxffloat_denorm(XF[s+f],nitems,&denorm,&badfp);
		    break;
	    }
	    if (denorm && badfp)
		DXWarning("Denormalized and NaN/Infinite floating point numbers in field '%s'",
			_dxd_gi_var[f]->name);
	    else if (denorm) 
		DXWarning("Denormalized floating point numbers in field '%s'",
			_dxd_gi_var[f]->name);
	    else if (badfp)
		DXWarning("NaN/Infinite floating point numbers in field '%s'",
			_dxd_gi_var[f]->name);
		
	}
  }
}
#endif /* !normal ieee */

/*
 * Convert cnt items of type from_type in the given array to floats.
 * Return a pointer to the converted array. 
 * We do an DXAllocateLocal here, because we know that the array data 
 * will eventually be copied into an Array with DXAddArrayData.
 */
static Pointer 
locations2float(Pointer from, Type from_type, int cnt)
{
    int i;
    float *f;

    f = (float*)DXAllocateLocal(cnt * sizeof(float));
    if (!f) {
	f = (float *)DXAllocate(cnt * sizeof(float));
	if (!f)
	    goto error;
    }		
	
    switch (from_type) {
	case TYPE_UBYTE:
	    for (i=0 ; i<cnt ; i++) f[i] = (float)DREF(ubyte,from,i);
	    break;
	case TYPE_BYTE:
	    for (i=0 ; i<cnt ; i++) f[i] = (float)DREF(byte,from,i);
	    break;
	case TYPE_SHORT:
	    for (i=0 ; i<cnt ; i++) f[i] = (float)DREF(short,from,i);
	    break;
	case TYPE_USHORT:
	    for (i=0 ; i<cnt ; i++) f[i] = (float)DREF(ushort,from,i);
	    break;
	case TYPE_INT:
	    for (i=0 ; i<cnt ; i++) f[i] = (float)DREF(int,from,i);
	    break;
	case TYPE_UINT:
	    for (i=0 ; i<cnt ; i++) f[i] = (float)DREF(uint,from,i);
	    break;
	case TYPE_DOUBLE:	/* FIXME: may need to check value on PVS */
	    for (i=0 ; i<cnt ; i++) f[i] = (float)DREF(double,from,i);
	    break;
	default:
	    DXErrorGoto(ERROR_UNEXPECTED,"Can't copy locations for given type");
    }
    return (Pointer)f;
error:
    if (f) DXFree((Pointer)f);
    return NULL;
}

