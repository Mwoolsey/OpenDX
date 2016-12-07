/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>

/*
 * stub code for making DXlite link ok without rendering included.
 */

#include <dx/dx.h>
#include "arrayClass.h"

#define DISCLAIMER(what)  DXSetError(ERROR_BAD_CLASS, \
		         "%s not available with this version of the library", \
			  what) 


Error _dxfField_Shade(Field f, struct shade *old)
{
    DISCLAIMER("Rendering");
    return ERROR;
}
Error _dxfXform_Shade(Xform x, struct shade *old)
{
    DISCLAIMER("Rendering");
    return ERROR;
}
Error _dxfScreen_Shade(Screen s, struct shade *old)
{
    DISCLAIMER("Rendering");
    return ERROR;
}
Error _dxfGroup_Shade(Group g, struct shade *old)
{
    DISCLAIMER("Rendering");
    return ERROR;
}
Error _dxfLight_Shade(Light l, struct shade *shade)
{
    DISCLAIMER("Rendering");
    return ERROR;
}
Error _dxfClipped_Shade(Clipped c, struct shade *old)
{
    DISCLAIMER("Rendering");
    return ERROR;
}

Object _dxfField_Gather(Field f, struct gather *gather, struct tile *tile)
{
    DISCLAIMER("Rendering");
    return NULL;
}
Object _dxfGroup_Gather(Group g, struct gather *gather, struct tile *tile)
{
    DISCLAIMER("Rendering");
    return NULL;
}
Object _dxfLight_Gather(Light l, struct gather *gather, struct tile *tile)
{
    DISCLAIMER("Rendering");
    return NULL;
}
Object _dxfXform_Gather(Xform x, struct gather *gather, struct tile *tile)
{
    DISCLAIMER("Rendering");
    return NULL;
}
Object _dxfScreen_Gather(Screen s, struct gather *gather, struct tile *tile)
{
    DISCLAIMER("Rendering");
    return NULL;
}
Object _dxfClipped_Gather(Clipped clipped, struct gather *gather, struct tile *tile)
{
    DISCLAIMER("Rendering");
    return NULL;
}

Object _dxfField_Paint(Field f, struct buffer *b, int clip_status, struct tile *tile)
{
    DISCLAIMER("Rendering");
    return NULL;
}
Object _dxfGroup_Paint(Group g, struct buffer *b, int clip_status, struct tile *tile)
{
    DISCLAIMER("Rendering");
    return NULL;
}
Object _dxfLight_Paint(Light l, struct buffer *b, int clip_status)
{
    DISCLAIMER("Rendering");
    return NULL;
}
Object _dxfXform_Paint(Xform x, struct buffer *b, int clip_status, struct tile *tile)
{
    DISCLAIMER("Rendering");
    return NULL;
}
Object _dxfScreen_Paint(Screen s, struct buffer *b, int clip_status, struct tile *tile)
{
    DISCLAIMER("Rendering");
    return NULL;
}
Object _dxfClipped_Paint(Clipped clipped, struct buffer *b, int clip_status,
	       struct tile *tile)
{
    DISCLAIMER("Rendering");
    return NULL;
}

Group DXPartition(Field f, int n, int size)
{
    DISCLAIMER("Partitioning");
    return NULL;
}
Error _dxfField_Partition(Field f, int *n, int size, Object *o, int delete)
{
    DISCLAIMER("Partitioning");
    return ERROR;
}

/* This routine is only here because it's needed for libDX but 
 * not for libDXcallm.
 *
 * stub to shut off run-time loading.
 */
Error _dxf_initloader(void) { return OK; }
