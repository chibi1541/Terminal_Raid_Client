#pragma once

#include "ReplCharacter.h"
#include "Input/InputComponent.h"
#include "Camera/CameraComponent.h"
#include "Protocol/Protocol.pb.h"

#include <memory>

// 이 클라이언트의 유저가 조종하는 플레이어.
//
// 서버가 S_ENTER_ROOM.myObject로 알려준 개체 하나만 이 타입으로 스폰된다.
// 나머지는 전부 RemotePlayer다.
//
// 이동은 클라이언트 예측 구조다 - 입력이 오면 즉시 로컬에서 움직이고(Tick의
// 이동 계산), 방향이 바뀔 때마다 C_MOVE로 서버에 알린다. 서버는 S_MOVE_ACK로
// 그 입력을 처리한 순간의 위치를 돌려주는데, 그 값은 도착 시점엔 이미 왕복
// 지연시간만큼 과거라 ReconcileMove는 이걸로 위치를 스냅하지 않는다(스냅하면
// 그 사이 예측 이동을 지우고 되돌리는 꼴이라 방향 전환마다 롤백처럼 보인다) -
// 지금은 디버그 마커(F3)용으로만 저장해 둔다.
//
// ★ 클라는 벽 충돌을 모른다 ★ 서버(Room::IsFootprintBlocked)는 이미 벽
// 충돌 판정을 하는데 클라는 아무 검사 없이 자유롭게 움직인다 - 벽 근처에서는
// 진짜로 위치가 어긋난다. 나중에 이걸 반영하려면(예: 충돌 시 스냅) 위의
// SetPosition 스냅 문제 때문에 반드시 미확인 입력 재생(replay) 방식으로
// 붙여야 한다 - ack.pos로 그냥 스냅하면 방향 전환마다 롤백이 재현된다.
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

	// 방향이 바뀌었을 때만 C_MOVE를 보낸다("방향-홀드" 모델 - 서버는 다음
	// C_MOVE가 올 때까지 스스로 그 방향으로 계속 이동시킨다).
	void SendMoveInputIfChanged();

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

	// 초당 이동할 칸 수. 서버 DEFAULT_MOVE_SPEED_CELLS와 일치시킨다 - 어긋나면
	// 한 방향을 오래 누르고 있는 동안 서버와 계속 벌어지다가 방향을 바꿀 때마다
	// (그때만 S_MOVE_ACK가 오므로) 눈에 띄게 되돌아간다.
	float moveSpeed = 6.0f;

	// 이동 누적값. 1.0을 넘으면 한 칸 움직인다.
	// (프레임마다 무조건 한 칸씩 움직이면 프레임레이트에 따라 속도가 달라짐)
	float moveAmount = 0.0f;

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

	// C_MOVE.clientTick에 실어 보내는 로컬 틱 카운터.
	// 서버는 지금 로깅용으로만 갖고 있다(Room::HandleMove가 버림) - 정밀할 필요 없다.
	uint32 localTick = 0;

	// 디버그: 서버가 마지막 S_MOVE_ACK로 알려준 위치(= 예측 보정이 스냅하는 목표).
	// 내 캐릭터의 실제 위치(예측)와 이 마커 사이의 간격이 곧 "롤백 크기"다.
	// F3로 켜고 끈다.
	Craft::Vector2 lastServerPosition = Craft::Vector2::Zero;
	bool showServerPositionDebug = false;
};
