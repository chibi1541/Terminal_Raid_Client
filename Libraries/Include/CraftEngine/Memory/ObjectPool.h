#pragma once
#include "MemoryPool.h"

NAME_SPACE_BEGIN(Craft)

template<typename Type>
class CRAFT_API ObjectPool
{
public:
	template<typename... Args>
	static Type* Pop(Args&&... args)
	{
		// 메모리 풀로부터 메모리 획득
		Type* memory = static_cast<Type*>(MemoryHeader::DetachHeader(s_pool.Pop(), s_allocSize));

		// 획득한 메모리 영역에 생성자 호출(placement new)
		new(memory)Type(forward<Args>(args)...);
		return memory;
	}

	static void Push(Type* obj)
	{
		// 메모리를 할당 해제하는 것이 아니므로 직접 소멸자를 호출
		obj->~Type();

		// 반환 받은 메모리 영역에 MemoryHeader 부분을 복원해서 풀로 반환
		s_pool.Push(MemoryHeader::AttachHeader(obj));
	}

	template<typename... Args>
	static shared_ptr<Type> MakeShared(Args&&... args)
	{
		// 스마트 포인트 생성해서 반환
		// = 이후의 값들은 ControlBlock이 관리할 객체 메모리, deallocator 인듯(여기서 할당 해제가 아니라 풀로 반환되는 구조)
		shared_ptr<Type> ptr = { Pop(forward<Args>(args)...), Push };
		return ptr;
	}

private:
	static int32		s_allocSize;
	static MemoryPool	s_pool;

};

template<typename Type>
int32 ObjectPool<Type>::s_allocSize = sizeof(Type) + sizeof(MemoryHeader);

template<typename Type>
MemoryPool ObjectPool<Type>::s_pool{ s_allocSize };

NAME_SPACE_END
