#pragma once

#include "Core/CraftObject.h"
#include "Math/Rect.h"

NAME_SPACE_BEGIN(Craft)

class Actor;

class CRAFT_API Level : public CraftObject, public std::enable_shared_from_this<Level>
{
	// friend 선언
	friend class Engine;

	TYPE_DECLARATIONS(Level, CraftObject)

public:
	Level();
	virtual ~Level();

	virtual void OnInitialized();

	// 게임 플레이 이벤트 함수.
	virtual void BeginPlay();
	virtual void Tick(float deltaTime);
	virtual void Draw();

	// 액터 추가 함수(템플릿)
	// Actor를 상속한 타입만 받도록 설정()
	// Args가 타입 추론이 들어가야 하기 때문에 보편 참조가 됨
	// typename = std::enable_if_t<std::is_base_of<Actor, T>::value> 조건이 false가 되면 해당 타입의 템플릿이 생성되지 않는다고 함
	template<typename T, typename ...Args, 
		typename = std::enable_if_t<std::is_base_of<Actor, T>::value>>
	std::shared_ptr<T> SpawnActor(Args&& ...args)
	{
		std::shared_ptr<T> newActor = std::make_shared<T>(std::forward<Args>(args)...);

		addRequestedActorList.emplace_back(newActor);

		// TODO : ownership setting(has - a 관계 정립)
		// 언리얼에서 네트워크 쓰면 outer private에 서버가 있는 경우가 있나?
		newActor->SetOwner(weak_from_this());

		// 생성한 액터 반환
		return newActor;
	}

	// 액터 검색 함수(템플릿)
	// std::enable_if_t : std::enable_if_t<bool _Test, class _Ty = void>::type
	// _Test가 true인 경우만 템플릿 선언이 되어 있음
	// struct enable_if<true, _Ty> { _Test using type = _Ty; } 
	// 즉 struct enable_if<true, void> { _Test using type = void; } ::type -> void가 반환됨
	// 근데 만약 _Test 케이스가 false인 경우 해당하는 template 타입이 없어서 에러가 발생함
	//
	template<typename T, 
		typename = std::enable_if_t<std::is_base_of<Actor, T>::value>>
		std::shared_ptr<T> FindActor()

	{
		// 검색 - 형변환
		for (const auto& actor : actorList)
		{
			// T 타입으로 형변환 시도
			std::shared_ptr<T> targetActor = std::dynamic_pointer_cast<T>(actor);

			if (targetActor)
			{
				return targetActor;
			}
		}

		return nullptr;
	}

	template<typename T,
		typename = std::enable_if_t<std::is_base_of<Actor, T>::value>>
		vector<std::shared_ptr<T>> FindActors()

	{
		vector<std::shared_ptr<T>> ret;

		// 검색 - 형변환
		for (const auto& actor : actorList)
		{
			// T 타입으로 형변환 시도
			std::shared_ptr<T> targetActor = std::dynamic_pointer_cast<T>(actor);

			if (targetActor)
			{
				ret.push_back(targetActor);
			}
		}

		return ret;
	}


	// getter/setter
	inline bool HasInitialized() const { return hasInitialized; }

	// 카메라 클램프에 쓰이는 월드 경계(콘솔 셀 단위).
	//
	// 비어 있으면(기본값) 클램프하지 않는다 = 무한 월드, 기존 동작 유지.
	// CameraManager가 레벨 교체 후 이 값을 읽으므로, 경계는 Level 생성자나
	// OnInitialized 초반에 설정하는 것을 규약으로 한다.
	inline Rect GetWorldBounds() const { return worldBounds; }
	inline void SetWorldBounds(const Rect& bounds) { worldBounds = bounds; }

protected:
	void ProcessAddAndDestoryActors();

protected:

	bool hasInitialized = false;

	// 월드 경계. 기본값은 비어 있음(size 0) = 클램프 없음.
	Rect worldBounds;

	// 레벨에 배치된 모든 액터
	std::vector<std::shared_ptr<Actor>> actorList;

	// 레벨에 추가 요청된 액터를 저장해두는 목록
	// 현재 프레임을 처리하는 과정에서 액터 추가 요청이 발생하면
	// 해당 액터를 바로 추가하면 기존 액터 처리에 문제가 발생할 수 있어서
	// 현재 프레임을 모두 처리한 후에 추가 요청된 액터를 actorList로 옮김
	std::vector<std::shared_ptr<Actor>> addRequestedActorList;
};

NAME_SPACE_END