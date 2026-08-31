#pragma once

#include "Utils/EngineMacro.h"
#include "Asset/PrimaryDataAsset.h"
#include <string>
#include <unordered_map>

NAME_SPACE_BEGIN(Craft)

// 애니메이션 애셋의 이름 -> 파일 경로 매핑 테이블.
//
// 지금까지는 액터가 클립/상태 머신 파일 경로를 직접 하드코딩해서 들고 있었다.
// 이 애셋을 거치면 액터는 "Player" 같은 이름만 알면 되고,
// 그 이름이 어느 파일을 가리키는지는 데이터(XML) 쪽 관심사가 된다.
//
// 이 애셋 자체는 경로만 들고 있을 뿐 클립을 읽지 않는다.
// 실제 로드는 이름으로 경로를 찾은 호출부가 그 경로를 다시 넘겨서 한다:
//
//   auto animData = AssetManager::Get().GetPrimaryAsset<AnimationDataAsset>("AnimationData");
//   const std::wstring& clipPath = animData->FindClipPath("Player");
//   animator->LoadClipsFromFile(clipPath.c_str());
//
// 파일 형식 (Assets/AnimationData.xml):
//
//   <AnimationData>
//       <Clips>
//           <Clip name="Player" path="../Assets/TestActor.anim.xml" />
//           <Clip name="Slime"  path="../Assets/Slime.anim.xml" />
//       </Clips>
//       <StateMachines>
//           <StateMachine name="Player" path="../Assets/TestActor.fsm.xml" />
//       </StateMachines>
//   </AnimationData>
//
//   Clips/Clip                 : AnimationClip 정의 파일(*.anim.xml).
//   StateMachines/StateMachine : 상태 머신 정의 파일(*.fsm.xml 또는 *.canvas).
//                                상태 머신 없이 클립만 쓰는 이름도 있을 수 있어서,
//                                클립과 상태 머신은 서로 다른 목록으로 분리했다.
//
// 경로는 Config/ 와 같은 관습대로 프로젝트 폴더 기준 상대 경로다.
class CRAFT_API AnimationDataAsset : public PrimaryDataAsset
{
	TYPE_DECLARATIONS(AnimationDataAsset, PrimaryDataAsset)

public:
	AnimationDataAsset() = default;
	virtual ~AnimationDataAsset() = default;

	virtual bool LoadFromXml(XmlNode& root) override;

	// 이름으로 경로를 찾는다. 등록되지 않은 이름이면 빈 문자열.
	// (이름 오타는 그 경로로 로드를 시도하는 호출부에서 걸린다)
	const std::wstring& FindClipPath(const std::string& name) const;
	const std::wstring& FindStateMachinePath(const std::string& name) const;

	inline bool HasClip(const std::string& name) const
	{
		return clipPaths.find(name) != clipPaths.end();
	}

	inline bool HasStateMachine(const std::string& name) const
	{
		return stateMachinePaths.find(name) != stateMachinePaths.end();
	}

	inline int GetClipCount() const { return static_cast<int>(clipPaths.size()); }
	inline int GetStateMachineCount() const { return static_cast<int>(stateMachinePaths.size()); }

private:
	// <Clips>/<StateMachines> 처럼 형식이 같은 목록을 읽는 공통 처리.
	// listTag 자식이 없으면 아무것도 안 넣고 true를 반환한다(선택 항목).
	// 항목에 name이나 path가 비어있으면 false.
	static bool LoadPathMap(
		XmlNode& root,
		const WCHAR* listTag,
		const WCHAR* itemTag,
		std::unordered_map<std::string, std::wstring>& outMap);

private:
	std::unordered_map<std::string, std::wstring> clipPaths;
	std::unordered_map<std::string, std::wstring> stateMachinePaths;
};

NAME_SPACE_END
