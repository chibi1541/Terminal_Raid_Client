#pragma once

#include "Actor/Actor.h"
#include "Input/InputComponent.h"
#include "Protocol/Protocol.pb.h"

#include <deque>
#include <memory>
#include <unordered_map>
#include <vector>

// 서버 권위 데이터 디버그 오버레이.
//
// 화면에 겹쳐 그리는 것 외에 게임 상태에 아무 영향도 주지 않는다. ObjectManager가
// 룸 입장 때 하나 스폰하고 weak_ptr로 들고 있으며, S_DEBUG_* 패킷을 이쪽으로 넘긴다.
//
//   F4 : 서버 충돌 격자 오버레이 (막힘 셀을 빨간 체커로)
//   F5 : 길찾기 경로 / 노드 오버레이
//
// 토글을 켜고 끌 때만 C_DEBUG_CONFIG를 보낸다. 서버는 구독한 세션에게만 데이터를 흘린다.
class ServerDebugActor : public Craft::Actor
{
	TYPE_DECLARATIONS(ServerDebugActor, Craft::Actor)

public:
	ServerDebugActor() = default;

	virtual void BeginPlay() override;
	virtual void Tick(float deltaTime) override;
	virtual void Draw() override;

	// 네트워크 스레드가 게임 스레드로 넘긴 잡에서 ObjectManager가 부른다.
	void OnDebugLevel(const Protocol::S_DEBUG_LEVEL& pkt);
	void OnDebugPath(const Protocol::S_DEBUG_PATH& pkt);
	void OnDebugQuadtree(const Protocol::S_DEBUG_QUADTREE& pkt);

private:
	void OnToggleGrid();
	void OnTogglePaths();
	void OnToggleCollision();
	void SendConfig();

	void DrawGrid();
	void DrawPaths();
	void DrawCollision();

private:
	std::shared_ptr<Craft::InputComponent> inputComponent;

	bool showGrid = false;
	bool showPaths = false;
	bool showCollision = false;

	// --- 쿼드트리 + 타이밍 (S_DEBUG_QUADTREE) ---
	struct QuadNode { int minX, minY, maxX, maxY; };
	std::vector<QuadNode> quadNodes;
	uint32_t quadObjectCount = 0;
	uint32_t quadBuildMicros = 0;
	uint32_t quadCollisionMicros = 0;
	uint32_t quadServerTick = 0;

	// --- 서버 충돌 격자 (S_DEBUG_LEVEL) ---
	int gridWidth = 0;
	int gridHeight = 0;
	int gridTileSize = 0;
	std::vector<uint8_t> blocked;	// row-major, 1 = 장애물. 크기 = gridWidth * gridHeight

	// --- 길찾기 상태 (S_DEBUG_PATH), objectId 별 ---
	struct PathDebug
	{
		std::vector<Craft::Vector2> waypoints;
		std::vector<Craft::Vector2> searchNodes;
		uint32_t currentIndex = 0;
		std::deque<Craft::Vector2> trail;	// 실제 이동 궤적(클라가 매 틱 샘플)
	};
	std::unordered_map<uint64_t, PathDebug> paths;

	static constexpr size_t kTrailMax = 256;
	float trailSampleAccumSec = 0.0f;

	// 화면 한 줄 조립 버퍼(프레임마다 재할당 방지). 개행 금지.
	std::string rowScratch;
};
