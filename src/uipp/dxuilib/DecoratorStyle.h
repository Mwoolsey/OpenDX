/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"





#ifndef _DecoratorStyle_h_
#define _DecoratorStyle_h_

#include "Base.h"
#include "SymbolManager.h"
#include <X11/Intrinsic.h>


#define ClassDecoratorStyle	"DecoratorStyle"

class Dictionary;
class Decorator;

typedef Decorator* 
(*DecoratorAllocator)(boolean developerStyle);

extern void BuildtheDecoratorStyleDictionary(void);

class DecoratorStyle : public Base {
  private:

    Symbol		name;
    DecoratorAllocator	allocateDecorator;
    boolean		isDefault;
    char *		key;
    boolean		useInVPE;

  public:
	
    enum DecoratorStyleEnum {
	    UnknownStyle     = 0,
	    DefaultStyle     = 1,	
	    LabelStyle       = 2,
	    SeparatorStyle   = 3,
	    PostItStyle      = 4
    };

  protected:
    DecoratorStyleEnum style;

  public:
    //
    // Added supported style/name/decoratorbuild group to the list
    // of supported styles for the given named decorator (node name).
    //
    static boolean AddSupportedStyle(const char *decorator,
		    DecoratorStyleEnum style,
		    const char *stylename,
		    boolean useInVPE,
		    DecoratorAllocator ia);

    static void BuildDictionary(void);
    //
    // Get the DecoratorStyle entry associated with the give decorator
    // (node name) and the give style or style name.
    //
    static DecoratorStyle *GetDecoratorStyle(const char* decorator,
			    DecoratorStyleEnum style);
    static DecoratorStyle *GetDecoratorStyle(const char* decorator,
			    const char *stylname);
    static void 	   SetDefaultStyle(const char* decorator,
			    DecoratorStyleEnum style);

    //
    // Return the list of DecoratorStyles for the give decorator (node name).
    //
    static Dictionary *GetDecoratorStyleDictionary(const char* decorator);
    static Dictionary *GetDecoratorStyleDictionary();


    DecoratorStyle(DecoratorStyleEnum s, const char *n, boolean useInVPE, 
				DecoratorAllocator ia, const char *key);

    ~DecoratorStyle();

    Decorator *createDecorator(boolean developerStyle);

    boolean allowedInVPE() { return this->useInVPE; }

    const char *getKeyString()
	{ return this->key; }
    const char *getNameString() 
	{ return theSymbolManager->getSymbolString(this->name); }
    DecoratorStyleEnum getStyleEnum() { return this->style; }

    const char *getClassName() 
		{ return ClassDecoratorStyle; }

};


#endif	// _DecoratorStyle_h_
