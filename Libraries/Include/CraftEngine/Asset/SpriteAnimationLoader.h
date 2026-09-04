#pragma once

#include "Utils/EngineMacro.h"
#include "Utils/Types.h"
#include "Asset/AnimationClip.h"
#include <memory>
#include <string>
#include <vector>

NAME_SPACE_BEGIN(Craft)

// 스프라이트 애니메이션 정의 XML을 읽어서 AnimationClip 목록으로 만드는 로더.
//
// 파일 형식 (Assets/*.anim.xml):
//
//   <SpriteAnimation>
//       <Clip name="Idle" fps="3" loop="true">
//           <Frame>
//               .DDDDD.D
//               .DLLLL.D
//           </Frame>
//           <Frame> ... </Frame>
//       </Clip>
//   </SpriteAnimation>
//
//   name : 클립 이름. 상태 머신의 State가 지목하는 "논리" 이름이고,
//          SpriteAnimatorComponent::PlayClip()에 넘기는 그 이름이기도 하다.
//   facing : 선택. 이 그림이 그려진 시점(視點). "Up" "Right" "Down" "Left" "Side".
//          생략하면 방향이 없는 클립이 되어 네 방향 슬롯을 전부 차지한다
//          (= 이 속성이 생기기 전의 모든 파일이 예전과 똑같이 동작한다).
//
//          같은 name을 facing만 다르게 여러 번 적으면 하나의 방향 세트가 된다.
//          상태 머신은 계속 "Walk" 하나만 알고, 어느 슬롯을 그릴지는
//          SpriteAnimatorComponent::SetFacing()이 정한다 - 상태를 방향마다 복제하지 않는다.
//
//              <Clip name="Walk" facing="Down"> ... </Clip>
//              <Clip name="Walk" facing="Up">   ... </Clip>
//
//          "Side"는 측면 아트 한 장으로 좌우를 다 덮는다는 선언이다. 그림이 그려진 방향이
//          Right가 되고 반대편은 좌우 반전으로 채워진다(AnimInstance의 flipX가 처리하므로
//          미러 이미지를 따로 굽지 않는다). 좌우를 따로 그렸다면 Left/Right를 직접 쓴다.
//
//          ★ 빠진 슬롯은 AnimInstance::FinalizeClips()가 로드 시점에 채운다 ★
//          그래서 런타임에는 "이 슬롯이 비었나" 분기가 없다(PropSprite와 같은 계약).
//          채우는 순서는 다음과 같다.
//            1. 방향 없는 클립이 있으면 네 슬롯 전부 그것.
//            2. Side 선언은 Right에 들어간다.
//            3. 측면끼리 미러 - Left가 비고 Right가 진짜 아트면 Left = Right + 반전. (반대도 동일)
//               ★ 진짜로 선언된 슬롯에서만 미러한다 ★ 폴백으로 채워진 앞모습을 다시 미러하면
//               얼굴이 반대편에 붙은 그림이 나온다.
//            4. 앞뒤 폴백 - Up <- Down, Down <- Up. 반전하지 않는다
//               (앞모습으로 뒷모습을 때우는 것이지 거울이 아니다).
//            5. 그래도 비면 채워진 아무 슬롯(Right -> Down -> Up -> Left 순).
//
//          같은 (name, facing)을 두 번 적으면 로드할 때 크래시한다(조용한 덮어쓰기 방지).
//   fps  : 초당 넘길 프레임 수. 생략하면 12.
//   loop : 끝에서 처음으로 돌아갈지. 생략하면 true.
//   width / height : 선택. 적어두면 픽셀맵과 대조해서 다르면 크래시한다.
//          진실의 원천은 픽셀맵이고 이건 선언일 뿐이다.
//          Sprite 생성자는 "줄 길이가 서로 다른" 경우만 잡기 때문에,
//          줄이 통째로 빠져 8x8이 8x7이 된 실수는 이 선언이 있어야 걸린다.
//   pivot : 선택. "x,y" 형태. 액터 위치에 놓일 스프라이트 안의 점(셀 단위).
//          생략하면 가운데 맨 아래(발밑) = ((width-1)*0.5, height-1).
//          x가 실수인 이유는 짝수 너비의 가운데가 정수로 안 떨어지기 때문이다(8칸이면 3.5).
//          정수로 반올림해두면 좌우 반전할 때마다 그림이 한 칸씩 튄다.
//   Frame: 픽셀맵 한 장. 줄 앞뒤 공백은 버리므로 XML 들여쓰기를 자유롭게 써도 된다.
//          기호는 SymbolPalette의 16개 대문자 + 투명('.')만 쓸 수 있다.
//   Notify: 선택. 이 클립이 재생되는 동안 게임플레이에게 보낼 신호.
//          <Notify frame="3"   name="AttackHit" />  -> 3번 프레임에 진입할 때
//          <Notify frame="end" name="RollEnd"   />  -> 논루프 클립이 끝난 순간
//
//          frame="end"와 마지막 프레임 노티파이는 다르다. 마지막 프레임 노티파이는
//          그 장에 "들어갈 때" 울려서 클립이 실제로 끝나기 한 프레임 빠르다.
//          구르기 종료처럼 정확한 끝이 필요하면 frame="end"를 쓴다.
//
//          게임플레이는 SpriteAnimatorComponent::HasNotify("RollEnd")로 받는다.
//          큐는 프레임 단위 수명이라 그 프레임 안에서만 참이다.
//
//          로드할 때 잡는 것 - 이름 없음, 범위 밖 frame,
//          루프 클립에 frame="end"(영영 안 울림), 1프레임 클립에 frame="end".
//
// TODO : 지금은 액터가 파일 경로를 직접 들고 있다.
//        애셋 매니저 / 데이터 애셋이 생기면 그쪽에서 캐싱해 나눠주는 구조로 옮긴다.
class CRAFT_API SpriteAnimationLoader
{
public:
	// XML에서 클립 목록을 읽는다.
	// 파일이 없거나 파싱에 실패하면 빈 목록을 반환한다.
	// (애니메이션 하나 때문에 게임이 아예 안 뜨는 상황을 막기 위함 - Palette와 같은 방침)
	static std::vector<std::shared_ptr<const AnimationClip>> LoadFromFile(const WCHAR* path);

private:
	// XML 텍스트를 픽셀맵 문자열로 다듬는다.
	// 줄 앞뒤 공백 제거 + 빈 줄 제거 + '\n'으로 재조립.
	static std::string NormalizePixelMap(const std::string& rawText);

	SpriteAnimationLoader() = delete;
};

NAME_SPACE_END
