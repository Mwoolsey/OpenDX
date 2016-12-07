/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

/*
\section{Clipped class}
A clipped object specifies an object to be rendered and specifies a second object against which to clip it during rendering.
*/

INCLUDE  objectClass.X

SUBCLASS Clipped OF Object

IMPLEMENTS Copy BoundingBox Delete
IMPLEMENTS Gather Paint Shade

struct clipped {			/* clipped object */
    struct object object;		/* object preamble */
    Object render;			/* object to be rendered */
    Object clipping;			/* object against which to clip it */
};

