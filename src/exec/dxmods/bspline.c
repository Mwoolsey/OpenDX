/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dx/dx.h>
#include <math.h>

static int Knot(int i, int knotK, int knotN)
{
  if (i < knotK) 
    return 0;
  else if (i > knotN)
    return knotN - knotK + 2;
  else
    return i - knotK + 1;
}

static float NBlend(int i, int k, float u, int KnotK, int KnotN) 
{
  int t;
  float v;

  if (k==1) {
    v=0;
    if ((Knot(i, KnotK, KnotN) <= u)&&(u<(Knot(i+1, KnotK, KnotN)))) {
       v=1;
    }
  }
  else {
    v=0;
    t=Knot(i+k-1, KnotK, KnotN)-Knot(i, KnotK, KnotN);
    if (t != 0) 
      v=(u-Knot(i, KnotK, KnotN))*NBlend(i,k-1,u, KnotK, KnotN)/t;
    t=Knot(i+k, KnotK, KnotN)-Knot(i+1, KnotK, KnotN);
    if (t!=0)
      v=v+(Knot(i+k, KnotK, KnotN)-u)*NBlend(i+1,k-1,u, KnotK, KnotN)/t;
   }
   if ((i==5)&&(k==3)&&(u==4)) {
   }
   return v;
}

static Error BSpline(float *result, float u, int n, int k,
                     float *p, int dim)
{
  int i, knotK, knotN;
  float b;
  float x,y,z;

  knotK = k;
  knotN = n;

  x=0;
  y=0;
  z=0;

  if (dim==3) { 
    for (i=0; i<=n; i++) {
      b = NBlend(i,k,u,knotK, knotN);
      x = x+p[3*i]*b;
      y = y+p[3*i+1]*b;
      z = z+p[3*i+2]*b;
    }
    result[0] = x;
    result[1] = y;
    result[2] = z;
  }
  else if (dim==2) {
    for (i=0; i<=n; i++) {
      b = NBlend(i,k,u,knotK, knotN);
      x = x+p[2*i]*b;
      y = y+p[2*i+1]*b;
    }
    result[0] = x;
    result[1] = y;
  }
  else {
    for (i=0; i<=n; i++) {
      b = NBlend(i,k,u,knotK, knotN);
      x = x+p[i]*b;
    }
    result[0] = x;
  }
  return OK;
} 

Error
m_BSpline(Object *in, Object *out)
{
  Type type;
  Category category;
  int rank, shape[32], numitems, order, i, newnumitems, dim;
  float *p=NULL, *point=NULL;
  Array resultarray=NULL;
  float u;

  /* in[0] should be the list of 3D points */
  if (!in[0]) {
    DXSetError(ERROR_BAD_PARAMETER, "#10000","input");
    goto error;
  }
  if (DXGetObjectClass(in[0])!= CLASS_ARRAY) {
    DXSetError(ERROR_DATA_INVALID,"input must be a vector list");
    goto error;
  }
  if (!DXGetArrayInfo((Array)in[0], &numitems, &type, &category, &rank, shape))
    goto error;

  if (!in[1]) {
    newnumitems = 10;
  }
  else {
    if (!DXExtractInteger(in[1], &newnumitems)) {
      DXSetError(ERROR_DATA_INVALID,"numitems must be an integer");
      goto error;
    }
  }

  if (!in[2]) {
    if (numitems > 4)
       order = 4;
    else
       order = numitems;
  }
  else {
    if (!DXExtractInteger(in[2], &order)) {
       DXSetError(ERROR_DATA_INVALID,"order must be a positive integer");
       goto error;
    }
    if (order < 1) {
       DXSetError(ERROR_DATA_INVALID,"order must be a positive integer");
       goto error;
    }
    if (order > numitems) {
       DXWarning("order must be <= number of items, order set to %d",
                 numitems);
       order = numitems;
    }
  }
  if (rank==0) {
    dim=1;
  }
  else if (rank==1) {
    if ((shape[0]!=1)&&(shape[0]!=2)&&(shape[0]!=3)) {
      DXSetError(ERROR_DATA_INVALID,
                 "input must be a list of scalars or 2- or 3-vectors");
      goto error;
    }
    dim = shape[0];
  }
  else {
    DXSetError(ERROR_DATA_INVALID,"input must be a list of 2- or 3-vectors");
    goto error;
  }

  point = DXAllocate(dim*sizeof(float));
  if (!point) goto error;
  p = DXAllocate(numitems*dim*sizeof(float));
  if (!p) goto error;
  if (!DXExtractParameter(in[0], TYPE_FLOAT, dim, numitems, (Pointer)p))
     goto error;

  resultarray = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, dim);
  if (!resultarray) goto error;

  if (!DXAddArrayData(resultarray, 0, newnumitems, NULL))
     goto error;


  for (i=0; i<newnumitems; i++) {
     u = 0.99999*(numitems+1-order)*((float)i)/(float)(newnumitems-1);
     BSpline(point, u, numitems-1, order, p, dim);
     if (!DXAddArrayData(resultarray, i, 1, point)) goto error;
  }
  out[0]=(Object)resultarray;
  DXFree((Pointer)point);
  DXFree((Pointer)p);
  return OK;
error:
  DXDelete((Object)resultarray);
  DXFree((Pointer)point);
  DXFree((Pointer)p);
  return ERROR;

  
}
