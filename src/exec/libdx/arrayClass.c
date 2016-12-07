/*
 * Automatically generated - DO NOT EDIT!
 */

#ifndef __objectClass
#define __objectClass
/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

/*
\section{Object class}

Every object begins with an object preamble, which contains the class
number and a reference count.
*/

#include <dx/dx.h>

/* the following make ANSI compilers happier */
struct shade;
struct buffer;
struct tile;
struct gather;
struct survey;
struct count;

struct _root {
    int size;
    Class class;
    char *name;
};
#define CLASS_SIZE(x) (((struct _root *)(x))->size)
#define CLASS_CLASS(x) (((struct _root *)(x))->class)
#define CLASS_NAME(x) (((struct _root *)(x))->name)


extern struct object_class _dxdobject_class;
Error _dxfDelete(Object);
Error _dxfno_Delete(Object);
Error _dxfShade(Object, struct shade *);
Error _dxfno_Shade(Object, struct shade *);
Object _dxfBoundingBox(Object, Point*, Matrix*, int);
Object _dxfno_BoundingBox(Object, Point*, Matrix*, int);
Object _dxfPaint(Object, struct buffer *, int, struct tile *);
Object _dxfno_Paint(Object, struct buffer *, int, struct tile *);
Object _dxfGather(Object, struct gather *, struct tile *);
Object _dxfno_Gather(Object, struct gather *, struct tile *);
Error _dxfPartition(Object, int*, int, Object*, int);
Error _dxfno_Partition(Object, int*, int, Object*, int);
Object _dxfGetType(Object, Type*, Category*, int*, int*);
Object _dxfno_GetType(Object, Type*, Category*, int*, int*);
Object _dxfCopy(Object, enum _dxd_copy);
Object _dxfno_Copy(Object, enum _dxd_copy);
Error _dxfObject_Delete(Object);
Object _dxfObject_BoundingBox(Object, Point*, Matrix*, int);
Error _dxfObject_Shade(Object, struct shade *);

#define NATTRIBUTES 2			/* number of attributes in object */

struct object {				/* object preamble */
    struct object_class *class;		/* class vector */
    Class class_id;			/* class id (for debugging only!) */
    lock_type lock;			/* for Reference and Delete */
    int count;				/* reference count */
    int tag;				/* object tag */
    struct attribute {			/* object attributes */
	char *name;			/* attribute name */
	Object value;			/* attribue value */
    } local[NATTRIBUTES], *attributes;	/* the attributes */
    int nattributes;			/* number of attributes */
    int attr_alloc;			/* allocated space for attributes */
};

#if 0 /* was if OPTIMIZED */
#define CHECK(obj, cls) { \
    if (!obj) \
	return ERROR; \
}
#else
#define CHECK(obj, cls) { \
    if (!obj) \
        return ERROR; \
    if (DXGetObjectClass((Object)(obj)) != cls) \
        DXErrorReturn(ERROR_BAD_CLASS, "called with object of wrong class"); \
}
#endif
/**
This macro eases the task of checking argument class.  Note: This is not needed
when a method implementation is called, because {\tt o} and its class will
both have been checked by the method.
**/

Object _dxf_NewObject(struct object_class *class);
/**
This internal routine is called only by other {\tt New...} routines to
create and initialize the object preamble.
**/

Object _dxf_CopyObject(Object new, Object old, enum _dxd_copy copy);
/**
Copies the portion of the data of {\tt old} managed by the {\tt
Object} class to {\tt new}.  This is provided for subclasses of {\tt Object}
to use in their copy routines.  Copying works something like creating
an object.  Every class {\tt X} that implements copying should provide
an {\tt \_CopyX} routine that copies relevant data from an old object
to a new object, so that subclass copy routines may call this routine
to copy the superclass's data.  The {\tt CopyX} routine just creates a
new object of the appropriate type and then calls {\tt \_CopyX} to copy
the data.
**/
#endif
#ifndef __arrayClass
#define __arrayClass
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



extern struct array_class _dxdarray_class;
Pointer _dxfGetArrayData(Array);
Pointer _dxfno_GetArrayData(Array);
Error _dxfArray_Delete(Array);
Object _dxfArray_GetType(Array, Type*, Category*, int*, int*);
Pointer _dxfArray_GetArrayData(Array);
Object _dxfArray_Copy(Array, enum _dxd_copy);


extern struct sharedarray_class _dxdsharedarray_class;
Error _dxfSharedArray_Delete(SharedArray);
Pointer _dxfSharedArray_GetArrayData(SharedArray);


extern struct regulararray_class _dxdregulararray_class;
Error _dxfRegularArray_Delete(RegularArray);
Pointer _dxfRegularArray_GetArrayData(RegularArray);


extern struct patharray_class _dxdpatharray_class;
Pointer _dxfPathArray_GetArrayData(PathArray);


extern struct productarray_class _dxdproductarray_class;
Error _dxfProductArray_Delete(ProductArray);
Pointer _dxfProductArray_GetArrayData(ProductArray);


extern struct mesharray_class _dxdmesharray_class;
Error _dxfMeshArray_Delete(MeshArray);
Pointer _dxfMeshArray_GetArrayData(MeshArray);


extern struct constantarray_class _dxdconstantarray_class;
Pointer _dxfConstantArray_GetArrayData(ConstantArray);

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
#endif

struct object_class {
    struct _root root;
    Class class;
    char *name;
    Error (*Delete)();
    Error (*Shade)();
    Object (*BoundingBox)();
    Object (*Paint)();
    Object (*Gather)();
    Error (*Partition)();
    Object (*GetType)();
    Object (*Copy)();
};

struct array_class {
    struct object_class super;
    Class class;
    char *name;
    Pointer (*GetArrayData)();
};

Pointer _dxfno_GetArrayData(Array a) {
    char *name;
    name = (*(struct array_class **)a)->name;
    DXSetError(ERROR_BAD_PARAMETER, "#12130", "DXGetArrayData", name);
    return NULL;
}

struct array_class _dxdarray_class = {
    sizeof(struct array),
    CLASS_ARRAY,
    "array",
    CLASS_ARRAY,
    "array",
    _dxfArray_Delete,
    _dxfObject_Shade,
    _dxfObject_BoundingBox,
    _dxfno_Paint,
    _dxfno_Gather,
    _dxfno_Partition,
    _dxfArray_GetType,
    _dxfArray_Copy,
    CLASS_ARRAY,
    "array",
    _dxfArray_GetArrayData,
};

Pointer _dxfGetArrayData(Array a) {
    if (!a)
        return NULL;
    return (*(struct array_class **)a)->GetArrayData((Array)a);
}

Class DXGetArrayClass(Array o) {
    return o? (*(struct array_class **)o)->class: CLASS_MIN;
}

struct sharedarray_class {
    struct array_class super;
    Class class;
    char *name;
};

struct sharedarray_class _dxdsharedarray_class = {
    sizeof(struct sharedarray),
    CLASS_SHAREDARRAY,
    "sharedarray",
    CLASS_ARRAY,
    "array",
    _dxfSharedArray_Delete,
    _dxfObject_Shade,
    _dxfObject_BoundingBox,
    _dxfno_Paint,
    _dxfno_Gather,
    _dxfno_Partition,
    _dxfArray_GetType,
    _dxfArray_Copy,
    CLASS_SHAREDARRAY,
    "sharedarray",
    _dxfSharedArray_GetArrayData,
    CLASS_SHAREDARRAY,
    "sharedarray",
};

Class DXGetSharedArrayClass(SharedArray o) {
    return o? (*(struct sharedarray_class **)o)->class: CLASS_MIN;
}

struct regulararray_class {
    struct array_class super;
    Class class;
    char *name;
};

struct regulararray_class _dxdregulararray_class = {
    sizeof(struct regulararray),
    CLASS_REGULARARRAY,
    "regulararray",
    CLASS_ARRAY,
    "array",
    _dxfRegularArray_Delete,
    _dxfObject_Shade,
    _dxfObject_BoundingBox,
    _dxfno_Paint,
    _dxfno_Gather,
    _dxfno_Partition,
    _dxfArray_GetType,
    _dxfArray_Copy,
    CLASS_REGULARARRAY,
    "regulararray",
    _dxfRegularArray_GetArrayData,
    CLASS_REGULARARRAY,
    "regulararray",
};

Class DXGetRegularArrayClass(RegularArray o) {
    return o? (*(struct regulararray_class **)o)->class: CLASS_MIN;
}

struct patharray_class {
    struct array_class super;
    Class class;
    char *name;
};

struct patharray_class _dxdpatharray_class = {
    sizeof(struct patharray),
    CLASS_PATHARRAY,
    "patharray",
    CLASS_ARRAY,
    "array",
    _dxfArray_Delete,
    _dxfObject_Shade,
    _dxfObject_BoundingBox,
    _dxfno_Paint,
    _dxfno_Gather,
    _dxfno_Partition,
    _dxfArray_GetType,
    _dxfArray_Copy,
    CLASS_PATHARRAY,
    "patharray",
    _dxfPathArray_GetArrayData,
    CLASS_PATHARRAY,
    "patharray",
};

Class DXGetPathArrayClass(PathArray o) {
    return o? (*(struct patharray_class **)o)->class: CLASS_MIN;
}

struct productarray_class {
    struct array_class super;
    Class class;
    char *name;
};

struct productarray_class _dxdproductarray_class = {
    sizeof(struct productarray),
    CLASS_PRODUCTARRAY,
    "productarray",
    CLASS_ARRAY,
    "array",
    _dxfProductArray_Delete,
    _dxfObject_Shade,
    _dxfObject_BoundingBox,
    _dxfno_Paint,
    _dxfno_Gather,
    _dxfno_Partition,
    _dxfArray_GetType,
    _dxfArray_Copy,
    CLASS_PRODUCTARRAY,
    "productarray",
    _dxfProductArray_GetArrayData,
    CLASS_PRODUCTARRAY,
    "productarray",
};

Class DXGetProductArrayClass(ProductArray o) {
    return o? (*(struct productarray_class **)o)->class: CLASS_MIN;
}

struct mesharray_class {
    struct array_class super;
    Class class;
    char *name;
};

struct mesharray_class _dxdmesharray_class = {
    sizeof(struct mesharray),
    CLASS_MESHARRAY,
    "mesharray",
    CLASS_ARRAY,
    "array",
    _dxfMeshArray_Delete,
    _dxfObject_Shade,
    _dxfObject_BoundingBox,
    _dxfno_Paint,
    _dxfno_Gather,
    _dxfno_Partition,
    _dxfArray_GetType,
    _dxfArray_Copy,
    CLASS_MESHARRAY,
    "mesharray",
    _dxfMeshArray_GetArrayData,
    CLASS_MESHARRAY,
    "mesharray",
};

Class DXGetMeshArrayClass(MeshArray o) {
    return o? (*(struct mesharray_class **)o)->class: CLASS_MIN;
}

struct constantarray_class {
    struct array_class super;
    Class class;
    char *name;
};

struct constantarray_class _dxdconstantarray_class = {
    sizeof(struct constantarray),
    CLASS_CONSTANTARRAY,
    "constantarray",
    CLASS_ARRAY,
    "array",
    _dxfArray_Delete,
    _dxfObject_Shade,
    _dxfObject_BoundingBox,
    _dxfno_Paint,
    _dxfno_Gather,
    _dxfno_Partition,
    _dxfArray_GetType,
    _dxfArray_Copy,
    CLASS_CONSTANTARRAY,
    "constantarray",
    _dxfConstantArray_GetArrayData,
    CLASS_CONSTANTARRAY,
    "constantarray",
};

Class DXGetConstantArrayClass(ConstantArray o) {
    return o? (*(struct constantarray_class **)o)->class: CLASS_MIN;
}

