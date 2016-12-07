/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

//
// $Header: /src/master/dx/src/exec/libdx/fieldinterpClass.X,v 1.5 2003/07/11 05:50:42 davidt Exp $
//

typedef struct fieldinterpolator *FieldInterpolator;

INCLUDE  	interpClass.X
SUBCLASS    	FieldInterpolator     OF Interpolator
DEFINES     	Error  PrimitiveInterpolate(FieldInterpolator, int *, float **, Pointer *, int);
IMPLEMENTS 	Interpolate Delete

FieldInterpolator _dxfSelectFieldInterpolator(Field,
				enum interp_init, float, Matrix *);

int _dxfRecognizeCubesII(Field);
int _dxfRecognizeCubesRR(Field);
int _dxfRecognizeLinesRR1D(Field);
int _dxfRecognizeQuadsRR2D(Field);
int _dxfRecognizeTetras(Field);
int _dxfRecognizeTrisRI2D(Field);
int _dxfRecognizeLinesRI1D(Field);

#define DATA_POSITIONS_DEPENDENT	0x01
#define DATA_CONNECTIONS_DEPENDENT	0x02
#define DATA_FACES_DEPENDENT		0x03

struct fieldinterpolator
{
    struct interpolator	   interpolator;
    int			   initialized;
    int			   localized;
    float		   fuzz;
    int			   data_dependency;
    InvalidComponentHandle invCon;
    Pointer		   cstData;
    Matrix 		   xform;
    int			   xflag;
};

FieldInterpolator _dxf_NewFieldInterpolator(Field, float, Matrix *,
				struct fieldinterpolator_class *);

FieldInterpolator _dxf_CopyFieldInterpolator(FieldInterpolator, 
				FieldInterpolator, enum _dxd_copy copy);

#define ELT_TYPECHECK(field, name)					      \
{									      \
    Object at = DXGetComponentValue(field, "connections");		      \
    if (! at)								      \
	return 0;							      \
									      \
    at = DXGetComponentAttribute(field, "connections", "element type");	      \
    if (! at) {								      \
	DXSetError(ERROR_DATA_INVALID,					      \
		"#10255", "connections", "element type");		      \
	return 0;							      \
    }									      \
									      \
    if (DXGetObjectClass(at) != CLASS_STRING) {				      \
	DXSetError(ERROR_DATA_INVALID, "#10200", "element type attribute");   \
	return 0;							      \
    }									      \
									      \
    if (strcmp(name, DXGetString((String)at)))				      \
	return 0;							      \
}
