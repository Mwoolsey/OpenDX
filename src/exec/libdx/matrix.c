/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>



#include <math.h>
#include <dx/arch.h>
#include <dx/basic.h>


/*
 * Implementation of transform matrix generation
 * and manipulation routines.
 */

Matrix DXRotateX(Angle angle)
{
    float c = cos(angle), s = sin(angle);
    return DXMat(
	 1,  0,  0,
	 0,  c,  s,
	 0, -s,  c,
	 0,  0,  0
    );
}

Matrix DXRotateY(Angle angle)
{
    float c = cos(angle), s = sin(angle);
    return DXMat(
	 c,  0, -s,
         0,  1,  0,
	 s,  0,  c,
	 0,  0,  0
    );
}

Matrix DXRotateZ(Angle angle)
{
    float c = cos(angle), s = sin(angle);
    return DXMat(
	 c,  s,  0,
	-s,  c,  0,
	 0,  0,  1,
	 0,  0,  0
    );
}

Matrix DXScale(double x, double y, double z)
{
    return DXMat(
	x, 0, 0,
	0, y, 0,
	0, 0, z,
	0, 0, 0
    );
}

Matrix DXTranslate(Vector v)
{
    return DXMat(
	 1,   0,   0,
	 0,   1,   0,
	 0,   0,   1,
	v.x, v.y, v.z
    );
}

Matrix DXConcatenate(Matrix s, Matrix t)
{
    register int i, j, k;
    register double sum;
    Matrix r;
    for (i=0; i<3; i++) {
	for (j=0; j<3; j++) {
	    sum = 0;
	    for (k=0; k<3; k++)
		sum += s.A[i][k]*t.A[k][j];
	    r.A[i][j] = sum;
	}
    }
    for (i=0; i<3; i++) {
	sum = t.b[i];
	for (j=0; j<3; j++)
	    sum += s.b[j]*t.A[j][i];
	r.b[i] = sum;
    }	
    return r;
}


