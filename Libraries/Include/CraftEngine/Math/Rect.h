#pragma once

#include "Math/Vector2.h"

NAME_SPACE_BEGIN(Craft)

// 콘솔 셀 단위의 사각 영역.
//
// UI 레이아웃(위젯의 배치 영역)과 렌더러의 클리핑 영역이 같은 타입을 써야 해서
// UI가 아니라 Math에 둔다. Renderer가 UI 헤더를 include하면 의존 방향이 뒤집힌다.
//
// 셀 격자 위의 값이므로 전부 정수다. 실수를 들고 다니다 마지막에 반올림하면
// 위젯이 한 칸씩 떠는 현상이 생기므로 처음부터 정수로만 계산한다.
//
// position은 좌상단 셀, size는 셀 개수다.
// 따라서 오른쪽 끝 셀은 position.x + size.x - 1 이다(GetRight가 그 값을 준다).
struct CRAFT_API Rect
{
	Rect() = default;
	Rect(const Vector2& position, const Vector2& size)
		: position(position), size(size)
	{
	}

	Rect(int x, int y, int width, int height)
		: position(x, y), size(width, height)
	{
	}

	// 경계 셀 좌표. Right/Bottom은 "마지막 셀"이지 "그 다음 칸"이 아니다.
	inline int GetLeft() const { return position.x; }
	inline int GetTop() const { return position.y; }
	inline int GetRight() const { return position.x + size.x - 1; }
	inline int GetBottom() const { return position.y + size.y - 1; }

	// 너비나 높이가 0 이하면 그릴 것도 배치할 것도 없다.
	inline bool IsEmpty() const { return size.x <= 0 || size.y <= 0; }

	// 점이 이 영역 안에 있는지. 마우스 히트 테스트에 쓴다.
	bool Contains(const Vector2& point) const
	{
		if (IsEmpty())
		{
			return false;
		}

		return point.x >= GetLeft() && point.x <= GetRight()
			&& point.y >= GetTop() && point.y <= GetBottom();
	}

	// 두 영역의 교집합.
	//
	// 겹치는 부분이 없으면 size가 0 이하인 사각형이 나온다(IsEmpty()가 참).
	// 부모가 허용한 영역과 자식이 그리려는 영역을 겹쳐 클리핑 범위를 구하는 데 쓴다.
	Rect Intersect(const Rect& other) const
	{
		if (IsEmpty() || other.IsEmpty())
		{
			return Rect();
		}

		// std::max/min을 쓰면 Windows.h의 min/max 매크로와 충돌할 수 있어서
		// (NOMINMAX가 정의돼 있어도 다른 헤더 순서에 의존하게 된다) 직접 비교한다.
		const int left = (position.x > other.position.x) ? position.x : other.position.x;
		const int top = (position.y > other.position.y) ? position.y : other.position.y;

		const int thisRight = GetRight();
		const int otherRight = other.GetRight();
		const int right = (thisRight < otherRight) ? thisRight : otherRight;

		const int thisBottom = GetBottom();
		const int otherBottom = other.GetBottom();
		const int bottom = (thisBottom < otherBottom) ? thisBottom : otherBottom;

		// 겹치지 않으면 right < left가 되어 size가 0 이하로 나온다.
		return Rect(left, top, right - left + 1, bottom - top + 1);
	}

	bool operator==(const Rect& other) const
	{
		return position == other.position && size == other.size;
	}

	bool operator!=(const Rect& other) const
	{
		return !(*this == other);
	}

public:
	// 좌상단 셀 좌표.
	Vector2 position;

	// 셀 개수(너비, 높이).
	Vector2 size;
};

NAME_SPACE_END
