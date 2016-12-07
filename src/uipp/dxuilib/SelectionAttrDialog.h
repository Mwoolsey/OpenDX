/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#ifndef _SelectionAttrDialog_h
#define _SelectionAttrDialog_h



#include "SetAttrDialog.h"
#include "ListEditor.h"


//
// Class name definition:
//
#define ClassSelectionAttrDialog	"SelectionAttrDialog"

//
// Referenced Classes
//
class InteractorInstance;
class ComponentAttributes;
class LocalAttributes;
class SelectionInstance;

typedef long Type;	// From DXType.h

//
// SelectionAttrDialog class definition:
//				
class SelectionAttrDialog : protected ListEditor, public SetAttrDialog
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
    static String DefaultResources[];

    Widget	valueText, labelText;

    //
    // The current type of the values in the value list. 
    //
    Type	valueType;	

    //
    // Build the interactive set attributes widgets that sit in the dialog.
    //
    virtual void createAttributesPart(Widget parentDialog);

    //
    // Build the interacive value editing part.
    // This probably called by createEditingPart().
    //
    virtual Widget createValueEditor(Widget mainForm);

    //
    // Called by this->listCallback() when a new item is selected in the list.
    // This should update the the values displayed in the editor to match
    // the values selected (i.e. listItem).
    //
    virtual void updateValueEditor(const char *listItem);

    //
    // Get the value in the value editor that should be added to the list.
    // Return T/F if value should be added.
    // The returned string must be freed by the caller.
    // Upon return FALSE, the value is not created.
    //
    virtual char * getValueEditorValue();

    //
    // Desensitize the data-driven attributes in the dialog.
    //
    virtual void setAttributeSensitivity();


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
    SelectionAttrDialog(Widget parent, const char *title, 
				SelectionInstance *si);

    //
    // Destructor:
    //
    ~SelectionAttrDialog();

    virtual void loadAttributes();
    virtual boolean storeAttributes();
    virtual void updateDisplayedAttributes();

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassSelectionAttrDialog;
    }
};


#endif // _SelectionAttrDialog_h
