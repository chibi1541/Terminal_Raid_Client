#include "pch.h"
#include "ServerPacketHandler.h"

using namespace Craft;

PacketHandlerFunc GPacketHandler[UINT16_MAX];

bool Handle_S_LOGIN(std::shared_ptr<Session>& session, Protocol::S_LOGIN& pkt)
{
	return true;
}

bool Handle_S_PONG(std::shared_ptr<Session>& session, Protocol::S_PONG& pkt)
{
	return true;
}

bool Handle_S_ENTER_ROOM(std::shared_ptr<Session>& session, Protocol::S_ENTER_ROOM& pkt)
{
	return true;
}

bool Handle_S_EXIT_ROOM(std::shared_ptr<Session>& session, Protocol::S_EXIT_ROOM& pkt)
{
	return true;
}
