/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _WorkSpacePage_h
#define _WorkSpacePage_h

#include "Base.h"
#include "WorkSpaceRoot.h"
#include "SymbolManager.h"


//
// Class name definition:
//
#define ClassWorkSpacePage "WorkSpacePage"

class WorkSpaceRoot;

//
// WorkSpace class definition:
//				
class WorkSpacePage : public Base
{
  private:
    WorkSpaceRoot* root_workspace;

  protected:

    WorkSpacePage(WorkSpaceRoot* root) {
	this->root_workspace = root;
    }

    WorkSpaceRoot* getWorkSpaceRoot() { return this->root_workspace; }

  public:
    ~WorkSpacePage() { }
  
    const char* getClassName() { return ClassWorkSpacePage; }

    virtual boolean isA (Symbol classname) {
	Symbol s = theSymbolManager->registerSymbol(ClassWorkSpacePage);
	return (s == classname);
    }
    boolean isA (const char* classname) {
	Symbol s = theSymbolManager->registerSymbol(classname);
	return this->isA(s);
    }
};


#endif // _WorkSpacePage_h




