#pragma once

#include "Utils/EngineMacro.h"
#include <string>

NAME_SPACE_BEGIN(Craft)

// 애니메이션 한 장(프레임)에 해당하는 그림 데이터.
//
// Renderer::SubmitPixels()가 받는 형식을 그대로 들고 있다.
// 즉 '\n'으로 줄이 구분된 "기호 문자열"이고, 기호 -> 색 변환은
// 여기가 아니라 SymbolPalette가 공통으로 담당한다.
// 그림과 색을 분리해두면 같은 스프라이트를 다른 팔레트로 재활용할 수 있다.
//
// ★ 저장 형식 불변식 ★
// 생성자가 입력을 정규화해서 "width칸 + '\n'"이 height번 반복되는 형태로만 저장한다.
// 줄 끝 '\r'은 제거되고, 마지막 줄에도 '\n'이 붙는다.
// 덕분에 (row, col) 픽셀의 위치를 row * (width + 1) + col 로 바로 계산할 수 있고,
// 레이어 합성(AnimInstance::Composite)이 이 규칙에 기대고 있다.
class CRAFT_API Sprite
{
public:
	Sprite() = default;

	// explicit: 문자열이 실수로 Sprite로 암시 변환되는 걸 막는다.
	// (AnimationClip이 vector<Sprite>를 받기 때문에 실수하기 쉬운 자리)
	explicit Sprite(const std::string& pixelMap);

	~Sprite() = default;

	// Renderer::SubmitPixels()에 그대로 넘기는 값(정규화된 형태).
	inline const std::string& GetPixelMap() const { return pixelMap; }

	inline int GetWidth() const { return width; }
	inline int GetHeight() const { return height; }

	inline bool IsEmpty() const { return pixelMap.empty(); }

	// (row, col) 한 칸의 기호. 범위를 벗어나면 데이터/로직 실수이므로 크래시.
	char GetPixel(int row, int col) const;

	// 정규화된 픽셀맵에서 (row, col)이 놓인 문자 위치.
	// 합성기가 결과 버퍼에 직접 쓸 때도 같은 계산을 써야 해서 공개해 둔다.
	inline int GetPixelIndex(int row, int col) const { return row * (width + 1) + col; }

private:
	// '\n'으로 줄이 구분된 기호 문자열. 위의 불변식을 만족한다.
	std::string pixelMap;

	// 한 줄의 길이(칸 수). 모든 줄이 같은 길이다.
	int width = 0;

	// 줄 수.
	int height = 0;
};

NAME_SPACE_END
