/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/* import spreadsheet data 					*/
/* first pass 	-given delimiter, import data			*/
/* 		-take first line as field name			*/
/*		-create invalid data component for missing data	*/

#include <dxconfig.h>


#include <stdio.h>
#include <dx/dx.h>
#include <sys/types.h>
#include "genimp.h"
#include "cat.h" 

#define MAX_COLUMNS 200
#define MAX_DSTR 256
#define MAX_STRING 100
#define YES 1
#define NO 0
#define ON 1
#define OFF 0
#define OLD -1

/* Flags to FileTok() 'newline' parameter */
#define NO_NEWLINE      0
#define NEWLINE_OK      1
#define FORCE_NEWLINE   2

struct  parse_state {
      FILE    *fp;
      char    *filename;
      int     lineno;
      char    *line;
      char    token[MAX_DSTR];
      char    *current;
};

typedef enum {
	LABEL_ROW = 0,
	DATA_ROW = 1,
	COMMENT_ROW = 2
} row_type;
struct file_info {
  FILE *fp;
  char delimiter[4];
  int grid;
  int start;
  int end;
  int delta;
  int skip;
  int labelline;
  int ncolumns;
  int npoints;
  int which[MAX_COLUMNS];
};
struct column_info {
  Type type;
  char name[MAX_STRING]; /* name of component	*/
  Array array;		/* data array 		*/
  int inv_count;	/* counter for number of invalid elements */
  Array inv;		/* invalid array 	*/
};

static Error InitFileTok(struct parse_state *, FILE *, char *);
static Error FileTok(struct parse_state *,char *, int , char** );

static Error import_all(char *filename,char **varlist,char **categorize,struct file_info *fs,Object *out);
static Error parse_it_first(struct parse_state *ps,char **varlist,
		struct file_info *fs, struct column_info **ds);
static Error evaluate_oneline(struct parse_state *ps, char *delimiter,
			struct column_info **test, int *ncolumns);
static Error parse_it(struct parse_state *ps, struct file_info *fs,
			struct column_info **ds, int np, int nrec);
static Error load_data(struct column_info **src,struct column_info **ds,
	struct file_info *fs,int *index);
static Error load_first(struct column_info **src1, struct column_info **src2,
	char **varlist,struct column_info **ds,struct file_info *fs,int *index);
static void replace_row(struct column_info **old, struct column_info **new);
static Field build_field(struct file_info *fs, struct column_info **ds, char** cat_string);
static Error _dxf_getline(struct parse_state *ps);
static Type _dxf_isnumber(char *p, int *d, float *f);
static Error create_array(struct column_info *ds);
static Error process_data(char *p, struct column_info *ds, int index, 
	char *delmiter);
static Error cleanup (struct column_info **ds, int ncolumns);
static Field make_grid(Object o,struct file_info *fs, struct column_info **ds);
static Object make_list(struct column_info **ds,struct file_info *fs,
	int nimported);
static Error shrink_array(struct column_info *ds,int npoints);
static Error redo (struct column_info *ds, Type newtype);


Error m_ImportSpreadsheet(Object *in, Object *out)
{

  char *filename;
  char *string;
  struct file_info fs;
  char **fp = NULL, **fcat = NULL;
  int i,nfields,ncat;
  int startframe,endframe,deltaframe;
  int labelline,skip;

/* Parse the inputs */
  if (!in[0]) 
  {
    DXSetError(ERROR_BAD_PARAMETER,"filename must be specified");
    return ERROR;
  }
  if (!DXExtractString(in[0], &filename)) 
  {
    DXSetError(ERROR_BAD_PARAMETER,"filename must be a string");
    return ERROR;
  }

  if (!in[1])
    strcpy(fs.delimiter," \n\r");
  else
  {
    if (DXExtractString(in[1],&string) && strlen(string) == 1){
	 strcpy(fs.delimiter, string);
	 strcat(fs.delimiter,"\n\r");
    }
    else
    {
	 DXSetError(ERROR_BAD_PARAMETER,"delimiter must be one character");
       return ERROR;
    }
  }
 
  /* variable names build null terminated list */
  if (in[2]){
      nfields = 0;
      while (DXExtractNthString(in[2], nfields, &string))
          nfields++;

      if(nfields == 0) {
          DXSetError(ERROR_BAD_PARAMETER, "#10200", "variable");
          return ERROR;
      }
      if(!(fp = (char **)DXAllocate(sizeof(char *) * (nfields + 1))))
          return ERROR;

      for(i = 0; i < nfields; i++)
          if(!DXExtractNthString(in[2], i, &fp[i])) {
              DXSetError(ERROR_BAD_PARAMETER, "#10200", "variable");
              goto error;
          }

      fp[i] = NULL;
  }

  /* what type of field, 2-d grid or 1-d */
  if (in[3]){
    if (!DXExtractString(in[3],&string)){
      DXSetError(ERROR_BAD_PARAMETER,"format must be 1-d or 2-d");
      goto error;
    }
    if (!strcmp(string,"1-d"))
      fs.grid = NO;
    else if (!strcmp(string,"2-d"))
      fs.grid = YES;
    else{
      DXSetError(ERROR_BAD_PARAMETER,"format must be 1-d or 2-d");
      goto error;
    }
  }
  else
    fs.grid = NO;

  /* categorize which components, build null terminated list */
  if (in[4]){
      ncat = 0;
      while (DXExtractNthString(in[4], ncat, &string))
          ncat++;

      if(ncat == 0) {
          DXSetError(ERROR_BAD_PARAMETER, "#10200", "categorize which");
          return ERROR;
      }
      if(!(fcat = (char **)DXAllocate(sizeof(char *) * (ncat + 1))))
          return ERROR;

      for(i = 0; i < ncat; i++)
          if(!DXExtractNthString(in[4], i, &fcat[i])) {
              DXSetError(ERROR_BAD_PARAMETER, "#10200", "categorize which");
              goto error;
          }
      fcat[i] = NULL;
  }
  else
    fcat = NULL;

    /* start, end and delta for records */
    if(in[5]) {
        if(!DXExtractInteger(in[5], &startframe) || startframe < 0) {
            DXSetError(ERROR_BAD_PARAMETER, "#10030", "start");
            goto error;
        }
        fs.start = startframe;
    } else
        fs.start = 0;

    if(in[6]) {
        if(!DXExtractInteger(in[6], &endframe) || endframe < 0) {
            DXSetError(ERROR_BAD_PARAMETER, "#10030", "end");
            goto error;
        }
        fs.end = endframe;
    } else
        fs.end = DXD_MAX_INT;

    if(in[7]) {
        if(!DXExtractInteger(in[7], &deltaframe) || deltaframe < 0) {
            DXSetError(ERROR_BAD_PARAMETER, "#10030", "delta");
            goto error;
        }
        fs.delta = deltaframe;
    } else
        fs.delta = 1;

    if (fs.end < fs.start) {
      DXSetError(ERROR_BAD_PARAMETER, "#12260", fs.start, fs.end);
      goto error;
    }

  if (in[8]) {	/* number of comment lines to skip before reading data */
    if (!DXExtractInteger(in[8], &skip) || skip < 0) {
      DXSetError(ERROR_BAD_PARAMETER, "#10030", "headerlines");
      goto error;
    }
    fs.skip = skip;
  } else
    fs.skip = -1;

  if (in[9]) { /* line where label are found */
    if (!DXExtractInteger(in[9], &labelline) || labelline < 0) {
      DXSetError(ERROR_BAD_PARAMETER, "#10030", "labelline");
      goto error;
    }
    fs.labelline = labelline;
  } else
    fs.labelline = -1;

	
  if (!import_all(filename,fp,fcat,&fs,out))
    return ERROR;
  DXFree(fp);
  if (fcat) DXFree(fcat);
  return OK;

error:
  if (fp) DXFree(fp);
  if (fcat) DXFree(fcat);
  return ERROR;
}

static
Error import_all(char *filename,char **varlist,char **categorize,struct file_info *fs,Object *out)
{
  Object o;
  struct column_info **ds;
  int i;
  struct parse_state ps;
  int nimported;
  char **lp;
  char *outname;

/* open the file */
  if (!(fs->fp = _dxfopen_dxfile(filename,NULL,&outname,".ss")))
	   return ERROR;

/* initialize the column_info struct */
  ds = NULL;
  if( (ds = (struct column_info **)DXAllocate(
	     MAX_COLUMNS * sizeof(struct column_info *))) == NULL)
    return ERROR;
  for (i=0; i<MAX_COLUMNS; i++){
    ds[i] = NULL;
    fs->which[i] = OFF;
  }
  fs->ncolumns = 0;

/* initialize parse state */
  if (!InitFileTok(&ps,fs->fp,filename))
   goto error;

/* parse data and find the first row of data (what line #) */
  if (!parse_it_first(&ps,varlist, fs, ds))
    goto error;

  switch(fs->grid){
    case(NO):
      /* build the field from data */
      if ((out[0] = (Object)build_field(fs, ds,categorize))==NULL)
        goto error;
      break;
    case(YES):
      if ((o = (Object)build_field(fs,ds,categorize))==NULL)
	goto error;
      if ((out[0] = (Object)make_grid(o, fs, ds))==NULL)
	goto error;
      break;
  }
      
/* how many are imported */
      nimported=0;
      if (varlist != NULL && varlist[0] != NULL) {
	for (lp = varlist; *lp; lp++)
	  nimported++;
      }
      else
	nimported = fs->ncolumns;
/* create column header string list */
  if ((out[1] = make_list(ds,fs,nimported))==NULL)
    goto error;

/* cleanup memory */
  cleanup(ds,fs->ncolumns);
/* free parse_state->line */
  DXFree((Object)ps.line);

/* close the file */
  fclose(fs->fp);
  return OK;

error:
  cleanup(ds,fs->ncolumns);
  fclose(fs->fp);
  return ERROR;
}


/* parse the data looking for the pattern in the data 			*/
/* if successful ds will be allocated and the labels (if exist) 	*/
/* will be loaded, and the first (one or two) rows of data will be loaded */
Error parse_it_first(struct parse_state *ps, char **varlist,
	struct file_info *fs, struct column_info **ds)
{
  int i=0,j;		/* column counter */
  struct column_info **test1,**test2;
  int ncolumn1=0,ncolumn2=0;
  row_type rtype = COMMENT_ROW;
  int np=0,nrec=0;
  int skip = -1;

  /* initialize the column_info struct */
  test1 = test2  = NULL;
  if( ((test1 = (struct column_info **)DXAllocate(
	     MAX_COLUMNS * sizeof(struct column_info *))) == NULL) ||
      ((test2 = (struct column_info **)DXAllocate(
	     MAX_COLUMNS * sizeof(struct column_info *))) == NULL) )
    return ERROR;
  for (i=0; i<MAX_COLUMNS; i++){
    test1[i] = NULL;
    test2[i] = NULL;
  }

  /* if any comment lines specified skip them first */
  if (fs->labelline > 0) {
    skip = fs->skip;
    for (i=0; i<fs->labelline; i++){
      if (!_dxf_getline(ps))
	goto error;
      skip--;
    }
  }
  else if (fs->skip > 0) {
    for (i=0; i<fs->skip+1; i++){
      if (!_dxf_getline(ps))
	goto error;
    }
  }

  /* read in the first line */
  if (!evaluate_oneline(ps,fs->delimiter,test1,&ncolumn1))
     goto error;

  if (fs->labelline > 0 && skip > 0) {
    for (i=0; i<skip+1; i++){
      if (!_dxf_getline(ps))
	goto error;
    }
  }

next:
  /* read in the second line */
  if (!evaluate_oneline(ps,fs->delimiter,test2,&ncolumn2))
     goto error;

  /* what do we have */
  if(ncolumn2 == -1) { /* no second line to read */
  		fs->ncolumns = ncolumn1;
		if(!load_first(test1, NULL, varlist, ds, fs, &nrec))
			goto error;
  } else {
	  if (ncolumn1 == ncolumn2 && ncolumn1 != 0) {	/* possibly a match */
		/* check for data in this row (data is any numeric field)	*/
		for (i=0; i<ncolumn1; i++) 
		  if (test1[i]->type != TYPE_STRING &&
			test1[i]->type != TYPE_BYTE)
		break;	
		if (i == ncolumn1){	 	/* this is a label row? */
		  for (j=0; j<ncolumn2; j++) 
		if (test2[j]->type != TYPE_STRING &&
			test2[j]->type != TYPE_BYTE)
		  break;
		  if (j != ncolumn2) 	/* First line is label, then data */
		rtype = LABEL_ROW;
		  else 			/* data is all strings */
			rtype = DATA_ROW;
		}
		else			/* contains numeric data, the data */
		  rtype = DATA_ROW;
	  } 
	  else 				/* number of columns unequal, comments */
		rtype = COMMENT_ROW;
		
	  if (fs->skip > 0 && rtype == COMMENT_ROW) {
		DXSetError(ERROR_BAD_PARAMETER,"detected comments after skipping specified number of comment lines");
		goto error;
	  }
	  if (fs->labelline > 0 && rtype == COMMENT_ROW) {
		DXSetError(ERROR_BAD_PARAMETER,"detected comments after labels, use headerlines parameter to specify this");
		goto error;
	  }
	  else if (fs->labelline > 0 && rtype == DATA_ROW)
		rtype = LABEL_ROW;
	
	  switch (rtype) {
		case(LABEL_ROW):
		  /* place first row into real label and second row into data */
		  fs->ncolumns = ncolumn1;
		  if (!load_first(test2,test1,varlist,ds,fs,&nrec))
		goto error;
		  np=1;
		  break;
		case(DATA_ROW):
		  /* load into real column structure */
		  fs->ncolumns = ncolumn1;
		  if (!load_first(test1,NULL,varlist,ds,fs,&nrec))
		goto error;
		  np=1;
		  if (np >= fs->start && np <= fs->end &&(np - fs->start) % fs->delta == 0){
			if (!load_data(test2,ds,fs,&nrec))
		  goto error;
		  }
		  np++;
		  break;
		case(COMMENT_ROW):
		  replace_row(test1,test2);
		  ncolumn1=ncolumn2;
		  break;
	  }
	  if (rtype == COMMENT_ROW) 	/* continue until you find data */
		goto next;

  }

/* get rest of data */
  if (!parse_it(ps,fs,ds,np,nrec))
    goto error;

  cleanup(test1,ncolumn1);
  cleanup(test2,ncolumn2);
  return OK;
error:
  if (test1) cleanup(test1,ncolumn1);
  if (test2) cleanup(test2,ncolumn2);
  return ERROR;
}
    
/* read one line of data and get type, data->name, and number of columns */
static Error evaluate_oneline(struct parse_state *ps, char *delimiter,
		struct column_info **test, int *ncolumns)
{
  int nc=0;
  char *p;
  int d;
  float f;

  while (FileTok(ps,delimiter,NO_NEWLINE,&p) && p) {
    if (nc > MAX_COLUMNS-1){ 
      DXSetError(ERROR_DATA_INVALID,"only 200 columns supported");
      nc--;
      goto error;
    }
    if ((test[nc] = (struct column_info *)DXAllocate(
				 sizeof(struct column_info)))==NULL)
         goto error;
    test[nc]->array = NULL;
    test[nc]->type = _dxf_isnumber(p,&d,&f);
    if (test[nc]->type == TYPE_HYPER){
	DXSetError(ERROR_INTERNAL,"error matching regular expresion");
	goto error;
    }
    if (p[0] == delimiter[0]) 		/* missing data */
      test[nc]->type = TYPE_BYTE;
    if (strlen(p) > 99) 
       DXWarning("truncating name to 100 characters");
    strncpy(test[nc]->name,p,100);
    nc++; 
  }

  if(p == NULL && ps->current==NULL)
	*ncolumns = -1;
  else
	*ncolumns = nc;
 
  if (p && nc == 0 && strlen(p) > 1)	/* to catch case where token returning */
    return ERROR;		/* is greater than 256 , delimiter is wrong */

  return OK;
error:
  *ncolumns = nc;
  return ERROR;
}

/* parse data, read it into the data structure 	*/
/* already gathered the info 			*/
static Error parse_it(struct parse_state *ps, struct file_info *fs, 
			struct column_info **ds,int np1,int np2)
{
  char *p;
  int nc=0;		/* number of columns */
  int np,nrec;

  np = np1;		/* rows already processed */	
  nrec = np2;		/* number of records already added to the arrays */

/* get rest of data */
  while (FileTok(ps,fs->delimiter,FORCE_NEWLINE,&p) && p){
    if (np >= fs->start && np <= fs->end && (np - fs->start) % fs->delta == 0) {
      nc=0;
      if (fs->which[nc] == ON)
        process_data(p,ds[0],nrec,fs->delimiter);
      nc=1;
      while (FileTok(ps,fs->delimiter,NO_NEWLINE,&p) && p) {
        if (fs->which[nc] == ON)
          process_data(p,ds[nc],nrec,fs->delimiter);
        nc++;
      }
      nrec++;
      if (nc != fs->ncolumns) { 
	DXSetError(ERROR_DATA_INVALID,
	  "%d row: number of columns do not match previous rows",np);
	goto error;
      }
    }
    if (np > fs->end)
      break;
    np++;
  }
  fs->npoints = nrec;
  if (nrec < 1) {
    DXSetError(ERROR_DATA_INVALID,"no records match start, end, delta parameters");
    goto error;
  }
  DXResetError();

  return OK;

error:
  return ERROR;
}

/* load the second line of data */
static Error load_data(struct column_info **src,struct column_info **ds,
	struct file_info *fs,int *index)
{
  int i;

  for (i=0; i<fs->ncolumns; i++) {
    if (fs->which[i] == ON)
      process_data(src[i]->name,ds[i],*index,fs->delimiter);
  }
  (*index)++;
  return OK;
}

/* allocate the column structure and load the data 		*/
/* and load labels (default if none exist)			*/
/* if varlist exists load only the columns specified by user	*/
Error load_first(struct column_info **src, struct column_info **labelsrc,
	char **varlist,struct column_info **ds,struct file_info *fs,int *index)
{
  int i,iprev;
  char **lp;
  char name[10];

  if (varlist != NULL && varlist[0] != NULL) {	  /* user specified list */
    for (i=0; i<fs->ncolumns; i++) 
      fs->which[i] = OFF;
    for (lp = varlist; *lp; lp++) {
      for (i=0; i<fs->ncolumns; i++) {
	if (labelsrc){
	  if (!strcmp(labelsrc[i]->name,*lp))
	    break;
	}
	  sprintf(name,"column%d",i);
	  if (!strcmp(name,*lp))
	    break;
      }
      if (i == fs->ncolumns) {
	DXSetError(ERROR_BAD_PARAMETER,"%s not found in file",*lp);
	goto error;
      }
      fs->which[i] = ON; 
    }
  }
  else{
    for (i=0; i<fs->ncolumns; i++)
      fs->which[i] = ON;
  }
 
  /* allocate structure for each column and set labels for each column */
  /* create arrays and set type for each */
  for (i=0; i<fs->ncolumns; i++) {
      if (fs->which[i] == OFF) 
	continue;
      if ((ds[i] = (struct column_info *)DXAllocate(
				 sizeof(struct column_info)))==NULL)
        goto error;
      ds[i]->type = src[i]->type;
      if (labelsrc && labelsrc[i]->type != TYPE_BYTE){
        for (iprev=0; iprev<i-1; iprev++)
          if(!strcmp(labelsrc[i]->name,labelsrc[iprev]->name))
	   DXWarning("component names match(%s) last one will overwrite others",src[i]->name);
        strcpy(ds[i]->name,labelsrc[i]->name);
      }
      else
        sprintf(ds[i]->name,"column%d",i);
      if (!create_array(ds[i]))
	goto error;
  }
  
  /* place data in the array if within start,end,delta parameters */
  if (*index >= fs->start && *index <= fs->end &&
	(*index - fs->start) % fs->delta == 0) {
        for (i=0; i<fs->ncolumns; i++){ 
          if (fs->which[i] == ON)
            process_data(src[i]->name,ds[i],*index,fs->delimiter);
        }
        (*index)++;
  }

  return OK;
error:
  return ERROR;
}

/* replace one column structure with another */
void replace_row(struct column_info **old, struct column_info **new)
{
  int i;

  /* remove old structure and replace with new */
  for (i=0; old[i]; i++){
    DXFree(old[i]);
    old[i]=NULL;
  }
  for (i=0; new[i]; i++){
    old[i] = new[i];
    new[i] = NULL;
  }
}

/* fill data arrays */
static Error process_data(char *p,struct column_info *ds,int index,char *delimiter)
{
  float f;
  char inv_string[MAX_STRING];
  int d;
  Array a,inv;
  byte b;
  Type number_type;

  a = ds->array;
  inv = ds->inv;
  strcpy(inv_string," ");
  number_type = _dxf_isnumber(p,&d,&f);
  /* check for missing data */
  if (p[0] == delimiter[0] || number_type==TYPE_BYTE) {
    switch(ds->type) {	
	case(TYPE_FLOAT):
	  f = -999.0;
	  if (!DXAddArrayData((Array)a,index,1,&f))
	    goto error;
	  break;	
	case(TYPE_INT):
	  d = -999;
	  if (!DXAddArrayData((Array)a,index,1,&d))
	    goto error;
 	  break;	
        case(TYPE_STRING):
	  if (!DXAddArrayData((Array)a,index,1,inv_string))
	     goto error;
	  break;
        case(TYPE_BYTE):
	  b = 0;
	  if (!DXAddArrayData((Array)a,index,1,&b))
	    goto error;
	  break;
        default:
          break;
    }
    /* add to the invalid array list */
    if (!DXAddArrayData((Array)inv,ds->inv_count,1,&index))
      goto error;
    ds->inv_count++;

    return OK;
  }

  switch (ds->type) {
  case(TYPE_FLOAT):
    if (number_type==TYPE_FLOAT){
      if (!DXAddArrayData((Array)a,index,1,&f))
	goto error;
    }
    else if (number_type==TYPE_INT){
      f = (float)d;
      if (!DXAddArrayData((Array)a,index,1,&f))
	goto error;
    }
    else{
      redo(ds,TYPE_STRING);
      process_data(p,ds,index,delimiter);
    }
    break;
  case(TYPE_INT):
    if (number_type==TYPE_INT){
      if (!DXAddArrayData((Array)a,index,1,&d))
	goto error;
    }
    else if (number_type==TYPE_FLOAT) {
      redo(ds,TYPE_FLOAT);
      process_data(p,ds,index,delimiter);
    }
    else {
      redo(ds,TYPE_STRING);
      process_data(p,ds,index,delimiter);
    }
    break;
  case(TYPE_STRING):
    if (!DXAddArrayData((Array)a,index,1,p))
	 goto error;
    break;
  case(TYPE_BYTE):
    if (number_type==TYPE_INT)
      redo(ds,TYPE_INT);
    else if (number_type==TYPE_FLOAT)    
      redo(ds,TYPE_FLOAT);
    else
      redo(ds,TYPE_STRING);
    process_data(p,ds,index,delimiter);
    break;
   default:
    break;
  }
  return OK;

error:
  return ERROR;
}

/* create empty data arrays */
static Error create_array(struct column_info *ds)
{
  /* create array default string arrays to size 100 */
  if (ds->type == TYPE_STRING)
    ds->array = DXNewArray(ds->type, CATEGORY_REAL, 1, MAX_STRING);
  else
    ds->array = DXNewArray(ds->type, CATEGORY_REAL, 0);
  if (!ds->array)
    goto error;
  
  /* create an invalid array (a list of invalid indexes */
  ds->inv = DXNewArray(TYPE_INT,CATEGORY_REAL,0);
  if (!ds->inv)
    goto error;
  ds->inv_count = 0;

  return OK;

error:
  return ERROR;
}

/* build the field from the columns, each column is data array	*/
/* add regular connections and regular positions 		*/
static Field build_field(struct file_info *fs, struct column_info **ds,
char **cat_string)
{
  Array pos = NULL;
  Array con = NULL;
  Object o = NULL;
  Field f = NULL;
  int i;
  char inv_name[MAX_STRING];
  int j, invalid = 0;
  int *index;
  InvalidComponentHandle ivh;
  Object catted_object;
  int ncat=0;
  char **new_string;
  new_string=NULL;

  /* create new field */ 
  if (!(f = DXNewField()))
    goto error;

  /* default field, regular connections, regular positions, data */
  pos = (Array)DXMakeGridPositions(1,fs->npoints,0.0,1.0);
  if (!pos)
    goto error;
  con = (Array)DXMakeGridConnections(1,fs->npoints);
  if (!con){
    DXDelete((Object)pos);
    goto error;
  }

  if (!(DXSetComponentValue(f,"positions", (Object)pos)))
    goto error;
  if (!(DXSetComponentValue(f,"connections",(Object)con)))
    goto error;

  for (i=0; i < fs->ncolumns; i++) { 
    if (fs->which[i] == OFF)
      continue;

    if (ds[i]->type == TYPE_STRING) {	
	  if (!shrink_array(ds[i],fs->npoints))
	    goto error; 
    }
    if (!(DXSetComponentValue(f,ds[i]->name, (Object)ds[i]->array)))
	 goto error;
    if (!(o = (Object)DXNewString("positions")) || 
	    !DXSetComponentAttribute(f,ds[i]->name,"dep", o))
	 goto error;
    if (ds[i]->inv_count > 0){
      sprintf(inv_name, "%s missingvalues",ds[i]->name);
      if (!DXSetComponentValue(f,inv_name, (Object)ds[i]->inv))
        goto error;
      if (!DXSetComponentAttribute(f,inv_name,"ref", o))
        goto error;
      invalid = 1;
    }
    else 
      DXDelete((Object)ds[i]->inv);

    o = NULL;
  }

  /* create an invalid positions component which is the union of        */
  /* all the other invalid components                                   */
  if (invalid){
  ivh = DXCreateInvalidComponentHandle((Object)f,NULL,"positions");
  for (i=0; i < fs->ncolumns; i++) {
    if (fs->which[i] == OFF)
      continue;
    if (ds[i]->inv_count > 0){
      index = (int *)DXGetArrayData(ds[i]->inv);
      for (j=0; j<ds[i]->inv_count; j++)
        DXSetElementInvalid(ivh,index[j]);
    }
  }
  if (!DXSaveInvalidComponent(f,ivh))
    goto error;
  if (!DXFreeInvalidComponentHandle(ivh))
    goto error;
  }

  if (!DXEndField(f))
    goto error;

  /* if categorize is not NULL do it here */
  if (cat_string){
    if (cat_string[1] == NULL && !strcmp(cat_string[0],"allstring")){
      if(!(new_string = (char **)DXAllocate(sizeof(char *) *(fs->ncolumns+1))))
        return ERROR;
      for (i=0; i < fs->ncolumns; i++) { 
        if (fs->which[i] == OFF)
          continue;
	if (ds[i]->type == TYPE_STRING){ 
	  if (!(new_string[ncat] = (char *)DXAllocate(sizeof(char)*MAX_STRING)))
	    return ERROR;
	  strcpy(new_string[ncat],ds[i]->name);
	  ncat++;
	}
      } 
      if (ncat == 0){
	DXFree(new_string);
	DXWarning("there are no columns of type string imported");
	return (Field)f;
      }
      if (!(new_string[ncat] = (char *)DXAllocate(sizeof(char)*MAX_STRING)))
        return ERROR;
      new_string[ncat] = NULL;
      ncat++;
      catted_object = _dxfCategorize((Object)f,new_string); 
      if (!catted_object)
        goto error;
      DXDelete((Object)f);
      for (i=0; i<ncat; i++)
        DXFree(new_string[i]);
      DXFree(new_string);
      return (Field)catted_object;
    }
    else {

      catted_object = _dxfCategorize((Object)f,cat_string); 
      if (!catted_object)
        goto error;
      DXDelete((Object)f);
      return (Field)catted_object;
    }
  }
  else
    return (Field)f;

error:
  if (f) DXDelete((Object)f);
  if (o) DXDelete(o);
  if (new_string) {
    for (i=0;i<ncat; i++)
      DXFree(new_string[i]);
    DXFree(new_string);
  }
  return NULL;
}

/* makes a 2-d grid from a field with each column as a separate array */
static Field make_grid(Object o,struct file_info *fs, struct column_info **ds)
{
  int i,j,count=0;
  int np,nc=0;
  Field f=NULL;
  Array pos=NULL, con=NULL;
  Array data_array=NULL;
  float *new_f,*old_f;
  int *new_i,*old_i;
  ubyte *new_u,*old_u;
  char **string2, *string;
  Array a;
  Type type, datatype=TYPE_BYTE;
  InvalidComponentHandle ivh = NULL;
  int *index;

  np = fs->npoints;

  /* check that each array is the same type 	*/
  /* And if not can they be? 			*/
  for (i = 0;i < fs->ncolumns; i++){ 
    if (fs->which[i] != ON)
      continue;
    nc++;
    a = (Array)DXGetComponentValue((Field)o,ds[i]->name);
    DXGetArrayInfo(a,NULL,&type,NULL,NULL,NULL);
    if (datatype != type){
      if (datatype == TYPE_BYTE) datatype = type;
      else if (datatype == TYPE_UBYTE && type != TYPE_STRING) datatype = type;
      else if (datatype == TYPE_INT && type == TYPE_FLOAT) datatype=TYPE_FLOAT;
      else if ((datatype == TYPE_STRING && type != TYPE_STRING) ||
	(datatype != TYPE_STRING && type == TYPE_STRING)){
        DXSetError(ERROR_DATA_INVALID,
	  "all data must be the same type to use 2-d");
	goto error;
      }
    }
  }

  /* create a new data array which is each individual array put together */
  /* and convert data type if ness.					 */
  switch(datatype){
    case TYPE_UBYTE:
      data_array = DXNewArray(datatype, CATEGORY_REAL, 0);
      data_array = DXAddArrayData((Array)data_array,0,nc*np,NULL);
      new_u = (ubyte *)DXGetArrayData(data_array); 
      for (i=0; i<fs->ncolumns; i++){
        if (fs->which[i] != ON)
	  continue;
        a = (Array)DXGetComponentValue((Field)o,ds[i]->name);
        old_u = (ubyte *)DXGetArrayData(a);
        for (j=0; j<np; j++)
	  new_u[j+count] = old_u[j];
        count +=np;
      }
      break;
    case TYPE_INT:
      data_array = DXNewArray(datatype, CATEGORY_REAL, 0);
      data_array = DXAddArrayData((Array)data_array,0,nc*np,NULL);
      new_i = (int *)DXGetArrayData(data_array); 
      for (i=0; i<fs->ncolumns; i++){
        if (fs->which[i] != ON)
	  continue;
        a = (Array)DXGetComponentValue((Field)o,ds[i]->name);
        DXGetArrayInfo(a,NULL,&type,NULL,NULL,NULL);
	switch (type){
	  case TYPE_UBYTE:
	    old_u = (ubyte *)DXGetArrayData(a);
            for (j=0; j<np; j++)
	      new_i[j+count] = (int)old_u[j];
            count +=np;
	    break;
	  case TYPE_INT:
	    old_i = (int *)DXGetArrayData(a);
            for (j=0; j<np; j++)
	      new_i[j+count] = old_i[j];
            count +=np;
	    break;
	  default:
	    break;
	}
      }
      break;
    case TYPE_FLOAT:
      data_array = DXNewArray(datatype, CATEGORY_REAL, 0);
      data_array = DXAddArrayData((Array)data_array,0,nc*np,NULL);
      new_f = (float *)DXGetArrayData(data_array); 
      for (i=0; i<fs->ncolumns; i++){
        if (fs->which[i] != ON)
	  continue;
        a = (Array)DXGetComponentValue((Field)o,ds[i]->name);
        DXGetArrayInfo(a,NULL,&type,NULL,NULL,NULL);
	switch (type){
	  case TYPE_UBYTE:
	    old_u = (ubyte *)DXGetArrayData(a);
            for (j=0; j<np; j++)
	      new_f[j+count] = (float)old_u[j];
            count +=np;
	    break;
	  case TYPE_INT:
	    old_i = (int *)DXGetArrayData(a);
            for (j=0; j<np; j++)
	      new_f[j+count] = (float)old_i[j];
            count +=np;
	    break;
	  case TYPE_FLOAT:
	    old_f = (float *)DXGetArrayData(a);
            for (j=0; j<np; j++)
	      new_f[j+count] = old_f[j];
            count +=np;
	    break;
	  default:
	    break;
	}
      }
      break;
    case TYPE_STRING:
      string2 = (char**)DXAllocate(sizeof(char *) * np*nc);
      for (i=0; i<fs->ncolumns; i++){
        if (fs->which[i] != ON)
	  continue;
        a = (Array)DXGetComponentValue((Field)o,ds[i]->name);
        for (j=0; j<np; j++) {
    	  DXExtractNthString((Object)a,j,&string);
    	  *(string2+j+count) = string;
        }
        count+=np;
      }
      data_array = (Array)DXMakeStringListV(np*nc,string2);
      DXFree((Pointer)string2);
      break;
    default:
      break;
  }
 
 
  /* default field, regular connections, regular positions, data */
  f = DXNewField();
  pos = (Array)DXMakeGridPositions(2,nc,np,0.0,(float)np-1.0,1.0,0.0,0.0,-1.0);
  if (!pos)
    goto error;
  con = (Array)DXMakeGridConnections(2,nc,np);
  if (!con){
    DXDelete((Object)pos);
    goto error;
  }

  if (!(DXSetComponentValue(f,"positions", (Object)pos)))
    goto error;
  if (!(DXSetComponentValue(f,"connections",(Object)con)))
    goto error;
  if (!(DXSetComponentValue(f,"data",(Object)data_array)))
    goto error;
  
  /* create new invalid data component if invalid data exists */
  if (DXGetComponentValue((Field)o,"invalid positions"))
    ivh = DXCreateInvalidComponentHandle((Object)f,NULL,"positions");

  count = 0;
  for (i=0; i<fs->ncolumns; i++){
    if (fs->which[i] != ON)
      continue;
    if (ds[i]->inv_count > 0){	/* merge the invalid data components */
      index = (int *)DXGetArrayData(ds[i]->inv);
      for (j=0; j<ds[i]->inv_count; j++)
        DXSetElementInvalid(ivh,index[j]+count);
    }
    count+=np;
  }

  if (ivh && !DXSaveInvalidComponent(f,ivh))
    goto error;
  if (ivh && !DXFreeInvalidComponentHandle(ivh))
    goto error;

  DXDelete((Object)o);

  if (!DXEndField(f))
    goto error;
  
  return f;

error:
  if (f) DXDelete((Object)f);
  if (ivh && !DXFreeInvalidComponentHandle(ivh))
    goto error;
  return NULL;
}

/* Makes a string list (one of the outputs) */
static Object make_list(struct column_info **ds, struct file_info *fs,
	int nimported)
{
  int i,imp=0;
  char **string;
  Object o;

  string = (char **)DXAllocate(sizeof(char *) * nimported);
  for (i=0; i<fs->ncolumns; i++) {
    if (fs->which[i] == ON){
      string[imp] = (char *)DXAllocate(sizeof(char) * (1+strlen(ds[i]->name)));
      strcpy(string[imp],ds[i]->name);
      imp++;
    }
  }
  o = (Object)DXMakeStringListV(nimported,string);
  if (!o)
    goto error;
  
  for (i=0; i<nimported; i++) 
    DXFree(string[i]);
  DXFree(string);

  return o;

error:
  for (i=0; i<nimported; i++)
    DXFree(string[i]);
  DXFree(string);
  return NULL;
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
   int n=0;

   /* when the delimiter is specfied, check for special case of
    * empty field (delimiterdelimiter or delimiternewline) 
    * and return delimiter
    */
   if (sep[0] != ' '  && current && current[0] == sep[0] && 
      (current[1] == sep[0] || current[1] == sep[1] || current[1] == sep[2])){
     *tp = *current;
     tp++; current++;
     *tp = '\0';
     *token = &ps->token[0];
     ps->current = current;
     return OK;
   }
   
   /*
    * Looking for a token (something with a character not found in sep).
    * Scan across newlines if newlinek is true.
    */
   do {
      /*
       * DXRead a new line if
       *      a) this is the first time called
       *      b) the last time we were called left *current == '\0';
       *      c) we are scanning for a token on the next line
       */
      if (current == NULL || *current == '\0' || (newline & FORCE_NEWLINE)) {
        if (!ps->filename || !_dxf_getline(ps)) {
        	ps->current = NULL;
			*token = NULL;
          return ERROR;
        }
        current = ps->line;

	/* when the delimiter is specfied, check for special case of
	 * empty field at beginning of the line
	 */
	if (sep[0] != ' ' && current[0] == sep[0]){
          *tp = *current;
	  tp++;
          *tp = '\0';
          *token = &ps->token[0];
          ps->current = current;
          return OK;
	}
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
      	n++;
	if (n > 254){
	  DXSetError(ERROR_DATA_INVALID,"each column must be less than 256");
	  *tp = '\0';	/* finish token */
	  *token = &ps->token[0];
	  goto error;
	}
      } while (*current && !strchr(sep,*current));
      *tp = '\0';
      *token = &ps->token[0];
   } else {
      /* No token was found (before the end of line) */
      *token = NULL;
   }

   ps->current = current;
   return OK;

error:
   return ERROR;
}


/* cleanup data structure */
Error cleanup(struct column_info **ds,int ncolumns)
{
  int i;

  /* Free the data structure AddArrayData make a copy */
  for (i=0; i<ncolumns; i++)
    if (ds[i]) DXFree(ds[i]);
  DXFree((Pointer)ds);
  return OK;
}

/* getline allocating space as you go
 */
static
Error _dxf_getline(struct parse_state *ps)
{
  char str[MAX_DSTR];
  int n=0;
  char *line = ps->line;
  char *current = ps->current;
  int slop = 24;

  /* get newline allocating space if ness */
  if ((fgets(str,MAX_DSTR,ps->fp))==NULL){
    DXSetError(ERROR_DATA_INVALID,"error reading file");
    return ERROR;
  }
  ps->lineno++;
  strcpy(line,str);
  while ((int)strlen(str)>MAX_DSTR-2 && str[MAX_DSTR-2] != '\n'){
    if ((fgets(str,MAX_DSTR,ps->fp))==NULL){
      DXSetError(ERROR_DATA_INVALID,"error reading data file");
      return ERROR;
    }
    n++;
    line = (char *)DXReAllocate(line,(unsigned int)strlen(str)+n*MAX_DSTR+slop);
    strcat(line,str);
  }
  current = line;
  ps->current = current;
  ps->line = line;

  return OK;
}

static Type _dxf_isnumber(char *p,int *d, float *ff)
{
  int ret,ret1,type;
  float f;
  char string[250];
  char percent[1];
  
  *ff = 0;
  *d = 0;
/* use scanf statements to find out if a number */
  ret1 = sscanf(p,"$%g %1s %s",&f,percent,string); 
  ret = sscanf(p,"%g %1s %s",&f,percent,string);
  if (ret == 1 || (ret == 2 && !strcmp(percent,"%")) ||
     (ret1 == 1) || (ret1 == 2 && !strcmp(percent,"%"))){
    type = 1;				/* A floating point number */
    if (!strchr(p,'.')){
      if (f < (float)DXD_MAX_INT){
		sscanf(p+(*p=='$'), "%d", d);
        type = 2;  /* An integer */
      }
    }
  }
  else {
    type = 0;		/* A string */
    while(*p && *p==' ')
      p++;
    if (!*p)
      type =3;		/* all white space becomes invalid */
  }
  
  switch(type){
      case 0:
        return TYPE_STRING;
        break;
      case 1:
	*ff = f;
        return TYPE_FLOAT;
        break;
      case 2:
        return TYPE_INT;
        break;
      case 3:
	return TYPE_BYTE;
	break;
  } 
  return TYPE_HYPER;
}

/* the string arrays are allocated for MAX_STRING length 	*/
/* extract each item and use MakeStringList to save space	*/
static Error shrink_array(struct column_info *ds,int npoints)
{
  int i;
  char **string2, *string;
  Array a,b;

  a = ds->array;

  string2 = (char**)DXAllocate(sizeof(char *) * npoints);
  for (i=0; i<npoints; i++) {
    DXExtractNthString((Object)a,i,&string);
    *(string2+i) = string;
  }
  b = (Array)DXMakeStringListV(npoints,string2);
  DXFree((Pointer)string2);
  DXDelete((Object)a);
  ds->array = b;

  return OK;
}

/* copy old array to new array and replace */
static Error redo (struct column_info *ds, Type newtype)
{
  int i,items;
  float *f;
  char string[MAX_STRING];
  int *d;
  Array newarray;

  if (!DXGetArrayInfo(ds->array,&items,NULL,NULL,NULL,NULL))
    goto error;
  /* create newarray default string arrays to size 100 */
  if (newtype == TYPE_STRING)
    newarray = DXNewArray(newtype, CATEGORY_REAL, 1, MAX_STRING);
  else{
    newarray = DXNewArray(newtype, CATEGORY_REAL, 0);
    newarray = DXAddArrayData((Array)newarray,0,items,NULL);
  }
  if (!newarray)
    goto error;

  switch (ds->type){
    case(TYPE_BYTE):
      if (newtype == TYPE_INT){ 
	d = (int *)DXGetArrayData(newarray);
	for(i=0; i<items; i++)
	  d[i] = -999;
      }
      else if (newtype == TYPE_FLOAT){
	f = (float *)DXGetArrayData(newarray);
	for(i=0; i<items; i++)
	  f[i] = -999.0;
      }
      else{		/* TYPE_STRING */
        for (i=0; i<items; i++) {
	  sprintf(string,"%s"," ");
	  if (!DXAddArrayData((Array)newarray,i,1,string))
	     goto error;
 	} 
      } 
      break;	
    case(TYPE_INT):
      d = (int *)DXGetArrayData(ds->array);
      if (newtype == TYPE_FLOAT){
        f = (float *)DXGetArrayData(newarray);
	for (i=0; i<items; i++)
	  f[i] = (float)d[i];
      }
      else{
        for (i=0; i<items; i++) {
	  sprintf(string,"%d",d[i]);
	  if (!DXAddArrayData((Array)newarray,i,1,string))
	     goto error;
 	} 
      } 
      break;
    case(TYPE_FLOAT):
      f = (float *)DXGetArrayData(ds->array);
      for (i=0; i<items; i++) {
        sprintf(string,"%g",f[i]);
	if (!DXAddArrayData((Array)newarray,i,1,string))
	   goto error;
      } 
      break;
    default:
      break;
  }

  DXDelete((Object)ds->array);
  ds->array = newarray;
  ds->type = newtype;
  return OK;

error:
  return ERROR;
}

