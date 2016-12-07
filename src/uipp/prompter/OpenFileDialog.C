/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#include "../base/DXStrings.h"
#include "OpenFileDialog.h"
#include "../base/Application.h"
#include "../base/WarningDialogManager.h"
#include <Xm/Text.h>
#include <Xm/SelectioB.h>
#include "GARApplication.h"

boolean OpenFileDialog::ClassInitialized = FALSE;

String OpenFileDialog::DefaultResources[] =
{
        "*dirMask:	*.general",
        "*textColumns:	30",
	".dialogTitle:	Open a Data Prompter Header...",
        NULL
};

Widget OpenFileDialog::createDialog(Widget parent)
{
    Widget shell = this->FileDialog::createDialog(parent);
    return shell;
}

void OpenFileDialog::okFileWork(const char *filenm)
{
    std::ifstream *from = new std::ifstream(filenm);
#ifdef aviion
    std::ifstream *from2 = new std::ifstream(filenm);
#endif
    if(!from)
    {
	WarningMessage("File open failed.");
	return;
    }

    unsigned long mode = this->gmw->getMode(from);

#ifdef aviion
    theGARApplication->changeMode(mode, from2);
#else
    from->clear();
    from->seekg(0, std::ios::beg);
    theGARApplication->changeMode(mode, from);
#endif

    //
    // The main window may have been destroyed and re-created, so use
    // the one the application knows about.
    //
    theGARApplication->getMainWindow()->setFilename((char *)filenm);
}

OpenFileDialog::OpenFileDialog(GARMainWindow *gmw ) : 
                       FileDialog("openFileDialog", gmw->getRootWidget())
{
    this->gmw = gmw;
    this->hasCommentButton = False;

    //
    // Install the default resources for THIS class (not the derived classes)
    //
    if (NOT OpenFileDialog::ClassInitialized)
    {
	ASSERT(theApplication);
        OpenFileDialog::ClassInitialized = TRUE;
	this->installDefaultResources(theApplication->getRootWidget());
    }
}

//
// Install the default resources for this class.
//
void OpenFileDialog::installDefaultResources(Widget baseWidget)
{
    this->setDefaultResources(baseWidget, OpenFileDialog::DefaultResources);
    this->FileDialog::installDefaultResources(baseWidget);
}

void OpenFileDialog::post()
{
    this->FileDialog::post();
}
