/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/



INCLUDE  objectClass.X

SUBCLASS    Interpolator    OF Object
DEFINES     Error    Interpolate(Interpolator, int *, float **, Pointer *, Interpolator *, int)
DEFINES	    Interpolator   LocalizeInterpolator(Interpolator);
IMPLEMENTS  Delete GetType

/*
 * Fuzz factor for interpolation: interpolators should assume points
 * within this proportion of an estimated primitive edge length from a
 * primitive actually lie within that primitive.
 */
#define FUZZ		0.0003
#define MAX_DIM		8

struct interpolator
{
    struct object	object;
    Object		dataObject;
    Object		rootObject;
    float		min[MAX_DIM];
    float		max[MAX_DIM];
    int			nDim;
    Type		type;
    Category		category;
    int			rank, shape[32];
    float		fuzz;
    Matrix		matrix;
};

Interpolator	_dxfNewInterpolatorSwitch(Object,
				enum interp_init, float, Matrix *);

Interpolator	_dxf_NewInterpolator(struct interpolator_class *, Object);
				
Interpolator	_dxf_CopyInterpolator(Interpolator, Interpolator);

#define FUZZ_ON	 1
#define FUZZ_OFF 0
