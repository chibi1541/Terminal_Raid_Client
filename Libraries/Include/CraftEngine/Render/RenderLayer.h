#pragma once

NAME_SPACE_BEGIN(Craft)

// 그리기 정렬 순서(sortingOrder)의 대역.
//
// Input의 InputPriority와 같은 이유로 한 곳에 모은다.
// 어떤 값이 어떤 계층인지가 코드 여기저기 흩어지면
// "체력바가 왜 인벤토리 창 위에 뜨지"를 추적할 수 없게 된다.
//
// enum class가 아니라 constexpr int인 것도 같은 이유다.
// sortingOrder는 크기 비교로 쓰이고, 대역 사이에 끼워 넣어야 할 때가 있다.
//   RenderLayer::UI + 10   // 같은 창 안에서 위에 겹칠 것
//
// Renderer의 z 테스트는 `기존값 > 새값`이면 건너뛴다.
// 즉 값이 같으면 나중에 제출한 것이 위에 그려진다(페인터 알고리즘).
// 그래서 UI 트리는 대역 값 하나를 공유하고 제출 순서로 앞뒤를 정한다.
namespace RenderLayer
{
	// 월드 배경(타일맵, 지형). 액터(기본 0)보다 아래다.
	//
	// -1이 하한이다. Renderer의 Frame::Clear가 sortingOrderArray를 -1로 채우고
	// z 테스트가 `기존값 > 새값`이라, -2 이하로 제출한 명령은 절대 그려지지 않는다.
	// 즉 이 값은 "가장 아래 레이어"이자 동시에 "아무것도 안 그려짐" 센티넬이다.
	//
	// 배경 아래에 레이어를 하나 더 끼워야 한다면(패럴랙스, 하늘),
	// 이 값을 낮추는 게 아니라 Frame::Clear의 초기값을 INT_MIN으로 내려야 한다.
	constexpr int Background = -1;

	// 월드 액터의 위치 기반 깊이 정렬 기준값.
	//
	// 실제 값은 이 값 + "화면 세로 좌표"다. 아래에 있는 액터일수록 커져서 앞에 그려진다.
	// Level::Draw가 매 프레임 덮어쓴다(UsesDepthSorting이 켜진 액터에 한해).
	//
	// 기준값이 필요한 이유 - 카메라가 180/270°면 깊이 키가 음수가 된다.
	// 0 근처에 두면 Background(-1)를 파고들어 액터가 통째로 사라진다.
	// 월드 좌표는 수천 단위라 이 여유(100,000)로 WorldUI(500,000)까지 닿지 않는다.
	constexpr int WorldActorDepth = 100'000;

	// 액터에 붙어서 따라다니는 위젯(머리 위 체력바 등).
	// 액터보다는 위, 뷰포트 UI보다는 아래.
	constexpr int WorldUI = 500'000;

	// 화면에 고정된 UI(메뉴, 인벤토리, 상태창).
	// 월드의 무엇에도 가려지지 않는다.
	constexpr int UI = 1'000'000;

	// FPS 표시는 Engine이 INT_MAX로 제출하므로 항상 이 위다.
}

NAME_SPACE_END
