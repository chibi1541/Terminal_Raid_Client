#pragma once

#include "PlayerActor.h"
#include "Input/InputComponent.h"
#include "Camera/CameraComponent.h"

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

	// 마우스가 절대 기준이다.
	//
	// 이동 방향으로 정하지 않는 이유 - 이 게임에서 "보는 방향"은 조준 방향이다.
	// 뒷걸음질치며 앞을 겨누는 동작이 이동 방향 기준으로는 표현되지 않는다.
	// 원격 플레이어와 몬스터는 마우스가 없어서 이동/공격 방향으로 대신한다(RemotePlayer 참고).
	virtual Craft::EFacing ComputeWorldFacing() const override;

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

	// 뷰를 90도씩 돌린다. Q가 반시계, E가 시계 방향.
	//
	// 다른 입력과 달리 값만 세우고 Tick으로 미루지 않고 여기서 바로 처리한다.
	// 이동/애니메이션과 달리 누적할 것도 없고, 카메라 상태는 CameraManager가 가진다.
	void OnRotateViewLeft();
	void OnRotateViewRight();

private:
	std::shared_ptr<Craft::InputComponent> inputComponent;

	// 이 클라이언트의 화면을 비추는 카메라.
	//
	// autoActivate 기본값이 true라 등록되는 순간 활성 카메라가 된다.
	// RemotePlayer에는 붙이지 않는다 - 화면을 비추는 건 내가 조종하는 하나뿐이다.
	std::shared_ptr<Craft::CameraComponent> cameraComponent;

	// 뷰 회전에 쓰는 보간 시간(초). 0이면 즉시 스냅이라 화면이 튄다.
	static constexpr float viewRotateBlendTime = 0.25f;

	// 마우스 각도를 재는 기준점을 액터 위치에서 얼마나 위로 올릴지(칸).
	//
	// 액터 위치는 발밑이고 스프라이트는 거기서 위로 뻗어 있다. 발밑을 기준으로 각을 재면
	// 캐릭터의 "가슴"보다 아래에 원점이 놓여서, 커서를 캐릭터 몸통 위에 얹어도
	// 아래쪽(앞모습) 섹터로 계산된다. 몸 한가운데로 올려야 화면에서 보이는 대로 맞는다.
	static constexpr int facingAnchorOffsetY = -4;

	// 섹터 경계에서 방향을 유지하는 여유각(도).
	//
	// 없으면 커서가 경계에 걸쳐 있을 때 1칸 흔들림에도 앞뒤 그림이 매 프레임 교차한다.
	static constexpr float facingHysteresisDegrees = 8.0f;

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
