#pragma once

#include "Utils/EngineMacro.h"
#include "Math/SymbolPalette.h"
#include "Xml/XmlParser.h"
#include <string>

NAME_SPACE_BEGIN(Craft)

// 레벨 하나의 지형 격자. 한 글자가 콘솔 셀 한 칸이다.
//
// AssetManager가 파일 단위로 캐싱하는 타입이다(AnimationClipSet과 같은 자리).
// 어느 파일을 읽을지는 LevelDataAsset이 이름 -> 경로로 알려준다.
//
// 이 데이터는 "보이는 것"만 담는다. 이동 가능 여부(충돌)는 서버가 자기
// 레벨 데이터로 판정하며 여기에는 그 정보가 없다.
// blocked / tileSize 같은 항목을 추가하지 말 것 -
// 같은 정보를 서버와 클라가 각자 들고 있으면 반드시 갈라진다.
//
// 파일 형식 (Assets/Cemetery.level.xml):
//
//   <Level levelId="Cemetery" width="256" height="192" desc="...">
//       <Row>FWWFGNNWGFWWFFF...</Row>   <- width 글자, height 줄
//   </Level>
//
//   levelId : 콘텐츠 식별자. 진단 메시지에 찍힌다.
//             조회 키가 아니다 - 조회는 LevelData.xml의 name= 으로 한다.
//   width   : 한 행의 글자 수. 모든 행이 정확히 이 길이여야 한다.
//   height  : 행 개수. 정확히 일치해야 한다.
//   desc    : 읽지 않는다. 사람이 보는 메모다.
//             이 클래스에 desc를 읽는 코드가 없다는 것이 그 약속의 유일한 증거이므로,
//             멤버도 접근자도 만들지 않는다.
//
// 기호는 SymbolPalette(Math/SymbolPalette.h) 그대로다.
// '.'(TransparentSymbol)은 그리지 않고 건너뛰는 칸이다.
class CRAFT_API LevelMap
{
public:
	LevelMap() = default;

	// XML에서 격자를 읽는다.
	// 파일이 없거나 형식이 틀리면 빈 격자를 반환한다(SpriteAnimationLoader와 같은 방침 -
	// 레벨 하나 때문에 게임이 아예 안 뜨는 것보다 낫다). 이유는 출력 창에 남긴다.
	static LevelMap LoadFromFile(const WCHAR* path);

	inline const std::string& GetLevelId() const { return levelId; }
	inline int GetWidth() const { return width; }
	inline int GetHeight() const { return height; }
	inline bool IsEmpty() const { return cells.empty(); }

	// 한 칸의 기호. 범위 밖이면 투명이다.
	//
	// 헤더에 인라인으로 두는 이유는 프레임당 1만 번 이상 불리기 때문이다.
	// 화면 전체를 훑는 루프의 가장 안쪽이라, 함수 호출 한 번이 그대로 비용이 된다.
	//
	// 범위 밖을 크래시가 아니라 투명으로 처리하는 것이 중요하다.
	// 카메라가 월드 가장자리에 닿으면 화면 일부가 레벨 밖을 가리키는 것이 정상이다.
	inline char GetCell(int x, int y) const
	{
		if (x < 0 || y < 0 || x >= width || y >= height)
		{
			return SymbolPalette::TransparentSymbol;
		}

		return cells[static_cast<size_t>(y) * width + x];
	}

private:
	bool ParseFromXml(XmlNode& root);

	// 한 행을 검사하면서 cells에 이어 붙인다.
	// 길이나 기호가 틀리면 어느 행 어느 열인지까지 찍고 false.
	bool AppendRow(int rowIndex, const WCHAR* line);

	// 진단 메시지 한 줄.
	// 49,152자 중 어디가 틀렸는지가 진단의 전부라, 실패 경로마다 반드시 남긴다.
	void ReportError(const char* format, ...) const;

private:
	std::string levelId;

	int width = 0;
	int height = 0;

	// 평탄한 격자. 행이 연속으로 놓인다(y * width + x).
	// 2차원 벡터로 나누면 행마다 별도 할당이 생기고 캐시 지역성이 깨진다.
	std::string cells;
};

NAME_SPACE_END
