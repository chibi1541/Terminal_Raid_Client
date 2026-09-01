#include "pch.h"
#include "PlayerActor.h"

#include "Asset/AssetManager.h"
#include "Asset/AnimationDataAsset.h"
#include "Render/Renderer.h"

using namespace Craft;

namespace
{
	// AnimationData.xml의 조회 키.
	//
	// TODO : chartype -> 애셋 이름 매핑.
	// 서버가 PlayerInfo.chartype을 아직 채우지 않아서(항상 CHARACTER_NONE)
	// 지금은 모든 플레이어가 같은 애셋을 쓴다.
	const char* playerAnimAssetName = "TestActor";

	// 이름표를 캐릭터 위 몇 칸에 띄울지.
	// 스프라이트가 위치를 기준으로 위로 뻗어 있어서 그 위로 더 올린다.
	const int nameLabelOffsetY = -9;
}

void PlayerActor::BeginPlay()
{
	animator = AddComponent<SpriteAnimatorComponent>();

	auto animData = AssetManager::Get().GetPrimaryAsset<AnimationDataAsset>("AnimationData");

	// 클립 로드는 워커 쓰레드가 한다. 여기서는 요청만 걸고 바로 다음 줄로 넘어간다.
	// 클립이 도착할 때까지 몇 프레임 동안은 캐릭터가 안 보이지만 게임은 멈추지 않는다.
	const std::wstring stateMachinePath = animData->FindStateMachinePath(playerAnimAssetName);

	// 콜백이 도착하기 전에 액터가 파괴될 수 있다(디스폰이 먼저 오는 경우).
	// 그때는 아무것도 하지 않고 조용히 빠져나가야 한다.
	std::weak_ptr<Actor> weakSelf = weak_from_this();

	animator->LoadClipsFromFileAsync(animData->FindClipPath(playerAnimAssetName).c_str(),
		[weakSelf, stateMachinePath](int loadedClipCount)
		{
			std::shared_ptr<PlayerActor> self = Cast<PlayerActor>(weakSelf.lock());

			if (nullptr == self)
			{
				return;
			}

			// 애셋을 못 읽으면 아무것도 안 그려져서 원인을 찾기 어렵다. 여기서 바로 잡는다.
			// 이 콜백은 게임 쓰레드에서 불리므로 여기서 터지면 콜스택이 그대로 읽힌다.
			ASSERT_CRASH(loadedClipCount > 0);

			// 상태의 clip 이름을 검증하기 때문에 클립을 먼저 읽어야 한다.
			const int loadedLayerCount = self->animator->LoadStateMachineFromFile(stateMachinePath.c_str());
			ASSERT_CRASH(loadedLayerCount > 0);
		});

	// scaleX/scaleY로 셀 비율 보정 + 크기 조절.
	animator->SetScale(1, 1);

	super::BeginPlay();
}

void PlayerActor::ApplyObjectInfo(const Protocol::ObjectInfo& info)
{
	super::ApplyObjectInfo(info);

	// 플레이어가 아닌 개체에는 player 필드가 비어 있다.
	// 그 경우 has_player()가 false라 이름을 덮어쓰지 않는다.
	if (info.has_player())
	{
		playerName = info.player().name();
	}
}

void PlayerActor::Draw()
{
	super::Draw();

	if (playerName.empty())
	{
		return;
	}

	// 액터는 월드 객체이므로 월드 좌표로 제출한다. 카메라가 화면 좌표로 옮긴다.
	// 정렬 순서를 한 칸 올려서 스프라이트에 가리지 않게 한다.
	Renderer::Get().SubmitWorld(
		playerName,
		GetPosition() + Vector2(0, nameLabelOffsetY),
		GetNameColor(),
		GetSortingOrder() + 1);
}
