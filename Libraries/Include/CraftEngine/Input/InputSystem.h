#pragma once

#include "Utils/EngineMacro.h"
#include "Input/InputHandler.h"

#include <memory>
#include <vector>

NAME_SPACE_BEGIN(Craft)

// 이번 프레임의 입력 상태를 이벤트로 바꿔서 등록된 InputHandler에 전달하는 시스템.
//
// Input이 "지금 어떤 키가 어떤 상태인가"를 들고 있다면,
// 여기는 "그래서 누구의 어떤 함수를 부를 것인가"를 정한다.
//
// 계층 처리:
// 우선순위가 높은 핸들러부터 순회하다가 키를 소비하는 핸들러를 만나면 멈춘다.
// 소비한 핸들러는 그 키를 뗄 때까지 소유(래치)하므로, 중간에 다른 핸들러가
// 끼어들어서 Pressed와 Released의 짝이 어긋나는 일이 없다.
class CRAFT_API InputSystem
{
	// Engine 클래스 friend 선언.
	friend class Engine;

	// 관리할 가상 키의 수. Input의 키 배열 크기와 같다.
	enum { KeyCount = 256, };

	// 핸들러 목록의 항목.
	struct HandlerEntry
	{
		// 실제로 사용할 참조. 소유는 컴포넌트/위젯 쪽이므로 약참조로 잡는다.
		std::weak_ptr<InputHandler> handler;

		// 목록에서 지울 때 쓰는 식별자.
		//
		// 소멸자에서 Unregister를 부르는 시점에는 weak_ptr을 lock할 수 없어서
		// (이미 참조 카운트가 0) 약참조만으로는 자기 항목을 찾지 못한다.
		// 오직 주소 비교에만 쓰고 절대 역참조하지 않는다.
		InputHandler* rawHandler = nullptr;
	};

public:
	InputSystem();
	~InputSystem();

	// 외부에서 접근이 가능하도록 해주는 함수.
	static InputSystem& Get();

	// 엔진 종료 순서 때문에 시스템이 이미 사라졌을 수 있어서
	// 소멸자 같은 곳에서는 Get 전에 이 함수로 확인해야 한다.
	static bool HasInstance();

	// InputHandler가 스스로 호출한다.
	void RegisterHandler(const std::weak_ptr<InputHandler>& handler);
	void UnregisterHandler(const InputHandler* handler);

	// 우선순위가 바뀌었으니 다음 디스패치 전에 다시 정렬하라는 표시.
	inline void RequestSort() { needsSort = true; }

private:
	// 이번 프레임의 입력을 이벤트로 만들어 핸들러에 전달한다.
	// Engine이 매 프레임 Tick 직전에 호출한다.
	void DispatchInput();

	// 만료된 항목을 정리하고 필요하면 우선순위로 다시 정렬한다.
	void RefreshHandlers();

	// 키 하나의 이벤트 하나를 적절한 핸들러에게 보낸다.
	void RouteEvent(int keyCode, EInputEvent event);

	// 키 소유권을 놓는다. 키를 뗐을 때만 부른다.
	void ReleaseKeyOwnership(int keyCode);

private:
	// 등록된 핸들러 목록. 우선순위 내림차순으로 정렬해서 들고 있다.
	std::vector<HandlerEntry> handlers;

	// 이번 프레임에 처리할 핸들러의 강참조 목록.
	//
	// 콜백 안에서 등록/해제나 액터 파괴가 일어나도 순회가 깨지지 않도록
	// 디스패치 시작 시점에 한 번 잠가서 복사해 둔다.
	// 매 프레임 재할당하지 않으려고 멤버로 둔다.
	std::vector<std::shared_ptr<InputHandler>> dispatchCache;

	// 키 단위 소유권(래치).
	// Pressed를 소비한 핸들러가 그 키를 뗄 때까지 계속 소유한다.
	std::weak_ptr<InputHandler> keyOwners[KeyCount];

	// 이 키에 소유자가 지정된 적이 있는지.
	//
	// keyOwners만으로는 "원래 주인이 없던 키"와 "주인이 죽은 키"를 구분할 수 없다.
	// weak_ptr은 둘 다 lock에 실패하기 때문이다.
	// 이 구분이 필요한 이유는 아래 RouteEvent 주석에 있다.
	bool keyHasOwner[KeyCount] = {};

	// 다음 디스패치 전에 정렬이 필요한지 여부.
	bool needsSort = false;

	// 전역 접근이 가능하도록 변수 추가.
	static InputSystem* instance;
};

NAME_SPACE_END
