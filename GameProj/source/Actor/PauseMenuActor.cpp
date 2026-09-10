#include "pch.h"
#include "Actor/PauseMenuActor.h"

#include "Engine/Engine.h"
#include "Input/Input.h"
#include "Input/InputHandler.h"
#include "Render/Renderer.h"
#include "Render/RenderLayer.h"
#include "Math/Color.h"

#include "UI/PixelText.h"

#include <cstring>
#include <string>
#include <unordered_map>

using namespace Craft;

namespace
{
	// 버튼 픽셀 문자는 4x6 작은 폰트, 배율 1 (메인 메뉴 버튼보다 대략 20% 작다).
	constexpr int kButtonScale = 1;
	constexpr int kButtonPadX = 2;
	constexpr int kButtonPadY = 1;
	constexpr int kButtonGapY = 2;

	// 패널 안쪽 여백.
	constexpr int kPanelPad = 3;

	const char* kContinueLabel = "CONTINUE GAME";
	const char* kExitLabel = "EXIT GAME";

	const std::unordered_map<char, Color>& PanelBgPalette()
	{
		// 디버그 HUD 와 같은 색.
		static const std::unordered_map<char, Color> p = { { '#', Color::DarkBlue } };
		return p;
	}
	const std::unordered_map<char, Color>& PanelBorderPalette()
	{
		static const std::unordered_map<char, Color> p = { { '#', Color::White } };
		return p;
	}
	const std::unordered_map<char, Color>& ButtonTextPalette(bool hot)
	{
		static const std::unordered_map<char, Color> normal = { { 'W', Color::White } };
		static const std::unordered_map<char, Color> hovered = { { 'W', Color::Black } };
		return hot ? hovered : normal;
	}

	int ButtonWidth(const char* label)
	{
		return PixelText::Width(std::strlen(label), PixelText::Font::Small) * kButtonScale + kButtonPadX * 2;
	}
	int ButtonHeight()
	{
		return PixelText::Height(PixelText::Font::Small) * kButtonScale + kButtonPadY * 2;
	}

	void SubmitButton(const Rect& rect, const char* label, bool hot)
	{
		if (hot)
		{
			Renderer::Get().SubmitPixels(
				PixelText::SolidBlock(rect.size.x, rect.size.y, '#'), PanelBorderPalette(),
				rect.position, RenderLayer::UI + 102, ' ', 1, 1);
		}

		Renderer::Get().SubmitPixels(
			PixelText::Make(label, 'W', PixelText::Font::Small), ButtonTextPalette(hot),
			Vector2(rect.position.x + kButtonPadX, rect.position.y + kButtonPadY),
			RenderLayer::UI + 103, '.', kButtonScale, kButtonScale);
	}
}

void PauseMenuActor::BeginPlay()
{
	// InputComponent 등록은 super 보다 앞 (ServerDebugActor / LocalPlayer 와 같은 이유).
	_input = AddComponent<InputComponent>();
	_input->SetInputPriority(InputPriority::System);
	_input->BindKey(VK_ESCAPE, EInputEvent::Pressed, this, &PauseMenuActor::OnToggle);
	// 닫혀 있을 때는 ESC 만 소비하고 나머지는 통과. 열리면 SetOpen 이 blockAllInput 을 켠다.
	_input->SetBlockAllInput(false);

	// 카메라가 원점에서 멀어져도 컬링되지 않도록(위치가 (0,0) 인 화면 고정 오버레이).
	shouldDraw = true;
	SetUsesDepthSorting(false);

	super::BeginPlay();

	ComputeLayout();
}

void PauseMenuActor::OnToggle()
{
	SetOpen(!_open);
}

void PauseMenuActor::SetOpen(bool open)
{
	_open = open;
	_hovered = Button::None;

	if (_input != nullptr)
	{
		// 열려 있는 동안 플레이어(및 다른 하위 우선순위) 입력을 전부 막는다.
		_input->SetBlockAllInput(open);
	}
}

void PauseMenuActor::ComputeLayout()
{
	const Vector2 screen = Renderer::Get().GetScreenSize();

	const int contW = ButtonWidth(kContinueLabel);
	const int exitW = ButtonWidth(kExitLabel);
	const int btnH = ButtonHeight();
	const int innerW = (contW > exitW) ? contW : exitW;

	const int panelW = innerW + kPanelPad * 2;
	const int panelH = btnH * 2 + kButtonGapY + kPanelPad * 2;

	const int panelX = (screen.x - panelW) / 2;
	const int panelY = (screen.y - panelH) / 2;

	_panelRect = Rect(panelX, panelY, panelW, panelH);

	const int b1y = panelY + kPanelPad;
	const int b2y = b1y + btnH + kButtonGapY;

	_continueRect = Rect((screen.x - contW) / 2, b1y, contW, btnH);
	_exitRect = Rect((screen.x - exitW) / 2, b2y, exitW, btnH);
}

void PauseMenuActor::Tick(float deltaTime)
{
	super::Tick(deltaTime);

	if (_open == false)
	{
		return;
	}

	ComputeLayout();

	const Vector2 mouse = Input::Get().GetMousePosition();

	if (_continueRect.Contains(mouse))
	{
		_hovered = Button::Continue;
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

	if (_hovered == Button::Continue)
	{
		SetOpen(false);
	}
	else
	{
		Engine::Get().Quit();
	}
}

void PauseMenuActor::Draw()
{
	super::Draw();

	if (_open == false)
	{
		return;
	}

	ComputeLayout();

	Renderer& renderer = Renderer::Get();

	// 패널: 흰 테두리(한 겹 크게) 위에 DarkBlue 배경. 패널 밖은 게임 화면 그대로.
	renderer.SubmitPixels(
		PixelText::SolidBlock(_panelRect.size.x + 2, _panelRect.size.y + 2, '#'), PanelBorderPalette(),
		Vector2(_panelRect.position.x - 1, _panelRect.position.y - 1), RenderLayer::UI + 100, ' ', 1, 1);

	renderer.SubmitPixels(
		PixelText::SolidBlock(_panelRect.size.x, _panelRect.size.y, '#'), PanelBgPalette(),
		_panelRect.position, RenderLayer::UI + 101, ' ', 1, 1);

	SubmitButton(_continueRect, kContinueLabel, _hovered == Button::Continue);
	SubmitButton(_exitRect, kExitLabel, _hovered == Button::Exit);
}
