#pragma once

#include "Utils/EngineMacro.h"
#include "Core/CraftObject.h"
#include "Xml/XmlParser.h"
#include <string>

NAME_SPACE_BEGIN(Craft)

// 언리얼의 Primary Data Asset을 참고한 추상 베이스.
//
// 일반 AssetManager::Load<T>()로 다루는 애셋(AnimationClipSet 등)과 달리,
// PrimaryDataAsset은 게임이 시작할 때 매니페스트(Assets/PrimaryAssets.xml 등)를 통해
// 한꺼번에 로드되고, 이름으로 조회되고, 유휴 언로드 대상이 되지 않는다
// (AssetManager::primaryAssets가 TypedCache와 완전히 분리된 별도 저장소이기 때문).
//
// 실제 데이터 타입은 이 클래스를 상속해서 만든다 (예: CharacterDataAsset, ItemDataAsset).
// 상속한 타입은 반드시 AssetManager::RegisterPrimaryAssetType<T>()로 문자열 이름을 등록해야
// 매니페스트의 type="..." 항목과 매칭된다.
class CRAFT_API PrimaryDataAsset : public CraftObject
{
	TYPE_DECLARATIONS(PrimaryDataAsset, CraftObject)

public:
	virtual ~PrimaryDataAsset() = default;

	// 매니페스트에 적힌 이름. AssetManager::GetPrimaryAsset()의 조회 키.
	inline const std::string& GetName() const { return name; }

	// 이 애셋의 실제 데이터를 읽어온 XML 경로.
	inline const std::wstring& GetSourcePath() const { return sourcePath; }

	// path가 가리키는 XML의 루트 노드를 받아 파생 타입이 자기 필드를 채운다.
	// 형식이 틀리거나 필수 값이 없으면 false를 반환한다.
	// (매니페스트 로더가 false를 받으면 크래시한다 - Primary 애셋은 필수 데이터라서)
	//
	// XmlNode의 조회 함수들이 전부 비-const라 root도 비-const 참조로 받는다.
	virtual bool LoadFromXml(XmlNode& root) = 0;

private:
	std::string name;
	std::wstring sourcePath;

	// name/sourcePath는 매니페스트를 읽는 AssetManager가 로드 직후에 채운다.
	friend class AssetManager;
};

NAME_SPACE_END
