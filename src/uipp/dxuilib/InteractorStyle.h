/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"





#ifndef _InteractorStyle_h_
#define _InteractorStyle_h_

#include "Base.h"
#include "SymbolManager.h"

enum InteractorStyleEnum {
        UnknownStyle    = 0,
        DefaultStyle    = 1,	
        StepperStyle    = 2,
        SliderStyle     = 3,
        DialStyle       = 4,
        VectorStyle     = 5,
	TextStyle	= 6,	// These three should have the same value
        ValueStyle      = 6,    // These three should have the same value
        StringStyle     = 6,    // These three should have the same value
        SelectorOptionMenuStyle   = 7,

        IntegerListEditorStyle= 8,
        ScalarListEditorStyle = 9,
        VectorListEditorStyle = 10,
	TextListEditorStyle   = 11, // These three should have the same value
        ValueListEditorStyle  = 11, // These three should have the same value
        StringListEditorStyle = 11, // These three should have the same value

        FileSelectorStyle = 12,
 
	SelectorToggleStyle = 13,	// Toggle button for Selector
	SelectorRadioStyle = 15,	// Radio buttons 

	ToggleToggleStyle = 16,		// Toggle button
	SelectorListToggleStyle = 17,	// Toggle button

	SelectorPulldownStyle = 18,	//  scrollable menu
	SelectorListStyle = 19,		// scrolled list 

        UserStyle       = 1000 
};

#define ClassInteractorStyle	"InteractorStyle"

class InteractorInstance;
class Dictionary;
class Interactor;

typedef Interactor* (*InteractorAllocator)(const char *name,
						InteractorInstance *ii);
extern void BuildtheInteractorStyleDictionary(void);

class InteractorStyle : public Base {
  private:

    InteractorStyleEnum style;
    Symbol		name;
    char		*interactorName;	// Name applied to widget
    InteractorAllocator	allocateInteractor;
    boolean		isDefault;
    char*		javaStyle;

  protected:

  public:
	
    //
    // Added supported style/name/interactorbuild group to the list
    // of supported styles for the given named interactor (node name).
    //
    static boolean AddSupportedStyle(const char *interactor,
		    InteractorStyleEnum style,
		    const char *stylename,
		    InteractorAllocator ia);
    static boolean AddSupportedStyle(const char *interactor,
		    InteractorStyleEnum style,
		    const char *stylename,
		    const char *javaStyle,
		    InteractorAllocator ia);

    //
    // Get the InteractorStyle entry associated with the give interactor
    // (node name) and the give style or style name.
    //
    static InteractorStyle *GetInteractorStyle(const char* interactor,
			    InteractorStyleEnum style);
    static InteractorStyle *GetInteractorStyle(const char* interactor,
			    const char *stylname);
    static void 	   SetDefaultStyle(const char* interactor,
			    InteractorStyleEnum style);

    //
    // Return the list of InteractorStyles for the give interactor (node name).
    //
    static Dictionary *GetInteractorStyleDictionary(const char* interactor);


    InteractorStyle(InteractorStyleEnum s, const char *n, const char* javaStyle,
				InteractorAllocator ia);

    ~InteractorStyle();

    Interactor *createInteractor(InteractorInstance *ii);

    const char* getJavaStyle() { return this->javaStyle; }
    boolean hasJavaStyle() { return (this->javaStyle != NUL(char*)); }

    const char *getNameString() 
	{ return theSymbolManager->getSymbolString(this->name); }
    InteractorStyleEnum getStyleEnum() { return this->style; }

    const char *getClassName() 
		{ return ClassInteractorStyle; }

};


#endif	// _InteractorStyle_h_
