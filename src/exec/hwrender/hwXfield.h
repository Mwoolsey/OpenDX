/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#ifndef tdmXfield_h
#define tdmXfield_h

/*---------------------------------------------------------------------------*\
 $Source: /src/master/dx/src/exec/hwrender/hwXfield.h,v $
  Author: Tim Murphy

  This file contains definitions for the 'xfield' structure. This structure
  represents a DX fieldobject which has been converted to a form which is more
  pallatable to the render(s).

\*---------------------------------------------------------------------------*/
enum approxE {
  approx_none,	/* no approximation */
  approx_flat,	/* flat shading - NOT IMPLEMENTED */
  approx_lines,	/* line rendering - NOT IMPLEMENTED */
  approx_dots,	/* dots */
  approx_box	/* box */
};

typedef struct SavedTextureS {
   void * Address;
   int Index;
} SavedTextureT;

typedef struct approxS {
  enum approxE	approx;
  int		density;
} approxT;

enum xr {
    XR_OPTIONAL=0,
    XR_REQUIRED=1
};

enum xd {
    XD_NONE,
    XD_GLOBAL,
    XD_LOCAL
};

typedef enum {
    tw_repeat,
    tw_clamp,
    tw_clamp_to_edge,
    tw_clamp_to_border
} textureWrapE;

typedef enum {
    tf_nearest,
    tf_linear,
    tf_nearest_mipmap_nearest,
    tf_nearest_mipmap_linear,
    tf_linear_mipmap_nearest,
    tf_linear_mipmap_linear
} textureFilterE;

typedef enum {
    tfn_decal,
    tfn_replace,
    tfn_modulate,
    tfn_blend
} textureFunctionE;

typedef enum {
    cf_off,
    cf_front,
    cf_back,
    cf_front_and_back
} cullFaceE;

typedef enum {
    lm_one_side,
    lm_two_side
} lightModelE;

/* dependencies XXX do NOT reorder these, they are used and indices into
 * static arrays in xfield.c
 */
typedef enum dependencyE  {
  dep_none,
  dep_field,
  dep_positions,
  dep_connections,
  dep_polylines
} dependencyT,*dependencyP;

/* type of connections */
typedef enum connectionTypeE {	
  ct_none,	       	/* none specified */
  ct_lines,		/* line segments */
  ct_polylines,		/* external polylines */
  ct_triangles,		/* triangles */
  ct_quads,		/* quads */
  ct_tetrahedra,	/* tetrahedra */
  ct_cubes,		/* cubes */
  ct_tmesh,		/* special internal case triangle mesh */
  ct_qmesh,		/* special internal case quad mesh */
  ct_flatGrid,		/* special internal case planer quad grid */
  ct_pline		/* special internal case polylines  */
} connectionTypeT,*connectionTypeP;

typedef struct materialAttrS
{
    float ambient;
    float diffuse;
    float specular;
    float shininess;
} materialAttrT, *materialAttrP;

typedef struct attributeS {
    
  hwFlagsT	flags;		/* booleans */
  int		flat_z;		/* use flat z in volume rendering ? */
  int 		skip;		/* skip faces at random in volume rendering */
  float 	color_multiplier;	/* multiplier for volume colors */
  float 	opacity_multiplier;	/* multiplier for volume opacities */
  int		shade;		/* 0 =  subpress lighting even if normals */
  approxT	buttonDown;
  approxT	buttonUp;
  float		fuzz;
  float		ff;
  float		linewidth;
  int		aalines;	/* 1 = turn on line anti-aliasing */
  materialAttrT front;
  materialAttrT back;

  float	mm[4][4];		/* Model transformation matrix */
  float	vm[4][4];

  dxObject texture;

  textureWrapE     texture_wrap_s;
  textureWrapE     texture_wrap_t;
  textureFilterE   texture_min_filter;
  textureFilterE   texture_mag_filter;
  textureFunctionE texture_function;

  cullFaceE        cull_face;
  lightModelE      light_model;

} attributeT;

typedef struct xfieldS {

  attributeT	attributes;

  float	Near;			/* near plane for perspective clipping */
  int	ntransConnections;    	/* count of total transparent connections
				   in all fields */
  
  /* bounding box */
  Point		box[8];
  
/* XXX hack for SGI complier */
#define nconnections ncntns
#define connections cntns
#define connections_array cntns_array
#define connections_data cntns_data

  /* connections */
  Array 	connections_array;
  ArrayHandle	connections;
  connectionTypeT connectionType;
  int		posPerConn;
  int 		nconnections;
  InvalidComponentHandle 	invCntns; /* invalid connections */

  /*
   * texture-mapping stuff
   */
  dxObject texture_field;
  ubyte *texture;
  int   myTextureData;
  Array uv_array;
  ArrayHandle uv;
  int   textureWidth, textureHeight;
  int   textureIsRGBA;
			     
  Array		polylines_array, edges_array;
  int *polylines, npolylines; /* polylines are handled separately since */
  int *edges, nedges;         /* they require an indirection through an */
			      /* "edges" component                      */
  
  /* positions */
  Array 	positions_array;
  ArrayHandle 	positions;
  int 		npositions;
  int		shape;			      /* 2 = 2d data, 3 = 3d data */
  InvalidComponentHandle 	invPositions; /* invalid connections */
  
  
  /* normals */
  Array 	normals_array;
  ArrayHandle 	normals;
  int		nnormals;
  dependencyT	normalsDep;
  
  /* front colors */
  Array 	fcolors_array;
  ArrayHandle 	fcolors;
  int 		nfcolors;
  dependencyT	fcolorsDep;
  dependencyT	colorsDep;

  /* back colors */
  Array 	bcolors_array;
  ArrayHandle 	bcolors;
  int 		nbcolors;
  dependencyT	bcolorsDep;
  
  /* color map */
  Array 	cmap_array;
  ArrayHandle 	cmap;
  int		ncmap;
  
  /* opacities */
  Array 	opacities_array;
  ArrayHandle 	opacities;
  int		nopacities;
  dependencyT	opacitiesDep;
  
  /* opacity map */
  Array 	omap_array;
  ArrayHandle 	omap;
  int		nomap;
  
  /* neighbors */
  Array 	neighbors_array;
  ArrayHandle 	neighbors;
  int	       	nneighbors;

  /* meshes  */
  Array 	meshes;
  int	       	nmeshes;
  dxObject	meshObject;

  Array		origConnections_array;
  int		origNConnections;

  /* Lighting coeficients amb, diff, spec, shine */
  float	lk[4];

  /* information about regular partition */
  int k[3];		/* counts, strides in each direction */

  int 		nClips;
  Point		*clipPts;
  Vector	*clipVecs;

  /* DEVICE PRIVATE DATA */
  /* NOTE: XXX This should really be a pointer to a private section */

  void		(*deletePrivate)(struct xfieldS *);

  /* for OLD_PORT_LAYER_CALLS */
  Field		field;
  Pointer	connections_data;
  Pointer	positions_data;
  Pointer	normals_data;
  Pointer	fcolors_data;
  Pointer	cmap_data;
  Pointer	opacities_data;
  Pointer	omap_data;
  Pointer	invCntns_data;

  /* Fields collected only for texture mapping */
  Point		v0,v1,v2,v3;		/* Text world space quad corners */
  Field		image;			/* image field */

  /* This is for the device dependent GLobject */
  long		glObject;

  SavedTextureT FlatGridTexture;

  int           UseDisplayList;
  int           UseFastClip;

} xfieldT;


typedef struct clipS
{
   unsigned char clip[8];
} clipT, *clipP;

struct sortListElement
{
    xfieldT *xf;
    int     poly;
    float   depth;
};

xfieldO       _dxf_newXfieldO(Field, attributeP, void *);
xfieldP       _dxf_getXfieldData(xfieldO);

xfieldP       _dxf_newXfieldP(Field, attributeP, void *);
int           _dxf_xfieldNconnections(xfieldP xf);
int           _dxf_xfieldSidesPerConnection(xfieldP xf);
attributeP    _dxf_xfieldAttributes(xfieldP xf);
materialAttrP _dxf_attributeFrontMaterial(attributeP att);

Error	      _dxf_getHwXfieldClipPlanes(xfieldO xo,
			int *nClips, Point **pts, Vector **vecs);
Error	      _dxf_setHwXfieldClipPlanes(xfieldO xo,
			int nClips, Point *pts, Vector *vecs);

Error      _dxf_deleteXfield(xfieldP xf);

#endif /* tdmXfield_h */
