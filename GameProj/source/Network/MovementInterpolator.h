#pragma once

#include "Protocol/Struct.pb.h"

#include <deque>
#include <cmath>
#include <algorithm>

// 서버 위치 스냅샷을 모아 "지금보다 살짝 과거" 시점을 재생한다.
//
// 렌더 좌표계가 정수 칸이라 내부만 float로 굴리고 출력은 반올림한다.
// 서버가 권위이므로 이 계산은 순수하게 "보여주기"용 (결정성 불필요).
//
// RemotePlayer / Monster / Projectile 이 각자 하나씩 들고 쓴다.

class MovementInterpolator
{
public:
	static constexpr float TICKS_PER_SEC = 20.0f;  // 서버 Tick() = 20Hz, 1틱 50ms
	static constexpr float INTERP_DELAY_TICKS = 4.0f;   // 재생을 최신 수신 틱보다 200ms 뒤에서
	static constexpr float MAX_EXTRAPOLATE_TICKS = 6.0f;  // 패킷 끊기면 이만큼만 외삽하고 정지
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
	static FVec2 VelocityFromServer(Protocol::DirectionType dir, int32 speedSubunitsPerSec)
	{
		const float speed = static_cast<float>(speedSubunitsPerSec) / POS_SUBUNITS;
		switch (dir)
		{
		case Protocol::DIR_LEFT:       return { -speed, 0.0f };
		case Protocol::DIR_RIGHT:      return { speed, 0.0f };
		case Protocol::DIR_UP:         return { 0.0f, -speed };
		case Protocol::DIR_DOWN:       return { 0.0f,  speed };
		case Protocol::DIR_UP_LEFT:    return { -speed * DIAG, -speed * DIAG };
		case Protocol::DIR_UP_RIGHT:   return { speed * DIAG, -speed * DIAG };
		case Protocol::DIR_DOWN_LEFT:  return { -speed * DIAG,  speed * DIAG };
		case Protocol::DIR_DOWN_RIGHT: return { speed * DIAG,  speed * DIAG };
		default:                       return { 0.0f, 0.0f };
		}
	}

	// 스폰 / 재입장의 경우 => 보간 없이 그 자리에 놓고 시작한다.
	void Reset(Craft::Vector2 cell, uint32 serverTick, FVec2 vel)
	{
		_samples.clear();
		_samples.push_back({ static_cast<float>(serverTick),
			static_cast<float>(cell.x), static_cast<float>(cell.y), vel.x, vel.y });
		_playbackTick = static_cast<float>(serverTick);
		_rendered = { static_cast<float>(cell.x), static_cast<float>(cell.y) };
		_started = true;
	}

	// S_MOVE 패킷 정보를 Sample로 저장.
	void AddSample(uint32 serverTick, Craft::Vector2 cell, FVec2 vel)
	{
		const float sTick = static_cast<float>(serverTick);

		// 순서 역전 방어. UDP 아니어도 재정렬은 방어해 두는 게 싸다.
		if (!_samples.empty() && sTick <= _samples.back().tick)
			return;

		_samples.emplace_back(std::move(Sample{ sTick, static_cast<float>(cell.x), static_cast<float>(cell.y), vel.x, vel.y }));
		while (_samples.size() > MAX_SAMPLES)
			_samples.pop_front();

		if (!_started)
		{
			_playbackTick = sTick - INTERP_DELAY_TICKS;
			_rendered = { static_cast<float>(cell.x), static_cast<float>(cell.y) };
			_started = true;
		}
	}

	// 매 프레임. dt(초). 그릴 칸 좌표를 돌려준다.
	Craft::Vector2 Evaluate(float deltaTime)
	{
		if (!_started || _samples.empty())
			return Craft::Vector2(std::lround(_rendered.x), std::lround(_rendered.y));

		_playbackTick += deltaTime * TICKS_PER_SEC;

		// 재생 시계 드리프트 보정 : 목표(최신 - 지연)로 살살 당긴다. 스냅하면 튄다.
		const float newest = _samples.back().tick;
		const float target = newest - INTERP_DELAY_TICKS;
		_playbackTick += (target - _playbackTick) * fmin(1.0f, deltaTime * 3.0f);

		// 외삽 한계. 여기 걸리면 패킷이 끊긴 것 -> 마지막 위치에서 멈춘다.
		if (_playbackTick > newest + MAX_EXTRAPOLATE_TICKS)
			_playbackTick = newest + MAX_EXTRAPOLATE_TICKS;

		_rendered = EvaluateAt(_playbackTick);
		return Craft::Vector2(std::lround(_rendered.x), std::lround(_rendered.y));
	}

	// 현재 이동 방향 (facing 계산용). 최신 스냅샷의 속도.
	FVec2 GetVelocity() const { return _samples.empty() ? FVec2{} : FVec2{ _samples.back().vx, _samples.back().vy };}

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


	FVec2 EvaluateAt(float time) const
	{
		const Sample& front = _samples.front();
		if (_samples.size() == 1 || time <= front.tick)
			return { front.x, front.y };

		for (size_t i = 1; i < _samples.size(); ++i)
		{
			if (time <= _samples[i].tick)
			{
				const Sample& a = _samples[i - 1];
				const Sample& b = _samples[i];
				const float span = b.tick - a.tick;
				const float f = (span > 1e-4f) ? (time - a.tick) / span : 1.0f;
				return { a.x + (b.x - a.x) * f, a.y + (b.y - a.y) * f };  // 선형 보간
			}
		}

		// time 가 최신 스냅샷보다 미래 -> dir·speed로 외삽
		const Sample& last = _samples.back();
		const float ahead = (time - last.tick) / TICKS_PER_SEC;             // 초
		return { last.x + last.vx * ahead, last.y + last.vy * ahead };
	}


private:

	std::deque<Sample> _samples;
	float _playbackTick = 0.0f;
	FVec2 _rendered = {};
	bool  _started = false;
};

