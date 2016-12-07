/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dxmods/ambientlight.c,v 1.4 2000/08/24 20:04:23 davidt Exp $
 */

#include <dxconfig.h>


/***
MODULE: 
 AmbientLight
SHORTDESCRIPTION: 
 Creates an ambient light source.
CATEGORY:
 Rendering
INPUTS:
 color;        vector value;   [0.2,0.2,0.2];  white light
OUTPUTS:
 light;        Light;    NULL;    an ambient light source
BUGS:
 Not tested yet.
AUTHOR:
 module by ns collins, Light code by Bruce Lucas
END:
***/

#include <string.h>
#include <dx/dx.h>

#if 0
static int GoodRGB(RGBColor);
#endif

int
m_AmbientLight(Object *in, Object *out)
{
    RGBColor c;
    char *colorstring;


    if(!in[0])
	c = DXRGB(0.2, 0.2, 0.2);

    else if(!DXExtractParameter(in[0], TYPE_FLOAT, 3, 1, (Pointer)&c)) {
        if (!DXExtractString(in[0], &colorstring)) {
          DXSetError(ERROR_BAD_PARAMETER, "#10510", "color");
          return ERROR;
        }
        if (!DXColorNameToRGB(colorstring, &c))
          return ERROR;
    }   
        
    out[0] = (Object)DXNewAmbientLight(c);

    return(out[0] ? OK : ERROR);
}


#if 0
static int GoodRGB(RGBColor col)
{

  /* negative colors are an error */
  if (col.r < 0.0 || col.g < 0.0 || col.b < 0.0 ) {
    DXSetError(ERROR_BAD_PARAMETER,"bad rgb color");
    return ERROR;
  }

  if (col.r > 1.0 || col.g > 1.0 || col.b > 1.0) {
    DXSetError(ERROR_BAD_PARAMETER,"bad rgb color");
    return ERROR;
  }
  return OK;
}
#endif

