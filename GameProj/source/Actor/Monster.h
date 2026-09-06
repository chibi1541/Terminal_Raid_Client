#pragma once

#include "ReplCharacter.h"

#include <memory>

// 전방 선언
NAME_SPACE_BEGIN(Craft)

class SpriteAnimatorComponent;

NAME_SPACE_END

class Monster : public ReplCharacter
{
	TYPE_DECLARATIONS(Monster, ReplCharacter)

public:
	Monster() = default;
	virtual ~Monster() = default;

	virtual void ApplyObjectInfo(const Protocol::ObjectInfo& info) override;
	virtual void BeginPlay() override;
	virtual void Tick(float deltaTime) override;

protected:
	// 마우스도 입력도 없다. 서버가 알려준 이동 방향(lastDirection)이 유일한 근거다.
	// RemotePlayer와 같은 규칙 - ReplCharacter::FacingFromServerDirection 참고.
	virtual Craft::EFacing ComputeWorldFacing() const override
	{
		return FacingFromServerDirection(lastDirection, facing);
	}

private:
	float moveSpeed = 20.f;
	
	// 이동 누적값. 1.0을 넘으면 한 칸 움직인다.
	// (프레임마다 무조건 한 칸씩 움직이면 프레임레이트에 따라 속도가 달라짐)
	float moveAmount = 0.0f;


	// 애니메이션 & AI와 연관이 있는 상태 변수
	bool isAttack = false;

};

