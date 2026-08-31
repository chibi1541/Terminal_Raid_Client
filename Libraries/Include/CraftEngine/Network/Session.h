#pragma once
#include "NetAddress.h"


NAME_SPACE_BEGIN(Craft)

class RecvBuffer;
class SendBuffer;

class Session
{
	enum { RECV_BUFFER_SIZE = 0x1000, };

	friend class Service;

public:
	Session(NetAddress address);
	virtual ~Session();

public:
	bool				Connect();
	void				Disconnect(const WCHAR* cause);
	// send queue에 있던 요청을 sendBuffer로 옮기고 그 사이즈를 반환
	int32				ReadyForSend();
	void				RegisterSend(void* buffer, int32 size);

public:
	/* 정보 관련 */
	void		SetNetAddress(NetAddress address) { _netAddress = address; }
	NetAddress	GetAddress() { return _netAddress; }
	SOCKET		GetSocket() { return _socket; }
	bool		IsConnected() { return _connected; }

	void HandleError(int32 errorCode);

private:
	void ProcessRecv(int32 numOfBytes);
	void ProcessSend(int32 numOfBytes);

protected:
	// 필요한 상황에 컨텐츠 코드에서 오버라이드 할 함수
	virtual void OnConnected() {}
	virtual int32 OnRecv(BYTE* buffer, int32 len) { return len; }
	virtual void OnSend(int32 len) {}
	virtual void OnDisconnected() {}

private:
	USE_LOCK;
	SOCKET			_socket = INVALID_SOCKET;
	NetAddress		_netAddress = {};
	atomic<bool>	_connected = false;

	std::unique_ptr<RecvBuffer> _recvBuffer;
	std::unique_ptr<SendBuffer> _sendBuffer;
	atomic<bool>		_sendRegistered = false;

private:
};

NAME_SPACE_END
