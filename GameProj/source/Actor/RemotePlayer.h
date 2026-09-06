#pragma once

#include "ReplCharacter.h"

// 다른 유저가 조종하는 플레이어.
//
// 입력을 받지 않는다. 위치와 상태는 오직 서버가 보낸 ObjectInfo/MoveInfo로만 바뀐다.
// 스폰 시점 위치는 ApplyObjectInfo(베이스)가, 그 뒤 매 이동은 ApplyMove(베이스)가
// interpolator에 샘플로 쌓고, 여기 Tick이 매 프레임 재생 좌표를 꺼내 반영한다.
class RemotePlayer : public ReplCharacter
{
	TYPE_DECLARATIONS(RemotePlayer, ReplCharacter)

public:
	RemotePlayer() = default;

	virtual void BeginPlay() override;
	virtual void Tick(float deltaTime) override;

	// 서버 공격 모션 트리거. "이 순간 재생하라"는 1회성 신호라 매 틱 리셋한다
	// (Tick 끝에서 isAttacking = false) - LocalPlayer가 매 프레임 입력을 다시
	// 채우고 끝에서 비우는 것과 같은 관례다.
	virtual void ApplyAttackStart(const Protocol::S_ATTACK_START& pkt) override;

protected:
	// 남의 캐릭터는 흰색 이름표.
	virtual Craft::Color GetNameColor() const override { return Craft::Color::White; }

	// 마우스가 없으니 각도를 잴 기준이 없다. 대신 서버가 알려준 이동 방향이 근거다.
	//
	// 공격이 이동을 이긴다 - 옆으로 물러나면서 앞을 때리는 동작에서 봐야 하는 것은
	// 물러나는 쪽이 아니라 때리는 쪽이다. LocalPlayer가 마우스를 절대 기준으로 삼는 것과
	// 같은 이유(= 보는 방향은 조준 방향이다)이고, 근거만 다르다.
	virtual Craft::EFacing ComputeWorldFacing() const override;

protected:
	// TODO : 공격 방향. 프로토콜에 아직 공격 패킷이 없어서 항상 꺼져 있다.
	//        C_ATTACK / S_ATTACK이 생기면 ObjectManager가 여기 꽂아준다.
	//        자리를 미리 만들어 두는 이유는, 이 두 줄이 없으면 위 규칙이
	//        "이동 방향으로 정한다"로 읽혀서 나중에 규칙 자체를 다시 세우게 되기 때문이다.
	Craft::Vector2 attackDirection = Craft::Vector2::Zero;
	bool isAttacking = false;
};
