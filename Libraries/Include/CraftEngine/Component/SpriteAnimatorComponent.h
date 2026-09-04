#pragma once

#include "Utils/EngineMacro.h"
#include "Utils/Types.h"
#include "Component/ActorComponent.h"
#include "Animation/AnimInstance.h"
#include "Math/Vector2.h"
#include "Math/SymbolPalette.h"
#include "Asset/AssetTypes.h"
#include <string>
#include <memory>
#include <functional>

NAME_SPACE_BEGIN(Craft)

// 액터에 스프라이트 애니메이션을 붙이는 컴포넌트.
// 언리얼의 USkeletalMeshComponent 자리다 - 화면에 그리고, AnimInstance를 소유한다.
//
// 판단은 전부 AnimInstance가 한다. 이 컴포넌트는
//   Tick : AnimInstance에 시간을 넘기고
//   Draw : AnimInstance가 내놓은 픽셀맵을 Renderer에 제출한다
// 그 이상은 하지 않는다.
//
// 게임플레이는 GetParameters()에 값만 넣는다. 클립 이름을 알 필요가 없다.
class CRAFT_API SpriteAnimatorComponent : public ActorComponent
{
	TYPE_DECLARATIONS(SpriteAnimatorComponent, ActorComponent)

public:
	SpriteAnimatorComponent() = default;
	virtual ~SpriteAnimatorComponent() = default;

	virtual void Tick(float deltaTime) override;
	virtual void Draw() override;

	// XML 애니메이션 정의(*.anim.xml)를 읽어서 클립을 한꺼번에 등록한다.
	// 등록된 클립 수를 반환한다(0이면 파일이 없거나 파싱 실패).
	int LoadClipsFromFile(const WCHAR* path);

	// 위와 같지만 파싱을 워커 쓰레드에 맡긴다. 호출은 즉시 반환된다.
	// 등록된 클립 수가 onLoaded로 넘어온다(0이면 파일이 없거나 파싱 실패).
	//
	// onLoaded는 메인 쓰레드의 AssetManager::Tick() 안에서 불린다.
	// 상태 머신은 클립 이름을 검증하므로 onLoaded 안에서 이어 읽어야 한다.
	//
	// 주의 - 콜백이 도착하기 전에 소유 액터가 파괴될 수 있다.
	// 콜백 쪽에서 weak_ptr로 생존을 확인할 것.
	void LoadClipsFromFileAsync(const WCHAR* path, std::function<void(int)> onLoaded);

	// XML 상태 머신 정의(*.fsm.xml)를 읽어서 레이어와 상태 머신을 채운다.
	// 채운 레이어 수를 반환한다.
	//
	// 상태의 clip 이름을 검증하기 때문에 반드시 LoadClipsFromFile 다음에 호출해야 한다.
	int LoadStateMachineFromFile(const WCHAR* path);

	// 클립을 하나 등록한다. 키는 clip->GetName().
	void AddClip(const std::shared_ptr<const AnimationClip>& clip);

	// 등록된 클립을 이름으로 직접 재생한다. 없는 이름이면 false.
	//
	// 상태 머신을 쓰지 않는 액터(클립 하나만 계속 트는 이펙트 등)를 위한 경로다.
	// 상태 머신이 로드된 레이어에서는 매 틱 현재 상태의 클립으로 덮어쓰이므로
	// 이 함수를 쓰면 안 된다. 그때는 GetParameters()로 파라미터만 바꿀 것.
	bool PlayClip(const std::string& name, bool forceRestart = false);

	// getter/setter
	// 게임플레이가 애니메이션에 값을 건네는 창구.
	inline AnimParameters& GetParameters() { return animInstance.GetParameters(); }

	// 레이어/재생 상태를 직접 다뤄야 할 때.
	inline AnimInstance& GetAnimInstance() { return animInstance; }
	inline const AnimInstance& GetAnimInstance() const { return animInstance; }

	// 이번 프레임에 그릴 방향 슬롯.
	//
	// 여기 넣는 값은 월드 방향이 아니라 "화면" 슬롯이다
	// (= RotateFacing(월드 방향, 카메라 회전)). 카메라를 아는 것은 액터 쪽이라 거기서 정한다.
	//
	// 클립이 방향별로 나뉘어 있으면(*.anim.xml의 @facing) 이 값이 어느 그림을 쓸지 고르고,
	// 좌우 반전도 그 슬롯이 정한다. 방향이 없는 클립만 있는 액터에 불러도 안전하다 -
	// 네 슬롯이 전부 같은 클립이라 그림은 그대로고 왼쪽 슬롯에서만 반전이 켜진다.
	//
	// ★ 호출 시점 주의 ★
	// 이 컴포넌트의 Tick이 애니메이션을 평가하므로, 소유 액터는 super::Tick(=Actor::Tick)
	// "앞에서" 이 함수를 불러야 같은 프레임에 반영된다. 뒤에 두면 언제나 한 프레임 늦는다.
	inline void SetFacing(EFacing newFacing) { animInstance.SetFacing(newFacing); }
	inline EFacing GetFacing() const { return animInstance.GetFacing(); }

	// 좌우 반전. 아트가 그려진 방향이 false다.
	// 피벗을 축으로 뒤집으므로 방향을 바꿔도 캐릭터 위치는 그대로다.
	//
	// ★ SetFacing을 쓰는 액터에서는 부르지 말 것 ★
	// 매 틱 현재 슬롯의 값으로 덮어써져서 아무 효과가 없다.
	inline void SetFlipX(bool newFlipX) { animInstance.SetFlipX(newFlipX); }
	inline bool GetFlipX() const { return animInstance.GetFlipX(); }

	// 애니메이션이 게임플레이에게 보내는 신호.
	//
	// Actor::Tick이 컴포넌트를 먼저 돌리므로, 게임플레이가 super::Tick(dt) 다음에 물어보면
	// 같은 프레임 안에서 지연 없이 받는다.
	//   if (animator->HasNotify("RollEnd")) { isRolling = false; }
	inline bool HasNotify(const std::string& name) const { return animInstance.HasNotify(name); }
	inline const std::vector<AnimNotifyEvent>& GetNotifies() const { return animInstance.GetNotifies(); }

	inline bool HasClip(const std::string& name) const { return animInstance.HasClip(name); }
	inline int GetClipCount() const { return animInstance.GetClipCount(); }

	// 콘솔 셀은 정사각형이 아니라서 비율 보정이 필요하다.
	// 스프라이트를 크게 그릴 때도 같이 쓴다. (Renderer::SubmitPixels 참고)
	inline void SetScale(int newScaleX, int newScaleY)
	{
		ASSERT_CRASH(newScaleX >= 1 && newScaleY >= 1);
		scaleX = newScaleX;
		scaleY = newScaleY;
	}

	// 액터 위치를 기준으로 한 스프라이트의 그리기 오프셋.
	// 액터의 기준점(발밑 등)과 그림의 좌상단을 맞출 때 쓴다.
	inline void SetOffset(const Vector2& newOffset) { offset = newOffset; }
	inline Vector2 GetOffset() const { return offset; }

private:
	// 파라미터 + 레이어 + 상태 머신 + 합성을 전부 들고 있는 애니메이션의 주체.
	AnimInstance animInstance;

	// AssetManager::Load<AnimationClipSet>()이 돌려준 캐시 항목을 붙들고 있는 용도.
	// 클립은 개별적으로 animInstance에 등록해서 쓰지만, 이 참조가 살아있어야
	// AssetManager 캐시의 refcount가 "사용 중"으로 잡혀서 유휴 언로드 대상이 안 된다.
	std::shared_ptr<const AnimationClipSet> loadedClips;

	Vector2 offset = Vector2::Zero;

	int scaleX = 1;
	int scaleY = 1;
};

NAME_SPACE_END
