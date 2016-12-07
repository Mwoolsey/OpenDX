/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _IBMMainWindow_h
#define _IBMMainWindow_h


#include <Xm/Xm.h>
#include "MainWindow.h"


//
// Class name definition:
//
#define ClassIBMMainWindow	"IBMMainWindow"

//
// XtCallbackProc (*CB), XtEventHandler (*EH) and XtActionProc (*AP)
// DialogCallback (*DCB) functions for this and derived classes
//
extern "C" void IBMMainWindow_HelpCB(Widget, XtPointer, XtPointer);

class Command;
class CommandInterface;
class WizardDialog;

#if 0
//
// Customized help callback reason:
//
#define DxCR_HELP	9001
#endif


//
// IBMMainWindow class definition:
//				
class IBMMainWindow : public MainWindow 
{
  private:
    //
    // XmNhelpCallback callback routine for this class:
    //
    static boolean ClassInitialized;

    Command	     	*helpOnContextCmd;
    Command		*helpOnWindowCmd;
    Command		*helpAboutAppCmd;
    Command		*helpTechSupportCmd;
    CommandInterface 	*onContextOption;
    CommandInterface 	*onHelpOption;
    CommandInterface 	*onManualOption;
    CommandInterface 	*onWindowOption;
    CommandInterface 	*aboutAppOption;
    CommandInterface 	*techSupportOption;

#if 0
    friend void IBMMainWindow_HelpCB(Widget    widget,
			     XtPointer clientData,
			     XtPointer callData);
#endif

  protected:
    //
    // These resources are expected to be loaded by the derived classes.
    //
    static String DefaultResources[];

    //
    // Protected member data:
    //
    Widget 		helpMenu;
    Widget 		helpMenuPulldown;


    //
    // Create the help pulldown and add the standard menu options.
    //
    virtual void createBaseHelpMenu(Widget parent, 
				boolean add_standard_options = TRUE,
				boolean addAboutApp = FALSE);
    //
    // Constructor for the subclasses:
    //
    IBMMainWindow(const char* name, boolean hasMenuBar = TRUE);

    //
    // Install the default resources for this class and then call the
    // same super class method to get the default resources from the
    // super classes.
    //
    virtual void installDefaultResources(Widget baseWidget);

    WizardDialog* wizardDialog;
    virtual void postWizard();

  public:

    //
    // Destructor:
    //
    ~IBMMainWindow();

   
    //
    // Call the super-class method and then install the help callback on 
    // the main window. 
    //
    virtual void initialize();

    //
    // ...also posts the wizard if necesary
    //
    virtual void manage();

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassIBMMainWindow;
    }
};


#endif // _IBMMainWindow_h
