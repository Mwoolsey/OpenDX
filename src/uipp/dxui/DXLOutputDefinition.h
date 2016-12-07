/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#ifndef _DXLOutputDefinition_h
#define _DXLOutputDefinition_h


#include "DrivenDefinition.h"


//
// Class name definition:
//
#define ClassDXLOutputDefinition	"DXLOutputDefinition"

//
// Referenced classes

//
// DXLOutputDefinition class definition:
//				
class DXLOutputDefinition : public DrivenDefinition
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
    // Allocate a new Node of the corresponding type.
    //
    virtual Node *newNode(Network *net, int instance = -1); 

    virtual SIAllocator getSIAllocator();

  public:
    //
    // Constructor:
    //
    DXLOutputDefinition();

    //
    // Destructor:
    //
    ~DXLOutputDefinition(){}

    //
    // Create a new Module and NodeDefinition of 'this' type. 
    //
    static NodeDefinition *AllocateDefinition();


    virtual boolean isAllowedInMacro() { return FALSE; }

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassDXLOutputDefinition;
    }
};


#endif // _DXLOutputDefinition_h
