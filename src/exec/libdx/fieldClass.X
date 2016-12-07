/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

/*
\section{Field class}
*/

INCLUDE  objectClass.X
SUBCLASS Field OF Object
IMPLEMENTS Copy GetType Delete BoundingBox
IMPLEMENTS Gather Paint Shade Partition

#define NCOMPONENTS 8

struct field {

    struct object object;		/* object preamble */

    /* the component/attribute table */
    struct component {			/* the components */
	char *name;			/* component name */
	Object value;			/* component value */
    } local[NCOMPONENTS], *components;	/* the components */
    int ncomponents;			/* number of components */
    int comp_alloc;			/* allocated space for components */

    /* helper stuff */
    int cell;				/* whether we'r in a cell or not */
    PointId *pts;			/* the points in current face */
    int npts;				/* number of pts in current face */
    int pts_alloc;			/* allocated size of pts array */
    struct hash {			/* hash table to eliminate dup faces */
	Triangle tri;			/* the triangle */
	int count;			/* multiplicity */
    } *hash;
    int nsurface, ninner;		/* counts in hash table */
    int hash_alloc;			/* allocate size of hash table */
};



