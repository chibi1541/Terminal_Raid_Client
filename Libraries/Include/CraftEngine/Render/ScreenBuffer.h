#pragma once

#include "Math/Vector2.h"

NAME_SPACE_BEGIN(Craft)

// 콘솔 셀(글자 한 칸)의 픽셀 크기와 폰트 이름.
//
// 콘솔 창이 가질 수 있는 최대 칸 수는 "화면 픽셀 / 셀 픽셀"로 정해진다.
// 즉 세로 줄 수를 늘리는 유일한 방법은 height를 줄이는 것이다.
// (2560x1440 화면에서 셀 높이 16px이면 85줄, 8px이면 170줄)
struct ConsoleFontDesc
{
	// 셀 너비(px). 0이면 자동(높이에서 폰트 고유 비율로 계산).
	//
	// 래스터 폰트("Terminal")는 너비와 높이를 요청한 값 그대로 지켜준다.
	// TrueType(Consolas 등)은 너비를 무시하고 항상 높이의 절반으로 잡는다.
	int width = 8;

	// 셀 높이(px). 세로 줄 수를 직접 결정하는 값.
	int height = 8;

	// 폰트 이름.
	//
	// L"Terminal"은 래스터 폰트로, 8x8 같은 정사각 셀을 정확히 지켜준다.
	// TrueType 계열은 너비:높이가 1:2로 고정이라 정사각 셀이 나오지 않는다.
	const wchar_t* faceName = L"Terminal";
};

class ScreenBuffer
{
public:
	ScreenBuffer(const Vector2& screenSize, const ConsoleFontDesc& fontDesc = ConsoleFontDesc());
	~ScreenBuffer();

	// 콘솔 초기화 - 화면 지우기
	void Clear() const;

	// Palette의 16색을 이 콘솔 버퍼의 ColorTable에 적용한다.
	void ApplyPalette() const;

	void Draw(const CHAR_INFO* const charInfo) const;

	// Getter
	inline HANDLE GetBuffer() const { return buffer; }

	// 실제로 잡힌 화면 크기.
	// 요청한 크기가 물리적 화면에 안 들어가면 생성자가 줄여서 잡으므로,
	// 요청값과 다를 수 있다.
	inline Vector2 GetSize() const { return size; }

	// 콘솔이 실제로 적용한 셀 픽셀 크기.
	//
	// 요청값과 다를 수 있다 - SetCurrentConsoleFontEx는 폰트가 대체돼도 TRUE를
	// 반환하기 때문에, 되읽지 않으면 어긋난 걸 알 방법이 없다.
	inline Vector2 GetCellPixelSize() const { return cellPixelSize; }

private:
	// 폰트를 적용하고, 실제로 적용된 셀 픽셀 크기를 되읽어서 돌려준다.
	Vector2 ApplyFont(const ConsoleFontDesc& fontDesc) const;

	// size를 화면에 들어가는 크기로 줄이고 버퍼/창을 잡는다.
	// 실패해도 크래쉬하지 않고, 마지막에 실제로 잡힌 크기를 size에 반영한다.
	void ResizeToFit();

public:
	// 콘솔 화면 펴버 핸들
	HANDLE buffer = nullptr;

	// 화면 크기
	Vector2 size;

	// 실제로 적용된 셀 픽셀 크기
	Vector2 cellPixelSize;
};

NAME_SPACE_END
