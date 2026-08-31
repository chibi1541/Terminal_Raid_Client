#pragma once


// ########################
// #	Read-Write-Lock   #
// ########################

NAME_SPACE_BEGIN(Craft)

// Write Lock -> 하나의 쓰레드만 획득 가능(mutex)
// 다른 쓰레드가 read, write lock 중 하나라도 점유 중이라면 획득 할 수 없음
// 본인이 점유 중인 경우에 카운트를 올리는 방식으로 중복 점유가 가능하고 카운트가 모두 해제됬을 시에 Lock을 반환

// Read Lock -> 복수의 쓰레드가 동시에 점유 가능(읽기 전용 lock. 세마포어에 가까우려나?)
// Write Lock을 점유 중인 다른 쓰레드가 없다면 획득 가능, 본인이 Write Lock을 점유 중이라면 획득 가능 

class CRAFT_API Lock
{
	enum : uint32
	{
		AQUIRE_TIMEOUT_TICK		= 10'000,
		MAX_SPIN_COUNT			= 5'000,
		// 32비트 중에 상위 16비트
		// Write Lock을 점유 중인 ThreadID를 기록하기 위한 마스크
		WRITE_THREAD_MASK		= 0xFFFF'0000,
		// 32비트 중 하위 16비트
		// Read Lock을 점유하고 있는 쓰레드 수를 카운팅하기 위한 마스크
		READ_COUNT_MASK			= 0x0000'FFFF,
		EMPTY_FLAG				= 0x0000'0000
	};

public:
	void WriteLock(const char* name);
	void WriteUnlock(const char* name);
	void ReadLock(const char* name);
	void ReadUnlock(const char* name);

private:
	std::atomic<uint32> _lockFlag = EMPTY_FLAG;
	// 재귀적으로 write lock을 획득하기 위한 카운팅 변수
	// atomic 변수가 아닌 이유는 어차피 lock을 획득한 쓰레드에서만 접근할 것이므로 쓰레드 세이프하게 사용
	uint32 _writeCount = 0;
};

// Lock의 사용은 RAII 방식으로
class CRAFT_API ReadLockGuard
{
public:
	ReadLockGuard(Lock& lock, const char* name) : _lock(lock), _name(name)
	{
		_lock.ReadLock(name);
	}

	~ReadLockGuard()
	{
		_lock.ReadUnlock(_name);
	}


private:
	Lock& _lock;
	const char* _name;
};

class CRAFT_API WriteLockGuard
{
public:
	WriteLockGuard(Lock& lock, const char* name) : _lock(lock), _name(name)
	{
		_lock.WriteLock(name);
	}
	~WriteLockGuard()
	{
		_lock.WriteUnlock(_name);
	}

private:
	Lock& _lock;
	const char* _name;
};

NAME_SPACE_END

