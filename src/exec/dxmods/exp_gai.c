/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#include <dx/dx.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "exp_gai.h"

#define MAXRANK 16
#define OFF 0
#define ON 1

static struct ap_info {
   char *spacedformat;
   char *plainformat;
   int width;
} ap[] = {
	/* strings */
	{NULL,     NULL,             1},   /* use pstring instead */
	/* unsigned bytes */
	{"%3u",   "%u",              3},
	/* bytes */
	{"%3d",   "%d",              3},
	/* unsigned shorts */
	{"%6u",   "%u",              6},
	/* shorts */
	{"%6d",   "%d",              6},
	/* unsigned ints */
	{"%9u",   "%u",	            9},
	/* ints */
	{"%9d",   "%d",              9},
	/* floats */
	{"%12.7g",  "%g",           12},
	/* doubles */
	{"%15g",    "%g",           15},
};

struct array_info {
	ArrayHandle 	handle;
	char *		format;		/* format for printf statement */
	Type 		type;		/* TYPE of value */
	int 		dim;		/* dim of item */
	int 		width;		/* width of longest value printed */
	Pointer		last;		/* last value for sequential access */
};

struct how {
	FILE *	dfp;
	int quote;
	int header;
	int tabbed;
};

static Error object_ex(Object o,struct how *h);
static Error array_ex(Array a ,struct how *h);
static Error field_ex(Field f,struct how *h);
static Error group_ex(Group g,struct how *h);
static Error xform_ex(Xform o,struct how *h);
static Error screen_ex(Screen o,struct how *h);
static Error clipped_ex(Clipped o,struct how *h);
static void pstring(FILE *fp, int width, int quote, int tab, char *cp);
static Error pvalue(struct array_info *arinfo,struct how *h,int item,int blank,int first);
static void pblank(struct array_info *info,struct how *h);
static void gethandle(Array a, struct array_info *info,int tab);
static void freehandle(struct array_info *info);
static Error object_header(Object o,struct how *h);
static Error group_header(Group g, struct how *h);
static void field_header(Field f, struct how *h);
static void array_header(Array a, struct how *h);
static Error xform_header(Xform o, struct how *h);
static Error screen_header(Screen o, struct how *h);
static Error clipped_header(Clipped o, struct how *h);

Object _dxfExportArray(Object o, char *filename, char *format)
{
   char *cp,*lcp,lcasefmt[64];
   int rc;
   struct how f;
   char *name;

   if (!o) {
      DXSetError(ERROR_BAD_PARAMETER,"#10000","input");
      return NULL;
   }

   if (!filename) {
      DXSetError(ERROR_BAD_PARAMETER,"#10000","filename");
      return NULL;
   }

   /* should we add an extension ? */
   f.dfp = fopen(filename,"w+");
   if (!f.dfp) {
      DXSetError(ERROR_BAD_PARAMETER,"#12248",filename);
      goto error;
   }
   
   /* turn format into lower case and look for keywords */
   f.quote = OFF;
   f.header = OFF;
   f.tabbed = OFF;
   if (format) {
      for (cp=format, lcp=lcasefmt; *cp && lcp < &lcasefmt[64]; cp++, lcp++)
	 *lcp = isupper(*cp) ? tolower(*cp) : *cp;
      *lcp = '\0';

      /* what keywords ? */
      for (cp=lcasefmt; *cp; ) {
	 if (isspace(*cp)) {
	    cp++;
	    continue;
	 }
	 if (!strncmp(cp, "array", 5)) {
	    cp += 5;
	    continue;
	 }
	 if (!strncmp(cp, "spreadsheet", 11)) {
	    cp += 11;
	    continue;
	 }
	 if (!strncmp(cp, "quotes", 6)) {
	    cp += 6;
	    f.quote = ON;
	    continue;
	 }
	 if (!strncmp(cp, "headers", 7)) {
	    cp += 7;
	    f.header = ON;
	    continue;
	 }
	 if (!strncmp(cp, "tabbed", 6)) {
	    cp += 6;
	    f.tabbed = ON;
	    continue;
	 }
	 DXSetError(ERROR_BAD_PARAMETER,"#12238",cp);
	 rc =ERROR;
	 goto error;
      }
   }

   if (f.header == ON) {
      /* name of object */
      if(DXGetStringAttribute((Object)o,"name",&name)){
         pstring(f.dfp,0,0,0,name);
         fprintf(f.dfp,"\n");
      }
      if (!object_header(o,&f))
         goto error;
   }

   rc = object_ex(o,&f);
   
   fclose(f.dfp);

   if (rc !=OK)
      return ERROR;

   return (o);

error:
   if (f.dfp)
      fclose(f.dfp);
   return NULL;

}

static Error object_ex(Object o, struct how *f)
{
   int rc=0;

   /* only support groups, fields, and arrays ? */
   switch (DXGetObjectClass(o)) {
      
      case CLASS_GROUP:
	 rc = group_ex((Group)o,f);
	 break;
      case CLASS_FIELD:
	 rc = field_ex((Field)o,f);
	 break;
      case CLASS_ARRAY:
	 rc = array_ex((Array)o,f);
	 break;
      case CLASS_XFORM:
	 rc = xform_ex((Xform)o,f);
	 break;
      case CLASS_SCREEN:
	 rc = screen_ex((Screen)o,f);
	 break;
      case CLASS_CLIPPED:
	 rc = clipped_ex((Clipped)o,f);
	 break;
      default:
	 DXSetError(ERROR_DATA_INVALID,"cannot export this object in array format");
   }

   if (rc != OK)
      return ERROR;

   return OK;
}


static Error field_ex(Field f, struct how *h)
{
   int i,n,numarrays=0; 	/* number of arrays written */
   int items,count;
   Array a;
   char *depon,*name;
   struct array_info *arinfo;
   int *printthis;
   InvalidComponentHandle invalidhandle=NULL;
   int array_beg = 1;		/* set to 0 if positions component exists */

   printthis=NULL;
   arinfo=NULL;
   for (count=0; (a=(Array)DXGetEnumeratedComponentValue((Field)f,count,NULL));
	     count++)
      ;
   /* initialize arrays */
   if (!(printthis = (int *)DXAllocate((count+1) * sizeof(int))))
      goto error;
   if (!(arinfo = (struct array_info *)DXAllocate((count+1) * 
						  sizeof(struct array_info))))
      goto error;

   /* Export position dep arrays (default) */
   for (i=0; (a=(Array)DXGetEnumeratedComponentValue((Field)f,i,&name)); i++){
      if (DXGetStringAttribute((Object)a,"dep",&depon) &&
          !strcmp("positions",depon) && strcmp("invalid positions",name) ) {
	 printthis[i] = ON;
         numarrays++;
      }
      else
	 printthis[i] = OFF;
   }

   /* are there invalid positions */
   if(DXGetComponentValue((Field)f,"invalid positions")){
      invalidhandle=DXCreateInvalidComponentHandle((Object)f,NULL,"positions");
      if (!invalidhandle)
	 goto error;
   }
   
   numarrays=1;
   /* fill info about each array to be printed (ie. format,type, ...) */
   for (i=0; (a=(Array)DXGetEnumeratedComponentValue((Field)f,i,&name)); i++){
      if (printthis[i]==ON){
	 DXGetArrayInfo((Array)a,&items,NULL,NULL,NULL,NULL);
	 if (!strcmp(name,"positions")) {
	    gethandle((Array)a,&arinfo[0],h->tabbed);
	    array_beg = 0;
	 }
	 else {
	    gethandle((Array)a,&arinfo[numarrays],h->tabbed);
	    numarrays++;
	 }
      }
   }

   /* loop through arrays printing one item at a time */
   for (i=0; i<items; i++) {
      if (invalidhandle && DXIsElementInvalidSequential(invalidhandle,i) ) {
         if (array_beg == 0) { 		/* print all positions even invalid ones */
	    if (!pvalue(&arinfo[0],h,i,0,1))
	       goto error;
	 }
	 else {
	    if (!pvalue(&arinfo[array_beg],h,i,1,1))
	       goto error;
	 }
	 for (n=array_beg+1; n<numarrays; n++) 
            if (!pvalue(&arinfo[n],h,i,1,0))
	       goto error;
      }
      else {
	 if (!pvalue(&arinfo[array_beg],h,i,0,1))
	    goto error;
         for (n=array_beg+1; n<numarrays; n++){
            if (!pvalue(&arinfo[n],h,i,0,0))
	       goto error;
         }
      }
      fprintf(h->dfp,"\n");
   }

   fprintf(h->dfp,"\n");	/* blank line separating fields */

   /* free the handles */
   for (n=array_beg; n<numarrays; n++)
      freehandle(&arinfo[n]);
   if (arinfo) DXFree((Pointer)arinfo);
   if (printthis) DXFree((Pointer)printthis);
   if (invalidhandle) DXFreeInvalidComponentHandle(invalidhandle);
   return OK;

error:
   if (arinfo) {
      for (n=array_beg; n<numarrays; n++)
         freehandle(&arinfo[n]);
      DXFree((Pointer)arinfo);
   }
   if (printthis) DXFree((Pointer)printthis);
   if (invalidhandle) DXFreeInvalidComponentHandle(invalidhandle);
   return ERROR;
}
      

static Error group_ex(Group g, struct how *h)
{
   int i;
   Object o;
   char *name;

   for (i=0; (o=DXGetEnumeratedMember(g,i,&name)); i++){
      if (!object_ex((Object)o,h))
	 return ERROR;
   }

   return OK;
}

static Error array_ex(Array a, struct how *h)
{
   int i,items;
   struct array_info *arinfo;

   if (!(arinfo = (struct array_info *)DXAllocate(sizeof(struct array_info))))
      goto error;
   
   DXGetArrayInfo((Array)a,&items,NULL,NULL,NULL,NULL);
   gethandle((Array)a,&arinfo[0],h->tabbed);
   for (i=0; i<items; i++) {
      if (!pvalue(&arinfo[0],h,i,0,1))
         goto error;
      fprintf(h->dfp,"\n");
   }
   DXFree((Pointer)arinfo);
   return OK;

error:
   if (arinfo) DXFree((Pointer)arinfo);
   return ERROR;
}

static Error xform_ex(Xform o,struct how *h)
{
   Object subo;
   DXGetXformInfo(o,&subo,NULL);
   if (!object_ex((Object)subo,h))
      return ERROR;
   
   return OK;
}

static Error screen_ex(Screen o,struct how *h)
{
   Object subo;
   DXGetScreenInfo(o,&subo,NULL,NULL);
   if (!object_ex((Object)subo,h))
      return ERROR;
   
   return OK;
}

static Error clipped_ex(Clipped o, struct how *h)
{
   Object subo;
   DXGetClippedInfo(o,&subo,NULL);
   if (!object_ex((Object)subo,h))
      return ERROR;
   
   return OK;
}


static Error pvalue(struct array_info *arinfo,struct how *h,int item,int blank,int first)
{
   int dim;
   Type type;
   char *format;
   Pointer scratch;
   int k;
   float *nextf;
   int *nexti;
   byte *nextb;
   short *nexts;
   uint *nextui;
   ubyte *nextub;
   ushort *nextus;
   double *nextd;
   char *nextstr;
   char *del;
   char *space = " ";
   char *tab = "\t";

   dim = arinfo->dim;
   format = arinfo->format;
   type = arinfo->type;
   if (h->tabbed == OFF) 
      del = space;
   else
      del = tab;
   switch(type) {
      case(TYPE_FLOAT):
	 scratch = (Pointer)DXAllocate(dim * sizeof(float));
	 nextf = (float *)DXAllocate(dim * sizeof(float));
         nextf = (float *)DXIterateArray(arinfo->handle,item,
				    arinfo->last,scratch);
	 arinfo->last = (Pointer)nextf;
	 for (k=0; k<dim; k++) {
	    if (first == ON) 
	       first = OFF;
	    else
	       fprintf(h->dfp,del);
	    if (blank == ON) 
	       pblank(arinfo,h);
	    else
	       fprintf(h->dfp,format,nextf[k]);
	 }
         break;
      case(TYPE_INT):
	 scratch = (Pointer)DXAllocate(dim * sizeof(int));
	 nexti = (int *)DXAllocate(dim * sizeof(int));
         nexti = (int *)DXIterateArray(arinfo->handle,item,
				    arinfo->last,scratch);
	 arinfo->last = (Pointer)nexti;
	 for (k=0; k<dim; k++) {
	    if (first == ON) 
	       first = OFF;
	    else
	       fprintf(h->dfp,del);
	    if (blank == ON) 
	       pblank(arinfo,h);
	    else
	       fprintf(h->dfp,format,nexti[k]);
	 }
	 break;
      case(TYPE_UINT):
	 scratch = (Pointer)DXAllocate(dim * sizeof(uint));
	 nextui = (uint *)DXAllocate(dim * sizeof(uint));
         nextui = (uint *)DXIterateArray(arinfo->handle,item,
				    arinfo->last,scratch);
	 arinfo->last = (Pointer)nextui;
	 for (k=0; k<dim; k++) {
	    if (first == ON) 
	       first = OFF;
	    else
	       fprintf(h->dfp,del);
	    if (blank == ON) 
	       pblank(arinfo,h);
	    else
	       fprintf(h->dfp,format,nextui[k]);
	 }
	 break;
      case(TYPE_BYTE):
	 scratch = (Pointer)DXAllocate(dim * sizeof(byte));
	 nextb = (byte *)DXAllocate(dim * sizeof(byte));
         nextb = (byte *)DXIterateArray(arinfo->handle,item,
				    arinfo->last,scratch);
	 arinfo->last = (Pointer)nextb;
	 for (k=0; k<dim; k++) {
	    if (first == ON) 
	       first = OFF;
	    else
	       fprintf(h->dfp,del);
	    if (blank == ON) 
	       pblank(arinfo,h);
	    else
	       fprintf(h->dfp,format,nextb[k]);
	 }
	 break;
      case(TYPE_UBYTE):
	 scratch = (Pointer)DXAllocate(dim * sizeof(ubyte));
	 nextub = (ubyte *)DXAllocate(dim * sizeof(ubyte));
         nextub = (ubyte *)DXIterateArray(arinfo->handle,item,
				    arinfo->last,scratch);
	 arinfo->last = (Pointer)nextub;
	 for (k=0; k<dim; k++) {
	    if (first == ON) 
	       first = OFF;
	    else
	       fprintf(h->dfp,del);
	    if (blank == ON) 
	       pblank(arinfo,h);
	    else
	       fprintf(h->dfp,format,nextub[k]);
	 }
	 break;
      case(TYPE_SHORT):
	 scratch = (Pointer)DXAllocate(dim * sizeof(short));
	 nexts = (short *)DXAllocate(dim * sizeof(short));
         nexts = (short *)DXIterateArray(arinfo->handle,item,
				    arinfo->last,scratch);
	 arinfo->last = (Pointer)nexts;
	 for (k=0; k<dim; k++) {
	    if (first == ON) 
	       first = OFF;
	    else
	       fprintf(h->dfp,del);
	    if (blank == ON) 
	       pblank(arinfo,h);
	    else
	       fprintf(h->dfp,format,nexts[k]);
	 }
	 break;
      case(TYPE_USHORT):
	 scratch = (Pointer)DXAllocate(dim * sizeof(ushort));
	 nextus = (ushort *)DXAllocate(dim * sizeof(ushort));
         nextus = (ushort *)DXIterateArray(arinfo->handle,item,
				    arinfo->last,scratch);
	 arinfo->last = (Pointer)nextus;
	 for (k=0; k<dim; k++) {
	    if (first == ON) 
	       first = OFF;
	    else
	       fprintf(h->dfp,del);
	    if (blank == ON) 
	       pblank(arinfo,h);
	    else
	       fprintf(h->dfp,format,nextus[k]);
	 }
	 break;
      case(TYPE_DOUBLE):
	 scratch = (Pointer)DXAllocate(dim * sizeof(double));
	 nextd = (double *)DXAllocate(dim * sizeof(double));
         nextd = (double *)DXIterateArray(arinfo->handle,item,
				    arinfo->last,scratch);
	 arinfo->last = (Pointer)nextd;
	 for (k=0; k<dim; k++) {
	    if (first == ON) 
	       first = OFF;
	    else
	       fprintf(h->dfp,del);
	    if (blank == ON) 
	       pblank(arinfo,h);
	    else
	       fprintf(h->dfp,format,nextd[k]);
	 }
	 break;
      case(TYPE_STRING):
	 scratch = (Pointer)DXAllocate(arinfo->dim + 1);
	 nextstr = (char *)DXIterateArray(arinfo->handle,item,
				  arinfo->last,scratch);
	 arinfo->last = (Pointer)nextstr;
	 if (first == OFF)
	    fprintf(h->dfp,del);
	 if (blank == ON)
	    pblank(arinfo,h);
	 else {
	    pstring(h->dfp, dim, h->quote,h->tabbed,nextstr);
	 }
	 break;
      default:
	 DXSetError(ERROR_DATA_INVALID,"not implemented");
	 goto error;
   }

   DXFree((Pointer)scratch);
   return OK;

error:
   return ERROR;
}

static void pstring(FILE *fp, int width, int quote, int tab, char *cp)
{
   int i,fill;

   if (!cp) {
      fprintf(fp," ");
      return;
   }

   if (tab == OFF){
      /* make each string the same length by adding spaces */
      if ((fill = width - (int)strlen(cp)) > 0){
         for (i=0; i<fill; i++)
	    putc(' ',fp);
      }
   }

   /* if quotes put them here */
   if (quote == ON)
      putc('"',fp);
   
   while(*cp) {
      switch(*cp) {
#ifndef DXD_NO_ANSI_ALERT
	case '\a': putc('\\', fp); putc('a', fp);  break;
#endif
	case '\b': putc('\\', fp); putc('b', fp);  break;
	case '\f': putc('\\', fp); putc('f', fp);  break;
	case '\n': putc('\\', fp); putc('n', fp);  break;
	case '\r': putc('\\', fp); putc('r', fp);  break;
	case '\t': putc('\\', fp); putc('t', fp);  break;
	case '\v': putc('\\', fp); putc('v', fp);  break;
	case '\\': putc('\\', fp); putc('\\', fp); break;
	case '"':  putc('\\', fp); putc('"', fp);  break;
	default:
	   if (isprint(*cp))
	      putc(*cp, fp);
	   else
	      fprintf(fp, "\\%03o", (uint)*cp);  /* octal code */
      }
      ++cp;
   }
   if (quote == ON)
      putc('"',fp);
}

static void pblank(struct array_info *info,struct how *h)
{
   int width,i;

   /* print blanks of same width as colums */
   if ( h->tabbed == ON){
      fprintf(h->dfp," ");
      return;
   }
      
   width = (info->width *info->dim);

   for (i=0; i<width; i++) 
      putc(' ',h->dfp);

   /* if strings with quotes ON add 2 extra blanks */
   if (info->type == TYPE_STRING && h->quote == ON) {
      putc(' ',h->dfp); putc(' ',h->dfp);
   }

   fprintf(h->dfp,"  ");
   
}

static void gethandle(Array a, struct array_info *info,int tab)
{
   int items,rank;
   Type type;
   Category cat;
   int shape[MAXRANK];
   int numtype;

      
	 DXGetArrayInfo(a,&items,&type,&cat,&rank,shape);
	
	 switch(type) {
	    case TYPE_STRING:	numtype = 0; break;
	    case TYPE_UBYTE:	numtype = 1; break;
	    case TYPE_BYTE:	numtype = 2; break;
	    case TYPE_USHORT:	numtype = 3; break;
	    case TYPE_SHORT: 	numtype = 4; break;
	    case TYPE_UINT:	numtype = 5; break;
	    case TYPE_INT:	numtype = 6; break;
	    case TYPE_FLOAT:	numtype = 7; break;
	    case TYPE_DOUBLE: 	numtype = 8; break;
	    default:
	       return;
	 }

	 if (tab == 1)		/* tabbed uses plain format */
	    info->format = ap[numtype].plainformat;
	 else
	    info->format = ap[numtype].spacedformat;
	 info->dim = (rank < 1) ? 1 : shape[0];
	 info->type = type;
	 info->width = ap[numtype].width;
	 info->handle = DXCreateArrayHandle(a);
	 info->last = NULL;
}

static void freehandle(struct array_info *ar)
{
   DXFreeArrayHandle(ar->handle);
}


/* this section is for printing the headers */

static Error object_header(Object o,struct how *f)
{
   int rc=OK;

   /* only support groups, fields, and arrays ? */
   switch (DXGetObjectClass(o)) {
      
      case CLASS_GROUP:
	 rc = group_header((Group)o,f);
	 break;
      case CLASS_FIELD:
	 field_header((Field)o,f);
	 break;
      case CLASS_ARRAY:
	 array_header((Array)o,f);
	 break;
      case CLASS_XFORM:
	 rc = xform_header((Xform)o,f);
	 break;
      case CLASS_SCREEN:
	 rc = screen_header((Screen)o,f);
	 break;
      case CLASS_CLIPPED:
	 rc = clipped_header((Clipped)o,f);
	 break;
      
      default:
	 DXSetError(ERROR_DATA_INVALID,"cannot export this object in array format");
         return ERROR;
   }

   if (rc == ERROR) 
      return ERROR;

   return OK;
}

int indent = 0;
char indent_char[3] = "  ";

static Error group_header(Group g, struct how *h)
{
   char *name;
   int n,i,j;
   Object o;
   float pos;		/* series positions */

   /* how many fields or groups */
   DXGetMemberCount((Group)g,&n);
   fprintf(h->dfp,"%d members",n);
   fprintf(h->dfp,"\n");

   indent++;
   switch (DXGetGroupClass(g)){
   case (CLASS_GROUP):
   case (CLASS_MULTIGRID):
   case (CLASS_COMPOSITEFIELD):
      for (i=0; (o=DXGetEnumeratedMember((Group)g,i,&name)); i++){ 
         /* indent */
	 for (j=0; j<indent; j++)
            fprintf(h->dfp,"%s",indent_char);
         if (name != NULL) {
	    pstring(h->dfp,0,0,0,name);
            fprintf(h->dfp,"\n");
         }
         if (!object_header((Object)o,h))
	    return ERROR;
      }
      break;
   case (CLASS_SERIES):
      for (i=0; i<n; i++){
         /* indent */
	 for (j=0; j<indent; j++)
            fprintf(h->dfp,"%s",indent_char);
	 o = DXGetSeriesMember((Series)g,i,&pos);
	 fprintf(h->dfp,"series position %g \n",pos);
         if (!object_header((Object)o,h))
	    return ERROR;
      }
      break;
    default:
      break;
   }

   indent--;
   return OK;
}


static void field_header(Field f,struct how *h)
{
   char *name, *depon;   
   int i,j,first=ON;
   Array a,*terms = NULL;
   Type type;
   int rank,items,shape[MAXRANK];
   /* int dim; */
   char *cmp;
   char *del;
   char *space = " ";
   char *tab = "\t";

   if (h->tabbed == ON)
      del = tab;
   else 
      del = space;

   /* name of field */
   if (DXGetStringAttribute((Object)f,"name",&name)){
      pstring(h->dfp,0,0,0,name);
      fprintf(h->dfp," ");
   }

   /* dimensions of the grid from connections */
   a = (Array)DXGetComponentValue((Field)f,"connections");
   if (a && DXGetArrayClass(a)==CLASS_MESHARRAY){
      if (DXQueryGridConnections(a,&rank,shape)){
         fprintf(h->dfp,"dimensions = ");
         for (i=0; i<rank; i++)
	    fprintf(h->dfp,"%d ",shape[i]);
	 fprintf(h->dfp,"\n");
      }
      else if (DXGetMeshArrayInfo((MeshArray)a,&rank,terms)) {
         fprintf(h->dfp,"dimensions = ");
         for (i=0; i<rank; i++){
            DXGetArrayInfo((Array)terms[i],&items,NULL,NULL,NULL,NULL);
            fprintf(h->dfp,"%d ",items);
         }
	 fprintf(h->dfp,"\n");
      }
   }
   else{
      DXWarning("cannot get dimensions");
      fprintf(h->dfp,"\n");
   }

   /* which components are printed */
   if ((a = (Array)DXGetComponentValue((Field)f,"positions")) != NULL){
      DXGetArrayInfo((Array)a,NULL,NULL,NULL,&rank,shape);
      /* dim = (rank < 1) ? 1 : shape[0]; */
      if (rank > 0) {
	 for (i=0; i<shape[0]; i++){
	    if (i > 0)
	       fprintf(h->dfp,del);
	    fprintf(h->dfp,"positions_cmp%d",i);
	 }
      }
      else
	 fprintf(h->dfp,"positions");
      first = OFF;
   }

   /* Export position dep arrays (default) */
   for (i=0; (a=(Array)DXGetEnumeratedComponentValue((Field)f,i,&name)); i++){
      if (DXGetStringAttribute((Object)a,"dep",&depon) &&
          !strcmp("positions",depon) && strcmp("invalid positions",name) 
	  && strcmp("positions",name) ) {
	 if (first == OFF) 
	    fprintf(h->dfp,del);
	 DXGetArrayInfo((Array)a,NULL,&type,NULL,&rank,shape);
	 /* dim = (rank < 1) ? 1 : shape[0]; */
	 if (type != TYPE_STRING && rank > 0 && shape[0] > 1) {
	    cmp = (char *)DXAllocate(strlen(name)+6);
	    for (j=0; j<shape[0]; j++) {
	       if (j > 0) fprintf(h->dfp, del);
	       sprintf(cmp,"%s_cmp%d",name,j);
	       pstring(h->dfp,0,0,0,cmp);
	    }
	    DXFree(cmp);
	 }
	 else
	    pstring(h->dfp,0,0,0,name);
	 first = OFF;
      }
   }
   fprintf(h->dfp,"\n");
}

static void array_header(Array a,struct how *h)
{
   char *name;

   /* name of Array */
   if (DXGetStringAttribute((Object)a,"name",&name)){
      pstring(h->dfp,0,0,0,name);
      fprintf(h->dfp,"\n");
   }
}

static Error xform_header(Xform o,struct how *h)
{
   Object subo;
   char *name;
   
   /* name of object */
   if(DXGetStringAttribute((Object)o,"name",&name)){
      pstring(h->dfp,0,0,0,name);
      fprintf(h->dfp,", ");
   }
   fprintf(h->dfp,"tranformed object,");
   DXGetXformInfo(o,&subo,NULL);
   if (!object_header((Object)subo,h))
      return ERROR;
   
   return OK;
}

static Error screen_header(Screen o,struct how *h)
{
   Object subo;
   char *name;
   
   /* name of object */
   if(DXGetStringAttribute((Object)o,"name",&name)){
      pstring(h->dfp,0,0,0,name);
      fprintf(h->dfp,", ");
   }
   fprintf(h->dfp,"screen object,");
   DXGetScreenInfo(o,&subo,NULL,NULL);
   if (!object_header((Object)subo,h))
      return ERROR;
   
   return OK;
}

static Error clipped_header(Clipped o, struct how *h)
{
   Object subo;
   char *name;
  
   /* name of object */
   if(DXGetStringAttribute((Object)o,"name",&name)){
      pstring(h->dfp,0,0,0,name);
      fprintf(h->dfp,", ");
   }
   fprintf(h->dfp,"clipped object,");
   DXGetClippedInfo(o,&subo,NULL);
   if (!object_header((Object)subo,h))
      return ERROR;
   
   return OK;
}

