#include "pch.h"
#include "Actor/CrosshairActor.h"

#include "Input/Input.h"
#include "Render/Renderer.h"
#include "Render/RenderLayer.h"
#include "Math/Color.h"

#include <string>
#include <unordered_map>

using namespace Craft;

namespace
{
	const std::string kCross = ".O.\nORO\n.O.";

	const std::unordered_map<char, Color>& CrossPalette()
	{
		static const std::unordered_map<char, Color> p = { { 'O', Color::Orange },{ 'R', Color::Red} };
		return p;
	}
}

void CrosshairActor::BeginPlay()
{
	// 화면 고정 오버레이 - 카메라가 원점에서 멀어져도 컬링되지 않도록.
	shouldDraw = true;
	SetUsesDepthSorting(false);

	super::BeginPlay();
}

void CrosshairActor::Draw()
{
	super::Draw();

	const Vector2 cursor = Input::Get().GetMousePosition();

	// 3x3 이므로 커서 셀이 한가운데 오도록 좌상단을 (-1, -1) 옮긴다.
	// 다른 UI(일시정지 패널 등) 위에 오도록 정렬 순서를 높게, FPS(INT_MAX) 아래로.
	Renderer::Get().SubmitPixels(
		kCross, CrossPalette(), Vector2(cursor.x - 1, cursor.y - 1),
		RenderLayer::UI + 500, '.', 1, 1);
}
