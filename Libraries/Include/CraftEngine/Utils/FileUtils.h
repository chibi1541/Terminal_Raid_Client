#pragma once

#include <vector>
#include <filesystem>
#include <fstream>
#include <string>
#include "Types.h"

using namespace std;

/*-----------------
	FileUtils
------------------*/

namespace fs = std::filesystem;

class CRAFT_API FileUtils
{
public:
	static vector<BYTE>				ReadFile(const WCHAR* path);
	static wstring					Convert(string str);
	static string					Convert(wstring str);

};

inline vector<BYTE> FileUtils::ReadFile(const WCHAR* path)
{
	vector<BYTE> ret;

	fs::path filePath{ path };

	const uint32 fileSize = static_cast<uint32>(fs::file_size(filePath));
	ret.resize(fileSize);

	basic_ifstream<BYTE> inputStream{ filePath };
	inputStream.read(&ret[0], fileSize);

	return ret;
}

inline wstring FileUtils::Convert(string str)
{
	const int32 srcLen = static_cast<int32>(str.size());

	wstring ret;
	if (srcLen == 0)
		return ret;

	const int32 retLen = ::MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<char*>(&str[0]), srcLen, NULL, 0);
	ret.resize(retLen);
	::MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<char*>(&str[0]), srcLen, &ret[0], retLen);

	return ret;
}

// Convert(string)의 반대 방향. XML 파서가 WCHAR로 동작해서 되돌릴 일이 생긴다.
inline string FileUtils::Convert(wstring str)
{
	const int32 srcLen = static_cast<int32>(str.size());

	string ret;
	if (srcLen == 0)
		return ret;

	const int32 retLen = ::WideCharToMultiByte(CP_UTF8, 0, &str[0], srcLen, NULL, 0, NULL, NULL);
	ret.resize(retLen);
	::WideCharToMultiByte(CP_UTF8, 0, &str[0], srcLen, &ret[0], retLen, NULL, NULL);

	return ret;
}

