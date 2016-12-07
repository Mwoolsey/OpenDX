/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _ArkStandIn_h
#define _ArkStandIn_h


#include "Base.h"
#include <../widgets/WorkspaceW.h>


//
// Class name definition:
//
#define ClassArkStandIn	"ArkStandIn"

//
// referenced classes
//

//
// ArkStandIn class definition:
//				
class ArkStandIn : public Base
{
  private:
    //
    // Private member data:
    //
    XmWorkspaceWidget parent;
    XmWorkspaceLine line;

  protected:
    //
    // Protected member data:
    //


  public:
    //
    // Constructor:
    //
    ArkStandIn(XmWorkspaceWidget w, XmWorkspaceLine l)
    {
       this->line = l;
       this->parent = w;
    }

    //
    // Destructor:
    //
    ~ArkStandIn()
    {
        XmDestroyWorkspaceLine((XmWorkspaceWidget) this->parent, 
                               this->line, FALSE);
    }
    void setWorkSpaceLine(XmWorkspaceLine l)
    {
        this->line = l;
    }
    XmWorkspaceLine getWorkSpaceLine()
    {
        return this->line;
    }

    //
    // Print a representation of the stand in on a PostScript device.  We
    // assume that the device is in the same coordinate space as the parent
    // of this uicomponent (i.e. we send the current geometry to the 
    // PostScript output file).  
    //
    boolean printAsPostScript(FILE *f);

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassArkStandIn;
    }
};


#endif // _ArkStandIn_h
