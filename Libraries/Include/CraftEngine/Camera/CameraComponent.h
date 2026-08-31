#pragma once

#include "Utils/EngineMacro.h"
#include "Component/ActorComponent.h"
#include "Math/Vector2.h"

#include <memory>

NAME_SPACE_BEGIN(Craft)

// 액터에 붙여서 "화면을 어디서 비출지"를 결정하는 컴포넌트. (언리얼의 UCameraComponent)
//
// 렌더링 기준을 이 컴포넌트로 옮긴다. 활성 카메라의 뷰 중심이 화면 가운데에 오도록
// CameraManager가 매 프레임 뷰 원점을 계산하고, Renderer가 그 원점을 빼서 그린다.
//
// enable_shared_from_this를 여기서 직접 상속하는 것이 핵심이다.
// ActorComponent에는 없고(부모 계층에 중복이 없어 안전), Actor::AddComponent가
// make_shared<T>로 만들기 때문에 concrete 타입에 붙이면 weak_from_this가 정상 동작한다.
// (InputComponent가 이 문제를 안 겪는 이유는 등록되는 게 컴포넌트가 아니라 InputHandler라서다)
class CRAFT_API CameraComponent
	: public ActorComponent
	, public std::enable_shared_from_this<CameraComponent>
{
	TYPE_DECLARATIONS(CameraComponent, ActorComponent)

public:
	CameraComponent() = default;
	virtual ~CameraComponent();

	// 생성자 금지 규약을 따라 BeginPlay에서 매니저에 등록한다.
	virtual void BeginPlay() override;

	// 뷰 중심이 놓일 월드 좌표 = 오너 액터 위치 + offset.
	Vector2 GetViewCenter() const;

	// 뷰 회전(90° 단위). 콘솔은 셀 격자라 정지 각도는 항상 k*90°다.
	//
	// blendTime > 0 이면 그 시간 동안 선형 보간해서 돈다(연출). 0이면 즉시 스냅.
	// 이 컴포넌트가 현재 활성 카메라면 매니저에 바로 반영되고,
	// 비활성이면 값만 저장됐다가 활성화될 때 매니저가 읽는다.
	void SetViewQuarterTurns(int turns, float blendTime = 0.0f);
	void AddViewQuarterTurns(int delta, float blendTime = 0.0f);
	inline int GetViewQuarterTurns() const { return viewQuarterTurns; }

	// getter/setter
	inline Vector2 GetOffset() const { return offset; }
	inline void SetOffset(const Vector2& newOffset) { offset = newOffset; }

	// 활성 카메라가 없을 때 첫 등록/승계 대상이 될지 여부. (언리얼 bAutoActivate)
	inline bool IsAutoActivate() const { return autoActivate; }
	inline void SetAutoActivate(bool value) { autoActivate = value; }

private:
	// 오너 위치에서 뷰 중심을 밀 때 쓴다.
	Vector2 offset = Vector2::Zero;

	// 활성 카메라가 없으면 자동으로 활성화될지 여부.
	bool autoActivate = true;

	// 목표(정지) 회전. 0~3. 보간 진행 상태는 CameraManager가 소유한다.
	int viewQuarterTurns = 0;

	// 매니저에 이미 등록됐는지. 중복 등록 방지용.
	bool registered = false;
};

NAME_SPACE_END
