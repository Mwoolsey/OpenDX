/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



// SetAttrDialog.h -						    	    //
//                                                                          //
// Definition for the Virtual SetAttrDialog class.			    //
// 
// The SetAttrDialog class implements dialogs that are used to
// set the attributes (very generally speaking) of an InteractorInstance
// or InteractorNode and therefore an Interactor.  Each Interactor class
// defines a newSetAttrDialog() method that creates a SetAttrDialog that
// is compatible with the given Interactor.  The derived classes must
// implement this->createDialog() which is pure virtual in the
// ancestor class.  The derived class must also implement this->loadAttributes()
// which reads the attributes from this->interactorInstance and displays
// them on the already created dialog.  this->storeAttributes stores 
// the attributes represented in the dialog back with this->interactorInstance.
// this->storeAttributes is called in this->okCallback() which is called when
// the 'send attributes' condition is met (typically this is the ok button). 
// After the okCallback() calls this->storeAttributes(), it asks the 
// InteractorNode associated with this->interactorInstance to notify all
// its InteractorInstances that the attributes have changed.
// 
//                                                                          //

#ifndef _SetAttrDialog_h
#define _SetAttrDialog_h


#include "Dialog.h"


//
// Class name definition:
//
#define ClassSetAttrDialog	"SetAttrDialog"

//
// Referenced Classes
//
class InteractorInstance;
class LocalAttributes;
class ScalarInstance;



//
// SetAttrDialog class definition:
//				
class SetAttrDialog : public Dialog
{
  private:
    //
    // Private member data:
    //


  protected:
    //
    // Protected member data:
    //

    InteractorInstance	*interactorInstance;
    char 		*title;

    //
    // Build the widget tree. 
    //
    virtual Widget createDialog(Widget parent);

    //
    // Build the interactive set attributes widgets that sit in the dialog. 
    // Called by createDialog()
    //
    virtual void createAttributesPart(Widget parentDialog) = 0;

    //
    // Apply the changed attributes 
    //
    virtual boolean okCallback(Dialog *clientData); 

    //
    // Reject the changed attributes, generally this just returns 
    //
    virtual void cancelCallback(Dialog *clientData); 

    //
    // Load the dialog with the attributes from the given interactor instance.
    //
    virtual void loadAttributes() = 0;

    //
    // Store the attributes in the dialog into the given interactor instance.
    //
    virtual boolean storeAttributes() = 0;

    //
    // Update the displayed attributes. 
    //
    virtual void updateDisplayedAttributes() = 0;

    //
    // Set the sensitivity of the displayed attributes.  This is a 
    // chained method.  
    //
    virtual void setAttributeSensitivity();

    //
    // Constructor (for derived classes only):
    //
    SetAttrDialog(const char *name, Widget parent, 
		  const char *title, InteractorInstance *ii); 

  public:

    //
    // Destructor:
    //
    ~SetAttrDialog(); 

    //
    // load and update the displayed attributes
    //
    void refreshDisplayedAttributes();

    //
    // Redisplay the attributes and then call parent's manage(). 
    //
    virtual void manage();

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassSetAttrDialog;
    }
};


#endif // _SetAttrDialog_h
