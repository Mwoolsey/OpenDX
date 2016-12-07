/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _SelectorInteractor_h
#define _SelectorInteractor_h


#include <X11/Intrinsic.h>
 
#include "Interactor.h"
#include "List.h"


//
// Class name definition:
//
#define ClassSelectorInteractor	"SelectorInteractor"

//
// XtCallbackProc (*CB), XtEventHandler (*EH) and XtActionProc (*AP)
// DialogCallback (*DCB), XtInputCallbackProc (*ICP), XtWorkProc (*WP)
// functions for this and derived classes
//
extern "C" void SelectorInteractor_OptionMenuCB(Widget, XtPointer, XtPointer);


//
// SelectorInteractor class definition:
//				
class SelectorInteractor : public Interactor
{
  private:
    //
    // Private member data:
    //

    //
    // FIXME:  the data item I really want is WorkSpaceComponent::visualsInited
    // which is private.  It should just be made protected.  
    // SelectorInteractor::setAppearance needs to check its value.
    //
    boolean visinit;

  protected:
    //
    // Protected member data:
    //

    //
    // One widget for each component (scalar or n-vector).
    //
    // Use a form around the menu so we can easily destory/rebuild the menu
    Widget      optionForm;
    Widget	optionMenu;
    Widget	pulldown;
    List	optionWidgets;

    static boolean SelectorInteractorClassInitialized;

    static String DefaultResources[];

    friend void SelectorInteractor_OptionMenuCB(Widget widget, XtPointer clientData, 
				     XtPointer callData);

    //
    // Get the nth widget. 
    //
    Widget getOptionWidget(int i) 
		{ ASSERT(i>0); 
		  return (Widget)this->optionWidgets.getElement(i); 
		}
    boolean appendOptionWidget(Widget w) 
		{ return this->optionWidgets.appendElement((const void*) w); }

    //
    // [Re]load the options into this->pulldown.
    //
    void reloadMenuOptions();

  public:
    //
    // Allocate an interactor. 
    //
    static Interactor *AllocateInteractor(const char *name,
					InteractorInstance *ii);


    //
    // Accepts value changes and reflects them into other interactors, cdbs
    // and off course the interactor node output.
    //
    virtual void optionMenuCallback(Widget w, int optnum, XtPointer cb);

    //
    // Constructor:
    //
    SelectorInteractor(const char *name, InteractorInstance *ii);

    //
    // Destructor:
    //
    ~SelectorInteractor(){}

    //
    // Update the displayed values for this interactor.
    //
    void updateDisplayedInteractorValue(void);

    //
    // 
    //
    Widget createInteractivePart(Widget p);
    void   completeInteractivePart();

    // In addition to the superclass' method set {top,bottom}Offset of
    // the option menu.  It makes the option menu appear more balanced
    // against the label widget.
    virtual void layoutInteractorHorizontally();
    virtual void layoutInteractorVertically();


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
    // CDE Motif use enableEtchedInMenu and when it sets up a
    // session for a new user, it adds this resource set to true to .Xdefaults.
    // As a result options menus are created using gadgets and it's impossible
    // to change the background color of a gadget since it has no XmNbackground.
    // That breaks switching dialog/user style.  So when setAppearance happens,
    // we'll recreate the menu so that we also get the proper color.
    //
    virtual void setAppearance (boolean developerStyle);

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassSelectorInteractor;
    }
};


#endif // _SelectorInteractor_h
