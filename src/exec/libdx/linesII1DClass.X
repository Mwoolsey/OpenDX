/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

//
// $Header: /src/master/dx/src/exec/libdx/linesII1DClass.X,v 1.3 2003/07/11 05:50:45 davidt Exp $
//


typedef struct linesii1dinterpolator *LinesII1DInterpolator;

INCLUDE      fieldinterpClass.X
SUBCLASS    LinesII1DInterpolator OF FieldInterpolator
IMPLEMENTS  PrimitiveInterpolate Delete Copy LocalizeInterpolator

#include "binSort.h"

LinesII1DInterpolator _dxfNewLinesII1DInterpolator(Field,
				enum interp_init, double, Matrix *);
int   _dxfRecognizeLinesII1D(Field);

typedef struct linearCoord	 	 LinearCoord;

struct linearCoord
{
    float p, q;
};

struct linesii1dinterpolator
{
    struct fieldinterpolator	fieldInterpolator;

    Array 		pointsArray;
    ArrayHandle		pHandle;
    int			nPoints;

    Array 		linesArray;
    ArrayHandle		lHandle;
    int			nLines;

    Array 		dataArray;
    ArrayHandle		dHandle;
    int			nElements;

    int			hint;			/* last segment interpolated */

    Grid		grid;
    int			gridFlag;
};

LinesII1DInterpolator _dxf_NewLinesII1DInterpolator(Field,
			enum interp_init, float, Matrix *,
			struct linesii1dinterpolator_class *);

LinesII1DInterpolator _dxf_CopyLinesII1DInterpolator(LinesII1DInterpolator,
			LinesII1DInterpolator, enum _dxd_copy);

