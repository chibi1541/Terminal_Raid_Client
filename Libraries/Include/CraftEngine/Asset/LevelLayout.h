#pragma once

#include "Utils/EngineMacro.h"
#include "Utils/Types.h"
#include "Actor/Facing.h"
#include "Xml/XmlParser.h"
#include <string>
#include <vector>

NAME_SPACE_BEGIN(Craft)

// 레벨 하나에 세울 프롭 배치 목록.
//
// LevelMap이 "무엇이 그려지는가"(지형 격자)라면 이쪽은 "무엇이 서 있는가"다.
// 둘을 나눈 이유 - 지형은 아트에서 변환해 만드는 생성물이고(convert_level.py),
// 배치는 사람이 계속 손보는 원본이다. 한 파일에 두면 재변환이 배치를 날린다.
//
// 파일 형식 (Assets/*.LevelLayout.xml):
//
//   <LevelLayout levelId="Cemetery" propSet="Cemetery" tileSize="12">
//       <Prop name="FENCE" x="10"  y="8" facing="Up" />
//       <Prop name="GATE"  x="178" y="8" facing="Up" />
//   </LevelLayout>
//
//   levelId  : 콘텐츠 식별자. 진단 메시지에만 쓴다. 조회 키가 아니다.
//   propSet  : Assets/PropData.xml의 name=. 이 배치가 쓰는 스프라이트 묶음이다.
//   tileSize : 배치 격자 한 칸의 셀 수. 레벨이 이 값을 받아 간다(Level::SetTileSize).
//
//   Prop x,y : ★ 기준점이 아니라 "타일 좌상단 셀 좌표"다 ★
//              기준점은 facing/스팬/타일 크기에 따라 위치가 달라지는데,
//              그 산수를 데이터에 적어두면 규칙이 바뀔 때마다 전부 다시 계산해야 한다.
//              타일 좌표로 적으면 격자 정렬이 구조적으로 보장되고,
//              손으로 옮길 때도 타일 크기만큼만 더하면 된다.
//              실제 기준점 변환은 TileMapLevel이 스폰할 때 한 곳에서 한다.
//   facing   : Up / Down / Left / Right. 프롭이 월드에서 보는 방향이다.
//              카메라와 무관하고, 회전해도 바뀌지 않는다.
//
// 이 파일은 손으로 유지한다. 생성기를 두지 않는다 -
// 배치는 원본을 고쳐 변환하는 작업이 아니라 이 파일 자체를 고치는 작업이다.
class CRAFT_API LevelLayout
{
public:
	// 프롭 하나의 배치 정보.
	struct Placement
	{
		std::string name;
		int tileX = 0;
		int tileY = 0;
		EFacing facing = EFacing::Up;
	};

	LevelLayout() = default;

	// 실패 시 빈 배치를 반환한다.
	// (배치 하나 때문에 게임이 아예 안 뜨는 상황을 막기 위함 - LevelMap과 같은 방침)
	static LevelLayout LoadFromFile(const WCHAR* path);

	inline const std::string& GetLevelId() const { return levelId; }
	inline const std::string& GetPropSetName() const { return propSetName; }
	inline int GetTileSize() const { return tileSize; }

	inline const std::vector<Placement>& GetPlacements() const { return placements; }
	inline bool IsEmpty() const { return placements.empty(); }

private:
	bool ParseFromXml(XmlNode& root);
	void ReportError(const char* format, ...) const;

private:
	std::string levelId;
	std::string propSetName;

	// 0이면 레벨이 자기 기본값을 유지한다.
	int tileSize = 0;

	std::vector<Placement> placements;
};

NAME_SPACE_END
