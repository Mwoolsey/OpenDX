/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"





#ifndef _Client_h
#define _Client_h


#include "Base.h"
#include "SymbolManager.h"


//
// Class name definition:
//
#define ClassClient	"Client"


//
// Client class definition:
//				
class Client : virtual public Base
{
  protected:
    //
    // Constructor:
    //   Protected to prevent direct instantiation....
    //
    Client(){}

  public:
    //
    // Destructor:
    //
    ~Client(){}

    //
    // This routine should be implemented by subclasses to handle
    // notifications by the server.
    //
    virtual void notify(const Symbol message, 
	const void *data=NULL, const char *msg = NULL) = 0;

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassClient;
    }
};


#endif // _Client_h
