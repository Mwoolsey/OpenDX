/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#ifndef _WizardDialog_h
#define _WizardDialog_h


#include "TextEditDialog.h"

//
// Class name definition:
//
#define ClassWizardDialog	"WizardDialog"

class List;

//
// WizardDialog class definition:
//				

class WizardDialog : public TextEditDialog
{
  private:
    //
    // Private member data:
    //
    static boolean ClassInitialized;

    char* text;
    char* parent_name;
    boolean file_read;

    Widget closeToggle;

    //
    // Keep a list of names of windows for whom we've already shown wizards so
    // that we don't have to show the same wizard over again.
    //
    static List* AlreadyWizzered;

  protected:
    //
    // Protected member data:
    //
    static String  DefaultResources[];

    //
    // Install the default resources for this class and then call the
    // same super class method to get the default resources from the
    // super classes.
    //
    virtual void installDefaultResources(Widget baseWidget);

    virtual boolean okCallback(Dialog*);

    virtual Widget createDialog(Widget parent);

  public:

    //
    // Constructor:
    //
    WizardDialog(Widget parent, const char* parent_name);

    //
    // Destructor:
    //
    ~WizardDialog();

    //
    // Read a file named after the parent widget, and supply the text.
    //
    virtual const char* getText();

    virtual void post();

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassWizardDialog;
    }
};


#endif // _WizardDialog_h
