/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


/*
 * $Header: /src/master/dx/src/exec/dxmods/_refinereg.c,v 1.5 2000/08/24 20:04:19 davidt Exp $
 */

#include <stdio.h>
#include <string.h>
#include "math.h"
#include <dx/dx.h>
#include "_refine.h"

#define MAXDIM	16

static Array RefineDepPos(Array, int, int, int *, int *);
static Array RefineDepPosIrreg(Array, int, int, int *, int *);
static Array RefineDepCon(Array, int, int, int *, int *);
static Array RefineDepConIrreg(Array, int, int, int *, int *);
static Array RefineRegReferences(Array, int, int, int *, int *, int);

#if 0
static Array RefineDepConBoolean(Array, int, int, int *, int *);
#endif

#define REF_POSITIONS		1
#define REF_CONNECTIONS		2

Field
_dxfRefineReg(Field field, int n)
{
    Array  inArray, outArray;
    int	   nDim;
    int    inCounts[MAXDIM], outCounts[MAXDIM];
    int    i;
    char   *name;
    Object attr;
    char   *str;
    float origin[MAXDIM];
    float deltas[MAXDIM*MAXDIM];
    int   counts[MAXDIM*MAXDIM];
    int   regDim, totItems;
    int   meshOffsets[MAXDIM];
    int	  irreg_positions;

    /*
     * Create refined regular positions and connections.  In the subsequent
     * loop we will traverse the list of component arrays refining all 
     * non-grid arrays in place.  Since we cannot refine grid arrays in place,
     * we need to do so before we enter the later loop.  We now that 
     * connections are grid or we wouldn't have gotten here; we need to test
     * positions.
     */
    inArray = (Array)DXGetComponentValue(field, "connections");
    if (! inArray)
	goto error;

    if (DXGetArrayClass(inArray) != CLASS_MESHARRAY)
	goto error;
    
    DXQueryGridConnections(inArray, &nDim, inCounts);

    totItems = 1;
    for (i = 0; i < nDim; i++)
    {
	int k;

	outCounts[i] = ((inCounts[i] - 1) * (1 << n)) + 1;

	k = totItems * outCounts[i];
	if (k / totItems != outCounts[i])
	{
	    int j;
	    for (j = 1; j < 32; j++)
	    {
		outCounts[i] = ((inCounts[i] - 1) * (1 << n)) + 1;
		k = totItems * outCounts[i];
		if (k / totItems != outCounts[i])
		    break;
	    }

	    DXSetError(ERROR_BAD_PARAMETER, "#10040", "level", 0, (n-1));
	    goto error;
	}

	totItems = k;
    }

    if (NULL == (outArray = DXMakeGridConnectionsV(nDim, outCounts)))
	goto error;

    if (DXGetMeshOffsets((MeshArray)inArray, meshOffsets))
    {
	for (i = 0; i < nDim; i++)
	    meshOffsets[i] = meshOffsets[i] << n;
	
	DXSetMeshOffsets((MeshArray)outArray, meshOffsets);
    }
    
    DXSetComponentValue(field, "connections", (Object)outArray);

    inArray = (Array)DXGetComponentValue(field, "positions");
    if (! inArray)
	goto error;

    if (DXQueryGridPositions(inArray, &regDim, counts, origin, deltas))
    {
	for (i = 0; i < regDim*regDim; i++)
	    deltas[i] /= (1 << n);
	
	totItems = 1;
	for (i = 0; i < regDim; i++)
	    if (counts[i])
	    {
		int k;

		counts[i] = ((counts[i] - 1) * (1 << n)) + 1;

		k = totItems * counts[i];
		if (k / totItems != counts[i])
		{
		    int j;
		    for (j = 1; j < 32; j++)
		    {
			outCounts[i] = ((inCounts[i] - 1) * (1 << n)) + 1;
			k = totItems * outCounts[i];
			if (k / totItems != outCounts[i])
			    break;
		    }

		    DXSetError(ERROR_BAD_PARAMETER, "#10040", "level", 0, (n-1));
		    goto error;
		}

		totItems = k;
	    }

	outArray = DXMakeGridPositionsV(regDim, counts, origin, deltas);
	if (NULL == outArray)
	    goto error;

	DXSetComponentValue(field, "positions", (Object)outArray);
	irreg_positions = 0;
    }
    else
	irreg_positions = 1;
    
    i = 0;
    while(NULL !=
	(inArray=(Array)DXGetEnumeratedComponentValue(field, i++, &name)))
    {
	Array out = inArray;

	/*
	 * Skip connections and regular positions
	 */
	if (!strcmp(name, "connections") ||
	    !strcmp(name, "box") 	 ||
	    (!strcmp(name, "positions") && !irreg_positions))
	    continue;

	attr = DXGetComponentAttribute(field, name, "dep");
	if (attr)
	{
	    if (! DXGetComponentAttribute(field, name, "ref"))
	    {
		str = DXGetString((String)attr);

		/*
		 * _dxfRefine it in place if it is dep on positions
		 * or connections and is not positions  (unless
		 * positions are irregular) or connections
		 */

		if (!strcmp(str, "positions"))
		{
		    if (strcmp(name, "positions") || irreg_positions)
		    {
			out = RefineDepPos(inArray, n, nDim,
					    inCounts, outCounts);

			if (! out)
			    goto error;
		    }
		}
		else if (strcmp(name, "connections")
				&& !strcmp(str, "connections"))
		{
		    out = RefineDepCon(inArray, n, nDim, inCounts, outCounts);

		    if (! out)
			goto error;
		}
	    }
	}
	else if (NULL != (attr = DXGetComponentAttribute(field, name, "ref")))
	{
	    str = DXGetString((String)attr);

	    if (!strcmp(str, "positions"))
	    {
		out = RefineRegReferences(inArray, nDim, n, 
				    inCounts, outCounts, REF_POSITIONS);

		if (! out)
		    goto error;
	    }
	    else if (strcmp(name, "connections")
				&& !strcmp(str, "connections"))
	    {
		out = RefineRegReferences(inArray, nDim, n, 
				    inCounts, outCounts, REF_CONNECTIONS);

		if (! out)
		    goto error;
	    }
	}

	if (! DXSetComponentValue(field, name, (Object)out))
	{
	    DXDelete((Object)out);
	    goto error;
	}
    }

    DXDeleteComponent(field, "data statistics");

    if (! DXEndField(field))
	goto error;

    return field;

error:

    return NULL;
}

static Array
RefineDepCon(Array inArray, int n, int nDim, int *inCounts, int *outCounts)
{
    Array outArray;

    outArray = NULL;

    /*
     * If its a constant array, then so's the result
     */
    if (DXQueryConstantArray(inArray, NULL, NULL))
    {
	int num, i, rank, shape[32];
	Type type;
	Category cat;

	DXGetArrayInfo(inArray, NULL, &type, &cat, &rank, shape);

	num = 1;
	for (i = 0; i < nDim; i++)
	    num *= outCounts[i];

	outArray = (Array)DXNewConstantArrayV(num, 
		    DXGetConstantArrayData(inArray), type, cat, rank, shape);

	if (! outArray)
	    return NULL;
    }
    /*
     * If its a varying regular array and
     * nDim == 1, we can create a regular output.
     */
    else if (DXGetArrayClass(inArray) == CLASS_REGULARARRAY && nDim == 1)
    {
	int num, i, size, aDim;
	Type type;
	Pointer o, d;

	o = d = NULL;

	size = DXGetItemSize(inArray);
	
	o = DXAllocate(size);
	d = DXAllocate(size);
	if (! o || ! d)
	    goto block_error;
	
	DXGetRegularArrayInfo((RegularArray)inArray, NULL, o, d);
	DXGetArrayInfo(inArray, NULL, &type, NULL, NULL, &aDim);


#define REDUCE_DELTAS(type)						\
{									\
    type *ptr = (type *)d;						\
    for (i = 0; i < aDim; i++)						\
	ptr[i] = ((*inCounts - 1)) / ((*outCounts) - 1);		\
}

	if (*inCounts > 1)
	    switch(type) {
		case TYPE_DOUBLE: REDUCE_DELTAS(double); break;	
		case TYPE_FLOAT:  REDUCE_DELTAS(float);  break;	
		case TYPE_INT:    REDUCE_DELTAS(int);    break;	
		case TYPE_UINT:   REDUCE_DELTAS(uint);   break;	
		case TYPE_SHORT:  REDUCE_DELTAS(short);  break;	
		case TYPE_USHORT: REDUCE_DELTAS(ushort); break;	
		case TYPE_BYTE:   REDUCE_DELTAS(byte);   break;	
		case TYPE_UBYTE:  REDUCE_DELTAS(ubyte);  break;	
	        default: break;
	    }

#undef REDUCE_DELTAS
	    
	num = 1;
	for (i = 0; i < nDim; i++)
	    num *= outCounts[i];
    
	outArray = (Array)DXNewRegularArray(type, aDim, num, o, d);

block_error:
	DXFree((Pointer)d);
	DXFree((Pointer)o);
    }
    else
    {
	outArray = RefineDepConIrreg(inArray, n, nDim, inCounts, outCounts);
    }

    return outArray;
}

static Array
RefineDepConIrreg(Array inArray, int n, int nDim, int *inCounts, int *outCounts)
{
    Array outArray;
    Type t; Category c; int r, s[32];
    int i, j, k, nInItems, nOutItems, itemSize, count;
    int oCC[32], skip[32];
    int  iknt[32], oknt[32];
    char *ibase[32], *obase[32];
    char *inBase, *outBase, *oPtr, *inBuffer;

    inBuffer = NULL;
    outArray = NULL;

    /*
     * Get stats on input array and create an output array of the same type
     */
    DXGetArrayInfo(inArray, &nInItems, &t, &c, &r, s);
    outArray = DXNewArrayV(t, c, r, s);
    if (! outArray)
	goto error;

    /*
     * Count is the number of output samples along a single axis
     * produced from each input sample.
     */
    count = 1 << n;

    /* 
     * Number of output samples is the number of input samples times the
     * number of output samples derived from each.  This, in turn, is the
     * count along one axis raised to the power of the dimensionality of 
     * the space.
     */
    nOutItems = nInItems * pow((double)count, (double)nDim);

    /* 
     * Get number of output values along each axis.  Since we are dep on
     * connections, this is the outCounts - 1, since the outCounts count the
     * number of positions along the axis.  While we are at it, reverse the 
     * indices.
     */
    for (i = 0; i < nDim; i++)
	oCC[(nDim-1) - i] = outCounts[i] - 1;

    itemSize = DXGetItemSize(inArray);

    /*
     * Get the input data.
     */
    inBuffer = (char *)DXGetArrayDataLocal(inArray);
    if (! inBuffer)
    {
	DXResetError();
	inBuffer = (char *)DXGetArrayData(inArray);
    }

    /*
     * DXAllocate the output array buffer
     */
    if (! DXAddArrayData(outArray, 0, nOutItems, NULL))
	goto error;

    /* 
     * The outermost loop will index through the input samples.  Within
     * this loop we will follow a base that represents the first sample 
     * location in the output in which to copy the input sample.  Given 
     * this base we enter a nDim-dimensional loop which stuffs the input
     * sample into each appropriate output slot.  To do so we need to set
     * up some skip sizes.  Here we do the index reversal: remember, Z 
     * varies fastest.  So the Z skip is the item size, the Y skip is the 
     * item size times the Z count (eg count[nDim-1]) and so on.
     */
    skip[0] = itemSize;
    for (i = 1; i < nDim; i++)
	skip[i] = skip[i-1] * oCC[i-1];
    
    /*
     * And off we go.
     */
    inBase  = inBuffer;
    outBase = (char * )DXGetArrayData(outArray);
    
    for (i = 0; i < nDim; i++)
    {
	obase[i] = outBase;
	oknt[i]  = 0;
    }

    for (i = 0; i < nInItems; i++)
    {
	for (j = 0; j < nDim; j++)
	{
	    ibase[j] = outBase;
	    iknt[j] = 0;
	}

	while(1)
	{
	    oPtr = ibase[0];
	    for (j = 0; j < count; j++)
	    {
		memcpy(oPtr, inBase, itemSize);
		oPtr += itemSize;
	    }

	    for (j = 1; j < nDim; j++)
	    {
		iknt[j] ++;
		if (iknt[j] < count)
		    break;
	    }

	    if (j >= nDim)
		break;
	
	    ibase[j] += skip[j];

	    for (k = j-1; k >= 0; --k)
	    {
		ibase[k] = ibase[j];
		iknt[k]  = 0;
	    }
	}

	inBase  += itemSize;

	for (j = 0; j < nDim; j++)
	{
	    oknt[j] += count;
	    if (oknt[j] < oCC[j])
		break;
	}
	    
	if (j >= nDim)
	    break;
	    
	obase[j] += count * skip[j];

	for (k = j-1; k >= 0; k--)
	{
	    obase[k] = obase[j];
	    oknt[k]  = 0;
	}

	outBase = obase[0];
    }
	
    if (inBuffer != (char *)DXGetArrayData(inArray))
	DXFreeArrayDataLocal(inArray, (Pointer)inBuffer);

    return outArray;

error:

    DXDelete((Object)outArray);
    DXFree((Pointer)inBuffer);

    return NULL;
}

#if 0
static Array
RefineDepConBoolean(Array inArray, int n,
			int nDim, int *inCounts, int *outCounts)
{
    Array outArray;
    Type t; Category c; int r, s[32];
    int i, j, k, nInItems, nOutItems, itemSize, zSize, count;
    int oCC[32], skip[32];
    int  iknt[32], oknt[32];
    char *ibase[32], *obase[32];
    char *inBase, *outBase, *oPtr, *inBuffer;

    inBuffer = NULL;
    outArray = NULL;

    /*
     * Get stats on input array and create an output array of the same type
     */
    DXGetArrayInfo(inArray, &nInItems, &t, &c, &r, s);

    if ((t != TYPE_BYTE && t != TYPE_UBYTE) || c != CATEGORY_REAL)
    {
	DXSetError(ERROR_DATA_INVALID, 
		"invalid data must be byte or ubyte reals");
	goto error;
    }

    outArray = DXNewArrayV(t, c, r, s);
    if (! outArray)
	goto error;

    /*
     * Count is the number of output samples along a single axis
     * produced from each input sample.
     */
    count = 1 << n;

    /* 
     * Number of output samples is the number of input samples times the
     * number of output samples derived from each.  This, in turn, is the
     * count along one axis raised to the power of the dimensionality of 
     * the space.
     */
    nOutItems = nInItems * pow((double)count, (double)nDim);

    /* 
     * Get number of output values along each axis.  Since we are dep on
     * connections, this is the outCounts - 1, since the outCounts count the
     * number of positions along the axis.  While we are at it, reverse the 
     * indices.
     */
    for (i = 0; i < nDim; i++)
	oCC[(nDim-1) - i] = outCounts[i] - 1;

    itemSize = DXGetItemSize(inArray);

    if (itemSize != 1)
    {
	DXSetError(ERROR_DATA_INVALID, 
		"invalid data must be scalar or 1-vector");
	goto error;
    }

    /*
     * Get the input data.
     */
    inBuffer = (char *)DXGetArrayDataLocal(inArray);
    if (! inBuffer)
    {
	DXResetError();
	inBuffer = (char *)DXGetArrayData(inArray);
    }

    /*
     * DXAllocate the output array buffer
     */
    if (! DXAddArrayData(outArray, 0, nOutItems, NULL))
	goto error;

    /* 
     * The outermost loop will index through the input samples.  Within
     * this loop we will follow a base that represents the first sample 
     * location in the output in which to copy the input sample.  Given 
     * this base we enter a nDim-dimensional loop which stuffs the input
     * sample into each appropriate output slot.  To do so we need to set
     * up some skip sizes.  Here we do the index reversal: remember, Z 
     * varies fastest.  So the Z skip is the item size, the Y skip is the 
     * item size times the Z count (eg count[nDim-1]) and so on.
     */
    skip[0] = itemSize;
    for (i = 1; i < nDim; i++)
	skip[i] = skip[i-1] * oCC[i-1];
    
    /*
     * And off we go.
     */
    inBase  = inBuffer;
    outBase = (char * )DXGetArrayData(outArray);
    
    for (i = 0; i < nDim; i++)
    {
	obase[i] = outBase;
	oknt[i]  = 0;
    }

    for (i = 0; i < nInItems; i++)
    {
	for (j = 0; j < nDim; j++)
	{
	    ibase[j] = outBase;
	    iknt[j] = 0;
	}

	while(1)
	{
	    oPtr = ibase[0];
	    for (j = 0; j < count; j++)
		*oPtr++ = *inBase;

	    for (j = 1; j < nDim; j++)
	    {
		iknt[j] ++;
		if (iknt[j] < count)
		    break;
	    }

	    if (j >= nDim)
		break;
	
	    ibase[j] += skip[j];

	    for (k = j-1; k >= 0; --k)
	    {
		ibase[k] = ibase[j];
		iknt[k]  = 0;
	    }
	}

	inBase  += itemSize;

	for (j = 0; j < nDim; j++)
	{
	    oknt[j] += count;
	    if (oknt[j] < oCC[j])
		break;
	}
	    
	if (j >= nDim)
	    break;
	    
	obase[j] += count * skip[j];

	for (k = j-1; k >= 0; k--)
	{
	    obase[k] = obase[j];
	    oknt[k]  = 0;
	}

	outBase = obase[0];
    }
	
    if (inBuffer != (char *)DXGetArrayData(inArray))
	DXFreeArrayDataLocal(inArray, (Pointer)inBuffer);

    return outArray;

error:

    DXDelete((Object)outArray);
    DXFree((Pointer)inBuffer);

    return NULL;
}
#endif


static Array
RefineDepPos(Array inArray, int n, int nDim, int *inCounts, int *outCounts)
{
    Array outArray;

    outArray = NULL;

    /*
     * If its a constant array, then so's the result
     */
    if (DXQueryConstantArray(inArray, NULL, NULL))
    {
	int num, i, rank, shape[32];
	Type type;
	Category cat;

	DXGetArrayInfo(inArray, NULL, &type, &cat, &rank, shape);

	num = 1;
	for (i = 0; i < nDim; i++)
	    num *= outCounts[i];

	outArray = (Array)DXNewConstantArrayV(num, 
		    DXGetConstantArrayData(inArray), type, cat, rank, shape);

	if (! outArray)
	    return NULL;
    }
    /*
     * If its a varying regular array and
     * nDim == 1, we can create a regular output.
     */
    else if (DXGetArrayClass(inArray) == CLASS_REGULARARRAY && nDim == 1)
    {
	int num, i, size, aDim;
	Type type;
	Pointer o, d;

	o = d = NULL;

	size = DXGetItemSize(inArray);
	
	o = DXAllocate(size);
	d = DXAllocate(size);
	if (! o || ! d)
	    goto block_error;
	
	DXGetRegularArrayInfo((RegularArray)inArray, NULL, o, d);
	DXGetArrayInfo(inArray, NULL, &type, NULL, NULL, &aDim);


#define REDUCE_DELTAS(type)						\
{									\
    type *dptr = (type *)d;						\
    type *optr = (type *)o;						\
    for (i = 0; i < aDim; i++)						\
	dptr[i]  = ((*inCounts - 1)) / ((*outCounts) - 1);		\
	optr[i] -= (dptr[i]/2.0);					\
}

	if (*inCounts > 1)
	    switch(type) {
		case TYPE_DOUBLE: REDUCE_DELTAS(double); break;	
		case TYPE_FLOAT:  REDUCE_DELTAS(float);  break;	
		case TYPE_INT:    REDUCE_DELTAS(int);    break;	
		case TYPE_UINT:   REDUCE_DELTAS(uint);   break;	
		case TYPE_SHORT:  REDUCE_DELTAS(short);  break;	
		case TYPE_USHORT: REDUCE_DELTAS(ushort); break;	
		case TYPE_BYTE:   REDUCE_DELTAS(byte);   break;	
		case TYPE_UBYTE:  REDUCE_DELTAS(ubyte);  break;	
	        default: break;
	    }

#undef REDUCE_DELTAS
	    
	num = 1;
	for (i = 0; i < nDim; i++)
	    num *= outCounts[i];
    
	outArray = (Array)DXNewRegularArray(type, aDim, num, o, d);

block_error:
	DXFree((Pointer)d);
	DXFree((Pointer)o);
    }
    else
    {
	outArray = RefineDepPosIrreg(inArray, n, nDim, inCounts, outCounts);
    }

    return outArray;
}

static Array
RefineDepPosIrreg(Array inArray, int n, int nDim, int *inCounts, int *outCounts)
{
    Array    outArray;
    Type     type;
    Category cat;
    int	     nInItems, nOutItems, r, s[MAXDIM];
    byte    *dIn;
    float    *dOut;
    float    *src;
    float    *dst;
    int	     i, j, k, skip[MAXDIM], counts[MAXDIM];
    int	     level;
    int	     olim, oskp;
    int	     nBytes, nValues;
    int	     oddCount, o;
    float    div;
    float    *buffer;
    struct
    {
	int    inc;
	int    start;
	int    limit;
	int    skip;
	float  *ptr;
	} outerLoop[MAXDIM], innerLoop[MAXDIM], *loop0, *loop1;
    static float divisors[] = {1.0/1.0,  1.0/2.0, 
			       1.0/4.0,  1.0/8.0, 
			       1.0/16.0, 1.0/32.0};
    
    buffer   = NULL;
    outArray = NULL;

    DXGetArrayInfo(inArray, &nInItems, &type, &cat, &r, s);
    if (cat != CATEGORY_REAL)
    {
	DXSetError(ERROR_NOT_IMPLEMENTED, "#11150", "dependent components");
	goto error;
    }

    outArray = DXNewArrayV(type, cat, r, s);

    nBytes   = DXGetItemSize(inArray);
    nValues  = nBytes / DXTypeSize(type);

    nOutItems = 1;
    for (i = 0; i < nDim; i++)
	nOutItems *= outCounts[i];

    /*
     * We are going to hit each output sample once for each input sample
     * that affects it.  Create a local buffer (if possible) for the
     * output values.  Leave the input in global memory.
     */
    buffer = (float *)DXAllocateLocal(nOutItems * nValues * sizeof(float));
    if (! buffer)
    {
	DXResetError();
	buffer = (float *)DXAllocate(nOutItems * nValues * sizeof(float));
    }
    if (! buffer)
	goto error;
    
    /*
     * We will be accumulating in the buffer, so we must zero it first.
     */
    memset(buffer, 0, nOutItems * nValues * sizeof(float));
    
    dIn  = DXGetArrayData(inArray);
    dOut = buffer;

    for (i = 0; i < nDim; i++)
	counts[i] = inCounts[(nDim-1)-i];

    skip[0] = nValues * (1 << n);
    for (i = 1; i < nDim; i++)
	skip[i] = outCounts[nDim-i] * skip[i-1];
	
    /*
     * First we copy the input data into the appropriate slots 
     * of the output data array
     */
    for (i = 0, loop0 = outerLoop; i < nDim; i++, loop0++)
    {
	loop0->ptr    = dOut;
	loop0->skip   = skip[i];
	loop0->inc    = 0;
	loop0->limit  = counts[i];
    }

    olim = outerLoop[0].limit;
    oskp  = skip[0];

    for (;;)
    {
	dst = outerLoop[0].ptr;

#define COPYDATA(type)					\
{							\
    type *tPtr;						\
    register int j;					\
							\
    tPtr = (type *)dIn;					\
    for (i = 0; i < olim; i++)				\
    {							\
	for (j = 0; j < nValues; j++)			\
	    *dst++ += *tPtr++;				\
		    					\
	dst += oskp - nValues;				\
	dIn  = (Pointer)((char *)dIn + nBytes);		\
    }							\
}

	switch(type)
	{
	    case TYPE_DOUBLE: COPYDATA(double); break;
	    case TYPE_FLOAT:  COPYDATA(float);  break;
	    case TYPE_UINT:   COPYDATA(uint);   break;
	    case TYPE_INT:    COPYDATA(int);    break;
	    case TYPE_USHORT: COPYDATA(ushort); break;
	    case TYPE_SHORT:  COPYDATA(short);  break;
	    case TYPE_UBYTE:  COPYDATA(ubyte);  break;
	    case TYPE_BYTE:   COPYDATA(byte);   break;
	    default: break;
	}

#undef COPYDATA

	if (nDim == 0) break;

	for (i = 1, loop0 = outerLoop + 1; i < nDim; i++, loop0++)
	{
	    loop0->inc ++;
	    loop0->ptr += loop0->skip;
	    if (loop0->inc < loop0->limit)
		break;
	}

	if (i == nDim)
	    break;

	for (j = i-1, loop1 = outerLoop + (i-1); j >= 0; j--, loop1--)
	{
	    loop1->ptr = loop0->ptr;
	    loop1->inc = 0;
	}
    }

    /*
     * Now for each level, we insert midpoints.  Assume at this point that
     * the outer loop is set up to index each input point.
     */
    for (level = 0; level < n; level++)
    {

	for (i = 0, loop0 = outerLoop; i < nDim; i++, loop0++)
	{
	    loop0->ptr    = dOut;
	    loop0->inc    = 0;

	    innerLoop[i].skip = loop0->skip >> 1;
	}

	olim = outerLoop[0].limit;
	oskp = outerLoop[0].skip;

	/*
	 * Outer loop: for each input sample....
	 */
	for (;;)
	{
	    /*
	     * Inner loop: accrue input samples onto 3x...x3 neighbors
	     */
	    for (i = 0, src = outerLoop[0].ptr; i < olim; i++, src += oskp)
	    {
		outerLoop[0].inc = i;

		dst = src;

		/*
		 * Set up inner loop structure.  
		 * Boundary conditions vary the inner loop starts and ends
		 */
		loop0 = outerLoop;
		loop1 = innerLoop;
		for (j = 0; j < nDim; j++, loop0++, loop1++)
		{

		    if (loop0->inc == 0)
			loop1->start = 0;
		    else
		    {
			loop1->start = -1;
			dst -= loop1->skip;
		    }
		
		    if (loop0->inc == loop0->limit-1)
			loop1->limit = 1;
		    else
			loop1->limit = 2;
		}

		for (j = 0, loop0 = innerLoop; j < nDim; j++, loop0++)
		{
		    loop0->inc = loop0->start;
		    loop0->ptr = dst;
		}
	    
		/*
		 * Inner loop: add src values to neighbors.
		 */
		for (;;)
		{
		    dst = innerLoop[0].ptr;

		    for (j = innerLoop[0].inc; j < innerLoop[0].limit; j++)
		    {
			if (src != dst) /* neighbors only, not source */
			    for (k = 0; k < nValues; k++)
				dst[k] += src[k];
		    
			dst += innerLoop[0].skip;
		    }
	    
		    if (nDim == 0) 
			break;

		    for (j = 1, loop0 = innerLoop + 1; j < nDim; j++, loop0++)
		    {
			loop0->inc ++;
			loop0->ptr += loop0->skip;
			if (loop0->inc < loop0->limit)
			    break;
		    }

		    if (j == nDim)
			break;

		    loop1 = innerLoop + (j-1);
		    for (k = j-1; k >= 0; k--, loop1--)
		    {
			loop1->ptr = loop0->ptr;
			loop1->inc = loop1->start;
		    }
		}
	    }

	    /*
	     * Update outer loop counters
	     */
	    if (nDim == 0) break;

	    for (i = 1, loop0 = outerLoop + 1; i < nDim; i++, loop0++)
	    {
		loop0->inc ++;
		loop0->ptr += loop0->skip;
		if (loop0->inc < loop0->limit)
		    break;
	    }

	    if (i == nDim)
		break;

	    for (j = i-1, loop1 = outerLoop + (i-1); j >= 0; j--, loop1--)
	    {
		loop1->ptr = loop0->ptr;
		loop1->inc = 0;
	    }
	}

	/*
	 * At this point we have accumulated all midpoint values.  Need
	 * to divide each by number of input samples that accrued there.
	 * This is related to whether the midpoint is the midpoint of an edge,
	 * a face, a cube ...  Which is determined by how many of its indices
	 * are odd: 0, is an input point which does not require dividing;
	 * 1 for an edge midpoint to be divided by 2; 2 for the midpoint of a
	 * face to be divided by 4; 3, a cube, by 8 and so forth.
	 *
	 * Reset outer loop for division pass.  We now reset this loop
	 * structure to index ech valid value in the output array.  This
	 * agrees with the state at the start of the levels loop, which ends
	 * when the division for this level is done.
	 */
	for (i = 0, loop0 = outerLoop; i < nDim; i++, loop0++)
	{
	    loop0->skip   = loop0->skip >> 1;
	    loop0->limit  = (loop0->limit << 1) - 1;
	    loop0->ptr    = dOut;
	    loop0->inc    = 0;
	}

	olim = outerLoop[0].limit;
	oskp = outerLoop[0].skip;

	oddCount = 0;
	for (;;)
	{
	    dst = outerLoop[0].ptr;

	    for (i = 0; i < olim; i++)
	    {
		o = oddCount + (i & 0x01);

		if (o)
		{
		    div = divisors[o];
		    for (j = 0; j < nValues; j++)
			dst[j] *= div;
		}

		dst += oskp;
	    }

	    if (nDim == 0) break;

	    for (i = 1, loop0 = outerLoop + 1; i < nDim; i++, loop0++)
	    {
		loop0->inc ++;
		loop0->ptr += loop0->skip;
		if (loop0->inc < loop0->limit)
		    break;
	    }

	    if (i == nDim)
		break;

	    for (j = i-1, loop1 = outerLoop + (i-1); j >= 0; j--, loop1--)
	    {
		loop1->ptr = loop0->ptr;
		loop1->inc = 0;
	    }

	    oddCount = 0;
	    for (i = 0, loop0 = outerLoop; i < nDim; i++, loop0++)
		oddCount += (loop0->inc & 0x01);
	}
    }

    /*
     * Results are now complete in buffer.  Stick them into the array and
     * return.  Do a type conversion if necessary.
     */

#define COPYOUT_CONVERSION(type)					\
	{								\
	    register float *limit;					\
	    register float *srcPtr;					\
	    register type  *dstPtr;					\
									\
	    if (! DXAddArrayData(outArray, 0, nOutItems, NULL))		\
		goto error;						\
	    								\
	    dstPtr = (type *)DXGetArrayData(outArray);			\
	    srcPtr  = buffer;						\
									\
	    limit = srcPtr + nOutItems*nValues;				\
									\
	    while(srcPtr < limit)					\
		*dstPtr++ = (type)*srcPtr++;				\
	}


    switch(type)
    {
	case TYPE_DOUBLE: COPYOUT_CONVERSION(double); break;
	case TYPE_FLOAT:
	    if (! DXAddArrayData(outArray, 0, nOutItems, (Pointer)buffer))
		goto error;
	    break;
	case TYPE_UBYTE:  COPYOUT_CONVERSION(ubyte); break;
	case TYPE_BYTE:   COPYOUT_CONVERSION(byte); break;
	case TYPE_USHORT: COPYOUT_CONVERSION(ushort); break;
	case TYPE_SHORT:  COPYOUT_CONVERSION(short); break;
	case TYPE_INT:    COPYOUT_CONVERSION(int); break;
	case TYPE_UINT:   COPYOUT_CONVERSION(uint); break;
        default: break;
    }

#undef COPYOUT_CONVERSION


    DXFree((Pointer)buffer);
    return outArray;

error:

    DXFree((Pointer)buffer);
    return NULL;
}

#if 0
static Array
RefineDepPosIrregBoolean(Array inArray, int n,
			    int nDim, int *inCounts, int *outCounts)
{
    Array    outArray;
    Type     type;
    Category cat;
    int	     nInItems, nOutItems, r, s[MAXDIM];
    byte     *dIn;
    byte     *dOut;
    byte     *src;
    byte     *dst;
    int	     i, j, k, skip[MAXDIM], counts[MAXDIM];
    int	     level;
    int	     olim, oskp;
    int	     nBytes, nValues;
    int	     oddCount, o;
    byte     *buffer;
    struct
    {
	int    inc;
	int    start;
	int    limit;
	int    skip;
	byte  *ptr;
	} outerLoop[MAXDIM], innerLoop[MAXDIM], *loop0, *loop1;
    
    buffer   = NULL;
    outArray = NULL;

    DXGetArrayInfo(inArray, &nInItems, &type, &cat, &r, s);
    if (cat != CATEGORY_REAL)
    {
	DXSetError(ERROR_NOT_IMPLEMENTED, "#11150", "dependent components");
	goto error;
    }

    if (type != TYPE_BYTE && type != TYPE_UBYTE)
    {
	DXSetError(ERROR_DATA_INVALID, "invalid flags must be byte or ubyte");
	goto error;
    }

    outArray = DXNewArrayV(type, cat, r, s);

    nBytes   = 1;
    nValues  = 1;

    nOutItems = 1;
    for (i = 0; i < nDim; i++)
	nOutItems *= outCounts[i];

    /*
     * We are going to hit each output sample once for each input sample
     * that affects it.  Create a local buffer (if possible) for the
     * output values.  Leave the input in global memory.
     */
    buffer = (byte *)DXAllocateLocal(nOutItems * nValues * sizeof(byte));
    if (! buffer)
    {
	DXResetError();
	buffer = (byte *)DXAllocate(nOutItems * nValues * sizeof(byte));
    }
    if (! buffer)
	goto error;
    
    /*
     * We will be accumulating in the buffer, so we must zero it first.
     */
    memset(buffer, 0, nOutItems * nValues * sizeof(byte));
    
    dIn  = DXGetArrayData(inArray);
    dOut = buffer;

    for (i = 0; i < nDim; i++)
	counts[i] = inCounts[(nDim-1)-i];

    skip[0] = nValues * (1 << n);
    for (i = 1; i < nDim; i++)
	skip[i] = outCounts[nDim-i] * skip[i-1];
	
    /*
     * First we copy the input data into the appropriate slots 
     * of the output data array
     */
    for (i = 0, loop0 = outerLoop; i < nDim; i++, loop0++)
    {
	loop0->ptr    = dOut;
	loop0->skip   = skip[i];
	loop0->inc    = 0;
	loop0->limit  = counts[i];
    }

    olim = outerLoop[0].limit;
    oskp  = skip[0];

    for (;;)
    {
	dst = outerLoop[0].ptr;
	src = dIn;
    
	for (i = 0; i < olim; i++)
	{
	    *dst = *src;
	    src++;
	    dst += oskp;
	}

	if (nDim == 0) break;

	for (i = 1, loop0 = outerLoop + 1; i < nDim; i++, loop0++)
	{
	    loop0->inc ++;
	    loop0->ptr += loop0->skip;
	    if (loop0->inc < loop0->limit)
		break;
	}

	if (i == nDim)
	    break;

	for (j = i-1, loop1 = outerLoop + (i-1); j >= 0; j--, loop1--)
	{
	    loop1->ptr = loop0->ptr;
	    loop1->inc = 0;
	}
    }

    /*
     * Now for each level, we insert midpoints.  Assume at this point that
     * the outer loop is set up to index each input point.
     */
    for (level = 0; level < n; level++)
    {

	for (i = 0, loop0 = outerLoop; i < nDim; i++, loop0++)
	{
	    loop0->ptr    = dOut;
	    loop0->inc    = 0;

	    innerLoop[i].skip = loop0->skip >> 1;
	}

	olim = outerLoop[0].limit;
	oskp = outerLoop[0].skip;

	/*
	 * Outer loop: for each input sample....
	 */
	for (;;)
	{
	    /*
	     * Inner loop: accrue input samples onto 3x...x3 neighbors
	     */
	    for (i = 0, src = outerLoop[0].ptr; i < olim; i++, src += oskp)
	    {
		outerLoop[0].inc = i;

		dst = src;

		/*
		 * Set up inner loop structure.  
		 * Boundary conditions vary the inner loop starts and ends
		 */
		loop0 = outerLoop;
		loop1 = innerLoop;
		for (j = 0; j < nDim; j++, loop0++, loop1++)
		{

		    if (loop0->inc == 0)
			loop1->start = 0;
		    else
		    {
			loop1->start = -1;
			dst -= loop1->skip;
		    }
		
		    if (loop0->inc == loop0->limit-1)
			loop1->limit = 1;
		    else
			loop1->limit = 2;
		}

		for (j = 0, loop0 = innerLoop; j < nDim; j++, loop0++)
		{
		    loop0->inc = loop0->start;
		    loop0->ptr = dst;
		}
	    
		/*
		 * Inner loop: add src values to neighbors.
		 */
		for (;;)
		{
		    dst = innerLoop[0].ptr;

		    for (j = innerLoop[0].inc; j < innerLoop[0].limit; j++)
		    {
			if (src != dst)
			    if (*src)
				*dst = 1;
		    
			dst += innerLoop[0].skip;
		    }
	    
		    if (nDim == 0) 
			break;

		    for (j = 1, loop0 = innerLoop + 1; j < nDim; j++, loop0++)
		    {
			loop0->inc ++;
			loop0->ptr += loop0->skip;
			if (loop0->inc < loop0->limit)
			    break;
		    }

		    if (j == nDim)
			break;

		    loop1 = innerLoop + (j-1);
		    for (k = j-1; k >= 0; k--, loop1--)
		    {
			loop1->ptr = loop0->ptr;
			loop1->inc = loop1->start;
		    }
		}
	    }

	    /*
	     * Update outer loop counters
	     */
	    if (nDim == 0) break;

	    for (i = 1, loop0 = outerLoop + 1; i < nDim; i++, loop0++)
	    {
		loop0->inc ++;
		loop0->ptr += loop0->skip;
		if (loop0->inc < loop0->limit)
		    break;
	    }

	    if (i == nDim)
		break;

	    for (j = i-1, loop1 = outerLoop + (i-1); j >= 0; j--, loop1--)
	    {
		loop1->ptr = loop0->ptr;
		loop1->inc = 0;
	    }
	}
    }

    if (! DXAddArrayData(outArray, 0, nOutItems, (Pointer)buffer))
	goto error;

    DXFree((Pointer)buffer);
    return outArray;

error:

    DXFree((Pointer)buffer);
    return NULL;
}
#endif

#define INDICES(ref, indices, strides, ndim)            \
{                                                       \
    int i, j = (ref);                                   \
    for (i = 0; i < (ndim)-1; i++) {                    \
	(indices)[i] = j / (strides)[i];                \
	j = j % (strides)[i];                           \
    }                                                   \
    (indices)[i] = j;                                   \
}

#define REFERENCE(indices, ref, strides, ndim)          \
{                                                       \
    int i, j = 0;                                       \
    for (i = 0; i < (ndim)-1; i++)                      \
	j += (indices)[i]*(strides)[i];                 \
    (ref) = j + (indices)[(ndim)-1];                    \
}


static Array
RefineRegReferences(Array in, int nDim, int levels,
			int *inKnts, int *outKnts, int ref)
{
    Array out = NULL;
    Type type; Category cat;
    int i, nRefs, rank, shape[32], *ptr, *refs;
    int indices[32], max[32], min[32], inS[32], outS[32];
    SegListSegment *segment;
    SegList *segList = DXNewSegList(sizeof(int), 0, 0);
    if (! segList)
	goto error;
    
    DXGetArrayInfo(in, &nRefs, &type, &cat, &rank, shape);
    if (type != TYPE_INT && cat != CATEGORY_REAL)
    {
	DXSetError(ERROR_DATA_INVALID, "ref arrays must be real integers");
	goto error;
    }

    nRefs *= DXGetItemSize(in) / sizeof(int);

    refs = (int *)DXGetArrayData(in);
    if (! refs)
	goto error;
    
    DXGetArrayInfo(in, &nRefs, NULL, NULL, NULL, NULL);

    if (ref == REF_POSITIONS)
    {
	inS[nDim-1] = outS[nDim-1] = 1;
	for (i = nDim-2; i >= 0; i--)
	{
	    inS[i]  = inKnts[i+1]* inS[i+1];
	    outS[i] = outKnts[i+1]*outS[i+1];
	}
    }
    else
    {
	inS[nDim-1] = outS[nDim-1] = 1;
	for (i = nDim-2; i >= 0; i--)
	{
	    inS[i]  = (inKnts[i+1]-1)* inS[i+1];
	    outS[i] = (outKnts[i+1]-1)*outS[i+1];
	}
    }

    for (i = 0; i < nRefs; i++)
    {
	int overlap, nOutRefs, j, r;
	SegListSegment *seg;

	if ((r = refs[i]) != -1)
	{
	    INDICES(r, indices, inS, nDim);

	    for (j = 0; j < nDim; j++)
		indices[j] *= (1 << levels);
	
	    if (ref == REF_POSITIONS)
	    {
		overlap = (1 << levels) - 1;

		nOutRefs = 1;
		for (j = 0; j < nDim; j++)
		{
		    max[j] = indices[j] + overlap;
		    if (max[j] >= outKnts[j])
			max[j] = outKnts[j] - 1;

		    min[j] = indices[j] - overlap;
		    if (min[j] < 0) 
			min[j] = 0;
		    
		    indices[j] = min[j];

		    nOutRefs *= (max[j] - min[j]) + 1;
		}
	    }
	    else
	    {
		overlap = (1 << levels) - 1;

		nOutRefs = 1;
	        for (j = 0; j < nDim; j++)
		{
		    min[j] = indices[j];
		    max[j] = indices[j] + overlap;

		    nOutRefs *= (max[j] - min[j]) + 1;
		}
	    }

	    seg = DXNewSegListSegment(segList, nOutRefs);
	    if (! seg)
		goto error;
	    
	    ptr = (int *)DXGetSegListSegmentPointer(seg);

	    for ( ;; )
	    {
		REFERENCE(indices, *ptr, outS, nDim);
		ptr ++;

		for (j = 0; j < nDim; j++)
		{
		    indices[j] ++;
		    if (indices[j] > max[j])
			indices[j] = min[j];
		    else
			break;
		}

		if (j == nDim)
		    break;
	    }
	}
    }

    out = DXNewArrayV(type, cat, rank, shape);
    if (! out)
	goto error;
    
    if (! DXAddArrayData(out, 0, DXGetSegListItemCount(segList), NULL))
	goto error;
    
    ptr = (int *)DXGetArrayData(out);

    DXInitGetNextSegListSegment(segList);
    while(NULL != (segment = DXGetNextSegListSegment(segList)))
    {
	int knt = DXGetSegListSegmentItemCount(segment);
	memcpy((char *)ptr, DXGetSegListSegmentPointer(segment), 
							knt*sizeof(int));
	ptr += knt;
    }

    DXDeleteSegList(segList);

    return out;

error:
    DXDeleteSegList(segList);
    DXDelete((Object)out);

    return NULL;
}
