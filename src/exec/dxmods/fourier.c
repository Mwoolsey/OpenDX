/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dxmods/fourier.c,v 1.7 2006/01/03 17:02:22 davidt Exp $
 */


#include <dxconfig.h>

#if defined(HAVE_STRING_H)
#include <string.h>
#endif

#include <math.h>
#include "unpart.h"

typedef Error		(*PFE)();
extern Error DXAddLikeTasks(PFE, Pointer, int, double, int); /* from libdx/task.c */

#if defined(intelnt) || defined(WIN32)
#define  strcasecmp      stricmp
#endif

#define	XF_COMPONENT	"XF"
#define	XF_TYPE		TYPE_FLOAT
#define	XF_CATEGORY	CATEGORY_COMPLEX
typedef	float		xf_type;

typedef struct
{
    xf_type	r;
    xf_type	i;
} complex_t;

typedef struct
{
    int		inverse;
    int		center;
    int		dft;
} XFArgs;

typedef struct 
{
    Object	cf;
    FieldInfo	*info;
    FieldInfo	*global;
    FieldInfo	*src;
    FieldInfo	*dst;
    int		element;
} WorkerData;

typedef struct
{
    XFArgs	args;
    FieldInfo	*info;
    int		procs;
    int		dim;
} XFData;

#define	ALLOCATE_LOCAL(_v,_t,_n)\
{\
    _v = (_t *) DXAllocateLocal (_n);\
    if (! _v)\
    {\
	DXResetError ();\
	_v = (_t *) DXAllocate (_n);\
	if (! _v)\
	    goto cleanup;\
    }\
}


/*
 * $$$$$ REMOVE THIS AT SOME POINT WHEN UNPART CAN DEAL WITH IT
 */

#define	THREE_DIMENSION_HACK(_op)\
{\
    int		i;\
    if (info _op ndims > 3)\
    {\
	DXSetError (ERROR_BAD_PARAMETER, "#11390", "input", 3);\
	goto cleanup;\
    }\
    for (i = info _op ndims; i < 3; i++)\
    {\
	info _op origin[i] = 0;\
	info _op counts[i] = 1;\
    }\
    info _op ndims = 3;\
}


static Error TransformEntry  (Object *in, Object *out, int dft);
static Error TransformObject (Object o, XFArgs *xfa);
static Error TransformField  (Object f, XFArgs *xfa);
static Error TransformCF     (Object f, XFArgs *xfa);
static Error TransformGroup  (Object g, XFArgs *xfa);

static Error TransformAggregate (FieldInfo *f, XFArgs *xfa);

static Error W1 (WorkerData *arg, int id);
static Error W2 (WorkerData *arg, int id);
static Error W3 (WorkerData *arg, int id);
static Error W4 (WorkerData *arg, int id);

static Error W5 (XFData *arg, int id);

static void fft_trig (/*t, logn, inv*/);
#if 0
    register complex_t	*t;
    register int	logn;
    register int	inv;
#endif
static void fft_brev (/*b, logn*/);
#if 0
    register int	*b;
    register int	logn;
#endif
static void fft (/*c, logn, trig, brev*/);
#if 0
    register complex_t	*c;
    register int	logn;
    register complex_t	*trig;
    register int	*brev;
#endif
static void fft_norm (/*c, logn*/);
#if 0
    register complex_t	*c;
    register int	logn;
#endif
static void dft (/*n, from, to, inv*/);
#if 0
    int			n;
    register complex_t	*from;
    register complex_t	*to;
    int			inv;
#endif

Error
m_DFT (Object *in, Object *out)
{
    return (TransformEntry (in, out, TRUE));
}

Error
m_FFT (Object *in, Object *out)
{
    return (TransformEntry (in, out, FALSE));
}

static Error
TransformEntry (Object *in, Object *out, int dft)
{
    Error	ret	= ERROR;
    Object	copy	= NULL;
    int		inverse	= FALSE;
    int		center	= FALSE;
    XFArgs	xfa;
    Class	class;
    char	*cp;

    if (! in[0])
    {
	DXSetError (ERROR_BAD_PARAMETER, "#10000", "input");
	goto cleanup;
    }

    class = DXGetObjectClass (in[0]);
    if (class != CLASS_FIELD &&
	class != CLASS_GROUP &&
	class != CLASS_XFORM &&
	class != CLASS_CLIPPED)
    {
	DXSetError (ERROR_BAD_PARAMETER, "input");
	goto cleanup;
    }

    copy = DXCopy (in[0], COPY_STRUCTURE);
    if (! copy)
	goto cleanup;

    if (in[1])
    {
	if (DXExtractInteger (in[1], &inverse))
	{
	    if (inverse < 0)
		inverse = TRUE;
	    else if (inverse > 0)
		inverse = FALSE;
	    else
	    {
		DXSetError (ERROR_BAD_PARAMETER, "inverse:  must be non-zero");
		goto cleanup;
	    }
	}
	else if (DXExtractString (in[1], &cp))
	{
	    if (! strcasecmp (cp, "forward"))
		inverse = FALSE;
	    else if (! strcasecmp (cp, "backward"))
		inverse = TRUE;
	    else if (! strcasecmp (cp, "inverse"))
		inverse = TRUE;
	    else
	    {
		DXSetError (ERROR_BAD_PARAMETER,
		      "inverse:  must be either forward, backward, or inverse");
		goto cleanup;
	    }
	}
	else
	{
	    DXSetError (ERROR_BAD_PARAMETER, "inverse");
	    goto cleanup;
	}
    }

    if (in[2])
    {
	if (! DXExtractInteger (in[2], &center))
	{
	    DXSetError (ERROR_BAD_PARAMETER, "invalid center");
	    goto cleanup;
	}
    }

    xfa.inverse = inverse;
    xfa.center  = center;
    xfa.dft	= dft;
    if (! TransformObject (copy, &xfa))
	goto cleanup;

    ret = OK;

cleanup:
    if (ret == ERROR)
	DXDelete (copy);
    else
	out[0] = copy;
    
    return (ret);
}


static Error
TransformObject (Object o, XFArgs *xfa)
{
    Object	kid;

    switch (DXGetObjectClass (o))
    {
	case CLASS_FIELD:
	    return (TransformField (o, xfa));

	case CLASS_GROUP:
	    if (DXGetGroupClass ((Group) o) == CLASS_COMPOSITEFIELD)
		return (TransformCF (o, xfa));
	    else
		return (TransformGroup (o, xfa));

	case CLASS_XFORM:
	    if (! DXGetXformInfo ((Xform) o, &kid, NULL))
		return (ERROR);
	    return (TransformObject (kid, xfa));

	case CLASS_CLIPPED:
	    if (! DXGetClippedInfo ((Clipped) o, &kid, NULL))
		return (ERROR);
	    return (TransformObject (kid, xfa));

	default:
	    DXSetError (ERROR_BAD_PARAMETER, "invalid input");
	    return (ERROR);
    }
}


static Error
TransformGroup (Object g, XFArgs *xfa)
{
    int		i	= 0;
    Object	o;
    Type	type;
    Category	cat;
    int		rank, shape[MAXDIMS];

    while ((o = DXGetEnumeratedMember ((Group) g, i++, NULL)) != NULL)
	if (! TransformObject (o, xfa))
	    return (ERROR);
    if (! DXGetType ((Object) g, &type, &cat, &rank, shape))
    {
	DXSetError (ERROR_INTERNAL, "missing type information");
	    return (ERROR);
    }
    if (! DXSetGroupTypeV ((Group) g, XF_TYPE, XF_CATEGORY, rank, shape))
	return (ERROR);

    return (OK);
}


static Error
TransformField (Object f, XFArgs *xfa)
{
    FieldInfo	*fis	= NULL;
    FieldInfo	*info;
    FieldInfo	*global;
    FieldInfo	*new;
    Array	space	= NULL;
    Array	data	= NULL;
    Error	ret	= ERROR;
    int		i;
    int		items;
    int		size;
    Type	type;
    Category	cat;
    int		rank;
    int		shape[MAXDIMS];

    if (DXEmptyField ((Field) f))
    {
	ret = OK;
	goto cleanup;
    }

    fis = (FieldInfo *) DXAllocate (3 * sizeof (FieldInfo));
    if (! fis)
	goto cleanup;
    info   = fis;
    global = fis + 1;
    new    = fis + 2;

    info->obj = f;
    if (! _dxfGetFieldInformation (info))
	goto cleanup;
    
    THREE_DIMENSION_HACK(->);

    /*
     * Create the appropriate global space for the operation to be
     * performed.
     */

    for (i = 0, size = 1; i < info->ndims; i++)
	size *= info->counts[i];

    space = DXNewArrayV (XF_TYPE, XF_CATEGORY, 0, NULL);
    if (! space)
	goto cleanup;
    if (! DXAddArrayData (space, 0, size, NULL))
	goto cleanup;

    *global      = *info;
    global->type = XF_TYPE;
    global->cat  = XF_CATEGORY;
    global->epi  = 1;
    global->data = space;

    /*
     * Create the new data array and put it into the field as the
     * transform component
     */
    
    if (! DXGetArrayInfo (info->data, &items, &type, &cat, &rank, shape))
	goto cleanup;
    data = DXNewArrayV (XF_TYPE, XF_CATEGORY, rank, shape);
    if (! data)
	goto cleanup;
    if (! DXAddArrayData (data, 0, items, NULL))
	goto cleanup;
    if (! DXCopyAttributes ((Object) data, (Object) info->data))
	goto cleanup;
    if (! DXSetComponentValue ((Field) f, XF_COMPONENT, (Object) data))
	goto cleanup;

    *new      = *info;
    new->type = XF_TYPE;
    new->cat  = XF_CATEGORY;
    new->data = data;

    data = NULL;

    /*
     * OK, now collect up the ith element, do the operation, and put it
     * back.
     */

    for (i = 0; i < info->epi; i++)
    {
	if (! _dxfCoalesceFieldElement (info, global, i, 0, 1))
	    goto cleanup;

	if (! TransformAggregate (global, xfa))
	    goto cleanup;

	if (! _dxfExtractFieldElement (global, new, i, 0, 1))
	    goto cleanup;
    }

    if (! DXRename (f, XF_COMPONENT, "data"))
	goto cleanup;
    if (! DXChangedComponentValues ((Field) f, "data"))
	goto cleanup;
    ret = OK;

cleanup:
    DXFree ((Pointer) fis);
    DXDelete ((Object) space);
    DXDelete ((Object) data);
    return (ret);
}


#define	TASK_GROUP(_f,_d,_n)\
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
}


static Error
TransformCF (Object f, XFArgs *xfa)
{
    FieldInfo	*info	= NULL;
    FieldInfo	*global	= NULL;
    FieldInfo	*src	= NULL;
    FieldInfo	*dst	= NULL;
    Array	space	= NULL;
    Error	ret	= ERROR;
    int		i;
    int		size;
    WorkerData	data;
    Type	type;
    Category	cat;
    int		rank;
    int		shape[MAXDIMS];

    /*
     * Find out about the composite field.
     */

    info = (FieldInfo *) DXAllocate (sizeof (FieldInfo));
    if (! info)
	goto cleanup;
    info->obj = f;
    if (! _dxfGetFieldInformation (info))
	goto cleanup;
    
    THREE_DIMENSION_HACK (->);

    /*
     * Construct the field information descriptors for the global space
     * that we are going to use for coalescing the data together.
     */

    size = sizeof (FieldInfo);
    global = (FieldInfo *) DXAllocate (size);
    if (! global)
	goto cleanup;
    memset (global, 0, size);

    size *= info->members;
    src = (FieldInfo *) DXAllocate (size);
    dst = (FieldInfo *) DXAllocate (size);
    if (! src || ! dst)
	goto cleanup;
    memset (src, 0, size);
    memset (dst, 0, size);

    /*
     * Create the appropriate global space for the operation to be
     * performed.
     */

    for (i = 0, size = 1; i < info->ndims; i++)
	size *= info->counts[i];

    space = DXNewArrayV (XF_TYPE, XF_CATEGORY, 0, NULL);
    if (! space)
	goto cleanup;
    if (! DXAddArrayData (space, 0, size, NULL))
	goto cleanup;

    *global         = *info;
    global->type    = XF_TYPE;
    global->cat     = XF_CATEGORY;
    global->epi     = 1;
    global->data    = space;

    /*
     * Create the appropriate new spaces for return.
     */

    data.cf     = f;
    data.info   = info;
    data.global = global;
    data.src    = src;
    data.dst    = dst;

    TASK_GROUP (W1, data, info->members);

    /*
     * OK, now collect up the ith element, do the operation, and put it
     * back.
     */

    for (i = 0; i < info->epi; i++)
    {
	data.element = i;
	TASK_GROUP (W2, data, info->members);

	if (! TransformAggregate (global, xfa))
	    goto cleanup;

	TASK_GROUP (W3, data, info->members);
    }

    TASK_GROUP (W4, data, info->members);

    if (! DXGetType (f, &type, &cat, &rank, shape))
    {
	DXSetError (ERROR_INTERNAL, "missing type information");
	goto cleanup;
    }
    if (! DXSetGroupTypeV ((Group) f, XF_TYPE, XF_CATEGORY, rank, shape))
	goto cleanup;

    ret = OK;

cleanup:
    DXFree ((Pointer) info);
    DXFree ((Pointer) global);
    DXFree ((Pointer) src);
    DXFree ((Pointer) dst);
    DXDelete ((Object) space);

    return (ret);
}


static Error
W1 (WorkerData *arg, int id)
{
    Field		f;
    FieldInfo		*info;
    FieldInfo		*dinfo;
    int			items;
    Type		type;
    Category		cat;
    int			rank;
    int			shape[MAXDIMS];
    Array		fdata;
    Array		data	= NULL;
    Error		ret	= ERROR;

    f = (Field) DXGetEnumeratedMember ((Group) arg->cf, id, NULL);
    if (! f)
	goto cleanup;

    if (DXEmptyField ((Field) f))
    {
	ret = OK;
	goto cleanup;
    }

    info  = arg->src + id;
    dinfo = arg->dst + id;

    info->obj = (Object) f;
    if (! _dxfGetFieldInformation (info))
	goto cleanup;
    fdata = info->data;

    THREE_DIMENSION_HACK (->);

    /*
     * Create the new data array and put it into the field as the
     * transform component
     */
    
    if (! DXGetArrayInfo (fdata, &items, &type, &cat, &rank, shape))
	goto cleanup;
    data = DXNewArrayV (XF_TYPE, XF_CATEGORY, rank, shape);
    if (! data)
	goto cleanup;
    if (! DXAddArrayData (data, 0, items, NULL))
	goto cleanup;
    if (! DXCopyAttributes ((Object) data, (Object) fdata))
	goto cleanup;
    if (! DXSetComponentValue (f, XF_COMPONENT, (Object) data))
	goto cleanup;

    *dinfo       = *info;
    dinfo->type  = XF_TYPE;
    dinfo->cat   = XF_CATEGORY;
    dinfo->data  = data;

    data = NULL;
    ret = OK;

cleanup:
    if (data)
	DXDelete ((Object) data);
    return (ret);
}


static Error
W2 (WorkerData *arg, int id)
{
    WorkerData	larg;
    FieldInfo	*info;
    Error	ret;

    larg = *arg;
    info = larg.src + id;
    if (info->data == NULL)
	return (OK);
    ret  = _dxfCoalesceFieldElement (info, larg.global, larg.element, 0, 1);
    return (ret);
}


static Error
W3 (WorkerData *arg, int id)
{
    WorkerData	larg;
    FieldInfo	*info;
    Error	ret;

    larg = *arg;
    info = larg.dst + id;
    if (info->data == NULL)
	return (OK);
    ret = _dxfExtractFieldElement (larg.global, info, larg.element, 0, 1);
    return (ret);
}


static Error
W4 (WorkerData *arg, int id)
{
    WorkerData	larg;
    FieldInfo	*info;
    Field	f;

    larg = *arg;
    info = larg.dst + id;
    if (info->data == NULL)
	return (OK);
    f    = (Field) info->obj;
    if (! DXRename ((Object) f, XF_COMPONENT, "data"))
	return (ERROR);
    if (! DXChangedComponentValues (f, "data"))
	return (ERROR);
    return (OK);
}


static Error
TransformAggregate (FieldInfo *f, XFArgs *xfa)
{
    XFData		data;
    int			dims;
    int			i;
    Error		ret	= ERROR;

    data.args  = *xfa;
    data.info  = f;
    data.procs = DXProcessors (0);

    dims = f->ndims;

    for (i = 0; i < dims; i++)
    {
	data.dim = i;
	TASK_GROUP (W5, data, data.procs);
    }

    ret = OK;

cleanup:
    return (ret);
}


static int
Log2N (int n)
{
    int		i;
    int		mask	= 1;
    int		cnt	= 0;
    int		log	= 0;

    for (i = 0; i < 32; i++, mask <<= 1)
    {
	if (n & mask)
	{
	    log = i;
	    cnt++;
	}
    }

    return (cnt == 1 ? log : -1);
}


#define	APPLY_CENTERING \
{ \
    p1 = data; \
    inv = (row + col) & 1; \
    for (j = 0; j < s; j++) \
    { \
	if (inv) \
	{ \
	    *p1++ *= (xf_type) -1; \
	    *p1++ *= (xf_type) -1; \
	} \
	else \
	    p1 += csize; \
\
	inv = ! inv; \
    } \
}


static Error
W5 (XFData *arg, int id)
{
    XFData		larg;
    unsigned int	s, ds;
    unsigned int	r, dr, row;
    unsigned int	c, dc, col;
    unsigned int	n;
    unsigned int	i, j;
    int			csize;
    int			inv;
    unsigned int	delta[MAXDIMS];
    xf_type		*data	= NULL;
    xf_type		*trig	= NULL;
    int			*brev	= NULL;
    xf_type		*raw;
    xf_type		*p0;
    xf_type		*p1;
    int			dims;
    unsigned int	size;
    int			logs;
    Error		ret	= ERROR;

    larg = *arg;

    csize = DXCategorySize (larg.info->cat);
    dims  = larg.info->ndims;
    n = (unsigned int) (csize);
    for (i = dims; i--; )
    {
	delta[i] = n;
	n *= (unsigned int) larg.info->counts[i];
    }

    switch (larg.dim)
    {
	case 0:
	    ds = delta[0]; s = (unsigned int) larg.info->counts[0];
	    dr = delta[1]; r = (unsigned int) larg.info->counts[1];
	    dc = delta[2]; c = (unsigned int) larg.info->counts[2];
	    break;
	case 1:
	    dr = delta[0]; r = (unsigned int) larg.info->counts[0];
	    ds = delta[1]; s = (unsigned int) larg.info->counts[1];
	    dc = delta[2]; c = (unsigned int) larg.info->counts[2];
	    break;
	case 2:
	    dr = delta[0]; r = (unsigned int) larg.info->counts[0];
	    dc = delta[1]; c = (unsigned int) larg.info->counts[1];
	    ds = delta[2]; s = (unsigned int) larg.info->counts[2];
	    break;
	default:
	    DXSetError (ERROR_BAD_PARAMETER, "unsupported dimensionality");
	    goto cleanup;
    }

    /*
     * A quick out.  If the vector only has 1 element then we don't need to
     * do anything other than apply centering to it if necessary.
     */

    if (s == 1)
    {
	if (! larg.args.center)
	    goto ok;
	if (  larg.args.inverse && larg.dim != 2)
	    goto ok;
	if (! larg.args.inverse && larg.dim != 0)
	    goto ok;
    }

    /*
     * The checks for logs == -1 have to do with determining whether to
     * do an FFT or a DFT.  In the case of an FFT the result is returned
     * in place.  For a DFT the result is returned in the trig array
     * since it is not precomputed due to it's O(n^2) size.  This is
     * important for the replace a vector step.
     */

    logs = Log2N (s);
    if (logs == -1 && ! larg.args.dft)
    {
	DXSetError (ERROR_BAD_PARAMETER,
		  "dimension %d is not an integral power of 2", larg.dim);
	goto cleanup;
    }

    n = r * c;

    size = s * csize * sizeof (xf_type);
    ALLOCATE_LOCAL (data, xf_type, size);
    ALLOCATE_LOCAL (trig, xf_type, size);

    if (logs != -1)
    {
	size = s * sizeof (int);
	ALLOCATE_LOCAL (brev, int,     size);
    }

    raw = (xf_type *) DXGetArrayData (larg.info->data);
    if (! raw)
	goto cleanup;

    if (logs != -1)
    {
	fft_brev (brev, logs);
	fft_trig (trig, logs, larg.args.inverse);
    }

    for (i = id; i < n; i += larg.procs)
    {
	row = i / c;
	col = i - (row * c);

	/*
	 * DXExtract a vector
	 */

	p0  = raw + row * dr + col * dc;
	p1  = data;
	for (j = 0; j < s; j++)
	{
	    *(p1    ) = *(p0    );
	    *(p1 + 1) = *(p0 + 1);
	    p0 += ds;
	    p1 += csize;
	}

	/*
	 * Operate on it.
	 * If we are performing the centering operation it happens BEFORE
	 * a forward transform, and AFTER and inverse.
	 */
	
	if (larg.dim == 0 && larg.args.center && ! larg.args.inverse)
	    APPLY_CENTERING;

	if (logs != -1)
	{
	    fft (data, logs, trig, brev);
	    if (larg.args.inverse)
		fft_norm (data, logs);
	}
	else
	{
	    dft (s, data, trig, larg.args.inverse);
	}

	if (larg.dim == 2 && larg.args.center && larg.args.inverse)
	    APPLY_CENTERING;

	/*
	 * DXReplace it
	 */

	p0  = raw + row * dr + col * dc;
	p1  = logs != -1 ? data : trig;
	for (j = 0; j < s; j++)
	{
	    *(p0    ) = *(p1    );
	    *(p0 + 1) = *(p1 + 1);
	    p0 += ds;
	    p1 += csize;
	}
    }

ok:
    ret = OK;

cleanup:
    DXFree ((Pointer) data);
    DXFree ((Pointer) trig);
    DXFree ((Pointer) brev);
    return (ret);
}


/****************************************************************************/
/*
 * fft_trig
 *
 *   Precomputes a table of all the trigonometry required by fft() for
 *   either a forward or reverse transform.
 *
 *   The data is stored in the user-supplied buffer, t, which must contain
 *   room for (2 ^ logn) complex numbers.
 *
 *   The flag, inv, should be nonzero to set up the trig for an inverse
 *   transform, zero otherwise.
 */

static void
fft_trig (t, logn, inv)
    register complex_t	*t;
    register int	logn;
    register int	inv;
{
    register int	le, n;
    register xf_type	ang;

    n	= 1 << logn;
    ang	= inv ? (xf_type) M_PI : (xf_type) -M_PI;

    for (le = 1; le < n; le <<= 1)
    {
	register xf_type	ur, ui, wr, wi;
	register complex_t     *p, *pe;

	ur = (xf_type) 1.0;
	ui = (xf_type) 0.0;
	wr = (xf_type) cos (ang);
	wi = (xf_type) sin (ang);
	p  = t + le - 1;
	pe = p + le;

	while (p < pe)
	{
	    register xf_type	t;

	    p->r = ur;
	    p->i = ui;
	    t	 = ur;
	    ur	 = t * wr - ui * wi;
	    ui	 = t * wi + ui * wr;
	    p++;
	}

	ang *= (xf_type) 0.5;
    }
}

/*
 * fft_brev
 *
 *   Precomputes a table of all the bit reversals that can be used by fft(),
 *   independent of whether the transform is forward or reverse.
 *
 *   The data is stored in the user-supplied buffer, b, which must contain
 *   room for (2 ^ logn) integers.
 *
 *   Notes:  There are always a few less than n / 2 bit reversals, but we
 *	     require a from and to for each, plus a zero at the end.
 *
 *           Indices are multiplied by sizeof (complex_t) so it's not
 *           necessary later.
 */

static void
fft_brev (b, logn)
    register int	*b;
    register int	logn;
{
    register int	n, i, j, k;

    n	= 1 << logn;
    j	= 0;

    for (i = 0; i < n - 1; i++)
    {
	if (i < j)
	{
	    *b++ = i * sizeof (complex_t);	/* From */
	    *b++ = j * sizeof (complex_t);	/* To	*/
	}

	for (k = n >> 1; j >= k; k >>= 1)
	    j -= k;

	j += k;
    }

    *b++	= 0;	/* Mark end of table */
}

/*
 * fft
 *
 *   Computes an FFT on the input array, c, containing 2 ^ logn complex
 *   numbers, leaving the output in the same array.
 *
 *   A trigonometric table, as computed by fft_trig, is required on input.
 *   The table determines the FFT direction, forward or reverse.
 *
 *   A bit reversal table, as computed by fft_brev, is used on input.
 */

static void
fft (c, logn, trig, brev)
    register complex_t	*c;
    register int	logn;
    register complex_t	*trig;
    register int	*brev;
{
    register int	n, le;
    register int	i;

    n	= 1 << logn;

    while ((i = *brev++) != 0)
    {
	register xf_type	tr, ti;
	register complex_t  *pi, *pj;

	pi	= (complex_t *) ((char *) c + i	   );
	pj	= (complex_t *) ((char *) c + *brev++);

	tr	  = pi->r;
	ti	  = pi->i;
	pi->r = pj->r;
	pi->i = pj->i;
	pj->r = tr;
	pj->i = ti;
    }

    le	= 1;

    while (le < n)
    {
	register complex_t     *p, *q, *qe;
	register int		le1;

	p   = trig + le - 1;
	qe  = c + le;
	le1 = le;

	le <<= 1;

	for (q = c; q < qe; q++)
	{
	    register xf_type	ur, ui;
	    register complex_t	*p1, *p2, *p1e;

	    ur	= p->r;
	    ui	= p->i;

	    p++;

	    p1	= q;
	    p1e	= p1 + n;
	    p2	= p1 + le1;

	    while (p1 < p1e)
	    {
		register xf_type	r1, r2, i1, i2, tr, ti;

		r2 = p2->r;
		i2 = p2->i;
		r1 = p1->r;
		i1 = p1->i;
		tr = r2 * ur - i2 * ui;
		ti = r2 * ui + i2 * ur;

		p2->r = r1 - tr;
		p2->i = i1 - ti;
		p2   += le;

		p1->r = r1 + tr;
		p1->i = i1 + ti;
		p1   += le;
	    }
	}
    }
}


static void
fft_norm (c, logn)
    register complex_t	*c;
    register int	logn;
{
    register int	n;
    register xf_type	nr;
    register complex_t	*ce;

    n  = 1 << logn;
    nr = (xf_type) 1.0 / n;
    ce = c + n;

    while (c < ce)
    {
	c->r *= nr;
	c->i *= nr;
	c++;
    }
}


/****************************************************************************/

/*
 * Since there are O(n^2) sin/cos coefficients we do not precompute
 * them for use here as we did with the FFT.
 */

static void
dft (n, from, to, inv)
    int			n;
    register complex_t	*from;
    register complex_t	*to;
    int			inv;
{
    int			u;
    int			x;
    register xf_type	tor;
    register xf_type	toi;
    register xf_type	fromr;
    register xf_type	fromi;
    register xf_type	cval;
    register xf_type	sval;
    register xf_type	one_n;
    register xf_type	twopi_n;
    register xf_type	twopiu_n;
    register double	twopiux_n;
    complex_t		*fsave;
    complex_t		*tsave;

    fsave = from;
    tsave = to;

    one_n    = (xf_type) 1.0 / n;
    twopi_n  = (xf_type) -2.0 * M_PI * one_n;
    twopi_n *= (xf_type) inv ? -1.0 : 1.0;

    for (u = 0; u < n; u++, to++)
    {
	twopiu_n = twopi_n * u;
	tor = (xf_type) 0.0;
	toi = (xf_type) 0.0;

	for (x = 0, from = fsave; x < n; x++, from++)
	{
	    twopiux_n = (double) twopiu_n * x;
	    cval      = (xf_type) cos (twopiux_n);
	    sval      = (xf_type) sin (twopiux_n);
	    fromr     = from->r;
	    fromi     = from->i;
	    tor      += (fromr * cval - fromi * sval);
	    toi      += (fromr * sval + fromi * cval); 
        }

	to->r = tor;
	to->i = toi;
    }

    if (inv)
    {
	for (u = 0, to = tsave; u < n; u++, to++)
	{
	    to->r *= one_n;
	    to->i *= one_n;
	}
    }
}
