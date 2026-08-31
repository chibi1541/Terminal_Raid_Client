#pragma once

#include "Utils/EngineMacro.h"
#include "Utils/Types.h"
#include <string>
#include <unordered_map>

NAME_SPACE_BEGIN(Craft)

// 게임플레이가 애니메이션 계층에 값을 건네는 유일한 창구. (언리얼 AnimInstance의 프로퍼티들)
//
// 여기 담기는 건 "지금 캐릭터가 어떤 상태인가"를 나타내는 날것의 수치다.
//   speed, isAttacking, isFalling ...
// 어떤 클립을 틀지는 이 값을 보고 상태 머신이 정한다.
// 그래서 게임플레이 코드는 클립 이름을 몰라도 되고, 애니메이션은 게임플레이를 건드리지 않는다.
//
// 값을 전부 float 하나로 저장한다.
// bool은 0.0f / 1.0f, int도 float로 정확히 표현되므로 타입을 나눌 실익이 없고,
// 조건 비교(AnimCondition)도 float 비교 하나로 통일된다.
//
// 주의 - bool/int는 0/1처럼 정확한 값이라 '=='를 써도 안전하지만,
// 진짜 실수값(speed 등)에 '=='를 쓰면 부동소수점 비교가 되므로 '>=' / '<='를 쓸 것.
class CRAFT_API AnimParameters
{
public:
	void SetFloat(const std::string& name, float value);
	void SetBool(const std::string& name, bool value);
	void SetInt(const std::string& name, int value);

	float GetFloat(const std::string& name, float defaultValue = 0.0f) const;
	bool GetBool(const std::string& name, bool defaultValue = false) const;
	int GetInt(const std::string& name, int defaultValue = 0) const;

	bool Contains(const std::string& name) const;

	// 로더가 XML의 <Parameter>를 선언해두면, 조건에 적힌 파라미터 이름의 오타를
	// 로드 시점에 잡을 수 있다.
	inline int GetCount() const { return static_cast<int>(values.size()); }

private:
	std::unordered_map<std::string, float> values;
};

NAME_SPACE_END
