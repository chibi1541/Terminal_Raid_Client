#pragma once

#include "Utils/EngineMacro.h"
#include "Utils/Types.h"
#include "Animation/AnimInstance.h"

NAME_SPACE_BEGIN(Craft)

// 상태 머신 정의를 읽어서 AnimInstance에 BaseLayer/Overlay를 채우는 로더.
//
// 두 가지 포맷을 받는다.
//   *.fsm.xml  손으로 쓰는 XML. 상태 두어 개짜리 간단한 FSM은 이쪽이 빠르다.
//   *.canvas   Obsidian 캔버스(JSON). 그래프를 눈으로 보며 편집한다.
//
// 둘은 표기만 다를 뿐 만들어내는 결과가 같다.
// 캔버스를 중간 XML로 변환하지 않고 곧바로 읽는 이유는,
// 어차피 매 실행마다 JSON을 읽어야 해서 중간 파일이 아무것도 아껴주지 않기 때문이다.
// (저작 포맷과 런타임 포맷을 나누는 건 옳지만 그건 빌드 타임에 할 일이다)
//
// ★ 레이어는 정확히 2개, 역할이 고정이다 ★
// AnimInstance가 BaseLayer/Overlay 고정 슬롯을 가지므로(Animation/AnimInstance.h 참고),
// 로더가 만드는 레이어도 그 두 이름/역할에만 대응한다. 그 외 이름이나 3개 이상은 크래시.
//
// -------------------------------------------------------------------------
// XML 형식 (Assets/*.fsm.xml)
// -------------------------------------------------------------------------
//
//   <AnimStateMachine>
//       <Parameters>
//           <Parameter name="speed" value="0" />
//           <Parameter name="isRolling" value="false" />
//       </Parameters>
//
//       <Layer name="Base">
//           <States entry="Idle">
//               <State name="Idle" clip="Idle" />
//               <State name="Roll" clip="Roll" blend="false" />
//           </States>
//           <Transitions>
//               <Transition from="Idle" to="Roll">
//                   <Condition test="isRolling" />
//               </Transition>
//           </Transitions>
//       </Layer>
//
//       <Layer name="Overlay">
//           <States entry="Idle">
//               <State name="Idle" clip="Idle" rows="4-7" />
//               <State name="Walk" clip="Walk" rows="4-7" blend="false" />
//           </States>
//           <Transitions>
//               <Transition from="Idle" to="Walk">
//                   <Condition test="speed gt 0" />
//               </Transition>
//           </Transitions>
//       </Layer>
//   </AnimStateMachine>
//
//   Parameter  : 조건에서 쓸 파라미터 선언 + 기본값. 선언하지 않은 이름을 조건에 쓰면 크래시한다.
//                (오타를 실행 중이 아니라 로드 시점에 잡기 위한 장치)
//   Layer      : name은 정확히 "Base" 하나, 그리고 선택으로 "Overlay" 하나까지만 허용된다.
//                그 외 이름이나 3개 이상은 크래시. Overlay는 아예 생략해도 된다(단일 레이어 액터).
//   State      : clip은 이미 AnimInstance에 등록된 클립 이름이어야 한다.
//                따라서 클립을 먼저 로드하고 이걸 호출해야 한다.
//                empty="true"면 아무것도 출력하지 않는 상태가 되고 clip은 무시된다.
//                rows   : Overlay의 State에서만 쓴다. 담당 행 범위. 생략하면 전체.
//                         Base의 State에 rows를 쓰면 크래시한다(의미가 없는 자리라 실수로 본다).
//                blend  : 생략하면 true. 레이어 역할에 따라 뜻이 다르다(AnimState 참고).
//                           Base 상태   - false면 이 상태인 동안 Overlay를 통째로 숨긴다.
//                           Overlay 상태 - true면 투명한 칸으로 Base가 비친다.
//                                        false면 담당 행을 통째로 가져가 Base를 가린다.
//
//   Base와 Overlay의 클립은 폭(가로)이 달라도 된다(세로는 캐릭터별로 고정이라 항상 같아야 한다).
//   합성 캔버스는 항상 Base 클립의 크기다. Overlay는 두 클립의 피벗(발밑 등)이 같은 지점을
//   가리키도록 자동으로 정렬되어 얹힌다 - <Clip pivot="x,y">를 발밑 중심으로 맞춰 그리기만 하면
//   작가가 오프셋을 직접 계산할 필요가 없다. Overlay가 Base보다 넓으면 넘친 부분은 잘리고,
//   좁으면 Overlay가 닿지 않는 양 끝은 Base 자신의 그림이 그대로 남는다.
//   두 피벗이 정수 칸으로 안 맞아떨어지면(폭 홀짝이 서로 다름) 크래시하지 않고
//   가장 가까운 칸으로 반올림한다(최대 반 칸 오차 허용).
//   Transition : from을 비우면 Any State. 쓴 순서가 곧 우선순위다.
//   Condition  : "파라미터 연산자 값" 3토큰. 여러 개면 전부 참이어야 전이(AND).
//                연산자는 기호(> >= == != < <=)와 단어(gt ge eq ne lt le)를 모두 받는다.
//                XML 속성값에는 '<'를 그대로 못 넣으므로 단어형이 편하다.
//
// -------------------------------------------------------------------------
// 캔버스 형식 (Assets/**/*.canvas)
// -------------------------------------------------------------------------
//
// Obsidian 캔버스의 JSON 스키마는 건드리지 않는다.
// 노드 텍스트와 엣지 라벨 안의 표기 규약만 정해서 읽는다.
// 그래야 Obsidian을 평범한 편집기로 계속 쓸 수 있다.
//
//   그룹 라벨    "Base" 또는 "Overlay" - 이름만 쓴다. [layer=N] 같은 순서 태그는 없다.
//                그룹 0개 : 전체를 그룹 없는 Base 레이어 하나로 본다.
//                그룹 1개 : 반드시 "Base"여야 한다.
//                그룹 2개 : 반드시 "Base"와 "Overlay" 각각 하나씩이어야 한다.
//                그 외(3개 이상, 이름이 안 맞음)는 크래시.
//
//   노드 텍스트  "# Idle [entry] [rows=4-7] [blend=false]"  +  "clip: Idle"
//                첫 줄이 상태 이름 + [태그]들, 이후 줄은 "키: 값"(지금은 clip만 쓴다).
//                Obsidian이 마크다운으로 렌더하므로 줄 앞의 # - * 는 떼어내고 읽는다.
//                태그 : [entry] 시작 상태, [empty] 아무것도 출력하지 않는 상태,
//                       [rows=A-B] 담당 행(Overlay 전용, Base에 쓰면 크래시),
//                       [blend=true|false] XML의 blend와 동일.
//                clip: 생략 시 상태 이름을 클립 이름으로 쓴다.
//                예약 이름 : Any(Any State 의사 노드), Parameters(파라미터 선언 노드)
//
//   엣지 라벨    "[0] IsDead == true"        선두 [N]은 우선순위(작을수록 먼저)
//                "IsAttack == true && speed gt 0"   && 로 AND
//                (빈 라벨)                   무조건 전이
//
//   소속 판정    노드 중심점이 들어가는 가장 작은 그룹.
//                완전 포함이 아니라 중심점이라 노드가 살짝 삐져나가도 유지된다.
//
// 주의 - Obsidian은 팬/줌만 해도 파일을 다시 쓰면서 nodes/edges 배열 순서를 뒤섞는다.
// 그래서 배열 순서에 의존하지 않는다. 상태는 이름순, 전이는 [N] 다음 (from, to, label) 순으로
// 정렬해서 결과를 항상 같게 만든다. XML과 달리 캔버스에서 전이 우선순위를 제어하는 방법은
// [N] 태그뿐이다.
class CRAFT_API AnimStateMachineLoader
{
public:
	// 확장자를 보고 포맷을 고른다. .canvas면 Obsidian 그래프, 아니면 XML.
	// 호출부는 어느 포맷인지 몰라도 된다.
	static int LoadIntoInstance(const WCHAR* path, AnimInstance& outInstance);

	// 포맷을 명시해서 부르고 싶을 때.
	// 둘 다 채운 레이어 수(0~2)를 반환하고, 파일이 없거나 파싱에 실패하면 0이다.
	// (애니메이션 하나 때문에 게임이 아예 안 뜨는 상황을 막기 위함 - Palette와 같은 방침)
	static int LoadXmlIntoInstance(const WCHAR* path, AnimInstance& outInstance);
	static int LoadCanvasIntoInstance(const WCHAR* path, AnimInstance& outInstance);

private:
	AnimStateMachineLoader() = delete;
};

NAME_SPACE_END
