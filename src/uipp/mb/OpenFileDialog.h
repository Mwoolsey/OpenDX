/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _OpenFileDialog_h
#define _OpenFileDialog_h


#include "FileDialog.h"
#include "MBMainWindow.h"

#include <Xm/Xm.h>

//
// Class name definition:
//
#define ClassOpenFileDialog	"OpenFileDialog"

class Dialog;
class MBMainWindow;

//
// OpenFileDialog class definition:
//				
class OpenFileDialog : public FileDialog
{
    static boolean ClassInitialized;

    MBMainWindow *mbmw;

  protected:
    static String  DefaultResources[];

    virtual void okFileWork(const char *string); 

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
    OpenFileDialog(MBMainWindow *mbmw);

    //
    // Destructor:
    //
    ~OpenFileDialog(){}

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassOpenFileDialog;
    }
};


#endif // _OpenFileDialog_h
