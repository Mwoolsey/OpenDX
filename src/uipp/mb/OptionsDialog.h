/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#ifndef _OptionsDialog_h
#define _OptionsDialog_h


#include "Dialog.h"

//
// Class name definition:
//
#define ClassOptionsDialog	"OptionsDialog"

class MBMainWindow;

//
// OptionsDialog class definition:
//				

class OptionsDialog : public Dialog
{

  private:
    //
    // Private member data:
    //
    static boolean ClassInitialized;
    static String  DefaultResources[];

    MBMainWindow *mbmw;

    Widget 	mainform;
    Widget	pin_tb;
    Widget	side_effect_tb;
    Widget	asynch_tb;
    Widget	persistent_tb;
    Widget	outboard_label;
    Widget	host_label;
    Widget	host_text;

    boolean	pin;
    boolean	side_effect;
    boolean	asynchronous;
    boolean	persistent;
    char        *host;

  protected:
    //
    // Protected member data:
    //
    virtual boolean okCallback(Dialog *d);
    virtual void cancelCallback(Dialog *d);
    Widget createDialog(Widget);

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
    OptionsDialog(Widget parent, MBMainWindow*);


    //
    // Destructor:
    //
    ~OptionsDialog();

    void getValues(boolean &pin, boolean &side_effect);
    void setPin(boolean pin);
    void setSideEffect(boolean side_effect);
    void notifyOutboardStateChange(boolean outboard);

    virtual void post();
    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassOptionsDialog;
    }
};
#endif // _OptionsDialog_h
