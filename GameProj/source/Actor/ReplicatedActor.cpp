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
	radius = state.radius();

	// 스폰/재입장 스냅샷 - 보간 없이 그 자리에서 시작한다.
	// state에 실려온 dir/speed로 속도를 채워 두면, 스폰 직후 아직 S_MOVE가
	// 한 건도 안 왔어도(외삽 구간) 이미 움직이고 있던 개체가 멈칫하지 않는다.
	interpolator.Reset(pos, 0,
		MovementInterpolator::VelocityFromServer(state.dir(), state.speed()));
}

void ReplicatedActor::ApplyMove(const Protocol::MoveInfo& info)
{
	lastDirection = info.dir();

	// 서버가 서브유닛(1셀=256) 권위 위치를 실어 보낸다. 셀로 잘라 쓰면 셀 내부 위치가
	// 사라져 원격 캐릭터가 계단식으로 움직인다 - 256으로 나눠 소수 셀로 보간한다.
	const float cellX = static_cast<float>(info.possubx()) / MovementInterpolator::POS_SUBUNITS;
	const float cellY = static_cast<float>(info.possuby()) / MovementInterpolator::POS_SUBUNITS;

	interpolator.AddSample(
		info.servertick(),
		cellX, cellY,
		MovementInterpolator::VelocityFromServer(info.dir(), info.speed()));
}

Protocol::ObjectType ReplicatedActor::GetObjectType() const
{
	return ObjectIdHandler::GetObjectType(objectId);
}
