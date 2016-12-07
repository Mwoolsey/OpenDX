/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


/*---------------------------------------------------------------------------*\
 $Source: /src/master/dx/src/exec/hwrender/hwGroupInteractor.c,v $
  Author: Mark Hood

  A Group interactor is an interactor that mutates itself into a copy of
  one of three other interactors, depending upon which mouse button is used
  when starting a stroke.

  There is no echo associated with a group interactor.  To refresh a group
  interactor's display, its component interactors' echo methods must be
  invoked explicitly.

\*---------------------------------------------------------------------------*/

#include <stdio.h>
#include <math.h>

#include "hwDeclarations.h"

#if !defined(DX_NATIVE_WINDOWS)
#ifdef STANDALONE
#include <X11/Xlib.h>
#else
#include "hwMemory.h"
#include <X11/Xlib.h>
#endif
#endif

#include "hwPortLayer.h"

#include "hwDebug.h"

/*
 *  Forward references
 */

static void StartStroke (tdmInteractor I, int x, int y, int btn, int s) ;
static void Destroy (tdmInteractor I) ;

/*
 *  Null functions
 */
static void 
NullStrokePoint (tdmInteractor I, int a, int b, int type, int s)
{
}

static void
NullEndStroke (tdmInteractor I, tdmInteractorReturnP R)
{
  R->change = 0 ;
}

static void
NullDoubleClick (tdmInteractor I, int x, int y, tdmInteractorReturnP R)
{
  R->change = 0 ;
}

static void
NullResumeEcho (tdmInteractor I, tdmInteractorRedrawMode redraw_mode)
{
}


tdmInteractor
_dxfCreateInteractorGroup (tdmInteractorWin W,
			   tdmInteractor btn1,
			   tdmInteractor btn2,
			   tdmInteractor btn3) 
{
  /*
   *  Initialize and return a handle to an interactor group.  
   */

  register tdmInteractor I ;

  ENTRY(("_dxfCreateInteractorGroup(0x%x, 0x%x, 0x%x, 0x%x)",
	 W, btn1, btn2, btn3));

  if (W && (I = _dxfAllocateInteractor(W, 0)))
    {
      /* instance initial interactor methods */
      FUNC(I, StartStroke) = StartStroke ;
      FUNC(I, StrokePoint) = NullStrokePoint ;
      FUNC(I, EndStroke) = NullEndStroke ;
      FUNC(I, DoubleClick) = NullDoubleClick ;
      FUNC(I, ResumeEcho) = NullResumeEcho ;
      FUNC(I, Destroy) = Destroy ;
      FUNC(I, KeyStruck) = _dxfNullKeyStruck;

      
      /* init data */
      BTN1(I) = btn1 ;
      BTN2(I) = btn2 ;
      BTN3(I) = btn3 ;
      IS_GROUP(I) = 1 ;

      EXIT(("I = 0x%x", I));
      return I ;
    }
  else
    {
      EXIT(("ERROR"));
      return 0 ;
    }
}


void
_dxfUpdateInteractorGroup (tdmInteractor I,
			  tdmInteractor btn1,
			  tdmInteractor btn2,
			  tdmInteractor btn3) 
{
  ENTRY(("_dxfUpdateInteractorGroup(0x%x, 0x%x, 0x%x, 0x%x)",
	 I, btn1, btn2, btn3));
  
  if (I)
    {
      BTN1(I) = btn1 ;
      BTN2(I) = btn2 ;
      BTN3(I) = btn3 ;
      EXIT((""));
    }
  else
    EXIT(("ERROR: bad group"));
}
      

static void 
StartStroke (tdmInteractor I, int x, int y, int btn, int s)
{
  tdmInteractor btn1, btn2, btn3 ;

  ENTRY(("StartStroke(0x%x, %d, %d, %d)",
	 I, x, y, btn));
  
  /* save button interactors */
  btn1 = BTN1(I) ;
  btn2 = BTN2(I) ;
  btn3 = BTN3(I) ;

  /* copy interactor info */
  switch(btn)
    {
    case 1:
      *I = *BTN1(I) ;
      break ;
    case 2:
      *I = *BTN2(I) ;
      break ;
    default:
      *I = *BTN3(I) ;
      break ;
    }

  /* call new start stroke method */
  tdmStartStroke (I, x, y, btn, s) ;

  /* restore group info */
  BTN1(I) = btn1 ;
  BTN2(I) = btn2 ;
  BTN3(I) = btn3 ;
  IS_GROUP(I) = 1 ;

  FUNC(I, StartStroke) = StartStroke ;
  FUNC(I, Destroy) = Destroy ;

  EXIT((""));
}

static void
Destroy (tdmInteractor I)
{
  ENTRY(("Destroy(0x%x)", I));
  
  if (!I) {
    EXIT(("I = NULL"));
    return ;
  }

  PRIVATE(I) = (void *)0 ;
  _dxfDeallocateInteractor(I) ;

  EXIT((""));
}
