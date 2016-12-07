/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"





#ifndef _LoadMDFDialog_h
#define _LoadMDFDialog_h


#include "FileDialog.h"


//
// Class name definition:
//
#define ClassLoadMDFDialog	"LoadMDFDialog"

class DXApplication;

//
// LoadMDFDialog class definition:
//				
class LoadMDFDialog : public FileDialog
{
    static boolean ClassInitialized;
    static String  DefaultResources[];

  protected:
    virtual void okFileWork(const char *string);
    virtual void helpCallback(Dialog* dialog);
    DXApplication *dxApp;

    //
    // Constructor: For derived classes.
    //
    LoadMDFDialog(char *name, Widget        parent, DXApplication *app);

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
    LoadMDFDialog(Widget        parent, DXApplication *app);


    //
    // Destructor:
    //
    ~LoadMDFDialog(){}

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassLoadMDFDialog;
    }
};


#endif // _LoadMDFDialog_h
