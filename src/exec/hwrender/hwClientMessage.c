/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


/*---------------------------------------------------------------------------*\
 $Source: /src/master/dx/src/exec/hwrender/hwClientMessage.c,v $

  This file contains functions implementing the client message protocol for
  communicating with other DX processes.  Most of the direct interactor
  management is handled here, since they are created and restored in
  response to messages from the UI process.

  The primary caller of these functions is _dxfProcessEvents().  No calls to
  the graphics library are made directly at this level.

\*---------------------------------------------------------------------------*/

#include <stdio.h>
#include <math.h>

#include "hwDeclarations.h"
#include "hwClientMessage.h"
#include "hwWindow.h"
#if defined(DX_NATIVE_WINDOWS)
#include <windows.h>
#else
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#endif

#include "hwDebug.h"

#define COS05  0.996
#define SIN15  0.259
#define COS15  0.966
#define SIN25  0.423
#define COS25  0.906
#define SIN45  0.707
#define SIN35  0.574

struct atomRep _dxdtdmAtoms[MAX_ATOMS] = {{ None, NULL}};

extern Error _dxfDraw (void*, dxObject, Camera, int);
extern void _dxfCacheState(char *, dxObject, dxObject, int);

static void
_sendLinkCamera(tdmChildGlobalP globals,
  float *from, float *to, float *up,
  float width, float aspect, float fov, float dist,
  int projection, int pixwidth);

#if defined(DX_NATIVE_WINDOWS)
static int
internAtoms ()
#else
static int
internAtoms (Display *dpy)
#endif
{
  /*
   *  Intern UI/renderer protocol elements into the X server to allow the
   *  two processes to understand each other.
   */

  ENTRY(("internAtoms(0x%x)",dpy));

/*
 * IMPORTANT!  Do not change the names of these defines, they are both the
 * define and the *string* that is shared with the UI. Changing the name
 * will make the atom unrecognizable by the UI.
 */

  INTERNATOM(GLWindow0);		/* Camera0 */
  INTERNATOM(GLWindow1);		/* Camera1 */
  INTERNATOM(GLWindow2);		/* Camera2 */
  INTERNATOM(GLWindow2Execute);		/* Camera2Execute */
  INTERNATOM(StartRotateInteraction);
  INTERNATOM(FromPoint);
  INTERNATOM(UpVector);
  INTERNATOM(StartZoomInteraction);
  INTERNATOM(StartPanZoomInteraction);
  INTERNATOM(Zoom1);
  INTERNATOM(Zoom2);
  INTERNATOM(StartCursorInteraction);
  INTERNATOM(CursorChange);
  INTERNATOM(SetCursorConstraint);
  INTERNATOM(SetCursorSpeed);
  INTERNATOM(StartRoamInteraction);
  INTERNATOM(RoamPoint);
  INTERNATOM(StartNavigateInteraction);
  INTERNATOM(NavigateMotion);
  INTERNATOM(NavigatePivot);
  INTERNATOM(StopInteraction);
  INTERNATOM(DisplayGlobe);
  INTERNATOM(CameraUndoable);
  INTERNATOM(CameraRedoable);
  INTERNATOM(UndoCamera);
  INTERNATOM(RedoCamera);
  INTERNATOM(PushCamera);
  INTERNATOM(ButtonMapping1);
  INTERNATOM(ButtonMapping2);
  INTERNATOM(ButtonMapping3);
  INTERNATOM(NavigateLookAt);
  INTERNATOM(ExecuteOnChange);
  INTERNATOM(Set_View);
  INTERNATOM(GLDestroyWindow);
  INTERNATOM(GLShutdown);
  INTERNATOM(POKE_CONNECTION);
  INTERNATOM(StartPickInteraction);
  INTERNATOM(PickPoint);
  INTERNATOM(ImageReset);
  INTERNATOM(StartUserInteraction);

#if defined(DX_NATIVE_WINDOWS)
    INTERNATOM(XA_Integer);
#else
  /* Add X predefined Atoms to theDisplay *dpy) list of available atoms */
  _dxdtdmAtoms[XA_Integer].atom = XA_INTEGER;
  _dxdtdmAtoms[XA_Integer].spelling="XA_Integer";
#endif

  EXIT(("1"));
  return 1 ;
}

static char *
lookupAtomName (Atom a)
{
  /*
   *  Return the spelling of a given atom.  Used for debug.
   */

  int i, numAtoms ;

  /* ENTRY(("lookupAtomName(0x%x a)",)); */

  numAtoms = sizeof(_dxdtdmAtoms) / sizeof(struct atomRep) ;

  for (i = 0 ; i < numAtoms ; i++)
    if (_dxdtdmAtoms[i].atom == a){
      /* EXIT(("found")); */
      return _dxdtdmAtoms[i].spelling ;
    }

  /* EXIT(("not found")); */
  return "unknown atom" ;
}


#if defined(DX_NATIVE_WINDOWS)
void
_dxfSendClientMessage (HWND win, int atomIndex, tdmMessageDataP data)
{
    SendMessage(win, WM_CLIENT_MESSAGE, atomIndex, (void *)data);
}
#else

void
_dxfSendClientMessage
    (Display *dpy, Window win, int atomIndex, tdmMessageDataP data)
{
  /*
   *  Send a client message to the specified window.
   */
  
  XEvent event;
  Atom	atom = ATOM(atomIndex);

  ENTRY(("_dxfSendClientMessage(0x%x, 0x%x, %d, 0x%x)",
	 dpy, win, atomIndex, data));

  event.type = ClientMessage ;
  event.xclient.window = win ;
  event.xclient.format = 32 ;
  event.xclient.message_type = atom ;
  if (data)
    {
      event.xclient.data.l[0] = data->l[0] ;
      event.xclient.data.l[1] = data->l[1] ;
      event.xclient.data.l[2] = data->l[2] ;
      event.xclient.data.l[3] = data->l[3] ;
      event.xclient.data.l[4] = data->l[4] ;
    }

  switch (XSendEvent (dpy, win, True, NoEventMask, &event))
    {
    case 0:
      PRINT(("error: wire protocol conversion failed"));
      break ;
    case BadValue:
      PRINT(("error: BadValue"));
      break ;
    case BadWindow:
      PRINT(("error: BadWindow"));
      break ;
    default:
      PRINT(("atom: \"%s\"", lookupAtomName(atom)));
      break ;
    }
  XFlush(dpy) ;
  EXIT((""));
}
#endif

static void
_SendCamera (WinP win, float to[3], float up[3], float from[3],
	     float width, int pixwidth, float aspect, float fov,
	     int proj, int refresh)
{
  /*
   *  Send a complete camera to the UI process.
   */

  tdmMessageData data0, data1, data2 ;
  DEFWINDATA(win);

  ENTRY(("_SendCamera (0x%x, 0x%x, 0x%x, 0x%x, %f, %d, %f, %f, %d, %d)",
	 win, to, up, from, width, pixwidth, aspect, fov, proj, refresh));

#if !defined(DX_NATIVE_WINDOWS)
  data0.l[0] = MSG_WINDOW ; /* kludge: ought to be win.xid */
#endif
  data0.f[1] = to[0] ;
  data0.f[2] = to[1] ;
  data0.f[3] = to[2] ;
  PRINT(("look-to point:")); VPRINT(to);

  data0.f[4] = up[0] ;
  data1.f[0] = up[1] ;
  data1.f[1] = up[2] ;
  PRINT(("up vector:"));  VPRINT(up);

  data1.f[2] = from[0] ;
  data1.f[3] = from[1] ;
  data1.f[4] = from[2] ;
  PRINT(("look-from point:")); VPRINT(from);

  data2.f[0] = proj ? 30 : width ;
  PRINT(("width %f", data2.f[0]));

  data2.l[1] = pixwidth ;
  PRINT(("pixwidth %d", pixwidth));

  data2.f[2] = aspect ;
  PRINT(("aspect %f", aspect));

  data2.f[3] = proj ? RAD2DEG(2*atan(fov/2.0)) : 0 ;
  PRINT(("view angle %f", data2.f[3]));

  data2.l[4] = proj ;
  PRINT(("%s projection", proj ? "perspective" : "orthographic"));

#if defined(DX_NATIVE_WINDOWS)
  _dxfSendClientMessage(PARENT_WINDOW, Camera0, &data0) ;
  _dxfSendClientMessage(PARENT_WINDOW, Camera1, &data1) ;
#else
  _dxfSendClientMessage(DPY, PARENT_WINDOW, Camera0, &data0) ;
  _dxfSendClientMessage(DPY, PARENT_WINDOW, Camera1, &data1) ;
#endif

#ifdef DX_NATIVE_WINDOWS
  if (refresh)
      _dxfSendClientMessage(PARENT_WINDOW, Camera2Execute, &data2) ;
  else
      _dxfSendClientMessage(PARENT_WINDOW, Camera2, &data2) ;
#else
  if (refresh)
      _dxfSendClientMessage(DPY, PARENT_WINDOW, Camera2Execute, &data2) ;
  else
      _dxfSendClientMessage(DPY, PARENT_WINDOW, Camera2, &data2) ;
#endif

  EXIT((""));
}
  
int
_dxfInitCMProtocol (WinP win)
{
  /*
   *  The renderer process cannot receive client message events on the GL
   *  window (as of 8 October 1991).  We create an unmapped intermediate X
   *  window from which to select the client messages.
   */
  
  static int first_time = 1 ;
  DEFWINDATA(win) ;

  ENTRY(("_dxfInitCMProtocol(0x%x), win"));

#if defined(DX_NATIVE_WINDOWS)
  if (first_time && !internAtoms())
#else
  if (first_time && !internAtoms(DPY))
#endif
      goto error ;
  else
      first_time = 0 ;

#if !defined(DX_NATIVE_WINDOWS)
  MSG_WINDOW = XCreateSimpleWindow
      (DPY, DefaultRootWindow(DPY), 0, 0, 1, 1, 0, 0, 0) ;

  if (MSG_WINDOW)
      XSelectInput (DPY, MSG_WINDOW, 0) ;
  else
      DXErrorGoto (ERROR_INTERNAL, "Could not create ClientMessage window") ;
#endif

  EXIT(("1"));
  return 1 ;

 error:
  EXIT(("ERROR"));
  return 0 ;
}


void
_dxfConnectUI (WinP win)
{
  /*
   *  By invoking this routine, the UI is given the following information:
   *
   *  1) the window identifier to which to direct its client messages.
   *  2) the complete state of the current camera.
   *  3) that the renderer is ready for direct interaction.
   */
  
  Vector up ;
  dxMatrix rot ;
  Point to, from ;
  int pixwidth, pixheight ;
  float width, aspect, u[3], t[3], f[3] ;
  DEFWINDATA(win);

  ENTRY(("_dxfConnectUI (0x%x)",win));

  if (! CAMERA)
    {
      EXIT(("CAMERA == NULL"));
      return ;
    }

  DXGetView (CAMERA, &from, &to, &up) ;
  f[0] = from.x ; f[1] = from.y ; f[2] = from.z ;
  t[0] =   to.x ; t[1] =   to.y ; t[2] =   to.z ;

  rot = DXGetCameraRotation (CAMERA) ;
  u[0] = rot.A[0][1] ; u[1] = rot.A[1][1] ; u[2] = rot.A[2][1] ;

  DXGetCameraResolution (CAMERA, &pixwidth, &pixheight) ;

  if (DXGetOrthographic (CAMERA, &width, &aspect))
      _SendCamera (win, t, u, f, width, pixwidth, aspect, 0, 0, 0) ;
  else
    {
      float fov ;

      DXGetPerspective (CAMERA, &fov, &aspect) ;
      _SendCamera (win, t, u, f, 0, pixwidth, aspect, fov, 1, 0) ;
    }

  EXIT((""));
}
  
static void
EchoFunction (tdmInteractor I, void *data, float rot[4][4], int draw)
{
  DEFGLOBALDATA(data);
  DEFPORT(PORT_HANDLE) ;
  tdmChildGlobalP globals = _gdata; /* XXX should we define MACROS for accessing
				       globals ? */

  /*
   *  This function is used as the echo for the view and navigation
   *  interactors.
   */

  ENTRY(("EchoFunction (0x%x, 0x%x, 0x%x, %d)",I, data, rot, draw));

  if (draw)
    {
      /* draw button-down pass */
      _dxfDraw (globals,OBJECT,CAMERA,0);

      /* draw interactor echos in double buffer mode */
      if (globals->CrntInteractor == globals->CursorGroup)
	{
	  tdmResumeEcho(globals->Cursor3D, tdmViewEchoMode) ;
	}
      else if (globals->CrntInteractor == globals->RoamGroup)
	{
	  tdmResumeEcho(globals->Roamer, tdmViewEchoMode) ;
	}
      else if (/*globals->CrntInteractor == globals->Navigator ||*/
	       globals->CrntInteractor == globals->RotateGroup)
	{
	  tdmResumeEcho(globals->GnomonTwirl, tdmViewEchoMode) ;
	}
    }
  else
    {
      /* Do nothing */
    }

  EXIT((""));
}


void
_dxfInitDXInteractors (tdmChildGlobalP globals)
{
  DEFGLOBALDATA(globals) ;
  DEFPORT(PORT_HANDLE) ;
  tdmInteractorWin W ;

  ENTRY(("_dxfInitDXInteractors (0x%x)", globals));

  /* Turn off trace macros from this level down */
  /* Turn it on by exporting this variable */
  DEBUG_CALL ( if (!getenv("DXHW_DEBUG_DETAIL")) DEBUG_OFF(); );

  /* initialize interactor common data */
  W = INTERACTOR_DATA = 
      _dxfInitInteractors(PORT_HANDLE,MATRIX_STACK) ;

  /* create individual interactors */
  globals->User = _dxfCreateUserInteractor(W, EchoFunction, (void *)globals);
  globals->Zoomer = _dxfCreateZoomInteractor(W) ;
  globals->PanZoom = _dxfCreatePanZoomInteractor(W) ;
  globals->Cursor3D = _dxfCreateCursorInteractor(W, tdmProbeCursor) ;
  globals->Roamer = _dxfCreateCursorInteractor(W, tdmRoamCursor) ;
  globals->Pick = _dxfCreateCursorInteractor(W, tdmPickCursor) ;
  globals->GnomonRotate = _dxfCreateRotationInteractor
      (W, tdmGnomonEcho, tdmXYPlaneRoll);

  /* the Globe is always passive, and is driven by the Gnomon */
  globals->Globe = _dxfCreateRotationInteractor (W, tdmGlobeEcho, 
                                                 tdmXYPlaneRoll) ;
  _dxfAssociateInteractorEcho (globals->GnomonRotate, globals->Globe) ;
  
  /* the Z rotation interactor for Execute Once mode has no echo... */
  globals->GnomonTwirl = _dxfCreateRotationInteractor
      (W, tdmNullEcho, tdmZTwirl) ;
  /* ...it drives the GnomonRotate echo instead */
  _dxfAssociateInteractorEcho (globals->GnomonTwirl, globals->GnomonRotate) ;
  
  /* next interactors are used in Execute On Change mode */
  globals->Navigator = _dxfCreateNavigator
      (W, EchoFunction, (void *)globals) ;
  globals->ViewTwirl = _dxfCreateViewRotationInteractor
      (W, EchoFunction, tdmZTwirl, (void *)globals) ;
  globals->ViewRotate = _dxfCreateViewRotationInteractor
      (W, EchoFunction, tdmXYPlaneRoll, (void *)globals) ;

  /* bind interactors to specific mouse buttons, assuming Execute Once mode */
  globals->RotateGroup = _dxfCreateInteractorGroup
      (W, globals->GnomonRotate, globals->GnomonRotate, globals->GnomonTwirl) ;
  globals->CursorGroup = _dxfCreateInteractorGroup
      (W, globals->Cursor3D, globals->GnomonRotate, globals->GnomonTwirl) ;
  globals->RoamGroup = _dxfCreateInteractorGroup
      (W, globals->Roamer, globals->GnomonRotate, globals->GnomonTwirl) ;

  /* default is not to display globe */
  globals->displayGlobe = 0 ;
  if (DXUI)
    {
      /* if UI, initial state is Execute Once with no current interactor */
      globals->executeOnChange = 0 ;
      globals->CrntInteractor = (tdmInteractor) 0 ;
    }
  else
    {
      /* no UI, initial state is Execute On Change with default RotateGroup */
      globals->executeOnChange = 1 ;
      globals->CrntInteractor = globals->RotateGroup ;
      _dxfUpdateInteractorGroup (globals->RotateGroup,
	   globals->ViewRotate, globals->ViewRotate, globals->ViewTwirl) ;
    }

  DEBUG_ON();
  EXIT((""));
}


static int
switchInteractors(tdmChildGlobalP globals)
{
  /*
   *  This is a helper routine for _dxfReceiveClientMessage, used when
   *  switching interactor modes.
   *
   *  If the current interactor has a persistent echo, make it invisible.
   *  Making an interactor echo invisible, however, doesn't erase it; it
   *  merely means that it is not to be redrawn.
   *
   *  `refresh' is set to let the caller of _dxfReceiveClientMessage know
   *  that it is to refresh the image window to get rid of the persistent
   *  interactor echo.  Most echos are of this type.  Transient echos are
   *  only currently used for the zoom boxes of the Zoomer and PanZoom
   *  interactors. 
   *
   *  The Globe interactor is used only for its echo, which is oriented to
   *  reflect the current world coordinate axes whenever it is drawn.  The
   *  Gnomon has the same default behavior, except that it can be directly
   *  manipulated when in Execute Once mode.  The Globe is only displayed
   *  when the displayGlobe state is true.
   *
   *  The Globe and Gnomon can also be `associated' with other interactors
   *  to indicate their orientations; in this case they can only be
   *  redrawn under the control of the associated interactor.  Thus it is
   *  important to disassociate the Gnomon from the current interactor
   *  when switching modes, so that it is available to the new interactor
   *  mode.
   *
   *  Interactor associations are implemented by a list attached to each
   *  interactor.  Making an interactor invisible prevents any interactors
   *  after it in its association list from being drawn as well.
   *
   *  The echo of the ViewRotation interactor is implemented by the
   *  renderer itself.  This interactor also uses the Gnomon as a
   *  persistent echo in addition to the rendered view, but not in the
   *  `associated' mode, since all the Gnomon has to perform is its
   *  default behavior of rendering the world coordinate axes.
   */

  int refresh = 0 ;
  DEFGLOBALDATA(globals) ;

  ENTRY(("switchInteractors(0x%x)",globals));

  if (globals->CrntInteractor == globals->RotateGroup)
    {
      _dxfRotateInteractorInvisible(globals->GnomonTwirl) ;
      _dxfDisassociateInteractorEcho (INTERACTOR_DATA,
				      globals->GnomonTwirl) ;
      refresh = 1 ;
    }
  else if (globals->CrntInteractor == globals->CursorGroup)
    {
      _dxfCursorInteractorInvisible(globals->Cursor3D) ;
      _dxfRotateInteractorInvisible(globals->GnomonTwirl) ;
      _dxfDisassociateInteractorEcho (INTERACTOR_DATA,
				      globals->GnomonTwirl) ;
      refresh = 1 ;
    }
  else if (globals->CrntInteractor == globals->RoamGroup)
    {
      _dxfCursorInteractorInvisible(globals->Roamer) ;
      _dxfRotateInteractorInvisible(globals->GnomonTwirl) ;
      _dxfDisassociateInteractorEcho (INTERACTOR_DATA,
				      globals->GnomonTwirl) ;
      refresh = 1 ;
    }
  else if (globals->CrntInteractor == globals->Pick)
    {
      _dxfCursorInteractorInvisible(globals->Pick) ;
      refresh = 1 ;
    }
#if 0
  else if (globals->CrntInteractor == globals->Navigator)
    {
      _dxfRotateInteractorInvisible(globals->GnomonTwirl) ;
      refresh = 1 ;
    }
#endif

  EXIT(("refresh = %d",refresh));
  return refresh ;
}

#define INTERACTION_NONE	1
#define INTERACTION_ROTATE	2
#define INTERACTION_CURSORS	3
#define INTERACTION_ROAM	4
#define INTERACTION_PICK	5
#define INTERACTION_NAVIGATE	6
#define INTERACTION_ZOOM	7
#define INTERACTION_PANZOOM	8
#define INTERACTION_USER	9

void
_dxfSetInteractionMode(tdmChildGlobalP globals, int mode, dxObject args)
{
    switch(mode)
    {
        case INTERACTION_NONE:
	    globals->CrntInteractor = (tdmInteractor) 0 ;
	    break;
	
	case INTERACTION_ROTATE:
	    _dxfRotateInteractorVisible(globals->GnomonTwirl) ;
	    _dxfRotateInteractorVisible(globals->GnomonRotate) ;
      	    tdmResumeEcho (globals->GnomonTwirl, tdmBothBufferDraw) ;
      	    globals->CrntInteractor = globals->RotateGroup ;
	    break;


	case INTERACTION_CURSORS:
            _dxfCursorInteractorVisible(globals->Cursor3D) ;
            _dxfRotateInteractorVisible(globals->GnomonTwirl) ;
            _dxfRotateInteractorVisible(globals->GnomonRotate) ;
            _dxfAssociateInteractorEcho (globals->Cursor3D,
						globals->GnomonTwirl) ;
            tdmResumeEcho (globals->Cursor3D, tdmBothBufferDraw) ;
            globals->CrntInteractor = globals->CursorGroup ;
	    break;


	case INTERACTION_ROAM:
            _dxfCursorInteractorVisible(globals->Roamer) ;
            _dxfRotateInteractorVisible(globals->GnomonTwirl) ;
            _dxfRotateInteractorVisible(globals->GnomonRotate) ;
            _dxfAssociateInteractorEcho (globals->Roamer,
						globals->GnomonTwirl) ;
            tdmResumeEcho (globals->Roamer, tdmBothBufferDraw) ;
            globals->CrntInteractor = globals->RoamGroup ;
            break;

	case INTERACTION_PICK:
	    _dxfCursorInteractorVisible(globals->Pick) ;
            globals->CrntInteractor = globals->Pick ;
	    break;

	case INTERACTION_NAVIGATE:
	{
	    dxMatrix rot ;
            tdmMessageData data;
            float width, aspect, viewDirReturn[3], viewUpReturn[3] ;
	    DEFGLOBALDATA(globals);

            PRINT(("StartNavigateInteraction"));
            /* align with current view before starting new navigation session */
            rot = DXGetCameraRotation(CAMERA) ;
            _dxfSetNavigateLookAt (globals->Navigator, tdmLOOK_ALIGN, 0.0,
			          rot.A, viewDirReturn, viewUpReturn) ;
      
            if (DXGetOrthographic (CAMERA, &width, &aspect))
	      {
	        /* navigation switches to perspective, so push camera first */
	        tdmMessageData data ;
	        data.l[0] = _dxfPushInteractorCamera(INTERACTOR_DATA) ;
#if defined(DX_NATIVE_WINDOWS)
	        _dxfSendClientMessage (PARENT_WINDOW, CameraUndoable, &data) ;
#else
	        _dxfSendClientMessage (DPY, PARENT_WINDOW, CameraUndoable, &data) ;
#endif
      
	      }
      
            /* switch to Execute On Change mode */
            data.l[0] = 1 ;
#if defined(DX_NATIVE_WINDOWS)
            _dxfSendClientMessage (PARENT_WINDOW, ExecuteOnChange, &data) ;
#else
            _dxfSendClientMessage (DPY, PARENT_WINDOW, ExecuteOnChange, &data) ;
#endif
      
	    globals->CrntInteractor = globals->Navigator ;
	    break;
	}

	case INTERACTION_ZOOM:
	    globals->CrntInteractor = globals->Zoomer ;
	    break;

	case INTERACTION_PANZOOM:
	    globals->CrntInteractor = globals->PanZoom ;
	    break;

	case INTERACTION_USER:
	    globals->CrntInteractor = globals->User;
	    _dxfSetUserInteractorMode(globals->User, args);
	    break;

	default:
	    break;
    }
}

#if defined(DX_NATIVE_WINDOWS)
Error
_dxfReceiveClientMessage (tdmChildGlobalP globals,
			  UINT msg, void *data, int *NeedRefresh)
#else
Error
_dxfReceiveClientMessage (tdmChildGlobalP globals,
			  XClientMessageEvent *message, int *NeedRefresh)
#endif
{
  /*
   *  Process messages received from the UI process.  The bulk of these
   *  are requests to start and stop interaction modes, in which case the
   *  principal output is to set the pointer to the current interactor.
   *  This pointer is then used by _dxfProcessEvents to drive the
   *  interaction in response to mouse movement.
   *
   *  The other requests are mainly for setting view information.  The UI
   *  doesn't have access to some of the info upon which views depend, so
   *  the response here is send the UI the info it needs.
   */
  
  DEFGLOBALDATA(globals);
  tdmMessageData mess_data;
  tdmMessageDataP dataP = &mess_data;
  int pixwidth, projection ;
  float from[3], to[3], up[3], width, aspect, fov, dist, Near, Far ;


  ENTRY(("_dxfReceiveClientMessage (0x%x, 0x%x, 0x%x)",
	 globals, message, NeedRefresh));
  PRINT(("message_type = %d", message->message_type));

  /* get current view parameters */
  _dxfGetInteractorViewInfo (INTERACTOR_DATA,
		from, to, up, &dist, &fov, &width,
		&aspect, &projection, &Near, &Far,
		&pixwidth) ;

  /* Xlib.h has message.data.l defined as long's.  Fill in our expected
     format to avoid problems on arch's with 64 bit long's */

#if defined(DX_NATIVE_WINDOWS)
  dataP->l[0] = ((long *)data)[0];
  dataP->l[1] = ((long *)data)[1];
  dataP->l[2] = ((long *)data)[2];
  dataP->l[3] = ((long *)data)[3];
  dataP->l[4] = ((long *)data)[4];
#else
  dataP->l[0] = (int32)message->data.l[0];
  dataP->l[1] = (int32)message->data.l[1];
  dataP->l[2] = (int32)message->data.l[2];
  dataP->l[3] = (int32)message->data.l[3];
  dataP->l[4] = (int32)message->data.l[4];
#endif

#if defined(DX_NATIVE_WINDOWS)
#define COMPARE_MSGTYPE(t)	((msg) == (t))
#else
#define COMPARE_MSGTYPE(t)	(message->message_type == ATOM((t)))
#endif

  if (COMPARE_MSGTYPE(DisplayGlobe))
    {
      PRINT(("DisplayGlobe %d", dataP->l[0]));
      if (dataP->l[0] && !globals->displayGlobe)
	{
	  PRINT(("turning globe display on"));
	  _dxfRotateInteractorVisible(globals->Globe) ;

	  if (globals->CrntInteractor == globals->RoamGroup ||
#if 0
	      globals->CrntInteractor == globals->Navigator ||
#endif
	      globals->CrntInteractor == globals->RotateGroup ||
	      globals->CrntInteractor == globals->CursorGroup)
	      tdmResumeEcho (globals->Globe, tdmFrontBufferDraw) ;
	      
	}
      else if (!dataP->l[0] && globals->displayGlobe)
	{
	  PRINT(("turning globe display off"));
	  _dxfRotateInteractorInvisible(globals->Globe) ;
	  if (globals->CrntInteractor == globals->RoamGroup ||
#if 0
	      globals->CrntInteractor == globals->Navigator ||
#endif
	      globals->CrntInteractor == globals->RotateGroup ||
	      globals->CrntInteractor == globals->CursorGroup)
	      *NeedRefresh = 1 ;
	}
      globals->displayGlobe = dataP->l[0] ;
    }
  else if (COMPARE_MSGTYPE(ExecuteOnChange))
    {
      PRINT(("ExecuteOnChange %d", dataP->l[0]));
      if (dataP->l[0] && !globals->executeOnChange)
	{
	  PRINT(("switching to Execute On Change mode"));
	  _dxfUpdateInteractorGroup (globals->RoamGroup,
				    globals->Roamer,
				    globals->ViewRotate,
				    globals->ViewTwirl) ;
	  _dxfUpdateInteractorGroup (globals->CursorGroup,
				    globals->Cursor3D,
				    globals->ViewRotate,
				    globals->ViewTwirl) ;
	  _dxfUpdateInteractorGroup (globals->RotateGroup,
				    globals->ViewRotate,
				    globals->ViewRotate,
				    globals->ViewTwirl) ;
	}
      else if (!dataP->l[0] && globals->executeOnChange)
	{
	  PRINT(("switching to Execute Once mode"));
	  _dxfUpdateInteractorGroup (globals->RoamGroup,
				    globals->Roamer,
				    globals->GnomonRotate,
				    globals->GnomonTwirl) ;
	  _dxfUpdateInteractorGroup (globals->CursorGroup,
				    globals->Cursor3D,
				    globals->GnomonRotate,
				    globals->GnomonTwirl) ;
	  _dxfUpdateInteractorGroup (globals->RotateGroup,
				    globals->GnomonRotate,
				    globals->GnomonRotate,
				    globals->GnomonTwirl) ;
	}
      globals->executeOnChange = dataP->l[0] ;
    }
  else if (COMPARE_MSGTYPE(StopInteraction))
    {
      PRINT(("StopInteraction"));
      *NeedRefresh |= switchInteractors(globals) ;
       _dxfSetInteractionMode(globals, INTERACTION_NONE, NULL);
    }
  else if (COMPARE_MSGTYPE(StartRotateInteraction))
    {
      PRINT(("StartRotateInteraction"));
      *NeedRefresh |= switchInteractors(globals) ;
       _dxfSetInteractionMode(globals, INTERACTION_ROTATE, NULL);

    }
  else if (COMPARE_MSGTYPE(StartCursorInteraction))
    {
      PRINT(("StartCursorInteraction"));
      *NeedRefresh |= switchInteractors(globals) ;
       _dxfSetInteractionMode(globals, INTERACTION_CURSORS, NULL);
    }
  else if (COMPARE_MSGTYPE(StartRoamInteraction))
    {
      PRINT(("StartRoamInteraction"));
      *NeedRefresh |= switchInteractors(globals) ;
       _dxfSetInteractionMode(globals, INTERACTION_ROAM, NULL);
    }
  else if (COMPARE_MSGTYPE(StartPickInteraction))
    {
      PRINT(("StartPickInteraction"));
      *NeedRefresh |= switchInteractors(globals) ;
       _dxfSetInteractionMode(globals, INTERACTION_PICK, NULL);
    }
  else if (COMPARE_MSGTYPE(SetCursorConstraint))
    {
      /* set global cursor constraints (applies to Roamer and Cursor3D) */
      PRINT(("SetCursorConstraint"));
      _dxfSetCursorConstraint (INTERACTOR_DATA, dataP->l[0]) ;
    }
  else if (COMPARE_MSGTYPE(SetCursorSpeed))
    {
      /* set global cursor speed (applies to Roamer and Cursor3D) */
      PRINT(("SetCursorSpeed"));
      _dxfSetCursorSpeed (INTERACTOR_DATA, dataP->l[0]) ;
    }
  else if (COMPARE_MSGTYPE(CursorChange))
    {
      /*
       *  This is the UI's handle for moving, deleting, and adding
       *  cursors.  Applies only to the cursor interactor.  Cursors are
       *  usually manipulated by the user, of course, in which case the
       *  info is sent back to the UI in _dxfSendInteractorData.
       */
      PRINT(("CursorChange"));
      _dxfCursorInteractorChange (globals->Cursor3D, dataP->l[0], dataP->l[1],
				  dataP->f[2], dataP->f[3], dataP->f[4]) ;
    }
  else if (COMPARE_MSGTYPE(StartNavigateInteraction))
    {
      /*
       *  Start navigation mode.  Align the current navigation vectors with
       *  the current view before entering this mode.  
       */
      
      *NeedRefresh |= switchInteractors(globals) ;
      _dxfSetInteractionMode(globals, INTERACTION_NAVIGATE, NULL);
    }
  else if (COMPARE_MSGTYPE(NavigateLookAt))
    {
      /*
       *  Generate a new view relative to current navigation vectors.  The
       *  new direction is specified in the first long of the message packet
       *  followed by an angle.  Send a complete new camera back to the UI.
       */

      dxMatrix rot ;
      Vector up ;
      Point from, to ;
      double dist ;
      int pixwidth, pixheight ;
      float viewDirReturn[3], viewUpReturn[3], f[3], t[3];
      tdmMessageData data;

      PRINT(("NavigateLookAt"));

      /* push camera before changing */
      data.l[0] = _dxfPushInteractorCamera(INTERACTOR_DATA) ;
#if defined(DX_NATIVE_WINDOWS)
      _dxfSendClientMessage (PARENT_WINDOW, CameraUndoable, &data) ;
#else
      _dxfSendClientMessage (DPY, PARENT_WINDOW, CameraUndoable, &data) ;
#endif

      /*
       *  Provide current view to _dxfSetNavigateLookAt, which then returns
       *  new view direction and view up vectors.
       */

      rot = DXGetCameraRotation(CAMERA) ;
      _dxfSetNavigateLookAt (globals->Navigator, dataP->l[0], dataP->f[1],
			    rot.A, viewDirReturn, viewUpReturn) ;
  
      /* compute new `to' point based on from-to distance and view direction */
      DXGetView (CAMERA, &from, &to, &up) ;
      dist = DXLength(DXSub(from, to)) ;
      f[0] = from.x ; t[0] = f[0] + dist*viewDirReturn[0] ;
      f[1] = from.y ; t[1] = f[1] + dist*viewDirReturn[1] ;
      f[2] = from.z ; t[2] = f[2] + dist*viewDirReturn[2] ;
      
      /* get other info that UI needs to complete camera */
      DXGetCameraResolution (CAMERA, &pixwidth, &pixheight) ;

      if (DXGetOrthographic (CAMERA, &width, &aspect))
	  /* force perspective projection */
	  fov = 2.0 * atan((width/dist) / 2.0) ;
      else
	  DXGetPerspective (CAMERA, &fov, &aspect) ;

      /* send camera info to UI */
      _SendCamera (LWIN, t, viewUpReturn, f, 0, pixwidth,
		   aspect, fov, 1, 1) ;
    
      if (LINK)
	    _sendLinkCamera(globals, f, t, viewUpReturn, width, aspect, fov, dist,
					      projection, pixwidth);
    }
  else if (COMPARE_MSGTYPE(NavigateMotion))
    {
      PRINT(("NavigateMotion"));
      _dxfSetNavigateTranslateSpeed (globals->Navigator, dataP->f[0]) ;
    }
  else if (COMPARE_MSGTYPE(NavigatePivot))
    {
      PRINT(("NavigatePivot"));
      _dxfSetNavigateRotateSpeed (globals->Navigator, dataP->f[0]) ;
    }
  else if (COMPARE_MSGTYPE(StartZoomInteraction))
    {
      PRINT(("StartZoomInteraction"));
      *NeedRefresh |= switchInteractors(globals) ;
      _dxfSetInteractionMode(globals, INTERACTION_ZOOM, NULL);
    }
  else if (COMPARE_MSGTYPE(StartPanZoomInteraction))
    {
      PRINT(("StartPanZoomInteraction"));
      *NeedRefresh |= switchInteractors(globals) ;
      _dxfSetInteractionMode(globals, INTERACTION_PANZOOM, NULL);
    }
  else if (COMPARE_MSGTYPE(ImageReset))
    {
      /* reset interactors that require it */
      SAVE_BUF_VALID = FALSE;
      PRINT(("ImageReset"));
      if (globals->CrntInteractor == globals->RoamGroup)
      {
	  _dxfResetCursorInteractor(globals->Roamer) ;
	  printf("_dxfResetCursorInteractor(globals->Roamer);\n");
      }
      else if (globals->CrntInteractor == globals->Navigator)
      {
	  _dxfResetNavigateInteractor(globals->Navigator) ;
	  printf("_dxfResetNavigateInteractor(globals->Navigator);\n");
      }
    }
  else if (COMPARE_MSGTYPE(PushCamera))
    {
      /*
       *  There are two camera stacks associated with each image window:
       *  the Undo stack and the Redo stack.  Anytime the view is about to
       *  be changed, the current view is pushed onto the Undo stack.  A
       *  message must then be sent to the UI to indicate that it is now
       *  possible to perform an Undo.
       *
       *  If the Undo stack is popped, the result gets pushed onto the
       *  Redo stack, and vice versa.  Whenever a stack is popped, a
       *  message is sent to the UI indicating whether the stack is now
       *  empty.
       *
       *  The top of the Undo stack is always the current view.
       */
      
      tdmMessageData data ;
      PRINT(("PushCamera"));

      data.l[0] = _dxfPushInteractorCamera(INTERACTOR_DATA) ;
#if defined(DX_NATIVE_WINDOWS)
      _dxfSendClientMessage (PARENT_WINDOW, CameraUndoable, &data) ;
#else
      _dxfSendClientMessage (DPY, PARENT_WINDOW, CameraUndoable, &data) ;
#endif
    }
  else if (COMPARE_MSGTYPE(UndoCamera))
    {
      int r ;
      tdmMessageData data ;
      PRINT(("UndoCamera"));

      /* pop undo stack */
      if ((r = _dxfPopInteractorCamera(INTERACTOR_DATA)) != 0)
	{
	  int pixwidth, projection ;
	  float f[3], t[3], u[3], width, aspect, fov, dist, Near, Far ;

	  /* popped camera has been pushed onto redo stack */
	  data.l[0] = 1 ;
#if defined(DX_NATIVE_WINDOWS)
	  _dxfSendClientMessage (PARENT_WINDOW, CameraRedoable, &data) ;
#else
	  _dxfSendClientMessage (DPY, PARENT_WINDOW, CameraRedoable, &data) ;
#endif

	  /* check to see if the undo stack is empty */
	  data.l[0] = (r == -1 ? 0 : 1) ;

	  /* get new camera info and pass it on to the UI */
	  _dxfGetInteractorViewInfo (INTERACTOR_DATA,
 				    f, t, u, &dist, &fov, &width,
 				    &aspect, &projection, &Near, &Far,
 				    &pixwidth) ;

	  _SendCamera (LWIN, t, u, f, width, pixwidth,
		       aspect, fov, projection, 1) ;

	  if (LINK)
	    _sendLinkCamera(globals, f, t, u, width, aspect, fov, dist,
					      projection, pixwidth);
	}
      else
	  data.l[0] = 0 ;

#if defined(DX_NATIVE_WINDOWS)
      _dxfSendClientMessage (PARENT_WINDOW, CameraUndoable, &data) ;
#else
      _dxfSendClientMessage (DPY, PARENT_WINDOW, CameraUndoable, &data) ;
#endif
    }
  else if (COMPARE_MSGTYPE(RedoCamera))
    {
      int r ;
      tdmMessageData data ;
      PRINT(("RedoCamera"));

      /* pop the redo stack */
      if ((r = _dxfRedoInteractorCamera(INTERACTOR_DATA)) != 0)
	{
	  int pixwidth, projection ;
	  float f[3], t[3], u[3], width, aspect, fov, dist, Near, Far ;

	  /* re-done camera is pushed onto undo stack */
	  data.l[0] = 1 ;
#if defined(DX_NATIVE_WINDOWS)
	  _dxfSendClientMessage (PARENT_WINDOW, CameraUndoable, &data) ;
#else
	  _dxfSendClientMessage (DPY, PARENT_WINDOW, CameraUndoable, &data) ;
#endif

	  /* check to see if the redo stack is empty */
	  data.l[0] = (r == -1 ? 0 : 1) ;

	  /* get new camera info and pass it on to the UI */
	  _dxfGetInteractorViewInfo (INTERACTOR_DATA,
 				    f, t, u, &dist, &fov, &width,
 				    &aspect, &projection, &Near, &Far,
 				    &pixwidth) ;

	  _SendCamera (LWIN, t, u, f, width, pixwidth,
		       aspect, fov, projection, 1) ;

	  if (LINK)
	    _sendLinkCamera(globals, f, t, u, width, aspect, fov, dist,
					      projection, pixwidth);

	}
      else
	  data.l[0] = 0 ;

#if defined(DX_NATIVE_WINDOWS)
      _dxfSendClientMessage (PARENT_WINDOW, CameraRedoable, &data) ;
#else
      _dxfSendClientMessage (DPY, PARENT_WINDOW, CameraRedoable, &data) ;
#endif
    }
  else if (COMPARE_MSGTYPE(Set_View))
    {
      /*
       *  The UI process is asking us for the `view-from' point and `up'
       *  vector corresponding to the specified view type.  The only
       *  reason they're computed here is that they used to be based upon
       *  the bounding box of the object, and the UI didn't have access to
       *  the box.
       */

      Vector up, dir ;
      float f[3], u[3], t[3];
      Point from, to ;
      tdmMessageData data ;
     
      PRINT(("Set_View"));
      if (! DXGetView (CAMERA, &from, &to, &up)) {
	EXIT(("ERROR: cant get current view"));
	DXErrorReturn (ERROR_DATA_INVALID, "Can't get current view") ;
      }

      up = DXVec(0.0, 1.0, 0.0) ;
      switch (dataP->l[0])
	{
	case FRONT:
	  PRINT(("front"));
	  dir = DXVec(0.0, 0.0, 1.0) ;
	  break;
	case OFF_FRONT:
	  PRINT(("off_front"));
	  dir = DXVec(SIN15, SIN15, COS15) ;
	  break;
	case BACK:
	  PRINT(("back"));
	  dir = DXVec(0.0, 0.0, -1.0) ;
	  break;
	case OFF_BACK:
	  PRINT(("off_back"));
	  dir = DXVec(SIN15, SIN15, -COS15) ;
	  break;
	case TOP:
	  PRINT(("top"));
	  dir = DXVec(0.0, 1.0, 0.0) ;
	  up = DXVec(0.0, 0.0, -1.0) ;
	  break;
	case OFF_TOP:
	  PRINT(("off_top"));
	  dir = DXVec(-SIN15, COS15, -SIN15) ;
	  up = DXVec(0.0, 0.0, -1.0) ;
	  break;
	case BOTTOM:
	  PRINT(("bottom"));
	  dir = DXVec(0.0, -1.0, 0.0) ;
	  up = DXVec(0.0, 0.0, 1.0) ;
	  break;
	case OFF_BOTTOM:
	  PRINT(("off_bottom"));
	  dir = DXVec(SIN15, -COS15, SIN15) ;
	  up = DXVec(0.0, 0.0, 1.0) ;
	  break;
	case RIGHT:
	  PRINT(("right"));
	  dir = DXVec(1.0, 0.0, 0.0) ;
	  break;
	case OFF_RIGHT:
	  PRINT(("off_right"));
	  dir = DXVec(COS05, SIN15, SIN15) ;
	  break;
	case LEFT:
	  PRINT(("left"));
	  dir = DXVec(-1.0, 0.0, 0.0) ;
	  break;
	case OFF_LEFT:
	  PRINT(("off_left"));
	  dir = DXVec(-COS05, SIN15, SIN15) ;
	  break;
	case DIAGONAL:
	  PRINT(("diagonal"));
	  dir = DXVec(SIN45, SIN45, SIN45) ;
	  break;
	case OFF_DIAGONAL:
	  PRINT(("off_diagonal"));
	  dir = DXVec(SIN35, SIN35, SIN45) ;
	  break;
	}

      /* compute new `from' point at same distance as original */
      from = DXAdd(to, DXMul(DXNormalize(dir), DXLength(DXSub(from, to)))) ;

      data.f[0] = f[0] = from.x ;
      data.f[1] = f[1] = from.y ;
      data.f[2] = f[2] = from.z ;
      PRINT(("new look-from point")); VPRINT(data.f) ;
#if defined(DX_NATIVE_WINDOWS)
      _dxfSendClientMessage (PARENT_WINDOW, FromPoint, &data) ;
#else
      _dxfSendClientMessage (DPY, PARENT_WINDOW, FromPoint, &data) ;
#endif

      data.f[0] = u[0] = up.x ;
      data.f[1] = u[0] = up.y ;
      data.f[2] = u[0] = up.z ;
      PRINT(("new up-vector")); VPRINT(data.f) ;
#if defined(DX_NATIVE_WINDOWS)
      _dxfSendClientMessage (PARENT_WINDOW, UpVector, &data) ;
#else
      _dxfSendClientMessage (DPY, PARENT_WINDOW, UpVector, &data) ;
#endif

      

      if (LINK)
      {
	t[0] = to.x;
	t[1] = to.y;
	t[2] = to.z;
	_sendLinkCamera(globals, f, t, u, width, aspect, fov, dist,
					      projection, pixwidth);
      }
    }
  else
    {
      PRINT(("%s: ignored", lookupAtomName(message->message_type)));
    }

  EXIT(("OK"));
  return OK ;
}


#define CACHE_CAMERA "CACHED_CAMERA_"

static void
_sendLinkCamera(tdmChildGlobalP globals,
  float *from, float *to, float *up,
  float width, float aspect, float fov, float dist,
  int projection, int pixwidth)
{
  DEFGLOBALDATA(globals);
  Point F, T;
  Vector U;
  Camera c;
  RGBColor bkgnd;

  DXUIMessage("LINK",
      "VALUE camera %s [%f %f %f] [%f %f %f] [%f %f %f] %f %d %f %f %d",
      WHERE,
      to[0], to[1], to[2],
      up[0], up[1], up[2],
      from[0], from[1], from[2],
      width, pixwidth, aspect, fov, projection);

  c = DXNewCamera();

  F.x = from[0]; F.y = from[1]; F.z = from[2];
  T.x = to[0];   T.y = to[1];   T.z = to[2];
  U.x = up[0];   U.y = up[1];   U.z = up[2];

  DXSetView(c, F, T, U);
  DXSetResolution(c, pixwidth, (double)aspect);
  if (projection)
      DXSetPerspective(c, fov, aspect);
  else
      DXSetOrthographic(c, width, aspect);

  DXGetBackgroundColor(CAMERA, &bkgnd);
  DXSetBackgroundColor(c, bkgnd);
    
  _dxfCacheState(ORIGINALWHERE, NULL, (dxObject)c, 0);
}

void
_dxfSendInteractorData (tdmChildGlobalP globals,
                       tdmInteractor I, tdmInteractorReturn *returnP) 
{
  /*
   *  This is invoked after the user has released a mouse button, ending the
   *  current interaction stroke.  If anything happened during the stroke,
   *  send the interactor's results back to the UI.
   */
  
  DEFGLOBALDATA(globals);
  tdmMessageData data0, data1, data2 ;
  tdmInteractorReturn local, *R ;
  int pixwidth, projection ;
  float from[3], to[3], up[3], width, aspect, fov, dist, Near, Far ;

  ENTRY(("_dxfSendInteractorData(0x%x, 0x%x, 0x%x)",
	 globals,I,returnP));

  if (!I)
    {
      EXIT(("I == NULL"));
      return ;
    }

#ifdef DX_NATIVE_WINDOWS
  if (!globals || !PARENT_WINDOW)
    {
      PRINT(("I = %0x%x", I));
      PRINT(("globals = 0x%x", globals));
      if (globals) {
	PRINT(("UI window = 0x%x", PARENT_WINDOW));
      }
      EXIT(("ERROR"));
      return ;
    }
#else
  if (!globals || !DPY || !PARENT_WINDOW)
    {
      PRINT(("I = %0x%x", I));
      PRINT(("globals = 0x%x", globals));
      if (globals) {
	PRINT(("display = 0x%x", DPY));
	PRINT(("UI window = 0x%x", PARENT_WINDOW));
      }
      EXIT(("ERROR"));
      return ;
    }
#endif

  if (returnP)
      /* if provided, use it.  occurs as result of double click */
      R = returnP ;
  else
    {
      /* get it ourselves. occurs at end of normal interaction stroke */
      tdmEndStroke (I, &local) ;
      R = &local ;
    }
  
  if (!R || !R->change)
    {
      /* nothing happened */
      EXIT(("no change"));
      return ;
    }

  /* get current view parameters */
  _dxfGetInteractorViewInfo (INTERACTOR_DATA,
		from, to, up, &dist, &fov, &width,
		&aspect, &projection, &Near, &Far,
		&pixwidth) ;

  if (I == globals->Zoomer || I == globals->PanZoom)
    {
      /*
      Vector up ;
      Point from, to ;
      */
      float fov, width, aspect ;

      data0.l[0] = _dxfPushInteractorCamera(INTERACTOR_DATA) ;
#if defined(DX_NATIVE_WINDOWS)
      _dxfSendClientMessage (PARENT_WINDOW, CameraUndoable, &data0) ;
#else
      _dxfSendClientMessage (DPY, PARENT_WINDOW, CameraUndoable, &data0) ;
#endif

      /*
      DXGetView (CAMERA, &from, &to, &up) ;
      */

      if (DXGetPerspective (CAMERA, &fov, &aspect))
	{
	  data1.l[0] = projection = 1 ;   /* perspective flag */
	  data1.f[1] = 1.0 ; /* nominal zoom factor */
	  data1.f[2] = fov = RAD2DEG(2 * atan(fov * R->zoom_factor / 2)) ;
	  if (data1.f[2] < 0.001)
	    {
	      PRINT(("view angle is too small"));
	      DXWarning ("#13940") ;
	      data1.f[2] = 0.001 ;
	    }
	  PRINT(("sending perspective camera fov %f", data1.f[2]));
	}
      else
	{
	  DXGetOrthographic (CAMERA, &width, &aspect) ;
	  data1.l[0] = projection = 0 ;   /* orthogonal flag */
	  data1.f[1] = R->zoom_factor ;
	  data1.f[2] = fov = 0 ;   /* nominal view angle */
	  PRINT(("sending ortho camera zoom factor %f", data1.f[1]));
	}

      data1.f[3] = to[0] = R->to[0] ;
      data1.f[4] = to[1] = R->to[1] ;
      data2.f[0] = to[2] = R->to[2] ;
      PRINT(("sending new look-to point")); VPRINT(R->to) ;

      data2.f[1] = from[0] = R->from[0] ;
      data2.f[2] = from[1] = R->from[1] ;
      data2.f[3] = from[2] = R->from[2] ;
      PRINT(("sending new look-from point")); VPRINT(R->from) ;

#if defined(DX_NATIVE_WINDOWS)
      _dxfSendClientMessage (PARENT_WINDOW, Zoom1, &data1) ;
      _dxfSendClientMessage (PARENT_WINDOW, Zoom2, &data2) ;
#else
      _dxfSendClientMessage (DPY, PARENT_WINDOW, Zoom1, &data1) ;
      _dxfSendClientMessage (DPY, PARENT_WINDOW, Zoom2, &data2) ;
#endif
    }
  else if (I == globals->User)
    {
      int pixwidth, pixheight, proj ;
      float width, aspect, fov;

      data0.l[0] = _dxfPushInteractorCamera(INTERACTOR_DATA) ;
#if defined(DX_NATIVE_WINDOWS)
      _dxfSendClientMessage (PARENT_WINDOW, CameraUndoable, &data0) ;
#else
      _dxfSendClientMessage (DPY, PARENT_WINDOW, CameraUndoable, &data0) ;
#endif

      DXGetCameraResolution (CAMERA, &pixwidth, &pixheight) ;
      if (! DXGetOrthographic (CAMERA, &width, &aspect))
        {
          DXGetPerspective (CAMERA, &fov, &aspect) ;
          proj = 1;
        }
        else
          proj = 0;

        _SendCamera (LWIN, R->to, R->up, R->from,
                   width, pixwidth, aspect, fov, proj, 0) ;

    }
  else if (I == globals->RotateGroup ||
	   ((I == globals->CursorGroup || I == globals->RoamGroup) &&
	    R->reason == tdmROTATION_UPDATE))
    {
      if (! PARENT_WINDOW)
        {
          /* no UI:  this is the only interactor available */
	  /* replace from, to, up, and dist with returned values */
	  _dxfSetInteractorViewInfo (INTERACTOR_DATA,
				R->from, R->to, R->up, R->dist,
 				fov, width, aspect, projection,
				Near, Far) ;
        }
      else
	{
	  data0.l[0] = _dxfPushInteractorCamera(INTERACTOR_DATA) ;
#if defined(DX_NATIVE_WINDOWS)
	  _dxfSendClientMessage (PARENT_WINDOW, CameraUndoable, &data0) ;
#else
	  _dxfSendClientMessage (DPY, PARENT_WINDOW, CameraUndoable, &data0) ;
#endif

	  data1.f[0] = from[0] = R->from[0] ;
	  data1.f[1] = from[1] = R->from[1] ;
	  data1.f[2] = from[2] = R->from[2] ;
	  PRINT(("new look-from point")); VPRINT(R->from) ;
	  
#if defined(DX_NATIVE_WINDOWS)
	  _dxfSendClientMessage (PARENT_WINDOW, FromPoint, &data1) ;
#else
	  _dxfSendClientMessage (DPY, PARENT_WINDOW, FromPoint, &data1) ;
#endif
	  
	  data1.f[0] = up[0] = R->up[0] ;
	  data1.f[1] = up[1] = R->up[1] ;
	  data1.f[2] = up[2] = R->up[2] ;
	  PRINT(("new up vector")); VPRINT(R->up) ;
	  
#if defined(DX_NATIVE_WINDOWS)
	  _dxfSendClientMessage (PARENT_WINDOW, UpVector, &data1) ;
#else
	  _dxfSendClientMessage (DPY, PARENT_WINDOW, UpVector, &data1) ;
#endif
	}
    }
  else if (I == globals->Navigator)
    {
      int pixwidth, pixheight ;
      float width, aspect, fov, viewdir[3] ;
      
      data0.l[0] = _dxfPushInteractorCamera(INTERACTOR_DATA) ;
#if defined(DX_NATIVE_WINDOWS)
      _dxfSendClientMessage (PARENT_WINDOW, CameraUndoable, &data0) ;
#else
      _dxfSendClientMessage (DPY, PARENT_WINDOW, CameraUndoable, &data0) ;
#endif

      DXGetCameraResolution (CAMERA, &pixwidth, &pixheight) ;
      if (DXGetOrthographic (CAMERA, &width, &aspect))
	{
	  /* Navigator has changed projection */
	  VSUB (viewdir, R->from, R->to) ;
	  fov = 2.0 * atan((width/LENGTH(viewdir)) / 2.0) ;
	  projection = 0;
	}
      else
        {
	  DXGetPerspective (CAMERA, &fov, &aspect) ;
	  projection = 1;
	}

      _SendCamera (LWIN, R->to, R->up, R->from,
		   0, pixwidth, aspect, fov, 1, 1) ;

    }
  else if (I == globals->CursorGroup && R->reason != tdmROTATION_UPDATE)
    {
      data1.l[0] = R->id ;
      data1.l[1] = R->reason ;
      data1.f[2] = R->x ;
      data1.f[3] = R->y ;
      data1.f[4] = R->z ;

      PRINT(("sending cursor %d change,", R->id));
      PRINT((" reason %d", R->reason));
      PRINT(("coordinates")); VPRINT((&data1.f[2])) ;
#if defined(DX_NATIVE_WINDOWS)
      _dxfSendClientMessage (PARENT_WINDOW, CursorChange, &data1) ;
#else
      _dxfSendClientMessage (DPY, PARENT_WINDOW, CursorChange, &data1) ;
#endif
    }
  else if (I == globals->RoamGroup && R->reason != tdmROTATION_UPDATE)
    {
      data1.f[0] = R->x ;
      data1.f[1] = R->y ;
      data1.f[2] = R->z ;

      PRINT(("sending new roam point")); VPRINT(data1.f) ;
#if defined(DX_NATIVE_WINDOWS)
      _dxfSendClientMessage (PARENT_WINDOW, RoamPoint, &data1) ;
#else
      _dxfSendClientMessage (DPY, PARENT_WINDOW, RoamPoint, &data1) ;
#endif

    }
  else if (I == globals->Pick)
    {
      data1.f[0] = R->x ;
      data1.f[1] = R->y ;
      data1.f[2] = R->z ;

      PRINT(("sending new pick point")); VPRINT(data1.f) ;
#ifdef DX_NATIVE_WINDOWS
      _dxfSendClientMessage (PARENT_WINDOW, RoamPoint, &data1) ;
#else
      _dxfSendClientMessage (DPY, PARENT_WINDOW, PickPoint, &data1) ;
#endif
    }

  if (LINK)
    _sendLinkCamera(globals, from, to, up, width, aspect, fov, dist,
					      projection, pixwidth);


  EXIT((""));
}
