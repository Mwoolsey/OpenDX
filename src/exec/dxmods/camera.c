/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>



#include <string.h>
#include <dx/dx.h>
#include "bounds.h"
#include <math.h>
#include <ctype.h>

#define AMIN(a, b)  ((fabs(a))>(fabs(b)) ? (b) : (a))     
#define AMAX(a, b)  ((fabs(a))<(fabs(b)) ? (b) : (a))     

#define DEG2RAD(x)      (x/180.0*M_PI)   /* degrees to radians */
#define RAD2DEG(x)      (x/M_PI*180.0)   /* u guess */

#define IsEmpty(o)      ((DXGetObjectClass(o) == CLASS_FIELD) && \
			 (DXEmptyField((Field)o)))

#define FUZZ ((float) 1e-37) /* for floating point number compares - this is
			      *  not MINFLOAT as defined in <values.h>, 
			      *  because it is 1e-45, which is denormalized 
			      *  and will cause the i860 chip to trap.
	                      */

#define TOOBIG ((float) 1e37)  /* similiar idea for large values.  this is
				*  smaller than DXD_MAX_FLOAT because it is a test
				*  for something which will subsequently be
				*  multiplied and divided further and there
				*  needs to still be a little headroom.
				*/

/* directions the autocamera can point */
#define D_UNDEFINED	0x00
#define D_OFF        	0x80
#define D_RIGHT		0x01
#define D_OFFRIGHT	0x81
#define D_LEFT		0x02
#define D_OFFLEFT	0x82
#define D_TOP		0x03
#define D_OFFTOP	0x83
#define D_BOTTOM	0x04
#define D_OFFBOTTOM	0x84
#define D_DIAGONAL	0x05
#define D_OFFDIAGONAL	0x85
#define D_FRONT		0x06
#define D_OFFFRONT	0x86
#define D_BACK		0x07
#define D_OFFBACK	0x87

#define MAXDIR          16    /* longest acceptable direction string + 1 */


struct cameraparms {
    int isauto;         /* camera or autocamera */
    int isupdate;       /* updating an existing camera */
    int isobject;	/* first parm is object */
    Point bbox[8];	/* if object, bounding box */
    int nobox;          /* if set, object with no bbox */
    Point t;		/* to point (look-at point) */
    Point f;		/* from point or from direction vector */
    float width;	/* viewport width in user's units (ortho only) */
    int hsize;		/* horizontal width of image size in pixels */
    float aspect;	/* aspect ratio of image (height/width) */
    Vector u;		/* up vector */
    int upset;		/* set if up is specified by user */
    int doperspective;	/* see flags below */
    float angle;	/* perspective: view angle */
    float fov;          /* resulting field of view */
    RGBColor color;	/* background color */
};

/* doperspective flags */
#define C_ORTHOGRAPHIC 0	/* orthographic projection */
#define C_PERSP_ANGLE  1	/* perspective, specifying view angle */
#define C_PERSP_DIST   2	/* perspective, distance along view vector */
#define C_PERSP_35MM   3	/* perspective, 35mm camera lens */

/* default perspective angle */
#if 0
#define P_ANGLE 57.9
#define P_FOV   1.106338   /* 2 * tan(57.9/2) */
#else
#define P_ANGLE 30.0
#define P_FOV   0.53590    /* 2 * tan(30.0/2) */
#endif

/* prototypes */
static Camera single_camera(Object *in, int isauto, int isupdate);
static Camera make_cam(struct cameraparms *p);
static void default_camera(struct cameraparms *p, int isauto);
static Error existing_camera(struct cameraparms *p, Camera c, int isauto);
static Error parse_inputs(Object *in, struct cameraparms *p);
static Error good_camera(struct cameraparms *p);
static Error first_parms(Object *in, struct cameraparms *p);
static Error second_parms(Object *in, struct cameraparms *p);
static Error third_parms(Object *in, struct cameraparms *p);
static int find_direction(char *);
static Error set_direction(char *, struct cameraparms *p);

extern Error DXColorNameToRGB(char *, RGBColor *); /* from libdx/color.c */


/* external module entry points 
 */
int m_AutoCamera(Object *in, Object *out)
{
    out[0] = (Object)single_camera(in, 1, 0);
    return (out[0] ? OK : ERROR);
}

int m_Camera(Object *in, Object *out)
{
    out[0] = (Object)single_camera(in, 0, 0);
    return (out[0] ? OK : ERROR);
}

int m_UpdateCamera(Object *in, Object *out)
{
    out[0] = (Object)single_camera(in, 0, 1);
    return (out[0] ? OK : ERROR);
}


/* internal routines
 */
static Camera single_camera(Object *in, int isauto, int isupdate)
{
    struct cameraparms p;

    /* initialize the cameraparms structure */
    if (isupdate) {
	if (!existing_camera(&p, (Camera)in[0], isauto))
	    return NULL;
	in++;              /* space past initial camera */
    } else 
	default_camera(&p, isauto);

    /* compute camera parms from inputs */
    if (!parse_inputs(in, &p))
	return NULL;

    /* error checks */
    if (!good_camera(&p))
	return NULL;

    /* generate camera object and return */
    return make_cam(&p);

}


/*
 */
static void default_camera(struct cameraparms *p, int isauto)
{
    /* zero everything */
    memset((char *)p, '\0', sizeof(struct cameraparms));

    /* now set things which shouldn't default to zero */
    p->isauto = isauto;
    p->f = DXPt(0.0, 0.0, 1.0);
    p->width = 100.0;
    p->hsize = 640;
    p->aspect = 0.75;
    p->u = DXPt(0.0, 1.0, 0.0);
    p->angle = P_ANGLE;
    p->fov = P_FOV;
}

/*
 */
static Error existing_camera(struct cameraparms *p, Camera c, int isauto)
{
    /* zero everything */
    memset((char *)p, '\0', sizeof(struct cameraparms));

    if (!c || DXGetObjectClass((Object)c) != CLASS_CAMERA) {
	DXSetError(ERROR_DATA_INVALID, "#10660", "camera");
	return ERROR;
    }

    p->isauto = isauto;
    p->isupdate = 1;

    /* extract defaults from existing camera */
    DXGetView(c, &p->f, &p->t, &p->u);
    DXGetCameraResolution(c, &p->hsize, NULL);

    /* if initially an ortho camera, set the defaults for perspective
     * anyway in case they just toggle the perspective flag on without
     * setting the view angle.
     * if already a perspective camera, extract the existing fov and 
     * invert it back to an angle because that's how the user specifies
     * the fov - the code which comes later expects to have to do the
     * angle-to-fov conversion.  set width in case toggling back to ortho.
     */
    if (DXGetOrthographic(c, &p->width, &p->aspect)) {
	p->angle = P_ANGLE;
	p->fov = P_FOV;
    } else {
	DXGetPerspective(c, &p->fov, &p->aspect);
	p->angle = RAD2DEG(atan(p->fov / 2.)) * 2.;
	p->width = 2 * DXLength(DXSub(p->f, p->t)) * tan(DEG2RAD(p->angle));
	p->doperspective = 1;
    }

    DXGetBackgroundColor(c, &p->color);
    return OK;
}

/*
 */
static Error parse_inputs(Object *in, struct cameraparms *p)
{

    /* the simple parms: resolution, aspect, up vector and perspective flag,
     *  background color
     */
    if (!first_parms(in, p))
	return ERROR;

    /* the more complicated ones: to point, from vector
     */
    if (!second_parms(in, p))
	return ERROR;

    /* the most complicated ones: from point, width and angle
     */
    if (!third_parms(in, p))
	return ERROR;


    return OK;
}

/*
 */
static Error first_parms(Object *in, struct cameraparms *p)
{
    char *colorname;

    /* resolution (final image width in pixels) */
    if (in[3] && (!DXExtractInteger(in[3], &p->hsize) || p->hsize <= 0)) {
	DXSetError(ERROR_BAD_PARAMETER, "#10020", "resolution");
	return ERROR;
    }
    
    
    /* aspect ratio = horizontal pixels / vertical pixels */
    if (in[4] && (!DXExtractFloat(in[4], &p->aspect) || p->aspect <= 0.0)) {
	DXSetError(ERROR_BAD_PARAMETER, "#10090", "aspect");
	return ERROR;
    }
    
    
    /* up vector */
    if (in[5]) {
        if (!DXExtractParameter(in[5], TYPE_FLOAT, 3, 1, (Pointer)&p->u)) {
	    DXSetError(ERROR_BAD_PARAMETER, "#10230", "up", 3);
	    return ERROR;
	}
        if (DXLength(p->u) < FUZZ) {
	    DXSetError(ERROR_BAD_PARAMETER, "#11822", "up");
	    return ERROR;
	}
	p->upset++;
    }
    
    
    /* perspective flag */
    if (in[6]) {
	if (!DXExtractInteger(in[6], &p->doperspective)) {
	    DXSetError(ERROR_BAD_PARAMETER, "#10020", "perspective");
	    return ERROR;
	}
	if (p->doperspective < 0 || p->doperspective > 1) {
	    DXSetError(ERROR_BAD_PARAMETER, "#10040", "perspective", 0, 1);
	    return ERROR;
	}
    }

    /* background color */
    if (in[8]) {
	/* color name */
	if (DXExtractString(in[8], &colorname)) {
	    if (!DXColorNameToRGB(colorname, &p->color)) {
		return ERROR;
	    }
	} 

	/* R,G,B vector */
	else if (DXExtractParameter(in[8], TYPE_FLOAT, 3, 1, (Pointer)&p->color))
		;

	/* error */
	else {
	    DXSetError(ERROR_BAD_PARAMETER, "#10510", "background");
	    return ERROR;
	}
    }


    return OK;
}


/*
 */
static Error second_parms(Object *in, struct cameraparms *p)
{
    char *direction;
    Point bbox2[8];

    /* to: object or explicit point 
     *  can default for Camera, required for AutoCamera
     */

    if (in[0]) {

	/* explicit point */
	if (DXExtractParameter(in[0], TYPE_FLOAT, 3, 1, (Pointer)&p->t))
	    ;
	
	/* object */
	else if (DXBoundingBox(in[0], p->bbox)) {
	    p->t.x = (p->bbox[7].x + p->bbox[0].x) / 2;
	    p->t.y = (p->bbox[7].y + p->bbox[0].y) / 2;
	    p->t.z = (p->bbox[7].z + p->bbox[0].z) / 2;
	    p->isobject++;
	}
	
	/* if the input is a genuine empty field, use the defaults
	 *  without a warning.
	 */
	else if (IsEmpty(in[0])) {
	    DXResetError();
            p->nobox++;

	/* anything else gets a warning */
	} else {
	    DXResetError();
	    DXWarning("#4020");
	    p->nobox++;
	} 

    }

    /* autocamera can't default the to-point */
    if (!in[0] && p->isauto && !p->isupdate) {
	DXSetError(ERROR_BAD_PARAMETER, "#10000", "object");
	return ERROR;
    } 
    
    
    /* from: vector or string (AutoCamera)
     *       vector or object (Camera)
     */

    if (in[1]) {
	
	/* vector/point */
	if (DXExtractParameter(in[1], TYPE_FLOAT, 3, 1, (Pointer)&p->f))
	    ;

	/* string */
	else if (DXExtractString(in[1], &direction)) {
	    if (p->isauto) {
		if (!set_direction(direction, p))
		    return ERROR;
	    } else {
		DXSetError(ERROR_BAD_PARAMETER, "#10460", "from");
		return ERROR;
	    }
	}

	/* object */
	else if (DXBoundingBox(in[1], bbox2)) {
	    if (p->isauto) {
		DXSetError(ERROR_BAD_PARAMETER, "#10510", "`direction'");
		return ERROR;
	    }
	    p->f.x = (bbox2[7].x + bbox2[0].x) / 2;
	    p->f.y = (bbox2[7].y + bbox2[0].y) / 2;
	    p->f.z = (bbox2[7].z + bbox2[0].z) / 2;
	}

	/* error */
	else {
	    if (p->isauto)
		DXSetError(ERROR_BAD_PARAMETER, "#10510", "`direction'");
	    else
		DXSetError(ERROR_BAD_PARAMETER, "#10460", "from");
	    return ERROR;
	}

	/* check for from == to */
	if (p->isauto && DXLength(p->f) < FUZZ) {
	    DXSetError(ERROR_BAD_PARAMETER, "#11822", "direction");
	    return ERROR;
	}
	
	if (!p->isauto && DXLength(DXSub(p->f, p->t)) < FUZZ) {
	    DXSetError(ERROR_BAD_PARAMETER, "#11010", "`from'", "`to'");
	    return ERROR;
	}

    } 

    /* for autocamera, default to front;  for camera, from is already set */
    else if (p->isauto && !p->isupdate) {
	if (!set_direction("front", p))
	    return ERROR;
    }
    
    return OK;
}


/*
 */
static Error third_parms(Object *in, struct cameraparms *p)
{
    float diag = 0.0;

    /* width: scalar or object for both camera and autocamera.
     */
    if (in[2]) {
	if (!p->isauto && p->doperspective) {
	    DXWarning("width ignored for perspective camera");

	} else {

	    /* explicit width */
	    if (DXExtractFloat(in[2], &p->width)) {
		if (p->width <= 0) {
		    DXSetError(ERROR_BAD_PARAMETER, "#10090", "width");
		    return ERROR;
		} 
	    }
	    /* object */
	    else if(_dxf_BBoxDistance(in[2], &diag, BB_DIAGONAL)) {
		
		if (diag == 0)      /* object is single point */
		    p->width = 1.0;
		else
		    p->width = (p->aspect >= 1 ? diag : diag / p->aspect) 
			* 1.01;
		
	    }
	    /* if empty field,
             * or object with no bounding box and first object had no bbox,
             * then use default width
             */
	    else if (IsEmpty(in[2]) || p->nobox)
		DXResetError();

	    /* error */
	    else {
		DXSetError(ERROR_BAD_PARAMETER, "#10560", "width");
		return ERROR;
	    }
	}
    } 
    /* camera default already set; autocamera can default if first parm
     *  is an object.
     */
    else if (p->isauto && !p->isupdate) {
	if (!p->isobject && !p->nobox) {
	    DXSetError(ERROR_BAD_PARAMETER, "#10470", "width", "object");
	    return ERROR;
	}

	diag = DXLength(DXSub(p->bbox[7], p->bbox[0]));
	if (diag == 0)
	    p->width = 1.0;
	else
	    p->width = (p->aspect >= 1 ? diag : diag / p->aspect) * 1.01;
    }



    /* if perspective flag is set, compute angle/distance */

    switch (p->doperspective) {
      case 0:		/* ortho */
	break;
	
      case 1:		/* view angle */
	if (in[7]) {
	    if (!DXExtractFloat(in[7], &p->angle) 
		|| p->angle < 0.0 || p->angle >= 180.0) {
		DXSetError(ERROR_BAD_PARAMETER, "#10110", "angle", 0, 180);
		return ERROR;
	    }
	    /* allow user to turn off perspective with angle of zero */
	    if (p->angle < FUZZ)
		p->doperspective = 0;
	} 

	if (p->doperspective) {
	    p->fov = 2 * tan(DEG2RAD(p->angle/2));
	    if (p->fov < FUZZ)
		p->doperspective = 0;
	}
	break;
	
    }


    /* for autocamera, adjust the 'from' point to be a reasonable distance
     *  along the given direction so it works for either ortho or perspective.
     *  (the actual location of 'from' doesn't matter for ortho, and this
     *  makes it easier on the Image macro under some circumstances.)
     */
    if (p->isauto) {

	p->f = DXNormalize(p->f);

	/* turn off perspective if the field of view is too small and
         *  would cause fp overflows in the normalize/multiply step.
	 */
	if (p->fov < FUZZ || ((double)p->width/p->fov) > TOOBIG) {
	    p->doperspective = 0;

	    /* if the length of the direction vector is too small compared
             *  to the value of the to point, it will get lost when they
	     *  are added, and it will look like to and from are the same.
	     *  scale the length of the from vector so it is far enough away.
	     */
	    if (DXLength(p->t) > 1.0)
		p->f = DXMul(p->f, DXLength(p->t));
	} else
	    /* adjust the from point so it is at the right distance to match
	     * the angle and viewport width relative to the 'to' point.
	     */
	    p->f = DXMul(p->f, p->width / p->fov);

	/* autocamera 'from' is relative to the 'to' point.  the camera
         *  calls want absolute points, so adjust 'from' here.
         */
	p->f = DXAdd(p->f, p->t);
    }

    return OK;
}

#define SIN05  0.087
#define COS05  0.996
#define SIN15  0.259
#define COS15  0.966
#define SIN25  0.423
#define COS25  0.906
#define SIN35  0.574
#define SIN45  0.707

/*
 */
static Error set_direction(char *direction, struct cameraparms *p)
{
    int direct;

    if ((direct = find_direction(direction)) == D_UNDEFINED) {
	DXSetError(ERROR_BAD_PARAMETER, "#10480", "direction");
	return ERROR;
    }
    
    switch(direct) {
      case D_FRONT:     /* deltas: x = 0, y = 0, z = ++ */
	p->f = DXPt(0., 0., 1.);
	break;
	
      case D_OFFFRONT:  /* deltas: x = +, y = +, z = ++ */
	p->f = DXPt(SIN15, SIN15, COS15);
	break;
	
      case D_BACK:     /* deltas: x = 0, y = 0, z = -- */
	p->f = DXPt(0., 0., -1.);
	break;
	
      case D_OFFBACK:  /* deltas: x = +, y = +, z = -- */
	p->f = DXPt(SIN15, SIN15, -COS15);
	break;
	
      case D_TOP:      /* deltas: x = 0, y = ++, z = 0; up = -z */
	p->f = DXPt(0., 1., 0.);
	if (!p->upset)
	    p->u = DXVec(0., 0., -1.);
	break;
	
      case D_OFFTOP:   /* deltas: x = -, y = ++, z = - */
	p->f = DXPt(-SIN15, COS15, -SIN15);
	if (!p->upset)
	    p->u = DXVec(0., 0., -1.);
	break;
	
      case D_BOTTOM:   /* deltas: x = 0, y = --, z = 0; up = +z */
	p->f = DXPt(0., -COS15, 0.);
	if (!p->upset)
	    p->u = DXVec(0., 0., 1.);
	break;
	
      case D_OFFBOTTOM:   /* deltas: x = +, y = --, z = +; up = +z */
	p->f = DXPt(SIN15, -COS15, SIN15);
	if (!p->upset)
	    p->u = DXVec(0., 0., 1.);
	break;
	
      case D_RIGHT:       /* deltas: x = ++, y = 0, z = 0 */
	p->f = DXPt(1., 0., 0.);
	break;
	
      case D_OFFRIGHT:    /* deltas: x = ++, y = +, z = + */
	p->f = DXPt(COS05, SIN15, SIN15);
	break;
	
      case D_LEFT:        /* deltas: x = --, y = 0, z = 0 */
	p->f = DXPt(-1., 0., 0.);
	break;
	
      case D_OFFLEFT:     /* deltas: x = --, y = +, z = + */
	p->f = DXPt(-COS05, SIN15, SIN15);
	break;
	
      case D_DIAGONAL:    /* deltas: x = +, y = +, z = + */
	p->f = DXPt(SIN45, SIN45, SIN45);
	break;
	
      case D_OFFDIAGONAL: /* deltas: x = ++, y = ++, z = +++ */
	p->f = DXPt(SIN35, SIN35, SIN45);
	break;
	
      default:
	DXSetError(ERROR_BAD_PARAMETER, "#10480", "direction");
	return ERROR;
    }

	
    return OK;
}




/*
 */
static Error good_camera(struct cameraparms *p)
{
    Vector x;

    /* try to catch bad cameras */

    /* make sure from != to */

    if (fabs(DXLength(DXSub(p->f, p->t))) < FUZZ) {
	if (p->isauto)
	    DXSetError(ERROR_BAD_PARAMETER, "#11822", "direction");
	else
	    DXSetError(ERROR_BAD_PARAMETER, "#11010", "to", "from");
	return ERROR;
    }


    /* is there a bad up vector? */
    
    x = DXCross(p->u, DXSub(p->f, p->t));
    if (fabs(DXLength(x)) < FUZZ) {
	DXWarning("up vector in line with viewpoint.");
	p->u.z += 0.01;
	x = DXCross(p->u, DXSub(p->f, p->t));
	if (fabs(DXLength(x)) < FUZZ) {
	    p->u.z -= 0.01;
	    p->u.x += 0.01;
	    DXWarning("up.X changed to %f", p->u.x);
	} else
	    DXWarning("up.Z changed to %f", p->u.z);
    }

    return OK;
}


/* libdx calls to actually build the camera.
 */
static Camera make_cam(struct cameraparms *p)
{
    Camera cam = NULL;

    if (!(cam = DXNewCamera()))
	return NULL;
    
    if (p->doperspective) {
        if (!DXSetPerspective(cam, p->fov, p->aspect))
            goto error;
    } else {
        if (!DXSetOrthographic(cam, p->width, p->aspect))
            goto error;
    }

    if (!DXSetResolution(cam, p->hsize, 1.))
	goto error;
    
    if (!DXSetView(cam, p->f, p->t, p->u))
	goto error;

    if (!DXSetBackgroundColor(cam, p->color))
	goto error;

    return cam;

  error:
    DXDelete((Object)cam);
    return NULL;
}


/*
 * match a string with a direction.  case insensitive, and space,
 *  dot, dash and underscore all match the "off" directions.
 */
static int find_direction(char *inval)
{
    char dbuf[MAXDIR], *cp;
    int dir = D_UNDEFINED;

    /* make a copy and convert to lower case */
    strncpy(dbuf, inval, MAXDIR-1);
    dbuf[MAXDIR-1] = '\0';
    for (cp = dbuf; *cp; cp++)
	if (isupper(*cp))
	    *cp = tolower(*cp);

    cp = dbuf;

  again:
    switch(*cp) {
      case 'r':
	if (!strcmp(cp, "right"))
	    return (dir | D_RIGHT);
	break;

      case 'l':
	if (!strcmp(cp, "left"))
	    return (dir | D_LEFT);
	break;

      case 't':
	if (!strcmp(cp, "top"))
	    return (dir | D_TOP);
	break;

      case 'b':
	if (!strcmp(cp, "back"))
	    return (dir | D_BACK);
	if (!strcmp(cp, "bottom"))
	    return (dir | D_BOTTOM);
	break;

      case 'f':
	if (!strcmp(cp, "front"))
	    return (dir | D_FRONT);
	break;

      case 'd':
	if (!strcmp(cp, "diagonal"))
	    return (dir | D_DIAGONAL);
	break;

      case 'o':
	if (strncmp(cp, "off", 3))
	    return D_UNDEFINED;

	cp += 3;
	switch(*cp) { 
	    /* valid things to skip as separators */
	  case '-':
	  case '_':
	  case ' ':
	    cp++;

	    /* and fall thru to accept offdirection with no sep */

	  case 'r':
	  case 'l':
	  case 't':
	  case 'b':
	  case 'f':
	  case 'd':
	    dir = D_OFF;
	    goto again;

	  default:
	    break;
	}
	break;

      default:
	break;
    }
    
    return D_UNDEFINED;
}

