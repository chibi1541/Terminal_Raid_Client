#pragma once

#include "Protocol/Protocol.pb.h"

#include <functional>
#include <memory>
#include <unordered_map>

class ReplicatedActor;
class LocalPlayer;
class ServerDebugActor;
class PauseMenuActor;
class CrosshairActor;

namespace Craft { class Level; }

// objectId -> 레벨에 올라간 액터를 잇는 표.
//
// 서버가 보낸 스폰/디스폰 패킷을 실제 액터 생성/제거로 옮기는 유일한 통로다.
//
// !! 이 클래스의 모든 함수는 게임(메인) 쓰레드 전용이다. !!
// 패킷 핸들러는 네트워크 쓰레드에서 돌기 때문에, 반드시
// Engine::RunOnGameThread()를 거쳐서 들어와야 한다.
// BindGameThread()가 기록해 둔 쓰레드 ID와 비교해서 위반을 즉시 잡는다.
class ObjectManager
{
public:
	static ObjectManager& Get();

	// 게임 쓰레드에서 한 번 불러 기준 쓰레드 ID를 기록한다.
	void BindGameThread();

	// 아래는 전부 패킷 핸들러가 게임 쓰레드로 넘긴 잡 안에서 호출된다.
	void OnEnterRoom(const Protocol::S_ENTER_ROOM& pkt);
	void OnExitRoom();
	void OnSpawn(const Protocol::S_SPAWN& pkt);
	void OnDespawn(const Protocol::S_DESPAWN& pkt);
	void OnMove(const Protocol::S_MOVE& pkt);
	void OnMoveAck(const Protocol::S_MOVE_ACK& pkt);
	void OnHit(const Protocol::S_HIT& pkt);
	void OnDeath(const Protocol::S_DEATH& pkt);
	void OnAttackStart(const Protocol::S_ATTACK_START& pkt);
	void OnDebugLevel(const Protocol::S_DEBUG_LEVEL& pkt);
	void OnDebugPath(const Protocol::S_DEBUG_PATH& pkt);
	void OnDebugQuadtree(const Protocol::S_DEBUG_QUADTREE& pkt);

	std::shared_ptr<ReplicatedActor> Find(uint64 objectId) const;

	// 디버그 오버레이용 - 지금 살아있는 모든 복제 액터를 훑는다 (게임 스레드 전용).
	void ForEachActor(const std::function<void(ReplicatedActor&)>& fn) const;
	std::shared_ptr<LocalPlayer> GetLocalPlayer() const;

	inline uint64 GetMyObjectId() const { return myObjectId; }
	inline int GetCount() const { return static_cast<int>(objects.size()); }

private:
	// 게임 쓰레드에서 불렸는지 확인한다. 모든 진입점의 첫 줄에서 부른다.
	void EnsureGameThread() const;

	// ObjectInfo 하나를 개체 타입(objectId 상위 16비트)에 따라 갈라 스폰한다.
	// isLocal이면 플레이어를 LocalPlayer로, 아니면 RemotePlayer로 만든다.
	void Spawn(const Protocol::ObjectInfo& info, bool isLocal);

	// 타입별 스폰. 각각 info 의 하위 메시지에서 세부 종류를 꺼내 ActorDataAsset 으로 조회한다.
	// 레벨에 올리고 shared_ptr 을 돌려준다 (ApplyObjectInfo / 표 등록은 Spawn 이 한다).
	std::shared_ptr<ReplicatedActor> SpawnPlayer(const Protocol::ObjectInfo& info, bool isLocal);
	std::shared_ptr<ReplicatedActor> SpawnMonster(const Protocol::ObjectInfo& info);
	std::shared_ptr<ReplicatedActor> SpawnProjectile(const Protocol::ObjectInfo& info);

	// 표에 있는 액터를 전부 제거한다(룸을 나가거나 새로 들어올 때).
	void ClearAll();

	// 이번 스폰이 올라갈 레벨.
	//
	// 보통은 Engine::GetLevel() 이지만, OnEnterRoom 이 MenuLevel 위에서 불리면
	// 그 프레임엔 GetLevel() 이 아직 MenuLevel 을 돌려준다(레벨 교체는 프레임 끝).
	// OnEnterRoom 이 만들어 둔 다음 레벨(TileMapLevel)이 있으면 그쪽에 스폰해야
	// 액터가 곧 버려질 MenuLevel 에 들어가지 않는다.
	std::shared_ptr<Craft::Level> SpawnLevel() const;

private:
	// 액터의 소유권은 Level이 가진다.
	// 여기서 shared_ptr을 들면 Destroy() 뒤에도 액터가 살아남아 누수가 된다.
	std::unordered_map<uint64, std::weak_ptr<ReplicatedActor>> objects;

	// 룸 입장마다 하나 스폰하는 디버그 오버레이 액터. 소유권은 Level.
	std::weak_ptr<ServerDebugActor> debugActor;

	// 룸 입장마다 하나 스폰하는 ESC 일시정지 메뉴. 소유권은 Level.
	std::weak_ptr<PauseMenuActor> pauseMenu;

	// 마우스 십자선 오버레이. 소유권은 Level.
	std::weak_ptr<CrosshairActor> crosshair;

	// OnEnterRoom 이 MenuLevel 위에서 만든 다음 레벨(TileMapLevel).
	// 그 프레임의 스폰들이 이쪽으로 가야 한다. 교체 후엔 mainLevel 이 되어 GetLevel() 과 같아진다.
	std::weak_ptr<Craft::Level> pendingSpawnLevel;

	// TODO 이건 여기에 박히면 안되는 정보라 GameState 클래스에 옮기기
	// 내가 조종하는 개체. S_ENTER_ROOM의 myObject에서 온다.
	uint64 myObjectId = 0;

	// BindGameThread를 부른 쓰레드. 0이면 아직 바인드 전이라 검사하지 않는다.
	uint32 gameThreadId = 0;
};
