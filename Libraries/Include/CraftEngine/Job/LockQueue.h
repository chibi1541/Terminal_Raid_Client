#pragma once
#include <queue>

// ########################
// #	  LockQueue		  #
// ########################

NAME_SPACE_BEGIN(Craft)

template<typename T>
class CRAFT_API LockQueue
{
public:
	void Push(T item)
	{
		WRITE_LOCK;
		_items.push(item);
	}

	T Pop()
	{
		WRITE_LOCK;
		if (_items.empty())
			return T();

		T ret = _items.front();
		_items.pop();

		return ret;
	}

	// Lock을 물고 전체를 돌면서 삭제 하지 않고
	// Job 값을 전부 복사로 전달 받아서 Lock을 해제하고 Flush 하도록 구현
	void PopAll(OUT std::vector<T>& items)
	{
		WRITE_LOCK;
		while (T item = Pop())
		{
			items.emplace_back(item);
		}
	}

	void Clear()
	{
		WRITE_LOCK;
		_items = queue<T>();
	}

private:
	USE_LOCK;
	std::queue<T> _items;
};

NAME_SPACE_END

