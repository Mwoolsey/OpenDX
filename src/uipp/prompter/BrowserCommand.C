/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#include <Xm/Xm.h>

#include "../base/NoUndoCommand.h"
#include "BrowserCommand.h"
#include "Browser.h"
#include "GARApplication.h"
#include "../base/ButtonInterface.h"

String BrowserCommand::DefaultResources[] =
{
        NULL
};

BrowserCommand::BrowserCommand(const char*   name,
                         CommandScope* scope,
                         boolean       active,
                         Browser* browser,
			 int option):NoUndoCommand(name,scope,active)
{
	this->browser = browser;	
	this->option = option;	
}

boolean BrowserCommand::doIt(CommandInterface *)
{
    Browser 	  *browser = this->browser;

    theGARApplication->setBusyCursor(TRUE);
    switch (this->option)
    {
	case Browser::PLACE_MARK:
	    browser->placeMark();
	    break;

	case Browser::CLEAR_MARK:
	    browser->clearMark();
	    break;

	case Browser::GOTO_MARK:
	    browser->gotoMark();
	    break;

	case Browser::FIRST_PAGE:
	    browser->firstPage();
	    break;

	case Browser::LAST_PAGE:
	    browser->lastPage();
	    break;

	case Browser::SEARCH:
	    browser->postSearchDialog();
	    break;

	case Browser::CLOSE:
	    browser->unmanage();
	    break;

	default:
	    ASSERT(FALSE);
    }
    theGARApplication->setBusyCursor(FALSE);
    return TRUE;
}
