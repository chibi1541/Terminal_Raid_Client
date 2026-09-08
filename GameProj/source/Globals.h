#pragma once
#include "Network/Service.h"

#include <memory>

// 서버와의 연결을 소유한다.
//
// 네트워크 쓰레드가 GService->Run()을 돌리므로, 이 포인터는 그 쓰레드가
// 완전히 죽은 뒤에 정리되어야 한다.
// Engine::AddShutdownHandler로 등록한 Stop()이 그 순서를 보장한다.
extern std::unique_ptr<Craft::Service> GService;

// 서버 연결을 개시한다(소켓 connect + 네트워크 쓰레드 기동).
//
// 여러 번 불러도 최초 1회만 동작한다. 메뉴의 Game Start 가 부른다.
// 예전에는 main()이 부팅 시 무조건 했다 - 그래서 타이틀 화면을 띄울 수 없었다.
void StartServerConnection();

// 인게임 HUD(안내 라벨 + 네트워크 상태 보드) 위젯을 만들어 뷰포트에 올린다.
//
// 메뉴 화면에는 뜨면 안 되므로 부팅 시 만들지 않고, 메뉴에서 게임으로 넘어갈 때 부른다.
// 반드시 게임(메인) 쓰레드에서 호출할 것 - NetStatus 가 기준 쓰레드 ID를 여기서 잡는다.
void CreateInGameHud();
