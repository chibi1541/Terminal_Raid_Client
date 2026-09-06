#include "pch.h"
#include "Monster.h"

void Monster::ApplyObjectInfo(const Protocol::ObjectInfo& info)
{
	super::ApplyObjectInfo(info);
}

void Monster::BeginPlay()
{
	// 나중에 수정
	animName = "Necromancer";

	super::BeginPlay();
}

void Monster::Tick(float deltaTime)
{
	// 보간기가 시작된 뒤에만 반영한다 - ApplyObjectInfo(스폰)를 안 거친 액터
	// (Game.cpp의 "//temp" 테스트 스폰 등)는 시작되지 않은 채라 원점으로 튀는 걸 막는다.
	if (interpolator.IsStarted())
	{
		SetPosition(interpolator.Evaluate(deltaTime));
	}

	super::Tick(deltaTime);
}


