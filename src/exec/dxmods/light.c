/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>



#include <dx/dx.h>


Error
m_Light(Object *in, Object *out)
{
    Vector v;
    RGBColor c;
    int camera = 0;
    char *colorstr;

    /* if first parm isn't a vector, try to see if it's a camera object.
     *  if so, use the same calculation as the renderer uses for the default
     *  light - slightly to the left of the camera position.
     */
    if (!in[0]) {
	v.x = v.y = 0.0;
	v.z = 1.0;
    } 
    else if (!DXExtractParameter(in[0], TYPE_FLOAT, 3, 1, (Pointer)&v)) {

	if (DXGetObjectClass(in[0]) == CLASS_CAMERA) {
	    Point from, to;
	    Vector eye, up, left;

	    DXGetView((Camera)in[0], &from, &to, &up);
	    eye = DXNormalize(DXSub(from,to));
	    left = DXCross(eye, up);
	    v = DXAdd(eye, left);
	} 
	else {
	    DXSetError(ERROR_BAD_PARAMETER, "#10670", "`where'");
	    return ERROR;
	}
    }             


    /* color, default is white */
    if (!in[1]) {
	c.r = c.b = c.g = 1.0;
    } 
    else if (DXExtractString(in[1],&colorstr)) {
        if (!DXColorNameToRGB(colorstr, &c))
	    /* it sets a reasonable error string */
	    return ERROR;
    } 
    else if (!DXExtractParameter(in[1], TYPE_FLOAT, 3, 1, (Pointer)&c)) {
        DXSetError(ERROR_BAD_PARAMETER, "#10510", "color");
        return ERROR;
    }             

    
    /* absolute or camera relative? */
    if (in[2] && !DXExtractInteger(in[2], &camera)) {
        DXSetError(ERROR_BAD_PARAMETER, "#10070", "camera flag");
        return ERROR;
    }


    if (!camera)
	out[0] = (Object)DXNewDistantLight(v, c);
    else
	out[0] = (Object)DXNewCameraDistantLight(v, c);
    
		
    return(out[0] ? OK : ERROR);
}











