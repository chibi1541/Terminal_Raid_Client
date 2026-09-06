#include "pch.h"
#include "ReplCharacter.h"
#include "Component/SpriteAnimatorComponent.h"
#include "Asset/AssetManager.h"
#include "Asset/AnimationDataAsset.h"
#include "Camera/CameraManager.h"
#include "Render/Renderer.h"
#include "Render/RenderLayer.h"

#include <algorithm>
#include <cmath>

using namespace Craft;

void ReplCharacter::BeginPlay()
{
	animator = AddComponent<SpriteAnimatorComponent>();

	auto animData = AssetManager::Get().GetPrimaryAsset<AnimationDataAsset>("AnimationData");

	if (animName.empty() == false)
	{
		// 클립 로드는 워커 쓰레드가 한다. 여기서는 요청만 걸고 바로 다음 줄로 넘어간다.
		// 클립이 도착할 때까지 몇 프레임 동안은 캐릭터가 안 보이지만 게임은 멈추지 않는다.
		const std::wstring stateMachinePath = animData->FindStateMachinePath(animName);

		// 콜백이 도착하기 전에 액터가 파괴될 수 있다(디스폰이 먼저 오는 경우).
		// 그때는 아무것도 하지 않고 조용히 빠져나가야 한다.
		std::weak_ptr<Actor> weakSelf = weak_from_this();

		animator->LoadClipsFromFileAsync(animData->FindClipPath(animName).c_str(),
			[weakSelf, stateMachinePath](int loadedClipCount)
			{
				std::shared_ptr<ReplCharacter> self = Cast<ReplCharacter>(weakSelf.lock());

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
	}




	super::BeginPlay();
}

void ReplCharacter::Tick(float deltaTime)
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

void ReplCharacter::UpdateFacing()
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

Vector2 ReplCharacter::DeltaFromServerDirection(Protocol::DirectionType dir)
{
	switch (dir)
	{
	case Protocol::DIR_LEFT:       return Vector2(-1, 0);
	case Protocol::DIR_RIGHT:      return Vector2(1, 0);
	case Protocol::DIR_UP:         return Vector2(0, -1);
	case Protocol::DIR_DOWN:       return Vector2(0, 1);
	case Protocol::DIR_UP_LEFT:    return Vector2(-1, -1);
	case Protocol::DIR_UP_RIGHT:   return Vector2(1, -1);
	case Protocol::DIR_DOWN_LEFT:  return Vector2(-1, 1);
	case Protocol::DIR_DOWN_RIGHT: return Vector2(1, 1);
	default:                       return Vector2::Zero;	// DIR_NONE(정지).
	}
}

EFacing ReplCharacter::FacingFromServerDirection(Protocol::DirectionType dir, EFacing previous)
{
	if (dir == Protocol::DIR_NONE)
	{
		return previous;	// 판단 근거가 없다 - 보던 방향 유지.
	}

	// FacingFromDelta는 절댓값 비교로 우세한 축을 고르고, 정확히 같으면(대각선)
	// previous를 유지한다. 대각 방향들이 축 성분 크기가 항상 같도록(±1,±1)
	// 델타를 구성해서 그 규칙을 그대로 물려받는다.
	return FacingFromDelta(DeltaFromServerDirection(dir), previous);
}

void ReplCharacter::ApplyObjectInfo(const Protocol::ObjectInfo& info)
{
	super::ApplyObjectInfo(info);

	hp = info.state().hp();
	maxHp = info.state().maxhp();

	// 플레이어가 아닌 개체에는 player 필드가 비어 있다.
	// 그 경우 has_player()가 false라 이름을 덮어쓰지 않는다.
	if (info.has_player())
	{
		characterName = info.player().name();
	}
}

void ReplCharacter::ApplyHit(const Protocol::S_HIT& pkt)
{
	// 데미지 계산은 서버 몫이다. 클라는 결과(newHp)를 그대로 반영만 한다.
	hp = pkt.newhp();
}

void ReplCharacter::ApplyDeath(const Protocol::S_DEATH& pkt)
{
	hp = 0;
}

void ReplCharacter::Draw()
{
	super::Draw();

	if (characterName.empty() == false)
	{
		// 이름표 - 화면 공간 오프셋으로 빌보드 처리한다(뷰가 회전해도 머리 위에 고정).
		// 정렬 순서를 한 칸 올려서 스프라이트에 가리지 않게 한다.
		Renderer::Get().SubmitWorld(
			characterName,
			GetPosition(),
			GetNameColor(),
			GetSortingOrder() + 1,
			std::nullopt,
			std::nullopt,
			Vector2(0, nameTagScreenOffsetY));
	}

	// 체력바 - 발밑 아래 가운데 정렬, WorldUI 대역(액터보다 위, 뷰포트 UI보다 아래).
	// 배경색 블록(SubmitPixelsWorld)으로 그려서 텍스트 색상보다 굵고 또렷하게 보이게 한다.
	if (maxHp > 0)
	{
		const float fraction = std::clamp(static_cast<float>(hp) / static_cast<float>(maxHp), 0.0f, 1.0f);
		const int filledCount = static_cast<int>(std::lround(fraction * hpBarWidth));

		const Color barColor =
			(fraction > 0.5f) ? Color::Green :
			(fraction > 0.25f) ? Color::Yellow : Color::Red;

		// 캐릭터 발밑(x=0) 기준으로 좌우 가운데 오도록 왼쪽 끝을 절반만큼 당긴다.
		const Vector2 hpBarScreenOffset(-hpBarWidth / 2, hpBarScreenOffsetY);

		std::string hpBarPixelMap(hpBarWidth, 'E');
		hpBarPixelMap.replace(0, filledCount, filledCount, 'F');

		const std::unordered_map<char, Color> hpBarPalette =
		{
			{ 'F', barColor },
			{ 'E', Color::DarkGray },
		};

		Renderer::Get().SubmitPixelsWorld(
			hpBarPixelMap,
			hpBarPalette,
			GetPosition(),
			RenderLayer::WorldUI,
			'\0',	// 투명 취급할 기호 없음 - 빈 칸도 어두운 배경 블록으로 채운다.
			1, 1,
			std::nullopt,
			hpBarScreenOffset);
	}
}
