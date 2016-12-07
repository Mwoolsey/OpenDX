/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>



#include <stdarg.h>
#include <string.h>
#include "arrayClass.h"


static MeshArray
_NewMeshArrayV(int n, Array *terms, struct mesharray_class *class)
{
    int i, items, shape;
    MeshArray a;

    /* figure out and check type */
    for (i=0, items=1, shape=1; i<n; i++) {
	Array b = terms[i];
	if (b->type!=TYPE_INT || b->category!=CATEGORY_REAL || b->rank!=1)
	    DXErrorReturn(ERROR_BAD_TYPE, "Mesh array term has bad type");
	items *= terms[i]->items;
	shape *= terms[i]->shape[0];
    }

    /* create array */
    a = (MeshArray) _dxf_NewArrayV(TYPE_INT, CATEGORY_REAL, 1, &shape,
			       (struct array_class *)class);
    if (!a)
	return NULL;

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


MeshArray
DXNewMeshArrayV(int n, Array *terms)
{
    return _NewMeshArrayV(n, terms, &_dxdmesharray_class);
}


MeshArray
DXNewMeshArray(int n, ...)
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
    return DXNewMeshArrayV(n, terms);
}


MeshArray
DXGetMeshArrayInfo(MeshArray a, int *n, Array *terms)
{
    int i;

    CHECKARRAY(a, CLASS_MESHARRAY);

    if (n)
	*n = a->nterms;

    if (terms)
	for (i=0; i<a->nterms; i++)
	    terms[i] = a->terms[i];

    return a;
}


Pointer
_dxfMeshArray_GetArrayData(MeshArray a)
{
    int i, j, k, l, m, nterms, nle, nre, nlp, nrp, nr;
    Array *terms;
    int *result = NULL, *res, *right, *rt, *left, *lt;
    int freelocal = 0;

    /* DXlock array */
    EXPAND_LOCK(a);

    nterms = a->nterms;
    terms = a->terms;

    /* first left operand is first term */
    left = (int *) DXGetArrayData(terms[0]);
    nle = terms[0]->items;
    nlp = terms[0]->shape[0];

    /* copy first term data if nterms is 1 */
    /* XXX - is there any way to avoid this? */
    if (nterms==1) {

	int size = nle * nlp * sizeof(int);
	result = (int *) DXAllocate(size);
	if (!result)
	    goto error;
	memcpy(result, left, size);

    /* accumulate product left to right */
    } else for (i=1; i<nterms; i++) {

	/* right operand is next term */
	right = (int *) DXGetArrayDataLocal(terms[i]);
	if (!right) {
	    freelocal = 0;
	    right = (int *) DXGetArrayData(terms[i]);
	    if (!right)
		goto error;
	} else
	    freelocal = 1;

	nre = terms[i]->items;
	nrp = terms[i]->shape[0];

	/* calculate nr, the number of points in right operand */
	for (j=0, nr= -1; j<nre*nrp; j++)
	    if (right[j] > nr)
		nr = right[j];
	nr += 1;
	
	/* allocate result */
	res = result = (int *) DXAllocate(nle*nre*nlp*nrp*sizeof(int));
	if (!result)
	    goto error;

	/* left operand times right operand */
	switch (nrp) {
	case 2:
	    for (j=0, lt=left; j<nle; j++, lt+=nlp)	/* ea left elt */
		for (k=0, rt=right; k<nre; k++, rt+=nrp)/* ea right elt */
		    for (l=0; l<nlp; l++) {		/* ea left point */
			int tmp = lt[l]*nr;		/* help the compiler */
			*res++ = tmp + rt[0];		/* new point number */
			*res++ = tmp + rt[1];		/* new point number */
		    }
	    break;

	default:
	    for (j=0, lt=left; j<nle; j++, lt+=nlp)	/* ea left elt */
		for (k=0, rt=right; k<nre; k++, rt+=nrp)/* ea right elt */
		    for (l=0; l<nlp; l++) {		/* ea left point */
			int tmp = lt[l]*nr;		/* help the compiler */
			for (m=0; m<nrp; m++)		/* ea right point */
			    *res++ = tmp + rt[m];	/* new point number */
		    }
	    break;
	}

	/* next left operand is result */
	if (i>1)
	    DXFree((Pointer)left);
	left = result;
	nle *= nre;
	nlp *= nrp;

	if (freelocal)
	    DXFreeArrayDataLocal(terms[i], (Pointer)right);
    }

    ASSERT(nle==a->array.items);
    ASSERT(nlp==a->array.shape[0]);

    /* DXunlock and return */
    EXPAND_RETURN(a, result);

error:
    if (freelocal)
	DXFreeArrayDataLocal(terms[i], (Pointer)right);    /* and fall thru */
    EXPAND_ERROR(a);
}

Error
_dxfMeshArray_Delete(MeshArray a)
{
    int i;
    for (i=0; i<a->nterms; i++)
	DXDelete((Object)a->terms[i]);
    if ((Pointer)a->terms != (Pointer)a->array.ldata)
	DXFree((Pointer)a->terms);
    return _dxfArray_Delete((Array)a);
}

MeshArray
DXSetMeshOffsets(MeshArray a, int *offsets)
{
    int i, n;
    Array terms[100];
    if (!DXGetMeshArrayInfo(a, &n, terms))
	return NULL;
    ASSERT(n<100);
    for (i=0; i<n; i++)
	if (DXGetArrayClass(terms[i])==CLASS_PATHARRAY)
	    if (!DXSetPathOffset((PathArray)terms[i], offsets[i]))
		return NULL;
    return a;
}

MeshArray
DXGetMeshOffsets(MeshArray a, int *offsets)
{
    int i, n;
    Array terms[100];
    if (!DXGetMeshArrayInfo(a, &n, terms))
	return NULL;
    ASSERT(n<100);
    for (i=0; i<n; i++)
	if (!DXGetPathOffset((PathArray)terms[i], offsets+i))
	    return NULL;
    return a;
}
