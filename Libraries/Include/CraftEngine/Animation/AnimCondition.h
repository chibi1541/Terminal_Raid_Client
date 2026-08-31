#pragma once

#include "Utils/EngineMacro.h"
#include "Utils/Types.h"
#include "Animation/AnimParameters.h"
#include <string>

NAME_SPACE_BEGIN(Craft)

// 조건을 평가할 때 필요한 "한 레이어의 지금 상황".
//
// animTime / animFinished / stateTime은 레이어마다 값이 다르다.
// 그래서 공용 블랙보드(AnimParameters)에 넣지 않고 평가할 때 같이 넘긴다.
// 넣어버리면 레이어끼리 값을 덮어쓰게 된다.
struct AnimEvalContext
{
	// 게임플레이가 채운 값들. AnimInstance가 소유한 것을 가리킨다.
	const AnimParameters* parameters = nullptr;

	// 현재 클립 재생 진행도 0.0 ~ 1.0
	float animTime = 0.0f;

	// 루프가 아닌 클립이 마지막 프레임에 도달했는지
	bool animFinished = false;

	// 현재 상태에 머문 시간(초)
	float stateTime = 0.0f;
};

enum class AnimCompareOp
{
	Equal,
	NotEqual,
	Greater,
	GreaterEqual,
	Less,
	LessEqual,
};

// 전이 조건 하나. "파라미터 연산자 값" 3토큰이 전부다.
//
// 표현식 파서를 두지 않은 건 의도다.
// 3토큰으로 제한하면 파싱이 확정적이라 오타를 로드 시점에 전부 잡을 수 있고,
// 복합 조건은 <Condition>을 여러 개 쓰는 것(AND)으로 충분히 표현된다.
struct CRAFT_API AnimCondition
{
	std::string parameterName;
	AnimCompareOp op = AnimCompareOp::Equal;
	float value = 0.0f;

	bool Evaluate(const AnimEvalContext& context) const;

	// "speed > 0" 같은 문자열을 파싱한다. 형식이 틀리면 false.
	//
	// 허용하는 형태:
	//   3토큰    "speed > 0"  "isAttacking == true"  "animTime ge 0.8"
	//   1토큰    "animFinished"  -> "animFinished == true"로 취급 (가장 흔한 형태라 축약을 허용)
	//
	// 연산자는 기호와 단어를 둘 다 받는다.
	//   ==/eq  !=/ne  >/gt  >=/ge  </lt  <=/le
	// XML 속성값에는 '<'를 그대로 못 넣어서(&lt;로 이스케이프해야 함) 단어형이 편하고,
	// 기호형은 Obsidian 엣지 라벨을 그대로 옮겨쓸 수 있어서 함께 받는다.
	//
	// 값은 숫자 또는 true / false.
	static bool Parse(const std::string& text, OUT AnimCondition& outCondition);

	// animTime / animFinished / stateTime 처럼 컨텍스트에서 오는 이름인지.
	// 로더가 "선언되지 않은 파라미터" 검사를 할 때 이걸로 걸러낸다.
	static bool IsBuiltInParameter(const std::string& name);
};

NAME_SPACE_END
