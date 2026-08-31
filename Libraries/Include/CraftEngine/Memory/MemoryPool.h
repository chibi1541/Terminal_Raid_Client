#pragma once

// ########################
// #	  MemoryPool	  #
// ########################

// 여기까지 하는게 맞는지 모르겠지만
// 멀티쓰레드 환경에서 쓰레드 세이브한 오브젝트 풀 사용을 위해 추가합니다

enum
{
	SLIST_ALIGNMENT = 16
};

// Memory Header
// 스레드 세이프하게 (ABA문제 회피) 사용하기 위해 SLIST 상속
DECLSPEC_ALIGN(SLIST_ALIGNMENT)
struct MemoryHeader : public SLIST_ENTRY
{
	// 메모리 풀의 메모리 구조
	// [MemoryHeader][Data]
	MemoryHeader(int32 size) : allocSize(size)
	{
	}

	// Rookiss 님 강의 들을 때부터 생각했던건데 
	// 강의에서는 이 함수 이름이 AttachHeader
	// 근데 엄밀히 따지할 할당 받은 영역에서 MemoryHeader를 건너뛰고 실제 데이터 영역을 반환하니까
	// 머리를 떼는(DetachHead) 함수 는 이쪽이 아닌가... 나는 그렇게 갑니다!
	static void* DetachHeader(MemoryHeader* header, int32 size)
	{
		new(header)MemoryHeader(size);	// placement new : 이미 할당된 영역에 해당 타입을 초기화
		return reinterpret_cast<void*>(++header);
	}

	// 전달받은 데이터 영역(void*) 앞에 붙어있는 MemoryHeader 위치를 복구하는 함수
	// 할당 해제 할때는 이 영역까지 할당 해제 해달라고 해야 터지지 않음
	static MemoryHeader* AttachHeader(void* ptr)
	{
		// MemoryHeader*로 변환하고 -1 만큼 하면 MemoryHeader 크기 만큼 앞 주소로 이동하니 머리가 돌아옴 
		MemoryHeader* header = reinterpret_cast<MemoryHeader*>(ptr) - 1;
		return header;
	}


	int32 allocSize;
};


NAME_SPACE_BEGIN(Craft)

// CRAFT_API 키워드를 넣어야 하나?
// dll에서 직접 참조하는 애들만 넣어야 하는게 아닌가... 잘 모르겠네...
// link 에러나면 넣는 걸루

// 여기도 메모리를 SLIST_ALIGNMENT(16)바이트로 정렬 
DECLSPEC_ALIGN(SLIST_ALIGNMENT)
class MemoryPool
{
public:
	MemoryPool(int32 allocSize);
	~MemoryPool();

	void			Push(MemoryHeader* ptr);
	MemoryHeader*	Pop();

private:
	// 메모리 풀의 시작 부분
	SLIST_HEADER	_header;
	int32			_allocSize = 0;

	// 여러 쓰레드에서 접근하는 걸 가정해서 atomic 변수로 선언
	atomic<int32>	_useCount = 0;	  
	atomic<int32>	_reserveCount = 0; 
};


NAME_SPACE_END
