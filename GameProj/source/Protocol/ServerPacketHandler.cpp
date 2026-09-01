#include "pch.h"
#include "ServerPacketHandler.h"
#include "Network/NetStatus.h"

using namespace Craft;

// ---------------------------------------------------------------------------
// 이 파일의 Handle_S_XXX 함수는 전부 "네트워크 쓰레드"에서 실행된다.
// 게임 쓰레드(Engine::Run 루프)와 완전히 다른 쓰레드다.
//
// 그래서 여기서는 다음 세 가지를 규약으로 지킨다.
//
//  1. 게임 객체(Level / Actor / UI / AssetManager)를 절대 만지지 않는다.
//     Engine::RunOnGameThread()로 잡을 미는 것 외에 아무것도 하지 않는다.
//     잡은 다음 프레임의 정해진 지점(DispatchInput 직전)에서 게임 쓰레드가 소비한다.
//
//  2. pkt은 참조로 들어오고 이 함수가 끝나면 사라진다.
//     그러므로 람다에 반드시 "값으로 복사"해서 넘긴다(protobuf 메시지는 복사 가능).
//     참조나 포인터로 캡처하면 잡이 실행될 때는 이미 죽은 객체다.
//
//  3. session 포인터는 캡처하지 않는다.
//     응답을 보내야 한다면 게임 쓰레드에서 GService를 통해 보낸다.
//     송신 경로(MakeSendBuffer + RegisterSend)는 이미 쓰레드 세이프하다.
//     - MakeSendBuffer가 쓰는 버퍼 청크는 thread_local
//     - SendBuffer::PushSendQueue는 락을 잡고 복사
// ---------------------------------------------------------------------------

PacketHandlerFunc GPacketHandler[UINT16_MAX];

bool Handle_INVALID(const Session* session, BYTE* buffer, int32 len)
{
	return false;
}

bool Handle_S_LOGIN(const Session* session, Protocol::S_LOGIN& pkt)
{
	Engine::Get().RunOnGameThread([pkt]()
		{
			NetStatus::Get().OnLogin(pkt);
		});

	return true;
}

bool Handle_S_PONG(const Session* session, Protocol::S_PONG& pkt)
{
	Engine::Get().RunOnGameThread([]()
		{
			NetStatus::Get().OnPong();
		});

	return true;
}

bool Handle_S_ENTER_ROOM(const Session* session, Protocol::S_ENTER_ROOM& pkt)
{
	Engine::Get().RunOnGameThread([pkt]()
		{
			NetStatus::Get().OnEnterRoom(pkt);
		});

	return true;
}

bool Handle_S_EXIT_ROOM(const Session* session, Protocol::S_EXIT_ROOM& pkt)
{
	Engine::Get().RunOnGameThread([]()
		{
			NetStatus::Get().OnExitRoom();
		});

	return true;
}

bool Handle_S_SPAWN(const Session* session, Protocol::S_SPAWN& pkt)
{
	Engine::Get().RunOnGameThread([pkt]()
		{
			NetStatus::Get().OnSpawn(pkt);
		});

	return true;
}

bool Handle_S_DESPAWN(const Session* session, Protocol::S_DESPAWN& pkt)
{
	Engine::Get().RunOnGameThread([pkt]()
		{
			NetStatus::Get().OnDespawn(pkt);
		});

	return true;
}
