/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include "QueuedPackets.h"
#include "PacketIF.h"

QueuedBytes::QueuedBytes(const char* data, int length)
{
    if (!length) {
	this->length = strlen(data);
	this->data = new char[this->length+1];
	strcpy (this->data, data);
    } else {
	this->length = length;
	this->data = new char[length+1];
	strncpy (this->data, data, this->length);
	this->data[this->length] = NUL(char);
    }
}

QueuedPacket::QueuedPacket(int type, int packetId, const char* data, int length) :
    QueuedBytes(data, length)
{
    this->type = type;
    this->packetId = packetId;
}

QueuedImmediate::QueuedImmediate(const char* data, int length) : QueuedBytes(data,length)
{
}

QueuedBytes::~QueuedBytes()
{
    if (this->data) delete this->data;
}

void QueuedPacket::send(PacketIF* pif)
{
    pif->_sendPacket (this->type, this->packetId, this->data, this->length);
}

void QueuedImmediate::send(PacketIF* pif)
{
    pif->_sendImmediate (this->data);
}

void QueuedBytes::send(PacketIF* pif)
{
    pif->_sendBytes (this->data);
}


