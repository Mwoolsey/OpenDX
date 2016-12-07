/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#include <math.h>
#include <X11/Xlib.h>

void gamma_correct(XColor *cell_def)
{
unsigned int red255, green255, blue255;

    red255   =  cell_def->red >> 8;
    green255 =  cell_def->green >> 8;
    blue255  =  cell_def->blue >> 8;

    red255 =sqrt(red255/256.0) * 256.0;
    green255 = sqrt(green255/256.0) * 256.0;
    blue255 = sqrt(blue255/256.0) * 256.0;

    cell_def->red = red255 << 8;
    cell_def->green = green255 << 8;
    cell_def->blue = blue255 << 8;
}

