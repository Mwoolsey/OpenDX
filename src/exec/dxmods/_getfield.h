/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dxmods/_getfield.h,v 1.5 2001/04/06 16:59:15 davidt Exp $
 */

#include <dxconfig.h>


#ifndef  __GETFIELD_H_
#define  __GETFIELD_H_

#include <dx/dx.h>


typedef enum
{
    LINES, 
    TRIANGLES,
    QUADRILATERALS,
    TETRAHEDRA,
    CUBES,
    PRISMS,
    CUBES4D,
    NONE,
    ET_UNKNOWN
}
std_connection_element_types; 


typedef enum
{
    POSITIONS           = 0,
    CONNECTIONS         = 1,
    DATA                = 2,
    COLORS              = 3,
    OPACITY             = 4,
    NORMALS             = 5,
    NEIGHBORS           = 6,
    BACK_COLORS         = 7,
    FRONT_COLORS        = 8,
    ORIGINAL_POSITIONS  = 9,
    INVALID_POSITIONS   = 10,
    INVALID_CONNECTIONS = 11,
    COLOR_MAP           = 12,
    NOT_A_STANDARD      = 99
}
std_component_names;
#define  STD_COMPONENT_LIM  13


typedef enum
{
    DEP          = 0,
    REF          = 1,
    DER          = 2,
    DX_ELEMENT_TYPE = 3
}
std_component_attribute_names;
#define  STD_COMPONENT_ATTRIBUTE_LIM  4


typedef enum
{
    FFALSE = 0,
    TTRUE  = 1
}
bboolean;


/* ------------------------------------------------------------------------- */
typedef struct  _attribute_info
{
    /*
     * XXX - field fuzz value is type float
     */
    char   *name;   /* DXGetEnumeratedAttribute */
    char   *value;  /* DXGetString */
}
*attribute_info;


/* ------------------------------------------------------------------------- */
typedef struct  _array_info
{
    /*
     * array base class
     *
     *  This is a superclass which is interpreted as follows:
     *
     *  array class           grid    typecast to use
     *  ------------------    ----    --------------------
     *  CLASS_ARRAY:                  array_info
     *  CLASS_REGULARARRAY:           regular_array_info
     *  CLASS_PATHARRAY:              path_array_info
     *  CLASS_PRODUCTARRAY:   FALSE:  product_array_info
     *  CLASS_PRODUCTARRAY,   TRUE:   grid_positions_info
     *  CLASS_MESHARRAY,      FALSE:  mesh_array_info
     *  CLASS_MESHARRAY,      TRUE:   grid_connections_info
     */

    Class     class;          /* DXGetArrayClass */
    Array     array;
    int       items;          /* DXGetArrayInfo */
    Type      type;           /* DXGetArrayInfo */
    Category  category;       /* DXGetArrayInfo */
    int       rank;           /* DXGetArrayInfo */
    int       *shape;         /* DXGetArrayInfo */
    Pointer   data;           /* DXGetArrayData */
    int       itemsize;       /* DXGetItemSize  */
    int       original_items; /* DXQueryOriginalSizes (from DXGrow) */
    Error     (*get_item) (); /* ( int, array_info, Pointer ); */
    ArrayHandle  handle;      /* DXCreateArrayHandle */
              /* requires component 'class' information to work */
#if 0
    Pointer   data_local;     /* DXGetArrayDataLocal */
#endif
}
*array_info;


/* ------------------------------------------------------------------------- */
typedef struct  _regular_array_info
{
    struct _array_info   array;  /* subclass of array */

    /*
     * regular array subclass
     * 
     * reminder: origin and delta can be Point, char, ... 
     *    since regular arrays aren't used just for positions
     */

#if 0
    int      count;    /* DXGetRegularArrayInfo */
#endif
    Pointer  origin;   /* DXGetRegularArrayInfo, DXQueryConstantArray */
    Pointer  delta;    /* DXGetRegularArrayInfo */
}
*regular_array_info;


/* ------------------------------------------------------------------------- */
typedef struct  _path_array_info
{
    struct _array_info   array;  /* subclass of array */

    /*
     * path array subclass (connections)
     */

#if 0 
    int  count;   /* DXGetPathArrayInfo */
#endif
    int  offset;  /* DXGetPathOffset */
}
*path_array_info;


/* ------------------------------------------------------------------------- */
typedef struct  _product_array_info
{
    struct _array_info   array;  /* subclass of array */

    /*
     * product array subclass (positions)
     */

    bboolean    grid;   /* DXQueryGridPositions */
    int         n;      /* DXGetProductArrayInfo, number of terms */
    array_info  terms;  /* in_memory_array */
}
*product_array_info;


/* ------------------------------------------------------------------------- */
typedef struct  _mesh_array_info
{
    struct _array_info   array;  /* subclass of array */

    /*
     * mesh array subclass (connections product)
     *
     *   XXX - offsets are set for grids only
     */

    bboolean    grid;          /* DXQueryGridConnections */
    int         n;             /* DXGetMeshArrayInfo, number of terms */
    array_info  terms;         /* in_memory_array */
    int        *offsets;       /* DXGetMeshOffsets */
    int        *grow_offsets;  /* DXQueryOriginalMeshExtents */
}
*mesh_array_info;


/* ------------------------------------------------------------------------- */
typedef struct  _grid_connections_info
{
    struct _mesh_array_info   mesh;  /* subclass of mesh array */

    /*
     * grid connections subclass
     */

    int  *counts;  /* DXQueryGridConnections */
}
*grid_connections_info;


/* ------------------------------------------------------------------------- */
typedef struct  _grid_positions_info
{
    struct _product_array_info   prod;  /* subclass of product array */

    /*
     * grid positions subclass
     */

#if 0
    int      grid_dim;  /* DXQueryGridPositions */
#endif
    int      *counts;   /* DXQueryGridPositions */
    Pointer  origin;    /* DXQueryGridPositions */
    Pointer  deltas;    /* DXQueryGridPositions */
}
*grid_positions_info;


/* ------------------------------------------------------------------------- */
struct _array_allocator
{
    /* 
     * The 'allocator' type is required for sub/super classed types.
     * Be sure to allocate and increment using the full frame size,
     *   based on the largest class in the hierarchy.
     * 
     * XXX - the exact size of the pad should be carefully considered
     */
    struct _grid_positions_info  contents;   /* in_memory_array */
    int  XXX_safety_pad[10];
};


/* ------------------------------------------------------------------------- */
typedef struct  _component_info
{
    struct _array_allocator array; /* components contain arrays assumption */

    /*
     * component 
     * 
     * Assumptions:
     *   Component objects will always be Arrays.
     *   Component attributes will always be Strings.
     */
    char       *name;         /* DXGetEnumeratedComponentValue  */
    int        attrib_count;  /* DXGetEnumeratedComponentValue  */

    std_component_names           std_name;
    attribute_info std_attribs [ STD_COMPONENT_ATTRIBUTE_LIM ];
    std_connection_element_types  element_type;
    attribute_info                attrib_list;   /* in_memory_attribute */
}
*component_info;


/* ------------------------------------------------------------------------- */
typedef struct  _field_info
{
    /*
     * field
     *
     * XXX - conveniently ignore field attributes
     */
    Field           field;
    component_info  std_comps [ STD_COMPONENT_LIM ];
    component_info  comp_list;   /* in_memory_component */
    int             comp_count;  /* DXGetEnumeratedComponentValue */
}
*field_info;


/* ------------------------------------------------------------------------- */

typedef struct { PointId  p, q, r, s, t, u; } Prism;

typedef struct { float    x;          } Point1D;
typedef struct { float    x, y;       } Point2D;
typedef struct { Point1D  du;         } Deltas1D;
typedef struct { Point2D  du, dv;     } Deltas2D;
typedef struct { Point    du, dv, dw; } Deltas3D;

/* XXX - Anyone for 4D ? */


typedef struct { int i[2]; } neighbor2;  /* Neighbors for Lines */
typedef struct { int i[3]; } neighbor3;  /* Neighbors for Triangles */
typedef struct { int i[4]; } neighbor4;  /* Neighbors for Quads, Tetrahedra */
typedef struct { int i[5]; } neighbor5;  /* Neighbors for Prisms */
typedef struct { int i[6]; } neighbor6;  /* Neighbors for Cubes */


typedef struct _mesh_bounds   /* augmented mesh offsets */
{
    int  i_min, j_min, k_min;
    int  i_max, j_max, k_max;
}
*mesh_bounds;

/* ----------------------------- extern section ----------------------------- */


field_info     _dxf_InMemory     ( Field          input );
Error          _dxf_FreeInMemory ( field_info     input );
component_info _dxf_SetIterator  ( component_info input );
component_info _dxf_FindCompInfo ( field_info     input, char *locate );
 
/* Isosurface data, cast to float */
component_info _dxf_SetIterator_i ( component_info input );

/*
 * Stupid Q:
 *   If these are accessed by func ptr in struct, then can they be statics?
 */

Error _dxf_get_item_by_handle  ( int index, array_info d, Pointer out );

/* Declarations of the Generics to be instantiated */

Error _dxf_get_data_irr_BYTE   ( int index, array_info d, float *out );
Error _dxf_get_data_irr_DOUBLE ( int index, array_info d, float *out );
Error _dxf_get_data_irr_FLOAT  ( int index, array_info d, float *out );
Error _dxf_get_data_irr_HYPER  ( int index, array_info d, float *out );
Error _dxf_get_data_irr_INT    ( int index, array_info d, float *out );
Error _dxf_get_data_irr_SHORT  ( int index, array_info d, float *out );
Error _dxf_get_data_irr_UBYTE  ( int index, array_info d, float *out );
Error _dxf_get_data_irr_UINT   ( int index, array_info d, float *out );
Error _dxf_get_data_irr_USHORT ( int index, array_info d, float *out );

Error _dxf_get_data_reg_BYTE   ( int index, array_info d, float *out );
Error _dxf_get_data_reg_DOUBLE ( int index, array_info d, float *out );
Error _dxf_get_data_reg_FLOAT  ( int index, array_info d, float *out );
Error _dxf_get_data_reg_HYPER  ( int index, array_info d, float *out );
Error _dxf_get_data_reg_INT    ( int index, array_info d, float *out );
Error _dxf_get_data_reg_SHORT  ( int index, array_info d, float *out );
Error _dxf_get_data_reg_UBYTE  ( int index, array_info d, float *out );
Error _dxf_get_data_reg_UINT   ( int index, array_info d, float *out );
Error _dxf_get_data_reg_USHORT ( int index, array_info d, float *out );

Error _dxf_get_point_irr_1D  ( int index, array_info c, Point1D *out );
Error _dxf_get_point_grid_1D ( int index, array_info c, Point1D *out );
Error _dxf_get_point_irr_2D  ( int index, array_info c, Point2D *out );
Error _dxf_get_point_grid_2D ( int index, array_info c, Point2D *out );
Error _dxf_get_point_irr_3D  ( int index, array_info c, Point   *out );
Error _dxf_get_point_grid_3D ( int index, array_info c, Point   *out );

Error _dxf_get_conn_grid_LINES
                 ( int index, array_info c, Line          *out );
Error _dxf_get_conn_irr_LINES 
                 ( int index, array_info c, Line          *out );
Error _dxf_get_conn_grid_QUADS
                 ( int index, array_info c, Quadrilateral *out );
Error _dxf_get_conn_irr_QUADS 
                 ( int index, array_info c, Quadrilateral *out );
Error _dxf_get_conn_irr_TRIS  
                 ( int index, array_info c, Triangle      *out );
Error _dxf_get_conn_grid_CUBES
                 ( int index, array_info c, Cube          *out );
Error _dxf_get_conn_irr_CUBES 
                 ( int index, array_info c, Cube          *out );
Error _dxf_get_conn_irr_TETS  
                 ( int index, array_info c, Tetrahedron   *out );
Error _dxf_get_conn_irr_PRSMS 
                 ( int index, array_info c, Prism         *out );

Error _dxf_get_neighb_irr_LINES
                 ( int index, array_info n, neighbor2 *out );
Error _dxf_get_neighb_irr_QUADS
                 ( int index, array_info n, neighbor4 *out );
Error _dxf_get_neighb_irr_TRIS
                 ( int index, array_info n, neighbor3 *out );
Error _dxf_get_neighb_irr_TETS
                 ( int index, array_info n, neighbor4 *out );
Error _dxf_get_neighb_irr_CUBES
                 ( int index, array_info n, neighbor6 *out );
Error _dxf_get_neighb_irr_PRSMS
                 ( int index, array_info n, neighbor5 *out );

Error _dxf_get_neighb_grid_LINES
                 ( int index, array_info c, mesh_bounds b, neighbor2 *out );
Error _dxf_get_neighb_grid_QUADS
                 ( int index, array_info c, mesh_bounds b, neighbor4 *out );
Error _dxf_get_neighb_grid_CUBES
                 ( int index, array_info c, mesh_bounds b, neighbor6 *out );

#endif /* __GETFIELD_H_ */
