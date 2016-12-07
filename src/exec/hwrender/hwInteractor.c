/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/hwrender/hwInteractor.c,v 1.6 2006/01/03 17:02:27 davidt Exp $"  ;
 */

#include <dxconfig.h>

#if defined(HAVE_STRINGS_H)
#include <strings.h>
#endif

#if defined(HAVE_STRING_H)
#include <string.h>
#endif

/*---------------------------------------------------------------------------*\
 $Source: /src/master/dx/src/exec/hwrender/hwInteractor.c,v $
    
  This file contains core utilities for managing direct interactors.

\*---------------------------------------------------------------------------*/


#include <math.h>
#include "hwDeclarations.h"
#ifndef STANDALONE
#include "hwMemory.h"
#endif

#include "hwDebug.h"

/*
 *  Allocation utils
 */

static tdmInteractor _allocateInteractorArray (tdmInteractorWin W) ;
static tdmInteractor _allocateMoreInteractors (tdmInteractorWin W) ;

/* interactor storage is allocated in multiples of ARRAYSIZE */
#define ARRAYSIZE 16

/* pointer to array of `ARRAYSIZE' interactors */
typedef tdmInteractorT (*InteractorArray)[ARRAYSIZE] ; 


tdmInteractorWin
_dxfInitInteractors (void *portHandle, void *stack)
{
  tdmInteractorWin W ;

  ENTRY(("_dxfInitInteractors(0x%x, 0x%x)", portHandle, stack));

  /* allocate interactor common data associated with image window */
  if (! (W = (tdmInteractorWin) tdmAllocateLocal (sizeof(tdmInteractorWinT))))
    {
      /* !!!!! No warning message set???? */
      EXIT(("ERROR:out of memory"));
      return (tdmInteractorWin) 0 ;
    }

  W->Interactors = (tdmInteractor *) 0 ;
  W->extents = (void *) 0 ;
  W->numUsed = 0 ;
  W->numAllocated = 0 ;

  /* initialize camera stack */
  W->Cam = (tdmInteractorCam) 0 ;
  W->redo = (tdmInteractorCam) 0 ;
  _dxfPushInteractorCamera(W) ;

  W->Cam->view_state++ ;
  W->Cam->cursor_speed = tdmMEDIUM_CURSOR ;

  /* copy handles to graphics API context and software transformation stack */
  W->Cam->phP = portHandle ;
  W->Cam->stack = stack ;

  EXIT(("W = 0x%x", W));
  return W ;
}

void
_dxfDestroyAllInteractors (tdmInteractorWin W)
{
  int i ;
  tdmInteractorCam curr, next ;

  ENTRY(("_dxfDestroyAllInteractors(0x%x)", W));

  if (! W)
    {
      EXIT(("ERROR: bad window"));
      return ;
    }

  /* destroy all used interactors */
  if (W->Interactors)
    {
      for (i = 0 ; i < W->numAllocated ; i++)
          if (IS_USED(W->Interactors[i]))
              /*
               *  Invoke the interactor's destroy method.  It must call
               *  _dxfDeallocateInteractor() itself at some point. 
               */
              tdmDestroyInteractor(W->Interactors[i]) ;

      tdmFree((void *) W->Interactors) ;
      W->Interactors = (tdmInteractor *) 0 ;
    }

  /* destroy all allocated interactor data */
  if (W->extents)
    {
      InteractorArray *array = (InteractorArray *) W->extents ;
      int numInteractorArrays = W->numAllocated / ARRAYSIZE ;

      for (i = 0 ; i < numInteractorArrays ; i++)
        {
          tdmFree((void *) array[i]) ;
          array[i] = (InteractorArray) 0 ;
        }

      tdmFree(W->extents) ;
      W->extents = (void *) 0 ;
    }

  /* destroy all camera info on undo and redo stacks */
  curr = W->Cam ;
  W->Cam = (tdmInteractorCam) 0 ;
  while (curr)
    {
      next = curr->next ;
      curr->next = (tdmInteractorCam) 0 ;

      tdmFree ((void *) curr) ;

      if (next)
          curr = next ;
      else
        {
          curr = W->redo ;
          W->redo = (tdmInteractorCam) 0 ;
        }
    }

  tdmFree((void *) W) ;
  EXIT((""));
}

tdmInteractor
_dxfAllocateInteractor (tdmInteractorWin W, int size)
{
  int i ;
  tdmInteractor I = (tdmInteractor) 0 ;

  ENTRY(("_dxfAllocateInteractor(0x%x, %d)", W, size));

  if (! W) goto error ;

  if (W->numUsed == W->numAllocated)
      /* create more interactors */
      I = _allocateMoreInteractors(W) ;
  else
      /* find an unused interactor */
      for (i = 0 ; i < W->numAllocated ; i++)
          if (! IS_USED(W->Interactors[i]))
            {
              I = W->Interactors[i] ;
              break ;
            }

  if (! I) goto error ;
  
  if (size) {
      /* allocate interactor private data */
      if (! (PRIVATE(I) = tdmAllocateLocal(size))) {
          goto error ;
      } else {
          bzero ((char *) PRIVATE(I), size) ;
      }
  }

  WINDOW(I) = W ;
  AUX(I) = (tdmInteractor) 0 ;
  IS_AUX(I) = 0 ;
  IS_GROUP(I) = 0 ;
  IS_USED(I) = 1 ;
  W->numUsed++ ;

  /*
   * Default event mask
   */
  I->eventMask =  DXEVENT_LEFT | DXEVENT_MIDDLE | DXEVENT_RIGHT;

  EXIT(("I = 0x%x", I));
  return I ;

 error:

  EXIT(("ERROR"));
  return (tdmInteractor) 0 ;
}

void
_dxfDeallocateInteractor (tdmInteractor interactor)
{
  tdmInteractorWin W ;

  ENTRY(("_dxfDeallocateInteractor(0x%x)", interactor));

  if (! interactor || ! (W = WINDOW(interactor)))
    {
      EXIT(("ERROR"));
      return ;
    }
      
  if (PRIVATE(interactor))
      tdmFree((void *) PRIVATE(interactor)) ;

  bzero ((char *) interactor, sizeof(tdmInteractorT)) ;
  IS_USED(interactor) = 0 ;
  W->numUsed-- ;

  EXIT((""));
}
          

/*
 *  Here is a picture of some of the lower level details implemented below.
 *  Boxes represent contiguous arrays.  A downward arrow is a pointer to
 *  the first element of a contiguous array.  Left arrows are pointers to
 *  interactor objects.
 *
 *      W.extents                                 W.Interactors
 *          |                                           |
 *          v                                           |
 *  +---------------+                                   |
 *  |InteractorArray|------------------------.          |
 *  |InteractorArray|-------------.          |          |
 *  |       .       |             |          v          v   
 *  |       .       |             |    +----------+   +---+ 
 *  |InteractorArray|--.          |    |interactor|<--|ptr| -+        
 *  +---------------+  |          |    |interactor|<--|ptr|  |        
 *                     |          |    |     .    |<--| . |  ARRAYSIZE
 *                     |          |    |     .    |<--| . |  |        
 *                     |          v    |interactor|<--|ptr| -+        
 *                     |    +----------+----------+   |   |           
 *                     |    |interactor|<-------------|ptr| -+        
 *                     |    |interactor|<-------------|ptr|  |        
 *                     |    |     .    |<-------------| . |  ARRAYSIZE
 *                     |    |     .    |<-------------| . |  |        
 *                     v    |interactor|<-------------|ptr| -+        
 *               +----------+----------+              |   |           
 *               |interactor|<------------------------|ptr| -+        
 *               |interactor|<------------------------|ptr|  |        
 *               |     .    |<------------------------| . |  ARRAYSIZE
 *               |     .    |<------------------------| . |  |        
 *               |interactor|<------------------------|ptr| -+        
 *               +----------+                         +---+
 */

static tdmInteractor
_allocateMoreInteractors (tdmInteractorWin W)
{
  int i, j ;
  tdmInteractor newArray, *tmp ;

  ENTRY(("_allocateMoreInteractors(0x%x)", W));

  /* allocate ARRAYSIZE more interactor pointers */
  tmp = (tdmInteractor *)
        tdmAllocateLocal((W->numUsed + ARRAYSIZE) * sizeof(tdmInteractor)) ;

  if (! tmp) goto error ;

  /* allocate a new storage array with space for ARRAYSIZE interactors */
  newArray = _allocateInteractorArray(W) ;

  if (! newArray)
    {
      tdmFree((void *) tmp) ;
      goto error ;
    }
  else
      W->numAllocated = W->numUsed + ARRAYSIZE ;
  
  /* copy exiting pointers */
  for (i = 0 ; i < W->numUsed ; i++)
      tmp[i] = W->Interactors[i] ;

  /* init new pointers */
  for (i = W->numUsed, j = 0 ; i < W->numAllocated ; i++, j++)
      tmp[i] = &newArray[j] ;

  if (W->Interactors) tdmFree((void *) W->Interactors) ;
  W->Interactors = tmp ;

  EXIT(("OK"));
  return W->Interactors[W->numUsed] ;

 error:

  EXIT(("ERROR"));
  return (tdmInteractor) 0 ;
}

static tdmInteractor
_allocateInteractorArray (tdmInteractorWin W)
{
  InteractorArray *tmp ;
  int i, numInteractorArrays ;

  ENTRY(("_allocateInteractorArray(0x%x)", W));
  
  numInteractorArrays = W->numAllocated / ARRAYSIZE ;

  /* get a bigger array of array pointers */
  tmp = (InteractorArray *)
        tdmAllocateLocal((numInteractorArrays + 1) * sizeof(InteractorArray)) ;

  if (! tmp) {
    EXIT(("ERROR"));
    return (tdmInteractor) 0 ;
  }
  
  /* allocate a new InteractorArray of size ARRAYSIZE */
  tmp[numInteractorArrays] =
      (InteractorArray) tdmAllocateLocal(ARRAYSIZE * sizeof(tdmInteractorT)) ;

  if (! tmp[numInteractorArrays])
    {
      tdmFree((void *) tmp) ;
      EXIT(("ERROR"));
      return (tdmInteractor) 0 ;
    }
  else
      bzero ((char *) tmp[numInteractorArrays],
             ARRAYSIZE * sizeof(tdmInteractorT)) ;
  
  /* copy existing array pointers */
  for (i = 0 ; i < numInteractorArrays ; i++)
      tmp[i] = ((InteractorArray *) W->extents)[i] ;

  /* free old array pointer array */
  if (W->extents) tdmFree(W->extents) ;

  /* copy new */
  W->extents = (void *) tmp ;

  EXIT((""));
  return *tmp[numInteractorArrays] ;
}


/*
 *  Camera stack utils:
 *
 *  Each window contains two stacks: the undo stack (W->Cam) and the redo
 *  stack (W->redo).  The top of the undo stack is the current camera in
 *  use.  When camera info is popped off the undo stack, it is pushed onto
 *  the redo stack and vice versa.
 *
 *  Note that these stacks are entirely separate from the stack managed by
 *  the routines in hwMatrix.c.  The latter routines are used to maintain
 *  the state of the transformations used during application data
 *  structure traversal, while the routines in this file maintain a
 *  history of view transforms and other important information. 
 *
 *  The undo and redo stacks are limited in size to STACKLIMIT, which if
 *  exceeded frees the bottom stack entry.
 */

#define STACKLIMIT 10

static int
_PushInteractorCamera (tdmInteractorCam *stack, tdmInteractorCam new)
{
  register int i ;
  register tdmInteractorCam p ;

  ENTRY(("_PushInteractorCamera(0x%x, 0x%x)", stack, new));

  if (! stack)
    {
      EXIT(("bad camera stack reference"));
      return 0 ;
    }

  /* Chop tail.  This is crude.  Not worth improving for small stack limit. */
  for (p= *stack, i=0 ; p && i<STACKLIMIT-1 ; p=p->next, i++) ;
  if (p && p->next)
    {
      tdmFree((void *)p->next) ;
      p->next = (tdmInteractorCam) 0 ;
    }

  if (! new) {
      /* allocate new camera */
      if ( (new = (tdmInteractorCam) tdmAllocateLocal(sizeof(tdmInteractorCamT))) ) {
          if (*stack) {
              /* copy current top */
              memcpy ((void *)new, (void *)*stack, sizeof(tdmInteractorCamT)) ;
          } else {
              /* init new camera */
              bzero ((char *)new, sizeof(tdmInteractorCamT)) ;
          }
      } else {
          EXIT(("out of memory"));
          return 0 ;
      }
  }

  new->next = *stack ;
  *stack = new ;

  EXIT(("OK"));
  return 1 ;
}

int 
_dxfPushInteractorCamera (tdmInteractorWin W)
{

  ENTRY(("_dxfPushInteractorCamera(0x%x)", W));

  if (! W)
    {
      EXIT(("bad window"));
      return 0 ;
    }
  if (_PushInteractorCamera (&W->Cam, 0))
    {
      EXIT(("OK"));
      return 1 ;
    }
  else
    {
      EXIT(("error"));
      return 0 ;
    }
}

static int
_MoveInteractorCamera (tdmInteractorCam *dest, tdmInteractorCam *source)
{
  /* pop source stack and push onto destination stack */
  tdmInteractorCam top ;

  ENTRY(("_MoveInteractorCamera(0x%x, 0x%x)", dest, source));
  
  if (!source || !*source)
    {
      EXIT(("empty source stack"));
      return 0 ;
    }

  top = *source ;
  *source = top->next ;
  top->next = 0 ;
  if (*source)
    (*source)->view_state++ ;

  if (! _PushInteractorCamera (dest, top))
    {
      EXIT(("ERROR"));
      return 0 ;
    }

  EXIT(("OK"));
  return 1 ;
}

int
_dxfPopInteractorCamera (tdmInteractorWin W)
{
  ENTRY(("_dxfPopInteractorCamera(0x%x)", W));
  
  /* there must always be at least one camera in W->Cam stack */
  if (! W->Cam->next)
    {
      EXIT(("tried to pop last camera"));
      return 0 ;
    }

  /* pop current camera stack and push onto redo stack */
  if (! _MoveInteractorCamera (&W->redo, &W->Cam))
    {
      EXIT(("ERROR"));
      return 0 ;
    }
  
  /* indicate if OK to pop further */
  if (W->Cam->next)
    {
      EXIT(("OK"));
      return 1 ;
    }
  else
    {
      EXIT(("last undoable camera"));
      return -1 ;
    }
}
  
int
_dxfRedoInteractorCamera (tdmInteractorWin W)
{
  ENTRY(("_dxfRedoInteractorCamera(0x%x)", W));

  /* pop redo stack and push onto current camera stack */
  if (! _MoveInteractorCamera (&W->Cam, &W->redo))
    {
      EXIT(("ERROR"));
      return 0 ;
    }
  
  /* indicate if anything left on redo stack */
  if (W->redo)
    {
      EXIT((""));
      return 1 ;
    }
  else
    {
      EXIT(("popped last camera"));
      return -1 ;
    }
}
  

/*
 *  General interactor utils
 */

void
_dxfRedrawInteractorEchos (tdmInteractorWin W)
{
  int i ;

  /* redraw all appropriate interactor echos in this window */
  ENTRY(("_dxfRedrawInteractorEchos(0x%x)", W));

  if (! W)
    {
      EXIT(("bad window"));
      return ;
    }

  for (i = 0 ; i < W->numAllocated ; i++)
      if (IS_USED(W->Interactors[i]) &&
	  !IS_AUX(W->Interactors[i]) && !IS_GROUP(W->Interactors[i]))
          tdmResumeEcho (W->Interactors[i], tdmBothBufferDraw) ;

  EXIT((""));
  return ;
}

void
_dxfAssociateInteractorEcho (tdmInteractor T, tdmInteractor A)
{
  ENTRY(("_dxfAssociateInteractorEcho(0x%x, 0x%x)", T, A));

  /*
   *  Associate the echo of A to T.  An associated interactor need not be
   *  directly manipulated since it is driven by the interactor it is
   *  associated with.
   *
   *  Associations are stored in a list.  AUX(T) is the head of the list
   *  for interactor T.  If T is a rotation interactor and is redrawn with
   *  tdmResumeEcho(T), it first checks to see if it is visible, redraws
   *  itself, and then calls tdmResumeEcho(AUX(T)).  AUX(T) may also be
   *  the head of its own sublist, in which case it calls
   *  tdmResumeEcho(AUX(AUX(T))) recursively.
   *
   *  This is not a general facility.  One major restriction is that if an
   *  interactor uses either the globe or the gnomon as its echo, then all
   *  its associated interactors succeeding it in the list must also use
   *  echo either globes or gnomons.  This is because configuring the
   *  frame buffer for a globe or gnomon is something that can only be
   *  done once per redraw.
   */

  if (T) AUX(T) = A ;
  if (A) IS_AUX(A) = 1 ;

  EXIT((""));
}

void
_dxfDisassociateInteractorEcho (tdmInteractorWin W, tdmInteractor A)
{
  int i ;

  ENTRY(("_dxfDisassociateInteractorEcho(0x%x, 0x%x)", W, A));

  /* break all associations with aux interactor A */
  if (!W || !A || !IS_AUX(A)) {
    EXIT((""));
    return ;
  }

  for (i = 0 ; i < W->numAllocated ; i++)
      if (IS_USED(W->Interactors[i]) && AUX(W->Interactors[i]) == A)
	  AUX(W->Interactors[i]) = (tdmInteractor) 0 ;

  IS_AUX(A) = 0 ;

  EXIT((""));
}

void
_dxfSetInteractorBox (tdmInteractorWin W, float box[8][3])
{
  int i ;
  tdmInteractorCam C ;

  ENTRY(("_dxfSetInteractorBox(0x%x, 0x%x)", W, box));

  /* set up global box for all cursors in this window */
  if (!W || !(C = W->Cam)) goto error ;

  for (i = 0 ; i < 8 ; i++)
    {
      VPRINT(box[i]);
      C->box[i][0] = box[i][0] ;
      C->box[i][1] = box[i][1] ;
      C->box[i][2] = box[i][2] ;
    }

  C->box_state++ ;
  EXIT((""));
  return ;

 error:

  EXIT(("error"));
  return;
}

void
_dxfSetInteractorViewInfo (tdmInteractorWin W,
                          float *f, float *t, float *u, float dist,
                          float fov, float width, float aspect, int proj,
			  float Near, float Far)
{
  tdmInteractorCam C ;

  ENTRY(("_dxfSetInteractorViewInfo"
	 "(0x%x, 0x%x, 0x%x, 0x%x, %f, %f, %f, %f, %d, %f, %f)",
	 W, f, t, u, dist, fov, width, aspect, proj, Near, Far));

  if (!W || !(C = W->Cam)) goto error ;

  C->from[0] = f[0] ; C->from[1] = f[1] ; C->from[2] = f[2] ;
  C->  to[0] = t[0] ; C->  to[1] = t[1] ; C->  to[2] = t[2] ;
  C->  up[0] = u[0] ; C->  up[1] = u[1] ; C->  up[2] = u[2] ;

  if (dist != 0)
      C->vdist = dist ;
  else
      C->vdist = (float) sqrt ((double)((f[0]-t[0])*(f[0]-t[0]) +
                                        (f[1]-t[1])*(f[1]-t[1]) +
                                        (f[2]-t[2])*(f[2]-t[2]))) ;

  /*  !!!!!!! This is very questionable coding style if it's correct !!!!! */
  if ( (C->projection = proj) )
    {
      /* perspective */
      C->fov = fov ;
      PRINT(("perspective projection fov %f", fov));
    }
  else
    {
      /* orthogonal width in world coordinates */
      C->width = width ;
      PRINT(("orthogonal projection width %f", width));
    }

  C->aspect = aspect ;
  C->view_coords_set = 1 ;
  C->Near = Near ;
  C->Far = Far ;
  C->view_state++ ;

  PRINT(("look-to point:"));
  VPRINT(C->to);
  PRINT(("look-from point, dist %f", C->vdist));
  VPRINT(C->from) ;
  PRINT(("up vector, length %f", LENGTH(C->up)));
  VPRINT(C->up) ;
  PRINT(("Near clip: %f", C->Near));
  PRINT((" Far clip: %f", C->Far));
  EXIT((""));
  return ;

 error:
  EXIT(("ERROR"));
  return;
}

void
_dxfSetInteractorWindowSize (tdmInteractorWin W, int width, int height)
{
  tdmInteractorCam C ;

  ENTRY(("_dxfSetInteractorWindowSize(0x%x, %d, %d)", W, width, height));

  /* record size of this window in pixels */
  if (!W || !(C = W->Cam)) goto error ;

  C->w = width ; C->h = height ;
  C->aspect = (float)height/(float)width ;
  PRINT(("width: %d height: %d aspect: %f", C->w, C->h, C->aspect));

  _dxfInteractorViewChanged(W) ;
  EXIT((""));
  return ;

 error:
  EXIT(("ERROR"));
  return;
}

void
_dxfSetInteractorViewDepth (tdmInteractorWin W, int depth)
{
  tdmInteractorCam C ;

  ENTRY(("_dxfSetInteractorViewDepth(0x%x, %d)", W, depth));

  /* record range of Z buffer for this window */
  if (!W || !(C = W->Cam)) goto error ;

  C->d = depth ;
  PRINT(("depth: %d", C->d));

  _dxfInteractorViewChanged(W) ;
  EXIT((""));
  return ;

 error:
  EXIT(("ERROR"));
  return;
}

void
_dxfGetInteractorViewInfo (tdmInteractorWin W,
                          float *f, float *t, float *u, float *dist,
                          float *fov, float *width, float *aspect,
			  int *projection, float *Near, float *Far,
			  int *pixwidth)
{
  tdmInteractorCam C ;

  ENTRY(("_dxfGetInteractorViewInfo(0x%x, 0x%x, 0x%x, 0x%x, 0x%x, "
	 "0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x)",
	 W, f, t, u, dist, fov, width, aspect,
	 projection, Near, Far, pixwidth));
  
  if (!W || !(C = W->Cam) || !C->view_coords_set) goto error ;

  f[0] = C->from[0] ; f[1] = C->from[1] ; f[2] = C->from[2] ;
  t[0] = C->  to[0] ; t[1] = C->  to[1] ; t[2] = C->  to[2] ;
  u[0] = C->  up[0] ; u[1] = C->  up[1] ; u[2] = C->  up[2] ;

  /* !!!!! should we check for NULL?? */
  *dist = C->vdist ;
  *fov = C->fov ;
  *width = C->width ;
  *aspect = C->aspect ;
  *projection = C->projection ;
  *Near = C->Near ;
  *Far = C->Far ;
  *pixwidth = C->w ;
  
  PRINT(("look-to point"));
  VPRINT(C->to) ;
  PRINT(("look-from point"));
  VPRINT(C->from) ;
  PRINT(("to-from dist: %12f" , C->vdist));
  PRINT(("up vector, length %f", LENGTH(C->up)));
  VPRINT(C->up) ;
  PRINT(("width in pixels: %d aspect: %f", C->w, C->aspect));
  PRINT(("%s", C->projection ? "perspective fov " : "ortho width "));
  PRINT(("%f", C->projection ? C->fov : C->width));
  PRINT(("Near clip: %f Far clip: %f", C->Near, C->Far)); 

  EXIT((""));
  return ;

 error:
  EXIT(("ERROR"));
  return;
}

void
_dxfInteractorViewChanged(tdmInteractorWin W)
{
  tdmInteractorCam C ;

  ENTRY(("_dxfInteractorViewChanged(0x%x)", W));

  if (!W || !(C = W->Cam)) goto error ;

  C->view_state++ ;

  EXIT((""));
  return ;

 error:
  EXIT(("ERROR"));
  return;
}

void
_dxfSetInteractorImage (tdmInteractorWin W, int w, int h, void *image)
{
  tdmInteractorCam C ;

  ENTRY(("_dxfSetInteractorImage(0x%x, %d, %d, 0x%x)", W, w, h, image));

  /* copy pointer to application window background image */
  if (!W || !(C = W->Cam)) goto error ;

  C->iw = w ;
  C->ih = h ;
  C->image = image ;

  EXIT((""));
  return ;

 error:
  EXIT(("ERROR"));
  return;
}

void
_dxfSetCursorSpeed (tdmInteractorWin W, int speed)
{
  tdmInteractorCam C ;

  ENTRY(("_dxfSetCursorSpeed(0x%x, %d)", W, speed));

  /* set speed global to all cursors in this window */
  if (!W || !(C = W->Cam)) goto error ;

  C->cursor_speed = speed ;

  EXIT((""));
  return ;

 error:
  EXIT(("ERROR"));
  return;
}

void
_dxfSetCursorConstraint (tdmInteractorWin W, int constraint)
{
  tdmInteractorCam C ;

  ENTRY(("_dxfSetCursorConstraint(0x%x, %d)", W, constraint));

  /* set constraints global to all cursors in this window */
  if (!W || !(C = W->Cam)) goto error ;

  C->cursor_constraint = constraint ;
  _dxfRedrawInteractorEchos(W) ;

  EXIT((""));
  return ;

 error:
  EXIT(("ERROR"));
  return;
}

/*
 *  Null functions for interactor implementations to use.
 */

void
_dxfNullDoubleClick (tdmInteractor I, int x, int y, tdmInteractorReturn *R)
{
  R->change = 0 ;
}

void
_dxfNullStartStroke (tdmInteractor I, int x, int y, int btn, int s)
{
}

void 
_dxfNullStrokePoint (tdmInteractor I, int a, int b, int type, int s)
{
}

void
_dxfNullEndStroke (tdmInteractor I, tdmInteractorReturn *R)
{
  R->change = 0 ;
}

void 
_dxfNullResumeEcho (tdmInteractor I, tdmInteractorRedrawMode redrawMode)
{
}

void
_dxfNullKeyStruck(tdmInteractor I, int x, int y, char c, int s)
{
}
