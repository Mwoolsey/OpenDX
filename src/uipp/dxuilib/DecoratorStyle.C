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
#include "DecoratorStyle.h"
#include "Dictionary.h"
#include "DictionaryIterator.h"
#include "LabelDecorator.h"
#include "SeparatorDecorator.h"
#include "VPEAnnotator.h"
#include "VPEPostIt.h"

static Dictionary *theDecoratorStyleDictionary =  NULL;
						

void BuildtheDecoratorStyleDictionary() { DecoratorStyle::BuildDictionary(); }
void DecoratorStyle::BuildDictionary()
{
   theDecoratorStyleDictionary = new Dictionary;
 
   DecoratorStyle::AddSupportedStyle("Label", DecoratorStyle::LabelStyle,  
			    "Label",  FALSE, LabelDecorator::AllocateDecorator);
   DecoratorStyle::AddSupportedStyle("Separator", DecoratorStyle::SeparatorStyle,  
			    "Separator", FALSE, SeparatorDecorator::AllocateDecorator);

   // Warning: The name Annotate is used in EditorWindow.C so that he can pick
   // off just the vpe annotation dictionary and loop over it.  If you change the
   // name here, then change it there also or else put a loop there which checks
   // allowedInVPE().
   DecoratorStyle::AddSupportedStyle("Annotate", DecoratorStyle::LabelStyle,  
			    "Label", TRUE, VPEAnnotator::AllocateDecorator);
   DecoratorStyle::AddSupportedStyle("Annotate", DecoratorStyle::PostItStyle,  
			    "Marker", TRUE, VPEPostIt::AllocateDecorator);

   DecoratorStyle::SetDefaultStyle("Label", DecoratorStyle::LabelStyle);
   DecoratorStyle::SetDefaultStyle("Separator", DecoratorStyle::SeparatorStyle);
   DecoratorStyle::SetDefaultStyle("Annotate", DecoratorStyle::LabelStyle);
}

DecoratorStyle::DecoratorStyle(DecoratorStyleEnum s, const char *n, boolean useInVPE, 
			DecoratorAllocator ia, const char *key)
{
    this->style = s;
    this->name = theSymbolManager->registerSymbol(n);
    this->allocateDecorator = ia;
    this->isDefault = FALSE;
    this->key = DuplicateString(key);
    this->useInVPE = useInVPE;
}
DecoratorStyle::~DecoratorStyle()
{
    delete key;
}

//
// Return the list of DecoratorStyles for the give decorator.
//
Dictionary *DecoratorStyle::GetDecoratorStyleDictionary(
					const char* decorator)
{
   ASSERT(theDecoratorStyleDictionary != NUL(Dictionary*));
   return (Dictionary*)theDecoratorStyleDictionary->findDefinition(decorator);
}

//
// Return the entire DecoratorStyle dictionary
//
Dictionary *DecoratorStyle::GetDecoratorStyleDictionary()
{
   ASSERT(theDecoratorStyleDictionary != NUL(Dictionary*));
   return (Dictionary*)theDecoratorStyleDictionary;
}

//
// Set the default style for the given decorator.
// If this function is never called, the default style will be the first one
// in the style dictionary.
//
void DecoratorStyle::SetDefaultStyle(const char* decorator,
			DecoratorStyleEnum style)
{
    Dictionary *styledict;
    DecoratorStyle *is;


    styledict = DecoratorStyle::GetDecoratorStyleDictionary(decorator); 
    //if (!styledict) return;
    ASSERT(styledict);

    DictionaryIterator iterator(*styledict);

    while ( (is = (DecoratorStyle *) iterator.getNextDefinition()) ) 
	is->isDefault = is->style == style; 
}

//
// Get the DecoratorStyle entry associated with the give decorator
// (node name) and the give style.
//
DecoratorStyle *DecoratorStyle::GetDecoratorStyle(const char* decorator,
			DecoratorStyleEnum style)
{
    Dictionary *styledict;
    DecoratorStyle *is, *firstIs = NULL;


    styledict = DecoratorStyle::GetDecoratorStyleDictionary(decorator); 
    if (!styledict)	 
	return NUL(DecoratorStyle*);

    DictionaryIterator iterator(*styledict);

    while ( (is = (DecoratorStyle *) iterator.getNextDefinition()) ) {
	if (firstIs == NULL)
	    firstIs = is;
	if ((is->style == style)  
	    OR (style == DecoratorStyle::DefaultStyle AND is->isDefault))
	    return is;
    }

    if(style == DecoratorStyle::DefaultStyle)
	 return firstIs; 

    return NULL;
}
//
// Get the DecoratorStyle entry associated with the give decorator
// (node name) and the given style.
//
DecoratorStyle *DecoratorStyle::GetDecoratorStyle(const char* decorator,
			const char *stylename)
{
    Dictionary *styledict;

    styledict = DecoratorStyle::GetDecoratorStyleDictionary(decorator); 
    if (!styledict)	 
	return NUL(DecoratorStyle*);

    return (DecoratorStyle*)styledict->findDefinition(stylename);

}

//
// Added supported style/name/decoratorbuild group to the list
// of supported styles for the given named decorator (node name). 
//
boolean	DecoratorStyle::AddSupportedStyle(const char *decorator, 
		DecoratorStyleEnum style,  
		const char *stylename, 
		boolean useInVPE,
		DecoratorAllocator ia)
{
    Dictionary		*styledict;
    DecoratorStyle	*is;

    styledict = DecoratorStyle::GetDecoratorStyleDictionary(decorator); 
    if (!styledict) {
	styledict = new Dictionary;
	ASSERT(styledict);
	if (!theDecoratorStyleDictionary->addDefinition(decorator,
							(void*)styledict)) 
	{
	    delete styledict;
            ASSERT(0);
	}
    }
    
    is = new DecoratorStyle(style, stylename, useInVPE, ia, decorator);

    boolean ret = styledict->addDefinition(stylename,(const void*)is); 
    ASSERT(ret);
    return ret;
}
	

Decorator *DecoratorStyle::createDecorator(boolean developerStyle)
{ 
    ASSERT(this->allocateDecorator); 
    Decorator *decorator = this->allocateDecorator(developerStyle);
    decorator->initialize();
    return decorator;
}

