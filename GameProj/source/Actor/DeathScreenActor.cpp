#include "pch.h"
#include "Actor/DeathScreenActor.h"

#include "Engine/Engine.h"
#include "Input/Input.h"
#include "Render/Renderer.h"
#include "Render/RenderLayer.h"
#include "Math/Color.h"

#include "UI/PixelText.h"
#include "Game/ObjectManager.h"
#include "Actor/LocalPlayer.h"
#include "Network/NetSend.h"
#include "Protocol/Protocol.pb.h"

#include <cstring>
#include <string>
#include <unordered_map>

using namespace Craft;

namespace
{
	// 전부 5x7 큰 폰트, 배율 1.
	constexpr int kScale = 1;
	constexpr int kButtonPadX = 2;
	constexpr int kButtonPadY = 1;
	constexpr int kPanelPad = 3;
	constexpr int kTitleGapY = 3;

	const char* kTitle = "YOU DIED";
	const char* kButtonLabel = "RESPAWN";

	const std::unordered_map<char, Color>& TitlePalette()
	{
		static const std::unordered_map<char, Color> p = { { 'W', Color::Red } };
		return p;
	}
	const std::unordered_map<char, Color>& ButtonTextPalette(bool hot)
	{
		static const std::unordered_map<char, Color> normal = { { 'W', Color::White } };
		static const std::unordered_map<char, Color> hovered = { { 'W', Color::Black } };
		return hot ? hovered : normal;
	}
	const std::unordered_map<char, Color>& PanelBgPalette()
	{
		static const std::unordered_map<char, Color> p = { { '#', Color::DarkBlue } };
		return p;
	}
	const std::unordered_map<char, Color>& PanelBorderPalette()
	{
		static const std::unordered_map<char, Color> p = { { '#', Color::White } };
		return p;
	}

	int TitleWidthPx()  { return PixelText::Width(std::strlen(kTitle), false) * kScale; }
	int ButtonWidthPx() { return PixelText::Width(std::strlen(kButtonLabel), false) * kScale; }
	int GlyphHeightPx() { return PixelText::Height(false) * kScale; }
}

void DeathScreenActor::BeginPlay()
{
	// 화면 고정 오버레이 - 카메라가 원점에서 멀어져도 컬링되지 않도록.
	shouldDraw = true;
	SetUsesDepthSorting(false);

	super::BeginPlay();

	ComputeLayout();
}

void DeathScreenActor::ComputeLayout()
{
	const Vector2 screen = Renderer::Get().GetScreenSize();

	const int innerW = (TitleWidthPx() > ButtonWidthPx()) ? TitleWidthPx() : ButtonWidthPx();
	const int btnH = GlyphHeightPx() + kButtonPadY * 2;

	const int panelW = innerW + kPanelPad * 2;
	const int panelH = GlyphHeightPx() + kTitleGapY + btnH + kPanelPad * 2;

	const int panelX = (screen.x - panelW) / 2;
	const int panelY = (screen.y - panelH) / 2;
	_panelRect = Rect(panelX, panelY, panelW, panelH);

	const int btnW = ButtonWidthPx() + kButtonPadX * 2;
	const int btnY = panelY + kPanelPad + GlyphHeightPx() + kTitleGapY;
	_respawnRect = Rect((screen.x - btnW) / 2, btnY, btnW, btnH);
}

void DeathScreenActor::Tick(float deltaTime)
{
	super::Tick(deltaTime);

	std::shared_ptr<LocalPlayer> local = ObjectManager::Get().GetLocalPlayer();

	if (local == nullptr || local->IsDeadState() == false)
	{
		// 부활 완료(또는 아직 안 죽음) - 다음 죽음을 위해 리셋.
		_requested = false;
		_hovered = false;
		return;
	}

	// 죽었지만 아직 Death 클립 재생 중 - UI 를 아직 안 띄운다.
	if (local->IsDeathAnimDone() == false)
	{
		_hovered = false;
		return;
	}

	ComputeLayout();

	_hovered = _respawnRect.Contains(Input::Get().GetMousePosition());

	if (_requested == false && _hovered && Input::Get().GetKeyDown(VK_LBUTTON))
	{
		Protocol::C_RESPAWN pkt;
		SendToServer(pkt);
		_requested = true;
	}
}

void DeathScreenActor::Draw()
{
	super::Draw();

	std::shared_ptr<LocalPlayer> local = ObjectManager::Get().GetLocalPlayer();
	if (local == nullptr || local->IsDeathAnimDone() == false)
		return;

	ComputeLayout();

	Renderer& r = Renderer::Get();

	// 패널: 흰 테두리(한 겹 크게) + DarkBlue 배경. 패널 밖은 게임 화면 그대로.
	r.SubmitPixels(
		PixelText::SolidBlock(_panelRect.size.x + 2, _panelRect.size.y + 2, '#'), PanelBorderPalette(),
		Vector2(_panelRect.position.x - 1, _panelRect.position.y - 1), RenderLayer::UI + 100, ' ', 1, 1);
	r.SubmitPixels(
		PixelText::SolidBlock(_panelRect.size.x, _panelRect.size.y, '#'), PanelBgPalette(),
		_panelRect.position, RenderLayer::UI + 101, ' ', 1, 1);

	// 타이틀 "YOU DIED" (빨강).
	const int screenW = r.GetScreenSize().x;
	r.SubmitPixels(
		PixelText::Make(kTitle, 'W', false), TitlePalette(),
		Vector2((screenW - TitleWidthPx()) / 2, _panelRect.position.y + kPanelPad),
		RenderLayer::UI + 102, '.', kScale, kScale);

	if (_requested)
	{
		// 전송 후 대기 - 버튼 대신 안내 텍스트.
		const char* msg = "RESPAWNING...";
		r.Submit(msg, Vector2((screenW - static_cast<int>(std::strlen(msg))) / 2,
			_respawnRect.position.y + _respawnRect.size.y / 2),
			Color::White, RenderLayer::UI + 103);
		return;
	}

	if (_hovered)
	{
		r.SubmitPixels(
			PixelText::SolidBlock(_respawnRect.size.x, _respawnRect.size.y, '#'), PanelBorderPalette(),
			_respawnRect.position, RenderLayer::UI + 102, ' ', 1, 1);
	}
	r.SubmitPixels(
		PixelText::Make(kButtonLabel, 'W', false), ButtonTextPalette(_hovered),
		Vector2(_respawnRect.position.x + kButtonPadX, _respawnRect.position.y + kButtonPadY),
		RenderLayer::UI + 103, '.', kScale, kScale);
}
