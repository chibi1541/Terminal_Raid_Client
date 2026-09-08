#pragma once

#include "Actor/Actor.h"
#include "Input/InputComponent.h"
#include "Math/Rect.h"

#include <memory>

// 인게임 일시정지 메뉴.
//
// ESC 로 열고 닫는다. 열려 있는 동안:
//   - 화면 가운데에 패널(디버그 HUD 와 같은 DarkBlue + White 테두리) + 픽셀 문자 버튼
//     "Continue Game" / "Exit Game" 를 그린다. 패널 밖은 게임 화면이 그대로 보인다.
//   - System 우선순위 InputComponent 가 blockAllInput 으로 플레이어 입력을 전부 막는다.
//   - 게임 루프/네트워크는 계속 돈다(멈추지 않는다).
//
// Continue Game -> 닫고 게임 복귀. Exit Game -> Engine::Get().Quit().
//
// ObjectManager 가 룸 입장 때 하나 스폰하고 weak_ptr 로 들고 있다(ServerDebugActor 와 같은 규약).
class PauseMenuActor : public Craft::Actor
{
	TYPE_DECLARATIONS(PauseMenuActor, Craft::Actor)

public:
	PauseMenuActor() = default;

	virtual void BeginPlay() override;
	virtual void Tick(float deltaTime) override;
	virtual void Draw() override;

	bool IsOpen() const { return _open; }

private:
	enum class Button { None, Continue, Exit };

	void OnToggle();
	void SetOpen(bool open);

	// 화면 크기로 패널/버튼 사각형을 다시 계산한다. Tick/Draw 첫머리에서 부른다.
	void ComputeLayout();

	std::shared_ptr<Craft::InputComponent> _input;

	bool        _open = false;
	Button      _hovered = Button::None;

	Craft::Rect _panelRect;
	Craft::Rect _continueRect;
	Craft::Rect _exitRect;
};
