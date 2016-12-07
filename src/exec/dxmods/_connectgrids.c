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
#include <dx/dx.h>
#include "_connectgrids.h"

struct arg_radius {
  Object ino;
  Field base;
  float radius;
  float exponent;
  Array missing;
};

struct arg_nearest {
  Object ino;
  Field base;
  int number;
  float *radius;
  float radiusvalue;
  float exponent;
  Array missing;
};

struct arg_scatter {
  Object ino;
  Field base;
  Array missing;
};


typedef struct matrix2D {
  float A[2][2];
  float b[2];
} Matrix2D;

typedef struct vector2D {
  float x, y;
} Vector2D;

#define PI 3.14156
#define E .0001                                         /* fuzz */
#define FLOOR(x) ((x)+E>0? (int)((x)+E) : (int)((x)+E)-1)  /* fuzzy floor */
#define CEIL(x) ((x)-E>0? (int)((x)-E)+1 : (int)((x)-E))  /* fuzzy ceiling */
#define ABS(x) ((x)<0? -(x) : (x)) 
#define RND(x) (x<0? x-0.5 : x+0.5)
#define POW(x,y) (pow((double)x, (double)y))

#define INSERT_NEAREST_BUFFER(TYPE)                               \
{                                                                 \
              TYPE *holder=(TYPE *)bufdata;                       \
              float distancesq=0;                                   \
              switch(shape_base[0]) {                             \
                case (1):                                         \
                   distancesq=                                    \
                      (gridposition1D-inputposition_ptr[l])*      \
                      (gridposition1D-inputposition_ptr[l]);      \
                   break;                                         \
                case (2):                                         \
                   distancesq = (gridposition2D.x-                \
                                    inputposition_ptr[2*l])*      \
                              (gridposition2D.x-                  \
                                    inputposition_ptr[2*l]) +     \
                                (gridposition2D.y-                \
                                    inputposition_ptr[2*l+1])*    \
                                (gridposition2D.y-                \
                                    inputposition_ptr[2*l+1]);    \
                   break;                                         \
                case (3):                                         \
	           distancesq = (gridposition.x-                  \
                                   inputposition_ptr[3*l])*       \
                              (gridposition.x-                    \
                                   inputposition_ptr[3*l]) +      \
	                      (gridposition.y-                    \
                                   inputposition_ptr[3*l+1])*     \
	                      (gridposition.y-                    \
                                   inputposition_ptr[3*l+1]) +    \
	                      (gridposition.z-                    \
                                   inputposition_ptr[3*l+2])*     \
	                      (gridposition.z-                    \
                                   inputposition_ptr[3*l+2]);    \
                   break;                                         \
              }                                                   \
        /* first try to insert into the already present buffer */ \
	      for (m=0; m<numvalid; m++) {                        \
		if (distancesq < bufdistance[m]) {                \
		  if (numvalid < numnearest) numvalid++;          \
		  for (n=numvalid-1; n>m; n--) {                  \
		    bufdistance[n] = bufdistance[n-1];            \
                    memcpy(&holder[shape[0]*n], &holder[shape[0]*(n-1)], \
                           DXGetItemSize(newcomponent));          \
		  }                                               \
		  bufdistance[m]=distancesq;                      \
                  memcpy(&holder[shape[0]*m], tmpdata,            \
                         DXGetItemSize(newcomponent));            \
                  goto filled_buf##TYPE;                          \
	        }                                                 \
              }                                                   \
              if (numvalid == 0) {                                \
                bufdistance[0] = distancesq;                      \
                memcpy(&holder[0], tmpdata, DXGetItemSize(newcomponent)); \
                numvalid++;                                       \
              }                                                   \
	    filled_buf##TYPE:                                     \
              tmpdata += datastep;                                \
	      continue;                                           \
}                                                                 \


#define ACCUMULATE_DATA(TYPE)                                        \
{                                                                    \
            TYPE *holder;                                            \
            float dis;                                               \
            float radiussq = radiusvalue*radiusvalue;                \
            int dc;                                                  \
            for (ii=0, holder=(TYPE *)bufdata; ii<numvalid; ii++) {  \
              dis = sqrt(bufdistance[ii]);                           \
              if (bufdistance[ii] != 0.0) {                          \
	        if (radius) {                                          \
	  	  if (bufdistance[ii] <= radiussq) {                \
		    distanceweight += 1.0/POW(dis, exponent);           \
                    for (dc=0; dc<shape[0]; dc++) {                    \
		      data[dc] +=                                      \
                   (float)(holder[dc])/POW(dis, exponent); \
                    }                                                  \
		  }                                                    \
	        }                                                      \
	        else {                                                 \
	  	  distanceweight += 1.0/(POW(dis,exponent));           \
                  for (dc=0; dc<shape[0]; dc++) {                      \
		     data[dc] += (float)holder[dc]/POW(dis, exponent);   \
                  }                                                   \
	        }                                                      \
              }                                                      \
              else {                                                 \
                distanceweight = 1.0;                                \
                for (dc=0; dc<shape[0]; dc++) {                      \
                  data[dc] = (float)holder[dc];                      \
                }                                                    \
                break;                                               \
              }                                                      \
              holder+=shape[0];                                      \
            }                                                        \
}                                                                    \

#define CHECK_AND_ALLOCATE_MISSING(missing,size,type,type_enum,shape,ptr) \
{                                                                      \
       ptr = (type *)DXAllocateLocal(size);                            \
       if (!ptr)                                                      \
          goto error;                                                  \
       if(missing){                                                    \
        if (!DXExtractParameter((Object)missing, type_enum,            \
                               shape, 1, (Pointer)ptr)) {              \
          DXSetError(ERROR_DATA_INVALID,                               \
                    "missing must match all position-dependent components; does not match %s", name);      \
          goto error;                                                 \
        }                                                              \
       }                                                               \
       else{                                                           \
        DXWarning("Grid missing values assigned to 0 and marked invalid");  \
        for(i=0;i<shape;i++) ptr[i]=0;                                 \
       }                                                               \
}                                                                      \

static Error ConnectNearest(Group, Field, int, float *, float, float, Array); 
static Error FillBuffersIrreg(int, int, ubyte *, float *,  float *,  
                              float *, InvalidComponentHandle, 
                              InvalidComponentHandle, int *, int, float);
static Error FillDataArray(Type, Array, Array, ubyte *, float *, 
                           int, int, int, float, Array, char *); 
static Error ConnectRadiusField(Pointer);
static Error ConnectNearestField(Pointer);
static Error ConnectScatterField(Pointer);
static Error ConnectScatter(Field, Field, Array);
static Error ConnectRadius(Field, Field, float, float, Array);
static Error ConnectRadiusRegular(Field, Field, float, float,  Array); 
static Error ConnectRadiusIrregular(Field, Field, float, float, Array); 

static Vector2D Vec2D(double x, double y)
{
  Vector2D v;
  v.x = x; v.y = y;
  return v;
}

static Vector2D Apply2D(Vector2D vec, Matrix2D mat) 
{
  Vector2D result;

  result.x = vec.x*mat.A[0][0] + vec.y*mat.A[1][0] + mat.b[0];
  result.y = vec.x*mat.A[0][1] + vec.y*mat.A[1][1] + mat.b[1];
  return result;
}

static Matrix2D Invert2D(Matrix2D matrix2d)
{
  float a,b,c,d,notsing;
  Matrix2D invert;
  Vector2D vec2d, invertb;

  a = matrix2d.A[0][0];
  b = matrix2d.A[0][1];
  c = matrix2d.A[1][0];
  d = matrix2d.A[1][1];

  /* check singularity */
  notsing = a*d-b*c;
  if (notsing) {
     invert.A[0][0] = (1.0/notsing)*(d);
     invert.A[0][1] = (1.0/notsing)*(-b);
     invert.A[1][0] = (1.0/notsing)*(-c);
     invert.A[1][1] = (1.0/notsing)*(a);
     invert.b[0] = 0;
     invert.b[1] = 0;
  }
     
  invertb.x = matrix2d.b[0];
  invertb.y = matrix2d.b[1];
  vec2d = Apply2D(invertb, invert);
  invert.b[0] = -vec2d.x; 
  invert.b[1] = -vec2d.y; 
  return invert;
}


Error _dxfConnectNearestObject(Object ino, Object base, 
                                      int numnearest, 
                                      float *radius, float exponent, 
                                      Array missing) 
{
  /* recurse on base to the field level */
  struct arg_nearest arg;
  Object subbase;
  int i;
  
  switch (DXGetObjectClass(base)) {
  case CLASS_FIELD:
    /* create the task group */
    arg.ino = ino;
    arg.base = (Field)base;
    arg.number = numnearest;
    arg.radius = radius;
    arg.exponent = exponent;
    if (radius)
      arg.radiusvalue = *radius;
    else
      arg.radiusvalue = 0.0;
    arg.missing = missing;
    if (!DXAddTask(ConnectNearestField,(Pointer)&arg, sizeof(arg),0.0))
      goto error;
    break;
  case CLASS_GROUP:
    for (i=0; (subbase=DXGetEnumeratedMember((Group)base, i, NULL)); i++) {
      if (!(_dxfConnectNearestObject((Object)ino, (Object)subbase, 
                                      numnearest, radius, exponent, missing))) 
	goto error;
    }
    break;
  default:
    DXSetError(ERROR_DATA_INVALID,"base must be a field or a group");
    goto error; 
  }
  return OK;
  
 error:
  return ERROR;  
  
}


Error _dxfConnectRadiusObject(Object ino, Object base, 
                                     float radius, float exponent, 
                                     Array missing)
     /* recurse on the base to the field level */
{
  struct arg_radius arg;
  Object subbase;
  int i;
  
  
  switch (DXGetObjectClass(base)) {
  case CLASS_FIELD:
    /* create the task group */
    arg.ino = ino;
    arg.base = (Field)base;
    arg.radius = radius;
    arg.exponent = exponent;
    arg.missing = missing;
    if (!DXAddTask(ConnectRadiusField,(Pointer)&arg, sizeof(arg),0.0))
      goto error;
    break;
  case CLASS_GROUP:
    for (i=0; (subbase=DXGetEnumeratedMember((Group)base, i, NULL)); i++) {
      if (!(_dxfConnectRadiusObject((Object)ino, (Object)subbase, 
				radius, exponent, missing))) 
	goto error;
    }
    break;
  default:
    DXSetError(ERROR_DATA_INVALID,"base must be a field or a group");
    goto error; 
  }
  return OK;
  
 error:
  return ERROR;  
  
}

static Error ConnectNearestField(Pointer p)
{
  struct arg_nearest *arg = (struct arg_nearest *)p;
  Object ino, subino, newino=NULL; 
  int i, numnearest;
  Field base;
  float *radius;
  float radiusvalue;
  float exponent;
  Array missing;
  
  ino = arg->ino;
  newino = DXApplyTransform(ino,NULL);
  
  switch (DXGetObjectClass(newino)) {
    
  case CLASS_FIELD:

    base = arg->base;
    radius = arg->radius;
    radiusvalue = arg->radiusvalue;
    missing = arg->missing;
    numnearest = arg->number;
    exponent = arg->exponent;
    if (!ConnectNearest((Group)newino, base, numnearest, radius, 
                         radiusvalue, exponent, missing))
      goto error;
    break;
    
  case CLASS_GROUP:
   
    switch (DXGetGroupClass((Group)newino)) {
    case CLASS_COMPOSITEFIELD:
      /* for now, can't handle a composite field */
      DXSetError(ERROR_NOT_IMPLEMENTED,"input cannot be a composite field");
        goto error; 
      base = arg->base;
      radius = arg->radius;
      radiusvalue = arg->radiusvalue;
      missing = arg->missing;
      numnearest = arg->number;
      exponent = arg->exponent;
      if (!ConnectNearest((Group)newino, base, numnearest, radius, 
                           radiusvalue, exponent, missing))
	goto error;
      break;
    default:
     DXSetError(ERROR_NOT_IMPLEMENTED,"input must be a single field");
       goto error;
     /* recurse on the members of the group */
      for (i=0; (subino=DXGetEnumeratedMember((Group)newino, i, NULL)); i++) {
        base = arg->base;
        radius = arg->radius;
        radiusvalue = arg->radiusvalue;
        missing = arg->missing;
        numnearest = arg->number;
        exponent = arg->exponent;
        if (!ConnectNearest((Group)subino, base, numnearest, radius, 
                           radiusvalue, exponent, missing))
	  goto error;
      }
      break;
   }
   break; 
  default:
      DXSetError(ERROR_DATA_INVALID,"input must be a field or a group");
      goto error;
  }
  DXDelete((Object)newino);
  return OK;
  
 error:
  DXDelete((Object)newino);
  return ERROR;
  
}


static Error ConnectRadiusField(Pointer p)
{
  struct arg_radius *arg = (struct arg_radius *)p;
  Object ino, newino=NULL;
  Field base;
  float radius;
  float exponent;
  Array missing;

  /* this one recurses on object ino */

  ino = arg->ino;
  newino = DXApplyTransform(ino,NULL);

  switch (DXGetObjectClass(newino)) {

    case CLASS_FIELD:
       base = arg->base;
       radius = arg->radius;
       missing = arg->missing;
       exponent = arg->exponent;
       if (!ConnectRadius((Field)newino, base, radius, exponent, missing))
         goto error;
      break;

    case CLASS_GROUP:
      DXSetError(ERROR_NOT_IMPLEMENTED,"input cannot be a group");
      goto error;
      break;

    default:
      DXSetError(ERROR_DATA_INVALID,"input must be a field");
      goto error;
  }
  DXDelete((Object)newino);
  return OK;

error:
  DXDelete((Object)newino);
  return ERROR;

}

static Error ConnectNearest(Group ino, Field base, int numnearest, 
                            float *radius, float radiusvalue, 
                            float exponent, Array missing)
{
  int componentsdone, anyinvalid=0;
  Matrix matrix;
  Matrix2D matrix2D;
  Array positions_base, positions_ino, oldcomponent=NULL, newcomponent;
  InvalidComponentHandle out_invalidhandle=NULL, in_invalidhandle=NULL;
  int counts[8], rank_base, shape_base[8], shape_ino[8], rank_ino; 
  int rank, shape[8], i, j, k, l, m, n, numvalid, pos_count=0;
  int numitemsbase, compcount, numitemsino, numitems;
  float distanceweight, distance=0, *data=NULL, *dp, *scratch=NULL;
  ArrayHandle handle=NULL;
  Type type;
  Point gridposition = {0, 0, 0};
  Vector2D gridposition2D = {0, 0};
  float gridposition1D=0, origin=0, delta=0;
  float *bufdistance=NULL;
  char *bufdata=NULL;
  char *name;
  Category category;
  float *inputposition_ptr;
  int data_count, invalid_count=0, ii, datastep, dc, regular;
  char *old_data_ptr=NULL, *tmpdata=NULL;
  float    *new_data_ptr_f=NULL, *missingval_f=NULL;
  int      *new_data_ptr_i=NULL, *missingval_i=NULL;
  short    *new_data_ptr_s=NULL, *missingval_s=NULL;
  double   *new_data_ptr_d=NULL, *missingval_d=NULL;
  byte     *new_data_ptr_b=NULL, *missingval_b=NULL;
  ubyte    *new_data_ptr_ub=NULL, *missingval_ub=NULL;
  ushort   *new_data_ptr_us=NULL, *missingval_us=NULL;
  uint     *new_data_ptr_ui=NULL, *missingval_ui=NULL;

  if (DXEmptyField((Field)ino))
     return OK;

  positions_base = (Array)DXGetComponentValue((Field)base, "positions");
  if (!positions_base) {
    DXSetError(ERROR_MISSING_DATA,"base field has no positions");
    goto error;
  }
  positions_ino = (Array)DXGetComponentValue((Field)ino, "positions");
  if (!positions_ino) {
    DXSetError(ERROR_MISSING_DATA,"input field has no positions");
    goto error;
  }
  
  DXGetArrayInfo(positions_base, &numitemsbase, &type, &category, 
                 &rank_base, 
		 shape_base);

  regular = 0;
  if ((DXQueryGridPositions((Array)positions_base,NULL,counts,
			    (float *)&matrix.b,(float *)&matrix.A) )) {
    regular = 1;
    if (shape_base[0]==2) {
      matrix2D.A[0][0] = matrix.A[0][0];                           
      matrix2D.A[1][0] = matrix.A[0][1];                          
      matrix2D.A[0][1] = matrix.A[0][2];                         
      matrix2D.A[1][1] = matrix.A[1][0];                        
      matrix2D.b[0] = matrix.b[0];                             
      matrix2D.b[1] = matrix.b[1];                           
    }
    else if (shape_base[0]==1) {
      origin = matrix.b[0];
      delta = matrix.A[0][0];
    }
  }
  else {
    /* irregular positions */
    if (!(handle = DXCreateArrayHandle(positions_base)))
      goto error;
    scratch = DXAllocateLocal(DXGetItemSize(positions_base));
    if (!scratch) goto error;
  }
  
  if ((type != TYPE_FLOAT)||(category != CATEGORY_REAL)) {
    DXSetError(ERROR_DATA_INVALID,
	       "positions component must be type float, category real");
    goto error;
  }
  
  
  
  out_invalidhandle = DXCreateInvalidComponentHandle((Object)base, NULL,
                                                     "positions");
  if (!out_invalidhandle)
     goto error;
  DXSetAllValid(out_invalidhandle);
  in_invalidhandle = DXCreateInvalidComponentHandle((Object)ino, NULL,
                                                     "positions");
  if (!in_invalidhandle)
     return ERROR;
  
  
  DXGetArrayInfo(positions_ino, &numitemsino, &type, &category, &rank_ino, 
		 shape_ino);


  if ((type != TYPE_FLOAT)||(category != CATEGORY_REAL)) {
    DXSetError(ERROR_DATA_INVALID,
	       "positions component must be type float, category real");
    goto error;
  }
  if (rank_base == 0)
    shape_base[0] = 1;
  if (rank_base > 1) {
    DXSetError(ERROR_DATA_INVALID,
	       "rank of base positions cannot be greater than 1");
    goto error;
  }
  if (rank_ino == 0)
    shape_ino[0] = 1;
  if (rank_ino > 1) {
    DXSetError(ERROR_DATA_INVALID,
	       "rank of input positions cannot be greater than 1");
    goto error;
  }
  
  if ((shape_base[0]<0)||(shape_base[0]>3)) {
    DXSetError(ERROR_DATA_INVALID,
	       "only 1D, 2D, and 3D positions for base supported");
    goto error;
  }
  
  if (shape_base[0] != shape_ino[0]) {
    DXSetError(ERROR_DATA_INVALID,
	       "dimensionality of base (%dD) does not match dimensionality of input (%dD)", 
	       shape_base[0], shape_ino[0]);
    goto error;
  }
  
  
  if ((shape_ino[0]<0)||(shape_ino[0]>3)) {
    DXSetError(ERROR_DATA_INVALID,
	       "only 1D, 2D, and 3D positions for input supported");
    goto error;
  }
  /* first set the extra counts to one to simplify the code for handling
     1, 2, and 3 dimensions */
  if (regular) {
    if (shape_base[0]==1) {
      counts[1]=1;
      counts[2]=1;
    }
    else if (shape_base[0]==2) {
      counts[2]=1;
    }
  }
  else {
    counts[0]=numitemsbase;
    counts[1]=1;
    counts[2]=1;
  }
  
  /* need to get the components we are going to copy into the new field */
  /* copied wholesale from gda */
  compcount = 0;
  componentsdone = 0;
  while (NULL != 
	 (oldcomponent=(Array)DXGetEnumeratedComponentValue((Field)ino,
							    compcount,
							    &name))) {
    Object attr;

    data_count=0;
    invalid_count=0;

    
    if (DXGetObjectClass((Object)oldcomponent) != CLASS_ARRAY)
      goto component_done;
    
    if (!strcmp(name,"positions") || 
        !strcmp(name,"connections") ||
        !strcmp(name,"invalid positions")) 
      goto component_done;
    
    /* if the component refs anything, leave it */
    if (DXGetComponentAttribute((Field)ino,name,"ref"))
      goto component_done;

    attr = DXGetComponentAttribute((Field)ino,name,"der");
    if (attr)
       goto component_done;
    
    attr = DXGetComponentAttribute((Field)ino,name,"dep");
    if (!attr) {
      /* does it der? if so, throw it away */
      attr = DXGetComponentAttribute((Field)ino,name,"der");
      if (attr)
        goto component_done;
      /* it doesn't ref, doesn't dep and doesn't der. 
         Copy it to the output and quit */
      if (!DXSetComponentValue((Field)base,name,(Object)oldcomponent))
         goto error; 
      goto component_done; 
    }
    
    if (DXGetObjectClass(attr) != CLASS_STRING) {
      DXSetError(ERROR_DATA_INVALID, "dependency attribute not type string");
      goto error;
    }
    
    if (strcmp(DXGetString((String)attr),"positions")){
      DXWarning("Component '%s' is not dependent on positions!",name); 
      DXWarning("Regrid will remove '%s' and output the base grid 'data' component if it exists",name);
      goto component_done; 
    }


    /* if the component happens to be "invalid positions" ignore it */
    if (!strcmp(name,"invalid positions"))
      goto component_done;


    componentsdone++;
    
    DXGetArrayInfo((Array)oldcomponent,&numitems, &type, 
		   &category, &rank, shape);
    if (rank==0) shape[0]=1;
    
    
    
    /* check that the missing component, if present, matches the component
       we're working on */
    if (missing)
      switch (type) {
      case (TYPE_FLOAT):
        CHECK_AND_ALLOCATE_MISSING(missing, DXGetItemSize(oldcomponent),
                                   float, type, shape[0], missingval_f);
        break;
      case (TYPE_INT):
        CHECK_AND_ALLOCATE_MISSING(missing, DXGetItemSize(oldcomponent), 
                                   int, type, shape[0], missingval_i);
        break;
      case (TYPE_DOUBLE):
        CHECK_AND_ALLOCATE_MISSING(missing, DXGetItemSize(oldcomponent), 
                                   double, type, shape[0], missingval_d);
        break;
      case (TYPE_SHORT):
        CHECK_AND_ALLOCATE_MISSING(missing, DXGetItemSize(oldcomponent), 
                                   short, type, shape[0], missingval_s);
        break;
      case (TYPE_BYTE):
        CHECK_AND_ALLOCATE_MISSING(missing, DXGetItemSize(oldcomponent), 
                                   byte, type, shape[0], missingval_b);
        break;
      case (TYPE_UINT):
        CHECK_AND_ALLOCATE_MISSING(missing, DXGetItemSize(oldcomponent), 
                                   uint, type, shape[0], missingval_ui);
        break;
      case (TYPE_USHORT):
        CHECK_AND_ALLOCATE_MISSING(missing, DXGetItemSize(oldcomponent), 
                                   ushort, type, shape[0], missingval_us);
        break;
      case (TYPE_UBYTE):
        CHECK_AND_ALLOCATE_MISSING(missing, DXGetItemSize(oldcomponent), 
                                   ubyte, type, shape[0], missingval_ub);
        break;
      default:
        DXSetError(ERROR_DATA_INVALID,"unsupported data type");
        goto error;
      }
    
    
    if (rank == 0)
      shape[0] = 1;
    if (rank > 1) {
      DXSetError(ERROR_NOT_IMPLEMENTED,"rank larger than 1 not implemented");
      goto error;
    }
    if (numitems != numitemsino) {
      DXSetError(ERROR_BAD_PARAMETER,
		 "number of items in %s component not equal to number of positions", 
		 name);
      goto error;
    }
    newcomponent = DXNewArrayV(type, category, rank, shape);
    if (!DXAddArrayData(newcomponent, 0, numitemsbase, NULL))
      goto error;
    old_data_ptr = (char *)DXGetArrayData(oldcomponent);
    
    switch (type) {
    case (TYPE_FLOAT):
      new_data_ptr_f = (float *)DXGetArrayData(newcomponent);
      break;
    case (TYPE_INT):
      new_data_ptr_i = (int *)DXGetArrayData(newcomponent);
      break;
    case (TYPE_SHORT):
      new_data_ptr_s = (short *)DXGetArrayData(newcomponent);
      break;
    case (TYPE_BYTE):
      new_data_ptr_b = (byte *)DXGetArrayData(newcomponent);
      break;
    case (TYPE_DOUBLE):
      new_data_ptr_d = (double *)DXGetArrayData(newcomponent);
      break;
    case (TYPE_UINT):
      new_data_ptr_ui = (uint *)DXGetArrayData(newcomponent);
      break;
    case (TYPE_UBYTE):
      new_data_ptr_ub = (ubyte *)DXGetArrayData(newcomponent);
      break;
    case (TYPE_USHORT):
      new_data_ptr_us = (ushort *)DXGetArrayData(newcomponent);
      break;
    default:
      DXSetError(ERROR_DATA_INVALID,"unrecognized data type");
      goto error;
    }
    
    
    
    /* need to allocate a buffer of length n to hold the nearest neighbors*/
    bufdata = 
       (char *)DXAllocateLocal(numnearest*DXGetItemSize(newcomponent)*shape[0]);
    if (!bufdata) goto error;
    bufdistance = (float *)DXAllocateLocal(numnearest*sizeof(float));
    if (!bufdistance) goto error; 
    data = (float *)DXAllocateLocalZero(sizeof(float)*shape[0]);
    if (!data) goto error;
    datastep = DXGetItemSize(newcomponent);
    
    /* loop over all the grid points */
    inputposition_ptr = (float *)DXGetArrayData(positions_ino);
    for (i=0; i<counts[0]; i++) {
      for (j=0; j<counts[1]; j++) {
	for (k=0; k<counts[2]; k++) {
          if (regular) { 
	    switch (shape_base[0]) {
	    case (3):
	      gridposition = DXApply(DXVec(i,j,k), matrix);
	      break;
	    case (2):
	      gridposition2D = Apply2D(Vec2D(i,j), matrix2D); 
              break;
	    case (1):
	      gridposition1D = origin + i*delta;
	    }
          }
          else {
            switch(shape_base[0]) {
            case (3):
              dp = DXGetArrayEntry(handle, pos_count, scratch);
	      gridposition = DXPt(dp[0], dp[1], dp[2]);
              break; 
            case (2):
              dp = DXGetArrayEntry(handle, pos_count, scratch);
	      gridposition2D.x = dp[0];
	      gridposition2D.y = dp[1];
              break;
            case (1):
              dp = DXGetArrayEntry(handle, pos_count, scratch);
	      gridposition1D = dp[0];
              break;
            }
            pos_count++;
          } 
	  for (ii=0; ii<numnearest; ii++) {
	    bufdistance[ii] = DXD_MAX_FLOAT;
	  }
	  /* numvalid is how many valid data points are in the buffer 
	     for this particular grid point */
	  numvalid = 0;
	  for (l=0, tmpdata=old_data_ptr; l<numitemsino; l++) {
            /* don't do anything if the input is invalid */
            if (DXIsElementValid(in_invalidhandle, l)) {
	    switch (type) {
	    case (TYPE_FLOAT):
	      INSERT_NEAREST_BUFFER(float); 
	      break;
	    case (TYPE_INT):
	      INSERT_NEAREST_BUFFER(int); 
	      break;
	    case (TYPE_SHORT):
	      INSERT_NEAREST_BUFFER(short); 
	      break; 
	    case (TYPE_DOUBLE):
	      INSERT_NEAREST_BUFFER(double); 
	      break;
	    case (TYPE_BYTE):
	      INSERT_NEAREST_BUFFER(byte); 
	      break;
	    case (TYPE_UBYTE):
	      INSERT_NEAREST_BUFFER(ubyte); 
	      break;
	    case (TYPE_USHORT):
	      INSERT_NEAREST_BUFFER(ushort); 
	      break;
	    case (TYPE_UINT):
	      INSERT_NEAREST_BUFFER(uint); 
              break;
            default:
              DXSetError(ERROR_DATA_INVALID,"unsupported data type");
              goto error;
	    }
          }
	  }
	  /* have now looped over all the scattered points */
	  distanceweight = 0;
	  memset(data, 0, shape[0]*sizeof(float));
	  
	  
	  switch (type) {
	  case (TYPE_FLOAT):
	    ACCUMULATE_DATA(float);
	    break;
	  case (TYPE_INT):
	    ACCUMULATE_DATA(int);
	    break;
	  case (TYPE_BYTE):
	    ACCUMULATE_DATA(byte);
	    break;
	  case (TYPE_DOUBLE):
	    ACCUMULATE_DATA(double);
	    break;
	  case (TYPE_SHORT):
	    ACCUMULATE_DATA(short);
	    break;
	  case (TYPE_UINT):
	    ACCUMULATE_DATA(uint);
	    break;
	  case (TYPE_USHORT):
	    ACCUMULATE_DATA(ushort);
	    break;
	  case (TYPE_UBYTE):
	    ACCUMULATE_DATA(ubyte);
	    break;
          default:
            DXSetError(ERROR_DATA_INVALID,"unsupported data type");
            goto error;
	  } 
	  
	  /* now stick the data value into the new data component */
	  switch (type) {
	  case (TYPE_FLOAT):
	    if (distanceweight != 0.) {
	      for (dc=0; dc<shape[0]; dc++) {
		new_data_ptr_f[data_count++] = 
		  (float)(data[dc]/distanceweight);
	      }
	    }
	    else {
	      if (missing) {
		for (dc=0; dc<shape[0]; dc++) {
		  new_data_ptr_f[data_count++] = missingval_f[dc]; 
		}
	      }
	      else {
		/* use the invalid positions array and set the data
		   value to zero */
                if (!DXSetElementInvalid(out_invalidhandle, invalid_count))
                   goto error;
		for (dc=0; dc<shape[0]; dc++) {
		  new_data_ptr_f[data_count++] = 0;
		}
	      }
	    }
	    break; 
	  case (TYPE_INT):
	    if (distanceweight != 0) {
	      for (dc=0; dc<shape[0]; dc++) {
		new_data_ptr_i[data_count++] = 
		  (int)(RND(data[dc]/distanceweight));
	      }
	    }
	    else {
	      if (missing) {
		for (dc=0; dc<shape[0]; dc++) {
		  new_data_ptr_i[data_count++] = missingval_i[dc]; 
		}
	      }
	      else {
		/* use the invalid positions array and set the data
		   value to zero */
                if (!DXSetElementInvalid(out_invalidhandle, invalid_count))
                   goto error;
		for (dc=0; dc<shape[0]; dc++) {
		  new_data_ptr_i[data_count++] = 0;
		}
	      }
	    }
	    break;
	  case (TYPE_SHORT):
	    if (distanceweight != 0) {
	      for (dc=0; dc<shape[0]; dc++) {
		new_data_ptr_s[data_count++] = 
		  (short)(RND(data[dc]/distanceweight));
	      }
	    }
	    else {
	      if (missing) {
		for (dc=0; dc<shape[0]; dc++) {
		  new_data_ptr_s[data_count++] = missingval_s[dc]; 
		}
	      }
	      else {
		/* use the invalid positions array and set the data
		   value to zero */
                if (!DXSetElementInvalid(out_invalidhandle, invalid_count))
                   goto error;
		for (dc=0; dc<shape[0]; dc++) {
		  new_data_ptr_s[data_count++] = 0;
		}
	      }
	    }
	    break;
	  case (TYPE_BYTE):
	    if (distanceweight != 0) {
	      for (dc=0; dc<shape[0]; dc++) {
		new_data_ptr_b[data_count++] =
		  (byte)(RND(data[dc]/distanceweight));
	      }
	    }
	    else {
	      if (missing) {
		for (dc=0; dc<shape[0]; dc++) {
		  new_data_ptr_b[data_count++] = missingval_b[dc]; 
		}
	      }
	      else {
		/* use the invalid positions array and set the data
		   value to zero */ 
                if (!DXSetElementInvalid(out_invalidhandle, invalid_count))
                   goto error;
		for (dc=0; dc<shape[0]; dc++) {
		  new_data_ptr_b[data_count++] = 0;
		}
	      }
	    }
	    break;
	  case (TYPE_DOUBLE):
	    if (distanceweight != 0) {
	      for (dc=0; dc<shape[0]; dc++) {
		new_data_ptr_d[data_count++] = 
		  (double)(data[dc]/distanceweight);
	      }
	    }
	    else {
	      if (missing) {
		for (dc=0; dc<shape[0]; dc++) {
		  new_data_ptr_d[data_count++] = missingval_d[dc]; 
		}
	      }
	      else {
		/* use the invalid positions array and set the data
		   value to zero */
                if (!DXSetElementInvalid(out_invalidhandle, invalid_count))
                   goto error;
		for (dc=0; dc<shape[0]; dc++) {
		  new_data_ptr_d[data_count++] = 0;
		}
	      }
	    }
	    break;
	  case (TYPE_UBYTE):
	    if (distanceweight != 0) {
	      for (dc=0; dc<shape[0]; dc++) {
		new_data_ptr_ub[data_count++] = 
		  (ubyte)(RND(data[dc]/distanceweight));
	      }
	    }
	    else {
	      if (missing) {
		for (dc=0; dc<shape[0]; dc++) {
		  new_data_ptr_ub[data_count++] = missingval_ub[dc]; 
		}
	      }
	      else {
		/* use the invalid positions array and set the data
		   value to zero */
                if (!DXSetElementInvalid(out_invalidhandle, invalid_count))
                   goto error;
		for (dc=0; dc<shape[0]; dc++) {
		  new_data_ptr_ub[data_count++] = 0;
		}
	      }
	    }
	    break;
	  case (TYPE_USHORT): 
	    if (distanceweight != 0) {
	      for (dc=0; dc<shape[0]; dc++) {
		new_data_ptr_us[data_count++] = 
		  (ushort)(RND(data[dc]/distanceweight));
	      }
	    } 
	    else {
	      if (missing) {
		for (dc=0; dc<shape[0]; dc++) {
		  new_data_ptr_us[data_count++] = missingval_us[dc]; 
		}
	      }
	      else {
		/* use the invalid positions array and set the data
		   value to zero */
                if (!DXSetElementInvalid(out_invalidhandle, invalid_count))
                   goto error;
		for (dc=0; dc<shape[0]; dc++) {
		  new_data_ptr_us[data_count++] = 0;
		}
	      }
	    }
	    break;
	  case (TYPE_UINT):
	    if (distanceweight != 0) {
	      for (dc=0; dc<shape[0]; dc++) {
		new_data_ptr_ui[data_count++] = 
		  (uint)(RND(data[dc]/distanceweight));
	      }
	    }
	    else {
	      if (missing) {
		for (dc=0; dc<shape[0]; dc++) {
		  new_data_ptr_ui[data_count++] = missingval_ui[dc]; 
		}
	      } 
	      else {
		/* use the invalid positions array and set the data
		   value to zero */
                if (!DXSetElementInvalid(out_invalidhandle, invalid_count))
                   goto error;
		for (dc=0; dc<shape[0]; dc++) {
		  new_data_ptr_ui[data_count++] = 0;
		}
	      }
	    }
	    break;
            default:
              DXSetError(ERROR_DATA_INVALID,"unsupported data type");
              goto error;
	  }
	  /* increment the invalid positions array pointer */
	  invalid_count++;
	}
      }
    }
    
    
    if (!DXChangedComponentValues((Field)base,name))
      goto error;
    if (!DXSetComponentValue((Field)base,name,(Object)newcomponent))
      goto error;
    /* XXX and radius also (otherwise inv pos not nec) */
    /* should inv pos be done each time? (for each component) */
    if ((!missing)&&(componentsdone==1)) {
      if (!DXSaveInvalidComponent(base, out_invalidhandle))
         goto error;
                                   
    }
    newcomponent = NULL;
    if (!DXSetComponentAttribute((Field)base, name,
				 "dep", 
				 (Object)DXNewString("positions"))) 
      goto error;
    
  component_done:
    compcount++;
    DXFree((Pointer)missingval_f);
    missingval_f=NULL;
    DXFree((Pointer)missingval_s);
    missingval_s=NULL;
    DXFree((Pointer)missingval_i);
    missingval_i=NULL;
    DXFree((Pointer)missingval_d);
    missingval_d=NULL;
    DXFree((Pointer)missingval_b);
    missingval_b=NULL;
    DXFree((Pointer)missingval_ui);
    missingval_ui=NULL;
    DXFree((Pointer)missingval_ub);
    missingval_ub=NULL;
    DXFree((Pointer)missingval_us);
    missingval_us=NULL;
    DXFree((Pointer)bufdistance);
    bufdistance=NULL;
    DXFree((Pointer)bufdata);
    bufdata=NULL;
    DXFree((Pointer)data);
    data=NULL;
  }
  if (componentsdone==0) {
    if (missing) {
      DXWarning("no position dependent components found; missing value given cannot be used");
    }
    else {
      /* need to allocate a buffer of length n to hold the nearest neighbors*/
      bufdistance = (float *)DXAllocateLocal(numnearest*sizeof(float));
      if (!bufdistance) goto error;
      
      /* loop over all the grid points */
      inputposition_ptr = (float *)DXGetArrayData(positions_ino);
      for (i=0; i<counts[0]; i++) {
	for (j=0; j<counts[1]; j++) {
	  for (k=0; k<counts[2]; k++) {
	    if (regular) {
	      switch (shape_base[0]) {
	      case (3):
		gridposition = DXApply(DXVec(i,j,k), matrix);
		break;
	      case (2):
		gridposition2D = Apply2D(Vec2D(i,j), matrix2D);
                break;
	      case (1):
                gridposition1D = origin + i*delta;
                break;
	      }
	    }
	    else {
	      switch(shape_base[0]) {
	      case (3):
                dp = DXGetArrayEntry(handle, pos_count, scratch);
		gridposition = DXPt(dp[0], dp[1], dp[2]);
		break;
	      case (2):
                dp = DXGetArrayEntry(handle, pos_count, scratch);
		gridposition2D.x = dp[0];
		gridposition2D.y = dp[1];
		break;
              case (1):
                dp = DXGetArrayEntry(handle, pos_count, scratch);
                gridposition1D = dp[0];
                break;
	      }
	      pos_count++;
	    }
	    for (ii=0; ii<numnearest; ii++) {
	      bufdistance[ii] = DXD_MAX_FLOAT;
	    }
	    /* numvalid is how many valid data points are in the buffer
	       for this particular grid point */
	    numvalid = 0;
	    for (l=0, tmpdata=old_data_ptr; l<numitemsino; l++) {
	      /* don't do anything if the input is invalid */
	      if (DXIsElementValid(in_invalidhandle, l)) {
		switch(shape_base[0]) {
		case (1):
		  distance= ABS(gridposition1D-inputposition_ptr[l]);
		  break;
		case (2):
		  distance = sqrt(POW(gridposition2D.x-
				      inputposition_ptr[2*l],2) +
				  POW(gridposition2D.y-
				      inputposition_ptr[2*l+1],2));
		  break;
		case (3):
		  distance = DXLength(DXSub(gridposition,
					    DXPt(inputposition_ptr[3*l],
						 inputposition_ptr[3*l+1],
						 inputposition_ptr[3*l+2])));
		  break;
		}
		/* first try to insert into the already present buffer */ 
		for (m=0; m<numvalid; m++) {
		  if (distance < bufdistance[m]) {
		    if (numvalid < numnearest) numvalid++;
		    for (n=numvalid-1; n>m; n--) {
		      bufdistance[n] = bufdistance[n-1];
		    }
		    bufdistance[m]=distance;
		    goto filled_buf;
		  }
		}
		if (numvalid == 0) {
		  bufdistance[0] = distance;
		  numvalid++;
		} 
	      filled_buf:
		tmpdata += datastep;
		continue;
	      }
	    }
	    
	    /* have now looped over all the scattered points */
	    
	    if (radius && (bufdistance[0]>radiusvalue)) {
	      anyinvalid=1;
	      if (!DXSetElementInvalid(out_invalidhandle, invalid_count))
		goto error;
	    }
	    /* increment the invalid positions array pointer */
	    invalid_count++;
	  }
	}
      }
      /* put the invalid component in the field */
      if (anyinvalid) {
	if (!DXSaveInvalidComponent(base, out_invalidhandle))
	  goto error;
	
      }
      DXFree((Pointer)bufdistance);  
    }
  }
  
  
  DXFreeInvalidComponentHandle(out_invalidhandle);    
  DXFreeInvalidComponentHandle(in_invalidhandle);    
  DXFree((Pointer)scratch);
  DXFreeArrayHandle(handle);
  if (!DXEndField(base)) goto error;
  return OK;


 error:
  DXFreeInvalidComponentHandle(out_invalidhandle);    
  DXFreeInvalidComponentHandle(in_invalidhandle);    
  DXFree((Pointer)scratch);
  DXFreeArrayHandle(handle);
  DXFree((Pointer)bufdistance);
  DXFree((Pointer)bufdata);
  DXFree((Pointer)data);
  DXFree((Pointer)missingval_f);
  DXFree((Pointer)missingval_s);
  DXFree((Pointer)missingval_i);
  DXFree((Pointer)missingval_d);
  DXFree((Pointer)missingval_b);
  DXFree((Pointer)missingval_ui);
  DXFree((Pointer)missingval_ub);
  DXFree((Pointer)missingval_us);
  return ERROR; 
}



static Error ConnectRadius(Field ino, Field base, float radius, 
                           float exponent, Array missing)
{
  Array positions_base;

  if (DXEmptyField((Field)ino))
     return OK;
  
  positions_base = (Array)DXGetComponentValue((Field)base,"positions");
  if (!positions_base) {
    DXSetError(ERROR_MISSING_DATA,"missing positions component in base");
    goto error;
  }
  
  if (DXQueryGridPositions((Array)positions_base,NULL,NULL,NULL,NULL)) {
    if (!ConnectRadiusRegular(ino,base,radius,exponent, missing))
      goto error;
  }
  else {
    if (!ConnectRadiusIrregular(ino,base,radius,exponent, missing))
      goto error;
  }
  if (!DXEndField(base)) goto error;
  return OK;
 error:
  return ERROR;
}

static Error ConnectRadiusIrregular(Field ino, Field base, float radius, 
				    float exponent, Array missing) 
{
  int componentsdone, valid;
  Array positions_base, positions_ino;
  Array oldcomponent=NULL, newcomponent=NULL;
  InvalidComponentHandle out_invalidhandle=NULL, in_invalidhandle=NULL;
  int numitemsbase, numitemsino; 
  int rank_base, rank_ino, shape_base[30], shape_ino[30], i, j; 
  int anyinvalid=1, compcount, rank, shape[30];
  int numitems;
  Type type;
  ubyte *flagbuf=NULL;
  float *distancebuf=NULL; 
  Category category;
  float *pos_ino_ptr, *pos_base_ptr;
  char *name;
  
  /* all points within radius of a given grid position will be averaged 
   * (weighted) to give a data point to that position. If there are are
   * none within that range, the missing value is used. If missing is
   * NULL, then an invalid position indication is placed there and cull
   * will be used at the end */
 
  in_invalidhandle = DXCreateInvalidComponentHandle((Object)ino, NULL,
                                                    "positions");
  if (!in_invalidhandle)
     goto error; 
  if (!missing) {
    out_invalidhandle = DXCreateInvalidComponentHandle((Object)base,
                                                        NULL, "positions");
    if (!out_invalidhandle) 
       goto error;
    anyinvalid = 0;
  }
  
  positions_base = (Array)DXGetComponentValue((Field)base, "positions");
  if (!positions_base) {
    DXSetError(ERROR_MISSING_DATA,"base field has no positions");
    goto error;
  }
  positions_ino = (Array)DXGetComponentValue((Field)ino, "positions");
  if (!positions_ino) {
    DXSetError(ERROR_MISSING_DATA,"input field has no positions");
    goto error;
  }
  DXGetArrayInfo(positions_base, &numitemsbase, &type, &category, &rank_base, 
		 shape_base);
  if ((type != TYPE_FLOAT)||(category != CATEGORY_REAL)) {
    DXSetError(ERROR_DATA_INVALID,
	       "only real, floating point positions for base accepted");
    goto error;
  }
  if ((rank_base > 1 ) || (shape_base[0] > 3)) {
    DXSetError(ERROR_DATA_INVALID,
	       "only 1D, 2D, or 3D positions for base accepted");
    goto error;
  }
  DXGetArrayInfo(positions_ino, &numitemsino, &type, &category, &rank_ino, 
		 shape_ino);
  if ((type != TYPE_FLOAT)||(category != CATEGORY_REAL)) {
    DXSetError(ERROR_DATA_INVALID,
	       "only real, floating point positions for input accepted");
    goto error;
  }
  if ((rank_ino > 1 ) || (shape_ino[0] > 3)) {
    DXSetError(ERROR_DATA_INVALID,
	       "only 1D, 2D, or 3D positions for input accepted");
    goto error;
  }
  
  if (shape_base[0] != shape_ino[0]) {
    DXSetError(ERROR_DATA_INVALID,
	       "base positions are %dD; input positions are %dD",
	       shape_base[0], shape_ino[0]);
    goto error;
  }
  
  
  /* regular arrays? XXX */
  pos_ino_ptr = (float *)DXGetArrayData(positions_ino);
  
  /* make a buffer to hold the distances and flags */
  flagbuf = (ubyte *)DXAllocateLocalZero((numitemsbase*numitemsino*sizeof(ubyte)));
  if (!flagbuf) goto error;
  distancebuf = 
    (float *)DXAllocateLocalZero((numitemsbase*numitemsino*sizeof(float)));
  if (!distancebuf) goto error;
  
  if (out_invalidhandle) {
    DXSetAllValid(out_invalidhandle); 
  }
  
  /* call irregular method */
  pos_base_ptr = (float *)DXGetArrayData(positions_base);
  if (!(FillBuffersIrreg(numitemsbase, numitemsino, flagbuf, distancebuf,  
			 pos_ino_ptr, pos_base_ptr, out_invalidhandle,
                         in_invalidhandle,  
			 &anyinvalid, shape_base[0], radius)))
    goto error; 
  
  
  /* need to get the components we are going to copy into the new field */
  /* copied wholesale from gda */
  compcount = 0;
  componentsdone=0;
  while (NULL != 
	 (oldcomponent=(Array)DXGetEnumeratedComponentValue((Field)ino, 
							     compcount, 
							     &name))) {
    Object attr;
  
    if (DXGetObjectClass((Object)oldcomponent) != CLASS_ARRAY)
      goto component_done;
    
    if (!strcmp(name,"positions") || 
        !strcmp(name,"connections") ||
        !strcmp(name,"invalid positions")) 
      goto component_done;
    
    /* if the component refs anything, leave it */
    if (DXGetComponentAttribute((Field)ino,name,"ref"))
      goto component_done;
    
    attr = DXGetComponentAttribute((Field)ino,name,"der");
    if (attr)
       goto component_done;

    attr = DXGetComponentAttribute((Field)ino,name,"dep");
    if (!attr) {
      /* does it der? if so, throw it away */
      attr = DXGetComponentAttribute((Field)ino,name,"der");
      if (attr)
        goto component_done;
      /* it doesn't ref, doesn't dep and doesn't der. 
         Copy it to the output and quit */
      if (!DXSetComponentValue((Field)base,name,(Object)oldcomponent))
         goto error; 
      goto component_done;
    }

    
    if (DXGetObjectClass(attr) != CLASS_STRING) {
      DXSetError(ERROR_DATA_INVALID, "dependency attribute not type string");
      goto error;
    }
    
    if (strcmp(DXGetString((String)attr),"positions")){
      DXWarning("Component '%s' is not dependent on positions!",name); 
      DXWarning("Regrid will remove '%s' and output the base grid 'data' component if it exists",name);
      goto component_done; 
    }


    /* if the component happens to be "invalid positions" ignore it */
    if (!strcmp(name,"invalid positions"))
      goto component_done;


    componentsdone++;
    
    DXGetArrayInfo((Array)oldcomponent, &numitems, &type, &category, 
		   &rank, shape);
  
 


    if (rank == 0)
      shape[0] = 1;
    if (rank > 1) {
      DXSetError(ERROR_NOT_IMPLEMENTED,
                 "rank larger than 1 not implemented");
      goto error;
    }
    if (numitems != numitemsino) {
      DXSetError(ERROR_BAD_PARAMETER,
		 "number of items in %s component not equal to number of positions", name);
      goto error;
    }
    newcomponent = DXNewArrayV(type, category, rank, shape);

    
    if (!DXAddArrayData(newcomponent, 0, numitemsbase, NULL))
      goto error;
    
    
    /* here's where I use my preset buffers to fill the new array */
    /* XXX here do something about missing */ 
    if (!FillDataArray(type, oldcomponent, newcomponent, 
		       flagbuf, distancebuf, 
                       numitemsbase, numitemsino,
                       shape[0], exponent, missing, name))
	goto error;
    
    if (!DXChangedComponentValues((Field)base,name))
      goto error;
    if (!DXSetComponentValue((Field)base,name,(Object)newcomponent))
      goto error;
    newcomponent = NULL;
    if (!DXSetComponentAttribute((Field)base, name,
				 "dep", 
				 (Object)DXNewString("positions"))) 
      goto error;
  component_done:
    compcount++;
  }


  if (componentsdone==0) {
    if (missing) {
      DXWarning("no position-dependent components found; missing value provided cannot be used");
    }
    else {
      for (i=0; i<numitemsbase; i++) {
	valid = 0;
	for (j=0; j<numitemsino; j++) {
	  if (flagbuf[i*numitemsino+j]) { 
            /* there's at least one valid */
            valid = 1;
            break;
	  }
	}
	if (!valid) {
	  DXSetElementInvalid(out_invalidhandle, i);
	  anyinvalid=1;
	}
      }
    }
  }
  else {
    if (!missing) {
      for (i=0; i<numitemsbase; i++) {
	valid = 0;
	for (j=0; j<numitemsino; j++) {
	  if (flagbuf[i*numitemsino+j]) {
            /* there's at least one valid */
            valid = 1;
            break;
	  }
	}
	if (!valid) {
	  DXSetElementInvalid(out_invalidhandle, i);
	  anyinvalid=1;
	}
      }
    }
  }
  
  if (anyinvalid && out_invalidhandle) {
    if (!DXSaveInvalidComponent(base, out_invalidhandle))
      goto error;
  }
  
  DXFree((Pointer)flagbuf);
  DXFree((Pointer)distancebuf);
  DXFreeInvalidComponentHandle(out_invalidhandle);
  DXFreeInvalidComponentHandle(in_invalidhandle);
  return OK;
  
 error:
  DXDelete((Object)newcomponent);
  DXFree((Pointer)flagbuf);
  DXFree((Pointer)distancebuf);
  DXFreeInvalidComponentHandle(out_invalidhandle);
  DXFreeInvalidComponentHandle(in_invalidhandle);
  return ERROR;
  
}

static Error ConnectRadiusRegular(Field ino, Field base, float radius, 
                                  float exponent, Array missing) 
{
  int componentsdone;
  Array positions_base, positions_ino;
  Array oldcomponent, newcomponent=NULL;
  InvalidComponentHandle in_invalidhandle=NULL, out_invalidhandle=NULL;
  int numitemsbase, numitemsino, cntr; 
  int rank_base, rank_ino, shape_base[30], shape_ino[30], i, j, k, l, c; 
  int compcount, rank, shape[30];
  float tmpval;
  int numitems, counts[8], knt;
  Type type;
  float xpos, ypos, zpos, xmin, ymin, zmin, xmax, ymax, zmax, pos;
  float min, max;
  Vector minvec, maxvec, posvec;
  Vector2D minvec2D, maxvec2D, posvec2D;
  Matrix matrix, invmatrix;
  Matrix2D matrix2D, invmatrix2D;
  Category category;
  float *pos_ino_ptr, distance, *weights_ptr=NULL;
  float *data_ptr = NULL;
  float rsquared;
  byte *exact_hit;
  Point box[8];
  char *name;
  float    *data_in_f=NULL, *data_f=NULL, *missingval_f=NULL;
  byte     *data_in_b=NULL, *data_b=NULL, *missingval_b=NULL;
  int      *data_in_i=NULL, *data_i=NULL, *missingval_i=NULL;
  short    *data_in_s=NULL, *data_s=NULL, *missingval_s=NULL;
  double   *data_in_d=NULL, *data_d=NULL, *missingval_d=NULL;
  ubyte    *data_in_ub=NULL, *data_ub=NULL, *missingval_ub=NULL;
  ushort   *data_in_us=NULL, *data_us=NULL, *missingval_us=NULL;
  uint     *data_in_ui=NULL, *data_ui=NULL, *missingval_ui=NULL;



  
  positions_base = (Array)DXGetComponentValue((Field)base, "positions");
  if (!positions_base) {
    DXSetError(ERROR_MISSING_DATA,"base field has no positions");
    goto error;
  }
  if (!(DXQueryGridPositions((Array)positions_base,NULL,counts,
                     (float *)&matrix.b,(float *)&matrix.A) ))
      goto error;
  /* all points within radius of a given grid position will be averaged 
   * (weighted) to give a data point to that position. If there are are
   * none within that range, the missing value is used. If missing is
   * NULL, then an invalid position indication is placed there  */
  DXGetArrayInfo(positions_base, &numitemsbase, &type, &category, 
                 &rank_base, shape_base);
  out_invalidhandle=DXCreateInvalidComponentHandle((Object)base,
                                                    NULL,"positions");
  if (!out_invalidhandle) goto error;


  DXSetAllInvalid(out_invalidhandle);

  /* if radius==-1, that's infinity. I really ought to handle it separately */
  /* XXX */
  if (radius == -1) {
     if (!DXBoundingBox((Object)ino, box)) 
        goto error;
     radius = 2*DXLength(DXSub(box[0], box[7]));
  }
  
  positions_ino = (Array)DXGetComponentValue((Field)ino, "positions");
  if (!positions_ino) {
    DXSetError(ERROR_MISSING_DATA,"input field has no positions");
    goto error;
  }
  if ((type != TYPE_FLOAT)||(category != CATEGORY_REAL)) {
    DXSetError(ERROR_DATA_INVALID,
             "only real, floating point positions for base accepted");
    goto error;
  }
  if ((rank_base > 1 ) || (shape_base[0] > 3)) {
    DXSetError(ERROR_DATA_INVALID,
             "only 1D, 2D, or 3D positions for base accepted");
    goto error;
  }
  DXGetArrayInfo(positions_ino, &numitemsino, &type, &category, 
                 &rank_ino, shape_ino);
  if ((type != TYPE_FLOAT)||(category != CATEGORY_REAL)) {
    DXSetError(ERROR_DATA_INVALID,
             "only real, floating point positions for input accepted");
    goto error;
  }
  if ((rank_ino > 1 ) || (shape_ino[0] > 3)) {
    DXSetError(ERROR_DATA_INVALID,
             "only 1D, 2D, or 3D positions for input accepted");
    goto error;
  }

  if (shape_base[0] != shape_ino[0]) {
     DXSetError(ERROR_DATA_INVALID,
              "base positions are %dD; input positions are %dD",
              shape_base[0], shape_ino[0]);
     goto error;
  }


  /* regular arrays? XXX */
  pos_ino_ptr = (float *)DXGetArrayData(positions_ino);



  /* need to get the components we are going to copy into the new field */
  /* copied wholesale from gda */
  compcount = 0;
  componentsdone=0;
  while (NULL != (oldcomponent=(Array)DXGetEnumeratedComponentValue((Field)ino, 
							      compcount, 
							      &name))) {
    Object attr;
    
    if (DXGetObjectClass((Object)oldcomponent) != CLASS_ARRAY)
      goto component_done;
    
    if (!strcmp(name,"positions") || 
        !strcmp(name,"connections") ||
        !strcmp(name,"invalid positions")) 
      goto component_done;
    
    /* if the component refs anything, leave it */
    if (DXGetComponentAttribute((Field)ino,name,"ref"))
      goto component_done;
    
    attr = DXGetComponentAttribute((Field)ino,name,"der");
    if (attr)
       goto component_done;

    attr = DXGetComponentAttribute((Field)ino,name,"dep");
    if (!attr) {
      /* does it der? if so, throw it away */
      attr = DXGetComponentAttribute((Field)ino,name,"der");
      if (attr)
        goto component_done;
      /* it doesn't ref, doesn't dep and doesn't der. 
         Copy it to the output and quit */
      if (!DXSetComponentValue((Field)base,name,(Object)oldcomponent))
         goto error; 
      goto component_done;
    }


    
    if (DXGetObjectClass(attr) != CLASS_STRING) {
      DXSetError(ERROR_DATA_INVALID, 
                 "dependency attribute not type string");
      goto error;
    }
    
    if (strcmp(DXGetString((String)attr),"positions")){
      DXWarning("Component '%s' is not dependent on positions!",name); 
      DXWarning("Regrid will remove '%s' and output the base grid 'data' component if it exists",name);
      goto component_done; 
    }


    /* if the component happens to be "invalid positions" ignore it */
    if (!strcmp(name,"invalid positions"))
      goto component_done;

    componentsdone++;
    
    DXGetArrayInfo((Array)oldcomponent,&numitems, &type, &category, 
                    &rank, shape);
    if (rank==0) shape[0]=1;

    /* check that the missing component, if present, matches the component
       we're working on */
    if (missing) 
    switch (type) {
      case (TYPE_FLOAT):
        CHECK_AND_ALLOCATE_MISSING(missing, DXGetItemSize(oldcomponent), 
                                   float, type, shape[0], missingval_f);
        break;
      case (TYPE_INT):
        CHECK_AND_ALLOCATE_MISSING(missing, DXGetItemSize(oldcomponent), int,
                                   type, shape[0], missingval_i);
        break;
      case (TYPE_BYTE):
        CHECK_AND_ALLOCATE_MISSING(missing, DXGetItemSize(oldcomponent), byte,
                                   type, shape[0], missingval_b);
        break;
      case (TYPE_DOUBLE):
        CHECK_AND_ALLOCATE_MISSING(missing, DXGetItemSize(oldcomponent), 
                                   double, type, shape[0], missingval_d);
        break;
      case (TYPE_SHORT):
        CHECK_AND_ALLOCATE_MISSING(missing, DXGetItemSize(oldcomponent), short,
                                   type, shape[0], missingval_s);
        break;
      case (TYPE_UBYTE):
        CHECK_AND_ALLOCATE_MISSING(missing, DXGetItemSize(oldcomponent), ubyte,
                                   type, shape[0], missingval_ub);
        break;
      case (TYPE_USHORT):
        CHECK_AND_ALLOCATE_MISSING(missing, DXGetItemSize(oldcomponent), 
                                   ushort, type, shape[0], missingval_us);
        break;
      case (TYPE_UINT):
        CHECK_AND_ALLOCATE_MISSING(missing, DXGetItemSize(oldcomponent), uint,
                                   type, shape[0], missingval_ui);
        break;
      default:
        DXSetError(ERROR_DATA_INVALID,"unsupported data type");
        goto error;
    }





    if (rank == 0)
       shape[0] = 1;
    if (rank > 1) {
       DXSetError(ERROR_NOT_IMPLEMENTED,"rank larger than 1 not implemented");
       goto error;
    }
    if (numitems != numitemsino) {
      DXSetError(ERROR_BAD_PARAMETER,
	       "number of items in %s component not equal to number of positions", name);
      goto error;
    }
    
    weights_ptr = (float *)DXAllocateLocalZero(numitemsbase*sizeof(float));
    /* I need a floating point buffer to hold the weighted sum data */
    data_ptr = 
     (float *)DXAllocateLocalZero(numitemsbase*sizeof(float)*shape[0]);
    if (!weights_ptr) goto error;
    if (!data_ptr) goto error;
  
    /* new component is for the eventual output data */
    newcomponent = DXNewArrayV(type, category, rank, shape);
    if (!DXAddArrayData(newcomponent, 0, numitemsbase, NULL))
      goto error;

    /* allocate the exact hit buf */                   
    exact_hit = (byte *)DXAllocateLocalZero(numitemsbase*sizeof(byte));


    switch (type) {
     case (TYPE_FLOAT):
        data_in_f = (float *)DXGetArrayData(oldcomponent);          
        data_f = (float *)DXGetArrayData(newcomponent);          
        break;
     case (TYPE_INT):
        data_in_i = (int *)DXGetArrayData(oldcomponent);          
        data_i = (int *)DXGetArrayData(newcomponent);          
        break;
     case (TYPE_DOUBLE):
        data_in_d = (double *)DXGetArrayData(oldcomponent);        
        data_d = (double *)DXGetArrayData(newcomponent);          
        break;
     case (TYPE_SHORT):
        data_in_s = (short *)DXGetArrayData(oldcomponent);         
        data_s = (short *)DXGetArrayData(newcomponent);          
        break;
     case (TYPE_BYTE):
        data_in_b = (byte *)DXGetArrayData(oldcomponent);        
        data_b = (byte *)DXGetArrayData(newcomponent);          
        break;
     case (TYPE_UBYTE):
        data_in_ub = (ubyte *)DXGetArrayData(oldcomponent);         
        data_ub = (ubyte *)DXGetArrayData(newcomponent);          
        break;
     case (TYPE_USHORT):
        data_in_us = (ushort *)DXGetArrayData(oldcomponent); 
        data_us = (ushort *)DXGetArrayData(newcomponent);          
        break;
     case (TYPE_UINT):
        data_in_ui = (uint *)DXGetArrayData(oldcomponent);         
        data_ui = (uint *)DXGetArrayData(newcomponent);          
        break;
     default:
        DXSetError(ERROR_DATA_INVALID,"unsupported data type");
        goto error;
    }
    
#define INCREMENT_DATA_1D(type, data_in)                           \
    {                                                              \
      for (i=0; i<numitemsino; i++) {                              \
	xpos = pos_ino_ptr[i];                                     \
	xmin = xpos-radius;                                        \
	xmax = xpos+radius;                                        \
	min = (xmin/matrix.A[0][0])-(matrix.b[0]/matrix.A[0][0]);  \
	max = (xmax/matrix.A[0][0])-(matrix.b[0]/matrix.A[0][0]);  \
        if (min > max) {                                           \
           tmpval = min;                                           \
           min = max;                                              \
           max = tmpval;                                           \
        }                                                          \
	/* minvec and maxvec are the indices corresponding to the  \
	   bounding box around the sample point */                 \
	for (j=CEIL(min); j <= FLOOR(max) ; j++)                   \
          if (j>=0 && j<counts[0]) {                               \
	    /* here are the only ones which are in the bounding    \
	     * box */                                              \
	    pos = j*matrix.A[0][0]+matrix.b[0];                    \
	    distance = ABS(pos-xpos);                                   \
	    if (distance <= radius) {                              \
              DXSetElementValid(out_invalidhandle, j);             \
	      if ((distance > 0.0)&&(!exact_hit[j])) {             \
		weights_ptr[j] += 1.0/POW(distance,exponent);      \
		for (c = 0; c<shape[0]; c++)                       \
		  data_ptr[j*shape[0]+c] +=                        \
		    (float)data_in[i*shape[0]+c]/POW(distance,exponent); \
	      }                                                    \
	      else if (!exact_hit[j]){                             \
                /* we've hit right on; no more averaging here */     \
	        for (c = 0; c<shape[0]; c++)                         \
	  	  data_ptr[j*shape[0]+c] =                           \
		    (float)data_in[i*shape[0]+c];                    \
                weights_ptr[j]=1;                                    \
                exact_hit[j]=1;                                      \
	    }                                                      \
	  }                                                        \
        }                                                      \
      }                                                            \
    }                                                              \

#define INCREMENT_DATA_2D(type, data_in)                           \
    {                                                              \
      Matrix2D tmpmat;                                             \
      matrix2D.A[0][0] = matrix.A[0][0];                           \
      matrix2D.A[1][0] = matrix.A[0][1];                           \
      matrix2D.A[0][1] = matrix.A[0][2];                           \
      matrix2D.A[1][1] = matrix.A[1][0];                           \
      matrix2D.b[0] = matrix.b[0];                                 \
      matrix2D.b[1] = matrix.b[1];                                 \
      invmatrix2D = Invert2D(matrix2D);                            \
      /* loop over all scattered points */                         \
      rsquared = radius* radius;                                   \
      for (i=0; i<numitemsino; i++) {                              \
	xpos = pos_ino_ptr[2*i];                                   \
	ypos = pos_ino_ptr[2*i+1];                                 \
	xmin = xpos-radius;                                        \
	xmax = xpos+radius;                                        \
	ymin = ypos-radius;                                        \
	ymax = ypos+radius;                                        \
        tmpmat = invmatrix2D;                                       \
	minvec2D = Apply2D(Vec2D(xmin,ymin), tmpmat); \
        maxvec2D = Apply2D(Vec2D(xmax,ymax), tmpmat);         \
        if (minvec2D.x > maxvec2D.x) {                        \
           tmpval = minvec2D.x;                               \
           minvec2D.x = maxvec2D.x;                           \
           maxvec2D.x = tmpval;                               \
        }                                                     \
        if (minvec2D.y > maxvec2D.y) {                        \
           tmpval = minvec2D.y;                               \
           minvec2D.y = maxvec2D.y;                           \
           maxvec2D.y = tmpval;                               \
        }                                                     \
	/* minvec and maxvec are the indices corresponding to      \
           the bounding box around the sample point */             \
        tmpmat = matrix2D;                                         \
	for (j=CEIL(minvec2D.x); j <= FLOOR(maxvec2D.x) ; j++)     \
          if (j>=0 && j<counts[0])                                 \
            for (k=CEIL(minvec2D.y); k <= FLOOR(maxvec2D.y) ; k++) \
              if (k>=0 && k<counts[1]) {                           \
		/* here are the only ones which are in the         \
                   bounding box */                                 \
		posvec2D = Apply2D(Vec2D(j,k),tmpmat);             \
		distance = (posvec2D.x-xpos)*(posvec2D.x-xpos) +   \
		  (posvec2D.y-ypos)*(posvec2D.y-ypos);             \
		if (distance <= rsquared) {                        \
                  distance = sqrt(distance);                       \
                  cntr = j*counts[1]+k;                            \
                  DXSetElementValid(out_invalidhandle, cntr);      \
	          if ((distance > 0.0)&&(!exact_hit[cntr])) {      \
	    	    weights_ptr[cntr] += 1.0/POW(distance,exponent); \
		    for (c=0; c<shape[0]; c++)                     \
		      data_ptr[j*counts[1]*shape[0] + k*shape[0] +c] \
			+= (float)data_in[i*shape[0]+c]/POW(distance,exponent);  \
	          }                                                \
		  else if (!exact_hit[cntr]) {                    \
		    for (c=0; c<shape[0]; c++)                     \
		      data_ptr[j*counts[1]*shape[0] + k*shape[0]]    \
			= (float)data_in[i*shape[0]+c];            \
                    weights_ptr[cntr]=1;                           \
                    exact_hit[cntr]=1;                             \
		  }                                                \
		}                                                  \
	      }                                                    \
      }                                                            \
    }                                                              \

#define INCREMENT_DATA_3D(type, data_in)                           \
    {                                                              \
      Matrix tmpmat;                                               \
      invmatrix = DXInvert(matrix);                                \
      /* loop over all scattered points */                         \
      for (knt=0, i=0; i<numitemsino; i++) {                       \
	xpos = pos_ino_ptr[knt];                                   \
	ypos = pos_ino_ptr[knt+1];                                 \
	zpos = pos_ino_ptr[knt+2];                                 \
        knt+=3;                                                    \
	xmin = xpos-radius;                                        \
	xmax = xpos+radius;                                        \
	ymin = ypos-radius;                                        \
	ymax = ypos+radius;                                        \
	zmin = zpos-radius;                                        \
	zmax = zpos+radius;                                        \
        tmpmat=invmatrix;                                          \
	minvec = DXApply(DXVec(xmin,ymin,zmin), tmpmat);        \
	maxvec = DXApply(DXVec(xmax,ymax,zmax), tmpmat);        \
        if (minvec.x > maxvec.x) {                              \
           tmpval = minvec.x;                                   \
           minvec.x = maxvec.x;                                 \
           maxvec.x = tmpval;                                   \
        }                                                       \
        if (minvec.y > maxvec.y) {                              \
           tmpval = minvec.y;                                   \
           minvec.y = maxvec.y;                                 \
           maxvec.y = tmpval;                                   \
        }                                                       \
        if (minvec.z > maxvec.z) {                              \
           tmpval = minvec.z;                                   \
           minvec.z = maxvec.z;                                 \
           maxvec.z = tmpval;                                   \
        }                                                       \
	/* minvec and maxvec are the indices corresponding to the */ \
	/* bounding box around the sample point */                 \
        tmpmat=matrix;                                             \
        rsquared = radius * radius;                                \
	for (j=CEIL(minvec.x); j <= FLOOR(maxvec.x) ; j++)         \
          if (j>=0 && j<counts[0])                                 \
            for (k=CEIL(minvec.y); k <= FLOOR(maxvec.y) ; k++)     \
              if (k>=0 && k<counts[1])                             \
                for (l=CEIL(minvec.z); l <= FLOOR(maxvec.z) ; l++) \
                  if (l>=0 && l<counts[2]) {                       \
		    /* here are the only ones which are in the     \
                       bounding box */                             \
		    posvec = DXApply(DXVec(j,k,l),tmpmat);         \
		    distance = (posvec.x-xpos)*(posvec.x-xpos) +   \
		               (posvec.y-ypos)*(posvec.y-ypos) +   \
			       (posvec.z-zpos)*(posvec.z-zpos);    \
		    if (distance <= rsquared) {                      \
                      distance = sqrt(distance);                     \
                      cntr = j*counts[2]*counts[1] +               \
                             k*counts[2] + l;                      \
                      DXSetElementValid(out_invalidhandle, cntr);  \
		      if ((distance > 0.0)&&(!exact_hit[cntr]))  { \
			weights_ptr[cntr] += 1.0/POW(distance,exponent); \
			for (c=0; c<shape[0]; c++)                  \
			  data_ptr[j*counts[2]*counts[1]*shape[0] +   \
				 k*counts[2]*shape[0] + l*shape[0]+c] \
                               += (float)data_in[i*shape[0]+c]/POW(distance,exponent); \
		      }                                               \
		      else if (!exact_hit[cntr]){                     \
			for (c=0; c<shape[0]; c++)                    \
			  data_ptr[j*counts[2]*counts[1]*shape[0] +     \
				 k*counts[2]*shape[0] + l*shape[0] +c] \
                                = (float)data_in[i*shape[0]+c];       \
                          weights_ptr[cntr]=1;                       \
                          exact_hit[cntr] = 1;                        \
		      }                                               \
		    }                                                 \
                  }                                                   \
      }                                                               \
    }                                                                 \


    if (shape_base[0] == 1) {
       switch (type) {
         case(TYPE_FLOAT):
           INCREMENT_DATA_1D(float, data_in_f);
           break;
         case(TYPE_INT):
           INCREMENT_DATA_1D(int, data_in_i);
           break;
         case(TYPE_BYTE):
           INCREMENT_DATA_1D(byte, data_in_b);
           break;
         case(TYPE_SHORT):
           INCREMENT_DATA_1D(short, data_in_s);
           break;
         case(TYPE_DOUBLE):
           INCREMENT_DATA_1D(double, data_in_d);
           break;
         case(TYPE_UBYTE):
           INCREMENT_DATA_1D(ubyte, data_in_ub);
           break;
         case(TYPE_UINT):
           INCREMENT_DATA_1D(uint, data_in_ui);
           break;
         case(TYPE_USHORT):
           INCREMENT_DATA_1D(ushort, data_in_us);
           break;
         default:
           DXSetError(ERROR_DATA_INVALID,"unsupported data type");
           goto error;
       }
    }
    else if (shape_base[0] == 2) {
      switch (type) {
         case(TYPE_FLOAT):
           INCREMENT_DATA_2D(float, data_in_f);
           break;
         case(TYPE_INT):
           INCREMENT_DATA_2D(int, data_in_i);
           break;
         case(TYPE_BYTE):
           INCREMENT_DATA_2D(byte, data_in_b);
           break;
         case(TYPE_SHORT):
           INCREMENT_DATA_2D(short, data_in_s);
           break;
         case(TYPE_DOUBLE):
           INCREMENT_DATA_2D(double, data_in_d);
           break;
         case(TYPE_UBYTE):
           INCREMENT_DATA_2D(ubyte, data_in_ub);
           break;
         case(TYPE_UINT):
           INCREMENT_DATA_2D(uint, data_in_ui);
           break;
         case(TYPE_USHORT):
           INCREMENT_DATA_2D(ushort, data_in_us);
           break;
         default:
           DXSetError(ERROR_DATA_INVALID,"unsupported data type");
           goto error;
         }
    }
    else if (shape_base[0] ==3) {
      switch (type) {
         case(TYPE_FLOAT):
           INCREMENT_DATA_3D(float, data_in_f);
           break;
         case(TYPE_INT):
           INCREMENT_DATA_3D(int, data_in_i);
           break;
         case(TYPE_BYTE):
           INCREMENT_DATA_3D(byte, data_in_b);
           break;
         case(TYPE_SHORT):
           INCREMENT_DATA_3D(short, data_in_s);
           break;
         case(TYPE_DOUBLE):
           INCREMENT_DATA_3D(double, data_in_d);
           break;
         case(TYPE_UBYTE):
           INCREMENT_DATA_3D(ubyte, data_in_ub);
           break;
         case(TYPE_UINT):
           INCREMENT_DATA_3D(uint, data_in_ui);
           break;
         case(TYPE_USHORT):
           INCREMENT_DATA_3D(ushort, data_in_us);
           break;
         default:
           DXSetError(ERROR_DATA_INVALID,"unsupported data type");
           goto error;
       }
    }
    else {
      DXSetError(ERROR_DATA_INVALID,
	       "only 1D, 2D, and 3D input positions accepted");
      goto error;
    }


    /* now fill the actual data array (int or float or whatever) 
       with the computed sums */
    switch (type) {
    case (TYPE_FLOAT):
      for (i=0; i<numitemsbase; i++) {
        if (DXIsElementValid(out_invalidhandle, i))
          for (c=0; c<shape[0]; c++)
	    data_f[i*shape[0]+c] = data_ptr[i*shape[0]+c]/weights_ptr[i];
	else {
          /* XXX here do something about missing */
	  if (missing) 
            for (c=0; c<shape[0]; c++)
              data_f[i*shape[0]+c] = missingval_f[c];
          else
            for (c=0; c<shape[0]; c++)
              data_f[i*shape[0]+c] = 0;
        }
      }
      break;
      case (TYPE_INT):
      for (i=0; i<numitemsbase; i++) {
        if (DXIsElementValid(out_invalidhandle, i))
          for (c=0; c<shape[0]; c++)
	    data_i[i*shape[0]+c] = 
                 (int)(RND(data_ptr[i*shape[0]+c]/weights_ptr[i]));
	else {
	  if (missing) 
            for (c=0; c<shape[0]; c++)
              data_i[i*shape[0]+c] = (int)missingval_i[c];
          else
            for (c=0; c<shape[0]; c++)
              data_i[i*shape[0]+c] = (int)0;
        }
      }
      break;
    case (TYPE_SHORT):
      for (i=0; i<numitemsbase; i++) {
        if (DXIsElementValid(out_invalidhandle, i))
          for (c=0; c<shape[0]; c++)
	    data_s[i*shape[0]+c] 
               = (short)(RND(data_ptr[i*shape[0]+c]/weights_ptr[i]));
	else {
	  if (missing) 
            for (c=0; c<shape[0]; c++)
              data_s[i*shape[0]+c] =(short)missingval_s[c];
          else
            for (c=0; c<shape[0]; c++)
              data_s[i*shape[0]+c] =(short)0;
          }
      }
      break;
    case (TYPE_BYTE):
      for (i=0; i<numitemsbase; i++) {
        if (DXIsElementValid(out_invalidhandle, i))
          for (c=0; c<shape[0]; c++)
	    data_b[i*shape[0]+c] = 
                  (byte)(RND(data_ptr[i*shape[0]+c]/weights_ptr[i]));
	else {
	  if (missing) 
            for (c=0; c<shape[0]; c++)
              data_b[i*shape[0]+c] = (byte)missingval_b[c];
          else
            for (c=0; c<shape[0]; c++)
              data_b[i*shape[0]+c] = (byte)0;
        }
      }
      break;
    case (TYPE_DOUBLE):
      for (i=0; i<numitemsbase; i++) {
        if (DXIsElementValid(out_invalidhandle, i))
          for (c=0; c<shape[0]; c++)
	    data_d[i*shape[0]+c] = 
                 (double)(data_ptr[i*shape[0]+c]/weights_ptr[i]);
	else {
	  if (missing) 
            for (c=0; c<shape[0]; c++)
              data_d[i*shape[0]+c] = (double)missingval_d[c];
          else
            for (c=0; c<shape[0]; c++)
              data_d[i*shape[0]+c] = (double)0;
        }
      }
      break;
    case (TYPE_UINT):
      for (i=0; i<numitemsbase; i++) {
        if (DXIsElementValid(out_invalidhandle, i))
          for (c=0; c<shape[0]; c++)
	    data_ui[i*shape[0]+c] = 
                (uint)(RND(data_ptr[i*shape[0]+c]/weights_ptr[i]));
	else {
	  if (missing) 
            for (c=0; c<shape[0]; c++)
              data_ui[i*shape[0]+c] = (uint)missingval_ui[c];
          else 
            for (c=0; c<shape[0]; c++)
              data_ui[i*shape[0]+c] = (uint)0;
        }
      }
      break;
    case (TYPE_USHORT):
      for (i=0; i<numitemsbase; i++) {
        if (DXIsElementValid(out_invalidhandle, i))
          for (c=0; c<shape[0]; c++)
	    data_us[i*shape[0]+c] = 
                (ushort)(RND(data_ptr[i*shape[0]+c]/weights_ptr[i]));
	else {
	  if (missing) 
            for (c=0; c<shape[0]; c++)
              data_us[i*shape[0]+c] = (ushort)missingval_us[c];
          else
            for (c=0; c<shape[0]; c++)
              data_us[i*shape[0]+c] = (ushort)0;
          }
      }
      break;
    case (TYPE_UBYTE):
      for (i=0; i<numitemsbase; i++) {
        if (DXIsElementValid(out_invalidhandle, i))
          for (c=0; c<shape[0]; c++)
	    data_ub[i*shape[0]+c] = 
                (ubyte)(RND(data_ptr[i*shape[0]+c]/weights_ptr[i]));
	else {
	  if (missing) 
            for (c=0; c<shape[0]; c++)
              data_ub[i*shape[0]+c] = (ubyte)missingval_ub[c];
          else
            for (c=0; c<shape[0]; c++)
              data_ub[i*shape[0]+c] = (ubyte)0;
        }
      }
      break;
      default:
        DXSetError(ERROR_DATA_INVALID,"unsupported data type");
        goto error;
    }
    
    if (!DXSetComponentValue((Field)base,name,(Object)newcomponent))
      goto error;
    if (!DXSetComponentAttribute((Field)base, name,
			       "dep", 
			       (Object)DXNewString("positions"))) 
      goto error;
    newcomponent = NULL;
    if (!DXChangedComponentValues((Field)base,name))
      goto error;
  component_done:
    compcount++;
    DXFree((Pointer)missingval_f);
    missingval_f=NULL;
    DXFree((Pointer)missingval_b);
    missingval_b=NULL;
    DXFree((Pointer)missingval_i);
    missingval_i=NULL;
    DXFree((Pointer)missingval_s);
    missingval_s=NULL;
    DXFree((Pointer)missingval_d);
    missingval_d=NULL;
    DXFree((Pointer)missingval_ub);
    missingval_ub=NULL;
    DXFree((Pointer)missingval_us);
    missingval_us=NULL;
    DXFree((Pointer)missingval_ui);
    missingval_ui=NULL;
  }


  if (componentsdone==0) {
    if (missing) {
      DXWarning("no position-dependent components found; cannot use missing value provided");
    }
    else {
      switch (shape_base[0]) {
      case (1):
	for (i=0; i<numitemsino; i++) { 
	  xpos = pos_ino_ptr[i]; 
	  xmin = xpos-radius; 
	  xmax = xpos+radius;
	  min = (xmin/matrix.A[0][0])-(matrix.b[0]/matrix.A[0][0]); 
	  max = (xmax/matrix.A[0][0])-(matrix.b[0]/matrix.A[0][0]);
	  /* minvec and maxvec are the indices corresponding to the 
	     bounding box around the sample point */ 
	  for (j=CEIL(min); j <= FLOOR(max) ; j++)
	    if (j>=0 && j<counts[0]) { 
	      /* here are the only ones which are in the bounding 
	       * box */ 
	      pos = j*matrix.A[0][0]+matrix.b[0]; 
	      distance = pos-xpos; 
	      if (distance <= radius) {
		DXSetElementValid(out_invalidhandle, j);
	      }
	    } 
	}
	break; 
      case (2): {
	Matrix2D tmpmat; 
	matrix2D.A[0][0] = matrix.A[0][0]; 
	matrix2D.A[1][0] = matrix.A[0][1]; 
	matrix2D.A[0][1] = matrix.A[0][2];
	matrix2D.A[1][1] = matrix.A[1][0]; 
	matrix2D.b[0] = matrix.b[0]; 
	matrix2D.b[1] = matrix.b[1];
	invmatrix2D = Invert2D(matrix2D);
	/* loop over all scattered points */
	for (i=0; i<numitemsino; i++) {  
	  xpos = pos_ino_ptr[2*i]; 
	  ypos = pos_ino_ptr[2*i+1];
	  xmin = xpos-radius; 
	  xmax = xpos+radius;
	  ymin = ypos-radius;
	  ymax = ypos+radius;
	  tmpmat = invmatrix2D;
	  minvec2D = Apply2D(Vec2D(xmin,ymin), tmpmat); 
	  maxvec2D = Apply2D(Vec2D(xmax,ymax), tmpmat);
	  /* minvec and maxvec are the indices corresponding to  
	     the bounding box around the sample point */ 
	  tmpmat = matrix2D; 
	  for (j=CEIL(minvec2D.x); j <= FLOOR(maxvec2D.x) ; j++) 
	    if (j>=0 && j<counts[0]) 
	      for (k=CEIL(minvec2D.y); k <= FLOOR(maxvec2D.y) ; k++) 
		if (k>=0 && k<counts[1]) {  
		  /* here are the only ones which are in the  
		     bounding box */  
		  posvec2D = Apply2D(Vec2D(j,k),tmpmat);  
		  distance = POW(posvec2D.x-xpos, 2) +  
		    POW(posvec2D.y-ypos, 2);  
		  distance = sqrt(distance); 
		  if (distance <= radius) { 
		    cntr = j*counts[1]+k; 
		    DXSetElementValid(out_invalidhandle, cntr);  
		  }
		} 
	}  
	break;
      }
      case (3): {
	Matrix tmpmat;  
	invmatrix = DXInvert(matrix); 
	/* loop over all scattered points */ 
	for (i=0; i<numitemsino; i++) {  
	  xpos = pos_ino_ptr[3*i];  
	  ypos = pos_ino_ptr[3*i+1];
	  zpos = pos_ino_ptr[3*i+2];
	  xmin = xpos-radius; 
	  xmax = xpos+radius;
	  ymin = ypos-radius; 
	  ymax = ypos+radius;
	  zmin = zpos-radius;
	  zmax = zpos+radius; 
	  tmpmat=invmatrix;  
	  minvec = DXApply(DXVec(xmin,ymin,zmin), tmpmat); 
	  maxvec = DXApply(DXVec(xmax,ymax,zmax), tmpmat);
	  /* minvec and maxvec are the indices corresponding to the */ 
	  /* bounding box around the sample point */ 
	  tmpmat=matrix; 
	  for (j=CEIL(minvec.x); j <= FLOOR(maxvec.x) ; j++) 
	    if (j>=0 && j<counts[0]) 
	      for (k=CEIL(minvec.y); k <= FLOOR(maxvec.y) ; k++) 
		if (k>=0 && k<counts[1]) 
		  for (l=CEIL(minvec.z); l <= FLOOR(maxvec.z) ; l++) 
		    if (l>=0 && l<counts[2]) {  
		      /* here are the only ones which are in the 
			 bounding box */ 
		      posvec = DXApply(DXVec(j,k,l),tmpmat); 
		      distance = POW(posvec.x-xpos, 2) +
			POW(posvec.y-ypos, 2) +
			  POW(posvec.z-zpos, 2);
		      distance = sqrt(distance); 
		      if (distance <= radius) { 
			cntr = j*counts[2]*counts[1] + 
			  k*counts[2] + l; 
			DXSetElementValid(out_invalidhandle, cntr); 
		      }
		    }
	}
	break;
      }
      default:
	DXSetError(ERROR_DATA_INVALID,"grid must be 1-, 2-, or 3-dimensional");
	goto error;
	break;
      }
    }
  }
  


  if (!missing) {
    if (!DXSaveInvalidComponent(base,out_invalidhandle))
      goto error;
  }
  
  DXFreeInvalidComponentHandle(out_invalidhandle);
  DXFreeInvalidComponentHandle(in_invalidhandle);
  DXFree((Pointer)weights_ptr);
  return OK;
  
 error:
  DXFreeInvalidComponentHandle(out_invalidhandle);
  DXFreeInvalidComponentHandle(in_invalidhandle);
  DXDelete((Object)newcomponent);
  DXFree((Pointer)weights_ptr);
  DXFree((Pointer)missingval_f);
  DXFree((Pointer)missingval_b);
  DXFree((Pointer)missingval_i);
  DXFree((Pointer)missingval_s);
  DXFree((Pointer)missingval_d);
  DXFree((Pointer)missingval_ub);
  DXFree((Pointer)missingval_us);
  DXFree((Pointer)missingval_ui);
  return ERROR;
  
}


static Error FillDataArray(Type type, Array oldcomponent,
                           Array newcomponent,
                           ubyte *flagbuf, float *distancebuf,
                           int numitemsbase, int numitemsino, int shape,
                           float exponent, Array missing, char *name)
{
  float weight, distance;
  int i, j, k;
  float *buffer=NULL;
  float  *d_in_f=NULL, *d_f=NULL, *missingval_f=NULL;
  int    *d_in_i=NULL, *d_i=NULL, *missingval_i=NULL;
  short  *d_in_s=NULL, *d_s=NULL, *missingval_s=NULL;
  double *d_in_d=NULL, *d_d=NULL, *missingval_d=NULL;
  byte   *d_in_b=NULL, *d_b=NULL, *missingval_b=NULL;
  uint   *d_in_ui=NULL, *d_ui=NULL, *missingval_ui=NULL;
  ushort *d_in_us=NULL, *d_us=NULL, *missingval_us=NULL;
  ubyte  *d_in_ub=NULL, *d_ub=NULL, *missingval_ub=NULL;


  buffer = (float *)DXAllocateLocal(shape*sizeof(float));
  if (!buffer) goto error;

  /* check that the missing component, if present, matches the component
     we're working on */
  if (missing)
  switch (type) {
    case (TYPE_FLOAT):
      CHECK_AND_ALLOCATE_MISSING(missing, DXGetItemSize(oldcomponent), float, 
                                 type, shape, missingval_f);
      break;
    case (TYPE_INT):
      CHECK_AND_ALLOCATE_MISSING(missing, DXGetItemSize(oldcomponent), int, 
                                 type, shape, missingval_i);
      break;
    case (TYPE_BYTE):
      CHECK_AND_ALLOCATE_MISSING(missing, DXGetItemSize(oldcomponent), byte, 
                                 type, shape, missingval_b);
      break;
    case (TYPE_DOUBLE):
      CHECK_AND_ALLOCATE_MISSING(missing, DXGetItemSize(oldcomponent), double, 
                                 type, shape, missingval_d);
      break;
    case (TYPE_SHORT):
      CHECK_AND_ALLOCATE_MISSING(missing, DXGetItemSize(oldcomponent), short, 
                                 type, shape, missingval_s);
      break;
    case (TYPE_UBYTE):
      CHECK_AND_ALLOCATE_MISSING(missing, DXGetItemSize(oldcomponent), ubyte, 
                                 type, shape, missingval_ub);
      break;
    case (TYPE_USHORT):
      CHECK_AND_ALLOCATE_MISSING(missing, DXGetItemSize(oldcomponent), ushort, 
                                 type, shape, missingval_us);
      break;
    case (TYPE_UINT):
      CHECK_AND_ALLOCATE_MISSING(missing, DXGetItemSize(oldcomponent), uint, 
                                 type, shape, missingval_ui);
      break;
    default:
      DXSetError(ERROR_DATA_INVALID,"unsupported data type");
      goto error;
  }


  switch (type) {
    case (TYPE_FLOAT):
      d_in_f = (float *)DXGetArrayData(oldcomponent);
      d_f = (float *)DXGetArrayData(newcomponent);
      break;
    case (TYPE_INT):
      d_in_i = (int *)DXGetArrayData(oldcomponent);
      d_i = (int *)DXGetArrayData(newcomponent);
      break;
    case (TYPE_SHORT):
      d_in_s = (short *)DXGetArrayData(oldcomponent);
      d_s = (short *)DXGetArrayData(newcomponent);
      break;
    case (TYPE_DOUBLE):
      d_in_d = (double *)DXGetArrayData(oldcomponent);
      d_d = (double *)DXGetArrayData(newcomponent);
      break;
    case (TYPE_BYTE):
      d_in_b = (byte *)DXGetArrayData(oldcomponent);
      d_b = (byte *)DXGetArrayData(newcomponent);
      break;
    case (TYPE_UBYTE):
      d_in_ub = (ubyte *)DXGetArrayData(oldcomponent);
      d_ub = (ubyte *)DXGetArrayData(newcomponent);
      break;
    case (TYPE_USHORT):
      d_in_us = (ushort *)DXGetArrayData(oldcomponent);
      d_us = (ushort *)DXGetArrayData(newcomponent);
      break;
    case (TYPE_UINT):
      d_in_ui = (uint *)DXGetArrayData(oldcomponent);
      d_ui = (uint *)DXGetArrayData(newcomponent);
      break;
    default:
      DXSetError(ERROR_DATA_INVALID,"unrecognized data type");
      goto error;
   }


  for (i=0; i<numitemsbase; i++) {
    weight = 0;
    for (k=0; k<shape; k++) {
       buffer[k] = 0.0;
    }
    for (j=0; j<numitemsino; j++) {
      if (flagbuf[i*numitemsino+j]) {
	distance = distancebuf[i*numitemsino+j];
	if (distance != 0) {
	  for (k=0; k<shape; k++) {
            switch (type) {
              case (TYPE_FLOAT):
	        buffer[k] += (float)d_in_f[j*shape+k]/POW(distance,exponent);
                break;
              case (TYPE_INT):
	        buffer[k] += (float)d_in_i[j*shape+k]/POW(distance,exponent);
                break;
              case (TYPE_SHORT):
	        buffer[k] += (float)d_in_s[j*shape+k]/POW(distance,exponent);
                break;
              case (TYPE_BYTE):
	        buffer[k] += (float)d_in_b[j*shape+k]/POW(distance,exponent);
                break;
              case (TYPE_DOUBLE):
	        buffer[k] += (float)d_in_d[j*shape+k]/POW(distance,exponent);
                break;
              case (TYPE_UBYTE):
	        buffer[k] += (float)d_in_ub[j*shape+k]/POW(distance,exponent);
                break;
              case (TYPE_USHORT):
	        buffer[k] += (float)d_in_us[j*shape+k]/POW(distance,exponent);
                break;
              case (TYPE_UINT):
	        buffer[k] += (float)d_in_ui[j*shape+k]/POW(distance,exponent);
                break;
              default:
                DXSetError(ERROR_DATA_INVALID,"unsupported data type");
                goto error;
            }
	  }
	  weight += 1/POW(distance,exponent); 
	} 
	else {
          /* if distance == 0, then simply use the data value */
	  for (k=0; k<shape; k++) {
            switch (type) {
              case (TYPE_FLOAT):
	        buffer[k] = (float)d_in_f[j];
                break;
              case (TYPE_INT):
	        buffer[k] = (float)d_in_i[j];
                break;
              case (TYPE_BYTE):
	        buffer[k] = (float)d_in_b[j];
                break;
              case (TYPE_DOUBLE):
	        buffer[k] = (float)d_in_d[j];
                break;
              case (TYPE_SHORT):
	        buffer[k] = (float)d_in_s[j];
                break;
              case (TYPE_UINT):
	        buffer[k] = (float)d_in_ui[j];
                break;
              case (TYPE_USHORT):
	        buffer[k] = (float)d_in_us[j];
                break;
              case (TYPE_UBYTE):
	        buffer[k] = (float)d_in_us[j];
                break;
              default:
                DXSetError(ERROR_DATA_INVALID,"unsupported data type");
                goto error;
            }
	  }
	  weight = 1;
	  continue;
	}
      }
    }
    if (weight > 0) {
      for (k=0; k<shape; k++) { 
        switch (type) {
          case (TYPE_FLOAT):
	   d_f[shape*i+k] = (float)(buffer[k]/weight);
           break;
          case (TYPE_DOUBLE):
	   d_d[shape*i+k] = (double)(buffer[k]/weight);
           break;
          case (TYPE_INT):
	   d_i[shape*i+k] = (int)(RND(buffer[k]/weight));
           break;
          case (TYPE_SHORT):
	   d_s[shape*i+k] = (short)(RND(buffer[k]/weight));
           break;
          case (TYPE_BYTE):
	   d_b[shape*i+k] = (byte)(RND(buffer[k]/weight));
           break;
          case (TYPE_UBYTE):
	   d_ub[shape*i+k] = (ubyte)(RND(buffer[k]/weight));
           break;
          case (TYPE_UINT):
	   d_ui[shape*i+k] = (uint)(RND(buffer[k]/weight));
           break;
          case (TYPE_USHORT):
	   d_us[shape*i+k] = (ushort)(RND(buffer[k]/weight));
           break;
          default:
           DXSetError(ERROR_DATA_INVALID,"unsupported data type");
           goto error;
        }
      }
    }
    else {
      if (missing)
       for (k=0; k<shape; k++) { 
         switch (type) {
           case (TYPE_FLOAT):
	     d_f[i*shape+k] = (float)missingval_f[k];
             break; 
           case (TYPE_INT):
	     d_i[i*shape+k] = (int)missingval_i[k];
             break;
           case (TYPE_BYTE):
	     d_b[i*shape+k] = (byte)missingval_b[k];
             break;
           case (TYPE_DOUBLE):
	     d_d[i*shape+k] = (double)missingval_d[k];
             break;
           case (TYPE_SHORT):
	     d_s[i*shape+k] = (short)missingval_s[k];
             break;
           case (TYPE_UBYTE):
	     d_ub[i*shape+k] = (ubyte)missingval_ub[k];
             break;
           case (TYPE_UINT):
	     d_ui[i*shape+k] = (uint)missingval_ui[k];
             break;
           case (TYPE_USHORT):
	     d_us[i*shape+k] = (ushort)missingval_us[k];
             break;
           default:
             DXSetError(ERROR_DATA_INVALID,"unsupported data type");
             goto error;
         }
       }
     else
       for (k=0; k<shape; k++) { 
         switch (type) {
           case (TYPE_FLOAT):
	     d_f[i*shape+k] = 0.0;
             break;
           case (TYPE_INT):
	     d_i[i*shape+k] = 0.0;
             break;
           case (TYPE_BYTE):
	     d_b[i*shape+k] = 0.0;
             break;
           case (TYPE_DOUBLE):
	     d_d[i*shape+k] = 0.0;
             break; 
           case (TYPE_SHORT):
	     d_s[i*shape+k] = 0.0;
             break;
           case (TYPE_UBYTE):
	     d_ub[i*shape+k] = 0.0;
             break;
           case (TYPE_UINT):
	     d_ui[i*shape+k] = 0.0;
             break;
           case (TYPE_USHORT):
	     d_us[i*shape+k] = 0.0;
             break;
           default:
             DXSetError(ERROR_DATA_INVALID,"unsupported data type");
             goto error;
         }
       }
    }
  }
  DXFree((Pointer)missingval_f);
  missingval_f=NULL;
  DXFree((Pointer)missingval_i);
  missingval_i=NULL;
  DXFree((Pointer)missingval_d);
  missingval_d=NULL;
  DXFree((Pointer)missingval_s);
  missingval_s=NULL;
  DXFree((Pointer)missingval_b);
  missingval_b=NULL;
  DXFree((Pointer)missingval_ub);
  missingval_ub=NULL;
  DXFree((Pointer)missingval_ui);
  missingval_ui=NULL;
  DXFree((Pointer)missingval_us);
  missingval_us=NULL;
  return OK;
 error:
  DXFree ((Pointer)buffer);
  DXFree((Pointer)missingval_f);
  DXFree((Pointer)missingval_i);
  DXFree((Pointer)missingval_d);
  DXFree((Pointer)missingval_s);
  DXFree((Pointer)missingval_b);
  DXFree((Pointer)missingval_ub);
  DXFree((Pointer)missingval_ui);
  DXFree((Pointer)missingval_us);
  return ERROR;
}






static Error FillBuffersIrreg(int numitemsbase, int numitemsino, 
                              ubyte *flagbuf, 
                              float *distancebuf,  float *pos_ino_ptr,  
                              float *pos_base_ptr, 
                              InvalidComponentHandle out_invalidhandle, 
                              InvalidComponentHandle in_invalidhandle, 
                              int *anyinvalid, int shape_base, float radius)
{ 
  int i, j, anyvalid;
  float distance=0;

  /* there's a special check in here for radius == -1 (infinity) */
  
  
  /* fill the buffers */
  if (shape_base == 1) {
    for (i=0; i<numitemsbase; i++) {
      anyvalid = 0;
      for (j=0; j<numitemsino;j++) {
        if (DXIsElementValid(in_invalidhandle, j)) {
	  if (radius != -1) {
	    distance = ABS(pos_ino_ptr[j]-pos_base_ptr[i]);   
	    if (distance <= radius) {
	      anyvalid = 1;
	      flagbuf[i*numitemsino + j]=1;
	      distancebuf[i*numitemsino + j]=distance;
	    }
	  }
	  else {
	    anyvalid = 1;
	    flagbuf[i*numitemsino + j]=1;
	    distancebuf[i*numitemsino + j]=distance;
	  }
	}
      }
      if (!anyvalid) {
	if (out_invalidhandle) 
	  DXSetElementInvalid(out_invalidhandle, i);
	*anyinvalid = 1;
      }
    }
  }

  else if (shape_base == 2) {
    for (i=0; i<numitemsbase; i++) {
      anyvalid = 0;
      for (j=0; j<numitemsino;j++) {
        if (DXIsElementValid(in_invalidhandle, j)) {
	  if (radius != -1) {
	    if (((pos_ino_ptr[2*j]-pos_base_ptr[2*i])<radius)&&
		((pos_ino_ptr[2*j+1]-pos_base_ptr[2*i+1])<radius)) {
	      distance = POW(pos_ino_ptr[2*j]-pos_base_ptr[2*i], 2) +   
		POW(pos_ino_ptr[2*j+1]-pos_base_ptr[2*i+1], 2);
	      distance = sqrt(distance);   
	      if (distance <= radius) {
		anyvalid = 1;
		flagbuf[i*numitemsino + j]=1;
		distancebuf[i*numitemsino + j]=distance;
	      }
	    }
	  }
	  else {
	    anyvalid = 1;
	    flagbuf[i*numitemsino + j]=1;
	    distancebuf[i*numitemsino + j]=distance;
	  }
	}
      }
      if (!anyvalid) {
	if (out_invalidhandle) 
	  DXSetElementInvalid(out_invalidhandle, i);
	*anyinvalid = 1;
      }
    }
  }
  
  
  else if (shape_base==3) {
    for (i=0; i<numitemsbase; i++) {
      anyvalid = 0;
      for (j=0; j<numitemsino;j++) {
        if (DXIsElementValid(in_invalidhandle, j)) {
	  if (radius != -1) {
	    if (((pos_ino_ptr[3*j]-pos_base_ptr[3*i])<radius)&&
		((pos_ino_ptr[3*j+1]-pos_base_ptr[3*i+1])<radius) &&
		((pos_ino_ptr[3*j+2]-pos_base_ptr[3*i+2])<radius)) {
	      distance = POW(pos_ino_ptr[3*j]-pos_base_ptr[3*i], 2) +   
		POW(pos_ino_ptr[3*j+1]-pos_base_ptr[3*i+1], 2) +
		  POW(pos_ino_ptr[3*j+2]-pos_base_ptr[3*i+2], 2);
	      distance = sqrt(distance);   
	      if (distance <= radius) {
		anyvalid = 1;
		flagbuf[i*numitemsino + j]=1;
		distancebuf[i*numitemsino + j]=distance;
	      }
	    }
	  }
	  else {
	    anyvalid = 1;
	    flagbuf[i*numitemsino + j]=1;
	    distancebuf[i*numitemsino + j]=distance;
	  }
	}
      }
      if (!anyvalid) {
	if (out_invalidhandle) 
	  DXSetElementInvalid(out_invalidhandle, 1); 
	*anyinvalid = 1;
      }
    }
  }
  else {
    DXSetError(ERROR_BAD_PARAMETER,"only 1D 2D 3D positions accepted");
    goto error;
  }
  return OK;
 error:
  return ERROR;
}  


/*Added by Jeff Braun to put scattered data on closest grid point*/

Error _dxfConnectScatterObject(Object ino, Object base, Array missing) 
{
  struct arg_scatter arg;
  Object subbase;
  int i;
  
  switch (DXGetObjectClass(base)) {
  case CLASS_FIELD:
    /* create the task group */
    arg.ino = ino;
    arg.base = (Field)base;
    arg.missing = missing;
    if (!DXAddTask(ConnectScatterField,(Pointer)&arg, sizeof(arg),0.0))
      goto error;
    break;
  case CLASS_GROUP:
    for (i=0; (subbase=DXGetEnumeratedMember((Group)base, i, NULL)); i++) {
      if (!(_dxfConnectScatterObject((Object)ino, (Object)subbase, missing))) 
	goto error;
    }
    break;
  default:
    DXSetError(ERROR_DATA_INVALID,"base must be a field or a group");
    goto error; 
  }
  return OK;
  
 error:
  return ERROR;  
  
}

static Error ConnectScatterField(Pointer p)
{
  struct arg_scatter *arg = (struct arg_scatter *)p;
  Object ino, newino=NULL;
  Field base;
  Array missing;

  ino = arg->ino;
  newino = DXApplyTransform(ino,NULL);

  switch (DXGetObjectClass(newino)) {

    case CLASS_FIELD:
       base = arg->base;
       missing = arg->missing;
       if (!ConnectScatter((Field)newino, base, missing))
         goto error;
      break;

    case CLASS_GROUP:
      DXSetError(ERROR_NOT_IMPLEMENTED,"input cannot be a group");
      goto error;
      break;

    default:
      DXSetError(ERROR_DATA_INVALID,"input must be a field");
      goto error;
  }
  DXDelete((Object)newino);
  return OK;

error:
  DXDelete((Object)newino);
  return ERROR;

}

static Error ConnectScatter(Field ino, Field base, Array missing)
{
  Matrix matrix, inverse;
  Matrix2D matrix2D, inverse2D;
  Type type;
  Category category;
  Array positions_base, positions_ino, oldcomponent=NULL, newcomponent;
  InvalidComponentHandle out_invalidhandle=NULL, in_invalidhandle=NULL;

  int rank, shape[8];
  int counts[8], rank_base, shape_base[8], shape_ino[8], rank_ino;
  int nitemsbase, nitemsino, nitems, compcount, componentsdone;
  float *ino_positions;
  char *name; 

  int i,j,k, ijk[3], index, *count=NULL;
  float xyz[3], sum, eps=.000001;

 
  float    *old_data_f=NULL, *new_data_ptr_f=NULL, *missingval_f=NULL;
  int      *old_data_i=NULL, *new_data_ptr_i=NULL, *missingval_i=NULL;
  short    *old_data_s=NULL, *new_data_ptr_s=NULL, *missingval_s=NULL;
  double   *old_data_d=NULL, *new_data_ptr_d=NULL, *missingval_d=NULL;
  byte     *old_data_b=NULL, *new_data_ptr_b=NULL, *missingval_b=NULL;
  ubyte    *old_data_ub=NULL, *new_data_ptr_ub=NULL, *missingval_ub=NULL;
  ushort   *old_data_us=NULL, *new_data_ptr_us=NULL, *missingval_us=NULL;
  uint     *old_data_ui=NULL, *new_data_ptr_ui=NULL, *missingval_ui=NULL;

  if (DXEmptyField((Field)ino))
     return OK;

  positions_base = (Array)DXGetComponentValue((Field)base, "positions");
  if (!positions_base) {
    DXSetError(ERROR_MISSING_DATA,"base field has no positions");
    goto error;
  }
  positions_ino = (Array)DXGetComponentValue((Field)ino, "positions");
  if (!positions_ino) {
    DXSetError(ERROR_MISSING_DATA,"input field has no positions");
    goto error;
  }

 /*Grid Position Data*/
  DXGetArrayInfo(positions_base, &nitemsbase, &type, &category,
                 &rank_base, shape_base);
  if ((DXQueryGridPositions((Array)positions_base,NULL,counts,
                            (float *)&matrix.b,(float *)&matrix.A) )) {
    if (shape_base[0]==2) {
      matrix2D.A[0][0] = matrix.A[0][0];
      matrix2D.A[1][0] = matrix.A[0][1];
      matrix2D.A[0][1] = matrix.A[0][2];
      matrix2D.A[1][1] = matrix.A[1][0];
      matrix2D.b[0] = matrix.b[0];
      matrix2D.b[1] = matrix.b[1];
      inverse2D=Invert2D(matrix2D);
      inverse.A[0][0]=inverse2D.A[0][0];
      inverse.A[0][1]=inverse2D.A[0][1];
      inverse.A[1][0]=inverse2D.A[1][0];
      inverse.A[1][1]=inverse2D.A[1][1];
    }
    else if (shape_base[0]==1) {
      /*origin = matrix.b[0];*/
      /*delta = matrix.A[0][0];*/
      DXSetError(ERROR_DATA_INVALID, "Only 2D and 3D grids supported");
      goto error;
    }
  }
  else {
    /* irregular positions */
    DXSetError(ERROR_DATA_INVALID, "irregular grids not currently supported for radius=0");
    goto error;
  }

  if ((type != TYPE_FLOAT)||(category != CATEGORY_REAL)) {
    DXSetError(ERROR_DATA_INVALID,
               "positions component must be type float, category real");
    goto error;
  }

 /*Scatter Position Data*/
  DXGetArrayInfo(positions_ino, &nitemsino, &type, &category, &rank_ino,
                 shape_ino);

  if ((type != TYPE_FLOAT)||(category != CATEGORY_REAL)) {
    DXSetError(ERROR_DATA_INVALID,
               "positions component must be type float, category real");
    goto error;
  }

  if (rank_base == 0)
    shape_base[0] = 1;
  if (rank_base > 1) {
    DXSetError(ERROR_DATA_INVALID,
               "rank of base positions cannot be greater than 1");
    goto error;
  }
  if (rank_ino == 0)
    shape_ino[0] = 1;
  if (rank_ino > 1) {
    DXSetError(ERROR_DATA_INVALID,
               "rank of input positions cannot be greater than 1");
    goto error;
  }

  if ((shape_base[0]<0)||(shape_base[0]>3)) {
    DXSetError(ERROR_DATA_INVALID,
               "only 1D, 2D, and 3D positions for base supported");
    goto error;
  }

  if (shape_base[0] != shape_ino[0]) {
    DXSetError(ERROR_DATA_INVALID,
               "dimensionality of base (%dD) does not match dimensionality of input (%dD)",
               shape_base[0], shape_ino[0]);
    goto error;
  }

  /* first set the extra counts to one to simplify the code for handling
     1, 2, and 3 dimensions */

  if (shape_base[0]==1) {
    counts[1]=1;
    counts[2]=1;
  }
  else if (shape_base[0]==2) {
    counts[2]=1;
  }

  /*Will mark as invalid if 'missing' not specified*/

  if (!missing){
    out_invalidhandle = DXCreateInvalidComponentHandle((Object)base, NULL,
                                                     "positions");
    if (!out_invalidhandle)
      goto error;
    DXSetAllInvalid(out_invalidhandle);
  }
  in_invalidhandle = DXCreateInvalidComponentHandle((Object)ino, NULL,
                                                     "positions");
  if (!in_invalidhandle)
     return ERROR;

  /* need to get the components we are going to copy into the new field */
  /* copied portions from ConnectNearest -JAB */
  compcount = 0;
  componentsdone = 0;
  while (NULL !=
         (oldcomponent=(Array)DXGetEnumeratedComponentValue(ino, compcount,
                                                            &name))) {
    Object attr;

    if (DXGetObjectClass((Object)oldcomponent) != CLASS_ARRAY)
      goto component_done;
   
    if (!strcmp(name,"positions") ||
        !strcmp(name,"connections") ||
        !strcmp(name,"invalid positions"))
      goto component_done;
   
    /* if the component refs anything, leave it */
    if (DXGetComponentAttribute((Field)ino,name,"ref"))
      goto component_done;

    attr = DXGetComponentAttribute((Field)ino,name,"der");
    if (attr)
       goto component_done;

    attr = DXGetComponentAttribute((Field)ino,name,"dep");
    if (!attr) {
      /* does it der? if so, throw it away */
      attr = DXGetComponentAttribute((Field)ino,name,"der");
      if (attr)
        goto component_done;
      /* it doesn't ref, doesn't dep and doesn't der.
         Copy it to the output and quit */
      if (!DXSetComponentValue((Field)base,name,(Object)oldcomponent))
         goto error;
      goto component_done;
    }

    if (DXGetObjectClass(attr) != CLASS_STRING) {
      DXSetError(ERROR_DATA_INVALID, "dependency attribute not type string");
      goto error;
    }

    if (strcmp(DXGetString((String)attr),"positions")){
      DXWarning("Component '%s' is not dependent on positions!",name); 
      DXWarning("Regrid will remove '%s' and output the base grid 'data' component if it exists",name);
      goto component_done; 
    }


    /* if the component happens to be "invalid positions" ignore it */
    if (!strcmp(name,"invalid positions"))
      goto component_done;


    componentsdone++;

    DXGetArrayInfo((Array)oldcomponent,&nitems, &type,
                   &category, &rank, shape);
    if (rank==0) shape[0]=1;

    /* check that the missing component, if present, matches the component
       we're working on. If missing not present, 0 is used as default */
      switch (type) {
      case (TYPE_FLOAT):
        CHECK_AND_ALLOCATE_MISSING(missing, DXGetItemSize(oldcomponent),
                                   float, type, shape[0], missingval_f);
        break;
      case (TYPE_INT):
        CHECK_AND_ALLOCATE_MISSING(missing, DXGetItemSize(oldcomponent),
                                   int, type, shape[0], missingval_i);
        break;
      case (TYPE_DOUBLE):
        CHECK_AND_ALLOCATE_MISSING(missing, DXGetItemSize(oldcomponent),
                                   double, type, shape[0], missingval_d);
        break;
      case (TYPE_SHORT):
        CHECK_AND_ALLOCATE_MISSING(missing, DXGetItemSize(oldcomponent),
                                   short, type, shape[0], missingval_s);
        break;
      case (TYPE_BYTE):
        CHECK_AND_ALLOCATE_MISSING(missing, DXGetItemSize(oldcomponent),
                                   byte, type, shape[0], missingval_b);
        break;
      case (TYPE_UINT):
        CHECK_AND_ALLOCATE_MISSING(missing, DXGetItemSize(oldcomponent),
                                   uint, type, shape[0], missingval_ui);
        break;
      case (TYPE_USHORT):
        CHECK_AND_ALLOCATE_MISSING(missing, DXGetItemSize(oldcomponent),
                                   ushort, type, shape[0], missingval_us);
        break;
      case (TYPE_UBYTE):
        CHECK_AND_ALLOCATE_MISSING(missing, DXGetItemSize(oldcomponent),
                                   ubyte, type, shape[0], missingval_ub);
        break;
      default:
        DXSetError(ERROR_DATA_INVALID,"unsupported data type");
        goto error;
      }

    if (rank > 1) {
      DXSetError(ERROR_NOT_IMPLEMENTED,"rank larger than 1 not implemented");
      goto error;
    }
    if (nitems != nitemsino) {
      DXSetError(ERROR_BAD_PARAMETER,
                 "number of items in %s component not equal to number of positions",
                 name);
      goto error;
    }


   /*Intialize newcomponent with missing value*/
    newcomponent = DXNewArrayV(type, category, rank, shape);
    if (!DXAddArrayData(newcomponent, 0, nitemsbase, NULL))
      goto error;
   
    switch (type) {
    case (TYPE_FLOAT):
      old_data_f = (float *)DXGetArrayData(oldcomponent);
      new_data_ptr_f = (float *)DXGetArrayData(newcomponent);
      for(i=0;i<nitemsbase;i++){
        for(j=0;j<shape[0];j++){
          new_data_ptr_f[i*shape[0]+j] = missingval_f[j];
        }
      }
      break;
    case (TYPE_INT):
      old_data_i = (int *)DXGetArrayData(oldcomponent);
      new_data_ptr_i = (int *)DXGetArrayData(newcomponent);
      for(i=0;i<nitemsbase;i++){
        for(j=0;j<shape[0];j++){
          new_data_ptr_i[i*shape[0]+j] = missingval_i[j];
        }
      }
      break;
    case (TYPE_SHORT):
      old_data_s = (short *)DXGetArrayData(oldcomponent);
      new_data_ptr_s = (short *)DXGetArrayData(newcomponent);
      for(i=0;i<nitemsbase;i++){
        for(j=0;j<shape[0];j++){
          new_data_ptr_s[i*shape[0]+j] = missingval_s[j];
        }
      }
      break;
    case (TYPE_BYTE):
      old_data_b = (byte *)DXGetArrayData(oldcomponent);
      new_data_ptr_b = (byte *)DXGetArrayData(newcomponent);
      for(i=0;i<nitemsbase;i++){
        for(j=0;j<shape[0];j++){
          new_data_ptr_b[i*shape[0]+j] = missingval_b[j];
        }
      }
      break;
    case (TYPE_DOUBLE):
      old_data_d = (double *)DXGetArrayData(oldcomponent);
      new_data_ptr_d = (double *)DXGetArrayData(newcomponent);
      for(i=0;i<nitemsbase;i++){
        for(j=0;j<shape[0];j++){
          new_data_ptr_d[i*shape[0]+j] = missingval_d[j];
        }
      }
      break;
    case (TYPE_UINT):
      old_data_ui = (uint *)DXGetArrayData(oldcomponent);
      new_data_ptr_ui = (uint *)DXGetArrayData(newcomponent);
      for(i=0;i<nitemsbase;i++){
        for(j=0;j<shape[0];j++){
          new_data_ptr_ui[i*shape[0]+j] = missingval_ui[j];
        }
      }
      break;
    case (TYPE_UBYTE):
      old_data_ub = (ubyte *)DXGetArrayData(oldcomponent);
      new_data_ptr_ub = (ubyte *)DXGetArrayData(newcomponent);
      for(i=0;i<nitemsbase;i++){
        for(j=0;j<shape[0];j++){
          new_data_ptr_ub[i*shape[0]+j] = missingval_ub[j];
        }
      }
      break;
    case (TYPE_USHORT):
      old_data_us = (ushort *)DXGetArrayData(oldcomponent);
      new_data_ptr_us = (ushort *)DXGetArrayData(newcomponent);
      for(i=0;i<nitemsbase;i++){
        for(j=0;j<shape[0];j++){
          new_data_ptr_us[i*shape[0]+j] = missingval_us[j];
        }
      }
      break;
    default:
      DXSetError(ERROR_DATA_INVALID,"unrecognized data type");
      goto error;
    }


    /*Need to allocate count and compute inverse of grid matrix*/
    count = (int *) DXAllocateLocalZero(sizeof(int)*nitemsbase);
    if (! count)
      goto error;
    if(shape_base[0]==3){
      inverse = DXInvert(matrix);
    }

    ino_positions = (float *)DXGetArrayData(positions_ino);

    /*Put Scattered data into grid data array*/
    for(i=0;i<nitemsino;i++){  /*loops through scatter data */
     if(DXIsElementValid(in_invalidhandle,i)){ /*seems to work fine without this*/
      index=0;
      for(j=0;j<shape_base[0];j++){
        xyz[j]=ino_positions[shape_base[0]*i+j]-matrix.b[j];
      }
      for(j=0;j<shape_base[0];j++){
        sum=0;
        for(k=0;k<shape_base[0];k++){
          sum=sum+inverse.A[k][j]*xyz[k];
        }
        ijk[j]=rint(sum + eps);
        if(ijk[j]<0 || ijk[j]>=counts[j]){
          index=-1;
        } 
      }

      /* Finds data array index number for grid position*/
      if(index !=-1){
        for(j=0;j<shape_base[0];j++){
          for(k=shape_base[0]-1;k>j;k--){
            ijk[j]=ijk[j]*counts[k];
          }
          index=index+ijk[j];
        }
      }


      if(index >= 0 && index <nitemsbase){
        if(!missing){
          if(!DXSetElementValid(out_invalidhandle, index)) goto error;
        }
        switch (type){
        case(TYPE_FLOAT):
          for(j=0;j<shape[0];j++){
            if(count[index] == 0)
              new_data_ptr_f[shape[0]*index+j]=old_data_f[shape[0]*i+j];
            else
              new_data_ptr_f[shape[0]*index+j]=
                        (new_data_ptr_f[shape[0]*index+j]*count[index]
                         +old_data_f[shape[0]*i+j])/(count[index]+1);
          }
          break;
        case (TYPE_INT):
          for(j=0;j<shape[0];j++){
            if(count[index] == 0)
              new_data_ptr_i[shape[0]*index+j]=old_data_i[shape[0]*i+j];
            else
              new_data_ptr_i[shape[0]*index+j]=
                        (new_data_ptr_i[shape[0]*index+j]*count[index]
                         +old_data_i[shape[0]*i+j])/(count[index]+1);
          }
          break;
        case (TYPE_SHORT):
          for(j=0;j<shape[0];j++){
            if(count[index] == 0)
              new_data_ptr_s[shape[0]*index+j]=old_data_s[shape[0]*i+j];
            else
              new_data_ptr_s[shape[0]*index+j]=
                        (new_data_ptr_s[shape[0]*index+j]*count[index]
                         +old_data_s[shape[0]*i+j])/(count[index]+1);
          }
          break;
        case (TYPE_DOUBLE):
          for(j=0;j<shape[0];j++){
            if(count[index] == 0)
              new_data_ptr_d[shape[0]*index+j]=old_data_d[shape[0]*i+j];
            else
              new_data_ptr_d[shape[0]*index+j]=
                        (new_data_ptr_d[shape[0]*index+j]*count[index]
                         +old_data_d[shape[0]*i+j])/(count[index]+1);
          }
          break;
        case (TYPE_BYTE):
          for(j=0;j<shape[0];j++){
            if(count[index] == 0)
              new_data_ptr_b[shape[0]*index+j]=old_data_b[shape[0]*i+j];
            else
              new_data_ptr_b[shape[0]*index+j]=
                        (new_data_ptr_b[shape[0]*index+j]*count[index]
                         +old_data_b[shape[0]*i+j])/(count[index]+1);
          }
          break;
        case (TYPE_UINT):
          for(j=0;j<shape[0];j++){
            if(count[index] == 0)
              new_data_ptr_ui[shape[0]*index+j]=old_data_ui[shape[0]*i+j];
            else
              new_data_ptr_ui[shape[0]*index+j]=
                        (new_data_ptr_ui[shape[0]*index+j]*count[index]
                         +old_data_ui[shape[0]*i+j])/(count[index]+1);
          }
          break;
        case (TYPE_USHORT):
          for(j=0;j<shape[0];j++){
            if(count[index] == 0)
              new_data_ptr_us[shape[0]*index+j]=old_data_us[shape[0]*i+j];
            else
              new_data_ptr_us[shape[0]*index+j]=
                        (new_data_ptr_us[shape[0]*index+j]*count[index]
                         +old_data_us[shape[0]*i+j])/(count[index]+1);
          }
          break;
        case (TYPE_UBYTE):
          for(j=0;j<shape[0];j++){
            if(count[index] == 0)
              new_data_ptr_ub[shape[0]*index+j]=old_data_ub[shape[0]*i+j];
            else
              new_data_ptr_ub[shape[0]*index+j]=
                        (new_data_ptr_ub[shape[0]*index+j]*count[index]
                         +old_data_ub[shape[0]*i+j])/(count[index]+1);
          }
          break;
        default:
          DXSetError(ERROR_DATA_INVALID,"unrecognized data type");
          goto error;
        }
        count[index]++;
      }
     }
    }
    if (!DXChangedComponentValues(base,name))
      goto error;
    if (!DXSetComponentValue(base,name,(Object)newcomponent))
      goto error;
    newcomponent = NULL;
    if (!DXSetComponentAttribute((Field)base, name, "dep", 
				 (Object)DXNewString("positions"))) 
      goto error;

  component_done:
    compcount++;
    DXFree((Pointer)missingval_f);
    missingval_f=NULL;
    DXFree((Pointer)missingval_s);
    missingval_s=NULL;
    DXFree((Pointer)missingval_i);
    missingval_i=NULL;
    DXFree((Pointer)missingval_d);
    missingval_d=NULL;
    DXFree((Pointer)missingval_b);
    missingval_b=NULL;
    DXFree((Pointer)missingval_ui);
    missingval_ui=NULL;
    DXFree((Pointer)missingval_ub);
    missingval_ub=NULL;
    DXFree((Pointer)missingval_us);
    missingval_us=NULL;
    DXFree((Pointer)count);
    count=NULL;
  }

  if(!missing){
    for(i=0;i<nitemsbase;i++){
      if (DXIsElementInvalid(out_invalidhandle,i)) {
        i=nitemsbase;
        if(!DXSaveInvalidComponent(base, out_invalidhandle)) goto error;
      }
    }
  }
  DXFreeInvalidComponentHandle(out_invalidhandle);
  DXFreeInvalidComponentHandle(in_invalidhandle);
  if(!DXEndField(base)) goto error;
  return OK;

error:
    DXFreeInvalidComponentHandle(out_invalidhandle);
    DXFreeInvalidComponentHandle(in_invalidhandle);
    DXFree((Pointer)missingval_f);
    DXFree((Pointer)missingval_s);
    DXFree((Pointer)missingval_i);
    DXFree((Pointer)missingval_d);
    DXFree((Pointer)missingval_b);
    DXFree((Pointer)missingval_ui);
    DXFree((Pointer)missingval_ub);
    DXFree((Pointer)missingval_us);
    DXFree((Pointer)count);
  return ERROR;

}
