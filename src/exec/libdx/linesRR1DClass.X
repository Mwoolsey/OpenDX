/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

//
// $Header: /src/master/dx/src/exec/libdx/linesRR1DClass.X,v 1.3 2003/07/11 05:50:45 davidt Exp $
//


typedef struct linesrr1dinterpolator *LinesRR1DInterpolator;

INCLUDE  	fieldinterpClass.X
SUBCLASS    	LinesRR1DInterpolator OF FieldInterpolator
IMPLEMENTS  	PrimitiveInterpolate Delete Copy LocalizeInterpolator

LinesRR1DInterpolator _dxfNewLinesRR1DInterpolator(Field, 
			enum interp_init, double, Matrix *);
int   	_dxfRecognizeLinesRR1D(Field);

struct linesrr1dinterpolator
{
    struct fieldinterpolator	fieldInterpolator;

    float 			origin;	  /* origin for each axis */
    float 			invDelta; /* inverse of stepsz for each axis */

    int				count;

    Array 			pointsArray;
    Array 			dataArray;
    int	  			nElements;

    ArrayHandle			data;

    float			fuzz;
};

LinesRR1DInterpolator _dxf_NewLinesRR1DInterpolator(Field, 
			enum interp_init, float, Matrix *m,
			    struct linesrr1dinterpolator_class *);

LinesRR1DInterpolator _dxf_CopyLinesRR1DInterpolator(LinesRR1DInterpolator,
			LinesRR1DInterpolator, enum _dxd_copy);

