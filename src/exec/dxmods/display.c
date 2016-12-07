/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>

#if sgi
#include <limits.h>
#endif
#include <string.h>
#include <stdio.h>
#include <dx/dx.h>
#include <stdlib.h>
#include "../libdx/displayutil.h"

static Matrix Identity = {
                             {{ 1.0, 0.0, 0.0 },
                              { 0.0, 1.0, 0.0 },
                              { 0.0, 0.0, 1.0 }}
                         };

#if defined(DX_NATIVE_WINDOWS)
/* This structure is defined to allow PostMessage to send a Windows
 * message back to a window with the information needed to extract
 * the actual image. 
 */
typedef struct _WindowsPixels {
	int numBytes;
	int width;
	int height;
	byte * pixels; } WindowsPixels;
#endif

#if 0
#include <dl.h>
#include <sys/stat.h>
#include <errno.h>
#endif

extern Object _dxf_EncodeImage(Field); /* from libdx/buffer.c */
extern Field  _dxf_DecodeImage(Object); /* from libdx/buffer.c */

#ifdef DXD_HAS_LIBIOP
#define RLE_DEFAULT	0
#else
#define RLE_DEFAULT	1
#endif

static Object message(Object image, char *name, int buttonState,
                      int ddcamera, Object o);
static Error _link_message(char *link, char *where, Camera c);
static Error _dxf_NoHardwareMessage();

#if DXD_CAN_HAVE_HW_RENDERING
Error _dxfAsyncRender (Object, Camera, char *, char *);
Error _dxfAsyncDelete (char *);
#endif

int
m_Display(Object *in, Object *out)
{
    char *where, copy[201], *s, type[201];
    char *arg1=NULL, *arg2=NULL, *arg3=NULL, *arg4=NULL, *arg5=NULL;
    Object image=NULL, object=NULL, mode, bstate, ddcam;
    Camera camera;
    char   cacheid[201];	/* "Display.xxx" */
    int ui_window = 0;
    int external_window = 0;
    double *old = NULL, new;
    Private p;
    int buttonState, ddcamera;

    /* throttle - borrow the where parameter for a while */
    if (in[2] && DXExtractString(in[2], &where))
        ;
    else
        where = "throttle";

    p = (Private) DXGetCacheEntry(where, 0, 0);
    if (p)
        old = (double *) DXGetPrivateData(p);

    if (in[3]) {
        float throttle;

        if (!DXExtractFloat(in[3], &throttle))
            DXErrorReturn(ERROR_BAD_PARAMETER, "#13610");

        if (throttle < 0)
            DXErrorReturn(ERROR_BAD_PARAMETER, "#13620");

        if (throttle && p && old) {
            new = DXGetTime();
            if ((throttle - new + *old) > 0)
                DXWaitTime(throttle - new + *old);
        }
    }

    if (!p) {
        old = (double *) DXAllocate(sizeof(double));
        if (!old)
            return ERROR;

        p = DXNewPrivate((Pointer)old, DXFree);
        if (!p)
            return ERROR;

        if (!DXSetCacheEntry((Object)p, CACHE_PERMANENT, where, 0, 0)) {
            DXDelete((Object)p);
            old = NULL;
        }
    }
    if (old)
        *old = DXGetTime();


    /* object parameter */
    object = in[0];

    /* camera parameter */
    camera = (Camera) in[1];

#if 0

    DXPrintV((Object)camera, "rd", 0);
#endif

    /* extract where parameter */
    if (in[2]) {
        char *c;

        if (!DXExtractString(in[2], &where))
            DXErrorGoto(ERROR_BAD_PARAMETER, "#13630");

        if (where)
            for (c = where; c[0] && c[1]; c++) {
                if (c[0] == '#' && c[1] == '#') {
                    ui_window ++;
                    break;
                } else if (c[0] == '#' && c[1] == 'X') {
                    external_window ++;
                    break;
                }
            }
    } else
        where = "X";


    /* check length of where parameter */
    /* XXX - better yet, make this dynamic */
    if (strlen(where)>=sizeof(copy)) {
        DXSetError(ERROR_BAD_PARAMETER, "#13660", sizeof(copy)-1);
        return ERROR;
    }

    /* parse where parameter */
    strcpy(copy, where);
    for (s=copy; *s; s++) {
        if (*s==',') {
            if (!arg1)
                arg1 = s+1;
            else if (!arg2)
                arg2 = s+1;
            else if (!arg3)
                arg3 = s+1;
            else if (!arg4)
                arg4 = s+1;
            else if (!arg5)
                arg5 = s+1;
            *s = '\0';
        }
    }
    strcpy(type, *copy? copy : "X");
    if (type[0] == 'X') 	/* matches X, X8, X12, X15, X16, X24 */
    {
        if (arg1)
        {
            strcat(type, ",");
            strcat(type, arg1);
        }
    }

    if (in[1] &&
            NULL != (mode = DXGetAttribute(object, "rendering mode"))) {
        char *s;

        if (!DXExtractString(mode, &s))
            DXErrorGoto(ERROR_BAD_PARAMETER, "#13640");

        if (strcmp(s,"hardware")==0 &&
                !DXGetCacheEntry("_DXD_NO_HARDWARE", 0, 0)) {
#if DXD_CAN_HAVE_HW_RENDERING
            DXSetSoftwareWindowActive(where, 0);
            if  (_dxfAsyncRender(object, camera, NULL, where))
                goto done;

            if (DXGetError() != ERROR_NO_HARDWARE_RENDERING)
                goto error;

            DXResetError();
            DXSetSoftwareWindowActive(where, 0);
            _dxf_NoHardwareMessage();
#else

            _dxf_NoHardwareMessage();
#endif /* DXD_CAN_HAVE_HW_RENDERING */

        }
    }

    /* look for button state */
    if (NULL != (bstate = DXGetAttribute(object, "button state"))) {
        if (!DXExtractInteger(bstate, &buttonState) ||
                (buttonState != 1 && buttonState != 2))
            DXErrorGoto(ERROR_BAD_PARAMETER,
                        "button state must be an integer and either 1 or 2");
    } else
        buttonState = 1;

    /* look for ddcamera */
    if (NULL != (ddcam = DXGetAttribute(object, "ddcamera"))) {
        if (!DXExtractInteger(ddcam, &ddcamera) ||
                (ddcamera != 1 && ddcamera != 2))
            DXErrorGoto(ERROR_BAD_PARAMETER,
                        "ddcamera must be an integer and either 1 or 2");
    } else
        ddcamera = 1;

#if DXD_CAN_HAVE_HW_RENDERING

    if (!_dxfAsyncDelete(where))
        return ERROR;
#endif /* DXD_CAN_HAVE_HW_RENDERING */

    /* render? */
    if (in[1]) {
        int obTag, renderEvery;
        char *renderApprox;
        Object attr;
        char *cacheTag;

        /*
         * stash the camera and object in cache
         */
        cacheTag = (char *)DXAllocate(strlen("CACHED_OBJECT_")
                                      + strlen(where) + 32);

        sprintf(cacheTag, "%s%s", "CACHED_OBJECT_", where);
        DXSetCacheEntry(in[0], CACHE_PERMANENT, cacheTag, 0, 0);

        sprintf(cacheTag, "%s%s", "CACHED_CAMERA_", where);
        DXSetCacheEntry(in[1], CACHE_PERMANENT, cacheTag, 0, 0);

        DXFree((Pointer)cacheTag);

        if (!object)
            DXErrorGoto(ERROR_MISSING_DATA, "#13650");

        attr = DXGetAttribute(object, "object tag");
        if (attr)
            DXExtractInteger(attr, &obTag);
        else
            obTag = DXGetObjectTag(object);

        attr = DXGetAttribute(object, "render every");
        if (attr)
            DXExtractInteger(attr, &renderEvery);
        else
            renderEvery = 1;

        attr = DXGetAttribute(object, "rendering approximation");
        if (attr)
            renderApprox = DXGetString((String)attr);
        else
            renderApprox = "none";

        sprintf (cacheid, "Display.%s.%x.%d.%s",
                 type, obTag, renderEvery, renderApprox);

        image = DXGetCacheEntry(cacheid, 0, 1, in[1]);
        if (image) {
            if (DXGetAttribute(image, "encoding type")) {
                Field new_image = _dxf_DecodeImage(image);
                if (! new_image)
                    goto error;

                DXDelete(image);
                image = DXReference((Object)new_image);
            }
        } else {
            Object encoded_image;
            int cacheIt = 1;
            Object attr;

            image = (Object) DXRender(object, (Camera)in[1], where);
            if (!image)
                goto error;

            DXReference(image);

            attr = DXGetAttribute(object, "rendering approximation");
            if (attr) {
                if (DXGetObjectClass(attr) != CLASS_STRING) {
                    DXSetError(ERROR_BAD_CLASS,
                               "rendering approximation attribute");
                    goto error;
                }

                if (strcmp(DXGetString((String)attr), "none"))
                    cacheIt = 0;
            }

            if (cacheIt) {
                attr = DXGetAttribute(object, "cache");
                if (attr)
                    if (! DXExtractInteger(attr, &cacheIt))
                        goto error;
            }

            if (cacheIt) {
                int rle;

                if (! DXGetIntegerAttribute(object, "rle", &rle))
                    rle = RLE_DEFAULT;

                if (rle) {
                    encoded_image = _dxf_EncodeImage((Field)image);
                    if (! encoded_image)
                        goto error;

                    if (!DXSetCacheEntry(encoded_image, 0.0,
                                         cacheid, 0, 1, in[1]))
                        goto error;
                } else {
                    if (!DXSetCacheEntry(image, 0.0,
                                         cacheid, 0, 1, in[1]))
                        goto error;
                }

            }

        }

    } else {

        image = DXReference(object);
        if (!image)
            DXErrorGoto(ERROR_MISSING_DATA, "#13500");
    }

    /* display */
#if defined(DX_NATIVE_WINDOWS)
	if (type[0] == 'W') {
		uint DX_DISPLAYREADY;
		HWND window;
		char *win = arg2;
		unsigned char* bytes;
		WindowsPixels wp;
		int width, height;

		DX_DISPLAYREADY = RegisterWindowMessage("DX_DISPLAY_READY");
		win+=2;
		if(ui_window && !message(image, arg2, buttonState, ddcamera, object))
			goto error;
		// If doing 64 bit, will need to worry about changing atoi to atoi64
		if (!DXGetImageBounds(image, NULL, NULL, &width, &height))
            goto error;

		bytes = _dxf_GetWindowsPixels(image, NULL);
		wp.numBytes = width * height * 3;
		wp.pixels = bytes;
		wp.width = width;
		wp.height = height;
		printf("pixels ptr: %p\n", wp.pixels);
		fflush(stdout);
		window = (HWND) atoi(win); //parse after ## of arg1
		if(window && bytes) {
			if(PostMessage(window, DX_DISPLAYREADY, &wp, 0)==0){
				/* Error */
				TCHAR szBuf[80]; 
				LPVOID lpMsgBuf;
				DWORD dw = GetLastError(); 

				FormatMessage(
					FORMAT_MESSAGE_ALLOCATE_BUFFER | 
					FORMAT_MESSAGE_FROM_SYSTEM,
					NULL,
					dw,
					MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
					(LPTSTR) &lpMsgBuf,
					0, NULL );

				wsprintf(szBuf, 
					"%s failed with error %d: %s", 
					"PostMessage", dw, lpMsgBuf); 

				MessageBox(NULL, szBuf, "Error", MB_OK); 

				LocalFree(lpMsgBuf);
			}
		}
		else
			goto error;
	} else
#endif
	if (type[0] == 'X') {

        if (ui_window && !message(image, arg2, buttonState, ddcamera, object))
            goto error;

        if (external_window && !_link_message(arg3, where, camera))
            goto error;

#if defined(DX_NATIVE_WINDOWS)

        if(!DXDisplay(image, 0, arg1, arg2))
            goto error;
#else

        if(!DXDisplay(image, _dxf_getXDepth(type), arg1, arg2))
            goto error;
#endif /* DX_NATIVE_WINDOWS */

    } else if (strcmp(type,"FB")==0) {
        int x, y;
        if (!message(image, arg4, buttonState, ddcamera, object))
            goto error;
        x = arg2? atoi(arg2) : 0;
        y = arg3? atoi(arg3) : 0;
        if (!DXDisplayFB(image, arg1, x, y))
            goto error;
    } else {
        DXSetError(ERROR_BAD_PARAMETER, "#13630");
        goto error;
    }

    DXDelete(image);

done:
    out[0] = (Object)DXNewString(where);
    return OK;

error:
    DXDelete(image);
    return ERROR;
}


#define THREE(v) v.x,v.y,v.z

static Object
message(Object image, char *name, int buttonState, int ddcamera, Object object)
{
    Camera c;
    Array a;
    Point *box1, corner[4], from, to, up;
    Matrix m;
    int i, width, height;
    float cwidth, aspect, fov;
    float minX = DXD_MAX_FLOAT, maxX = -DXD_MAX_FLOAT;
    float minY = DXD_MAX_FLOAT, maxY = -DXD_MAX_FLOAT;
    float minZ = DXD_MAX_FLOAT, maxZ = -DXD_MAX_FLOAT;

    c = (Camera) DXGetAttribute(image, "camera");
    a = (Array) DXGetAttribute(image, "object box");
    box1 = (Point *)DXGetArrayData(a);
    if (!name)
        name = "";

    if (c && box1) {

        m = Identity;

#if 0

        Object attr;
        if (NULL != (attr = DXGetAttribute(object, "autoaxes"))) {
            int aaflag;

            if (! DXExtractInteger(attr, &aaflag)) {
                DXSetError(ERROR_INTERNAL,
                           "autoaxes attribute is not an integer");
                return NULL;
            }

            if (aaflag) {
                if (! DXGetXformInfo((Xform)object, NULL, &m))
                    return NULL;
            }
        }
#endif

        DXGetView(c, &from, &to, &up);

        if (!DXGetImageBounds(image, NULL, NULL, &width, &height))
            return NULL;

        /* t = DXGetCameraMatrix(c); */
        for (i=0; i<8; i++) {
            if (box1[i].x > maxX)
                maxX = box1[i].x;
            if (box1[i].x < minX)
                minX = box1[i].x;
            if (box1[i].y > maxY)
                maxY = box1[i].y;
            if (box1[i].y < minY)
                minY = box1[i].y;
            if (box1[i].z > maxZ)
                maxZ = box1[i].z;
            if (box1[i].z < minZ)
                minZ = box1[i].z;
        }

        corner[0].x = minX;
        corner[0].y = minY;
        corner[0].z = minZ;
        corner[1].x = minX;
        corner[1].y = minY;
        corner[1].z = maxZ;
        corner[2].x = minX;
        corner[2].y = maxY;
        corner[2].z = minZ;
        corner[3].x = maxX;
        corner[3].y = minY;
        corner[3].z = minZ;

        if (DXGetOrthographic(c, &cwidth, &aspect)) {

            DXUIMessage("IMAGE", "%s %dx%d width=%.6g aspect=%.3f "
                        "from=(%.6g,%.6g,%.6g) "
                        "to=(%.6g,%.6g,%.6g) "
                        "up=(%.6g,%.6g,%.6g) "
                        "box=(%.6g,%.6g,%.6g)(%.6g,%.6g,%.6g)"
                        "(%.6g,%.6g,%.6g)(%.6g,%.6g,%.6g) "
                        "aamat=(%.6g,%.6g,%.6g)(%.6g,%.6g,%.6g)"
                        "(%.6g,%.6g,%.6g)(%.6g,%.6g,%.6g) "
                        " ddcamera=%d button=%d",
                        name, width, height, cwidth, aspect,
                        THREE(from), THREE(to), THREE(up),
                        THREE(corner[0]), THREE(corner[1]),
                        THREE(corner[2]), THREE(corner[3]),
                        m.A[0][0], m.A[0][1], m.A[0][2],
                        m.A[1][0], m.A[1][1], m.A[1][2],
                        m.A[2][0], m.A[2][1], m.A[2][2],
                        m.b[0], m.b[1], m.b[2],
                        ddcamera, buttonState);

        } else if (DXGetPerspective(c, &fov, &aspect)) {

            DXUIMessage("IMAGE", "%s %dx%d fov=%.6g aspect=%.3f "
                        "from=(%.6g,%.6g,%.6g) "
                        "to=(%.6g,%.6g,%.6g) "
                        "up=(%.6g,%.6g,%.6g) "
                        "box=(%.6g,%.6g,%.6g)(%.6g,%.6g,%.6g)"
                        "(%.6g,%.6g,%.6g)(%.6g,%.6g,%.6g) "
                        "aamat=(%.6g,%.6g,%.6g)(%.6g,%.6g,%.6g)"
                        "(%.6g,%.6g,%.6g)(%.6g,%.6g,%.6g) "
                        " ddcamera=%d button=%d",
                        name, width, height, fov, aspect,
                        THREE(from), THREE(to), THREE(up),
                        THREE(corner[0]), THREE(corner[1]),
                        THREE(corner[2]), THREE(corner[3]),
                        m.A[0][0], m.A[0][1], m.A[0][2],
                        m.A[1][0], m.A[1][1], m.A[1][2],
                        m.A[2][0], m.A[2][1], m.A[2][2],
                        m.b[0], m.b[1], m.b[2],
                        ddcamera, buttonState);

        }

    } else {

        if (!DXGetImageBounds(image, NULL, NULL, &width, &height))
            return NULL;
        DXUIMessage("IMAGE", "%s %dx%d", name, width, height);

    }

    DXMarkTime("uimessage");
    return image;
}

#define CACHE_APPENDAGE "CACHED_CAMERA_"

static Error
_link_message(char *link, char *where, Camera c)
{
    Point F, T;
    Vector U;
    char *tag;
    float width = 0;
    int   pixwidth;
    float aspect;
    float fov = 0;
    int projection;

    DXGetView(c, &F, &T, &U);

    if (DXGetOrthographic(c, &width, &aspect)) {
        projection = 0;
    } else {
        DXGetPerspective(c, &fov, &aspect);
        projection = 1;
    }

    DXGetCameraResolution(c, &pixwidth, NULL);

    DXUIMessage("LINK",
                "VALUE camera %s [%f %f %f] [%f %f %f] [%f %f %f] %f %d %f %f %d",
                where,
                T.x, T.y, T.z,
                U.x, U.y, U.z,
                F.x, F.y, F.z,
                width, pixwidth, aspect, fov, projection);


    DXReference((Object)c);
    tag = DXAllocate(strlen(CACHE_APPENDAGE) + strlen(where) + 10);
    sprintf(tag, "%s%s", CACHE_APPENDAGE, where);
    DXSetCacheEntry((Object)c, CACHE_PERMANENT, tag, 0, 0);
    DXFree(tag);
    DXUnreference((Object)c);

    DXReadyToRunNoExecute(link);

    return OK;
}

static Error
_dxf_NoHardwareMessage()
{
    if (! DXGetCacheEntry("_DXD_NO_HARDWARE", 0, 0)) {
        String str = DXNewString("no hardware rendering");
        if (! str)
            return ERROR;

        DXSetCacheEntry((Object)str,
                        CACHE_PERMANENT, "_DXD_NO_HARDWARE", 0, 0);

        if (DXGetError() != ERROR_NONE) {
            DXWarning(DXGetErrorMessage());
            DXWarning("(couldn't load hardware rendering library)");
        } else
            DXWarning("hardware rendering is unavailable");
    }

    DXResetError();

    return OK;
}

