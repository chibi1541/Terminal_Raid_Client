#pragma once

#include "UI/Widget.h"

#include <vector>

NAME_SPACE_BEGIN(Craft)

namespace UI
{
	// 자식 하나와, 그 자식을 부모 안에 어떻게 놓을지에 대한 정보. 언리얼의 UPanelSlot.
	//
	// 정렬과 여백이 자식이 아니라 슬롯에 있는 이유:
	// 같은 버튼이라도 세로 상자 안에서는 가운데, 격자 안에서는 왼쪽에 놓고 싶을 수 있다.
	// 그건 버튼의 성질이 아니라 "이 부모 안에서의 자리"에 대한 성질이다.
	// 위젯 자신에게 두면 같은 위젯을 다른 곳에 재사용할 때마다 값을 바꿔야 한다.
	//
	// 패널마다 필요한 정보가 달라서(격자는 행/열, 캔버스는 앵커) 파생 슬롯이 생긴다.
	class CRAFT_API PanelSlot
	{
	public:
		PanelSlot() = default;
		virtual ~PanelSlot() = default;

		inline const std::shared_ptr<Widget>& GetContent() const { return content; }

		inline const Margin& GetPadding() const { return padding; }
		inline void SetPadding(const Margin& newPadding) { padding = newPadding; }

		inline EHorizontalAlignment GetHorizontalAlignment() const { return horizontalAlignment; }
		inline void SetHorizontalAlignment(EHorizontalAlignment alignment) { horizontalAlignment = alignment; }

		inline EVerticalAlignment GetVerticalAlignment() const { return verticalAlignment; }
		inline void SetVerticalAlignment(EVerticalAlignment alignment) { verticalAlignment = alignment; }

	protected:
		friend class PanelWidget;

		// 이 슬롯이 담고 있는 위젯.
		// 부모가 자식을 강참조로 들고, 자식은 부모를 weak_ptr로 본다.
		std::shared_ptr<Widget> content;

		// 자식 주위의 여백. 위젯 자신의 padding과 별개로 더해진다.
		Margin padding;

		// 슬롯 안에서의 정렬. 기본은 채우기다.
		EHorizontalAlignment horizontalAlignment = EHorizontalAlignment::Fill;
		EVerticalAlignment verticalAlignment = EVerticalAlignment::Fill;
	};

	// 자식을 여러 개 가질 수 있는 위젯의 공통 기반. 언리얼의 UPanelWidget.
	//
	// 이 클래스 자체는 배치 규칙이 없다. 자식 목록을 관리하고,
	// 그리기를 자식들에게 순서대로 넘기는 일만 한다.
	// 어떻게 놓을지는 파생 클래스(세로 상자, 격자, 캔버스)가 정한다.
	class CRAFT_API PanelWidget : public Widget
	{
		TYPE_DECLARATIONS(PanelWidget, Widget)

	public:
		PanelWidget() = default;
		virtual ~PanelWidget() = default;

		// 복사 금지.
		//
		// 자식 목록이 unique_ptr<PanelSlot>이라 복사할 수 없는데,
		// CRAFT_API(dllexport)가 붙으면 컴파일러가 쓰지도 않는 복사 대입 연산자까지
		// 강제로 만들어 보다가 에러를 낸다. AssetManager가 복사를 지운 것과 같은 이유다.
		//
		// 의미상으로도 위젯을 복사하는 건 말이 안 된다.
		// 자식들의 parent가 원본을 가리킨 채로 남아서 트리가 깨진다.
		PanelWidget(const PanelWidget&) = delete;
		PanelWidget& operator=(const PanelWidget&) = delete;

		// 자식을 추가하고, 배치 정보를 설정할 수 있도록 슬롯을 돌려준다.
		//
		// 반환값의 소유권은 패널에 있다. 호출부는 설정만 하고 보관하지 않는다.
		//   panel->AddChild(label)->SetPadding(Margin(1));
		//
		// 파생 패널은 자기 슬롯 타입을 돌려주는 함수를 따로 둔다
		// (언리얼의 AddChildToVerticalBox 같은 것).
		PanelSlot* AddChild(const std::shared_ptr<Widget>& child);

		// 자식을 떼어낸다. 없으면 아무 일도 하지 않는다.
		bool RemoveChild(const std::shared_ptr<Widget>& child);

		void ClearChildren();

		inline int GetChildCount() const { return static_cast<int>(slots.size()); }

		// 범위를 벗어나면 nullptr.
		std::shared_ptr<Widget> GetChildAt(int index) const;

		// 범위를 벗어나면 nullptr.
		PanelSlot* GetSlotAt(int index) const;

		// 그리기는 여기서 공통으로 처리한다.
		//
		// 자식을 배열 순서대로 그리고, 부모의 영역과 교집합을 낸 클리핑 범위를 넘긴다.
		// Renderer의 z 테스트가 "값이 같으면 나중 것이 위"이므로
		// 이 순서 자체가 곧 겹침 순서다(페인터 알고리즘).
		virtual int OnPaint(const Geometry& allottedGeometry, const PaintContext& context) const override;

		virtual void Tick(float deltaTime) override;

		// 수명 훅을 자식들에게도 전달한다.
		//
		// 트리 안에 들어 있는 UserWidget도 화면에 올라간 시점을 알아야
		// 그때 입력을 등록할 수 있다. 루트만 알면 중첩된 창이 입력을 못 받는다.
		virtual void NativeConstruct() override;
		virtual void NativeDestruct() override;

	protected:
		// 파생 패널이 자기 슬롯 타입을 쓸 수 있도록 생성을 갈라 놓는다.
		// 기본은 PanelSlot을 만든다.
		virtual std::unique_ptr<PanelSlot> CreateSlot();

		// 이 패널이 자식을 몇 개까지 받는지. 0 이하면 제한 없음.
		// 내용 위젯(Border 등)은 1을 돌려준다.
		virtual int GetMaxChildCount() const { return 0; }

	protected:
		// 자식 목록. 배열 순서가 곧 그리기 순서이자 겹침 순서다.
		std::vector<std::unique_ptr<PanelSlot>> slots;
	};
}

NAME_SPACE_END
