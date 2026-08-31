#pragma once

#include "Core/CraftObject.h"
#include "UI/Geometry.h"

#include <memory>
#include <type_traits>

NAME_SPACE_BEGIN(Craft)

namespace UI
{
	class PanelWidget;

	// 모든 UI 요소의 기반 클래스. 언리얼의 UWidget에 해당.
	//
	// 배치와 그리기가 두 단계로 나뉘어 있다(언리얼과 같은 구조).
	//
	//   1) ComputeDesiredSize()  자식 -> 부모 방향. "나는 이만큼이 필요하다"
	//   2) ArrangeChildren()     부모 -> 자식 방향. "너는 여기에 이만큼 써라"
	//   3) OnPaint()             배치가 끝난 영역에 실제로 그린다
	//
	// 크기를 정하는 일과 위치를 정하는 일을 분리해야 하는 이유는,
	// 세로로 쌓는 상자가 자식들의 높이를 다 알아야 자기 높이를 정할 수 있고,
	// 그 뒤에야 각 자식에게 몇 번째 줄인지 알려줄 수 있기 때문이다.
	// 한 번에 하려고 하면 자식을 두 번 방문하거나 순환하게 된다.
	class CRAFT_API Widget : public CraftObject, public std::enable_shared_from_this<Widget>
	{
		TYPE_DECLARATIONS(Widget, CraftObject)

	public:
		Widget() = default;
		virtual ~Widget() = default;

		// 위젯을 만드는 공식 통로.
		//
		// InputHandler::Create()와 같은 자리다. 위젯은 shared_ptr로만 존재해야 한다.
		//  - 부모가 자식을 shared_ptr로 들고, 자식은 부모를 weak_ptr로 본다
		//  - 입력 핸들러가 weak_from_this()로 자기 수명을 알린다
		// 스택이나 unique_ptr로 만들면 두 가지가 다 깨진다.
		template<typename T, typename ...Args>
		static std::shared_ptr<T> Create(Args&& ...args)
		{
			static_assert(std::is_base_of<Widget, T>::value,
				"Create는 Widget 파생 타입에만 쓸 수 있다");

			return std::make_shared<T>(std::forward<Args>(args)...);
		}

		// ---- 레이아웃 ----

		// 이 위젯이 원하는 크기(셀 개수)를 계산해서 캐시에 넣고 돌려준다.
		//
		// const가 아닌 이유 - 계산 결과를 cachedDesiredSize에 기록한다.
		// 패널은 여기서 자식들의 ComputeDesiredSize를 먼저 부른 뒤 자기 크기를 정한다.
		virtual Vector2 ComputeDesiredSize();

		// 마지막으로 계산된 희망 크기.
		inline Vector2 GetDesiredSize() const { return cachedDesiredSize; }

		// 부모가 정해준 영역을 받아 자기 자리를 확정하고, 자식들에게 나눠준다.
		//
		// 자식이 없는 위젯은 재정의할 필요가 없다(기본 구현이 영역만 기억한다).
		virtual void ArrangeChildren(const Geometry& allottedGeometry);

		// 마지막으로 배치된 영역. 그리기와 히트 테스트가 이걸 쓴다.
		inline const Geometry& GetCachedGeometry() const { return cachedGeometry; }

		// 레이아웃을 다시 계산해야 한다고 표시한다. 언리얼의 Invalidate에 해당.
		//
		// 지금은 UISystem이 매 프레임 무조건 두 패스를 돌기 때문에 실제 효과가 없다.
		// 그래도 호출부를 미리 써 두는 이유는, 나중에 더티 플래그를 켤 때
		// "무효화를 빠뜨린 자리"를 찾아다니지 않아도 되게 하기 위해서다.
		// (그 누락은 UMG에서 가장 잡기 어려운 종류의 버그다)
		void Invalidate();

		// ---- 수명 ----

		// 화면에 실제로 올라간 직후에 불린다. 언리얼의 NativeConstruct에 해당.
		//
		// 생성자가 아니라 여기서 입력을 등록해야 하는 이유는 Actor와 같다.
		// 생성자 시점에는 아직 shared_ptr로 감싸지기 전이라 weak_from_this()가 비어 있고,
		// 그러면 입력 시스템에 자기 수명을 알릴 방법이 없다.
		//
		// 재정의할 때는 반드시 super::NativeConstruct()를 부를 것.
		virtual void NativeConstruct();

		// 화면에서 내려가기 직전에 불린다. 언리얼의 NativeDestruct에 해당.
		// 재정의할 때는 반드시 super::NativeDestruct()를 부를 것.
		virtual void NativeDestruct();

		// 지금 화면에 올라가 있는지 여부.
		inline bool IsConstructed() const { return isConstructed; }

		// ---- 갱신 / 그리기 ----

		// 매 프레임 호출. 표시할 값이 바뀌는 위젯(체력바 등)이 재정의한다.
		virtual void Tick(float deltaTime);

		// 자기 자신을 그린다.
		//
		// 반환값은 이 위젯이 실제로 사용한 가장 높은 layerId다(언리얼과 같은 규약).
		// 보통은 받은 값을 그대로 돌려준다. Renderer의 z 테스트가
		// "같으면 나중 것이 위"라서 트리를 깊이 우선으로 그리는 것만으로
		// 부모 -> 자식, 형제는 배열 순서대로 겹치기 때문이다.
		virtual int OnPaint(const Geometry& allottedGeometry, const PaintContext& context) const;

		// ---- 속성 ----

		inline EVisibility GetVisibility() const { return visibility; }
		void SetVisibility(EVisibility newVisibility);

		// 그려지는지 여부.
		inline bool IsVisible() const { return visibility == EVisibility::Visible; }

		// 레이아웃에서 자리를 차지하는지 여부.
		// Collapsed만 자리를 내놓는다. Hidden은 안 보여도 자리는 지킨다.
		inline bool TakesSpace() const { return visibility != EVisibility::Collapsed; }

		inline const Margin& GetPadding() const { return padding; }
		void SetPadding(const Margin& newPadding);

		inline EHorizontalAlignment GetHorizontalAlignment() const { return horizontalAlignment; }
		void SetHorizontalAlignment(EHorizontalAlignment alignment);

		inline EVerticalAlignment GetVerticalAlignment() const { return verticalAlignment; }
		void SetVerticalAlignment(EVerticalAlignment alignment);

		// 부모 패널. 루트 위젯이면 비어 있다.
		std::shared_ptr<PanelWidget> GetParent() const { return parent.lock(); }

		// PanelWidget 전용. 자식으로 편입될 때 불린다.
		void SetParent(const std::weak_ptr<PanelWidget>& newParent) { parent = newParent; }

	protected:
		// 부모를 weak_ptr로 보는 이유는 Actor -> Level과 같다.
		// 부모가 자식을 shared_ptr로 들고 있으므로 여기서도 강참조를 하면 순환이 된다.
		std::weak_ptr<PanelWidget> parent;

		// ArrangeChildren이 기록하는 최종 배치 영역.
		Geometry cachedGeometry;

		// ComputeDesiredSize가 기록하는 희망 크기.
		Vector2 cachedDesiredSize = Vector2::Zero;

		EVisibility visibility = EVisibility::Visible;

		// NativeConstruct가 불렸고 아직 NativeDestruct가 안 불린 상태인지.
		// 같은 위젯을 두 번 올리거나 두 번 내려도 훅이 한 번만 돌게 한다.
		bool isConstructed = false;

		// 위젯 자신의 바깥 여백.
		Margin padding;

		EHorizontalAlignment horizontalAlignment = EHorizontalAlignment::Fill;
		EVerticalAlignment verticalAlignment = EVerticalAlignment::Fill;
	};
}

NAME_SPACE_END
