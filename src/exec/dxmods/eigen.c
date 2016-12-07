/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * eigen.c - routines to calculate eigenvectors and eigenvalues of a
 *           symmetric matrix
 */
#include <string.h>
#include <dx/dx.h>
#include <math.h>
#include "eigen.h"

#include <dxconfig.h>


#define CYCLE(a,i,j,k,l) g=a[i][j]; h=a[k][l]; a[i][j]=g-s*(h+g*tau);\
					       a[k][l]=h+s*(g-h*tau);
Error
_dxfEigen(float **a, int n, float d[], float **v)
{
    float threshold, angle, tau, t, sum, s, h, g, c, *b, *z;
    int j, Q, P, i, row, col;

    /* check if matrix symmetric */
    for (row = 1; row <= n; row++) {
	for (col = 1; col <= n; col++) {
	    if (a[row][col] != a[col][row]) {
    		DXSetError (ERROR_DATA_INVALID, "A tensor must be a 3X3 symmetric matrix.");
    		return ERROR;
	    }
	}
    }

    b = _dxfEigenVector(1, n);
    z = _dxfEigenVector(1, n);

    for (P = 1; P <= n; P++) {
	for (Q = 1; Q <= n; Q++) v[P][Q]=0.0;
	v[P][P] = 1.0;
    }

    for (P = 1; P <= n; P++) {
	b[P] = d[P]= a[P][P];
	z[P] = 0.0;
    }

    for (i = 1; i <= 50; i++) {
	sum = 0.0;
	for (P = 1; P <= n-1; P++) {
	    for (Q = P+1; Q<=n; Q++)
		sum += fabs(a[P][Q]);
	}
	if (sum == 0.0) {
    	    _dxfEigenFreeVector(z,1);
	    _dxfEigenFreeVector(b,1);
	    return OK;
	}
	if (i < 4)
	    threshold = 0.2 * sum / (n*n);
	else
	    threshold = 0.0;
	for (P = 1; P <= n-1; P++) {
	    for (Q = P+1; Q <= n; Q++) {
		g=100.0*fabs(a[P][Q]);
		if (i > 4 && (float)(fabs(d[P]) + g) == (float)fabs(d[P])
	    	    && (float)(fabs(d[Q]) + g) == (float)fabs(d[Q]))
	            a[P][Q]=0.0;
		else if (fabs(a[P][Q]) > threshold) {
		    h = d[Q] - d[P];
		    if ((float)(fabs(h) + g) == (float)fabs(h))
			t = (a[P][Q]) / h;
		    else {
			angle = 0.5 * h / (a[P][Q]);
			t = 1.0 / (fabs(angle) + sqrt(1.0 + angle*angle));
			if (angle < 0.0) t = -t;
	    	    }
	    	    c = 1.0 / sqrt(1 + t * t);
		    s = t * c;
		    tau = s / (1.0 + c);
		    h = t * a[P][Q];
		    z[P] = z[P] - h;
		    z[Q] = z[Q] + h;
		    d[P] = d[P] - h;
		    d[Q] = d[Q] + h;
		    a[P][Q] = 0.0;
		    for (j = 1; j <= P-1; j++) {
			CYCLE(a, j, P, j, Q)
		    }
		    for (j = P+1; j <= Q-1; j++) {
			CYCLE(a, P, j, j, Q)
		    }
		    for (j = Q+1; j <= n; j++) {
			CYCLE(a, P, j, Q, j)
		    }
	      	    for (j = 1; j <= n; j++) {
			CYCLE(v, j, P, j, Q)
		    }
		}
    	    }
	}
	for (P = 1; P <= n; P++) {
    	    b[P] = b[P] + z[P];
	    d[P] = b[P];
    	    z[P] = 0.0;
	}
    }

    /* evidently it shouldn't get here */
    DXSetError (ERROR_DATA_INVALID, "Error calculating eigenvectors.");
    return ERROR;
}
#undef CYCLE

float *_dxfEigenVector(int start_vector, int end_vector)
{
    float *v;

    /* Allocates a memory for a vector */

    v=(float *)DXAllocate((unsigned) (end_vector-start_vector+1)*sizeof(float));
    if (!v) 
	return NULL;
    return v-start_vector;
}

float **_dxfEigenMatrix(int start_matrix_row, int end_matrix_row, int start_matrix_col, int end_matrix_col)
{
    int i;
    float **m;

    /* Allocates memory for a matrix */

    m=(float **) DXAllocate((unsigned) (end_matrix_row-start_matrix_row+1)*sizeof(float*));
    if (!m) 
	return NULL;
    m -= start_matrix_row;

    for(i=start_matrix_row;i<=end_matrix_row;i++) {
	m[i]=(float *) DXAllocate((unsigned) (end_matrix_col-start_matrix_col+1)*sizeof(float));
	if (!m[i])
	    return NULL;
	m[i] -= start_matrix_col;
    }
    return m;
}

void _dxfEigenFreeVector(float *vec, int start_vector)
{
    /* fress memory Allocated by _dxfEigenVector */
    DXFree((char*) (vec+start_vector));
}

void _dxfEigenFreeMatrix(float **m, int start_matrix_row, int end_matrix_row, int start_matrix_col)
{
    int i;

    /* frees memory Allocated by _dxfEigenMatrix */

    for(i=end_matrix_row;i>=start_matrix_row;i--) 
	DXFree((char*) (m[i]+start_matrix_col));
    DXFree((char*) (m+start_matrix_row));
}

float **_dxfEigenConvertMatrix(float *a, int start_matrix_row, int end_matrix_row, int start_matrix_col, int end_matrix_col)
{
    int i,j,nrow,ncol;
    float **m;

    /* 
     * Allocates a float matrix 
     * m[start_matrix_row..end_matrix_row][start_matrix_col..end_matrix_col]
     * 
     */ 

    nrow=end_matrix_row-start_matrix_row+1;
    ncol=end_matrix_col-start_matrix_col+1;
    m = (float **) DXAllocate((unsigned) (nrow)*sizeof(float*));
    if (!m) 
	return NULL;
    m -= start_matrix_row;
    for(i=0,j=start_matrix_row;i<=nrow-1;i++,j++) 
	m[j]=a+ncol*i-start_matrix_col;
    return m;
}

void _dxfEigenFreeConvertMatrix(float**c, int start_matrix_row)
{
    /* free the memory Allocated by _dxfEigenConvertMatrix */
    DXFree((char*) (c+start_matrix_row));
}
