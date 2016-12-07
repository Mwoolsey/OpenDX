/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _GARNewCommand_h
#define _GARNewCommand_h


#include "../base/OptionalPreActionCommand.h"

//
// Class name definition:
//
#define ClassGARNewCommand	"GARNewCommand"

class  GARMainWindow;

//
// GARNewCommand class definition:
//				
class GARNewCommand : public OptionalPreActionCommand
{
  private:
    //
    // Private member data:
    //
    GARMainWindow *gmw;

  protected:
    //
    // Protected member data:
    //
    virtual boolean needsConfirmation();
    virtual void    doPreAction();

    virtual boolean doIt(CommandInterface *ci);
  public:

    //
    // Constructor:
    //
    GARNewCommand(const char      *name,
	       CommandScope    *scope,
	       boolean		active,
	       GARMainWindow   *gmw, 
		Widget		dialogParent);


    //
    // Destructor:
    //
    ~GARNewCommand(){}

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassGARNewCommand;
    }
};


#endif // _GARNewCommand_h
