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

#define MAXRANK 16
#define MSGLEN 240

static int readvalue(Object,char *,char *,Object *);

#if 0
static int compare_arrays(Array *a, Array *b,int items,int dim,Type type);
static Error read_and_compare(Object cvalue,Object svalue,Object uvalue,int *on,          Object *out); 
#endif

int m_Toggle(Object *in, Object *out)
{
   struct einfo ei;
   char *id, *label;
   Object idobj;
   char *set="set= ",*unset="unset= ";
   int shape[MAXRANK];
   int toggle,change;

   ei.msgbuf = NULL;
   out[0] = in[1];

   /* get id */
   if (!DXExtractString(in[0], &id)){
       DXSetError(ERROR_BAD_PARAMETER,"#10000","id");
       goto error;
   }
   idobj=in[0];

   /* read toggle value (set=1, unset=0) */
   if (!in[2])
      toggle=0;
   else{
      if (!DXExtractInteger(in[2],&toggle)){
	 DXSetError(ERROR_INTERNAL,"toggle value is not integer");
	 goto error;
      }
   }
   
   /* read set value */
   change = _dxfcheck_obj_cache(in[3],id,3,idobj);
   if (change==0) set=NULL;
   if (in[3]){	/* set value is data-driven */
      if (toggle==1){
	 if (!readvalue(in[3],id,set,out))
            goto error;
      }
      else{
	 if (!readvalue(in[3],id,set,NULL))
	    goto error;
      }
   }

   /* read unset value */
   change = _dxfcheck_obj_cache(in[4],id,4,idobj);
   if (change==0) unset=NULL;
   if (in[4]){
      if (toggle==0){
	 if (!readvalue(in[4],id,unset,out))
            goto error;
      }
      else{
	 if (!readvalue(in[4],id,unset,NULL))
	    goto error;
      }
   }

   /* read label */
   if (in[5]){
      if (!DXExtractString(in[5], &label)){
         DXSetError(ERROR_BAD_PARAMETER,"#10200","label");
         goto error;
      }
      ei.maxlen = (int)strlen(label)+SLOP;
      ei.msgbuf = (char *)DXAllocateZero(ei.maxlen);
      ei.atend = 0;
      if (!ei.msgbuf)
         goto error;
      ei.mp =ei.msgbuf;
      shape[0] = (int)strlen(label);
      sprintf(ei.mp, "label="); while(*ei.mp) ei.mp++;
      if (!_dxfprint_message(label,&ei,TYPE_STRING,1,shape,1))
         goto error;
      DXUIMessage(id,ei.msgbuf);
      DXFree(ei.msgbuf);
   }


   return OK;

error:
   if (out[0]) out[0]=NULL;
   if (ei.msgbuf) DXFree(ei.msgbuf);
   return ERROR;
}


/* routine for reading, and optionally sending UIMessage or setting output */
int readvalue(Object o,char *id,char *value,Object *out)
{
   struct einfo ei;
   char *set_string;
   int items,shape[MAXRANK],rank;
   Type type;
   Category cat;
   void *p_set;
      
   /* setup message */
   ei.maxlen = MSGLEN;
   ei.atend = 0;
   if (value) ei.msgbuf = (char *)DXAllocateZero(ei.maxlen+SLOP);
   if (!ei.msgbuf)
      goto error;
   ei.mp =ei.msgbuf;

      if (!DXExtractString((Object)o,&set_string)){
         if (!DXGetArrayInfo((Array)o,&items,&type,&cat,&rank,shape))
            return ERROR;
         p_set = DXGetArrayData((Array)o); 
         if (rank==0) shape[0]=1;
         if (value){
            sprintf(ei.mp, "%s",value); while(*ei.mp) ei.mp++;
	    if (!_dxfprint_message(p_set,&ei,type,rank,shape,items))
	       goto error;
	 }
	 if (out){
            out[0] = (Object)DXNewArrayV(type,CATEGORY_REAL,rank,shape);
            if(!out[0] || !DXAddArrayData((Array)out[0],0,items,(Pointer)p_set))
	       goto error;
	 }
      }
      else{
         shape[0] = (int)strlen(set_string);
         if (value){
            sprintf(ei.mp,"%s",value); while(*ei.mp) ei.mp++;
	    if (!_dxfprint_message(set_string,&ei,TYPE_STRING,1,shape,1))
               goto error;
	 }
	 if (out) out[0] = (Object)DXNewString(set_string);
      }

   if (value) {
      DXUIMessage(id,ei.msgbuf);
      DXFree(ei.msgbuf);
   }
   return OK;

error:
      return ERROR;
}

#if 0
static Error
read_and_compare(Object cvalue,Object svalue,Object uvalue,int *on,Object *out) 
{
   char *cstring,*sstring;
   void *p_set,*p_current;
   int items,i,shape[MAXRANK],sp[MAXRANK],rank,r;
   int n,dim;
   Type type,t;
   Category cat,c;

      if (!DXExtractString((Object)cvalue,&cstring)){
         if (!DXGetArrayInfo((Array)cvalue,&items,&type,&cat,&rank,shape))
            return ERROR;
         p_current = DXGetArrayData((Array)cvalue); 
	 /* compare current array with previous set array */
	 if (DXGetArrayInfo((Array)svalue,&i,&t,&c,&r,sp)){
	    if (items==i && type==t && cat==c && rank==r ){
	       if (rank==0) dim=1;
	       else dim=sp[0];
	       *on = compare_arrays((Array)cvalue,(Array)svalue,items,dim,type);
            }
	 }
	 else
	    *on=0; 	/* toggle is unset */

         if (out){
            out[0] = (Object)DXNewArrayV(type,CATEGORY_REAL,rank,shape);
            if(!out[0] || !DXAddArrayData((Array)out[0],0,items,(Pointer)p_current))
               goto error;
         }
      }
      else{
	 if (DXExtractString((Object)svalue,&sstring)){
	       if(!strcmp(cstring,sstring)) 
		  *on=1;
	       else 
		  *on=0;
	       if (out) out[0] = (Object)DXNewString(cstring);
	 }
	 else
	    *on=0;
      }
      return OK;

error:
      return ERROR;
}


int
compare_arrays(Array *a, Array *b,int items,int dim,Type type)
{
   int n,count=0;
   int *a_int,*b_int;
   float *a_f,*b_f;
      
      switch(type){
	 case(TYPE_INT):
	       a_int = (int *)DXGetArrayData((Array)a);
	       b_int = (int *)DXGetArrayData((Array)b);
	       for (n=0; n<items*dim; n++){
	          if (a_int[n] == b_int[n])
	             count++;
	       }
	       break;
	 case(TYPE_FLOAT):
	       a_f = (float *)DXGetArrayData((Array)a);
	       b_f = (float *)DXGetArrayData((Array)b);
	       for (n=0; n<items*dim; n++){
	          if (a_f[n] == b_f[n])
	          count++;
	       }
	       break;
	 default:
	       return(-1);
	 }
	 if (count==items*dim)   	/* arrays are the same */
	    return (1);
	 else
	     return(0);

}
#endif
