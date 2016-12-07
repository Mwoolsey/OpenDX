/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dxmods/band.c,v 1.4 2000/08/24 20:04:24 davidt Exp $
 */

#include <dxconfig.h>


#include <string.h>
#include <dx/dx.h>
#include "_helper_jea.h"
#include "_isosurface.h"

static iso_arg_type ISO_ARG_INITIALIZER =
            { ISOSURFACE, NO_NORMALS,
             -1, NULL, ORIGINAL_DATA, NULL, NULL, 0, 0, 0,
             -1, -1.0, NULL, 0, DXD_MAX_FLOAT, -DXD_MAX_FLOAT, NULL, 0 };

Error
m_Band ( Object *in, Object *out )
{

#define  I_data    in[0]
#define  I_value   in[1]
#define  I_number  in[2]
#define  I_remap   in[3]
#define  O_band    out[0]

    iso_arg_type iso_arg;
    char         *remap_string;

    O_band               = NULL;

    _dxd_user_def_values = NULL;
    iso_arg              = ISO_ARG_INITIALIZER;
    iso_arg.module       = BAND;
    iso_arg.normal_type  = NO_NORMALS;

    if ( !I_data )
        ErrorGotoPlus1
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
                ErrorGotoPlus1
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
            ErrorGotoPlus1
                ( ERROR_BAD_PARAMETER,
                  "#10130", /* %s must be a scalar or scalar list */
                  "'value' parameter" );

        if ( ERROR == ( _dxd_user_def_values
                            = (float *) DXAllocate
                                          ( sizeof(float) *
                                            (iso_arg.number + 1) ) )
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

    if ( I_remap )
    {
        if ( !DXExtractString ( I_remap, &remap_string ) )
        {
            DXSetError
                ( ERROR_BAD_PARAMETER,
                  "#10200", /* %s must be a string */
                  "'remap'" );
            goto error;
        }

        if ( 0 == strcmp ( remap_string, "original" ) )
            iso_arg.remap = ORIGINAL_DATA;

        else if ( 0 == strcmp ( remap_string, "low" ) )
            iso_arg.remap = DATA_LOW;

        else if ( 0 == strcmp ( remap_string, "high" ) )
            iso_arg.remap = DATA_HIGH;

        else 
            ErrorGotoPlus2
                ( ERROR_BAD_PARAMETER,
                  "#10211", /* %s must be one of the following strings: %s */
                  "'remap' parameter",
                  "\"original\", \"low\", or \"high\"" )
    }
    else
        iso_arg.remap = DATA_LOW;

    if ( ( ERROR == ( O_band = _dxf_IsosurfaceObject ( iso_arg ) ) )
         ||
         ( DXGetError() != ERROR_NONE ) )
        goto error;

    DXFree   ( (Pointer) _dxd_user_def_values );
    DXDelete ( (Object)  iso_arg.gradient_interpolator );

    _dxd_user_def_values = NULL;

    return OK;

    error:
        if ( O_band ) DXDelete ( O_band );

        DXFree ( (Pointer) _dxd_user_def_values );

        _dxd_user_def_values = NULL;
        O_band               = NULL;

        DXASSERT ( DXGetError() != ERROR_NONE );
        return ERROR;
}
