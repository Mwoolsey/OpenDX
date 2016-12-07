/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _SetImageNameDialog_h
#define _SetImageNameDialog_h


#include "SetNameDialog.h"


//
// Class name definition:
//
#define ClassSetImageNameDialog	"SetImageNameDialog"

class ImageWindow;

//
// SetImageNameDialog class definition:
//				
class SetImageNameDialog : public SetNameDialog
{
  private:
    //
    // Private member data:
    //
    ImageWindow		*imageWindow;
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
    SetImageNameDialog(ImageWindow *iw);

    //
    // Destructor:
    //
    ~SetImageNameDialog();

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassSetImageNameDialog;
    }
};


#endif // _SetImageNameDialog_h
