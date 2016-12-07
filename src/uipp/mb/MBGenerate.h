/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>



#ifdef TRUE
#undef TRUE
#endif
#ifdef FALSE
#undef FALSE
#endif

#if defined(intelnt)
#define    DOUBLE	DX_DOUBLE
#define    FLOAT	DX_FLOAT
#define    INT		DX_INT
#define    UINT		DX_UINT
#define    SHORT	DX_SHORT
#define    USHORT	DX_USHORT
#define    BYTE		DX_BYTE
#define    UBYTE	DX_UBYTE
#define    STRING	DX_STRING
#define    NO_DATATYPE	DX_NO_DATATYPE
#endif

enum truefalse
{
    TRUE,
    FALSE,
    UNKNOWN
};

enum language
{
    C,
    FORTRAN
};


typedef struct
{
    char               *name;
    char               *category;
    char               *description;
    char               *outboard_host;
    char               *outboard_executable;
    char               *loadable_executable;
    char               *include_file;
    enum language 	language;
    enum truefalse      outboard_persistent;
    enum truefalse      asynchronous;
    enum truefalse      pinned;
    enum truefalse      side_effect;
} Module;

enum structure
{
    VALUE,
    GROUP_FIELD,
    NO_STRUCTURE
};

enum datatype
{
    DOUBLE, 
    FLOAT, 
    INT,
    UINT,
    SHORT,
    USHORT,
    BYTE,
    UBYTE,
    STRING,
    NO_DATATYPE
};

enum datashape
{
    SCALAR,
    VECTOR_1,
    VECTOR_2,
    VECTOR_3,
    VECTOR_4,
    VECTOR_5,
    VECTOR_6,
    VECTOR_7,
    VECTOR_8,
    VECTOR_9,
    NO_DATASHAPE
};

enum counts
{
    COUNTS_1,
    COUNTS_SAME_AS_INPUT,
    NO_COUNTS
};

enum gridstructure 
{
    GRID_REGULAR,
    GRID_IRREGULAR,
    GRID_NOT_REQUIRED
};

enum dependency 
{
    DEP_POSITIONS,
    DEP_CONNECTIONS,
    DEP_INPUT,
    DEP_NONE,
    NO_DEPENDENCY
};

enum elementtype
{
    ELT_LINES,
    ELT_TRIANGLES,
    ELT_QUADS,
    ELT_TETRAHEDRA,
    ELT_CUBES,
    ELT_NOT_REQUIRED 
};

typedef struct
{
    char               *name;
    char               *description;
    enum truefalse      required;
    char               *default_value;
    enum truefalse      descriptive;
    char               *types;
    enum structure      structure;
    enum datatype       datatype;
    enum datashape      datashape;
    enum counts         counts;
    enum gridstructure  positions;
    enum gridstructure  connections;
    enum dependency     dependency;
    enum elementtype    elementtype;
} Parameter;


