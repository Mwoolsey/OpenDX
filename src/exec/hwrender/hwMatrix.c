/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Source: /src/master/dx/src/exec/hwrender/hwMatrix.c,v $
 */

#include <dxconfig.h>

#if defined(HAVE_STRINGS_H)
#include <strings.h>
#endif

#include <stdio.h>
#include <math.h>

#ifndef STANDALONE
#include "hwMemory.h"
#endif
#include "hwDeclarations.h"
#include "hwMatrix.h"
#include "hwPortLayer.h"
#include "hwDebug.h"

/*
 *  TDM maintains the current transform on both the hardware stack and its
 *  own stack as implemented here.  We do this because we need to be able
 *  to access the modeling, viewing, projection, and screen transforms
 *  independently for certain computations.
 *
 *  While some API's such as Starbase provide a separate modeling
 *  transformation stack, other API's such as GL combine the
 *  modeling/viewing matrix.  (This is determined by the coordinate system
 *  in which the API performs lighting.  Starbase performs lighting in
 *  world coordinates and so combines the non-orthogonal projection
 *  components of the transformation pipeline into a viewing/projection
 *  matrix, while GL computes lighting in viewing coordinates and so has
 *  an independent projection matrix).
 *
 *  We implement a GL-style matrix stack for the software shadow.
 */

typedef struct tdmTransformS {
  float	m[4][4] ;
  float fuzz ;
  float ff ;
} tdmTransformT ;

typedef struct tdmViewportS {
  int left, right, bottom, top ;
} tdmViewportT ;

typedef struct tdmTransformStackS {
  /* modeling transform stack */
  int mSize ;
  tdmTransformT	*mStack ;
  tdmTransformT	*mTop ;

  /* view transform stack */
  int vSize ;
  tdmTransformT *vStack ;
  tdmTransformT *vTop ;

  /* viewport stack */
  int pSize ;
  tdmViewportT *pStack ;
  tdmViewportT *pTop ;

  /* composite (model X view X fuzz) matrix */
  float composite[4][4] ;
  int composite_current ;

  /* projection info */
  float projmat[4][4], width, aspect, Near, Far ;
  int projection, resolution, projection_current ;
} tdmTransformStackT, *tdmTransformStackP ;


#define	EMPTY(stack, top) (top == stack)
#define	FULL(stack, top, size) (top == &stack[size-1])

#define FREE(stack)                                                         \
{                                                                           \
  if (stack)                                                                \
    {                                                                       \
      tdmFree(stack) ;                                                      \
      stack = NULL ;                                                        \
    }                                                                       \
}

#define GROW(type, stack, top, size)   	       	       	                    \
{								            \
  register int i ;						            \
  register type *p, *newTop, *newStack ;			            \
 								            \
  newStack = newTop = (type *) tdmAllocateLocal(sizeof(type) * (size+32)) ; \
  								            \
  for (i=0, p=stack ; i<size ; i++)			                    \
      *newTop++ = *p++ ;					            \
								            \
  if (stack) tdmFree(stack) ;				                    \
								            \
  stack = newStack ;						            \
  top = newTop ;						            \
  size += 32 ;						                    \
}

#define POP(stack, top)                                                     \
{		       						            \
  if (!EMPTY(stack, top)) top-- ;				            \
}

#define PUSH(type, stack, top, size)                                        \
{								            \
  /* push copy of top */					            \
  if (FULL(stack, top, size))					            \
      GROW(type, stack, top, size) ;				            \
								            \
  *(top+1) = *top ;						            \
  top++ ;							            \
}


void *
_dxfCreateStack()
{
  tdmTransformStackP tdmStack ;

  ENTRY(("_dxfCreateStack()"));

  if (! (tdmStack = (tdmTransformStackP)
	            tdmAllocateLocal(sizeof(tdmTransformStackT))))
      goto error ;

  tdmStack->mSize = 0 ;
  tdmStack->mTop = tdmStack->mStack = NULL ;
  GROW(tdmTransformT, tdmStack->mStack, tdmStack->mTop, tdmStack->mSize) ;

  tdmStack->vSize = 0 ;
  tdmStack->vTop = tdmStack->vStack = NULL ;
  GROW(tdmTransformT, tdmStack->vStack, tdmStack->vTop, tdmStack->vSize) ;

  tdmStack->pSize = 0 ;
  tdmStack->pTop = tdmStack->pStack = NULL ;
  GROW(tdmViewportT, tdmStack->pStack, tdmStack->pTop, tdmStack->pSize) ;

  EXIT((""));
  return (void *)tdmStack ;

 error:

  EXIT(("ERROR"));
  return (void *)0 ;
}

void
_dxfInitStack(void *stack, int resolution, float view[4][4], int projection,
	     float width, float aspect, float Near, float Far)
{
  tdmTransformStackP tdmStack = (tdmTransformStackP) stack ;

  ENTRY(("_dxfInitStack(0x%x, %d, 0x%x, %d, %f, %f, %f, %f)",
	 stack, resolution, view, projection, width, aspect, Near, Far));

  tdmStack->mTop = tdmStack->mStack ;
  tdmStack->mTop->fuzz = 0 ;
  tdmStack->mTop->ff = 1 ;
  _dxfLoadMatrix(stack, (float (*)[4])identity) ;

  tdmStack->vTop = tdmStack->vStack ;
  _dxfLoadViewMatrix(stack, view) ;

  tdmStack->resolution = resolution ;
  _dxfSetProjectionInfo(stack, projection, width, aspect, Near, Far) ;

  tdmStack->composite_current = 0 ;

  EXIT((""));
}

void
_dxfFreeStack (void *stack)
{
  tdmTransformStackP tdmStack = (tdmTransformStackP) stack ;

  ENTRY(("_dxfFreeStack(0x%x)", stack));

  if (!tdmStack) {
    EXIT((""));
    return ;
  }

  FREE(tdmStack->mStack) ; /* modeling */
  FREE(tdmStack->vStack) ; /* viewing */
  FREE(tdmStack->pStack) ; /* viewport */

  tdmFree(tdmStack) ;

  EXIT((""));
}

void
_dxfSetFuzzValue (void *stack, float fuzz)
{
  register tdmTransformStackP tdmStack = (tdmTransformStackP) stack ;

  ENTRY(("_dxfSetFuzzValue(0x%x, %f)", stack, fuzz));

  /* ff is normally 1.0 except when drawing screen objects */
  tdmStack->mTop->fuzz = tdmStack->mTop->ff * fuzz ;
  tdmStack->composite_current = 0 ;

  EXIT((""));
}


void
_dxfSetScreenFuzzFactor (void *stack, float ff)
{
  register tdmTransformStackP tdmStack = (tdmTransformStackP) stack ;

  ENTRY(("_dxfSetScreenFuzzFactor(0x%x, %f", stack, ff));

  /* ff is normally 1.0 except when drawing screen objects */
  tdmStack->mTop->ff = ff ;
  tdmStack->mTop->fuzz *= ff ;
  tdmStack->composite_current = 0 ;

  EXIT((""));
}


void
_dxfPushMatrix (void *stack)
{
  /* push modeling transform and copy to top */
  register tdmTransformStackP tdmStack = (tdmTransformStackP) stack ;

  ENTRY(("_dxfPushMatrix(0x%x)", stack));

  PUSH(tdmTransformT, tdmStack->mStack, tdmStack->mTop, tdmStack->mSize) ;

  EXIT((""));
}

void
_dxfPushViewMatrix (void *stack)
{
  /* push viewing transform and copy to top */
  register tdmTransformStackP tdmStack = (tdmTransformStackP) stack ;

  ENTRY(("_dxfPushViewMatrix(0x%x)", stack));

  PUSH(tdmTransformT, tdmStack->vStack, tdmStack->vTop, tdmStack->vSize) ;

  EXIT((""));
}

void
_dxfPushViewport (void *stack)
{
  /* push viewport and copy to top */
  register tdmTransformStackP tdmStack = (tdmTransformStackP) stack ;

  ENTRY(("_dxfPushViewport(0x%x)", stack));

  PUSH(tdmViewportT, tdmStack->pStack, tdmStack->pTop, tdmStack->pSize) ;

  EXIT((""));
}

void
_dxfPopMatrix (void *stack)
{
  /* pop modeling transform */
  register tdmTransformStackP tdmStack = (tdmTransformStackP) stack ;

  ENTRY(("_dxfPopMatrix(0x%x)", stack));

  POP(tdmStack->mStack, tdmStack->mTop) ;
  tdmStack->composite_current = 0 ;

  EXIT((""));
}

void
_dxfPopViewMatrix (void *stack)
{
  /* pop view transform */
  register tdmTransformStackP tdmStack = (tdmTransformStackP) stack ;

  ENTRY(("_dxfPopViewMatrix(0x%x)", stack));
  
  POP(tdmStack->vStack, tdmStack->vTop) ;
  tdmStack->composite_current = 0 ;

  EXIT((""));
}

void
_dxfPopViewport (void *stack)
{
  /* pop viewport stack */
  register tdmTransformStackP tdmStack = (tdmTransformStackP) stack ;

  ENTRY(("_dxfPopViewport(0x%x)", stack));

  POP(tdmStack->pStack, tdmStack->pTop) ;

  EXIT((""));
}


void
_dxfMultMatrix(void *stack, register float m[4][4])
{
  /* replace top modeling transform with [m] X [current top] */
  register tdmTransformStackP tdmStack = (tdmTransformStackP) stack ;
  float newMatrix[4][4] ;

  ENTRY(("_dxfMultMatrix(0x%x, 0x%x)", stack, m));

  MULTMATRIX(newMatrix, m, tdmStack->mTop->m) ;
  COPYMATRIX(tdmStack->mTop->m, newMatrix) ;
  tdmStack->composite_current = 0 ;

  EXIT((""));
}

void
_dxfLoadMatrix(void *stack, float m[4][4])
{
  /* replace top modeling transform with [m] */
  register tdmTransformStackP tdmStack = (tdmTransformStackP) stack ;

  ENTRY(("_dxfLoadMatrix(0x%x, 0x%x)", stack, m));

  COPYMATRIX(tdmStack->mTop->m, m) ;
  tdmStack->composite_current = 0 ;

  EXIT((""));
}

void
_dxfLoadViewMatrix(void *stack, float m[4][4])
{
  /* replace top viewing matrix with [m] */
  register tdmTransformStackP tdmStack = (tdmTransformStackP) stack ;

  ENTRY(("_dxfLoadViewMatrix(0x%x, 0x%x)", stack, m));

  COPYMATRIX(tdmStack->vTop->m, m) ;
  tdmStack->composite_current = 0 ;

  EXIT((""));
}

void
_dxfSetViewport(void *stack, int left, int right, int bottom, int top)
{
  /* record viewport coordinates */
  register tdmTransformStackP tdmStack = (tdmTransformStackP) stack ;

  ENTRY(("_dxfSetViewport(0x%x, %d, %d, %d, %d)",
	 stack, left, right, bottom, top));

  tdmStack->pTop->left = left ;
  tdmStack->pTop->right = right ;
  tdmStack->pTop->bottom = bottom ;
  tdmStack->pTop->top = top ;

  EXIT((""));
}

void
_dxfGetMatrix(void *stack, float m[4][4])
{
  /* return modeling transform from top of stack */
  register tdmTransformStackP tdmStack = (tdmTransformStackP) stack ;

  ENTRY(("_dxfGetMatrix(0x%x, 0x%x)", stack, m));

  COPYMATRIX(m, tdmStack->mTop->m) ;

  EXIT((""));
}

void
_dxfGetViewMatrix(void *stack, float m[4][4])
{
  /* return view transform from top of stack */
  register tdmTransformStackP tdmStack = (tdmTransformStackP) stack ;

  ENTRY(("_dxfGetViewMatrix(0x%x, 0x%x)", stack, m));

  COPYMATRIX(m, tdmStack->vTop->m) ;

  EXIT((""));
}

void
_dxfGetViewport(void *stack, int *left, int *right, int *bottom, int *top)
{
  /* return viewport from top of stack */
  register tdmTransformStackP tdmStack = (tdmTransformStackP) stack ;

  ENTRY(("_dxfGetViewport(0x%x, 0x%x, 0x%x, 0x%x, 0x%x)",
	 stack, left, right, bottom, top));

  /* !!!! Should test for NULL pointers !!!! */
  *left = tdmStack->pTop->left ;
  *right = tdmStack->pTop->right ;
  *bottom = tdmStack->pTop->bottom ;
  *top = tdmStack->pTop->top ;

  EXIT((""));
}

void
_dxfGetViewMatrixWithFuzz(void *stack, double fuzz, register float m[4][4])
{
  /* return [view] X [fuzz] */
  register tdmTransformStackP tdmStack = (tdmTransformStackP) stack ;

  ENTRY(("_dxfGetViewMatrixWithFuzz(0x%x, %f, 0x%x)", stack, fuzz, m));

  COPYMATRIX(m, tdmStack->vTop->m) ;

  /* [view] x [fuzz] */
  if (fuzz != 0.0)
    {
      /*
       *  Apply fuzz.  See "Method for drawing lines, curves, and points
       *  coincident with surfaces" [Lucas, Allain 1992].
       */

      float dOffset, half_pixel ;
      half_pixel = tdmStack->width / (2.0 * tdmStack->resolution) ;
      dOffset = fuzz * half_pixel ;

      if (tdmStack->projection)
	{
	  /* apply matrix from equation (8) for perspective depth offset */
	  register float f = 1.0 - dOffset ;

	  m[0][0] *= f ;  m[0][1] *= f ;  m[0][2] *= f ;
	  m[1][0] *= f ;  m[1][1] *= f ;  m[1][2] *= f ;
	  m[2][0] *= f ;  m[2][1] *= f ;  m[2][2] *= f ;
	  m[3][0] *= f ;  m[3][1] *= f ;  m[3][2] *= f ;
	}
      else
	{
	  /* apply matrix from equation (4) for orthogonal depth offset */
	  m[3][2] += dOffset ;
	}
    }

  PRINT(("[view] X [fuzz] ="));
  MPRINT(m);

  EXIT((""));
}

void
_dxfGetCompositeMatrix(void *stack, register float m[4][4])
{
  /* return [model] X [view] X [fuzz] */
  register tdmTransformStackP tdmStack = (tdmTransformStackP) stack ;


  ENTRY(("_dxfGetCompositeMatrix(0x%x, 0x%x)", stack, m));

  if (tdmStack->composite_current)
    {
      COPYMATRIX(m, tdmStack->composite) ;
      EXIT(("composite was not recomputed"));
      return ;
    }

  /* [view] X [fuzz] */
  _dxfGetViewMatrixWithFuzz(stack, tdmStack->mTop->fuzz, m);

  /* [model] X [view] X [fuzz] */
  MULTMATRIX(tdmStack->composite, tdmStack->mTop->m, m) ;

  COPYMATRIX(m, tdmStack->composite) ;
  tdmStack->composite_current = 1 ;

  PRINT(("[model] X [view] X [fuzz] ="));
  MPRINT(m);
  EXIT((""));
}

void
_dxfGetProjectionInfo (void *stack, int *projection,
		      float *width, float *aspect, float *Near, float *Far)
{
  /* return projection info */
  register tdmTransformStackP tdmStack = (tdmTransformStackP) stack ;

  ENTRY(("_dxfGetProjectionInfo(0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x)",
	 stack, projection, width, aspect, Near, Far));

  /* !!!! Should test for NULL pointers !!!! */
  *projection = tdmStack->projection ;
  *width = tdmStack->width ;
  *aspect = tdmStack->aspect ;
  *Near = tdmStack->Near ;
  *Far = tdmStack->Far ;

  EXIT((""));
}

void
_dxfSetProjectionInfo (void *stack, int projection,
		      float width, float aspect, float Near, float Far)
{
  /* record projection info */
  register tdmTransformStackP tdmStack = (tdmTransformStackP) stack ;

  ENTRY(("_dxfSetProjectionInfo(0x%x, %d, %f, %f, %f, %f)",
	 stack, projection, width, aspect, Near, Far));

  tdmStack->projection = projection ;
  tdmStack->width = width ;
  tdmStack->aspect = aspect ;
  tdmStack->Near = Near ;
  tdmStack->Far = Far ;
  tdmStack->projection_current = 0 ;

  /*
   *  Warn about view angles < 0.001 degrees.  This is equivalent to a width
   *  of 2*tan(0.001/2) = ~1.745329e-05 = ~tan(0.0005) for this small angle.
   */
  
  if (projection && width < 1.745329e-05)
    {
      PRINT(("view angle less than 0.001 degrees: display unpredictable"));
      DXWarning ("#13930") ;
    }

  EXIT((""));
}

void
_dxfGetProjectionMatrix (void *portHandle, void *stack, float projmat[4][4])
{
  DEFPORT(portHandle);
  register tdmTransformStackP tdmStack = (tdmTransformStackP) stack ;

  ENTRY(("_dxfGetProjectionMatrix(0x%x, 0x%x, 0x%x)",
	 portHandle, stack, projmat));

  if (! tdmStack->projection_current)
    {
      _dxf_MAKE_PROJECTION_MATRIX
	  (tdmStack->projection, tdmStack->width,
	   tdmStack->aspect, tdmStack->Near, tdmStack->Far,
	   tdmStack->projmat) ;

      tdmStack->projection_current = 1 ;
    }

  COPYMATRIX(projmat, tdmStack->projmat) ;

  EXIT((""));
}

/*
 *  Misc. algebra
 */

void
_dxfMult44f (register double s0[4][4], register float s1[4][4])
{
  double res[4][4] ;
  register int i, j, k ;

  ENTRY(("_dxfMult44f(0x%x, 0x%x)", s0, s1));
  
  DMCOPY (res, dZero) ;
  for (i = 0 ; i < 4 ; i++)
      for (j = 0 ; j < 4 ; j++)
          for (k = 0 ; k < 4 ; k++)
              res[i][j] += (s0[i][k] * (double)s1[k][j]) ;

  DMCOPY (s0, res) ;

  EXIT((""));
}

void
_dxfMult44 (register double s0[4][4], register double s1[4][4])
{
  double res[4][4];
  register int i, j, k;

  ENTRY(("_dxfMult44(0x%x, 0x%x)", s0, s1));
  
  DMCOPY (res, dZero) ;
  for (i = 0 ; i < 4 ; i++)
      for (j = 0 ; j < 4 ; j++)
          for (k = 0 ; k < 4 ; k++)
              res[i][j] += (s0[i][k] * s1[k][j]) ;

  DMCOPY (s0, res) ;

  EXIT((""));
}

#define DET33(s) \
        (( s[0][0] * ((s[1][1] * s[2][2]) - (s[2][1] * s[1][2])) ) + \
         (-s[0][1] * ((s[1][0] * s[2][2]) - (s[2][0] * s[1][2])) ) + \
         ( s[0][2] * ((s[1][0] * s[2][1]) - (s[2][0] * s[1][1])) ) )

static double
cofac (register double s0[4][4], register int s1, register int s2)
{
  register int i, j, I, J ;
  double cofac[4][4] ;

  ENTRY(("cofac(0x%x, %d, %d)", s0, s1, s2));
  
  for (i = 0, I = 0 ; i < 4 ; i++)
      if (i != s1)
        {
          for (j = 0, J = 0 ; j < 4 ; j++)
              if (j != s2)
                  cofac[I][J++] = s0[i][j] ;
          I++ ;
        }

  EXIT((""));
  return DET33(cofac) ;
}
/* s1 <- inverse (s0) */

void
_dxfInverse (register double s1[4][4], register double s0[4][4])
{
  register int i, j ;
  double det ;

  ENTRY(("_dxfInverse(0x%x, 0x%x)", s1, s0));
  
  det = (s0[0][0] * cofac(s0, 0, 0)) + (-s0[0][1] * cofac(s0, 0, 1)) +
        (s0[0][2] * cofac(s0, 0, 2)) + (-s0[0][3] * cofac(s0, 0, 3)) ;

  det = 1/det ;

  for (i = 0 ; i < 4 ; i++)
      for (j = 0 ; j < 4 ; j++)
          s1[j][i] = ((((i + j + 1) % 2) * 2) - 1) * cofac(s0, i, j) * det ;

  EXIT((""));
}

/* s1 <- adjointTranspose (s0) */
void
_dxfAdjointTranspose (register double s1[4][4], register double s0[4][4])
{
  register int i, j ;

  ENTRY(("_dxfAdjointTranspose(0x%x, 0x%x)", s1, s0));
  
  for (i = 0 ; i < 3 ; i++)
      for (j = 0 ; j < 3 ; j++)
          s1[i][j] = ((((i + j + 1) % 2) * 2) - 1) * cofac(s0, i, j);

  for(i = 0; i < 3 ; i++) {
    s1[3][i] = 0.0; 
    s1[i][3] = 0.0;
  }

  s1[3][3] = 1.0;

  EXIT((""));
}

/* s1 <- Transpose (s0) */
void
_dxfTranspose (register double s1[4][4], register double s0[4][4])
{
  register int i, j ;

  ENTRY(("_dxfTranspose(0x%x, 0x%x)", s1, s0));

  for (i = 0 ; i < 4 ; i++)
      for (j = 0 ; j < 4 ; j++) {
	s1[i][j] = s0[j][i];
      }

  EXIT((""));
}

void
_dxfRenormalizeView (register float m[4][4])
{
  double len, x[3], z[3] ;


  ENTRY(("_dxfRenormalizeView(0x%x)", m));
  
  /* normalize Z axis */
  len = sqrt((double)(m[0][2]*m[0][2] + m[1][2]*m[1][2] + m[2][2]*m[2][2])) ;
  m[0][2] = z[0] = m[0][2] / len ;
  m[1][2] = z[1] = m[1][2] / len ;
  m[2][2] = z[2] = m[2][2] / len ;

  /* cross Y with Z to get X axis */
  x[0] = m[1][1]*z[2] - m[2][1]*z[1] ;
  x[1] = m[2][1]*z[0] - m[0][1]*z[2] ;
  x[2] = m[0][1]*z[1] - m[1][1]*z[0] ;

  /* normalize the X axis */
  len = sqrt(x[0]*x[0] + x[1]*x[1] + x[2]*x[2]) ;
  m[0][0] = x[0] = x[0] / len ;
  m[1][0] = x[1] = x[1] / len ;
  m[2][0] = x[2] = x[2] / len ;

  /* cross Z with X axis to get new Y axis */
  m[0][1] = z[1]*x[2] - z[2]*x[1] ;
  m[1][1] = z[2]*x[0] - z[0]*x[2] ;
  m[2][1] = z[0]*x[1] - z[1]*x[0] ;

  /* this shouldn't be necessary, but IBM GL fails here! */
  m[0][3] = 0 ;
  m[1][3] = 0 ;
  m[2][3] = 0 ;
  m[3][3] = 1 ;

  EXIT((""));
}

void
_dxfGetNearFar(int projection, int resolution, float width,
	      float from[3], float zaxis[3], float box[8][3],
              float *Near, float *Far)
{
  register int i ;
  register float ClipMin, ClipMax, len, fuzz ;
  float BoxVecs[8][3] ;

  /*
   *  Calculate new clip planes based on WC bounding box.
   */

  ENTRY(("_dxfGetNearFar(%d, %d, %f, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x)",
	 projection, resolution, width, from, zaxis, box, Near, Far));
  
  PRINT(("from point:"));
  VPRINT(from);
  PRINT(("view vector:"));
  VPRINT(zaxis);

  for (i=0; i<8; i++)
    {
      BoxVecs[i][0] = box[i][0] - from[0];
      BoxVecs[i][1] = box[i][1] - from[1];
      BoxVecs[i][2] = box[i][2] - from[2];
    }

  ClipMin =  DXD_MAX_FLOAT ;
  ClipMax = -DXD_MAX_FLOAT ;
  for (i=0; i<8; i++)
    {
      /* project vectors from `from' to box corners onto view direction */
      len = BoxVecs[i][0]*zaxis[0] +
	    BoxVecs[i][1]*zaxis[1] +
	    BoxVecs[i][2]*zaxis[2];
      if (len < ClipMin) ClipMin = len;
      if (len > ClipMax) ClipMax = len;
    }

  PRINT(("Far  clip pre-fuzz:  %f", ClipMax));
  PRINT(("Near clip pre-fuzz:  %f", ClipMin));

  /*
   *  DX maintains a notion of "fuzz" which is used to prevent surfaces
   *  from burying coincident lines, curves and points (surface markings).
   *  Fuzz is a function of the ratio of pixels to world coordinate units,
   *  and is added to the projection depth coordinates of all surface
   *  markings, so we must adjust the clip planes by at least this amount.
   *
   *  We won't know the fuzz value until DX object traversal starts,
   *  but 4 is the most typical value.
   */

  /*
   * The aformentioned '4' didn't take accumulated fuzz into
   * account.  For the time being we're going to assume that
   * the sum total of fuzz will be < 100
   */

  /*
  fuzz = 4/(2.0*resolution/width) ;
  */
  fuzz = 100/(2.0*resolution/width) ;

  fuzz = 1.1 * (projection? ClipMax*fuzz: fuzz) ;
  fuzz = fuzz < 0? -fuzz: fuzz ;

  if (projection && ClipMin < 2*fuzz)
      ClipMin = ClipMax > 0? ClipMax/1000: fuzz ;
  else
      ClipMin -= fuzz ;

  if (ClipMax <= ClipMin)
      ClipMax = ClipMin + fuzz ;
  else
      ClipMax += fuzz ;

  *Far = ClipMax;
  *Near = ClipMin;

  PRINT(("fuzz: %f", fuzz));
  PRINT(("Far  clip post-fuzz: %f", *Far));
  PRINT(("Near clip post-fuzz: %f", *Near));

  EXIT((""));
}

void
_dxfSetProjectionMatrix(void *portHandle, void *stack, float projmat[4][4])
{
  register tdmTransformStackP tdmStack = (tdmTransformStackP) stack ;
  COPYMATRIX(tdmStack->projmat, projmat) ;
  tdmStack->projection_current = 1;
}

