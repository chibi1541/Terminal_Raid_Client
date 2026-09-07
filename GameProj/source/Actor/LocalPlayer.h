#pragma once

#include "ReplCharacter.h"
#include "Input/InputComponent.h"
#include "Camera/CameraComponent.h"
#include "Protocol/Protocol.pb.h"
#include "Shared/MovementMath.h"

#include <memory>
#include <deque>

// 이 클라이언트의 유저가 조종하는 플레이어.
//
// 서버가 S_ENTER_ROOM.myObject로 알려준 개체 하나만 이 타입으로 스폰된다.
// 나머지는 전부 RemotePlayer다.
//
// 이동은 클라이언트 예측 + 서버 재조정(reconciliation) 구조다.
//  - 입력이 오면 즉시 로컬에서 움직인다. 예측은 서버와 같은 고정소수점 적분식
//    (Shared/MovementMath.h)으로, 마지막 ack 위치에서 미확인 입력을 전부 재생한다.
//  - 방향이 바뀌거나 하트비트(250ms) 주기가 되면 C_MOVE(inputSeq, clientTimeMs, dir)를 보낸다.
//  - 서버는 클라 타임스탬프 차이로 "그 입력 시각까지"만 정확히 적분한 위치를 S_MOVE_ACK로
//    돌려준다. ReconcileMove가 그 위치로 기준점을 옮기고 남은 미확인 입력을 replay하므로
//    스냅해도 방향 전환 롤백이 보이지 않는다.
//
// ★ 클라는 벽 충돌을 모른다 ★ 서버(Room::IsFootprintBlocked)만 벽 판정을 한다.
// 벽 근처에서 서버가 슬라이드시키면 예측과 갈라지고, 다음 ack에서 그쪽으로 보정된다
// (이 보정은 눈에 보이는 게 정상 - replay가 흡수하는 건 "지연", 충돌은 진짜 차이).
// C_ATTACK 전송은 아직 없다(TODO로 남은 별개 사안).
class LocalPlayer : public ReplCharacter
{

	TYPE_DECLARATIONS(LocalPlayer, ReplCharacter)

public:
	LocalPlayer() = default;

	virtual void BeginPlay() override;
	virtual void Tick(float deltaTime) override;
	virtual void Draw() override;

	// 스폰 시점 위치로 디버그 마커도 같이 맞춰 둔다(첫 이동 전까지 원점에 안 뜨게).
	virtual void ApplyObjectInfo(const Protocol::ObjectInfo& info) override;

	// S_MOVE_ACK 반영 - ObjectManager가 부른다.
	void ReconcileMove(const Protocol::S_MOVE_ACK& pkt);

protected:
	// 내 캐릭터는 노란색 이름표로 구분한다.
	virtual Craft::Color GetNameColor() const override { return Craft::Color::Yellow; }

	// 마우스가 절대 기준이다.
	//
	// 이동 방향으로 정하지 않는 이유 - 이 게임에서 "보는 방향"은 조준 방향이다.
	// 뒷걸음질치며 앞을 겨누는 동작이 이동 방향 기준으로는 표현되지 않는다.
	// 원격 플레이어와 몬스터는 마우스가 없어서 이동/공격 방향으로 대신한다(RemotePlayer 참고).
	virtual Craft::EFacing ComputeWorldFacing() const override;

private:
	// 입력 바인딩 콜백.
	//
	// 여기서는 값을 세우기만 하고 실제 처리(이동, 애니메이션 파라미터)는 Tick에서 한다.
	// 엔진이 디스패치를 Tick보다 먼저 돌리므로 같은 프레임 안에서 반영된다.
	void OnMoveLeft();
	void OnMoveRight();
	void OnMoveUp();
	void OnMoveDown();
	void OnAttack();
	void OnRollPressed();

	// 롤백(서버 보정) 디버깅용 - 서버가 마지막으로 알려준 위치를 화면에 마커로 찍는다.
	void OnToggleServerPositionDebug();

	// 뷰를 90도씩 돌린다. Q가 반시계, E가 시계 방향.
	//
	// 다른 입력과 달리 값만 세우고 Tick으로 미루지 않고 여기서 바로 처리한다.
	// 이동/애니메이션과 달리 누적할 것도 없고, 카메라 상태는 CameraManager가 가진다.
	void OnRotateViewLeft();
	void OnRotateViewRight();

	// 방향이 바뀌었을 때(또는 하트비트 주기마다) C_MOVE를 보낸다("방향-홀드" 모델 -
	// 서버는 다음 C_MOVE가 올 때까지 스스로 그 방향으로 계속 이동시킨다).
	// 보낸 입력은 pendingInputs에 쌓여 재조정 시 replay된다.
	void SendMoveInputIfChanged();

	// ackFp(마지막 S_MOVE_ACK 권위 위치)에서 시작해 미확인 입력을 전부 재생하고
	// 현재 시각까지 적분해 predFp(예측 위치)를 다시 구한다. 매 프레임 + ack 수신 시 부른다.
	void RecomputePrediction();

	// 주어진 기준점(start*)에서 startDir로 시작해, endMs 시점까지 pendingInputs를 재생한다.
	// endMs를 넘어서는(또는 같은) 입력은 아직 적용하지 않는다. RecomputePrediction과
	// ReconcileMove(특정 시점의 과거 예측을 되짚을 때)가 공유한다.
	void ReplayInputs(int32 startFpX, int32 startFpY, uint32 startMs,
		Protocol::DirectionType startDir, uint32 endMs,
		int32& outFpX, int32& outFpY) const;

private:
	std::shared_ptr<Craft::InputComponent> inputComponent;

	// 이 클라이언트의 화면을 비추는 카메라.
	//
	// autoActivate 기본값이 true라 등록되는 순간 활성 카메라가 된다.
	// RemotePlayer에는 붙이지 않는다 - 화면을 비추는 건 내가 조종하는 하나뿐이다.
	std::shared_ptr<Craft::CameraComponent> cameraComponent;

	// 뷰 회전에 쓰는 보간 시간(초). 0이면 즉시 스냅이라 화면이 튄다.
	static constexpr float viewRotateBlendTime = 0.25f;

	// 마우스 각도를 재는 기준점을 액터 위치에서 얼마나 위로 올릴지(칸).
	//
	// 액터 위치는 발밑이고 스프라이트는 거기서 위로 뻗어 있다. 발밑을 기준으로 각을 재면
	// 캐릭터의 "가슴"보다 아래에 원점이 놓여서, 커서를 캐릭터 몸통 위에 얹어도
	// 아래쪽(앞모습) 섹터로 계산된다. 몸 한가운데로 올려야 화면에서 보이는 대로 맞는다.
	static constexpr int facingAnchorOffsetY = -4;

	// 섹터 경계에서 방향을 유지하는 여유각(도).
	//
	// 없으면 커서가 경계에 걸쳐 있을 때 1칸 흔들림에도 앞뒤 그림이 매 프레임 교차한다.
	static constexpr float facingHysteresisDegrees = 8.0f;

	// --- 클라이언트 예측 & 서버 재조정(reconciliation) ---
	//
	// 예측 위치는 서버와 같은 1/256 셀 고정소수점으로 굴린다(적분식은 Shared/MovementMath.h).
	// 렌더/충돌이 보는 정수 셀 위치(SetPosition)는 매 프레임 predFp >> POS_SHIFT로 파생한다.
	int32 predFpX = 0;
	int32 predFpY = 0;

	// 마지막 S_MOVE_ACK가 준 권위 상태 = replay의 기준점.
	// ack.pos는 "그 입력을 만든 클라 시각(ackClientTimeMs)"의 위치다 - 서버가 클라
	// 타임스탬프 차이로 정확히 그 지점까지만 적분하므로. 그래서 스냅해도 그 뒤 입력은
	// replay로 되살아나 방향 전환 롤백이 보이지 않는다.
	int32 ackFpX = 0;
	int32 ackFpY = 0;
	Protocol::DirectionType ackDir = Protocol::DIR_NONE;
	uint32 ackClientTimeMs = 0;

	// 아직 ack되지 않은 입력들. 방향이 바뀐 순간(또는 하트비트)마다 하나씩 쌓인다.
	struct PendingInput
	{
		uint32 seq;
		Protocol::DirectionType dir;
		uint32 clientTimeMs;	// 이 입력을 만든 시점의 localTimeMs
	};
	std::deque<PendingInput> pendingInputs;

	// 방향을 안 바꾸고 계속 눌러도 이 주기로 C_MOVE를 재전송한다. 서버가 최소 이 간격으로
	// 정확한 ack를 주므로 예측 드리프트가 이 시간 이상 누적되지 않는다. 서버의 실측 시간
	// 창(heldMsServer)도 촘촘하게 유지되어 이동량 검증 해상도가 확보된다.
	static constexpr double moveHeartbeatMs = 250.0;
	double lastMoveSendTimeMs = 0.0;

	// 보정 스무딩(B). 재조정으로 화면 위치가 튀는 만큼(서브유닛)을 여기 담아 몇 프레임에
	// 걸쳐 0으로 감쇠시킨다. 서버가 서브유닛 위치를 정확히 주므로 벽이 없으면 이 값은
	// 거의 0이고, 벽 슬라이드 같은 실제 보정만 부드럽게 흡수된다.
	int32 smoothOffsetX = 0;
	int32 smoothOffsetY = 0;
	static constexpr double smoothDurationMs = 150.0;			// 이 시간에 걸쳐 오프셋을 0으로
	static constexpr int32 smoothMaxSub = MoveMath::POS_SCALE * 8;	// 8칸 초과 보정은 텔레포트 - 즉시 스냅

	// 구르기 상태.
	// 스페이스로 켜지고, 구르기 클립의 RollEnd 노티파이를 받으면 꺼진다.
	bool isRolling = false;

	// 이번 프레임에 모인 입력. Tick 끝에서 리셋한다.
	Craft::Vector2 inputDirection = Craft::Vector2::Zero;
	bool isAttack = false;

	// 마지막으로 서버에 보낸 방향. 초기값 DIR_NONE과 실제 첫 이동 입력이
	// 다르므로 별도 sentinel 없이 자연스럽게 "변경"으로 감지된다.
	Protocol::DirectionType lastSentDirection = Protocol::DIR_NONE;

	// 서버로 보내는 입력에 붙이는 증가 시퀀스. 0은 쓰지 않는다
	// (서버가 inputSeq==0을 디버그 호출로 특별 취급한다).
	uint32 nextInputSeq = 1;

	// 마지막으로 반영한 ack의 시퀀스. 순서가 뒤바뀌어 도착한 낡은 ack를
	// 걸러낸다 - 서버의 lastProcessedInputSeq 역전 방어와 대칭이다.
	uint32 lastAckedInputSeq = 0;

	// C_MOVE.clientTick에 실어 보내는 로컬 단조 시계(ms).
	// 서버(Room::HandleMove)가 이 값의 "차이"를 자기 실측 시간과 대조해
	// 클라가 이동 시간을 부풀렸는지 검증한다 - 프레임 카운트가 아니라 실제 경과 ms여야 한다.
	double localTimeMs = 0.0;

	// 디버그: 서버가 마지막 S_MOVE_ACK로 알려준 위치(= 예측 보정이 스냅하는 목표).
	// 내 캐릭터의 실제 위치(예측)와 이 마커 사이의 간격이 곧 "롤백 크기"다.
	// F3로 켜고 끈다.
	Craft::Vector2 lastServerPosition = Craft::Vector2::Zero;
	bool showServerPositionDebug = false;
};
