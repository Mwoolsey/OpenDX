/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include <defines.h>




#include "Server.h"
#include "ListIterator.h"
#include "Client.h"


void Server::notifyClients(Symbol message, const void *data, const char *msg)
{
    ListIterator iterator(this->clientList);
    Client*      client;

    //
    // Notify each of the clients in the client list.
    //
    while( (client = (Client*)iterator.getNext()) )
    {
	client->notify(message, data, msg);
    }
}


boolean Server::registerClient(Client* client)
{
    ASSERT(client);

    //
    // If the client is already present in the list return FALSE;
    // otherwise, append the client to the list and return the result.
    //
    if (this->clientList.isMember(client))
    {
	return FALSE;
    }
    else
    {
	 return this->clientList.appendElement(client);
    }
}


boolean Server::unregisterClient(Client* client)
{
    int position;

    //
    // If the client is in the list, delete the client from the list
    // and return the result; otherwise, return FALSE.
    //
    if ( (position = this->clientList.getPosition(client)) )
    {
	return this->clientList.deleteElement(position);
    }
    else
    {
	return FALSE;
    }
}


