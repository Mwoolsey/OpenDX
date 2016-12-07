/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 *        Piechart   module to create a 2D piechart using
 *                   a list of percentages to define to angular
 *                   measure of a wedge.
 *        Author:    Donna Gresh 
 *                   05/20/97
 *
*/
/*=====================Include Files===================================*/

#include <dxconfig.h>


#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#if defined(HAVE_STRING_H)
#include <string.h>
#endif

#include <dx/dx.h>

/*=====================Definitions=====================================*/


typedef struct
{
  float x, y;
} Point2D;


#define AUTORADIUSMIN .2
#define AUTORADIUSMAX 1.0
#define AUTOHEIGHTMIN .05
#define AUTOHEIGHTMAX .5

static int LabelsAreUnique(Array); 
static Array GetLabels(Object, Object);
static Error MakePie(Object, Object, int, 
                     float, Array, Group *, Group *, Array *, Array *, 
                     Field, int *, int);
static Error MakePie3D(Object, Object, Field, float *, int);
static Error ScalePie(Object, int, float *, int);
static Error ScaleLabels(Object, int, float *);
static RGBColor *CheckColors(Array, Field, int);
static Error CheckLabels(int, Array, int *);
static Error CheckHeight(Object, Object, Object, Object, Object, 
                         int, int *, float **);
static Error CheckRadius(Object, Object, Object, Object, Object, 
                         int, int *, float **);
static Error CopyComponents(Field, Field, Field, int, int, int);
static Error AddData(float *, Field, Field, int, int);
static Error AddColors(Group *, Group *, int, RGBColor *);
static Error GetParameters(Object, Object, Object, Object, Object, 
                           float *, float *, float *, float *);

#define FORMAT_LIST(type)                                              \
{                                                                      \
        type * dataptr;                                                \
        int maxlen=0, i;                                               \
        char buf[2048];                                                \
        dataptr = (type *)DXGetArrayData(component);                   \
        for (i=0; i<numitems; i++){                                    \
           sprintf(buf, format, dataptr[i]);                           \
           if (strlen(buf)>maxlen) maxlen=strlen(buf);                 \
        }                                                              \
        newarray = DXNewArray(TYPE_STRING, CATEGORY_REAL, 1, maxlen+1);\
        for (i=0; i<numitems; i++) {                                   \
            sprintf(buf, format, dataptr[i]);                          \
            if (!DXAddArrayData(newarray, i, 1, &buf))                 \
               goto error;                                             \
        }                                                              \
}                                                                      \
 

#define FILL_PERCENT_ARRAY(type)                    \
{                                                   \
        type * dataptr;                             \
        dataptr = (type *)DXGetArrayData(data);     \
        if (percentflag == 0) {                     \
           for (i=0; i<numitems; i++) {             \
              percentarray[i] = dataptr[i]*3.6;     \
           }                                        \
        }                                           \
        else if (percentflag == 1) {                \
           for (i=0; i<numitems; i++) {             \
              percentarray[i] = dataptr[i]*360;     \
           }                                        \
        }                                           \
        else {                                      \
        /* raw numbers */                           \
        /*XXX neg numbers? invalid data */          \
           cumvalue = 0;                            \
           for (i=0; i<numitems; i++) {             \
              cumvalue += dataptr[i];               \
           }                                        \
           for (i=0; i<numitems; i++) {             \
              percentarray[i] = (dataptr[i]/cumvalue)*360; \
           }                                        \
        }                                           \
}
 

Error m_Pie(Object *in, Object *out) {
  
  Group pie=NULL, newpie;
  Group edges=NULL, newedges;
  Field labelfield=NULL;
  Object In_percents, In_colors, In_labels, In_spiffiness;
  Object In_radius, In_height, In_percentflag;
  Object In_heightscale, In_radiusscale, In_labelformat, In_showpercent;
  Object In_heightmax, In_heightmin, In_heightratio;
  Object In_radiusmax, In_radiusmin, In_radiusratio;
  int dim, numwedges, show_percents;
  int radtype, percentflag;
  char *flagstring;
  float quality, *height=NULL, *radius=NULL;
  Array coloroutput=NULL, labeloutput=NULL, labellist=NULL;
  ModuleInput input[20];
  ModuleOutput output[20];
  
  
  In_percents   = in[0];
  In_percentflag   = in[1];
  In_radius     = in[2];
  In_radiusscale   = in[3];
  In_radiusmin   = in[4];
  In_radiusmax   = in[5];
  In_radiusratio   = in[6];
  In_height     = in[7];
  In_heightscale   = in[8];
  In_heightmin   = in[9];
  In_heightmax   = in[10];
  In_heightratio   = in[11];
  In_spiffiness = in[12];
  In_colors     = in[13];
  In_labels     = in[14];
  In_labelformat = in[15];
  In_showpercent = in[16];
  
  if (In_spiffiness) {
    if (!DXExtractFloat(In_spiffiness, &quality)) {
      DXSetError(ERROR_DATA_INVALID,
		 "quality must be a scalar between 0 and 1");
      goto error;
    }
    if ((quality < 0)||(quality > 1))  {
      DXSetError(ERROR_DATA_INVALID,
		 "quality must be a scalar between 0 and 1");
      goto error;
    }
  }
  else {
    quality = .25;
  }
  
  if (!In_percents) {
    DXSetError(ERROR_MISSING_DATA,"percents must be specified");
    goto error;
  }


  /* get the percents flag */
  if (In_percentflag) {
   if (!DXExtractNthString((Object)In_percentflag, 0, &flagstring)) {
    DXSetError(ERROR_DATA_INVALID,
           "percentflag must be one of `percents`, `fractions`, `values`");
    goto error;
   }
   /* XXX note that upper case should be handled */
   if (!strcmp(flagstring,"percents"))
     percentflag = 0;
   else if (!strcmp(flagstring,"fractions"))
     percentflag = 1;
   else if (!strcmp(flagstring,"values"))
     percentflag = 2;
   else {
    DXSetError(ERROR_DATA_INVALID,
           "percentflag must be one of `percents`, `fractions`, `values`");
    goto error;
   }
  }
  else {
    percentflag = 2;
  }
  
  
  /* make the pie */
  pie = DXNewGroup();
  if (!pie)
    goto error;
  /* make the edges field */
  edges = DXNewGroup();
  if (!edges)
    goto error;
  
  labelfield = DXNewField();
  if (!labelfield)
    goto error;

  if (In_showpercent) {
    if (!DXExtractInteger(In_showpercent, &show_percents)){
      DXSetError(ERROR_DATA_INVALID,"showpercents must be 0 or 1");
      goto error;
    } 
    if ((show_percents < 0)||(show_percents > 1)) {
      DXSetError(ERROR_DATA_INVALID,"showpercents must be 0 or 1");
      goto error;
    } 
  }
  else {
    show_percents = 0;
  }

  if (In_labels) {
     labellist = GetLabels(In_labels, In_labelformat);
     if (!labellist)
       goto error;
  }
 


  /* this makes a simple 2D pie, of a single radius */  
  if (!MakePie(In_percents, In_colors, percentflag, quality, labellist, 
	       &pie, &edges, 
	       &labeloutput, &coloroutput, labelfield, &numwedges,
               show_percents))
    goto error;
  
  
  
  
  
  /* 
   * look at what the user passed in for the radius param, and
   * muck with the radius values 
   */
  if (!CheckRadius(In_radius, In_radiusscale, In_radiusmin, In_radiusmax,
                   In_radiusratio, numwedges, &radtype, &radius))
      goto error; 


  /* 
   * if the user passed in anything for the height param, muck with
   * the height values
   */
  if (In_height) {
    if (!CheckHeight(In_height, In_heightscale, In_heightmin, In_heightmax, 
                     In_heightratio, numwedges, &dim, &height))
      goto error; 
  }
  /*
   * if the user didn't pass anything in for height, it's flat
   */
  else
    dim = 0;
  
  


  /* non-flat*/
  if ((dim == 1)||(dim == 2)) {
    
    if (!MakePie3D((Object)pie, (Object)edges, labelfield, 
		   height, dim))
      goto error; 

    if (!ScalePie((Object)pie, radtype, radius, 0))
      goto error;
    if (!ScalePie((Object)edges, radtype, radius, 0))
      goto error;
    if (!ScaleLabels((Object)labelfield, radtype, radius))
      goto error;
    
    
    /* add normals for shading */ 
    /* use call module to call FaceNormals, Normals */
    
    DXModSetObjectInput(&input[0], NULL, (Object)pie);
    DXModSetObjectOutput(&output[0], NULL, (Object *)&newpie);
    if (DXCallModule("FaceNormals", 1, input, 1, output)==ERROR)
      goto error;
    
    DXModSetObjectInput(&input[0], NULL, (Object)edges);
    DXModSetObjectOutput(&output[0], NULL, (Object *)&newedges);
    if (DXCallModule("FaceNormals", 1, input, 1, output)==ERROR)
      goto error;
    out[0] = (Object)newpie;
    out[1] = (Object)newedges;
    

  }
  /* flat */
  else {
    if (!ScalePie((Object)pie, radtype, radius, 0))
      goto error;
    if (!ScalePie((Object)edges, radtype, radius, 0))
      goto error;
    if (!ScaleLabels((Object)labelfield, radtype, radius))
      goto error;

    /* since the edges are lines, add some fuzz */
    if (!DXSetFloatAttribute((Object)edges, "fuzz", 4))
      goto error;

    out[0] = (Object)pie;
    out[1] = (Object)edges;
  }
  
  out[2] = (Object)labelfield;
  out[3] = (Object)labeloutput;
  out[4] = (Object)coloroutput;

  DXFree((Pointer)height);
  DXFree((Pointer)radius);
  DXDelete((Object)labellist);
  
  return OK; 


error: 
  DXDelete((Object) pie);
  DXDelete((Object) edges);
  DXDelete((Object) labelfield);
  DXDelete((Object) coloroutput);
  DXFree((Pointer)height);
  DXFree((Pointer)radius);
  DXDelete((Object)labellist);
  out[0]=NULL;
  out[1]=NULL;
  out[2]=NULL;
  out[3]=NULL;
  out[4]=NULL;
  return ERROR;
  
}


static Error MakePie(Object In_percents, Object In_colors, int percentflag,  
                     float quality, Array labellist, 
                     Group *pie, Group *edges, 
                     Array *labeloutput, Array *coloroutput, 
                     Field labelfield, int *numwedges, int showpercents)
{
  Array data, c;
  Array colors=NULL;
  Array labelpositions=NULL;
  float labelangle;
  RGBColor *col;
  RGBColor *cptr=NULL;
  Type type;
  Category category;
  Object member, newpie, newedges;
  int numitems, rank, shape[32];
  float angle, arc, cumpercent; 
  Field F1=NULL, F2=NULL;
  int i, poscount, concount, maxstringlength;
  Array wedgespositions=NULL, wedgesconnections=NULL, wedgesdata=NULL;
  Array edgesconnections=NULL, edgesdata=NULL;
  Array wedgescolors=NULL, edgescolors=NULL;
  Triangle tri;
  float *percentarray=NULL, cumvalue;
  Point2D pt2D, labelpos;
  char *stringlabel=NULL, *nthstring, *tmpstring;
  ModuleInput input[20];
  ModuleOutput output[20];
  Object colormap;
  
  float drg2rad = 4*atan(1.)/180.;



  /* In_percents can be a field or an array */
  
  switch (DXGetObjectClass(In_percents)) {
  case CLASS_FIELD:
    data = (Array)DXGetComponentValue((Field)In_percents,"data");
    if (!data) {
      DXSetError(ERROR_MISSING_DATA,"missing data component");
      goto error;
    }
    /* does the input have colors? */
    colors = (Array)DXGetComponentValue((Field)In_percents,"colors");
    
    break;
  case CLASS_ARRAY:
    data = (Array)In_percents;
    break;
  default:
    DXSetError(ERROR_DATA_INVALID,"invalid percents");
    goto error;
  }
  
  if (!DXGetArrayInfo(data,&numitems, &type, &category, &rank, shape))
    goto error; 
  *numwedges = numitems;

  /* Input colors param has precendence over colors in the field */
  if (In_colors) {
    /* check that In_colors is an array */
    if (DXGetObjectClass(In_colors) != CLASS_ARRAY) {
       DXSetError(ERROR_DATA_INVALID,"invalid colors parameter");
       goto error;
    }
    cptr = CheckColors((Array)In_colors, NULL, numitems);
    if (!cptr)
      goto error;
  }
  else if (colors) {
    cptr = CheckColors(colors, (Field)In_percents, numitems);
    if (!cptr)
      goto error;
  }
  

  /* check the labels */
  if (labellist) {
    if (!CheckLabels(numitems, labellist, &maxstringlength))
      goto error;
  }
  else
    maxstringlength=0;

  stringlabel = DXAllocate((maxstringlength+10)*sizeof(char));
  if (!stringlabel)
    goto error;


  /* make the percents and colors arrays. These are the extra outputs */
  *labeloutput = DXNewArray(TYPE_STRING, CATEGORY_REAL, 1, 
			    maxstringlength+10);
  if (!*labeloutput)
    goto error;
  *coloroutput = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, 3);
  if (!*coloroutput)
    goto error;
  
  if (!DXAddArrayData(*labeloutput, 0, numitems, NULL))
    goto error; 

  if (!DXAddArrayData(*coloroutput, 0, numitems, NULL))
    goto error;


  if (category != CATEGORY_REAL) {
    DXSetError(ERROR_DATA_INVALID,"percents must be real");
    goto error;
  }
  if (!((rank == 0)||((rank == 1)&&(shape[0]==1)))) {
    DXSetError(ERROR_DATA_INVALID,"percents must be scalar");
    goto error;
  }
  
  percentarray = DXAllocate(numitems*sizeof(float));
 
  switch (type) {
     case (TYPE_FLOAT):
        FILL_PERCENT_ARRAY(float);
        break;
     case (TYPE_INT):
        FILL_PERCENT_ARRAY(int);
        break;
     case (TYPE_BYTE):
        FILL_PERCENT_ARRAY(byte);
        break;
     case (TYPE_SHORT):
        FILL_PERCENT_ARRAY(short);
        break;
     case (TYPE_UINT):
        FILL_PERCENT_ARRAY(uint);
        break;
     case (TYPE_UBYTE):
        FILL_PERCENT_ARRAY(ubyte);
        break;
     case (TYPE_USHORT):
        FILL_PERCENT_ARRAY(ushort);
        break;
     case (TYPE_DOUBLE):
        FILL_PERCENT_ARRAY(double);
        break;
     default:
        DXSetError(ERROR_DATA_INVALID,"unknown data type");
        goto error;
     }
  labelpositions = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, 2);
  
    
  angle = 0;
  cumpercent = 0;
  for (i=0; i<numitems; i++) {
    F1 = DXNewField(); 
    if (!F1)
      goto error;
    F2 = DXNewField(); 
    if (!F2)
      goto error;
    
    wedgespositions = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, 2);
    wedgesconnections = DXNewArray(TYPE_INT, CATEGORY_REAL, 1, 3);
    
    /* this equation is set up so that 
       a quality of 0 leads to 10 degrees;
       a quality of 1 leads to .25 degrees;
       */
    
    arc = (10.0 - 9.75*quality);
    /* draw the first 2 points */
    poscount = 0;
    pt2D.x = 0;
    pt2D.y = 0;
    if (!DXAddArrayData(wedgespositions, 0, 1, &pt2D))
      goto error;
    poscount++;
    pt2D.x = cos(angle*drg2rad);
    pt2D.y = sin(angle*drg2rad);
    if (!DXAddArrayData(wedgespositions, poscount, 1, &pt2D))
      goto error;
    poscount++;


    concount=0;
    while (angle+arc < percentarray[i]+cumpercent) {
      angle += arc;
      pt2D.x = cos(angle*drg2rad); 
      pt2D.y = sin(angle*drg2rad); 
      if (!DXAddArrayData(wedgespositions, poscount, 1, (Pointer)&pt2D))
	goto error;
      tri = DXTri(poscount-1, poscount, 0);
      if (!DXAddArrayData(wedgesconnections, concount, 1, (Pointer)&tri))
	goto error; 
      
      poscount++;
      concount++;
    }
    /* finish it off */
    labelangle = cumpercent + percentarray[i]/2.0;
    cumpercent +=percentarray[i];
    angle = cumpercent;
    pt2D.x = cos(angle*drg2rad);
    pt2D.y = sin(angle*drg2rad);
    if (!DXAddArrayData(wedgespositions, poscount, 1, (Pointer)&pt2D))
      goto error; 
    tri = DXTri(poscount-1, poscount, 0);
    if (!DXAddArrayData(wedgesconnections, concount, 1, (Pointer)&tri))
      goto error; 
    poscount++;
    concount++;
    
    /* add the origin back in. It will simplify things for the edges so
       that mesh arrays can be used */ 
    pt2D.x = 0;
    pt2D.y = 0;
    if (!DXAddArrayData(wedgespositions, poscount, 1, &pt2D))
      goto error;
    poscount++;
    

    /* now do the label */
    labelpos.x = cos(labelangle*drg2rad);
    labelpos.y = sin(labelangle*drg2rad);
    if (!DXAddArrayData(labelpositions, i, 1, &labelpos))
      goto error;
    
    if (!DXSetComponentValue(F1,"positions", (Object)wedgespositions))
      goto error;
    if (!DXSetComponentValue(F2,"positions", (Object)wedgespositions))
      goto error;
    wedgespositions=NULL;

    
    if (!DXSetComponentValue(F1,"connections", (Object)wedgesconnections))
      goto error;
    wedgesconnections=NULL;
    
    /* there are two more edge connections than wedges; one line for each
       wedge arc, plus the two lines from the origin to define the wedge.
       And remember that MakeGridConnections tells how many positions
       are connected, not how many connections there are. Thus the +3.
       */
    edgesconnections = (Array)DXMakeGridConnections(1, concount+3);
    if (!DXSetComponentValue(F2,"connections", (Object)edgesconnections))
      goto error;
    edgesconnections=NULL;
    
    DXSetComponentAttribute(F1,"connections", "ref", 
			    (Object)DXNewString("positions"));
    DXSetComponentAttribute(F1,"connections", "element type", 
			    (Object)DXNewString("triangles"));
    
    DXSetComponentAttribute(F2,"connections", "ref", 
			    (Object)DXNewString("positions"));
    DXSetComponentAttribute(F2,"connections", "element type", 
			    (Object)DXNewString("lines"));

    /* now do all of the other components which dep the same thing
       as data deps. Obviously this is only appropriate if the input
       is a field. note that colors will be copied as is. That should
       be fine. */
    if (DXGetObjectClass(In_percents)==CLASS_FIELD) {
      if (!CopyComponents((Field)In_percents, F1, F2, concount, 
			  concount+2, i))
	goto error;
    }
    else {
      /* if it's not a field, then we've got to put some data on */
      if (!AddData(&percentarray[i], F1, F2, concount, concount+2))
	goto error; 
    }
  


    /* make the label output */
    if (labellist) {
      if (!DXExtractNthString((Object)labellist, i, &nthstring))
	goto error;
      if (showpercents)
        sprintf(stringlabel, "%s %6.2f %%", nthstring, percentarray[i]/3.6);
      else
        sprintf(stringlabel, "%s", nthstring);
      if (!DXAddArrayData(*labeloutput, i, 1, stringlabel))
	goto error;
    }
    else {
      /* XXX what about smaller numbers? */
      sprintf(stringlabel, "%6.2f %%", percentarray[i]/3.6);
      if (!DXAddArrayData(*labeloutput, i, 1, stringlabel))
	goto error;
    }
    

    DXEndField(F1);
    DXEndField(F2);

    
    if (labellist) { 
      /* problem here is that labels can be duplicated */
      if (LabelsAreUnique(labellist)) {
         if (!DXExtractNthString((Object)labellist, i, &tmpstring))
	   goto error;
         DXSetMember(*pie, tmpstring, (Object)F1);
         DXSetMember(*edges, tmpstring, (Object)F2);
         F1=NULL;
         F2=NULL;
      }
      else {
         DXSetEnumeratedMember(*pie, i, (Object)F1);
         DXSetEnumeratedMember(*edges, i, (Object)F2);
         F1=NULL;
         F2=NULL;
      }
    }
    else {
      DXSetEnumeratedMember(*pie, i, (Object)F1);
      DXSetEnumeratedMember(*edges, i, (Object)F2);
      F1=NULL;
      F2=NULL;
    }
  }


  if (!DXSetComponentValue(labelfield, "positions", 
			   (Object)labelpositions))
    goto error;
  labelpositions=NULL;
  if (!DXSetComponentValue(labelfield, "data", 
			   (Object)*labeloutput))
    goto error;
  if (!DXSetComponentAttribute(labelfield,"data", "dep",
                               (Object)DXNewString("positions")))
    goto error;

  /* if the user provided colors via In_colors, add them in now */
  if (In_colors) {
    if (!AddColors(pie, edges, numitems, cptr))
      goto error;
  }
  
  
  /* if there weren't any colors, add some */
  if ((!colors)&&(!In_colors)) {
    DXReference((Object)*pie);
    /* use call module to call AutoColor */
    DXModSetObjectInput(&input[0], "data", (Object)*pie);
    DXModSetObjectInput(&input[1], "min", (Object)*pie);
    DXModSetObjectOutput(&output[0], NULL, (Object *)&newpie);
    DXModSetObjectOutput(&output[1], NULL, (Object *)&colormap);
    if (DXCallModule("AutoColor", 2, input, 2, output)==ERROR)
      goto error;
    DXDelete((Object)colormap);
    DXReference((Object)*edges);
    /* use call module to call AutoColor */
    DXModSetObjectInput(&input[0], "data", (Object)*edges);
    DXModSetObjectInput(&input[1], "min", (Object)*edges);
    DXModSetObjectOutput(&output[0], NULL, (Object *)&newedges);
    DXModSetObjectOutput(&output[1], NULL, (Object *)&colormap);
    if (DXCallModule("AutoColor", 2, input, 2, output)==ERROR)
      goto error;
    DXDelete((Object)colormap);
    /* grab the colors */
    for (i=0; i<numitems; i++) {
      member = (Object)DXGetEnumeratedMember((Group)newpie, i, NULL);
      c = (Array)DXGetComponentValue((Field)member,"colors");
      col = (RGBColor *)DXGetConstantArrayData(c);
      if (!DXAddArrayData(*coloroutput, i, 1, col))
	goto error;
    }  
  }
  else {
    /* grab the colors */
    for (i=0; i<numitems; i++) {
      if (!DXAddArrayData(*coloroutput, i, 1, &cptr[i]))
	goto error;
    }  
  }
  


  
  if ((!colors)&&(!In_colors)) {
    DXDelete((Object)*edges);
    DXDelete((Object)*pie);
    *edges = (Group)newedges;
    *pie = (Group)newpie;
  }
 
  DXFree((Pointer)cptr); 
  DXFree((Pointer)stringlabel); 
  DXFree((Pointer)percentarray); 
  return OK;
  
error:
  DXFree((Pointer)cptr); 
  DXFree((Pointer)stringlabel); 
  DXFree((Pointer)percentarray); 
  DXDelete((Object)F1);
  DXDelete((Object)F2);
  DXDelete((Object)wedgespositions);
  DXDelete((Object)wedgesconnections);
  DXDelete((Object)wedgesdata);
  DXDelete((Object)edgesconnections);
  DXDelete((Object)edgesdata);
  DXDelete((Object)wedgescolors);
  DXDelete((Object)edgescolors);
  DXDelete((Object)labelpositions);
  return ERROR;
  

}


static Error ScalePie(Object pie, int radtype, float *radius, 
		      int index)
{
  Object member;
  int shape[32], j, i, numitems;
  Point *pos3d, *newpos3d;
  Point2D *pos2d, *newpos2d;
  Array positions, newpositions = NULL;
  float angle, rad;
  
  switch (DXGetObjectClass((Object)pie)) {
  case (CLASS_GROUP): 
    for (j=0; (member = DXGetEnumeratedMember((Group)pie, j, NULL)); j++) {
      if (!ScalePie(member, radtype, radius, j))
	goto error;
    }
    
    break;
  case (CLASS_FIELD):
    positions = (Array)DXGetComponentValue((Field)pie,"positions");
    DXGetArrayInfo(positions, &numitems, NULL, NULL, NULL, shape);
    newpositions = DXNewArrayV(TYPE_FLOAT, CATEGORY_REAL, 1, shape);
    if (!newpositions) goto error;
    if (!DXAddArrayData(newpositions, 0, numitems, NULL))
      goto error; 
    if (shape[0]==2) {
      pos2d = (Point2D *)DXGetArrayData(positions);
      newpos2d = (Point2D *)DXGetArrayData(newpositions);
      for (i=0; i<numitems; i++) {
	angle = atan2(pos2d[i].y, pos2d[i].x);
	rad = sqrt(pos2d[i].y*pos2d[i].y + pos2d[i].x*pos2d[i].x);
	if (radtype == 1)
	  rad = radius[index]*rad;
	else
	  rad = radius[0]*rad;
	newpos2d[i].y = rad*sin(angle);
	newpos2d[i].x = rad*cos(angle);
      } 
    }
    else if (shape[0]==3) {
      pos3d = (Point *)DXGetArrayData(positions);
      newpos3d = (Point *)DXGetArrayData(newpositions);
      for (i=0; i<numitems; i++) {
	angle = atan2(pos3d[i].y, pos3d[i].x);
	rad = sqrt(pos3d[i].y*pos3d[i].y + pos3d[i].x*pos3d[i].x);
	if (radtype == 1)
	  rad = radius[index]*rad;
	else
	  rad = radius[0]*rad;
	newpos3d[i].y = rad*sin(angle);
	newpos3d[i].x = rad*cos(angle);
	newpos3d[i].z = pos3d[i].z;
      } 
    }
    if (!DXSetComponentValue((Field)pie, "positions", 
			     (Object)newpositions))
      goto error;
    newpositions=NULL;
    DXChangedComponentValues((Field)pie,"positions");
    
    break;

  default:
    break;
  }
  
  return OK;
error:
  DXDelete((Object)newpositions);
  return ERROR;
}


static RGBColor *CheckColors(Array colors, Field F, int numdata)
{
/* given an array of colors (either passed in on the "colors" param,
   or from the "colors" component of the field), check that they are
   valid colors, and create an rgb float array to return. */      

  int numcolors;
  Type type;
  Category category;
  int rank, shape[32], i;
  ArrayHandle handle=NULL;
  Array colormap;
  float *scratchRGB=NULL, *scratchbytecolors=NULL, *scratchbyte=NULL;
  int *scratchRGBint=NULL;
  RGBColor *colorRGB_ptr=NULL, *colormap_ptr, colorRGB;
  int *colorRGBint_ptr=NULL;
  RGBColor *cptr;
  ubyte *colorbytecolor_ptr; 
  ubyte *colorbyte_ptr=0;
  char *colorstring;

  /* check that num of colors matches num of data */
  if (!DXGetArrayInfo(colors, &numcolors, &type, &category,
		      &rank, shape)) 
    goto error;
  
  if (numcolors != numdata) {
    DXSetError(ERROR_DATA_INVALID,
	       "number of colors does not match number of percent items");
    goto error;
  }
  
  if (category != CATEGORY_REAL) {
    DXSetError(ERROR_DATA_INVALID,"unknown color type");
    goto error;
  }
  
  /* allocate an array to hold the floating point colors */
  cptr = (RGBColor *)DXAllocate(numcolors*sizeof(RGBColor));
  if (!cptr) 
    goto error; 
  
  /* create an array handle */
  handle=DXCreateArrayHandle(colors);
  if (!handle) 
    goto error;
  
  /* check what sort of colors they are */
  switch (type) {
  case (TYPE_STRING):
    /* a list of colorname strings. This is only valid if they
       came in on the "colors" parameter; otherwise, the names
       are going to get replicated onto the colors component,
       which makes no sense at all. XXX */
       for (i=0; i<numcolors; i++) {
          if (!DXExtractNthString((Object)colors, i, &colorstring)) {
             DXSetError(ERROR_DATA_INVALID,"invalid colors");
             goto error;
          }
          if (!DXColorNameToRGB(colorstring, &colorRGB))
             goto error;
          cptr[i] = colorRGB;
       }
    break;
  case (TYPE_INT):
    /* check that they are 3D */
    if (rank != 1) {
      DXSetError(ERROR_DATA_INVALID,"unknown color type");
      goto error;
    }
    if (shape[0] != 3) {
      DXSetError(ERROR_DATA_INVALID,"unknown color type");
      goto error;
    }
    scratchRGBint = DXAllocate(3*sizeof(int));
    if (!scratchRGBint)
      goto error;
    /* iterate through the colors array */
    for (i=0; i<numcolors; i++) {
      colorRGBint_ptr = DXIterateArray(handle, i, colorRGBint_ptr, 
                                       scratchRGBint);
      (cptr[i]).r = (float)(colorRGBint_ptr[0]);
      (cptr[i]).g = (float)(colorRGBint_ptr[1]);
      (cptr[i]).b = (float)(colorRGBint_ptr[2]);
    } 
    break;
    
  case (TYPE_FLOAT):
    /* check that they are 3D */
    if (rank != 1) {
      DXSetError(ERROR_DATA_INVALID,"unknown color type");
      goto error;
    }
    if (shape[0] != 3) {
      DXSetError(ERROR_DATA_INVALID,"unknown color type");
      goto error;
    }
    scratchRGB = DXAllocate(sizeof(RGBColor));
    if (!scratchRGB)
      goto error;
    /* iterate through the colors array */
    for (i=0; i<numcolors; i++) {
      colorRGB_ptr = DXIterateArray(handle, i, colorRGB_ptr, scratchRGB);
      cptr[i] = *colorRGB_ptr;
    } 
    break;
    
  case (TYPE_UBYTE):
    /* color lookup */
    if ((rank == 0)||((rank == 1)&&(shape[0] == 1))) {
      scratchbyte = DXAllocate(sizeof(ubyte));
      if (!scratchbyte)
	goto error;
      colorbyte_ptr = DXAllocate(sizeof(ubyte));
      if (!colorbyte_ptr)
	goto error;
      /* check for colormap component */
      /* this option not valid for colors as an array sent in on a parameter,
         as opposed to in the input field */
      if (!F) {
         DXSetError(ERROR_DATA_INVALID,"invalid colors input");
         goto error;
      }
      colormap = (Array)DXGetComponentValue(F, "color map");
      if (!colormap) {
	DXSetError(ERROR_MISSING_DATA,
		   "missing colormap for delayed colors");
	goto error;
      }	
      colormap_ptr = DXGetArrayData(colormap);
      if (!colormap_ptr)
	goto error;
      
      for (i=0; i<numcolors; i++) {
	colorbyte_ptr=DXIterateArray(handle, i,
				     colorbyte_ptr,scratchbyte);
	
	colorRGB = colormap_ptr[(int)(*colorbyte_ptr)];
	cptr[i] = colorRGB; 
      } 
      
    }
    /* byte colors */
    else if ((rank == 1)&&(shape[0]==3)) {
      scratchbytecolors = DXAllocate(3*sizeof(ubyte));
      if (!scratchbytecolors)
	goto error;
      colorbytecolor_ptr = DXAllocate(3*sizeof(ubyte));
      if (!colorbyte_ptr)
	goto error;
      for (i=0; i<numcolors; i++) {
	colorbytecolor_ptr=DXIterateArray(handle, i,
					  colorbytecolor_ptr,
					  scratchbytecolors);
	colorRGB = DXRGB((double)(colorbytecolor_ptr[0]/255.0),
			 (double)(colorbytecolor_ptr[1]/255.0),
			 (double)(colorbytecolor_ptr[2]/255.0));
	cptr[i] = colorRGB; 
      } 
    }
    else {
      DXSetError(ERROR_DATA_INVALID, "unknown color type");
      goto error;
    }
    break; 
  default:
    DXSetError(ERROR_DATA_INVALID,"unknown color type");
    goto error;
  }
  
  DXFree((Pointer)scratchRGB);
  DXFree((Pointer)scratchRGBint);
  DXFree((Pointer)scratchbytecolors);
  DXFree((Pointer)scratchbyte);
  DXFreeArrayHandle(handle);
  return cptr;
error:
  DXFree((Pointer)scratchRGB);
  DXFree((Pointer)scratchbytecolors);
  DXFree((Pointer)scratchbyte);
  DXFreeArrayHandle(handle);
  return NULL;
}



static Error MakePie3D(Object pie, Object edges, Field labelfield, 
                       float *h, int dim)
{
  /* this routine bumps up the positions of the pie by height,
     and turns the edges into quads */
  Array positions, newpositions=NULL, newconnections=NULL;
  Point2D *pos_2D_ptr;
  Point   *pos_3D_ptr;   
  int numitems, i, j;
  Object member;
  float height=0;
  

  if (dim == 1)
    height = h[0];
  
  /* first bump up the positions of the pie */
  for (j=0; (member = DXGetEnumeratedMember((Group)pie, j, NULL)); j++) {
    
    positions = (Array)DXGetComponentValue((Field)member,"positions");
    pos_2D_ptr = (Point2D *)DXGetArrayData(positions);
    if (!DXGetArrayInfo(positions, &numitems, NULL, NULL, NULL, NULL))
      goto error;
    
  
 
    newpositions = DXNewArray(TYPE_FLOAT,CATEGORY_REAL, 1, 3);
    if (!newpositions)
      goto error;
    if (!DXAddArrayData(newpositions, 0, numitems, NULL))
      goto error;
    
    pos_3D_ptr = (Point *)DXGetArrayData(newpositions);
    if (!pos_3D_ptr)
      goto error;
    
    if (dim == 1) {
      for (i=0; i<numitems; i++) {
	pos_3D_ptr[i] = DXPt(pos_2D_ptr[i].x, pos_2D_ptr[i].y, height); 
      }
    }
    else {
      height = h[j];
      for (i=0; i<numitems; i++) {
	pos_3D_ptr[i] = DXPt(pos_2D_ptr[i].x, pos_2D_ptr[i].y, height); 
      }
    }
    
    if (!DXSetComponentValue((Field)member, "positions", 
			     (Object)newpositions))
      goto error;
    newpositions=NULL;
    if (!DXChangedComponentValues((Field)member,"positions"))
      goto error;
  }
  

  /* also bump up the positions of the labels */
  positions = (Array)DXGetComponentValue((Field)labelfield,"positions");
  if (!DXGetArrayInfo(positions,&numitems, NULL,NULL,NULL,NULL))
    goto error;
  pos_2D_ptr = (Point2D *)DXGetArrayData(positions);
  newpositions = DXNewArray(TYPE_FLOAT,CATEGORY_REAL, 1, 3);
  if (!newpositions)
    goto error;
  if (!DXAddArrayData(newpositions, 0, numitems, NULL))
    goto error;
  
  pos_3D_ptr = (Point *)DXGetArrayData(newpositions);
  if (!pos_3D_ptr)
    goto error;
  
  if (dim == 1) {
    for (i=0; i<numitems; i++) {
      pos_3D_ptr[i] = DXPt(pos_2D_ptr[i].x, pos_2D_ptr[i].y, height); 
    }
  }
  else {
    for (i=0; i<numitems; i++) {
      height = h[i];
      pos_3D_ptr[i] = DXPt(pos_2D_ptr[i].x, pos_2D_ptr[i].y, height); 
    }
  }
  
  if (!DXSetComponentValue((Field)labelfield, "positions", 
			   (Object)newpositions))
    goto error;
  newpositions=NULL;
  
  if (!DXChangedComponentValues((Field)labelfield,"positions"))
    goto error;
  

  /* now bump up the edges of the pie */
  for (j=0; (member = DXGetEnumeratedMember((Group)edges, j, NULL)); j++) {
    
    positions = (Array)DXGetComponentValue((Field)member,"positions");
    pos_2D_ptr = (Point2D *)DXGetArrayData(positions);
    if (!DXGetArrayInfo(positions, &numitems, NULL, NULL, NULL, NULL))
      goto error;
    
  
 
    newpositions = DXNewArray(TYPE_FLOAT,CATEGORY_REAL, 1, 3);
    if (!newpositions)
      goto error;
    
    if (!DXAddArrayData(newpositions, 0,numitems*2, NULL))
      goto error;
    
    pos_3D_ptr = (Point *)DXGetArrayData(newpositions);
    if (!pos_3D_ptr)
      goto error;
    
    if (dim == 1) {
      for (i=0; i<numitems; i++) {
	pos_3D_ptr[2*i] = DXPt(pos_2D_ptr[i].x, pos_2D_ptr[i].y, 0.0); 
	pos_3D_ptr[2*i+1] = DXPt(pos_2D_ptr[i].x, pos_2D_ptr[i].y, height); 
      }
    }
    else {
      height = h[j];
      for (i=0; i<numitems; i++) {
	pos_3D_ptr[2*i] = DXPt(pos_2D_ptr[i].x, pos_2D_ptr[i].y, 0.0); 
	pos_3D_ptr[2*i+1] = DXPt(pos_2D_ptr[i].x, pos_2D_ptr[i].y, height); 
      }
    }
    
      
    
    if (!DXSetComponentValue((Field)member, "positions", 
			     (Object)newpositions))
      goto error;
    newpositions=NULL;
    DXChangedComponentValues((Field)member,"positions");
    
    /* the connections are a mesh array */
    newconnections = (Array)DXMakeGridConnections(2, numitems, 2);
    if (!newconnections)
      goto error;
    if (!DXSetComponentValue((Field)member, "connections", 
			     (Object)newconnections))
      goto error;
    newconnections=NULL;
    if (!DXSetComponentAttribute((Field)member, "connections", 
				 "element type", 
				 (Object)DXNewString("quads")))
      goto error;
    if (!DXChangedComponentValues((Field)member,"connections"))
      goto error;
    
  }
  return OK;
error:
  DXDelete((Object)newpositions);
  DXDelete((Object)newconnections);
  return ERROR; 
  
}



static Error CheckLabels(int numitems, Array labellist, int *maxstringlength)
{
  int shape[32], numstrings;
  
  if (!DXExtractNthString((Object)labellist, 0, NULL)) {
    DXSetError(ERROR_DATA_INVALID,"labels must be a string list");
    goto error;
  }
  
  if (!DXGetArrayInfo((Array)labellist, &numstrings, NULL, NULL, NULL, shape))
    goto error;
  
  if (numstrings != numitems) {
    DXSetError(ERROR_DATA_INVALID, 
	       "number of labels must match number of percents");
    goto error;
  }
  
  *maxstringlength = shape[0];
  
  return OK;
  
error:
  return ERROR;
  
}


static Error CopyComponents(Field input, Field pie, Field edges, 
                            int numwedges, int numedges, int index)
{
  int compcount, numitems, rank, shape[32], componentsdone=0;
  int size;
  Type type;
  Category category;
  char *name;
  Object dataattr;
  Array component, newwedgescomponent=NULL, newedgescomponent=NULL;
  char *dataptr=NULL;
  


  compcount=0;
  dataattr = DXGetComponentAttribute((Field)input, "data", "dep");
  if (DXGetObjectClass(dataattr) != CLASS_STRING) {
    DXSetError(ERROR_DATA_INVALID,  
	       "data dependency attribute not string");
    goto error;
  }
  
  while (NULL != 
	 (component=(Array)DXGetEnumeratedComponentValue(input, 
							 compcount, 
							 &name))) {
    Object attr;
      
    if (DXGetObjectClass((Object)component) != CLASS_ARRAY)
      goto component_done;
    
    if (!strcmp(name,"positions")||
	!strcmp(name,"connections") ||
	!strcmp(name,"invalid positions"))
      goto component_done;
    
    if (DXGetComponentAttribute((Field)input, name, "ref"))
      goto component_done;
    
    attr = DXGetComponentAttribute((Field)input, name, "der");
    if (attr)
      goto component_done;
    
       
    attr = DXGetComponentAttribute((Field)input, name, "dep");
    if (!attr) {
      attr = DXGetComponentAttribute((Field)input, name,"der");
      if (attr)
	goto component_done;
      /* it doesn't ref, doesn't dep, and doesn't der.
	 Copy it to the output and quit */
      if (!DXSetComponentValue((Field)pie, name, (Object)component))
	goto error;
      if (!DXSetComponentValue((Field)edges, name, (Object)component))
	goto error;
      goto component_done;
    }
    if (DXGetObjectClass(attr) != CLASS_STRING) {
      DXSetError(ERROR_DATA_INVALID, "dependency attribute not string");
      goto error;
    }

      
    /* we only bother with components which dep the same thing that
       data deps */ 
    if (strcmp(DXGetString((String)attr), DXGetString((String)dataattr)))
      goto component_done; 
    
    componentsdone++;
    if (!DXGetArrayInfo((Array)component, &numitems, &type,
			&category, &rank, shape))
      goto error;
    
    dataptr = (char *)DXGetArrayData(component);
    if (!dataptr)
      goto error;
    
    size = DXGetItemSize(component);
    dataptr+=index*size;
    
    newwedgescomponent = (Array)DXNewConstantArrayV(numwedges, 
						    dataptr, 
						    type, 
						    category, rank, shape);
    if (!DXSetComponentValue(pie,name, (Object)newwedgescomponent))
      goto error;
    newwedgescomponent = NULL;
    
    newedgescomponent = (Array)DXNewConstantArrayV(numedges, 
						   dataptr, 
						   type, 
						   category, rank, shape);
    if (!DXSetComponentValue(edges,name, (Object)newedgescomponent))
      goto error;
    newwedgescomponent = NULL;
    if (!DXSetComponentAttribute((Field)pie, name,
                                 "dep",
                                 (Object)DXNewString("connections")))
      goto error;
    if (!DXSetComponentAttribute((Field)edges, name,
                                 "dep",
                                 (Object)DXNewString("connections")))
      goto error;

component_done:
    compcount++;
    
  
  }


  return OK;
error:
  DXDelete((Object)newwedgescomponent);
  DXDelete((Object)newedgescomponent);
  return ERROR;
}

static Error AddData(float *percentval, Field pie, Field edges, 
                     int numitemspie, int numitemsedges)
{
  
  Array piedata=NULL, edgesdata=NULL;
  


  piedata = (Array)DXNewConstantArray(numitemspie, percentval, 
				      TYPE_FLOAT, 
				      CATEGORY_REAL, 0);
  if (!piedata)
    goto error;
  edgesdata = (Array)DXNewConstantArray(numitemsedges, percentval, 
					TYPE_FLOAT, 
					CATEGORY_REAL, 0);
  if (!edgesdata)
    goto error;
  
  if (!DXSetComponentValue(pie, "data", (Object)piedata))
    goto error;
  piedata=NULL;
  DXSetComponentAttribute(pie,"data", "dep", 
			  (Object)DXNewString("connections"));
  
  if (!DXSetComponentValue(edges, "data", (Object)edgesdata))
    goto error;
  edgesdata=NULL;
  DXSetComponentAttribute(edges,"data", "dep", 
			  (Object)DXNewString("connections"));
  
  
  return OK;
error:
  DXDelete((Object)piedata);
  DXDelete((Object)edgesdata);
  return ERROR;
}


/* this is only called if the height input is non-null */
static Error CheckHeight(Object input, Object heightscale, Object heightmin,
                         Object heightmax, Object heightratio, 
                         int numwedges, int *dim, float **h)
{
  int numitems=1, i;
  float scaleval, minval, maxval, ratioval, m, b;
  Array data;
  
  
  if (!GetParameters(input, heightscale, heightmin, heightmax, heightratio,
                     &scaleval, &minval, &maxval,
                     &ratioval))
    goto error;
  
  
  /* first check that it's an array or field */
  switch (DXGetObjectClass(input)) {
    case (CLASS_ARRAY):

    /* now see how many */
    if (!DXGetArrayInfo((Array)input, &numitems, NULL, NULL, NULL, NULL))
      goto error;
  
    if ((numitems != numwedges)&&(numitems != 1)) {
      DXSetError(ERROR_DATA_INVALID,
  	       "height must be a single value or a list which matches percents");
      goto error;
    }
  
    if (numitems == 1) {
      *h = DXAllocate(sizeof(float));
      if (!*h)
        goto error;
      if (!DXExtractFloat(input, *h)) {
        DXSetError(ERROR_DATA_INVALID, 
  		 "height must be a positive scalar");
        goto error;
      }
      if ((**h < 0)) {
        DXSetError(ERROR_DATA_INVALID, 
  		 "height must be a positive scalar");
        goto error;
      }
      if (**h == 0)
        *dim = 0;
      else
        *dim = 1;
    }
    else {
      *h = DXAllocate(numitems*sizeof(float));
      if (!*h)
        goto error;
      if (!DXExtractParameter(input, TYPE_FLOAT, 1, numitems, *h)) {
        DXSetError(ERROR_DATA_INVALID,
  		 "height must be a single value or a list which matches percents");
        goto error;
      }
      *dim = 2;
    }
    break;

  case (CLASS_FIELD):
    data = (Array)DXGetComponentValue((Field)input,"data");
    if (!data) {
      DXSetError(ERROR_DATA_INVALID,"height field has no data");
      goto error;
    }
    /* now see how many */
    if (!DXGetArrayInfo((Array)data, &numitems, NULL, NULL, NULL, NULL))
      goto error;
  
    if ((numitems != numwedges)&&(numitems != 1)) {
      DXSetError(ERROR_DATA_INVALID,
  	       "height must be a single value or a list which matches percents");
      goto error;
    }
  
    if (numitems == 1) {
      *h = DXAllocate(sizeof(float));
      if (!*h)
        goto error;
      if (!DXExtractFloat((Object)data, *h)) {
        DXSetError(ERROR_DATA_INVALID, 
  		 "height must be a positive scalar");
        goto error;
      }
      if ((**h < 0)) {
        DXSetError(ERROR_DATA_INVALID, 
  		 "height must be a positive scalar");
        goto error;
      }
      if (**h == 0)
        *dim = 0;
      else
        *dim = 1;
    }
    else {
      *h = DXAllocate(numitems*sizeof(float));
      if (!*h)
        goto error;
      if (!DXExtractParameter((Object)data, TYPE_FLOAT, 1, numitems, *h)) {
        DXSetError(ERROR_DATA_INVALID,
  		 "height must be a single value or a list which matches percents");
        goto error;
      }
      *dim = 2;
    }

    break;

  default: 
    DXSetError(ERROR_DATA_INVALID,
	       "height must be a scalar list or field");
    goto error;
  }
  
  /* autoscaling only makes sense if dim is 2 */
  if (*dim == 2) {
    for (i=0; i<numitems; i++) {
         /* y = mx + b defines the line */
         m = scaleval*(1-ratioval)/(maxval-minval);
         b = scaleval*(ratioval*maxval - minval)/(maxval-minval);
         (*h)[i] = m*(*h)[i] + b;
         if ((*h)[i] < 0) {
           DXSetError(ERROR_DATA_INVALID,
                      "parameters result in negative height");
           goto error;
         }
       }
    }
  return OK;
error:
  return ERROR;
  
}

static Error CheckRadius(Object input, Object radiusscale,
                         Object radiusmin, Object radiusmax, 
                         Object radiusratio, 
                         int numwedges, int *radtype, 
                         float **radius)
{
  int numitems=1, rank, shape[32],i;
  float scaleval, minval, maxval, ratioval, m,b;
  Type type;
  Category category;
  Array data;
  
  
 
  /* check the radius input */
  if (input) {
    if (!GetParameters(input, radiusscale, radiusmin, radiusmax, radiusratio, 
                       &scaleval, &minval, &maxval, 
                       &ratioval))
      goto error;
    /* check that it's an array or field */
    switch (DXGetObjectClass(input)) {
    case (CLASS_ARRAY):
      /* see how many */
      if (!DXGetArrayInfo((Array)input,&numitems, &type, &category,
			  &rank, shape))
	goto error;
      
      *radius = DXAllocate(numitems*sizeof(float));
      if (!*radius) goto error;
      
      if (numitems == 1) {
	if (!DXExtractFloat(input, *radius)) {
	  DXSetError(ERROR_DATA_INVALID,
		     "radius must be a scalar list ");   
	  goto error;
	}
	if (**radius < 0) {
	  DXSetError(ERROR_DATA_INVALID,
		     "radius must be positive scalars");   
	  goto error;
	}
	*radtype = 0;
      }
      else { 
	if (!DXExtractParameter(input, TYPE_FLOAT, 1, numitems, 
				*radius)) {
	  DXSetError(ERROR_DATA_INVALID,
		     "radius must be a scalar list ");   
	  goto error;
	}
        if (numitems != numwedges) {
          DXSetError(ERROR_DATA_INVALID,
  	       "radius must be a single value or a list which matches percents");
          goto error;
        }
	for (i=0; i<numitems; i++) {
	  if ((*radius)[i] < 0) {
	    DXSetError(ERROR_DATA_INVALID,
		       "radius must be positive scalars");   
	    goto error;
	  }
	}
	*radtype = 1;
      }
      break;
    case (CLASS_FIELD):
      data = (Array)DXGetComponentValue((Field)input,"data");
      if (!data) {
	DXSetError(ERROR_DATA_INVALID, "radius field has no data");
	goto error;
      } 
      /* see how many */
      if (!DXGetArrayInfo((Array)data,&numitems, &type, &category,
			  &rank, shape))
	goto error;
      if (numitems != numwedges) {
        DXSetError(ERROR_DATA_INVALID,
        "radius must be a single value or a list which matches percents");
        goto error;
      }
      
      *radius = DXAllocate(numitems*sizeof(float));
      if (!*radius) goto error;
      
      if (numitems == 1) {
	if (!DXExtractFloat((Object)data, *radius)) {
	  DXSetError(ERROR_DATA_INVALID,
		     "radius must be a scalar list ");   
	  goto error;
	}
	if (**radius < 0) {
	  DXSetError(ERROR_DATA_INVALID,
		     "radius must be positive scalars");   
	  goto error;
	}
	*radtype = 0;
      }
      else { 
	if (!DXExtractParameter((Object)data, TYPE_FLOAT, 1, numitems, 
				*radius)) {
	  DXSetError(ERROR_DATA_INVALID,
		     "radius must be a scalar list ");   
	  goto error;
	}
	for (i=0; i<numitems; i++) {
	  if ((*radius)[i] < 0) {
	    DXSetError(ERROR_DATA_INVALID,
		       "radius must be positive scalars");   
	    goto error;
	  }
	}
	*radtype = 1;
      }
      
      break;
      
    default:
      DXSetError(ERROR_DATA_INVALID,"radius must be a scalar list or field");
      goto error;
    }
    /* now scale the values so that radiusmaxval -> radiusscale, 
     * radiusminval -> radiusscale*ratio 
     */
    if (numitems == 1) {
      **radius = maxval;
      *radtype = 0;
    }
    else {
       if ((maxval - minval) == 0) {
         DXSetError(ERROR_DATA_INVALID,"radiusmax and radiusmin are equal");
         goto error;
       }
    
       for (i=0; i<numitems; i++) {
         /* y = mx + b defines the line */
         m = scaleval*(1-ratioval)/(maxval-minval);
         b = scaleval*(ratioval*maxval - minval)/(maxval-minval);
         (*radius)[i] = m*(*radius)[i] + b;
         if ((*radius)[i] < 0) {
	   DXSetError(ERROR_DATA_INVALID, 
	   	      "parameters result in negative radius");
	   goto error;
         }
       }
    }
  }
  /* user didn't specify a radius; just make it 1 */
  else {
    *radius = DXAllocate(sizeof(float));
    if (!*radius) goto error;
    **radius = 1.0;
    *radtype = 0;
  }
  return OK;


error:
  return ERROR;
}


static Error ScaleLabels(Object labels, int radtype, float *radius)
{
  int shape[32], i, numitems;
  Point *pos3d, *newpos3d;
  Point2D *pos2d, *newpos2d;
  Array positions, newpositions = NULL;
  float angle, rad;
  
  positions = (Array)DXGetComponentValue((Field)labels,"positions");
  DXGetArrayInfo(positions, &numitems, NULL, NULL, NULL, shape);
  newpositions = DXNewArrayV(TYPE_FLOAT, CATEGORY_REAL, 1, shape);
  if (!newpositions) goto error;
  if (!DXAddArrayData(newpositions, 0, numitems, NULL))
    goto error; 
  if (shape[0]==2) {
    pos2d = (Point2D *)DXGetArrayData(positions);
    newpos2d = (Point2D *)DXGetArrayData(newpositions);
    for (i=0; i<numitems; i++) {
      angle = atan2(pos2d[i].y, pos2d[i].x);
      rad = sqrt(pos2d[i].y*pos2d[i].y + pos2d[i].x*pos2d[i].x);
      if (radtype == 1)
	rad = radius[i]*rad;
      else
	rad = radius[0]*rad;
      newpos2d[i].y = rad*sin(angle);
      newpos2d[i].x = rad*cos(angle);
    } 
  }
  else if (shape[0]==3) {
    pos3d = (Point *)DXGetArrayData(positions);
    newpos3d = (Point *)DXGetArrayData(newpositions);
    for (i=0; i<numitems; i++) {
      angle = atan2(pos3d[i].y, pos3d[i].x);
      rad = sqrt(pos3d[i].y*pos3d[i].y + pos3d[i].x*pos3d[i].x);
      if (radtype == 1)
	rad = radius[i]*rad;
      else
	rad = radius[0]*rad;
      newpos3d[i].y = rad*sin(angle);
      newpos3d[i].x = rad*cos(angle);
      newpos3d[i].z = pos3d[i].z;
    } 
  }
  if (!DXSetComponentValue((Field)labels, "positions", 
			   (Object)newpositions))
    goto error;
  newpositions=NULL;
  DXChangedComponentValues((Field)labels,"positions");
  

  
  return OK; 
error:
  DXDelete((Object)newpositions);
  return ERROR;
}


static Error AddColors(Group *pie, Group *edges, int numitems, RGBColor *cptr)
{
  Object member;
  int numcon, i;
  Array newcolors=NULL, connections;
  RGBColor *tmp_ptr;

  /* this routine adds user-given colors to the already created wedges
     and edges */
  tmp_ptr = cptr;

  for (i=0; i<numitems; i++) {
    member = DXGetEnumeratedMember(*pie, i,NULL);
    connections = (Array)DXGetComponentValue((Field)member, "connections");
    if (!DXGetArrayInfo((Array)connections, &numcon, NULL,NULL,NULL,NULL))
      goto error;
    newcolors = (Array)DXNewConstantArray(numcon, 
		       		          &tmp_ptr[i], 
					  TYPE_FLOAT, 
					  CATEGORY_REAL, 1, 3);
    if (!DXSetComponentValue((Field)member, "colors", (Object)newcolors))
      goto error; 
    DXSetComponentAttribute((Field)member,"colors", "dep", 
			    (Object)DXNewString("connections"));
    newcolors=NULL;
    member = DXGetEnumeratedMember(*edges, i,NULL);
    connections = (Array)DXGetComponentValue((Field)member, "connections");
    if (!DXGetArrayInfo((Array)connections, &numcon, NULL,NULL,NULL,NULL))
      goto error;
    newcolors = (Array)DXNewConstantArray(numcon, 
		       		          &tmp_ptr[i], 
					  TYPE_FLOAT, 
					  CATEGORY_REAL, 1, 3);
    if (!DXSetComponentValue((Field)member, "colors", (Object)newcolors))
      goto error;
    DXSetComponentAttribute((Field)member,"colors", "dep", 
			    (Object)DXNewString("connections"));
    newcolors=NULL;
  } 
  return OK;
error:
  DXDelete((Object)newcolors);
  return ERROR;
}

static Array GetLabels(Object labels, Object labelformat)
{
/* this routine looks at the labels input. If they are strings, just copy
and return. Otherwise, convert them to strings using labelformat, if 
given, or with a guess otherwise) */
  
  Array newarray=NULL, data, component;
  int numitems, rank, shape[32];
  Category category;
  Type type;
  char *format;
  Pointer ptr=NULL;

  /* first get labelformat */
  if (labelformat) {
    if (!DXExtractString(labelformat, &format)) {
       DXSetError(ERROR_DATA_INVALID,"labelformat must be a string");
       goto error;
    }
  }
 
 
  /* it has to be either a field or an array */

  switch (DXGetObjectClass(labels)) {
    case (CLASS_FIELD):
      data = (Array)DXGetComponentValue((Field)labels,"data");
      if (!data) {
        DXSetError(ERROR_MISSING_DATA,"labels field is missing data component");
        goto error;
      }
      component = data;
      break;
    case (CLASS_ARRAY):
      component = (Array)labels;
      break;
    default:
      DXSetError(ERROR_DATA_INVALID,"labels must be a field or a list");
      goto error;
    }

    if (!DXGetArrayInfo(component, &numitems, &type, &category, &rank, shape))
      goto error; 
    if (type == TYPE_STRING) {
       /* just copy the strings to the new array */
       newarray = DXNewArray(TYPE_STRING, CATEGORY_REAL, 1, shape[0]);
       ptr = (Pointer)DXGetArrayData(component);
       if (!ptr)
         goto error;
       if (!newarray)
         goto error;
       if (!DXAddArrayData(newarray, 0, numitems, ptr))
         goto error;
    }
    else {
       if (!labelformat) {
         switch(type) {
           case (TYPE_FLOAT):
           case (TYPE_DOUBLE):
             format = "%f";
             break;
           case (TYPE_INT):
           case (TYPE_SHORT):
           case (TYPE_BYTE):
           case (TYPE_UINT):
           case (TYPE_USHORT):
           case (TYPE_UBYTE):
             format = "%d";
           default:
             break;
         }
       }
       switch (type) {
           case (TYPE_FLOAT):
              FORMAT_LIST(float);
              break;
           case (TYPE_INT):
              FORMAT_LIST(int);
              break;
           case (TYPE_UINT):
              FORMAT_LIST(uint);
              break;
           case (TYPE_DOUBLE):
              FORMAT_LIST(double);
              break;
           case (TYPE_SHORT):
              FORMAT_LIST(short);
              break;
           case (TYPE_USHORT):
              FORMAT_LIST(ushort);
              break;
           case (TYPE_BYTE):
              FORMAT_LIST(byte);
              break;
           case (TYPE_UBYTE):
              FORMAT_LIST(ubyte);
              break;
           default:
              DXSetError(ERROR_DATA_INVALID,"unknown type for labels");
              goto error;
    }

  }
  return newarray;
  
error:
  DXDelete((Object)newarray);
  return NULL;
}

static int LabelsAreUnique(Array labellist)
{
  int numitems, i, j;
  char *string1, *string2;


  if (!DXGetArrayInfo(labellist, &numitems, NULL, NULL, NULL, NULL))
    return 0;
  for (i=0; i<numitems; i++) {
    if (!DXExtractNthString((Object)labellist, i, &string1))
       return 0; 
    for (j=0; j<numitems; j++) {
       if (!DXExtractNthString((Object)labellist, j, &string2))
          return 0; 
      if (i != j) {
        if (!strcmp(string1, string2))
          return 0;
      } 
    }
  }
  return 1;

}



/*
 * given some values as input, get min and max for scaling */
static Error GetParameters(Object input, Object scale, 
                           Object min, Object max,
                           Object ratio, 
                           float *scaleval, float *minval, 
                           float *maxval, float *ratioval)
{
   int maxset=0;

   if (!scale) 
      *scaleval = 1;
   else {
      if (!DXExtractFloat(scale, scaleval))
         goto error;
   }

   if (!min) {
      if (!DXStatistics(input, "data", minval, NULL, NULL, NULL))
        goto error;
   }
   else {
      if (!DXExtractFloat(min, minval)) {
         /* XXX should do better checking here e.g. string data? */
         if (!DXStatistics(input, "data", minval, maxval, NULL, NULL))
            goto error;
         maxset = 1;
      }
   }

   if ((!max)&&(!maxset)) {
      if (!DXStatistics(input, "data", NULL, maxval, NULL, NULL))
        goto error;
   }
   else {
      if (!DXExtractFloat(max, maxval)) {
         if (!DXStatistics(input, "data", NULL, maxval, NULL, NULL))
            goto error;
      }
   }

   if (!ratio) 
      *ratioval = .1;
   else {
      if (!DXExtractFloat(ratio, ratioval))
         goto error;
   }

   return OK;
error:
   return ERROR;



}
