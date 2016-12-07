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

#include "NoUndoCommand.h"
#include "GARApplication.h"
#include "GARCommand.h"
#include "GARMainWindow.h"
#include "ButtonInterface.h"
#include "OpenFileDialog.h"
#include "SADialog.h"
#include "CommentDialog.h"
#include "../base/WarningDialogManager.h"

using namespace std;

String GARCommand::DefaultResources[] =
{
        NULL
};

GARCommand::GARCommand(const char*   name,
                         CommandScope* scope,
                         boolean       active,
                         GARMainWindow* gmw,
			 int option):NoUndoCommand(name,scope,active)
{
	this->gmw = gmw;	
	this->option = option;	
}

boolean GARCommand::doIt(CommandInterface *)
{
    GARMainWindow *gmw = this->gmw;
    char	  *fname;
    unsigned long mode;
    int dirty;
#if defined(HAVE_SSTREAM)
    std::stringstream tmpstr;
#else
    strstream tmpstr;
#endif
#ifdef aviion
#if defined(HAVE_SSTREAM)
    std::stringstream tmpstr2;
#else
    strstream tmpstr2;
#endif
#endif

    switch (this->option)
    {
	case GARMainWindow::SAVE:
	    fname = gmw->getFilename();
	    gmw->saveGAR(fname);
	    delete fname;
	    break;
	case GARMainWindow::SAVE_AS:
	    gmw->save_as_fd->post();
	    break;
	case GARMainWindow::CLOSE_GAR:
	    gmw->unmanage();
	    break;
	case GARMainWindow::CLOSE:
	    break;
	case GARMainWindow::COMMENT:
	    gmw->comment_dialog->post();
	    break;
	case GARMainWindow::FULLGAR:
	    {
                dirty = theGARApplication->isDirty();
		if(!gmw->validateGAR(False)) return False;
		gmw->saveGAR(&tmpstr);
		theGARApplication->changeMode(GMW_FULL, &tmpstr);
                if (dirty) theGARApplication->setDirty();
	    }
	    break;
	case GARMainWindow::SIMPLIFY:
                dirty = theGARApplication->isDirty();
		if(!gmw->validateGAR(False)) return False;
		gmw->saveGAR(&tmpstr);
#ifdef aviion
		gmw->saveGAR(&tmpstr2);
#endif
		mode = gmw->getMode(&tmpstr);
		if(mode & GMW_FULL)
		{
		    WarningMessage("Sorry, the prompter can't be simplified");
		    break;
		}
// All these ifdefs because the aviion C++ libs don't seem to have seekg
#ifdef aviion
		theGARApplication->changeMode(mode, &tmpstr2);
#else
		tmpstr.clear();
		tmpstr.seekg(0, std::ios::beg);
		theGARApplication->changeMode(mode, &tmpstr);
#endif
                if (dirty) theGARApplication->setDirty();
	    break;

	default:
	    ASSERT(FALSE);
    }
    return TRUE;
}


