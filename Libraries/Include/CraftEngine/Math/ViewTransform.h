#pragma once

#include "Math/Vector2.h"
#include "Math/Rect.h"

#include <cmath>

NAME_SPACE_BEGIN(Craft)

// 카메라 뷰의 월드<->화면 변환. 회전이 없으면 순수 평행이동이고,
// 90° 단위 정지 상태는 정수만으로, 보간 전환 중에는 (cos,sin)으로 계산한다.
//
// Renderer와 CameraManager가 같은 변환을 두 벌 들면 드리프트하므로 여기 한 곳에 둔다.
// Math에 두면 Renderer가 Camera 헤더 의존 없이 include할 수 있다(Rect와 같은 이유).
//
// 좌표계: 콘솔은 y가 아래로 증가한다. quarterTurns는 화면 기준 시계방향 90° 횟수.
//   k=1: (dx,dy) -> (-dy, dx)   k=2: (-dx,-dy)   k=3: (dy,-dx)
// viewHalfSize는 screenSize/2 (정수). center/half가 같고 k=0이면
// ViewWorldToScreen(world) == world 가 되어 회전 이전 동작과 완전히 동일하다.

namespace ViewTransformDetail
{
	// Windows.h의 min/max 매크로 충돌을 피하려고 직접 비교한다(Rect.h와 같은 방침).
	inline int MinInt(int a, int b) { return a < b ? a : b; }
	inline int MaxInt(int a, int b) { return a > b ? a : b; }
	inline int RoundToCell(float v) { return static_cast<int>(::floorf(v + 0.5f)); }
}

// k회 90° 회전(정수). quarterTurns는 음수/4이상도 허용(내부에서 정규화).
inline Vector2 Rotate90(const Vector2& v, int quarterTurns)
{
	const int k = ((quarterTurns % 4) + 4) % 4;

	switch (k)
	{
	case 1:  return Vector2(-v.y, v.x);
	case 2:  return Vector2(-v.x, -v.y);
	case 3:  return Vector2(v.y, -v.x);
	default: return v;
	}
}

// 임의 각도 회전. c=cos, s=sin은 호출부가 프레임당 1회 precompute한다.
// 반올림은 floorf(v + 0.5f) - (int)(v+0.5f)는 음수에서 0쪽으로 절단돼 1칸 어긋난다.
inline Vector2 RotateCosSin(const Vector2& v, float c, float s)
{
	const float fx = static_cast<float>(v.x);
	const float fy = static_cast<float>(v.y);

	return Vector2(
		ViewTransformDetail::RoundToCell(fx * c - fy * s),
		ViewTransformDetail::RoundToCell(fx * s + fy * c));
}

// ---- 월드 -> 화면 ----

inline Vector2 ViewWorldToScreen(const Vector2& world, const Vector2& center, const Vector2& half, int quarterTurns)
{
	return Rotate90(world - center, quarterTurns) + half;
}

inline Vector2 ViewWorldToScreenF(const Vector2& world, const Vector2& center, const Vector2& half, float c, float s)
{
	return RotateCosSin(world - center, c, s) + half;
}

// ---- 화면 -> 월드 (역회전) ----

inline Vector2 ViewScreenToWorld(const Vector2& screen, const Vector2& center, const Vector2& half, int quarterTurns)
{
	// 역회전 = 4-k.
	return center + Rotate90(screen - half, 4 - (((quarterTurns % 4) + 4) % 4));
}

inline Vector2 ViewScreenToWorldF(const Vector2& screen, const Vector2& center, const Vector2& half, float c, float s)
{
	// 역회전 = (c, -s).
	return center + RotateCosSin(screen - half, c, -s);
}

// ---- 월드 Rect -> 화면 공간 AABB ----
//
// 회전된 Rect는 axis-aligned Rect로 표현 못 하므로 네 꼭짓점을 변환해 감싸는 AABB를 만든다.
// 회전이 없으면 결과는 입력을 평행이동한 것과 동일하다.

namespace ViewTransformDetail
{
	template <typename TransformFunc>
	inline Rect CornersToAABB(const Rect& worldRect, TransformFunc&& transform)
	{
		const Vector2 corners[4] = {
			Vector2(worldRect.GetLeft(),  worldRect.GetTop()),
			Vector2(worldRect.GetRight(), worldRect.GetTop()),
			Vector2(worldRect.GetLeft(),  worldRect.GetBottom()),
			Vector2(worldRect.GetRight(), worldRect.GetBottom()),
		};

		Vector2 first = transform(corners[0]);
		int minX = first.x, maxX = first.x;
		int minY = first.y, maxY = first.y;

		for (int i = 1; i < 4; ++i)
		{
			const Vector2 p = transform(corners[i]);
			minX = MinInt(minX, p.x);
			maxX = MaxInt(maxX, p.x);
			minY = MinInt(minY, p.y);
			maxY = MaxInt(maxY, p.y);
		}

		return Rect(minX, minY, (maxX - minX) + 1, (maxY - minY) + 1);
	}
}

inline Rect ViewWorldRectToScreenAABB(const Rect& worldRect, const Vector2& center, const Vector2& half, int quarterTurns)
{
	if (worldRect.IsEmpty())
	{
		return worldRect;
	}

	return ViewTransformDetail::CornersToAABB(worldRect,
		[&](const Vector2& p) { return ViewWorldToScreen(p, center, half, quarterTurns); });
}

inline Rect ViewWorldRectToScreenAABBF(const Rect& worldRect, const Vector2& center, const Vector2& half, float c, float s)
{
	if (worldRect.IsEmpty())
	{
		return worldRect;
	}

	return ViewTransformDetail::CornersToAABB(worldRect,
		[&](const Vector2& p) { return ViewWorldToScreenF(p, center, half, c, s); });
}

NAME_SPACE_END
