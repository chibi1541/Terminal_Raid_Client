#include "pch.h"
#include "Globals.h"

#include "Engine/Engine.h"
#include "Thread/ThreadManager.h"

std::unique_ptr<Craft::Service> GService;

LoginRequest GLoginRequest;

void StartServerConnection()
{
	// 두 번 눌러도(또는 재입장 시) 한 번만.
	static bool started = false;
	if (started || GService == nullptr)
	{
		return;
	}
	started = true;

	ASSERT_CRASH(GService->Start());

	Craft::Engine::Get().GetThreadManager()->Launch([]()
		{
			GService->Run();
		}
	);
}
