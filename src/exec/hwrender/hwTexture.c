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

#include "hwDeclarations.h"
#include "hwXfield.h"
#include "hwMatrix.h"
#include "hwMemory.h"
#include "hwPortLayer.h"
#include "hwWindow.h"
#include "hwObjectHash.h"

#define String dxString
#define Object dxObject
#define Angle dxAngle
#define Matrix dxMatrix
#define Screen dxScreen
#define Boolean dxBoolean
#include "../libdx/internals.h"
#undef String
#undef Object
#undef Angle
#undef Matrix
#undef Screen
#undef Boolean

#include "hwDebug.h"

#define FLOAT_TO_UBYTE(val)  ((val)<=0.0 ? (ubyte)0.f   : \
			      (val)>=1.0 ? (ubyte)255.f : (ubyte)((val)*255.0f))

typedef struct _texture
{
    int       width, height;
    ubyte     *pixels;
    dxObject  textureObj;
    int       myPixels;
    int       isRGBA;
} *Texture;

static Error _getTexture_Field(Texture);
static Error _getTexture_CompositeField(Texture);

static Error
_deleteTexture(Pointer p)
{
    if (p)
    {
	Texture t = (Texture)p;
	if (t->myPixels)
	    DXFree((Pointer)t->pixels);
	if (t->textureObj)
	    DXDelete(t->textureObj);
    }
    return OK;
}

static dxObject
_newTexture(dxObject obj)
{
    dxObject txo = NULL;
    Texture texture = (Texture)DXAllocateZero(sizeof(struct _texture));
    if (! texture)
	goto error;

    texture->textureObj = DXReference(obj);

    if (DXGetObjectClass(obj) == CLASS_FIELD)
    {
	if (! _getTexture_Field(texture))
	    goto error;
    }
    else if (DXGetObjectClass(obj) == CLASS_GROUP &&
	    DXGetGroupClass((Group)obj) == CLASS_COMPOSITEFIELD)
    {
	if (! _getTexture_CompositeField(texture))
	    goto error;
    }
    else
    {
	DXSetError(ERROR_DATA_INVALID, "invalid object type for texture");
	goto error;
    }

    txo = (dxObject)DXNewPrivate((Pointer)texture, _deleteTexture);
    if (! txo)
	goto error;

    return txo;

error:
    _deleteTexture((Pointer)texture);
    return ERROR;
}

Error
_XTexture(Field f, xfieldT* xf, void *globals)
{
    DEFGLOBALDATA(globals);
    Type  type;
    int   rank, shape[32];
    dxObject attr, texture = _dxf_xfieldAttributes(xf)->texture;
    dxObject txo;

    if (texture)
    {
	Texture txp;

	xf->uv_array = (Array)DXGetComponentValue(f, "uv");
	if (! xf->uv_array)
	{
	    DXSetError(ERROR_DATA_INVALID,
		"texture attribute but no uv component");
	    goto error;
	}

	DXGetArrayInfo(xf->uv_array, NULL, &type, NULL, &rank, shape);

	if (type != TYPE_FLOAT || rank != 1 || shape[0] != 2)
	{
	    DXSetError(ERROR_DATA_INVALID, 
		"uv component must contain float 2-vectors");
		goto error;
	}

	attr = DXGetAttribute((dxObject)xf->uv_array, "dep");
	if (DXGetObjectClass(attr) != CLASS_STRING)
	{
	    DXSetError(ERROR_DATA_INVALID, 
		"uv dep attribute must be a DXString object");
	    goto error;
	}

	if (strcmp(DXGetString((dxString)attr), "positions"))
	{
	    DXSetError(ERROR_DATA_INVALID, 
		"uv must be dep on positions");
	    goto error;
	}

	xf->uv = DXCreateArrayHandle(xf->uv_array);
	if (! xf->uv)
	    goto error;
    
	DXReference((dxObject)xf->uv_array);

	txo = _dxf_QueryObject(TEXTUREHASH, texture);
	if (! txo)
	{
	    txo = _newTexture(texture);
	    if (! txo)
		goto error;

	    _dxf_InsertObject(TEXTUREHASH, texture, txo);
	}

	xf->texture_field = DXReference(txo);
	txp = (Texture)DXGetPrivateData((Private)txo);

	xf->textureWidth    = txp->width;
	xf->textureHeight   = txp->height;
	xf->texture         = txp->pixels;
	xf->textureIsRGBA   = txp->isRGBA;

        if ( xf->textureIsRGBA )
            /*  Set flag so primitives will be added to transparent SORTLIST  */
            _dxf_setFlags(_dxf_attributeFlags(_dxf_xfieldAttributes(xf)),
                          CONTAINS_TRANSPARENT_TEXTURE);
    }
    else
    {
	xf->texture_field = NULL;
	xf->uv_array      = NULL;
	xf->texture       = NULL;
    }

    return OK;

error:
    if (xf->texture_field)
	DXDelete((dxObject)xf->texture_field);
    if (xf->uv_array)
	DXDelete((dxObject)xf->uv_array);

    xf->texture_field = NULL;
    xf->texture = NULL;
    xf->uv_array = NULL;
    xf->uv = NULL;
    xf->myTextureData = 0;

    return ERROR;
}

static Error
_expandColorMap(Field tf, ubyte cmap[768] )
    /*  Extract the color map and convert to ubyte[3] format  */
{
    Type     type;
    int      i, n, rank, shape[32];
    Array    array;

    array = (Array)DXGetComponentValue(tf, "color map");
    if (! array)
    {   
	DXSetError(ERROR_MISSING_DATA, 
	    "texture map has indexed colors but is missing color map");
	goto error;
    }

    DXGetArrayInfo(array, &n, &type, NULL, &rank, shape);

    if ( n > 256 ) 
    {
	DXSetError(ERROR_MISSING_DATA, 
	    "texture map's color map has more than 256 entries");
	goto error;
    }

    if (type == TYPE_FLOAT && rank == 1 && shape[0] == 3)
    {
	float *map_src = (float *)DXGetArrayData(array);

	for (i = 0; i < 3*n; i++)
	{
	    float c = *map_src++;
	    cmap[i] = FLOAT_TO_UBYTE(c);
	}
    }
    else if (type == TYPE_UBYTE && rank == 1 && shape[0] == 3)
    {
	memcpy( cmap, (ubyte *)DXGetArrayData(array), 3*n );
    }
    else
    {
	DXSetError(ERROR_DATA_INVALID,
	    "texture has invalid color map type "
	    "(must be float or ubyte 3-vectors)");
	goto error;
    }
	
    return OK;

error:
    return ERROR;
}


static Error
_expandOpacityMap(Field tf, ubyte omap[256] )
    /*  Extract the opacity map and convert to ubyte format  */
{
    Type     type;
    int      i, n, rank, shape[32];
    Array    array;

    array = (Array)DXGetComponentValue(tf, "opacity map");
    if (! array)
    {   
	DXSetError(ERROR_MISSING_DATA, 
	    "indirect texture map missing opacity map component");
	goto error;
    }

    DXGetArrayInfo(array, &n, &type, NULL, &rank, shape);

    if ( n > 256 ) 
    {
	DXSetError(ERROR_MISSING_DATA, 
	    "texture map's opacity map has more than 256 entries");
	goto error;
    }

    if ( (type == TYPE_FLOAT &&
	  (rank == 0 || (rank == 1 && shape[0] == 1) )) )
    {
	float *map_src = (float *)DXGetArrayData(array);

	for (i = 0; i < n; i++)
	{
	    float o = *map_src++;
	    omap[i] = FLOAT_TO_UBYTE(o);
	}
    }
    else
    {
	DXSetError(ERROR_DATA_INVALID,
	    "texture has invalid opacity map type "
	    "(must be float scalars)");
	goto error;
    }
	
    return OK;

error:
    return ERROR;
}


static Error
_getTexture_Field(Texture t)
{
    Type     type, o_type;
    Category cat , o_cat;
    int   n, o_n, rank, o_rank, shape[32], o_shape[32];
    int   counts[2];
    Array tmp,colors,opac;
    int i;
    Field tf = (Field)t->textureObj;

    t->pixels   = NULL;
    t->myPixels = 0;

    tmp = (Array)DXGetComponentValue(tf, "connections");
    if (! tmp)
    {
	DXSetError(ERROR_DATA_INVALID, 
	    "texture field has no connections");
	goto error;
    }

    if (! DXQueryGridConnections(tmp, &n, counts))
    {
	DXSetError(ERROR_DATA_INVALID, 
	    "texture field must have regular connections");
	goto error;
    }

    if (n != 2)
    {
	DXSetError(ERROR_DATA_INVALID, 
	    "texture field must be 2-d regular connections");
	goto error;
    }

    t->width  = counts[1];
    t->height = counts[0];

    colors = (Array)DXGetComponentValue(tf, "colors");
    if (! colors)
	colors = (Array)DXGetComponentValue(tf, "data");
    if (! colors)
    {
	DXSetError(ERROR_DATA_INVALID, 
	    "texture has neither colors nor data");
	return ERROR;
    }
    DXGetArrayInfo(colors, &n, &type, &cat, &rank, shape);

    opac = (Array)DXGetComponentValue(tf, "opacities");
    if ( opac ) {
        DXGetArrayInfo(opac, &o_n, &o_type, &o_cat, &o_rank, o_shape);
        if ( o_n != n ) {
	    DXSetError(ERROR_MISSING_DATA, 
	        "length of color and opacity components do not match");
	    goto error;
        }
    }

    /*  Handle ubyte[3] colors (w/ optional float opacities)  */
    if (type == TYPE_UBYTE && rank == 1 && shape[0] == 3)
    {
	if ( !opac ) {
	    t->pixels = (ubyte *)DXGetArrayData(colors);
	    t->isRGBA   = 0;
	    t->myPixels = 0;
	}
	else {
    	    if (! (o_type == TYPE_FLOAT &&
    	          (o_rank == 0 || (o_rank == 1 && o_shape[0] == 1) )) ) {
		DXSetError(ERROR_MISSING_DATA, 
		    "opacities for a direct color texture map must be float");
		goto error;
	    }
	    {
		ubyte *dst, *c_src = (ubyte *)DXGetArrayData(colors);
		float *o_src = (float *)DXGetArrayData(opac);
	        t->pixels = (ubyte *)tdmAllocate(4*n*sizeof(ubyte));
	        dst   = t->pixels;

	        for (i = 0; i < n; i++) {
		    float opacity = *(o_src++);
		    *(dst++) = *(c_src++);
		    *(dst++) = *(c_src++);
		    *(dst++) = *(c_src++);
		    *(dst++) = FLOAT_TO_UBYTE(opacity);
                }
	        t->isRGBA   = 1;
	        t->myPixels = 1;
	    }
	}
    }
    else
    {
	int Bpp = opac ? 4 : 3;
	t->pixels = (ubyte *)tdmAllocate(Bpp*n*sizeof(ubyte));
	if (! t->pixels)
		goto error;
	t->myPixels = 1;	
    
	/*  Handle float[3] colors (w/ optional float opacities)  */
	if (type == TYPE_FLOAT && rank == 1 && shape[0] == 3)
	{
	    int i;
	    ubyte *dst = t->pixels;
	    float *c_src = (float *)DXGetArrayData(colors);
	    float *o_src = NULL;
	    
	    if ( opac ) {
	        o_src = (float *)DXGetArrayData(opac);
		if ( ! (o_type == TYPE_FLOAT &&
			(o_rank == 0 || (o_rank == 1 && o_shape[0] == 1) )) ) {
		    DXSetError(ERROR_MISSING_DATA, 
		     "opacities for a direct color texture map must be float");
		    goto error;
		}
		Bpp = 4;
	    }
    
	    t->isRGBA = opac != NULL;

	    for (i = 0; i < n; i++)
	    {
		float c, o;
		c = *c_src++, *dst++ = FLOAT_TO_UBYTE(c);
		c = *c_src++, *dst++ = FLOAT_TO_UBYTE(c);
		c = *c_src++, *dst++ = FLOAT_TO_UBYTE(c);
		if ( opac )
		  o = *o_src++, *dst++ = FLOAT_TO_UBYTE(o);
	    }
	}

	/*  Handle ubyte colors w/ float[3] or ubyte[3] colormap    */
	/*    (w/ optional ubyte opacities with float opacity map)  */
        else if (type == TYPE_UBYTE &&
                 (rank == 0 || (rank == 1 && shape[0] == 1) ))
	{
	    int i;
	    ubyte *dst = t->pixels;
	    ubyte *c_src = (ubyte *)DXGetArrayData(colors);
	    ubyte *o_src = NULL;
	    ubyte cmap[768], omap[256];

	    if (!_expandColorMap(tf, cmap))
	        goto error;

	    if ( opac ) {
		if ( !(o_type == TYPE_UBYTE &&
		       (o_rank == 0 || (o_rank == 1 && o_shape[0] == 1) )) ) {
		    DXSetError(ERROR_MISSING_DATA, 
			"indirect texture map opacities must be scalar ubytes");
		    goto error;
		}
		if ( !_expandOpacityMap(tf, omap))
		    goto error;

	        o_src = (ubyte *)DXGetArrayData(opac);
	    }

	    for (i = 0; i < n; i++)
	    {
		ubyte *c = cmap + 3 * *c_src++;
		*dst++ = *c++;
		*dst++ = *c++;
		*dst++ = *c++;
		if ( opac )
		   *dst++ = omap[ *o_src++ ];
	    }
	    t->isRGBA = opac != NULL;
	}
	else
	{ 
	    DXSetError(ERROR_DATA_INVALID,
		"Bad texture found.  Texture must contain float or ubyte "
		"3-vector colors (with optional float opacities), OR contain "
		"scalar ubyte colors with a float or ubyte 3-vector color map "
		"(with optional ubyte opacities and a float opacity map)");
	    goto error;
	}
    }
	
    return OK;

error:
    if ( t->pixels && t->myPixels ) {
        DXFree((Pointer)t->pixels);
	t->pixels   = NULL;
	t->myPixels = 0;
    }
    return ERROR;
}

static Error
_getTexture_CompositeField(Texture t)
{
    Type  type, o_type;
    Category cat, o_cat;
    int   n, o_n, rank, o_rank, shape[32], o_shape[32], counts[2];
    int i, Bpp;
    Group cf = (Group)t->textureObj;
    Field member;
    int nMembers;

    t->pixels   = NULL;
    t->myPixels = 0;

    DXGetMemberCount(cf, &nMembers);
    if ( nMembers == 0 ) {
        DXSetError(ERROR_DATA_INVALID, 
	           "composite texture field has no members");
	goto error;
    }

    counts[0] = counts[1] = 0;

    /*  Verify all members are regular 2D grids, and calc cumulative size  */
    for (i = 0; i < nMembers; i++)
    {
	Array grid;
	int memberCounts[2], memberOffsets[2], nDim, cnt;

	member = (Field)DXGetEnumeratedMember(cf, i, NULL);
	grid = (Array)DXGetComponentValue(member, "connections");

	if (!DXQueryGridConnections(grid, &nDim, memberCounts) ||
	    !DXGetMeshOffsets((MeshArray)grid, memberOffsets))
	{
	    DXSetError(ERROR_DATA_INVALID, 
		"texture field must have regular connections");
	    goto error;
	}

	if (nDim != 2)
	{
	    DXSetError(ERROR_DATA_INVALID, "texture fields must be 2-D");
	    goto error;
	}

	if ((cnt = memberOffsets[0] + memberCounts[0]) > counts[0])
	    counts[0] = cnt;

	if ((cnt = memberOffsets[1] + memberCounts[1]) > counts[1])
	    counts[1] = cnt;
    
    }
    t->width  = counts[1];
    t->height = counts[0];

    n = counts[0] * counts[1];

    /*  Allocate texture buffer  */
    member = (Field)DXGetEnumeratedMember(cf, 0, NULL);
    t->isRGBA = (DXGetComponentValue(member, "opacities") != NULL);
    Bpp       = t->isRGBA ? 4 : 3;
    t->pixels = (ubyte *)tdmAllocate(Bpp*n*sizeof(ubyte));
    if (! t->pixels)
	goto error;
    t->myPixels = 1;

    /*  Copy colors/opacities from each member into the texture buffer  */
    for (i = 0; i < nMembers; i++)
    {
	Array colors,opac;
	Array grid;
	int memberCounts[2], memberOffsets[2];

	member = (Field)DXGetEnumeratedMember(cf, i, NULL);
	grid = (Array)DXGetComponentValue(member, "connections");

	colors = (Array)DXGetComponentValue(member, "colors");
	if (! colors)
	    colors = (Array)DXGetComponentValue(member, "data");
	if (! colors)
	{
	    DXSetError(ERROR_DATA_INVALID, 
		"texture has neither colors nor data");
	    return ERROR;
	}
	DXGetArrayInfo(colors, NULL, &type, &cat, &rank, shape);

	opac = (Array)DXGetComponentValue(member, "opacities");
	if ( opac ) {
	    DXGetArrayInfo(opac, &o_n, &o_type, &o_cat, &o_rank, o_shape);
	    if ( o_n != n ) {
		DXSetError(ERROR_MISSING_DATA, 
		    "length of color and opacity components do not match");
		goto error;
	    }
	}


	DXQueryGridConnections(grid, NULL, memberCounts);
	DXGetMeshOffsets((MeshArray)grid, memberOffsets);

	/*  Handle ubyte[3] colors (w/ optional float opacities)  */
	if (type == TYPE_UBYTE && rank == 1 && shape[0] == 3)
	{
	    int j, k;
	    ubyte *c_src = (ubyte *)DXGetArrayData(colors);
	    float *o_src = NULL;
	    ubyte *dst = t->pixels + Bpp*((memberOffsets[0]*counts[1])
					     + memberOffsets[1]);
	    int dst_stride = Bpp*(counts[1] - memberCounts[1]);

	    if ( opac ) {
		if (! (o_type == TYPE_FLOAT &&
		      (o_rank == 0 || (o_rank == 1 && o_shape[0] == 1) )) ) {
		    DXSetError(ERROR_MISSING_DATA, 
		      "opacities for a direct color texture map must be float");
		    goto error;
		}
	        o_src = (float *)DXGetArrayData(opac);
	    }

	    for (j = 0; j < memberCounts[0]; j++)
	    {
		for (k = 0; k < memberCounts[1]; k++)
		{
		    float o;
		    *dst++ = *c_src++;
		    *dst++ = *c_src++;
		    *dst++ = *c_src++;
		    if ( opac )
		        o = *o_src++, *dst++ = FLOAT_TO_UBYTE(o);
		}
		dst += dst_stride;
	    }
	}

	/*  Handle float[3] colors (w/ optional float opacities)  */
	else if (type == TYPE_FLOAT && rank == 1 && shape[0] == 3)
	{
	    int j, k;
	    float *c_src = (float *)DXGetArrayData(colors);
	    float *o_src = NULL;
	    ubyte *dst = t->pixels + Bpp*((memberOffsets[0]*counts[1])
					     + memberOffsets[1]);
	    int dst_stride = Bpp*(counts[1] - memberCounts[1]);

	    if ( opac ) {
		if (! (o_type == TYPE_FLOAT &&
		      (o_rank == 0 || (o_rank == 1 && o_shape[0] == 1) )) ) {
		    DXSetError(ERROR_MISSING_DATA, 
		      "opacities for a direct color texture map must be float");
		    goto error;
		}
	        o_src = (float *)DXGetArrayData(opac);
	    }

	    for (j = 0; j < memberCounts[0]; j++)
	    {
		for (k = 0; k < memberCounts[1]; k++)
		{
		    float c, o;
		    c = *c_src++, *dst++ = FLOAT_TO_UBYTE(c);
		    c = *c_src++, *dst++ = FLOAT_TO_UBYTE(c);
		    c = *c_src++, *dst++ = FLOAT_TO_UBYTE(c);
		    if ( opac )
		        o = *o_src++, *dst++ = FLOAT_TO_UBYTE(o);
		}
		dst += dst_stride;
	    }
	}

	/*  Handle ubyte colors w/ float[3] or ubyte[3] colormap    */
	/*    (w/ optional ubyte opacities with float opacity map)  */
	else if (type == TYPE_UBYTE &&
	    (rank == 0 || (rank == 1 && shape[0] == 1) ))
	{
	    int j, k;
	    ubyte cmap[768], omap[256];
	    ubyte *c_src = (ubyte *)DXGetArrayData(colors);
	    ubyte *o_src = NULL;
	    int dst_stride = Bpp*(counts[1] - memberCounts[1]);
	    ubyte *dst = t->pixels + Bpp*((memberOffsets[0]*counts[1])
					     + memberOffsets[1]);

	    if (!_expandColorMap(member, cmap))
	        goto error;

	    if ( opac ) {
		if ( !(o_type == TYPE_UBYTE &&
		       (o_rank == 0 || (o_rank == 1 && o_shape[0] == 1) )) ) {
		    DXSetError(ERROR_MISSING_DATA, 
			"indirect texture map opacities must be scalar ubytes");
		    goto error;
		}

		if ( !_expandOpacityMap(member, omap))
		    goto error;

	        o_src = (ubyte *)DXGetArrayData(opac);
	    }

	    for (j = 0; j < memberCounts[0]; j++)
	    {
		for (k = 0; k < memberCounts[1]; k++)
		{
		    ubyte *c = cmap + (3 * *c_src++);
		    *dst++ = *c++;
		    *dst++ = *c++;
		    *dst++ = *c++;
		    if ( opac )
		        *dst++ = omap[ *o_src++ ];
		}
		dst += dst_stride;
	    }
	}
	else
	{ 
	    DXSetError(ERROR_DATA_INVALID,
		"Bad texture found.  Texture must contain float or ubyte "
		"3-vector colors (with optional float opacities), OR contain "
		"scalar ubyte colors with a float or ubyte 3-vector color map "
		"(with optional ubyte opacities and a float opacity map)");
	    goto error;
	}
    }
	
    return OK;

error:
    if ( t->pixels && t->myPixels ) {
        DXFree((Pointer)t->pixels);
	t->pixels   = NULL;
	t->myPixels = 0;
    }
    return ERROR;
}
