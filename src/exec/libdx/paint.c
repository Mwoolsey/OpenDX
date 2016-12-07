/***********************************************************************/ 
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


/*
 * Header: /usr/people/gresh/code/svs/src/libdx/RCS/paint.c,v 5.0 92/11/12 09:08:25 svs Exp Locker: gresh 
 * Locker: gresh 
 */

#include <string.h>
#include <dx/dx.h>
#include "internals.h"
#include "render.h"


/*
 * Parameters
 */

#define PARAMETER(parm, name, type, get) {				    \
    type p;								    \
    Object a;								    \
    a = DXGetAttribute(o, name);						    \
    if (a) {								    \
        if (!get(a, &p))						    \
	    DXErrorReturn(ERROR_BAD_PARAMETER, "invalid " name " attribute"); \
        new->parm = p;							    \
    }									    \
}

static Error
parameters(Object o, struct tile *new, struct tile *old, int *if_object)
{
    *new = *old;
    *if_object = 0;
    if (DXGetAttribute(o, "interference object")) {
	new->if_object = 1;
	if (!old->if_object) *if_object = 1;
    }
    if (DXGetAttribute(o, "interference group"))
	new->if_group = 1;
    if (!_dxf_approx(o, &new->approx))
	return ERROR;

    return OK;
}



Object
_dxfField_Paint(Field f, struct buffer *b, int clip_status, struct tile *tile)
{
    int n;
    struct xfield xf;
    int if_object = 0;

    /* get the stuff we need */
    DXMarkTimeLocal("start field");
    if (DXEmptyField(f))
	return (Object) f;
#if 0
    if (!_dxf_XBox(f, &xf, XR_REQUIRED, XD_GLOBAL)) goto error;
    if (xf.box[0].x>=b->max.x || xf.box[0].y>=b->max.y ||
      xf.box[7].x<b->min.x || xf.box[7].y<b->min.y) {
	DXMarkTimeLocal("culled");
	return (Object) f;
    }
#endif

    /* store parameters and get new ones */
    _dxf_XZero(&xf);
    if (!parameters((Object)f, &xf.tile, tile, &if_object)) goto error;
    if (xf.tile.if_group && !xf.tile.if_object) {
	xf.tile.if_object = 1;
	if_object = 1;
    }
    DXMarkTimeLocal("parameters");

    /* extract what we need */
    if (clip_status==CLIPPING) {
	if (xf.tile.approx!=approx_dots) {
	    if (!_dxf_XConnections(f, &xf, XR_OPTIONAL)) goto error;
	    if (!_dxf_XPolylines(f, &xf, XR_OPTIONAL, XD_GLOBAL)) goto error;
	}
    } else {
	if (!_dxf_XColors(f, &xf, XR_REQUIRED, XD_GLOBAL)) goto error;
	if (xf.tile.approx!=approx_dots) {
	    if (!_dxf_XOpacities(f, &xf, XR_OPTIONAL, XD_GLOBAL)) goto error;
	    if (!_dxf_XConnections(f, &xf, XR_OPTIONAL)) goto error;
	    if (!_dxf_XPolylines(f, &xf, XR_OPTIONAL, XD_GLOBAL)) goto error;
	    if (xf.ct!=ct_triangles && xf.ct!=ct_quads && xf.ct != ct_polylines
	      && xf.ct!=ct_lines && xf.ct!=ct_none)
		return (Object) f;
	}
    }
    DXMarkTimeLocal("extract");
    if (!_dxf_XPositions(f, &xf, XR_REQUIRED, XD_LOCAL)) goto error;
    n = xf.nconnections;
    DXMarkTimeLocal("localize");

    /* get box for near plane for perspective clipping */
    if (xf.tile.perspective) {
	if (!_dxf_XBox(f, &xf, XR_REQUIRED, XD_GLOBAL)) goto error;
	xf.nearPlane = xf.box[0].z;
    }

    if (! _dxf_XLighting(f, &xf))
	goto error;

    /* for now */
#   define EXPAND_QUADS \
    xf.c.quads = (Quadrilateral *) DXGetArrayData(xf.connections_array);	\
    if (!xf.c.quads)							\
	goto error;							\
    DXGetArrayInfo(xf.connections_array, &n, NULL, NULL, NULL, NULL);


    /* if we're a clipping surface */
    if (clip_status==CLIPPING) {
	if (xf.ct==ct_triangles) {
	    if (!_dxf_TriangleClipping(b, &xf, n, xf.c.triangles, NULL))
		goto error;
	} else if (xf.ct==ct_quads) {
	    EXPAND_QUADS;
	    if (!_dxf_QuadClipping(b, &xf, n, xf.c.quads, NULL))
		goto error;
	} else if (xf.ct!=ct_none)
	    DXErrorGoto(ERROR_DATA_INVALID,
			"clipping object must be triangles or quads");

    /* if we're an opaque surface */
    } else if (!xf.opacities) {

	/* render triangles */
	if (xf.ct==ct_triangles) {
	    if (!_dxf_XInvalidConnections(f, &xf)) goto error;
	    if (xf.colors_dep == dep_positions || xf.lights) {
		if (!_dxf_Triangle(b, &xf, n, xf.c.triangles, NULL,
					0, clip_status, INV_UNKNOWN))
		    goto error;
	    } else if (xf.colors_dep == dep_connections) {
		if (!_dxf_TriangleFlat(b, &xf, n, xf.c.triangles, NULL,
				   xf.fcolors, xf.bcolors, xf.opacities,
				   0, clip_status, INV_UNKNOWN))
		    goto error;
	    }
	}

	/* render quads */
	else if (xf.ct==ct_quads) {
	    EXPAND_QUADS;
	    if (!_dxf_XInvalidConnections(f, &xf)) goto error;
	    if (xf.colors_dep == dep_positions || xf.lights) {
		if (!_dxf_Quad(b, &xf, n, xf.c.quads, NULL, 0,  
					clip_status, INV_UNKNOWN))
			goto error;
	    } else if (xf.colors_dep == dep_connections) {
		if (!_dxf_QuadFlat(b, &xf, n, xf.c.quads, NULL,
			       xf.fcolors, xf.bcolors, xf.opacities,
			       1, clip_status, INV_UNKNOWN))
		    goto error;
	    }
	}

	/* render lines */
	else if (xf.ct==ct_lines) {
	    if (!_dxf_XInvalidConnections(f, &xf)) goto error;
	    if (xf.colors_dep == dep_positions || xf.lights) {
		if (!_dxf_Line(b, &xf, xf.nconnections, xf.c.lines, NULL,
					clip_status, INV_UNKNOWN))
		    goto error;
	    } else if (xf.colors_dep == dep_connections) {
		if (!_dxf_LineFlat(b, &xf, xf.nconnections, xf.c.lines, NULL,
			       xf.fcolors, xf.opacities,
					clip_status, INV_UNKNOWN))
		    goto error;
	    } else
		DXErrorGoto(ERROR_DATA_INVALID, "invalid color dependency");
	}

	else if (xf.ct==ct_polylines) {
	    if (!_dxf_XInvalidPolylines(f, &xf)) goto error;
	    if (xf.colors_dep == dep_positions || xf.lights) {
		if (!_dxf_Polyline(b, &xf, xf.nconnections, xf.polylines, NULL,
					clip_status, INV_UNKNOWN))
		    goto error;
	    } else if (xf.colors_dep == dep_polylines) {
		if (!_dxf_PolylineFlat(b, &xf, xf.nconnections, xf.polylines,
				NULL, xf.fcolors, xf.opacities,
				clip_status, INV_UNKNOWN))
		    goto error;
	    } else
		DXErrorGoto(ERROR_DATA_INVALID, "invalid color dependency");
	}

	/*
	 * Note: we do the loops in reverse order so that the first
         * loop gets painted last.  This ensures that the first loop
         * determines whether front or back colors are used, on the
         * assumption that the first loop is the "main" loop.
         * XXX - find a better solution.  Also, if this is the case,
         * the colors need not be interpolated in any but the first
         * loop.
         */

	else if (xf.ct==ct_none) {
	    if (!_dxf_XInvalidPositions(f, &xf)) goto error;
	    if (xf.colors_dep==dep_positions) {
		if (!_dxf_Points(b, &xf))
		    goto error;
	    } else
		DXErrorGoto(ERROR_DATA_INVALID, "invalid color dependency");
	}

	/* eh? */
	else
	    DXErrorGoto(ERROR_DATA_INVALID, "unknown connections type");
    }

    if (if_object)
	_dxf_InterferenceObject(b);

    _dxf_XFreeLocal(&xf);
    DXMarkTimeLocal("work");
    return (Object) f;

error:
    _dxf_XFreeLocal(&xf);
    return NULL;
}


/*
 * DXRender the opaque "triangles" of a group
 */

Object
_dxfGroup_Paint(Group g, struct buffer *b, int clip_status, struct tile *tile)
{
    Object o;
    int i, if_object;
    struct tile new;
    float minx, miny, maxx, maxy;

    minx = b->min.x - .5 /* for lines */;
    miny = b->min.y - .5 /* for lines */;
    maxx = b->max.x + .5 /* for lines */;
    maxy = b->max.y + .5 /* for lines */;

    if (!parameters((Object)g, &new, tile, &if_object)) return NULL;
    if (DXGetObjectClass((Object)g)==CLASS_COMPOSITEFIELD)
	new.ignore = 1;

    for (i=0; (o=DXGetEnumeratedMember(g, i, NULL)); i++) {
	if (DXGetObjectClass(o)==CLASS_FIELD) {
	    Array a;
	    Point *box;
	    a = (Array)DXGetComponentValue((Field)o, BOX);
	    box = (Point *) DXGetArrayData(a);
	    if (box[0].x>=maxx || box[0].y>=maxy ||
		box[7].x<minx || box[7].y<miny)
		continue;
	    if (!_dxfField_Paint((Field)o, b, clip_status, &new))
		return NULL;
	} else
	    if (!_dxfPaint(o, b, clip_status, &new))
		return NULL;
    }

    if (if_object)
	_dxf_InterferenceObject(b);

    return (Object) g;
}


/*
 * Nothing to paint in a light
 * XXX - if Shade() removed lights, this would not be necessary
 */

Object
_dxfLight_Paint(Light l, struct buffer *b, int clip_status)
{
    return (Object) l;
}


/*
 * The xform node was not removed so we could
 * preserve rendering parameters.
 * XXX - check that matrix is identity?
 */

Object
_dxfXform_Paint(Xform x, struct buffer *b, int clip_status, struct tile *tile)
{
    Object o;
    struct tile new;
    int if_object;

    if (!parameters((Object)x, &new, tile, &if_object)) return NULL;
    if (!DXGetXformInfo(x, &o, NULL))
	return NULL;
    o = _dxfPaint(o, b, clip_status, &new);
    if (if_object)
	_dxf_InterferenceObject(b);
    return o;
}


Object
_dxfScreen_Paint(Screen s, struct buffer *b, int clip_status, struct tile *tile)
{
    Object o;
    struct tile new;
    int if_object;

    if (!parameters((Object)s, &new, tile, &if_object)) return NULL;
    new.perspective = 0;
    if (!DXGetScreenInfo(s, &o, NULL, NULL))
	return NULL;
    o = _dxfPaint(o, b, UNCLIPPED, &new);
    if (if_object)
	_dxf_InterferenceObject(b);
    return o;
}


Object
_dxfClipped_Paint(Clipped clipped, struct buffer *b, int clip_status,
	       struct tile *tile)
{
    Object render, clipping;
    struct tile new;
    int if_object;

    DXGetClippedInfo(clipped, &render, &clipping);

    if (!parameters((Object)clipped, &new, tile, &if_object)) return NULL;

    if (CLIPPED(clip_status)) {
	DXErrorReturn(ERROR_NOT_IMPLEMENTED,
		    "Nested Clipped objects not supported");
    } else if (clip_status==CLIPPING) {
	DXErrorReturn(ERROR_BAD_PARAMETER,
		    "Clipped objects not allowed in clipping object");
    }

    if (new.approx==approx_none) {
	_dxf_BeginClipping(b);
	if (!_dxfPaint(clipping, b, CLIPPING, &new))
	    return NULL;
	if (tile->perspective)
	    if (! _dxfCapClipping(b, DXD_MAX_FLOAT))
		return NULL;
	if (!_dxfPaint(render, b, NEST(clip_status), &new))
	    return NULL;
	_dxf_EndClipping(b);
    } else {
	if (!_dxfPaint(render, b, clip_status, &new))
	    return NULL;
    }

    if (if_object)
	_dxf_InterferenceObject(b);

    return (Object) clipped;
}


