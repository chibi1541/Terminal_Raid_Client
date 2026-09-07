#pragma once

#include "Protocol/Enum.pb.h"

#include <string>
#include <unordered_map>

// 몬스터 정의 테이블. ActorDataAsset 이 소유하고, ActorData.xml 의 <MonsterData path> 가
// 가리키는 파일(Server/Data/MonsterData.xml 에서 sync_actor_data.py 로 복제됨)을 읽는다.
//
// key = (int)Protocol::MonsterType. 스폰 리팩터 후 Spawn 이 MonsterType 을 받아 이 테이블로
// animClip / 충돌 사이즈 / 스탯을 조회한다.
class MonsterDataTable
{
public:
	bool LoadFromFile(const wchar_t* path);

	Protocol::MonsterType	GetDefault() const { return _default; }

	const std::string&	GetAnimClip(Protocol::MonsterType type) const;
	int					GetFootprintTiles(Protocol::MonsterType type) const;
	int					GetCollisionCells(Protocol::MonsterType type) const;
	int					GetRadius(Protocol::MonsterType type) const;
	int					GetMaxHp(Protocol::MonsterType type) const;
	int					GetAttackPower(Protocol::MonsterType type) const;
	int					GetMoveSpeedCells(Protocol::MonsterType type) const;

private:
	struct Entry
	{
		std::string	animClip = "Zombie";
		int			footprintTiles = 2;
		int			collisionCells = 8;
		int			radius = 4;
		int			maxHp = 60;
		int			attackPower = 8;
		int			moveSpeedCells = 8;
	};

	const Entry& Find(Protocol::MonsterType type) const;

	std::unordered_map<int, Entry>	_entries;	// key = (int)MonsterType
	Entry							_fallback;
	Protocol::MonsterType			_default = Protocol::Monster_Zombie;
};
