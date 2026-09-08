#pragma once
#include "Level/Level.h"
#include "Math/Rect.h"

// 서버에 붙기 전에 뜨는 타이틀/메뉴 레벨.
//
// 큰 픽셀 문자 "TERMINAL RAID" 타이틀(초록) + 그 아래 작은 1셀 텍스트 버튼
// "Game Start" / "Exit Game". 마우스가 버튼 위에 있으면 반전 하이라이트, 좌클릭으로 실행한다.
//  - Game Start : StartServerConnection() 으로 로그인/룸 입장을 개시하고,
//                 S_ENTER_ROOM 이 도착하면 ObjectManager 가 TileMapLevel 로 교체한다.
//  - Exit Game  : Engine::Get().Quit().
//
// 액터도 위젯도 쓰지 않는다. Draw 에서 직접 텍스트를 제출하고 Tick 에서 마우스를 폴링한다.
class MenuLevel : public Craft::Level
{
	TYPE_DECLARATIONS(MenuLevel, Craft::Level)

public:
	virtual void OnInitialized() override;
	virtual void Tick(float deltaTime) override;
	virtual void Draw() override;

private:
	enum class Button { None, Start, Exit };

	// 화면 크기로 두 버튼의 스크린 사각형을 다시 계산한다. Tick/Draw 둘 다 첫머리에서 부른다.
	void ComputeLayout();

	Button      _hovered = Button::None;

	// Game Start 를 누른 뒤. 실제 레벨 교체는 S_ENTER_ROOM 도착 시 일어나므로
	// 그때까지 입력을 잠그고 "connecting..." 을 띄운다.
	bool        _connecting = false;

	Craft::Rect _startRect;
	Craft::Rect _exitRect;
};
