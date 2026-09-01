#include "pch.h"
#include "ReplicatedActor.h"

#include "Utils/ObjectIdHandler.h"

using namespace Craft;

void ReplicatedActor::ApplyObjectInfo(const Protocol::ObjectInfo& info)
{
	objectId = info.objectid();

	const Protocol::CreatureState& state = info.state();

	// 서버 좌표계와 클라 좌표계가 같은 격자다(콘솔 셀 단위).
	// 변환이 필요해지면 반드시 이 한 곳에서만 한다.
	SetPosition(Vector2(state.pos().x(), state.pos().y()));

	hp = state.hp();
	maxHp = state.maxhp();
}

Protocol::ObjectType ReplicatedActor::GetObjectType() const
{
	return ObjectIdHandler::GetObjectType(objectId);
}
