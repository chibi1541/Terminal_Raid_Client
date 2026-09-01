#pragma once
#include "Network/Service.h"

#include <memory>

// 서버와의 연결을 소유한다.
//
// 네트워크 쓰레드가 GService->Run()을 돌리므로, 이 포인터는 그 쓰레드가
// 완전히 죽은 뒤에 정리되어야 한다.
// Engine::AddShutdownHandler로 등록한 Stop()이 그 순서를 보장한다.
extern std::unique_ptr<Craft::Service> GService;
