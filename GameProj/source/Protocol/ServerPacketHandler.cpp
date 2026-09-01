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
	// 지금은 일단 크래쉬
	ASSERT_CRASH(pkt.success());




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

bool Handle_S_SPAWN(const Session* session, Protocol::S_SPAWN& pkt)
{
	return true;
}

bool Handle_S_DESPAWN(const Session* session, Protocol::S_DESPAWN& pkt)
{
	return true;
}


