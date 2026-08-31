#pragma once

#include "Utils/EngineMacro.h"
#include "Utils/Types.h"
#include "Math/Color.h"
#include <unordered_map>

NAME_SPACE_BEGIN(Craft)

// 픽셀맵 기호(대문자 한 글자) -> Color 변환표.
//
// 스프라이트는 ".DLLKL.D" 같은 기호 문자열로 그려지는데,
// 그 기호가 어떤 색인지를 정하는 게 이 표다.
// Color enum의 16색에 각각 대문자 하나씩을 붙여서 전부 바인딩해 뒀다.
//
// 주의 - Math/Palette와는 다른 개념이다.
//   Palette       : Color 슬롯(0~15) <-> 실제 RGB      (Config/Palette.xml)
//   SymbolPalette : 픽셀맵 기호      <-> Color 슬롯    (여기, 코드에 고정)
// 기호 체계는 스프라이트 데이터가 전부 공유해야 하므로 코드에 고정한다.
// 색감 조정은 Palette.xml의 RGB만 바꾸면 되고, 기호는 건드릴 필요가 없다.
//
// 어디서든 접근할 수 있도록 정적 클래스로 만들었다. (Palette와 같은 방식)
class CRAFT_API SymbolPalette
{
public:
	// 투명(그리지 않고 건너뜀)을 뜻하는 기호. 색이 아니라서 표에는 없다.
	static constexpr char TransparentSymbol = '.';

	// Renderer::SubmitPixels()에 그대로 넘길 수 있는 변환표.
	static const std::unordered_map<char, Color>& GetTable();

	// 표에 있는 기호인지 확인. 투명 기호('.')는 false다.
	static bool Contains(char symbol);

	// 기호에 대응하는 색. 없는 기호면 데이터 실수이므로 크래시.
	static Color Get(char symbol);

private:
	SymbolPalette() = delete;
};

NAME_SPACE_END
