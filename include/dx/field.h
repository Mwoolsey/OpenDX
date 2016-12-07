/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/


#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#ifndef _DXI_FIELD_H_
#define _DXI_FIELD_H_

/* TeX starts here. Do not remove this comment. */

/*
\section{Field class}
\label{fieldsec}

This section describes the field data structure.  Each field has some
number of named components.  Each component may have a value, which is an
{\tt Object}, and some number of attributes, whose values are strings.
The defined components and attributes are listed in Chapter ??? of the
User's Manual.
*/

Field DXNewField(void);
/**
\index{DXNewField}
Creates a new field object.  Returns the field, or returns null and
sets the error code to indicate an error.
**/

Field DXSetComponentValue(Field f, char *name, Object value);
/**
\index{DXSetComponentValue}
Sets the value of the component of the field {\tt f} specified by {\tt
name} to the specified object {\tt value}.  Makes a {\tt DXReference()}
to the object {\tt value}.  The {\tt name} may be null, in which case
a new component is added that may be accessed only via {\tt
DXGetEnumeratedComponentValue()}.  Any attributes set on the previous
value of the component are copied to the new value; this may override
any attributes set on the new component value before {\tt DXSetComponentValue()}
is called.  If this is not the desired behavior, you may either delete
the old component first, or set the new component attributes after you set
the component value.  Returns {\tt f} on success, or
returns null and sets the error code to indicate an error.
**/

Field DXSetComponentAttribute(Field f, char *name, char *attribute, Object value);
/**
\index{DXSetComponentAttribute}
Sets the value of the specified {\tt attribute} of component {\tt
name} to {\tt value}.  Returns {\tt f} on success, or returns null and
sets the error code to indicate an error.
**/

Object DXGetComponentValue(Field f, char *name);
/**
\index{DXGetComponentValue}
Returns the value of component {\tt name} of field {\tt f}, or returns
null but does not set the error code if no such component exists.
**/

Object DXGetComponentAttribute(Field f, char *name, char *attribute);
/**
\index{DXGetComponentAttribute}
Returns the value of the specified {\tt attribute} of component {\tt
name}, or returns null but does not set the error code if no such
component exists.
**/

Object DXGetEnumeratedComponentValue(Field f, int n, char **name);
Object DXGetEnumeratedComponentAttribute(Field f, int n, char **name, char *attribute);
/**
\index{DXGetEnumeratedComponentValue}\index{DXGetEnumeratedComponentAttribute}
Enumerates a field's components. Call {\tt GetEnumeratedComponent()}
with successive values of {\tt n} starting with 0 until null is
returned.  Note that the numbering changes as components are added and
deleted.  These routines return the name of the component in *name.  The
first routine returns the value of the {\tt n}th component, while the
second routine returns the value of the specified {\tt attribute} of the
{\tt n}th component.  Thus {\tt n} and {\tt attribute} are input
parameters and {\tt name} is an output parameter.  Returns the value
of the component, or returns null but does not set the error code
if {\tt n} is out of range.
**/

Field DXDeleteComponent(Field f, char *component);
/**
\index{DXDeleteComponent}
Deletes the named component from a field.  Returns {\tt f}, or returns
null but does not set the error code if the component does not exist.
**/

Error DXComponentReq(Array a, Pointer *data, int *n, int nreq, Type t, int dim);
Error DXComponentOpt(Array a, Pointer *data, int *n, int nreq, Type t, int dim);
Error DXComponentReqLoc(Array a, Pointer *data, int *n, int nreq, Type t, int dim);
Error DXComponentOptLoc(Array a, Pointer *data, int *n, int nreq, Type t, int dim);
/**
\index{DXComponentOpt}\index{DXComponentReq}
\index{DXComponentOptLoc}\index{DXComponentReqLoc}
These functions combine several common operations in accessing and
checking a field component array.  The four routines have identical
calling sequences, but differ as follows: First, {\tt DXComponentOpt()}
and {\tt DXComponentReq()} return pointers to the global copy of the
array data, while {\tt DXComponentOptLoc()} and {\tt DXComponentReqLoc()}
return pointers to a local copy of the array data, and should be
matched by a {\tt FreeDataLocal()} call.  Second, {\tt DXComponentReq()}
and {\tt DXComponentReqLoc()} consider it an error if the component is
missing ({\tt a} is null), while {\tt DXComponentOpt()} and {\tt
DXComponentOptLoc()} consider the component optional and do not consider
a null {\tt a} to be an error.  If {\tt data} is not null, a pointer
to a global or local copy of the data is returned in {\tt *data}.  If
{\tt n} is not null, the number of items in the array is returned in
{\tt *n}.  If {\tt n} is null, the number of array items must be {\tt
nreq}.  The type of the array must be {\tt type}.  If {\tt dim} is 0,
the array must have rank 0 (scalar).  If {\tt dim} is non-zero, the
array must have rank 1 and shape equal to {\tt dim}.  Returns {\tt OK}
if no error occurs, or returns {\tt ERROR} and sets the error code in
case of error.
**/

/*
\paragraph{Usage notes.}
An example of the expected usage of {\tt DXComponentReq()} etc. is
as follows:
\begin{program}
    a = DXGetComponentValue(f, "positions");
    if (!DXComponentReq(a, &points, &npoints, 0, TYPE_FLOAT, 3))
        return NULL;

    a = DXGetComponentValue(f, "colors");
    if (!DXComponentOpt(a, &colors, NULL, npoints, TYPE_FLOAT, 3))
	return NULL;
    if (colors) ...
\end{program}
The first two statements check and retrieve a required ``positions''
component,  while the next two statements check and retrieve an optional
``colors'' component that must have the same number of elements as the
``positions'' component.  Since the second call uses {\tt DXComponentOpt()},
the program must then subsequently check if {\tt colors} is null to 
determine whether the colors were present.
*/

#endif /* _DXI_FIELD_H_ */

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
