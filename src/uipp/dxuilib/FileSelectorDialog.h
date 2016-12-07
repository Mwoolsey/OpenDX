/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"





#ifndef _FileSelectorDialog_h
#define _FileSelectorDialog_h


#include "ApplyFileDialog.h"


//
// Class name definition:
//
#define ClassFileSelectorDialog	"FileSelectorDialog"

//
// XtCallbackProc (*CB), XtEventHandler (*EH) and XtActionProc (*AP)
// DialogCallback (*DCB), XtInputCallbackProc (*ICP), XtWorkProc (*WP)
// functions for this and derived classes
//
extern "C" void FileSelectorDialog_FilterChangeCB(Widget, XtPointer, XtPointer);

class FileSelectorInstance;

//
// FileSelectorDialog class definition:
//				
class FileSelectorDialog : public ApplyFileDialog
{
    static boolean ClassInitialized;
    static String  DefaultResources[];

  protected:

    FileSelectorInstance *fsInstance;

    //
    // Capture and save a filter change with the instance.
    //
    friend void FileSelectorDialog_FilterChangeCB(Widget, 
						XtPointer, XtPointer);
    void filterChangeCallback();

    //
    // Call super class and then set the dialog title and the dirMask resource. 
    //
    virtual Widget createDialog(Widget parent);

    //
    // Set the output value of  the interactor to the given string.
    //
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
    FileSelectorDialog(Widget        parent, FileSelectorInstance *fsi);

    //
    // Destructor:
    //
    ~FileSelectorDialog(){}

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassFileSelectorDialog;
    }
};

#endif // _FileSelectorDialog_h
