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
#include "Level/MenuLevel.h"

#include "UI/UISystem.h"
#include "UI/Border.h"
#include "UI/TextBlock.h"

#include "Protocol/ServerPacketHandler.h"

#include "Globals.h"
#include "Network/ServerSession.h"
#include "Network/NetStatus.h"
#include "Game/ObjectManager.h"
#include "Game/ActorDataAsset.h"
#include "Thread/ThreadManager.h"
#include <memory>
#include <string>
#include <cstring>
#include <cstdlib>

using namespace Craft;

namespace
{
	// 커맨드 라인으로 서버 주소를 못 받았을 때 쓰는 기본값.
	constexpr const wchar_t* kDefaultServerIp = L"172.16.30.188";
	constexpr uint16 kDefaultServerPort = 7777;

	std::wstring Widen(const std::string& s)
	{
		// IP / 포트는 ASCII 라 단순 확장으로 충분하다.
		return std::wstring(s.begin(), s.end());
	}

	// 실행 인자로 접속할 서버 주소를 정한다.
	//   Client.exe <ip> [port]
	//   Client.exe <ip:port>
	// 빠진 값은 기본값으로 채운다.
	Craft::NetAddress ParseServerAddress(int argc, char* argv[])
	{
		std::wstring ip = kDefaultServerIp;
		uint16 port = kDefaultServerPort;

		if (argc >= 2 && argv[1] != nullptr && argv[1][0] != '\0')
		{
			const std::string first = argv[1];
			const size_t colon = first.find(':');
			if (colon != std::string::npos)
			{
				ip = Widen(first.substr(0, colon));
				port = static_cast<uint16>(std::atoi(first.substr(colon + 1).c_str()));
			}
			else
			{
				ip = Widen(first);
			}
		}

		if (argc >= 3 && argv[2] != nullptr && argv[2][0] != '\0')
		{
			port = static_cast<uint16>(std::atoi(argv[2]));
		}

		if (port == 0)
		{
			port = kDefaultServerPort;
		}

		std::wcout << L"[net] server " << ip << L":" << port << std::endl;
		return Craft::NetAddress(ip, port);
	}
}

// 인게임 HUD 위젯을 만들어 뷰포트에 올린다.
//
// 메뉴 화면에는 뜨면 안 되므로 부팅 시 만들지 않는다. MenuLevel 이 Game Start 를
// 처리할 때(게임 = 메인 쓰레드) 부른다. AddToViewport 는 기본 persistent=false 라
// 다음 레벨 교체 때 함께 정리된다 - 여기서는 곧바로 TileMapLevel 로 넘어가므로
// 그 교체보다 나중에 처리되어(ProcessPendingWidgets) 게임 레벨과 함께 살아남는다.
void CreateInGameHud()
{
	// --- 상시 표시되는 안내 HUD ---
	//
	// 입력을 받지 않는 평범한 위젯이다(UserWidget이 아니다).
	// 일시정지 메뉴는 TestActor가 만들어서 연다 - 게임플레이가 자기 UI를 소유하는 구조다.
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
	auto netLabel = UI::Widget::Create<UI::TextBlock>("net : connecting", Color::White);
	netLabel->SetBackgroundColor(Color::DarkBlue);

	auto netBoard = UI::Widget::Create<UI::Border>();
	netBoard->SetBackgroundColor(Color::DarkBlue);
	netBoard->SetShowBorder(true);
	netBoard->SetBorderColor(Color::White);
	netBoard->SetPadding(UI::Margin(1));

	netBoard->SetContent(netLabel);

	// 화면 맨 위 두 줄(FPS 표시, 디버그 바)에 가려지지 않게 아래로 내린다.
	auto netAnchor = UI::Widget::Create<UI::Border>();
	netAnchor->SetPadding(UI::Margin(0, 2, 0, 0));

	// 기존 HUD가 오른쪽 위에 붙으므로 이쪽은 왼쪽 위로 보낸다.
	netAnchor->SetHorizontalAlignment(UI::EHorizontalAlignment::Left);
	netAnchor->SetVerticalAlignment(UI::EVerticalAlignment::Top);

	netAnchor->SetContent(netBoard);

	UI::UISystem::Get().AddToViewport(netAnchor);

	// 반드시 게임(메인) 쓰레드에서 부른다.
	// 이때의 쓰레드 ID가 "게임 쓰레드"로 기록되어 이후 잡 호출을 검사하는 기준이 된다.
	NetStatus::Get().BindTextBlock(netLabel);
}

int main(int argc, char* argv[])
{

	Engine engine;


	// 패킷 핸들러 Init
	ServerPacketHandler::Init();

	GService = std::make_unique<Craft::ServerService>(ParseServerAddress(argc, argv), [](Craft::NetAddress address)
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

	// 서버 연결은 여기서 하지 않는다. 메뉴의 Game Start 가 StartServerConnection()으로 개시한다.
	// (예전에는 여기서 GService->Start() + 네트워크 쓰레드를 무조건 띄웠다)

	// 화면 전체 배경색 설정 (아무것도 안 그려진 칸이 이 색으로 남음)
	Renderer::Get().SetClearColor(Color::Black);

	// Primary Data Asset 로드
	AssetManager::Get().RegisterPrimaryAssetType<AnimationDataAsset>("AnimationData");
	AssetManager::Get().RegisterPrimaryAssetType<LevelDataAsset>("LevelData");
	AssetManager::Get().RegisterPrimaryAssetType<PropDataAsset>("PropData");
	AssetManager::Get().RegisterPrimaryAssetType<ActorDataAsset>("ActorData");
	AssetManager::Get().LoadPrimaryAssetManifest(L"../Assets/PrimaryAssets.xml");

	// 첫 화면은 서버 연결 전의 타이틀/메뉴 레벨.
	// Game Start -> 연결 개시. S_ENTER_ROOM 도착 시 ObjectManager 가 TileMapLevel 로 교체한다.
	Engine::Get().AddNewLevel<MenuLevel>();

	// 서버가 보낸 스폰/디스폰을 반영할 주체.
	// 메인 쓰레드에서 기준 쓰레드 ID를 기록해 둔다.
	ObjectManager::Get().BindGameThread();

	Engine::Get().Run();
}
