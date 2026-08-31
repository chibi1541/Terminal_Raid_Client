#pragma once
#include "pch.h"
#include <iostream>
#include "Engine/Engine.h"
#include "Level/Level.h"
#include "Render/Renderer.h"
#include "Actor/TestActor.h"
#include "Asset/AssetManager.h"
#include "Asset/AnimationDataAsset.h"

#include "UI/UISystem.h"
#include "UI/Border.h"
#include "UI/TextBlock.h"

#include "Protocol/ServerPacketHandler.h"

#include "Globals.h"
#include "Network/ServerSession.h"
#include "Thread/ThreadManager.h"
#include <memory>

using namespace Craft;

int main(int argc, char* argv[])
{

	Engine engine;


	// 패킷 핸들러 Init
	ServerPacketHandler::Init();

	GService = new Craft::ServerService(Craft::NetAddress(L"127.0.0.1", 7777), [](Craft::NetAddress address) 
		{
			return std::make_unique<ServerSession>(address);
		}
	);

	ASSERT_CRASH(GService->Start());

	engine.GetThreadManager()->Launch([=]()
		{
			GService->Run();
		}
	);

	// 화면 전체 배경색 설정 (아무것도 안 그려진 칸이 이 색으로 남음)
	//
	// UI 확인용으로 일부러 눈에 띄는 색을 쓴다.
	// 배경색 합성이나 클리핑이 깨지면 초록 위에서 바로 보인다.
	Renderer::Get().SetClearColor(Color::Green);

	// Primary Data Asset 로드
	AssetManager::Get().RegisterPrimaryAssetType<AnimationDataAsset>("AnimationData");
	AssetManager::Get().LoadPrimaryAssetManifest(L"../Assets/PrimaryAssets.xml");
	Engine::Get().AddNewLevel<Level>();
	Engine::Get().GetLevel()->SpawnActor<TestActor>(Vector2(60, 15), Color::Green);

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

	Engine::Get().Run();
}
