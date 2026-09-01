#pragma once

#include "ReplicatedActor.h"
#include "Component/SpriteAnimatorComponent.h"

#include <memory>
#include <string>

// LocalPlayer와 RemotePlayer가 공유하는 부분.
//
// "보이는 것"만 여기 있다 - 스프라이트 애니메이터와 머리 위 이름표.
// 입력은 LocalPlayer만 갖고, 서버 위치 보간은 RemotePlayer만 갖는다.
class PlayerActor : public ReplicatedActor
{
	TYPE_DECLARATIONS(PlayerActor, ReplicatedActor)

public:
	PlayerActor() = default;

	virtual void BeginPlay() override;
	virtual void Draw() override;

	// PlayerInfo.name을 여기서 받는다.
	virtual void ApplyObjectInfo(const Protocol::ObjectInfo& info) override;

	inline const std::string& GetPlayerName() const { return playerName; }

protected:
	// 이름표 색. 내 캐릭터와 남을 화면에서 구분하는 유일한 수단이다.
	// (서버가 chartype을 안 보내서 스프라이트는 둘 다 같다)
	virtual Craft::Color GetNameColor() const { return Craft::Color::White; }

protected:
	// 스프라이트 애니메이션 재생 담당.
	// 생성자가 아니라 BeginPlay에서 만든다(weak_from_this가 그때부터 유효).
	std::shared_ptr<Craft::SpriteAnimatorComponent> animator;

	std::string playerName;
};
