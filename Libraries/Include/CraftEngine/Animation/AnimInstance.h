#pragma once

#include "Utils/EngineMacro.h"
#include "Utils/Types.h"
#include "Actor/Facing.h"
#include "Animation/AnimParameters.h"
#include "Animation/AnimStateMachine.h"
#include "Animation/AnimationPlayer.h"
#include "Asset/AnimationClip.h"
#include <string>
#include <memory>
#include <vector>
#include <unordered_map>

NAME_SPACE_BEGIN(Craft)

// 이번 프레임에 실제로 발생한 노티파이 하나.
//
// 선언(AnimNotify)은 클립이 들고 있고, 이건 그게 "울린 결과"다.
struct CRAFT_API AnimNotifyEvent
{
	std::string name;

	// 어느 클립에서 났는지. 제네릭하게 처리할 때 쓴다.
	std::string clipName;

	// 어느 레이어에서 났는지.
	//
	// 필요한 이유 - Base와 Overlay가 같은 클립을 재생하는 경우가 실제로 있다
	// (지금 TestActor의 캔버스에서 양쪽 다 Idle을 쓴다). 그럴 때 한 프레임에 같은 이름이
	// 두 번 나오는데, 이 값이 없으면 어느 쪽에서 온 건지 구분할 방법이 없다.
	bool isFromOverlay = false;
};

// 방향 슬롯 하나. 폴백 해소가 끝나면 clip은 절대 널이 아니다.
struct CRAFT_API DirectionalClipSlot
{
	std::shared_ptr<const AnimationClip> clip;

	// 이 슬롯을 그릴 때 좌우를 뒤집어야 하는가.
	//
	// 미러 이미지를 따로 굽지 않는 이유 - AnimInstance는 이미 합성이 끝난 결과를
	// 한 번만 뒤집고 피벗도 같이 옮긴다(Composite 참고). 프레임을 복제하면 메모리만 늘고,
	// 무엇보다 좌 <-> 우 전환에서 클립 포인터가 달라져 재생이 끊긴다.
	// 같은 포인터를 공유하고 이 플래그만 다르게 두면 전환이 공짜다.
	bool mirrored = false;
};

// 논리 클립 이름 하나에 대한 네 방향 슬롯. (PropSprite가 스프라이트로 하는 일과 같다)
//
// ★ 폴백은 FinalizeClips()에서 끝난다 ★
// 그래서 런타임에는 "이 슬롯이 비었나" 분기가 아예 없다.
// 아트가 정면 한 장뿐이어도 네 슬롯이 전부 채워진 상태로 틱에 들어간다.
struct CRAFT_API DirectionalClipSet
{
	DirectionalClipSlot slots[FacingCount];

	// 데이터가 진짜로 선언한 슬롯인지(폴백으로 채워진 것과 구분).
	//
	// 미러 폴백이 이 값을 봐야 한다. 폴백으로 들어온 앞모습을 다시 좌우 반전하면
	// 얼굴이 반대편에 붙은 그림이 나온다 - 진짜 측면 아트에서만 미러해야 한다.
	bool authored[FacingCount] = { false, false, false, false };

	// @facing을 안 적은 클립. 있으면 네 슬롯 전부의 원본이 된다.
	std::shared_ptr<const AnimationClip> omniClip;
};

// 상태 머신 하나와 그 재생 상태를 묶은 단위.
//
// 5단계부터 AnimInstance는 이 타입을 정확히 2개(BaseLayer/Overlay) 고정으로 갖는다.
// 그래서 이름표나 마스크/블렌딩 같은 "역할을 구분하기 위한" 필드가 필요 없다 -
// 역할은 AnimInstance::GetBaseLayer() / GetOverlayLayer() 중 어느 쪽으로 접근했느냐로 정해지고,
// 영역(rows)과 블렌딩 여부는 지금 재생 중인 AnimState가 들고 있다(AnimState::region/canBlend).
class CRAFT_API AnimLayer
{
public:
	AnimStateMachine stateMachine;

	AnimationPlayer player;

	// 현재 상태에 머문 시간(초). 전이 조건 stateTime의 값이 된다.
	float stateTime = 0.0f;

	// 지금 재생 중인 클립의 "논리" 이름.
	//
	// player.GetClip()->GetLogicalName()으로도 알 수 있지만, 클립이 널일 때와
	// 상태가 막 바뀐 순간을 구분해야 해서 따로 든다.
	// 이 값이 같은데 클립 포인터만 달라졌다 = 방향만 바뀐 것 -> 재생 위치를 이어받는다.
	std::string playingLogicalClip;
};

// 애니메이션 계층의 주체. (언리얼의 UAnimInstance)
//
// 게임플레이 -> 파라미터 -> 전이 -> 클립 진행 -> 프레임 산출로 이어지는 단방향 파이프라인의
// 한가운데 있다. 애니메이션이 게임플레이를 거꾸로 건드리는 경로는 없다.
//
// SpriteAnimatorComponent는 이걸 소유해서 매 틱 Tick()을 돌리고,
// Draw에서 GetCurrentPixelMap()이 내놓은 결과만 화면에 제출한다.
//
// ★ 레이어는 정확히 2개, 역할이 고정이다 ★
//   BaseLayer : 항상 존재하고 항상 전신을 담당한다. 최우선 기준.
//               현재 상태의 canBlend가 false면 Overlay를 통째로 숨긴다
//               (구르기/사망/피격경직처럼 전신이 하나로 통일돼야 하는 상태).
//   Overlay   : 선택. 상태 머신이 비어 있으면(HasOverlayLayer()==false) 그냥 안 쓰인다.
//               현재 상태의 region이 담당 행, canBlend가 그 안에서 투명을 살릴지(true)
//               통째로 가져갈지(false)를 정한다.
//
// 예전에는 레이어를 N개까지 두는 vector<AnimLayer>였지만, 2개를 넘길 일이 없고
// "레이어에 고정된 마스크"로는 오버레이 하나로 여러 다른 모양의 겹침을 표현할 수 없어서
// (예: 다리만 겹치는 상태와 상체만 겹치는 상태를 같은 Overlay 슬롯에서 오가는 것) 이렇게 굳혔다.
class CRAFT_API AnimInstance
{
public:
	AnimInstance() = default;
	~AnimInstance() = default;

	// 복사 금지.
	//
	// 실용적인 이유 - AnimLayer는 복사 가능하지만(더 이상 unique_ptr을 안 씀),
	// AnimInstance는 여전히 특정 액터의 "지금 재생 상태"라 통째로 복사해서 쓸 물건이 아니다.
	AnimInstance(const AnimInstance&) = delete;
	AnimInstance& operator=(const AnimInstance&) = delete;

	// 게임플레이가 값을 넣는 창구.
	inline AnimParameters& GetParameters() { return parameters; }
	inline const AnimParameters& GetParameters() const { return parameters; }

	// 클립 등록. 키는 clip->GetName()(= 방향까지 포함한 등록 이름).
	void AddClip(const std::shared_ptr<const AnimationClip>& clip);

	// 이름으로 클립을 찾는다. 논리 이름을 주면 "지금 보고 있는 방향"의 슬롯을 돌려준다.
	// 등록 이름("Walk@Up")을 직접 줘도 찾는다.
	std::shared_ptr<const AnimationClip> FindClip(const std::string& name) const;

	inline int GetClipCount() const { return static_cast<int>(clipMap.size()); }

	// 등록 이름 또는 논리 이름 둘 중 하나로 걸리면 참.
	//
	// ★ 논리 이름을 봐야 하는 이유 ★
	// AnimStateMachineLoader가 State의 clip 이름을 이걸로 검증한다. 상태 머신은
	// "Walk"만 아는데 클립 맵에는 "Walk@Up"만 들어있을 수 있으므로,
	// 등록 이름만 보면 방향별로 쪼갠 순간 로드가 크래시한다.
	bool HasClip(const std::string& name) const;

	// 등록된 클립을 논리 이름 -> 네 방향 슬롯으로 정리하고 빈 슬롯을 채운다.
	//
	// 모든 AddClip이 끝난 뒤 한 번 부르면 된다. 잊어도 다음 조회 시점에 알아서 불리지만
	// (AddClip이 dirty 표시를 남긴다), 로드가 끝난 자리에서 명시적으로 불러야
	// 데이터 실수가 프레임 중간이 아니라 로드할 때 터진다.
	void FinalizeClips() const;

	// 지금 그릴 방향 슬롯. 게임플레이가 매 틱 정해서 넣는다.
	//
	// 여기 들어오는 값은 월드 방향이 아니라 "화면" 슬롯이다
	// (= RotateFacing(월드 방향, 카메라 회전)). 카메라를 아는 것은 액터 쪽이다.
	void SetFacing(EFacing displaySlot);
	inline EFacing GetFacing() const { return facing; }

	// 레이어 접근. 둘 다 항상 존재하는 객체를 돌려준다 - Overlay를 안 쓰면
	// 그냥 상태 머신이 비어 있는 채로 남아서 Tick/Composite에서 조용히 무시된다.
	inline AnimLayer& GetBaseLayer() { return baseLayer; }
	inline const AnimLayer& GetBaseLayer() const { return baseLayer; }
	inline AnimLayer& GetOverlayLayer() { return overlayLayer; }
	inline const AnimLayer& GetOverlayLayer() const { return overlayLayer; }
	inline bool HasOverlayLayer() const { return !overlayLayer.stateMachine.IsEmpty(); }

	// 평가 -> 재생 -> 합성. 매 틱 한 번.
	void Tick(float deltaTime);

	// 이번 프레임에 그릴 픽셀맵(합성 결과). 그릴 게 없으면 빈 문자열.
	inline const std::string& GetCurrentPixelMap() const { return compositeBuffer; }

	// 좌우 반전. 아트가 그려진 방향이 false다.
	//
	// ★ SetFacing을 쓰는 액터에서는 이걸 직접 부르지 말 것 ★
	// 방향 슬롯이 정해지는 순간 그 슬롯의 mirrored 값으로 매 틱 덮어써진다.
	// 방향 세트를 안 쓰는 액터(단일 측면 아트 + 게임플레이가 직접 뒤집는 경우)만 이 경로를 쓴다.
	//
	// 합성이 끝난 뒤 결과를 한 번만 뒤집는다. BaseLayer/Overlay를 따로 뒤집지 않는 이유는
	// 마스크가 행 기준이라 가로 반전과 무관해서 결과가 같고, 한 번이면 충분하기 때문이다.
	// Renderer도 건드릴 필요가 없다.
	void SetFlipX(bool newFlipX);
	inline bool GetFlipX() const { return flipX; }

	// 합성 결과 안에서 액터 위치에 놓여야 할 칸(셀 단위, 반올림 완료).
	// 반전 상태를 반영한다. 그릴 게 없으면 Vector2::Zero.
	//
	// 화면 좌표로 옮기는 건 호출자 몫이다 - 확대 배율을 아는 쪽이 곱해야 한다.
	//   좌상단 = 액터 위치 + offset - (피벗셀.x * scaleX, 피벗셀.y * scaleY)
	Vector2 GetCurrentPivotCell() const;

	// --- 노티파이 --------------------------------------------------------
	// 애니메이션이 게임플레이에게 보내는 유일한 신호 경로.
	//
	// 큐는 Tick() 시작 시 비워진다. 프레임 단위 수명이라 "소비" 개념이 필요 없고,
	// 같은 프레임 안에서 몇 번을 물어봐도 결과가 같다.
	//
	// Actor::Tick이 컴포넌트를 먼저 돌리므로, 게임플레이가 super::Tick() 다음에 읽으면
	// 같은 프레임 안에서 지연 없이 받는다.
	inline const std::vector<AnimNotifyEvent>& GetNotifies() const { return notifyQueue; }
	bool HasNotify(const std::string& name) const;

private:
	// 레이어 하나의 상태를 갱신한다(전이 평가 -> 클립 적용 -> 시간 전진).
	void TickLayer(AnimLayer& layer, float deltaTime);

	// 논리 이름 + 현재 facing -> 그릴 슬롯. 등록되지 않은 이름이면 nullptr.
	const DirectionalClipSlot* ResolveSlot(const std::string& logicalName) const;

	// BaseLayer의 현재 상태가 Overlay를 화면에 내보내도 되는지.
	//
	// Composite()와 노티파이 수집이 반드시 같은 판단을 써야 해서 함수로 묶었다.
	// 따로 복사해두면 나중에 한쪽만 바뀌었을 때
	// "화면엔 안 보이는데 소리는 나는" 종류의 버그가 생긴다.
	bool AllowsOverlay() const;

	// 레이어의 이번 틱 프레임 이벤트를 클립의 노티파이 선언과 대조해 큐에 넣는다.
	void CollectNotifies(const AnimLayer& layer, bool isOverlay);

	// BaseLayer를 깔고, 허용되면 Overlay의 담당 영역을 얹어 compositeBuffer를 만든다.
	void Composite();

private:
	// 등록 이름 -> 클립. 방향 변형은 여기서 "Walk@Up"처럼 서로 다른 키를 갖는다.
	std::unordered_map<std::string, std::shared_ptr<const AnimationClip>> clipMap;

	// 논리 이름 -> 네 방향 슬롯. clipMap에서 파생되는 캐시다.
	//
	// mutable인 이유 - FindClip/HasClip 같은 const 조회에서도 필요하면 다시 만들어야 한다.
	// "쓰기 전에 FinalizeClips를 부를 것"이라는 규약으로 두면 언젠가 잊고, 그때 증상이
	// "아무것도 안 그려진다"라 원인을 찾기 어렵다. 잊을 수 없게 만드는 쪽을 골랐다.
	mutable std::unordered_map<std::string, DirectionalClipSet> directionalClips;

	// clipMap이 바뀐 뒤 directionalClips를 다시 만들어야 하는지.
	mutable bool clipsDirty = false;

	// 지금 그리고 있는 방향 슬롯.
	EFacing facing = EFacing::Down;

	// SetFacing이 한 번이라도 불렸는지.
	//
	// 이게 false면 flipX를 건드리지 않는다 - 방향을 안 쓰는 기존 액터(TestActor 등)가
	// 게임플레이에서 직접 SetFlipX로 뒤집던 동작을 그대로 유지하기 위한 것이다.
	bool facingDriven = false;

	AnimParameters parameters;

	AnimLayer baseLayer;
	AnimLayer overlayLayer;

	// 매 프레임 문자열을 새로 만들지 않도록 재사용하는 합성 버퍼.
	std::string compositeBuffer;

	// 합성 결과의 크기와 피벗. Composite()가 채운다. 둘 다 BaseLayer의 현재 클립을 따른다.
	int compositeWidth = 0;
	int compositeHeight = 0;
	float compositePivotX = 0.0f;
	float compositePivotY = 0.0f;

	bool flipX = false;

	// 이번 프레임에 발생한 노티파이들. Tick() 시작 시 비워진다.
	std::vector<AnimNotifyEvent> notifyQueue;
};

NAME_SPACE_END
