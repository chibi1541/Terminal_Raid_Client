#pragma once
#include "Level/Level.h"
#include "Math/Rect.h"

#include <string>

// 서버에 붙기 전에 뜨는 타이틀/메뉴 레벨.
//
// 큰 픽셀 문자 "TERMINAL RAID" 타이틀(초록) 아래로, 재목 → 플레이어 이름 → 캐릭터 타입 →
// 스타트 버튼 → 종료 버튼 순으로 배치한다.
//  - 플레이어 이름 : 키보드로 입력(A-Z 0-9 스페이스, 백스페이스). 빈칸이면 서버가 "Player" 로 잡는다.
//  - 캐릭터 타입   : 좌우 삼각형(< >) 버튼으로 Knight → Archer → Mage 순환. 최초 Knight.
//  - Game Start    : 고른 이름/타입을 GLoginRequest 에 넣고 StartServerConnection().
//                    S_ENTER_ROOM 이 도착하면 ObjectManager 가 TileMapLevel 로 교체한다.
//  - Exit Game     : Engine::Get().Quit().
//
// 액터도 위젯도 쓰지 않는다. Draw 에서 직접 텍스트를 제출하고 Tick 에서 마우스/키보드를 폴링한다.
class MenuLevel : public Craft::Level
{
	TYPE_DECLARATIONS(MenuLevel, Craft::Level)

public:
	virtual void OnInitialized() override;
	virtual void Tick(float deltaTime) override;
	virtual void Draw() override;

private:
	enum class Button { None, Start, Exit, CharPrev, CharNext };

	// 화면 크기로 각 UI 요소의 스크린 사각형을 다시 계산한다. Tick/Draw 둘 다 첫머리에서 부른다.
	void ComputeLayout();

	// 키보드로 이름 한 글자씩 편집.
	void PollNameInput();

	Button      _hovered = Button::None;

	// Game Start 를 누른 뒤. 실제 레벨 교체는 S_ENTER_ROOM 도착 시 일어나므로
	// 그때까지 입력을 잠그고 "connecting..." 을 띄운다.
	bool        _connecting = false;

	std::string _playerName;			// 빈 문자열 = 기본값("Player")
	int         _charType = 1;			// Protocol::CHARACTER_KNIGHT

	int         _nameRowY = 0;
	int         _charRowY = 0;

	Craft::Rect _startRect;
	Craft::Rect _exitRect;
	Craft::Rect _charPrevRect;
	Craft::Rect _charNextRect;
};
