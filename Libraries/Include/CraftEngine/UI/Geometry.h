#pragma once

#include "Math/Rect.h"

NAME_SPACE_BEGIN(Craft)

// UI 위젯 계층.
//
// Craft 바로 아래가 아니라 UI 안에 넣는 이유:
// Image, Border, Overlay, Button 같은 이름은 게임 코드 어디서나 쓰이는 흔한 단어다.
// 실제로 Craft::Image(Asset/Image.h)가 이미 있어서 UI의 Image와 부딪힌다.
// XmlParser.h와 FileUtils.h가 전역에 using namespace std를 하고 있어
// PCH를 통해 std 이름까지 전역으로 새는 상황이라 충돌 위험이 평소보다 크다.
namespace UI
{
	// 위젯 바깥 여백. 언리얼의 FMargin에 해당.
	//
	// 셀 격자 위의 값이라 전부 정수다. 실수로 계산하다 마지막에 반올림하면
	// 값이 조금만 흔들려도 위젯이 한 칸씩 튀어서 눈에 거슬린다.
	struct Margin
	{
		Margin() = default;

		// 네 방향 같은 값.
		explicit Margin(int all)
			: left(all), top(all), right(all), bottom(all)
		{
		}

		// 가로 / 세로.
		Margin(int horizontal, int vertical)
			: left(horizontal), top(vertical), right(horizontal), bottom(vertical)
		{
		}

		Margin(int left, int top, int right, int bottom)
			: left(left), top(top), right(right), bottom(bottom)
		{
		}

		inline int GetTotalWidth() const { return left + right; }
		inline int GetTotalHeight() const { return top + bottom; }

		int left = 0;
		int top = 0;
		int right = 0;
		int bottom = 0;
	};

	// 가로 정렬. 언리얼의 EHorizontalAlignment.
	enum class EHorizontalAlignment : uint8
	{
		// 주어진 폭을 전부 채운다.
		Fill,

		Left,
		Center,
		Right,
	};

	// 세로 정렬. 언리얼의 EVerticalAlignment.
	enum class EVerticalAlignment : uint8
	{
		// 주어진 높이를 전부 채운다.
		Fill,

		Top,
		Center,
		Bottom,
	};

	// 위젯이 보이는지 / 자리를 차지하는지. 언리얼의 ESlateVisibility 축소판.
	enum class EVisibility : uint8
	{
		// 보이고 자리도 차지한다.
		Visible,

		// 안 보이지만 자리는 차지한다(레이아웃이 안 흔들림).
		Hidden,

		// 안 보이고 자리도 차지하지 않는다(없는 것처럼 밀려서 배치됨).
		Collapsed,
	};

	// 위젯에게 배정된 화면상의 영역. 언리얼의 FGeometry.
	//
	// 언리얼과 달리 스케일/회전이 없다. 콘솔은 셀 격자라 변환이 존재할 수 없고,
	// 절대 좌표 사각형 하나면 배치와 히트 테스트에 충분하다.
	struct Geometry
	{
		Geometry() = default;
		explicit Geometry(const Rect& absoluteRect)
			: absoluteRect(absoluteRect)
		{
		}

		// 화면 좌상단 기준 절대 좌표.
		Rect absoluteRect;
	};

	// 한 번의 그리기 순회 동안 들고 다니는 상태. 언리얼의 FPaintContext.
	struct PaintContext
	{
		PaintContext() = default;
		PaintContext(const Rect& cullingRect, int layerId)
			: cullingRect(cullingRect), layerId(layerId)
		{
		}

		// 이 위젯이 그릴 수 있는 최대 영역.
		//
		// 부모가 자기 영역과 교집합을 내서 자식에게 넘긴다.
		// 위젯은 이 값을 Renderer의 clipRect로 그대로 넘기면 되고,
		// 문자열을 직접 잘라낼 필요가 없다.
		Rect cullingRect;

		// 그리기 정렬 순서(Renderer의 sortingOrder).
		//
		// Renderer의 z 테스트가 "값이 같으면 나중에 제출한 쪽이 위"이므로,
		// 트리를 깊이 우선으로 순회하며 제출하면 부모 -> 자식, 형제는 배열 순서로
		// 자연스럽게 겹친다. 그래서 보통은 트리 전체가 같은 값을 쓴다.
		// 값을 올리는 건 툴팁/드롭다운처럼 제출 순서를 일부러 깨야 할 때뿐이다.
		int layerId = 0;
	};

	// 주어진 영역 안에 desiredSize 크기의 내용을 정렬해서 놓을 자리를 구한다.
	//
	// 가운데 정렬에서 남는 칸이 홀수일 때 어느 쪽으로 몰지(여기서는 내림)를
	// 이 함수 하나에만 두는 것이 핵심이다. 위젯마다 각자 계산하면
	// 어떤 창은 왼쪽으로, 어떤 창은 오른쪽으로 1칸씩 치우쳐서 원인을 못 찾게 된다.
	inline Rect AlignInRect(
		const Rect& availableRect,
		const Vector2& desiredSize,
		EHorizontalAlignment horizontalAlignment,
		EVerticalAlignment verticalAlignment,
		const Margin& padding = Margin())
	{
		// 패딩을 뺀 나머지가 내용이 놓일 수 있는 영역이다.
		const int availableWidth = availableRect.size.x - padding.GetTotalWidth();
		const int availableHeight = availableRect.size.y - padding.GetTotalHeight();

		const int left = availableRect.position.x + padding.left;
		const int top = availableRect.position.y + padding.top;

		// 패딩이 영역보다 크면 놓을 자리가 없다.
		if (availableWidth <= 0 || availableHeight <= 0)
		{
			return Rect(Vector2(left, top), Vector2::Zero);
		}

		int width = desiredSize.x;
		int height = desiredSize.y;
		int x = left;
		int y = top;

		switch (horizontalAlignment)
		{
		case EHorizontalAlignment::Fill:
			width = availableWidth;
			break;

		case EHorizontalAlignment::Left:
			break;

		case EHorizontalAlignment::Center:
			x = left + ((availableWidth - width) / 2);
			break;

		case EHorizontalAlignment::Right:
			x = left + (availableWidth - width);
			break;
		}

		switch (verticalAlignment)
		{
		case EVerticalAlignment::Fill:
			height = availableHeight;
			break;

		case EVerticalAlignment::Top:
			break;

		case EVerticalAlignment::Center:
			y = top + ((availableHeight - height) / 2);
			break;

		case EVerticalAlignment::Bottom:
			y = top + (availableHeight - height);
			break;
		}

		// 내용이 영역보다 크면 넘치는 만큼은 잘라낸다.
		// 여기서 안 자르면 위젯이 부모 밖으로 삐져나간 채로 배치된다.
		if (width > availableWidth)
		{
			width = availableWidth;
		}

		if (height > availableHeight)
		{
			height = availableHeight;
		}

		return Rect(Vector2(x, y), Vector2(width, height));
	}
}

NAME_SPACE_END
