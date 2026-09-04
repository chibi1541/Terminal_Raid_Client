#include "pch.h"
#include "LocalPlayer.h"
#include "Component/SpriteAnimatorComponent.h"
#include "Camera/CameraManager.h"
#include "Input/Input.h"
#include "Render/Renderer.h"

#include <cmath>

using namespace Craft;

void LocalPlayer::BeginPlay()
{
	// TODO 나중에 서버로부터 인덱스를 받아서 해당 캐릭터 로딩하도록 수정
	animName = "Mage";

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

	// 뷰 회전도 누른 순간 한 번씩이므로 Pressed.
	// Held로 걸면 키를 꾹 누르는 동안 매 프레임 목표 각도가 밀려서 보간이 끝나지 않는다.
	inputComponent->BindKey('Q', EInputEvent::Pressed, this, &LocalPlayer::OnRotateViewLeft);
	inputComponent->BindKey('E', EInputEvent::Pressed, this, &LocalPlayer::OnRotateViewRight);

	// 화면의 기준점. 등록되는 순간 활성 카메라가 되어
	// 이 액터의 위치가 화면 중앙에 오도록 CameraManager가 매 프레임 뷰를 갱신한다.
	//
	// 이것도 InputComponent와 같은 이유로 super보다 앞이어야 한다.
	// CameraComponent는 자기 BeginPlay에서 CameraManager에 등록되는데,
	// 순서를 어기면 등록이 안 돼 활성 카메라가 없는 채로 뷰가 동결된다.
	// 그러면서도 Tick/Draw는 정상으로 불리기 때문에 원인을 찾기 어렵다.
	cameraComponent = AddComponent<CameraComponent>();

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

void LocalPlayer::OnRotateViewLeft()
{
	// 예외 처리 - BeginPlay 전에는 컴포넌트가 없다.
	if (nullptr == cameraComponent)
	{
		return;
	}

	// 스크린 y가 아래로 갈수록 커지는 좌표계라
	// +1이 화면상 시계 방향이다. Q는 그 반대.
	cameraComponent->AddViewQuarterTurns(-1, viewRotateBlendTime);
}

void LocalPlayer::OnRotateViewRight()
{
	if (nullptr == cameraComponent)
	{
		return;
	}

	cameraComponent->AddViewQuarterTurns(1, viewRotateBlendTime);
}

Craft::EFacing LocalPlayer::ComputeWorldFacing() const
{
	CameraManager& camera = CameraManager::Get();

	// 뷰 회전 보간(0.25초) 중에는 화면 좌표가 매 프레임 미끄러진다.
	// 그 사이에 각을 다시 재면 회전이 끝날 때까지 슬롯이 계속 요동친다. 끝날 때까지 잡아둔다.
	if (camera.IsRotationBlending())
	{
		return facing;
	}

	// 구르는 동안만 방향을 잠근다.
	//
	// 구르기는 방향을 정해서 몸을 던지는 동작이라 도중에 조준을 따라가면 안 되고,
	// 끝나는 시점도 RollEnd 노티파이로 분명하게 정해져 있다.
	//
	// ★ 공격은 잠그지 않는다 ★
	// 로컬 플레이어에게 마우스는 절대 기준이다. 게다가 좌클릭은 Held로도 묶여 있어서
	// 여기에 isAttack을 넣으면 버튼을 누르고 있는 내내 방향이 얼어붙는다 -
	// 누르면 멈추고 떼면 따라오는, 갱신이 멋대로인 것처럼 보이는 움직임이 된다.
	if (isRolling)
	{
		return facing;
	}

	const Vector2 selfScreen =
		camera.WorldToScreen(GetPosition() + Vector2(0, facingAnchorOffsetY));

	// 마우스는 애초에 콘솔 셀 좌표로 들어온다.
	// "월드가 아니라 카메라 기준"이라는 규칙이 변환 없이 그대로 성립한다.
	const Vector2 mousePosition = Input::Get().GetMousePosition();

	const int deltaX = mousePosition.x - selfScreen.x;
	const int deltaY = mousePosition.y - selfScreen.y;

	// 기준점과 정확히 겹치면 각도가 정의되지 않는다. 보던 방향을 유지한다.
	if (0 == deltaX && 0 == deltaY)
	{
		return facing;
	}

	// 콘솔은 y가 아래로 커진다. -deltaY로 뒤집어야 화면 위쪽이 +90°가 되고,
	// 그래야 "60~120도는 뒷모습"이라는 규칙이 성립한다.
	// 콘솔 셀이 정사각(8x8 폰트)이라 종횡비 보정은 필요 없다.
	const float radians =
		::atan2f(-static_cast<float>(deltaY), static_cast<float>(deltaX));

	const float degrees = NormalizeDegrees(radians * 57.29578f);

	const int quarterTurns = camera.GetViewQuarterTurns();

	// 히스테리시스는 화면 슬롯 기준으로 건다 - 흔들리는 것은 커서고, 커서는 화면에 있다.
	const EFacing previousScreenSlot = RotateFacing(facing, quarterTurns);

	const EFacing screenSlot = FacingFromScreenAngleSticky(
		degrees, previousScreenSlot, facingHysteresisDegrees);

	// 화면 슬롯 -> 월드 방향. displaySlot 계산(RotateFacing(facing, k))의 역변환이다.
	// 화면 값을 그대로 들고 있으면 카메라를 돌리는 순간 캐릭터가 실제로 도는 셈이 된다.
	return RotateFacing(screenSlot, -quarterTurns);
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

		// 좌우 반전은 여기서 하지 않는다.
		// 어느 방향 그림을 쓸지와 그걸 뒤집을지는 PlayerActor::UpdateFacing이 고른
		// 방향 슬롯이 정한다(SpriteAnimatorComponent::SetFacing 참고).
		// 여기서 SetFlipX를 부르면 다음 틱에 슬롯 값으로 덮어써져 아무 효과가 없다.
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

void LocalPlayer::Draw()
{
	super::Draw();

	// 액터는 월드 객체이므로 월드 좌표로 제출한다. 카메라가 화면 좌표로 옮긴다.
// 정렬 순서를 한 칸 올려서 스프라이트에 가리지 않게 한다.
	Renderer::Get().SubmitWorld(
		characterName,
		GetPosition() + Vector2(0, nameLabelOffsetY),
		GetNameColor(),
		GetSortingOrder() + 1);
}
