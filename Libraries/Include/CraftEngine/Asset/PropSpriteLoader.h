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
//   tileSize : <PropSprites tileSize="4"> - 이 아트가 그려진 타일 격자 크기.
//              기본 피벗이 이 값에 걸린다. 생략하면 DefaultTileSize.
//   tileSpan : 타일 영역의 "벽 방향" 길이(타일 개수). 생략하면 1. 깊이는 언제나 1타일이다.
//              정사각인 것은 타일 한 칸의 정의이지 이 영역이 아니다.
//              스팬이 걸리는 축은 facing이 정한다 - Up/Down이면 가로, Left/Right면 세로.
//              손으로 적지 말 것. Tools/convert_props.py가 정면 폭 / 타일 크기로 계산한다.
//   dir      : Up / Down / Left / Right. 액터가 보는 방향이지 카메라 방향이 아니다.
//   width/height : 선택. 픽셀맵과 대조해서 다르면 크래시한다.
//              진실의 원천은 픽셀맵이고 이건 선언일 뿐이다.
//              Sprite 생성자는 "줄 길이가 서로 다른" 경우만 잡으므로,
//              줄이 통째로 빠져 12x14가 12x13이 된 실수는 이 선언이 있어야 걸린다.
//   mirror   : 선택. x | xy | none. Left <-> Right 폴백에서 뒤집을 축. 생략하면 x.
//              좌우 슬롯은 거울이 아니라 수직축 180도 회전이라 월드 두 축이 다 뒤집힌다.
//              이미지에서 어느 축이 뒤집히는지는 그 그림의 세로축이 무엇이냐에 달렸다 -
//              서 있는 울타리/기둥/묘비는 세로축이 높이라 x, 문처럼 개구부 전체를
//              담은 그림은 세로축이 벽 방향이라 xy.
//   pivot    : 선택. "x,y". 생략하면 슬롯과 무관하게
//              ((W-1)/2, (H-1) - tileSize) 가 쓰인다.
//              슬롯마다 다른 그림이 필요하면 규칙으로 유도하지 말고 여기 직접 적는다.
//              (PropSprite::GetDefaultPivotX/Y 참고)
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
	static bool LoadProp(XmlNode& propNode, int tileSize, OUT PropSprite& outProp);

	// 채워진 슬롯을 빈 슬롯으로 복사한다. 위 규칙대로.
	//
	// Left <-> Right 폴백은 그림을 좌우로 뒤집는다. 측면 아트가 한 장뿐이면
	// 반대편에서 봐도 같은 그림이 나와서 벽 하나가 거울이 아니게 된다.
	// mirrorModesRaw는 .cpp 안에서만 쓰는 enum 배열이다.
	// 그 enum을 헤더로 끌어올리면 이 파일을 include하는 모든 곳에 새는데,
	// 폴백 방식은 로더 내부 사정이라 밖에서 알 필요가 없다.
	static void ResolveFallbacks(
		OUT PropSprite& outProp,
		const bool (&hasSlot)[FacingCount],
		const void* mirrorModesRaw);

	PropSpriteLoader() = delete;
};

NAME_SPACE_END
