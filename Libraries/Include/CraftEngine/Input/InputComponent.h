#pragma once

#include "Utils/EngineMacro.h"
#include "Component/ActorComponent.h"
#include "Input/InputHandler.h"

#include <memory>

NAME_SPACE_BEGIN(Craft)

// 액터가 입력을 받게 해주는 컴포넌트. (언리얼의 UInputComponent 자리다)
//
// 실제 바인딩 테이블과 소비 규칙은 전부 InputHandler가 들고 있고,
// 이 클래스는 그걸 액터 수명에 맞춰 붙였다 떼는 얇은 어댑터다.
//
// 이렇게 나눠 둔 이유:
// InputSystem이 InputComponent를 직접 알면 입력을 받으려면 무조건 Actor여야 한다.
// UI는 Actor가 아닌 편이 자연스러운데(위치/이미지/정렬 순서를 안 쓰고,
// 수명이 Level에 묶이면 곤란하다) 그때 InputHandler만 들면 되도록 해 둔 것이다.
//
// 주의 - 다른 컴포넌트와 마찬가지로 액터 생성자가 아니라 BeginPlay에서 추가할 것.
class CRAFT_API InputComponent : public ActorComponent
{
	TYPE_DECLARATIONS(InputComponent, ActorComponent)

public:
	InputComponent();
	virtual ~InputComponent() = default;

	// InputSystem에 핸들러를 등록한다.
	// 오버라이드할 때는 super::BeginPlay()를 먼저 호출할 것.
	virtual void BeginPlay() override;

	// 키 하나에 실행할 함수를 등록한다(람다 / 자유 함수).
	void BindKey(int keyCode, EInputEvent event,
		InputHandler::InputCallback callback, bool consume = true);

	// 키 하나에 실행할 멤버 함수를 등록한다.
	//
	//   input->BindKey(VK_SPACE, EInputEvent::Pressed, this, &TestActor::OnRoll);
	template<typename T>
	void BindKey(int keyCode, EInputEvent event, T* object, void (T::* method)(), bool consume = true)
	{
		handler->BindKey(keyCode, event, object, method, consume);
	}

	void ClearBindings();

	// getter/setter
	// 우선순위는 InputPriority의 밴드 값을 기준으로 준다.
	inline int GetInputPriority() const { return handler->GetPriority(); }
	inline void SetInputPriority(int newPriority) { handler->SetPriority(newPriority); }

	// 모달처럼 아래 계층을 통째로 막아야 할 때.
	inline void SetBlockAllInput(bool newBlockAllInput) { handler->SetBlockAllInput(newBlockAllInput); }

	// 세밀한 제어가 필요하면 핸들러를 직접 다룬다.
	inline InputHandler& GetHandler() const { return *handler; }

private:
	// 바인딩 테이블. 이 컴포넌트가 소유하므로 컴포넌트보다 오래 살지 않는다.
	std::shared_ptr<InputHandler> handler;
};

NAME_SPACE_END
