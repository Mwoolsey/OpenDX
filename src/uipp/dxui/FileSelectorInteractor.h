/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#ifndef _FileSelectorInteractor_h
#define _FileSelectorInteractor_h

#include <Xm/Xm.h>

#include "ValueInteractor.h"

//
// Class name definition:
//
#define ClassFileSelectorInteractor	"FileSelectorInteractor"

//
// XtCallbackProc (*CB), XtEventHandler (*EH) and XtActionProc (*AP)
// DialogCallback (*DCB), XtInputCallbackProc (*ICP), XtWorkProc (*WP)
// functions for this and derived classes
//
extern "C" void FileSelectorInteractor_ButtonCB(Widget, XtPointer, XtPointer);

//class InteractorNode;
class InteractorInstance;
class FileSelectorDialog;
//class Node;
//class Dialog;
//class SetAttrDialog;

//
// Virtual Interactor class definition:
//				
class FileSelectorInteractor : public ValueInteractor
{
  private:
    //
    // Private member data:
    // 
    static boolean ClassInitialized;

    //
    // Calls the virtual method this->buttonCallback() when the 
    // button is pressed.
    //
    friend void FileSelectorInteractor_ButtonCB(Widget w, XtPointer clientData, XtPointer callData);

  protected:
    //
    // Protected member data:
    //

    static String DefaultResources[]; 

    FileSelectorDialog *fileSelectorDialog;
    Widget 	fsdButton;

    //
    // Put the super class's interactive part in a form with a button that
    // opens the file selection box.
    //
    virtual Widget createInteractivePart(Widget p);

    // 
    // Perform any actions that need to be done after the parent of 
    // this->interactivePart has been managed.  This may include a call 
    // to Interactor::PassEvents().
    // 
    virtual void completeInteractivePart();

    //
    // Opens the interactors file selector dialog.
    //
    virtual void buttonCallback(Widget w, XtPointer callData);

    //
    // After calling the super class method, set the label on the 
    // FileSelectorDialog if we have one.
    //
    virtual void setLabel(const char *labelString, boolean re_layout = TRUE);

#if 0
    //
    // Handle a change in the text.
    // Do the same actions as the super class, but right justify the
    // text in the window.
    //
    void valueChangeCallback(Widget w, XtPointer cb);
#endif

  public:
    //
    // Constructor:
    //
    FileSelectorInteractor(const char * name, InteractorInstance *ii);

    //
    // Destructor:
    //
    ~FileSelectorInteractor(); 

    //
    // Build the interactor class and widget tree.
    //
    static Interactor *AllocateInteractor(const char *name,
					InteractorInstance *ii);

    //
    // Call the superclass method and then update the FileSelectorDialog's
    // title. 
    //
    virtual void handleInteractivePartStateChange(
				InteractorInstance *src_ii,
				boolean major_change);

    //
    // Update the text value from the instance's notion of the current text.
    // This is the same as ValueInteractor except that we strip of the
    // double quotes and right justify the text.
    //
    virtual void updateDisplayedInteractorValue();


    //
    // One time initialize for the class. 
    // 
    virtual void initialize();

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassFileSelectorInteractor;
    }
};


#endif // _FileSelectorInteractor_h
