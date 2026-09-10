#include "pch.h"
#include "Level/MenuLevel.h"

#include "Engine/Engine.h"
#include "Input/Input.h"
#include "Render/Renderer.h"
#include "Render/RenderLayer.h"
#include "Math/Color.h"

#include "Globals.h"
#include "UI/PixelText.h"
#include "Actor/CrosshairActor.h"

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
	constexpr int kButtonGapY = 3;

	// 폼 글자는 3x5 아주 작은 픽셀 폰트, 배율 1 (GAME START 버튼 글자보다 확연히 작다).
	// 재목 블록 → 이름 라벨/값 → 캐릭터 라벨/컨트롤 → 버튼 순으로 쌓는다.
	constexpr int kFormGap = 4;			// 재목과 이름 라벨 사이
	constexpr int kLabelValueGap = 1;	// 라벨과 값 사이
	constexpr int kSectionGap = 2;		// 이름 섹션과 캐릭터 섹션 사이
	constexpr int kFormToButtonGap = 3;	// 폼과 첫 버튼 사이
	constexpr int kCharPartGap = 2;		// "<" / 이름 / ">" 사이

	constexpr size_t kMaxNameLen = 12;

	const char* kStartLabel = "GAME START";
	const char* kExitLabel = "EXIT GAME";

	// 캐릭터 이름 표시는 6글자로 고정(KNIGHT/ARCHER = 6, MAGE = 4). 가운데 정렬.
	constexpr int kCharNameChars = 6;

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
	// 폼 픽셀 글자 팔레트. ink 기호는 PixelText::Make 에 넘긴 'W'.
	const std::unordered_map<char, Color>& FormPalette(Color c)
	{
		static const std::unordered_map<char, Color> label = { { 'W', Color::Gray } };
		static const std::unordered_map<char, Color> text = { { 'W', Color::White } };
		static const std::unordered_map<char, Color> hot = { { 'W', Color::Yellow } };
		if (c == Color::Gray)   return label;
		if (c == Color::Yellow) return hot;
		return text;
	}

	int ButtonHeight()
	{
		return PixelText::Height(PixelText::Font::Big) * kButtonScale + kButtonPadY * 2;
	}

	int TitleBottomY()
	{
		return kTitleY
			+ PixelText::kBigH * kTitleScale + kTitleLineGap
			+ PixelText::kBigH * kTitleScale;
	}

	// 폼 글자용 아주 작은 폰트.
	int FormW() { return PixelText::kTinyW; }
	int FormH() { return PixelText::kTinyH; }
	constexpr PixelText::Font kFormFont = PixelText::Font::Tiny;

	Rect ButtonRect(const char* label, int screenW, int y)
	{
		const int textW = PixelText::Width(std::strlen(label), PixelText::Font::Big) * kButtonScale;
		const int w = textW + kButtonPadX * 2;
		return Rect((screenW - w) / 2, y, w, ButtonHeight());
	}

	const char* CharTypeName(int type)
	{
		switch (type)
		{
		case 2:  return "ARCHER";
		case 3:  return "MAGE";
		case 1:
		default: return "KNIGHT";
		}
	}

	void SubmitTitleLine(const std::string& text, int screenW, int y)
	{
		const int widthCells = PixelText::Width(text.size(), PixelText::Font::Big) * kTitleScale;
		Renderer::Get().SubmitPixels(
			PixelText::Make(text, 'W', PixelText::Font::Big), TitlePalette(),
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
			PixelText::Make(label, 'W', PixelText::Font::Big), ButtonPalette(hot),
			Vector2(rect.position.x + kButtonPadX, rect.position.y + kButtonPadY),
			RenderLayer::UI, '.', kButtonScale, kButtonScale);
	}

	// 작은 픽셀 글자를 x 위치에 그린다.
	void SubmitSmall(const std::string& text, int x, int y, Color color)
	{
		Renderer::Get().SubmitPixels(
			PixelText::Make(text, 'W', kFormFont), FormPalette(color),
			Vector2(x, y), RenderLayer::UI, '.', 1, 1);
	}

	// 작은 픽셀 글자를 화면 가운데에 그린다.
	void SubmitSmallCentered(const std::string& text, int screenW, int y, Color color)
	{
		SubmitSmall(text, (screenW - PixelText::Width(text.size(), kFormFont)) / 2, y, color);
	}
}

void MenuLevel::OnInitialized()
{
	super::OnInitialized();

	// 아무것도 안 그려진 칸은 검게. (게임 레벨과 같은 기본값)
	Renderer::Get().SetClearColor(Color::Black);

	// 마우스 십자선 오버레이.
	SpawnActor<CrosshairActor>();

	ComputeLayout();
}

void MenuLevel::ComputeLayout()
{
	const int screenW = Renderer::Get().GetScreenSize().x;

	_nameRowY = TitleBottomY() + kFormGap;								// 이름 라벨
	const int nameValueY = _nameRowY + FormH() + kLabelValueGap;		// 이름 값
	_charRowY = nameValueY + FormH() + kSectionGap;						// 캐릭터 라벨
	const int charCtrlY = _charRowY + FormH() + kLabelValueGap;			// 캐릭터 컨트롤

	const int y0 = charCtrlY + FormH() + kFormToButtonGap;

	_startRect = ButtonRect(kStartLabel, screenW, y0);
	_exitRect = ButtonRect(kExitLabel, screenW, y0 + ButtonHeight() + kButtonGapY);

	// 캐릭터 컨트롤 "<  NAME  >" - 좌우 삼각형 히트 박스.
	const int arrowW = PixelText::Width(1, kFormFont);
	const int fieldW = PixelText::Width(kCharNameChars, kFormFont);
	const int totalW = arrowW + kCharPartGap + fieldW + kCharPartGap + arrowW;
	const int ctrlX = (screenW - totalW) / 2;

	_charPrevRect = Rect(ctrlX - 1, charCtrlY - 1, arrowW + 2, FormH() + 2);
	_charNextRect = Rect(ctrlX + totalW - arrowW - 1, charCtrlY - 1, arrowW + 2, FormH() + 2);
}

void MenuLevel::PollNameInput()
{
	Input& in = Input::Get();

	if (in.GetKeyDown(VK_BACK) && _playerName.empty() == false)
	{
		_playerName.pop_back();
	}

	if (_playerName.size() >= kMaxNameLen)
	{
		return;
	}

	for (int vk = 'A'; vk <= 'Z'; ++vk)
	{
		if (in.GetKeyDown(vk))
			_playerName += static_cast<char>(vk);
	}
	for (int vk = '0'; vk <= '9'; ++vk)
	{
		if (in.GetKeyDown(vk))
			_playerName += static_cast<char>(vk);
	}
	// 선행/연속 공백은 막는다.
	if (in.GetKeyDown(VK_SPACE) && _playerName.empty() == false && _playerName.back() != ' ')
	{
		_playerName += ' ';
	}
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

	PollNameInput();

	const Vector2 mouse = Input::Get().GetMousePosition();

	if (_startRect.Contains(mouse))
		_hovered = Button::Start;
	else if (_exitRect.Contains(mouse))
		_hovered = Button::Exit;
	else if (_charPrevRect.Contains(mouse))
		_hovered = Button::CharPrev;
	else if (_charNextRect.Contains(mouse))
		_hovered = Button::CharNext;
	else
		_hovered = Button::None;

	if (_hovered == Button::None || Input::Get().GetKeyDown(VK_LBUTTON) == false)
	{
		return;
	}

	switch (_hovered)
	{
	case Button::Start:
		GLoginRequest.name = _playerName;	// 빈 문자열이면 ServerSession 이 "Player" 로 채운다
		GLoginRequest.charType = _charType;
		_connecting = true;
		CreateInGameHud();
		StartServerConnection();
		break;

	case Button::Exit:
		Engine::Get().Quit();
		break;

	case Button::CharNext:
		_charType = _charType % 3 + 1;		// Knight -> Archer -> Mage -> Knight
		break;

	case Button::CharPrev:
		_charType = (_charType + 1) % 3 + 1;	// Knight -> Mage -> Archer -> Knight
		break;

	default:
		break;
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

	// --- 플레이어 이름 ---
	SubmitSmallCentered("PLAYER NAME", screenW, _nameRowY, Color::Gray);
	// 커서(_) 를 붙여 빈 칸에도 입력 위치가 보이게 한다.
	SubmitSmallCentered(_playerName + "_", screenW, _nameRowY + FormH() + kLabelValueGap, Color::White);

	// --- 캐릭터 타입 ---
	SubmitSmallCentered("CHARACTER TYPE", screenW, _charRowY, Color::Gray);

	// "<  NAME  >" - NAME 은 6글자 가운데 정렬. 좌우 삼각형은 호버 시 노랑.
	std::string field = CharTypeName(_charType);
	const int pad = kCharNameChars - static_cast<int>(field.size());
	const int left = pad / 2;
	const std::string centeredField =
		std::string(left, ' ') + field + std::string(static_cast<size_t>(pad - left), ' ');

	const int arrowW = PixelText::Width(1, kFormFont);
	const int fieldW = PixelText::Width(kCharNameChars, kFormFont);
	const int totalW = arrowW + kCharPartGap + fieldW + kCharPartGap + arrowW;
	const int ctrlX = (screenW - totalW) / 2;
	const int charCtrlY = _charRowY + FormH() + kLabelValueGap;

	SubmitSmall("<", ctrlX, charCtrlY, _hovered == Button::CharPrev ? Color::Yellow : Color::White);
	SubmitSmall(centeredField, ctrlX + arrowW + kCharPartGap, charCtrlY, Color::White);
	SubmitSmall(">", ctrlX + totalW - arrowW, charCtrlY, _hovered == Button::CharNext ? Color::Yellow : Color::White);

	// --- 버튼 ---
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
