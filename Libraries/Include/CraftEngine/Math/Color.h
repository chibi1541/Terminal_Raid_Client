#pragma once

NAME_SPACE_BEGIN(Craft)

// 콘솔 색상 속성은 4비트라 동시에 쓸 수 있는 색이 16개로 고정된다.
// 각 값은 팔레트(ColorTable[16])의 슬롯 인덱스이고,
// 실제 RGB는 Palette가 정한다(Config/Palette.xml에서 조정 가능).
//
// 같은 계열은 인덱스를 연속으로 배치해서 램프(명암 단계)를 만든다.
// 덕분에 한 단계 밝게 하려면 인덱스를 +1 하면 된다.
enum class CRAFT_API Color : WORD
{
	// 무채색 램프
	Black =			0,
	DarkGray =		1,
	Gray =			2,
	White =			3,

	// 녹색 램프 (풀, 나뭇잎)
	DarkGreen =		4,
	Green =			5,

	// 갈색 램프 (흙, 나무, 가죽, 피부)
	DarkBrown =		6,
	Brown =			7,
	Tan =			8,

	// 적색 램프 (적, 피, HP)
	DarkRed =		9,
	Red =			10,

	// 난색 (불, 금화, 발사체)
	Orange =		11,
	Yellow =		12,

	// 청색 램프 (물, 얼음, 마법)
	DarkBlue =		13,
	Blue =			14,

	// 마법, 레어 등급
	Purple =		15,
};

// 전경색 비트 패턴을 배경색 비트로 이동 (Win32 콘솔: BACKGROUND_* == FOREGROUND_* << 4)
constexpr WORD ToBackgroundAttribute(Color color)
{
	return static_cast<WORD>(color) << 4;
}

NAME_SPACE_END