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
#if defined(HAVE_SSTREAM)
#include <sstream>
#elif defined(HAVE_STRSTREAM_H)
#include <strstream.h>
#elif defined(HAVE_STRSTREA_H)
#include <strstrea.h>
#endif

#include "StartupCommand.h"
#include "StartupWindow.h"

String StartupCommand::DefaultResources[] =
{
        NULL
};

StartupCommand::StartupCommand(const char*   name,
                                       CommandScope* scope,
                                       boolean       active,
                                       StartupWindow  *startup,
                                       StartupCommandType comType ) :
        NoUndoCommand(name, scope, active)
{
    this->commandType = comType;
    this->startup_window = startup;
}
 


boolean StartupCommand::doIt (CommandInterface *ci)
{
    StartupWindow *sw = this->startup_window;
    ASSERT (sw);
    boolean ret = TRUE;

    switch (this->commandType) {
	case StartupCommand::StartImage:
	    sw->startImage();
	    break;
	case StartupCommand::StartPrompter:
	    sw->startPrompter();
	    break;
	case StartupCommand::StartEditor:
	    sw->startEditor();
	    break;
	case StartupCommand::EmptyVPE:
	    sw->startEmptyVPE();
	    break;
	case StartupCommand::StartTutorial:
	    sw->startTutorial();
	    break;
	case StartupCommand::StartDemo:
	    sw->startDemo();
	    break;
	case StartupCommand::Quit:
	    sw->quit();
	    break;
	case StartupCommand::Help:
	    sw->help();
	    break;
	default:
	    ASSERT(0);
    }

    return ret;
}

