/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#ifndef _VectorDefinition_h
#define _VectorDefinition_h

#include <Xm/Xm.h>

#include "ScalarDefinition.h"


//
// Class name definition:
//
#define ClassVectorDefinition	"VectorDefinition"

//
// Forward definitions 
//
class ParameterDefinition;
class Interactor;

//
// VectorDefinition class definition:
//				
class VectorDefinition : public ScalarDefinition 
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
    VectorDefinition() { }

    //
    // Destructor:
    //
    ~VectorDefinition() { }
	
    //
    // Allocate a new VectorDefinition.  
    // This function is intended to be used as an allocator in
    // theNDAllocatorDictionary.
    //
    static NodeDefinition *AllocateDefinition();

    //
    // Returns a pointer to the class name.
    //
    virtual const char* getClassName() 
		{ return ClassVectorDefinition; }
};


#endif // _VectorDefinition_h
