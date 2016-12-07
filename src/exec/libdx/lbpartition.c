/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>



#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "objectClass.h"     /* for _dxfPartition */
#include "internals.h"

#define PARALLEL 1

/*
 * DXPartition - divide an a object into about n
 * spatially local partitions no smaller than size
 */

/* XXX - find a place for this */
Group _dxf_PartitionRegular(Field f, int n, int size);

Group
DXPartition(Field f, int n, int size)
{
    Group g;
    Object *o;
    int i;

    /* empty field check */
    if (DXGetObjectClass((Object)f)==CLASS_FIELD && DXEmptyField(f))
	return (Group)f /*yuck*/;

    /* try for regular field */
    g = _dxf_PartitionRegular(f, n, size);
    if (g || DXGetError()!=ERROR_NONE) {
	if (g)
	    DXCopyAttributes((Object)g, (Object)f);
	return g;
    }

    /* array to hold resulting objects */
    o = (Object *) DXAllocateZero(n * sizeof(Object));
    if (!o)
	goto error;

    /* do the partitioning */
    if (PARALLEL) {
	DXCreateTaskGroup();
	if (!_dxfPartition((Object)f, &n, size, o, 0))
	    goto error;
	if (!DXExecuteTaskGroup())
	    goto error;
    } else {
	if (!_dxfPartition((Object)f, &n, size, o, 0))
	    goto error;
    }

    /* put objects in group */
    g = (Group) DXNewCompositeField();
    for (i=0; i<n; i++)
	if (o[i])
	    DXSetMember(g, NULL, o[i]);
    DXFree((Pointer)o);
    if (g)
	DXCopyAttributes((Object)g, (Object)f);
    return g;

error:
    DXFree((Pointer)o);
    return NULL;
}


/*
 * Note - voting assumes (1) POSITIVE and NEGATIVE sum to 0 and
 * (2) all votes satisfy one and only one of ISPOS and ISNEG
 */

#define UNDEF      (-1)       /* point number not assigned yet */
#define POSITIVE   (1)        /* point is on positive side */
#define NEGATIVE   (2)        /* point is on negative side */
#define ISPOS(x)   (x>0)      /* is point on pos/neg side? */
#define ISNEG(x)   (!ISPOS(x))

#define DEP_NONE	0
#define DEP_POSITIONS   1
#define DEP_CONNECTIONS 2

#define REF_NONE	0
#define REF_POSITIONS   1
#define REF_CONNECTIONS 2

static Error
partition(Field f, int *n, int size, Object *o, int delete)
{
    struct ptinfo {		/* info about points */
	int posneg;		/* +1/-1 */
	int pos;		/* point id in pos partition */
	int neg;		/* point id in neg partition */
    } *ptinfo;
    ArrayHandle points = NULL;	/* the positions component */
    Pointer pbuf = NULL;
    ArrayHandle connections = NULL;	/* the connections component */
    Pointer cbuf = NULL;
    ArrayHandle comp = NULL;
    int *abuf = NULL;
    int npoints, nconnections;
    int npospts=0, nnegpts=0;	/* number of points in pos/neg partition */
    int nposelts=0, nnegelts=0;	/* number of connections in pos/neg partition */
    Field pos = NULL, neg = NULL;
    int i, j, k, vote, maxdim, npos, nneg, rank, dim, histo, vPerE;
    float mid, min[100], max[100], sum[100];
    Array a, pA, cA;
    char *name;
    Type type;
    Category category;
    Array posA = NULL, negA = NULL;
    char *pptr = NULL, *nptr = NULL;
    int  *ipsrc = NULL, *insrc = NULL;
    byte *c_part  = NULL; /* table indicating which side the element lies in */
    int  *c_index = NULL; /* table indicating index of connection in partition */
    int  shape[32];

    /* XXX */
    histo = getenv("HISTO")? 1 : 0;

    /* get and check the positions component */
    pA = (Array) DXGetComponentValue(f, POSITIONS);
    cA = (Array) DXGetComponentValue(f, CONNECTIONS);

    if (!pA)
    {
	DXSetError(ERROR_DATA_INVALID, "field has no positions component");
	goto error;
    }

    DXGetArrayInfo(pA, &npoints, &type, &category, &rank, &dim);
    if (type!=TYPE_FLOAT || category!=CATEGORY_REAL
				   || dim<1 || dim>100 || rank != 1)
    {
	DXSetError(ERROR_DATA_INVALID, "positions have bad type");
	goto error;
    }

    if (cA)
    {
	DXGetArrayInfo(cA, &nconnections, &type, &category, &rank, &vPerE);
	if (type!=TYPE_INT || category!=CATEGORY_REAL || dim<1 || dim>100)
	{
	    DXSetError(ERROR_DATA_INVALID, "connections have bad type");
	    goto error;
	}
    }
    else
	nconnections = 0;

    if (npoints<=size || *n<=1) {
	DXEndField(f);
	*o = (Object) f;
	*n = 1;
	return OK;
    }

    points = DXCreateArrayHandle(pA);
    if (!points)
	return ERROR;

    pbuf = DXAllocateLocal(DXGetItemSize(pA));
    if (! pbuf)
	goto error;
    
    /* initialize min, max, sum */
    for (j=0; j<dim; j++) {
	min[j] = DXD_MAX_FLOAT;
	max[j] = -DXD_MAX_FLOAT;
	sum[j] = 0;
    }

    /* find bounds, sum */
    for (i=0; i<npoints; i++)
    {
	float *p = (float *)DXGetArrayEntry(points, i, pbuf);

	for (j=0; j<dim; j++)
	{
	    float m = p[j];
	    if (m < min[j]) min[j] = m;
	    if (m > max[j]) max[j] = m;
	    sum[j] += m;
	}
    }

    /* find longest dimension and its average point */
    for (maxdim=0, j=1; j<dim; j++)
	if (max[j]-min[j] > max[maxdim]-min[maxdim])
	    maxdim = j;

#define NHISTO 1000

    if (histo)
    {
	int histo[NHISTO+1], i, n;
	float scale, mn=min[maxdim], mx=max[maxdim];
	for (i=0; i<=NHISTO; i++)
	    histo[i] = 0;
	scale = NHISTO / (mx-mn);

	for (i=0; i<npoints; i++)
	{
	    float *p = (float *)DXGetArrayEntry(points, i, pbuf);
	    histo[(int)((p[maxdim]-mn)*scale)]++;
	}

	for (i=0, n=0; i<=NHISTO && n<npoints/2; i++)
	    n += histo[i];
	mid = i/scale + mn;
    }
    else
    {
	mid = sum[maxdim] / npoints;
    }

    /*
     * count pts on either side, record local point id
     */
    ptinfo = (struct ptinfo *)DXAllocateLocal(npoints*sizeof(struct ptinfo));
    if (!ptinfo) 
	goto error;

    for (i=0, nnegpts=npospts=0; i<npoints; i++)
    {
	float *p = (float *)DXGetArrayEntry(points, i, pbuf);

	if (p[maxdim] < mid)
	{
	    ptinfo[i].posneg = NEGATIVE;
	    ptinfo[i].pos = UNDEF;
	    ptinfo[i].neg = nnegpts++;
	}
	else
	{
	    ptinfo[i].posneg = POSITIVE;
	    ptinfo[i].pos = npospts++;
	    ptinfo[i].neg = UNDEF;
	}
    }

    DXFreeArrayHandle(points);
    points = NULL;
    DXFree(pbuf);
    pbuf = NULL;

    if (nconnections)
    {
	/*
	 * Go through interpolation elements assigning each to one side or
	 * the other by majority vote of which side the control points are
	 * on.  Resolve ties consistently to one partition or the other.
	 * If necessary, assign point numbers for each of the other points
	 * on the side that we just put the element on.
	 */

	c_part  = (byte *)DXAllocateLocal(nconnections*sizeof(byte));
	c_index = (int  *)DXAllocateLocal(nconnections*sizeof(int));
	if (! c_part || ! c_index)
	    goto error;

	connections = DXCreateArrayHandle(cA);
	if (! connections)
	    goto error;
    
	cbuf = (int *)DXAllocateLocal(DXGetItemSize(cA));
	if (! cbuf)
	    goto error;

	nposelts = nnegelts = 0;
	for (i = 0; i < nconnections; i++)
	{
	    int *e, *elt = (int *)DXGetArrayEntry(connections, i, cbuf);

	    for (vote=0, k=0, e=elt; k<vPerE; k++, e++)
		if (ptinfo[*e].posneg == POSITIVE)
		    vote += 1;
		else if (ptinfo[*e].posneg == NEGATIVE)
		    vote -= 1;

	    if (ISPOS(vote))
	    {
		for (k=0, e=elt; k<vPerE; k++, e++)
		{
		    struct ptinfo *p = ptinfo + *e;	
		    if (p->pos==UNDEF)		
			p->pos = npospts++;
		}

		if (c_part)
		    c_part[i] = POSITIVE;
		if (c_index)
		    c_index[i] = nposelts++;

	    }
	    else
	    {
		for (k=0, e=elt; k<vPerE; k++, e++)
		{
		    struct ptinfo *p = ptinfo + *e;
		    if (p->neg==UNDEF)		
			p->neg = nnegpts++;
		}
		if (c_part)
		    c_part[i] = NEGATIVE;
		if (c_index)
		    c_index[i] = nnegelts++;

	    }
	}

	DXFreeArrayHandle(connections);
	connections = NULL;

	DXFree(cbuf);
	cbuf = NULL;

	if (nposelts == 0 || nnegelts == 0)
	{
	    if (! DXEndField(f))
		goto error;
	    *o = (Object)f;
	    *n = 1;
	    DXFree((Pointer)c_part);
	    DXFree((Pointer)c_index);
	    return OK;
	}

    }


    /* XXX - check for no points or no faces & don't do */

    /*
     * positive, negative partitions
     * don't copy attributes: they are at top level
     * some attributes (e.g. color multipliers) must not
     * be replicated at multiple levels
     */
    pos = (Field) DXCopy((Object)f, COPY_STRUCTURE);
    while (DXGetEnumeratedAttribute((Object)pos, 0, &name))
	DXSetAttribute((Object)pos, name, NULL);
    neg = (Field) DXCopy((Object)f, COPY_STRUCTURE);
    while (DXGetEnumeratedAttribute((Object)neg, 0, &name))
	DXSetAttribute((Object)neg, name, NULL);
    
    /*
     * Now go through the components and divide them into two partitions
     */
    i = 0;
    while (NULL != (a = (Array)DXGetEnumeratedComponentValue(f, i++, &name)))
    {
	int ref = REF_NONE, dep = DEP_NONE;
	Object attr;
	int  nEntries, itemSize, nItems;

	posA = NULL;
	negA = NULL;

	if (! strcmp(name, "connections"))
	{
	    dep = DEP_CONNECTIONS;
	}
	else if (! strcmp(name, "positions"))
	{
	    dep = DEP_POSITIONS;
	}
	else
	{
	    attr = DXGetAttribute((Object)a, "dep");
	    if (attr || !strcmp(name, "connections")) 
	    {
		if (DXGetObjectClass(attr) != CLASS_STRING)
		{
		    DXSetError(ERROR_DATA_INVALID, "#10200", "dep attribute");
		    goto error;
		}

		if (! strcmp(DXGetString((String)attr), "connections"))
		    dep = DEP_CONNECTIONS;
		else if (! strcmp(DXGetString((String)attr), "positions"))
		    dep = DEP_POSITIONS;
	    }
	}
		    
	attr = DXGetAttribute((Object)a, "ref");
	if (attr)
	{
	    if (DXGetObjectClass(attr) != CLASS_STRING)
	    {
		DXSetError(ERROR_DATA_INVALID, "#10200", "ref attribute");
		goto error;
	    }

	    if (! strcmp(DXGetString((String)attr), "connections"))
		ref = REF_CONNECTIONS;
	    else if (! strcmp(DXGetString((String)attr), "positions"))
		ref = REF_POSITIONS;
	}

	if (dep == DEP_NONE && ref == REF_NONE)
	    continue;

	DXGetArrayInfo(a, &nItems, &type, &category, &rank, shape);
	itemSize = DXGetItemSize(a);
	nEntries = itemSize / DXTypeSize(type);

	/*
	 * handle constant arrays separately
	 */
	if (DXQueryConstantArray(a, NULL, NULL))
	{
	    int np, nn;

	    if (dep == DEP_POSITIONS)
	    {
		np = npospts; nn = nnegpts;
	    }
	    else
	    {
		np = nposelts; nn = nnegelts;
	    }

	    if (ref == REF_NONE)
	    {
		posA = (Array)DXNewConstantArrayV(np,
		    DXGetConstantArrayData(a), type, category, rank, shape);

		negA = (Array)DXNewConstantArrayV(nn,
		    DXGetConstantArrayData(a), type, category, rank, shape);
	    }
	    else if (ref == REF_POSITIONS)
	    {
		int *src = (int *)DXGetConstantArrayData(a);
		
		abuf = (int *)DXAllocateLocal(DXGetItemSize(a));
		if (! abuf)
		    goto error;

		for (j = 0; j < nEntries; j++)
		    if (ptinfo[src[j]].pos == UNDEF)
			abuf[j] = -1;
		    else abuf[j] = ptinfo[src[j]].pos;

		posA = (Array)DXNewConstantArrayV(np,
			    (Pointer)abuf, type, category, rank, shape);

		for (j = 0; j < nEntries; j++)
		    if (ptinfo[src[j]].neg == UNDEF)
			abuf[j] = -1;
		    else abuf[j] = ptinfo[src[j]].neg;

		negA = (Array)DXNewConstantArrayV(nn,
			    (Pointer)abuf, type, category, rank, shape);
		
		DXFree((Pointer)abuf);
		abuf = NULL;
	    }
	    else if (ref == REF_CONNECTIONS)
	    {
		int *src = (int *)DXGetConstantArrayData(a);
		
		abuf = (int *)DXAllocateLocal(DXGetItemSize(a));
		if (! abuf)
		    goto error;

		for (j = 0; j < nEntries; j++)
		    if (c_part[src[j]] == NEGATIVE)
			abuf[j] = -1;
		    else abuf[j] = c_index[src[j]];

		posA = (Array)DXNewConstantArrayV(np,
			    (Pointer)abuf, type, category, rank, shape);

		for (j = 0; j < nEntries; j++)
		    if (c_part[src[j]] == POSITIVE)
			abuf[j] = -1;
		    else abuf[j] = c_index[src[j]];

		negA = (Array)DXNewConstantArrayV(nn,
			    (Pointer)abuf, type, category, rank, shape);

		DXFree((Pointer)abuf);
		abuf = NULL;
	    }
	}
	else
	{
	    int np, nn;

	    DXGetArrayInfo(a, &nItems, &type, &category, &rank, shape);

	    comp = DXCreateArrayHandle(a);
	    if (! comp)
		goto error;

	    abuf = (int *)DXAllocateLocal(DXGetItemSize(a));
	    if (! abuf)
		goto error;

	    nn = np = 0;

	    /*
	     * Figure out how many go to either side.  If the component deps 
	     * positions or connections, we already know how many.  Otherwise,
	     * the component is referential and we have to count.  We add an
	     * element to one or both partitions depending on whether they have
	     * at least one referent into that partition.  Referents to the 
	     * other partition get a -1.
	     */
	    if (dep == DEP_POSITIONS)
	    {
		np = npospts; nn = nnegpts;
	    }
	    else if (dep == DEP_CONNECTIONS)
	    {
		np = nposelts; nn = nnegelts;
	    }
	    else if (ref == REF_POSITIONS)
	    {
		for (j = 0; j < nItems; j++)
		{
		    int p = 0, n = 0;
		    int *elt = (int *)DXGetArrayEntry(comp, j, abuf);

		    for (k = 0; k < nEntries; k++, elt++)
		    {
			if (ptinfo[*elt].pos != UNDEF) p++;
			if (ptinfo[*elt].neg != UNDEF) n++;
		    }

		    if (n) nn++;
		    if (p) np++;
		}
	    }
	    else if (ref == REF_CONNECTIONS)
	    {
		for (j = 0; j < nItems; j++)
		{
		    int p = 0, n = 0;
		    int *elt = (int *)DXGetArrayEntry(comp, j, abuf);

		    for (k = 0; k < nEntries; k++, elt++)
		    {
			if (c_part[*elt] == POSITIVE) p++;
			if (c_part[*elt] == NEGATIVE) n++;
		    }

		    if (n) nn++;
		    if (p) np++;
		}
	    }
		
	    /* 
	     * allocate the sub-arrays.
	     */
	    if (np)
	    {
		posA = (Array)DXNewArrayV(type, category, rank, shape);
		if (! posA)
		    goto error;

		if (! DXAddArrayData(posA, 0, np, NULL))
		    goto error;

		pptr = (char *)DXGetArrayData(posA);
	    }
	    else
		posA = NULL;

	    if (nn)
	    {
		negA = (Array)DXNewArrayV(type, category, rank, shape);
		if (! negA)
		    goto error;

		if (! DXAddArrayData(negA, 0, nn, NULL))
		    goto error;
		
		nptr = (char *)DXGetArrayData(negA);
	    }
	    else
		negA = NULL;

	    /*
	     * If the component contains references, we need to create buffers
	     * to re-map the references.
	     */
	    if (ref != REF_NONE)
	    {
		ipsrc = (int *)DXAllocateLocal(itemSize);
		insrc = (int *)DXAllocateLocal(itemSize);
		if (! ipsrc || ! insrc)
		    goto error;
	    }
	    else insrc = ipsrc = NULL;


	    /*
	     * Now we partition based on the dependency of the component.
	     * If the component is dep either positions or connections, we 
	     * have to put the new elements in the right place.  Otherwise,
	     * we just add 'em on to the end
	     */
	    for (j = 0; j < nItems; j++)
	    {
		char *src = (char *)DXGetArrayEntry(comp, j, abuf);
		char *psrc = (char *)src, *nsrc = (char *)src;
		int  n = 0, p = 0;

		if (ref == REF_POSITIONS)
		{
		    int *isrc = (int *)src;

		    psrc = (char *)ipsrc;
		    nsrc = (char *)insrc;

		    for (k = 0; k < nEntries; k++)
		    {
			if (isrc[k] == -1 || ptinfo[isrc[k]].pos == UNDEF)
			    ipsrc[k] = -1;
			else
			{
			    ipsrc[k] = ptinfo[isrc[k]].pos;
			    p++;
			}

			if (isrc[k] == -1 || ptinfo[isrc[k]].neg == UNDEF)
			    insrc[k] = -1;
			else
			{
			    insrc[k] = ptinfo[isrc[k]].neg;
			    n++;
			}
		    }
		}
		else if (ref == REF_CONNECTIONS)
		{
		    int *isrc = (int *)src;

		    psrc = (char *)ipsrc;
		    nsrc = (char *)insrc;

		    for (k = 0; k < nEntries; k++)
			if (isrc[k] >= 0 && c_part[isrc[k]] == POSITIVE)
			{
			    ipsrc[k] = c_index[isrc[k]];
			    insrc[k] = -1;
			    p++;
			}
			else if (isrc[k] >= 0 && c_part[isrc[k]] == NEGATIVE)
			{
			    insrc[k] = c_index[isrc[k]];
			    ipsrc[k] = -1;
			    n++;
			}
			else
			{
			    ipsrc[k] = -1;
			    insrc[k] = -1;
			}
		}
		    
		if (dep == DEP_POSITIONS)
		{
		    if (ptinfo[j].pos != UNDEF)
			memcpy(pptr+(ptinfo[j].pos*itemSize), psrc, itemSize);

		    if (ptinfo[j].neg != UNDEF)
			memcpy(nptr+(ptinfo[j].neg*itemSize), nsrc, itemSize);
		}
		else if (dep == DEP_CONNECTIONS)
		{
		    if (c_part[j] == POSITIVE)
			memcpy(pptr+(c_index[j]*itemSize), psrc, itemSize);

		    if (c_part[j] == NEGATIVE)
			memcpy(nptr+(c_index[j]*itemSize), nsrc, itemSize);
		}
		else 
		{
		    if (p)
		    {
			memcpy(pptr, psrc, itemSize);
			pptr += itemSize;
		    }

		    if (n)
		    {
			memcpy(nptr, nsrc, itemSize);
			nptr += itemSize;
		    }
		}
	    }

	    DXFreeArrayHandle(comp);

	    DXFree((Pointer)ipsrc);
	    DXFree((Pointer)insrc);
	    DXFree((Pointer)abuf);
	}

	if (! DXSetComponentValue(pos, name, (Object)posA))
	    goto error;

	if (! DXSetComponentValue(neg, name, (Object)negA))
	    goto error;
    }

    DXDeleteComponent(pos, "box");
    DXDeleteComponent(neg, "box");
    DXDeleteComponent(pos, "data statistics");
    DXDeleteComponent(neg, "data statistics");

    /* delete parent if requested, free ptinfo */
    if (delete)
	DXDelete((Object) f);

    DXFree((Pointer)ptinfo);
    DXFree((Pointer)c_part);
    DXFree((Pointer)c_index);

    /* recursively partition and delete pos */
    npos = (*n+1)/2;
    if (!_dxfPartition((Object) pos, &npos, size, o, 1))
	goto error;

    /* recursively partition and delete neg */
    nneg = *n-npos;
    if (!_dxfPartition((Object) neg, &nneg, size, o+npos, 1))
	goto error;

    /* return number of partitions generated */
    *n = npos + nneg;
    return OK;

error:

    DXFreeArrayHandle(comp);
    DXFreeArrayHandle(points);
    DXFreeArrayHandle(connections);
    DXDelete((Object)pos);
    DXDelete((Object)neg);
    DXDelete((Object)posA);
    DXDelete((Object)negA);
    DXFree(ipsrc);
    DXFree(insrc);
    DXFree(abuf);
    DXFree(pbuf);
    DXFree(cbuf);
    return ERROR;
}


/*
 * Parallelize
 */

struct arg {
    Field f;
    int n;
    int size;
    Object *o;
    int delete;
};

static Error
task(Pointer p)
{
    struct arg *arg = (struct arg *)p;
    return partition(arg->f, &arg->n, arg->size, arg->o, arg->delete);
}


Error
_dxfField_Partition(Field f, int *n, int size, Object *o, int delete)
{
    if (PARALLEL) {
	struct arg arg;
	arg.f = f;
	arg.n = *n;
	arg.size = size;
	arg.o = o;
	arg.delete = delete;
	if (!DXAddTask(task, (Pointer)&arg, sizeof(arg), 0.0))
	    return ERROR;
	return OK;
    } else {
	return partition(f, n, size, o, delete);
    }
}

