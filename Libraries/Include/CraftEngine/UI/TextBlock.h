#pragma once

#include "Math/Color.h"
#include "UI/Widget.h"

#include <optional>
#include <string>

NAME_SPACE_BEGIN(Craft)

namespace UI
{
	// 글자를 표시하는 위젯. 언리얼의 UTextBlock에 해당.
	//
	// 비트맵 폰트가 없다. 콘솔 화면 버퍼가 이미 문자 셀 격자라서
	// 한 글자가 그대로 한 칸에 들어간다(Renderer::Submit이 하는 일이 정확히 그것이다).
	//
	// 다만 스프라이트는 1픽셀이 1셀이라, 8x8 캐릭터가 8칸인 데 반해
	// 글자 하나도 1칸이다. 즉 글자가 캐릭터에 비해 상대적으로 크게 보인다.
	// 콘솔에서 이건 피할 수 없고, UI를 캐릭터보다 크게 잡는 쪽으로 받아들인다.
	class CRAFT_API TextBlock : public Widget
	{
		TYPE_DECLARATIONS(TextBlock, Widget)

	public:
		TextBlock() = default;
		explicit TextBlock(const std::string& text, Color color = Color::White);

		inline const std::string& GetText() const { return text; }
		void SetText(const std::string& newText);

		inline Color GetColor() const { return color; }
		inline void SetColor(Color newColor) { color = newColor; }

		// 글자 뒤에 깔 배경색.
		//
		// 지정하지 않으면 셀의 배경 속성이 검정으로 덮인다.
		// 패널 배경 위에 얹을 때는 반드시 부모와 같은 색을 넣어야
		// 글자가 있는 칸만 검게 파이지 않는다.
		inline const std::optional<Color>& GetBackgroundColor() const { return backgroundColor; }
		inline void SetBackgroundColor(std::optional<Color> newBackgroundColor) { backgroundColor = newBackgroundColor; }

		virtual Vector2 ComputeDesiredSize() override;
		virtual int OnPaint(const Geometry& allottedGeometry, const PaintContext& context) const override;

	private:
		// 표시할 문자열. '\n'으로 여러 줄을 넣을 수 있다.
		std::string text;

		Color color = Color::White;

		std::optional<Color> backgroundColor;
	};
}

NAME_SPACE_END
