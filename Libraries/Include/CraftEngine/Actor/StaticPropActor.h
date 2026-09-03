#pragma once

#include "Utils/EngineMacro.h"
#include "Actor/Actor.h"
#include "Actor/Facing.h"
#include "Asset/PropSpriteSet.h"
#include "Math/Rect.h"
#include <memory>
#include <string>

NAME_SPACE_BEGIN(Craft)

// 필드에 고정되어 움직이지 않는 오브젝트. (묘비, 울타리, 문기둥)
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
// 기준점(GetPosition)은 타일 영역의 하단 중앙이다.
// 타일 영역은 정사각이고 카메라가 돌아도 회전하지 않는다 - GetTileBounds 참고.
class CRAFT_API StaticPropActor : public Actor
{
	TYPE_DECLARATIONS(StaticPropActor, Actor)

public:
	// propSetName : Assets/PropData.xml의 name= (예 "Graveyard")
	// propName    : 그 파일 안의 <Prop name="..."> (예 "TOMB_A")
	// facing      : 이 프롭이 월드에서 보고 있는 방향. 카메라와 무관한 고정값이다.
	StaticPropActor(
		std::string propSetName,
		std::string propName,
		const Vector2& position,
		EFacing facing = EFacing::Up);

	virtual ~StaticPropActor() = default;

	virtual void BeginPlay() override;
	virtual void Draw() override;

	// 카메라 회전이 정착한 순간에만 불린다. Tick이 아니다.
	virtual void OnViewRotationChanged(int quarterTurns) override;

	// 타일 영역(월드 공간, 정사각). 길찾기/이동 차단이 붙을 자리다.
	//
	// 기준점이 하단 중앙이므로 영역은 기준점에서 위로 뻗는다.
	// 카메라 회전을 참조하지 않는다 - 회전해도 이 값은 절대 변하지 않는다.
	// (그래서 이동형 액터의 판정이 뷰에 따라 달라지지 않는다)
	Rect GetTileBounds() const;

	inline EFacing GetFacing() const { return facing; }

	// 지금 화면에 그리고 있는 방향 슬롯. facing과 달리 카메라 회전에 따라 바뀐다.
	// 회전이 제대로 반영됐는지 밖에서 확인할 수 있어야 해서 공개한다.
	inline EFacing GetDisplaySlot() const { return displaySlot; }
	inline const std::string& GetPropName() const { return propName; }

	// 애셋이 도착해서 그릴 수 있는 상태인지.
	inline bool IsSpriteReady() const { return propSprite != nullptr; }

private:
	// 비동기 로드가 끝나면 묶음에서 자기 프롭을 찾아 붙든다.
	void OnPropSetLoaded(std::shared_ptr<const PropSpriteSet> loaded);

private:
	std::string propSetName;
	std::string propName;

	// 월드 방향. 절대 변하지 않는다(정적이니까).
	EFacing facing = EFacing::Up;

	// 캐시 참조 유지용.
	//
	// AssetManager는 "매니저 말고 아무도 안 들고 있는" 애셋을 유휴 언로드하므로
	// 이 shared_ptr을 놓으면 30초 뒤에 스프라이트가 사라진다.
	std::shared_ptr<const PropSpriteSet> loadedSet;

	// loadedSet 안의 항목을 가리킨다. loadedSet이 살아있는 동안만 유효하다.
	// (unordered_map은 재해싱해도 원소 주소가 안 변한다 - 여기서는 삽입도 없다)
	const PropSprite* propSprite = nullptr;

	// 지금 화면에 그릴 슬롯 = RotateFacing(facing, 카메라 회전).
	// OnViewRotationChanged에서만 바뀐다.
	EFacing displaySlot = EFacing::Up;
};

NAME_SPACE_END
