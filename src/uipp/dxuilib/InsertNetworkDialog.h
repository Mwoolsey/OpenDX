/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#ifndef _InsertNetworkDialog_h
#define _InsertNetworkDialog_h


#include "FileDialog.h"
#include "OpenNetworkDialog.h"
#include "Application.h"


//
// Class name definition:
//
#define ClassInsertNetworkDialog	"InsertNetworkDialog"


//
// InsertNetworkDialog class definition:
//				
class InsertNetworkDialog: public OpenNetworkDialog 
{
    static boolean ClassInitialized;

  protected:

    static String  DefaultResources[];

    virtual void okFileWork(const char *string);

    //
    // For sub-classes of this dialog
    //
    InsertNetworkDialog(const char*   name, Widget        parent);

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
    InsertNetworkDialog(Widget        parent);


    //
    // Destructor:
    //
    ~InsertNetworkDialog();

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassInsertNetworkDialog;
    }
};


#endif // _InsertNetworkDialog_h
