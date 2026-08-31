#pragma once

#include "Utils/EngineMacro.h"
#include "Core/CraftObject.h"

#include <memory>

NAME_SPACE_BEGIN(Craft)

class Actor;

// 액터에 붙여서 기능 하나를 담당하는 부품. (언리얼의 UActorComponent에 해당)
//
// 액터가 모든 기능을 직접 상속으로 떠안지 않게 하려는 것이 목적이다.
// 액터의 BeginPlay/Tick/Draw 이벤트는 Actor가 자기 컴포넌트 목록을 돌면서 그대로 전달한다.
class CRAFT_API ActorComponent : public CraftObject
{
	TYPE_DECLARATIONS(ActorComponent, CraftObject)

public:
	ActorComponent() = default;
	virtual ~ActorComponent() = default;

	// 게임 플레이 이벤트 함수. Actor가 소유한 컴포넌트를 순회하며 호출한다.
	// 파생 클래스에서 오버라이드할 때는 super::BeginPlay()를 먼저 호출할 것.
	virtual void BeginPlay();
	virtual void Tick(float deltaTime);
	virtual void Draw();

	// getter/setter
	// ownership(약참조하던 객체를 shared_ptr로 만들어서 반환)
	inline std::shared_ptr<Actor> GetOwner() const { return owner.lock(); }
	inline void SetOwner(std::weak_ptr<Actor> newOwner) { owner = newOwner; }

	inline bool IsActive() const { return isActive; }
	inline void SetActive(bool newActive) { isActive = newActive; }

	inline bool HasBeganPlay() const { return hasBeganPlay; }

protected:
	// ownership
	// Actor -> Component 방향이 shared_ptr이므로 역방향은 weak_ptr로 잡는다.
	// (Actor가 Level을 weak_ptr로 참조하는 것과 같은 이유 - 순환 참조 방지)
	std::weak_ptr<Actor> owner;

	// 컴포넌트 활성화 여부 플래그. false면 Tick/Draw를 건너뛴다.
	bool isActive = true;

	// BeginPlay
	bool hasBeganPlay = false;
};

NAME_SPACE_END
