#include "pch.h"
#include "LocalPlayer.h"

using namespace Craft;

void LocalPlayer::BeginPlay()
{
	// 주의 - super::BeginPlay()는 이 함수의 "맨 마지막"에 불러야 한다.
	//
	// Actor::BeginPlay가 그 시점의 componentList를 훑어 각 컴포넌트의 BeginPlay를 부르는데,
	// InputComponent는 자기 BeginPlay에서 handler->Register()로 InputSystem에 등록된다.
	// 그래서 super를 먼저 부르고 컴포넌트를 나중에 추가하면
	// 그 컴포넌트의 BeginPlay가 영영 불리지 않아 입력이 한 번도 들어오지 않는다.

	// 게임플레이 입력 바인딩.
	//
	// 액터가 매 틱 키를 물어보는 게 아니라, 어떤 키에 무엇을 할지만 등록한다.
	// 어떤 키가 눌렸는지 찾아서 부르는 일은 InputSystem이 한다.
	inputComponent = AddComponent<InputComponent>();
	inputComponent->SetInputPriority(InputPriority::Gameplay);

	// 이동은 눌려 있는 동안 계속 필요하므로 Held.
	// 네 방향을 따로 걸어두면 대각선 입력이 자연스럽게 합쳐진다.
	// 콘솔 좌표는 y가 아래로 갈수록 커지므로 위/아래가 반대다.
	inputComponent->BindKey('A', EInputEvent::Held, this, &LocalPlayer::OnMoveLeft);
	inputComponent->BindKey('D', EInputEvent::Held, this, &LocalPlayer::OnMoveRight);
	inputComponent->BindKey('W', EInputEvent::Held, this, &LocalPlayer::OnMoveUp);
	inputComponent->BindKey('S', EInputEvent::Held, this, &LocalPlayer::OnMoveDown);

	inputComponent->BindKey(VK_LBUTTON, EInputEvent::Pressed, this, &LocalPlayer::OnAttack);
	inputComponent->BindKey(VK_LBUTTON, EInputEvent::Held, this, &LocalPlayer::OnAttack);

	// 구르기는 누른 순간 한 번만 시작되어야 하므로 Pressed.
	inputComponent->BindKey(VK_SPACE, EInputEvent::Pressed, this, &LocalPlayer::OnRollPressed);

	// 애니메이터 셋업은 PlayerActor가 하고, 컴포넌트 BeginPlay 전파는 Actor가 한다.
	// 위에서 만든 inputComponent도 여기서 함께 BeginPlay를 받는다.
	super::BeginPlay();
}

void LocalPlayer::OnMoveLeft()
{
	inputDirection.x -= 1;
}

void LocalPlayer::OnMoveRight()
{
	inputDirection.x += 1;
}

void LocalPlayer::OnMoveUp()
{
	inputDirection.y -= 1;
}

void LocalPlayer::OnMoveDown()
{
	inputDirection.y += 1;
}

void LocalPlayer::OnAttack()
{
	isAttack = true;
}

void LocalPlayer::OnRollPressed()
{
	isRolling = true;
}

void LocalPlayer::Tick(float deltaTime)
{
	// 컴포넌트(= 애니메이션 평가/재생)로 deltaTime을 전달하는 처리가 여기 들어있다.
	super::Tick(deltaTime);

	const bool isMoving = (inputDirection != Vector2::Zero);

	// 예외 처리 - BeginPlay 전에는 컴포넌트가 없다.
	if (nullptr != animator)
	{
		// 애니메이션이 게임플레이에게 보내는 유일한 신호.
		//
		// 파이프라인은 "게임플레이 -> 파라미터 -> 전이 -> 클립"으로 흐르는 단방향인데,
		// 구르기 종료만은 반대 방향이 필요하다. 클립이 끝났다는 걸 여기서 듣고 조종을 되돌린다.
		// 이게 없으면 IsRolling이 계속 참이라 Any->Roll이 매번 다시 잡아채 구르기에 갇힌다.
		if (animator->HasNotify("RollEnd"))
		{
			isRolling = false;
		}

		// 게임플레이가 애니메이션에 넘기는 건 이 값들뿐이다.
		// 어떤 클립을 틀지는 상태 머신의 전이 규칙이 정한다.
		animator->GetParameters().SetFloat("speed", isMoving ? moveSpeed : 0.0f);
		animator->GetParameters().SetFloat("IsAttack", isAttack ? 1.0f : 0.0f);
		animator->GetParameters().SetFloat("IsRolling", isRolling ? 1.0f : 0.0f);

		// 가로 입력이 있을 때만 방향을 바꾼다.
		// 위아래로만 움직이거나 멈춰 있는 동안에는 보던 방향을 유지한다.
		if (inputDirection.x != 0)
		{
			animator->SetFlipX(inputDirection.x < 0);
		}
	}

	if (isMoving)
	{
		// 초당 moveSpeed칸 속도로 이동 (프레임레이트가 달라져도 속도는 일정)
		//
		// 지금은 서버에 알리지 않는 순수 로컬 이동이다.
		// 서버도 스폰 뒤에는 위치를 갱신하지 않으므로 좌표가 서로 어긋난 채로 간다.
		// 다음 단계에서 C_MOVE 전송과 S_MOVE 보정이 여기 붙는다.
		moveAmount += deltaTime * moveSpeed;

		while (moveAmount >= 1.0f)
		{
			moveAmount -= 1.0f;
			SetPosition(GetPosition() + inputDirection);
		}
	}
	else
	{
		// 키를 뗀 상태. 다음 입력에 바로 반응하도록 누적값을 비운다.
		moveAmount = 0.0f;
	}

	// 이번 프레임에 모인 입력은 여기까지만 유효하다.
	// 다음 프레임의 디스패치가 이 Tick 뒤에 오므로 지금 비워도 안전하다.
	inputDirection = Vector2::Zero;
	isAttack = false;
}
