/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>

#include <dx/dx.h>

#if defined(HAVE_STRING_H)
#include <string.h>
#endif

extern void _dxfPermute(); /* from libdx/permute.c */
Object DXObjectPartition(Object, int, int); 

/* default number of partitions if number of processors > 1
 */
#define FACTOR(x)  ((x-1) * 2)

int
m_Partition(Object *in, Object *out)
{
    int maxnum, mincount;
    
    out[0] = NULL;

    if (!in[0]) {
	DXSetError(ERROR_BAD_PARAMETER, "#10000", "input");
	return ERROR;
    }

    if (in[1]) {
	if (!DXExtractInteger(in[1], &maxnum)) {
	    DXSetError(ERROR_BAD_PARAMETER, "#10020", "number of partitions");
	    return ERROR;
	}
	if (maxnum <= 0) {
	    DXSetError(ERROR_BAD_PARAMETER, "#10020", "number of partitions");
	    return ERROR;
	}
    } else {
	maxnum = DXProcessors(0);
	if (maxnum > 1)
	    maxnum = FACTOR(maxnum);
    }
    
    if (in[2]) {
	if (!DXExtractInteger(in[2], &mincount)) {
	    DXSetError(ERROR_BAD_PARAMETER, "#10030", "number of points");
	    return ERROR;
	}
	if (mincount < 0) {
	    DXSetError(ERROR_BAD_PARAMETER, "#10030", "number of points");
	    return ERROR;
	}
    } else
	mincount = 1;
    

    if (maxnum == 1)
	out[0] = in[0];
    else
	out[0] = DXObjectPartition(in[0], maxnum, mincount);


    return(out[0] ? OK : ERROR);
}

static Error          Pass1(Object, HashTable);
static Object         Pass2(Object, HashTable);
static CompositeField MkPartitionTemplate(Field, int, int);
static CompositeField CpPartitionTemplate(Field, CompositeField);
static CompositeField CpPartitionTemplate_Reg(Field, CompositeField);
static CompositeField CpPartitionTemplate_Irreg(Field, CompositeField);
static int            Cmp(Key, Key);
static PseudoKey      Hash(Key);

#define MAXDIM	32

/*
 * Object-level interface for sharing partitioning among objects that share
 * positions and connections
 */

typedef struct 
{
    Object         p;
    Object         c;
    Field          f;
    CompositeField template;
    int		   knt;
    int		   unpart;	/* fields which return unpartitioned */
} HElement;

Object
DXObjectPartition(Object o, int n, int size)
{
    HashTable hTable = NULL;
    HElement *elt;

    hTable = DXCreateHash(sizeof(HElement), Hash, Cmp);
    if (! hTable)
	goto error;
    
    /*
     * Pass1 finds each field contained in the input object (exclusive of
     * those alreay contained in the input) and accesses the hash table 
     * keyed by the positions and connections array addresses.  If no entry
     * is found then this pair is unique (so far) and an entry is created.
     * Otherwise, the hash table entry's counter field is incremented.
     */
    if (! Pass1(o, hTable))
	goto error;
    
    /*
     * Traverse the hash table partitioning each unique positions/connections
     * pair.  If only one field referenced the pair, partition it directly.
     * Otherwise, create a partition template.
     */
    DXInitGetNextHashElement(hTable);
    while ((elt = (HElement *)DXGetNextHashElement(hTable)) != NULL)
    {
	if (elt->knt == 1)
	    elt->template = (CompositeField)DXPartition(elt->f, n, size);
	else
	    elt->template = MkPartitionTemplate(elt->f, n, size);

	if (! elt->template)
	    goto error;

	/* did partition actually do anything, or was the return from
	 * partition the identical field we sent in?
	 */
	elt->unpart = ((Pointer)elt->template == (Pointer)elt->f);
    }
    
    /*
     * Pass2 copies the input object, replacing each field contained
     * therein (excepting those already contained by a composite field)
     * with a partitioned version thereof.
     */
    o = Pass2(o, hTable);
    if (! o)
	goto error;

    /*
     * Need to delete any templates we created
     */
    DXInitGetNextHashElement(hTable);
    while ((elt = (HElement *)DXGetNextHashElement(hTable)) != NULL)
	if (elt->knt > 1 && !elt->unpart)
	    DXDelete((Object)elt->template);
    
    DXDestroyHash(hTable);

    if (! DXEndObject(o))
    {
	DXDelete(o);
	goto error;
    }
    
    return o;

error:
    DXDestroyHash(hTable);
    return ERROR;
}

static Error
Pass1(Object o, HashTable hTable)
{
    Class class;
    HElement helt, *hptr;
    Field f;

    if (! o)
	return OK;
    
    class = DXGetObjectClass(o);
    if (class == CLASS_GROUP)
	class = DXGetGroupClass((Group)o);

    switch(class)
    {
	case CLASS_COMPOSITEFIELD:
	    break;

	case CLASS_SERIES:
	case CLASS_MULTIGRID:
	case CLASS_GROUP:
	{
	    int i;
	    Object c;

	    i = 0;
	    while ((c = DXGetEnumeratedMember((Group)o, i++, NULL)) != NULL)
		if (! Pass1(c, hTable))
		    goto error;
	    
	    break;
	}

	case CLASS_XFORM:
	{
	    Object c;

	    if (! DXGetXformInfo((Xform)o, &c, NULL))
		goto error;

	    if (! Pass1(c, hTable))
		goto error;
		
	    break;
	}
	
	case CLASS_SCREEN:
	{
	    Object c;

	    if (! DXGetScreenInfo((Screen)o, &c, NULL, NULL))
		goto error;

	    if (! Pass1(c, hTable))
		goto error;
	    
	    break;
	}
	
	case CLASS_CLIPPED:
	{
	    Object c0, c1;

	    if (! DXGetClippedInfo((Clipped)o, &c0, &c1))
		goto error;

	    if (c0)
		if (! Pass1(c0, hTable))
		    goto error;

	    if (c1)
		if (! Pass1(c1, hTable))
		    goto error;

	    break;
	}

	case CLASS_FIELD:
	{
	    f = (Field)o;

	    if (DXEmptyField(f))
		break;
	    
	    helt.p        = DXGetComponentValue(f, "positions");
	    helt.c        = DXGetComponentValue(f, "connections");
	    helt.f        = f;
	    helt.template = NULL;
	    helt.knt      = 1;

	    if (!helt.p || !helt.c)
		break;
	
	    hptr = (HElement *)DXQueryHashElement(hTable, (Key)&helt);
	    if (hptr)
	    {
		hptr->knt ++;
		break;
	    }
		
	    if (! DXInsertHashElement(hTable, (Element)&helt))
		goto error;

	    break;
	}

	default:
	    break;
    }

    return OK;

error:
    return ERROR;
}

static Object
Pass2(Object o, HashTable hTable)
{
    Class class;

    if (! o)
	return NULL;
    
    class = DXGetObjectClass(o);
    if (class == CLASS_GROUP)
	class = DXGetGroupClass((Group)o);

    switch(class)
    {
	/* 
	 * Excludes series and composite fields
	 */
	case CLASS_GROUP:
	{
	    int i;
	    Object c, cnew = NULL;
	    Group g;
	    char *name;

	    g = (Group)DXCopy(o, COPY_ATTRIBUTES);
	    if (! g)
		return NULL;

	    i = 0;
	    while ((c = DXGetEnumeratedMember((Group)o, i, &name)) != NULL)
	    {
		cnew = Pass2(c, hTable);
		if (! cnew)
		{
		    DXDelete((Object)g);
		    return NULL;
		}

		if (name)
		{
		    if (! DXSetMember(g, name, cnew))  
		    {
			DXDelete((Object)g);
			if (cnew != c)
			    DXDelete((Object)cnew);
			return NULL;
		    }
		}
		else
		{
		    if (! DXSetEnumeratedMember(g, i, cnew))  
		    {
			DXDelete((Object)g);
			if (cnew != c)
			    DXDelete((Object)cnew);
			return NULL;
		    }
		}

		i++;
	    }
	    
	    return (Object)g;
	}

	case CLASS_MULTIGRID:
	{
	    int i;
	    Object c, cnew = NULL;
	    MultiGrid g;
	    char *name;

	    g = (MultiGrid)DXCopy(o, COPY_ATTRIBUTES);
	    if (! g)
		return NULL;

	    i = 0;
	    while ((c = DXGetEnumeratedMember((Group)o, i, &name)) != NULL)
	    {
		cnew = Pass2(c, hTable);
		if (! cnew)
		{
		    DXDelete((Object)g);
		    return NULL;
		}

		if (name)
		{
		    if (! DXSetMember((Group)g, name, cnew))  
		    {
			DXDelete((Object)g);
			if (cnew != c)
			    DXDelete((Object)cnew);
			return NULL;
		    }
		}
		else
		{
		    if (! DXSetEnumeratedMember((Group)g, i, cnew))  
		    {
			DXDelete((Object)g);
			if (cnew != c)
			    DXDelete((Object)cnew);
			return NULL;
		    }
		}

		i++;
	    }
	    
	    return (Object)g;
	}

	case CLASS_SERIES:
	{
	    int i;
	    Object c, cnew = NULL;
	    Series g;
	    float pos;

	    g = (Series)DXCopy(o, COPY_ATTRIBUTES);
	    if (! g)
		return NULL;

	    i = 0;
	    while ((c = DXGetSeriesMember((Series)o, i, &pos)) != NULL)
	    {
		cnew = Pass2(c, hTable);
		if (! cnew)
		{
		    DXDelete((Object)g);
		    return NULL;
		}

		if (! DXSetSeriesMember(g, i, pos, cnew))  
		{
		    DXDelete((Object)g);
		    if (c != cnew)
			DXDelete((Object)cnew);
		    return NULL;
		}

		i++;
	    }
	    
	    return (Object)g;
	}

	case CLASS_XFORM:
	{
	    Object c, cnew = NULL;
	    Matrix m;
	    Xform  x;

	    if (! DXGetXformInfo((Xform)o, &c, &m))
		return NULL;

	    cnew = Pass2(c, hTable);
	    if (!cnew)
		return NULL;
		
	    x = DXNewXform(cnew, m);
	    if (! x && c != cnew)
		DXDelete((Object)cnew);

	    return (Object)x;
	}
	
	case CLASS_SCREEN:
	{
	    Object c, cnew = NULL;
	    int    f, z;
	    Screen s;

	    if (! DXGetScreenInfo((Screen)o, &c, &f, &z))
		return NULL;

	    cnew = Pass2(c, hTable);
	    if (!cnew)
		return NULL;

	    s = DXNewScreen(cnew, f, z);
	    if (! s && c != cnew)
		DXDelete((Object)cnew);

	    return (Object)s;
	}
	
	case CLASS_CLIPPED:
	{
	    Object c0, c1, c0new = NULL;
	    Clipped c;
	    
	    if (! DXGetClippedInfo((Clipped)o, &c0, &c1))
		return NULL;

	    if (c0)
	    {
		c0new = Pass2(c0, hTable);
		if (!c0new)
		    return NULL;
	    }

	    c = DXNewClipped(c0new, c1);
	    if (! c && c0 != c0new)
		DXDelete((Object)c0new);
		
	    return (Object)c;
	}

	case CLASS_FIELD:
	{
	    HElement       helt, *hptr;
	    CompositeField cf;

	    if (DXEmptyField((Field)o))
		return DXCopy(o, COPY_STRUCTURE);

	    helt.p   = DXGetComponentValue((Field)o, "positions");
	    helt.c   = DXGetComponentValue((Field)o, "connections");
	    helt.knt = 1;

	    if (!helt.p || !helt.c)
		return DXCopy(o, COPY_STRUCTURE);
	
	    hptr = (HElement *)DXQueryHashElement(hTable, (Key)&helt);

	    if (! hptr)
	    {
		DXSetError(ERROR_INTERNAL, 
			"ObjectPartition2: missing pos/con pair\n");
		return NULL;
	    }

	    /* if this field wasn't actually partitioned (DXPartition 
	     *  returned the input instead of a new composite field)
	     *  just return the original object here.
	     */
	    if (hptr->unpart)
		return o;

	    /*
	     * If only a single field contained this connections/positions pair,
	     * then we simply partitioned it.  Otherwise, we created a partition
	     * template.
	     */
	    if (hptr->knt > 1)
		cf = CpPartitionTemplate((Field)o, hptr->template);
	    else
		cf = hptr->template;

	    if (cf == NULL)
		return NULL;
	
	    return (Object)cf;
	}

	default:
	    return o;
    }
}

#define MKDATATEMPLATE(f, oname, nname)				\
{								\
    int cnt, i, *ptr;						\
    Array a = (Array)DXGetComponentValue(f, oname);		\
								\
    DXGetArrayInfo(a, &cnt, NULL, NULL, NULL, NULL);		\
								\
    a = DXNewArray(TYPE_INT, CATEGORY_REAL, 0, 0);		\
								\
    if (! DXAddArrayData(a, 0, cnt, NULL))			\
	goto error;						\
								\
    ptr = (int *)DXGetArrayData(a);				\
    for (i = 0; i < cnt; i++)					\
	*ptr++ = i;						\
								\
    if (! DXSetComponentValue(f, nname, (Object)a))		\
	goto error;						\
								\
    if (! DXSetComponentAttribute(f, nname, "dep", 		\
				(Object)DXNewString(oname)))	\
	goto error;						\
}
    
static CompositeField
MkPartitionTemplate(Field f, int n, int size)
{
    Array          array;
    Object         attr;
    Field          nf = NULL;
    CompositeField cf = NULL;

    nf = DXNewField();
    if (! nf)
	goto error;
    
    array = (Array)DXGetComponentValue(f, "positions");
    if (! array)
	goto error;
    
    DXSetComponentValue(nf, "positions", (Object)array);

    array = (Array)DXGetComponentValue(f, "connections");
    if (! array)
	goto error;

    DXSetComponentValue(nf, "connections", (Object)array);

    attr = DXGetComponentAttribute(f, "connections", "element type");
    if (! attr)
	goto error;
    
    DXSetComponentAttribute(nf, "connections", "element type", attr);

    attr = DXGetComponentAttribute(f, "connections", "ref");
    if (! attr)
	goto error;
    
    DXSetComponentAttribute(nf, "connections", "ref", attr);
    DXSetComponentAttribute(nf, "positions", "dep", attr);

    if (! DXQueryGridConnections(array, NULL, NULL))
    {
	MKDATATEMPLATE(nf, "positions",   "prefs");
	MKDATATEMPLATE(nf, "connections", "crefs");
    }

    DXReference((Object)nf);

    cf = (CompositeField)DXPartition(nf, n, size);
    if ((Pointer)cf == (Pointer)nf) {
	DXDelete((Object)nf);
	return (CompositeField)f;     /* it really is a field */
    }

    DXDelete((Object)nf);
    return cf;

error:
    DXDelete((Object)nf);
    return NULL;

}

static CompositeField
CpPartitionTemplate(Field in, CompositeField template)
{
    Array a = (Array)DXGetComponentValue(in, "connections");

    /*
     * Two cases: regular connections, in which the partitioning template
     * information is implicit in the connections array, and irregular
     * connections, in which the partitioning template information is
     * explicit in the map arrays "prefs" and "crefs".
     */

    if (a && DXQueryGridConnections(a, NULL, NULL))
	return CpPartitionTemplate_Reg(in, template);
    else
	return CpPartitionTemplate_Irreg(in, template);
}
    
static CompositeField
CpPartitionTemplate_Reg(Field in, CompositeField template)
{
    int            i;
    CompositeField out = NULL;
    Array          a, dst = NULL;
    int		   ndim, inCCounts[MAXDIM], inPCounts[MAXDIM];
    int		   inCStrides[MAXDIM], inPStrides[MAXDIM];
    Field          c, f = NULL;
    Pointer	   o = NULL, d = NULL;

    /*
     * We will be copying data out of a regular grid.  Need information
     * regarding the input grid.
     */
    a = (Array)DXGetComponentValue(in, "connections");
    if (! a)
	goto error;
    
    if (! DXQueryGridConnections(a, &ndim, inPCounts))
	goto error;
    
    for (i = 0; i < ndim; i++)
	inCCounts[i] = inPCounts[i] - 1;
    
    inPStrides[ndim-1] = 1;
    inCStrides[ndim-1] = 1;
    for (i = ndim-2; i >= 0; i--)
    {
	inPStrides[i] = inPCounts[i+1]*inPStrides[i+1];
	inCStrides[i] = inCCounts[i+1]*inCStrides[i+1];
    }

    /*
     * Output structure
     */
    out = DXNewCompositeField();
    if (! out)
	goto error;
    
    i = 0;
    while((c = (Field)DXGetEnumeratedMember((Group)template, i++, NULL)) != NULL)
    {
	int    outPCounts[MAXDIM], outCCounts[MAXDIM], offset[MAXDIM];
	int    outPStrides[MAXDIM], outCStrides[MAXDIM];
	char   *name;
	int    j, np, nc;
	Array  cArray, src;

	/*
	 * Look at the connections component of the partition member
	 * to determine the part of the input field that whis partition
	 * member represents.
	 */
	cArray = (Array)DXGetComponentValue(c, "connections");
	if (! cArray)
	    goto error;

	if (! DXQueryGridConnections(cArray, NULL, outPCounts))
	    goto error;
	
	np = nc = 1;
	for (j = 0; j < ndim; j++)
	{
	    outCCounts[j] = outPCounts[j] - 1;
	    np *= outPCounts[j];
	    nc *= outCCounts[j];
	}

	outPStrides[ndim-1] = 1;
	outCStrides[ndim-1] = 1;
	for (j = ndim-2; j >= 0; j--)
	{
	    outPStrides[j] = outPCounts[j+1]*outPStrides[j+1];
	    outCStrides[j] = outCCounts[j+1]*outCStrides[j+1];
	}
	
	if (! DXGetMeshOffsets((MeshArray)cArray, offset))
	    goto error;

	f = (Field)DXCopy((Object)in, COPY_STRUCTURE);
	if (! f)
	    goto error;

	j = 0;
	while (NULL != (src=(Array)DXGetEnumeratedComponentValue(in, j++, &name)))
	{
	    Object   attr;
	    int      size, n;
	    byte    *srcData, *dstData;
	    int      k;
	    int      *inStrides, *outCounts, *outStrides;
	    Type     t;
	    Category cat;
	    int      r, s[MAXDIM];

	    /*
	     * If the component exists in the template, use it.  Otherwise,
	     * carve the necessary portion out of the corresponding component
	     * of the input array.
	     */
	    dst = (Array)DXGetComponentValue(c, name);
	    if (dst)
		goto dst_done;

	    /* 
	     * Select appropriate grid parameters depending on the 
	     * component's dependency.
	     */
	    attr = (Object)DXGetComponentAttribute(in, name, "dep");
	    if (! attr)
		continue;
	    
	    if (!strcmp(DXGetString((String)attr), "positions"))
	    {
		inStrides  = inPStrides;
		outStrides = outPStrides;
		/*inCounts   = inPCounts;*/
		outCounts  = outPCounts;
		n          = np;
	    }
	    else
	    {
		inStrides  = inCStrides;
		outStrides = outCStrides;
		/*inCounts   = inCCounts;*/
		outCounts  = outCCounts;
		n          = nc;
	    }

	    DXGetArrayInfo(src, NULL, &t, &cat, &r, s);
	    size = DXGetItemSize(src);

	    /*
	     * If the input component is constant regular, then 
	     * produce an appropriately sized constant regular
	     * partition component.
	     */
	    if (DXQueryConstantArray(src, NULL, NULL))
	    {
		dst = (Array)DXNewConstantArrayV(n, DXGetConstantArrayData(src),
						     t, cat, r, s);
		if (!dst)
		    goto error;
		else
		    goto dst_done;
	    }

	    /*
	     * OK.  Its a non-constant array that doesn't appear in the
	     * template.  Create a new array and copy in the relevant portions.
	     */
	    dst = DXNewArrayV(t, cat, r, s);
	    if (! dst)
		goto error;
	    
	    if (! DXAddArrayData(dst, 0, n, NULL))
		goto error;

	    srcData = (byte *)DXGetArrayData(src);
	    dstData = (byte *)DXGetArrayData(dst);

	    for (k = 0; k < ndim; k++)
		srcData += offset[k]*inStrides[k]*size;

	    _dxfPermute(ndim, dstData, outStrides, outCounts,
					size, srcData, inStrides);

dst_done:
	    DXCopyAttributes((Object)dst, (Object)src);

	    DXSetComponentValue(f, name, (Object)dst);
	    dst = NULL;

	}

	DXSetMember((Group)out, NULL, (Object)f);
	f = NULL;
    }

    return out;

error:
    DXFree(d);
    DXFree(o);
    DXDelete((Object)dst);
    DXDelete((Object)f);
    DXDelete((Object)out);

    return NULL;
}
		
static CompositeField
CpPartitionTemplate_Irreg(Field in, CompositeField template)
{
    int            i;
    CompositeField out = NULL;
    Field          c;
    Field	   f = NULL;
    Array	   dst = NULL;
    Pointer 	   d = NULL, o = NULL;

    out = DXNewCompositeField();
    if (! out)
	goto error;

    i = 0;
    while((c = (Field)DXGetEnumeratedMember((Group)template, i++, NULL)) != NULL)
    {
	Array  pRefArray = (Array)DXGetComponentValue(c, "prefs");
	Array  cRefArray = (Array)DXGetComponentValue(c, "crefs");
	Array  src;
	int    *pRefs, *cRefs;
	int    np, nc;
	int    psz, csz;
	char   *name;
	int    j;

	f = (Field)DXCopy((Object)in, COPY_STRUCTURE);
	if (! f)
	    goto error;
	
	DXGetArrayInfo(pRefArray, &np, NULL, NULL, NULL, NULL);
	DXGetArrayInfo(cRefArray, &nc, NULL, NULL, NULL, NULL);

	psz = DXGetItemSize(pRefArray);
	csz = DXGetItemSize(cRefArray);

	pRefs = (int *)DXGetArrayData(pRefArray);
	cRefs = (int *)DXGetArrayData(cRefArray);

	j = 0;
	while (NULL != (src=(Array)DXGetEnumeratedComponentValue(in, j++, &name)))
	{
	    Object   attr;
	    int      *refs, size, n;
	    byte    *srcData, *dstData;
	    int      k;
	    Type     t;
	    Category cat;
	    int      r, s[MAXDIM];

	    /*
	     * If the component exists in the template, use it.  Otherwise,
	     * carve the necessary portion out of the corresponding component
	     * of the input array.
	     */
	    dst = (Array)DXGetComponentValue(c, name);
	    if (dst)
		goto dst_done;
	    
	    if (! strcmp(name, "neighbors") ||
		DXGetComponentAttribute(in, name, "der"))
	    {
		DXDeleteComponent(f, name);
		continue;
	    }

	    if (DXGetComponentAttribute(in, name, "ref"))
	    {
		DXWarning("component %s contains references and cannot %s\n"
			  "partition.  It will be deleted.\n", name);
		DXDeleteComponent(f, name);
		continue;
	    }

	    /* 
	     * Select appropriate grid parameters depending on the 
	     * component's dependency.
	     */
	    attr = (Object)DXGetComponentAttribute(in, name, "dep");
	    if (! attr)
		continue;
	    
	    if (!strcmp(DXGetString((String)attr), "positions"))
	    {
		refs = pRefs;
		n    = np;
		size = psz;
	    }
	    else
	    {
		refs = cRefs;
		n    = nc;
		size = csz;
	    }

	    DXGetArrayInfo(src, NULL, &t, &cat, &r, s);
	    size = DXGetItemSize(src);

	    /*
	     * If the input component is constant regular, then 
	     * produce an appropriately sized constant regular
	     * partition component.
	     */
	    if (DXQueryConstantArray(src, NULL, NULL))
	    {
		dst = (Array)DXNewConstantArrayV(n, DXGetConstantArrayData(src),
						     t, cat, r, s);
		if (!dst)
		    goto error;
		else
		    goto dst_done;
	    }
		
	    /*
	     * OK.  Its a non-constant array that doesn't appear in the
	     * template.  Create a new array and copy in the relevant portions.
	     */
	    dst = DXNewArrayV(t, cat, r, s);
	    if (! dst)
		goto error;
		
	    if (! DXAddArrayData(dst, 0, n, NULL))
		goto error;

	    srcData = (byte *)DXGetArrayData(src);
	    dstData = (byte *)DXGetArrayData(dst);

	    for (k = 0; k < n; k++)
	    {
		memcpy(dstData, srcData+size*(*refs), size);
		dstData += size;
		refs ++;
	    }

dst_done:
	    if (! DXCopyAttributes((Object)dst, (Object)src))
		goto error;
	    
	    if (! DXSetComponentValue(f, name, (Object)dst))
		goto error;
	    dst = NULL;

	}

	if (! DXSetMember((Group)out, NULL, (Object)f))
	    goto error;
	f = NULL;
    }

    return out;

error:
    DXFree(d);
    DXFree(o);
    DXDelete((Object)dst);
    DXDelete((Object)f);
    DXDelete((Object)out);

    return NULL;
}

static int
Cmp(Key k0, Key k1)
{
    HElement *h0 = (HElement *)k0;
    HElement *h1 = (HElement *)k1;

    return ! ((h0->p == h1->p) && (h0->c == h1->c));
}

static PseudoKey
Hash(Key k)
{
    long p;
    HElement *h = (HElement *)k;

    p = (((long)h->p) ^ (((long)h->c) << 12)) >> 2;

    return (PseudoKey)p;
}
