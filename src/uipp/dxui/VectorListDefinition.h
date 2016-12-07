/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#ifndef _VectorListDefinition_h
#define _VectorListDefinition_h

#include <Xm/Xm.h>

#include "ScalarListDefinition.h"


//
// Class name definition:
//
#define ClassVectorListDefinition	"VectorListDefinition"


//
// VectorListDefinition class definition:
//				
class VectorListDefinition : public ScalarListDefinition 
{
  private:
	
  protected:
    //
    // Protected member data:
    //

    //
    // Allocate a new Node of the corresponding type.
    //
    virtual Node *newNode(Network *net, int instance = -1); 

  public:
    //
    // Constructor:
    //
    VectorListDefinition() { }

    //
    // Destructor:
    //
    ~VectorListDefinition() { }
	
    //
    // Allocate a new VectorListDefinition.  
    // This function is intended to be used as an allocator in
    // theNDAllocatorDictionary.
    //
    static NodeDefinition *AllocateDefinition();

    //
    // Returns a pointer to the class name.
    //
    virtual const char* getClassName() 
		{ return ClassVectorListDefinition; }
};


#endif // _VectorListDefinition_h
