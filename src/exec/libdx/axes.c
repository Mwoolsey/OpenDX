/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>

/*
 * Header: /a/max/homes/max/gresh/code/svs/src/libdx/RCS/axes.c,v 5.0 92/11/12
09:07:33 svs Exp Locker: gresh 
 * Locker: gresh 
 */


#include <string.h>
#include <dx/dx.h>
#include <math.h>

static RGBColor TICKCOLOR = {1.0, 1.0, 0.0};
static RGBColor LABELCOLOR = {1.0, 1.0, 1.0};
static RGBColor AXESCOLOR = {1.0, 1.0, 0.0};
static RGBColor GRIDCOLOR = {0.3, 0.3, 0.3};
static RGBColor BACKGROUNDCOLOR = {0.07, .07, 0.07};

#if defined(sgi)
#define log10 log10f
#endif

#define EPSILON 0.01	/* distance away from face for marks */
#define XYZ     0.1	/* cross in middle of cube */
#define XY0     0.05	/* cross in middle of face, toward corner */
#define XY1	0.05    /* cross in middle of face, toward edge */
#define X0      0.05	/* tick mark in corner of cube */
#define X1      0.08	/* tick mark at edge of cube */
#define MAJOR   0.05	/* default major tick proportion of axis length */
#define MAJORMIN  6	/* minimum major tick length in pixels */
#define MINOR   0.4	/* minor tick length is this times major tick length */
#define FS      0.8	/* font scaled by FS times minimum delta */
#define FMAX	20	/* maximun font scale */
#define OFFSET  0.7	/* tick labels are offset by OFFSET times font ht */
#define MAXWIDTH  7     /* numbers wider than this are printed in exp notation */
                        /* can be overridden by DXAXESMAXWIDTH env var */
#define LABELSCALE  1.2 /* axes labels can be bigger than tic labels */

    
#define ABS(x) ((x)<0? -(x) : (x))
#define SGN(x) ((x)<0? -1 : 1)

#define E 0.0001						/* fuzz */
#define FL(x) ((x)+E>0? (int)((x)+E) : (int)((x)+E)-1)	/* fuzzy floor */
#define CL(x) ((x)-E>0? (int)((x)-E)+1 : (int)((x)-E))	/* fuzzy ceiling */

Error _dxfGetFormat(float *, char *, int, float);
Error _dxfCheckLocationsArray(Array, int *, float **);

/* From helper.c. Should probably be in .h file */
extern Field _dxfBeginFace(Field); 
extern Field _dxfNextPoint(Field, PointId);
extern Field _dxfEndFace(Field);

static Error GetProjections(Vector cameraup, Vector eye, 
                            float *flipx, float *flipy, float *flipz,
                            float *flipxlab, float *flipylab, float *flipzlab,
                            Vector sgn)
{
  Vector cameraupprime;
  Vector xprime, yprime, zprime, xfup, yfup, zfup;
  
  /* I want upprime and standardprime to be the projections of
     up and [0 1 0] in the plane perpendicular to the eye vector */
  
  /* first derive up and right. They must simply be mutually
     perpendicular with eye */

/*
{
	Vector up, right;
	if ((eye.x==0)&&(eye.y==0)&&(eye.z==1)) {
		up = DXVec(0, 1, 0);
		right = DXVec(1, 0, 0);
	}
	else if ((eye.x==0)&&(eye.y==0)&&(eye.z==-1)) {
		up = DXVec(0, 1, 0);
		right = DXVec(1, 0, 0);
	}
	else {
		up = DXVec(0,0,1);
		up = DXCross(up,eye);
		right = DXCross(up,eye);
	}
}
 */

 
  /* now do the projection */
  cameraupprime = DXSub(cameraup, DXMul(eye, DXDot(cameraup, eye)));
  
  /* do the projections of the axes now */
  xprime = DXSub(DXVec(1,0,0), DXMul(eye, DXDot(DXVec(1,0,0), eye)));
  yprime = DXSub(DXVec(0,1,0), DXMul(eye, DXDot(DXVec(0,1,0), eye)));
  zprime = DXSub(DXVec(0,0,1), DXMul(eye, DXDot(DXVec(0,0,1), eye)));
  
  /* now get the "font up" directions, which are the cross products
     of x,y,zprime and eye */
  
  xfup = DXCross(xprime, eye);
  yfup = DXCross(yprime, eye);
  zfup = DXCross(zprime, eye);
 
  if (DXDot(xprime, cameraupprime) != 0)
     *flipx = SGN(DXDot(xprime, cameraupprime));
  else
     *flipx = -1;
  *flipy = SGN(DXDot(yprime, cameraupprime));
  *flipz = SGN(DXDot(zprime, cameraupprime));
  
  /* need to do some fudging for the sgn param. This was trial
     and error, I admit */
  
  if ((sgn.x < 0 && sgn.y < 0 && sgn.z < 0)||
      (sgn.x > 0 && sgn.y > 0 && sgn.z > 0)) {
    *flipxlab = -SGN(DXDot(xfup, cameraupprime));
    if (DXDot(yfup, cameraupprime) != 0)
      *flipylab = -SGN(DXDot(yfup, cameraupprime));
    else
      *flipylab = -1;
    *flipzlab = -SGN(DXDot(zfup, cameraupprime));
  }
  /* else exactly one is negative. Flip the two that aren't negative */
  else if (sgn.x * sgn.y * sgn.z < 0) {
    *flipxlab = -SGN(DXDot(xfup, cameraupprime))* (-sgn.x);
    if (DXDot(yfup, cameraupprime) != 0)
      *flipylab = -SGN(DXDot(yfup, cameraupprime))* (-sgn.y);
    else
      *flipylab = -1;
    *flipzlab = -SGN(DXDot(zfup, cameraupprime))* (-sgn.z);
  }
  /* else two are negative. Flip those two. */
  else {
    *flipxlab = -SGN(DXDot(xfup, cameraupprime))* (sgn.x);
    if (DXDot(yfup, cameraupprime) != 0)
       *flipylab = -SGN(DXDot(yfup, cameraupprime))* (sgn.y);
    else
      *flipylab = -1;
    *flipzlab = -SGN(DXDot(zfup, cameraupprime))* (sgn.z);
  }
  return OK;   
}

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



/*
 * Do a face of the box.
 */

static 
  void
  face(
       Field f,		/* field to hold the faces */
       int *pt,		/* point numbers */
       Point o,		/* origin */
       Vector sx,  	/* "x" vector */
       Vector sy,  	/* "y" vector */
       RGBColor c		/* color of face */
       ) {
    Vector normal;
    int i;
    
    /* face corner points */
    DXAddPoint(f, *pt+0, o);
    DXAddPoint(f, *pt+1, DXAdd(o,sx));
    DXAddPoint(f, *pt+2, DXAdd(sy,DXAdd(o,sx)));
    DXAddPoint(f, *pt+3, DXAdd(o,sy));
    
    /* face colors, normals */
    normal = DXNormalize(DXCross(sx,sy));
    for (i=0; i<4; i++) {
      DXAddColor(f, *pt+i, c);
      DXAddNormal(f, *pt+i, normal);
    }
    
    /* face */
    _dxfBeginFace(f);
    for (i=0; i<4; i++)
      _dxfNextPoint(f, *pt+i);
    _dxfEndFace(f);
    
    *pt += 4;
}


/*
 * Macro to do a line segment, adding points,
 * of the given color.
 */

#define LINE(rgb, x0,y0,z0, x1,y1,z1) {	\
    Line line;				\
    line.p = pt;			\
    line.q = pt+1;			\
    DXAddLine(f, ln++, line);		\
    DXAddPoint(f, line.p, DXPt(x0,y0,z0));	\
    DXAddPoint(f, line.q, DXPt(x1,y1,z1));	\
    DXAddColor(f, line.p, rgb); 		\
    DXAddColor(f, line.q, rgb);		\
    pt += 2;				\
}   

#define LINE1(rgb, x0,y0,z0, x1,y1,z1) {	\
    Line line;				\
    line.p = pt1;			\
    line.q = pt1+1;			\
    DXAddLine(f1, ln1++, line);		\
    DXAddPoint(f1, line.p, DXPt(x0,y0,z0));	\
    DXAddPoint(f1, line.q, DXPt(x1,y1,z1));	\
    DXAddColor(f1, line.p, rgb); 		\
    DXAddColor(f1, line.q, rgb);		\
    pt1 += 2;				\
}   

/*
 * Make tick marks, cursor, labels for a plane
 */

#define BETWEEN(a, x, b) (((a)<(x) && (x)<(b)) || ((b)<(x) && (x)<(a)))

static 
Error
mark(
     Group g,			/* group to hold result */
     int show_front,		/* whether to show front outlines */
     int show_axes,		/* whether to show axes lines */
     char *xlabel,		/* x axis label */
     char *ylabel,		/* y axis label */
     int fliplx,		/* flip the x label */
     int fliply,		/* flip the y label */
     int reverse,		/* whteher to reverse labels */
     Point o,			/* origin */
     Vector s,			/* size */
     Vector sgn,		/* sign of size */
     Point *c,			/* cursor position */
     int nx,			/* number of x tick marks */
     int flipxtics,
     int subx,			/* number of x sub tick marks */
     double lx,			/* first tick mark on x axis */
     double dx,			/* spacing of x tick marks */
     double lsx,		/* label scaling for x */
     int flipx,			/* whether to flip x tick labels */
     char *fmtx,		/* format for x tick labels */
     int ny,			/* number of y tick marks */
     int flipytics,
     int suby,			/* number of y sub tick marks */
     double ly,			/* first tick mark on y axis */
     double dy,			/* spacing of y tick marks */
     double lsy,		/* label scaling for y */
     int flipy,			/* whether to flip y tick labels */
     char *fmty,		/* format for x tick labels */
     double fs,			/* font scaling for labels */
     int px, int py, int pz,	/* permutation */
     int grid,                  /* whether to draw the grid */
     double lengthx,		/* axis length for tick mark calculation */
     double lengthy,            /* axis length for tick mark calculation */
     RGBColor labelcolor,       /* color of labels */
     RGBColor ticcolor,        /* color of ticks */
     RGBColor axescolor,        /* color of axes */
     RGBColor gridcolor,        /* color of grid */
     float labelscale,        /* scalefactor for labels */
     char *fontname,            /* font to use for labels */
     float *xlocs, 
     float *ylocs,             /* list of locations for tics */
     char **xlabels,
     char **ylabels            /* list of labels for tics */
) {
    static Matrix zero;		/* zero matrix */
    Matrix t;			/* matrix */
    float z;			/* z distance above face for marks */
    Object font=NULL;		/* font for labels */
    static float ascent;	/* baseline to top of label */
    float xwidth;		/* max width of x axis tick labels */
    float ywidth;		/* max width of y axis tick labels */
    float width;		/* width temp variable */
    Group group = NULL;		/* group to hold labels */
    Object text;		/* the label */
    char buf[100];		/* the label */
    float offset;		/* amount to move label so 0's don't collide */
    float hdir, vdir;		/* direction of tick label compared to axis */
    Field f = NULL;		/* field for marks */
    Field f1 = NULL;		/* field for tics */
    Xform xform = NULL;		/* various transformed objects */
    int pt=0, ln=0;		/* current point, line number within f */
    int pt1=0, ln1=0;		/* current point, line number within f */
    float x, y, a, b, major, minor, xx, yy;
    int i, j;
    char *tempstring;
    int createdxlocs = 0, createdylocs = 0;
    int createdxlabels = 0, createdylabels = 0;
    int axesdirection;

    z = o.z /*+EPSILON*s.z*/;	/* z distance above face for marks */
    
    group = DXNewGroup();
    if (!group) goto error;
    
    /* a field for the ticks. They have a larger fuzz than the grid lines */
    f1 = DXNewField();
    if (!f1) goto error;
    DXSetFloatAttribute((Object)f1, "fuzz", 2);
    DXSetMember(group, NULL, (Object) f1);
    
    /* a field for everything else */
    f = DXNewField();
    if (!f) goto error;
    DXSetFloatAttribute((Object)f, "fuzz", 1);
    DXSetMember(group, NULL, (Object) f);
    
    

    /*
     * Put cursor projections on xy plane as follows:
     *
     *             B
     *     y ------+--------  o+s
     *      |               |
     *    C +      +        + D
     *      |     E         |
     *      |               |
     *    o  ------+-------- x
     *             A
     */    

    if (c) {

	/* cursor projections */
	LINE(axescolor, 
             c->x, o.y, z,           c->x, o.y+s.y*X0,     z);    /* A */
	LINE(axescolor,  
             c->x, o.y+s.y, z,       c->x, o.y+s.y*(1-X1), z);    /* B */
	LINE(axescolor,  
             o.x, c->y, z,           o.x+s.x*X0, c->y,     z);    /* C */
	LINE(axescolor,  
             o.x+s.x, c->y, z,       o.x+s.x*(1-X1), c->y, z);    /* D */
	LINE(axescolor,  
             c->x+s.x*XY1, c->y, z,  c->x-s.x*XY0, c->y,   z);    /* E */
	LINE(axescolor,  
             c->x, c->y+s.y*XY1, z,  c->x, c->y-s.y*XY0,   z);    /* E */

	/* our contribution to cursor in middle of box */
	if (s.z!=0)
	    LINE(axescolor,  
                 c->x+s.x*XYZ, c->y, c->z,    c->x-s.x*XYZ, c->y, c->z);
    }

    /* our contribution to front face outlines */
    
    /* for a three-d plot */
    if (s.z != 0){
      if (show_front)
	LINE(axescolor,  o.x, o.y+s.y, o.z+s.z,   o.x+s.x, o.y+s.y, o.z+s.z);
    }
    /* for a two-d plot */
    else {
      /* reset showaxes to 1; it looks funny otherwise */
      show_axes = 1;
      if (show_front) {
	LINE(axescolor,  o.x, o.y+s.y, o.z+s.z,   o.x, o.y, o.z+s.z);
	LINE(axescolor,  o.x+s.x, o.y, o.z+s.z,   o.x, o.y, o.z+s.z);
      }
    }

    /* x-axis tick mark length, based on y-axis size */
    major = MAJOR;
    lengthy = ABS(lengthy);
    if (lengthy>0 && major*lengthy<MAJORMIN)
      major = MAJORMIN/lengthy;
    minor = major * MINOR;
    

    /* create list of tic mark locations */
    /* will need to set o and s for given tic list */
    a = o.x - .001*s.x;
    b = o.x + 1.001*s.x;
    if (!xlocs) {
      createdxlocs = 1;
      xlocs = (float *)DXAllocate(nx*sizeof(float));
      for (i=0; i<nx; i++) 
        xlocs[i] = lx + i*dx;
    }
    if (!xlabels) {
      createdxlabels = 1;
      xlabels = (char **)DXAllocate(nx*sizeof(char *));
      for (i = 0; i<nx; i++) {
	tempstring = (char *)DXAllocate(100*sizeof(char));
        sprintf(tempstring, fmtx, xlocs[i]*lsx);
	xlabels[i] = tempstring;
      } 
    }
    /* tick marks on x axis */
    for (i=0; i<nx; i++) {
      /* major ticks are evenly spaced for log plot */
      x = xlocs[i];
      if (BETWEEN(a, x, b)) {
        if (flipxtics) {
	  LINE1(ticcolor,  x, o.y+s.y, z,  x, o.y+(1+major)*s.y, z);
        }
        else {
	  LINE1(ticcolor,  x, o.y+s.y, z,  x, o.y+(1-major)*s.y, z);
        }
	/* for the first and last, only draw the grid if we're NOT
	 * drawing the axes (otherwise they're coincident */
	if (grid) {
	  if ((i==nx-1)||(i==0)) {
	    if (!show_axes) LINE(gridcolor,  x, o.y, z,  x, o.y+s.y, z);
	  }
	  else
	    LINE(gridcolor,  x, o.y, z,  x, o.y+s.y, z);
	}
      }
      /* minor ticks are not evenly spaced */
      if (i < nx-1)
	for (j=0; j<subx; j++) {
	  xx = x + j*dx/subx;
	  if (BETWEEN(a, xx, b)) {
            if (flipxtics) {
	      LINE1(ticcolor,  xx, o.y+s.y, z,  xx, o.y+(1+minor)*s.y, z);
            }
            else {
	      LINE1(ticcolor,  xx, o.y+s.y, z,  xx, o.y+(1-minor)*s.y, z);
            }
          }
	}
    }
    
    /* y-axis tick mark length, based on x-axis size */
    major = MAJOR;
    lengthx = ABS(lengthx);
    if (lengthx>0 && major*lengthx<MAJORMIN)
      major = MAJORMIN/lengthx;
    minor = major * MINOR;

    a = o.y - .001*s.y;
    b = o.y + 1.001*s.y;
    if (!ylocs) {
      createdylocs = 1;
      ylocs = (float *)DXAllocate(ny*sizeof(float));
      for (i=0; i<ny; i++) 
        ylocs[i] = ly + i*dy;
    }
    if (!ylabels) {
      createdylabels = 1;
      ylabels = (char **)DXAllocate(ny*sizeof(char *));
      for (i = 0; i<ny; i++) {
	tempstring = (char *)DXAllocate(100*sizeof(char));
        sprintf(tempstring, fmty, ylocs[i]*lsy);
	ylabels[i] = tempstring;
      } 
    }

    /* tick marks on y axis */
    for (i=0; i<ny; i++) {
      y = ylocs[i];
      if (BETWEEN(a, y, b)) {  
        if (flipytics) {
	   LINE1(ticcolor,  o.x+s.x, y, z,  o.x+(1+major)*s.x, y, z);
        }
        else {
	   LINE1(ticcolor,  o.x+s.x, y, z,  o.x+(1-major)*s.x, y, z);
        }
	if (grid) {
	  if ((i==ny-1)||(i==0)) {
	    if (!show_axes) LINE(gridcolor,  o.x, y, z,  o.x+s.x, y, z);
	  }
	  else
	    LINE(gridcolor,  o.x, y, z,  o.x+s.x, y, z);
	}
      }
      if (i < ny-1)
	for (j=0; j<suby; j++) {
	  yy = y + j*dy/suby;
	  if (BETWEEN(a, yy, b)) {
            if (flipytics) {
	       LINE1(ticcolor,  o.x+s.x, yy, z,  o.x+(1+minor)*s.x, yy, z);
            }
            else {
	       LINE1(ticcolor,  o.x+s.x, yy, z,  o.x+(1-minor)*s.x, yy, z);
            }
          }
	}
    }

    
    if (show_axes) {
      LINE(axescolor,  o.x, o.y+s.y, z,   o.x+s.x, o.y+s.y, z);
      LINE(axescolor,  o.x+s.x, o.y, z,   o.x+s.x, o.y+s.y, z);
    }

    /* use fixed-width font here */
    if (!strcmp(fontname,"standard"))
      font = DXGetFont("fixed", &ascent, NULL);
    else
      font = DXGetFont(fontname, &ascent, NULL);
   
    if (!font)
      goto error;
   


    /* offset is half of a font height */
    axesdirection = SGN(xlocs[nx-1]-xlocs[0]);
    offset =  .5*ascent*fs*labelscale;
    
    
    /* x axis tick labels */
    a = o.x - .001*s.x;
    b = o.x + 1.001*s.x;

    for (i=0, xwidth=0; i<nx; i++) {
      if (!BETWEEN(a, xlocs[i], b))
	continue;
      sprintf(buf, "%s", xlabels[i]);
      text = DXGeometricText(buf, font, &width);
      _dxfSetTextColor((Field)text,labelcolor);

      /* hdir and vdir determine whether the labels will be mirrored */ 
      hdir = sgn.z * flipx;
      vdir = flipx;

 
      /* here we inset the first and last labels away from the corners */ 
      if (i==0)
         x = (axesdirection==1) ? xlocs[i] : xlocs[i]-2*offset;
      else if (i==nx-1)
         x = (axesdirection==1) ? xlocs[i]-2*offset : xlocs[i];
      /* these end the last label at the tick mark */
      /* and start the 1st label at the tick mark */
      else
         x = xlocs[i]-offset;
      /* this centers the label on the tick mark */

      /* to allow for the effect of the mirror scale */
      if (vdir > 0)
        x = x+2*offset;  

      if (!flipxtics)
        y = o.y + s.y + sgn.y*fs + 
 	    (sgn.y*hdir>0? 0 : sgn.y*fs*labelscale*width);
      else
        y = o.y + s.y + sgn.y*fs + major*s.y + 
 	    (sgn.y*hdir>0? 0 : sgn.y*fs*labelscale*width);


      t = DXScale(hdir*fs*labelscale, vdir*fs*labelscale, 1);
      xform = DXNewXform((Object)text, t);
      xform = DXNewXform((Object)xform, DXRotateZ(90*DEG));
      xform = DXNewXform((Object)xform, DXTranslate(DXVec(x,y,z)));
      if (!xform) goto error;
      DXSetMember(group, NULL, (Object)xform);
      if (width>xwidth)
	xwidth = width;
    }
    
    /* y axis tick labels */
    a = o.y - .001*s.y;
    b = o.y + 1.001*s.y;
    axesdirection = SGN(ylocs[ny-1]-ylocs[0]);
    offset =  .5*ascent*fs*labelscale;

    for (i=0, ywidth=0; i<ny; i++) {
      if (!BETWEEN(a, ylocs[i], b))
	continue;
      sprintf(buf,"%s", ylabels[i]);
      text = DXGeometricText(buf, font, &width);
      _dxfSetTextColor((Field)text,labelcolor);
      hdir = sgn.z * flipy;
      vdir = flipy;
      if (!flipytics) 
        x = o.x + s.x + sgn.x*fs + 
	   (sgn.x*hdir>0? 0 : sgn.x*fs*width*labelscale);
      else
        x = o.x + s.x + sgn.x*fs + major*s.x +
	   (sgn.x*hdir>0? 0 : sgn.x*fs*width*labelscale);

      if (i==0)
         y = (axesdirection==1) ? ylocs[0] : ylocs[0]-2*offset;
      else if (i==ny-1)
         y = (axesdirection==1) ? ylocs[i]-2*offset : ylocs[i];
      /* these end the last label at the tick mark */
      /* and starts the 1st label at the tick mark */
      else
         y = ylocs[i]-offset;  /* this centers the label on the tick mark */

      /* to allow for the effect of the mirror scale */
      if (vdir < 0)
        y = y+2*offset;  

      t = DXScale(hdir*fs*labelscale, vdir*fs*labelscale, 1);
      xform = DXNewXform((Object)text, t);
      xform = DXNewXform((Object)xform, DXTranslate(DXVec(x,y,z)));
      if (!xform) goto error;
      DXSetMember(group, NULL, (Object)xform);
      if (width>ywidth)
	ywidth = width;
    }
    
    /* use proportional font here */
    DXDelete(font);
    
    if (!strcmp(fontname,"standard")) 
      font = DXGetFont("variable", &ascent, NULL);
    else
      font = DXGetFont(fontname, &ascent, NULL);
    
    if (!font)
      goto error;
    
    /* x label */
    if (xlabel) {
      if (strcmp(xlabel,"")) {
	text = DXGeometricText(xlabel, font, &width);
        _dxfSetTextColor((Field)text,labelcolor);

	x = o.x + s.x/2 - 
	  LABELSCALE*fliplx*sgn.x*fs*labelscale*width/2 *reverse;

        if (!flipxtics) 
           y = o.y + s.y + sgn.y*fs*(1 + labelscale*(xwidth*1.2) +
				  ascent*labelscale)
	    + (fliplx*sgn.y < 0 ? fs*ascent*labelscale : 0); 
        else
           y = o.y + s.y + sgn.y*fs*(1 + labelscale*(xwidth*1.2) +
				  ascent*labelscale) +
              major*s.y + 
	    + (fliplx*sgn.y < 0 ? fs*ascent*labelscale : 0); 
	
	t = DXScale(fliplx*sgn.x*fs*reverse*LABELSCALE*labelscale, 
                    fliplx*sgn.y*fs*LABELSCALE*labelscale, 0); 
	xform = DXNewXform((Object)text, t);
	xform = DXNewXform((Object)xform, DXTranslate(DXVec(x,y,z)));
	if (!xform) goto error;
	DXSetMember(group, NULL, (Object)xform);
      }
    }

    /* y label */
    if (ylabel) {
      if (strcmp(ylabel,"")) {
	text = DXGeometricText(ylabel, font, &width);
        _dxfSetTextColor((Field)text,labelcolor);
        if (!flipytics)
           x = o.x + s.x + sgn.x*fs*(1 + labelscale*(ywidth*1.2) +
				  ascent*labelscale)
	     + (fliply*sgn.x < 0 ? fs*ascent*labelscale : 0); 
        else
           x = o.x + s.x + sgn.x*fs*(1 + labelscale*(ywidth*1.2) +
				  ascent*labelscale) +
              major*s.x + 
	     + (fliply*sgn.x < 0 ? fs*ascent*labelscale : 0); 
	
	y = o.y  + s.y/2 + 
	  LABELSCALE*fliply*sgn.y*fs*width*labelscale/2 *reverse;
	t = DXScale(fliply*sgn.y*fs *reverse*LABELSCALE*labelscale, 
                    fliply*sgn.x*fs*LABELSCALE*labelscale, 0);
	xform = DXNewXform((Object)text, t);
	xform = DXNewXform((Object)xform, DXRotateZ(-90*DEG));
	xform = DXNewXform((Object)xform, DXTranslate(DXVec(x,y,z)));
	if (!xform) goto error;
	DXSetMember(group, NULL, (Object)xform);
      }
    }

    /* end field */
    if (!DXEndField(f)) goto error;
    if (!DXEndField(f1)) goto error;
    
    /* permute and put in group */
    t = zero;
    t.A[px][0] = 1;
    t.A[py][1] = 1;
    t.A[pz][2] = 1;
    xform = DXNewXform((Object)group, t);
    if (!xform) goto error;
    DXSetMember(g, NULL, (Object)xform);
    DXSetFloatAttribute((Object)group, "fuzz", 4);
    
    /* all ok */
    DXDelete(font);
    if (createdxlocs) {
          DXFree((Pointer)xlocs);
    }
    if (createdxlabels) {
       for (i=0;i<nx;i++) {
          DXFree((Pointer)xlabels[i]);
       }
       DXFree((Pointer)xlabels);
    }
    if (createdylocs) {
          DXFree((Pointer)ylocs);
    }
    if (createdylabels) {
       for (i=0;i<ny;i++) {
          DXFree((Pointer)ylabels[i]);
       }
       DXFree((Pointer)ylabels);
    }
    return OK;

  error:
    DXDelete(font);
    DXDelete((Object)group);
    if (createdxlocs) {
          DXFree((Pointer)xlocs);
    }
    if (createdxlabels) {
       for (i=0;i<nx;i++) {
          DXFree((Pointer)xlabels[i]);
       }
       DXFree((Pointer)xlabels);
    }
    if (createdylocs) {
          DXFree((Pointer)ylocs);
    }
    if (createdylabels) {
       for (i=0;i<ny;i++) {
          DXFree((Pointer)ylabels[i]);
       }
       DXFree((Pointer)ylabels);
    }
    return ERROR;
  }


static
void _dxfaaxes_delta(float *o, float *s, float scale, int *n, float *l, float *d,
		 char *fmt, int *sub, int adjust)
{
    double a, lg, po, lo, hi, sd, so, ss, range;
    int i, k, n1, n2, width, precision, maxwidth;
    char *cstring;
    static struct {			/* permissible tick mark spacings */
      double delta;			/* the spacing itself */
      int precision;			/* our contribution to precision */
      int sub;			/* spacing of sub-marks */
    } ds[] = {
      { 1.0,   0,  5 },
      { 2.0,   0,  4 },
      { 2.5,   1,  5 },
      { 5.0,   0,  5 },
      { 10.0, -1,  5 }
      };
    
    
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
    lo = n1 * sd;			/* lo value (towards o end of range) */
    hi = n2 * sd;			/* hi value (towards o+s end) */
    
    /* return new values, possibly adjusted */
    *n = (n2 - n1) * SGN(ss) + 1;	/* number of divisions */
    *sub = ds[i].sub;			/* sub tick marks */
    *l = lo / scale;			/* lo value */
    *o = so / scale;			/* origin */
    *s = ss / scale;			/* size */
    *d = sd * SGN(ss) / scale;		/* delta */

    /* format */
    precision = -k + ds[i].precision;	/* precision */
    if (precision < 0)
      precision = 0;
    lg = log10(MAX(ABS(lo),ABS(hi)));	/* integer part */
    width = 1 + FL(lg);
    if (width<1)
      width = 1;
    if (precision>0)			/* fraction and decimal point */
      width += precision + 1;
    if (lo<0 || hi<0)			/* sign */
      width += 1;


    cstring = (char *)getenv("DXAXESMAXWIDTH");
    if (cstring != NULL)
       maxwidth = atoi(cstring);
    else
       maxwidth = MAXWIDTH;
    if (width <= maxwidth) {
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

      prec = 6;
      minprec = 1;

      	 
      range = ABS(hi-lo); 
      for (i=0; i< *n; i++) {
	val = lo + i*sd;
	/* check for essentially zero relative to the range */
	if (ABS(val) < range/100000.) val=0.;
	sprintf(tbuf, "%.*e", prec, val);
	lastz = prec + 1 + (tbuf[0]=='-' ? 1 : 0);     /* least significant digit */
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


static Object
  _Axes(
	Point o,		/* origin */
	Vector s,		/* size */
	Vector sgn,		/* sign of s */
	Vector ls,		/* label scale */
	Vector length,	/* on-screen lengths of each axis */
	Point *c,		/* cursor */
	RGBColor cxy,	/* color of xy plane */
	RGBColor cyz,	/* color of yz plane */
	RGBColor czx,	/* color of zx plane */
	char *xlabel,	/* x axis label */
	char *ylabel,	/* y axis label */
	char *zlabel,	/* z axis label */
	int frame,		/* whether to show the front faces as outline */
	int n,		/* number of tick labels */
	int nx,		/* number for x axis, if n==0 */
	int ny,		/* number for y axis, if n==0 */
	int nz,		/* number for z axis, if n==0 */
	int adjust,  	/* whether to move endpoints of axes to */
	int grid,           /* whether to draw the grid */
	RGBColor labelcolor, /* color of labels */
	RGBColor ticcolor,  /* color of tics */
	RGBColor axescolor,  /* color of axes */
	RGBColor gridcolor,  /* color of grid */
	float labelscale,    /* scalefactor for labels */
	char *fontname,      /* font to use for labels */
	Vector up,            /* camera up vector */
	Vector eye,            /* camera from-to vector */
	Array xlocs, 
	Array ylocs, 
	Array zlocs,
	Array xlabels, 
	Array ylabels,
	Array zlabels 
	) {
    Field f = NULL;
    Group g = NULL;
    int pt=0, subx=0, suby=0, subz=0;
    float dx, dy, dz, lx=0, ly=0, lz=0;
    float fs, sum, flipx, flipy, flipz, ax, ay, az, p;
    float flipxlab, flipylab, flipzlab;
    int reverse, i;
    int background, frontframe, showaxes;
    char fmtx[20], fmty[20], fmtz[20];
    Error rc;
    float *xptr=NULL, *yptr=NULL, *zptr=NULL;
    char **xlabptr=NULL, **ylabptr=NULL, **zlabptr=NULL, *tmpstring;
    Type type;
    int numlabels, flipxtics, flipytics, flipztics;

    /* number of tick labels on each axis */
    ax = ABS(s.x);
    ay = ABS(s.y);
    az = ABS(s.z);
    flipxtics = 0;
    flipytics = 0;
    flipztics = 0;
    if (n>0) {
      if (n==1) n = 2;
      sum = ax + ay + az;
      nx = ax / sum * n + 0.5;
      ny = ay / sum * n + 0.5;
      nz = az / sum * n + 0.5;
      if (nx < 2) nx=2;
      if (ny < 2) ny=2;
      if (nz < 2) nz=2;
    }
    if (n<0) {
      flipxtics = 1;
      flipytics = 1;
      flipztics = 1;
      n = -n;
      if (n==1) n = 2;
      sum = ax + ay + az;
      nx = ax / sum * n + 0.5;
      ny = ay / sum * n + 0.5;
      nz = az / sum * n + 0.5;
    }
    if (nx < 0) {
      flipxtics = 1;
      nx = -nx;
    }
    if (ny < 0) {
      flipytics = 1;
      ny = -ny;
    }
    if (nz < 0) {
      flipztics = 1;
      nz = -nz;
    }
    if (nx < 2) nx=2;
    if (ny < 2) ny=2;
    if (nz < 2) nz=2;

    
    
    /* First deal with xlocs, ylocs, zlocs, and xlabels, ylabels, zlabels */
    /* Redo origin and s if xlocs etc are given */
    
    if (xlocs) {
      if (!_dxfCheckLocationsArray(xlocs, &nx, &xptr))
	goto error;
      /*
      o.x = xptr[0];
      s.x = xptr[nx-1]-o.x; */
      subx = 0;
      if (xlabels) {
        
         if (!DXGetArrayInfo((Array)xlabels, &numlabels, 
                              &type,NULL, NULL,NULL))
           return ERROR;
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
      /*
      o.y = yptr[0];
      s.y = yptr[ny-1]-o.y; */
      suby = 0;
      if (ylabels) {
         if (!DXGetArrayInfo((Array)ylabels, &numlabels, 
                              &type,NULL, NULL,NULL))
           return ERROR;
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
    if (zlocs) {
      if (!_dxfCheckLocationsArray(zlocs, &nz, &zptr))
	goto error;
      /*
      o.z = zptr[0];
      s.z = zptr[nz-1]-o.z; */
      subz = 0;
      if (zlabels) {
         if (!DXGetArrayInfo((Array)zlabels, &numlabels, 
                              &type,NULL, NULL,NULL))
           return ERROR;
         if (numlabels != nz) {
           DXSetError(ERROR_DATA_INVALID,
                      "number of zlabels must match number of zlocations");
           goto error;
         }
         if (type != TYPE_STRING) {
           DXSetError(ERROR_DATA_INVALID,"zlabels must be a string list");
           goto error;
         }
         zlabptr = (char **)DXAllocate(nz*sizeof(char *)); 
         for (i = 0; i< nz; i++) {
             if (!DXExtractNthString((Object)zlabels, i, &tmpstring)) {
                DXSetError(ERROR_DATA_INVALID,"invalid zlabels");
                goto error;
             }
             zlabptr[i] = tmpstring; 
          }
      }
    }

    /* only draw the axes lines if the background is not going to be drawn */
    /* frame = 2 or 3 means that the background is not drawn */
    if ((frame == 0) || (frame == 1)) {
      background = 1;
      showaxes = 0;
    }
    else {
      background = 0;
      showaxes = 1;
    }
    if ((frame == 1) || (frame == 3))
      frontframe = 1;
    else
      frontframe = 0;
    
    g = DXNewGroup();
    if (!g) goto error;
    
    /* deltas - may change o and s, so do this before faces */
    if (nx>0) {
      if (!xlocs)
	_dxfaaxes_delta(&o.x, &s.x, ls.x, &nx, &lx, &dx, fmtx, &subx, 
		       adjust);
      else { 
	lx = xptr[0];
	dx = s.x/(nx-1);
        if (!xlabels) _dxfGetFormat(xptr, fmtx, nx, ls.x);
      }
    }
    if (ny>0) {
      if (!ylocs) 
        _dxfaaxes_delta(&o.y, &s.y, ls.y, &ny, &ly, &dy, fmty, &suby, 
   		       adjust);
      else {
	ly = yptr[0];
	dy = s.y/(ny-1);
        if (!ylabels) _dxfGetFormat(yptr, fmty, ny, ls.y);
      }
    }
    if (nz>0) {
      if (!zlocs)
        _dxfaaxes_delta(&o.z, &s.z, ls.z, &nz, &lz, &dz, fmtz, &subz, 
		       adjust);
      else {
	lz = zptr[0];
	dz = s.z/(nz-1);
        if (!zlabels) _dxfGetFormat(zptr, fmtz, nz, ls.z);
      } 
    }
    
    
    /* font scaling is related to minimum delta, and limited by max length */
    if (nx==0) dx = 1e20; /* XXX */
    if (ny==0) dy = 1e20; /* XXX */
    if (nz==0) dz = 1e20; /* XXX */
    fs = FS * MIN(ABS(dx),MIN(ABS(dy),ABS(dz)));

    /* normalize up vector */
    up = DXNormalize(up);
    eye = DXNormalize(eye);
    
    /* find the quadrant with respect to up */
    GetProjections(up, eye, &flipx, &flipy, &flipz, 
                   &flipxlab, &flipylab, &flipzlab, sgn);
    
    reverse = sgn.x * sgn.y * sgn.z;

    /* futz for scaling */
    if (xlocs) {
      for (i=0; i<nx; i++) 
         xptr[i] = xptr[i]/ls.x;
    }
    if (ylocs) {
      for (i=0; i<ny; i++) 
         yptr[i] = yptr[i]/ls.y;
    }
    if (zlocs) {
      for (i=0; i<nz; i++) 
         zptr[i] = zptr[i]/ls.z;
    }
    if (s.x!=0) {
      p = ABS(length.x/s.x);
      if (fs * p > FMAX)
	fs = FMAX / p;
    }
    if (s.y!=0) {
      p = ABS(length.y/s.y);
      if (fs * p > FMAX)
	fs = FMAX / p;
    }
    if (s.z!=0) {
      p = ABS(length.z/s.z);
      if (fs * p > FMAX)
	fs = FMAX / p;
    }

    /* only draw faces for frame = 0 or 1 */
    if (background) {
      /* back faces */
      f = DXNewField();
      if (!f) goto error;
      DXSetMember(g, NULL, (Object) f);
      DXSetFloatAttribute((Object)f, "fuzz", -4);
      DXSetFloatAttribute((Object)f, "specular", 0);
      if (ax>0 && ay>0)
	face(f, &pt, o, DXVec(s.x,0,0), DXVec(0,s.y,0), cxy);
      if (ay>0 && az>0)
	face(f, &pt, o, DXVec(0,s.y,0), DXVec(0,0,s.z), cyz);
      if (az>0 && ax>0)
	face(f, &pt, o, DXVec(0,0,s.z), DXVec(s.x,0,0), czx);
      if (!DXEndField(f)) goto error;
    }
    
    
    if (ax>0 && ay>0) {
      rc = mark(g, frontframe, showaxes, xlabel, ylabel, flipxlab, -flipylab, 
		reverse, o, s, sgn, c,
		nx,flipxtics, subx,lx,dx,ls.x,-flipx,fmtx,
		ny,flipytics, suby,ly,dy,ls.y,flipy,fmty,fs,0,1,2, 
                grid, 
		length.x, length.y, labelcolor, ticcolor,
		axescolor, gridcolor, labelscale, fontname, xptr, yptr, 
		xlabptr, ylabptr);
      if (!rc) goto error;
    }
    if (ay>0 && az>0) {
      Vector cc;
      if (c) cc = DXVec(c->y, c->z, c->x);
      rc = mark(g, frontframe, showaxes, ylabel, zlabel, flipylab, -flipzlab,
		reverse, DXVec(o.y,o.z,o.x), DXVec(s.y,s.z,s.x), 
		DXVec(sgn.y,sgn.z,sgn.x), 
		c ? &cc : NULL,
		ny,flipytics, suby,ly,dy,ls.y,-flipy,fmty,
		nz,flipztics, subz,lz,dz,ls.z,flipz,fmtz,fs,2,0,1, grid,
		length.y, length.z, labelcolor,
		ticcolor, axescolor, gridcolor, labelscale, fontname,
		yptr, zptr, ylabptr, zlabptr);
      if (!rc) goto error;
    }
    if (az>0 && ax>0) {
      Vector cc;
      if (c) cc = DXVec(c->z, c->x, c->y);
      rc = mark(g, frontframe, showaxes, zlabel, xlabel, flipzlab, -flipxlab, 
		reverse, DXVec(o.z,o.x,o.y), DXVec(s.z,s.x,s.y), 
		DXVec(sgn.z,sgn.x,sgn.y), c? &cc : NULL,
		nz,flipztics, subz,lz,dz,ls.z,-flipz,fmtz,
		nx,flipxtics, subx,lx,dx,ls.x,flipx,fmtx, fs,1,2,0, 
                grid,
		length.z, length.x, labelcolor,
		ticcolor, axescolor, gridcolor, labelscale, fontname,
		zptr,xptr,zlabptr,xlabptr);
      if (!rc) goto error;
    }
    /* all ok */
    DXFree((Pointer)xptr);
    DXFree((Pointer)yptr);
    DXFree((Pointer)zptr);
    DXFree((Pointer)xlabptr);
    DXFree((Pointer)ylabptr);
    DXFree((Pointer)zlabptr);
    return (Object) g;
    
  error:
    DXFree((Pointer)xptr);
    DXFree((Pointer)yptr);
    DXFree((Pointer)zptr);
    DXFree((Pointer)xlabptr);
    DXFree((Pointer)ylabptr);
    DXFree((Pointer)zlabptr);
    DXDelete((Object)g);
    return NULL;
  }



struct axes_struct{
  Object object;
  Camera camera;
  char *xlabel;
  char *ylabel;
  char *zlabel;
  float labelscale;
  char *font;
  int t;
  int tx;
  int ty;
  int tz;
  Object corners;
  int frame;
  int adjust;
  Point *cursor;
  char *plottypex;
  char *plottypey;
  char *plottypez; 
  int grid;
  RGBColor labelcolor;
  RGBColor ticcolor;
  RGBColor axescolor;
  RGBColor gridcolor;
  RGBColor backgroundcolor;
  Array xlocs;
  Array ylocs;
  Array zlocs;
  Array xlabels;
  Array ylabels;
  Array zlabels;
};


extern 
Object
_dxfAutoAxes(Pointer p)
{
  Object object, corners;
  Camera cam; 
  char *xlabel, *ylabel, *zlabel;
  char *fontname;
  int n, nx, ny, nz, frame, adjust, grid;
  Point *cursor;
  RGBColor labelcolor, ticcolor, axescolor, gridcolor, backgroundcolor;
  


  Point box[8], origin, eye, min, max, mm[2], c;
  float mm2[4];
  Vector scale, sgn, ls, length, o, from, to, up;
  int i, numx, numy, numz;
  float l, labelscale;
  Object axes;
  Group g;
  Array xlocs, ylocs, zlocs, xlabels, ylabels, zlabels;
  float *xptr, *yptr, *zptr;


  struct axes_struct *axes_struct = (struct axes_struct *)p;
  
  object = axes_struct->object;
  cam = axes_struct->camera;
  xlabel = axes_struct->xlabel; 
  ylabel = axes_struct->ylabel; 
  zlabel = axes_struct->zlabel; 
  labelscale = axes_struct->labelscale; 
  fontname = axes_struct->font; 
  n = axes_struct->t; 
  nx = axes_struct->tx; 
  ny = axes_struct->ty; 
  nz = axes_struct->tz; 
  corners = axes_struct->corners;
  frame = axes_struct->frame;
  adjust = axes_struct->adjust;
  cursor = axes_struct->cursor; 
  /*plottypex = axes_struct->plottypex;*/  /*  FIXME:  Never used.  A bug?  */
  /*plottypey = axes_struct->plottypey;*/  /*  FIXME:  Never used.  A bug?  */
  /*plottypez = axes_struct->plottypez;*/  /*  FIXME:  Never used.  A bug?  */
  grid = axes_struct->grid;
  labelcolor = axes_struct->labelcolor;
  ticcolor = axes_struct->ticcolor;
  axescolor = axes_struct->axescolor;
  gridcolor = axes_struct->gridcolor;
  backgroundcolor = axes_struct->backgroundcolor;
  xlocs = axes_struct->xlocs;
  ylocs = axes_struct->ylocs;
  zlocs = axes_struct->zlocs;
  xlabels = axes_struct->xlabels;
  ylabels = axes_struct->ylabels;
  zlabels = axes_struct->zlabels;
  
  /* label scale based on transform */
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
  
  
  /* get camera info */
  if (cam) {
    if (DXGetObjectClass((Object)cam)!=CLASS_CAMERA)
      DXErrorReturn(ERROR_BAD_PARAMETER, "bad camera");
    DXGetView(cam, &from, &to, &up);
  } else {
    to = DXVec(0,0,0);
    from = DXVec(0,0,1);
    up = DXVec(0,1,0);
  }
  eye = DXSub(from, to);
  
  
  /* obtain min/max */
  if (DXExtractParameter(corners, TYPE_FLOAT, 3, 2, (Pointer)mm)) {
    min.x = mm[0].x / ls.x;
    min.y = mm[0].y / ls.y;
    min.z = mm[0].z / ls.z;
    max.x = mm[1].x / ls.x;
    max.y = mm[1].y / ls.y;
    max.z = mm[1].z / ls.z;

  }
  else if (DXExtractParameter(corners, TYPE_FLOAT, 2, 2, (Pointer)mm2)) {
    min.x = mm2[0] / ls.x;
    min.y = mm2[1] / ls.y;
    min.z = 0;
    max.x = mm2[2] / ls.x;
    max.y = mm2[3] / ls.y;
    max.z = 0;
  } else {
    if (corners) {
      if (!DXBoundingBox(corners, box)) {
	/* corners parameter must either be 2 vectors or be an object 
	   with a bounding box */
	DXSetError(ERROR_BAD_PARAMETER, "#11815");
	return ERROR;
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
    } else {  /* corners were not given; use the actual bounding box */
      if (!DXBoundingBox(object, box)) {
	if (DXGetError()==ERROR_NONE)
	  DXSetError(ERROR_BAD_PARAMETER,
		     "bad input parameter (has no bounding box)");
	return ERROR;
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
      /* if the user gave tic positions, these can override the object's
         bounding box (but should not override given corners) */
      if (xlocs)
       {
         if (!DXGetArrayInfo(xlocs, &numx, NULL, NULL, NULL, NULL))
            return ERROR; 
         xptr = DXGetArrayData(xlocs);
         if (!xptr) return ERROR;
         if (xptr[0]/ls.x < min.x) min.x = xptr[0]/ls.x;
         if (xptr[numx-1]/ls.x > max.x) max.x = xptr[numx-1]/ls.x;
       }
      if (ylocs)
       {
         if (!DXGetArrayInfo(ylocs, &numy, NULL, NULL, NULL, NULL))
            return ERROR;
         yptr = DXGetArrayData(ylocs);
         if (!yptr) return ERROR;
         if (yptr[0]/ls.y < min.y) min.y = yptr[0]/ls.y;
         if (yptr[numy-1]/ls.y > max.y) max.y = yptr[numy-1]/ls.y;
       }
      if (zlocs)
       {
         if (!DXGetArrayInfo(zlocs, &numz, NULL, NULL, NULL, NULL))
            return ERROR;
         zptr = DXGetArrayData(zlocs);
         if (!zptr) return ERROR;
         if (zptr[0]/ls.z < min.z) min.z = zptr[0]/ls.z;
         if (zptr[numz-1]/ls.z > max.z) max.z = zptr[numz-1]/ls.z;
       }
    }
  }
  
    
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
  
  /* 3-d: adjust for viewpoint */
  /* problem is that this takes no account of "up" */
  if (scale.z != 0) {
    if (eye.x * sgn.x < 0)
      origin.x += scale.x, scale.x = -scale.x, sgn.x = -sgn.x;
    if (eye.y * sgn.y < 0)
      origin.y += scale.y, scale.y = -scale.y, sgn.y = -sgn.y;
    if (eye.z * sgn.z < 0)
      origin.z += scale.z, scale.z = -scale.z, sgn.z = -sgn.z;
    
    /* 2-d: labels left, bottom */
  } else {
    if (sgn.x * eye.z > 0)
      origin.x += scale.x, scale.x = -scale.x, sgn.x = -sgn.x;
    if (sgn.y > 0)
      origin.y += scale.y, scale.y = -scale.y, sgn.y = -sgn.y;
    if (eye.z < 0)
      sgn.z = -sgn.z;
  }

  /* transform the box, record on-screen lengths of each side */
  if (cam) {
    Vector tx, ty, tz;
    Matrix m;
    m = DXGetCameraMatrix(cam);
    o = DXApply(origin, m);
    tx = DXApply(DXAdd(origin,DXVec(scale.x,0,0)), m);
    ty = DXApply(DXAdd(origin,DXVec(0,scale.y,0)), m);
    tz = DXApply(DXAdd(origin,DXVec(0,0,scale.z)), m);
    if (DXGetPerspective(cam, NULL, NULL)) {
      o.x /= o.z;    o.y /= o.z;	/* XXX */
      tx.x /= tx.z;  tx.y /= tx.z;    /* XXX */
      ty.x /= ty.z;  ty.y /= ty.z;    /* XXX */
      tz.x /= tz.z;  tz.y /= tz.z;    /* XXX */
    }
    tx = DXSub(tx,o);
    ty = DXSub(ty,o);
    tz = DXSub(tz,o);
    tx.z = ty.z = tz.z = 0;
    length.x = DXLength(tx);
    length.y = DXLength(ty);
    length.z = DXLength(tz);

    /* add up the sides, figure # labels from that */
    /* default condition */
    if (n==-9999) {
      l = length.x + length.y + length.z;
      n = l / 30;
      if (n>100) {
	DXWarning("using 100 tick marks instead of %d", n);
	n = 100;
      }
      if (n < 2) n=2;
    }
    
  } else {
    
    /* assume on-screen size is same as given size */
    length = scale;
    
  }
  
  /* scale the cursor */
  if (cursor)
    c = *cursor;  
  
  /* do the axes */
  axes = _Axes(origin, scale, sgn, ls, length, cursor? &c : NULL,
	       backgroundcolor, backgroundcolor, backgroundcolor,
	       xlabel, ylabel, zlabel, frame, n, nx, ny, nz, adjust,
	       grid, labelcolor, ticcolor,
	       axescolor, gridcolor, labelscale, fontname, up, eye,
	       xlocs,ylocs,zlocs,xlabels,ylabels,zlabels);
  if (!axes)
    return NULL;
  
  if (! DXSetIntegerAttribute(axes, "pickable", 0))
    {
      DXDelete((Object)axes);
      return NULL;
    }
  
  g = DXNewGroup();
  if (!g) return NULL;
  DXSetMember(g, NULL, axes);
  DXSetMember(g, NULL, object);
  
  return (Object)g; 
}


extern Pointer _dxfNewAxesObject(void)
{
  struct axes_struct *new;
  
  new = (struct axes_struct *)DXAllocate(sizeof(struct axes_struct));
  /* fill the defaults */
  if (new) {
  new->object=NULL;
  new->xlabel="x";
  new->ylabel="y";
  new->zlabel="z";
  new->font="fixed";
  new->t=10;
  new->tx=0;
  new->ty=0;
  new->tz=0;
  new->corners=NULL;
  new->frame=0;
  new->adjust=0;
  new->cursor=NULL;
  new->plottypex="lin";
  new->plottypey="lin";
  new->plottypez="lin";
  new->grid=0;
  new->labelcolor=LABELCOLOR;
  new->ticcolor=TICKCOLOR;
  new->axescolor=AXESCOLOR;
  new->gridcolor=GRIDCOLOR;
  new->backgroundcolor=BACKGROUNDCOLOR;
  return (Pointer)new;
  }
  return NULL;
}





extern Error _dxfSetAxesCharacteristic(Pointer p, char *characteristic,
                                       Pointer value)
{
  struct axes_struct *st = (struct axes_struct *)p;
  
  /* objects */
  if (!strcmp(characteristic,"OBJECT")) 
    st->object = *(Object *)value;
  else if (!strcmp(characteristic,"CORNERS")) 
    st->corners = *(Object *)value;
  
  /* cameras */
  else if (!strcmp(characteristic,"CAMERA")) 
    st->camera = *(Camera *)value;
  
  /* strings */
  else if (!strcmp(characteristic,"XLABEL")) 
    st->xlabel = (char *)value;
  else if (!strcmp(characteristic,"YLABEL")) 
    st->ylabel = (char *)value;
  else if (!strcmp(characteristic,"ZLABEL")) 
    st->zlabel = (char *)value;
  else if (!strcmp(characteristic,"TYPEX")) 
    st->plottypex = (char *)value;
  else if (!strcmp(characteristic,"TYPEY")) 
    st->plottypey = (char *)value;
  else if (!strcmp(characteristic,"TYPEZ")) 
    st->plottypez = (char *)value;
  else if (!strcmp(characteristic,"FONT")) 
    st->font = (char *)value;
  
  /* integers */
  else if (!strcmp(characteristic,"NUMTICS")) 
    st->t = *(int *)value;
  else if (!strcmp(characteristic,"NUMTICSX")) 
    st->tx = *(int *)value;
  else if (!strcmp(characteristic,"NUMTICSY")) 
    st->ty = *(int *)value;
  else if (!strcmp(characteristic,"NUMTICSZ")) 
    st->tz = *(int *)value;
  else if (!strcmp(characteristic,"FRAME")) 
    st->frame = *(int *)value;
  else if (!strcmp(characteristic,"ADJUST")) 
    st->adjust = *(int *)value;
  else if (!strcmp(characteristic,"GRID")) 
    st->grid = *(int *)value;
  
  /* Point *  */
  else if (!strcmp(characteristic,"CURSOR")) 
    st->cursor = (Point *)value;
  
  /* float */
  else if (!strcmp(characteristic,"LABELSCALE")) {
    st->labelscale= *(float *)value;
  }
  
  /* colors */
  else if (!strcmp(characteristic,"TICKCOLOR")) 
    st->ticcolor = *(RGBColor *)value;
  else if (!strcmp(characteristic,"LABELCOLOR")) 
    st->labelcolor = *(RGBColor *)value;
  else if (!strcmp(characteristic,"AXESCOLOR")) 
    st->axescolor = *(RGBColor *)value;
  else if (!strcmp(characteristic,"GRIDCOLOR")) 
    st->gridcolor = *(RGBColor *)value;
  else if (!strcmp(characteristic,"BACKGROUNDCOLOR")) 
    st->backgroundcolor = *(RGBColor *)value;


  /* Arrays */
  else if (!strcmp(characteristic,"XLOCS"))
    st->xlocs = (Array)value;
  else if (!strcmp(characteristic,"YLOCS"))
    st->ylocs = (Array)value;
  else if (!strcmp(characteristic,"ZLOCS"))
    st->zlocs = (Array)value;
  else if (!strcmp(characteristic,"XLABELS"))
    st->xlabels = (Array)value;
  else if (!strcmp(characteristic,"YLABELS"))
    st->ylabels = (Array)value;
  else if (!strcmp(characteristic,"ZLABELS"))
    st->zlabels = (Array)value;


  else {
    DXSetError(ERROR_DATA_INVALID,"unknown option %s", characteristic);
    return ERROR;
  }

  
  return OK;
}

Error _dxfCheckLocationsArray(Array locs, int *n, float **p)
{
  int numitems;
  float *ptr=NULL;
  
  /* check whether the array is floating point real and all that */
  if (!DXGetArrayInfo(locs, &numitems, NULL, NULL, NULL, NULL))
    goto error;
  ptr = (float *)DXAllocate(numitems*sizeof(float));
  *n = numitems;
  if (!DXExtractParameter((Object)locs, TYPE_FLOAT, 0, numitems, ptr)) {
    DXSetError(ERROR_DATA_INVALID,"tic locations must be int or floats");
    goto error;
  }
  *p = ptr;
  return OK;
 error:
  return ERROR;
  
}



Error _dxfGetFormat(float *locs, char *fmt, int num, float scale)
{
    int width, i; 
    char tbuf[32], *cp;
    int prec,z,minprec=1, lastz, maxwidth;
    char *cstring;
    float hi,lo,lg,range;
    double val;


    /* format */

    prec = 6;
    minprec = 1;
    lo = locs[0];
    hi = locs[num-1]; 

    lg = log10(MAX(ABS(lo),ABS(hi)));
    width = 1 + FL(lg);
    sprintf(tbuf, "%%%d.%df", width, prec);

    /* look for trailing zeros */

    for (i=0; i<num; i++) {
       sprintf(fmt, tbuf, locs[i]); 
       /* XXX added width to following line */
       /* lastz = prec + width + 
               1 + (tbuf[0]=='-' ? 1 : 0);  */
       lastz = strlen(fmt)-1;
       /* least significant digit */ 
       for (z=prec, cp= &fmt[lastz]; z>minprec; --z, --cp)
	  if (*cp != '0')				
	    break;
	
	minprec = MAX(minprec, z);
    }

    if (width<1)
      width = 1;
    if (minprec>0)			
      width += minprec + 1;
    if (lo<0 || hi<0)
      width += 1;

    cstring = (char *)getenv("DXAXESMAXWIDTH");
    if (cstring != NULL)
       maxwidth =atoi(cstring); 
    else
       maxwidth = MAXWIDTH;

    if (width <= maxwidth) {
      sprintf(fmt, "%%%d.%df", width, minprec);
      return OK;
    } 


    /* if the width of the format would be too long to look nice, 
     * use exponential notation and try to print the minimum number 
     * of significant digits.
     */
    { 

      /* exp format is [-]m.dddddde+XX, where the precision controls the
       *  number of digits in the fraction.  start with the default precision
       *  of 6 and look for trailing 0's.  if there are none, you are done.
       *  if some or all are 0's, go through each tick label and take the
       *  largest value which isn't 0.  the minimum return value is 1; this
       *  could be changed to 0 if you prefer "me+XX" instead of "m.0e+XX".
       */

      	 
      range = ABS(hi-lo); 
      for (i=0; i< num; i++) {
	val = locs[i]; 
	/* check for essentially zero relative to the range */
	if (ABS(val) < range/100000.) val=0.;
	sprintf(tbuf, "%.*e", prec, val);
	lastz = prec + 1 + (tbuf[0]=='-' ? 1 : 0); /* least significant digit */
	for (z=prec, cp= &tbuf[lastz]; z>minprec; --z, --cp)
	  if (*cp != '0')				
	    break;
	
	if (z == prec) {
	  sprintf(fmt, "%%.%de", prec);
	  return OK;
	}
	minprec = MAX(minprec, z);
      }
      sprintf(fmt, "%%.%de", minprec);
    }
    return OK;
}
