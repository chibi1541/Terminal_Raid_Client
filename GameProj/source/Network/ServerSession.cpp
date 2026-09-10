#include "pch.h"
#include "ServerSession.h"
#include "Protocol/ServerPacketHandler.h"
#include "Protocol/Protocol.pb.h"
#include "Globals.h"

using namespace Craft;

ServerSession::ServerSession(NetAddress address)
	: Session(address)
{
		
}

ServerSession::~ServerSession()
{
}

void ServerSession::OnConnected()
{
	// 메뉴에서 고른 이름/캐릭터. 빈 이름이면 "Player".
	Protocol::C_LOGIN loginPkt;
	loginPkt.set_name(GLoginRequest.name.empty() ? "Player" : GLoginRequest.name);
	loginPkt.set_chartype(static_cast<Protocol::CharacterType>(GLoginRequest.charType));
	int32 size = 0;
	BYTE* buffer = ServerPacketHandler::MakeSendBuffer(loginPkt, OUT size);
	RegisterSend(buffer, size);
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
	Session::OnSend(len);
}

void ServerSession::OnRecvPacket(BYTE* buffer, int32 len)
{
	ServerPacketHandler::HandlePacket(this, buffer, len);
}
