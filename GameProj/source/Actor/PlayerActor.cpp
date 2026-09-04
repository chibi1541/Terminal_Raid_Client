#include "pch.h"
#include "PlayerActor.h"

#include "Asset/AssetManager.h"
#include "Asset/AnimationDataAsset.h"
#include "Camera/CameraManager.h"
#include "Render/Renderer.h"

using namespace Craft;

namespace
{
	// AnimationData.xml의 조회 키.
	//
	// TODO : chartype -> 애셋 이름 매핑.
	// 서버가 PlayerInfo.chartype을 아직 채우지 않아서(항상 CHARACTER_NONE)
	// 지금은 모든 플레이어가 같은 애셋을 쓴다.
	const char* playerAnimAssetName = "Knight";

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

void PlayerActor::Tick(float deltaTime)
{
	// ★ super::Tick보다 먼저다 ★
	// Actor::Tick이 컴포넌트를 돌리고, 애니메이터가 거기서 이번 프레임의 그림을 확정한다.
	// 뒤에 두면 방향이 언제나 한 프레임 늦게 반영되어 커서를 빠르게 돌릴 때 어긋나 보인다.
	//
	// 노티파이(HasNotify)는 정확히 반대로 super::Tick '뒤'에 읽어야 한다는 점에 주의.
	// 한쪽은 애니메이션에 넣는 값이고 다른 쪽은 애니메이션이 내놓은 값이라 방향이 반대다.
	UpdateFacing();

	super::Tick(deltaTime);
}

void PlayerActor::UpdateFacing()
{
	facing = ComputeWorldFacing();

	// 월드 방향을 화면 슬롯으로. 카메라를 k번 돌리면 화면에서도 그만큼 같이 돈다.
	// (StaticPropActor::OnViewRotationChanged와 같은 변환이다)
	displaySlot = RotateFacing(facing, CameraManager::Get().GetViewQuarterTurns());

	// 예외 처리 - BeginPlay 전에는 컴포넌트가 없다.
	if (nullptr != animator)
	{
		animator->SetFacing(displaySlot);
	}
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
