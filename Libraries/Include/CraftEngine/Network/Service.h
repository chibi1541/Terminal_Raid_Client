#pragma once
#include "NetAddress.h"
#include <atomic>

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
	bool CanStart() { return _session != nullptr; }
	virtual void CloseService();

	virtual void Run() abstract;

	// Run() 루프를 빠져나오게 한다.
	//
	// 게임 쓰레드에서 부르고 네트워크 쓰레드가 읽는다. 그래서 _isRunning이 atomic이다.
	// 이걸 부르지 않으면 ThreadManager::Join()이 무한 루프인 Run()을 영영 기다린다.
	virtual void Stop() { _isRunning.store(false); }
	bool IsRunning() const { return _isRunning.load(); }

	virtual void HandleRecv();
	virtual void HandleSend();
	
	const Session* GetSession() const {return _session.get(); }
	NetAddress GetAddress() const;

protected:
	USE_LOCK;
	std::unique_ptr<class Session> _session;
	//SessionFactory _sessionFactory;

	// Run() 루프의 생존 플래그.
	// 파생 클래스가 아니라 여기 있는 이유는 Stop()이 base API이기 때문이다.
	atomic<bool> _isRunning = false;
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
};




NAME_SPACE_END
