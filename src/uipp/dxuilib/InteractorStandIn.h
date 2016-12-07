/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _InteractorStandIn_h
#define _InteractorStandIn_h


#include "StandIn.h"


//
// Class name definition:
//
#define ClassInteractorStandIn	"InteractorStandIn"


//
// InteractorStandIn class definition:
//				
class InteractorStandIn : public StandIn
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
    // Constructor:
    //
    InteractorStandIn(WorkSpace *w, Node *n) : StandIn(w,n) {}

  public:

    //
    // Allocate a instance of this class. Used by the SIAllocatorDictionary. 
    //
    static StandIn *AllocateStandIn(WorkSpace *w, Node *n);

    //
    // Destructor:
    //
    ~InteractorStandIn(){}

    //
    // Called when the StandIn has been selected by the Editor. 
    //
    virtual void handleSelectionChange(boolean select);

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassInteractorStandIn;
    }
};


#endif // _InteractorStandIn_h
