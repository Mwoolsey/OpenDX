/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _SetVectorAttrDialog_h
#define _SetVectorAttrDialog_h


#include "SetScalarAttrDialog.h"


//
// Class name definition:
//
#define ClassSetVectorAttrDialog	"SetVectorAttrDialog"

//
// XtCallbackProc (*CB), XtEventHandler (*EH) and XtActionProc (*AP)
// DialogCallback (*DCB), XtInputCallbackProc (*ICP), XtWorkProc (*WP)
// functions for this and derived classes
//
extern "C" void SetVectorAttrDialog_ComponentOptionCB(Widget, XtPointer, XtPointer);
extern "C" void SetVectorAttrDialog_StepperCB(Widget, XtPointer, XtPointer);


//
// SetVectorAttrDialog class definition:
//				
class SetVectorAttrDialog : public SetScalarAttrDialog
{
  private:
    //
    // Private member data:
    //
    friend void SetVectorAttrDialog_StepperCB(Widget  widget, 
			XtPointer clientData, 
			XtPointer callData);
    friend void SetVectorAttrDialog_ComponentOptionCB(Widget  widget, 
			XtPointer clientData, 
			XtPointer callData);

    static boolean ClassInitialized;

    Widget createComponentPulldown(Widget parent, const char *name);

  protected:
    //
    // Protected member data:
    //
    static String DefaultResources[];

    Widget	allComponents;
    Widget	selectedComponent;
    Widget 	componentOptions; 
    Widget 	componentStepper; 

    //
    // Build the interactive set attributes widgets that sit in the dialog.
    //
    virtual void createAttributesPart(Widget parentDialog);

    // 
    // Get the current component number.
    // 
    virtual int getCurrentComponentNumber();

    // 
    // Get whether or not the which component option is set. 
    // Return TRUE if set to 'All Components' 
    // 
    boolean isAllAttributesMode();

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
    SetVectorAttrDialog(Widget parent, const char *title, ScalarInstance *si); 


    //
    // Destructor:
    //
    ~SetVectorAttrDialog();

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassSetVectorAttrDialog;
    }
};


#endif // _SetVectorAttrDialog_h
