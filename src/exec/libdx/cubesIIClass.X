/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

//
// $Header: /src/master/dx/src/exec/libdx/cubesIIClass.X,v 1.3 2003/07/11 05:50:41 davidt Exp $
//


typedef struct cubesiiinterpolator *CubesIIInterpolator;

INCLUDE  	fieldinterpClass.X
SUBCLASS    	CubesIIInterpolator OF FieldInterpolator
IMPLEMENTS  	PrimitiveInterpolate Delete Copy LocalizeInterpolator

CubesIIInterpolator _dxfNewCubesIIInterpolator(Field,
			enum interp_init, double, Matrix *);

int   RecognizeCubesII(Field);

#include "binSort.h"

struct cubesiiinterpolator
{
    struct fieldinterpolator	fieldInterpolator;

    Array 		pointsArray;
    ArrayHandle		pHandle;
    int			nPoints;

    Array 		cubesArray;
    ArrayHandle		cHandle;
    int			nCubes;

    Array 		dataArray;
    ArrayHandle		dHandle;
    int			nElements;

    Array               nbrsArray;
    int                 *nbrs;

    int			gridCounts[3];
    int			Cyz, Cz;

    /*
     * grid for fast search
     */
    int			gridFlag;
    Grid		grid;

    int			*visited;
    int			vCount;

    int			hint;			/* last cube interpolated */
};

CubesIIInterpolator _dxf_NewCubesIIInterpolator(Field, 
				enum interp_init, float, Matrix *,
				struct cubesiiinterpolator_class *);

CubesIIInterpolator _dxf_CopyCubesIIInterpolator(CubesIIInterpolator,
				CubesIIInterpolator, enum _dxd_copy);

