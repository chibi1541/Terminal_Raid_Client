#include "pch.h"
#include "ProjectileActor.h"
#include "Animation/AnimationPlayer.h"
#include "Asset/AssetManager.h"
#include "Asset/AssetTypes.h"
#include "Asset/AnimationClip.h"
#include "Asset/AnimationDataAsset.h"
#include "Render/Renderer.h"
#include "Math/SymbolPalette.h"
#include "Game/ActorDataAsset.h"

#include <cmath>

using namespace Craft;

namespace
{
	constexpr float kPosSubunits = 256.0f;	// MoveMath::POS_SCALE 와 동일 (서버 서브유닛)

	// 위치 오차를 이 시간(초)에 걸쳐 0으로 당긴다. 데드레커닝 스냅 방지.
	constexpr float kCorrectionDurationSec = 0.1f;
}

ProjectileActor::ProjectileActor()
{
}

void ProjectileActor::BeginPlay()
{
	// AnimationData.xml 에서 animSet("Projectile") 이름으로 클립 모음 파일을 찾고,
	// 그 파일 안에서 animClip("Pellet") 논리 이름의 클립을 고른다.
	std::string animSet = "Projectile";
	std::string clipName = "Pellet";

	if (const auto actorData =
		Craft::AssetManager::Get().GetPrimaryAsset<ActorDataAsset>("ActorData"))
	{
		const ProjectileDataTable& projData = actorData->Projectiles();
		// 서버가 준 타입. 없으면(구버전/디버그) 기본 투사체.
		const Protocol::ProjectileType type =
			(projType != Protocol::Projectile_None) ? projType : projData.GetDefault();
		animSet = projData.GetAnimSet(type);
		clipName = projData.GetAnimClip(type);
	}

	animPlayer = std::make_shared<AnimationPlayer>();

	auto animData = AssetManager::Get().GetPrimaryAsset<AnimationDataAsset>("AnimationData");

	AssetManager::Get().LoadAsync<AnimationClipSet>(
		animData->FindClipPath(animSet).c_str(),
		[this, clipName](std::shared_ptr<const AnimationClipSet> clips)
		{
			if (nullptr == clips || clips->empty())
			{
				ASSERT_CRASH(false);
				return;
			}

			// 클립 이름으로 고른다. 로더가 facing 이 있는 클립을 "Pellet@Down" 으로 등록하므로
			// GetName() 이 아니라 GetLogicalName()("Pellet") 으로 비교한다.
			// 투사체는 방향이 안 바뀌어서 anim.xml 에 facing="Down" 하나만 있다 - 논리 이름이
			// 같은 첫 클립을 쓰면 된다. 못 찾으면 첫 클립으로 폴백.
			std::shared_ptr<const AnimationClip> chosen = clips->front();
			for (const auto& clip : *clips)
			{
				if (clip != nullptr && clip->GetLogicalName() == clipName)
				{
					chosen = clip;
					break;
				}
			}

			animPlayer->Play(chosen);
		});

	super::BeginPlay();
}

void ProjectileActor::ApplyObjectInfo(const Protocol::ObjectInfo& info)
{
	super::ApplyObjectInfo(info);

	const Protocol::CreatureState& state = info.state();

	renderX = static_cast<float>(state.pos().x());
	renderY = static_cast<float>(state.pos().y());
	velCellsX = static_cast<float>(state.velsubx()) / kPosSubunits;
	velCellsY = static_cast<float>(state.velsuby()) / kPosSubunits;
	started = true;

	SetPosition(Vector2(state.pos().x(), state.pos().y()));
}

void ProjectileActor::ApplyMove(const Protocol::MoveInfo& info)
{
	// 데드레커닝 - 속도는 유지하고 위치만 서버 권위값으로 당긴다(다음 프레임들에서).
	if (started == false)
	{
		renderX = static_cast<float>(info.possubx()) / kPosSubunits;
		renderY = static_cast<float>(info.possuby()) / kPosSubunits;
		started = true;
		return;
	}

	const float authX = static_cast<float>(info.possubx()) / kPosSubunits;
	const float authY = static_cast<float>(info.possuby()) / kPosSubunits;

	// 즉시 스냅하지 않고, 큰 차이만 잘라낸다(패킷 유실 후 복귀 등). 작은 오차는 Tick에서 흡수.
	const float dx = authX - renderX;
	const float dy = authY - renderY;
	if (dx * dx + dy * dy > 9.0f * 9.0f)	// 9셀 초과 → 순간이동
	{
		renderX = authX;
		renderY = authY;
	}
	else
	{
		// 목표를 향해 절반쯤 당긴다(부드럽게). 나머지는 다음 S_MOVE + Tick.
		renderX += dx * 0.5f;
		renderY += dy * 0.5f;
	}
}

void ProjectileActor::Tick(float deltaTime)
{
	if (started)
	{
		renderX += velCellsX * deltaTime;
		renderY += velCellsY * deltaTime;
		SetPosition(Vector2(
			static_cast<int>(std::lround(renderX)),
			static_cast<int>(std::lround(renderY))));
	}

	super::Tick(deltaTime);

	if (nullptr != animPlayer)
		animPlayer->Tick(deltaTime);
}

void ProjectileActor::Draw()
{
	if (nullptr == animPlayer || animPlayer->GetClip() == nullptr)
		return;

	const float compositePivotX = animPlayer->GetClip()->GetPivotX();
	const float compositePivotY = animPlayer->GetClip()->GetPivotY();
	const int offsetX = static_cast<int>(::floorf(compositePivotX + 0.5f));
	const int offsetY = static_cast<int>(::floorf(compositePivotY + 0.5f));

	image = animPlayer->GetCurrentSprite()->GetPixelMap();
	Renderer::Get().SubmitPixelsWorld(
		image,
		SymbolPalette::GetTable(),
		GetPosition(),
		GetSortingOrder(),
		SymbolPalette::TransparentSymbol,
		1,
		1,
		std::nullopt,
		Vector2(-offsetX, -offsetY));
}
