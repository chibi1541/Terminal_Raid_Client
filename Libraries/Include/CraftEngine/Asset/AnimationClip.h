#pragma once

#include "Utils/EngineMacro.h"

#include "Actor/Facing.h"
#include "Asset/Sprite.h"
#include <string>
#include <vector>

NAME_SPACE_BEGIN(Craft)

// 클립의 특정 시점에서 게임플레이에게 보내는 신호. (언리얼의 AnimNotify에 해당)
//
// 애니메이션 파이프라인은 "게임플레이 -> 파라미터 -> 전이 -> 클립 -> 프레임"으로 흐르는 단방향인데,
// 노티파이가 그 유일한 역방향 채널이다.
//   "구르기 클립이 끝났다"  -> 게임플레이가 조종 가능 상태로 되돌린다
//   "공격 3번 프레임이다"   -> 게임플레이가 판정을 낸다
//
// 중요 - 노티파이는 게임플레이까지만 올라가고 파라미터를 직접 건드리지 않는다.
// 노티파이가 파라미터를 쓰면 "노티파이 -> 파라미터 -> 전이 -> 노티파이" 순환이 생긴다.
// 무엇을 할지는 언제나 게임플레이가 정한다.
//
// 상태(AnimState)가 아니라 클립이 소유한다 - 같은 클립을 여러 상태가 재생해도 선언은 하나면 되고,
// "이 클립의 3번 프레임에서 판정이 나간다"는 애니메이션 자체의 속성이기 때문이다.
struct CRAFT_API AnimNotify
{
	std::string name;

	// fireOnFinish가 false일 때만 의미가 있다. 이 프레임에 "진입"하는 순간 발생한다.
	int frameIndex = 0;

	// true면 프레임이 아니라 "논루프 클립이 끝난 순간"에 발생한다. (XML의 frame="end")
	//
	// 마지막 프레임 노티파이와 다르다 - 그건 마지막 장에 "들어갈 때" 울리므로
	// 클립이 실제로 끝나기 한 프레임 빠르다. 구르기 종료처럼 정확한 끝이 필요하면 이쪽을 쓴다.
	bool fireOnFinish = false;
};

// 하나의 동작을 이루는 스프라이트 시퀀스. (언리얼의 AnimSequence에 해당)
//
// "무엇을 그릴지"만 들고 있고 "지금 몇 번째 프레임인지"는 갖지 않는다.
// 재생 위치는 AnimationPlayer가 따로 관리한다.
// 이렇게 나눠야 여러 액터가 같은 클립을 동시에, 서로 다른 시점으로 재생할 수 있다.
//
// 그래서 클립은 항상 shared_ptr<const AnimationClip>로 공유해서 쓴다.
// 추후에 AssetManager가 본체를 관리할 예정
//
// ★ 프레임 크기 불변식 ★
// 한 클립 안의 모든 프레임은 크기가 같아야 한다. 생성자가 검사한다.
// 피벗 기본값이 "프레임 크기"에서 나오고, 합성기도 이 전제 위에서 동작한다.
class CRAFT_API AnimationClip
{
public:
	AnimationClip() = default;

	// 피벗을 프레임 크기에서 자동으로 정한다. (가운데 맨 아래 = 발밑)
	// framesPerSecond: 초당 몇 장을 넘길지. isLooping: 끝에서 처음으로 돌아갈지.
	AnimationClip(
		const std::string& name,
		const std::vector<Sprite>& frames,
		float framesPerSecond = 12.0f,
		bool isLooping = true
	);

	// 피벗을 직접 지정한다.
	AnimationClip(
		const std::string& name,
		const std::vector<Sprite>& frames,
		float framesPerSecond,
		bool isLooping,
		float pivotX,
		float pivotY
	);

	~AnimationClip() = default;

	// 프레임 하나를 가져온다. 범위를 벗어나면 양 끝으로 잘라서(clamp) 반환한다.
	const Sprite& GetFrame(int index) const;

	inline int GetFrameCount() const { return static_cast<int>(frames.size()); }

	// 프레임 한 장이 화면에 머무는 시간(초).
	inline float GetFrameDuration() const { return frameDuration; }

	// 클립 전체를 한 번 재생하는 데 걸리는 시간(초).
	inline float GetDuration() const { return frameDuration * GetFrameCount(); }

	inline bool IsLooping() const { return isLooping; }
	inline const std::string& GetName() const { return name; }

	// 모든 프레임이 같은 크기이므로 클립 단위로 물어볼 수 있다.
	inline int GetWidth() const { return width; }
	inline int GetHeight() const { return height; }

	// 액터 위치에 놓일 스프라이트 안의 점. 셀 단위이고 실수다.
	//
	// 실수인 이유 - 짝수 너비의 가운데는 정수로 떨어지지 않는다(8칸이면 3.5).
	// 좌우 반전은 박스 중심 (W-1)/2 축으로 미러링하는 것이라,
	// 피벗이 그 값과 정확히 같아야 뒤집어도 그림이 제자리에 남는다.
	// 3으로 반올림해 저장하면 뒤집을 때마다 한 칸씩 튄다.
	inline float GetPivotX() const { return pivotX; }
	inline float GetPivotY() const { return pivotY; }

	// 기본 피벗 - 가운데 맨 아래.
	static float GetDefaultPivotX(int width) { return (width - 1) * 0.5f; }
	static float GetDefaultPivotY(int height) { return static_cast<float>(height - 1); }

	// 이 클립이 어느 논리 동작의 어느 방향 변형인지 로더가 적어준다.
	//
	// 등록 이름(GetName)과 논리 이름(GetLogicalName)이 갈리는 이유 -
	// AnimInstance의 클립 맵은 이름 하나에 클립 하나라, 같은 "Walk"를 방향별로 넣으려면
	// 등록 키는 달라야 한다("Walk@Up"). 반면 상태 머신이 지목하는 것은 언제나 논리 이름이다.
	// 상태를 방향마다 복제하지 않기 위한 분리다.
	//
	// AddNotify와 같은 이유로 생성자 인자가 아니다 - 이미 6인자 오버로드가 있어서
	// 더 늘리면 호출부를 읽을 수 없다.
	void SetFacingVariant(const std::string& newLogicalName, EFacingSlotSpec spec);

	// 상태 머신이 아는 이름. SetFacingVariant를 안 부른 클립은 등록 이름과 같다.
	inline const std::string& GetLogicalName() const
	{
		return logicalName.empty() ? name : logicalName;
	}

	// 데이터가 선언한 방향 슬롯. None이면 "방향이 없는 클립"이라 네 슬롯 전부가 된다.
	inline EFacingSlotSpec GetFacingSpec() const { return facingSpec; }

	// 노티파이를 등록한다. 잘못된 선언은 여기서 크래시한다.
	//
	// 생성자 인자로 받지 않는 이유 - 이미 6인자짜리 오버로드가 있어서 더 늘리면 호출부가 읽기 어렵다.
	// 로더가 make_shared<AnimationClip>(비-const)로 만들어 이걸 호출한 뒤
	// shared_ptr<const AnimationClip>로 넘기면(암시 변환) 밖에서는 변경할 수 없다.
	void AddNotify(const AnimNotify& notify);

	inline const std::vector<AnimNotify>& GetNotifies() const { return notifies; }
	inline bool HasNotifies() const { return !notifies.empty(); }

private:
	// 프레임 크기가 모두 같은지 확인하고 width/height를 채운다.
	void ValidateFrames();

private:
	// 클립 식별자. SpriteAnimatorComponent가 이 이름을 키로 클립을 등록한다.
	// 나중에 상태(AnimState)가 재생할 클립을 지목할 때도 이 이름을 쓴다.
	std::string name;

	// 상태 머신이 지목하는 이름. 비어 있으면 name과 같다는 뜻이다.
	std::string logicalName;

	// 이 클립이 그려진 시점(視點). 로더가 @facing에서 읽어 채운다.
	EFacingSlotSpec facingSpec = EFacingSlotSpec::None;

	std::vector<Sprite> frames;

	// 모든 프레임의 공통 크기.
	int width = 0;
	int height = 0;

	float pivotX = 0.0f;
	float pivotY = 0.0f;

	// 이 클립이 재생되는 동안 발생할 신호들. 선언 순서는 의미가 없다
	// (한 프레임에 여러 개가 걸려 있으면 선언 순서대로 나가지만, 그걸 규약으로 삼지는 않는다).
	std::vector<AnimNotify> notifies;

	// fps가 아니라 "한 장당 지속 시간"으로 저장한다.
	// 매 틱 나눗셈을 반복하지 않도록 생성 시점에 한 번만 역수를 구해둔다.
	// TODO : 프레임마다 다른 지속 시간이 필요해지면 vector<float>로 확장.
	float frameDuration = 1.0f / 12.0f;

	bool isLooping = true;
};

NAME_SPACE_END
