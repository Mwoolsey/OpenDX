/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>



#ifndef _ImageFileDialog_h
#define _ImageFileDialog_h


#include "FileDialog.h"

class SaveImageDialog;

//
// Class name definition:
//
#define ClassImageFileDialog	"ImageFileDialog"

//
// ImageFileDialog class definition:
//				

class ImageFileDialog : public FileDialog
{
  private:
    //
    // Private member data:
    //
    static boolean ClassInitialized;

    SaveImageDialog* sid;

  protected:
    //
    // Protected member data:
    //
    static String  DefaultResources[];

    virtual void okFileWork(const char *string);
    virtual void cancelCallback(Dialog* );

    //
    // Install the default resources for this class and then call the
    // same super class method to get the default resources from the
    // super classes.
    //
    virtual void installDefaultResources(Widget baseWidget);

    virtual Widget createDialog (Widget parent);

  public:

    //
    // Constructor: protected since this is an abstract class
    //
    ImageFileDialog(Widget parent, SaveImageDialog *sid);

    //
    // Destructor:
    //
    ~ImageFileDialog(){};

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassImageFileDialog;
    }
};


#endif // _ImageFileDialog_h
