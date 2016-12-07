/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _Server_h
#define _Server_h


#include "Base.h"
#include "SymbolManager.h"
#include "List.h"


//
// Class name definition:
//
#define ClassServer	"Server"


//
// Referenced classes:
//
class Client;


//
// Server class definition:
//				
class Server : virtual public Base
{
  protected:
    //
    // Protected member data:
    //
    List clientList;	// list of registered clients to notify

    //
    // Constructor:
    //   Protected to prevent direct instantiation....
    //
    Server(){}

    //
    // Sends notification to all the clients.
    //   Protected to be callable only by subclasses.
    //
    void notifyClients(Symbol message, const void *msgdata=NULL, const char *msg = NULL);

  public:
    //
    // Destructor:
    //
    ~Server(){}

    //
    // Registers client.
    // Returns TRUE if the client has been registered.
    // Returns FALSE if the client has already been registered or
    // if the client cannot be registered.
    //
    virtual boolean registerClient(Client* client);
	
    //
    // Unregisters client.
    // Returns TRUE if the client has been unregistered.
    // Returns FALSE if the client has already been unregistered or
    // if the client cannot be unregistered.
    //
    virtual boolean unregisterClient(Client* client);

    //
    // Returns a pointer to the class name.
    //
    const char* getClassName()
    {
	return ClassServer;
    }
};


#endif // _Server_h
