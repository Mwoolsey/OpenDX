/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

/*
\section{Light class}
*/

INCLUDE  objectClass.X

SUBCLASS Light OF Object
IMPLEMENTS Delete Gather
IMPLEMENTS Paint Shade Copy

struct light {
    struct object object;	/* object preamble */

    enum {invalid_kind, point, distant, ambient} kind;
				/* what kind of light */
    enum {invalid_relative, camera, world} relative;
				/* relative to what */

    Point position;		/* position, if point */
    Vector direction;		/* direction, if distant */
    RGBColor color;		/* color, if point, distant, or ambient */
};

