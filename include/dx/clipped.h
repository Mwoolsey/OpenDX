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

#ifndef _DXI_CLIPPED_H_
#define _DXI_CLIPPED_H_

/* TeX starts here. Do not remove this comment. */

/*
\section{Clipped class}
A {\tt Clipped} object represents one object clipped by another.  The
first object is actually rendered; the second represents a region to
which the first object is clipped.  The clipping object is expected to
be a closed convex surface.  The portion of the first object that is
within the clipping object is rendered.  If the clipping object is not
a closed convex surface, the results are undefined.  The clipping is
performed by the renderer during the rendering process.  Thus clipping
is provided as a data structure for representing the clipped object to
the renderer, rather than as an explicit operation.
*/

Clipped DXNewClipped(Object render, Object clipping);
/**
\index{DXNewClipped}
Constructs a clipped object that instructs the renderer to render the
first argument {\tt render} clipped by the second argument {\tt
clipping}.  The {\tt clipping} object must have only surface data (no
volume data); the colors and opacity of the surface are ignored.  In
the current version of the Data Explorer, nesting of clipping objects
is not supported, and the clipping object must be convex.  Every
volume and translucent surface in a scene must have the same clipping
object.  Returns the clipped object, or returns null and sets the
error code to indicate an error.
**/

Clipped DXGetClippedInfo(Clipped c, Object *render, Object *clipping);
/**
\index{DXGetClippedInfo}
If {\tt render} is not null, this routine returns the object to be
rendered in {\tt *render}.  If {\tt clipping} is not null, it returns
the clipping object in {\tt *clipping}.  Returns {\tt c}, or returns
null and sets the error code to indicate an error.
**/

Clipped DXSetClippedObjects(Clipped c, Object render, Object clipping);
/**
\index{SetClippedObject}
If {\tt render} is not null, his routine sets the object to be
rendered to {\tt render}.  If {\tt clipping} is not null, this routine
sets the clipping object to {\tt clipping}.  Returns {\tt c}, or
returns null and sets the error code to indicate an error.
**/

#endif /* _DXI_CLIPPED_H_ */

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
