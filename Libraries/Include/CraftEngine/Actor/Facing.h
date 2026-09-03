#pragma once

#include "Utils/EngineMacro.h"
#include <string>

NAME_SPACE_BEGIN(Craft)

// 액터가 "어디를 보는가". 각도가 아니라 4방향 enum이다.
//
// 콘솔은 셀 격자라 정지 각도가 언제나 90° 단위이고, 액터는 회전하지 않는다.
// 각도(float)를 들고 다니면 비교/스위치마다 반올림 경계를 신경 써야 하는데
// 실제로 필요한 정보는 "넷 중 어느 쪽"뿐이다.
//
// ★ 값 순서가 Rotate90(Math/ViewTransform.h)과 같아야 한다 ★
// Rotate90은 화면 기준 시계 방향이고 이 enum도 Up->Right->Down->Left 시계 방향이다.
// 그래서 카메라를 k회 돌렸을 때의 화면상 방향이 단순 덧셈 하나로 나온다.
// 순서를 바꾸면 RotateFacing이 조용히 틀린 그림을 고른다.
enum class EFacing : int
{
	Up = 0,
	Right = 1,
	Down = 2,
	Left = 3,
};

// 방향 슬롯 개수. 배열 크기로 쓴다.
constexpr int FacingCount = 4;

inline int ToFacingIndex(EFacing facing)
{
	return static_cast<int>(facing);
}

// 카메라가 k회 90° 돌면 월드 방향은 화면에서 그만큼 같이 돈다.
//
// 정적 액터가 화면에 보여줄 슬롯이 바로 이 값이다.
// facing(월드 값)은 그대로 두고 표시 슬롯만 회전시킨다 - 액터가 실제로 도는 게 아니다.
inline EFacing RotateFacing(EFacing facing, int quarterTurns)
{
	const int k = ((quarterTurns % 4) + 4) % 4;

	return static_cast<EFacing>((static_cast<int>(facing) + k) % 4);
}

// 측면(좌/우)으로 보이는 방향인지.
//
// 피벗 기본값이 여기서 갈린다 - 정면은 하단 중앙, 측면은 이미지 중앙이다.
// 타일 영역이 회전하지 않기 때문에 이렇게 갈라야 충돌 영역과 그림이 안 어긋난다.
inline bool IsSideFacing(EFacing facing)
{
	return facing == EFacing::Left || facing == EFacing::Right;
}

// "Up"/"Down"/"Left"/"Right"를 enum으로. 못 알아보면 fallback을 돌려준다.
// (데이터 오타를 크래시로 잡을지 폴백할지는 호출부가 정한다)
inline EFacing ParseFacing(const std::string& text, EFacing fallback, bool* outParsed = nullptr)
{
	const bool parsed =
		(text == "Up" || text == "Down" || text == "Left" || text == "Right");

	if (outParsed != nullptr)
	{
		*outParsed = parsed;
	}

	if (text == "Up")
	{
		return EFacing::Up;
	}

	if (text == "Right")
	{
		return EFacing::Right;
	}

	if (text == "Down")
	{
		return EFacing::Down;
	}

	if (text == "Left")
	{
		return EFacing::Left;
	}

	return fallback;
}

NAME_SPACE_END
