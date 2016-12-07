/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _VPEPage_h
#define _VPEPage_h


#include "EditorWorkSpace.h"
#include "WorkSpacePage.h"
#include "SymbolManager.h"


//
// Class name definition:
//
#define ClassVPEPage	"VPEPage"

class WorkSpaceInfo;
class WorkSpaceRoot;
class EditorWindow;

//
// EditorWorkSpace class definition:
//				
class VPEPage : public EditorWorkSpace, public WorkSpacePage
{
  private:

  protected:

    virtual boolean isRoot() { return FALSE; }

    virtual void saveWorkSpaceParams(WorkSpace*);

  public:

    VPEPage(const char *name, Widget parent, WorkSpaceInfo *info, 
			EditorWindow *editor, WorkSpaceRoot* root);
 
    ~VPEPage(){
	this->getWorkSpaceRoot()->removePage(this);
    }

    virtual void resize() {}

    const char* getClassName() { return ClassVPEPage; }
    virtual boolean isA (Symbol classname);
    boolean isA (const char* classname);
};


#endif // _VPEPage_h
