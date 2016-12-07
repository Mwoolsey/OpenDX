/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>



/***
MODULE:
    Route
SHORTDESCRIPTION:
    Passes the 2nd input object to the output path(s) specified by the selector list 
    or NULL if the selector is not in valid range.
CATEGORY:
    Special
INPUTS:
    $Select;	Integer list;	0;	The selector value
    ...;	Object;		NULL;	The input object list
OUTPUTS:
    Output;   	Object; 	NULL;	The 2nd input object or NULL
FLAGS:
    MODULE_ROUTE 
BUGS:
AUTHOR:
    Nancy R. Brown
END:
***/

#include <dx/dx.h>
#define NARGS 22
#define MAXOUTPUTS 4

Error
m_Route (Object *in, Object *out)
{
    /* Route module handled entirely by the executive.  */
    /* (will put following code in evalgraph.c)         */
    
    DXSetError(ERROR_INTERNAL, "#8015", "Route");
    return(ERROR);

#if 0
    int i,limit;
    int numselitems, rank, shape[32], *selptr;
    int numoutitems = 0;
    Type type;
    Category category;

    if (in[0] == NULL)
    {
	out[0] == NULL;
	return(OK);
    }
    if (!(DXGetObjectClass(in[0])==CLASS_ARRAY)) {
      DXSetError(ERROR_BAD_PARAMETER,
                 "#10010", "route value");
      return(ERROR);
    }
    if (!DXGetArrayInfo((Array)in[0], &numselitems, &type, &category,
                        &rank, shape)) {
      return(ERROR);
    }
    if (type!=TYPE_INT) {
      DXSetError(ERROR_BAD_PARAMETER,
                 "selector must be an integer or an integer list");
      return(ERROR);
    }
    if (! ((rank == 0) || ((rank == 1)&&(shape[0]=1)))) {
      DXSetError(ERROR_BAD_PARAMETER,
                 "selector must be an integer or an integer list");
      return(ERROR);
    }
/*** ===> How can you determine actual number of outputs ??? ***/
    for (i = 0, limit = MAXOUTPUTS; i < limit; ++i) {
      if (out[i])
        numoutitems++; 
    }
    DXDebug("*1","Route module has %d outputs",numoutitems);
    if (numselitems == 1) { 
      DXDebug("*1","Route module input selector is an integer");
      if (DXExtractInteger(in[0], &i) == ERROR)
      {
	 DXSetError(ERROR_BAD_PARAMETER, "#10010", "route value");
	 return(ERROR);
      }

      for (i = 0, limit = numoutitems; i < limit; ++i) {
        out[i] == NULL;
      }    /* initialize all outputs to NULL */ 
      if (i <= 0 || i >= numoutitems) 
	 return(OK);
      else
      {
         DXDebug("*1","Routing input object to output# %d in Route module",i);
         out[i] = in[1];
         return(OK);
      }
    }   /* end of selector = integer code */
    DXDebug("*1","Route module input selector is an integer list");
    selptr = (int *)DXGetArrayData((Array)in[0]);
    for (i = 0, limit = numselitems; i < limit; ++i) {  
       /*** if (selptr[i] > 0 && selptr[i] <= numoutitems) { use MAXOUTPUTS for now ... ***/        
       if (selptr[i] > 0 && selptr[i] <= MAXOUTPUTS) {        
         DXDebug("*1","Routing input object to output# %d in Route module",i);
         out[selptr[i]] = in[1];
       }
    else
      DXDebug("*1","Route module input selector value %d is out of range",
             selptr[i]);
    }   
    return(OK);
#endif
}
