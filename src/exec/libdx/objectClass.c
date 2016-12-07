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

Error _dxfno_Delete(Object a) {
    char *name;
    name = (*(struct object_class **)a)->name;
    DXSetError(ERROR_BAD_PARAMETER, "#12130", "DXDelete", name);
    return ERROR;
}

Error _dxfno_Shade(Object a, struct shade * b) {
    char *name;
    name = (*(struct object_class **)a)->name;
    DXSetError(ERROR_BAD_PARAMETER, "#12130", "DXShade", name);
    return ERROR;
}

Object _dxfno_BoundingBox(Object a, Point* b, Matrix* c, int d) {
    char *name;
    name = (*(struct object_class **)a)->name;
    DXSetError(ERROR_BAD_PARAMETER, "#12130", "DXBoundingBox", name);
    return NULL;
}

Object _dxfno_Paint(Object a, struct buffer * b, int c, struct tile * d) {
    char *name;
    name = (*(struct object_class **)a)->name;
    DXSetError(ERROR_BAD_PARAMETER, "#12130", "DXPaint", name);
    return NULL;
}

Object _dxfno_Gather(Object a, struct gather * b, struct tile * c) {
    char *name;
    name = (*(struct object_class **)a)->name;
    DXSetError(ERROR_BAD_PARAMETER, "#12130", "DXGather", name);
    return NULL;
}

Error _dxfno_Partition(Object a, int* b, int c, Object* d, int e) {
    char *name;
    name = (*(struct object_class **)a)->name;
    DXSetError(ERROR_BAD_PARAMETER, "#12130", "DXPartition", name);
    return ERROR;
}

Object _dxfno_GetType(Object a, Type* b, Category* c, int* d, int* e) {
    char *name;
    name = (*(struct object_class **)a)->name;
    DXSetError(ERROR_BAD_PARAMETER, "#12130", "DXGetType", name);
    return NULL;
}

Object _dxfno_Copy(Object a, enum _dxd_copy b) {
    char *name;
    name = (*(struct object_class **)a)->name;
    DXSetError(ERROR_BAD_PARAMETER, "#12130", "DXCopy", name);
    return NULL;
}

struct object_class _dxdobject_class = {
    sizeof(struct object),
    CLASS_OBJECT,
    "object",
    CLASS_OBJECT,
    "object",
    _dxfObject_Delete,
    _dxfObject_Shade,
    _dxfObject_BoundingBox,
    _dxfno_Paint,
    _dxfno_Gather,
    _dxfno_Partition,
    _dxfno_GetType,
    _dxfno_Copy,
};

Error _dxfDelete(Object a) {
    if (!a)
        return ERROR;
    return (*(struct object_class **)a)->Delete((Object)a);
}

Error _dxfShade(Object a, struct shade * b) {
    if (!a)
        return ERROR;
    return (*(struct object_class **)a)->Shade((Object)a,b);
}

Object _dxfBoundingBox(Object a, Point* b, Matrix* c, int d) {
    if (!a)
        return NULL;
    return (*(struct object_class **)a)->BoundingBox((Object)a,b,c,d);
}

Object _dxfPaint(Object a, struct buffer * b, int c, struct tile * d) {
    if (!a)
        return NULL;
    return (*(struct object_class **)a)->Paint((Object)a,b,c,d);
}

Object _dxfGather(Object a, struct gather * b, struct tile * c) {
    if (!a)
        return NULL;
    return (*(struct object_class **)a)->Gather((Object)a,b,c);
}

Error _dxfPartition(Object a, int* b, int c, Object* d, int e) {
    if (!a)
        return ERROR;
    return (*(struct object_class **)a)->Partition((Object)a,b,c,d,e);
}

Object _dxfGetType(Object a, Type* b, Category* c, int* d, int* e) {
    if (!a)
        return NULL;
    return (*(struct object_class **)a)->GetType((Object)a,b,c,d,e);
}

Object _dxfCopy(Object a, enum _dxd_copy b) {
    if (!a)
        return NULL;
    return (*(struct object_class **)a)->Copy((Object)a,b);
}

Class DXGetObjectClass(Object o) {
    return o? (*(struct object_class **)o)->class: CLASS_MIN;
}

