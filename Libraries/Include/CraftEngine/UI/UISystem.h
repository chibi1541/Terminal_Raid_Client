#pragma once

#include "UI/Widget.h"

#include <memory>
#include <vector>

NAME_SPACE_BEGIN(Craft)

namespace UI
{
	// 화면에 올라간 위젯들을 관리하는 시스템.
	// 언리얼의 UGameViewportClient(+ AddToViewport)에 해당한다.
	//
	// 위젯은 Actor가 아니라서 Level의 액터 목록에 없다.
	// 따라서 갱신과 그리기를 자동으로 받지 못하고, 이 시스템이 직접 돌려야 한다.
	//
	// 매 프레임 하는 일:
	//   Tick()   위젯의 값 갱신(체력바 수치 등)
	//   Paint()  레이아웃 두 패스를 돌린 뒤 그리기
	//
	// 배치를 Tick이 아니라 Paint 진입 시점에 하는 이유는,
	// 이번 프레임에 바뀐 값(글자 길이 등)을 같은 프레임에 반영하기 위해서다.
	// Tick보다 먼저 배치하면 항상 한 프레임 늦은 크기로 그려진다.
	class CRAFT_API UISystem
	{
		// 뷰포트에 올라간 위젯 하나.
		struct ViewportEntry
		{
			std::shared_ptr<Widget> widget;

			// 겹침 순서. 큰 값이 위에 그려진다.
			int zOrder = 0;

			// 레벨이 바뀌어도 살아남을지 여부.
			//
			// 대부분의 UI는 레벨에 딸린 것이라(HUD 등) 레벨이 바뀌면 같이 사라져야 한다.
			// 메인 메뉴처럼 레벨을 넘나드는 것만 참으로 둔다.
			bool persistent = false;
		};

	public:
		UISystem();
		~UISystem();

		// 싱글턴이므로 복사할 일이 없다.
		UISystem(const UISystem&) = delete;
		UISystem& operator=(const UISystem&) = delete;

		// 전역 접근.
		static UISystem& Get();

		// 엔진 종료 순서 때문에 존재 여부를 물어봐야 할 때가 있다.
		static bool HasInstance();

		// 위젯을 화면에 올린다.
		//
		// 실제 추가는 이번 프레임 끝에 이뤄진다(ProcessPendingWidgets).
		// 목록을 순회하는 도중에 목록이 바뀌는 상황을 없애기 위해서인데,
		// 이건 Level이 액터를 다루는 방식과 같다.
		void AddToViewport(const std::shared_ptr<Widget>& widget, int zOrder = 0, bool persistent = false);

		// 위젯을 화면에서 내린다. 실제 제거도 이번 프레임 끝에 이뤄진다.
		//
		// 즉시 제거하면 안 되는 결정적인 이유가 있다.
		// 입력 콜백 안에서 창을 닫는 경우(Esc를 눌러 메뉴 닫기),
		// 그 자리에서 위젯과 입력 핸들러가 죽으면 InputSystem의 키 소유권이 풀려서
		// 같은 프레임의 남은 이벤트가 게임플레이로 새어 내려간다.
		void RemoveFromViewport(const std::shared_ptr<Widget>& widget);

		// 레벨에 딸린 위젯을 전부 내린다. 레벨 교체 직전에 엔진이 부른다.
		void ClearNonPersistent();

		// 지연된 추가/제거를 실제로 반영한다. 프레임 끝에 엔진이 부른다.
		void ProcessPendingWidgets();

		void Tick(float deltaTime);

		// 레이아웃을 갱신하고 화면에 그린다.
		void Paint();

		// 현재 올라가 있는 위젯 수(대기 중인 것은 제외).
		inline int GetWidgetCount() const { return static_cast<int>(entries.size()); }

	private:
		// 뷰포트 전체 영역. 렌더러가 실제로 잡은 화면 크기를 쓴다.
		Rect GetViewportRect() const;

		// zOrder 오름차순으로 정렬한다. 먼저 그린 것이 아래에 깔린다.
		void SortEntries();

	private:
		static UISystem* instance;

		// 화면에 올라가 있는 위젯들. zOrder 오름차순.
		std::vector<ViewportEntry> entries;

		// 이번 프레임에 추가 요청된 것들.
		std::vector<ViewportEntry> pendingAddEntries;

		// 이번 프레임에 제거 요청된 것들.
		std::vector<std::shared_ptr<Widget>> pendingRemoveWidgets;

		// 정렬이 필요한지 여부. 추가될 때만 켜진다.
		bool needsSort = false;
	};
}

NAME_SPACE_END
