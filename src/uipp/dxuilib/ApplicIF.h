/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _ApplicIF_h
#define _ApplicIF_h


#include "Base.h"
#include "PacketIF.h"


//
// Class name definition:
//
#define ClassApplicIF	"ApplicIF"


class CmdEntry;

//
// ApplicIF class definition:
//				
class ApplicIF : public PacketIF
{
  private:
    //
    // Private member data:
    //

  protected:
    //
    // Protected member data:
    //
    static void ApplicationMessage(void *clientData, int id, void *line);
    static void ApplicationInformation(void *clientData, int id, void *line);
    static void ApplicationError(void *clientData, int id, void *line);
    static void ApplicationCompletion(void *clientData, int id, void *line);
    static void ApplicationForeground(void *clientData, int id, void *line);
    static void ApplicationQuery(void *clientData, int id, void *line);
    static void ApplicationLink(void *clientData, int id, void *line);
    static void HandleApplicationError(void *clientData, char *message);
    static void PassThruDXL(void *clientData, int id, void *line);


    virtual void installLinkHandler();

    virtual void parsePacket();

  public:
    //
    // Constructor:
    //
    ApplicIF(const char *server, int port, int local);

    //
    // Initializer
    //
    virtual void initializePacketIO();

    //
    // Do what ever is necessary when a connection to the server
    // (theDXApplication->getPacketIF()) has been established.
    //
    void handleServerConnection();

    void send(int type, const char* data = NULL);

    const char* getClassName()
    {
	return ClassApplicIF;
    }
};

#endif // _ApplicIF_h
