/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dxmods/showbox.c,v 1.4 1999/07/12 22:32:57 gda Exp $
 */

#include <dxconfig.h>



#include <dx/dx.h>
#include "bounds.h" 


#define MAXDIM 16
#define BOXCOLOR  DXRGB(0.7, 0.7, 0.0) 

int
m_ShowBox(Object *in, Object *out)
{
    Field f = NULL;
    Array a = NULL;
    RGBColor color, delta;
    Point box[MAXDIM], center;
    Line *lp;
    Class class;
    
    out[0] = NULL;
    out[1] = NULL;

    if (!in[0]) {
	DXSetError(ERROR_BAD_PARAMETER, "#10000", "input");
	return ERROR;
    }

    class = DXGetObjectClass(in[0]);

    if ((class == CLASS_FIELD) && DXEmptyField((Field)in[0])) {
	out[0] = in[0];
	return OK;
    }

    if ((class == CLASS_ARRAY) || (class == CLASS_STRING))
	DXErrorGoto2
            ( ERROR_BAD_PARAMETER,
              "#10190", /* %s must be a field or a group */
              "the `input' parameter" );

    if (!DXValidPositionsBoundingBox(in[0], box)) {
	if (DXGetError() != ERROR_NONE)
	    return ERROR;

	DXWarning("input object has no bounding box");
	DXResetError();
	out[0] = (Object)DXEndField(DXNewField());
	return OK;
    }



    if (!(f = DXNewField()))
	return ERROR;

#define NUM_3D_CORNERS 8

    if (!DXAddPoints(f, 0, NUM_3D_CORNERS, box))
	goto error;


    /* set default colors, using regular (compact) array */
    color = BOXCOLOR;
    delta = DXRGB(0.0, 0.0, 0.0);
    if(!(a = (Array)DXNewRegularArray(TYPE_FLOAT, 3, NUM_3D_CORNERS, 
	                               (Pointer)&color, (Pointer)&delta)))
	goto error;


    if (!DXSetStringAttribute((Object)a, "dep", "positions"))
	goto error;

    /* if this succeeds, then 'a' is now part of 'f', and will be deleted 
     * when 'f' is deleted.  set 'a' to NULL to prevent duplicate deletions
     * on error.
     */
    if (!DXSetComponentValue(f, "colors", (Object)a))
	goto error;
    else
	a = NULL;


    /* now set lines */
    if(!(a = DXNewArray(TYPE_INT, CATEGORY_REAL, 1, 2)))
	goto error;
    
    if (!DXAddArrayData(a, 0, 12, NULL))
	goto error;
    
    
    lp = (Line *)DXGetArrayData(a);
    
    lp->p = 0;    lp->q = 1;    lp++;
    lp->p = 0;    lp->q = 2;    lp++;
    lp->p = 1;    lp->q = 3;    lp++;
    lp->p = 2;    lp->q = 3;    lp++;
    
    lp->p = 4;	  lp->q = 5;    lp++;
    lp->p = 4;	  lp->q = 6;    lp++;
    lp->p = 5;	  lp->q = 7;    lp++;
    lp->p = 6;	  lp->q = 7;    lp++;

    lp->p = 0;	  lp->q = 4;    lp++;
    lp->p = 1;	  lp->q = 5;    lp++;
    lp->p = 2;	  lp->q = 6;    lp++;
    lp->p = 3;	  lp->q = 7;    lp++;
    

    if (!DXSetStringAttribute((Object)a, "element type", "lines"))
	goto error;

    if (!DXSetComponentValue(f, "connections", (Object)a))
	goto error;
    else
	a = NULL;

    if (!DXEndField(f))
	goto error;

    /* prevent lines from being buried if they are coincident with a plane */
    if (!DXSetIntegerAttribute((Object)f, "fuzz", 4))
	goto error;


    if (!(_dxf_BBoxPoint((Object)in[0], &center, BB_CENTER)))
	goto error;

    a = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, 3);
    if (!DXAddArrayData(a, 0, 1, (Pointer)&center))
	goto error;

    
    out[0] = (Object)f;
    out[1] = (Object)a;

    if (ERROR == DXCopyAttributes(out[0], in[0]))
	goto error;

    return OK;

  error:
    DXDelete((Object)f);
    DXDelete((Object)a);

    out[0] = NULL;
    out[1] = NULL;

    return ERROR;
}
