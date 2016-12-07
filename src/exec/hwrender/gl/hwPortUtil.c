/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


/*---------------------------------------------------------------------------*\
 $Source: /src/master/dx/src/exec/hwrender/gl/hwPortUtil.c,v $

\*---------------------------------------------------------------------------*/

#ifndef HELPERCODE

#include <math.h>

#define X_H
#define Cursor glCursor
#define String glString
#define Boolean glBoolean
#define Object glObject
#include <gl/gl.h>
#undef Object
#undef Boolean
#undef String
#undef Cursor
#undef X_H

#include "hwDeclarations.h"
#include "hwPortLayer.h"
#include "hwXfield.h"
#include "hwWindow.h"
#include "hwPortGL.h"
#include "hwTmesh.h"
#include "hwSort.h"

#define String dxString
#define Object dxObject
#define Angle dxAngle
#define Matrix dxMatrix
#define Screen dxScreen
#define Boolean dxBoolean
#include "internals.h"
#undef String
#undef Object
#undef Angle
#undef Matrix
#undef Screen
#undef Boolean

#define TIMER(s) finish(); DXMarkTime(s);
#include "hwDebug.h"

#if 0
#define DXGetArrayEntry(x,y,z) (&zeroFloat)
static float zeroFloat[] = {0.57, 0.57, 0.57};
#endif

#ifdef DOPV
#include "pv/pv.h"
#else
#define PHASE_START(x)	{}
#define PHASE_END(y)	{}
#endif

#define NOTUSED	0

typedef void (*helperFunc) (xfieldP xf, int posPerConn, 
			    int ctIndex, int *translation, float *rgba);

static helperFunc
  getHelper(dependencyT colorsDep, 
	    dependencyT normalsDep, 
	    dependencyT opacitiesDep,
	    int texture);

static Error
  points (xfieldP xf, helperFunc helper,
	  int firstConnection, int lastConnection, int face,
	  enum approxE approx, int skip);

static Error
  bbox (xfieldP xf);

static void
_deleteGLObject(xfieldP xf);
#define RENDER_OPAQUE 1
#define RENDER_TRANSPARENT 2

#define	SENDPOSITION(offset) 						\
{ 								       	\
  float *position; 							\
  position = (float*)DXGetArrayEntry(xf->positions,offset,fscratch); 	\
  if(xf->shape < 3)							\
    v2f(position);	/* gl default z = 0.0 if 2d data */ 		\
  else									\
    v3f(position);							\
}

#ifdef sgi

#define SENDUV(offset)                                                  \
{                                                                       \
  float *uv ;                                                   	\
  uv = (float *)DXGetArrayEntry(xf->uv,offset,uvscratch) ;      	\
  t2f(uv) ;                                                   		\
}

#endif


#define	OPACITY(offset)								\
  ((xf->nomap) ?								\
	  *(float *)DXGetArrayEntry(xf->omap,					\
				    *(char *)DXGetArrayEntry(xf->opacities,	\
								offset,		\
								&iscratch),	\
				    fscratch)					\
      :										\
	  *(float *)DXGetArrayEntry(xf->opacities, offset, fscratch))


#define SENDOPACITY(offset) 					\
  if((xf->opacitiesDep != dep_none && GETOPACITY(0) < .75) ||	\
       xf->attributes.flags & CONTAINS_VOLUME) 			\
      setpattern(PATTERN_INDEX)
     

#define	SENDNORMAL(offset) \
{ \
  Vector normal; \
  normal = *(Vector*)DXGetArrayEntry(xf->normals,offset,fscratch); \
  n3f((float*)&normal); \
}

#define SENDSPECCOLOR()  				\
{							\
  RGBColor rgbspec; 					\
  rgbspec.r = rgbspec.g = rgbspec.b = 			\
    xf->attributes.front.specular;			\
  lmcolor (LMC_SPECULAR); 				\
  c3f((float *)&rgbspec); 				\
}

#define GETCOLOR(offset)							\
{										\
  Vector  rgbvec;								\
										\
  if(xf->ncmap) 							      	\
     rgbvec = *(Vector*)DXGetArrayEntry(xf->cmap, 			      	\
					*(ubyte *)DXGetArrayEntry(xf->fcolors, 	\
							       offset, 	      	\
							       &iscratch),    	\
					fscratch);			      	\
  else									      	\
     rgbvec = *(Vector*)DXGetArrayEntry(xf->fcolors,offset,fscratch);        	\
										\
  rgba[0] = rgbvec.x;								\
  rgba[1] = rgbvec.y;								\
  rgba[2] = rgbvec.z;								\
}

#define GETOPACITY(offset)							\
{										\
  rgba[3] = OPACITY(offset);							\
}

#define	SENDLITCOLOR(transp)  						      	\
{ 									      	\
  lmcolor(LMC_AD); 							      	\
  if (transp)									\
      c4f(rgba);								\
  else 										\
      c3f(rgba);								\
}

#define	SENDCOLOR(transp)  						      	\
{ 									      	\
  lmcolor(LMC_COLOR); 							\
  if (transp)									\
      c4f(rgba);								\
  else 										\
      c3f(rgba);								\
}

#define PERFIELD(lit, transp)							\
{ 										\
  int _lit = lit && xf->normalsDep != dep_none;					\
  int sendc = (xf->colorsDep == dep_field &&					\
		(!transp || xf->opacitiesDep == dep_field));			\
										\
  if (xf->colorsDep == dep_field)						\
      GETCOLOR(0);								\
										\
  if (transp && xf->opacitiesDep == dep_field)					\
      GETOPACITY(0);								\
										\
  if(_lit) {									\
      SENDSPECCOLOR(); 								\
      lmcolor(LMC_AD); 								\
      if(xf->normalsDep == dep_field) 						\
        SENDNORMAL(0); 								\
      if (sendc)								\
	  SENDLITCOLOR(transp);							\
  } else {									\
      lmcolor(LMC_COLOR); 							\
      if (sendc)								\
	  SENDCOLOR(transp);							\
  }										\
										\
  if (doesPattern)								\
      if ((xf->opacitiesDep != dep_none) && (OPACITY(0) < 0.75))		\
	  setpattern(PATTERN_INDEX);						\
}


#define ENDPERFIELD() 								\
    if (doesPattern) setpattern(0);


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

static int doesTrans, doesPattern;

static Error
dots (xfieldP xf, enum approxE approx, int skip)
{
  static float	fscratch[3];
  static int	iscratch;
  int		p,i;
  helperFunc	helper;
  float 	rgba[4];

  PERFIELD(0, 0);

  for(p=0;p<xf->npositions;) {
    bgnpoint();
    for(i=0;p<xf->npositions && i<256;) {
      if(! xf->invPositions || ! DXIsElementInvalidSequential(xf->invPositions,
							      p)) {
        SENDCOLOR(p);

	SENDPOSITION(p);
	i++;
	p+= skip;
      } else {
	p++;
      }
    }
    endpoint();
  }

  EXIT(("OK"));
  return OK;
}

static Error
translucentPoints (xfieldP xf, helperFunc helper,
	SortD list, int n, int face,
	enum approxE approx, int skip)
{
  static int	translation[8];
  static float	fscratch[3];
  static int	iscratch;
  int		i, c, c1;
  dependencyT   odep = xf->opacitiesDep;
  dependencyT   cdep = xf->colorsDep;
  float		opacity;
  float 	rgba[4];

  PERFIELD(0, 1);

  if (!n)
      return OK;
      
  bgnpoint();
  for (c = 0; c < n; c ++)
  {
    if (xf->connections)
        c1 = *(int*)DXGetArrayEntry(xf->connections,
			list[c].poly,(Pointer)iscratch);
    else
        c1 = list[c].poly;
    
    if (odep == dep_connections)
	GETOPACITY(list[c].poly);
    
    if (cdep == dep_connections)
	GETCOLOR(list[c].poly);

    (*helper)(xf,1,list[c].poly,&c1,rgba);
  }
  endpoint();

  ENDPERFIELD();

  return OK;
}

static Error
points (xfieldP xf, helperFunc helper,
	int firstConnection, int lastConnection, int face,
	enum approxE approx, int skip)
{
  static int	translation[8];
  static float	fscratch[3];
  static int	iscratch;
  int		c, i=0, translu = 0;
  dependencyT   odep = xf->opacitiesDep;
  dependencyT   cdep = xf->colorsDep;
  float		opacity;
  float 	rgba[4];

  ENTRY(("points(0x%x, 0x%x, %d, %d, %d, %d, %d)",
	xf, helper, firstConnection, lastConnection, face, approx, skip));

  PERFIELD(0, 0);

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
		    break;

		if (odep == dep_connections)
		    translu |= OPACITY(c) < 1.0f;
		else if (odep == dep_positions)
		    translu |= OPACITY(c1) < 1.0f;

		if (! translu)
		{
		    if (i++ == 0)
		        bgnpoint();
		    
		    if (cdep == dep_connections)
			GETCOLOR(c1);

		    (*helper)(xf,1,c,&c1,rgba);
		}
	    }
	  }

	  if (i)
	    endpoint();
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
		c1 = *(int*)DXGetArrayEntry(xf->connections,
					c, (Pointer)iscratch);
	    else
		c1 = c;

	    if(xf->invPositions && DXIsElementInvalid(xf->invPositions, c1))
		break;

	    if (i++ == 0)
		bgnpoint();
	    
	    if (cdep == dep_connections)
		GETCOLOR(c1);

	    (*helper)(xf,xf->posPerConn,c,translation,rgba);
	}
      }

      if (i)
	    endpoint();
  }

  ENDPERFIELD();

  EXIT(("OK"));
  return OK;

error:
  return ERROR;
}


static int noLines = 0;

static Error translucentLines (xfieldP xf, helperFunc helper,
                                 SortD list, int n, int face,
				enum approxE approx, int skip)
{
  static int	translation[8];
  static float	fscratch[3];
  static int	iscratch;
  int		c;
  dependencyT   odep = xf->opacitiesDep;
  dependencyT   cdep = xf->colorsDep;
  float		opacity;
  float 	rgba[4];

  if (noLines)
      return OK;

  PERFIELD(0, 1);

  for(c = 0; c < n; c++)
  {
    if(!xf->invCntns || ! DXIsElementInvalid(xf->invCntns,c))
    {
	bgnline();
	GET_CONNECTION(list[c].poly);

	if (odep == dep_connections)
	    GETOPACITY(list[c].poly);

	if (cdep == dep_connections)
	    GETCOLOR(list[c].poly);

	(*helper)(xf,xf->posPerConn,list[c].poly,translation,rgba);
	endline();
    }
  }

  ENDPERFIELD();

  return OK;
}

static Error
lines (xfieldP xf, helperFunc helper,
       int firstConnection, int lastConnection,int face,
       enum approxE approx, int skip)
{
  static int	translation[8];
  static float	fscratch[3];
  static int	iscratch;
  int		c, translu = 0;
  int		doliting = _dxf_xfieldAttributes(xf)->shade ? 1 : 0;
  dependencyT   odep = xf->opacitiesDep;
  dependencyT   cdep = xf->colorsDep;
  float		opacity;
  float 	rgba[4];

  if (noLines)
      return OK;

  ENTRY(("lines(0x%x, 0x%x, %d, %d, %d, %d, %d)",
	 xf, helper, firstConnection, lastConnection, face, approx, skip));

  if (approx == approx_box) {
    EXIT(("calling bbox"));
    return bbox(xf);
  }

  PERFIELD(doliting, 0);

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
	    if (cdep == dep_connections)
		GETCOLOR(c);

	    bgnline();
	    (*helper)(xf,xf->posPerConn,c,translation,rgba);
            endline();
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
          bgnline();
	  GET_CONNECTION(c);
	  (*helper)(xf,xf->posPerConn,c,translation,rgba);
          endline();
	}
      }
  }
  for(c=firstConnection;c<lastConnection;c+=skip) {
    if(!xf->invCntns || ! DXIsElementInvalid(xf->invCntns,c)) {
      GET_CONNECTION(c);
      (*helper)(xf,xf->posPerConn,c,translation,rgba);
    }
  }

  ENDPERFIELD();

  EXIT((""));
  return OK;

error:  
  return ERROR;
}

#define MAXGLPLINEVERT 100

static Error polylines (xfieldP xf, helperFunc helper,
		    enum approxE approx, int skip)
{
    static float fscratch[3];
    static int	 iscratch;
    int		 c, i;
    int	         start, end, knt, max, thisknt;
    int		 doliting = _dxf_xfieldAttributes(xf)->shade ? 1 : 0;
    dependencyT  cdep = xf->colorsDep;
    float	 opacity;
    float	 rgba[4];

    if (approx == approx_box)
    {
	EXIT(("calling bbox"));
	return bbox(xf);
    }

    PERFIELD(doliting, 0);

    if (skip > 1)
    {
	for(c = 0; c < xf->npolylines; c ++)
	{
	    if(!xf->invCntns || ! DXIsElementInvalid(xf->invCntns,c))
	    {
		start = xf->polylines[c];
		end   = (c == xf->npolylines-1) ? xf->nedges :
						    xf->polylines[c+1];
		knt = (end - start) - 1;

		if (cdep == dep_polylines)
		    GETCOLOR(c);

		bgnline();
		while (knt > 0)
		{
		    thisknt = (knt > max) ? max : knt;
			
		    (*helper)(xf, 2, c, xf->edges+start,rgba);

		    start += skip;
		    knt -= skip;
		}
		endline();

	    }
	}
    }
    else
    {

	max = MAXGLPLINEVERT;

	for(c = 0; c < xf->npolylines; c ++)
	{
	    if(!xf->invCntns || ! DXIsElementInvalid(xf->invCntns,c))
	    {
		start = xf->polylines[c];
		end   = (c == xf->npolylines-1) ? xf->nedges :
						    xf->polylines[c+1];
		knt = end - start;

		if (cdep == dep_polylines)
		    GETCOLOR(c);

		bgnline();
		while (knt)
		{
		    thisknt = (knt > max) ? max : knt;

		    (*helper)(xf, thisknt, c, xf->edges+start,rgba);

		    start += (thisknt-1);
		    knt -= (thisknt-1);
		    if(thisknt == 1)
		       knt = 0;
		}
		endline();
	    }
	}
    }

    ENDPERFIELD();

    EXIT((""));
    return OK;
}

static Error
closedlines (xfieldP xf, helperFunc helper,
       int firstConnection, int lastConnection,int face,
       enum approxE approx, int skip)
{
  static int	translation[8];
  static float	fscratch[3];
  static int	iscratch;
  int		c;
  dependencyT   cdep = xf->colorsDep;
  float		opacity;
  float 	rgba[4];

  ENTRY(("closedlines(0x%x, 0x%x, %d, %d, %d, %d, %d)",
	 xf, helper, firstConnection, lastConnection, face, approx, skip));

  if (approx == approx_box) {
    EXIT(("calling bbox"));
    return bbox(xf);
  }

  PERFIELD(0, 0);

#ifdef sgi

  if(xf->texture)    /*  (:Q=   */
  {
     static float Holder[3] = { 0.5f, 0.7f, 1.0f };
     c3f(Holder);
  }

#endif

  for(c=firstConnection;c<lastConnection;c+=skip) {
    if(!xf->invCntns || ! DXIsElementInvalid(xf->invCntns,c)) {

      GET_CONNECTION(c);

      bgnclosedline();

      if (cdep == dep_connections)
	  GETCOLOR(c);

      (*helper)(xf,xf->posPerConn,c,translation,rgba);

      endclosedline();
    }
  }

  ENDPERFIELD();

  EXIT(("OK"));
  return OK;
}


static Error
polygons (xfieldP xf, helperFunc helper,
	  int firstConnection, int lastConnection, int face,
	  enum approxE approx, int skip)
{
  static int	translation[8];
  static float	fscratch[3];
  static int	iscratch;
  int		c;
  int		translu = 0;
  dependencyT   odep = xf->opacitiesDep;
  dependencyT   cdep = xf->colorsDep;
  float		opacity;
  float 	rgba[4];

  ENTRY(("polygons(0x%x, 0x%x, %d, %d, %d, %d, %d)",
	 xf, helper, firstConnection, lastConnection, face, approx, skip));

  switch(approx) {
  case approx_dots:
    EXIT(("calling dots"));
    return dots(xf,approx,skip);
    break;
  case approx_box:
    EXIT(("calling bbox"));
    return bbox(xf);
    break;
  case approx_lines:
    helper = getHelper(xf->colorsDep,dep_none,dep_none,NULL);
    EXIT(("calling closedlines"));
    return closedlines(xf,helper,0,xf->nconnections,face,approx,skip);
    break;
  }


  if (doesTrans)
  {
     switch(odep)
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
	   {
		translu = OPACITY(c) < 1.0f;
	   }
	   else if (odep == dep_positions)
	   {
	      int i;
	      for (i = 0; !translu && i < xf->posPerConn; i++)
		  if (OPACITY(translation[i]) < 1.0f)
		      translu = 1;
	   }

	   if (! translu)
	   {
	      bgnpolygon();

	      PERFIELD(1, 0);

	      if (cdep == dep_connections)
		  GETCOLOR(c);

	      (*helper)(xf,xf->posPerConn,c,translation,rgba);
	      endpolygon();
	   }
	}
     }
  }
  else
  {
     PERFIELD(1, 0);

     for(c=firstConnection;c<lastConnection;c+=skip)
     {
       if(!xf->invCntns || ! DXIsElementInvalid(xf->invCntns,c))
       {
	   GET_CONNECTION(c);

	   if (cdep == dep_connections)
	      GETCOLOR(c);

	   bgnpolygon();
	   (*helper)(xf,xf->posPerConn,c,translation,rgba);
	   endpolygon();
       }
     }
  }

  ENDPERFIELD();

  EXIT(("OK"));
  return OK;

error:
  return ERROR;
}

static Error
translucentPolygons (xfieldP xf, helperFunc helper,
	  SortD list, int n, int face,
	  enum approxE approx, int skip)
{
  static int	translation[8];
  static float	fscratch[3];
  static int	iscratch;
  int		c;
  int		translu = 0;
  dependencyT   odep = xf->opacitiesDep;
  dependencyT   cdep = xf->colorsDep;
  float		rgba[4];
  float		opacity;

  PERFIELD(1, 1);

  for(c = 0; c < n; c ++)
  {
       if(!xf->invCntns || ! DXIsElementInvalid(xf->invCntns,c))
       {
	   GET_CONNECTION(list[c].poly);
	   bgnpolygon();
	   (*helper)(xf,xf->posPerConn,list[c].poly,translation,rgba);
	   endpolygon();
       }
  }

  ENDPERFIELD();

  EXIT(("OK"));
  return OK;

error:
  return ERROR;
}

#define NOTUSED	0

static Error
tmeshlines (xfieldP xf, helperFunc helper, enum approxE approx, int skip);

static Error
tmesh (xfieldP xf, helperFunc helper, enum approxE approx, int skip)
{
  static int	*translation,*strip;
  static float	fscratch[3];
  static int	iscratch;
  int		count;
  int		meshI,i;
  int		skipDelta;
  InvalidComponentHandle tmpic,tmpip;
  struct  {
    int start;
    int count;
  }  *meshes;
  dependencyT   odep = xf->opacitiesDep;
  dependencyT   cdep = xf->colorsDep;
  float		opacity;
  float 	rgba[4];
 
  ENTRY(("tmesh(0x%x, 0x%x, %d, %d)", xf, helper, approx, skip));

  tmpic = xf->invCntns;
  tmpip = xf->invPositions;
  xf->invCntns = NULL;
  xf->invPositions = NULL;

  switch(approx) {
  case approx_dots:
    if (tmpic || tmpip) {
      helper = getHelper(xf->colorsDep,dep_none,dep_none,NULL);
      points (xf, helper,0,xf->nconnections,0,approx,skip);
      goto done;
    } else {
      dots(xf,approx,skip);
      goto done;
    }
    break;
  case approx_box:
    bbox(xf);
    goto done;
    break;
  case approx_lines:
    helper = getHelper(xf->colorsDep,dep_none,dep_none,NULL);
    tmeshlines(xf, helper, approx, skip);
    goto done;
    break;
  }

  PERFIELD(1, 0);

#ifdef sgi

  if(xf->texture)
     glStartTexture(xf);

#endif

  /* NOTE:
   * The code below is optimized for tmesh. It depends on two 
   * qualities of our tmesh generator.
   * 1) tmeshes are NOT generated for Npc or Cpc
   * 2) tmeshes create a regular array in xf->connections_array;
   */
  translation = DXGetArrayData(xf->connections_array);
  if(!(meshes = DXGetArrayData(xf->meshes))) goto error;
  /* Not Skipping */
  if(skip <= 1) {
    for(meshI=0;meshI<xf->nmeshes;meshI++) {


      bgntmesh();

      PERFIELD(1, 0);

#if defined(DXD_HW_TMESH_SWAP_OK)
      {
	int remainder = meshes[meshI].count;
	int *startPtr,*endPtr;
	
	startPtr = endPtr = &translation[meshes[meshI].start];
	{
	  for(i=0;i<remainder;i++) {
	    if (*(endPtr++) < 0) {
	      (*helper)(xf,i,NOTUSED,startPtr,rgba);
	      swaptmesh();
	      remainder -= (i + 1);
	      i = -1;	/* allow for loop increment */
	      startPtr = endPtr;
	    }
	  }
	}  while (i<remainder);
	if(remainder) (*helper)(xf,remainder,NOTUSED,startPtr,rgba);
      }
#else
      (*helper)(xf,meshes[meshI].count,NOTUSED,
		&translation[meshes[meshI].start],rgba);
#endif
      endtmesh();
    }
  } else {  /* skipping */

    PERFIELD(1, 0);

    skipDelta = 0;
    for(meshI=0;meshI<xf->nmeshes;meshI++) {

      strip = &translation[meshes[meshI].start];

      for(count=skipDelta;
	  count < meshes[meshI].count-2;
	  count += skipDelta) {
	bgnpolygon();

#if defined(DXD_HW_TMESH_SWAP_OK)
	{
	  int	tri[3];
	  int	i,j;

	  for (i=0,j=0;i<3;i++,j++) {
	    /* if we have a swap in the triangle */
	    if (strip[count+i] < 0)
	      {
	      if(i==1)	/* swap in middle of tri requires special treatment */
		tri[0] = strip[count-1];
	      j++;
	      }
	    tri[i] = strip[count+j];
	  }
	  (*helper)(xf,3,NOTUSED,tri,rgba);
	}
#else
	(*helper)(xf,3,NOTUSED,&strip[count],rgba);
#endif
	endpolygon();
	skipDelta = skip;
      }
      skipDelta = count - (meshes[meshI].count-2);
    }
  }

  ENDPERFIELD();

 done:

#ifdef sgi

  if(xf->texture)
     glEndTexture();

#endif

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

static Error
tmeshlines (xfieldP xf, helperFunc helper, enum approxE approx, int skip)
{
  static int	*translation,*strip;
  static int	edge[MaxTstripSize+1];
  static float	fscratch[3];
  static int	iscratch;
  int		count;
  int		meshI;
  int		edgeI;
  int		skipDelta;
  dependencyT	odep;
  float 	opacity;	
  struct  {
    int start;
    int count;
  }  *meshes;
  float		rgba[4];

  ENTRY(("tmeshlines(0x%x, 0x%x, %d, %d)", xf, helper, approx, skip));

  /* NOTE:
   * The code below is optimized for tmesh. It depends on two 
   * qualities of our tmesh generator.
   * 1) tmeshes are NOT generated for Npc or Cpc
   * 2) tmeshes create a regular array in xf->connections_array;
   */
  translation = DXGetArrayData(xf->connections_array);

  PERFIELD(0, 0);

#ifdef sgi

  if(xf->texture)    /*  (:Q=   */
  { 
     static float Holder[3] = { 0.5f, 0.7f, 1.0f };
     c3f(Holder);
  }

#endif

  if(!(meshes = DXGetArrayData(xf->meshes))) goto error;

  if(skip <= 1) {	/* NOT skipping */
    for(meshI=0;meshI<xf->nmeshes;meshI++) {

      strip = &translation[meshes[meshI].start];

      /* Draw zigzap */
#if defined(DXD_HW_TMESH_SWAP_OK)
      for(edgeI=0,count=0;
	   count < meshes[meshI].count;
	    count++, edgeI++) 
	{
	  if(strip[count] < 0)	/* for swap */
	    edge[edgeI] = edge[edgeI-2];
	  else
	    edge[edgeI] = strip[count];
	}
      if(edgeI > 1) {
	bgnline();
	(*helper)(xf,edgeI,NOTUSED,edge,rgba);
	endline();
      }
#else
      bgnline();
      (*helper)(xf, meshes[meshI].count, NOTUSED, strip,rgba);
      endline();
#endif
      /* close boundary of zigzags with even indices */
      for(edgeI=0,count=0;
	   count < meshes[meshI].count;
	    count+=2) 
	{
	  if(strip[count] < 0) continue;	/* for swap */
	  edge[edgeI++] = strip[count];
	}
      if(edgeI > 1){
	bgnline();
	(*helper)(xf,edgeI,NOTUSED,edge,rgba);
	endline();
      }

      /* close boundary of zigzags with odd indices */
      for(edgeI=0,count=1;
	   count < meshes[meshI].count;
	    count+=2) 
	{
	  if(strip[count] < 0) continue;	/* for swap */
	  edge[edgeI++] = strip[count];
	}
      if(edgeI > 1) {
	bgnline();
	(*helper)(xf,edgeI,NOTUSED,edge,rgba);
	endline();
      }
    }
  } else { /* skipping */
    skipDelta = 0;
    for(meshI=0;meshI<xf->nmeshes;meshI++) {

      strip = &translation[meshes[meshI].start];

      for(count=skipDelta;
	  count < meshes[meshI].count-2;
	  count += skipDelta) {
	bgnclosedline();
	
#if defined(DXD_HW_TMESH_SWAP_OK)
	{
	  int	tri[3];
	  int	i,j;

	  for (i=0,j=0;i<3;i++,j++) {
	    /* if we have a swap in the triangle */
	    if (strip[count+i] < 0)
	      {
		if(i==1)/* swap in middle of tri requires special treatment */
		  tri[0] = strip[count-1];
		j++;
	      }
	    tri[i] = strip[count+j];
	  }
	  (*helper)(xf,3,NOTUSED,tri,rgba);
	}
#else
	(*helper)(xf,3,NOTUSED,&strip[count],rgba);
#endif
	endclosedline();
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


static Error
plines (xfieldP xf, helperFunc helper, enum approxE approx, int skip)
{
  static int	*translation,*strip;
  static int	edge[MaxTstripSize+1];
  static float	fscratch[3];
  static int	iscratch;
  int		count;
  int		start;
  int		step;
  int		meshI;
  int		edgeI;
  int		skipDelta;
  int		doliting = _dxf_xfieldAttributes(xf)->shade ? 1 : 0;
  dependencyT	odep;
  float 	opacity;	
  struct  {
    int start;
    int count;
  }  *meshes;
  float		rgba[4];

  ENTRY(("plines(0x%x, 0x%x, %d, %d)", xf, helper, approx, skip));
  if (approx == approx_box) {
    EXIT(("calling bbox"));
    return bbox(xf);
  }
  translation = DXGetArrayData(xf->connections_array);

  PERFIELD(doliting, 0);

  if(!(meshes = DXGetArrayData(xf->meshes))) goto error;

  if(skip <= 1) {	/* NOT skipping */
    for(meshI=0;meshI<xf->nmeshes;meshI++) {
#if 0
      count = meshes[meshI].count;
      start = meshes[meshI].start;
      for(step = 0; step < count; step += MAXGLPLINEVERT) {
	bgnline();
	(*helper)(xf, 
		  step + MAXGLPLINEVERT < count ?
		     MAXGLPLINEVERT :
		     count - step, 
		  NOTUSED, &translation[step+start],rgba);
	endline();
      }
#else
	bgnline();
	(*helper)(xf, meshes[meshI].count, NOTUSED, 
		  &translation[meshes[meshI].start],rgba);
	endline();
#endif
    }
  } else { /* skipping */
    skipDelta = 0;
    for(meshI=0;meshI<xf->nmeshes;meshI++) {

      strip = &translation[meshes[meshI].start];

      for(count=skipDelta;
	  count < meshes[meshI].count-1;
	  count += skipDelta) {
	bgnline();
	(*helper)(xf,2,NOTUSED,&strip[count],rgba);
	skipDelta = skip;
	endline();
      }
      skipDelta = count - (meshes[meshI].count-1);
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


static Error
bbox (xfieldP xf)
{
  static float	fscratch[3];
  static int	iscratch;
  int		p,i;
  static float	yellow[] = {1.0, 1.0, 0.0};
  dependencyT	odep;
  float 	opacity;	
  float		rgba[4];

  ENTRY(("bbox(0x%x)", xf));

  PERFIELD(0, 0);

  lmcolor(LMC_COLOR); 
  c3f((float*)&yellow[0]);
  bgnline();
  v3f((float*)&xf->box[0]);
  v3f((float*)&xf->box[1]);
  v3f((float*)&xf->box[3]);
  v3f((float*)&xf->box[2]);
  v3f((float*)&xf->box[0]);
  v3f((float*)&xf->box[4]);
  v3f((float*)&xf->box[5]);
  v3f((float*)&xf->box[7]);
  v3f((float*)&xf->box[6]);
  v3f((float*)&xf->box[4]);
  endline();
  bgnline();
  v3f((float*)&xf->box[2]);
  v3f((float*)&xf->box[6]);
  endline();
  bgnline();
  v3f((float*)&xf->box[3]);
  v3f((float*)&xf->box[7]);
  endline();
  bgnline();
  v3f((float*)&xf->box[1]);
  v3f((float*)&xf->box[5]);
  endline();

  EXIT(("OK"));
  return OK;
}

#ifdef sgi

static void
TNnone (xfieldP xf, int posPerConn, int ctIndex, int *translation)
{
  static  float fscratch[3];
  static  float uvscratch[2];
  static  int   iscratch;
  static  int   p;
  char c = 0xff;
  RGBcolor(c,c,c);
  for(p=0;p<posPerConn;p++)  {
    SENDUV(translation[p]);
    SENDPOSITION(translation[p]);
  }

  EXIT((""));
}


static void
TNpositions (xfieldP xf, int posPerConn, int ctIndex, int *translation)
{
  static float  fscratch[3];
  static float  uvscratch[2];
  static int    iscratch;
  static int    p;
  char c = 0xff;

  ENTRY(("TNpositions(0x%x, %d, %d, 0x%x)",
         xf, posPerConn, ctIndex, translation));

  RGBcolor(c,c,c);
  for(p=0;p<posPerConn;p++)  {
    int q = translation[p];
    SENDUV(q);
    SENDNORMAL(q);
    SENDPOSITION(q);
  }

  EXIT((""));
}

#endif


/*
 * Helper functions:
 *     colors, normals and opacities may be 
 *     each may be per vertex, per element or per field or none.
 *     code is v, e, f, n
 *
 *		colors
 *	       /normals
 *	      //opacities
 *	     ///
 *   helper_xxx
 *
 * The following program generates the helper table and
 * the helper routines themselves.  If you need to recreate
 * it, cut out the code and run it, sending stdout to "helper.include"
 */

#include "helper.include"

#define DEP2INDEX(dep, index)			\
{						\
  switch(dep)					\
  {						\
      case dep_positions:   index = 0; break;	\
      case dep_connections: index = 1; break;	\
      case dep_field:       index = 2; break;	\
      case dep_none:        index = 3; break;	\
  }						\
}

static helperFunc
getHelper(dependencyT colorsDep, 
	  dependencyT normalsDep, 
	  dependencyT opacitiesDep,
	  int texture)
{
  int		index, c_index, n_index, o_index;
  helperFunc	ret;

  ENTRY(("getHelper(%d, %d, %d)", colorsDep, normalsDep, opacitiesDep));

#ifdef sgi

  if(texture)
  {
#if 0
     if (normalsDep == dep_connections)
	return TNconnections;
     else
#endif
     if(normalsDep == dep_positions)
	return TNpositions;
     else
	return TNnone;
  }

#endif

  DEP2INDEX(colorsDep,    c_index);
  DEP2INDEX(normalsDep,   n_index);
  DEP2INDEX(opacitiesDep, o_index);

  index = ((c_index*4) + n_index)*4 + o_index;

  ret = helperTable[index];

  EXIT(("ret = 0x%x", ret));
  return ret;
}

static void _getLightAttrs(tdmPortHandleP phP, dxObject f,
			   float *k_ambi, float *k_diff,
			   float *k_spec, float *k_shin);

static Error parameters(dxObject o, attributeP new, attributeP old);

static Error
_drawPrimitives(tdmPortHandleP portHandle, xfieldP xf, 
		  enum approxE approx, int density)
{
  helperFunc	helper;
  
  ENTRY(("_drawPrimitives(0x%x, 0x%x, %d, %d)",
	 portHandle, xf, approx, density));

  TIMER("> drawPrimitives");

  if(xf->invCntns)
    if(! DXInitGetNextInvalidElementIndex(xf->invCntns))
      goto error;

  finish(); /* Flush Geometry Pipeline */

  PHASE_START("Primitives");

  switch (xf->connectionType) {
  case ct_none:
    TIMER("> points");
    helper = getHelper(xf->colorsDep,dep_none,dep_none,NULL);
    points(xf,helper,0,xf->npositions,0,approx,density);
    TIMER("< points");
    break;
  case ct_lines:
    if(xf->attributes.linewidth > 0)
       linewidth((int)xf->attributes.linewidth);

    if(xf->attributes.aalines) 
        linesmooth(SML_ON);

    helper = getHelper(xf->colorsDep,dep_none,dep_none,NULL);
    TIMER("> lines");
    lines(xf,helper,0,xf->nconnections,0,approx,density);
    TIMER("< lines");
    linesmooth(SML_OFF);
    break;
  case ct_polylines:
    if(xf->attributes.linewidth > 0)
       linewidth((int)xf->attributes.linewidth);

    if(xf->attributes.aalines) 
        linesmooth(SML_ON);

    helper = getHelper(xf->colorsDep,dep_none,dep_none,NULL);
    TIMER("> plines");
    polylines(xf,helper,approx,density);
    TIMER("< plines");
    linesmooth(SML_OFF);
    break;
  case ct_pline:
    if(xf->attributes.linewidth > 0)
       linewidth((int)xf->attributes.linewidth);

    if(xf->attributes.aalines) 
        linesmooth(SML_ON);

    helper = getHelper(xf->colorsDep,dep_none,dep_none,NULL);
    TIMER("> plines");
    plines(xf,helper,approx,density);
    TIMER("< plines");
    linesmooth(SML_OFF);
    break;
  case ct_triangles: 
    helper = getHelper(xf->colorsDep,xf->normalsDep,dep_none,
		       (xf->texture != NULL) && (approx == approx_none));
    TIMER("> triangles");
    polygons(xf,helper,0,xf->nconnections,0,approx,density);
    TIMER("< triangles");
    break;
  case ct_quads:
    helper = getHelper(xf->colorsDep,xf->normalsDep,dep_none,
		       (xf->texture != NULL) && (approx == approx_none));
    TIMER("> quads");
    polygons(xf,helper,0,xf->nconnections,4,approx,density);
    TIMER("< quads");
    break;
/* 
 * XXX NOTE:
 * The volume code below is very inefficient for screen door rendering.
 * It draws a vast number of completely obscure faces. This is to make
 * sure that all the boundary faces are drawn. It would be far better
 * to do a show boundary and store the result of that in the xfield.
 * Next pass.
 */
  case ct_tetrahedra:
    helper = getHelper(xf->colorsDep,xf->normalsDep,dep_none,
		       (xf->texture != NULL) && (approx == approx_none));
    TIMER("> tetras");
    polygons(xf,helper,0,xf->nconnections,0,approx,density);
    polygons(xf,helper,0,xf->nconnections,1,approx,density);
    polygons(xf,helper,0,xf->nconnections,2,approx,density);
    polygons(xf,helper,0,xf->nconnections,3,approx,density);
    TIMER("< tetras");
    break;
  case ct_cubes:
    helper = getHelper(xf->colorsDep,xf->normalsDep,dep_none,
		       (xf->texture != NULL) && (approx == approx_none));
    TIMER("> cubes");
    polygons(xf,helper,0,xf->nconnections,4,approx,density);
    polygons(xf,helper,0,xf->nconnections,5,approx,density);
    polygons(xf,helper,0,xf->nconnections,6,approx,density);
    polygons(xf,helper,0,xf->nconnections,7,approx,density);
    polygons(xf,helper,0,xf->nconnections,8,approx,density);
    polygons(xf,helper,0,xf->nconnections,9,approx,density);
    TIMER("< cubes");
    break;
  case ct_tmesh:
    helper = getHelper(xf->colorsDep,xf->normalsDep,dep_none,
		       (xf->texture != NULL) && (approx == approx_none));
    TIMER("> tmesh");
    tmesh(xf,helper,approx,density);
    TIMER("< tmesh");
    break;
  case ct_qmesh:
    helper = getHelper(xf->colorsDep,xf->normalsDep,dep_none,
		       (xf->texture != NULL) && (approx == approx_none));
    TIMER("> qmesh");
    tmesh(xf,helper,approx,density);
    TIMER("< qmesh");
    break;
#if 0
#ifdef DXD_HW_TEXTURE_MAPPING
  case ct_flatGrid:
    if((approx == approx_none) &&
       (density == 1)) {
      TIMER("> texture");
      if(!hwTextureDraw(portHandle, xf))
	goto error;
      TIMER("< texture");
      break;
    } else {
      helper = getHelper(xf->colorsDep,xf->normalsDep,dep_none,
			 xf->texture != NULL);
      TIMER("> quads");
      polygons(xf,helper,0,xf->nconnections,4,approx,density);
      TIMER("< quads");
      break;
    }
	  
#endif
#endif
  default:
    DXErrorGoto (ERROR_INTERNAL, "invalid xfield connections type") ;
    break;
  }
  PHASE_END("Primitives");
  
 done:
  TIMER("< drawPrimitives");
  EXIT(("OK"));
  return OK;

 error:
  TIMER("< drawPrimitives");
  EXIT(("ERROR"));
  return ERROR;
}

Error
_dxf_DrawOpaque (tdmPortHandleP portHandle, xfieldP xf,	
		 RGBColor ambientColor, int buttonUp)
{
  int		density;
  enum approxE	approx;
  dxObject Obj;
  int      DisplayList;
  DEFPORT(portHandle);

  ENTRY(("_dxf_DrawOpaque(0x%x, 0x%x, 0x%x, %d)",
	 portHandle, xf, &ambientColor, buttonUp));
  
  TIMER("> DrawOpaque");
  
  PHASE_START("DrawOpaque");

  if (_dxf_isFlagsSet(_dxf_SERVICES_FLAGS(), SF_DOES_TRANS))
  {
      if (getgdesc(GD_BLEND))
      {
	  doesTrans   = 1;
	  doesPattern = 0;
      }
      else
      {
	  doesTrans   = 0;
	  doesPattern = 1;
      }
  }
  else
  {
      doesTrans = 0;
      doesPattern = 0;
  }

  if(xf->deletePrivate == NULL)
     xf->deletePrivate = _deleteGLObject;

  if(xf->normalsDep != dep_none &&
     (buttonUp &&xf->attributes.buttonUp.approx == approx_none) ||
     (!buttonUp &&xf->attributes.buttonDown.approx == approx_none)) {

    _dxf_SET_GLOBAL_LIGHT_ATTRIBUTES(PORT_CTX ,1);

    _dxf_SET_MATERIAL_SPECULAR (PORT_CTX, 1.0 /*R*/, 1.0 /*G*/, 1.0 /*B*/,
				(float) xf->attributes.front.shininess
				/* specular power */) ;
  }


  _dxf_PUSH_MULTIPLY_MATRIX(PORT_CTX,xf->attributes.mm);

  if(xf->colorsDep == dep_positions || xf->normalsDep == dep_positions)
    shademodel(GOURAUD);
  else
    shademodel(FLAT);

  blendfunction(BF_ONE, BF_ZERO);

  if(buttonUp) {
    density = xf->attributes.buttonUp.density;
    approx = xf->attributes.buttonUp.approx;
  } else {
    density = xf->attributes.buttonDown.density;
    approx = xf->attributes.buttonDown.approx;
  }
  /* 
   * indigos get better performance from globjects, so try to 
   * use one
   */
  PHASE_START("DrawPrimitives");

/*
#ifdef DXD_HW_API_DISPLAY_LIST_MEMORY

  if(Obj = DXGetIntegerAttribute((dxObject)(xf), "hwDisplayList", &DisplayList))
  {
     if(DisplayList)
     {
	printf("hwDisplayList Set.\n");fflush(stdout);
     }
     else
     {
	printf("hwDisplayList !Set.\n");fflush(stdout);
     }
  }
*/
if(xf->UseDisplayList)
{
  /*
   * Building an object is expensive, so only build one if we think we can
   * reuse it. i.e. for button down or button up where approx is equal 
   * to button down.
   */
  if(!buttonUp)
  {
    if (!xf->glObject)
    {
      xf->glObject = genobj();
      xf->deletePrivate = _deleteGLObject;
      TIMER("> makeobj");
      printf("A");
      makeobj(xf->glObject);
      if(!_drawPrimitives(portHandle, xf, approx, density))
      {
	closeobj();
	_deleteGLObject(xf);
	goto error;
      }
      closeobj();
    }
    TIMER("< makeobj");
    TIMER("> callobj");
    printf(".");
    callobj(xf->glObject);
    TIMER("< callobj");
  } /* !buttonUp */
  else
  if(xf->UseFastClip)
  {
     if(!xf->glObject)
     {
        xf->glObject = genobj();
        xf->deletePrivate = _deleteGLObject;
        TIMER("> makeobj");
	printf("B");
        makeobj(xf->glObject);
        if (!_drawPrimitives(portHandle, xf, approx, density)) {
	  closeobj();
	  _deleteGLObject(xf);
	  goto error;
        }
        closeobj();
     }
     printf(",");
     callobj(xf->glObject);
  } /* !xf->UseFastClip */
  else
  {
    if (!_drawPrimitives(portHandle, xf, approx, density))
      goto error;
  }
} /* !xf->UseDisplayList */
else
  if(xf->UseFastClip)
  {
     if(!xf->glObject)
     {
        xf->glObject = genobj();
        xf->deletePrivate = _deleteGLObject;
        TIMER("> makeobj");
	printf("C");
        makeobj(xf->glObject);
        if (!_drawPrimitives(portHandle, xf, approx, density))
	{
	  closeobj();
	  _deleteGLObject(xf);
	  goto error;
        }
        closeobj();
     }
     printf("//");
     callobj(xf->glObject);
  } /* !xf->UseFastClip */
  /*
  else
  {
    if (!_drawPrimitives(portHandle, xf, approx, density))
      goto error;
  }
  */
else
/*
#else
*/

  if (!_drawPrimitives(portHandle, xf, approx, density))
    goto error;
/*
#endif
*/
  PHASE_END("DrawPrimitives");
  fflush(stdout);

 done:
  /*
   * Disable lighting, reset matrix stack
   */
  _dxf_SET_GLOBAL_LIGHT_ATTRIBUTES(PORT_CTX ,0);

  _dxf_POP_MATRIX(PORT_CTX);

  PHASE_END("DrawOpaque");

  TIMER("< DrawOpaque");
  EXIT(("OK"));
  return OK;

 error:

  /*
   * Disable lighting, reset matrix stack
   */
  _dxf_SET_GLOBAL_LIGHT_ATTRIBUTES(PORT_CTX ,0);
  _dxf_POP_MATRIX(PORT_CTX);

  PHASE_END("DrawOpaque");
  TIMER("< DrawOpaque");

  EXIT(("ERROR"));
  return ERROR;
}

_drawTranslucentPrimitives(tdmPortHandleP portHandle, xfieldP xf, 
		SortD list, int n,
		enum approxE approx, int density)
{
  helperFunc	helper;
  
  ENTRY(("_drawPrimitives(0x%x, 0x%x, %d, %d)",
	 portHandle, xf, approx, density));

  TIMER("> drawTranslucentPrimitives");

  if(xf->invCntns)
    if(! DXInitGetNextInvalidElementIndex(xf->invCntns))
      goto error;

  finish(); /* Flush Geometry Pipeline */

  PHASE_START("TranslucentPrimitives");

  blendfunction(BF_SA, BF_MSA);

  switch (xf->connectionType) {
  case ct_none:
    TIMER("> translucent points");
    translucentPolygons(xf,helper,list,n,0,approx,density);
    TIMER("< dots");
    break;
  case ct_lines:
    if(xf->attributes.aalines) 
        linesmooth(SML_ON);
    helper = getHelper(xf->colorsDep,dep_none,xf->opacitiesDep,NULL);
    TIMER("> lines");
    translucentLines(xf,helper,list,n,0,approx,density);
    TIMER("< lines");
    linesmooth(SML_OFF);
    break;
  case ct_triangles: 
    helper = getHelper(xf->colorsDep,xf->normalsDep,xf->opacitiesDep,
		       (xf->texture != NULL) && (approx == approx_none));
    TIMER("> triangles");
    translucentPolygons(xf,helper,list,n,0,approx,density);
    TIMER("< triangles");
    break;
  case ct_quads:
    helper = getHelper(xf->colorsDep,xf->normalsDep,xf->opacitiesDep,
		       (xf->texture != NULL) && (approx == approx_none));
    TIMER("> quads");
    translucentPolygons(xf,helper,list,n,4,approx,density);
    TIMER("< quads");
    break;
    break;
  default:
    DXErrorGoto (ERROR_INTERNAL, "invalid xfield connections type in translucent") ;
    break;
  }
  PHASE_END("TranslucentPrimitives");
  
 done:
  TIMER("< drawTranslucentPrimitives");
  EXIT(("OK"));
  return OK;

 error:
  TIMER("< drawTranslucentPrimitives");
  EXIT(("ERROR"));
  return ERROR;
}

Error
_dxf_DrawTranslucent (void *globals,
		 RGBColor ambientColor, int buttonUp)
{
  int		density;
  enum approxE	approx;
  dxObject 	Obj;
  int      	DisplayList;
  int  		i, j;
  xfieldP       xf;
  DEFGLOBALDATA(globals);
  DEFPORT(PORT_HANDLE);
  SortListP     sl = (SortListP)SORTLIST;
  SortD         sorted = sl->sortList;
  int           nSorted = sl->itemCount;

  TIMER("> DrawTranslucent");

  for (i = 0; i < nSorted; i = j)
  {
    xf = (xfieldP)sorted[i].xfp;

    if(xf->normalsDep != dep_none &&
       (buttonUp &&xf->attributes.buttonUp.approx == approx_none) ||
       (!buttonUp &&xf->attributes.buttonDown.approx == approx_none)) {

      _dxf_SET_GLOBAL_LIGHT_ATTRIBUTES(PORT_CTX ,1);

      _dxf_SET_MATERIAL_SPECULAR (PORT_CTX, 1.0 /*R*/, 1.0 /*G*/, 1.0 /*B*/,
				(float) xf->attributes.front.shininess
				/* specular power */) ;
    }

    _dxf_PUSH_MULTIPLY_MATRIX(PORT_CTX,xf->attributes.mm);

    if(xf->colorsDep == dep_positions || xf->normalsDep == dep_positions || xf->opacitiesDep == dep_positions)
      shademodel(GOURAUD);
    else
      shademodel(FLAT);

    if (xf->nClips)
      if (! _dxf_ADD_CLIP_PLANES(LWIN, xf->clipPts, xf->clipVecs, xf->nClips))
	  goto error;


    if(buttonUp) {
      density = xf->attributes.buttonUp.density;
      approx = xf->attributes.buttonUp.approx;
    } else {
      density = xf->attributes.buttonDown.density;
      approx = xf->attributes.buttonDown.approx;
    }

    /*
     * Count the successive primitives from th same xfield
     */
    for (j = i+1; j < nSorted && xf == (xfieldP)sorted[j].xfp; j++);

    /* 
     * indigos get better performance from globjects, so try to 
     * use one
     */
    if(xf->UseFastClip)
    {
       if(!xf->glObject)
       {
	  xf->glObject = genobj();
	  xf->deletePrivate = _deleteGLObject;
	  TIMER("> makeobj");
	  printf("C");
	  makeobj(xf->glObject);
	  if (!_drawTranslucentPrimitives(PORT_HANDLE, xf, sorted+i, j-i, approx, density))
	  {
	    closeobj();
	    _deleteGLObject(xf);
	    goto error;
	  }
	  closeobj();
       }
       printf("//");
       callobj(xf->glObject);
    } 
    else
      if (!_drawTranslucentPrimitives(PORT_HANDLE, xf, sorted+i, j-i, approx, density))
	goto error;
    
    if (xf->nClips)
      if (! _dxf_REMOVE_CLIP_PLANES(LWIN, xf->nClips))
	  goto error;

    _dxf_POP_MATRIX(PORT_CTX) ;
  }

  PHASE_END("DrawPrimitives");
  fflush(stdout);

done:
  /*
   * Disable lighting, reset matrix stack
   */
  _dxf_SET_GLOBAL_LIGHT_ATTRIBUTES(PORT_CTX ,0);

  TIMER("< DrawTranslucent");
  EXIT(("OK"));
  return OK;

error:

  /*
   * Disable lighting, reset matrix stack
   */
  _dxf_SET_GLOBAL_LIGHT_ATTRIBUTES(PORT_CTX ,0);
  _dxf_POP_MATRIX(PORT_CTX);

  PHASE_END("DrawOpaque");
  TIMER("< DrawOpaque");

  EXIT(("ERROR"));
  return ERROR;
}

   
static void
_deleteGLObject(xfieldP xf)
{
  ENTRY(("_deleteGLObject(0x%x)", xf));

  if(xf->glObject)
  {
    delobj(xf->glObject);
    xf->glObject = NULL;
  }

#ifdef sgi
  if(xf->FlatGridTexture.Index)
  {
  /* (:Q=  Only Needed if LRU Paging Is Undesirable
     if(xf->FlatGridTexture.Index > 0)
        texbind(TX_TEXTURE_IDLE, xf->FlatGridTexture.Index);
  */
     xf->FlatGridTexture.Index = 0;
     xf->FlatGridTexture.Address = NULL;
  }
#endif

  EXIT((""));
}

#ifdef sgi

Error
glStartTexture(xfieldP xf)
{
   unsigned long * Tex = NULL;
   unsigned char * Char;
   unsigned char * CharOrig;
   unsigned long R, G, B;
   unsigned long CtrWidth, CtrHeight;
   unsigned long Out;
   unsigned long Tmp;

   static float TexType[5] = { TX_MINFILTER, TX_BILINEAR,
			       TX_MAGFILTER, TX_BILINEAR,
			       TX_NULL };
   static float TevType[2] = { TV_MODULATE, TV_NULL };

   Matrix Ident = { 1.0f, 0.0f, 0.0f,
                    0.0f, 1.0f, 0.0f,
                    0.0f, 0.0f, 1.0f };
   if((Tex = DXAllocate(xf->textureWidth * xf->textureHeight * 4)) == NULL)
   {
      DXSetError(ERROR_NO_MEMORY,
		 "Falied to Allocate Texture Translation Space");
      return ERROR;
   }

   Char = (unsigned char *)Tex;
   CharOrig = (unsigned char *)xf->texture;

   for(CtrHeight = 0; CtrHeight < xf->textureHeight; CtrHeight ++)
   {
      for(CtrWidth = 0; CtrWidth < xf->textureWidth; CtrWidth ++)
      {
	 Out = (*(CharOrig) << 24) + (*(CharOrig + 1) << 16) +
	       (*(CharOrig + 2) << 8) + *(CharOrig + 3);
         R = ((Out & 0x000000ff));
         G = ((Out & 0x0000ff00) >> 8);
         B = ((Out & 0x00ff0000) >> 16);
	 *Char++ = (char)G;
	 *Char++ = (char)B;
	 *Char++ = (char)R;
	 CharOrig += 3;
      }
      for(Tmp = (xf->textureWidth * 3); (Tmp % 4) != 0; Tmp ++)
      {
	 *Char++ = 0;
	 CharOrig ++;
      }
   }

   texdef2d(1, 3, xf->textureWidth, xf->textureHeight,
	    (unsigned long *)Tex, 5, TexType);

   DXFree(Tex);

   tevdef(1, 3, TevType);
   mmode(MTEXTURE);
   tevbind(TV_ENV0, 1);
   texbind(TX_TEXTURE_0, 1);
   mmode(MVIEWING);

   return OK;
}

Error
glEndTexture()
{
    tevbind(TV_ENV0, 0);
}
#endif


#else


/*
 * HELPERCODE
 * code to generate helper routines and table
 */

#include <stdio.h>
main()
{
     int i;
     char c, n, o, tbl[] = {'v', 'e', 'f', 'n'};
     for (i = 0; i < 64; i++)
     {
  	  c = tbl[i>>4];
  	  n = tbl[0x3&(i>>2)];
  	  o = tbl[0x3&(i)];
  	  printf("static void\n"
		 "helper_%c%c%c(xfieldP xf, int ppc,"
		 " int c, int *t, float *rgba)\n"
		 "{\n"
		 "    static float fscratch[3];\n"
		 "    static int p, iscratch;\n\n",
  		 c, n, o);
	  if (c == 'e')
	      printf("    GETCOLOR(c);\n");
	  if (o == 'e')
	      printf("    GETOPACITY(c);\n");
	  if (n == 'e')
	      printf("    SENDNORMAL(c);\n");
	  if ((o == 'e' || o == 'n') && c == 'e' && (n == 'e' || n == 'n' || n == 'f'))
	      if (n == 'n')
		  printf("    SENDCOLOR(%d);\n", o != 'n' ? 1 : 0);
	      else
		  printf("    SENDLITCOLOR(%d);\n", o != 'n' ? 1 : 0);

	  printf("    for (p = 0; p < ppc; p++) {\n");

	  if (n == 'v')
	      printf("        SENDNORMAL(t[p]);\n");
#if 0
	  else if (n == 'f')
	      printf("        SENDNORMAL(0);\n");
#endif
	  if (c == 'v')
	      printf("        GETCOLOR(t[p]);\n");
	  if (o == 'v')
	      printf("        GETOPACITY(t[p]);\n");
	  if (c == 'v' || o == 'v' || n == 'v')
	      if (n == 'n')
		  printf("        SENDCOLOR(%d);\n", o != 'n' ? 1 : 0);
	      else
		  printf("        SENDLITCOLOR(%d);\n", o != 'n' ? 1 : 0);
	  printf("        SENDPOSITION(t[p]);\n");
	  printf("    }\n");
	  printf("    return;\n");
	  printf("}\n\n");
     }
    
     printf("static helperFunc helperTable[] = {\n");
     for (i = 0; i < 64; i++)
     {
  	  c = tbl[i>>4];
  	  n = tbl[0x3&(i>>2)];
  	  o = tbl[0x3&(i)];
  	  printf("%shelper_%c%c%c%c%c",
    	         ((i%4) == 0) ? "    " : " ",
  		 c, n, o, (i == 63) ? ' ' : ',',
  		 ((i%4) == 3) ? '\n' : ' ');
     }
     printf("};\n\n");
}
  
#endif
