/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dxmods/genimp_parse.c,v 1.9 2002/03/21 02:57:35 rhh Exp $
 */

#include <dxconfig.h>


#include <string.h>
#include "genimp.h"

/* This should be the only visible entry point for this file */
Error _dxf_gi_InputInfoTable   (char *, char *,FILE **);

/* Flags to FileTok() 'newline' parameter */
#define NO_NEWLINE	0
#define NEWLINE_OK	1
#define FORCE_NEWLINE	2
#define L_CASE(str)                                             \
                {int si;                                        \
                for(si = 0; si < strlen(str); si++)             \
                    str[si] = tolower(str[si]);}

struct  parse_state {
        FILE    *fp;
        char    *filename;
	int	lineno;
        char    *line;
        char    token[MAX_DSTR];
        char    *current;
};

static Error parse_marker(char *p,char *marker);
static Error nextchar(char **p, unsigned int *c);
static Error infoFile         (struct  parse_state *); 
static Error infoGrid         (struct  parse_state *); 
static Error infoSeries       (struct  parse_state *); 
static Error infoField        (struct  parse_state *); 
static Error infoStructure    (struct  parse_state *); 
static Error infoFormat       (struct  parse_state *); 
static Error infoType         (struct  parse_state *); 
static Error infoLayout       (struct  parse_state *); 
static Error infoMajority     (struct  parse_state *); 
static Error infoHeader       (struct  parse_state *); 
static Error infoInterleaving (struct  parse_state *); 
static Error infoBlock        (struct  parse_state *); 
static Error infoPositions    (struct  parse_state *, struct place_state *); 
static Error infoPoints       (struct  parse_state *); 
static Error infoDepend       (struct  parse_state *); 
static Error infoRecSeparat   (struct  parse_state *);
static Error infoEnd          (struct  parse_state *); 
static Error MakeupDefault    (char *, int);
static Error CheckInputs(void);
static Error InitFileTok(struct parse_state *, FILE *, char *);
static Error FileTok(struct parse_state *,char *, int , char** );
static Error get_header(struct parse_state *, char *, struct header *);
static Error _dxf_getline(struct parse_state *);
static Error parse_template(char *, char **);
static Error getmorevar(int *);
static Error readplacement(char **, struct parse_state *, int, int *,char *);
static Error readdesc(char **,struct parse_state *,int *,ByteOrder *,Type *,
		     char *);
static Error parse_datafile(struct place_state *);
static Error get_format(struct parse_state *,char **,int *,ByteOrder *);
static Error get_type(struct parse_state *,char **,Type *,int *);

/* 
 * Bits for flags field of struct parameter 
 */
#define PARAM_SET		0x1

#define  PARAM_SPECIFIED(pnum)		(para[pnum].flags & PARAM_SET)
#define  SET_PARAM_SPECIFIED(pnum)	(para[pnum].flags |= PARAM_SET)
#define  CLR_PARAM_SPECIFIED(pnum)	(para[pnum].flags &= ~PARAM_SET)

struct parameter{
	char	name[32];
	int	(* getInfo)();
	int	flags;
};

#define   PARAM_File            0
#define   PARAM_Grid            1
#define   PARAM_Series          2
#define   PARAM_Fields          3
#define   PARAM_Structure       4
#define   PARAM_Format          5
#define   PARAM_Type            6
#define   PARAM_Layout          7
#define   PARAM_Majority        8
#define   PARAM_Header          9
#define   PARAM_Interleaving    10
#define   PARAM_Block           11
#define   PARAM_Positions       12
#define   PARAM_Points          13
#define   PARAM_Depend          14
#define   PARAM_RecSeparat      15
#define   PARAM_End             16
#define   NUM_PARS              17      /* number of parameters in table */

static struct parameter para[] = 
{
	{ "file",         infoFile,		0},
	{ "grid",         infoGrid,		0},
	{ "series",       infoSeries,		0},
	{ "field",        infoField,		0},
	{ "structure",    infoStructure,	0},
	{ "format",       infoFormat,		0},
	{ "type",         infoType,		0},
	{ "layout",       infoLayout,		0},
	{ "majority",     infoMajority,		0},
	{ "header",       infoHeader,		0},
	{ "interleaving", infoInterleaving,	0},
	{ "block",        infoBlock,		0},
	{ "positions",    infoPositions,	0},
	{ "points",       infoPoints,		0},
	{ "dependency",   infoDepend,		0},
	{ "recordseparator", infoRecSeparat,	0},
	{ "end",          infoEnd,		0}
}; 

static struct type_spec {
	Type 	type;
	char	*type_name;
} type_table [] = {
	{ TYPE_UBYTE,  "byte" },
	{ TYPE_UBYTE,  "ubyte" },
	{ TYPE_BYTE,   "sbyte" },
	{ TYPE_SHORT,  "short"},
	{ TYPE_USHORT, "ushort"},
	{ TYPE_INT,    "int" },
	{ TYPE_UINT,   "uint" },
	{ TYPE_STRING, "string"},
	{ TYPE_FLOAT,  "float"},
	{ TYPE_DOUBLE, "double"},
	{ TYPE_DOUBLE, NULL} /* terminating entry (name=NULL),must be last */
};


static struct header **rec_sep;
static int max_vars = MAX_VARS;  /* number of variables allocated */

/*
   FN: read in the information table and set up the information.
   EF: return with information in arg1 so that processing may proceed. 
*/

Error
_dxf_gi_InputInfoTable(char *table, char *format, FILE **datafp)
{
  FILE *fp;
  char *p, *headerfile;
  int i;
  struct parse_state ps;
  struct place_state dataps;

  for(i=0; i<NUM_PARS; i++)
    CLR_PARAM_SPECIFIED(i);
  max_vars = MAX_VARS;

  fp = NULL;
  dataps.fp = *datafp;
  ps.lineno = 0;
  /* copy name of file to headerfile so this can change if 
   * template exists and allocate own space for filename */
  headerfile = (char *)DXAllocateLocal(strlen(table) +1);
  if (!headerfile)
     goto error;
  strcpy(headerfile,table);

  /* if format exists table is data file and format is header file */
  if (!parse_template(format,&headerfile))
    goto error;

  /* if no template given then all header info is input thru Import */
  if (headerfile[0] == '\0') 
     goto done;

  if(!(_dxf_gi_OpenFile(headerfile, &fp, 1)))	/* 1 = info file */
    goto error;

  if (!InitFileTok(&ps,fp,headerfile))
    goto error;

  while (FileTok(&ps," =\n\t",FORCE_NEWLINE,&p)) {
     if (p)  {			/* A non-blank line */
	  /* get parameter */
          for(i=0; i<NUM_PARS ; i++) {
	      if (*p == '#') { 	/* A comment */  
		    i = NUM_PARS;	/* Exit for loop */
	      } else if (!strcmp(para[i].name, p)) {
	            if (PARAM_SPECIFIED(i)) {
			DXWarning("Ignoring extra '%s' statement in '%s'",
				   para[i].name,headerfile);
                        i = NUM_PARS; /* Exit for loop */
		    } else {
			if (i==PARAM_Positions){
			   if(!(* para[i].getInfo)(&ps,&dataps))
			      goto parse_error;
			}
			else
			   if(!(* para[i].getInfo)(&ps))
			      goto parse_error;
			SET_PARAM_SPECIFIED(i);
			i = NUM_PARS; 	/* Exit for loop */
		    }
	      }
	  }
          if (i == NUM_PARS ) {
	      DXSetError(ERROR_BAD_PARAMETER,"#10841",p);
	      goto parse_error;
	  }
	  if (PARAM_SPECIFIED(PARAM_End))
		break;
     }
  }
  DXResetError();		/* Ignore EOF error from FileTok() */
  _dxfclose_dxfile(fp,headerfile);

  /* free parse_state->line */
  DXFree(ps.line);

done:
  /* if header info contained in data file (and not already read)
   * read it here */
  if (_dxd_gi_fromfile && !parse_datafile(&dataps))
     goto error;

  *datafp = dataps.fp;

  if (!CheckInputs())
	goto error;

  if (!MakeupDefault(headerfile,ps.lineno))
	goto error;
  DXFree(headerfile);
  return OK;
 
parse_error:
  DXAddMessage(" (file %s: line %d)",headerfile,ps.lineno);
error:
  if (fp)
   _dxfclose_dxfile(fp,headerfile);
  if (headerfile)
   DXFree(headerfile);

  return ERROR;
}

/*
 *  Examine the user's input options for illegal or conflicting combinations.
 */
static Error 
CheckInputs(void)  
{
  int i,f,n;
  dep_type firstdep; 
   
  if (!PARAM_SPECIFIED(PARAM_Grid) && !PARAM_SPECIFIED(PARAM_Points)){ 
    DXSetError(ERROR_BAD_PARAMETER,"#10845","grid","points");
    goto error;
  }

  /* I don't think we'll ever see this one */
  if (PARAM_SPECIFIED(PARAM_Points) && _dxd_gi_numdims > 1){ 
    DXSetError(ERROR_BAD_PARAMETER,"#10846","points","1-D positions");
    goto error;
  }

  if (PARAM_SPECIFIED(PARAM_Layout))
    {
      if( _dxd_gi_interleaving == IT_FIELD && _dxd_gi_format == ASCII ) 
	_dxd_gi_asciiformat = FIX;
      else{ 
	DXSetError(ERROR_BAD_PARAMETER,"#10846","layout",
	"field-interleaved ascii data");
        goto error;
      }
    }
  
  if (PARAM_SPECIFIED(PARAM_Block))
    {
      if (_dxd_gi_format == ASCII && _dxd_gi_interleaving != IT_FIELD)
	_dxd_gi_asciiformat = FIX;
      else{
	DXSetError(ERROR_BAD_PARAMETER,"#10846","block",
        "non-field-interleaved ascii data");
	goto error;
      }
    }

  /* if no dimensions then grid or points was empty */
  if (_dxd_gi_numdims == 0){
     DXSetError(ERROR_DATA_INVALID,"#10850","grid","points");
     goto error;
  }

  /* check for grid size <=0 */
  for (i=0; i<_dxd_gi_numdims; i++) {
     if (_dxd_gi_dimsize[i] <= 0) {
	DXSetError(ERROR_DATA_INVALID,"#10020","'grid' dimensions or 'points'");
	goto error;
     }
  }

  /* dependency DEP_CONNECTIONS not allowed with interleaving field
   * locations can't be dep connections
   * points can't be specified
   */
  if (PARAM_SPECIFIED(PARAM_Depend)){
     firstdep = _dxd_gi_var[0]->datadep;
     for (f=0; f<_dxd_gi_numflds; f++){
       if (_dxd_gi_interleaving==IT_FIELD && firstdep!=_dxd_gi_var[f]->datadep){
           DXSetError(ERROR_DATA_INVALID,"#10855","dependency");
           goto error;
           }
        if (_dxd_gi_var[f]->datadep == DEP_CONNECTIONS){
           if (_dxd_gi_positions == FIELD_PRODUCT && f==_dxd_gi_loc_index){
           DXSetError(ERROR_DATA_INVALID,"#10856","locations");
           goto error;
           }
           if( _dxd_gi_connections == CONN_POINTS){
           DXSetError(ERROR_DATA_INVALID,"#10856","points");
           goto error;
           }
        }
     }
  }

  /* check that number of record separators matches data */
  if (PARAM_SPECIFIED(PARAM_RecSeparat)){
    if (_dxd_gi_interleaving!=IT_RECORD &&
        _dxd_gi_interleaving!=IT_RECORD_VECTOR &&
	_dxd_gi_interleaving!=IT_VECTOR){
        DXSetError(ERROR_BAD_PARAMETER,"#10846","recordseparator",
         "record, vector and record-vector interleaved data");
        goto error;
    }
    if (_dxd_gi_interleaving==IT_RECORD && PARAM_SPECIFIED(PARAM_Structure)){
      for (f=0, n=0; f < _dxd_gi_numflds; f++) 
	if (_dxd_gi_var[f]->datatype != TYPE_STRING)
	   n+=_dxd_gi_var[f]->datadim;
	else
	   n++;
    }
    else
      n = _dxd_gi_numflds;

    if (rec_sep[1]->type == SKIP_NOTHING){
       rec_sep[1]->size = rec_sep[0]->size;
       rec_sep[1]->type = rec_sep[0]->type;
       strcpy(rec_sep[1]->marker, rec_sep[0]->marker);
       for (f=2; f<n; f++){
          if ((rec_sep[f] = (struct header *)DXAllocate(sizeof(struct header))) 
	      == NULL )
	     goto error;
          rec_sep[f]->size = rec_sep[0]->size;
          rec_sep[f]->type = rec_sep[0]->type;
          strcpy(rec_sep[f]->marker,rec_sep[0]->marker);
       }
       rec_sep[n-1]->type = SKIP_NOTHING;
    }
    else if (rec_sep[n-2]->type == SKIP_NOTHING ||
             rec_sep[n-1]->type != SKIP_NOTHING){
       DXSetError(ERROR_DATA_INVALID,"#10840","recordseparator");
       goto error;
    }
    /* if format=binary separator can't be lines */
    if (_dxd_gi_format==BINARY){
       for (f=0; f<n-1; f++){
          if (rec_sep[f]->type == SKIP_LINES){
             DXSetError(ERROR_DATA_INVALID,
              "separators cannot be lines for binary data");
             goto error;
          }
       }
    }
    /* if ascii format is free and separator is lines add one line */
    if (_dxd_gi_asciiformat==FREE){
       for (f=0; f<n-1; f++){
          if (rec_sep[f]->type==SKIP_LINES)
             rec_sep[f]->size = rec_sep[f]->size +1;
       }
    }
  } 

  /* if format=binary separator can't be lines */
  if (PARAM_SPECIFIED(PARAM_Series)){
      if (_dxd_gi_format==BINARY){
         if (_dxd_gi_serseparat.type == SKIP_LINES){
            DXSetError(ERROR_DATA_INVALID,
             "separators cannot be lines for binary");
            goto error;
         }
      }
      if (_dxd_gi_asciiformat==FREE && _dxd_gi_serseparat.type==SKIP_LINES)
         _dxd_gi_serseparat.size = _dxd_gi_serseparat.size +1;
  }

  return OK;
error:
  return ERROR;
}

/*
   FN: Set defaults if they are not given 
   EF: return with all the information required to read the data file and
       generate a DX file.
*/

static Error 
MakeupDefault(char *name,int linenum)  
{
  int i,j, n=0;
  char s[10];
  int dim;

  /* check required parameters */
  
  /*int size = _dxfnumGridPoints();*/

  /* if no file given use same as info file
   * if no header then start at end of infofile 
   * last line from parsing
   */
  if (!PARAM_SPECIFIED(PARAM_File)){
    _dxd_gi_filename = (char *)DXAllocate(strlen(name) +1);
    strcpy(_dxd_gi_filename, name);    
    if (!PARAM_SPECIFIED(PARAM_Header)){
      _dxd_gi_header.size= linenum;
      _dxd_gi_header.type = SKIP_LINES;
    }
  }
  
  /* makeup variables */
  if( _dxd_gi_numflds == 0 ) 
    _dxd_gi_numflds = 1;
  
  if (!PARAM_SPECIFIED(PARAM_Fields))
    {
      for(i = 0; i < _dxd_gi_numflds; i++)
	{
	  sprintf(s, "field%d", i);
	  strcpy(_dxd_gi_var[i]->name, s);
	}
    }

  if (!PARAM_SPECIFIED(PARAM_Structure))
    for(i = 0; i < _dxd_gi_numflds; i++)
      _dxd_gi_var[i]->datadim = 1;

  if (!PARAM_SPECIFIED(PARAM_Type)) {
    j = DXTypeSize(TYPE_FLOAT); 
    for (i = 0; i < _dxd_gi_numflds; i++) {
      _dxd_gi_var[i]->datatype = TYPE_FLOAT;
      _dxd_gi_var[i]->databytes = j; 
    }
  }

  if (!PARAM_SPECIFIED(PARAM_Depend)) {
    for (i=0; i<_dxd_gi_numflds; i++)
       _dxd_gi_var[i]->datadep = DEP_POSITIONS;
  }

  /* place record header in correct places 
   * and set default if no record header
   * and free rec_sep 
   */ 
  if (_dxd_gi_interleaving == IT_RECORD){
   if (PARAM_SPECIFIED(PARAM_RecSeparat)) {
     for(i=0; i<_dxd_gi_numflds; i++){
	if (_dxd_gi_var[i]->datatype == TYPE_STRING) dim = 1;
	else dim = _dxd_gi_var[i]->datadim;
        for (j=0; j<dim; j++){
           _dxd_gi_var[i]->rec_separat[j].size = rec_sep[n]->size;
           _dxd_gi_var[i]->rec_separat[j].type = rec_sep[n]->type;
           strcpy(_dxd_gi_var[i]->rec_separat[j].marker,rec_sep[n]->marker);
           n++;
        }
     }
   }
   else{
     for (i=0; i<_dxd_gi_numflds; i++){
	if (_dxd_gi_var[i]->datatype == TYPE_STRING) dim = 1;
	else dim = _dxd_gi_var[i]->datadim;
        for (j=0; j<dim; j++)
           _dxd_gi_var[i]->rec_separat[j].type=SKIP_NOTHING;
     }
   }
  }
  else{
   if (PARAM_SPECIFIED(PARAM_RecSeparat)) {
     for(i=0; i<_dxd_gi_numflds; i++){
        _dxd_gi_var[i]->rec_separat[0].size = rec_sep[i]->size;
        _dxd_gi_var[i]->rec_separat[0].type = rec_sep[i]->type;
        strcpy(_dxd_gi_var[i]->rec_separat[0].marker,rec_sep[i]->marker);
        n++;
     }
   }
   else{
     for (i=0; i<_dxd_gi_numflds; i++)
        _dxd_gi_var[i]->rec_separat[0].type=SKIP_NOTHING;
   }
  }
  
  /* Clean up Record Sep */
  if (PARAM_SPECIFIED(PARAM_RecSeparat)){
     for (i=0; i<n; i++)
        DXFree((Pointer)rec_sep[i]); 
     DXFree((Pointer)rec_sep);
  }
 
  return OK;
}


/*
  FN: read the fields names. If the variables haven't been constructed, then
  DXAllocate memory for variables and update the number of fields. Otherwise
  do the compatibility checking.
  EF: return the variables with valid field names.
  */

static Error
infoField(struct parse_state *ps)
{
  char *current=ps->current;
  char *p;
  int myGI_numflds = 0;
  char name[MAX_MARKER];

  /* search for = and skip it, then filename can contain = */
  /* Skip any leading white space and '=' */
  while (*current && (*current == ' ' || *current == '\t' || *current == '='))
	current++;
  if (!*current) goto error;
  ps->current = current;

  while(FileTok(ps, ", \n\t",NO_NEWLINE,&p) && p)
    {
      if (myGI_numflds >= max_vars){
         if (!getmorevar(&max_vars))
            goto error;
      }
      if ( _dxd_gi_var[myGI_numflds] == NULL )
	{
	  if ((_dxd_gi_var[myGI_numflds] = 
	      (struct variable *)DXAllocate(sizeof(struct variable))) == NULL )
	      goto error; 
	}
      if (!parse_marker(p,name))
	 goto error;
      strcpy(_dxd_gi_var[myGI_numflds]->name, name);
      if (!strcmp(p,"locations"))
	{
	  _dxd_gi_loc_index = myGI_numflds;
	  _dxd_gi_positions = FIELD_PRODUCT;
	}
      ++myGI_numflds;
    } 
  if ( _dxd_gi_numflds == 0 ) 
    _dxd_gi_numflds = myGI_numflds;    /* update number of fields */
  else if ( _dxd_gi_numflds != myGI_numflds ) {
      DXSetError(ERROR_BAD_PARAMETER,"#10840","field");
      goto error;
  }
  return OK;

 error:
  return ERROR;
}


/*
  FN: read the structure of data of each field. 
      If variables have been constructed 
      via DXAllocate then check for compatibility between structure and field.
      Otherwise, the number of structures is the number of fields.
      Do variable construction
  EF: return with _dxd_gi_var[0.._dxd_gi_numflds]->datadim being set.
  Note: the strucutre here is scalar, 2-vector, .... up to 5-vector.
        This can be extended by adding the statement strcmp(p, "?-vector") and
        set the _dxd_gi_var[i]->datadim.
*/

static Error
infoStructure(struct parse_state *ps)
{
  char *p;
  int n=0;
  
  while( FileTok(ps,"=, \n\t",NO_NEWLINE,&p) && p)
    {
      if (n >= max_vars){
         if (!getmorevar(&max_vars))
         goto error;
      }
      if ( _dxd_gi_var[n]==NULL )
	{
	  if ((_dxd_gi_var[n] = (struct variable *)DXAllocate(sizeof(struct variable))) 
	      == NULL )
	      goto error;
	} 
      if (!strcmp(p, "scalar") ) 
	_dxd_gi_var[n]->datadim = 1;
      else if ( !strcmp(p+1, "-vector") ) {
	int k = atoi(p);
	if ((k <= 0) || (k > MAX_POS_DIMS)) {
	  DXSetError(ERROR_NOT_IMPLEMENTED,"#10860","structure",p);
	  goto error; 
	}
	_dxd_gi_var[n]->datadim = k; 
      }
      else if (!strncmp(p,"string",6)){
	int k = atoi(p+7);
	if (k <= 0) {
	   DXSetError(ERROR_BAD_PARAMETER,"#10122","stringlength",1);
	   goto error;
	}
	_dxd_gi_var[n]->datadim = k+1;
      }
      else
	{ 
	  DXSetError(ERROR_BAD_PARAMETER,"#10862","structure",p); 
	  goto error; 
	}
      ++n;
    }
  if (_dxd_gi_numflds == 0 ) 
    _dxd_gi_numflds = n;
  else if (_dxd_gi_numflds != n ) {
      DXSetError(ERROR_BAD_PARAMETER,"#10840","structure");
      goto error;
  }
  return OK;

 error:
  return ERROR;
}

/*
    FN: read in the data format. ( either ascii or binary ).
    EF: Update the data format.
*/

static Error
infoFormat(struct parse_state *ps)
{
  char *p; 

  if (!FileTok(ps,"=, \n\t",NO_NEWLINE,&p)) {
	DXAddMessage("#10865","format");
	goto error;
  }

  if (!p){
	DXSetError(ERROR_DATA_INVALID,"#10865","format");
        goto error;
  }
  if (!get_format(ps,&p,&_dxd_gi_format,&_dxd_gi_byteorder))
     goto error;
  if (_dxd_gi_format < 0){
     DXSetError(ERROR_BAD_PARAMETER,"#10862","format",p);
     goto error;
  }

  return OK;
error:
  return ERROR;
}

/* routine to parse format statement */
static Error
get_format(struct parse_state *ps,char **p,int *format,ByteOrder *order)
{
  char *t;

  t = *p;
  L_CASE(t);
  *p = t;

  /* Parse the byte order if present (ok for it to not be present) */
  if (!strcmp("lsb", *p) ) {
    *format = BINARY;
    *order = BO_LSB;	
  } else if (!strcmp("msb", *p) ) {
    *format = BINARY;
    *order = BO_MSB;
  } else		
    *order = BO_UNKNOWN;

  if (*order != BO_UNKNOWN) {	/* msb/lsb specified, get next token */
    if (!FileTok(ps," ,\n\t",NO_NEWLINE,p)) {
	DXAddMessage("#10865","format");
	goto error;
    }
    if (!*p){
	DXSetError(ERROR_DATA_INVALID,"#10865","format");
        goto error;
    }
  }
 
  if (!strcmp("binary", *p) || !strcmp("ieee",*p)) {
    *format = BINARY;
    if (*order == BO_UNKNOWN)
    	*order = DEFAULT_BYTEORDER; 
  } else if (!strcmp("ascii", *p) || !strcmp("text", *p))  {
    *format = ASCII;
    if (*order != BO_UNKNOWN)
	DXWarning("Byte order in 'format' statement ignored");
  } else 
     *format = -1;

  return OK;

error:
  return ERROR;
}

/*
    FN: read in the datatype of each variable. If the variables haven't been
        constructed, then DXAllocate memory for variables and update the number         of fields. Otherwise do the compatibility checking.
    EF: return the variables with datatype and databytes been set.
*/

static Error 
infoType(struct parse_state *ps)
{
  char *p; 
  int n = 0;
  int bytes;
  Type type;

  while (FileTok(ps," =,\n\t",NO_NEWLINE,&p) && p)
    {
      if (n >= max_vars){
         if (!getmorevar(&max_vars))
         goto error;
      }
      if (_dxd_gi_var[n]==NULL )
	{
	  if ((_dxd_gi_var[n] = (struct variable *)DXAllocate(sizeof(struct variable))) 
	      == NULL )
		goto error;
	}
      if (!get_type(ps,&p,&type,&bytes))
	 goto error;
      _dxd_gi_var[n]->datatype = type;
      _dxd_gi_var[n]->databytes = bytes;
      if (bytes<0)
      {
          DXSetError(ERROR_BAD_PARAMETER,"#10862","type",p);
	  goto error;
      }
      ++n;
    }

  if (_dxd_gi_numflds == 0 ) 
    _dxd_gi_numflds = n;
  else if (_dxd_gi_numflds != n ) {
      DXSetError(ERROR_BAD_PARAMETER,"#10840","type");
      goto error;
  }
  return OK;

 error:
  return ERROR;
}

/* routine for parsing type */
static Error
get_type(struct parse_state *ps,char **p,Type *type,int *bytes)
{
  char signedtype[7];
  int i,found;

     if (!strcmp(*p,"unsigned")){
        if (!FileTok(ps," ,\n\t",NO_NEWLINE,p)&& *p){
          DXAddMessage("#10862","type");
          goto error;
        }
        if (!strcmp(*p,"byte")) strcpy(signedtype,"ubyte");
        else if (!strcmp(*p,"int")) strcpy(signedtype,"uint");
        else if (!strcmp(*p,"short")) strcpy(signedtype,"ushort");
        else{
           DXSetError(ERROR_DATA_INVALID,"#10862","type","unsigned ?");
           goto error;
        }
        *p = signedtype;
     } 
     else if (!strcmp(*p,"signed")){
        if (!FileTok(ps," ,\n\t",NO_NEWLINE,p) && *p){
          DXAddMessage("#10862","type");
          goto error;
        }
        if (!strcmp(*p,"byte")){
           strcpy(signedtype,"sbyte");
           *p = signedtype;
        }
        else if (!strcmp(*p,"int"))  ;
        else if (!strcmp(*p,"short"))  ;
        else{
           DXSetError(ERROR_DATA_INVALID,"#10862","type","signed ?");
           goto error;
        }
     }
    for (found=i=0; type_table[i].type_name !=NULL; i++){
	if (!strcmp(*p,type_table[i].type_name)) {
		*type = type_table[i].type;
		*bytes = DXTypeSize(type_table[i].type); 
		found = 1;
		break;
	}
    }
    if (!found)
       *bytes = -1;

  return OK;

error:
  return ERROR;
}

/*

*/
static Error
infoRecSeparat(struct parse_state *ps)
{
  char *p;
  int n = 0;

  if ((rec_sep =
   (struct header **)DXAllocate(sizeof(struct header*) *max_vars*MAX_POS_DIMS)) == NULL)
     goto error;

  for(n=0; ;n++){
     if (!FileTok(ps,"=, \n\t",NO_NEWLINE,&p)) {
        DXAddMessage("#10862","recordseparator");
        return ERROR;
     }
     if (n > max_vars*MAX_POS_DIMS){
        DXSetError(ERROR_DATA_INVALID,"too many record separators");
        goto error;
     }
     if ((rec_sep[n] = (struct header *)DXAllocate(sizeof(struct header))) 
	      == NULL )
	goto error;
     if (!p){
        break; 
     }
     if (!get_header(ps,p, rec_sep[n])){
        DXAddMessage("reading recordseparator %d",n);
        return ERROR;
     }
    
  }
  rec_sep[n]->type = SKIP_NOTHING;
  if (n == 0) {
     DXSetError(ERROR_DATA_INVALID,"no recordseparators specified");
     goto error;
  }

  return OK;

 error:
  return ERROR;
}



/*
    FN: read in the width (# of bytes) of data of each variable in data file.
    EF: return variable with valid widths.
*/
static Error
infoLayout(struct parse_state *ps)
{
  char *p;
  int n=0;
  
/* Layout is only use for field interleaved data */ 
  if (!PARAM_SPECIFIED(PARAM_Fields))
	_dxd_gi_interleaving = IT_FIELD;

  while (FileTok(ps," =,\n\t",NO_NEWLINE,&p) && p)
    {
      if (n >= max_vars){
         if (!getmorevar(&max_vars))
         goto error;
      }
      if (_dxd_gi_var[n]==NULL )
	{
	  if ((_dxd_gi_var[n] = (struct variable *)DXAllocate(sizeof(struct variable)))
	      == NULL )
	    goto error; 
	}

     
      /* 
       * Get the 'skip' value 
       */
      if (!isdigit( *p ) ) {
	  DXSetError(ERROR_BAD_PARAMETER,"#10870","skip",n+1,"layout");
	  goto error;
      } else
	  _dxd_gi_var[n]->leading = atoi(p);

      /* 
       * Get the 'width' value 
       */
      if (!FileTok(ps,", \t\n",NO_NEWLINE,&p)) {
	  DXAddMessage("\n");
	  DXAddMessage("#10870","width",n+1,"layout");
	  goto error;
      } else if (!p)  {
	  DXSetError(ERROR_MISSING_DATA,"#10871","width",n+1,"layout");
	  goto error;
      } if ( !isdigit( *p ) ) {
          DXSetError(ERROR_BAD_PARAMETER,"#10870","width",n+1,"layout");
	  goto error;
      } else
	  _dxd_gi_var[n]->width = atoi( p );

      ++n;
    }
    
  if (_dxd_gi_numflds == 0 ) 
    _dxd_gi_numflds = n;
  else if (_dxd_gi_numflds != n ) {
      DXSetError(ERROR_BAD_PARAMETER,"#10840","layout");
      goto error;
  }
  return OK;
  
 error:
  return ERROR;
}

/*
    FN: read in the majority for the array ordering. (row/column)
    EF: return with majority being set.
*/

static Error
infoMajority(struct parse_state *ps)
{
  char *p;

  if (!FileTok(ps,"= \n\t",NO_NEWLINE,&p)) {
	DXAddMessage("#10865","majority");
	goto error;
  }
  if (!p){
	DXSetError(ERROR_DATA_INVALID,"#10865","majority"); 
        goto error;
  }

  if ( !strcmp(p, "column") ) 
    _dxd_gi_majority = COL;
  else if ( !strcmp(p, "row") ) 
    _dxd_gi_majority = ROW;
  else
    {
      DXSetError(ERROR_BAD_PARAMETER,"#10862","majority",p);
      goto error;
    }
  return OK;
error:
  return ERROR; 
}
/*
    FN: read in the dependency for each variable (positions/connections)
    EF: return with dependency being set.
*/

static Error
infoDepend(struct parse_state *ps)
{
   char *p;
   int n=0;

   while (FileTok(ps," =,\n\t",NO_NEWLINE,&p) && p)
    {
      if (n >= max_vars){
         if (!getmorevar(&max_vars))
         goto error;
      }
      if (_dxd_gi_var[n]==NULL )
        {
          if ((_dxd_gi_var[n] = (struct variable *)DXAllocate(sizeof(struct variable)))
              == NULL )
                goto error;
        }
        if (!strcmp(p,"connections")) 
                _dxd_gi_var[n]->datadep = DEP_CONNECTIONS;
        else if (!strcmp(p,"positions"))
		_dxd_gi_var[n]->datadep = DEP_POSITIONS;
        else{
          DXSetError(ERROR_BAD_PARAMETER,"#10862","dependency",p);
          goto error;
        }
      ++n;
    }
  if (_dxd_gi_numflds == 0 )
    _dxd_gi_numflds = n;
  else if (_dxd_gi_numflds != n ) {
      DXSetError(ERROR_BAD_PARAMETER,"#10840","dependency");
      goto error;
  }
  return OK;

 error:
  return ERROR;
}

/*
    FN: read in the size of header information.
    EF: return with header BEING SET.
*/
static Error
infoHeader(struct parse_state *ps)
{
  char *p;

  if (!FileTok(ps,"= \n\t",NO_NEWLINE,&p)) {
        DXAddMessage("#10865","header");
        return ERROR;
  }
  if (!p){
        DXSetError(ERROR_DATA_INVALID,"#10865","header");
        return ERROR;
  }

  if (!get_header(ps,p,&_dxd_gi_header)){
     DXAddMessage("reading header statment");
     return ERROR;
  }

   return OK;
}

/* 
   FN: parse header (header, series header, record header)
   EF: return header set
*/
static Error
get_header(struct parse_state *ps, char *p, struct header *head)
{

  if (!strcmp(p,"lines"))  {
	head->type = SKIP_LINES;
        if (!FileTok(ps,", \n\t",NO_NEWLINE,&p)) {
	    DXAddMessage("#10865","lines");
	    goto error;
  	}
        if (!p){
	    DXSetError(ERROR_DATA_INVALID,"#10865","lines");
            goto error;
        }
	if (!isdigit(*p)){
	    DXSetError(ERROR_DATA_INVALID,"#10873","lines");
            goto error;
        }
  	head->size = atoi(p);
        head->marker[0] = '\0';
  } else if (!strcmp(p,"bytes"))  {
	head->type = SKIP_BYTES;
        if (!FileTok(ps,", \n\t",NO_NEWLINE,&p)) {
	    DXAddMessage("#10865","bytes");
	    goto error;
  	}
        if (!p){
	    DXSetError(ERROR_DATA_INVALID,"#10865","bytes");
            goto error;
        }
	if (!isdigit(*p)){
	    DXSetError(ERROR_DATA_INVALID,"#10873","bytes");
            goto error;
        }
  	head->size = atoi(p);
        head->marker[0] = '\0';
  } else if (!strcmp(p,"marker"))  {
	head->type = SKIP_MARKER;
        if (!FileTok(ps," ,\n",NO_NEWLINE,&p)) {
	    DXAddMessage("#10865","marker");
	    goto error;
  	}
	if (!p){ 
	     DXSetError(ERROR_DATA_INVALID,"#10865","marker");
             goto error;
        }
	if (!parse_marker(p,head->marker))
	    goto error;
	head->size = 0;
  } else { 
	DXSetError(ERROR_DATA_INVALID,"#10862","header",p);
	goto error;
  }

  return OK;

error:
  return ERROR;
}


/*
  FN: read in the interleaving specification.
  EF: return with interleaving being set.  
  */
static Error
infoInterleaving(struct parse_state *ps)
{
  char *p;

  if (!FileTok(ps,"= \t\n",NO_NEWLINE,&p)) {
    DXAddMessage("#10865","interleaving");
    goto error;
  }

  if (!p){ 
    DXSetError(ERROR_DATA_INVALID,"10865","interleaving");
    goto error;
  }

  if     ( !strcmp(p, "field") )          
    _dxd_gi_interleaving = IT_FIELD;
  else if ( !strcmp(p, "record") )         
    _dxd_gi_interleaving = IT_RECORD;
  else if ( !strcmp(p, "vector") )         
    _dxd_gi_interleaving = IT_VECTOR;
  else if ( !strcmp(p, "record-vector") )  
    _dxd_gi_interleaving = IT_RECORD_VECTOR;
  else if ( !strcmp(p, "series-vector") )         
    _dxd_gi_interleaving = IT_VECTOR;
  else
    {
	DXSetError(ERROR_DATA_INVALID,"#10862","interleaving",p);
	goto error;
    }
  return OK;

 error:
  return ERROR;
}

/*
    FN: read in the leading blanks per line and number of elements per line of
        each field.
    EF: return variables with valid leading blanks and elements.
*/
static Error
infoBlock(struct parse_state *ps)
{
  char *p;
  int n=0;
  
  while (FileTok(ps,"=, \t\n",NO_NEWLINE,&p) && p)
    {
      if (n >= max_vars){
         if (!getmorevar(&max_vars))
         goto error;
      }
      if ( _dxd_gi_var[n]==NULL )
	{
	  if ((_dxd_gi_var[n] = (struct variable *)DXAllocate(sizeof(struct variable))) 
	     == NULL )
              goto error; 
	}

      /* 
       * Get the 'skip' value 
       */
      if ( !isdigit( *p ) ) {
	  DXSetError(ERROR_BAD_PARAMETER,"#10870","skip",n+1,"block");
	  goto error;
      } else
	  _dxd_gi_var[n]->leading = atoi(p);
      
      /* 
       * Get the 'elements' value 
       */
      if (!FileTok(ps,", \t\n",NO_NEWLINE,&p)) {
	  DXAddMessage("\n");
	  DXAddMessage("#10871","elements",n+1);
	  goto error;
      } else if (!p) {
	  DXSetError(ERROR_MISSING_DATA,"#10871","elements",n+1,"block");
	  goto error;
      } else if ( !isdigit( *p ) ) {
	  DXSetError(ERROR_BAD_PARAMETER,"#10870","elements",n+1,"block"); 	
	  goto error;
      } else
	    _dxd_gi_var[n]->elements = atoi( p );

      /* 
       * Get the 'width' value 
       */
      if (!FileTok(ps,", \t\n",NO_NEWLINE,&p))  {
	  DXAddMessage("\n");
	  DXAddMessage("#10871","width",n+1,"block");
	  goto error;
      } else if (!p)  {
	  DXSetError(ERROR_MISSING_DATA,"#10871","width",n+1,"block");
	  goto error;
      } if ( !isdigit( *p ) ) {
          DXSetError(ERROR_BAD_PARAMETER,"#10870","width",n+1,"block");
	  goto error;
      } else
	    _dxd_gi_var[n]->width = atoi( p );
      ++n;
    }

  if ( _dxd_gi_numflds == 0 ) 
    _dxd_gi_numflds = n;
  else if ( _dxd_gi_numflds != n ) {
      DXSetError(ERROR_BAD_PARAMETER,"#10840","block");
      goto error;
  }
  return OK;
  
 error:
  return ERROR;
}

/*
  FN: read in the locations data from infomation table to _dxd_gi_dataloc[]. construct
      locations variable _dxd_gi_var[locidx] when positions non-PRODUCT. otherwise
      construct variables _dxd_gi_var[locidx..locidx+_dxd_gi_numdims-1] for every axis.
  EF: return with valid locations variable(s) and update locidx.
*/

static Error
infoPositions(struct parse_state *ps,struct place_state *dataps)
{
  char *p;
  int n = 0, size, i=0, posnums, tempi1, tempi2;
  int istart=0,max_values=0;
  Type type = TYPE_FLOAT;
  int format = ASCII;
  ByteOrder order = DEFAULT_BYTEORDER;

  if (_dxd_gi_positions != DEFAULT_PRODUCT)
    {
      DXWarning("#4460");
      return OK;
    }
    
  if (!PARAM_SPECIFIED(PARAM_Points) && !PARAM_SPECIFIED(PARAM_Grid)){ 
    DXSetError(ERROR_DATA_INVALID,"#10875","points","grid","positions");
    goto error;
  }

  /* _dxd_gi_dataloc is freed in genimpcleanup() */
  
  /* Get first token, expect it to be a number or 'regular' or 'irregular'
   * or a marker indicating the positions are to be read from the data file
   */
  if (!FileTok(ps,"=, \t\n",NO_NEWLINE,&p))  {
	DXAddMessage("#10865","positions");
	goto error;
  }

  if (!p) {
	DXSetError(ERROR_MISSING_DATA,"#10865","positions");
	goto error;
  }

  /* if positions from file, fill _dxd_gi_fromfile */
  if (!isdigit(*p) && *p != '.' && *p != '-' &&
	     strncmp(p,"reg",3) && strncmp(p,"irr",3)){
    if (_dxd_gi_fromfile){
       for (i=0; _dxd_gi_fromfile[i]; i++)
	  ;
    }
    else if ((_dxd_gi_fromfile =(struct infoplace **)
            DXAllocate(sizeof(struct infoplace *) * MAX_POS_DIMS*3))== NULL)
       goto error;
    /* is the format and/or the type specified */
    if (!readdesc(&p,ps,&format,&order,&type,"positions"))
       goto error;
    istart = i;
    for (i=0; i<MAX_POS_DIMS*2; i++){
	 _dxd_gi_fromfile[istart+i+1]=NULL;
         if ((_dxd_gi_fromfile[istart+i] =(struct infoplace *)DXAllocate(
	      sizeof(struct infoplace)))== NULL)
            goto error;
	 _dxd_gi_fromfile[istart+i]->type = type;
	 _dxd_gi_fromfile[istart+i]->format = format;
	 _dxd_gi_fromfile[istart+i]->byteorder = order;
	 _dxd_gi_fromfile[istart+i]->index = 10+i;
         if (!readplacement(&p,ps,(istart+i),&max_values,"positions"))
	    goto error;
	 if (!p)
	    break;
    }
    _dxd_gi_fromfile[++i+istart]=NULL;
    _dxd_gi_positions = REGULAR_PRODUCT;
    posnums = MAX_POS_DIMS*2;
    if ((_dxd_gi_dataloc = (float *)DXAllocate(posnums*sizeof(float)))==NULL)
	goto error; 
    return OK;
  }

  /* if positions not from file but grid or points are, parse_datafile now */
  if (_dxd_gi_fromfile && !parse_datafile(dataps))
     goto error;

  size = _dxfnumGridPoints(-1);

  if (isalpha(*p)) 
    {
      posnums = 0;
      for (i=0; i<_dxd_gi_numdims; i++)
	{
	  if ( !strncmp(p,"regular",7) )
	    { 
	      _dxd_gi_pos_type[i] = REGULAR;
	      posnums += 2;
	    }
	  else if ( !strncmp(p,"irregular",9) ) 
	    {
	      _dxd_gi_pos_type[i] = IRREGULAR;
	      posnums += _dxd_gi_dimsize[i];
	    }
	  else
	    {
	      DXSetError(ERROR_BAD_PARAMETER,"#10862","positions",p);
	      goto error;
	    }
	  
	  if (!FileTok(ps,", \t\n",NO_NEWLINE,&p)) 
	    {
	      if (i == _dxd_gi_numdims-1)  /* Should be reading the numbers */
		DXAddMessage("#10877","positions");
	      else			/* Should be read a position type */
		DXAddMessage("#10878","positions");
	      goto error;
	    }
	  
	  if (!p) 
	    {
	      if (i == _dxd_gi_numdims-1)  /* Should be reading the numbers */
		DXSetError(ERROR_MISSING_DATA,"#10877","positions");
	      else
		DXSetError(ERROR_MISSING_DATA,"#10878","positions");
	      goto error;
	    }
	}
      n = 0;
      if ((_dxd_gi_dataloc = (float *)DXAllocate(posnums*_dxd_gi_numdims*sizeof(float)))==NULL)
	goto error; 
      do 
	{	/* DXRead the numbers */
	  if (!isdigit(*p) && *p != '.' && *p != '-')
	    {
	      if (!strcmp(p,"end")) 
		{
		  /* This is a recognized and handled keyword */
		  SET_PARAM_SPECIFIED(PARAM_End);
		  break;	
		} 
	      else if (*p == '#') 	/* A comment */ 
		{
		  break;
		}
	      else 
		{
		  DXSetError(ERROR_DATA_INVALID,"#10879",n+1,"positions",p);
		  goto error;
		}
	    }
	  if (n>=posnums){
	    DXSetError(ERROR_BAD_PARAMETER,"#10880","positions");
            goto error;
          }
	  _dxd_gi_dataloc[n++] = atof(p);
	} 
      while (FileTok(ps,", \t\n",NEWLINE_OK,&p) && p);
      DXResetError();		/* EOF is ok here */
      if (posnums != n){
	DXSetError(ERROR_BAD_PARAMETER,"#10880","positions");
        goto error;
      }
      if (_dxd_gi_numdims == 1)  
	{
	  if (_dxd_gi_pos_type[0] == IRREGULAR)
	    _dxd_gi_positions = IRREGULAR_PRODUCT;
	  else  
	    _dxd_gi_positions = REGULAR_PRODUCT;
	} 
      else
	_dxd_gi_positions = MIXED_PRODUCT;
    } 
  else
    {	/* Parse a string of numbers that can span more than 1 line */
      n = 0;
      tempi1 = (2*_dxd_gi_numdims);
      if (size==1) tempi2 = 2*_dxd_gi_numdims;
      else tempi2 = size*_dxd_gi_numdims;
      if ((_dxd_gi_dataloc = (float *)DXAllocate(2*_dxd_gi_numdims*sizeof(float)))==NULL)
	goto error; 
      do 
	{	/* DXRead the numbers */
	  if (!isdigit(*p) && *p != '.' && *p != '-') 
	    {
	      if (!strcmp(p,"end")) 
		{
		  /* This is a recognized and handled keyword */
		  SET_PARAM_SPECIFIED(PARAM_End);
		  break;	
		} 
	      else if (*p == '#') 	/* A comment */ 
		{
		  break;
		}
	      else
		{
		  DXSetError(ERROR_DATA_INVALID,"#10879",n+1,"positions",p);
		  goto error;
		}
	    }
	  if (n == tempi1)
	    {
	      if((_dxd_gi_dataloc = (float *)DXReAllocate((Pointer)_dxd_gi_dataloc,
						size*_dxd_gi_numdims*sizeof(float)))
		 ==NULL)
		goto error; 
	    }
	  if (n >= tempi2){
	    DXSetError(ERROR_BAD_PARAMETER,"#10880","positions");
            goto error;
          }
	  _dxd_gi_dataloc[n++] = atof(p);
	}
      while (FileTok(ps,", \t\n",NEWLINE_OK,&p) && p);
      DXResetError();		/* EOF is ok here */
      if (size*_dxd_gi_numdims == n )
	_dxd_gi_positions = IRREGULAR_PRODUCT;
      else if ( 2*_dxd_gi_numdims == n)
	_dxd_gi_positions = REGULAR_PRODUCT;
      else{
	DXSetError(ERROR_BAD_PARAMETER,"#10880","positions");
        goto error;
      }
	  
    }
  return OK;

 error:
  return ERROR;

}

/*
    FN: read in data file name and construct output DX file name.
    EF: return with valid data filename and output DX file name.
*/

static Error
infoFile(struct parse_state *ps)
{
  char *current = ps->current;
  char *p;

  /* search for = and skip it, then filename can contain = */
  /* Skip any leading white space and '=' */
  while (*current && (*current == ' ' || *current == '\t' || *current == '='))
	current++;
  if (!*current) goto error;
  ps->current = current;
   
  if (!FileTok(ps," ,\t\n",NO_NEWLINE,&p)) {
	DXAddMessage("#10865","file");
	goto error;
  }

  if (!p){ 
	DXSetError(ERROR_MISSING_DATA,"#10865","file");
        goto error;
  }
 
  _dxd_gi_filename = (char *)DXAllocate(strlen(p) +1);
  strcpy(_dxd_gi_filename, p);    
  return OK;
error:
  return ERROR;
}

/*
  FN: read in dimensionality and shape.
  EF: return with valid _dxd_gi_numdims and _dxd_gi_dimsize[].
      and set _dxd_gi_connections.
  */

static Error
infoPoints(struct parse_state *ps)
{
  char *p;
  int max_values = 0;
  Type type=TYPE_INT;
  int format=ASCII;
  ByteOrder order = DEFAULT_BYTEORDER;

  if (_dxd_gi_connections != CONN_UNKNOWN){ 
	DXSetError(ERROR_DATA_INVALID,"#10882","points","grid");
        goto error;
  }

  if (!FileTok(ps,"= ,\t\n",NO_NEWLINE,&p)) {
	DXAddMessage("#10865","points");
	goto error;
  }
  if (!p){ 
	DXSetError(ERROR_MISSING_DATA,"#10865","points");
        goto error;
  }

  if (isdigit(*p))
    {
      _dxd_gi_dimsize[0] = atoi( p );
      if (_dxd_gi_dimsize[0] == 0) {
	    DXSetError(ERROR_DATA_INVALID,"#10020",
		"number of points specified in 'points' statement");
	    goto error;
      }
    }
  else {		/* points read from file */
    if (!_dxd_gi_filename){
	DXSetError(ERROR_DATA_INVALID,"'file' must come before 'points'");
	goto error;
    }
    if (!_dxd_gi_fromfile && (_dxd_gi_fromfile = (struct infoplace **)
        DXAllocate(sizeof(struct infoplace *) * MAX_POS_DIMS*3))==NULL)
	goto error;
    if ((_dxd_gi_fromfile[0] = (struct infoplace *)DXAllocate(
	 sizeof(struct infoplace)))== NULL)
       goto error;
     /* is the format and/or the type specified */
     if (!readdesc(&p,ps,&format,&order,&type,"points"))
	goto error;
    _dxd_gi_fromfile[1]=NULL;
    _dxd_gi_fromfile[0]->type = type;
    _dxd_gi_fromfile[0]->format = format;
    _dxd_gi_fromfile[0]->byteorder = order;
    _dxd_gi_fromfile[0]->index = 0;
    if (!readplacement(&p,ps,0,&max_values,"points"))
	goto error;
  }
  _dxd_gi_numdims = 1; 

  _dxd_gi_connections = CONN_POINTS;
  return OK;

error:
   return ERROR;
}
/*
  FN: read in dimensionality and shape.
  EF: return with valid _dxd_gi_numdims and _dxd_gi_dimsize[].
      and set _dxd_gi_connections.
  */

static Error
infoGrid(struct parse_state *ps)
{
  char *p;
  int i=0,max_values=0;
  Type type=TYPE_INT;
  int format=ASCII;
  ByteOrder order = DEFAULT_BYTEORDER;

  _dxd_gi_numdims=0;
  if (_dxd_gi_connections != CONN_UNKNOWN){ 
	DXSetError(ERROR_DATA_INVALID,"#10882","grid","points");
        goto error;
  }
  /* check first character, if alpha data is read from file */
  if (!FileTok(ps,"=xX, \t\n",NO_NEWLINE,&p)) {
     DXAddMessage("\nGrid statement is incorrect");	
     goto error;
  }
  if (!p){
     DXSetError(ERROR_DATA_INVALID,"grid statement is empty");
     goto error;
  }

  if (isdigit(*p)){
     for (i=0 ; i<MAX_POS_DIMS; i++) {
        _dxd_gi_dimsize[i] = atoi( p );
        _dxd_gi_numdims++;
        if (!FileTok(ps,"xX \t\n",NO_NEWLINE,&p)) {
	   DXAddMessage("\nGrid statement is incorrect");	
	   goto error;
        }
        if (!p) 
	   break;
     }
  }
  else {		/* grid dimensions are contained in the data file */
     if (!_dxd_gi_fromfile && (_dxd_gi_fromfile =(struct infoplace **)
      DXAllocate(sizeof(struct infoplace *) * MAX_POS_DIMS*3))== NULL)
         goto error;
     
     /* is the format and/or the type specified */
     if (!readdesc(&p,ps,&format,&order,&type,"grid"))
	goto error;
     for (i=0; i<MAX_POS_DIMS; i++) {
         _dxd_gi_fromfile[i+1]=NULL;
	 if ((_dxd_gi_fromfile[i] =(struct infoplace *)DXAllocate(
	      sizeof(struct infoplace)))== NULL)
            goto error;
	 _dxd_gi_fromfile[i]->type = type;
	 _dxd_gi_fromfile[i]->format = format;
	 _dxd_gi_fromfile[i]->byteorder = order;
	 _dxd_gi_fromfile[i]->index = i;
	 if (!readplacement(&p,ps,i,&max_values,"grid"))
	    goto error;
	 if (!p)
	    break;
         if (!FileTok(ps,"xX, \t\n",NO_NEWLINE,&p)) {
	    DXAddMessage("\nGrid statement is incorrect");	
	    goto error;
         }
	 if (!p)
	    break;
     }
     _dxd_gi_fromfile[++i]=NULL;
  } 

  _dxd_gi_connections = CONN_GRID;
  return OK;

error:
   return ERROR;
}


/*
  FN: read in the number of time series (records).
  */
static Error
infoSeries(struct parse_state *ps)
{
  char *p;
  int i = 0;
  float start = 0.0, delta = 0.0;
  while (FileTok(ps,"=, \t\n",NO_NEWLINE,&p) && p)
    {
      if (isalpha(*p)){
         if (i<0){
 	    DXSetError(ERROR_DATA_INVALID,"#10885","series separator");
	    goto error;
         } 
         if (!strncmp(p,"separator",9)){
            if (!FileTok(ps,"= \n\t",NO_NEWLINE,&p)) {
               DXAddMessage("#10865","series");
               goto error;
            }
            if (!p){
               DXSetError(ERROR_DATA_INVALID,"#10865","series separator");
               goto error;
            }

            if (!get_header(ps,p,&_dxd_gi_serseparat)){
               DXAddMessage("reading series separator");
               goto error;
            }
         }
         else{
            DXSetError(ERROR_DATA_INVALID,"#10862","series");
            goto error;
         }
         if (FileTok(ps," \n\t",NO_NEWLINE,&p) && p){
            DXSetError(ERROR_DATA_INVALID,"#10885","series separator");
            goto error;
         }
         break;
      }
      else{
         i++;
         if (i == 1)
	   _dxd_gi_series = atoi( p );
         else if (i == 2)
	   start = atof( p );
         else if (i == 3)
	   delta = atof( p );
      }
    }

  if (i == 0) {
      DXSetError(ERROR_MISSING_DATA,"#10865","series");
      goto error;
  } else if (i == 1) {
      start = 1.0;
      delta = 1.0;
  } else if (i == 2) {
      delta = 1.0;
  } 
  
  if (!(_dxd_gi_seriesvalues = (float *)DXAllocate(_dxd_gi_series*sizeof(float))))
      goto error;
  
  _dxd_gi_seriesvalues[0] = start;
  for (i=1; i<_dxd_gi_series; i++)
    _dxd_gi_seriesvalues[i] = _dxd_gi_seriesvalues[i-1] + delta;
  
  return OK;
  
 error:
  return ERROR;
}

/*
  FN: do nothing but catch the 'end' keyword. 
  */
static Error
infoEnd(struct parse_state *ps)
{
	return OK;
}

static Error 
parse_marker(char *p, char marker[MAX_MARKER])
{
    int i; 
    unsigned int c;

    /* Skip any leading white space */
    while (*p && (*p == ' ' || *p == '\t')){
	p++;
    }
    if (!*p) goto error;

    marker[0] = '\0';
    if (*p == '"') { /* DXCopy everything inside of the two "'s */
	p++; 
	for (i=0 ; (i<MAX_MARKER) && *p && (*p != '"') ; i++) {
	    if (!nextchar(&p, &c))
		goto error;
	    marker[i] = (char)c;
	    p++; 
	}
	if ((i<MAX_MARKER) && (*p != '"')){
	    DXSetError(ERROR_DATA_INVALID,"#10886","marker");
            goto error;
        }
        p++;
	    
    } else {	/* DXCopy everything until the first white space or , */
	for (i=0 ; 
	    (i<MAX_MARKER) && *p && (*p != ' ') && (*p != '\t') && (*p != ','); 
	    i++) { 
	    if (!nextchar(&p, &c))
		goto error;
	    marker[i] = (char)c;
	    p++; 
	}
    } 	
    if (i>=MAX_MARKER){ 
	DXSetError(ERROR_DATA_INVALID,"#10887","marker");
        goto error;
    }
    marker[i] = '\0';

    return OK;

error:
    return ERROR;
}

static Error
nextchar(char **p, unsigned int *c)
{
    unsigned int i, digit, num;

    if (**p == '\\') {	
	(*p)++;
	switch (**p) {
	case 'n': *c = '\n'; break;		/* '\n'  */ 
	case 't': *c = '\t'; break;		/* '\t'  */ 
	case 'r': *c = '\r'; break;		/* '\r'  */ 
	case 'f': *c = '\f'; break;		/* '\r'  */ 
	case 'b': *c = '\b'; break;		/* '\b'  */ 
	default:
	    if (isdigit(**p)) {
		/* DXExtract an 1, 2 or 3 digit octal number */
		num = i = 0; (*p)--;	/* Prime the loop */
		do {
		    (*p)++;
		    digit = **p - '0';
		    if (digit >= 8)   {
			DXSetError(ERROR_DATA_INVALID,"#10890",**p); 
			goto error;
		    }
		    num = num*8 + **p - '0';
		} while ((++i < 3) && isdigit(*(*p+1)));
		if (num > 0xff) {
			DXSetError(ERROR_DATA_INVALID,"10891",num); 
			goto error;
		}
		*c = num; 
	    } else {
		    *c = (int)**p;
	    }
	}
    } else 
	*c = (int)**p;

    return OK;

error:
    return ERROR;
}


/* parse the template if it exists the template is header and
 * file is data file. Set filename.
 */
static Error
parse_template(char *format, char **filename)
{
   char *file;
   char *p,*pstr,*pstart,*pend,*pcomma;
   int i,length;
   struct parse_state ps;
   char *statement;

   if (!format)
      return OK;

   /* Skip "general" */
   format+=7;

   /* Skip any leading white space */
   while (*format && (*format == ' ' || *format == '\t' || *format == ',')){
       format++;
   }
   if (!*format) return OK; /* just space, no template */

   /* set data filename */
   SET_PARAM_SPECIFIED(PARAM_File);
   _dxd_gi_filename = (char *)DXAllocate(strlen(*filename) +1);
   strcpy(_dxd_gi_filename,*filename);

   ps.line=NULL;
   if (!strncmp(format,"template",8)){
      if ((p = strchr(format,'=')) ==NULL){
         DXSetError(ERROR_DATA_INVALID,"#10895","template");
         goto error;
      } 
      p++;
      while (*p && (*p == ' ' || *p == '\t' || *p == ',')){
         p++;
      }
      if (p){		/* get header filename length */
	 i=0;
	 file = p;
         while (*p && *p != ' ' && *p != '\t' && *p != ','){
	    p++;
	    i++;
	 }
      }
      else{
         DXSetError(ERROR_BAD_PARAMETER,"#10865","template");
         goto error;
      }
      if (i > strlen(*filename)){
	 DXFree(*filename);
	 *filename = (char *)DXAllocate(i+1);
      }
      strncpy(*filename,file,i);
      (*filename)[i]='\0';
      /* skip to next statement */
      while (*p && (*p == ' ' || *p == '\t' || *p == ',')){
         p++;
      }
   }
   else{
      /* no template name is given 
       * all header info is given in format string from Import
       */
      *filename[0] = '\0';
      p = format;
      /* skip to next statement */
      while (*p && (*p == ' ' || *p == '\t' || *p == ',')){
         p++;
      }
   }

   if (*p){
   
   /* initialize ps without a filepointer to read the template string */
    ps.filename = NULL;
    ps.fp = NULL;
    ps.current = NULL;
    ps.lineno = 0;
    ps.line = (char *)DXAllocate(MAX_DSTR);
    if (!ps.line)
       goto error;

   /* copy each statement into ps.line (which is allocated already) */
   /* then current points to beginning of ps.line */
   pstart = p;
   pend = pstart;

   while(pstart){
   *ps.line = '\0';
   pend = pstart;
   statement = ps.line;
   while(!*ps.line){
   if ((pend = strchr(pend, ','))==NULL){
      /* last statement */
      length = strlen(pstart);
      if (length < MAX_DSTR-2){
         /* strncpy(statement,pstart,length);
         statement[length]=' ';
         statement[length+1]='\0';
         ps.line = statement;
	 */
         strncpy(ps.line,pstart,length);
         ps.line[length]=' ';
         ps.line[length+1]='\0';
      }
      else{
          DXSetError(ERROR_INTERNAL,"statement too long");
          goto error;
      }
   }
   else{
      /* is this a keyword */
      pcomma = pend;  	/* mark comma so can replace with space */
      pend++;
      while(*pend && (*pend == ' ' || *pend == '\t'))
	 pend++;
      for (i=0; i<NUM_PARS; i++){
	 if (!strncmp(para[i].name,pend,strlen(para[i].name))){
	    /* special case positions which can be in dependency field */
	    if (i==PARAM_Positions){
	      if (!strchr(pend,'=') || (strchr(pend,',') < strchr(pend,'=')))
		 continue;
	    }
	    length = pend-pstart;
	    if (length < MAX_DSTR-1){
	       strncpy(statement,pstart,length);
	       statement[length - (pend - pcomma)] = ' ';
	       statement[length-1]=' ';
	       statement[length]='\0';
	       ps.line = statement;
	    }
	    else{
	       DXSetError(ERROR_INTERNAL,"statement too long");
	       goto error;
	    }
	    break;
	 }
      }
   }
   }
   ps.current = ps.line;
   if (FileTok(&ps," =,\n\t",NO_NEWLINE,&pstr)){
      for(i=0; i<NUM_PARS ; i++) {
	 if (!strcmp(para[i].name, pstr)) {
	    if (PARAM_SPECIFIED(i)) {
		 DXWarning("Ignoring extra '%s' statement in format string",
			    para[i].name);
                 i = NUM_PARS; /* Exit for loop */
	     } else {
		if(!(* para[i].getInfo)(&ps))
		    goto error;
		SET_PARAM_SPECIFIED(i);
		i = NUM_PARS; 	/* Exit for loop */
	     }
	 }
      }
      if (i == NUM_PARS) {
	 DXSetError(ERROR_BAD_PARAMETER,"#10841",pstr);
	 goto error;
      }
   }
   pstart =pend;
   } 
   DXFree(ps.line);

   }
 
   return OK;

error:
   if (ps.line) DXFree(ps.line);
   return ERROR;
}

/* get the format, byteorder, and type of the data read from data file 
 * one for each statement
 */
static Error
readdesc(char **p,struct parse_state *ps,int *format,ByteOrder *order,
	 Type *type, char *statement)
{
   int bytes,form;
   ByteOrder or;

   if (!get_format(ps,p,&form,&or))
      goto error;
   if (form >= 0){ 	/* format was specified */
      *format = form;
      *order = or;
      if (!FileTok(ps," ,\t\n",NO_NEWLINE,p)){
        DXAddMessage("reading %s statement",statement);
        goto error;
      }
   }
      
   if (!get_type(ps,p,type,&bytes))
      goto error;
   if(bytes >= 0 && !FileTok(ps," ,\t\n",NO_NEWLINE,p)){
     DXAddMessage("reading %s statement",statement);
     goto error;
   }

   return OK;
error:
   return ERROR;
}

/* fill one infoplace struct */
static Error
readplacement(char **p,struct parse_state *ps,int which, int *max_values,
	      char *statement)
{
   int n=0,bytes;
   struct infoplace *fromfile;
   Type t;

   fromfile = _dxd_gi_fromfile[which];
   t = fromfile->type;
   if ((fromfile->skip = (int *)DXAllocate(MAX_POS_DIMS *sizeof(int)))== NULL)
      goto error;
   if ((fromfile->width = (int *)DXAllocate(MAX_POS_DIMS *sizeof(int)))== NULL)
      goto error;
   bytes = DXTypeSize(t);
   if ((fromfile->data = (void *)DXAllocate(MAX_POS_DIMS *bytes))==NULL)
      goto error;

   /* if no data in the file use the default 
    * for positions only */
   if (**p == '?'){
      fromfile->begin.type=SKIP_NOTHING;
      fromfile->skip[0]=0;
      n=1;
      if (!FileTok(ps,", \t\n",NO_NEWLINE,p)) 		/* advance the fp */
	 goto error;
      goto done;
   }

   if (!get_header(ps,*p,&fromfile->begin)){
      DXAddMessage("reading %s statment",statement);
      goto error;
   }
   while (FileTok(ps," ,\t\n",NO_NEWLINE,p) && (*p) && isdigit(**p)){
   fromfile->skip[n] = atoi(*p);	/* get the 'skip' value */
   if (!FileTok(ps,", \t\n",NO_NEWLINE,p)) {
      DXAddMessage("\n");
      DXAddMessage("#10870","width",which,statement);
      goto error;
   }
   if (!*p)  {
      DXSetError(ERROR_MISSING_DATA,"#10871","width",which,statement);
      goto error;
   } 
   if ( !isdigit( **p ) ) {
      DXSetError(ERROR_BAD_PARAMETER,"#10870","width",which,statement);
      goto error;
   }
   fromfile->width[n] = atoi( *p );
   n++;
   }

done:
   fromfile->skip[n]=-1;
   if (n==0)		/* no skip info (how many values?) */
	 *max_values+=MAX_POS_DIMS;
   else 
      *max_values+=n;
   
   return OK;
error:
   return ERROR;
}

/* read header info from data file and place in a correct place */
static Error
parse_datafile(struct place_state *dataps)
{
   int i,j=0,k=0,num;
   int read_header;
   float f=0;

   for (i=0; _dxd_gi_fromfile[i]; i++){
      for (num=0; _dxd_gi_fromfile[i]->skip[num]>0; num++)
	 ;
      if (num==0) num=MAX_POS_DIMS;
      /*max_size =+ DXTypeSize(_dxd_gi_fromfile[i]->type) * num;*/
   }

   dataps->lines = 0;
   dataps->bytes = 0;
   if (!_dxf_gi_extract(dataps,&read_header)){
      DXAddMessage("reading header information from data file");
      goto error;
   }

   /* if header statement exists adjust accordingly */
   if (read_header==1){		/* header marker was already read */
      DXWarning("header info not before data, cannot pipe");
      if (_dxd_gi_filename[0]=='!'){
         DXSetError(ERROR_DATA_INVALID,
         "can't use filter if header info is after begining of data");
          goto error;
      }
      rewind(dataps->fp);
   }

   if (_dxd_gi_header.type!=SKIP_NOTHING){
      switch (_dxd_gi_header.type){
      case(SKIP_LINES):
	 if (_dxd_gi_header.size < dataps->lines){
	    DXWarning("header info not before data, cannot pipe");
	    if (_dxd_gi_filename[0]=='!'){
	       DXSetError(ERROR_DATA_INVALID,
	       "can't use filter if header info is after begining of data");
	       goto error;
	    }
	    rewind(dataps->fp);
	 }
	 else
	    _dxd_gi_header.size = _dxd_gi_header.size-dataps->lines;
	 break;
      case(SKIP_BYTES):
	 if (_dxd_gi_header.size < dataps->bytes){
	    DXWarning("header info not before data, cannot pipe");
	    if (_dxd_gi_filename[0]=='!'){
	       DXSetError(ERROR_DATA_INVALID,
	       "can't use filter if header info is after begining of data");
	       goto error;
	    }
	    rewind(dataps->fp);
	 }
	 else
	    _dxd_gi_header.size = _dxd_gi_header.size-dataps->bytes;
	 break;
      default:
         break;
      }
   }

   /* extract data from structure and place in correct place */
   for (i=0; _dxd_gi_fromfile[i]; i++){
      if (_dxd_gi_fromfile[i]->begin.type==SKIP_NOTHING){
	 if ((float)(int)(k/2.0) == (float)k/2.0) _dxd_gi_dataloc[k++]=0.0;
	 else _dxd_gi_dataloc[k++]=1.0;
      }
      else{

      /*t = _dxd_gi_fromfile[i]->type;*/
      for (num=0; _dxd_gi_fromfile[i]->skip[num]>=0; num++){
	 switch(_dxd_gi_fromfile[i]->type){
	 case(TYPE_UBYTE):
	    f = (float)DREF(ubyte,_dxd_gi_fromfile[i]->data,num);
	    break;
	 case(TYPE_BYTE):
	    f = (float)DREF(byte,_dxd_gi_fromfile[i]->data,num);
	    break;
	 case(TYPE_USHORT):
	    f = (float)DREF(ushort,_dxd_gi_fromfile[i]->data,num);
	    break;
	 case(TYPE_SHORT):
	    f = (float)DREF(short,_dxd_gi_fromfile[i]->data,num);
	    break;
	 case(TYPE_UINT):
	    f = (float)DREF(uint,_dxd_gi_fromfile[i]->data,num);
	    break;
	 case(TYPE_INT):
	    f = (float)DREF(int,_dxd_gi_fromfile[i]->data,num);
	    break;
	 case(TYPE_FLOAT):
	    f = (float)DREF(float,_dxd_gi_fromfile[i]->data,num);
	    break;
	 case(TYPE_DOUBLE):
	    f = (float)DREF(double,_dxd_gi_fromfile[i]->data,num);
	    break;
	 default:
	    break;
         } 
	 if (_dxd_gi_fromfile[i]->index < 10) 	/* for grid or points */
	    _dxd_gi_dimsize[j++]= (int)f;
         else		/* for positions */
	    _dxd_gi_dataloc[k++]=f;
      }
      }
   }
   if (j>0) _dxd_gi_numdims = j;

   /* clean up _dxd_gi_fromfile, done using this */
   for (i=0; _dxd_gi_fromfile[i]; i++){
      DXFree((Pointer)_dxd_gi_fromfile[i]->skip);
      DXFree((Pointer)_dxd_gi_fromfile[i]->width);
      DXFree((Pointer)_dxd_gi_fromfile[i]->data);
      DXFree((Pointer)_dxd_gi_fromfile[i]);
   }
   DXFree(_dxd_gi_fromfile);
   _dxd_gi_fromfile=NULL;

   return OK;

error:
   return ERROR;
}


/*
 * Initialized a parse_state variable for use with StrTok().
 * File is assumed to already be open with  file pointer fp.
 */
static Error
InitFileTok(struct parse_state *ps, FILE *fp, char *filename)
{
    ps->filename = filename;
    ps->current = NULL;
    ps->fp = fp;
    ps->lineno = 0;
    ps->line = (char *)DXAllocate(MAX_DSTR);
    return OK;
}

/*
 * Operate similar to C lib strtok(), but allows the user  to specify
 * whether a new line should be read to find the next token.
 * ERROR/OK is returned with error code set and *token is returned
 * as NULL (if no token found) or as a pointer to string containing
 * the token.
 */
static Error
FileTok(struct parse_state *ps,char *sep, int newline, char** token)
{
    char *current = ps->current;
    char *tp = &ps->token[0];

    /* 
     * Looking for a token (something with a character not found in sep).
     * Scan across newlines if newlinek is true.
     */
    do {
	/* 
	 * DXRead a new line if 
	 * 	a) this is the first time called
	 * 	b) the last time we were called left *current == '\0'; 
	 * 	c) we are scanning for a token on the next line
	 */
        if (current == NULL || *current == '\0' || (newline & FORCE_NEWLINE)) {
           if (!ps->filename || !_dxf_getline(ps))
              return ERROR;
           current = ps->line;
        }

	/* Find first character not found in sep */
        while (*current && strchr(sep,*current))
            current++;
    } while ((newline & NEWLINE_OK) && !*current);

    if (*current) {
	/* We found a token */
        /* if token begins with " go to next " before looking for end */
        if (*current == '\"'){
           do { 
                *tp = *current;
                tp++; current++;
              }while (*current && *current!='\"');
        }
        do {
                *tp = *current;
                tp++; current++;
              /*  if (*current=='\\'){
                 current++;
                 if (*current==','){ *tp=*current; tp++; current++;}
                 else {current--;}
                }
              */
        } while (*current && !strchr(sep,*current));
        *tp = '\0';
        *token = &ps->token[0];
    } else {
	/* No token was found (before the end of line) */
        *token = NULL;
    }

    ps->current = current;
    return OK;
}

static
Error _dxf_getline(struct parse_state *ps)
{
   char str[MAX_DSTR];
   char *line = ps->line;
   char *current = ps->current;
   int n=0;

   /* get newline allocating space if ness */
   if ((fgets(str,MAX_DSTR,ps->fp))==NULL){
     DXSetError(ERROR_DATA_INVALID,"#10898",ps->filename);
     return ERROR;
   }
   ps->lineno++;
   strcpy(line,str);
   while ((int)strlen(str)>MAX_DSTR-2){
      if ((fgets(str,MAX_DSTR,ps->fp))==NULL){
        DXSetError(ERROR_DATA_INVALID,"#10898",ps->filename);
        return ERROR;
      }
      n++;
      line = (char *)DXReAllocate(line,(unsigned int)strlen(str)+n*MAX_DSTR);
      strcat(line,str);
   }
   if ( strlen(line) >= 2 && line[strlen(line)-2] == '\r') 
     line[strlen(line)-2] = '\n';
   current = line;
   ps->current = current;
   ps->line = line;
 
   return OK;
}

static
Error getmorevar(int *num)
{
   int i,oldnum = *num;

   *num = *num + *num *.5;
   if ((_dxd_gi_var =
    (struct variable **)DXReAllocate((Pointer)_dxd_gi_var, sizeof(struct variable *) * *num)) == NULL)
      return ERROR;

   for (i=oldnum; i < *num; i++)
      _dxd_gi_var[i] = NULL;
 
   return OK;
} 
