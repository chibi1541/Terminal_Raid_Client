#pragma once

#include "Utils/EngineMacro.h"
#include <string>

NAME_SPACE_BEGIN(Craft)

// XML 텍스트를 Sprite가 받는 픽셀맵 문자열로 다듬는다.
//
//   줄 앞뒤의 공백/탭/'\r' 제거 + 빈 줄 제거 + '\n'으로 재조립.
//
// 픽셀맵은 투명을 ' '가 아니라 '.'으로 쓰기 때문에 공백을 지워도 그림이 망가지지 않고,
// 덕분에 XML 안에서 자유롭게 들여쓸 수 있다.
//
// 팔레트에 없는 기호가 섞여 있으면 여기서 크래시한다.
// Renderer까지 흘려보내지 말고 데이터를 읽는 지점에서 잡는 편이 원인 찾기가 쉽다.
//
// SpriteAnimationLoader와 PropSpriteLoader가 공유한다.
// 한쪽에만 두면 다른 쪽이 복사해가고, 그 순간부터 두 규칙이 갈라진다.
CRAFT_API std::string NormalizePixelMapText(const std::string& rawText);

NAME_SPACE_END
