/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#ifndef _FileSelectorDefinition_h
#define _FileSelectorDefinition_h

#include "ValueDefinition.h"


//
// Class name definition:
//
#define ClassFileSelectorDefinition	"FileSelectorDefinition"

//
// Forward definitions 
//
class ParameterDefinition;
class Interactor;

//
// FileSelectorDefinition class definition:
//				
class FileSelectorDefinition : public ValueDefinition 
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
    FileSelectorDefinition() { }

    //
    // Destructor:
    //
    ~FileSelectorDefinition() { }
	
    //
    // Allocate a new FileSelectorDefinition.  
    // This function is intended to be used as an allocator in
    // theNDAllocatorDictionary.
    //
    static NodeDefinition *AllocateDefinition();

    //
    // Returns a pointer to the class name.
    //
    virtual const char* getClassName() { return ClassFileSelectorDefinition; }
};


#endif // _FileSelectorDefinition_h
