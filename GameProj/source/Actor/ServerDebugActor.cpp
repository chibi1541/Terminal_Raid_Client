#include "pch.h"
#include "ServerDebugActor.h"

#include "Actor/ReplicatedActor.h"
#include "Camera/CameraManager.h"
#include "Engine/Engine.h"
#include "Game/ObjectManager.h"
#include "Math/ViewTransform.h"
#include "Network/NetSend.h"
#include "Render/Renderer.h"
#include "Render/RenderLayer.h"

#include <cmath>

using namespace Craft;

namespace
{
	// 격자 오버레이 심볼 (SubmitPixels 배경색 채우기).
	constexpr char kBlockSymbol = 'B';
	constexpr char kTransparent = '.';

	const std::unordered_map<char, Color>& GridPalette()
	{
		static const std::unordered_map<char, Color> table = {
			{ kBlockSymbol, Color::Red },
		};
		return table;
	}

	// 두 셀 사이를 브레젠험으로 훑으며 콜백. 경로 선을 점으로 그릴 때 쓴다.
	template <typename Fn>
	void ForEachCellOnLine(Vector2 a, Vector2 b, Fn&& fn)
	{
		int x0 = a.x, y0 = a.y;
		const int x1 = b.x, y1 = b.y;
		const int dx = ::abs(x1 - x0);
		const int dy = -::abs(y1 - y0);
		const int sx = (x0 < x1) ? 1 : -1;
		const int sy = (y0 < y1) ? 1 : -1;
		int err = dx + dy;

		for (int guard = 0; guard < 4096; ++guard)
		{
			fn(Vector2(x0, y0));
			if (x0 == x1 && y0 == y1)
				break;
			const int e2 = 2 * err;
			if (e2 >= dy) { err += dy; x0 += sx; }
			if (e2 <= dx) { err += dx; y0 += sy; }
		}
	}
}

void ServerDebugActor::BeginPlay()
{
	// InputComponent 등록은 super보다 앞 - LocalPlayer::BeginPlay 주석의 이유와 동일
	// (super가 그 시점의 컴포넌트 목록만 훑어 BeginPlay를 전파한다).
	inputComponent = AddComponent<InputComponent>();
	inputComponent->SetInputPriority(InputPriority::Gameplay);
	inputComponent->BindKey(VK_F4, EInputEvent::Pressed, this, &ServerDebugActor::OnToggleGrid);
	inputComponent->BindKey(VK_F5, EInputEvent::Pressed, this, &ServerDebugActor::OnTogglePaths);
	inputComponent->BindKey(VK_F6, EInputEvent::Pressed, this, &ServerDebugActor::OnToggleCollision);

	shouldDraw = true;
	super::BeginPlay();
}

void ServerDebugActor::OnToggleGrid()
{
	showGrid = !showGrid;
	SendConfig();

	char msg[96];
	sprintf_s(msg, "[ServerDebug] grid overlay %s\n", showGrid ? "ON" : "off");
	::OutputDebugStringA(msg);
}

void ServerDebugActor::OnTogglePaths()
{
	showPaths = !showPaths;
	if (showPaths == false)
		paths.clear();
	SendConfig();

	char msg[96];
	sprintf_s(msg, "[ServerDebug] path overlay %s\n", showPaths ? "ON" : "off");
	::OutputDebugStringA(msg);
}

void ServerDebugActor::OnToggleCollision()
{
	showCollision = !showCollision;
	if (showCollision == false)
		quadNodes.clear();
	SendConfig();

	char msg[96];
	sprintf_s(msg, "[ServerDebug] collision overlay %s\n", showCollision ? "ON" : "off");
	::OutputDebugStringA(msg);
}

void ServerDebugActor::SendConfig()
{
	Protocol::C_DEBUG_CONFIG pkt;
	pkt.set_wantlevelgrid(showGrid);
	pkt.set_wantpaths(showPaths);
	pkt.set_wantquadtree(showCollision);
	SendToServer(pkt);
}

void ServerDebugActor::OnDebugQuadtree(const Protocol::S_DEBUG_QUADTREE& pkt)
{
	quadNodes.clear();
	quadNodes.reserve(pkt.nodes_size());
	for (const Protocol::DebugRect& r : pkt.nodes())
		quadNodes.push_back({ r.minx(), r.miny(), r.maxx(), r.maxy() });

	quadObjectCount = pkt.objectcount();
	quadBuildMicros = pkt.buildmicros();
	quadCollisionMicros = pkt.collisionmicros();
	quadServerTick = pkt.servertick();
}

void ServerDebugActor::OnDebugLevel(const Protocol::S_DEBUG_LEVEL& pkt)
{
	const int w = static_cast<int>(pkt.width());
	const int h = static_cast<int>(pkt.height());
	if (w <= 0 || h <= 0)
		return;

	// 첫 청크(startRow == 0)에서 버퍼를 새로 잡는다. 이후 청크는 덧칠.
	if (pkt.startrow() == 0 || gridWidth != w || gridHeight != h ||
		blocked.size() != static_cast<size_t>(w) * h)
	{
		gridWidth = w;
		gridHeight = h;
		blocked.assign(static_cast<size_t>(w) * h, 0);
	}
	gridTileSize = static_cast<int>(pkt.tilesize());

	const int startRow = static_cast<int>(pkt.startrow());
	const int rowCount = static_cast<int>(pkt.rowcount());
	const int64_t chunkCells = static_cast<int64_t>(w) * rowCount;
	const std::string& bits = pkt.blockedbits();

	for (int64_t i = 0; i < chunkCells; ++i)
	{
		const size_t byteIndex = static_cast<size_t>(i >> 3);
		if (byteIndex >= bits.size())
			break;
		if ((static_cast<uint8_t>(bits[byteIndex]) & (1u << (i & 7))) != 0)
		{
			const size_t globalIndex = static_cast<size_t>(startRow) * w + i;
			if (globalIndex < blocked.size())
				blocked[globalIndex] = 1;
		}
	}

	char msg[144];
	sprintf_s(msg, "[ServerDebug] level grid %d x %d (tile %d) chunk rows %d..%d\n",
		gridWidth, gridHeight, gridTileSize, startRow, startRow + rowCount - 1);
	::OutputDebugStringA(msg);
}

void ServerDebugActor::OnDebugPath(const Protocol::S_DEBUG_PATH& pkt)
{
	const uint64_t id = pkt.objectid();

	if (pkt.cleared())
	{
		paths.erase(id);
		return;
	}

	PathDebug& pd = paths[id];
	pd.currentIndex = pkt.currentindex();

	pd.waypoints.clear();
	pd.waypoints.reserve(pkt.waypoints_size());
	for (const Protocol::DebugPathNode& n : pkt.waypoints())
		pd.waypoints.emplace_back(n.cell().x(), n.cell().y());

	// searchNodes는 goto/path 명령 때만 채워져 온다. 비어 있으면 기존 것을 지운다.
	pd.searchNodes.clear();
	pd.searchNodes.reserve(pkt.searchnodes_size());
	for (const Protocol::DebugPathNode& n : pkt.searchnodes())
		pd.searchNodes.emplace_back(n.cell().x(), n.cell().y());
}

void ServerDebugActor::Tick(float deltaTime)
{
	super::Tick(deltaTime);

	// 실제 이동 궤적 샘플링 (~50ms 간격). 경로를 추종 중인 오브젝트만.
	if (showPaths && paths.empty() == false)
	{
		trailSampleAccumSec += deltaTime;
		if (trailSampleAccumSec >= 0.05f)
		{
			trailSampleAccumSec = 0.0f;
			for (auto& kv : paths)
			{
				if (std::shared_ptr<ReplicatedActor> actor = ObjectManager::Get().Find(kv.first))
				{
					std::deque<Vector2>& trail = kv.second.trail;
					const Vector2 p = actor->GetPosition();
					if (trail.empty() || trail.back() != p)
					{
						trail.push_back(p);
						while (trail.size() > kTrailMax)
							trail.pop_front();
					}
				}
			}
		}
	}
}

void ServerDebugActor::Draw()
{
	super::Draw();

	if (CameraManager::HasInstance() == false)
		return;

	if (showGrid)
		DrawGrid();

	if (showPaths)
		DrawPaths();

	if (showCollision)
		DrawCollision();
}

void ServerDebugActor::DrawCollision()
{
	Renderer& renderer = Renderer::Get();

	// 쿼드트리 노드 - 각 노드 경계의 네 변을 셀 단위 점으로. 깊은 노드일수록 밝게.
	for (const QuadNode& n : quadNodes)
	{
		for (int x = n.minX; x <= n.maxX; ++x)
		{
			renderer.SubmitWorld("`", Vector2(x, n.minY), Color::DarkGray, RenderLayer::WorldUI);
			renderer.SubmitWorld("`", Vector2(x, n.maxY), Color::DarkGray, RenderLayer::WorldUI);
		}
		for (int y = n.minY; y <= n.maxY; ++y)
		{
			renderer.SubmitWorld("`", Vector2(n.minX, y), Color::DarkGray, RenderLayer::WorldUI);
			renderer.SubmitWorld("`", Vector2(n.maxX, y), Color::DarkGray, RenderLayer::WorldUI);
		}
	}

	// 액터 반경 원 (미드포인트). 타입별 색.
	ObjectManager::Get().ForEachActor([&renderer](ReplicatedActor& actor)
	{
		const int r = actor.GetRadius();
		if (r <= 0)
			return;

		const Vector2 c = actor.GetPosition();

		Color color = Color::Blue;
		switch (actor.GetObjectType())
		{
		case Protocol::OBJECT_PLAYER:     color = Color::Green;  break;
		case Protocol::OBJECT_MONSTER:    color = Color::Red;    break;
		case Protocol::OBJECT_PROJECTILE: color = Color::Yellow; break;
		default: break;
		}

		int x = r;
		int y = 0;
		int err = 1 - r;
		while (x >= y)
		{
			const int px[8] = { c.x + x, c.x + y, c.x - y, c.x - x, c.x - x, c.x - y, c.x + y, c.x + x };
			const int py[8] = { c.y + y, c.y + x, c.y + x, c.y + y, c.y - y, c.y - x, c.y - x, c.y - y };
			for (int i = 0; i < 8; ++i)
				renderer.SubmitWorld("o", Vector2(px[i], py[i]), color, RenderLayer::WorldUI);

			y++;
			if (err < 0)
				err += 2 * y + 1;
			else { x--; err += 2 * (y - x) + 1; }
		}
	});

	// 타이밍 수치 - 화면 고정.
	char line[160];
	sprintf_s(line,
		"[F6] quadtree  nodes %d  objs %u  |  build %u us  resolve %u us  (tick %u)",
		static_cast<int>(quadNodes.size()), quadObjectCount,
		quadBuildMicros, quadCollisionMicros, quadServerTick);
	renderer.Submit(line, Vector2(1, 1), Color::White, RenderLayer::UI);
}

void ServerDebugActor::DrawGrid()
{
	if (blocked.empty() || gridWidth <= 0 || gridHeight <= 0)
		return;

	const CameraManager& camera = CameraManager::Get();
	const Vector2 screenSize = Renderer::Get().GetScreenSize();
	const Vector2 center = camera.GetViewCenterWorld();
	const Vector2 half(screenSize.x / 2, screenSize.y / 2);

	rowScratch.resize(static_cast<size_t>(screenSize.x));

	// TileMapLevel::Draw와 같은 방식 - 화면을 순회하고 각 칸을 월드 셀로 역변환한다.
	// 회전 여부와 무관하게 지형과 정확히 겹친다.
	auto emitRow = [&](int screenY, auto&& worldAt)
	{
		bool anyOpaque = false;
		for (int screenX = 0; screenX < screenSize.x; ++screenX)
		{
			const Vector2 world = worldAt(screenX);
			const int cx = world.x;
			const int cy = world.y;

			char symbol = kTransparent;
			if (cx >= 0 && cy >= 0 && cx < gridWidth && cy < gridHeight &&
				blocked[static_cast<size_t>(cy) * gridWidth + cx] != 0 &&
				(((cx + cy) & 1) == 0))	// 체커 - 밑그림이 비쳐 보이게
			{
				symbol = kBlockSymbol;
				anyOpaque = true;
			}
			rowScratch[screenX] = symbol;
		}

		if (anyOpaque)
		{
			Renderer::Get().SubmitPixels(
				rowScratch, GridPalette(), Vector2(0, screenY),
				RenderLayer::WorldUI, kTransparent);
		}
	};

	if (camera.IsRotationBlending())
	{
		const float radians = camera.GetViewAngleDegrees() * (3.14159265358979323846f / 180.0f);
		const float viewCos = ::cosf(radians);
		const float viewSin = ::sinf(radians);

		for (int screenY = 0; screenY < screenSize.y; ++screenY)
		{
			emitRow(screenY, [&](int screenX)
			{
				return ViewScreenToWorldF(Vector2(screenX, screenY), center, half, viewCos, viewSin);
			});
		}
	}
	else
	{
		const int quarterTurns = camera.GetViewQuarterTurns();
		const int inverseTurns = (4 - (((quarterTurns % 4) + 4) % 4)) % 4;
		const Vector2 stepX = Rotate90(Vector2(1, 0), inverseTurns);

		for (int screenY = 0; screenY < screenSize.y; ++screenY)
		{
			Vector2 world = ViewScreenToWorld(Vector2(0, screenY), center, half, quarterTurns);
			emitRow(screenY, [&](int screenX)
			{
				const Vector2 w = world;
				world = world + stepX;
				return w;
			});
		}
	}
}

void ServerDebugActor::DrawPaths()
{
	Renderer& renderer = Renderer::Get();

	for (const auto& kv : paths)
	{
		const PathDebug& pd = kv.second;

		// JPS 탐색 노드 (가장 아래).
		for (const Vector2& node : pd.searchNodes)
			renderer.SubmitWorld("x", node, Color::Purple, RenderLayer::WorldUI);

		// 실제 이동 궤적.
		for (const Vector2& p : pd.trail)
			renderer.SubmitWorld(".", p, Color::Orange, RenderLayer::WorldUI);

		// 계획 경로 - 웨이포인트 사이를 점선으로 잇고, 노드를 찍는다.
		for (size_t i = 0; i + 1 < pd.waypoints.size(); ++i)
		{
			ForEachCellOnLine(pd.waypoints[i], pd.waypoints[i + 1], [&](Vector2 c)
			{
				renderer.SubmitWorld("+", c, Color::Blue, RenderLayer::WorldUI);
			});
		}

		for (size_t i = 0; i < pd.waypoints.size(); ++i)
		{
			const bool isCurrent = (i == pd.currentIndex);
			renderer.SubmitWorld(
				isCurrent ? "@" : "O",
				pd.waypoints[i],
				isCurrent ? Color::Yellow : Color::Blue,
				RenderLayer::WorldUI);
		}
	}
}
