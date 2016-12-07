/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>




#include <dx/dx.h>
#include <math.h>
#include <string.h>

#define WHERE_IN 	in[0]
#define DEFAULT_CAMERA 	in[1]
#define RESET_CAMERA  	in[2]
#define DEFAULT_OBJECT 	in[3]
#define RESET_OBJECT 	in[4]
#define WINDOW_SIZE     in[5]
#define MOUSE           in[6]
#define MODE            in[7]
#define ARGS            in[8]

#define OBJECT		out[0]
#define CAMERA		out[1]
#define WHERE_OUT	out[2]
#define EVENTS		out[3]

#define CACHE_CAMERA	"CACHED_CAMERA_"
#define CACHE_OBJECT	"CACHED_OBJECT_"

static Error Spin(Camera, Camera *, Object, Object *,
		int, DXEvents, int *, int, Object, DXEvents *, int *);

extern Error DXRegisterForNotification(char *, Pointer); /* from libdx/notify.c */

extern int _dxd_nUserInteractors;
extern void *_dxd_UserInteractors;

int
m_SuperviseState(Object *in, Object *out)
{
    char *where, *cameraCacheTag = NULL, *objectCacheTag = NULL;
    Pointer modid = NULL;
    int resetCamera, resetObject;
    int mode;
    int windowSz[2];
    char old[256], new[256];
    char *left, *right;

    CAMERA = NULL;
    WHERE_OUT = NULL;
    EVENTS = NULL;

    if (!WHERE_IN || !DXExtractString(WHERE_IN, &where))
    {
	DXSetError(ERROR_MISSING_DATA, "where parameter");
	return ERROR;
    }

    if (! strstr(where, "#X"))
    {
	DXSetError(ERROR_BAD_PARAMETER, "where must be a SuperviseWindow where output");
	goto error;
    }

    left = strchr(where, '[');
    if (left && ((right = strchr(where, ']')) != NULL) && right > left)
    {
	Object o;
	char oTag[512], nTag[512];

	strncpy(new, where, (left - where));
	new[left-where] = '\0';
	strncpy(old, left+1, (right - (left+1)));
	old[right - (left+1)] = '\0';

	DXMessage("old: %s new %s", old, new);

	where = new;

	sprintf(oTag, "%s%s", CACHE_CAMERA, old);
	sprintf(nTag, "%s%s", CACHE_CAMERA, new);
	if (NULL != (o = DXGetCacheEntry(oTag, 0, 0)))
	{
	    DXSetCacheEntry(o, CACHE_PERMANENT, nTag, 0, 0);
	    DXSetCacheEntry(NULL, CACHE_PERMANENT, oTag, 0, 0);
	}

	sprintf(oTag, "%s%s", CACHE_OBJECT, old);
	sprintf(nTag, "%s%s", CACHE_OBJECT, new);
	if (NULL != (o = DXGetCacheEntry(oTag, 0, 0)))
	{
	    DXSetCacheEntry(o, CACHE_PERMANENT, nTag, 0, 0);
	    DXSetCacheEntry(NULL, CACHE_PERMANENT, oTag, 0, 0);
	}

	WHERE_OUT = (Object)DXNewString(new);
    }
    else
	WHERE_OUT = WHERE_IN;

    if (RESET_CAMERA)
    {
	if (! DXExtractInteger(RESET_CAMERA, &resetCamera))
	    DXSetError(ERROR_BAD_PARAMETER, "resetCamera");
    }
    else
	resetCamera = 0;
    
    cameraCacheTag = (char *)DXAllocate(strlen(where)+strlen(CACHE_CAMERA)+2);
    if (! cameraCacheTag)
	goto error;
    
    sprintf(cameraCacheTag, "%s%s", CACHE_CAMERA, where);

    if (! resetCamera)
    {
	Object lastCam;
	modid = DXGetModuleId();
	DXRegisterForNotification(cameraCacheTag, modid);
	DXFreeModuleId(modid);
	lastCam = DXGetCacheEntry(cameraCacheTag, 0, 0);
	CAMERA = DXCopy(lastCam, COPY_STRUCTURE);
	DXReference(CAMERA);
	DXDelete(lastCam);
    }

    if (! CAMERA)
    {
	if (! DEFAULT_CAMERA)
	{
	    DXSetError(ERROR_MISSING_DATA, 
		"either cached camera or default camera must be available");
	    goto error;
	}
	CAMERA = DXCopy((Object)DEFAULT_CAMERA, COPY_STRUCTURE);
	DXReference(CAMERA);
    }

    if (RESET_OBJECT)
    {
	if (! DXExtractInteger(RESET_OBJECT, &resetObject))
	    DXSetError(ERROR_BAD_PARAMETER, "resetObject");
    }
    else
	resetObject = 0;
    
    objectCacheTag = (char *)DXAllocate(strlen(where)+strlen(CACHE_OBJECT)+2);
    if (! objectCacheTag)
	goto error;
    
    sprintf(objectCacheTag, "%s%s", CACHE_OBJECT, where);

    if (! resetObject)
    {
	modid = DXGetModuleId();
	DXRegisterForNotification(objectCacheTag, modid);
	DXFreeModuleId(modid);
	OBJECT = DXGetCacheEntry(objectCacheTag, 0, 0);
    }

    if (! OBJECT)
    {
	if (! DEFAULT_OBJECT)
	{
	    DXSetError(ERROR_MISSING_DATA, 
		"either cached object or default object must be available");
	    goto error;
	}
	OBJECT = DXCopy((Object)DEFAULT_OBJECT, COPY_STRUCTURE);
	DXReference(OBJECT);
    }

    mode = -1;
    if (MODE && !DXExtractInteger(MODE, &mode))
    {
	DXSetError(ERROR_BAD_PARAMETER, "mode must be an int");
	goto error;
    }

    if (WINDOW_SIZE)
    {
	float w;

	if (! DXExtractParameter(WINDOW_SIZE, TYPE_INT, 2, 1, &windowSz))
	{
	    DXSetError(ERROR_DATA_INVALID, "windowsize");
	    goto error;
	}

	DXSetResolution((Camera)CAMERA, windowSz[0], 1.0);

	if (DXGetOrthographic((Camera)CAMERA, &w, NULL))
	    DXSetOrthographic((Camera)CAMERA, w, 
			(double)(windowSz[1]+0.99)/windowSz[0]);
	else
	{
	    DXGetPerspective((Camera)CAMERA, &w, NULL);
	    DXSetPerspective((Camera)CAMERA, w, 
			(double)(windowSz[1]+0.99)/windowSz[0]);
	}
    }
    else 
	DXGetCameraResolution((Camera)CAMERA, &windowSz[0], &windowSz[1]);


    if (mode >= _dxd_nUserInteractors)
    {
	DXSetError(ERROR_BAD_PARAMETER, 
	    "only %d user interactors are available", _dxd_nUserInteractors);
	goto error;
    }

    if ((mode >= 0) && MOUSE)
    {
	DXEvents dxEvents;
	int nDXEvents;
	Category c;
	Type t;
	int r, s;
	DXEvents uEvents;
	int nuEvents;

	if (DXGetObjectClass(MOUSE) != CLASS_ARRAY)
	{
	    DXSetError(ERROR_DATA_INVALID, "mouse must be an array");
	    return ERROR;
	}

	DXGetArrayInfo((Array)MOUSE, &nDXEvents, &t, &c, &r, &s);

	if (t != TYPE_INT || r != 1 || s != sizeof(DXEvent)/DXTypeSize(TYPE_INT))
	{
	    DXSetError(ERROR_DATA_INVALID,
		    "mouse must be an array of mouse events");
	    return ERROR;
	}

	dxEvents = DXGetArrayData((Array)MOUSE);

	if (!Spin((Camera)CAMERA, (Camera *)&CAMERA, OBJECT, &OBJECT,
			nDXEvents, dxEvents, windowSz, mode, ARGS,
			&uEvents, &nuEvents) ||
			! CAMERA || ! OBJECT)
	     goto error;

	if (nuEvents)
	{
	    EVENTS = (Object)DXNewArray(TYPE_INT, CATEGORY_REAL, 1, sizeof(DXEvent)/DXTypeSize(TYPE_INT));
	    DXAddArrayData((Array)EVENTS, 0, nuEvents, (Pointer)uEvents);
	    DXFree((Pointer)uEvents);
	}
    }

    DXSetCacheEntry(CAMERA, CACHE_PERMANENT, cameraCacheTag, 0, 0);
    DXSetCacheEntry(OBJECT, CACHE_PERMANENT, objectCacheTag, 0, 0);

    if (mode == -1)
    {
	DXSetAttribute(CAMERA, "interaction mode", NULL);
    }
    else
    {
	Group g = DXNewGroup();
	Array a;
	int *i;
	a = DXNewArray(TYPE_INT, CATEGORY_REAL, 0);
	DXAddArrayData(a, 0, 1, NULL);
	i = (int *)DXGetArrayData(a);
	*i = 9; /* INTERACTION_USER in hwClientMessage.c */
	DXSetMember(g, "mode", (Object)a);
	a = DXNewArray(TYPE_INT, CATEGORY_REAL, 0);
	DXAddArrayData(a, 0, 1, NULL);
	i = (int *)DXGetArrayData(a);
	*i = mode;
	DXSetMember(g, "submode", (Object)a);
	DXSetMember(g, "args", ARGS);

	DXSetAttribute(OBJECT, "interaction mode", (Object)g);
    }

    DXUnreference((Object)CAMERA);
    DXUnreference((Object)OBJECT);

    DXFree((Pointer)cameraCacheTag);
    DXFree((Pointer)objectCacheTag);

    return OK;

error:
    DXFree((Pointer)cameraCacheTag);
    DXFree((Pointer)objectCacheTag);

    return ERROR;
    
}

static Error
Spin(Camera in_camera, Camera *out_camera,
	   Object in_object, Object *out_object,
	   int nEvents, DXEvents events, int *wsz,
	   int mode, Object args,
	   DXEvents *uevents, int *nuevents)
{
    float aspect;
    float F[3], T[3], U[3];
    Point fp, tp;
    Vector uv;
    float width;
    int   pixwidth;
    float fov;
    int projection;

    *uevents = NULL;
    *nuevents = 0;

    DXGetView(in_camera, (Point *)F, (Point *)T, (Vector *)U);

    if (DXGetOrthographic(in_camera, &width, &aspect))
    {
	projection = 0;
    }
    else
    {
	DXGetPerspective(in_camera, &fov, &aspect);
	projection = 1;
    }
    DXGetCameraResolution(in_camera, &pixwidth, NULL);

    if (nEvents)
    {
	int i;
	void *udata = NULL;
	int mask;
	UserInteractor *imode = ((UserInteractor *)_dxd_UserInteractors) + mode;

	udata = imode->InitMode(args, wsz[0], wsz[1], &mask);

	if (imode->SetCamera)
	    imode->SetCamera(udata, T, F, U, projection, fov, width);
	    
	if (imode->SetRenderable)
	    imode->SetRenderable(udata, in_object);

	*nuevents = 0;

	for (i = 0; i < nEvents; i++)
	{
	    if (! (events[i].any.event & mask))
	    {
		(*nuevents)++;
		continue;
	    }

	    imode->EventHandler(udata, (DXEvent *)(events+i));
	}

	if (imode->GetCamera)
	    imode->GetCamera(udata, T, F, U, &projection, &fov, &width);
			
	if (imode->GetRenderable)
	{
	    Object tmp;

	    imode->GetRenderable(udata, &tmp);
	    if (tmp)
	    {
		DXDelete(in_object);
		in_object = DXReference(tmp);
		tmp = NULL;
	    }
	}

	imode->EndMode(udata);

	if (*nuevents)
	{
	    int i, j;

	    *uevents = DXAllocate(*nuevents * sizeof(DXEvent));
	    if (! *uevents)
		 return ERROR;
	    
	    for (i = j = 0; i < nEvents; i++)
		if (! (events[i].any.event & mask))
		    (*uevents)[j++] = events[i];
	}
    }

    if (out_object)
	*out_object = in_object;

    if (out_camera)
    {
	*out_camera = (Camera)DXCopy((Object)in_camera, COPY_STRUCTURE);
	if (! *out_camera)
	    return ERROR;
	
	DXReference((Object)*out_camera);

	aspect = (double)(wsz[1]+0.99)/wsz[0];

	fp.x = F[0]; fp.y = F[1]; fp.z = F[2];
	tp.x = T[0]; tp.y = T[1]; tp.z = T[2];
	uv.x = U[0]; uv.y = U[1]; uv.z = U[2];

	DXSetView(*out_camera, fp, tp, uv);
	if (projection)
	    DXSetPerspective(*out_camera, fov, aspect);
	else
	    DXSetOrthographic(*out_camera, width, aspect);
	
    }

    return OK;
}
