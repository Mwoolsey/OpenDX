/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>

#if defined(DX_NATIVE_WINDOWS)

#include <dx/dx.h>
#include <windows.h>

#define ImageWindow int

struct _iwx
{
    int 		win_x, win_y;
};


static int _dxf_getBestVisual(ImageWindow *);
static int _dxf_processEvents(int, void *);

int
_dxf_samplePointer(ImageWindow *iw, int flag)
{
	DXSetError(ERROR_NOT_IMPLEMENTED, "Not available for Windows native yet.");
	return 0;
}

static int
_dxf_processEvents(int fd, void *d)
{
	DXSetError(ERROR_NOT_IMPLEMENTED, "Not available for Windows native yet.");
	return 0;
}

void
_dxf_deleteWindowX(ImageWindow *iw)
{
	DXSetError(ERROR_NOT_IMPLEMENTED, "Not available for Windows native yet.");
	return 0;
}
    
int
_dxf_mapWindowX(ImageWindow *iw, int m)
{
	DXSetError(ERROR_NOT_IMPLEMENTED, "Not available for Windows native yet.");
	return 0;
}

int
_dxf_getParentSize(char *displayString, int wid, int *width, int *height)
{
	DXSetError(ERROR_NOT_IMPLEMENTED, "Not available for Windows native yet.");
	return 0;
}

void
_dxf_setWindowSize(ImageWindow *iw, int *size)
{
	DXSetError(ERROR_NOT_IMPLEMENTED, "Not available for Windows native yet.");
	return 0;
}

void
_dxf_setWindowOffset(ImageWindow *iw, int *offset)
{
	DXSetError(ERROR_NOT_IMPLEMENTED, "Not available for Windows native yet.");
	return 0;
}

int
_dxf_createWindowX(ImageWindow *iw, int map)
{
	DXSetError(ERROR_NOT_IMPLEMENTED, "Not available for Windows native yet.");
	return 0;
}

int
_dxf_getWindowId(ImageWindow *iw)
{
	DXSetError(ERROR_NOT_IMPLEMENTED, "Not available for Windows native yet.");
	return 0;
}

#define ISACCEPTABLE(ret, dpth, clss)				\
{								\
    ret = 0; 							\
    for(i = 0; !ret && acceptableVisuals[i].depth; i++)		\
	if (acceptableVisuals[i].depth == (dpth) &&		\
	    acceptableVisuals[i].class == (clss))  		\
	     ret = 1;						\
}

int
_dxf_checkDepth(ImageWindow *iw, int d)
{
	DXSetError(ERROR_NOT_IMPLEMENTED, "Not available for Windows native yet.");
	return 0;
}
	

    
static int
_dxf_getBestVisual (ImageWindow *iw)
{
	DXSetError(ERROR_NOT_IMPLEMENTED, "Not available for Windows native yet.");
	return 0;
}

#endif /* DX_NATIVE_WINDOWS */
