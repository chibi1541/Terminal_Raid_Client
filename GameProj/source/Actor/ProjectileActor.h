#pragma once

#include "ReplicatedActor.h"
#include "Protocol/Enum.pb.h"
#include <memory>

// 전방 선언
NAME_SPACE_BEGIN(Craft)

class AnimationPlayer;

NAME_SPACE_END


// 서버 권위 직진 투사체. 스폰 스냅샷의 속도 벡터(CreatureState.velSub)로 데드레커닝한다.
// MovementInterpolator(200ms 지연)는 빠른 투사체에 안 맞아서 안 쓴다 -
// 매 프레임 pos += vel*dt, S_MOVE(posSub) 도착 시 그 권위 위치로 부드럽게 당겨 오차만 보정.
class ProjectileActor : public ReplicatedActor
{
	TYPE_DECLARATIONS(ProjectileActor, ReplicatedActor)

public:
	ProjectileActor();

	virtual void BeginPlay() override;
	virtual void Tick(float deltaTime) override;
	virtual void Draw() override;

	virtual void ApplyObjectInfo(const Protocol::ObjectInfo& info) override;
	virtual void ApplyMove(const Protocol::MoveInfo& info) override;

	// ObjectManager::Spawn 이 서버 ProjectileType 을 BeginPlay 전에 꽂는다.
	void SetProjectileType(Protocol::ProjectileType type) { projType = type; }

protected:
	std::shared_ptr<Craft::AnimationPlayer> animPlayer;

	// 서버가 준 종류. Projectile_None 이면 BeginPlay 가 ProjectileData 기본값을 쓴다.
	Protocol::ProjectileType projType = Protocol::Projectile_None;

	// 데드레커닝 상태 (셀 단위, float).
	float renderX = 0.0f;
	float renderY = 0.0f;
	float velCellsX = 0.0f;	// 셀/초
	float velCellsY = 0.0f;
	bool  started = false;
};
