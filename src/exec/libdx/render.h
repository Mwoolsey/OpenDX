/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>




/* TeX starts here. Do not remove this comment. */

/*
\section{Rendering}
This section describes the internal routines and data structures
associated with rendering.
*/

/*
\paragraph{Xfield}  The {\tt xfield} data structure contains information
extracted from a field.  Extracting this information once avoids a lot
of overhead.  The {\tt tile} structure contains tiling parameters relevant
to the field.  We declare it as a separate structure, because we use it
later on to pass down tiling parameters as we traverse the structure.
*/

struct tile {		/* tiling parameters */
    int ignore;		/* don't get new parameters - e.g. w/in comp field */
    int fast_exp;	/* use fast exponent in volume rendering */
    int flat_x;		/* flat x in volume rendering */
    int flat_y;		/* flat y in volume rendering */
    int flat_z;		/* flat z in volume rendering */
    int skip;		/* skip faces at random in volume rendering */
    int patch_size;	/* volume rendering patch size */
    float color_multiplier;	/* multiplier for volume colors */
    float opacity_multiplier;	/* multiplier for volume opacities */
    int if_group;	/* in interference group */
    int if_object;	/* in interference object */
    int perspective;	/* do perspective for this field? */
    enum approx {
	approx_none,	/* no approximation */
	approx_flat,	/* flat shading - NOT IMPLEMENTED */
	approx_lines,	/* line rendering - NOT IMPLEMENTED */
	approx_dots,	/* dots */
	approx_box	/* box */
    } approx;
};

Error _dxf_approx(Object o, enum approx *approx);


#define MAXLIGHTS 100

struct lightList
{
    int      na;
    int      nl;
    RGBColor ambient;
    Light    lights[MAXLIGHTS];
    Vector   h[MAXLIGHTS];
};

typedef struct lightList *LightList;

typedef enum {
	dep_positions,
	dep_connections,
	dep_polylines
} dependency;

struct xfield {

    Point *box;			/* bounding box */
    int nbox;
    float nearPlane;			/* near plane for perspective clipping */

    /* positions and typically point-dependent data */
    Point *positions;		/* control points */
    Pointer fcolors, bcolors;	/* front/back colors or byte data */
    RGBColor *cmap;		/* color map iff above is byte data */
    Pointer opacities;		/* opacities or byte data */
    float *omap;		/* opacity map iff above is byte data */
    Vector *normals;		/* normals */

    /* counts */
    int npositions;		/* number of positions */
    int ncolors;		/* number of colors, normals, opacities */

    /* dependencies */
    dependency colors_dep, normals_dep;

    /* standard connections */
    enum ct {			/* type of connections */
	ct_none,		/* none specified */
	ct_lines,		/* line segments */
	ct_polylines,		/* line segments */
	ct_triangles,		/* triangles */
	ct_quads,		/* quads */
	ct_tetrahedra,		/* tetrahedra */
	ct_cubes		/* cubes */
    } ct;
    union {			/* data array for the irregular ones only */
	Line *lines;
	Triangle *triangles;
	Tetrahedron *tetrahedra;
	Quadrilateral *quads;
	Cube *cubes;
	int *i;
    } c;
    int volume;			/* whether it is a volume connection */
    int nconnections;		/* number of connection elements */
    Pointer neighbors;		/* neighbors data */

    int *polylines, npolylines;	/* polylines are handled separately since */
    int *edges, nedges;         /* they require an indirection through an */
				/* "edges" component			  */

    /* information about regular partition */
    Point o;			/* back corner of partition */
    Vector d[3];		/* axes of partition box, relative to o */
    int n;			/* point id of back corner of partition */
    int k[3], s[3];		/* counts, strides in each direction */
    int cn;			/* point id of back corner of partition */
    int ck[3], cs[3];		/* counts, strides in each direction */

    /* tiling options applicable to this field */
    struct tile tile;

    /* the arrays */
    Array box_array;
    Array positions_array;
    Array neighbors_array;
    Array fcolors_array;
    Array bcolors_array;
    Array opacities_array;
    Array normals_array;
    Array faces_array;
    Array polylines_array;
    Array loops_array;
    Array edges_array;
    Array inner_array;
    Array connections_array;
    Array cmap_array;
    Array omap_array;

    /* flags to indicate whether data is local */
    char box_local;
    char positions_local;
    char neighbors_local;
    char opacities_local;
    char fcolors_local;
    char bcolors_local;
    char normals_local;
    char surface_local;
    char inner_local;
    char faces_local;
    char polylines_local;
    char loops_local;
    char edges_local;
    char cmap_local;
    char omap_local;
    char volAlg;

    InvalidComponentHandle iElts, iPts;

    /*
     * lighting parameters for delayed lighting
     */
    LightList lights;
    float kaf, kdf, ksf;
    int   kspf;
    float kab, kdb, ksb;
    int   kspb;

    int   *indices;

    /*
     * The following has been added to support constant normals and colors
     * components without expansion.  If the components are constant, the 
     * following buffers will contain the component value and the cst flags
     * will be set.
     */
    Point    nbuf;
    RGBColor fbuf, bbuf;
    float    obuf;
    char     fcst, bcst, ncst, ocst;
    char     fbyte, bbyte, obyte;

};

#define InitializeXfield(xfld) xfld.box = NULL; \
    xfld. positions = NULL; \
    xfld.neighbors = NULL; \
    xfld.fcolors = NULL; \
    xfld.bcolors = NULL; \
    xfld.normals = NULL; \
	xfld.opacities = NULL; \
	xfld.polylines = NULL; \
	xfld.edges = NULL; \
    xfld.cmap = NULL; \
    xfld.omap = NULL; \
    xfld.box_array = NULL; \
    xfld.positions_array = NULL; \
    xfld.neighbors_array = NULL; \
    xfld.fcolors_array = NULL; \
    xfld.bcolors_array = NULL; \
    xfld.normals_array = NULL; \
    xfld.opacities_array = NULL; \
    xfld.faces_array = NULL; \
    xfld.polylines_array = NULL; \
    xfld.loops_array = NULL; \
    xfld.edges_array = NULL; \
    xfld.inner_array = NULL; \
    xfld.connections_array = NULL; \
    xfld.cmap_array = NULL; \
    xfld.omap_array = NULL; \
	xfld.iPts = NULL; \
	xfld.iElts = NULL;


enum xr {
    XR_OPTIONAL=0,
    XR_REQUIRED=1
};

enum xd {
    XD_NONE,
    XD_GLOBAL,
    XD_LOCAL
};

typedef enum
{
    INV_UNKNOWN,
    INV_VALID,
    INV_INVALID
} inv_stat;

void _dxf_XZero(struct xfield *xf);
Error _dxf_XBox(Field f, struct xfield *xf, enum xr required, enum xd xd);
Error _dxf_XPositions(Field f, struct xfield *xf, enum xr required, enum xd xd);
Error _dxf_XConnections(Field f, struct xfield *xf, enum xr required);
Error _dxf_XNeighbors(Field f, struct xfield *xf, enum xr required, enum xd xd);
Error _dxf_XColors(Field f, struct xfield *xf, enum xr required, enum xd xd);
Error _dxf_XNormals(Field f, struct xfield *xf, enum xr required, enum xd xd);
Error _dxf_XOpacities(Field f, struct xfield *xf, enum xr required, enum xd xd);
Error _dxf_XSurface(Field f, struct xfield *xf, enum xr required, enum xd xd);
Error _dxf_XInner(Field f, struct xfield *xf, enum xr required, enum xd xd);
Error _dxf_XLighting(Field f, struct xfield *xf);
Error _dxf_XInvalidPositions(Field f, struct xfield *xf);
Error _dxf_XInvalidConnections(Field f, struct xfield *xf);
Error _dxf_XInvalidPolylines(Field f, struct xfield *xf);
Error _dxf_XPolylines(Field f, struct xfield *xf, enum xr required, enum xd xd);
/**
These routines extract various components.  The {\tt xr} parameter
indicates whether the component is required, i.e. whether it is an
error for it not to be present.  The {\tt xd} parameter tells us
whether to get the data, and if so whether to copy it to local memory.
This is an issue because getting the data from a regular array
potentially involves expanding a compact representation.
**/

Error _dxf_XFreeLocal(struct xfield *xf);
/**
Frees the local data for all the arrays in {\tt *xf} that were copied
to local memory.
**/

/*
\paragraph{Rendering Buffer}  The rendering buffer data structure contains
information relevant to rendering a particular screen patch, including a
pointer to the pixels themselves (in local memory).  This section describes
the rendering buffer and a set of low-level routines for rendering primitives
into the buffer.
*/

struct rgbo {
    float r, g, b, o;
};

struct buffer {
    int width, height;			/* size */
    int ox, oy;				/* origin */
    int merged;				/* have back and z been merged? */
    Point min, max;			/* for bbox check */
    Point omin, omax;			/* object size for interference */
    Point fmin, fmax;			/* face size */

    union {
	struct big {
	    RGBColor c;			/* accumulated color */
	    struct rgbo co;		/* 1) volume interp 2) face color */
	    float z;			/* last rendered z */
	    float front, back;		/* front/back clip surfaces */
	    int in;			/* in/out of 1) volume 2) face */
	} *big;				/* buffer */
	struct fast {
	    RGBColor c;			/* color */
	    float z;			/* z value */
	} *fast;			/* fast buffer */
	struct zbuf {
	    float z;			/* z value */
	} *zbuf;
    } u;				/* possibly aligned buffer */
    enum pix_type {
	pix_big,
	pix_fast,
	pix_zbuf
    } pix_type;				/* buffer pixel type */
    Pointer buffer;			/* the buffer for freeing */
    
    int iwidth;				/* image width */
    union {
	struct fbpixel *fb;		/* image is fb pixels */
	struct RGBColor *rgb;		/* image is float colors */
	unsigned char *x;		/* image is x byte image */
    } img;
    enum img_type {
	img_fb,				/* image is fb pixels */
	img_rgb,			/* image is float colors */
	img_x				/* image is x byte image */
    } img_type;

    int init;				/* init method to use */
    int showpatches;			/* whether to show patch outline */
    int notriangles;			/* return at beginning of Triangle */
    int setuponly;			/* return after triangle setup */
    int tri;				/* triangle method to use */
    int irregular;			/* force irregular */
    char *vol;				/* regular volume method to use */
    int triangles;			/* return count */
    int pixels;				/* return count */
};

Error _dxf_InitBuffer(struct buffer *b, enum pix_type pix_type,
		  int width, int height, int ox, int oy, RGBColor c);
/**
Initialize a rendering buffer structure.
**/

Error _dxf_CopyBufferToImage(struct buffer *b, Field i, int xoff, int yoff);
Error _dxf_CopyBufferToFBImage(struct buffer *b, Field i, int xoff, int yoff);
Error _dxf_CopyBufferToXImage(struct buffer *b, Field i, int xoff, int yoff);
/**
Copies rendering buffer {\tt b} to image {\tt i} at the specified offset
in the image.  Returns {\tt b} or null to indicate an error.
**/

Error _dxf_BeginClipping(struct buffer *b);
/**
Initialize the buffer for clipping.
**/

Error _dxf_EndClipping(struct buffer *b);
/**
Restore the buffer after clipping.
**/

Error _dxfCapClipping(struct buffer *b, float);
/**
If in perspective, may need to cap the clipping volume
(eg. if part of the front surface was clipped away by the
Z clip).
**/

Error _dxf_Triangle(struct buffer *b, struct xfield *xf, int n,
		Triangle *tri, int *indices,
		int surface, int clip_status, inv_stat invalid_status);
Error _dxf_Quad(struct buffer *b, struct xfield *xf, int n,
		Quadrilateral *quad, int *indices,
		int surface, int clip_status, inv_stat invalid_status);
/**
Renders face {\tt tri} from field {\tt xf} into rendering buffer
{\tt b}, treating it as a surface face or an inner face according to
{\tt surface}.  Returns {\tt b} or null to indicate an error.
(Question: should we include {\tt clip\_status} as an argument to
allow selection of clipped/non-clipped algorithm, for performance?)
**/

Error _dxf_TriangleFlat(struct buffer *b, struct xfield *xf, int n,
		    Triangle *tri, int *indices,
		    Pointer front, Pointer back, Pointer o,
		    int surface, int clip_status, inv_stat invalid_status);
Error _dxf_QuadFlat(struct buffer *b, struct xfield *xf, int n,
		    Quadrilateral *quad, int *indices,
		    Pointer front, Pointer back, Pointer o,
		    int surface, int clip_status, inv_stat invalid_status);
/**
Renders face {\tt tri} from field {\tt xf} into rendering buffer
{\tt b}, treating it as a surface face or an inner face according to
{\tt surface}.  Uses flat shading with specified color and opacity.
Returns {\tt b} or null to indicate an error.
**/

Error _dxf_TriangleClipping(struct buffer *b, struct xfield *xf,
			    int n, Triangle *tri, int *indices);
Error _dxf_QuadClipping(struct buffer *b, struct xfield *xf,
			int n, Quadrilateral *quad, int *indices);
/**
Renders face {\tt tri} from field {\tt xf} as a clipping triangle.
**/

/* These aren't needed in the header. 
static Error _dxf_TriangleComposite(struct buffer *b, struct xfield *xf, int n,
				   Triangle *tri, int *indices,
			 	   int clip_status, inv_stat invalid_status);
static Error _dxf_QuadComposite(struct buffer *b, struct xfield *xf, int n,
			 	Quadrilateral *quad, int *indices,
			 	int clip_status, inv_stat invalid_status);
*/
/**
Composites a triangle or a quad.
**/

Error _dxf_TriangleCompositeFlat(struct buffer *b, struct xfield *xf, int n,
			     Triangle *tri, int *indices, Pointer colors,
			     Pointer op, int clip_status, inv_stat invalid_status);
Error _dxf_QuadCompositeFlat(struct buffer *b, struct xfield *xf, int n,
			     Quadrilateral *quad, int *indices, Pointer colors,
			     Pointer op, int clip_status, inv_stat invalid_status);

Error _dxf_Polyline(struct buffer *b, struct xfield *xf, int n, int *line,
	    int *indices, int clip_status, inv_stat invalid_status);
Error _dxf_PolylineFlat(struct buffer *b, struct xfield *xf, int n, int *line,
		int *indices,
		Pointer colors, Pointer opacities, int clip_status, inv_stat invalid_status);

Error _dxf_Line(struct buffer *b, struct xfield *xf, int n, Line *line,
	    int *indices, int clip_status, inv_stat invalid_status);
Error _dxf_LineFlat(struct buffer *b, struct xfield *xf, int n, Line *line,
		int *indices,
		Pointer colors, Pointer opacities, int clip_status, inv_stat invalid_status);

Error _dxf_Points(struct buffer *b, struct xfield *xf);
Error _dxf_Point(struct buffer *b, struct xfield *xf, int i);
/**
DXRender all the positions of field {\tt xf} as points in buffer {\tt b}, or
render one translucent point.
**/

#define ALL8(b,op,m,x) ( \
    b[0].x op m.x && b[1].x op m.x && b[2].x op m.x && b[3].x op m.x && \
    b[4].x op m.x && b[5].x op m.x && b[6].x op m.x && b[7].x op m.x )
#define CULLBOX(b,min,max) \
    (ALL8(b,<,min,x)|| ALL8(b,<,min,y)||ALL8(b,>=,max,x)||ALL8(b,>=,max,y))


/*
\paragraph{Object traversal}  The following routines traverse entire objects.
In general, they are called from Tile.
*/

/* clip_status values */
#define CLIPPING (-1)			/* this is a clipping object */
#define UNCLIPPED 0			/* this is an unclipped object */
#define CLIPPED(x) ((x)>0)		/* this is a clipped object */
#define NEST(x) ((x)+1)			/* nested clipping */

Object _dxfPaint(Object o, struct buffer *b, int clip_status, struct tile *tile);
/**
Adds the opaque parts of the {\tt Object} to the rendering buffer.
The {\tt clip} argument specifies whether we are rendering an unclipped
object, rendering a clipped object, or rendering the object that does
the clipping.  Returns {\tt b} or null to indicate an error.
**/

struct count {
    lock_type DXlock;		/* for parallel update */
    int nx, ny;			/* input number of screen patches */
    int width, height;		/* size of image */
    int irregular;		/* number of irregular (vol/surf) fields */
    int volume;			/* number of volume fields */
    int regular;		/* number of regular volume fields */
    int fast;			/* whether fast small fb can be used */
    struct pcount {		/* patch count */
	int nconnections;	/* number of connections for work estimate */
	int tfields;		/* number of translucent fields */
	int nbytes;		/* number of bytes for sort array */
    } *pcount;
};

#define SORT_POINT	    1	/* index into point connections component */
#define SORT_LINE	    2	/* index into line connections component */
#define SORT_POLYLINE	    3	/* index into line connections component */
#define SORT_TRI	    4	/* index into triangle connections component */
#define SORT_QUAD	    5	/* index into quads connections component */
#define SORT_TRI_PTS	    6	/* three point indices forming a triangle */
#define SORT_QUAD_PTS	    7	/* four point indices forming a quad */
#define SORT_TRI_FLAT	    8	/* three point indices plus color index */
#define SORT_QUAD_FLAT	    9	/* four point indices plus color index */

/*
 * For volume elements that we a) sort and b) have to render even if
 * invalid, determine the validity type.
 */
#define SORT_INV_TRI_PTS   10	/* three point indices forming a triangle */
#define SORT_INV_QUAD_PTS  11	/* four point indices forming a quad */
#define SORT_INV_TRI_FLAT  12	/* three point indices plus color index */
#define SORT_INV_QUAD_FLAT 13	/* four point indices plus color index */

struct sort {
    short field;		/* index into gather->xfields array */
    char type;			/* type (one of the SORT_... constants) */
    char surface;		/* whether it's a surface face */
};

struct sortindex {
    struct sort *sort;
    float z;
};

struct gather {
    int cull;			/* do/don't cull (e.g. perspective) XXX */
    Point min, max;		/* bounds to cull clip against */
    struct sort *sort;		/* beginning of sort array */
    struct sort *current;	/* current position in sort array */
    int nthings;		/* number of triangles, etc. seen */
    int nfields;		/* number of fields seen */
    struct xfield *fields;	/* array of xfield data structures */
    Object clipping;		/* clipping object for translucent surfaces */
};

Object _dxfGather(Object o, struct gather *gather, struct tile *tile);
/**
Gathers all the translucent faces from {\tt r} into {\tt
sort} that are visible in the box specified by {\tt min} and {\tt
max}, and adds the count of the number of faces gathered to {\tt
nfaces}.  The faces can then be sorted by $z$ and rendered.
(Question: sort by $z$ is only approximately correct; this is ok for
small faces in volume rendering, but what about translucent surfaces?)
The clipping object, if any, is returned in {\tt clipping}; all
volumes and translucent faces in the scene must have the same clipping
object.
**/

Error _dxf_VolumeIrregular(struct buffer *b, struct gather *gather, int clip);
/**
DXRender the volume and translucent faces gathered in {\tt gather} into
buffer {\tt b}, using the irregular face algorithm.
**/

Error _dxf_VolumeRegular(struct buffer *b, struct gather *gather, int clip);
/**
DXRender the volumes gathered in {\tt gather} into buffer {\tt b},
using a regular volume rendering algorithm.
**/

Error
_dxf_CompositePlane(struct buffer *b, float osx, float osy, float osz, int clip,
		float ux, float uy, float uz, float vx, float vy, float vz,
		Pointer ic, char cstcolors, RGBColor *cmap,
		Pointer io, char cstopacities, float *omap,
		float cmul, float omul,
		int idx, int idy, int inx, int iny);
/**
Composite a plane of quads.
**/

Error
_dxf_MergeBackIntoZ(struct buffer *b);
/**
Merge the back and z buffers; needed for the irregular face algorithm.
**/

#define IN_FACE	        1
#define IF_OBJECT       2
#define IF_UNION        4
#define IF_INTERSECTION 8

#define D_POS 1
#define D_CON 2

Error _dxf_CopyFace(struct buffer *b, int if_object);
Error _dxf_InterferenceObject(struct buffer *buf);
Private   _dxfGetLights(Object, Camera);
Error     _dxfFreeLightList(LightList);
Error     _dxfInitApplyLights(float, float, float, int,
			      float, float, float, int,
			      Pointer, Pointer, RGBColor *,
			      Vector *, LightList, dependency, dependency,
			      RGBColor *, RGBColor *, int,
			      char, char, char, char, char);
Error     _dxfApplyLights(int *, int *, int);
Field _dxf_ZeroPixels(Field image, int left, int right, int top, int bot, RGBColor color);
int _dxf_getXDepth(char *type);
Object _dxfXShade(Object o, Camera c, struct count *count, Point *box);


#define FRONTFACING 1
#define BACKFACING  2

int _dxf_zclip_triangles(struct xfield *, int *,
			    int, int *, int, struct xfield *, inv_stat);
int _dxf_zclip_quads(struct xfield *, int *,
			    int, int *, int, struct xfield *, inv_stat);

extern float _dxd_ubyteToFloat[256];
void  _dxf_initUbyteToFloat();


