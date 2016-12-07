/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 */

#include <dxconfig.h>



#include <stdio.h>
#include <math.h>
#include <string.h>
#include "_connectvor.h"

#define DEBUG 1

typedef struct
{
  float x;
  float y;
} Point2D;

typedef struct
{
  int first;
  int second;
  int tri1;
  int tri2;
} edge;

static int CheckThem(Triangle *, Triangle *, int);
static int InTheSet(int, Triangle *, int, int);
static float GetAngle(float *, int, int, int);
static int Not(Triangle, int, int);
static int NeighborEdge(int, int, int, Triangle *, Triangle *);
static Error FlipEm(int, edge *, Triangle *, Triangle *, float *);
static Error DoTheFlip(int, int, int, int, int, int, Triangle *,
                       Triangle *, edge *);


/* returns the handedness of a triangle */
static int Handedness(float *pos_ptr, int i, int index1, int index2)
{
  Vector v1, v2, v3, v4, v5;
  float c;
  
  v1 = DXVec(pos_ptr[2*i], pos_ptr[2*i+1], 0.0);
  v2 = DXVec(pos_ptr[2*index1], pos_ptr[2*index1+1], 0.0);
  v3 = DXVec(pos_ptr[2*index2], pos_ptr[2*index2+1], 0.0);
  v4 = DXSub(v2, v1);
  v5 = DXSub(v3, v1);
  /* since I only care about the z component, I won't bother with
   * the true cross product */
  c = v4.x*v5.y - v4.y*v5.x;
  if (c < 0) 
    return -1;
  else if (c > 0)
    return 1;
  else 
    return 0;
}



/* find out if the new point is on any of the boundaries of the triangle */
static int AnyDegenerate(Triangle vertices, float *pos_ptr, int new)
{
  int hand1, hand2, hand3;
  
  hand1 = Handedness(pos_ptr, vertices.p, vertices.q, new);
  hand2 = Handedness(pos_ptr, new, vertices.q, vertices.r);
  hand3 = Handedness(pos_ptr, vertices.r, vertices.p, new);
  
  if (hand1==0)
    return 1;
  else if (hand2==0)
    return 2;
  else if (hand3==0)
    return 3;
  else 
    return 0;

}



static Error GetVertices(Triangle *trianglelist, int j, Triangle *vertices)
{
  
  *vertices = trianglelist[j];
  return OK;

}





/* determines whether or not a given 2D point is inside a given 2D triangle */
static int Inside(float *pos_ptr, Triangle vertices, int new)
{
  int hand1, hand2, hand3;
  
  hand1 = Handedness(pos_ptr, vertices.p, vertices.q, new);
  hand2 = Handedness(pos_ptr, new, vertices.q, vertices.r);
  hand3 = Handedness(pos_ptr, vertices.r, vertices.p, new);
  
  /* Handedness returns 0 if the 3 points are on a line. The 
   * following checks for hand==0 ensure that if a point is on
   * the boundary of a triangle, it will be considered in the triangle 
   */ 
  if (hand1==0) {
    if ((hand2==0)||(hand3==0)) 
      return -1;
    if (hand2==hand3)
      return 1;
  }
  else if (hand2==0) {
    if ((hand1==0)||(hand3==0)) 
      return -1;
    if (hand1==hand3) 
      return 1;
  }
  else if (hand3==0) {
    if ((hand1==0)||(hand2==0)) 
      return -1;
    if (hand1==hand2)
      return 1;
  }
  else if ((hand1==hand2)&&(hand2==hand3)) {
    return 1;
  }
  return 0;
}




/* given a position list and an index, along with a list of triangles 
 * covering the plane, determines which of those triangles the new point
 * lies within */

static Error FindWhichTriangle(float *pos_ptr, Triangle *trianglelist, 
                               int howmany, int new, int *which)
{
  int i, in;
  Triangle vertices;
  
  
  /* for all the triangles there are.... discount the first three, it
   * won't be there by definition */
  for (i=0; i<howmany; i++) {
    /* get the vertices of the triangle */
    if (!GetVertices(trianglelist, i, &vertices))
      goto error;
    if ((vertices.p!=-1) && (vertices.q!=-1) && (vertices.r!=-1)) {
      /* is the new point inside that triangle? */
      in = Inside(pos_ptr, vertices, new);
      if (in==1) {
	*which = i;
	return OK;
      }
      /* if in==-1, then the point was a duplicate */
      else if (in==-1) {
	DXWarning("duplicated position %d ignored", new-3); 
	*which = -1;
	return OK;
      }
    }
  }
  DXSetError(ERROR_INTERNAL, 
             "could not find the triangle which the new point is in");
  return ERROR;
  
 error:
  return ERROR;
}


static Error GetNeighbors(Triangle *neighborlist, int j, Triangle *neighbors)
{
  
  *neighbors = neighborlist[j];
  return OK;
  
}


static int Convex(float *pos_ptr, int a, int b, int c, int d)
{
  int h1, h2, h3, h4, hand;
  
  /* handedness can be zero */
  h1 = Handedness(pos_ptr, a, b, c);
  h2 = Handedness(pos_ptr, b, c, d);
  h3 = Handedness(pos_ptr, c, d, a);
  h4 = Handedness(pos_ptr, d, a, b);

  /* if any three points are colinear, we don't want to consider it
   * convex (otherwise we get degenerate triangles) */

  /* let's try commenting this out
  if ((h1==0)||(h2==0)||(h3==0)||(h4==0))
    return 0;
  */

  /* check if handedness is all the same */
  /* first initialize hand to the first non-zero value */
  if (h1 != 0)
     hand = h1;
  else if (h2 != 0)
     hand = h2;
  else if (h3 != 0)
     hand = h3;
  else hand = h4;

  if (h1 != 0) {
     if (h1 != hand) return 0;
  }
  if (h2 != 0) {
     if (h2 != hand) return 0;
  }
  if (h3 != 0) {
     if (h3 != hand) return 0;
  }
  if (h4 != 0) {
     if (h4 != hand) return 0;
  }
  return 1;
     
  /* 
  if ((h1==h2)&&(h2==h3)&&(h3==h4))
     return 1;
  else
     return 0; 
  */
}

static Error ConnectVoronoiField(Field, Vector);


Error _dxfConnectVoronoiObject(Object ino, Vector normal) 
{
  int i;
  Object sub;
  
  
  switch (DXGetObjectClass(ino)) {
  case (CLASS_GROUP):
    switch (DXGetGroupClass((Group)ino)) {
    case (CLASS_COMPOSITEFIELD):
    case (CLASS_MULTIGRID):
      DXSetError(ERROR_DATA_INVALID,
		 "cannot connect composite or multigrid fields using voronoi method");
      goto error;
    case (CLASS_SERIES):
    case (CLASS_GROUP):
      /* simply recurse */
      for (i=0; (sub=DXGetEnumeratedMember((Group)ino, i, NULL)); i++) {
	if (!_dxfConnectVoronoiObject(sub, normal))
	  goto error;
      }
      break;
    default:
      DXSetError(ERROR_DATA_INVALID,"unknown group class");
      goto error;
    }
    break;
  case (CLASS_FIELD):
    if (!ConnectVoronoiField((Field)ino, normal))
      goto error;
    break;    
  case (CLASS_XFORM):
    /* just recurse through it */
    if (!(DXGetXformInfo((Xform)ino, &sub, NULL)))
      goto error;
    if (!_dxfConnectVoronoiObject(sub, normal))
      goto error;
    break;
  case (CLASS_CLIPPED):
    /* just recurse through it */
    if (!(DXGetClippedInfo((Clipped)ino, &sub, NULL)))
      goto error;
    if (!_dxfConnectVoronoiObject(sub, normal))
      goto error;
    break;
  default:
    break;
  }
 return OK;

 error:
  return ERROR;
}

static Error ConnectVoronoiField(Field ino, Vector normal)
{
  Array positions, connections=NULL;
  Type type;
  float *pos_ptr=NULL, *old_pos_ptr=NULL, *tmp=NULL, *threeD_pos_ptr=NULL; 
  float minx, miny, maxx, maxy, offset, deltax, deltay, x, y; 
  Category category;
  Triangle triangle, *trianglelist=NULL, *neighborlist=NULL; 
  Triangle vertices, neighbors, n_vertices, n_neighbors, n;
  int theseneighbors[4];
  int i, j, numpos, rank, shape[8], numtri, trianglecount;
  int numsuspect, validcount;
  int whichdegenerate;
  edge suspectedges[4];
  Point box[8], oldpt;
  Vector up, right, newvec;
  InvalidComponentHandle handle=NULL;

  
  if (!(DXGetObjectClass((Object)ino) == CLASS_FIELD)) {
    DXSetError(ERROR_DATA_INVALID,"input must be a field");
    goto error;
  }
  if (DXEmptyField(ino))
    return OK;


  /* create an invalid component handle */

  handle = DXCreateInvalidComponentHandle((Object)ino, NULL, "positions");
  if (!handle) goto error;
  if (!DXInitGetNextValidElementIndex(handle)) goto error;

  
  
  positions = (Array)DXGetComponentValue(ino,"positions");
  DXGetArrayInfo(positions,&numpos, &type, &category, &rank, shape);
  
  if ((type!=TYPE_FLOAT)||(category!=CATEGORY_REAL)) {
    DXSetError(ERROR_DATA_INVALID,"positions must be float, real");
    goto error;
  }
  validcount = DXGetValidCount(handle); 



  /* need to be at least three XXX*/
  if (validcount < 3) {
    DXWarning("there must be at least three valid positions to connect");
    return OK;
  }
  
  /* allocate the arrays for the triangle list and the neighbors list */
  trianglelist = (Triangle *)DXAllocateLocal((2*(numpos+3)-2)*sizeof(Triangle));
  neighborlist = (Triangle *)DXAllocateLocal((2*(numpos+3)-2)*sizeof(Triangle));
  if (!trianglelist || !neighborlist) 
     goto error;
 
  if (rank != 1) {
    DXSetError(ERROR_DATA_INVALID,"positions must be rank 1");
    goto error;
  }
  if ((shape[0]!=2)&&(shape[0]!=3)) {
    DXSetError(ERROR_DATA_INVALID,"positions must be 2D or 3D");
    goto error;
  }

  /* need to do a projection */
  if (shape[0] == 3) {

    /* first figure out an "up" and a "right" vector for our eventual 
     * plane. All that matters is that they, with the normal, are
     * mutually perpendicular.
     */
    if ((normal.x==0)&&(normal.y==0)&&(normal.z==1)) {
       up = DXVec(0, 1, 0);
       right = DXVec(1, 0, 0);
    }
    else if ((normal.x==0)&&(normal.y==0)&&(normal.z==-1)) {
       up = DXVec(0, 1, 0);
       right = DXVec(1, 0, 0);
    }
    else {
       /* this up won't be parallel to normal, we know */
       up = DXVec(0, 0, 1);
       up = DXCross(up, normal);
       right = DXCross(up, normal);
    }

    old_pos_ptr = (float *)DXAllocateLocal(numpos*2*sizeof(float));
    threeD_pos_ptr = (float *)DXGetArrayData(positions);
    for (i=0; i<numpos; i++) {
      oldpt = DXPt(threeD_pos_ptr[3*i], 
                   threeD_pos_ptr[3*i+1], 
                   threeD_pos_ptr[3*i+2]);
      newvec = DXSub(oldpt, DXMul(normal, DXDot(oldpt, normal)));
      y = DXDot(up, newvec);
      x = DXDot(right, newvec);
      old_pos_ptr[2*i]= x;
      old_pos_ptr[2*i+1]= y; 
    }
  }
  else {
    old_pos_ptr = (float *)DXGetArrayData(positions);
  }

  pos_ptr = (float *)DXAllocateLocal((numpos+3)*2*sizeof(float));
  if (!pos_ptr)
    goto error;

  /* need to figure out the bounding triangle of the projected points */
  if (shape[0]==3) {
    /* first need to figure out the min and max x's and y's for the
     * projected points */
    minx=DXD_MAX_FLOAT;
    miny=DXD_MAX_FLOAT;
    maxx=-DXD_MAX_FLOAT; 
    maxy=-DXD_MAX_FLOAT; 
    for (i=0; i<numpos; i++) {
      if (old_pos_ptr[2*i]<minx) minx = old_pos_ptr[2*i];
      if (old_pos_ptr[2*i+1]<miny) miny = old_pos_ptr[2*i+1];
      if (old_pos_ptr[2*i]>maxx) maxx = old_pos_ptr[2*i];
      if (old_pos_ptr[2*i+1]>maxy) maxy = old_pos_ptr[2*i+1];
    }
    deltax = maxx-minx;
    deltay = maxy-miny;
    /* make the box much bigger for good measure */
    minx = minx-15*(deltax);
    miny = miny-15*(deltay);
    maxx = maxx+15*(deltax);
    maxy = maxy+15*(deltay);


    /* construct the triangle which bounds the points */
    deltax = (maxx-minx);
    deltay = (maxy-miny);
    offset = sqrt(deltax*deltay/2.0);
 
    pos_ptr[0] = minx-offset;
    pos_ptr[1] = miny;
    pos_ptr[2] = maxx+offset;
    pos_ptr[3] = miny;
    pos_ptr[4] = minx+(deltax/2.0);
    pos_ptr[5] = maxy+offset;
  }
  else {
    /* get the bounding box of the points */
    if (!DXBoundingBox((Object)ino,box)) {
      DXSetError(ERROR_DATA_INVALID,"object has no bounding box");
      goto error;
    }
    
    minx = DXD_MAX_FLOAT; 
    miny = DXD_MAX_FLOAT; 
    maxx = -DXD_MAX_FLOAT; 
    maxy = -DXD_MAX_FLOAT; 
    for (i=0; i<7; i++) {
      if (box[i].x < minx) minx = box[i].x;
      if (box[i].y < miny) miny = box[i].y;
      if (box[i].x > maxx) maxx = box[i].x;
      if (box[i].y > maxy) maxy = box[i].y;
    }
    deltax = maxx-minx;
    deltay = maxy-miny;
    /* make the box bigger for good measure */
    minx = minx-15*(deltax);
    miny = miny-15*(deltay);
    maxx = maxx+15*(deltax);
    maxy = maxy+15*(deltay);


    /* construct the triangle which bounds the points */
    deltax = (maxx-minx);
    deltay = (maxy-miny);
    offset = sqrt(deltax*deltay/2.0);
  
    pos_ptr[0] = minx-offset;
    pos_ptr[1] = miny;
    pos_ptr[2] = maxx+offset;
    pos_ptr[3] = miny;
    pos_ptr[4] = minx+(deltax/2.0);
    pos_ptr[5] = maxy+offset;
  }
  tmp = &pos_ptr[6];
  memcpy(tmp, old_pos_ptr, numpos*2*sizeof(float));
 

  
  
  /* initialize the first three triangles. 
   * -1 corresponds to the point at infinity. 
   * XXX what if the first triangle is degenerate? */

     
  /* By convention, 
   * if trianglelist = a, b, c, and neighborlist = d, e, f, then 
   * d is the neighbor lying between a and b, e is the neighbor lying 
   * between b and c, and f is the neighbor lying between c and a. 
   */

  /* the triangle in the center */
  trianglelist[3] = DXTri(0, 2, 1);
  neighborlist[3] = DXTri(2, 1, 0);
  /* the boundary triangles */
  trianglelist[0] = DXTri(0, 1, -1);
  neighborlist[0] = DXTri(3, 1, 2);
  trianglelist[1] = DXTri(1, 2, -1);
  neighborlist[1] = DXTri(3, 2, 0);
  trianglelist[2] = DXTri(2, 0, -1);
  neighborlist[2] = DXTri(3, 0, 1);
  
  numtri = 4;
  
  /* for the rest of the valid points */

  while ((i=DXGetNextValidElementIndex(handle)) != -1) {
    i+=3;

    /* j is the triangle which the new point is found in */
    if (!FindWhichTriangle(pos_ptr, trianglelist, numtri, i, &j))
      goto error;
    /* if j == -1, it was a duplicated point */
    if (j==-1)
      continue;
    
    
    /* vertices are the vertices of the triangle which the new point
     * was found within */
    if (!GetVertices(trianglelist, j, &vertices))
      goto error;
    
    /* neighbors are the three neighbors of the triangle the new point
     * was found within */
    if (!GetNeighbors(neighborlist, j, &neighbors))
      goto error; 
    theseneighbors[0] = neighbors.p;
    theseneighbors[1] = neighbors.q;
    theseneighbors[2] = neighbors.r;
    
    /* add the new triangles; the first one replaces triangle j */
    if ((whichdegenerate=AnyDegenerate(vertices, pos_ptr, i))) {
      numsuspect = 4;
      if (whichdegenerate==1) {
	/* p q new is degenerate */
	trianglelist[j] = DXTri(i, vertices.q, vertices.r);
	neighborlist[j] = DXTri(neighbors.p, neighbors.q, numtri);

        /* need to update the old neighbor of j along the pr side */
        /* which is theseneighbors[2] */

        if (((trianglelist[theseneighbors[2]].p == vertices.p)&&
            (trianglelist[theseneighbors[2]].q == vertices.r)) ||
           ((trianglelist[theseneighbors[2]].p == vertices.r)&&
            (trianglelist[theseneighbors[2]].q == vertices.p)))
           neighborlist[theseneighbors[2]].p = numtri;
        else if (((trianglelist[theseneighbors[2]].q == vertices.p)&&
                  (trianglelist[theseneighbors[2]].r == vertices.r)) ||
                 ((trianglelist[theseneighbors[2]].q == vertices.r)&&
                  (trianglelist[theseneighbors[2]].r == vertices.p)))
           neighborlist[theseneighbors[2]].q = numtri;
        else if (((trianglelist[theseneighbors[2]].r == vertices.p)&&
                  (trianglelist[theseneighbors[2]].p == vertices.r)) ||
                 ((trianglelist[theseneighbors[2]].r == vertices.r)&&
                  (trianglelist[theseneighbors[2]].p == vertices.p)))
           neighborlist[theseneighbors[2]].r = numtri;
        else {
          DXSetError(ERROR_INTERNAL,"invalid neighbors");
          goto error;
        }


        
	
	suspectedges[0].first = vertices.q;
	suspectedges[0].second = vertices.r;
	suspectedges[0].tri1 = j;
	suspectedges[0].tri2 = neighbors.q;
	
	trianglelist[numtri] = DXTri(i, vertices.r, vertices.p);
	neighborlist[numtri] = DXTri(j, neighbors.r, numtri+1);

	suspectedges[1].first = vertices.r;
	suspectedges[1].second = vertices.p;
	suspectedges[1].tri1 = numtri;
	suspectedges[1].tri2 = neighbors.r;
	
	numtri++;
	
	GetVertices(trianglelist, neighbors.p, &n_vertices);
	GetNeighbors(neighborlist, neighbors.p, &n_neighbors);
	
	/* need to update the old neighbor of j along the p q side (we
	   need to split that triangle) Depends on how that neighbor
	   is oriented */

	if ((n_vertices.r==vertices.p)&& (n_vertices.q==vertices.q)) {
	  trianglelist[neighbors.p]=DXTri(i, n_vertices.p, n_vertices.q);
	  neighborlist[neighbors.p]=DXTri(numtri,n_neighbors.p, j);
	  
	  suspectedges[2].first = n_vertices.p;
	  suspectedges[2].second = n_vertices.q;
	  suspectedges[2].tri1 = neighbors.p;
	  suspectedges[2].tri2 = n_neighbors.p;
	  
	  trianglelist[numtri]=DXTri(i, n_vertices.r, n_vertices.p);
	  neighborlist[numtri]=DXTri(numtri-1, n_neighbors.r, neighbors.p); 
	  
	  suspectedges[3].first = n_vertices.r;
	  suspectedges[3].second = n_vertices.p;
	  suspectedges[3].tri1 = numtri;
	  suspectedges[3].tri2 = n_neighbors.r;
	  

	  GetNeighbors(neighborlist, n_neighbors.r, &n);  
	  /* update the neighbor of the old triangle neighbors.p */
	  if (n.p == neighbors.p) neighborlist[n_neighbors.r].p = numtri;
	  else if (n.q == neighbors.p) neighborlist[n_neighbors.r].q = numtri;
	  else if (n.r == neighbors.p) neighborlist[n_neighbors.r].r = numtri;
	  else {
	    DXSetError(ERROR_DATA_INVALID,"invalid neighbors");
	    goto error;
	  }
	}
	
	else if ((n_vertices.r==vertices.q)&& (n_vertices.q==vertices.p)) {
	  trianglelist[neighbors.p]=DXTri(i, n_vertices.p, n_vertices.r);
	  neighborlist[neighbors.p]=DXTri(numtri,n_neighbors.r, j);
          /* changed n_neighbors.p to n_neighbors.r above XXX */
	  
	  suspectedges[2].first = n_vertices.p;
	  suspectedges[2].second = n_vertices.r;
	  suspectedges[2].tri1 = neighbors.p;
	  suspectedges[2].tri2 = n_neighbors.r;
          /* changed n_neighbors.p to n_neighbors.r above XXX 1/6/95*/
	  
	  trianglelist[numtri]=DXTri(i, n_vertices.q, n_vertices.p);
	  neighborlist[numtri]=DXTri(numtri-1, n_neighbors.p, neighbors.p); 
	  
	  suspectedges[3].first = n_vertices.q;
	  suspectedges[3].second = n_vertices.p;
	  suspectedges[3].tri1 = numtri;
	  suspectedges[3].tri2 = n_neighbors.p;
	  

	  GetNeighbors(neighborlist, n_neighbors.p, &n);  
	  /* update the neighbor of the old triangle neighbors.p */
	  if (n.p == neighbors.p) neighborlist[n_neighbors.p].p = numtri;
	  else if (n.q == neighbors.p) neighborlist[n_neighbors.p].q = numtri;
	  else if (n.r == neighbors.p) neighborlist[n_neighbors.p].r = numtri;
	  else {
	    DXSetError(ERROR_DATA_INVALID,"invalid neighbors");
	    goto error;
	  }
	}
	 
	else if ((n_vertices.q==vertices.p)&& (n_vertices.p==vertices.q)) {
	  trianglelist[neighbors.p]=DXTri(i, n_vertices.r, n_vertices.p);
	  /* XXX changed n_vertices.q to n_vertices.p above */
	  neighborlist[neighbors.p]=DXTri(numtri,n_neighbors.r, j);
	  
	  suspectedges[2].first = n_vertices.r;
	  suspectedges[2].second = n_vertices.p;
	  /* XXX changed n_vertices.q to n_vertices.p above */
	  suspectedges[2].tri1 = neighbors.p;
	  suspectedges[2].tri2 = n_neighbors.r;
	  
	  trianglelist[numtri]=DXTri(i, n_vertices.q, n_vertices.r);
	  neighborlist[numtri]=DXTri(numtri-1,n_neighbors.q, neighbors.p);
	  
	  suspectedges[3].first = n_vertices.q;
	  suspectedges[3].second = n_vertices.r;
	  suspectedges[3].tri1 = numtri;
	  suspectedges[3].tri2 = n_neighbors.q;

	  GetNeighbors(neighborlist, n_neighbors.q, &n);  
	  /* update the neighbor of the old triangle neighbors.p */
	  if (n.p == neighbors.p) neighborlist[n_neighbors.q].p = numtri;
	  else if (n.q == neighbors.p) neighborlist[n_neighbors.q].q = numtri;
	  else if (n.r == neighbors.p) neighborlist[n_neighbors.q].r = numtri;
	  else {
	    DXSetError(ERROR_DATA_INVALID,"invalid neighbors");
	    goto error;
	  }
	}
	
	else if ((n_vertices.q==vertices.q)&& (n_vertices.p==vertices.p)) {
	  trianglelist[neighbors.p]=DXTri(i, n_vertices.r, n_vertices.q);
	  neighborlist[neighbors.p]=DXTri(numtri,n_neighbors.q, j);
	  
	  suspectedges[2].first = n_vertices.r;
	  suspectedges[2].second = n_vertices.q;
	  suspectedges[2].tri1 = neighbors.p;
	  suspectedges[2].tri2 = n_neighbors.q;
	  
	  trianglelist[numtri]=DXTri(i, n_vertices.p, n_vertices.r);
	  neighborlist[numtri]=DXTri(numtri-1,n_neighbors.r, neighbors.p);
	  
	  suspectedges[3].first = n_vertices.p;
	  suspectedges[3].second = n_vertices.r;
	  suspectedges[3].tri1 = numtri;
	  suspectedges[3].tri2 = n_neighbors.r;
	  
	  GetNeighbors(neighborlist, n_neighbors.r, &n);  
	  /* update the neighbor of the old triangle neighbors.p */
	  if (n.p == neighbors.p) neighborlist[n_neighbors.r].p = numtri;
	  else if (n.q == neighbors.p) neighborlist[n_neighbors.r].q = numtri;
	  else if (n.r == neighbors.p) neighborlist[n_neighbors.r].r = numtri;
	  else {
	    DXSetError(ERROR_DATA_INVALID,"invalid neighbors");
	    goto error;
	  }
	}
	 
	else if ((n_vertices.p==vertices.p)&& (n_vertices.r==vertices.q)) {
	  trianglelist[neighbors.p]=DXTri(i, n_vertices.q, n_vertices.r);
	  neighborlist[neighbors.p]=DXTri(numtri,n_neighbors.q, j);
	  
	  suspectedges[2].first = n_vertices.q;
	  suspectedges[2].second = n_vertices.r;
	  suspectedges[2].tri1 = neighbors.p;
	  suspectedges[2].tri2 = n_neighbors.q;
	  
	  trianglelist[numtri]=DXTri(i, n_vertices.p, n_vertices.q);
	  neighborlist[numtri]=DXTri(numtri-1, n_neighbors.p, neighbors.p); 
	  
	  suspectedges[3].first = n_vertices.p;
	  suspectedges[3].second = n_vertices.q;
	  suspectedges[3].tri1 = numtri;
	  suspectedges[3].tri2 = n_neighbors.p;
	  
	  GetNeighbors(neighborlist, n_neighbors.p, &n);  
	  /* update the neighbor of the old triangle neighbors.p */
	  if (n.p == neighbors.p) neighborlist[n_neighbors.p].p = numtri;
	  else if (n.q == neighbors.p) neighborlist[n_neighbors.p].q = numtri;
	  else if (n.r == neighbors.p) neighborlist[n_neighbors.p].r = numtri;
	  else {
	    DXSetError(ERROR_DATA_INVALID,"invalid neighbors");
	    goto error;
	  }
	}
	 
	else if ((n_vertices.p==vertices.q)&& (n_vertices.r==vertices.p)) {
	  trianglelist[neighbors.p]=DXTri(i, n_vertices.q, n_vertices.p);
	  neighborlist[neighbors.p]=DXTri(numtri,n_neighbors.p, j);
	  
	  suspectedges[2].first = n_vertices.q;
	  suspectedges[2].second = n_vertices.p;
	  suspectedges[2].tri1 = neighbors.p;
	  suspectedges[2].tri2 = n_neighbors.p;
	  
	  trianglelist[numtri]=DXTri(i, n_vertices.r, n_vertices.q);
	  neighborlist[numtri]=DXTri(numtri-1, n_neighbors.q, neighbors.p); 
	  
	  suspectedges[3].first = n_vertices.r;
	  suspectedges[3].second = n_vertices.q;
	  suspectedges[3].tri1 = numtri;
	  suspectedges[3].tri2 = n_neighbors.q;
	  
	  GetNeighbors(neighborlist, n_neighbors.q, &n);  
	  /* update the neighbor of the old triangle neighbors.p */
	  if (n.p == neighbors.p) neighborlist[n_neighbors.q].p = numtri;
	  else if (n.q == neighbors.p) neighborlist[n_neighbors.q].q = numtri;
	  else if (n.r == neighbors.p) neighborlist[n_neighbors.q].r = numtri;
	  else {
	    DXSetError(ERROR_DATA_INVALID,"invalid neighbors");
	    goto error;
	  }
	}
	
	else {
	  DXSetError(ERROR_INTERNAL,"neighbors are incorrect");
	  goto error;
	}
	numtri++;
      }
      else if (whichdegenerate==2) {
	/* new q r is degenerate */
	trianglelist[j] = DXTri(i, vertices.p, vertices.q);
	neighborlist[j] = DXTri(numtri, neighbors.p, neighbors.q);
	
	suspectedges[0].first = vertices.p;
	suspectedges[0].second = vertices.q;
	suspectedges[0].tri1 = j;
	suspectedges[0].tri2 = neighbors.p;

       /* need to update the old neighbor of j along the pr side */
        /* which is theseneighbors[2] */

        if (((trianglelist[theseneighbors[2]].p == vertices.p)&&
            (trianglelist[theseneighbors[2]].q == vertices.r)) ||
           ((trianglelist[theseneighbors[2]].p == vertices.r)&&
            (trianglelist[theseneighbors[2]].q == vertices.p)))
           neighborlist[theseneighbors[2]].p = numtri;
        else if (((trianglelist[theseneighbors[2]].q == vertices.p)&&
                  (trianglelist[theseneighbors[2]].r == vertices.r)) ||
                 ((trianglelist[theseneighbors[2]].q == vertices.r)&&
                  (trianglelist[theseneighbors[2]].r == vertices.p)))
           neighborlist[theseneighbors[2]].q = numtri;
        else if (((trianglelist[theseneighbors[2]].r == vertices.p)&&
                  (trianglelist[theseneighbors[2]].p == vertices.r)) ||
                 ((trianglelist[theseneighbors[2]].r == vertices.r)&&
                  (trianglelist[theseneighbors[2]].p == vertices.p)))
           neighborlist[theseneighbors[2]].r = numtri;
        else {
          DXSetError(ERROR_INTERNAL,"invalid neighbors");
          goto error;
        }


	
	trianglelist[numtri] = DXTri(i, vertices.r, vertices.p);
	neighborlist[numtri] = DXTri(numtri+1, neighbors.r, j);
	
	suspectedges[1].first = vertices.r;
	suspectedges[1].second = vertices.p;
	suspectedges[1].tri1 = numtri;
	suspectedges[1].tri2 = neighbors.r;

	numtri++;
	
	GetVertices(trianglelist, neighbors.q, &n_vertices);
	GetNeighbors(neighborlist, neighbors.q, &n_neighbors);
	
	/* need to update the old neighbor of j along the q r side (we
	   need to split that triangle) Depends on how that neighbor
	   is oriented */
	
	if ((n_vertices.q==vertices.r)&& (n_vertices.r==vertices.q)) {
	  trianglelist[neighbors.q]=DXTri(i, n_vertices.r, n_vertices.p);
	  neighborlist[neighbors.q]=DXTri(j,n_neighbors.r, numtri);
	  
	  suspectedges[2].first = n_vertices.r;
	  suspectedges[2].second = n_vertices.p;
	  suspectedges[2].tri1 = neighbors.q;
          /* XXX changed n_neighbors.q to neighbors.q above */
	  suspectedges[2].tri2 = n_neighbors.r;
	  
	  trianglelist[numtri]=DXTri(i, n_vertices.p, n_vertices.q);
	  neighborlist[numtri]=DXTri(neighbors.q, n_neighbors.p, numtri-1);
	  
	  suspectedges[3].first = n_vertices.p;
	  suspectedges[3].second = n_vertices.q;
	  suspectedges[3].tri1 = numtri;
	  suspectedges[3].tri2 = n_neighbors.p;
	  
	  GetNeighbors(neighborlist, n_neighbors.p, &n);  
	  /* update the neighbor of the old triangle neighbors.p */
	  if (n.p == neighbors.q) neighborlist[n_neighbors.p].p = numtri;
	  else if (n.q == neighbors.q) neighborlist[n_neighbors.p].q = numtri;
	  else if (n.r == neighbors.q) neighborlist[n_neighbors.p].r = numtri;
	  else {
	    DXSetError(ERROR_DATA_INVALID,"invalid neighbors");
	    goto error;
	  }
	}
	
	else if ((n_vertices.r==vertices.r)&& (n_vertices.q==vertices.q)) {
	  trianglelist[neighbors.q]=DXTri(i, n_vertices.q, n_vertices.p);
	  neighborlist[neighbors.q]=DXTri(j,n_neighbors.p, numtri);
	  
	  suspectedges[2].first = n_vertices.q;
	  suspectedges[2].second = n_vertices.p;
	  suspectedges[2].tri1 = neighbors.q;
	  suspectedges[2].tri2 = n_neighbors.p;
	  
	  trianglelist[numtri]=DXTri(i, n_vertices.p, n_vertices.r);
	  neighborlist[numtri]=DXTri(neighbors.q, n_neighbors.r, numtri-1);
	  
	  suspectedges[3].first = n_vertices.p;
	  suspectedges[3].second = n_vertices.r;
	  suspectedges[3].tri1 = numtri;
	  suspectedges[3].tri2 = n_neighbors.r;
	  
	  GetNeighbors(neighborlist, n_neighbors.r, &n);  
	  /* update the neighbor of the old triangle neighbors.p */
	  if (n.p == neighbors.q) neighborlist[n_neighbors.r].p = numtri;
	  else if (n.q == neighbors.q) neighborlist[n_neighbors.r].q = numtri;
	  else if (n.r == neighbors.q) neighborlist[n_neighbors.r].r = numtri;
	  else {
	    DXSetError(ERROR_DATA_INVALID,"invalid neighbors");
	    goto error;
	  }
	}
	
	else if ((n_vertices.p==vertices.r)&& (n_vertices.q==vertices.q)) {
	  trianglelist[neighbors.q]=DXTri(i, n_vertices.q, n_vertices.r);
	  neighborlist[neighbors.q]=DXTri(j,n_neighbors.q, numtri);
	  
	  suspectedges[2].first = n_vertices.q;
	  suspectedges[2].second = n_vertices.r;
	  suspectedges[2].tri1 = neighbors.q;
	  suspectedges[2].tri2 = n_neighbors.q;
	  
	  trianglelist[numtri]=DXTri(i, n_vertices.r, n_vertices.p);
	  neighborlist[numtri]=DXTri(neighbors.q, n_neighbors.r, numtri-1);
	  
	  suspectedges[3].first = n_vertices.r;
	  suspectedges[3].second = n_vertices.p;
	  suspectedges[3].tri1 = numtri;
	  suspectedges[3].tri2 = n_neighbors.r;
	  
	  GetNeighbors(neighborlist, n_neighbors.r, &n);  
	  /* update the neighbor of the old triangle neighbors.p */
	  if (n.p == neighbors.q) neighborlist[n_neighbors.r].p = numtri;
	  else if (n.q == neighbors.q) neighborlist[n_neighbors.r].q = numtri;
	  else if (n.r == neighbors.q) neighborlist[n_neighbors.r].r = numtri;
	  else {
	    DXSetError(ERROR_DATA_INVALID,"invalid neighbors");
	    goto error;
	  }
	}
	
	else if ((n_vertices.q==vertices.r)&& (n_vertices.p==vertices.q)) {
	  trianglelist[neighbors.q]=DXTri(i, n_vertices.p, n_vertices.r);
	  neighborlist[neighbors.q]=DXTri(j,n_neighbors.r, numtri);
	  
	  suspectedges[2].first = n_vertices.p;
	  suspectedges[2].second = n_vertices.r;
	  suspectedges[2].tri1 = neighbors.q;
	  suspectedges[2].tri2 = n_neighbors.r;
	  
	  trianglelist[numtri]=DXTri(i, n_vertices.r, n_vertices.q);
	  neighborlist[numtri]=DXTri(neighbors.q, n_neighbors.q, numtri-1);
	  
	  suspectedges[3].first = n_vertices.r;
	  suspectedges[3].second = n_vertices.q;
	  suspectedges[3].tri1 = numtri;
	  suspectedges[3].tri2 = n_neighbors.q;

	  GetNeighbors(neighborlist, n_neighbors.q, &n);  
	  /* update the neighbor of the old triangle neighbors.p */
	  if (n.p == neighbors.q) neighborlist[n_neighbors.q].p = numtri;
	  else if (n.q == neighbors.q) neighborlist[n_neighbors.q].q = numtri;
	  else if (n.r == neighbors.q) neighborlist[n_neighbors.q].r = numtri;
	  else {
	    DXSetError(ERROR_DATA_INVALID,"invalid neighbors");
	    goto error;
	  }
	}
	
	else if ((n_vertices.r==vertices.r)&& (n_vertices.p==vertices.q)) {
	  trianglelist[neighbors.q]=DXTri(i, n_vertices.p, n_vertices.q);
	  neighborlist[neighbors.q]=DXTri(j,n_neighbors.p, numtri);
	  
	  suspectedges[2].first = n_vertices.p;
	  suspectedges[2].second = n_vertices.q;
	  suspectedges[2].tri1 = neighbors.q;
	  suspectedges[2].tri2 = n_neighbors.p;
	  
	  trianglelist[numtri]=DXTri(i, n_vertices.q, n_vertices.r);
	  neighborlist[numtri]=DXTri(neighbors.q, n_neighbors.q, numtri-1);
	  
	  suspectedges[3].first = n_vertices.q;
	  suspectedges[3].second = n_vertices.r;
	  suspectedges[3].tri1 = numtri;
	  suspectedges[3].tri2 = n_neighbors.q;
	  
	  GetNeighbors(neighborlist, n_neighbors.q, &n);  
	  /* update the neighbor of the old triangle neighbors.p */
	  if (n.p == neighbors.q) neighborlist[n_neighbors.q].p = numtri;
	  else if (n.q == neighbors.q) neighborlist[n_neighbors.q].q = numtri;
	  else if (n.r == neighbors.q) neighborlist[n_neighbors.q].r = numtri;
	  else {
	    DXSetError(ERROR_DATA_INVALID,"invalid neighbors");
	    goto error;
	  }
	}
	else if ((n_vertices.p==vertices.r)&& (n_vertices.r==vertices.q)) {
	  trianglelist[neighbors.q]=DXTri(i, n_vertices.r, n_vertices.q);
	  neighborlist[neighbors.q]=DXTri(j,n_neighbors.q, numtri);
	  
	  suspectedges[2].first = n_vertices.r;
	  suspectedges[2].second = n_vertices.q;
	  suspectedges[2].tri1 = neighbors.q;
	  suspectedges[2].tri2 = n_neighbors.q;
	  
	  trianglelist[numtri]=DXTri(i, n_vertices.q, n_vertices.p);
	  neighborlist[numtri]=DXTri(neighbors.q, n_neighbors.p, numtri-1);
	  
	  suspectedges[3].first = n_vertices.q;
	  suspectedges[3].second = n_vertices.p;
	  suspectedges[3].tri1 = numtri;
	  suspectedges[3].tri2 = n_neighbors.p;
	  
	  GetNeighbors(neighborlist, n_neighbors.p, &n);  
	  /* update the neighbor of the old triangle neighbors.p */
	  if (n.p == neighbors.q) neighborlist[n_neighbors.p].p = numtri;
	  else if (n.q == neighbors.q) neighborlist[n_neighbors.p].q = numtri;
	  else if (n.r == neighbors.q) neighborlist[n_neighbors.p].r = numtri;
	  else {
	    DXSetError(ERROR_DATA_INVALID,"invalid neighbors");
	    goto error;
	  }
	}
	else {
	  DXSetError(ERROR_INTERNAL,"neighbors are incorrect");
	  goto error;
	}
	numtri++;
      }
      /* else whichdegenerate==3 */
      else {
	/* r p new is degenerate */
	trianglelist[j] = DXTri(i, vertices.q, vertices.r);
	neighborlist[j] = DXTri(numtri, neighbors.q, neighbors.r);
	
	suspectedges[0].first = vertices.q;
	suspectedges[0].second = vertices.r;
	suspectedges[0].tri1 = j;
	suspectedges[0].tri2 = neighbors.q;


       /* need to update the old neighbor of j along the pq side */
        /* which is theseneighbors[0] */

        if (((trianglelist[theseneighbors[0]].p == vertices.p)&&
            (trianglelist[theseneighbors[0]].q == vertices.q)) ||
           ((trianglelist[theseneighbors[0]].p == vertices.q)&&
            (trianglelist[theseneighbors[0]].q == vertices.p)))
           neighborlist[theseneighbors[0]].p = numtri;
        else if (((trianglelist[theseneighbors[0]].q == vertices.p)&&
                  (trianglelist[theseneighbors[0]].r == vertices.q)) ||
                 ((trianglelist[theseneighbors[0]].q == vertices.q)&&
                  (trianglelist[theseneighbors[0]].r == vertices.p)))
           neighborlist[theseneighbors[0]].q = numtri;
        else if (((trianglelist[theseneighbors[0]].r == vertices.p)&&
                  (trianglelist[theseneighbors[0]].p == vertices.q)) ||
                 ((trianglelist[theseneighbors[0]].r == vertices.q)&&
                  (trianglelist[theseneighbors[0]].p == vertices.p)))
           neighborlist[theseneighbors[0]].r = numtri;
        else {
          DXSetError(ERROR_INTERNAL,"invalid neighbors");
          goto error;
        }

	
	trianglelist[numtri] = DXTri(i, vertices.p, vertices.q);
	neighborlist[numtri] = DXTri(numtri+1, neighbors.p, j);
	
	suspectedges[1].first = vertices.p;
	suspectedges[1].second = vertices.q;
	suspectedges[1].tri1 = numtri;
	suspectedges[1].tri2 = neighbors.p;
	
	numtri++;
	
	GetVertices(trianglelist, neighbors.r, &n_vertices);
	GetNeighbors(neighborlist, neighbors.r, &n_neighbors);
	
	/* need to update the old neighbor of j along the r p side (we
	   need to split that triangle) Depends on how that neighbor
	   is oriented */
	
	if ((n_vertices.p==vertices.p)&& (n_vertices.q==vertices.r)) {
	  trianglelist[neighbors.r]=DXTri(i, n_vertices.q, n_vertices.r);
	  neighborlist[neighbors.r]=DXTri(j, n_neighbors.q, numtri);
	  
	  suspectedges[2].first = n_vertices.q;
	  suspectedges[2].second = n_vertices.r;
	  suspectedges[2].tri1 = neighbors.r;
	  suspectedges[2].tri2 = n_neighbors.q;
	  
	  trianglelist[numtri]=DXTri(i, n_vertices.r, n_vertices.p);
	  neighborlist[numtri]=DXTri(neighbors.r, n_neighbors.r, numtri-1);
	  
	  suspectedges[3].first = n_vertices.r;
	  suspectedges[3].second = n_vertices.p;
	  suspectedges[3].tri1 = numtri;
	  suspectedges[3].tri2 = n_neighbors.r;
	  
	  GetNeighbors(neighborlist, n_neighbors.r, &n);  
	  /* update the neighbor of the old triangle neighbors.p */
	  if (n.p == neighbors.r) neighborlist[n_neighbors.r].p = numtri;
	  else if (n.q == neighbors.r) neighborlist[n_neighbors.r].q = numtri;
	  else if (n.r == neighbors.r) neighborlist[n_neighbors.r].r = numtri;
	  else {
	    DXSetError(ERROR_DATA_INVALID,"invalid neighbors");
	    goto error;
	  }
	}
	else if ((n_vertices.q==vertices.p)&& (n_vertices.p==vertices.r)) {
	  trianglelist[neighbors.r]=DXTri(i, n_vertices.r, n_vertices.p);
	  neighborlist[neighbors.r]=DXTri(numtri, n_neighbors.r, j);
	  
	  suspectedges[2].first = n_vertices.r;
	  suspectedges[2].second = n_vertices.p;
	  suspectedges[2].tri1 = neighbors.r;
	  suspectedges[2].tri2 = n_neighbors.r;
	  
	  trianglelist[numtri]=DXTri(i, n_vertices.q, n_vertices.r);
	  neighborlist[numtri]=DXTri(numtri-1,n_neighbors.q, neighbors.r);
	  
	  suspectedges[3].first = n_vertices.q;
	  suspectedges[3].second = n_vertices.r;
	  suspectedges[3].tri1 = numtri;
	  suspectedges[3].tri2 = n_neighbors.q;
	  
	  GetNeighbors(neighborlist, n_neighbors.q, &n);  
	  /* update the neighbor of the old triangle neighbors.p */
	  if (n.p == neighbors.r) neighborlist[n_neighbors.q].p = numtri;
	  else if (n.q == neighbors.r) neighborlist[n_neighbors.q].q = numtri;
	  else if (n.r == neighbors.r) neighborlist[n_neighbors.q].r = numtri;
	  else {
	    DXSetError(ERROR_DATA_INVALID,"invalid neighbors");
	    goto error;
	  }
	}
	
	else if ((n_vertices.r==vertices.p)&& (n_vertices.p==vertices.r)) {
	  trianglelist[neighbors.r]=DXTri(i, n_vertices.p, n_vertices.q);
	  neighborlist[neighbors.r]=DXTri(j,n_neighbors.p, numtri);
	  
	  suspectedges[2].first = n_vertices.p;
	  suspectedges[2].second = n_vertices.q;
	  suspectedges[2].tri1 = neighbors.r;
	  suspectedges[2].tri2 = n_neighbors.p;
	  
	  trianglelist[numtri]=DXTri(i, n_vertices.q, n_vertices.r);
	  neighborlist[numtri]=DXTri(neighbors.r, n_neighbors.q, numtri-1);
	  
	  suspectedges[3].first = n_vertices.q;
	  suspectedges[3].second = n_vertices.r;
	  suspectedges[3].tri1 = numtri;
	  suspectedges[3].tri2 = n_neighbors.q;
	  
	  GetNeighbors(neighborlist, n_neighbors.q, &n);  
	  /* update the neighbor of the old triangle neighbors.p */
	  if (n.p == neighbors.r) neighborlist[n_neighbors.q].p = numtri;
	  else if (n.q == neighbors.r) neighborlist[n_neighbors.q].q = numtri;
	  else if (n.r == neighbors.r) neighborlist[n_neighbors.q].r = numtri;
	  else {
	    DXSetError(ERROR_DATA_INVALID,"invalid neighbors");
	    goto error;
	  }
	}
	
	else if ((n_vertices.p==vertices.p)&& (n_vertices.r==vertices.r)) {
	  trianglelist[neighbors.r]=DXTri(i, n_vertices.r, n_vertices.q);
	  neighborlist[neighbors.r]=DXTri(j,n_neighbors.q, numtri);
	  
	  suspectedges[2].first = n_vertices.r;
	  suspectedges[2].second = n_vertices.q;
	  suspectedges[2].tri1 = neighbors.r;
	  suspectedges[2].tri2 = n_neighbors.q;
	  
	  trianglelist[numtri]=DXTri(i, n_vertices.q, n_vertices.p);
	  neighborlist[numtri]=DXTri(neighbors.r, n_neighbors.p, numtri-1);
	  
	  suspectedges[3].first = n_vertices.q;
	  suspectedges[3].second = n_vertices.p;
	  suspectedges[3].tri1 = numtri;
	  suspectedges[3].tri2 = n_neighbors.p;
	  
	  GetNeighbors(neighborlist, n_neighbors.p, &n);  
	  /* update the neighbor of the old triangle neighbors.p */
	  if (n.p == neighbors.r) neighborlist[n_neighbors.p].p = numtri;
	  else if (n.q == neighbors.r) neighborlist[n_neighbors.p].q = numtri;
	  else if (n.r == neighbors.r) neighborlist[n_neighbors.p].r = numtri;
	  else {
	    DXSetError(ERROR_DATA_INVALID,"invalid neighbors");
	    goto error;
	  }
	}
	 
	else if ((n_vertices.q==vertices.p)&& (n_vertices.r==vertices.r)) {
	  trianglelist[neighbors.r]=DXTri(i, n_vertices.r, n_vertices.p);
	  neighborlist[neighbors.r]=DXTri(j,n_neighbors.r, numtri);
	  
	  suspectedges[2].first = n_vertices.r;
	  suspectedges[2].second = n_vertices.p;
	  suspectedges[2].tri1 = neighbors.r;
	  suspectedges[2].tri2 = n_neighbors.r;
	  
	  trianglelist[numtri]=DXTri(i, n_vertices.p, n_vertices.q);
	  neighborlist[numtri]=DXTri(neighbors.r, n_neighbors.p, numtri-1);
	  
	  suspectedges[3].first = n_vertices.p;
	  suspectedges[3].second = n_vertices.q;
	  suspectedges[3].tri1 = numtri;
	  suspectedges[3].tri2 = n_neighbors.p;
	  
	  GetNeighbors(neighborlist, n_neighbors.p, &n);  
	  /* update the neighbor of the old triangle neighbors.p */
	  if (n.p == neighbors.r) neighborlist[n_neighbors.p].p = numtri;
	  else if (n.q == neighbors.r) neighborlist[n_neighbors.p].q = numtri;
	  else if (n.r == neighbors.r) neighborlist[n_neighbors.p].r = numtri;
	  else {
	    DXSetError(ERROR_DATA_INVALID,"invalid neighbors");
	    goto error;
	  }
	}
	 
	else if ((n_vertices.r==vertices.p)&& (n_vertices.q==vertices.r)) {
	  trianglelist[neighbors.r]=DXTri(i, n_vertices.q, n_vertices.p);
	  neighborlist[neighbors.r]=DXTri(j,n_neighbors.p, numtri);
	  
	  suspectedges[2].first = n_vertices.q;
	  suspectedges[2].second = n_vertices.p;
	  suspectedges[2].tri1 = neighbors.r;
	  suspectedges[2].tri2 = n_neighbors.p;
	  
	  trianglelist[numtri]=DXTri(i, n_vertices.p, n_vertices.r);
	  neighborlist[numtri]=DXTri(neighbors.r, n_neighbors.r, numtri-1);
	  
	  suspectedges[3].first = n_vertices.p;
	  suspectedges[3].second = n_vertices.r;
	  suspectedges[3].tri1 = numtri;
	  suspectedges[3].tri2 = n_neighbors.r;
	  
	  GetNeighbors(neighborlist, n_neighbors.r, &n);  
	  /* update the neighbor of the old triangle neighbors.p */
	  if (n.p == neighbors.r) neighborlist[n_neighbors.r].p = numtri;
	  else if (n.q == neighbors.r) neighborlist[n_neighbors.r].q = numtri;
	  else if (n.r == neighbors.r) neighborlist[n_neighbors.r].r = numtri;
	  else {
	    DXSetError(ERROR_DATA_INVALID,"invalid neighbors");
	    goto error;
	  }
	}
	
	else {
	  DXSetError(ERROR_INTERNAL,"neighbors are incorrect");
	  goto error;
	}
	numtri++;
      } 
    }
    /* else there aren't any degeneracies */
    else {
      numsuspect=3;
      trianglelist[j] = DXTri(vertices.p, vertices.q, i);
      neighborlist[j] = DXTri(neighbors.p, numtri, numtri+1);
      
      suspectedges[0].first = vertices.p;
      suspectedges[0].second = vertices.q;
      suspectedges[0].tri1 = j;
      suspectedges[0].tri2 = neighbors.p;
      
      trianglelist[numtri] = DXTri(i, vertices.q, vertices.r);
      neighborlist[numtri] = DXTri(j, neighbors.q, numtri+1);
      
      suspectedges[1].first = vertices.q;
      suspectedges[1].second = vertices.r;
      suspectedges[1].tri1 = numtri;
      suspectedges[1].tri2 = neighbors.q;
      
      /* need to update the old neighbor of j along the q r side,
       * which is theseneighbors[1] */
      if (((trianglelist[theseneighbors[1]].p == vertices.q)&&
	   (trianglelist[theseneighbors[1]].q == vertices.r)) ||
	  ((trianglelist[theseneighbors[1]].p == vertices.r)&&  
	   (trianglelist[theseneighbors[1]].q == vertices.q))) 
	neighborlist[theseneighbors[1]].p = numtri;
      else if (((trianglelist[theseneighbors[1]].q == vertices.q)&&
		(trianglelist[theseneighbors[1]].r == vertices.r)) ||
	       ((trianglelist[theseneighbors[1]].q == vertices.r)&&  
		(trianglelist[theseneighbors[1]].r == vertices.q))) 
	neighborlist[theseneighbors[1]].q = numtri;
      else if (((trianglelist[theseneighbors[1]].r == vertices.q)&&
		(trianglelist[theseneighbors[1]].p == vertices.r)) ||
	       ((trianglelist[theseneighbors[1]].r == vertices.r)&&  
		(trianglelist[theseneighbors[1]].p == vertices.q))) 
	neighborlist[theseneighbors[1]].r = numtri;
      else {
	DXSetError(ERROR_INTERNAL,"invalid neighbors"); 
	goto error;
      }
      

      numtri++;
      trianglelist[numtri] = DXTri(vertices.p, i, vertices.r);
      neighborlist[numtri] = DXTri(j, numtri-1, neighbors.r);
      
      suspectedges[2].first = vertices.p;
      suspectedges[2].second = vertices.r;
      suspectedges[2].tri1 = numtri;
      suspectedges[2].tri2 = neighbors.r;
      
      /* need to update the old neighbor of j along the p r side,
       * which is theseneighbors[2] */
      if (((trianglelist[theseneighbors[2]].p == vertices.p)&&
	   (trianglelist[theseneighbors[2]].q == vertices.r)) ||
	  ((trianglelist[theseneighbors[2]].p == vertices.r)&&  
	   (trianglelist[theseneighbors[2]].q == vertices.p))) 
	neighborlist[theseneighbors[2]].p = numtri;
      else if (((trianglelist[theseneighbors[2]].q == vertices.p)&&
		(trianglelist[theseneighbors[2]].r == vertices.r)) ||
	       ((trianglelist[theseneighbors[2]].q == vertices.r)&&  
		(trianglelist[theseneighbors[2]].r == vertices.p))) 
	neighborlist[theseneighbors[2]].q = numtri;
      else if (((trianglelist[theseneighbors[2]].r == vertices.p)&&
		(trianglelist[theseneighbors[2]].p == vertices.r)) ||
	       ((trianglelist[theseneighbors[2]].r == vertices.r)&&  
		(trianglelist[theseneighbors[2]].p == vertices.p))) 
	neighborlist[theseneighbors[2]].r = numtri;
      else {
	DXSetError(ERROR_INTERNAL,"invalid neighbors"); 
	goto error;
      }
      
      numtri++;
    }

    
    /* now we're ready to start flipping bonds */
    /* need to check all the suspect edges. We start with either three
     * or four suspect edges (three if the new point was strictly inside
     * a triangle; four if it was on an edge) */
    
    FlipEm(numsuspect, suspectedges, trianglelist, neighborlist, pos_ptr);

#if DEBUG
    if (!CheckThem(neighborlist, trianglelist, numtri)) {
       DXSetError(ERROR_INTERNAL, "Check Failed i = %d", i);
       goto error;
    }
#endif
    
  }

  /* we've munged the connections */
  if (!DXChangedComponentStructure((Field)ino,"connections"))
    goto error;
  
  connections = (Array)DXNewArray(TYPE_INT, CATEGORY_REAL, 1, 3);
  if (!connections) goto error;
  
  /* we don't want to add any triangles with -1 to 2 in them */
  /* there will be less than numtri triangles */
  if (!DXAllocateArray(connections, numtri))
    goto error;
  
  trianglecount = 0; 
  for (i=0; i<numtri; i++) {
    triangle = trianglelist[i];
    if ((triangle.p >= 3) && (triangle.q >= 3) && (triangle.r >= 3)) {
      if (Handedness(pos_ptr, triangle.p, triangle.q, triangle.r)==-1) {
	triangle = 
	  DXTri(triangle.p, triangle.r, triangle.q);
      }
      /* subtract off the three added points at the beginning */
      triangle.p = triangle.p-3;
      triangle.q = triangle.q-3;
      triangle.r = triangle.r-3;
      if (!DXAddArrayData(connections, trianglecount, 1, &triangle))
	goto error;
      trianglecount++;
    }
  }
  if (trianglecount == 0) {
     DXSetError(ERROR_DATA_INVALID,
                "all triangles were degenerate");
     goto error;
  } 
  
  
  if (!DXSetComponentValue((Field)ino,"connections", (Object)connections))
    goto error;
  connections=NULL;
  if (!DXSetComponentAttribute((Field)ino, "connections",
			       "element type",
			       (Object)DXNewString("triangles")))
    goto error;
  if (!DXSetComponentAttribute((Field)ino, "connections",
			       "ref",
			       (Object)DXNewString("positions")))
    goto error;


  /* get rid of the invalid positions */
  if (!DXInvalidateConnections((Object)ino))
    goto error;
  if (!DXInvalidateUnreferencedPositions((Object)ino))
    goto error;
  if (!DXCull((Object)ino))
    goto error;

  /* check how many triangles got made */


  if (!DXEndField((Field)ino))
    goto error;

  DXGetArrayInfo(connections, &trianglecount, NULL, NULL, NULL, NULL);
  if (trianglecount == 0) {
     DXWarning("no triangles created; all were degenerate");
  }
 
  DXFree((Pointer)trianglelist);
  DXFree((Pointer)neighborlist);
  DXFree((Pointer)pos_ptr);
  DXFreeInvalidComponentHandle(handle);
  if (shape[0]==3) DXFree((Pointer)old_pos_ptr);
  return OK;
 error:
  DXFree((Pointer)trianglelist);
  DXFreeInvalidComponentHandle(handle);
  DXFree((Pointer)neighborlist);
  DXFree((Pointer)pos_ptr);
  DXDelete((Object)connections);
  if (shape[0]==3) DXFree((Pointer)old_pos_ptr);
  DXAbortTaskGroup();
  return ERROR;
}

static Error FlipEm(int numsuspect, edge *edges, Triangle *trianglelist,
                    Triangle *neighborlist, float *pos_ptr)
{

  int k, index1, index2, tri1, tri2;
  int A, B, C, D;
  edge *suspectedges;
  Triangle vertices1, vertices2;
  float angle1, angle2;

 
  suspectedges = DXAllocate(2*sizeof(edge));
  if (!suspectedges)
     return ERROR; 
  
  for (k = 0; k<numsuspect; k++) {
    /* check the edge */
    index1 = edges[k].first;
    index2 = edges[k].second;
    tri1 = edges[k].tri1;
    tri2 = edges[k].tri2;
    
    /* need to get A, B, C, D around the quad to figure out if we
       should flip it or not */
    GetVertices(trianglelist, tri1, &vertices1);
    GetVertices(trianglelist, tri2, &vertices2);
    A = Not(vertices1, index1, index2);
    B = index2;
    C = Not(vertices2, index1, index2);
    D = index1;
    

    /*

                           A


                        D------B


                           C
     */


    /* now check if this quad needs to be flipped. It should be flipped if   
       angle(DAB)+angle(BCD) > angle(ABC)+angle(CDA)
       */
    
    /* don't flip if the outer triangle is one of the three boundary tris */
    if (tri2 > 2) {

      /* don't flip if the quad is not convex */
      if (Convex(pos_ptr, A, B, C, D)) {
	
	angle1 = GetAngle(pos_ptr, D, A, B) + GetAngle(pos_ptr, B, C, D);
	angle2 = GetAngle(pos_ptr, A, B, C) + GetAngle(pos_ptr, C, D, A);
	
	if (angle1 > angle2) {
	  if (!DoTheFlip(A, B, C, D, tri1, tri2, 
		    trianglelist, neighborlist, suspectedges))
              return ERROR;
	  /* two more suspect edges */
	  /* A is supposed to be our new point, by the way */
	  FlipEm(2, suspectedges, trianglelist, neighborlist, pos_ptr);  
	}
      }
    }
  }
  DXFree((Pointer)suspectedges); 
  return OK;
}

static Error DoTheFlip(int A, int B, int C, int D, int first, int second,
                       Triangle *trianglelist, 
                       Triangle *neighborlist, edge *suspectedges)
{
  /* given an edge which should be flipped (BD), it flips it,
   * and also returns the two new suspect edges in suspectedges 
   */
  /* The suspect edges are BC and CD */
  
  int n11, n21, n31, n12, n22, n32, n1old, n2old, neighborBC, neighborCD;
  
  neighborBC = NeighborEdge(second, B, C, trianglelist, neighborlist);
  neighborCD = NeighborEdge(second, C, D, trianglelist, neighborlist);
  
  suspectedges[0].first = B;
  suspectedges[0].second = C;
  suspectedges[0].tri1 = first;
  suspectedges[0].tri2 = neighborBC;
  suspectedges[1].first = C;
  suspectedges[1].second = D;
  suspectedges[1].tri1 = second;
  suspectedges[1].tri2 = neighborCD;
  
  n1old = NeighborEdge(first, A, D, trianglelist, neighborlist);
  n2old = NeighborEdge(second, B, C, trianglelist, neighborlist);
  
  n11 = NeighborEdge(first, A, B, trianglelist, neighborlist);
  n21 = NeighborEdge(second, B, C, trianglelist, neighborlist);
  n31 = second;
  
  n12 = NeighborEdge(second, C, D, trianglelist, neighborlist);
  n22 = NeighborEdge(first, D, A, trianglelist, neighborlist);
  n32 = first;
  
  trianglelist[first] = DXTri(A, B, C);
  neighborlist[first] = DXTri(n11, n21, n31);
  trianglelist[second] = DXTri(C, D, A);
  neighborlist[second] = DXTri(n12, n22, n32);
  
  /* need to update the old neighbor of first along the AD side */
  
  if (((trianglelist[n1old].p == A)&&
       (trianglelist[n1old].q == D)) ||
      ((trianglelist[n1old].p == D)&&
       (trianglelist[n1old].q == A)))
    neighborlist[n1old].p = second;
  
  else if (((trianglelist[n1old].q == A)&&
	    (trianglelist[n1old].r == D)) ||
	   ((trianglelist[n1old].q == D)&&
	    (trianglelist[n1old].r == A)))
    neighborlist[n1old].q = second;
  else if (((trianglelist[n1old].r == A)&&
	    (trianglelist[n1old].p == D)) ||
	   ((trianglelist[n1old].r == D)&&
	    (trianglelist[n1old].p == A)))
    neighborlist[n1old].r = second;
  else {
    DXSetError(ERROR_INTERNAL,"invalid neighbors");
    return ERROR;
  }

  /* need to update the old neighbor of second along the BC side */
  
  if (((trianglelist[n2old].p == B)&&
       (trianglelist[n2old].q == C)) ||
      ((trianglelist[n2old].p == C)&&
       (trianglelist[n2old].q == B)))
    neighborlist[n2old].p = first;
  
  else if (((trianglelist[n2old].q == B)&&
	    (trianglelist[n2old].r == C)) ||
	   ((trianglelist[n2old].q == C)&&
	    (trianglelist[n2old].r == B)))
    neighborlist[n2old].q = first;
  
  else if (((trianglelist[n2old].r == B)&&
	    (trianglelist[n2old].p == C)) ||
           ((trianglelist[n2old].r == C)&&
	    (trianglelist[n2old].p == B)))
    neighborlist[n2old].r = first;
  else {
    DXSetError(ERROR_INTERNAL,"invalid neighbors");
    return ERROR;
  }
  return OK;
}

static int NeighborEdge(int whichtri, int vert1, int vert2, 
			Triangle *trianglelist, Triangle *neighborlist)
{
  
  Triangle neighbors, vertices;
  
  
  GetVertices(trianglelist, whichtri, &vertices);
  GetNeighbors(neighborlist, whichtri, &neighbors);
  
  if (((vert1==vertices.p)&&(vert2==vertices.q)) ||
      ((vert2==vertices.p)&&(vert1==vertices.q)))
    return neighbors.p;
  if (((vert1==vertices.q)&&(vert2==vertices.r)) ||
      ((vert2==vertices.q)&&(vert1==vertices.r)))
    return neighbors.q;
  if (((vert1==vertices.r)&&(vert2==vertices.p)) ||
      ((vert2==vertices.r)&&(vert1==vertices.p)))
    return neighbors.r;

  return 0;
}

static float GetAngle(float *pos_ptr, int A, int B, int C)
{
  Point2D a, b, c;
  float angle, dotprod, mag;
  double factor;
  
  a.x = pos_ptr[2*A];
  a.y = pos_ptr[2*A+1];
  
  b.x = pos_ptr[2*B];
  b.y = pos_ptr[2*B+1];
  
  c.x = pos_ptr[2*C];
  c.y = pos_ptr[2*C+1];
  
  dotprod = (a.x - b.x)*(c.x - b.x) + (a.y - b.y)*(c.y - b.y);
  mag = sqrt((a.x-b.x)*(a.x-b.x) + (a.y-b.y)*(a.y-b.y)) * 
    sqrt((c.x-b.x)*(c.x-b.x) + (c.y-b.y)*(c.y-b.y));

  factor = dotprod/mag;
  if ((factor >= .99999)) {
     angle = 0.0;
  }
  else if ((factor <= -.99999)) {
     angle = 3.1415926;
  }
  else {
     angle = acos(factor);
  }
  return angle;
}

static int Not(Triangle tri, int i, int j) 
{
  if ((tri.p != i)&&(tri.p != j))
    return tri.p;
  else if ((tri.q != i)&&(tri.q != j))
    return tri.q;
  else if ((tri.r != i)&&(tri.r != j))
    return tri.r;

  return 0;
}

static int CheckThem(Triangle *neighborlist, Triangle *trianglelist,
                     int numtri)
{
   int i;
   Triangle vertices, neighbors;

   for (i=0; i<numtri; i++) {
      GetVertices(trianglelist, i, &vertices);
      GetNeighbors(neighborlist, i, &neighbors);
      
      /* check whether the first neighbor shares pq vertices */
      if (!(InTheSet(neighbors.p, trianglelist, vertices.p, vertices.q)))
        return 0;
      /* check whether the second neighbor shares qr vertices */
      if (!(InTheSet(neighbors.q, trianglelist, vertices.q, vertices.r)))
        return 0;
      /* check whether the third neighbor shares rp vertices */
      if (!(InTheSet(neighbors.r, trianglelist, vertices.r, vertices.p)))
        return 0;
   }
   return 1;
}

static int InTheSet(int neighbor, Triangle *trianglelist, int index1, int index2)
{
   Triangle vertices;

   GetVertices(trianglelist, neighbor, &vertices);
   if (((index1 == vertices.p)&&(index2 == vertices.q)) ||
       ((index1 == vertices.q)&&(index2 == vertices.p)) ||
       ((index1 == vertices.q)&&(index2 == vertices.r)) ||
       ((index1 == vertices.r)&&(index2 == vertices.q)) ||
       ((index1 == vertices.r)&&(index2 == vertices.p)) ||
       ((index1 == vertices.p)&&(index2 == vertices.r)))
        return 1;
   else
        return 0; 
}



