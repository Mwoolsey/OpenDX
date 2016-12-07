/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
#if !defined(_QUUED_PACKETS_H_)
#define _QUUED_PACKETS_H_

#include <dxconfig.h>

class PacketIF;
//
// dxui and dxexec have a dining philosophers problem.  Each writes (thru
// a socket) to the other.  When the sockets' buffers are full, then the
// next fwrite or fputs blocks and never returns.  So we wind up with both
// processes having filled their buffers and neither able to read from the
// other.
//
// PacketIF has been changed so that it always checks for socket input
// before writing.  If input is available then PacketIF instances one
// of the objects in this file.  Each is really just a struct that
// holds the output until we find time to ship it to dxexec.  Many places
// within dxuilib and dxui directories used to access the socket directly,
// but now they always go thru PacketIF.
//
// These 3 classes will be instanced and stuck into a fifo queue in PacketIF.
// They differ in the way they cause their buffered information to be sent
// to dxexec.
//

//
// Store raw data.  These bytes will be sent unformatted.
//
class QueuedBytes {
    protected:
	char* data;
	int length;
    public:
	QueuedBytes(const char* data, int length=0);
	virtual ~QueuedBytes();
	virtual void send(PacketIF*);
};

//
// Store one packet.  These bytes will be packetized in PacketIF::_sendPacket()
//
class QueuedPacket : public QueuedBytes {
    protected:
	int type;
	int packetId;
    public:
	QueuedPacket(int type, int packetId, const char* data, int length);
	virtual ~QueuedPacket(){}
	virtual void send(PacketIF*);
};

//
// Store raw bytes.  These bytes will be formatted with a $
//
class QueuedImmediate : public QueuedBytes {
    public:
	QueuedImmediate(const char* data, int length=0);
	virtual ~QueuedImmediate(){}
	virtual void send(PacketIF*);
};

#endif // _QUUED_PACKETS_H_
