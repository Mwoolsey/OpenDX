/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#define	GROW_BUG_IN_REF		0

#include <ctype.h>
#include <math.h>
#include <string.h>
#include <dx/dx.h>

typedef Error			(*PFE)();
extern Error DXAddLikeTasks(PFE, Pointer, int, double, int); /* from libdx/task.c */

#if defined(intelnt) || defined(WIN32)
#define	strcasecmp	stricmp
#endif

#define	DEFAULT_FILTER		"gaussian"
#define	DEFAULT_MASK		"box"
#define	DEFAULT_COMPONENT	"data"
#define	DEFAULT_MORPH		"erode"

#define	FILTER_MEDIAN		"median"
#define	FILTER_MIN		"min"
#define	FILTER_MAX		"max"

#ifndef	MAX
#define	MAX(a,b)		((a) >= (b) ? (a) : (b))
#define	MIN(a,b)		((a) <= (b) ? (a) : (b))
#endif

#ifndef	TRUE
#define	TRUE			1
#define	FALSE			0
#endif

#ifndef	MAXDIMS
#define	MAXDIMS			8
#endif

#define	LOCAL_SORT		128		/* enough for a 5x5x5	*/

typedef enum
{
    DEP_OTHER,
    DEP_POSITIONS,
    DEP_CONNECTIONS
} DepOn;

typedef struct
{
    float		c;	/* ubyte(char) 	8-bits	unsigned	*/
    float		s;	/* short	16-bits	signed		*/
    float		i;	/* integer	32-bits signed		*/
    float		f;	/* float	32-bits			*/
    float		d;	/* double				*/
    float		b;	/* byte		 8-bits signed		*/
    float		us;	/* ushort	16-bits unsigned	*/
    float		ui;	/* uint		32-bits unsigned	*/
} Uvalue;


typedef union
{
    ubyte		*c;
    short		*s;
    int			*i;
    float		*f;
    double		*d;
    char		*b;
    ushort		*us;
    uint		*ui;
    Pointer		p;
} Upointer;


typedef struct
{
    unsigned int	from;		/* start of iteration		*/
    unsigned int	to;		/* end   of iteration		*/
    unsigned int	delta;		/* values to skip over		*/
    unsigned int	incr;		/* delta in bytes		*/
} Steps;


typedef enum
{
    FT_CONVOLVE,			/* convolution filter		*/
    FT_RANKVALUE,			/* rank value - value specified	*/
    FT_RVMEDIAN,			/* rank value - compute median	*/
    FT_RVMAX,				/* rank value - use max		*/
    FT_ERODE,				/* morphological erosion	*/
    FT_DILATE				/* morphological dilation	*/
} FilterType;

typedef struct
{
    char	*name;			/* name of the filter		*/
    FilterType	type;			/* type of operation		*/
    float	rvalue;			/* offset if rank-value filter	*/
    int		rank;			/* rank of the filter		*/
    int		maxdim;
    int		count;			/* total number of values	*/
    float	*vals;			/* the filter itself		*/
    int		shape[MAXDIMS];		/* shape of the filter		*/
    int		delta[MAXDIMS];		/* delta in each dimension	*/
} Filter;


typedef struct
{
    char	*name;			/* name of the filter		*/
    int		rank;			/* rank of the filter		*/
    int		*shape;			/* shape of the filter		*/
    float	*vals;			/* the filter itself		*/
} FilterTable;


typedef struct
{
    Object	obj;			/* Object to operate on		*/
    Class	class;			/* It's class			*/
    Filter	*filter;		/* filter to use		*/
    char	*component;		/* component to filter		*/
} FilterTask;


typedef struct
{
    FilterType		ftype;
    float		rvalue;
    int			rank;
    Type		type;
    unsigned int	size;
    Upointer		src;
    Upointer		dst;
    Steps		*srcb;
    Steps		*dstb;
    int			fsize;		/* number of filter elements	*/
    int			*offsets;	/* offsets within a field	*/
    float		*filter;	/* local copy of filter vals	*/
} IrregularData;


/*
 * All of our helper functions.
 */

static Error	m_FilterWorker (Object *in, Object *out, int morph);

static Error	_CheckComponent		(Object obj, char **component);
static Error	_CheckFilter		(Object obj, Object mask,
					 Filter **filterp, int morph);
static Error	_CheckInput		(Object obj);
static void 	_DestroyFilter		(Filter *f);

static Error	_Filter_Object		(Object obj, Filter *filter,
					 char *component, int top);
static Error	_Filter_Group		(Group obj, Filter *filter,
					 char *component);
static Error	_Filter_CompositeField	(CompositeField obj, Filter *filter,
					 char *component);
static Error	_Filter_Field		(Field obj, Filter *filter,
					 char *component);

static Array 	_Filter_Array 		(Field obj, Filter *filter,
					 Array ga, Array oa, DepOn d,
					 int *olength, int *offsets);

static Error	_Work_G			(FilterTask *ft, int n);

static Error	_SetFieldDims		(Array a, int *rank, int *dims,
					 DepOn d);
static Error	_SetFilterData		(Filter *f, int rank);
static Filter	*_CopyFilter		(Filter *old);

static void	_Apply_Irregular	(IrregularData *data);
static Error	_Apply_3x3		(IrregularData *data);


/*
 * Dimension arrays for the filters.
 */

static int	_3[]		= {      3};
static int	_3x3[]		= {   3, 3};
static int	_5x5[]		= {   5, 5};
static int	_7x7[]		= {   7, 7};
static int	_3x3x3[]	= {3, 3, 3};

#if 0
    float		c;	/* ubyte(char) 	8-bits	unsigned	*/
    float		s;	/* short	16-bits	signed		*/
    float		i;	/* integer	32-bits signed		*/
    float		f;	/* float	32-bits			*/
    float		d;	/* double				*/
    float		b;	/* byte		 8-bits signed		*/
    float		us;	/* ushort	16-bits unsigned	*/
    float		ui;	/* uint		32-bits unsigned	*/
#endif

/*
	 XXX WARNING: Compiler Bug fix here!! DXD_MIN_INT which is the correct
	min int value for i860 gets changed to ABS(MIN_INT)+1 when assigning
	it to float. Also, the obvious (DXD_MIN_INT + 1) doesn't work. We need
	to assign the actual vaule of (DXD_MIN_INT + 1).

	XXX WARNING 2: neither the 6000s or SGI can handle
	float f = DXD_MIN_INT!    Make it -2147483647 in all cases
*/
#if 0

#ifdef ibmpvs
#define DXD_MIN_INT_AS_FLOAT -2147483647
#else
#define DXD_MIN_INT_AS_FLOAT DXD_MIN_INT
#endif

#else

#define DXD_MIN_INT_AS_FLOAT -2147483647

#endif

static Uvalue	_minclamp	=
{
    0.0,
    DXD_MIN_SHORT,
    DXD_MIN_INT_AS_FLOAT,
    0.0,
    0.0,
    DXD_MIN_BYTE,
    0.0,
    0.0
};


static Uvalue	_maxclamp	=
{
    DXD_MAX_UBYTE,
    DXD_MAX_SHORT,
    DXD_MAX_INT,
    0.0,
    0.0,
    DXD_MAX_BYTE,
    DXD_MAX_USHORT,
    DXD_MAX_UINT
};

/*
 * The filter kernels themselves.
 *
 * NOTE:  1)  If you copy a filter kernel from a book that is not symmetric
 *		about its vertical axis (e.g [ xy | yx ]) you will need to
 *		use its mirror image here (e.g given [abc] use [cba]).
 */

#define	I_3	(1.0 / 3.0)
#define	I_5	(1.0 / 5.0)
#define	I_7	(1.0 / 7.0)
#define	I_9	(1.0 / 9.0)
#define	I_27	(1.0 / 27.0)

static float f_box_3[]=
{
    I_3, I_3, I_3
};

static float f_4_connected[] =
{
    0.0, I_5, 0.0,
    I_5, I_5, I_5,
    0.0, I_5, 0.0
};

static float f_box_3x3[] =
{
    I_9, I_9, I_9,
    I_9, I_9, I_9,
    I_9, I_9, I_9
};

static float f_6_connected[] =
{
    0.0, 0.0, 0.0, 0.0, I_7, 0.0, 0.0, 0.0, 0.0,
    0.0, I_7, 0.0, I_7, I_7, I_7, 0.0, I_7, 0.0,
    0.0, 0.0, 0.0, 0.0, I_7, 0.0, 0.0, 0.0, 0.0
};

static float f_box_3x3x3[] =
{
    I_27, I_27, I_27, I_27, I_27, I_27, I_27, I_27, I_27,
    I_27, I_27, I_27, I_27, I_27, I_27, I_27, I_27, I_27,
    I_27, I_27, I_27, I_27, I_27, I_27, I_27, I_27, I_27
};

static float f_compass_n[] =
{
     1.0,  1.0,  1.0,
     1.0, -2.0,  1.0,
    -1.0, -1.0, -1.0
};

static float f_compass_ne[] =
{
     1.0,  1.0,  0.0,
     1.0, -2.0, -1.0,
     1.0, -1.0, -1.0
};

static float f_compass_e[] =
{
     1.0,  1.0, -1.0,
     1.0, -2.0, -1.0,
     1.0,  1.0, -1.0
};

static float f_compass_se[] =
{
     1.0, -1.0, -1.0,
     1.0, -2.0, -1.0,
     1.0,  1.0,  1.0
};

static float f_compass_s[] =
{
    -1.0, -1.0, -1.0,
     1.0, -2.0,  1.0,
     1.0,  1.0,  1.0
};

static float f_compass_sw[] =
{
    -1.0, -1.0,  1.0,
    -1.0, -2.0,  1.0,
     1.0,  1.0,  1.0
};

static float f_compass_w[] =
{
    -1.0,  1.0,  1.0,
    -1.0, -2.0,  1.0,
    -1.0,  1.0,  1.0
};

static float f_compass_nw[] =
{
     1.0,  1.0,  1.0,
    -1.0, -2.0,  1.0,
    -1.0, -1.0,  1.0
};

static float	f_gaussian_3x3[] =	/* sigma = 1.0 */
{
    0.075114, 0.123841, 0.075114,
    0.123841, 0.204180, 0.123841,
    0.075114, 0.123841, 0.075114
};

static float f_gaussian_5x5[] =		/* sigma = 1.0 */
{
    0.002969, 0.013306, 0.021938, 0.013306, 0.002969,
    0.013306, 0.059634, 0.098320, 0.059634, 0.013306,
    0.021938, 0.098320, 0.162103, 0.098320, 0.021938,
    0.013306, 0.059634, 0.098320, 0.059634, 0.013306,
    0.002969, 0.013306, 0.021938, 0.013306, 0.002969,
};

static float f_gaussian_7x7[] =		/* sigma = 1.0 */
{
    0.000020, 0.000239, 0.001073, 0.001769, 0.001073, 0.000239, 0.000020,
    0.000239, 0.002917, 0.013071, 0.021551, 0.013071, 0.002917, 0.000239,
    0.001073, 0.013071, 0.058582, 0.096585, 0.058582, 0.013071, 0.001073,
    0.001769, 0.021551, 0.096585, 0.159241, 0.096585, 0.021551, 0.001769,
    0.001073, 0.013071, 0.058582, 0.096585, 0.058582, 0.013071, 0.001073,
    0.000239, 0.002917, 0.013071, 0.021551, 0.013071, 0.002917, 0.000239,
    0.000020, 0.000239, 0.001073, 0.001769, 0.001073, 0.000239, 0.000020
};

static float f_isotropic[] =
{
     0.0,     -M_SQRT2, -2.0,
     M_SQRT2,  0.0,     -M_SQRT2,
     2.0,      M_SQRT2,  0.0
};

static float f_kirsch[] =
{
     5.0,  5.0,  5.0,
    -3.0,  0.0, -3.0,
    -3.0, -3.0, -3.0
};

static float	f_laplacian_1d[] =
{
   -1.0,  2.0, -1.0
};

static float	f_laplacian_2d[] =
{
    0.0, -1.0,  0.0,
   -1.0,  4.0, -1.0,
    0.0, -1.0,  0.0
};

static float	f_laplacian_3d[] =
{
    0.0,  0.0,  0.0,  0.0, -1.0,  0.0,  0.0,  0.0,  0.0,
    0.0, -1.0,  0.0, -1.0,  6.0, -1.0,  0.0, -1.0,  0.0,
    0.0,  0.0,  0.0,  0.0, -1.0,  0.0,  0.0,  0.0,  0.0 
};

static float f_line_e_w[] =
{
    -1.0, -1.0, -1.0,
     2.0,  2.0,  2.0,
    -1.0, -1.0, -1.0
};

static float f_line_ne_sw[] =
{
    -1.0, -1.0,  2.0,
    -1.0,  2.0, -1.0,
     2.0, -1.0, -1.0
};

static float f_line_n_s[] =
{
    -1.0,  2.0, -1.0,
    -1.0,  2.0, -1.0,
    -1.0,  2.0, -1.0
};

static float f_line_nw_se[] =
{
     2.0, -1.0, -1.0,
    -1.0,  2.0, -1.0,
    -1.0, -1.0,  2.0
};

static float f_prewitt[] =		/* also called "smoothed" */
{
     0.0, -1.0, -2.0,
     1.0,  0.0, -1.0,
     2.0,  1.0,  0.0
};

static float f_roberts[] =
{
     0.0,  0.0,  0.0,
     1.0,  1.0,  0.0,
    -1.0, -1.0,  0.0
};

static float f_sobel[] =
{
     0.0, -2.0, -2.0,
     2.0,  0.0, -2.0,
     2.0,  2.0,  0.0
};

/*
 * Tables of filters that are valid for data with this dimensionality.
 *
 * NOTE:  1) All alphabetic characters MUST be lower case here.
 *	  2) The tables must be kept in sorted order.
 */

static FilterTable	_1D_Filters[] =
{
    {"box",		1, _3,		f_box_3},
    {"box:1d",		1, _3,		f_box_3},
    {"laplacian",	1, _3,		f_laplacian_1d},
    {"laplacian:1d",	1, _3,		f_laplacian_1d},
    {NULL}
};

static FilterTable	_2D_Filters[] =
{
    {"4-connected",	2, _3x3, 	f_4_connected},
    {"4connected",	2, _3x3, 	f_4_connected},
    {"8-connected",	2, _3x3, 	f_box_3x3},
    {"8connected",	2, _3x3, 	f_box_3x3},
    {"box",		2, _3x3, 	f_box_3x3},
    {"box:2d",		2, _3x3, 	f_box_3x3},
    {"compass:e",	2, _3x3, 	f_compass_e},
    {"compass:n",	2, _3x3, 	f_compass_n},
    {"compass:ne",	2, _3x3, 	f_compass_ne},
    {"compass:nw",	2, _3x3, 	f_compass_nw},
    {"compass:s",	2, _3x3, 	f_compass_s},
    {"compass:se",	2, _3x3, 	f_compass_se},
    {"compass:sw",	2, _3x3, 	f_compass_sw},
    {"compass:w",	2, _3x3, 	f_compass_w},
    {"gaussian",	2, _3x3,	f_gaussian_3x3},
    {"gaussian:2d",	2, _3x3,	f_gaussian_3x3},
    {"gaussian:3x3",	2, _3x3,	f_gaussian_3x3},
    {"gaussian:5x5",	2, _5x5,	f_gaussian_5x5},
    {"gaussian:7x7",	2, _7x7,	f_gaussian_7x7},
    {"isotropic",	2, _3x3, 	f_isotropic},
    {"kirsch",		2, _3x3, 	f_kirsch},
    {"laplacian",	2, _3x3,	f_laplacian_2d},
    {"laplacian:2d",	2, _3x3,	f_laplacian_2d},
    {"line:e-w",	2, _3x3, 	f_line_e_w},
    {"line:n-s",	2, _3x3, 	f_line_n_s},
    {"line:ne-sw",	2, _3x3, 	f_line_ne_sw},
    {"line:nw-se",	2, _3x3, 	f_line_nw_se},
    {"prewitt",		2, _3x3, 	f_prewitt},
    {"roberts",		2, _3x3, 	f_roberts},
    {"smoothed",	2, _3x3, 	f_prewitt},
    {"sobel",		2, _3x3, 	f_sobel},
    {NULL}
};

static FilterTable	_3D_Filters[] =
{
    {"26-connected",	3, _3x3x3,	f_box_3x3x3},
    {"26connected",	3, _3x3x3,	f_box_3x3x3},
    {"6-connected",	3, _3x3x3,	f_6_connected},
    {"6connected",	3, _3x3x3,	f_6_connected},
    {"box",		3, _3x3x3,	f_box_3x3x3},
    {"box:3d",		3, _3x3x3,	f_box_3x3x3},
    {"laplacian",	3, _3x3x3,	f_laplacian_3d},
    {"laplacian:3d",	3, _3x3x3,	f_laplacian_3d},
    {NULL}
};


/*
 * A handle to all the dimensional filters.
 */

static FilterTable	*AllFilters[] =
{
    NULL,
    _1D_Filters,
    _2D_Filters,
    _3D_Filters
};

/*
Morph (filter, operation, mask);
*/

Error
m_Morph (Object *in, Object *out)
{
    Error	ret	= ERROR;
    Object	opmin	= (Object) DXNewString ("min");
    Object	opmax	= (Object) DXNewString ("max");
    Object	op1	= NULL;
    Object	op2	= NULL;
    char	*str	= DEFAULT_MORPH;
    Object	min[4];
    Object	obj1	= NULL;
    Object	obj2	= NULL;

    if (! opmin || ! opmax)
	goto error;

    if (in[1])
    {
	if (! DXExtractString (in[1], &str))
	{
	    DXSetError (ERROR_BAD_PARAMETER, "#10200", "operation");
	    goto error;
	}
    }

    if      (! strcasecmp (str, "erode"))
    {
	op1 = opmax;
	DXDelete(opmin);
    }
    else if (! strcasecmp (str, "dilate"))
    {
	op1 = opmin;
	DXDelete(opmax);
    }
    else if (! strcasecmp (str, "open"))
    {
	op1 = opmax;
	op2 = opmin;
    }
    else if (! strcasecmp (str, "close"))
    {
	op1 = opmin;
	op2 = opmax;
    }
    else
    {
	DXSetError (ERROR_BAD_PARAMETER, "#10210", str, "operation");
	goto error;
    }
	
    min[0] = in[0];
    min[1] = op1;
    min[2] = NULL;
    min[3] = in[2];

    if (! m_FilterWorker (min, &obj1, TRUE))
	goto error;

    if (op2)
    {
	min[0] = DXReference (obj1);
	min[1] = op2;
	if (! m_FilterWorker (min, &obj2, TRUE))
	    goto error;
	out[0] = obj2;
	obj2 = NULL;
    }
    else
    {
	out[0] = obj1;
	obj1 = NULL;
    }

    ret = OK;

cleanup:
    DXDelete (obj1);
    DXDelete (obj2);
    DXDelete ((Object) op1);
    DXDelete ((Object) op2);
    return (ret);

error:
    goto cleanup;
}

Error
m_Filter (Object *in, Object *out)
{
    return (m_FilterWorker (in, out, FALSE));
}

#define	I_INPUT		in[0]
#define	I_FILTER	in[1]
#define	I_COMPONENT	in[2]
#define	I_MASK		in[3]

static Error
m_FilterWorker (Object *in, Object *out, int morph)
{
    Object		input		= I_INPUT;
    Error		status		= ERROR;
    Filter		*filter		= NULL;
    char		*component	= NULL;
    Object		output		= NULL;

    if (_CheckInput (input) == ERROR)
	goto cleanup;
    
    if (_CheckFilter (I_FILTER, I_MASK, &filter, morph) == ERROR)
	goto cleanup;
    
    if (_CheckComponent (I_COMPONENT, &component) == ERROR)
	goto cleanup;

    /*
     * Since we can't directly grow Fields, we wrap them up in a
     * CompositeField to make life easier.  We will also have to
     * check lower down for the case where a Field is contained
     * in something like a Group.  If we find this then we
     * can just attach a helper CompositeField above the Field since
     * the Field will already have a positive reference count.  We
     * have to play this little game here and a few lines down since
     * the copy creates a Field with a zero reference count and the
     * subsequent deletion of the CF would cause the Field that we want
     * to go away.
     */

    if (DXGetObjectClass (input) == CLASS_FIELD)
    {
	input = (Object) DXNewCompositeField ();
	DXSetEnumeratedMember ((Group) input, 0, in[0]);
    }

    output = DXCopy (input, COPY_STRUCTURE);

    if (output == NULL)
	goto cleanup;

    status = _Filter_Object (output, filter, component, TRUE);

cleanup:
    _DestroyFilter (filter);

    /*
     * This is the second half of dealing with a top level Field in which we
     * extract it.  We extract the field back out of the copy.  Since it
     * has a positive reference count we make a copy and then blow away the
     * all of the helper CompositeField.  This copy now becomes the return
     * value.
     */

    if (input != I_INPUT)
    {
	if (output)
	{
	    Object		f;
	    Object		r;

	    f = DXGetEnumeratedMember ((Group) output, 0, NULL);
	    r = DXCopy (f, COPY_STRUCTURE);
	    if (r == NULL)
		status = ERROR;
	    DXDelete (output);
	    output = r;
	}

	DXDelete (input);
    }

    if (status == OK)
	out[0] = output;
    else
    {
	/*
	 * Make sure that we only delete REAL copies.
	 */

	if (output != input)
	    DXDelete (output);
    }

    return (status);
}


static Error _CheckInput (Object obj)
{
    if (obj == NULL)
    {
	DXSetError (ERROR_BAD_PARAMETER, "#10000", "input");
	return (ERROR);
    }

    return (OK);
}


static Error
_CheckFilter (Object filt, Object mask, Filter **filterp, int morph)
{
    Object	obj	= NULL;
    char	*name	= NULL;
    char	*desc;
    Filter	*filter	= NULL;
    FilterType	type	= FT_CONVOLVE;
    int		done	= FALSE;
    float	rvalue	= 1.0;
    int		size;

    *filterp = NULL;

    /*
     * If the filter has been specified by name we need to see if it is
     * a normal convolution filter or a rank-value filter.  If it is
     * the latter then we may need to decide later what its specific
     * value should be.  If the filter is a single number then we'll 
     * assume that it is a rank-value filter and that this is the value
     * that is being specified.
     */

    switch (type)
    {
	case FT_CONVOLVE:
	    desc = "filter";
	    name = (obj = filt) ? NULL : DEFAULT_FILTER;
	    if (! obj)
		break;

	    if (DXExtractString (obj, &name))
	    {
		char	*tname = NULL;

		if (     ! strcmp (name, FILTER_MEDIAN))
		    type = FT_RVMEDIAN;
		else if (! strcmp (name, FILTER_MAX))
		    type = morph ? FT_ERODE : FT_RVMAX;
		else if (! strcmp (name, FILTER_MIN))
		    type = morph ? FT_DILATE : FT_RANKVALUE;
		else
		    tname = name;	/* propagate the name */
		
		name = tname;
	    }
	    else if (DXExtractFloat (obj, &rvalue))
		type = FT_RANKVALUE;
	    DXResetError ();		/* OK, since may be a matrix */
	    break;

	case FT_RVMEDIAN:
	case FT_RVMAX:
	case FT_RANKVALUE:
	case FT_ERODE:
	case FT_DILATE:
setupmask:
	    done = TRUE;
	    desc = "mask";
	    name = (obj = mask) ? NULL : DEFAULT_MASK;
	    if (! obj)
		break;

	    if (DXExtractString (obj, &name))
		break;
	    DXResetError ();
	    break;
    }

    if (! done)
    {
	size = sizeof (Filter);
	filter = (Filter *) DXAllocate (size);
	if (filter == NULL)
	    goto error;
	memset (filter, 0, size);
    }

    filter->name   = name;
    filter->type   = type;
    filter->rvalue = rvalue;

    /*
     * If the filter has been specified by name then we will look it up
     * with each individual field since there may be 1D, 2D and 3D fields
     * combined in a group.  If it has been explicitly specified as an
     * Array then we fill in the information now and expect that the
     * dimensions will be correct.  If they aren't then we'll pass back
     * an error later.
     */

    if (name == NULL && (type == FT_CONVOLVE || done))
    {
	int		it;
	Type		type;
	Category	cat;
	int		rank;
	int		shape[MAXDIMS];
	int		delta[MAXDIMS];
	int		count;
	int		maxdim;
	float		*vals;
	int		i;

	if (DXGetObjectClass (obj) != CLASS_ARRAY ||
	    DXGetArrayInfo ((Array)obj, &it, &type, &cat, &rank, shape) == NULL)
	{
	    DXSetError (ERROR_BAD_PARAMETER, "#11920", desc);
	    goto error;
	}

	if (it != 1)
	{
	    DXSetError (ERROR_BAD_PARAMETER, "#11832", desc, "items");
	    goto error;
	}
	
	if (cat != CATEGORY_REAL)
	{
	    DXSetError (ERROR_BAD_PARAMETER, "#10330", desc);
	    goto error;
	}
	
	/*
	 * Wrap scalars up as 1-vectors.
	 */

	if (rank == 0)
	{
	    rank = 1;
	    shape[0] = 1;
	}

	filter->rank = rank;

	for (i = rank, count = 1, maxdim = 1; i--; )
	{
	    delta[i] = count;
	    count *= shape[i];
	    if (shape[i] > maxdim)
		maxdim = shape[i];
	    if ((shape[i] & 1) == 0)
	    {
		DXSetError (ERROR_BAD_PARAMETER, "#10370", desc, 
			    "given odd sized dimensions");
		goto error;
	    }
	}

	filter->count  = count;
	filter->maxdim = maxdim;
	memcpy (filter->shape, shape, rank * sizeof (int));
	memcpy (filter->delta, delta, rank * sizeof (int));

	/*
	 * The actual filter values are always stored as floats.
	 */

	vals = (float *) DXAllocate (count * sizeof (float));
	if (vals == NULL)
	    goto error;
	
	filter->vals = vals;

#define	COPYUP(_t)\
{\
    _t		*src	= (_t *) DXGetArrayData ((Array) obj);\
    float	*dst	= vals;\
    int		_i;\
    for (_i = 0; _i < count; _i++)\
	*dst++ = (float) *src++;\
}
	switch (type)
	{
	    case TYPE_BYTE:	COPYUP (byte);	 break;
	    case TYPE_UBYTE:	COPYUP (ubyte);	 break;
	    case TYPE_USHORT:	COPYUP (ushort); break;
	    case TYPE_UINT:	COPYUP (uint);	 break;
	    case TYPE_SHORT:	COPYUP (short);	 break;
	    case TYPE_INT:	COPYUP (int);	 break;
	    case TYPE_FLOAT:	COPYUP (float);	 break;
	    case TYPE_DOUBLE:	COPYUP (double); break;
	    default:
		DXSetError (ERROR_BAD_PARAMETER, "#10320", desc);
		goto error;
	}
    }

    if (type != FT_CONVOLVE && ! done)
	goto setupmask;

    *filterp = filter;
    return (OK);

error:
    _DestroyFilter (filter);
    return (ERROR);
}


static Error _CheckComponent (Object obj, char **component)
{
    char	*name	= NULL;
    char	*junk	= NULL;

    if (obj == NULL)
	name = DEFAULT_COMPONENT;
    else
    {
	if (DXExtractString (obj, &name) != NULL)
	{
	    if (DXExtractNthString (obj, 1, &junk) != NULL)
	    {
		DXSetError (ERROR_BAD_PARAMETER, "#10200", "component");
		return (ERROR);
	    }
	}
	else
	{
	    DXSetError (ERROR_BAD_PARAMETER, "#11838",
			"component specification");
	    return (ERROR);
	}
    }
	
    *component = name;
    return (OK);
}


static void _DestroyFilter (Filter *f)
{
    if (f == NULL)
	return;

    DXFree ((Pointer) f->vals);
    DXFree ((Pointer) f);
}

static Error _Filter_Object (Object obj, Filter *filter,
			     char *component, int top)
{
    Class		class;
    Error		ret;
    Object		kid;
    Object		cf;
    Category		cat;
    int			rank;
    int			shape[100];

    class = DXGetObjectClass (obj);
    if (class == CLASS_GROUP)
	class = DXGetGroupClass ((Group) obj);

    switch (class)
    {
	case CLASS_GROUP:
	case CLASS_SERIES:
	case CLASS_MULTIGRID:
	    ret = _Filter_Group ((Group) obj, filter, component);
	    break;

	case CLASS_COMPOSITEFIELD:
	    ret = _Filter_CompositeField ((CompositeField) obj, filter,
					  component);
	    break;

	/*
	 * This assumes that the field has already been grown.
	 */

	case CLASS_FIELD:
	    ret = _Filter_Field ((Field) obj, filter, component);
	    break;

	/*
	 * For the following cases, the enclosed object needs to be wrapped
	 * in a composite field so that it can be grown if it is a field.
	 */

	case CLASS_XFORM:
	case CLASS_CLIPPED:
	case CLASS_SCREEN:
	    switch (class)
	    {
		case CLASS_XFORM:
		    if (! DXGetXformInfo ((Xform) obj, &kid, NULL))
			return (ERROR);
		    break;
		case CLASS_CLIPPED:
		    if (! DXGetClippedInfo ((Clipped) obj, &kid, NULL))
			return (ERROR);
		    break;
		case CLASS_SCREEN:
		    if (! DXGetScreenInfo ((Screen) obj, &kid, NULL, NULL))
			return (ERROR);
		    break;
	        default:
		    break;
	    }

	    if (DXGetObjectClass (kid) != CLASS_FIELD)
		ret = _Filter_Object (kid, filter, component, top);
	    else
	    {
		cf = (Object) DXNewCompositeField ();
		if (cf == NULL)
		    return (ERROR);
		DXSetEnumeratedMember ((Group) cf, 0, kid);
		ret = _Filter_CompositeField ((CompositeField) cf, filter,
					      component);
		DXDelete ((Object) cf);
	    }
	    break;

	default:
	    if (top)
	    {
		DXSetError (ERROR_BAD_PARAMETER, "#10191", "input");
		ret = ERROR;
	    }
	    else
		ret = OK;
	    break;
    }

    if (ret != OK)
	return (ret);

    /*
     * If we are doing morphology then we always get back a TYPE_UBYTE
     * regardless of what the original type was.  This needs to be reflected
     * in the group typing information of special kinds of groups, e.g.
     * other than generic groups.
     */

    if (filter->type == FT_ERODE || filter->type == FT_DILATE)
    {
	switch (class)
	{
	    case CLASS_SERIES:
	    case CLASS_MULTIGRID:
	    case CLASS_COMPOSITEFIELD:
		DXGetType (obj, NULL, &cat, &rank, shape);
		DXSetGroupTypeV ((Group) obj, TYPE_UBYTE, CATEGORY_REAL,
				 rank, shape);
	    default: 
	        break;
	}
    }

    return (ret);
}


static Error _Filter_Group (Group obj, Filter *filter, char *component)
{
    FilterTask		ft;
    Error		ret = ERROR;
    int			n;

    ft.obj       = (Object) obj;
    ft.class	 = DXGetObjectClass ((Object) obj);
    if (ft.class == CLASS_GROUP)
	ft.class = DXGetGroupClass (obj);
    ft.filter    = filter;
    ft.component = component;

    for (n = 0; DXGetEnumeratedMember ((Group) obj, n, NULL); n++)
	;	/* count up the number of members */

    switch (n)
    {
	case 0:
	    return (OK);

	case 1:
	    ret = _Work_G (&ft, 0);
	    break;

	default:
	    if (DXCreateTaskGroup () != OK)
		goto cleanup;

	    if (DXAddLikeTasks ((PFE) _Work_G, (Pointer) &ft,
			      sizeof (ft), 1.0, n) != OK)
	    {
		DXAbortTaskGroup ();
		goto cleanup;
	    }

	    ret = DXExecuteTaskGroup ();
	    break;
    }

cleanup:
    return (ret);
}


static Error _Work_G (FilterTask *ft, int n)
{
    FilterTask	lft;
    Object	obj;
    Error	ret;
    Object	cf	= NULL;
    Class	class;

    lft = *ft;

    obj = DXGetEnumeratedMember ((Group) lft.obj, n, NULL);
    class = DXGetObjectClass (obj);

    /*
     * Wrap any stray fields contained within Groups with CompositeFields
     * so that DXGrow will work properly.
     */

    if (class == CLASS_FIELD && lft.class != CLASS_COMPOSITEFIELD)
    {
	cf = (Object) DXNewCompositeField ();
	if (cf == NULL)
	    return (ERROR);
	DXSetEnumeratedMember ((Group) cf, 0, obj);
	obj = cf;
    }

    ret = _Filter_Object (obj, lft.filter, lft.component, FALSE);

    /*
     * DXRemove any CompositeField wrappers we may have put on.
     */

    if (cf != NULL)
	DXDelete ((Object) cf);

    return (ret);
}

static Error _Filter_CompositeField (CompositeField obj, Filter *filter,
				     char *component)
{
    Error	ret		= ERROR;
    Field	f;
    Array	a;
    int		dims[MAXDIMS];
    int		rank		= 0;
    Filter	*nfilter	= NULL;
    int		copied		= FALSE;
    Object	o;
    char	*dep;
    DepOn	d;
    int		growth;

    /*
     * We'll use the first member in the CF to give us its dimensionality.
     */

    f = (Field) DXGetEnumeratedMember ((Group) obj, 0, NULL);
    if (f == NULL)
	return (OK);

    if (DXGetObjectClass ((Object) f) != CLASS_FIELD)
    {
	DXSetError (ERROR_BAD_PARAMETER, "#10191", "composite field member");
	goto cleanup;
    }

    if (DXEmptyField (f))
	return (OK);

    /*
     * Make sure that this CF contains the desired component before we really
     * start doing work.
     */

    a = (Array) DXGetComponentValue (f, component);
    if (a == NULL)
    {
	DXSetError (ERROR_BAD_PARAMETER, "#10250", "input", component);
	goto cleanup;
    }

    /*
     * Make sure that the component that we are going to filter depends on
     * the "positions" component.
     */

    o = DXGetComponentAttribute (f, component, "dep");
    if (o == NULL)
    {
	DXSetError (ERROR_BAD_PARAMETER, "#10255", component, "dep");
	goto cleanup;
    }

     

    o = DXExtractString (o, &dep);
    if (o == NULL)
    {
	DXSetError (ERROR_BAD_PARAMETER, "#11250", component);
	goto cleanup;
    }
    if (! strcmp (dep, "positions"))
	d = DEP_POSITIONS;
    else if (! strcmp (dep, "connections"))
	d = DEP_CONNECTIONS;
    else
    {
	DXSetError (ERROR_BAD_PARAMETER, "#11250", component);
	goto cleanup;
    }

    /*
     * Make sure that the component we are going to filter doesn't
     * ref something else. This module does not correctly handle
     * delayed colors
     */
    o = DXGetComponentAttribute (f, component, "ref");
    if (o != NULL)
    {
	DXSetError (ERROR_BAD_PARAMETER, "component %s may not reference another component (e.g. delayed colors not supported)", component);
	goto cleanup;
    } 


    /*
     * OK, let's get the Field's (and hence the CF's) dimensionality.
     * In addition, we are also checking to make sure that it is regular.
     */

    a = (Array) DXGetComponentValue (f, "connections");
    if (_SetFieldDims (a, &rank, dims, d) != OK)
	goto cleanup;

    /*
     * If we need to do a filter lookup or change its dimension to match
     * that of the data then let's make a copy of it.
     */

    if (filter->name || filter->rank != rank)
    {
	nfilter = _CopyFilter (filter);
	if (nfilter == NULL)
	    goto cleanup;
	copied = TRUE;
    }
    else
	nfilter = filter;

    if (_SetFilterData (nfilter, rank) != OK)
	goto cleanup;

    /*
     * Now that everything seems to check out, we can get on to the real work.
     */

    o = (Object) obj;
    growth = nfilter->maxdim >> 1;

    if (growth > 0)
    {
	o = DXGrow (o, growth, GROW_REPLICATE,
		    component, "invalid positions", "invalid connections",
		    NULL);
	if (o == NULL)
	    goto cleanup;
    }

    ret = _Filter_Group ((Group) o, nfilter, component);

    if (growth > 0 && ret == OK)
    {
	o = DXShrink (o);
	if (o == NULL)
	    ret = ERROR;
    }

cleanup:
    if (copied)
	_DestroyFilter (nfilter);

    return (ret);
}


static Error
_SetFieldDims (Array a, int *rank, int *dims, DepOn d)
{
    int		i;

    if (a == NULL)
    {
	DXSetError (ERROR_BAD_PARAMETER, "#10240", "connections");
	return (ERROR);
    }

    a = DXQueryGridConnections (a, rank, dims);
    if (a == NULL)
    {
	DXSetError (ERROR_BAD_PARAMETER, "#10610", "input");
	return (ERROR);
    }
    
    if (d == DEP_CONNECTIONS)
    {
	for (i = 0; i < *rank; i++)
	    dims[i]--;
    }
    
    return (OK);
}


static Filter *_CopyFilter (Filter *old)
{
    int		size;
    Filter	*new	= NULL;
    float	*vals	= NULL;

    size = sizeof (Filter);
    new = (Filter *) DXAllocate (size);
    if (new == NULL)
	goto error;

    size = old->count * sizeof (float);
    if (size > 0)
    {
	vals = (float *) DXAllocate (size);
	if (vals == NULL)
	    goto error;
	memcpy (vals, old->vals, size);
    }

    *new = *old;
    new->vals = vals;

    return (new);

error:
    DXFree ((Pointer) new);
    DXFree ((Pointer) vals);
    return (NULL);
}


static Error _SetFilterData (Filter *f, int rank)
{
    int		i;
    int		maxfilter;
    FilterTable	*ft=NULL;
    int		count;
    int		frank;

    if (rank > MAXDIMS)
    {
	DXSetError (ERROR_BAD_PARAMETER, "#10180", "input dimensiobs", MAXDIMS);
	return (ERROR);
    }

    /*
     * If this is a named filter then let's try to resolve it now.  We
     * do this by looking through each dimension's filter table, starting
     * at the rank of the data for a filter whose name matches.
     */

    if (f->name)
    {
	char		buf[256];
	char		*np;
	int		delta[MAXDIMS];
	float		*vals;
	int		maxdim;

	maxfilter = sizeof (AllFilters) / sizeof (FilterTable *) - 1;

	for (i = 0, np = f->name; (buf[i++] = (char) tolower ((int) *np++)); )
	    ;

	for (i = MIN (rank, maxfilter); i > 0; i--)
	{
	    for (ft = AllFilters[i]; ft->name; ft++)
		if (! strcmp (buf, ft->name))
		    break;

	    /*
	     * If we found a match then take us out of the for loop too.
	     */

	    if (ft->name)
		break;
	}

	if (ft->name == NULL)
	{
	    DXSetError (ERROR_BAD_PARAMETER, "#12150", f->name, rank);
	    return (ERROR);
	}

	/*
	 * Now fill in the data about the filter.  We remove the name to
	 * let others know that this is a resolved filter.
	 */

	f->name = NULL;
	frank = f->rank = ft->rank;
	for (i = frank, count = 1, maxdim = 1; i--; )
	{
	    delta[i] = count;
	    count *= ft->shape[i];
	    if (ft->shape[i] > maxdim)
		maxdim = ft->shape[i];
	}

	vals = (float *) DXAllocate (count * sizeof (float));
	if (vals == NULL)
	    return (ERROR);
	
	f->count  = count;
	f->maxdim = maxdim;
	f->vals   = vals;
	memcpy (f->shape, ft->shape, frank * sizeof (int));
	memcpy (f->delta,     delta, frank * sizeof (int));
	memcpy (vals,     ft->vals,  count * sizeof (float));
    }

    if (f->rank > rank)
    {
	DXSetError (ERROR_BAD_PARAMETER, "#12153", f->rank, rank);
	return (ERROR);
    }

    /*
     * At this stage we just need to bump up the dimensionality of the filter
     * to match that of the data being filtered.
     */

    if (f->rank != rank)
    {
	int		dr;

	frank = f->rank;

	dr    = rank - frank;
	count = f->count;

	for (i = rank; i--; )
	    f->shape[i] = (i < dr) ? 1     : f->shape[i - dr];
	for (i = rank; i--; )
	    f->delta[i] = (i < dr) ? count : f->delta[i - dr];
	
	f->rank = rank;
    }

    return (OK);
}

static Error
Invalidate (Field obj, int items, DepOn d, int olength, int *offsets)
{
    Error	ret	= ERROR;
    Object	inv;
    char	*what;
    char	*orig;
    int		ninv;
    InvalidComponentHandle 	ihandle = NULL;
    InvalidComponentHandle 	ohandle = NULL;
    int		i, j;
    int		elem, e;

    if (d == DEP_POSITIONS)
    {
	what = "positions";
	/* invl = "invalid positions"; */
	orig = "original invalid positions";
    }
    else
    {
	what = "connections";
	/* invl = "invalid connections"; */
	orig = "original invalid connections";
    }

    inv = DXGetComponentValue (obj, orig);
    if (! inv)
	return (OK);

#if GROW_BUG_IN_REF
    if (DXGetAttribute (inv, "ref"))
    {
	DXSetError (ERROR_INTERNAL, "DXGrow/ref");
	return (ERROR);
    }
#endif

    ihandle = DXCreateInvalidComponentHandle ((Object) obj, NULL, what);
    ohandle = DXCreateInvalidComponentHandle ((Object) obj, NULL, what);
    if (! ihandle || ! ohandle)
	goto cleanup;

    ninv = DXGetInvalidCount (ihandle);
    for (i = 0; i < ninv; i++)
    {
	elem = DXGetNextInvalidElementIndex (ihandle);

	for (j = 0; j < olength; j++)
	{
	    e = elem + offsets[j];
	    if (e < 0 || e >= items)
		continue;
	    if (! DXSetElementInvalid (ohandle, e))
		goto cleanup;
	}
    }

    if (! DXDeleteComponent (obj, orig))
	goto cleanup;

    if (! DXSaveInvalidComponent (obj, ohandle))
	goto cleanup;
    
    ret = OK;

cleanup:
    DXFreeInvalidComponentHandle (ihandle);
    DXFreeInvalidComponentHandle (ohandle);
    return (ret);
}


static Error _Filter_Field (Field obj, Filter *filter, char *component)
{
    Error	ret 	= ERROR;
    char	buf[256];
    Array	ga;			/* Grown    component		*/
    Array	oa;			/* Original component		*/
    Array	fa	= NULL;		/* Filtered component		*/
    DepOn	d;
    int		items;
    int		olength = 0;
    int		*offsets = NULL;

    if (DXEmptyField (obj))
    {
	ret = OK;
	goto cleanup;
    }

    strcpy (buf, "original ");
    strcat (buf, component);

    offsets = (int *)DXAllocate(filter->count * sizeof(int));
    if (!offsets)
	goto cleanup;

    ga = (Array) DXGetComponentValue (obj, component);
    oa = filter->maxdim == 1 ? ga : (Array) DXGetComponentValue (obj, buf);

    if (! DXGetArrayInfo (ga, &items, NULL, NULL, NULL, NULL))
	goto cleanup;

    {
	Object		o;
	char		*dep;

	o = DXGetComponentAttribute (obj, component, "dep");
	if (o == NULL)
	{
	    DXSetError (ERROR_BAD_PARAMETER, "#10255", component, "dep");
	    goto cleanup;
	}

	o = DXExtractString (o, &dep);
	if (o == NULL)
	{
	    DXSetError (ERROR_BAD_PARAMETER, "#11250", component);
	    goto cleanup;
	}

	if (! strcmp (dep, "positions"))
	    d = DEP_POSITIONS;
	else if (! strcmp (dep, "connections"))
	    d = DEP_CONNECTIONS;
	else
	{
	    DXSetError (ERROR_BAD_PARAMETER, "#11250", component);
	    goto cleanup;
	}
    }

    fa = _Filter_Array (obj, filter, ga, oa, d, &olength, offsets);
    if (fa == NULL)
	goto cleanup;

    obj = DXSetComponentValue (obj, component, (Object) fa);
    if (obj == NULL)
	goto cleanup;
	
    if (filter->maxdim != 1)
    {
	obj = DXDeleteComponent (obj, buf);
	if (obj == NULL)
	    goto cleanup;
    }

    obj = DXChangedComponentValues (obj, component);
    if (obj == NULL)
	goto cleanup;

    if (! Invalidate (obj, items, d, olength, offsets))
	goto cleanup;

    obj = DXEndField (obj);
    if (obj == NULL)
	goto cleanup;

    ret = OK;

cleanup:
    DXFree((Pointer)offsets);
    return (ret);
}


static Array
_Filter_Array (Field obj, Filter *filter, Array ga, Array oa, DepOn d,
		int *olength, int *offsets)
{
    Array		ret		= NULL;
    int			oi;
    Type		ot;
    Type		newt;
    Category		oc;
    int			or;
    int			os[MAXDIMS];
    Array		pos;
    int			rank;
    unsigned int	shape[MAXDIMS];
    unsigned int	delta[MAXDIMS];
    int			isize;
    unsigned int	count;
    Steps		gbound[MAXDIMS];
    Steps		obound[MAXDIMS];
    int			incr;
    Pointer		src;
    Pointer		dst;
    int			tsize;
    int			tosize;
    IrregularData	irrdata;
    int			size;
    int			s;
    int			i, j, k;
    int			inc, val, ival, lim;

    memset (&irrdata, 0, sizeof (irrdata));

    /*
     * Compute the number of elements in each item.
     */

    if (DXGetObjectClass ((Object) oa) != CLASS_ARRAY)
    {
	DXSetError (ERROR_BAD_PARAMETER, "#10191", "input");
	goto error;
    }

    if (DXGetArrayInfo (oa, &oi, &ot, &oc, &or, os) == NULL)
	goto error;

    for (i = 0, isize = 1; i < or; i++)
	isize *= os[i];

    switch (ot)
    {
	case TYPE_BYTE:		tsize = sizeof (byte);   break;
	case TYPE_UBYTE:	tsize = sizeof (ubyte);  break;
	case TYPE_USHORT:	tsize = sizeof (ushort); break;
	case TYPE_UINT:		tsize = sizeof (uint);   break;
	case TYPE_SHORT:	tsize = sizeof (short);  break;
	case TYPE_INT:		tsize = sizeof (int);    break;
	case TYPE_FLOAT:	tsize = sizeof (float);  break;
	case TYPE_DOUBLE:	tsize = sizeof (double); break;
	default:
	    DXSetError (ERROR_BAD_PARAMETER, "#10320", "");
	    goto error;
    }

    /*
     * If we're doing binary morphology then reduce it to an unsigned
     * char array to save space.
     */

    switch (filter->type)
    {
	case FT_ERODE:
	case FT_DILATE:
	    newt = TYPE_UBYTE;
	    tosize = sizeof (ubyte);
	    break;

	default:
	    newt = ot;
	    tosize = tsize;
	    break;
    }

    /*
     * Get the size of the grown data we are working on, compute the
     * deltas between each dimension and set up the stepping bounds.
     */

    incr = filter->maxdim >> 1;

    rank = 0;
    pos = (Array) DXGetComponentValue (obj, "connections");
    if (_SetFieldDims (pos, &rank, (int *) shape, d) != OK)
	goto error;

    for (i = rank, count = isize; i--; )
    {
	delta[i] = count;
	count *= shape[i];
    }

    for (i = 0; i < rank; i++)
    {
	gbound[i].from  = incr;
	gbound[i].to    = shape[i] - incr;
	gbound[i].delta = delta[i];
	gbound[i].incr  = delta[i] * tsize;
    }

    /*
     * Now do the same thing for the ungrown data.  This gives us correct
     * steps for the destination array.  All we really need here are the
     * deltas although we'll fill in the rest just for information.
     */

    rank = 0;
    pos = (Array) DXGetComponentValue (obj, incr ? "original connections"
					       : "connections");
    if (_SetFieldDims (pos, &rank, (int *) shape, d) != OK)
	goto error;

    for (i = rank, count = isize; i--; )
    {
	delta[i] = count;
	count *= shape[i];
    }

    for (i = 0; i < rank; i++)
    {
	obound[i].from  = 0;
	obound[i].to    = shape[i];
	obound[i].delta = delta[i];
	obound[i].incr  = delta[i] * tosize;
    }

    /*
     * OK we're finally ready to create our output Array.
     */

    ret = DXNewArrayV (newt, oc, or, os);
    if (ret == NULL)
	goto error;

    if (DXAddArrayData (ret, 0, oi, NULL) == NULL)
	goto error;

    src = DXGetArrayData (ga);
    dst = DXGetArrayData (ret);
    if (src == NULL || dst == NULL)
	goto error;

    irrdata.ftype   = filter->type;
    irrdata.rvalue  = filter->rvalue;
    irrdata.rank    = rank;
    irrdata.type    = ot;
    irrdata.size    = isize;
    irrdata.src.p   = src;
    irrdata.dst.p   = dst;
    irrdata.srcb    = gbound;
    irrdata.dstb    = obound;
    irrdata.fsize   = filter->count;

    /*
     * Compute the offsets within the src data of each element of the filter.
     */

    size = irrdata.fsize * sizeof (int);
    irrdata.offsets = (int *) DXAllocateLocal (size);
    if (irrdata.offsets == NULL)
	irrdata.offsets = (int *) DXAllocate (size);
    if (irrdata.offsets == NULL)
	goto error;

    memset (irrdata.offsets, 0, size);
    memset (offsets, 0, size);
    for (i = 0; i < rank; i++)
    {
	s = filter->shape[i] >> 1;
	if (s == 0)
	    continue;

	inc  = gbound[i].delta;
	val  = -s * inc;
	lim  = filter->delta[i];
	ival = -val;

	k = 0;
	for (j = 0; j < irrdata.fsize; j++)
	{
	    irrdata.offsets[j] += val;
	    if (++k == lim)
	    {
		k = 0;
		val += inc;
	    }
	    if (val > ival)
		val = -ival;
	}
    }

    *olength = irrdata.fsize;
    for (j = 0; j < irrdata.fsize; j++)
	offsets[j] = irrdata.offsets[j] / isize;

    /*
     * Make a copy of the filter kernel in local memory so that only the
     * src data is coming out of global memory.  By doing this we are able
     * to make MUCH better use of the PVS's line cache.
     */

    size = irrdata.fsize * sizeof (float);
    irrdata.filter  = (float *) DXAllocateLocal (size);
    if (irrdata.filter == NULL)
	irrdata.filter = (float *) DXAllocate (size);
    if (irrdata.filter == NULL)
	goto error;
    memcpy (irrdata.filter, filter->vals, size);

    /*
     * OK, call the appropriate worker to actually do the filtering.
     */

    if (irrdata.ftype == FT_CONVOLVE && irrdata.rank == 2 &&
	filter->shape[0] == 3 && filter->shape[1] == 3)
    {
	if (_Apply_3x3 (&irrdata) != OK)
	    goto error;
    }
    else
    {
	/*
	 * Compress the zeros out of the filter to save time.
	 * If this is a rank value filter then set the rvalue if necessary.
	 */

	for (i = 0, j = 0; i < irrdata.fsize; i++)
	{
	    if (irrdata.filter[i] != 0.0)
	    {
		irrdata.filter [j] = irrdata.filter [i];
		irrdata.offsets[j] = irrdata.offsets[i];
		j++;
	    }
	}

	irrdata.fsize = j;

	switch (irrdata.ftype)
	{
	    case FT_RVMEDIAN:
		irrdata.ftype  = FT_RANKVALUE;
		irrdata.rvalue = (float) (irrdata.fsize + 1) * 0.5;
		break;

	    case FT_RVMAX:
		irrdata.ftype  = FT_RANKVALUE;
		irrdata.rvalue = (float) irrdata.fsize;
		break;

	    case FT_RANKVALUE:
		if (irrdata.rvalue < 1.0 ||
		    irrdata.rvalue > (float) irrdata.fsize)
		{
		    DXSetError (ERROR_BAD_PARAMETER, "#10120",
				"filter", 1.0, (float) irrdata.fsize);
		    goto error;
		}
		break;
	    default:
	        break;
	}

	irrdata.rvalue -= 1.0;

	_Apply_Irregular (&irrdata);
    }

normal:
    DXFree ((Pointer) irrdata.filter);
    DXFree ((Pointer) irrdata.offsets);
    return (ret);

error:
    DXDelete ((Object) ret);
    ret = NULL;
    goto normal;
}


/* CONVOLUTION WITHOUT CLAMPING */
#define LOOP(_L,_TYPE)\
    for (i = from; i < to; i++)\
    {\
	for (j = 0; j < input.size; j++, input.src._L++, input.dst._L++)\
	{\
	    sum    = (float) 0.0;\
	    fval   = input.filter;\
	    offset = input.offsets;\
	    for (k = 0; k < input.fsize; k++)\
		sum += *fval++ * (float) *(input.src._L + *offset++);\
	    *input.dst._L = (_TYPE) sum;\
	}\
    }

/* CONVOLUTION WITH CLAMPING */
#define LOOPC(_L,_TYPE)\
    for (i = from; i < to; i++)\
    {\
	for (j = 0; j < input.size; j++, input.src._L++, input.dst._L++)\
	{\
	    sum    = (float) 0.0;\
	    fval   = input.filter;\
	    offset = input.offsets;\
	    for (k = 0; k < input.fsize; k++)\
		sum += *fval++ * (float) *(input.src._L + *offset++);\
	    if (sum < _minclamp._L)\
		sum = _minclamp._L;\
	    else if (sum > _maxclamp._L)\
		sum = _maxclamp._L;\
	    *input.dst._L = (_TYPE) sum;\
	}\
    }

/* RANK VALUE */
#define	RVLOOP(_L,_TYPE)\
{\
    int		arg1	= (int) input.rvalue;\
    int		arg2	= arg1 == input.fsize - 1 ? arg1 : arg1 + 1;\
    float 	delta	= input.rvalue - (float) arg1;\
    float	ds;\
    for (i = from; i < to; i++)\
    {\
	register float	sum;\
	register int	change;\
	register _TYPE	tmp;\
	register _TYPE	*sort	= (_TYPE *) _sort;\
	for (j = 0; j < input.size; j++, input.src._L++, input.dst._L++)\
	{\
	    register int	*offset = input.offsets;\
	    for (k = 0; k < input.fsize; k++)\
		sort[k] = *(input.src._L + *offset++);\
	    for (change = TRUE; change; )\
	    {\
		change = FALSE;\
		for (k = 1; k < input.fsize; k++)\
		{\
		    if (sort[k] < sort[k - 1])\
		    {\
			change = TRUE;\
			tmp = sort[k];\
			sort[k] = sort[k - 1];\
			sort[k - 1] = tmp;\
		    }\
		}\
	    }\
	    ds  = (float) sort[arg2] - (float) sort[arg1];\
	    sum = (float) sort[arg1] + delta * ds;\
	    *input.dst._L = (_TYPE) sum;\
	}\
    }\
}

/* EROSION */
#define ERLOOP(_L,_TYPE)\
{\
    register int	isum;\
    register _TYPE	zero = (_TYPE) 0;\
    for (i = from; i < to; i++)\
    {\
	for (j = 0; j < input.size; j++, input.src._L++, input.dst.c++)\
	{\
	    isum   = 0;\
	    offset = input.offsets;\
	    for (k = 0; k < input.fsize; k++)\
		isum += *(input.src._L + *offset++) == zero ? 0 : 1;\
	    *input.dst.c = (ubyte) (isum < input.fsize ? 0 : 1);\
	}\
    }\
}

/* DILATION */
#define DLOOP(_L,_TYPE)\
{\
    register int	isum;\
    register _TYPE	zero = (_TYPE) 0;\
    for (i = from; i < to; i++)\
    {\
	for (j = 0; j < input.size; j++, input.src._L++, input.dst.c++)\
	{\
	    isum   = 0;\
	    offset = input.offsets;\
	    for (k = 0; k < input.fsize; k++)\
		isum += *(input.src._L + *offset++) == zero ? 0 : 1;\
	    *input.dst.c = (ubyte) (isum > 0 ? 1 : 0);\
	}\
    }\
}

static void _Apply_Irregular (IrregularData *data)
{
    IrregularData	input;

    unsigned int	sincr;
    unsigned int	dincr;
    unsigned int	from;
    unsigned int	to;
    unsigned int	i, j, k;
    float		sum;
    int			*offset;
    float		*fval;
    int			size;
    double		__sort[LOCAL_SORT];
    double		*_sort	= NULL;

    input = *data;

    from  = input.srcb->from;
    to    = input.srcb->to;
    sincr = input.srcb->incr;
    dincr = input.dstb->incr;

    input.src.c += sincr * from;

    if (input.rank != 1)
    {
	input.rank -= 1;
	input.srcb += 1;
	input.dstb += 1;

	for (i = from; i < to; i++, input.src.c += sincr, input.dst.c += dincr)
	    _Apply_Irregular (&input);

	return;
    }

    size = input.fsize * sizeof (double);
    _sort = (double *) (input.fsize <= LOCAL_SORT ? __sort
						  : DXAllocateLocal (size));
    if (! _sort)
    {
	_sort = (double *) DXAllocate (size);
	if (! _sort)
	    return;
    }

    switch (input.ftype)
    {
	case FT_CONVOLVE:
	    switch (input.type)
	    {
		case TYPE_BYTE:		LOOPC (b,byte);		break;
		case TYPE_UBYTE:	LOOPC (c,ubyte);	break;
		case TYPE_USHORT:	LOOPC (us,ushort); 	break;
		case TYPE_UINT:		LOOPC (ui,uint); 	break;
		case TYPE_SHORT:	LOOPC (s,short);	break;
		case TYPE_INT:		LOOPC (i,int);		break;
		case TYPE_FLOAT:	LOOP  (f,float);	break;
		case TYPE_DOUBLE:	LOOP  (d,double);	break;
	        default: break;
	    }
	    break;
	
	case FT_RANKVALUE:
	    switch (input.type)
	    {
		case TYPE_BYTE:		RVLOOP (b,byte);	break;
		case TYPE_UBYTE:	RVLOOP (c,ubyte);	break;
		case TYPE_USHORT:	RVLOOP (us,ushort);	break;
		case TYPE_UINT:		RVLOOP (ui,uint);	break;
		case TYPE_SHORT:	RVLOOP (s,short);	break;
		case TYPE_INT:		RVLOOP (i,int);		break;
		case TYPE_FLOAT:	RVLOOP (f,float);	break;
		case TYPE_DOUBLE:	RVLOOP (d,double);	break;
	        default: break;
	    }
	    break;

	case FT_ERODE:
	    switch (input.type)
	    {
		case TYPE_BYTE:		ERLOOP (b,byte);	break;
		case TYPE_UBYTE:	ERLOOP (c,ubyte);	break;
		case TYPE_USHORT:	ERLOOP (us,ushort);	break;
		case TYPE_UINT:		ERLOOP (ui,uint);	break;
		case TYPE_SHORT:	ERLOOP (s,short);	break;
		case TYPE_INT:		ERLOOP (i,int);		break;
		case TYPE_FLOAT:	ERLOOP (f,float);	break;
		case TYPE_DOUBLE:	ERLOOP (d,double);	break;
	        default: break;
	    }
	    break;

	case FT_DILATE:
	    switch (input.type)
	    {
		case TYPE_BYTE:		DLOOP (b,byte);		break;
		case TYPE_UBYTE:	DLOOP (c,ubyte);	break;
		case TYPE_USHORT:	DLOOP (us,ushort);	break;
		case TYPE_UINT:		DLOOP (ui,uint);	break;
		case TYPE_SHORT:	DLOOP (s,short);	break;
		case TYPE_INT:		DLOOP (i,int);		break;
		case TYPE_FLOAT:	DLOOP (f,float);	break;
		case TYPE_DOUBLE:	DLOOP (d,double);	break;
	        default: break;
	    }
	    break;
         default: 
	    break;
    }

    if (_sort && _sort != __sort)
	DXFree ((Pointer) _sort);
}

#undef	LOOP
#undef  LOOPC
#undef	RVLOOP

/*
 * $$$$ Still to look at:
 * $$$$ If the local allocate fails then don't do the copy in any more.
 */

#define	MAXROWLEN	8192

static Error _Apply_3x3 (IrregularData *data)
{
    IrregularData	input;
    register float	f00, f01, f02, f10, f11, f12, f20, f21, f22;
    register float	c00=0, c01=0, c02=0, c10=0, c11=0, c12=0, c20=0, c21=0, c22=0;
    register float	*r0ptr,  *r1ptr,  *r2ptr,  *outptr;
    float		*r0base, *r1base, *r2base, *outbase;
    float		*rtmp;
    register int	r, c, i;
    register int	numitems;
    int			numrows;
    int			numcols;
    int			nselem;
    int			ndelem;
    int			cnt;
    Upointer		src;
    Upointer		dst;
    Steps		*dstb;
    float		row0[MAXROWLEN];
    float		row1[MAXROWLEN];
    float		row2[MAXROWLEN];
    float		rowo[MAXROWLEN];
    int			size;
    Error		ret = ERROR;

    input    = *data;
    numitems = input.size;
    dstb     = input.dstb;
    numrows  = dstb->to;
    dstb++;
    numcols  = dstb->to;
    src      = input.src;
    dst      = input.dst;

    /*
     * We add 2 here since numcols is the number of columns in the destintion
     * and the source has been padded out by 1 on each side.
     */

    ndelem   = numcols * numitems;
    nselem   = ndelem + 2 * numitems;

    /*
     * Load the filter into our local variables.
     */

    f00 = input.filter[0]; f01 = input.filter[1]; f02 = input.filter[2];
    f10 = input.filter[3]; f11 = input.filter[4]; f12 = input.filter[5];
    f20 = input.filter[6]; f21 = input.filter[7]; f22 = input.filter[8];

    size = nselem * sizeof (float);
    r0base  = nselem <= MAXROWLEN ? row0 : (float *) DXAllocateLocal (size);
    r1base  = nselem <= MAXROWLEN ? row1 : (float *) DXAllocateLocal (size);
    r2base  = nselem <= MAXROWLEN ? row2 : (float *) DXAllocateLocal (size);
    outbase = nselem <= MAXROWLEN ? rowo : (float *) DXAllocateLocal (size);

    if (r0base == NULL || r1base == NULL || r2base == NULL || outbase == NULL)
	goto cleanup;

    /*
     * Load rows 0 and 1 of the src into the local row buffers.
     */

#define LOOP(_L,_TYPE)\
    for (cnt = 0, rtmp = r0base; cnt < nselem; cnt++)\
	*rtmp++ = (float) *src._L++;\
    for (cnt = 0, rtmp = r1base; cnt < nselem; cnt++)\
	*rtmp++ = (float) *src._L++;

    switch (input.type)
    {
	case TYPE_BYTE:		LOOP(b,byte);	break;
	case TYPE_UBYTE:	LOOP(c,ubyte);	break;
	case TYPE_USHORT:	LOOP(us,ushort);break;
	case TYPE_UINT:		LOOP(ui,uint);	break;
	case TYPE_SHORT:	LOOP(s,short);	break;
	case TYPE_INT:		LOOP(i,int);	break;
	case TYPE_FLOAT:	LOOP(f,float);	break;
	case TYPE_DOUBLE:	LOOP(d,double);	break;
        default: break;
    }
#undef LOOP
	
    for (r = 0; r < numrows; r++)
    {
	/*
	 * Load the new bottom row.
	 */

#define LOOP(_L,_TYPE)\
    for (cnt = 0, rtmp = r2base; cnt < nselem; cnt++)\
	*rtmp++ = (float) *src._L++;

	switch (input.type)
	{
	    case TYPE_BYTE:	LOOP(b,byte);	break;
	    case TYPE_UBYTE:	LOOP(c,ubyte);	break;
	    case TYPE_USHORT:	LOOP(us,ushort);break;
	    case TYPE_UINT:	LOOP(ui,uint);break;
	    case TYPE_SHORT:	LOOP(s,short);	break;
	    case TYPE_INT:	LOOP(i,int);	break;
	    case TYPE_FLOAT:	LOOP(f,float);	break;
	    case TYPE_DOUBLE:	LOOP(d,double);	break;
	    default: break;
	}
#undef LOOP

	for (i = 0; i < numitems; i++)
	{
	    /*
	     * Set up the row pointers, offset according to which item
	     * we are currently working on.
	     */

	    r0ptr  = r0base  + i;
	    r1ptr  = r1base  + i;
	    r2ptr  = r2base  + i;
	    outptr = outbase + i;

	    /*
	     * Set up columns 0 and 1 in c#0 and c#1
	     */

	    c00 = *r0ptr; r0ptr += numitems;
	    c01 = *r0ptr; r0ptr += numitems;
	    c10 = *r1ptr; r1ptr += numitems;
	    c11 = *r1ptr; r1ptr += numitems;
	    c20 = *r2ptr; r2ptr += numitems;
	    c21 = *r2ptr; r2ptr += numitems;

	    /*
	     * Now we slide the window along a single item.  We also rotate
	     * the locally held columns so that we don't have to reload
	     * things (that's why there are 3 copies of essentially the same
	     * stuff in this loop.
	     *
	     * Each iteration consists of loading the new rightmost column
	     * into our local cache, computing the resulting value.
	     * We try to keep a 3 stage pipeline busy here hence the 2
	     * final sets of instructions for column sizes that aren't
	     * multiples of 3.
	     *
	     * We also optimize the order of the multiplies, doing the ones
	     * that use the next set of column values to be loaded first
	     * so that they their loading can hopefully be overlapped with
	     * the rest of the arithmetic.  We do this by transposing the
	     * "matrix of operations".
	     */

#define	LOAD_C0		c00 = *r0ptr; c10 = *r1ptr; c20 = *r2ptr
#define	LOAD_C1		c01 = *r0ptr; c11 = *r1ptr; c21 = *r2ptr
#define	LOAD_C2		c02 = *r0ptr; c12 = *r1ptr; c22 = *r2ptr
#define	BUMPPTR		r0ptr += numitems; r1ptr += numitems;\
			r2ptr += numitems; outptr += numitems
#define	CONV012		*outptr =\
    f00 * c00 + f10 * c10 + f20 * c20 +\
    f01 * c01 + f11 * c11 + f21 * c21 +\
    f02 * c02 + f12 * c12 + f22 * c22
#define	CONV120		*outptr =\
    f00 * c01 + f10 * c11 + f20 * c21 +\
    f01 * c02 + f11 * c12 + f21 * c22 +\
    f02 * c00 + f12 * c10 + f22 * c20
#define	CONV201		*outptr =\
    f00 * c02 + f10 * c12 + f20 * c22 +\
    f01 * c00 + f11 * c10 + f21 * c20 +\
    f02 * c01 + f12 * c11 + f22 * c21

	    for (c = numcols; c > 2; c -= 3)
	    {
		 LOAD_C2; CONV012; BUMPPTR;
		 LOAD_C0; CONV120; BUMPPTR;
		 LOAD_C1; CONV201; BUMPPTR;
	    }

	    if (c-- > 0)
	    {
		 LOAD_C2; CONV012; BUMPPTR;
	    }

	    if (c-- > 0)
	    {
		 LOAD_C0; CONV120; BUMPPTR;
	    }

#undef	LOAD_C0
#undef	LOAD_C1
#undef	LOAD_C2
#undef	BUMPPTR
#undef	CONV012
#undef	CONV120
#undef	CONV201
	}

	/*
	 * rotate the row buffers
	 */

	rtmp   = r0base;
	r0base = r1base;
	r1base = r2base;
	r2base = rtmp;

	/*
	 * DXWrite the convolved output line out to the destination buffer.
	 */

#define LOOP(_L,_TYPE)\
    for (cnt = 0, rtmp = outbase; cnt < ndelem; cnt++)\
	*dst._L++ = (_TYPE) *rtmp++;

#define LOOPC(_L,_TYPE)\
    for (cnt = 0, rtmp = outbase; cnt < ndelem; cnt++)\
    {\
	if (*rtmp < _minclamp._L)\
	    *rtmp = _minclamp._L;\
	else if (*rtmp > _maxclamp._L)\
	    *rtmp = _maxclamp._L;\
	*dst._L++ = (_TYPE) *rtmp++;\
    }

	switch (input.type)
	{
	    case TYPE_BYTE:	LOOPC(b,byte);	break;
	    case TYPE_UBYTE:	LOOPC(c,ubyte);	break;
	    case TYPE_USHORT:	LOOPC(us,ushort);break;
	    case TYPE_UINT:	LOOPC(ui,uint);break;
	    case TYPE_SHORT:	LOOPC(s,short);	break;
	    case TYPE_INT:	LOOPC(i,int);	break;
	    case TYPE_FLOAT:	LOOP(f,float);	break;
	    case TYPE_DOUBLE:	LOOP(d,double);	break;
	    default: break;
	}
#undef LOOP
#undef LOOPC
    }

    ret = OK;

cleanup:
    if (r0base != row0 && r0base != row1 && r0base != row2)
	DXFree ((Pointer) r0base);
    if (r1base != row0 && r1base != row1 && r1base != row2)
	DXFree ((Pointer) r1base);
    if (r2base != row0 && r2base != row1 && r2base != row2)
	DXFree ((Pointer) r2base);
    if (outbase != rowo)
	DXFree ((Pointer) outbase);
    return (ret);
}
