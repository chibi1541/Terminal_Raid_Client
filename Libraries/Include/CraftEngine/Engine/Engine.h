#pragma once

#include <functional>
#include <memory>
#include <vector>

NAME_SPACE_BEGIN(Craft)

class Level;
class Input;
class InputSystem;
class Renderer;
class CameraManager;
class AssetManager;
class ThreadManager;
class JobQueue;

namespace UI
{
	class UISystem;
}


// dll에서 외부 모듈로 노출시키겠다는 키워드
// 언리얼의 모듈이름_API 키워드와 비슷
class CRAFT_API Engine
{
	// 엔진 설정 (데이터)
	struct Setting
	{
		enum { BUFFER_SIZE = 2048, };

		// 목표 프레임 수
		float framerate = 0.f;

		// 사용할 콘솔 화면 너비
		int width = 0;

		// 사용한 콘솔 화면 높이
		int height = 0;
	};

public:
	Engine();
	virtual ~Engine();

	// 엔진 실행 함수
	void Run();

	// 엔진 종료 함수
	void Quit();

	// 레벨 추가 요청 함수
	template<typename T, typename = std::enable_if_t<std::is_base_of<Level, T>:: value>>
	void AddNewLevel()
	{
		nextLevel = std::make_shared<T>();
	}

	// 전역 인스턴스 반환
	// Engine::Get(); -> Engine의 instance를 반환
	static Engine& Get();

	// 실제로 잡힌 화면 크기(콘솔 셀 개수).
	//
	// Setting.txt의 값을 그대로 돌려주면 안 된다.
	// 콘솔은 화면 해상도/폰트에 따라 요청한 크기를 못 잡아주고,
	// ScreenBuffer가 GetLargestConsoleWindowSize로 줄여서 잡는다.
	// 설정값을 믿고 UI를 배치하면 화면 밖에 그리게 되고,
	// 마우스 좌표(실제 셀 좌표)와도 좌표계가 어긋난다.
	//
	// renderer가 만들어지기 전(엔진 생성 도중)에는 설정값으로 답한다.
	int GetWidth() const;
	int GetHeight() const;

	// 현재 프레임 수
	inline float GetFps() const { return currentFps; }

	// 화면에 프레임 수를 표시할지 여부
	inline void SetShowFps(bool show) { showFps = show; }

	std::shared_ptr<Level> GetLevel();

	AssetManager* GetAssetManager() const { return assetManager.get(); }
	ThreadManager* GetThreadManager() const { return threadManager.get(); }

	// 다른 쓰레드가 게임(메인) 쓰레드에서 실행되기를 원하는 작업을 밀어 넣는다.
	//
	// 어느 쓰레드에서 불러도 안전하다. 밀어 넣기만 하고 바로 돌아온다.
	// 실제 실행 지점은 Run() 루프 안의 딱 한 곳(PumpGameThreadJobs)뿐이다.
	//
	// 네트워크 쓰레드에서 도착한 패킷을 게임에 반영하는 통로가 이것이다.
	// 패킷 핸들러가 액터나 UI를 직접 만지면 메인 쓰레드의 Tick/Draw와 그대로 경합한다.
	//
	// 정의가 .cpp에 있어서 JobQueue 타입이 이 헤더로 새지 않는다.
	// (헤더에서 JobQueue::DoAsync가 인스턴스화되면 ObjectPool<Job>의 정적 멤버를
	//  다시 정의하려 들어 링크가 깨진다 - AssetManager.h의 같은 주석 참고)
	void RunOnGameThread(std::function<void()> job);

	// 엔진 종료 시, 워커 쓰레드를 Join()하기 직전에 호출될 콜백을 등록한다.
	//
	// 엔진이 만들지 않은 쓰레드(예: ThreadManager::Launch로 띄운 네트워크 쓰레드)에
	// 정지 신호를 보내는 자리다. 이게 없으면 무한 루프인 그 쓰레드를 기다리며
	// Join()이 영영 돌아오지 않는다 - 게임이 종료되지 않는다.
	void AddShutdownHandler(std::function<void()> handler);

protected:
	// 입력 처리(폴링 방식 vs 이벤트)
	void ProcessInput();

	// 이번 프레임의 입력을 이벤트로 만들어 InputComponent들에게 전달.
	//
	// BeginPlay 뒤에 두는 이유 - 이번 프레임에 등록된 컴포넌트도 바로 입력을 받는다.
	// Tick 앞에 두는 이유 - 콜백이 세운 값을 같은 프레임의 Tick이 읽는다(한 프레임도 안 밀림).
	void DispatchInput();

	// 다른 쓰레드가 RunOnGameThread로 밀어 넣은 작업을 여기서 전부 소비한다.
	// 게임 쓰레드에서 실행되는 유일한 소비 지점이다.
	void PumpGameThreadJobs();

	// 초기화 함수.
	void OnInitialized();


	// 게임 플레이 이벤트 함수
	// 게임 플레이 초기화
	void BeginPlay();

	// 게임 플레이 업데이트
	void Tick(float deltaTime);

	// 레벨 그리기 함수
	void Draw();

	// 프레임 간 입력 값 저장을 위함 함수(입력의 변화를 체크하기 위함)
	void SavePreviousInputState();

	// 프레임 수 측정 함수
	void UpdateFps(float deltaTime);

	// 엔진 종료 시 리소스 정리
	void Shutdown();

	// 엔진 설정 로드 함수
	void LoadEngineSetting();

protected:
	bool isQuit = false;

	Setting setting;

	// 전역으로 접근이 가능하도록 변수 선언
	// Engine::instance
	static Engine* instance;

	// 메인 레벨
	std::shared_ptr<Level> mainLevel;

	// 추가 요청된 레벨
	std::shared_ptr<Level> nextLevel;

	// 입력 시스템 변수
	std::unique_ptr<Input> input;

	// 입력 이벤트를 InputComponent들에게 전달하는 디스패처.
	// Input의 상태를 읽으므로 input보다 나중에 만들고 먼저 정리한다.
	std::unique_ptr<InputSystem> inputSystem;

	std::unique_ptr<Renderer> renderer;

	// 활성 카메라를 추적하고 매 프레임 뷰 원점을 계산한다.
	//
	// renderer 다음에 두는 이유가 앞뒤로 하나씩 있다.
	//  - 생성: renderer 다음이어야 실제로 잡힌 화면 크기(뷰 크기)를 알 수 있다.
	//  - 파괴: uiSystem보다 나중에 죽어야, 먼저 죽는 uiSystem이 카메라를 안전하게 참조한다.
	std::unique_ptr<CameraManager> cameraManager;

	// 화면에 올라간 위젯들의 갱신/배치/그리기를 담당.
	//
	// renderer 다음에 두는 이유가 앞뒤로 하나씩 있다.
	//  - 생성: renderer 다음이어야 실제로 잡힌 화면 크기를 알 수 있다.
	//          inputSystem 다음이기도 해야 위젯이 입력 핸들러를 등록할 수 있다.
	//  - 파괴: renderer/inputSystem보다 먼저 죽어야
	//          위젯이 이미 사라진 렌더러나 입력 시스템을 건드리지 않는다.
	std::unique_ptr<UI::UISystem> uiSystem;

	// 애셋 로드/캐싱/유휴 언로드를 담당
	std::unique_ptr<AssetManager> assetManager;

	// 애셋 파싱용 워커 쓰레드를 들고 있다.
	// assetManager보다 먼저 정리돼야 워커가 죽은 매니저를 만지지 않는다.
	std::unique_ptr<ThreadManager> threadManager;

	// 다른 쓰레드 -> 게임 쓰레드로 넘어오는 작업 큐.
	//
	// 어느 워커보다도 먼저 만들어져야 한다. 워커가 뜬 뒤에 만들면
	// 첫 잡이 아직 없는 큐로 들어온다.
	// (JobQueue가 enable_shared_from_this를 상속해서 shared_ptr이어야 한다)
	std::shared_ptr<JobQueue> gameThreadQueue;

	// Shutdown()에서 Join() 직전에 순서대로 호출된다.
	std::vector<std::function<void()>> shutdownHandlers;

	// 화면에 프레임 수를 표시할지 여부
	bool showFps = true;

	// 프레임 수 측정용 누적값.
	// 매 프레임 값을 그대로 쓰면 숫자가 심하게 흔들려서 읽기 어려우므로
	// 일정 시간 동안 모아서 평균을 낸다.
	float fpsElapsedTime = 0.0f;
	int fpsFrameCount = 0;
	float currentFps = 0.0f;

private:

};


NAME_SPACE_END
