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
#ifndef __groupClass
#define __groupClass
/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
\section{Group class}
*/



extern struct group_class _dxdgroup_class;
Error _dxfGroup_Delete(Group);
Object _dxfGroup_GetType(Group, Type*, Category*, int*, int*);
Object _dxfGroup_Copy(Group, enum _dxd_copy);
Object _dxfGroup_BoundingBox(Group, Point*, Matrix*, int);
Object _dxfGroup_Gather(Group, struct gather *, struct tile *);
Object _dxfGroup_Paint(Group, struct buffer *, int, struct tile *);
Error _dxfGroup_Shade(Group, struct shade *);
Group _dxfSetMember(Group, char *, Object);
Group _dxfno_SetMember(Group, char *, Object);
Group _dxfSetEnumeratedMember(Group, int, Object);
Group _dxfno_SetEnumeratedMember(Group, int, Object);
Group _dxfGroup_SetMember(Group, char *, Object);
Group _dxfGroup_SetEnumeratedMember(Group, int, Object);


extern struct series_class _dxdseries_class;
Group _dxfSeries_SetMember(Series, char *, Object);
Group _dxfSeries_SetEnumeratedMember(Series, int, Object);
Object _dxfSeries_Copy(Series, enum _dxd_copy);


extern struct compositefield_class _dxdcompositefield_class;
Group _dxfCompositeField_SetMember(CompositeField, char *, Object);
Group _dxfCompositeField_SetEnumeratedMember(CompositeField, int, Object);
Object _dxfCompositeField_BoundingBox(CompositeField, Point*, Matrix*, int);


extern struct multigrid_class _dxdmultigrid_class;
Group _dxfMultiGrid_SetMember(MultiGrid, char *, Object);
Group _dxfMultiGrid_SetEnumeratedMember(MultiGrid, int, Object);

struct group {

    struct object object;	/* object preamble */
    lock_type lock;		/* for Set(Enumerated)Member only */

    /* the members */
    struct member {		/* the members */
	char *name;		/* the member name */
	Object value;		/* the member */
	float position;		/* position on series axis for series group */
    } *members;			/* the members */
    int nmembers;		/* number of members */
    int alloc;			/* space allocated */

    /* SetGroupType()/GetType() */
    int typed;			/* whether type was set */
    Type type;			/* the type */
    Category category;		/* the category */
    int rank;			/* rank */
    int *shape;			/* shape */
};

struct series {			/* series object */
    struct group group;		/* group preamble */
};

struct compositefield {		/* composite field object */
    struct group group;		/* group preamble */
};

struct multigrid {		/* multigrid object */
    struct group group;		/* group preamble */
};

Group _dxf_NewGroup(struct group_class *class);

Group _dxf_SetEnumeratedMember(Group g, int n, double position, Object value);
/**
Internal routine to support both {\tt SetEnumeratedMember()} and
{\tt SetSeriesMember()}.
**/

Object _dxf_GetEnumeratedMember(Group g, int n, float *position, char **name);
/**
Internal routine to support both {\tt GetEnumeratedMember()} and
{\tt GetSeriesMember()}.
**/

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

struct group_class {
    struct object_class super;
    Class class;
    char *name;
    Group (*SetMember)();
    Group (*SetEnumeratedMember)();
};

Group _dxfno_SetMember(Group a, char * b, Object c) {
    char *name;
    name = (*(struct group_class **)a)->name;
    DXSetError(ERROR_BAD_PARAMETER, "#12130", "DXSetMember", name);
    return NULL;
}

Group _dxfno_SetEnumeratedMember(Group a, int b, Object c) {
    char *name;
    name = (*(struct group_class **)a)->name;
    DXSetError(ERROR_BAD_PARAMETER, "#12130", "DXSetEnumeratedMember", name);
    return NULL;
}

struct group_class _dxdgroup_class = {
    sizeof(struct group),
    CLASS_GROUP,
    "group",
    CLASS_GROUP,
    "group",
    _dxfGroup_Delete,
    _dxfGroup_Shade,
    _dxfGroup_BoundingBox,
    _dxfGroup_Paint,
    _dxfGroup_Gather,
    _dxfno_Partition,
    _dxfGroup_GetType,
    _dxfGroup_Copy,
    CLASS_GROUP,
    "group",
    _dxfGroup_SetMember,
    _dxfGroup_SetEnumeratedMember,
};

Group _dxfSetMember(Group a, char * b, Object c) {
    if (!a)
        return NULL;
    return (*(struct group_class **)a)->SetMember((Group)a,b,c);
}

Group _dxfSetEnumeratedMember(Group a, int b, Object c) {
    if (!a)
        return NULL;
    return (*(struct group_class **)a)->SetEnumeratedMember((Group)a,b,c);
}

Class DXGetGroupClass(Group o) {
    return o? (*(struct group_class **)o)->class: CLASS_MIN;
}

struct series_class {
    struct group_class super;
    Class class;
    char *name;
};

struct series_class _dxdseries_class = {
    sizeof(struct series),
    CLASS_SERIES,
    "series",
    CLASS_GROUP,
    "group",
    _dxfGroup_Delete,
    _dxfGroup_Shade,
    _dxfGroup_BoundingBox,
    _dxfGroup_Paint,
    _dxfGroup_Gather,
    _dxfno_Partition,
    _dxfGroup_GetType,
    _dxfSeries_Copy,
    CLASS_SERIES,
    "series",
    _dxfSeries_SetMember,
    _dxfSeries_SetEnumeratedMember,
    CLASS_SERIES,
    "series",
};

Class DXGetSeriesClass(Series o) {
    return o? (*(struct series_class **)o)->class: CLASS_MIN;
}

struct compositefield_class {
    struct group_class super;
    Class class;
    char *name;
};

struct compositefield_class _dxdcompositefield_class = {
    sizeof(struct compositefield),
    CLASS_COMPOSITEFIELD,
    "compositefield",
    CLASS_GROUP,
    "group",
    _dxfGroup_Delete,
    _dxfGroup_Shade,
    _dxfCompositeField_BoundingBox,
    _dxfGroup_Paint,
    _dxfGroup_Gather,
    _dxfno_Partition,
    _dxfGroup_GetType,
    _dxfGroup_Copy,
    CLASS_COMPOSITEFIELD,
    "compositefield",
    _dxfCompositeField_SetMember,
    _dxfCompositeField_SetEnumeratedMember,
    CLASS_COMPOSITEFIELD,
    "compositefield",
};

Class DXGetCompositeFieldClass(CompositeField o) {
    return o? (*(struct compositefield_class **)o)->class: CLASS_MIN;
}

struct multigrid_class {
    struct group_class super;
    Class class;
    char *name;
};

struct multigrid_class _dxdmultigrid_class = {
    sizeof(struct multigrid),
    CLASS_MULTIGRID,
    "multigrid",
    CLASS_GROUP,
    "group",
    _dxfGroup_Delete,
    _dxfGroup_Shade,
    _dxfGroup_BoundingBox,
    _dxfGroup_Paint,
    _dxfGroup_Gather,
    _dxfno_Partition,
    _dxfGroup_GetType,
    _dxfGroup_Copy,
    CLASS_MULTIGRID,
    "multigrid",
    _dxfMultiGrid_SetMember,
    _dxfMultiGrid_SetEnumeratedMember,
    CLASS_MULTIGRID,
    "multigrid",
};

Class DXGetMultiGridClass(MultiGrid o) {
    return o? (*(struct multigrid_class **)o)->class: CLASS_MIN;
}

