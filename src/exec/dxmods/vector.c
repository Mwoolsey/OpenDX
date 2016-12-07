/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>

#if defined(HAVE_STRING_H)
#include <string.h>
#endif

#include "interact.h"
#include "vectors.h"

#define MAX_INPUTS 9

extern Array DXScalarConvert(Array ); /* from libdx/stats.h */

static Error vec_minmax(Object, int , int *,int , float *, float *); 
static Error constant_minmax(Array , Type ,int ,float *, float *);
static Error IsInvalid(Object, InvalidComponentHandle *);
static Error vec_reset(float *, float *, int , int , int , start_type, float *);
static Error setoutput(float *, int , int , int , int , Object *);
static int isthisint(float *,int ,int );

int 
m_Vector(Object *in, Object *out)
{
   if (!_dxfvector_base(in, out,0))
      return ERROR;
   return OK;
}

Error _dxfvector_base(Object *in, Object *out, int islist)
{
   struct einfo ei;
   float valtemp=0;
   float *min, *max, *incr;
   int *decimal;
   float *val,min1,max1,incr1;
   char *label, *id, *incrmethod, *startstr;
   int i,item=0,change=0,j,n,nitem=-1,range=0,isint=0, start=START_MIDPOINT;
   int rank=1,dim=3,size,valdim,dec1;
   int change1,change4,change5,changen=0,req_param=1 /*,decimalzero=1*/;
   method_type method;
   Object idobj;
   int iprint[MAXPRINT],ip[MAXPRINT],msglen=0,shape[1];
   int refresh = 0;

   /* required parameters */
   if (!in[1] && (!in[4] || !in[5]))
      req_param=0;
    ei.msgbuf=NULL;

    /* get id */
    if (!DXExtractString(in[0], &id)){
        DXSetError(ERROR_BAD_PARAMETER,"#10000","id");
        return ERROR;
    }
    idobj=in[0];
  
   val = NULL;
   /* initialize ip to 0 (print nothing) */
   for (i=0; i<MAXPRINT; i++)
      ip[i]=0;

   /* get the dimension (all inputs must be same dimension)
    * find first existing input and set dimension
    * allocate space for all inputs
    */ 
   for (i=1; i<MAX_INPUTS; i++){
      if (in[i] && i!=7 && i!=2){
         if (!DXGetType(in[i],NULL,NULL,&n,&j))
            return ERROR;
         if (n==1){
            rank = n;
            dim = j;
            break;
         }
      }
   }
   min = (float *)DXAllocate(sizeof(float) *dim);
   max = (float *)DXAllocate(sizeof(float) *dim);
   incr = (float *)DXAllocate(sizeof(float) *dim);
   decimal = (int *)DXAllocate(sizeof(int) *dim);
   for (i=0; i<dim; i++){
      min[i] = DXD_MAX_FLOAT;
      max[i] = -DXD_MAX_FLOAT;
   }

   /* read in object, data comp must be vector(any dimension) 
    * check cache if change then get min and max else change=0 
    * find min,max of each dimension 
    */
   change1 = _dxfcheck_obj_cache(in[1],id,0,idobj);
   change4 = _dxfcheck_obj_cache(in[4],id,4,idobj);
   change5 = _dxfcheck_obj_cache(in[5],id,5,idobj);

   if (change1 > 0 || change4 >0 || change5 >0 ){
      change=1;
      /* DXWarning("getting min max, change=%d",change); */

      if (in[1]){
      /* function vec_minmax takes vector array and returns min,max
       * of each dimension 
       */
         if (!DXGetType(in[1],NULL,NULL,&rank,NULL))
            goto error;
         if (rank!=1){
            DXSetError(ERROR_DATA_INVALID,"#10221","input data");
            goto error;
         }
         if (!in[4] || !in[5]){
            if (!vec_minmax(in[1],dim,&size,rank,min,max))
               goto error;
         }
      }

      if (in[4]){
         if (DXExtractFloat(in[4],&min1)){
            for (i=0; i<dim; i++)
               min[i] = min1;
         }
         else{
            if (!DXExtractParameter(in[4], TYPE_FLOAT,dim,1,(Pointer)min)){
               DXSetError(ERROR_BAD_PARAMETER,"#10230","min",dim);
               goto error;
            }
         }
      }
      else if (!in[1]){
         for (i=0; i<dim; i++)
            min[i] = -1000000;
      }
      if (in[5]){
         if (DXExtractFloat(in[5],&max1)){
            for (i=0; i<dim; i++)
               max[i] = max1;
         }
         else{
            if (!DXExtractParameter(in[5], TYPE_FLOAT,dim,1,(Pointer)max)){
               DXSetError(ERROR_BAD_PARAMETER,"#10230","max",dim);
               goto error;
            }
         }
      }
      else if (!in[1]){
         for (i=0; i<dim; i++)
            max[i] = 1000000;
      }
   
      /* Print warning if max<min */
      for (i=0; i<dim; i++){
         if (max[i] < min[i]){ 
            DXSetError(ERROR_BAD_PARAMETER,"#10150","min",max[i] );
            goto error;
         }
      }
      if (in[1] && (in[4] || in[5]))
         DXWarning("the min and/or max values have been specified; these override the min/max of the data");
   }
    
   /* read increment(default 1/20), decimal and label
    * make them the same dimension as data
    */
   if (in[7]){
      if (!DXExtractString(in[7],&incrmethod)){
         DXSetError(ERROR_BAD_PARAMETER,"#10200","increment method");
         goto error1;
       }
       if (!strncmp("absolute",incrmethod,8))
          method = ABSOLUTE;
       else if (!strncmp("relative",incrmethod,8)){
          if (req_param==0){
             DXSetError(ERROR_BAD_PARAMETER,
              "#10925",incrmethod,"use 'absolute'");
             goto error;
          }
          method = PERCENT;
       }
       else if (!strncmp("rounded",incrmethod,7)){
          if (req_param==0){
             DXSetError(ERROR_BAD_PARAMETER,
              "#10925",incrmethod,"use 'absolute'");
             goto error;
          }
          method = PERCENT_ROUND;
       }
       else{
          DXSetError(ERROR_BAD_PARAMETER,"#10480","method");
          goto error1;
       }
   }
   else{
      if (req_param==0) method = ABSOLUTE;
      else method = PERCENT_ROUND;
   }

   if (in[6]){
      if (DXExtractFloat(in[6], &incr1)){
         for (i=0; i<dim; i++)
            incr[i] = incr1;
      }
      else{
         if (!DXExtractParameter(in[6], TYPE_FLOAT,dim,1,(Pointer)incr)){
           DXSetError(ERROR_BAD_PARAMETER,"#10230","delta",dim);
           goto error1;
         }
      }
      for (i=0; i<dim; i++){
         if (method!=ABSOLUTE && (incr[i] < 0.0 || incr[i] > 1.0)){
            DXSetError(ERROR_BAD_PARAMETER,"#10110","delta",0,1);
            goto error1;
         }
         else if (method==ABSOLUTE && incr[i] < 0.0){
            DXSetError(ERROR_BAD_PARAMETER,"#10010","delta");
            goto error1;
         }
      }
   }
   else{
      if (in[7] && method==ABSOLUTE){
         for (i=0; i<dim; i++)
            incr[i] = 1.0;
      }
      else{
         for (i=0; i<dim; i++)
            incr[i] = .01;
      }
   }

   if (in[10]){
      if (!DXExtractString(in[10], &label)){
        DXSetError(ERROR_BAD_PARAMETER,"#10200","label");
        goto error1;
      }
   }
   else
      label="";
  
   if (in[8]){ 
      if (DXExtractInteger(in[8],&dec1)){
         for (i=0; i<dim; i++)
            decimal[i] = dec1;
      }
      else{
         if (!DXExtractParameter(in[8],TYPE_INT,dim,1,(Pointer)decimal)){
	    DXSetError(ERROR_BAD_PARAMETER,"#10230","decimal",dim);
            goto error1;
         }
      }
      n=0;
      for (i=0; i<dim; i++){
	 if (decimal[i] != 0) n++; 
         if (decimal[i] < 0 || decimal[i] > 6){
             DXSetError(ERROR_BAD_PARAMETER,"#10040","decimal",0,6);
             goto error1;
         }
      }
      /*if (n > 0)decimalzero = 0;*/
   }
   else{
      if (req_param==0){
         for (i=0; i<dim; i++)
            decimal[i]=5;
      }
      else{
         for (i=0; i<dim; i++)
            decimal[i]=DXD_MAX_INT;
      }
   }

   if (islist){
      if (in[9]){
         if (req_param==0){
            DXSetError(ERROR_BAD_PARAMETER,
                "#10925","nitems","");
            goto error1;
         }
         if (!DXExtractInteger(in[9],&nitem) || nitem<0){
            DXSetError(ERROR_BAD_PARAMETER,"#10020","default");
            goto error1;
         }
      }
      else
         item = 11;
   }
   else {
      item=1;
      if(in[9]) {
		  if (!DXExtractString(in[9],&startstr)){
			 DXSetError(ERROR_BAD_PARAMETER,"#10200","start");
			 goto error1;
		   }
		   if (!strncmp("minimum",startstr,7))
			  start = START_MINIMUM;
		   else if (!strncmp("midpoint",startstr,8))
			  start = START_MIDPOINT;
		   else if (!strncmp("maximum",startstr,7))
			  start = START_MAXIMUM;
		   else{
			  DXSetError(ERROR_BAD_PARAMETER,"#10480","start");
			  goto error1;
		   }
      }
   }

   if (in[3]) {
      if (req_param==0){
         DXSetError(ERROR_BAD_PARAMETER,"#10925","refresh","");
         goto error1;
      }
      if (!DXExtractInteger(in[3],&refresh) || refresh<0 || refresh > 1){
         DXSetError(ERROR_BAD_PARAMETER,"must be 0 or 1");
         goto error1;
      }
   }
   else
      refresh = 0;

   /* cache attributes and get min and max if unchanged */
   for (n=0; n<dim; n++){
      if (! _dxfinteract_float(id,idobj,&min[n],&max[n],
                            &incr[n],&decimal[n],label,n,method,&nitem,iprint))
         goto error;
      for (j=0; j<MAXPRINT; j++)
         if (iprint[j]==1)
            ip[j]=iprint[j];
   }

   if (refresh == 1){
      if (nitem >=0) item=nitem;
      val = (float *)DXAllocate(sizeof(float)*item *dim);
      if (!vec_reset(min,max,item,dim,-1,start,val)) 
         goto error1;
      ip[4]=1;
   }
   else {   
   if (islist) changen=ip[6];
   if (ip[0]==1 || ip[1]==1) change=1;
   else change=0;

   /* read in scalar parameter 
    * if no change then pass flag and use previously cached value
    * possible exec changed it previously
    */
   if (in[2]){
      if (!DXGetArrayInfo((Array)in[2],&item,NULL,NULL,NULL,&valdim))
         goto error1;
      if (req_param==1 && (valdim != dim || changen==1)){
         /* if dimension or number of items change, reset */
         if (nitem >=0) item=nitem;
         val = (float *)DXAllocate(sizeof(float)*item *dim);
         if (!vec_reset(min,max,item,dim,-1,start,val)) 
            goto error1;
         ip[4]=1;
      }
      else{
       if (change==0){
         val = (float *)DXAllocate(sizeof(float)*item *dim);
         if (!DXExtractParameter(in[2],TYPE_FLOAT,valdim,item, (Pointer)val)){
            DXSetError(ERROR_INTERNAL,"can't extract current value");
            goto error1;
         }
       }
       /* check for outofrange the min or max have changed*/
       else{
         val = (float *)DXAllocate(sizeof(float)*item *dim);
         if (valdim != dim){
          for (i=0; i<item; i++){
            for (j=0; j<dim; j++){
               if (in[4]) val[i*dim +j] = min[j];
               else if (in[5]) val[i*dim +j] = max[j];
               else val[i*dim +j] = 0.0;
            }
          }
          ip[4]=1;
         }
         else{
          if (!DXExtractParameter(in[2],TYPE_FLOAT,valdim,item, (Pointer)val)){
            DXSetError(ERROR_INTERNAL,"can't extract current value");
            goto error1;
          }
         }
         if (req_param==1){
            for (i=0; i<item; i++){
               for (j=0; j<dim; j++){
                  range = OUTOFRANGE(val[i*dim +j],min[j],max[j]);
                  if (range==1){
                     if (nitem >=0) item=nitem;
		     DXFree((Pointer)val);
                     val = (float *)DXAllocate(sizeof(float)*item *dim);
                     if (!vec_reset(min,max,item,dim,-1,start,val))
                        goto error1;
                     ip[4]=1;
   		     break;
                  }
	       }
	       if (range==1)
     		 break;
            }
         }
         else{
            for (i=0; i<item; i++){
               for (j=0; j<dim; j++){
               if (in[4]) valtemp = CLAMPMIN(val[i*dim +j],min[j]);
               if (in[5]) valtemp = CLAMPMAX(val[i*dim +j],max[j]);
               if (valtemp != val[i*dim +j])
                  ip[4]=1;
               val[i*dim +j] = valtemp;
               }
            }
         }
       }
      }
   }
   else if (req_param==1){
      /* first time placed */
      if (nitem>=0) item=nitem;
      val = (float *)DXAllocate(sizeof(float)*item*dim);
      if (!vec_reset(min,max,item,dim,-1,start,val))
         goto error1;
      ip[4]=1;
   }
   else{
      item=1;
      val = (float *)DXAllocate(sizeof(float)*item *dim);
      for (j=0; j<dim; j++){
         val[j] = 0.0;
         if (in[4]) valtemp = CLAMPMIN(val[j],min[j]);
         if (in[5]) valtemp = CLAMPMAX(val[j],max[j]);
         val[j] = valtemp;
      }
      ip[4]=1;
   }
   } 
   /* are these integer or float values, if decimal specified > 0 float */
   if (!in[8])
      isint = isthisint(val,dim,item);
   else{
      for(n=0; n<dim; n++){
	 isint=1;
         if (decimal[n] > 0) isint = 0;
      }
   }
   if (isint==0){
      for(n=0; n<dim; n++)
	 if (decimal[n] == 0) decimal[n]=1;
   }
   else{	/* change values to be integers (still fp) */
      ip[4]=1;
      for (i=0; i<item*dim; i++)
	 val[i] = (int)val[i];
   }

   /* if req_params don't exist print out what is specified */
   if (req_param==0){
      if (ip[0]==1 && !in[4]) ip[0]=0;
      if (ip[1]==1 && !in[5]) ip[1]=0;
      if (ip[2]==1 && !in[6]) ip[2]=0;
      if (ip[3]==1 && !in[8]) ip[3]=0;
   }

   /* calculate the size of the UI message */
   if (ip[4] == 1) msglen += (dim*item*NUMBER_CHARS);
   if (ip[5] == 1) msglen += strlen(label);

   for (i=0; i < MAXPRINT; i++){
      if (ip[i] == 1)
	 msglen += (dim*NUMBER_CHARS + NAME_CHARS);
   }
   if (in[7])
      msglen += (NAME_CHARS + METHOD_CHARS);
   if (!islist && in[9])
      msglen = msglen + NAME_CHARS + METHOD_CHARS;
   
   msglen += NAME_CHARS; /* for the dim statement */

   /* Print out the message
    * min,max,new value(s)
    */
   ei.maxlen = msglen;
   ei.atend = 0;
   ei.msgbuf = (char *)DXAllocateZero(ei.maxlen+SLOP);
   if (!ei.msgbuf)
      return ERROR;
   ei.mp =ei.msgbuf;

   shape[0] = 1;
   strcpy(ei.mp,"");
   if (ip[0]==1 || ip[1]==1 || ip[2]==1 || ip[3]==1 || ip[4]==1){
      sprintf(ei.mp,"dim="); while(*ei.mp) ei.mp++;
      if (!_dxfprint_message(&dim, &ei, TYPE_INT,0,shape,1))
         goto error1;
   }
   shape[0] = dim;
   if (ip[0]==1){
     sprintf(ei.mp,"min="); while(*ei.mp) ei.mp++;
     if (!_dxfprint_message(min, &ei, TYPE_FLOAT,rank,shape,1))
        goto error1;
   }
   if (ip[1]==1){
     sprintf(ei.mp, "max="); while(*ei.mp) ei.mp++;
     if (!_dxfprint_message(max, &ei, TYPE_FLOAT,rank,shape,1))
        goto error1;
   }
   if (ip[2]==1){
     sprintf(ei.mp, "delta="); while(*ei.mp) ei.mp++;
     if (!_dxfprint_message(incr, &ei,TYPE_FLOAT,rank,shape,1))
       goto error1;
   }
   if (ip[3]==1){
     sprintf(ei.mp, "decimals="); while(*ei.mp) ei.mp++;
     if (!_dxfprint_message(decimal, &ei, TYPE_INT,rank,shape,1))
        goto error1;
   }
   if (ip[4]==1){
     if (islist){
        sprintf(ei.mp, "list="); while(*ei.mp) ei.mp++;
        if (item==1){sprintf(ei.mp,"{"); while(*ei.mp) ei.mp++;}
     }
     else{ sprintf(ei.mp, "value="); while(*ei.mp) ei.mp++;}
     if (!_dxfprint_message(val, &ei, TYPE_FLOAT,rank,shape,item))
        goto error1;
     if (islist && item==1){sprintf(ei.mp,"}"); while(*ei.mp) ei.mp++;}
   }
   if (in[7]){
      if (method==ABSOLUTE) sprintf(ei.mp, "method=\"absolute\"");
      else if(method==PERCENT) sprintf(ei.mp, "method=\"relative\"");
      else sprintf(ei.mp, "method=\"rounded\"");
      while(*ei.mp) ei.mp++;
   }
   if (!islist && in[9]){
      if(start==START_MINIMUM) sprintf(ei.mp, "start=\"minimum\"");
      else if(start==START_MIDPOINT) sprintf(ei.mp, "start=\"midpoint\"");
      else sprintf(ei.mp, "start=\"maximum\"");
      while(*ei.mp) ei.mp++;
   }
   if (ip[5]==1 && strlen(label)>0){
      shape[0]=(int)strlen(label);
     sprintf(ei.mp,"label="); while(*ei.mp) ei.mp++;
     if (!_dxfprint_message(label, &ei,TYPE_STRING,1,shape,1))
        goto error1;
   }

   /* in DXMessage there is static buffer of MAX_MSGLEN so check length */
   if (strlen(ei.msgbuf) > MAX_MSGLEN){
      DXSetError(ERROR_DATA_INVALID,"#10920");
      goto error1;
   }

   if (strcmp(ei.msgbuf,""))
   DXUIMessage(id,ei.msgbuf);

   /* create array for output object */
   if (ip[4]==1){
      if (item==0) out[0]=NULL;
      else{
         if (!setoutput(val,rank,item,dim,isint,out))
            goto error;
      }
   }
   else
      out[0]=in[2];

   DXFree(min);
   DXFree(max);
   DXFree(incr);
   DXFree(decimal);
   DXFree(ei.msgbuf);
   if (val) DXFree((Pointer)val);
   return OK;

error:
error1:
   /* change cache of data object so min and max will be cached */
   change1 = _dxfcheck_obj_cache(in[0],id,0,idobj);
   DXFree(min);
   DXFree(max);
   DXFree(incr);
   DXFree(decimal);
   if (val) DXFree(val);
   if (ei.msgbuf) DXFree(ei.msgbuf);
   return ERROR;

}

static
Error vec_minmax(Object o,int dim,int *size,int rank, float *min, float *max) 
{
   float *vec_f=NULL;
   int *vec_i=NULL;
   double *vec_d=NULL;
   short *vec_s=NULL;
   byte *vec_b=NULL;
   uint *vec_ui=NULL;
   ubyte *vec_ub=NULL;
   ushort *vec_us=NULL;
   int i,ig,j;
   Array a;
   float v;
   Type type;
   Category category;
   Object oo;
   Class class;
   InvalidComponentHandle inv=NULL;
 
   /* determine class of object, if group recurse */
   class = DXGetObjectClass(o);
   switch (class){
  
   case CLASS_FIELD:
   case CLASS_ARRAY:

     if (class!=CLASS_ARRAY){ 
        a = (Array)DXGetComponentValue((Field)o,"data");
        if(!a){
           DXSetError(ERROR_BAD_PARAMETER,"#10250","object","data");
           return ERROR;
        }
        /* is there invalid data with this field */
        if (!IsInvalid(o,&inv))
	   return ERROR;
     }
     else
        a = (Array)o;

     if (!DXGetArrayInfo(a,size,&type,&category,NULL,NULL))
        return ERROR;
     if (category != CATEGORY_REAL){
        DXSetError(ERROR_BAD_TYPE,"#11150","data");
        return ERROR;
     }

     /* if class constant array return min, max */
     if (DXQueryConstantArray(a,NULL,NULL)){
	if (!constant_minmax(a,type,dim,min,max))
           return ERROR;
        return OK;
     }


   /* if one-vector use statistics to get min and max */
   if (dim==1){
      if (!DXStatistics(o, "data", &min[0], &max[0], NULL, NULL))
         return ERROR;
      return OK;
   }
      
   /* get pointer to data dependant on type */
   switch (type){
   case (TYPE_FLOAT):
      vec_f = (float *)DXGetArrayData(a); 
      break;
   case (TYPE_INT):
      vec_i = (int *)DXGetArrayData(a);
      break;
   case (TYPE_DOUBLE):
      vec_d = (double *)DXGetArrayData(a);
      break;
   case (TYPE_SHORT):
      vec_s = (short *)DXGetArrayData(a);
      break;
   case (TYPE_BYTE):
      vec_b = (byte *)DXGetArrayData(a);
      break;
   case (TYPE_UINT):
      vec_ui = (uint *)DXGetArrayData(a);
      break;
   case (TYPE_UBYTE):
      vec_ub = (ubyte *)DXGetArrayData(a);
      break;
   case (TYPE_USHORT):
      vec_us = (ushort *)DXGetArrayData(a);
      break;
   default:
      DXSetError(ERROR_BAD_TYPE,"#10600","data type");
      return ERROR;
   }   


   for (i=0; i<(*size); i++){
      if (inv && (DXIsElementInvalidSequential(inv,i)))
         continue;
      for (j=0; j<dim; j++){
         switch(type){
         case (TYPE_FLOAT):
            v = (float)vec_f[i*(dim) +j];
            break;
         case (TYPE_INT):
            v = (float)vec_i[i*(dim) +j];
            break;
         case (TYPE_DOUBLE):
            v = (float)vec_d[i*(dim) +j];
            break;
         case (TYPE_SHORT):
	    v = (float)vec_s[i*(dim) +j];
            break;
         case (TYPE_BYTE):
            v = (float)vec_b[i*(dim) +j];
            break;
         case (TYPE_UINT):
            v = (float)vec_ui[i*(dim) +j];
            break;
         case (TYPE_UBYTE):
            v = (float)vec_ub[i*(dim) +j];
            break;
         case (TYPE_USHORT):
            v = (float)vec_us[i*(dim) +j];
            break;

         default:
            DXSetError(ERROR_BAD_TYPE,"#10600","data type");
               return ERROR;
         }
         min[j] = MIN(v, min[j]);
         max[j] = MAX(v, max[j]);
      }
   }

   break;  

   case CLASS_SCREEN:
   
   if (!DXGetScreenInfo((Screen)o, &oo, NULL,NULL))
      return ERROR;
   if (!vec_minmax(oo,dim,size,rank,min,max))
      return ERROR;
   break;

   case CLASS_XFORM:

   if (!DXGetXformInfo((Xform)o, &oo, NULL))
      return ERROR;
   if (!vec_minmax(oo,dim,size,rank,min,max))
      return ERROR;
   break;
  
   case CLASS_CLIPPED:

   if (!DXGetClippedInfo((Clipped)o, &oo, NULL))
      return ERROR;
   if (!vec_minmax(oo,dim,size,rank,min,max))
      return ERROR;
   break;
 
   case CLASS_GROUP:

   for (ig=0; (oo=DXGetEnumeratedMember((Group)o,ig,NULL)); ig++)
      if (!vec_minmax(oo,dim,size,rank,min,max))
         return ERROR;
   break;

   default:
      DXSetError(ERROR_BAD_TYPE,"#10190","object");
      return ERROR;
   } 
   

   return OK;
}

static
Error constant_minmax(Array a, Type type, int dim, float *min, float *max)
{
   float *vec_f;
   int *vec_i;
   double *vec_d;
   short *vec_s;
   byte *vec_b;
   uint *vec_ui;
   ubyte *vec_ub;
   ushort *vec_us;
   int i;
   
        switch(type){
           case (TYPE_FLOAT):
           vec_f = (float *)DXAllocate(sizeof(float) *dim);
              DXQueryConstantArray(a,NULL,vec_f);
              for (i=0; i<dim; i++)
                 min[i] = vec_f[i];
	      DXFree(vec_f);
              break;
           case (TYPE_INT):
           vec_i = (int *)DXAllocate(sizeof(int) *dim);
              DXQueryConstantArray(a,NULL,vec_i);
              for (i=0; i<dim; i++)
                 min[i] = vec_i[i];
	      DXFree(vec_i);
              break;
           case (TYPE_DOUBLE):
           vec_d = (double *)DXAllocate(sizeof(double) *dim);
              DXQueryConstantArray(a,NULL,vec_d);
              for (i=0; i<dim; i++)
                 min[i] = (double)vec_d[i];
	      DXFree(vec_d);
              break;
           case (TYPE_SHORT):
           vec_s = (short *)DXAllocate(sizeof(short) *dim);
              DXQueryConstantArray(a,NULL,vec_s);
              for (i=0; i<dim; i++)
                 min[i] = (double)vec_s[i];
	      DXFree(vec_s);
              break;
           case (TYPE_BYTE):
           vec_b = (byte *)DXAllocate(sizeof(byte) *dim);
              DXQueryConstantArray(a,NULL,vec_b);
              for (i=0; i<dim; i++)
                 min[i] = (double)vec_b[i];
	      DXFree(vec_b);
              break;
           case (TYPE_UINT):
           vec_ui = (uint *)DXAllocate(sizeof(uint) *dim);
              DXQueryConstantArray(a,NULL,vec_ui);
              for (i=0; i<dim; i++)
                 min[i] = (double)vec_ui[i];
	      DXFree(vec_ui);
              break;
           case (TYPE_USHORT):
           vec_us = (ushort *)DXAllocate(sizeof(ushort) *dim);
              DXQueryConstantArray(a,NULL,vec_us);
              for (i=0; i<dim; i++)
                 min[i] = (double)vec_us[i];
	      DXFree(vec_us);
              break;
           case (TYPE_UBYTE):
           vec_ub = (ubyte *)DXAllocate(sizeof(ubyte) *dim);
              DXQueryConstantArray(a,NULL,vec_ub);
              for (i=0; i<dim; i++)
                 min[i] = (double)vec_ub[i];
	      DXFree(vec_ub);
              break;
         default:
            DXSetError(ERROR_BAD_TYPE,"#10600","data type");
            return ERROR;

        }
        DXWarning("constant min %g,%g,%g",min[0],min[1],min[2]);
        for (i=0; i<dim; i++)
           max[i] = min[i];

        return OK;
}

static
Error vec_reset(float *min,float *max,int item,int dim,int offset, start_type start,float *value)
{
   int i,j;
   
   if (item==1){
      if (start==START_MINIMUM){
          if (offset >= 0)
              value[offset] = min[offset];
          else {
              for (j=0; j<dim; j++)
                  value[j] = min[j];
          }
      }
      else if (start==START_MIDPOINT) {
		  if (offset >= 0)
			  value[offset] = (max[offset] +min[offset])/2.0;
		  else{
			 for (j=0; j<dim; j++)
				value[j] = (max[j] +min[j])/2.0;
		  }
      }
      else {
          if (offset >= 0)
              value[offset] = max[offset];
          else {
              for (j=0; j<dim; j++)
                  value[j] = max[j];
          }
      }
      return OK;
   }
   
   if (offset >= 0){
      for (i=0; i<item; i++)
            value[i*dim +offset] = i * ((max[offset]-min[offset])/(item-1))                  +min[offset];
      return OK;
   }
   else{    
      for (i=0; i<item; i++){
         for (j=0; j<dim; j++)
            value[i*dim +j] = i * ((max[j]-min[j])/(item-1)) +min[j];
      }
      return OK;
   }
}

static 
int isthisint(float *val,int dim,int item)
{
   int i,j,notint=0;
   
   for (i=0; i<item; i++){
     for (j=0; j<dim; j++){
      if (val[i*dim +j] == (float)(int)val[i*dim +j])
         ;
      else{
         notint=1;
         break;
      }
     }
     if (notint==1) break;
   }
   if (notint==1) return 0;
   return 1; 
}

static
Error setoutput(float *val,int rank,int item,int dim,int isint,Object *out)
{
   int i;
   int *ival;

   ival = NULL;
   out[0] = NULL;
   if (isint==1){
      ival = DXAllocate(sizeof(int) * dim * item);
      for (i=0; i<dim*item; i++)
         ival[i]=val[i];
      out[0] = (Object)DXNewArray(TYPE_INT, CATEGORY_REAL, rank,dim);
      if (!out[0] || !DXAddArrayData((Array)out[0], 0, item, (Pointer)ival))
         goto error;
      DXFree(ival);
   }
   else{
      out[0] = (Object)DXNewArray(TYPE_FLOAT, CATEGORY_REAL, rank,dim);
      if (!out[0] || !DXAddArrayData((Array)out[0], 0, item, (Pointer)val))
         goto error;
   }
   return OK;

error:
   if (ival) DXFree(ival);
   if (out[0]) DXDelete((Object)out[0]);
   out[0]=NULL;
   return ERROR;
}

static
Error IsInvalid(Object o, InvalidComponentHandle *ivh)
{
   Array invalid;
   char *attr;

   *ivh = NULL;
   attr = DXGetString((String)DXGetComponentAttribute((Field)o,"data","dep"));
   if (!strcmp(attr,"connections")){
      invalid = (Array)DXGetComponentValue((Field)o,"invalid connections");
      if (invalid){
         *ivh = DXCreateInvalidComponentHandle((Object)o,NULL,"connections");
         if (!*ivh)
            return ERROR;
      }
   }
   else{
      invalid = (Array)DXGetComponentValue((Field)o,"invalid positions");
      if (invalid){
         *ivh = DXCreateInvalidComponentHandle((Object)o,NULL,"positions");
         if (!*ivh)
            return ERROR;
      }
   }
   return OK;
}
