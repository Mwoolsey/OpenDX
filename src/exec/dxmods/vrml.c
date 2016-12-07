/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#include "dx/dx.h"
#include "vrml.h"
#include <stdio.h>

#if defined(HAVE_STRING_H)
#include <string.h>
#endif

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#include <math.h>
#define MAX_PARAMS 21
#define MAX_RANK 16
#define NO 0
#define YES 1

static FILE *fp=NULL;
static int text_as_string = NO;
static char *tab[]  = {"","  ","    ","      ","        ","          ","          ", "              ","                ","                  "}; 
static char *fontstyle=NULL;

Error parse_dx(Object dxobject,int *index,Matrix *mat,char *name,Point *box);
Error write_solid(Object dxobject,int triangle);
static Error write_positions(Object dxobject,int itab,int pos_dim);
static Error write_connections(Object dxobject,int triangle,int itab);
static Error write_colors(Object dxobject,int itab,RGBColor **rgb,float *op);
static Error write_normals(Object dxobject, int itab);
static Error write_elevation(Object dxobject);
static void write_appearence(RGBColor *rgb, float op,int lines, int itab);
static int write_transform(Matrix mat, int *trans_two);
Error SetViewPoint(Object dxobject);
static Error write_caption(Object o,Point *box);
static Error write_text(Object dxobject);
static void DGetRotationOfMatrix(Matrix mat, Vector *rotvec, float *sinhrot);

Error _dxfExportVRML(Object dxobject, char *filename)
{
   char wrlname[256];
   int index = 0;
   char *cp;
   Point box[8];
   char *string;

   /* open the vrml file */
   if (((cp=strrchr(filename, '.'))!=NULL) &&
		!strcmp(cp,".wrl") )
     strcpy(wrlname,filename);
   else{
     strcpy(wrlname,filename);
     strcat(wrlname,".wrl");
   }
   fp = fopen(wrlname,"w");

   if (!fp){
      DXSetError(ERROR_INTERNAL, "unable to open output file base on %s",
                                              filename);
          goto error;
   }

   /* Write out the parameters */
   fprintf(fp,"%s\n","#VRML V2.0 utf8");
   /* set Navigation default */
   fprintf(fp,"%s\n","NavigationInfo {");
   if (DXGetStringAttribute((Object)dxobject,"VRML Set NavigationInfo type",&string))
     fprintf(fp,"  type [%s]\n",string);
   else
     fprintf(fp,"%s\n","  type [\"EXAMINE\",\"WALK\",\"FLY\"] ");
   fprintf(fp,"%s\n","}");
   fprintf(fp,"%s\n","Group {");
   fprintf(fp,"%s\n"," children [");

   /* add first child node ?  */
   if (DXGetStringAttribute((Object)dxobject,"VRML Insert First Child Node",&string))
     fprintf(fp,"  %s\n",string);

   /* Calculate the Bounding Box of whole object so can position */
   /* Caption is exists */
   DXBoundingBox(dxobject,box);
   
   /* do we export strings are geometry? */
   if (!DXGetIntegerAttribute((Object)dxobject,"VRML text as string",
			&text_as_string)) 
     text_as_string = NO;

   /* if there a fontstyle */
   if (!DXGetStringAttribute((Object)dxobject,"VRML FontStyle",&fontstyle))
     fontstyle=NULL;

   /* now export object */
   if (!parse_dx(dxobject,&index,NULL,NULL,box))
      goto error;

   /* find the initial viewpoint based on bounding box of object */
   /* and a 45 degree field of view */
   SetViewPoint(dxobject);
     
   fprintf(fp,"%s\n","]");
   fprintf(fp,"%s\n","}");
   fclose(fp);

   return OK;

error:
   if (fp) fclose(fp);
   return ERROR;
}


/* if group recurse through the objects in a group */
Error parse_dx(Object dxobject, int *index,Matrix *mat,char *name,Point *box)
{
  Object o,o1;
  int i;
  Array a;
  Matrix tempmat;
  char *string;
  int simple_transform,screen_pos,trans_two=0;
  ModuleInput modin[2];
  ModuleOutput modout[1];

  switch(DXGetObjectClass((Object)dxobject)){
    case CLASS_GROUP:
    case CLASS_MULTIGRID:
      for (i=0; (o = DXGetEnumeratedMember((Group)dxobject,i,&name)); i++){
	    if (!parse_dx(o,index,mat,name,box))
	       goto error;
	 }
	 break;
    case CLASS_XFORM:
	 /* use Apply transform for any transformed object 
	    this can be changed to use write_transform if I could
	    figure out how to break the matrix into individual translations,
	    rotations, and scales correctly
	  */
	 DXGetXformInfo((Xform)dxobject,&o,&tempmat);
	 simple_transform = write_transform(tempmat,&trans_two);
	 if (simple_transform == 1) {
 	   if (!parse_dx(o,index,&tempmat,name,box))
	     goto error;
	   if (trans_two == 0) {
	     fprintf(fp,"%s\n"," ]");
	     fprintf(fp,"%s\n","}");
	   }
	   if (trans_two == 1) {
	     fprintf(fp,"%s\n"," ]");
	     fprintf(fp,"%s\n","}");
	     fprintf(fp,"%s\n"," ]");
	     fprintf(fp,"%s\n","}");
	   }
	 }
	 else {
	   o = DXApplyTransform((Object)dxobject,NULL);
	   if (!o)
	     goto error;
	   if (!parse_dx(o,index,&tempmat,name,box))
	     goto error;
	 }
	 break;
    case CLASS_SCREEN:
	 if (DXGetAttribute((Object)dxobject,"Caption string")) {
	   if (!write_caption(dxobject,box))
	     goto error;
	   break;
	 }
	 DXGetScreenInfo((Screen)dxobject,&o,&screen_pos,NULL);
	 /* Autoglyph text */
	 if (screen_pos == SCREEN_WORLD) {
	   if (DXGetXformInfo((Xform)o,&o1,&tempmat)
	           && DXGetAttribute((Object)o1,"Text string")) {
	     /* make these Billboard objects */
	     fprintf(fp,"%s\n","Billboard {");
	     fprintf(fp,"%s\n"," axisOfRotation 0.0 0.0 0.0");
	     fprintf(fp,"%s\n"," children [");
	     if (text_as_string == YES) {
	       if (!write_text(o1))
	         goto error;
	     }
	     else {
	       if (!parse_dx(o1,index,mat,name,box))
	         goto error;
	     }
	     fprintf(fp,"%s\n"," ]");
	     fprintf(fp,"%s\n","}");
	   }
	   else if (DXGetAttribute((Object)o,"Text string")) {
	     if (!write_text(o))
	       goto error;
	   }
	 }
	 else
    	   DXWarning("This screen objects are ignored");
	 break;
    case CLASS_FIELD:
	 if (!DXGetComponentValue((Field)dxobject,"positions")) {
	    DXWarning("Field does not contain positions, skipping");
	    break;
	 }

	 /* is this text ? */
	 if (text_as_string == YES) {
	   if (DXGetStringAttribute((Object)dxobject,"Text string",&string)){
	     if (!write_text(dxobject))
	       goto error;
	     break;
	   }
	 }

	 /* check connections component to get type of output 	*/
	 /* surface, lines, or points ?				*/
         if (!(a = (Array)DXGetComponentValue((Field)dxobject,"connections"))){
	    if (!write_solid(dxobject,0))
	       goto error;
	    break;
	 }
         else {

	 /* check element type and call solid with correct index */
	 if (!DXGetStringAttribute((Object)a,"element type",&string))
	    goto error;
	 if (!strcmp("lines",string)){
	    if (!write_solid(dxobject,1))
	       goto error;
	 }
	 else if (!strcmp("quads",string)){	
	   if (!write_solid(dxobject,3))
	      goto error;
	 }
	 else if (!strcmp("triangles",string)){  
	   if (!write_solid(dxobject,2))
	     goto error;
	 }
	 else {			/* not quad,triangle,lines, must be volume */
				/* call showboundary first 		   */
	   DXWarning("volume data is converted to boundary");
	   DXModSetObjectInput(&modin[0],"input",dxobject);
	   DXModSetIntegerInput(&modin[1],"validity",1);
	   DXModSetObjectOutput(&modout[0],NULL,&o);
	   if (!DXCallModule("ShowBoundary",2,modin,1,modout))
	     goto error;
	   if (!parse_dx(o,index,mat,name,box))
	     goto error;
	 }

	 break;
    }
    default:
	DXSetError(ERROR_DATA_INVALID,"not recognized type class"); 
	goto error;
  }

  return OK;
error:
  return ERROR;
}

/* write out each solid object */
Error write_solid(Object dxobject,int triangle)
{
  Type type;
  int rank,shape[MAX_RANK];
  Array a;
  int np;
  int itab=1;
  float op;	/* constant color and opacity */
  RGBColor *rgb;
  int pos_dim=3;
  float deltas[4];

  rgb =NULL; op = -1;
  /* get positions array and check dimensionality */
  a = (Array)DXGetComponentValue((Field)dxobject,"positions");
  if (!DXGetArrayInfo(a,&np,&type,NULL,&rank,shape))
    goto error;
  if (rank != 1 || shape[0] < 2){
    DXSetError(ERROR_DATA_INVALID,"positions must be 3-d or 2-d\n");
    goto error;
  }
  if (shape[0]==2){
    pos_dim = 2;
    /* regular orthogonal 2-d grid use ElevationGrid node */
    if (DXQueryGridPositions(a,NULL,NULL,NULL,deltas)){
      if ((deltas[0]==0 || deltas[1]==0) || (deltas[2]==0 || deltas[3]==0)) 
        return (write_elevation(dxobject));
    }
  }
   
  /* what type of shape is it ?		*/
  /* PointSet - no connections		*/
  /* IndexedFaceSet - connections of quads or triangles	*/
  /* IndexedLineSet - connections of lines		*/
  fprintf(fp,"%s%s\n",tab[itab++],"Shape {");
  if (triangle == 0) 
    fprintf(fp,"%s%s \n",tab[itab++],"geometry PointSet {");
  else if (triangle == 1)
    fprintf(fp,"%s%s \n",tab[itab++],"geometry IndexedLineSet {");
  else{
    fprintf(fp,"%s%s \n",tab[itab++],"geometry IndexedFaceSet {");
    fprintf(fp,"%s%s \n",tab[itab],"solid	FALSE");
    if (triangle == 3) 		/* quads don't have counterclockwise connect */
      fprintf(fp,"%s%s \n",tab[itab],"ccw	FALSE");
  }

  /* Write out the positions */
  if (!write_positions(dxobject,itab,pos_dim))
    goto error;

  /* Write out the connections */
  if (triangle > 0 ){ 
    if (!write_connections(dxobject,triangle,itab))
      goto error;
  }
  
  /* Get colors and opacity info */
  if (!write_colors(dxobject,itab,&rgb,&op))
    goto error;

  /* Write out normals  (normals should be unit length) */
  if (triangle > 1 && !write_normals(dxobject,itab))
    goto error;

  fprintf(fp,"%s}\n",tab[--itab]);	/* geometry */

  /* always write appearance node to get default lighting */
  write_appearence(rgb,op,triangle,itab);

  fprintf(fp,"%s}\n",tab[--itab]);	/* Shape */

  return OK;

error: return ERROR;
}

/* Write out the positions */
static Error write_positions(Object dxobject,int itab,int pos_dim)
{
  int i;
  Array a;
  int np,rank,shape[MAX_RANK];
  Type type;
  float *f;

  a = (Array)DXGetComponentValue((Field)dxobject,"positions");
  if (!DXGetArrayInfo(a,&np,&type,NULL,&rank,shape))
    goto error;
  fprintf(fp,"%s%s\n",tab[itab++],"coord Coordinate{ ");
  fprintf(fp,"%s%s\n",tab[itab++],"point [ ");
  f = (float *)DXGetArrayData(a);
  if (pos_dim == 2){
    for (i=0; i<np; i++)
      fprintf(fp,"%s%f %f 0.0\n", tab[itab],f[i*2], f[i*2 +1] ); 
  }
  else{
    for (i=0; i<np; i++)
      fprintf(fp,"%s%f %f %f\n", tab[itab],f[i*3], f[i*3 +1], f[i*3 +2]); 
  }
  fprintf(fp,"%s%s\n",tab[--itab],"]");
  fprintf(fp,"%s%s\n",tab[--itab],"}");
 
  return OK;
error:
  return ERROR;
}

/* Write out the connections */
static Error write_connections(Object dxobject,int triangle,int itab)
{
  int i,end=-1;
  Array a;
  int np,rank,shape[MAX_RANK];
  Type type;
  int *d;

      fprintf(fp,"%s%s\n",tab[itab++],"coordIndex [");
      a = (Array)DXGetComponentValue((Field)dxobject,"connections");
      if (!DXGetArrayInfo(a,&np,&type,NULL,&rank,shape))
        goto error;
      d = (int *)DXGetArrayData(a);

      if (triangle == 1) {
        for (i=0; i<np; i++)
          fprintf(fp,"%s%d %d %d\n", tab[itab],d[i*2],d[i*2 +1],end);
      }
      else if (triangle == 2) {
        for (i=0; i<np; i++)
          fprintf(fp,"%s%d %d %d %d\n", tab[itab],d[i*3],d[i*3 +1],d[i*3 +2],end);
      }
      else {
        for (i=0; i<np; i++) 	/* quads don't have counterclockwise connect */
        fprintf(fp,"%s%d %d %d %d %d\n",tab[itab],d[i*4],d[i*4 +1],d[i*4 +3],d[i*4 +2],end);
      }

      fprintf(fp,"%s%s\n",tab[--itab],"]");
  
  return OK;
error:
  return ERROR;
}

/* write colors using Color node if colors array exists		*/
/* set flag COLORS_PER_VERTEX :TRUE if dep positions 		*/
/*				 FALSE if dep connections	*/
/* if colors are constant return rgb set			*/
/* if opacities exist set op					*/
static 
Error write_colors(Object dxobject,int itab,RGBColor **rgb,float *op_avg)
{
  int i;
  Array a;
  int np,rank,shape[MAX_RANK];
  Type type;
  float *f,*op;
  char *string;

  if ((a = (Array)DXGetComponentValue((Field)dxobject,"colors"))){
    if (DXGetArrayClass(a) != CLASS_CONSTANTARRAY) {
      fprintf(fp,"%s%s\n",tab[itab++],"color Color {");
      fprintf(fp,"%s%s\n",tab[itab++],"color [");
      if (!DXGetArrayInfo(a,&np,&type,NULL,&rank,shape))
	goto error;
      f = (float *)DXGetArrayData(a);
      for (i=0; i<np; i++)
        fprintf(fp,"%s%f %f %f,\n",tab[itab],f[i*3], f[i*3 +1], f[i*3 +2]);
      fprintf(fp,"%s%s\n",tab[--itab],"]");
      fprintf(fp,"%s%s\n",tab[--itab],"}");
      if (!DXGetStringAttribute((Object)a,"dep",&string))
        goto error;
      if (!strcmp("connections",string)) 
        fprintf(fp,"%s%s\n",tab[itab],"colorPerVertex FALSE");
      else
        fprintf(fp,"%s%s\n",tab[itab],"colorPerVertex TRUE");
    }
    else 
      *rgb = (RGBColor *)DXGetConstantArrayData(a);
  }

  /* check for opacities */
  /* if constant array, use that value. if array, use average? 	*/
  /* in vrml 0.0-> opaque, 1.0-> transparent			*/
  if ((a = (Array)DXGetComponentValue((Field)dxobject,"opacities"))){
    if (DXGetArrayClass(a) != CLASS_CONSTANTARRAY){ 
      if (!DXStatistics(dxobject,"opacities",NULL,NULL,op_avg,NULL))
        goto error;
      *op_avg = 1.0 - *op_avg;
    }
    else{
      op = (float *)DXGetConstantArrayData(a);
      *op_avg = 1.0 - op[0];
    }
  }

  return OK;
error:
  return ERROR;
}

/* Write out normals  (normals should be unit length) */
static 
Error write_normals(Object dxobject, int itab)
{
  Array a;
  int i;
  int np,rank,shape[MAX_RANK];
  Type type;
  float *f;
  char *string;

  if ((a = (Array)DXGetComponentValue((Field)dxobject,"normals")) != NULL) {
    if (!DXGetArrayInfo(a,&np,&type,NULL,&rank,shape))
      goto error;
    f = (float *)DXGetArrayData(a);
    fprintf(fp,"%s%s\n",tab[itab++],"normal Normal {");
    fprintf(fp,"%s%s\n",tab[itab++],"vector [");
    for (i=0; i<np; i++)
      fprintf(fp,"%s%f %f %f\n", tab[itab],f[i*3], f[i*3 +1], f[i*3 +2]);
    fprintf(fp,"%s%s\n",tab[--itab],"]");
    fprintf(fp,"%s%s\n",tab[--itab],"}");
    if (!DXGetStringAttribute((Object)a,"dep",&string))
	    goto error;
    if (!strcmp("connections",string)) 
      fprintf(fp,"%s%s\n",tab[itab],"normalPerVertex	FALSE");
  }  
  else
    DXWarning("this surface has no normals");
  
  return OK;
error:
  return ERROR;
}
 
/* write out an Elevation Grid node of 2-d regular surfaces (flat) 	*/
/* height is constant zero 						*/
/* VRML positions this at [0,0] in x,z plane, so must be transformed	*/
static Error write_elevation(Object dxobject)
{
  int i=0;
  Array a,new_a;
  int counts[3];
  float origin[3],delta[4];
  int itab=1,np,data_np;
  float op,*f;
  RGBColor *rgb;
  ModuleInput modin[1];
  ModuleOutput modout[1];
  Object o;
  float scale=0,min;

  rgb=NULL; op=-1.0;
  /* call Reorient to make sure grid is x varies fastest	*/
  /* with positive deltas					*/
  DXReference(dxobject);
  DXModSetObjectInput(&modin[0],"image",dxobject);
  DXModSetObjectOutput(&modout[0],NULL,&o);
  if (!DXCallModule("Reorient",1,modin,1,modout))
    goto error;
  
  a = (Array)DXGetComponentValue((Field)o,"positions");
  if (!DXGetArrayInfo(a,&np,NULL,NULL,NULL,NULL))
      goto error;
  if (!DXQueryGridPositions(a,NULL,counts,origin,delta))
    goto error;

  /* what is the transformation need to place this surface */
  fprintf(fp,"%s%s\n",tab[itab++],"Transform {");
  fprintf(fp,"%s%s %g %g %g\n",tab[itab],"translation",origin[0],origin[1],0.0);
  fprintf(fp,"%s%s %g %g %g %g\n",tab[itab],"rotation",1.0,0.0,0.0,-1.57);
  fprintf(fp,"%s%s\n",tab[itab],"children [");
  fprintf(fp,"%s%s\n",tab[itab++],"Shape {");
  fprintf(fp,"%s%s\n",tab[itab++],"geometry ElevationGrid {");
  fprintf(fp,"%s%s %d\n",tab[itab],"xDimension",counts[1]);
  fprintf(fp,"%s%s %d\n",tab[itab],"zDimension",counts[0]);
  fprintf(fp,"%s%s %g\n",tab[itab],"xSpacing",delta[2]);
  fprintf(fp,"%s%s %g\n",tab[itab],"zSpacing",delta[1]);
  fprintf(fp,"%s%s\n",tab[itab],"solid FALSE");

  /* Look for special VRML attributes to place data into height field */
  fprintf(fp,"%s%s ",tab[itab],"height [");
  if (DXGetFloatAttribute((Object)o,"VRML use data as height",&scale) ||
      DXGetIntegerAttribute((Object)o,"VRML use data as height",&i)){ 
    if (i!=0) scale = (float)i;
    a = (Array)DXGetComponentValue((Field)o,"data");
    if (!DXGetArrayInfo(a,&data_np,NULL,NULL,NULL,NULL))
      goto error;
    if (data_np!=np){	
      DXSetError(ERROR_DATA_INVALID,
	"data not dep positions, can't use as height field");
      goto error;
    }
    new_a = (Array)DXArrayConvert(a,TYPE_FLOAT,CATEGORY_REAL,0);
    if (!new_a)
      goto error;
    if (!DXStatistics((Object)new_a,"data",&min,NULL,NULL,NULL))
      goto error;
    f = (float *)DXGetArrayData(new_a);
    fprintf(fp,"%g",f[0]*scale);
    for (i=1; i<np; i++){
      fprintf(fp,",%g",f[i]+(f[i]-min)*scale);
      if (i % 5 == 0) fprintf(fp,"%s%s","\n",tab[itab]);
    }
    DXDelete((Object)new_a);
  }
  else{
    /* create height data containing all zeros for flat surface */
    fprintf(fp,"%g",0.0);
    for (i=1; i<np; i++){
      fprintf(fp,",%g",0.0);
      if (i % 5 == 0) fprintf(fp,"%s%s","\n",tab[itab]);
    }
  }
  fprintf(fp," %s\n","]");
  
  /* Get colors and opacity info */
  if (!write_colors(o,itab,&rgb,&op))
    goto error;

  /* Write out normals  (normals should be unit length) */
  if (!write_normals(o,itab))
    goto error;
  
  fprintf(fp,"%s%s\n",tab[--itab],"}");	/* ElevationGrid */
  
  /* always write appearance node to get default lighting */
  write_appearence(rgb,op,2,itab);
  
  fprintf(fp,"%s%s\n",tab[--itab],"}"); 	/* Shape */
  fprintf(fp,"%s%s\n",tab[itab],"]");	/* Children */
  fprintf(fp,"%s%s\n",tab[--itab],"}");	/* Transform */

  return OK;
error:
  return ERROR;
}

static void write_appearence(RGBColor *rgb, float op, int lines, int itab)
{
  fprintf(fp,"%s%s\n",tab[itab++],"appearance Appearance {");
  fprintf(fp,"%s%s\n",tab[itab++],"material Material {");
  if (rgb) 
    fprintf(fp,"%s%s %f %f %f\n",tab[itab],"diffuseColor",rgb->r,rgb->g,rgb->b);
  if (lines == 1) {	/* lines connections */
    if (rgb) fprintf(fp,"%s%s %f %f %f\n",tab[itab],"emissiveColor",rgb->r,rgb->g,rgb->b);
    else fprintf(fp,"%s%s ",tab[itab],"emissiveColor 1 1 1");
  }
  if (op >=0.0) fprintf(fp,"%s%s %f\n",tab[itab],"transparency",op);
  fprintf(fp,"%s}\n",tab[--itab]);	/* Material */
  fprintf(fp,"%s}\n",tab[--itab]);	/* Appearance */
}

/* find the initial viewpoint based on bounding box of object */
/* and a 45 degree field of view */
Error SetViewPoint(Object dxobject)
{
  Point center;
  Point box[8];
  double d,distance;

  if (DXBoundingBox(dxobject,box)) {
    d = DXLength(DXSub(box[3],box[4]));
    distance = 1.12 * .5*d/tan(.5*.785398);
    center.x = (box[4].x +box[3].x)/2;
    center.y = (box[4].y +box[3].y)/2;
    center.z = (box[4].z +box[3].z)/2;
    fprintf(fp,"%s\n","Viewpoint {");
    fprintf(fp,"%s %g %g %g\n"," position",center.x,center.y,center.z+distance);
    fprintf(fp,"%s\n","}");
     
  }
    
  return OK;
}

/* write out transformation matrix */
static int write_transform(Matrix mat,int *trans_two)
{
  float angle;
  float sinhrot;
  Vector rotvec;

  /* what kind of transformation is it */
  if (mat.A[0][1]==0 && mat.A[0][2]==0 && mat.A[1][0]==0 && mat.A[1][2]==0 &&
	mat.A[2][0]==0 && mat.A[2][1]==0){ 
    if (mat.b[0]==0 && mat.b[1]==0 && mat.b[2]==0){ 
      if (mat.A[0][0]!=1 || mat.A[1][1]!=1 || mat.A[2][2]!=1){	/* scale */
        fprintf(fp,"%s\n","Transform {");
        fprintf(fp,"%s %f %f %f\n","scale",mat.A[0][0],mat.A[1][1],mat.A[2][2]);
      }
      else {	/* identity */
	*trans_two = -1;
	return 1;
      }
    }
    else {	/* translation */
      fprintf(fp,"%s\n","Transform {");
      fprintf(fp,"%s %f %f %f\n","translation",mat.b[0],mat.b[1],mat.b[2]);
    }
  }
  else{ 		/* rotation */
    DGetRotationOfMatrix(mat, &rotvec, &sinhrot);
    fprintf(fp,"%s %g %g %g %g\n","# rot angle ",rotvec.x,rotvec.y,rotvec.z,    	2.*asin((double)sinhrot));
    if (mat.A[0][0]==1)	{	/* about the x-axis */
      /* angle = acos(mat.A[1][1]); */
      angle = asin(mat.A[1][2]);
      if (mat.A[1][1] >= 0)
	angle = angle;
      else {
	angle = 3.14159 - angle;
        fprintf(fp,"%s %g\n","# cos sin ",angle);
      }
      fprintf(fp,"%s\n","Transform {");
      fprintf(fp,"%s %f\n","rotation 1 0 0",angle);
    }
    else if (mat.A[1][1]==1) {	/* about the y-axis */
      /* angle = acos(mat.A[0][0]); */
      angle = -asin(mat.A[0][2]);
      if (mat.A[0][0] >= 0)
	angle = angle;
      else {
	angle = 3.14159 - angle;
        fprintf(fp,"%s %g\n","# cos sin ",angle);
      }
      fprintf(fp,"%s\n","Transform {");
      fprintf(fp,"%s %f\n","rotation 0 1 0",angle);
    }
    else if (mat.A[2][2]==1) {	/* about the z-axis */
      /* angle = acos(mat.A[0][0]); */
      angle = asin(mat.A[0][1]);
      if (mat.A[0][0] >= 0)
	angle = angle;
      else {
	angle = 3.14159 - angle;
        fprintf(fp,"%s %g\n","# cos sin ",angle);
      }
      fprintf(fp,"%s\n","Transform {");
      fprintf(fp,"%s %f\n","rotation 0 0 1",angle);
    }
    else {
      /* special case check for permutations (AutoAxes uses these) */
      if(mat.A[0][1] == 1){
	if (mat.A[1][2]==1 && mat.A[2][0]==1 && mat.A[0][0]==0 && 
	    mat.A[0][2]==0 && mat.A[1][0]==0 && mat.A[1][1]==0 && 
	    mat.A[2][1]==0 && mat.A[2][2]==0){
          fprintf(fp,"%s\n","Transform {");
          fprintf(fp,"%s\n","rotation 0 1 0 1.57");
	  fprintf(fp,"%s\n"," children[");
	  fprintf(fp,"%s\n","Transform {");
          fprintf(fp,"%s\n","rotation 0 0 1 1.57");
	  *trans_two=1;
	}
	else return 0;
      }
      else if (mat.A[0][2]==1){
	if (mat.A[1][0]==1 && mat.A[2][1]==1 && mat.A[0][0]==0 && 
	    mat.A[0][1]==0 && mat.A[1][1]==0 && mat.A[1][2]==0 && 
	    mat.A[2][0]==0 && mat.A[2][2]==0){
          fprintf(fp,"%s\n","Transform {");
          fprintf(fp,"%s\n","rotation 1 0 0 -1.57");
	  fprintf(fp,"%s\n"," children[");
	  fprintf(fp,"%s\n","Transform {");
          fprintf(fp,"%s\n","rotation 0 0 1 -1.57");
	  *trans_two=1;
	}
	else return 0;
      }
      else
        return 0;		/* not a simple translation,rotation,scale */
    }
  }
  fprintf(fp,"%s\n"," children [");
  
  /* check for translation */
  /*
  if (mat.b[0]==0 && mat.b[1]==0 && mat.b[2]==0){ 
      fprintf(fp,"%s\n","Transform {");
      fprintf(fp,"%s %f %f %f\n","translation",mat.b[0],mat.b[1],mat.b[2]);
      fprintf(fp,"%s\n"," children [");
      *trans_two = 1;
  }
  */
  return 1;
}

/* Use the Text Node and ProximitySensor to create a caption that */
/* is fixed in the screen */
static Error write_caption(Object dxobject,Point *box)
{
  int i=0,itab=1;
  Point center;
  float *pos=NULL;
  char *string;
  Object o,o1;
  float *rgb;	/* constant color */
  Array a;

  rgb = NULL;
  /* is there a colors component (if so is always a constant array) */
  if (!DXGetScreenInfo((Screen)dxobject,&o,NULL,NULL))
    goto error;
  if (!DXGetXformInfo((Xform)o,&o1,NULL))
    goto error;
  if ((o=DXGetEnumeratedMember((Group)o1,0,NULL))==NULL)
    goto error;
  if (!DXGetScreenInfo((Screen)o,&o1,NULL,NULL))
    goto error;
  if (!DXGetXformInfo((Xform)o1,&o,NULL))
    goto error;
  if ((a = (Array)DXGetComponentValue((Field)o,"colors")))
    rgb = (float *)DXGetConstantArrayData(a);

  if (DXGetIntegerAttribute((Object)dxobject,"Caption flag",&i) && i != 0){
    DXWarning("VRML export can only have captions that are viewport relative");
    pos[0] = 0.5;
    pos[1] = 0.05;
  }
  o = DXGetAttribute((Object)dxobject,"Caption position");
  pos = (float *)DXGetArrayData((Array)o);
  o = DXGetAttribute((Object)dxobject,"Caption string");

  /* Calculate the Transformation needed to place the caption at position */
  /* I have fixed the translation in z so size of caption is done by  */
  /* font size parameter		*/
  center.x = pos[0] * 0.2 -0.1;
  center.y = pos[1] * 0.12 -0.06;
  center.z = -0.15;
  fprintf(fp,"%s%s\n",tab[itab++],"DEF HudGroup Collision {");
  fprintf(fp,"%s%s\n",tab[itab],"collide FALSE");
  fprintf(fp,"%s%s\n",tab[itab++],"children [");
  fprintf(fp,"%s%s\n",tab[itab++],"DEF HudProx ProximitySensor {");
  fprintf(fp,"%s%s\n",tab[itab],"center 0 20 0");
  fprintf(fp,"%s%s\n",tab[itab],"size 500 500 500");
  fprintf(fp,"%s%s\n",tab[--itab],"}");
  fprintf(fp,"%s%s\n",tab[itab++],"DEF HudXform Transform {");
  fprintf(fp,"%s%s\n",tab[itab++],"children [");
  fprintf(fp,"%s%s\n",tab[itab++],"Transform {");
  fprintf(fp,"%s%s %g %g %g\n",tab[itab],"translation",center.x,center.y,center.z);
  fprintf(fp,"%s%s\n",tab[itab++],"children [");

  fprintf(fp,"%s%s\n",tab[itab++],"Shape {");
  fprintf(fp,"%s%s\n",tab[itab++],"geometry Text {");
  fprintf(fp,"%s%s", tab[itab],"string [");
  while (DXExtractNthString(o,i++,&string))
    fprintf(fp,"\"%s\"",string);
  fprintf(fp,"%s\n","]");
  if (DXGetStringAttribute((Object)dxobject,"VRML FontStyle",&string))
    fprintf(fp,"%s%s%s%s\n",tab[itab],"fontStyle FontStyle {",string," }");
  else {
    fprintf(fp,"%s%s\n",tab[itab],"fontStyle FontStyle {size .01");
    fprintf(fp,"%s%s\n",tab[itab], "justify [\"MIDDLE\" \"MIDDLE\"] }");
  }
  fprintf(fp,"%s%s\n",tab[--itab],"}");		/* Text */
  fprintf(fp,"%s%s\n",tab[itab++],"appearance Appearance {");
  fprintf(fp,"%s%s\n",tab[itab],"material Material {");
  if (rgb){
    fprintf(fp,"%s%s %g %g %g\n",tab[itab],"diffuseColor",rgb[0],rgb[1],rgb[2]);
    fprintf(fp,"%s%s %g %g %g\n",tab[itab],"emissiveColor",rgb[0],rgb[1],rgb[2]);
  }
  else
    fprintf(fp,"%s%s\n",tab[itab],"emissiveColor 1 1 1");
  fprintf(fp,"%s%s\n",tab[itab],"}");		/* Material */
  fprintf(fp,"%s%s\n",tab[--itab],"}");		/* Appearance */
  fprintf(fp,"%s%s\n",tab[--itab],"}");		/* Shape */

  fprintf(fp,"%s%s\n",tab[itab],"]");		/* children */
  fprintf(fp,"%s%s\n",tab[--itab],"}");		/* Transform */
  fprintf(fp,"%s%s\n",tab[--itab],"]");		/* children */
  fprintf(fp,"%s%s\n",tab[--itab],"}");		/* Transform */
  fprintf(fp,"%s%s\n",tab[--itab],"]");		/* children */
  fprintf(fp,"%s%s\n",tab[itab],"ROUTE HudProx.orientation_changed TO HudXform.rotation");
  fprintf(fp,"%s%s\n",tab[itab],"ROUTE HudProx.position_changed TO HudXform.translation");
  
  fprintf(fp,"%s%s\n",tab[--itab],"}");		/* collision */
  
  return OK;

error:
  return ERROR;
}
   
static Error
write_text(Object dxobject)
{
  char *string;
  float *rgb;
  Array a;
  int itab = 1;

  /* Use bounding box to get position of Text */
  /* 
  DXBoundingBox(dxobject,box);
  center.x = (box[4].x +box[3].x)/2;
  center.y = (box[4].y +box[3].y)/2;
  center.z = (box[4].z +box[3].z)/2;
  fprintf(fp,"%s%s\n",tab[itab++],"Transform {");
  fprintf(fp,"%s%s %g %g %g\n",tab[itab],"translation",center.x,center.y,center.z);
  fprintf(fp,"%s%s\n",tab[itab++],"children [");
  */
  fprintf(fp,"%s%s\n",tab[itab++],"Shape {");
  fprintf(fp,"%s%s\n",tab[itab++],"geometry Text {");
  fprintf(fp,"%s%s", tab[itab],"string [");
  DXGetStringAttribute((Object)dxobject,"Text string",&string);
  fprintf(fp,"\"%s\"",string);
  fprintf(fp,"%s\n","]");
  if (DXGetStringAttribute((Object)dxobject,"VRML FontStyle",&string))
    fprintf(fp,"%s%s%s%s\n",tab[itab],"fontStyle FontStyle {",string," }");
  else if (&fontstyle != NULL)
    fprintf(fp,"%s%s%s%s\n",tab[itab],"fontStyle FontStyle {",fontstyle," }");
  else
    fprintf(fp,"%s%s\n",tab[itab],"fontStyle FontStyle {size 1.25}");
  fprintf(fp,"%s%s\n",tab[--itab],"}");		/* Text */
  fprintf(fp,"%s%s\n",tab[itab++],"appearance Appearance {");
  fprintf(fp,"%s%s\n",tab[itab],"material Material {");
  if ((a = (Array)DXGetComponentValue((Field)dxobject,"colors"))){
    rgb = (float *)DXGetArrayData(a);
    fprintf(fp,"%s%s %g %g %g\n",tab[itab],"diffuseColor",rgb[0],rgb[1],rgb[2]);
    fprintf(fp,"%s%s %g %g %g\n",tab[itab],"emissiveColor",rgb[0],rgb[1],rgb[2]);
  }
  else
    fprintf(fp,"%s%s\n",tab[itab],"emissiveColor 1 1 1");
  fprintf(fp,"%s%s\n",tab[itab],"}");		/* Material */
  fprintf(fp,"%s%s\n",tab[--itab],"}");		/* Appearance */
  fprintf(fp,"%s%s\n",tab[--itab],"}");		/* Shape */
  /*
  fprintf(fp,"%s%s\n",tab[--itab],"]");
  fprintf(fp,"%s%s\n",tab[--itab],"}");
  */
  return OK;
}

void DGetRotationOfMatrix(Matrix mat, Vector *rotvec, float *sinhrot)
{
    /* Double precision version of GetRotationOfMatrix.			*/
    /* Input:	pointer to mat = rotation/transl (no scaling) matrix	*/
    /* Output:	pointer to rotvec = rotation unit vector		*/
    /* 		pointer to sinhrot = sine of rotation half-angle	*/

    int i, j, imax1=0, imax2=0;
    float rows[3][3],rowmag[3];
    Vector tanvec, cenvec, sensvec, maxaxis;
    float maxmag=0, minmag=0;
    Vector row_max,row_med,mat_max;

    /* Find the row of (mat - identity) whose magnitude is maximum, and	*/
    /* similarly for minimum.  The latter is to get the median row by	*/
    /* the process of elimination.					*/

    for(i = 0; i < 3; i++) {
	float amag = (float)sqrt(mat.A[i][0]*mat.A[i][0] + mat.A[i][1]*mat.A[i][1]
						+ mat.A[i][2]*mat.A[i][2]);
	rowmag[i] = 0.;
	for(j = 0; j < 3; j++) {
	    rows[i][j] = mat.A[i][j]/amag - (float)(i == j);
	    rowmag[i] += rows[i][j]*rows[i][j];
	}
	rowmag[i] = (float)sqrt(rowmag[i]);
	if(!i || rowmag[i] > maxmag) {
	    maxmag = rowmag[i];
	    imax1 = i;
	}
	if(!i || rowmag[i] < minmag) {
	    minmag = rowmag[i];
	    imax2 = i;
	}
    }
    /* If all changes are the same, complement one of the indices.	*/
    if(imax1 == imax2) imax2 = 2 - imax1;
    if(maxmag < 1.e-14) {
	/* There is no rotation.  Return a vector along z with zero	*/
	/* rotation (sine = 0).						*/
	rotvec->x = rotvec->y = *sinhrot = 0.;
	rotvec->z = 1.;
	return;
    }
    /* With min and max rows in hand, median row is only one left.	*/
    /* Find it from the fact that the sum of indices = 3.		*/
    imax2 = 3 - imax1 - imax2;
    row_max.x = rows[imax1][0];
    row_max.y = rows[imax1][1];
    row_max.z = rows[imax1][2];
    row_med.x = rows[imax2][0];
    row_med.y = rows[imax2][1];
    row_med.z = rows[imax2][2];
    mat_max.x = mat.A[imax1][0];
    mat_max.y = mat.A[imax1][1];
    mat_max.z = mat.A[imax1][2];
    /* Cross the max row with the second max row and unitize to get the	*/
    /* rotation unit vector.						*/
    *rotvec = DXNormalize(DXCross(row_max,row_med));
    /* Determine which direction it should point (backward or not)	*/
    /* by forcing it to lie in the same hemisphere as the cross product	*/
    /* of the axis vector corresponding to max row and its change.	*/
    /* That product establishes the right-handedness of the rotation.	*/
    sensvec = DXCross(mat_max, row_max);
    if(DXDot(*rotvec, sensvec) < 0.)
      *rotvec = DXNeg(*rotvec);
    /* Cross rotation vector into axis vector corresponding to max	*/
    /* row, and unitize to get direction of axis tip motion at 		*/
    /* beginning of rotation (i. e. initial tangent).			*/
    maxaxis.x=maxaxis.y=maxaxis.z=0.;
    if (imax1 == 0) maxaxis.x=1.;
    else if (imax1 == 1) maxaxis.y=1.;
    else maxaxis.z=1.;
    tanvec = DXNormalize(DXCross(*rotvec, maxaxis));
    /* Cross rotation unit vector into that initial tangent vector	*/
    /* to get corresponding centripetal vector.				*/
    cenvec = DXCross(*rotvec, tanvec);
    /* The centripetal component (dot product with centripetal unit	*/
    /* of unitized max row is sine of half-rotation angle.		*/
    row_max = DXNormalize(row_max);
    *sinhrot = DXDot(row_max, cenvec);
}
