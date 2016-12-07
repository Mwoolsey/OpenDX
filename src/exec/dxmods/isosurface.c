/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dxmods/isosurface.c,v 1.6 2003/07/11 05:50:35 davidt Exp $
 */

#include <dxconfig.h>


/***
MODULE:
    Isosurface
SHORTDESCRIPTION:
    computes an isosurface
CATEGORY:
    Realization
INPUTS:
    data;      scalar field;   NULL;            field to take isosurface(s) of
    value;     scalar list;    data mean;       isosurface value(s)
    number;    integer;        1;              number of isosurfaces or contours
    gradient;  vector field;   NULL;            gradient field
    flag;      integer;        input dependent; compute normals
    direction; integer;        -1;              how to orient normals
OUTPUTS:
    surface;   field or group; NULL;            isosurface
FLAGS:
BUGS:
    This routine will attempt to allocate all of the memory needed to
    perform its function first.  For large volumes, ( n * m * o >> 20,000 ),
    this will likely exceed available memory.
    Only floating point types are supported for the data field.
    Returns NULL if the isovalue lies outside of the data's range.
    Should return a Field with no points or triangles.
AUTHOR:
    John E. Allain, David A. Epstein
END:
***/

/*
 * This is adapted from:
 *
 * "A High-Speed Image Display Algorithm for 3D Grid Data"
 * Akio Doi and Akio Koide, Tokyo Research Laboratory IBM Japan, Ltd.
 */

#include <stdio.h>
#include <string.h>
#include <dx/dx.h>
#include "_isosurface.h"


#define DXErrorGoto4(e,s,a,b,c) {DXSetError(e,s,a,b,c); goto error;}



float *_dxd_user_def_values = NULL;
int   _dxd_isosurface_task_counter = 0;

static iso_arg_type ISO_ARG_INITIALIZER =
            { ISOSURFACE, NO_NORMALS,
             -1, NULL, ORIGINAL_DATA, NULL, NULL, 0, 0, 0,
             -1, -1.0, NULL, 0, DXD_MAX_FLOAT, -DXD_MAX_FLOAT, NULL, 0 };


Error
m_Isosurface ( Object *in,  Object *out )
{

#define  I_data        in[0]
#define  I_value       in[1]
#define  I_number      in[2]
#define  I_gradient    in[3]
#define  I_flag        in[4]
#define  I_direction   in[5]
#define  O_surface     out[0]

    int          add_normals;
    iso_arg_type iso_arg;
    Class        class;

    Type         grad_type;
    Category     grad_cat;
    int          grad_rank;
    int          grad_shape[8];

    O_surface            = NULL;
    _dxd_user_def_values = NULL;
    iso_arg              = ISO_ARG_INITIALIZER;

    if ( !I_data )
        DXErrorGoto2
            ( ERROR_MISSING_DATA,
              "#10000", /* %s must be specified */ "'data' parameter" );

    iso_arg.self = I_data;

    if ( !I_value )
    {
        iso_arg.isovals = NULL;

        if ( !I_number )
        {
            iso_arg.number   = 1;
            iso_arg.use_mean = 1;
        }
        else
        {
            if ( !DXExtractInteger ( I_number, &iso_arg.number )
                 ||
                 ( iso_arg.number < 1 ) )
                DXErrorGoto2
                    ( ERROR_BAD_PARAMETER, 
                      "#10020", /* %s must be a positive integer */
                      "'number' parameter" );

            iso_arg.use_mean = 0;
        }
    }
    else 
    {
        iso_arg.use_mean = 0;

        if ( !DXQueryParameter ( I_value, TYPE_FLOAT, 1, &iso_arg.number ) )
            DXErrorGoto2
                ( ERROR_BAD_PARAMETER,
                  "#10130", /* %s must be a scalar or scalar list */
                  "'value' parameter" );

        if ( ERROR == ( _dxd_user_def_values
                            = (float *) DXAllocate
                                          ( sizeof(float) * iso_arg.number ) )
             ||
             !DXExtractParameter
                ( I_value, TYPE_FLOAT, 1, iso_arg.number,
                  (Pointer)_dxd_user_def_values ) )
            goto error;

        if ( I_number )
            DXWarning
                ( "#4110", /* %s has been specified; %s assignment ignored */
                  "'value' parameter", "'number'" );

        iso_arg.isovals = _dxd_user_def_values;
    }

    if ( I_flag )
    {
        if ( !DXExtractInteger ( I_flag, &add_normals )
             ||
             ( add_normals < 0 )
             ||
             ( add_normals > 1 ) )
            DXErrorGoto2
                ( ERROR_BAD_PARAMETER,
                  "#10070", /* %s must be either 0 or 1 */ "'flag' parameter" )

        else if ( !add_normals && I_gradient )
            DXWarning
               ( "#4110", /* %s has been specified; %s assignment ignored */
                 "'flag' = 0", "'gradient' parameter" );
    }
    else 
        add_normals = 1;


    if ( add_normals && I_gradient )
    {
        /* If not a Field and not a CompositeField */
        if ( ( ( class = DXGetObjectClass ( I_gradient ) ) != CLASS_FIELD )
             &&
             ( ( class != CLASS_GROUP )
               ||
               ( DXGetGroupClass ( (Group)I_gradient )
                   != CLASS_COMPOSITEFIELD ) ) )
            DXErrorGoto2
                ( ERROR_DATA_INVALID,
                  "#10191", /* %s must be a field */ "'gradient'" );

        if ( !DXMapCheck
                  ( I_data, I_gradient, "positions",
                    &grad_type, &grad_cat, &grad_rank, grad_shape ) )
        {
            DXAddMessage
                ( "#10310", /* %s does not match hierarchy of %s */
                  "'gradient' parameter", "'data'" );
            goto error;
        }

        if ( ( grad_type != TYPE_FLOAT ) || ( grad_cat != CATEGORY_REAL )
             ||
             ( grad_rank != 1 ) || ( grad_shape[0] != 3 ) )
            DXErrorGoto3
                ( ERROR_DATA_INVALID,
                  "#10230", /* %s must be a %d-vector */
                  "'gradient' parameter data", 3 );

        if ( ERROR == ( iso_arg.gradient_interpolator
                            = DXNewInterpolator
                                  ( I_gradient, INTERP_INIT_PARALLEL, -1.0 ) ) )
        {
            DXAddMessage
                ( "Some types of external Gradient are not supported." );
            DXAddMessage
                ( "Try removing the 'gradient' parameter setting." );

            goto error;
        }
    }
    else 
        /*
         * Note that the gradient interpolator is not created
         * if flag specifically requests not to use gradient
         */
        iso_arg.gradient_interpolator = NULL;

    iso_arg.normal_type
        = ( add_normals )
             ? ( ( iso_arg.gradient_interpolator )
                    ? GRADIENT_NORMALS
                    : NORMALS_COMPUTED )
             : NO_NORMALS;

    if ( I_direction )
    {
        if ( !DXExtractInteger ( I_direction, &iso_arg.normal_direction )
             ||
             ( ( iso_arg.normal_direction != -1 )
               &&
               ( iso_arg.normal_direction !=  1 ) ) )
            DXErrorGoto4
                ( ERROR_BAD_PARAMETER,
                  "#10071", /* %s must be either %d or %d */
                  "'direction' parameter", -1, 1 )
    }
    else
        iso_arg.normal_direction = -1;

    if ( !( O_surface = _dxf_IsosurfaceObject ( iso_arg ) ) 
         ||
         ( DXGetError() != ERROR_NONE ) )
        goto error;

    DXFree   ( (Pointer) _dxd_user_def_values );
    DXDelete ( (Object)  iso_arg.gradient_interpolator );

    _dxd_user_def_values = NULL;

    return OK;

    error:
        if ( O_surface ) DXDelete ( O_surface );

        DXFree   ( (Pointer) _dxd_user_def_values );
        DXDelete ( (Object) iso_arg.gradient_interpolator );

        _dxd_user_def_values = NULL;
        O_surface            = NULL;

        DXASSERT ( DXGetError() != ERROR_NONE );
        return ERROR;
}
