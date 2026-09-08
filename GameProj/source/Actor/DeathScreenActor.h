#pragma once

#include "Actor/Actor.h"
#include "Math/Rect.h"

// 로컬 플레이어 사망 시 화면 가운데에 뜨는 리스폰 화면.
//
// "YOU DIED" + "RESPAWN" 픽셀 문자 버튼. 마우스 호버 하이라이트 + 좌클릭.
// 클릭하면 C_RESPAWN 을 보내고, 서버 S_RESPAWN 이 도착해 로컬 플레이어가 다시 살아나면
// (ReplCharacter::isDead 해제) 화면이 사라진다.
//
// 죽은 플레이어의 입력은 이미 무력화돼 있어(LocalPlayer::Tick 이 zero 처리) InputComponent /
// blockAllInput 불필요 - 마우스만 직접 폴링한다. PauseMenuActor 보다 가벼운 버전.
//
// ObjectManager 가 룸 입장 때 하나 스폰하고 weak_ptr 로 들고 있다.
class DeathScreenActor : public Craft::Actor
{
	TYPE_DECLARATIONS(DeathScreenActor, Craft::Actor)

public:
	DeathScreenActor() = default;

	virtual void BeginPlay() override;
	virtual void Tick(float deltaTime) override;
	virtual void Draw() override;

private:
	void ComputeLayout();

	// C_RESPAWN 을 이미 보냈고 아직 부활 통지를 못 받은 상태. 중복 전송 방지.
	bool        _requested = false;

	bool        _hovered = false;

	Craft::Rect _panelRect;
	Craft::Rect _respawnRect;
};
