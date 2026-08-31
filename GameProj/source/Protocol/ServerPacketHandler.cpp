#include "pch.h"
#include "ServerPacketHandler.h"

using namespace Craft;

PacketHandlerFunc GPacketHandler[UINT16_MAX];

bool Handle_INVALID(const Session* session, BYTE* buffer, int32 len)
{
	return false;
}

bool Handle_S_LOGIN(const Session* session, Protocol::S_LOGIN& pkt)
{
	return true;
}

bool Handle_S_PONG(const Session* session, Protocol::S_PONG& pkt)
{
	return true;
}

bool Handle_S_ENTER_ROOM(const Session* session, Protocol::S_ENTER_ROOM& pkt)
{
	return true;
}

bool Handle_S_EXIT_ROOM(const Session* session, Protocol::S_EXIT_ROOM& pkt)
{
	return true;
}


