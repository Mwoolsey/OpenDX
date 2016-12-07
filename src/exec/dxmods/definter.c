/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>

#include <dx/dx.h>
#include <stdio.h>
#include <math.h>

#define N_USER_INTERACTORS 3

static void *RotateInitMode(Object, int, int, int *);
static void RotateEndMode(void *);
static void RotateSetCamera(void *, float *, float *,
		float *, int, float, float);
static int RotateGetCamera(void *, float *, float *,
		float *, int *, float *, float *);
static void RotateSetRenderable(void *, Object);
static int  RotateGetRenderable(void *, Object *);
static void  RotateEventHandler(void *, DXEvent *);

static void *PanInitMode(Object, int, int, int *);
static void PanEndMode(void *);
static void PanSetCamera(void *, float *, float *,
		float *, int, float, float);
static int PanGetCamera(void *, float *, float *,
		float *, int *, float *, float *);
static void PanSetRenderable(void *, Object);
static int  PanGetRenderable(void *, Object *);
static void  PanEventHandler(void *, DXEvent *);


static void *ZoomInitMode(Object, int, int, int *);
static void ZoomEndMode(void *);
static void ZoomSetCamera(void *, float *, float *,
		float *, int, float, float);
static int ZoomGetCamera(void *, float *, float *,
		float *, int *, float *, float *);
static void ZoomSetRenderable(void *, Object);
static int  ZoomGetRenderable(void *, Object *);
static void  ZoomEventHandler(void *, DXEvent *);

static UserInteractor _userInteractionTable[N_USER_INTERACTORS];

int
DXDefaultUserInteractors(int *n, void *t)
{
    _userInteractionTable[0].InitMode      = RotateInitMode;
    _userInteractionTable[0].EndMode       = RotateEndMode;
    _userInteractionTable[0].SetCamera     = RotateSetCamera;
    _userInteractionTable[0].GetCamera     = RotateGetCamera;
    _userInteractionTable[0].SetRenderable = RotateSetRenderable;
    _userInteractionTable[0].GetRenderable = RotateGetRenderable;
    _userInteractionTable[0].EventHandler  = RotateEventHandler;

    _userInteractionTable[1].InitMode      = PanInitMode;
    _userInteractionTable[1].EndMode       = PanEndMode;
    _userInteractionTable[1].SetCamera     = PanSetCamera;
    _userInteractionTable[1].GetCamera     = PanGetCamera;
    _userInteractionTable[1].SetRenderable = PanSetRenderable;
    _userInteractionTable[1].GetRenderable = PanGetRenderable;
    _userInteractionTable[1].EventHandler  = PanEventHandler;

    _userInteractionTable[2].InitMode      = ZoomInitMode;
    _userInteractionTable[2].EndMode       = ZoomEndMode;
    _userInteractionTable[2].SetCamera     = ZoomSetCamera;
    _userInteractionTable[2].GetCamera     = ZoomGetCamera;
    _userInteractionTable[2].SetRenderable = ZoomSetRenderable;
    _userInteractionTable[2].GetRenderable = ZoomGetRenderable;
    _userInteractionTable[2].EventHandler  = ZoomEventHandler;

    *n = N_USER_INTERACTORS;
    *(long **)t = (long *)_userInteractionTable;
    return 1;
}
     
/***** Rotate *****/

static void RotateStartStroke(void *, DXMouseEvent *);
static void RotateEndStroke(void *, DXMouseEvent *);

typedef struct RotateData
{
    Object args;	/* Interactor arguments		*/
    int w, h;		/* Width and height of window 	*/
    Object obj;		/* Current object		*/

    float to[3];	/* Current camera parameters	*/
    float from[3];
    float up[3];
    int p;
    float fov;
    float width;

    float grad;

    int motionFlag;
    struct
    {
        int x, y;
    } buttonPositions[3];
    int buttonStates[3];

} *RotateData;


static void *
RotateInitMode(Object args, int w, int h, int *mask)
{
    RotateData rdata = (RotateData)DXAllocateZero(sizeof(struct RotateData));
    if (! rdata)
	return NULL;
    
    if (args)
    {
    }

    /* 
     * Grab the window size 
     */
    rdata->w = w;
    rdata->h = h;

    if (w > h)
	rdata->grad = h / 2.0;
    else
	rdata->grad = w / 2.0;

    rdata->buttonStates[0] = BUTTON_UP;
    rdata->buttonStates[1] = BUTTON_UP;
    rdata->buttonStates[2] = BUTTON_UP;

    *mask = DXEVENT_LEFT | DXEVENT_RIGHT;
	
    return (void *)rdata;
}

static void
RotateEndMode(void *data)
{
    RotateData rdata = (RotateData)data;
    if (rdata)
    {
	if (rdata->args)
	    DXDelete(rdata->args);
	
	if (rdata->obj)
	    DXDelete(rdata->obj);
    }

    DXFree(data);
}

static void 
RotateSetCamera(void *data, float to[3], float from[3], float up[3],
		    int proj, float fov, float width)
{
    int i;
    RotateData rdata = (RotateData)data;

    /*
     * Grab the camera
     */
    for (i = 0; i < 3; i++)
	rdata->to[i]   = to[i],
	rdata->from[i] = from[i],
	rdata->up[i]   = up[i];

    rdata->p     = proj;
    rdata->fov   = fov;
    rdata->width = width;
}

static int 
RotateGetCamera(void *data, float to[3], float from[3], float up[3],
		    int *proj, float *fov, float *width)
{
    int i;
    RotateData rdata = (RotateData)data;

    /*
     * Return the camera 
     */
    for (i = 0; i < 3; i++)
	to[i]   = rdata->to[i],
	from[i] = rdata->from[i],
	up[i]   = rdata->up[i];

    *proj  = rdata->p;
    *fov   = rdata->fov;
    *width = rdata->width;

    return 1;
}

static void
RotateSetRenderable(void *data, Object obj)
{
    RotateData rdata = (RotateData)data;
    DXReference(obj);
    if (rdata->obj)
	DXDelete(rdata->obj);
    rdata->obj = obj;
}
    

static int
RotateGetRenderable(void *data, Object *obj)
{
    RotateData rdata = (RotateData)data;
    if (rdata->obj)
    {
	*obj = rdata->obj;
	return 1;
    }
    else
    {
	*obj = NULL;
	return 0;
    }
}

static void
RotateEventHandler(void *data, DXEvent *event)
{
    RotateData rdata = (RotateData)data;

    switch(event->any.event)
    {
	case DXEVENT_LEFT:
	case DXEVENT_MIDDLE:
	case DXEVENT_RIGHT:
	    {
		int b = WHICH_BUTTON(event);

		switch (event->mouse.state)
		{
		    case BUTTON_DOWN:
			RotateStartStroke(data, (DXMouseEvent *)event);
			rdata->buttonStates[b] = BUTTON_DOWN;
			break;
		    
		    case BUTTON_MOTION:
			if (rdata->buttonStates[b] == BUTTON_UP)
			    RotateStartStroke(data, (DXMouseEvent *)event);
			else
			    RotateEndStroke(data, (DXMouseEvent *)event);
			rdata->buttonStates[b] = BUTTON_DOWN;
			break;

		    case BUTTON_UP:
			RotateEndStroke(data, (DXMouseEvent *)event);
			rdata->buttonStates[b] = BUTTON_UP;
			break;

		    default:
			break;
		}
	
	    }
	    break;
		
        default:
            break;
    }
}


static void 
RotateStartStroke(void *data, DXMouseEvent *event)
{
    RotateData rdata = (RotateData)data;

    /*
     * Just keep track of which button is pressed
     * and where it was pressed.
     */
    int b = WHICH_BUTTON(event);
    if (b != NO_BUTTON)
    {
        rdata->buttonPositions[b].x = event->x;
        rdata->buttonPositions[b].y = event->y;
    }
    return;
}

static void
RotateEndStroke(void *data, DXMouseEvent *event)
{
    int b = WHICH_BUTTON(event);

    RotateData rdata = (RotateData)data;
    if (! rdata)
	return;

    if (b == LEFTBUTTON)
    {
	float ov[3], nv[3], rot[4][4];
	float rt[3], up[3];
	float stroke[3], axis[3], d;
	float a, dx, dy;
	float t, s, c;

	dx = event->x - rdata->buttonPositions[b].x;
	dy = event->y - rdata->buttonPositions[b].y;

	a = sqrt(dx*dx + dy*dy);
	if (a != 0.0)
	{

	    ov[0] = rdata->from[0] - rdata->to[0];
	    ov[1] = rdata->from[1] - rdata->to[1];
	    ov[2] = rdata->from[2] - rdata->to[2];

	    rt[0] = ov[1]*rdata->up[2] - ov[2]*rdata->up[1];
	    rt[1] = ov[2]*rdata->up[0] - ov[0]*rdata->up[2];
	    rt[2] = ov[0]*rdata->up[1] - ov[1]*rdata->up[0];

	    d = sqrt(rt[0]*rt[0] + rt[1]*rt[1] + rt[2]*rt[2]);
	    rt[0] /= d;
	    rt[1] /= d;
	    rt[2] /= d;

	    up[0] = rt[1]*ov[2] - rt[2]*ov[1];
	    up[1] = rt[2]*ov[0] - rt[0]*ov[2];
	    up[2] = rt[0]*ov[1] - rt[1]*ov[0];
	    d = sqrt(up[0]*up[0] + up[1]*up[1] + up[2]*up[2]);
	    rdata->up[0] = up[0] /= d;
	    rdata->up[1] = up[1] /= d;
	    rdata->up[2] = up[2] /= d;

	    dx /= rdata->grad;
	    dy /= rdata->grad;

	    stroke[0] = dx*rt[0] + dy*up[0];
	    stroke[1] = dx*rt[1] + dy*up[1];
	    stroke[2] = dx*rt[2] + dy*up[2];

	    axis[0] = ov[1]*stroke[2] - ov[2]*stroke[1];
	    axis[1] = ov[2]*stroke[0] - ov[0]*stroke[2];
	    axis[2] = ov[0]*stroke[1] - ov[1]*stroke[0];
	    d = sqrt(axis[0]*axis[0] + axis[1]*axis[1] + axis[2]*axis[2]);
	    axis[0] /= d;
	    axis[1] /= d;
	    axis[2] /= d;

	    a = sqrt(dx*dx + dy*dy);
	    s = (float)sin(a);
	    c = (float)cos(a);
	    t = 1.0 - c;

	    ROTXYZ(rot, axis[0], axis[1], axis[2], s, c, t);

	    nv[0] = rot[0][0]*ov[0] +
		    rot[1][0]*ov[1] +
		    rot[2][0]*ov[2];

	    nv[1] = rot[0][1]*ov[0] +
		    rot[1][1]*ov[1] +
		    rot[2][1]*ov[2];

	    nv[2] = rot[0][2]*ov[0] +
		    rot[1][2]*ov[1] +
		    rot[2][2]*ov[2];

	    rdata->from[0] = rdata->to[0] + nv[0];
	    rdata->from[1] = rdata->to[1] + nv[1];
	    rdata->from[2] = rdata->to[2] + nv[2];
	}
    }
    else if (b == RIGHTBUTTON)
    {
	float v[3], rot[4][4];
	float n[3], d;
	float a, a0, a1;
	float x0, y0;
	float x1, y1;
	float t, s, c;
	int cx = rdata->w / 2;
	int cy = rdata->h / 2;

	x0 = event->x - cx;
	y0 = event->y - cy;

	if (x0 == 0 && y0 == 0)
	    x0 = 1;

	d = sqrt(x0*x0 + y0*y0);
	x0 /= d;
	y0 /= d;
	if (y0 < 0)
	    a0 = acos(x0);
	else 
	    a0 = (2*3.1415926) - acos(x0);

	x1 = rdata->buttonPositions[b].x - cx;
	y1 = rdata->buttonPositions[b].y - cy;

	if (x1 == 0 && y1 == 0)
	    x1 = 1;

	d = sqrt(x1*x1 + y1*y1);
	x1 /= d;
	y1 /= d;
	if (y1 < 0)
	    a1 = acos(x1);
	else 
	    a1 = (2*3.1415926) - acos(x1);

	v[0] = rdata->from[0] - rdata->to[0];
	v[1] = rdata->from[1] - rdata->to[1];
	v[2] = rdata->from[2] - rdata->to[2];
	d = sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
	v[0] /= d;
	v[1] /= d;
	v[2] /= d;

	a = a1 - a0;

	s = (float)sin(a);
	c = (float)cos(a);
	t = 1.0 - c;

	ROTXYZ(rot, v[0], v[1], v[2], s, c, t);

	n[0] = rot[0][0]*rdata->up[0] +
	       rot[1][0]*rdata->up[1] +
	       rot[2][0]*rdata->up[2];

	n[1] = rot[0][1]*rdata->up[0] +
	       rot[1][1]*rdata->up[1] +
	       rot[2][1]*rdata->up[2];

	n[2] = rot[0][2]*rdata->up[0] +
	       rot[1][2]*rdata->up[1] +
	       rot[2][2]*rdata->up[2];

	rdata->up[0] = n[0];
	rdata->up[1] = n[1];
	rdata->up[2] = n[2];
    }

    rdata->buttonPositions[b].x = event->x;
    rdata->buttonPositions[b].y = event->y;

    rdata->motionFlag = (event->state == BUTTON_MOTION);

    return;

}

/***** Pan *****/

static void PanStartStroke(void *, DXMouseEvent *);
static void PanEndStroke(void *, DXMouseEvent *);

typedef struct PanData
{
    Object args;	/* Interactor arguments		*/
    int w, h;		/* Width and height of window 	*/
    Object obj;		/* Current object		*/

    float to[3];	/* Current camera parameters	*/
    float from[3];
    float up[3];
    int p;
    float fov;
    float width;

    float grad;
    int motionFlag;
    struct
    {
        int x, y;
    } buttonPositions[3];
    int buttonStates[3];

} *PanData;


static void *
PanInitMode(Object args, int w, int h, int *mask)
{
    PanData pdata = (PanData)DXAllocateZero(sizeof(struct PanData));
    if (! pdata)
	return NULL;
    
    if (args)
    {
    }

    /* 
     * Grab the window size 
     */
    pdata->w = w;
    pdata->h = h;

    if (w > h)
	pdata->grad = h / 2.0;
    else
	pdata->grad = w / 2.0;

    pdata->buttonStates[0] = BUTTON_UP;
    pdata->buttonStates[1] = BUTTON_UP;
    pdata->buttonStates[2] = BUTTON_UP;

    *mask = DXEVENT_LEFT;
	
    return (void *)pdata;
}

static void
PanEndMode(void *data)
{
    PanData pdata = (PanData)data;
    if (pdata)
    {
	if (pdata->args)
	    DXDelete(pdata->args);
	
	if (pdata->obj)
	    DXDelete(pdata->obj);
    }

    DXFree(data);
}

static void 
PanSetCamera(void *data, float to[3], float from[3], float up[3],
		    int proj, float fov, float width)
{
    int i;
    float dx = to[0] - from[0];
    float dy = to[1] - from[1];
    float dz = to[2] - from[2];
    float vd = sqrt(dx*dx + dy*dy + dz*dz);
    PanData pdata = (PanData)data;

    /*
     * Grab the camera
     */
    for (i = 0; i < 3; i++)
	pdata->to[i]   = to[i],
	pdata->from[i] = from[i],
	pdata->up[i]   = up[i];

    pdata->p     = proj;
    pdata->fov   = fov;
    pdata->width = width;

    if (proj)
	pdata->grad = (fov * vd) / pdata->w;
    else
	pdata->grad = width / pdata->w;
}

static int 
PanGetCamera(void *data, float to[3], float from[3], float up[3],
		    int *proj, float *fov, float *width)
{
    int i;
    PanData pdata = (PanData)data;

    /*
     * Return the camera 
     */
    for (i = 0; i < 3; i++)
	to[i]   = pdata->to[i],
	from[i] = pdata->from[i],
	up[i]   = pdata->up[i];

    *proj  = pdata->p;
    *fov   = pdata->fov;
    *width = pdata->width;

    return 1;
}

static void
PanSetRenderable(void *data, Object obj)
{
    PanData pdata = (PanData)data;
    DXReference(obj);
    if (pdata->obj)
	DXDelete(pdata->obj);
    pdata->obj = obj;
}
    

static int
PanGetRenderable(void *data, Object *obj)
{
    PanData pdata = (PanData)data;
    if (pdata->obj)
    {
	*obj = pdata->obj;
	return 1;
    }
    else
    {
	*obj = NULL;
	return 0;
    }
}

static void
PanEventHandler(void *data, DXEvent *event)
{
    PanData pdata = (PanData)data;

    switch(event->any.event)
    {
	case DXEVENT_LEFT:
	case DXEVENT_MIDDLE:
	case DXEVENT_RIGHT:
	    {
		int b = WHICH_BUTTON(event);

		switch (event->mouse.state)
		{
		    case BUTTON_DOWN:
			PanStartStroke(data, (DXMouseEvent *)event);
			pdata->buttonStates[b] = BUTTON_DOWN;
			break;
		    
		    case BUTTON_MOTION:
			if (pdata->buttonStates[b] == BUTTON_UP)
			    PanStartStroke(data, (DXMouseEvent *)event);
			else
			    PanEndStroke(data, (DXMouseEvent *)event);
			pdata->buttonStates[b] = BUTTON_DOWN;
			break;

		    case BUTTON_UP:
			PanEndStroke(data, (DXMouseEvent *)event);
			pdata->buttonStates[b] = BUTTON_UP;
			break;

		    default:
			break;
		}
	
	    }
	    break;
		
        default:
            break;
    }
}

static void 
PanStartStroke(void *data, DXMouseEvent *event)
{
    PanData pdata = (PanData)data;

    /*
     * Just keep track of which button is 
     * pressed and where it was pressed.
     */
    int b = WHICH_BUTTON(event);
    if (b != NO_BUTTON)
    {
        pdata->buttonPositions[b].x = event->x;
        pdata->buttonPositions[b].y = event->y;
    }
    return;
}

static void
PanEndStroke(void *data, DXMouseEvent *event)
{
    int b = WHICH_BUTTON(event);

    PanData pdata = (PanData)data;
    if (! pdata)
	return;

    pdata->motionFlag = (event->state == BUTTON_MOTION);

    if (event->x != pdata->buttonPositions[b].x ||
	event->y != pdata->buttonPositions[b].y)
    {
	float a, u[3], r[3], v[3];
	float dx, dy;

	dx = event->x - pdata->buttonPositions[b].x;
	dy = event->y - pdata->buttonPositions[b].y;

	a = sqrt(pdata->up[0]*pdata->up[0] +
		 pdata->up[1]*pdata->up[1] +
		 pdata->up[2]*pdata->up[2]);
	    
	u[0] = pdata->up[0]/a;
	u[1] = pdata->up[1]/a;
	u[2] = pdata->up[2]/a;

	v[0] = pdata->from[0] - pdata->to[0];
	v[1] = pdata->from[1] - pdata->to[1];
	v[2] = pdata->from[2] - pdata->to[2];

	r[0] = v[2]*pdata->up[1] - v[1]*pdata->up[2];
	r[1] = v[0]*pdata->up[2] - v[2]*pdata->up[0];
	r[2] = v[1]*pdata->up[0] - v[0]*pdata->up[1];

	a = sqrt(r[0]*r[0] + r[1]*r[1]+ r[2]*r[2]);
	r[0] /= a;
	r[1] /= a;
	r[2] /= a;

	v[0] = (-dx*r[0] + dy*u[0]) * pdata->grad;
	v[1] = (-dx*r[1] + dy*u[1]) * pdata->grad;
	v[2] = (-dx*r[2] + dy*u[2]) * pdata->grad;

	pdata->to[0] += v[0];
	pdata->to[1] += v[1];
	pdata->to[2] += v[2];

	pdata->from[0] += v[0];
	pdata->from[1] += v[1];
	pdata->from[2] += v[2];
    }

    pdata->buttonPositions[b].x = event->x;
    pdata->buttonPositions[b].y = event->y;

    return;

}

/***** Zoom *****/

static void ZoomStartStroke(void *, DXMouseEvent *);
static void ZoomEndStroke(void *, DXMouseEvent *);

typedef struct ZoomData
{
    Object args;	/* Interactor arguments		*/
    int w, h;		/* Width and height of window 	*/
    Object obj;		/* Current object		*/

    float to[3];	/* Current camera parameters	*/
    float from[3];
    float up[3];
    int p;
    float fov;
    float width;

    float init_width;

    int motionFlag;

    struct
    {
        int x, y;
    } buttonPositions[3];
    int buttonStates[3];

} *ZoomData;


static void *
ZoomInitMode(Object args, int w, int h, int *mask)
{
    ZoomData zdata = (ZoomData)DXAllocateZero(sizeof(struct ZoomData));
    if (! zdata)
	return NULL;
    
    if (args)
    {
    }

    /* 
     * Grab the window size 
     */
    zdata->w = w;
    zdata->h = h;

    zdata->buttonStates[0] = BUTTON_UP;
    zdata->buttonStates[1] = BUTTON_UP;
    zdata->buttonStates[2] = BUTTON_UP;

    *mask = DXEVENT_LEFT;
	
    return (void *)zdata;
}

static void
ZoomEndMode(void *data)
{
    ZoomData zdata = (ZoomData)data;
    if (zdata)
    {
	if (zdata->args)
	    DXDelete(zdata->args);
	
	if (zdata->obj)
	    DXDelete(zdata->obj);
    }

    DXFree(data);
}

static void 
ZoomSetCamera(void *data, float to[3], float from[3], float up[3],
		    int proj, float fov, float width)
{
    int i;
    ZoomData zdata = (ZoomData)data;

    /*
     * Grab the camera
     */
    for (i = 0; i < 3; i++)
	zdata->to[i]   = to[i],
	zdata->from[i] = from[i],
	zdata->up[i]   = up[i];

    zdata->p     = proj;
    zdata->fov   = fov;
    zdata->width = zdata->init_width = width;
}

static int 
ZoomGetCamera(void *data, float to[3], float from[3], float up[3],
		    int *proj, float *fov, float *width)
{
    int i;
    ZoomData zdata = (ZoomData)data;

    /*
     * Return the camera 
     */
    for (i = 0; i < 3; i++)
	to[i]   = zdata->to[i],
	from[i] = zdata->from[i],
	up[i]   = zdata->up[i];

    *proj  = zdata->p;
    *fov   = zdata->fov;
    *width = zdata->width;

    return 1;
}

static void
ZoomSetRenderable(void *data, Object obj)
{
    ZoomData zdata = (ZoomData)data;
    DXReference(obj);
    if (zdata->obj)
	DXDelete(zdata->obj);
    zdata->obj = obj;
}
    

static int
ZoomGetRenderable(void *data, Object *obj)
{
    ZoomData zdata = (ZoomData)data;
    if (zdata->obj)
    {
	*obj = zdata->obj;
	return 1;
    }
    else
    {
	*obj = NULL;
	return 0;
    }
}

static void
ZoomEventHandler(void *data, DXEvent *event)
{
    ZoomData zdata = (ZoomData)data;

    switch(event->any.event)
    {
	case DXEVENT_LEFT:
	case DXEVENT_MIDDLE:
	case DXEVENT_RIGHT:
	    {
		int b = WHICH_BUTTON(event);

		switch (event->mouse.state)
		{
		    case BUTTON_DOWN:
			ZoomStartStroke(data, (DXMouseEvent *)event);
			zdata->buttonStates[b] = BUTTON_DOWN;
			break;
		    
		    case BUTTON_MOTION:
			if (zdata->buttonStates[b] == BUTTON_UP)
			    ZoomStartStroke(data, (DXMouseEvent *)event);
			else
			    ZoomEndStroke(data, (DXMouseEvent *)event);
			zdata->buttonStates[b] = BUTTON_DOWN;
			break;

		    case BUTTON_UP:
			ZoomEndStroke(data, (DXMouseEvent *)event);
			zdata->buttonStates[b] = BUTTON_UP;
			break;

		    default:
			break;
		}
	
	    }
	    break;
		
        default:
            break;
    }
}

static void 
ZoomStartStroke(void *data, DXMouseEvent *event)
{
    ZoomData zdata = (ZoomData)data;

    /*
     * Just keep track of which button is 
     * pressed and where it was pressed.
     */
    int b = WHICH_BUTTON(event);
    if (b != NO_BUTTON)
    {
        zdata->buttonPositions[b].x = event->x;
        zdata->buttonPositions[b].y = event->y;
    }
    return;
}

static void
ZoomEndStroke(void *data, DXMouseEvent *event)
{
    float dy;
    int b = WHICH_BUTTON(event);

    ZoomData zdata = (ZoomData)data;
    if (! zdata)
	return;

    zdata->motionFlag = (event->state == BUTTON_MOTION);

    dy = (event->y - zdata->buttonPositions[b].y) /
			((float) zdata->h);

    if (event->x != zdata->buttonPositions[b].x ||
	event->y != zdata->buttonPositions[b].y)
    {
	if (zdata->p)
	{
	    zdata->from[0] += 0.9*dy*(zdata->to[0] - zdata->from[0]);
	    zdata->from[1] += 0.9*dy*(zdata->to[1] - zdata->from[1]);
	    zdata->from[2] += 0.9*dy*(zdata->to[2] - zdata->from[2]);
	}
	else
	{
	    zdata->width -= dy * zdata->init_width;
	}
    }

    zdata->buttonPositions[b].x = event->x;
    zdata->buttonPositions[b].y = event->y;

    return;

}
