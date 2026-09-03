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
//           <Level name="Cemetery" path="../Assets/Cemetery.level.xml" />
//       </Levels>
//   </LevelData>
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

	inline bool HasLevel(const std::string& name) const
	{
		return levelPaths.find(name) != levelPaths.end();
	}

	inline int GetLevelCount() const { return static_cast<int>(levelPaths.size()); }

private:
	std::unordered_map<std::string, std::wstring> levelPaths;
};

NAME_SPACE_END
