#pragma once

#include "Actor/Actor.h"
#include "Protocol/Struct.pb.h"

// 서버와 동기화되는 모든 개체의 베이스.
//
// 서버의 GameObject(ServerProj/source/Game/GameObject.h)와 1:1로 대응하고,
// 그 계약서가 Protocol::ObjectInfo다.
// 나중에 몬스터(MonsterActor)와 투사체(ProjectileActor)가 이 클래스의 형제로 붙는다.
//
// !! 이 클래스와 파생 클래스는 전부 게임(메인) 쓰레드 전용이다. !!
// 서버에서 온 값은 ObjectManager를 거쳐 게임 쓰레드에서만 반영된다.
// 네트워크 쓰레드에서 직접 만지면 안 된다.
class ReplicatedActor : public Craft::Actor
{
	TYPE_DECLARATIONS(ReplicatedActor, Craft::Actor)

public:
	ReplicatedActor() = default;

	// 서버가 보낸 개체 정보를 반영한다.
	//
	// 파생 클래스는 super를 먼저 부른 뒤 자기 전용 필드만 덧칠한다.
	// 서버 쪽 GameObject::FillObjectInfo가 쓰는 규약과 정확히 대칭이다.
	virtual void ApplyObjectInfo(const Protocol::ObjectInfo& info);

	inline uint64 GetObjectId() const { return objectId; }

	// 개체 타입은 따로 실려오지 않는다. objectId 상위 16비트에 들어 있다.
	Protocol::ObjectType GetObjectType() const;
protected:
	// 서버가 발급한 식별자. 클라이언트는 발급하지 않고 받아서 해석만 한다.
	uint64 objectId = 0;
};
