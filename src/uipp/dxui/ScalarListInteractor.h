/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#ifndef _ScalarListInteractor_h
#define _ScalarListInteractor_h


#include <X11/Intrinsic.h>
 
#include "StepperInteractor.h"


//
// Class name definition:
//
#define ClassScalarListInteractor	"ScalarListInteractor"

//
// XtCallbackProc (*CB), XtEventHandler (*EH) and XtActionProc (*AP)
// DialogCallback (*DCB), XtInputCallbackProc (*ICP), XtWorkProc (*WP)
// functions for this and derived classes
//
extern "C" void ScalarListInteractor_ValueCB(Widget, XtPointer, XtPointer);
extern "C" void ScalarListInteractor_DeleteCB(Widget, XtPointer, XtPointer);
extern "C" void ScalarListInteractor_AddCB(Widget, XtPointer, XtPointer);
extern "C" void ScalarListInteractor_ListCB(Widget, XtPointer, XtPointer);


class InteractorInstance;


//
// ScalarListInteractor class definition:
//				
class ScalarListInteractor : public StepperInteractor
{
  private:
    //
    // Private member data:
    //

    static boolean ScalarListInteractorClassInitialized;


    friend void ScalarListInteractor_ListCB(Widget w, XtPointer clientData, XtPointer callData);
    friend void ScalarListInteractor_AddCB(Widget w, XtPointer clientData, XtPointer callData);
    friend void ScalarListInteractor_DeleteCB(Widget w, XtPointer clientData, XtPointer callData);
    friend void ScalarListInteractor_ValueCB(Widget w, XtPointer clientData, XtPointer callData);

  protected:
    //
    // Protected member data:
    //
    int	selectedItemIndex;
    Widget listForm;
    Widget listFrame;
    Widget valueList;
    Widget addButton, deleteButton;
    Widget steppers;

    static String DefaultResources[];

    //
    // We don't care if a the stepper value has changed until, the add or delete
    // button is hit. 
    //
    virtual void stepperCallback(Widget widget, int component, 
							XtPointer callData);
    //
    // Create a numeric list instead of a text list.
    //
    Widget createList(Widget     parent);
    void createListEditor(Widget mainForm);

#if 0
    //
    // Build the interactive value editing part (i.e. the stepper).  
    //
    virtual void createValueEditor(Widget mainForm);
#endif

    //
    //  Create and complete the stepper and list
    //
    Widget createInteractivePart(Widget p);
    void   completeInteractivePart();


    void addCallback(Widget w, XtPointer callData);
    void deleteCallback(Widget w, XtPointer callData);
    virtual void listCallback(Widget w, XtPointer callData);

    //
    // Build a DX style list of  the items in the list.
    // Returns a string representing the list of items; the string must be
    // freed by the caller.
    // If the list is empty, the string "NULL" is returned.
    //
    char *buildDXList();

  public:
    //
    // Allocate an n-dimensional stepper for the given instance and the list.
    //
    static Interactor *AllocateInteractor(const char *name,
				InteractorInstance *ii);

    //
    // Don't let superclass reset XmNalignment on the steppers
    //
    virtual void layoutInteractorHorizontally() 
	{ this->Interactor::layoutInteractorHorizontally(); }
    virtual void layoutInteractorVertically()
	{ this->Interactor::layoutInteractorVertically(); }

    //
    // Constructor:
    //
    ScalarListInteractor(const char *name, InteractorInstance *ii);

    //
    // Destructor:
    //
    ~ScalarListInteractor(){}

    //
    // Update the displayed values for this interactor.
    //
    virtual void updateDisplayedInteractorValue(void);

    //
    // Make sure the resources (including the value) match the attributes
    // for the widgets.
    //
    virtual void handleInteractivePartStateChange(
					InteractorInstance *src_ii,
					boolean major_change);
 
    //
    // One time initialize for the class.
    //
    virtual void initialize();

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassScalarListInteractor;
    }
};


#endif // _ScalarListInteractor_h
