#include "pch.h"
#include "ProjectileDataTable.h"
#include "Xml/XmlParser.h"

using namespace Craft;

namespace
{
	std::string ToNarrow(const wchar_t* text)
	{
		std::string result;
		if (text == nullptr)
			return result;
		for (const wchar_t* p = text; *p != 0; ++p)
			result += (*p < 128) ? static_cast<char>(*p) : '?';
		return result;
	}

	Protocol::ProjectileType ParseType(const wchar_t* text)
	{
		Protocol::ProjectileType value = Protocol::Projectile_None;
		Protocol::ProjectileType_Parse(ToNarrow(text), &value);
		return value;
	}
}

bool ProjectileDataTable::LoadFromFile(const wchar_t* path)
{
	XmlParser parser;
	XmlNode root;

	if (parser.ParseFromFile(path, OUT root) == false || root.IsValid() == false)
		return false;

	const Protocol::ProjectileType parsedDefault =
		ParseType(root.GetStringAttr(L"defaultProjectile"));
	if (parsedDefault != Protocol::Projectile_None)
		_default = parsedDefault;

	_entries.clear();

	for (XmlNode& node : root.FindChildren(L"Projectile"))
	{
		const Protocol::ProjectileType type = ParseType(node.GetStringAttr(L"id"));
		if (type == Protocol::Projectile_None)
			continue;

		Entry entry;
		entry.animSet = ToNarrow(node.GetStringAttr(L"animSet", L"Projectile"));
		entry.animClip = ToNarrow(node.GetStringAttr(L"animClip", L"Pellet"));
		entry.spawnUpCells = node.GetInt32Attr(L"spawnUpCells", 6);
		entry.spawnForwardCells = node.GetInt32Attr(L"spawnForwardCells", 3);
		entry.fireIntervalMs = node.GetInt32Attr(L"fireIntervalMs", 250);

		_entries[static_cast<int>(type)] = entry;
	}

	char msg[128];
	sprintf_s(msg, "[ProjectileData] entries=%d default=%d\n",
		static_cast<int>(_entries.size()), static_cast<int>(_default));
	::OutputDebugStringA(msg);

	return true;
}

const ProjectileDataTable::Entry&
ProjectileDataTable::Find(Protocol::ProjectileType type) const
{
	const auto it = _entries.find(static_cast<int>(type));
	return (it != _entries.end()) ? it->second : _fallback;
}

const std::string& ProjectileDataTable::GetAnimSet(Protocol::ProjectileType type) const
{
	return Find(type).animSet;
}

const std::string& ProjectileDataTable::GetAnimClip(Protocol::ProjectileType type) const
{
	return Find(type).animClip;
}

int ProjectileDataTable::GetSpawnUpCells(Protocol::ProjectileType type) const
{
	return Find(type).spawnUpCells;
}

int ProjectileDataTable::GetFireIntervalMs(Protocol::ProjectileType type) const
{
	return Find(type).fireIntervalMs;
}
