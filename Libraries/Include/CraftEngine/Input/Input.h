#pragma once

#include <Math/Vector2.h>
#include <Windows.h>

NAME_SPACE_BEGIN(Craft)
class CRAFT_API Input
{
	// Engine 클래스 friend 선언.
	friend class Engine;

	// 관리할 가상 키의 수.
	// keyStates 배열 크기와 한 곳에서 묶어두기 위해 enum으로 선언.
	enum { KeyCount = 256, };

	// 키 입력 상태를 저장하기 위한 구조체.
	//
	// 이전 프레임 상태를 들고 비교하는 방식이 아니라, 이번 프레임에
	// "전이가 있었는지"를 직접 기록한다.
	// ProcessInput이 콘솔 입력 버퍼를 통째로 드레인하기 때문에 한 프레임 안에서
	// 눌렀다 뗀 입력은 isKeyDown이 false로 끝나고, 이전 프레임 비교 방식으로는
	// 눌린 사실 자체가 사라진다(짧게 톡 친 입력이 씹힘).
	struct KeyState
	{
		// 현재 실제 눌림 상태.
		bool isKeyDown = false;

		// 이번 프레임에 눌림 전이가 발생했는지 여부.
		// SavePreviousStates에서 지워지는 프레임 단위 플래그.
		bool pressedThisFrame = false;

		// 이번 프레임에 뗌 전이가 발생했는지 여부.
		bool releasedThisFrame = false;
	};

public:
	Input();
	~Input();

	// 이번 프레임에 눌리기 시작했으면 true 반환.
	bool GetKeyDown(int keyCode) const;

	// 이번 프레임에 떼졌으면 true 반환.
	bool GetKeyUp(int keyCode) const;

	// 눌려 있는 동안 매 프레임 true 반환.
	bool GetKey(int keyCode) const;

	// 이번 프레임에 이 키와 관련된 일이 하나라도 있었는지 여부.
	// InputSystem이 256개 키를 순회할 때 빠르게 건너뛰기 위한 함수.
	bool HasKeyActivity(int keyCode) const;

	// 현재 마우스 포인터의 콘솔 셀 좌표를 반환.
	//
	// 매 프레임 실제 커서 위치를 폴링해서 갱신한다(ProcessInput 참고).
	// 콘솔 마우스 이벤트만 믿으면 이벤트가 오는 순간에만 값이 바뀌어서,
	// 조준처럼 "지금 어디를 가리키나"가 매 프레임 필요한 쪽에서는 값이 띄엄띄엄 는다.
	const Vector2& GetMousePosition() const { return mousePosition; }

	// 화면 격자 크기(셀 개수)를 알려준다. 커서 픽셀 좌표를 셀로 환산할 때 쓴다.
	//
	// Input이 Renderer를 직접 보지 않는 이유 - 둘 다 Engine이 만드는 하위 시스템이고,
	// Input이 먼저 생긴다. 크기를 아는 쪽(Engine)이 알려주는 편이 의존 방향이 깔끔하다.
	// 이 값이 설정되기 전에는 폴링을 건너뛰고 콘솔 이벤트 값만 쓴다.
	void SetScreenCellSize(const Vector2& newScreenCellSize);

	// 외부에서 접근이 가능하도록 해주는 함수.
	static Input& Get();

private:
	// 현재 프레임에 특정 키 입력이 발생했는지를 처리하는 함수.
	void ProcessInput();

	// 프레임 단위 전이 플래그를 정리하는 함수.
	// 프레임의 끝에서 호출되어 다음 프레임의 경계를 만든다.
	void SavePreviousStates();

	// 키 하나의 눌림 상태를 갱신하면서 전이를 기록하는 함수.
	//
	// 콘솔은 키를 누르고 있으면 자동 반복(typematic)으로 같은 눌림 이벤트를
	// 계속 보내고, 마우스는 커서를 움직이기만 해도 버튼 상태를 다시 보낸다.
	// 그래서 상태가 실제로 바뀐 순간에만 플래그를 세워야
	// Pressed 이벤트가 매 프레임 연사되지 않는다.
	void UpdateKeyState(int keyCode, bool isKeyDown);

	// 마우스 버튼 상태를 실제 상태로 맞추는 함수.
	//
	// 콘솔은 더블클릭의 두 번째 클릭을 DOUBLE_CLICK 이벤트 하나로 보내는데,
	// 그에 대응하는 버튼 해제 이벤트가 오지 않는다.
	// 마우스 이벤트는 상태가 바뀔 때만 오기 때문에 보정하지 않으면
	// 커서를 움직여 다음 이벤트가 올 때까지 버튼이 눌린 채로 남는다.
	void ReconcileMouseButtons();

	// 실제 커서 위치를 읽어 콘솔 셀 좌표로 환산하는 함수.
	//
	// 콘솔 입력 버퍼의 MOUSE_EVENT는 "셀이 바뀌었을 때"만 오고, 창 밖으로 나가거나
	// 포커스가 오갈 때는 아예 안 온다. 그래서 이벤트만으로는 커서를 멈춘 사이에
	// 화면이 스크롤되거나 창이 움직인 경우를 따라가지 못한다.
	// 여기서는 창의 클라이언트 영역을 격자로 나눠 직접 계산하므로
	// 폰트 크기나 DPI 배율이 어떻든 같은 결과가 나온다.
	void PollMousePosition();

	// 콘솔 창이 입력 포커스를 가지고 있는지 확인하는 함수.
	//
	// FOCUS_EVENT는 문서상 "내부용이므로 무시하라"고 되어 있어서 신뢰하기 어렵다.
	// 창 핸들을 직접 비교하는 쪽이 확실하고 호출 비용도 작다.
	bool HasConsoleFocus() const;

private:
	// 키 상태를 관리할 배열.
	KeyState keyStates[KeyCount] = { };

	// 콘솔 입력 이벤트를 읽기 위한 핸들.
	HANDLE inputHandle = INVALID_HANDLE_VALUE;

	// 프로그램 시작 시 설정되어 있던 콘솔 입력 모드.
	DWORD originalConsoleMode = 0;

	// 종료할 때 기존 콘솔 입력 모드를 복구할지 여부.
	bool shouldRestoreConsoleMode = false;

	// 현재 마우스 포인터의 콘솔 셀 좌표.
	Vector2 mousePosition = Vector2::Zero;

	// 화면 격자 크기(셀 개수). 0이면 아직 아무도 안 알려준 것이라 폴링하지 않는다.
	Vector2 screenCellSize = Vector2::Zero;

	// 전역 접근이 가능하도록 변수 추가.
	static Input* instance;
};

NAME_SPACE_END
