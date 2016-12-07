/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>

#include "../base/defines.h"

#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include "ColorMapEditorP.h"
#include "Color.h"

#define R 5
#define G 9
#define B 5

#define DX 2
#define DY 2
#define D (DX*DY)

/*
 * MIX(r,g,b,) calculates the color table index for r,g,b;
 * 0 <= r < R, 0 <= g < G; 0 <= b < B.
 */

#define MIX(r,g,b) (((r)*G+(g))*B+(b))

/* DITH(C,c,d) dithers one color component of one pixel;
 *
 * C is the number of shades of the color; C = R, G, or B
 * c is the input pixel, i.e. c = in->red, in->green of in->blue; 0 <= c< 255
 * d is a dither matrix entry scaled by 256, 0 <= d <= (D-1) * 256
 *
 * Hint: dithering produces D*(C-1)+1 shades of color C
 */

#define DITH(C,c,d) (((unsigned)((D*(C-1)+1)*c+d))/(D*256))

/*
 * The dither matrix
 */

/* Normal matrix for 2x2 dither */
static short matrix[DX][DY] = 
    { 	{ 0*256, 2*256 },
	{ 3*256, 1*256 },
    };
#ifdef Comment
/* Normal matrix for 4x4 dither */
static short matrix[DX][DY] = 
    { 	0*256,	8*256,	2*256,	10*256,
	12*256,	4*256,	14*256,	6*256,	
	3*256,	11*256,	1*256,	9*256,	
	15*256,	7*256,	13*256,	5*256
    };

static short matrix[DX][DY] = 
    { 	0*256,	2*256,	3*256,	1*256,
	3*256,	1*256,	0*256,	2*256,	
	1*256,	0*256,	2*256,	3*256,	
	2*256,	3*256,	1*256,	0*256
    };
#endif


/*
 * Dither an image
 */	

void Dither(XColor *in, int width, int height, XColor *out, ControlColor *color)
{
int y;
register int x, d;
register short *m;

    for(y = 0; y < height; y++)
	{
	m = &(matrix[y&(DY-1)][0]);
	for(x = width; --x >= 0; in++)
	    {
	    d = m[x&(DX-1)];
	    (out++)->pixel = color->rgb[DITH(R, in->red, d)]
				       [DITH(G, in->green, d)]
				       [DITH(B, in->blue, d)].pixel;
	    }
	}
}
