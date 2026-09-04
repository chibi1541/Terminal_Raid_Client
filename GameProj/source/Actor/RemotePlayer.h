#pragma once

#include "ReplCharacter.h"
#include "Render/Renderer.h"

// 다른 유저가 조종하는 플레이어.
//
// 입력을 받지 않는다. 위치와 상태는 오직 서버가 보낸 ObjectInfo로만 바뀐다.
//
// TODO : 위치 보간.
// 지금은 서버 값을 그대로 꽂아서 갱신이 올 때마다 순간이동한다.
// 이동 패킷(S_MOVE)이 생기면 목표 지점을 향해 부드럽게 따라가도록 바꾼다.

//TODO : cpp 만들자 좀!
class RemotePlayer : public ReplCharacter
{
	TYPE_DECLARATIONS(RemotePlayer, ReplCharacter)

public:
	RemotePlayer() = default;

	virtual void BeginPlay() override
	{
		animName = "Knight";

		super::BeginPlay();
	}

	// 서버가 보낸 위치에서 이동 방향을 뽑아낸다.
	//
	// 방향을 따로 받지 않기 때문에 위치 변화가 유일한 단서다.
	// 서버가 방향을 보내주기 시작하면 이 유추는 사라지고 값을 그대로 받으면 된다.
	virtual void ApplyObjectInfo(const Protocol::ObjectInfo& info) override
	{
		const Craft::Vector2 previousPosition = GetPosition();

		super::ApplyObjectInfo(info);

		const Craft::Vector2 delta = GetPosition() - previousPosition;

		// 제자리면 마지막 이동 방향을 지우지 않는다. 멈춰 섰다고 정면으로 홱 돌면
		// 서버 갱신이 뜸한 순간마다 캐릭터가 방향을 잃는다.
		if (delta != Craft::Vector2::Zero)
		{
			lastMoveDelta = delta;
		}
	}

protected:
	// 남의 캐릭터는 흰색 이름표.
	virtual Craft::Color GetNameColor() const override { return Craft::Color::White; }

	// 마우스가 없으니 각도를 잴 기준이 없다. 대신 행동이 방향을 말해준다.
	//
	// 공격이 이동을 이긴다 - 옆으로 물러나면서 앞을 때리는 동작에서 봐야 하는 것은
	// 물러나는 쪽이 아니라 때리는 쪽이다. LocalPlayer가 마우스를 절대 기준으로 삼는 것과
	// 같은 이유(= 보는 방향은 조준 방향이다)이고, 근거만 다르다.
	virtual Craft::EFacing ComputeWorldFacing() const override
	{
		if (isAttacking && attackDirection != Craft::Vector2::Zero)
		{
			return Craft::FacingFromDelta(attackDirection, facing);
		}

		if (lastMoveDelta != Craft::Vector2::Zero)
		{
			return Craft::FacingFromDelta(lastMoveDelta, facing);
		}

		return facing;
	}

	// TODO : 함수 정리 왜 이거 cpp 안만들었냐?
	virtual void Draw() override
	{
		super::Draw();

		// 액터는 월드 객체이므로 월드 좌표로 제출한다. 카메라가 화면 좌표로 옮긴다.
// 정렬 순서를 한 칸 올려서 스프라이트에 가리지 않게 한다.
		Craft::Renderer::Get().SubmitWorld(
			characterName,
			GetPosition() + Craft::Vector2(0, -9),
			GetNameColor(),
			GetSortingOrder() + 1);
	}

protected:
	// 마지막으로 관측된 이동 변화량. ApplyObjectInfo가 채운다.
	Craft::Vector2 lastMoveDelta = Craft::Vector2::Zero;

	// TODO : 공격 방향. 프로토콜에 아직 공격 패킷이 없어서 항상 꺼져 있다.
	//        C_ATTACK / S_ATTACK이 생기면 ObjectManager가 여기 꽂아준다.
	//        자리를 미리 만들어 두는 이유는, 이 두 줄이 없으면 위 규칙이
	//        "이동 방향으로 정한다"로 읽혀서 나중에 규칙 자체를 다시 세우게 되기 때문이다.
	Craft::Vector2 attackDirection = Craft::Vector2::Zero;
	bool isAttacking = false;
};
