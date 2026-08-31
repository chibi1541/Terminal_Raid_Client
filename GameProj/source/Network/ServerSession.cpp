#include "pch.h"
#include "ServerSession.h"
#include "Protocol/ServerPacketHandler.h"

using namespace Craft;

ServerSession::ServerSession(NetAddress address)
	: Session(address)
{
		
}

ServerSession::~ServerSession()
{
}

int32 ServerSession::OnRecv(BYTE* buffer, int32 len)
{
	int32 processLen = 0;
	while (true)
	{
		int32 dataSize = len - processLen;

		if (dataSize < sizeof(PacketHeader))
			break;

		PacketHeader header = *(reinterpret_cast<PacketHeader*>(&buffer[processLen]));

		if (dataSize < header.size)
			break;

		OnRecvPacket(&buffer[processLen], header.size);

		processLen += header.size;
	}

	return processLen;
}

void ServerSession::OnSend(int32 len)
{
}

void ServerSession::OnRecvPacket(BYTE* buffer, int32 len)
{
	ServerPacketHandler::HandlePacket(this, buffer, len);
}
