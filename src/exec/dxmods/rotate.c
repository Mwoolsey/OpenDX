/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dxmods/rotate.c,v 1.3 1999/05/10 15:45:30 gda Exp $
 */

#include <dxconfig.h>


#include <dx/dx.h>
#include <ctype.h>

static int getdim(Object dimo, int *dim);
static int getmatrix(int dim, float deg, Matrix *m);

/* should the default rotation be 90? */
#define DEF_AXIS  1
#define DEF_DEG   0.0

#define MAXINPUTS 45  /* obj + 2 inputs w/defaults + 2 w/o + 20 repeat pairs */

int
m_Rotate(Object *in, Object *out)
{
    float deg = DEF_DEG;
    int i, dim = DEF_AXIS;
    int seen_null = 0;
    int say_warn = 0;
    Object xformed = NULL;
    Matrix m;

    out[0] = NULL;

    if (!in[0]) {
	DXSetError(ERROR_BAD_PARAMETER, "#10000", "input");
	return ERROR;
    }

    i = 1;

    /* handle the first 2 parameters because they have default values. 
     */
    if (!in[1] || !in[2]) {

        if (in[1] && !getdim(in[1], &dim))
	    return ERROR;

	if (in[2] && !DXExtractFloat(in[2], &deg)) {
	    DXSetError(ERROR_BAD_PARAMETER, "#10080", "rotation");
            return ERROR;
	}
	
	if (!getmatrix(dim, deg, &m))
	    return ERROR;
	
	xformed = (Object)DXNewXform(in[0], m);
	if (!xformed)
	    return ERROR;
	
        /* start at 3rd parm */
        i = 3;
    }
    
    
    /* parse rest of parms in pairs: axis letter, number of degrees 
     *  if either of the first two parms was null, this loop starts at 3.
     *  otherwise it starts at 1.
     */
    for ( ; i<MAXINPUTS; i+=2) {
	
        /* allow a pair of nulls, but if one is non-null, make sure it
         *  has a matching parameter.
         */
        if (!in[i] && in[i+1]) {
	    DXSetError(ERROR_BAD_PARAMETER, "#10000", "axis");
	    goto error;
	}

        if (in[i] && !in[i+1]) {
            DXSetError(ERROR_BAD_PARAMETER, "#10000", "rotation");
	    goto error;
	}
	
	if (!in[i] && !in[i+1]) {
	    seen_null = 1;
	    continue;
	}

        if (seen_null) {
            say_warn = 1;
	    seen_null = 0;
	}

	/* get axis and number of degrees of rotation.
	 */
	if (!getdim(in[i], &dim))
	    goto error;

        if (!DXExtractFloat(in[i+1], &deg)) {
            DXSetError(ERROR_BAD_PARAMETER, "#10080", "rotation");
	    goto error;
	}

	/* turn axis & degrees into a transformation matrix
	 */
	if (!getmatrix(dim, deg, &m)) 
	    goto error;
	
	/* the following shuffling is done so that if there is an
	 *  error, we can delete the partially rotated object.
	 *  out[0] get zeroed so that if there is an error in a
	 *  subsequent set of rotations, we can just call DXErrorReturn.
	 */
	out[0] = (Object)DXNewXform((xformed ? xformed : in[0]), m);
	
	if (!out[0])
	    goto error;
	
	xformed = out[0];
	out[0] = NULL;
	    
    }

    if (say_warn)
        DXWarning("skipping null objects in rotation list");

    out[0] = xformed;
    return(out[0] ? OK : ERROR);

  error:
    DXDelete(xformed);
    out[0] = NULL;
    return ERROR;

}

static int getdim(Object dimo, int *dim)
{
    char *ax;

    /* if string given, convert to integer */
    if (DXExtractString(dimo, &ax)) {

	if (ax[1] != '\0')
	    goto error;

	switch(ax[0]) {
	  case 'x':
	  case 'X': *dim = 0; break;
	  case 'y':
	  case 'Y': *dim = 1; break;
	  case 'z':
	  case 'Z': *dim = 2; break;
	  default:
	    goto error;

	}

    } else if (DXExtractInteger(dimo, dim)) {

	if (*dim < 0 || *dim > 3)
	    goto error;

    } else
	goto error;


    return OK;

  error:
    DXErrorReturn(ERROR_BAD_PARAMETER, "axis must be X, Y, Z or 0, 1, 2");

}


static int getmatrix(int dim, float deg, Matrix *m)
{
    /* set rotation matrix */
    switch (dim) {
      case 0: 
	*m = DXRotateX(deg * DEG);  
	break;
      case 1: 
	*m = DXRotateY(deg * DEG);  
	break;
      case 2: 
	*m = DXRotateZ(deg * DEG);  
	break;
      default:
	DXSetError(ERROR_BAD_PARAMETER, "axis must be X, Y, Z or 0, 1, 2");
	return ERROR;
    }

    return OK;
}
