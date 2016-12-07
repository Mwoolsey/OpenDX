/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>

#define tdmWindow_c

#if defined(DX_NATIVE_WINDOWS)

#define FLING_TIMEOUT 3
#define FLING_GNOMON_FEECHURE

#include <windows.h>
#include <sys/types.h>
#include <stdio.h>
#include <math.h>
#include <time.h>

#include "hwDeclarations.h"
#include "hwWindow.h"
#include "hwClientMessage.h"
#include "hwMatrix.h"
#include "hwMemory.h"

#include "hwDebug.h"
#include "../../../VisualDX/dxexec/resource.h"

#include <GL/gl.h>
#include <GL/glu.h>

#include "opengl/hwPortOGL.h"

#if defined(HAVE_CTYPE_H)
#include <ctype.h>
#else
extern int isdigit (int);
#endif

extern int GLTEST();

typedef struct _OGLWindowThreadParams
{
    tdmChildGlobalP	globals;
    HANDLE 			busy;
    HANDLE			windowReady;
    long   			threadID;
    char   			*winName;
}
OGLWindowThreadParams;


/*
 * Internal  Function Declarations 
 */

static Error  _tdmResizeImage(WinP win) ;
Error  _tdmResizeWindow(WinP win, int w, int h, int *change) ;
static void   _tdmClearWindow(WinP win) ;
static void   _tdmCleanupChild(tdmChildGlobalP childGlobals) ;
int _dxfUIType(char *);

static void
StartStroke(OGLWindow *oglw, int x0, int y0, int button)
{
    tdmChildGlobalP globals = (tdmChildGlobalP)oglw->globals;
    tdmStartStroke (globals->CrntInteractor, x0, y0, button, 0) ;
}

static void
EndStroke(OGLWindow *oglw, WPARAM wParam, LPARAM lParam)
{
    tdmChildGlobalP globals = (tdmChildGlobalP)oglw->globals;
    tdmInteractorReturn local;
    int x, y;

    oglw->lastX = x = LOWORD(lParam);
    oglw->lastY = y = HIWORD(lParam);


    tdmEndStroke(globals->CrntInteractor, &local);
}

static void
MousePoint(HWND hWnd, OGLWindow *oglw, WPARAM wParam, LPARAM lParam)
{
    tdmChildGlobalP globals = (tdmChildGlobalP)oglw->globals;
    int l = wParam & MK_LBUTTON;
    int m = wParam & MK_MBUTTON;
    int r = wParam & MK_RBUTTON;
    int c = wParam & MK_CONTROL;
    int s = wParam & MK_SHIFT;
    int x, y;

    oglw->lastX = x = LOWORD(lParam);
    oglw->lastY = y = HIWORD(lParam);

    if (l)
        tdmStrokePoint(globals->CrntInteractor, x, y, INTERACTOR_BUTTON_DOWN, 0);

    return;
}

static void
ButtonUp(HWND hWnd, UINT msg, int which, WPARAM wParam, LPARAM lParam)
{
    OGLWindow *oglw = GetOGLWPtr(hWnd);
    if (oglw) {
        tdmChildGlobalP globals = (tdmChildGlobalP)oglw->globals;
        DEFGLOBALDATA(globals);
        int dxEventMask = EVENT_MASK(globals->CrntInteractor);

        if (dxEventMask & which) {
            if (CAN_RENDER) {
                ReleaseCapture();
                ReleaseMutex(LOCK);
                EndStroke(oglw, wParam, lParam);
            }
        } else
            SendMessage(PARENT_WINDOW, msg, wParam, lParam);

        oglw->buttonState &= ~which;
    }

    return;
}

static void
ButtonDown(HWND hWnd, UINT msg, int which, WPARAM wParam, LPARAM lParam)
{
    OGLWindow *oglw = GetOGLWPtr(hWnd);
    if (oglw) {
        tdmChildGlobalP globals = (tdmChildGlobalP)oglw->globals;
        DEFGLOBALDATA(globals);
        int dxEventMask = EVENT_MASK(globals->CrntInteractor);

        if (dxEventMask & which) {
            if (CAN_RENDER) {
                SetCapture(hWnd);
                if (DXWaitForSignal(1, &LOCK) == WAIT_TIMEOUT)
                    return;

                StartStroke(oglw, LOWORD(lParam), HIWORD(lParam), wParam);
            } else
                which = which;
        } else
            SendMessage(PARENT_WINDOW, msg, wParam, lParam);

        oglw->buttonState |= which;

    }

    return;
}



static LONG WINAPI
ProcessWindowMessages(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static int buttonDown = 0;
    static int pp = 0;

    if (msg == WM_CREATE) {
        return DefWindowProc(hWnd, msg, wParam, lParam);
    } else {
        OGLWindow *oglw = GetOGLWPtr(hWnd);
        if (oglw) {
            tdmChildGlobalP globals = (tdmChildGlobalP)oglw->globals;
            DEFGLOBALDATA(globals);
            int dxEventMask = EVENT_MASK(globals->CrntInteractor);

            switch (msg) {
            case WM_DESTROY:
                lParam = wParam;
                break;

            case WM_USER:
                wParam = lParam;
                break;

            case WM_KEYDOWN:
                if (! (dxEventMask & DXEVENT_KEYPRESS))
                    SendMessage(PARENT_WINDOW, msg, wParam, lParam);
                else if (CAN_RENDER) {
                    char keyState[256];
                    char key[2];
                    int n, i;
                    GetKeyboardState(keyState);
                    n = ToAscii(wParam, MapVirtualKey(wParam, 1),
                                keyState, &key, 0);
                    if (n == 1)
                        for (i = 0; i < (lParam & 0xffff); i++)
                            tdmKeyStruck(globals->CrntInteractor,
                                         oglw->lastX, oglw->lastY, key[0], 0);
                }
                break;


            case WM_MOUSEMOVE: {
                    int l, m, r;

                    l = (oglw->buttonState & DXEVENT_LEFT)   && (wParam & MK_LBUTTON);
                    m = (oglw->buttonState & DXEVENT_MIDDLE) && (wParam & MK_MBUTTON);
                    r = (oglw->buttonState & DXEVENT_RIGHT)  && (wParam & MK_RBUTTON);

                    if ((l && (dxEventMask & DXEVENT_LEFT)) ||
                            (m && (dxEventMask & DXEVENT_MIDDLE)) ||
                            (r && (dxEventMask & DXEVENT_RIGHT))) {
                        if (CAN_RENDER)
                            MousePoint(hWnd, oglw, wParam, lParam);
                    } else
                        SendMessage(PARENT_WINDOW, hWnd, wParam, lParam);
                }
                break;


            case WM_SIZE: {
                    if (oglw && oglw->globals) {
                        DEFGLOBALDATA(oglw->globals);

                        if (MATRIX_STACK && OBJECT && CAMERA) {
                            int flag;
                            _dxfSetInteractorWindowSize(INTERACTOR_DATA, LOWORD(lParam), HIWORD(lParam));
                            _tdmResizeWindow(LWIN, LOWORD(lParam), HIWORD(lParam), &flag);

                            if (flag && !buttonDown) {
                                DEFGLOBALDATA(oglw->globals);
                                _dxfDraw(oglw->globals, OBJECT, CAMERA, 1);
                            }
                        }

                    }
                }
                break;

            case WM_PAINT:
                if (! buttonDown) {
                    DEFGLOBALDATA(oglw->globals);
                    _dxfDraw(oglw->globals, OBJECT, CAMERA, 1);
                }
                break;

            case WM_LBUTTONUP:
                ButtonUp(hWnd, msg, DXEVENT_LEFT, wParam, lParam);
                buttonDown = 0;
                break;

            case WM_MBUTTONUP:
                ButtonUp(hWnd, msg, DXEVENT_MIDDLE, wParam, lParam);
                buttonDown = 0;
                break;

            case WM_RBUTTONUP:
                ButtonUp(hWnd, msg, DXEVENT_RIGHT, wParam, lParam);
                buttonDown = 0;
                break;

            case WM_LBUTTONDOWN:
                ButtonDown(hWnd, msg, DXEVENT_LEFT, wParam, lParam);
                buttonDown = 1;
                break;

            case WM_MBUTTONDOWN:
                ButtonDown(hWnd, msg, DXEVENT_MIDDLE, wParam, lParam);
                buttonDown = 1;
                break;

            case WM_RBUTTONDOWN:
                ButtonDown(hWnd, msg, DXEVENT_RIGHT, wParam, lParam);
                buttonDown = 1;
                break;
            }

        }

        return DefWindowProc(hWnd, msg, wParam, lParam);
    }

}

void
ProcessMessages(HWND hWnd)
{
    MSG msg;
    OGLWindow *oglw;

    while (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)) {
        if (GetMessage(&msg, NULL, 0, 0) != TRUE)
            ExitThread(0);

        if (msg.hwnd == hWnd)
            ProcessWindowMessages(msg.message, hWnd, msg.wParam, msg.lParam);
        else {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    oglw = GetOGLWPtr(hWnd);
    if (! oglw)
        return;

    if (oglw && oglw->globals && oglw->repaint) {
        DEFGLOBALDATA(oglw->globals);
        DEFPORT(PORT_HANDLE);
        if (WaitForSingleObject(LOCK, 0) == WAIT_OBJECT_0) {
            _dxfDraw(oglw->globals, OBJECT, CAMERA, 1);

            ReleaseMutex(LOCK);
            oglw->repaint = 0;
        }
    }
}

static LONG WINAPI
OGLWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    return 	ProcessWindowMessages(hWnd, msg, wParam, lParam);
}

static char *oglWindowClassName;
static char *oglDefaultTitle = "Data Explorer";
static HINSTANCE OGLWindowClassRegistered = (HINSTANCE)0;

static Error
_registerOGLWindowClass()
{
    HINSTANCE hInstance;

    if (! DXGetWindowsInstance(&hInstance))
        return ERROR;

    if (hInstance != OGLWindowClassRegistered) {
        WNDCLASS wc;

        wc.style         = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc   = (WNDPROC)OGLWndProc;
        wc.cbClsExtra    = 0;
        wc.cbWndExtra    = sizeof(OGLWindow *);
        wc.hInstance     = hInstance;
        wc.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_STARTUP));
        wc.hCursor       = NULL;
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wc.lpszMenuName  = NULL;

		if(oglWindowClassName)
			DXFree(oglWindowClassName);
        oglWindowClassName = (char *)DXAllocate(strlen("DXOGLWin_") + 10);
        sprintf(oglWindowClassName, "DXOGLWin_%d", (int)clock());

        wc.lpszClassName = oglWindowClassName;

        if (! RegisterClass(&wc)) {
            LPVOID lpMsgBuf;

            FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                          NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                          (LPTSTR) &lpMsgBuf,0, NULL);

            fprintf(stderr, "RegisterClass failed: %s\n", lpMsgBuf);

            return ERROR;
        }

        OGLWindowClassRegistered = hInstance;
    }

    return OK;
}

static BOOL
ChildSize(HWND kid, LPARAM arg)
{
    RECT rect;
    GetClientRect(kid, &rect);
    fprintf(stderr, "0x%08lx t %d b %d l %d r %d\n",
            (int)kid, rect.top, rect.bottom, rect.left, rect.right);
}

static int
CreateOGLWindow(tdmChildGlobalP globals, char *winName)
{
    DEFGLOBALDATA(globals) ;
    DEFPORT(PORT_HANDLE);
    HINSTANCE hInstance;
    MSG msg;
    int k;

    OGLWindow *oglw = (OGLWindow *)DXAllocateZero(sizeof(OGLWindow));

    static PIXELFORMATDESCRIPTOR pfd =
        {
            sizeof(PIXELFORMATDESCRIPTOR),
            1,
            PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
            PFD_TYPE_RGBA,
            24,
            0, 0, 0,
            0, 0, 0,
            0, 0,
            0, 0, 0, 0, 0,
            32, 0, 0,
            PFD_MAIN_PLANE,
            0, 0, 0, 0
        };
    int nM;

    if (! DXGetWindowsInstance(&hInstance))
        return ERROR;

    ENTRY(("_dxfCreateHWWindow(0x%x, \"%s\")",globals, winName));

    if (PARENT_WINDOW) {
        DWORD dwStyle = WS_CHILD | WS_CLIPCHILDREN;
        XWINID = CreateWindow(oglWindowClassName,
                              winName, dwStyle,
                              CW_USEDEFAULT, 0, PIXW, PIXH,
                              PARENT_WINDOW, NULL, hInstance, NULL);
    } else {
        DWORD dwStyle = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN;
        XWINID = CreateWindow(oglWindowClassName,
                              winName, dwStyle,
                              CW_USEDEFAULT, 0, PIXW, PIXH,
                              NULL, NULL, hInstance, NULL);
    }

    if (! XWINID) {
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
        return ERROR;
    }

    oglw->globals = globals;

    SetOGLWPtr(XWINID, oglw);

    _dxf_CREATE_WINDOW(globals, winName, PIXX, PIXY);

    ShowWindow(XWINID, 1);
    UpdateWindow(XWINID);
	SetForegroundWindow(XWINID);

    return OK;
}


extern tdmParsedFormatT*
    _tdmParseDisplayString (char *displayString,char** cacheIdP);
extern Error
    _dxfDeleteParsedDisplayString(tdmParsedFormatT *pFormat);


void
_dxf_SetHardwareWindowsWindowActive(char *where, int active)
{
    tdmParsedFormatT  *pFormat = NULL;
    Private 	      cacheObject = NULL;
    tdmChildGlobalP   globals = NULL;
    char*		      cacheId = NULL;

    if (!(pFormat = _tdmParseDisplayString (where, &cacheId)))
        goto error;
    if(cacheObject = (Private) DXGetCacheEntry(cacheId, 0, 0)) {
        globals = (tdmChildGlobalP) DXGetPrivateData(cacheObject);
        if (globals) {
            DEFGLOBALDATA(globals);
            if (XWINID > 0)
                ShowWindow(XWINID, active ? SW_SHOW : SW_HIDE);
        }

        DXDelete((dxObject)cacheObject);
    }

error:
    if (cacheId)
        tdmFree(cacheId);
    if (pFormat)
        _dxfDeleteParsedDisplayString(pFormat);
    return;
}


Error
_dxfCreateHWWindow (tdmChildGlobalP globals, char *winName, Camera c)
{
    DEFGLOBALDATA(globals);

    if (!_registerOGLWindowClass())
        goto error;

    PARENT_WINDOW = _dxfConvertWinName(winName) ;
    UI_TYPE = _dxfUIType(winName);
    VISIBILITY = 0;
    MAPPED = 0;

    CAN_RENDER = 0;

    if (c) {
        DXGetCameraResolution(c, &PIXX, &PIXY);
    } else {
        PIXX = 640;
        PIXY = 480;
    }

    PIXW = PIXX;
    PIXH = PIXY;

    MATRIX_STACK = _dxfCreateStack();
    _dxfInitDXInteractors(globals);
    _dxfSetInteractorWindowSize(INTERACTOR_DATA, PIXX, PIXY);

    CreateOGLWindow(globals, winName);


    CAN_RENDER = 1;

    return OK;

error:
    return ERROR;
}

static Error
_tdmResizeImage (WinP win)
{
    DEFWINDATA(win);
    int camw, camh;

    ENTRY(("_tdmResizeImage (0x%x)", win));

    if (CAMERA) {
        if (! DXGetCameraResolution (CAMERA, &camw, &camh))
            DXErrorGoto (ERROR_INVALID_DATA, "#13530") ;
    } else
        if (! DXGetImageSize (OBJECT, &camw, &camh))
            DXErrorGoto (ERROR_INVALID_DATA, "#13540") ;
#if 0

    _dxfSetInteractorWindowSize(INTERACTOR_DATA, camw, camh);
#endif

    return _tdmResizeWindow(win, camw, camh, NULL);

error:
    return ERROR ;
}


Error
_tdmResizeWindow (WinP win, int w, int h, int *winSizeChange)
{
    DEFWINDATA(win) ;
    DEFPORT(PORT_HANDLE) ;
    int camw, camh, changed
    /*
     *  If we're not already at correct size, resize.
     */

    ENTRY(("_tdmResizeWindow (0x%x, %d, %d, 0x%x)",win, w, h, winSizeChange));

    changed = (PIXX != w || PIXY != h);
    if (changed) {
        if (winSizeChange)
            *winSizeChange = 1;

        PIXX = w;
        PIXY = h;
    } else
        if (winSizeChange)
            *winSizeChange = 0;

#if 0

    if (CAMERA) {
        if (! DXGetCameraResolution (CAMERA, &camw, &camh))
            DXErrorGoto (ERROR_INVALID_DATA, "#13530") ;
    } else
        if (! DXGetImageSize (OBJECT, &camw, &camh))
            DXErrorGoto (ERROR_INVALID_DATA, "#13540") ;


    if (changed || VIEWPORT_W != camw || VIEWPORT_H != camh) {

        _dxf_SET_VIEWPORT(PORT_CTX, 0, camw-1, h-camh, h-1);
        _dxfSetViewport(MATRIX_STACK, 0, camw-1, h-camh, h-1);
        _dxfInteractorViewChanged(INTERACTOR_DATA);
        VIEWPORT_W = w;
        VIEWPORT_H = h;
    }
#else
    DXSetResolution(CAMERA, w, ((double)h)/w);
    _dxf_SET_VIEWPORT(PORT_CTX, 0, w-1, 0, h-1);
    _dxfSetViewport(MATRIX_STACK, 0, w-1, 0, h-1);
    _dxfInteractorViewChanged(INTERACTOR_DATA);
    PIXX = VIEWPORT_W = w;
    PIXY = VIEWPORT_H = h;
#endif


    CORRECT_SIZE = TRUE ;

    return OK ;

error:

    return ERROR ;
}


void
_dxfChangeBufferMode(WinP win, int mode)
{
    DEFWINDATA(win);
    DEFPORT(PORT_HANDLE);

    ENTRY(("_dxfChangeBufferMode(0x%x, %d)",win,mode));

    if (mode == DoubleBufferMode) {
        if (BUFFER_MODE == SingleBufferMode) /* needs changing? */
        {
            _dxf_DOUBLE_BUFFER_MODE(PORT_CTX) ;
            BUFFER_MODE = DoubleBufferMode ;
        }
    } else /* mode == SingleBufferMode */
    {
        if (BUFFER_MODE == DoubleBufferMode) /* needs changing? */
        {
            _tdmClearWindow(win) ;
            _dxf_SINGLE_BUFFER_MODE(PORT_CTX) ;
            BUFFER_MODE = SingleBufferMode ;
        }
    }

    EXIT((""));
}


static void
_tdmClearWindow (WinP win)
{
    DEFWINDATA(win) ;
    DEFPORT(PORT_HANDLE) ;

    ENTRY(("_tdmClearWindow (0x%x)",win));

    _dxf_CLEAR_AREA (PORT_CTX, 0, PIXW - 1 , 0, PIXH - 1) ;

    EXIT((""));
}

int
_dxfTrySaveBuffer (WinP win)
{
    return 0;
}

int
_dxfTryRestoreBuffer (WinP win)
{
    return 0;
}

static void
_tdmCleanupChild (tdmChildGlobalP globals)
{
    DEFGLOBALDATA(globals) ;
    HWND tmpParentWindow = PARENT_WINDOW;

    ENTRY(("_tdmCleanupChild (0x%x)", globals));

    if (globals->cacheId) {
        /*
         *  DXRemove cache entry for this instance.  This will cause
         *  _dxfEndRenderModule() to be invoked as a side effect, since it is
         *  registered as the callback for the cache deletion.
         */
        PRINT(("cacheId exists, deleting cache entry")) ;
        DXSetCacheEntry (0, CACHE_PERMANENT, globals->cacheId, 0, 0) ;

    }

    EXIT((""));
}


int
_dxfConvertWinName(char *winName)
{
    int i ;

    /* ENTRY(("_dxfConvertWinName(\"%s\")",winName)); */

    for (i = strlen(winName)-1 ; i >= 0 ; i--)
        if (!isdigit(winName[i]))
            break;

    if (i == -1) {
        /* no pound signs, all digits */
        /* EXIT(("0")); */
        return 0 ;
        /* #X was
         * } else if (winName[i] == '#' && (i !=0 && winName[i-1] == '#')) {
         */
    } else if ((winName[i] == '#' || winName[i] == 'X') && (i !=0 && winName[i-1] == '#')) {
        /* 2 pound signs */
        /* EXIT(("fill in this value")); */
        return atoi(&winName[i+1]) ;
    } else {
        /* EXIT(("0")); */
        return 0 ;
    }
}

int
_dxfUIType(char *winName)
{

    int i ;

    for (i = strlen(winName)-1 ; i >= 0 ; i--)
        if (!isdigit(winName[i]))
            break;

    if (i > 0 && winName[i] == '#' && i !=0 && winName[i-1] == '#')
        return DXD_DXUI;
    else if (i > 0 && winName[i] == 'X' && i !=0 && winName[i-1] == '#')
        return DXD_EXTERNALUI;
    else
        return DXD_NOUI;
}
/* #X end */


extern Field DXMakeImageFormat(int w, int h, char *format);

Field
_dxfCaptureHardwareImage(tdmChildGlobalP globals)
{
    DEFGLOBALDATA(globals) ;
    DEFPORT(PORT_HANDLE) ;
    Field image = NULL;
    Array colors;

    image = DXMakeImageFormat(PIXW, PIXH, "BYTE");
    if (! image)
        goto error;

    colors = (Array)DXGetComponentValue(image, "colors");
    if (! colors)
        goto error;

    if (! _dxf_READ_IMAGE(LWIN, DXGetArrayData(colors)))
        goto error;

    return image;

error:
    DXDelete((dxObject)image);
    return NULL;
}

/****************************** END WINDOWS *********************************/
#else /* DX_NATIVE_WINDOWS */

#define FLING_TIMEOUT 3
#define FLING_GNOMON_FEECHURE

/*
* Environment:		Risc System 6000
* Current Source:	$Source: /src/master/dx/src/exec/hwrender/hwWindow.c,v $
* Author:		Tim Murphy
*
*/

#include <sys/types.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>

#if defined(HAVE_STRING_H)
#include <string.h>
#endif

#if defined(HAVE_CTYPE_H)
#include <ctype.h>
#else
extern int isdigit (int);
#endif

#include "hwDeclarations.h"
#include "hwWindow.h"
#include "hwClientMessage.h"
#include "hwMatrix.h"
#include "hwMemory.h"

#include "hwDebug.h"

/*
*  X Function Declarations.
*/

extern Bool
    XCheckTypedWindowEvent(Display *, Window, int, XEvent *) ;

#if 0
#ifdef DXD_WIN
extern void XPutBackEvent(Display *, XEvent *) ;
#else
extern XPutBackEvent(Display *, XEvent *) ;
#endif
#endif

/*
* Internal  Function Declarations
*/

static Error
_tdmResizeImage(WinP win) ;
static Error
_tdmResizeWindow(WinP win, int w, int h, int *change) ;
static void
_tdmClearWindow(WinP win) ;
static void
_tdmCleanupChild(tdmChildGlobalP childGlobals) ;
static Window
_getTopLevelShell (Display *dpy, Window w) ;

int _dxfUIType(char *);
extern Error _dxfDraw (void*, dxObject, Camera, int);


/*
*  These atoms are used by the window manager to notify us if we're about
*  to get killed (eg, evil user hits "close" item on Motif window menu).
*/

static Atom XA_WM_PROTOCOLS ;
static Atom XA_WM_DELETE_WINDOW ;
static Atom XA_GL_WINDOW_ID ;

/* Other commuication Atoms */
extern struct atomRep _dxdtdmAtoms[];

/*
*  X error handling
*/

static int _tdmXerror = 0 ;

void _dxfSetXError (Display *dpy, XErrorEvent *event)
{
    char buffer[128] ;
    ENTRY(("_dxfSetXError(0x%x, 0x%x)",dpy, event));

    XGetErrorText (dpy, event->error_code, buffer, sizeof(buffer)) ;
    /*
    DXSetError (ERROR_INTERNAL, "#13490", buffer) ;
    */

    EXIT(("X Error: %s", buffer));
}


int _dxfXErrorHandler (Display *dpy, XErrorEvent *error)
{
    ENTRY(("_dxfXErrorHandler (0x%x, 0x%x)",dpy, error));

    _dxfSetXError (dpy, error) ;
    _tdmXerror = 1 ;

    EXIT(("1"));
    return 1 ;
}

int _dxfCatchWinopenErrors (Display *dpy, XErrorEvent *rep)
{
    /*
    * We get this during winopen on 3.1.5.
    */

    ENTRY(("_dxfCatchWinopenErrors (0x%x, 0x%x)",dpy, rep));

    if (rep->error_code == BadAccess) {
        EXIT(("BadAccess error ignored"));
        return 0 ;
    }

    _dxfSetXError (dpy, rep) ;

    EXIT(("1"));
    return 1 ;
}

/*
*  This function creates a graphics window (if possible on this machine), and
*  attaches the resulting window id to the window that the UI created.
*
*  This function should only be called ONCE per tdmRender module instance.
*  It will create a graphics window each time.
*/

Error _dxfCreateWindow (tdmChildGlobalP globals, char *winName)
{
    XWindowAttributes attr ;
    unsigned long eventMask ;
    DEFGLOBALDATA(globals) ;
    DEFPORT(PORT_HANDLE) ;
    XWindowChanges xwc;

    ENTRY(("_dxfCreateWindow(0x%x, \"%s\")",globals, winName));

    /* ignore spurious errors generated by some environments */
    XSetErrorHandler (_dxfCatchWinopenErrors) ;

    /* convert text representation of window id */
    PARENT_WINDOW = _dxfConvertWinName(winName) ;
    UI_TYPE = _dxfUIType(winName);

    /* Assume the window is obsured, we'll change this when we get a
    * visibilityNotify event.
    */
    VISIBILITY = VisibilityFullyObscured ;

    /* set window size as uninitialized */
    PIXW = -1 ;
    PIXH = -1 ;

    /* Mark window as unmapped */
    MAPPED = 0;

    /* create the graphics window */
    if (PARENT_WINDOW) {
        /* if there is a UI ... */
        XGetWindowAttributes (DPY, PARENT_WINDOW, &attr) ;
        PRINT(("Before _dxf_CREATE_WINDOW(UI)"));
        if(!( XWINID =
                    (Window)_dxf_CREATE_WINDOW (globals, winName,
                                                attr.width, attr.height)))
            goto error;
        PRINT(("After _dxf_CREATE_WINDOW"));
    } else {
        PRINT(("Before _dxf_CREATE_WINDOW (script)"));
        if(!( XWINID =
                    (Window)_dxf_CREATE_WINDOW (globals, winName, 640, 480)))
            goto error;
        PRINT(("\nAfter _dxf_CREATE_WINDOW"));
    }

    /* #X
    * if (PARENT_WINDOW)
    */
    if (DXUI) {
        /*
        *  UI is present.
        *
        *  Find the top level shell of the UI image window in order to
        *  select for ConfigureNotify; otherwise, we don't get a
        *  ConfigureNotify on window moves since our graphics window never
        *  moves relative to its immediate parent.
        *
        *  We need to know when the window moves since some (broken?) X
        *  servers neither copy the graphics window's pixels along with the
        *  window nor send exposure events.
        *
        *  Also select for ConfigureNotify in the immediate parent
        *  (wigWindow) of the graphics window in order to be informed when
        *  the UI resizes the immediate parent in response to new camera
        *  sizes from us.  We don't seem to receive this ConfigureNotify on
        *  the top level shell.
        */

        TOPLEVEL =
            _getTopLevelShell (DPY, PARENT_WINDOW) ;

        if (TOPLEVEL)
            XSelectInput (DPY,
                          TOPLEVEL, StructureNotifyMask) ;
        else
            /* Can't find top level UI shell */
            DXErrorGoto (ERROR_UNEXPECTED, "#13570") ;

        /* record initial placment of top level shell */
        XGetWindowAttributes (DPY, TOPLEVEL, &attr) ;
        PIXX = attr.x ;
        PIXY = attr.y ;

        /* reparent the graphics window */
        XReparentWindow (DPY,
                         XWINID,
                         PARENT_WINDOW,
                         DXD_HW_REPARENT_OFFSET_X,
                         DXD_HW_REPARENT_OFFSET_Y) ;

        /* select for ConfigureNotify on our new parent */
        XSelectInput (DPY,
                      PARENT_WINDOW, StructureNotifyMask | PropertyChangeMask) ;

        /* setup UI client message protocol */
        if (!_dxfInitCMProtocol(LWIN)) {
            PRINT(("UI client message protocol init failed")) ;
            DXErrorGoto (ERROR_INTERNAL, "#13580") ;
        }

        /* set mask for appropriate events on the graphics window */
        if (PIXDEPTH == 8)
            /* 8 bit display requires enter/leave window events for colormap */
            eventMask = VisibilityChangeMask | ExposureMask |
                        ButtonPressMask | ButtonReleaseMask |
                        EnterWindowMask | LeaveWindowMask | KeyPressMask;
        else
            eventMask = VisibilityChangeMask | ExposureMask |
                        ButtonPressMask | ButtonReleaseMask | KeyPressMask;
    }
    /* #X follows */
    else if (EXTERNALUI) {
        /*
        *  No UI.  Record initial placment of graphics window.  Set mask for
        *  ConfigureNotify on the graphics window along with the other events
        *  we need.
        */

        /* reparent the graphics window */
        XReparentWindow (DPY,
                         XWINID,
                         PARENT_WINDOW,
                         DXD_HW_REPARENT_OFFSET_X,
                         DXD_HW_REPARENT_OFFSET_Y) ;

        XGetWindowAttributes (DPY, XWINID, &attr) ;
        PIXX = attr.x ;
        PIXY = attr.y ;

        if (PIXDEPTH == 8)
            /* 8 bit display requires enter/leave window events for colormap */
            eventMask = VisibilityChangeMask | ExposureMask |
                        ButtonPressMask | ButtonReleaseMask |
                        EnterWindowMask | LeaveWindowMask |
                        StructureNotifyMask | KeyPressMask;
        else
            eventMask = VisibilityChangeMask | ExposureMask |
                        ButtonPressMask | ButtonReleaseMask |
                        StructureNotifyMask | KeyPressMask;
    }
    /* #X end */
    else {
        /*
        *  No UI.  Record initial placment of graphics window.  Set mask for
        *  ConfigureNotify on the graphics window along with the other events
        *  we need.
        */

        XGetWindowAttributes (DPY, XWINID, &attr) ;
        PIXX = attr.x ;
        PIXY = attr.y ;

        if (PIXDEPTH == 8)
            /* 8 bit display requires enter/leave window events for colormap */
            eventMask = VisibilityChangeMask | ExposureMask |
                        ButtonPressMask | ButtonReleaseMask |
                        EnterWindowMask | LeaveWindowMask |
                        StructureNotifyMask | KeyPressMask;
        else
            eventMask = VisibilityChangeMask | ExposureMask |
                        ButtonPressMask | ButtonReleaseMask |
                        StructureNotifyMask | KeyPressMask;
    }

    xwc.stack_mode = Below;
    XConfigureWindow(DPY, XWINID, CWStackMode, &xwc);


    /* this handler should handle errors from now on */
    XSetErrorHandler(_dxfXErrorHandler) ;

    /* get the window manager to inform us if it intends to kill our window */
    XA_WM_PROTOCOLS = XInternAtom (DPY, "WM_PROTOCOLS", 0) ;
    XA_WM_DELETE_WINDOW = XInternAtom (DPY, "WM_DELETE_WINDOW", 0) ;
    XA_GL_WINDOW_ID = XInternAtom (DPY, "GL_WINDOW_ID", 0) ;
    XChangeProperty (DPY,XWINID,
                     XA_WM_PROTOCOLS, XA_ATOM, 32, PropModeReplace,
                     (unsigned char *)&XA_WM_DELETE_WINDOW, 1) ;

    /* select the events we need on the graphics window */
    XSelectInput (DPY, XWINID, eventMask) ;

    /* create a software matrix stack to shadow hardware matrix stack */
    MATRIX_STACK = _dxfCreateStack() ;


    /* initialize hardware direct interactors */
    _dxfInitDXInteractors(globals) ;

    if (_tdmXerror) {
        PRINT(("X error occured")) ;
    }

    _tdmXerror = 0 ;

    EXIT(("OK"));
    return OK ;

error:

    EXIT(("ERROR"));
    return ERROR ;
}

/*
*  This is the currently defined resize behavior:
*
*  With the UI:
*
*  When the user changes the camera values the window will size to those
*  dimensions and redraw.
*
*  When the user grabs the window corner the UI will update the camera and
*  the window will resize to those dimensions and redraw, isotropically
*  scaling for an appropriate fit.
*
*  Without the UI:
*
*  When the user changes the camera values the window will size to those
*  dimensions and redraw, scaling the image isotropically.
*
*  When the user grabs the window corner the window will resize, and as
*  much of the image as will fit will be moved to the upper left corner
*  of the new window.  The Camera will remain unchanged.  The window will
*  automatically resize to the camera dimensions if Display is called
*  again.
*/

static Error
_tdmResizeImage (WinP win)
{
    DEFWINDATA(win);
    Window RootReturn ;
    int XReturn, YReturn, camw, camh, dummy ;
    unsigned int BorderWidthReturn, DepthReturn ;
    unsigned int wigW, wigH ;

    ENTRY(("_tdmResizeImage (0x%x)", win));

    if (CAMERA) {
        if (! DXGetCameraResolution (CAMERA, &camw, &camh))
            /* unable to get camera resolution */
            DXErrorGoto (ERROR_DATA_INVALID, "#13530") ;
    } else
        /* assume we have been given an image */
        if (! DXGetImageSize (OBJECT, &camw, &camh))
            /* unable to get image resolution */
            DXErrorGoto (ERROR_DATA_INVALID, "#13540") ;

    PRINT (("DX object resolution: %dx%d", camw, camh));

    /* #X was
    * if (PARENT_WINDOW)
    */
    if (DXUI)	/* if there is a UI ... */
    {
        XGetGeometry(DPY, PARENT_WINDOW, &RootReturn, &XReturn, &YReturn,
                     &wigW, &wigH, &BorderWidthReturn, &DepthReturn) ;

        PRINT(("parent window size: %dx%d", wigW, wigH));

        /*
        *  If the image resolution and the widget window size are the same,
        *  just get resizeWindow to set the graphics window to that size .
        */

        if (camw == wigW && camh == wigH)
            _tdmResizeWindow (win, wigW, wigH, &dummy) ;
        else
        {
            /*
            *  Else get the UI to make the widget size and resolution agree.
            *  We'll get the graphics window to agree when we get the configure
            *  notify on the widget window.
            */

            tdmMessageData data ;

            /*
            CORRECT_SIZE = FALSE ;
            */

            data.l[0] = camw ;
            data.l[1] = camh ;

            PRINT(("notifying UI of current resolution")) ;
            _dxfSendClientMessage (DPY, PARENT_WINDOW, XA_Integer, &data) ;
        }
    }
    else /* No UI ... */
    {
        /*
        *  Make sure the saved window size is correct for the GLwindow
        *  then resize.
        */

        XGetGeometry (DPY, XWINID, &RootReturn, &XReturn, &YReturn,
                      (unsigned int *)&PIXW, (unsigned int *)&PIXH,
                      &BorderWidthReturn, &DepthReturn) ;

        PRINT(("graphics window size: %dx%d", PIXW, PIXH));

        _dxfSetInteractorWindowSize (INTERACTOR_DATA, camw, camh) ;
        if (camw != PIXW || camh != PIXH) {
            DEFWINDATA(win) ;
            DEFPORT(PORT_HANDLE) ;
            _dxf_SET_WINDOW_SIZE (win, camw, camh) ;
        }

        _tdmResizeWindow (win, camw, camh, &dummy) ;
    }

    if (_tdmXerror)
        goto error ;

    EXIT(("OK"));
    return OK ;

error:
    _tdmXerror = 0 ;

    EXIT(("ERROR"));
    return ERROR ;
}


static Error
_tdmResizeWindow (WinP win, int w, int h, int *winSizeChange)
{
    int camw, camh ;
    DEFWINDATA(win) ;
    DEFPORT(PORT_HANDLE) ;

    /*
    *  If we're not already at correct size, resize.
    */

    ENTRY(("_tdmResizeWindow (0x%x, %d, %d, 0x%x)",win, w, h, winSizeChange));

    if (w != PIXW || h != PIXH) {
        /*
        * If we have a UI set the GL window to the new size.
        * For script we only do this when we resizeImage.
        */

        /* #X was
        * if (PARENT_WINDOW)
        */
        if (DXUI)	/* if there is a UI ... */
        {
            _dxf_SET_WINDOW_SIZE (win, w, h) ;
            _dxfSetInteractorWindowSize (INTERACTOR_DATA, w, h) ;
        }

        PIXW = w ;
        PIXH = h ;

        _tdmClearWindow(win) ;
        *winSizeChange = 1 ;
    } else
        *winSizeChange = 0 ;

    if (CAMERA) {
        if (! DXGetCameraResolution (CAMERA, &camw, &camh))
            /* unable to get camera resolution */
            DXErrorGoto (ERROR_DATA_INVALID, "#13530") ;
    } else
        if (! DXGetImageSize (OBJECT, &camw, &camh))
            /* unable to get image resolution */
            DXErrorGoto (ERROR_DATA_INVALID, "#13540") ;

    PRINT(("current DX resolution: %dx%d", camw, camh));
    PRINT(("current viewport: %dx%d", VIEWPORT_W, VIEWPORT_H));

    if (*winSizeChange || VIEWPORT_W != camw || VIEWPORT_H != camh) {
        _dxf_SET_VIEWPORT (PORT_CTX, 0, camw-1, h-camh, h-1) ;
        _dxfSetViewport (MATRIX_STACK, 0, camw-1, h-camh, h-1) ;
        /* !!!! check for increment view_state twice !!!! */
        _dxfInteractorViewChanged(INTERACTOR_DATA) ;
        VIEWPORT_W = camw ;
        VIEWPORT_H = camh ;
    }

    if (_tdmXerror)
        goto error ;
    CORRECT_SIZE = TRUE ;

    EXIT(("OK"));
    return OK ;

error:
    _tdmXerror = 0 ;

    EXIT(("ERROR"));
    return ERROR ;
}

/*
*  _dxfProcessEvents() is invoked in two ways: 1) from tdmRender() in
*  response to a new DX object, camera, or render option; and 2) as a
*  callback in response to input on the file associated with the X
*  display connection.
*/

extern int _dxf_ExIsExecuting();

static Bool checkDestroy(Display *d, XEvent *ev, char *arg)
{
    return (ev->type == DestroyNotify);
}

Error
_dxfProcessEvents (int fd, tdmChildGlobalP globals, int flags)
{
    Display *dpy ;
    XEvent ev ;
    unsigned int mask;
    int doRedraw, winSizeChange ;
    tdmInteractorReturn R ;
    XWindowAttributes attr;
    static Time lasttime = 0 ;
    DEFGLOBALDATA(globals) ;
    DEFPORT(PORT_HANDLE) ;
    int dxEventMask = EVENT_MASK(globals->CrntInteractor);
    Window eventWindow;

    if (fd != -1)
        DEBUG_MARKER("_dxfProcessEvents ENTRY");
    ENTRY(("_dxfProcessEvents (%d, 0x%x)",fd, globals));

    /*
    * Look ahead for destroy event
    */
    if (XCheckIfEvent(DPY, &ev, checkDestroy, NULL)) {
        DXMessage("Found destroy notify... cleaning up");
        GL_WINDOW = FALSE;
        _tdmCleanupChild(globals) ;
        goto done;
    }

    dpy = DPY ;

    /* assume we don't need to refresh image */
    doRedraw = FALSE ;

    if (fd == -1) {

        /* remap window if unmapped */
        if (! MAPPED) {
            XEvent event;

            PRINT(("mapping window %d", XWINID));
            if(DXUI) {
                XChangeProperty(DPY,PARENT_WINDOW,
                                XA_GL_WINDOW_ID,
                                XA_GL_WINDOW_ID,
                                32,
                                PropModeReplace,
                                (unsigned char*)&XWINID,
                                1);
                for((*(XPropertyEvent*)&event).state = !PropertyDelete;
                        (*(XPropertyEvent*)&event).state != PropertyDelete;) {
                    /* wait for UI to respond before starting to draw */
                    XWindowEvent (DPY, PARENT_WINDOW,
                                  PropertyChangeMask, &event) ;
                }
            }
            /* #X follows */
            else if (EXTERNALUI) {
                /* map the graphics window */
                XMapWindow (DPY, PARENT_WINDOW) ;
                XMapWindow (DPY, XWINID) ;
                if (STEREOSYSTEMMODE >= 0) {
                    if (XWINID != LEFTWINDOW)
                        XMapWindow (DPY, LEFTWINDOW) ;
                    if (XWINID != RIGHTWINDOW)
                        XMapWindow (DPY, RIGHTWINDOW) ;
                }
            }
            /* #X end */
            else {
                /* map the graphics window */
                XMapWindow (DPY, XWINID) ;
                if (STEREOSYSTEMMODE >= 0) {
                    if (XWINID != LEFTWINDOW)
                        XMapWindow (DPY, LEFTWINDOW) ;
                    if (XWINID != RIGHTWINDOW)
                        XMapWindow (DPY, RIGHTWINDOW) ;
                } else
                    XWindowEvent (DPY, XWINID, ExposureMask, &event) ;

                /* wait for pending requests to be processed by X
                * before graphics output
                */
            }
            XSync (DPY, False) ;

            MAPPED = 1 ;
        }

        doRedraw = 1;
        /* adjust graphics window image size to camera image size if necessary */
        _tdmResizeImage(LWIN) ;
    }

    /*
    *  Loop while events pending or until UI responds to new camera size with
    *  a ConfigureNotify.
    */

    while (XPending(dpy) || !CORRECT_SIZE) {
#if defined(DXD_HW_WINDOW_DESTRUCTION_CHECK)
        /*
        *  Check to ensure graphics window still exists.  On some
        *  architectures (SGI/GL) the graphics window dies without
        *  notification and breaks the connection to the DX server on the
        *  next graphics API call.
        */
        XSync(dpy, False);
        GL_WINDOW = XGetWindowAttributes(dpy,XWINID,&attr);
#else
/*  set the global variable to TRUE on all other arch. */
GL_WINDOW = TRUE;
#endif

        XNextEvent(dpy, &ev) ;
        switch (ev.type) {
        case KeyPress:
            if (flags || !_dxf_ExIsExecuting()) {
                PRINT (("KeyPress")) ;
                if (!CORRECT_SIZE)
                    continue ;

                if (! (dxEventMask & DXEVENT_KEYPRESS)) {
                    Window tmp = ev.xany.window;
                    ev.xany.window = PARENT_WINDOW;
                    XSendEvent(dpy, PARENT_WINDOW, True, 0, &ev);
                    ev.xany.window = tmp;
                } else {
                    char buf[32];
                    XLookupString(&(ev.xkey), buf, 32, NULL, NULL);
                    if (buf[0])
                        tdmKeyStruck(globals->CrntInteractor, ev.xkey.x, ev.xkey.y, buf[0], ev.xkey.state);
                }
            }
            break;

        case ConfigureNotify:
            PRINT (("ConfigureNotify"));
            if (DXUI) {
                /*
                *  UI case: ConfigureNotify selected on top level shell and
                *  the immediate parent.
                */
                winSizeChange = 0 ;
                if (ev.xconfigure.window == PARENT_WINDOW) {
                    /* check for size changes on the immediate parent only */
                    XWindowAttributes attr ;

                    XGetWindowAttributes (dpy, PARENT_WINDOW, &attr) ;
                    _tdmResizeWindow (LWIN, attr.width,
                                      attr.height, &winSizeChange) ;
                }
            } else
                /*  #X comment changed
                *  Script or external UI case: ConfigureNotify selected
                * on graphics window.
                */
                _tdmResizeWindow (LWIN, ev.xconfigure.width,
                                  ev.xconfigure.height, &winSizeChange) ;

            /*
            *  If no window size change, check for movement.  We have to do
            *  this since some (broken) X servers neither copy the
            *  graphics window's pixels nor generate an exposure event on
            *  movement.
            *
            *  HP and IBM handle this correctly.  Need to check on Sun and SGI
            */
#if !(DXD_HW_XSERVER_MOVE_OK)

            if (!winSizeChange &&
                    (ev.xconfigure.x != PIXX ||
                     ev.xconfigure.y != PIXY)) {
                doRedraw = TRUE ;
                PIXX = ev.xconfigure.x ;
                PIXY = ev.xconfigure.y ;
            }
#endif
            break ;

        case ClientMessage:
            PRINT (("ClientMessage")) ;
            /* check for window manager kill using ICCC protocol */
            if (ev.xclient.message_type == XA_WM_PROTOCOLS) {
                PRINT (("window manager kill")) ;
                GL_WINDOW = FALSE;
                _tdmCleanupChild(globals) ;
                goto done ;
            }
#if 1
            /* #X was
            * if (PARENT_WINDOW &&
            */
            else if (DXUI &&
                     ev.xclient.message_type == ATOM(GLDestroyWindow)) {

                _tdmCleanupChild(globals);
                goto done;
            }
#endif
            /* #X was
            * if (PARENT_WINDOW)
            */
            else if (DXUI)
                _dxfReceiveClientMessage
                (globals, (XClientMessageEvent *) &ev, &doRedraw) ;
            break ;

        case ButtonPress:
            XSetInputFocus(DPY, XWINID, RevertToPointerRoot, CurrentTime);
            eventWindow = ev.xbutton.window;
            if (flags || !_dxf_ExIsExecuting()) {
                PRINT (("ButtonPress")) ;
                if (!CORRECT_SIZE)
                    continue ;

                if ((ev.xbutton.button == Button1 && !(dxEventMask & DXEVENT_LEFT)) ||
                        (ev.xbutton.button == Button2 && !(dxEventMask & DXEVENT_MIDDLE)) ||
                        (ev.xbutton.button == Button3 && !(dxEventMask & DXEVENT_RIGHT))) {
                    XSendEvent(dpy, PARENT_WINDOW, True, 0, &ev);
                } else {
                    if (ev.xbutton.time - lasttime < 350) {
                        PRINT(("DoubleClick")) ;

                        lasttime = 0 ;
                        tdmDoubleClick (globals->CrntInteractor,
                                        ev.xbutton.x, ev.xbutton.y, &R) ;

                        /* #X was
                        * if (PARENT_WINDOW)
                        */
                        if (DXUI)
                            _dxfSendInteractorData
                            (globals, globals->CrntInteractor, &R) ;
                    } else {
                        /* sample mouse pointer to drive interactor */
                        time_t t0, t1 ;
                        Window windummy ;
                        int n, idummy, x, y, x0, y0, xlast, ylast, dx, dy, button ;

                        n = 0 ;
                        t0 = time(0) ;
                        x0 = xlast = ev.xbutton.x ;
                        y0 = ylast = ev.xbutton.y ;
                        button = ev.xbutton.button ;
                        lasttime = ev.xbutton.time ;

                        tdmStartStroke (globals->CrntInteractor, x0, y0, button, ev.xbutton.state) ;
                        XQueryPointer (dpy, eventWindow, &windummy, &windummy,
                                       &idummy, &idummy, &x, &y, &mask) ;

                        /* keep going until a button-up */
                        while (mask &
                                (Button1Mask | Button2Mask | Button3Mask |
                                 Button4Mask | Button5Mask)) {
                            /* maintain a count to compute average fling deltas */
                            if (x != xlast || y != ylast) {
                                xlast = x ;
                                ylast = y ;
                                n++ ;
                            }

                            tdmStrokePoint (globals->CrntInteractor, x, y, INTERACTOR_BUTTON_DOWN, mask) ;
                            XQueryPointer (dpy, eventWindow, &windummy, &windummy,
                                           &idummy, &idummy, &x, &y, &mask) ;
                        }

                        if (!_dxf_isFlagsSet(_dxf_SERVICES_FLAGS(),
                                             SF_FLING)) {
                            /*
                            * If user has not asked for FLING (via export of undocumented
                            * env variable)... behave normally.
                            */
                            XCheckWindowEvent (dpy, eventWindow, ButtonReleaseMask, &ev) ;
                            tdmStrokePoint (globals->CrntInteractor,
                                            ev.xbutton.x, ev.xbutton.y, INTERACTOR_BUTTON_UP, ev.xbutton.state) ;
                        } else {
                            /* See if fling conditions have been met */
                            t1 = time(0) ;
                            dx = n ? (x - x0) / n : 0 ;
                            dy = n ? (y - y0) / n : 0 ;

                            if ((dx != 0 || dy != 0) &&
#ifndef FLING_GNOMON_FEECHURE
                                    globals->executeOnChange &&
#endif
                                    (t1 - t0 < FLING_TIMEOUT) &&
                                    (x<0 || y<0 || x>PIXW || y>PIXH) &&
                                    ((button == 2 &&
                                      (globals->CrntInteractor == globals->RoamGroup ||
                                       globals->CrntInteractor == globals->CursorGroup)) ||
                                     (button != 3 &&
                                      (globals->CrntInteractor == globals->RotateGroup)))) {
                                PRINT(("flinging...")) ;
                                while (!XCheckWindowEvent (dpy,
                                                           eventWindow,
                                                           ButtonPressMask,
                                                           &ev)) {
                                    xlast += dx ;
                                    ylast += dy ;
                                    tdmStrokePoint (globals->CrntInteractor, xlast, ylast, INTERACTOR_BUTTON_DOWN, ev.xbutton.state) ;
                                }
                            } else {
                                /*
                                * if not, behave normally.
                                */
                                XCheckWindowEvent (dpy, eventWindow, ButtonReleaseMask, &ev) ;
                                if (ev.xbutton.x != x || ev.xbutton.y != y)
                                    tdmStrokePoint (globals->CrntInteractor,
                                                    ev.xbutton.x, ev.xbutton.y, INTERACTOR_BUTTON_UP, ev.xbutton.state) ;
                            }
                        }

                        _dxfSendInteractorData (globals, globals->CrntInteractor, 0) ;
                    }
                }
            }
            break ;

        case VisibilityNotify:
            PRINT(("VisibilityNotify")) ;
            VISIBILITY = ev.xvisibility.state ;
            break ;

        case Expose:
            PRINT (("Expose")) ;
            doRedraw = TRUE ;
            break ;

        case EnterNotify:
            /* we only receive this event from 8-bit windows */
            PRINT (("EnterNotify")) ;
            XInstallColormap (dpy, CLRMAP) ;
            XFlush(dpy) ;
            break ;

        case LeaveNotify:
            /* we only receive this event from 8-bit windows */
            PRINT (("LeaveNotify")) ;
            XUninstallColormap (dpy, CLRMAP) ;
            XFlush(dpy) ;
            break ;

        case DestroyNotify:
            PRINT (("DestroyNotify")) ;
            if(ev.xdestroywindow.window == TOPLEVEL)
                break;
            GL_WINDOW = FALSE;
            _tdmCleanupChild(globals) ;
            goto done ;
            break ;

        default:
            PRINT(("ignored event %d", ev.type)) ;
            break ;
        }
    }

    if (OBJECT_TAG != DXGetObjectTag(OBJECT) ||
            CAMERA_TAG != DXGetObjectTag((dxObject)CAMERA))
        SAVE_BUF_VALID = FALSE ;

#if defined(DXD_HW_WINDOW_DESTRUCTION_CHECK)

    if (GL_WINDOW)
#endif

        if (doRedraw) {
            /*
            *  If the window needs to be refreshed, lets try to do it from
            *  the saved buffer, or if that fails, redraw.
            */
            PRINT(("refreshing image")) ;
            /*
#if defined(sgi)
            if (GL_WINDOW)
            zclear();
#endif
            */
#if 0

            if (CAMERA)
#endif

            {
                /* otherwise, redo final approximation pass */
                if (! _dxfTryRestoreBuffer(LWIN))
                {
                    PRINT(("no backing store, regenerating image"));
#if 0

                    _tdmClearWindow(LWIN) ;
#endif

                    if(!(_dxfDraw (globals, OBJECT, CAMERA, 1)))
                        goto error;

                    if ( _dxfTrySaveBuffer(LWIN)) {
                        SAVEBUFOBJECTTAG = OBJECT_TAG;
                        SAVEBUFCAMERATAG = CAMERA_TAG;
                    }

                    /*
                    *  Redraw any defined interactor echos.  The echos can't
                    *  be saved by _dxfTrySaveBuffer() since they are updated
                    *  more frequently than once per rendering pass.
                    */

                    _dxfRedrawInteractorEchos(INTERACTOR_DATA) ;
                }
            }

            /* tell the UI that we're ready */
            /*
            * #X was
            * if ((fd == -1) && PARENT_WINDOW)
            */
            if ((fd == -1) && DXUI)
                _dxfConnectUI(LWIN) ;

            /*
            XXX do we still need this ? TJM
            YYY I'm not sure for the UI case, but I don't think so otherwise.
            It causes unnecessary event looping in external UI cases.
            I've changed it to ONLY do it if its the DX UI.  GDA
            */
            /* hack to force client messages through immediately */
            if (DXUI)
                _dxfSendClientMessage (dpy, XWINID, POKE_CONNECTION, 0) ;
        }

done:
    if (_tdmXerror)
        goto error ;

    EXIT(("OK"));
    if (fd != -1)
        DEBUG_MARKER("_dxfProcessEvents EXIT");
    return OK ;

error:
    _tdmXerror = 0 ;

    EXIT(("ERROR"));
    if (fd != -1)
        DEBUG_MARKER("_dxfProcessEvents EXIT");
    return ERROR ;
}

#if 0
typedef struct argbS
{
    char a, b, g, r;
}
argbT,*argbP;


void *_dxfOutputRGB(WinP win, Field i)
{
    DEFWINDATA(win);
    int x, y, n, r, g, b;
    int w, h;
    RGBColor *pixels;
    RGBColor *from;

    ENTRY(("_dxfOutputRGB(0x%x, 0x%x)", win, i));

    /* the next two can be static as they are only set once */
    /* (all subsequent child processes will merely read them) */
    static int firsttime = 1;
    static unsigned char gamma[256];

    if (!i) {
        /* no image received for rendering */
        DXSetError (ERROR_UNEXPECTED, "#13500" ) ;
        EXIT(("no image"));
        return NULL ;
    }

    /* increment reference count here, decrement in _dxfEndSWRenderPass() */
    DXReference((dxObject)i) ;

    if (DXGetObjectClass ((dxObject)(i)) != CLASS_FIELD) {
        /* invalid image received */
        DXSetError (ERROR_BAD_CLASS, "#13510") ;
        EXIT(("invalid image"));
        return NULL ;
    }

    if (! DXGetImageSize (i, &w, &h)) {
        /* could not get image size */
        DXSetError (ERROR_INTERNAL, "#13520");
        EXIT(("could not get image size"));
        return NULL ;
    }

    pixels = DXGetPixels(i) ;
    if (! pixels) {
        DXWarning ("#5170") ;   /* no pixels found in image */
        EXIT(("no pixles"));
        return NULL ;
    }

    n = PIXW * PIXH * sizeof(argbT);
    if (n != SW_BUF_SIZE) {
        if (SW_BUF)
            tdmFree(SW_BUF) ;
        SW_BUF = (void *) NULL;
        SW_BUF = (void *) tdmAllocateLocal(n);
        if (!SW_BUF) {
            SW_BUF_SIZE = 0;
            EXIT(("malloc failed"));
            return NULL;
        } else
            SW_BUF_SIZE = n;
    }

    /* create the gamma ramp */
    if (firsttime) {
        /* create the map -- code adapted from BuildXColorMap in display.c */
        int i;

        for (i=0; i<256; i++)
            gamma[i] = sqrt(i/256.0)*256.0;
        firsttime = 0;
    }

    /* work around an sgi compiler bug */
#define CLAMP(p) (p=(p<0?0:p), p=(p>255?255:p))

    /* convert from XY to linear index */
#define INDX(x,y) (y*w + x)

    for (y=0; y<h; y++) {
        from = pixels + y * w;
        for (x=0; x<w; x++) {
            /* it should be possible to simplify this, but every time I try to
            some of the high levels overflow and wrap to 0 */
            r = 255*from[x].r;
            g = 255*from[x].g;
            b = 255*from[x].b;
            r = gamma[CLAMP(r)];
            g = gamma[CLAMP(g)];
            b = gamma[CLAMP(b)];
            ((argbP) SW_BUF)[INDX(x,y)].r = CLAMP(r);
            ((argbP) SW_BUF)[INDX(x,y)].g = CLAMP(g);
            ((argbP) SW_BUF)[INDX(x,y)].b = CLAMP(b);
        }
    }

    /*
    * Mark buffer as current, and draw it.
    */
    SW_BUF_CURRENT = TRUE;
    _dxfSWImageDraw(win);

    EXIT((""));
}

void
_dxfSWImageDraw (WinP win)
{
    int camw, camh ;
    DEFWINDATA(win);
    DEFPORT(PORT_HANDLE);

    ENTRY(("_dxfSWImageDraw(0x%x)", win));

    _dxfChangeBufferMode (win, SingleBufferMode) ;

    if (CAMERA)
        DXGetCameraResolution (CAMERA, &camw, &camh) ;
    else
        DXGetImageSize (OBJECT, &camw, &camh) ;

    if (camw < PIXW)
        _dxf_CLEAR_AREA (PORT_CTX, camw, PIXW - 1, 0, PIXH - 1) ;

    if (camh < PIXH)
        _dxf_CLEAR_AREA (PORT_CTX, 0, PIXW - 1, 0, PIXH - (camh+1)) ;

    _dxf_WRITE_PRECISE_RENDERING (win, camw, camh) ;

    EXIT((""));
}
#endif

void
_dxfChangeBufferMode(WinP win, int mode)
{
    DEFWINDATA(win);
    DEFPORT(PORT_HANDLE);

    ENTRY(("_dxfChangeBufferMode(0x%x, %d)",win,mode));

    if (mode == DoubleBufferMode) {
        if (BUFFER_MODE == SingleBufferMode) /* needs changing? */
        {
            PRINT (("single buffer -> double buffer")) ;
            _dxf_DOUBLE_BUFFER_MODE(PORT_CTX) ;
            BUFFER_MODE = DoubleBufferMode ;
        }
    } else /* mode == SingleBufferMode */
    {
        if (BUFFER_MODE == DoubleBufferMode) /* needs changing? */
        {
            /*
            *  If we're going from double to single buffer mode,
            *  clear the back buffer first to keep junk from showing
            *  when buffer becomes visible.
            */

            PRINT (("double buffer -> single buffer")) ;
            _tdmClearWindow(win) ;
            _dxf_SINGLE_BUFFER_MODE(PORT_CTX) ;
            BUFFER_MODE = SingleBufferMode ;
        }
    }

    EXIT((""));
}


static void
_tdmClearWindow (WinP win)
{
    DEFWINDATA(win) ;
    DEFPORT(PORT_HANDLE) ;

    ENTRY(("_tdmClearWindow (0x%x)",win));

    _dxf_CLEAR_AREA (PORT_CTX, 0, PIXW - 1 , 0, PIXH - 1) ;

    EXIT((""));
}

int
_dxfTrySaveBuffer (WinP win)
{
    XEvent xev ;
    XVisibilityEvent *xvis = (XVisibilityEvent *) &xev ;
    int camw, camh ;
    DEFWINDATA(win) ;
    DEFPORT(PORT_HANDLE) ;

    ENTRY(("_dxfTrySaveBuffer (0x%x)", win));

    /* loop all Visibility Events to determine visibility state */
    while (XCheckTypedWindowEvent (DPY, XWINID, VisibilityNotify, &xev))
        VISIBILITY = xvis->state ;

    /* if we already have a buffer, leave now */
    if (SAVE_BUF_VALID)
        goto done ;

    if (CAMERA) {
        if (! DXGetCameraResolution(CAMERA, &camw, &camh))
            /* unable to get camera resolution */
            DXErrorGoto (ERROR_DATA_INVALID, "#13530") ;
    } else {
        /* assume we have been given an image */
        if (! DXGetImageSize (OBJECT, &camw, &camh))
            /* unable to get image resolution */
            DXErrorGoto (ERROR_DATA_INVALID, "#13540") ;
    }

    /* read the window */
    if (! _dxf_READ_APPROX_BACKSTORE(win, camw, camh)) {
        /* can't read saveunder */
        DXWarning("#13550") ;
        SAVE_BUF_VALID = FALSE ;
        goto done ;
    }

    /* check to see if we are obscured */
    if (VISIBILITY != VisibilityUnobscured)
        SAVE_BUF_VALID = FALSE ;
    else {
        /* check for Exposures and invalidate result if present */
        if (XCheckTypedWindowEvent(DPY, XWINID, Expose, &xev)) {
            XPutBackEvent(DPY, &xev) ;
            SAVE_BUF_VALID = FALSE ;
            goto done ;
        } else
            SAVE_BUF_VALID = TRUE ;
    }


    /* give access to direct interactors */
    _dxfSetInteractorImage (INTERACTOR_DATA, camw, camh, SAVE_BUF) ;

    /*
    * In some cases the backing store may be simply filled with
    * black pixels (i.e. Evans and Sutherland).  In this case
    * we want the interactors to access this memory (previous line)
    * but we do not want to use the backing store for redraws.  This
    * is a temporary solution to E&S glReadPixel performance probs.
    */
    if (_dxf_isFlagsSet(_dxf_SERVICES_FLAGS(),
                        SF_INVALIDATE_BACKSTORE)) {
        SAVE_BUF_VALID = FALSE ;
    }

done:
    if (_tdmXerror)
        goto error ;

    EXIT((""));
    return 1;

error:
    _tdmXerror = 0 ;

    EXIT(("ERROR"));
    return 0;
}


int
_dxfTryRestoreBuffer (WinP win)
{
    int camw, camh ;
    DEFWINDATA(win);
    DEFPORT(PORT_HANDLE) ;

    ENTRY(("_dxfTryRestoreBuffer (0x%x)",win));

    /* check buffer status */
    if (! SAVE_BUF_VALID)
        goto error ;

    if (CAMERA) {
        if (! DXGetCameraResolution(CAMERA, &camw, &camh))
            /* unable to get camera resolution */
            DXErrorGoto (ERROR_DATA_INVALID, "#13530") ;
    } else {
        /* assume we have been given an image */
        if (! DXGetImageSize (OBJECT, &camw, &camh))
            /* unable to get image resolution */
            DXErrorGoto (ERROR_DATA_INVALID, "#13540") ;
    }

    /* check for presence of buffer and size */
    if (! SAVE_BUF || (SAVE_BUF_SIZE != camw * camh))
        goto error ;

    if (camw < PIXW)
        _dxf_CLEAR_AREA (PORT_CTX, camw, PIXW - 1, 0, PIXH - 1) ;

    if (camh < PIXH)
        _dxf_CLEAR_AREA (PORT_CTX, 0, PIXW - 1, 0, PIXH - (camh+1)) ;

    PRINT(("writing from backing store"));
    _dxf_SET_OUTPUT_WINDOW(win, XWINID);
    _dxf_WRITE_APPROX_BACKSTORE (win, camw, camh) ;
#if defined(solaris)

    _dxf_SWAP_BUFFERS(PORT_CTX, XWINID);
#endif

    /* If we restored from a (partialially) invalid buffer, make sure we
    * still refresh.
    */
#if 0

    if (! SAVE_BUF_VALID )
        goto error ;
#endif

    EXIT(("OK"));
    return OK ;

error:
    _tdmXerror = 0 ;

    EXIT(("ERROR"));
    return ERROR ;
}


static void
_tdmCleanupChild (tdmChildGlobalP globals)
{
    DEFGLOBALDATA(globals) ;

    ENTRY(("_tdmCleanupChild (0x%x)", globals));

    if (globals->cacheId) {
        /*
        *  DXRemove cache entry for this instance.  This will cause
        *  _dxfEndRenderModule() to be invoked as a side effect, since it is
        *  registered as the callback for the cache deletion.
        */
        PRINT(("cacheId exists, deleting cache entry")) ;
        DXSetCacheEntry (0, CACHE_PERMANENT, globals->cacheId, 0, 0) ;

    }

    EXIT((""));
}


int
_dxfConvertWinName(char *winName)
{
    int i ;

    /* ENTRY(("_dxfConvertWinName(\"%s\")",winName)); */

    for (i = strlen(winName)-1 ; i >= 0 ; i--)
        if (!isdigit(winName[i]))
            break;

    if (i == -1) {
        /* no pound signs, all digits */
        /* EXIT(("0")); */
        return 0 ;
        /* #X was
        * } else if (winName[i] == '#' && (i !=0 && winName[i-1] == '#')) {
        */
    } else if ((winName[i] == '#' || winName[i] == 'X') && (i !=0 && winName[i-1] == '#')) {
        /* 2 pound signs */
        /* EXIT(("fill in this value")); */
        return atoi(&winName[i+1]) ;
    } else {
        /* EXIT(("0")); */
        return 0 ;
    }
}

/*
* #X follows
*/
int
_dxfUIType(char *winName)
{

    int i ;

    for (i = strlen(winName)-1 ; i >= 0 ; i--)
        if (!isdigit(winName[i]))
            break;

    if (i > 0 && winName[i] == '#' && i !=0 && winName[i-1] == '#')
        return DXD_DXUI;
    else if (i > 0 && winName[i] == 'X' && i !=0 && winName[i-1] == '#')
        return DXD_EXTERNALUI;
    else
        return DXD_NOUI;
}
/* #X end */

static Window
_getTopLevelShell (Display *dpy, Window w)
{
    /*
    *  Find the top level shell widget by walking up the window hierarchy.
    *
    *  NOTE: This routine WILL NOT WORK if the Local allocator is not
    *  malloc!  It calls XQueryTree() which uses malloc to allocate the
    *  children array!
    */

    unsigned int numChild ;
    Window root, parent, *childList ;

    ENTRY(("_getTopLevelShell (0x%x, 0x%x)", dpy, w));

    while (XQueryTree (dpy, w, &root, &parent, &childList, &numChild)) {
        XFree((char *)childList) ;

        if (parent != root)
            w = parent ;
        else {
            EXIT(("w = 0x%x",w)) ;
            return w ;
        }
    }

    EXIT(("XQueryTree failed")) ;
    return 0 ;
}

extern Field DXMakeImageFormat(int w, int h, char *format);

Field
_dxfCaptureHardwareImage(tdmChildGlobalP globals)
{
    DEFGLOBALDATA(globals) ;
    DEFPORT(PORT_HANDLE) ;
    Field image = NULL;
    Array colors;

    image = DXMakeImageFormat(PIXW, PIXH, "BYTE");
    if (! image)
        goto error;

    colors = (Array)DXGetComponentValue(image, "colors");
    if (! colors)
        goto error;

    if (! _dxf_READ_IMAGE(LWIN, DXGetArrayData(colors)))
        goto error;

    return image;

error:
    DXDelete((dxObject)image);
    return NULL;
}

#endif /* DX_NATIVE_WINDOWS */

void
_dxfSetCurrentView(WinP win, float *to, float *from, float *up, float fov, float width)
{
    DEFWINDATA(win);
    CURRENT_TO[0]   = to[0];
    CURRENT_TO[1]   = to[1];
    CURRENT_TO[2]   = to[2];
    CURRENT_FROM[0] = from[0];
    CURRENT_FROM[1] = from[1];
    CURRENT_FROM[2] = from[2];
    CURRENT_UP[0]   = up[0];
    CURRENT_UP[1]   = up[1];
    CURRENT_UP[2]   = up[2];
    CURRENT_FOV     = fov;
    CURRENT_WIDTH   = width;
}

/*
 *===================================================================
 *                END OF FILE 
 *                $Source: /src/master/dx/src/exec/hwrender/hwWindow.c,v $
 *===================================================================
 */
#undef tdmWindow_c
