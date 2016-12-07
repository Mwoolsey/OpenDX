/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"





#ifndef _SetColormapNameDialog_h
#define _SetColormapNameDialog_h


#include "SetNameDialog.h"


//
// Class name definition:
//
#define ClassSetColormapNameDialog	"SetColormapNameDialog"

class ColormapNode;

//
// SetColormapNameDialog class definition:
//				
class SetColormapNameDialog : public SetNameDialog
{
  private:
    //
    // Private member data:
    //
    ColormapNode 	*colormapNode;
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
    SetColormapNameDialog(Widget parent, ColormapNode *cn);

    //
    // Destructor:
    //
    ~SetColormapNameDialog();

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassSetColormapNameDialog;
    }
};


#endif // _SetColormapNameDialog_h
