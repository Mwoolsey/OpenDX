/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/


/*
\section{Camera class}
The camera object stores the camera parameters specified by the user.
The viewing transformation matrix is then computed when it is needed.
This could be changed as follows:  the camera object will store both
the input parameters (e.g. {\tt to}, {\tt from}) and the computed
parameters (e.g. the viewing matrix).  Each time an input parameter is
changed, the computed parameters will be recomputed.  This way one could
add alternative ways of specifying the view, e.g. viewing direction
({\tt to-from}).
*/

INCLUDE  objectClass.X

SUBCLASS Camera OF Object
IMPLEMENTS Delete Copy

struct camera {				/* camera object */
    struct object object;		/* object preamble */
    Point from;				/* camera position */
    Point to;				/* look-at position */
    Vector up;				/* up vector */
    int ortho;				/* orthographic projection? */
    float width;			/* viewport width */
    float aspect;			/* aspect ratio */
    float pix_aspect;			/* pixel aspect ratio */
    int resolution;			/* resolution for image generation */
    Matrix m;				/* computed camera matrix */
    Matrix rot;				/* rotation part of matrix */
    RGBColor background;                /* image background color */
};


