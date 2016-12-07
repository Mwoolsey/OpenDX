/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>

#if defined(HAVE_STRING_H)
#include <string.h>
#endif

#include <stdio.h>
#include <dx/dx.h>
#include "_autocolor.h"
#include "_colormap.h"

extern FILE *_dxfopen_dxfile(char *name,char *auxname,char **outname,char *ext); /* from libdx/edfio.c */
extern Error _dxfclose_dxfile(FILE *fp,char *filename); /* from libdx/edfio.c */

static FILE *openfile(char *filename);
static Error scale_positions(Field *f,float min, float max);

#if 0
static Error addarray(Field f, int rank, int shape0, char *component);
#endif

typedef struct map {
   float level;
   float value;
} Map;
typedef struct hsvo {
   Map *map;
   char *name;
   int count;
   int minmax;
} HSVO;

Object 
DXImportCM(char *filename, char **fieldname)
{
    Group g = NULL;
    Field rgb=NULL,hsv=NULL,of=NULL;
    FILE *fp = NULL;
    double min, max;
    int i, n, nitems;
    double level, value;
    int color=1,opacity=1;
    float wrap, x, y;   /* wrap, x, y not used */
    HSVO maps[4];
    Array a[4];
    char *hname = "hue positions";
    char *sname = "sat positions";
    char *vname = "val positions";
    char *oname = "opacity positions";

    /* find the file and fopen it */
    fp = openfile(filename);
    if (!fp)
	return NULL;
   
    for (i=0; i<4; i++)
       a[i]=NULL;

    /* Are there any fieldnames (only colormap and opacity allowed) */
    if (fieldname != NULL){
      color=0;
      opacity=0;
      if (!strcmp(fieldname[0],"colormap") || 
		(fieldname[1] && !strcmp(fieldname[1],"colormap"))) 
	 color=1;
      if (!strcmp(fieldname[0],"opacitymap") ||
		(fieldname[1] && !strcmp(fieldname[1],"opacitymap")))
	 opacity=1;
      if (color==0 && opacity==0){
        DXSetError(ERROR_BAD_PARAMETER,"fieldname must be colormap or opacitymap");
        goto error;
      }
    }

    /* read min, max */
    if (fscanf(fp, "%lg %lg", &min, &max) != 2)
	goto error;
   
    /* set up HSVO struct */
    maps[0].name = &hname[0];
    maps[1].name = &sname[0];
    maps[2].name = &vname[0];
    maps[3].name = &oname[0];

    /* read the data, fill map structure and control point arrays */
    for (n=0; n<4; n++){
       a[n] = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, 2);
       if (!a[n])
          goto error;
       /* read npts each field: hue, sat, value, opacity */
       if (fscanf(fp, "%d", &nitems) != 1)
	   goto badformat;
       maps[n].count = nitems;
       if(nitems > 0){
	  if (!DXAddArrayData(a[n],0,nitems,NULL))
            goto error;
          maps[n].map = (Map *)DXGetArrayData(a[n]);
          for (i=0; i<nitems; i++) {
	    if (fscanf(fp, "%lg %lg %g %g %g", &level, &value, &wrap, &x, &y)
			!= 5)
	       goto badformat;
            /* put these in the xxx positions arrays */
            maps[n].map[i].level = (float)level;
	    maps[n].map[i].value = (float)value;
          }
       }
       else
	  a[n]=NULL;
    }

    if (color){
   /* make the colormap field using the control points */
    hsv = (Field)_dxfeditor_to_hsv(a[0],a[1],a[2]);
    if (!hsv) 
       goto error;
    rgb = (Field)_dxfMakeRGBColorMap((Field)hsv);
    DXDelete((Object)hsv);
    if (!rgb)
	  goto error;
   
    /* scale the positions of rgb to the min and max */
    if (!scale_positions(&rgb,(float)min,(float) max))
       goto error;
    for(n=0; n<3; n++){
       if (maps[n].count > 0){
	   if(!DXSetComponentValue((Field)rgb,maps[n].name,(Object)a[n]))
             goto error;
           a[n]=NULL;
       }
    }
    if (!DXEndField(rgb))
       goto error;
    }

    if (opacity){
       /* make the opacity field using the control points */
       of = (Field)_dxfeditor_to_opacity(a[3]);
       if (!of)
          goto error;
    
       /* scale the positions of rgb to the min and max */
       if (!scale_positions(&of,(float)min,(float) max))
          goto error;
    
       if (maps[3].count > 0){ 
	  if (!DXSetComponentValue((Field)of,maps[3].name,(Object)a[3]))
             goto error;
	  a[3]=NULL;
       }
       if (!DXEndField(of))
          goto error;
    }

    /* depending on fieldname, make either one or both fields.
     * don't put them into the group until you know you want them 
     * both, because the thing going back should be unreferenced
     * and it gets hard once it's been in a group.
     */
   if (fieldname == NULL || (color && opacity)){
      g = DXNewGroup();
      if (!DXSetMember((Group)g,"colormap",(Object)rgb))
         goto error;
      if (!DXSetMember((Group)g,"opacitymap",(Object)of))
         goto error;
   }

      _dxfclose_dxfile(fp,filename);
      if (g) return (Object)g;
      if (color) return (Object)rgb;
      if (opacity) return (Object)of;

  badformat:
    DXSetError(ERROR_DATA_INVALID, "file not in colormap format");
    /* fall thru */

  error:
    _dxfclose_dxfile(fp,filename);
    for (i=0; i<4; i++)
       if (a[i]) DXDelete((Object)a[i]);
    if (rgb) DXDelete((Object)rgb);
    if (of) DXDelete((Object)of);
    if (g) DXDelete((Object)g);
    return NULL;
}

static FILE *
openfile(char *filename)
{
    FILE *fp = NULL;
    char *outname;

    outname = NULL;
    /* getenv(the net path?) */
    fp = _dxfopen_dxfile(filename,NULL,&outname,".cm");
    if (!fp) {
	DXSetError(ERROR_BAD_PARAMETER, "#12240", filename);
	return NULL;
    }
    DXFree(outname);    
    return fp;
}

#if 0
static Error 
addarray(Field f, int rank, int shape0, char *component)
{
    Array a = NULL;

    if (rank == 0)
	a = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, rank);
    
    else if (rank == 1)
	a = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, rank, shape0);
    
    else {
	DXSetError(ERROR_INTERNAL, "bad rank creating colormap field");
	return ERROR;
    }
    
    if (!a)
	return ERROR;
    
    if (!DXSetComponentValue(f, component, (Object)a))
	return ERROR;
    
    return OK;
}
#endif

/* scale the positions array to new min and max from normalized space */
static Error
scale_positions(Field *f,float min, float max)
{
   int i,num;
   Array a;
   float *pts;

   a = (Array)DXGetComponentValue((Field)*f,"positions");
   if (!DXGetArrayInfo((Array)a,&num,NULL,NULL,NULL,NULL))
      goto error;
   pts = (float *)DXGetArrayData(a);
   if (!pts)
      goto error;
   for (i=0; i<num; i++)
      pts[i] = pts[i] *(max-min) +min;
   
   DXChangedComponentValues((Field)*f,"positions");
   /*DXSetComponentValue((Field)*f,"positions",(Object)a);
    */

   if (!DXEndField(*f))
      goto error;

   return OK;

error:
   return ERROR;
}

#if 0
static Error
_dxfExportCM(Object o, char *filename)
{
    /* find the positions min/max, nbins */

    /* if there are original components, format them */

    /* if not, convert to hsv 1-for-1 and output them */

    return OK;
}
#endif
