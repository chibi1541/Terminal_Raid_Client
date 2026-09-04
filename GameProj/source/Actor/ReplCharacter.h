#pragma once

#include "ReplicatedActor.h"
#include "Actor/Facing.h"

#include <memory>
#include <string>

// 전방 선언
NAME_SPACE_BEGIN(Craft)

class SpriteAnimatorComponent;

NAME_SPACE_END


// 애니메이션을 가지고 움직이는 ReplicatedActor가 공유
//
// "보이는 것"만 여기 있다 - 스프라이트 애니메이터와 머리 위 이름표.
class ReplCharacter : public ReplicatedActor
{
	TYPE_DECLARATIONS(ReplCharacter, ReplicatedActor)

public:
	ReplCharacter() = default;

	virtual void BeginPlay() override;

	// 방향 슬롯을 갱신하고 컴포넌트를 돌린다.
	virtual void Tick(float deltaTime) override;

	virtual void Draw() override;

	// PlayerInfo.name을 여기서 받는다.
	virtual void ApplyObjectInfo(const Protocol::ObjectInfo& info) override;

	inline const std::string& GetCharacterName() const { return characterName; }

	inline int32 GetHp() const { return hp; }
	inline int32 GetMaxHp() const { return maxHp; }
	inline bool IsAlive() const { return hp > 0; }

protected:
	// 이름표 색. 내 캐릭터와 남을 화면에서 구분하는 유일한 수단이다.
	// (서버가 chartype을 안 보내서 스프라이트는 둘 다 같다)
	virtual Craft::Color GetNameColor() const { return Craft::Color::White; }

	// "지금 이 캐릭터는 월드에서 어디를 보고 있는가".
	//
	// 이동형 액터가 정적 프롭과 갈라지는 지점이 여기다. 프롭은 방향이 고정값이지만
	// 이쪽은 조작 주체에 따라 근거가 다르다.
	//   LocalPlayer  - 마우스 각도(화면 기준)
	//   otherReplicatedChara - 공격 중이면 공격 방향, 아니면 이동 방향
	// 기본 구현은 "보던 방향 유지"다. 근거가 없는 액터는 아무것도 안 하면 된다.
	virtual Craft::EFacing ComputeWorldFacing() const { return facing; }

	// facing -> displaySlot -> 애니메이터. Tick이 super보다 먼저 부른다.
	void UpdateFacing();

protected:
	// 월드에서 보고 있는 방향. 카메라와 무관하다.
	// 나중에 서버와 주고받게 될 값도 이쪽이다(화면 슬롯은 클라마다 다르다).
	Craft::EFacing facing = Craft::EFacing::Down;

	// 지금 화면에 그리는 슬롯 = RotateFacing(facing, 카메라 회전).
	//
	// StaticPropActor는 같은 값을 회전이 끝난 순간에만 갱신하지만(그림이 튀지 않게),
	// 이동형은 어차피 매 틱 방향이 바뀌므로 여기서는 매 틱 다시 계산한다.
	Craft::EFacing displaySlot = Craft::EFacing::Down;

protected:
	std::string animName = {};

	int32 hp = 0;
	int32 maxHp = 0;

	// 스프라이트 애니메이션 재생 담당.
	// 생성자가 아니라 BeginPlay에서 만든다(weak_from_this가 그때부터 유효).
	std::shared_ptr<Craft::SpriteAnimatorComponent> animator;

	std::string characterName;
};
