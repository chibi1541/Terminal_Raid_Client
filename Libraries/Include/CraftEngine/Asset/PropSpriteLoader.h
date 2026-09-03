#pragma once

#include "Utils/EngineMacro.h"
#include "Utils/Types.h"
#include "Asset/PropSpriteSet.h"
#include "Xml/XmlParser.h"

NAME_SPACE_BEGIN(Craft)

// 정적 프롭 스프라이트 정의 XML을 읽어서 PropSpriteSet으로 만드는 로더.
//
// 파일 형식 (Assets/*.prop.xml):
//
//   <PropSprites>
//       <Prop name="TOMB_A" tileSpan="1">
//           <Sprite dir="Up" width="12" height="14">
//               ...KKKKKK...
//               ..KLLLLLLK..
//           </Sprite>
//           <Sprite dir="Left" ... > ... </Sprite>
//       </Prop>
//   </PropSprites>
//
//   name     : 조회 키. StaticPropActor 생성자에 넘기는 그 이름이다.
//   tileSpan : 타일 영역(정사각)의 한 변 길이(타일 개수). 생략하면 1.
//              한 타일은 PropTileSize(Level/TileMetrics.h) 칸이다.
//   dir      : Up / Down / Left / Right. 액터가 보는 방향이지 카메라 방향이 아니다.
//   width/height : 선택. 픽셀맵과 대조해서 다르면 크래시한다.
//              진실의 원천은 픽셀맵이고 이건 선언일 뿐이다.
//              Sprite 생성자는 "줄 길이가 서로 다른" 경우만 잡으므로,
//              줄이 통째로 빠져 12x14가 12x13이 된 실수는 이 선언이 있어야 걸린다.
//   pivot    : 선택. "x,y". 생략하면 슬롯에 따라 다른 기본값이 쓰인다
//              (PropSprite::GetDefaultPivotY 참고 - 정면은 발밑, 측면은 이미지 중앙).
//
// ★ 빠진 방향 슬롯은 여기서 채운다 ★
//   Up 없으면 -> Down, Down 없으면 -> Up, Left 없으면 -> Right, Right 없으면 -> Left.
//   그래도 없으면 있는 것 아무거나.
// 원본 아트가 정면/측면 2장뿐이어도 네 슬롯이 다 채워진 상태로 나간다.
// 폴백을 로드 시점에 끝내야 그리는 쪽에 분기가 안 생긴다.
//
// 파일이 없거나 파싱에 실패하면 빈 묶음을 반환한다.
// (프롭 하나 때문에 게임이 아예 안 뜨는 상황을 막기 위함 - SpriteAnimationLoader와 같은 방침)
class CRAFT_API PropSpriteLoader
{
public:
	static PropSpriteSet LoadFromFile(const WCHAR* path);

private:
	// <Prop> 하나를 읽는다. 쓸 만한 스프라이트가 하나도 없으면 false.
	static bool LoadProp(XmlNode& propNode, OUT PropSprite& outProp);

	// 채워진 슬롯을 빈 슬롯으로 복사한다. 위 규칙대로.
	static void ResolveFallbacks(OUT PropSprite& outProp, const bool (&hasSlot)[FacingCount]);

	PropSpriteLoader() = delete;
};

NAME_SPACE_END
