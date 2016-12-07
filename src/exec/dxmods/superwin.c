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

#include <dx/dx.h>
#include <time.h>
#include "superwin.h"

#define IN_NAME		in[0]
#define IN_DISPLAY	in[1]
#define IN_SIZE		in[2]
#define IN_OFFSET	in[3]
#define IN_PARENT	in[4]
#define IN_DEPTH	in[5]
#define IN_MAP		in[6]
#define IN_PICKFLAG	in[7]
#define IN_SIZEFLAG	in[8]
#define IN_OFFSETFLAG	in[9]
#define IN_DECORATIONS  in[10]

static ImageWindow *_dxf_getImageWindow(char *, Private *);
static ImageWindow *_dxf_createWindow(char *, char *, int, int *, int *,
				int, int, int, int, char *, Private *, int);

static Error  _dxf_deleteWindow(Pointer p);
static Object getResolution(ImageWindow *iw);
static Object getWhere(ImageWindow *iw, char *);
static Object getDXEvents(ImageWindow *iw);

int
m_SuperviseWindow(Object *in, Object *out)
{
    char *displayString = NULL;
    ImageWindow *iw = NULL;
    int parentId;
    char *name;
    Pointer modid = NULL;
    int size[2];
    int offset[2];
    int pick = 0;
    Private p = NULL;
    int sizeFlag = 0, sizeSet = 0;
    int offsetFlag = 0;
    char *title;
    int map, mapSet = 0;
    char *oldWhere = NULL;
    int depth;
    int decorations;

    if (IN_DISPLAY)
    {
	if (! DXExtractString(IN_DISPLAY, &displayString))
	{
	    DXSetError(ERROR_DATA_INVALID, "displayString");
	    goto error;
	}
    }
    else if (NULL == (displayString = (char *)getenv("DISPLAY")))
	displayString = "localhost";
    
    if (IN_NAME)
    {
	if (! DXExtractString(IN_NAME, &name))
	{
	    DXSetError(ERROR_DATA_INVALID, "windowName must be string");
	    goto error;
	}
	title = name;
    }
    else
    {
	modid = DXGetModuleId();
	name = (char *)modid;
	title = "SuperviseWindow";
    }
    
    if (IN_SIZE)
    {
	if (DXGetObjectClass(IN_SIZE) == CLASS_CAMERA)
	    DXGetCameraResolution((Camera)IN_SIZE, &size[0], &size[1]);
	else
	    if (! DXExtractParameter(IN_SIZE, TYPE_INT, 2, 1, (Pointer)&size))
	    {
		DXSetError(ERROR_DATA_INVALID, "size");
		goto error;
	    }

	sizeSet = 1;
    }
    else
	size[0] = 640, size[1] = 480;

    if (IN_SIZEFLAG)
    {
	if (! DXExtractInteger(IN_SIZEFLAG, &sizeFlag))
	{
	    DXSetError(ERROR_DATA_INVALID, "sizeFlag");
	    goto error;
	}
    }
    else
	sizeFlag = 0;

    if (IN_OFFSET)
    {
	if (! DXExtractParameter(IN_OFFSET, TYPE_INT, 2, 1, (Pointer)&offset))
	{
	    DXSetError(ERROR_DATA_INVALID, "offset");
	    goto error;
	}
    }
    else
	offset[0] = 0, offset[1] = 0;

    if (IN_OFFSETFLAG)
    {
	if (! DXExtractInteger(IN_OFFSETFLAG, &offsetFlag))
	{
	    DXSetError(ERROR_DATA_INVALID, "offsetFlag");
	    goto error;
	}
    }
    else
	offsetFlag = 0;

    if (IN_PICKFLAG)
	if (! DXExtractInteger(IN_PICKFLAG, &pick))
	{
	    DXSetError(ERROR_DATA_INVALID, "pick");
	    goto error;
	}


    if (IN_PARENT)
    {
	if (! DXExtractInteger(IN_PARENT, &parentId))
	{
	    char *str;
	    int i;
	    if (! DXExtractString(IN_PARENT, &str))
	    {
		DXSetError(ERROR_BAD_PARAMETER,"parent");
		goto error;
	    }
	    for (i = 0; str[i] && str[i] != '#'; i++);
	    if (str[i])
	    {
		if (str[i+1] != 'X' && str[i+1] != '#')
		{
		    DXSetError(ERROR_BAD_PARAMETER,"parent");
		    goto error;
		}
		if(sscanf(str+(i+2), "%d", &parentId) != 1) {
                    DXSetError(ERROR_BAD_PARAMETER, "parent");
                    goto error;
                }

		if (! sizeSet)
		    if (! _dxf_getParentSize(displayString,
					parentId, &size[0], &size[1]))
		    goto error;
	    }
	    else
		parentId = -1;
	}
    }
    else
	parentId = -1;

    if (IN_MAP)
    {
	if (! DXExtractInteger(IN_MAP, &map))
	{
	    DXSetError(ERROR_DATA_INVALID, "visibility");
	    goto error;
	}

	mapSet = 1;
    }
    else
	map = WINDOW_OPEN;


    if (IN_DECORATIONS)
    {
	if (! DXExtractInteger(IN_DECORATIONS, &decorations))
	{
	    DXSetError(ERROR_DATA_INVALID, "decorations");
	    goto error;
	}
    }
    else
	decorations = 1;

    if (IN_DEPTH)
    {
	if (! DXExtractInteger(IN_DEPTH, &depth))
	{
	    DXSetError(ERROR_DATA_INVALID, "depth");
	    goto error;
	}
    }
    else if (getenv("DX_WINDOW_DEPTH"))
	depth = atoi(getenv("DX_WINDOW_DEPTH"));
    else
	depth = 24;
    

    iw = _dxf_getImageWindow(name, &p);
    if (iw &&
	((iw->parentId != parentId) ||
	(iw->depth != depth && IN_DEPTH && _dxf_checkDepth(iw, depth))))
    {
	size[0] = iw->size[0];
	size[1] = iw->size[1];
	offset[0] = iw->offset[0];
	offset[1] = iw->offset[1];
	oldWhere = iw->where;
	iw->where = NULL;
	DXDelete(iw->object);
	iw = NULL;
    }

    if (! iw)
    {
	iw = _dxf_createWindow(displayString, title, depth, size, offset,
			sizeFlag, offsetFlag, parentId, map, name, &p, decorations);

	if (! iw)
	    goto error;
    }
    else
    {
	if (IN_SIZE && sizeFlag &&
		    (iw->size[0] != size[0] || iw->size[1] != size[1]))
	    _dxf_setWindowSize(iw, size);
	
	if (IN_OFFSET && offsetFlag &&
		    (iw->offset[0] != offset[0] || iw->offset[1] != offset[1]))
	    _dxf_setWindowOffset(iw, offset);

	if (mapSet)
	    _dxf_mapWindowX(iw, map);
    }

    iw->pickFlag = pick;

    out[0] = getWhere(iw, oldWhere);
    out[1] = getResolution(iw);
    out[2] = getDXEvents(iw);
    if (DXGetError() != ERROR_NONE)
	goto error;

    if (out[2])
    {
	if (! modid)
	    modid = DXGetModuleId();
	DXReadyToRunNoExecute(modid);
    }

    if (oldWhere)
	DXFree((Pointer)oldWhere);

    DXDelete((Object)p);

    if (modid) DXFreeModuleId(modid);

    return OK;

error:
    if (modid) DXFreeModuleId(modid);
    return ERROR;
}
	
static Error
_dxf_deleteWindow(Pointer p)
{
    ImageWindow *iw = (ImageWindow *)p;
    _dxf_deleteWindowX(p);
    DXFree(iw->iwx);
    if (iw->where)
    {
	DXFree((void *)iw->where);
	iw->where = NULL;
    }
    DXFreeModuleId(iw->mod_id);
    DXFree((void *)iw->cacheTag);
    DXFree((void *)iw);
    return OK;
}


char *getCacheTag(char *name)
{
    char *buf = (char *)DXAllocate(strlen(name) + strlen("_DXWINDOW") + 4);
    sprintf(buf, "_DXWINDOW_%s", name);
    return buf;
}

static ImageWindow *
_dxf_getImageWindow(char *name, Private *p)
{
    ImageWindow *iw;
    char *tag = getCacheTag(name);

    *p = (Private) DXGetCacheEntry(tag, 0, 0);
    if (*p)
	iw = (ImageWindow *)DXGetPrivateData(*p);
    else
	iw = NULL;
    
    DXFree((Pointer)tag);

    return iw;
}

static ImageWindow *
_dxf_createWindow(char *displayString,
		  char *title,
		  int depth, 
		  int *size,
		  int *offset,
		  int sizeFlag,
		  int offsetFlag,
		  int parentId,
		  int map, 
		  char *name,
		  Private *p,
		  int decorations)
{
    ImageWindow *iw = NULL;
    int i;
    char buf[1024];

    iw = (ImageWindow *)DXAllocateZero(sizeof(ImageWindow));
    if (! iw)
	goto error;

    iw->depth = depth;
    iw->size[0] = size[0];
    iw->size[1] = size[1];
    iw->sizeFlag = sizeFlag;
    iw->offset[0] = offset[0];
    iw->offset[1] = offset[1];
    iw->offsetFlag = offsetFlag;
    iw->parentId = parentId;
    iw->decorations = decorations;

    iw->events = NULL;
    for (i = 0; i < 3; i++)
	iw->state[i] = BUTTON_UP;

    iw->displayString = (char *)DXAllocate(strlen(displayString)+1);
    strcpy(iw->displayString, displayString);

    iw->title = (char *)DXAllocate(strlen(title)+1);
    strcpy(iw->title, title);

    if (! _dxf_createWindowX(iw, map))
    {
	DXSetError(ERROR_INTERNAL, "unable to create window");
	goto error;
    }

    *p = DXNewPrivate((Pointer)iw, _dxf_deleteWindow);
    if (! *p)
	goto error;
    
    DXReference((Object)*p);

    iw->mod_id = DXGetModuleId();

    iw->cacheTag = getCacheTag(name);
    iw->object = (Object)*p;

    sprintf(buf, "X%d,%s,#X%d", iw->depth, iw->displayString, _dxf_getWindowId(iw));
    iw->where = DXAllocate(strlen(buf) + 1);
    strcpy(iw->where, buf);

    if (! DXSetCacheEntry((Object)*p, CACHE_PERMANENT, iw->cacheTag, 0, 0))
	goto error;

    return iw;

error:

    DXFree((Pointer)iw);
    return NULL;
}

Error
saveEvent(ImageWindow *iw, int event, int state)
{
    Array b = iw->events;
    int n;
    DXEvent ev;

    if (!b)
    {
	iw->events = DXNewArray(TYPE_INT, CATEGORY_REAL, 1, sizeof(DXEvent)/DXTypeSize(TYPE_INT));
	b = iw->events;
    }

    switch (event)
    {
	case DXEVENT_LEFT:
	case DXEVENT_MIDDLE:
	case DXEVENT_RIGHT:
	{
	    DXMouseEvent *mev = (DXMouseEvent *)&ev;
	    mev->event      = event;
	    mev->x          = iw->x;
	    mev->y          = iw->y;
	    mev->kstate     = iw->kstate;
	    mev->state      = state;
	    break;
	}
	
	case DXEVENT_KEYPRESS:
	{
	    DXKeyPressEvent *kpev = (DXKeyPressEvent *)&ev;
	    kpev->event = event;
	    kpev->x     = iw->x;
	    kpev->y     = iw->y;
	    kpev->key   = state;
	    kpev->kstate = iw->kstate;
	    break;
	}
    }

    DXGetArrayInfo(b, &n, NULL, NULL, NULL, NULL);
    if (! DXAddArrayData(b, n, 1, (Pointer)&ev))
	goto error;


    return OK;

error:
    return ERROR;
}

static Object
getDXEvents(ImageWindow *iw)
{
    Object result;

    if (! _dxf_samplePointer(iw, 1))
	return NULL;

    result = (Object)iw->events;
    iw->events = NULL;

    if (! _dxf_samplePointer(iw, 0))
	return NULL;

    return (Object)result;
}

static Object
getWhere(ImageWindow *iw, char *oldWhere)
{
    Object where;
    if (oldWhere)
    {
	char buf[1024];
	sprintf(buf, "%s[%s]", iw->where, oldWhere);
	where = (Object)DXNewString(buf);
    }
    else
	where = (Object)DXNewString(iw->where);

    return where;
}

static Object
getResolution(ImageWindow *iw)
{
    Array a = DXNewArray(TYPE_INT, CATEGORY_REAL, 1, 2);
    DXAddArrayData(a, 0, 1, (Pointer)iw->size);
    return (Object)a;
}

int
_dxf_mapSupervisedWindow(char *name, int map)
{
    ImageWindow *iw = NULL;
    Private p = NULL;

    if (! name)
	return 0;
    
    iw = _dxf_getImageWindow(name, &p);
    if (! iw)
	return 0;
    
    _dxf_mapWindowX(iw, map);

    DXDelete((Object)p);
    return OK;
}

