/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _ToggleAttrDialog_h
#define _ToggleAttrDialog_h



#include "SetAttrDialog.h"


//
// Class name definition:
//
#define ClassToggleAttrDialog	"ToggleAttrDialog"

//
// Referenced Classes
//
class InteractorInstance;
class ComponentAttributes;
class LocalAttributes;
class ToggleInstance;

typedef long Type;	// From DXType.h

//
// ToggleAttrDialog class definition:
//				
class ToggleAttrDialog : public SetAttrDialog
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

    Widget	setText, resetText;
    
    //
    // The current type of the values in the value list. 
    //
    Type	valueType;	

    //
    // Build the interactive set attributes widgets that sit in the dialog.
    //
    virtual void createAttributesPart(Widget parentDialog);

    //
    // Desensitize the data-driven attributes in the dialog.
    //
    virtual void setAttributeSensitivity();

    virtual void loadAttributes();
    virtual boolean storeAttributes();
    virtual void updateDisplayedAttributes();

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
    ToggleAttrDialog(Widget parent, const char *title, ToggleInstance *si);

    //
    // Destructor:
    //
    ~ToggleAttrDialog();

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassToggleAttrDialog;
    }
};


#endif // _ToggleAttrDialog_h
