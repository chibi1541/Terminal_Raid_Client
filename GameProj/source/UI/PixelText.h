#pragma once

#include <string>

// 팔레트 기호 픽셀맵으로 큰 글자를 그리는 유틸.
//
// Renderer::SubmitPixels 에 그대로 넘길 수 있는 문자열('\n' 줄구분, '.'=투명)을 만든다.
// 콘솔 폰트가 없는 이 엔진에서 "픽셀 문자" 타이틀/버튼을 그리는 유일한 수단이다.
// 메인 메뉴(MenuLevel)와 일시정지 메뉴(PauseMenuActor)가 공유한다.
namespace PixelText
{
	// 큰 폰트 5x7 (메인 메뉴 타이틀/버튼).
	constexpr int kBigW = 5;
	constexpr int kBigH = 7;

	// 작은 폰트 4x6 (일시정지 메뉴 - 큰 폰트보다 대략 15~20% 작다).
	constexpr int kSmallW = 4;
	constexpr int kSmallH = 6;

	// 글자 사이 간격(픽셀 열).
	constexpr int kGap = 1;

	// text 를 픽셀맵으로. '#' 위치 -> ink, 그 외 -> '.'. 소문자는 대문자로 취급.
	// 폰트에 없는 글자는 공백. small=true 면 4x6 폰트.
	std::string Make(const std::string& text, char ink, bool small = false);

	// Make 결과의 픽셀 폭(배율 1).
	int Width(size_t charCount, bool small = false);

	// Make 결과의 픽셀 높이(배율 1) = kBigH / kSmallH.
	int Height(bool small = false);

	// w x h 를 전부 ink 로 채운 픽셀맵(패널/하이라이트 배경용).
	std::string SolidBlock(int w, int h, char ink);
}
