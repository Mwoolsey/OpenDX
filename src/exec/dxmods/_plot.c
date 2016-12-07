/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#include <math.h>
#include <stdio.h>
#include <string.h>
#include <dx/dx.h>
#include "plot.h" 

#define EPSILON 0.01    /* distance away from face for marks */

#define LABELSCALE 1.2    /* scalefactor for axes labels rel to tic labels */
#define LABELOFFSET 1.1    /* scalefactor for axes labels rel to tic labels */

#define MAXWIDTH 7   /* switch to exponential notation */
#define XYZ     0.1     /* cross in middle of cube */
#define XY0     0.05    /* cross in middle of face, toward corner */
#define XY1     0.05    /* cross in middle of face, toward edge */
#define X0      0.05    /* tick mark in corner of cube */
#define X1      0.08    /* tick mark at edge of cube */
#define MAJOR   0.05    /* default major tick proportion of axis length */
#define MAJORMIN  6     /* minimum major tick length in pixels */
#define MINOR   0.4     /* minor tick length is this times major tick length */
#define FS      0.3     /* font scaled by FS times minimum delta */
#define FMAX    12      /* maximun font scale */
#define OFFSET  0.7     /* tick labels are offset by OFFSET times font ht */

static RGBColor TICKCOLOR = {1.0, 1.0, 0.0};
static RGBColor LABELCOLOR = {1.0, 1.0, 1.0};
static RGBColor AXESCOLOR = {1.0, 1.0, 0.0};
static RGBColor GRIDCOLOR = {0.3, 0.3, 0.3};
static RGBColor BACKGROUNDCOLOR = {0.0, 0.0, 0.0};


#define ABS(x) ((x)<0? -(x) : (x))
#define SGN(x) ((x)<0? -1 : 1)

#define E .0001                                         /* fuzz */
#define FL(x) ((x)+E>0? (int)((x)+E) : (int)((x)+E)-1)  /* fuzzy floor */
#define CL(x) ((x)-E>0? (int)((x)-E)+1 : (int)((x)-E))  /* fuzzy ceiling */

#define BETWEEN(a, x, b) (((a)<(x) && (x)<(b)) || ((b)<(x) && (x)<(a)))

/*
 * Macro to do a line segment, adding points,
 * of the given color.
 */

#define LINE(rgb, x0,y0,z0, x1,y1,z1) { \
    Line line;                          \
    line.p = pt;                        \
    line.q = pt+1;                      \
    DXAddLine(f, ln++, line);             \
    DXAddPoint(f, line.p, DXPt(x0,y0,z0));  \
    DXAddPoint(f, line.q, DXPt(x1,y1,z1));  \
    DXAddColor(f, line.p, rgb);           \
    DXAddColor(f, line.q, rgb);           \
    pt += 2;                            \
}
extern Error _dxfGetFormat(float *, char *, int, float); /* from libdx/axes.c */
extern Error _dxfCheckLocationsArray(Array, int *, float **); /* from libdx/axes.c */

static Error _dxfSetTextColor(Field f, RGBColor color)
{
  Array pos, col;
  int numpos;

  pos = (Array)DXGetComponentValue(f,"positions");
  DXGetArrayInfo(pos,&numpos,NULL,NULL,NULL,NULL);
  col = (Array)DXNewConstantArray(numpos, (Pointer)&color, TYPE_FLOAT,
                           CATEGORY_REAL, 1, 3);
  DXSetComponentValue(f,"colors",(Object)col);
  return OK;

} 


static 
  Error DrawBackground(Group g,		/* group to hold result */
               RGBColor color,          /* background color */ 
	       Point o,			/* origin */
       	       Vector s                 /* size */
  )
  {
  Field f; 
  Array positions=NULL, connections=NULL, colors=NULL;
  float *pos_ptr;
  int *con_ptr;

  
  f = DXNewField();
  if (!f) goto error;
  DXSetFloatAttribute((Object)f, "fuzz", -4); 
  

  positions = DXNewArray(TYPE_FLOAT,CATEGORY_REAL,1,2);
  if (!DXAddArrayData(positions, 0, 4, NULL))
     goto error;
  pos_ptr = (float *)DXGetArrayData(positions);
  
  connections = DXNewArray(TYPE_INT,CATEGORY_REAL,1,4);
  if (!DXAddArrayData(connections, 0, 1, NULL))
     goto error;
  con_ptr = (int *)DXGetArrayData(connections);

  pos_ptr[0]=o.x;
  pos_ptr[1]=o.y;
  
  pos_ptr[2]=o.x;
  pos_ptr[3]=o.y+s.y;

  pos_ptr[4]=o.x+s.x;
  pos_ptr[5]=o.y;

  pos_ptr[6]=o.x+s.x;
  pos_ptr[7]=o.y+s.y;

  con_ptr[0]=0;
  con_ptr[1]=1;
  con_ptr[2]=2;
  con_ptr[3]=3;
 
  /* set the color */
  colors = (Array)DXNewConstantArray(4, &color, TYPE_FLOAT,
                                     CATEGORY_REAL, 1, 3);
  if (!colors) goto error;

  DXSetComponentValue(f,"positions",(Object)positions);
  positions=NULL;
 
  DXSetComponentValue(f,"connections",(Object)connections);
  connections=NULL;
  DXSetComponentAttribute(f, "connections", "element type",
                          (Object)DXNewString("quads"));
 
  DXSetComponentValue(f,"colors",(Object)colors);
  colors=NULL;
 
  /* end field */
  if (!DXEndField(f)) goto error;
  DXSetMember(g, NULL, (Object)f);
  f=NULL;
  
  DXDelete((Object)positions);
  DXDelete((Object)connections);
  DXDelete((Object)colors);
  DXDelete((Object)f);
  return OK;
  
 error:
  DXDelete((Object)positions);
  DXDelete((Object)connections);
  DXDelete((Object)colors);
  DXDelete((Object)f);
  return ERROR;
}


/* this routine is much like mark in _dxfAutoAxes, but only draws a single axis */
/* it also does not do gridding or framing */
static 
  Error marklinehor(Group g,		/* group to hold result */
	       char *label,		/* axis label */
	       int flipl, 		/* flip the label */
	       int reverse,		/* whteher to reverse labels */
	       Point o,			/* origin */
	       Vector s,	       	/* size */
	       Vector sgn,		/* sign of size */
	       int n,			/* number of tick marks */
               int fliptics, 
	       int sub,			/* number of sub tick marks */
	       double l,	        /* first tick mark on axis */
	       double d,       		/* spacing of tick marks */
	       double ls, 		/* label scaling  */
	       int flip,       		/* whether to flip tick labels */
	       char *fmt, 		/* format for tick labels */
	       double fs,      		/* font scaling for labels */
	       double length,		/* axis length for tick mark calc */
	       int islog,               /* log axes*/
               int putlabels,          /* put on tic labels */
               float minticsize,       /* minimum tic size */
               RGBColor rgb,          /* color of marks */
               int axeslines,          /* draw lines along the tics */
               RGBColor labelcolor,    /* color of axes labels */
               RGBColor axescolor,     /* color of axes */
               float labelscale,       /* scaling for labels */
	       char *fontname, 		/* font for labels */
               float *locs,
               char **labels
	       ) {
  Matrix t;			/* matrix */
  float z;			/* z distance above face for marks */
  double value, range;
  Object font=NULL;		/* font for labels */
  static float ascent;	/* baseline to top of label */
  float maxwidth;		/* max width of axis tick labels */
  float width, widthonechar;	/* width temp variable */
  Group group = NULL;		/* group to hold labels */
  Object text;		/* the label */
  char buf[100];		/* the label */
  float offset;		/* amount to move label so 0's don't collide */
  float hdir, vdir;		/* direction of tick label compared to axis */
  Field f = NULL;		/* field for marks */
  Xform xform = NULL;		/* various transformed objects */
  int pt=0, ln=0;		/* current point, line number within f */
  float x, y, a, b, major, minor, xx;
  int i, j;
  char *tempstring=NULL;
  int createdlocs = 0, createdlabels = 0;

  z = o.z /*+EPSILON*s.z*/;	/* z distance above face for marks */
  
  group = DXNewGroup();
  if (!group) goto error;
  f = DXNewField();
  if (!f) goto error;
  /* DXSetFloatAttribute((Object)f, "fuzz", 4); */
  DXSetMember(group, NULL, (Object)f);
  
  /* x-axis tick mark length, based on y-axis size */
  major = MAJOR;
  length = ABS(s.y);
  if ((major*length < minticsize)&&(length > 0.0))
    major = minticsize/length;
  minor = major * MINOR;

  /* create a list of tic mark locations */

  
  /* tick marks on axis */
  a = o.x - .001*s.x;
  b = o.x + 1.001*s.x;
  if (!locs) {
     createdlocs = 1;
     locs = (float *)DXAllocate(n*sizeof(float));
     if (!locs)
       goto error;
     for (i=0; i<n; i++)
        locs[i] = l + i*d;
  }
  if (!labels) {
     createdlabels = 1;
     labels = (char **)DXAllocate(n*sizeof(char *));
     if (!labels)
       goto error;
     for (i=0; i<n; i++) {
        tempstring = (char *)DXAllocate(100*sizeof(char));
        sprintf(tempstring, fmt, locs[i]*ls);
        labels[i] = tempstring;
     }
   }


  for (i=0; i<n; i++) {
    x = locs[i]; 
    if (BETWEEN(a, x, b)) {
      if (!fliptics) {
         LINE(rgb,  x, o.y+s.y, z,  x, o.y+(1-major)*s.y, z);
      }
      else {
         LINE(rgb,  x, o.y+s.y, z,  x, o.y+(1+major)*s.y, z);
      }
    }
    if (!islog) {
      /* minor ticks */
      if (i < n-1)
	for (j=0; j<sub; j++) {
	  xx = x + j*d/sub;
	  if (BETWEEN(a, xx, b)) {
            if (!fliptics) {
	      LINE(rgb,  xx, o.y+s.y, z,  xx, o.y+(1-minor)*s.y, z);
            }
            else {
	      LINE(rgb,  xx, o.y+s.y, z,  xx, o.y+(1+minor)*s.y, z);
            }
	  }
	}
    }
    else {
      if (i < n-1)
	for (j=2; j<=9; j++) {
	  if (d >= 0)
	    xx = x + (float)log10((double)j)*d;
	  else 
	    xx = x + d - (float)log10((double)j)*d;
	  if (BETWEEN(a, xx, b)) {
            if (!fliptics) {
	      LINE(rgb,  xx, o.y+s.y, z,  xx, o.y+(1-minor)*s.y, z);
            }
            else {
	      LINE(rgb,  xx, o.y+s.y, z,  xx, o.y+(1+minor)*s.y, z);
            }
	  }
	}
    }
  }
  /* draw the line that goes with it */
  if (axeslines)
    LINE(axescolor, o.x, o.y+s.y, o.z, o.x+s.x, o.y+s.y, o.z);
  
  if (putlabels) { 
    /* use fixed-width font here */
    if (strcmp(fontname,"standard"))
      font = DXGetFont(fontname, &ascent, NULL);
    else
      font = DXGetFont("fixed", &ascent, NULL);
    if (!font)
      goto error;
    /* offset =  -.5*ascent*fs*labelscale; */
    offset = 0;
    
  
    sprintf(buf,"%s","M"); 
    text = DXGeometricText(buf, font, &widthonechar); 
    DXDelete((Object)text);
    /* axis tick labels */
    a = o.x - .001*s.x;
    b = o.x + 1.001*s.x;
    for (i=0, maxwidth=0; i<n; i++) {
      if (!BETWEEN(a, locs[i], b))
	continue;

      /* need to check for essentially zero */
      value = (locs[i]) * ls;
      range = ABS(n*d*ls);    
      if (ABS(value) < range/100000.) value =0;

      if (!islog)
	sprintf(buf, "%s", labels[i]);
      else 
	sprintf(buf, fmt, pow(10.0,(double)((l + i*d) * ls))); 
      text = DXGeometricText(buf, font, &width);
      _dxfSetTextColor((Field)text, labelcolor);
      hdir = sgn.z * flip;
      vdir = flip;
      x = locs[i] + 
           sgn.x*offset + (sgn.x*vdir>0? sgn.x*ascent*fs*labelscale
                                                 : 0);
      if (!fliptics) 
        y = o.y + s.y + sgn.y*fs*labelscale*widthonechar + 
             (sgn.y*hdir>0? 0 : sgn.y*fs*labelscale*width);
      else
        y = o.y + s.y + sgn.y*fs*labelscale*widthonechar + 
             major*s.y +
             (sgn.y*hdir>0? 0 : sgn.y*fs*labelscale*width);
      t = DXScale(hdir*fs*labelscale, vdir*fs*labelscale, 1);
      t = DXConcatenate(t, DXRotateZ(90*DEG));
      t = DXConcatenate(t, DXTranslate(DXVec(x, y, z)));
      xform = DXNewXform((Object)text, t);
      if (!xform) goto error;
      DXSetMember(group, NULL, (Object)xform);
      if (width>maxwidth)
	maxwidth = width;
    }
    DXDelete(font);
  }
  else {
    maxwidth=0.0;
  }
  
  /* label */
  if (label) {
    /* use proportional font here */
    if (strcmp(fontname,"standard"))
      font = DXGetFont(fontname, &ascent, NULL);
    else
      font = DXGetFont("variable", &ascent, NULL);
    if (!font)
      goto error;
    if (strcmp(label,"")) {
      text = DXGeometricText(label, font, &width);
      _dxfSetTextColor((Field)text, labelcolor);
      x = o.x + s.x/2 - LABELSCALE*flipl*sgn.x*fs*labelscale*width/2 *reverse;
      if (!fliptics) 
         y = o.y + s.y + sgn.y*fs*(1 + labelscale*(maxwidth*LABELOFFSET) +
                                ascent*labelscale);
      else
         y = o.y + s.y + major*s.y + sgn.y*fs*(1 + labelscale*(maxwidth*LABELOFFSET) +
                                ascent*labelscale);

      t = DXScale(LABELSCALE*flipl*sgn.x*fs*labelscale *reverse, 
                  LABELSCALE*flipl*sgn.y*fs*labelscale, 0);
      t = DXConcatenate(t, DXTranslate(DXVec(x, y, z)));
      xform = DXNewXform((Object)text, t);
      if (!xform) goto error;
      DXSetMember(group, NULL, (Object)xform);
    }
  }
  
  /* end field */
  if (!DXEndField(f)) goto error;
  
  DXSetMember(g, NULL, (Object)group);
  DXSetFloatAttribute((Object)group, "fuzz", 4);
  
  /* all ok */
  DXDelete(font);
  if (createdlocs) {
     DXFree((Pointer)locs);
  }
  if (createdlabels) {
     for (i=0;i<n;i++)
        DXFree((Pointer)labels[i]);
     DXFree((Pointer)labels);
  }
  return OK;
  
 error:
  if (createdlocs) {
     DXFree((Pointer)locs);
  }
  if (createdlabels) {
     for (i=0;i<n;i++)
        DXFree((Pointer)labels[i]);
     DXFree((Pointer)labels);
  }
  DXDelete(font);
  DXDelete((Object)group); 
  return ERROR;
}


static 
  Error markgridver(Group group, Point o, Vector s, int n, int islog,
                    float l, float d, RGBColor rgb, float *locptr)
{
  float a, b, z, x;
  Field f=NULL;
  int i, ln=0, pt=0;
    
  z = o.z;
  
  f = DXNewField();
  if (!f) goto error;
  DXSetFloatAttribute((Object)f, "fuzz", -2);
  DXSetMember((Group)group, NULL, (Object)f);
  
  if (!islog) {
    /* grid marks on axis */
    a = o.x - .001*s.x;
    b = o.x + 1.001*s.x;
    for (i=0; i<n; i++) {
      /* grid along major ticks */
      if (!locptr) 
         x = l + i*d;
      else
         x = locptr[i];
      if (BETWEEN(a, x, b)) {
	LINE(rgb,  x, o.y+s.y, z,  x, o.y, z);
      }
    }
  }
  /* else log scale */
  else {
    /* grid marks on axis */
    a = o.x - .001*s.x;
    b = o.x + 1.001*s.x;
    for (i=0; i<n; i++) {
      x = l + i*d;
      if (BETWEEN(a, x, b)) {
	LINE(rgb,  x, o.y+s.y, z,  x, o.y, z);
      }
    }
  }
  
  /* end field */
  if (!DXEndField(f)) goto error;
  
  return OK;
  
 error:
  DXDelete((Object)f);
  return ERROR;
}

static 
  Error markgridhor(Group group, Point o, Vector s, int n, int islog,
                    float l, float d, RGBColor rgb, float *locptr)
{
  float a, b, z, x;
  Field f=NULL;
  int i, ln=0, pt=0;


    
  z = o.z;
  
  f = DXNewField();
  if (!f) goto error;
  DXSetFloatAttribute((Object)f, "fuzz", -2);
  DXSetMember(group, NULL, (Object)f);
  
  if (!islog) {
    /* grid marks on axis */
    a = o.y - .001*s.y;
    b = o.y + 1.001*s.y;
    for (i=0; i<n; i++) {
      /* grid along major ticks */
      if (!locptr) 
         x = l + i*d;
      else
         x = locptr[i];
      if (BETWEEN(a, x, b)) {
	LINE(rgb,  o.x+s.x, x, z,  o.x, x, z);
      }
    }
  }
  /* else log scale */
  else {
    /* grid marks on axis */
    a = o.y - .001*s.y;
    b = o.y + 1.001*s.y;
    for (i=0; i<n; i++) {
      x = l + i*d;
      if (BETWEEN(a, x, b)) {
	LINE(rgb, o.x+s.x, x, z,  o.x, x, z);
      }
    }
  }
  
  /* end field */
  if (!DXEndField(f)) goto error;
  
  return OK;
  
 error:
  DXDelete((Object)f);
  return ERROR;
}


/* this routine is much like mark in _dxfAutoAxes, but only draws a single axis */
/* it also does not do gridding or framing */
static 
  Error marklinever(Group g,		/* group to hold result */
	       char *label,		/* axis label */
	       int flipl, 		/* flip the label */
	       int reverse,		/* whteher to reverse labels */
	       Point o,			/* origin */
	       Vector s,	       	/* size */
	       Vector sgn,		/* sign of size */
	       int n,			/* number of tick marks */
               int fliptics, 
	       int sub,			/* number of sub tick marks */
	       double l,	        /* first tick mark on axis */
	       double d,       		/* spacing of tick marks */
	       double ls, 		/* label scaling  */
	       int flip,       		/* whether to flip tick labels */
	       char *fmt, 		/* format for tick labels */
	       double fs,      		/* font scaling for labels */
	       double length,		/* axis length for tick mark calc */
	       int islog,               /* log axes*/
               int putlabels,           /* put on tic labels */
               float minticsize,        /* minimum tic size */
               RGBColor rgb,            /* color of marks */
               int axeslines,           /* draw axes lines along tics */
               RGBColor labelcolor,      /* color of labels */
               RGBColor axescolor,      /* color of axes */
               float labelscale,        /* scaling for labels */
	       char *fontname,		/* font for labels */
               float *locs,
               char **labels
	       ) {
  Matrix t;			/* matrix */
  float z;			/* z distance above face for marks */
  Object font=NULL;		/* font for labels */
  static float ascent;	/* baseline to top of label */
  float maxwidth;		/* max width of axis tick labels */
  float width, widthonechar;	/* width temp variable */
  Group group = NULL;		/* group to hold labels */
  Object text;		/* the label */
  char buf[100];		/* the label */
  float offset;		/* amount to move label so 0's don't collide */
  double value, range;
  float hdir, vdir;		/* direction of tick label compared to axis */
  Field f = NULL;		/* field for marks */
  Xform xform = NULL;		/* various transformed objects */
  int pt=0, ln=0;		/* current point, line number within f */
  float x, y, a, b, major, minor, xx;
  int i, j;
  char *tempstring=NULL;
  int createdlocs=0, createdlabels=0;

  z = o.z /*+EPSILON*s.z*/;	/* z distance above face for marks */
  
  group = DXNewGroup();
  if (!group) goto error;
  f = DXNewField();
  if (!f) goto error;
  DXSetFloatAttribute((Object)f, "fuzz", 4);
  DXSetMember(group, NULL, (Object)f);
  
  /* x-axis tick mark length, based on y-axis size */
  major = MAJOR;
  length = ABS(s.x);
  if ((major*length < minticsize)&&(length > 0.0))
    major = minticsize/length;
  minor = major * MINOR;
  
  /* tick marks on axis */
  a = o.y - .001*s.y;
  b = o.y + 1.001*s.y;
  if (!locs) {
    createdlocs = 1;
    locs = (float *)DXAllocate(n*sizeof(float));
    if (!locs)
       goto error;
    for (i=0; i<n; i++) 
      locs[i] = l + i*d;
  }
  if (!labels) {
    createdlabels = 1;
    labels = (char **)DXAllocate(n*sizeof(char *));
    if (!labels)
       goto error;
    for (i=0; i<n; i++) {
      tempstring = (char *)DXAllocate(100*sizeof(char));
      sprintf(tempstring, fmt, locs[i]*ls);
      labels[i] = tempstring;
    }
  }
    
  for (i=0; i<n; i++) {
    /* major ticks are evenly spaced for log plot */
    x = locs[i];
    if (BETWEEN(a, x, b)) {
      if (!fliptics) {
	LINE(rgb,  o.x+s.x, x, z,  o.x+(1-major)*s.x, x, z);
      }
      else {
	LINE(rgb,  o.x+s.x, x, z,  o.x+(1+major)*s.x, x, z);
      }
    }
    if (!islog) {
      /* minor ticks are not evenly spaced */
      if (i < n-1)
	for (j=0; j<sub; j++) {
	  xx = x + j*d/sub;
	  if (BETWEEN(a, xx, b)) {
	    if (!fliptics) {
	      LINE(rgb,  o.x+s.x, xx, z,  o.x+(1-minor)*s.x, xx, z);
	    }
	    else {
	      LINE(rgb,  o.x+s.x, xx, z,  o.x+(1+minor)*s.x, xx, z);
	    }
	  }
	}
    }
    else {
      if (i < n-1)
        for (j=2; j<=9; j++) {
          if (d >= 0)
            xx = x + (float)log10((double)j)*d;
          else
            xx = x + d - (float)log10((double)j)*d;
          if (BETWEEN(a, xx, b)) {
            if (!fliptics) {
              LINE(rgb, o.x+s.x,  xx, z,  o.x+(1-minor)*s.x, xx, z);
            }
            else {
              LINE(rgb, o.x+s.x,  xx, z,  o.x+(1+minor)*s.x, xx, z);
            }
	  }
        }
    }
  }
  /* draw the line that goes with it */
  if (axeslines)
    LINE(axescolor, o.x+s.x, o.y, o.z, o.x+s.x, o.y+s.y, o.z);
  
  
  if (putlabels) {
    /* use fixed-width font here */
    if (strcmp(fontname,"standard"))
      font = DXGetFont(fontname, &ascent, NULL);
    else
      font = DXGetFont("fixed", &ascent, NULL);
    if (!font)
      goto error;
    /* offset = -.5*ascent*fs*labelscale; */
    offset = 0;
    
    
    /* axis tick labels */
    a = o.y - .001*s.y;
    b = o.y + 1.001*s.y;
    sprintf(buf,"%s","M");
    text = DXGeometricText(buf,font,&widthonechar);
    DXDelete((Object)text);
    for (i=0, maxwidth=0; i<n; i++) {
      if (!BETWEEN(a, locs[i], b))
	continue;
      
      /* need to check for essentially zero */
      value = (locs[i]) * ls;
      range = ABS(n*d*ls);
      if (ABS(value) < range/100000.) value =0;
      
      if (!islog)
	sprintf(buf, "%s", labels[i]);
      else 
	/* XXX need to deal with scaling later */
	sprintf( buf, fmt, pow(10.0,(double)((l + i*d) * ls))); 
      text = DXGeometricText(buf, font, &width);
      _dxfSetTextColor((Field)text,labelcolor);
      hdir = sgn.z * flip;
      vdir = flip;
      if (!fliptics) 
        x = o.x + s.x + sgn.y*fs*labelscale*widthonechar +
	  (sgn.x*hdir>0? 0 : sgn.x*fs*labelscale*width);
      else
        x = o.x + s.x + major*s.x + sgn.y*fs*labelscale*widthonechar +
	  (sgn.x*hdir>0? 0 : sgn.x*fs*labelscale*width);
      /* here */
      y = locs[i] + sgn.y*offset; 
         /* (sgn.y*vdir>0? 0 : sgn.y*ascent*fs*labelscale); */
      t = DXScale(labelscale*hdir*fs, labelscale*vdir*fs, 1);
      t = DXConcatenate(t, DXTranslate(DXVec(x, y, z)));
      xform = DXNewXform((Object)text, t);
      if (!xform) goto error;
      DXSetMember(group, NULL, (Object)xform);
      if (width>maxwidth)
	maxwidth = width;
    }
    DXDelete(font);
  }
  else {
    maxwidth=0.0;
  }
  
  
  /* label */
  if (label) {
    /* use proportional font here */
    if (strcmp(fontname,"standard"))
      font = DXGetFont(fontname, &ascent, NULL);
    else
      font = DXGetFont("variable", &ascent, NULL);
    if (!font)
      goto error;
    if (strcmp(label,"")) {
      text = DXGeometricText(label, font, &width);
      _dxfSetTextColor((Field)text,labelcolor);
      
      /* need to adust the x position based on labelscale */
      
      if (!fliptics) 
        x = o.x + s.x + sgn.x*fs*(1 + labelscale*(maxwidth*LABELOFFSET)) -
	  flipl*ascent*fs*labelscale; 
      else
        x = o.x + s.x + major*s.x + sgn.x*fs*(1 + labelscale*(maxwidth*LABELOFFSET)) -
	  flipl*ascent*fs*labelscale; 
      y = o.y  + s.y/2 + LABELSCALE*flipl*sgn.y*fs*labelscale*width/2 *reverse;
      t = DXScale(LABELSCALE*flipl*sgn.y*fs*labelscale *reverse, 
                  LABELSCALE*flipl*sgn.x*fs*labelscale, 0);
      t = DXConcatenate(t, DXRotateZ(-90*DEG));
      t = DXConcatenate(t, DXTranslate(DXVec(x, y, z)));
      xform = DXNewXform((Object)text, t);
      if (!xform) goto error;
      DXSetMember(group, NULL, (Object)xform);
    }
  }
  
  /* end field */
  if (!DXEndField(f)) goto error;
  
  DXSetMember(g, NULL, (Object)group);
  DXSetFloatAttribute((Object)group, "fuzz", 4);
  
  /* all ok */
  DXDelete(font);
  if (createdlocs) {
     DXFree((Pointer)locs);
  }
  if (createdlabels) {
     for (i=0;i<n;i++)
        DXFree((Pointer)labels[i]);
     DXFree((Pointer)labels);
  }
  return OK;
  
 error:
  DXDelete(font);
  DXDelete((Object)group); 
  if (createdlocs) {
     DXFree((Pointer)locs);
  }
  if (createdlabels) {
     for (i=0;i<n;i++)
        DXFree((Pointer)labels[i]);
     DXFree((Pointer)labels);
  }
  return ERROR;
}

struct axes_struct{
  Object object;
  char *xlabel;
  char *ylabel;
  float labelscale;
  char *font;
  int t;
  int *tx;
  int *ty;
  Object corners;
  int frame;
  int adjust;
  int grid;
  int islogx;
  int islogy;
  float minticsize;
  int axeslines;
  int justright;
  float *actualcorners;
  RGBColor labelcolor;
  RGBColor ticcolor;
  RGBColor axescolor;
  RGBColor gridcolor;
  RGBColor backgroundcolor;
  int background;
  Array xlocs;
  Array ylocs;
  Array xlabels;
  Array ylabels;
  int dofixedfontsize;
  float fixedfontsize;
};

static float first_axes_fs;

Object _dxfAxes2D(Pointer p) 
{
  Group g=NULL;
  Point box[8], origin, mm[2], min, max;
  float mm2[4], fs, lx=0, ly=0, dx=0, dy=0; 
  double lengthx, lengthy;
  Vector scale, sgn, ls;
  char fmtx[20], fmty[20];
  int i, flipl, flip, reverse, subx=0, suby=0, putlabels;
  int numx, numy;
  float *xp, *yp;
  Field f = NULL;
  int pt=0, ln=0;
  Object object, corners;
  char *xlabel, *ylabel, *fontname;
  int t, *tx, *ty, frame, adjust, grid, islogx, islogy, axeslines, justright;
  int background;
  float minticsize, *actualcorners, labelscale;
  RGBColor ticcolor, labelcolor, axescolor, gridcolor, backgroundcolor;
  Array xlocs, ylocs, xlabels, ylabels;
  float *xptr=NULL, *yptr=NULL;
  char **xlabptr=NULL, **ylabptr=NULL;
  char *tmpstring;
  int nx, ny,numlabels, flipxtics, flipytics;
  Type type;
  int dofixedfontsize;
  float fixedfontsize;

  struct axes_struct *axes_struct = (struct axes_struct *)p;

  object = axes_struct->object;
  corners = axes_struct->corners;
  xlabel = axes_struct->xlabel;
  ylabel = axes_struct->ylabel;
  labelscale = axes_struct->labelscale;
  fontname = axes_struct->font;
  t = axes_struct->t;
  tx = axes_struct->tx;
  ty = axes_struct->ty;
  frame = axes_struct->frame;
  adjust = axes_struct->adjust;
  grid = axes_struct->grid;
  islogx = axes_struct->islogx;
  islogy = axes_struct->islogy;
  axeslines = axes_struct->axeslines;
  justright = axes_struct->justright;
  minticsize = axes_struct->minticsize;
  actualcorners = axes_struct->actualcorners;
  labelcolor = axes_struct->labelcolor;
  ticcolor = axes_struct->ticcolor;
  axescolor = axes_struct->axescolor;
  gridcolor = axes_struct->gridcolor;
  backgroundcolor = axes_struct->backgroundcolor;
  background = axes_struct->background;
  xlocs = axes_struct->xlocs;
  ylocs = axes_struct->ylocs;
  xlabels = axes_struct->xlabels;
  ylabels = axes_struct->ylabels;
  dofixedfontsize = axes_struct->dofixedfontsize;
  fixedfontsize = axes_struct->fixedfontsize;


  /* need to check that they didn't give tick locations with log */
  if ((xlocs)&&(islogx)) {
     DXSetError (ERROR_DATA_INVALID,
                 "x tick locations can not be specified with log x axis");
     goto error;
  }
  if ((ylocs)&&(islogy)) {
     DXSetError (ERROR_DATA_INVALID,
                 "y tick locations can not be specified with log y axis");
     goto error;
  }

  if (DXGetObjectClass(object)==CLASS_XFORM) {
     Matrix m;
     DXGetXformInfo((Xform)object, NULL, &m);
     if (m.A[0][1]==0 && m.A[0][2]==0 &&
         m.A[1][0]==0 && m.A[1][2]==0 &&
         m.A[2][0]==0 && m.A[2][1]==0)
         ls = DXVec(1.0/m.A[0][0], 1.0/m.A[1][1], 1.0/m.A[2][2]);
     else
         ls = DXVec(1, 1, 1);
 } else {
     ls = DXVec(1, 1, 1);
 }

  g = DXNewGroup();
  if (!g) goto error;
  /* DXSetFloatAttribute(object,"fuzz", 4.0);  */
  DXSetMember(g,NULL,(Object)object);

  
  /* here I need to figure out corners in order to get s and o */
    /* obtain min/max */
    if (DXExtractParameter(corners, TYPE_FLOAT, 3, 2, (Pointer)mm)) {
      min.x = mm[0].x;
      min.y = mm[0].y;
      max.x = mm[1].x;
      max.y = mm[1].y;
      /* if it's a log plot, then the corners need to be converted to log */
      if (islogx) {
        if ((min.x > 0.0) && (max.x > 0.0)) {
          min.x = (float)log10((double)min.x);
          max.x = (float)log10((double)max.x);
        }
        else {
          DXSetError(ERROR_BAD_PARAMETER,
                   "negative minimum or maximum x invalid for log axes");
          goto error;
        }
      }
      if (islogy) {
        if ((min.y > 0.0) && (max.y > 0.0)) {
          min.y = (float)log10((double)min.y);
          max.y = (float)log10((double)max.y);
        }
        else {
          DXSetError(ERROR_BAD_PARAMETER,
                   "negative minimum or maximum y invalid for log axes");
          goto error;
        }
      }
      /* scale the corners using the scaling factor */
      /* only for non-log plots */
      if (!islogx) {
        min.x = min.x / ls.x;
        max.x = max.x / ls.x;
      }
      if (!islogy) {
        min.y = min.y / ls.y;
        max.y = max.y / ls.y;
      }
    }
    else if (DXExtractParameter(corners, TYPE_FLOAT, 2, 2, (Pointer)mm2)) {
      min.x = mm2[0];
      min.y = mm2[1];
      max.x = mm2[2];
      max.y = mm2[3];
      /* if it's a log plot, then the corners need to be converted to log */
      if (islogx) {
        if ((min.x > 0.0) && (max.x > 0.0)) {
          min.x = (float)log10((double)min.x);
          max.x = (float)log10((double)max.x);
        }
        else {
          DXSetError(ERROR_BAD_PARAMETER,
                   "negative minimum or maximum x invalid for log axes");
          goto error;
        }
      }
      if (islogy) {
        if ((min.y > 0.0) && (max.y > 0.0)) {
          min.y = (float)log10((double)min.y);
          max.y = (float)log10((double)max.y);
        }
        else {
          DXSetError(ERROR_BAD_PARAMETER,
                   "negative minimum or maximum y invalid for log axes");
          goto error;
        }
      }
      /* scale the corners using the scaling factor */
      /* only for non log */
      if (!islogx) {
        min.x = min.x / ls.x;
        max.x = max.x / ls.x;
      }
      if (!islogy) {
        min.y = min.y / ls.y;
        max.y = max.y / ls.y;
      }
   } else {
      if (corners) {
        if (!DXBoundingBox(corners, box)) {
          if (DXGetError()==ERROR_NONE)
            DXSetError(ERROR_BAD_PARAMETER,
                     "bad corners parameter (has no bounding box)");
          goto error;
        }
        min.x = min.y = min.z = DXD_MAX_FLOAT;
        max.x = max.y = max.z = -DXD_MAX_FLOAT;
        for (i=0; i<8; i++) {
          if (box[i].x < min.x) min.x = box[i].x;
          if (box[i].y < min.y) min.y = box[i].y;
          if (box[i].z < min.z) min.z = box[i].z;
          if (box[i].x > max.x) max.x = box[i].x;
          if (box[i].y > max.y) max.y = box[i].y;
          if (box[i].z > max.z) max.z = box[i].z;
        }
      } else { /* corners were not given; use the actual bounding box */
        if (!DXBoundingBox(object, box)) {
          if (DXGetError()==ERROR_NONE)
            DXSetError(ERROR_BAD_PARAMETER,
                     "bad input parameter (has no bounding box)");
          goto error;
        }
        min.x = min.y = min.z = DXD_MAX_FLOAT;
        max.x = max.y = max.z = -DXD_MAX_FLOAT;
        for (i=0; i<8; i++) {
          if (box[i].x < min.x) min.x = box[i].x;
          if (box[i].y < min.y) min.y = box[i].y;
          if (box[i].z < min.z) min.z = box[i].z;
          if (box[i].x > max.x) max.x = box[i].x;
          if (box[i].y > max.y) max.y = box[i].y;
          if (box[i].z > max.z) max.z = box[i].z;
        }
        /* check whether the bounding box has zero area (all x's
           or all y's the same */
          if (min.x == max.x) {
              if (min.x*ls.x != 0) {
                 min.x = (1.0-ZERO_HEIGHT_FRACTION)*min.x;
                 max.x = (1.0+ZERO_HEIGHT_FRACTION)*max.x;
              }
              else {
                 min.x = -1;
                 max.x = 1;
              }
          }
          if (min.y == max.y) {
              if (min.y*ls.y != 0) {
                 min.y = (1.0-ZERO_HEIGHT_FRACTION)*min.y;
                 max.y = (1.0+ZERO_HEIGHT_FRACTION)*max.y;
              }
              else {
                 min.y = -1;
                 max.y = 1;
              }
          }
        /* if the user gave tick positions, these can override the
           object's bounding box (but should not override given corners */
        if (xlocs)
        {
           if (!DXGetArrayInfo(xlocs, &numx, NULL, NULL, NULL, NULL))
              return ERROR;
           xp = DXGetArrayData(xlocs);
           if (!xp) return ERROR;
           if (xp[0]/ls.x < min.x) min.x = xp[0]/ls.x;
           if (xp[numx-1]/ls.x > max.x) max.x = xp[numx-1]/ls.x;
        }
        if (ylocs)
        {
           if (!DXGetArrayInfo(ylocs, &numy, NULL, NULL, NULL, NULL))
              return ERROR;
           yp = DXGetArrayData(ylocs);
           if (!yp) return ERROR;
           if (yp[0]/ls.y < min.y) min.y = yp[0]/ls.y;
           if (yp[numy-1]/ls.y > max.y) max.y = yp[numy-1]/ls.y;
        }
      }
      /* because bounding box factors in the xform */
      /* need to undo for log plots */
      if (islogx) {
        min.x = min.x*ls.x;
        max.x = max.x*ls.x;
      }
      if (islogy) {
        min.y = min.y*ls.y;
        max.y = max.y*ls.y;
      }
    }
    min.z = 0;
    max.z = 0;

 
    /* XXX - for now, align the box */
    box[0] = DXPt(min.x, min.y, min.z);
    box[1] = DXPt(min.x, min.y, max.z);
    box[2] = DXPt(min.x, max.y, min.z);
    box[3] = DXPt(min.x, max.y, max.z);
    box[4] = DXPt(max.x, min.y, min.z);
    box[5] = DXPt(max.x, min.y, max.z);
    box[6] = DXPt(max.x, max.y, min.z);
    box[7] = DXPt(max.x, max.y, max.z);

   

    /* origin and scale */
    origin = box[0];
    scale = DXSub(box[7], origin);
    sgn.x = SGN(scale.x);
    sgn.y = SGN(scale.y);
    sgn.z = SGN(scale.z);


    /* First deal with xlocs, ylocs, and xlabels, ylabels */
    /* Redo origin and s if xlocs etc are given */
    if (xlocs) {
      if (!_dxfCheckLocationsArray(xlocs, &nx, &xptr))
        goto error;
      subx = 0;
      if (xlabels) {

         if (!DXGetArrayInfo((Array)xlabels, &numlabels,
                              &type,NULL, NULL,NULL))
           goto error;
         if (numlabels != nx) {
           DXSetError(ERROR_DATA_INVALID,
                      "number of xlabels must match number of xlocations");
           goto error;
         }
         if (type != TYPE_STRING) {
           DXSetError(ERROR_DATA_INVALID,"xlabels must be a string list");
           goto error;
         }
         xlabptr = (char **)DXAllocate(nx*sizeof(char *));
         for (i = 0; i< nx; i++) {
             if (!DXExtractNthString((Object)xlabels, i, &tmpstring)) {
                DXSetError(ERROR_DATA_INVALID,"invalid xlabels");
                goto error;
             }
             xlabptr[i] = tmpstring;
          }
      }
    }
   
    if (ylocs) {
      if (!_dxfCheckLocationsArray(ylocs, &ny, &yptr))
        goto error;
      suby = 0;
      if (ylabels) {

         if (!DXGetArrayInfo((Array)ylabels, &numlabels,
                              &type,NULL, NULL,NULL))
           goto error;
         if (numlabels != ny) {
           DXSetError(ERROR_DATA_INVALID,
                      "number of ylabels must match number of ylocations");
           goto error;
         }
         if (type != TYPE_STRING) {
           DXSetError(ERROR_DATA_INVALID,"ylabels must be a string list");
           goto error;
         }
         ylabptr = (char **)DXAllocate(ny*sizeof(char *));
         for (i = 0; i< ny; i++) {
             if (!DXExtractNthString((Object)ylabels, i, &tmpstring)) {
                DXSetError(ERROR_DATA_INVALID,"invalid ylabels");
                goto error;
             }
             ylabptr[i] = tmpstring;
          }
      }
    }


   /* font scaling is related to minimum delta, and limited by max length */

  /* number of tick labels on each axis */
  flipxtics = 0;
  flipytics = 0;
  if (t < 0) {
     flipxtics = 1;
     flipytics = 1;
     t = -t;
  }
  if (*tx < 0) {
     flipxtics = 1;
     *tx = -(*tx);
  }
  if (*ty < 0) {
     flipytics = 1;
     *ty = -(*ty);
  }
  _dxfHowManyTics(islogx, islogy, ls.x, ls.y, scale.x, scale.y, t, tx, ty);
  if (xlocs) *tx = nx;
  if (ylocs) *ty = ny;

  /* figure out the deltas along each axis */
  /* deltas - may change origin and scale, so do this before faces */
  if (*tx>0) {
    if (!xlocs)
      _dxfaxes_delta(&origin.x, &scale.x, ls.x, tx, &lx, &dx, fmtx, &subx, 
 	             adjust, islogx);
    else {
      if (!xlabels) _dxfGetFormat(xptr, fmtx, nx, ls.x);
      for (i=0; i<nx; i++) {
          xptr[i]=xptr[i]/ls.x;
      }
      origin.x = min.x; 
      scale.x = (max.x-min.x);

      lx = xptr[0];
      if (nx != 1)
        dx = scale.x/(nx-1);
      else
        dx = scale.x;
    }
    if (!islogx) {
      if (actualcorners) {
          actualcorners[0] = origin.x;
          actualcorners[2] = origin.x+scale.x;
      }
    }
    else {
      if (actualcorners) {
        actualcorners[0] = origin.x*ls.x;
        actualcorners[2] = (origin.x+scale.x)*ls.x;
      }
    }
  }
  if (*ty>0) {
    if (!ylocs)
      _dxfaxes_delta(&origin.y, &scale.y, ls.y, ty, &ly, &dy, fmty, &suby, 
 	             adjust, islogy);
    else {
      if (!ylabels) _dxfGetFormat(yptr, fmty, ny, ls.y);
      for (i=0; i<ny; i++) {
          yptr[i]=yptr[i]/ls.y;
      }
      origin.y = min.y;
      scale.y = (max.y-min.y);
 
      ly = yptr[0];
      if (ny != 1) 
        dy = scale.y/(ny-1);
      else
        dy = scale.y;
    }
    if (!islogy) {
      if (actualcorners) {
        actualcorners[1] = origin.y;
        actualcorners[3] = origin.y+scale.y;
      }
    }
    else {
      if (actualcorners) {
        actualcorners[1] = origin.y*ls.x;
        actualcorners[3] = (origin.y+scale.y)*ls.x;
      }
    }
  }
  
   if (*tx==0) dx = 1e20; /* XXX */
   if (*ty==0) dy = 1e20; /* XXX */


   if (dofixedfontsize) {
      fs = ABS(scale.x)*fixedfontsize;
   } else {
   /* If we're just doing the right hand side, then use the size already
      decided on by the left hand side */
     if (!justright) {
       if ((*tx !=0)||(*ty != 0))
          fs = FS*MIN(ABS(dx),ABS(dy));
       else {
          fs = FS*MIN(ABS(scale.y), ABS(scale.x));
          first_axes_fs = fs;
       }
     } else 
         fs = first_axes_fs;
   }


  if (!justright) {
  /* labels left, bottom */
  if (sgn.x  > 0)
    origin.x += scale.x, scale.x = -scale.x, sgn.x = -sgn.x;
  if (sgn.y > 0)
    origin.y += scale.y, scale.y = -scale.y, sgn.y = -sgn.y;
  }


  flip = 1;
  reverse = 1;
  lengthx = scale.x;
  lengthy = scale.y;
  putlabels = 1;
 
  /* draw the background */ 
  if (background) { 
    if (!DrawBackground(g, backgroundcolor, origin, scale))
      goto error;
  } 


  /* first draw the x axis */
  flipl = -1;
  if (!justright) {
    if (!marklinehor(g, xlabel, flipl, reverse, origin, scale, sgn, *tx, 
                     flipxtics, 
                     subx, lx, dx, ls.x, flip, fmtx, fs, lengthy,
                     islogx, putlabels, minticsize, ticcolor, axeslines,
                     labelcolor, axescolor, labelscale, fontname, xptr,
                     xlabptr))
      goto error;
  }
 
  /* now draw the y axis */
  if (!justright) 
    flipl = 1;
  else 
    flipl = -1;
  if (!marklinever(g, ylabel, flipl, reverse, origin, scale, sgn, *ty, 
                   flipytics, 
                   suby, ly, dy, ls.y, flip, fmty, fs, lengthx, 
                   islogy, putlabels, minticsize, ticcolor, axeslines,
                   labelcolor, axescolor, labelscale, fontname, yptr,
                   ylabptr))
    goto error;

  if ((grid==1)||(grid==3)) {
    /* horizontal grid lines */
    /* need to fix this */
    if (!markgridhor(g, origin, scale, *ty, islogy,
                     ly, dy, gridcolor, yptr))
       goto error;
  }
  if ((grid==2)||(grid==3)) {
    /* vertical grid lines */
    if (!markgridver(g, origin, scale, *tx, islogx,
                     lx, dx, gridcolor, xptr))
       goto error;
  }

  /* do the frame */
  if (frame == 1) {
    /* just the framing lines, no tics */
    f = DXNewField();
    if (!f) goto error;
    LINE(axescolor, origin.x, origin.y, origin.z, 
         origin.x, origin.y+scale.y, origin.z);
    LINE(axescolor, origin.x, origin.y, origin.z, 
         origin.x+scale.x, origin.y, origin.z);
  }
  else if (frame == 2) {
    /* first draw the x axis */
    origin = DXAdd(origin, scale);
    scale = DXNeg(scale);
    putlabels = 0;
    if (!marklinehor(g, "", flipl, reverse, origin, scale, sgn, *tx, 
                     flipxtics, subx, 
  		     lx, dx, ls.x, flip, fmtx, fs, lengthy, islogx,
                     putlabels, minticsize, ticcolor, axeslines,labelcolor,
                     axescolor, labelscale, fontname, xptr, xlabptr))
      goto error;
    /* now draw the y axis */
    if (!marklinever(g, "", flipl, reverse, origin, scale, sgn, *ty, 
                     flipytics, suby, 
  		     ly, dy, ls.y, flip, fmty, fs, lengthx, islogy,
                     putlabels, minticsize, ticcolor, axeslines,labelcolor,
                     axescolor, labelscale, fontname, yptr, ylabptr))
      goto error;
  }
  /* there's a second y axis */
  else if (frame == 3) {
    /* first draw the x axis */
    origin = DXAdd(origin, scale);
    scale = DXNeg(scale);
    putlabels = 0;
    if (!marklinehor(g, "", flipl, reverse, origin, scale, sgn, *tx, 
                     flipxtics, subx, 
  		     lx, dx, ls.x, flip, fmtx, fs, lengthy, islogx,
                     putlabels, minticsize, ticcolor, axeslines,labelcolor,
                     axescolor,labelscale, fontname, xptr, xlabptr))
      goto error;
  }

  DXSetMember(g,NULL,(Object)f);

  if (xlabptr) {
     DXFree((Pointer)xlabptr);
  }
  if (ylabptr) {
     DXFree((Pointer)ylabptr);
  }
  DXFree((Pointer)xptr);
  DXFree((Pointer)yptr);

  return (Object)g;
  
 error:
  if (xlabptr) {
     DXFree((Pointer)xlabptr);
  }
  if (ylabptr) {
     DXFree((Pointer)ylabptr);
  }
  DXFree((Pointer)xptr);
  DXFree((Pointer)yptr);
  DXDelete ((Object)g);
  return ERROR;
}

int _dxfHowManyTics(int islogx, int islogy, float lsx, float lsy, 
                       float scalex, float scaley, int n, int *nx, int *ny)
{
  float ax, ay, sum;

  /* I've put in the scalings for log because of the difference in
   * treatment of log and lin axes */
  if (islogx) 
     ax = ABS(scalex/lsx);
  else
     ax = ABS(scalex);
  if (islogy)
     ay = ABS(scaley/lsy);
  else
     ay = ABS(scaley);

  if (n>0) {
    if (n==1) n = 2;
    sum = ax + ay;
    *nx = ax / sum * n + 0.5;
    *ny = ay / sum * n + 0.5;
    if (*nx<=1) *nx = 2;
    if (*ny<=1) *ny = 2;
  }
  return 1;
}





Error _dxfFreeAxesHandle(Pointer p)
{

  struct axes_struct *axeshandle = (struct axes_struct *)p;

  if (p) {
     DXFree(axeshandle);
  }
  return OK;
}



Pointer _dxfNew2DAxesObject(void)
{
  struct axes_struct *new;

  new = (struct axes_struct *)DXAllocate(sizeof(struct axes_struct));
  /* fill the defaults */
  if (new) {
  new->object=NULL;
  new->xlabel="x";
  new->ylabel="y";
  new->labelscale=1.0;
  new->font="fixed";
  new->t=10;
  new->tx=NULL;
  new->ty=NULL;
  new->corners=NULL;
  new->frame=0;
  new->adjust=0;
  new->grid=0;
  new->islogx=0;
  new->islogy=0;
  new->minticsize=0;
  new->axeslines=1;
  new->justright=0;
  new->actualcorners=NULL;
  new->labelcolor=LABELCOLOR;
  new->ticcolor=TICKCOLOR;
  new->axescolor=AXESCOLOR;
  new->gridcolor=GRIDCOLOR;
  new->backgroundcolor=BACKGROUNDCOLOR;
  new->background=0;
  new->xlocs=NULL;
  new->ylocs=NULL;
  new->xlabels=NULL;
  new->ylabels=NULL;
  new->dofixedfontsize=0;
  new->fixedfontsize=0;
  return (Pointer)new;
  }
  return NULL;
} 

Error _dxfSet2DAxesCharacteristic(Pointer p, char *characteristic,
                                         Pointer value)
{
  struct axes_struct *st = (struct axes_struct *)p;

/* colors */
  if (!strcmp(characteristic,"TICKCOLOR")) {
    st->ticcolor = *(RGBColor *)value;
  }
  else if (!strcmp(characteristic,"LABELCOLOR")) {
    st->labelcolor = *(RGBColor *)value;
  }
  else if (!strcmp(characteristic,"AXESCOLOR")) {
    st->axescolor = *(RGBColor *)value;
  }
  else if (!strcmp(characteristic,"GRIDCOLOR")) {
    st->gridcolor = *(RGBColor *)value;
  }
  else if (!strcmp(characteristic,"BACKGROUNDCOLOR")) {
    st->backgroundcolor = *(RGBColor *)value;
  }

/* objects */
  else if (!strcmp(characteristic,"OBJECT")) {
    st->object = *(Object *)value;
  }
  else if (!strcmp(characteristic,"CORNERS")) {
    st->corners = *(Object *)value;
  }

/* int pointers */
  else if (!strcmp(characteristic,"TICKSX")) {
    st->tx = (int *)value;
  }
  else if (!strcmp(characteristic,"TICKSY")) {
    st->ty = (int *)value;
  }

/* float pointers */
  else if (!strcmp(characteristic,"RETURNCORNERS")) {
    st->actualcorners= (float *)value;
  }

/* float */
  else if (!strcmp(characteristic,"MINTICKSIZE")) {
    st->minticsize= *(float *)value;
  }
  else if (!strcmp(characteristic,"LABELSCALE")) {
    st->labelscale= *(float *)value;
  }
  else if (!strcmp(characteristic,"FIXEDFONTSIZE")) {
    st->fixedfontsize= *(float *)value;
  }

/* string */
  else if (!strcmp(characteristic,"XLABEL")) {
    st->xlabel = (char *)value;
  }
  else if (!strcmp(characteristic,"YLABEL")) {
    st->ylabel = (char *)value;
  }
  else if (!strcmp(characteristic,"FONT")) {
    st->font = (char *)value;
  }

/* integer */
  else if (!strcmp(characteristic,"TICKS")) {
    st->t = *(int *)value;
  }
  else if (!strcmp(characteristic,"FRAME")) {
    st->frame = *(int *)value;
  }
  else if (!strcmp(characteristic,"ADJUST")) {
    st->adjust = *(int *)value;
  }
  else if (!strcmp(characteristic,"GRID")) {
    st->grid = *(int *)value;
  }
  else if (!strcmp(characteristic,"ISLOGX")) {
    st->islogx = *(int *)value;
  }
  else if (!strcmp(characteristic,"ISLOGY")) {
    st->islogy = *(int *)value;
  }
  else if (!strcmp(characteristic,"AXESLINES")) {
    st->axeslines = *(int *)value;
  }
  else if (!strcmp(characteristic,"JUSTRIGHT")) {
    st->justright = *(int *)value;
  }
  else if (!strcmp(characteristic,"BACKGROUND")) {
    st->background = *(int *)value;
  }
  else if (!strcmp(characteristic,"DOFIXEDFONTSIZE")) {
    st->dofixedfontsize = *(int *)value;
  }

  /* Arrays */
  else if (!strcmp(characteristic,"XLOCS")) {
    st->xlocs = (Array)value;
  }
  else if (!strcmp(characteristic,"YLOCS")) {
    st->ylocs = (Array)value;
  }
  else if (!strcmp(characteristic,"XLABELS")) {
    st->xlabels = (Array)value;
  }
  else if (!strcmp(characteristic,"YLABELS")) {
    st->ylabels = (Array)value;
  }
  else {
     DXSetError(ERROR_DATA_INVALID,"unknown option %s",characteristic);
     return ERROR;
  }
  return OK;
}


void
_dxfaxes_delta(float *o, float *s, float scale, int *n, float *l, float *d,
      char *fmt, int *sub, int adjust, int islog)
{
    /* there are some significant differences between the lin and the
     * log case; for log plots, always want labels at all powers of 10 */

    double a, lg, po, lo, hi, sd, so, ss, range;
    int max, min, maxwidth;
    char *cstring;
    float absmin, absmax, biggestnum, smallestnum;
    int i, k, n1, n2, width, precision;
    static struct {                     /* permissible tick mark spacings */
        double delta;                   /* the spacing itself */
        int precision;                  /* our contribution to precision */
        int sub;                        /* spacing of sub-marks */
    } ds[] = {
        { 1.0,   0,  5 },
        { 2.0,   0,  4 },
        { 2.5,   1,  5 },
        { 5.0,   0,  5 },
        { 10.0, -1,  5 }
    };

    if (islog) {
      /* for the log case, we want to munge start, step by the scaling
       * factor. Untransformed, we want them to be simple integers */
      if (*s > 0) {
        /* the last value -- going to next higher integer */
        max = CL( *o + *s);
        /* the first value -- going to next lower integer */
        *o = FL( *o);
        /* the distance between them */
        *s = max - *o;
        absmin = *o;
        absmax = max;
        *d = 1;
        /* number of items */
        *n = ABS(*s) + 1;
        /* now munge them */
        *o = *o/scale;
        *s = *s/scale;
        *d = *d/scale;
      }
      else {
        /* the smallest value, going to next lower */
        min = FL(*o + *s);
        /* the largest value, going to next higher */
        *o = CL (*o);
        /* the distance */
        *s = min - *o;
        absmin = min;
        absmax = *o;
        *d = -1;
        /* number of items */
        *n = ABS(*s) + 1;
        /* now munge them */
        *o = *o/scale;
        *s = *s/scale;
        *d = *d/scale;
      }
      *l = *o;
      biggestnum = MAX(absmin, absmax);
      smallestnum = MIN(absmin, absmax);
      if (biggestnum < 0)
        width = 1;
      else
        width = CL(biggestnum) + 1;
      if (smallestnum < 0) 
        precision = -smallestnum;
      else
        precision = 0;
      width = width + precision + 1;
      /* make the choice between fixed point and exponential */
      cstring = (char *)getenv("DXAXESMAXWIDTH");
      if (cstring != NULL)
          maxwidth = atoi(cstring);
      else 
          maxwidth = MAXWIDTH;
      if (width < maxwidth)
          sprintf(fmt, "%%%d.%df", width, precision);
      else
          sprintf(fmt, "%%%d.%de", 2, precision);
      /* the number of ticks is simply the range + 1 */
      return;
    }
    /* if *s is zero, we generate 0 ticks */
    if (*s==0) {
        *n = 0;
        return;
    }

    /* scaled size and origin as given to us */
    ss = *s * scale;
    so = *o * scale;

    /*
     * Find k and i such that
     *
     *     |s| / delta <= *n, where
     *     delta = 10^k d_i
     *
     * delta is then the absolute value of our delta
     */
    a = ABS(ss) / *n;
    lg = log10(a);
    k = FL(lg);
    po = pow(10.0, (float)k);
    for (i=0; (sd=po*ds[i].delta) * (1+E) < a; i++)
        continue;

    /* tick mark position numbers n1 and n2 */
    if (ss>0) {
        n1 = FL(so/sd);
        n2 = CL((so+ss)/sd);
    } else {
        n1 = CL(so/sd);
        n2 = FL((so+ss)/sd);
    }

    /* adjust axes size to major tick? */
    if (adjust) {
        so = n1*sd;
        ss = n2*sd - so;
    }

    /* number ranges */
    lo = n1 * sd;                       /* lo value (towards o end of range) */
    hi = n2 * sd;                       /* hi value (towards o+s end) */

    /* return new values, possibly adjusted */
    *n = (n2 - n1) * SGN(ss) + 1;       /* number of divisions */
    *sub = ds[i].sub;                   /* sub tick marks */
    *l = lo / scale;                    /* lo value */
    *o = so / scale;                    /* origin */
    *s = ss / scale;                    /* size */
    *d = sd * SGN(ss) / scale;          /* delta */

    /* format */
    precision = -k + ds[i].precision;   /* precision */
    if (precision < 0)
        precision = 0;
    lg = log10(MAX(ABS(lo),ABS(hi)));   /* integer part */
    width = 1 + FL(lg);
    if (width<1)
        width = 1;
    if (precision>0)                    /* fraction and decimal point */
        width += precision + 1;
    if (lo<0 || hi<0)                   /* sign */
        width += 1;
    if (width <= MAXWIDTH) {
        sprintf(fmt, "%%%d.%df", width, precision);
        return;
    }

    /* if the width of the format would be too long to look nice,
     * use exponential notation and try to print the minimum number
     * of significant digits.
     */
    { char tbuf[32], *cp;
      int z, lastz;
      int prec;
      int minprec;
      double val;

      /* exp format is [-]m.dddddde+XX, where the precision controls the
       *  number of digits in the fraction.  start with the default precision
       *  of 6 and look for trailing 0's.  if there are none, you are done.
       *  if some or all are 0's, go through each tick label and take the
       *  largest value which isn't 0.  the minimum return value is 1; this
       *  could be changed to 0 if you prefer "me+XX" instead of "m.0e+XX".
       */

      cstring = (char *)getenv("DXAXESMAXWIDTH");
      if (cstring != NULL)
        maxwidth = atoi(cstring);
      else
        maxwidth = MAXWIDTH; 
      prec = maxwidth;
      minprec = 1;


      range = ABS(hi-lo);
      for (i=0; i< *n; i++) {
          val = lo + i*sd;
          /* check for essentially zero relative to the range */
          if (ABS(val) < range/100000.) val=0.;
          sprintf(tbuf, "%.*e", prec, val);
          lastz = prec + 1 + (tbuf[0]=='-' ? 1 : 0);     /* least significant di
git */
          for (z=prec, cp= &tbuf[lastz]; z>minprec; --z, --cp)
              if (*cp != '0')
                  break;

          if (z == prec) {
              sprintf(fmt, "%%.%de", prec);
              return;
          }
          minprec = MAX(minprec, z);
      }

      sprintf(fmt, "%%.%de", minprec);
  }
}



