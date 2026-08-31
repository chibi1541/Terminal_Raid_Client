#pragma once

NAME_SPACE_BEGIN(Craft)

class RecvBuffer
{
	// 할당 할 때 입력한 사이즈의 BUFFER_COUNT 배수 만큼 할당함
	enum {BUFFER_COUNT = 10};

public:
	RecvBuffer(int32 bufferSize);
	~RecvBuffer();

	void Clean();
	bool OnWrite(int32 numOfBytes);
	bool OnRead(int32 numOfBytes);

	BYTE* WritePos() {return &_buffer[_writeIndex]; }
	BYTE* ReadPos() {return &_buffer[_readIndex]; }
	int32 DataSize() {return _writeIndex - _readIndex; }
	int32 FreeSize() {return _capacity - _writeIndex; }

private:
	int32 _capacity;
	int32 _bufferSize;
	int32 _writeIndex;
	int32 _readIndex;
	vector<BYTE> _buffer;
};

NAME_SPACE_END
