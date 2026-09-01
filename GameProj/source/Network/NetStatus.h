#pragma once

#include "Protocol/Protocol.pb.h"

#include <memory>
#include <string>

NAME_SPACE_BEGIN(Craft)
namespace UI
{
	class TextBlock;
}
NAME_SPACE_END

// 서버에서 온 패킷이 실제로 게임 쓰레드에서 처리되는지 눈으로 확인하기 위한 상태 보드.
//
// 다음 단계에서 만들 Object 계층(objectId -> Actor 매핑)이 들어오면
// 여기 있는 On... 함수들의 몸통은 그쪽으로 옮겨가고 이 클래스는 순수 HUD로 남거나 사라진다.
// 지금은 "패킷 핸들러 -> 게임 쓰레드" 경계가 동작하는지 검증하는 것이 유일한 목적이다.
//
// !! 이 클래스의 모든 함수는 게임(메인) 쓰레드 전용이다. !!
// 네트워크 쓰레드에서 직접 부르면 안 되고, 반드시
// Engine::RunOnGameThread()를 통해 넘어와야 한다.
// BindTextBlock()이 기록해 둔 쓰레드 ID와 비교해서 위반을 즉시 잡는다.
class NetStatus
{
public:
	static NetStatus& Get();

	// 상태를 표시할 위젯을 물려준다. 반드시 게임 쓰레드에서 부른다.
	// 이때의 쓰레드 ID를 "게임 쓰레드"로 기억해 이후 호출을 검사한다.
	void BindTextBlock(const std::shared_ptr<Craft::UI::TextBlock>& textBlock);

	// 아래는 전부 패킷 핸들러가 게임 쓰레드로 넘긴 잡 안에서 호출된다.
	void OnLogin(const Protocol::S_LOGIN& pkt);
	void OnPong();
	void OnEnterRoom(const Protocol::S_ENTER_ROOM& pkt);
	void OnExitRoom();
	void OnSpawn(const Protocol::S_SPAWN& pkt);
	void OnDespawn(const Protocol::S_DESPAWN& pkt);

private:
	// 게임 쓰레드에서 불렸는지 확인하고, 이번 잡을 실행한 쓰레드 ID를 기록한다.
	// 모든 On... 함수의 첫 줄에서 부른다.
	void EnterGameThreadJob(const char* packetName);

	// 현재 상태를 위젯 텍스트로 반영한다.
	void Refresh();

private:
	// UISystem이 위젯을 소유하므로 여기서는 약참조만 든다.
	std::weak_ptr<Craft::UI::TextBlock> board;

	// BindTextBlock을 부른 쓰레드 = 게임 쓰레드.
	// 0이면 아직 바인드 전이라 검사를 하지 않는다.
	uint32 gameThreadId = 0;

	// 마지막으로 잡을 실행한 쓰레드 ID.
	//
	// 이 값이 gameThreadId와 같게 화면에 찍히면 경계가 제대로 동작하는 것이다.
	// (네트워크 쓰레드에서 그냥 처리했다면 다른 값이 찍힌다)
	uint32 lastJobThreadId = 0;

	std::string state = "connecting";
	std::string lastPacket = "-";

	int packetCount = 0;
};
