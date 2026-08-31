#pragma once

#include "Utils/EngineMacro.h"
#include "Utils/Types.h"
#include "Animation/AnimCondition.h"
#include <string>
#include <vector>

NAME_SPACE_BEGIN(Craft)

// 레이어가 담당하는 행 범위. [startRow, endRow] 양끝 포함.
//
// 본이 없는 2D 픽셀에서 언리얼의 Layered blend per bone에 대응하는 것이 행 마스크다.
// 8x8 캐릭터면 0~3행이 상체, 4~7행이 하체쯤 된다.
//
// 5단계부터는 AnimLayer가 아니라 AnimState가 이 값을 갖는다.
// (Overlay 레이어의 어느 상태냐에 따라 담당 영역이 달라질 수 있어서다.
// BaseLayer는 항상 전신을 담당하므로 이 값을 갖지 않는다.)
struct CRAFT_API AnimLayerMask
{
	int startRow = 0;

	// -1이면 스프라이트 끝까지(= 전체).
	int endRow = -1;

	bool Contains(int row) const;
};

// 상태 하나. "이 상태일 때는 이 클립을 튼다"가 전부다.
// 상태가 하는 일이 이것뿐이라 클립 재생은 AnimationPlayer가, 전이는 AnimStateMachine이 맡는다.
struct CRAFT_API AnimState
{
	std::string name;

	// AnimInstance에 등록된 클립 이름. (AnimationClip::GetName())
	std::string clipName;

	// Overlay 레이어의 상태에서만 의미가 있다. 이 상태가 담당하는 행 범위.
	// BaseLayer는 항상 전신을 담당하므로 이 값은 무시된다.
	AnimLayerMask region;

	// 레이어 역할에 따라 뜻이 다르다. (AnimInstance.h의 Composite 주석 참고)
	//
	//   BaseLayer 상태   : true(기본값)면 Overlay를 평소대로 얹는다.
	//                      false면 이 상태가 재생되는 동안 Overlay를 통째로 숨긴다.
	//                      구르기/사망/피격경직처럼 전신이 한 클립으로 통일돼야 하는 상태에 쓴다.
	//   Overlay 상태     : true(기본값)면 투명한 칸으로 BaseLayer가 비친다.
	//                      false면 담당 행(region)을 통째로 가져가 BaseLayer를 완전히 가린다.
	//                      다리를 드느라 픽셀을 "빼는" 표현은 false가 아니면 반영되지 않는다.
	bool canBlend = true;
};

// 상태 사이의 화살표.
struct CRAFT_API AnimTransition
{
	// 비어있으면 Any State - 어느 상태에서든 이 전이를 검사한다. (언리얼과 같은 개념)
	std::string fromStateName;

	std::string toStateName;

	// 전부 참이어야 전이한다(AND). 비어있으면 무조건 전이.
	std::vector<AnimCondition> conditions;
};

// 상태 집합 + 전이 규칙 + 현재 상태. (언리얼 AnimGraph의 State Machine 노드)
//
// 자기가 클립을 재생하지는 않는다. "지금 어느 상태인가"만 답한다.
// 실제 재생은 이 상태 머신을 소유한 AnimLayer의 AnimationPlayer가 한다.
class CRAFT_API AnimStateMachine
{
public:
	void AddState(const AnimState& state);
	void AddTransition(const AnimTransition& transition);

	// 시작 상태를 지정하고 현재 상태를 거기로 맞춘다. 없는 이름이면 false.
	bool SetEntryState(const std::string& name);

	// 조건을 평가해서 현재 상태를 갱신한다. 상태가 바뀌었으면 true.
	//
	// 전이 우선순위는 AddTransition으로 들어간 순서다(= XML에 쓴 순서).
	// Any State 전이도 같은 목록에 있으므로 먼저 검사받고 싶으면 위에 쓰면 된다.
	bool Evaluate(const AnimEvalContext& context);

	// 지금 상태. 상태가 하나도 없으면 nullptr.
	const AnimState* GetCurrentState() const;

	// 현재 상태를 시작 상태로 되돌린다.
	void Reset();

	// getter (로더 검증용)
	inline bool IsEmpty() const { return states.empty(); }
	inline int GetStateCount() const { return static_cast<int>(states.size()); }
	inline int GetTransitionCount() const { return static_cast<int>(transitions.size()); }
	bool HasState(const std::string& name) const;

	// 목록 그대로 읽기. 범위를 벗어나면 nullptr.
	// 전이는 인덱스 순서가 곧 우선순위라, 로더가 의도한 순서로 넣었는지 확인할 때 쓴다.
	const AnimState* GetState(int index) const;
	const AnimTransition* GetTransition(int index) const;

private:
	int FindStateIndex(const std::string& name) const;

	// 이번 틱에 발동할 첫 번째 전이를 찾는다. 없으면 nullptr.
	const AnimTransition* FindFirstMatchingTransition(const AnimEvalContext& context) const;

private:
	// 한 틱에 전이가 무한히 연쇄하는 걸 막는 상한.
	// (A->B 조건과 B->A 조건이 동시에 참인 데이터 실수를 게임이 멈추지 않고 넘기기 위함)
	enum { MaxTransitionsPerTick = 3, };

	std::vector<AnimState> states;

	// 순서 = 우선순위.
	std::vector<AnimTransition> transitions;

	int currentStateIndex = -1;
	int entryStateIndex = -1;
};

NAME_SPACE_END
