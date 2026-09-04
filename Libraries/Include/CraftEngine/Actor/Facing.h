#pragma once

#include "Utils/EngineMacro.h"
#include "Math/Vector2.h"
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

// ---------------------------------------------------------------------------
// 이동형 액터용 - 데이터 슬롯 표기와 방향 판정
// ---------------------------------------------------------------------------

// 데이터(*.anim.xml의 @facing)가 선언하는 슬롯. EFacing과 다른 타입인 이유가 둘 있다.
//
//   Side : 측면 아트 한 장으로 좌/우를 모두 덮는다는 선언. 로더가 Right에 넣고
//          반대편은 미러로 채운다. 작가가 좌우 두 장을 따로 그리면 Left/Right를 직접 쓴다.
//   None : @facing을 아예 안 적은 클립. "방향이 없다"는 뜻이라 네 슬롯 전부가 된다.
//          기존 anim.xml이 전부 여기 해당해서, 이 값이 있어야 예전 파일이 그대로 동작한다.
//
// EFacing에 이 둘을 섞으면 Rotate90과의 값 정렬(Up=0..Left=3)이 깨진다.
enum class EFacingSlotSpec : int
{
	None = 0,
	Up,
	Right,
	Down,
	Left,
	Side,
};

// "Up"/"Right"/"Down"/"Left"/"Side"를 슬롯 선언으로. 빈 문자열이면 None(=방향 없음).
// 알아보지 못하는 문자열은 None을 돌려주면서 outParsed를 false로 둔다
// (오타를 크래시로 잡을지 흘려보낼지는 호출부가 정한다 - ParseFacing과 같은 방침).
inline EFacingSlotSpec ParseFacingSpec(const std::string& text, bool* outParsed = nullptr)
{
	EFacingSlotSpec spec = EFacingSlotSpec::None;
	bool parsed = true;

	if (text.empty())      { spec = EFacingSlotSpec::None; }
	else if (text == "Up")    { spec = EFacingSlotSpec::Up; }
	else if (text == "Right") { spec = EFacingSlotSpec::Right; }
	else if (text == "Down")  { spec = EFacingSlotSpec::Down; }
	else if (text == "Left")  { spec = EFacingSlotSpec::Left; }
	else if (text == "Side")  { spec = EFacingSlotSpec::Side; }
	else                      { parsed = false; }

	if (nullptr != outParsed)
	{
		*outParsed = parsed;
	}

	return spec;
}

// 각도를 [0, 360)으로 접는다.
inline float NormalizeDegrees(float degrees)
{
	while (degrees < 0.0f)     { degrees += 360.0f; }
	while (degrees >= 360.0f)  { degrees -= 360.0f; }

	return degrees;
}

namespace FacingDetail
{
	// 화면 각도 -> 화면 슬롯. [begin, end) 반개구간이다.
	struct FacingSector
	{
		float beginDegrees;
		float endDegrees;
		EFacing facing;
	};

	// ★ 좌우가 반대로 보이면 고칠 곳은 여기 하나뿐이다 ★
	//
	// 각도는 반시계, +x가 0°, 화면 위쪽이 +90°다(= atan2(-dy, dx), 콘솔 y가 아래로 커지니 부호를 뒤집는다).
	// 섹터가 비대칭인 것은 의도다 - 앞/뒤는 60°, 측면은 120°.
	// 측면 아트가 실루엣이 가장 잘 읽히고 좌우 반전으로 두 슬롯을 덮으므로 넓게 잡는다.
	//
	// Right는 0°를 걸치고 있어서 두 칸으로 나뉜다. 한 칸으로 접으려면 감싸는 구간 처리를
	// 표에 넣어야 하는데, 그러면 표를 읽어서 경계를 확인하기가 어려워진다.
	constexpr FacingSector ScreenSectors[] =
	{
		{   0.0f,  60.0f, EFacing::Right },
		{  60.0f, 120.0f, EFacing::Up    },   // 뒷모습
		{ 120.0f, 240.0f, EFacing::Left  },
		{ 240.0f, 300.0f, EFacing::Down  },   // 앞모습
		{ 300.0f, 360.0f, EFacing::Right },
	};

	constexpr int ScreenSectorCount = static_cast<int>(sizeof(ScreenSectors) / sizeof(ScreenSectors[0]));

	// degrees가 [begin, end) 안에 있는지. begin/end는 여유각 때문에 [0,360) 밖으로
	// 나갈 수 있으므로 정규화한 뒤 0°를 감싸는 구간도 처리한다.
	inline bool IsDegreesWithin(float degrees, float beginDegrees, float endDegrees)
	{
		const float begin = NormalizeDegrees(beginDegrees);
		const float end = NormalizeDegrees(endDegrees);

		if (begin <= end)
		{
			return (degrees >= begin) && (degrees < end);
		}

		// 0°를 감싸는 구간(예 292° ~ 8°).
		return (degrees >= begin) || (degrees < end);
	}
}

// 화면 각도(도) -> 화면에 그릴 방향 슬롯.
//
// 여기서 나오는 값은 월드 방향이 아니라 "화면" 슬롯이다.
// 월드 값이 필요하면 RotateFacing(결과, -카메라회전)으로 되돌린다.
inline EFacing FacingFromScreenAngle(float degrees)
{
	const float normalized = NormalizeDegrees(degrees);

	for (int index = 0; index < FacingDetail::ScreenSectorCount; ++index)
	{
		const FacingDetail::FacingSector& sector = FacingDetail::ScreenSectors[index];

		if ((normalized >= sector.beginDegrees) && (normalized < sector.endDegrees))
		{
			return sector.facing;
		}
	}

	// 표가 [0,360)을 빈틈없이 덮으므로 여기까지 오지 않는다. 방어적 기본값.
	return EFacing::Down;
}

// 위와 같지만 previous 섹터를 marginDegrees만큼 넓혀서 유지한다.
//
// 이게 없으면 커서가 섹터 경계에 걸쳐 있을 때 1셀 흔들림에도 스프라이트가 떤다.
// 상태가 previous 하나뿐이라 호출부는 자기 facing만 넘기면 된다
// (저역통과 필터는 지연을, 타이머 락은 반응 상한을 만든다 - 여기서는 둘 다 필요 없다).
inline EFacing FacingFromScreenAngleSticky(float degrees, EFacing previous, float marginDegrees)
{
	const float normalized = NormalizeDegrees(degrees);
	const EFacing candidate = FacingFromScreenAngle(normalized);

	if (candidate == previous)
	{
		return previous;
	}

	// 아직 이전 섹터의 여유각 안이면 갈아타지 않는다.
	// Right처럼 표에 두 칸으로 나뉜 방향도 각 칸을 따로 넓히면 결과가 같다.
	for (int index = 0; index < FacingDetail::ScreenSectorCount; ++index)
	{
		const FacingDetail::FacingSector& sector = FacingDetail::ScreenSectors[index];

		if (sector.facing != previous)
		{
			continue;
		}

		if (FacingDetail::IsDegreesWithin(
			normalized, sector.beginDegrees - marginDegrees, sector.endDegrees + marginDegrees))
		{
			return previous;
		}
	}

	return candidate;
}

// 이동/공격 델타 -> 방향. 우세한 축이 이긴다.
//
// 원격 플레이어와 몬스터가 쓴다 - 마우스가 없으니 각도를 잴 기준점이 없고,
// 있는 정보는 "어디로 움직였나 / 어디를 때렸나"뿐이다.
// 대각선으로 정확히 동률이면 보던 방향을 유지한다 - 규칙으로 한쪽을 고르면
// 대각 이동 중에 두 방향 사이를 매 틱 오간다.
inline EFacing FacingFromDelta(const Vector2& delta, EFacing previous)
{
	const int absX = (delta.x < 0) ? -delta.x : delta.x;
	const int absY = (delta.y < 0) ? -delta.y : delta.y;

	if (absX == absY)
	{
		// 정지(0,0)와 정확한 대각선이 여기로 온다. 둘 다 판단 근거가 없다.
		return previous;
	}

	if (absX > absY)
	{
		return (delta.x > 0) ? EFacing::Right : EFacing::Left;
	}

	// 콘솔은 y가 아래로 커진다.
	return (delta.y > 0) ? EFacing::Down : EFacing::Up;
}

NAME_SPACE_END
