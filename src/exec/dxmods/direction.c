/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


/*
 * Transform spherical to cartesian coordinates. 
 */

#include <math.h>
#include <string.h>
#include <dx/dx.h>

#define SIN(x)    (sin(((double) x + 0) / (180.0/M_PI)))
#define COS(x)    (cos(((double) x + 0) / (180.0/M_PI)))


int
m_Direction(Object *in, Object *out)
{
    Vector v;
    float az, el, r;
    
    if(in[0]) {
	if(!DXExtractFloat(in[0], &az)) {
	    DXSetError(ERROR_BAD_PARAMETER, "#10080", "azimuth");
	    return ERROR;
	}
    } else
	az = 0.0;

    if(in[1]) {
	if(!DXExtractFloat(in[1], &el)) {
	    DXSetError(ERROR_BAD_PARAMETER, "#10080", "elevation");
	    return ERROR;
	}
    } else
	el = 0.0;

    if(in[2]) {
	if(!DXExtractFloat(in[2], &r)) {
	    DXSetError(ERROR_BAD_PARAMETER, "#10080", "distance");
	    return ERROR;
	}
    } else
	r = 1.0;


    v.x = r * SIN(az) * COS(el);
    v.y = r * SIN(el);
    v.z = r * COS(az) * COS(el);


    out[0] = (Object)DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, 3);
    DXAddArrayData((Array)out[0], 0, 1, (Pointer)&v);

    return (out[0] ? OK : ERROR);
}




