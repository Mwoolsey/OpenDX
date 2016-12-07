/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

//
// $Header: /src/master/dx/src/exec/libdx/quadsII2DClass.X,v 1.3 2003/07/11 05:50:46 davidt Exp $
//


typedef struct quadsii2dinterpolator *QuadsII2DInterpolator;

INCLUDE      fieldinterpClass.X
SUBCLASS    QuadsII2DInterpolator OF FieldInterpolator
IMPLEMENTS  PrimitiveInterpolate Delete Copy LocalizeInterpolator

#include "binSort.h"
				
QuadsII2DInterpolator _dxfNewQuadsII2DInterpolator(Field, 
			enum interp_init, double, Matrix *m);

int   _dxfRecognizeQuadsII2D(Field);


struct quadCoord
{
    float p, q, r, s;
};

typedef struct quadCoord	 	QuadCoord;

struct quadsii2dinterpolator
{
    struct fieldinterpolator	fieldInterpolator;

    Array 		pArray;
    ArrayHandle		pHandle;
    int			nPoints;

    Array		nArray;
    int			*neighbors;

    Array 		qArray;
    ArrayHandle		qHandle;
    int			nQuads;

    Array 		dArray;
    ArrayHandle		dHandle;
    int			nElements;

    int			gridCounts[2];

    /*
     * Stuff for regular array of bins used to locate containing
     * quad quickly
     */
    Grid		grid;
    int			gridFlag;

    int			vCount;
    int			*visited;

    int			hint;			/* last tet interpolated */
};

QuadsII2DInterpolator _dxf_NewQuadsII2DInterpolator(Field,
			enum interp_init, float, Matrix *,
			struct quadsii2dinterpolator_class *);

QuadsII2DInterpolator _dxf_CopyQuadsII2DInterpolator(QuadsII2DInterpolator,
			QuadsII2DInterpolator, enum _dxd_copy);

