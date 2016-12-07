/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#include "lex.h"
#include "Ark.h"
#include "Parameter.h"
#include "ParameterDefinition.h"
#include "List.h"
#include "ListIterator.h"

//////////////////////////////////////////////////////////////////////////////
Parameter::Parameter(ParameterDefinition *pd)
{
    this->definition = pd;
    this->value = NUL(DXValue*);
    this->dirty = TRUE;
    this->defaultingWhenUnconnected = TRUE;
    if (pd) {
        this->visible = pd->getDefaultVisibility();
        this->cacheability = pd->getDefaultCacheability();
    } else {
        this->visible = TRUE; 
        this->cacheability = OutputFullyCached; 
    }
}

//
// Delete all non-static state associated with a Parameter.  
// Don't delete the definition as that is a pointer into the Parameter 
// definition list of a NodeDefinition.
// Don't delete the arcs from the arc list, as the Network should do
// that.  We can however, clear the list (remove the links).
//
Parameter::~Parameter()
{
    if (this->value) 
	delete this->value;

    this->disconnectArks();

}
//
// Remove all the Arks connected to this parameter.
//
void Parameter::disconnectArks()
{
    //
    // call getElement(1) because the arc destructor will 
    // remove itself from the parameter list.  So, the 
    // arc that should be destroyed is always the first 
    // element in the list.
    //
    Ark  *a;
    while ( (a = (Ark*) this->arcs.getElement(1)) ) 
       delete a;

}
//
// Return true if the Executive needs to know the value of this parameter
//
boolean Parameter::isNeededValue(boolean ignoreDirty)
{
	if (this->definition->isInput()) {		// An input
		if (!this->isConnected())
			return ignoreDirty || this->isDirty();
	} else if (this->isConnected()) {		// An output
		return ignoreDirty || this->isDirty();
	}

	return FALSE;
}

//
// For the given type, try and augment the given string value with 
// syntactic sugar to coerce the given value to the given type.
// The types we can coerce are StringType, ListType, VectorType and TensorType.
// Return T/F indicating whether the value was successfully set (and coerced)
//
boolean Parameter::coerceAndSetValue(const char *value, Type type)
{
    char *s = DXValue::CoerceValue(value, type);
    boolean r = FALSE;
    if (s) {
        r = this->value->setValue(s, type);
	delete s;
    }
    return r;
}
//
// Set this parameter to have the given value which must match one of the
// types in the ParameterDefinition's list of types.
// On success return the Type the value was found to match, and set the value
// of this parameter to the value given.
// On failure, return DXType::UndefinedType
//
Type Parameter::setValue(const char *value)
{
    DXType *dxtype;
    Type type;
    ParameterDefinition *pd;
    ListIterator iterator;

    if (!value) {
	if (this->setValue(NULL, DXType::UndefinedType))
	    return DXType::UndefinedType;
	else if (this->hasValue())
	    return this->getValueType();
	else
	    return DXType::UndefinedType;
    }

    pd = this->getDefinition();

    ASSERT(pd);

    //
    // Try matching the value to the type without coersion first. 
    //
    FOR_EACH_PARAM_TYPE(pd, dxtype, iterator) {
        type = dxtype->getType();
        if (this->setValue(value,type, FALSE))
	    return type;
    }   

    //
    // If the above didn't work, lets try and coerce the values
    //
    FOR_EACH_PARAM_TYPE(pd, dxtype, iterator) {
        type = dxtype->getType();
        if (this->setValue(value,type, TRUE))
	    return type;
    }   

    return DXType::UndefinedType;
}
boolean Parameter::setValue(const char *value, Type type, boolean coerce)
{
	boolean success;

	if (!this->value) 
		this->value = new DXValue;


	if (type == DXType::UndefinedType AND NOT value) {
		if (this->value) delete this->value;
		this->value = NUL(DXValue*);
		success = TRUE;
	} else {
		ParameterDefinition *pd = this->getDefinition();
		ListIterator li(*pd->getTypes());
		DXType *dxt;
		boolean typeMatch = FALSE;
		while( (dxt = (DXType*)li.getNext()) )
			if (dxt->MatchType(type, dxt->getType()))
			{
				typeMatch = TRUE;
				break;
			}

		if (typeMatch && (
			this->value->setValue(value, type)  ||
			(coerce && this->coerceAndSetValue(value,type)))) {
				success = TRUE;
		} else
			success = FALSE;
	}

	if (success) {
		this->setDirty();
		if (type == DXType::UndefinedType AND NOT value)
			this->setUnconnectedDefaultingStatus(TRUE);
		else
			this->setUnconnectedDefaultingStatus(FALSE);
	}

	return success;
}
//
// Set the stored value.  
// If the parameter is not defaulting, this is
// the same as setValue, but if it is defaulting, then we set the
// value but leave the parameter clean and defaulting.
//
boolean Parameter::setSetValue(const char *value, Type type)
{
    boolean was_defaulting = this->defaultingWhenUnconnected;

    boolean r = this->setValue(value,type);
        
    if (was_defaulting) {
        this->clearDirty();
        this->setUnconnectedDefaultingStatus();
    }   
    return r;
}       

//
// This method is somewhat of a hack, but at least it belongs here and
// not in ScalarNode.C which is what uses it (for now anyway).
// 
// Get the number of component of a vector, scalar or integer.  For scalars
// and integers, the component count is 1.  
//
int Parameter::getComponentCount()
{
    int ret = 0;

    switch (this->getValueType()) {
        case DXType::IntegerListType:
        case DXType::ScalarListType:
        case DXType::IntegerType:
        case DXType::ScalarType:
        case DXType::FlagType:
            ret = 1; 
            break;
        case DXType::VectorType:
        case DXType::VectorListType:
            ret = this->value->getVectorComponentCount();
            break;
        default:
            ASSERT(0);
    }
    return ret;
}
//
// This method is somewhat of a hack, but at least it belongs here and
// not in ScalarNode.C which is what uses it (for now anyway).
// 
// Get the n'th component of a vector, scalar or integer.  For scalars
// and integers, the component number must be 1.  
// Components are indexed from 1.
//
double Parameter::getComponentValue(int component)
{
    double ret = 0;
    ASSERT(component > 0);

    switch (this->getValueType()) {
        case DXType::FlagType:
        case DXType::IntegerType:
            ASSERT(component == 1);
            ret = (double)this->getIntegerValue();
            break;
        case DXType::ScalarType:
            ASSERT(component == 1);
            ret = this->getScalarValue();
            break;
        case DXType::VectorType:
            ret = this->getVectorComponentValue(component);
            break;
        default:
            ASSERT(0);
    }
    return ret;
}

//
// This method is somewhat of a hack, but at least it belongs here and
// not in ScalarNode.C which is what uses it (for now anyway).
// 
// Set the n'th component of a vector, scalar or integer.  For scalars
// and integers, the component number must be 1.  
// Components are indexed from 1.
//
boolean Parameter::setComponentValue(int component, double val)
{
    ASSERT(component > 0);
    DXValue *v = this->value;
    boolean r = true;

    ASSERT(v);

    switch (this->getValueType()) {
        case DXType::FlagType:
            ASSERT(component == 1);
            r = v->setInteger(val == 0.0 ? 0 : 1);
            break;
        case DXType::IntegerType:
            ASSERT(component == 1);
            r = v->setInteger((int)val);
            break;
        case DXType::ScalarType:
            ASSERT(component == 1);
            r = v->setScalar(val);
            break;
        case DXType::VectorType:
            r = v->setVectorComponentValue(component, val);
            break;
        default:
            ASSERT(0);
    }
    this->setDirty();
    return r;
}
//
// Determine if this instance is derived from the given class 
//
boolean Parameter::isA(const char *classname)
{
    Symbol s = theSymbolManager->registerSymbol(classname);
    return this->isA(s);
}
//
// Determine if this instance is derived from the given class 
//
boolean Parameter::isA(Symbol classname)
{
    Symbol s = theSymbolManager->registerSymbol(ClassParameter);
    return (s == classname);
}

boolean Parameter::isDefaulting()
{
   if (this->isInput() && this->isConnected())
	return FALSE;
   else
	return this->defaultingWhenUnconnected;
}
void Parameter::setUnconnectedDefaultingStatus(boolean defaulting)	
{
    if (this->defaultingWhenUnconnected != defaulting) 
    {
	this->setDirty(); 
	this->defaultingWhenUnconnected = defaulting; 
    }
}
boolean Parameter::addArk(Ark *a)	
{
    boolean r = this->arcs.appendElement((const void*)a);

    //
    // If this is the first arc and this is an output parameter,
    // be sure it gets sent on the next execution (assuming someone
    // asks this parameter if it needs to be sent).
    //
    if (r && (this->arcs.getSize() == 1) && !this->isInput())  
	this->setDirty();

    return r;
}

//
// Get the selectable values for this parameter. 
// This returns a pointer to a constant array of pointers to
// constant strings which is NOT to be manipulated by the caller.
// NULL may be returned;
// The returned array of pointers is NULL terminated.
//
const char* const *Parameter::getValueOptions()	
{
    return this->definition->getValueOptions();
}

#ifdef DXUI_DEVKIT 
//
// Get any variable declarations that will be required by the code
// generated in getObjectCreateCode(). NULL is returned if none required.
// The returned string must be freed by the caller.
//
char *Parameter::getObjectCodeDecl(const char *indent, const char *tag)
{
    if (!this->hasValue())
	return NULL;

    char *p, *code = new char[1024];
    Type type = this->getValueType();
    int i, count,items,tuples, hasDecimal;
    double *values, *data;

    switch (this->getValueType()) {

        case DXType::IntegerListType:
        case DXType::VectorListType:
        case DXType::ScalarListType:

	    items = DXValue::GetDoublesFromList(
			this->getValueString(),type,&data,&tuples);
	    
	    count = items * tuples;

	    for (i = hasDecimal = 0; i < count && ! hasDecimal; i++)
	        if ((int)data[i] != data[i])
		    hasDecimal = 1;

	    if (hasDecimal)
	    {
		sprintf(code,"%sfloat %s_tmp1[] = { ",indent, tag);
		for (i=0, p = code+STRLEN(code) ; i<count ; i++, p+=STRLEN(p))
		{
		    sprintf(p," %g", data[i]); 
		    if (i != (count-1) && count > 1) strcat(p,", ");
		}
		strcat(p,"};\n");
	    }
	    else
	    {
		sprintf(code,"%sint %s_tmp1[] = { ",indent, tag);
		for (i=0, p = code+STRLEN(code) ; i<count ; i++, p+=STRLEN(p))
		{
		    sprintf(p," %d", (int)data[i]); 
		    if (i != (count-1) && count > 1) strcat(p,", ");
		}
		strcat(p,"};\n");
	    }

	    delete data;
 
	    break;

        case DXType::FlagType:
        case DXType::IntegerType:
        case DXType::ScalarType:
        case DXType::VectorType:
	    count =  this->getComponentCount();

	    values = new double[count];

	    for (i = hasDecimal = 0; i < count; i++)
	    {
		values[i] = this->getComponentValue(i+1);
		if ((int)values[i] != values[i])
		    hasDecimal = 1;
	    }

	    if (hasDecimal)
	    {
		sprintf(code,"%sfloat %s_tmp1[] = { ",indent, tag);
		for (i=0, p = code+STRLEN(code) ; i<count ; i++, p+=STRLEN(p))
		{
		    sprintf(p," %g", values[i]);
		    if (i != (count-1) && count > 1) strcat(p,", ");
		}
		strcat(p,"};\n");
	    }
	    else
	    {
		sprintf(code,"%sint %s_tmp1[] = { ",indent, tag);
		for (i=0, p = code+STRLEN(code) ; i<count ; i++, p+=STRLEN(p))
		{
		    sprintf(p," %g", values[i]);
		    if (i != (count-1) && count > 1) strcat(p,", ");
		}
		strcat(p,"};\n");
	    }
		
	    delete values;

	    break;

        default:
	    delete code;
	    code = NULL;

    }

    return code;

}
//
// Get a string that represents the libDX C code to generate
// a DX Object from the value of the parameter.  The code is
// created to assign the object to the given lvalue variable name.
// The returned string must be freed by the caller.
//
char *Parameter::getObjectCreateCode(const char *indent, 
					const char *tag, const char *lvalue)
{
    if (!this->hasValue())
	return NULL;

    char *listitem, *code = new char[1024], varspec[128];
    int i, rank, shape, items;
    Type type = this->getValueType();
    DXTensor tensor;
    boolean r;
    int count, hasDecimal;
    double *data;

    switch (this->getValueType()) {

        case DXType::IntegerListType:
        case DXType::VectorListType:
        case DXType::ScalarListType:

	    items = DXValue::GetDoublesFromList(
			this->getValueString(),type,&data,&shape);
	    
	    count = items * shape;

	    for (i = hasDecimal = 0; i < count && ! hasDecimal; i++)
	        if ((int)data[i] != data[i])
		    hasDecimal = 1;


	    delete data;
	    break;

        case DXType::FlagType:
        case DXType::IntegerType:
        case DXType::ScalarType:
        case DXType::VectorType:

	    items = 1;
	    shape = count = this->getComponentCount();

	    for (i = hasDecimal = 0; i < count; i++)
	    {
		double value = this->getComponentValue(i+1);
		if ((int)value != value)
		    hasDecimal = 1;
	    }

	    break;
	
        case DXType::StringType:
	    sprintf(code,
		"%s%s = (Object)DXNewString(%s);\n",
		indent,lvalue,this->getValueString());
	    return code;

        default:
	    sprintf(code,"%s%s = NULL;\n",indent,lvalue);
            fprintf(stderr,"Conversion to DX Object for type %s"
                           " not supported yet.\n",DXType::TypeToString(type)); 
	    return code;
    }
    
    rank = (shape == 1) ? 0 : 1;
    
    sprintf(code,
	"    %s = (Object)DXNewArray(%s, CATEGORY_REAL, %d, %d);\n"
	"    if (%s == NULL) goto error;\n"
	"    if (DXAddArrayData((Array)%s,0,%d,%s_tmp1) == NULL)\n"
	"	goto error;\n",
	lvalue, hasDecimal ? "TYPE_FLOAT" : "TYPE_INT", rank, shape, 
	lvalue, 
	lvalue, items, tag);
    
    return code;
}
// 
// Generate code to clean up after the code from getObjectCreateCode()
// was executed. lvalue must be the same value that was passed to 
// getObjectCreateCode().
// NULL may be returned;
// The returned string must be freed by the caller.
//
char *Parameter::getObjectCleanupCode(const char *indent, const char *tag)
{

    if (!this->hasValue())
	return NULL;

    char *code = new char[1024];
    Type type = this->getValueType();
    switch (this->getValueType()) {

        default:
	    delete code;
	    code = NULL;
    }

    return code;
    return NULL;
}
#endif // DXUI_DEVKIT
