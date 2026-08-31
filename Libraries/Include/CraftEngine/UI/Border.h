#pragma once

#include "Math/Color.h"
#include "UI/PanelWidget.h"

#include <optional>

NAME_SPACE_BEGIN(Craft)

namespace UI
{
	// 배경과 테두리를 그리고 자식 하나를 감싸는 위젯. 언리얼의 UBorder에 해당.
	//
	// 창(인벤토리, 상태창, 메뉴)의 바탕이 되는 위젯이다.
	//
	// 언리얼은 배경을 FSlateBrush로 그리고 테두리가 안 늘어나도록 9분할을 쓰지만,
	// 여기서는 그럴 필요가 없다. 콘솔은 셀 격자라 배경이 곧 색깔 사각형이고
	// 테두리는 글자 한 줄이다. 늘려도 깨질 것이 없다.
	class CRAFT_API Border : public PanelWidget
	{
		TYPE_DECLARATIONS(Border, PanelWidget)

	public:
		Border() = default;

		// 자식은 하나만 받는다. 여러 개를 넣고 싶으면 안에 상자 위젯을 둔다.
		void SetContent(const std::shared_ptr<Widget>& content);
		std::shared_ptr<Widget> GetContent() const { return GetChildAt(0); }

		// 배경색. 지정하지 않으면 배경을 칠하지 않는다(뒤가 그대로 비침).
		inline const std::optional<Color>& GetBackgroundColor() const { return backgroundColor; }
		void SetBackgroundColor(std::optional<Color> newBackgroundColor);

		// 테두리를 그릴지 여부와 그 색.
		//
		// 테두리 문자는 ASCII(+ - |)만 쓴다.
		// ScreenBuffer가 WriteConsoleOutputA로 그리기 때문에 CP437 박스 문자는
		// 시스템 코드 페이지에 따라 다른 글자로 나올 수 있다.
		inline bool IsShowingBorder() const { return showBorder; }
		void SetShowBorder(bool show);

		inline Color GetBorderColor() const { return borderColor; }
		inline void SetBorderColor(Color newBorderColor) { borderColor = newBorderColor; }

		virtual Vector2 ComputeDesiredSize() override;
		virtual void ArrangeChildren(const Geometry& allottedGeometry) override;
		virtual int OnPaint(const Geometry& allottedGeometry, const PaintContext& context) const override;

	protected:
		// 내용 위젯이므로 자식은 하나뿐이다.
		virtual int GetMaxChildCount() const override { return 1; }

	private:
		// 테두리가 안쪽에서 잡아먹는 칸 수(테두리를 그리면 사방 1칸).
		inline int GetBorderThickness() const { return showBorder ? 1 : 0; }

		// 배경과 테두리를 그린다.
		void PaintBackground(const Rect& rect, const Rect& clipRect, int layerId) const;

	private:
		std::optional<Color> backgroundColor;

		bool showBorder = false;
		Color borderColor = Color::White;
	};
}

NAME_SPACE_END
