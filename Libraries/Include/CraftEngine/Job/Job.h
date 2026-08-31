#pragma once
#include <functional>

// ########################
// #		Job			  #
// ########################

// void 타입에 인자를 안받는 상태로 정의
// 인자는 람다 형태로 캡쳐하거나 멤버 변수를 사용
using CallBack = std::function<void()>;

NAME_SPACE_BEGIN(Craft)

class CRAFT_API Job
{
public:
	Job(CallBack&& callback) : _callback(std::move(callback))
	{

	}

	// 이거 memFunc의 시그니처에서 이미 Args 타입이 결정되서 타입 추론이 일어나지 않기 때문에
	// Args&&는 RValue Reference 고정입니다(그래서 std::move로 안넘기면 에러 발생한거였음...)

	template<typename T, typename Ret, typename... Args>
	Job(shared_ptr<T> owner, Ret(T::* memFunc)(Args...), Args&&... args)
	{
		_callback = [owner, memFunc, args...]()
			{
				(owner.get()->*memFunc)(args...);
			};
	}

	// Args에 타입 추론이 일어나서 보편 참조가 가능하도록 구조 변경
	// 변수 인자에 별도의 템플릿 타입을 사용
	template<typename T, typename Ret, typename... Params, typename... Args>
	Job(shared_ptr<T> owner, Ret(T::* memFunc)(Params...), Args&&... args)
	{
		// 캡쳐 전달할 때 복사 방지
		_callback = [owner = std::move(owner), memFunc, tup = std::make_tuple(std::forward<Args>(args)...)]() mutable
			{
				std::apply([&](auto&&... unpacked) {
					(owner.get()->*memFunc)(std::forward<decltype(unpacked)>(unpacked)...);
					}, tup);
			};
	}


	void Execute()
	{
// DEBUG 모드에서는 그냥 터트려서 문제를 찾음
#if _DEBUG == 0
		if (_callback != nullptr)
#endif // _DEBUG
			_callback();
	}

private:
	CallBack _callback;

};

NAME_SPACE_END
