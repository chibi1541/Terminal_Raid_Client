#include "pch.h"
#include "TestActor.h"
#include "Input/Input.h"
#include "Render/Renderer.h"
#include "Asset/AssetManager.h"
#include "Asset/AnimationDataAsset.h"

#include "UI/UISystem.h"
#include "UI/Border.h"
#include "UI/TextBlock.h"

// 애니메이션 애셋 경로.
// 실행 파일이 아니라 프로젝트 폴더 기준의 상대 경로다(Config/ 와 같은 관습).
//
// TODO : 애셋 매니저 / 데이터 애셋이 생기면 액터가 경로를 직접 들고 있지 않게 바꾼다.
//        지금은 어떤 액터가 어떤 애니메이션을 쓰는지 한눈에 보이도록 여기 하드코딩해 둔다.
static const WCHAR* clipAssetPath = L"../Assets/TestActor.anim.xml";

// 상태 머신은 XML과 Obsidian 캔버스 둘 다 읽을 수 있다.
// 확장자를 보고 로더가 알아서 고르므로 경로만 바꾸면 된다.
//   ../Assets/TestActor.fsm.xml  손으로 쓴 XML (Idle <-> Walk)
static const WCHAR* stateMachineAssetPath =
	L"../Assets/StateMachine/StateMachine/player_anim_state_test.canvas";

TestActor::TestActor(const Vector2& position, Color color)
	: super("", position, color)
{
	// 주의 - 여기서 AddComponent를 호출하면 안 된다.
	// 아직 shared_ptr로 감싸지기 전이라 weak_from_this()가 비어있다.
}

void TestActor::BeginPlay()
{

	animator = AddComponent<SpriteAnimatorComponent>();

	auto animData = AssetManager::Get().GetPrimaryAsset<AnimationDataAsset>("AnimationData");

	// 클립 로드는 워커 쓰레드가 한다. 여기서는 요청만 걸고 바로 다음 줄로 넘어간다.
	// 클립이 도착할 때까지 몇 프레임 동안은 그릴 게 없어서 캐릭터가 안 보이지만,
	// 그동안 게임은 멈추지 않는다(동기 로드였다면 이 줄에서 프레임이 통째로 섰다).
	const std::wstring stateMachinePath = animData->FindStateMachinePath("TestActor");

	// 콜백이 도착하기 전에 액터가 파괴될 수 있다.
	// 그때는 아무것도 하지 않고 조용히 빠져나가야 한다.
	std::weak_ptr<Actor> weakSelf = weak_from_this();

	animator->LoadClipsFromFileAsync(animData->FindClipPath("TestActor").c_str(),
		[weakSelf, stateMachinePath](int loadedClipCount)
		{
			std::shared_ptr<TestActor> self = Cast<TestActor>(weakSelf.lock());

			if (nullptr == self)
			{
				return;
			}

			// 애셋을 못 읽으면 아무것도 안 그려져서 원인을 찾기 어렵다. 여기서 바로 잡는다.
			// (경로가 틀렸거나 XML 형식이 깨진 경우)
			//
			// 이 콜백은 메인 쓰레드에서 불리므로 여기서 터지면 콜스택이 그대로 읽힌다.
			ASSERT_CRASH(loadedClipCount > 0);

			// 상태의 clip 이름을 검증하기 때문에 클립을 먼저 읽어야 한다.
			// 그래서 상태 머신 로드가 클립 완료 콜백 안으로 들어왔다.
			const int loadedLayerCount = self->animator->LoadStateMachineFromFile(stateMachinePath.c_str());
			ASSERT_CRASH(loadedLayerCount > 0);
		});

	// scaleX/scaleY로 셀 비율 보정 + 크기 조절.
	// 셀이 가로로 길면 scaleY를, 세로로 길면 scaleX를 키운다.
	animator->SetScale(1, 1);

	// 게임플레이 입력 바인딩.
	//
	// 액터가 매 틱 키를 물어보는 게 아니라, 어떤 키에 무엇을 할지만 등록한다.
	// 어떤 키가 눌렸는지 찾아서 부르는 일은 InputSystem이 한다.
	inputComponent = AddComponent<InputComponent>();
	inputComponent->SetInputPriority(InputPriority::Gameplay);

	// 이동은 눌려 있는 동안 계속 필요하므로 Held.
	// 네 방향을 따로 걸어두면 대각선 입력이 자연스럽게 합쳐진다.
	// 콘솔 좌표는 y가 아래로 갈수록 커지므로 위/아래가 반대다.
	inputComponent->BindKey('A', EInputEvent::Held, this, &TestActor::OnMoveLeft);
	inputComponent->BindKey('D', EInputEvent::Held, this, &TestActor::OnMoveRight);
	inputComponent->BindKey('W', EInputEvent::Held, this, &TestActor::OnMoveUp);
	inputComponent->BindKey('S', EInputEvent::Held, this, &TestActor::OnMoveDown);

	inputComponent->BindKey(VK_LBUTTON, EInputEvent::Pressed, this, &TestActor::OnAttack);
	inputComponent->BindKey(VK_LBUTTON, EInputEvent::Held, this, &TestActor::OnAttack);

	// 구르기는 누른 순간 한 번만 시작되어야 하므로 Pressed.
	inputComponent->BindKey(VK_SPACE, EInputEvent::Pressed, this, &TestActor::OnRollPressed);

	// 마우스 왼쪽 버튼을 누르고 있는 동안 매 프레임 호출되는지 확인용.
	inputComponent->BindKey(VK_LBUTTON, EInputEvent::Held, this, &TestActor::OnMouseHeld);

	// Component Register
	//inputComponent->GetHandler().Register();

	// 우선순위 계층과 모달 차단을 확인하기 위한 두 번째 컴포넌트.
	// UI 밴드에 있으므로 게임플레이보다 항상 먼저 처리된다.
	blockerInput = AddComponent<InputComponent>();
	blockerInput->SetInputPriority(InputPriority::UI);
	blockerInput->BindKey('B', EInputEvent::Pressed, this, &TestActor::OnToggleBlock);

	// 일시정지 메뉴. M으로 열고 Esc로 닫는다.
	inputComponent->BindKey('M', EInputEvent::Pressed, this, &TestActor::OnOpenMenu);

	// 입력 래치 확인용 바인딩.
	//
	// 메뉴가 Esc를 소비해 자기를 닫는 순간, 그 Esc가 여기로 새면 안 된다.
	// 세 가지를 다 거는 이유는 Pressed만 막고 Held/Released가 새는 경우를 잡기 위해서다.
	// (InputSystem은 Pressed를 보낸 직후 같은 프레임에 Held도 보낸다)
	inputComponent->BindKey(VK_ESCAPE, EInputEvent::Pressed, this, &TestActor::OnEscapePressed);
	inputComponent->BindKey(VK_ESCAPE, EInputEvent::Released, this, &TestActor::OnEscapeReleased);
	inputComponent->BindKey(VK_ESCAPE, EInputEvent::Held, this, &TestActor::OnEscapeHeld);

	CreatePauseMenu();

	super::BeginPlay();
}

void TestActor::CreatePauseMenu()
{
	auto title = UI::Widget::Create<UI::TextBlock>("== PAUSED ==\n\nEsc : close", Color::Yellow);

	// 패널 배경 위에 얹으므로 글자 뒤 색을 패널과 맞춘다.
	// 빼면 글자가 있는 칸만 배경이 검게 파인다.
	title->SetBackgroundColor(Color::DarkBlue);

	auto frame = UI::Widget::Create<UI::Border>();
	frame->SetBackgroundColor(Color::DarkBlue);
	frame->SetShowBorder(true);
	frame->SetBorderColor(Color::White);
	frame->SetPadding(UI::Margin(2, 1));

	// 화면을 채우지 않고 내용 크기만큼만 잡아 가운데에 놓는다.
	frame->SetHorizontalAlignment(UI::EHorizontalAlignment::Center);
	frame->SetVerticalAlignment(UI::EVerticalAlignment::Center);
	frame->SetContent(title);

	pauseMenu = UI::Widget::Create<UI::UserWidget>();
	pauseMenu->SetRootWidget(frame);

	// 열려 있는 동안에는 바인딩이 없는 키까지 전부 삼킨다.
	// 모달 다이얼로그와 같은 동작이고, 그동안 캐릭터는 움직이지 않는다.
	pauseMenu->SetBlockAllInput(true);

	// Esc로 스스로 닫힌다.
	//
	// this가 아니라 위젯 자신을 캡처한다. 이 콜백은 위젯의 핸들러가 들고 있고,
	// 핸들러의 수명은 위젯과 같으므로 안전하다.
	UI::UserWidget* menu = pauseMenu.get();

	pauseMenu->BindKey(VK_ESCAPE, EInputEvent::Pressed,
		[menu]()
		{
			menu->SetVisibility(UI::EVisibility::Hidden);
		});

	// 처음에는 닫혀 있다.
	// 숨겨진 위젯은 그려지지도 않고 입력도 받지 않는다(EnabledCheck가 막는다).
	pauseMenu->SetVisibility(UI::EVisibility::Hidden);

	UI::UISystem::Get().AddToViewport(pauseMenu);
}

void TestActor::OnOpenMenu()
{
	if (nullptr == pauseMenu)
	{
		return;
	}

	pauseMenu->SetVisibility(UI::EVisibility::Visible);
}

void TestActor::OnEscapePressed()
{
	++escapePressedCount;
}

void TestActor::OnEscapeReleased()
{
	++escapeReleasedCount;
}

void TestActor::OnEscapeHeld()
{
	++escapeHeldCount;
}

void TestActor::OnMoveLeft()
{
	inputDirection.x -= 1;
}

void TestActor::OnMoveRight()
{
	inputDirection.x += 1;
}

void TestActor::OnMoveUp()
{
	inputDirection.y -= 1;
}

void TestActor::OnMoveDown()
{
	inputDirection.y += 1;
}

void TestActor::OnAttack()
{
	isAttack = true;
}

void TestActor::OnRollPressed()
{
	isRolling = true;

	// 스페이스를 꾹 눌러도 이 값이 1만 올라야 정상이다.
	// 계속 오르면 자동 반복 이벤트가 Pressed로 새어 들어온 것이다.
	++rollPressCount;
}

void TestActor::OnMouseHeld()
{
	// 버튼을 누르고 있는 동안 계속 올라야 정상이다.
	// 마우스를 움직이지 않아도 멈추면 안 된다.
	++mouseHeldFrameCount;
}

void TestActor::OnToggleBlock()
{
	isBlocking = !isBlocking;

	// 켜면 이 핸들러가 모든 키를 소비해서 게임플레이 쪽으로 아무것도 안 내려간다.
	// 모달 다이얼로그가 열린 상황과 같다.
	blockerInput->SetBlockAllInput(isBlocking);
}

void TestActor::Tick(float deltaTime)
{
	// 컴포넌트(= 애니메이션 평가/재생)로 deltaTime을 전달하는 처리가 여기 들어있다.
	super::Tick(deltaTime);

	const bool isMoving = (inputDirection != Vector2::Zero);

	// 예외 처리 - BeginPlay 전에는 컴포넌트가 없다.
	if (nullptr != animator)
	{
		// 애니메이션이 게임플레이에게 보내는 유일한 신호.
		//
		// 파이프라인은 "게임플레이 -> 파라미터 -> 전이 -> 클립"으로 흐르는 단방향인데,
		// 구르기 종료만은 반대 방향이 필요하다. 클립이 끝났다는 걸 여기서 듣고 조종을 되돌린다.
		// 이게 없으면 IsRolling이 계속 참이라 Any->Roll이 매번 다시 잡아채 구르기에 갇힌다.
		//
		// super::Tick(deltaTime)이 컴포넌트를 먼저 돌렸으므로 이 프레임 것을 바로 읽을 수 있다.
		if (animator->HasNotify("RollEnd"))
		{
			isRolling = false;
		}

		// 게임플레이가 애니메이션에 넘기는 건 이 값들뿐이다.
		// 어떤 클립을 틀지는 상태 머신의 전이 규칙이 정한다.
		// 여기에 클립 이름이 등장하지 않는 게 이 구조의 핵심이다.
		animator->GetParameters().SetFloat("speed", isMoving ? moveSpeed : 0.0f);
		animator->GetParameters().SetFloat("IsAttack", isAttack ? 1.0f : 0.0f);
		animator->GetParameters().SetFloat("IsRolling", isRolling ? 1.0f : 0.0f);

		// 가로 입력이 있을 때만 방향을 바꾼다.
		// 위아래로만 움직이거나 멈춰 있는 동안에는 보던 방향을 유지한다.
		if (inputDirection.x != 0)
		{
			animator->SetFlipX(inputDirection.x < 0);
		}
	}

	// 입력 시스템 동작 확인용 표시.
	// 어떤 바인딩이 이번 프레임에 불렸는지 한 줄로 보여준다.
	char debugText[160] = {};
	sprintf_s(debugText, "DIR %2d,%2d  ATK %d  ROLL %d(%d)  LMB %d  BLOCK %d  ESC P%d R%d H%d",
		inputDirection.x,
		inputDirection.y,
		isAttack ? 1 : 0,
		isRolling ? 1 : 0,
		rollPressCount,
		mouseHeldFrameCount,
		isBlocking ? 1 : 0,
		escapePressedCount,
		escapeReleasedCount,
		escapeHeldCount);

	Renderer::Get().Submit(debugText, Vector2(0, 1), Color::White, INT_MAX);

	if (isMoving)
	{
		// 초당 moveSpeed칸 속도로 이동 (프레임레이트가 달라져도 속도는 일정)
		moveAmount += deltaTime * moveSpeed;

		while (moveAmount >= 1.0f)
		{
			moveAmount -= 1.0f;
			SetPosition(GetPosition() + inputDirection);
		}
	}
	else
	{
		// 키를 뗀 상태. 다음 입력에 바로 반응하도록 누적값을 비운다.
		moveAmount = 0.0f;
	}

	// 이번 프레임에 모인 입력은 여기까지만 유효하다.
	// 다음 프레임의 디스패치가 이 Tick 뒤에 오므로 지금 비워도 안전하다.
	inputDirection = Vector2::Zero;
	isAttack = false;
}
