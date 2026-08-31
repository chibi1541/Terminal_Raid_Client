#pragma once

#include "Input/InputHandler.h"
#include "UI/PanelWidget.h"

NAME_SPACE_BEGIN(Craft)

namespace UI
{
	// 하나의 "화면"을 이루는 위젯. 언리얼의 UUserWidget에 해당.
	//
	// 메뉴, 인벤토리, 상태창처럼 게임이 열고 닫는 단위가 이것이다.
	// 안에 위젯 트리를 하나 담고(SetRootWidget), 입력 핸들러를 하나 소유한다.
	//
	// 입력을 위젯이 직접 들 수 있는 이유는 InputHandler가 Actor를 모르기 때문이다.
	// InputComponent가 액터를 위해 하는 일을 여기서는 위젯을 위해 그대로 한다.
	// (InputHandler.h의 주석이 정확히 이 구조를 예정하고 있다)
	class CRAFT_API UserWidget : public PanelWidget
	{
		TYPE_DECLARATIONS(UserWidget, PanelWidget)

	public:
		UserWidget();
		virtual ~UserWidget();

		// 이 화면이 담을 위젯 트리의 루트.
		void SetRootWidget(const std::shared_ptr<Widget>& root);
		std::shared_ptr<Widget> GetRootWidget() const { return GetChildAt(0); }

		// ---- 입력 ----

		// 키에 함수를 건다. InputComponent와 같은 형태다.
		void BindKey(int keyCode, EInputEvent event, InputHandler::InputCallback callback, bool consume = true);

		template<typename T>
		void BindKey(int keyCode, EInputEvent event, T* object, void (T::* method)(), bool consume = true)
		{
			BindKey(keyCode, event, [object, method]() { (object->*method)(); }, consume);
		}

		void ClearBindings();

		int GetInputPriority() const;
		void SetInputPriority(int priority);

		// 모달. 바인딩이 없는 키까지 전부 삼켜서 아래 계층으로 안 내려보낸다.
		// 인벤토리를 열어둔 동안 캐릭터가 안 움직이게 하는 것이 이 값이다.
		void SetBlockAllInput(bool block);
		bool IsBlockAllInput() const;

		InputHandler& GetHandler() { return *handler; }

		// ---- 수명 ----

		// 화면에 올라간 시점에 입력을 등록한다.
		// 생성자에서 하면 안 되는 이유는 Widget.h의 주석에 있다.
		virtual void NativeConstruct() override;
		virtual void NativeDestruct() override;

		virtual Vector2 ComputeDesiredSize() override;
		virtual void ArrangeChildren(const Geometry& allottedGeometry) override;

	protected:
		// 화면 하나에 루트 트리는 하나다.
		virtual int GetMaxChildCount() const override { return 1; }

	private:
		// 이 화면의 입력 바인딩 표.
		//
		// InputSystem은 이것만 알고 위젯은 모른다.
		// 살아있는지 여부는 SetEnabledCheck로 알려준다.
		std::shared_ptr<InputHandler> handler;
	};
}

NAME_SPACE_END
