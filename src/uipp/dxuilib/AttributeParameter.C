/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#include "AttributeParameter.h"

AttributeParameter::AttributeParameter(ParameterDefinition *pd, 
		Node *n, int index) : BinaryParameter(pd)
{ 
    this->set2ndValue("NULL");
    this->node = n; 
    this->index = index;
    this->syncOnTypeMatch = TRUE;
}
//
// Copy the parameter value into the Attribute value.
//
boolean  AttributeParameter::syncAttributeValue() 
{
    if (this->hasValue()) {
	Type t = this->getValueType();
	const char *v = this->getSetValueString();
	return  this->set2ndValue(v,t,FALSE);
    } else 
	return TRUE;

}


//
// Make sure that the primary parameter has the same value as the
// attribute when appropriate.  Appropriate is defined as the primary
// parameter having a value and a type which is the same as the attribute's.
//
boolean  AttributeParameter::syncPrimaryValue(boolean force) 
{
    if (force || this->syncOnTypeMatch) {
	Type t = this->get2ndValueType();
	if (force || (this->hasValue() && this->getValueType() == t)) {
	    ASSERT(this->isInput());
	    const char *v = this->get2ndValueString();
	    return this->node->setInputSetValue( this->index,v,t,FALSE) != 
			DXType::UndefinedType;
        }
    }
    return TRUE;
}

//
// Determine if the attribute that shadows this parameter is writeable. 
// Attributes are writeable if the primary parameter is not connected
// and (the primary parameter is defaulting or the value has the same 
// type as the attribute value.
//
boolean  AttributeParameter::isAttributeVisuallyWriteable()
{ 
    return this->isDefaulting();
}


//
// Determine if this parameter is of the given class.
//
boolean AttributeParameter::isA(Symbol classname)
{
    return 
      (classname == theSymbolManager->registerSymbol(ClassAttributeParameter))
	||
      this->Parameter::isA(classname);
}
