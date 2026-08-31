#pragma once
#include "Actor/Actor.h"
#include "Component/SpriteAnimatorComponent.h"
#include "Input/InputComponent.h"
#include "UI/UserWidget.h"
#include <memory>

using namespace Craft;

class TestActor : public Actor
{
	TYPE_DECLARATIONS(TestActor, Actor)

public:
	TestActor(const Vector2& position, Color color = Color::White);

	virtual void BeginPlay() override;
	virtual void Tick(float deltaTime) override;

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
	void OnMouseHeld();
	void OnToggleBlock();

	// 일시정지 메뉴 열기.
	void OnOpenMenu();

	// 입력 래치 확인용.
	//
	// 메뉴가 Esc를 소비해서 자기를 닫을 때, 그 Esc가 게임플레이까지 내려오면 안 된다.
	// 내려온다면 아래 카운터가 오른다.
	void OnEscapePressed();
	void OnEscapeReleased();
	void OnEscapeHeld();

	// 일시정지 메뉴를 만든다. 화면에 올리는 것은 BeginPlay에서 한다.
	void CreatePauseMenu();

private:
	// 스프라이트 애니메이션 재생을 담당하는 컴포넌트.
	// 생성자가 아니라 BeginPlay에서 만든다(weak_from_this가 그때부터 유효).
	std::shared_ptr<SpriteAnimatorComponent> animator;

	// 게임플레이 입력. 가장 낮은 계층이다.
	std::shared_ptr<InputComponent> inputComponent;

	// 계층/모달 동작 확인용. InputPriority::UI로 올려둔 컴포넌트.
	// B키로 켜면 아래 계층(=inputComponent)의 입력이 전부 막힌다.
	std::shared_ptr<InputComponent> blockerInput;

	// 초당 이동할 칸 수
	float moveSpeed = 20.0f;

	// 이동 누적값. 1.0을 넘으면 한 칸 움직인다.
	// (프레임마다 무조건 한 칸씩 움직이면 프레임레이트에 따라 속도가 달라짐)
	float moveAmount = 0.0f;

	// 구르기 상태 변수.
	// 스페이스로 켜지고, 구르기 클립의 RollEnd 노티파이를 받으면 꺼진다.
	bool isRolling = false;

	// 이번 프레임에 모인 입력. Tick 끝에서 리셋한다.
	Vector2 inputDirection = Vector2::Zero;
	bool isAttack = false;

	// 동작 확인용 카운터.
	// 스페이스를 꾹 눌러도 rollPressCount가 1만 오르면 Pressed가 정상이다.
	// 마우스를 누르고 있으면 mouseHeldFrameCount가 계속 올라야 Held가 정상이다.
	int rollPressCount = 0;
	int mouseHeldFrameCount = 0;

	// 모달 차단 상태.
	bool isBlocking = false;

	// M으로 열고 Esc로 닫는 일시정지 메뉴.
	// 게임플레이가 자기 UI를 소유하는 구조다(언리얼에서 플레이어 컨트롤러가 하는 일).
	std::shared_ptr<Craft::UI::UserWidget> pauseMenu;

	// 게임플레이가 받은 Esc 이벤트 수.
	//
	// 메뉴를 여닫아도 이 값들이 0에서 안 움직여야 정상이다.
	// 메뉴가 없을 때 Esc를 누르면 오른다(바인딩이 살아있다는 확인).
	int escapePressedCount = 0;
	int escapeReleasedCount = 0;
	int escapeHeldCount = 0;
};
