/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * Header: /usr/people/gresh/code/svs/src/libdx/RCS/permute.c,v 5.0 92/11/12 09:08:32 svs Exp Locker: gresh 
 *
 */

#include <dxconfig.h>



#include <string.h>
#include <dx/dx.h>
 
/*
 * Inputs are: a pointer to the output array, the dimensionality of the 
 * output array, and the "strides" along each dimension in the ouput array.  
 * The stride for a given dimension is the number of elements in the 
 * linearized array that you move for a unit move along that dimension.  
 * For example, for a 5x4x3x2 array, the strides for each dimension are
 * respectively 4x3x2, 3x2, 2, and 1 (assuming the last dimension varies
 * fastest).  Finally, you give an array of counts along each output
 * dimension, which may be the same as or smaller than the length along
 * the corresponding dimension of the input array.  The input array is
 * represented by a pointer that has already been offset by the starting
 * point along each dimension, and the set of strides along each
 * dimension.  If the input array pointer is NULL, zero fill the outputs.
 *
 */
 
/* this should be void *, but i need to increment it by 1 and most
 *  compilers won't let you increment a void. 
 */
#define PPP unsigned char *


void _dxfPermute(
    int dim,            /* the number of dimensions in the output array */
    PPP out,            /* the output array */
    int *ostrides,      /* the strides for each dimension in output array */
    int *counts,        /* the counts in each dimension */
    int size,           /* the number of bytes in one item */
    PPP in,             /* the input array */
    int *istrides       /* the strides for each dimension in input array */
) {
    int k, os, n, is;
 
    os = ostrides[0] * size;
    is = istrides[0] * size;
    n = counts[0];
 
    if (dim == 1) {
	if (in == NULL)
	    for (k=0; k<n; k++, out+=os, in+=is)
		memset (out, '\0', size);
	
	else if (is == size && os == size)
	    memcpy (out, in, n * size);
	
	else if (size == sizeof(unsigned int))
	    for (k=0; k<n; k++, out+=os, in+=is)
		*(unsigned int *)out = *(unsigned int *)in;
	
	else if (size == sizeof(unsigned char))
	    for (k=0; k<n; k++, out+=os, in+=is)
		*(unsigned char *)out = *(unsigned char *)in;
	
	else
	    for (k=0; k<n; k++, out+=os, in+=is)
		memcpy (out, in, size);
	
    } else
        for (k=0; k<n; k++, out+=os, in+=is)
	    _dxfPermute (dim-1, out, ostrides+1, counts+1, size, in, istrides+1);


}
