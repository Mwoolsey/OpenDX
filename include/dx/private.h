/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

/* TeX starts here. Do not remove this comment. */

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#define delete __delete__
#endif

#ifndef _DXI_PRIVATE_H_
#define _DXI_PRIVATE_H_

/*
\section{Private class}
\label{privatesec}

Private objects are an extension mechanism to allow the user to store
arbitrary private data in the form of an object.  This is useful for
example in the cache interface, where the cached items must be
objects.  The user is entirely responsible for maintaining the
contents of the private data.  In particular, when you create the
private object you may specify a deletion routine that will be called
when the object is deleted, which may for example free the private
storage if it was allocated.
*/

Private
DXNewPrivate(Pointer data, Error (*delete)(Pointer));
/**
\index{DXNewPrivate}
This routine creates an object that contains a pointer to your private
{\tt data}.  You are responsible for the contents of the private data.
If {\tt delete} is not null, it will be called when the private object
is deleted.  It will be passed the pointer to your {\tt data}.  Returns
the private object, or returns null and sets the error code to indicate
an error.
**/

Pointer
DXGetPrivateData(Private p);
/**
\index{DXGetPrivateData}
Returns the private data pointer specified when {\tt p} was created.
**/

#endif /* _DXI_PRIVATE_H_ */

#if defined(__cplusplus) || defined(c_plusplus)
#undef delete
}
#endif
