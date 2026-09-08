#include "pch.h"
#include "Level/MenuLevel.h"

#include "Engine/Engine.h"
#include "Input/Input.h"
#include "Render/Renderer.h"
#include "Render/RenderLayer.h"
#include "Math/Color.h"

#include "Globals.h"

#include <array>
#include <cctype>
#include <cstring>
#include <string>
#include <unordered_map>

using namespace Craft;

namespace
{
	// ---------------------------------------------------------------------------
	// 5x7 대문자 블록 폰트. 타이틀 + 버튼에 나오는 글자만:
	//   TERMINAL RAID / GAME START / EXIT GAME -> A D E G I L M N R S T X + 공백.
	// ---------------------------------------------------------------------------
	constexpr int kGlyphW = 5;
	constexpr int kGlyphH = 7;
	constexpr int kGlyphGap = 1;

	using Glyph = std::array<const char*, kGlyphH>;

	const std::unordered_map<char, Glyph>& Font()
	{
		static const std::unordered_map<char, Glyph> table = {
			{ ' ', { ".....", ".....", ".....", ".....", ".....", ".....", "....." } },
			{ 'A', { ".###.", "#...#", "#...#", "#####", "#...#", "#...#", "#...#" } },
			{ 'D', { "####.", "#...#", "#...#", "#...#", "#...#", "#...#", "####." } },
			{ 'E', { "#####", "#....", "#....", "####.", "#....", "#....", "#####" } },
			{ 'G', { ".####", "#....", "#....", "#.###", "#...#", "#...#", ".####" } },
			{ 'I', { "#####", "..#..", "..#..", "..#..", "..#..", "..#..", "#####" } },
			{ 'L', { "#....", "#....", "#....", "#....", "#....", "#....", "#####" } },
			{ 'M', { "#...#", "##.##", "#.#.#", "#.#.#", "#...#", "#...#", "#...#" } },
			{ 'N', { "#...#", "##..#", "#.#.#", "#.#.#", "#..##", "#...#", "#...#" } },
			{ 'R', { "####.", "#...#", "#...#", "####.", "#.#..", "#..#.", "#...#" } },
			{ 'S', { ".####", "#....", "#....", ".###.", "....#", "....#", "####." } },
			{ 'T', { "#####", "..#..", "..#..", "..#..", "..#..", "..#..", "..#.." } },
			{ 'X', { "#...#", "#...#", ".#.#.", "..#..", ".#.#.", "#...#", "#...#" } },
		};
		return table;
	}

	std::string MakePixelText(const std::string& text, char ink)
	{
		std::array<std::string, kGlyphH> rows;

		for (size_t i = 0; i < text.size(); ++i)
		{
			const char upper = static_cast<char>(std::toupper(static_cast<unsigned char>(text[i])));

			auto it = Font().find(upper);
			const Glyph& glyph = (it != Font().end()) ? it->second : Font().at(' ');

			for (int r = 0; r < kGlyphH; ++r)
			{
				for (int c = 0; c < kGlyphW; ++c)
				{
					rows[r] += (glyph[r][c] == '#') ? ink : '.';
				}
				if (i + 1 < text.size())
				{
					rows[r].append(kGlyphGap, '.');
				}
			}
		}

		std::string out;
		for (int r = 0; r < kGlyphH; ++r)
		{
			out += rows[r];
			if (r + 1 < kGlyphH)
			{
				out += '\n';
			}
		}
		return out;
	}

	int PixelTextWidth(size_t charCount)
	{
		if (charCount == 0)
		{
			return 0;
		}
		return static_cast<int>(charCount) * kGlyphW + (static_cast<int>(charCount) - 1) * kGlyphGap;
	}

	// w x h 를 전부 채운 픽셀맵(하이라이트 배경용).
	std::string SolidBlock(int w, int h, char ink)
	{
		std::string row(static_cast<size_t>(w > 0 ? w : 0), ink);
		std::string out;
		for (int r = 0; r < h; ++r)
		{
			out += row;
			if (r + 1 < h)
			{
				out += '\n';
			}
		}
		return out;
	}

	const std::unordered_map<char, Color>& InkPalette(char key, Color color)
	{
		// key 는 항상 'W'(잉크) 또는 '#'(배경). 두 팔레트만 있으면 된다.
		static const std::unordered_map<char, Color> title = { { 'W', Color::Green } };
		static const std::unordered_map<char, Color> white = { { 'W', Color::White } };
		static const std::unordered_map<char, Color> highlight = { { 'W', Color::Black } };
		static const std::unordered_map<char, Color> bg = { { '#', Color::White } };

		if (key == '#') return bg;
		if (color == Color::Green) return title;
		if (color == Color::Black) return highlight;
		return white;
	}

	// --- 레이아웃 (콘솔 셀 단위) ---
	constexpr int kTitleY = 6;
	constexpr int kTitleScale = 2;
	constexpr int kTitleLineGap = 3;

	// 버튼: 타이틀과 같은 픽셀 폰트를 배율 1 로. (예전 1셀 텍스트의 약 5배 크기)
	constexpr int kButtonScale = 1;
	constexpr int kButtonPadX = 2;
	constexpr int kButtonPadY = 1;
	constexpr int kButtonBlockGap = 8;
	constexpr int kButtonGapY = 3;

	const char* kStartLabel = "GAME START";
	const char* kExitLabel = "EXIT GAME";

	int ButtonHeight()
	{
		return kGlyphH * kButtonScale + kButtonPadY * 2;
	}

	int FirstButtonY()
	{
		const int titleBlock = kGlyphH * kTitleScale + kTitleLineGap + kGlyphH * kTitleScale;
		return kTitleY + titleBlock + kButtonBlockGap;
	}

	Rect ButtonRect(const char* label, int screenW, int y)
	{
		const int textW = PixelTextWidth(std::strlen(label)) * kButtonScale;
		const int w = textW + kButtonPadX * 2;
		return Rect((screenW - w) / 2, y, w, ButtonHeight());
	}

	void SubmitTitleLine(const std::string& text, int screenW, int y)
	{
		const int widthCells = PixelTextWidth(text.size()) * kTitleScale;
		Renderer::Get().SubmitPixels(
			MakePixelText(text, 'W'), InkPalette('W', Color::Green),
			Vector2((screenW - widthCells) / 2, y), RenderLayer::UI, '.', kTitleScale, kTitleScale);
	}

	void SubmitButton(const char* label, const Rect& rect, bool hot)
	{
		if (hot)
		{
			Renderer::Get().SubmitPixels(
				SolidBlock(rect.size.x, rect.size.y, '#'), InkPalette('#', Color::White),
				rect.position, RenderLayer::UI - 1, ' ', 1, 1);
		}

		Renderer::Get().SubmitPixels(
			MakePixelText(label, 'W'), InkPalette('W', hot ? Color::Black : Color::White),
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
	SubmitTitleLine("RAID", screenW, kTitleY + kGlyphH * kTitleScale + kTitleLineGap);

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
