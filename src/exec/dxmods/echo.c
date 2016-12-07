/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dxmods/echo.c,v 1.6 2006/06/10 16:33:58 davidt Exp $
 */

#include <dxconfig.h>



#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <dx/dx.h>
#include "echo.h"


#define MAXPARMS   21
#define MSGLEN     1024   /* can this be bigger and not blow up the UI? */
#define SLOP	   128
#define AtEnd(p)    ((p)->atend)

struct einfo {
    int maxlen;
    int atend;
    char *mp;
    char *msgbuf;

    int bufSize;
    int reallocFlag;
};

static int EndCheck(struct einfo *ep)
{
    if (ep->atend)
        return 1;

    if ((ep->mp - ep->msgbuf) >= ep->maxlen) {

        if( ep->reallocFlag){
	  int currLen = ep->mp - ep->msgbuf;
	  ep->bufSize *= 2;
	  ep->msgbuf = (char*) DXReAllocate( ep->msgbuf, ep->bufSize );
	  ep->mp = ep->msgbuf + currLen;
	  ep->maxlen = ep->bufSize - SLOP;
	  return 0;
        }
        else {
          ep->atend = 1;

          sprintf(ep->mp, " <msg too long> ");
          while(*ep->mp) 
            ep->mp++;
        
          return 1;
        }
    }
    
    return 0;
}


static char *ClassNameString(Class c);
static void pv(struct einfo *ep, Type t, Pointer value, int offset);
static void pc(struct einfo *ep, Type t, Pointer value, int offset);
static void pq(struct einfo *ep, Type t, Pointer value, int offset);
static void pa(struct einfo *ep, Category c, Type t, Pointer value, int offset);
static void pvalue(struct einfo *ep, Type type, Category category, 
                   int rank, int *shape, Pointer value);
static void pvlist(struct einfo *ep, int nitems, 
                   Type type, Category category, 
                   int rank, int *shape, Pointer value, int itemsize);
static void ps(struct einfo *ep, char *);


int
m_Echo(Object *in, Object *out)
{
    int input, incount;
    char *echo_string = NULL;



    /* ok, until we get an arg count from the exec, we can count up
     *  the parms ourselves.  print until the last non-null one.
     */
    for (input = 0, incount = -1; input < MAXPARMS; input++)
	if(in[input])
	    incount = input+1;

    /* the default message used to be an empty string - now it just
     *  returns WITHOUT printing if input is null.
     */
    if (incount <= 0)
	return OK;


    if (!_dxf_ConvertObjectsToStringValues(in,incount,&echo_string,1))
	goto error;

    /* prevent messages from being mangled if they start with # */
    if (echo_string[0] == '#')
        DXExpandMessage(0);

    DXUIMessage("ECHO", echo_string);

    if (echo_string[0] == '#')
	DXExpandMessage(1);
    
    if (echo_string)
	DXFree((Pointer)echo_string);
    
    return OK;
    
  error:
    if (echo_string)
	DXFree((Pointer)echo_string);
    return ERROR;
}



static char *
ClassNameString(Class c)
{
    switch(c) {    
      case CLASS_MIN:            return "Min object ";
      case CLASS_OBJECT:         return "Generic Object ";
      case CLASS_PRIVATE:        return "Private object ";
      case CLASS_STRING:	 return "String object ";	
      case CLASS_FIELD:	       	 return "Field object ";	
      case CLASS_GROUP:	       	 return "Group object ";	
      case CLASS_SERIES:         return "Series object ";
      case CLASS_COMPOSITEFIELD: return "Compositefield object ";
      case CLASS_MULTIGRID: 	 return "Multigrid object ";
      case CLASS_ARRAY:	       	 return "Array object ";		
      case CLASS_REGULARARRAY:   return "Regulararray object ";
      case CLASS_PATHARRAY:      return "Patharray object ";    
      case CLASS_PRODUCTARRAY:   return "Productarray object ";
      case CLASS_MESHARRAY:      return "Mesharray object ";
      case CLASS_XFORM:	       	 return "Xform object ";		
      case CLASS_SCREEN:	 return "Screen object ";		
      case CLASS_CLIPPED:	 return "Clipped object ";	
      case CLASS_CAMERA:	 return "Camera object ";	
      case CLASS_LIGHT:	       	 return "Light object ";	
      case CLASS_MAX:            return "Max object ";
      case CLASS_DELETED:        return "Deleted object ";        
      case CLASS_CONSTANTARRAY:  return "Constantarray object ";        
      default: 			 return "Unknown object ";
    }
    /* notreached */
}


static void pv(struct einfo *ep, Type t, Pointer value, int offset)
{
    switch(t) {
      case TYPE_STRING:
	if (((char *)value)[offset] == '%')
	    *ep->mp = '%'; ep->mp++;	
        sprintf(ep->mp, "%c", ((char *)value)[offset]); break;
      case TYPE_SHORT:
        sprintf(ep->mp, "%d", ((short *)value)[offset]); break;
      case TYPE_INT:
        sprintf(ep->mp, "%d", ((int *)value)[offset]); break;
      case TYPE_FLOAT:
        sprintf(ep->mp, "%g", ((float *)value)[offset]); break;
      case TYPE_DOUBLE:
        sprintf(ep->mp, "%g", ((double *)value)[offset]); break;
      case TYPE_UINT:
        sprintf(ep->mp, "%u", ((uint *)value)[offset]); break;
      case TYPE_USHORT:
        sprintf(ep->mp, "%u", ((ushort *)value)[offset]); break;
      case TYPE_UBYTE:
        sprintf(ep->mp, "%u", ((ubyte *)value)[offset]); break;
      case TYPE_BYTE:
        sprintf(ep->mp, "%d", ((byte *)value)[offset]); break;
      case TYPE_HYPER:
      default:
	*ep->mp = '?'; ep->mp++; break;
    }
    while(*ep->mp) ep->mp++;
    EndCheck(ep);
}

static void pc(struct einfo *ep, Type t, Pointer value, int offset)
{
    if(EndCheck(ep))
        return;    
    sprintf(ep->mp, "(");      while(*ep->mp) ep->mp++;
    pv(ep, t, value, offset);
    if (EndCheck(ep))
        return;    
    sprintf(ep->mp, ", ");     while(*ep->mp) ep->mp++;
    pv(ep, t, value, offset+1);
    if (EndCheck(ep))
        return;    
    sprintf(ep->mp, ")");      while(*ep->mp) ep->mp++;
}

static void pq(struct einfo *ep, Type t, Pointer value, int offset)
{
    if (EndCheck(ep)) 
        return;    
    sprintf(ep->mp, "(");      while(*ep->mp) ep->mp++;
    pv(ep, t, value, offset);
    if (EndCheck(ep)) 
        return;    
    sprintf(ep->mp, ", ");     while(*ep->mp) ep->mp++;
    pv(ep, t, value, offset+1);
    if (EndCheck(ep)) 
        return;    
    sprintf(ep->mp, ", ");     while(*ep->mp) ep->mp++;
    pv(ep, t, value, offset+2);
    if (EndCheck(ep)) 
        return;    
    sprintf(ep->mp, ", ");     while(*ep->mp) ep->mp++;
    pv(ep, t, value, offset+3);
    if (EndCheck(ep)) 
        return;    
    sprintf(ep->mp, ")");      while(*ep->mp) ep->mp++;
}

static void pa(struct einfo *ep, Category c, Type t, Pointer value, int offset)
{
    offset *= DXCategorySize(c);
    switch (c) {
      case CATEGORY_REAL:
        pv(ep, t, value, offset);
        if (EndCheck(ep)) 
            return;    
        break;
      case CATEGORY_COMPLEX:
        pc(ep, t, value, offset);
        if (EndCheck(ep)) 
            return;    
        break;
      case CATEGORY_QUATERNION:
        pq(ep, t, value, offset);
        if (EndCheck(ep)) 
            return;    
        break;
    }
}

static void pvalue(struct einfo *ep, Type type, Category category, 
                   int rank, int *shape, Pointer value)
{
    int i, j, k;
    
    switch(rank) {
      case 0:
        pa(ep, category, type, value, 0);
        if (EndCheck(ep)) 
            return;    
        sprintf(ep->mp, " ");         while(*ep->mp) ep->mp++;
        break;
      case 1:
        if (EndCheck(ep)) 
            return;
	if (type == TYPE_STRING)
	    ps(ep, (char *)value);
	else {
	    sprintf(ep->mp, "[");        while(*ep->mp) ep->mp++;
	    for(i=0; i<shape[0]; i++) {
		pa(ep, category, type, value, i);
		if (EndCheck(ep)) 
		    return;    
		if (i<shape[0]-1)
		    sprintf(ep->mp, " ");     while(*ep->mp) ep->mp++;
	    }
	    if (EndCheck(ep)) 
		return;    
	    sprintf(ep->mp, "] ");        while(*ep->mp) ep->mp++;
	}
	break;
      case 2:
        if (EndCheck(ep)) 
            return;    
        sprintf(ep->mp, "[");        while(*ep->mp) ep->mp++;
        for(i=0; i<shape[0]; i++) {
            if (EndCheck(ep)) 
                return;    
	    if (type == TYPE_STRING) 
	    {
		ps(ep, ((char *)value) + i*shape[1]);
		if (i < (shape[0]-1)) *ep->mp++ = ' ';
	    }
	    else {
		sprintf(ep->mp, "[");    while(*ep->mp) ep->mp++;
		for(j=0; j<shape[1]; j++) {
		    pa(ep, category, type, value, i*shape[1] + j);
		    if (EndCheck(ep)) 
			return;   
		    if (j<shape[1]-1)
			sprintf(ep->mp, " "); while(*ep->mp) ep->mp++;
		}
		if (EndCheck(ep))
		    return;    
		sprintf(ep->mp, "]");    while(*ep->mp) ep->mp++;
	    }
        }
        if (EndCheck(ep)) 
            return;    
        sprintf(ep->mp, "]");        while(*ep->mp) ep->mp++;
        break;
      case 3:
        if (EndCheck(ep)) 
            return;    
        sprintf(ep->mp, "[");            while(*ep->mp) ep->mp++;
        for(i=0; i<shape[0]; i++) {
            if (EndCheck(ep)) 
                return;    
            sprintf(ep->mp, "[");        while(*ep->mp) ep->mp++;
            for(j=0; j<shape[1]; j++) {
                if (EndCheck(ep)) 
                    return;    
		if (type == TYPE_STRING)
		{
		    ps(ep, ((char *)value + i*shape[1] + j*shape[2]));
		    if (i < (shape[1]-1)) *ep->mp++ = ' ';
		}
		else {
		    sprintf(ep->mp, "[");    while(*ep->mp) ep->mp++;
		    for(k=0; k<shape[2]; k++) {
			pa(ep, category, type, value, 
			   i*shape[1] + j*shape[2] + k);
			if (EndCheck(ep)) 
			    return;   
			if (k<shape[2]-1)
			    sprintf(ep->mp, " "); while(*ep->mp) ep->mp++;
		    }
		    if (EndCheck(ep)) 
			return;    
		    sprintf(ep->mp, "]");    while(*ep->mp) ep->mp++;
		}
            }
            if (EndCheck(ep)) 
                return;    
            sprintf(ep->mp, "]");        while(*ep->mp) ep->mp++;
        }
        if (EndCheck(ep)) 
            return;    
        sprintf(ep->mp, "]");            while(*ep->mp) ep->mp++;
        break;
      default:
        sprintf(ep->mp, "is rank %d ", rank); while(*ep->mp) ep->mp++;
        break;
    }
    EndCheck(ep);
}


static void pvlist(struct einfo *ep, int nitems, 
                   Type type, Category category, 
                   int rank, int *shape, Pointer value, int itemsize)
{
    int i;
    
    if (EndCheck(ep)) 
        return;    
    sprintf(ep->mp, "{ ");    while(*ep->mp) ep->mp++;
    for (i=0; i<nitems; i++) {
        pvalue(ep, type, category, rank, shape, 
               (Pointer)((long)value + i * itemsize));
        if (EndCheck(ep))
            return;
	if (i < (nitems-1)) *ep->mp++ = ' ';
    }
    
    if (EndCheck(ep)) 
        return;  
    sprintf(ep->mp, " }");    while(*ep->mp) ep->mp++;
}


static void ps(struct einfo *ep, char *cp)
{
    while(*cp) {
        if(*cp == '%')
            *ep->mp++ = '%';
        
        *ep->mp++ = *cp++;
        if (EndCheck(ep))
            return;
    }
    *ep->mp = '\0';
}

Error _dxf_ConvertObjectsToStringValues(Object *in, int nobj, char **retstr, int rf)
{
    struct einfo ei;
    int input, items, rank, shape[100];
    char *cp;
    Pointer dp;
    Class class;
    Type t;
    Category c;

    /* allocate a buffer in which to collect the message.
     */
    ei.maxlen = MSGLEN;
    ei.atend = 0;
    ei.msgbuf = (char *)DXAllocateZero(MSGLEN+SLOP);
    if (!ei.msgbuf)
 	goto error;	
    ei.bufSize = MSGLEN+SLOP;
    ei.reallocFlag = rf;

    /* loop through input, adding to the end of the accumulated string
     *  each time.  since we used DXAllocateZero, the first time through
     *  the loop, *mp will be null and won't be advanced.
     */
    ei.mp = ei.msgbuf;
    for (input=0; input < nobj && !AtEnd(&ei); input++) {
        
	/* check for null parms */
	if(!in[input]) {
	    sprintf(ei.mp, "NULL");
            while(*ei.mp) ei.mp++;
	}
	else
	    switch ((class = DXGetObjectClass(in[input]))) {
	      case CLASS_ARRAY:
		if (!DXGetArrayInfo((Array)in[input], &items,
					    &t, &c, &rank, shape))
		    goto error;
		
		dp = DXGetArrayData((Array)in[input]);
		if (!dp)
		    goto error;
		
		if (items == 1)
		    pvalue(&ei, t, c, rank, shape, dp);
		else
		    pvlist(&ei, items, t, c, rank, shape, 
			   dp, DXGetItemSize((Array)in[input]));
		break;
		
	      case CLASS_STRING:
		cp = DXGetString((String)(in[input]));
		ps(&ei, cp);
		break;
		
	      case CLASS_GROUP:
		class = DXGetGroupClass((Group)in[input]);
		/* fall thru! */
		
	      default:
		sprintf(ei.mp, "%s", ClassNameString(class));
		while(*ei.mp) ei.mp++;
		break;
	    }
	
	if (input < (nobj-1)) 
	    *ei.mp++ = ' ';
        
    }
    
    *retstr = ei.msgbuf;
    return OK;
    
  error:
    if (ei.msgbuf) DXFree((Pointer)ei.msgbuf);
    *retstr = NULL;
    return ERROR;
}
