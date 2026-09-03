#pragma once

#include "Utils/EngineMacro.h"
#include "Actor/Facing.h"
#include "Asset/PropSpriteSet.h"
#include "Level/Level.h"
#include <memory>
#include <string>

NAME_SPACE_BEGIN(Craft)

class LevelMap;
class LevelLayout;
class StaticPropActor;

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

	// 프롭 하나를 타일 좌표에 세운다. 스프라이트 묶음이 도착한 뒤에만 동작한다.
	//
	// 기준점 변환(타일 좌표 -> 월드 좌표)이 여기 한 곳에만 있다.
	// facing에 따라 스팬 축과 앵커가 달라지는 산수를 호출부마다 반복하면
	// 반드시 한 군데가 어긋난다.
	std::shared_ptr<StaticPropActor> SpawnProp(
		const std::string& propName, int tileX, int tileY, EFacing facing);

private:
	// 화면 한 줄을 제출한다. 전부 투명이면 제출하지 않는다.
	void SubmitRow(int screenY, int width);

	// 비동기 로드가 끝나면 격자를 받아 월드 경계까지 세운다.
	void OnLevelMapLoaded(std::shared_ptr<const LevelMap> loaded);

	// 배치가 도착하면 타일 크기를 받아 세우고, 스프라이트 묶음 로드를 이어서 건다.
	void OnLayoutLoaded(std::shared_ptr<const LevelLayout> loaded);

	// 스프라이트 묶음까지 도착하면 배치대로 액터를 만든다.
	void OnPropSetLoaded(std::shared_ptr<const PropSpriteSet> loaded);

private:
	std::string levelName;

	// 조회는 OnInitialized에서 한 번만 한다.
	// GetPrimaryAsset은 해시 조회 + Cast<T>(dynamic_cast)라 Draw에서 부를 것이 아니다.
	std::shared_ptr<const LevelMap> levelMap;

	// 배치 목록. 스프라이트 묶음이 도착할 때까지 들고 있다가 스폰에 쓴다.
	std::shared_ptr<const LevelLayout> levelLayout;

	// 스프라이트 묶음. 액터들이 이걸 공유한다(액터마다 로드하지 않는다).
	std::shared_ptr<const PropSpriteSet> propSet;

	// 화면 한 줄을 조립하는 버퍼. 프레임마다 재할당하지 않으려고 멤버로 둔다.
	//
	// 개행이 들어가면 안 된다 - Renderer::ForEachLine이 줄을 쪼개서
	// 뒤쪽 칸들이 다음 행 위치에 그려진다.
	std::string rowScratch;
};

NAME_SPACE_END
