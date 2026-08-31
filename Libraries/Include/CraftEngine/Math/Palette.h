#pragma once

#include "Math/Color.h"

NAME_SPACE_BEGIN(Craft)

// Color enum의 16개 슬롯에 실제 RGB를 대응시키는 팔레트.
//
// 콘솔은 동시에 16색만 쓸 수 있지만, 그 16개가 어떤 RGB인지는
// SetConsoleScreenBufferInfoEx의 ColorTable로 지정할 수 있다.
// 실제 적용은 ScreenBuffer::ApplyPalette()가 한다.
//
// 어디서든 접근할 수 있도록 정적 클래스로 만들었다.
// (Engine -> Renderer -> ScreenBuffer로 값을 넘기지 않아도 되게)
class CRAFT_API Palette
{
public:
	enum { ColorCount = 16, };

	// XML에서 팔레트를 읽어온다.
	// 파일이 없거나 파싱에 실패하면 기본 팔레트를 유지하고 false를 반환한다.
	// (색 설정 하나 때문에 게임이 아예 안 뜨는 상황을 막기 위함)
	static bool LoadFromFile(const WCHAR* path);

	// 슬롯 하나의 RGB 값
	static COLORREF Get(Color color);

	// ColorTable에 그대로 넘길 수 있는 16개짜리 배열
	static const COLORREF* GetTable();

private:
	static COLORREF table[ColorCount];
};

NAME_SPACE_END
