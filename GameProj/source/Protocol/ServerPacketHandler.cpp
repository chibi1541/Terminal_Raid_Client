#include "pch.h"
#include "ServerPacketHandler.h"
#include "Network/NetStatus.h"
#include "Network/NetSend.h"
#include "Game/ObjectManager.h"

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

			// 로그인만으로는 룸에 못 들어간다. 서버는 C_ENTER_ROOM을 받아야
			// Room::Enter를 돌리고 그 결과로 S_ENTER_ROOM을 돌려준다.
			Protocol::C_ENTER_ROOM enterPkt;
			SendToServer(enterPkt);
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
			// 액터 배치가 먼저다. NetStatus가 개체 수를 ObjectManager에서 읽는다.
			ObjectManager::Get().OnEnterRoom(pkt);
			NetStatus::Get().OnEnterRoom(pkt);
		});

	return true;
}

bool Handle_S_EXIT_ROOM(const Session* session, Protocol::S_EXIT_ROOM& pkt)
{
	Engine::Get().RunOnGameThread([]()
		{
			ObjectManager::Get().OnExitRoom();
			NetStatus::Get().OnExitRoom();
		});

	return true;
}

bool Handle_S_SPAWN(const Session* session, Protocol::S_SPAWN& pkt)
{
	Engine::Get().RunOnGameThread([pkt]()
		{
			ObjectManager::Get().OnSpawn(pkt);
			NetStatus::Get().OnSpawn(pkt);
		});

	return true;
}

bool Handle_S_DESPAWN(const Session* session, Protocol::S_DESPAWN& pkt)
{
	Engine::Get().RunOnGameThread([pkt]()
		{
			ObjectManager::Get().OnDespawn(pkt);
			NetStatus::Get().OnDespawn(pkt);
		});

	return true;
}
