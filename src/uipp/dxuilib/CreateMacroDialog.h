/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _CreateMacroDialog_h
#define _CreateMacroDialog_h


#include "Dialog.h"
#include "SetMacroNameDialog.h"
#include "TextFile.h"

//
// Class name definition:
//
#define ClassCreateMacroDialog	"CreateMacroDialog"

class EditorWindow;

extern "C" void CreateMacroDialog_fsdButtonCB(Widget w,
					   XtPointer clientData,
					   XtPointer callData);

//
// CreateMacroDialog class definition:
//				
class CreateMacroDialog : public SetMacroNameDialog 
{
  private:
    //
    // Private member data:
    //
    static boolean ClassInitialized;
    static void ConfirmationCancel(void *data);
    static void ConfirmationOk(void *data);
    EditorWindow *editor;

  protected:
    //
    // Protected member data:
    //
    static String DefaultResources[];
    
    Widget filename;
    TextFile *textFile;

    virtual boolean okCallback(Dialog *d);
    
    virtual boolean createMacro();

    virtual Widget createDialog(Widget parent);

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
    CreateMacroDialog(Widget parent, EditorWindow *e);

    //
    // Destructor:
    //
    ~CreateMacroDialog();
	
    //
    // Set method to be called be FileSelector
    //
    virtual void setFileName(const char *filename);

    // 
    // Manage the Dialog
    //

    virtual void manage();

    //
    // Returns a pointer to the class name.
    //
    virtual const char* getClassName()
    {
	return ClassCreateMacroDialog;
    }
};


#endif // _CreateMacroDialog_h




