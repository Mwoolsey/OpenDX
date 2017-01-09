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
#ifndef __interpClass
#define __interpClass
/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/





extern struct interpolator_class _dxdinterpolator_class;
Error _dxfInterpolate(Interpolator, int *, float **, Pointer *, Interpolator *, int);
Error _dxfno_Interpolate(Interpolator, int *, float **, Pointer *, Interpolator *, int);
Interpolator _dxfLocalizeInterpolator(Interpolator);
Interpolator _dxfno_LocalizeInterpolator(Interpolator);
Error _dxfInterpolator_Delete(Interpolator);
Object _dxfInterpolator_GetType(Interpolator, Type*, Category*, int*, int*);

/*
 * Fuzz factor for interpolation: interpolators should assume points
 * within this proportion of an estimated primitive edge length from a
 * primitive actually lie within that primitive.
 */
#define FUZZ		0.0003
#define MAX_DIM		8

struct interpolator
{
    struct object	object;
    Object		dataObject;
    Object		rootObject;
    float		min[MAX_DIM];
    float		max[MAX_DIM];
    int			nDim;
    Type		type;
    Category		category;
    int			rank, shape[32];
    float		fuzz;
    Matrix		matrix;
};

Interpolator	_dxfNewInterpolatorSwitch(Object,
				enum interp_init, float, Matrix *);

Interpolator	_dxf_NewInterpolator(struct interpolator_class *, Object);
				
Interpolator	_dxf_CopyInterpolator(Interpolator, Interpolator);

#define FUZZ_ON	 1
#define FUZZ_OFF 0
#endif
#ifndef __groupinterpClass
#define __groupinterpClass
/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/



typedef struct groupinterpolator *GroupInterpolator;



extern struct groupinterpolator_class _dxdgroupinterpolator_class;
Error _dxfGroupInterpolator_Interpolate(GroupInterpolator, int *, float **, Pointer *, Interpolator *, int);
Error _dxfGroupInterpolator_Delete(GroupInterpolator);
Object _dxfGroupInterpolator_Copy(GroupInterpolator, enum _dxd_copy);
Interpolator _dxfGroupInterpolator_LocalizeInterpolator(GroupInterpolator);

GroupInterpolator NewGroupInterpolator(Group, enum interp_init, float fuzz);

struct groupinterpolator
{
    struct interpolator	interpolator;
    int			nMembers;
    Interpolator	*subInterp;
    Interpolator	hint;
};

GroupInterpolator _dxfNewGroupInterpolator(Group,
			enum interp_init, float, Matrix *);

GroupInterpolator _dxf_NewGroupInterpolator(Group, enum interp_init,
			float, Matrix *, struct groupinterpolator_class *);

GroupInterpolator _dxf_CopyGroupInterpolator(GroupInterpolator,
			GroupInterpolator, enum _dxd_copy copy);
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

struct interpolator_class {
    struct object_class super;
    Class class;
    char *name;
    Error (*Interpolate)();
    Interpolator (*LocalizeInterpolator)();
};

struct groupinterpolator_class {
    struct interpolator_class super;
    Class class;
    char *name;
};

struct groupinterpolator_class _dxdgroupinterpolator_class = {
    sizeof(struct groupinterpolator),
    CLASS_GROUPINTERPOLATOR,
    "groupinterpolator",
    CLASS_INTERPOLATOR,
    "interpolator",
    _dxfGroupInterpolator_Delete,
    _dxfObject_Shade,
    _dxfObject_BoundingBox,
    _dxfno_Paint,
    _dxfno_Gather,
    _dxfno_Partition,
    _dxfInterpolator_GetType,
    _dxfGroupInterpolator_Copy,
    CLASS_GROUPINTERPOLATOR,
    "groupinterpolator",
    _dxfGroupInterpolator_Interpolate,
    _dxfGroupInterpolator_LocalizeInterpolator,
    CLASS_GROUPINTERPOLATOR,
    "groupinterpolator",
};

Class DXGetGroupInterpolatorClass(GroupInterpolator o) {
    return o? (*(struct groupinterpolator_class **)o)->class: CLASS_MIN;
}

