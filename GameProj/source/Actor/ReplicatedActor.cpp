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
	const Vector2 pos(state.pos().x(), state.pos().y());
	SetPosition(pos);

	lastDirection = state.dir();

	// 스폰/재입장 스냅샷 - 보간 없이 그 자리에서 시작한다.
	// state에 실려온 dir/speed로 속도를 채워 두면, 스폰 직후 아직 S_MOVE가
	// 한 건도 안 왔어도(외삽 구간) 이미 움직이고 있던 개체가 멈칫하지 않는다.
	interpolator.Reset(pos, 0,
		MovementInterpolator::VelocityFromServer(state.dir(), state.speed()));
}

void ReplicatedActor::ApplyMove(const Protocol::MoveInfo& info)
{
	lastDirection = info.dir();

	interpolator.AddSample(
		info.servertick(),
		Vector2(info.pos().x(), info.pos().y()),
		MovementInterpolator::VelocityFromServer(info.dir(), info.speed()));
}

Protocol::ObjectType ReplicatedActor::GetObjectType() const
{
	return ObjectIdHandler::GetObjectType(objectId);
}
