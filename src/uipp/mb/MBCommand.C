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

#include "DXStrings.h"
#include "NoUndoCommand.h"
#include "MBApplication.h"
#include "MBCommand.h"
#include "MBMainWindow.h"
#include "ButtonInterface.h"
#include "OptionsDialog.h"


String MBCommand::DefaultResources[] =
{
        NULL
};

MBCommand::MBCommand(const char*   name,
                         CommandScope* scope,
                         boolean       active,
                         MBMainWindow* mdmw,
			 int option):NoUndoCommand(name,scope,active)
{
	this->mdmw = mdmw;	
	this->option = option;	
}

boolean MBCommand::doIt(CommandInterface *ci)
{
    Arg      	   wargs[8];
    MBMainWindow *mdmw = this->mdmw;
    char	  *fname;

    switch (this->option)
    {
	case MBMainWindow::OPEN:
	    mdmw->open_fd->post();
	    break;
	case MBMainWindow::SAVE:
	    fname = mdmw->getFilename();
	    mdmw->saveMB(fname);
	    delete fname;
	    break;
	case MBMainWindow::SAVE_AS:
	    mdmw->save_as_fd->post();
	    break;
	case MBMainWindow::CLOSE:
	    break;
	case MBMainWindow::COMMENT:
	    mdmw->comment_dialog->post();
	    mdmw->comment_dialog->installEditorText(mdmw->comment_text);
	    break;
	case MBMainWindow::OPTIONS:
	    mdmw->options_dialog->post();
	    break;
	case MBMainWindow::BUILD_MDF:
	    mdmw->build(DO_MDF);
	    break;
	case MBMainWindow::BUILD_C:
	    mdmw->build(DO_C);
	    break;
	case MBMainWindow::BUILD_MAKEFILE:
	    mdmw->build(DO_MAKE);
	    break;
	case MBMainWindow::BUILD_ALL:
	    mdmw->build(DO_C|DO_MDF|DO_MAKE);
	    break;
	case MBMainWindow::BUILD_EXECUTABLE:
	    mdmw->build(DO_C|DO_MDF|DO_MAKE|EXECUTABLE);
	    break;
	case MBMainWindow::BUILD_RUN:
	    mdmw->build(DO_C|DO_MDF|DO_MAKE|RUN);
	    break;
	default:
	    ASSERT(FALSE);
    }
    return TRUE;
}


