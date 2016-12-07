/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#ifndef _GARCommand_h
#define _GARCommand_h


#include "../base/NoUndoCommand.h"
#include <Xm/Xm.h>

class  GARMainWindow;

//
// GARCommand class definition:
//				

#define ClassGARCommand  "GARCommand"

class GARCommand : public NoUndoCommand 
{
  private:

    static  String	DefaultResources[];
    GARMainWindow* 	gmw;
    int			option;

  protected:
    virtual boolean doIt(CommandInterface *ci);

  public:
    //
    // Constructor:
    //
    GARCommand(const char*,
		CommandScope*,
		boolean active,
                GARMainWindow*,
		int option);

    //
    // Destructor:
    //
    ~GARCommand(){}

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassGARCommand;
    }
};


#endif // _GARCommand_h
