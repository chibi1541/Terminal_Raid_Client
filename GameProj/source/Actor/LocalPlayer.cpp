#include "pch.h"
#include "LocalPlayer.h"
#include "Component/SpriteAnimatorComponent.h"
#include "Camera/CameraManager.h"
#include "Input/Input.h"
#include "Level/Level.h"
#include "Asset/AssetManager.h"
#include "Game/ActorDataAsset.h"
#include "Network/NetSend.h"
#include "Render/Renderer.h"
#include "Render/RenderLayer.h"

#include <cmath>

using namespace Craft;

namespace
{
	int32 Abs32(int32 v) { return (v < 0) ? -v : v; }

	// 서버 8방향(DirectionType) <-> 클라 입력 델타(-1/0/1) 변환.
	// ReplCharacter::DeltaFromServerDirection의 역방향이다 - 그쪽은
	// "서버가 알려준 방향"을 델타로 풀고, 이쪽은 "내가 누른 키"를 방향으로 묶는다.
	Protocol::DirectionType DirectionTypeFromInput(const Vector2& dir)
	{
		const int sx = (dir.x > 0) - (dir.x < 0);
		const int sy = (dir.y > 0) - (dir.y < 0);

		if (sx == 0 && sy == 0) return Protocol::DIR_NONE;
		if (sx == 0 && sy < 0)  return Protocol::DIR_UP;
		if (sx == 0 && sy > 0)  return Protocol::DIR_DOWN;
		if (sy == 0 && sx < 0)  return Protocol::DIR_LEFT;
		if (sy == 0 && sx > 0)  return Protocol::DIR_RIGHT;
		if (sx < 0 && sy < 0)   return Protocol::DIR_UP_LEFT;
		if (sx > 0 && sy < 0)   return Protocol::DIR_UP_RIGHT;
		if (sx < 0 && sy > 0)   return Protocol::DIR_DOWN_LEFT;
		return Protocol::DIR_DOWN_RIGHT;	// sx > 0 && sy > 0
	}
}

void LocalPlayer::BeginPlay()
{
	// TODO 나중에 서버로부터 인덱스를 받아서 해당 캐릭터 로딩하도록 수정
	animName = "Mage";

	// 주의 - super::BeginPlay()는 이 함수의 "맨 마지막"에 불러야 한다.
	//
	// Actor::BeginPlay가 그 시점의 componentList를 훑어 각 컴포넌트의 BeginPlay를 부르는데,
	// InputComponent는 자기 BeginPlay에서 handler->Register()로 InputSystem에 등록된다.
	// 그래서 super를 먼저 부르고 컴포넌트를 나중에 추가하면
	// 그 컴포넌트의 BeginPlay가 영영 불리지 않아 입력이 한 번도 들어오지 않는다.

	// 게임플레이 입력 바인딩.
	//
	// 액터가 매 틱 키를 물어보는 게 아니라, 어떤 키에 무엇을 할지만 등록한다.
	// 어떤 키가 눌렸는지 찾아서 부르는 일은 InputSystem이 한다.
	inputComponent = AddComponent<InputComponent>();
	inputComponent->SetInputPriority(InputPriority::Gameplay);

	// 이동은 눌려 있는 동안 계속 필요하므로 Held.
	// 네 방향을 따로 걸어두면 대각선 입력이 자연스럽게 합쳐진다.
	// 콘솔 좌표는 y가 아래로 갈수록 커지므로 위/아래가 반대다.
	inputComponent->BindKey('A', EInputEvent::Held, this, &LocalPlayer::OnMoveLeft);
	inputComponent->BindKey('D', EInputEvent::Held, this, &LocalPlayer::OnMoveRight);
	inputComponent->BindKey('W', EInputEvent::Held, this, &LocalPlayer::OnMoveUp);
	inputComponent->BindKey('S', EInputEvent::Held, this, &LocalPlayer::OnMoveDown);

	inputComponent->BindKey(VK_LBUTTON, EInputEvent::Pressed, this, &LocalPlayer::OnAttack);
	inputComponent->BindKey(VK_LBUTTON, EInputEvent::Held, this, &LocalPlayer::OnAttack);

	// 구르기는 누른 순간 한 번만 시작되어야 하므로 Pressed.
	inputComponent->BindKey(VK_SPACE, EInputEvent::Pressed, this, &LocalPlayer::OnRollPressed);

	// 뷰 회전도 누른 순간 한 번씩이므로 Pressed.
	// Held로 걸면 키를 꾹 누르는 동안 매 프레임 목표 각도가 밀려서 보간이 끝나지 않는다.
	inputComponent->BindKey('Q', EInputEvent::Pressed, this, &LocalPlayer::OnRotateViewLeft);
	inputComponent->BindKey('E', EInputEvent::Pressed, this, &LocalPlayer::OnRotateViewRight);

	// 롤백 디버그 마커 토글. 누를 때마다 뒤집으면 되므로 Pressed.
	inputComponent->BindKey(VK_F3, EInputEvent::Pressed, this, &LocalPlayer::OnToggleServerPositionDebug);

	// 화면의 기준점. 등록되는 순간 활성 카메라가 되어
	// 이 액터의 위치가 화면 중앙에 오도록 CameraManager가 매 프레임 뷰를 갱신한다.
	//
	// 이것도 InputComponent와 같은 이유로 super보다 앞이어야 한다.
	// CameraComponent는 자기 BeginPlay에서 CameraManager에 등록되는데,
	// 순서를 어기면 등록이 안 돼 활성 카메라가 없는 채로 뷰가 동결된다.
	// 그러면서도 Tick/Draw는 정상으로 불리기 때문에 원인을 찾기 어렵다.
	cameraComponent = AddComponent<CameraComponent>();

	// 애니메이터 셋업은 PlayerActor가 하고, 컴포넌트 BeginPlay 전파는 Actor가 한다.
	// 위에서 만든 inputComponent도 여기서 함께 BeginPlay를 받는다.
	super::BeginPlay();
}

void LocalPlayer::OnMoveLeft()
{
	inputDirection.x -= 1;
}

void LocalPlayer::OnMoveRight()
{
	inputDirection.x += 1;
}

void LocalPlayer::OnMoveUp()
{
	inputDirection.y -= 1;
}

void LocalPlayer::OnMoveDown()
{
	inputDirection.y += 1;
}

void LocalPlayer::OnAttack()
{
	isAttack = true;
}

void LocalPlayer::OnRollPressed()
{
	isRolling = true;
}

void LocalPlayer::OnToggleServerPositionDebug()
{
	showServerPositionDebug = !showServerPositionDebug;
}

void LocalPlayer::ApplyObjectInfo(const Protocol::ObjectInfo& info)
{
	super::ApplyObjectInfo(info);

	// 예측 기준점을 스폰 위치(셀 중심)로 시드한다. 서버 GameObject::SetPos와 같은 식.
	const Vector2 spawn = GetPosition();
	predFpX = ackFpX = spawn.x * MoveMath::POS_SCALE + MoveMath::POS_SCALE / 2;
	predFpY = ackFpY = spawn.y * MoveMath::POS_SCALE + MoveMath::POS_SCALE / 2;
	ackDir = info.state().dir();
	ackClientTimeMs = static_cast<uint32>(localTimeMs);
	pendingInputs.clear();

	// 스폰 시점에도 맞춰 둔다 - 안 그러면 첫 이동(첫 ack) 전까지 마커가
	// 기본값인 원점(0,0)에 떠서 실제 위치와 아무 상관없어 보인다.
	lastServerPosition = spawn;
}

void LocalPlayer::OnRotateViewLeft()
{
	// 예외 처리 - BeginPlay 전에는 컴포넌트가 없다.
	if (nullptr == cameraComponent)
	{
		return;
	}

	// 스크린 y가 아래로 갈수록 커지는 좌표계라
	// +1이 화면상 시계 방향이다. Q는 그 반대.
	cameraComponent->AddViewQuarterTurns(-1, viewRotateBlendTime);
}

void LocalPlayer::OnRotateViewRight()
{
	if (nullptr == cameraComponent)
	{
		return;
	}

	cameraComponent->AddViewQuarterTurns(1, viewRotateBlendTime);
}

Craft::EFacing LocalPlayer::ComputeWorldFacing() const
{
	CameraManager& camera = CameraManager::Get();

	// 뷰 회전 보간(0.25초) 중에는 화면 좌표가 매 프레임 미끄러진다.
	// 그 사이에 각을 다시 재면 회전이 끝날 때까지 슬롯이 계속 요동친다. 끝날 때까지 잡아둔다.
	if (camera.IsRotationBlending())
	{
		return facing;
	}

	// 구르는 동안만 방향을 잠근다.
	//
	// 구르기는 방향을 정해서 몸을 던지는 동작이라 도중에 조준을 따라가면 안 되고,
	// 끝나는 시점도 RollEnd 노티파이로 분명하게 정해져 있다.
	//
	// ★ 공격은 잠그지 않는다 ★
	// 로컬 플레이어에게 마우스는 절대 기준이다. 게다가 좌클릭은 Held로도 묶여 있어서
	// 여기에 isAttack을 넣으면 버튼을 누르고 있는 내내 방향이 얼어붙는다 -
	// 누르면 멈추고 떼면 따라오는, 갱신이 멋대로인 것처럼 보이는 움직임이 된다.
	if (isRolling)
	{
		return facing;
	}

	const Vector2 selfScreen =
		camera.WorldToScreen(GetPosition() + Vector2(0, facingAnchorOffsetY));

	// 마우스는 애초에 콘솔 셀 좌표로 들어온다.
	// "월드가 아니라 카메라 기준"이라는 규칙이 변환 없이 그대로 성립한다.
	const Vector2 mousePosition = Input::Get().GetMousePosition();

	const int deltaX = mousePosition.x - selfScreen.x;
	const int deltaY = mousePosition.y - selfScreen.y;

	// 기준점과 정확히 겹치면 각도가 정의되지 않는다. 보던 방향을 유지한다.
	if (0 == deltaX && 0 == deltaY)
	{
		return facing;
	}

	// 콘솔은 y가 아래로 커진다. -deltaY로 뒤집어야 화면 위쪽이 +90°가 되고,
	// 그래야 "60~120도는 뒷모습"이라는 규칙이 성립한다.
	// 콘솔 셀이 정사각(8x8 폰트)이라 종횡비 보정은 필요 없다.
	const float radians =
		::atan2f(-static_cast<float>(deltaY), static_cast<float>(deltaX));

	const float degrees = NormalizeDegrees(radians * 57.29578f);

	const int quarterTurns = camera.GetViewQuarterTurns();

	// 히스테리시스는 화면 슬롯 기준으로 건다 - 흔들리는 것은 커서고, 커서는 화면에 있다.
	const EFacing previousScreenSlot = RotateFacing(facing, quarterTurns);

	const EFacing screenSlot = FacingFromScreenAngleSticky(
		degrees, previousScreenSlot, facingHysteresisDegrees);

	// 화면 슬롯 -> 월드 방향. displaySlot 계산(RotateFacing(facing, k))의 역변환이다.
	// 화면 값을 그대로 들고 있으면 카메라를 돌리는 순간 캐릭터가 실제로 도는 셈이 된다.
	return RotateFacing(screenSlot, -quarterTurns);
}

void LocalPlayer::Tick(float deltaTime)
{
	// 컴포넌트(= 애니메이션 평가/재생)로 deltaTime을 전달하는 처리가 여기 들어있다.
	super::Tick(deltaTime);

	// 로컬 단조 시계 갱신. 아래 SendMoveInputIfChanged가 이 값을 C_MOVE에 싣는다.
	localTimeMs += static_cast<double>(deltaTime) * 1000.0;

	const bool isMoving = (inputDirection != Vector2::Zero);

	// 이번 프레임 입력이 확정된 시점 - 방향이 바뀌었으면 서버에 알린다.
	SendMoveInputIfChanged();

	// 좌클릭이 눌려 있고 쿨다운이 지났으면 발사 요청.
	SendAttackIfReady();

	// 예외 처리 - BeginPlay 전에는 컴포넌트가 없다.
	if (nullptr != animator)
	{
		// 애니메이션이 게임플레이에게 보내는 유일한 신호.
		//
		// 파이프라인은 "게임플레이 -> 파라미터 -> 전이 -> 클립"으로 흐르는 단방향인데,
		// 구르기 종료만은 반대 방향이 필요하다. 클립이 끝났다는 걸 여기서 듣고 조종을 되돌린다.
		// 이게 없으면 IsRolling이 계속 참이라 Any->Roll이 매번 다시 잡아채 구르기에 갇힌다.
		if (animator->HasNotify("RollEnd"))
		{
			isRolling = false;
		}

		// 게임플레이가 애니메이션에 넘기는 건 이 값들뿐이다.
		// 어떤 클립을 틀지는 상태 머신의 전이 규칙이 정한다.
		animator->GetParameters().SetFloat("speed",
			isMoving ? static_cast<float>(MoveMath::DEFAULT_MOVE_SPEED_CELLS) : 0.0f);
		animator->GetParameters().SetFloat("IsAttack", isAttack ? 1.0f : 0.0f);
		animator->GetParameters().SetFloat("IsRolling", isRolling ? 1.0f : 0.0f);

		// 좌우 반전은 여기서 하지 않는다.
		// 어느 방향 그림을 쓸지와 그걸 뒤집을지는 PlayerActor::UpdateFacing이 고른
		// 방향 슬롯이 정한다(SpriteAnimatorComponent::SetFacing 참고).
		// 여기서 SetFlipX를 부르면 다음 틱에 슬롯 값으로 덮어써져 아무 효과가 없다.
	}

	// 클라이언트 예측 - 서버 응답을 기다리지 않고 즉시 움직인다.
	//
	// 서버와 같은 고정소수점 적분식(Shared/MovementMath.h)으로, ackFp 기준점에서
	// 미확인 입력을 전부 재생한 뒤 현재까지 적분한다. 방향이 바뀌거나 하트비트 주기가
	// 되면 SendMoveInputIfChanged가 C_MOVE를 보내고, 서버의 S_MOVE_ACK를 ReconcileMove가
	// 받아 기준점을 정확한 권위 위치로 옮긴다(그 뒤 입력은 replay로 유지되어 롤백이 안 보임).
	RecomputePrediction();

	// 보정 스무딩 오프셋을 이번 프레임만큼 0으로 감쇠시킨다(smoothDurationMs에 걸쳐 소멸).
	if (smoothOffsetX != 0 || smoothOffsetY != 0)
	{
		double retain = 1.0 - static_cast<double>(deltaTime) * 1000.0 / smoothDurationMs;
		if (retain < 0.0)
		{
			retain = 0.0;
		}
		smoothOffsetX = static_cast<int32>(smoothOffsetX * retain);
		smoothOffsetY = static_cast<int32>(smoothOffsetY * retain);
		if (Abs32(smoothOffsetX) < 2) smoothOffsetX = 0;
		if (Abs32(smoothOffsetY) < 2) smoothOffsetY = 0;
	}

	SetPosition(Vector2((predFpX + smoothOffsetX) >> MoveMath::POS_SHIFT,
		(predFpY + smoothOffsetY) >> MoveMath::POS_SHIFT));

	// 이번 프레임에 모인 입력은 여기까지만 유효하다.
	// 다음 프레임의 디스패치가 이 Tick 뒤에 오므로 지금 비워도 안전하다.
	inputDirection = Vector2::Zero;
	isAttack = false;
}

void LocalPlayer::SendMoveInputIfChanged()
{
	const Protocol::DirectionType currentDirection = DirectionTypeFromInput(inputDirection);

	const bool changed = (currentDirection != lastSentDirection);

	// 방향이 그대로여도 움직이는 중이면 하트비트 주기마다 재전송한다.
	// (정지 상태는 서버가 이미 멈춰 있으니 보낼 필요 없다)
	const bool heartbeatDue =
		(currentDirection != Protocol::DIR_NONE) &&
		(localTimeMs - lastMoveSendTimeMs >= moveHeartbeatMs);

	if (!changed && !heartbeatDue)
	{
		return;
	}

	lastSentDirection = currentDirection;
	lastMoveSendTimeMs = localTimeMs;

	const uint32 seq = nextInputSeq++;
	const uint32 nowMs = static_cast<uint32>(localTimeMs);

	// 재조정 replay용으로 보관. 서버가 ack로 확인해 준 것부터 앞은 ReconcileMove가 제거한다.
	pendingInputs.push_back({ seq, currentDirection, nowMs });
	while (pendingInputs.size() > 128)	// 폭주 방어. 정상적으론 RTT 안쪽 몇 개.
	{
		pendingInputs.pop_front();
	}

	Protocol::C_MOVE pkt;
	pkt.set_inputseq(seq);
	pkt.set_clienttick(nowMs);	// 로컬 단조 시계(ms). 서버가 직전 입력과의 차이로 이동 시간을 검증/적분한다.
	pkt.set_dir(currentDirection);

	SendToServer(pkt);

	{
		char message[176];
		sprintf_s(message,
			"[LocalPlayer] C_MOVE sent - seq=%u dir=%d t=%u %s predictedPos=(%d, %d)\n",
			seq, static_cast<int>(currentDirection), nowMs,
			changed ? "change" : "heartbeat", GetPosition().x, GetPosition().y);
		::OutputDebugStringA(message);
	}
}

void LocalPlayer::SendAttackIfReady()
{
	if (isAttack == false)
	{
		return;	// 이번 프레임 좌클릭 없음
	}

	const std::shared_ptr<const ActorDataAsset> actorData =
		AssetManager::Get().GetPrimaryAsset<ActorDataAsset>("ActorData");
	if (actorData == nullptr)
	{
		return;	// 데이터 아직 로드 전
	}

	const ProjectileDataTable& projData = actorData->Projectiles();
	const Protocol::ProjectileType projType = projData.GetDefault();

	const double intervalMs = static_cast<double>(projData.GetFireIntervalMs(projType));
	if (lastAttackSendMs != 0.0 && localTimeMs - lastAttackSendMs < intervalMs)
	{
		return;	// 쿨다운
	}

	CameraManager& camera = CameraManager::Get();

	// 마우스는 화면 셀 좌표 -> 월드로 변환(카메라 회전이 여기서만 들어간다, 마우스가 원래 화면 점이라).
	const Vector2 aimCell = camera.ScreenToWorld(Input::Get().GetMousePosition());

	// 머즐 = 몸통 중심(오프셋 없음). 전방 오프셋 산수는 서버가 권위 위치로 전담한다.
	// 스프라이트가 위치 중심에 정렬돼 있어 화면 공간 보정(발밑 y-6)이 필요 없다.
	const Vector2 muzzleCell = GetPosition();

	Protocol::C_ATTACK pkt;
	pkt.mutable_aimcell()->set_x(aimCell.x);
	pkt.mutable_aimcell()->set_y(aimCell.y);
	pkt.mutable_muzzlecell()->set_x(muzzleCell.x);
	pkt.mutable_muzzlecell()->set_y(muzzleCell.y);
	pkt.set_clienttimems(static_cast<uint32>(localTimeMs));
	SendToServer(pkt);

	lastAttackSendMs = localTimeMs;

	{
		char message[160];
		sprintf_s(message,
			"[LocalPlayer] C_ATTACK sent - aim=(%d,%d) muzzle=(%d,%d)\n",
			aimCell.x, aimCell.y, muzzleCell.x, muzzleCell.y);
		::OutputDebugStringA(message);
	}
}

void LocalPlayer::ReplayInputs(int32 startFpX, int32 startFpY, uint32 startMs,
	Protocol::DirectionType startDir, uint32 endMs,
	int32& outFpX, int32& outFpY) const
{
	int32 fpX = startFpX;
	int32 fpY = startFpY;
	uint32 cursorMs = startMs;
	Protocol::DirectionType dir = startDir;

	// 서버 Room 과 같은 격자·같은 충돌 박스로 벽을 막는다. 레벨/프롭 로드 전이면
	// IsCellBlocked 가 아직 false 라 자유 이동(오늘 동작) -> 로드 후 자동으로 유효해진다.
	Level* const level = GetOwner().get();

	// 캐릭터 위치를 중심으로 한 셀 박스 판정 (= Room::IsActorBoxBlocked). 스프라이트 8x8 전체.
	auto footprintBlocked = [level](int32 centerX, int32 centerY) -> bool
	{
		if (level == nullptr)
		{
			return false;
		}

		return MoveMath::BoxBlockedCells(centerX, centerY,
			MoveMath::PLAYER_COLLISION_CELLS_WIDE, MoveMath::PLAYER_COLLISION_CELLS_HIGH,
			[level](int32 x, int32 y) { return level->IsCellBlocked(x, y); });
	};

	auto integrate = [&](Protocol::DirectionType d, uint32 fromMs, uint32 toMs)
	{
		if (toMs <= fromMs)
		{
			return;
		}

		const Vector2 unit = DeltaFromServerDirection(d);
		MoveMath::IntegrateSlide(fpX, fpY, unit.x, unit.y,
			MoveMath::DEFAULT_MOVE_SPEED_SUBUNITS, static_cast<int32>(toMs - fromMs),
			footprintBlocked);
	};

	// startDir은 startMs부터 첫 미확인 입력 직전까지 유효했던 방향이다.
	for (const PendingInput& p : pendingInputs)
	{
		if (p.clientTimeMs >= endMs)
		{
			break;	// endMs 시점엔 아직 이 입력이 적용되지 않았다.
		}
		integrate(dir, cursorMs, p.clientTimeMs);
		dir = p.dir;
		cursorMs = p.clientTimeMs;
	}
	integrate(dir, cursorMs, endMs);

	outFpX = fpX;
	outFpY = fpY;
}

void LocalPlayer::RecomputePrediction()
{
	ReplayInputs(ackFpX, ackFpY, ackClientTimeMs, ackDir,
		static_cast<uint32>(localTimeMs), predFpX, predFpY);
}

void LocalPlayer::Draw()
{
	super::Draw();

	if (showServerPositionDebug == false)
	{
		return;
	}

	// 예측 위치(GetPosition(), 스프라이트가 그려지는 자리)와 이 마커(마지막 ack 권위
	// 위치) 사이의 간격. 재조정이 제대로 물리면 이 간격은 대략 왕복 지연시간만큼의
	// 이동 거리 안에서 오르내리기만 하고 평균이 0이어야 한다 - 한쪽으로 계속
	// 커지기만 한다면 그건 진짜 드리프트(좌표 계산식이 서버와 어긋난 것).
	Renderer::Get().SubmitWorld(
		"S",
		lastServerPosition,
		Color::Purple,
		RenderLayer::WorldUI);
}

void LocalPlayer::ReconcileMove(const Protocol::S_MOVE_ACK& pkt)
{
	// 순서 역전 방어 - 서버의 lastProcessedInputSeq 방어와 대칭이다.
	// 늦게 도착한 낡은 ack로 최신 위치를 덮어쓰면 안 된다.
	if (pkt.lastprocessedinputseq() <= lastAckedInputSeq)
	{
		return;
	}

	lastAckedInputSeq = pkt.lastprocessedinputseq();

	// 보정 스무딩 기준 - 재조정 직전에 화면에 보이던 위치(예측 + 현재 스무딩 오프셋).
	const Vector2 before = GetPosition();
	const int32 shownBeforeX = predFpX + smoothOffsetX;
	const int32 shownBeforeY = predFpY + smoothOffsetY;

	// 이 ack가 대응하는 입력의 "클라 시각"을 pending에서 찾는다.
	// 서버는 그 입력의 clientTimeMs 지점까지만 정확히 적분했으므로, ack.pos는
	// 바로 그 시각의 위치다 -> 그 시각을 기준점으로 삼아야 replay가 맞물린다.
	uint32 ackedInputTimeMs = 0;
	bool found = false;
	for (const PendingInput& p : pendingInputs)
	{
		if (p.seq == pkt.lastprocessedinputseq())
		{
			ackedInputTimeMs = p.clientTimeMs;
			found = true;
			break;
		}
	}

	// 확인된 입력과 그 이전 입력을 버퍼에서 제거한다.
	while (pendingInputs.empty() == false &&
		pendingInputs.front().seq <= pkt.lastprocessedinputseq())
	{
		pendingInputs.pop_front();
	}

	// 권위 기준점 갱신. 서버가 서브유닛(posSubX/Y)을 그대로 보내주므로 셀 중앙 가정 없이
	// 정확히 앵커링한다 - 벽이 없으면 예측과 서브유닛 단위로 일치해 스냅이 사실상 no-op다.
	// 남은 미확인 입력은 RecomputePrediction의 replay로 되살아나 방향 전환 롤백이 없다.
	ackFpX = pkt.possubx();
	ackFpY = pkt.possuby();
	ackDir = pkt.dir();

	if (found)
		ackClientTimeMs = ackedInputTimeMs;
	else if (pendingInputs.empty() == false)
		ackClientTimeMs = pendingInputs.front().clientTimeMs;	// 버퍼에서 밀려남 - 남은 첫 입력 기준
	else
		ackClientTimeMs = static_cast<uint32>(localTimeMs);		// 미확인 입력 없음 - 현재로

	RecomputePrediction();

	// 보정 스무딩(B) : 재조정으로 화면 위치가 튀는 만큼을 오프셋에 실어 몇 프레임에 걸쳐
	// 0으로 감쇠시킨다(Tick). 벽 슬라이드처럼 실제 어긋남이 있을 때만 값이 실린다.
	// 텔레포트 급(smoothMaxSub 초과)이면 스무딩 없이 즉시 스냅한다.
	const int32 corrX = shownBeforeX - predFpX;
	const int32 corrY = shownBeforeY - predFpY;
	if (Abs32(corrX) <= smoothMaxSub && Abs32(corrY) <= smoothMaxSub)
	{
		smoothOffsetX = corrX;
		smoothOffsetY = corrY;
	}
	else
	{
		smoothOffsetX = 0;
		smoothOffsetY = 0;
	}

	SetPosition(Vector2((predFpX + smoothOffsetX) >> MoveMath::POS_SHIFT,
		(predFpY + smoothOffsetY) >> MoveMath::POS_SHIFT));

	lastServerPosition = Vector2(pkt.pos().x(), pkt.pos().y());

	{
		char message[208];
		sprintf_s(message,
			"[LocalPlayer] S_MOVE_ACK recv - seq=%u serverPos=(%d, %d) pred %d,%d -> %d,%d delta=(%d, %d)\n",
			pkt.lastprocessedinputseq(), pkt.pos().x(), pkt.pos().y(),
			before.x, before.y, GetPosition().x, GetPosition().y,
			GetPosition().x - before.x, GetPosition().y - before.y);
		::OutputDebugStringA(message);
	}
}
