#pragma once

#include "Actor/Actor.h"

// 마우스 커서를 따라다니는 오렌지색 십자선(+) 오버레이.
//
//   .O.
//   OOO
//   .O.
//
// 게임 상태에 아무 영향도 주지 않는다. 매 프레임 커서 셀 위치에 화면 고정으로 그린다.
// MenuLevel 과 인게임(ObjectManager 가 룸 입장 때 스폰) 양쪽에 하나씩 존재한다.
class CrosshairActor : public Craft::Actor
{
	TYPE_DECLARATIONS(CrosshairActor, Craft::Actor)

public:
	CrosshairActor() = default;

	virtual void BeginPlay() override;
	virtual void Draw() override;
};
