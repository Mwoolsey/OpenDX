/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#include <string.h>
#include <stdarg.h>
#include "arrayClass.h"


ProductArray
_NewProductArrayV(int n, Array *terms, struct productarray_class *class)
{
    Type type;
    Category category;
    ProductArray a;
    int i, items, rank, *shape;

    /* set type according to first term */
    if (n>0) {
	type = terms[0]->type;
	category = terms[0]->category;
	rank = terms[0]->rank;
	shape = terms[0]->shape;
    } else {
	type = TYPE_FLOAT;
	category = CATEGORY_REAL;
	rank = 0;
	shape = NULL;
    }

    /* create array */
    a = (ProductArray) _dxf_NewArrayV(type, category, rank, shape,
				  (struct array_class *)class);
    if (!a)
	return NULL;

    /* typecheck and set terms */
    for (i=0, items=1; i<n; i++) {
	if (!DXTypeCheckV(terms[i], type, category, rank, shape))
	    DXErrorReturn(ERROR_BAD_TYPE,
			"Inconsistent product array term types");
	items *= terms[i]->items;
    }

    /* copy info */
    a->nterms = n;
    if (n * sizeof(*terms) <= sizeof(a->array.ldata)) {
	a->terms = (Array *) a->array.ldata;
    } else {
	a->terms = (Array *) DXAllocate(n * sizeof(*terms));
	if (!a->terms)
	    return NULL;
    }
    for (i=0; i<n; i++)
	a->terms[i] = (Array)DXReference((Object)terms[i]);
    a->array.items = items;

    return a;
}


ProductArray
DXNewProductArrayV(int n, Array *terms)
{
    return _NewProductArrayV(n, terms, &_dxdproductarray_class);
}


ProductArray
DXNewProductArray(int n, ...)
{
    Array terms[100];
    int i;
    va_list arg;

    ASSERT(n<100);

    /* collect args */
    va_start(arg,n);
    for (i=0; i<n; i++)
	terms[i] = va_arg(arg, Array);
    va_end(arg);

    /* call V version */
    return DXNewProductArrayV(n, terms);
}


ProductArray
DXGetProductArrayInfo(ProductArray a, int *n, Array *terms)
{
    int i;

    CHECKARRAY(a, CLASS_PRODUCTARRAY);

    if (n)
	*n = a->nterms;

    if (terms)
	for (i=0; i<a->nterms; i++)
	    terms[i] = a->terms[i];

    return a;
}


/*
 * XXX - the DXsyncmem()s below are because somebody else may have created
 * the data and the allocation for it might be in a new segment.  This
 * is little kludgy; we really need a better solution to this problem.
 */

Pointer
_dxfProductArray_GetArrayData(ProductArray a)
{
    int i, j, k, l, nterms, ndim, nl, nr, rank;
    Array *terms;
    float *result = NULL, *res, *right, *rt, *left, *lt;
    int freelocal = 0;

    /* DXlock array */
    EXPAND_LOCK(a);

    nterms = a->nterms;
    terms = a->terms;
    rank = a->array.rank;
    for (i=0, ndim=1;  i<rank;  i++)
	ndim *= a->array.shape[0];

    /* first left operand is first term */
    left = (float *) DXGetArrayData(terms[0]);
    if (!left)
	goto error;
    nl = terms[0]->items;
    /*ASSERT(terms[0]->shape[0]==ndim);*/

    /* copy first term data if nterms is 1 */
    /* XXX - is there any way to avoid this? */
    if (nterms==1) {

	int size = nl * ndim * sizeof(float);
	result = (float *) DXAllocate(size);
	if (!result)
	    goto error;
	memcpy(result, left, size);

    /* accumulate product left to right */
    } else for (i=1; i<nterms; i++) {

	/* right operand is next term */
	right = (float *) DXGetArrayDataLocal(terms[i]);
	if (!right) {
	    freelocal = 0;
	    right = (float *) DXGetArrayData(terms[i]);
	    if (!right)
		goto error;
	} else
	    freelocal = 1;
	    
	nr = terms[i]->items;
	/*ASSERT(terms[i]->shape[0]==ndim);*/
	
	/* allocate result */
	res = result = (float *) DXAllocate(nl*nr*ndim*sizeof(float));
	if (!result)
	    goto error;

	/* left operand plus right operand */
	switch (ndim) {
	case 1:
	    for (j=0, lt=left; j<nl; j++, lt+=ndim) {	/* ea left point */
		float t0 = lt[0];
		for (k=0, rt=right; k<nr; k++, rt+=ndim)/* ea right point */
		    *res++ = t0 + rt[0];		/* sum */
	    }
	    break;

	case 2:
	    /* left operand plus right operand */
	    for (j=0, lt=left; j<nl; j++, lt+=ndim) {	/* ea left point */
		float t0 = lt[0];
		float t1 = lt[1];
		for (k=0, rt=right; k<nr; k++, rt+=ndim) {/* ea right point */
		    *res++ = t0 + rt[0];		/* sum */
		    *res++ = t1 + rt[1];		/* sum */
		}
	    }
	    break;

	case 3:
	    /* left operand plus right operand */
	    for (j=0, lt=left; j<nl; j++, lt+=ndim) {	/* ea left point */
		float t0 = lt[0];
		float t1 = lt[1];
		float t2 = lt[2];
		for (k=0, rt=right; k<nr; k++, rt+=ndim) { /* ea right point */
		    *res++ = t0 + rt[0];		/* sum */
		    *res++ = t1 + rt[1];		/* sum */
		    *res++ = t2 + rt[2];		/* sum */
		}
	    }
	    break;

	default:
	    /* left operand plus right operand */
	    for (j=0, lt=left; j<nl; j++, lt+=ndim) {	/* ea left point */
		for (k=0, rt=right; k<nr; k++, rt+=ndim)/* ea right point */
		    for (l=0; l<ndim; l++)		/* ea dimension */
			*res++ = lt[l] + rt[l];		/* sum */
	    }
	    break;
	}

	/* next left operand is result */
	if (i>1)
	    DXFree((Pointer)left);
	left = result;
	nl *= nr;
	if (freelocal)
	    DXFreeArrayDataLocal(terms[i], (Pointer)right);
    }

    ASSERT(nl==a->array.items);

    /* DXunlock and return */
    EXPAND_RETURN(a, result);

error:
    if (freelocal)
	DXFreeArrayDataLocal(terms[i], (Pointer)right);    /* and fall thru */
    EXPAND_ERROR(a);
}


Error
_dxfProductArray_Delete(ProductArray a)
{
    int i;
    for (i=0; i<a->nterms; i++)
	DXDelete((Object)a->terms[i]);
    if ((Pointer)a->terms != (Pointer)a->array.ldata)
	DXFree((Pointer)a->terms);
    return _dxfArray_Delete((Array)a);
}

