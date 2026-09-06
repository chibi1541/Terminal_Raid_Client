#pragma once

#include "Protocol/Protocol.pb.h"

#include <memory>
#include <unordered_map>

class ReplicatedActor;
class LocalPlayer;

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

	std::shared_ptr<ReplicatedActor> Find(uint64 objectId) const;
	std::shared_ptr<LocalPlayer> GetLocalPlayer() const;

	inline uint64 GetMyObjectId() const { return myObjectId; }
	inline int GetCount() const { return static_cast<int>(objects.size()); }

private:
	// 게임 쓰레드에서 불렸는지 확인한다. 모든 진입점의 첫 줄에서 부른다.
	void EnsureGameThread() const;

	// ObjectInfo 하나를 액터로 만들어 레벨에 올린다.
	// isLocal이면 LocalPlayer로, 아니면 RemotePlayer로 만든다.
	void Spawn(const Protocol::ObjectInfo& info, bool isLocal);

	// 표에 있는 액터를 전부 제거한다(룸을 나가거나 새로 들어올 때).
	void ClearAll();

private:
	// 액터의 소유권은 Level이 가진다.
	// 여기서 shared_ptr을 들면 Destroy() 뒤에도 액터가 살아남아 누수가 된다.
	std::unordered_map<uint64, std::weak_ptr<ReplicatedActor>> objects;

	// TODO 이건 여기에 박히면 안되는 정보라 GameState 클래스에 옮기기
	// 내가 조종하는 개체. S_ENTER_ROOM의 myObject에서 온다.
	uint64 myObjectId = 0;

	// BindGameThread를 부른 쓰레드. 0이면 아직 바인드 전이라 검사하지 않는다.
	uint32 gameThreadId = 0;
};
