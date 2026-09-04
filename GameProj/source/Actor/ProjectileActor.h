#pragma once

#include "ReplicatedActor.h"
#include <memory>

// 전방 선언
NAME_SPACE_BEGIN(Craft)

class AnimationPlayer;

NAME_SPACE_END

class ProjectileActor : public ReplicatedActor
{
	TYPE_DECLARATIONS(ProjectileActor, ReplicatedActor)

public:
	ProjectileActor();

	virtual void BeginPlay() override;

	virtual void Tick(float deltaTime) override;

	virtual void Draw() override;

protected:
	// 아직 상속을 하게 될 지 모르겠지만 특수한 궤적 혹은 기믹을 가진 투사체 때문에
	// 상속하게 될 때를 대비
	std::shared_ptr<Craft::AnimationPlayer> animPlayer;

	float _speed = 20.f;

	std::string animName;

private:
	

};

