#include "CCLExperimentDirector.h"

#include "Agents/CCLAgentWorldSubsystem.h"
#include "CCLWorldSimulationSubsystem.h"
#include "CCLWorldSnapshot.h"
#include "CCLTerrainRegion.h"
#include "CCLWorldGenerationStore.h"
#include "CCLTerrainReplication.h"
#include "CCLWorldEnvironmentConfig.h"
#include "AIController.h"
#include "BrainComponent.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/EngineVersion.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Net/UnrealNetwork.h"
#include "Serialization/JsonSerializer.h"
#include "TimerManager.h"

ACCLExperimentDirector::ACCLExperimentDirector()
{
	bReplicates = true;
	bAlwaysRelevant = true;
	SetNetUpdateFrequency(5);
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bTickEvenWhenPaused = true;
}

void ACCLExperimentDirector::BeginPlay()
{
	Super::BeginPlay();
	if (!HasAuthority() || UCCLWorldSimulationSubsystem::DomainForWorld(GetWorld()) == ECCLWorldDomain::Campaign)
	{
		SetActorTickEnabled(false);
		return;
	}

	Generation = FGuid::NewGuid();
	TSet<FName> Ids;
	TSet<int32> Zones;
	FString Error;
	for (const UCCLExperimentDefinition* Definition : Definitions)
	{
		if (!Definition || !Definition->Validate(Error) || Ids.Contains(Definition->CaseId) || Zones.Contains(Definition->Zone))
		{
			UE_LOG(LogTemp, Error, TEXT("CCL_EXPERIMENT invalid definition: %s"), *Error);
			return;
		}

		Ids.Add(Definition->CaseId);
		Zones.Add(Definition->Zone);
		FCCLExperimentResult Result;
		Result.CaseId = Definition->CaseId;
		Result.Generation = Generation;
		Result.Status = Definition->IsImplemented() ? ECCLExperimentStatus::Ready : ECCLExperimentStatus::NotImplemented;
		Result.Detail = Definition->Instructions.ToString();
		Results.Add(MoveTemp(Result));
	}

	if (Definitions.Num() != 12)
	{
		UE_LOG(LogTemp, Error, TEXT("CCL_EXPERIMENT requires all twelve zone definitions."));
		return;
	}

	auto* Agents = GetWorld()->GetSubsystem<UCCLAgentWorldSubsystem>();
	auto* Store = GetGameInstance()->GetSubsystem<UCCLAgentSessionStore>();
	auto& Baseline = Store->ExperimentInitialSnapshots.FindOrAdd(UCCLWorldSimulationSubsystem::DomainForWorld(GetWorld()));
	if (Baseline.IsEmpty() && (!Agents || !Agents->Save(Baseline)))
	{
		UE_LOG(LogTemp, Error, TEXT("CCL_EXPERIMENT could not capture initial clock/life state."));
	}

	InitialSnapshot = Baseline;
	if (!InitialSnapshot.IsEmpty())
	{
		InitializeTerrainExperiment();
	}
}

void ACCLExperimentDirector::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (HasAuthority())
	{
		InitializeWaterExperiment();
		TickTerrainExperiment();
		TickWaterExperiment();
		TickSnowExperiment();
	}
	if (!ActiveCase.IsNone() && FPlatformTime::Seconds() - RunStarted > ((ActiveCase == TEXT("Zone_05") || ActiveCase == TEXT("Zone_04") || ActiveCase == TEXT("Zone_03")) ? 120. : 10.))
	{
		if (auto* Result = Results.FindByPredicate([this](const auto& R) { return R.CaseId == ActiveCase; }))
		{
			Finish(*Result, false, TEXT("실제 경과 시간 제한을 초과했다."));
		}
	}
}

void ACCLExperimentDirector::EndPlay(EEndPlayReason::Type Reason)
{
	GetWorldTimerManager().ClearTimer(RunTimer);
	Super::EndPlay(Reason);
}

void ACCLExperimentDirector::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ACCLExperimentDirector, Results);
	DOREPLIFETIME(ACCLExperimentDirector, Generation);
	DOREPLIFETIME(ACCLExperimentDirector, Operator);
	DOREPLIFETIME(ACCLExperimentDirector, ActiveCase);
	DOREPLIFETIME(ACCLExperimentDirector, TerrainRegion);
}

void ACCLExperimentDirector::AssignOperator(APlayerController* Controller)
{
	if (HasAuthority() && Controller && (!IsValid(Operator) || Operator->IsInactive()))
	{
		Operator = Controller->PlayerState;
		ForceNetUpdate();
	}
}

void ACCLExperimentDirector::ReleaseOperator(APlayerController* Controller)
{
	if (HasAuthority() && Controller && Controller->PlayerState == Operator)
	{
		Operator = nullptr;
		for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
		{
			if (It->Get() != Controller)
			{
				AssignOperator(It->Get());
			}
		}

		ForceNetUpdate();
	}
}

bool ACCLExperimentDirector::Execute(APlayerController* Requester, ECCLExperimentAction Action, FName CaseId,
	FGuid ExpectedGeneration, FGuid ExpectedRun, FString& Message, uint64 ExpectedTerrainSerial)
{
	Message.Reset();
	if (!HasAuthority() || !IsReady() || ExpectedGeneration != Generation ||
		UCCLWorldSimulationSubsystem::DomainForWorld(GetWorld()) == ECCLWorldDomain::Campaign)
	{
		Message = TEXT("실험이 준비되지 않았거나 이전 실행 세대의 요청이다.");
		return false;
	}

	if (bTerrainRestorePending || bTravelPending)
	{
		Message = TEXT("저장된 지형과 세계 상태를 함께 복원하고 있다.");
		return false;
	}

	if (Action == ECCLExperimentAction::Teleport)
	{
		const auto* Definition = FindDefinition(CaseId);
		return Requester && Definition && MoveToSafety(Requester, Definition->Zone, Message);
	}

	if (!CanOperate(Requester))
	{
		Message = TEXT("조작 담당자만 실행할 수 있다. 다른 참가자는 결과를 확인할 수 있다.");
		return false;
	}

	if (Action >= ECCLExperimentAction::WaterRain && Action <= ECCLExperimentAction::WaterDry)
	{
		if (!ActiveCase.IsNone())
		{
			Message = TEXT("실행 중인 시험을 마치거나 중지한 뒤 물 입력을 바꿀 수 있다.");
			return false;
		}
		return WaterAction(Action, CaseId, Message);
	}

	if (Action >= ECCLExperimentAction::TerrainExcavate && Action <= ECCLExperimentAction::TerrainReset)
	{
		if (!TerrainRegion || ExpectedTerrainSerial != TerrainRegion->GetPublicationSerial())
		{
			Message = TEXT("지형 상태가 바뀌었다. 최신 상태 수신 후 다시 조작해 줘.");
			return false;
		}

		return ActiveCase.IsNone() && TerrainAction(Requester, Action, Message);
	}

	if (Action == ECCLExperimentAction::Reset)
	{
		return ResetExperiment(Message);
	}

	if (Action == ECCLExperimentAction::Stop)
	{
		auto* Result = Results.FindByPredicate([CaseId](const auto& R) { return R.CaseId == CaseId; });
		if (!Result || Result->RunId != ExpectedRun || Result->Status != ECCLExperimentStatus::Running)
		{
			Message = TEXT("중지할 실행 ID가 일치하지 않는다.");
			return false;
		}

		if (CaseId == TEXT("Zone_05") && TerrainRegion)
		{
			TerrainRegion->CancelPendingEdit();
			TerrainCaseStep = 0;
		}

		Finish(*Result, false, TEXT("조작 담당자가 실행을 중지했다."));
		return true;
	}

	if (!ActiveCase.IsNone())
	{
		Message = TEXT("실행 중인 시험을 마치거나 중지한 뒤 조작할 수 있다.");
		return false;
	}

	auto* Runtime = GetWorld()->GetSubsystem<UCCLWorldSimulationSubsystem>();
	switch (Action)
	{
	case ECCLExperimentAction::SnowFall:
	case ECCLExperimentAction::SnowMelt:
		return SnowAction(Action, CaseId, Message);
	case ECCLExperimentAction::Start:
		SnowWalker = Requester->GetPawn();
		return StartCase(CaseId, Message);
	case ECCLExperimentAction::Save:
		return SaveCheckpoint(Message);
	case ECCLExperimentAction::Load:
		return LoadCheckpoint(Message);
	case ECCLExperimentAction::NextLatitude:
	case ECCLExperimentAction::NextObliquity:
	case ECCLExperimentAction::RotateQuarter:
	case ECCLExperimentAction::OrbitQuarter:
	case ECCLExperimentAction::CycleOpening:
		return ChangeEnvironment(Action, Message);
	case ECCLExperimentAction::ScaleZero:
	case ECCLExperimentAction::ScaleOne:
	case ECCLExperimentAction::ScaleSixty:
		if (UCCLWorldSimulationSubsystem::DomainForWorld(GetWorld()) != ECCLWorldDomain::Scenario)
		{
			Message = TEXT("전역 시간 배율 변경은 격리 실험 맵에서만 할 수 있다.");
			return false;
		}

		return Runtime && Runtime->ChangeTimeScale(Action == ECCLExperimentAction::ScaleZero ? 0 :
			Action == ECCLExperimentAction::ScaleOne ? 1 : 60, Message);
	default:
		break;
	}

	FString Destination;
	switch (Action)
	{
	case ECCLExperimentAction::TravelHub: Destination = TEXT("/Game/Maps/EnvironmentPlayground"); break;
	case ECCLExperimentAction::TravelScenario: Destination = TEXT("/Game/Maps/EnvironmentScenario"); break;
	case ECCLExperimentAction::TravelCombat: Destination = TEXT("/Game/Maps/CombatPlayground"); break;
	case ECCLExperimentAction::TravelMultiplayer: Destination = TEXT("/Game/Maps/MultiplayerPlayground"); break;
	default: Message = TEXT("지원하지 않는 실험 조작이다."); return false;
	}

	auto* Agents = GetWorld()->GetSubsystem<UCCLAgentWorldSubsystem>();
	if (!Agents || !Agents->SaveSessionForTravel(Message))
	{
		return false;
	}

	bTravelPending = 1;
	if (GetNetMode() == NM_Standalone)
	{
		UGameplayStatics::OpenLevel(this, FName(*Destination));
		return true;
	}

	if (GetWorld()->ServerTravel(Destination))
	{
		return true;
	}

	bTravelPending = 0;
	Agents->CancelPreparedTravel();
	Message = TEXT("맵 전환을 시작할 수 없다.");
	return false;
}

bool ACCLExperimentDirector::ResetExperiment(FString& Error)
{
	if (!HasAuthority() || !IsReady())
	{
		Error = TEXT("초기화할 실험 상태가 없다.");
		return false;
	}

	FCCLWorldSnapshot Initial;
	auto* Runtime = GetWorld()->GetSubsystem<UCCLWorldSimulationSubsystem>();
	auto* Agents = GetWorld()->GetSubsystem<UCCLAgentWorldSubsystem>();
	if (!Runtime || !Agents || !FCCLWorldSnapshotCodec::Decode(InitialSnapshot, Initial, Error) ||
		Runtime->GetIdentity().Generation == MAX_uint64)
	{
		return false;
	}

	FCCLSimulationSnapshot Current;
	if (!Agents->GetSimulation().Capture(Current))
	{
		Error = TEXT("Agent 쓰기가 진행 중이어서 초기화를 재시도해야 한다.");
		return false;
	}

	Initial.Identity.Generation = Runtime->GetIdentity().Generation + 1;
	TArray<uint8> ResetBytes;
	if (!FCCLWorldSnapshotCodec::Encode(Initial, ResetBytes, Error) || !MoveToSafety(nullptr, 0, Error))
	{
		return false;
	}

	if (TerrainRegion && TerrainRegion->IsTerrainReady())
	{
		FCCLTerrainStore Base;
		auto Definition = TerrainRegion->GetTerrainStore().GetSnapshot().Definition;
		Definition.WorldId = Initial.Identity.WorldId;
		const auto Context = FCCLWorldGenerationStore::ContextFor(Initial);
		TArray<uint8> TerrainBytes;
		return Base.Initialize(Definition, Error) && Base.Capture(Context, TerrainBytes, Error)
			&& QueueWorldRestore(TerrainBytes, Context, ResetBytes, Error);
	}

	for (TActorIterator<AAIController> It(GetWorld()); It; ++It)
	{
		It->StopMovement();
		if (auto* Brain = It->GetBrainComponent())
		{
			Brain->StopLogic(TEXT("Experiment reset"));
		}
	}

	GetWorldTimerManager().ClearTimer(RunTimer);
	Generation = FGuid::NewGuid();
	ActiveCase = NAME_None;
	WaterCaseStep = 0;
	if (!Agents->Restore(ResetBytes, Error))
	{
		return false;
	}

	auto* SessionStore = GetGameInstance()->GetSubsystem<UCCLAgentSessionStore>();
	SessionStore->TravelSnapshots.Remove(Initial.Identity.Domain);
	SessionStore->SnapshotFor(Initial.Identity.Domain) = ResetBytes;
	for (auto& Result : Results)
	{
		Result.RunId.Invalidate();
		Result.Generation = Generation;
		const auto* Definition = FindDefinition(Result.CaseId);
		Result.Status = Definition && Definition->IsImplemented() ? ECCLExperimentStatus::Ready : ECCLExperimentStatus::NotImplemented;
		Result.Detail = Definition ? Definition->Instructions.ToString() : FString();
		Result.CompletedWorldSeconds = Runtime->GetClock().GetWorldSeconds();
	}

	ForceNetUpdate();
	return true;
}

bool ACCLExperimentDirector::CanOperate(const APlayerController* Requester) const
{
	if (Requester && Requester->PlayerState && Requester->PlayerState == Operator)
	{
		return true;
	}

#if !UE_BUILD_SHIPPING && !UE_BUILD_TEST
	return !Requester && HasAuthority() && FParse::Param(FCommandLine::Get(), TEXT("CCLExperimentSmoke"));
#else
	return false;
#endif
}

bool ACCLExperimentDirector::CanStart(FName CaseId, FString& Error) const
{
	const auto* Definition = FindDefinition(CaseId);
	if (!Definition || !Definition->IsImplemented())
	{
		Error = TEXT("아직 구현되지 않은 시험이다.");
		return false;
	}

	if (!ActiveCase.IsNone())
	{
		Error = TEXT("다른 시험이 실행 중이다.");
		return false;
	}

	if (Definition->Kind == ECCLExperimentKind::Clock &&
		UCCLWorldSimulationSubsystem::DomainForWorld(GetWorld()) != ECCLWorldDomain::Scenario)
	{
		Error = TEXT("공통 시간 시험은 격리 실험 맵에서 실행한다.");
		return false;
	}

	return true;
}

const UCCLExperimentDefinition* ACCLExperimentDirector::FindDefinition(FName CaseId) const
{
	for (const UCCLExperimentDefinition* Definition : Definitions)
	{
		if (Definition && Definition->CaseId == CaseId)
		{
			return Definition;
		}
	}

	return nullptr;
}

const FCCLExperimentResult* ACCLExperimentDirector::FindResult(FName CaseId) const
{
	return Results.FindByPredicate([CaseId](const auto& R) { return R.CaseId == CaseId; });
}

ACCLExperimentDirector* ACCLExperimentDirector::Find(const UWorld* World)
{
	for (TActorIterator<ACCLExperimentDirector> It(World); It; ++It)
	{
		return *It;
	}

	return nullptr;
}

bool ACCLExperimentDirector::StartCase(FName CaseId, FString& Error)
{
	if (!CanStart(CaseId, Error))
	{
		return false;
	}

	auto* Result = Results.FindByPredicate([CaseId](const auto& R) { return R.CaseId == CaseId; });
	if (!Result)
	{
		return false;
	}

	Result->RunId = FGuid::NewGuid();
	Result->Generation = Generation;
	Result->Status = ECCLExperimentStatus::Running;
	Result->Detail = TEXT("시험을 실행하고 있다.");
	ActiveCase = CaseId;
	RunStarted = FPlatformTime::Seconds();
	if (CaseId == TEXT("Zone_03"))
	{
		if (!StartSnowExperiment(Error))
		{
			Finish(*Result, false, Error);
			return false;
		}
		ForceNetUpdate();
		return true;
	}
	if (CaseId == TEXT("Zone_04"))
	{
		if (!StartWaterExperiment(Error))
		{
			Finish(*Result, false, Error);
			return false;
		}
		ForceNetUpdate();
		return true;
	}
	if (CaseId == TEXT("Zone_05"))
	{
		if (!ResetTerrain(Error))
		{
			Finish(*Result, false, Error);
			return false;
		}

		TerrainCaseStep = 1;
		ForceNetUpdate();
		return true;
	}

	GetWorldTimerManager().SetTimer(RunTimer, [this, CaseId, Run = Result->RunId, Token = Generation]()
	{
		CompleteCase(CaseId, Run, Token);
	}, 0.35f, false);
	ForceNetUpdate();
	return true;
}

void ACCLExperimentDirector::CompleteCase(FName CaseId, FGuid RunId, FGuid Token)
{
	auto* Result = Results.FindByPredicate([CaseId](const auto& R) { return R.CaseId == CaseId; });
	const auto* Definition = FindDefinition(CaseId);
	if (!Result || !Definition || Token != Generation || Result->RunId != RunId || ActiveCase != CaseId)
	{
		return;
	}

	FString Error;
	bool bPassed = false;
	switch (Definition->Kind)
	{
	case ECCLExperimentKind::Guide:
		bPassed = Definitions.Num() == 12 && GetWorld()->GetSubsystem<UCCLWorldSimulationSubsystem>()->IsRunning();
		break;
	case ECCLExperimentKind::Clock: bPassed = RunClock(Error); break;
	case ECCLExperimentKind::Snapshot: bPassed = RunSnapshot(RunId, Error); break;
	case ECCLExperimentKind::Celestial:
		bPassed = RunCelestials(Error) && (UCCLWorldSimulationSubsystem::DomainForWorld(GetWorld()) != ECCLWorldDomain::Scenario || RunClock(Error));
		break;
	case ECCLExperimentKind::Shelter: bPassed = RunShelter(Error); break;
	default: Error = TEXT("미구현 시험은 실행할 수 없다."); break;
	}

	Finish(*Result, bPassed, bPassed ? TEXT("예상 결과를 확인했다. 같은 조건으로 재실행할 수 있다.") : Error);
}

void ACCLExperimentDirector::Finish(FCCLExperimentResult& Result, bool bPassed, const FString& Detail)
{
	GetWorldTimerManager().ClearTimer(RunTimer);
	if (WaterCaseStep)
	{
		FString Ignore;
		GetWorld()->GetSubsystem<UCCLWorldSimulationSubsystem>()->ChangeSurfaceForcing(WaterRegionId(), WaterCaseForcing, Ignore);
		WaterCaseStep = 0;
	}
	ActiveCase = NAME_None;
	Result.Status = bPassed ? ECCLExperimentStatus::Passed : ECCLExperimentStatus::Failed;
	Result.Detail = Detail;
	Result.CompletedWorldSeconds = GetWorld()->GetSubsystem<UCCLWorldSimulationSubsystem>()->GetClock().GetWorldSeconds();
	auto Json = MakeShared<FJsonObject>();
	Json->SetStringField(TEXT("case"), Result.CaseId.ToString());
	Json->SetStringField(TEXT("run"), Result.RunId.ToString());
	Json->SetStringField(TEXT("generation"), Generation.ToString());
	Json->SetBoolField(TEXT("passed"), bPassed);
	Json->SetStringField(TEXT("detail"), Detail);
	Json->SetNumberField(TEXT("world_seconds"), Result.CompletedWorldSeconds);
	const auto* Runtime = GetWorld()->GetSubsystem<UCCLWorldSimulationSubsystem>();
	const auto* Definition = FindDefinition(Result.CaseId);
	Json->SetStringField(TEXT("engine"), FEngineVersion::Current().ToString());
	Json->SetStringField(TEXT("map"), GetWorld()->GetMapName());
	Json->SetStringField(TEXT("world_id"), Runtime->GetIdentity().WorldId.ToString());
	Json->SetStringField(TEXT("snapshot_generation"), LexToString(Runtime->GetIdentity().Generation));
	Json->SetNumberField(TEXT("net_mode"), GetNetMode());
	Json->SetNumberField(TEXT("seed"), Definition ? Definition->Seed : 0);
	Json->SetStringField(TEXT("instructions"), Definition ? Definition->Instructions.ToString() : FString());
	Json->SetStringField(TEXT("expected"), Definition ? Definition->Expected.ToString() : FString());
	Json->SetNumberField(TEXT("game_seconds"), Runtime->GetClock().GetGameSeconds());
	Json->SetNumberField(TEXT("time_scale"), Runtime->GetClock().GetTimeScale());
	Json->SetNumberField(TEXT("pending_game_seconds"), Runtime->GetClock().GetPendingGameSeconds());
	FString Text;
	FJsonSerializer::Serialize(Json, TJsonWriterFactory<>::Create(&Text));
	const FString Directory = FPaths::ProjectSavedDir() / TEXT("EnvironmentExperiments/Results");
	IFileManager::Get().MakeDirectory(*Directory, true);
	const FString Path = Directory / (Result.RunId.ToString() + TEXT(".json"));
	if (!FFileHelper::SaveStringToFile(Text, *Path))
	{
		Result.Status = ECCLExperimentStatus::Failed;
		Result.Detail += TEXT(" 결과 파일 기록에 실패했다.");
	}

	ForceNetUpdate();
	UE_LOG(LogTemp, Display, TEXT("CCL_EXPERIMENT %s Case=%s Run=%s Generation=%s Time=%.9f Detail=%s ResultFile=%s"),
		Result.Status == ECCLExperimentStatus::Passed ? TEXT("PASS") : TEXT("FAIL"), *Result.CaseId.ToString(), *Result.RunId.ToString(),
		*Generation.ToString(), Result.CompletedWorldSeconds, *Result.Detail, *Path);
}

bool ACCLExperimentDirector::RunClock(FString& Error)
{
	auto* Runtime = GetWorld()->GetSubsystem<UCCLWorldSimulationSubsystem>();
	auto* Agents = GetWorld()->GetSubsystem<UCCLAgentWorldSubsystem>();
	for (int32 Index = 0; Index < 256 && Runtime->GetClock().HasPendingTime(); ++Index)
	{
		if (!Runtime->AdvancePending(200, 20000, Error))
		{
			return false;
		}
	}

	if (Runtime->GetClock().HasPendingTime())
	{
		Error = TEXT("기존 대기 시간을 처리한 뒤 다시 실행해야 한다.");
		return false;
	}

	const double Before = Runtime->GetClock().GetWorldSeconds();
	if (!Runtime->ChangeTimeScale(60, Error) || !Runtime->QueueGameTime(120, Error))
	{
		return false;
	}

	FCCLWorldClockSnapshot Queued;
	Runtime->GetClock().Capture(Queued, Error);
	if (Runtime->AdvancePending(200, 20000, Error, 1) || Runtime->GetClock().GetWorldSeconds() != Before ||
		Agents->GetSimulation().GetTime() != Before)
	{
		Error = TEXT("의도한 처리 예산 실패가 시계·Agent 상태를 보존하지 못했다.");
		return false;
	}

	for (int32 Index = 0; Index < 256 && Runtime->GetClock().HasPendingTime(); ++Index)
	{
		if (!Runtime->AdvancePending(200, 20000, Error))
		{
			return false;
		}
	}

	if (Runtime->GetClock().HasPendingTime() || Runtime->GetClock().GetWorldSeconds() != Queued.RequestedWorldSeconds ||
		Runtime->GetClock().GetWorldSeconds() != Agents->GetSimulation().GetTime())
	{
		Error = TEXT("재시도 후 요청 시각과 실제 완료 시각이 다르다.");
		return false;
	}

	return true;
}

bool ACCLExperimentDirector::RunSnapshot(FGuid RunId, FString& Error)
{
	auto* Runtime = GetWorld()->GetSubsystem<UCCLWorldSimulationSubsystem>();
	auto* Agents = GetWorld()->GetSubsystem<UCCLAgentWorldSubsystem>();
	TArray<uint8> Bytes;
	if (!Agents->Save(Bytes))
	{
		Error = TEXT("실험 상태를 저장하지 못했다.");
		return false;
	}

	const double Before = Runtime->GetClock().GetWorldSeconds();
	const FGuid WorldId = Runtime->GetIdentity().WorldId;
	TArray<uint8> Broken = Bytes;
	Broken[40] ^= 1;
	if (Agents->Restore(Broken, Error) || Runtime->GetClock().GetWorldSeconds() != Before)
	{
		Error = TEXT("손상 저장 거부 후 상태가 바뀌었다.");
		return false;
	}

	const FString ProbeSlot = SlotName() + TEXT("_Probe_") + RunId.ToString();
	TArray<uint8> Read;
	const bool bSaved = UGameplayStatics::SaveDataToSlot(Bytes, ProbeSlot, 0);
	const bool bLoaded = bSaved && UGameplayStatics::LoadDataFromSlot(Read, ProbeSlot, 0);
	const bool bRemoved = !bSaved || UGameplayStatics::DeleteGameInSlot(ProbeSlot, 0);
	if (!bLoaded || !bRemoved || Read != Bytes || !Agents->Restore(Read, Error) ||
		Runtime->GetClock().GetWorldSeconds() != Before || Agents->GetSimulation().GetTime() != Before ||
		Runtime->GetIdentity().WorldId != WorldId)
	{
		Error = TEXT("파일 저장·재로딩·시계와 Agent 복원 검사가 실패했다.");
		return false;
	}

	return true;
}

bool ACCLExperimentDirector::SaveCheckpoint(FString& Error)
{
	if (TerrainRegion)
	{
		return SaveTerrainWorld(Error);
	}

	TArray<uint8> Bytes;
	if (!GetWorld()->GetSubsystem<UCCLAgentWorldSubsystem>()->Save(Bytes) || !UGameplayStatics::SaveDataToSlot(Bytes, SlotName(), 0))
	{
		Error = TEXT("실험 저장에 실패했다.");
		return false;
	}

	Error = TEXT("이 실험 맵의 상태를 저장했다.");
	return true;
}

bool ACCLExperimentDirector::LoadCheckpoint(FString& Error)
{
	if (TerrainRegion)
	{
		return LoadTerrainWorld(Error);
	}

	TArray<uint8> Bytes;
	FCCLWorldSnapshot Snapshot;
	if (!UGameplayStatics::LoadDataFromSlot(Bytes, SlotName(), 0) || !FCCLWorldSnapshotCodec::Decode(Bytes, Snapshot, Error) ||
		Snapshot.Identity.Domain != UCCLWorldSimulationSubsystem::DomainForWorld(GetWorld()) || !MoveToSafety(nullptr, 0, Error) ||
		!GetWorld()->GetSubsystem<UCCLAgentWorldSubsystem>()->Restore(Bytes, Error))
	{
		Error = TEXT("저장된 실험 상태가 없거나 복원하지 못했다.");
		return false;
	}

	Generation = FGuid::NewGuid();
	for (auto& Result : Results)
	{
		Result.Generation = Generation;
		Result.RunId.Invalidate();
		Result.Status = FindDefinition(Result.CaseId)->IsImplemented() ? ECCLExperimentStatus::Ready : ECCLExperimentStatus::NotImplemented;
		Result.Detail = TEXT("저장 상태를 복원했다. 시험 결과는 다시 실행해서 확인한다.");
	}

	ForceNetUpdate();
	Error = TEXT("이 실험 맵의 저장 상태를 복원했다.");
	return true;
}

bool ACCLExperimentDirector::MoveToSafety(APlayerController* OnlyPlayer, int32 Zone, FString& Error)
{
	int32 Index = 0;
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		auto* PC = It->Get();
		if (!PC || (OnlyPlayer && PC != OnlyPlayer))
		{
			continue;
		}

		if (APawn* Pawn = PC->GetPawn())
		{
			if (auto* Character = Cast<ACharacter>(Pawn))
			{
				Character->GetCharacterMovement()->StopMovementImmediately();
			}

			if (Zone == 5 && TerrainRegion)
			{
				const auto* Replication = PC->FindComponentByClass<UCCLTerrainReplication>();
				if (!TerrainRegion->IsTerrainReady() || TerrainRegion->IsPreparing() || !Replication || !Replication->IsClientReady(TerrainRegion))
				{
					Error = TEXT("참가자의 지형 충돌 준비가 끝나면 이동할 수 있다.");
					return false;
				}
			}

			const FVector Destination = UCCLExperimentDefinition::ZoneCenter(Zone) + FVector(-550, -450 + 180 * Index++, Zone == 5 ? 530 : 130);
			if (!Pawn->TeleportTo(Destination, FRotator::ZeroRotator))
			{
				Error = TEXT("안전 지점으로 이동할 수 없어 초기화를 중단했다.");
				return false;
			}
		}
	}

	return true;
}

FString ACCLExperimentDirector::SlotName() const
{
	const bool bScenario = UCCLWorldSimulationSubsystem::DomainForWorld(GetWorld()) == ECCLWorldDomain::Scenario;
	const bool bTest = FParse::Param(FCommandLine::Get(), TEXT("CCLExperimentSmoke"))
		|| FParse::Param(FCommandLine::Get(), TEXT("CCLTerrainScenarioSmoke"))
		|| FParse::Param(FCommandLine::Get(), TEXT("CCLWaterSmoke"))
		|| FParse::Param(FCommandLine::Get(), TEXT("CCLSnowSmoke"));
	FString Slot = FString(bTest ? TEXT("CCL_Experiment_Validation_") : TEXT("CCL_Experiment_")) +
		(bScenario ? TEXT("Scenario") : TEXT("Playground"));
#if !UE_BUILD_SHIPPING && !UE_BUILD_TEST
	FString Run;
	FGuid RunId;
	if (bTest && FParse::Value(FCommandLine::Get(), TEXT("CCLExperimentRun="), Run) && FGuid::Parse(Run, RunId))
	{
		Slot += TEXT("_") + RunId.ToString(EGuidFormats::Digits);
	}
#endif
	return Slot;
}

bool ACCLExperimentDirector::RunCelestials(FString& Error)
{
	const auto* Runtime = GetWorld()->GetSubsystem<UCCLWorldSimulationSubsystem>();
	FCCLCelestialObservation A;
	FCCLCelestialObservation B;
	FCCLCelestialSystem Rebuilt;
	const auto& Inputs = Runtime->GetEnvironmentInputs();
	if (!Runtime->ObserveCelestials(A, Error) || !Rebuilt.Initialize(Inputs.Celestial, Error)
		|| !Rebuilt.Observe(Runtime->GetClock().GetWorldSeconds(), Inputs.Observer, B, Error))
	{
		return false;
	}

	if (A.Stars.IsEmpty() || A.Stars.Num() != B.Stars.Num() || A.ObserverPositionKm != B.ObserverPositionKm
		|| A.TotalHorizontalIrradianceWattsPerM2 != B.TotalHorizontalIrradianceWattsPerM2)
	{
		Error = TEXT("천체 관측 재현 결과가 다르다.");
		return false;
	}

	for (int32 Index = 0; Index < A.Stars.Num(); ++Index)
	{
		if (A.Stars[Index].SolarHours != B.Stars[Index].SolarHours || A.Stars[Index].LocalDirection != B.Stars[Index].LocalDirection)
		{
			Error = TEXT("현지 시각 또는 항성 방향이 일치하지 않는다.");
			return false;
		}
	}

	return true;
}

bool ACCLExperimentDirector::RunShelter(FString& Error)
{
	const auto* Runtime = GetWorld()->GetSubsystem<UCCLWorldSimulationSubsystem>();
	const auto* Config = ACCLWorldEnvironmentConfig::Find(GetWorld());
	if (!Config)
	{
		Error = TEXT("차폐 시험의 맵 설정이 없다.");
		return false;
	}

	const auto* Inside = Config->Probes.FindByPredicate([](const auto& Probe) { return Probe.ProbeId == TEXT("Inside"); });
	const auto* Outside = Config->Probes.FindByPredicate([](const auto& Probe) { return Probe.ProbeId == TEXT("Outside"); });
	auto Inputs = Runtime->GetEnvironmentInputs();
	if (!Inside || !Outside || Inputs.Openings.IsEmpty())
	{
		Error = TEXT("실내외 진단 위치 또는 문 입력이 없다.");
		return false;
	}

	FCCLSurfaceScene Scene;
	FCCLShelterQuery Query;
	Query.BodyId = Inputs.Observer.BodyId;
	Query.PositionMeters = Inside->PositionMeters;
	Query.ToWindSource = Inside->ToWindSource;
	FCCLShelterSample Sample;
	Inputs.Openings[0].OpenFraction = 0.;
	if (!Scene.Replace(Inputs.Surfaces, Inputs.Openings, 1, Error)
		|| !FCCLShelterEvaluator::Evaluate(Scene, Query, Sample, Error)
		|| Sample.Transmission.Precipitation != 0. || Sample.Transmission.Wind != 0.)
	{
		Error = TEXT("닫힌 문과 지붕의 차폐 결과가 다르다.");
		return false;
	}

	Inputs.Openings[0].OpenFraction = 1.;
	if (!Scene.Replace(Inputs.Surfaces, Inputs.Openings, 2, Error)
		|| !FCCLShelterEvaluator::Evaluate(Scene, Query, Sample, Error)
		|| Sample.Transmission.Wind != 1. || Sample.Transmission.Precipitation != 0.)
	{
		Error = TEXT("문을 열어도 지붕의 강수 차폐는 유지되어야 한다.");
		return false;
	}

	Query.PositionMeters = Outside->PositionMeters;
	if (!FCCLShelterEvaluator::Evaluate(Scene, Query, Sample, Error) || Sample.Transmission.Precipitation != 1.)
	{
		Error = TEXT("실외 진단 위치의 강수 노출이 다르다.");
		return false;
	}

	FCCLSurfaceQuery Ray;
	Ray.BodyId = Inputs.Observer.BodyId;
	Ray.OriginMeters = Inside->PositionMeters;
	TArray<FCCLSurfaceSample> Above;
	TArray<FCCLSurfaceSample> Below;
	if (!Scene.QuerySurfaces(Ray, Above, Error))
	{
		return false;
	}

	Ray.Direction = -FVector::UpVector;
	if (!Scene.QuerySurfaces(Ray, Below, Error) || Above.IsEmpty() || Below.IsEmpty()
		|| Above[0].Normal.Z > -0.9 || Below[0].Normal.Z < 0.9 || Above[0].SurfaceId == Below[0].SurfaceId)
	{
		Error = TEXT("천장 아랫면과 바닥을 별도 표면으로 찾지 못했다.");
		return false;
	}

	return true;
}

bool ACCLExperimentDirector::ChangeEnvironment(ECCLExperimentAction Action, FString& Error)
{
	if (Action != ECCLExperimentAction::CycleOpening
		&& UCCLWorldSimulationSubsystem::DomainForWorld(GetWorld()) != ECCLWorldDomain::Scenario)
	{
		Error = TEXT("천체 입력 변경은 격리 실험 맵에서만 할 수 있다.");
		return false;
	}

	auto* Runtime = GetWorld()->GetSubsystem<UCCLWorldSimulationSubsystem>();
	auto Inputs = Runtime->GetEnvironmentInputs();
	if (Inputs.Revision == MAX_uint64 || Inputs.SurfaceRevision == MAX_uint64)
	{
		Error = TEXT("환경 입력 리비전을 더 증가시킬 수 없다.");
		return false;
	}

	++Inputs.Revision;
	++Inputs.SurfaceRevision;
	auto* Body = Inputs.Celestial.Bodies.FindByPredicate([&Inputs](const auto& Value) { return Value.BodyId == Inputs.Observer.BodyId; });
	if (!Body)
	{
		return false;
	}

	switch (Action)
	{
	case ECCLExperimentAction::NextLatitude:
		Inputs.Observer.LatitudeDegrees = Inputs.Observer.LatitudeDegrees >= 89. ? -90. : FMath::Min(90., Inputs.Observer.LatitudeDegrees + 45.);
		break;
	case ECCLExperimentAction::NextObliquity:
		Body->ObliquityDegrees = Body->ObliquityDegrees >= 44. ? 0. : Body->ObliquityDegrees < 1. ? 23.44 : 45.;
		break;
	case ECCLExperimentAction::RotateQuarter:
		Body->SpinPhaseDegrees = FMath::Fmod(Body->SpinPhaseDegrees + 90., 360.);
		break;
	case ECCLExperimentAction::OrbitQuarter:
		Body->MeanAnomalyDegrees = FMath::Fmod(Body->MeanAnomalyDegrees + 90., 360.);
		break;
	case ECCLExperimentAction::CycleOpening:
		if (Inputs.Openings.IsEmpty())
		{
			Error = TEXT("조작할 개구부가 없다.");
			return false;
		}

		Inputs.Openings[0].OpenFraction = Inputs.Openings[0].OpenFraction >= 1. ? 0. : Inputs.Openings[0].OpenFraction + 0.5;
		break;
	default:
		return false;
	}

	if (!Runtime->ReplaceEnvironmentInputs(Inputs, Error))
	{
		return false;
	}

	Error = TEXT("시험 입력을 변경했다. 위상 조작은 세계 시각을 건너뛰지 않으며 저장·초기화 대상이다.");
	return true;
}
