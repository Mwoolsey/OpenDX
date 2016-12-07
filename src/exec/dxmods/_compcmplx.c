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
#include <float.h>
#include <string.h>
#include <dx/dx.h>
#include "_compute.h"
#include "_compputils.h"
#include "_compoper.h"

complexFloat
_dxfComputeAddComplexFloat(complexFloat x, complexFloat y) 
{
    complexFloat result;
    REAL(result) = REAL(x) + REAL(y);
    IMAG(result) = IMAG(x) + IMAG(y);

    return result;
}
complexFloat
_dxfComputeSubComplexFloat(complexFloat x, complexFloat y) 
{
    complexFloat result;
    REAL(result) = REAL(x) - REAL(y);
    IMAG(result) = IMAG(x) - IMAG(y);

    return result;
}
complexFloat
_dxfComputeNegComplexFloat(complexFloat x)
{
    complexFloat result;
    REAL(result) = -REAL(x);
    IMAG(result) = -IMAG(x);

    return result;
}
complexFloat
_dxfComputeMulComplexFloat(complexFloat x, complexFloat y) 
{
    complexFloat result;
    REAL(result) = REAL(x) * REAL(y) - IMAG(x) * IMAG(y);
    IMAG(result) = REAL(x) * IMAG(y) + IMAG(x) * REAL(y);

    return result;
}
complexFloat
_dxfComputeDivComplexFloat(complexFloat x, complexFloat y) 
{
    complexFloat result;
    float        denom;

    denom = REAL(y) * REAL(y) + IMAG(y) * IMAG(y);
    REAL(result) = REAL(x) * REAL(y) + IMAG(x) * IMAG(y);
    IMAG(result) = IMAG(x) * REAL(y) - REAL(x) * IMAG(y);
    REAL(result) /= denom;
    IMAG(result) /= denom;

    return result;
}
float
_dxfComputeAbsComplexFloat(complexFloat x)
{
    float result;

    result = REAL(x) * REAL(x) + IMAG(x) * IMAG(x);
    result = sqrt(result);

    return result;
}
float
_dxfComputeArgComplexFloat(complexFloat x)
{
    float result;

    result = atan2(IMAG(x), REAL(x));

    return result;
}
complexFloat
_dxfComputeSqrtComplexFloat(complexFloat x)
{
    complexFloat result;

    REAL(result) = REAL(x) + _dxfComputeAbsComplexFloat(x);
    REAL(result) = sqrt(REAL(result) / 2.0);
    IMAG(result) = -REAL(x) + _dxfComputeAbsComplexFloat(x);
    IMAG(result) = sqrt(IMAG(result) / 2.0);
    if (IMAG(x) < 0)
	IMAG(result) = -IMAG(result);

    return result;
}
complexFloat
_dxfComputeLnComplexFloat(complexFloat x)
{
    complexFloat result;

    REAL(result) = log(_dxfComputeAbsComplexFloat(x));
    IMAG(result) = _dxfComputeArgComplexFloat(x);

    return result;
}
complexFloat
_dxfComputePowComplexFloatFloat(complexFloat x, float y)
{
    complexFloat result;
    float        k;

    k = pow(_dxfComputeAbsComplexFloat(x), y);

    REAL(result) = k * cos(y * _dxfComputeArgComplexFloat(x));
    IMAG(result) = k * sin(y * _dxfComputeArgComplexFloat(x));

    return result;
}
complexFloat
_dxfComputeExpComplexFloat(complexFloat x)
{
    complexFloat result;
    float        k;

    k = exp(REAL(x));

    REAL(result) = k * cos(IMAG(x));
    IMAG(result) = k * sin(IMAG(x));

    return result;
}
complexFloat
_dxfComputePowComplexFloat(complexFloat x, complexFloat y)
{
    complexFloat result;

    result = _dxfComputeMulComplexFloat(y, _dxfComputeLnComplexFloat(x));
    result = _dxfComputeExpComplexFloat(result);

    return result;
}
complexFloat
_dxfComputeSinComplexFloat(complexFloat x)
{
    complexFloat result;

    REAL(result) = sin(REAL(x)) * cosh(IMAG(x));
    IMAG(result) = cos(REAL(x)) * sinh(IMAG(x));

    return result;
}
complexFloat
_dxfComputeCosComplexFloat(complexFloat x)
{
    complexFloat result;

    REAL(result) = cos(REAL(x)) * cosh(IMAG(x));
    IMAG(result) = -sin(REAL(x)) * sinh(IMAG(x));

    return result;
}
complexFloat
_dxfComputeSinhComplexFloat(complexFloat x)
{
    complexFloat result;

    REAL(result) = sinh(REAL(x)) * cos(IMAG(x));
    IMAG(result) = cosh(REAL(x)) * sin(IMAG(x));

    return result;
}
complexFloat
_dxfComputeCoshComplexFloat(complexFloat x)
{
    complexFloat result;

    REAL(result) = cosh(REAL(x)) * cos(IMAG(x));
    IMAG(result) = sinh(REAL(x)) * sin(IMAG(x));

    return result;
}
complexFloat
_dxfComputeTanComplexFloat(complexFloat x)
{
    complexFloat result;
    float        denom;

    denom = cos(2.0*REAL(x)) + cosh(2.0*IMAG(x));
    REAL(result) =  sin(2.0*REAL(x)) / denom;
    IMAG(result) = sinh(2.0*IMAG(x)) / denom;

    return result;
}
complexFloat
_dxfComputeTanhComplexFloat(complexFloat x)
{
    complexFloat result;

    result = _dxfComputeCoshComplexFloat(x);
    if (REAL(result) == 0 && IMAG(result) == 0)
	REAL(result) = IMAG(result) = 1e20F;
    else
	result = _dxfComputeDivComplexFloat(_dxfComputeSinhComplexFloat(x), result);

    return result;
}

#if defined(HAVE_ISNAN)
int
_dxfComputeIsnanComplexFloat(complexFloat x)
{
    int result;

    result = isnan(REAL(x)) || isnan(IMAG(x));
    return result;
}
#endif
#if defined(HAVE_FINITE)
int
_dxfComputeFiniteComplexFloat(complexFloat x)
{
    int result;

    result = finite(REAL(x)) && finite(IMAG(x));
    return result;
}
#endif
complexFloat
_dxfComputeAsinComplexFloat(complexFloat x)
{
    complexFloat result;
    complexFloat temp;

    REAL(result) = 1;
    IMAG(result) = 0;
    temp = _dxfComputeMulComplexFloat(x, x);
    result = _dxfComputeSubComplexFloat(result, temp);
    result = _dxfComputeSqrtComplexFloat(result);
    REAL(temp) = 0;
    IMAG(temp) = 1;
    temp = _dxfComputeMulComplexFloat(temp, x);
    result = _dxfComputeAddComplexFloat(temp, result);
    result = _dxfComputeLnComplexFloat(result);
    REAL(temp) = 0;
    IMAG(temp) = -1;
    result = _dxfComputeMulComplexFloat(temp, result);

    return result;
}
complexFloat
_dxfComputeAcosComplexFloat(complexFloat x)
{
    complexFloat result;
    complexFloat temp;

    REAL(result) = 1;
    IMAG(result) = 0;
    temp = _dxfComputeMulComplexFloat(x, x);
    result = _dxfComputeSubComplexFloat(temp, result);
    result = _dxfComputeSqrtComplexFloat(result);
    result = _dxfComputeAddComplexFloat(x, result);
    result = _dxfComputeLnComplexFloat(result);
    REAL(temp) = 0;
    IMAG(temp) = -1;
    result = _dxfComputeMulComplexFloat(temp, result);

    return result;
}
/* 1/2 i ln[(1+ix) / (1-ix)] */
complexFloat
_dxfComputeAtanComplexFloat(complexFloat x)
{
    complexFloat result;
    complexFloat ix;
    complexFloat temp;

    REAL(temp) = 0;
    IMAG(temp) = 1;
    ix = _dxfComputeMulComplexFloat(temp, x);
    REAL(temp) = 1;
    IMAG(temp) = 0;
    result = _dxfComputeAddComplexFloat(temp, ix);
    temp = _dxfComputeSubComplexFloat(temp, ix);
    if (REAL(temp) != 0 || IMAG(temp) != 0)
	result = _dxfComputeDivComplexFloat(result, temp);
    result = _dxfComputeLnComplexFloat(result);
    REAL(temp) = 0;
    IMAG(temp) = -0.5;
    result = _dxfComputeMulComplexFloat(result, temp);

    return result;
}
