/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _SelectorRadioInteractor_h
#define _SelectorRadioInteractor_h


#include <X11/Intrinsic.h>
 
#include "Interactor.h"
#include "List.h"

//
// Class name definition:
//
#define ClassSelectorRadioInteractor	"SelectorRadioInteractor"

//
// XtCallbackProc (*CB), XtEventHandler (*EH) and XtActionProc (*AP)
// DialogCallback (*DCB), XtInputCallbackProc (*ICP), XtWorkProc (*WP)
// functions for this and derived classes
//
extern "C" void SelectorRadioInteractor_SelectorToggleCB(Widget, XtPointer, XtPointer);


//
// SelectorRadioInteractor class definition:
//				
class SelectorRadioInteractor : public Interactor
{
  private:
    //
    // Private member data:
    //

  protected:
    //
    // Protected member data:
    //

    //
    // One widget for each component (scalar or n-vector).
    //
    Widget      form;
    Widget      toggleRadio;
    List	toggleWidgets;

    static boolean SelectorRadioInteractorClassInitialized;

    static String DefaultResources[];

    friend void SelectorRadioInteractor_SelectorToggleCB(Widget widget, XtPointer clientData, 
				     XtPointer callData);

    //
    // Get the nth widget. 
    //
    Widget getOptionWidget(int i) 
		{ ASSERT(i>0); 
		  return (Widget)this->toggleWidgets.getElement(i); 
		}
    boolean appendOptionWidget(Widget w) 
		{ return this->toggleWidgets.appendElement((const void*) w); }

    //
    // [Re]load the options into this->pulldown.
    //
    void reloadMenuOptions();

    //
    // Layout ourself when resized
    //
    virtual void layoutInteractorHorizontally();
    virtual void layoutInteractorVertically();

  public:
    //
    // Allocate this class 
    //
    static Interactor *AllocateInteractor(const char *name,
					InteractorInstance *ii);


    //
    // Accepts value changes and reflects them into other interactors, cdbs
    // and off course the interactor node output.
    //
    virtual void toggleCallback(Widget w, int optnum, XtPointer cb);

    //
    // Constructor:
    //
    SelectorRadioInteractor(const char *name, InteractorInstance *ii);

    //
    // Destructor:
    //
    ~SelectorRadioInteractor(){}

    //
    // Update the displayed values for this interactor.
    //
    void updateDisplayedInteractorValue(void);

    //
    // 
    //
    Widget createInteractivePart(Widget p);
    void   completeInteractivePart();

    //
    // Make sure the attributes match the resources for the widgets. 
    //
    void handleInteractivePartStateChange(InteractorInstance *src_ii,
						boolean major_change);

    //
    // One time initialize for the class.
    //
    void initialize();

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassSelectorRadioInteractor;
    }
};


#endif // _SelectorRadioInteractor_h
