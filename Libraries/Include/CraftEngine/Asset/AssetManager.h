#pragma once

#include "Utils/EngineMacro.h"
#include "Core/CraftObject.h"
#include "Asset/PrimaryDataAsset.h"
#include <atomic>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <system_error>
#include <typeindex>
#include <unordered_map>
#include <vector>

NAME_SPACE_BEGIN(Craft)

// 잡 큐는 순전히 구현 세부라 헤더에 노출하지 않는다.
// ObjectPool<Job>이 dllexport 클래스 템플릿이라, 이 헤더를 쓰는 클라이언트 모듈에서
// JobQueue::DoAsync가 인스턴스화되면 정적 멤버를 다시 정의하려 들어 링크가 깨진다.
// 큐를 만지는 코드는 전부 AssetManager.cpp 안에만 둔다.
class JobQueue;

// 타입 T를 파일 경로로부터 만들어내는 방법.
// AssetManager::RegisterLoader<T>()로 등록해두면 Load<T>()가 캐시 미스일 때 이걸로 채운다.
template<typename T>
using AssetLoaderFunc = std::function<std::shared_ptr<const T>(const WCHAR* path)>;

// 비동기 로드가 끝났을 때 메인 쓰레드에서 불리는 완료 콜백.
// 로드에 실패하면 asset이 nullptr로 들어온다(파일이 없거나 파싱 실패).
template<typename T>
using AssetLoadedDelegate = std::function<void(std::shared_ptr<const T> asset)>;

// 파일 경로로 애셋을 로드/캐싱하고, 오래 참조되지 않은 애셋은 자동으로 내리는 매니저.
// 언리얼의 Primary Asset / Asset Manager를 참고한 구조.
//
// - Load<T>(path) : 캐시에 있으면 즉시 반환, 없으면 등록된 로더로 로드해서 캐시에 넣고 반환.
// - 언로드 조건   : 매니저 말고 아무도 안 들고 있는 상태(shared_ptr 참조 1)가 unloadThreshold초
//                   이상 지속되면 내린다. Tick()에서 매 프레임 검사한다.
//
// 주의 - Load<T>()가 돌려준 shared_ptr을 호출부가 계속 들고 있어야 참조로 잡힌다.
// 반환값을 즉시 버리면(예: 내용물만 꺼내 쓰고 컨테이너는 버리는 경우) 다음 Tick부터
// 바로 유휴 판정이 시작되므로, 실제로 계속 쓰는 애셋이라면 멤버 변수 등으로 붙들고 있을 것.
class CRAFT_API AssetManager
{
	// 타입을 지우기 위한 인터페이스. caches 맵에 타입별 캐시를 이걸로 담는다.
	struct TypedCacheBase
	{
		virtual ~TypedCacheBase() = default;
		virtual void Tick(float deltaTime, float unloadThreshold) = 0;
	};

	template<typename T>
	struct TypedCache : TypedCacheBase
	{
		struct Entry
		{
			std::shared_ptr<const T> asset;
			float idleTime = 0.0f;
		};

		std::unordered_map<std::wstring, Entry> entries;
		AssetLoaderFunc<T> loader;

		// 로드 진행 중인 경로 -> 완료를 기다리는 콜백들.
		// 같은 파일을 여럿이 동시에 요청해도 워커에는 한 번만 넘긴다.
		// 메인 쓰레드에서만 접근하므로 락이 필요 없다.
		std::unordered_map<std::wstring, std::vector<AssetLoadedDelegate<T>>> pending;

		virtual void Tick(float deltaTime, float unloadThreshold) override
		{
			// Level::ProcessAddAndDestoryActors()와 동일한 순회-중-삭제 패턴.
			for (auto it = entries.begin(); it != entries.end(); )
			{
				Entry& entry = it->second;

				// 매니저(entry.asset) 말고 다른 곳에서도 들고 있으면 사용 중 - 유휴 시간 리셋.
				if (entry.asset.use_count() > 1)
				{
					entry.idleTime = 0.0f;
				}
				else
				{
					entry.idleTime += deltaTime;
				}

				if (entry.idleTime >= unloadThreshold)
				{
					it = entries.erase(it);
				}
				else
				{
					++it;
				}
			}
		}
	};

public:
	AssetManager();
	~AssetManager();

	// cache가 unique_ptr의 map 이므로 복사 대입 처리 delete 안해주면 컴파일 에러 발생...
	AssetManager(const AssetManager&) = delete;
	AssetManager& operator=(const AssetManager&) = delete;

	// Input::Get()/Renderer::Get()과 동일한 자기-싱글턴 패턴.
	static AssetManager& Get();

	// 타입 T를 로드할 방법을 등록한다. 보통 엔진 초기화 시점에 한 번씩.
	template<typename T>
	void RegisterLoader(AssetLoaderFunc<T> loader)
	{
		GetOrCreateCache<T>().loader = loader;
	}

	// 캐시에 있으면 반환, 없으면 등록된 로더로 로드 후 캐시에 넣고 반환.
	// 등록된 로더가 없으면 크래시한다 - 로더 누락은 런타임 상황이 아니라 코드 실수이기 때문.
	template<typename T>
	std::shared_ptr<const T> Load(const WCHAR* path)
	{
		TypedCache<T>& cache = GetOrCreateCache<T>();

		const std::wstring key(path);
		auto it = cache.entries.find(key);

		if (it != cache.entries.end())
		{
			return it->second.asset;
		}

		ASSERT_CRASH(cache.loader);

		typename TypedCache<T>::Entry entry;
		entry.asset = cache.loader(path);

		cache.entries.emplace(key, entry);

		return entry.asset;
	}

	// 비동기 로드. 파일 읽기와 파싱은 워커 쓰레드가 하고,
	// onLoaded는 항상 메인 쓰레드의 Tick() 안에서 불린다.
	//
	// 캐시에 이미 있어도 즉시 호출하지 않고 완료 큐를 거친다.
	// "콜백은 언제나 Tick 시점"이라는 타이밍이 한결같아야
	// 호출부가 캐시 히트/미스를 나눠서 생각하지 않아도 된다.
	//
	// 주의 - onLoaded가 불릴 때 요청자가 이미 파괴됐을 수 있다.
	// 콜백에서 액터/컴포넌트를 만진다면 weak_ptr로 생존을 확인할 것.
	template<typename T>
	void LoadAsync(const WCHAR* path, AssetLoadedDelegate<T> onLoaded)
	{
		TypedCache<T>& cache = GetOrCreateCache<T>();

		const std::wstring key(path);

		// 1) 캐시 히트 - 파싱 없이 완료 큐로 바로 넘긴다.
		auto it = cache.entries.find(key);

		if (it != cache.entries.end())
		{
			std::shared_ptr<const T> asset = it->second.asset;

			EnqueueCompletionJob([onLoaded, asset]() { onLoaded(asset); });

			return;
		}

		// 2) 이미 같은 경로를 로드 중이면 콜백만 덧붙인다(중복 파싱 방지).
		auto pendingIt = cache.pending.find(key);

		if (pendingIt != cache.pending.end())
		{
			pendingIt->second.push_back(onLoaded);

			return;
		}

		// 3) 새 요청. 로더가 없으면 코드 실수다.
		ASSERT_CRASH(cache.loader);

		cache.pending[key].push_back(onLoaded);

		// 워커가 캐시를 만지지 않도록 로더를 값으로 복사해서 넘긴다.
		AssetLoaderFunc<T> loader = cache.loader;
		AssetManager* self = this;

		EnqueueLoadJob([self, loader, key]()
			{
				// ===== 여기부터 워커 쓰레드 =====
				// 엔진 자료구조는 하나도 건드리지 않는다. 파싱해서 결과만 만든다.
				std::shared_ptr<const T> asset = nullptr;

				// FileUtils::ReadFile 안의 fs::file_size가 예외를 던지는데
				// 엔진은 예외를 쓰지 않으므로 워커에서 던지면 그대로 terminate다.
				// 그래서 읽기 전에 존재를 확인한다(ec 버전이라 이 호출 자체는 안 던진다).
				std::error_code ec;

				if (std::filesystem::exists(key, ec) && !ec)
				{
					asset = loader(key.c_str());
				}
				// ===== 워커 쓰레드 끝 =====

				// 캐시 삽입과 콜백 호출은 메인에게 맡긴다.
				self->EnqueueCompletionJob([self, key, asset]() { self->OnAssetLoaded<T>(key, asset); });
			});
	}

	// 매 프레임 호출. 완료된 로드를 처리하고, 유휴 시간을 누적해 정리한다.
	void Tick(float deltaTime);

	// 워커 쓰레드를 멈춘다. Engine::Shutdown()에서 Join() 전에 반드시 부를 것.
	// 이걸 안 부르면 워커가 무한 루프라 Join()에서 영영 안 돌아온다.
	void StopWorkers();

	// 워커 쓰레드가 도는 루프. ThreadManager::Launch()에 넘긴다.
	void WorkerLoop();

	// 유휴 판정 임계값(초). 기본 30초.
	inline void SetUnloadThreshold(float seconds) { unloadThreshold = seconds; }
	inline float GetUnloadThreshold() const { return unloadThreshold; }

	// --- Primary Data Asset -----------------------------------------------
	// 매니페스트로 한꺼번에 로드되고, 이름으로 조회되고, 유휴 언로드되지 않는 애셋.
	// 자세한 설명은 PrimaryDataAsset.h 참고.

	// 파생 타입 T를 문자열 typeName으로 등록한다. 매니페스트의 type="..."이 이 이름과 매칭된다.
	// LoadPrimaryAssetManifest()보다 먼저 호출해야 한다.
	template<typename T>
	void RegisterPrimaryAssetType(const std::string& typeName)
	{
		primaryAssetFactories[typeName] = []() { return std::make_shared<T>(); };
	}

	// 매니페스트 XML을 읽어 <Asset type=".." name=".." path=".."> 항목을 전부 로드한다.
	// 관련 RegisterPrimaryAssetType() 호출들이 모두 끝난 뒤, 다른 애셋을 쓰기 전에 호출할 것.
	// Primary 애셋은 없으면 게임이 못 뜨는 필수 데이터라, 실패 시 크래시한다
	// (Palette/AnimationClip처럼 조용히 기본값으로 폴백하지 않음).
	// 로드된 개수를 반환한다.
	int LoadPrimaryAssetManifest(const WCHAR* manifestPath);

	// 이름으로 조회한다. 등록 안 된 이름이거나 타입이 안 맞으면 nullptr.
	template<typename T>
	std::shared_ptr<const T> GetPrimaryAsset(const std::string& name) const
	{
		auto it = primaryAssets.find(name);

		if (it == primaryAssets.end())
		{
			return nullptr;
		}

		// Cast<T>가 shared_ptr<T>를 돌려주고, 이게 shared_ptr<const T>로 암묵 변환된다.
		return Cast<T>(it->second);
	}

private:
	// 큐에 잡을 넣는 통로. 정의가 .cpp에 있어서 JobQueue 타입이 헤더로 새지 않는다.
	void EnqueueLoadJob(std::function<void()> job);
	void EnqueueCompletionJob(std::function<void()> job);

	// 워커가 만들어 온 결과를 캐시에 넣고 대기 중인 콜백들을 부른다.
	// 완료 큐를 통해서만 불리므로 항상 메인 쓰레드다.
	template<typename T>
	void OnAssetLoaded(const std::wstring& key, std::shared_ptr<const T> asset)
	{
		TypedCache<T>& cache = GetOrCreateCache<T>();

		// 성공한 경우에만 캐시에 넣는다.
		// 실패(nullptr)를 캐시에 넣으면 파일을 고쳐도 계속 실패가 반환된다.
		if (nullptr != asset)
		{
			typename TypedCache<T>::Entry entry;
			entry.asset = asset;

			cache.entries.emplace(key, entry);
		}

		auto pendingIt = cache.pending.find(key);

		if (pendingIt == cache.pending.end())
		{
			return;
		}

		// 콜백이 같은 경로를 다시 LoadAsync해도 안전하도록
		// 목록을 꺼내고 맵에서 먼저 지운 뒤에 호출한다.
		std::vector<AssetLoadedDelegate<T>> delegates = std::move(pendingIt->second);
		cache.pending.erase(pendingIt);

		for (AssetLoadedDelegate<T>& onLoaded : delegates)
		{
			onLoaded(asset);
		}
	}

	// 템플릿 멤버 함수라 정의를 여기(헤더)에 둬야 함 - cpp로 분리 불가.
	template<typename T>
	TypedCache<T>& GetOrCreateCache()
	{
		const std::type_index key = std::type_index(typeid(T));
		auto it = caches.find(key);

		if (it != caches.end())
		{
			return static_cast<TypedCache<T>&>(*it->second);
		}

		auto newCache = std::make_unique<TypedCache<T>>();
		TypedCache<T>& ref = *newCache;
		caches.emplace(key, std::move(newCache));

		return ref;
	}

private:
	std::unordered_map<std::type_index, std::unique_ptr<TypedCacheBase>> caches;

	float unloadThreshold = 30.0f;

	// 문자열 타입 이름 -> 빈 인스턴스를 만드는 팩토리.
	std::unordered_map<std::string, std::function<std::shared_ptr<PrimaryDataAsset>()>> primaryAssetFactories;

	// 이름 -> 로드 완료된 프라이머리 애셋.
	// caches(TypedCache)와 완전히 분리된 별도 저장소라서 Tick()의 유휴 정리 대상이 아니다.
	std::unordered_map<std::string, std::shared_ptr<PrimaryDataAsset>> primaryAssets;

	// 메인 -> 워커. 파일 읽기 + 파싱 잡이 쌓인다.
	std::shared_ptr<JobQueue> loadQueue;

	// 워커 -> 메인. 캐시 삽입 + 콜백 호출 잡이 쌓이고, Tick()에서 비운다.
	std::shared_ptr<JobQueue> completionQueue;

	// 워커 루프 종료 신호.
	std::atomic<bool> isRunning = true;

	static AssetManager* instance;
};

NAME_SPACE_END
