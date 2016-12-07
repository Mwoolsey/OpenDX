/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#ifndef _HelpOnNetworkDialog_h
#define _HelpOnNetworkDialog_h


#include "SetNetworkCommentDialog.h"

//
// Class name definition:
//
#define ClassHelpOnNetworkDialog	"HelpOnNetworkDialog"

class Network;

//
// HelpOnNetworkDialog class definition:
//				

class HelpOnNetworkDialog : public SetNetworkCommentDialog 
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
    HelpOnNetworkDialog(Widget parent, Network *n);

    //
    // Destructor:
    //
    ~HelpOnNetworkDialog();

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassHelpOnNetworkDialog;
    }
};


#endif // _HelpOnNetworkDialog_h
