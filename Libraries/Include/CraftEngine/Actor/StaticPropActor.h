#pragma once

#include "Utils/EngineMacro.h"
#include "Actor/Actor.h"
#include "Actor/Facing.h"
#include "Asset/PropSpriteSet.h"
#include "Math/Rect.h"
#include <memory>
#include <string>

NAME_SPACE_BEGIN(Craft)

// 필드에 고정되어 움직이지 않는 오브젝트. (묘비, 울타리, 문기둥, 철문)
//
// 이동형 액터와 갈라지는 지점:
//  - 표시 이미지가 카메라 회전 "하나만"으로 정해진다.
//    이동형은 여기에 이동 방향과 공격 방향이 더해지고, 그건 animate 단계 일이다.
//  - 서버와 동기화하지 않는다. 리플리케이트 액터가 아니다.
//    지형처럼 양쪽이 각자 같은 데이터를 읽으면 되는 것이라 보낼 게 없다.
//
// ★ 그릴 이미지는 매 프레임 고르지 않는다 ★
// OnViewRotationChanged가 카메라 회전이 "완전히 끝난" 순간에만 불리고,
// 거기서 displaySlot 하나만 갱신한다. Draw는 카메라를 아예 보지 않는다.
// 보간 중에 그림이 바뀌면 회전하는 도중에 오브젝트가 튀는 것처럼 보인다.
//
// ★ 애셋을 직접 로드하지 않는다 ★
// 이미 로드된 PropSpriteSet을 받아서 생성된다. 배치가 100개를 넘으면
// 액터마다 LoadAsync를 걸 이유가 없다(같은 파일에 콜백만 100개 쌓인다).
// 로딩은 레벨이 한 번 하고(TileMapLevel), 결과를 나눠준다.
class CRAFT_API StaticPropActor : public Actor
{
	TYPE_DECLARATIONS(StaticPropActor, Actor)

public:
	// propSet  : 이미 로드된 스프라이트 묶음. 캐시가 "사용 중"으로 보도록 계속 들고 있는다.
	// propName : 그 묶음 안의 <Prop name="..."> (예 "TOMB_A")
	// position : 기준점(월드). 타일 좌표가 아니다 - 변환은 스폰하는 쪽이 한다.
	// facing   : 이 프롭이 월드에서 보고 있는 방향. 카메라와 무관한 고정값이다.
	StaticPropActor(
		std::shared_ptr<const PropSpriteSet> propSet,
		std::string propName,
		const Vector2& position,
		EFacing facing = EFacing::Up);

	virtual ~StaticPropActor() = default;

	virtual void Draw() override;

	// 카메라 회전이 정착한 순간에만 불린다. Tick이 아니다.
	virtual void OnViewRotationChanged(int quarterTurns) override;

	// 타일 영역(월드 공간). 길찾기/이동 차단이 붙을 자리다.
	//
	// 정사각이 아니다 - 정사각인 것은 타일 한 칸이고, 오브젝트는 크기만큼
	// 타일을 여러 개 먹는다(깊이는 언제나 1타일). 스팬이 걸리는 축과
	// 기준점의 위치는 카메라가 아니라 facing이 정하므로, 화면을 돌려도 이 값은 안 변한다.
	Rect GetTileBounds() const;

	inline EFacing GetFacing() const { return facing; }

	// 지금 화면에 그리고 있는 방향 슬롯. facing과 달리 카메라 회전에 따라 바뀐다.
	// 회전이 제대로 반영됐는지 밖에서 확인할 수 있어야 해서 공개한다.
	inline EFacing GetDisplaySlot() const { return displaySlot; }
	inline const std::string& GetPropName() const { return propName; }

	// 묶음에서 이름을 찾지 못하면(데이터 오타) 그릴 것이 없다.
	inline bool IsSpriteReady() const { return propSprite != nullptr; }

private:
	std::string propName;

	// 월드 방향. 절대 변하지 않는다(정적이니까).
	EFacing facing = EFacing::Up;

	// 캐시 참조 유지용.
	//
	// AssetManager는 "매니저 말고 아무도 안 들고 있는" 애셋을 유휴 언로드하므로
	// 이 shared_ptr을 놓으면 30초 뒤에 스프라이트가 사라진다.
	std::shared_ptr<const PropSpriteSet> propSet;

	// propSet 안의 항목을 가리킨다. propSet이 살아있는 동안만 유효하다.
	// (unordered_map은 재해싱해도 원소 주소가 안 변한다 - 여기서는 삽입도 없다)
	const PropSprite* propSprite = nullptr;

	// 지금 화면에 그릴 슬롯 = RotateFacing(facing, 카메라 회전).
	// OnViewRotationChanged에서만 바뀐다.
	EFacing displaySlot = EFacing::Up;
};

NAME_SPACE_END
