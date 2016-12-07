/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"





#include <ctype.h>

#include "DXStrings.h"
#include "InteractorInstance.h"
#include "InteractorStyle.h"
#include "Dictionary.h"
#include "DictionaryIterator.h"
#include "Interactor.h"
#include "StepperInteractor.h"
#include "DialInteractor.h"
#include "SliderInteractor.h"
#include "ValueInteractor.h"
#include "ValueListInteractor.h"
#include "ScalarListInteractor.h"
#include "SelectorInteractor.h"
#include "FileSelectorInteractor.h"
#include "SelectorRadioInteractor.h"
//#include "SelectorPulldownInteractor.h"
#include "SelectorListInteractor.h"
#include "ToggleToggleInteractor.h"
#include "SelectorListToggleInteractor.h"
#include "ErrorDialogManager.h"


static Dictionary *theInteractorStyleDictionary =  NULL;
						

void BuildtheInteractorStyleDictionary()
{
   theInteractorStyleDictionary = new Dictionary;
 
   //
   // Supported styles for Integer, Scalar and vector interactors.
   //
   InteractorStyle::AddSupportedStyle("Integer", StepperStyle,  "Stepper",  
			       "StepperInteractor",
				StepperInteractor::AllocateInteractor);
   InteractorStyle::AddSupportedStyle("Integer", DialStyle,  "Dial",
                                DialInteractor::AllocateInteractor);
   InteractorStyle::AddSupportedStyle("Integer", SliderStyle,  "Slider",
                                SliderInteractor::AllocateInteractor);
   InteractorStyle::AddSupportedStyle("Integer", TextStyle,  "Text", "ScalarInteractor",
                                ValueInteractor::AllocateInteractor);

   InteractorStyle::AddSupportedStyle("Scalar",  StepperStyle, "Stepper", 
			       "StepperInteractor",
				StepperInteractor::AllocateInteractor);
   InteractorStyle::AddSupportedStyle("Scalar", DialStyle,  "Dial",
                                DialInteractor::AllocateInteractor);
   InteractorStyle::AddSupportedStyle("Scalar", SliderStyle,  "Slider",
                                SliderInteractor::AllocateInteractor);
   InteractorStyle::AddSupportedStyle("Scalar", TextStyle,  "Text", "ScalarInteractor",
                                ValueInteractor::AllocateInteractor);

   InteractorStyle::AddSupportedStyle("Vector",  StepperStyle, "Stepper", 
				"StepperInteractor",
				StepperInteractor::AllocateInteractor);
   InteractorStyle::AddSupportedStyle("Vector",  TextStyle, "Text",  "Interactor",
				ValueInteractor::AllocateInteractor);


   //
   // Supported styles for list interactors.
   //
   InteractorStyle::AddSupportedStyle("ScalarList",
			ScalarListEditorStyle, "List Editor", 
			ScalarListInteractor::AllocateInteractor);
   InteractorStyle::AddSupportedStyle("ScalarList",   
			TextStyle, "Text", "Interactor",
			ValueInteractor::AllocateInteractor);

   InteractorStyle::AddSupportedStyle("IntegerList",
			IntegerListEditorStyle, "List Editor", 
			ScalarListInteractor::AllocateInteractor);
   InteractorStyle::AddSupportedStyle("IntegerList",   
			TextStyle, "Text", "Interactor",
			ValueInteractor::AllocateInteractor);

   InteractorStyle::AddSupportedStyle("VectorList",
			VectorListEditorStyle, "List Editor",
			ScalarListInteractor::AllocateInteractor);
   InteractorStyle::AddSupportedStyle("VectorList",   
			TextStyle, "Text", "Interactor",
			ValueInteractor::AllocateInteractor);

   InteractorStyle::AddSupportedStyle("Value",   
			TextStyle,   "Text",    "Interactor",
			ValueInteractor::AllocateInteractor);
   InteractorStyle::AddSupportedStyle("String",  
			TextStyle,   "Text", "StringInteractor", 
			ValueInteractor::AllocateInteractor);

   InteractorStyle::AddSupportedStyle("ValueList", 
			TextListEditorStyle, "List Editor",
			ValueListInteractor::AllocateInteractor);
   InteractorStyle::AddSupportedStyle("ValueList",   
			TextStyle, "Text", "Interactor",
			ValueInteractor::AllocateInteractor);

   InteractorStyle::AddSupportedStyle("StringList",  
			TextListEditorStyle, "List Editor",
			ValueListInteractor::AllocateInteractor);
   InteractorStyle::AddSupportedStyle("StringList",  
			TextStyle, "Text", "Interactor",
			ValueInteractor::AllocateInteractor);


   //
   // Supported styles for Selector interactors 
   //
   InteractorStyle::AddSupportedStyle("Selector", 
			SelectorOptionMenuStyle,  "Option Menu", "Selector",
			SelectorInteractor::AllocateInteractor);
   InteractorStyle::AddSupportedStyle("Selector", 
			SelectorRadioStyle,  "Radio Button", "RadioGroup",
			SelectorRadioInteractor::AllocateInteractor);
// This interactor solves a problem that users have with data-driven
// Option Menu selectors.  That is that the menu can become too big
// very easily.  Nonetheless, I'm not adding support for this new
// interactor because this problem can be solved by switching from
// Option Menu to Scrolled List. I'm just not happy with having so
// much overlap in functionality. 
//   InteractorStyle::AddSupportedStyle("Selector", 
//			SelectorPulldownStyle,  "Pulldown Menu",
//			SelectorPulldownInteractor::AllocateInteractor);
   InteractorStyle::AddSupportedStyle("Selector", 
			SelectorListStyle,  "Scrolled List", "ListSelector",
			SelectorListInteractor::AllocateInteractor);

   //
   // Supported styles for other interactors 
   //
   InteractorStyle::AddSupportedStyle("FileSelector",FileSelectorStyle,
			"FileSelector",
			FileSelectorInteractor::AllocateInteractor);
   InteractorStyle::AddSupportedStyle("Toggle",ToggleToggleStyle,
			"Toggle", "Toggle",
			ToggleToggleInteractor::AllocateInteractor);
   InteractorStyle::AddSupportedStyle("Reset",ToggleToggleStyle,
			"Toggle", "Toggle",
			ToggleToggleInteractor::AllocateInteractor);
   InteractorStyle::AddSupportedStyle("SelectorList",SelectorListToggleStyle,
			"Button List", "ToggleGroup",
			SelectorListToggleInteractor::AllocateInteractor);
   InteractorStyle::AddSupportedStyle("SelectorList",SelectorListStyle,
			"Scrolled List", "ListSelector",
			SelectorListInteractor::AllocateInteractor);

   //
   // AT LAST, set the Default Style for each interactor.
   //
   InteractorStyle::SetDefaultStyle("Integer", StepperStyle);
   InteractorStyle::SetDefaultStyle("Scalar", StepperStyle);
   InteractorStyle::SetDefaultStyle("Vector", StepperStyle);
   InteractorStyle::SetDefaultStyle("IntegerList", TextListEditorStyle);
   InteractorStyle::SetDefaultStyle("ScalarList", TextListEditorStyle);
   InteractorStyle::SetDefaultStyle("VectorList", TextListEditorStyle);
   InteractorStyle::SetDefaultStyle("ValueList", TextListEditorStyle);
   InteractorStyle::SetDefaultStyle("StringList", TextListEditorStyle);
   InteractorStyle::SetDefaultStyle("Selector", SelectorOptionMenuStyle);
   InteractorStyle::SetDefaultStyle("FileSelector", FileSelectorStyle);

}
InteractorStyle::InteractorStyle(InteractorStyleEnum s, const char *n, 
		const char* javaStyle, InteractorAllocator ia)
{
    this->style = s;
    this->name = theSymbolManager->registerSymbol(n);
    this->allocateInteractor = ia;
    this->isDefault = FALSE;
    this->interactorName = NULL;
    if (javaStyle)
	this->javaStyle = DuplicateString(javaStyle);
    else
	this->javaStyle = NUL(char*);
}
InteractorStyle::~InteractorStyle()
{
    if (this->interactorName)
	delete this->interactorName;
    if (this->javaStyle)
	delete this->javaStyle;
}

//
// Return the list of InteractorStyles for the give interactor (node name).
//
Dictionary *InteractorStyle::GetInteractorStyleDictionary(
					const char* interactor)
{
   ASSERT(theInteractorStyleDictionary != NUL(Dictionary*));
   return (Dictionary*)theInteractorStyleDictionary->findDefinition(interactor);
}

//
// Set the default style for the given interactor.
// If this function is never called, the default style will be the first one
// in the style dictionary.
//
void InteractorStyle::SetDefaultStyle(const char* interactor,
			InteractorStyleEnum style)
{
    Dictionary *styledict;
    InteractorStyle *is;


    styledict = InteractorStyle::GetInteractorStyleDictionary(interactor); 
    if (!styledict)	 
	return;

    DictionaryIterator iterator(*styledict);

    while ( (is = (InteractorStyle *) iterator.getNextDefinition()) ) 
	is->isDefault = is->style == style; 
}

//
// Get the InteractorStyle entry associated with the give interactor
// (node name) and the give style.
//
InteractorStyle *InteractorStyle::GetInteractorStyle(const char* interactor,
			InteractorStyleEnum style)
{
    Dictionary *styledict;
    InteractorStyle *is, *firstIs = NULL;


    styledict = InteractorStyle::GetInteractorStyleDictionary(interactor); 
    if (!styledict)	 
	return NUL(InteractorStyle*);

    DictionaryIterator iterator(*styledict);

    while ( (is = (InteractorStyle *) iterator.getNextDefinition()) ) {
	if (firstIs == NULL)
	    firstIs = is;
	if ((is->style == style)  
	    OR (style == DefaultStyle AND is->isDefault))
	    return is;
    }

    if(style == DefaultStyle)
	 return firstIs; 

    return NULL;
}
//
// Get the InteractorStyle entry associated with the give interactor
// (node name) and the given style.
//
InteractorStyle *InteractorStyle::GetInteractorStyle(const char* interactor,
			const char *stylename)
{
    Dictionary *styledict;

    styledict = InteractorStyle::GetInteractorStyleDictionary(interactor); 
    if (!styledict)	 
	return NUL(InteractorStyle*);

    return (InteractorStyle*)styledict->findDefinition(stylename);

}

//
// Added supported style/name/interactorbuild group to the list
// of supported styles for the given named interactor (node name). 
//
boolean	InteractorStyle::AddSupportedStyle(const char *interactor, 
		InteractorStyleEnum style,  
		const char *stylename, 
		InteractorAllocator ia)
{
    return InteractorStyle::AddSupportedStyle 
	(interactor, style, stylename, NUL(char*), ia);
}
boolean	InteractorStyle::AddSupportedStyle(const char *interactor, 
		InteractorStyleEnum style,  
		const char *stylename, 
		const char *javaStyle,
		InteractorAllocator ia)
{
    Dictionary		*styledict;
    InteractorStyle	*is;

    styledict = InteractorStyle::GetInteractorStyleDictionary(interactor); 
    if (!styledict) {
	styledict = new Dictionary;
	ASSERT(styledict);
	if (!theInteractorStyleDictionary->addDefinition(interactor,
							(void*)styledict)) 
	{
	    delete styledict;
            ASSERT(0);
	}
    }
    
    is = new InteractorStyle(style, stylename, javaStyle, ia);

    boolean ret = styledict->addDefinition(stylename,(const void*)is); 
    ASSERT(ret);
    return ret;
}
	

Interactor *InteractorStyle::createInteractor(InteractorInstance *ii)
{ 
    ASSERT(this->allocateInteractor); 

    if (!this->interactorName) {
	//
	// Generate a unique widget name based on the Node name and style.
	//
	char *styleName = (char*)theSymbolManager->getSymbolString(this->name);
	styleName = StripWhiteSpace(styleName);
	if (isupper(styleName[0]))
	    styleName[0] = tolower(styleName[0]);

	int len = STRLEN(styleName) + STRLEN("Interactor") + 15;
	this->interactorName = new char[len];
	sprintf(this->interactorName,"%sInteractor",styleName);
	delete styleName;
    }

    Interactor *interactor = this->allocateInteractor(this->interactorName,ii); 
    interactor->initialize();
    interactor->createInteractor();
    return interactor;
}

