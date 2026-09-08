#include "pch.h"
#include "Level/MenuLevel.h"

#include "Engine/Engine.h"
#include "Input/Input.h"
#include "Render/Renderer.h"
#include "Render/RenderLayer.h"
#include "Math/Color.h"

#include "Globals.h"
#include "UI/PixelText.h"

#include <cstring>
#include <string>
#include <unordered_map>

using namespace Craft;

namespace
{
	// --- 레이아웃 (콘솔 셀 단위) ---
	constexpr int kTitleY = 6;
	constexpr int kTitleScale = 2;
	constexpr int kTitleLineGap = 3;

	// 버튼: 타이틀과 같은 5x7 폰트를 배율 1 로.
	constexpr int kButtonScale = 1;
	constexpr int kButtonPadX = 2;
	constexpr int kButtonPadY = 1;
	constexpr int kButtonBlockGap = 8;
	constexpr int kButtonGapY = 3;

	const char* kStartLabel = "GAME START";
	const char* kExitLabel = "EXIT GAME";

	const std::unordered_map<char, Color>& TitlePalette()
	{
		static const std::unordered_map<char, Color> p = { { 'W', Color::Green } };
		return p;
	}
	const std::unordered_map<char, Color>& ButtonPalette(bool hot)
	{
		static const std::unordered_map<char, Color> normal = { { 'W', Color::White } };
		static const std::unordered_map<char, Color> hovered = { { 'W', Color::Black } };
		return hot ? hovered : normal;
	}
	const std::unordered_map<char, Color>& HighlightBgPalette()
	{
		static const std::unordered_map<char, Color> p = { { '#', Color::White } };
		return p;
	}

	int ButtonHeight()
	{
		return PixelText::Height(false) * kButtonScale + kButtonPadY * 2;
	}

	int FirstButtonY()
	{
		const int titleBlock = PixelText::kBigH * kTitleScale + kTitleLineGap + PixelText::kBigH * kTitleScale;
		return kTitleY + titleBlock + kButtonBlockGap;
	}

	Rect ButtonRect(const char* label, int screenW, int y)
	{
		const int textW = PixelText::Width(std::strlen(label), false) * kButtonScale;
		const int w = textW + kButtonPadX * 2;
		return Rect((screenW - w) / 2, y, w, ButtonHeight());
	}

	void SubmitTitleLine(const std::string& text, int screenW, int y)
	{
		const int widthCells = PixelText::Width(text.size(), false) * kTitleScale;
		Renderer::Get().SubmitPixels(
			PixelText::Make(text, 'W', false), TitlePalette(),
			Vector2((screenW - widthCells) / 2, y), RenderLayer::UI, '.', kTitleScale, kTitleScale);
	}

	void SubmitButton(const char* label, const Rect& rect, bool hot)
	{
		if (hot)
		{
			Renderer::Get().SubmitPixels(
				PixelText::SolidBlock(rect.size.x, rect.size.y, '#'), HighlightBgPalette(),
				rect.position, RenderLayer::UI - 1, ' ', 1, 1);
		}

		Renderer::Get().SubmitPixels(
			PixelText::Make(label, 'W', false), ButtonPalette(hot),
			Vector2(rect.position.x + kButtonPadX, rect.position.y + kButtonPadY),
			RenderLayer::UI, '.', kButtonScale, kButtonScale);
	}
}

void MenuLevel::OnInitialized()
{
	super::OnInitialized();

	// 아무것도 안 그려진 칸은 검게. (게임 레벨과 같은 기본값)
	Renderer::Get().SetClearColor(Color::Black);

	ComputeLayout();
}

void MenuLevel::ComputeLayout()
{
	const int screenW = Renderer::Get().GetScreenSize().x;
	const int y0 = FirstButtonY();

	_startRect = ButtonRect(kStartLabel, screenW, y0);
	_exitRect = ButtonRect(kExitLabel, screenW, y0 + ButtonHeight() + kButtonGapY);
}

void MenuLevel::Tick(float deltaTime)
{
	super::Tick(deltaTime);

	ComputeLayout();

	// 연결 개시 후에는 입력을 잠근다. 레벨 교체는 S_ENTER_ROOM 이 한다.
	if (_connecting)
	{
		return;
	}

	const Vector2 mouse = Input::Get().GetMousePosition();

	if (_startRect.Contains(mouse))
	{
		_hovered = Button::Start;
	}
	else if (_exitRect.Contains(mouse))
	{
		_hovered = Button::Exit;
	}
	else
	{
		_hovered = Button::None;
	}

	if (_hovered == Button::None || Input::Get().GetKeyDown(VK_LBUTTON) == false)
	{
		return;
	}

	if (_hovered == Button::Start)
	{
		_connecting = true;
		CreateInGameHud();
		StartServerConnection();
	}
	else
	{
		Engine::Get().Quit();
	}
}

void MenuLevel::Draw()
{
	super::Draw();

	ComputeLayout();

	const int screenW = Renderer::Get().GetScreenSize().x;

	// 타이틀 - 큰 픽셀 문자, 초록, 두 줄.
	SubmitTitleLine("TERMINAL", screenW, kTitleY);
	SubmitTitleLine("RAID", screenW, kTitleY + PixelText::kBigH * kTitleScale + kTitleLineGap);

	SubmitButton(kStartLabel, _startRect, _hovered == Button::Start);
	SubmitButton(kExitLabel, _exitRect, _hovered == Button::Exit);

	if (_connecting)
	{
		const char* msg = "connecting...";
		Renderer::Get().Submit(
			msg, Vector2((screenW - static_cast<int>(std::strlen(msg))) / 2, _exitRect.GetBottom() + 2),
			Color::White, RenderLayer::UI);
	}
}
