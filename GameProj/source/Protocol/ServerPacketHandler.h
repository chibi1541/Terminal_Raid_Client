#pragma once
#include "Protocol.pb.h"
#include "Network/SendBuffer.h"
#include "Network/ServerSession.h"
#include "Engine/Engine.h"
#include "Thread/ThreadManager.h"

using namespace Craft;

// jinja2 템플릿 엔진을 활용한 코드 자동화용 템플릿(클라이언트 용)

using PacketHandlerFunc = std::function<bool(const Session*, BYTE*, int32)>;
extern PacketHandlerFunc GPacketHandler[UINT16_MAX];

// PKT enum 자동화
enum : uint16
{
	PKT_C_LOGIN = 1000,
	PKT_S_LOGIN = 1001,
	PKT_C_PING = 1002,
	PKT_S_PONG = 1003,
	PKT_C_ENTER_ROOM = 1004,
	PKT_S_ENTER_ROOM = 1005,
	PKT_C_EXIT_ROOM = 1006,
	PKT_S_EXIT_ROOM = 1007,
	PKT_S_SPAWN = 1008,
	PKT_S_DESPAWN = 1009,
	PKT_C_MOVE = 1010,
	PKT_S_MOVE = 1011,
	PKT_S_MOVE_ACK = 1012,
};

bool Handle_INVALID(const Session* session, BYTE* buffer, int32 len);

// PKT handle 함수 자동 선언, 선언부만 만들어주기 때문에 정의부를 따로 생성해야 함
bool Handle_S_LOGIN(const Session* session, Protocol::S_LOGIN& pkt);
bool Handle_S_PONG(const Session* session, Protocol::S_PONG& pkt);
bool Handle_S_ENTER_ROOM(const Session* session, Protocol::S_ENTER_ROOM& pkt);
bool Handle_S_EXIT_ROOM(const Session* session, Protocol::S_EXIT_ROOM& pkt);
bool Handle_S_SPAWN(const Session* session, Protocol::S_SPAWN& pkt);
bool Handle_S_DESPAWN(const Session* session, Protocol::S_DESPAWN& pkt);
bool Handle_S_MOVE(const Session* session, Protocol::S_MOVE& pkt);
bool Handle_S_MOVE_ACK(const Session* session, Protocol::S_MOVE_ACK& pkt);

// PacketHandler 클래스 자동화
class ServerPacketHandler
{
public:
	static void Init()
	{
		for (int32 i = 0; i < UINT16_MAX; ++i)
			GPacketHandler[i] = Handle_INVALID;

		// Handler 함수 등록 자동화
		GPacketHandler[PKT_S_LOGIN] = [](const Session* session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_LOGIN>(Handle_S_LOGIN, session, buffer, len); };
		GPacketHandler[PKT_S_PONG] = [](const Session* session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_PONG>(Handle_S_PONG, session, buffer, len); };
		GPacketHandler[PKT_S_ENTER_ROOM] = [](const Session* session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_ENTER_ROOM>(Handle_S_ENTER_ROOM, session, buffer, len); };
		GPacketHandler[PKT_S_EXIT_ROOM] = [](const Session* session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_EXIT_ROOM>(Handle_S_EXIT_ROOM, session, buffer, len); };
		GPacketHandler[PKT_S_SPAWN] = [](const Session* session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_SPAWN>(Handle_S_SPAWN, session, buffer, len); };
		GPacketHandler[PKT_S_DESPAWN] = [](const Session* session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_DESPAWN>(Handle_S_DESPAWN, session, buffer, len); };
		GPacketHandler[PKT_S_MOVE] = [](const Session* session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_MOVE>(Handle_S_MOVE, session, buffer, len); };
		GPacketHandler[PKT_S_MOVE_ACK] = [](const Session* session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::S_MOVE_ACK>(Handle_S_MOVE_ACK, session, buffer, len); };

	}

	static bool HandlePacket(const Session* session, BYTE* buffer, int32 len)
	{
		PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);
		return GPacketHandler[header->id](session, buffer, len);
	}

	// sendbuffer 작성 자동화, 선언부만 만들어주기 때문에 정의부를 따로 생성해야 함
	static BYTE* MakeSendBuffer(Protocol::C_LOGIN& pkt, OUT int32& size) {return MakeSendBuffer(pkt, PKT_C_LOGIN, size); }
	static BYTE* MakeSendBuffer(Protocol::C_PING& pkt, OUT int32& size) {return MakeSendBuffer(pkt, PKT_C_PING, size); }
	static BYTE* MakeSendBuffer(Protocol::C_ENTER_ROOM& pkt, OUT int32& size) {return MakeSendBuffer(pkt, PKT_C_ENTER_ROOM, size); }
	static BYTE* MakeSendBuffer(Protocol::C_EXIT_ROOM& pkt, OUT int32& size) {return MakeSendBuffer(pkt, PKT_C_EXIT_ROOM, size); }
	static BYTE* MakeSendBuffer(Protocol::C_MOVE& pkt, OUT int32& size) {return MakeSendBuffer(pkt, PKT_C_MOVE, size); }


private:
	template<typename PacketType, typename ProcessFunc>
	static bool HandlePacket(ProcessFunc func, const Session* session, BYTE* buffer, uint32 len)
	{
		PacketType pkt;
		if (pkt.ParseFromArray(buffer + sizeof(PacketHeader), len - sizeof(PacketHeader)) == false)
			return false;

		return func(session, pkt);
	}

	template<typename T>
	static BYTE* MakeSendBuffer(T& pkt, uint16 pktId, OUT int32& size)
	{
		uint16 dataSize = static_cast<uint16>(pkt.ByteSizeLong());
		uint16 packetSize = dataSize + sizeof(PacketHeader);

		BYTE* bufferChunk = Engine::Get().GetThreadManager()->OpenBufferChunk(packetSize);
		PacketHeader* header = reinterpret_cast<PacketHeader*>(bufferChunk);
		header->size = packetSize;
		header->id = pktId;
		ASSERT_CRASH(pkt.SerializeToArray((&header[1]), dataSize));
		Engine::Get().GetThreadManager()->CloseBufferChunk(packetSize);

		size = packetSize;

		return bufferChunk;
	}
};
