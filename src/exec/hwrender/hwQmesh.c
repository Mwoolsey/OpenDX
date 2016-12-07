/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/* $Source: /src/master/dx/src/exec/hwrender/hwQmesh.c,v $
 */

#include <dxconfig.h>

#if defined(HAVE_STRINGS_H)
#include <strings.h>
#endif

#if defined(HAVE_STRING_H)
#include <string.h>
#endif

#define tdmQmesh_c 

#include <stdio.h>
#include "hwDeclarations.h"
#include "hwXfield.h"
#include "hwQmesh.h"
#include "hwMemory.h"
#include "hwWindow.h"
#include "hwObjectHash.h"

#include "hwDebug.h"


typedef struct
{
   int *meshes;
   int nmeshes;
   int *connections;
   int nconnections;
   Array meshArray;
   Array connectionArray;
} QMeshS, *QMesh;

static Error
_deleteQMesh(void *p)
{
    QMesh t = (QMesh)p;
    if (t->connectionArray)
        DXDelete((dxObject)t->connectionArray);
    if (t->meshArray)
        DXDelete((dxObject)t->meshArray);
   
    DXFree(p);

    return OK;
}

static Error
_newQMesh(int nStrips, int nStrippedPts, QMesh *mesh, dxObject *o)
{
    Private priv = NULL;
    QMesh tmp = NULL;

    *mesh = NULL;
    *o = NULL;

    tmp = (QMesh)DXAllocateZero(sizeof(QMeshS));
    if (! tmp)
        return ERROR;

    tmp->meshArray = DXNewArray(TYPE_INT, CATEGORY_REAL, 1, 2);
    if (! tmp->meshArray)
        goto error;

    DXReference((dxObject)tmp->meshArray);

    if ( !DXAllocateArray (tmp->meshArray, nStrips))
        goto error;

    tmp->connectionArray = DXNewArray(TYPE_INT, CATEGORY_REAL, 0);
    if (! tmp->connectionArray)
        goto error;

    DXReference((dxObject)tmp->connectionArray);

    if ( !DXAllocateArray (tmp->connectionArray, nStrippedPts))
        goto error;

    tmp->meshes       = (int *)DXGetArrayData(tmp->meshArray);
    tmp->connections  = (int *)DXGetArrayData(tmp->connectionArray);
    tmp->nmeshes      = nStrips;
    tmp->nconnections = nStrippedPts;

    priv = (Private)DXNewPrivate((Pointer)tmp, _deleteQMesh);
    if (! priv)
       goto error;

    *mesh = tmp;
    *o = (dxObject)priv;

    return OK;

error:
    if (tmp)
    {
        if (tmp->connectionArray)
            DXDelete((dxObject)tmp->connectionArray);
        if (tmp->meshArray)
            DXDelete((dxObject)tmp->meshArray);

        DXFree((Pointer)tmp);
    }

    return ERROR;
}

static void
_installQMeshInfo(xfieldT *xf, QMesh qmesh)
{
    if (xf->connections)
        DXFreeArrayHandle(xf->connections);

    if (xf->connections_array)
        DXDelete((dxObject)xf->connections_array);

    xf->connections       = NULL;
    xf->connections_array = (Array)DXReference((dxObject)qmesh->connectionArray);
    xf->nconnections      = qmesh->nconnections;
    xf->meshes            = (Array)DXReference((dxObject)qmesh->meshArray);
    xf->nmeshes           = qmesh->nmeshes;
    xf->posPerConn        = 1;
    xf->connectionType    = ct_qmesh;
}

typedef struct _Quadruple
{
  int p[4] ;
} Quadruple ;


/* add point i in quad quad to the strip */
#define AddPoint(quad,i) (point[nPtsInStrip++] = quads[quad].p[i])


/* return the quad index that refers to point pt in quad quad */
#define LookUp(quad,pt)             \
  (                                 \
  quads[quad].p[0] == pt ? 0 :      \
  quads[quad].p[1] == pt ? 1 :      \
  quads[quad].p[2] == pt ? 2 :      \
  quads[quad].p[3] == pt ? 3 : -1   \
  )


/* return the direction (neighbor index) from q1 to q2 */
#define LookUpDir(q1,q2)             \
  (                                  \
  neighbors[q1].p[0] == q2 ? 0 :     \
  neighbors[q1].p[1] == q2 ? 1 :     \
  neighbors[q1].p[2] == q2 ? 2 :     \
  neighbors[q1].p[3] == q2 ? 3 : -1  \
  )


/* converts from Up to Down, Down to Up, Left to Right, Right to Left */
static int flip_flop[4] = { Up,Down,Right,Left } ;


/*
 *  Return the 2 indices not present of the set [0,1,2,3].
 *  other_index_2 for i2, other_index_3 for i3 (given i0, i1).
 */
static int other_index_2[4][4] =
  { {-1, 2,-1,-1 },
    { 3,-1,-1,-1 },
    {-1,-1,-1, 0 },
    {-1,-1, 1,-1 } };

static int other_index_3[4][4] =
  { {-1, 3,-1,-1 },
    { 2,-1,-1,-1 },
    {-1,-1,-1, 1 },
    {-1, -1, 0,-1 } };


#define Bytes(nquad) (int)(nquad/8+1)
#define Byte(quad) (int)(quad/8)

#define Mask(quad) (unsigned char)(1<<(quad%8))
#define MarkQuad(quad) usedArray[Byte(quad)] |= Mask(quad)

#define UsedQuad(quad) (usedArray[Byte(quad)] & Mask(quad))
#define ValidQuad(ich, quad) (ich ? DXIsElementValid (ich, quad) : 1)
#define GoodQuad(n) (n != -1 && !UsedQuad(n) && ValidQuad(xf->invCntns, n))


/*
 *  Move to the next quad in the same direction we've been moving.
 *  Someday we could use swaps to "take corners" instead of insisting on
 *  linear strips.  LookUpDir and flip_flop translate direction in the
 *  case of quads in different orientations to each other.
 */
#define NextQuad(quad)                                                        \
  {                                                                           \
  if (nPtsInStrip >= MaxQstripSize)     /* if strip is too long...    */      \
    quad = -1;                             /*    ...chop it off          */   \
  else                                                                        \
    {                                                                         \
    prev_quad = quad;                                                         \
    prev_i0 = i0;                                                             \
    prev_i1 = i1;                                                             \
    prev_i2 = i2;                                                             \
    prev_i3 = i3;                                                             \
    quad = neighbors[prev_quad].p[dir];    /* jump to adjacent quad        */ \
    if (!GoodQuad(quad))                   /* if it's used or invalid...   */ \
      quad = -1;                           /*   ...end the strip           */ \
    else                                                                      \
      {                                    /* adjust indices to new quad   */ \
      i0 = LookUp(quad,quads[prev_quad].p[prev_i2]);                          \
      i1 = LookUp(quad,quads[prev_quad].p[prev_i3]);                          \
      i2 = other_index_2[i0][i1];                                             \
      i3 = other_index_3[i0][i1];                                             \
      if(i3 == -1 || i2 == -1) 						      \
	quad = -1;							      \
      else {								      \
        dir = LookUpDir(quad,prev_quad);   /* find dir back to prev_quad   */ \
        dir = flip_flop[dir];              /* cont in the dir that we came */ \
	}								      \
      }                                                                       \
    }                                                                         \
  }


/*
 *  Choose the indicies for the 1st quad in a strip.  Look ahead one quad
 *  to avoid single quad strips.
 */
#define InitFirstQuad(i0,i1,i2,i3)                      \
  {                                                     \
  if (GoodQuad(neighbors[quad].p[Up]))                  \
    {dir = Up;    i0 = 0; i1 = 1; i2 = 2; i3 = 3;}      \
  else                                                  \
  if (GoodQuad(neighbors[quad].p[Right]))               \
    {dir = Right; i0 = 2; i1 = 0; i2 = 3; i3 = 1;}      \
  else                                                  \
  if (GoodQuad(neighbors[quad].p[Down]))                \
    {dir = Down;  i0 = 3; i1 = 2; i2 = 1; i3 = 0;}      \
  else                                                  \
    {dir = Left; i0 = 1; i1 = 3; i2 = 0; i3 = 2;}       \
  }



/************************************************************

              prev_i2      dir = Right             quads = 0 1 6 5
         6-----------5-----------4-----------7             1 2 5 4
         |prev_i0    |i0       i2|           |             2 3 4 7
         |           |           |           |     strip = 6 0 5 1 4 2...
         | prev_quad |   quad    |           |
         |           |           |           |
         |prev_i1    |i1       i3|           |
         0-----------1-----------2-----------3
              prev_i3            i2

*************************************************************/


static Quadruple
*getNeighbors (xfieldT *xf)
{
  /*
   *  Gets an array giving all the neighbors of a given quad.
   *  neighbor[quad].p[n] is the quad adjacent to quad quad, across from
   *  quad quad's nth point.  A -1 indicates no quad is adjacent across
   *  from the nth point.
   *
   *  If connections are expressed in a compact representation, then no
   *  neighbors array is necessary.
   */

  ENTRY(("getNeighbors(0x%x)", xf));

  if (DXGetArrayClass(xf->connections_array) == CLASS_MESHARRAY)
    {
      EXIT(("CLASS_MESHARRAY"));
      return 0 ;
    }
  else
    {
      if (!xf->neighbors_array)
	{
	  EXIT(("couldn't get neighbors from xfield"));
	  DXErrorReturn (ERROR_INTERNAL, "#13870") ;
	}

      if (!DXTypeCheck (xf->neighbors_array, TYPE_INT, CATEGORY_REAL, 1, 4))
	{
	  EXIT(("neighbors array has bad type"));
	  DXErrorReturn (ERROR_INTERNAL, "#13870") ;
	}

      EXIT((""));
      return (Quadruple *) DXGetArrayData(xf->neighbors_array) ;
    }
}


#define AllocateAndCopyQstripPointArray()			             \
{                                                                            \
if ( (stripArray[nStrips].point = (int *)tdmAllocate(nPtsInStrip*sizeof(int))) ) \
  {                                                                          \
    bcopy (point, stripArray[nStrips].point, nPtsInStrip * sizeof(int)) ;    \
    stripArray[nStrips].points = nPtsInStrip ;                               \
    nStrips++ ;                                                              \
    nStrippedPts += nPtsInStrip ;				             \
  }								             \
else								             \
  {								             \
    PRINT(("out of memory")) ;					             \
    DXErrorGoto (ERROR_NO_MEMORY, "#13000") ;			             \
  }                                                                          \
}


#define FreeTempStrips()						     \
{									     \
  for (i = 0 ; i < nStrips ; i++)					     \
      if (stripArray[i].point)						     \
	  tdmFree((Pointer) stripArray[i].point) ;			     \
									     \
  tdmFree((Pointer) stripArray) ;					     \
}


Error
_dxf_quadsToQmesh (xfieldT *xf, void *globals)
{
  DEFGLOBALDATA(globals) ;
  Quadruple *neighbors ;       /* neighbors array gives a quad's neighbors  */
  Qstrip *stripArray = 0 ;     /* temp array of strips                      */
  int point[MaxQstripSize] ;   /* array into which to build point list      */
  char *usedArray = 0 ;        /* marks quads already used                  */
  int nStrips = 0 ;            /* number of strips generated                */
  int nStrippedPts = 0 ;       /* number of points stripped                 */
  int nPtsInStrip ;            /* number of points in current strip         */
  int start_quad ;             /* first quad of the strip                   */
  int quad ;                   /* the current quad                          */
  int dir, i0, i1, i2, i3 ;    /* the 4 indexes into quads[quad].p[?]       */
  int prev_quad, prev_i0,      /* used to determine the same current info   */
      prev_i1,prev_i2,prev_i3 ;
  Quadruple *quads ;           /* array of original quad connections        */
  int nquads ;                 /* number of original quad connections       */
  int i, j, k ;                /* misc. indices                             */
  QMesh qmesh = NULL;
  dxObject qmesho = NULL;

  ENTRY(("_dxf_quadsToQmesh(0x%x)", xf));

  qmesho = _dxf_QueryObject(MESHHASH, (dxObject)xf->field);
  if (qmesho)
  {
      xf->meshObject = DXReference(qmesho);
      qmesh = (QMesh)DXGetPrivateData((Private)qmesho);
      _installQMeshInfo(xf, qmesh);

      return OK;
  }

  if (xf->connectionType != ct_quads)
    {
      EXIT(("xf->connectionType not ct_quads"));
      return OK ;
    }

  PRINT(("invalid connections: %d",
	   xf->invCntns? DXGetInvalidCount(xf->invCntns): 0)) ;

  neighbors = getNeighbors(xf);

  /* get quad connection info; quad connections are replaced by strips */
  quads = DXGetArrayData(xf->connections_array) ;
  nquads = xf->nconnections ;
  /*npoints = xf->npositions ;*/

  xf->origNConnections = nquads;
  xf->origConnections_array = xf->connections_array;
  xf->connections_array = NULL;

  /* allocate temporary array to hold maximum possible number of strips */
  if (!(stripArray = (Qstrip *) tdmAllocate(sizeof(Qstrip) * nquads)))
    {
      PRINT(("out of memory"));
      DXErrorGoto (ERROR_NO_MEMORY, "#13000") ;
    }

  if (neighbors)
    {
      /* quad connections expressed explicitly, we need usedArray */
      if (!(usedArray = tdmAllocateZero(sizeof(char) * Bytes(nquads))))
	{
	  PRINT(("out of memory"));
	  DXErrorGoto (ERROR_NO_MEMORY, "#13000") ;
	}

      /* strip the field, placing strip info into temporary arrays */
      for (start_quad = 0; start_quad < nquads; start_quad++)
	{
	  if (GoodQuad(start_quad))
	    {
	      nPtsInStrip = 0 ;
	      quad = start_quad ;
	      InitFirstQuad(i0,i1,i2,i3) ; /* pick 1st quad initial vertices */
	      AddPoint(quad,i1) ;          /* add the 1st two...             */
	      AddPoint(quad,i0) ;          /* ...points to the mesh          */
	      while (quad >= 0)            /* while there's a valid quad...  */
		{
		  AddPoint(quad,i3) ;      /*  add another quad to the strip */
		  AddPoint(quad,i2) ;
		  MarkQuad(quad) ;         /*  mark it as used               */
		  NextQuad(quad) ;         /*  move on to next quad          */
		}

	      /* save strip points */
	      AllocateAndCopyQstripPointArray() ;
	    }
	}
      tdmFree((Pointer)usedArray) ;
      usedArray = NULL;
    }
  else
    {
      /* quad connections expressed in compact mesh array form */
      int n, counts[100] ;

      if (!DXQueryGridConnections (xf->origConnections_array, &n, counts) || n != 2)
	  DXErrorGoto (ERROR_INTERNAL, "#13140") ;

      PRINT(("counts[0] = %d", counts[0]));
      PRINT(("counts[1] = %d", counts[1]));

      for (n = 0, i = 0 ; i < counts[0]-1 ; i++)
	{
	  j = 0 ;
	  while (j < counts[1]-1)
	    {
	      for ( ; j < counts[1]-1 && !ValidQuad(xf->invCntns, n) ; j++, n++)
		  /* skip invalid quads up to end of row/column */ ;

	      if (j == counts[1]-1)
		  break ;

	      /* start triangle strip */
	      nPtsInStrip = 0 ;
	      point[nPtsInStrip++] = n + i ;
	      point[nPtsInStrip++] = n + i + counts[1] ;

	      while (j < counts[1]-1 &&
		     nPtsInStrip < MaxQstripSize &&
		     ValidQuad(xf->invCntns, n))
		{
		  /* add valid quads up to end of row/column or strip limit  */
		  point[nPtsInStrip++] = n + i + 1 ;
		  point[nPtsInStrip++] = n + i + 1 + counts[1] ;
		  j++ ; n++ ;
		}

	      /* save strip points */
	      AllocateAndCopyQstripPointArray() ;
	    }
	}
    }

  PRINT(("stripped %d quadrangles", (nStrippedPts - 2*nStrips)/2));
  PRINT(("generated %d triangle strips", nStrips));
  PRINT(("average number of triangles per strip: %f",
	   nStrips? (nStrippedPts - 2*nStrips)/(float)nStrips: 0));

  if (! _newQMesh(nStrips, nStrippedPts, &qmesh, &qmesho))
      goto error;

  for (i=j=k=0 ; i<nStrips ; i++)
    {
      qmesh->meshes[j++] = k;
      qmesh->meshes[j++] = stripArray[i].points;
      memcpy(qmesh->connections+k, stripArray[i].point, stripArray[i].points*sizeof(int));
      k += stripArray[i].points;
    }

  _dxf_InsertObject(MESHHASH, (dxObject)(xf->field), (dxObject)qmesho);
  xf->meshObject = DXReference(qmesho);
  _installQMeshInfo(xf, qmesh);

  /* free temporary array of strips */
  if (usedArray)
      tdmFree((Pointer)usedArray);
  if (stripArray)
      FreeTempStrips() ;

  EXIT(("OK"));
  return OK ;

 error:
  if (usedArray)
      tdmFree((Pointer)usedArray);
  if (stripArray)
      FreeTempStrips() ;

  EXIT(("ERROR"));
  return 0 ;
}
