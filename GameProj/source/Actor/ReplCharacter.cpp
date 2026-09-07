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
#include <optional>

using namespace Craft;

void ReplCharacter::BeginPlay()
{
	animator = AddComponent<SpriteAnimatorComponent>();

	auto animData = AssetManager::Get().GetPrimaryAsset<AnimationDataAsset>("AnimationData");

	const std::wstring clipPath =
		(animName.empty() == false) ? animData->FindClipPath(animName) : std::wstring();

	if (clipPath.empty() == false)
	{
		// 클립 로드는 워커 쓰레드가 한다. 여기서는 요청만 걸고 바로 다음 줄로 넘어간다.
		// 클립이 도착할 때까지 몇 프레임 동안은 캐릭터가 안 보이지만 게임은 멈추지 않는다.
		const std::wstring stateMachinePath = animData->FindStateMachinePath(animName);

		// 콜백이 도착하기 전에 액터가 파괴될 수 있다(디스폰이 먼저 오는 경우).
		// 그때는 아무것도 하지 않고 조용히 빠져나가야 한다.
		std::weak_ptr<Actor> weakSelf = weak_from_this();

		animator->LoadClipsFromFileAsync(clipPath.c_str(),
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
	else if (animName.empty() == false)
	{
		// animName 은 있는데 AnimationData.xml 에 그 이름의 클립이 없다(아직 아트 미제작 등).
		// 크래시 대신 안 그리고 넘어간다.
		char msg[128];
		sprintf_s(msg, "[ReplCharacter] no anim clip for '%s' - actor will be invisible\n", animName.c_str());
		::OutputDebugStringA(msg);
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

	// 피격 상태 해제 : Hit 클립 끝의 HitEnd 노티파이(애니메이션 흐름), 또는 노티파이 유실 대비 폴백 타임아웃.
	// (super::Tick 이 애니메이터를 돌린 뒤라야 이번 프레임 노티파이가 읽힌다)
	if (isHit)
	{
		hitFallbackSec -= deltaTime;
		if (hitFallbackSec <= 0.0f || (nullptr != animator && animator->HasNotify("HitEnd")))
			isHit = false;
	}

	// 흰색 피격 깜빡임(경직 없는 피격 연출). 현재 클립은 그대로 재생되고 색만 토글된다.
	if (hitFlashSec > 0.0f && nullptr != animator)
	{
		hitFlashSec -= deltaTime;
		if (hitFlashSec <= 0.0f)
		{
			hitFlashSec = 0.0f;
			animator->SetTint(std::nullopt);
		}
		else
		{
			// 주기의 앞 절반은 흰색, 뒤 절반은 원색.
			const float phase = std::fmod(hitFlashSec, hitFlashPeriod);
			const bool white = phase > (hitFlashPeriod * 0.5f);
			animator->SetTint(white ? std::optional<Craft::Color>(Craft::Color::White) : std::nullopt);
		}
	}
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

	// stunMs > 0 이면 피격 경직 상태로. (보스 등 경직 면역은 서버가 stunMs=0 으로 보낸다)
	// 이 창 동안 로컬 플레이어는 이동/공격 입력이 막히고, Hit 클립이 재생된다.
	if (hp > 0 && pkt.stunms() > 0)
	{
		isHit = true;
		hitFallbackSec = pkt.stunms() * 0.001f + 0.15f;	// 서버 경직 + 약간 여유. 보통 HitEnd 노티파이가 먼저 풀어준다.
	}
	else if (hp > 0)
	{
		// 경직이 없는 피격(보스 등 서버가 stunMs=0 으로 보냄) - 상태 이상 없이 흰색 깜빡임만.
		hitFlashSec = hitFlashDuration;
	}
}

void ReplCharacter::ApplyDeath(const Protocol::S_DEATH& pkt)
{
	hp = 0;
	isDead = true;
	isHit = false;

	// 사망 연출(Death 클립)은 원래 색으로 보여야 한다.
	hitFlashSec = 0.0f;
	if (nullptr != animator)
		animator->SetTint(std::nullopt);
}

void ReplCharacter::Draw()
{
	super::Draw();

	// 액터 기준점 = 몸통 중심. 발밑/머리는 여기서 collisionCells/2 만큼 떨어져 있다.
	const int halfBox = collisionCells / 2;

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
			Vector2(0, -halfBox - nameTagMarginY));
	}

	// 체력바 - 몸통(발밑) 아래 가운데 정렬, WorldUI 대역(액터보다 위, 뷰포트 UI보다 아래).
	// 배경색 블록(SubmitPixelsWorld)으로 그려서 텍스트 색상보다 굵고 또렷하게 보이게 한다.
	// 색: 남은 체력은 빨강, 닳은 부분은 검정 (초록 배경에서 초록/회색 바가 안 보여서).
	if (maxHp > 0)
	{
		const float fraction = std::clamp(static_cast<float>(hp) / static_cast<float>(maxHp), 0.0f, 1.0f);
		const int filledCount = static_cast<int>(std::lround(fraction * hpBarWidth));

		// 좌우는 가운데 정렬(왼쪽 끝을 절반 당김), 세로는 발밑(중심 + collisionCells/2) 바로 아래.
		const Vector2 hpBarScreenOffset(-hpBarWidth / 2, halfBox + hpBarMarginY);

		std::string hpBarPixelMap(hpBarWidth, 'E');
		hpBarPixelMap.replace(0, filledCount, filledCount, 'F');

		const std::unordered_map<char, Color> hpBarPalette =
		{
			{ 'F', Color::Red },
			{ 'E', Color::Black },
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
