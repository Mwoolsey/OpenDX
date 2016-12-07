/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#ifndef _SetNetworkCommentDialog_h
#define _SetNetworkCommentDialog_h


#include "TextEditDialog.h"

//
// Class name definition:
//
#define ClassSetNetworkCommentDialog	"SetNetworkCommentDialog"

class Network;

//
// SetNetworkCommentDialog class definition:
//				

class SetNetworkCommentDialog : public TextEditDialog
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

    Network		*network;
    //
    // Get the the text that is to be installed in the text window.
    //
    virtual const char *getText();

    //
    // Save the text in the text window 
    //
    virtual boolean saveText(const char *);

    //
    // Constructor (for derived classes):
    //
    SetNetworkCommentDialog(const char *name,
			Widget parent, boolean readonly, Network *n);

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
    SetNetworkCommentDialog(Widget parent, boolean readonly, Network *n);

    //
    // Destructor:
    //
    ~SetNetworkCommentDialog();


    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassSetNetworkCommentDialog;
    }
};


#endif // _SetNetworkCommentDialog_h
