#pragma once

// 템플릿이 외부 모듈로 공개되기 때문에 발생하는 경고 메시지
// 방법이 없어서 거슬리지만 않게 경고만 끔
#pragma warning(disable: 4251)

#include <iostream>
//#include <chrono> stl에서 시간 체크하는 함수 모음, 여기서는 안씀
#include <Windows.h>
#include <vector>
#include <memory>
#include <string>

#include "Utils/EngineMacro.h"
#include "Utils/FileUtils.h"
#include "Xml/XmlParser.h"
#include "Math/Vector2.h"
#include "Utils/json.hpp"
#include "Thread/Lock.h"
#include "Memory/ObjectPool.h"

// winsock
#include <winsock2.h>
#include <mswsock.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
