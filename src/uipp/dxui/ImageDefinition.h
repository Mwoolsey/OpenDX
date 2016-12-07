/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#ifndef _ImageDefinition_h
#define _ImageDefinition_h


#include "NodeDefinition.h"


//
// Class name definition:
//
#define ClassImageDefinition	"ImageDefinition"

//
// Referenced classes
class Network;

//
// ImageDefinition class definition:
//				
class ImageDefinition : public NodeDefinition
{
  private:
    //
    // Private member data:
    //
    Cacheability defaultInternalCacheability;    // are the image mods cached by default

  protected:
    //
    // Protected member data:
    //

    //
    // Defines the function that allocates CDBs for this node.
    //
    virtual CDBAllocator getCDBAllocator();

    //
    // Allocate a new Node of the corresponding type.
    //
    virtual Node *newNode(Network *net, int instance = -1); 

  public:
    Cacheability getDefaultInternalCacheability()
                                { return this->defaultInternalCacheability; }

    //
    // Constructor:
    //
    ImageDefinition();

    //
    // Destructor:
    //
    ~ImageDefinition(){}

    //
    // Create a new Module and NodeDefinition of 'this' type. 
    //
    static NodeDefinition *AllocateDefinition();

    virtual void finishDefinition();

    virtual boolean isAllowedInMacro() { return FALSE; }

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassImageDefinition;
    }
};


#endif // _ImageDefinition_h
