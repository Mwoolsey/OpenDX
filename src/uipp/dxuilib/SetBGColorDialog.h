/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _SetBGColorDialog_h
#define _SetBGColorDialog_h


#include "SetNameDialog.h"


//
// Class name definition:
//
#define ClassSetBGColorDialog	"SetBGColorDialog"

class ImageWindow;

//
// SetBGColorDialog class definition:
//				
class SetBGColorDialog : public SetNameDialog
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
    ImageWindow *imageWindow;
    static String DefaultResources[];

    virtual boolean saveText(const char *s);
    virtual const char *getText();


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
    SetBGColorDialog(ImageWindow *image);

    //
    // Destructor:
    //
    ~SetBGColorDialog();

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassSetBGColorDialog;
    }
};


#endif // _SetBGColorDialog_h
