/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>

#ifndef _INTERNALS_H_
#define _INTERNALS_H_

#if !defined(DX_NATIVE_WINDOWS)
#include <X11/X.h>
#endif

char *_dxfstring(char *s, int add);
/**
Add a string to the string table.
**/

Object _dxfstringobject(char *s, int add);
/**
Add a string object to the string table.
**/

Object _dxf_SetPermanent(Object o);
/**
Make object {\tt o} undeletable.
**/

Error _dxf_SetType(Group g, Object o);
/**
Set the type of group {\tt g} to be the same as the type of object {\tt o}.
**/

Error _dxf_initmemqueue();
/**
The message queue is maintained as a linked list of messages.
The struct queue contains the pointers to the first and last
elements of the list, and a DXlock for the list.  It is allocated
and initialized by _dxf_initmessages().
**/

Array _dxf_TetraNeighbors(Field f);
Array _dxf_CubeNeighbors(Field f);
int _dxf_initlocks(void);
int _dxf_initmemory(void);
Error _dxf_initmessages(void);
Error _dxf_inittiming(void);
Error _dxf_initobjects(void);
Error _dxf_initstringtable(void);
Error _dxf_initcache(void);
Error _dxf_initRIH (void);
Error _dxf_initloader(void);
/**
The previous functions are used in init, so prototype them here.
**/

Array _dxf_TransformArray(Matrix *mp, Camera c, Array points,
		      Array *box, Field f, int *behind, float fuzz);
Array _dxf_TransformBox(Matrix *mp, Camera c, Array points,
		    int *behind, float fuzz);
Array _dxf_TransformNormals(Matrix *mp, Array points);
/**
Transforms an array.  For ordinary arrays, may return a bounding box if
{\tt box} is not null.  Even if a bounding box is requested, it may
not be returned ({\tt *box} will be set to null) in the regular case,
because it is best just to transform the given bounding box.
**/

/* this table is now really declared in image.c */
#define NC (1<<16)			/* num entries in table */
extern unsigned char _dxd_convert[NC];	/* table accessed by unsigned short */
#define UNSIGN(i) ((i)+(1<<15))		/* offset for access by signed short */


union hl {				/* union to take apart float */
    float f;				/* here's a float */
#if defined(WORDS_BIGENDIAN)
    struct {short hi, lo;} hl;		/* two signed shorts, msb first */
#else
    struct {short lo, hi;} hl;		/* two signed shorts, lsb first */
#endif
};

/*
 * Frame buffer stuff
 */

struct fbpixel {			/* a frame buffer pixel */
    unsigned char b, g, r, a;
};
#define FBPIXEL(r,g,b) ((0xff<<24) | (r<<16) |  (g<<8) | (b<<0 ))

Field _dxf_MakeFBImage(int width, int height);
struct fbpixel *_dxf_GetFBPixels(Field i);
Field _dxf_ZeroFBPixels(Field image, int left, int right,
				int top, int bot, RGBColor c);

/*
 * Dithering stuff
 *
 * MIX(r,g,b) calculates the color table index for r,g,b;
 * 0<=r<R, 0<=g<G, 0<=b<B.
 *
 * DITH(C,c,d) dithers one color component of one pixel;
 *
 * C is number of shades of the color; C = R, G, or B
 * c is the input pixel, i.e. c = in->r, in->g, or in->b; 0<=c<C
 * d is a dither matrix entry scaled by 256; 0<=d<=(D-1)*256
 *
 * Hint: dithering produces D*(C-1)+1 shades of color C
 */

#define DX 4               /* x dimension of dither matrix */
#define DY 4               /* y dimension of dither matrix */
#define DD (DX*DY)         /* number of entries in dither matrix */

static short _dxd_dither_matrix[DY][DX] = {
    { 0 *256,  8 *256,  2 *256, 10 *256 },
    { 12 *256,  4 *256, 14 *256,  6 *256 },
    { 3 *256, 11 *256,  1 *256,  9 *256 },
    { 15 *256,  7 *256, 13 *256,  5 *256 }
};

#define MAXRGBCMAPSIZE 256
#define MAXCMAPSIZE 4096
#define MAXSIZE ((MAXRGBCMAPSIZE*4)>MAXCMAPSIZE?(MAXRGBCMAPSIZE*4):MAXCMAPSIZE)

#if defined DX_NATIVE_WINDOWS

enum translationTypeE {
	direct,
	mapped,
	rgbt,
	rgb888
};

typedef struct translationS {
	enum translationTypeE	translationType;
	HPALETTE				cmap;
	unsigned char			invertY;
	float					gamma;
	unsigned long			*rtable;
	unsigned long			*gtable;
	unsigned long			*btable;
	unsigned long			*gammaTable;
	unsigned long			*table;
	int						nColors;
	int						redShift,	redBits,	redRange;
	int						greenShift, greenBits,	greenRange;
	int						blueShift,	blueBits,	blueRange;
	int						depth;
	int						bytesPerPixel;
} translationT,*translationP;

typedef Error (*Handler)(Pointer);

Handler handler;


typedef struct window {
    HDC					hDC;			/* graphics context */
	unsigned char		*pixels;		/* the converted pixels */
    Object				pixels_owner;	/* owner of the pixels, if not us */
    int					winFlag;		/* is a X window associated with the struct? */
    HWND				hWnd;			/* window id */
	HWND				parent;			/* parent window id */
    int					windowMode;		/* is window owned by UI, another external source, or local? */
    int					refresh;		/* set to cause refresh of window */
    int					wwidth, 
						wheight, 
						wdepth;			/* current window size */
    int					pwidth, pheight;/* current image size */
    int					waitPixmap;		/* whether to wait for pixmap */
    int					waitEvent;		/* whether to wait for particular event */
    int					pixmapFull;		/* whether the pixmap is full or available for new image */
    int					error;			/* whether X error occurred */
    HBITMAP				bitmap;			/* pixmap for ui */
	void				*bmi;			/* bitmap info */
    char				*title;			/* window title */
    translationP		translation;	/* color table translation info */
    Object				translation_owner;/* object that keeps the translation */
    int					colormapTag;	/* tag of user-defined cmap (if so) */
    char				*cacheid;		/* cache id */
    Private				dpy_object;		/* object handle for display */
    int					objectTag;		/* tag of object currently in pixmap */
    int					win_invalid;	/* has the window been destroyed? */
    Handler				handler;		/* which input handler to use */
    int					active;			/* whether the window is currently active */
    int					cmap_installed;	/* is the colormap installed? */
	int					directMap;
} SWWindow;	

#else

typedef struct translationS {
    void* 	  	dpy;		/* display handle, for delete */
    void*		visual;		/* visual handle */
    Colormap		cmap;		/* color map handle */
    unsigned int	depth;	       	/* visual depth used to create 
					   translation */
    unsigned char     	invertY;	/* invert image in Y ? */
    float		gamma;		/* gamma value for gamma correction */
    enum translationTypeE {
      NoMap, 
      GrayStatic,
      ColorStatic,
      GrayMapped,
      OneMap,
      ThreeMap,
      NoTranslation 
      }	translationType;		/* flag, use table or r,g,btable */
    int			redRange;	/* number of source red colors */
    int			greenRange;	/* number of source red colors */
    int			blueRange;	/* number of source red colors */
    int		      	usefulBits;	/* how many bits per RGB are used */
    unsigned long 	*rtable;	/*map from red ramp to pixels */
    unsigned long 	*gtable;	/*map from green ramp to pixel*/
    unsigned long 	*btable;	/*map from blue ramp to pixels*/
    unsigned long 	*table;		/* map from 3 color ramp to pixels */
    unsigned long	data[MAXSIZE];
    Private		dpy_object;
    int			gammaTable[256];
    int			bytesPerPixel;
    int			ownsCmap;
} translationT,*translationP;

#endif

#define MIX(r,g,b)(((r)*gg+(g))*bb+(b))
#define RGBMIX(r,g,b) ((r) | (g) | (b))

#define DITH(C,c,d) (((unsigned)((DD*(C-1)+1)*c+d)) / (DD*256))

/*
 * The following are the recognized pixel types for translation into
 */

#define IN_BYTE_VECTOR  1
#define IN_FLOAT_VECTOR 2
#define IN_BYTE_SCALAR  3
#define IN_BIG_PIX      4
#define IN_FAST_PIX     5

Field _dxf_MakeImage(int width, int height, int depth, char *where);

Error _dxf_translateImage(Object, int, int, int, int, int, int, int, int, int, int,
			translationP, unsigned char *, int, unsigned char *);
Object _dxf_XServer(char *where);

/*
 * string table stuff 
 * (try to save memory by sharing common strings and string objects.)
 */

#if 0
#define EXTERN_DECL(x) extern char * _dxd_##x; extern Object _dxd_o_##x

EXTERN_DECL(back_colors);
EXTERN_DECL(binormals);
EXTERN_DECL(both_colors);
EXTERN_DECL(box);
EXTERN_DECL(colors);
EXTERN_DECL(color_map);
EXTERN_DECL(color_multiplier);
EXTERN_DECL(connections);
EXTERN_DECL(cubes);
EXTERN_DECL(data);
EXTERN_DECL(dep);
EXTERN_DECL(der);
EXTERN_DECL(element_type);
EXTERN_DECL(faces);
EXTERN_DECL(fb_image);
EXTERN_DECL(fontwidth);
EXTERN_DECL(front_colors);
EXTERN_DECL(image);
EXTERN_DECL(image_type);
EXTERN_DECL(inner);
EXTERN_DECL(invalid_positions);
EXTERN_DECL(invalid_connections);
EXTERN_DECL(lines);
EXTERN_DECL(neighbors);
EXTERN_DECL(normals);
EXTERN_DECL(opacities);
EXTERN_DECL(opacity_map);
EXTERN_DECL(opacity_multiplier);
EXTERN_DECL(points);
EXTERN_DECL(polylines);
EXTERN_DECL(positions);
EXTERN_DECL(quads);
EXTERN_DECL(ref);
EXTERN_DECL(surface);
EXTERN_DECL(tangents);
EXTERN_DECL(tetrahedra);
EXTERN_DECL(time);
EXTERN_DECL(tri);
EXTERN_DECL(triangles);
EXTERN_DECL(vol);
EXTERN_DECL(x_image);
EXTERN_DECL(x_server);
#else
extern char * _dxd_back_colors; extern Object _dxd_o_back_colors;
extern char * _dxd_binormals; extern Object _dxd_o_binormals;
extern char * _dxd_both_colors; extern Object _dxd_o_both_colors;
extern char * _dxd_box; extern Object _dxd_o_box;
extern char * _dxd_colors; extern Object _dxd_o_colors;
extern char * _dxd_color_map; extern Object _dxd_o_color_map;
extern char * _dxd_color_multiplier; extern Object _dxd_o_color_multiplier;
extern char * _dxd_connections; extern Object _dxd_o_connections;
extern char * _dxd_cubes; extern Object _dxd_o_cubes;
extern char * _dxd_data; extern Object _dxd_o_data;
extern char * _dxd_dep; extern Object _dxd_o_dep;
extern char * _dxd_der; extern Object _dxd_o_der;
extern char * _dxd_element_type; extern Object _dxd_o_element_type;
extern char * _dxd_faces; extern Object _dxd_o_faces;
extern char * _dxd_fb_image; extern Object _dxd_o_fb_image;
extern char * _dxd_fontwidth; extern Object _dxd_o_fontwidth;
extern char * _dxd_front_colors; extern Object _dxd_o_front_colors;
extern char * _dxd_image; extern Object _dxd_o_image;
extern char * _dxd_image_type; extern Object _dxd_o_image_type;
extern char * _dxd_inner; extern Object _dxd_o_inner;
extern char * _dxd_invalid_positions; extern Object _dxd_o_invalid_positions;
extern char * _dxd_invalid_connections; extern Object _dxd_o_invalid_connections;
extern char * _dxd_lines; extern Object _dxd_o_lines;
extern char * _dxd_neighbors; extern Object _dxd_o_neighbors;
extern char * _dxd_normals; extern Object _dxd_o_normals;
extern char * _dxd_opacities; extern Object _dxd_o_opacities;
extern char * _dxd_opacity_map; extern Object _dxd_o_opacity_map;
extern char * _dxd_opacity_multiplier; extern Object _dxd_o_opacity_multiplier;
extern char * _dxd_points; extern Object _dxd_o_points;
extern char * _dxd_polylines; extern Object _dxd_o_polylines;
extern char * _dxd_positions; extern Object _dxd_o_positions;
extern char * _dxd_quads; extern Object _dxd_o_quads;
extern char * _dxd_ref; extern Object _dxd_o_ref;
extern char * _dxd_surface; extern Object _dxd_o_surface;
extern char * _dxd_tangents; extern Object _dxd_o_tangents;
extern char * _dxd_tetrahedra; extern Object _dxd_o_tetrahedra;
extern char * _dxd_time; extern Object _dxd_o_time;
extern char * _dxd_tri; extern Object _dxd_o_tri;
extern char * _dxd_triangles; extern Object _dxd_o_triangles;
extern char * _dxd_vbox; extern Object _dxd_o_vbox;
extern char * _dxd_vol; extern Object _dxd_o_vol;
extern char * _dxd_x_image; extern Object _dxd_o_x_image;
extern char * _dxd_x_server; extern Object _dxd_o_x_server;
#endif


#define BACK_COLORS     _dxd_back_colors
#define BINORMALS	_dxd_binormals
#define BOTH_COLORS	_dxd_both_colors
#define BOX		_dxd_box
#define COLORS		_dxd_colors
#define CONNECTIONS	_dxd_connections
#define DATA		_dxd_data
#define DEP		_dxd_dep
#define DER		_dxd_der
#define ELEMENT_TYPE	_dxd_element_type
#define FACES		_dxd_faces
#define FB_IMAGE	_dxd_fb_image
#define FONTWIDTH	_dxd_fontwidth
#define FRONT_COLORS	_dxd_front_colors
#define IMAGE		_dxd_image
#define IMAGE_TYPE	_dxd_image_type
#define INNER		_dxd_inner
#define INVALID_POSITIONS _dxd_invalid_positions
#define INVALID_CONNECTIONS _dxd_invalid_connections
#define NEIGHBORS	_dxd_neighbors
#define NORMALS		_dxd_normals
#define OPACITIES	_dxd_opacities
#define POSITIONS	_dxd_positions
#define POLYLINES	_dxd_polylines
#define REF		_dxd_ref
#define SURFACE		_dxd_surface
#define TANGENTS	_dxd_tangents
#define TIME		_dxd_time
#define TRI		_dxd_tri
#define VBOX		_dxd_vbox
#define VOL		_dxd_vol
#define X_IMAGE		_dxd_x_image
#define X_SERVER	_dxd_x_server

#define O_BACK_COLORS	_dxd_o_back_colors
#define O_BINORMALS	_dxd_o_binormals
#define O_BOTH_COLORS	_dxd_o_both_colors
#define O_BOX		_dxd_o_box
#define O_COLORS	_dxd_o_colors
#define O_CONNECTIONS	_dxd_o_connections
#define O_DATA		_dxd_o_data
#define O_DEP		_dxd_o_dep
#define O_DER		_dxd_o_der
#define O_ELEMENT_TYPE	_dxd_o_element_type
#define O_FACES		_dxd_o_faces
#define O_FB_IMAGE	_dxd_o_fb_image
#define O_FONTWIDTH	_dxd_o_fontwidth
#define O_FRONT_COLORS	_dxd_o_front_colors
#define O_IMAGE		_dxd_o_image
#define O_IMAGE_TYPE	_dxd_o_image
#define O_INNER		_dxd_o_inner
#define O_INVALID_POSITIONS _dxd_o_invalid_positions
#define O_INVALID_CONNECTIONS _dxd_o_invalid_connections
#define O_NEIGHBORS	_dxd_o_neighbors
#define O_NORMALS	_dxd_o_normals
#define O_OPACITIES	_dxd_o_opacities
#define O_POLYLINES	_dxd_o_polylines
#define O_POSITIONS	_dxd_o_positions
#define O_REF		_dxd_o_ref
#define O_SURFACE	_dxd_o_surface
#define O_TANGENTS	_dxd_o_tangents
#define O_TIME		_dxd_o_time
#define O_TRI		_dxd_o_tri
#define O_VBOX		_dxd_o_vbox
#define O_VOL		_dxd_o_vol
#define O_X_IMAGE	_dxd_o_x_image
#define O_X_SERVER	_dxd_o_x_server

#define O_POINTS	_dxd_o_points
#define O_LINES		_dxd_o_lines
#define O_TRIANGLES	_dxd_o_triangles
#define O_QUADS		_dxd_o_quads
#define O_TETRAHEDRA	_dxd_o_tetrahedra
#define O_CUBES		_dxd_o_cubes

#endif /* _INTERNALS_H_ */
