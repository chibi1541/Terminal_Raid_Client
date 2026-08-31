#pragma once
#include <array>


NAME_SPACE_BEGIN(Craft)

class CRAFT_API SendBuffer
{
	enum
	{
		// SendBuffer를 4KB로 설정
		SENDBUFFER_SIZE = 0x1000,
		BUFFER_RESET_SIZE = 256,
	};

public:
	SendBuffer();
	~SendBuffer();

	BYTE*			Buffer() { return &_buffer[_reservedPos]; }
	int32			FreeSize() { return SENDBUFFER_SIZE - _reservedPos; }
	int32			RemainSize() {return _reservedPos - _consumePos; }
	int32			BufferCapacity() { return SENDBUFFER_SIZE - _writePos; }
	int32			BufferRemainSize() {return _writePos - _readPos;}
	void			PushSendQueue(void* data, int32 size);
	void			ProcessBuffer(int32 size);
	int32			ConsumeBuffer();

private:
	void			ResetQueue();
	void			ResetBuffer();


private:
	// SendBuffer가 버퍼 큐의 역할을 하게되어 Lock이 필요
	// _queue에 접근할 때만 걸도록 함
	USE_LOCK;
	array<BYTE, SENDBUFFER_SIZE> _queue = {};
	array<BYTE, SENDBUFFER_SIZE> _buffer = {};
	uint32 _reservedPos = 0;
	uint32 _consumePos = 0;
	uint32 _writePos = 0;
	uint32 _readPos = 0;
};

// 처음에는 어떻게 만들까 했는데 Thread Local로 만들어서 사용
// 안그러면 락을 걸어야하는데 말도 웃긴 구조가 됨
class CRAFT_API BufferChunk
{
	enum
	{
		SENDBUFFER_CHUNK_SIZE = 0x4000,
	};

public:
	BufferChunk();
	~BufferChunk();

	void			Reset();
	BYTE*			Open(uint32 size);
	void			Close(uint32 size);

	BYTE* Buffer()	{ return &_buffer[_usedSize]; }
	uint32			FreeSize() { return static_cast<uint32>(_buffer.size()) - _usedSize; }
	bool			IsOpen() { return _open; }

private:
	array<BYTE, SENDBUFFER_CHUNK_SIZE> _buffer = {};
	bool _open = false;
	uint32 _usedSize = 0;

};

NAME_SPACE_END
