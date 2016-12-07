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

#include "integer.h"
#include "interact.h"

static Error int_reset(float , float , int, start_type, int *);

int 
m_Integer(Object *in, Object *out)
{
   if (!_dxfinteger_base(in, out,0))
      return ERROR;
   return OK;
}

Error _dxfinteger_base(Object *in, Object *out, int islist)
{
   struct einfo ei;
   float min,max,valtemp;
   float val,incr;
   char *label, *id, *incrmethod, *startstr;
   int i,item=0,change=0,range,nitem=-1;
   int change1,change4,change5,changen=0,req_param=1, start=START_MIDPOINT;
   int *ival,iincr,imax,imin;
   method_type method;
   Object idobj=NULL;
   int iprint[MAXPRINT],msglen=0,shape[1];
   int refresh = 0;

   /* check required parameters existence */
    if (!in[1] && (!in[4] || !in[5]))
       req_param=0;
    ei.msgbuf=NULL;
    ival = NULL;

    /* get id */
    if (!DXExtractString(in[0], &id)){
        DXSetError(ERROR_MISSING_DATA,"#10000","id");
        goto error;
    }
    idobj=in[0];

   ival = NULL;
   /* read in object if changed get statistics then check for user input
    * if no change in min and max change=0
    */

   change1 = _dxfcheck_obj_cache(in[1],id,0,idobj);
   change4 = _dxfcheck_obj_cache(in[4],id,4,idobj);
   change5 = _dxfcheck_obj_cache(in[5],id,5,idobj);

   if (change1 > 0 || change4 > 0 || change5 > 0 ){
      change=1;
      /* DXWarning("getting min max, change=%d",change); */
      if (in[1]){
         if (!DXStatistics(in[1], "data", &min, &max, NULL, NULL))
            return ERROR;
      }
      if (in[4]){
         if (!DXExtractInteger(in[4], &imin)){
            DXSetError(ERROR_BAD_PARAMETER,"#10080","minimum");
            goto error;
         }
         min=imin;
      }
      else if (!in[1])
         min=-1000000; 
      if (in[5]){
         if (!DXExtractInteger(in[5], &imax)){
            DXSetError(ERROR_BAD_PARAMETER,"#10080","maximum");
            goto error;
         }
         max=imax;
      }
      else if (!in[1])
         max=1000000;
      /* Print warning if max<min */
      if (max < min){
         DXSetError(ERROR_BAD_PARAMETER,"#10150","minimum",max);
         goto error;
      }
      if (in[1] && (in[4] || in[5]))
         DXWarning("the min and/or max values have been specified; these override the min/max of the data");
   }
   else{
      min=DXD_MAX_FLOAT;
      max=-DXD_MAX_FLOAT;
   }

   /* read increment(default 1/100), and label
    * just pass through to message 
    */
   if (in[7]){
      if (!DXExtractString(in[7],&incrmethod)){
         DXSetError(ERROR_BAD_PARAMETER,"#10200","delta method");
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
      if (!DXExtractFloat(in[6], &incr) || incr < 0.0){
        DXSetError(ERROR_BAD_PARAMETER,"#10020","delta");
        goto error1;
      }
      if ((method==PERCENT || method==PERCENT_ROUND) &&
           (incr > 1.0 || incr < 0.0)){
        DXSetError(ERROR_BAD_PARAMETER,"#10110","delta",0,1);
        goto error1;
      }
   }
   else{
      if (in[7] && method==ABSOLUTE) incr=1.0;
      else incr=.01;
   }

   if (in[10]){
      if (!DXExtractString(in[10], &label)){
        DXSetError(ERROR_BAD_PARAMETER,"#10200","label");
        goto error1;
      }
   }
   else
      label="";
  
   /* decimal = DXD_MAX_INT; */

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

   if (min!=DXD_MAX_FLOAT) min = ceil((double)min);
   if (max!=-DXD_MAX_FLOAT) max = floor((double)max);
   /* for case when range of data is less than 1 set min=max */
   if (min!=DXD_MAX_FLOAT && max!=-DXD_MAX_FLOAT && min > max) min = max;
   
   /* cache attributes */
   if (!_dxfinteract_float(id,idobj,&min,&max,&incr,\
                           NULL,label,0,method,&nitem,iprint))
        return ERROR;

   /* if refresh is set force a reevaluation based on current data */
   if (refresh == 1){
         if (nitem != -1) item=nitem;
         ival = (int *)DXAllocate(sizeof(int) *item);
         int_reset(min,max,item,start,ival);
         iprint[4]=1;
   }
   else {
   if (islist) changen=iprint[6];
   if (iprint[0]==1 || iprint[1]==1) change=1;
   else change=0;
   /* read in integer parameter */
   if (in[2]){
      if (!DXQueryParameter(in[2],TYPE_INT,1,&item)){
         DXSetError(ERROR_BAD_PARAMETER,"#10050","value(s)");
         goto error1; 
      }
      if (req_param==1 && changen==1){
         /* number of list items changes, reset */
         if (nitem != -1) item=nitem;
         ival = (int *)DXAllocate(sizeof(int) *item);
         int_reset(min,max,item,start,ival);
         iprint[4]=1;
      }
      else if (change == 1){
         /* check for outofrange */
         ival = (int *)DXAllocate(sizeof(int) *item);
         if (!DXExtractParameter(in[2],TYPE_INT,1,item,(Pointer)ival)){
            DXSetError(ERROR_INTERNAL,"can't extract current value");
            goto error1;
         }
         if (req_param==1){
            for (i=0; i<item; i++){
               val = ival[i];
               range = OUTOFRANGE(val,min,max);
               if (range==1){
	          if (nitem!=-1) item=nitem;
		  DXFree((Pointer)ival);
		  ival = (int *)DXAllocate(sizeof(int) *item);
                  if (!int_reset(min,max,item,start,ival))
                     goto error;
                  iprint[4]=1;
                  break;
               }
            }
         }
         else{
               for (i=0; i<item; i++){
                  val=ival[i];
                  valtemp=val;
                  if (in[4]) valtemp = CLAMPMIN(val,min);
                  if (in[5]) valtemp = CLAMPMAX(val,max);
                  if (val != valtemp)
                     iprint[4]=1;
                  ival[i] = valtemp;
               }
         }
      }
   }
   else if (req_param==1){
      /* first time */
      if (nitem != -1) item = nitem;
      ival = (int *)DXAllocate(sizeof(int) *item);
      int_reset(min,max,item,start,ival);
      iprint[4]=1;
   }
   else{ 
     item=1;
     ival = (int *)DXAllocate(sizeof(int));
     val = 0;
     valtemp=val;
     if (in[4]) valtemp = CLAMPMIN(val,min);
     if (in[5]) valtemp = CLAMPMAX(val,max);
     ival[0] = valtemp;
     iprint[4]=1;
   } 
   }

   if (incr<1 && incr>0) {incr=1; /* decimal=0; */ }
   iincr=incr;
   imin = min;
   imax = max; 

   /* if req_params don't exist print out what is specified */
   if (req_param==0){
      if (iprint[0]==1 && !in[4]) iprint[0]=0;
      if (iprint[1]==1 && !in[5]) iprint[1]=0;
      if (iprint[2]==1 && !in[6]) iprint[2]=0;
   }

   /* calculate the size of the UI message */
   if (iprint[4] == 1) msglen += (NUMBER_CHARS*item);
   if (iprint[5] == 1) msglen += strlen(label);

   for (i=0; i<MAXPRINT; i++){
      if (iprint[i]==1)
	 msglen += (NUMBER_CHARS + NAME_CHARS);
   }
   if (in[7])
      msglen += (METHOD_CHARS + NAME_CHARS);
   if (!islist && in[9])
      msglen = msglen + NAME_CHARS + METHOD_CHARS;

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
   if (iprint[0]==1){
     sprintf(ei.mp, "min="); while(*ei.mp) ei.mp++;
     if (!_dxfprint_message(&imin, &ei, TYPE_INT, 0,shape,1))
       goto error1;
   }
   if (iprint[1]==1){
     sprintf(ei.mp, "max="); while(*ei.mp) ei.mp++;
     if (!_dxfprint_message(&imax, &ei, TYPE_INT, 0,shape,1))
        goto error1;
   }
   if (iprint[2]==1){
     sprintf(ei.mp, "delta="); while(*ei.mp) ei.mp++;
     if (!_dxfprint_message(&iincr, &ei, TYPE_INT,0,shape,1))
        goto error1;
   }
   if (iprint[4]==1){
     if (islist){
         sprintf(ei.mp, "list="); while(*ei.mp) ei.mp++;
         if (item==1){sprintf(ei.mp,"{"); while(*ei.mp) ei.mp++;}
     }
     else{ sprintf(ei.mp, "value="); while(*ei.mp) ei.mp++;}
     if (!_dxfprint_message(ival, &ei, TYPE_INT,0,shape,item))
        goto error1;
     if (islist && item==1){sprintf(ei.mp,"}"); while(*ei.mp) ei.mp++;}
   }
   if (iprint[5]==1 && strlen(label)>0){
      shape[0] = (int)strlen(label);
      sprintf(ei.mp, "label="); while(*ei.mp) ei.mp++;
      if (!_dxfprint_message(label, &ei, TYPE_STRING,1,shape,1))
         goto error1;
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

   if (strlen(ei.msgbuf) > MAX_MSGLEN){
    DXSetError(ERROR_DATA_INVALID,"#10920");
    goto error1;
   }
   if (strcmp(ei.msgbuf,""))
   DXUIMessage(id,ei.msgbuf);

   /* create array for output object */
   /* if no items output NULL to be consistent with non-DD output 
    * of empty list */
   if (iprint[4]==1){
      if (item==0) out[0] = NULL;
      else{
         out[0] = (Object)DXNewArray(TYPE_INT, CATEGORY_REAL, 0);
         if (!out[0] || !DXAddArrayData((Array)out[0], 0, item, (Pointer)ival))
            return ERROR;
      }
   }
   else
      out[0]=in[2];

   if (ival) DXFree((Pointer)ival);
   DXFree(ei.msgbuf);

   return OK;

error:
error1:
   /* change cache of data object so min and max will be cached */
   change1 = _dxfcheck_obj_cache(in[0],id,0,idobj);
   if (ei.msgbuf) DXFree((Pointer)ei.msgbuf);
   if (ival) DXFree(ival);
   return ERROR;

}
 
static
Error int_reset(float min, float max, int item, start_type start, int *ivalue)
{
   int i;
   
   if (item==1){
      if(start==START_MINIMUM)
          ivalue[0] = min;
      else if(start==START_MIDPOINT)
          ivalue[0] = (max+min)/2.0;
      else
          ivalue[0] = max;
      return OK;
   }
   for (i=0; i<item; i++)
     ivalue[i] = i*((max-min)/(item-1)) +min;
   return OK;
} 
 
