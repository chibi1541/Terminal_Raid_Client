#pragma once
#include "pch.h"
#include <iostream>
#include "Engine/Engine.h"
#include "Level/Level.h"
#include "Render/Renderer.h"
#include "Actor/TestActor.h"
#include "Actor/Monster.h"
#include "Actor/ProjectileActor.h"
#include "Asset/AssetManager.h"
#include "Asset/AnimationDataAsset.h"
#include "Asset/LevelDataAsset.h"
#include "Asset/PropDataAsset.h"
#include "Level/TileMapLevel.h"

#include "UI/UISystem.h"
#include "UI/Border.h"
#include "UI/TextBlock.h"

#include "Protocol/ServerPacketHandler.h"

#include "Globals.h"
#include "Network/ServerSession.h"
#include "Network/NetStatus.h"
#include "Game/ObjectManager.h"
#include "Thread/ThreadManager.h"
#include <memory>

using namespace Craft;

int main(int argc, char* argv[])
{

	Engine engine;


	// 패킷 핸들러 Init
	ServerPacketHandler::Init();

	GService = std::make_unique<Craft::ServerService>(Craft::NetAddress(L"127.0.0.1", 7777), [](Craft::NetAddress address) 
		{
			return std::make_unique<ServerSession>(address);
		}
	);

	// 엔진이 쓰레드를 Join()하기 직전에 네트워크 쓰레드에 정지 신호를 보낸다.
	//
	// 이걸 등록하지 않으면 ServerService::Run()이 무한 루프라서
	// 게임을 종료해도 Join()이 영영 돌아오지 않는다(프로세스가 매달린다).
	// Start()보다 먼저 등록해 두면 어느 경로로 나가든 신호가 간다.
	engine.AddShutdownHandler([]()
		{
			if (GService)
			{
				GService->Stop();
			}
		}
	);

	ASSERT_CRASH(GService->Start());

	engine.GetThreadManager()->Launch([]()
		{
			GService->Run();
		}
	);

	// 화면 전체 배경색 설정 (아무것도 안 그려진 칸이 이 색으로 남음)
	//
	// 타일맵이 쓰는 기호 아홉 중 Black에 대응하는 것이 하나도 없다.
	// 그래서 검은 칸을 "지형이 안 칠해진 칸"으로 바로 읽을 수 있다.
	// (예전에 쓰던 초록은 최다 기호 G(Color::Green)와 같아서
	//  타일맵이 그려졌는지 안 그려졌는지를 구분할 수 없다)
	Renderer::Get().SetClearColor(Color::Black);

	// Primary Data Asset 로드
	AssetManager::Get().RegisterPrimaryAssetType<AnimationDataAsset>("AnimationData");
	AssetManager::Get().RegisterPrimaryAssetType<LevelDataAsset>("LevelData");
	AssetManager::Get().RegisterPrimaryAssetType<PropDataAsset>("PropData");
	AssetManager::Get().LoadPrimaryAssetManifest(L"../Assets/PrimaryAssets.xml");
	// 프롭 배치는 Assets/Cemetery.LevelLayout.xml에 있다.
	// TileMapLevel이 그 파일을 읽어서 세운다 - 여기서 스폰할 것이 없다.
	Engine::Get().AddNewLevel<TileMapLevel>();

	// 플레이어 액터는 여기서 만들지 않는다.
	// 서버가 S_ENTER_ROOM으로 알려준 정보대로 ObjectManager가 스폰한다.
	// (TestActor는 엔진 입력/UI 회귀 테스트용으로 남겨둔다. 필요하면 여기서 다시 스폰)

	// --- 상시 표시되는 안내 HUD ---
	//
	// 입력을 받지 않는 평범한 위젯이다(UserWidget이 아니다).
	// 일시정지 메뉴는 TestActor가 만들어서 연다 - 게임플레이가 자기 UI를 소유하는 구조다.
	//
	// 레벨이 없어도 이 UI만으로 화면이 그려진다.
	// (예전에는 Engine::Draw가 레벨이 없으면 통째로 빠져나가서
	//  renderer->Draw()조차 호출되지 않았다)
	auto label = UI::Widget::Create<UI::TextBlock>("M : menu", Color::Yellow);

	// 패널 배경 위에 얹으므로 글자 뒤에 깔 색을 패널과 같게 맞춘다.
	// 이걸 빼면 글자가 있는 칸만 배경이 검게 파인다.
	label->SetBackgroundColor(Color::DarkBlue);

	auto hud = UI::Widget::Create<UI::Border>();
	hud->SetBackgroundColor(Color::DarkBlue);
	hud->SetShowBorder(true);
	hud->SetBorderColor(Color::White);
	hud->SetPadding(UI::Margin(1));

	// 화면 전체를 채우지 않고 내용 크기만큼만 잡아서 오른쪽 위에 붙인다.
	// (루트 위젯의 정렬 기본값은 Fill이라 그냥 두면 화면을 꽉 채운다)
	hud->SetHorizontalAlignment(UI::EHorizontalAlignment::Right);
	hud->SetVerticalAlignment(UI::EVerticalAlignment::Top);

	hud->SetContent(label);

	UI::UISystem::Get().AddToViewport(hud);

	// --- 네트워크 상태 보드(검증용) ---
	//
	// 화면의 "game thread"와 "job thread" 값이 같으면
	// 패킷 핸들러가 넘긴 잡이 실제로 게임 쓰레드에서 소비된 것이다.
	// 경계가 깨지거나 펌프가 안 돌면 여기서 바로 드러난다.
	//
	// Object 계층이 들어오면 이 보드는 역할을 넘기고 사라진다.
	auto netLabel = UI::Widget::Create<UI::TextBlock>("net : connecting", Color::White);
	netLabel->SetBackgroundColor(Color::DarkBlue);

	auto netBoard = UI::Widget::Create<UI::Border>();
	netBoard->SetBackgroundColor(Color::DarkBlue);
	netBoard->SetShowBorder(true);
	netBoard->SetBorderColor(Color::White);
	netBoard->SetPadding(UI::Margin(1));

	netBoard->SetContent(netLabel);

	// 화면 맨 위 두 줄(FPS 표시, 디버그 바)에 가려지지 않게 아래로 내린다.
	//
	// Border의 padding은 "안쪽" 여백이라 상자를 밀어내지 못한다.
	// 그 값을 키우면 상자만 커지고 테두리는 그대로 0행에 남는다.
	// 그래서 배경도 테두리도 없는(= 화면에 아무것도 안 그리는) Border로
	// 한 겹 감싸서 바깥 여백을 만든다.
	auto netAnchor = UI::Widget::Create<UI::Border>();
	netAnchor->SetPadding(UI::Margin(0, 2, 0, 0));

	// 기존 HUD가 오른쪽 위에 붙으므로 이쪽은 왼쪽 위로 보낸다.
	netAnchor->SetHorizontalAlignment(UI::EHorizontalAlignment::Left);
	netAnchor->SetVerticalAlignment(UI::EVerticalAlignment::Top);

	netAnchor->SetContent(netBoard);

	UI::UISystem::Get().AddToViewport(netAnchor);

	// 반드시 메인 쓰레드에서 부른다.
	// 이때의 쓰레드 ID가 "게임 쓰레드"로 기록되어 이후 잡 호출을 검사하는 기준이 된다.
	NetStatus::Get().BindTextBlock(netLabel);

	// 서버가 보낸 스폰/디스폰을 반영할 주체.
	// NetStatus와 같은 이유로 메인 쓰레드에서 기준 쓰레드 ID를 기록해 둔다.
	ObjectManager::Get().BindGameThread();

	//temp
	std::shared_ptr<Monster> monster = Engine::Get().GetLevel()->SpawnActor<Monster>();
	monster->SetPosition(Craft::Vector2(200, 130));
	std::shared_ptr<ProjectileActor> projectile = Engine::Get().GetLevel()->SpawnActor<ProjectileActor>();
	projectile->SetPosition(Craft::Vector2(170, 130));

	Engine::Get().Run();
}
