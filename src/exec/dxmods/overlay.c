/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dxmods/overlay.c,v 1.7 2006/01/03 17:02:23 davidt Exp $
 */

#include <dxconfig.h>


/***
MODULE:
    Overlay
SHORTDESCRIPTION:
    overlays one image on top of another using either blending or keying.
CATEGORY:
    Import and Export
INPUTS:
    overlay; image; NULL; overlay image 
    base;    image; NULL; base image 
    blend;   scalar or field or string or vector ; .5;  0 for base image only; 1 for overlay image only
OUTPUTS:
    combined; image; NULL; combined image
FLAGS:
BUGS:
AUTHOR: David Wood 
END:
***/

#include <dx/dx.h>
#include "_helper_jea.h"
#include  <math.h>		/* for fabs() */
#include <stdlib.h>

#if defined(HAVE_STRING_H)
#include <string.h>
#endif

#define MAX_SHAPE	64

/* 
 * Default allowable difference in color values when doing chromakeying.
 * This number is derived from visual inspection. 
 */
#define DELTA_COLOR	.001	

/* 
 * Determine whether two colors are the same 
 */
#define COLORMATCH(rgb, red,green,blue) \
	( (fabs((rgb)->r - (red)) <= DELTA_COLOR) && \
	  (fabs((rgb)->g - (green)) <= DELTA_COLOR) && \
	  (fabs((rgb)->b - (blue)) <= DELTA_COLOR) ) 

/* 
 * Supported operations. 
 */
#define  BLENDING 	0x1	/* Constant blend over images */
#define  MATTE		0x2	/* Per pixel blend over images */
#define  CHROMAKEY	0x3	/* Only overlay when base==color */

/*
 * Structure used to pass arguments to tasks.
 * If you add a new structure member, make sure it gets copied to
 * largs (as appropriate) in create_overlay_tasks().
 */
struct overlay_args {			/* Input args to overlay_task */
	Field base, overlay;		/* The 2 images to overlay */
	Field combined;			/* The output image to build */
	int	op;			/* How do we do the overlay */
	union arg3 {			/* 3rd argument to overlay */
		float	 blend;		/* Field blending coefficient */
		Field	 matte;		/* Blend matte */ 
		RGBColor chromakey;	/* Color to key on in overlay */
	} u;
};

typedef enum pixtype
{
    const_ubyte_rgb,
    const_float_rgb,
    const_mapped,
    ubyte_rgb,
    float_rgb,
    mapped
} Pixtype;

static Error blend_images(Pixtype, Pointer, RGBColor*,
			  Pixtype, Pointer, RGBColor*,
			  Pixtype, Pointer,
			  int, int, float);
static Error matte_images(Pixtype, Pointer, RGBColor*,
			  Pixtype, Pointer, RGBColor*,
			  Pixtype, Pointer,
			  int, int, float *);
static Error key_images(Pixtype, Pointer, RGBColor*,
			  Pixtype, Pointer, RGBColor*,
			  Pixtype, Pointer,
			  int, int, RGBColor *);
static Error valid_image_object(Object image);
static Error create_overlay_tasks(struct overlay_args *);
static Error overlay_task(Pointer);
static Error valid_blend_field(Field matte);
static Error compatible_grids(Field, Field, char *, char *);
static Error getImagePixelData(Field, Pixtype *, Pointer *, RGBColor **);

/*
 * Overlay two images using either of 2 methods depending on the type of 
 * the 3rd argument.  If the argument is of type...
 *
 *   scalar or Field ) 	Blend the two images such that
 *				o = base * (1-blend) + overlay * blend
 *		       	where 'blend' is
 *				scalar ) a float constant	
 *				field )  a field of float "data" constants 
 *					that map 1:1 onto both images.
 * 
 *   vector or string) specifies a color to use as the key when doing
 *			chromakeying, so that    
 *				if (base[i] == colorkey) 
 *					o[i] = overlay[i]
 *				else
 *					o[i] = base[i]
 *			This is comparable to creating a blend field that
 *			has value 1 where base[i] = colorkey and value 0
 *			elsewhere. 
 *
 * The default (when the 3rd argument is not present) is to overlay
 * the two images as if a single float blend  value of .5 had been
 * provided.
 *
 * Partitioned fields are handled properly.
 *
 * Currently, the input images (and the blend field) must have the
 * same positions.
 * 
 * 
 */
int
m_Overlay ( Object *in, Object *out )
{

#define  I_overlay	in[0]
#define  I_base		in[1]
#define  I_blend	in[2]


    Object	combined = NULL;
    char 	*colorstr;
    struct overlay_args args;

    if ( !I_overlay )
        ErrorGotoPlus1 ( ERROR_MISSING_DATA, "#10000", "'overlay'" );

    if ( !I_base )
        ErrorGotoPlus1 ( ERROR_MISSING_DATA, "#10000", "'base'" );

    if (!I_blend)   {
	args.u.blend = 0.5;	/* Default is to do a 50/50 blend */
	args.op = BLENDING;
    } 
    /*  If it's a string, then it is a color to key on */
    else if (DXExtractString(I_blend, &colorstr)) {
        /*  convert from string to DXRGB */
        if (!(DXColorNameToRGB(colorstr, &args.u.chromakey)))
            ErrorGotoPlus1( ERROR_DATA_INVALID, "#11760",colorstr );
        args.op = CHROMAKEY;
    }
    /*  If it's an DXRGB color */
    else  if (DXExtractParameter(I_blend,TYPE_FLOAT,3,1,
					(Pointer)&args.u.chromakey)) {
        args.op = CHROMAKEY;
    }
    /* See if it is a float blending value */
    else if (DXExtractFloat ( I_blend, &args.u.blend ) )  {
        if ( ( args.u.blend < 0.0 ) || ( args.u.blend > 1.0 ) ) 
            ErrorGotoPlus3( ERROR_DATA_INVALID, "#10110", "'blend'", 0, 1 );
        args.op = BLENDING;
    } 
    /* See if it is a matte/field */
    else if (valid_blend_field((Field)I_blend)) {
	args.u.matte = (Field)I_blend;
        args.op = MATTE;
    }
    /* Bad data type */
    else {
        ErrorGotoPlus1(ERROR_DATA_INVALID, "#10232","blend");
    }

    /*
     * Create the structure for the outgoing image and then build
     * a task group to do the overlay work. 
     */
    if ((combined = DXCopy(I_base, COPY_STRUCTURE))) {
    	DXCreateTaskGroup();
	args.base = (Field)I_base;
	args.overlay = (Field)I_overlay;
	args.combined = (Field)combined;
    	if (!create_overlay_tasks(&args)) {
	    DXAbortTaskGroup();
	    goto error;
	}
	else if (!DXExecuteTaskGroup())
	    goto error;
    } 

    out[0] = combined;
    return OK;

error:
    if (combined != I_base) 
	DXDelete(combined);
    return ERROR;
}


/* 
 * Create one task for each object with a class of CLASS_FIELD in the
 * base image (args->base).   Called recursively for CLASS_COMPOSITEFIELD
 * objects.
 */
static Error 
create_overlay_tasks(struct overlay_args *args)
{
    Object o, b;
    int i;
    struct overlay_args largs; 

    switch (DXGetObjectClass((Object)args->base)) {
	case CLASS_FIELD:
	    /* 
	     * Make sure the overlay image is compatible with the base. 
	     */
	    if (DXGetObjectClass((Object)args->overlay) != CLASS_FIELD) {
		ErrorGotoPlus2(ERROR_DATA_INVALID, "#12310", "base", "overlay");
	    }
	    else if (!compatible_grids(args->base, args->overlay, 
					"'base'","'overlay'")) {
		goto error;
	    } 
	    /* 
	     * Check to be sure the matte (if being used) is compatible. 
	     */
	    if (args->op == MATTE) { 
		if (DXGetObjectClass((Object)args->u.matte) != CLASS_FIELD) {
		    ErrorGotoPlus2(ERROR_DATA_INVALID,"#12315","blend","matte");
		}
	        else if (!compatible_grids(args->base, args->u.matte,
					"'base'","'blend'")) 
		    goto error;
	    }
	    /* 
	     * Queue up the work. 
	     */
	    if (!DXAddTask(overlay_task, (Pointer)args, sizeof(*args), 0.0))
		goto error;
	    break;
	case CLASS_GROUP:	/* Image must be a composite field */
	    /* 
	     * Make sure the input base image is luking good.
	     */
            if (DXGetGroupClass((Group)args->base) != CLASS_COMPOSITEFIELD)
	    	ErrorGotoPlus1(ERROR_DATA_INVALID, "#12320", "base");
	    /* 
	     * Check to be sure that the overlay is the same type. 
	     */
    	    if ((DXGetObjectClass((Object)args->overlay) != CLASS_GROUP) || 
                (DXGetGroupClass((Group)args->overlay) != CLASS_COMPOSITEFIELD))
	    	ErrorGotoPlus2(ERROR_DATA_INVALID, "#12315","overlay","image"); 
	    /* 
	     * Check to be sure the matte (if being used) is compatible. 
	     */
    	    if ((args->op == MATTE) &&
		((DXGetObjectClass((Object)args->u.matte) != CLASS_GROUP) || 
                 (DXGetGroupClass((Group)args->u.matte)!=CLASS_COMPOSITEFIELD)))
	    	ErrorGotoPlus2(ERROR_DATA_INVALID, "#12315", "blend","matte");
	    /* 
	     * Load largs with the constants over all tasks. 
	     */
	    largs.op = args->op;
	    if (largs.op == BLENDING)
		largs.u.blend = args->u.blend;
	    else if (largs.op == CHROMAKEY)
		largs.u.chromakey = args->u.chromakey;
	    /* 
	     * Create one task for each member of the inputs.
	     */
	    for (i=0; (b = DXGetEnumeratedMember((Group)args->base,i,NULL)) &&
	    	      (o = DXGetEnumeratedMember((Group)args->overlay,i,NULL)) ;
		 i++) {

		largs.overlay = (Field)o; 
		largs.base = (Field)b; 
		largs.combined = (Field)DXGetEnumeratedMember(
					(Group)args->combined,i,NULL); 
		if (largs.op == MATTE) {
		    largs.u.matte = (Field)DXGetEnumeratedMember(
					(Group)args->u.matte, i,NULL); 
		    if (!largs.u.matte)
			goto error;
		}
		if (!create_overlay_tasks(&largs)) 
		    goto error;
	    }
	    break;
	default:
	    ErrorGotoPlus1(ERROR_DATA_INVALID, "#12320", "base");
    }
    return OK;
error:
    return ERROR;
}

/*
 *  Overlay two images
 *	args->combined = foo(args->base, args->overlay) 
 *  The supplied images (args->[base,overlay,combined]) are all assumed to be
 *  simple images (i.e. not composite fields).
 */
static Error 
overlay_task(Pointer ptr)
{
    struct overlay_args *args = (struct overlay_args *)ptr;
    Field combined = args->combined;
    Field base = args->base;
    Field overlay = args->overlay;
    Array a = NULL, blend;
    Pointer  	c_ptr, o_ptr, b_ptr;
    float 	*m_ptr;
    RGBColor	*o_map = NULL, *b_map = NULL;
    Pixtype     o_type, b_type, c_type;
    Type	type;
    Category	cat;
    int       	height1, width1, height2, width2;
    int 	nitems, rank, shape[MAX_SHAPE]; 

    /*
     * Be sure that the base object is an image
     */ 
    if (!valid_image_object((Object)base))
	ErrorGotoPlus1 ( ERROR_DATA_INVALID, "#12320", "base" );

    /*
     * Be sure that the overlay object is an image
     */ 
    if (!valid_image_object((Object)overlay))
	ErrorGotoPlus1 ( ERROR_DATA_INVALID, "#12320", "overlay" );
    
    /*
     * Get overlay, base and output image info
     */
    if (! getImagePixelData(overlay, &o_type, &o_ptr, &o_map))
	goto error;
	
    if (! getImagePixelData(base, &b_type, &b_ptr, &b_map))
	goto error;
	
    /*
     * Get and compare sizes of base and overlay
     */
    if (!DXGetImageSize(overlay, &width1, &height1) ||
        !DXGetImageSize(base, &width2, &height2))
        DXErrorGoto( ERROR_DATA_INVALID, "#12325");

    /* 
     * Make sure the images are the same size 
     */
    if ( (height1 != height2) || (width1 != width2) ) { 
        DXSetError ( ERROR_DATA_INVALID, "#12330",
              height1, width1, height2, width2 );
	goto error;
    }

    /*
     * Create a new array of pixels to hold the combined image.
     */
    if (! getenv("DXPIXELTYPE") || !strcmp(getenv("DXPIXELTYPE"), "DXFloat"))
	a = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, 3);
    else
	a = DXNewArray(TYPE_UBYTE, CATEGORY_REAL, 1, 3);

    if (!a || !DXAddArrayData(a, 0, width1*height1, NULL))
	goto error;

    /* 
     * Install the new image (colors) in the output field 
     */
    if (!DXSetComponentValue(combined, "colors", (Object)a))
	goto error;

    if (! getImagePixelData(combined, &c_type, &c_ptr, NULL))
	goto error;
    
    /* 
     * Do the work to overlay the images 
     */
    if (args->op == BLENDING) {
	if (! blend_images(b_type, b_ptr, b_map,
		     o_type, o_ptr, o_map,
		     c_type, c_ptr,
		     height1, width1, args->u.blend)) goto error;
    } 
    else if (args->op == MATTE) {
	/*
	 * Make sure the field has the correct structure
	 */
        if (!valid_blend_field(args->u.matte))
	    ErrorGotoPlus1 ( ERROR_DATA_INVALID, "#12335", "blend" );

	/*
	 * DXExtract the array of blend values
	 */
	blend = (Array)DXGetComponentValue(args->u.matte,"data");
	if (!blend)
	    ErrorGotoPlus2 ( ERROR_DATA_INVALID, "#10250", "'blend'", "data");

	/*
	 * Check the number, type, category, rank and shape of data provided
	 */ 
	DXGetArrayInfo(blend, &nitems, &type, &cat, &rank, shape);
	if (nitems != (width1 * height1))
	    ErrorGotoPlus1 ( ERROR_DATA_INVALID, "#12340","blend" );
	if (type != TYPE_FLOAT) 
	    ErrorGotoPlus1 ( ERROR_DATA_INVALID, "#10330", "'blend' data" );
	if (cat != CATEGORY_REAL) 
	    ErrorGotoPlus1(ERROR_DATA_INVALID,"#11150","'blend' data"); 
	if ((rank != 0) && !(rank == 1 && shape[0] == 1)) 
	    ErrorGotoPlus1 ( ERROR_DATA_INVALID,  "#10081", "'blend' data");

	/*
	 * Get a pointer to the real float values
	 */
	m_ptr = (float*)DXGetArrayData(blend);
	if (!m_ptr)
	    ErrorGotoPlus2 ( ERROR_INTERNAL, "#10250", "'blend'","data" );

	if (! matte_images(b_type, b_ptr, b_map,
		     o_type, o_ptr, o_map,
		     c_type, c_ptr,
		     height1, width1, m_ptr)) goto error;
    } 
    else if (args->op == CHROMAKEY) {
	if (! key_images(b_type, b_ptr, b_map,
		   o_type, o_ptr, o_map,
		   c_type, c_ptr,
		   height1, width1, &args->u.chromakey)) goto error;
    } else 
	DXErrorGoto ( ERROR_UNEXPECTED, "bad operation" );


    /* Need to remove any "direct color map" attribute if present, 
     * since colors are expanded, this attribute does not make sense. 
     * Similarly, colors should not ref a colormap, 
     * and the color map component should be removed. DLG 4/30/97 
     */
    if (!DXSetAttribute((Object)combined,"direct color map", NULL))
       goto error;	
    if (!DXSetComponentAttribute((Field)combined,"colors","ref",NULL))
       goto error;
    if (!DXSetComponentValue((Field)combined,"color map",NULL))
       goto error;
    return OK;		

error:
    if (a) DXDelete((Object)a);
    return ERROR;
}

#define BYTE_TO_FLOAT(a) ((a)*(1.0/255.0))
#define FLOAT_TO_BYTE(a) ((a)*255.0)

#define GET_PIXEL(dst, tmp, type, u_p, f_p, map)		\
{								\
    if (type == ubyte_rgb)					\
    {								\
	tmp.r = BYTE_TO_FLOAT(*u_p++);				\
	tmp.g = BYTE_TO_FLOAT(*u_p++);				\
	tmp.b = BYTE_TO_FLOAT(*u_p++);				\
	dst = &tmp;						\
    }								\
    else if (type == float_rgb)					\
    {								\
	dst = f_p++;						\
    }								\
    else if (type == mapped)					\
    {								\
	dst = map + *u_p++;					\
    }								\
}

#define PUT_PIXEL(type, u_p, f_p, res)				\
{								\
    if (type == ubyte_rgb)					\
    {								\
	*u_p++ = FLOAT_TO_BYTE((res)->r);				\
	*u_p++ = FLOAT_TO_BYTE((res)->g);				\
	*u_p++ = FLOAT_TO_BYTE((res)->b);				\
    }								\
    else if (type == float_rgb)					\
    {								\
	f_p->r = (res)->r;					\
	f_p->g = (res)->g;					\
	f_p->b = (res)->b;					\
	f_p++;							\
    }								\
}
	
	 
/*
 * Blend two images such that
 * 	dest = (1-blend)*base + blend*overlay;
 */
static Error
blend_images(Pixtype b_type, Pointer b_ptr, RGBColor *b_map,
	     Pixtype o_type, Pointer o_ptr, RGBColor *o_map,
	     Pixtype c_type, Pointer c_ptr,
	     int height, int width, float blend)
{
    float 	bbar = 1.0 - blend;
    int		i,n;
    RGBColor	*fb = (RGBColor *)b_ptr, b_tmp, *b=NULL;
    RGBColor	*fo = (RGBColor *)o_ptr, o_tmp, *o=NULL;
    RGBColor	*fc = (RGBColor *)c_ptr;
    ubyte	*ub = (ubyte *)b_ptr;
    ubyte	*uo = (ubyte *)o_ptr;
    ubyte	*uc = (ubyte *)c_ptr;

    if (b_type == const_ubyte_rgb)
    {
	b_tmp.r = BYTE_TO_FLOAT(*ub++);
	b_tmp.g = BYTE_TO_FLOAT(*ub++);
	b_tmp.b = BYTE_TO_FLOAT(*ub++);
	b = &b_tmp;
    }
    else if (b_type == const_float_rgb)
    {
	b = (RGBColor *)b_ptr;
    }
    else if (b_type == const_mapped)
    {
	b = b_map + *ub;
    }

    if (o_type == const_ubyte_rgb)
    {
	o_tmp.r = BYTE_TO_FLOAT(*uo++);
	o_tmp.g = BYTE_TO_FLOAT(*uo++);
	o_tmp.b = BYTE_TO_FLOAT(*uo++);
	o = &o_tmp;
    }
    else if (o_type == const_float_rgb)
    {
	o = (RGBColor *)o_ptr;
    }
    else if (o_type == const_mapped)
    {
	o = o_map + *uo;
    }

    n = height*width;
    for (i=0; i<n; i++ )
    {
	RGBColor result;
	GET_PIXEL(b, b_tmp, b_type, ub, fb, b_map);
	GET_PIXEL(o, o_tmp, o_type, uo, fo, o_map);
	result.r = (b->r * bbar) + (o->r * blend);
	result.g = (b->g * bbar) + (o->g * blend);
	result.b = (b->b * bbar) + (o->b * blend);
	PUT_PIXEL(c_type, uc, fc, &result);
    }

    return OK;
}

/*
 * Chromakey two images such that
 * 	dest->pixel = (base->pixel == key ? overlay->pixel : base->pixel) 
 * for all pixels in base.
 */
static Error
key_images(Pixtype b_type, Pointer b_ptr, RGBColor *b_map,
	     Pixtype o_type, Pointer o_ptr, RGBColor *o_map,
	     Pixtype c_type, Pointer c_ptr,
	     int height, int width, RGBColor *key)
{
    int		i,n;
    RGBColor	*fb = (RGBColor *)b_ptr, b_tmp, *b=NULL;
    RGBColor	*fo = (RGBColor *)o_ptr, o_tmp, *o=NULL;
    RGBColor	*fc   = (RGBColor *)c_ptr;
    ubyte	*ub   = (ubyte *)b_ptr;
    ubyte	*uo   = (ubyte *)o_ptr;
    ubyte	*uc   = (ubyte *)c_ptr;
    float	red   = key->r;
    float	green = key->g;
    float	blue  = key->b;

    if (b_type == const_ubyte_rgb)
    {
	b_tmp.r = BYTE_TO_FLOAT(*ub++);
	b_tmp.g = BYTE_TO_FLOAT(*ub++);
	b_tmp.b = BYTE_TO_FLOAT(*ub++);
	b = &b_tmp;
    }
    else if (b_type == const_float_rgb)
    {
	b = (RGBColor *)b_ptr;
    }
    else if (b_type == const_mapped)
    {
	b = b_map + *ub;
    }

    if (o_type == const_ubyte_rgb)
    {
	o_tmp.r = BYTE_TO_FLOAT(*uo++);
	o_tmp.g = BYTE_TO_FLOAT(*uo++);
	o_tmp.b = BYTE_TO_FLOAT(*uo++);
	o = &o_tmp;
    }
    else if (o_type == const_float_rgb)
    {
	o = (RGBColor *)o_ptr;
    }
    else if (o_type == const_mapped)
    {
	o = o_map + *uo;
    }

    n = height*width;
    for (i=0; i<n; i++ )
    {
	GET_PIXEL(b, b_tmp, b_type, ub, fb, b_map);
	GET_PIXEL(o, o_tmp, o_type, uo, fo, o_map);
	if (COLORMATCH(((RGBColor *)b), red, green, blue))
	    PUT_PIXEL(c_type, uc, fc, o)
	else
	    PUT_PIXEL(c_type, uc, fc, b);
    }

    return OK;
}

/*
 * Blend two images with a matte of blends such that
 *   dest->pixel = base->pixel *(1-matte->data) + overlay->pixel*matte->data)
 * for all pixels in base.
 */
static Error
matte_images(Pixtype b_type, Pointer b_ptr, RGBColor *b_map,
	     Pixtype o_type, Pointer o_ptr, RGBColor *o_map,
	     Pixtype c_type, Pointer c_ptr,
	     int height, int width, float *blend)
{
    int		i,n;
    RGBColor	*fb = (RGBColor *)b_ptr, b_tmp, *b=NULL;
    RGBColor	*fo = (RGBColor *)o_ptr, o_tmp, *o=NULL;
    RGBColor	*fc   = (RGBColor *)c_ptr;
    ubyte	*ub   = (ubyte *)b_ptr;
    ubyte	*uo   = (ubyte *)o_ptr;
    ubyte	*uc   = (ubyte *)c_ptr;

    if (b_type == const_ubyte_rgb)
    {
	b_tmp.r = BYTE_TO_FLOAT(*ub++);
	b_tmp.g = BYTE_TO_FLOAT(*ub++);
	b_tmp.b = BYTE_TO_FLOAT(*ub++);
	b = &b_tmp;
    }
    else if (b_type == const_float_rgb)
    {
	b = (RGBColor *)b_ptr;
    }
    else if (b_type == const_mapped)
    {
	b = b_map + *ub;
    }

    if (o_type == const_ubyte_rgb)
    {
	o_tmp.r = BYTE_TO_FLOAT(*uo++);
	o_tmp.g = BYTE_TO_FLOAT(*uo++);
	o_tmp.b = BYTE_TO_FLOAT(*uo++);
	o = &o_tmp;
    }
    else if (o_type == const_float_rgb)
    {
	o = (RGBColor *)o_ptr;
    }
    else if (o_type == const_mapped)
    {
	o = o_map + *uo;
    }

    n = height*width;
    for (i=0; i<n; i++, blend++)
    {
	RGBColor result;
	float blnd = *blend;
	float obar = 1.0 - *blend;

	if ( blnd < 0 || blnd > 1.0)	/* Must check blends */
	    ErrorGotoPlus3( ERROR_DATA_INVALID, "#10110", 
		    "'blend' field datum", 0, 1 );

	GET_PIXEL(b, b_tmp, b_type, ub, fb, b_map);
	GET_PIXEL(o, o_tmp, o_type, uo, fo, o_map);

	result.r = (b->r * obar) + (o->r * blnd);
	result.g = (b->g * obar) + (o->g * blnd);
	result.b = (b->b * obar) + (o->b * blnd);

	PUT_PIXEL(c_type, uc, fc, &result);
    }

    return OK;

error:
    return ERROR;
}

/*
 * Determine if the given object has top level structure that is consistent
 * with an image. 
 */
static Error 
valid_image_object(Object image)
{
    switch ( DXGetObjectClass (image) )
    {
        case CLASS_FIELD:
            return(_dxf_ValidImageField ((Field) image));
        case CLASS_GROUP:
            switch ( DXGetGroupClass ( (Group) image)) {
                case CLASS_COMPOSITEFIELD:
		    return(OK);
	        default:
		   break;
            }
        default:
	   break;
    }
    return(ERROR); 
}
/*
 * Determine whether the given object represents a valid blending field.
 */
static Error 
valid_blend_field(Field matte)
{
    switch ( DXGetObjectClass ((Object)matte) )
    {
        case CLASS_FIELD:
            return OK;
        case CLASS_GROUP:
            switch ( DXGetGroupClass ( (Group) matte)) {
                case CLASS_COMPOSITEFIELD:
		    return OK;
	        default:
		   break;
            }
        default:
	   break;
    }
    return ERROR; 
}
/*
 * Check two images to be sure they have similar positions.
 *
 * Compatible images must have grids that...
 *	1) are 2 dimensional. 
 *	2) are dependendent on positions (not connections)
 *	3) have the same number of positions in each dimension
 *	4) have the same origin. 
 *	5) have the save deltas 
 *
 * Note: We assume (for error messages) that f1 is the base image and f2 
 * 	is the overlay image.
 *
 * Returns OK/ERROR and sets error code.
 */
static Error
compatible_grids(Field f1, Field f2, char *f1name, char *f2name)
{
    Array 	p1, p2;
    int		n1,n2,i;
    /* Make these larger than they have to be in case the grid is > 2 */
    int		cnt1[2], cnt2[2];
    float	origin1[2], origin2[2];
    float	delta1[2][2], delta2[2][2];

    p1 = (Array)DXGetComponentValue(f1,	"positions");
    if (!p1)   {
	DXSetError(ERROR_BAD_PARAMETER,"#10250",f1name, "positions");
	goto error;
    }
    

    p2 = (Array)DXGetComponentValue(f2,	"positions");
    if (!p2)   {
	DXSetError(ERROR_BAD_PARAMETER,"#10250",f2name, "positions");
	goto error;
    }
    

    if (!DXQueryGridPositions(p1, &n1, NULL, NULL, NULL))
	ErrorGotoPlus1(ERROR_BAD_PARAMETER, "#10612",f1name);

    if (!DXQueryGridPositions(p2, &n2, NULL, NULL, NULL))
	ErrorGotoPlus1(ERROR_BAD_PARAMETER, "#10612",f2name);

    if (n1 != 2) 
	ErrorGotoPlus1(ERROR_BAD_PARAMETER,"#10333",f1name);

    if (n2 != 2) 
	ErrorGotoPlus1(ERROR_BAD_PARAMETER,"#10333",f2name);

    /* Now that we know n1=n2=2, fill the arrays of information */
    if (!DXQueryGridPositions(p1, NULL, cnt1, origin1, delta1[0]) ||
	!DXQueryGridPositions(p2, NULL, cnt2, origin2, delta2[0])) 
	DXErrorGoto(ERROR_UNEXPECTED, "Cannot re-query grid positions");

    for (i=0 ; i<2 ; i++) {
	if (cnt1[i] != cnt2[i]) {
	    DXSetError(ERROR_BAD_PARAMETER,"#12345",
		i,f2name,cnt2[i],f1name,cnt1[i]);
	    goto error;
	}
	if (origin1[i] != origin2[i])  {
	    DXSetError(ERROR_BAD_PARAMETER,"#12345",
		i,f2name, origin2[i],f1name, origin1[i]);
	    goto error;
	}
	if ((delta1[i][0] != delta2[i][0]) || (delta1[i][1] != delta2[i][1])) {
	    DXSetError(ERROR_BAD_PARAMETER, "#12355",
		i,f2name,delta2[i][0],delta2[i][1],
		  f1name,delta1[i][0],delta1[i][1]);
	    goto error;
	}
    }
 
    return OK;
error:
    return ERROR;
}


static Error
getImagePixelData(Field image, Pixtype *ptype, Pointer *pixels, RGBColor **map)
{
    Type type;
    Category cat;
    int rank, shape[32];

    Array a = (Array)DXGetComponentValue(image, "colors");
    if (! a)
	DXErrorGoto(ERROR_DATA_INVALID, "#12325");
    
    DXGetArrayInfo(a, NULL, &type, &cat, &rank, shape);

    if (type == TYPE_FLOAT)
    {
	if (cat != CATEGORY_REAL || rank != 1 || shape[0] != 3)
	    DXErrorGoto(ERROR_DATA_INVALID, "pixel type is invalid");
	
	*ptype = float_rgb;
	if (map)
	    *map  = NULL;
    }
    else if (type == TYPE_UBYTE && rank == 1 && shape[0] == 3)
    {
	if (cat != CATEGORY_REAL)
	    DXErrorGoto(ERROR_DATA_INVALID, "pixel type is invalid");
	
	*ptype = ubyte_rgb;
	if (map)
	    *map  = NULL;
    }
    else if (type == TYPE_UBYTE && (rank == 0 || (rank == 1 && shape[0] == 1)))
    {
	Array mapa = (Array)DXGetComponentValue(image, "color map");

	if (!map)
	    DXErrorGoto(ERROR_DATA_INVALID, "illegal delayed-color image");

	if (cat != CATEGORY_REAL)
	    DXErrorGoto(ERROR_DATA_INVALID, "pixel type is invalid");
	
	if (!mapa)
	    DXErrorGoto(ERROR_DATA_INVALID, "invalid delayed-color image");

	*ptype = mapped;
	if (map)
	    *map  = (RGBColor *)DXGetArrayData(mapa);

    }
    else
	DXErrorGoto(ERROR_DATA_INVALID, "pixel type is invalid");
    
    if (DXGetArrayClass(a) == CLASS_CONSTANTARRAY)
    {
	*pixels = DXGetConstantArrayData(a);
	if      (*ptype == float_rgb) *ptype = const_float_rgb;
	else if (*ptype == ubyte_rgb) *ptype = const_ubyte_rgb;
	else if (*ptype == mapped)    *ptype = const_mapped;
    }
    else
	*pixels = DXGetArrayData(a);

    return OK;

error:
    return ERROR;
}

