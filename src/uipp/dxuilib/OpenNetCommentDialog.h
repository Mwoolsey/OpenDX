/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#ifndef _OpenNetCommentDialog_h
#define _OpenNetCommentDialog_h


#include "TextEditDialog.h"

//
// Class name definition:
//
#define ClassOpenNetCommentDialog	"OpenNetCommentDialog"

//
// OpenNetCommentDialog class definition:
//				

class OpenNetCommentDialog : public TextEditDialog
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

    char	*text; 

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
    OpenNetCommentDialog(Widget parent);

    //
    // Destructor:
    //
    ~OpenNetCommentDialog();


    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassOpenNetCommentDialog;
    }
};


#endif // _OpenNetCommentDialog_h
