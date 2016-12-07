/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
#define String dxString
#define Object dxObject
#define Angle dxAngle
#define Matrix dxMatrix
#define Screen dxScreen
#define Boolean dxBoolean

#include <dxconfig.h>


#include <dx/dx.h>

#undef String
#undef Object
#undef Angle
#undef Matrix
#undef Screen
#undef Boolean

#if !defined(DX_NATIVE_WINDOWS)
#include <X11/Xlib.h>
#endif

#include <math.h>

#include "hwDeclarations.h"
#include "hwWindow.h"
#include "hwStereo.h"


/************** Default Stereo Camera Mode ********************
 * The arguments of the default camera define a asymmetric frustum
 * and ocular separation
 ****************************************************************/

typedef struct
{
    float overlap;
    float separation;
    float aspect;
    float lp[16];
    float rp[16];
} StereoArgs;
    

static void *defInitializeStereoCameraMode0(void *data, dxObject arg);
static int  defExitStereoCameraMode0(void *data);
static int  defCreateStereoCameras0(void *,
					int, float, float,
					float *, float *, float *, float, float,
					float *, float *, float *, float **,
					float *, float *, float *, float **);
#if defined(DX_NATIVE_WINDOWS)
/* Place holder for windows version -- correct prototype? */
static int  defMapStereoXY0(void *, HWND, HWND, WindowInfo,
							int, int, int*, int*);
#else
static int  defMapStereoXY0(void *, Window, Window, WindowInfo,
					int, int, int*, int*);
#endif

static StereoCameraMode _defaultStereoCameraModes[1];

int
DXDefaultStereoCameraModes(int *nModes, StereoCameraMode **scms)
{
    *scms = _defaultStereoCameraModes;

    (*scms)[0].initializeStereoCameraMode = defInitializeStereoCameraMode0;
    (*scms)[0].exitStereoCameraMode       = defExitStereoCameraMode0;
    (*scms)[0].createStereoCameras        = defCreateStereoCameras0;
    (*scms)[0].mapStereoXY 		  = defMapStereoXY0;
    
    *nModes = 1;

    return OK;
}

static void *
defInitializeStereoCameraMode0(void *data, dxObject arg)
{
    if (!data) 
	data = (void *)DXAllocate(sizeof(StereoArgs));
    
    if (!arg || !DXExtractParameter(arg, TYPE_FLOAT, 3, 1, (Pointer)data))
    {
	DXSetError(ERROR_BAD_PARAMETER, "bad args to default stereo camera model");
	goto error;
    }

    return data;

error:
    DXFree((Pointer)data);
    return NULL;
}

static int
defExitStereoCameraMode0(void *data)
{
    DXFree((Pointer)data);
    return OK;
}

static int
defCreateStereoCameras0(void *data,
		    int p, float fov, float width,
		    float *to, float *from, float *up, float n, float f,
		    float *lto, float *lfrom, float *lup, float **lProj,
		    float *rto, float *rfrom, float *rup, float **rProj)
{
    StereoArgs *args = (StereoArgs *)data;
    float vx, vy, vz;
    float ux, uy, uz;
    float ax, ay, az;
    float d;
    float l, r, b, t;
    float *lp = args->lp;
    float *rp = args->rp;

    vx = from[0] - to[0];
    vy = from[1] - to[1];
    vz = from[2] - to[2];
    d = sqrt(vx*vx + vy*vy + vz*vz);
    vx = vx / d;
    vy = vy / d;
    vz = vz / d;

    ux = up[0];
    uy = up[1];
    uz = up[2];
    d = sqrt(ux*ux + uy*uy + uz*uz);
    ux = ux / d;
    uy = uy / d;
    vz = vz / d;

    ax = uy*vz - uz*vy;
    ay = uz*vx - ux*vz;
    az = ux*vy - uy*vx;

    lfrom[0] = from[0] - args->separation*ax;
    lfrom[1] = from[1] - args->separation*ay;
    lfrom[2] = from[2] - args->separation*az;
    lto[0]   = to[0]   - args->separation*ax;
    lto[1]   = to[1]   - args->separation*ay;
    lto[2]   = to[2]   - args->separation*az;
    lup[0]   = up[0];
    lup[1]   = up[1];
    lup[2]   = up[2];

    rfrom[0] = from[0] + args->separation*ax;
    rfrom[1] = from[1] + args->separation*ay;
    rfrom[2] = from[2] + args->separation*az;
    rto[0]   = to[0]   + args->separation*ax;
    rto[1]   = to[1]   + args->separation*ay;
    rto[2]   = to[2]   + args->separation*az;
    rup[0]   = up[0];
    rup[1]   = up[1];
    rup[2]   = up[2];

    {
	float t = n;
	n = f;
	f = t;
    }

    r = args->overlap;
    l = r - 1;
    t = (0.5 * args->aspect);
    b = -t;

    if (p)
    {
	r *= fov;
	l *= fov;
	t *= fov;
	b *= fov;
    }
    else
    {
	r *= width;
	l *= width;
	t *= width;
	b *= width;
    }

    lp[ 0] = 2/(r-l); lp[ 4] =     0.0; lp[ 8] = (r+l)/(r-l); lp[12] =         0.0;
    lp[ 1] = 0.0;     lp[ 5] = 2/(t-b); lp[ 9] = (t+b)/(t-b); lp[13] =         0.0;
    lp[ 2] = 0.0;     lp[ 6] =     0.0; lp[10] = (f+n)/(f-n); lp[14] = 2*f*n/(f-n);
    lp[ 3] = 0.0;     lp[ 7] =     0.0; lp[11] =        -1.0; lp[15] =         0.0;

    *lProj = lp;

    l = -args->overlap;
    r = l + 1;
    t = (0.5 * args->aspect);
    b = -t;

    if (p)
    {
	r *= fov;
	l *= fov;
	t *= fov;
	b *= fov;
    }
    else
    {
	r *= width;
	l *= width;
	t *= width;
	b *= width;
    }

    rp[ 0] = 2/(r-l); rp[ 4] =     0.0; rp[ 8] = (r+l)/(r-l); rp[12] =         0.0;
    rp[ 1] = 0.0;     rp[ 5] = 2/(t-b); rp[ 9] = (t+b)/(t-b); rp[13] =         0.0;
    rp[ 2] = 0.0;     rp[ 6] =     0.0; rp[10] = (f+n)/(f-n); rp[14] = 2*f*n/(f-n);
    rp[ 3] = 0.0;     rp[ 7] =     0.0; rp[11] =        -1.0; rp[15] =         0.0;

    *rProj = rp;

    return OK;
}

#if defined(DX_NATIVE_WINDOWS)
static int
defMapStereoXY0(void *data, HWND frame, HWND w, WindowInfo wi,
				int xi, int yi, int *xo, int *yo)
#else
static int
defMapStereoXY0(void *data, Window frame, Window w, WindowInfo wi,
			int xi, int yi, int *xo, int *yo)
#endif
{
    *xo = xi;
    *yo = yi;
    return 0;
}



static void *
defInitializeStereoCameraMode1(void *data, dxObject arg)
{
    if (!data) 
	data = (void *)DXAllocate(sizeof(StereoArgs));
    
    if (!arg || !DXExtractParameter(arg, TYPE_FLOAT, 3, 1, (Pointer)data))
    {
	DXSetError(ERROR_BAD_PARAMETER, "bad args to default stereo camera model");
	goto error;
    }

    return data;

error:
    DXFree((Pointer)data);
    return NULL;
}

