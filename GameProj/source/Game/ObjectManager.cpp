#include "pch.h"
#include "ObjectManager.h"

#include "Actor/LocalPlayer.h"
#include "Actor/RemotePlayer.h"
#include "Actor/Monster.h"
#include "Actor/ProjectileActor.h"
#include "Actor/ServerDebugActor.h"
#include "Actor/PauseMenuActor.h"
#include "Asset/AssetManager.h"
#include "Engine/Engine.h"
#include "Game/ActorDataAsset.h"
#include "Level/Level.h"
#include "Level/TileMapLevel.h"
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

	// 메뉴(MenuLevel)에서 넘어온 첫 입장이면 여기서 게임 레벨로 교체한다.
	// AddNewLevel 은 nextLevel 만 세우고 교체는 프레임 끝 - 그래서 이 프레임 동안
	// Engine::GetLevel() 은 아직 MenuLevel 을 돌려준다. AddNewLevel 이 돌려준
	// 새 레벨을 pendingSpawnLevel 에 잡아두고, 아래 스폰들은 SpawnLevel() 로 그쪽에 올린다.
	// OnEnterRoom 은 게임 쓰레드 잡에서 도므로 AddNewLevel 호출이 안전하다.
	pendingSpawnLevel.reset();
	if (Cast<TileMapLevel>(Engine::Get().GetLevel()) == nullptr)
	{
		pendingSpawnLevel = Engine::Get().AddNewLevel<TileMapLevel>();
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

	// 디버그 오버레이 액터 하나. 이전 룸 것이 남아 있으면 정리하고 새로 만든다.
	if (std::shared_ptr<ServerDebugActor> old = debugActor.lock())
	{
		old->Destroy();
	}
	if (std::shared_ptr<Level> level = SpawnLevel())
	{
		debugActor = level->SpawnActor<ServerDebugActor>();
	}

	// 인게임 일시정지 메뉴(ESC). 디버그 오버레이와 같은 규약 - 재입장 시 이전 것 정리.
	if (std::shared_ptr<PauseMenuActor> old = pauseMenu.lock())
	{
		old->Destroy();
	}
	if (std::shared_ptr<Level> level = SpawnLevel())
	{
		pauseMenu = level->SpawnActor<PauseMenuActor>();
	}
}

std::shared_ptr<Craft::Level> ObjectManager::SpawnLevel() const
{
	if (std::shared_ptr<Craft::Level> pending = pendingSpawnLevel.lock())
	{
		return pending;
	}
	return Engine::Get().GetLevel();
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

	if (std::shared_ptr<LocalPlayer> localPlayer = GetLocalPlayer())
	{
		localPlayer->ReconcileMove(pkt);
	}
}

void ObjectManager::OnHit(const Protocol::S_HIT& pkt)
{
	EnsureGameThread();

	// 피격자만 반영한다. 공격자(attackerId)는 표시할 UI가 아직 없다.
	std::shared_ptr<ReplicatedActor> actor = Find(pkt.targetid());

	if (actor != nullptr)
	{
		actor->ApplyHit(pkt);
	}
}

void ObjectManager::OnDeath(const Protocol::S_DEATH& pkt)
{
	EnsureGameThread();

	std::shared_ptr<ReplicatedActor> actor = Find(pkt.objectid());

	if (actor != nullptr)
	{
		actor->ApplyDeath(pkt);
	}

	// 실제 제거는 여기서 하지 않는다. 주석대로 이 직후 S_DESPAWN이 뒤따라오고,
	// OnDespawn이 Destroy()를 부른다. 여기서 먼저 지우면 그 사이 한두 프레임
	// 재생돼야 할 사망 연출(IsDead)이 아예 안 뜬다.
}

void ObjectManager::OnAttackStart(const Protocol::S_ATTACK_START& pkt)
{
	EnsureGameThread();

	std::shared_ptr<ReplicatedActor> actor = Find(pkt.objectid());

	if (actor != nullptr)
	{
		actor->ApplyAttackStart(pkt);
	}
}

void ObjectManager::OnDebugLevel(const Protocol::S_DEBUG_LEVEL& pkt)
{
	EnsureGameThread();

	if (std::shared_ptr<ServerDebugActor> actor = debugActor.lock())
	{
		actor->OnDebugLevel(pkt);
	}
}

void ObjectManager::OnDebugPath(const Protocol::S_DEBUG_PATH& pkt)
{
	EnsureGameThread();

	if (std::shared_ptr<ServerDebugActor> actor = debugActor.lock())
	{
		actor->OnDebugPath(pkt);
	}
}

void ObjectManager::OnDebugQuadtree(const Protocol::S_DEBUG_QUADTREE& pkt)
{
	EnsureGameThread();

	if (std::shared_ptr<ServerDebugActor> actor = debugActor.lock())
	{
		actor->OnDebugQuadtree(pkt);
	}
}

void ObjectManager::ForEachActor(const std::function<void(ReplicatedActor&)>& fn) const
{
	EnsureGameThread();

	for (const auto& kv : objects)
	{
		if (std::shared_ptr<ReplicatedActor> actor = kv.second.lock())
			fn(*actor);
	}
}

// ObjectInfo 하나를 개체 타입에 따라 갈라 스폰한다.
//
// 개체 타입은 objectId 상위 16비트에 들어 있고, 그 타입별 세부 종류
// (CharacterType / MonsterType / ProjectileType) 는 info 의 하위 메시지(player/monster/projectile)
// 로 온다. 각 Spawn 헬퍼가 그 종류로 ActorDataAsset 을 조회해 스프라이트를 정한다.
void ObjectManager::Spawn(const Protocol::ObjectInfo& info, bool isLocal)
{
	const uint64 objectId = info.objectid();

	// 중복 스폰. S_ENTER_ROOM과 S_SPAWN이 겹쳐 도착하면 여기서 걸린다.
	if (objects.find(objectId) != objects.end())
		return;

	if (SpawnLevel() == nullptr)
		return;

	const Protocol::ObjectType objectType = ObjectIdHandler::GetObjectType(objectId);

	std::shared_ptr<ReplicatedActor> actor;

	switch (objectType)
	{
	case Protocol::OBJECT_PLAYER:     actor = SpawnPlayer(info, isLocal); break;
	case Protocol::OBJECT_MONSTER:    actor = SpawnMonster(info);         break;
	case Protocol::OBJECT_PROJECTILE: actor = SpawnProjectile(info);      break;
	default:
		// 아직 클라이언트에 대응 타입이 없는 개체. 조용히 건너뛴다.
		return;
	}

	if (actor == nullptr)
		return;

	// SpawnActor는 액터를 추가 요청 목록에 넣고 shared_ptr을 바로 돌려준다.
	// 그래서 레벨에 실제로 올라가기 전인 지금 초기 상태를 꽂을 수 있다(BeginPlay 전).
	actor->ApplyObjectInfo(info);

	objects[objectId] = actor;
}

namespace
{
	// ActorDataAsset 은 비동기 로드라 스폰 시점에 아직 없을 수 있다(초반 몇 프레임).
	// 그때는 nullptr - 각 액터 BeginPlay 의 폴백 애니메이션이 뜬다.
	std::shared_ptr<const ActorDataAsset> GetActorData()
	{
		return Craft::AssetManager::Get().GetPrimaryAsset<ActorDataAsset>("ActorData");
	}
}

std::shared_ptr<ReplicatedActor> ObjectManager::SpawnPlayer(const Protocol::ObjectInfo& info, bool isLocal)
{
	std::shared_ptr<Level> level = SpawnLevel();

	std::shared_ptr<ReplCharacter> chara = isLocal
		? std::static_pointer_cast<ReplCharacter>(level->SpawnActor<LocalPlayer>())
		: std::static_pointer_cast<ReplCharacter>(level->SpawnActor<RemotePlayer>());

	if (const auto actorData = GetActorData())
	{
		const CharacterDataTable& chars = actorData->Characters();
		const Protocol::CharacterType type = info.player().chartype();
		chara->SetAnimName(chars.GetAnimClip(type));
		chara->SetCollisionCells(chars.GetCollisionCells(type));
	}

	return chara;
}

std::shared_ptr<ReplicatedActor> ObjectManager::SpawnMonster(const Protocol::ObjectInfo& info)
{
	std::shared_ptr<Monster> monster = SpawnLevel()->SpawnActor<Monster>();

	if (const auto actorData = GetActorData())
	{
		const MonsterDataTable& monsters = actorData->Monsters();
		const Protocol::MonsterType type = info.monster().monstertype();
		monster->SetAnimName(monsters.GetAnimClip(type));
		monster->SetCollisionCells(monsters.GetCollisionCells(type));
	}

	return monster;
}

std::shared_ptr<ReplicatedActor> ObjectManager::SpawnProjectile(const Protocol::ObjectInfo& info)
{
	std::shared_ptr<ProjectileActor> proj = SpawnLevel()->SpawnActor<ProjectileActor>();

	proj->SetProjectileType(info.projectile().projectiletype());

	return proj;
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
