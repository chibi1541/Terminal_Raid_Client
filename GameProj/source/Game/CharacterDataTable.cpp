#include "pch.h"
#include "CharacterDataTable.h"
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

	Protocol::CharacterType ParseType(const wchar_t* text)
	{
		Protocol::CharacterType value = Protocol::CHARACTER_NONE;
		Protocol::CharacterType_Parse(ToNarrow(text), &value);
		return value;
	}
}

bool CharacterDataTable::LoadFromFile(const wchar_t* path)
{
	XmlParser parser;
	XmlNode root;

	if (parser.ParseFromFile(path, OUT root) == false || root.IsValid() == false)
		return false;

	const Protocol::CharacterType parsedDefault =
		ParseType(root.GetStringAttr(L"defaultCharacter"));
	if (parsedDefault != Protocol::CHARACTER_NONE)
		_default = parsedDefault;

	_entries.clear();

	for (XmlNode& node : root.FindChildren(L"Character"))
	{
		const Protocol::CharacterType type = ParseType(node.GetStringAttr(L"id"));
		if (type == Protocol::CHARACTER_NONE)
			continue;

		Entry entry;
		entry.animClip = ToNarrow(node.GetStringAttr(L"animClip", L"Knight"));
		entry.footprintTiles = node.GetInt32Attr(L"footprintTiles", 2);
		entry.collisionCells = node.GetInt32Attr(L"collisionCells", 8);
		entry.radius = node.GetInt32Attr(L"radius", 2);
		entry.maxHp = node.GetInt32Attr(L"maxHp", 100);
		entry.attackPower = node.GetInt32Attr(L"attackPower", 10);
		entry.moveSpeedCells = node.GetInt32Attr(L"moveSpeedCells", 20);

		_entries[static_cast<int>(type)] = entry;
	}

	char msg[128];
	sprintf_s(msg, "[CharacterData] entries=%d default=%d\n",
		static_cast<int>(_entries.size()), static_cast<int>(_default));
	::OutputDebugStringA(msg);

	return true;
}

const CharacterDataTable::Entry&
CharacterDataTable::Find(Protocol::CharacterType type) const
{
	const auto it = _entries.find(static_cast<int>(type));
	return (it != _entries.end()) ? it->second : _fallback;
}

const std::string& CharacterDataTable::GetAnimClip(Protocol::CharacterType type) const { return Find(type).animClip; }
int CharacterDataTable::GetFootprintTiles(Protocol::CharacterType type) const { return Find(type).footprintTiles; }
int CharacterDataTable::GetCollisionCells(Protocol::CharacterType type) const { return Find(type).collisionCells; }
int CharacterDataTable::GetRadius(Protocol::CharacterType type) const { return Find(type).radius; }
int CharacterDataTable::GetMaxHp(Protocol::CharacterType type) const { return Find(type).maxHp; }
int CharacterDataTable::GetAttackPower(Protocol::CharacterType type) const { return Find(type).attackPower; }
int CharacterDataTable::GetMoveSpeedCells(Protocol::CharacterType type) const { return Find(type).moveSpeedCells; }
