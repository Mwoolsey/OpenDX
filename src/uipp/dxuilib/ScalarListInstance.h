/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#ifndef _ScalarListInstance_h
#define _ScalarListInstance_h


#include "ScalarInstance.h"


class ScalarListNode;

//
// Class name definition:
//
#define ClassScalarListInstance	"ScalarListInstance"


//
// ScalarListInstance class definition:
//				
class ScalarListInstance : public ScalarInstance
{
  private:
    //
    // Private member data:
    //

  protected:
    //
    // Protected member data:
    //


    //
    // Make sure the given value (assumed to be valid value that type matches
    // with the given output is) complies with any attributes.
    // This is called by InteractorInstance::setOutputValue() which is
    // intern intended to be called by the Interactors.
    // If verification fails (returns FALSE), then a reason is expected to
    // placed in *reason.  This string must be freed by the caller.
    // At this level we always return TRUE (assuming that there are no
    // attributes) and set *reason to NULL.
    //
    // This class verifies the dimensionality of vectors and the range
    // of the component values.
    //
    virtual boolean verifyValueAgainstAttributes(int output, const char *val,
							    Type t,
                                                            char **reason);

  public:
    //
    // Constructor:
    //
    ScalarListInstance(ScalarListNode *n);

    //
    // Destructor:
    //
    ~ScalarListInstance();

    //
    // S/Get value associated with this interactor instance.
    // ScalarListInstances have a component value that is always local to 
    // the instance (i.e. saved in the LocalAttributes list). 
    //
    virtual double getComponentValue(int component);
    virtual void setComponentValue(int component, double val);

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassScalarListInstance;
    }
};


#endif // _ScalarListInstance_h
