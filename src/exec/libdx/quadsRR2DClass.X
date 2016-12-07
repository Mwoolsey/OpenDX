/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

//
// $Header: /src/master/dx/src/exec/libdx/quadsRR2DClass.X,v 1.3 2003/07/11 05:50:46 davidt Exp $
//


typedef	struct quadsrr2dinterpolator *QuadsRR2DInterpolator;

INCLUDE  	fieldinterpClass.X
SUBCLASS    	QuadsRR2DInterpolator OF FieldInterpolator
IMPLEMENTS  	PrimitiveInterpolate Delete Copy LocalizeInterpolator
				
QuadsRR2DInterpolator _dxfNewQuadsRR2DInterpolator(Field, 
			enum interp_init, double, Matrix *);
int   _dxfRecognizeQuadsRR2D(Field);

struct quadsrr2dinterpolator
{
    struct fieldinterpolator	fieldInterpolator;

    int	  size[2];	/* sizeof subarray size for indexing */
    int   counts[2];

    Array pointsArray;
    Array dataArray;
    int	  nElements;
    int   meshOffsets[2];

    ArrayHandle dHandle;

    float  fuzz;
};

QuadsRR2DInterpolator _dxf_NewQuadsRR2DInterpolator(Field, 
			enum interp_init, float, Matrix *,
			struct quadsrr2dinterpolator_class *);

QuadsRR2DInterpolator _dxf_CopyQuadsRR2DInterpolator(QuadsRR2DInterpolator, 
			QuadsRR2DInterpolator, enum _dxd_copy);
