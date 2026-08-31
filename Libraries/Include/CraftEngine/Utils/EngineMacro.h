#pragma once

#define NAME_SPACE_BEGIN(name)		namespace name {
#define NAME_SPACE_END				} 

#define DLLEXPORT			__declspec(dllexport)
#define DLLIMPORT			__declspec(dllimport)

#if ENGINE_BUILD_DLL
#define CRAFT_API			DLLEXPORT
#else
#define CRAFT_API			DLLIMPORT
#endif


/*---------------
	  Crash
---------------*/

#define CRASH(cause)						\
{											\
	uint32* crash = nullptr;				\
	__analysis_assume(crash != nullptr);	\
	*crash = 0xDEADBEEF;					\
}

#define ASSERT_CRASH(expr)			\
{									\
	if (!(expr))					\
	{								\
		CRASH("ASSERT_CRASH");		\
		__analysis_assume(expr);	\
	}								\
}

// ###########################
// #		Lock			 #
// ###########################

#define USE_MANY_LOCKS(count)	Craft::Lock _locks[count];
#define USE_LOCK				USE_MANY_LOCKS(1);
#define READ_LOCK_IDX(idx)		Craft::ReadLockGuard readLockGuard_##idx(_locks[idx], typeid(this).name());
#define READ_LOCK				READ_LOCK_IDX(0)
#define WRITE_LOCK_IDX(idx)		Craft::WriteLockGuard writeLockGuard_##idx(_locks[idx], typeid(this).name());
#define WRITE_LOCK				WRITE_LOCK_IDX(0)
