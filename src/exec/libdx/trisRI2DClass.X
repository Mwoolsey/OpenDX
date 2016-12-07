/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

//
// $Header: /src/master/dx/src/exec/libdx/trisRI2DClass.X,v 1.3 2003/07/11 05:50:48 davidt Exp $
//


typedef struct trisri2dinterpolator *TrisRI2DInterpolator;

INCLUDE      fieldinterpClass.X
SUBCLASS    TrisRI2DInterpolator OF FieldInterpolator
IMPLEMENTS  PrimitiveInterpolate Delete Copy LocalizeInterpolator

#include "binSort.h"
				
TrisRI2DInterpolator _dxfNewTrisRI2DInterpolator(Field, 
			enum interp_init, double, Matrix *);
int   _dxfRecognizeTrisRI2D(Field);

typedef struct triCoord	 	 TriCoord;

struct triCoord
{
    float p, q, r;
};

struct trisri2dinterpolator
{
    struct fieldinterpolator	fieldInterpolator;

    Array 		pArray;
    ArrayHandle		pHandle;
    int			nPoints;

    Array		nArray;
    Triangle		*neighbors;

    Array 		tArray;
    Triangle		*triangles;
    int			nTriangles;

    Array 		dArray;
    ArrayHandle		dHandle;
    int			nElements;

    /*
     * Stuff for regular array of bins used to locate containing
     * triangle quickly
     */
    Grid		grid;

    int			hint;			/* last tet interpolated */
};

TrisRI2DInterpolator _dxf_NewTrisRI2DInterpolator(Field, 
			enum interp_init, float, Matrix *,
			struct trisri2dinterpolator_class *);

TrisRI2DInterpolator _dxf_CopyTrisRI2DInterpolator(TrisRI2DInterpolator,
			TrisRI2DInterpolator, enum _dxd_copy);

