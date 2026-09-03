#pragma once

#include "Utils/EngineMacro.h"
#include "Actor/Facing.h"
#include "Asset/Sprite.h"
#include "Math/Vector2.h"
#include <string>

NAME_SPACE_BEGIN(Craft)

// 정적 프롭 한 종류의 그림 데이터. (묘비 하나, 울타리 한 칸, 문기둥 하나)
//
// 방향 슬롯 4개를 들고 있고, 화면에 무엇을 그릴지는
// RotateFacing(액터의 facing, 카메라 회전)으로 고른다.
//
// ★ 폴백은 로드 시점에 끝난다 ★
// 원본 아트에 정면/측면 2장뿐이어도 로더가 네 슬롯을 전부 채워놓는다.
// 그래서 런타임에는 "이 슬롯이 비었나" 분기가 아예 없다.
// 나중에 앞뒤가 다른 오브젝트(문, 조각상)가 생기면 데이터에 슬롯만 추가하면 되고
// 이 클래스도 그리는 쪽도 바뀌지 않는다.
class CRAFT_API PropSprite
{
public:
	PropSprite() = default;

	inline const std::string& GetName() const { return name; }

	// 표시 슬롯의 그림. 폴백이 끝나 있으므로 어떤 방향을 물어도 비어있지 않다
	// (프롭 자체가 스프라이트를 하나도 못 읽은 경우는 로더가 애초에 등록하지 않는다).
	inline const Sprite& GetSprite(EFacing facing) const
	{
		return sprites[ToFacingIndex(facing)];
	}

	// 기준점에 놓일 스프라이트 안의 점(셀 단위, 실수).
	//
	// 실수인 이유는 AnimationClip::GetPivotX와 같다 -
	// 짝수 너비의 가운데는 정수로 안 떨어진다(12칸이면 5.5).
	inline float GetPivotX(EFacing facing) const { return pivotX[ToFacingIndex(facing)]; }
	inline float GetPivotY(EFacing facing) const { return pivotY[ToFacingIndex(facing)]; }

	// 타일 영역(정사각)의 한 변 길이. 타일 개수이지 셀 개수가 아니다.
	inline int GetTileSpan() const { return tileSpan; }

	inline bool IsEmpty() const { return sprites[0].IsEmpty(); }

	// 슬롯별 피벗 기본값. 정면과 측면이 다르다.
	//
	//   정면(Up/Down)    : 가운데 맨 아래(발밑). 이동형 액터와 같은 규칙이다.
	//   측면(Left/Right) : 이미지 한가운데.
	//
	// 갈라야 하는 이유 - 타일 영역은 카메라가 돌아도 회전하지 않는다.
	// 그림은 타일보다 위로 솟아 있는데(12x12 타일에 12x16 그림) 측면에서도
	// 발밑을 기준으로 잡으면 보이는 그림이 타일 영역 밖으로 내려가버린다.
	static float GetDefaultPivotX(int width) { return (width - 1) * 0.5f; }
	static float GetDefaultPivotY(int height, EFacing facing)
	{
		if (IsSideFacing(facing))
		{
			return (height - 1) * 0.5f;
		}

		return static_cast<float>(height - 1);
	}

private:
	// 로더만 채운다. 밖에서는 읽기 전용이다.
	friend class PropSpriteLoader;

	std::string name;

	// EFacing 인덱스. 폴백 해소가 끝난 상태다.
	Sprite sprites[FacingCount];

	float pivotX[FacingCount] = { 0.0f, 0.0f, 0.0f, 0.0f };
	float pivotY[FacingCount] = { 0.0f, 0.0f, 0.0f, 0.0f };

	int tileSpan = 1;
};

NAME_SPACE_END
