#include "pch.h"
#include "MonsterDataTable.h"
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

	Protocol::MonsterType ParseType(const wchar_t* text)
	{
		Protocol::MonsterType value = Protocol::Monster_None;
		Protocol::MonsterType_Parse(ToNarrow(text), &value);
		return value;
	}
}

bool MonsterDataTable::LoadFromFile(const wchar_t* path)
{
	XmlParser parser;
	XmlNode root;

	if (parser.ParseFromFile(path, OUT root) == false || root.IsValid() == false)
		return false;

	const Protocol::MonsterType parsedDefault =
		ParseType(root.GetStringAttr(L"defaultMonster"));
	if (parsedDefault != Protocol::Monster_None)
		_default = parsedDefault;

	_entries.clear();

	for (XmlNode& node : root.FindChildren(L"Monster"))
	{
		const Protocol::MonsterType type = ParseType(node.GetStringAttr(L"id"));
		if (type == Protocol::Monster_None)
			continue;

		Entry entry;
		entry.animClip = ToNarrow(node.GetStringAttr(L"animClip", L"Zombie"));
		entry.footprintTiles = node.GetInt32Attr(L"footprintTiles", 2);
		entry.collisionCells = node.GetInt32Attr(L"collisionCells", 8);
		entry.radius = node.GetInt32Attr(L"radius", 4);
		entry.maxHp = node.GetInt32Attr(L"maxHp", 60);
		entry.attackPower = node.GetInt32Attr(L"attackPower", 8);
		entry.moveSpeedCells = node.GetInt32Attr(L"moveSpeedCells", 8);

		_entries[static_cast<int>(type)] = entry;
	}

	char msg[128];
	sprintf_s(msg, "[MonsterData] entries=%d default=%d\n",
		static_cast<int>(_entries.size()), static_cast<int>(_default));
	::OutputDebugStringA(msg);

	return true;
}

const MonsterDataTable::Entry&
MonsterDataTable::Find(Protocol::MonsterType type) const
{
	const auto it = _entries.find(static_cast<int>(type));
	return (it != _entries.end()) ? it->second : _fallback;
}

const std::string& MonsterDataTable::GetAnimClip(Protocol::MonsterType type) const { return Find(type).animClip; }
int MonsterDataTable::GetFootprintTiles(Protocol::MonsterType type) const { return Find(type).footprintTiles; }
int MonsterDataTable::GetCollisionCells(Protocol::MonsterType type) const { return Find(type).collisionCells; }
int MonsterDataTable::GetRadius(Protocol::MonsterType type) const { return Find(type).radius; }
int MonsterDataTable::GetMaxHp(Protocol::MonsterType type) const { return Find(type).maxHp; }
int MonsterDataTable::GetAttackPower(Protocol::MonsterType type) const { return Find(type).attackPower; }
int MonsterDataTable::GetMoveSpeedCells(Protocol::MonsterType type) const { return Find(type).moveSpeedCells; }
