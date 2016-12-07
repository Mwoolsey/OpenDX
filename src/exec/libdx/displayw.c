/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 2002                                        */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>

#if defined(DX_NATIVE_WINDOWS)

#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include <stdlib.h>

#include "displayw.h"
#include "displayutil.h"
#include "internals.h"
#include "render.h"
#include "../../../VisualDX/dxexec/resource.h"

#define WINDOW_UI	1
#define WINDOW_EXTERNAL 2
#define WINDOW_LOCAL    3

static char *SWWindowClassName = NULL;
static char *SWDefaultTitle = "Data Explorer";
static HINSTANCE SWWindowClassRegistered = (HINSTANCE)0;

#ifdef _WIN32
#define GetSWWPtr(hwnd)		(SWWindow *)GetWindowLong((hwnd),0)
#define SetSWWPtr(hwnd,ptr)	SetWindowLong((hwnd),0,(LONG)(ptr))
#else
#define GetSWWPtr(hwnd)		(SWWindow *)GetWindowLong((hwnd),0)
#define SetSWWPtr(hwnd,ptr)	SetWindowLong((hwnd),0,(WORD)(ptr))
#endif

void *
_dxf_GetTranslation(Object o)
{
    translationT *d;

    if(o) {
      d = (void *) DXGetPrivateData((Private)o);
      return d;
    } else
      return NULL;
}

static LONG WINAPI 
SWWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{

	HDC hdc;
	SWWindow *sww = GetSWWPtr(hWnd);	
	static int cxClient, cyClient;
	PAINTSTRUCT ps;
	int r;

	switch (msg)
	{
		case WM_CREATE:
			return 0;

		case WM_LBUTTONDOWN:
		case WM_MBUTTONDOWN:
		case WM_RBUTTONDOWN:	
		case WM_LBUTTONUP:
		case WM_MBUTTONUP:
		case WM_RBUTTONUP:
		case WM_KEYUP:
		case WM_KEYDOWN:
		case WM_MOUSEMOVE: 
			if (sww && sww->parent)
				SendMessage(sww->parent, msg, wParam, lParam);
			break;

		case WM_SIZE:
			if (sww && sww->parent)
				SendMessage(sww->parent, msg, wParam, lParam);
			break;

		case WM_NCHITTEST:
			break;
				
		case WM_PAINT:
			hdc = BeginPaint(hWnd, &ps);

			if(sww->refresh == 1) {
				sww->refresh = 0;
			}
			
			if (sww->bitmap)
			{
				int ind = 4;
				
				RealizePalette(hdc);
				
				/*
				r = StretchDIBits(hdc, 0, 0, cxClient, cyClient,
					0, 0, sww->wwidth, sww->wheight, 
					(const void *)sww->pixels, 
					(const BITMAPINFO *)sww->bmi, DIB_RGB_COLORS, 
					(DWORD)SRCCOPY);
*/
				SetDIBitsToDevice(hdc, 0, 0, sww->pwidth, sww->pheight, 
					0, 0, 0, sww->wheight,
					(const void *)sww->pixels,
					(const BITMAPINFO *)sww->bmi, 
					DIB_RGB_COLORS);
/*				
				if (r == GDI_ERROR)
					r = GetLastError();
*/	
			}
			EndPaint(hWnd, &ps);
			return 0;

		case WM_DESTROY:
			destroy_window(sww);
			sww->refresh = 1;

		case WM_ERASEBKGND:
			return 1;
			
	}
	
	return DefWindowProc(hWnd, msg, wParam, lParam);
}


static Error
_registerSWWindowClass()
{
	HINSTANCE hInstance;

	if (! DXGetWindowsInstance(&hInstance))
		return ERROR;


    if (hInstance != SWWindowClassRegistered)
    {
	    WNDCLASS wc;
		wc.style         = CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc   = (WNDPROC)SWWndProc;
		wc.cbClsExtra    = 0;
		wc.cbWndExtra    = sizeof(SWWindow *);
		wc.hInstance     = hInstance;
		wc.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_STARTUP));
		wc.hCursor       = NULL;
		wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
		wc.lpszMenuName  = NULL;
		
		if(SWWindowClassName)
			DXFree(SWWindowClassName);
		SWWindowClassName = (char *)DXAllocate(strlen("SoftwareRenderer_") + 20);
		sprintf(SWWindowClassName, "SoftwareRenderer_%d", (int)clock());
		
		wc.lpszClassName = SWWindowClassName;

		if (! RegisterClass(&wc))
		{
			LPVOID lpMsgBuf;
			DWORD dw = GetLastError();
			
			FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
				NULL, dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
				(LPTSTR) &lpMsgBuf,0, NULL);
			
			fprintf(stderr, "RegisterClass failed: %s\n", lpMsgBuf);

			return ERROR;
		}

        SWWindowClassRegistered = hInstance;
    }

    return OK;
}

    
    
Error 
_dxfCreateSWWindow (SWWindow *w)
{
	DWORD thread;
	HINSTANCE hInstance;

    if (!_registerSWWindowClass())
        goto error;

    if (! DXGetWindowsInstance(&hInstance))
		return ERROR;

	if (w->parent)
		w->hWnd = CreateWindow(SWWindowClassName,
			w->title,
			WS_CHILD | WS_CLIPCHILDREN,
			CW_USEDEFAULT, 0, w->wwidth, w->wheight,
			w->parent, NULL, hInstance, NULL);
	else
		w->hWnd = CreateWindow(SWWindowClassName,
			w->title,
			WS_OVERLAPPED | WS_MINIMIZEBOX | WS_CAPTION | WS_SYSMENU,
			CW_USEDEFAULT, 0, w->wwidth, w->wheight,
			w->parent, NULL, hInstance, NULL);

	SetSWWPtr(w->hWnd, w);
	if (! w->hWnd) {
		LPVOID lpMsgBuf;
		FormatMessage( 
			FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			FORMAT_MESSAGE_FROM_SYSTEM | 
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			(LPTSTR) &lpMsgBuf,
			0,
			NULL 
			);
		DXSetError(ERROR_INTERNAL, lpMsgBuf);
		// Free the buffer.
		LocalFree( lpMsgBuf );
		goto error;

	}

	w->hDC = GetDC(w->hWnd);

    return OK;
    
error:
    return ERROR;
}


void 
_dxfResizeSWWindow(SWWindow *w)
{ 

	if (w && w->hWnd > 0)
	{
		RECT r, oldrect;

		GetWindowRect(w->hWnd, &oldrect);

		r.left = oldrect.left;
		r.top = oldrect.top;
		r.right = r.left + w->wwidth;
		r.bottom = r.top + w->wheight;

		if(w->parent)
			AdjustWindowRect(&r, 
			WS_CHILD | WS_CLIPCHILDREN, 
			FALSE);
		else
			AdjustWindowRect(&r, 
			WS_OVERLAPPED | WS_MINIMIZEBOX | WS_CAPTION | WS_SYSMENU, 
			FALSE);
		
		if(r.left < 0) {
			r.right -= r.left;
			r.left = 0;
		}
		if(r.top < 0) {
			r.bottom -= r.top;
			r.top = 0;
		}

		MoveWindow((HWND)w->hWnd,
			r.left, r.top,
			r.right - r.left, r.bottom - r.top,
			TRUE);
	}
}

unsigned char *
_dxf_GetWindowsPixels(Field f, int *size)
{
    Array a;

    a = (Array)DXGetComponentValue(f, IMAGE);
    if (! a || DXGetAttribute((Object)a, IMAGE_TYPE) != O_X_IMAGE)
    {
    	DXSetError(ERROR_BAD_PARAMETER, "#11610");
		goto error;
    }

    if (DXTypeCheck(a, TYPE_UBYTE, CATEGORY_REAL, 0))
	{
		if (size) *size = 1;
		return (unsigned char *)DXGetArrayData(a);
	}
	else if (DXTypeCheck(a, TYPE_USHORT, CATEGORY_REAL, 0))
	{
		if (size) *size = 2;
		return (unsigned char *)DXGetArrayData(a);
	}
    else if (DXTypeCheck(a, TYPE_UBYTE, CATEGORY_REAL, 1, 3))
	{
		if (size) *size = 3;
		return (unsigned char *)DXGetArrayData(a);
	}
    else if (DXTypeCheck(a, TYPE_UINT, CATEGORY_REAL, 0))
	{
		if (size) *size = 4;
		return (unsigned char *)DXGetArrayData(a);
	}
	else
    {
    	DXSetError(ERROR_BAD_PARAMETER, "#11610");
		goto error;
    }

    return (unsigned char *)DXGetArrayData(a);

error:
    return NULL;
}

static int
_dxf_GetBytesPerPixel(translationP t)
{
	return t->bytesPerPixel;
}

static Error
getBestVisual(struct window *w, translationP t)
{
	int bitsPerPixel = GetDeviceCaps(w->hDC, BITSPIXEL);
	int rasterCaps = GetDeviceCaps(w->hDC, RASTERCAPS);
	int palette = rasterCaps & RC_PALETTE;

	if (w->windowMode == WINDOW_UI)
		bitsPerPixel = 24;

	if (t->depth == 0)
		t->depth = bitsPerPixel;
 
	t->bytesPerPixel = bitsPerPixel >> 3;

	if (palette)
		t->translationType = mapped;
	else if (bitsPerPixel >= 24)
	{
		t->translationType = rgb888;
	}
	else
	{
		t->translationType = rgbt;
		
		if (bitsPerPixel < 3)
		{
			DXSetError(ERROR_INTERNAL, "at least 3 bits per pixel is required");
			goto error;
		}

		t->blueBits = bitsPerPixel / 3;
		bitsPerPixel -= t->blueBits;
		t->redBits = bitsPerPixel / 2;
		t->greenBits = bitsPerPixel - t->redBits;

		t->blueShift = 0;
		t->greenShift = t->blueBits;
		t->redShift = (t->blueBits + t->greenBits);
	}

	return OK;

error:
	return ERROR;
}

static Error
CheckColormappedImage(Object image, Array colormap)
{
    Array a;
    int n;
    Type t;
    Category c;
    int r, s[32];

    if (image)
    {
		if (DXGetObjectClass(image) != CLASS_FIELD)
		{
			DXSetError(ERROR_INVALID_DATA, "color-mapped images must be fields");
			goto error;
		}

		a = (Array)DXGetComponentValue((Field)image, "colors");
		if (! a)
		{
			DXSetError(ERROR_INVALID_DATA, "missing colors component");
			goto error;
		}

		if (NULL == DXGetArrayInfo(a, NULL, &t, &c, &r, s))
			return NULL;

		if ((t != TYPE_BYTE && t != TYPE_UBYTE) ||
			(c != CATEGORY_REAL) ||
			(r != 0 && (r != 1 || s[0] != 1)))
		{
			DXSetError(ERROR_INVALID_DATA,
			  "colormapped images must be byte or ubyte, scalar or 1-vector");
			return NULL;
		}
    }

    if (colormap)
    {
		if (DXGetObjectClass((Object)colormap) != CLASS_ARRAY)
		{
			DXSetError(ERROR_INVALID_DATA,
			  "image colormap images must be an Array");
			goto error;
		}

		DXGetArrayInfo(colormap, &n, &t, &c, &r, s);

		if ((n > 256) ||
			(t != TYPE_FLOAT) ||
			(c != CATEGORY_REAL) ||
			(r != 1) ||
			(s[0] != 1 && s[0] != 3))
		{
			DXSetError(ERROR_INVALID_DATA,
				"image colormap images must be >=256 float 3-vectors");
			goto error;
		}
    }

    return OK;

error:
    return ERROR;
}

#define WINDOW_TEMPLATE "Window Software X%s"

static char * 
getWindowCacheTag(char *title)
{
    char *cacheid = (char *)DXAllocate(strlen(WINDOW_TEMPLATE) +
				        strlen(title) + 1);
    if (! cacheid)
		return NULL;
    
    sprintf(cacheid, WINDOW_TEMPLATE, title);

    return cacheid;
}

#define WINDOW_ID_TEMPLATE "Display Window ID %s"

static char * 
getWindowIDCacheTag()
{
    char *modid;
    char *cacheid;

    modid = (char *)DXGetModuleId();

    cacheid = (char *)DXAllocate(strlen(WINDOW_ID_TEMPLATE) +
				 strlen(modid) + 1);

    sprintf(cacheid, WINDOW_ID_TEMPLATE, modid);

    DXFreeModuleId((Pointer)modid);

    if (! cacheid)
	return NULL;

    return cacheid;
}

static Error
free_private_string(Pointer p)
{
    DXFree(p);
    p = NULL;
    return OK;
}


static Error 
setWindowIDInCache(char *tag, char *title)
{
    Private p = NULL;
    char *tmp = NULL;
    
    tmp = (char *)DXAllocate(strlen(title) + 1);
    if (! tmp)
		goto error;
    
    strcpy(tmp, title);

    p = DXNewPrivate((Pointer)tmp, free_private_string);
    if (! p)
		goto error;
    tmp = NULL;
    
    if (! DXSetCacheEntry((Object)p, CACHE_PERMANENT, tag, 0, 0))
		goto error;
    p = NULL;

    return OK;

error:
    DXFree((Pointer)tmp);
    DXDelete((Object)p);
    return ERROR;
}

static char *
getWindowIDFromCache(char *tag)
{
    Private p = NULL;
    char *tmp, *p_data;

    p = DXGetCacheEntry(tag, 0, 0);
    
    if (! p)
		return NULL;

    p_data = (char *)DXGetPrivateData(p);

    tmp = (char *)DXAllocate(strlen(p_data) + 1);
    if (! tmp)
		goto error;
    
    strcpy(tmp, p_data);
    DXDelete((Object)p);

    return tmp;

error:
    DXDelete((Object)p);
    return NULL;
}

static Error
_DXSetSoftwareWindowsWindowActive(SWWindow *w, int active)
{
	if(w->hWnd <= 0) {
		_dxfCreateSWWindow(w);
	} else {
		if (active)
			ShowWindow(w->hWnd, SW_SHOW);
		else
			ShowWindow(w->hWnd, SW_HIDE);
	}
	
	return OK;
}

Error
_dxf_SetSoftwareWindowsWindowActive(char *where, int active)
{
    char *cachetag = NULL;
    Private p = NULL;
    char *host = NULL, *title = NULL;

    if (! DXParseWhere(where, NULL, NULL, &host, &title, NULL, NULL))
	 goto error;

    cachetag = getWindowCacheTag(title);

    p = (Private)DXGetCacheEntry(cachetag, 0, 0);
    DXFree((Pointer)cachetag);
    cachetag = NULL;

    if (p)
    {
	struct window *w = (struct window *)DXGetPrivateData(p);

	if (! _DXSetSoftwareWindowsWindowActive(w, active))
	    goto error;

	DXDelete((Object)p);
    }

    DXFree((Pointer)host);
    DXFree((Pointer)title);
    host = title = NULL;

    return OK;

error:
    DXDelete((Object)p);
    DXFree((Pointer)host);
    DXFree((Pointer)title);
    host = title = NULL;

    return ERROR;
}
	
HWND 
DXGetWindowsHandle(char *where)
{
	HWND win = NULL;
	char *cachetag = NULL;
    Private p = NULL;
    char *host = NULL, *title = NULL;

    if (! DXParseWhere(where, NULL, NULL, &host, &title, NULL, NULL))
	 return NULL;

    cachetag = getWindowCacheTag(title);

    p = (Private)DXGetCacheEntry(cachetag, 0, 0);
    DXFree((Pointer)cachetag);
    cachetag = NULL;

	if (p)
	{
		struct window *w = (struct window *)DXGetPrivateData(p);
		win = w->hWnd;

		DXDelete((Object)p);
	}

    DXFree((Pointer)host);
    DXFree((Pointer)title);
    host = title = NULL;

    return win;
}


Error
DXSetSoftwareWindowsWindowActive(char *where, int active)
{
    return _dxf_SetSoftwareWindowsWindowActive(where, active);
}


static Error
delete_translation(Pointer p)
{
    translationT *d = (translationT *)p;

	if (d->cmap)
	{
		UnrealizeObject((HGDIOBJ)d->cmap);
		DeleteObject((HGDIOBJ)d->cmap);
	}

    DXFree(p);
    p = NULL;
    return OK;
}


static Error
getMappedTranslation(struct window *w)
{
	translationP t = w->translation;
	typedef struct tagLOGPALETTE { // lgpl 
		WORD         palVersion; 
		WORD         palNumEntries; 
		PALETTEENTRY palPalEntry[225]; 
	} LOGPALETTE; 

	LOGPALETTE	lplgpl;
    int 		i, comp[256];
	int			r, rr;
	int			g, gg;
	int			b, bb;
	float		invgamma = 1.0 / t->gamma;

	rr = 5;
	gg = 9;
	bb = 5;

	t->redRange = 5;
	t->greenRange = 9;
	t->blueRange = 5;

	lplgpl.palVersion = 0x300;
	lplgpl.palNumEntries = 225;

	/*
     * gamma compensation
     */
    for (i=0; i<256; i++)
		comp[i] = 255*pow((i/255.0), invgamma);

	for (r=0; r<rr; r++)
	    for (g=0; g<gg; g++)
			for (b=0; b<bb; b++)
			{
				int p = MIX(r, g, b);

				lplgpl.palPalEntry[p].peRed   = comp[r*255/(rr-1)];
				lplgpl.palPalEntry[p].peGreen = comp[g*255/(gg-1)];
				lplgpl.palPalEntry[p].peBlue  = comp[b*255/(bb-1)];
			}

	t->cmap = CreatePalette((const LOGPALETTE *)&lplgpl);
	SelectPalette(w->hDC, t->cmap, 0);

	return (t->cmap == NULL) ? ERROR : OK;
}

static Error
getDirectTranslation(translationP t)
{
	DXSetError(ERROR_INTERNAL, "no direct translation yet");
	return ERROR;
}

static Error
getRGBTranslation(translationP t)
{
	int i;
	float invgamma = 1.0 / t->gamma;

	t->table = (unsigned long *)DXAllocate(1024*sizeof(unsigned long));
	if (! t->table)
		goto error;

	t->redRange   = (1 << t->redBits) - 1;
	t->greenRange = (1 << t->greenBits) - 1;
	t->blueRange  = (1 << t->blueBits) - 1;

	t->rtable = t->table;
	t->gtable = t->table + 256;
	t->btable = t->table + 512;

	t->gammaTable = t->table + 768;

	for (i = 0; i < t->redRange; i++)
		t->rtable[i] = i << t->redShift;

	for (i = 0; i < t->greenRange; i++)
		t->gtable[i] = i << t->greenShift;

	for (i = 0; i < t->blueRange; i++)
		t->btable[i] = i << t->blueShift;

	for (i = 0; i < 256; i++)
		t->gammaTable[i] = 255 * pow(i/255.0, invgamma);

	t->depth = t->redBits + t->greenBits + t->blueBits;

	return OK;
	
	
error:
	if (t->table)
		DXFree((Pointer)t->table);
	t->table = t->rtable = t->gtable = t->btable = t->gammaTable = NULL;
	return ERROR;
}

static Error
getRGB888Translation(translationP t)
{
		int i;
	float invgamma = 1.0 / t->gamma;

	t->table = (unsigned long *)DXAllocate(256*sizeof(unsigned long));
	if (! t->table)
		goto error;

	t->gammaTable = t->table;

	for (i = 0; i < 256; i++)
		t->gammaTable[i] = 255 * pow(i/255.0, invgamma);

#if 0
	t->depth = 24;
#endif

	return OK;
	
	
error:
	if (t->table)
		DXFree((Pointer)t->table);
	t->table = t->rtable = t->gtable = t->btable = t->gammaTable = NULL;
	return ERROR;
}

static void *
createTranslation(int desiredDepth, SWWindow *w)
{
	translationT 	*trans;
    Private 		p = NULL;
    HINSTANCE		hInstance;

    if (! DXGetWindowsInstance(&hInstance))
		return ERROR;

    w->translation = trans = (translationT *)DXAllocateZero(sizeof(translationT));
    if (! trans)
        goto error;
    
	p = DXNewPrivate((Pointer)trans, delete_translation);
	if (! p)
	{
	    DXFree((Pointer)trans);
	    trans = NULL;
	    goto error;
	}

	DXReference((Object)p);

	trans->depth = desiredDepth;
#if 0
	trans->invertY = 1;		/* all SW images are inverted */
#else
	trans->invertY = 0;
#endif
	trans->gamma = 2.0;
		
	if (! getBestVisual(w, w->translation))
		goto error;
 
	if (w->translation->translationType == direct)
	{
		if (! getDirectTranslation(w->translation))
			goto error;
	} 
	else if (w->translation->translationType == mapped)
	{
		if (! getMappedTranslation(w))
			goto error;
	}
	else if (w->translation->translationType == rgbt)
	{
		if (! getRGBTranslation(w->translation))
			goto error;
	}
	else if (w->translation->translationType == rgb888)
	{
		if (! getRGB888Translation(w->translation))
			goto error;
	}

#if 0
	/*
	 * If its not a UI window and its depth is wrong or a direct
	 * mapping is requested and this window can't handle it, delete the
	 * window.
	 */
	if (w && w->winFlag && w->windowMode == WINDOW_LOCAL &&
	    ((!w->translation) ||
	     (w->translation->depth != desiredDepth && desiredDepth != 0) ||
	     ((w->directMap == 1) && w->translation->translationType != direct) ||
	     ((w->directMap == 0) && w->translation->translationType == direct)))
	{
	}
#endif


	return (Object)p;

error:
	return NULL;
}

static Error
reset_window(struct window *w)
{

#if 0
	if (w->winFlag && w->windowMode == WINDOW_LOCAL)
#endif
	{
		DestroyWindow(w->hWnd);
    }
	
    return OK;
}

/*
 * We need two different delete window functions here. 
 * 1. We need one that is called from the Windows callback proc when the window
 *    is Destroyed.
 * 2. The other is needed as a callback for deleting the object from the cache and
 *    is attached with the DXNewPrivate call on the cache.
 */
 
static Error
destroy_window(struct window *w)
{
	if(w) {
		w->active = 0;
		w->hWnd = 0;
		
		//DXFree(w);
	}

/* Needs to remove the window object from the cache then delete the object itself */

	/* Try not deleting the cache. --
	char *cachetag = NULL;
	
	if(w->title) {
		cachetag = getWindowCacheTag(w->title);
		DXSetCacheEntry(NULL, CACHE_PERMANENT, cachetag, 0, 0);
	}
	DXFree(cachetag);
	
	  */

	return OK;
}

static Error
delete_window(Pointer p)
{
    struct window *w = (struct window *)p;

/* hDC */
    if (w->hDC)
		ReleaseDC(w->hWnd, w->hDC);

/* pixels will be deleted with the call to Delete the bitmap */

/* pixels_owner */
	if(w->pixels_owner)
		DXDelete(w->pixels_owner);
		
/* bitmap, DeleteObject will release the memory of the pixels. */
    if (w->bitmap)
		DeleteObject((HGDIOBJ)w->bitmap);

/* bmi */
	if(w->bmi)
		DXFree(w->bmi);
		
/* title */
    DXFree((Pointer)w->title);

/* translation_owner */
    if (w->translation_owner)
		DXDelete((Object)w->translation_owner);

/* cacheid */
    DXFree((Pointer)w->cacheid);

/* dpy_object */
    DXDelete((Object)w->dpy_object);


/* Any others */
    if (w->cmap_installed)
    {
		UnrealizeObject((HGDIOBJ)w->translation->cmap);
		w->cmap_installed = 0;
    }

    DXFree(p);
    p = NULL;

    return OK;

error:    
    return ERROR;
}

static Error
getWindowStructure(int depth, char *title, int directMap,
	SWWindow **window, Private *window_object, int required)
{
    Private p = NULL;
    char *cachetag = NULL;
    SWWindow *w = NULL;

    *window = NULL;
    *window_object = NULL;

    if (!title || !*title)
	    title = "OpenDX Image";
	 
    cachetag = getWindowCacheTag(title);

    /*
     * If this is a UI window, then check the "title" (which is in
     * fact ##windowid) against the previous title to see if we have
     * a new window Id.
     */
    if (title[0] == '#' && title[1] == '#')
    {
	    char *old_title = NULL, *title_tag = getWindowIDCacheTag();
 	        if (! title_tag)
			    goto error;

	    old_title = getWindowIDFromCache(title_tag);
	    if (old_title)
	    {
			/*
			 * If the two differ, throw away the cached window object
			 * and save new window id
			 */
			if (strcmp(title, old_title))
			{
				char *old_tag = getWindowCacheTag(old_title);

				if (! DXSetCacheEntry(NULL, 0, old_tag, 0, 0))
				{
				    DXFree((Pointer)old_title);
				    DXFree((Pointer)title_tag);
				    goto error;
				}
			
				DXFree((Pointer)old_tag);
				old_tag = NULL;

				/*
				 * and save new window id
				 */
				setWindowIDInCache(title_tag, title);
			 }

			DXFree((Pointer)old_title);
			old_title = NULL;

		}
		else
		    setWindowIDInCache(title_tag, title);

		DXFree((Pointer)title_tag);
		title_tag = NULL;
    }
		
    p = (Private)DXGetCacheEntry(cachetag, 0, 0);
    
    if (!p && required)
		return ERROR;

    if (p)
		w = (SWWindow *)DXGetPrivateData(p);

    
    /*
     * If there is a window, we need to determine whether the associated
     * translation is OK.
     */
#if 1
    if (w && w->translation && (depth == 0 || w->translation->depth == depth))
#else
    if (w)
#endif
	{
		/*
		 * If we received a direct colormap, then we need to update the
		 * translation colormap if the previous translation was of type
		 * direct and the new colormap is different from the
		 * previous colormap.  We need to try to create a *new* translation
		 * if the previous translation was not type direct.
         */
		if (directMap)
		{
			if (w->translation->translationType != direct)
			{
				DXDelete(w->translation_owner);
				w->translation = NULL;
				w->translation_owner = NULL;
				w->directMap = directMap;

				w->colormapTag = 0;

				w->translation_owner = (Object)createTranslation(depth, w);
		
				w->translation = _dxf_GetTranslation(w->translation_owner);
	    
				if (w->winFlag)
					 RealizePalette(w->hDC);
			}
		}
#if 0
		else if (w->translation->translationType == direct || (depth != 0 && w->translation->depth != depth))
#else
		else if (w->translation->translationType == direct)
#endif
		{
			DXDelete(w->translation_owner);
			w->translation = NULL;
			w->translation_owner = NULL;
			w->directMap = directMap;

			w->colormapTag = 0;

			w->translation_owner = (Object)createTranslation(depth, w);
		    
			w->translation = _dxf_GetTranslation(w->translation_owner);
	    
			if (w->winFlag)
				RealizePalette(w->hDC);
	
		}
    }
    else
    {
		char *num;

		if (w)
		{
			/*
			 * then we need to reset the window since we
			 * will be changing visuals.
			*/
			reset_window(w);
		}
		else
		{
			/*
			 * window structure wasn't in cache
			 */
			w = (SWWindow *)DXAllocateZero(sizeof(SWWindow));
			if (! w)
				goto error;

			p = DXNewPrivate((Pointer)w, delete_window);
			if (! p)
				goto error;
	    
			if (! DXSetCacheEntry((Object)p, CACHE_PERMANENT, cachetag, 0, 0))
				goto error;
	    
			DXReference((Object)p);
	
			num = title;

			w->hWnd = 0;

			if (num[0]=='#' && num[1]=='X')
			{
				w->windowMode = WINDOW_EXTERNAL;
				w->handler = (Handler)handler;
				num += 2;
				w->parent = (HWND)atoi(num);
				w->winFlag = 1;
			}
			else if (num[0] == '#' && num[1]=='#')
			{
				w->windowMode = WINDOW_UI;
				w->handler = (Handler) handler;
				num += 2;
				w->parent = (HWND)atoi(num);
				w->winFlag = 1;
			}
			else
			{
				w->windowMode = WINDOW_LOCAL;
				w->handler = (Handler)handler;
				w->parent = 0;
				w->winFlag = 0;
			}

			w->title = (char *)DXAllocate(strlen(title) + 1);
			strcpy(w->title, title);

			w->cacheid = (char *)DXAllocate(strlen(cachetag) + 1);
			strcpy(w->cacheid, cachetag);
		}

		if(w->windowMode == WINDOW_UI) {
			w->hWnd = 0;
		} else {
			if (! _dxfCreateSWWindow(w) || w->hWnd == 0)
				DXErrorReturn(ERROR_INTERNAL, "_dxfCreateSWWindow");
		}

		w->objectTag = 0;
		w->directMap = directMap;

		w->translation_owner = (Object)createTranslation(depth, w);
		if (! w->translation_owner)
			goto error;

		w->translation = _dxf_GetTranslation(w->translation_owner);

		if (!w->winFlag && w->translation->translationType == mapped)
			RealizePalette(w->hDC);
   }

    *window = w;
    *window_object = p;

    DXFree((Pointer)cachetag);
    cachetag = NULL;

    return OK;

error:
    if (p)
		DXDelete((Object)p);
    else {
		DXFree((Pointer)w);
		w = NULL;
	}

    if (! DXSetCacheEntry(NULL, CACHE_PERMANENT, cachetag, 0, 0))
		goto error;
	
    if (cachetag) {
		DXFree((Pointer)cachetag);
		cachetag = NULL;
	}

    return ERROR;
}

static Array
getColormap(Object o)
{
    Array iArray = NULL;
    Array map;

    if (DXGetObjectClass(o) == CLASS_ARRAY)
    {
		map = (Array)o;
    }
    else
    {
		int i;
		float *indices;

		iArray = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, 1);
		if (! iArray)
			goto error;
	
		if (! DXAddArrayData(iArray, 0, 255, NULL))
			goto error;

		indices = (float *)DXGetArrayData(iArray);

		for (i = 0; i < 256; i++)
			indices[i] = i;
		
		map = (Array)DXMap((Object)iArray, o, NULL, NULL);
		if (! map)
			goto error;
		
		DXDelete((Object)iArray);
	}

    return (Array)DXReference((Object)map);

error:
    DXDelete((Object)iArray);
    return NULL;
}

static Error
setColors(SWWindow *w, void *cmap, Array iCmap)
{
	PALETTEENTRY colors[255];
    float *cIn = (float *)DXGetArrayData(iCmap);
    int   i, j, n, grayIn;
    translationT *trans = w->translation;
    int r, g, b;

    if (! trans)
    {
        DXSetError(ERROR_INTERNAL, "cannot setColors without translation");
        return ERROR;
    }

    DXGetArrayInfo(iCmap, &n, NULL, NULL, NULL, &grayIn);
    if (grayIn == 3) 
		grayIn = 0;

    if (grayIn)
    {
        for (i = 0; i < n; i++)
        {
		    j = (int)(*cIn++ * 255);

			colors[i].peRed   = 0x000000ff * trans->gammaTable[j];
            colors[i].peGreen = colors[i].peBlue = colors[i].peRed;
        }
    }
    else
    {
        for (i = 0; i < n; i++)
        {
			r = (int)(*cIn++ * 255);
			g = (int)(*cIn++ * 255);
			b = (int)(*cIn++ * 255);

			colors[i].peRed   = 0x000000ff * trans->gammaTable[r];
			colors[i].peGreen = 0x000000ff * trans->gammaTable[g];
			colors[i].peBlue  = 0x000000ff * trans->gammaTable[b];
        }
    }

	SetPaletteEntries((HPALETTE)cmap, 0, n, (const PALETTEENTRY *)colors);

	UnrealizeObject((HGDIOBJ)cmap);
	RealizePalette((HPALETTE)cmap);

    return OK;
}

/*
 * Dither options: dither in color space or no dither
 */
#define DITHER									\
{												\
    d = m[(x+left) &(DX-1)]; 					\
    ir = DITH(rr,ir,d);							\
    ig = DITH(gg,ig,d);							\
    ib = DITH(bb,ib,d);							\
}

#define NO_DITHER

/*
 * translation: three types: translate RGB separately in three
 * tables prior to creating a combined pixel, translate the combined
 * pixel, and no translation.
 */
#define THREE_XLATE 							\
{												\
    ir = rtable[ir];							\
    ig = gtable[ig];							\
    ib = btable[ib];							\
}

#define ONE_XLATE								\
{												\
    i = table[i];								\
}

#define NO_XLATE

/*
 * color mapping: create color from index
 */
#define COLORMAP								\
{												\
    ir = colormap[i].r;							\
    ig = colormap[i].g;							\
    ib = colormap[i].b;							\
}

#define NO_COLORMAP
	
/*
 * Extract data: float or byte, index or vector, pixel from renderer
 */
#define FLOAT_VECTOR_TO_INDEX_VECTOR 			\
{												\
    float *_ptr = (float *)_src;				\
    ir = (int)(255 * _ptr[0]);					\
    ir = (ir > 255) ? 255 : (ir < 0) ? 0 : ir;	\
    ig = (int)(255 * _ptr[1]);					\
    ig = (ig > 255) ? 255 : (ig < 0) ? 0 : ig;	\
    ib = (int)(255 * _ptr[2]);					\
    ib = (ib > 255) ? 255 : (ib < 0) ? 0 : ib;	\
}
#define BYTE_VECTOR_TO_INDEX_VECTOR	 			\
{												\
    ir = (int)(_src[0]);						\
    ig = (int)(_src[1]);						\
    ib = (int)(_src[2]);						\
}

#define BYTE_SCALAR_TO_INDEX_VECTOR				\
    ir = ig = ib = *(unsigned char *)_src;				

#define BYTE_SCALAR_TO_INDEX     				\
    i = *(unsigned char *)_src;	

#define BIG_PIX_TO_INDEX_VECTOR					\
{												\
    struct big *_ptr = (struct big *)_src;		\
    ir = (int)(255 * _ptr->c.r);				\
    ir = (ir > 255) ? 255 : (ir < 0) ? 0 : ir;	\
    ig = (int)(255 * _ptr->c.g);				\
    ig = (ig > 255) ? 255 : (ig < 0) ? 0 : ig;	\
    ib = (int)(255 * _ptr->c.b);				\
    ib = (ib > 255) ? 255 : (ib < 0) ? 0 : ib;	\
}

#define FAST_PIX_TO_INDEX_VECTOR				\
{												\
    struct fast *_ptr = (struct fast *)_src;	\
    ir = (int)(255 * _ptr->c.r);				\
    ir = (ir > 255) ? 255 : (ir < 0) ? 0 : ir;	\
    ig = (int)(255 * _ptr->c.g);				\
    ig = (ig > 255) ? 255 : (ig < 0) ? 0 : ig;	\
    ib = (int)(255 * _ptr->c.b);				\
    ib = (ib > 255) ? 255 : (ib < 0) ? 0 : ib;	\
}

/*
 * form pixel from rgb: either 5/9/5 for 8-bit shared, 
 * 12/12/12 for 12-bit shared, 256/256/256 for true or
 * direct color, or no pixel formation for 8 or 12 bit
 * private.
 */

#define PIXEL_SUM								\
    i = (ir) + (ig) + (ib)

#define PIXEL_1									\
    i = (((((ir)*gg)+(ig))*bb)+(ib))

#define PIXEL_3									\
    i = ir | ig | ib	

#define SHIFT_PIXEL_3							\
	i = (ib | (ig << 8) | (ir<<16))

#define DIRECT_PIXEL_1

#define SHIFT_PIXEL_1							\
    i = (i << 16) | (i << 8) | i


/* 
 * Gamma: yes or no, applied in color space
 */
#define NO_GAMMA

#define GAMMA									\
{												\
    ir = gamma[ir];								\
    ig = gamma[ig];								\
    ib = gamma[ib];								\
}

#define GAMMA_1									\
{												\
	i = gamma[i];								\
}

/*
 * store the result.  Type dependent on output depth
 */
#define STORE_BYTE								\
    *(unsigned char *)t_dst = i;

#define STORE_SHORT								\
    *(short *)t_dst = i;	

#define STORE_3									\
{												\
	struct rgb {unsigned char r, g, b; };		\
	*(struct rgb *)t_dst = *(struct rgb *)&i;	\
}
				

#define STORE_INT32								\
    *(int32 *)t_dst = i;	
				


/*
 * Options on loop:
 *     extract: 
 *	 Extraction methods produce pixel values in the 0-255 range
 *		BIG_PIX_TO_INDEX_VECTOR
 *		FAST_PIX_TO_INDEX_VECTOR
 *		FLOAT_VECTOR_TO_INDEX_VECTOR
 *		BYTE_VECTOR_TO_INDEX_VECTOR
 *		BYTE_SCALAR_TO_INDEX_VECTOR
 *		BYTE_SCALAR_TO_INDEX
 *     gamma:
 *	 Gamma lookups are in 0-255 range
 *		NO_GAMMA
 * 		GAMMA
 *
 *     dither: 
 *	 Dither methods inputs and outputs are both in the 0-255 range
 *		NO_DITHER
 *		THREE_DITHER
 * 		ONE_DITHER
 *
 *     colormap:
 *	 Colormap method inputs and outputs are both in the 0-255 range
 *		NO_COLORMAP
 *		COLORMAP
 *
 *     translate:
 *       Translation inputs and outputs are in 0-255 range
 *		NO_XLATE
 *		THREE_XLATE	(used before forming pixel)
 *		ONE_XLATE	(used after forming pixel)
 *
 *     pixel:
 *	 pixel inputs are 0-255, outputs are display dependent
 *		PIXEL_1
 *		PIXEL_3
 *		DIRECT_PIXEL_1
 *		SHIFT_PIXEL_1
 *
 *     result:
 *       input is a pixel, output is display dependent
 *              STORE_BYTE
 *              STORE_SHORT
 *              STORE_LONE
 */

#define LOOP(isz, osz, D, ext, cmap, gam, dith, x1, pix, x2, res)		\
{ 																		\
    unsigned char *_dst = dst + (dst_imp_offset + dst_exp_offset)*osz; 	\
    unsigned char *_src = src + (src_imp_offset + src_exp_offset)*isz;	\
    unsigned char *t_dst; 												\
    int _d0 = d0*osz, _d1 = d1*osz;										\
    int ir, ig, ib, i;													\
																		\
    for (y=0;  y<height;  y++)											\
    { 																	\
	if (D)																\
	    m = &(_dxd_dither_matrix[(oy-(p+y))&(DY-1)][0]);				\
																		\
	t_dst = _dst;														\
	for (x = 0; x < width; x++)											\
	{																	\
	    ext;															\
	    cmap;															\
	    gam;															\
	    dith;															\
	    x1;																\
	    pix;															\
	    x2;																\
	    res;															\
																		\
	    t_dst += _d1;													\
	    _src += isz;													\
	}																	\
	_dst += _d0;														\
    }																	\
}		

#define SGN(x) ((x)>0? 1 : (x)<0? -1 : 0)

#define OUT_BYTE	1
#define OUT_SHORT	2
#define OUT_3		3
#define OUT_LONG	4


Error
_dxf_translateWindowsImage(Object image, /* object defining overall pixel grid          */
	int iwidth, int iheight,  /* composite width and height                  */
	int offx, int offy,	  /* origin of composite image in pixel grid     */
	int px, int py,		  /* explicit partition origin                   */
	int cx, int cy,		  /* explicit partition width and height         */
	int slab, int nslabs,	  /* implicit partitioning slab number and count */
	translationP translation, 
	unsigned char *src, int inType, unsigned char *dst)
{
    unsigned long *table;
    int width, height;
    int p, x, y, d, i, r, g, b;
    short *m;
    unsigned char *conv;
    struct { float x, y; } deltas[2];
    int d0, d1, dims[2], ox, oy;
    Array a, cmap;
    unsigned long		*rtable,*gtable,*btable;
    int				rr,gg,bb;
    int				left=0;
    int				outType;
    int				*gamma;
    int				src_imp_offset, src_exp_offset;
    int				dst_imp_offset, dst_exp_offset;
    int				ir, ig, ib;
	int				bytesPerPixel;

    if (!translation)
		DXErrorReturn(ERROR_INTERNAL, "#11610");

    rr = translation->redRange;
    gg = translation->greenRange;
    bb = translation->blueRange;

    gamma = translation->gammaTable;

    /*
     * determine orientation of image
     */
    a = (Array) DXGetComponentValue((Field)image, POSITIONS);
    DXQueryGridPositions(a, NULL, dims, NULL, (float *)deltas);

    deltas[0].x = SGN(deltas[0].x);
    deltas[0].y = SGN(deltas[0].y);
    deltas[1].x = SGN(deltas[1].x);
    deltas[1].y = SGN(deltas[1].y);

    height = cx;
    width  = cy;

    /*
     * The origin of this partition in output is the offset of
     * the partition in the composite image minus the origin of
     * the composite image in the output.
     */
    ox = (px*deltas[0].x + py*deltas[1].x) - offx;
    oy = (px*deltas[0].y + py*deltas[1].y) - offy;

    /*
     * Get the strides in the output.  
     */
    d0 = 1*deltas[0].x + iwidth*deltas[0].y;
    d1 = 1*deltas[1].x + iwidth*deltas[1].y;

    /*
     * if we need to invert the image, we need to reposition the
     * origin in the output image and negate the per-row skip.
     */
    if (translation->invertY)
    {
		oy = (iheight-1) - oy;
		if (d0 == 1 || d0 == -1) d1 = -d1;
		else                     d0 = -d0;
    }

    /*
     * offsets for implicit partitioning
     */
    if (nslabs > 1)
    {
		p = slab*height/nslabs;

		height = (slab+1)*height/nslabs - p;

		src_imp_offset = p*width;
		dst_imp_offset = p*d0;
    }
    else
    {
		src_imp_offset = dst_imp_offset = 0;
		p = 0;
    }

    /*
     * offsets for explicit partitioning
     */
    src_exp_offset = 0;
    dst_exp_offset = oy*iwidth + ox;

    left = ox;

    bytesPerPixel = _dxf_GetBytesPerPixel(translation);

    if (_dxf_GetBytesPerPixel(translation) == 4)
		outType = OUT_LONG;
    else if (_dxf_GetBytesPerPixel(translation) == 3)
		outType = OUT_3;
    else if (_dxf_GetBytesPerPixel(translation) == 2)
		outType = OUT_SHORT;
    else
		outType = OUT_BYTE;

	
    if (inType == IN_BYTE_SCALAR)
    {
        /*
         * if direct, colormap will be installed.  
         */
        if (translation->translationType == direct)
        {
			if (outType == OUT_BYTE)
				LOOP(sizeof(byte), 1, 0, BYTE_SCALAR_TO_INDEX, 
					NO_COLORMAP, NO_GAMMA, NO_DITHER, NO_XLATE,
					DIRECT_PIXEL_1, NO_XLATE, STORE_BYTE)
			else 
			{
				DXSetError(ERROR_INTERNAL, "direct translations must be 8 bits per pixel");
				goto error;
			}
        }
        else
        {
			/*
			 * Otherwise, it must be applied on a pixel by pixel basis
			 */
			int n, gray;
			float *fmap;
			struct { int r, g, b; } colormap[256];
    
			cmap = (Array) DXGetComponentValue((Field)image, "color map");
			if (! cmap)
			{
				DXSetError(ERROR_INTERNAL, "colormap required for byte images");
				goto error;
			}
    
			fmap = (float *)DXGetArrayData(cmap);
			if (!fmap)
				goto error;
    
			DXGetArrayInfo(cmap, &n, NULL, NULL, NULL, &gray);
    
			if (gray == 1)
				for (i=0; i<256; i++)
				{
					colormap[i].r = *fmap++ * 255.0;
					colormap[i].g = colormap[i].b = colormap[i].r;
				}
			else
				for (i=0; i<256; i++)
				{
					colormap[i].r = *fmap++ * 255.0;
					colormap[i].g = *fmap++ * 255.0;
					colormap[i].b = *fmap++ * 255.0;
				}
    
			if (translation->translationType == mapped) 
			{
				table = translation->table;
      
				if (outType == OUT_BYTE)
					LOOP(sizeof(byte), 1, 1, BYTE_SCALAR_TO_INDEX,
					  COLORMAP, NO_GAMMA, DITHER, NO_XLATE, PIXEL_1,
					  ONE_XLATE, STORE_BYTE)
				else 
				{
					DXSetError(ERROR_INTERNAL, "mapped translation must be 8 bits per pixel");
					goto error;
				}
			}	 
			else if (translation->translationType == rgbt)
			{
				if (outType == OUT_BYTE)
					LOOP(sizeof(byte), 1, 0, BYTE_SCALAR_TO_INDEX, 
					  COLORMAP, NO_GAMMA, NO_DITHER, NO_XLATE, PIXEL_3, 
					  NO_XLATE, STORE_BYTE)
				else if (outType == OUT_SHORT)
					LOOP(sizeof(byte), 2, 0, BYTE_SCALAR_TO_INDEX, 
					  NO_COLORMAP, NO_GAMMA, NO_DITHER, NO_XLATE, PIXEL_3, 
					  NO_XLATE, STORE_SHORT)
				else if (outType == OUT_3)
					LOOP(sizeof(byte), 2, 0, BYTE_SCALAR_TO_INDEX, 
					  NO_COLORMAP, NO_GAMMA, NO_DITHER, NO_XLATE, PIXEL_3, 
					  NO_XLATE, STORE_SHORT)
				else
					LOOP(sizeof(byte), 4, 0, BYTE_SCALAR_TO_INDEX, 
					  NO_COLORMAP, NO_GAMMA, NO_DITHER, NO_XLATE, PIXEL_3, 
					  NO_XLATE, STORE_INT32);
			}
			else if (translation->translationType == rgb888)
			{
				if (outType == OUT_3)
					LOOP(sizeof(byte), 3, 0, BYTE_SCALAR_TO_INDEX, 
					  NO_COLORMAP, GAMMA_1, NO_DITHER, NO_XLATE, SHIFT_PIXEL_1, 
					  NO_XLATE, STORE_3)
				else if (outType == OUT_LONG)
					LOOP(sizeof(byte), 4, 0, BYTE_SCALAR_TO_INDEX, 
					  NO_COLORMAP, GAMMA_1, NO_DITHER, NO_XLATE, SHIFT_PIXEL_1, 
					  NO_XLATE, STORE_INT32)
				else
				{
					DXSetError(ERROR_INTERNAL, "rgb888 translations must be 24 or 32 bits per pixel");
					goto error;
				}
			}
        }
    }
    else if (inType == IN_FLOAT_VECTOR)
    {
		if (translation->translationType == mapped)
		{
			table = translation->table;

			if (outType == OUT_BYTE)
				LOOP(3*sizeof(float), 1, 1, FLOAT_VECTOR_TO_INDEX_VECTOR, 
				  NO_COLORMAP, NO_GAMMA, DITHER, NO_XLATE, PIXEL_1, 
				  ONE_XLATE, STORE_BYTE)
			else 
			{
				DXSetError(ERROR_INTERNAL, "mapped translation must be 8 bits per pixel");
				goto error;
			}
		}
		else if (translation->translationType == rgbt)
		{
			rtable = translation->rtable;
			gtable = translation->gtable;
			btable = translation->btable;

			if (outType == OUT_BYTE)
				LOOP(3*sizeof(float), 1, 1, FLOAT_VECTOR_TO_INDEX_VECTOR, 
				  NO_COLORMAP, GAMMA, DITHER, THREE_XLATE, PIXEL_3,
				  NO_XLATE, STORE_BYTE)
			else if (outType == OUT_SHORT)
				LOOP(3*sizeof(float), 2, 1, FLOAT_VECTOR_TO_INDEX_VECTOR, 
				  NO_COLORMAP, GAMMA, DITHER, THREE_XLATE, PIXEL_3, 
				  NO_XLATE, STORE_SHORT)
			else if (outType == OUT_3)
				LOOP(3*sizeof(float), 3, 1, FLOAT_VECTOR_TO_INDEX_VECTOR, 
				  NO_COLORMAP, GAMMA, DITHER, THREE_XLATE, PIXEL_3, 
				  NO_XLATE, STORE_3)
			else
				LOOP(3*sizeof(float), 4, 1, FLOAT_VECTOR_TO_INDEX_VECTOR, 
				  NO_COLORMAP, GAMMA, DITHER, THREE_XLATE, PIXEL_3, 
				  NO_XLATE, STORE_INT32);
		}
		else if (translation->translationType == rgb888)
		{
			if (outType == OUT_3)
				LOOP(3*sizeof(float), 3, 0, FLOAT_VECTOR_TO_INDEX_VECTOR, 
				  NO_COLORMAP, GAMMA, NO_DITHER, NO_XLATE, SHIFT_PIXEL_3, 
				  NO_XLATE, STORE_3)
			else if (outType == OUT_LONG)
				LOOP(3*sizeof(float), 4, 0, FLOAT_VECTOR_TO_INDEX_VECTOR, 
				  NO_COLORMAP, GAMMA, NO_DITHER, NO_XLATE, SHIFT_PIXEL_3, 
				  NO_XLATE, STORE_INT32)
			else
			{
				DXSetError(ERROR_INTERNAL, "rgb888 translations must be 24 or 32 bits per pixel");
				goto error;
			}
		}
		else 
		{
			DXSetError(ERROR_INTERNAL, "inappropriate translation type");
			goto error;
		}
    }
    else if (inType == IN_BIG_PIX)
	{
		if (translation->translationType == mapped)
		{
			table = translation->table;

			if (outType == OUT_BYTE)
				LOOP(sizeof(struct big), 1, 1, BIG_PIX_TO_INDEX_VECTOR, 
				  NO_COLORMAP, NO_GAMMA, DITHER, NO_XLATE, PIXEL_1, 
				  ONE_XLATE, STORE_BYTE)
			else		
			{
				DXSetError(ERROR_INTERNAL, "mapped translation must be 8 bits per pixel");
				goto error;
			}

		}
		else if (translation->translationType == rgbt)
		{
			rtable = translation->rtable;
			gtable = translation->gtable;
			btable = translation->btable;

			if (outType == OUT_BYTE)
				LOOP(sizeof(struct big), 1, 1, BIG_PIX_TO_INDEX_VECTOR, 
				  NO_COLORMAP, GAMMA, DITHER, THREE_XLATE, PIXEL_3,
				  NO_XLATE, STORE_BYTE)
			else if (outType == OUT_SHORT)
				LOOP(sizeof(struct big), 2, 1, BIG_PIX_TO_INDEX_VECTOR, 
				  NO_COLORMAP, GAMMA, DITHER, THREE_XLATE, PIXEL_3, 
				  NO_XLATE, STORE_SHORT)
			else if (outType == OUT_3)
				LOOP(sizeof(struct big), 3, 1, BIG_PIX_TO_INDEX_VECTOR, 
				  NO_COLORMAP, GAMMA, DITHER, THREE_XLATE, PIXEL_3, 
				  NO_XLATE, STORE_3)
			else
				LOOP(sizeof(struct big), 4, 1, BIG_PIX_TO_INDEX_VECTOR, 
				  NO_COLORMAP, GAMMA, DITHER, THREE_XLATE, PIXEL_3, 
				  NO_XLATE, STORE_INT32);
		}
		else if (translation->translationType == rgb888)
		{
			if (outType == OUT_3)
				LOOP(sizeof(struct big), 3, 0, BIG_PIX_TO_INDEX_VECTOR, 
				  NO_COLORMAP, GAMMA, NO_DITHER, NO_XLATE, SHIFT_PIXEL_3, 
				  NO_XLATE, STORE_3)
			else if (outType == OUT_LONG)
				LOOP(sizeof(struct big), 4, 0, BIG_PIX_TO_INDEX_VECTOR, 
				  NO_COLORMAP, GAMMA, NO_DITHER, NO_XLATE, SHIFT_PIXEL_3, 
				  NO_XLATE, STORE_INT32)
			else
			{
				DXSetError(ERROR_INTERNAL, "rgb888 translations must be 24 or 32 bits per pixel");
				goto error;
			}
		}
		else 
		{
			DXSetError(ERROR_INTERNAL, "inappropriate translation type");
			goto error;
		}
    }
    else if (inType == IN_FAST_PIX)	
	{
		if (translation->translationType == mapped)
		{
			table = translation->table;

			if (outType == OUT_BYTE)
				LOOP(sizeof(struct fast), 1, 1, FAST_PIX_TO_INDEX_VECTOR, 
				  NO_COLORMAP, NO_GAMMA, DITHER, NO_XLATE, PIXEL_1, 
				  NO_XLATE, STORE_BYTE)
			else		
			{
				DXSetError(ERROR_INTERNAL, "mapped translation must be 8 bits per pixel");
				goto error;
			}
		}
		else if (translation->translationType == rgbt)
		{
			rtable = translation->rtable;
			gtable = translation->gtable;
			btable = translation->btable;

			if (outType == OUT_BYTE)
				LOOP(sizeof(struct fast), 1, 1, FAST_PIX_TO_INDEX_VECTOR, 
				  NO_COLORMAP, GAMMA, DITHER, THREE_XLATE, PIXEL_3,
				  NO_XLATE, STORE_BYTE)
			else if (outType == OUT_SHORT)
				LOOP(sizeof(struct fast), 2, 1, FAST_PIX_TO_INDEX_VECTOR, 
				  NO_COLORMAP, GAMMA, DITHER, THREE_XLATE, PIXEL_3, 
				  NO_XLATE, STORE_SHORT)
			else if (outType == OUT_3)
				LOOP(sizeof(struct fast), 3, 1, FAST_PIX_TO_INDEX_VECTOR, 
				  NO_COLORMAP, GAMMA, DITHER, THREE_XLATE, PIXEL_3, 
				  NO_XLATE, STORE_3)
			else
				LOOP(sizeof(struct fast), 4, 1, FAST_PIX_TO_INDEX_VECTOR, 
				  NO_COLORMAP, GAMMA, DITHER, THREE_XLATE, PIXEL_3, 
				  NO_XLATE, STORE_INT32);
		}
		else if (translation->translationType == rgb888)
		{
			if (outType == OUT_3)
				LOOP(sizeof(struct fast), 3, 0, FAST_PIX_TO_INDEX_VECTOR, 
				  NO_COLORMAP, GAMMA, NO_DITHER, NO_XLATE, SHIFT_PIXEL_3, 
				  NO_XLATE, STORE_3)
			else if (outType == OUT_LONG)
				LOOP(sizeof(struct fast), 4, 0, FAST_PIX_TO_INDEX_VECTOR, 
				  NO_COLORMAP, GAMMA, NO_DITHER, NO_XLATE, SHIFT_PIXEL_3, 
				  NO_XLATE, STORE_INT32)
			else
			{
				DXSetError(ERROR_INTERNAL, "rgb888 translations must be 24 or 32 bits per pixel");
				goto error;
			}
		}
		else 
		{
			DXSetError(ERROR_INTERNAL, "inappropriate translation type");
			goto error;
		}
    }
    else if (inType == IN_BYTE_VECTOR)
	{
		if (translation->translationType == mapped)
		{
			table = translation->table;

			if (outType == OUT_BYTE)
				LOOP(3*sizeof(ubyte), 1, 1, BYTE_VECTOR_TO_INDEX_VECTOR, 
				  NO_COLORMAP, NO_GAMMA, DITHER, NO_XLATE, PIXEL_1, 
				  ONE_XLATE, STORE_BYTE)
			else	
			{
				DXSetError(ERROR_INTERNAL, "mapped translation must be 8 bits per pixel");
				goto error;
			}

		}
		else if (translation->translationType == rgbt)
		{
			rtable = translation->rtable;
			gtable = translation->gtable;
			btable = translation->btable;

			if (outType == OUT_BYTE)
				LOOP(3*sizeof(ubyte), 1, 1, BYTE_VECTOR_TO_INDEX_VECTOR, 
				  NO_COLORMAP, GAMMA, DITHER, THREE_XLATE, PIXEL_3,
				  NO_XLATE, STORE_BYTE)
			else if (outType == OUT_SHORT)
				LOOP(3*sizeof(ubyte), 2, 1, BYTE_VECTOR_TO_INDEX_VECTOR, 
				  NO_COLORMAP, GAMMA, DITHER, THREE_XLATE, PIXEL_3, 
				  NO_XLATE, STORE_SHORT)
			else if (outType == OUT_3)
				LOOP(3*sizeof(ubyte), 3, 1, BYTE_VECTOR_TO_INDEX_VECTOR, 
				  NO_COLORMAP, GAMMA, DITHER, THREE_XLATE, PIXEL_3, 
				  NO_XLATE, STORE_3)
			else
				LOOP(3*sizeof(ubyte), 4, 1, BYTE_VECTOR_TO_INDEX_VECTOR, 
				  NO_COLORMAP, GAMMA, DITHER, THREE_XLATE, PIXEL_3, 
				  NO_XLATE, STORE_INT32);
		}
		else if (translation->translationType == rgb888)
		{
			if (outType == OUT_3)
				LOOP(3*sizeof(ubyte), 3, 0, BYTE_VECTOR_TO_INDEX_VECTOR, 
				  NO_COLORMAP, GAMMA, NO_DITHER, NO_XLATE, SHIFT_PIXEL_3, 
				  NO_XLATE, STORE_3)
			else if (outType == OUT_LONG)
				LOOP(3*sizeof(ubyte), 4, 0, BYTE_VECTOR_TO_INDEX_VECTOR, 
				  NO_COLORMAP, GAMMA, NO_DITHER, NO_XLATE, SHIFT_PIXEL_3, 
				  NO_XLATE, STORE_INT32)
			else
			{
				DXSetError(ERROR_INTERNAL, "rgb888 translations must be 24 or 32 bits per pixel");
				goto error;
			}
		}
		else 
		{
			DXSetError(ERROR_INTERNAL, "inappropriate translation type");
			goto error;
		}
    }

    return OK;

error:
    return ERROR;
}

static Error
GetBitMap(struct window *w, int width, int height)
{
	if (w->translation->translationType == mapped)
	{	
		BITMAPINFOHEADER *bmih;
		RGBQUAD *map, *c;
		int r, g, b;

		if (w->bmi) {
			DXFree(w->bmi);
			w->bmi = NULL;
		}

		w->bmi = DXAllocate(sizeof(BITMAPINFOHEADER) + 225*sizeof(RGBQUAD));

		if (! w->bmi)
			goto error;

		bmih = (BITMAPINFOHEADER *)w->bmi;
		map = (RGBQUAD *)(((char *)bmih) + sizeof(BITMAPINFOHEADER));
		
		bmih->biSize = sizeof(BITMAPINFOHEADER);
		bmih->biWidth = width;
		bmih->biHeight = height; 
		bmih->biPlanes = 1;
		bmih->biBitCount = 8;
		bmih->biCompression = BI_RGB;
		bmih->biSizeImage = 0;
		bmih->biXPelsPerMeter = 0;
		bmih->biYPelsPerMeter = 0;
		bmih->biClrUsed = 256;
		bmih->biClrImportant = 225;

		for (c = map, r = 0; r < 5; r++)
			for (g = 0; g < 9; g++)
				for (b = 0; b < 5; b++, c++)
		{
			c->rgbRed   = 255 * r/5.0;
			c->rgbGreen = 255 * g/9.0;
			c->rgbBlue  = 255 * b/5.0;
		}

		w->bitmap = CreateDIBSection(w->hDC, (const BITMAPINFO *)w->bmi, (UINT)DIB_RGB_COLORS,
								(void **)&w->pixels, NULL, (DWORD)0);
		w->pwidth = width;
		w->pheight = height;
	}
	else if (w->translation->translationType == direct)
	{	
		BITMAPINFOHEADER *bmih;
		RGBQUAD *map, *c;
		int r, g, b;

		if (w->bmi) {
			DXFree(w->bmi);
			w->bmi = NULL;
		}

		w->bmi = DXAllocate(sizeof(BITMAPINFOHEADER) + 225*sizeof(RGBQUAD));

		if (! w->bmi)
			goto error;

		bmih = (BITMAPINFOHEADER *)w->bmi;
		map = (RGBQUAD *)(((char *)bmih) + sizeof(BITMAPINFOHEADER));
		
		bmih->biSize = sizeof(BITMAPINFOHEADER);
		bmih->biWidth = width;
		bmih->biHeight = height; 
		bmih->biPlanes = 1;
		bmih->biBitCount = 8;
		bmih->biCompression = BI_RGB;
		bmih->biSizeImage = 0;
		bmih->biXPelsPerMeter = 0;
		bmih->biYPelsPerMeter = 0;
		bmih->biClrUsed = 256;
		bmih->biClrImportant = 256;

		w->bitmap = CreateDIBSection(w->hDC, (const BITMAPINFO *)w->bmi, (UINT)DIB_RGB_COLORS,
								(void **)&w->pixels, NULL, (DWORD)0);
		w->pwidth = width;
		w->pheight = height;
	}
	else if (w->translation->translationType == rgbt)
	{
		BITMAPINFOHEADER *bmih;
		DWORD *masks;

		if (w->bmi) {
			DXFree(w->bmi);
			w->bmi = NULL;
		}

		w->bmi = DXAllocate(sizeof(BITMAPINFOHEADER) + 3*sizeof(DWORD));
		if (! w->bmi)
			goto error;

		bmih = (BITMAPINFOHEADER *)w->bmi;
		masks = (DWORD *)(((char *)bmih) + sizeof(BITMAPINFOHEADER));

		
		bmih->biSize = sizeof(BITMAPINFOHEADER);
		bmih->biWidth = width;
		bmih->biHeight = height; 
		bmih->biPlanes = 1;
		bmih->biBitCount = w->translation->depth;
		bmih->biCompression = BI_BITFIELDS;
		bmih->biSizeImage = 0;
		bmih->biXPelsPerMeter = 0;
		bmih->biYPelsPerMeter = 0;
		bmih->biClrUsed = 0;
		bmih->biClrImportant = 0;

		masks[0] = ((1L << w->translation->redBits) - 1) << w->translation->redShift;
		masks[1] = ((1L << w->translation->greenBits) - 1) << w->translation->greenShift;
		masks[2] = ((1L << w->translation->blueBits) - 1) << w->translation->blueShift;

		if (w->bitmap)
			DeleteObject((HGDIOBJ)w->bitmap);


		w->bitmap = CreateDIBSection(w->hDC, (const BITMAPINFO *)w->bmi, (UINT)DIB_RGB_COLORS,
								(void **)&w->pixels, NULL, (DWORD)0);
		w->pwidth = width;
		w->pheight = height;
	}
	else if (w->translation->translationType == rgb888)
	{
		BITMAPINFOHEADER *bmih;
		DWORD *masks;

		if (w->bmi) {
			DXFree(w->bmi);
			w->bmi = NULL;
		}

		w->bmi = DXAllocate(sizeof(BITMAPINFOHEADER) + 3*sizeof(DWORD));
		if (! w->bmi)
			goto error;

		bmih = (BITMAPINFOHEADER *)w->bmi;
		masks = (DWORD *)(((char *)bmih) + sizeof(BITMAPINFOHEADER));

		
		bmih->biSize = sizeof(BITMAPINFOHEADER);
		bmih->biWidth = width;
		bmih->biHeight = height; 
		bmih->biPlanes = 1;
		bmih->biBitCount = 32;
		bmih->biCompression = BI_RGB;
		bmih->biSizeImage = 0;
		bmih->biXPelsPerMeter = 0;
		bmih->biYPelsPerMeter = 0;
		bmih->biClrUsed = 0;
		bmih->biClrImportant = 0;

		if (w->bitmap)
			DeleteObject((HGDIOBJ)w->bitmap);

		w->bitmap = CreateDIBSection(w->hDC, (const BITMAPINFO *)w->bmi, (UINT)DIB_RGB_COLORS,
								(void **)&w->pixels, NULL, (DWORD)0);
		w->pwidth = width;
		w->pheight = height;
	}

	if (! w->bitmap)
	{
		DXSetError(ERROR_INTERNAL, "unable to create bitmap");
		goto error;
	}



	return OK;
error:
	return ERROR;
}

static Error
display(Object image, int width, int height, int ox, int oy, struct window *w)
{
    int 			i, j, x, y;
    Object			o;
	RECT r;

    if (w->active == 0)
    {
		if (! _DXSetSoftwareWindowsWindowActive(w, 1))
			goto error;
	 
		w->active = 1;
    } else
		InvalidateRect(w->hWnd, NULL, TRUE); 

	GetClientRect(w->hWnd, &r);
	
	w->wwidth = width;
	w->wheight = height;
	
	if (r.right != width || r.bottom != height)
		_dxfResizeSWWindow(w);
	
	GdiFlush();

	w->objectTag = DXGetObjectTag(image);

    /* is it a simple translated image? */
     if (DXGetObjectClass(image)==CLASS_FIELD && DXGetComponentValue((Field)image,IMAGE))
    {
		int image_tag;

        if (! DXGetIntegerAttribute(image, "image tag", &image_tag))
			image_tag = (int)DXGetObjectTag(image);

		if ((int)DXGetObjectTag(w->pixels_owner) != image_tag) 
		{
			void *pixels;
			int size;

			if (! GetBitMap(w, width, height))
				goto error;

			DXDelete(w->pixels_owner);
			w->pixels_owner = DXReference(image);
			pixels = _dxf_GetWindowsPixels((Field)w->pixels_owner, &size);
			if (!pixels)
			{
				DXSetError(ERROR_INTERNAL, "unable to get pixels from pixel_owner");
				goto error;
			}

			if (w->translation->translationType == mapped)
			{
				if (size == 1)
				{
					int i, n = width * height;
					ubyte *dst = w->pixels;
					ubyte *src = (ubyte *)pixels;

					for (i = 0; i < n; i++)
						*dst++ = *src++;
				}
				else if (size == 2)
				{
					memcpy((void *)w->pixels, (void *)pixels, width*height*2);
				}
				else
				{
					DXSetError(ERROR_INTERNAL, "invalid pixel size for mapped image: %d bytes", size);
					goto error;
				}
			}
			else
				memcpy((void *)w->pixels, (void *)pixels, width*height*size);
		}
    }

    /*
     * no; (re)-allocate bitmap if necessary
     */
    else 
	{
		if (width != w->pwidth || height != w->pheight)
		{

			DXDelete(w->pixels_owner);
			w->pixels_owner = NULL;

			if (! GetBitMap(w, width, height))
				goto error;

			w->pwidth = width;
			w->pheight = height;
		}

		/*
		 * dither the image
		 */
		if (!w->pixels_owner)
		{
			/*
			 * zero whole thing if composite, to avoid holes
			 */
			if (DXGetObjectClass(image) != CLASS_FIELD) {
				memset((void *)w->pixels, 0, width*height*w->translation->bytesPerPixel);
			}
		}

		if (!_dxf_dither(image, width, height, ox, oy, w->translation, w->pixels))
			goto error;
	}

    w->wwidth = width;
    w->wheight = height;

    /* handle events (i.e. actually draw image) */

	ShowWindow(w->hWnd, SW_SHOWNORMAL);
	UpdateWindow(w->hWnd);
	SetForegroundWindow(w->hWnd);
	SetWindowPos(w->hWnd, HWND_TOP, 0,0,0,0, SWP_DRAWFRAME | SWP_NOMOVE | SWP_NOSIZE);

    return OK;

error:
    return ERROR;
}

Object
DXDisplayWindows(Object image, int depth, char *host, char *window)
{
    SWWindow *w = NULL;
    int directMap = 0;
    Object attr;
    int  doPixels = 1;
    Private w_obj = NULL;;

    attr = DXGetAttribute(image, "direct color map");
    if (attr)
        if (! DXExtractInteger(attr, &directMap))
        {
			DXSetError(ERROR_BAD_PARAMETER, "direct color map attribute must be an integer");
			goto error;
        }

    /*
     * Get the structure that describes the window.  The following
     * routine either gets it from the cache or creates it and
     * places it there.   Note that
     * if we *can't* do direct mapping, then getWindowStructure will
     * reset the directMap flag.
     */
    if (! getWindowStructure(depth, window, directMap, &w, &w_obj, 0))
        return NULL;
    
    /*
     * we can only direct map if the visual was PseudoColor or GrayScale
     */
    if (w->directMap && w->translation->translationType == direct)
		directMap = 0;
  
    /*
     * We don't need to re-translate if its the same image and the
     * translation is either direct or the color map is the 
     * same.
     */
    if (DXGetObjectTag(image) == w->objectTag)
    {
		Object o;

		doPixels = 0;

		if (! directMap)
		{
			o = DXGetAttribute(image, "color map");

			if (!o && DXGetObjectClass(image) == CLASS_FIELD)
				o = DXGetComponentValue((Field)image, "color map");
			
			if (o && DXGetObjectTag(o) != w->colormapTag)
				doPixels = 1;
		}
    }

    if (doPixels)
    {
		int width, height, ox, oy;

		/*
		 * composite image size
		 */
		if (! DXGetImageBounds(image, &ox, &oy, &width, &height))
			return NULL;
  
		if (! w->translation)
			goto error;
  
		if (!display(image, width, height, ox, oy, w))
			goto error;

		w->objectTag = DXGetObjectTag(image);
    }
      
    if (directMap)
    {
		Object iCmap;

		if (w->translation->translationType == direct)
		{
		
			iCmap = DXGetAttribute(image, "color map");

			if (!iCmap && DXGetObjectClass(image) == CLASS_FIELD)
				iCmap = DXGetComponentValue((Field)image, "color map");

			if (iCmap)
			{
				Array colormap = getColormap(iCmap);

				if (! colormap)
					goto error;
				
				if (! CheckColormappedImage(NULL, colormap))
				{
					if ((Object)colormap != iCmap)
						DXDelete((Object)colormap);
					goto error;
				}
      
				if (! setColors(w, (void *)w->translation->cmap, colormap))
				{
					if ((Object)colormap != iCmap)
						DXDelete((Object)colormap);
					goto error;
				}
				
				if ((Object)colormap != iCmap)
					DXDelete((Object)colormap);
			}
		}
    }

    DXDelete((Object)w_obj);

    return image;
  
error:
    DXDelete((Object)w_obj);
    return NULL;
}

Field 
_dxf_CaptureWindowsSoftwareImage(int depth, char *host, char *window)
{
    DXSetError(ERROR_NOT_IMPLEMENTED, "no native windows display yet");
    goto error;

    return NULL;

error:
    return NULL;
}
#define FLOATCONVERT 								\
	t->r = f->c.r, t->g = f->c.g, t->b = f->c.b;

#define UBYTECONVERT 								\
{													\
    t->r = (f->c.r >= 1.0) ? 255 : 					\
	   (f->c.r <= 0.0) ? 0   : 255 * f->c.r;		\
    t->g = (f->c.g >= 1.0) ? 255 : 					\
	   (f->c.g <= 0.0) ? 0   : 255 * f->c.g;		\
    t->b = (f->c.b >= 1.0) ? 255 : 					\
	   (f->c.b <= 0.0) ? 0   : 255 * f->c.b;		\
}

#define COPYCOLORS_TOP(srcType, dstType)			\
{													\
    dstType *pixels = (dstType *)DXGetArrayData(a);	\
    dstType *t;										\
    register dstType *tt;							\
    struct srcType *ff;								\
    register struct srcType *f;						\
													\
    ff = b->u.srcType;								\
    tt = pixels + yoff*iwidth + xoff;				\
    for (y=0; y<bheight; y++) {						\
		for (x=0, f=ff, t=tt; x<bwidth; x++, f++, t++)

#define COPYCOLORS_BOTTOM							\
	ff += bwidth;									\
	tt += iwidth;									\
    }												\
}

#define COPYZ_TOP(srcType)							\
{													\
    float *z_pixels = (float *)DXGetArrayData(a);	\
    float *t;										\
    register float *tt;								\
    struct srcType *ff;								\
    register struct srcType *f;						\
													\
    ff = b->u.srcType;								\
    tt = z_pixels + yoff*iwidth + xoff;				\
    for (y=0; y<bheight; y++) {						\
	for (x=0, f=ff, t=tt; x<bwidth; x++, f++, t++)

#define COPYZ_BOTTOM								\
	ff += bwidth;									\
	tt += iwidth;									\
    }												\
}


Error
_dxf_CopyBufferToWindowsImage(struct buffer *b, Field i, int xoff, int yoff)
{
    int iwidth, iheight, bheight=b->height, y;
    register int x, bwidth=b->width;
    Object o;
    Type type;
    Array a;

    o = DXGetComponentAttribute(i, IMAGE, IMAGE_TYPE);

    if (o==O_X_IMAGE)
    {
		unsigned char *dst, *src;
		translationP  translation;
		int	      inType;
		Object        o;

		o = DXGetComponentAttribute(i, IMAGE, X_SERVER);
		translation = _dxf_GetTranslation(o);

		if (! translation)
		{
			DXSetError(ERROR_INTERNAL,
				"missing translation in _dxf_CopyBufferToWindowsImage");
			return ERROR;
		}
    
		dst = _dxf_GetWindowsPixels(i, NULL);
		if(!dst)
		  return NULL;

		if (b->pix_type == pix_fast)
		{
			src = (unsigned char *)b->u.fast;
			inType = IN_FAST_PIX;
		}
		else if (b->pix_type == pix_big)
		{
			src = (unsigned char *)b->u.big;
			inType = IN_BIG_PIX;
		}
		else
		{
			DXSetError(ERROR_INTERNAL, "unrecognized pix type");
			return ERROR;
		}

		if (!DXGetImageSize(i, &iwidth, &iheight))
			return NULL;
    
		return _dxf_translateWindowsImage((Object)i, iwidth, iheight,
			0, 0, yoff, xoff, b->height, b->width,
			0, 1, translation,
			src, inType, dst);
	}
	else if (o)
		DXErrorReturn(ERROR_BAD_PARAMETER, "#11600");

	if (!DXGetImageSize(i, &iwidth, NULL))
		return NULL;
    
	a = (Array)DXGetComponentValue(i, COLORS);
	if (! a)
		return NULL;
    
	DXGetArrayInfo(a, NULL, &type, NULL, NULL, NULL);

	if (type == TYPE_FLOAT)
	{
		if (b->pix_type == pix_fast)
		{
			COPYCOLORS_TOP(fast, RGBColor)
			FLOATCONVERT
			COPYCOLORS_BOTTOM
		}
		else if (b->pix_type == pix_big)
		{
			COPYCOLORS_TOP(big, RGBColor)
			FLOATCONVERT
			COPYCOLORS_BOTTOM
		}
	}
	else if (type == TYPE_UBYTE)
	{
		if (b->pix_type == pix_fast)
		{
			COPYCOLORS_TOP(fast, RGBByteColor)
			UBYTECONVERT
			COPYCOLORS_BOTTOM
		}
		else if (b->pix_type == pix_big)
		{
			COPYCOLORS_TOP(big, RGBByteColor)
			UBYTECONVERT
			COPYCOLORS_BOTTOM
		}
	}

    a = (Array)DXGetComponentValue(i, "zbuffer");
    if (a) 
    {
		if (b->pix_type == pix_fast)
		{
			COPYZ_TOP(fast)
			*t = f->z;
			COPYZ_BOTTOM
		}
		else if (b->pix_type == pix_big)
		{
		    COPYZ_TOP(big)
		    *t = f->z;
		    COPYZ_BOTTOM
		}
    }

    return OK;

}


Field _dxf_MakeWindowsImage(int width, int height, int depth, char *where)
{
    Field i = NULL;
    Array a = NULL;
    translationP	translation;
    Private w_obj = NULL;
    SWWindow *w;
    char *s, copy[201], *host = NULL, *window = NULL;
    int directMap = 0;

    strcpy(copy, where);
    for (s = copy; *s && !window; s++)
		if (*s == ',')
		{
			if (host == NULL) 
				host = s+1;
			else
				window = s+1;
			
			*s = '\0';
		}

    /* an image is a field */
    i = DXNewField();
    if (!i) goto error;

    /* positions */
    a = DXMakeGridPositions(2, height,width,
			  /* origin: */ 0.0,0.0,
			  /* deltas: */ 0.0,1.0, 1.0,0.0);
    if (!a)
	    goto error;

    DXSetComponentValue(i, POSITIONS, (Object)a);
    a = NULL;

    /* connections */
    a = DXMakeGridConnections(2, height,width);
    if (!a)
		goto error;

    DXSetConnections(i, "quads", a);
    a = NULL;

    if (! getWindowStructure(depth, window, 0, &w, &w_obj, 0))
		goto error;

	if (w->windowMode != WINDOW_UI && w->hWnd == 0)
	{	
		w->wwidth = width;
		w->wheight = height;

		if (! _dxfCreateSWWindow(w) || w->hWnd == 0)
			DXErrorReturn(ERROR_INTERNAL, "_dxfCreateSWWindow");
	
	    w->winFlag = 1;
	}
	else if (w->windowMode != WINDOW_UI && (width != w->wwidth || height != w->wheight))
	{
		w->wwidth = width;
		w->wheight = height;

		_dxfResizeSWWindow(w); 
	}

    translation = w->translation;
    if (!translation)
		goto error;

    if (_dxf_GetBytesPerPixel(translation) == 4)
    {
      a = DXNewArray(TYPE_UINT, CATEGORY_REAL, 0);
    }
    else if (_dxf_GetBytesPerPixel(translation) == 2)
    {
      a = DXNewArray(TYPE_USHORT, CATEGORY_REAL, 0);
    }
    else if (_dxf_GetBytesPerPixel(translation) == 3)
    {
      a = DXNewArray(TYPE_UBYTE, CATEGORY_REAL, 1, 3);
    }
    else
    {
      a = DXNewArray(TYPE_UBYTE, CATEGORY_REAL, 0);
    }
    
    if (!a)
		goto error;

    if (!DXAddArrayData(a, 0, width*height, NULL))
    {
		DXAddMessage("can't make %dx%d image", width, height);
		goto error;
    }

    DXSetAttribute((Object)a, IMAGE_TYPE, O_X_IMAGE);
    DXSetAttribute((Object)a, X_SERVER, w->translation_owner);
    DXSetComponentValue(i, IMAGE, (Object)a);
    a = NULL;

    DXDelete((Object)w_obj);

    /* set default ref and dep */
    return DXEndField(i);

error:
    DXDelete((Object)w_obj);
    DXDelete((Object)a);
    DXDelete((Object)i);
    return NULL;
}

#define ZERO_LOOP(osz, d, dith, x1, pix, x2, res)					\
{ 																	\
    unsigned char *_dst = dst + ((iheight-top)*iwidth + left)*osz; 	\
    unsigned char *t_dst; 											\
    int _ir = ir, _ig = ig, _ib = ib, _y = (iheight-top);			\
																	\
    for (y=0;  y<bheight;  y++, _y++)								\
    { 																\
	if (d)															\
	    m = &(_dxd_dither_matrix[_y&(DY-1)][0]);					\
																	\
	t_dst = _dst;													\
	for (x = 0; x < bwidth; x++)									\
	{																\
	    ir = _ir;													\
	    ig = _ig;													\
	    ib = _ib;													\
																	\
	    dith;														\
	    x1;															\
	    pix;														\
	    x2;															\
	    res;														\
																	\
	    t_dst += osz;												\
	}																\
																	\
	_dst += osz*iwidth;												\
    }																\
}

Field
_dxf_ZeroWindowsPixels(Field image, int left, int right, int top, int bot, RGBColor c)
{
    int                        x, y, d, i;
    Object 			oo;
    int				rr, gg, bb;
    int				outType;
    int				*gamma;
    translationP	translation = NULL;
    int				ir, ig, ib;
    int				bheight, bwidth;
    int				iheight, iwidth;
    unsigned char 	*dst = (unsigned char *)_dxf_GetWindowsPixels(image, NULL);
    short			*m;
	int				bytesPerPixel;

    if (! dst)
    {
		DXSetError(ERROR_INTERNAL, "unable to access pixels in _dxf_ZeroXPixels");
		goto error;
    }

    oo = DXGetComponentAttribute(image, IMAGE, X_SERVER);
    if (oo)
		translation = _dxf_GetTranslation(oo);

    bytesPerPixel = _dxf_GetBytesPerPixel(translation);


    if (!translation)
		DXErrorReturn(ERROR_INTERNAL, "#11610");

    rr = translation->redBits;
    gg = translation->greenBits;
    bb = translation->blueBits;

    gamma = translation->gammaTable;

    bwidth = right - left;
    bheight = top - bot;
    DXGetImageSize(image, &iwidth, &iheight);

    if (_dxf_GetBytesPerPixel(translation) == 4)
		outType = OUT_LONG;
    else if (_dxf_GetBytesPerPixel(translation) == 3)
		outType = OUT_3;
    else if (_dxf_GetBytesPerPixel(translation) == 2)
		outType = OUT_SHORT;
	else
		outType = OUT_BYTE;
	
    ir = (int)(255 * c.r);
    ir = (ir > 255) ? 255 : (ir < 0) ? 0 : ir;
    ig = (int)(255 * c.g);
    ig = (ig > 255) ? 255 : (ig < 0) ? 0 : ig;
    ib = (int)(255 * c.b);
    ib = (ib > 255) ? 255 : (ib < 0) ? 0 : ib;

    if (translation->translationType == mapped)
    {
		unsigned long *table = translation->table;

		if (outType == OUT_BYTE)
			ZERO_LOOP(1, 1, DITHER, NO_XLATE, PIXEL_1, NO_XLATE, STORE_BYTE)
		else if (outType == OUT_SHORT)
			ZERO_LOOP(2, 1, DITHER, NO_XLATE, PIXEL_1, NO_XLATE, STORE_SHORT)
		else if (outType == OUT_3)
			ZERO_LOOP(3, 1, DITHER, NO_XLATE, PIXEL_1, NO_XLATE, STORE_3)
		else 
			ZERO_LOOP(4, 1, DITHER, NO_XLATE, PIXEL_1, NO_XLATE, STORE_INT32);
    }
	else if (translation->translationType == rgbt)
	{
		int i = (((int)(0.5 + ((ir/255.0) * translation->redRange))) << translation->redShift) |
				(((int)(0.5 + ((ig/255.0) * translation->greenRange))) << translation->greenShift) |
				(((int)(0.5 + ((ib/255.0) * translation->blueRange))) << translation->blueShift);

		if (outType == OUT_BYTE)
			ZERO_LOOP(1, 1, NO_DITHER, NO_XLATE, DIRECT_PIXEL_1, NO_XLATE, STORE_BYTE)
		else if (outType == OUT_SHORT)
			ZERO_LOOP(2, 1, NO_DITHER, NO_XLATE, DIRECT_PIXEL_1, NO_XLATE, STORE_SHORT)
		else if (outType == OUT_3)
			ZERO_LOOP(3, 1, NO_DITHER, NO_XLATE, DIRECT_PIXEL_1, NO_XLATE, STORE_3)
		else 
			ZERO_LOOP(4, 1, NO_DITHER, NO_XLATE, DIRECT_PIXEL_1, NO_XLATE, STORE_INT32);

	}
	else if (translation->translationType == rgb888)
	{	
		int i = ir | (ig << 8) | (ib << 16);

		if (outType == OUT_BYTE)
			ZERO_LOOP(1, 1, NO_DITHER, NO_XLATE, DIRECT_PIXEL_1, NO_XLATE, STORE_BYTE)
		else if (outType == OUT_SHORT)
			ZERO_LOOP(2, 1, NO_DITHER, NO_XLATE, DIRECT_PIXEL_1, NO_XLATE, STORE_SHORT)
		else if (outType == OUT_3)
			ZERO_LOOP(3, 1, NO_DITHER, NO_XLATE, DIRECT_PIXEL_1, NO_XLATE, STORE_3)
		else 
			ZERO_LOOP(4, 1, NO_DITHER, NO_XLATE, DIRECT_PIXEL_1, NO_XLATE, STORE_INT32);
	}

    return image;

error:
    return NULL;
}

#endif /* DX_NATIVE_WINDOWS */

