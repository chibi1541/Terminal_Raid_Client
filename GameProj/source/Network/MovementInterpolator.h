#pragma once

#include "Protocol/Struct.pb.h"

#include <deque>

// 서버 위치 스냅샷을 모아 "지금보다 살짝 과거" 시점을 재생한다.
//
// 렌더 좌표계가 정수 칸이라 내부만 float로 굴리고 출력은 반올림한다.
// 서버가 권위이므로 이 계산은 순수하게 "보여주기"용 (결정성 불필요).
//
// RemotePlayer / Monster / Projectile 이 각자 하나씩 들고 쓴다.
//
// 정의부는 MovementInterpolator.cpp. 여기는 선언 + 상수 + 자명한 접근자만.

class MovementInterpolator
{
public:
	static constexpr float TICKS_PER_SEC = 20.0f;  // 서버 Tick() = 20Hz, 1틱 50ms
	static constexpr float INTERP_DELAY_TICKS = 4.0f;   // 재생을 최신 수신 틱보다 200ms 뒤에서
	static constexpr float MAX_EXTRAPOLATE_TICKS = 6.0f;  // 패킷 끊기면 이만큼만 외삽하고 정지

	// 직전 샘플과 이 이상 벌어지면 "그 사이를 보간"이 성립하지 않는다 - 새 샘플 기준으로
	// 재생 시계를 즉시 되잡는다 (AddSample 참고). 이동 중이었어도 마지막 샘플이 너무 낡음.
	static constexpr float MAX_INTERP_GAP_TICKS = INTERP_DELAY_TICKS + MAX_EXTRAPOLATE_TICKS;

	static constexpr size_t MAX_SAMPLES = 32;
	static constexpr float POS_SUBUNITS = 256.0f; // 서버 speed 단위: 서브유닛/초 (1칸=256, 1<<8)
	static constexpr float DIAG = 0.70710678f;

	struct FVec2
	{
		float x = 0.0f;
		float y = 0.0f;
	};

	// 서버 DirectionType + speed(서브유닛/초) -> 칸/초 속도벡터.
	// y는 아래로 증가 (서버·클라 동일). 대각은 축당 1/√2.
	static FVec2 VelocityFromServer(Protocol::DirectionType dir, int32 speedSubunitsPerSec);

	// 스폰 / 재입장의 경우 => 보간 없이 그 자리에 놓고 시작한다.
	void Reset(Craft::Vector2 cell, uint32 serverTick, FVec2 vel);

	// S_MOVE 패킷 정보를 Sample로 저장. 위치는 셀 단위(소수 허용) - 서버가 보낸
	// 서브유닛 좌표를 256으로 나눠 넘기면 셀 내부 위치까지 보존된다.
	void AddSample(uint32 serverTick, float cellX, float cellY, FVec2 vel);

	// 정수 셀 좌표 편의 오버로드 (스폰 스냅샷 등 소수부가 없는 경우).
	void AddSample(uint32 serverTick, Craft::Vector2 cell, FVec2 vel);

	// 매 프레임. dt(초). 그릴 칸 좌표를 돌려준다.
	Craft::Vector2 Evaluate(float deltaTime);

	// 현재 이동 방향 (facing 계산용). 최신 스냅샷의 속도.
	FVec2 GetVelocity() const;

	// Reset()이 한 번도 안 불렸으면 false.
	//
	// 서버 스폰(ApplyObjectInfo)을 거치지 않고 로컬에서 직접 SetPosition으로 배치한
	// 액터(테스트용 임시 스폰 등)는 이 값이 계속 false다. Tick에서 이 값을 확인하지
	// 않고 매 프레임 Evaluate() 결과로 SetPosition을 덮으면, 시작하지 않은 보간기가
	// 기본값 (0,0)을 돌려주면서 그런 액터를 원점으로 순간이동시켜 버린다.
	bool IsStarted() const { return _started; }

private:
	struct Sample
	{
		float tick = 0.f;
		float x = 0.f;
		float y = 0.f;
		float vx = 0.f;	// vector x
		float vy = 0.f; // vector y
	};

	FVec2 EvaluateAt(float time) const;

private:
	std::deque<Sample> _samples;
	float _playbackTick = 0.0f;
	FVec2 _rendered = {};
	bool  _started = false;
};
