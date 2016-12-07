/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>



#include <string.h>
#include <ctype.h>
#include <dx/dx.h>
#include "_compute.h"

static Array MakeSequence(Array start, Array end, int *n, Array delta, 
			  char *method);
static Array MakeLinearList(Array start, Array end, int *n, Array delta);
static Array MakeLogList(Array start, Array end, int *n, Array delta);
static Array MakeLog10List(Array start, Array end, int *n, Array delta);
static Array MakeExponentialList(Array start, Array end, int *n, Array delta);
static Array MakeGeometricList(Array start, Array end, int *n, Array delta);
static Array MakeFibonacciList(Array start, Array end, int *n);
static void lowercase(char *in, char *out);
static int IsZero(Array a);
static int IsEqual(Array a, Array b);
static int IsNegative(Array a);
static char *TypeName(Type t);



/* the module is intended to make generating lists easy.
 *  if any 3 of the first 4 parms are given, this generates the 
 *  missing parm and makes a list.
 * 
 * if the last parm is a string, this generates lists with uneven spacings
 * like exponential, fibbonaci, etc
 */

#define icheck(obj, label) 					\
    if (obj) {							\
        int n; 							\
								\
        if (DXGetObjectClass(obj) != CLASS_ARRAY) { 		\
	    DXSetError(ERROR_BAD_PARAMETER, "#10380", label); 	\
            goto error;						\
	}							\
								\
	if (!DXGetArrayInfo((Array)obj, &n, NULL, NULL, NULL, NULL)) \
            goto error;						\
								\
        if (n != 1) { 						\
	    DXSetError(ERROR_BAD_PARAMETER, "#10381", label); 	\
            goto error;						\
	}							\
    }

int
m_Enumerate(Object *in, Object *out)
{
    int i;
    int *ip = NULL;
    char *cp = NULL;
    out[0] = NULL;


    icheck(in[0], "start");
    icheck(in[1], "end");

    if (in[2]) {
	if (!DXExtractInteger(in[2], &i) || (i <= 0)) {
	    DXSetError(ERROR_BAD_PARAMETER, "#10020", "count");
	    goto error;
	}
	ip = &i;
    }
    
    icheck(in[3], "delta");

    if (in[4] && !DXExtractString(in[4], &cp)) {
	DXSetError(ERROR_BAD_PARAMETER, "#10200", "method");
	goto error;
    }
    
    out[0] = (Object)MakeSequence((Array)in[0], (Array)in[1], ip, 
				  (Array)in[3], cp);
    
  error:
    return out[0] ? OK : ERROR;
}


/*
 * internal entry point; generate a single array given 3 of the 4
 *  start, end, nitems, delta  using method for spacing.
 */
static 
Array MakeSequence(Array start, Array end, int *n, Array delta, char *method)
{
    char *lmethod = NULL;
    Array output = NULL;

    if (!method)
	return MakeLinearList(start, end, n, delta);

    lmethod = DXAllocateLocalZero(strlen(method)+1);
    if (!lmethod)
	return NULL;

    lowercase(method, lmethod);

    if (!strcmp(lmethod, "linear")) {
	output = MakeLinearList(start, end, n, delta);
	goto done;
    }

    if (!strcmp(lmethod, "log")) {
	output = MakeLogList(start, end, n, delta);
	goto done;
    }

    if (!strcmp(lmethod, "log10")) {
	output = MakeLog10List(start, end, n, delta);
	goto done;
    }

    if (!strcmp(lmethod, "exponential")) {
	output = MakeExponentialList(start, end, n, delta);
	goto done;
    }

    if (!strcmp(lmethod, "geometric")) {
	output = MakeGeometricList(start, end, n, delta);
	goto done;
    }

    if (!strcmp(lmethod, "fibonacci")) {
	if (delta)
	    DXWarning("ignoring `delta' for fibonacci method");

	output = MakeFibonacciList(start, end, n);
	goto done;
    }

    /* put other methods here and in the error message list below */

#if 0 /* eventually */
    DXSetError(ERROR_BAD_PARAMETER, "#10211", "method", 
        "`linear', `log', `log10', `exponential', `geometric' or `fibonacci'");
#else
    DXSetError(ERROR_BAD_PARAMETER, "#10370", "method", "`linear'");
#endif
    
  done:
    DXFree((Pointer)lmethod);
    return output;
}

#define MAXSHAPE 100
#define INCVOID(d, n) (void *)((byte *)d + n)

#define MAXCOMPINPUTS 22

/* item[n] = start + n * delta */
static 
Array MakeLinearList(Array start, Array end, int *n, Array delta)
{
    int i, j;
    Array a_start = NULL, a_end = NULL, a_delta = NULL;
    int delstart = 0, delend = 0, deldelta = 0;
    Array alist[3];
    Array output = NULL;
    int count = 1;
    int bytes;
    Type t;
    Category c;
    int rank;
    int shape[MAXSHAPE];
    int nitems;
    int i_start, i_end, i_delta;
    float f_start, f_end, f_delta, f_count;
    Pointer dp;
    int *ip;
    Object in[MAXCOMPINPUTS];  /* hardcoded in compute */
    Object out;
    String compstr = NULL;
    char cbuf[64];
    Error rc;

    /* do this or die in compute */
    for (i=0; i<MAXCOMPINPUTS; i++)
	in[i] = NULL;

    /* find common format of start, end, delta and check for 4 parms */
    i = 0;
    if (start)
	alist[i++] = start; 

    if (end)
	alist[i++] = end;

    if (delta)
	alist[i++] = delta;

    if (n) {
	count = *n;
	if (i == 3) {
	    DXWarning("too many inputs specified; ignoring delta");
	    i--;
	    delta = NULL;
	} 
    } 
    else if (i == 2)
	count = 2;

    
    if (i < 2) {
	DXSetError(ERROR_BAD_PARAMETER, 
		   "not enough inputs specified to generate a list");
	return NULL;
    }

    if (!DXQueryArrayCommonV(&t, &c, &rank, shape, i, alist)) {
	DXAddMessage("start, end and/or delta");
	return NULL;
    }

    /* shortcut the process here if the data is scalar integer or
     *  scalar float.  otherwise, if the data is vector or ubyte
     *  or whatever, fall through and use Compute so we can increment
     *  by irregular values.
     */
    if (t != TYPE_INT && t != TYPE_FLOAT)
	goto complicated;
    if (c != CATEGORY_REAL || rank != 0)
	goto complicated;


    /* compute missing value(s):
     * start = end - ((count - 1) * delta) 
     * end = start + ((count - 1) * delta) 
     * count = ((end - start) / delta) + 1 
     * delta = (end - start) / (count - 1) 
     */

    /* convert to the common format */
    if (start)
	a_start = DXArrayConvertV(start, t, c, rank, shape);
    if (end)
	a_end   = DXArrayConvertV(end,   t, c, rank, shape);
    if (delta)
	a_delta = DXArrayConvertV(delta, t, c, rank, shape);


    /* for integer, scalar lists */
    if (t == TYPE_INT) {
	if (!start) {
	    i_end = *(int *)DXGetArrayData(a_end);
	    i_delta = *(int *)DXGetArrayData(a_delta);
	    
	    i_start = i_end - ((count - 1) * i_delta);
	}

	if (!end) {
	    i_start = *(int *)DXGetArrayData(a_start);
	    i_delta = *(int *)DXGetArrayData(a_delta);
	    
	    i_end = i_start + ((count - 1) * i_delta);
	} 

	if (!delta) {
	    /* if count == 1, generate a zero of the right type.  otherwise
	     *  divide to figure out the right delta to make count-1 steps 
	     *  between start and end.  it's count-1 because if you want
	     *  N numbers between start and end, you have N-1 increments.
	     */
	    i_start = *(int *)DXGetArrayData(a_start);
	    i_end = *(int *)DXGetArrayData(a_end);
	    if (count == 1)
		i_delta = 0;
	    else
		i_delta = (i_end - i_start) / (count - 1);
	    
	    /* try to catch the case where delta ends up being 0 (like
	     * because the inputs are int and the count is larger than
	     * the difference between start and end).  allow it to be zero
	     * only if start == end;  i suppose if you ask for 10 things
	     * where start == end you should be able to get them.
	     */
	    if (i_delta == 0 && i_start != i_end) {
		DXSetError(ERROR_BAD_PARAMETER, 
		     "count too large to generate list between start and end");
		goto error;
	    }
	    
	}

	/* if all three arrays are there, count must be missing */
	if (i == 3) {
	    i_start = *(int *)DXGetArrayData(a_start);
	    i_end = *(int *)DXGetArrayData(a_end);
	    i_delta = *(int *)DXGetArrayData(a_delta);

	    if (i_delta == 0)
		count = 1;
	    else {
		if ((i_end >= i_start && i_delta > 0) ||
		    (i_end < i_start && i_delta < 0))
		    count = (int)(((double)i_end-i_start) / (double)i_delta) +1;
		else {
		    if (i_delta < 0)
			DXSetError(ERROR_BAD_PARAMETER,
			   "delta must be positive if start is less than end");
		    else
			DXSetError(ERROR_BAD_PARAMETER,
			   "delta must be negative if end is less than start");
		    goto error;
		}
	    }
	}

	output = (Array)DXNewRegularArray(TYPE_INT, 1, count, 
					 (Pointer)&i_start, (Pointer)&i_delta);
    }

    /* for float, scalar lists */
    if (t == TYPE_FLOAT) {
	if (!start) {
	    f_end = *(float *)DXGetArrayData(a_end);
	    f_delta = *(float *)DXGetArrayData(a_delta);
	    
	    f_start = f_end - ((count - 1.0) * f_delta);
	}

	if (!end) {
	    f_start = *(float *)DXGetArrayData(a_start);
	    f_delta = *(float *)DXGetArrayData(a_delta);
	    
	    f_end = f_start + ((count - 1.0) * f_delta);
	}

	if (!delta) {
	    /* if count == 1, generate a zero of the right type.  otherwise
	     *  divide to figure out the right delta to make count-1 steps 
	     *  between start and end.  it's count-1 because if you want
	     *  N numbers between start and end, you have N-1 increments.
	     */
	    f_start = *(float *)DXGetArrayData(a_start);
	    f_end = *(float *)DXGetArrayData(a_end);
	    if (count == 1)
		f_delta = 0.0;
	    else 
		f_delta = (f_end - f_start) / (count - 1.0);

	    /* try to catch the case where delta ends up being 0 (like
	     * because the inputs are int and the count is larger than
	     * the difference between start and end).  allow it to be zero
	     * only if start == end;  i suppose if you ask for 10 things
	     * where start == end you should be able to get them.
	     */
	    if (f_delta == 0.0 && f_start != f_end) {
		DXSetError(ERROR_BAD_PARAMETER, 
		   "count too large to generate list between start and end");
		goto error;
	    }
	}

	/* if all three arrays are there, count must be missing */
	if (i == 3) {
	    f_start = *(float *)DXGetArrayData(a_start);
	    f_end = *(float *)DXGetArrayData(a_end);
	    f_delta = *(float *)DXGetArrayData(a_delta);

	    if (f_delta == 0.0)
		count = 1;
	    else {
		if ((f_end >= f_start && f_delta > 0) ||
		    (f_end < f_start && f_delta < 0)) {
		    /* the intermediate float variable below is to minimize
		     * float round-off error.  if delta is 0.1 and you
		     * ask for a list between 0 and 1, it does the math in
		     * double, the delta used is actually 0.10000001, and 
		     * you get counts = 10.9999999 instead of 11.  when
		     * converted directly to int it becomes just 10 and your 
		     * list ends at 0.9 instead of 1.  
		     * math in base 2 has some problems.
		     */
		    f_count = ((f_end - f_start) / f_delta) +1;
		    count = (int)f_count;
		} else {
		    if (f_delta < 0)
			DXSetError(ERROR_BAD_PARAMETER,
			   "delta must be positive if start is less than end");
		    else
			DXSetError(ERROR_BAD_PARAMETER,
			   "delta must be negative if end is less than start");
		    goto error;
		}
	    }
	}

	output = (Array)DXNewRegularArray(TYPE_FLOAT, 1, count, 
					 (Pointer)&f_start, (Pointer)&f_delta);
    }
    
    DXDelete((Object)a_start);
    DXDelete((Object)a_end);
    DXDelete((Object)a_delta);
    
    /* return Array */
    return output;
    

    /* input is a vector, or a data type different from int or float.
     * use compute so this code doesn't have to be replicated for each
     * different shape and type.
     */

  complicated:
    nitems = 1;
    for (j=0; j<rank; j++)
	nitems *= shape[j];

    /* compute missing value(s):
     * start = end - ((count - 1) * delta) 
     * end = start + ((count - 1) * delta) 
     * count = ((end - start) / delta) + 1 
     * delta = (end - start) / (count - 1) 
     */

    if (!start) {
	compstr = DXNewString("$0 - (($1 - 1) * $2)");
	if (!compstr)
	    goto error;
	
	in[0] = (Object)compstr;
	in[1] = (Object)end;
	in[3] = (Object)delta;
	in[2] = (Object)DXNewArray(TYPE_INT, CATEGORY_REAL, 0);
	if (!in[2])
	    goto error;
	if (!DXAddArrayData((Array)in[2], 0, 1, (Pointer)&count))
	    goto error;

	/* i need to explain this - it's basically so if compute was 
         * going to try to cache this, it could add a reference and
         * then later when i call delete the object won't get deleted
         * out from underneath compute.  (i know compute doesn't cache
         * things, but a different module might.)
	 */
	DXReference((Object)compstr);
	DXReference(in[2]);

	rc = m_Compute(in, &out);

	DXDelete((Object)compstr);
	compstr = NULL;
	DXDelete(in[2]);
	in[2] = NULL;

	if (rc == ERROR)
	    goto error;

	start = (Array)out;
	delstart++;
    }

    if (!end) {
	compstr = DXNewString("$0 + (($1 - 1) * $2)");
	if (!compstr)
	    goto error;
	
	in[0] = (Object)compstr;
	in[1] = (Object)start;
	in[3] = (Object)delta;
	in[2] = (Object)DXNewArray(TYPE_INT, CATEGORY_REAL, 0);
	if (!in[2])
	    goto error;
	if (!DXAddArrayData((Array)in[2], 0, 1, (Pointer)&count))
	    goto error;

	DXReference((Object)compstr);
	DXReference(in[2]);

	rc = m_Compute(in, &out);

	DXDelete((Object)compstr);
	compstr = NULL;
	DXDelete(in[2]);
	in[2] = NULL;

	if (rc == ERROR)
	    goto error;

	end = (Array)out;
	delend++;
    }

    if (!delta) {
	/* if count == 1, generate a zero of the right type.  otherwise
         *  divide to figure out the right delta to make count-1 steps 
	 *  between start and end.  it's count-1 because if you want
         *  N numbers between start and end, you have N-1 increments.
	 */
	if (count == 1)
	    compstr = DXNewString("$1 - $1");
	else
	    compstr = DXNewString("($2 - $0) / ($1 - 1)");
	if (!compstr)
	    goto error;
	
	in[0] = (Object)compstr;
	in[1] = (Object)start;
	in[3] = (Object)end;
	in[2] = (Object)DXNewArray(TYPE_INT, CATEGORY_REAL, 0);
	if (!in[2])
	    goto error;
	if (!DXAddArrayData((Array)in[2], 0, 1, (Pointer)&count))
	    goto error;

	DXReference((Object)compstr);
	DXReference(in[2]);

	rc = m_Compute(in, &out);

	DXDelete((Object)compstr);
	compstr = NULL;
	DXDelete(in[2]);
	in[2] = NULL;

	if (rc == ERROR)
	    goto error;

	delta = (Array)out;
	deldelta++;

	/* try to catch the case where delta ends up being 0 (like
	 * because the inputs are int and the count is larger than
	 * the difference between start and end).  allow it to be zero
	 * only if start == end;  i suppose if you ask for 10 things
	 * where start == end you should be able to get them.
	 */
	if (IsZero(delta) && !IsEqual(start, end)) {
	    DXSetError(ERROR_BAD_PARAMETER, 
		    "count too large to generate list between start and end");
	    goto error;
	}

    }

    /* if all three arrays are there, count must be missing */
    if (i == 3) {
	char tbuf[512];
	int firsttime = 1;
	int lastcount = 0;

	/* this loop allows us to to handle vectors or matricies as
	 *  well as scalars.   it requires that the deltas compute to
         *  a consistent count.  like start=[0 2 4], end=[4 8 16],
	 *  would work if delta=[1 2 4] but not if delta was [1 2 2].
	 */
	for (j=0; j < nitems; j++) {
	    /* i think this code only works for vectors - i'm not sure
             * what it will do with rank=2 data.
	     */

	    /* this point of this next compute expression:
	     * if the delta is 0, don't divide by zero - the count is 1. 
	     * if the end is smaller than the start, the delta has to be
             *  negative.  if it's not, return -1.  you can't generate a
             *  negative count from the equations, so this is a safe signal.
	     */
	    sprintf(tbuf, 
		    "float($2.%d) == 0.0   ? "
		    "  1 : "
		    " (  (($1.%d >= $0.%d) && ($2.%d > 0) || "
		    "     ($1.%d <  $0.%d) && ($2.%d < 0))    ? "
		    "       int(float($1.%d - $0.%d) / float($2.%d)) + 1 : "
		    "       -1 ) ", 
		    j, j, j, j, j, j, j, j, j, j);
	    compstr = DXNewString(tbuf);
	    if (!compstr)
		goto error;
	    
	    in[0] = (Object)compstr;
	    in[1] = (Object)start;
	    in[2] = (Object)end;
	    in[3] = (Object)delta;
	    
	    DXReference((Object)compstr);

	    rc = m_Compute(in, &out);
	    
	    DXDelete((Object)compstr);
	    compstr = NULL;
	    
	    if (rc == ERROR)
		goto error;
	    
	    if (!DXExtractInteger(out, &count)) {
		DXSetError(ERROR_BAD_PARAMETER, 
			   "can't compute number of items");
		goto error;
	    }

	    DXDelete((Object)out);
	    if (count == 0)
		continue;

	    if (count < 0) {
		if (IsNegative(delta))
		    DXSetError(ERROR_BAD_PARAMETER,
			 "delta must be positive if start is less than end");
		else
		    DXSetError(ERROR_BAD_PARAMETER,
			 "delta must be negative if end is less than start");
		goto error;
	    }

	    if (firsttime) {
		lastcount = count;
		firsttime = 0;
	    } else {
		if (count != lastcount) {
		    DXSetError(ERROR_BAD_PARAMETER, 
			   "inconsistent number of items required by inputs");
		    goto error;
		}
	    }
	}    
    }

    /* now have 4 consistant values - once again make sure they are
     * converted into an identical format.
     */
    a_start = DXArrayConvertV(start, t, c, rank, shape);
    a_end   = DXArrayConvertV(end,   t, c, rank, shape);
    a_delta = DXArrayConvertV(delta, t, c, rank, shape);

    /* make empty array with n items */
    output = DXNewArrayV(t, c, rank, shape);
    if (!output)
	goto error;

    if (!DXAddArrayData(output, 0, count, NULL))
	goto error;

    dp = DXGetArrayData(output);
    if (!dp)
	goto error;

    /* foreach n */
    /*  call compute to add delta */
    /*  memcpy to right offset in array */
    /* end */

    bytes = DXGetItemSize(output);

    sprintf(cbuf, "%s($0 + ($1 * $2))", TypeName(t));
    compstr = DXNewString(cbuf);
    if (!compstr)
	goto error;
    
    in[0] = (Object)compstr;
    in[1] = (Object)a_start;
    in[3] = (Object)a_delta;

    in[2] = (Object)DXNewArray(TYPE_INT, CATEGORY_REAL, 0);
    if (!in[2])
	goto error;
    if (!DXAddArrayData((Array)in[2], 0, 1, NULL))
	goto error;
    ip = (int *)DXGetArrayData((Array)in[2]);
    
    DXReference((Object)compstr);
    DXReference(in[2]);

    for (i=0; i<count; i++) {

	*ip = i;

	rc = m_Compute(in, &out);
	if (rc == ERROR)
	    goto error;

	memcpy(INCVOID(dp, bytes*i), DXGetArrayData((Array)out), bytes);
	DXDelete((Object)out);
    }

    DXDelete((Object)compstr);
    DXDelete(in[2]);
    DXDelete((Object)a_start);
    DXDelete((Object)a_end);
    DXDelete((Object)a_delta);
    if (delstart)
	DXDelete((Object)start);
    if (delend)
	DXDelete((Object)end);
    if (deldelta)
	DXDelete((Object)delta);

    /* return Array */
    return output;
    
  error:
    DXDelete((Object)output);
    DXDelete((Object)compstr);
    DXDelete((Object)a_start);
    DXDelete((Object)a_end);
    DXDelete((Object)a_delta);
    if (delstart)
	DXDelete((Object)start);
    if (delend)
	DXDelete((Object)end);
    if (deldelta)
	DXDelete((Object)delta);

    return NULL;
}

/* item[n] = exp(ln[start] + n * delta) */
static 
Array MakeLogList(Array start, Array end, int *n, Array delta)
{
    /* find common format of start, end, n, delta */
    /* compute missing value(s) */

    /* take log of start, end */
    /* call linear list with them */
    /* call exp on each of the returns? */
    DXSetError(ERROR_NOT_IMPLEMENTED, "#10370", "method", "`linear'");
    return NULL;
}

/* item[n] = 10 ** (log10[start] + n * delta) */
static 
Array MakeLog10List(Array start, Array end, int *n, Array delta)
{
    /* find common format of start, end, n, delta */
    /* compute missing value(s) */

    /* take log of start, end */
    /* call linear list with them */
    /* call exp on each of the returns? */
    DXSetError(ERROR_NOT_IMPLEMENTED, "#10370", "method", "`linear'");
    return NULL;
}

/* item[n] = item[n-1] + delta[n], delta[n+1] = 2*delta[n] */
static 
Array MakeExponentialList(Array start, Array end, int *n, Array delta)
{
    /* find common format of start, end, n, delta */
    /* compute missing value(s) */

    /* take exp of start, end */
    /* call even list with them */
    /* call log on each of the returns */
    DXSetError(ERROR_NOT_IMPLEMENTED, "#10370", "method", "`linear'");
    return NULL;
}

/* item[n] = item[n-1] + delta[n], delta[n+1] = ? */
static 
Array MakeGeometricList(Array start, Array end, int *n, Array delta)
{
    /* find common format of start, end, n, delta */
    /* compute missing value(s) */

    /* what? */
    DXSetError(ERROR_NOT_IMPLEMENTED, "#10370", "method", "`linear'");
    return NULL;
}

/* item[n] = item[n-1] + delta[n], delta[n] = delta[n-1] + delta[n-2]
 * so if the start = 10 and n = 7, 
 * the sequence is: 10, 11, 13, 16, 21, 29, 42.
 * (the deltas are:    1,  2,  3,  5,  8, 13)
 * giving all three parms (start, end and n) is overconstrained,
 * one must be ignored.  last one loses?  that would be n.
 */
static 
Array MakeFibonacciList(Array start, Array end, int *n)
{

    /* find common format of start, end, n, delta */
    /* compute missing value(s) */
    /* this is overconstrained with 3 parms - any 2 are sufficient */
    /* check for end < start and make delta sign negative */

    /* compute values */
    /* return Array */

    DXSetError(ERROR_NOT_IMPLEMENTED, "#10370", "method", "`linear'");
    return NULL;
}


static void lowercase(char *in, char *out)
{
    while (*in) {
	if (!isspace(*in)) 
	    *out++ = isupper(*in) ? tolower(*in) : *in;
	in++;
    }
    *out = '\0';
}

/* return whether the value is zero or not.  1 = yes, 0 = no
 * assumed to be a single item array, scalar.
 */
static int IsZero(Array a)
{
    Type t;
    Pointer value;
    
    if (!DXGetArrayInfo(a, NULL, &t, NULL, NULL, NULL))
	return 0;

    value = DXGetArrayData(a);
    if (!value)
	return 0;

    switch (t) {
        case TYPE_BYTE:   return (byte)0     == *(byte *)value;
        case TYPE_UBYTE:  return (ubyte)0    == *(ubyte *)value;
        case TYPE_SHORT:  return (short)0    == *(short *)value;
        case TYPE_USHORT: return (ushort)0   == *(ushort *)value;
        case TYPE_INT:    return (int)0      == *(int *)value;
        case TYPE_UINT:   return (uint)0     == *(uint *)value;
        case TYPE_FLOAT:  return (float)0.0  == *(float *)value;
        case TYPE_DOUBLE: return (double)0.0 == *(double *)value;
        default:	  return 0;
    }

    /* NOTREACHED */
}
    
/* return whether two values are the same or not.  in this case, we
 * don't care what the values are, so a byte-by-byte compare is good enough.
 */
static int IsEqual(Array a, Array b)
{
    int size;
    Pointer valueA, valueB;
    
    size = DXGetItemSize(a);
    
    valueA = DXGetArrayData(a);
    valueB = DXGetArrayData(b);
    
    if (size <= 0 || !valueA || !valueB)
	return 0;

    if (memcmp(valueA, valueB, size) == 0)
	return 1;

    return 0;
}

/* invert all the values - THIS MODIFIES THE DATA IN THE ARRAY - use it
 *  only on a copy of real inputs.
 */
#if 0
static void Negate(Array a)
{
    Type t;
    Pointer value;
    
    if (!DXGetArrayInfo(a, NULL, &t, NULL, NULL, NULL))
	return;
    
    value = DXGetArrayData(a);
    if (!value)
	return;
    
    switch (t) {
      case TYPE_BYTE:   *(byte *)value = - *(byte *)value;  break;
      case TYPE_SHORT:  *(short *)value = - *(short *)value;  break;
      case TYPE_INT:    *(int *)value = - *(int *)value;  break;
      case TYPE_FLOAT:  *(float *)value = - *(float *)value;  break;
      case TYPE_DOUBLE: *(double *)value = - *(double *)value;  break;
	
      case TYPE_UBYTE:
      case TYPE_USHORT:
      case TYPE_UINT:
      default:
	/* can't do anything with the unsigned values */
	break;
    }
    
}
#endif
    
/* return 1 if value is negative, 0 if >= 0
 */
static int IsNegative(Array a)
{
    Type t;
    Pointer value;
    
    if (!DXGetArrayInfo(a, NULL, &t, NULL, NULL, NULL))
	return 0;
    
    value = DXGetArrayData(a);
    if (!value)
	return 0;
    
    switch (t) {
      case TYPE_BYTE:   return (*(byte *)value < (byte)0);
      case TYPE_SHORT:  return (*(short *)value < (short)0);
      case TYPE_INT:    return (*(int *)value < 0);
      case TYPE_FLOAT:  return (*(float *)value < 0.0);
      case TYPE_DOUBLE: return (*(double *)value < 0.0);
	
      case TYPE_UBYTE:
      case TYPE_USHORT:
      case TYPE_UINT:
      default:
	return 0;
    }
    /* NOT REACHED */
}
    
static char *TypeName(Type t)
{
    switch(t) {    
      case TYPE_UBYTE:		return "ubyte";
      case TYPE_BYTE:		return "byte";
      case TYPE_USHORT:		return "ushort";
      case TYPE_SHORT:		return "short";
      case TYPE_UINT:		return "uint";
      case TYPE_INT:		return "int";
   /* case TYPE_UHYPER:		return "uhyper"; */
      case TYPE_HYPER:		return "hyper";
      case TYPE_FLOAT:		return "float";
      case TYPE_DOUBLE:		return "double";
      case TYPE_STRING:		return "string";
      default:			return "unknown";
    }
    /* notreached */
}
