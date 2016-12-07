/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _SliderInteractor_h
#define _SliderInteractor_h


#include <X11/Intrinsic.h>
 
#include "ScalarInteractor.h"
#include "List.h"


//
// Class name definition:
//
#define ClassSliderInteractor	"SliderInteractor"

//
// XtCallbackProc (*CB), XtEventHandler (*EH) and XtActionProc (*AP)
// DialogCallback (*DCB), XtInputCallbackProc (*ICP), XtWorkProc (*WP)
// functions for this and derived classes
//
extern "C" void SliderInteractor_SliderCB(Widget, XtPointer, XtPointer);


class InteractorInstance;


//
// SliderInteractor class definition:
//				
class SliderInteractor : public ScalarInteractor
{
  private:
    //
    // Private member data:
    //

    //
    // One widget for each component (scalar or n-vector).
    //
    Widget	componentForm;	// Holds the steppers for a vector interactor
    Widget	sliderWidget;

    static boolean SliderInteractorClassInitialized;

    // Call the virtual callback for value changes. 
    //
    friend void SliderInteractor_SliderCB(Widget                  widget,
               			XtPointer                clientData,
               			XtPointer                callData);

  protected:
    //
    // Protected member data:
    //

    static String DefaultResources[];

    //
    // Accepts value changes and reflects them into other interactors, cdbs
    // and off course the interactor node output.
    //
    void sliderCallback(Widget widget, int component, 
						XtPointer clientData);

    //
    // Create and complete the steppers 
    //
    virtual Widget createInteractivePart(Widget p);
    virtual void   completeInteractivePart();

    //
    // Update the displayed values for the slider. 
    // Does the work on behalf of updateDisplayedInteractorValue().
    //
    void updateSliderValue();


  public:
    //
    // Allocate an interactor for the given instance.
    //
    static Interactor *AllocateInteractor(const char *name,
					InteractorInstance *ii);

    //
    // Constructor:
    //
    SliderInteractor(const char *name, InteractorInstance *ii) ;

    //
    // Destructor:
    //
    ~SliderInteractor(){}

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
    void initialize();

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassSliderInteractor;
    }
};

#endif // _SliderInteractor_h
