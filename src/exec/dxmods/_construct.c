/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 */

#include <dxconfig.h>


#include <string.h>
#include <dx/dx.h>
#include "_construct.h"

static Error AddData(Field, Array);
static Field HardCase(Array, Array);
static Field MakeIR(Array, int *, int, int);
static Field MakeII(Array);
static Field MakeRR(Array, Array, Array);
static Array DefaultOrigin(Array);
static Array DefaultDeltas(Array);
static Array DefaultCounts(Array);
static Array MyExtractIntVector(Array);
static Array MyExtractFloatVector(Array);
#if 0
static Array MyExtractFloat(Array);
#endif

Field
_dxfConstruct(Array o, Array d, Array c, Array data)
{
    Field f=NULL;
    Type type;
    int rank, shape[32], *c_ptr, i;
    Category category;
    
    if (o) {
	if (DXGetObjectClass((Object)o) != CLASS_ARRAY)
	{
	    DXSetError(ERROR_DATA_INVALID, "#10220","origin");
	    goto error;
	}
        DXGetArrayInfo((Array)o, NULL, &type, &category, NULL, NULL);
        if ((type != TYPE_FLOAT)&&(type != TYPE_INT)) 
        {
            DXSetError(ERROR_BAD_PARAMETER,
                     "origin must be of type float or integer");
            goto error;
        } 
        if (category != CATEGORY_REAL) 
        {
            DXSetError(ERROR_BAD_PARAMETER,
                     "origin must be category real");
            goto error;
        }
        /* DXReference((Object)o);  */
    }


    if (d) {
	if (DXGetObjectClass((Object)d) != CLASS_ARRAY)
	{
	    DXSetError(ERROR_DATA_INVALID, "#10220", "deltas");
	    goto error;
	}
        DXGetArrayInfo((Array)d, NULL, &type, &category, NULL, NULL);
        if ((type != TYPE_FLOAT)&&(type != TYPE_INT)) 
        {
            DXSetError(ERROR_BAD_PARAMETER,
                     "deltas must be of type float or integer");
            goto error;
        }
        if (category != CATEGORY_REAL) 
        {
            DXSetError(ERROR_BAD_PARAMETER,
                     "deltas must be category real");
            goto error;
        }
        /* DXReference((Object)d);   */
    }


    if (c) {
	if (DXGetObjectClass((Object)c) != CLASS_ARRAY)
	{
	    DXSetError(ERROR_DATA_INVALID, "#10541", "counts");
	    goto error;
	}
        DXGetArrayInfo((Array)c, NULL, &type, &category, &rank, shape);
        c_ptr = (int *)DXGetArrayData((Array)c);
        if (rank==0) {
	    if (c_ptr[0] <= 0) {
		DXSetError(ERROR_BAD_PARAMETER,"#10021","counts");
		goto error;
	    }
        }
        else if (rank==1) {
            for (i=0;i<shape[0];i++) {
		if (c_ptr[i] <= 0) {
		    DXSetError(ERROR_BAD_PARAMETER,"#10021", "counts");
		    goto error;
		}
            }
        }
        else {
	    DXSetError(ERROR_BAD_PARAMETER, "#10541", "counts");
	    goto error;
        }
        if (type != TYPE_INT)
	{
	    DXSetError(ERROR_BAD_PARAMETER,"#10011", "counts");
	    goto error;
        }
        if (category != CATEGORY_REAL) 
        {
            DXSetError(ERROR_BAD_PARAMETER,
                     "counts must be category real");
            goto error;
        }
        /* DXReference((Object)c);   */
    }


    if (data) 
    {
	if (DXGetObjectClass((Object)data) != CLASS_ARRAY) 
	{
	    /* special case: it might be a single string. */
            if (DXGetObjectClass((Object)data) == CLASS_STRING)
		data = (Array)DXMakeStringList(1, DXGetString((String)data));

            else {
		DXSetError(ERROR_DATA_INVALID, "#10260", "data");
		goto error;
            }
	}
        DXGetArrayInfo((Array)data, NULL, &type, &category, NULL, NULL);
        if (category != CATEGORY_REAL) {
	    DXSetError(ERROR_BAD_PARAMETER, "data must be category real");
            goto error;
        }
    }
    
    
    if (!o && !d && !c) {
        f = DXNewField();
        if (data) {
	    DXWarning("Construct data provided without origin, deltas, or counts; empty field created");
        }
        return f;
    }


    if ( o && !d && !c)
    {
	f = MakeII(o);
    }
    else if ( o && !d && c)
    {
	f = HardCase(o, c);
    }
    else
    {
	Array driver;

	if (o)
	    driver = o;
	else if (d)
	    driver = d;
	else driver = NULL;

        DXReference((Object)o);
        DXReference((Object)d);
        DXReference((Object)c);
	if (! o) {
	    o = DefaultOrigin(driver);
        }

	if (! c) {
	    c = DefaultCounts(driver);
        }

	if (! d) {
	    d = DefaultDeltas(driver);
        }

	f = MakeRR(o,d,c);
        DXDelete((Object)d);
        DXDelete((Object)o);
        DXDelete((Object)c);
    }

    if (! f)
	goto error;

    if (data)
	if (! AddData(f, data))
	    goto error;
    
    if (! DXEndField(f))
	goto error;
   
    return f;

error:
    DXDelete((Object)f);
    return NULL;
}

static Error
AddData(Field f, Array dA)
{
    Array cA, pA, outD = NULL, newdata=NULL;
    int   nC, nP, nD;
    int   r, s[32];
    Type type;
    Category category;
    char *att="";

    cA = (Array)DXGetComponentValue(f, "connections");
    
    pA = (Array)DXGetComponentValue(f, "positions");
    if (! pA)
	goto error;
    
    if (cA) 
         DXGetArrayInfo(cA, &nC, NULL, NULL, NULL, NULL);
    else
         /* no connections */
         nC = -1;
    DXGetArrayInfo(pA, &nP, NULL, NULL, NULL, NULL);
    DXGetArrayInfo(dA, &nD, &type, &category, &r, s);


    
    if (nD == 1 && nP != 1) 
    {
     Array tmp;
     outD = DXNewArrayV(type, category, r, s); 
     if (!(DXAddArrayData(outD,0,nD,(Pointer)DXGetArrayData(dA))))
        goto error;
     if (! outD)
	goto error;
     tmp = (Array)DXNewConstantArrayV(nP, DXGetArrayData(outD), type,
                                      CATEGORY_REAL, r,  s);
     if (! tmp) {
       goto error;
     }
	    
     DXDelete((Object)outD);
     outD = tmp;
     DXSetAttribute((Object)outD,"dep", (Object)DXNewString("positions"));
     nD = nP;
    }
    else
      outD = dA;


    /* check the attribute of the original array */
    DXGetStringAttribute((Object)outD, "dep", &att);
       

    if (nD == nP)
    {
        if (strcmp(att,"positions")) {
          /* need to make a new array because the attribute is different */
          newdata = (Array)_dxfReallyCopyArray(outD);          
          if (! DXSetComponentValue(f, "data", (Object)newdata))
  	      goto error;
          newdata = NULL;
        }
        else {
          if (! DXSetComponentValue(f, "data", (Object)outD))
  	      goto error;
          outD = NULL;
        }

	if (! DXSetComponentAttribute(f, "data", "dep",
				(Object)DXNewString("positions")))
	    goto error;

    }
    else if (nD == nC)
    {
        if (strcmp(att,"connections")) {
          /* need to make a new array because the attribute is different */
          newdata = (Array)_dxfReallyCopyArray(outD);          
          if (! DXSetComponentValue(f, "data", (Object)newdata))
  	      goto error;
          newdata = NULL;
        }
        else {
          if (! DXSetComponentValue(f, "data", (Object)outD))
  	      goto error;
          outD = NULL;
        }
	if (! DXSetComponentAttribute(f, "data", "dep",
				(Object)DXNewString("connections")))
	    goto error;
    }
    else
    {
       if (nC != -1) {
	DXSetError(ERROR_DATA_INVALID, 
     "number of data elements (%d) matches neither number of positions (%d) nor number of connections (%d)", 
         nD, nP, nC);
	goto error;
       }
       else {
	DXSetError(ERROR_DATA_INVALID, 
           "number of data elements (%d) does not match number of positions(%d)", 
         nD, nP);
	goto error;
       }
    }

    return OK;

error:
    DXDelete((Object)outD);
    DXDelete((Object)newdata);

    return ERROR;
}
	    
static Field
HardCase(Array o, Array c)
{
    int counts[64], s[32], cShape;
    int r, nC, nP, i, dim;
    Field re;

    DXGetArrayInfo(o, &nP, NULL, NULL, &r, s);
    if (r == 0)
	dim = 1;
    else if (r == 1)
	dim = s[0];
    else
    {
	DXSetError(ERROR_DATA_INVALID, "#10215","origin");
	goto error;
    }

    DXGetArrayInfo(c, &nC, NULL, NULL, &r, s);

    if (nC != 1)
    {
        DXSetError(ERROR_DATA_INVALID, "#11825","counts","integer","origin");
	goto error;
    }

    if (r == 0)
    {
	s[0] = 0;
	cShape = 1;
    }
    else
	cShape = s[0];
    
    if (! DXExtractParameter((Object)c, TYPE_INT, s[0], 1, (Pointer)counts))
    {
        DXSetError(ERROR_DATA_INVALID, "#11825","counts","integer","origin");
	goto error;
    }

    if (r != 0 && r != 1)
    {
        DXSetError(ERROR_DATA_INVALID, "#11825","counts","integer","origin");
	goto error;
    }

    /*
     * If the counts shape is not 1, it specifies a max dimensionality
     * Otherwise, the deltas do.
     */
    if (r == 1 && s[0] != 1)
	dim = s[0];

    if (r == 0 || (r == 1 && s[0] == 1))
    {
	/*
	 * Expand the counts to the guessed dimensionality or until
	 * the product matches the number of positions, in which case
	 * the connections dimensionality will be lower than that of
	 * the positions.
	 */
	nC = 1;
	for (i = 0; i < dim; i++)
	{
	    nC *= counts[0];
	    counts[i] = counts[0];
	    if (nC == nP)
	    {
		dim = i + 1;
		break;
	    }
	}
    }
    else
    {
	nC = counts[0];
	for (i = 1; i < dim; i++)
	    nC *= counts[i];
    }

    if (nC == nP)
    {
	return MakeIR(o, counts, dim, nP);
    }
    else if (cShape == 1 && counts[0] == nP)
    {
	return MakeII(o);
    }
    else if (nP == 1)
    {
        Array d = DefaultDeltas(o);
	re = MakeRR(o, d, c);
        DXDelete((Object)d);
        return re;
    }
    else
    {
	DXSetError(ERROR_DATA_INVALID, "#11826","counts","origin");
	goto error;
    }

error:
    return NULL;
}

static Field
MakeIR(Array o, int *counts, int dim, int nP)
{
    int i, j;
    Array cA = NULL;
    Array pA = NULL;
    Field f = NULL;

    /* positions have to be vectors, even if 1D */
    pA = MyExtractFloatVector(o);
    if (!pA)
	goto error;
    
    f = DXNewField();
    if (! f)
	goto error;
    
    if (! DXSetComponentValue(f, "positions", (Object)pA))
	goto error;
    pA = NULL;

    for (i = 0, j = 0; i < dim; i++)
	if (counts[i] > 1)
	    counts[j++] = counts[i];
    
    dim = j;

    if (dim > 0)
    {
	cA = DXMakeGridConnectionsV(dim, counts);
	if (! cA)
	    goto error;
    
	if (! DXSetComponentValue(f, "connections", (Object)cA))
	    goto error;
	cA = NULL;

	if (dim == 1)
	{
	    if (! DXSetComponentAttribute(f, "connections", "element type",
					(Object)DXNewString("lines")))
		goto error;
	}
	else if (dim == 2)
	{
	    if (! DXSetComponentAttribute(f, "connections", "element type",
					(Object)DXNewString("quads")))
		goto error;
	}
	else if (dim == 3)
	{
	    if (! DXSetComponentAttribute(f, "connections", "element type",
					(Object)DXNewString("cubes")))
	    goto error;
	}
	else
	{
	    char str[64];

	    sprintf(str, "cubes%dD", dim);
	    if (! DXSetComponentAttribute(f, "connections", "element type",
				    (Object)DXNewString(str)))
		goto error;
	}
    }

    return f;

error:
    DXDelete((Object)pA);
    DXDelete((Object)cA);
    DXDelete((Object)f);
    return NULL;
}

static Field
MakeII(Array o)
{
    Array cA = NULL;
    Field f = NULL;
    int n;
    Type t;
    Category c;
    int r, s[32];
    Array pA = NULL;

    DXGetArrayInfo(o, &n, &t, &c, &r, s);
    if (c != CATEGORY_REAL || (r != 1 && r != 0))
    {
	DXSetError(ERROR_DATA_INVALID,"#10215", "origin");
	goto error;
    }

    /* positions have to be vectors, even if 1D */
    pA = MyExtractFloatVector(o);
    if (! pA)
	goto error;
    
    f = DXNewField();
    if (! f)
	goto error;

    if (! DXSetComponentValue(f, "positions", (Object)pA))
	goto error;
    pA = NULL;
    
    if (n > 1)
    {
        /*
	cA = (Array)DXNewPathArray(n); */
        cA = DXMakeGridConnectionsV(1,&n);
	if (! cA)
	    goto error;
    
	if (! DXSetComponentValue(f, "connections", (Object)cA))
	    goto error;
	cA = NULL;

	if (! DXSetComponentAttribute(f, "connections", "element type",
					    (Object)DXNewString("lines")))
	    goto error;
    }
	
    return f;

error:
    DXDelete((Object)pA);
    DXDelete((Object)cA);
    DXDelete((Object)f);

    return ERROR;
}

static Array
DefaultOrigin(Array driver)
{
    float p[32];
    Array a = NULL;
    int dim, i, r;

    if (driver)
    {
	DXGetArrayInfo(driver, NULL, NULL, NULL, &r, &dim);
	if (r == 0)
	    dim = 1;
    }
    else
	dim = 3;

    a = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, dim);
    if (! a)
	goto error;
    
    for (i = 0; i < dim; i++)
	p[i] = 0.0;

    if (! DXAddArrayData(a, 0, 1, (Pointer)p))
	goto error;
    
    return a;

error:
    DXDelete((Object)a);
    return NULL;
}
    
static Array
DefaultDeltas(Array driver)
{
    float p[256];
    Array a = NULL;
    int i, k, dim, r;

    if (driver)
    {
	DXGetArrayInfo(driver, NULL, NULL, NULL, &r, &dim);
	if (r == 0)
	    dim = 1;
    }
    else
	dim = 3;

    k = 0;
    for (i = 0; i < dim; i++)
	 p[k++] = 1.0;

    a = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, dim);
    if (! a)
	goto error;
    
    if (! DXAddArrayData(a, 0, 1, (Pointer)p))
	goto error;
    
    return a;

error:
    DXDelete((Object)a);
    return NULL;
}
    
static Array
DefaultCounts(Array driver)
{
    int c[16];
    Array a = NULL;
    int i, k, dim, r;

    if (driver)
    {
	DXGetArrayInfo(driver, NULL, NULL, NULL, &r, &dim);
	if (r == 0)
	    dim = 1;
    }
    else
	dim = 3;

    k = 0;
    for (i = 0; i < dim; i++)
	 c[k++] = 2;

    a = DXNewArray(TYPE_INT, CATEGORY_REAL, 1, dim);
    if (! a)
	goto error;
    
    if (! DXAddArrayData(a, 0, 1, (Pointer)c))
	goto error;
    
    return a;

error:
    DXDelete((Object)a);
    return NULL;
}

static Field
MakeRR(Array or, Array de, Array co)
{
    Array pA = NULL, cA = NULL;
    Field f = NULL;
    Array o = NULL, d = NULL, c = NULL;
    int   dim, nO, nD, nC, r, s[32], i, j;
    float *origin = NULL;
    float *deltas = NULL;
    int	  *counts = NULL;

    if (or) o = MyExtractFloatVector(or);
    if (de) d = MyExtractFloatVector(de);
    if (co) c = MyExtractIntVector(co);

    DXGetArrayInfo(o, &nO, NULL, NULL, &r, s);
    if (nO != 1 || r != 1)
    {
	DXSetError(ERROR_DATA_INVALID, "#11827","deltas",
                   "origin containing more than one point");
	goto error;
    }

    dim = s[0];

    origin = (float *)DXAllocate(dim*sizeof(float));
    if (! origin)
	goto error;

    if (! DXExtractParameter((Object)o, TYPE_FLOAT, dim, 1, (Pointer)origin))
    {
	DXSetError(ERROR_DATA_INVALID, "#10221","origin");
	goto error;
    }

    DXGetArrayInfo(d, &nD, NULL, NULL, &r, s);
    if (r != 1 || s[0] != dim)
    {
	DXSetError(ERROR_DATA_INVALID, "#11828", "deltas", "origin");
	goto error;
    }

    if (nD != 1 && nD != dim)
    {
	DXSetError(ERROR_DATA_INVALID, "#11828", "deltas","origin");
	goto error;
    }

    deltas = (float *)DXAllocate(dim*dim*sizeof(float));
    if (! deltas)
        goto error;

    if (! DXExtractParameter((Object)d, TYPE_FLOAT, dim, nD, (Pointer)deltas))
    {
	DXSetError(ERROR_DATA_INVALID, "#11828", "deltas", "origin");
	goto error;
    }

    if (nD == 1)
    {
	for (i = 1; i < dim; i++)
	    for (j = 0; j < dim; j++)
		if (i == j)
		{
		    deltas[i*dim+j] = deltas[j];
		    deltas[j] = 0.0;
		}
		else
		    deltas[i*dim+j] = 0.0;
    }

    DXGetArrayInfo(c, &nC, NULL, NULL, &r, s);
    if (nC != 1 || !((r == 1 && (s[0] == 1 || s[0] == dim)) || r == 0))
    {
	DXSetError(ERROR_DATA_INVALID,"#11826", "counts", "origin and deltas");
	goto error;
    }

    counts = (int *)DXAllocate(dim*sizeof(int));
    if (! counts)
	goto error;

    if (r == 0)
    {
	s[0] = 0;
	r = 1;
    }
    else
	r = s[0];

    if (! DXExtractParameter((Object)c, TYPE_INT, s[0], 1, (Pointer)counts))
    {
	DXSetError(ERROR_DATA_INVALID, "#11826","counts","origin and deltas");
        goto error;
    }

    for (i = r; i < dim; i++)
        counts[i] = counts[0];
    
    pA = DXMakeGridPositionsV(dim, counts, origin, deltas);
    if (! pA)
	goto error;

    for (i = j = 0; i < dim; i++)
	if (counts[i] > 1)
	    counts[j++] = counts[i];

    dim = j;
    
    f = DXNewField();
    if (! f)
	goto error;
    
    if (! DXSetComponentValue(f, "positions", (Object)pA))
	goto error;
    pA = NULL;
    
    if (dim > 0)
    {
	cA = DXMakeGridConnectionsV(dim, counts);
	if (! cA)
	    goto error;
    
	if (! DXSetComponentValue(f, "connections", (Object)cA))
	    goto error;
	cA = NULL;

	if (dim == 1)
	{
	    if (! DXSetComponentAttribute(f, "connections", "element type",
					(Object)DXNewString("lines")))
		goto error;
	}
	else if (dim == 2)
	{
	    if (! DXSetComponentAttribute(f, "connections", "element type",
					(Object)DXNewString("quads")))
		goto error;
	}
	else if (dim == 3)
	{
	    if (! DXSetComponentAttribute(f, "connections", "element type",
					(Object)DXNewString("cubes")))
	    goto error;
	}
	else
	{
	    char str[64];

	    sprintf(str, "cubes%dD", dim);
	    if (! DXSetComponentAttribute(f, "connections", "element type",
					(Object)DXNewString(str)))
		goto error;
	}
    }

    DXFree((Pointer)counts);
    DXFree((Pointer)deltas);
    DXFree((Pointer)origin);
    if (o != or) DXDelete((Object)o);
    if (c != co) DXDelete((Object)c);
    if (d != de) DXDelete((Object)d);

    return f;

error:
    DXFree((Pointer)counts);
    DXFree((Pointer)deltas);
    DXFree((Pointer)origin);
    if (o != or) DXDelete((Object)o);
    if (c != co) DXDelete((Object)c);
    if (d != de) DXDelete((Object)d);
    DXDelete((Object)pA);
    DXDelete((Object)cA);
    DXDelete((Object)f);
    return NULL;
}

static Array
MyExtractFloatVector(Array in)
{
    Array out = NULL;
    Type  t;
    Category c;
    int r, s[32];
    int nItems, i;
    float *dst;

    DXGetArrayInfo(in, &nItems, &t, &c, &r, s);
    if (r==0) {
       r = 1;
       s[0] = 1;
    }

    /* check the input array. Maybe we can use it as is */
    if (DXGetArrayClass(in)==CLASS_REGULARARRAY) {
       out = in;
       return out;
    }
    out = DXNewArrayV(TYPE_FLOAT, c, r, s);
    if (! out)
	goto error;
    
    if (! DXAddArrayData(out, 0, nItems, NULL))
	goto error;
    
    dst = (float *)DXGetArrayData(out);
    if (! dst)
	goto error;

    nItems *= DXGetItemSize(in)/DXTypeSize(t);

    switch(t)
    {
	case TYPE_DOUBLE:
	{
	    double *src;
	    
	    src = (double *)DXGetArrayData(in);
	    if (! src)
		goto error;
	    
	    for (i = 0; i < nItems; i++)
		*dst++ = (float) *src++;
	    
	    break;
	}
    

	case TYPE_FLOAT:
	{
	    float *src;
	    
	    src = (float *)DXGetArrayData(in);
	    if (! src)
		goto error;
	    
	    for (i = 0; i < nItems; i++)
		*dst++ = (float) *src++;
	    
	    break;
	}

	case TYPE_INT:
	{
	    int *src;
	    
	    src = (int *)DXGetArrayData(in);
	    if (! src)
		goto error;
	    
	    for (i = 0; i < nItems; i++)
		*dst++ = (float) *src++;
	    
	    break;
	}

	case TYPE_SHORT:
	{
	    short *src;
	    
	    src = (short *)DXGetArrayData(in);
	    if (! src)
		goto error;
	    
	    for (i = 0; i < nItems; i++)
		*dst++ = (float) *src++;
	    
	    break;
	}

	case TYPE_BYTE:
	{
	    byte *src;
	    
	    src = (byte *)DXGetArrayData(in);
	    if (! src)
		goto error;
	    
	    for (i = 0; i < nItems; i++)
		*dst++ = (float) *src++;
	    
	    break;
	}

	case TYPE_UBYTE:
	{
	    ubyte *src;
	    
	    src = (ubyte *)DXGetArrayData(in);
	    if (! src)
		goto error;
	    
	    for (i = 0; i < nItems; i++)
		*dst++ = (float) *src++;
	    
	    break;
	}

	case TYPE_USHORT:
	{
	    ushort *src;
	    
	    src = (ushort *)DXGetArrayData(in);
	    if (! src)
		goto error;
	    
	    for (i = 0; i < nItems; i++)
		*dst++ = (float) *src++;
	    
	    break;
	}

	case TYPE_UINT:
	{
	    uint *src;
	    
	    src = (uint *)DXGetArrayData(in);
	    if (! src)
		goto error;
	    
	    for (i = 0; i < nItems; i++)
		*dst++ = (float) *src++;
	    
	    break;
	}
        default:
	    break;
    }
	    
    return out;

error:
    DXDelete((Object)out);
    return NULL;
}

#if 0
static Array
MyExtractFloat(Array in)
{
    Array out = NULL;
    Type  t;
    Category c;
    int r, s[32];
    int nItems, i;
    float *dst;

    DXGetArrayInfo(in, &nItems, &t, &c, &r, s);
    if (r==0)
      s[0]=1;

    out = DXNewArrayV(TYPE_FLOAT, c, r, s);
    if (! out)
	goto error;
    
    if (! DXAddArrayData(out, 0, nItems, NULL))
	goto error;
    
    dst = (float *)DXGetArrayData(out);
    if (! dst)
	goto error;

    nItems *= DXGetItemSize(in)/DXTypeSize(t);

    switch(t)
    {
	case TYPE_DOUBLE:
	{
	    double *src;
	    
	    src = (double *)DXGetArrayData(in);
	    if (! src)
		goto error;
	    
	    for (i = 0; i < nItems; i++)
		*dst++ = (float) *src++;
	    
	    break;
	}
    

	case TYPE_FLOAT:
	{
	    float *src;
	    
	    src = (float *)DXGetArrayData(in);
	    if (! src)
		goto error;
	    
	    for (i = 0; i < nItems; i++)
		*dst++ = (float) *src++;
	    
	    break;
	}

	case TYPE_INT:
	{
	    int *src;
	    
	    src = (int *)DXGetArrayData(in);
	    if (! src)
		goto error;
	    
	    for (i = 0; i < nItems; i++)
		*dst++ = (float) *src++;
	    
	    break;
	}

	case TYPE_SHORT:
	{
	    short *src;
	    
	    src = (short *)DXGetArrayData(in);
	    if (! src)
		goto error;
	    
	    for (i = 0; i < nItems; i++)
		*dst++ = (float) *src++;
	    
	    break;
	}

	case TYPE_UBYTE:
	{
	    byte *src;
	    
	    src = (byte *)DXGetArrayData(in);
	    if (! src)
		goto error;
	    
	    for (i = 0; i < nItems; i++)
		*dst++ = (float) *src++;
	    
	    break;
	}
        default:
	    break;
    }
	    
    return out;

error:
    DXDelete((Object)out);
    return NULL;
}
#endif

static Array
MyExtractIntVector(Array in)
{
    Array out = NULL;
    Type  t;
    Category c;
    int r, s[32];
    int nItems, i;
    int *dst;

    DXGetArrayInfo(in, &nItems, &t, &c, &r, s);
    if (r==0)
      s[0]=1;

    out = DXNewArrayV(TYPE_INT, c, r, s);
    if (! out)
	goto error;
    
    if (! DXAddArrayData(out, 0, nItems, NULL))
	goto error;
    
    dst = (int *)DXGetArrayData(out);
    if (! dst)
	goto error;

    nItems *= DXGetItemSize(in)/DXTypeSize(t);

    switch(t)
    {
	case TYPE_INT:
	{
	    int *src;
	    
	    src = (int *)DXGetArrayData(in);
	    if (! src)
		goto error;
	    
	    for (i = 0; i < nItems; i++)
		*dst++ = (int) *src++;
	    
	    break;
	}

	case TYPE_SHORT:
	{
	    short *src;
	    
	    src = (short *)DXGetArrayData(in);
	    if (! src)
		goto error;
	    
	    for (i = 0; i < nItems; i++)
		*dst++ = (int) *src++;
	    
	    break;
	}

	case TYPE_UBYTE:
	{
	    byte *src;
	    
	    src = (byte *)DXGetArrayData(in);
	    if (! src)
		goto error;
	    
	    for (i = 0; i < nItems; i++)
		*dst++ = (int) *src++;
	    
	    break;
	}
        default:
            DXSetError(ERROR_BAD_PARAMETER,"#11829","counts");
            goto error;
    }
	    
    return out;

error:
    DXDelete((Object)out);
    return NULL;
}
