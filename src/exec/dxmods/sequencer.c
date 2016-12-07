/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#include "interact.h"
#include "../dpexec/vcr.h"
#include "../dpexec/_variable.h"

/*
 * Input indices
 */
#define I_ID	0
#define I_DATA	1	/* Not used yet */
#define I_FRAME	2	/* it is a rerouted input */
#define I_MIN	3
#define I_MAX	4
#define I_INCR  5
#define I_UIVAR 6
#define NUM_UIVAR 6
#define MAXPR_SEQ 4   	/* number of possible print statement */

/*
 * Output indices
 */
#define O_FRAME	0

int m_Sequencer(Object *in, Object *out)
{
   struct einfo ei;
   char *id;
   int min,max,incr;
   int next,frame;
   int start,end;
   int reset=1,uivar[NUM_UIVAR];
   int iprint[MAXPR_SEQ]={1,1,1,1}, msglen=0,shape[1];

   /* get id */
   if (!DXExtractString(in[I_ID], &id)){
       DXSetError(ERROR_BAD_PARAMETER,"#10000","id");
       goto error;
   }
   /*idobj=in[I_ID];*/
   ei.msgbuf=NULL;

   /* get uimin and max delta */
   if (in[I_UIVAR]){
      if (!DXExtractParameter(in[I_UIVAR],TYPE_INT,0,NUM_UIVAR,&uivar)){
         DXSetError(ERROR_INTERNAL,"uivariables missing");
         goto error;
      }
   }

   /* set min and max to uimin,uimax for default 
    * default for start and end are min and max
    * default for incr is 1
    */
   if (in[I_MIN]){ 
      if (!DXExtractInteger(in[I_MIN],&min) || min < -16383 || min > 16383){
         DXSetError(ERROR_BAD_PARAMETER,"#10040","minimum",-16383,16383);
         goto error;
      }
   }
   else{
      min=uivar[0];
      iprint[0]=0;
   }
   if (in[I_MAX]){
      if (!DXExtractInteger(in[I_MAX],&max) || max < -16383 || max > 16383){
         DXSetError(ERROR_BAD_PARAMETER,"#10040","maximum",-16383,16383);
         goto error;
      }
   }
   else{
      max=uivar[1];
      iprint[1]=0;
   }
   if (max<min){
      if (!in[I_MIN] && in[I_MAX]) min=max;
      else if (in[I_MIN] && !in[I_MAX]) max=min;
      else{
         DXSetError(ERROR_BAD_PARAMETER,"max must be greater than min");
         goto error;
      }
   }

   /* check uimin, uimax if same as min,max use frame
      else start at min */
   if (uivar[5]==1) reset=0;
   if (uivar[0] != min || uivar[1] != max) reset=1;
   start = uivar[3];
   end = uivar[4];
  
   /* if min or max exist, start=min, end=max
    * else make sure uistart and uiend are within new [min,max] range
    */
   if (reset==1){
      if (in[I_MIN]) start = min;
      else if (start < min || start > max) start = min;
      if (in[I_MAX]) end = max;
      else if (end < min || end > max ) end = max;
   }
   else{
      iprint[0]=0;
      iprint[1]=0;
   }

   if (in[I_INCR]){
      if (!DXExtractInteger(in[I_INCR],&incr) || incr<=0 ||
           (incr >(end-start) && end!=start)){
         DXSetError(ERROR_BAD_PARAMETER,"#10040","increment",0,(end-start+1));
         goto error;
      }
   }
   else{
      incr=uivar[2];
      iprint[2]=0;
   }

   if (in[I_FRAME]){
      /* check uimin, uimax if same as min,max use frame 
         else start at min */
      if (reset==0){
         if (!DXExtractInteger(in[I_FRAME],&frame)){
            DXSetError(ERROR_BAD_PARAMETER,"#10010","frame");
            goto error;
         }
	 /* for case when program was saved with frame=current
	  * and when read in and execute, want the same current frame value
	  * but @frame already advanced one */
	 /* frame-=1; */
         if (frame < start) frame=start;
         if (frame > end) frame=start;
      }
      else
         frame=start;
   }
   else{
     DXSetError(ERROR_INTERNAL,"frame not set");
     goto error;
   }

   /* send message to executive to set @'s */
   if(!DXSetIntVariable("@startframe", start, 1, 0))
      return ERROR;
   if(!DXSetIntVariable("@frame", frame, 1, 0))
      return ERROR;
   if(!DXSetIntVariable("@endframe", end, 1, 0))
      return ERROR;

   if(!DXSetIntVariable("@deltaframe", incr, 1, 0))
      return ERROR;
   next = frame+incr; 
   if (next > end) next = min;
   else if (next < start) next = min;
   if(!DXSetIntVariable("@nextframe", next, 1, 0))
      return ERROR;

   _dxf_ExVCRRedo(); 

   /* set output frame */
   out[O_FRAME] = (Object)DXNewArray(TYPE_INT, CATEGORY_REAL, 0);
   if (!out[O_FRAME] || 
       !DXAddArrayData((Array)out[O_FRAME], 0, 1, (Pointer)&frame))
        return ERROR;

   /* calculate the size of the UI message */
   /* since only four numbers can be printed will just use a constant number
    * rather than calculate this */
   msglen = MAXPR_SEQ * (NAME_CHARS + NUMBER_CHARS);

   /* send message to ui
    *  next=start+delta
    * these are calculated in UI
     */
   ei.maxlen = msglen;
   ei.atend = 0;
   ei.msgbuf = (char *)DXAllocateZero(ei.maxlen+SLOP);
   if (!ei.msgbuf)
      return ERROR;
   ei.mp =ei.msgbuf;

   shape[0]=1;
   strcpy(ei.mp,"");
   if (iprint[0]==1){
      sprintf(ei.mp, "min="); while(*ei.mp) ei.mp++;
      if (!_dxfprint_message(&min, &ei, TYPE_INT, 0,shape,1))
         goto error;
   }
   if (iprint[1]==1){
      sprintf(ei.mp, "max="); while(*ei.mp) ei.mp++;
      if (!_dxfprint_message(&max, &ei, TYPE_INT, 0,shape,1))
         goto error;
   }
   if (iprint[2]==1){
      sprintf(ei.mp, "delta="); while(*ei.mp) ei.mp++;
      if (!_dxfprint_message(&incr, &ei, TYPE_INT,0,shape,1))
         goto error;
   }
   sprintf(ei.mp, "frame="); while(*ei.mp) ei.mp++;
   if (!_dxfprint_message(&frame, &ei, TYPE_INT, 0,shape,1))
      goto error;

   if (strcmp(ei.msgbuf,""))
   DXUIMessage(id,ei.msgbuf);
   DXFree(ei.msgbuf);

   return OK;

error:
   if (ei.msgbuf) DXFree(ei.msgbuf);
   return ERROR;

} 
