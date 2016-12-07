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

#include <dx/dx.h>

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

#ifndef MAXDIMS
#define MAXDIMS 32
#endif



/*
 * A table of depComp (dependent component) structures is use to
 * allow these components to be changed to reflect the changes in
 * the postions component upon which they depend.
 */


typedef struct depComp {
    char	*name;		
    Array	newA;		
    Pointer	new_data;
    Array	oldA;
    Pointer	old_data;
    int		iSize;
} depComp;


typedef struct argblck {
    Object inField;
    Object outField;
    float scale;
} argblck;


static Object DoIsolate(Object o,float scale);

static Error Isolate_Field(Pointer p);

Error m_Isolate(Object *in, Object *out)
{
    
    Object o = NULL;
    float scale;
    
    /* Check for required input object */
    
    if(!in[0]) {
	DXSetError(ERROR_BAD_PARAMETER,"missing input");
	goto error;
    }
    
    /* Typecheck scale input or set to default (.5) */
    
    if(!in[1]) 
      scale = .5;
    else if (!DXExtractFloat(in[1],&scale)){
	DXSetError(ERROR_BAD_PARAMETER,"scale must be a scalar value");
	goto error;
    }

    if ((scale<0) || (scale>1)) {
	DXSetError(ERROR_BAD_PARAMETER,"scale must be between 0 and 1");
	goto error;
    }

    if (scale==0)
	DXWarning("scale value for isolate is zero.");
    
    /* Copy input object and call DoIsolate on the copy */
    
    DXCreateTaskGroup();
    
    o = DoIsolate(in[0],1-scale);
    if (! o)
      goto error;
    
    if (!DXExecuteTaskGroup())
      goto error;
    
    if(!o)
      goto error;
    
    /* o is now our output */
    
    out[0] = o;
    return OK;
    
    /* Handle errors */
    
  error:	
    if (o != in[0])
      DXDelete((Object)o);
    
    out[0] = NULL;
    return ERROR;
    
}

static Object DoIsolate(Object o,float scale)
{
    
    Object oo,subo,subo2;
    int fixed,z;
    Matrix m;
    int i;
    char *name;
    struct argblck arg;
    
    switch (DXGetObjectClass(o)) {
	
      case CLASS_FIELD:
	
	if (DXEmptyField((Field)o))
	    return o;

	oo = DXCopy(o,COPY_STRUCTURE);
	
	if(!oo)
	  goto error;
	
	arg.inField = o;
	arg.outField = oo;
	arg.scale = scale;
	
	if(!DXAddTask(Isolate_Field,&arg,sizeof(arg),0.0))
	  goto error;
	
	return oo;
	
	break;
	
      case CLASS_GROUP:
	{
	    Group g = (Group)DXCopy(o, COPY_HEADER);
	    if (! g)
	      return NULL;
	    
	    for (i=0; (oo= DXGetEnumeratedMember((Group)o,i,&name)); i++)
	      {
		  if(!(oo = DoIsolate(oo,scale)))
		    goto error;	
		  
		  name ? DXSetMember(g,name,oo) :
		    DXSetEnumeratedMember(g,i,oo);
		  
	      }
	    
	    return (Object)g;
	    
	    break;
	}
	
      case CLASS_XFORM:
	
	{
	    Xform x = (Xform)DXCopy(o,COPY_HEADER);
	    
	    DXGetXformInfo((Xform)o,&subo,&m);
	    
	    if(!(oo = DoIsolate(subo,scale)))
	      goto error;
	    
	    DXSetXformObject(x,oo);
	    
	    return (Object)x;
	    
	    break;
	}
	
      case CLASS_CLIPPED:
	
	{
	    Clipped c = (Clipped)DXCopy(o,COPY_HEADER);
	    
	    DXGetClippedInfo((Clipped)o,&subo,&subo2);
	    
	    if(!(oo = DoIsolate(subo,scale)))
	      goto error;
	    
	    DXSetClippedObjects(c,oo,subo2);
	    
	    return (Object)c;
	    
	    break;
	    
	}
	
      case CLASS_SCREEN:
	
	{	
	    
	    Screen s = (Screen)DXCopy(o,COPY_HEADER);
	    
	    DXGetScreenInfo((Screen)o,&subo,&fixed, &z);
	    
	    if(!(oo = DoIsolate(subo,scale)))
	      goto error;
	    
	    DXSetScreenObject(s,oo);
	    
	    return (Object)s;
	    
	    break;
	    
	}
	
      default:
	DXSetError(ERROR_BAD_PARAMETER,"object must be a field or group");
	goto error;
	
    }
    
    
    return o;
    
  error:
    
    /* Delete anything we might have lying around */
    
    return ERROR;
}



static Error 
  Isolate_Field(Pointer p)
{
    
    Object attr;
    Array old_pA,new_pA,old_cA,new_cA;
    Array compArray;
    ArrayHandle old_cHandle = NULL,old_pHandle = NULL;
    InvalidComponentHandle old_invPHandle = NULL,new_invPHandle = NULL;
    /* The following fixes a bizarre compiler optimizer bug on hp700 <JKAROL622> */
#ifdef hp700
    volatile
#endif
    Point point[8],delta,centroid;
    depComp *depPCompA = NULL;		/* posns dependant component table */
    char *eType,*name,*str;
    float *new_positions;	
    int *new_connections;
    int i,j,k;
    int nVperE,nElements,pDim;
    int ndepPComps = 0;
    int has_invalid_posns = FALSE;
    int newIndex = 0;
    Field inField,outField;
    float scale;
    int shape[MAXDIMS];
    int rank;
    
    struct argblck *arg = (struct argblck *)p;
    
    inField = (Field)arg->inField;
    outField = (Field)arg->outField;
    scale = arg->scale;

    /* Check that input has connections */
    
    old_cA = (Array)DXGetComponentValue(inField,"connections");
    if(!old_cA) {
	DXSetError(ERROR_MISSING_DATA,"input has no connections");
	goto error;
    }
    
    old_pA = (Array)DXGetComponentValue(inField,"positions");
    if(!old_pA){
	DXSetError(ERROR_MISSING_DATA,"field has no positions");
	goto error;
    }
    
    /* get dimensionality of positions array */
    
    DXGetArrayInfo(old_pA,NULL,NULL,NULL,&rank,shape);
    for (i=0, pDim=1;i<rank;i++)
	pDim *= shape[i];
    
    /* Determine the number of connection elements and vertices/element */
    
    DXGetArrayInfo(old_cA,&nElements,NULL,NULL,&rank,shape);
    for (i=0, nVperE=1;i<rank;i++)
	nVperE *= shape[i];
    
    /* Save the element type so we can use it latter	*/
    if (!(eType=(char *)(DXGetString((String)DXGetComponentAttribute(inField,
							    "connections",
							    "element type"))))){
        DXSetError(ERROR_MISSING_DATA,
                   "missing connection element type attribute");
	goto error;
    }
    
    /*
     * Count the number of components that are dep positions so we can
     * allocate a table of pointers to these arrays for use when we
     * create the new postions. We don't count "positions","connections"
     * or anything that is also ref positions 'cause we will throw it out.
     */
    
    i = 0;
    while(NULL != 
	  (compArray = (Array)DXGetEnumeratedComponentValue((Field)inField,
							    i++,
							    &name)))
    {
	  if(!strcmp(name, "connections") || !strcmp(name, "positions"))
	    continue;
	  
	  attr = DXGetComponentAttribute(inField,name,"dep");
	  if( attr != NULL ) {
	      str = DXGetString((String)attr);
	      
	      /* if component is dep positions */
	      if(str != NULL)
		  if(!strcmp(str, "positions")){
		      /* get the ref attribute */
		      if(NULL != (attr = DXGetComponentAttribute(inField,name,"ref")))
			{
			    /*
			     * if component is also ref positions we ignore it because
			     * it will be deleted or treated specially if it's
			     * "invalid positions"
			     */
			    
			    str = DXGetString((String)attr);
			    if( str != NULL && !strcmp(str, "positions"))
			      continue;
			}
		      ndepPComps++;
		  }
	}
      }
    
    if(ndepPComps){
	depPCompA = DXAllocate(ndepPComps*sizeof(depComp));
	if(!depPCompA)
	  goto error;
    }
    
    ndepPComps = 0; 
    
    /*
     *  Search the field for components that are dep positions and 
     *  create an entry in the depPComp table for them.  If they are
     *  ref positons toss them unless it's "invalid positons" which 
     *  is a special case. 
     */
    
    i = 0;
    
    while(NULL != 
	  (compArray = (Array)DXGetEnumeratedComponentValue((Field)inField,
							    i++,
							    &name)))
      {
	  if(!strcmp(name, "connections") || !strcmp(name, "positions"))
	    continue;
	  
	  if (NULL != (attr = DXGetComponentAttribute(inField,name,"ref")))
	    {
		str = DXGetString((String)attr);
		if( str != NULL && !strcmp(str, "positions"))
		  {
		      /* component name is ref positions, we will delete it
		       * unless it's invalid positions 
		       */
		      
		      if(strcmp(name,"invalid positions"))
			DXDeleteComponent((Field)inField,name);
		      else
			has_invalid_posns = TRUE;
		  }
		continue;
	    }
	  
	  attr = DXGetComponentAttribute(inField,name,"dep");
	  
	  if(attr)
	    {
		str = DXGetString((String)attr);
		if (str != NULL && !strcmp(str,"positions"))
		  {
		      /*
		       * component is dep positions so create a new array
		       * for it and save the info in depPCompA
		       */
		      
		      int rank;
		      int shape[MAXDIMS];
		      Category cat;
		      Type type;
		      
		      if(!DXGetArrayInfo(compArray,NULL,&type,
					 &cat,&rank,shape))
			goto error;
		      
		      depPCompA[ndepPComps].name = name;
		      depPCompA[ndepPComps].newA = 
			(Array)DXNewArrayV(type,cat,rank,shape);
		      if(!DXAddArrayData(depPCompA[ndepPComps].newA,
					 0,nElements*nVperE,NULL))
			goto error;
		      depPCompA[ndepPComps].new_data =
			(Pointer)DXGetArrayData(depPCompA[ndepPComps].newA);
		      depPCompA[ndepPComps].oldA = compArray;
		      depPCompA[ndepPComps].old_data = 	
			(Pointer)DXGetArrayData(compArray);
		      depPCompA[ndepPComps].iSize = DXGetItemSize(compArray);
		      ndepPComps++;
		  }
	    }
      }
    
    /* Allocate the new positions array. It will be of size */
    /* nElements x nVperE	                                  */
    
    new_pA = (Array)DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1 , pDim);
    if(!new_pA)
      goto error;
    
    if(!DXAddArrayData(new_pA, 0,nElements*nVperE, NULL))
      goto error;
    
    new_positions = (float *)DXGetArrayData(new_pA);
    if(!new_positions)
      goto error;
    
    /* delete components der positions or connections */
    
    DXChangedComponentValues(outField,"positions");
    DXChangedComponentValues(outField,"connections");
    
    /* Allocate the new connection array. It will be the same size */
    /* as the original.						 */
    
    new_cA = (Array)DXNewArray(TYPE_INT,CATEGORY_REAL,1,nVperE);
    if(!new_cA)
      goto error;
    
    if(!DXAddArrayData(new_cA, 0,nElements, NULL))
      goto error;
    
    new_connections = (int*)DXGetArrayData(new_cA);
    if(!new_connections)
      goto error;
    
    /*		   
     * Replace positions and connections in the output field with
     * the new arrays.
     */
    
    
    DXSetComponentValue(outField,"positions",(Object)new_pA);
    DXSetComponentValue(outField,"connections",(Object)new_cA);
    DXSetComponentAttribute((Field)outField,
			    "connections",
			    "element type",
			    (Object)DXNewString(eType));
    DXSetComponentAttribute((Field)outField,
			    "connections",
			    "ref",
			    (Object)DXNewString("positions"));
    
    /* get handles for the original position and connection arrays */
    
    old_pHandle = DXCreateArrayHandle(old_pA);
    if(!old_pHandle)
      goto error;
    
    old_cHandle = DXCreateArrayHandle(old_cA);
    if(!old_cHandle)
      goto error;
    
    /* If the input field has an invalid positions component
     * create the old and new invalid component handles.  
     */
    
    if(has_invalid_posns){
	DXDeleteComponent(outField,"invalid positions");
	old_invPHandle = DXCreateInvalidComponentHandle((Object)inField,
							NULL,
							"positions");
	new_invPHandle = DXCreateInvalidComponentHandle((Object)outField,
							NULL,
							"positions");
    }
    
    /* Loop thru all the connection elements and isolate the positions */
    /* to their new locations					    */
    
    for(i = 0;i<nElements;i++) {
	
	int eBuffer[8], *element;
	float pBuffer[3];
	element = (int*)DXGetArrayEntry(old_cHandle, i,(Pointer)eBuffer);
	if(pDim == 3){
	    centroid.x=0;centroid.y=0;centroid.z=0;
	    for(j=0;j<nVperE;j++){
		point[j] = *(Point *)DXGetArrayEntry(old_pHandle,
						     element[j],
						     (Pointer)pBuffer);
		centroid = DXAdd(centroid,point[j]);
	    }
	    centroid = DXDiv(centroid,(double)nVperE);
	    for(j=0;j<nVperE;j++){
		delta = DXSub(centroid,point[j]);
		point[j] = DXAdd(point[j],DXMul(delta,(double)scale));
		
		*new_positions++ = point[j].x;
		*new_positions++ = point[j].y;
		*new_positions++ = point[j].z;
		
		if(has_invalid_posns){
		    if(DXIsElementValid(old_invPHandle,element[j]))
		      DXSetElementValid(new_invPHandle,newIndex);
		    else
		      DXSetElementInvalid(new_invPHandle,newIndex);
		    
		    newIndex++;
		}
		
		
		for(k=0;k<ndepPComps;k++){
		    memcpy(depPCompA[k].new_data,
			   (char*)depPCompA[k].old_data+(element[j]*depPCompA[k].iSize),
			   depPCompA[k].iSize);
		    depPCompA[k].new_data = 
		      (char *)depPCompA[k].new_data + depPCompA[k].iSize;
		}
	    }
	}       
	else if(pDim == 2){
	    centroid.x=0;centroid.y=0;centroid.z=0;
	    for(j=0;j<nVperE;j++){
		point[j] = *(Point *)DXGetArrayEntry(old_pHandle
						     ,element[j]
						     ,(Pointer)pBuffer);
		point[j].z = 0;
		centroid = DXAdd(centroid,point[j]);
	    }
	    centroid = DXDiv(centroid,(double)nVperE);
	    
	    for(j=0;j<nVperE;j++){
		delta = DXSub(centroid,point[j]);
		point[j] = DXAdd(point[j],DXMul(delta,(double)scale));
		*new_positions++ = point[j].x;
		*new_positions++ = point[j].y;
		
		if(has_invalid_posns){
		    if(DXIsElementValid(old_invPHandle,element[j]))
		      DXSetElementValid(new_invPHandle,newIndex);
		    else
		      DXSetElementInvalid(new_invPHandle,newIndex);
		    
		    newIndex++;
		}
		
		for(k=0;k<ndepPComps;k++){
		    memcpy(depPCompA[k].new_data,
			   (char*)depPCompA[k].old_data+(element[j]*depPCompA[k].iSize),
			   depPCompA[k].iSize);
		    depPCompA[k].new_data = 
		      (char *)depPCompA[k].new_data + depPCompA[k].iSize;
		}
	    }
	}
	else{
	    DXSetError(ERROR_DATA_INVALID,"Positions must be 2D or 3D");
	    goto error;
	}
	
	/* OK the new positions are in now we create the new connection */
	
	for(j=0;j<nVperE;j++)
	  new_connections[i*nVperE+j] = i*nVperE+j;
	
    }
    
    /* Save the invalid positions component if it exists */
    
    if(has_invalid_posns){
	if(!DXSaveInvalidComponent(outField,new_invPHandle))
	  goto error;
    }
    
    
    /* Set  the new arrays for any components that were dep positions */
    
    for(k=0;k<ndepPComps;k++){
	if(!DXSetComponentValue(outField,depPCompA[k].name,
					(Object)depPCompA[k].newA))
	  goto error;
	depPCompA[k].newA = NULL;
    }
    
    if(! DXEndField(outField))
      goto error;
    
    if (old_pHandle) DXFreeArrayHandle(old_pHandle);
    if (old_cHandle) DXFreeArrayHandle(old_cHandle);
    if (old_invPHandle) DXFreeInvalidComponentHandle(old_invPHandle);
    if (new_invPHandle) DXFreeInvalidComponentHandle(new_invPHandle);
    if (depPCompA) DXFree((Pointer)depPCompA);
    return OK;
    
  error:
    
    if (old_pHandle) DXFreeArrayHandle(old_pHandle);
    if (old_cHandle) DXFreeArrayHandle(old_cHandle);
    if (old_invPHandle) DXFreeInvalidComponentHandle(old_invPHandle);
    if (new_invPHandle) DXFreeInvalidComponentHandle(new_invPHandle);
    if (depPCompA) DXFree((Pointer)depPCompA);
    return ERROR;
    
}


















