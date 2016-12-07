/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"





#ifndef _OpenNetworkDialog_h
#define _OpenNetworkDialog_h


#include "FileDialog.h"
#include "Application.h"


//
// Class name definition:
//
#define ClassOpenNetworkDialog	"OpenNetworkDialog"

class Dialog;
class TextEditDialog;

//
// OpenNetworkDialog class definition:
//				
class OpenNetworkDialog : public FileDialog
{
    static boolean ClassInitialized;

  protected:

    static String  DefaultResources[];

    TextEditDialog   *netCommentDialog;
	
    virtual void okFileWork(const char *string);
    virtual void helpCallback(Dialog* dialog);

    //
    // For sub-classes of this dialog
    //
    OpenNetworkDialog(const char*   name, Widget        parent);

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
    OpenNetworkDialog(Widget        parent);


    //
    // Destructor:
    //
    ~OpenNetworkDialog();

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassOpenNetworkDialog;
    }
};


#endif // _OpenNetworkDialog_h
