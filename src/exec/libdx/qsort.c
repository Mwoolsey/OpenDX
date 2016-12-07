/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


/*
 * Header: /usr/people/gresh/code/svs/src/libdx/RCS/qsort.c,v 5.0 92/11/12 09:08:41 svs Exp Locker: gresh 
 * Locker: gresh 
 */

/*
 * To use, define macros for
 *    TYPE	type of element
 *    LT(a,b)	a<b
 *    GT(a,b)	a>b
 *    QUICKSORT	name of routine
 */

#define SWAP(a, b, s) (s) = (a), (a) = (b), (b) = (s)

#define THRESHOLD          	4	/* threshold for insertion */
#define MEDIANTHRESHOLD         6	/* threshold for median */

static void QUICKSORT_LOCAL(TYPE *base, TYPE *qslmax);

static void
QUICKSORT(TYPE *base, int n)
{
    register TYPE *i;
    register TYPE *j;
    register TYPE *hi;
    register TYPE *lo;
    TYPE swaptmp;
    TYPE *min;
    TYPE *qsmax;

    /*
     * don't bother to short really short arrays
     */
    if (n <= 1)
	return;

    qsmax = base + n;
    if (n >= THRESHOLD)
    {
	QUICKSORT_LOCAL(base, qsmax);
	hi = base + THRESHOLD;
    }
    else
    {
	hi = qsmax;
    }

    /*
     * find sentinel for insertion sort
     */
    for (j = lo = base; ++lo < hi;)
	if (LT(lo, j))
	    j = lo;

    if (j != base)
	SWAP(*base, *j, swaptmp);

    /*
     * sort using insertion sort
     */
    for (min = base; (hi = ++min) < qsmax;)
    {
	while (hi--, GT(hi, min))
	     continue ;

	if (++hi != min)
	{
	    swaptmp = *min;
	    for (i = j = min; --j >= hi; i = j)
		*i = *j;
	    *i = swaptmp;
	}
    }
}


/*
 * perform quicksort partitioning
 */

static void
QUICKSORT_LOCAL(TYPE *base, TYPE *qslmax)
{
    register TYPE *i;
    register TYPE *j;
    register TYPE *t;
    TYPE *mid;
    TYPE *tmp;
    TYPE swaptmp;
    int         lo, hi;

    lo = qslmax - base;
    do
    {
	/*
	 * Find good median guess by looking at first, last and middle
	 * values.  Just use middle value if less than threshold
	 */
	mid = base + (lo >> 1);
	i = mid;
	if (lo >= MEDIANTHRESHOLD)
	{
	    /*
	     * compare first and middle values
	     */
	    t = base;
	    j = GT(t, i) ? t : i;

	    /*
	     * compare larger of first and middle with last
	     */
	    tmp = qslmax - 1;
	    if (GT(j, tmp))
	    {
		j = (j == t) ? i : t;
		if (LT(j, tmp))
		    j = tmp;
	    }
	    if (j != i)
		SWAP(*i, *j, swaptmp);
	}

	/*
	 * now do the sorting
	 */
	for (i = base, j = qslmax - 1;;)
	{
	    while (i < mid && !GT(i, mid))
		i++;
	    while (j > mid)
	    {
		if (!GT(mid, j))
		{
		    j--;
		    continue;
		}
		tmp = i + 1;	/* value of i after swap */
		if (i == mid)
		{
		    /* j <-> mid, new mid is j */
		    mid = t = j;
		}
		else
		{
		    /* i <-> j */
		    t = j;
		    j--;
		}
		goto swap;
	    }
	    if (i == mid)
	    {
		break;
	    }
	    else
	    {
		/* i <-> mid, new mid is i */
		t = mid;
		tmp = mid = i;	/* value of i after swap */
		j--;
	    }
    swap:
	    SWAP(*i, *t, swaptmp);
	    i = tmp;
	}

	/*
	 * if partition size is larger than THRESHOLD recurse, otherwise
	 * continue to iterate
	 */
	j = mid;
	i = j + 1; 
	lo = j - base;
	hi = qslmax - i;
	if (lo <= hi)
	{
	    if (lo >= THRESHOLD)
		QUICKSORT_LOCAL(base, j);
	    base = i;
	    lo = hi;
	}
	else /* hi > lo */
	{
	    if (hi >= THRESHOLD)
		QUICKSORT_LOCAL(i, qslmax);
	    qslmax = j;
	}

    } while (lo >= THRESHOLD);
}
