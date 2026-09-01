#pragma once

#include "PlayerActor.h"

// 다른 유저가 조종하는 플레이어.
//
// 입력을 받지 않는다. 위치와 상태는 오직 서버가 보낸 ObjectInfo로만 바뀐다.
//
// TODO : 위치 보간.
// 지금은 서버 값을 그대로 꽂아서 갱신이 올 때마다 순간이동한다.
// 이동 패킷(S_MOVE)이 생기면 목표 지점을 향해 부드럽게 따라가도록 바꾼다.
class RemotePlayer : public PlayerActor
{
	TYPE_DECLARATIONS(RemotePlayer, PlayerActor)

public:
	RemotePlayer() = default;

protected:
	// 남의 캐릭터는 흰색 이름표.
	virtual Craft::Color GetNameColor() const override { return Craft::Color::White; }
};
