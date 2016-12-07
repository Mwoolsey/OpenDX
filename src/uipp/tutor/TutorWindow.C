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
#include <Xm/CascadeB.h>
#include <Xm/RowColumn.h>
#include <Xm/PushB.h>


#include "TutorWindow.h"
#include "TutorApplication.h"
#include "lex.h"
#include "DXStrings.h"
#include "ErrorDialogManager.h"
#include "ButtonInterface.h"

#include "CommandScope.h"
#include "help.h"
#include "../widgets/MultiText.h"

String TutorWindow::DefaultResources[] =
{
    ".width:                                      490",
    ".height:                                     700",
    ".minWidth:                                   300",
    ".minHeight:                                  200",
    ".iconName:                                   DX Tutorial",
    ".title:                                      DX Tutorial",
    "*helpCloseOption.labelString:		  Quit",

    "*fileMenu.labelString:                       File",
    "*fileMenu.mnemonic:                          F",
    "*quitOption.labelString:                     Quit",

    "*helpMenu.labelString:                      Help",
    "*helpMenu.mnemonic:                         H",

    "*XmForm.accelerators:			#augment\n\
    <Key>Return:				BulletinBoardReturn()",

    NULL
};


boolean TutorWindow::ClassInitialized = FALSE;

TutorWindow::TutorWindow() : HelpWin("dxTutorWindow", TRUE)
{

    //
    // Initialize member data.
    //

    //
    // Initialize member data.
    //
    this->fileMenu          = NUL(Widget);

    this->fileMenuPulldown  = NUL(Widget);
	
}


TutorWindow::~TutorWindow()
{
    // FIXME: memory leak city !!!! delete everything that was allocated.

    //
    // Make the panel disappear from the display (i.e. don't wait for
    // (two phase destroy to remove the panel). 
    //
    if (this->getRootWidget())
        XtUnmapWidget(this->getRootWidget());
}


void TutorWindow::initialize()
{

    if (!this->isInitialized()) {
	//
	// Initialize default resources (once only).
	//
	if (NOT TutorWindow::ClassInitialized)
	{
	    TutorWindow::ClassInitialized = TRUE;
	    ASSERT(theApplication);
	    this->setDefaultResources(theApplication->getRootWidget(),
					TutorWindow::DefaultResources);
	    this->setDefaultResources(theApplication->getRootWidget(),
					HelpWin::DefaultResources);
	    this->setDefaultResources(theApplication->getRootWidget(),
					MainWindow::DefaultResources);
	}

    }

    //
    // Now, call the superclass initialize().
    //
    this->HelpWin::initialize();

}

//
//  Build the menu bar for the tutorial window.
//
void TutorWindow::createMenus(Widget parent)
{
    this->createFileMenu(parent);
}
// FIXME: not complete yet
void TutorWindow::createFileMenu(Widget parent)
{
    //
    // Create "File" menu and options.
    //
    Widget pulldown =
        this->fileMenuPulldown =
            XmCreatePulldownMenu(parent, "fileMenuPulldown", 
					NUL(ArgList), 0);
    this->fileMenu =
        XtVaCreateManagedWidget
            ("fileMenu",
             xmCascadeButtonWidgetClass,
             parent,
             XmNsubMenuId, pulldown,
             NULL);

    new ButtonInterface(pulldown,"quitOption",
					theTutorApplication->quitCmd);

}
void TutorWindow::unmanage()
{
    exit(0);
}

