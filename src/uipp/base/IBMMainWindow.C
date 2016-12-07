/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include <defines.h>




#include <Xm/Xm.h>
#include <Xm/CascadeB.h>
#include <Xm/Separator.h>
#include <Xm/RowColumn.h>

#include "DXStrings.h"
#include "IBMMainWindow.h"
#include "IBMApplication.h"
#include "CommandScope.h"
#include "HelpOnContextCommand.h"
#include "UIComponentHelpCommand.h"
#include "ButtonInterface.h"
#include "WarningDialogManager.h"
#include "HelpMenuCommand.h"
#include "icon50.h"
#include "WizardDialog.h"


boolean IBMMainWindow::ClassInitialized = FALSE;

String IBMMainWindow::DefaultResources[] =
{
    "*helpMenu.labelString:                       Help",
    "*helpMenu.mnemonic:                          H",
    "*onContextOption.labelString:                Context-Sensitive Help",
    "*onContextOption.mnemonic:                   C",
    "*onWindowOption.labelString:                 Overview (of Window)...",
    "*onWindowOption.mnemonic:                    O",
    "*onManualOption.labelString:                 Table of Contents...",
    "*onManualOption.mnemonic:                    T",
    "*onHelpOption.labelString:                   Using Help...",
    "*onHelpOption.mnemonic:                      H",
    "*aboutAppOption.labelString:                 Product Information...",
    "*aboutAppOption.mnemonic:                    n",
    "*techSupportOption.labelString:              Technical Support...",
    "*techSupportOption.mnemonic:                 S",

    NULL
};
#if 0
//
// This method should only be called if this window does not have a
// menu bar (i.e. this->hasMenuBar == FALSE);
//
void IBMMainWindow::createMenus(Widget parent)
{
    this->MainWindow::createMenus(parent);
}
#endif

void IBMMainWindow::createBaseHelpMenu(Widget parent, 
				boolean add_standard_help, 
				boolean addAboutApp)
{
    ASSERT(parent);
    Widget            pulldown;

    ASSERT(this->menuBar);
    //
    // Create "Help" menu and options.
    //
    pulldown =
        this->helpMenuPulldown =
            XmCreatePulldownMenu(parent, "helpMenuPulldown", NUL(ArgList), 0);

        this->helpMenu =
            XtVaCreateManagedWidget
                ("helpMenu",
                 xmCascadeButtonWidgetClass,
                 parent,
                 XmNsubMenuId, pulldown,
                 NULL);

    if (add_standard_help) {
	this->onContextOption =
            new ButtonInterface (pulldown, "onContextOption", 
                this->helpOnContextCmd);

	this->onWindowOption =
            new ButtonInterface(pulldown, "onWindowOption",
                this->helpOnWindowCmd);

	this->onManualOption =
            new ButtonInterface(pulldown, "onManualOption",
                theIBMApplication->genericHelpCmd);

	this->onHelpOption =
            new ButtonInterface(pulldown, "onHelpOption",
                theIBMApplication->genericHelpCmd);


    }

    if (addAboutApp) {
    	 XtVaCreateManagedWidget("separator", xmSeparatorWidgetClass, pulldown,
                        	NULL);

        this->helpAboutAppCmd =
            new HelpMenuCommand
                ("helpAboutApp", NULL, TRUE, HelpMenuCommand::AboutApp);
	this->aboutAppOption =
            new ButtonInterface(pulldown, "aboutAppOption",
                this->helpAboutAppCmd);

        this->helpTechSupportCmd =
            new HelpMenuCommand
                ("helpTechSupport", NULL, TRUE, HelpMenuCommand::TechSupport);
	this->techSupportOption =
            new ButtonInterface(pulldown, "techSupportOption",
                this->helpTechSupportCmd);
    } 

    XtVaSetValues(parent, XmNmenuHelpWidget, this->helpMenu, NULL);
}


IBMMainWindow::IBMMainWindow(const char* name, boolean usesMenuBar): 
		MainWindow(name, usesMenuBar)
{
    this->helpMenuPulldown = NULL;
    this->helpMenu = NULL;

    this->onContextOption = NULL;
    this->onHelpOption = NULL;
    this->onManualOption = NULL;
    this->onWindowOption = NULL;
    this->aboutAppOption = NULL;
    this->helpAboutAppCmd = NULL;
    this->techSupportOption = NULL;
    this->helpTechSupportCmd = NULL;

    if (usesMenuBar)  {
        this->helpOnContextCmd =
            new HelpOnContextCommand
                ("helpOnContext", NULL, TRUE, this);
        this->helpOnWindowCmd =
            new UIComponentHelpCommand
                ("helpOnWindow", NULL, TRUE, this);
    } else {
        this->helpOnContextCmd  = NULL;
        this->helpOnWindowCmd = NULL;
    }

    this->wizardDialog = NUL(WizardDialog*);
}


IBMMainWindow::~IBMMainWindow()
{
    if (this->onContextOption) delete this->onContextOption;
    if (this->onHelpOption) delete this->onHelpOption;
    if (this->onManualOption) delete this->onManualOption;
    if (this->onWindowOption) delete this->onWindowOption;

    if (this->helpOnContextCmd) delete this->helpOnContextCmd;
    if (this->helpOnWindowCmd) delete this->helpOnWindowCmd;

    if (this->aboutAppOption) delete this->aboutAppOption;
    if (this->helpAboutAppCmd) delete this->helpAboutAppCmd;

    if (this->techSupportOption) delete this->techSupportOption;
    if (this->helpTechSupportCmd) delete this->helpTechSupportCmd;

    if (this->wizardDialog) delete this->wizardDialog;
}


//
// Install the default resources for this class.
//
void IBMMainWindow::installDefaultResources(Widget baseWidget)
{
    this->setDefaultResources(baseWidget, IBMMainWindow::DefaultResources);
    this->MainWindow::installDefaultResources(baseWidget);
}

void IBMMainWindow::initialize()
{

    this->MainWindow::initialize();
    if (this->main)
	this->installComponentHelpCallback(this->main);

    if(theApplication->getIconPixmap() != XtUnspecifiedPixmap)
	XtVaSetValues(this->getRootWidget(), 
	    XmNiconPixmap, theApplication->getIconPixmap(), 
	    NULL);
}

void IBMMainWindow::postWizard()
{
    if (theIBMApplication->inWizardMode() == FALSE) return;
    if ((this->wizardDialog) && (this->wizardDialog->isManaged())) return ;

    if (theIBMApplication->isWizardWindow(this->UIComponent::name)) {
	if (this->wizardDialog == NUL(WizardDialog*))
	    this->wizardDialog =
		new WizardDialog (this->getRootWidget(), this->UIComponent::name);
	if (this->wizardDialog->getText())
	    this->wizardDialog->post();
    }
}

void IBMMainWindow::manage()
{
    this->MainWindow::manage();

    //
    // Post the wizard window.
    //
    if (theIBMApplication->inWizardMode())
	this->postWizard();
}
