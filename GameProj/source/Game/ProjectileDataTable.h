#pragma once

#include "Protocol/Enum.pb.h"

#include <string>
#include <unordered_map>

// 투사체 정의 테이블. ActorDataAsset 이 소유하고, ActorData.xml 의 <ProjectileData path> 가
// 가리키는 파일(Server/Config/ProjectileData.xml 에서 복제됨)을 읽어 채운다.
//
// 액터 데이터 종류마다 이런 Table 클래스를 하나씩 두고 ActorDataAsset 이 모은다
// (나중에 PlayerDataTable / MonsterDataTable ...).
class ProjectileDataTable
{
public:
	// XML 파일 하나를 읽어 테이블을 채운다. 실패하면 false (기본값은 그대로).
	bool LoadFromFile(const wchar_t* path);

	// 플레이어 기본 공격이 쏘는 투사체.
	Protocol::ProjectileType	GetDefault() const { return _default; }

	const std::string&	GetAnimSet(Protocol::ProjectileType type) const;
	const std::string&	GetAnimClip(Protocol::ProjectileType type) const;
	int					GetSpawnUpCells(Protocol::ProjectileType type) const;
	int					GetFireIntervalMs(Protocol::ProjectileType type) const;

private:
	struct Entry
	{
		std::string	animSet = "Projectile";
		std::string	animClip = "Pellet";
		int			spawnUpCells = 6;
		int			spawnForwardCells = 3;
		int			fireIntervalMs = 250;
	};

	const Entry& Find(Protocol::ProjectileType type) const;

	std::unordered_map<int, Entry>	_entries;	// key = (int)ProjectileType
	Entry							_fallback;
	Protocol::ProjectileType		_default = Protocol::Projectile_Pellet;
};
