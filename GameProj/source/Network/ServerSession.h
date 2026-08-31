#pragma once

#include "Network/Session.h"
#include "Network/NetAddress.h"

class ServerSession : public Craft::Session
{
public:
	ServerSession(Craft::NetAddress address);
	virtual ~ServerSession();

protected:
	virtual void		OnConnected() override;
	virtual int32		OnRecv(BYTE* buffer, int32 len) override;
	virtual void		OnSend(int32 len) override;
	virtual void		OnRecvPacket(BYTE* buffer, int32 len);
};

struct PacketHeader
{
	uint16 id;
	uint16 size;
};