/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



// AttributeParameter.h -						    //
//                                                                          //
// Definition for the AttributeParameter class.				    //
//
// The AttributeParameter (Data-driven parameter) class is intended to be used
// with data-driven interactors, in which interactor attributes may also
// be parameters to the executive module that does the data-driving.
// In this class, we use the second value of the BinaryParameter class to
// hold the attribute value with the primary parameter value always taking
// presidence over the secondary value.
//                                                                          //


#ifndef _AttributeParameter_h
#define _AttributeParameter_h


#include "BinaryParameter.h"


//
// Class name definition:
//
#define ClassAttributeParameter	"AttributeParameter"

class Node;

//
// AttributeParameter class definition:
//				
class AttributeParameter : public BinaryParameter 
{
  private:
    //
    // Private member data:
    //
  protected:

     Node 	*node;
     int	index;
     boolean	syncOnTypeMatch;

  public:


    //
    // Constructor:
    //
    AttributeParameter(ParameterDefinition *pd, Node *n, int index) ; 

    //
    // Destructor:
    //
    ~AttributeParameter() { }


#if 0	// 6/10/93
// If we use this then DrivenNode::ioParameterValueChanged() needs to
// notifyVisualsOfStateChange() so that the displayed attribute is in 
// sync with the internal value.
    //
    // If the super-class method succeeds then, copy the value into 
    // the attribute (the secondary parameter value).
    //
    virtual boolean 	setValue(const char *v, Type type, boolean coerce=TRUE)
		{  boolean r = this->BinaryParameter::setValue(v,type,coerce);
		   if (r && this->syncOnTypeMatch && (this->hasValue() &&
			     this->getValueType() == this->get2ndValueType())) {
			r = this->set2ndValue(this->getSetValueString(),
							type,FALSE);
			ASSERT(r);
		   }
		   return r;
		}
#endif
		
    //
    // Copy the parameter value into the Attribute value.
    //
    boolean  syncAttributeValue();

    //
    // Make sure that the primary parameter has the same value as the
    // attribute when appropriate.  Appropriate is defined as the primary
    // parameter having a value and a type which is the same as the attribute's.
    //
    boolean	syncPrimaryValue(boolean force = FALSE); 

    void setSyncOnTypeMatch(boolean v = TRUE) 
			{ this->syncOnTypeMatch = v; }

    //
    // Determine if the attribute that shadows this parameter is writeable.
    // Attributes are writeable if the primary parameter is not connected
    // and (the primary parameter is defaulting or the value has the same 
    // type as the attribute value.
    //
    boolean  isAttributeVisuallyWriteable();

    boolean 	setAttributeValue(const char *val, boolean force = FALSE)
		{ return this->set2ndValue(val) && 
				this->syncPrimaryValue(force); }

    boolean 	initAttributeValue(const char *val)
		{ return this->set2ndValue(val) && 
			 this->syncPrimaryValue(TRUE); }



    const char	*getAttributeValueString()
		{  ASSERT(this->has2ndValue()); 
		   return this->get2ndValueString(); 
		}
    Type	getAttributeValueType()
		{  ASSERT(this->has2ndValue()); 
		   return this->get2ndValueType(); 
		}

    //
    // S/Get the i'th component of a vector attribute.  
    //
    boolean setAttributeComponentValue(int index, double value) 
		{ return this->set2ndComponentValue(index, value) && 
					this->syncPrimaryValue(); }

    double getAttributeComponentValue(int index) 
		{ ASSERT(this->has2ndValue()); 
		  return this->get2ndComponentValue(index);
		}

    int	getAttributeComponentCount()  
		{ return (this->has2ndValue() ? 
				this->get2ndComponentCount() : 0); }

    virtual boolean isA(Symbol classname);

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassAttributeParameter;
    }
};


#endif // _AttributeParameter_h
