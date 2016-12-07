/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/


/*
\section{Xform class}
An xform object specifies a transformation matrix to be applied to
a renderable object.
*/

INCLUDE  objectClass.X

SUBCLASS Xform OF Object
IMPLEMENTS Copy BoundingBox Delete GetType
IMPLEMENTS Shade Paint Gather

struct xform {				/* xform object */
    struct object object;		/* object preamble */
    Matrix m;				/* xform to apply */
    Object o;				/* transformed object */
};

