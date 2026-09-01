#pragma once
#include <thread>
#include <functional>
#include <mutex>
#include "TLS.h"

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

	BYTE* OpenBufferChunk(int32 size);
	void CloseBufferChunk(int32 size);

	// 현재 이 함수를 부른 쓰레드의 ID.
	//
	// 정의가 반드시 .cpp(=DLL 안)에 있어야 한다.
	// LThreadId는 thread_local인데 TLS.h에서 dllimport 없이 선언돼 있어서,
	// 이 접근자가 헤더에 인라인으로 있으면 EXE 쪽 코드가 DLL의 TLS 슬롯을
	// 직접 읽으려다 잘못된 주소를 잡고 그대로 터진다.
	uint32 GetThreadID() const;

private:
	// read-write-lock이 의미가 없어서 mutex를 사용
	std::mutex _lock;
	std::vector<std::thread> _threads;
};

NAME_SPACE_END
