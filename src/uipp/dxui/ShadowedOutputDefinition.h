/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#ifndef _ShadowedOutputDefinition_h
#define _ShadowedOutputDefinition_h

// #include <Xm/Xm.h>

#include "DrivenDefinition.h"

//
// Class name definition:
//
#define ClassShadowedOutputDefinition	"ShadowedOutputDefinition"

//
// Forward definitions 
//

//
// ShadowedOutputDefinition class definition:
//				
class ShadowedOutputDefinition : public DrivenDefinition 
{
  private:
	
  protected:
    //
    // Protected member data:
    //

  public:
    //
    // Constructor:
    //
    ShadowedOutputDefinition() { }

    //
    // Destructor:
    //
    ~ShadowedOutputDefinition() { }
	
    //
    // Returns a pointer to the class name.
    //
    virtual const char* getClassName() { return ClassShadowedOutputDefinition; }
};


#endif // _ShadowedOutputDefinition_h
