#pragma once

#include "Utils/EngineMacro.h"
#include "Actor/Facing.h"
#include "Asset/Sprite.h"
#include "Level/TileMetrics.h"
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

	// 타일 영역의 "벽 방향" 길이(타일 개수). 셀 개수가 아니다.
	//
	// 정사각인 것은 타일 한 칸의 정의(8x8 / 12x12 / 16x16)이지 이 영역이 아니다.
	// 오브젝트는 크기에 따라 타일을 여러 개 먹는다. 깊이는 언제나 1타일이고,
	// 스팬이 걸리는 축은 facing이 정한다(StaticPropActor::GetTileBounds 참고).
	inline int GetTileSpan() const { return tileSpan; }

	// 이 아트가 그려진 타일 격자 크기(셀 개수). <PropSprites tileSize="..">에서 온다.
	//
	// 기본 피벗이 이 값에 걸려 있어서 애셋이 알고 있어야 한다.
	// 프롭 묶음은 파일 단위로 캐시되는 공유 애셋이라, 타일 크기가 다른 레벨이
	// 같은 묶음을 쓰면 피벗이 통째로 어긋난다 - 스폰할 때 레벨 값과 대조한다.
	inline int GetTileSize() const { return tileSize; }

	inline bool IsEmpty() const { return sprites[0].IsEmpty(); }

	// 기준점에서 이미지 바닥까지의 거리(행 수).
	//
	// 기본 피벗 / 타일 영역 / 타일 좌표 -> 기준점 변환, 세 곳이 이 값을 공유한다.
	// 각자 적어두면 한 곳만 고쳐졌을 때 그림과 충돌 영역이 조용히 갈라진다.
	//
	// 타일 한 변의 절반이다. 발밑 타일의 가운데 높이쯤에 기준점이 놓인다.
	// 정수로 두는 이유는 기준점이 셀 격자 위에 있어야 하기 때문이다.
	static int GetFloorOffset(int tileSize) { return tileSize / 2; }

	// 피벗 기본값. ★ 슬롯에 따라 갈리지 않는다 ★
	//
	//   x : 가운데 타일 열의 중앙  (W-1)/2
	//   y : 이미지 하단에서 GetFloorOffset만큼 위
	//
	// 예전에는 슬롯마다 앵커를 90도씩 돌렸는데, 그게 빌보드를 깨뜨렸다.
	// 탑다운 뷰에서 오브젝트는 카메라를 어떻게 돌리든 화면 위로 서 있어야 하는데,
	// 이미지가 붙는 변이 회전마다 바뀌니 그림이 통째로 옆으로 밀렸다.
	// 기준점은 고정된 월드 점 하나이고 이미지는 언제나 거기 같은 자리에 붙는다.
	//
	// 슬롯별로 다른 그림이 필요하면 규칙으로 유도하지 말고 @pivot으로 직접 적는다.
	static float GetDefaultPivotX(int width)
	{
		return (width - 1) * 0.5f;
	}

	static float GetDefaultPivotY(int height, int tileSize)
	{
		return static_cast<float>(height - 1 - GetFloorOffset(tileSize));
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

	// 이 아트가 그려진 타일 격자 크기.
	int tileSize = DefaultTileSize;
};

NAME_SPACE_END
