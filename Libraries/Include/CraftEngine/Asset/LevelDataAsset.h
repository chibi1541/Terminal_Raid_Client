#pragma once

#include "Utils/EngineMacro.h"
#include "Asset/PrimaryDataAsset.h"
#include <string>
#include <unordered_map>

NAME_SPACE_BEGIN(Craft)

// 레벨 이름 -> 레벨 파일 경로 매핑 테이블.
//
// AnimationDataAsset과 정확히 같은 자리이고 같은 역할이다.
// 레벨을 쓰는 쪽은 "Cemetery" 같은 이름만 알면 되고,
// 그 이름이 어느 파일인지는 데이터(XML) 쪽 관심사가 된다.
//
// 이 애셋 자체는 경로만 들고 있을 뿐 격자를 읽지 않는다.
// 실제 로드는 이름으로 경로를 찾은 호출부가 그 경로를 다시 넘겨서 한다:
//
//   auto levelData = AssetManager::Get().GetPrimaryAsset<LevelDataAsset>("LevelData");
//   const std::wstring& path = levelData->FindLevelPath("Cemetery");
//   AssetManager::Get().LoadAsync<LevelMap>(path.c_str(), callback);
//
// 파일 형식 (Assets/LevelData.xml):
//
//   <LevelData>
//       <Levels>
//           <Level name="Cemetery" path="../Assets/Cemetery.level.xml"
//                                layout="../Assets/Cemetery.LevelLayout.xml" />
//       </Levels>
//   </LevelData>
//
//   path   : 지형 격자(LevelMap). 필수.
//   layout : 프롭 배치(LevelLayout). 선택 - 없으면 프롭 없는 레벨이다.
//            레이아웃은 레벨과 1:1이라 별도 PrimaryDataAsset을 두지 않고 여기 붙인다.
//
// 경로는 Config/ 와 같은 관습대로 프로젝트 폴더 기준 상대 경로다.
class CRAFT_API LevelDataAsset : public PrimaryDataAsset
{
	TYPE_DECLARATIONS(LevelDataAsset, PrimaryDataAsset)

public:
	LevelDataAsset() = default;
	virtual ~LevelDataAsset() = default;

	virtual bool LoadFromXml(XmlNode& root) override;

	// 이름으로 경로를 찾는다. 등록되지 않은 이름이면 빈 문자열.
	// (이름 오타는 그 경로로 로드를 시도하는 호출부에서 걸린다)
	const std::wstring& FindLevelPath(const std::string& name) const;

	// 등록되지 않은 이름이거나 layout 속성이 없으면 빈 문자열.
	const std::wstring& FindLayoutPath(const std::string& name) const;

	inline bool HasLevel(const std::string& name) const
	{
		return levelPaths.find(name) != levelPaths.end();
	}

	inline int GetLevelCount() const { return static_cast<int>(levelPaths.size()); }

private:
	std::unordered_map<std::string, std::wstring> levelPaths;

	// layout이 적힌 항목만 들어간다. levelPaths보다 작을 수 있다.
	std::unordered_map<std::string, std::wstring> layoutPaths;
};

NAME_SPACE_END
