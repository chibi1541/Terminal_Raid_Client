#pragma once

#include "Utils/EngineMacro.h"
#include "Level/Level.h"
#include <memory>
#include <string>

NAME_SPACE_BEGIN(Craft)

class LevelMap;

// LevelDataAsset의 지형을 배경으로 깔고 그 위에 액터를 올리는 레벨.
//
// 카메라가 보는 영역만 그린다. 레벨이 아무리 커도 비용은 화면 크기에 고정된다
// (256x192든 4096x4096이든 같다). 컬링을 제출 전에 우리가 해야 하는 이유는
// Renderer::Submit이 가시성과 무관하게 줄마다 할당하고,
// 화면 밖 판정은 그 뒤 DrawRenderQueue에서 일어나기 때문이다.
//
// 사용법 - 레벨 이름은 Assets/LevelData.xml의 name= 이다.
// (PrimaryAssets.xml의 name이 아니다. 그쪽은 LevelData 목록 자체의 이름이고,
//  여기는 그 목록 안의 항목 이름이다)
//
//   Engine::Get().AddNewLevel<TileMapLevel>();
//
// 격자는 비동기로 로드된다. 도착 전 몇 프레임은 지형 없이 그려진다.
class CRAFT_API TileMapLevel : public Level
{
	TYPE_DECLARATIONS(TileMapLevel, Level)

public:
	// Engine::AddNewLevel<T>()가 인자를 전달하지 않으므로 기본값이 있어야 한다.
	explicit TileMapLevel(std::string levelName = "Cemetery");
	virtual ~TileMapLevel() = default;

	virtual void OnInitialized() override;
	virtual void Draw() override;

private:
	// 화면 한 줄을 제출한다. 전부 투명이면 제출하지 않는다.
	void SubmitRow(int screenY, int width);

	// 비동기 로드가 끝나면 격자를 받아 월드 경계까지 세운다.
	void OnLevelMapLoaded(std::shared_ptr<const LevelMap> loaded);

private:
	std::string levelName;

	// 조회는 OnInitialized에서 한 번만 한다.
	// GetPrimaryAsset은 해시 조회 + Cast<T>(dynamic_cast)라 Draw에서 부를 것이 아니다.
	std::shared_ptr<const LevelMap> levelMap;

	// 화면 한 줄을 조립하는 버퍼. 프레임마다 재할당하지 않으려고 멤버로 둔다.
	//
	// 개행이 들어가면 안 된다 - Renderer::ForEachLine이 줄을 쪼개서
	// 뒤쪽 칸들이 다음 행 위치에 그려진다.
	std::string rowScratch;
};

NAME_SPACE_END
