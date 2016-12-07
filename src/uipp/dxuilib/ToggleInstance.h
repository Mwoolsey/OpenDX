/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#ifndef _ToggleInstance_h
#define _ToggleInstance_h


#include "InteractorInstance.h"
#include "ToggleNode.h"

//
// Class name definition:
//
#define ClassToggleInstance	"ToggleInstance"

//
// Describes an instance of an scalar interactor in a control Panel.
//
class ToggleInstance : public InteractorInstance  {

    friend class ToggleNode;

  private:

  protected:

    //
    // Create the default  set attributes dialog box for this class of
    // Interactor.
    //
    virtual SetAttrDialog *newSetAttrDialog(Widget parent);

    virtual boolean defaultVertical() { return FALSE; }

    virtual const char* javaName() { return "toggle"; }

  public:
    ToggleInstance(ToggleNode *n);

    ~ToggleInstance();

    virtual boolean hasSetAttrDialog();

    virtual void setVerticalLayout(boolean val = TRUE);

    virtual boolean printAsJava (FILE* );

    const char *getClassName() { return ClassToggleInstance; }
	
};

#endif // _ToggleInstance_h

