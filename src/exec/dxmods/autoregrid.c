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



/* issues: partitioned grids for base, handling 2D and 3D */
/* should do not only data but all components dep positions */
/* what about data dep connections? need to do a set type after */
/* the work is done */

#include <stdio.h>
#include <math.h>

#if defined(HAVE_STRING_H)
#include <string.h>
#endif 

#include <dx/dx.h>
#include "_glyph.h"
#include "_connectgrids.h"

static
  Object MakeGrid(Object, float *, Object);

#define RND(x) (x<0? x-0.5 : x+0.5)

int
  m_AutoGrid(Object *in, Object *out)
{
  char *string;
  Class class;
  Category category;
  Object ino=NULL, base=NULL;
  float one=1;
  Array missing=NULL, density;
  float radius,  *radius_ptr, exponent;
  Type type;
  int numitems, rank, shape[30], numnearest;
  float makeradius;

  radius_ptr = &radius;
  
  if (!in[0]) {
    DXSetError(ERROR_BAD_PARAMETER, "#10000","input");
    return ERROR;
  }
  
  class = DXGetObjectClass(in[0]);
  if ((class != CLASS_ARRAY)&&
      (class != CLASS_FIELD)&&
      (class != CLASS_GROUP)&&
      (class != CLASS_XFORM)) {
    DXSetError(ERROR_BAD_PARAMETER, "#10630", "input");
    return ERROR;
  }

  /* density factor */
  if (!in[1]) {
    /* make a grid based on the data file */ 
    density = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 0);
    if (!DXAddArrayData(density, 0, 1, &one))
       goto error;
    base = MakeGrid(in[0], &makeradius, (Object)density);
    DXDelete((Object)density);
    if (!base) return ERROR;  
    if (DXEmptyField((Field)base)) {
      out[0] = base;
      return OK;
    }
  }
  else {
       /* make a grid based on the data file */ 
       base = MakeGrid(in[0], &makeradius, in[1]);
       if (!base) goto error;  
       if (DXEmptyField((Field)base)) {
         out[0] = base;
         return OK;
       }
  }
     

  /* now get numnearest, radius, missing, exponent */

  /* numnearest */
  if (!in[2]) {
     numnearest = 1;
  }
  else {
     if (!DXExtractInteger(in[2], &numnearest)) {
        if (!DXExtractString(in[2], &string)) {
           DXSetError(ERROR_BAD_PARAMETER, "#10370",
                      "nearest", "an integer or the string `infinity`");
           goto error;
        }
        /* XXX lower case */
        if (strcmp(string,"infinity")) {
           DXSetError(ERROR_BAD_PARAMETER, "#10370",
                      "nearest", "an integer or the string `infinity`");
           goto error;
        }
        numnearest = -1;
     }
     else {
        if (numnearest < 0) {
           DXSetError(ERROR_BAD_PARAMETER,"#10020","nearest");
           goto error; 
        }
     }
  }
 

  /* radius */
  if (!in[3]) {
     *radius_ptr = makeradius; 
  }
  else {
     if (!DXExtractFloat(in[3], radius_ptr)) {
        if (!DXExtractString(in[3], &string)) {
           DXSetError(ERROR_BAD_PARAMETER, "#10370",
                "radius", "a positive scalar value or the string `infinity`");
           goto error;
        }
        /* XXX lower case */
        if (strcmp(string,"infinity")) {
           DXSetError(ERROR_BAD_PARAMETER, "#10370",
                "radius", "a positive scalar value or the string `infinity`");
           goto error;
        }
        *radius_ptr = -1;
     }
     else {
        if (*radius_ptr < 0) {
           DXSetError(ERROR_BAD_PARAMETER, "#10370",
                "radius", "a positive scalar value or the string `infinity`");
           goto error; 
        }
        else if(*radius_ptr == 0) {
           DXWarning("Regrid radius set to 0, data values assigned to nearest grid point");
           numnearest=0;
        }
     }
  }



  /* now get exponent */
  if (!in[4]) {
     exponent = 2;
  }
  else {
     if (!DXExtractFloat(in[4], &exponent)) {
        DXSetError(ERROR_BAD_PARAMETER,"#10080",
                   "exponent");
        goto error;
     }
  }

  /* get missing */
  if (in[5]) {
      if (!(DXGetObjectClass(in[5])==CLASS_ARRAY)) {
	DXSetError(ERROR_BAD_PARAMETER, "#10261",
		   "missing");
	goto error;
      }
      missing = (Array)in[5];
  }


  /* if nearest is infinity, we use the "radius" method. In all other cases
     we use the "nearest" method" */

  if (numnearest == -1) {
    /* use the "radius" method */
    class = DXGetObjectClass(in[0]);
    if (class == CLASS_ARRAY) {
      if (!DXGetArrayInfo((Array)in[0], &numitems, &type, &category, 
			  &rank, shape))
        goto error;
      if ((type != TYPE_FLOAT)||(rank != 1)) {
        /* should also handle scalar I guess XXX */
        DXSetError(ERROR_BAD_PARAMETER,"#10630","input");
        goto error;
      }
      if (shape[0]<1 || shape[0]> 3) {
        DXSetError(ERROR_BAD_PARAMETER,"#10370",
                   "input","1-, 2-, or 3-dimensional");
        goto error;
      }
      /* just a list of positions; let's make it a field with positions*/
      ino = (Object)DXNewField();
      if (!ino)
        goto error;
      if (!DXSetComponentValue((Field)ino,"positions",in[0]))
        goto error; 
    }
    else {
      /* so I can safely delete it at the bottom, as well as modify it */
      ino = DXCopy(in[0], COPY_STRUCTURE);
    }

    /* remove connections */
    if (DXExists(ino, "connections"))
     DXRemove(ino,"connections");
   
    /* cull */
    ino = DXCull(ino);

    /* copy the attributes of the input scattered points to the output grid*/
    if (!DXCopyAttributes(base, ino)) 
      goto error; 

    if (!DXCreateTaskGroup())
      goto error;

    if (!_dxfConnectRadiusObject((Object)ino, (Object)base, 
				 radius, exponent, missing))
      goto error;

    if (!DXExecuteTaskGroup())
      goto error;

    DXDelete((Object)ino);
    out[0] = base;
  }
  else if (numnearest == 0) {
    /* use the assign to nearest point method */
    class = DXGetObjectClass(in[0]);
    if (class == CLASS_ARRAY) {
      if (!DXGetArrayInfo((Array)in[0], &numitems, &type, &category, 
			  &rank, shape))
        goto error;
      if ((type != TYPE_FLOAT)||(rank != 1)) {
        /* should also handle scalar I guess XXX */
        DXSetError(ERROR_BAD_PARAMETER,"#10630","input");
        goto error;
      }
      if (shape[0]<1 || shape[0]> 3) {
        DXSetError(ERROR_BAD_PARAMETER,"#10370",
                   "input","1-, 2-, or 3-dimensional");
        goto error;
      }
      /* just a list of positions; let's make it a field with positions*/
      ino = (Object)DXNewField();
      if (!ino)
        goto error;
      if (!DXSetComponentValue((Field)ino,"positions",in[0]))
        goto error; 
    }
    else {
      /* so I can safely delete it at the bottom, as well as modify it */
      ino = DXCopy(in[0], COPY_STRUCTURE);
    }

    /* remove connections */
    if (DXExists(ino, "connections"))
     DXRemove(ino,"connections");
   
    /* cull */
    ino = DXCull(ino);

    /* copy the attributes of the input scattered points to the output grid*/
    if (!DXCopyAttributes(base, ino)) 
      goto error; 

    if (!DXCreateTaskGroup())
      goto error;

    if (!_dxfConnectScatterObject((Object)ino, (Object)base, missing))
      goto error;

    if (!DXExecuteTaskGroup())
      goto error;

    DXDelete((Object)ino);
    out[0] = base;  
  }
  else {
    /* use the "nearest" method */
    class = DXGetObjectClass(in[0]);
    if (class == CLASS_ARRAY) {
      if (!DXGetArrayInfo((Array)in[0], &numitems, &type, &category, 
			  &rank, shape))
        goto error;
      if ((type != TYPE_FLOAT)||(rank != 1)) {
        /* should also handle scalar I guess XXX */
        DXSetError(ERROR_BAD_PARAMETER,"#10630","input");
        goto error;
      }
      if (shape[0]<1 || shape[0]> 3) {
        DXSetError(ERROR_BAD_PARAMETER,"#10370","input", 
                   "1-, 2-, or 3-dimensional");
        goto error;
      }
      /* just a list of positions; let's make it a field with positions*/
      ino = (Object)DXNewField();
      if (!ino)
        goto error;
      if (!DXSetComponentValue((Field)ino,"positions",in[0]))
        goto error; 
    }
    else {
      /* so I can safely delete it at the bottom, as well as modify it */
      ino = DXCopy(in[0],COPY_STRUCTURE);
    }

    /* remove connections */
    if (DXExists(ino, "connections"))
     DXRemove(ino,"connections");
   
    /* cull */
    ino = DXCull(ino);

    /* copy the attributes of the input scattered points to the output grid*/
    if (!DXCopyAttributes(base, ino)) 
      goto error; 
   
    if (!DXCreateTaskGroup())
       goto error;

    /* if radius was infinity... */
    if (*radius_ptr==-1)
       radius_ptr = NULL;
    if (!_dxfConnectNearestObject(ino, base, numnearest, radius_ptr, 
			          exponent, missing))
      goto error;


    if (!DXExecuteTaskGroup())
      goto error;

    DXDelete((Object)ino);
    out[0] = base;
  }
 
  /* put the radius used on as an attribute */
  if (radius_ptr == NULL ) {
    if (!DXSetStringAttribute(out[0], "AutoGrid radius", 
                              "infinity"))
        goto error;
  }
  else {
    if (!DXSetFloatAttribute(out[0], "AutoGrid radius", 
                             *radius_ptr))
        goto error;
  } 
  return OK;
  
 error:
  DXDelete((Object)ino);
  DXDelete((Object)base);
  return ERROR;
  
}

static Object MakeGrid(Object object, float *radius, Object density)
{

   Point box[8], origin, densvector;
   int numitems, rank, shape[10];
   float DX, DY, DZ;
   float dx, dy, dz;
   float densval;
   float nx_float, ny_float, nz_float;
   int nx, ny, nz;
   Class class;
   Field field, grid;
   Array positions, gridpositions, gridconnections;

   class = DXGetObjectClass(object);
   if (!class) goto error;

   switch (class) {
   case (CLASS_FIELD):
      if (DXEmptyField((Field)object))
         return object; 
      if (!DXBoundingBox((Object)object, box))
         goto error;
      positions = (Array)DXGetComponentValue((Field)object,"positions");
      if (!positions) {
         DXSetError(ERROR_MISSING_DATA,"missing positions component");
         goto error;
      }
      if (!DXGetArrayInfo(positions,&numitems, NULL, NULL, &rank, shape))
         goto error;
      break;
   case (CLASS_XFORM):
   case (CLASS_GROUP):
      field = DXGetPart(object,0);
      if (!field) goto error; 
      if (!DXBoundingBox((Object)field, box))
         goto error;
      positions = (Array)DXGetComponentValue((Field)field,"positions");
      if (!positions) {
         DXSetError(ERROR_MISSING_DATA,"missing positions component");
         goto error;
      }
      if (!DXGetArrayInfo(positions,&numitems, NULL, NULL, &rank, shape))
         goto error;
      break;
   case (CLASS_ARRAY):
      if (!DXGetArrayInfo((Array)object,&numitems, NULL, NULL, &rank, shape))
         goto error;
      break;
   default: 
      break;
   }

   /* now get the density */
   class = DXGetObjectClass(density);
   if (!class) goto error;

   if (class != CLASS_ARRAY) {
      DXSetError(ERROR_DATA_INVALID,
                 "densityfactor must be a scalar or vector");
      goto error;
   }
   

   /* 1D positions */
   if ((rank==0)||(rank==1 && shape[0] ==1)) {
      origin = box[0];
      dx = (box[7].x -box[0].x)/((float)numitems);
      if (dx == 0) {
         DXSetError(ERROR_DATA_INVALID,"object has size 0 bounding box");
         goto error;
      }
      if (!DXExtractFloat(density, &densval)) {
         DXSetError(ERROR_DATA_INVALID,
                    "for 1D data, densityfactor must be a scalar value");
         goto error;
      }
      if (densval <=0) {
         DXSetError(ERROR_DATA_INVALID,
                    "densityfactor must be positive");
         goto error;
      }
      dx = dx/densval;
      numitems = RND(numitems*densval);
      gridpositions = DXMakeGridPositions(1, numitems+1, origin.x, dx);
      *radius = 2*dx;
      gridconnections = DXMakeGridConnections(1, numitems+1);
   }
   /* 2D positions */
   else if ((rank==1)&&(shape[0]==2)) {
      if (!DXExtractParameter(density, TYPE_FLOAT,2,1,&densvector)) {
         if (!DXExtractFloat(density, &densval)) {
            DXSetError(ERROR_DATA_INVALID,
                     "for 2D data densityfactor must be a scalar or 2-vector");
            goto error;
         }
         densvector.x = densval;
         densvector.y = densval;
      }
      if ((densvector.x <=0)||(densvector.y <=0)) {
        DXSetError(ERROR_DATA_INVALID,
                   "densityfactor must be positive");
        goto error;
      }

      origin = box[0];
      DX = box[7].x - box[0].x;
      DY = box[7].y - box[0].y;
      if ((DX == 0)&&(DY==0)) {
         DXSetError(ERROR_DATA_INVALID,"object has size 0 bounding box");
         goto error;
      }

      /* make lines in 2space */
      if (DX == 0) {
         /* down in dimensionality */
         ny_float = numitems;
         ny = RND(ny_float*densvector.y);
         if (ny < 2) ny = 2;
         dy = DY/ny;
         gridpositions = DXMakeGridPositions(2, 
                                             1, ny+1, 
                                             origin.x, origin.y,
                                             0.0, 0.0,
                                             0.0, dy);
         *radius = 2*dy;
         gridconnections = DXMakeGridConnections(1, 
                                                 ny+1);
      }
      else if (DY == 0) {
         /* down in dimensionality */
         nx_float = numitems;
         nx = RND(nx_float*densvector.x);
         if (nx < 2) nx = 2;
         dx = DX/nx;
         gridpositions = DXMakeGridPositions(2, 
                                             nx+1, 1, 
                                             origin.x, origin.y,
                                             dx, 0.0,
                                             0.0, 0.0);
         *radius = 2*dx;
         gridconnections = DXMakeGridConnections(1, 
                                                 nx+1);
      }
      else {
         ny_float = sqrt((numitems * DY)/DX);
         nx_float = (DX/DY)*ny_float;
         ny = RND(ny_float*densvector.y);
         nx = RND(nx_float*densvector.x);

         if (nx < 2) nx = 2;
         if (ny < 2) ny = 2;

         dx = DX/nx;
         dy = DY/ny;
         gridpositions = DXMakeGridPositions(2, 
                                          nx+1, ny+1, 
                                          origin.x, origin.y,
                                          dx, 0.0,
                                          0.0, dy);
         *radius = 2*MAX(dx,dy);
         gridconnections = DXMakeGridConnections(2, 
                                              nx+1, ny+1);
     }
   }
   /* 3D positions */
   else if ((rank==1)&&(shape[0]==3)) {
      if (!DXExtractParameter(density, TYPE_FLOAT,3,1,&densvector)) {
         if (!DXExtractFloat(density, &densval)) {
            DXSetError(ERROR_DATA_INVALID,
                     "for 3D data densityfactor must be a scalar or 3-vector");
            goto error;
         }
         densvector.x = densval;
         densvector.y = densval;
         densvector.z = densval;
      }
      if ((densvector.x <=0)||(densvector.y <=0)||(densvector.z <= 0)) {
        DXSetError(ERROR_DATA_INVALID,
                   "densityfactor must be positive");
        goto error;
      }
      origin = box[0];
      DX = box[7].x - box[0].x;
      DY = box[7].y - box[0].y;
      DZ = box[7].z - box[0].z;
      if ((DX == 0)&&(DY==0)&&(DZ==0)) {
         DXSetError(ERROR_DATA_INVALID,"object has size 0 bounding box");
         goto error;
      }

      if ((DX == 0) && (DY == 0)) {
         /* make lines in 3-space */
         nz_float = numitems;
         nz = RND(nz_float*densvector.z);
         if (nz < 2) nz = 2;
         dz = DZ/nz;
         gridpositions = DXMakeGridPositions(3, 
                                             1, 1, nz+1, 
                                             origin.x, origin.y, origin.z,
                                             0.0, 0.0, 0.0,
                                             0.0, 0.0, 0.0,
                                             0.0, 0.0, dz);
         *radius = 2*dz;
         gridconnections = DXMakeGridConnections(1, 
                                                 nz+1);
      }
      else if ((DX == 0) && (DZ == 0)) {
         /* make lines in 3-space */
         ny_float = numitems;
         ny = RND(ny_float*densvector.y);
         if (ny < 2) ny = 2;
         dy = DY/ny;
         gridpositions = DXMakeGridPositions(3, 
                                             1, ny+1, 1, 
                                             origin.x, origin.y, origin.z,
                                             0.0, 0.0, 0.0,
                                             0.0, dy, 0.0,
                                             0.0, 0.0, 0.0);
         *radius = 2*dy;
         gridconnections = DXMakeGridConnections(1, 
                                                 ny+1);
      }
      else if ((DY == 0) && (DZ == 0)) {
         /* make lines in 3-space */
         nx_float = numitems;
         nx = RND(nx_float*densvector.x);
         if (nx < 2) nx = 2;
         dx = DX/nx;
         gridpositions = DXMakeGridPositions(3, 
                                             nx+1, 1, 1, 
                                             origin.x, origin.y, origin.z,
                                             dx, 0.0, 0.0,
                                             0.0, 0.0, 0.0,
                                             0.0, 0.0, 0.0);
         *radius = 2*dx;
         gridconnections = DXMakeGridConnections(1, 
                                                 nx+1);
      }
      else if (DX == 0) {
         /* down a dimension */
         nz_float = sqrt((numitems * DZ)/DY);
         ny_float = (DY/DZ)*nz_float;
         nz = RND(nz_float*densvector.z);
         ny = RND(ny_float*densvector.y);
         if (ny < 2) ny = 2;
         if (nz < 2) nz = 2;
         dy = DY/ny;
         dz = DZ/nz; 
         gridpositions = DXMakeGridPositions(3, 
                                             1, ny+1, nz+1, 
                                             origin.x, origin.y, origin.z,
                                             0.0, 0.0, 0.0,
                                             0.0, dy, 0.0, 
                                             0.0, 0.0, dz);
         *radius = 2*MAX(dz,dy);
         gridconnections = DXMakeGridConnections(2, 
                                                 ny+1, nz+1);
      }
      else if (DY == 0) {
         /* down a dimension */
         nz_float = sqrt((numitems * DZ)/DX);
         nx_float = (DX/DZ)*nz_float;
         nz = RND(nz_float*densvector.z);
         nx = RND(nx_float*densvector.x);
         if (nx < 2) nx = 2;
         if (nz < 2) nz = 2;
         dx = DX/nx;
         dz = DZ/nz; 
         gridpositions = DXMakeGridPositions(3, 
                                             nx+1, 1, nz+1, 
                                             origin.x, origin.y, origin.z,
                                             dx, 0.0, 0.0,
                                             0.0, 0.0, 0.0, 
                                             0.0, 0.0, dz);
         *radius = 2*MAX(dz,dx);
         gridconnections = DXMakeGridConnections(2, 
                                                 nx+1, nz+1);
      }
      else if (DZ == 0) {
         /* down a dimension */
         ny_float = sqrt((numitems * DY)/DX);
         nx_float = (DX/DY)*ny_float;
         ny = RND(ny_float*densvector.y);
         nx = RND(nx_float*densvector.x);
         if (nx < 2) nx = 2;
         if (ny < 2) ny = 2;
         dx = DX/nx;
         dy = DY/ny; 
         gridpositions = DXMakeGridPositions(3, 
                                             nx+1, ny+1, 1, 
                                             origin.x, origin.y, origin.z,
                                             dx, 0.0, 0.0,
                                             0.0, dy, 0.0, 
                                             0.0, 0.0, 0.0);
         *radius = 2*MAX(dy,dx);
         gridconnections = DXMakeGridConnections(2, 
                                                 nx+1, ny+1);
      }
      else { /* all are non-zero */

      nz_float = numitems*DZ*DZ/(DX*DY);
      nz_float = pow(nz_float, .3333);
      ny_float = nz_float*(DY/DZ);
      nx_float = ny_float*(DX/DY); 

      nz = RND(nz_float*densvector.z);
      ny = RND(ny_float*densvector.y);
      nx = RND(nx_float*densvector.x);

      if (nx < 2) nx = 2;
      if (ny < 2) ny = 2;
      if (nz < 2) nz = 2;

      dx = DX/nx;
      dy = DY/ny;
      dz = DZ/nz; 
      gridpositions = DXMakeGridPositions(3, 
                                          nx+1, ny+1, nz+1, 
                                          origin.x, origin.y, origin.z,
                                          dx, 0.0, 0.0,
                                          0.0, dy, 0.0, 
                                          0.0, 0.0, dz);
      *radius = 2*MAX(dx,MAX(dy,dz));
      gridconnections = DXMakeGridConnections(3, 
                                              nx+1, ny+1, nz+1);
     }
   }
   else {
     DXSetError(ERROR_DATA_INVALID,"positions must be 1,2, or 3D");
     goto error;
   } 

  grid = DXNewField();
  if (!grid) goto error;
  if (!DXSetComponentValue((Field)grid, "positions", (Object)gridpositions))
     goto error;
  if (!DXSetComponentValue((Field)grid, "connections", (Object)gridconnections))
     goto error;

  return (Object)grid; 
error:
  return NULL;
}



