#include "pch.h"
#include "Monster.h"
#include "Component/SpriteAnimatorComponent.h"

#include <cmath>

void Monster::ApplyObjectInfo(const Protocol::ObjectInfo& info)
{
	super::ApplyObjectInfo(info);
}

void Monster::BeginPlay()
{
	// 나중에 수정
	animName = "Necromancer";

	super::BeginPlay();
}

void Monster::Tick(float deltaTime)
{
	// 보간기가 시작된 뒤에만 반영한다 - ApplyObjectInfo(스폰)를 안 거친 액터
	// (Game.cpp의 "//temp" 테스트 스폰 등)는 시작되지 않은 채라 원점으로 튀는 걸 막는다.
	if (interpolator.IsStarted())
	{
		SetPosition(interpolator.Evaluate(deltaTime));
	}

	// 컴포넌트(= 애니메이션 평가/재생)로 deltaTime을 전달하는 처리가 여기 들어있다.
	super::Tick(deltaTime);

	// 예외 처리 - BeginPlay 전에는 컴포넌트가 없다.
	if (nullptr != animator)
	{
		// RemotePlayer/LocalPlayer와 같은 파라미터 계약(speed/IsAttack/IsDead) -
		// necromancer 상태 머신도 같은 이름의 파라미터를 기대한다.
		// speed는 서버가 보낸 최신 속도 벡터의 크기.
		const MovementInterpolator::FVec2 velocity = interpolator.GetVelocity();
		const float currentSpeed = std::sqrt(velocity.x * velocity.x + velocity.y * velocity.y);

		animator->GetParameters().SetFloat("speed", currentSpeed);
		animator->GetParameters().SetFloat("IsAttack", isAttack ? 1.0f : 0.0f);
		animator->GetParameters().SetFloat("IsDead", IsAlive() ? 0.0f : 1.0f);
	}
}


