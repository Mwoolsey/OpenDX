/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _ButtonInterface_h
#define _ButtonInterface_h


#include "CommandInterface.h"


//
// Class name definition:
//
#define ClassButtonInterface	"ButtonInterface"


//
// ButtonInterface class definition:
//				
class ButtonInterface : public CommandInterface
{
  public:
    //
    // Constructor:
    //
    ButtonInterface(Widget   parent,
		    char*    name,
		    Command* command,
		    const char *bubbleHelp = NUL(char*));

    //
    // Destructor:
    //
    ~ButtonInterface(){}

    //
    // Set the label string.
    //
    void setLabel(const char* string);

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassButtonInterface;
    }
};


#endif // _ButtonInterface_h
