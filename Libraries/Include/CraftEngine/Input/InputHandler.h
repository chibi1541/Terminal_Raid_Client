#pragma once

#include "Utils/EngineMacro.h"
#include "Utils/Types.h"

#include <functional>
#include <memory>
#include <vector>

NAME_SPACE_BEGIN(Craft)

// 입력 이벤트의 종류.
//
// Held는 엄밀히는 이벤트가 아니라 상태지만, 눌려 있는 동안 매 프레임 콜백이
// 필요한 경우(이동, 마우스 드래그)가 많아서 같은 자리에 둔다.
enum class EInputEvent : uint8
{
	// 이번 프레임에 눌리기 시작함.
	Pressed,

	// 이번 프레임에 떼짐.
	Released,

	// 눌려 있는 동안 매 프레임.
	Held,
};

// 입력 우선순위 밴드.
// 어떤 값이 어떤 계층인지가 코드 여기저기 흩어지지 않도록 한 곳에 모은다.
//
// enum class가 아니라 constexpr int인 이유:
// 우선순위는 크기 비교로 쓰이고, 밴드 사이에 끼워 넣어야 할 때가 있다.
//   handler->SetPriority(InputPriority::UI + 1);   // 팝업 위의 툴팁
// enum class면 호출부마다 static_cast가 붙고, SetPriority가 열거값만 받게 하면
// 밴드에 있는 값 외에는 쓸 수 없어서 정수 우선순위를 고른 이유가 사라진다.
namespace InputPriority
{
	constexpr int Gameplay = 0;
	constexpr int World = 100;
	constexpr int UI = 1000;

	// 디버그 콘솔, 일시정지 등 무엇보다 먼저 처리되어야 하는 것들.
	constexpr int System = 10000;
}

// 키 하나하나에 실행할 함수를 묶어두는 바인딩 테이블.
//
// InputSystem이 아는 유일한 대상이 이 클래스다.
// Actor도 UI도 모르기 때문에, 액터에 붙이려면 InputComponent가 이걸 들고,
// 나중에 UI 위젯을 만들면 위젯이 똑같이 이걸 들면 된다.
// (액터 계층에 묶어두면 UI가 Actor여야만 하는 구조가 된다)
class CRAFT_API InputHandler : public std::enable_shared_from_this<InputHandler>
{
public:
	// 바인딩할 함수의 형태.
	using InputCallback = std::function<void()>;

	// 소유자가 아직 입력을 받을 수 있는 상태인지 물어보는 함수의 형태.
	using EnabledCheck = std::function<bool()>;

	// Register가 shared_from_this를 쓰므로 반드시 shared_ptr로 만들어야 한다.
	// 생성자를 감춰서 스택/new 생성을 막는다.
	static std::shared_ptr<InputHandler> Create();

	~InputHandler();

	// InputSystem에 등록/해제.
	void Register();
	void Unregister();

	// 키 하나에 실행할 함수를 등록한다.
	//
	// consume이 true면 이 핸들러가 그 키를 소비해서 우선순위가 낮은 쪽으로
	// 내려가지 않는다. 관찰만 하는 용도(디버그 로깅 등)라면 false를 준다.
	void BindKey(int keyCode, EInputEvent event, InputCallback callback, bool consume = true);

	// 멤버 함수를 바인딩하는 편의 오버로드.
	// object의 수명이 이 핸들러보다 길어야 한다(보통 이 핸들러를 소유한 객체).
	template<typename T>
	void BindKey(int keyCode, EInputEvent event, T* object, void (T::* method)(), bool consume = true)
	{
		BindKey(keyCode, event, [object, method]() { (object->*method)(); }, consume);
	}

	// 등록된 바인딩 제거.
	void ClearBindings();
	void ClearBindings(int keyCode);

	// getter/setter
	inline int GetPriority() const { return priority; }

	// 값이 바뀌면 InputSystem에 다시 정렬하라고 알린다.
	void SetPriority(int newPriority);

	inline bool IsEnabled() const { return isEnabled; }
	inline void SetEnabled(bool newEnabled) { isEnabled = newEnabled; }

	// 모달 다이얼로그용.
	// 바인딩이 없는 키까지 전부 소비해서 아래로 안 내려보낸다.
	inline bool IsBlockAllInput() const { return blockAllInput; }
	inline void SetBlockAllInput(bool newBlockAllInput) { blockAllInput = newBlockAllInput; }

	// 소유자가 살아있고 활성인지 물어보는 선택적 훅.
	// InputHandler가 Actor나 위젯을 몰라도 되게 해주는 지점이다.
	inline void SetEnabledCheck(EnabledCheck newCheck) { enabledCheck = std::move(newCheck); }

	// isEnabled이면서 enabledCheck도 통과했는지 여부.
	bool IsEffectivelyEnabled() const;

	// InputSystem 전용.
	// 이 키/이벤트에 바인딩이 하나라도 있었으면 true를 반환하고,
	// 그중 하나라도 consume이면 outConsumed를 true로 만든다.
	bool HandleInput(int keyCode, EInputEvent event, bool& outConsumed);

private:
	// Create를 통해서만 만들 수 있다.
	InputHandler() = default;

	// 키 하나 + 이벤트 종류 하나에 대한 바인딩.
	struct Binding
	{
		int keyCode = 0;
		EInputEvent event = EInputEvent::Pressed;
		InputCallback callback;
		bool consume = true;
	};

private:
	// 등록된 바인딩 목록.
	// 한 키에 여러 개를 걸 수 있으므로 맵이 아니라 배열로 둔다.
	std::vector<Binding> bindings;

	// 소유자 상태를 물어보는 훅. 비어있으면 항상 통과.
	EnabledCheck enabledCheck;

	// 우선순위. 큰 값이 먼저 처리된다.
	int priority = InputPriority::Gameplay;

	// 입력 수신 여부 플래그.
	bool isEnabled = true;

	// 바인딩이 없는 키도 막을지 여부.
	bool blockAllInput = false;

	// InputSystem에 등록되어 있는지 여부. 중복 등록 방지용.
	bool isRegistered = false;
};

NAME_SPACE_END
