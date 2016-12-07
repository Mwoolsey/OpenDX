/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#include "DeferrableAction.h"

DeferrableAction::DeferrableAction(DeferrableActionFunction daf, void *data)
{
    this->required  = FALSE;
    this->deferrals = 0;
    this->actionFunction = daf;
    this->staticData = data;
}

//
// Request that the action be taken.  If it is currently deferred, then
// the action won't be taken.
//
void    DeferrableAction::requestAction(void *requestData)
{   
    if (this->isActionDeferred())
	this->setActionRequired();
    else
	this->actionFunction(this->staticData, requestData);
}
//
// Undefer the action, which results in the action being performed if
// it was requested prior to the undeferral.   If the action is called,
// then the requestData parameter passed to the action function is NULL.
// This meant to be called in pairs with deferAction().
//
void DeferrableAction::undeferAction()
{  
    ASSERT(this->deferrals > 0);
    if ((--this->deferrals == 0) && this->required) {
	this->required = FALSE;
	this->actionFunction(this->staticData, NULL);
    }
}

