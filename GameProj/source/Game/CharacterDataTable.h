#pragma once

#include "Protocol/Enum.pb.h"

#include <string>
#include <unordered_map>

// 플레이어 캐릭터 정의 테이블. ActorDataAsset 이 소유하고, ActorData.xml 의 <CharacterData path>
// 가 가리키는 파일(Server/Data/CharacterData.xml 에서 sync_actor_data.py 로 복제됨)을 읽는다.
//
// key = (int)Protocol::CharacterType. 스폰 리팩터 후 Spawn 이 CharacterType 을 받아 이 테이블로
// animClip / 충돌 사이즈 / 스탯을 조회한다.
class CharacterDataTable
{
public:
	bool LoadFromFile(const wchar_t* path);

	Protocol::CharacterType	GetDefault() const { return _default; }

	const std::string&	GetAnimClip(Protocol::CharacterType type) const;
	int					GetFootprintTiles(Protocol::CharacterType type) const;
	int					GetCollisionCells(Protocol::CharacterType type) const;
	int					GetRadius(Protocol::CharacterType type) const;
	int					GetMaxHp(Protocol::CharacterType type) const;
	int					GetAttackPower(Protocol::CharacterType type) const;
	int					GetMoveSpeedCells(Protocol::CharacterType type) const;

private:
	struct Entry
	{
		std::string	animClip = "Knight";
		int			footprintTiles = 2;
		int			collisionCells = 8;
		int			radius = 2;
		int			maxHp = 100;
		int			attackPower = 10;
		int			moveSpeedCells = 20;
	};

	const Entry& Find(Protocol::CharacterType type) const;

	std::unordered_map<int, Entry>	_entries;	// key = (int)CharacterType
	Entry							_fallback;
	Protocol::CharacterType			_default = Protocol::CHARACTER_KNIGHT;
};
