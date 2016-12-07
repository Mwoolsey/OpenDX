/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _ValueListInteractor_h
#define _ValueListInteractor_h

#include <Xm/Xm.h>

#include "ValueInteractor.h"
#include "ListEditor.h"

//
// Class name definition:
//
#define ClassValueListInteractor	"ValueListInteractor"

class InteractorNode;
class InteractorInstance;
class Node;
class Network;
class ControlPanel;
class Dialog;
class SetAttrDialog;

//
// Virtual Interactor class definition:
//				
class ValueListInteractor : public ValueInteractor, public ListEditor
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

    Widget 	listForm;

    //
    // Build the stepper, dial, ... widget tree and set any information
    // that is specific to the derived class.  Passes back an unmanaged widget 
    // that is put in the frame created by this->createFrame().
    //
    virtual Widget createInteractivePart(Widget p);

    // 
    // Perform any actions that need to be done after the parent of 
    // this->interactivePart has been managed.  This may include a call 
    // to Interactor::PassEvents().
    // 
    virtual void completeInteractivePart();


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
    // The returned string must be freed by the caller.
    // Returns NULL if the value in the value editor is not valid and
    // can issue an error message in that case.
    //
    virtual char *getValueEditorValue();

    //
    // Before calling the superclass method, we check and massage the entered 
    // value to make sure it matches the output type for this module.
    //
    void valueEditorCallback(Widget w, XtPointer cb);

    //
    // After calling the superclass method to update the list, we set the
    // current output to a DX style list containing the listed items.
    //
    void addCallback(Widget w, XtPointer cb);
    void deleteCallback(Widget w, XtPointer cb);
    //
    // Read the displayed values and set the output from them.
    //
    boolean updateOutputFromDisplay();

    //
    // Determine if the text in the ValueEditor can be added to the current
    // list of items.   If the current list is size is 1 and the item is
    // selected, then the text can match any of the outputs, otherwise it
    // must match the item type of current list of items.
    // Returns NULL if it does not type match, otherwise a string that must
    // be freed by the caller.  The returned string may be different from the
    // valueEditor text if the text was coerced to the expected type.
    // An error message is issued if the text is not valid.
    //
    char *verifyValueEditorText();


  public:
    //
    // Constructor:
    //
    ValueListInteractor(const char * name, InteractorInstance *ii);

    //
    // Destructor:
    //
    ~ValueListInteractor(); 


    //
    // Allocate the interactor class and widget tree.
    //
    static Interactor *AllocateInteractor(const char *name,
					InteractorInstance *ii);

    virtual void handleInteractivePartStateChange(
				InteractorInstance *src_ii,
				boolean major_change);

    //
    // Update the display values for an interactor; 
    // Called when an InteractorNode does a this->setOutputValue().
    // 
    virtual void updateDisplayedInteractorValue(void);

	
    //
    // One time initialize for the class. 
    // 
    virtual void initialize();

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassValueListInteractor;
    }
};


#endif // _ValueListInteractor_h
