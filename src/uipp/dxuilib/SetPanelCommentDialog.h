/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#ifndef _SetPanelCommentDialog_h
#define _SetPanelCommentDialog_h


#include "TextEditDialog.h"

//
// Class name definition:
//
#define ClassSetPanelCommentDialog	"SetPanelCommentDialog"

class ControlPanel;

//
// SetPanelCommentDialog class definition:
//				

class SetPanelCommentDialog : public TextEditDialog
{
  private:
    //
    // Private member data:
    //
    static boolean ClassInitialized;

  protected:
    //
    // Protected member data:
    //
    static String  DefaultResources[];

    ControlPanel 	*controlPanel;
    //
    // Get the the text that is to be installed in the text window.
    //
    virtual const char *getText();

    //
    // Save the text in the text window 
    //
    virtual boolean saveText(const char *);

    //
    // The title to be applied the newly managed dialog.
    // The returned string is freed by the caller (TextEditDialog::manage()).
    //
    virtual char *getDialogTitle();

    //
    // Constructor (for derived classes) 
    //
    SetPanelCommentDialog(const char *name,
			Widget parent, boolean readonly, ControlPanel *cp);

    //
    // Install the default resources for this class and then call the
    // same super class method to get the default resources from the
    // super classes.
    //
    virtual void installDefaultResources(Widget baseWidget);

  public:

    //
    // Constructor (for instances of THIS class):
    //
    SetPanelCommentDialog(Widget parent, boolean readonly, ControlPanel *cp);

    //
    // Destructor:
    //
    ~SetPanelCommentDialog();


    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassSetPanelCommentDialog;
    }
};


#endif // _SetPanelCommentDialog_h
