/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>

#if defined(HAVE_STRING_H)
#include <string.h>
#endif

#include <stdio.h>
#include <math.h>
#include "hwDeclarations.h"
#include "hwUserInteractor.h"
#include "hwInteractorEcho.h"
#include "hwMatrix.h"
#include "hwWindow.h"
 
#ifndef STANDALONE
#include "hwMemory.h"
#endif

#include "hwPortLayer.h"

#include "hwDebug.h"

typedef float MATRIX[4][4];

extern Error DXNotifyRegistered(char *name);
extern Error DXNotifyRegisteredNoExecute(char *name);

void _dxf_computeMatrix(float *, float *, float *, float, MATRIX *);

static void UserDoubleClick(tdmInteractor, int, int, tdmInteractorReturn *);
static void UserStartStrokeP(tdmInteractor, int, int, int, int);
static void UserKeyStruck(tdmInteractor, int, int, char, int);
static void UserStrokePoint(tdmInteractor, int, int, int, int);
static void UserEndStrokeP(tdmInteractor, tdmInteractorReturnP);
static void UserDestroy(tdmInteractor);

void _dxfCacheState(char *baseTag, dxObject o, dxObject c, int quietly);

tdmInteractor
_dxfCreateUserInteractor(tdmInteractorWin W, tdmViewEchoFunc E, void *udata)
{
    register tdmInteractor I;

    if(W && (I = _dxfAllocateInteractor (W, sizeof(tdmUserInteractorData))))
    {
        DEFDATA(I,tdmUserInteractorData);
        DEFPORT(I_PORT_HANDLE);
    
        /* instance initial interactor methods */
	FUNC(I, DoubleClick)   = UserDoubleClick;
        FUNC(I, StartStroke)   = UserStartStrokeP;
        FUNC(I, StrokePoint)   = UserStrokePoint;
        FUNC(I, EndStroke)     = UserEndStrokeP;
        FUNC(I, Destroy)       = UserDestroy;
	FUNC(I, KeyStruck)     = UserKeyStruck;
        FUNC(I, ResumeEcho)    = _dxfNullResumeEcho;
        FUNC(I, EchoFunc)      = E;

	UDATA(I) 	       = udata;
	PDATA(mode) 	       = -1;

	return I;
    }

    EXIT(("ERROR"));
    return 0;
}

extern int _dxd_nUserInteractors;
extern void *_dxd_UserInteractors;

void
_dxfSetUserInteractorMode(tdmInteractor I, dxObject args)
{
    DEFDATA(I, tdmUserInteractorData);
    DEFGLOBALDATA(UDATA(I));
    int mode;
    UserInteractor *imode;
    Array amode;

    if (!args)
    {
	DXMessage("no object interaction mode given, none used");
	mode = -1;
    }
    else 
    {
	if (DXGetObjectClass(args) == CLASS_GROUP)
	{
	    amode = (Array)DXGetMember((Group)args, "submode");
	    args = (dxObject)DXGetMember((Group)args, "args");
	}
	else if (DXGetObjectClass(args) == CLASS_ARRAY)
	{
	    amode = (Array)args;
	    args = NULL;
	}
	else
	{
	    amode = NULL;
	    args = NULL;
	}

	if (!amode || !DXExtractInteger((dxObject)amode, &mode))
	{
	    DXMessage("Unable to access mode - must be an integer - none used");
	    mode = -1;
	}
    }
    
    imode = ((UserInteractor *)_dxd_UserInteractors) + PDATA(mode);

    if (mode > _dxd_nUserInteractors)
    {
	DXMessage("only %d user camera interactors are available",
						_dxd_nUserInteractors);
	mode = -1;
    }

    if (PDATA(mode) >= 0)
	imode->EndMode(PDATA(userData));
    
    PDATA(mode) = mode;

    imode = ((UserInteractor *)_dxd_UserInteractors) + mode;

    if (mode >= 0)
    {
	PDATA(userData) = imode->InitMode(args, CDATA(w), CDATA(h), &I->eventMask);
    }
    else
	I->eventMask = 0;
}


/*
 *  Method implementations
 */

static void
UserDoubleClick(tdmInteractor I, int x, int y, tdmInteractorReturn *R)
{
    R->change = 0;
}

static void 
UserStartStrokeP(tdmInteractor I, int x, int y, int btn, int s)
{
    float dx, dy, dz;
    DEFDATA(I,tdmUserInteractorData);
    DEFGLOBALDATA(UDATA(I));
    UserInteractor *imode = ((UserInteractor *)_dxd_UserInteractors) + PDATA(mode);
    DXMouseEvent mouse;

    if (PDATA(mode) < 0 || !(imode->EventHandler))
        return;


    CDATA(xlast) = x;
    CDATA(ylast) = y;

    PDATA(button) = btn;

    _dxfGetViewMatrix(CDATA(stack), CDATA(viewXfm));

    if (imode->SetCamera)
	imode->SetCamera(PDATA(userData),  CDATA(to), CDATA(from), CDATA(up),
				CDATA(projection), CDATA(fov), CDATA(width)); 
    if (imode->SetRenderable)
	imode->SetRenderable(PDATA(userData), OBJECT);

    switch (PDATA(button))
    {
	case 1: mouse.event = DXEVENT_LEFT;   break;
	case 2: mouse.event = DXEVENT_MIDDLE; break;
	case 3: mouse.event = DXEVENT_RIGHT;  break;
    }

    mouse.x	 = x;
    mouse.y	 = y;
    mouse.state  = BUTTON_DOWN;
    mouse.kstate = s;

    imode->EventHandler(PDATA(userData), (DXEvent *)&mouse);

    dx = CDATA(to)[0] - CDATA(from)[0];
    dy = CDATA(to)[1] - CDATA(from)[1];
    dz = CDATA(to)[2] - CDATA(from)[2];
    CDATA(vdist) = sqrt(dx*dx + dy*dy + dz*dz);

    EXIT((""));
}

static void
UserStrokePoint (tdmInteractor I, int x, int y, int flag, int s)
{
    float Near, Far;
    float to[3], from[3], up[3], zaxis[3];
    float fov, width;
    int   p;
    dxObject newObject;
    DEFDATA(I,tdmUserInteractorData) ;
    DEFPORT(I_PORT_HANDLE) ;
    DEFGLOBALDATA(UDATA(I));
    UserInteractor *imode = ((UserInteractor *)_dxd_UserInteractors) + PDATA(mode);
    int repaint = 0;
    DXMouseEvent mouse;

    if (PDATA(mode) < 0 || !(imode->EventHandler))
        return;


    CDATA(xlast) = x;
    CDATA(ylast) = y;

    switch (PDATA(button))
    {
	case 1: mouse.event = DXEVENT_LEFT;   break;
	case 2: mouse.event = DXEVENT_MIDDLE; break;
	case 3: mouse.event = DXEVENT_RIGHT;  break;
    }

    mouse.x	 = x;
    mouse.y	 = y;
    mouse.state  = (flag == INTERACTOR_BUTTON_UP) ? BUTTON_UP : BUTTON_MOTION;
    mouse.kstate = s;

    imode->EventHandler(PDATA(userData), (DXEvent *)&mouse);

    if (imode->GetCamera &&
	imode->GetCamera(PDATA(userData), to, from, up, &p, &fov, &width))
    {
	repaint = 1;

	_dxf_computeMatrix(to, from, up, width, (MATRIX *)&CDATA(viewXfm));

	GET_VIEW_DIRECTION(CDATA(viewXfm), zaxis);

	_dxfGetNearFar(p, CDATA(w),
		p ? fov : width,
		from, zaxis, CDATA(box), &Near, &Far);

	/*
	 * setting new clip planes requires creating a new projection matrix
	 */
	if (p)
	{
	  width = 0.0;
	  _dxfSetProjectionInfo(CDATA(stack), 1, fov, CDATA(aspect), Near, Far);
	  _dxf_SET_PERSPECTIVE_PROJECTION(PORT_CTX, fov, CDATA(aspect), Near, Far);
	}
        else
	{
	  fov = 0.0;
	  _dxfSetProjectionInfo(CDATA(stack), 0, width, CDATA(aspect), Near, Far);
	  _dxf_SET_ORTHO_PROJECTION(PORT_CTX, width, CDATA(aspect), Near, Far);
	}

	_dxf_LOAD_MATRIX (PORT_CTX, CDATA(viewXfm)) ;
	_dxfLoadViewMatrix (CDATA(stack), CDATA(viewXfm)) ;
	CDATA(view_state)++ ;
    }

    if (imode->GetRenderable &&
	imode->GetRenderable(PDATA(userData), &newObject))
    {
	repaint = 1;
	DXDelete(OBJECT);
	OBJECT = DXReference(newObject);
    }

    if (repaint)
    {
	_dxfSetCurrentView(LWIN, to, from, up, fov, width);
	_dxfCacheState(ORIGINALWHERE, OBJECT, NULL, 1);
	CALLFUNC(I,EchoFunc) (I, UDATA(I), CDATA(viewXfm), 1);
    }

    EXIT((""));
}

static void 
UserEndStrokeP(tdmInteractor I, tdmInteractorReturnP R)
{
    DEFDATA(I,tdmUserInteractorData);
    DEFGLOBALDATA(UDATA(I));
    float to[3], from[3], up[3];
    float fov, width;
    float dx, dy, dz;
    int   p;
    dxObject newObject;
    UserInteractor *imode = ((UserInteractor *)_dxd_UserInteractors) + PDATA(mode);
    int changed = 0;

    if (PDATA(mode) < 0)
        return;

    /*
     *  End stroke and return appropriate info.
     */

    ENTRY(("EndStroke(0x%x, 0x%x)", I, R));

    if (imode->GetCamera && 
	imode->GetCamera(PDATA(userData), to, from, up, &p, &fov, &width))
    {
	CDATA(to[0])      = to[0];
	CDATA(to[1])      = to[1];
	CDATA(to[2])      = to[2];
	CDATA(from[0])    = from[0];
	CDATA(from[1])    = from[1];
	CDATA(from[2])    = from[2];
	CDATA(up[0])      = up[0];
	CDATA(up[1])      = up[1];
	CDATA(up[2])      = up[2];
	CDATA(projection) = p;
	CDATA(fov)        = fov;
	CDATA(width)      = width;
	changed 	  = 1;
    }

    dx = to[0] - from[0];
    dy = to[1] - from[1];
    dz = to[2] - from[2];
    CDATA(vdist) = sqrt(dx*dx + dy*dy + dz*dz);

    _dxf_computeMatrix(CDATA(to), CDATA(from), CDATA(up),
			CDATA(width), (MATRIX *)&CDATA(viewXfm));
    _dxfRenormalizeView(CDATA(viewXfm));
        
    R->from[0] = CDATA(from[0]);
    R->from[1] = CDATA(from[1]);
    R->from[2] = CDATA(from[2]);

    R->to[0] = CDATA(to[0]);
    R->to[1] = CDATA(to[1]);
    R->to[2] = CDATA(to[2]);
        
    R->up[0] = CDATA(up[0]);
    R->up[1] = CDATA(up[1]);
    R->up[2] = CDATA(up[2]);
  
    R->dist = CDATA(vdist);
  
    COPYMATRIX(R->view, CDATA(viewXfm));

    R->reason = tdmROTATION_UPDATE;
    R->change = 1;

    if (imode->GetRenderable &&
        imode->GetRenderable(PDATA(userData), &newObject))
    {
	DXDelete(OBJECT);
	OBJECT = DXReference(newObject);
	changed = 1;
    }

    if (changed)
	_dxfCacheState(ORIGINALWHERE, OBJECT, NULL, 0);

    _dxfSetCurrentView(LWIN, to, from, up, fov, width);

    EXIT((""));
}


static void 
UserDestroy(tdmInteractor I)
{
    DEFDATA(I,tdmUserInteractorData);
    DEFPORT(I_PORT_HANDLE);
    UserInteractor *imode = ((UserInteractor *)_dxd_UserInteractors) + PDATA(mode);

    ENTRY(("Destroy(0x%x)", I));

    if (PDATA(mode) >= 0)
	imode->EndMode(PDATA(userData));

    _dxfDeallocateInteractor(I);
  
    EXIT((""));
}


static void
UserKeyStruck(tdmInteractor I, int x, int y, char c, int s)
{
    float Near, Far;
    float to[3], from[3], up[3], zaxis[3];
    float fov, width;
    int   p;
    dxObject newObject;
    DEFDATA(I,tdmUserInteractorData) ;
    DEFPORT(I_PORT_HANDLE) ;
    DEFGLOBALDATA(UDATA(I));
    UserInteractor *imode = ((UserInteractor *)_dxd_UserInteractors) + PDATA(mode);
    int repaint = 0;
    DXKeyPressEvent keypress;

    if (PDATA(mode) < 0 || !(imode->EventHandler))
        return;

    if (imode->SetCamera)
	imode->SetCamera(PDATA(userData),  CDATA(to), CDATA(from), CDATA(up),
				CDATA(projection), CDATA(fov), CDATA(width)); 
    if (imode->SetRenderable)
	imode->SetRenderable(PDATA(userData), OBJECT);

    CDATA(xlast) = x;
    CDATA(ylast) = y;

    keypress.event  = DXEVENT_KEYPRESS;
    keypress.x      = x;
    keypress.y      = y;
    keypress.key    = c;
    keypress.kstate = s;

    imode->EventHandler(PDATA(userData), (DXEvent *)&keypress);
    
    if (imode->GetCamera &&
	imode->GetCamera(PDATA(userData), to, from, up, &p, &fov, &width))
    {
	repaint = 1;

	_dxf_computeMatrix(to, from, up, width, (MATRIX *)&CDATA(viewXfm));

	GET_VIEW_DIRECTION(CDATA(viewXfm), zaxis);

	_dxfGetNearFar(p, CDATA(w),
		p ? fov : width,
		from, zaxis, CDATA(box), &Near, &Far);

	/*
	 * setting new clip planes requires creating a new projection matrix
	 */
	if (p)
	{
	  _dxfSetProjectionInfo(CDATA(stack), 1, fov, CDATA(aspect), Near, Far);
	  _dxf_SET_PERSPECTIVE_PROJECTION(PORT_CTX, fov, CDATA(aspect), Near, Far);
	}
        else
	{
	  _dxfSetProjectionInfo(CDATA(stack), 0, width, CDATA(aspect), Near, Far);
	  _dxf_SET_ORTHO_PROJECTION(PORT_CTX, width, CDATA(aspect), Near, Far);
	}

	_dxf_LOAD_MATRIX (PORT_CTX, CDATA(viewXfm)) ;
	_dxfLoadViewMatrix (CDATA(stack), CDATA(viewXfm)) ;
	CDATA(view_state)++ ;
    }

    if (imode->GetRenderable &&
	imode->GetRenderable(PDATA(userData), &newObject))
    {
	repaint = 1;
	DXDelete(OBJECT);
	OBJECT = DXReference(newObject);
    }

    if (repaint)
    {
	_dxfCacheState(ORIGINALWHERE, OBJECT, NULL, 0);
	CALLFUNC(I,EchoFunc) (I, UDATA(I), CDATA(viewXfm), 1);
    }

    EXIT((""));
}

void
_dxf_computeMatrix(float *to, float *from, float *up, float width, MATRIX *m)
{
    float a, v[3], xaxis[3], yaxis[3], zaxis[3], r[4][4];
    float t[4][4];


    t[0][0] =      1.0; t[0][1] =      0.0; t[0][2] =      0.0; t[0][3] =      0.0;
    t[1][0] =      0.0; t[1][1] =      1.0; t[1][2] =      0.0; t[1][3] =      0.0;
    t[2][0] =      0.0; t[2][1] =      0.0; t[2][2] =      1.0; t[2][3] =      0.0;
    t[3][0] = -from[0]; t[3][1] = -from[1]; t[3][2] = -from[2]; t[3][3] =      1.0;

    v[0] = from[0] - to[0];
    v[1] = from[1] - to[1];
    v[2] = from[2] - to[2];

    a = sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
    
    zaxis[0] = v[0] / a;
    zaxis[1] = v[1] / a;
    zaxis[2] = v[2] / a;

    v[0] = up[1]*zaxis[2] - up[2]*zaxis[1];
    v[1] = up[2]*zaxis[0] - up[0]*zaxis[2];
    v[2] = up[0]*zaxis[1] - up[1]*zaxis[0];

    a = sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);

    xaxis[0] = v[0] / a;
    xaxis[1] = v[1] / a;
    xaxis[2] = v[2] / a;

    v[0] = zaxis[1]*xaxis[2] - zaxis[2]*xaxis[1];
    v[1] = zaxis[2]*xaxis[0] - zaxis[0]*xaxis[2];
    v[2] = zaxis[0]*xaxis[1] - zaxis[1]*xaxis[0];

    a = sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);

    yaxis[0] = v[0] / a;
    yaxis[1] = v[1] / a;
    yaxis[2] = v[2] / a;

    r[0][0] = xaxis[0]; r[0][1] = yaxis[0]; r[0][2] = zaxis[0]; r[0][3] = 0.0;
    r[1][0] = xaxis[1]; r[1][1] = yaxis[1]; r[1][2] = zaxis[1]; r[1][3] = 0.0;
    r[2][0] = xaxis[2]; r[2][1] = yaxis[2]; r[2][2] = zaxis[2]; r[2][3] = 0.0;
    r[3][0] =      0.0; r[3][1] =      0.0; r[3][2] =      0.0; r[3][3] = 1.0;

    MULTMATRIX((*m), t, r);
}

#define CACHE_OBJECT_TAG "CACHED_OBJECT_"
#define CACHE_CAMERA_TAG "CACHED_CAMERA_"

void
_dxfCacheState(char *baseTag, dxObject o, dxObject c, int quietly)
{
    char *tag = NULL;

#if 0
    if (o || c)
	DXMessage("Caching state %s", quietly ? "QUIETLY": "LOUDLY");
#endif

    if (o)
    {
	tag = DXAllocate(strlen(CACHE_OBJECT_TAG) + strlen(baseTag) + 10);
	sprintf(tag, "%s%s", CACHE_OBJECT_TAG, baseTag);
	DXSetCacheEntry(o, CACHE_PERMANENT, tag, 0, 0);
	if (quietly)
	    DXNotifyRegisteredNoExecute(tag);
	else
	    DXNotifyRegistered(tag);
	DXFree((Pointer)tag);

    }

    if (c)
    {
	tag = DXAllocate(strlen(CACHE_CAMERA_TAG) + strlen(baseTag) + 10);
	sprintf(tag, "%s%s", CACHE_CAMERA_TAG, baseTag);
	DXSetCacheEntry(c, CACHE_PERMANENT, tag, 0, 0);
	if (quietly)
	    DXNotifyRegisteredNoExecute(tag);
	else
	    DXNotifyRegistered(tag);
	DXFree((Pointer)tag);
    }
}


