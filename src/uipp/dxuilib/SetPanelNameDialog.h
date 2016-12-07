/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _SetPanelNameDialog_h
#define _SetPanelNameDialog_h


#include "SetNameDialog.h"


//
// Class name definition:
//
#define ClassSetPanelNameDialog	"SetPanelNameDialog"

class ControlPanel;

//
// SetPanelNameDialog class definition:
//				
class SetPanelNameDialog : public SetNameDialog
{
  private:
    //
    // Private member data:
    //
    ControlPanel	*controlPanel;
    static boolean 	ClassInitialized;

  protected:
    //
    // Protected member data:
    //

    static String DefaultResources[];

    virtual const char *getText();
    virtual boolean saveText(const char *s);

    //
    // Install the default resources for this class and then call the
    // same super class method to get the default resources from the
    // super classes.
    //
    virtual void installDefaultResources(Widget baseWidget);

  public:
    //
    // Constructor:
    //
    SetPanelNameDialog(Widget parent, ControlPanel *cp);

    //
    // Destructor:
    //
    ~SetPanelNameDialog();

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassSetPanelNameDialog;
    }
};


#endif // _SetPanelNameDialog_h
