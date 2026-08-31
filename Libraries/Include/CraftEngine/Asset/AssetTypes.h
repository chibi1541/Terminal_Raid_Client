#pragma once

#include "Asset/AnimationClip.h"
#include <memory>
#include <vector>

NAME_SPACE_BEGIN(Craft)

// SpriteAnimationLoader::LoadFromFile()이 파일 하나에서 뽑아내는 클립 묶음.
// AssetManager는 파일 단위로 캐싱하므로 이 묶음 자체가 캐시되는 단위다.
using AnimationClipSet = std::vector<std::shared_ptr<const AnimationClip>>;

NAME_SPACE_END
