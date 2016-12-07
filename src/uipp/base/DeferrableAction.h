/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _DeferrableAction_h
#define _DeferrableAction_h


#include "Base.h"


//
// Class name definition:
//
#define ClassDeferrableAction	"DeferrableAction"

typedef void (*DeferrableActionFunction)(void *staticData, void *requestData);

//
// DeferrableAction class definition:
//				
class DeferrableAction : public Base
{
  private:
    //
    // Private member data:
    //
    boolean	required;	// flag if the action is required.
    int		deferrals;	// Number of pending action deferrals
    DeferrableActionFunction actionFunction;
    void		*staticData;

  protected:
    //
    // Protected member data:
    //

    //
    // Flag that the action is required. 
    //
    void    setActionRequired()	{ this->required = TRUE; }

  public:
    //
    // Constructor:
    //
    DeferrableAction(DeferrableActionFunction daf, void *data = NULL);

    //
    // Destructor:
    //
    ~DeferrableAction(){}

    //
    // Request that the action be taken.  If it is currently deferred, then
    // the action won't be taken.
    //
    void    requestAction(void *requestData);
				
    //
    // Defer the action, assuming that the caller uses isActionDeferred()
    // and setActionRequired() when attempting the action. 
    // This meant to be called in pairs with undeferAction().
    // 
    void deferAction()   { this->deferrals++; }

    //
    // Undefer the action, which results in the action being performed if
    // it was requested prior to the undeferral.   If the action is called,
    // then the requestData parameter passed to the action function is NULL.
    // This meant to be called in pairs with deferAction().
    //
    void undeferAction();

    //
    // See if the action is currently deferred.
    //
    boolean isActionDeferred() { return this->deferrals != 0;}

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassDeferrableAction;
    }
};


#endif // _DeferrableAction_h
