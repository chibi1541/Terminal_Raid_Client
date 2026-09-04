#include "pch.h"
#include "ProjectileActor.h"
#include "Animation/AnimationPlayer.h"
#include "Asset/AssetManager.h"
#include "Asset/AssetTypes.h"
#include "Asset/AnimationClip.h"
#include "Render/Renderer.h"
#include "Math/SymbolPalette.h"
#include "Asset/AnimationDataAsset.h"

ProjectileActor::ProjectileActor()
{
}

void ProjectileActor::BeginPlay()
{
	animName = "TestProjectile";
	animPlayer = std::make_shared<Craft::AnimationPlayer>();

	auto animData = Craft::AssetManager::Get().GetPrimaryAsset<Craft::AnimationDataAsset>("AnimationData");


	Craft::AssetManager::Get().LoadAsync<Craft::AnimationClipSet>(animData->FindClipPath(animName).c_str(),
		[this](std::shared_ptr<const Craft::AnimationClipSet> clips)
		{
			if (nullptr == clips)
			{
				ASSERT_CRASH(false);
				return;
			}

			animPlayer->Play(clips->front());
		});

	super::BeginPlay();
}

void ProjectileActor::Tick(float deltaTime)
{
	super::Tick(deltaTime);

	if (nullptr == animPlayer)
		return;

	animPlayer->Tick(deltaTime);
}

void ProjectileActor::Draw()
{
	if (animPlayer->GetClip() == nullptr)
		return;

	float compositePivotX = animPlayer->GetClip()->GetPivotX();
	float compositePivotY = animPlayer->GetClip()->GetPivotY();
	const int offsetX = static_cast<int>(::floorf(compositePivotX + 0.5f));
	const int offsetY = static_cast<int>(::floorf(compositePivotY + 0.5f));

	image = animPlayer->GetCurrentSprite()->GetPixelMap();
	Craft::Renderer::Get().SubmitPixelsWorld(
		image,
		Craft::SymbolPalette::GetTable(),
		GetPosition(),
		GetSortingOrder(),
		Craft::SymbolPalette::TransparentSymbol,
		1,
		1,
		std::nullopt,
		Craft::Vector2(-offsetX, -offsetY));
}
