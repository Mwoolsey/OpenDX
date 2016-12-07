/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#ifndef tdmClientMessage_h
#define tdmClientMessage_h
/*---------------------------------------------------------------------------*\
 $Source: /src/master/dx/src/exec/hwrender/hwClientMessage.h,v $
  Author: Mark Hood

  Types, prototypes, and other stuff for the client message utilities.

  When including this file be sure to include hwWindow.h. It cannot be
  included here because of conflicts with the Object type defined in both
  Picture.h and dx/dx.h. The Object that hwWindow requires is the one
  defined in dx/dx.h.

\*---------------------------------------------------------------------------*/

/*
 *  The following view types were taken from Picture.h
 */

#define FRONT		1
#define OFF_FRONT	2
#define BACK		3
#define OFF_BACK	4
#define TOP		5
#define OFF_TOP		6
#define BOTTOM		7
#define OFF_BOTTOM	8
#define RIGHT		9
#define OFF_RIGHT      10
#define LEFT	       11
#define OFF_LEFT       12
#define DIAGONAL       13
#define OFF_DIAGONAL   14

typedef union 
{
  int32 l[5] ;
  float f[5] ;
} tdmMessageData, *tdmMessageDataP ;

int
_dxfInitCMProtocol(WinP) ;

void
_dxfInitDXInteractors(tdmChildGlobalP) ;

#if defined(DX_NATIVE_WINDOWS)
void
_dxfSendClientMessage (HWND, int, tdmMessageDataP) ;
#else
void
_dxfSendClientMessage (Display *, Window, int, tdmMessageDataP) ;
#endif

void
_dxfConnectUI(WinP) ;

void
_dxfSendInteractorData (tdmChildGlobalP, tdmInteractor, tdmInteractorReturn *) ;

#if defined(DX_NATIVE_WINDOWS)
Error
_dxfReceiveClientMessage (tdmChildGlobalP, MSG *, int *) ;
#else
Error
_dxfReceiveClientMessage (tdmChildGlobalP, XClientMessageEvent *, int *) ;
#endif


/*
 *  UI/Renderer protocol elements:
 */
/*
 * IMPORTANT!  Do not change the names of these defines, they are both the
 * define and the *string* that is shared with the UI. Changing the name
 * will make the atom unrecognizable by the UI. (See internAtoms in 
 * hwClientMessage.c and INTERNATOM below)
 */

#if defined(DX_NATIVE_WINDOWS)

#define WM_CLIENT_MESSAGE (WM_USER + 0)
#define Atom int
#define None -1
#define INTERNATOM(name) \
    _dxdtdmAtoms[name].atom = name; \
    _dxdtdmAtoms[name].spelling = #name

#else

#define INTERNATOM(name) \
  _dxdtdmAtoms[name].atom = XInternAtom(dpy, #name, FALSE);\
  _dxdtdmAtoms[name].spelling = #name

#endif

#define GLWindow0                        0
#define GLWindow1                        1
#define GLWindow2                        2
#define GLWindow2Execute                 3
#define StartRotateInteraction           4
#define FromPoint                        5
#define UpVector                         6
#define StartZoomInteraction             7
#define StartPanZoomInteraction          8
#define Zoom1                            9
#define Zoom2                           10
#define StartCursorInteraction          11
#define CursorChange                    12
#define SetCursorConstraint             13
#define SetCursorSpeed                  14
#define StartRoamInteraction            15
#define RoamPoint                       16
#define StartNavigateInteraction        17
#define NavigateMotion                  18
#define NavigatePivot                   19
#define StopInteraction                 20
#define DisplayGlobe                    21
#define CameraUndoable                  22
#define CameraRedoable                  23
#define UndoCamera                      24
#define RedoCamera                      25
#define PushCamera                      26
#define ButtonMapping1                  27
#define ButtonMapping2                  28
#define ButtonMapping3                  29
#define NavigateLookAt                  30
#define ExecuteOnChange                 31
#define Set_View                        32
#define GLDestroyWindow			33
#define GLShutdown			34
#define XA_Integer			35	/* add predefined XA_INTEGER */
#define POKE_CONNECTION			36
#define StartPickInteraction            37
#define PickPoint                       38
#define ImageReset                      39
#define StartUserInteraction		40
#define MAX_ATOMS			41	/* Change this to be the last
						   atom +1 */

#define Camera0 GLWindow0		/* Alias, mnemonic */
#define Camera1 GLWindow1		/* Alias, mnemonic */
#define Camera2 GLWindow2		/* Alias, mnemonic */
#define Camera2Execute GLWindow2Execute	/* Alias, mnemonic */

struct atomRep {
  Atom atom ;
  char *spelling ;
};

#define ATOM(x) _dxdtdmAtoms[x].atom

#if defined(DEBUG)
#define CHECKATOM(a) 							\
{ 									\
  if (a.atom != None && a.atom != BadAlloc && a.atom != BadValue) { 	\
    PRINT(("%s interned value %d\n", a.spelling, a.atom)); 		\
  } else {								\
    PRINT(("Failed to intern %s", a.spelling)) ; 			\
    DXSetError (ERROR_INTERNAL, "#13005", a.spelling) ; 		\
  } 									\
}
#else
#define CHECKATOM(a)
#endif

#endif /* tdmClientMessage_h */
