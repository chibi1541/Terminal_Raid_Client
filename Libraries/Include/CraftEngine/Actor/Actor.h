#pragma once

#include "Math/Vector2.h"
#include "Math/Color.h"
#include "Core/CraftObject.h"
#include "Component/ActorComponent.h"
#include <memory>
#include <vector>
#include <type_traits>

NAME_SPACE_BEGIN(Craft)

class Level;

class CRAFT_API Actor : public CraftObject, public std::enable_shared_from_this<Actor>
{
	TYPE_DECLARATIONS(Actor, CraftObject)

public:
	Actor( const std::string& image = "", const Vector2& position = Vector2::Zero, Color color = Color::White);
	virtual ~Actor();

	// 게임 플레이 이벤트 함수.
	// 오버라이드할 때는 반드시 super::XXX()를 호출할 것.
	// 컴포넌트로 이벤트를 전달하는 처리가 이 기본 구현에 들어있다.
	virtual void BeginPlay();
	virtual void Tick(float deltaTime);
	virtual void Draw();

	// 액터 제거 함수.
	void Destroy();

	// 게임(엔진) 종료 함수.
	void QuitGame();

	// 컴포넌트 추가 함수(템플릿).
	// ActorComponent를 상속한 타입만 받는다.
	//
	// 주의 - 생성자에서 호출하면 안 된다.
	// weak_from_this()는 이 액터가 shared_ptr로 감싸진 뒤에야 유효한데,
	// 생성자 시점에는 아직 shared_ptr이 만들어지기 전이라 빈 weak_ptr이 들어간다.
	// 컴포넌트 추가는 BeginPlay()에서 할 것.
	template<typename T, typename ...Args,
		typename = std::enable_if_t<std::is_base_of<ActorComponent, T>::value>>
	std::shared_ptr<T> AddComponent(Args&& ...args)
	{
		std::shared_ptr<T> newComponent = std::make_shared<T>(std::forward<Args>(args)...);

		// ownership setting
		newComponent->SetOwner(weak_from_this());

		componentList.emplace_back(newComponent);

		return newComponent;
	}

	// 컴포넌트 검색 함수(템플릿). 같은 타입이 여러 개면 처음 것을 반환한다.
	template<typename T,
		typename = std::enable_if_t<std::is_base_of<ActorComponent, T>::value>>
	std::shared_ptr<T> GetComponent() const
	{
		for (const std::shared_ptr<ActorComponent>& component : componentList)
		{
			// 엔진 자체 타입 시스템(Is/TypeId)을 쓰는 Cast로 형변환 시도.
			std::shared_ptr<T> targetComponent = Cast<T>(component);

			if (targetComponent)
			{
				return targetComponent;
			}
		}

		return nullptr;
	}

	template<typename T,
		typename = std::enable_if_t<std::is_base_of<ActorComponent, T>::value>>
		std::vector<std::shared_ptr<T>> GetComponents() const
	{
		std::vector<std::shared_ptr<T>> ret;
		for (const std::shared_ptr<ActorComponent>& component : componentList)
		{
			// 엔진 자체 타입 시스템(Is/TypeId)을 쓰는 Cast로 형변환 시도.
			std::shared_ptr<T> targetComponent = Cast<T>(component);

			if (targetComponent)
			{
				ret.emplace_back(targetComponent);
			}
		}

		return ret;
	}

	// getter/setter
	inline bool HasBeganPlay() const { return hasBeganPlay; }
	inline bool IsActive() const { return isActive && !hadExpired; }
	inline bool HasExpired() const { return hadExpired; }
	// ownership(약참조하던 객체를 shared_ptr로 만들어서 반환)
	inline std::shared_ptr<Level> GetOwner() const { return owner.lock(); }
	inline void SetOwner(std::weak_ptr<Level> newOwner) { owner = newOwner; }

	inline Vector2 GetPosition() const { return position; }
 	virtual void SetPosition(const Vector2& newPosition);

	// 컴포넌트가 액터의 정렬 순서를 그대로 쓸 수 있도록 공개.
	inline int GetSortingOrder() const { return sortingOrder; }
	inline void SetSortingOrder(int newSortingOrder) { sortingOrder = newSortingOrder; }

protected:
	// BeginPlay
	bool hasBeganPlay = false;

	// 액터 활성화 여부 플래그
	bool isActive = true;

	// 삭제 요청 여부 플래그
	// 쓰레드 분리?
	bool hadExpired = false;

	// ownership
	// 순환 참조 문제를 없애기 위해서 weak_ptr로 참조
	// 그래서 사용할 때는 유효한지 확인해야 함
	std::weak_ptr<Level> owner;

	// 이 액터가 소유한 컴포넌트 목록.
	// 액터가 소유하는 쪽이므로 shared_ptr.
	std::vector<std::shared_ptr<ActorComponent>> componentList;

	// 실제 화면에 그릴 글자
	std::string image;

	// 글자 색상
	Color color = Color::White;

	// 글자 길이
	int width = 0;

	int sortingOrder = 0;

	Vector2 position;
};

NAME_SPACE_END
