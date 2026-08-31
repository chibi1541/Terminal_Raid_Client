#pragma once

#include "Utils/EngineMacro.h"
#include "Utils/Types.h"

#include "Asset/AnimationClip.h"
#include <memory>
#include <vector>

NAME_SPACE_BEGIN(Craft)

// 클립 하나를 재생하는 "시간 커서".
//
// Draw 체인(Engine::Draw -> Level::Draw -> Actor::Draw)에는 deltaTime이 없다.
// 그래서 시간을 밀어주는 일(Tick)과 그리는 일(Draw)을 반드시 나눠야 하고,
// 이 클래스가 그 경계에 선다.
//   Tick : 시간을 누적해 currentFrameIndex를 전진시킨다.
//   Draw : GetCurrentSprite()로 "지금 그릴 한 장"만 읽어간다.
//
// CraftObject를 상속하지 않는다. 타입 질의 대상이 아니고,
// 컴포넌트 안에 값으로 들어가는 가벼운 상태 덩어리이기 때문.
class CRAFT_API AnimationPlayer
{
public:
	AnimationPlayer() = default;
	~AnimationPlayer() = default;

	// 재생할 클립을 지정한다.
	//
	// 이미 재생 중인 클립과 같으면 아무것도 하지 않는다(forceRestart가 false일 때).
	// 이 "같은 클립이면 무시" 규칙이 중요하다. 나중에 상태 머신이 매 틱
	// 현재 상태의 클립을 지정하게 되는데, 그때마다 재생이 0프레임으로 되돌아가면
	// 애니메이션이 첫 장에서 멈춰 보이기 때문이다.
	void Play(const std::shared_ptr<const AnimationClip>& newClip, bool forceRestart = false);

	// 시간을 누적해서 프레임을 전진시킨다. Actor::Tick 경로에서만 호출.
	void Tick(float deltaTime);

	// 재생 위치를 맨 앞으로 되돌린다. 클립은 그대로 유지.
	void Reset();

	// 지금 그려야 할 한 장. 재생할 클립이 없으면 nullptr.
	// 반환된 포인터는 clip이 살아있는 동안 유효하다(this가 shared_ptr로 붙잡고 있음).
	const Sprite* GetCurrentSprite() const;

	inline const std::shared_ptr<const AnimationClip>& GetClip() const { return clip; }
	inline int GetCurrentFrameIndex() const { return currentFrameIndex; }

	// 루프가 아닌 클립이 마지막 프레임에 도달했는지.
	// 나중에 "공격 애니가 끝나면 Idle로" 같은 전이 조건의 재료가 된다.
	inline bool HasFinished() const { return hasFinished; }

	// 클립 전체에서 현재 재생 위치(0.0 ~ 1.0).
	// "80% 지점부터 다음 상태로" 같은 전이 조건에 쓰려고 미리 만들어 둔다.
	float GetNormalizedTime() const;

	inline float GetPlayRate() const { return playRate; }

	// 재생 속도 배율. 0이면 일시정지.
	// TODO : 역재생(음수)은 아직 지원하지 않음.
	inline void SetPlayRate(float rate)
	{
		ASSERT_CRASH(rate >= 0.0f);
		playRate = rate;
	}

	// --- 프레임 이벤트 ---------------------------------------------------
	// 재생기는 "무슨 일이 있었는지"만 남긴다. 이름을 붙이고 게임플레이로 보내는 건
	// AnimInstance의 일이다. 여기까지가 시간/프레임 커서의 책임 범위다.

	// 기록을 비운다. 프레임 경계를 정하는 건 소유자(AnimInstance::TickLayer)다.
	//
	// Tick() 안에서 비우지 않는 이유 - TickLayer는 "평가 -> Play -> Tick" 순인데
	// Play가 클립을 바꾸면 Reset이 "0번 진입"을 기록한다. Tick 맨 앞에서 비우면 그게 지워진다.
	// 밖에서 명시적으로 비우면 Tick()의 얼리 아웃(1프레임 클립, 이미 끝난 논루프)도 안전하다.
	void ClearFrameEvents();

	// 이번 틱에 새로 진입한 프레임들. 진입한 순서대로 들어있다.
	//
	// 한 틱에 여러 장을 넘길 수 있어서(저프레임/빠른 클립) 목록이다.
	// 마지막 것만 남기면 저프레임에서 중간 프레임의 판정이 통째로 씹힌다.
	inline const std::vector<int>& GetFramesEnteredThisTick() const { return framesEnteredThisTick; }

	// 이번 틱에 논루프 클립이 막 끝났는지. (hasFinished가 이번에 켜졌는지)
	inline bool HasJustFinished() const { return hasJustFinished; }

private:
	// 재생 중인 클립. 여러 액터가 같은 클립을 공유하므로 const shared_ptr.
	std::shared_ptr<const AnimationClip> clip;

	// 현재 프레임에 머문 시간(초). frameDuration을 넘으면 다음 장으로 넘어간다.
	float elapsedTime = 0.0f;

	int currentFrameIndex = 0;

	float playRate = 1.0f;

	bool hasFinished = false;

	// 이번 틱에 진입한 프레임들. ClearFrameEvents로 비우고 Reset/Tick이 채운다.
	std::vector<int> framesEnteredThisTick;

	bool hasJustFinished = false;
};

NAME_SPACE_END
