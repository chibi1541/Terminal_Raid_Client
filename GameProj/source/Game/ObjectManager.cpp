#include "pch.h"
#include "ObjectManager.h"

#include "Actor/LocalPlayer.h"
#include "Actor/RemotePlayer.h"
#include "Actor/Monster.h"
#include "Actor/ProjectileActor.h"
#include "Engine/Engine.h"
#include "Level/Level.h"
#include "Thread/ThreadManager.h"
#include "Utils/ObjectIdHandler.h"

using namespace Craft;

ObjectManager& ObjectManager::Get()
{
	static ObjectManager instance;
	return instance;
}

void ObjectManager::BindGameThread()
{
	gameThreadId = Engine::Get().GetThreadManager()->GetThreadID();
}

void ObjectManager::EnsureGameThread() const
{
	const uint32 currentThreadId = Engine::Get().GetThreadManager()->GetThreadID();

	// 네트워크 쓰레드에서 직접 불렀다는 뜻이다.
	// 여기서 잡지 않으면 레벨의 액터 목록을 두 쓰레드가 동시에 만지게 된다.
	ASSERT_CRASH(gameThreadId == 0 || currentThreadId == gameThreadId);
}

void ObjectManager::OnEnterRoom(const Protocol::S_ENTER_ROOM& pkt)
{
	EnsureGameThread();

	if (pkt.success() == false)
	{
		return;
	}

	// 재입장일 수 있다. 이전 룸의 액터가 남아있으면 안 된다.
	ClearAll();

	myObjectId = pkt.myobject().objectid();

	// 내 캐릭터. 서버는 본인에게 S_SPAWN을 보내지 않으므로
	// LocalPlayer가 만들어지는 곳은 여기 한 군데뿐이다.
	Spawn(pkt.myobject(), true);

	// 내가 들어오기 전부터 룸에 있던 개체들.
	for (const Protocol::ObjectInfo& info : pkt.objects())
	{
		Spawn(info, false);
	}
}

void ObjectManager::OnExitRoom()
{
	EnsureGameThread();

	ClearAll();
	myObjectId = 0;
}

void ObjectManager::OnSpawn(const Protocol::S_SPAWN& pkt)
{
	EnsureGameThread();

	for (const Protocol::ObjectInfo& info : pkt.objects())
	{
		// 서버는 본인에게 자기 S_SPAWN을 보내지 않지만,
		// 혹시 들어오더라도 내 캐릭터를 두 번 만들지 않도록 막는다.
		Spawn(info, info.objectid() == myObjectId);
	}
}

void ObjectManager::OnDespawn(const Protocol::S_DESPAWN& pkt)
{
	EnsureGameThread();

	for (const uint64 objectId : pkt.objectids())
	{
		auto findIt = objects.find(objectId);

		if (findIt == objects.end())
		{
			continue;
		}

		// Destroy()는 만료 플래그만 세운다.
		// 실제 목록 제거는 레벨이 프레임 끝에 한다(ProcessAddAndDestoryActors).
		if (std::shared_ptr<ReplicatedActor> actor = findIt->second.lock())
		{
			actor->Destroy();
		}

		objects.erase(findIt);
	}
}

void ObjectManager::OnMove(const Protocol::S_MOVE& pkt)
{
	EnsureGameThread();

	for (const Protocol::MoveInfo& info : pkt.moves())
	{
		// 내 캐릭터는 로컬 예측이 담당한다. S_MOVE_ACK로 따로 보정
		if (info.objectid() == myObjectId)
			continue;

		std::shared_ptr<ReplicatedActor> actor = Find(info.objectid());
		if (actor == nullptr)
			continue;

		actor->ApplyMove(info);
	}
}

void ObjectManager::OnMoveAck(const Protocol::S_MOVE_ACK& pkt)
{
	EnsureGameThread();

	// TODO : local 플레이어 prediction 처리

}

void ObjectManager::Spawn(const Protocol::ObjectInfo& info, bool isLocal)
{
	const uint64 objectId = info.objectid();

	// 중복 스폰. S_ENTER_ROOM과 S_SPAWN이 겹쳐 도착하면 여기서 걸린다.
	if (objects.find(objectId) != objects.end())
	{
		return;
	}

	std::shared_ptr<Level> level = Engine::Get().GetLevel();

	if (level == nullptr)
	{
		return;
	}

	// 개체 타입은 objectId 상위 16비트에 들어 있다. 따로 실려오지 않는다.
	const Protocol::ObjectType objectType = ObjectIdHandler::GetObjectType(objectId);

	std::shared_ptr<ReplicatedActor> actor;

	switch (objectType)
	{
	case Protocol::OBJECT_PLAYER:
		actor = isLocal
			? std::static_pointer_cast<ReplicatedActor>(level->SpawnActor<LocalPlayer>())
			: std::static_pointer_cast<ReplicatedActor>(level->SpawnActor<RemotePlayer>());
		break;

	case Protocol::OBJECT_MONSTER:
		actor = std::static_pointer_cast<ReplicatedActor>(level->SpawnActor<Monster>());
		break;

	case Protocol::OBJECT_PROJECTILE:
		actor = std::static_pointer_cast<ReplicatedActor>(level->SpawnActor<ProjectileActor>());
		break;

	default:
		// 아직 클라이언트에 대응 타입이 없는 개체. 조용히 건너뛴다.
		return;
	}

	// SpawnActor는 액터를 추가 요청 목록에 넣고 shared_ptr을 바로 돌려준다.
	// 그래서 레벨에 실제로 올라가기 전인 지금 초기 상태를 꽂을 수 있다.
	actor->ApplyObjectInfo(info);

	objects[objectId] = actor;
}

void ObjectManager::ClearAll()
{
	for (auto& item : objects)
	{
		if (std::shared_ptr<ReplicatedActor> actor = item.second.lock())
		{
			actor->Destroy();
		}
	}

	objects.clear();
}

std::shared_ptr<ReplicatedActor> ObjectManager::Find(uint64 objectId) const
{
	auto findIt = objects.find(objectId);

	if (findIt == objects.end())
	{
		return nullptr;
	}

	return findIt->second.lock();
}

std::shared_ptr<LocalPlayer> ObjectManager::GetLocalPlayer() const
{
	return Cast<LocalPlayer>(Find(myObjectId));
}
