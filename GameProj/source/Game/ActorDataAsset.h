#pragma once

#include "Asset/PrimaryDataAsset.h"
#include "Game/ProjectileDataTable.h"

// 액터 관련 데이터의 단일 진입점. (PrimaryAssets.xml 의 type="ActorData")
//
// Assets/Actor/ActorData.xml 은 종류별 하위 데이터 파일 경로만 들고 있고,
// 이 애셋이 그 파일들을 각 타입의 Table 로 파싱해 소유한다. 게임은 여기서 전부 가져간다:
//
//   auto actorData = AssetManager::Get().GetPrimaryAsset<ActorDataAsset>("ActorData");
//   const ProjectileDataTable& proj = actorData->Projectiles();
//
// 새 액터 데이터 종류 추가 절차:
//   1. Assets/Actor/ActorData.xml 에 <XxxData path="..."/> 한 줄
//   2. XxxDataTable 클래스 (ProjectileDataTable 처럼)
//   3. 아래에 멤버 + 접근자 + LoadFromXml 의 dispatch 한 줄
class ActorDataAsset : public Craft::PrimaryDataAsset
{
	TYPE_DECLARATIONS(ActorDataAsset, Craft::PrimaryDataAsset)

public:
	ActorDataAsset() = default;
	virtual ~ActorDataAsset() = default;

	// XmlNode 는 전역 네임스페이스 (엔진 PrimaryDataAsset::LoadFromXml 시그니처와 동일).
	virtual bool LoadFromXml(XmlNode& root) override;

	const ProjectileDataTable&	Projectiles() const { return _projectiles; }

	// 나중에:
	//   const PlayerDataTable&  Players() const  { return _players; }
	//   const MonsterDataTable& Monsters() const { return _monsters; }

private:
	ProjectileDataTable	_projectiles;
	//  PlayerDataTable  _players;
	//  MonsterDataTable _monsters;
};
