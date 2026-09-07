#include "pch.h"
#include "ActorDataAsset.h"
#include "Xml/XmlParser.h"

using namespace Craft;

namespace
{
	// <Tag path="..."/> 하위 파일이 있으면 loader 로 읽힌다. 없으면 조용히 넘어간다(선택).
	template <typename LoadFn>
	void LoadSubData(XmlNode& root, const wchar_t* tag, LoadFn&& load)
	{
		XmlNode node = root.FindChild(tag);
		if (node.IsValid() == false)
			return;

		const wchar_t* path = node.GetStringAttr(L"path");
		if (path == nullptr || path[0] == 0)
			return;

		if (load(path) == false)
		{
			char msg[128];
			sprintf_s(msg, "[ActorData] sub-data load failed\n");
			::OutputDebugStringA(msg);
		}
	}
}

bool ActorDataAsset::LoadFromXml(XmlNode& root)
{
	LoadSubData(root, L"ProjectileData",
		[this](const wchar_t* path) { return _projectiles.LoadFromFile(path); });
	LoadSubData(root, L"CharacterData",
		[this](const wchar_t* path) { return _characters.LoadFromFile(path); });
	LoadSubData(root, L"MonsterData",
		[this](const wchar_t* path) { return _monsters.LoadFromFile(path); });

	return true;	// Primary 애셋이라 false 면 크래시. 결측은 각 Table 의 기본값으로 흡수.
}
