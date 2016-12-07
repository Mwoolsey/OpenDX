/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#include <stdio.h>
#include <string.h>
 
#include "DXStrings.h"
#include "ScalarListNode.h"
#include "ListIterator.h"
#include "ScalarListInstance.h"
#include "ErrorDialogManager.h"
#include "Parameter.h"


//
// Create the component 
//
ScalarListNode::ScalarListNode(NodeDefinition *nd,
			Network *net, int instance, 
			boolean isvector, int dimensions) :
                        ScalarNode(nd, net, instance, isvector, dimensions)
{
}

//
// Called after allocation is complete.
// The work done here is to assigned default values to the InteractorNode inputs
// so that we can use them later when setting the attributes for the
// Interactor. 
//
boolean ScalarListNode::initialize()
{
    if (!this->verifyInputCount())
	return FALSE;

    const char *value=NULL;
    switch (this->numComponents) {
	case 1: if (this->isVectorType()) 
		    value = "{ [ 0 ] }"; 
		else
		    value = "{ 0 }"; 
		break;
	case 2: value = "{ [ 0 0 ] }"; break;
	case 3: value = "{ [ 0 0 0 ] }"; break;
	default: ASSERT(0); 
    }

    if ((this->setOutputValue(1,value, DXType::UndefinedType, FALSE) == 
				DXType::UndefinedType)
	||
    	!this->setDefaultAttributes()) {
        ErrorMessage(
      "Error setting default attributes for %s interactor, check ui.mdf\n",
		this->getNameString());
	return FALSE;
    }

    //
    // Make the shadows defaulting (even though we have a current output)
    // so that the executive module can tell when it is executing a just
    // placed module and one that is read in from a .net or .cfg file.
    // When read in, the output will be set again which should make the
    // corresponding shadowing input be non-defaulting.
    //
    this->setShadowingInputsDefaulting();
    return TRUE;
}
//
// Do what ever is necessary when the given component of the output
// value is out of range as indicated by the limits.  Typically, this just
// means resetting the current output value that is associated with the
// node.  ScalarListNodes however, have the current (non-output) value
// associated with the interactor instance. Therefore, ScalarListNodes,
// re-implement this to reset the component values of all their instances.
// If 'component' is less than 0, then min/max are ignored and all
// components are checked with the current min/max values.
// NOTE: this is a copy of what is used in VectorListNode (no it is not a
// 	sub-class) so if changes are made here they may also need to be
//	made there.
//
extern boolean ClampVSIValue(const char *val, Type valtype,
                        double *mins, double *maxs,
                        char **clampedval);

void ScalarListNode::doRangeCheckComponentValue(int component, 
						double min, double max)

{
    char *clamped;

    int i, ncomp = this->getComponentCount();
    double *mins = new double[ncomp];
    double *maxs = new double[ncomp];

    for (i=1 ; i<=ncomp ; i++) {
	mins[i-1] = this->getComponentMinimum(i);
	maxs[i-1] = this->getComponentMaximum(i);
    }

    const char *val = this->getOutputValueString(1);
    Type output_type = this->getOutputSetValueType(1);

#if 1
    if (DXValue::ClampVSIValue(val,output_type,mins,maxs,&clamped)) {
	this->setOutputValue(1,clamped,DXType::UndefinedType,TRUE);
        delete clamped;
    }
#else
    ListIterator iterator(this->instanceList);
    ScalarListInstance *sli;
    while (sli = (ScalarListInstance*) iterator.getNext()) {
	double value = sli->getComponentValue(component);
	if (value < min) {
	    doit = TRUE;
	    sli->setComponentValue(component,min);
	} else if (value > max) {
	    doit = TRUE;
	    sli->setComponentValue(component,max);
	}
    }
#endif

    delete mins;
    delete maxs;

}

//
// Get a ScalarInstance instead of an InteractorInstance.
//
InteractorInstance *ScalarListNode::newInteractorInstance()
{
    ScalarListInstance *si = new ScalarListInstance(this);

    return (InteractorInstance*)si;
}

//
// In addition to the ScalarNode messages we can also handle
// the following
//
// 		'list={%g, %g...}'
// or 
// 		'list={ [ %g %g ... ], ...}'
//
// If any input or output values are to be changed, don't send them
// because the module backing up the interactor will have just executed
// and if the UI is in 'execute on change' mode then there will be an
// extra execution.
//
// Return the number of tokens handled.
//
int ScalarListNode::handleInteractorMsgInfo(const char *line)
{
    int values = this->ScalarNode::handleInteractorMsgInfo(line);

    if (this->isVectorType())
	values += this->handleVectorListMsgInfo(line);
    else
	values += this->handleScalarListMsgInfo(line);

    return values;
}
//
// Handle
//
// 		'list={%g, %g...}'
//
// If any input or output values are to be changed, don't send them
// because the module backing up the interactor will have just executed
// and if the UI is in 'execute on change' mode then there will be an
// extra execution.
//
// Return the number of tokens handled.
//
int ScalarListNode::handleScalarListMsgInfo(const char *line)
{
    char *p;
    int  values = 0;

    //
    // Handle the 'list={...}' part of the message.
    //
    if ( (p = strstr((char*)line,"list=")) ) {
	values++;
	while (*p != '=') p++;
	p++;
        p = FindDelimitedString(p, '{', '}');
   	if (p) {
	    this->setShadowedOutputSentFromServer(1,p,DXType::UndefinedType);	
	    delete p;
	} else {
	    this->setShadowedOutputSentFromServer(1,
				"NULL",DXType::UndefinedType);	
	}
    }
    return values;
}


//
// Handle the vector list message 
//
// 		'list={ [ %g %g ... ], ...}'
//
// If any input or output values are to be changed, don't send them
// because the module backing up the interactor will have just executed
// and if the UI is in 'execute on change' mode then there will be an
// extra execution.
//
// Return the number of tokens handled.
//
int ScalarListNode::handleVectorListMsgInfo(const char *line)
{
    char *p;
    int  values = 0;

    //
    // Handle the 'list={...}' part of the message.
    //
    if ( (p = strstr((char*)line,"list=")) ) {
	values++;
	while (*p != '=') p++;
	p++;
        p = FindDelimitedString(p, '{', '}');
   	if (p) {
	    this->setShadowedOutputSentFromServer(1,p,DXType::UndefinedType);	
	    delete p;
	} else {
	    this->setShadowedOutputSentFromServer(1,
				"NULL",DXType::UndefinedType);	
	}
    }
    return values;
}

boolean ScalarListNode::adjustOutputDimensions(int old_dim, int new_dim)
{

#define CHUNK	256
    char buf[256];

    //
    // Adjust the output value
    //
    const char *outval = this->getOutputValueString(1);
    char *value = new char[CHUNK];
    strcpy(value,"{ ");
    int out_index = -1;
    char *v = value;
    int valuelen = STRLEN(value);
    int maxlen = CHUNK;
    v += valuelen;
    while (DXValue::NextListItem(outval, &out_index, 
			DXType::VectorListType, buf, 256)) {
	char *vec = DXValue::AdjustVectorDimensions(buf,new_dim, 0.0, 
				this->isIntegerTypeComponent());
	if (!vec) {
	    delete value;
	    return FALSE;
	}
	int veclen = STRLEN(vec);
	if (valuelen + veclen + 1 >= maxlen - 1 ) {
	    // The +1 and -1 above are to make sure we have room for '}'.
	    value = (char*)REALLOC((void*)value, 
					(maxlen + CHUNK) * sizeof(char));
	    maxlen += CHUNK;
	}
	sprintf(v,"%s ",vec);
	v += veclen + sizeof(char); 
	valuelen += veclen + sizeof(char);
	delete vec;
    }
    strcat(v,"}");
    this->setOutputValue(1,value,DXType::VectorListType,TRUE);
    delete value;
    return TRUE;

}

//
// Determine if this node is of the given class.
//
boolean ScalarListNode::isA(Symbol classname)
{
    Symbol s = theSymbolManager->registerSymbol(ClassScalarListNode);
    if (s == classname)
	return TRUE;
    else
	return this->ScalarNode::isA(classname);
}
