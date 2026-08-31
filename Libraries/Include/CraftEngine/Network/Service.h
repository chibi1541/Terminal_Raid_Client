#pragma once
#include "NetAddress.h"

NAME_SPACE_BEGIN(Craft)

class Session;

using SessionFactory = function<std::unique_ptr<Session>(NetAddress)>;

// 클라이언트 기능만 지원(이쪽에서 커넥트 요청 후 통신)
class CRAFT_API Service
{
public:
	Service(NetAddress address, SessionFactory sessionFactory);
	virtual ~Service();

	virtual bool Start() abstract;
	bool CanStart() { return _sessionFactory != nullptr; }
	virtual void CloseService();

	virtual void Run() abstract;

	virtual void HandleRecv();
	virtual void HandleSend();
	
	const Session* GetSession() const {return _session.get(); }
	NetAddress GetAddress() const;

protected:
	USE_LOCK;
	std::unique_ptr<class Session> _session;
	SessionFactory _sessionFactory;
};

class CRAFT_API ServerService : public Service
{
public:
	ServerService(NetAddress address, SessionFactory sessionFactory);
	virtual ~ServerService();

	virtual bool Start() override;
	virtual void Run() override;
	virtual void CloseService() override;



	// select 방식의 소켓 모델 사용
private:
	fd_set _readSet;
	fd_set _writeSet;
	
	bool _isRunning = false;
};




NAME_SPACE_END
