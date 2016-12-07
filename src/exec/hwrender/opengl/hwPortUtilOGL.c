/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../hwDeclarations.h"

#ifndef HELPERCODE

/*---------------------------------------------------------------------------*\
$Header: /src/master/dx/src/exec/hwrender/opengl/hwPortUtilOGL.c,v 1.17 2006/06/26 21:27:23 davidt Exp $

Author:  Ellen Ball

Based on hwrender/gl/hwPortUtil.c

\*---------------------------------------------------------------------------*/

#include <math.h>
#include <string.h>

#include "../hwDebug.h"

#ifdef TIMER
#undef TIMER
#endif

#ifdef DEBUG
#define TIMER(s) glFlush(); DXMarkTime(s);
#else
#define TIMER(s)
#endif

#include "../hwXfield.h"
#include "../hwWindow.h"
#include "../hwPortLayer.h"
#include "../hwTmesh.h"
#include "../hwObjectHash.h"
#include "../hwSort.h"
#include "hwPortOGL.h"


#if defined(HAVE_WINDOWS_H)
#include <windows.h>
#endif

#if defined(HAVE_WINGDI_H)
#include <wingdi.h>
#endif

#include <GL/gl.h>
#include <GL/glu.h>

static void loadTexture(xfieldP);
static void startTexture(xfieldP);
static void endTexture();

static int doesTrans;
/* screen door half-transparency pattern */
static GLubyte screen_door_50[] =
{
  0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
  0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
  0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
  0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
  0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
  0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
  0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
  0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
  0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
  0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
  0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
  0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
  0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
  0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
  0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
  0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55
} ;


#define LightModel(xf) \
   do {                                                   \
     if(xf->attributes.light_model == lm_two_side)        \
        glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);  \
     else                                                 \
        glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_FALSE); \
   }                                                      \
   while (0)

#define Cull(xf)                       \
   if(xf->attributes.cull_face == cf_back)  \
   {                                   \
      glEnable(GL_CULL_FACE);          \
      glCullFace(GL_BACK);             \
   }                                   \
   else                                \
   if(xf->attributes.cull_face == cf_front) \
   {                                   \
      glEnable(GL_CULL_FACE);          \
      glCullFace(GL_FRONT);            \
   }                                   \
   else                                \
   if(xf->attributes.cull_face == cf_front_and_back) \
   {                                   \
      glEnable(GL_CULL_FACE);          \
      glCullFace(GL_FRONT_AND_BACK);   \
   }                                   \
   else                                \
      glDisable(GL_CULL_FACE);


typedef void (*helperFunc) (xfieldP xf, int posPerConn, 
			    int ctIndex, int *translation);


static helperFunc
  getHelper(dependencyT colorsDep, 
	    dependencyT normalsDep, 
	    dependencyT opacitiesDep, 
	    int texture) ;

static Error
  points (xfieldP xf, helperFunc helper,
	  int firstConnection, int lastConnection, int face,
	  enum approxE approx, int skip) ;

static Error
  bbox (xfieldP xf) ;

#define ALL_PRIMS         0
#define OPAQUE_PRIMS      1
#define TRANSLUCENT_PRIMS 2

/*  We presume OpenGL >= 1.2. */
/*  If < OpenGL 1.3...        */
#ifndef GL_CLAMP_TO_BORDER
#  ifdef sgi
#    define GL_CLAMP_TO_BORDER GL_CLAMP_TO_BORDER_SGIS
#  else
#    define GL_CLAMP_TO_BORDER 0x812D
#  endif
#endif
#ifndef GL_CLAMP_TO_EDGE
#  define GL_CLAMP_TO_EDGE 0x812F
#endif


/*-------- These happen once per field -- outside glBegin/glEnd -------*/

#define	SENDPOSITION(offset) 						\
{ 								       	\
  const GLfloat *position ;						\
  position = (const GLfloat *)DXGetArrayEntry(xf->positions,offset,fscratch) ;\
  if(xf->shape < 3)							\
    glVertex2fv(position) ;  	/* When only x and y are specified   	\
				 * z defaults to 0.0 */			\
  else									\
    glVertex3fv(position) ;						\
}

#define	SENDUV(offset) 							\
{ 								       	\
  const GLfloat *uv ;							\
  uv = (const GLfloat *)DXGetArrayEntry(xf->uv,offset,uvscratch) ;	\
  glTexCoord2fv(uv) ;							\
}

#define OPACITY(offset)							\
    ((xf->omap) ? *(float *)                                            \
      DXGetArrayEntry(xf->omap,                                         \
          *(char *) DXGetArrayEntry(xf->opacities, offset, &iscratch),  \
          fscratch) :                                                   \
      ((xf->opacities) ?                                		\
          *(float *) DXGetArrayEntry(xf->opacities, offset, fscratch)   \
          : 1.0f ))

#if defined(alphax)
#undef _dxf_SERVICES_FLAGS
extern hwFlags _dxf_SERVICES_FLAGS();
#define OPACITY_STIPPLE							\
{									\
if (!_dxf_isFlagsSet(_dxf_SERVICES_FLAGS(),				\
                        SF_TRANSPARENT_OFF)) {				\
     if((xf->opacitiesDep != dep_none && OPACITY(0) < .75) ||		\
         xf->attributes.flags & CONTAINS_VOLUME) { 			\
        glEnable (GL_POLYGON_STIPPLE) ;					\
        glPolygonStipple (screen_door_50) ;				\
      }									\
  }									\
  }
#else
#define OPACITY_STIPPLE							\
{									\
  if((xf->opacitiesDep != dep_none && OPACITY(0) < .75) ||		\
      xf->attributes.flags & CONTAINS_VOLUME) { 			\
    glEnable (GL_POLYGON_STIPPLE) ;					\
    glPolygonStipple (screen_door_50) ;					\
  }									\
  }
#endif

static float color[4];

/*
static float _opacity = 1.0f;
static float ambient[4];
static float diffuse[4];
*/

static int separateAmbientAndDiffuse;
static int depthMaskState;
#define DEPTHMASK_SET		0
#define DEPTHMASK_UNSET		1

#define SENDOPACITY(offset)						\
{									\
   if((xf->opacitiesDep != dep_none) ||					\
      (xf->attributes.flags & CONTAINS_VOLUME))				\
   {									\
      float * opT;							\
      if(xf->opacities)							\
      {									\
	 _opacity = OPACITY(offset);					\
	 PRINT(("Opacity per field (%f)", _opacity));			\
      }									\
   }									\
}

#define	SENDNORMAL(offset) 						\
{ 									\
  oglVector *normal; 							\
  normal = (oglVector *)DXGetArrayEntry(xf->normals,offset,fscratch) ;	\
  glNormal3fv((const GLfloat *) normal) ;				\
}

#define SENDSPECCOLOR()                                 		\
{                                                       		\
  oglRGBColor rgbspec ;							\
  rgbspec.r = rgbspec.g = rgbspec.b =                   		\
      (GLfloat) xf->attributes.front.specular ;         		\
  glColorMaterial (GL_FRONT_AND_BACK, GL_SPECULAR) ;    		\
  glColor3fv ((const GLfloat  *)  &rgbspec) ;           		\
}

#define	APPLY_LIGHTING							\
{ 									\
  if (separateAmbientAndDiffuse)					\
  {  									\
       color[0] = xf->attributes.front.ambient * color[0];		\
       color[1] = xf->attributes.front.ambient * color[1];		\
       color[2] = xf->attributes.front.ambient * color[2];		\
  } 									\
  else									\
  { 									\
       color[0] = xf->attributes.front.diffuse * color[0];		\
       color[1] = xf->attributes.front.diffuse * color[1];		\
       color[2] = xf->attributes.front.diffuse * color[2];		\
  }									\
}

#define	GETCOLOR(offset) 						\
{ 									\
  float *cPtr;								\
									\
  if(xf->ncmap) 							\
  {									\
    cPtr = DXGetArrayEntry(xf->cmap,					\
	   *(ubyte  *) DXGetArrayEntry(xf->fcolors, offset, &iscratch),	\
	   fscratch);							\
  }									\
  else 									\
  {									\
    cPtr = DXGetArrayEntry(xf->fcolors, offset,fscratch);		\
  }									\
  									\
  color[0] = *cPtr;							\
  color[1] = *(cPtr+1);							\
  color[2] = *(cPtr+2);							\
}

#define SENDCOLOR							\
{									\
    glColor4fv(color);							\
}

#define SENDCOLOROPACITY						\
{									\
    glColor4fv(color);							\
}

#define GETOPACITY(offset)						\
{									\
      color[3] = OPACITY(offset);					\
}

#define SENDPERFIELD(lit) 						\
{ 									\
  int _lit = lit && xf->normalsDep != dep_none;				\
 									\
  if (xf->attributes.front.ambient == xf->attributes.front.diffuse) 	\
      separateAmbientAndDiffuse = 0;					\
  else									\
      separateAmbientAndDiffuse = 1;					\
									\
  if(xf->colorsDep == dep_field){              				\
     GETCOLOR(0);							\
     color[3] = 1.0f;							\
  }									\
  if(xf->opacitiesDep == dep_field)            				\
  {									\
    GETOPACITY(0);							\
    if (color[3] < 0.5)							\
    {									\
      glDepthMask(GL_FALSE);						\
      OGL_FAIL_ON_ERROR(glDepthMask);					\
      depthMaskState = DEPTHMASK_UNSET;					\
    }									\
    else								\
    {									\
      glDepthMask(GL_TRUE);						\
      OGL_FAIL_ON_ERROR(glDepthMask);					\
      depthMaskState = DEPTHMASK_SET;					\
    }									\
  }									\
  									\
  if(! _lit)								\
  {									\
    glDisable(GL_LIGHTING);						\
  }									\
  else 									\
  { 									\
    glEnable(GL_LIGHTING);						\
    SENDSPECCOLOR(); 							\
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);		\
    if(xf->normalsDep == dep_field) 					\
    {									\
      SENDNORMAL(0); 							\
      if (xf->colorsDep == dep_field)					\
	APPLY_LIGHTING;							\
    }									\
  } 									\
									\
  if(doesTrans)								\
  {									\
    if (xf->colorsDep == dep_field) {                                   \
      if (xf->opacitiesDep == dep_none) SENDCOLOR			\
      else if (xf->opacitiesDep == dep_field) SENDCOLOROPACITY;		\
    }									\
  }									\
  else									\
     OPACITY_STIPPLE;							\
}

#define ENDPERFIELD()							\
{									\
    if (! doesTrans) glDisable (GL_POLYGON_STIPPLE);			\
    if (depthMaskState == DEPTHMASK_UNSET)				\
    {									\
      glDepthMask(GL_TRUE);						\
      OGL_FAIL_ON_ERROR(glDepthMask);					\
      depthMaskState = DEPTHMASK_SET;					\
    }									\
}

/*
 * For cubes each face of the cube is drawn as a quad with 
 * face = [4..9] and posPerCon = 4.
 * For Tetras, triangles with face=[0..3] and posPerCon = 3
 */
static int faceList[][4] = {
  {0,1,2,0},	/* Tetra front */	/* <- also all triangles */
  {1,3,2,0},	/* Tetra right */
  {0,2,3,0},	/* Tetra left */
  {0,1,3,0},	/* Tetra bottom */

  {2,3,1,0},	/* Cube front */ 	/* <- also all quads */
  {0,1,5,4},	/* Cube bottom */
  {0,4,6,2},	/* Cube left */
  {1,5,7,3},	/* Cube right */
  {3,7,6,2},	/* Cube top */
  {4,5,7,6},	/* Cube back */
};
#define TRANSLATE(new,old,face) \
{  \
    int i;  \
    for(i=xf->posPerConn-1;i>=0;i--)  { \
      new[i] = old[faceList[face][i]]; \
    } \
}


#define GET_CONNECTION(ctIndex)	\
{					\
  static int	cscratch[8];		\
  static int	*connection;		\
                                        \
  connection = (int*)DXGetArrayEntry(xf->connections,ctIndex,cscratch);	\
  TRANSLATE(translation,connection,face); \
}

#define NOT_LIT 0
#define LIT 1

static Error dots (xfieldP xf, enum approxE approx, int skip)
{
  static float	fscratch[3];
  static int	iscratch;
  int		p,i;

  SENDPERFIELD(NOT_LIT);

  for(p = i = 0; p < xf->npositions; p += skip)
      if(!xf->invPositions || !DXIsElementInvalid(xf->invPositions, p))
      {
	if (i == 0)
	    glBegin(GL_POINTS);

	SENDCOLOR;
	SENDPOSITION(p);

	if (i++ == 256)
	{
	    glEnd();
	    i = 0;
	}
  }

  if (i != 0)
      glEnd();

  EXIT(("OK"));
  return OK;
}

static Error translucentPoints (xfieldP xf, helperFunc helper,
                                 SortD list, int n, int face,
				 enum approxE approx, int skip)
{
  static float	fscratch[3];
  static long	iscratch;
  int		i = 0, c, c1;

  glBegin(GL_POINTS);

  for(c = 0; c < n; c++)
  {
    if (i == 256)
    {
	glEnd();
	glBegin(GL_POINTS);
        SENDPERFIELD(NOT_LIT);
    }

    if (xf->connections)
	c1 = *(int*)DXGetArrayEntry(xf->connections,list[c].poly,(Pointer)iscratch);
    else
	c1 = list[c].poly;
	
    (*helper)(xf,1,list[c].poly,&c1);

    i++;
  }

  if (i)
    glEnd();

  return OK;
}

static Error points (xfieldP xf, helperFunc helper,
		     int firstConnection, int lastConnection, int face,
		     enum approxE approx, int skip)
{
  static int	translation[8];
  static float	fscratch[3];
  static long	iscratch;
  int		i = 0, c;
  dependencyT   odep;
  int 		translu = 0;

  ENTRY(("points (0x%x, 0x%x, %d, %d, %d, %d, %d)",
	 xf, helper, firstConnection, lastConnection, face, approx, skip));

  SENDPERFIELD(NOT_LIT);

  if (doesTrans)
  {
      switch(odep = xf->opacitiesDep)
      {
         case dep_none:
	     translu = 0;
	     break;
         case dep_field:
	         translu = OPACITY(0) < 1.0f;
             break;
         default:
	     break;
      }

      if (! translu)
      {
	  for(c=firstConnection;c<lastConnection;c+=skip)
	  {
	    if(!xf->invCntns || ! DXIsElementInvalid(xf->invCntns,c))
	    {
		int c1;

		if (xf->connections)
		    c1 = *(int*)DXGetArrayEntry(xf->connections, c, (Pointer)iscratch);
		else
		    c1 = c;

		if(xf->invPositions && DXIsElementInvalid(xf->invPositions, c1))
		    continue;

		if (odep == dep_connections)
		    translu |= OPACITY(c) < 1.0f;
		else if (odep == dep_positions)
		    translu |= OPACITY(c1) < 1.0f;

		if (! translu)
		{
		    if (i == 0)
			glBegin(GL_POINTS);

		    (*helper)(xf,1,c,&c1);

			if (++i == 256)
		    {
			i = 0;
			glEnd();
		    }
		}
	    }
	  }

	  if (i)
	    glEnd();
      }
  }
  else
  {
      for(c=firstConnection;c<lastConnection;c+=skip)
      {
	if(!xf->invCntns || ! DXIsElementInvalid(xf->invCntns,c))
	{
	    int c1;

	    if (xf->connections)
		c1 = *(int*)DXGetArrayEntry(xf->connections, c, (Pointer)iscratch);
	    else
		c1 = c;

	    if(xf->invPositions && DXIsElementInvalid(xf->invPositions, c1))
		continue;

	    if (i == 0)
		glBegin(GL_POINTS);

	    (*helper)(xf,xf->posPerConn,c,translation);

	    if (++i == 256)
	    {
		i = 0;
		glEnd();
	    }
	}
      }

      if (i)
	glEnd();
  }

  ENDPERFIELD();

  EXIT((""));
  return OK;
}


static Error translucentLines (xfieldP xf, helperFunc helper,
                                SortD list, int n, int face,
				enum approxE approx, int skip)
{
  static int	translation[8];
  int		c;

  glBegin(GL_LINES);

  for(c = 0; c < n; c++)
  {
    if(!xf->invCntns || ! DXIsElementInvalid(xf->invCntns,c))
    {
	GET_CONNECTION(list[c].poly);
	(*helper)(xf,xf->posPerConn,list[c].poly,translation);
    }
  }

  glEnd();

  return OK;
}

static Error lines (xfieldP xf, helperFunc helper,
		    int firstConnection, int lastConnection,int face,
		    enum approxE approx, int skip)
{
  static int	translation[8];
  static float	fscratch[3];
  static int	iscratch;
  int		c;
  dependencyT   odep;
  int 		translu = 0;

  ENTRY(("lines (0x%x, 0x%x, %d, %d, %d, %d, %d)",
	 xf, helper, firstConnection, lastConnection, face, approx, skip));

  if(approx == approx_box)
  {
    EXIT(("calling bbox"));
    return bbox(xf);
  }

  SENDPERFIELD(NOT_LIT);

  if (doesTrans)
  {
      switch(odep = xf->opacitiesDep)
      {
         case dep_none:
	     translu = 0;
	     break;
         case dep_field:
	     translu = OPACITY(0) < 1.0f;
             break;
         default:
	     break;
      }

      for(c=firstConnection;c<lastConnection;c+=skip)
      {
	if(!xf->invCntns || ! DXIsElementInvalid(xf->invCntns,c))
	{
	  GET_CONNECTION(c);
	  if (odep == dep_connections)
	      translu |= OPACITY(c) < 1.0f;
	  else if (odep == dep_positions)
	      translu |= (OPACITY(translation[0]) < 1.0f) ||
				(OPACITY(translation[1]) < 1.0f);

	  if (! translu)
	  {
	    glBegin(GL_LINES);
	    (*helper)(xf,xf->posPerConn,c,translation);
	    glEnd();
	  }
	}
      }
  }
  else
  {
      for(c=firstConnection;c<lastConnection;c+=skip)
      {
	if(!xf->invCntns || ! DXIsElementInvalid(xf->invCntns,c))
	{
	  glBegin(GL_LINES);
	  GET_CONNECTION(c);
	  (*helper)(xf,xf->posPerConn,c,translation);
	  glEnd();
	}
      }
  }

  ENDPERFIELD();

  EXIT((""));
  return OK;
}

static Error closedlines (xfieldP xf, helperFunc helper,
			  int firstConnection, int lastConnection,int face,
			  enum approxE approx, int skip)
{
  static int	translation[8];
  static float	fscratch[3];
  static int	iscratch;
  int		c;

  ENTRY(("closedlines(0x%x, 0x%x, %d, %d, %d, %d, %d",
	 xf, helper, firstConnection, lastConnection, face, approx, skip));

  if(approx == approx_box)
  {
    EXIT(("calling bbox"));
    return bbox(xf);
  }

  SENDPERFIELD(NOT_LIT);

  for(c=firstConnection;c<lastConnection;c+=skip)
  {
    if(!xf->invCntns || ! DXIsElementInvalid(xf->invCntns,c))
    {
      glBegin (GL_LINE_LOOP);
      GET_CONNECTION(c);
      (*helper)(xf,xf->posPerConn,c,translation);
      glEnd();
    }
  }

  ENDPERFIELD();

  EXIT((""));
  return OK;
}



static Error translucentPolygons(xfieldP xf, helperFunc helper,
                                 SortD list, int n, int face,
                                 enum approxE approx, int skip)
{
   static int	translation[8];
   int 		i;

   for (i = 0; i < n; i++)
   {
      GET_CONNECTION(list[i].poly);
      glBegin(GL_POLYGON);
         (*helper)(xf,xf->posPerConn,list[i].poly,translation);
      glEnd();
   }

   EXIT((""));
   return OK;
}

static Error polygons (xfieldP xf, helperFunc helper,
		       int firstConnection, int lastConnection, int face,
		       enum approxE approx, int skip)
{
   static int	translation[8];
   static float	fscratch[3];
   static int	iscratch;
   register int	c;
   register int	c1, translu = 0;
   dependencyT  odep;

   ENTRY(("polygons(0x%x, 0x%x, %d, %d, %d, %d, %d)",
	  xf, helper, firstConnection, lastConnection, face, approx, skip));

   switch(approx)
   {
      case approx_dots:
        EXIT(("calling dots"));
        return dots(xf,approx,skip);
        break;
      case approx_box:
        EXIT(("calling bbox"));
        return bbox(xf);
        break;
      case approx_lines:
        helper = getHelper(xf->colorsDep,dep_none,dep_none,xf->texture != NULL);
        EXIT(("calling closedlines"));
        return closedlines(xf,helper,0,xf->nconnections,face,approx,skip);
        break;
      case approx_none: case approx_flat: break;
   }

   SENDPERFIELD(LIT);

   if(doesTrans)
   {
      if(xf->posPerConn > 4)
      {
         PRINT(("Error..."));
         DXSetError(ERROR_BAD_PARAMETER, "Positions", "");
         return(ERROR);
      }
 
      switch(odep = xf->opacitiesDep)
      {
         case dep_none:
	     translu = 0;
	     break;
         case dep_field:
	     translu = OPACITY(0) < 1.0f;
             break;
         default:
	     break;
      }

     for(c=firstConnection;c<lastConnection;c+=skip)
     {
          if(!xf->invCntns || ! DXIsElementInvalid(xf->invCntns,c))
          {
             GET_CONNECTION(c);

             if(odep == dep_positions)
             {
	        translu = 0;
	        for(c1 = 0; c1 < xf->posPerConn; c1 ++)
	        {
	           PRINT(("(translation[%d] = %d) = %f",
		         c1, translation[c1], OPACITY(translation[c1])));
	           translu |= OPACITY(translation[c1]) < 1.0f;
	        }
             }
             else if (odep == dep_connections)
	     {
		translu = OPACITY(c) < 1.0f;
	     }
   
             if(!translu)
             {
                glBegin(GL_POLYGON);
                   (*helper)(xf,xf->posPerConn,c,translation);
                glEnd();
             }
         }
      }
   }
   else /* !doesTrans */
   {
      for(c=firstConnection;c<lastConnection;c+=skip)
      {
         GET_CONNECTION(c);
         glBegin(GL_POLYGON);
            (*helper)(xf,xf->posPerConn,c,translation);
         glEnd();
      }
   }

   ENDPERFIELD();

   EXIT((""));
   return OK;
}


static int _drawTranslucentPrimitives(tdmPortHandleP portHandle, xfieldP xf,
	SortD list, int n, enum approxE approx, int density)
{
   helperFunc	helper;

   ENTRY(("_drawTranslucentPrims(0x%x, 0x%x, %d, %d)",
	  portHandle, xf, approx, density));

   switch (xf->connectionType)
   {
      case ct_none :
	 PRINT((" translucent points:"));
	 helper = getHelper(xf->colorsDep,xf->normalsDep,xf->opacitiesDep,
			    xf->texture != NULL) ;
         TIMER("> translucent points") ;
	 translucentPoints(xf,helper,list,n,0,approx,density) ;
	 TIMER("< translucent points") ;
	 break;
      case ct_lines :
	 PRINT((" translucent lines:"));
         if(xf->attributes.aalines) {
            glEnable(GL_LINE_SMOOTH);
            glHint(GL_LINE_SMOOTH_HINT, GL_DONT_CARE);
         }
	 helper = getHelper(xf->colorsDep,xf->normalsDep,xf->opacitiesDep,
			    xf->texture != NULL) ;
         TIMER("> translucent lines") ;
	 translucentLines(xf,helper,list,n,0,approx,density) ;
	 TIMER("< translucent lines") ;
         glDisable(GL_LINE_SMOOTH);
	 break;
      case ct_triangles :
	 PRINT((" tris:"));
	 helper = getHelper(xf->colorsDep,xf->normalsDep,xf->opacitiesDep,
			    xf->texture != NULL) ;
         TIMER("> triangles") ;
	 translucentPolygons(xf,helper,list,n,0,approx,density) ;
	 TIMER("< triangles") ;
	 break;
      case ct_quads:
	 PRINT((" quads:"));
	 helper = getHelper(xf->colorsDep,xf->normalsDep,xf->opacitiesDep,
                            xf->texture != NULL) ;
         TIMER("> quads") ;
         translucentPolygons(xf,helper,list,n,4,approx,density) ;
         TIMER("< quads") ;
         break;
      case ct_tetrahedra:
         PRINT((" tetra:"));
         helper = getHelper(xf->colorsDep,xf->normalsDep,xf->opacitiesDep, 0) ;
         TIMER("> tetras") ;
         translucentPolygons(xf,helper,list,n,0,approx,density) ;
         translucentPolygons(xf,helper,list,n,1,approx,density) ;
         translucentPolygons(xf,helper,list,n,2,approx,density) ;
         translucentPolygons(xf,helper,list,n,3,approx,density) ;
         TIMER("< tetras") ;
         break;
      case ct_cubes:
         PRINT((" cubes:"));
         helper = getHelper(xf->colorsDep,xf->normalsDep,xf->opacitiesDep, 0) ;
         TIMER("> cubes") ;
         translucentPolygons(xf,helper,list,n,4,approx,density) ;
         translucentPolygons(xf,helper,list,n,5,approx,density) ;
         translucentPolygons(xf,helper,list,n,6,approx,density) ;
         translucentPolygons(xf,helper,list,n,7,approx,density) ;
         translucentPolygons(xf,helper,list,n,8,approx,density) ;
         translucentPolygons(xf,helper,list,n,9,approx,density) ;
         TIMER("< cubes") ;
         break;
      case ct_tmesh:
      case ct_qmesh:
         /*  Note: we don't mesh translucent prims, so we'll never get here  */
         goto error;
         break;
      default:
         goto error;
         break;
   }
/* done: */
  EXIT(("OK"));
  return OK;

 error:

  EXIT(("ERROR"));
  return ERROR;
}


#define NOTUSED	0

static Error tmeshlines (xfieldP xf, helperFunc helper,
			 enum approxE approx, int skip);

static Error tmesh (xfieldP xf, helperFunc helper,
		    enum approxE approx, int skip)
{
  static int	*translation,*strip;
  static float	fscratch[3];
  static int	iscratch;
  int		count;
  int		meshI;
  int		skipDelta;
  InvalidComponentHandle tmpic,tmpip;
  struct  {
    int start;
    int count;
  }  *meshes;

  ENTRY(("tmesh (0x%x, 0x%x, %d, %d)",xf, helper, approx, skip));

  tmpic = xf->invCntns;
  tmpip = xf->invPositions;
  xf->invCntns = NULL;
  xf->invPositions = NULL;

  switch(approx)
  {
     case approx_dots:
       if(tmpic || tmpip)
       {
         helper = getHelper(xf->colorsDep,dep_none,dep_none,
			    xf->texture != NULL);
         points (xf, helper,0,xf->nconnections,0,approx,skip);
         PRINT(("called points"));
         goto done;
       }
       else
       {
         dots(xf,approx,skip);
         PRINT(("called dots"));
         goto done;
       }
       break;
     case approx_box:
       bbox(xf);
       PRINT(("called bbox"));
       goto done;
       break;
     case approx_lines:
       helper = getHelper(xf->colorsDep,dep_none,dep_none,xf->texture != NULL);
       tmeshlines(xf, helper, approx, skip);
       PRINT(("called tmeshlines"));
       goto done;
       break;
     case approx_none: case approx_flat: break;
  }

  SENDPERFIELD(LIT);

  /* NOTE:
   * The code below is optimized for tmesh. It depends on two 
   * qualities of our tmesh generator.
   * 1) tmeshes are NOT generated for Npc or Cpc
   * 2) tmeshes create a regular array in xf->connections_array;
   */
  translation = DXGetArrayData(xf->connections_array);

  if(!(meshes = DXGetArrayData(xf->meshes)))
     goto error;

  /* Not Skipping */
  if(skip <= 1)
  {
    for(meshI=0;meshI<xf->nmeshes;meshI++)
    {
      glBegin (GL_TRIANGLE_STRIP);
      (*helper)(xf,meshes[meshI].count,NOTUSED,
		&translation[meshes[meshI].start]);
      glEnd();
    }
  }
  else
  {  /* skipping */
    skipDelta = 0;
    for(meshI=0;meshI<xf->nmeshes;meshI++)
    {
      strip = &translation[meshes[meshI].start];

      glBegin(GL_TRIANGLES);
      for(count=skipDelta;
	  count < meshes[meshI].count-2;
	  count += skipDelta)
      {
	(*helper)(xf,3,NOTUSED,&strip[count]);
	skipDelta = skip;
      }
      glEnd();
      skipDelta = count - (meshes[meshI].count-2);
    }
  }

  ENDPERFIELD();

 done:
  
  xf->invCntns = tmpic;
  xf->invPositions = tmpip;

  EXIT(("OK"));
  return OK;

 error:
/*
XXX 
print error message
*/
  ENDPERFIELD();

  xf->invCntns = tmpic;
  xf->invPositions = tmpip;

  EXIT(("ERROR"));
  return ERROR;
}

static Error tmeshlines (xfieldP xf, helperFunc helper,
			 enum approxE approx, int skip)
{
  static int	*translation,*strip;
  static int	edge[MaxTstripSize+1];
  static float	fscratch[3];
  static int	iscratch;
  int		count;
  int		meshI;
  int		edgeI;
  int		skipDelta;
  struct  {
    int start;
    int count;
  }  *meshes;

  ENTRY(("tmeshlines(0x%x, 0x%x, %d, %d)",xf, helper, approx, skip));

  /* NOTE:
   * The code below is optimized for tmesh. It depends on two 
   * qualities of our tmesh generator.
   * 1) tmeshes are NOT generated for Npc or Cpc
   * 2) tmeshes create a regular array in xf->connections_array;
   */
  translation = DXGetArrayData(xf->connections_array);

  SENDPERFIELD(NOT_LIT);

  if(!(meshes = DXGetArrayData(xf->meshes)))
     goto error;

  if(skip <= 1)
  {	/* NOT skipping */
    for(meshI=0;meshI<xf->nmeshes;meshI++)
    {
      strip = &translation[meshes[meshI].start];

      /* Draw zigzap */
      glBegin (GL_LINE_STRIP);
      (*helper)(xf, meshes[meshI].count, NOTUSED, strip);
      glEnd();

      /* close boundary of zigzags with even indices */
      for(edgeI=0,count=0; count < meshes[meshI].count; count+=2) 
      {
	  if(strip[count] < 0) continue;	/* for swap */
	  edge[edgeI++] = strip[count];
      }
      if(edgeI > 1)
      {
	glBegin (GL_LINE_STRIP);
	(*helper)(xf,edgeI,NOTUSED,edge);
	glEnd();
      }

      /* close boundary of zigzags with odd indices */
      for(edgeI=0,count=1; count < meshes[meshI].count; count+=2) 
      {
	  if(strip[count] < 0)
	     continue;	/* for swap */
	  edge[edgeI++] = strip[count];
      }
      if(edgeI > 1)
      {
	glBegin (GL_LINE_STRIP);
	(*helper)(xf,edgeI,NOTUSED,edge);
	glEnd();
      }
    }
  }
  else
  { /* skipping */
    skipDelta = 0;
    for(meshI=0;meshI<xf->nmeshes;meshI++)
    {
      strip = &translation[meshes[meshI].start];

      for(count=skipDelta; count < meshes[meshI].count-2; count += skipDelta)
      {
	glBegin (GL_LINE_LOOP);
	(*helper)(xf,3,NOTUSED,&strip[count]);
	glEnd ();
	skipDelta = skip;
      }
      skipDelta = count - (meshes[meshI].count-2);
    }
  }

  ENDPERFIELD();

  EXIT(("OK"));
  return OK;

 error:
/*
XXX 
print error message
*/
  ENDPERFIELD();

  EXIT(("ERROR"));
  return ERROR;
}


/*******/

static Error polylines (xfieldP xf, helperFunc helper,
		    enum approxE approx, int skip)
{
    static float fscratch[3];
    static int	 iscratch;
    int		 c;
    int	         start, end, knt, max = 0, thisknt;

    if(approx == approx_box)
    {
	EXIT(("calling bbox"));
	return bbox(xf);
    }

    SENDPERFIELD(NOT_LIT);

    if(skip > 1)
    {
       for(c = 0; c < xf->npolylines; c ++)
       {
          if(!xf->invCntns || ! DXIsElementInvalid(xf->invCntns,c))
	  {
	     if(xf->colorsDep == dep_polylines)
		SENDCOLOR;
             start = xf->polylines[c];
	     end  = (c == xf->npolylines-1) ? xf->nedges : xf->polylines[c+1];
             knt = (end - start) - 1;

		glBegin(GL_LINES);
		while (knt > 0)
		{
		    thisknt = (knt > max) ? max : knt;

		    (*helper)(xf, 2, c, xf->edges+start);

		    start += skip;
		    knt -= skip;
		}
		glEnd();
	    }
	}
    }
    else
    {

	max = 100;

	for(c = 0; c < xf->npolylines; c ++)
	{
	    if(!xf->invCntns || ! DXIsElementInvalid(xf->invCntns,c))
	    {
	        if(xf->colorsDep == dep_polylines)
		   SENDCOLOR;
		start = xf->polylines[c];
		end = (c == xf->npolylines-1) ? xf->nedges :
						    xf->polylines[c+1];
		knt = end - start;

		glBegin(GL_LINE_STRIP);
		while (knt)
		{
		    thisknt = (knt > max) ? max : knt;

		    (*helper)(xf, thisknt, c, xf->edges+start);

		    start += (thisknt-1);
		    knt -= (thisknt-1);
		    if(thisknt == 1)
		       knt = 0;
		}
		glEnd();
	    }
	}
    }

    ENDPERFIELD();

    EXIT((""));
    return OK;
}
/*******/


static Error plines (xfieldP xf, helperFunc helper,
			enum approxE approx, int skip)
{
  static int	*translation,*strip;
  static float	fscratch[3];
  static int	iscratch;
  int		count;
  int		meshI;
  int		skipDelta;
  struct  {
    int start;
    int count;
  }  *meshes;

  ENTRY(("polylines(0x%x, 0x%x, %d, %d)",xf, helper, approx, skip));

  if(approx == approx_box)
  {
    EXIT(("calling bbox"));
    return bbox(xf);
  }

  translation = DXGetArrayData(xf->connections_array);

  SENDPERFIELD(NOT_LIT);

  if(!(meshes = DXGetArrayData(xf->meshes))) goto error;

#if defined(alphax)
/* This temporary fix is because DEC's Xserver core dumps when
   given more than 60 points per begin/end when rendering lines
   and there is a clipping plane active. */
{
  int		step;
  int		start;

#define MAX_PLINE_VERTS MaxTstripSize
  if(skip <= 1) {	/* NOT skipping */
    /* For each mesh....*/
    for(meshI=0;meshI<xf->nmeshes;meshI++) {
      count = meshes[meshI].count;
      start = meshes[meshI].start;
      /* DEC X server core dumps with too many verts/line when clipping */
      for(step = 0; step < count; step += MAX_PLINE_VERTS-1) {
	glBegin(GL_LINE_STRIP);
	(*helper)(xf, 
		  (((step + MAX_PLINE_VERTS) < count) ?
		   MAX_PLINE_VERTS :
		   (count - step)), 
		  NOTUSED, &translation[step+start]);
	glEnd();
      }
    }
  } else { /* skipping */
    skipDelta = 0;
    for(meshI=0;meshI<xf->nmeshes;meshI++) {
      strip = &translation[meshes[meshI].start];
      /* this is really bogus but we'll take two point lines here
	 because of DEC's X server's tendency to core dump when
	 given more */
      for(count=skipDelta;
	  count < meshes[meshI].count-1;
	  count += skipDelta) {
	glBegin (GL_LINES);
	(*helper)(xf,2,NOTUSED,&strip[count]);
	skipDelta = skip;
	glEnd ();
      }
      skipDelta = count - (meshes[meshI].count-1);
    }
  }
}
#else
  if(skip <= 1) {	/* NOT skipping */
    for(meshI=0;meshI<xf->nmeshes;meshI++) {
      /* This is for normal implementations */
      glBegin(GL_LINE_STRIP);
      (*helper)(xf, meshes[meshI].count, NOTUSED, 
		&translation[meshes[meshI].start]);
      glEnd();
    }
  } else { /* skipping */
    skipDelta = 0;
    for(meshI=0;meshI<xf->nmeshes;meshI++) {
      strip = &translation[meshes[meshI].start];
      glBegin (GL_LINES);
      for(count=skipDelta;
	  count < meshes[meshI].count-1;
	  count += skipDelta) {
	(*helper)(xf,2,NOTUSED,&strip[count]);
	skipDelta = skip;
      }
      glEnd ();
      skipDelta = count - (meshes[meshI].count-1);
    }
  }
#endif
  
  ENDPERFIELD();

  EXIT(("OK"));
  return OK;

 error:
/*
XXX 
print error message
*/
  ENDPERFIELD();

  EXIT(("ERROR"));
  return ERROR;
}

static Error bbox (xfieldP xf)
{
  static float	fscratch[3];
  static int	iscratch;
  static float	yellow[] = {1.0, 1.0, 0.0};

  ENTRY(("bbox(0x%x)",xf));

  SENDPERFIELD(NOT_LIT);

  glColor3fv ((const GLfloat *) &yellow[0]) ;
  glBegin (GL_LINE_STRIP);
  glVertex3fv((GLfloat*)&xf->box[0]);
  glVertex3fv((GLfloat*)&xf->box[1]);
  glVertex3fv((GLfloat*)&xf->box[3]);
  glVertex3fv((GLfloat*)&xf->box[2]);
  glVertex3fv((GLfloat*)&xf->box[0]);
  glVertex3fv((GLfloat*)&xf->box[4]);
  glVertex3fv((GLfloat*)&xf->box[5]);
  glVertex3fv((GLfloat*)&xf->box[7]);
  glVertex3fv((GLfloat*)&xf->box[6]);
  glVertex3fv((GLfloat*)&xf->box[4]);
  glEnd ();

  glBegin (GL_LINES);
  glVertex3fv((GLfloat*)&xf->box[2]);
  glVertex3fv((GLfloat*)&xf->box[6]);

  glVertex3fv((GLfloat*)&xf->box[3]);
  glVertex3fv((GLfloat*)&xf->box[7]);

  glVertex3fv((GLfloat*)&xf->box[1]);
  glVertex3fv((GLfloat*)&xf->box[5]);
  glEnd ();

  EXIT(("OK"));
  return OK;
}


#include "hwPortUtilOGL.help"

static helperFunc
getHelper(dependencyT colorsDep, 
	  dependencyT normalsDep, 
	  dependencyT opacitiesDep,
	  int texture)
{
  int		index;
  helperFunc	ret;

  ENTRY(("getHelper(%d, %d, %d)",colorsDep,normalsDep,opacitiesDep));

  index = getIndex(colorsDep, normalsDep, opacitiesDep,texture);
  ret = helperTable[index];

  return ret;
}

static Error
_drawPrimitives(tdmPortHandleP portHandle, xfieldP xf, 
		  enum approxE approx, int density)
{
  helperFunc	helper;
  
  ENTRY(("_drawPrimitives(0x%x, 0x%x, %d, %d)",
	 portHandle, xf, approx, density));
  
  if(xf->invCntns)
    if(! DXInitGetNextInvalidElementIndex(xf->invCntns))
      goto error;

  glGetFloatv(GL_MODELVIEW_MATRIX, (float *)xf->attributes.vm);
  glFinish() ;
  
  switch (xf->connectionType)
  {
  case ct_none:
    PRINT((" points:"));
    TIMER("> points") ;
    helper = getHelper(xf->colorsDep,dep_none,dep_none,xf->texture != NULL);
    points(xf,helper,0,xf->npositions,0,approx,density) ;
    TIMER("< points") ;
    break;
  case ct_lines:
    PRINT((" lines:"));
    if(xf->attributes.linewidth > 0)
       glLineWidth(xf->attributes.linewidth);

    if(xf->attributes.aalines) {
       glEnable(GL_LINE_SMOOTH);
       //glEnable(GL_BLEND);
       //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
       glHint(GL_LINE_SMOOTH_HINT, GL_DONT_CARE);
    }
    helper = getHelper(xf->colorsDep,dep_none,dep_none,xf->texture != NULL);
    TIMER("> lines") ;
    lines(xf,helper,0,xf->nconnections,0,approx,density) ;
    TIMER("< lines") ;
    glDisable(GL_LINE_SMOOTH);
    //glDisable(GL_BLEND);
    break;
  case ct_polylines:
     PRINT(("> polylines"));
     if(xf->attributes.linewidth > 0)
        glLineWidth(xf->attributes.linewidth);

     if(xf->attributes.aalines) {
        glEnable(GL_LINE_SMOOTH);
        //glEnable(GL_BLEND);
        //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glHint(GL_LINE_SMOOTH_HINT, GL_DONT_CARE);
     }
     helper = getHelper(xf->colorsDep, dep_none, dep_none, 0);
     TIMER("> polylines");
     polylines(xf, helper, approx, density);
     TIMER("< polylines");
     glDisable(GL_LINE_SMOOTH);
     //glDisable(GL_BLEND);
     break;
  case ct_pline:
     PRINT((" plines:"));
     if(xf->attributes.linewidth > 0)
        glLineWidth(xf->attributes.linewidth);

     if(xf->attributes.aalines) {
        glEnable(GL_LINE_SMOOTH);
        //glEnable(GL_BLEND);
        //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glHint(GL_LINE_SMOOTH_HINT, GL_DONT_CARE);
     }
     helper = getHelper(xf->colorsDep,dep_none,dep_none, xf->texture != NULL);
     TIMER("> plines");
     plines(xf,helper,approx,density);
     TIMER("< plines");
     glDisable(GL_LINE_SMOOTH);
     //glDisable(GL_BLEND);
     break;
  case ct_triangles: 
     PRINT((" tris:"));
     helper = getHelper(xf->colorsDep,xf->normalsDep,xf->opacitiesDep,
				    xf->texture != NULL) ;
     TIMER("> triangles") ;
     polygons(xf,helper,0,xf->nconnections,0,approx,density) ;
     TIMER("< triangles") ;
     break;
  case ct_quads:
     PRINT((" quads:"));
     helper = getHelper(xf->colorsDep,xf->normalsDep,xf->opacitiesDep,
				xf->texture != NULL) ;
     TIMER("> quads") ;
     polygons(xf,helper,0,xf->nconnections,4,approx,density) ;
     TIMER("< quads") ;
     break;
  case ct_tetrahedra:
    PRINT((" tetra:"));
    helper = getHelper(xf->colorsDep,xf->normalsDep,xf->opacitiesDep, 0) ;
    TIMER("> tetras") ;
    polygons(xf,helper,0,xf->nconnections,0,approx,density) ;
    polygons(xf,helper,0,xf->nconnections,1,approx,density) ;
    polygons(xf,helper,0,xf->nconnections,2,approx,density) ;
    polygons(xf,helper,0,xf->nconnections,3,approx,density) ;
    TIMER("< tetras") ;
    break;
  case ct_cubes:
    PRINT((" cubes:"));
    helper = getHelper(xf->colorsDep,xf->normalsDep,xf->opacitiesDep, 0) ;
    TIMER("> cubes") ;
    polygons(xf,helper,0,xf->nconnections,4,approx,density) ;
    polygons(xf,helper,0,xf->nconnections,5,approx,density) ;
    polygons(xf,helper,0,xf->nconnections,6,approx,density) ;
    polygons(xf,helper,0,xf->nconnections,7,approx,density) ;
    polygons(xf,helper,0,xf->nconnections,8,approx,density) ;
    polygons(xf,helper,0,xf->nconnections,9,approx,density) ;
    TIMER("< cubes") ;
    break;
  case ct_tmesh:
    PRINT((" tmesh:"));
    helper = getHelper(xf->colorsDep,xf->normalsDep,xf->opacitiesDep, xf->texture != NULL) ;
    TIMER("> tmesh") ;
    tmesh(xf,helper,approx,density) ;
    TIMER("< tmesh") ;
    break;
  case ct_qmesh:
    PRINT((" qmesh:"));
    helper = getHelper(xf->colorsDep,xf->normalsDep,xf->opacitiesDep, xf->texture != NULL) ;
    TIMER("> qmesh") ;
    tmesh(xf,helper,approx,density) ;
    TIMER("< qmesh") ;
    break;
#if 0
#ifdef DXD_HW_TEXTURE_MAPPING
  case ct_flatGrid:
    if(approx == approx_none) {
      PRINT((" texture:"));
      TIMER("> texture") ;
      if(!hwTextureDraw(portHandle, xf))
	goto error;
      TIMER("< texture") ;
      break;
    } else {
      PRINT((" texture pgons:"));
      helper = getHelper(xf->colorsDep,xf->normalsDep,xf->opacitiesDep, xf->texture != NULL) ;
      TIMER("> quads") ;
      polygons(xf,helper,0,xf->nconnections,4,approx,density) ;
      TIMER("< quads") ;
      break;
    }
#endif
#endif
  default:
    goto error;
    break;
  }

/* done: */
  EXIT(("OK"));
  return OK;

 error:

  EXIT(("ERROR"));
  return ERROR;
}

Error _dxf_DrawTranslucentOGL(void *globals, 
		RGBColor * ambientColor, int buttonUp)
{
  DEFGLOBALDATA(globals);
  DEFPORT(PORT_HANDLE);
  SortListP     sl = (SortListP)SORTLIST;
  SortD		sorted = sl->sortList;
  int 		nSorted = sl->itemCount;
  int           i,j;
  RGBColor	amCol;

  xfieldP       xf;
  int		density;
  enum approxE	approx;
  float		ambientLight;
  Light		newLight;
  float 	fscratch[3];
  int   	iscratch;
  dxObject      dlist = NULL;

  ENTRY(("_dxf_DrawTranslucentOGL(0x%x, 0x%x, 0x%x, 0x%x)",
         PORT_HANDLE, xf, ambientColor, buttonUp));

  //glEnable(GL_BLEND);
  //OGL_FAIL_ON_ERROR(glEnableBlend);
  //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  //OGL_FAIL_ON_ERROR(glBlendFunc);
  //glAlphaFunc(GL_GREATER, 0.0);
  //OGL_FAIL_ON_ERROR(glAlphaFunc);

  for (i = 0; i < nSorted; i = j)
  {
    xf = (xfieldP)sorted[i].xfp;

    Cull(xf);
    LightModel(xf);

    if(xf->normalsDep != dep_none &&
        xf->attributes.buttonUp.approx == approx_none)
    {
        _dxf_SET_GLOBAL_LIGHT_ATTRIBUTES(PORT_CTX ,1);

        if(!(xf->attributes.flags & CORRECTED_COLORS))
        {
          xf->attributes.front.ambient = 
	       sqrt((double)xf->attributes.front.ambient) ;
          xf->attributes.front.specular = 
	       sqrt((double)xf->attributes.front.specular) ;;
          xf->attributes.flags |= CORRECTED_COLORS;
        } /* end CORRECTED_COLORS */

        /* Beware divide by zero */
        if(xf->attributes.front.diffuse < .001f)
           xf->attributes.front.diffuse = .001f;

        ambientLight = xf->attributes.front.ambient;

        ambientLight = ambientLight / xf->attributes.front.diffuse;
        ambientLight = ambientLight * ambientLight;

        amCol.r = ambientColor->r * ambientLight;
        amCol.g = ambientColor->g * ambientLight;
        amCol.b = ambientColor->b * ambientLight;

        if(!(newLight = DXNewAmbientLight(amCol)))
          goto error;
    
        if(!_dxf_DEFINE_LIGHT (_wdata,1,newLight))
           goto error;

        DXDelete((dxObject)newLight) ;

        _dxf_SET_MATERIAL_SPECULAR(PORT_CTX, 1.0, 1.0, 1.0,
				   (float)xf->attributes.front.shininess);
    } /* end normals & no approximation */

    if (xf->texture)
    {
	if (DXGetObjectTag((dxObject)xf->texture_field) != CURTEX)
	{
	    CURTEX = DXGetObjectTag((dxObject)xf->texture_field);

	    if (DO_DISPLAY_LISTS)
	    {
		dlist = _dxf_QueryObject(TEXTURE_HASH, (dxObject)xf->texture_field);
		if (dlist)
		    _dxf_callDisplayListOGL(dlist);
		else
		{
		    dlist = _dxf_openDisplayListOGL();
		    loadTexture(xf);
		    _dxf_endDisplayListOGL();
		    _dxf_InsertObject(TEXTURE_HASH, (dxObject)xf->texture_field, dlist);
		    dlist = NULL;
		}
	    }
	    else
		loadTexture(xf);
	}

	startTexture(xf);
    }
    else
	endTexture();


    if (xf->nClips)
      if (! _dxf_ADD_CLIP_PLANES(LWIN, xf->clipPts, xf->clipVecs, xf->nClips))
        goto error;

    _dxf_PUSH_MULTIPLY_MATRIX(PORT_CTX,xf->attributes.mm) ;

    if(xf->colorsDep == dep_positions || xf->normalsDep == dep_positions)
       glShadeModel(GL_SMOOTH);
    else
       glShadeModel(GL_FLAT);

    if(buttonUp)
    {
       density = xf->attributes.buttonUp.density;
       approx = xf->attributes.buttonUp.approx;
    }
    else
    {
       density = xf->attributes.buttonDown.density;
       approx = xf->attributes.buttonDown.approx;
    }

    SENDPERFIELD(LIT);

    for (j = i+1; j < nSorted && xf == (xfieldP)sorted[j].xfp; j++);

    if(!_drawTranslucentPrimitives(PORT_HANDLE, xf, sorted+i, j-i, approx, density))
	goto error;

    ENDPERFIELD();

    if (xf->nClips)
      if (! _dxf_REMOVE_CLIP_PLANES(LWIN, xf->nClips))
        goto error;

    if(xf->texture)
       endTexture(xf);

    _dxf_SET_GLOBAL_LIGHT_ATTRIBUTES(PORT_CTX ,0) ;
    _dxf_POP_MATRIX(PORT_CTX) ;

   }

   //glDisable(GL_BLEND);

   //OGL_FAIL_ON_ERROR(_dxf_DrawTranslucentOGL);
   glFinish();
   EXIT(("OK"));
   return OK;

 error:

#if 0
   if(notFirstPass)
      glCallList(notFirstPass + 1);
#endif

   //glDisable(GL_BLEND);


   _dxf_SET_GLOBAL_LIGHT_ATTRIBUTES(PORT_CTX ,0) ;
   _dxf_POP_MATRIX(PORT_CTX) ;

   OGL_FAIL_ON_ERROR(_dxf_DrawTranslucentOGL);
   EXIT(("ERROR"));
   return ERROR;
}


Error _dxf_DrawOpaqueOGL(tdmPortHandleP portHandle, xfieldP xf, 
			 RGBColor ambientColor, int buttonUp)
{
  int		density;
  enum approxE	approx;
  float		ambient,diffuse;
  Light		newLight;
  dxObject      dlist = NULL;
  DEFPORT(portHandle) ;
  hwFlags attFlags = _dxf_attributeFlags(_dxf_xfieldAttributes(xf));
  int alphaTexture = (int) _dxf_isFlagsSet(attFlags,
					   CONTAINS_TRANSPARENT_TEXTURE);

  ENTRY(("_dxf_DrawOpaqueOGL(0x%x, 0x%x, 0x%x, 0x%x)",
	 portHandle, xf, ambientColor, buttonUp));

  /*  Alpha texture translucency is trapped out here.  Opacities            */
  /*     translucency caught in the primitive rendering funcs per element.  */
  if (alphaTexture) {
    EXIT(("OK"));
    return OK;
  }

  doesTrans = _dxf_isFlagsSet(_dxf_SERVICES_FLAGS(), SF_DOES_TRANS);

  Cull(xf);
  LightModel(xf);

  if(xf->normalsDep != dep_none &&
     xf->attributes.buttonUp.approx == approx_none)
  {
     _dxf_SET_GLOBAL_LIGHT_ATTRIBUTES(PORT_CTX ,1);

    if(!( xf->attributes.flags & CORRECTED_COLORS))
    {
      xf->attributes.front.ambient = 
	   sqrt((double)xf->attributes.front.ambient) ;
      xf->attributes.front.specular = 
	   sqrt((double)xf->attributes.front.specular) ;;
      xf->attributes.flags |= CORRECTED_COLORS;
    }

    /*
     * A major improvement for GL. We will ignore the ambient
     * coef in the device dependent stuff, instead we'll adjust 
     * the ambient light here and use the diffuse coef in place 
     * of the ambient in the device dependent code.
     */

    /* Beware divide by zero */
    if(xf->attributes.front.diffuse < .001f)
      xf->attributes.front.diffuse = .001f;

    ambient = xf->attributes.front.ambient;
    diffuse = xf->attributes.front.diffuse;
      
    ambient = ambient/diffuse;
    /*
     * DEFINE_LIGHT takes the sqrt() of the input, so we need to square 
     * the coefficient here
     */
    ambient = ambient * ambient;
    ambientColor.r *= ambient;
    ambientColor.g *= ambient;
    ambientColor.b *= ambient;

    if(!(newLight = DXNewAmbientLight(ambientColor)))
      goto error;
    
    /*
     * NOTE XXX A little hokey. We don't need the window to define
     * and ambient light, and we don't have it here.
     */
     if (!_dxf_DEFINE_LIGHT (OGLWINT,1,newLight))
      goto error;
    
    DXDelete((dxObject)newLight) ;

    _dxf_SET_MATERIAL_SPECULAR (PORT_CTX, 1.0 /*R*/, 1.0 /*G*/, 1.0 /*B*/,
				(float) xf->attributes.front.shininess
				/* specular power */) ;
  }

  if(xf->texture)
     startTexture(xf);

  _dxf_PUSH_MULTIPLY_MATRIX(PORT_CTX,xf->attributes.mm) ;

  if(xf->colorsDep == dep_positions || xf->normalsDep == dep_positions)
    glShadeModel(GL_SMOOTH);
  else
    glShadeModel (GL_FLAT);

  OGL_FAIL_ON_ERROR(glShadeModel);

  glDepthMask(GL_TRUE);
  OGL_FAIL_ON_ERROR(glDepthMask);

  if(buttonUp)
  {
    density = xf->attributes.buttonUp.density;
    approx = xf->attributes.buttonUp.approx;
  }
  else
  {
    density = xf->attributes.buttonDown.density;
    approx = xf->attributes.buttonDown.approx;
  }

  if (xf->texture)
  {
      if (DXGetObjectTag((dxObject)xf->texture_field) != CURTEX)
      {
          CURTEX = DXGetObjectTag((dxObject)xf->texture_field);

          if (DO_DISPLAY_LISTS)
          {
              dlist = _dxf_QueryObject(TEXTURE_HASH, (dxObject)xf->texture_field);
              if (dlist)
                  _dxf_callDisplayListOGL(dlist);
              else
              {
                  dlist = _dxf_openDisplayListOGL();
                  loadTexture(xf);
                  _dxf_endDisplayListOGL();
                  _dxf_InsertObject(TEXTURE_HASH, (dxObject)xf->texture_field, dlist);
		  dlist = NULL;
              }
          }
	  else
	      loadTexture(xf);
      }

      startTexture(xf);
  }
  else
      endTexture();

  if (DO_DISPLAY_LISTS)
  {
      dlist = _dxf_QueryDisplayList(DISPLAY_LIST_HASH, xf, buttonUp);
      if (dlist)
      {
          _dxf_callDisplayListOGL(dlist);
      }
      else
      {
          dlist = _dxf_openDisplayListOGL();
          if(!_drawPrimitives(portHandle, xf, approx, density))
          {
              _dxf_endDisplayListOGL();
              goto error;
          }
          _dxf_endDisplayListOGL();
          _dxf_InsertDisplayList(DISPLAY_LIST_HASH, xf, buttonUp, dlist);
      }

      dlist = NULL;
  }
  else
      if(!_drawPrimitives(portHandle, xf, approx, density))
          goto error;




  OGL_FAIL_ON_ERROR(Back);
/*  done: */
  if(xf->texture)
     endTexture(xf);

  /*
   * Disable lighting, reset matrix stack
   */
  _dxf_SET_GLOBAL_LIGHT_ATTRIBUTES(PORT_CTX ,0) ;
#if defined(DEBUG)
  WriteViewperfFile(portHandle, xf);
#endif
  _dxf_POP_MATRIX(PORT_CTX) ;

  OGL_FAIL_ON_ERROR(_dxf_DrawOpaqueOGL);

  EXIT(("OK"));
  return OK;

 error:

  /*
   * Disable lighting, reset matrix stack
   */
  _dxf_SET_GLOBAL_LIGHT_ATTRIBUTES(PORT_CTX ,0) ;
  _dxf_POP_MATRIX(PORT_CTX) ;

  OGL_FAIL_ON_ERROR(_dxf_DrawOpaqueOGL);
  EXIT(("ERROR"));
  return ERROR;
}

/*
static void
_deleteGLObject(xfieldP xf) ;


static void _deleteGLObject(xfieldP xf)
{

  ENTRY(("_deleteGLObject(0x%x)",xf));
  
  if(xf->glObject) {
    glDeleteLists(xf->glObject,1) ;
    xf->glObject = 0;
  }

  EXIT((""));
}
*/


#if defined(DEBUG)
#define POSITIONS 0
#define CONNECTIONS 1
#define NORMALS 2
WriteViewperfFile(void *instance, xfieldP xf)
{
/*
   static int been_here_done_that=0;
   
   if (been_here_done_that)
   return;
*/
  if (xf->nnormals != xf->npositions) {
    /* Only interested in lit fields */
    return;
  }

  if (getenv("DX_WRITE_VIEWPERF_FILE")) {
    GLfloat mat[4][4];
    char basename[40];

    glGetFloatv(GL_MODELVIEW_MATRIX, (GLfloat *)&mat);

    /*     been_here_done_that=1; */
    sprintf(basename,"dx.%d",(int)instance);

    WriteToFile(basename, xf, POSITIONS, &mat);
    WriteToFile(basename, xf, CONNECTIONS, &mat);
    WriteToFile(basename, xf, NORMALS, &mat);
  }
}

float *multMatrix(float *vec, float mat[4][4])
{
  float new[4];
  int i,j;


  for (i=0;i<3;i++) {
    new[i] = 0.0;
    for (j=0;j<3;j++) {
      new[i] += vec[j]*mat[j][i];
    }
    new[i] += mat[3][i];
  }

  for(i=0;i<3;i++)
    vec[i] = new[i];

  return vec;
}


#define DET33(s) \
        (( s[0][0] * ((s[1][1] * s[2][2]) - (s[2][1] * s[1][2])) ) + \
         (-s[0][1] * ((s[1][0] * s[2][2]) - (s[2][0] * s[1][2])) ) + \
         ( s[0][2] * ((s[1][0] * s[2][1]) - (s[2][0] * s[1][1])) ) )

float Mycofac (float s0[4][4], int s1, int s2)
{
  int i, j, I, J ;
  float cofac[4][4] ;

  for (i = 0, I = 0 ; i < 4 ; i++)
    if (i != s1) {
      for (j = 0, J = 0 ; j < 4 ; j++)
	if (j != s2)
	  cofac[I][J++] = s0[i][j] ;
      I++ ;
    }

  return DET33(cofac) ;
}


void adjointTranspose (float s1[4][4], float s0[4][4])
{
  int i, j ;

  for (i = 0 ; i < 3 ; i++)
    for (j = 0 ; j < 3 ; j++)
      s1[i][j] = ((((i + j + 1) % 2) * 2) - 1) * Mycofac(s0, i, j);

  for(i = 0; i < 3 ; i++) {
    s1[3][i] = 0.0; 
    s1[i][3] = 0.0;
  }

  s1[3][3] = 1.0;
}

WriteToFile(char *s, xfieldP xf, int type, float mat[4][4])
{
  int i, j;
  static int tot=0;
  float *pos;
  float fscratch[3];
  struct  {
    int start;
    int count;
  } *meshes;
  int *trans, meshI;
  int *con;
  int iscratch[3];
  FILE *fp = NULL;
  char filename[256];
  char extention[][8] = {
    ".coor",
    ".elem",
    ".vnorm"
  };
  float newmat[4][4];
  float *newpos;

  strcpy(filename, s);
  
  strcat(filename, extention[type]);
  
  fp = fopen(filename,"a+");

  if (!fp) {
    printf("Can not open input file: %s\n", filename);
    exit(0);
  }

  switch (type) {
  case POSITIONS:
    for (i=0;i<xf->npositions;i++) {
      pos = DXGetArrayEntry(xf->positions,i,fscratch);
      newpos = multMatrix(pos, mat);
      fprintf(fp,"%4d,% 7.6f,% 7.6f,% 7.6f\n", i+tot,
	      newpos[0], newpos[1], newpos[2]);
    }
    break;
  case CONNECTIONS:
    if (xf->connectionType == ct_tmesh ||
	xf->connectionType == ct_qmesh) {
      trans = DXGetArrayData(xf->connections_array); /* A list of all conns */
      meshes = DXGetArrayData(xf->meshes); /* a list of meshes */
      /* For all meshes */
      for(meshI=0;meshI<xf->nmeshes;meshI++) {
	/* print out all connections */
	fprintf(fp,"thing ");
	for (j=0;j<meshes[meshI].count;j++) {
	  fprintf(fp,"%4d ", trans[meshes[meshI].start+j]+tot);
	}
	fprintf(fp,"\n");
      }
    } else {
      for (i=0;i<xf->nconnections;i++) {
	con = (int*)DXGetArrayEntry(xf->connections,i,iscratch);
	fprintf(fp,"thing ");
	for (j=0;j<xf->posPerConn;j++) {
	  fprintf(fp,"%4d ",con[j]+tot);
	}
	fprintf(fp,"\n");
      }
    }
    break;
  case NORMALS:
    for (i=0;i<xf->npositions;i++) {
      pos = DXGetArrayEntry(xf->normals,i,fscratch);
      adjointTranspose(&newmat, mat);
      newpos = multMatrix(pos, &newmat);
      fprintf(fp,"% 7.6f % 7.6f % 7.6f\n",
	      newpos[0], newpos[1], newpos[2]);
    }
    tot+=xf->npositions;
    break;
  }
  fclose(fp);
}
#endif

static int isGLExtensionSupported( const char *extension )
  /* Blatent copy from: 
   *    http://www.opengl.org/developers/code/features/OGLextensions/\
   *    OGLextensions.html
   */
{
  const GLubyte *extensions = NULL;
  const GLubyte *start;
  GLubyte *where, *terminator;

  /* Extension names should not have spaces. */
  where = (GLubyte *) strchr(extension, ' ');
  if (where || *extension == '\0')
    return 0;
  extensions = glGetString(GL_EXTENSIONS);
  /* It takes a bit of care to be fool-proof about parsing the
     OpenGL extensions string. Don't be fooled by sub-strings,
     etc. */
  start = extensions;
  for (;;) {
      where = (GLubyte *) strstr((const char *) start, extension);
      if (!where)
          break;
      terminator = where + strlen(extension);
      if (where == start || *(where - 1) == ' ')
          if (*terminator == ' ' || *terminator == '\0')
              return 1;
      start = terminator;
  }
  return 0;
}

static int glSupports_CLAMP_TO_BORDER( void )
{
#ifdef GL_VERSION_1_3
  return 1;
#else
  static int Texture_border_supported = -1;
  if ( Texture_border_supported < 0 ) 
    Texture_border_supported = 
      ( !isGLExtensionSupported("SGIS_texture_border_clamp") &&
        !isGLExtensionSupported("GL_ARB_texture_border_clamp") );
  return Texture_border_supported;
#endif
}


static void
loadTexture(xfieldP xf)
{
    /*  Set texture  */
    GLenum type    = xf->textureIsRGBA ? GL_RGBA : GL_RGB;
    GLint  int_fmt = xf->textureIsRGBA ? 4 : 3;
    gluBuild2DMipmaps(GL_TEXTURE_2D, int_fmt, xf->textureWidth,
                 xf->textureHeight, type, GL_UNSIGNED_BYTE,
                 (GLubyte *)xf->texture);

    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
}

static void
startTexture(xfieldP xf)
{
    static struct 
      { textureFilterE dx; GLint gl; }
    filter_to_gl[] = { 
      { tf_nearest               , GL_NEAREST },
      { tf_linear                , GL_LINEAR  },
      { tf_nearest_mipmap_nearest, GL_NEAREST_MIPMAP_NEAREST },
      { tf_nearest_mipmap_linear , GL_NEAREST_MIPMAP_LINEAR  },
      { tf_linear_mipmap_nearest , GL_LINEAR_MIPMAP_NEAREST  },
      { tf_linear_mipmap_linear  , GL_LINEAR_MIPMAP_LINEAR   }
    };
    static struct 
      { textureFunctionE dx; GLint gl; }
    function_to_gl[] = { 
      { tfn_decal   , GL_DECAL    }, { tfn_replace , GL_REPLACE  },
      { tfn_modulate, GL_MODULATE }, { tfn_blend   , GL_BLEND    }
    };
    static struct 
      { textureWrapE dx; GLint gl; }
    wrap_to_gl[] = { 
      { tw_clamp , GL_CLAMP   }, { tw_clamp_to_edge  , GL_CLAMP_TO_EDGE   },
      { tw_repeat, GL_REPEAT  }, { tw_clamp_to_border, GL_CLAMP_TO_BORDER }
    };

    attributeP attr = &xf->attributes;
    int i, len;
    GLint wrap_s, wrap_t, min_filter, mag_filter, function;

    /*  Set texture wrap modes  */
    wrap_s = wrap_t = GL_CLAMP;
    len = sizeof(wrap_to_gl)/sizeof(*wrap_to_gl);
    if ( !glSupports_CLAMP_TO_BORDER() )
        len--;
    for ( i = 0; i < len; i++ ) {
      if ( attr->texture_wrap_s == wrap_to_gl[i].dx )
        wrap_s = wrap_to_gl[i].gl;
      if ( attr->texture_wrap_t == wrap_to_gl[i].dx )
        wrap_t = wrap_to_gl[i].gl;
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap_s);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap_t);

    /*  Set texture filters  */
    min_filter = GL_NEAREST, mag_filter = GL_NEAREST;
    for ( i = 0; i < sizeof(filter_to_gl)/sizeof(*filter_to_gl); i++ ) {
      if ( attr->texture_min_filter == filter_to_gl[i].dx )
        min_filter = filter_to_gl[i].gl;
      if ( attr->texture_mag_filter == filter_to_gl[i].dx )
        mag_filter = filter_to_gl[i].gl;
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag_filter);

    /*  Set texture function  */
    function = GL_MODULATE;
    for ( i = 0; i < sizeof(function_to_gl)/sizeof(*function_to_gl); i++ )
      if ( attr->texture_function == function_to_gl[i].dx )
        function = function_to_gl[i].gl;
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, function);

    /*  And activate texuring  */
    glEnable(GL_TEXTURE_2D);
}

static void
endTexture()
{  
    glDisable(GL_TEXTURE_2D);
}
    
#else

#include <stdio.h>

#define NONE		0
#define PER_FIELD	1
#define PER_POSITION	2
#define PER_CONNECTION	3

int cDeps[]     = {NONE, PER_FIELD, PER_POSITION, PER_CONNECTION};
int ncDeps      = 4;

int nDeps[]     = {NONE, PER_FIELD, PER_POSITION, PER_CONNECTION};
int nnDeps      = 4;

int oDeps[]     = {NONE, PER_FIELD, PER_POSITION, PER_CONNECTION};
int noDeps      = 4;

int getIndex(int c, int n, int o)
{
    return ((c*nnDeps)+n)*noDeps + o;
}

char *strings[] =
{
    "none",
    "pf",
    "pp",
    "pc"
};

int printGetIndex()
{
  printf("static int\n");
  printf("depToInt(dependencyT d)\n");
  printf("{\n");
  printf("  switch(d) {\n");
  printf("    case dep_none:        return %d;\n", NONE);
  printf("    case dep_field:       return %d;\n", PER_FIELD);
  printf("    case dep_positions:   return %d;\n", PER_POSITION);
  printf("    case dep_connections: return %d;\n", PER_CONNECTION);
  printf("    default:              return -1;\n");
  printf("  }\n");
  printf("}\n\n");
  printf("static int\n");
  printf("getIndex(dependencyT c, dependencyT n, dependencyT o)\n");
  printf("{\n");
  printf("  int ci = depToInt(c);\n");
  printf("  int ni = depToInt(n);\n");
  printf("  int oi = depToInt(o);\n");
  printf("  if (ci == -1 || ni == -1 || oi == -1)\n");
  printf("    return -1;\n");
  printf("  else return ((ci*%d)+ni)*%d+oi;\n", nnDeps, noDeps);
  printf("}\n\n");

  return;
}

printLabel(int cd, int nd, int od)
{
  printf("C%sN%sO%s", strings[cd], strings[nd], strings[od]);
}

printTable()
{
  int c, n, o;

  printf("static helperFunc helperTable[] = {\n");
  for (c = 0; c < ncDeps; c++)
    for (n = 0; n < nnDeps; n++)
      for (o = 0; o < noDeps; o++)
      {
	printf("    ");
	printLabel(cDeps[c], nDeps[n], oDeps[o]);
	printf("%c\n",
	      (c == ncDeps-1)&&(n == nnDeps-1)&&(o == noDeps-1) ? ' ' : ',');
      }
  printf("};\n\n");
}

printCase(int cd, int nd, int od)
{
  printf("static void\n");
  printLabel(cd, nd, od);
  printf("(xfieldP xf, int ppc, int ci, int *trans)\n{\n");
  printf("  static float  fscratch[3];\n");
  printf("  static int    iscratch;\n");
  printf("  static int    p;\n");

  if (od == PER_CONNECTION)
      printf("  GETOPACITY(ci);\n");
  
  if (cd == PER_CONNECTION)
  {
    printf("  GETCOLOR(ci);\n");
    if (nd != NONE)
      printf("  APPLY_LIGHTING;\n");

    if (od == NONE)
      printf("  SENDCOLOR;\n");
    else if (od == PER_FIELD || od == PER_CONNECTION)
      printf("  SENDCOLOROPACITY\n");
  }
  
  if (nd == PER_CONNECTION)
    printf("  SENDNORMAL(ci);\n");
  
  printf("\n  for (p = 0; p < ppc; p++) {\n");

  if (od == PER_POSITION)
    printf("    GETOPACITY(trans[p]);\n");
    
  if (cd == PER_POSITION)
  {
    printf("    GETCOLOR(trans[p]);\n");
    if (nd != NONE)
      printf("    APPLY_LIGHTING;\n");
  }
  
  if (cd == PER_POSITION)
  {
    if (od != NONE)
      printf("    SENDCOLOROPACITY;\n");
    else
      printf("    SENDCOLOR;\n");
  }
  else if (od == PER_POSITION)
    printf("    SENDCOLOROPACITY;\n");
  
  if (nd == PER_POSITION)
    printf("    SENDNORMAL(trans[p]);\n");

  printf("    SENDPOSITION(trans[p]);\n");

  printf("  }\n}\n\n");
}

main()
{
    int c, n, o;

    printGetIndex();

    for (c = 0; c < ncDeps; c++)
	for (n = 0; n < nnDeps; n++)
	    for (o = 0; o < noDeps; o++)
	        printCase(cDeps[c], nDeps[n], oDeps[o]);

    printTable();

    exit(0);
}
		
		
#endif
