#pragma once

#include "PlayerActor.h"
#include "Input/InputComponent.h"

#include <memory>

// 이 클라이언트의 유저가 조종하는 플레이어.
//
// 서버가 S_ENTER_ROOM.myObject로 알려준 개체 하나만 이 타입으로 스폰된다.
// 나머지는 전부 RemotePlayer다.
//
// 지금은 입력이 로컬에서만 반영된다(서버로 보내지 않는다).
// 다음 단계에서 C_MOVE / C_ATTACK 전송과 서버 보정이 이 자리에 들어온다.
class LocalPlayer : public PlayerActor
{
	TYPE_DECLARATIONS(LocalPlayer, PlayerActor)

public:
	LocalPlayer() = default;

	virtual void BeginPlay() override;
	virtual void Tick(float deltaTime) override;

protected:
	// 내 캐릭터는 노란색 이름표로 구분한다.
	virtual Craft::Color GetNameColor() const override { return Craft::Color::Yellow; }

private:
	// 입력 바인딩 콜백.
	//
	// 여기서는 값을 세우기만 하고 실제 처리(이동, 애니메이션 파라미터)는 Tick에서 한다.
	// 엔진이 디스패치를 Tick보다 먼저 돌리므로 같은 프레임 안에서 반영된다.
	void OnMoveLeft();
	void OnMoveRight();
	void OnMoveUp();
	void OnMoveDown();
	void OnAttack();
	void OnRollPressed();

private:
	std::shared_ptr<Craft::InputComponent> inputComponent;

	// 초당 이동할 칸 수
	float moveSpeed = 20.0f;

	// 이동 누적값. 1.0을 넘으면 한 칸 움직인다.
	// (프레임마다 무조건 한 칸씩 움직이면 프레임레이트에 따라 속도가 달라짐)
	float moveAmount = 0.0f;

	// 구르기 상태.
	// 스페이스로 켜지고, 구르기 클립의 RollEnd 노티파이를 받으면 꺼진다.
	bool isRolling = false;

	// 이번 프레임에 모인 입력. Tick 끝에서 리셋한다.
	Craft::Vector2 inputDirection = Craft::Vector2::Zero;
	bool isAttack = false;
};
