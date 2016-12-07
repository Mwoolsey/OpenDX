/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dxmods/_isosurface.h,v 1.4 2000/08/24 20:04:15 davidt Exp $
 */

#include <dxconfig.h>



#ifndef  __ISOSURFACE_H_
#define  __ISOSURFACE_H_

#include "_getfield.h"

/* requires inclusion of <dx/dx.h> */

extern float *_dxd_user_def_values;

typedef enum { ISOSURFACE, BAND }
     iso_module_types;

typedef enum { DATA_LOW, DATA_HIGH, ORIGINAL_DATA }
     band_remap_types;

typedef enum { NO_NORMALS, GRADIENT_NORMALS, NORMALS_COMPUTED }
     iso_norm_types;


typedef struct
{
    iso_module_types module;
    iso_norm_types   normal_type;
    int              normal_direction;
    Interpolator     gradient_interpolator;
    band_remap_types remap;
        /*
         * if ( number == 1 && use_mean )
         *     default to mean, else default using min/max
         */
    Object      self;
    char        *member_name;
    int         is_grown;
    int         use_mean;
    int         number;
    int         series_ordinal;
    float       series_FP_val;
    float       *isovals;
    int         have_minmax;
    float       band_min_cf;
    float       band_max_cf;
    Object      *parent;
    int         task_counter;
} iso_arg_type; 



/*-------------------------------------------------------------------------*
 *  Body of these routines are located in file:
 *     _isosurface.c
 *
 * XXX - externs not declared here: 
 *          _dxf_IsFlippedTet_3x3_method, _dxf_IsFlippedTet_4x4_method
 */

/* 
 * Special traverser for isosurface.
 */
Object _dxf_IsosurfaceObject ( iso_arg_type );
int _dxf_get_flip ( field_info, int *, int * );


#endif /* __ISOSURFACE_H_ */
