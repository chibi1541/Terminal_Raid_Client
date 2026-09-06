#include "pch.h"
#include "RemotePlayer.h"
#include "Component/SpriteAnimatorComponent.h"

#include <cmath>

using namespace Craft;

void RemotePlayer::BeginPlay()
{
	animName = "Knight";

	super::BeginPlay();
}

void RemotePlayer::Tick(float deltaTime)
{
	// 보간기가 시작된 뒤에만 반영한다 - ApplyObjectInfo(스폰)를 안 거친
	// 액터(로컬 테스트 스폰 등)는 시작되지 않은 채라 원점으로 튀는 걸 막는다.
	if (interpolator.IsStarted())
	{
		SetPosition(interpolator.Evaluate(deltaTime));
	}

	// 컴포넌트(= 애니메이션 평가/재생)로 deltaTime을 전달하는 처리가 여기 들어있다.
	super::Tick(deltaTime);

	// 예외 처리 - BeginPlay 전에는 컴포넌트가 없다.
	if (nullptr != animator)
	{
		// LocalPlayer::Tick과 같은 파라미터 계약(speed/IsAttack/IsDead)을 쓴다 -
		// 캐릭터 종류(Knight)가 같으면 상태 머신도 같은 파라미터를 기대한다.
		// speed는 서버가 보낸 최신 속도 벡터의 크기 - 실제로 움직이고 있는지를
		// 클라이언트가 따로 추정하지 않고 서버 값 그대로 넘긴다.
		const MovementInterpolator::FVec2 velocity = interpolator.GetVelocity();
		const float currentSpeed = std::sqrt(velocity.x * velocity.x + velocity.y * velocity.y);

		animator->GetParameters().SetFloat("speed", currentSpeed);
		animator->GetParameters().SetFloat("IsAttack", isAttacking ? 1.0f : 0.0f);
		animator->GetParameters().SetFloat("IsDead", IsAlive() ? 0.0f : 1.0f);
	}

	// S_ATTACK_START는 "이 순간 재생하라"는 1회성 트리거다. 여기서 지우지
	// 않으면 다음 패킷이 올 때까지 계속 공격 상태로 남는다.
	isAttacking = false;
}

void RemotePlayer::ApplyAttackStart(const Protocol::S_ATTACK_START& pkt)
{
	isAttacking = true;
	attackDirection = DeltaFromServerDirection(pkt.dir());
}

EFacing RemotePlayer::ComputeWorldFacing() const
{
	if (isAttacking && attackDirection != Vector2::Zero)
	{
		return FacingFromDelta(attackDirection, facing);
	}

	return FacingFromServerDirection(lastDirection, facing);
}
