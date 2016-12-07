/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include <dx/dx.h>

static Matrix Identity = {
  {{ 1.0, 0.0, 0.0 },
   { 0.0, 1.0, 0.0 },
   { 0.0, 0.0, 1.0 }
  }
};

#include <math.h>
#include <string.h>
#include "eigen.h"
#include "_glyph.h"
#include "_autocolor.h"
#include "_post.h"
#define TRUE                    1
#define FALSE                   0
#define DIMENSION3D             3
#define DIMENSION2D             2


static Error IndexGlyphField(Field, Object, float);
static Error CheckGlyphGroup(Object, int *, int *, char *); 
static Error GlyphXform2d(Matrix, int, Point *, float *);
static Error GetEigenVectors(Stress_Tensor, Vector *);
static Error GetEigenVectors2(Stress_Tensor2, Vector *);
static Error TextGlyphs(Group, char *, Matrix, RGBColor, float, char *);
static Matrix GlyphAlign (Vector, Vector);
static Error IncrementConnections(int, int, Triangle *, Triangle *);
static Error IncrementConnectionsLines(int, int, Line *, Line *);
static Error GlyphXform(Matrix, int, Point *, Point *);
static Error GetNumberValidPositions(Object, int *);
static Error GlyphNormalXform(Matrix, int, Point *, Point *);
static Error GetGlyphName(float, char *, int, char *, char *, char *, int *);
static Error GetGlyphs(char *, int *, int *, Point **, Triangle **, 
		Point **, Line **, int *, char *);
static Error
  GetGlyphField(Object, int *, int *, Point **, Triangle **, Line **, 
		Point **, Matrix *, int *, int *, int *, char *);
static int DivideByZero(float, float);
static int _dxfHasDataComponent(Object);

extern Object Exists2(Object o, char *name, char *name2); /* from libdx/component.c */
extern Error _dxfDXEmptyObject(Object); /* from libdx/component.c */

Error _dxfGetFieldTensorStats(Field, float *, float *);


typedef struct Point2D
{
   float x; 
   float y;
} Point2D;

typedef struct arg 
{
      Object outobject;
      char type[30];
      Object typefield;
      int type_is_field;
      float shape;
      float scale;
      int scale_set;
      float ratio;
      int ratio_set;
      float given_min;
      int min_set;
      float given_max;
      int max_set;
      float quality;
      float base_size;
      int AutoSize;
      Object parent;
      int parentindex;
      char font[100];
} Arg;
     
typedef struct textarg 
{
      Object inobject;
      Object outobject;
      int colored;
      float scale;
      char font[100];
} Textarg;
     
static RGBColor DEFAULT_SCALAR_COLOR = { 0.5F, 0.7F, 1.0F};
static RGBColor OVERRIDE_SCALAR_COLOR = { 1.0F, 0.8F, 0.8F};
static RGBColor DEFAULT_VECTOR_COLOR = { 0.7F, 0.7F, 0.0F};
static RGBColor DEFAULT_TENSOR_COLOR = { 1.0F, 0.0F, 0.0F};


int _dxfIsFieldorGroup(Object ob)
{
  Class cl;
  

  if ( ((cl = DXGetObjectClass(ob)) == CLASS_FIELD) || 
      (cl == CLASS_GROUP) )
    return OK;
  else
    return ERROR;
}


Error _dxfTextField(Pointer p)
{
   Object inobject, outobject;
   Array positions=NULL, data, colors=NULL, colormap, invalid_pos;
   int invalid;
   float scale;
   InvalidComponentHandle invalidhandle=NULL;
   Stress_Tensor t;
   int numcoloritems, n;
   Type type, colorstype;
   Category category;
   int numitems, rank, posshape[30], datashape[30], i, colored, reginputcolors=0;
   int colorsrank, colorsshape[30], delayedcolors=0, posted=0, counts[30], j;
   char glyph_rank[30]; 
   float s, posorigin[30], posdeltas[30];
   Vector v={0,0,0};
   unsigned char *colors_byte_ptr=NULL;
   RGBColor color = {0,0,0}, colororigin, *colors_ptr=NULL;
   char string[100], *string1, *attr, *font;
   Matrix translation; 
   float *data_ptr_f=NULL;
   ubyte *data_ptr_ub=NULL;
   byte *data_ptr_b=NULL;
   short *data_ptr_s=NULL;
   ushort *data_ptr_us=NULL;
   double *data_ptr_d=NULL;
   int *data_ptr_i=NULL;
   uint *data_ptr_ui=NULL;
   ArrayHandle handle=NULL;
   Pointer scratch=NULL;
   float *position=NULL;

   /* this routine has the object stripped down to the bottom field level
      of Object in. Object out is a group to put text glyphs into */

   /* inobject is a copy which needs to be deleted */
   
   Textarg *textarg = (Textarg *)p;
   
   inobject = textarg->inobject;
   outobject = textarg->outobject;
   colored = textarg->colored;
   scale = textarg->scale;
   font = textarg->font;
   
   /* Get the positions and data components of the input object */
   data = (Array)DXGetComponentValue((Field)inobject,"data");
   if (!data) {
     DXSetError(ERROR_MISSING_DATA,
                "data component required for text glyphs");
     goto error;
   }
   attr = DXGetString((String)DXGetComponentAttribute((Field)inobject,
						      "data", "dep"));
   if (!attr) {
      DXSetError(ERROR_DATA_INVALID,"#10255", "data","dep");
      goto error;
   }
   posted = 0;

   /* check the attribute */
   if (strcmp(attr,"connections") && (strcmp(attr,"positions"))) {
      DXSetError(ERROR_DATA_INVALID,"unsupported data dependency \'%s\'",attr);
      goto error;
   }



   if (!strcmp(attr,"connections")) {
     /* I need to check whether the positions are regular. If they
      * are, a simple grid shift is in order. Otherwise, use PostArray
      * to shift them */
     Array pos, con;
     pos = (Array)DXGetComponentValue((Field)inobject,"positions");
     con = (Array)DXGetComponentValue((Field)inobject,"connections");
     if ((DXQueryGridPositions(pos, &n, counts, posorigin, posdeltas)) &&
	 DXQueryGridConnections(con, NULL, NULL)) {
       for (i=0; i<n; i++) {
         for (j=0; j<n; j++) {
	   if (counts[j] > 1) {
	     posorigin[i]+=0.5*posdeltas[i + j*n];
	   }
         }
       }
       for (i=0; i<n; i++) {
         if (counts[i] >1) counts[i] = counts[i]-1;
       }  
       positions = DXMakeGridPositionsV(n, counts, posorigin, posdeltas); 
     }

     /* else the positions are irregular */
     else {
       positions = _dxfPostArray((Field)inobject, "positions","connections");
     }


     if (!positions) {
       DXSetError(ERROR_BAD_PARAMETER,
		  "could not compute glyph positions for cell-centered data");
       goto error;
     }

     posted = 1;
     if (!DXGetArrayInfo(positions,&numitems, &type, &category, 
			 &rank, posshape))
       goto error;
   }
   else if (!strcmp(attr,"faces")) {
      positions = _dxfPostArray((Field)inobject, "positions","faces");
      if (!positions) {
        DXAddMessage("could not compute glyph positions for face-centered data");
        goto error;
      }
      posted = 1;
      if (!DXGetArrayInfo(positions,&numitems, &type, &category, 
 			 &rank, posshape))
         goto error;

   }

   /* else dep positions */
   else {
     positions = (Array)DXGetComponentValue((Field)inobject,"positions");
     if (!positions) {
       DXSetError(ERROR_BAD_PARAMETER,"field has no positions");
       goto error;
     }
     if (!DXGetArrayInfo(positions,&numitems, &type, &category, 
			 &rank, posshape))
         goto error;
   }
   if ((type != TYPE_FLOAT)||(category != CATEGORY_REAL)) {
     DXSetError(ERROR_DATA_INVALID,
		"positions must be type float category real");
     goto error;
   }
   if (rank != 1) {
     DXSetError(ERROR_DATA_INVALID,"positions must be rank 1");
     goto error;
   }
   if ((posshape[0] < 1) || (posshape[0] > 3)) {
     DXSetError(ERROR_DATA_INVALID,"positions must be 1D, 2D, or 3D");
     goto error;
   }
   DXGetArrayInfo(data, &numitems, &type, &category, &rank, datashape);
   if (category != CATEGORY_REAL) {
     DXSetError(ERROR_DATA_INVALID,"data must be category real");
     goto error;
   }
   if (rank==0)
     strcpy(glyph_rank,"scalar");
   else if (rank==1 && datashape[0] == 1)
     strcpy(glyph_rank,"scalar");
   else if (rank==1 && datashape[0] == 2)
     strcpy(glyph_rank,"2-vector");
   else if (rank==1 && datashape[0] == 3)
     strcpy(glyph_rank,"3-vector");
   else if (rank==2 && datashape[0]==3 &&datashape[1]==3)
     strcpy(glyph_rank,"matrix");
   else if (rank==2 && datashape[0]==2 &&datashape[1]==2)
     strcpy(glyph_rank,"matrix2");
   else if (type != TYPE_STRING) {
     DXSetError(ERROR_DATA_INVALID,
		"data must be string, scalar, 2-vector, 3-vector, 2x2 or 3x3 matrix");
     goto error;
   }
   
   /* get the colors component, if necessary, and if present */
   if (colored) {
     colors = (Array)DXGetComponentValue((Field)inobject,"colors");
     if (!colors) {
       colored = 0;
     }
   }
   if (!colored) 
     color = DXRGB(1.0,1.0,1.0);
   
   
   /* get pointers to data and positions */
  
   /* prepare to access the positions */
   if (!(handle = DXCreateArrayHandle(positions)))
       goto error;
   scratch = DXAllocateLocal(posshape[0]*sizeof(float));
   if (!scratch)
       goto error;

 
   
   switch (type) {
   case (TYPE_SHORT):
     data_ptr_s = (short *)DXGetArrayData(data);
     break;
   case (TYPE_INT):
     data_ptr_i = (int *)DXGetArrayData(data);
     break;
   case (TYPE_FLOAT):
     data_ptr_f = (float *)DXGetArrayData(data);
     break;
   case (TYPE_DOUBLE):
     data_ptr_d = (double *)DXGetArrayData(data);
     break;
   case (TYPE_UBYTE):
     data_ptr_ub = (ubyte *)DXGetArrayData(data);
     break;
   case (TYPE_BYTE):
     data_ptr_b = (byte *)DXGetArrayData(data);
     break;
   case (TYPE_USHORT):
     data_ptr_us = (ushort *)DXGetArrayData(data);
     break;
   case (TYPE_UINT):
     data_ptr_ui = (uint *)DXGetArrayData(data);
     break;
   case (TYPE_STRING):
     strcpy(glyph_rank,"string");
     break;
   default:
     DXSetError(ERROR_DATA_INVALID,"unrecognized data type");
     goto error;
   } 
   
   if (colored) {
     if (DXQueryConstantArray(colors, NULL, (Pointer)&colororigin)) {
       reginputcolors = 1;
     }
     else {
       reginputcolors = 0;
       /* check delayed or not */
       DXGetArrayInfo(colors,NULL, &colorstype, NULL,&colorsrank,colorsshape);
       if (colorstype == TYPE_FLOAT) {
	 if ((colorsrank != 1)||(colorsshape[0]!=3)) {
	   DXSetError(ERROR_DATA_INVALID,
		      "only 3D floating point or 1D delayed colors acceptable");
	   goto error;
	 }
       }
       else {
	 if (colorstype != TYPE_UBYTE) {
	   DXSetError(ERROR_DATA_INVALID,
		      "only 3D floating point or 1D delayed colors acceptable");
	   goto error;
	 } 
	 if (!((colorsrank == 0)||((colorsrank ==1)&&(colorsshape[0]==1)))) {
	   DXSetError(ERROR_DATA_INVALID,
		      "only 3D floating point or 1D delayed colors acceptable");
	   goto error;
	 }
	 colormap = (Array)DXGetComponentValue((Field)inobject,"color map");
	 if (!colormap) {
	   DXSetError(ERROR_DATA_INVALID,
		      "only 3D floating point or 1D delayed colors acceptable (no color map component)");
	   goto error;
	 }
	 if (!DXGetArrayInfo(colormap,&numcoloritems,
			     &colorstype,NULL,&colorsrank, colorsshape))
	   goto error;
	 if (colorstype != TYPE_FLOAT) {
	   DXSetError(ERROR_DATA_INVALID,
		      "color map component must be of type float");
	   goto error;
	 }
	 if ((colorsrank != 1)||(colorsshape[0]!=3)) {
	   DXSetError(ERROR_DATA_INVALID,
		      "color map component must be 3D");
	   goto error;
	 }
	 if (numcoloritems != 256) {
	   DXSetError(ERROR_DATA_INVALID,
		      "color map component does not have 256 items");
	   goto error;
	 }
	 delayedcolors = 1;
	 colors_ptr = (RGBColor *)DXGetArrayData(colormap);
	 
       }
       if (!delayedcolors)
         colors_ptr = (RGBColor *)DXGetArrayData(colors);
       else
         colors_byte_ptr = (unsigned char *)DXGetArrayData(colors);
     }
   }
   
   
   /* see if there's an invalid positions (or connections) array  */
   invalid = 0;
   if (!strcmp(attr,"connections")) {
     invalid_pos = (Array)DXGetComponentValue((Field)inobject,
					      "invalid connections");
     if (invalid_pos) {
       invalid = 1;
       invalidhandle = DXCreateInvalidComponentHandle((Object)inobject,
                                                      NULL, "connections");
       if (!invalidhandle)
          goto error;
     }
   }
   else {
     invalid_pos = (Array)DXGetComponentValue((Field)inobject,
					      "invalid positions");
     if (invalid_pos) {
       invalid = 1;
       invalidhandle = DXCreateInvalidComponentHandle((Object)inobject,
                                                      NULL, "positions");
       if (!invalidhandle)
          goto error;
     }
   }
   

   
   if (!strcmp(glyph_rank,"string")) {
     for (i=0; i<numitems; i++) { 
       if (colored) {
	 if (reginputcolors) 
	   color = colororigin;
	 else {
	   if (!delayedcolors)
	     color = colors_ptr[i];
	   else
	     color = colors_ptr[(int)colors_byte_ptr[i]];
	 }
       }
       if ((invalid)&&(DXIsElementInvalidSequential(invalidhandle, i))) {
         /* don't do anything */
       }
       else { 
         if (!DXExtractNthString((Object)data, i, &string1)) {
	   DXSetError(ERROR_INTERNAL,
		      "error extracting %dth string from data");
           goto error;
	 }

         position = DXGetArrayEntry(handle,i,scratch);
         if (posshape[0]==1) 
           translation = DXTranslate(DXVec(position[0],0.0,0.0));
         else if (posshape[0]==2)
           translation = DXTranslate(DXVec(position[0],position[1],0.0));
         else
           translation = DXTranslate(DXVec(position[0],position[1],
                                     position[2]));
	 if (!TextGlyphs((Group)outobject,string1,translation, color, scale,
                          font)) {
            goto error;
	 }
       }
     }
   }
   else if (!strcmp(glyph_rank,"scalar")) {
     for (i=0; i<numitems; i++) { 
       if (colored) {
	 if (reginputcolors) 
	   color = colororigin;
	 else {
	   if (!delayedcolors)
	     color = colors_ptr[i];
	   else
	     color = colors_ptr[(int)colors_byte_ptr[i]];
	 }
       }
       if ((invalid)&&(DXIsElementInvalidSequential(invalidhandle, i))) {
         /* don't do anything */
       }
       else { 
	 switch (type) {
         case (TYPE_SHORT):
	   s = (float)data_ptr_s[i];
           break;
         case (TYPE_INT):
	   s = (float)data_ptr_i[i];
           break;
         case (TYPE_FLOAT):
	   s = (float)data_ptr_f[i];
           break;
         case (TYPE_DOUBLE):
	   s = (float)data_ptr_d[i];
           break;
         case (TYPE_BYTE):
	   s = (float)data_ptr_b[i];
           break;
         case (TYPE_UBYTE):
	   s = (float)data_ptr_ub[i];
           break;
         case (TYPE_USHORT):
	   s = (float)data_ptr_us[i];
           break;
         case (TYPE_UINT):
	   s = (float)data_ptr_ui[i];
           break;
         default:
           DXSetError(ERROR_DATA_INVALID,"unrecognized data type");
           goto error;
	 }
	 sprintf(string, "%g", s);
	 position = DXGetArrayEntry(handle,i,scratch);
	 if (posshape[0] == 1) {
	   translation = DXTranslate(DXVec(position[0], 0.0, 0.0));
	 }
	 else if (posshape[0] == 2) {
	   translation = DXTranslate(DXVec(position[0], position[1],0.0));
	 }
	 else
	   translation = DXTranslate(DXVec(position[0], 
					   position[1],
					   position[2]));
	 if (!TextGlyphs((Group)outobject,string,translation, color, 
                         scale, font)) {
            goto error;
	 }
       }
     }
   }
   else if (!strcmp(glyph_rank,"2-vector")) {
     for (i=0; i<numitems; i++) { 
       if (colored) {
	 if (reginputcolors) 
	   color = colororigin;
	 else { 
	   if (!delayedcolors)
	     color = colors_ptr[i];
	   else
	     color = colors_ptr[(int)colors_byte_ptr[i]];
	 }
       }
       if ((invalid)&&(DXIsElementInvalidSequential(invalidhandle, i))) {
         /* don't do anything */
       }
       else {
	 switch (type) {
         case (TYPE_SHORT):
	   v.x = (float)data_ptr_s[2*i];
	   v.y = (float)data_ptr_s[2*i+1];
           break;
         case (TYPE_INT):
	   v.x = (float)data_ptr_i[2*i];
	   v.y = (float)data_ptr_i[2*i+1];
           break;
         case (TYPE_FLOAT):
	   v.x = (float)data_ptr_f[2*i];
	   v.y = (float)data_ptr_f[2*i+1];
           break;
         case (TYPE_DOUBLE):
	   v.x = (float)data_ptr_d[2*i];
	   v.y = (float)data_ptr_d[2*i+1];
           break;
         case (TYPE_BYTE):
	   v.x = (float)data_ptr_b[2*i];
	   v.y = (float)data_ptr_b[2*i+1];
           break;
         case (TYPE_UBYTE):
	   v.x = (float)data_ptr_ub[2*i];
	   v.y = (float)data_ptr_ub[2*i+1];
           break;
         case (TYPE_USHORT):
	   v.x = (float)data_ptr_us[2*i];
	   v.y = (float)data_ptr_us[2*i+1];
           break;
         case (TYPE_UINT):
	   v.x = (float)data_ptr_ui[2*i];
	   v.y = (float)data_ptr_ui[2*i+1];
           break;
	 default:
	   break;
	 }
	 sprintf(string, "[%g %g]", v.x, v.y);
         position = DXGetArrayEntry(handle,i,scratch);
	 if (posshape[0] == 1) {
	   translation = DXTranslate(DXVec(position[0], 0.0, 0.0));
	 }
	 else if (posshape[0] == 2) {
	   translation = DXTranslate(DXVec(position[0], position[1],0.0));
	 }
	 else
	   translation = DXTranslate(DXVec(position[0], 
					   position[1],
					   position[2]));
	 if (!TextGlyphs((Group)outobject,string,translation,color, 
                          scale, font)) {
            goto error;
	 }
       }
     }
   }
   else if (!strcmp(glyph_rank,"3-vector")) {
     for (i=0; i<numitems; i++) { 
       if (colored) {
	 if (reginputcolors) 
	   color = colororigin;
	 else {
	   if (!delayedcolors)
	     color = colors_ptr[i];
	   else
	     color = colors_ptr[(int)colors_byte_ptr[i]];
	 }
       }
       if ((invalid)&&(DXIsElementInvalidSequential(invalidhandle, i))) {
         /* don't do anything */
       }
       else {
	 switch (type) {
         case (TYPE_SHORT):
	   v.x = (float)data_ptr_s[3*i];
	   v.y = (float)data_ptr_s[3*i+1];
	   v.z = (float)data_ptr_s[3*i+2];
           break;
         case (TYPE_INT):
	   v.x = (float)data_ptr_i[3*i];
	   v.y = (float)data_ptr_i[3*i+1];
	   v.z = (float)data_ptr_i[3*i+2];
           break;
         case (TYPE_FLOAT):
	   v.x = (float)data_ptr_f[3*i];
	   v.y = (float)data_ptr_f[3*i+1];
	   v.z = (float)data_ptr_f[3*i+2];
           break;
         case (TYPE_DOUBLE):
	   v.x = (float)data_ptr_d[3*i];
	   v.y = (float)data_ptr_d[3*i+1];
	   v.z = (float)data_ptr_d[3*i+2];
           break;
         case (TYPE_BYTE):
	   v.x = (float)data_ptr_b[3*i];
	   v.y = (float)data_ptr_b[3*i+1];
	   v.z = (float)data_ptr_b[3*i+2];
           break;
         case (TYPE_UBYTE):
	   v.x = (float)data_ptr_ub[3*i];
	   v.y = (float)data_ptr_ub[3*i+1];
	   v.z = (float)data_ptr_ub[3*i+2];
           break;
         case (TYPE_USHORT):
	   v.x = (float)data_ptr_us[3*i];
	   v.y = (float)data_ptr_us[3*i+1];
	   v.z = (float)data_ptr_us[3*i+2];
           break;
         case (TYPE_UINT):
	   v.x = (float)data_ptr_ui[3*i];
	   v.y = (float)data_ptr_ui[3*i+1];
	   v.z = (float)data_ptr_ui[3*i+2];
           break;
	 default:
	   break;
	 }
	 sprintf(string, "[%g %g %g]", v.x, v.y, v.z);
         position = DXGetArrayEntry(handle,i,scratch);
	 if (posshape[0] == 1) {
	   translation = DXTranslate(DXVec(position[0], 0.0, 0.0));
	 }
	 else if (posshape[0] == 2) {
	   translation = DXTranslate(DXVec(position[0], position[1],0.0));
	 }
	 else
	   translation = DXTranslate(DXVec(position[0], 
					   position[1],
					   position[2]));
	 if (!TextGlyphs((Group)outobject,string,translation,color, 
                          scale, font)) {
             goto error;
	 }
       }
     }
   }
   else  {
     for (i=0; i<numitems; i++) { 
       if (colored) {
	 if (reginputcolors) 
	   color = colororigin;
	 else {
	   if (!delayedcolors)
	     color = colors_ptr[i];
	   else
	     color = colors_ptr[(int)colors_byte_ptr[i]];
	 }
       }
       if ((invalid)&&(DXIsElementInvalidSequential(invalidhandle, i))) {
         /* don't do anything */
       }
       else {
	 switch (type) {
         case (TYPE_SHORT):
	   t.tau[0][0] = (float)data_ptr_s[9*i];
	   t.tau[0][1] = (float)data_ptr_s[9*i+1];
	   t.tau[0][2] = (float)data_ptr_s[9*i+2];
	   t.tau[1][0] = (float)data_ptr_s[9*i+3];
	   t.tau[1][1] = (float)data_ptr_s[9*i+4];
	   t.tau[1][2] = (float)data_ptr_s[9*i+5];
	   t.tau[2][0] = (float)data_ptr_s[9*i+6];
	   t.tau[2][1] = (float)data_ptr_s[9*i+7];
	   t.tau[2][2] = (float)data_ptr_s[9*i+8];
           break;
         case (TYPE_INT):
	   t.tau[0][0] = (float)data_ptr_i[9*i];
	   t.tau[0][1] = (float)data_ptr_i[9*i+1];
	   t.tau[0][2] = (float)data_ptr_i[9*i+2];
	   t.tau[1][0] = (float)data_ptr_i[9*i+3];
	   t.tau[1][1] = (float)data_ptr_i[9*i+4];
	   t.tau[1][2] = (float)data_ptr_i[9*i+5];
	   t.tau[2][0] = (float)data_ptr_i[9*i+6];
	   t.tau[2][1] = (float)data_ptr_i[9*i+7];
	   t.tau[2][2] = (float)data_ptr_i[9*i+8];
           break;
         case (TYPE_FLOAT):
	   t.tau[0][0] = (float)data_ptr_f[9*i];
	   t.tau[0][1] = (float)data_ptr_f[9*i+1];
	   t.tau[0][2] = (float)data_ptr_f[9*i+2];
	   t.tau[1][0] = (float)data_ptr_f[9*i+3];
	   t.tau[1][1] = (float)data_ptr_f[9*i+4];
	   t.tau[1][2] = (float)data_ptr_f[9*i+5];
	   t.tau[2][0] = (float)data_ptr_f[9*i+6];
	   t.tau[2][1] = (float)data_ptr_f[9*i+7];
	   t.tau[2][2] = (float)data_ptr_f[9*i+8];
           break;
         case (TYPE_DOUBLE):
	   t.tau[0][0] = (float)data_ptr_d[9*i];
	   t.tau[0][1] = (float)data_ptr_d[9*i+1];
	   t.tau[0][2] = (float)data_ptr_d[9*i+2];
	   t.tau[1][0] = (float)data_ptr_d[9*i+3];
	   t.tau[1][1] = (float)data_ptr_d[9*i+4];
	   t.tau[1][2] = (float)data_ptr_d[9*i+5];
	   t.tau[2][0] = (float)data_ptr_d[9*i+6];
	   t.tau[2][1] = (float)data_ptr_d[9*i+7];
	   t.tau[2][2] = (float)data_ptr_d[9*i+8];
           break;
         case (TYPE_BYTE):
	   t.tau[0][0] = (float)data_ptr_b[9*i];
	   t.tau[0][1] = (float)data_ptr_b[9*i+1];
	   t.tau[0][2] = (float)data_ptr_b[9*i+2];
	   t.tau[1][0] = (float)data_ptr_b[9*i+3];
	   t.tau[1][1] = (float)data_ptr_b[9*i+4];
	   t.tau[1][2] = (float)data_ptr_b[9*i+5];
	   t.tau[2][0] = (float)data_ptr_b[9*i+6];
	   t.tau[2][1] = (float)data_ptr_b[9*i+7];
	   t.tau[2][2] = (float)data_ptr_b[9*i+8];
           break;
         case (TYPE_UBYTE):
	   t.tau[0][0] = (float)data_ptr_ub[9*i];
	   t.tau[0][1] = (float)data_ptr_ub[9*i+1];
	   t.tau[0][2] = (float)data_ptr_ub[9*i+2];
	   t.tau[1][0] = (float)data_ptr_ub[9*i+3];
	   t.tau[1][1] = (float)data_ptr_ub[9*i+4];
	   t.tau[1][2] = (float)data_ptr_ub[9*i+5];
	   t.tau[2][0] = (float)data_ptr_ub[9*i+6];
	   t.tau[2][1] = (float)data_ptr_ub[9*i+7];
	   t.tau[2][2] = (float)data_ptr_ub[9*i+8];
           break;
         case (TYPE_USHORT):
	   t.tau[0][0] = (float)data_ptr_us[9*i];
	   t.tau[0][1] = (float)data_ptr_us[9*i+1];
	   t.tau[0][2] = (float)data_ptr_us[9*i+2];
	   t.tau[1][0] = (float)data_ptr_us[9*i+3];
	   t.tau[1][1] = (float)data_ptr_us[9*i+4];
	   t.tau[1][2] = (float)data_ptr_us[9*i+5];
	   t.tau[2][0] = (float)data_ptr_us[9*i+6];
	   t.tau[2][1] = (float)data_ptr_us[9*i+7];
	   t.tau[2][2] = (float)data_ptr_us[9*i+8];
           break;
         case (TYPE_UINT):
	   t.tau[0][0] = (float)data_ptr_ui[9*i];
	   t.tau[0][1] = (float)data_ptr_ui[9*i+1];
	   t.tau[0][2] = (float)data_ptr_ui[9*i+2];
	   t.tau[1][0] = (float)data_ptr_ui[9*i+3];
	   t.tau[1][1] = (float)data_ptr_ui[9*i+4];
	   t.tau[1][2] = (float)data_ptr_ui[9*i+5];
	   t.tau[2][0] = (float)data_ptr_ui[9*i+6];
	   t.tau[2][1] = (float)data_ptr_ui[9*i+7];
	   t.tau[2][2] = (float)data_ptr_ui[9*i+8];
           break;
	 default:
	   break;
	 }
	 sprintf(string, "[%g %g %g][%g %g %g][%g %g %g]", 
		 t.tau[0][0], t.tau[0][1], t.tau[0][2],
		 t.tau[1][0], t.tau[1][1], t.tau[1][2],
		 t.tau[2][0], t.tau[2][1], t.tau[2][2]);
         position = DXGetArrayEntry(handle,i,scratch);
	 if (posshape[0] == 1) {
	   translation = DXTranslate(DXVec(position[0], 0.0, 0.0));
	 }
	 else if (posshape[0] == 2) {
	   translation = DXTranslate(DXVec(position[0], position[1],0.0));
	 }
	 else
	   translation = DXTranslate(DXVec(position[0], 
					   position[1],
					   position[2]));
	 if (!TextGlyphs((Group)outobject,string,translation,color, 
                          scale, font)) {
            goto error;
	 }
       }
     }
   }
   if (posted) DXDelete((Object)positions);
   DXFreeInvalidComponentHandle(invalidhandle);
   DXFreeArrayHandle(handle);
   DXFree((Pointer)scratch);
   DXDelete((Object)inobject);
   return OK;
 error:
   DXFreeInvalidComponentHandle(invalidhandle);
   DXFreeArrayHandle(handle);
   DXFree((Pointer)scratch);
   if (posted) DXDelete((Object)positions);
   DXDelete((Object)inobject);
   return ERROR;
 }

static Error TextGlyphs(Group object, char *string, Matrix translation,
                        RGBColor color, float scalefactor, char *fontname)
{
   static Object font=NULL;
   Matrix scale;
   Object s, scr, t;
   int items;
   Field text_glyph;
   Array pos, colors=NULL;
  
   /*delta = DXRGB(0.0,0.0,0.0);*/

   /* get font and text glyph */
   font =DXGetFont(fontname, NULL, NULL);
   if (!font)  
     goto error;
   /* text glyphs are 15 pixels high, scaled by whatever scale is */
   scale = DXScale(15.0*scalefactor, 15.0*scalefactor, 15.0*scalefactor); 
   text_glyph = (Field) DXGeometricText(string, font, NULL);
   if (!text_glyph)
      goto error;
   /* I need to know how many positions there are to make the colors
      component */
   pos = (Array)DXGetComponentValue(text_glyph,"positions");
   if (!pos) {
     DXSetError(ERROR_INTERNAL,"no positions for text object");
     goto error;
   }
   DXGetArrayInfo(pos,&items,NULL,NULL,NULL,NULL);
   colors = (Array)DXNewConstantArray(items, (Pointer)&color,
                                      TYPE_FLOAT, CATEGORY_REAL, 1, 3);

   if (!colors) goto error;
   if (!DXSetComponentValue(text_glyph,"colors",(Object)colors))
     goto error;
   colors = NULL;
   
   
   /* must apply scaling before object becomes a screen object */
   s = (Object) DXNewXform((Object) text_glyph, scale);
   if (!s) {
     DXDelete((Object) text_glyph);
     goto error;
   }
   
   scr = (Object) DXNewScreen(s, 0, 1);
   if (!scr) {
     DXDelete((Object) text_glyph);
     goto error;
   }

   /* must apply translation after object becomes a screen object */
   t = (Object) DXNewXform(scr, translation);
   if (!t) {
     DXDelete(scr);
     goto error;
   }
   
   DXSetMember((Group) object, NULL, t);
   DXDelete((Object)font);
   
   return OK;
error:
   DXDelete((Object)colors);
   DXDelete((Object)font);
   return ERROR;
   
}


static int DivideByZero(float nmrtr, float dnmntr)
{
#define TRUE                    1
#define FALSE                   0
  
  /* obvious case */
  if (dnmntr == 0.0) return TRUE;
  
  /* take absolute value to make comparisons easier */
  nmrtr = fabs(nmrtr);
  dnmntr = fabs(dnmntr);
  
  /* number always gets smaller if denominator greater than one */
  if (dnmntr >= 1.0) return FALSE;
  
  /* number always get smaller if denominator greater than numerator */
  if (dnmntr >= nmrtr) return FALSE;
  
  /* we made it this far, check ratio */
  if (nmrtr >= dnmntr * DXD_MAX_FLOAT) 
    return TRUE;
  else
    return FALSE;
  
}


/* this routine gets the data stats for an object */
 
Error 
_dxfGetStatisticsAndSize(Object outo, char *type_string, int min_set, 
  		           float *given_min, int max_set, float *given_max,
		           float *base_size, int *stats_done)
{
  float min, max;
  
 
  /* XXX I need to check if the data component is tensor. If it is, I should
   * figure out the statistics on the three eigenvectors */

  if (_dxfDXEmptyObject(outo)) {
        *base_size=0;
        return OK;
  }
  if ((!strcmp(type_string,"tensor"))||(_dxfIsTensor(outo))) {
     if (!_dxfGetTensorStats(outo, &min, &max)) {
        return ERROR;
     }
     if (!min_set) *given_min = min;
     if (!max_set) *given_max = max;
  }
  else { 
  if (!DXStatistics(outo, "data", &min, &max, NULL, NULL)) {
    /* make sure it has a data component */
    if (!_dxfHasDataComponent(outo)) {
    if (!strcmp(type_string,"text")) {
      DXSetError(ERROR_DATA_INVALID,
	       "field must have a data component for text glyphs"); 
      return ERROR;
    }
    else {
      /* it's ok if it doesn't have data, but not if they've specified a
       * non-scalar glyph type */
      DXResetError();
      if ((!strcmp(type_string, "rocket"))||
          (!strcmp(type_string, "arrow"))||
          (!strcmp(type_string, "needle"))||
          (!strcmp(type_string, "tensor"))||
          (!strcmp(type_string, "arrow2d"))||
          (!strcmp(type_string, "needle2d"))||
          (!strcmp(type_string, "rocket2d"))) {
       DXSetError(ERROR_DATA_INVALID,
        "only scalar glyph types accepted when no data component is present");
       return ERROR;
      }
      if (!min_set) *given_min = 1.0;
      if (!max_set) *given_max = 1.0;
    }
  }
  DXResetError();
  }
  else {
    if (!min_set) *given_min = min;
    if (!max_set) *given_max = max;
  }
  }

  if (!_dxfWidthHeuristic(outo, base_size))
    return ERROR;

  if (stats_done) *stats_done = 1;  
  return OK;
  
}


/*
 * This routine will "recurse to individual", that is until a series or
 * composite field is reached
 */ 

/* the point of this is to find the level of the "individual" and get
 * statistics at that point */

Error
_dxfProcessGlyphObject(Object parent,
                         Object child, 
                         char *type_string, 
                         float shape, 
	                 float scale, 
                         int scale_set, 
                         float ratio, 
                         int ratio_set, 
	                 float given_min, 
                         int min_set, 
                         float given_max, 
                         int max_set, 
	                 float quality, 
                         int AutoSize, 
                         Object typefield, 
                         int type_is_field, 
                         int index, 
                         char *font,
                         int *stats_done,
                         float *base_size)
{ 
  struct arg arg;
  Object subo;
  Group newgroup=NULL;
  Field copyfield=NULL;
  int i;
  Array data;
  Type type;
  struct textarg textarg;
  
  switch(DXGetObjectClass(child)) {
    
  case CLASS_FIELD:
    /* if empty field just return it */
    if (DXEmptyField((Field)child))  {
      return OK;
    }

    /* it's a field. If it's text, don't do statistics stuff; just
       call the text processor */
    data = (Array)DXGetComponentValue((Field)child,"data");
    if ((data) && (!DXGetArrayInfo(data, NULL, &type, NULL, NULL, NULL)))
       goto error;

    if ((data) &&  ((type == TYPE_STRING) ||
        (!strcmp(type_string,"text")) || 
        (!strcmp(type_string,"coloredtext")))) {

      /* first make a new group which will replace the field */
      newgroup = DXNewGroup();
      if (!newgroup) goto error;

      /* make a copy of the field to work on */
      copyfield = (Field)DXCopy(child,COPY_STRUCTURE);

      /* set the new group as a member of parent */
      if (!DXSetEnumeratedMember((Group)parent, index, (Object)newgroup))
        goto error;

      textarg.inobject = (Object)copyfield;
      textarg.outobject = (Object)newgroup; 

      if (!strcmp(type_string,"coloredtext"))
         textarg.colored = 1;
      else
         textarg.colored = 0;
      textarg.scale = scale;
      strcpy(textarg.font,font); 
      if (!(DXAddTask(_dxfTextField, (Pointer)&textarg, sizeof(textarg), 
                      1.0)))
         goto error;
    }
    /* else it's a field, but it's not text stuff */
    else {
      if (!*stats_done) {
        if (!_dxfGetStatisticsAndSize(child, type_string, min_set, &given_min, 
     			              max_set, &given_max, base_size,
                                      NULL))
          goto error;
      } 
    
      arg.outobject = child;
      strcpy(arg.type, type_string);
      arg.typefield = typefield;
      arg.type_is_field = type_is_field;
      arg.shape = shape;
      arg.scale = scale;
      arg.scale_set = scale_set;
      arg.ratio = ratio;
      arg.ratio_set = ratio_set;
      arg.given_min = given_min;
      arg.min_set = min_set;
      arg.given_max = given_max;
      arg.max_set = max_set;
      arg.quality = quality;
      arg.base_size = *base_size;
      arg.AutoSize = AutoSize;
      arg.parent = parent;
      arg.parentindex = index;
    
      /* call GlyphTask */
      if (!(DXAddTask(_dxfGlyphTask, (Pointer)&arg, sizeof(arg), 1.0)))
        goto error;
    
      return OK;
   }
   break;
    
  case CLASS_GROUP:
    if (DXGetGroupClass((Group)child) == CLASS_COMPOSITEFIELD ||
	DXGetGroupClass((Group)child) == CLASS_SERIES ||
        DXGetGroupClass((Group)child) == CLASS_MULTIGRID) {
      /* We're ready to extract the min and max of this thing */
      if (!_dxfGetStatisticsAndSize(child, type_string, min_set, &given_min, 
				    max_set, &given_max, base_size, 
                                    stats_done))
         goto error;

      for (i=0; (subo=DXGetEnumeratedMember((Group)child,i,NULL)); i++) {
	if (!_dxfProcessGlyphObject(child, subo, type_string, shape, scale, 
			        scale_set, ratio, ratio_set, given_min, 
			        min_set, given_max, max_set, quality, 
                                AutoSize,
                                typefield, type_is_field, i, font, stats_done,
                                base_size))
	  goto error;
      }
    }
    else {
      /* must be a generic group, so we are not ready to get statistics */
      for (i=0; (subo=DXGetEnumeratedMember((Group)child,i,NULL)); i++) {
	if (!_dxfProcessGlyphObject(child, subo, type_string, shape, scale, 
			        scale_set, ratio, ratio_set, given_min, 
			        min_set, given_max, max_set, quality, 
                                AutoSize,
                                typefield, type_is_field, i, font, 
                                stats_done, base_size))
	  goto error;
      }
    }
    break;


    case (CLASS_CLIPPED):
      if (!(DXGetClippedInfo((Clipped)child, &subo, NULL)))
        goto error;
      if (!_dxfProcessGlyphObject(child, subo, type_string, shape, scale, 
		              scale_set, ratio, ratio_set, given_min, 
		              min_set, given_max, max_set, quality, AutoSize,
                              typefield, type_is_field, 0, font, stats_done,
                              base_size))
        goto error;
      break;


    default:
      DXSetError (ERROR_BAD_PARAMETER,"group or field required");
      goto error;
      
  }

  return OK;
error:
  DXDelete((Object)newgroup);
  DXDelete((Object)copyfield);
  return ERROR;
}







static 
Error
  ExpandComponents(Field field, char *dataattr, int newpoints, int ng, int *map, char *fexcept)
{
  Array sA, dA=NULL;
  int i=0;
  char *name;
 

    while (NULL != (sA = (Array)DXGetEnumeratedComponentValue(field, i++, &name))
)
    {
        int itemSize;
        Type type;
        Category cat;
        int rank, shape[30];
        Object attr = DXGetComponentAttribute(field, name, "dep");

        dA = NULL;

        /* 
         * don't do anything with the normals component; it's already correct 
         */
        if (!strcmp(name,"normals"))
            continue;
        /* ditto for connections */
        if (!strcmp(name,"connections"))
            continue;
        /*
         * since positions "dep" positions, we must ignore that too
         */
        if (!strcmp(name,"positions"))
            continue;

        if (! attr)
            continue;

        /* check dependency. Delete the component */
        if (strcmp(DXGetString((String)attr), dataattr)) {
             DXDeleteComponent(field, name);
             /* need to decrement i, because we've deleted a component */
             i--;
             continue;
        }
        /* check the "fexcept" string */
        if (!strcmp(name,fexcept))
           continue; 


        DXGetArrayInfo(sA, NULL, &type, &cat, &rank, shape);
        itemSize = DXGetItemSize(sA);
        if (DXQueryConstantArray((Array)sA, NULL, NULL)) 
        {
           int rank, shape[30];
           Pointer origin=NULL;
           Type type;
           Category category;
 

           DXGetArrayInfo(sA, NULL, &type, &category, &rank, shape);
           origin   = DXAllocateLocal(itemSize);
           if (origin == NULL)
           {
               DXFree(origin);
               goto cleanup;
           }

           if (DXQueryConstantArray((Array)sA, NULL, origin)) 
           dA = (Array)DXNewConstantArrayV(newpoints, origin, type, 
                                                category, rank, shape);

            DXFree(origin);
        }

        if (! dA)
        {
            int i, j;
            char *srcPtr;
            char *dstPtr;

            dA = DXNewArrayV(type, cat, rank, shape);
            if (! dA)
                goto cleanup;

            if (! DXAddArrayData(dA, 0, newpoints, NULL))
                goto cleanup;

            dstPtr = (char *)DXGetArrayData(dA);
            srcPtr = (char *)DXGetArrayData(sA);


            for (i = 0; i < ng; i++)
            {
                for (j = 0; j < map[i]; j++)
                {
                    memcpy(dstPtr, srcPtr, itemSize);
                    dstPtr += itemSize;
                }
                srcPtr += itemSize;
            }
        }

        if (! DXSetComponentValue(field, name, (Object)dA))
            goto cleanup;
        dA = NULL;

        if (! DXSetComponentAttribute(field, name, "dep",
                                        (Object)DXNewString("positions")))
            goto cleanup;
        DXChangedComponentValues((Field)field,name);

    }
    
    DXDelete((Object) dA);
    return OK;

cleanup:
       DXDelete((Object) dA);
       return ERROR;
}




Error
  _dxfGlyphTask(Pointer p)
{
  Object outo;
  RGBColor origin;
  int data_incr=0, pos_incr, totalpoints = 0, compact=0, numitems; 
  int hasnormals, out_pos_dim, invalid, valid;
  int num_to_be_overridden=0, num_not_drawn=0;
  InvalidComponentHandle invalidhandle=NULL;
  Array data_array, positions_array=NULL, new_positions_array, colors_array;
  Array invalid_pos;
  ArrayHandle handle=NULL;
  Pointer scratch=NULL;
  RGBColor defaultcolor;
  int c, *map = NULL;
  Matrix overridescalematrix;
  Array new_normals_array=NULL, new_connections_array;
  Triangle *conns=NULL, *newconns, *overrideconns, *newoverrideconns;
  Point *normals=NULL, *newnormals, *points=NULL;
  float *newpoints, *newoverridepoints;
  Point *overridepoints, *overridenormals;
  Point *newoverridenormals;
  Line *newlines, *lines, *overridelines, *newoverridelines;
  Matrix scalematrix, translatematrix, rotatematrix, t;
  char *given_type_string, glyph_rank[10], type_string[30];
  char connectiontype[30];
  char overridetype_string[30];
  float glyphshape, scale, ratio, given_min, given_max, quality, base_size;
  float min;
  int ratio_set;
  int data_rank, data_shape[30];
  int pos_rank, pos_shape[30], counts[30];
  int numpoints, numoverridepoints=0, numoverrideconns=0;
  int numconns, num_pos, i, j, n, countpos=0, countconn=0;
  float vector_width; 
  float *dp_old=NULL, datavalue=0, lv; 
  float datascale, range, x, posorigin[30], posdeltas[30];
  float *data_ptr_f=NULL;
  byte *data_ptr_b=NULL;
  ubyte *data_ptr_ub=NULL;
  short *data_ptr_s=NULL;
  ushort *data_ptr_us=NULL;
  int *data_ptr_i=NULL;
  uint *data_ptr_ui=NULL;
  double *data_ptr_d=NULL;
  Stress_Tensor st;
  Stress_Tensor2 st2;
  Vector eigen_vector[3];
  int AutoSize, num_data, posted;
  Vector v, canonical_v;
  Type pos_type, data_type;
  Category pos_category, data_category;
  Object typefield;
  int type_is_field, anyxforms;
  Matrix xform;
  char *attr;
  Object uniformscalingattr=NULL;

  Arg *arg = (Arg *)p;


  newnormals = NULL;
  newlines = NULL;
  newconns = NULL;
  newpoints = NULL;
  newoverridepoints = NULL;
  newoverridelines = NULL;
  newoverridenormals = NULL;
  newoverrideconns = NULL;
  

  outo = arg->outobject;
  given_type_string = arg->type;
  glyphshape = arg->shape;
  scale = arg->scale;
  /*scale_set = arg->scale_set;*/
  ratio = arg->ratio;
  ratio_set = arg->ratio_set;
  given_min = arg->given_min;
  /*min_set = arg->min_set;*/
  given_max = arg->given_max;
  /*max_set = arg->max_set;*/
  quality = arg->quality;
  base_size = arg->base_size;
  AutoSize = arg->AutoSize;
  typefield = arg->typefield;
  type_is_field = arg->type_is_field;
  /*parent = arg->parent;*/
  /*parentindex = arg->parentindex;*/
  /*font = arg->font;*/
  
  /* we have a field and are ready to put glyphs in the field */
  
  /* first figure out what kind of glyphs we have got to do */
  /* get the data component of the input field */
  
  data_array = (Array)DXGetComponentValue((Field)outo,"data");
  
  if (!data_array) {
    strcpy(glyph_rank,"scalar");
    /* since there's no data to follow, are there colors? If so, follow
     * those. Otherwise, follow positions */
    if (DXGetComponentValue((Field)outo,"colors")) {
      attr = DXGetString((String)DXGetComponentAttribute((Field)outo,"colors",
							 "dep"));
    }
    else {
      attr = "positions";
    }
  }
  else {
    if (!DXGetArrayInfo(data_array,&num_data, &data_type, &data_category, 
			&data_rank, data_shape))
      return ERROR;
    attr = DXGetString((String)DXGetComponentAttribute((Field)outo,
						       "data", "dep"));
    if (data_rank==0) {
      strcpy(glyph_rank,"scalar");
      data_incr = 1;
    }
    else if (data_rank==1 && data_shape[0] == 1) {
      strcpy(glyph_rank,"scalar");
      data_incr = 1;
    }
    else if (data_rank==1 && data_shape[0] == 2) {
      strcpy(glyph_rank,"2-vector");
      data_incr = 2;
    }
    else if (data_rank==1 && data_shape[0] == 3) {
      strcpy(glyph_rank,"3-vector");
      data_incr = 3;
    }
    else if (data_rank==2 && data_shape[0]==3 &&data_shape[1]==3) {
      strcpy(glyph_rank,"matrix");
      data_incr = 9;
    }
    else if (data_rank==2 && data_shape[0]==2 &&data_shape[1]==2) {
      strcpy(glyph_rank,"matrix2");
      data_incr = 4;
    }
    else {
      DXSetError(ERROR_DATA_INVALID,
		 "data must be scalar, 2-vector, 3-vector, 2x2 or 3x3 matrix");
      return ERROR;
    }
  }
  
  
  posted = 0;

  /* check the attribute */
  if (strcmp(attr,"connections") && (strcmp(attr,"positions"))) {
     DXSetError(ERROR_DATA_INVALID,"unsupported data dependency \'%s\'",attr);
     goto error;
  }


  if (!strcmp(attr,"connections")) {
    /* I need to check whether the positions are regular. If they
     * are, a simple grid shift is in order. Otherwise, use PostArray
     * to shift them */
    Array pos, con;
    pos = (Array)DXGetComponentValue((Field)outo,"positions");
    con = (Array)DXGetComponentValue((Field)outo,"connections");
    if ((DXQueryGridPositions(pos, &n, counts, posorigin, posdeltas)) &&
	DXQueryGridConnections(con, NULL, NULL)) {
      
      for (i=0; i<n; i++) {
	for (j=0; j<n; j++) {
	  if (counts[j] > 1) {
	    posorigin[i]+=0.5*posdeltas[i + j*n];
	  }
	}
      }
      for (i=0; i<n; i++) {
	if (counts[i] >1) counts[i] = counts[i]-1;
      }
      
      positions_array = DXMakeGridPositionsV(n, counts, posorigin, posdeltas); 
      
    }
    /* else the positions are irregular */
    else {
      positions_array = _dxfPostArray((Field)outo, "positions","connections");
    }
    if (!positions_array) {
      DXSetError(ERROR_BAD_PARAMETER,
		 "could not compute glyph positions for cell-centered data");
      return ERROR;
    }
    posted = 1;
    if (!DXGetArrayInfo(positions_array,&num_pos, &pos_type, &pos_category, 
			&pos_rank, pos_shape))
      return ERROR;
  }

  else if (!strcmp(attr, "faces")) {
      positions_array = _dxfPostArray((Field)outo, "positions","faces");
      if (!positions_array) {
        DXAddMessage("could not compute glyph positions for face-centered data");
        return ERROR;
      }
      posted = 1;
      if (!DXGetArrayInfo(positions_array,&num_pos, &pos_type, &pos_category, 
  			&pos_rank, pos_shape))
        return ERROR;
  }
 
  /* else dep positions */
  else {
    positions_array = (Array)DXGetComponentValue((Field)outo,"positions");
    if (!positions_array) {
      DXSetError(ERROR_BAD_PARAMETER,"field has no positions");
      return ERROR;
    }
    if (!DXGetArrayInfo(positions_array,&num_pos, &pos_type, &pos_category, 
			&pos_rank, pos_shape))
      return ERROR;
  }


  invalid=0;
  /* see if there's an invalid positions (or connections) array  */
  if (!strcmp(attr,"connections")) {
    invalid_pos = (Array)DXGetComponentValue((Field)outo,
                                                "invalid connections");
    if (invalid_pos) {
      DXGetArrayInfo(positions_array, &numitems, NULL, NULL, NULL, NULL); 
      invalidhandle = DXCreateInvalidComponentHandle((Object)outo,
                                                     NULL, "connections");
      if (!invalidhandle)
        goto error;
      invalid=1;
      /* if all invalid, just return. We are done */
      valid=0;
      for (i=0; i<numitems; i++) {
         if (DXIsElementValidSequential(invalidhandle, i)) {
            valid = 1;
            break;
         }
      }
      if (!valid) {
          Array temparray;
	  defaultcolor = DEFAULT_SCALAR_COLOR;
          temparray = (Array)DXNewConstantArray(numitems, 
                                  &defaultcolor, TYPE_FLOAT,
                                  CATEGORY_REAL, 1, 3);
          if (!temparray)
             goto error;
          DXSetComponentValue((Field)outo,"colors",(Object)temparray);
          DXSetComponentAttribute((Field)outo,"colors","dep", 
                                  (Object)DXNewString(attr));
          return OK;
      }
            
    }
  }
  else {
    invalid_pos = (Array)DXGetComponentValue((Field)outo,
                                                "invalid positions");
    if (invalid_pos) {
      invalidhandle = DXCreateInvalidComponentHandle((Object)outo,
                                                     NULL, "positions");
      DXGetArrayInfo(positions_array, &numitems, NULL, NULL, NULL, NULL); 
      if (!invalidhandle)
        goto error;
      invalid=1;
      /* if all invalid, just return. We are done */
      valid=0;
      for (i=0; i<numitems; i++) {
         if (DXIsElementValidSequential(invalidhandle, i)) {
            valid = 1;
            break;
         }
      }
      if (!valid) {
          Array temparray;
	  defaultcolor = DEFAULT_SCALAR_COLOR;
          temparray = (Array)DXNewConstantArray(numitems, 
                                  &defaultcolor, TYPE_FLOAT,
                                  CATEGORY_REAL, 1, 3);
          if (!temparray)
             goto error;
          DXSetComponentValue((Field)outo,"colors",(Object)temparray);
          DXSetComponentAttribute((Field)outo,"colors","dep", 
                                  (Object)DXNewString(attr));
          return OK;
         return OK;
      }
    }
  }
  
  
  if ((pos_shape[0] < 1)||(pos_shape[0] > 3)) {
    DXSetError(ERROR_DATA_INVALID,
	       "positions must be 1, 2, or 3-Dimensional");
    return ERROR;
  }
  pos_incr = pos_shape[0];

  /* prepare to access the positions component */
  if (!(handle = DXCreateArrayHandle(positions_array)))
     goto error;
  scratch = DXAllocateLocal(pos_incr*sizeof(float));
  if (!scratch)
     goto error;



  
  if (!ratio_set) {
    if (!strcmp(glyph_rank,"scalar")) 
      ratio = 0.05F;
    else 
      ratio = 0.0F;
  }
  
  if (!type_is_field) {
    if (!GetGlyphName(quality, glyph_rank, pos_shape[0], given_type_string, 
                      type_string, overridetype_string, &out_pos_dim))
      goto error;
  }
  
  /* now for whatever type_string it is, find out how many positions
     and connections will be added for each glyph, and get the arrays
     of positions and connections */ 

  /* normals are a special case. Everything else gets expanded into the
     glyphs */
  if (DXGetComponentValue((Field)outo,"normals")) 
    DXRemove(outo,"normals");

  /* set the shade attribute to yes */
  DXSetIntegerAttribute((Object)outo, "shade", 1);
  
  if (type_is_field) {
    xform = Identity;
    anyxforms = 0;
    if (!GetGlyphField(typefield, &numpoints, &numconns,
		       &points, &conns, &lines, &normals, &xform, &anyxforms, 
		       &hasnormals, &out_pos_dim, connectiontype))
      goto error;
    
    if (!strcmp(connectiontype,"triangles")) {
      newconns = (Triangle *)DXAllocateLocal((numconns)*(3*sizeof(int)));
      if (!newconns) {
        goto error;
      }
      if (strcmp(glyph_rank,"scalar")) {
	if ((pos_shape[0]==3)&&(hasnormals))
	  strcpy(overridetype_string,"sphere62");
	else
	  strcpy(overridetype_string,"circle10");
	if (!GetGlyphs(overridetype_string, &numoverridepoints,
		       &numoverrideconns,
		       &overridepoints, &overrideconns, &overridenormals,
		       &overridelines, &hasnormals, connectiontype))
	  goto error;
	newoverridepoints = 
	  (float *)DXAllocateLocal((numoverridepoints)*(out_pos_dim*sizeof(float)));
	if (!(newoverridepoints))
	  goto error;
	if (hasnormals) {
	  newoverridenormals = 
	    (Point *)DXAllocateLocal((numoverridepoints)*(3*sizeof(float)));
	  if (!(newoverridenormals)) {
	    goto error;
	  }
	}
	newoverrideconns = 
	  (Triangle *)DXAllocateLocal((numoverrideconns)*(3*sizeof(int)));
        if (!(newoverrideconns)) {
          goto error;
        }
      }
    }
    else {
      newlines = (Line *)DXAllocateLocal((numconns)*(2*sizeof(int)));
      if (!newlines) {
        goto error;
      }
      if (strcmp(glyph_rank,"scalar")) {
	strcpy(overridetype_string,"point");
	if (!GetGlyphs(overridetype_string, &numoverridepoints,
                       &numoverrideconns,
		       &overridepoints, &overrideconns, &overridenormals,
		       &overridelines, &hasnormals, connectiontype))
	  goto error;
	newoverridepoints = 
	  (float *)DXAllocateLocal((numoverridepoints)*(out_pos_dim*sizeof(float)));
	if (!(newoverridepoints))
	  goto error;
	if (hasnormals) {
	  newoverridenormals = 
	    (Point *)DXAllocateLocal((numoverridepoints)*(3*sizeof(float)));
	  if (!(newoverridenormals)) {
	    goto error;
	  }
	}
	newoverridelines = 
	  (Line *)DXAllocateLocal((numoverrideconns)*(2*sizeof(int)));
        if (!(newoverridelines)) {
          goto error;
        }
      }
    }
    
    newpoints = (float *)DXAllocateLocal((numpoints)*(3*sizeof(float)));
    if (!newpoints)
      goto error;
    if (hasnormals) {
      newnormals = (Point *)DXAllocateLocal((numpoints)*(3*sizeof(float)));
      if (!newnormals) {
	goto error;
      }
    }
    /* check for an attribute on the given glyph telling us
     * not to futz with the shape of the glyph. Just use it
     * as is, scaling length and width identically 
     */
    uniformscalingattr = (Object)DXGetAttribute(typefield,"uniformscale");
  }
  else {
    if (!GetGlyphs(type_string, &numpoints, &numconns, &points, 
                   &conns, &normals, &lines, &hasnormals, connectiontype))
      goto error;
    newpoints = (float *)DXAllocateLocal((numpoints)*(out_pos_dim*sizeof(float)));
    if (!(newpoints))
      goto error;
    if (hasnormals) {
      newnormals = (Point *)DXAllocateLocal((numpoints)*(3*sizeof(float)));
      if (!(newnormals)) {
        goto error; 
      }
    }
    if (!strcmp(connectiontype,"triangles")) {
      newconns = 
        (Triangle *)DXAllocateLocal((numconns)*(3*sizeof(int)));
      if (!(newconns)) {
        goto error;
      }
    }
    else {
      newlines = (Line *)DXAllocateLocal((numconns)*(2*sizeof(int)));
      if (!(newlines)) {
        goto error;
      }
    }
    /* for non-scalar glyphs, we may also need an "override" glyph (sphere) */
    if (strcmp(glyph_rank,"scalar")) {
      if (!GetGlyphs(overridetype_string, &numoverridepoints,&numoverrideconns,
                     &overridepoints, &overrideconns, &overridenormals,
                     &overridelines, &hasnormals, connectiontype))
        goto error;
      newoverridepoints = 
	(float *)DXAllocateLocal((numoverridepoints)*(out_pos_dim*sizeof(float)));
      if (!(newoverridepoints))
        goto error;
      if (hasnormals) {
        newoverridenormals = 
  	  (Point *)DXAllocateLocal((numoverridepoints)*(3*sizeof(float)));
        if (!(newoverridenormals)) {
	  goto error;
        }
      }
      if (!strcmp(connectiontype,"triangles")) {
        newoverrideconns = 
          (Triangle *)DXAllocateLocal((numoverrideconns)*(3*sizeof(int)));
        if (!(newoverrideconns)) {
          goto error;
        }
      }
      else {
        newoverridelines = 
	  (Line *)DXAllocateLocal((numoverrideconns)*(2*sizeof(int)));
        if (!(newoverridelines)) {
          goto error;
        }
      }
    }
  }


  map = (int *)DXAllocateLocalZero(num_pos*sizeof(int));
  if ((pos_type != TYPE_FLOAT)||(pos_category != CATEGORY_REAL)||
      (pos_rank != 1)) {
    DXSetError(ERROR_DATA_INVALID,
	       "positions component must type float, category real, rank 1");
    goto error;
  }
  
  
  /* get the data component */
  data_array = (Array)DXGetComponentValue((Field)outo,"data");
  if (data_array) {
    if (data_category != CATEGORY_REAL) {
      DXSetError(ERROR_DATA_INVALID,
		 "data component must be category real");
      goto error;
    }
    switch (data_type) {
    case (TYPE_SHORT):
      data_ptr_s = (short *)DXGetArrayData(data_array);
      break;
    case (TYPE_INT):
      data_ptr_i = (int *)DXGetArrayData(data_array);
      break;
    case (TYPE_FLOAT):
      data_ptr_f = (float *)DXGetArrayData(data_array);
      break;
    case (TYPE_DOUBLE):
      data_ptr_d = (double *)DXGetArrayData(data_array);
      break;
    case (TYPE_BYTE):
      data_ptr_b = (byte *)DXGetArrayData(data_array);
      break;
    case (TYPE_UBYTE):
      data_ptr_ub = (ubyte *)DXGetArrayData(data_array);
      break;
    case (TYPE_USHORT):
      data_ptr_us = (ushort *)DXGetArrayData(data_array);
      break;
    case (TYPE_UINT):
      data_ptr_ui = (uint *)DXGetArrayData(data_array);
      break;
    default:
      break;
    } 
  }
  
  /* get the colors component */
  colors_array = (Array)DXGetComponentValue((Field)outo,"colors"); 
  if (colors_array) c = 1;
  else c=0; 

  /* check the dependency of the colors vs the dependency of the data.
   * If they don't match, pretend there aren't any colors */
  if (colors_array) {
     if (data_array) {
       if (strcmp(DXGetString((String)DXGetAttribute((Object)colors_array,
                                                               "dep")), 
                  DXGetString((String)DXGetAttribute((Object)data_array,
                                                             "dep")))) {
          c = 0;
          colors_array=NULL;
      }
     }
  }
  
  
  if (!colors_array) {
    /* make a temporary colors component to expand later */
    /*delta = DXRGB(0.0, 0.0, 0.0);*/
    if (!strcmp(glyph_rank,"2-vector")||(!strcmp(glyph_rank,"3-vector"))) {
      origin = DEFAULT_VECTOR_COLOR;
      colors_array = (Array)DXNewConstantArray(num_pos, (Pointer)&origin,
                                               TYPE_FLOAT,CATEGORY_REAL,
                                               1, 3);
      DXSetComponentValue((Field)outo,"colors",(Object)colors_array);
      /* make the colors dep on whatever the data was dep on */
      DXSetComponentAttribute((Field)outo,"colors","dep", 
                              (Object)DXNewString(attr));
    }
    if (!strcmp(glyph_rank,"matrix")) {
      origin = DEFAULT_TENSOR_COLOR;
      colors_array = (Array)DXNewConstantArray(num_pos, (Pointer)&origin,
                                               TYPE_FLOAT,CATEGORY_REAL,
                                               1, 3);
      DXSetComponentValue((Field)outo,"colors",(Object)colors_array);
      /* make the colors dep on whatever the data was dep on */
      DXSetComponentAttribute((Field)outo,"colors","dep", 
                              (Object)DXNewString(attr));
    }
    if (!strcmp(glyph_rank,"matrix2")) {
      origin = DEFAULT_TENSOR_COLOR;
      colors_array = (Array)DXNewConstantArray(num_pos, (Pointer)&origin,
                                               TYPE_FLOAT,CATEGORY_REAL,
                                               1, 3);
      DXSetComponentValue((Field)outo,"colors",(Object)colors_array);
      /* make the colors dep on whatever the data was dep on */
      DXSetComponentAttribute((Field)outo,"colors","dep", 
                              (Object)DXNewString(attr));
    }
    if (!strcmp(glyph_rank,"scalar")) {
      if (data_array) {
	if (!DXStatistics(outo,"data",&min, NULL,NULL, NULL))
	  goto error;
	if (given_min <= min) {
	  compact = 1;
	  origin = DEFAULT_SCALAR_COLOR;
          colors_array = (Array)DXNewConstantArray(num_pos, (Pointer)&origin,
                                                   TYPE_FLOAT, CATEGORY_REAL,
                                                   1, 3);
	  DXSetComponentValue((Field)outo,"colors",(Object)colors_array);
	  /* make the colors dep on whatever the data was dep on */
	  DXSetComponentAttribute((Field)outo,"colors","dep", 
                                  (Object)DXNewString(attr));
	}
	else {
	  compact = 0;
	  colors_array = DXNewArray(TYPE_FLOAT,CATEGORY_REAL, 1, 3);
	  DXSetComponentValue((Field)outo,"colors",(Object)colors_array);
	  /* make the colors dep on whatever the data was dep on */
	  DXSetComponentAttribute((Field)outo,"colors","dep", 
                                  (Object)DXNewString(attr));
	}
      }
      else {
	compact = 1;
	origin = DEFAULT_SCALAR_COLOR;
        colors_array = (Array)DXNewConstantArray(num_pos, (Pointer)&origin,
                                                 TYPE_FLOAT, CATEGORY_REAL,
                                                 1, 3);
	DXSetComponentValue((Field)outo,"colors",(Object)colors_array);
	/* make the colors dep on whatever the data was dep on */
	DXSetComponentAttribute((Field)outo,"colors","dep", 
				(Object)DXNewString(attr));
      }
    } 
  }
  
  /* now make a new output positions component */
  new_positions_array = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, out_pos_dim);
  
  /* now make a new output connections component */
  /* need to do different things for needles and arrows, (not triangles) */
  if ( !strcmp(connectiontype,"lines")) {
    new_connections_array = DXNewArray(TYPE_INT, CATEGORY_REAL, 1, 2);
    DXSetComponentValue((Field)outo, "connections", 
			(Object)new_connections_array);
    DXSetComponentAttribute((Field)outo,"connections","ref",
			    (Object)DXNewString("positions"));
    DXSetComponentAttribute((Field)outo,"connections","element type",
			    (Object)DXNewString("lines"));
    new_normals_array = NULL;
  }
  else {
    new_connections_array = DXNewArray(TYPE_INT, CATEGORY_REAL, 1, 3);
    DXSetComponentValue((Field)outo, "connections", 
			(Object)new_connections_array);
    DXSetComponentAttribute((Field)outo,"connections","ref",
			    (Object)DXNewString("positions"));
    DXSetComponentAttribute((Field)outo,"connections","element type",
			    (Object)DXNewString("triangles"));
    
    /* now make a new output normals component */
    /* normals only apply to triangle connections */
    if (hasnormals) {
      new_normals_array = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, 3);
      DXSetComponentValue((Field)outo, "normals", (Object)new_normals_array);
      DXSetComponentAttribute((Field)outo,"normals","dep",
			      (Object)DXNewString("positions"));
    }
  }
  
 
  /* first figure out how many override glyphs there are going to be */
  /* only matters for vector data */ 
  num_to_be_overridden=0;
  num_not_drawn=0;
  if ((!strcmp(glyph_rank,"3-vector"))||(!strcmp(glyph_rank,"2-vector"))){
    for (i=0; i<num_pos; i++) {
      /* vector size formulas */
      if (!strcmp(glyph_rank,"3-vector")) {
	switch (data_type) {
	case (TYPE_SHORT):
	  v = DXVec((float)data_ptr_s[i*3], 
                    (float)data_ptr_s[i*3+1], 
		    (float)data_ptr_s[i*3+2]);
	  break;
	case (TYPE_INT):
	  v = DXVec((float)data_ptr_i[i*3], 
                    (float)data_ptr_i[i*3+1], 
		    (float)data_ptr_i[i*3+2]);
	  break;
	case (TYPE_FLOAT):
	  v = DXVec(data_ptr_f[i*3], 
                    data_ptr_f[i*3+1], 
                    data_ptr_f[i*3+2]);
	  break;
	case (TYPE_DOUBLE):
	  v = DXVec((float)data_ptr_d[i*3], 
                    (float)data_ptr_d[i*3+1], 
		    (float)data_ptr_d[i*3+2]);
	  break;
	case (TYPE_BYTE):
	  v = DXVec((float)data_ptr_b[i*3], 
                    (float)data_ptr_b[i*3+1], 
		    (float)data_ptr_b[i*3+2]);
	  break;
	case (TYPE_UBYTE):
	  v = DXVec((float)data_ptr_ub[i*3], 
                    (float)data_ptr_ub[i*3+1], 
		    (float)data_ptr_ub[i*3+2]);
	  break;
	case (TYPE_USHORT):
	  v = DXVec((float)data_ptr_us[i*3], 
                    (float)data_ptr_us[i*3+1], 
		    (float)data_ptr_us[i*3+2]);
	  break;
	case (TYPE_UINT):
	  v = DXVec((float)data_ptr_ui[i*3], 
                    (float)data_ptr_ui[i*3+1], 
		    (float)data_ptr_ui[i*3+2]);
	  break;
	default:
	  break;
	}
      }
      else {
	switch (data_type) {
	case (TYPE_SHORT):
	  v = DXVec((float)data_ptr_s[i*2], 
                    (float)data_ptr_s[i*2+1], 0.0); 
	  break;
	case (TYPE_INT):
	  v = DXVec((float)data_ptr_i[i*2], 
                    (float)data_ptr_i[i*2+1], 0.0); 
	  break;
	case (TYPE_FLOAT):
	  v = DXVec(data_ptr_f[i*2], 
                    data_ptr_f[i*2+1], 0.0);
	  break;
	case (TYPE_DOUBLE):
	  v = DXVec((float)data_ptr_d[i*2], 
                    (float)data_ptr_d[i*2+1], 0.0); 
	  break;
	case (TYPE_BYTE):
	  v = DXVec((float)data_ptr_b[i*2], 
                    (float)data_ptr_b[i*2+1], 0.0 );
	  break;
	case (TYPE_UBYTE):
	  v = DXVec((float)data_ptr_ub[i*2], 
                    (float)data_ptr_ub[i*2+1], 0.0 );
	  break;
	case (TYPE_USHORT):
	  v = DXVec((float)data_ptr_us[i*2], 
                    (float)data_ptr_us[i*2+1], 0.0 );
	  break;
	case (TYPE_UINT):
	  v = DXVec((float)data_ptr_ui[i*2], 
                    (float)data_ptr_ui[i*2+1], 0.0 );
	  break;
	default:
	  break;
	}
      }
      lv = DXLength(v);
      if (invalid) {
        if (!DXIsElementInvalidSequential(invalidhandle, i)) {
          if (((lv <= ratio*given_max) || (lv < given_min) || 
	       ((given_min == 0.0) && (given_max == 0.0)))) {
	    num_to_be_overridden++;
          }
        } 
        else {
          num_not_drawn++;
        }
      }
      else {
        if (((lv <= ratio*given_max) || (lv < given_min) || 
	     ((given_min == 0.0) && (given_max == 0.0)))) {
	  num_to_be_overridden++;
        }
      }
    }
  }
  
  /* Pre-allocate the arrays for savings in time */
  /* I'm only going to allocate enough assuming no overrides */
  
  
  if (!DXAllocateArray(new_positions_array, 
		       (num_pos-num_to_be_overridden-num_not_drawn)*numpoints +
                       num_to_be_overridden*numoverridepoints))
    goto error;
  if (!DXAllocateArray(new_connections_array, 
                       (num_pos-num_to_be_overridden-num_not_drawn)*numconns +
		       num_to_be_overridden*numoverrideconns))
    goto error;
  if (hasnormals)
    if (!DXAllocateArray(new_normals_array, 
                         (num_pos-num_to_be_overridden-num_not_drawn)*numpoints+
			 num_to_be_overridden*numoverridepoints))
      goto error;
  
  
  range = given_max - given_min;
  
  canonical_v.x = 0.0;
  canonical_v.y = 1.0;
  canonical_v.z = 0.0;




  if (!strcmp(glyph_rank,"scalar")) {
    for (i=0; i<num_pos; i++) {
      if ((invalid) && (DXIsElementInvalidSequential(invalidhandle, i))) {
        map[i] = 0;
      } 
      else {
      if (data_array) {
        switch (data_type) {
	case (TYPE_SHORT):
	  datavalue = (float)data_ptr_s[0];
	  break;
	case (TYPE_INT):
	  datavalue = (float)data_ptr_i[0];
	  break;
	case (TYPE_FLOAT):
	  datavalue = (float)data_ptr_f[0];
	  break;
	case (TYPE_DOUBLE):
	  datavalue = (float)data_ptr_d[0];
	  break;
	case (TYPE_BYTE):
	  datavalue = (float)data_ptr_b[0];
	  break;
	case (TYPE_UBYTE):
	  datavalue = (float)data_ptr_ub[0];
	  break;
	case (TYPE_USHORT):
	  datavalue = (float)data_ptr_us[0];
	  break;
	case (TYPE_UINT):
	  datavalue = (float)data_ptr_ui[0];
	  break;
	default:
	  break;
	}
	x = (1 - ratio)*fabs(datavalue - given_min);
        if ((!compact)&&(!c)) {
	  if (datavalue < given_min) 
	    defaultcolor = OVERRIDE_SCALAR_COLOR;
          else 
	    defaultcolor = DEFAULT_SCALAR_COLOR;
          if (!DXAddArrayData(colors_array, 
			      i,1, (Pointer)&defaultcolor))
            goto error;
        }
	if (DivideByZero(x,range)) 
	  datascale = 1.0;
	else
	  datascale = x/range + ratio;
	
	if (AutoSize) {
	  datascale = scale * base_size * datascale;
        }
	else {
	  datascale = scale * datascale * given_max;
        }
      }
      else {
	/* XXX what if min and max are given for no data case ? */
        if (AutoSize)
	  datascale = scale * base_size;
        else
	  datascale = scale;
      }
      scalematrix = DXScale(datascale, datascale, datascale); 
      dp_old = DXGetArrayEntry(handle, i, scratch);
      if (pos_shape[0] == 1)                                              
	translatematrix = DXTranslate(DXPt(dp_old[0], 0.0, 0.0));            
      else if (pos_shape[0] == 2)                                        
	translatematrix = DXTranslate(DXPt(dp_old[0], dp_old[1], 0.0));
      else                                                               
	translatematrix = DXTranslate(DXPt(dp_old[0],                      
					   dp_old[1],                    
					   dp_old[2]));                  
      
      t = DXConcatenate(scalematrix, translatematrix); 
      if (out_pos_dim == 3) {
        if (!GlyphXform(t, numpoints, points, (Point *)newpoints))
            goto error;
      }
      else {
        if (!GlyphXform2d(t, numpoints, points, newpoints))
            goto error;
      }
      
      if (!DXAddArrayData(new_positions_array,countpos, 
			  numpoints, (Pointer)newpoints)) 
         goto error;
      
      if (hasnormals)  
        if (!DXAddArrayData(new_normals_array,countpos,numpoints,
			    (Pointer)normals))
         goto error;
        if (!strcmp(connectiontype,"triangles")) {
	  if (!IncrementConnections(countpos, numconns, 
				    conns, newconns))
           goto error;
	  if (!DXAddArrayData(new_connections_array,countconn,numconns,
			      (Pointer)newconns))
           goto error;
        }
        else {
	  if (!IncrementConnectionsLines(countpos, numconns, 
					 lines, newlines))
           goto error;
	  if (!DXAddArrayData(new_connections_array,countconn,numconns,
			      (Pointer)newlines))
           goto error;
        } 
      
      
      map[i] = numpoints;
      totalpoints += numpoints;
      countpos += numpoints;
      countconn += numconns;
    }
    if (data_array) {
      /* add here */
      switch (data_type) {
        case (TYPE_FLOAT):
          data_ptr_f += data_incr;
          break;
        case (TYPE_BYTE):
          data_ptr_b += data_incr;
          break;
        case (TYPE_SHORT):
          data_ptr_s += data_incr;
          break;
        case (TYPE_INT):
          data_ptr_i += data_incr;
          break;
        case (TYPE_DOUBLE):
          data_ptr_d += data_incr;
          break;
        case (TYPE_USHORT):
          data_ptr_us += data_incr;
          break;
        case (TYPE_UBYTE):
          data_ptr_ub += data_incr;
          break;
        case (TYPE_UINT):
          data_ptr_ui += data_incr;
          break;
        default:
	  break;
      }
    }
   }
  }
  else if ((!strcmp(glyph_rank,"3-vector"))||(!strcmp(glyph_rank,"2-vector"))){
    countpos = 0;
    countconn = 0;

    /* these factors make the glyphs look nice. Vector glyphs
     * always have the same width, regardless of length. The width
     * is chosen based on either the "base_size" of the grid, or
     * on the maximum data value
     */
    if (AutoSize) {
      datascale = scale * base_size * 0.07;
      vector_width = scale*glyphshape * base_size/4.0; 
    }
    else {
      datascale = scale * given_max * 0.07;
      vector_width = scale*glyphshape*given_max/16.0; 
    }
    /* make the override scale matrix only once, here */
    overridescalematrix = DXScale(datascale, datascale, datascale); 
    
    for (i=0; i<num_pos; i++) {
        if ((invalid) && (DXIsElementInvalidSequential(invalidhandle, i))) {
           map[i] = 0;
         }
        else {
	/* vector size formulas */
        if (!strcmp(glyph_rank,"3-vector")) {
	switch (data_type) {
	case (TYPE_SHORT):
	  v = DXVec((float)data_ptr_s[0], (float)data_ptr_s[1], 
		    (float)data_ptr_s[2]);
	  break;
	case (TYPE_INT):
	  v = DXVec((float)data_ptr_i[0], (float)data_ptr_i[1], 
		    (float)data_ptr_i[2]);
	  break;
	case (TYPE_FLOAT):
	  v = DXVec(data_ptr_f[0], data_ptr_f[1], data_ptr_f[2]);
	  break;
	case (TYPE_DOUBLE):
	  v = DXVec((float)data_ptr_d[0], (float)data_ptr_d[1], 
		    (float)data_ptr_d[2]);
	  break;
	case (TYPE_BYTE):
	  v = DXVec((float)data_ptr_b[0], (float)data_ptr_b[1], 
		    (float)data_ptr_b[2]);
	  break;
	case (TYPE_UBYTE):
	  v = DXVec((float)data_ptr_ub[0], (float)data_ptr_ub[1], 
		    (float)data_ptr_ub[2]);
	  break;
	case (TYPE_USHORT):
	  v = DXVec((float)data_ptr_us[0], (float)data_ptr_us[1], 
		    (float)data_ptr_us[2]);
	  break;
	case (TYPE_UINT):
	  v = DXVec((float)data_ptr_ui[0], (float)data_ptr_ui[1], 
		    (float)data_ptr_ui[2]);
	  break;
	default:
	  break;
	}
      }
      else {
	switch (data_type) {
	case (TYPE_SHORT):
	  v = DXVec((float)data_ptr_s[0], (float)data_ptr_s[1], 0.0); 
	  break;
	case (TYPE_INT):
	  v = DXVec((float)data_ptr_i[0], (float)data_ptr_i[1], 0.0); 
	  break;
	case (TYPE_FLOAT):
	  v = DXVec(data_ptr_f[0], data_ptr_f[1], 0.0);
	  break;
	case (TYPE_DOUBLE):
	  v = DXVec((float)data_ptr_d[0], (float)data_ptr_d[1], 0.0); 
	  break;
	case (TYPE_BYTE):
	  v = DXVec((float)data_ptr_b[0], (float)data_ptr_b[1], 0.0 );
	  break;
	case (TYPE_UBYTE):
	  v = DXVec((float)data_ptr_ub[0], (float)data_ptr_ub[1], 0.0 );
	  break;
	case (TYPE_USHORT):
	  v = DXVec((float)data_ptr_us[0], (float)data_ptr_us[1], 0.0 );
	  break;
	case (TYPE_UINT):
	  v = DXVec((float)data_ptr_ui[0], (float)data_ptr_ui[1], 0.0 );
	  break;
	default:
	  break;
	}
      }
      lv = DXLength(v);
      /*override = 0;*/
      /* don't override with user given glyph */
      /* why not? go ahead */
      if (((lv <= ratio*given_max) || (lv < given_min) || 
			     ((given_min == 0.0) && (given_max == 0.0)))) {
        /*override = 1;*/
	/* over-riding the vector glyph with a sphere */
        dp_old = DXGetArrayEntry(handle, i, scratch);
        if (pos_shape[0] == 1)                                              
	  translatematrix = DXTranslate(DXPt(dp_old[0], 0.0, 0.0));            
        else if (pos_shape[0] == 2)                                        
	  translatematrix = DXTranslate(DXPt(dp_old[0], dp_old[1], 0.0));
        else                                                               
	  translatematrix = DXTranslate(DXPt(dp_old[0],                      
					     dp_old[1],                    
					     dp_old[2]));                  
        t = DXConcatenate(overridescalematrix, translatematrix); 
        if (out_pos_dim == 3) {
	  if (!GlyphXform(t, numoverridepoints, 
                          overridepoints, (Point *)newoverridepoints))
           goto error;
        }
        else {
	  if (!GlyphXform2d(t, numoverridepoints, 
			    overridepoints, newoverridepoints))
           goto error;
        }
	if (!DXAddArrayData(new_positions_array,countpos, 
			    numoverridepoints, (Pointer)newoverridepoints)) 
           goto error;
	/* different stuff for arrows and needles */
	if (hasnormals) {
	  if (!DXAddArrayData(new_normals_array,countpos,numoverridepoints,
			      (Pointer)overridenormals))
           goto error;
	}
        if (!strcmp(connectiontype,"triangles")) {
	  if (!IncrementConnections(countpos, numoverrideconns, 
				    overrideconns, newoverrideconns))
           goto error;
	  if (!DXAddArrayData(new_connections_array,countconn,numoverrideconns,
			      (Pointer)newoverrideconns))
           goto error;
        }
        else {
	  if (!IncrementConnectionsLines(countpos, numoverrideconns, 
					 overridelines, newoverridelines))
           goto error;
	  if (!DXAddArrayData(new_connections_array,countconn,numoverrideconns,
			      (Pointer)newoverridelines))
           goto error;
        } 
        map[i] = numoverridepoints;
        totalpoints += numoverridepoints;
	countpos += numoverridepoints;
	countconn += numoverrideconns;
      }
      else {
	/* not over-riding */ 
        if (AutoSize)
	  datascale = scale*4*lv*base_size/given_max;
        else 
	  datascale = scale*lv;

        /* if the user specified "uniform scaling" then do it */
        if (!uniformscalingattr)
	  scalematrix = DXScale(vector_width, datascale, vector_width); 
        else
	  scalematrix = DXScale(datascale, datascale, datascale); 
	rotatematrix = GlyphAlign(canonical_v, v);
        dp_old = DXGetArrayEntry(handle, i, scratch);
        if (pos_shape[0] == 1)                                              
	  translatematrix = DXTranslate(DXPt(dp_old[0], 0.0, 0.0));            
        else if (pos_shape[0] == 2)                                        
	  translatematrix = DXTranslate(DXPt(dp_old[0], dp_old[1], 0.0));
        else                                                               
	  translatematrix = DXTranslate(DXPt(dp_old[0],                      
					     dp_old[1],                    
					     dp_old[2]));                  
        t = DXConcatenate(scalematrix, rotatematrix);
        t = DXConcatenate(t, translatematrix); 
        if (out_pos_dim == 3) {
	  if (!GlyphXform(t, numpoints, points, (Point *)newpoints))
           goto error;
        }
        else {
	  if (!GlyphXform2d(t, numpoints, points, newpoints))
           goto error;
        }
	if (!DXAddArrayData(new_positions_array,countpos, 
			    numpoints, (Pointer)newpoints)) 
           goto error;
	/* different stuff for arrows and needles */
	if (hasnormals) {
          if (!GlyphNormalXform(t, numpoints, normals, newnormals))
           goto error;
	  if (!DXAddArrayData(new_normals_array,countpos,numpoints,
			      (Pointer)newnormals))
           goto error;
        }
        if (!strcmp(connectiontype,"triangles")) {
	  if (!IncrementConnections(countpos, numconns, 
				    conns, newconns))
           goto error;
	  if (!DXAddArrayData(new_connections_array,countconn,numconns,
			      (Pointer)newconns))
           goto error;
	}
	else {
	  if (!IncrementConnectionsLines(countpos, numconns, 
					 lines, newlines))
           goto error;
	  if (!DXAddArrayData(new_connections_array,countconn,numconns,
			      (Pointer)newlines))
           goto error;
	}
        map[i] = numpoints;
        totalpoints += numpoints;
	countpos += numpoints;
	countconn += numconns;
      }
      }
      /* add here */
      switch (data_type) {
        case (TYPE_FLOAT):
           data_ptr_f += data_incr;
           break;
        case (TYPE_BYTE):
           data_ptr_b += data_incr;
           break;
        case (TYPE_SHORT):
           data_ptr_s += data_incr;
           break;
        case (TYPE_INT):
           data_ptr_i += data_incr;
           break;
        case (TYPE_DOUBLE):
           data_ptr_d += data_incr;
           break;
        case (TYPE_USHORT):
           data_ptr_us += data_incr;
           break;
        case (TYPE_UBYTE):
           data_ptr_ub += data_incr;
           break;
        case (TYPE_UINT):
           data_ptr_ui += data_incr;
           break;
        default:
	   break;
      }
    }
  }
  else if ((!strcmp(glyph_rank, "matrix"))||(!strcmp(glyph_rank, "matrix2"))) {
    /* the tensor case */
    int validcount = 0;
    if (AutoSize) {
      vector_width = scale*glyphshape * base_size/4.0; 
    }
    else {
      vector_width = scale*glyphshape*given_max/16.0; 
    }
    if (!strcmp(glyph_rank,"matrix")) {
      for (i=0; i<num_pos; i++) {
	/* tensor size formulas */
	if ((invalid) && (DXIsElementInvalidSequential(invalidhandle, i))) {
	  map[i] = 0;
	}
	else {
	  switch (data_type) {
	  case (TYPE_SHORT):
	    st.tau[0][0] = (float)data_ptr_s[0];
	    st.tau[0][1] = (float)data_ptr_s[1];
	    st.tau[0][2] = (float)data_ptr_s[2];
	    st.tau[1][0] = (float)data_ptr_s[3];
	    st.tau[1][1] = (float)data_ptr_s[4];
	    st.tau[1][2] = (float)data_ptr_s[5];
	    st.tau[2][0] = (float)data_ptr_s[6];
	    st.tau[2][1] = (float)data_ptr_s[7];
	    st.tau[2][2] = (float)data_ptr_s[8];
	    break;
	  case (TYPE_INT):
	    st.tau[0][0] = (float)data_ptr_i[0];
	    st.tau[0][1] = (float)data_ptr_i[1];
	    st.tau[0][2] = (float)data_ptr_i[2];
	    st.tau[1][0] = (float)data_ptr_i[3];
	    st.tau[1][1] = (float)data_ptr_i[4];
	    st.tau[1][2] = (float)data_ptr_i[5];
	    st.tau[2][0] = (float)data_ptr_i[6];
	    st.tau[2][1] = (float)data_ptr_i[7];
	    st.tau[2][2] = (float)data_ptr_i[8];
	    break;
	  case (TYPE_FLOAT):
	    st.tau[0][0] = (float)data_ptr_f[0];
	    st.tau[0][1] = (float)data_ptr_f[1]; 
	    st.tau[0][2] = (float)data_ptr_f[2];
	    st.tau[1][0] = (float)data_ptr_f[3];
	    st.tau[1][1] = (float)data_ptr_f[4];
	    st.tau[1][2] = (float)data_ptr_f[5];
	    st.tau[2][0] = (float)data_ptr_f[6];
	    st.tau[2][1] = (float)data_ptr_f[7];
	    st.tau[2][2] = (float)data_ptr_f[8];
	    break;
	  case (TYPE_DOUBLE):
	    st.tau[0][0] = (float)data_ptr_d[0];
	    st.tau[0][1] = (float)data_ptr_d[1];
	    st.tau[0][2] = (float)data_ptr_d[2];
	    st.tau[1][0] = (float)data_ptr_d[3];
	    st.tau[1][1] = (float)data_ptr_d[4];
	    st.tau[1][2] = (float)data_ptr_d[5];
	    st.tau[2][0] = (float)data_ptr_d[6];
	    st.tau[2][1] = (float)data_ptr_d[7];
	    st.tau[2][2] = (float)data_ptr_d[8];
	    break;
	  case (TYPE_BYTE):
	    st.tau[0][0] = (float)data_ptr_b[0];
	    st.tau[0][1] = (float)data_ptr_b[1];
	    st.tau[0][2] = (float)data_ptr_b[2];
	    st.tau[1][0] = (float)data_ptr_b[3];
	    st.tau[1][1] = (float)data_ptr_b[4];
	    st.tau[1][2] = (float)data_ptr_b[5];
	    st.tau[2][0] = (float)data_ptr_b[6];
	    st.tau[2][1] = (float)data_ptr_b[7];
	    st.tau[2][2] = (float)data_ptr_b[8];
	    break;
	  case (TYPE_UBYTE):
	    st.tau[0][0] = (float)data_ptr_ub[0];
	    st.tau[0][1] = (float)data_ptr_ub[1];
	    st.tau[0][2] = (float)data_ptr_ub[2];
	    st.tau[1][0] = (float)data_ptr_ub[3];
	    st.tau[1][1] = (float)data_ptr_ub[4];
	    st.tau[1][2] = (float)data_ptr_ub[5];
	    st.tau[2][0] = (float)data_ptr_ub[6];
	    st.tau[2][1] = (float)data_ptr_ub[7];
	    st.tau[2][2] = (float)data_ptr_ub[8];
	    break;
	  case (TYPE_USHORT):
	    st.tau[0][0] = (float)data_ptr_us[0];
	    st.tau[0][1] = (float)data_ptr_us[1];
	    st.tau[0][2] = (float)data_ptr_us[2];
	    st.tau[1][0] = (float)data_ptr_us[3];
	    st.tau[1][1] = (float)data_ptr_us[4];
	    st.tau[1][2] = (float)data_ptr_us[5];
	    st.tau[2][0] = (float)data_ptr_us[6];
	    st.tau[2][1] = (float)data_ptr_us[7];
	    st.tau[2][2] = (float)data_ptr_us[8];
	    break;
	  case (TYPE_UINT):
	    st.tau[0][0] = (float)data_ptr_ui[0];
	    st.tau[0][1] = (float)data_ptr_ui[1];
	    st.tau[0][2] = (float)data_ptr_ui[2];
	    st.tau[1][0] = (float)data_ptr_ui[3];
	    st.tau[1][1] = (float)data_ptr_ui[4];
	    st.tau[1][2] = (float)data_ptr_ui[5];
	    st.tau[2][0] = (float)data_ptr_ui[6];
	    st.tau[2][1] = (float)data_ptr_ui[7];
	    st.tau[2][2] = (float)data_ptr_ui[8];
	    break;
	  default:
	    break;
	  }
	  if (!GetEigenVectors(st, eigen_vector)) {
	    goto error;
	  }
	  
	  for (j=0; j< 3; j++) { 
	    countpos = (3*validcount+j)*numpoints;
	    countconn = (3*validcount+j)*numconns;
	    if (AutoSize)
	      datascale = scale*4*DXLength(eigen_vector[j])*base_size/given_max;
	    else 
	      datascale = scale*DXLength(eigen_vector[j]);

            if (!uniformscalingattr)
	       scalematrix = DXScale(vector_width, datascale, vector_width); 
            else
	       scalematrix = DXScale(datascale, datascale, datascale); 
	    rotatematrix = GlyphAlign(canonical_v, eigen_vector[j]);
	    dp_old = DXGetArrayEntry(handle, i, scratch);
	    if (pos_shape[0] == 1)                                             
	      translatematrix = DXTranslate(DXPt(dp_old[0], 0.0, 0.0));       
	    else if (pos_shape[0] == 2)                                        
	      translatematrix = DXTranslate(DXPt(dp_old[0], dp_old[1], 0.0));
	    else                                                               
	      translatematrix = DXTranslate(DXPt(dp_old[0],                 
						 dp_old[1],                    
						 dp_old[2]));                  
	    t = DXConcatenate(scalematrix, rotatematrix);
	    t = DXConcatenate(t, translatematrix); 
	    if (out_pos_dim == 3) {
	      if (!GlyphXform(t, numpoints, points,(Point *)newpoints))
		goto error;
	    }
	    else {
	      if (!GlyphXform2d(t, numpoints, points, newpoints))
		goto error;
	    }
	    if (!DXAddArrayData(new_positions_array,countpos, 
				numpoints, (Pointer)newpoints)) 
	      goto error;
	    
	    /* different stuff for arrows and needles */
	    if (hasnormals) {
	      if (!GlyphNormalXform(t, numpoints, normals, newnormals))
		goto error;
	      if (!DXAddArrayData(new_normals_array,countpos,numpoints,
				  (Pointer)newnormals))
		goto error;
	    }
	    if (!strcmp(connectiontype,"triangles")) {
	      if (!IncrementConnections(countpos, numconns, 
					conns, newconns))
		goto error;
	      if (!DXAddArrayData(new_connections_array,countconn,numconns,
				  (Pointer)newconns))
		goto error;
	    }
	    else {
	      if (!IncrementConnectionsLines(countpos, numconns, 
					     lines, newlines))
		goto error;
	      if (!DXAddArrayData(new_connections_array,countconn,numconns,
				  (Pointer)newlines))
		goto error;
	    }
	    map[i] += numpoints;
	    totalpoints += numpoints;
	  }
	  validcount++;
	}
	/* add here */
	switch (data_type) {
        case (TYPE_FLOAT):
          data_ptr_f += data_incr;
          break;
        case (TYPE_BYTE):
          data_ptr_b += data_incr;
          break;
        case (TYPE_SHORT):
          data_ptr_s += data_incr;
          break;
        case (TYPE_INT):
          data_ptr_i += data_incr;
          break;
        case (TYPE_DOUBLE):
          data_ptr_d += data_incr;
          break;
        case (TYPE_USHORT):
          data_ptr_us += data_incr;
          break;
        case (TYPE_UBYTE):
          data_ptr_ub += data_incr;
          break;
        case (TYPE_UINT):
          data_ptr_ui += data_incr;
          break;
	default:
	  break;
	}
      }
    }
    else {
      for (i=0; i<num_pos; i++) {
	/* 2x2 tensor */
	/* tensor size formulas */
	if ((invalid) && (DXIsElementInvalidSequential(invalidhandle, i))) {
	  map[i] = 0;
	}
	else {
	  switch (data_type) {
	  case (TYPE_SHORT):
	    st2.tau[0][0] = (float)data_ptr_s[0];
	    st2.tau[0][1] = (float)data_ptr_s[1];
	    st2.tau[1][0] = (float)data_ptr_s[2];
	    st2.tau[1][1] = (float)data_ptr_s[3];
	    break;
	  case (TYPE_INT):
	    st2.tau[0][0] = (float)data_ptr_i[0];
	    st2.tau[0][1] = (float)data_ptr_i[1];
	    st2.tau[1][0] = (float)data_ptr_i[2];
	    st2.tau[1][1] = (float)data_ptr_i[3];
	    break;
	  case (TYPE_FLOAT):
	    st2.tau[0][0] = (float)data_ptr_f[0];
	    st2.tau[0][1] = (float)data_ptr_f[1]; 
	    st2.tau[1][0] = (float)data_ptr_f[2];
	    st2.tau[1][1] = (float)data_ptr_f[3];
	    break;
	  case (TYPE_DOUBLE):
	    st2.tau[0][0] = (float)data_ptr_d[0];
	    st2.tau[0][1] = (float)data_ptr_d[1];
	    st2.tau[1][0] = (float)data_ptr_d[2];
	    st2.tau[1][1] = (float)data_ptr_d[3];
	    break;
	  case (TYPE_BYTE):
	    st2.tau[0][0] = (float)data_ptr_b[0];
	    st2.tau[0][1] = (float)data_ptr_b[1];
	    st2.tau[1][0] = (float)data_ptr_b[2];
	    st2.tau[1][1] = (float)data_ptr_b[3];
	    break;
	  case (TYPE_UBYTE):
	    st2.tau[0][0] = (float)data_ptr_ub[0];
	    st2.tau[0][1] = (float)data_ptr_ub[1];
	    st2.tau[1][0] = (float)data_ptr_ub[2];
	    st2.tau[1][1] = (float)data_ptr_ub[3];
	    break;
	  case (TYPE_USHORT):
	    st2.tau[0][0] = (float)data_ptr_us[0];
	    st2.tau[0][1] = (float)data_ptr_us[1];
	    st2.tau[1][0] = (float)data_ptr_us[2];
	    st2.tau[1][1] = (float)data_ptr_us[3];
	    break;
	  case (TYPE_UINT):
	    st2.tau[0][0] = (float)data_ptr_ui[0];
	    st2.tau[0][1] = (float)data_ptr_ui[1];
	    st2.tau[1][0] = (float)data_ptr_ui[2];
	    st2.tau[1][1] = (float)data_ptr_ui[3];
	    break;
	  default:
	    break;
	  }
	  if (!GetEigenVectors2(st2, eigen_vector)) {
	    goto error;
	  }
	  
	  for (j=0; j< 2; j++) { 
	    countpos = (2*validcount+j)*numpoints; /* ???? */
	    countconn = (2*validcount+j)*numconns;
	    if (AutoSize)
	      datascale = scale*4*DXLength(eigen_vector[j])*base_size/given_max;
	    else 
	      datascale = scale*DXLength(eigen_vector[j]);
            if (!uniformscalingattr)
	       scalematrix = DXScale(vector_width, datascale, vector_width); 
            else
	       scalematrix = DXScale(datascale, datascale, datascale); 
	    rotatematrix = GlyphAlign(canonical_v, eigen_vector[j]);
	    dp_old = DXGetArrayEntry(handle, i, scratch);
	    if (pos_shape[0] == 1)                                            
	      translatematrix = DXTranslate(DXPt(dp_old[0], 0.0, 0.0));       
	    else if (pos_shape[0] == 2)                                        
	      translatematrix = DXTranslate(DXPt(dp_old[0], dp_old[1], 0.0));
	    else                                                               
	      translatematrix = DXTranslate(DXPt(dp_old[0],               
						 dp_old[1],                    
						 dp_old[2]));                  
	    t = DXConcatenate(scalematrix, rotatematrix);
	    t = DXConcatenate(t, translatematrix); 
	    if (out_pos_dim == 3) {
	      if (!GlyphXform(t, numpoints, points,(Point *)newpoints))
		goto error;
	    }
	    else {
	      if (!GlyphXform2d(t, numpoints, points, newpoints))
		goto error;
	    }
	    if (!DXAddArrayData(new_positions_array,countpos, 
				numpoints, (Pointer)newpoints)) 
	      goto error;
	    
	    /* different stuff for arrows and needles */
	    if (hasnormals) {
	      if (!GlyphNormalXform(t, numpoints, normals, newnormals))
		goto error;
	      if (!DXAddArrayData(new_normals_array,countpos,numpoints,
				  (Pointer)newnormals))
		goto error;
	    }
	    if (!strcmp(connectiontype,"triangles")) {
	      if (!IncrementConnections(countpos, numconns, 
					conns, newconns))
		goto error;
	      if (!DXAddArrayData(new_connections_array,countconn,numconns,
				  (Pointer)newconns))
		goto error;
	    }
	    else {
	      if (!IncrementConnectionsLines(countpos, numconns, 
					     lines, newlines))
		goto error;
	      if (!DXAddArrayData(new_connections_array,countconn,numconns,
				  (Pointer)newlines))
		goto error;
	    }
	    map[i] += numpoints;
	    totalpoints += numpoints;
	  }
	  validcount++;
	}
	/* add here */
	switch (data_type) {
        case (TYPE_FLOAT):
          data_ptr_f += data_incr;
          break;
        case (TYPE_BYTE):
          data_ptr_b += data_incr;
          break;
        case (TYPE_SHORT):
          data_ptr_s += data_incr;
          break;
        case (TYPE_INT):
          data_ptr_i += data_incr;
          break;
        case (TYPE_DOUBLE):
          data_ptr_d += data_incr;
          break;
        case (TYPE_USHORT):
          data_ptr_us += data_incr;
          break;
        case (TYPE_UBYTE):
          data_ptr_ub += data_incr;
          break;
        case (TYPE_UINT):
          data_ptr_ui += data_incr;
          break;
	default:
	  break;
	}
      }
    }
  }
  
 
  DXSetComponentValue((Field)outo, "positions", (Object)new_positions_array);

  /* before expanding components, delete the invalid positions or connections 
   */
  if (DXGetComponentValue((Field)outo,"invalid positions"))
     DXRemove(outo,"invalid positions"); 
  if (DXGetComponentValue((Field)outo,"invalid connections"))
     DXRemove(outo,"invalid connections"); 
  if (!ExpandComponents((Field)outo, attr, totalpoints, num_pos, map,""))
    goto error;
  
  if (newpoints) DXFree((Pointer)newpoints);
  if (newconns) DXFree((Pointer)newconns);
  if (newnormals) DXFree((Pointer)newnormals); 
  if (newlines) DXFree((Pointer)newlines); 
  if (newoverridepoints) DXFree((Pointer)newoverridepoints);
  if (newoverrideconns) DXFree((Pointer)newoverrideconns);
  if (newoverridenormals) DXFree((Pointer)newoverridenormals); 
  if (newoverridelines) DXFree((Pointer)newoverridelines); 
  DXFree((Pointer)map);
  if (type_is_field) {
     DXFree((Pointer)points);
     DXFree((Pointer)conns);
     if (hasnormals) DXFree((Pointer)normals);
  }

  DXChangedComponentValues((Field)outo,"positions");
  DXChangedComponentValues((Field)outo,"connections");
  DXChangedComponentValues((Field)outo,"normals");
  DXEndField((Field)outo);
  if (posted) DXDelete((Object)positions_array);
  DXFreeArrayHandle(handle);
  DXFreeInvalidComponentHandle(invalidhandle);
  DXFree((Pointer)scratch);
  return OK;

  error:
  if (posted) DXDelete((Object)positions_array);
  DXFree((Pointer)scratch);
  if (newpoints) DXFree((Pointer)newpoints);
  if (newconns) DXFree((Pointer)newconns);
  if (newnormals) DXFree((Pointer)newnormals); 
  if (newlines) DXFree((Pointer)newlines); 
  if (newoverridepoints) DXFree((Pointer)newoverridepoints);
  if (newoverrideconns) DXFree((Pointer)newoverrideconns);
  if (newoverridenormals) DXFree((Pointer)newoverridenormals); 
  if (newoverridelines) DXFree((Pointer)newoverridelines); 
  DXFree((Pointer)map);
  DXFreeArrayHandle(handle);
  DXFreeInvalidComponentHandle(invalidhandle);
  if (type_is_field) {
     DXFree((Pointer)points);
     DXFree((Pointer)conns);
     DXFree((Pointer)normals);
  }
  return ERROR;
}


static 
Error 
IncrementConnectionsLines(int countpos, int numconns, 
                          Line *connections,
                          Line *newconnections)
{
   int i;

   for (i=0; i<numconns; i++) {
       newconnections[i].p = connections[i].p + countpos;
       newconnections[i].q = connections[i].q + countpos;
   }
   return OK;
}


static 
Error 
IncrementConnections(int countpos, int numconns, 
                     Triangle *connections,
                     Triangle *newconnections)
{
   int i;



   for (i=0; i<numconns; i++) {
       newconnections[i].p = connections[i].p + countpos;
       newconnections[i].q = connections[i].q + countpos;
       newconnections[i].r = connections[i].r + countpos;
   }
   return OK;
}


static 
Error
GlyphNormalXform(Matrix t, int n,
   Point *normals_old, Point *normals_new)
{
    int i;

    /* Normal vector must be transformed by the transpose of the inverse. */
/*
    t = DXInvert(t);
    t = DXTranspose(t);
*/
    /* Alternatively, the adjoint transpose can be used. */
    t = DXAdjointTranspose(t);
    for (i = 0; i < n; i++) {
        normals_new[i] = DXApply(normals_old[i], t);
        /* must normalize the new result */
         normals_new[i] = DXNormalize(normals_new[i]); 
    }
    return OK;
}



static 
Error
GlyphXform2d(Matrix t, int n, Point *points_old, float *points_new)
{
    int i;
    Point pt;


    for (i = 0; i < n; i++) {
       pt = DXApply(points_old[i], t); 
       /* just strip off the z component */
       points_new[2*i] = pt.x;
       points_new[2*i+1] = pt.y;
    }
    return OK;
}


static 
Error
GlyphXform(Matrix t, int n, Point *points_old, Point *points_new)
{
    int i;


    for (i = 0; i < n; i++) {
       points_new[i] = DXApply(points_old[i], t); 
    }

    return OK;
}


static 
Error
  GetGlyphSquare(char *type_string, int *numpoints, int *numconns, 
                  Point **returnpoints, Triangle **returnconns) 
{
#include "glyph_SQUARE.h"


   *numpoints = SQUAREPTS;
   *numconns = SQUARETRS;
   *returnpoints = points;
   *returnconns = triangles;
   return OK;
}

static 
Error
  GetGlyphCircle4(char *type_string, int *numpoints, int *numconns, 
                  Point **returnpoints, Triangle **returnconns) 
{
#include "glyph_CIRCLE4.h"


   *numpoints = CIRCLE4PTS;
   *numconns = CIRCLE4TRS;
   *returnpoints = points;
   *returnconns = triangles;
   return OK;
}

static 
Error
  GetGlyphCircle6(char *type_string, int *numpoints, int *numconns, 
                  Point **returnpoints, Triangle **returnconns) 
{
#include "glyph_CIRCLE6.h"


   *numpoints = CIRCLE6PTS;
   *numconns = CIRCLE6TRS;
   *returnpoints = points;
   *returnconns = triangles;
   return OK;
}

static 
Error
  GetGlyphCircle8(char *type_string, int *numpoints, int *numconns, 
                  Point **returnpoints, Triangle **returnconns) 
{
#include "glyph_CIRCLE8.h"


   *numpoints = CIRCLE8PTS;
   *numconns = CIRCLE8TRS;
   *returnpoints = points;
   *returnconns = triangles;
   return OK;
}


static 
Error
  GetGlyphCircle10(char *type_string, int *numpoints, int *numconns, 
                   Point **returnpoints, Triangle **returnconns) 
{
#include "glyph_CIRCLE10.h"


   *numpoints = CIRCLE10PTS;
   *numconns = CIRCLE10TRS;
   *returnpoints = points;
   *returnconns = triangles;
   return OK;
}


static 
Error
  GetGlyphCircle20(char *type_string, int *numpoints, int *numconns, 
                   Point **returnpoints, Triangle **returnconns) 
{
#include "glyph_CIRCLE20.h"


   *numpoints = CIRCLE20PTS;
   *numconns = CIRCLE20TRS;
   *returnpoints = points;
   *returnconns = triangles;
   return OK;
}


static 
Error
  GetGlyphCircle40(char *type_string, int *numpoints, int *numconns, 
                   Point **returnpoints, Triangle **returnconns) 
{
#include "glyph_CIRCLE40.h"


   *numpoints = CIRCLE40PTS;
   *numconns = CIRCLE40TRS;
   *returnpoints = points;
   *returnconns = triangles;
   return OK;
}


static 
Error
  GetGlyphNeedle2d(char *type_string, int *numpoints, int *numconns, 
                   Point **returnpoints, Line **returnconns) 
{
#include "glyph_NDDL2D.h"

   *numpoints = NDDL2DPTS;
   *numconns = NDDL2DLNS;
   *returnpoints = points;
   *returnconns = lines;
   return OK;
}

static 
Error
  GetGlyphArrow2d(char *type_string, int *numpoints, int *numconns, 
                  Point **returnpoints, Line **returnconns) 
{
#include "glyph_ARRW2D.h"

   *numpoints = ARRW2DPTS;
   *numconns = ARRW2DLNS;
   *returnpoints = points;
   *returnconns = lines;
   return OK;
}


static 
Error
  GetGlyphRocket2d(char *type_string, int *numpoints, int *numconns, 
                   Point **returnpoints, Triangle **returnconns) 
{
#include "glyph_RCKT2D.h"

   *numpoints = RCKT2DPTS;
   *numconns = RCKT2DTRS;
   *returnpoints = points;
   *returnconns = triangles;
   return OK;
}



static 
Error
  GetGlyphDiamond(char *type_string, int *numpoints, int *numconns, 
                  Point **returnpoints, Triangle **returnconns, 
                  Point **returnnormals) 
{
#include "glyph_DMND.h"


   *numpoints = DMNDPTS;
   *numconns = DMNDTRS;
   *returnpoints = points;
   *returnnormals = normals;
   *returnconns = triangles;
   return OK;
}


static 
Error
  GetGlyphBox(char *type_string, int *numpoints, int *numconns, 
                   Point **returnpoints, Triangle **returnconns, 
                  Point **returnnormals)
{
#include "glyph_BOX.h"


   *numpoints = BOXPTS;
   *numconns = BOXTRS;
   *returnpoints = points;
   *returnnormals = normals;
   *returnconns = triangles;
   return OK;
}

static 
Error
  GetGlyphSphere14(char *type_string, int *numpoints, int *numconns, 
                   Point **returnpoints, Triangle **returnconns, 
                  Point **returnnormals)
{
#include "glyph_SPHR14.h"


   *numpoints = SPHR14PTS;
   *numconns = SPHR14TRS;
   *returnpoints = points;
   *returnnormals = normals;
   *returnconns = triangles;
   return OK;
}

static 
Error
  GetGlyphSphere26(char *type_string, int *numpoints, int *numconns, 
                   Point **returnpoints, Triangle **returnconns, 
                  Point **returnnormals)
{
#include "glyph_SPHR26.h"


   *numpoints = SPHR26PTS;
   *numconns = SPHR26TRS;
   *returnpoints = points;
   *returnnormals = normals;
   *returnconns = triangles;
   return OK;
}

static 
Error
  GetGlyphSphere42(char *type_string, int *numpoints, int *numconns, 
                   Point **returnpoints, Triangle **returnconns, 
                  Point **returnnormals)
{
#include "glyph_SPHR42.h"


   *numpoints = SPHR42PTS;
   *numconns = SPHR42TRS;
   *returnpoints = points;
   *returnnormals = normals;
   *returnconns = triangles;

   return OK;
}

static 
Error
  GetGlyphSphere62(char *type_string, int *numpoints, int *numconns, 
                   Point **returnpoints, Triangle **returnconns, 
                   Point **returnnormals)
{
#include "glyph_SPHR62.h"


   *numpoints = SPHR62PTS;
   *numconns = SPHR62TRS;
   *returnpoints = points;
   *returnnormals = normals;
   *returnconns = triangles;
   return OK;
}

static 
Error
  GetGlyphSphere114(char *type_string, int *numpoints, int *numconns, 
                    Point **returnpoints, Triangle **returnconns, 
                    Point **returnnormals)
{
#include "glyph_SPHR114.h"


   *numpoints = SPHR114PTS;
   *numconns = SPHR114TRS;
   *returnpoints = points;
   *returnnormals = normals;
   *returnconns = triangles;
   return OK;
}

static 
Error
  GetGlyphSphere146(char *type_string, int *numpoints, int *numconns, 
                    Point **returnpoints, Triangle **returnconns, 
                    Point **returnnormals)
{
#include "glyph_SPHR146.h"


   *numpoints = SPHR146PTS;
   *numconns = SPHR146TRS;
   *returnpoints = points;
   *returnnormals = normals;
   *returnconns = triangles;
   return OK;
}


static 
Error
  GetGlyphSphere266(char *type_string, int *numpoints, int *numconns, 
                    Point **returnpoints, Triangle **returnconns, 
                    Point **returnnormals)
{
#include "glyph_SPHR266.h"


   *numpoints = SPHR266PTS;
   *numconns = SPHR266TRS;
   *returnpoints = points;
   *returnnormals = normals;
   *returnconns = triangles;
   return OK;
}


static 
Error
  GetGlyphNeedle(char *type_string, int *numpoints, int *numconns, 
                 Point **returnpoints, Line **returnconns) 
{
#include "glyph_NDDL.h"


   *numpoints = NDDLPTS;
   *numconns = NDDLLNS;
   *returnpoints = points;
   *returnconns = lines;
   return OK;
}

static 
Error
  GetGlyphPoint(char *type_string, int *numpoints, int *numconns, 
                Point **returnpoints, Line **returnconns) 
{
#include "glyph_PNT.h"


   *numpoints = PNTPTS;
   *numconns = PNTLNS;
   *returnpoints = points;
   *returnconns = lines;
   return OK;
}



static 
Error
  GetGlyphArrow(char *type_string, int *numpoints, int *numconns, 
                Point **returnpoints, Line **returnconns) 
{
#include "glyph_ARRW.h"


   *numpoints = ARRWPTS;
   *numconns = ARRWLNS;
   *returnpoints = points;
   *returnconns = lines;
   return OK;
}


static 
Error
  GetGlyphRocket3(char *type_string, int *numpoints, int *numconns, 
                  Point **returnpoints, Triangle **returnconns, 
                  Point **returnnormals)
{
#include "glyph_RCKT3.h"

   *numpoints = RCKT3PTS;
   *numconns = RCKT3TRS;
   *returnpoints = points;
   *returnnormals = normals;
   *returnconns = triangles;
   return OK;
}

static 
Error
  GetGlyphRocket4(char *type_string, int *numpoints, int *numconns, 
                  Point **returnpoints, Triangle **returnconns, 
                  Point **returnnormals)
{
#include "glyph_RCKT4.h"


   *numpoints = RCKT4PTS;
   *numconns = RCKT4TRS;
   *returnpoints = points;
   *returnnormals = normals;
   *returnconns = triangles;
   return OK;
}

static 
Error
  GetGlyphRocket6(char *type_string, int *numpoints, int *numconns, 
                  Point **returnpoints, Triangle **returnconns, 
                  Point **returnnormals)
{
#include "glyph_RCKT6.h"


   *numpoints = RCKT6PTS;
   *numconns = RCKT6TRS;
   *returnpoints = points;
   *returnnormals = normals;
   *returnconns = triangles;
   return OK;
}

static 
Error
  GetGlyphRocket8(char *type_string, int *numpoints, int *numconns, 
                  Point **returnpoints, Triangle **returnconns, 
                  Point **returnnormals)
{
#include "glyph_RCKT8.h"


   *numpoints = RCKT8PTS;
   *numconns = RCKT8TRS;
   *returnpoints = points;
   *returnnormals = normals;
   *returnconns = triangles;
   return OK;
}


static 
Error
  GetGlyphRocket12(char *type_string, int *numpoints, int *numconns, 
                   Point **returnpoints, Triangle **returnconns, 
                   Point **returnnormals)
{
#include "glyph_RCKT12.h"


   *numpoints = RCKT12PTS;
   *numconns = RCKT12TRS;
   *returnpoints = points;
   *returnnormals = normals;
   *returnconns = triangles;
   return OK;
}


static 
Error
  GetGlyphRocket20(char *type_string, int *numpoints, int *numconns, 
                   Point **returnpoints, Triangle **returnconns, 
                   Point **returnnormals)
{
#include "glyph_RCKT20.h"


   *numpoints = RCKT20PTS;
   *numconns = RCKT20TRS;
   *returnpoints = points;
   *returnnormals = normals;
   *returnconns = triangles;
   return OK;

}


static 
Error
GetGlyphs(char *type_string, int *numpoints, int *numconn, 
          Point **returnpoints, Triangle **returnconns, 
          Point **returnnormals, Line **returnlines, int *hasnormals,
          char *connectiontype)
/* glyphs are defined in 3D (even flat ones). This makes the use of
 * transformation matrices and DXApply possible for all cases. The z dimension
 * is stripped off of the flat glyphs before placing in the field (this
 * happens inside of GlyphXform2D */
{
     if (!strcmp(type_string,"point")) {
       GetGlyphPoint(type_string, numpoints, numconn, returnpoints,
                     returnlines);
       *hasnormals = 0;
       strcpy(connectiontype,"lines");
     }
     else if (!strcmp(type_string,"diamond")) {
       GetGlyphDiamond(type_string, numpoints, numconn, returnpoints,
                       returnconns, returnnormals);
       *hasnormals = 1;
       strcpy(connectiontype,"triangles");
     }
     else if (!strcmp(type_string,"cube")) {
       GetGlyphBox(type_string, numpoints, numconn, returnpoints,
                       returnconns, returnnormals);
       *hasnormals = 1;
       strcpy(connectiontype,"triangles");
     }
     else if (!strcmp(type_string,"sphere14")) {
       GetGlyphSphere14(type_string, numpoints, numconn, returnpoints,
                        returnconns, returnnormals);
       *hasnormals = 1;
       strcpy(connectiontype,"triangles");
     }
     else if (!strcmp(type_string,"sphere26")) {
       GetGlyphSphere26(type_string, numpoints, numconn, returnpoints,
                        returnconns, returnnormals);
       *hasnormals = 1;
       strcpy(connectiontype,"triangles");
     }
     else if (!strcmp(type_string,"sphere42")) {
       GetGlyphSphere42(type_string, numpoints, numconn, returnpoints,
                        returnconns, returnnormals);
       *hasnormals = 1;
       strcpy(connectiontype,"triangles");
     }
     else if (!strcmp(type_string,"sphere62")) {
       GetGlyphSphere62(type_string, numpoints, numconn, returnpoints,
                        returnconns, returnnormals);
       *hasnormals = 1;
       strcpy(connectiontype,"triangles");
     }
     else if (!strcmp(type_string,"sphere114")) {
       GetGlyphSphere114(type_string, numpoints, numconn, returnpoints,
                        returnconns, returnnormals);
       *hasnormals = 1;
       strcpy(connectiontype,"triangles");
     }
     else if (!strcmp(type_string,"sphere146")) {
       GetGlyphSphere146(type_string, numpoints, numconn, returnpoints,
                        returnconns, returnnormals);
       *hasnormals = 1;
       strcpy(connectiontype,"triangles");
     }
     else if (!strcmp(type_string,"sphere266")) {
       GetGlyphSphere266(type_string, numpoints, numconn, returnpoints,
                        returnconns, returnnormals);
       *hasnormals = 1;
       strcpy(connectiontype,"triangles");
     }
     else if (!strcmp(type_string,"circle4")) {
       GetGlyphCircle4(type_string, numpoints, numconn, returnpoints,
                         returnconns);
       *hasnormals = 0;
       strcpy(connectiontype,"triangles");
     }
     else if (!strcmp(type_string,"square")) {
       GetGlyphSquare(type_string, numpoints, numconn, returnpoints,
                         returnconns);
       *hasnormals = 0;
       strcpy(connectiontype,"triangles");
     }
     else if (!strcmp(type_string,"circle6")) {
       GetGlyphCircle6(type_string, numpoints, numconn, returnpoints,
                         returnconns);
       *hasnormals = 0;
       strcpy(connectiontype,"triangles");
     }
     else if (!strcmp(type_string,"circle8")) {
       GetGlyphCircle8(type_string, numpoints, numconn, returnpoints,
                         returnconns);
       *hasnormals = 0;
       strcpy(connectiontype,"triangles");
     }
     else if (!strcmp(type_string,"circle10")) {
       GetGlyphCircle10(type_string, numpoints, numconn, returnpoints,
                         returnconns);
       *hasnormals = 0;
       strcpy(connectiontype,"triangles");
     }
     else if (!strcmp(type_string,"circle20")) {
       GetGlyphCircle20(type_string, numpoints, numconn, returnpoints,
                         returnconns);
       *hasnormals = 0;
       strcpy(connectiontype,"triangles");
     }
     else if (!strcmp(type_string,"circle40")) {
       GetGlyphCircle40(type_string, numpoints, numconn, returnpoints,
                         returnconns);
       *hasnormals = 0;
       strcpy(connectiontype,"triangles");
     }
     else if (!strcmp(type_string,"rocket3")) {
       GetGlyphRocket3(type_string, numpoints, numconn, returnpoints,
                        returnconns, returnnormals);
       *hasnormals = 1;
       strcpy(connectiontype,"triangles");
     }
     else if (!strcmp(type_string,"rocket4")) {
       GetGlyphRocket4(type_string, numpoints, numconn, returnpoints,
                        returnconns, returnnormals);
       *hasnormals = 1;
       strcpy(connectiontype,"triangles");
     }
     else if (!strcmp(type_string,"rocket6")) {
       GetGlyphRocket6(type_string, numpoints, numconn, returnpoints,
                        returnconns, returnnormals);
       *hasnormals = 1;
       strcpy(connectiontype,"triangles");
     }
     else if (!strcmp(type_string,"rocket8")) {
       GetGlyphRocket8(type_string, numpoints, numconn, returnpoints,
                        returnconns, returnnormals);
       *hasnormals = 1;
       strcpy(connectiontype,"triangles");
     }
     else if (!strcmp(type_string,"rocket12")) {
       GetGlyphRocket12(type_string, numpoints, numconn, returnpoints,
                        returnconns, returnnormals);
       *hasnormals = 1;
       strcpy(connectiontype,"triangles");
     }
     else if (!strcmp(type_string,"rocket20")) {
       GetGlyphRocket20(type_string, numpoints, numconn, returnpoints,
                        returnconns, returnnormals);
       *hasnormals = 1;
       strcpy(connectiontype,"triangles");
     } 
     else if (!strcmp(type_string,"rocket2d")) {
       GetGlyphRocket2d(type_string, numpoints, numconn, returnpoints,
                        returnconns);
       *hasnormals = 0;
       strcpy(connectiontype,"triangles");
     } 
     else if (!strcmp(type_string,"needle")) {
       GetGlyphNeedle(type_string, numpoints, numconn, returnpoints,
                        returnlines);
       *hasnormals = 0;
       strcpy(connectiontype,"lines");
     }
     else if (!strcmp(type_string,"needle2d")) {
       GetGlyphNeedle2d(type_string, numpoints, numconn, returnpoints,
                        returnlines);
       *hasnormals = 0;
       strcpy(connectiontype,"lines");
     }
     else if (!strcmp(type_string,"arrow")) {
       GetGlyphArrow(type_string, numpoints, numconn, returnpoints,
                        returnlines);
       *hasnormals = 0;
       strcpy(connectiontype,"lines");
     }
     else if (!strcmp(type_string,"arrow2d")) {
       GetGlyphArrow2d(type_string, numpoints, numconn, returnpoints,
                        returnlines);
       *hasnormals = 0;
       strcpy(connectiontype,"lines");
     }
     else {
       DXSetError(ERROR_BAD_PARAMETER,"unknown glyphtype %s", type_string);
       return ERROR;
     } 


     return OK;

}

static 
Error
  GetGlyphField(Object glyphfield, int *numpoints, int *numconn, 
		Point **returnpoints, Triangle **returnconns,
		Line **returnlines,
		Point **returnnormals, Matrix *xform, int *anyxforms, 
                int *hasnormals, int *out_pos_dim, char *conntype)
{

  Array positions, connections, normals;
  Type type;
  Category category;
  int rank, shape[30];
  char *attr;
  Matrix newxform;
  Object subfield;
  Point *norm_ptr=NULL;
  Point *temp_pos, *temp_norm;
  float *pos_ptr;
  Triangle *temp_conn;
  Line *temp_line;

  Triangle *con_ptr=NULL;
  Line *line_ptr=NULL;
  int i;

  normals = NULL;
  positions = NULL;
  connections = NULL;
  
  /* this may be an x-formed object. Therefore we need to recurse through
   * the object */
  

  switch (DXGetObjectClass(glyphfield)) {
  case CLASS_FIELD:
    positions = (Array)DXGetComponentValue((Field)glyphfield,"positions");
    connections = (Array)DXGetComponentValue((Field)glyphfield,"connections");
    normals = (Array)DXGetComponentValue((Field)glyphfield,"normals");
    if (normals)
       *hasnormals = 1;
    else
       *hasnormals = 0;
    
    if (!positions) {
      /* given glyph is missing positions */
      DXSetError(ERROR_MISSING_DATA,"#10250","given glyph", "positions");
      return ERROR;
    }
    if (!connections) {
      /* given glyph is missing connections */
      DXSetError(ERROR_MISSING_DATA,"#10250","given glyph", "connections");
      return ERROR;
    }
    
    attr = DXGetString((String)DXGetComponentAttribute((Field)glyphfield, 
						   "connections",
						   "element type"));
    if (strcmp(attr,"triangles")&&(strcmp(attr,"lines"))) {
      /* only triangles supported for given glyph */
      DXSetError(ERROR_DATA_INVALID,
		 "only lines and triangles supported for given glyph");
      return ERROR;
    }
    
    DXGetArrayInfo(positions, numpoints, &type, &category, &rank, shape);
    if ((type != TYPE_FLOAT) || (category != CATEGORY_REAL)) {
      DXSetError(ERROR_DATA_INVALID,"#10330", "given glyph positions");
      return ERROR;
    }
    if ((rank != 1) || ((shape[0] != 3) && (shape[0] != 2))) {
      DXSetError(ERROR_DATA_INVALID,"#10340","given glyph positions");
      return ERROR;
    }

    *out_pos_dim = shape[0];



    DXGetArrayInfo(connections, numconn, &type, &category, &rank, shape);
   
    if (*hasnormals)  {
      if (*out_pos_dim == 2) {
	  /* normals are invalid for 2D positions */
	  DXSetError(ERROR_DATA_INVALID, "#11817");
	  goto error;
      }
      if (strcmp(DXGetString((String)DXGetAttribute((Object)normals,"dep")), 
		 "positions")) {
         DXSetError(ERROR_DATA_INVALID,
             "normals of given glyph must be dependent on positions");
         goto error;
      }
      DXGetArrayInfo(normals, numpoints, &type, &category, &rank, shape);
      if ((type != TYPE_FLOAT) || (category != CATEGORY_REAL) 
  	|| (rank != 1) || (shape[0] != 3)) {
        /* given glyph must have 3D float normals */
        DXSetError(ERROR_DATA_INVALID,"#10331", "given glyph normals");
        return ERROR;
      }
    }

   /* glyphs are defined in 3D (even flat ones). This makes the use of
    * transformation matrices and DXApply possible for all cases. The z
    * dimension is stripped off of the flat glyphs before placing in the 
    * field (this happens inside of GlyphXform2D */

    /* need to xform the positions and normals (if present) using whatever
       xform we've got */ 
    *returnpoints = 
        (Point *)DXAllocateLocal((*numpoints)*3*sizeof(float));
    if (!(*returnpoints))
      return ERROR;
    if (!strcmp(attr,"triangles")) {
      strcpy(conntype,"triangles");
      *returnconns = (Triangle *)DXAllocateLocal((*numconn)*3*sizeof(int));
      if (!(*returnconns)) {
	DXFree((Pointer)*returnpoints);
	return ERROR;
      }
      con_ptr = (Triangle *)DXGetArrayData(connections);
    }
    else {
      strcpy(conntype,"lines");
      *returnlines = (Line *)DXAllocateLocal((*numconn)*2*sizeof(int));
      if (!(*returnlines)) {
	DXFree((Pointer)*returnpoints);
	return ERROR;
      }
      line_ptr = (Line *)DXGetArrayData(connections);
    }
    if (*hasnormals) {
      *returnnormals = (Point *)DXAllocateLocal((*numpoints)*3*sizeof(float));
      if (!(*returnnormals)) {
        DXFree((Pointer)*returnpoints);
        DXFree((Pointer)*returnconns);
        return ERROR;
      }
    }

    pos_ptr = (float *)DXGetArrayData(positions);
    if (*hasnormals) norm_ptr = (Point *)DXGetArrayData(normals);

    /* fill up the returned connections array */
    if (!strcmp(attr,"triangles")) {
      for (i=0, temp_conn = *returnconns; 
           i < *numconn; 
          i++, temp_conn++) {
          *temp_conn = con_ptr[i];
      }
    }
    else {
      for (i=0, temp_line = *returnlines; 
           i < *numconn; 
          i++, temp_line++) {
          *temp_line = line_ptr[i];
      }
    }

    /* now do the positions */
    switch (*out_pos_dim) {
    case (3):
      for (i=0,temp_pos = *returnpoints,temp_norm = *returnnormals;
	   i<*numpoints;
	   i++, temp_pos++, temp_norm++) {
	*temp_pos = DXPt(pos_ptr[3*i], pos_ptr[3*i+1], pos_ptr[3*i+2]);
	if (*hasnormals) *temp_norm = norm_ptr[i];
      }
      if (*anyxforms == 1) {
	if (!GlyphXform(*xform, *numpoints, 
                        *returnpoints, *returnpoints))
	  goto error;
	if (*hasnormals) {
	  if (!GlyphNormalXform(*xform, *numpoints, *returnnormals, 
				*returnnormals))
	    goto error;
	}
      }
      break;

    case (2):
      for (i=0,temp_pos = *returnpoints; i<*numpoints;
	   i++, temp_pos++) {
	*temp_pos = DXPt(pos_ptr[2*i], pos_ptr[2*i+1], 0.0);
      }
      if (*anyxforms == 1) {
	if (!GlyphXform(*xform,  *numpoints, *returnpoints, *returnpoints))
	  goto error;
      }

      break;
    }
     

    
    return OK;
    break;

  case CLASS_GROUP:
    /* glyph object must be a single field */
    DXSetError(ERROR_DATA_INVALID,"#10191", "given glyph");
    return ERROR;
    break;
    
  case CLASS_XFORM:
    *anyxforms = 1;
    if (!(DXGetXformInfo((Xform)glyphfield, &subfield, &newxform)))
      return ERROR;
    /* concatenate latest matrix onto what came in */
    newxform = DXConcatenate(*xform, newxform);
    
    if (!GetGlyphField(subfield, numpoints, numconn, returnpoints, 
		       returnconns, returnlines, returnnormals, 
                       &newxform, anyxforms,
                       hasnormals, out_pos_dim, conntype))
      return ERROR;
    
    break;
    
  default:
    DXSetError(ERROR_DATA_INVALID,"#10191", "given glyph");
    return ERROR;
    break;
    
  }
  return OK;

  error:
     DXFree((Pointer)*returnpoints);
     if (*hasnormals) DXFree((Pointer)*returnnormals);
     DXFree((Pointer)*returnconns);
     return ERROR;
}




/*
 * This routine returns a rotation matrix that takes an arbitrary       
 * vector V to another vector W, through the planes that contains
 * them both.
 * Note:  this routine should check for 0 vectors and return ERROR somehow
 *
 */

static 
Matrix 
GlyphAlign (Vector V, Vector W)
{
    Matrix Q, R, Qt;
    Vector M, N, NN, Z, Wp, Wpz90; 

    /* do not process NULL vectors */
    if (DXLength(V) == 0.0 || DXLength(W) == 0.0) return Identity;

    /* normalize in case incoming vectors are not */
    V = DXNormalize(V);
    W = DXNormalize(W);
    
    N = DXCross(V, W);
    if (DXLength(N) == 0.0) {
	/* we must find an arbitrary perpindicular vector */
        if (V.x==0)
	    N = DXVec(0, -V.z, V.y);
	else
	    N = DXVec(-V.y, V.x, 0);
    }


    NN = DXNormalize(N);
    M = DXNormalize(DXCross(NN, V));

    Q = DXMat(V.x, V.y, V.z,
	      M.x, M.y, M.z,
	      NN.x, NN.y, NN.z,
	      0.0, 0.0, 0.0);

    Qt = DXTranspose(Q);

    Wp = DXApply(W, Qt);
    Wpz90 = DXApply(Wp, DXRotateZ(90*DEG));

    Z.x = 0.0;
    Z.y = 0.0;
    Z.z = 1.0;

    R = DXMat(Wp.x, Wp.y, Wp.z,
	    Wpz90.x, Wpz90.y, Wpz90.z,
	    Z.x, Z.y, Z.z,
	    0.0, 0.0, 0.0);

    return DXConcatenate(DXConcatenate(Qt, R), Q);
}


static Error
GetEigenVectors(Stress_Tensor st, Vector *eigen_vector)
{
	int i, j;
	float *d, **v, **e;

	d = _dxfEigenVector(1, SHAPE);
	v = _dxfEigenMatrix(1, SHAPE, 1, SHAPE);
	e = _dxfEigenConvertMatrix(&st.tau[0][0], 1, SHAPE, 1, SHAPE);
	if ( DXGetError() != ERROR_NONE ) return ERROR;
	if (!_dxfEigen(e, SHAPE, d, v)) return ERROR;
	for (i=0, j=1; j<=SHAPE; j++, i++) {
		eigen_vector[i].x = v[1][j];
		eigen_vector[i].y = v[2][j];
		eigen_vector[i].z = v[3][j];
		eigen_vector[i] = DXNormalize(eigen_vector[i]);
		eigen_vector[i] = DXMul(eigen_vector[i], d[j]);
	}
	_dxfEigenFreeConvertMatrix(e, 1);
	_dxfEigenFreeMatrix(v, 1, SHAPE, 1);
	_dxfEigenFreeVector(d, 1);
	return OK;
}

static Error
GetEigenVectors2(Stress_Tensor2 st, Vector *eigen_vector)
{
	int i, j;
	float *d, **v, **e;

	d = _dxfEigenVector(1, 2);
	v = _dxfEigenMatrix(1, 2, 1, 2);
	e = _dxfEigenConvertMatrix(&st.tau[0][0], 1, 2, 1, 2);
	if ( DXGetError() != ERROR_NONE ) return ERROR;
	if (!_dxfEigen(e, 2, d, v)) return ERROR;
	for (i=0, j=1; j<=2; j++, i++) {
		eigen_vector[i].x = v[1][j];
		eigen_vector[i].y = v[2][j];
		eigen_vector[i].z = v[3][j];
		eigen_vector[i] = DXNormalize(eigen_vector[i]);
		eigen_vector[i] = DXMul(eigen_vector[i], d[j]);
	}
	_dxfEigenFreeConvertMatrix(e, 1);
	_dxfEigenFreeMatrix(v, 1, 2, 1);
	_dxfEigenFreeVector(d, 1);
	return OK;
}

static 
Error GetNumItems(Object o, int *numitems)
{
  int items, i;
  Array positions;
  Object child;

  switch (DXGetObjectClass((Object)o)) {
  case CLASS_FIELD:
     if (DXEmptyField((Field)o)) return OK;
     positions = (Array)DXGetComponentValue((Field)o,"positions");
     DXGetArrayInfo((Array)positions, &items, NULL, NULL, NULL, NULL);
     *numitems += items;
     break;
  case CLASS_GROUP:
     for (i=0; (child=DXGetEnumeratedMember((Group)o,i,NULL)); i++) {
        GetNumItems(child, numitems);
     }
     break;
  default:
     break;
  }

  return OK;

}

 
Error
_dxfWidthHeuristic(Object o, float *size)
{
    float bounding_width;
    Point box[8];
    int items;
    double len;
    float x, y;

    /* preliminary checks of incoming object */
    if (!DXValidPositionsBoundingBox(o, box)) {
      /* probably all invalid. That should be ok */
      return OK;
    }
    len = DXLength(DXSub(box[7], box[0]));
    if (len == 0.0) {
      *size = 1.0;
      return OK;
    }
   
    items = 0; 
    /* get total number of valid Positions from group or field */
    if (!GetNumberValidPositions(o, &items))
      return ERROR; 
    
      
    /* len was calculated first thing and is used again here */
    x = len;
    y = (3.4*pow((double) items, 1.0/3.0));
    if (DivideByZero(x, y)) {
	DXWarning ("Near Division by zero guessing Glyph size.");
	return ERROR;
     } else {
	bounding_width = x / y;
     }
     *size = bounding_width/2.0;
     return OK;
  }

static Error GetNumberValidPositions(Object o, int *items)
{
  Array inv_positions, positions;
  int numitems, i;
  InvalidComponentHandle ih;
  Object subo;

  switch (DXGetObjectClass(o)) {
  case (CLASS_FIELD):
     inv_positions= (Array)DXGetComponentValue((Field)o,"invalid positions");
     if (!inv_positions) {
       positions = (Array)DXGetComponentValue((Field)o,"positions");
       if (positions) {
          DXGetArrayInfo((Array)positions, &numitems, NULL, NULL, NULL, NULL);
       }
       else {
          numitems = 0;
       }
       *items = *items + numitems;
     }
     else {
       ih = DXCreateInvalidComponentHandle(o, NULL, "positions");
       if (!ih)
          return ERROR;
       numitems = DXGetValidCount(ih);
       *items = *items + numitems;
       DXFreeInvalidComponentHandle(ih);
     } 

     break;
  case (CLASS_GROUP):
     for (i=0; (subo=DXGetEnumeratedMember((Group)o, i, NULL)); i++) {
       if (!GetNumberValidPositions(subo, items))
          return ERROR;
     }
     break;
  case (CLASS_CLIPPED):
     if (!(DXGetClippedInfo((Clipped)o, &subo, NULL)))
          return ERROR;
     if (!GetNumberValidPositions(subo, items))
          return ERROR;
     break;
  case (CLASS_XFORM):
     if (!DXGetXformInfo((Xform)o, &subo, NULL))
          return ERROR;
     if (!GetNumberValidPositions(subo, items))
          return ERROR;
     break;
  default:
     DXSetError(ERROR_DATA_INVALID,"width heuristic can not operate");
     return ERROR;

  }
  return OK;


}

static Error GetGlyphName(float quality, char *glyph_rank, 
                          int position_rank, char *given_type_string, 
                          char *type_string, char *overridetype_string, 
                          int *max_shape)
/* this subroutine figures out what name of glyph we are going to be using.
 * it bases this on the dimensionality of the positions, the dimensionality
 * of the data, and the given glyphtype or quality factor. Flat glyphs are used
 * for 1D or 2D positions, provided the data is scalar or 2-vector. In all
 * other cases, non-flat glyphs are used. max_shape is the dimensionality
 * of the output glyphs: max_shape = 2 for flat glyphs, and 3 otherwise. For
 * non-scalar glyphs, this routine also provides the name of the "override"
 * glyphs (small circle or sphere of some sort). The user can override the
 * default choice of "flat" or "round" by explicitly specifying a glyphname:
 * diamond, sphere, arrow, needle, circle, arrow2D, needle2D, rocket2D, 
 * diamond2D */
{
 
  if (!strcmp(glyph_rank,"text")) {
     /* do nothing */
  }
  else if (!strcmp(glyph_rank,"scalar")) {
    if (position_rank == 3)
      *max_shape = 3;
    else 
      *max_shape = 2; 
  }
  else if (!strcmp(glyph_rank,"2-vector")) {
    if (position_rank == 3)
      *max_shape = 3;
    else 
      *max_shape = 2; 
  }
  else {
    *max_shape = 3;
  } 
  
  /* for 3D glyphs, figure out the glyph name */ 
  if (*max_shape == 3) {
    /* the user has provided a string describing the glyph */
    if (quality == -1) {
      if (!strcmp("spiffy",given_type_string)) {
	if (!strcmp(glyph_rank,"scalar")) 
	  strcpy(type_string,"sphere266");
	else {
	  strcpy(type_string,"rocket20");
	  strcpy(overridetype_string,"sphere146");
        }
      }
      else if (!strcmp("speedy",given_type_string)) {
	if (!strcmp(glyph_rank,"scalar")) 
	  strcpy(type_string,"diamond");
	else {
	  strcpy(type_string,"needle");
	  strcpy(overridetype_string,"point");
	}
      }
      else if (!strcmp("standard",given_type_string)) {
	if (!strcmp(glyph_rank,"scalar")) 
	  strcpy(type_string,"sphere62");
	else {
	  strcpy(type_string,"rocket6");
	  strcpy(overridetype_string,"sphere62");
        }
      }
      else if (!strcmp("circle",given_type_string)) {
	if (!strcmp(glyph_rank,"scalar")) {
	  strcpy(type_string,"circle40");
        }
	else {
           DXSetError(ERROR_BAD_PARAMETER,
                    "circle is inappropriate for non-scalar data");
           return ERROR;
        }
      }
      else if (!strcmp("sphere",given_type_string)) {
	if (!strcmp(glyph_rank,"scalar")) 
	  strcpy(type_string,"sphere266");
	else {
	  DXSetError(ERROR_BAD_PARAMETER,
		   "sphere is inappropriate for non-scalar data");
	  return ERROR;
	}
      }
      else if (!strcmp("cube",given_type_string)) {
	if (!strcmp(glyph_rank,"scalar")) 
	  strcpy(type_string,"cube");
	else {
	  DXSetError(ERROR_BAD_PARAMETER,
		   "cube is inappropriate for non-scalar data");
	  return ERROR;
	}
      }
      else if (!strcmp("diamond",given_type_string)) {
	if (!strcmp(glyph_rank,"scalar")) 
	  strcpy(type_string,"diamond");
	else {
	  DXSetError(ERROR_BAD_PARAMETER,
		   "diamond is inappropriate for non-scalar data");
	  return ERROR;
	}
      }
      else if (!strcmp("square",given_type_string)) {
	if (!strcmp(glyph_rank,"scalar")) 
	  strcpy(type_string,"square");
	else {
	  DXSetError(ERROR_BAD_PARAMETER,
		   "square is inappropriate for non-scalar data");
	  return ERROR;
	}
      }
      else if (!strcmp("diamond2d",given_type_string)) {
	if (!strcmp(glyph_rank,"scalar")) 
	  strcpy(type_string,"circle4");
	else {
	  DXSetError(ERROR_BAD_PARAMETER,
		   "diamond2d is inappropriate for non-scalar data");
	  return ERROR;
	}
      }
      else if (!strcmp("rocket",given_type_string)) {
	if (!strcmp(glyph_rank,"scalar")) {
	  DXSetError(ERROR_BAD_PARAMETER,
		   "rocket is an inappropriate glyph type for scalar data");
	  return ERROR;
        }
	else {
	  strcpy(type_string,"rocket20");
	  strcpy(overridetype_string,"sphere146");
	}
      }
      else if (!strcmp("rocket2d",given_type_string)) {
	if (!strcmp(glyph_rank,"scalar")) {
	  DXSetError(ERROR_BAD_PARAMETER,
		   "rocket2d is an inappropriate glyph type for scalar data");
	  return ERROR;
        }
	else {
	  strcpy(type_string,"rocket2d");
	  strcpy(overridetype_string,"circle20");
	}
      }
      else if (!strcmp("arrow",given_type_string)) {
	if (!strcmp(glyph_rank,"scalar")) {
	  DXSetError(ERROR_BAD_PARAMETER,
		   "arrow is an inappropriate glyph type for scalar data");
	  return ERROR;
        }
	else {
	  strcpy(type_string,"arrow");
	  strcpy(overridetype_string,"point");
	}
      }
      else if (!strcmp("arrow2d",given_type_string)) {
	if (!strcmp(glyph_rank,"scalar")) {
	  DXSetError(ERROR_BAD_PARAMETER,
		   "arrow2d is an inappropriate glyph type for scalar data");
	  return ERROR;
        }
	else {
	  strcpy(type_string,"arrow2d");
	  strcpy(overridetype_string,"point");
	}
      }
      else if (!strcmp("needle",given_type_string)) {
	if (!strcmp(glyph_rank,"scalar")) {
	  DXSetError(ERROR_BAD_PARAMETER,
		   "needle is an inappropriate glyph type for scalar data");
	  return ERROR;
        }
	else {
	  strcpy(type_string,"needle");
	  strcpy(overridetype_string,"point");
	}
      }
      else if (!strcmp("needle2d",given_type_string)) {
	if (!strcmp(glyph_rank,"scalar")) {
	  DXSetError(ERROR_BAD_PARAMETER,
		   "needle2d is an inappropriate glyph type for scalar data");
	  return ERROR;
        }
	else {
	  strcpy(type_string,"needle2d");
	  strcpy(overridetype_string,"point");
	}
      }
      else if (!strcmp("tensor",given_type_string)) {
	if (!strcmp(glyph_rank,"matrix")) {
	  strcpy(type_string,"rocket20");
	  strcpy(overridetype_string,"sphere146");
        }
	else if (!strcmp(glyph_rank,"matrix2")) {
	  strcpy(type_string,"rocket20");
	  strcpy(overridetype_string,"sphere146");
        }
	else {
	  DXSetError(
		   ERROR_BAD_PARAMETER,
		  "tensor is an inappropriate glyph type for non-tensor data");
	  return ERROR;
	}
      }
      else {
	/* %s is an unknown glyph type */
	DXSetError(ERROR_DATA_INVALID,"#11818", given_type_string);
       return ERROR;
      }
    }
    /* else, for 3D glyphs, quality is given as a floating point number  */
    else {
      if (quality <= 0.11) {
        if (!strcmp(glyph_rank,"scalar"))
          strcpy(type_string,"diamond");
        else {
          strcpy(type_string,"needle");
          strcpy(overridetype_string,"point");
        }
      }
      else if (quality <= 0.21) {
        if (!strcmp(glyph_rank,"scalar"))
          strcpy(type_string,"sphere14");
        else {
          strcpy(type_string,"arrow");
          strcpy(overridetype_string,"point");
        }
      }
      else if (quality <= 0.31) {
        if (!strcmp(glyph_rank,"scalar"))
          strcpy(type_string,"sphere26");
        else {
          strcpy(type_string,"rocket3");
          strcpy(overridetype_string,"sphere26");
        }
      }
      else if (quality <= 0.41) {
        if (!strcmp(glyph_rank,"scalar"))
          strcpy(type_string,"sphere42");
        else {
          strcpy(type_string,"rocket4");
          strcpy(overridetype_string,"sphere42");
        }
      }
      else if (quality <= 0.51) {
        if (!strcmp(glyph_rank,"scalar"))
          strcpy(type_string,"sphere62");
        else {
          strcpy(type_string,"rocket6");
          strcpy(overridetype_string,"sphere62");
        }
      }
      else if (quality <= 0.61) {
        if (!strcmp(glyph_rank,"scalar"))
          strcpy(type_string,"sphere114");
        else {
          strcpy(type_string,"rocket8");
          strcpy(overridetype_string,"sphere114");
        }
      }
      else if (quality <= 0.71) {
        if (!strcmp(glyph_rank,"scalar"))
          strcpy(type_string,"sphere114");
        else {
          strcpy(type_string,"rocket12");
          strcpy(overridetype_string,"sphere114");
        }
      }
      else if (quality <= 0.91) {
        if (!strcmp(glyph_rank,"scalar"))
          strcpy(type_string,"sphere146");
        else {
          strcpy(type_string,"rocket20");
          strcpy(overridetype_string,"sphere146");
        }
      }
      else {
        if (!strcmp(glyph_rank,"scalar"))
          strcpy(type_string,"sphere266");
        else {
          strcpy(type_string,"rocket20");
          strcpy(overridetype_string,"sphere146");
        }
      }
    }
  }
  /* else 2D glyphs are the right answer */
  else {
    /* explicitly given name */
    if (quality == -1) {
      if (!strcmp("spiffy",given_type_string)) {
        if (!strcmp(glyph_rank,"scalar"))
          strcpy(type_string,"circle40");
        else {
          strcpy(type_string,"rocket2d");
          strcpy(overridetype_string,"circle40");
        }
      }
      else if (!strcmp("speedy",given_type_string)) {
        if (!strcmp(glyph_rank,"scalar"))
          strcpy(type_string,"circle4");
        else {
          strcpy(type_string,"needle2d");
          strcpy(overridetype_string,"point");
        }
      }
      else if (!strcmp("standard",given_type_string)) {
        if (!strcmp(glyph_rank,"scalar"))
          strcpy(type_string,"circle10");
        else {
          strcpy(type_string,"rocket2d");
          strcpy(overridetype_string,"circle10");
        }
      }
      else if (!strcmp("circle",given_type_string)) {
        if (!strcmp(glyph_rank,"scalar")) {
          strcpy(type_string,"circle40");
        }
        else {
           DXSetError(ERROR_BAD_PARAMETER,
                    "circle is inappropriate for non-scalar data");
           return ERROR;
        }
      }
      else if (!strcmp("square",given_type_string)) {
        if (!strcmp(glyph_rank,"scalar")) {
          strcpy(type_string,"square");
        }
        else {
           DXSetError(ERROR_BAD_PARAMETER,
                    "square is inappropriate for non-scalar data");
           return ERROR;
        }
      }
      else if (!strcmp("cube",given_type_string)) {
        if (!strcmp(glyph_rank,"scalar")) {
          strcpy(type_string,"cube");
        }
        else {
           DXSetError(ERROR_BAD_PARAMETER,
                    "cube is inappropriate for non-scalar data");
           return ERROR;
        }
        /* need to override *max_shape */
        *max_shape = 3;
      }
      else if (!strcmp("sphere",given_type_string)) {
        if (!strcmp(glyph_rank,"scalar")) {
          strcpy(type_string,"sphere146");
        }
        else {
           DXSetError(ERROR_BAD_PARAMETER,
                    "sphere is inappropriate for non-scalar data");
           return ERROR;
        }
        /* need to override *max_shape */
        *max_shape = 3;
      }
      else if (!strcmp("diamond",given_type_string)) {
        if (!strcmp(glyph_rank,"scalar"))
          strcpy(type_string,"diamond");
        else {
          DXSetError(ERROR_BAD_PARAMETER,
                   "diamond is inappropriate for non-scalar data");
          return ERROR;
        }
        /* need to override *max_shape */
        *max_shape = 3;
      }
      else if (!strcmp("diamond2d",given_type_string)) {
        if (!strcmp(glyph_rank,"scalar"))
          strcpy(type_string,"circle4");
        else {
          DXSetError(ERROR_BAD_PARAMETER,
                   "diamond2d is inappropriate for non-scalar data");
          return ERROR;
        }
      }
      else if (!strcmp("rocket2d",given_type_string)) {
        if (!strcmp(glyph_rank,"scalar")) {
          DXSetError(ERROR_BAD_PARAMETER,
                   "rocket2d is an inappropriate glyph type for scalar data");
          return ERROR;
        }
        else {
          strcpy(type_string,"rocket2d");
          strcpy(overridetype_string,"circle40");
        }
      }
      else if (!strcmp("rocket",given_type_string)) {
        if (!strcmp(glyph_rank,"scalar")) {
          DXSetError(ERROR_BAD_PARAMETER,
                   "rocket is an inappropriate glyph type for scalar data");
          return ERROR;
        }
        else {
          strcpy(type_string,"rocket20");
          strcpy(overridetype_string,"sphere146");
        }
        /* need to override *max_shape */
        *max_shape = 3;
      }
      else if (!strcmp("arrow2d",given_type_string)) {
        if (!strcmp(glyph_rank,"scalar")) {
          DXSetError(ERROR_BAD_PARAMETER,
                   "arrow2d is an inappropriate glyph type for scalar data");
          return ERROR;
        }
        else {
          strcpy(type_string,"arrow2d");
          strcpy(overridetype_string,"point");
        }
      }
      else if (!strcmp("arrow",given_type_string)) {
        if (!strcmp(glyph_rank,"scalar")) {
          DXSetError(ERROR_BAD_PARAMETER,
                   "arrow is an inappropriate glyph type for scalar data");
          return ERROR;
        }
        else {
          strcpy(type_string,"arrow");
          strcpy(overridetype_string,"point");
        }
        /* need to override *max_shape */
        *max_shape = 3;
      }
      else if (!strcmp("needle2d",given_type_string)) {
        if (!strcmp(glyph_rank,"scalar")) {
          DXSetError(ERROR_BAD_PARAMETER,
                   "needle2d is an inappropriate glyph type for scalar data");
          return ERROR;
        }
        else {
          strcpy(type_string,"needle2d");
          strcpy(overridetype_string,"point");
        }
      }
      else if (!strcmp("needle",given_type_string)) {
        if (!strcmp(glyph_rank,"scalar")) {
          DXSetError(ERROR_BAD_PARAMETER,
                   "needle is an inappropriate glyph type for scalar data");
          return ERROR;
        }
        else {
          strcpy(type_string,"needle");
          strcpy(overridetype_string,"point");
        }
        /* need to override *max_shape */
        *max_shape = 3;
      }
      else if (!strcmp("tensor",given_type_string)) {
        if (!strcmp(glyph_rank,"matrix")) {
          strcpy(type_string,"rocket2d");
          strcpy(overridetype_string,"circle20");
        }
        else if (!strcmp(glyph_rank,"matrix2")) {
          strcpy(type_string,"rocket2d");
          strcpy(overridetype_string,"circle20");
        }
        else {
          DXSetError(
                   ERROR_BAD_PARAMETER,
                   "tensor is an inappropriate glyph type for non-tensor data");
          return ERROR;
        }
      }
      else {
        DXSetError(ERROR_DATA_INVALID,"%s is an unknown glyph type",
                 given_type_string);
        return ERROR;
      }
    }
    /* else 2D glyphs, given quality factor */
    else {
      if (quality <= 0.11) {
        if (!strcmp(glyph_rank,"scalar"))
          strcpy(type_string,"circle4");
        else {
          strcpy(type_string,"needle2d");
          strcpy(overridetype_string,"point");
        }
      }
      else if (quality <= 0.21) {
        if (!strcmp(glyph_rank,"scalar"))
          strcpy(type_string,"circle6");
        else {
          strcpy(type_string,"arrow2d");
          strcpy(overridetype_string,"point");
        }
      }
      else if (quality <= 0.31) {
        if (!strcmp(glyph_rank,"scalar"))
          strcpy(type_string,"circle6");
        else {
          strcpy(type_string,"rocket2d");
          strcpy(overridetype_string,"circle10");
        }
      }
      else if (quality <= 0.41) {
        if (!strcmp(glyph_rank,"scalar"))
          strcpy(type_string,"circle8");
        else {
          strcpy(type_string,"rocket2d");
          strcpy(overridetype_string,"circle8");
        }
      }
      else if (quality <= 0.61) {
        if (!strcmp(glyph_rank,"scalar"))
          strcpy(type_string,"circle10");
        else {
          strcpy(type_string,"rocket2d");
          strcpy(overridetype_string,"circle10");
        }
      }
      else if (quality <= 0.81) {
        if (!strcmp(glyph_rank,"scalar"))
          strcpy(type_string,"circle20");
        else {
          strcpy(type_string,"rocket2d");
          strcpy(overridetype_string,"circle20");
        }
      }
      else {
        if (!strcmp(glyph_rank,"scalar"))
          strcpy(type_string,"circle40");
        else {
          strcpy(type_string,"rocket2d");
          strcpy(overridetype_string,"circle40");
        }
      }
    }
  }
  return OK;
}


Error _dxfIndexGlyphObject(Object object, Object glyphs, float scale)
{
  int i;
  Object child;
  
  switch (DXGetObjectClass(object)) {
  case (CLASS_FIELD):
    if (!IndexGlyphField((Field)object, glyphs, scale))
      return ERROR;
    return OK;
    break;
    
  case (CLASS_GROUP):
    for (i=0; (child=DXGetEnumeratedMember((Group)object,i,NULL)); i++) {
      if (!_dxfIndexGlyphObject(child, glyphs, scale))
	return ERROR;
    }
    return OK;
    break;
    
  case (CLASS_CLIPPED): 
    if (!(DXGetClippedInfo((Clipped)object, &child, NULL)))
      return ERROR;
    if (!_dxfIndexGlyphObject(child, glyphs, scale))
      return ERROR;
    break;
    
  default:
    /* (shouldn't be xforms, cause I called ApplyTransform) */
    DXSetError(ERROR_DATA_INVALID,"must be a field or a group");
    return ERROR;
  }
  return OK;
}

static Error IndexGlyphField(Field field, Object glyphs, float scale)
{
  Array positions=NULL, data; 
  Array newpositions=NULL, newconnections=NULL, newcolors=NULL; 
  Array newnormals=NULL; 
  Array glyphpositions, glyphconnections, glyphcolors, glyphnormals;
  int numdata, datarank, datashape[8], posrank, posshape[8]; 
  int ipos, icon, iclr, inml, numprev, hasnormals=0;
  int newshape, glyphshape=0, i, j, dataindex=0, numglyphpos, numglyphcon;
  int colorrank, colorshape[8], posted =0, totalpoints;
  int *map;
  Type datatype, postype, colortype;
  Field glyphtouse;
  RGBColor *glyphcolor_ptr=NULL;
  Point *glyphnorm_ptr=NULL, normal;
  float *scratch=NULL, *dp;
  Point *dp3;
  Point2D *dp2;
  ArrayHandle handle=NULL;
  InvalidComponentHandle invalidhandle=NULL;
  Triangle oldtri, newtri;
  Line oldln, newln;
  Point pos3D, glyphpos;
  float *glyphpos_ptr;
  int *glyphcon_ptr;
  RGBColor color;
  Category datacategory, poscategory, colorcategory;
  int *data_ptr_i=NULL;
  uint *data_ptr_ui=NULL;
  short *data_ptr_s=NULL;
  ushort *data_ptr_us=NULL;
  byte *data_ptr_b=NULL;
  ubyte *data_ptr_ub=NULL;
  char *dataattr, *normattr=NULL, *colattr=NULL, glyphelementtype[30];
  
  /* XXX need to pass through data etc */
     
  if (!DXGetObjectClass(glyphs)==CLASS_GROUP) {
    DXSetError(ERROR_INTERNAL, "glyphs expected to be a group");
    return ERROR;
  }
  
  if (DXEmptyField(field))
    return OK;
  
  data = (Array)DXGetComponentValue(field, "data");
  if (!data) {
    DXSetError(ERROR_MISSING_DATA,"#10250", "data", "data");
    return ERROR;
  } 
  
  if (!DXGetArrayInfo(data, &numdata, &datatype, &datacategory, 
		      &datarank, datashape))
    return ERROR;
  
  /* check the dependency of the data component */
  dataattr = DXGetString((String)DXGetComponentAttribute((Field)field,
							 "data", 
							 "dep"));
  if ((strcmp(dataattr,"positions")&&strcmp(dataattr,"connections"))) {
    DXSetError(ERROR_DATA_INVALID,
	       "data must be dependent either on positions or on connections");
    goto error;
  }
  
  posted = 0;




  if (!strcmp(dataattr,"connections")) {
    /* I need to check whether the positions are regular. If they
     * are, a simple grid shift is in order. Otherwise, use PostArray
     * to shift them */
    Array pos, con;
    int counts[8], n; 
    float posorigin[8], posdeltas[8];
    pos = (Array)DXGetComponentValue((Field)field,"positions");
    con = (Array)DXGetComponentValue((Field)field,"connections");
    if ((DXQueryGridPositions(pos, &n, counts, posorigin, posdeltas)) &&
	DXQueryGridConnections(con, NULL, NULL)) {
      for (i=0; i<n; i++) {
	for (j=0; j<n; j++) {
	  if (counts[j] > 1) {
	    posorigin[i]+=0.5*posdeltas[i + j*n];
	  }
	}
      }
      for (i=0; i<n; i++) {
	if (counts[i] >1) counts[i] = counts[i]-1;
      }
      positions = DXMakeGridPositionsV(n, counts, posorigin, posdeltas);
    }
    
    /* else the positions are irregular */
    else {
      positions = _dxfPostArray((Field)field, "positions", dataattr);
    }
    
    
    if (!positions) {
      DXSetError(ERROR_BAD_PARAMETER,
		 "could not compute glyph positions for cell-centered data");
      return ERROR;
    }
    posted = 1;
  }
  else if (!strcmp(dataattr, "faces")) {
    positions = _dxfPostArray((Field)field, "positions", dataattr);
    if (!positions) {
      DXAddMessage("could not compute glyph positions for face-centered data");
      return ERROR;
    }
    posted = 1;
  }
  /* else dep positions */
  else {
    positions = (Array)DXGetComponentValue(field, "positions");
    if (!positions) {
      DXSetError(ERROR_MISSING_DATA,"#10250", "data", "positions");
      goto error;
    }
  }

  
  if ((datatype != TYPE_INT)&&(datatype != TYPE_UINT)&&
      (datatype != TYPE_SHORT)&&(datatype != TYPE_USHORT)&&
      (datatype != TYPE_BYTE)&&(datatype != TYPE_UBYTE)) {
    DXSetError(ERROR_DATA_INVALID,
	       "for index glyphs, data type must be of an integral type");
    goto error;
  }
  if (!((datarank==0)||((datarank==1)&&(datashape[0]==1)))) {
    DXSetError(ERROR_DATA_INVALID,"for index glyphs, data must be scalar");
    goto error;
  }
  
  /* XXX regular data? */
  switch (datatype) {
  case (TYPE_INT):
    data_ptr_i = (int *)DXGetArrayData(data);
    break;
  case (TYPE_UINT):
    data_ptr_ui = (uint *)DXGetArrayData(data);
    break;
  case (TYPE_SHORT):
    data_ptr_s = (short *)DXGetArrayData(data);
    break;
  case (TYPE_USHORT):
    data_ptr_us = (ushort *)DXGetArrayData(data);
    break;
  case (TYPE_BYTE):
    data_ptr_b = (byte *)DXGetArrayData(data);
    break;
  case (TYPE_UBYTE):
    data_ptr_ub = (ubyte *)DXGetArrayData(data);
    break;
  default:
    break;
  }


  if (!DXGetArrayInfo(positions, NULL, &postype, &poscategory, 
		      &posrank, posshape))
    goto error;
  
  if (posrank != 1) {
    DXSetError(ERROR_DATA_INVALID,"positions of data must be rank 1");
    goto error;
  }
  if ((posshape[0] < 0)||(posshape[0]>3)) {
    DXSetError(ERROR_DATA_INVALID,"positions of data must be 1, 2, or 3-D");
    goto error;
  }
  
  if (postype != TYPE_FLOAT) {
    DXSetError(ERROR_DATA_INVALID,"positions component must be type float");
    goto error;
  }
  
  if (!CheckGlyphGroup(glyphs, &glyphshape, &hasnormals, glyphelementtype))
    goto error;
  
  ipos = 0;
  icon = 0;
  iclr = 0;
  inml = 0;
  /*idata = 0;*/
  
  newshape = (posshape[0] >= glyphshape) ? posshape[0] : glyphshape;
  
  newpositions = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, newshape);
  if (!strcmp(glyphelementtype,"triangles"))
     newconnections = DXNewArray(TYPE_INT, CATEGORY_REAL, 1, 3);
  else
     newconnections = DXNewArray(TYPE_INT, CATEGORY_REAL, 1, 2);
  newcolors = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, 3);
  if (hasnormals) {
    newnormals = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, 3);
    if (!newnormals) goto error;
  }
  
  if ((!newpositions)||(!newconnections)||(!newcolors))
    goto error;
  
  
  /* prepare to access the positions component */
  if (!(handle = DXCreateArrayHandle(positions)))
    goto error;
  scratch = DXAllocateLocal(posshape[0]*sizeof(float));
  if (!scratch)
    goto error;


  /* set up the invalid component stuff */
  if (!(invalidhandle = DXCreateInvalidComponentHandle((Object)field,
                                                       NULL,dataattr)))
     goto error;
  if (!invalidhandle)
     goto error;


  numprev = 0;
  map = (int *)DXAllocateLocalZero(numdata*sizeof(int));
  for (i=0; i<numdata; i++) {
    if (DXIsElementValidSequential(invalidhandle, i)) {
    switch (datatype) {
    case (TYPE_INT):
      dataindex = (int)data_ptr_i[i];
      break;
    case (TYPE_UINT):
      dataindex = (int)data_ptr_ui[i];
      break;
    case (TYPE_SHORT):
      dataindex = (int)data_ptr_s[i];
      break;
    case (TYPE_USHORT):
      dataindex = (int)data_ptr_us[i];
      break;
    case (TYPE_BYTE):
      dataindex = (int)data_ptr_b[i];
      break;
    case (TYPE_UBYTE):
      dataindex = (int)data_ptr_ub[i];
      break;
    default:
      break;
    }
    glyphtouse = (Field)DXGetEnumeratedMember((Group)glyphs, 
					      dataindex, NULL);
    if (!glyphtouse) {
      DXSetError(ERROR_DATA_INVALID,
		 "index %d not represented by a glyph in the glyph group", 
                 dataindex);
      goto error;
    }
    if (DXEmptyField(glyphtouse))
       continue;

    
    
    glyphpositions = (Array)DXGetComponentValue(glyphtouse, "positions");
    glyphconnections = (Array)DXGetComponentValue(glyphtouse, "connections");
    glyphcolors = (Array)DXGetComponentValue(glyphtouse,"colors");
    glyphnormals = (Array)DXGetComponentValue(glyphtouse, "normals");
    if (glyphnormals) {
      normattr = DXGetString((String)DXGetComponentAttribute(glyphtouse,
							     "normals", 
							     "dep"));
      if ((strcmp(normattr,"positions")&&strcmp(normattr,"connections"))) {
	DXSetError(ERROR_DATA_INVALID,
		   "normals must be dependent either on positions or on connections");
	goto error;
      }
    }
    
    DXGetArrayInfo(glyphpositions, &numglyphpos, NULL, NULL, NULL,NULL); 
    map[i] = numglyphpos;
    DXGetArrayInfo(glyphconnections, &numglyphcon, NULL, NULL, NULL,NULL); 
    
    DXGetArrayInfo(glyphcolors, NULL, &colortype, &colorcategory, 
		   &colorrank,colorshape); 
    colattr = DXGetString((String)DXGetComponentAttribute(glyphtouse,
							  "colors", 
							  "dep"));
    if ((strcmp(colattr,"positions")&&strcmp(colattr,"connections"))) {
      DXSetError(ERROR_DATA_INVALID,
		 "colors must be dependent either on positions or on connections");
      goto error;
    }
    if (colortype != TYPE_FLOAT) {
      DXSetError(ERROR_DATA_INVALID,"glyph colors must be type float");
      goto error;
    }
    if (colorcategory != CATEGORY_REAL) {
      DXSetError(ERROR_DATA_INVALID,"glyph colors must be category real");
      goto error;
    }
    if (colorrank != 1) {
      DXSetError(ERROR_DATA_INVALID,"glyph colors must be rank 1");
      goto error;
    }
    if (colorshape[0] != 3) {
      DXSetError(ERROR_DATA_INVALID,"glyph colors must be shape 3");
      goto error;
    }
    
    /* XXX Compact colors */
    glyphcolor_ptr = (RGBColor *)DXGetArrayData(glyphcolors);
    
    glyphcon_ptr = (int *)DXGetArrayData(glyphconnections);
    
    if (glyphnormals)
      glyphnorm_ptr = (Point *)DXGetArrayData(glyphnormals);
    
    if (posshape[0] == 1) {
      dp = DXGetArrayEntry(handle, i, scratch);
      pos3D = DXPt(*dp, 0.0, 0.0);
    }
    else if (posshape[0] == 2) {
      dp2 = DXGetArrayEntry(handle, i, scratch);
      pos3D = DXPt(dp2->x, dp2->y, 0.0);
    }
    else if (posshape[0] == 3) {
      dp3 = DXGetArrayEntry(handle, i, scratch);
      pos3D = DXPt(dp3->x, dp3->y, dp3->z);
    }

    /* XXX if glyph positions are regular??? */
    glyphpos_ptr = (float *)DXGetArrayData(glyphpositions);
    if (!glyphpos_ptr) 
      goto error;
    for (j=0; j<numglyphpos; j++) {
      if (glyphshape == 1) {
	glyphpos = DXPt(glyphpos_ptr[j], 0.0, 0.0);
      }
      else if (glyphshape == 2) {
	glyphpos = DXPt(glyphpos_ptr[2*j], glyphpos_ptr[2*j+1], 0.0);
      }
      else if (glyphshape == 3) {
	glyphpos = DXPt(glyphpos_ptr[3*j], 
			glyphpos_ptr[3*j+1], 
			glyphpos_ptr[3*j+2]);
      }
      /* first scale the glyph if necessary */
      if (scale != 1.0)
	glyphpos = DXApply(glyphpos, DXScale(scale, scale, scale));       
      
      /* Now translate by the position value */ 
      glyphpos = DXApply(glyphpos, DXTranslate(pos3D));
      
      /* Now add it to the positions array */ 
      if (!DXAddArrayData(newpositions, ipos++, 1, &glyphpos))
	goto error;
      
      
      if (!strcmp(colattr, "positions")) {
	color = glyphcolor_ptr[j];
	if (!DXAddArrayData(newcolors, iclr++, 1, &color))
	  goto error;
      }
      
      if (hasnormals) {
	if (!strcmp(normattr, "positions")) {
	  normal = glyphnorm_ptr[j];
	  if (!DXAddArrayData(newnormals, inml++, 1, &normal))
	    goto error;
	}
      }
    }
    for (j=0; j<numglyphcon; j++) {
      /* Now do the connections */

      if (!strcmp(glyphelementtype,"triangles")) {
        oldtri = DXTri(glyphcon_ptr[3*j], glyphcon_ptr[3*j+1], 
   		       glyphcon_ptr[3*j+2]);
        newtri = DXTri(oldtri.p+numprev, oldtri.q+numprev, oldtri.r+numprev);
        if (!DXAddArrayData(newconnections, icon++, 1, &newtri))
  	  goto error;
      }
      else {
        oldln = DXLn(glyphcon_ptr[2*j], glyphcon_ptr[2*j+1]);
        newln = DXLn(oldln.p+numprev, oldln.q+numprev);
        if (!DXAddArrayData(newconnections, icon++, 1, &newln))
  	  goto error;
      }
      if (!strcmp(colattr, "connections")) {
        color = glyphcolor_ptr[j];
        if (!DXAddArrayData(newcolors, iclr++, 1, &color))
	  goto error;
      }
      if (hasnormals) {
	if (!strcmp(normattr, "connections")) {
	  normal = glyphnorm_ptr[j];
	  if (!DXAddArrayData(newnormals, inml++, 1, &normal))
	    goto error;
	}
      }
    }
    numprev += numglyphpos;
    }
  }






  /* now put the new arrays in the field */
  if (!DXSetComponentValue(field, "positions", (Object)newpositions))
    goto error;
  if (!DXGetArrayInfo(newpositions,&totalpoints,NULL,NULL,NULL,NULL))
    goto error;
  newpositions = NULL;


  if (!DXSetComponentValue(field, "connections", (Object)newconnections))
    goto error;
  newconnections = NULL;
  if (!strcmp(glyphelementtype,"triangles")) {
    if (!DXSetComponentAttribute(field,"connections", "element type", 
  			         (Object)DXNewString("triangles")))
      goto error;
  }
  else {
    if (!DXSetComponentAttribute(field,"connections", "element type", 
  			         (Object)DXNewString("lines")))
      goto error;
  }



  if (!DXSetComponentValue(field, "colors", (Object)newcolors))
    goto error;
  newcolors = NULL;
  if (!strcmp(colattr,"positions")) {
    if (!DXSetComponentAttribute(field,"colors", "dep", 
				 (Object)DXNewString("positions")))
      goto error;
  }
  else {
    if (!DXSetComponentAttribute(field,"colors", "dep", 
				 (Object)DXNewString("connections")))
      goto error;
  }

  
  if (hasnormals) {
    if (!DXSetComponentValue(field, "normals", (Object)newnormals))
      goto error;
    newnormals = NULL;
    if (!strcmp(normattr,"positions")) {
      if (!DXSetComponentAttribute(field,"normals", "dep", 
                                   (Object)DXNewString("positions")))
	goto error;
    }
    else {
      if (!DXSetComponentAttribute(field,"normals", "dep", 
                                   (Object)DXNewString("connections")))
	goto error;
    }
  }


  /* before expanding components, delete the invalid positions or connections 
   */
  if (DXGetComponentValue((Field)field,"invalid positions"))
     DXRemove((Object)field,"invalid positions"); 
  if (DXGetComponentValue((Field)field,"invalid connections"))
     DXRemove((Object)field,"invalid connections"); 

  if (!ExpandComponents((Field)field, dataattr, totalpoints, numdata, map, "colors"))
    goto error;

  if (!DXChangedComponentValues(field,"positions"))
    goto error;
  if (!DXEndField(field))
    goto error;

 
  DXFreeInvalidComponentHandle(invalidhandle); 
  DXFreeArrayHandle(handle);
  DXFree((Pointer)scratch);
  if (posted)
    DXDelete((Object)positions);
  return OK;
  
 error:
  DXFreeInvalidComponentHandle(invalidhandle); 
  DXFreeArrayHandle(handle);
  DXFree((Pointer)scratch);
  if (posted)
    DXDelete((Object)positions);
  DXDelete((Object)newpositions);
  DXDelete((Object)newconnections);
  DXDelete((Object)newcolors);
  DXDelete((Object)newnormals);
  return ERROR;
}


static Error CheckGlyphGroup(Object glyphs, int *outshape, int *hasnormals,
                             char *glyphelementtype) 
{
  int nummembers, first=1, i;
  Array positions, connections, colors, normals;
  Object child;
  char *attr;
  Type type;
  Category category;
  int rank, shape[8];
  
  if (DXGetObjectClass(glyphs) != CLASS_GROUP) {
    DXSetError(ERROR_DATA_INVALID,"glyph group not class group");
    return ERROR;
  }
 
  DXGetMemberCount((Group)glyphs, &nummembers);
  
  for (i=0; i<nummembers; i++) {
    child = DXGetEnumeratedMember((Group)glyphs, i, NULL);
    if (DXGetObjectClass(child) != CLASS_FIELD) {
      DXSetError(ERROR_DATA_INVALID,
		 "children of glyph group must be fields");
      return ERROR;
    }
    if (!DXEmptyField((Field)child)) {
      /* now check that what's in each field is ok */
      positions = (Array)DXGetComponentValue((Field)child,"positions");
      connections = (Array)DXGetComponentValue((Field)child,"connections");
      colors = (Array)DXGetComponentValue((Field)child,"colors");
      if ((!positions)||(!connections)||(!colors)) {
	DXSetError(ERROR_DATA_INVALID,
		   "glyph group member is missing positions, data, or colors");
	return ERROR;
      }
      attr = DXGetString((String)DXGetComponentAttribute((Field)child,
						         "connections", 
                                                         "element type"));
      if (!attr) {
	DXSetError(ERROR_DATA_INVALID,
		   "glyph group member is missing connections element type attribute");
	return ERROR;
      }
      if ((strcmp(attr,"triangles"))&&(strcmp(attr,"lines"))) {
	DXSetError(ERROR_DATA_INVALID,
		   "glyph group member connections must be triangles or lines");
	return ERROR;
      }
      if (!DXGetArrayInfo(positions,NULL, &type, &category, &rank, shape))
	return ERROR;
      
      if (type != TYPE_FLOAT) {
	DXSetError(ERROR_DATA_INVALID, 
		   "glyph group member must have floating point positions");
	return ERROR;
      }
      if (category != CATEGORY_REAL) {
	DXSetError(ERROR_DATA_INVALID,
		   "glyph group member must have category real positions");
	return ERROR;
      }
      if (rank != 1) {
	DXSetError(ERROR_DATA_INVALID,
		   "glyph group member must have rank 1 positions");
	return ERROR;
      }
      if ((shape[0]<0 || shape[0]>3)) {
	DXSetError(ERROR_DATA_INVALID,
		   "glyph group member must have 1D, 2D, or 3D positions");
	return ERROR;
      }
      
      normals = (Array)DXGetComponentValue((Field)child, "normals");
      
      if (first) {
	first = 0;
	*outshape = shape[0];
	if (normals)
	  *hasnormals = 1;
	else
	  *hasnormals = 0;
        strcpy(glyphelementtype, attr);
      }
      else {
	/* check that dimensionality matches others in the group */
	if (shape[0] != *outshape) {
	  DXSetError(ERROR_DATA_INVALID,
		     "dimensionality of all glyphs in group must be the same");
	  return ERROR;
	}
        /* check that whether or not it has normals matches others in the
         * group */
	if ((normals && (*hasnormals==0))||((!normals)&&(*hasnormals==1))) {
	  DXSetError(ERROR_DATA_INVALID,
		     "if one glyph in group has normals, all must");
	  return ERROR;
	}
        /* check that the element type matches others in the group */
        if (strcmp(attr, glyphelementtype)) {
	  DXSetError(ERROR_DATA_INVALID,
		     "element type of all glyphs in group must be the same");
	  return ERROR;
        }
      } 
    }
  } 
  return OK;
}



Error _dxfGetFontName(char *type_string, char *font)
{
  char *start, *oldstart;
  
  
  /* if it's "colored text" or "text" there may be font information 
   * attached */
  /* Look for "text" */
  start = strstr(type_string, "text");
  if (start != NULL) {
    /* found "text" */
    start+=4;
    if (*start != '\0') {
      /* now look for "font=" */
      oldstart=start;
      DXSetError(ERROR_DATA_INVALID,"unrecognized type string");
      start = strstr(start,"font=");
      if (start != NULL) {
	/* found "font=" */
	start+=5;
	if (*start != '\0') {
	  strcpy(font,start);
	  *oldstart='\0';
	  DXResetError();
	}
      }
    }
  }
  if (DXGetError()!=ERROR_NONE)  
    return ERROR;
  return OK;
}

Error _dxfGetTensorStats(Object object, float *min, float *max)
{
  Object subo;
  int i;
  
  switch (DXGetObjectClass(object)) {
  case (CLASS_FIELD):
    if (!_dxfGetFieldTensorStats((Field)object, min, max))
      goto error;
    break;
  case (CLASS_GROUP):
    for (i=0; (subo=DXGetEnumeratedMember((Group)object,i,NULL)); i++) {
      if (!_dxfGetTensorStats(subo, min, max))
	goto error;
    }
    break;
  case (CLASS_XFORM):
    if (!(DXGetXformInfo((Xform)object, &subo, NULL)))
      return ERROR;
    if (!_dxfGetTensorStats(subo, min, max))
      goto error;
    break;
  default:
    break;
  }
  return OK;
 error:
  return ERROR;
}


Error _dxfGetFieldTensorStats(Field field, float *min, float *max)
{
  Array data = NULL, invalid_pos=NULL;
  char *attr;
  Type type;
  InvalidComponentHandle invalidhandle=NULL;
  Category category;
  int i, j, numitems, rank, shape[10], invalid=0, matrixsize;
  float length;
  Stress_Tensor st;
  Stress_Tensor2 st2;
  short *data_ptr_s=NULL;
  int *data_ptr_i=NULL;
  float *data_ptr_f=NULL;
  double *data_ptr_d=NULL;
  ubyte *data_ptr_ub=NULL;
  byte *data_ptr_b=NULL;
  ushort *data_ptr_us=NULL;
  uint *data_ptr_ui=NULL;
  Vector eigen_vector[3];

  data = (Array)DXGetComponentValue(field,"data");
  if (!data) return OK;
  
  attr = DXGetString((String)DXGetComponentAttribute((Field)field,
						     "data", "dep"));
  if (!attr) {
    DXSetError(ERROR_DATA_INVALID,"#10255", "data","dep");
    goto error;
  }
  
  if (!DXGetArrayInfo(data,&numitems, &type, &category,
		      &rank, shape))
    goto error;
  
  if ((category != CATEGORY_REAL)||
      (rank != 2)||
      (!(((shape[0] == 3)&&(shape[1]==3))||((shape[0]==2)&&(shape[1]==2))))){
    DXSetError(ERROR_DATA_INVALID,"tensors must be 2x2 or 3x3 symmetric");
    goto error;
  }
  if (shape[0]==2)
     matrixsize = 2;
  else 
     matrixsize = 3;
  
  switch (type) {
  case (TYPE_SHORT):
    data_ptr_s = (short *)DXGetArrayData(data);
  case (TYPE_INT):
    data_ptr_i = (int *)DXGetArrayData(data);
    break;
  case (TYPE_FLOAT):
    data_ptr_f = (float *)DXGetArrayData(data);
    break;
  case (TYPE_DOUBLE):
    data_ptr_d = (double *)DXGetArrayData(data);
    break;
  case (TYPE_UBYTE):
    data_ptr_ub = (ubyte *)DXGetArrayData(data);
    break;
  case (TYPE_BYTE):
    data_ptr_b = (byte *)DXGetArrayData(data);
    break;
  case (TYPE_USHORT):
    data_ptr_us = (ushort *)DXGetArrayData(data);
    break;
  case (TYPE_UINT):
    data_ptr_ui = (uint *)DXGetArrayData(data);
    break;
  default:
    DXSetError(ERROR_DATA_INVALID,"unrecognized data type");
    goto error;
  }


  if (!strcmp(attr,"connections")) {
    invalid_pos = (Array)DXGetComponentValue((Field)field,
					     "invalid connections");
    if (invalid_pos) {
      invalid = 1;
      invalidhandle = DXCreateInvalidComponentHandle((Object)field,
						     NULL, "connections");
      if (!invalidhandle)
	goto error;
    }
  }
  else {
    invalid_pos = (Array)DXGetComponentValue((Field)field,
					     "invalid positions");
    if (invalid_pos) {
      invalid = 1;
      invalidhandle = DXCreateInvalidComponentHandle((Object)field,
						     NULL, "positions");
      if (!invalidhandle)
	goto error;
    }
  }

  if (matrixsize == 3) {
  for (i=0; i<numitems; i++) {
    /* tensor size formulas */
    if (!((invalid) && (DXIsElementInvalidSequential(invalidhandle, i)))) {
      switch (type) {
      case (TYPE_SHORT):
        st.tau[0][0] = (float)data_ptr_s[0];
        st.tau[0][1] = (float)data_ptr_s[1];
        st.tau[0][2] = (float)data_ptr_s[2];
        st.tau[1][0] = (float)data_ptr_s[3];
        st.tau[1][1] = (float)data_ptr_s[4];
        st.tau[1][2] = (float)data_ptr_s[5];
        st.tau[2][0] = (float)data_ptr_s[6];
        st.tau[2][1] = (float)data_ptr_s[7];
        st.tau[2][2] = (float)data_ptr_s[8];
        break;
      case (TYPE_INT):
        st.tau[0][0] = (float)data_ptr_i[0];
        st.tau[0][1] = (float)data_ptr_i[1];
        st.tau[0][2] = (float)data_ptr_i[2];
        st.tau[1][0] = (float)data_ptr_i[3];
        st.tau[1][1] = (float)data_ptr_i[4];
        st.tau[1][2] = (float)data_ptr_i[5];
        st.tau[2][0] = (float)data_ptr_i[6];
        st.tau[2][1] = (float)data_ptr_i[7];
        st.tau[2][2] = (float)data_ptr_i[8];
        break;
      case (TYPE_FLOAT):
        st.tau[0][0] = (float)data_ptr_f[0];
        st.tau[0][1] = (float)data_ptr_f[1];
        st.tau[0][2] = (float)data_ptr_f[2];
        st.tau[1][0] = (float)data_ptr_f[3];
        st.tau[1][1] = (float)data_ptr_f[4];
        st.tau[1][2] = (float)data_ptr_f[5];
        st.tau[2][0] = (float)data_ptr_f[6];
        st.tau[2][1] = (float)data_ptr_f[7];
        st.tau[2][2] = (float)data_ptr_f[8];
        break;
      case (TYPE_DOUBLE):
        st.tau[0][0] = (float)data_ptr_d[0];
        st.tau[0][1] = (float)data_ptr_d[1];
        st.tau[0][2] = (float)data_ptr_d[2];
        st.tau[1][0] = (float)data_ptr_d[3];
        st.tau[1][1] = (float)data_ptr_d[4];
        st.tau[1][2] = (float)data_ptr_d[5];
        st.tau[2][0] = (float)data_ptr_d[6];
        st.tau[2][1] = (float)data_ptr_d[7];
        st.tau[2][2] = (float)data_ptr_d[8];
        break;
      case (TYPE_BYTE):
        st.tau[0][0] = (float)data_ptr_b[0];
        st.tau[0][1] = (float)data_ptr_b[1];
        st.tau[0][2] = (float)data_ptr_b[2];
        st.tau[1][0] = (float)data_ptr_b[3];
        st.tau[1][1] = (float)data_ptr_b[4];
        st.tau[1][2] = (float)data_ptr_b[5];
        st.tau[2][0] = (float)data_ptr_b[6];
        st.tau[2][1] = (float)data_ptr_b[7];
        st.tau[2][2] = (float)data_ptr_b[8];
        break;
      case (TYPE_UBYTE):
        st.tau[0][0] = (float)data_ptr_ub[0];
        st.tau[0][1] = (float)data_ptr_ub[1];
        st.tau[0][2] = (float)data_ptr_ub[2];
        st.tau[1][0] = (float)data_ptr_ub[3];
        st.tau[1][1] = (float)data_ptr_ub[4];
        st.tau[1][2] = (float)data_ptr_ub[5];
        st.tau[2][0] = (float)data_ptr_ub[6];
        st.tau[2][1] = (float)data_ptr_ub[7];
        st.tau[2][2] = (float)data_ptr_ub[8];
        break;
      case (TYPE_USHORT):
        st.tau[0][0] = (float)data_ptr_us[0];
        st.tau[0][1] = (float)data_ptr_us[1];
        st.tau[0][2] = (float)data_ptr_us[2];
        st.tau[1][0] = (float)data_ptr_us[3];
        st.tau[1][1] = (float)data_ptr_us[4];
        st.tau[1][2] = (float)data_ptr_us[5];
        st.tau[2][0] = (float)data_ptr_us[6];
        st.tau[2][1] = (float)data_ptr_us[7];
        st.tau[2][2] = (float)data_ptr_us[8];
        break;
      case (TYPE_UINT):
        st.tau[0][0] = (float)data_ptr_ui[0];
        st.tau[0][1] = (float)data_ptr_ui[1];
        st.tau[0][2] = (float)data_ptr_ui[2];
        st.tau[1][0] = (float)data_ptr_ui[3];
        st.tau[1][1] = (float)data_ptr_ui[4];
        st.tau[1][2] = (float)data_ptr_ui[5];
        st.tau[2][0] = (float)data_ptr_ui[6];
        st.tau[2][1] = (float)data_ptr_ui[7];
        st.tau[2][2] = (float)data_ptr_ui[8];
        break;
      default:
        break;
      }
      if (!GetEigenVectors(st, eigen_vector)) {
	goto error;
      }
    }
    for (j=1; j<3; j++) {
      length = DXLength(eigen_vector[j]);
      if (length < *min) *min=length;
      if (length > *max) *max=length;
    }
  }
  }
  else {
  /* 2x2 matix */
  for (i=0; i<numitems; i++) {
    /* tensor size formulas */
    if (!((invalid) && (DXIsElementInvalidSequential(invalidhandle, i)))) {
      switch (type) {
      case (TYPE_SHORT):
        st2.tau[0][0] = (float)data_ptr_s[0];
        st2.tau[0][1] = (float)data_ptr_s[1];
        st2.tau[1][0] = (float)data_ptr_s[2];
        st2.tau[1][1] = (float)data_ptr_s[3];
        break;
      case (TYPE_INT):
        st2.tau[0][0] = (float)data_ptr_i[0];
        st2.tau[0][1] = (float)data_ptr_i[1];
        st2.tau[1][0] = (float)data_ptr_i[2];
        st2.tau[1][1] = (float)data_ptr_i[3];
        break;
      case (TYPE_FLOAT):
        st2.tau[0][0] = (float)data_ptr_f[0];
        st2.tau[0][1] = (float)data_ptr_f[1];
        st2.tau[1][0] = (float)data_ptr_f[2];
        st2.tau[1][1] = (float)data_ptr_f[3];
        break;
      case (TYPE_DOUBLE):
        st2.tau[0][0] = (float)data_ptr_d[0];
        st2.tau[0][1] = (float)data_ptr_d[1];
        st2.tau[1][0] = (float)data_ptr_d[2];
        st2.tau[1][1] = (float)data_ptr_d[3];
        break;
      case (TYPE_BYTE):
        st2.tau[0][0] = (float)data_ptr_b[0];
        st2.tau[0][1] = (float)data_ptr_b[1];
        st2.tau[1][0] = (float)data_ptr_b[2];
        st2.tau[1][1] = (float)data_ptr_b[3];
        break;
      case (TYPE_UBYTE):
        st2.tau[0][0] = (float)data_ptr_ub[0];
        st2.tau[0][1] = (float)data_ptr_ub[1];
        st2.tau[1][0] = (float)data_ptr_ub[2];
        st2.tau[1][1] = (float)data_ptr_ub[3];
        break;
      case (TYPE_USHORT):
        st2.tau[0][0] = (float)data_ptr_us[0];
        st2.tau[0][1] = (float)data_ptr_us[1];
        st2.tau[1][0] = (float)data_ptr_us[2];
        st2.tau[1][1] = (float)data_ptr_us[3];
        break;
      case (TYPE_UINT):
        st2.tau[0][0] = (float)data_ptr_ui[0];
        st2.tau[0][1] = (float)data_ptr_ui[1];
        st2.tau[1][0] = (float)data_ptr_ui[2];
        st2.tau[1][1] = (float)data_ptr_ui[3];
        break;
      default:
        break;
      }
      if (!GetEigenVectors2(st2, eigen_vector)) {
	goto error;
      }
    }
    for (j=1; j<3; j++) {
      length = DXLength(eigen_vector[j]);
      if (length < *min) *min=length;
      if (length > *max) *max=length;
    }
  }
  }
  return OK;
 error:
  return ERROR;
}


int _dxfIsTensor(Object o)
{
  int p;
  Object child;
  Array data;
  int rank, shape[10];

  /* do until a non-empty field is found */
  p = 0;
  child = (Object)DXGetPart((Object)o, p);
  while (child != NULL && DXEmptyField((Field)child)) {
      child = (Object)DXGetPart((Object)o, p++);
  }
  /* get the data type */
  data = (Array)DXGetComponentValue((Field)child,"data");
  if (!data) return 0;
  if (!DXGetArrayInfo(data,NULL,NULL,NULL,&rank,shape))
     return ERROR;
  if ((rank==2)&&(shape[0]==3)&&(shape[1]==3))
     return 1;
  else if ((rank==2)&&(shape[0]==2)&&(shape[1]==2))
     return 1;
  else
     return 0;
}


static int _dxfHasDataComponent(Object o)
{
if (Exists2(o, "positions", "data"))
   return 1;
else
   return 0;
}

