/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#ifndef _MBCommand_h
#define _MBCommand_h


#include "NoUndoCommand.h"
#include <Xm/Xm.h>

class  MBMainWindow;

//
// MBCommand class definition:
//				

#define ClassMBCommand  "MBCommand"

class MBCommand : public NoUndoCommand 
{
  private:

    static  String	DefaultResources[];
    MBMainWindow* 	mdmw;
    int			option;

  protected:
    virtual boolean doIt(CommandInterface *ci);

  public:
    //
    // Constructor:
    //
    MBCommand(const char*,
		CommandScope*,
		boolean active,
                MBMainWindow*,
		int option);

    //
    // Destructor:
    //
    ~MBCommand(){}

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassMBCommand;
    }
};


#endif // _MBCommand_h
