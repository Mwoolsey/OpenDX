/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/


#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#define private __private__
#endif

#ifndef _DXI_OBJECT_H_
#define _DXI_OBJECT_H_

/* TeX starts here. Do not remove this comment. */

/*
\section{Object class}
\label{objectsec}
\paragraph{Type definitions.}
An object is represented by a pointer to a C structure stored in global
memory.  The content of the structure is private to the implementation.
A {\tt typedef} is provided for each class of object:
*/
typedef struct object *Object;
typedef struct private *Private;
typedef struct string *String;
typedef struct field *Field;
typedef struct group *Group;
typedef struct series *Series;
typedef struct compositefield *CompositeField;
typedef struct array *Array;
typedef struct sharedarray *SharedArray;
typedef struct regulararray *RegularArray;
typedef struct constantarray *ConstantArray;
typedef struct patharray *PathArray;
typedef struct productarray *ProductArray;
typedef struct mesharray *MeshArray;
typedef struct interpolator *Interpolator;
typedef struct xform *Xform;
typedef struct screen *Screen;
typedef struct clipped *Clipped;
typedef struct camera *Camera;
typedef struct light *Light;
typedef struct multigrid *MultiGrid;
/*
An {\tt enum} provides a number for each class and subclass:
*/
typedef enum {
    CLASS_MIN,
    CLASS_OBJECT,
    CLASS_PRIVATE,
    CLASS_STRING,
    CLASS_FIELD,
    CLASS_GROUP,
    CLASS_SERIES,
    CLASS_COMPOSITEFIELD,
    CLASS_ARRAY,
    CLASS_REGULARARRAY,
    CLASS_PATHARRAY,
    CLASS_PRODUCTARRAY,
    CLASS_MESHARRAY,
    CLASS_INTERPOLATOR,
    CLASS_FIELDINTERPOLATOR,
    CLASS_GROUPINTERPOLATOR,
    CLASS_LINESRR1DINTERPOLATOR,
    CLASS_LINESII1DINTERPOLATOR,
    CLASS_LINESRI1DINTERPOLATOR,
    CLASS_QUADSRR2DINTERPOLATOR,
    CLASS_QUADSII2DINTERPOLATOR,
    CLASS_TRISRI2DINTERPOLATOR,
    CLASS_CUBESRRINTERPOLATOR,
    CLASS_CUBESIIINTERPOLATOR,
    CLASS_TETRASINTERPOLATOR,
    CLASS_FLE2DINTERPOLATOR,
    CLASS_GROUPITERATOR,
    CLASS_ITEMITERATOR,
    CLASS_XFORM,
    CLASS_SCREEN,
    CLASS_CLIPPED,
    CLASS_CAMERA,
    CLASS_LIGHT,
    CLASS_CONSTANTARRAY,
    CLASS_MULTIGRID,
    CLASS_SHAREDARRAY,
    CLASS_MAX,
    CLASS_DELETED
} Class;

/*
\paragraph{Classes and subclasses.} Subclasses are handled as
follows, taking the {\tt Group} class as an example.  The {\tt
DXGetObjectClass()} routine returns the subclass of {\tt Object} ({\tt
CLASS\_GROUP}, for example) so that a user whose operation does not
depend on the specific subclass is not affected.  For each class that
is subclassed, there is a {\tt Get...Class()} routine ({\tt
DXGetGroupClass()}, for example) to determine the subclass.  Objects of
the superclass ({\tt Group}, for example) may still be created and
manipulated using the generic superclass manipulation routines ({\tt
DXNewGroup()} and {\tt DXSetMember()} for example).  In such a case, the
{\tt Get...Class()} routine will return the name of the superclass;
thus, for example, {\tt DXGetGroupClass()} may return {\tt CLASS\_GROUP}
if the object is a generic group, or {\tt CLASS\_COMPOSITEFIELD} etc.
if it is an object of one of the subclasses of the group class.  The
subclass identifiers ({\tt CLASS\_COMPOSITEFIELD etc.}) are in the
same {\tt enum} as the class identifiers ({\tt CLASS\_GROUP}, etc.).

\paragraph{Object routines.}
There are a number of routines common to all objects.  These include
routines for reserving a reference to an object, for deleting an
object, and for saving an object to and restoring it from a file.
*/

Class DXGetObjectClass(Object o);
/**
\index{DXGetObjectClass}
Returns the class of an object, to be interpreted according to the
definitions given above.
**/

Object DXReference(Object o);
/**
\index{DXReference}
Indicates that there is a reference to object {\tt o}.  The object is
guaranteed to not be deleted until this reference is released, via the
{\tt DXDelete} routine.  When an object is first created, it has zero
references.  Returns {\tt o}, or returns null and sets the
error code to indicate an error. 
**/

Error DXDelete(Object o);
/**
\index{DXDelete}
Indicates that a previous reference to object {\tt o} (signified by
{\tt DXReference()}) no longer exists.  When no further references to an
object exist, all storage associated with the object will be deleted.
Returns {\tt OK} to indicate success, or returns {\tt ERROR} and sets
the error code to indicate an error.
**/

Error DXUnreference(Object o);
/**
\index{DXUnreference}
Special routine -- DXDelete is the normal way to remove a reference to
an object.  If the reference count should be decremented to zero but
the object should NOT be deleted, this routine can be used instead of
DXDelete.
**/

int DXGetObjectTag(Object o);
Object DXSetObjectTag(Object o, int tag);
/**
\index{DXGetObjectTag}\index{DXSetObjectTag}
Every object is assigned a unique non-zero integer tag when it is
created.  In addition, the executive sets the object tag of objects it
knows about by using {\tt DXSetObjectTag()}.  This tag is used, for
example, by the cache system to identify objects.  {\tt DXGetObjectTag()}
returns the object identifier, or returns 0 and sets the error code
to indicate an error.
**/

Object DXSetAttribute(Object o, char *name, Object value);
/**
\index{DXSetAttribute}
Every object may have associated with it a set of {\em attributes}, in
the form of name/value pairs.  This routine sets attribute {\tt name}
for object {\tt o} to {\tt value}.  Returns {\tt o} on success, or returns null
and sets the error code to indicate an error.
**/

Object DXDeleteAttribute(Object o, char *name);
/**
\index{DXDeleteAttribute}
This routine removes the attribute {\tt name}
from object {\tt o}. Returns {\tt o} on success or if the
attribute does not exist, or returns null
and sets the error code to indicate an error.
**/

Object DXGetAttribute(Object o, char *name);
Object DXGetEnumeratedAttribute(Object o, int n, char **name);
/**
\index{DXGetAttribute}\index{DXGetEnumeratedAttribute}
{\tt DXGetAttribute} retrieves the attribute specified by {\tt name}
from object {\tt o}.  {\tt DXGetEnumeratedAttribute} retrieves the {\tt
n}th attribute of object {\tt o}, and returns a pointer its name in
{\tt name}.  Returns {\tt o} on success, or returns null but does not
set the error code if the attribute does not exist.
**/

Object DXSetFloatAttribute(Object o, char *name, double x);
Object DXSetIntegerAttribute(Object o, char *name, int x);
Object DXSetStringAttribute(Object o, char *name, char *x);
/**
\index{DXSetFloatAttribute}\index{DXSetIntegerAttribute}\index{DXSetStringAttribute}
These routines provide an easy way to associate a floating point, integer,
or string attribute {\tt x} named {\tt name} with object {\tt o}.  The
float and integer attributes are represented by arrays; the string attribute
is represented by a string object.  Returns {\tt o} on success, or returns
null and sets the error code to indicate an error.
**/

Object DXGetFloatAttribute(Object o, char *name, float *x);
Object DXGetIntegerAttribute(Object o, char *name, int *x);
Object DXGetStringAttribute(Object o, char *name, char **x);
/**
\index{DXGetFloatAttribute}\index{DXGetIntegerAttribute}\index{DXGetStringAttribute}
These routines provide an easy way to retrieve a floating point, integer,
or string attribute {\tt x} named {\tt name} from object {\tt o}.  
If the attribute exists, these routines set pointer {\tt x} to the value.
Returns {\tt o} on success, or returns null but does not set the error code.
**/

Object DXCopyAttributes(Object dst, Object src);
/**
\index{DXCopyAttributes}
For each attribute in {\tt src}, that attribute in {\tt dst} is set to
the value from {\tt src}, overriding any setting it may have in {\tt
dst}.  Attributes that are present in {\tt dst} but not present in
{\tt src} remain unchanged. Returns {\tt dst} on success, or returns
null and sets the error code to indicate an error.
**/

enum _dxd_copy {
    COPY_ATTRIBUTES,
    COPY_HEADER,
    COPY_STRUCTURE,
    COPY_DATA
};

Object DXCopy(Object o, enum _dxd_copy copy);
/**
\index{DXCopy}
A variety of object copying operations are provided, differing in the
depth to which they copy the structure of an object.  The depth is
specified by the {\tt copy} parameter, which may be one of the
following (at present, only {\tt COPY\_STRUCTURE} is supported):

{\tt COPY\_DATA}: Copies the entire data structure, including all headers,
attributes, members, components, and arrays (including array data).
Note: this is not implemented in the current version of the Data Explorer.
\marginpar{Is this true?}

{\tt COPY\_STRUCTURE}: For {\tt Group}s, this copies the group header and
recursively copies all group members.  For {\tt Field}s, it copies the field
header but {\em does not} copy the components (which are generally arrays); 
rather if puts references to the components of the given
object into the resulting field.

{\tt COPY\_HEADER}: Copies only the header of the immediate object; {\em does 
not} recursively copy the attributes, members, components, and so on; rather 
it puts references to them into the new object.

{\tt COPY\_ATTRIBUTES}: Creates a new object of the same type as the
old, and copies all attributes and type information, but does {\em
not} put references to members, components, and so on in the new object.

Returns the copy, or returns null and sets the error code to indicate an
error.
**/


Object DXEndObject(Object o);
/**
\index{DXEndObject}
This routine does any standard processing needed after an object is
created.  It currently only called DXEndField on all field objects,
but preserves shared components if the object is a group of fields
which share components like positions or connections.
**/


/*
\paragraph{Types.}
Several object classes implement a notion of type.  For {\tt Array}s,
this is the same as the type set when the array was created, and is
the same as the information returned by {\tt DXGetArrayInfo}.  (See
section \ref{arraysec}.)  For {\tt Field}s, this is the same as the
type of the ``data'' component, if there is a data component.  For 
{\tt Group}s, this is the type set explicitly by {\tt DXSetGroupType()} or 
implicitly for series groups or composite groups,
*/

typedef enum {
    TYPE_BYTE,
    TYPE_UBYTE,
    TYPE_USHORT,
    TYPE_UINT,
    TYPE_SHORT,
    TYPE_INT,
    TYPE_HYPER,
    TYPE_FLOAT,
    TYPE_DOUBLE,
    TYPE_STRING
} Type;

typedef enum {
    CATEGORY_REAL,
    CATEGORY_COMPLEX,
    CATEGORY_QUATERNION
} Category;

Object DXGetType(Object o, Type *t, Category *c, int *rank, int *shape);
/**
\index{DXGetType}
If {\tt t} is not null, this routine returns the type of {\tt g} in {\tt *t}.  
If {\tt c} is not null, it returns the type of {\tt g} in {\tt *c}.  
If {\tt rank} is not null, it returns the rank of {\tt g} in {\tt *rank}.  
If {\tt shape} is not null, it returns the shape array of {\tt g} in 
{\tt *shape}.  Returns {\tt o} on if there is a type associated with
{\tt o}, or returns null but does not set the error code if there is
no type associated with {\tt o}.
**/

int DXTypeSize(Type t);
int DXCategorySize(Category c);
/**
\index{DXTypeSize}\index{DXCategorySize}
{\tt DXTypeSize()} returns the size in bytes of a variable of type {\tt t}.
{\tt DXCategorySize()} returns the size multiplier for category {\tt c}.
For a variable of type {\tt t} and category {\tt c}, the size in bytes is
{\tt DXTypeSize(t) * DXCategorySize(c)}.
**/

Error DXPrint(Object o, char *options, ...);
Error DXPrintV(Object o, char *options, char **components);
/**
\index{DXPrint}\index{DXPrintV}
Prints {\tt o} according to the specified {\tt options}; see the DXPrint
module for a description of the options.  Prints the components
specified by the null-terminated list of string arguments following
{\tt options} in the case of {\tt DXPrint()}, or by a null-terminated
array of pointers to names of components in the case of {\tt
DXPrintV()}.  If no components are specified or if {\tt components} is
null, all components are printed.  Returns {\tt OK} on success, or
returns null and sets the error code to indicate an error.
**/

#endif /* _DXI_OBJECT_H_ */

#if defined(__cplusplus) || defined(c_plusplus)
#undef private
}
#endif

