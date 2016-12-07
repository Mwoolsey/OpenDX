/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>

#include "cameraClass.h"

static Matrix Identity = {
  {{ 1.0, 0.0, 0.0 },
   { 0.0, 1.0, 0.0 },
   { 0.0, 0.0, 1.0 }
  }
};

/*
 * Compute the camera matrix from the current parameters
 */

#define ZERO(v) (v.x==0 && v.y==0 && v.z==0)

Camera
_dxfSetCameraProjection(Camera c, float *p)
{
    Matrix t;

    t.A[0][0] = p[ 0]; t.A[0][1] = p[ 1]; t.A[0][3] = p[ 2];
    t.A[1][0] = p[ 4]; t.A[1][1] = p[ 5]; t.A[1][3] = p[ 6];
    t.A[2][0] = p[ 8]; t.A[2][1] = p[ 9]; t.A[2][3] = p[10];
    t.b[0]    = p[12]; t.b[1]    = p[13]; t.b[2]    = p[14];

    c->m = c->rot = t;
    return c;
}


static Camera
matrix(Camera c)
{
    Vector xaxis, yaxis, zaxis, v;
    Matrix t;
    float xres;

    /* compute resolution */
    xres = c->resolution; /* yres = c->resolution*c->aspect/c->pix_aspect; */

    /* translate to origin of camera coordinate system */
    t = DXTranslate(DXNeg(c->from));

    /* camera coordinate system axes */
    v = DXSub(c->from, c->to);
    if (ZERO(v))
	DXErrorReturn(ERROR_BAD_PARAMETER,
		    "camera from and to points are identical");
    zaxis = DXNormalize(v);
    v = DXCross(c->up, zaxis);
    if (ZERO(v))
	DXErrorReturn(ERROR_BAD_PARAMETER, "degenerate camera up vector");
    xaxis = DXNormalize(v);
    yaxis = DXNormalize(DXCross(zaxis, xaxis));
    t = DXConcatenate(t, DXMat(
        xaxis.x, yaxis.x, zaxis.x,
        xaxis.y, yaxis.y, zaxis.y,
        xaxis.z, yaxis.z, zaxis.z,
	0,       0,       0		       
    ));
    c->rot = t;

    /*
     * scaling to resolution
     * In ortho case, we scale z similarly to x and y so that transformed
     * objects retain their shape, so that e.g. volume rendering can
     * look at voxel shape after transformation to determine what algorithm
     * to use.  Is this the best way to do this?  XXX - what about
     * perspective case?  XXX - fails if pix_aspect is not 1
     * XXX - undid this, because it distorts z values for volume
     * rendering - how to handle this?
     */
#if 1
    if (c->width==0)
	DXErrorReturn(ERROR_BAD_PARAMETER, "camera width is zero");
    t = DXConcatenate(t, DXScale(xres/c->width, xres/c->width/c->pix_aspect, 1));
    if (c->ortho) {
	Vector xto;
	xto = DXApply(c->to, t);
	t.b[2] -= xto.z;
    }
#else
    if (c->ortho)
	t = DXConcatenate(t, DXScale(
	    xres/c->width, xres/c->width/c->pix_aspect, xres/c->width));
    else
	t = DXConcatenate(t, DXScale(
	    xres/c->width, xres/c->width/c->pix_aspect, 1));
#endif
    c->m = t;
    return c;
}


/*
 * Camera object
 */

Camera
_NewCamera(struct camera_class *class)
{
    Camera c = (Camera) _dxf_NewObject((struct object_class *)class);
    if (!c)
	return NULL;

    c->from = DXPt(0,0,1);
    c->to = DXPt(0,0,0);
    c->up = DXVec(0,1,0);
    c->ortho = 1;
    c->width = 2;  /* XXX - +/- 1 */
    c->aspect = 3.0/4.0;
    c->pix_aspect = 1;
    c->resolution = 640;
    c->background.r = 0.0;
    c->background.g = 0.0;
    c->background.b = 0.0;
    return matrix(c);
}

Camera
DXNewCamera()
{
    return _NewCamera(&_dxdcamera_class);
}

int
_dxfCamera_Delete(Camera c)
{
    return OK;
}


Object
_dxfCamera_Copy(Camera old, enum _dxd_copy copy)
{
    Camera new;
    new = DXNewCamera();
    if (!new)
	return NULL;
    if (!_dxf_CopyObject((Object)new, (Object)old, copy))
	return NULL;
    new->from = old->from;
    new->to = old->to;
    new->up = old->up;
    new->ortho = old->ortho;
    new->width = old->width;
    new->aspect = old->aspect;
    new->pix_aspect = old->pix_aspect;
    new->resolution = old->resolution;
    new->m = old->m;
    new->rot = old->rot;
    new->background = old->background;
    return (Object)new;
}


Camera
DXSetView(Camera c, Point from, Point to, Vector up)
{
    CHECK(c, CLASS_CAMERA);

    c->from = from;
    c->to = to;
    c->up = up;
    return matrix(c);
}

Camera
DXSetPerspective(Camera c, double fov, double aspect)
{
    CHECK(c, CLASS_CAMERA);

    c->width = fov;
    c->aspect = aspect;
    c->ortho = 0;
    return matrix(c);
}

Camera
DXSetOrthographic(Camera c, double width, double aspect)
{
    CHECK(c, CLASS_CAMERA);

    c->width = width;
    c->aspect = aspect;
    c->ortho = 1;
    return matrix(c);
}

Camera
DXSetResolution(Camera c, int hres, double pix_aspect)
{
    CHECK(c, CLASS_CAMERA);

    c->resolution = hres;
    c->pix_aspect = pix_aspect;
    return matrix(c);
}

/*
 * Access routines
 */

Matrix
DXGetCameraMatrix(Camera c)
{
    return c? c->m : Identity;
}


Matrix
DXGetCameraMatrixWithFuzz(Camera c, float fuzz)
{
    Matrix m;
    if (!c) {
	m = Identity;
	m.b[2] = fuzz;
    } else {
	m = c->m;
	if (c->ortho) {
	    m = c->m;
	    m.b[2] += fuzz*c->width/c->resolution;
	} else {
	    float f = 1.0 - fuzz*c->width/c->resolution;
	    m.A[0][0] *= f;  m.A[0][1] *= f;  m.A[0][2] *= f;
	    m.A[1][0] *= f;  m.A[1][1] *= f;  m.A[1][2] *= f;
	    m.A[2][0] *= f;  m.A[2][1] *= f;  m.A[2][2] *= f;
	    m.b[0] *= f;     m.b[1] *= f;     m.b[2] *= f;
	}
    }
    return m;
}

Matrix
DXGetCameraRotation(Camera c)
{
    return c? c->rot : Identity;
}


Camera
DXGetCameraResolution(Camera c, int *width, int *height)
{
    CHECK(c, CLASS_CAMERA);

    if (width)
	*width = c->resolution;
    if (height)
	*height = c->resolution*c->aspect;

    return c;
}

Camera
DXGetView(Camera c, Point *from, Point *to, Vector *up)
{
    CHECK(c, CLASS_CAMERA);
    if (from)
	*from = c->from;
    if (to)
	*to = c->to;
    if (up)
	*up = c->up;
    return c;
}

Camera
DXGetPerspective(Camera c, float *fov, float *aspect)
{
    CHECK(c, CLASS_CAMERA);

    if (c->ortho)
	return NULL;
    if (fov)
	*fov = c->width;
    if (aspect)
	*aspect = c->aspect;

    return c;
}

Camera
DXGetOrthographic(Camera c, float *width, float *aspect)
{
    CHECK(c, CLASS_CAMERA);

    if (!c->ortho)
	return NULL;
    if (width)
	*width = c->width;
    if (aspect)
	*aspect = c->aspect;

    return c;
}

Camera
DXSetBackgroundColor(Camera c, RGBColor b)
{
    CHECK(c, CLASS_CAMERA);

    c->background = b;

    return c;
}

Camera
DXGetBackgroundColor(Camera c, RGBColor *b)
{
    CHECK(c, CLASS_CAMERA);

    *b = c->background;

    return c;
}
