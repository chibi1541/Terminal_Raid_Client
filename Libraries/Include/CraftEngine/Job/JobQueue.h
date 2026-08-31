#pragma once
#include "LockQueue.h"
#include "Job.h"
#include "Memory/ObjectPool.h"

// ########################
// #		JobQueue	  #
// ########################

NAME_SPACE_BEGIN(Craft)

class CRAFT_API JobQueue : public enable_shared_from_this<JobQueue>
{
public:
	virtual ~JobQueue() { ClearJobs(); }

	// 콜백을 직접 밀어 넣음
	void DoAsync(CallBack&& callback)
	{
		Push(ObjectPool<Job>::MakeShared(std::move(callback)));
	}

	template<typename T, typename Ret, typename... Param, typename... Args>
	void DoAsync(Ret(T::*memFunc)(Param...), Args&&... args)
	{
		shared_ptr<T> owner = static_pointer_cast<T>(shared_from_this());
		Push(ObjectPool<Job>::MakeShared(owner, memFunc, std::forward<Args>(args)...));
	}

	void ClearJobs() { _jobs.Clear(); }
	void Push(shared_ptr<Job> job);
	void Execute();

private:
	// 여기에 Lock이 걸리므로 따로 lock을 잡을 필요가 없음
	LockQueue<shared_ptr<Job>>		_jobs;
	atomic<int32>					_jobCount = 0;
};

NAME_SPACE_END
