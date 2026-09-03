#pragma once

#include "Utils/EngineMacro.h"
#include "Asset/PropSprite.h"
#include <string>
#include <unordered_map>

NAME_SPACE_BEGIN(Craft)

// PropSpriteLoader::LoadFromFile()이 파일 하나에서 뽑아내는 프롭 묶음.
// AssetManager는 파일 단위로 캐싱하므로 이 묶음 자체가 캐시되는 단위다.
// (AssetTypes.h의 AnimationClipSet과 같은 자리)
//
// 키는 <Prop name="..."> 이다. StaticPropActor가 이 이름으로 찾는다.
using PropSpriteSet = std::unordered_map<std::string, PropSprite>;

NAME_SPACE_END
