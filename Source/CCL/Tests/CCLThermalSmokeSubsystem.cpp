#include "CCLThermalSmokeSubsystem.h"
#include "Environment/CCLThermalExposure.h"
#include "Environment/CCLBiologyExperiment.h"
#include "Environment/CCLExperimentDirector.h"
#include "Environment/CCLWorldSimulationSubsystem.h"
#include "Agents/CCLAgentWorldSubsystem.h"
#include "Agents/CCLAgentComponent.h"
#include "Agents/CCLAgentTags.h"
#include "Campaign/CCLLifeVillager.h"
#include "Combat/CCLEnemyCharacter.h"
#include "AbilitySystem/CCLHealthSet.h"
#include "AbilitySystem/CCLAbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "Misc/FileHelper.h"

bool UCCLThermalSmokeSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
#if UE_BUILD_SHIPPING || UE_BUILD_TEST
	return false;
#else
	return FParse::Param(FCommandLine::Get(), TEXT("CCLThermalSmoke"));
#endif
}

void UCCLThermalSmokeSubsystem::Tick(float DeltaTime)
{
	const double Now = FPlatformTime::Seconds();
	if (!Started)
	{
		Started = Now;
	}
	if (Now - Started > 180.)
	{
		Finish(false, FString::Printf(TEXT("timeout step=%d"), Step));
		return;
	}
	auto* W = GetWorld();
	auto* R = W ? W->GetSubsystem<UCCLWorldSimulationSubsystem>() : nullptr;
	auto* D = W ? ACCLExperimentDirector::Find(W) : nullptr;
	auto* Lab = W ? ACCLBiologyExperiment::Find(W) : nullptr;
	if (!R || !D || !D->IsReady() || !Lab || Lab->GetView().Rows.Num() != 2 || Now < Next)
	{
		return;
	}
	const auto& View = Lab->GetView();
	FString Directory, Error;
	FParse::Value(FCommandLine::Get(), TEXT("CCLThermalSmokeDir="), Directory);
	auto Check = [&](bool OK, const TCHAR* Message)
	{
		if (!OK)
		{
			Finish(false, FString::Printf(TEXT("step=%d %s %s"), Step, Message, *Error));
		}
		return OK;
	};
	auto Oracle = [&]()
	{
		return FString::Printf(TEXT("%s,%.17g,%d,%.9g,%d"), *View.Epoch.ToString(), View.WorldSeconds,
							   View.Rows[1].DeathCause, View.Rows[1].Health, View.Rows[1].bProtected);
	};
	auto Write = [&](const TCHAR* Name, const FString& Text)
	{ return FFileHelper::SaveStringToFile(Text, *(Directory / Name)); };
	if (W->GetNetMode() == NM_Client)
	{
		const bool Late = FParse::Param(FCommandLine::Get(), TEXT("CCLThermalLate"));
		if (!Late && Step == 0 && Lab->FindActor(1) && View.Rows[1].Health == 100.f)
		{
			if (!Check(Write(TEXT("base-observed.txt"), TEXT("ready")), TEXT("baseline acknowledgement")))
			{
				return;
			}
			Step = 1;
		}
		FString Expected;
		if (!Late && Step == 1 && FFileHelper::LoadFileToString(Expected, *(Directory / TEXT("partial-view.txt"))) &&
			Expected == Oracle() && Lab->FindActor(1))
		{
			auto* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Lab->FindActor(1));
			if (!ASC || ASC->GetNumericAttribute(UCCLHealthSet::GetHealthAttribute()) != View.Rows[1].Health)
			{
				return;
			}
			if (!Check(Write(TEXT("partial-observed.txt"), TEXT("ready")), TEXT("injury acknowledgement")))
			{
				return;
			}
			Step = 2;
		}
		if ((!Late && Step != 2) || !FFileHelper::LoadFileToString(Expected, *(Directory / TEXT("final-view.txt"))) ||
			Expected != Oracle() || Lab->FindActor(1))
		{
			return;
		}
		FKSIWorldStep Fake;
		if (!Check(!W->GetSubsystem<UCCLThermalExposureSubsystem>()->Prepare(Fake, R->GetSurfaceSimulation(), Error),
				   TEXT("client cannot apply thermal step")))
		{
			return;
		}
		Finish(true, TEXT("replicated GAS injury and restored thermal death match server"));
		return;
	}
	auto* Agents = W->GetSubsystem<UCCLAgentWorldSubsystem>();
	auto* Thermal = W->GetSubsystem<UCCLThermalExposureSubsystem>();
	auto* PC = W->GetFirstPlayerController();
	if (!Agents || !Agents->IsRunning())
	{
		return;
	}
	auto& Life = Agents->GetSimulation();
	auto* TargetActor = Lab->FindActor(1);
	auto* ASC = TargetActor ? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor) : nullptr;
	auto HP = [&]() { return ASC ? ASC->GetNumericAttribute(UCCLHealthSet::GetHealthAttribute()) : -1.f; };
	auto Advance = [&]()
	{ return (R->GetClock().HasPendingTime() || R->QueueGameTime(.25, Error)) && R->AdvancePending(.25, 1., Error); };
	if (Step == 0)
	{
		R->SetTickableTickType(ETickableTickType::Never);
		if (!Check(R->ChangeTimeScale(0., Error), TEXT("pause world age")))
		{
			return;
		}
		Step = 1;
		return;
	}
	if (Step == 1)
	{
		if (R->GetClock().HasPendingTime())
		{
			if (!R->AdvancePending(.25, 1., Error))
			{
				Check(Error.Contains(TEXT("awaits")), TEXT("drain startup time"));
			}
			return;
		}
		if (!ASC || HP() != 100.f)
		{
			return;
		}
		UE_LOG(LogTemp, Display, TEXT("CCL_THERMAL BASE_READY"));
		Step = 2;
		return;
	}
	if (Step == 2)
	{
		if (!PC || !PC->GetPawn())
		{
			return;
		}

		if (W->GetNetMode() != NM_Standalone && !FPaths::FileExists(Directory / TEXT("base-observed.txt")))
		{
			return;
		}
		const auto Id = ACCLExperimentDirector::FireRegionId(false);
		const auto* Grid = R->GetSurfaceSimulation().FindRegion(Id);
		if (!Check(Grid && ASC, TEXT("hot region and protected resident")))
		{
			return;
		}
		HotPosition = FKSISurfaceThermalContext::CellPosition(*Grid, 5) * 100. + FVector(0., 0., 90.);
		SafePosition = TargetActor->GetActorLocation();
		TargetActor->SetActorLocation(HotPosition, false, nullptr, ETeleportType::TeleportPhysics);
		if (!Check(R->IgniteSurface(Id, 5, 600000., Error), TEXT("ignite actual fuel")))
		{
			return;
		}
		double Rate = 0.;
		if (!Check(CCLThermalExposure::Sample(R->GetSurfaceSimulation(), HotPosition / 100.,
											  TargetActor->FindComponentByClass<UCCLThermalReceiverComponent>()->Policy,
											  &R->GetSurfaceProvider(), R->GetEnvironmentInputs().Observer.BodyId, Rate,
											  Error) &&
					   Rate > 0.,
				   TEXT("actual spatial exposure")))
		{
			return;
		}
		auto* PlayerASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(PC->GetPawn());
		const FVector PlayerPosition = PC->GetPawn()->GetActorLocation();
		const float PlayerHealth = PlayerASC->GetNumericAttribute(UCCLHealthSet::GetHealthAttribute());
		PC->GetPawn()->SetActorLocation(HotPosition, false, nullptr, ETeleportType::TeleportPhysics);
		const FTransform EnemyTransform(FRotator::ZeroRotator, HotPosition);
		auto* Enemy =
			W->SpawnActorDeferred<ACCLEnemyCharacter>(ACCLEnemyCharacter::StaticClass(), EnemyTransform, nullptr,
													  nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!Check(Enemy != nullptr, TEXT("enemy receiver")))
		{
			return;
		}
		Enemy->AutoPossessAI = EAutoPossessAI::Disabled;
		Enemy->FinishSpawning(EnemyTransform);
		Enemy->GetCharacterMovement()->SetMovementMode(MOVE_None);
		auto* EnemyASC = Enemy->GetAbilitySystemComponent();
		const float EnemyHealth = EnemyASC->GetNumericAttribute(UCCLHealthSet::GetHealthAttribute());
		const double GameBefore = R->GetClock().GetGameSeconds();
		InitialTime = Life.GetTime();
		TArray<uint8> BeforeSurface, AfterSurface;
		R->GetSurfaceSimulation().Capture(BeforeSurface, Error);
		auto& Store = Life.GetAgents();
		const auto Lease = Store.Acquire(Store.GetHandle(Lab->AgentId(1)), FGuid::NewGuid());
		const bool Blocked = Lease.Writer.IsValid() && R->QueueGameTime(.25, Error) &&
							 !R->AdvancePending(.25, 1., Error) && Error.Contains(TEXT("thermal receiver"));
		Store.Release(Lease);
		if (!Check(Blocked && HP() == 100.f && R->GetClock().GetGameSeconds() == GameBefore,
				   TEXT("writer conflict preserves health and clock")))
		{
			return;
		}
		if (!Check(!R->AdvancePending(.25, 1., Error, 0) && HP() == 100.f &&
					   R->GetSurfaceSimulation().Capture(AfterSurface, Error) && BeforeSurface == AfterSurface &&
					   R->GetClock().GetGameSeconds() == GameBefore,
				   TEXT("life failure preserves exposure and surface")))
		{
			return;
		}
		Thermal->Commit();
		if (!Check(HP() == 100.f && Advance(), TEXT("cancelled candidate cannot apply, retry succeeds")))
		{
			return;
		}
		InjuredHealth = HP();
		if (!Check(FMath::IsNearlyEqual(PlayerASC->GetNumericAttribute(UCCLHealthSet::GetHealthAttribute()),
										PlayerHealth - float(Rate * .25), .0001f) &&
					   FMath::IsNearlyEqual(EnemyASC->GetNumericAttribute(UCCLHealthSet::GetHealthAttribute()),
											EnemyHealth - float(Rate * .25), .0001f),
				   TEXT("player and enemy receive the same spatial thermal dose")))
		{
			return;
		}
		PC->GetPawn()->SetActorLocation(PlayerPosition, false, nullptr, ETeleportType::TeleportPhysics);
		Enemy->Destroy();
		const float Injury = Life.Find(Lab->AgentId(1))
								 ->Features.FindChecked(CCLAgentTags::Feature_Needs)
								 .Data.Get<FCCLAgentNeeds>()
								 .Urgency.FindRef(CCLAgentTags::Injury);
		if (!Check(FMath::IsNearlyEqual(InjuredHealth, float(100. - Rate * .25), .0001f) &&
					   FMath::IsNearlyEqual(Injury, 1.f - InjuredHealth / 100.f, .00001f) &&
					   Life.GetTime() == InitialTime,
				   TEXT("game time thermal injury persists while world age is paused")))
		{
			return;
		}
		Thermal->Commit();
		if (!Check(HP() == InjuredHealth, TEXT("duplicate commit has no damage")))
		{
			return;
		}
		TargetActor->SetActorLocation(SafePosition, false, nullptr, ETeleportType::TeleportPhysics);
		Step = 3;
		Next = Now + .5;
		return;
	}
	if (Step == 3)
	{
		if (!Check(Advance() && HP() == InjuredHealth, TEXT("leaving hot cell stops damage")))
		{
			return;
		}
		FKSIAirSpace Room;
		Room.SpaceId = FGuid(7117, 7117, 7117, 1);
		Room.MinimumMeters = SafePosition / 100. - FVector(.5, .5, .8);
		Room.MaximumMeters = SafePosition / 100. + FVector(.5, .5, .8);
		Room.VolumeCubicMeters = 1.6;
		if (!Check(Room.Initialize(100., .004, Error) && R->AddAirSpace(Room, Error) && Advance()
					   && FMath::IsNearlyEqual(HP(), InjuredHealth - 1.f, .0001f),
				   TEXT("100 degree indoor air applies one HP in quarter game second")))
		{
			return;
		}
		InjuredHealth = HP();
		if (!Check(R->Save(Life, SavedBefore, Error), TEXT("save contact and hot air injury")))
		{
			return;
		}

		Step = 4;
		Next = Now + .5;
		return;
	}
	if (Step == 4)
	{
		if (View.Rows[1].Health != InjuredHealth)
		{
			return;
		}
		if (!Check(Write(TEXT("partial-view.txt"), Oracle()), TEXT("injured public oracle")))
		{
			return;
		}
		if (W->GetNetMode() != NM_Standalone && !FPaths::FileExists(Directory / TEXT("partial-observed.txt")))
		{
			return;
		}
		TargetActor->SetActorLocation(HotPosition, false, nullptr, ETeleportType::TeleportPhysics);
		Step = 5;
		return;
	}
	if (Step == 5)
	{
		if (Life.IsAlive(Lab->AgentId(1)))
		{
			Check(Advance(), TEXT("continued thermal exposure"));
			return;
		}
		if (TargetActor || View.Rows[1].DeathCause != 2)
		{
			return;
		}
		if (!Check(CCLAgentBiology::Find(*Life.Find(Lab->AgentId(1)))->bProtectNaturalAging &&
					   R->Save(Life, SavedAfter, Error) && R->Restore(Life, SavedBefore, Error),
				   TEXT("protected Agent heat death and injured snapshot restore")))
		{
			return;
		}
		Step = 6;
		Next = Now + 1.;
		return;
	}
	if (Step == 6)
	{
		if (!ASC || !Life.IsAlive(Lab->AgentId(1)) || View.Epoch != R->GetEpoch())
		{
			return;
		}
		if (!Check(FMath::IsNearlyEqual(HP(), InjuredHealth, .0001f), TEXT("injured GAS health restored")))
		{
			return;
		}
		TargetActor->SetActorLocation(HotPosition, false, nullptr, ETeleportType::TeleportPhysics);
		FKSIWorldStep Prepared;
		Prepared.Ticket = FGuid::NewGuid();
		Prepared.WorldFromSeconds = Prepared.WorldToSeconds = R->GetClock().GetWorldSeconds();
		Prepared.StepId = R->GetSurfaceSimulation().GetStepId() + 1;
		Prepared.GameFromSeconds = R->GetClock().GetGameSeconds();
		Prepared.GameToSeconds = Prepared.GameFromSeconds + .25;
		if (!Check(Thermal->Prepare(Prepared, R->GetSurfaceSimulation(), Error) && R->Restore(Life, SavedAfter, Error),
				   TEXT("prepare then restore epoch")))
		{
			return;
		}
		Thermal->Commit();
		Step = 7;
		Next = Now + 1.;
		return;
	}
	if (TargetActor || View.Epoch != R->GetEpoch() || View.Rows[1].DeathCause != 2 || View.Rows[1].Health != 0.f)
	{
		return;
	}
	if (!Check(Write(TEXT("final-view.txt"), Oracle()), TEXT("terminal public oracle")))
	{
		return;
	}
	UE_LOG(LogTemp, Display, TEXT("CCL_THERMAL RESTORE_READY"));
	Finish(true, TEXT("thermal injury rollback departure protected death save restore and stale epoch verified"));
}

void UCCLThermalSmokeSubsystem::Finish(bool Success, const FString& Message)
{
	bComplete = 1;
	UE_LOG(LogTemp, Display, TEXT("CCL_THERMAL %s %s"), Success ? TEXT("PASS") : TEXT("FAIL"), *Message);
	if (!Success || (GetWorld() && GetWorld()->GetNetMode() == NM_Standalone))
	{
		FPlatformMisc::RequestExitWithStatus(false, Success ? 0 : 1, TEXT("CCLThermalSmoke"));
	}
}

TStatId UCCLThermalSmokeSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UCCLThermalSmokeSubsystem, STATGROUP_Tickables);
}
