/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>

#include <string.h>
#include "interact.h"
#include "_autocolor.h"
#include "separate.h"
#include "histogram.h"
#include "_colormap.h"
#include "color.h"

#define NUMBINS 20

extern Array DXScalarConvert(Array ); /* from libdx/stats.h */

/* defined in this file */
Error _dxfcontrol_to_o(Object in0,Field *op);
Error _dxfcontrol_to_rgb(Object in0,Object in1,Object in2, Field *rgb);

static Object gethistogram(Object ,int ,int ,float *,float *);
static Error vect_scalar(Object );
static Error makescalar(Object);
static Error opacity_to_o(Object opacity,Field *out);
static Error print_map(Field f,char *component,char *name, char *id);
static Error scale_output(float *min,float *max,Field *rgb,Field *op,int ip[]);
static Error hsvo_to_rgb_and_op(Object in0,Object in1,Object in2,Object in3, 
		   Field *rgb,Field *opacity);
static Error colormap_to_rgb(Object inmap,Field *outmap);
static Error hsvo_to_cp_component(Field *f,float *pts,float *data,int num,
                                  int isvec);
static Error getui_minmax(Object o,float *min, float *max);
static int match_cpoints(Field inmap,Object defmap,int which);
static Error hsv_to_rgbfield(Object inmap,Field *rgb);

typedef struct map {
   float level;
   float value;
} Map;
typedef struct hsvo {
   Map *map;
   char *name;
   int count;
   int minmax;
} HSVO;
static Error array_to_float(Array a,float *data,Type type,int num);
static Error addarray(Field *f,Map *map,int items,char *component);

int
m_Colormap(Object *in,Field *out)
{
   struct einfo ei;
   int change1=0,change4,change5,change=0,changecmap=0,changeomap=0;
   float min,max,minmax[2];
   float defmin,defmax;		/*default min and max */
   Object idobj=NULL,histogram,color=NULL,opacity=NULL;
   char *id, *label;
   int *histo_pts=NULL,nhist;
   int i,iprint[MAXPRINT],msglen=0,shape[1];
   int no_input=0,outlier=1,old_version=0;
   int min_input=0,max_input=0,longmessage=0;
   char begin[] = "begin", end[] = "end";
   Field rgb_output,op_output;
   Array histo_array;

   if (!in[5] && !in[6] && !in[7]){
      no_input=1;
   }
   out[0] = NULL;
   out[1] = NULL;
   ei.msgbuf = NULL;
   histogram = NULL;
   rgb_output = NULL;
   op_output = NULL;
   histo_array = NULL;
 
   /* get id */
   if (!DXExtractString(in[4],&id)){
      DXSetError(ERROR_BAD_PARAMETER,"#10000","id");
      goto error;
   }
   idobj=in[4];

   /* initialize iprint */
   for (i=0; i<MAXPRINT; i++)
      iprint[i]=0;
   
   /* check for change in object which supply the min and max 
    * this needs to be done even when no input for case 
    * connect-unconnect-connect again */
   change1 = _dxfcheck_obj_cache(in[5],id,0,idobj);
   change4 = _dxfcheck_obj_cache(in[6],id,4,idobj);
   change5 = _dxfcheck_obj_cache(in[7],id,5,idobj);

   switch(no_input){
   case (0):

   /* read in object if changed get statistics then check for user input
    * if no change in min and max change=0, get min and max from previous
    */
 
   if ((change1 > 0 || change4 > 0 || change5 > 0)){
      change=1;
      if (in[5]){
         if (!DXStatistics(in[5], "data", &min, &max, NULL, NULL))
            return ERROR;
	 min_input=1; max_input=1;
	 iprint[0]=1; iprint[1]=1;
      }
      if (in[6]){
         if (!DXExtractFloat(in[6], &min)){
            DXSetError(ERROR_BAD_PARAMETER,"#10080","min");
            goto error;
         }
	 min_input=1;
	 iprint[0]=1;
      }
      if (in[7]){
         if (!DXExtractFloat(in[7], &max)){
            DXSetError(ERROR_BAD_PARAMETER,"#10080","max");
            goto error;
         }
	 max_input=1;
	 iprint[1]=1;
      }
      /* Print warning if max<min */
      if (max_input && min_input && max < min){
         DXSetError(ERROR_BAD_PARAMETER,"#10150","minimum",max);
         goto error;
      }
      if (in[5] && (in[6] || in[7]))
         DXWarning("the min and/or max values have been specified; these override the min/max of the data");
   }
   
   /* get number of bins in histogram */
   if (in[8]){
      if(!DXExtractInteger(in[8],&nhist)){
         DXSetError(ERROR_BAD_PARAMETER,"#10020","nbins");
         goto error;
      }
      iprint[6]=1;
   }
   else
      nhist=NUMBINS;
   
   case (1):
   /* check for colormap input
    * if exists and is group:colormap and opacity together 
    * verify that it is a colormap */
   changecmap = _dxfcheck_obj_cache(in[9],id,9,idobj);
   if (in[9] && changecmap > 0){
      if (DXGetObjectClass(in[9])==CLASS_GROUP){
	if (!(color = DXGetMember((Group)in[9],"colormap")) ||
            !DXGetMember((Group)in[9],"opacitymap")){
	   DXSetError(ERROR_BAD_PARAMETER,"#10371","color","colormap", "or imported cm file");
	   goto error;
	}
      }
      else
	color = in[9];
      if (DXGetObjectClass(color)==CLASS_GROUP){
	 DXSetError(ERROR_BAD_PARAMETER,"colormap can't be group");
	 goto error1;
      }
      if (_dxfIsColorMap(color)){
	 if (!colormap_to_rgb(color,&rgb_output))
	   goto error1;
      }
      /* if no rgb colorfield but just hue, sat, val positions */
      /* treat like control points were input */
      else if (DXGetComponentValue((Field)color,"hue positions") ||
               DXGetComponentValue((Field)color,"sat positions") ||
	       DXGetComponentValue((Field)color,"val positions") ){
	 DXResetError();
	 if (!hsv_to_rgbfield(color,&rgb_output))
	    goto error1;
      }
      else
	goto error;
      
      /* if this is first time and default matches input use first 4 inputs */
      if (changecmap == 2) {
         if (match_cpoints(rgb_output,in[12],1) && 
             match_cpoints(rgb_output,in[13],2) &&
	     match_cpoints(rgb_output,in[14],3)){
            DXDelete((Object)rgb_output);
	    rgb_output=NULL;
	    changecmap=0;
	    if (!_dxfcontrol_to_rgb(in[0],in[1],in[2],&rgb_output))
	       goto error1;
	 }
      }
   }
   else{
      /* read data from the UI Colormap editor 
       * control points (4 2-vector floats) DXVersion 2.1 and on
       * hsv triples for backward compatibility
       */
      if (DXQueryParameter(in[0],TYPE_FLOAT,0,&i)){
         if(!hsvo_to_rgb_and_op(in[0],in[1],in[2],in[3],&rgb_output,&op_output))
	    goto error1;
	 old_version=1;
      }
      else{
	 if (!_dxfcontrol_to_rgb(in[0],in[1],in[2],&rgb_output))
	    goto error1;
      }
   }
 
   /* check for opacity map input */
   changeomap = _dxfcheck_obj_cache(in[10],id,10,idobj);
   if (in[10] && changeomap > 0) {
      if (DXGetObjectClass(in[10])==CLASS_GROUP){
	if (!DXGetMember((Group)in[10],"colormap") ||
            !(opacity = DXGetMember((Group)in[10],"opacitymap"))){
	   DXSetError(ERROR_BAD_PARAMETER,"#10371","opacity","opacitymap", "or imported cm file");
	   goto error;
	}
      }
      else
         opacity = in[10];
   }
   if (opacity){
      if (DXGetObjectClass(opacity)==CLASS_GROUP){
         DXSetError(ERROR_BAD_PARAMETER,"opacity map must be field");
         goto error;
      }
      if (_dxfIsOpacityMap(opacity)){
	 if (!opacity_to_o(opacity,&op_output))
	    goto error1;
      }
      else
	 goto error;
      
      /* if this is first time and default matches input use first 4 inputs */
      if (changeomap == 2) {
         if (match_cpoints(op_output,in[15],4)){ 
            DXDelete((Object)op_output);
	    op_output=NULL;
	    changeomap = 0;
	    if (!_dxfcontrol_to_o(in[3],&op_output))
	       goto error1;
	 }
      }
   }
   else{
      if (!old_version){
         if (!_dxfcontrol_to_o(in[3],&op_output))
	    goto error1;
      }
   }

   /* if min and max were not input then use UI min and max */
   /* or if it is the first time executing this net */
   if (!max_input || !min_input || change1 == 2){ 
      if (!old_version){
         if (!DXExtractParameter(in[11],TYPE_FLOAT,0,2,minmax)){
	    DXSetError(ERROR_INTERNAL,"minmax from UI wrong");
	    goto error;
         }
      }
      else{
	 if (!getui_minmax(in[0],&minmax[0],&minmax[1]))
	    goto error;
      }
      if (change1 == 2){	/* if first time */
         if (in[16]){
	    if (!DXExtractFloat(in[16], &defmin)){
               DXSetError(ERROR_INTERNAL,"#10080","defmin");
               goto error;
            }
	    if (min == defmin){ 	  /* defaults match use uimin*/
	       min = minmax[0];
               iprint[0]=0;
	    }
	 }
	 if (in[17]){
            if (!DXExtractFloat(in[17], &defmax)){
               DXSetError(ERROR_INTERNAL,"#10080","defmax");
               goto error;
            }
            if (max == defmax){		   /* defaults match use uimax*/
	       max = minmax[1];
	       iprint[1]=0;
	    }
	 }
      }
      else{
         if (!min_input) min = minmax[0];
         if (!max_input) max = minmax[1];
      }
   }

   /* get label */
   change5 = _dxfcheck_obj_cache(in[18],id,8,idobj);
   if (change5 > 0) iprint[5]=1;
   if (in[18]){
      if (!DXExtractString(in[18], &label)){
        DXSetError(ERROR_BAD_PARAMETER,"#10200","label");
        goto error;
      }
   }
   else
   label="";

   /* cache inputs (min,max,label) and return old min and max */
   /* if (!_dxfinteract_float(id,idobj,&min,&max,
                           NULL,NULL,label,0,0,&nhist,iprint))
      return ERROR;

    */

   /* scale the output according to the new min and max */ 
   if (!scale_output(&min,&max,&rgb_output,&op_output,iprint))
      goto error1;

   /* calculate histogram */
   if (in[5] && nhist>1 && (change || iprint[6])){
      if (in[6] || in[7]) outlier = 0;
      if (!(histogram = gethistogram(in[5],nhist,outlier,&min,&max)))
         goto error1;
      histo_array = (Array)DXGetComponentValue((Field)histogram,"data");
      histo_pts = (int *)DXGetArrayData(histo_array);
      iprint[4]=1;
   }

   /* calculate the size of the first UI message */
   if (iprint[4] == 1) msglen += (nhist*NUMBER_CHARS);
   for (i=0; i<MAXPRINT; i++){
      if (iprint[i]==1)
	 msglen += (NUMBER_CHARS + NAME_CHARS);
   }
   msglen += (NUMBER_CHARS + NAME_CHARS); /* for the histogram=NULL statement*/

   /* print message */
   ei.maxlen = msglen;
   ei.atend = 0;
   ei.msgbuf = (char *)DXAllocateZero(ei.maxlen+SLOP);
   if (!ei.msgbuf)
      return ERROR;
   ei.mp = ei.msgbuf;

   shape[0]=1;
   strcpy(ei.mp,"");
   if (iprint[0]==1){
     sprintf(ei.mp, "min="); while(*ei.mp) ei.mp++;
     if (!_dxfprint_message(&min, &ei,TYPE_FLOAT,0,shape,1))
       goto error1;
   }
   else if ((change1 > 0 || change4 > 0) && !in[6] && !in[5]){
     sprintf(ei.mp, "min=NULL"); while(*ei.mp) ei.mp++;
   }
   if (iprint[1]==1){
     sprintf(ei.mp, "max="); while(*ei.mp) ei.mp++;
     if (!_dxfprint_message(&max, &ei,TYPE_FLOAT,0,shape,1))
        goto error1;
   }
   else if ((change1 > 0 || change5 > 0) && !in[7] && !in[5]){
     sprintf(ei.mp, " max=NULL"); while(*ei.mp) ei.mp++;
   }
   if (iprint[4]==1){
      sprintf(ei.mp, "histogram="); while(*ei.mp) ei.mp++;
      if (!_dxfprint_message(histo_pts,&ei,TYPE_INT,0,shape,nhist))
        goto error1;
   }
   if (in[5] && nhist<1){
      sprintf(ei.mp, "histogram=NULL"); while(*ei.mp) ei.mp++;
   }
   }

   /* in DXMessage there is static buffer of MAX_MSGLEN so check length */
   if (strlen(ei.msgbuf) > MAX_MSGLEN){
      DXSetError(ERROR_DATA_INVALID,"#10920");
      goto error1;
   }

   /* check if more the one line message */
   if (changecmap > 0 || changeomap > 0 ||
       (iprint[5]==1 && strlen(label)>0) || strcmp(ei.msgbuf,""))
       longmessage=1;

   /* if colormap exists and has changed send control points and 
    * data points in long message */
   if (longmessage)   
      DXUIMessage(id,begin);

   if (strcmp(ei.msgbuf,""))
      DXUIMessage(id,ei.msgbuf);

   if (iprint[5]==1 && strlen(label)>0){
      if (ei.msgbuf) DXFree((Pointer)ei.msgbuf);
      ei.msgbuf=NULL;
      ei.maxlen =(int)strlen(label);
      ei.atend = 0;
      ei.msgbuf = (char *)DXAllocateZero(ei.maxlen+SLOP);
      if (!ei.msgbuf)
         return ERROR;
      ei.mp = ei.msgbuf;
      sprintf(ei.mp, "title=%s",label);
      DXUIMessage(id,ei.msgbuf);
   }

   if (changecmap > 0 || changeomap > 0){
      if (in[9] && changecmap > 0){
         if (!print_map(rgb_output,"hue positions","huemap=",id))
	    goto error1;
         if (!print_map(rgb_output,"sat positions","satmap=",id))
	    goto error1;
         if (!print_map(rgb_output,"val positions","valmap=",id))
	    goto error1;
      }
      else if (!in[9] && changecmap > 0){
         if (!print_map(NULL,"hue positions","huemap=",id))
	    goto error1;
         if (!print_map(NULL,"sat positions","satmap=",id))
	    goto error1;
         if (!print_map(NULL,"val positions","valmap=",id))
	    goto error1;
      }
      if (in[10] && changeomap > 0){
         if (!print_map(op_output,"opacity positions","opmap=",id))
	    goto error1;
      }
      else if (!in[10] && changeomap > 0){
         if (!print_map(NULL,"opacity positions","opmap=",id))
	    goto error1;
      }
   }

   if (longmessage)
      DXUIMessage(id,end);

   out[0] = (Field)rgb_output;
   out[1] = (Field)op_output;

   if (ei.msgbuf) DXFree((Pointer)ei.msgbuf);
   if (histogram) DXDelete((Object)histogram);
   return OK;

error:
error1:
   if (histogram) DXDelete((Object)histogram);
   if (ei.msgbuf) DXFree((Pointer)ei.msgbuf);
   if (rgb_output) DXDelete((Object)rgb_output);
   if (op_output) DXDelete((Object)op_output);
   change1 = _dxfcheck_obj_cache(in[6],id,0,idobj);
   changeomap = _dxfcheck_obj_cache(in[6],id,10,idobj);
   changecmap = _dxfcheck_obj_cache(in[6],id,9,idobj);
   return ERROR;
}


/* to handle backwards compatibility the UI inputs could be
 * hsv_triples,hsv_pts,op_data,op_pts
 * the output is rgb field with control points component
 * and opacity field with control points component */
static
Error hsvo_to_rgb_and_op(Object in0,Object in1,Object in2,Object in3, 
		   Field *rgb,Field *opacity)
{
   float *hsv_pts,*hsv_data,*op_pts,*op_data;
   int num_hsv,num_op;
   Field hsv;
   Array a,b;

   hsv_pts = NULL; hsv_data = NULL;
   op_pts = NULL; op_data = NULL;
   hsv = NULL;
   /* read in the arrays and create a HSV Field */
   if (!DXQueryParameter(in0,TYPE_FLOAT,0,&num_hsv)){
      DXSetError(ERROR_INTERNAL,"hsv points must be scalar");
      goto error;
   }
   hsv = (Field)_dxfcolorfield(NULL, 0, num_hsv, 1);
   if (!hsv)
      goto error;
   a = (Array)DXGetComponentValue((Field)hsv,"positions");
   b = (Array)DXGetComponentValue((Field)hsv, "data");
   hsv_pts = (float *)DXGetArrayData(a);
   hsv_data = (float *)DXGetArrayData(b);
   if (!DXExtractParameter(in0,TYPE_FLOAT,0,num_hsv,hsv_pts)){
      DXSetError(ERROR_INTERNAL,"hsv pts");
      goto error;
   } 
   if (!DXExtractParameter(in1,TYPE_FLOAT,3,num_hsv,hsv_data)){
      DXSetError(ERROR_INTERNAL,"hsv_data");
      goto error;
   }

   if (!DXQueryParameter(in2,TYPE_FLOAT,0,&num_op)){
      DXSetError(ERROR_INTERNAL,"op points must be scalar");
      goto error;
   }
   *opacity = (Field)_dxfcolorfield(NULL, 0, num_op, 0);
   if (!*opacity)
      goto error;
   a = (Array)DXGetComponentValue((Field)*opacity,"positions");
   b = (Array)DXGetComponentValue((Field)*opacity, "data");
   op_pts = (float *)DXGetArrayData(a);
   op_data = (float *)DXGetArrayData(b);
   if (!DXExtractParameter(in2,TYPE_FLOAT,0,num_op,op_pts)){
      DXSetError(ERROR_INTERNAL,"op pts");
      goto error;
   }
   if (!DXExtractParameter(in3,TYPE_FLOAT,0,num_op,op_data)){
      DXSetError(ERROR_INTERNAL,"op_data");
      goto error;
   }
  
   /* convert to an RGB Field */
   *rgb = _dxfMakeRGBColorMap((Field)hsv);
   if (!*rgb)
      goto error;

   /* convert hsv data to control pt arrays */
   /* add the control point components to the fields */
   if (!hsvo_to_cp_component(rgb,hsv_pts,hsv_data,num_hsv,1))
      goto error;
   if (!hsvo_to_cp_component(opacity,op_pts,op_data,num_op,0))
      goto error;
   DXDelete((Object)hsv);
   
   return OK;

error:
   if (hsv) DXDelete((Object)hsv);
   return ERROR;
}


/* Convert control points to HSV space (editor_to_hsv) 
 * and convert to RGB with control points
 */
Error _dxfcontrol_to_rgb(Object huemap,Object satmap,Object valmap, Field *rgb)
{
   Field hsv;
   int n,items;
   Map *map;
   Array a[3];

   for (n=0; n<3; n++)
      a[n]=NULL;
  
   /* read the control point lists and create arrays */
   if (huemap){
      if (!DXQueryParameter((Object)huemap,TYPE_FLOAT,2,&items)){
         DXSetError(ERROR_INTERNAL,"hue map must be 2-vector floats");
         goto error;
      } 
      a[0] = DXNewArray(TYPE_FLOAT,CATEGORY_REAL,1,2);
      if (!a[0])
         return ERROR;
      if (!DXAddArrayData(a[0],0,items,NULL))
         goto error;
      map = (Map *)DXGetArrayData(a[0]);
      if(!DXExtractParameter((Object)huemap,TYPE_FLOAT,2,items,(Pointer)map)){
         DXSetError(ERROR_INTERNAL,"can't read hue control points");
         goto error;
      }
   }
   if (satmap){
      if (!DXQueryParameter((Object)satmap,TYPE_FLOAT,2,&items)){
         DXSetError(ERROR_INTERNAL,"sat map must be 2-vector floats");
         goto error;
      } 
      a[1] = DXNewArray(TYPE_FLOAT,CATEGORY_REAL,1,2);
      if (!a[1])
         return ERROR;
      if (!DXAddArrayData(a[1],0,items,NULL))
         goto error;
      map = (Map *)DXGetArrayData(a[1]);
      if (!DXExtractParameter((Object)satmap,TYPE_FLOAT,2,items,(Pointer)map)){
         DXSetError(ERROR_INTERNAL,"can't read sat control points");
         goto error;
      }
   }
   if(valmap){
      if (!DXQueryParameter((Object)valmap,TYPE_FLOAT,2,&items)){
         DXSetError(ERROR_INTERNAL,"val map must be 2-vector floats");
         goto error;
      } 
      a[2] = DXNewArray(TYPE_FLOAT,CATEGORY_REAL,1,2);
      if (!a[2])
         return ERROR;
      if (!DXAddArrayData(a[2],0,items,NULL))
         goto error;
      map = (Map *)DXGetArrayData(a[2]);
      if (!DXExtractParameter((Object)valmap,TYPE_FLOAT,2,items,(Pointer)map)){
         DXSetError(ERROR_INTERNAL,"can't read val control points");
         goto error;
      }
   }
   
   hsv = (Field)_dxfeditor_to_hsv(a[0],a[1],a[2]);
   if(!hsv)
      goto error;
    
   *rgb = (Field)_dxfMakeRGBColorMap((Field)hsv);
   DXDelete((Object)hsv);
   if (!*rgb)
      return ERROR;
   /* add the control points */
   if (!DXSetComponentValue(*rgb,"hue positions",(Object)a[0]))
      goto error;
   if (!DXSetComponentValue(*rgb,"sat positions",(Object)a[1]))
      goto error;
   if (!DXSetComponentValue(*rgb,"val positions",(Object)a[2]))
      goto error;
   for (n=0; n<3; n++)
      a[n]=NULL;

   if (!DXEndField(*rgb))
      goto error;

   return OK;

error:
   for(n=0; n<3; n++)
      if (a[n]) DXDelete((Object)a[n]);
   return ERROR;
}

/* convert opacity control points to opacity field */
Error _dxfcontrol_to_o(Object opmap,Field *opacity)
{
   Map *op;
   int num_op;
   Array c=NULL;
   op = NULL;

   if(opmap){
    if (!DXQueryParameter(opmap,TYPE_FLOAT,2,&num_op)){
      DXSetError(ERROR_INTERNAL,"op map must be 2-vector floats");
      goto error;
    } 
    c = DXNewArray(TYPE_FLOAT,CATEGORY_REAL,1,2);
    if (!c)
       return ERROR;
    if (!DXAddArrayData(c,0,num_op,NULL))
       goto error;
    op = (Map *)DXGetArrayData(c);
    if (!DXExtractParameter(opmap,TYPE_FLOAT,2,num_op,(Pointer)op)){
      DXSetError(ERROR_INTERNAL,"can't read op control points");
      goto error;
    }
   }
   /* get the opacity field */
   *opacity = (Field)_dxfeditor_to_opacity((Array)c);
   if (!*opacity)
      goto error;

    /* separate into positions and data and create a field */
    /*
    float *op_data = NULL, *op_pts = NULL;
    *opacity = (Field)_dxfcolorfield(NULL,NULL,num_op,0);
    a = (Array)DXGetComponentValue((Field)*opacity,"positions");
    b = (Array)DXGetComponentValue((Field)*opacity,"data");
    op_pts = (float *)DXGetArrayData(a);
    op_data = (float *)DXGetArrayData(b);
    for (i=0; i<num_op; i++){
      op_pts[i] = op[i].level;
      op_data[i] = op[i].value;
    }
   }
   else{
    num_op = 2;
    *opacity = (Field)_dxfcolorfield(NULL,NULL,num_op,0);
    a = (Array)DXGetComponentValue((Field)*opacity,"positions");
    b = (Array)DXGetComponentValue((Field)*opacity,"data");
    op_pts = (float *)DXGetArrayData(a);
    op_data = (float *)DXGetArrayData(b);
    op_pts[0] = 0.0;
    op_pts[1] = 1.0;
    op_data[0] = op_data[1] = 1.0;
   }
   */

   /* add the control points */
   if (!DXSetComponentValue(*opacity,"opacity positions",(Object)c))
      goto error;
   c=NULL;
   if (!DXEndField(*opacity))
      goto error;

   return OK;
   

error:
   if (c) DXDelete((Object)c);
   return ERROR;
}
  
/* input a colorfield without data and positions, just 
   hue, val, sat positions */
static
Error hsv_to_rgbfield(Object inmap,Field *rgb)
{
   Field hsv;
   Array hue,sat,val;

   /* pull hue, sat, val from the field into arrays */
   hue = (Array)DXGetComponentValue((Field)inmap,"hue positions");
   sat = (Array)DXGetComponentValue((Field)inmap,"sat positions");
   val = (Array)DXGetComponentValue((Field)inmap,"val positions");

   if (!hue || !sat || !val)
      goto error;

   hsv = (Field)_dxfeditor_to_hsv(hue,sat,val);
   if(!hsv)
      goto error;
    
   *rgb = (Field)_dxfMakeRGBColorMap((Field)hsv);
   DXDelete((Object)hsv);
   if (!*rgb)
      return ERROR;
   /* add the control points */
   if (!DXSetComponentValue(*rgb,"hue positions",(Object)hue))
      goto error;
   if (!DXSetComponentValue(*rgb,"sat positions",(Object)sat))
      goto error;
   if (!DXSetComponentValue(*rgb,"val positions",(Object)val))
      goto error;

   if (!DXEndField(*rgb))
      goto error;

   return OK;

error:
   return ERROR;
}

/* colormap is input if it has control points already do nothing 
 * otherwise create control points and add to the original colormap
 */
static 
Error colormap_to_rgb(Object inmap,Field *color)
{
   float *pts,min,max;
   Map *h,*s,*v;
   RGBColor *rgb;
   int i,num,num_rgb,num_pts;
   Array a,b,c,new_a;
   Type type;

   *color = (Field)DXCopy((Object)inmap,COPY_STRUCTURE);
   new_a = NULL;

   /* do control points exist */
   if (DXGetComponentValue((Field)*color,"hue positions") ||
       DXGetComponentValue((Field)*color,"sat positions") ||
       DXGetComponentValue((Field)*color,"val positions") )
      return OK;
   
   h = NULL; s = NULL; v = NULL;
   /* create control points by converting to hsv */
   if (!(a = (Array)DXGetComponentValue((Field)*color,"data"))){
      DXSetError(ERROR_BAD_PARAMETER,"colormap is bad");
      goto error;
   }
   if (!DXGetArrayInfo(a,&num_rgb,&type,NULL,NULL,NULL))
      goto error;
   if (type == TYPE_FLOAT){
      if (!(rgb = (RGBColor *)DXGetArrayData(a)))
         goto error;
   }
   else {
      new_a = DXArrayConvert(a,TYPE_FLOAT,CATEGORY_REAL,1,3);
      if (!(rgb = (RGBColor *)DXGetArrayData(new_a)))
         goto error;
   }


   if (!(b = (Array)DXGetComponentValue((Field)*color,"positions"))){
      DXSetError(ERROR_BAD_PARAMETER,"colormap is bad");
      goto error;
   }
   if (!DXGetArrayInfo(b,&num_pts,NULL,NULL,NULL,NULL))
      goto error;
   if (!(pts = (float *)DXGetArrayData(b)))
      goto error;
   min = pts[0];
   max = pts[num_pts-1];

   /* if dep connections create 2*num_rgb control points */
   if (num_pts == num_rgb+1)
      num = num_rgb*2;
   else num = num_rgb;
   
   if (!addarray(color, h, num,"hue positions"))
     goto error;
   if (!addarray(color, s, num,"sat positions"))
     goto error;
   if (!addarray(color, v, num,"val positions"))
     goto error;
   a = (Array)DXGetComponentValue(*color,"hue positions");
   b = (Array)DXGetComponentValue(*color,"sat positions");
   c = (Array)DXGetComponentValue(*color,"val positions");
   h = (Map *)DXGetArrayData(a);
   s = (Map *)DXGetArrayData(b);
   v = (Map *)DXGetArrayData(c);
   if (!h || !s || !v)
      goto error;
   
   if (num != num_rgb){ 	/* map dep connections */
      for (i=0; i<num_rgb; i++){
         if (!_dxfRGBtoHSV(rgb[i].r,rgb[i].g,rgb[i].b,
		        	&h[i*2].value,&s[i*2].value,&v[i*2].value))
            goto error;
	 h[i*2 +1].value = h[i*2].value;
	 s[i*2 +1].value = s[i*2].value;
	 v[i*2 +1].value = v[i*2].value;

      }
      h[0].level = s[0].level = v[0].level = (pts[0]-min)/(max-min); 
      for (i=1; i<num_pts-1; i++){
         h[i*2-1].level=s[i*2-1].level=v[i*2-1].level=(pts[i]-min)/(max-min);
	 h[i*2].level = s[i*2].level = v[i*2].level = (pts[i]-min)/(max-min);
      }
      h[num-1].level = s[num-1].level = v[num-1].level =
		                       (pts[num_pts-1]-min)/(max-min);
   }
   else{		/* map dep positions */ 
      for (i=0; i<num_rgb; i++){
         if (!_dxfRGBtoHSV(rgb[i].r,rgb[i].g,rgb[i].b,
		        	&h[i].value,&s[i].value,&v[i].value))
            goto error;
      }
      for (i=0; i<num_pts; i++){
         h[i].level = s[i].level = v[i].level = (pts[i]-min)/(max-min); 
      }
   }

   if (new_a) DXDelete((Object)new_a);
   return OK;

error:
   if (new_a) DXDelete((Object)new_a);
   return ERROR;   
}


/* from hvs data add control points to a Field */
static
Error hsvo_to_cp_component(Field *f,float *pts,float *data,int num,int isvec)
{
   Map *hue,*sat,*val,*op;
   int i;
   float min,max;
   Array a,b,c;

   hue = NULL; val = NULL; sat = NULL; op = NULL;

   switch (isvec){
      case(1):
         /* create normalized control points from hsv,op data and points */ 
         if (!addarray(f, NULL, num,"hue positions"))
           goto error;
         if (!addarray(f, NULL, num,"sat positions"))
           goto error;
         if (!addarray(f, NULL, num,"val positions"))
           goto error;
	 a = (Array)DXGetComponentValue(*f,"hue positions");
	 b = (Array)DXGetComponentValue(*f,"sat positions");
	 c = (Array)DXGetComponentValue(*f,"val positions");
         hue = (Map *)DXGetArrayData(a);
         sat = (Map *)DXGetArrayData(b);
         val = (Map *)DXGetArrayData(c);
	 min = pts[0];
	 max = pts[num-1];
         for (i=0; i<num; i++){
           hue[i].level = sat[i].level = val[i].level = (pts[i]-min)/(max-min);
           hue[i].value = data[i*3];
           sat[i].value = data[i*3 +1];
  	   val[i].value = data[i*3 +2];
         }
         break;
      case(0):
	 if (!addarray(f,op,num,"opacity positions"))
	    goto error;
         a = (Array)DXGetComponentValue(*f,"opacity positions");
	 op = (Map *)DXGetArrayData(a);
	 min = pts[0];
	 max = pts[num-1];
         for (i=0; i<num; i++){
           op[i].level = (pts[i]-min)/(max-min);
           op[i].value = data[i];
         }
	 break;
   }
   if (!DXEndField(*f))
      goto error;
   return OK;

error:
   return ERROR;
}

/* addarray of control points to field */
static
Error addarray(Field *f,Map *map,int items,char *component)
{
   Array a = NULL;

   a = DXNewArray(TYPE_FLOAT,CATEGORY_REAL,1,2);
   if (!a)
      return ERROR;
   
   if (!DXAddArrayData(a,0,items,(Pointer)map))
      goto error;
   
   if (!DXSetComponentValue(*f,component,(Object)a))
      goto error;

   return OK;

   error:
      if (a) DXDelete((Object)a);
      return ERROR;
}

/* scale the output to the new min and max */
static
Error scale_output(float *min,float *max,Field *rgb,Field *op,
int iprint[])
{
   Array a,b,newa,newb;
   int num_rgb,num_op;
   float *rgb_pts,*op_pts,*new_pts,*new_opts;
   float old_min,old_max;
   int i;

   newa = NULL;
   newb = NULL;
   /* data points are remapped if the min or max have changed */
   /* old_min and max is current range, the first and last points */
   /* extract the positions arrays and scale them to the new min and max */
   a = (Array)DXGetComponentValue((Field)*rgb,"positions");
   if (!DXGetArrayInfo(a,&num_rgb,NULL,NULL,NULL,NULL))
      goto error;
   rgb_pts = (float *)DXGetArrayData(a);
   b = (Array)DXGetComponentValue((Field)*op,"positions");
   if (!DXGetArrayInfo(b,&num_op,NULL,NULL,NULL,NULL))
      goto error;
   op_pts = (float *)DXGetArrayData(b);

   old_min = rgb_pts[0];
   old_max = rgb_pts[num_rgb -1];

   if (old_min!=*min || old_max!=*max){
      /* must copy points before you change them then set components */
      newa = DXNewArray(TYPE_FLOAT,CATEGORY_REAL,1,1);
      if (!newa)
	 goto error;
      if (!DXAddArrayData(newa,0,num_rgb,NULL))
	 goto error;
      new_pts = (float *)DXGetArrayData(newa);
      if (!new_pts)
	 goto error;
      for (i=0; i<num_rgb; i++){
	 new_pts[i] = rgb_pts[i];
         if (!_dxfnewvalue(&new_pts[i],old_min,old_max,*min,*max,1,&iprint[3]))
            goto error;
      }
      DXChangedComponentValues((Field)*rgb,"positions");
      DXSetComponentValue((Field)*rgb,"positions",(Object)newa);
      newa = NULL;
      if (!DXEndField((Field)*rgb))
         goto error;
   }
   old_min = op_pts[0];
   old_max = op_pts[num_op -1];
   if (old_min!=*min || old_max!=*max){
      newb = DXNewArray(TYPE_FLOAT,CATEGORY_REAL,1,1);
      if (!newb)
         goto error;
      if (!DXAddArrayData(newb,0,num_op,NULL))
         goto error;
      new_opts = (float *)DXGetArrayData(newb);
      if (!new_opts)
         goto error;
      for (i=0; i<num_op; i++){
	 new_opts[i] = op_pts[i];
         if (!_dxfnewvalue(&new_opts[i],old_min,old_max,*min,*max,1,&iprint[3]))
            goto error;
      }
      DXChangedComponentValues((Field)*op,"positions");
      DXSetComponentValue((Field)*op,"positions",(Object)newb);
      newb = NULL; 
      if (!DXEndField((Field)*op))
         goto error;
   }

   return OK;
error:
   if (newa) DXDelete((Object)newa);
   if (newb) DXDelete((Object)newa);
   return ERROR;
}


static
Error opacity_to_o(Object opacity,Field *out)
{
   float *pts,*data,min,max;
   Array a,b;
   Map *op;
   int num,i;
   Type type;

   *out = (Field)DXCopy((Object)opacity,COPY_STRUCTURE);

   if (DXGetComponentValue((Field)*out,"opacity positions"))
      return OK;
   
   /* create control points and add them to the field */
   a = (Array)DXGetComponentValue((Field)*out,"data");
   if (!a){
      DXSetError(ERROR_BAD_PARAMETER,"opacitymap is bad");
      goto error;
   }
   if (!DXGetArrayInfo(a,&num,&type,NULL,NULL,NULL))
      goto error;
   data = (float *)DXAllocate(sizeof(float) * num);
   if (!array_to_float(a,data,type,num))
      goto error;
   b = (Array)DXGetComponentValue((Field)*out,"positions");
   if (!b){
      DXSetError(ERROR_BAD_PARAMETER,"opacitymap is bad");
      goto error;
   }
   pts = (float *)DXGetArrayData(b);
   if (!pts)
      goto error;

   /* normalize the control points */
   min = pts[0];
   max = pts[num-1];
   op = (Map *)DXAllocate(sizeof(Map)*num);
   for (i=0; i<num; i++){
     op[i].level = (pts[i]-min)/(max-min);
     op[i].value = data[i];
   }
   if (!addarray(out,op,num,"opacity positions"))
      goto error;
   if (!DXEndField(*out))
      goto error;

   return OK;

error:
   return ERROR;  
}

static
int match_cpoints(Field o,Object defmap,int which)
{
   Map *map1, *map2;
   int i,item1=0, item2=0, match=0;
   Array inmap=NULL;

   map1=NULL;
   map2=NULL;

   switch (which){
      case(1):
         inmap = (Array)DXGetComponentValue((Field)o,"hue positions");
	 break;
      case(2):
         inmap = (Array)DXGetComponentValue((Field)o,"sat positions");
	 break;
      case(3):
         inmap = (Array)DXGetComponentValue((Field)o,"val positions");
	 break;
      case(4):
         inmap = (Array)DXGetComponentValue((Field)o,"opacity positions");
	 break;
   }

   if (inmap){
      if (!DXGetArrayInfo((Array)inmap,&item1,NULL,NULL,NULL,NULL))
         goto error;
      map1 = (Map *)DXGetArrayData((Array)inmap);
   }
   if (defmap){
      if (!inmap) return match;
      if (!DXQueryParameter((Object)defmap,TYPE_FLOAT,2,&item2)){
         DXSetError(ERROR_INTERNAL,"default control must be 2-vector floats");
         goto error;
      }
      if (item1 != item2)
	 return match;
      map2 = (Map *)DXAllocate(sizeof(Map) * item2);
      if (!DXExtractParameter((Object)defmap,TYPE_FLOAT,2,item2,(Pointer)map2)){
         DXSetError(ERROR_INTERNAL,"can't read control points");
         goto error;
      }
   }
   if (!inmap && !defmap) return (1);

   /* do the maps match */
   if (item1 == item2){
      for(i=0; i<item1; i++)
	 if (map1[i].level != map2[i].level || map1[i].value != map2[i].value){
	    goto done; 
      }
      match=1;
   }

done:
   if (map2) DXFree(map2);
   return match;

error:
   return -1;
}

static
Error array_to_float(Array a,float *data,Type type,int num)
{
   float *data_f;
   int *data_i;
   int i;

   switch (type){
      case(TYPE_FLOAT):
	 data_f = (float *)DXGetArrayData(a);
	 if (!data)
	    return ERROR;
	 for (i = 0; i<num; i++)
	    data[i] = (float)data_f[i];
	 break;
      case(TYPE_INT):
	 data_i = (int *)DXGetArrayData(a);
	 if (!data_i)
	    return ERROR;
	 for (i = 0; i<num; i++)
	    data[i] = (float)data_i[i];
	 break;
      default:
         break;
   }
   return OK;
}


static 
Error print_map(Field f,char *component,char *name, char *id)
{
   struct einfo ei;
   int num,msglen=0,shape[1];
   float *map;
   Array a;

   a = (Array)DXGetComponentValue((Field)f,component);
   if(a && !DXGetArrayInfo((Array)a,&num,NULL,NULL,NULL,NULL))
      goto error;
   if (!a || num==0){		/* no control points for this field */
      ei.msgbuf = (char *)DXAllocateZero(SLOP);
      if (!ei.msgbuf)
	 return ERROR;
      ei.mp = ei.msgbuf;
      sprintf(ei.mp,name);  while(*ei.mp) ei.mp++;
      sprintf(ei.mp, "NULL");
      DXUIMessage(id,ei.msgbuf);
      DXFree(ei.msgbuf);
      return OK;
   }

   map = (float *)DXGetArrayData((Array)a);
   /* send control points to UI in four separate messages */
   msglen = 2*num *NUMBER_CHARS + NAME_CHARS; 
   /* print message */
   ei.maxlen = msglen;
   ei.atend = 0;
   ei.msgbuf = (char *)DXAllocateZero(ei.maxlen+SLOP);
   if (!ei.msgbuf)
     return ERROR;
   ei.mp = ei.msgbuf;
   shape[0]=2;
   strcpy(ei.mp,"");
   sprintf(ei.mp,name); while(*ei.mp) ei.mp++;
   if (num==1){
      sprintf(ei.mp,"{"); ei.mp++;
   }
   if (!_dxfprint_message(map,&ei,TYPE_FLOAT,1,shape,num))
     goto error;
   if (num==1){
      sprintf(ei.mp,"}"); ei.mp++;
   }
   /* in DXMessage there is static buffer of MAX_MSGLEN so check length */
   if (strlen(ei.msgbuf) > MAX_MSGLEN){
      DXSetError(ERROR_DATA_INVALID,"#10920");
      goto error;
   }
   DXUIMessage(id,ei.msgbuf);
   DXFree(ei.msgbuf);
   
   return OK;

error:
   if (ei.msgbuf) DXFree(ei.msgbuf);
   return ERROR;
}

/* for old version need to get the UI min and max from the points
 * input (the first and last were alway the min and max before)
 */
static
Error getui_minmax(Object o,float *min, float *max)
{
   float *pts;
   int num;

   pts = NULL;
   /* read the point lists */
   if (!DXQueryParameter((Object)o,TYPE_FLOAT,0,&num)){
      DXSetError(ERROR_INTERNAL,"pts scalar floats");
      goto error;
   } 
   pts = (float *)DXAllocate(sizeof(float) * num);
   if (!DXExtractParameter((Object)o,TYPE_FLOAT,0,num,(Pointer)pts)){
    DXSetError(ERROR_INTERNAL,"can't read points");
    goto error;
   }
   *min = pts[0];
   *max = pts[num-1];

   DXFree((Pointer)pts);
   return OK;

error:
   if (pts) DXFree((Pointer)pts);
   return ERROR;
}

static
Object gethistogram(Object o,int nbins,int outl,float *min,float *max)
{
   int rank;
   Object histogram,newo;
   Class class,classg;

   /* Copy object so can change to scalar */
   newo = DXCopy(o,COPY_STRUCTURE);
 
   /* make sure group is right type */
   class = DXGetObjectClass((Object)newo);
   if (class==CLASS_GROUP){
      classg = DXGetGroupClass((Group)newo);
      if (classg!=CLASS_MULTIGRID && classg!=CLASS_SERIES && 
          classg!=CLASS_COMPOSITEFIELD){
         DXSetError(ERROR_BAD_PARAMETER,"group must be multigrid,or series");
         goto error;
      }
   }
           
   /* if data type is vector need to get magnitude */
   if (!DXGetType(newo,NULL,NULL,&rank,NULL)){
      DXSetError(ERROR_INTERNAL,"there is no type");
      goto error;
   }
   if (rank>0){
      if (!vect_scalar(newo))
         goto error; 
   }


   histogram = (Object)_dxfHistogram(newo,nbins,min,max,outl);
   /* 
   {
   Array a;
   int items, i, *p;
   Type t;
   int *localPts;

   if (!histogram)
      return ERROR;
   if ((class=DXGetObjectClass(histogram))==CLASS_FIELD){
      a = (Array)DXGetComponentValue((Field)histogram,"data");
      if (!DXGetArrayInfo(a,&items,&t, NULL,NULL,NULL))
         return ERROR;
      p = (int *)DXGetArrayData(a);
      *pts = localPts = (int *)DXAllocate(sizeof(int) *items);
      for (i=0; i<items; i++)
         localPts[i] = p[i];
   }
   else{
      DXSetError(ERROR_INTERNAL,"histogram is not field");
      goto error;
   }
      
   DXDelete(histogram);
   }
   */

   DXDelete((Object)newo);
   return (Object)histogram;
 
error:
   DXDelete((Object)newo);
   return NULL;
}

static
Error vect_scalar(Object o)
{
   int i;
   Object oo;

   switch (DXGetObjectClass(o)){
   case CLASS_FIELD:
      if (!makescalar(o))
         return ERROR;
      break;
   case CLASS_GROUP:
      for (i=0; (oo=DXGetEnumeratedMember((Group)o,i,NULL)); i++){
         if (!vect_scalar(oo))
            return ERROR;
      }
      if (!DXSetGroupType((Group)o,TYPE_FLOAT,CATEGORY_REAL,0))
	 return ERROR;
      break;
   default:
      DXSetError(ERROR_DATA_INVALID,"#10190");
      return ERROR;
   }

   return OK;
}

static
Error makescalar(Object o)
{
   Array va,sa;

      if (!(va = (Array)DXGetComponentValue((Field)o,"data")))
         return ERROR;
      if (!(sa = DXScalarConvert((Array)va)))
         return ERROR;
      if (!DXSetComponentValue((Field)o,"data",(Object)sa))
         return ERROR;

   return OK;
}

