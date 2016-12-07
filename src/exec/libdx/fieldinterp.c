/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>



#include <stdio.h>
#include <math.h>
#include <string.h>
#include "dx/dx.h"
#include "fieldinterpClass.h"

typedef int          	  (*PFI)();
typedef FieldInterpolator (*PFFI)();

int	_dxfRecognizeTetras(Field);
PFFI	_dxfNewTetrasInterpolator(Field, enum interp_init, double, Matrix *);

int	_dxfRecognizeLinesII1D(Field);
PFFI	_dxfNewLinesII1DInterpolator(Field, enum interp_init, double, Matrix *);

int	_dxfRecognizeLinesRR1D(Field);
PFFI	_dxfNewLinesRR1DInterpolator(Field, enum interp_init, double, Matrix *);

int	_dxfRecognizeTrisRI2D(Field);
PFFI	_dxfNewTrisRI2DInterpolator(Field, enum interp_init, double, Matrix *);

int	_dxfRecognizeQuadsRR2D(Field);
PFFI	_dxfNewQuadsRR2DInterpolator(Field, enum interp_init, double, Matrix *);

int	_dxfRecognizeQuadsII2D(Field);
PFFI	_dxfNewQuadsII2DInterpolator(Field, enum interp_init, double, Matrix *);

int	_dxfRecognizeCubesII(Field);
PFFI	_dxfNewCubesIIInterpolator(Field, enum interp_init, double, Matrix *);

int     _dxfRecognizeLinesRI1D(Field);
PFFI    _dxfNewLinesRI1DInterpolator(Field, enum interp_init, double, Matrix *);

int	_dxfRecognizeCubesRR(Field);
PFFI	_dxfNewCubesRRInterpolator(Field, enum interp_init, double, Matrix *);

int	_dxfRecognizeFLE2D(Field);
PFFI	_dxfNewFle2DInterpolator(Field, enum interp_init, double, Matrix *);

static struct subClass
{
    PFI		recognizeMth;
    PFFI	newMth;
} _dxdsubClasses[] =
    {
	{ _dxfRecognizeTetras,    (PFFI)_dxfNewTetrasInterpolator    },
	{ _dxfRecognizeLinesRR1D, (PFFI)_dxfNewLinesRR1DInterpolator },
	{ _dxfRecognizeLinesRI1D, (PFFI)_dxfNewLinesRI1DInterpolator },
	{ _dxfRecognizeLinesII1D, (PFFI)_dxfNewLinesII1DInterpolator },
	{ _dxfRecognizeTrisRI2D,  (PFFI)_dxfNewTrisRI2DInterpolator  },
	{ _dxfRecognizeQuadsRR2D, (PFFI)_dxfNewQuadsRR2DInterpolator },
	{ _dxfRecognizeQuadsII2D, (PFFI)_dxfNewQuadsII2DInterpolator },
	{ _dxfRecognizeCubesRR,   (PFFI)_dxfNewCubesRRInterpolator   },
	{ _dxfRecognizeCubesII,   (PFFI)_dxfNewCubesIIInterpolator   },
	{ _dxfRecognizeFLE2D,     (PFFI)_dxfNewFle2DInterpolator     },
	{ NULL,		          NULL		   	             }
    };

FieldInterpolator
_dxfSelectFieldInterpolator(Field f,
		enum interp_init initType, float fuzz, Matrix *m)
{
    FieldInterpolator	fi;
    struct subClass 	*sub;

    /*
     * If the field is empty of positions and/or connections,
     * return an empty interpolator
     */
    if (DXEmptyField(f))
        goto emptyInterpolator;
    
    for (sub = _dxdsubClasses; sub->recognizeMth != NULL; sub++)
    {
	if (sub->recognizeMth(f))
	    break;
	
	if (DXGetError() != ERROR_NONE)
	    break;
    }

    if (!sub->recognizeMth || DXGetError() != ERROR_NONE)
    {
	if (DXGetError() == ERROR_NONE)
	    DXSetError(ERROR_DATA_INVALID,  "#11850");
	return NULL;
    }
    
    fi = (*(sub->newMth))(f, initType, (double)fuzz, m);
    if (!fi)
	return NULL;
    
    return fi;

emptyInterpolator:

    fi = _dxf_NewFieldInterpolator(f, fuzz, m, &_dxdfieldinterpolator_class);
    
    return fi;
}

FieldInterpolator
_dxf_NewFieldInterpolator(Field f, float fuzz, Matrix *m,
			    struct fieldinterpolator_class *class)
{
    FieldInterpolator fi;
    Array	      boxArray, dArray;
    float	      *boxPts, *boxPt;
    int		      i, nDim, nItems;
    Object	      attr;
    char	      *str;

    fi = (FieldInterpolator)_dxf_NewInterpolator
		((struct interpolator_class *)class, (Object)f);
	
    if (! fi)
	return NULL;

    fi->initialized = 0;
    fi->localized   = 0;
    fi->fuzz 	    = fuzz;

    if (m)
    {
	fi->xform = DXInvert(*m);
	fi->xflag = 1;
    }
    else
	fi->xflag = 0;

    for (i = 0; i < MAX_DIM; i++)
    {
	fi->interpolator.min[i] =  DXD_MAX_FLOAT;
	fi->interpolator.max[i] = -DXD_MAX_FLOAT;
    }

    if (DXBoundingBox((Object) f, NULL))
    {
	boxArray = (Array)DXGetComponentValue(f, "box");

	boxPts = (float *)DXGetArrayData(boxArray);

	DXGetArrayInfo(boxArray, &nItems, NULL, NULL, NULL, &nDim);

	fi->interpolator.nDim = nDim;

	boxPt = boxPts;
	while (nItems-- > 0)
	{
	    for (i = 0; i < nDim; i++)
	    {
		if (*boxPt > fi->interpolator.max[i]) 
		    fi->interpolator.max[i] = *boxPt;
		if (*boxPt < fi->interpolator.min[i]) 
		    fi->interpolator.min[i] = *boxPt;
		boxPt ++;
	    }

	}
    }
    else if (DXGetError() != ERROR_NONE)
    {
	DXFree((Pointer)fi);
	return NULL;
    }
    else
	fi->interpolator.nDim = 0;
    
    if (NULL == (dArray = (Array)DXGetComponentValue(f, "data")))
    {    
	DXSetError(ERROR_MISSING_DATA, "#10250", "map", "data");
	return NULL;
    }

    if (DXQueryConstantArray(dArray, NULL, NULL))
	fi->cstData = DXGetConstantArrayData(dArray);
    else
	fi->cstData = NULL;

    /*
     * Check dependency of connections array
     */
    if ((attr = DXGetComponentAttribute(f, "data", "dep")) == NULL)
    {    
	DXSetError(ERROR_MISSING_DATA, "#10241", "map");
	return NULL;
    }

    if (DXGetObjectClass(attr) != CLASS_STRING)
    {    
	DXSetError(ERROR_MISSING_DATA, "#11080", "dependency attribute");
	return NULL;
    }

    str = DXGetString((String)attr);

    if (! strcmp(str, "positions"))
	fi->data_dependency = DATA_POSITIONS_DEPENDENT;
    else if (! strcmp(str, "connections"))
	fi->data_dependency = DATA_CONNECTIONS_DEPENDENT;
    else if (! strcmp(str, "faces"))
	fi->data_dependency = DATA_FACES_DEPENDENT;
    else
    {  
	DXSetError(ERROR_DATA_INVALID, "#10250", "data");
	DXFree((Pointer)fi);
	return NULL;
    }

    if (! DXInvalidateConnections((Object)f))
    {
	DXFree((Pointer)fi);
	return NULL;
    }

    if (DXGetComponentValue(f, "connections"))
    {
	fi->invCon = DXCreateInvalidComponentHandle((Object)f, NULL, "connections");
	if (! fi->invCon)
	{
	    DXFree((Pointer)fi);
	    return NULL;
	}

	if (DXGetInvalidCount(fi->invCon) == 0)
	{
	    DXFreeInvalidComponentHandle(fi->invCon);
	    fi->invCon = NULL;
	}
    }

    return fi;
}

Error _dxfFieldInterpolator_Delete(FieldInterpolator fi)
{
    if (fi->invCon)
	DXFreeInvalidComponentHandle(fi->invCon);
    return _dxfInterpolator_Delete((Interpolator) fi);
}

int
_dxfFieldInterpolator_Interpolate(FieldInterpolator fi, int *n,
	    float **points, Pointer *values, Interpolator *hint, int fuzzFlag)
{
    int i, j;
    int nStart;
    float *p, xpoints[MAX_DIM];

    CHECK(fi, CLASS_INTERPOLATOR);

    if (hint)
	*hint = NULL;
    
    if (fi->xflag)
    {
	p = xpoints;
	for (i = 0; i < fi->interpolator.nDim; i++)
	{
	    p[i] = fi->xform.b[i];
	
	    for (j = 0; j < fi->interpolator.nDim; j++)
		p[i] += (*points)[j]*fi->xform.A[j][i];
	}
    }
    else
	p = *points;

    if (fuzzFlag == FUZZ_ON)
	for (i = 0; i < fi->interpolator.nDim; i++)
	{
	    if (fi->interpolator.min[i]-fi->fuzz > p[i]) return OK;
	    if (fi->interpolator.max[i]+fi->fuzz < p[i]) return OK;
	}
    else
	for (i = 0; i < fi->interpolator.nDim; i++)
	{
	    if (fi->interpolator.min[i] > p[i]) return OK;
	    if (fi->interpolator.max[i] < p[i]) return OK;
	}

    /*
     * Set hint ONLY if we interpolated some points
     */

    nStart = *n;

    if (! _dxfPrimitiveInterpolate(fi, n, points, values, fuzzFlag))
	return ERROR;

    if (hint && nStart != *n)
	*hint = (Interpolator)fi;

    return OK;
}

FieldInterpolator
_dxf_CopyFieldInterpolator(FieldInterpolator new, 
				FieldInterpolator old, enum _dxd_copy copy)
{
    if (! _dxf_CopyInterpolator((Interpolator)new, (Interpolator)old))
	return NULL;
    
    new->localized          = old->localized;
    new->initialized        = old->initialized;
    new->fuzz 	            = old->fuzz;
    new->data_dependency    = old->data_dependency;
    new->cstData            = old->cstData;

    if (old->invCon)
	new->invCon = DXCreateInvalidComponentHandle
		((new->interpolator.dataObject), NULL, "connections");
    else
	new->invCon = NULL;
    
    return new;
}
    

