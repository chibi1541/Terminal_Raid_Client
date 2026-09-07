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

	// 서버가 권위인 hp를 그대로 반영한다. 데미지 계산은 클라에서 하지 않는다.
	virtual void ApplyHit(const Protocol::S_HIT& pkt) override;

	// hp가 0이 된 순간의 통지. S_HIT이 이미 0을 보냈어도 다시 한 번 확정한다 -
	// 패킷 순서가 뒤바뀌어 도착해도(HIT 유실 등) 사망 연출은 반드시 뜨게 하기 위함.
	virtual void ApplyDeath(const Protocol::S_DEATH& pkt) override;

	inline const std::string& GetCharacterName() const { return characterName; }

	// 재생할 애니메이션 키(AnimationData.xml 의 Clip/StateMachine name). ObjectManager::Spawn 이
	// 서버가 준 CharacterType/MonsterType 을 ActorDataAsset 으로 조회해 BeginPlay 전에 꽂는다.
	// 비어 있으면 각 서브클래스 BeginPlay 가 폴백 기본값을 쓴다.
	void SetAnimName(const std::string& name) { animName = name; }

	// 서버 충돌 박스 한 변(셀). ObjectManager::Spawn 이 Character/MonsterData 에서 꽂는다.
	// 액터 기준점이 몸통 중심이라 발밑 = 중심 + collisionCells/2, 머리 = 중심 - collisionCells/2.
	// 이름표 / 체력바 위치가 이 값으로 정해진다.
	void SetCollisionCells(int32 cells) { collisionCells = (cells > 0) ? cells : 1; }

	inline int32 GetHp() const { return hp; }
	inline int32 GetMaxHp() const { return maxHp; }
	inline bool IsAlive() const { return hp > 0; }

	// 피격 경직 중인가 (S_HIT.stunMs 로 켜지고, Hit 클립 끝 HitEnd 노티파이/타임아웃으로 꺼짐).
	inline bool IsHitReacting() const { return isHit; }
	// 사망 상태인가 (S_DEATH 로 켜짐, 해제 없음 - 게임오버/리스폰은 나중).
	inline bool IsDeadState() const { return isDead; }

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

	// 서버 8방향 -> 축당 -1/0/1 델타. FacingFromDelta·attackDirection이 공유한다.
	static Craft::Vector2 DeltaFromServerDirection(Protocol::DirectionType dir);

	// 서버가 보낸 8방향(lastDirection)을 4방향 화면 슬롯으로 접는다.
	//
	// RemotePlayer/Monster가 공유한다 - 둘 다 마우스가 없어서 서버가 알려준
	// 이동 방향이 유일한 근거다. 대각선은 FacingFromDelta의 동률 규칙을 그대로
	// 따른다(보던 방향 유지) - 대각 이동 중 매 틱 두 방향을 오가지 않기 위함이다.
	static Craft::EFacing FacingFromServerDirection(Protocol::DirectionType dir, Craft::EFacing previous);

protected:
	// 이름표 / 체력바는 몸통 중심에서 collisionCells/2(=발밑·머리) 만큼 나간 뒤 이 여유칸을 더 준다.
	// 빌보드(화면 공간 오프셋)라 뷰 회전에 영향받지 않는다.
	static constexpr int nameTagMarginY = 2;	// 머리 위쪽 여유
	static constexpr int hpBarMarginY = 1;		// 발밑 아래쪽 여유
	static constexpr int hpBarWidth = 10;

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

	// 서버 충돌 박스 한 변(셀). 기본값 = 플레이어(8). 스폰 시 데이터 테이블 값으로 덮인다.
	int32 collisionCells = 8;

	int32 hp = 0;
	int32 maxHp = 0;

	// 피격 / 사망 상태. ApplyHit/ApplyDeath 가 켜고, ReplCharacter::Tick 이 HitEnd 노티파이로 isHit 를 끈다.
	bool  isHit = false;
	bool  isDead = false;
	float hitFallbackSec = 0.0f;	// 노티파이 유실 대비 isHit 자동 해제 타이머.

	// 경직 없는 피격(보스 등 stunMs=0)에 쓰는 흰색 깜빡임. 상태 이상 없이 연출만.
	// ApplyHit 이 채우고, Tick 이 깎으면서 주기적으로 animator tint 를 White <-> 원색으로 토글한다.
	float hitFlashSec = 0.0f;
	static constexpr float hitFlashDuration = 0.24f;	// 총 지속
	static constexpr float hitFlashPeriod   = 0.12f;	// 한 깜빡임 주기(절반은 흰색, 절반은 원색)

	// 스프라이트 애니메이션 재생 담당.
	// 생성자가 아니라 BeginPlay에서 만든다(weak_from_this가 그때부터 유효).
	std::shared_ptr<Craft::SpriteAnimatorComponent> animator;

	std::string characterName;
};
