/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dxmods/_helper_jea.h,v 1.7 2003/07/11 05:50:34 davidt Exp $
 */

#include <dxconfig.h>


#ifndef  __HELPER_JEA_H_
#define  __HELPER_JEA_H_


#include <dx/dx.h>


#define  TRUE   1
#define  FALSE  0


#if !defined(intelnt) && !defined(WIN32)
typedef int boolean;
#endif
#if DXD_HAS_LIBIOP
#define  CAN_HAVE_ARRAY_DASD  1
#else
#define  CAN_HAVE_ARRAY_DASD  0
#endif

#if DXD_HAS_LOCAL_MEMORY
#define  CAN_HAVE_LOCAL_MEMORY  1
#else
#define  CAN_HAVE_LOCAL_MEMORY  0
#endif


#define ErrorGotoPlus1(e,s,a)     {DXSetError(e,s,a); goto error;}
#define ErrorGotoPlus2(e,s,a,b)   {DXSetError(e,s,a,b); goto error;}
#define ErrorGotoPlus3(e,s,a,b,c) {DXSetError(e,s,a,b,c); goto error;}


typedef enum
{
    /* Placeholder for "No Such Component" */
    NULL_COMP                  = -1,

    POSITIONS_2D_COMP          = 0,
    POSITIONS_3D_COMP          = 1,
    POINT_DATA_COMP            = 2,
    POINT_COLORS_COMP          = 3,
    POINT_NORMALS_COMP         = 4,
    TETRA_NEIGHBORS_COMP       = 5,
    CUBE_NEIGHBORS_COMP        = 6,

    /* required that *_CONNECTIONS_COMP elements be contiguous */
    LINE_CONNECTIONS_COMP      = 7,
    QUAD_CONNECTIONS_COMP      = 8,
    TRIANGLE_CONNECTIONS_COMP  = 9,
    CUBE_CONNECTIONS_COMP      = 10,
    TETRA_CONNECTIONS_COMP     = 11

#if 0
    /* These are not yet implemented anywhere in SVS */
    ,
    QUAD_NEIGHBORS_COMP        = 99,
    TRIANGLE_NEIGHBORS_COMP = 99,
    FACE_CONNECTIONS_COMP      = 99,
    LOOP_CONNECTIONS_COMP      = 99,
    EDGE_CONNECTIONS_COMP      = 99
#endif 

} Component_Type;

/*
 *  The position of operation refers to location with respect to start,
 *  so *_pos(*_first) will always be 0
 *  and specifically *_pos(*_last) == *_size - 1
 *  ...which is perfect for 'c' language.
 */

#define Component_Type_first  POSITIONS_2D_COMP 
#define Component_Type_last   TETRA_CONNECTIONS_COMP
#define Component_Type_pos(a) \
    ( (int)(a) - (int)Component_Type_first )
#define Component_Type_size \
    ( (int)Component_Type_last + 1 - (int)Component_Type_first )

#define Connective_Component_Type_first  LINE_CONNECTIONS_COMP
#define Connective_Component_Type_last   TETRA_CONNECTIONS_COMP
#define Connective_Component_Type_pos(a) \
    ( (int)(a) - (int)Connective_Component_Type_first )
#define Connective_Component_Type_size  \
    ( (int)Connective_Component_Type_last + 1 - \
      (int)Connective_Component_Type_first )

char * _dxf_ClassName ( Class class );

Pointer _dxf_AllocateBestLocal ( unsigned int n );

Pointer _dxf_ReAllocateBest ( Pointer p, unsigned int n );


/* Last three arguments for next two calls are optional */
Array _dxf_NewComponentArray ( Component_Type comp_type,
                          int            *count,
                          Pointer        origin,
                          Pointer        delta );

Error _dxf_GetComponentData ( Object         in_object,
                              Component_Type comp_type,
                              int            *count,
                              Pointer        origin,
                              Pointer        delta,
                              Pointer       *ptr );

int _dxf_greater_prime ( int );

Error _dxf_ValidImageField ( Field image );
/**
\index{_dxf_ValidImageField}
      Returns {\tt OK} if {\tt image} is a Field class object
      compatible for image operations, or else returns {\tt NULL}.
**/


Field _dxf_CheckImage     ( Field image );
Error _dxf_CountImages    ( Object image, int *count );

/* Get AND Check that they match across groups */
Error _dxf_GetImageDeltas ( Object image, float *deltas );


/* Check for acceptability: std. image or transposed. */
Error _dxf_CheckImageDeltas ( Object image, int *transposed );

Field _dxf_SetImageOrigin ( Field image, int xorigin, int yorigin );
/**
\index{_dxf_SetImageOrigin}
      Sets the origin of {\tt image} to be that of the values of {\tt xorigin}
      and {\tt yorigin}.
      Returns {\tt OK} upon successful completion, or if there is an error,
       sets the Error Code and returns {\tt ERROR}.
**/

CompositeField _dxf_SetCompositeImageOrigin
                   ( CompositeField image, int xorigin, int yorigin );
/**
\index{_dxf_SetCompositeImageOrigin}
      Sets the origin of CompositeField image {\tt image} to be that of
      {\tt xorigin} and {\tt yorigin}.
      Returns pointer to {\tt image} upon success or if there is an error,
       sets the Error Code and returns {\tt ERROR}.
**/


Field _dxf_SimplifyCompositeImage ( CompositeField image );
/**
\index{_dxf_SimplifyCompositeImage}
      Takes the pixel contents of CompositeField {\tt image} and simplifies
      them to be a simple Field object.
      Returns pointer to the constructed Field upon successful completion,
      or if there is an error, sets the Error Code and returns {\tt ERROR}.
**/


/*
 * for 'infield', calculate and set a "fuzz" component.
 * 'type' values progress as follows:
 *      1 for connections
 *      2 for positions
 */
Field _dxf_SetFuzzAttribute ( Field infield, int type );

#define  VALUE_UNSPECIFIED  -1

typedef struct sizedata
{
    int height;
    int width;
    int startframe;
    int endframe;
}
SizeData;


Array _dxf_CopyArray_jea ( Array in, enum _dxd_copy copy );

Field _dxf_GetEnumeratedImage ( Object o, int n );

SizeData * _dxf_GetImageAttributes ( Object o, SizeData *sd );


/********************************************************************\
 * _dxf_OutputX and OutputGL reside in separate files, not _helper_jea.c *
 * _dxf_OutputX will likely be integrated into libsvs                    *
 * OutputGL will likely be deleted                                  *
\********************************************************************/

Field _dxf_MakeFieldEmpty ( Field infield );



/*
 * If a color is not in place, create a regular one with the setting specified.
 */
Field _dxf_SetDefaultColor ( Field input, RGBColor color );


Error _dxf_OutputX ( Object image, char *XServerName, int XWindowID );
/**
\index{_dxf_OutputX}
      Display {\tt image} on optional 'X' windows server {\tt XServerName}.
      If {\tt XWindowID} is nonzero, then use a window that has already been
      created, perhaps by another application.
      Returns {\tt OK} upon successful completion, or if there is an error,
      sets the Error Code and returns {\tt ERROR}.
**/


/* calls operating on either nonspecific group or Series group */

/*
 * Turns nested CompositeFields into single level ones.
 * Turns a Field into a Field (for callers convenience).
 */
Object _dxf_FlattenHierarchy ( Object in );

Group _dxf_SetMember_G_S ( Group g, int n, char *name, float position, Object o );

Object _dxf_GetEnumeratedMember_G_S ( Group g, int n, char **name, float *position );


#endif /* __HELPER_JEA_H_ */



