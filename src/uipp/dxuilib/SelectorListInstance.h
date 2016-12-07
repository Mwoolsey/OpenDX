/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _SelectorListInstance_h
#define _SelectorListInstance_h



#include "DXStrings.h"
#include "SelectorListNode.h"
#include "SelectionInstance.h"

class SelectorListInteractor;


//
// Class name definition:
//
#define ClassSelectorListInstance	"SelectorListInstance"


//
// Describes an instance of an interactor in a control Panel.
//
class SelectorListInstance : public SelectionInstance {

  private:


  protected:

    virtual const char* javaName() { return "slist"; }

  public:
    SelectorListInstance(InteractorNode *node) : SelectionInstance(node) {}
	
    ~SelectorListInstance()  {}

    virtual boolean printAsJava(FILE* );

    const char *getClassName() 
		{ return ClassSelectorListInstance; }
};

#endif // _SelectorListInstance_h

