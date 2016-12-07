/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _ValueInteractor_h
#define _ValueInteractor_h

#include <Xm/Xm.h>

#include "Interactor.h"

//
// Class name definition:
//
#define ClassValueInteractor	"ValueInteractor"

//
// XtCallbackProc (*CB), XtEventHandler (*EH) and XtActionProc (*AP)
// DialogCallback (*DCB), XtInputCallbackProc (*ICP), XtWorkProc (*WP)
// functions for this and derived classes
//
extern "C" void ValueInteractor_ValueChangeCB(Widget, XtPointer, XtPointer);

class InteractorNode;
class InteractorInstance;
class Node;
class Network;
//class ControlPanel;
//class Dialog;
//class SetAttrDialog;

//
// Virtual Interactor class definition:
//				
class ValueInteractor : public Interactor 
{
  private:
    //
    // Private member data:
    // 
    static boolean ClassInitialized;

    friend void ValueInteractor_ValueChangeCB(Widget w, XtPointer clientData,
					XtPointer callData);

  protected:
    //
    // Protected member data:
    //

    static String DefaultResources[]; 
    Widget textEditor;

    //
    // Build the stepper, dial, ... widget tree and set any information
    // that is specific to the derived class.  Passes back an unmanaged widget 
    // that is put in the frame created by this->createFrame().
    //
    virtual Widget createInteractivePart(Widget p);
    void completeInteractivePart();

    //
    // Build the text editing widget (without callbacks)
    //
    Widget createTextEditor(Widget parent);


    //
    // Handle a change in the text 
    //
    void valueChangeCallback(Widget w, XtPointer cb);

    //
    // Update the text value with the give string.
    //
    void installNewText(const char *text);

    //
    // Get the text that is currently displayed in the text window.
    // The returned string must be freed by the caller.
    //
    char *getDisplayedText();


  public:
    //
    // Constructor:
    //
    ValueInteractor(const char * name, InteractorInstance *ii);

    //
    // Destructor:
    //
    ~ValueInteractor(); 

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
	return ClassValueInteractor;
    }
};


#endif // _ValueInteractor_h
