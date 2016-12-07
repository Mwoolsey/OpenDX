/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _BinaryParameter_h
#define _BinaryParameter_h


#include "Parameter.h"


//
// Class name definition:
//
#define ClassBinaryParameter	"BinaryParameter"

//
// BinaryParameter class definition:
//				
class BinaryParameter : public Parameter 
{
  private:
    //
    // Private member data:
    //
	Parameter	*secondValue;

  protected:

  public:


    //
    // Constructor:
    //
    BinaryParameter(ParameterDefinition *pd) : Parameter(pd) 
    {
	this->secondValue = new Parameter(pd);
    }

    //
    // Destructor:
    //
    ~BinaryParameter() { delete this->secondValue; }

    //
    // Manipulate the Value and definition
    //
    Type	set2ndValue(const char *v)
			{ return this->secondValue->setValue(v); }
    boolean 	set2ndValue(const char *v, Type t, boolean coerce = TRUE)
			{ return this->secondValue->setValue(v,t,coerce); }
    //
    // Set the stored value.
    // If the parameter is not defaulting, this is
    // the same as setValue, but if it is defaulting, then we set the
    // value but leave the parameter clean and defaulting.
    //
    boolean set2ndSetValue(const char *v, Type t)
			{ return this->secondValue->setSetValue(v,t); }


    boolean 	has2ndValue() 
		{ return this->secondValue->hasValue(); }
    Type    	get2ndValueType() 
		{ return this->secondValue->getValueType(); }

    const char 	*get2ndSetValueString() 
		{ return this->secondValue->getSetValueString(); }
    const char 	*get2ndValueString() 
		{ return this->secondValue->getValueString(); }

    const char 	*get2ndValueOrDefaultString() 
		{ return this->secondValue->getValueOrDefaultString(); }

    //
    // Get the i'th component of a vector value.  
    //
    double get2ndVectorComponentValue(int index) 
		{ return this->secondValue->getVectorComponentValue(index); }
    //
    // Get the floating point value of a scalar
    //
    double get2ndScalarValue() 
		{ return this->secondValue->getScalarValue(); }
    //
    // Get the value of an integer... 
    //
    int   get2ndIntegerValue() 
		{ return this->secondValue->getIntegerValue(); }
 
    //
    // These methods are somewhat of a hack, but at least they belong here and
    // not in ScalarNode.C which is what uses them (for now anyway).
    //
    // G/Set the n'th component of a vector, scalar or integer.  For scalars
    // and integers, the component number must be 1.
    // Components are indexed from 1.
    //
    double get2ndComponentValue(int component)
		{ return this->secondValue->getComponentValue(component); }
    boolean set2ndComponentValue(int component, double val)
		{ return this->secondValue->setComponentValue(component, val); }
    int get2ndComponentCount()
		{ return this->secondValue->getComponentCount(); }

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassBinaryParameter;
    }
};


#endif // _BinaryParameter_h
