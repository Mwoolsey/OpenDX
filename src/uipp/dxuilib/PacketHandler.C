/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#include "DXStrings.h"
#include "PacketHandler.h"

PacketHandler::PacketHandler(int plinger,
				 int ptype,
				 int pnumber,
				 PacketHandlerCallback pcallback,
				 void *pclientData,
				 const char *matchString)

{
    this->linger = plinger;
    this->type = ptype;
    this->number = pnumber;
    this->callback = pcallback;
    this->clientData = pclientData;
    this->matchString = matchString? DuplicateString(matchString): NULL;
    this->matchLen = matchString? STRLEN(matchString): 0;
}

PacketHandler::~PacketHandler()
{
    if (this->matchString)
	delete this->matchString;
}
boolean PacketHandler::match(const char *s)
{
    if (s == NULL)
	return this->matchString == NULL;
    if (this->matchString == NULL)
	return FALSE;
    return EqualString(this->matchString, s);
}
boolean PacketHandler::matchFirst(const char *s)
{
    if (this->matchString == NULL)
	return TRUE;
    if (s == NULL)
	return FALSE;
    return EqualSubstring(this->matchString, s, this->matchLen);
}

