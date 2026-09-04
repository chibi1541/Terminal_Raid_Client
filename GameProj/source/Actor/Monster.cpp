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
	super::Tick(deltaTime);


}


