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
#include "separate.h"
#include "plot.h"

static Error _dxfgetdelta(method_type,float ,float ,float *);
static Error getdec(float ,float ,float ,int *);

Error _dxfinteract_float(char *id, Object idobj, float *min, float *max,
           float *incr, int *decimal, char *label, int pos,
	   method_type method, int *items, int iprint[])
{
   int i;
   Array temp;
   float *old;

   Private p;
   char *cache_label = NULL;
   char *old_label;

   for (i=0; i<MAXPRINT; i++)
     iprint[i]=0;

   cache_label = (char *)DXAllocate(strlen(id) + 32);
   if (! cache_label)
       goto error;

   /*  set cache entry for min,max,inc,decimal */
   strcpy(cache_label,id);
   strcat(cache_label,"_array");

   temp = (Array)DXGetCacheEntry(cache_label,pos,1,idobj);
   if (!temp)
   {
      temp =DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, MAXPRINT-2);
        old = (float *)DXGetArrayData(temp);
      
      old[0] = DXD_MAX_FLOAT; 
      old[1] = -DXD_MAX_FLOAT;
      old[2] = DXD_MAX_FLOAT;
      old[3] = -1;
      old[4] = -1;

      if (incr && method!=ABSOLUTE){
         if (!_dxfgetdelta(method,*min,*max,incr))
            goto error;
      }
      if (decimal && *decimal==DXD_MAX_INT){
         if (!getdec(*min,*max,*incr,decimal))
            goto error;
      } 


      if(temp){
        DXReference((Object)temp);
        if (!DXSetCacheEntry((Object)temp,CACHE_PERMANENT,cache_label,pos,1,\
                              idobj))
           DXErrorGoto(ERROR_INTERNAL,"can't set cache");
      }
   }
   else{

        old = (float *)DXGetArrayData(temp);
        DXDelete((Object)temp);
        if (*min==DXD_MAX_FLOAT) *min=old[0];
        if (*max==-DXD_MAX_FLOAT) *max=old[1];
        if (incr && method!=ABSOLUTE){
           if (!_dxfgetdelta(method,*min,*max,incr))
              goto error;
        }
        if (decimal && *decimal==DXD_MAX_INT){
           if (!getdec(*min,*max,*incr,decimal))
              goto error;
        } 
        /* DXWarning("min,max,incr,%g,%g,%g",*min,*max,*incr);*/
   }

   /* set cache for label */
   strcpy(cache_label,id);
   strcat(cache_label,"_label");
    
   p = (Private)DXGetCacheEntry(cache_label,0,1,idobj);
   if (!p){
      old_label = (char *)DXAllocate(41); 
      if (!old_label)
          goto error;
      iprint[5]=1;
      strcpy(old_label,"j");
      p = DXNewPrivate(old_label,NULL);
      strncpy(DXGetPrivateData(p),label,40);
      if (p){
         DXReference((Object)p);
         if (!DXSetCacheEntry((Object)p,CACHE_PERMANENT,cache_label,0,1,idobj))
            DXErrorGoto(ERROR_INTERNAL,"can't set cache");
      }
   }
 
   else{

      if (strcmp(label,DXGetPrivateData(p))){
        iprint[5]=1;
        strncpy(DXGetPrivateData(p),label,40);

      }
   }



   /* Print out the message
    * min,max,incr,decimal if different than cached values
    */

   if (old[0]!=*min)
     iprint[0]=1;
   if (old[1]!=*max)
     iprint[1]=1;
   if (incr && old[2]!=*incr)
     iprint[2]=1;
   if (decimal && old[3]!=*decimal)
     iprint[3]=1;
   if (items && old[4]!=*items)
     iprint[6]=1;    
  
   old[0] = *min;
   old[1] = *max;
   if (incr)
      old[2] = *incr;
   else
      old[2] = 0;
   if (decimal)
      old[3] = *decimal;
   else
      old[3] = 0;
   if (items)
      old[4] = *items;
   else
      old[4] = 0;
   if (!DXAddArrayData(temp, 0,MAXPRINT-2,old))
       DXErrorGoto(ERROR_INTERNAL,"can't get array data");

   DXFree(cache_label);
   return OK;

error:
   DXFree(cache_label);
   return ERROR;
}

Error _dxfnewvalue(float *value, float old_min, float old_max,
                   float min, float max, int remap,int *ip)
{
   float valtemp;

   if (old_min==DXD_MAX_FLOAT) remap=0;

   if (remap==1){
      if (*value == old_min)
	  valtemp = min;
      else if (*value == old_max)
	  valtemp = max;
      else if ((old_max-old_min)!=0)
          valtemp=(max-min)*(1.0-((old_max-*value)/(old_max-old_min))) +min;
      else
        valtemp = min;
   }
   else
     valtemp = CLAMP(*value,min,max);

   if (*value!=valtemp) *ip=1;
   *value = valtemp;
 
   return OK;
}

Error _dxfprint_message(Pointer dp, struct einfo *ep, Type type, int rank,
        int *dim, int item)
{
   int i,j,k;
   int rank1,offset=0;
   char c;

   if (rank == 0) rank1 = 1;  /* if scalar set count to 1 */
   else rank1 = rank;

   if (item>1) {sprintf(ep->mp,"{"); ep->mp++;}
   for (i=0; i<item; i++){
     if (rank>=1){ 
      if (type==TYPE_STRING) {sprintf(ep->mp,"\""); ep->mp++;}
      else {sprintf(ep->mp,"["); ep->mp++;}
   }
    for (k=0; k<rank1; k++){
     if (rank > 1) {sprintf(ep->mp,"["); ep->mp++;}
     switch(type){
      case TYPE_INT:
        for (j=0; j<dim[k]; j++){
         sprintf(ep->mp,"%d ",((int *)dp)[offset]); while(*ep->mp) ep->mp++;
	 offset++;
	}
      break;
      case TYPE_FLOAT:
        for (j=0; j<dim[k]; j++){
         sprintf(ep->mp,"%.8g ",((float *)dp)[offset]);while(*ep->mp) ep->mp++;
	 offset++;
	}
      break;
      case TYPE_STRING:
        if ((dim[k] + ep->mp - ep->msgbuf) >= ep->maxlen) {
	  DXSetError(ERROR_INTERNAL,"too many items in list");
          return ERROR;
	}
        for (j=0; j<dim[k]; j++){
	   c = ((char *)dp)[offset];
           switch(c) {
	      case '%':  sprintf(ep->mp,"%%%%");   break; /* the printf conversion char */
#ifndef DXD_NO_ANSI_ALERT
	      case '\a': sprintf(ep->mp,"\\a");  break; /* attention (bell) */
#endif
	      case '\b': sprintf(ep->mp,"\\b");  break; /* backspace */
	      case '\f': sprintf(ep->mp,"\\f");  break; /* formfeed */
	      case '\n': sprintf(ep->mp,"\\n");  break; /* newline */
	      case '\r': sprintf(ep->mp,"\\r");  break; /* carriage return */
	      case '\t': sprintf(ep->mp,"\\t");  break; /* horizontal tab */
	      case '\v': sprintf(ep->mp,"\\v");  break; /* vertical tab */
	      case '\\': sprintf(ep->mp,"\\\\"); break; /* backslash */
	      case '"':  sprintf(ep->mp,"\\\""); break; /* double quote */
	      /* case '\0': sprintf(ep->mp,"\\0");  break;  NULL */
	      default:
		 sprintf(ep->mp,"%c",c);
	   }
	   while(*ep->mp) ep->mp++;
	   offset++;
	}
      break;
      default:
         DXErrorReturn(ERROR_BAD_TYPE,"unsupported type");
     }
     if (rank > 1) {sprintf(ep->mp,"]"); ep->mp++;}
    }

    if (rank>=1){ 
     if (type==TYPE_STRING) {sprintf(ep->mp,"\""); ep->mp++;}
     else {sprintf(ep->mp,"]"); ep->mp++;}
    }
    sprintf(ep->mp," "); ep->mp++;
    if (EndCheck(ep))
       return ERROR;
  }
  if (item>1) {sprintf(ep->mp,"}"); ep->mp++;}

     return OK;
}
 
int _dxfcheck_obj_cache(Object obj,char *id,int key,Object idobj)
{
   Object temp_obj;
   int remap=0;
   char *cache_label = NULL;

   cache_label = (char *)DXAllocate(strlen(id) + 32);
   if (! cache_label)
       goto error;

   strcpy(cache_label,id);
   strcat(cache_label,"_object");

   temp_obj = DXGetCacheEntry(cache_label,key,1,idobj);
   if (!temp_obj){
      if (obj) remap=2;
      /* DXWarning("object is new, %d,exist=%d",key,remap); */
      temp_obj=obj;
      DXSetCacheEntry(temp_obj,CACHE_PERMANENT,cache_label,key,1,idobj);
   }
   else{
      if (obj!=temp_obj){
         /* DXWarning("group object has changed,%d",key); */
         remap=1;
	 DXDelete(temp_obj);
         temp_obj=obj;
         /* DXReference(temp_obj); */
         DXSetCacheEntry(temp_obj,CACHE_PERMANENT,cache_label,key,1,idobj);
      }
      else
       DXDelete(temp_obj);
   }
   /* DXDelete(temp_obj); */

   DXFree(cache_label);
   return remap;

error:
   DXFree(cache_label);
   return 0;
}

int EndCheck(struct einfo *ep)
{
    if (ep->atend)
        return 1;

    if ((ep->mp - ep->msgbuf) >= ep->maxlen) {
        ep->atend = 1;
        DXSetError(ERROR_DATA_INVALID,"msg too long");
        return 1;
    }

    return 0;

}

static
Error _dxfgetdelta(method_type method,float min, float max, float *incr)
{
   int n=100,sub=0;
   float delta=0,lo=0,fuzz;
   char fmt[20];
   float range,start=0;

   switch(method){
    case(PERCENT_ROUND):
      if (*incr>0){
         range = max-min;
         fuzz = *incr * .1;
         n = (int)(1.0/ (*incr-fuzz));
         _dxfaxes_delta(&start,&range,1,&n,&lo,&delta,fmt,&sub,0,0);
         *incr=delta;
      }
      else
         *incr=0;
      break;
    case(PERCENT):
      delta = *incr *(max-min);
      *incr = delta;
      break;
    default:
      break;
   }

   return OK;
}

static
Error getdec(float min,float max,float incr, int *dec)
{
   float s;

   s = max-min;
   /* always scientific notation */
   if (s>1000000.0 || s<0.000001){
     if (incr>0)
        *dec =(int)(log10(s) +0.99999)-(int)(log10(incr) +0.99999);
     else{
        if (s==0.0)
           *dec=0;
        else
           *dec = (int)log10(s);
     }
   }
   else if (incr > 0.0 && incr < 1.0)
     *dec =(int)( -log10(incr)+0.99999);
   else if (incr == 0)
     *dec = 5;
   else
     *dec = 0;

   if (*dec > 6) *dec = 6; 
   return OK;
} 
     
