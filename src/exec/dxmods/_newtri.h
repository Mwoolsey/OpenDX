/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dxmods/_newtri.h,v 1.1 2000/08/24 20:04:16 davidt Exp $
 */

#include <dxconfig.h>

#ifndef  __NEWTRI_H_
#define  __NEWTRI_H_

#include <dx/dx.h>

typedef struct
{
    float               u;
    float               v;
} Point2;

typedef struct
{
    Point2              ll;
    Point2              ur;
} BBox;

typedef struct BVertex
{
    struct BVertex      *next;          /* boundary pointers         */
    struct BVertex      *prev;
    struct Loop         *lp;            /* initial parent loop       */
    int                 id;             /* point id                  */
    Point2              pos;            /* projection of vertex      */
    int                 tried;
    int                 pass;
} BVertex;

typedef struct
{
    int                 *faces;         /* the input faces              */
    int                 *loops;         /* the input loops              */
    int                 *edges;         /* the input edges              */
    float               *points;        /* the input points             */
    int                 *map;           /* the faces to triangles map   */
    int                 nfaces;         /* # of faces                   */
    int                 nloops;         /* # of loops                   */
    int                 nedges;         /* # of edges                   */
    int                 npoints;        /* # of points                  */
    int                 nDim;           /* dimensionality of points     */
} FLEP;

typedef struct Loop
{
    struct Loop         *next;
    struct Loop         *prev;
    int                 nvert;          /* number of vertices        */
    int                 *vids;          /* pointer to vertices       */
    BVertex             *base;          /* base of linked list       */
    int                 merged;         /* this loop has been merged */
    int                 sign;           /* direction of rotation     */
    BBox                box;            /* loop bounding box         */
} Loop;

/*
 * For the tree representing the loop nesting...
 */
typedef struct treenode *TreeNode;

struct treenode
{
    Loop                *loop;
    TreeNode            sibling;
    TreeNode            children;
    int                 nChildren;
    int                 level;
};

Error _dxfTriangulateField(Field);
int     _dxf_TriangleCount (int *f, int nf, int *l, int nl, int *e, int ne);
int     _dxf__TriangleCount (FLEP *in);
Error   _dxf_TriangulateFLEP (int *f, int nf, int *l, int nl, int *e, int ne,
                float *p, int np, Vector *normal, Triangle *tris, int *ntri,
                int *map, int nDim);
Error   _dxf__TriangulateFLEP (FLEP *in, Vector *normal, Triangle *tris, int *ntri,
                 FLEP *valid);


#endif /* __NEWTRI_H_ */

