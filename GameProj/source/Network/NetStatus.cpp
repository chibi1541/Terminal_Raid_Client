#include "pch.h"
#include "NetStatus.h"

#include "Engine/Engine.h"
#include "Thread/ThreadManager.h"
#include "UI/TextBlock.h"

using namespace Craft;


NetStatus& NetStatus::Get()
{
	static NetStatus instance;
	return instance;
}

void NetStatus::BindTextBlock(const std::shared_ptr<Craft::UI::TextBlock>& textBlock)
{

	board = textBlock;

	// 여기를 부른 쓰레드가 게임 쓰레드다.
	// Game.cpp가 엔진 루프에 들어가기 전, 메인 쓰레드에서 부르는 것을 전제로 한다.
	gameThreadId = Engine::Get().GetThreadManager()->GetThreadID();


	Refresh();

}

void NetStatus::EnterGameThreadJob(const char* packetName)
{
	const uint32 currentThreadId = Engine::Get().GetThreadManager()->GetThreadID();


	// 네트워크 쓰레드에서 직접 불렀다는 뜻이다.
	// 이 시점에 잡아두지 않으면 나중에 액터를 만지기 시작했을 때
	// 재현이 안 되는 경합으로 나타난다.
	ASSERT_CRASH(gameThreadId == 0 || currentThreadId == gameThreadId);

	lastJobThreadId = currentThreadId;
	lastPacket = packetName;
	++packetCount;
}

void NetStatus::OnLogin(const Protocol::S_LOGIN& pkt)
{
	EnterGameThreadJob("S_LOGIN");

	// 로그인 실패는 게임 쓰레드에서 터뜨린다.
	// 네트워크 쓰레드에서 터지면 덤프의 콜스택이 패킷 파싱 내부라 읽기 나쁘다.
	ASSERT_CRASH(pkt.success());

	state = "login ok (" + pkt.user().name() + ")";

	Refresh();
}

void NetStatus::OnPong()
{
	EnterGameThreadJob("S_PONG");
	Refresh();
}

void NetStatus::OnEnterRoom(const Protocol::S_ENTER_ROOM& pkt)
{
	EnterGameThreadJob("S_ENTER_ROOM");

	if (pkt.success() == false)
	{
		state = "enter room failed";
		Refresh();
		return;
	}

	// TODO : Object 계층이 들어오면 myObject/objects로 실제 액터를 스폰한다.
	// 지금은 개수만 센다.
	objectCount = pkt.objects_size() + 1;

	state = "in room " + std::to_string(pkt.width()) + "x" + std::to_string(pkt.height());

	Refresh();
}

void NetStatus::OnExitRoom()
{
	EnterGameThreadJob("S_EXIT_ROOM");

	objectCount = 0;
	state = "left room";

	Refresh();
}

void NetStatus::OnSpawn(const Protocol::S_SPAWN& pkt)
{
	EnterGameThreadJob("S_SPAWN");

	// TODO : Object 계층이 들어오면 여기서 액터를 스폰한다.
	objectCount += pkt.objects_size();

	Refresh();
}

void NetStatus::OnDespawn(const Protocol::S_DESPAWN& pkt)
{
	EnterGameThreadJob("S_DESPAWN");

	// TODO : Object 계층이 들어오면 여기서 액터를 제거한다.
	objectCount -= pkt.objectids_size();

	if (objectCount < 0)
	{
		objectCount = 0;
	}

	Refresh();
}

void NetStatus::Refresh()
{
	std::shared_ptr<Craft::UI::TextBlock> textBlock = board.lock();

	if (textBlock == nullptr)
	{
	
		return;
	}

	// job 쓰레드 ID가 game 쓰레드 ID와 같아야 경계가 동작하는 것이다.
	std::string text;
	text += "net  : " + state + "\n";
	text += "pkt  : " + lastPacket + " x" + std::to_string(packetCount) + "\n";
	text += "obj  : " + std::to_string(objectCount) + "\n";
	text += "game thread : " + std::to_string(gameThreadId) + "\n";
	text += "job  thread : " + std::to_string(lastJobThreadId);

	textBlock->SetText(text);
}
