#pragma once

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers

#ifdef _DEBUG
#pragma comment(lib, "CraftEngine\\Debug\\CraftEngine.lib")
#pragma comment(lib, "Protobuf\\Debug\\libprotobufd.lib")
#else
#pragma comment(lib, "CraftEngine\\Release\\CraftEngine.lib")
#pragma comment(lib, "Protobuf\\Release\\libprotobufd.lib")
#endif

#include "Utils/CorePch.h"
#include "Network/Session.h"
#include "Thread/TLS.h"