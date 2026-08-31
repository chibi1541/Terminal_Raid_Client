#pragma once
#include <thread>
#include <functional>
#include <mutex>

// ########################
// #	  ThreadManager	  #
// ########################

NAME_SPACE_BEGIN(Craft)
class CRAFT_API ThreadManager
{
public:
	ThreadManager();
	~ThreadManager();

	// 실행 쓰레드를 생성
	void Launch(function<void()> work);
	// 쓰레드가 전부 종료될 때까지(join 상태가 될 때 까지) 대기
	void Join();

	// Thread-Local-Storage 초기화
	static void InitTLS();
	// Thread-Local-Storage 정리 
	static void DestroyTLS();


private:
	// read-write-lock이 의미가 없어서 mutex를 사용
	std::mutex _lock;
	std::vector<std::thread> _threads;
};

NAME_SPACE_END
