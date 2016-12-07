/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#include <string.h>
#include <dx/dx.h>
#include "histogram.h"

#ifndef TRUE
#define	TRUE		1
#define	FALSE		0
#endif

#ifndef	MAXDIMS
#define	MAXDIMS		32
#endif

#define	DEFAULT_BINS	100
#define	DEFAULT_BINS_C	256

#define	TASK_GROUP(_f,_d,_n)\
{\
    if (_n > 0)\
    {\
	if (! DXCreateTaskGroup ())\
	    goto cleanup;\
	if (! DXAddLikeTasks ((PFE) _f, (Pointer) &(_d), sizeof (_d), 1.0, _n))\
	{\
	    DXAbortTaskGroup ();\
	    goto cleanup;\
	}\
	if (! DXExecuteTaskGroup ())\
	    goto cleanup;\
    }\
}

typedef Error		(*PFE)();

typedef struct EQData	*EQDP;

typedef struct EQData
{
    Object	obj;			/* copy of object to equalize	*/
    int		nbin;			/* number of bins or 0		*/
    int		minset;			/* min has been set		*/
    int		maxset;			/* max has been set		*/
    float	minv;			/* min value			*/
    float	maxv;			/* max value			*/
					/* transfer functions		*/
    EQDP	resolved;		/* where numbers were resolved	*/
    Object	ito;			/* input  histogram		*/
    Object	oto;			/* output histogram		*/
    char	*itf;			/* input  named function	*/
    char	*otf;			/* output named function	*/
    Pointer	base;			/* handle to allocated space	*/
    float	*idpd;			/* input  delta prob. dist.	*/
    float	*icpd;			/* input  cumm. prob. dist.	*/
    float	*odpd;			/* input  delta prob. dist.	*/
    float	*ocpd;			/* input  cumm. prob. dist.	*/
} EQData;

Error DXAddLikeTasks(PFE, Pointer, int, double, int); /* from libdx/task.c */

static Error ComputeCPD (int nbin, float *del, float *cum, int invert);
static Error Equalize (EQData *eq, Type type, int n, Pointer src, Pointer dst);
static Error EqualizeObject (EQData *eq);
static Error EqualizeField  (EQData *eq);
static Error EqualizeGroup  (EQData *eq);
static Error FuncDistribution (char *f,  int nbin, float *pd);
static Error HistDistribution (Object o, int nbin, float *pd);
static int   IsAHistogram (Object obj);
static void  Resample (int srcn, int *src, int dstn, float *dst);
static Error ResolveTransferFunctions (EQData *eq);
static Error SimpleArgs (Object *in, EQData *eq);
static Error TransferFunctions (Object *in, EQData *eq);
static Error WorkerEG (EQData *eq, int n);

/*
 * Equalize (input, nbins, min, max, ihist, ohist)
 */

#define	Input	in[0]
#define	NBin	in[1]
#define	Min	in[2]
#define	Max	in[3]
#define	IDist	in[4]
#define	ODist	in[5]

Error
m_Equalize (Object *in, Object *out)
{
    Error	ret	= ERROR;
    EQData	eq;

    memset (&eq, 0, sizeof (eq));

    if (! SimpleArgs (in, &eq))
	goto cleanup;

    if (! TransferFunctions (in, &eq))
	goto cleanup;

    eq.obj = DXCopy (Input, COPY_STRUCTURE);
    if (! eq.obj)
	goto cleanup;
    
    if (! EqualizeObject (&eq))
	goto cleanup;

    out[0] = eq.obj;
    eq.obj = NULL;
    ret = OK;

cleanup:
    if (eq.obj != Input)
	DXDelete (eq.obj);
    DXDelete (eq.ito);
    DXDelete (eq.oto);
    return (ret);
}


static Error
SimpleArgs (Object *in, EQData *eq)
{
    Error	ret	= ERROR;

    if (! Input)
    {
	DXSetError (ERROR_BAD_PARAMETER, "#10000", "input");
	goto cleanup;
    }

    if (NBin)
    {
	if ((! DXExtractInteger (NBin, &eq->nbin)) || (eq->nbin < 0))
	{
 	    DXSetError (ERROR_BAD_PARAMETER, "#10030", "number of bins");
	    goto cleanup;
	}
    }
    
    if (Min)
    {
	if (! DXExtractFloat (Min, &eq->minv))
	{
	    if (! DXStatistics (Min, "data", &eq->minv, &eq->maxv, NULL, NULL))
	    {
		DXSetError (ERROR_BAD_PARAMETER, "#10080", "bin mininum");
		goto cleanup;
	    }
	    if (! Max)
		eq->maxset = TRUE;
	}
	eq->minset = TRUE;
    }

    if (Max)
    {
	if (! DXExtractFloat (Max, &eq->maxv) &&
	    ! DXStatistics   (Max, "data", NULL, &eq->maxv, NULL, NULL))
	{
	    DXSetError (ERROR_BAD_PARAMETER, "#10080", "bin maximum");
	    goto cleanup;
	}
	eq->maxset = TRUE;
    }
    
    if (eq->minset && eq->maxset && (eq->minv >= eq->maxv))
    {
	DXSetError (ERROR_BAD_PARAMETER, "#11220", "min", "or equal to max");
	goto cleanup;
    }

    ret = OK;

cleanup:
    return (ret);
}


static Error
TransferFunctions (Object *in, EQData *eq)
{
    Error	ret	= ERROR;
    Object	ihist	= NULL;
    float	*minp	= NULL;
    float	*maxp	= NULL;
    char	*c;

    /*
     * Compute the input transfer function.  If no input distribution was
     * specified then just histogram the input for now.  Otherwise, it may
     * be one of several things:
     * $$$$$ 1.  A string specifying the desired function
     * 2.  A previously computed histogram
     * $$$$$ 3.  An object whose histogram we will take
     */

    if (eq->minset)
	minp = &eq->minv;
    
    if (eq->maxset)
	maxp = &eq->maxv;
    
    if (IDist)
    {
	if (IsAHistogram (IDist))
	    eq->ito = DXReference (IDist);
	else if (DXExtractString (IDist, &c))
	    eq->itf = c;
	else
	{
	    DXSetError (ERROR_BAD_PARAMETER, "#10370", "ihist", "a histogram");
	    goto cleanup;
	}
    }
    else
    {
	ihist = _dxfHistogram (Input, eq->nbin, minp, maxp, FALSE);
	if (! ihist)
	    goto cleanup;
	eq->ito = DXReference (ihist);
	ihist = NULL;
    }

    /*
     * Compute the output transfer function.  A similar set of thing apply
     * here.
     */

    if (ODist)
    {
	if (IsAHistogram (ODist))
	    eq->oto = DXReference (ODist);
	else if (DXExtractString (ODist, &c))
	    eq->otf = c;
	else
	{
	    DXSetError (ERROR_BAD_PARAMETER, "#10370", "ohist", "a histogram");
	    goto cleanup;
	}
    }

    ret = OK;

cleanup:
    DXDelete (ihist);
    return (ret);
}


/*
 * Checks to make sure that this looks like something produced by
 * the Histogram module.
 */

static int
IsAHistogram (Object obj)
{
    Array	ad;
    Array	ac;
    Array	ap;
    Class	c;
    int		n;
    Object	attr;
    char	*str;

    if (obj == NULL)
	return (FALSE);
    if (DXGetObjectClass (obj) != CLASS_FIELD)
	return (FALSE);
    
    ad = (Array) DXGetComponentValue ((Field) obj, "data");
    ac = (Array) DXGetComponentValue ((Field) obj, "connections");
    ap = (Array) DXGetComponentValue ((Field) obj, "positions");
    if (! ad || ! ac || ! ap)
	return (FALSE);

    if (! DXTypeCheckV (ad, TYPE_INT, CATEGORY_REAL, 0, NULL))
	return (FALSE);
    
    /*
     * Make sure that the connections are a 1D entity.  Unfortunately Construct
     * wraps the simple path array in a 1D mesh array.
     */

    c = DXGetArrayClass (ac);
    if (c != CLASS_PATHARRAY && c != CLASS_MESHARRAY)
	return (FALSE);
    if (c == CLASS_MESHARRAY)
    {
	if (! DXQueryGridConnections (ac, &n, NULL) || n != 1)
	    return (FALSE);
    }

    /*
     * Make sure that the positions are a 1D entity.  Unfortunately Construct
     * wraps the simple regular array in a 1D product array.
     */

    c = DXGetArrayClass (ap);
    if (c != CLASS_REGULARARRAY && c != CLASS_PRODUCTARRAY)
	return (FALSE);
    if (c == CLASS_PRODUCTARRAY)
    {
	if (! DXQueryGridPositions (ap, &n, NULL, NULL, NULL) || n != 1)
	    return (FALSE);
    }

    attr = DXGetAttribute ((Object) ac, "element type");
    if (attr == NULL || ! DXExtractString (attr, &str) ||
	strcmp (str, "lines"))
	return (FALSE);

    attr = DXGetAttribute ((Object) ad, "dep");
    if (attr == NULL || ! DXExtractString (attr, &str) ||
	strcmp (str, "connections"))
	return (FALSE);

    /*
     * Could also check that:
     *     ac ref positions
     */

    return (TRUE);
}


static Error
EqualizeObject (EQData *eq)
{
    Object	kid;
    EQData	leq;

    switch (DXGetObjectClass (eq->obj))
    {
	case CLASS_FIELD:
	    return (EqualizeField (eq));

	case CLASS_GROUP:
	    return (EqualizeGroup (eq));

	case CLASS_XFORM:
	    if (! DXGetXformInfo ((Xform) eq->obj, &kid, NULL))
		return (ERROR);
	    leq = *eq;
	    leq.obj = kid;
	    return (EqualizeObject (&leq));

	case CLASS_CLIPPED:
	    if (! DXGetClippedInfo ((Clipped) eq->obj, &kid, NULL))
		return (ERROR);
	    leq = *eq;
	    leq.obj = kid;
	    return (EqualizeObject (&leq));

	default:
	    DXSetError (ERROR_BAD_PARAMETER, "#10190", "input");
	    return (ERROR);
    }
}


static Error
WorkerEG (EQData *eq, int n)
{
    EQData	leq;

    leq = *eq;

    leq.obj = DXGetEnumeratedMember ((Group) leq.obj, n, NULL);
    if (leq.ito && DXGetObjectClass (leq.ito) != CLASS_FIELD)
	leq.ito = DXGetEnumeratedMember ((Group) leq.ito, n, NULL);
    return (EqualizeObject (&leq));
}


static Error
EqualizeGroup (EQData *eq)
{
    EQData	leq;
    int		n;
    Class	c;
    Error	ret	= ERROR;

    leq = *eq;
    for (n = 0; DXGetEnumeratedMember ((Group) leq.obj, n, NULL); n++)
	continue;

    c = DXGetGroupClass ((Group) leq.obj);
    if (c == CLASS_COMPOSITEFIELD || c == CLASS_SERIES)
	if (! ResolveTransferFunctions (&leq))
	    goto cleanup;

    TASK_GROUP (WorkerEG, leq, n);
    
    ret = OK;

cleanup:
    if (&leq == leq.resolved)
	DXFree (leq.base);
    return (ret);
}


static Error
EqualizeField (EQData *eq)
{
    Error	ret	= ERROR;
    EQData	leq;
    Field	f;
    Array	srca;
    Array	dsta	= NULL;
    int		items;
    Type	type;
    Category	cat;
    int		rank;
    int		shape[MAXDIMS];
    Pointer	src;
    Pointer	dst;

    f = (Field) eq->obj;
    if (DXEmptyField (f))
	return (OK);

    leq = *eq;

    if (! ResolveTransferFunctions (&leq))
	goto cleanup;

    srca = (Array) DXGetComponentValue (f, "data");
    if (! srca)
    {
	DXSetError (ERROR_BAD_PARAMETER, "#10240", "data");
	goto cleanup;
    }

    if (! DXGetArrayInfo (srca, &items, &type, &cat, &rank, shape))
	goto cleanup;

    if (cat != CATEGORY_REAL)
    {
	DXSetError (ERROR_BAD_PARAMETER, "#11150", "data");
	goto cleanup;
    }
    if (! (rank == 0 || (rank == 1 && shape[0] == 1)))
    {
	DXSetError (ERROR_BAD_PARAMETER, "#10370", "data",
		    "rank 0 or rank 1 shape 1");
	goto cleanup;
    }

    dsta = DXNewArrayV (type, cat, rank, shape);
    if (! dsta)
	goto cleanup;
    if (! DXAddArrayData (dsta, 0, items, NULL))
	goto cleanup;

    src = DXGetArrayData (srca);
    dst = DXGetArrayData (dsta);
    if (src == NULL || dst == NULL)
	goto cleanup;

    if (! Equalize (&leq, type, items, src, dst))
	goto cleanup;

    if (! DXCopyAttributes ((Object) dsta, (Object) srca))
	goto cleanup;
    if (! DXSetComponentValue (f, "data", (Object) dsta))
	goto cleanup;
    dsta = NULL;
    if (! DXChangedComponentValues (f, "data"))
	goto cleanup;
    if (! DXEndField (f))
	goto cleanup;
    
    ret = OK;

cleanup:
    if (&leq == leq.resolved)
	DXFree (leq.base);
    DXDelete ((Object) dsta);
    return (ret);
}


static Error
ResolveTransferFunctions (EQData *eq)
{
    Error	ret	= ERROR;
    float	minv;
    float	maxv;
    Type	type;
    int		nbin;
    int		size;
    Pointer	base	= NULL;
    int		output;
    float	*idpd	= NULL;		/* input  delta prob. dist.	*/
    float	*icpd	= NULL;		/* input  cumm. prob. dist.	*/
    float	*odpd	= NULL;		/* output delta prob. dist.	*/
    float	*ocpd	= NULL;		/* output cumm. prob. dist.	*/

    if (eq->resolved != NULL)
	return (OK);

    /*
     * Make sure that we have set the min and max values.
     */

    if (! eq->maxset || ! eq->minset)
    {
	if (! DXStatistics   (eq->obj, "data", &minv, &maxv, NULL, NULL))
	    goto cleanup;
	if (! eq->minset)
	    eq->minv = minv;
	if (! eq->maxset)
	    eq->maxv = maxv;
	eq->minset = TRUE;
	eq->maxset = TRUE;
    }

    minv = eq->minv;
    maxv = eq->maxv;

    /*
     * Make sure that the number of bins is set.
     */
    
    if (eq->nbin == 0)
    {
	if (! DXGetType (eq->obj, &type, NULL, NULL, NULL))
	    goto cleanup;

	if (type == TYPE_UBYTE)
	    eq->nbin = (int) maxv - (int) minv + 1;
	else
	    eq->nbin = DEFAULT_BINS;
    }

    nbin = eq->nbin;

    /*
     * Allocate enough space to compute the probability distributions,
     * both delta and cummulative, for the input and output.
     */

    output = eq->oto || eq->otf;
    size   = output ? 4 : 2;
    size  *= sizeof (float);
    size  *= nbin;
    base   = DXAllocate (size);
    if (! base)
	goto cleanup;
    
    eq->idpd = idpd = (float *) base;
    eq->icpd = icpd = idpd + nbin;
    if (output)
    {
	eq->odpd = odpd = icpd + nbin;
	eq->ocpd = ocpd = odpd + nbin;
    }

    /*
     * Extract/Compute the input and output functions.  If we are extracting
     * the we may also have to resample it so that it fits into the right
     * number of bins.
     */

    if (eq->ito && ! HistDistribution (eq->ito, nbin, idpd))
	goto cleanup;
    if (eq->itf && ! FuncDistribution (eq->itf, nbin, idpd))
	goto cleanup;
    if (! ComputeCPD (nbin, idpd, icpd, FALSE))
	goto cleanup;

    if (output)
    {
	if (eq->oto && ! HistDistribution (eq->oto, nbin, odpd))
	    goto cleanup;
	if (eq->otf && ! FuncDistribution (eq->otf, nbin, odpd))
	    goto cleanup;
	if (! ComputeCPD (nbin, odpd, ocpd, TRUE))
	    goto cleanup;
    }

    eq->resolved = eq;
    eq->base = base;
    base = NULL;
    ret = OK;

cleanup:
    DXFree (base);
    return (ret);
}


static Error
HistDistribution (Object o, int nbin, float *pd)
{
    Field	f;
    Error	ret	= ERROR;
    Array	a;
    int		items;
    Type	type;
    Category	cat;
    int         i;
    int		rank;
    int		shape[MAXDIMS];
    int 	*data;

    if (DXGetObjectClass (o) != CLASS_FIELD)
    {
	DXSetError (ERROR_BAD_PARAMETER, "#10370", "histogram", "a field");
	goto cleanup;
    }

    f = (Field) o;
    a = (Array) DXGetComponentValue (f, "data");
    if (! a)
    {
	DXSetError (ERROR_BAD_PARAMETER, "#10240", "data");
	goto cleanup;
    }

    if (! DXGetArrayInfo (a, &items, &type, &cat, &rank, shape))
	goto cleanup;
    
    if ((cat != CATEGORY_REAL) ||
        (! (rank == 0 || (rank == 1 && shape[0] == 1))))
    {
	DXSetError (ERROR_BAD_PARAMETER, "#10370", "histogram", 
		  "a real integer field");
	goto cleanup;
    }

    data = (int *) DXGetArrayData (a);
    if (! data)
	goto cleanup;

    if (items == nbin) {
	for (i=0; i<items; i++)
	    *pd++ = (float)(*data++);
    } else
	Resample (items, data, nbin, pd);
    
    ret = OK;

cleanup:
    return (ret);
}



static void
Resample (int srcn, int *src, int dstn, float *dst)
{
    int		u;
    int		x;
    double	acc;
    double	intensity;
    double	insfac;
    double	sizfac;
    double	inseg;
    double	outseg;

    sizfac = (double) dstn / (double) (srcn-1);
    insfac = 1.0 / sizfac;
    outseg = insfac;
    inseg  = 1.0;
    acc    = 0.0;

    for (x = 0, u = 0; x < dstn; )
    {
	intensity  = inseg * (double) src[u];
	intensity += (1.0 - inseg) * (double) src[u + 1];
	if (inseg < outseg)
	{
	    acc += intensity * inseg;
	    outseg -= inseg;
	    inseg = 1.0;
	    u++;
	}
	else
	{
	    acc += intensity * outseg;
	    dst[x] = (float) (acc * sizfac);
	    acc = 0.0;
	    inseg -= outseg;
	    outseg = insfac;
	    x++;
	}
    }
}


static Error
FuncDistribution (char *f, int nbin, float *pd)
{
    char	buf[4096];

    sprintf (buf, "histogram function:  %s", f);
    DXSetError (ERROR_BAD_PARAMETER, "#11838", buf);
    return (ERROR);
}


static Error
ComputeCPD (int nbin, float *del, float *cum, int invert)
{
    Error	ret	= ERROR;
    float	val;
    float	*src;
    float	*dst;
    float	sum;
    int		i;

    /*
     * Normalize the counts to get the actual probability distribution.
     */

    sum = 0.0;
    for (i = 0, src = del; i < nbin; i++)
    {
	val  = *src++;
	if (val < (float) 0.0)
	{
	    DXSetError (ERROR_BAD_PARAMETER, "#10532", 
		        invert ? "ohist" : "ihist");
	    goto cleanup;
	}
	sum += val;
    }

    /*
     * If they want an flat (null) histogram then so be it.
     */

    if (sum == (float) 0.0)
    {
	sum = 1.0;
    }
    sum = (float) 1.0 / sum;
    for (i = 0, src = del; i < nbin; i++)
	*src++ *= sum;

    /*
     * Compute the normalized cummulative probability distribution.
     */
    
    sum = (float) 0.0;
    for (i = 0, src = del, dst = cum; i < nbin; i++)
    {
	*dst++ = sum;
	sum += *src++;
    }

    ret = OK;

cleanup:
    return (ret);
}


#define	IVAL()\
if (val > minv && val < maxv)\
{\
    bin   = (int) ((val - minv) * ideltab);\
    bdist = (val - ibase[bin]) * ideltab;\
    norm  = icpd[bin] + idpd[bin] * bdist;\
    val   = minv + norm * deltav;\
}

#define	ILOOP(_type)\
{\
    register _type	*s = (_type *) src;\
    register _type	*d = (_type *) dst;\
    for (i = 0; i < n; i++)\
    {\
	val = (float) *s++;\
	IVAL ();\
	*d++ = val;\
    }\
}

#define	ILOOPK(_type,_n,_d)\
{\
    register _type	*s = (_type *) src;\
    register _type	*d = (_type *) dst;\
    _type		lookup[_n];\
    for (i = 0; i < _n; i++)\
    {\
	val = (float) i + _d;\
	IVAL ();\
	lookup[i] = (_type) val;\
    }\
    for (i = 0; i < n; i++)\
	*d++ = lookup[(int) *s++];\
}

/*
 * 1. map from value to 0.0 - 1.0 via the input function
 * 2. map from normalized value to new normalized value via inverse of output function
 * 3. map from normalized value to minv - maxv range
 */


#define	IOVAL()\
if (val > minv && val < maxv)\
{\
    bin   = (int) ((val - minv) * ideltab);\
    bdist = (val - ibase[bin]) * ideltab;\
    norm  = icpd[bin] + idpd[bin] * bdist;\
\
    if (norm <= maxnorm)\
    {\
	for (bin = lbin = 0, hbin = nbin - 1; lbin < hbin; )\
	{\
	    bin = (lbin + hbin) >> 1;\
	    if (norm < ocpd[bin])\
	    {\
		hbin = bin;\
		continue;\
	    }\
	    if (norm <= ocpd[bin] + odpd[bin])\
		break;\
	    lbin = bin + 1;\
	}\
	bdist = (norm - ocpd[bin]) * obase[bin];\
	norm  = ((float) bin + bdist) * odeltab;\
    }\
\
    val   = minv + norm * deltav;\
}

#define	IOLOOP(_type)\
{\
    register _type	*s = (_type *) src;\
    register _type	*d = (_type *) dst;\
    for (i = 0; i < n; i++)\
    {\
	val = (float) *s++;\
	IOVAL ();\
	*d++ = val;\
    }\
}

#define	IOLOOPK(_type,_n,_d)\
{\
    register _type	*s = (_type *) src;\
    register _type	*d = (_type *) dst;\
    _type		lookup[_n];\
    for (i = 0; i < _n; i++)\
    {\
	val = (float) i + _d;\
	IOVAL ();\
	lookup[i] = (_type) val;\
    }\
    for (i = 0; i < n; i++)\
	*d++ = lookup[(int) *s++];\
}


static Error
Equalize (EQData *eq, Type type, int n, Pointer src, Pointer dst)
{
    Error		ret	= ERROR;
    register int	i;
    int			output	= eq->oto || eq->otf;
    register int	nbin	= eq->nbin;
    register float	minv	= eq->minv;
    register float	maxv	= eq->maxv;
    register float	val;
    register int	bin;
    register float	bdist;
    register float	norm;
    register float	deltav	= maxv - minv;
    register float	deltab	= deltav / nbin;
    register float	ideltab;
    register float	odeltab	= (float) 1.0 / nbin;
    float		*base	= NULL;
    register float	*ibase;
    register float	*obase;
    register float	*idpd;
    register float	*icpd;
    register float	*odpd;
    register float	*ocpd;
    float		*fp;
    int			size;

    if (deltab == (float) 0.0)
    {
	size = n * DXTypeSize (type);
	memcpy (dst, src, size);
	ret = OK;
	goto cleanup;
    }

    ideltab = (float) 1.0 / deltab;

    /*
     * If we are on the PVS, or any other NUMA architecture for that matter,
     * then we want to copy the things that we will be using a lot, such as
     * the probability distributions into local memory.  This is particularly
     * important on the PVS since without doing so we will totally abuse
     * the line cache.  If we don't succeed then too bad for the cache
     * but at least we'll run.
     */

#if DXD_HAS_LOCAL_MEMORY
    size  = nbin;
    size *= output ? 6 : 3;
    size *= sizeof (float);
    base  = (float  *) DXAllocateLocal (size);

    if (base)
    {
	ibase = base;
	idpd  = ibase + nbin;
	icpd  = idpd  + nbin;
	obase = icpd  + nbin;
	odpd  = obase + nbin;
	ocpd  = odpd  + nbin;

	size = nbin * sizeof (float);
	memcpy (idpd, eq->idpd, size);
	memcpy (icpd, eq->icpd, size);
	if (output)
	{
	    memcpy (odpd, eq->odpd, size);
	    memcpy (ocpd, eq->ocpd, size);
	}
    }
    else
#endif
    {
	DXResetError ();
	size  = nbin;
	size *= output ? 2 : 1;
	size *= sizeof (float);
	base = (float *) DXAllocate (size);
	if (! base)
	    goto cleanup;
	
	ibase = base;
	obase = ibase + nbin;

	idpd = eq->idpd;
	icpd = eq->icpd;
	odpd = eq->odpd;
	ocpd = eq->ocpd;
    }

    for (i = 0, fp = ibase; i < nbin; i++)
	*fp++ = minv + i * deltab;

    if (! output)
    {
	switch (type)
	{
	    case TYPE_BYTE:	ILOOPK (byte,256,-128);	break;
	    case TYPE_UBYTE:	ILOOPK (ubyte,256,0);	break;
	    case TYPE_USHORT:	ILOOP  (ushort);	break;
	    case TYPE_UINT:	ILOOP  (uint);		break;
	    case TYPE_SHORT:	ILOOP  (short);		break;
	    case TYPE_INT:	ILOOP  (int);		break;
	    case TYPE_FLOAT:	ILOOP  (float);		break;
	    case TYPE_DOUBLE:	ILOOP  (double);	break;
	    default:
		DXSetError (ERROR_BAD_PARAMETER, "#10320", "");
		goto cleanup;
	}
    }
    else
    {
	register int	lbin, hbin;
	register float	maxnorm		= ocpd[nbin - 1] + odpd[nbin - 1];

	for (i = 0, fp = obase; i < nbin; i++)
	{
	    *fp++ = odpd[i] == (float) 0.0
		    ? (float) 0.0 : (float) 1.0 / odpd[i];
	}

	switch (type)
	{
	    case TYPE_BYTE:	IOLOOPK (byte,256,-128);break;
	    case TYPE_UBYTE:	IOLOOPK (ubyte,256,0);	break;
	    case TYPE_USHORT:	IOLOOP  (ushort);	break;
	    case TYPE_UINT:	IOLOOP  (uint);		break;
	    case TYPE_SHORT:	IOLOOP  (short);	break;
	    case TYPE_INT:	IOLOOP  (int);		break;
	    case TYPE_FLOAT:	IOLOOP  (float);	break;
	    case TYPE_DOUBLE:	IOLOOP  (double);	break;
	    default:
		DXSetError (ERROR_BAD_PARAMETER, "#10320", "");
		goto cleanup;
	}
    }

    ret = OK;

cleanup:
    DXFree ((Pointer) base);
    return (ret);
}
