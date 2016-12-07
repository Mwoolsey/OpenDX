/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

//
// $Header: /src/master/dx/src/exec/libdx/linesRI1DClass.X,v 1.3 2003/07/11 05:50:45 davidt Exp $
//


typedef struct linesri1dinterpolator *LinesRI1DInterpolator;

INCLUDE      fieldinterpClass.X
SUBCLASS    LinesRI1DInterpolator OF FieldInterpolator
IMPLEMENTS  PrimitiveInterpolate Delete Copy LocalizeInterpolator

LinesRI1DInterpolator _dxfNewLinesRI1DInterpolator(Field,
				enum interp_init, double, Matrix *);
int   _dxfRecognizeLinesRI1D(Field);

typedef struct linearCoord	 	 LinearCoord;

struct linearCoord
{
    float p, q;
};

struct linesri1dinterpolator
{
    struct fieldinterpolator	fieldInterpolator;

    Array 		pointsArray;
    ArrayHandle		pHandle;
    int			nPoints;

    Array 		linesArray;
    int			*lines;
    int			nLines;

    Array 		dataArray;
    ArrayHandle		dHandle;
    int			nElements;

    int			hint;			/* last segment interpolated */
    int			direction;
};

LinesRI1DInterpolator _dxf_NewLinesRI1DInterpolator(Field,
			enum interp_init, float, Matrix *,
			struct linesri1dinterpolator_class *);

LinesRI1DInterpolator _dxf_CopyLinesRI1DInterpolator(LinesRI1DInterpolator,
			LinesRI1DInterpolator, enum _dxd_copy);

