/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

//
// $Header: /src/master/dx/src/exec/libdx/fle2DClass.X,v 1.3 2003/07/11 05:50:42 davidt Exp $
//


typedef struct fle2dinterpolator *Fle2DInterpolator;

INCLUDE      fieldinterpClass.X
SUBCLASS    Fle2DInterpolator OF FieldInterpolator
IMPLEMENTS  PrimitiveInterpolate Delete Copy LocalizeInterpolator

#include "binSort.h"
				
Fle2DInterpolator _dxfNewFle2DInterpolator(Field, 
			enum interp_init, double, Matrix *);
int   _dxfRecognizeFle2D(Field);

typedef struct minmax
{
    float x, y, X, Y;
} MinMax;

struct fle2dinterpolator
{
    struct fieldinterpolator	fieldInterpolator;

    Array		fArray;
    int			*faces;
    int			nFaces;

    Array		lArray;
    int			*loops;
    int			nLoops;

    Array		eArray;
    int			*edges;
    int			nEdges;

    Array		pArray;
    float		*positions;

    Array		dArray;
    ArrayHandle		dHandle;

    Array		mmfArray;
    MinMax		*minmax_faces;

    Array		mmlArray;
    MinMax		*minmax_loops;

    int			hint;			/* last tet interpolated */
};

Fle2DInterpolator _dxf_NewFle2DInterpolator(Field, 
			enum interp_init, float, Matrix *,
			struct fle2dinterpolator_class *);

Fle2DInterpolator _dxf_CopyFle2DInterpolator(Fle2DInterpolator,
			Fle2DInterpolator, enum _dxd_copy);

