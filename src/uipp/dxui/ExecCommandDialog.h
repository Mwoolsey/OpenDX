/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#ifndef _ExecCommandDialog_h
#define _ExecCommandDialog_h


#include "SetNameDialog.h"


//
// Class name definition:
//
#define ClassExecCommandDialog	"ExecCommandDialog"

//
// ExecCommandDialog class definition:
//				
class ExecCommandDialog : public SetNameDialog
{
  private:
    //
    // Private member data:
    //
    static boolean 	ClassInitialized;

  protected:
    //
    // Protected member data:
    //
    char * lastCommand;

    static String DefaultResources[];

    void saveLastCommand(const char*);
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
    ExecCommandDialog(Widget parent);

    //
    // Destructor:
    //
    ~ExecCommandDialog();

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassExecCommandDialog;
    }
};


#endif // _ExecCommandDialog_h
