/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _SetInteractorNameDialog_h
#define _SetInteractorNameDialog_h


#include "SetNameDialog.h"


//
// Class name definition:
//
#define ClassSetInteractorNameDialog	"SetInteractorNameDialog"

class ControlPanel;

//
// SetInteractorNameDialog class definition:
//				
class SetInteractorNameDialog : public SetNameDialog
{
  private:
    //
    // Private member data:
    //
    static Boolean ClassInitialized;
    static String  DefaultResources[];

    ControlPanel *panel;

  protected:
    //
    // Protected member data:
    //

    virtual boolean saveText(const char *s);
    virtual const char *getText();

    //
    // Install the default resources for this class and then call the
    // same super class method to get the default resources from the
    // super classes.
    //
    virtual void installDefaultResources(Widget baseWidget);

    virtual Widget createDialog (Widget parent);

  public:
    //
    // Constructor:
    //
    SetInteractorNameDialog(ControlPanel *panel);

    //
    // Destructor:
    //
    ~SetInteractorNameDialog();

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassSetInteractorNameDialog;
    }
};


#endif // _SetInteractorNameDialog_h
