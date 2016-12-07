/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#ifndef _SelectionDefinition_h
#define _SelectionDefinition_h

#include "InteractorDefinition.h"


//
// Class name definition:
//
#define ClassSelectionDefinition	"SelectionDefinition"


//
// SelectionDefinition class definition:
//				
class SelectionDefinition : public InteractorDefinition 
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
    SelectionDefinition() { }

    //
    // Destructor:
    //
    ~SelectionDefinition() { }
	
    //
    // Returns a pointer to the class name.
    //
    virtual const char* getClassName() 
		{ return ClassSelectionDefinition; }
};


#endif // _SelectionDefinition_h
