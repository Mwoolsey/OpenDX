/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dxmods/genimp.c,v 1.5 2000/08/24 20:08:37 davidt Exp $
 */

#include <dxconfig.h>


#include <string.h>
#include  "genimp.h"


/* GLOBAL variables : Do not depend on these initializations!! */
char	*_dxd_gi_filename=NULL;	/* input data file name (w/wo full paths) */
int	_dxd_gi_numdims=0;		/* dimensionality 			  */
int	_dxd_gi_dimsize[MAX_POS_DIMS]={0};/* dimension sizes			  */
int	_dxd_gi_series=0;		/* total number of records (time steps)	  */
int	_dxd_gi_numflds=0;		/* total number of fields 		  */
int	_dxd_gi_format=0;		/* either ASCII or BINARY 		  */
int	_dxd_gi_asciiformat=0;      /* either FREE or FIX for ascii file format */
int	_dxd_gi_majority=0;		/* either ROW or COL 			  */
interleave_type _dxd_gi_interleaving=DEFAULT_INTERLEAVE;
position_type _dxd_gi_positions=DEFAULT_PRODUCT;
float	*_dxd_gi_dataloc=NULL;      	/* Position locations            	  */
struct  variable **_dxd_gi_var={0};	/* pointers to variables 		  */
struct infoplace **_dxd_gi_fromfile = {0};	/* pointers to placeoffsets of grid	  */
int     *_dxd_gi_whichser=NULL;         /* Series numbers to be imported          */
int     *_dxd_gi_whichflds=NULL;	/* Field numbers to be imported           */
float   *_dxd_gi_seriesvalues=NULL;	/* Series values                          */
int     _dxd_gi_loc_index=0;		/* Index of field locations in XF array   */
int     _dxd_gi_pos_type[MAX_POS_DIMS] = {0};/* Position type of each dimension   */
dep_type _dxd_gi_dependency = DEP_POSITIONS;
conn_type _dxd_gi_connections=CONN_UNKNOWN;
ByteOrder _dxd_gi_byteorder=BO_MSB;	/* Byte order of binary files */

struct header _dxd_gi_header = {0,SKIP_LINES};
struct header _dxd_gi_serseparat = {0,SKIP_LINES};

static Error SetDefault(void);
static Array make_positions(void *XF);
static Field build_Field(int, void *, Array, Array);
static Error genimpsetup(char **fldnames, int *start, int *end, int *delta);
static Error SetDefault(void);
static void genimpcleanup(void);
static Object import_Group(FILE **);

/*
 * Given the series and field index calculate the corresponding index into 
 * an array (XF) of data pointers. 
 */
#define DATA_INDEX(s_index, f_index) (s_index*_dxd_gi_numflds + f_index)

/*
 * This is the top level routine for general importer. It takes the
 * input parameters from the import module as defined by the m_Import
 * driver.
 *
 * Returns the imported Object on success, and NULL and sets the error code
 * on failure.  The return object can be a Field, Series, Group of fields
 * or a Group of Series depending upon the data that is imported.
 */
Object _dxf_gi_get_gen(struct parmlist *p)
{
  Object gr;
  FILE *datafp;

  datafp=NULL;
  if (!(SetDefault()))
    goto error;
  
  if (!(_dxf_gi_InputInfoTable(p->filename,p->format,&datafp)))
    goto error;
  
  if (!(genimpsetup(p->fieldlist, 
		    p->startframe, 
		    p->endframe, 
		    p->deltaframe)))
    goto error;
  
  if(!(gr = import_Group(&datafp)))
    goto error;

  if (datafp)
    _dxfclose_dxfile(datafp,_dxd_gi_filename);
  genimpcleanup();

  return gr;
  
 error:
  if (datafp)
    _dxfclose_dxfile(datafp,_dxd_gi_filename);
  genimpcleanup();

  if ( DXGetError() == ERROR_NONE )
    {
      DXSetError(ERROR_INTERNAL, 
	"General importer got an error but did not set the error code");
    }
  
  return NULL;
}


/*
 * Initialize the default global variable values  for the data file 
 * specification.
 * NOTE: these may overriden after table is processed.
 * Returns ERROR/OK and sets error code.
 */
static Error
SetDefault(void)
{
  int i;
  _dxd_gi_series = 1;
  
  /* temporarily set number of fields to 0 (it's like a flag). We construct 
     one variable with the name "field" (the default field name), so actually
     the number of fields is 1 */
  
  _dxd_gi_whichser = NULL;
  _dxd_gi_whichflds = NULL;
  _dxd_gi_seriesvalues = NULL;
  _dxd_gi_dataloc = NULL;
  _dxd_gi_filename = NULL;

  _dxd_gi_numflds = 0; 

  /* Allocate pointers to the variable structure */
  if ((_dxd_gi_var = 
    (struct variable **)DXAllocate(sizeof(struct variable *) *MAX_VARS))
       == NULL)
    goto error;

  for(i = 0; i < MAX_VARS; i++)
    _dxd_gi_var[i] = NULL;
    

  if((_dxd_gi_var[0] = (struct variable *)DXAllocate(sizeof(struct variable)))==NULL)
      goto error;

  strcpy(_dxd_gi_var[0]->name, "field");
  _dxd_gi_format       = ASCII;
  _dxd_gi_asciiformat  = FREE;
  _dxd_gi_majority     = ROW;
  _dxd_gi_interleaving = DEFAULT_INTERLEAVE;
  _dxd_gi_positions    = DEFAULT_PRODUCT;
  _dxd_gi_header.size  = 0;
  _dxd_gi_header.type = SKIP_NOTHING;
  _dxd_gi_connections = CONN_UNKNOWN;
  _dxd_gi_dependency = DEP_POSITIONS;
  _dxd_gi_serseparat.size = 0;
  _dxd_gi_serseparat.type = SKIP_NOTHING;
  _dxd_gi_fromfile = NULL;
  return OK;
  
error:
  return ERROR;
}


/*
 * Initializes the _dxd_gi_whichser and GI_whichfld arrays to be used in the
 * import_Group function.  This is done after parsing the info file
 * once we know how many series and fields there will be.
 * Returns OK/ERROR and sets error code.
 */
static Error
genimpsetup(char **fldnames, int *start, int *end, int *delta)
{
  int i, j, mystart, myend, mydelta;
  
  if(!(_dxd_gi_whichser = (int *)DXAllocate(_dxd_gi_series*sizeof(int))))
      goto error;

  if(!(_dxd_gi_whichflds = (int *)DXAllocate(_dxd_gi_numflds*sizeof(int))))
      goto error;
  
  for(i=0; i<_dxd_gi_numflds; i++)
    _dxd_gi_whichflds[i] = OFF;
  
  for(i=0; i<_dxd_gi_series; i++)
    _dxd_gi_whichser[i] = OFF;
  
  if (fldnames) {
      for (j=0 ; fldnames[j] ; j++) {
	  for(i=0 ; i<_dxd_gi_numflds; i++)	
    	    if(!strcmp(fldnames[j],_dxd_gi_var[i]->name)) {
		_dxd_gi_whichflds[i] = ON;
		break;
	    }
	  if (i == _dxd_gi_numflds) {
	    DXSetError(ERROR_BAD_PARAMETER,"#10900",fldnames[j]); 
	    goto error;
	  }
      }
      if (_dxd_gi_positions == FIELD_PRODUCT && 
	  _dxd_gi_whichflds[_dxd_gi_loc_index] == ON && j>1)
	 DXWarning("'locations' variable ignored when other field(s) are specified, the positions of the field(s) are the locations");
	
      if (_dxd_gi_positions == FIELD_PRODUCT)
	_dxd_gi_whichflds[_dxd_gi_loc_index] = ON;
  } else {
      for(i=0; i<_dxd_gi_numflds; i++)
	_dxd_gi_whichflds[i] = ON;
  }
  
  if (!start)
    mystart = 0;
  else
    mystart = *start;

  if (!end)
    myend = _dxd_gi_series-1;
  else
    myend = *end;

  if (!delta)
    mydelta = 1;
  else { 
    mydelta = *delta;
    if (mydelta <= 0)  {
      DXSetError(ERROR_BAD_PARAMETER,"10020","'delta'");
      goto error;
    }
  }

  if ( mystart >= _dxd_gi_series || myend >= _dxd_gi_series ){
      DXSetError(ERROR_BAD_PARAMETER,"#10901");
      goto error;
  }
  
  if ( mystart > myend ){
      DXSetError(ERROR_BAD_PARAMETER,"#12260",mystart,myend);
      goto error;
  }
  for(i = mystart; i <= myend; i += mydelta)
    _dxd_gi_whichser[i] = ON;
  return OK;
  
 error:
  return ERROR;
}

/*
 * Clean up allocated memory after we're all done importing the data.
 */
static void 
genimpcleanup(void)
{
  int i;

  for(i=0; i<_dxd_gi_numflds; i++)
    if(_dxd_gi_var[i])
      DXFree((Pointer)_dxd_gi_var[i]);

  DXFree((Pointer)_dxd_gi_var);

  if(_dxd_gi_seriesvalues)
    DXFree((Pointer)_dxd_gi_seriesvalues);

  if(_dxd_gi_whichflds)
    DXFree((Pointer)_dxd_gi_whichflds);

  if(_dxd_gi_whichser)
    DXFree((Pointer)_dxd_gi_whichser);

  if(_dxd_gi_dataloc)
    DXFree((Pointer)_dxd_gi_dataloc);

  if (_dxd_gi_filename)
    DXFree((Pointer)_dxd_gi_filename);

  if (_dxd_gi_fromfile){
     for (i=0; _dxd_gi_fromfile[i]; i++){
	DXFree((Pointer)_dxd_gi_fromfile[i]->data);
	DXFree((Pointer)_dxd_gi_fromfile[i]->skip);
	DXFree((Pointer)_dxd_gi_fromfile[i]->width);
        DXFree((Pointer)_dxd_gi_fromfile[i]);
     }
     DXFree(_dxd_gi_fromfile);
  }

}

/*
 * Generates a Field, Series, Field Group or Series  Group as defined
 * in the text file and the input parameters to Import().
 * 
 * XF  deserves some explanation; it is a one dimensional array of pointers
 * such that each non-NULL pointer points to data for a given series and field 
 * within that series.  Data for the s'th series and the f'th field within 
 * series s is located at XF[s*_dxd_gi_numflds + f], where '_dxd_gi_numflds' is the number of 
 * fields in a each series (i.e. that are imported).  We allocate the full list
 * of pointers (all NULL) and the initialize only the entries for which we 
 * should be importing data, indicated by _dxd_gi_whichser[s] and _dxd_gi_whichflds[f].
 * 
 * We can allocate the memory for XF in local memory as we eventually call
 * DXAddArrayData() with XF[?] as a parameter.  Since DXAddArrayData() copies the
 * data in it is ok to use local memory for XF.
 * 
 * Return the Object when successful otherwise a NULL and set the error code.
 */
static Object 
import_Group(FILE **datafp)
{
  Group   g = NULL;
  Object  o = NULL;
  Field   fld = NULL;
  Series  *ser = NULL;
  Array con = NULL;
  Array pos = NULL;
  int f, i, j, d, size, member, imported_members, imported_fields;
  int delete_pos = 0, delete_con = 0,loconly = 0;
  int s;
  void **XF;
  int dimsize[MAX_POS_DIMS];

  /*size = _dxfnumGridPoints();*/ 

  if (!(XF = (void**)DXAllocateLocalZero(_dxd_gi_series*_dxd_gi_numflds*sizeof(void*))))
      goto error;

  for (d = i = 0; i < _dxd_gi_series; i++)
    {
      if (_dxd_gi_whichser[i] == OFF )
	{
	  d+=_dxd_gi_numflds;
	}
      else if (_dxd_gi_whichser[i] == ON)
	{
	  for (j = 0; j < _dxd_gi_numflds; j++)
	    {
	      if (_dxd_gi_whichflds[j] == ON)
		{
	          size = _dxfnumGridPoints(j);
		  if (!(XF[d] = (void*)DXAllocateLocalZero(size *
				_dxd_gi_var[j]->datadim * _dxd_gi_var[j]->databytes)))
		      goto error;
		}
	      d++;
	    }
	}
      else
	  DXErrorGoto(ERROR_UNEXPECTED,"Error in generating group"); 
    }
  
  /*
   * DXRead in the data from the user's data file into the correct buffers
   * pointed to by XF[i].  Buffers come back filled with data in correct 
   * byte order, in row majority order and denormalized if necessary.
   */
  if (!_dxf_gi_read_file(XF,datafp))
    goto error;
  
  /*
   * Make up the positions component based on the user's  'positions = '
   * inputs.  If they have specified the field 'locations' keyword then
   * positions are specific to each series so wait to create the component
   * until we have the series number. 
   */
  if (_dxd_gi_positions != FIELD_PRODUCT) {
  	if (!(pos = make_positions(NULL)))
      		goto error;
	delete_pos = 1;
   }


  /*
   * Make a connections grid if they are needed (not using 'points ='). 
   */
  if (_dxd_gi_connections == CONN_GRID) {
    /* check for dimensions of 1 */
    for (i=0, j=0; i<_dxd_gi_numdims; i++) 
       if (_dxd_gi_dimsize[i] > 1) dimsize[j++] = _dxd_gi_dimsize[i];
    if( !(con = DXMakeGridConnectionsV(j, dimsize)) ) 
      goto error;
    delete_con = 1;
  }
  
  /*
   * Determine the number of fields and series to produce and thus the
   * type of output object to create and return.
   */
  for (i=0, imported_members=0; i<_dxd_gi_series ; i++)
    if (_dxd_gi_whichser[i] == ON)
      imported_members++;
  ASSERT(imported_members > 0); 

  /*
   * Determine the number of imported fields and 
   * if there is more than one member of (each of) the field(s), then 
   * create a series for each imported field. 
   */
  if (imported_members > 1) {
	ser = (Series*)DXAllocateLocalZero(_dxd_gi_numflds * sizeof(Series));
	if (!ser)
	    goto error;
  }

  for (f=0, imported_fields=0; f<_dxd_gi_numflds; f++) {
    if ((_dxd_gi_whichflds[f] == ON) && 
	!(_dxd_gi_positions == FIELD_PRODUCT && f == _dxd_gi_loc_index)) {
          imported_fields++;
	  if ((imported_members > 1) && !(ser[f] = DXNewSeries())) 
		goto error;
    }
  }
  if (imported_fields==0 && 
      _dxd_gi_positions == FIELD_PRODUCT ){
     imported_fields = 1;
     if ((imported_members > 1) && !(ser[0] = DXNewSeries()))
           goto error;
     loconly=1;
   }
     
  ASSERT(imported_fields > 0); 

  /*
   * Create a group if the number of fields is greater than 1.
   * The resulting group may be either a group of fields or series, but
   * in either case the number of imported fields is greater than 1. 
   */
  if ((imported_fields > 1) && !(g = DXNewGroup()))
	goto error;

	
  /*
   * For each imported series, and each imported field within that series
   * build the field with the correct positions and connections and possibly
   * insert it into either a series (imported_members > 1) or 
   * a group (imported_members == 1 && imported_fields > 1). 
   */
  for (member = -1, s=0; s<_dxd_gi_series; s++) {
    if (_dxd_gi_whichser[s] == ON) {		/* If this series was requested */
	member++;			/* Incremement series member # */
	if (_dxd_gi_positions == FIELD_PRODUCT){/* Make positions from 'locations' */
	    if (!(pos = make_positions(XF[s*_dxd_gi_numflds + _dxd_gi_loc_index])))
		goto error;
	    delete_pos = 1;
        }
        if (loconly==1){
           if (!(fld = build_Field(-1,XF[s*_dxd_gi_numflds+f], pos, con)))
                 goto error;
           /* pos and con are now referenced */
           delete_con = delete_pos = 0;

           if (imported_members > 1) {
              if (!(DXSetSeriesMember(ser[0], member,
                   (double)_dxd_gi_seriesvalues[s],(Object)fld)))
                          goto error;
              fld = NULL;     /* It is now referenced */
           }
           /* if only one member of series imported still add the 
            * series position attribute by hand */
	    else if (_dxd_gi_series > 1 ){
	       DXSetFloatAttribute((Object)fld,"series position",
				   _dxd_gi_seriesvalues[s]);
	    }
        }
	for (f=0; f<_dxd_gi_numflds ; f++) {
	    if (_dxd_gi_whichflds[f] == ON &&
              !(_dxd_gi_positions == FIELD_PRODUCT &&
                      f == _dxd_gi_loc_index)) {
		 if (!(fld = build_Field(f,XF[s*_dxd_gi_numflds+f], pos, con)))
		      goto error;

		    /* pos and con are now referenced */
		    delete_con = delete_pos = 0;

                    /* if only one member of series imported still add the 
		     * series position attribute by hand */
		    if (_dxd_gi_series > 1 && imported_members ==1){
		       DXSetFloatAttribute((Object)fld,"series position",
					   _dxd_gi_seriesvalues[s]);
		    }
		    if (imported_members > 1) {
			if (!(DXSetSeriesMember(ser[f], member, 
					      (double)_dxd_gi_seriesvalues[s], 
					      (Object)fld)))
			  goto error;
			fld = NULL;	/* It is now referenced */
		    } else if (g) {
			if (!(DXSetMember(g, _dxd_gi_var[f]->name, (Object)fld)))
			    goto error;
			fld = NULL;	/* It is now referenced */
		    }
	    }
	}
     }
  }

  /*
   * If imported_members > 1 && imported_fields > 1 then we have to 
   * create a group of series.
   */
  if (ser && g) {
	ASSERT(imported_members > 1);
	ASSERT(imported_fields  > 1);
	for (f=0 ; f<_dxd_gi_numflds ; f++) {
	    if (ser[f] && !(DXSetMember(g, _dxd_gi_var[f]->name, (Object)ser[f])))
		 goto error;
	    ser[f] = NULL;	/* It is now referenced */
	}
  } 

  /*
   * DXFree up the memory we used. 
   */
  if (XF)  {
    for (i = 0; i < _dxd_gi_series*_dxd_gi_numflds; i++)
      if (XF[i]) DXFree((Pointer)XF[i]);
    DXFree((Pointer)XF);
  }
 
  /*
   * Return the correct object. 
   */
  if (imported_fields == 1 && imported_members == 1) {
    ASSERT(fld != NULL);
    return (Object)fld;
  } else if (imported_fields == 1 && imported_members > 1) {
    /* 
     * Look for the first non-zero element of ser[], this is an alternative
     * to looking in _dxd_gi_whichflds[].
     */
    o = NULL;
    for (f=0 ; f<_dxd_gi_numflds && !o ; f++)
	o = (Object)ser[f];
    ASSERT(o != NULL);
    DXFree((Pointer)ser);
    return (Object)o;
  } else if (imported_fields > 1 && imported_members == 1) {
    ASSERT(g != NULL);
    return (Object)g;
  } else if (imported_fields > 1 && imported_members > 1) {
    ASSERT(g != NULL);
    DXFree((Pointer)ser);
    return (Object)g;
  }

 error:
  if (g) DXDelete((Object)g);
  if (fld) DXDelete((Object)fld);
  if (delete_pos && con) DXDelete((Object)pos); 
  if (delete_con && con) DXDelete((Object)con);

  if (ser)  {
    for (f = 0; f < _dxd_gi_numflds; f++)
      if (ser[f]) DXDelete((Object)ser[f]);
    DXFree((Pointer)ser);
  } 

  if (XF)  {
    for (i = 0; i < _dxd_gi_series*_dxd_gi_numflds; i++)
      if (XF[i]) DXFree((Pointer)XF[i]);
    DXFree((Pointer)XF);
  }
  
  return NULL;
  
}

/*
 * Build a field with the given series and field index, using data 
 * pointed to by XF[].  Specifics of the field are found in _dxd_gi_var[f_index].
 * Return a field when successful otherwise a NULL and set error code.
 */
static
Field build_Field(int f_index, void *XF, Array pos, Array con)
{
  Field f = NULL;
  Array a = NULL;
  Object o = NULL;
  int size;
  char	*depstr;
  
  /*size = _dxfnumGridPoints();*/ 
  size = _dxfnumGridPoints(f_index); 

  if (!(f = DXNewField())) 
    goto error;

  /* f_index = -1 flag for locations only field */
  if (f_index!=-1){
  /* 
   * Create the "data" component for the field.
   * FIXME:  Presently general importer supports only CATEGORY_REAL
   */ 
  if (_dxd_gi_var[f_index]->datadim > 1)  {
    int shape = _dxd_gi_var[f_index]->datadim;
    a = DXNewArrayV(_dxd_gi_var[f_index]->datatype, CATEGORY_REAL, 1, &shape);
  } else
    a = DXNewArray(_dxd_gi_var[f_index]->datatype, CATEGORY_REAL, 0);
  if (!(a))
    goto error;

  if ( !(DXAddArrayData(a, 0, size, (Pointer)XF))) 
    goto error;

  if ( !(DXSetComponentValue(f, "data", (Object)a)) ) 
    goto error;
  
  /*
   * Name the field.
   */
  if (!(o = (Object)DXNewString(_dxd_gi_var[f_index]->name))  ||
      !DXSetAttribute((Object)f, "name", o))
        goto error;
  o = NULL;

  /*
   * DXAdd correct data dependency. 
   */
  /*  if (_dxd_gi_dependency == DEP_CONNECTIONS) */
  if (_dxd_gi_var[f_index]->datadep == DEP_CONNECTIONS)
	depstr = "connections";
  else
	depstr = "positions";

  if (!(o = (Object)DXNewString(depstr))  ||
      !DXSetComponentAttribute(f, "data", "dep", o))
	goto error;
  o = NULL;
  }

  /*
   * DXAdd positions to the field.
   */
  if( !(DXSetComponentValue(f, "positions", (Object)pos)))
    goto error;
 
  /*
   * DXAdd connections if requested ('grid =' as opposed to 'points =') 
   * to the field.
   */
  if (_dxd_gi_connections == CONN_GRID)
    {
      if( !(DXSetComponentValue(f, "connections", (Object)con)))
	goto error;
    }

#ifdef DO_ENDFIELD
  if (!DXEndField(f)) 
    goto error;
#endif

  return f;

 error: 
  if (f) DXDelete((Object)f);
  if (a) DXDelete((Object)a);
  if (o) DXDelete((Object)o);

  return NULL;
}


/*
 * Create an Array of positions based on the global variable 'positions'
 * and possibly the field data at XF (and _dxd_gi_var[loc_indx], user
 * used 'locations' keyword) or _dxd_gi_dataloc[i] which holds data specified
 * in the 'positions = ' statement.
 *
 * The output positions array has '_dxd_gi_numdims'  dimensions as follows,
 * _dxd_gi_dimsize[0] X _dxd_gi_dimsize[1] ... X _dxd_gi_dimsize[_dxd_gi_numdims-1].
 * The output array can be either, regular, regular product, mixed product,
 * irregular or 'field' product for which data  is taken from XF.
 *
 * FIXME: general importer only supports TYPE_FLOAT and CATEGORY_REAL
 *
 */
static Array 
make_positions(void *XF)
{
  int size, i, j, k;
  float origin[MAX_POS_DIMS], deltas[MAX_POS_DIMS*MAX_POS_DIMS], *mixpositions;
  Array *mixarr = NULL, pos = NULL;
  int shape;

  /*size = _dxfnumGridPoints();*/ 
  size = _dxfnumGridPoints(-1); 
  mixarr = NULL;

  for (j=0; j<_dxd_gi_numdims; j++)
      origin[j] = 0.0;

  for (j=0; j<_dxd_gi_numdims*_dxd_gi_numdims; j++)
      deltas[j] = 0.0;
  
  if (_dxd_gi_positions == FIELD_PRODUCT)
    {

       /* 
	* Build an array to hold the user's data (which should be floats
	* or was converted to float in _dxf_gi_read_file()).
	* positions array should always be rank 1
	*/ 
      shape = _dxd_gi_var[_dxd_gi_loc_index]->datadim;
      pos = DXNewArrayV(TYPE_FLOAT, CATEGORY_REAL, 1, &shape);
      if (!pos)
	goto error;
      
       /* 
	* DXAdd the user's data to the array. 
	*/ 
      if (!DXAddArrayData(pos, 0, size, (Pointer)XF)) 
	goto error;
    }
  else if (_dxd_gi_positions == DEFAULT_PRODUCT)
    {
       /* 
	* Build the default positions array, a compact product of
	* arrays with each dimension having an origin of 0 and a delta of 1. 
	*/ 
      for (i=0; i<_dxd_gi_numdims; i++)
	  origin[i] = 0.0;

      for (i=0, j=0; i<_dxd_gi_numdims; i++)
	{
	  deltas[j] = 1.0;
	  j += _dxd_gi_numdims + 1;
	}

      if( !(pos = DXMakeGridPositionsV(_dxd_gi_numdims, _dxd_gi_dimsize, origin, deltas)) ) 
	goto error;
      
    }
  else if (_dxd_gi_positions == REGULAR_PRODUCT)
    {
       /* 
	* The user specified a regular product array with origin,delta 
	* for each dimension where origin[i] = _dxd_gi_dataloc[2*i] and
 	* delta[i] = _dxd_gi_dataloc[2*i + 1] for each dimension i.
	*/ 
      for (j=i=0; i<_dxd_gi_numdims; i++)
	{
	  origin[i] = _dxd_gi_dataloc[j];
	  j += 2;
	}
      for (i=0, k=0, j=1; i<_dxd_gi_numdims; i++)
	{
	  deltas[k] = _dxd_gi_dataloc[j];
	  j += 2;
	  k += _dxd_gi_numdims + 1;
	}
      if( !(pos = DXMakeGridPositionsV(_dxd_gi_numdims, _dxd_gi_dimsize, origin, deltas)) ) 
	goto error;
      
    }
  else if (_dxd_gi_positions == IRREGULAR_PRODUCT)
    {
       /* 
	* The user specified a set of irregular positions for each dimension 
	* that are to make up an irregular product array.
	*/ 
      shape = _dxd_gi_numdims;
      pos = DXNewArrayV(TYPE_FLOAT, CATEGORY_REAL, 1, &shape);
      if (!(pos))
	goto error;
      
      if (!DXAddArrayData(pos, 0, size, (Pointer)_dxd_gi_dataloc)) 
	goto error;

    }
  else if (_dxd_gi_positions == MIXED_PRODUCT)
    {
       /* 
	* The user specified a set of regular and irregular positions for 
	* each dimension that are to make up a product array.
	*/ 
      j = 0;
      if (!(mixarr = (Array *)DXAllocateZero(_dxd_gi_numdims*sizeof(Array))))
	goto error;
      for (i=0; i<_dxd_gi_numdims; i++)
	{
	  if (_dxd_gi_pos_type[i] == REGULAR)
	    {
	      for (k=0; k<_dxd_gi_numdims; k++)
		{
		  origin[k] = 0.0;
		  deltas[k] = 0.0;
		}
	      origin[i] = _dxd_gi_dataloc[j++];
	      deltas[i] = _dxd_gi_dataloc[j++];
	      mixarr[i] = (Array)DXNewRegularArray(TYPE_FLOAT, _dxd_gi_numdims, 
						 _dxd_gi_dimsize[i], 
						 (Pointer)origin, 
						 (Pointer)deltas);
	      if (!mixarr[i])	
		goto error;
	    }
	  else if (_dxd_gi_pos_type[i] == IRREGULAR)
	    {
	      if (!(mixpositions =
		    (float *)DXAllocate(_dxd_gi_numdims*_dxd_gi_dimsize[i]*sizeof(float))))
		  goto error;

	      for (k=0; k<_dxd_gi_dimsize[i]*_dxd_gi_numdims; k++)
		  mixpositions[k] = 0.0;

	      for (k=0; k<_dxd_gi_dimsize[i]; k++)
		  mixpositions[(k*_dxd_gi_numdims)+i] = _dxd_gi_dataloc[j++];
	      
	      
	      if (_dxd_gi_numdims > 1)
		{
	          shape = _dxd_gi_numdims;
		  if (!(mixarr[i] = 
		       DXNewArrayV(TYPE_FLOAT, CATEGORY_REAL, 1, &shape)))
		    goto error;
		  if (!(DXAddArrayData(mixarr[i], 0, _dxd_gi_dimsize[i], 
						(Pointer)mixpositions))) 
		    goto error;
		}
	      else
		{
		  if (!(pos = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 0)))
		    goto error;
		  if (!(DXAddArrayData(pos, 0, _dxd_gi_dimsize[i], 
						(Pointer)mixpositions))) 
		    goto error;
		}
	      DXFree((Pointer)mixpositions);
	    }
	}
      if (_dxd_gi_numdims > 1)
	if (!(pos = (Array)DXNewProductArrayV(_dxd_gi_numdims, mixarr)))
	  goto error;
      DXFree(mixarr);
    }

  return pos;

error:
  if (pos) DXDelete((Object)pos);

  if (mixarr){
    for(i=0; i<_dxd_gi_numdims; i++)
      if (mixarr[i])
	DXDelete((Object)mixarr[i]);
    DXFree(mixarr);
  }
  
  return NULL;
}


/*
   FN: figure out the number of grid points.
*/

int _dxfnumGridPoints(int f_index)
{
  int i, size;
  int count; 

  /* One less data point in each dimension of the grid  for dep connections */
  /* f_index = -1 always like dep connections */
  if (f_index!=-1 && _dxd_gi_var[f_index]->datadep == DEP_CONNECTIONS){
     for (i = 0,size = 1,count = 0; i < _dxd_gi_numdims; i++) {
	if (_dxd_gi_dimsize[i] > 1) 
	   size *= (_dxd_gi_dimsize[i] - 1);
	else
	   count++;
     }
     if (count == _dxd_gi_numdims)
	size = 0;
  }
  else{
     for(i = 0, size = 1; i < _dxd_gi_numdims; size *= _dxd_gi_dimsize[i], i++) 
        ;
  }

  return( size );
}


