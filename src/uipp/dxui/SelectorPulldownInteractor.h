/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _SelectorPulldownInteractor_h
#define _SelectorPulldownInteractor_h


#include <X11/Intrinsic.h>
 
#include "Interactor.h"

class TextSelector;

//
// Class name definition:
//
#define ClassSelectorPulldownInteractor	"SelectorPulldownInteractor"

extern "C" void SelectorPulldownInteractor_SelectionCB(Widget, XtPointer, XtPointer);


//
// SelectorPulldownInteractor class definition:
//				
class SelectorPulldownInteractor : public Interactor
{
  private:
    //
    // Private member data:
    //
    static boolean ClassInitialized;

    static String DefaultResources[];

    TextSelector *text_selector;

  protected:
    //
    // Protected member data:
    //

    friend void SelectorPulldownInteractor_SelectionCB
	(Widget widget, XtPointer clientData, XtPointer callData);

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
    // Constructor:
    //
    SelectorPulldownInteractor(const char *name, InteractorInstance *ii);

    //
    // Destructor:
    //
    ~SelectorPulldownInteractor(){}

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
    // Flatten out the parent change the color, and turn off selectibility.
    //
    virtual void setAppearance(boolean developerStyle);


    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassSelectorPulldownInteractor;
    }
};


#endif // _SelectorPulldownInteractor_h
