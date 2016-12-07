/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _XHandler_h
#define _XHandler_h


#include "Base.h"
#include "List.h"

#include <X11/Xlib.h>


//
// Class name definition:
//
#define ClassXHandler	"XHandler"

//
// Function to handle events, return true if this event should also be
// past to the Intrinsics for handling.
typedef boolean (*XHandlerCallback)(XEvent *event, void *clientData);

//
// XHandler class definition:
//				
class XHandler : public Base
{
  private:
    //
    // Private member data:
    //

  protected:
    //
    // Protected member data:
    //
    static List Handlers;

    int 	      eventName;
    Window            window;
    XHandlerCallback  callback;
    void             *clientData;


  public:
    //
    // Constructor:
    //
    XHandler(int eventName,
	     Window window,
	     XHandlerCallback cb,
	     void *clientData);

    //
    // Destructor:
    //
    ~XHandler();

    //
    // Function to handle events. Returns true if this event should also be
    // past to the Intrinsics for handling.
    //
    static boolean ProcessEvent(XEvent *event);

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassXHandler;
    }
};


#endif // _XHandler_h
