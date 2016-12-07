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

#include <stddef.h>
#include "interact.h" 
#include "separate.h"

#define MAXNAME 20
#define MAXRANK 16

static Object membernames(Object);
static Object fillnull(Object *,Object );
static Error cullout(Object *,Object *);
static int ismatch(Object , char *);
/*
 * SelectorList support added 3/3/95 - dawood
 */
#define SELECTOR_LIST_SUPPORT
#if !defined(SELECTOR_LIST_SUPPORT)
static Error dolist(Object , Object *, int ,struct einfo *, int);
#else
static int getmatches(Object , Object, int**);
static Error dolist(Object , Object *, int ,struct einfo *, int*, int );
#endif
static Object zerobase(int ,int);
static int getsize(Object);
static int selector_driver(Object *in, Object *out, int allowLists);

int
m_SelectorList(Object *in, Object *out)
{
   return selector_driver(in,out,1);
}
int
m_Selector(Object *in, Object *out)
{
   return selector_driver(in,out,0);
}


static int
selector_driver(Object *in, Object *out, int allowLists)
{
   struct einfo ei;
   int i,index;
   char *id, *label;
   Object vallist,strlist,newlist;
   char *cstring;
   int item1,item2,icull,startlist,msglen=0,shape[MAXRANK];
   Type type1;
   int needfree=0;
#if defined(SELECTOR_LIST_SUPPORT) 
   int match_count = 0, *matched_indices = NULL;
#endif

   ei.msgbuf = NULL;
   strlist = NULL;
   vallist = NULL;
   out[0] = NULL;
   out[1] = NULL;
   /* get id */
   if (!DXExtractString(in[0], &id)){
       DXSetError(ERROR_BAD_PARAMETER,"#10000","id");
       return ERROR;
   }
   /*idobj = in[0];*/

   /* read current string */
   if (in[1]){
      if (!DXExtractNthString(in[1],0,&cstring))
         DXErrorGoto(ERROR_INTERNAL,
			"current value must be string or string list");
   }
   else
      index=0;

   /* read cull parameter */
   if (in[5]){
      if (!DXExtractInteger(in[5],&icull) || icull>1 || icull<0){
         DXSetError(ERROR_BAD_PARAMETER,"#10070","cull");
         return ERROR;
      }
   }
   else
      icull=0;

   /* read label */
   if (in[6]){
      if (!DXExtractString(in[6], &label)){
         DXSetError(ERROR_BAD_PARAMETER,"#10200","label");
         goto error;
      }
   }

   /* if value list is integer create valuelist based on that integer
    * if list use that list
    * if NULL zerobased list */
   if(in[4]){
      if (!DXExtractInteger(in[4],&startlist)){
         if (!DXGetArrayInfo((Array)in[4],&item2,NULL,NULL,NULL,NULL))
            return ERROR;
      }
      else
         item2=0;
   }
   else{
      item2=0;
      startlist=0;
   }

   /* read data (must be group or stringlist) */
   if (in[3]){
     /* if group call membernames to get stringlist */
     strlist=membernames(in[3]);
     if (!strlist) 
        return ERROR;

     if (!DXGetArrayInfo((Array)strlist,&item1,&type1,NULL,NULL,NULL))
        goto error;
     if (type1!=TYPE_STRING){
        DXSetError(ERROR_BAD_PARAMETER,"#10193","object");
        goto error;
     }

     if (item2!=0 && item2!=item1)
        DXWarning("string list and value list not same length");

     /* get valuelist */
     if (item2==0){
        vallist = zerobase(item1,startlist);
        if (!vallist)
           return ERROR;
	needfree = 1;
     }
     else
        vallist=in[4];

     /* if cull is 0 fill in NULL strings with proper name 
      * otherwise cull out NULL strings but keep indexing same */ 
     if (icull==0){
        newlist=fillnull(&strlist,in[3]);
        if (!newlist)
           goto error;
        DXDelete(strlist);
        strlist=newlist;
     }
     else{
        if (!cullout(&strlist,&vallist))
          return ERROR;
	needfree = 1;
     } 
    

#if !defined(SELECTOR_LIST_SUPPORT)
    if (in[1])     
	index=ismatch(strlist,cstring);
    if (item2!=0 && index > item2-1)
	index = 0;
#else
     if (allowLists) {
	if (in[1]) {
	    match_count=getmatches(strlist,in[1],&matched_indices);
	} else {
	    match_count = 1;
	    matched_indices = &index;
	}
     } else {
	if (in[1])     
	    index=ismatch(strlist,cstring);
	if (item2!=0 && index > item2-1)
	    index = 0;
	match_count = 1;
	matched_indices = &index;
    }
#endif

     /* calculate the size of the UI message */
     if ((i = getsize(strlist)) == 0)
	return ERROR;
     msglen += i;
     if ((i = getsize(vallist)) == 0){
	DXSetError(ERROR_DATA_INVALID,"selector list is empty,unset cull");
	goto error;
     }
     msglen += i;
     msglen += NAME_CHARS*4;
     msglen += NUMBER_CHARS;
     if (in[6])
	msglen += (strlen(label) + 2);

     /* setup message */
     ei.maxlen = msglen;
     ei.atend = 0;
     ei.msgbuf = (char *)DXAllocateZero(ei.maxlen+SLOP); 
     if (!ei.msgbuf)
        goto error;
     ei.mp =ei.msgbuf;
  
     sprintf(ei.mp,"string list=");
     while(*ei.mp) ei.mp++;
#if !defined(SELECTOR_LIST_SUPPORT)
     if (!dolist(strlist,out,1,&ei,index))
        goto error;
#else
     if (!dolist(strlist,out,1,&ei,matched_indices,match_count))
        goto error;
#endif
    
     sprintf(ei.mp," value list=");
     while(*ei.mp) ei.mp++;
#if !defined(SELECTOR_LIST_SUPPORT)
     if (!dolist(vallist,out,0,&ei,index))
        goto error;
#else
     if (!dolist(vallist,out,0,&ei,matched_indices,match_count))
        goto error;
#endif

#if !defined(SELECTOR_LIST_SUPPORT)
     sprintf(ei.mp," index=%d",index);
     while(*ei.mp) ei.mp++;
#else
     sprintf(ei.mp," index=");
     while(*ei.mp) ei.mp++;
     if (match_count > 1) {
	 sprintf(ei.mp,"{");
	 while(*ei.mp) ei.mp++;
     }
     for (i=0 ; i<match_count ; i++) {
	 sprintf(ei.mp,"%d ",matched_indices[i]);
	 while(*ei.mp) ei.mp++;
     }
     if (match_count > 1) {
	 sprintf(ei.mp,"}");
	 while(*ei.mp) ei.mp++;
     }
#endif

     DXDelete(strlist);
     if (needfree) DXDelete(vallist);
     strlist = NULL;
     vallist = NULL;
   }
   else {
      if (in[4]){
         DXSetError(ERROR_DATA_INVALID,
         "valuelist is dependent on stringlist so stringlist must be specified");
         return ERROR;
      }
      if (in[5]){
         DXSetError(ERROR_DATA_INVALID,
          "cull is dependent on stringlist so stringlist must be specified");
         return ERROR;
      }
      /* setup message default message */
      msglen = 4*NAME_CHARS + 4*NUMBER_CHARS;
      if (in[6])
	 msglen += strlen(label) + 2;
      ei.maxlen = msglen;
      ei.atend = 0;
      ei.msgbuf = (char *)DXAllocateZero(ei.maxlen+SLOP); 
      if (!ei.msgbuf)
        goto error;
      ei.mp =ei.msgbuf;
      if (in[1]){
	 out[0] = in[2];
	 out[1] = in[1];
      }
      else{
	 /* make output */
	 i=1;
	 out[0] = (Object)DXNewArray(TYPE_INT,CATEGORY_REAL,0);
	 if (!DXAddArrayData((Array)out[0],0,1,&i))
	    goto error;
	 out[1] = (Object)DXNewString("on");
         sprintf(ei.mp,"string list=\"on\" "); while(*ei.mp) ei.mp++;
         sprintf(ei.mp,"value list= 1 "); while(*ei.mp) ei.mp++;
         sprintf(ei.mp,"index= 0"); while(*ei.mp) ei.mp++;
      }
   }

   /* print out label */
   if (in[6]){
      shape[0] = (int)strlen(label);
      sprintf(ei.mp, " label=");  while(*ei.mp) ei.mp++;
      if (!_dxfprint_message(label, &ei,TYPE_STRING,1,shape,1)){
         goto error;
      }
   }

   /* in DXMessage there is static buffer of MAX_MSGLEN so check length */
   if (strlen(ei.msgbuf) > MAX_MSGLEN){
      DXSetError(ERROR_DATA_INVALID,"#10920");
      goto error;
   }

   DXUIMessage(id,ei.msgbuf);
   DXFree(ei.msgbuf);
   if (matched_indices && (matched_indices != &index))
	DXFree(matched_indices);
   return OK;
 
error:
   if (out[0]) out[0] = NULL;
   if (out[1]) out[1] = NULL;
   if (strlist) DXDelete((Object)strlist);
   if (vallist && needfree) DXDelete((Object)vallist);
   if (ei.msgbuf) DXFree(ei.msgbuf);
   if (matched_indices && (matched_indices != &index))
	DXFree(matched_indices);
   return ERROR;

}
 

static Object membernames(Object o)
{
    int i = 0,n=0,maxlen,size;
    char **cp = NULL, *string;
    Array a;
    float position;
    Class class=CLASS_GROUP;

    switch(DXGetObjectClass((Object)o)){

    case CLASS_GROUP:
    class = DXGetGroupClass((Group)o);
    /* error if composite field */
    if (class==CLASS_COMPOSITEFIELD){
       DXSetError(ERROR_BAD_PARAMETER,"#10193","object");
       return NULL;
    }

    if (!DXGetMemberCount((Group)o, &i))
        return NULL;
    cp = (char **)DXAllocate(sizeof(char *) * (i+1));
    if (!cp)
        return NULL;

    if (class==CLASS_SERIES){
       for (i=0; DXGetSeriesMember((Series)o, i, &position); i++){
         cp[i] = (char *)DXAllocate(MAXNAME);
         if (!cp[i])
            goto error;
         sprintf(cp[i],"position=%g",position);
       }
    } 
    else{
      for (i=0; DXGetEnumeratedMember((Group)o, i, &string); i++){
         if (string) {
	    maxlen=strlen(string) + 1;
            cp[i] = (char *)DXAllocate(maxlen);
            if (!cp[i])
               goto error;
            strcpy(cp[i],string);
	 }
         else 
            cp[i]=NULL;
      }
    }

    break;

    case CLASS_ARRAY:
    if (!DXGetArrayInfo((Array)o,&i,NULL,NULL,NULL,&size))
       return NULL;
    cp = (char **)DXAllocate(sizeof(char *) * (i+1));
    if (!cp)
        return NULL;
    
    for (i=0; DXExtractNthString((Object)o, i, &string); i++){
         if (string) {
	    cp[i] = (char *)DXAllocate(sizeof(char) * (size+1));
            if (!cp[i])
               goto error;
            strcpy(cp[i],string);
	 }
	 else
            cp[i]=NULL;
    }

    break;

    case CLASS_STRING:
    cp = (char **)DXAllocate(sizeof(char *));
    if (!cp)
        return NULL;
   
    for (i=0; DXExtractNthString((Object)o, i, &string); i++){
         if (string) {
	    maxlen=strlen(string) +1;
            cp[i] = (char *)DXAllocate(maxlen);
            if (!cp[i])
              goto error;
            strcpy(cp[i],string);
	 }
	 else
            cp[i]=NULL;
    }
     break;
 
    default:
       class = DXGetObjectClass((Object)o);
       DXSetError(ERROR_BAD_PARAMETER,"#10193","object");
       return NULL;       
    }
    
    if (i == 0)
        a = DXMakeStringList(1, " ");
    else
        a = DXMakeStringListV(i, cp);


    n=i; 
    for (i=0; i<n; i++)
       if (cp[i])
	  DXFree((Pointer)cp[i]);

    DXFree((Pointer)cp);
    return (Object)a;

error:
    n=i;
    for (i=0; i<n; i++)
       DXFree((Pointer)cp[i]);

    DXFree((Pointer)cp);
    return NULL;
}


static Object fillnull(Object *list, Object o)
{
    char *string;
    char **cp=NULL;
    Class class;
    int i,item,size,*ifree=NULL;
    Object newlist;

    class = DXGetObjectClass((Object)o);
    /* extract strings */
    if (!DXGetArrayInfo((Array)*list,&item,NULL,NULL,NULL,&size))
       return NULL;
   
    size=12;
    cp = (char **)DXAllocate(sizeof(char *) * (item+1));
    if (!cp)
       return NULL;
    ifree = (int *)DXAllocate(sizeof(int) * item);
    if (!ifree)
       goto error;
     for (i=0; DXExtractNthString(*list, i, &string); i++){
        if (!strcmp("",string)){
           cp[i] = (char *)DXAllocate(sizeof(char) * (size+1));
           if (!cp[i])
              goto error;
           ifree[i]=1;
           if (class==CLASS_GROUP){
              sprintf(cp[i],"member %d",i);
           }
           else{
              sprintf(cp[i]," ");
           }
        }
        else{
          ifree[i]=0;
          *(cp+i)=string;
        }
     }

    if (i == 0)
        newlist = (Object)DXMakeStringList(1, " ");
    else
        newlist = (Object)DXMakeStringListV(i, cp);

    for (i=0; i<item; i++){
       if (ifree[i]==1)
          DXFree((Pointer)cp[i]);
    }

    DXFree((Pointer)ifree);
    DXFree((Pointer)cp);
    return (Object)newlist;

error:
    for (i=0; i<item; i++){
       if (ifree[i]==1)
          DXFree((Pointer)cp[i]);
    }
    if (cp) DXFree((Pointer)cp);
    if (ifree) DXFree((Pointer)ifree);
    return NULL;
}

static Error cullout(Object *slist,Object *vlist)
{
    int i,n=0,item,item1;
    Array newlist;
    void *old, *outval;
    Type type;
    Category cat;
    int rank,bytsize;
    int shape[MAXRANK];
    char **cp, *string;
 
    /* extract strings */
    if (!DXGetArrayInfo((Array)*slist,&item,NULL,NULL,NULL,NULL))
       return ERROR;
    old = DXGetArrayData((Array)*vlist);

    /* create new array to hold new valuelist */
    if (!DXGetArrayInfo((Array)*vlist,&item1,&type,&cat,&rank,shape))
       return ERROR;
    newlist = DXNewArrayV(type,cat,rank,shape);
    bytsize = DXGetItemSize((Array)*vlist);

    cp = (char **)DXAllocate(sizeof(char *) * (item+1));
    if (!cp)
       return ERROR;
    for (i=0; i<item; i++){
       DXExtractNthString(*slist, i, &string);
       if (strcmp("",string)){
         *(cp+n) = string;
         if (i<item1){
            outval = DXAllocate(bytsize);
            memcpy(outval,(void *)((byte *)old +bytsize*i),(size_t)bytsize);
            if (!DXAddArrayData((Array)newlist,n,1,(Pointer)outval)){
	       DXFree((Pointer)outval);
               return ERROR;
	    }
	    DXFree((Pointer)outval);
         }
         n++;
       }
    }


    if (n == 0)
        *slist = (Object)DXMakeStringList(1, " ");
    else
        *slist = (Object)DXMakeStringListV(n, cp);

    DXDelete((Object)*vlist);
    *vlist = (Object)newlist;

    DXFree((Pointer) cp);

    return OK;
}




/*
 * Search string list in list1 for strings in string list list2.
 * Return the number of unique matches found  and a newly allocated
 * array of indices into list1 of strings that were found in list2.
 */
static int getmatches(Object list1, Object list2, int **matched_indices)
{
   int i,j,k,count=0;
   char *string1, *string2;
   int list_size = 0;
   int *matches = NULL;

   for (i=0 ; DXExtractNthString(list1, i, &string1) ; i++) {
     for (j=0; DXExtractNthString(list2, j, &string2) ; j++) {
        if (!strcmp(string1,string2)) {
	    int already_found = 0;
	    for (k=0 ; k<count ; k++) {
		if (matches[k] == i)
		    already_found = 1;
	    }
	    if (!already_found) {
		if (count >= list_size)  {
		    if (list_size == 0) 
			list_size = 8;
		    else
			list_size *= 2;
		    matches = DXReAllocate(matches,sizeof(int) * list_size);
		}
		matches[count] = i;
		count++;
	    }
	    
        }
     }
   }
    
   *matched_indices = matches;
   return count;
} 
    
static int ismatch(Object list, char *cvalue)
{
   int i,index=0;
   char *string;

     for (i=0; DXExtractNthString(list, i, &string); i++){
        if (!strcmp(cvalue,string)){
           index=i;
           break;
        }
     }
    
     return index;
} 
    
#if !defined(SELECTOR_LIST_SUPPORT)
static Error dolist(Object o,Object *out,int iout,struct einfo *ep, int index)
#else
static Error dolist(Object o,Object *out,int iout,struct einfo *ep, 
				int *matched_indices, int matches)
#endif
{
     int i,item,rank,bytsize;
     int shape[MAXRANK];
     Type type;
     Category cat;
     void *p, *outval;
#if defined(SELECTOR_LIST_SUPPORT)
     void *dest;
#endif

     if (!DXGetArrayInfo((Array)o,&item,&type,&cat,&rank,shape))
        return ERROR;
     bytsize = DXGetItemSize((Array)o);
     if (rank==0) shape[0]=1;

     p = DXGetArrayData((Array)o);
     if (item<1){ 
        while(*ep->mp) ep->mp++;
        sprintf(ep->mp,"{}");
     }
     else{ 
        while(*ep->mp) ep->mp++;
        if (item==1) {
           sprintf(ep->mp,"{"); while(*ep->mp) ep->mp++;
           if (!_dxfprint_message(p,ep,type,rank,shape,item))
              return ERROR;
           while(*ep->mp) ep->mp++; sprintf(ep->mp,"}"); 
        }
        else
           if (!_dxfprint_message(p,ep,type,rank,shape,item))
              return ERROR;
     }
     while(*ep->mp) ep->mp++;

     /* create output object for value */
#if !defined(SELECTOR_LIST_SUPPORT)
     outval = DXAllocate(bytsize);
     memcpy(outval,(void *)((byte *)p +bytsize*index),(size_t)bytsize);
#else
     outval = DXAllocate(bytsize * matches);
     dest = outval;
     for (i=0 ; i<matches ; i++) {
	void *src = (void*)((byte*)p + bytsize * matched_indices[i]);
	memcpy(dest,src,(size_t)bytsize);
	dest = (byte*)dest + bytsize;
     }
#endif
     out[iout] = (Object)DXNewArrayV(type,cat,rank,shape);
#if !defined(SELECTOR_LIST_SUPPORT)
     if (!DXAddArrayData((Array)out[iout],0,1,(Pointer)outval)){
#else
     if (!DXAddArrayData((Array)out[iout],0,matches,(Pointer)outval)){
#endif
        DXFree((Pointer)outval);
        return ERROR;
     }
     DXFree((Pointer)outval);
     return OK;
}

static Object zerobase(int item,int start)

{
   int i;
   int *p;
   Array a;

   a = DXNewArray(TYPE_INT,CATEGORY_REAL,0);
   if (!DXAddArrayData(a,0,item,NULL))
      return NULL; 
   p = (int *)DXGetArrayData(a);
   if (!p)
      return NULL;
   for (i=0; i<item; i++)
      p[i]=start+i;
   
   return (Object)a;
}

static
int getsize(Object in)
{
   int i,rank,item,msglen=1,shape[MAXRANK];
   Type type;
   
   if (!DXGetArrayInfo((Array)in,&item,&type,NULL,&rank,shape))
      return 0;
   if (rank==0){
      /* scalar list */
      msglen = NUMBER_CHARS*item;
      goto done;
   }
   else if (rank==1){
      /* string list */
      if (type == TYPE_STRING){
         msglen = item*shape[0];
         goto done;
      }
      else{
      /* vector list */
	 msglen = item*shape[0]*NUMBER_CHARS;
         goto done;
      }
   }
   else{
      for (i=0; i<rank; i++)
	 msglen *= shape[i]*NUMBER_CHARS;
      msglen *= item;
   }
   
done:
   return msglen;
}
