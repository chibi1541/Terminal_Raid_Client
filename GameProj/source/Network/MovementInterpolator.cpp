#include "pch.h"
#include "MovementInterpolator.h"

#include <cmath>

MovementInterpolator::FVec2 MovementInterpolator::VelocityFromServer(
	Protocol::DirectionType dir, int32 speedSubunitsPerSec)
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

void MovementInterpolator::Reset(Craft::Vector2 cell, uint32 serverTick, FVec2 vel)
{
	_samples.clear();
	_samples.push_back({ static_cast<float>(serverTick),
		static_cast<float>(cell.x), static_cast<float>(cell.y), vel.x, vel.y });
	_playbackTick = static_cast<float>(serverTick);
	_rendered = { static_cast<float>(cell.x), static_cast<float>(cell.y) };
	_started = true;
}

void MovementInterpolator::AddSample(uint32 serverTick, float cellX, float cellY, FVec2 vel)
{
	const float sTick = static_cast<float>(serverTick);

	// 순서 역전 방어. UDP 아니어도 재정렬은 방어해 두는 게 싸다.
	if (!_samples.empty() && sTick <= _samples.back().tick)
		return;

	// 아래 두 경우엔 "직전 샘플 -> 이 샘플" 보간이 성립하지 않는다. 스테일 샘플을 버리고
	// 재생 시계를 이 샘플 기준으로 즉시 되잡는다. (그대로 두면 Evaluate 의 드리프트 보정 항이
	// 격차 전체를 ~0.3초에 훑으면서 그동안 버퍼에 쌓인 이동 샘플을 고속 재생한다 = 한 번에 확 이동)
	//  (1) 서버가 마지막으로 알려준 게 "이 개체는 정지"(dir=NONE -> vel 0). 그 사이 실제
	//      이동이 없었으므로 보간할 게 없다. (대기 중인 AI 몬스터는 S_MOVE 를 아예 안 보낸다 /
	//      스폰 시드는 dir=NONE @ tick 0 / 추적·패트롤 종료 직후)
	//  (2) 격차가 보간+외삽 창(MAX_INTERP_GAP_TICKS)을 넘음. 이동 중이었어도 마지막 샘플이
	//      너무 낡아 블렌드 기준이 못 된다 (심한 패킷 손실. 정지 통지 패킷 유실도 여기서 흡수).
	// _started 여부와 무관하게 동작 -> 스폰 tick-0 시드도 첫 실제 S_MOVE 에서 (1)로 되잡힌다.
	if (!_samples.empty())
	{
		const Sample& last = _samples.back();
		const bool lastStationary = (last.vx == 0.0f && last.vy == 0.0f);
		const float gap = sTick - last.tick;

		if (lastStationary || gap > MAX_INTERP_GAP_TICKS)
		{
			_samples.clear();
			_samples.push_back({ sTick, cellX, cellY, vel.x, vel.y });
			_playbackTick = sTick - INTERP_DELAY_TICKS;
			_rendered = { cellX, cellY };
			_started = true;
			return;
		}
	}

	_samples.emplace_back(std::move(Sample{ sTick, cellX, cellY, vel.x, vel.y }));
	while (_samples.size() > MAX_SAMPLES)
		_samples.pop_front();

	if (!_started)
	{
		_playbackTick = sTick - INTERP_DELAY_TICKS;
		_rendered = { cellX, cellY };
		_started = true;
	}
}

void MovementInterpolator::AddSample(uint32 serverTick, Craft::Vector2 cell, FVec2 vel)
{
	AddSample(serverTick, static_cast<float>(cell.x), static_cast<float>(cell.y), vel);
}

Craft::Vector2 MovementInterpolator::Evaluate(float deltaTime)
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

MovementInterpolator::FVec2 MovementInterpolator::GetVelocity() const
{
	return _samples.empty() ? FVec2{} : FVec2{ _samples.back().vx, _samples.back().vy };
}

MovementInterpolator::FVec2 MovementInterpolator::EvaluateAt(float time) const
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
