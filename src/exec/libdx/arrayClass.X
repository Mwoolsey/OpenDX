/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

/*
\section{Array class}

Dynamic arrays are implemented by re-allocating the storage block
pointed to by {\tt data} as necessary, thus increasing the size of the block in
proportion to the current size.  If only one dynamic array is being
created at a time, the overhead is minimal.  If two or more are being
created simultaneously, the overhead is a small fixed number of copy
operations of the entire data.  To demonstrate this: Let $\alpha>1$ be the factor by which
the block is enlarged.  Suppose $N$ is the final size of the block,
where $\alpha^n<N<\alpha^{n+1}$.  The number of bytes copied in
filling the block is $1+\alpha+\alpha^2+\cdots+\alpha^n =
(\alpha^{n+1}-1)/(\alpha-1)$.  The average number of copy operations is
the limit as $n$ goes to infinity of the average over the interval between
$\alpha^n$ and $\alpha^{n+1}$ of the ratio between the number of bytes
copied and $N$; this can be calculated to be $(\alpha\ln\alpha)/(\alpha-1)^2$.
For $\alpha={3\over2}$ we copy the data 2.4 times; for $\alpha={4\over3}$, 
3.4 times; for $\alpha={5\over4}$, 4.5 times. In any case, if the size of the 
array is known in advance, it can be pre-allocated.  DX also keeps track of 
a local copy of the data on each processor.
*/

INCLUDE  objectClass.X

SUBCLASS Array OF Object
DEFINESC Pointer GetArrayData(Array)
IMPLEMENTS Delete GetType GetArrayData Copy

SUBCLASS SharedArray OF Array
IMPLEMENTS Delete GetArrayData

SUBCLASS RegularArray OF Array
IMPLEMENTS Delete GetArrayData

SUBCLASS PathArray OF Array
IMPLEMENTS GetArrayData

SUBCLASS ProductArray OF Array
IMPLEMENTS Delete GetArrayData

SUBCLASS MeshArray OF Array
IMPLEMENTS Delete GetArrayData

SUBCLASS ConstantArray OF Array
IMPLEMENTS GetArrayData

#define MAX_PROCESSORS 32		/* maximum number of processors */
#define NLSHAPE 5			/* small shape array size */
#define NLDATA 12			/* small data size (in doubles) */

struct array {

    struct object object;		/* object preamble */

    Type type;				/* char, short, int, float, double */
    Category category;			/* real, complex, quaternion */
    int rank;				/* dimensionality */
    int *shape;				/* extent in each dimension */
    int lshape[NLSHAPE];		/* extents for small dims */	

    unsigned int items;			/* number of items defined */
    int size;				/* size of ea item (from type, etc.) */
    Pointer data;			/* the data itself */
    double ldata[NLDATA];		/* room for data (double aligned) */
    lock_type inprogress;		/* compact data expansion in prog? */
    unsigned int allocated;		/* number of items allocated */

#if 0
    struct {				/* local copies of data: */
	int reference;			/* number of references */
	Pointer data;			/* the local data itself */
    } local[MAX_PROCESSORS];		/* one for each processor */
#endif
};

struct sharedarray {			/* shared array structure */
    struct array array;			/* array class preamble */
    int id;				/* shared memory id */
    Pointer base;			/* base of shared segment on this proc */
    long offset;			/* offset of data in segment */
};

struct regulararray {			/* regular array structure */
    struct array array;			/* array class preamble */
    Pointer origin;			/* position of first */
    Pointer delta;			/* spacing of points */
};

struct patharray {			/* regular connections structure */
    struct array array;			/* array class preamble */
    int offset;				/* information offset relative to */
};					/* field that this is part of */

struct productarray {			/* product array structure */
    struct array array;			/* array class preamble */
    int nterms;				/* number of terms */
    Array *terms;			/* the terms */
};

struct mesharray {			/* product array structure */
    struct array array;			/* array class preamble */
    int nterms;				/* number of terms */
    Array *terms;			/* the terms */
};

struct constantarray {			/* constant array structure */
    struct array array;			/* array class preamble */
    Pointer data;			/* constant value */
};

Array _dxf_NewArrayV(Type type, Category category, int rank, int *shape,
		 struct array_class *class);


#if 0 /* was if OPTIMIZED */
#define CHECKARRAY(obj, cls) { \
    if (!obj) \
	return ERROR; \
}
#else
#define CHECKARRAY(obj, cls) { \
    if (!obj) \
        return ERROR; \
    if (DXGetArrayClass((Array)(obj)) != cls) \
        DXErrorReturn(ERROR_BAD_CLASS, "called with array of wrong class"); \
}
#endif
/**
This macro eases the task of checking argument class.  Note: This is not needed
when a method implementation is called, because {\tt o} and its class will
both have been checked by the method.
**/

/*
 * These macros are used by GetArrayData for the compact arrays
 * (RegularArray, ProductArray, PathArray, MeshArray) to lock the
 * arrays during expansion so that only one processor at a time
 * does it, avoiding memory leaks.
 */

#define EXPAND_LOCK(a) {				\
    if (a->array.data) {				\
	DXsyncmem();					\
	return a->array.data;				\
    }							\
    DXlock(&a->array.inprogress, DXProcessorId());	\
    if (a->array.data) {				\
	DXunlock(&a->array.inprogress, DXProcessorId());\
	DXsyncmem();					\
	return a->array.data;				\
    }							\
}

#define EXPAND_RETURN(a, result) {			\
    a->array.data = (Pointer)(result);			\
    DXunlock(&a->array.inprogress, DXProcessorId());	\
    return (Pointer)(result);				\
}

#define EXPAND_ERROR(a) {				\
    DXunlock(&a->array.inprogress, DXProcessorId());	\
    return NULL;					\
}
