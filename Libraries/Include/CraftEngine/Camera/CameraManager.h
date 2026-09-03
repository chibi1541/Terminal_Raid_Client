#pragma once

#include "Utils/EngineMacro.h"
#include "Math/Vector2.h"
#include "Math/Rect.h"

#include <memory>
#include <vector>

NAME_SPACE_BEGIN(Craft)

class CameraComponent;

// 활성 카메라를 추적하고 매 프레임 뷰 중심/회전을 계산하는 싱글턴.
// 언리얼의 APlayerCameraManager 자리다.
//
// Engine이 unique_ptr로 소유하며, 선언은 renderer 뒤 / uiSystem 앞이다.
//  - 생성: 화면 크기를 알아야 하므로 renderer 다음.
//  - 파괴: 역순이라 uiSystem이 먼저 죽어 카메라를 안전하게 참조하고 내려간다.
//
// 뷰 중심은 정수 Vector2다(lag가 없으면 float은 반올림 버그 표면만 늘린다).
// 회전은 정지 상태에서 90° 단위 정수이고, 보간 전환 중에만 float 각도를 쓴다.
class CRAFT_API CameraManager
{
	friend class Engine;

public:
	explicit CameraManager(const Vector2& viewSize);
	~CameraManager();

	// 싱글턴이므로 복사 금지.
	CameraManager(const CameraManager&) = delete;
	CameraManager& operator=(const CameraManager&) = delete;

	// 전역 접근.
	static CameraManager& Get();

	// 엔진 종료 순서 때문에 매니저가 이미 사라졌을 수 있다.
	// CameraComponent 소멸자 등에서는 Get 전에 이걸로 확인해야 한다.
	static bool HasInstance();

	// CameraComponent가 스스로 호출한다.
	void RegisterCamera(const std::weak_ptr<CameraComponent>& camera);
	void UnregisterCamera(const CameraComponent* camera);

	// 활성 카메라 교체. 언리얼 SetViewTarget 대응.
	void SetActiveCamera(const std::shared_ptr<CameraComponent>& camera);
	std::shared_ptr<CameraComponent> GetActiveCamera() const { return activeCamera.lock(); }

	// 매 프레임 뷰 원점을 갱신한다. Engine이 mainLevel->Tick 뒤, uiSystem->Tick 앞에서 부른다.
	void Tick(float deltaTime);

	// 활성 카메라가 요청하는 회전. 언리얼 SetControlRotation(yaw, 90° 스냅) 대응.
	// blendTime > 0 이면 그 시간 동안 선형 보간, 0이면 즉시 스냅.
	void BlendViewRotationTo(int quarterTurns, float blendTime);

	// 레벨 교체 시 등록 목록과 활성 카메라를 비운다.
	// 뷰(중심/각도/보간)는 유지한다 - 되돌리면 레벨 교체마다 화면이 튄다.
	void Reset();

	Vector2 GetViewCenterWorld() const { return viewCenterWorld; }
	Vector2 GetViewSize() const { return viewSize; }

	// 목표(정지) 회전. 보간이 끝나면 이 값이 실제 각도가 된다.
	int GetViewQuarterTurns() const { return targetQuarterTurns; }

	// 현재 각도(도). 보간 중에는 임의값, 정지 시 targetQuarterTurns*90.
	float GetViewAngleDegrees() const { return viewAngleDegrees; }

	// 보간 전환이 진행 중인지. Engine::Draw가 정수/float 경로를 고를 때 쓴다.
	bool IsRotationBlending() const { return blendDuration > 0.0f; }

	// 뷰 회전이 "정착한" 횟수. 보간이 끝나거나 즉시 스냅될 때마다 늘어난다.
	//
	// 회전 완료를 알리는 채널이다. 델리게이트 등록 목록을 쓰지 않는 이유 -
	// 액터와 레벨은 수시로 죽는데 콜백 목록은 그때마다 해제 누락 위험이 있다.
	// 정수 하나를 비교하는 쪽은 수명 문제가 원천적으로 없고,
	// 레벨이 보간 도중에 생성돼도 다음 비교에서 자동으로 맞춰진다.
	//
	// 1부터 시작한다. 보는 쪽이 0으로 시작하면 첫 프레임에 반드시 한 번 받는다.
	int GetViewRotationVersion() const { return viewRotationVersion; }

	// 뷰가 월드에서 차지하는 영역을 margin 칸만큼 넓힌 것. 액터 컬링용이다.
	//
	// 회전 상태(보간 포함)의 실제 AABB를 쓰므로 90°에서 가로/세로가 바뀐 것도 반영된다.
	// margin이 필요한 이유 - 경계를 화면에 딱 맞추면 폭이 넓은 오브젝트가
	// 기준점이 들어오는 순간에야 그려져서 화면 가장자리에서 튀어나오듯 나타난다.
	Rect GetViewWorldBounds(int margin) const;

	// 뷰 좌상단의 월드 좌표(디버그용, 회전 없을 때만 의미).
	Vector2 GetViewOrigin() const { return viewCenterWorld - Vector2(viewSize.x / 2, viewSize.y / 2); }

	// 좌표 변환. 월드 부착 위젯(체력바 등)이 쓴다.
	// 보간 중이면 float 경로. Tick 중 호출은 한 프레임 stale(뷰는 Draw 진입부에서 확정).
	Vector2 WorldToScreen(const Vector2& world) const;
	Vector2 ScreenToWorld(const Vector2& screen) const;

private:
	// 만료된 weak_ptr 항목을 목록에서 제거한다.
	void PruneExpired();

	// 카메라가 살아있고 활성이며 오너 액터도 활성인지.
	// InputComponent::BeginPlay의 SetEnabledCheck 람다와 동일한 검사다.
	static bool IsCameraUsable(const std::shared_ptr<CameraComponent>& camera);

	// 활성 카메라가 무효화됐을 때, 남은 카메라 중 autoActivate가 켜진 것을
	// 가장 최근 등록 순으로 승격한다. 없으면 활성 카메라 없음(동결).
	void PromoteFallbackCamera();

	// 원하는 뷰 중심을 레벨 경계 안으로 클램프한다. 경계가 비어 있으면 그대로 통과.
	//
	// 회전된 뷰가 월드에서 차지하는 AABB 기준으로 클램프한다.
	//  - 정지: k 짝수면 viewSize, 홀수면 (h,w) 스왑 (정수)
	//  - 보간 중: 현재 각도의 실제 AABB = ceil(w|cos|+h|sin|), ceil(w|sin|+h|cos|)
	Vector2 ClampViewCenter(const Vector2& desiredCenter) const;

	// 회전된 뷰의 월드 AABB 크기(위 규칙).
	Vector2 GetRotatedViewExtent() const;

	// 한 축의 클램프. (top-left 기준 - AABB 좌상단에 적용)
	//   viewExtent >= boundsExtent -> 중앙 정렬 고정(분기하지 않으면 구간이 역전된다)
	//   그 외                       -> [boundsMin, boundsMin + boundsExtent - viewExtent] 로 클램프
	// maxOrigin의 +1(= boundsExtent - viewExtent)을 잊으면 끝 열이 영영 안 보인다.
	static int ClampAxis(int origin, int viewExtent, int boundsMin, int boundsExtent);

	// 현재 프레임에 활성 승계가 일어났는지 검사하고, 그렇다면 새 카메라의
	// 목표 회전으로 즉시(blendTime 0) 맞춘다.
	void SyncRotationToActiveCamera(const std::shared_ptr<CameraComponent>& active);

private:
	static CameraManager* instance;

	// 등록된 카메라들. 등록 순서를 유지한다(승계가 최근 등록 우선이라).
	std::vector<std::weak_ptr<CameraComponent>> cameras;

	// 현재 화면을 비추는 카메라.
	std::weak_ptr<CameraComponent> activeCamera;

	// 뷰 크기(콘솔 셀 개수). Renderer가 실제로 잡은 화면 크기.
	Vector2 viewSize;

	// 뷰 중심의 월드 좌표. 카메라가 없으면 갱신을 건너뛴다(마지막 값 유지 = 동결).
	// 기본값은 생성자에서 viewSize/2로 잡아 ViewWorldToScreen(world)==world 를 만든다.
	Vector2 viewCenterWorld = Vector2::Zero;

	// ---- 회전 보간 상태 ----
	// 현재 각도(도). 정지 시 targetQuarterTurns*90, 보간 중에는 임의값.
	float viewAngleDegrees = 0.0f;

	// 목표(정지) 회전.
	int targetQuarterTurns = 0;

	// 회전이 정착한 횟수. 보간 완료와 즉시 스냅 양쪽에서 늘어난다.
	// 1부터 시작해서 보는 쪽(0으로 시작)이 첫 프레임에 반드시 한 번 받게 한다.
	int viewRotationVersion = 1;

	// 선형 보간 파라미터.
	float blendStartAngle = 0.0f;   // 보간 시작 각도(도)
	float blendDelta = 0.0f;        // 최단 부호부 델타 (-180, 180]
	float blendElapsed = 0.0f;
	float blendDuration = 0.0f;     // 0 = 보간 아님

	// 마지막으로 회전을 동기화한 활성 카메라(주소 비교 전용, 역참조 금지).
	// 이 값과 달라지면 카메라 전환/승계로 보고 새 각도로 즉시 맞춘다.
	const CameraComponent* lastSyncedActive = nullptr;
};

NAME_SPACE_END
