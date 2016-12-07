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

#ifndef _DXI_COMPONENT_H_
#define _DXI_COMPONENT_H_

/* TeX starts here. Do not remove this comment. */

/*
\section{Component manipulation}
This section describes advanced routines for explicitly manipulating
field components.
*/

Object DXRename(Object o, char *oldname, char *newname);
/**
\index{DXRename}
For each component in object {\tt o} named {\tt oldname}, this routine
renames it {\tt newname}. It replaces any components called {\tt
newname} that already exist.  It is an error if no components of the
specified {\tt name} are found in any of the fields of {\tt o}.
Returns {\tt o} on success, or returns null and sets the error code to
indicate an error.
**/


Object DXSwap(Object o, char *name1, char *name2);
/**
\index{DXSwap}
For each field in object {\tt o} which contains both the named
components, this routine exchanges the components.  It is an error
if no components of both names are found in any of the fields of {\tt o}.
Returns {\tt o} on success, or returns null and sets the error code to
indicate an error.
**/


Object DXExtract(Object o, char *name);
/**
\index{DXExtract}
Extracts the named component from {\tt o}, which may be a field or a
group.  If {\tt o} is a simple field, this routine returns the named
component of the field. If {\tt o} is a {\tt Group}, it returns a
hierarchy of groups of objects (typically arrays) that are the named
components.  It is an error if no components of the specified {\tt
name} are found in any of the fields of {\tt o}.  Returns {\tt o} on
success, or returns null and sets the error code to indicate an error.
**/


Object DXInsert(Object o, Object add, char *name);
/**
\index{DXInsert}
For each field in object {\tt o}, this routine adds object {\tt add}
as component {\tt name}. If {\tt o} is a simple field, {\tt add} must
be an {\tt Array}. If {\tt o} is a {\tt Group}, {\tt add} must be a
{\tt Group} of {\tt Array} objects, and the field hierarchies of {\tt
o} and {\tt add} must match.  Returns {\tt o} on success, or returns
null and sets the error code to indicate an error.
**/


Object DXReplace(Object o, Object add, char *src, char *dst);
/**
\index{DXReplace}
This routine takes any components named {\tt src} that are contained
in any field in the {\tt add} object, adds them to the corresponding
field in the {\tt o} object, and renames the newly placed components
{\tt dst}.  The field hierarchies of {\tt o} and {\tt add} must match.
It is an error if no components of name {\tt src} are found in any of
the fields of {\tt add}.  Returns {\tt o} on success, or returns null
and sets the error code to indicate an error.
**/


Object DXRemove(Object o, char *name);
/**
\index{DXRemove}
Deletes components of the specified {\tt name} for each field in
object {\tt o}.  It is an error if no components of the specified {\tt
name} are found in any of the fields of {\tt o}.  Returns {\tt o} on
success, or returns null and sets the error code to indicate an error.
**/


Object DXExists(Object o, char *name);
/**
\index{DXExists}
If any field in object {\tt o} contains a component of name {\tt name},
this routine returns {\tt o}, otherwise it returns null but does not
set the error code.
**/

#endif /* _DXI_COMPONENT_H_ */

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
