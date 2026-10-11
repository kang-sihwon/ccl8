#include "CCLEcologySpaceSmokeSubsystem.h"
#include "Environment/CCLEcologySpace.h"
#include "Environment/CCLEcologyRepresentationSubsystem.h"
#include "Environment/CCLCreatureActor.h"
#include "Environment/CCLTerrainRegion.h"
#include "Environment/CCLExperimentDirector.h"
#include "Environment/CCLWorldSimulationSubsystem.h"
#include "Agents/CCLAgentWorldSubsystem.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "NavigationSystem.h"
#include "NavigationData.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
	FGuid Id(uint32 N)
	{
		return FGuid(950,951,952,N);
	}
	const FVector Origin(25.,25.,30.);
	const EKSIEcologyMovement Media[]={EKSIEcologyMovement::Air,EKSIEcologyMovement::Water,EKSIEcologyMovement::Ground,EKSIEcologyMovement::Insect};
	FKSISurfaceGrid WaterGrid(bool bDry)
	{
		FKSISurfaceGrid G;
		G.RegionId=Id(1);
		G.OriginMeters=Origin;
		G.Size=FIntPoint(12,12);
		G.SpacingMeters=.5;
		G.Forcing.FlowConductance=0.;
		G.Forcing.InfiltrationMetersPerWorldSecond=0.;
		G.Forcing.DrainageMetersPerWorldSecond=0.;
		G.Forcing.PhaseMetersPerDegreeWorldSecond=0.;
		G.Cells.SetNum(144);
		for (int32 N=0; N<G.Cells.Num(); ++N)
		{
			auto& C=G.Cells[N];
			C.BedMeters=Origin.Z;
			C.CeilingMeters=Origin.Z+4.;
			C.WaterCubicMeters=bDry && N%12==6 ? 0. : .95;
		}
		G.Ledger.InitialCubicMeters=G.TotalCubicMeters();
		return G;
	}
}

bool UCCLEcologySpaceSmokeSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
#if UE_BUILD_SHIPPING || UE_BUILD_TEST
	return false;
#else
	return FParse::Param(FCommandLine::Get(),TEXT("CCLEcologySpaceSmoke"));
#endif
}

void UCCLEcologySpaceSmokeSubsystem::Tick(float DeltaTime)
{
	const double Now=FPlatformTime::Seconds();
	if (!Started)
	{
		Started=Now;
	}

	if (Now-Started>240.)
	{
		Finish(false,FString::Printf(TEXT("timeout step=%d"),Step));
		return;
	}

	auto* W=GetWorld();
	auto* Runtime=W?W->GetSubsystem<UCCLWorldSimulationSubsystem>():nullptr;
	auto* Agents=W?W->GetSubsystem<UCCLAgentWorldSubsystem>():nullptr;
	auto* R=W?W->GetSubsystem<UCCLEcologyRepresentationSubsystem>():nullptr;
	auto* Director=W?ACCLExperimentDirector::Find(W):nullptr;
	if (!W || !Runtime || !Agents || !R || Now<Next)
	{
		return;
	}

	FString Directory;
	FParse::Value(FCommandLine::Get(),TEXT("CCLEcologySpaceDir="),Directory);
	FString Error;
	auto Check=[&](bool OK,const TCHAR* Label) { if (!OK) { Finish(false,FString::Printf(TEXT("step=%d %s: %s"),Step,Label,*Error)); } return OK; };
	if (!Check(!Directory.IsEmpty(),TEXT("isolated output directory")))
	{
		return;
	}

	if (W->GetNetMode()==NM_Client)
	{
		FString ExpectedText;
		const bool bFinal=FPaths::FileExists(Directory/TEXT("poses.txt"));
		if (!FFileHelper::LoadFileToString(ExpectedText,*(Directory/(bFinal?TEXT("poses.txt"):TEXT("initial.txt")))))
		{
			return;
		}

		TArray<FString> Lines;
		ExpectedText.ParseIntoArrayLines(Lines);
		if (Lines.Num()!=4)
		{
			return;
		}

		int32 Seen=0;
		for (TActorIterator<ACCLCreatureActor> It(W);It;++It)
		{
			for (uint32 N=0;N<4;++N)
			{
				if (It->GetIndividualId()!=Id(60+N))
				{
					continue;
				}

				TArray<FString> Parts;
				Lines[N].ParseIntoArray(Parts,TEXT(","));
				if (!Check(Parts.Num()==3,TEXT("position oracle")))
				{
					return;
				}

				const FVector Expected(FCString::Atod(*Parts[0]),FCString::Atod(*Parts[1]),FCString::Atod(*Parts[2]));
				if (It->GetMovementMedium()==Media[N]&&It->GetActorLocation().Equals(Expected*100.,1.))
				{
					++Seen;
				}
			}
		}

		if (Seen!=4)
		{
			return;
		}

		if (!bFinal)
		{
			if (!Step)
			{
				UE_LOG(LogTemp,Display,TEXT("CCL_ECOLOGY_SPACE BASE_OBSERVED"));
				Step=1;
			}

			return;
		}
		FKSIEcologyBehavior Forbidden;
		Forbidden.Id=Id(60);
		if (!Check(!Runtime->AddEcologyBehavior(Forbidden,Error),TEXT("client spatial policy mutation refused")))
		{
			return;
		}

		Finish(true,TEXT("four spatial actor positions restored and replicated; authority enforced"));
		return;
	}

	if (!Runtime->IsRunning()||!Director||!Director->IsReady())
	{
		return;
	}

	const bool bRead=FParse::Param(FCommandLine::Get(),TEXT("CCLEcologySpaceRead"));
	FCCLEcologyWorldSpace Space(W);
	auto& E=Runtime->GetEcologySimulation();
	auto WritePoses=[&](const TCHAR* Name)
	{
		FString Poses;
		for (uint32 N=0;N<4;++N)
		{
			const auto* Individual=E.FindIndividual(Id(60+N));
			if (!Individual)
			{
				Error=TEXT("Expected spatial individual missing.");
				return false;
			}

			const auto& P=Individual->PositionMeters;
			Poses+=FString::Printf(TEXT("%.12f,%.12f,%.12f\n"),P.X,P.Y,P.Z);
		}
		return FFileHelper::SaveStringToFile(Poses,*(Directory/Name));
	};
	auto RecreateActors=[&]()
	{
		for (uint32 N=0;N<4;++N)
		{
			if (!R->SetRepresentation(Id(60+N),EKSIEcologyRepresentation::Actor,Error))
			{
				return false;
			}
		}
		return R->Flush(Error);
	};
	auto AdvanceBoundary=[&]()
	{
		if (!bAdvanceQueued)
		{
			if (!Check(Runtime->QueueGameTime(.25,Error),TEXT("queue one spatial input")))
			{
				return false;
			}

			bAdvanceQueued=1;
		}
		TArray<uint8> Before,After,SurfaceBefore,SurfaceAfter;
		if (!Check(E.Capture(Before,Error)&&Runtime->GetSurfaceSimulation().Capture(SurfaceBefore,Error),TEXT("capture retry baseline")))
		{
			return false;
		}

		const double WorldBefore=Runtime->GetClock().GetWorldSeconds();
		if (!Runtime->AdvancePending(.25,1.,Error))
		{
			const FString Reason=Error;
			if (!Check(Reason.Contains(TEXT("awaits"))&&E.Capture(After,Error)&&After==Before
				&&Runtime->GetSurfaceSimulation().Capture(SurfaceAfter,Error)&&SurfaceAfter==SurfaceBefore
				&&Runtime->GetClock().GetWorldSeconds()==WorldBefore,TEXT("unready space preserves candidate and queued input")))
			{
				return false;
			}

			UE_LOG(LogTemp,Display,TEXT("CCL_ECOLOGY_SPACE RETRY_PRESERVED %s"),*Reason);
			Next=Now+.25;
			return true;
		}
		bAdvanceQueued=0;
		return true;
	};
	if (Step==0)
	{
		if (!Check(Runtime->ChangeTimeScale(0.,Error),TEXT("freeze")))
		{
			return;
		}

		Runtime->SetTickableTickType(ETickableTickType::Never);
		Step=1;
		Next=Now+2.;
		return;
	}
	if (Step==1)
	{
		auto* Terrain=Director->GetTerrainRegion();
		if (!Terrain || !Terrain->IsNavigationReady())
		{
			auto* Nav=FNavigationSystem::GetCurrent<UNavigationSystemV1>(W);
			UE_LOG(LogTemp,Display,TEXT("CCL_ECOLOGY_SPACE WAIT terrain=%s ready=%d nav=%s dirty=%d tasks=%d"),
				*GetNameSafe(Terrain),Terrain?int32(Terrain->IsTerrainReady()):0,
				*GetNameSafe(Nav?Nav->GetDefaultNavDataInstance(FNavigationSystem::DontCreate):nullptr),
				Nav?int32(Nav->IsNavigationDirty()):-1,Nav?Nav->GetNumRemainingBuildTasks():-1);
			Next=Now+5.;
			return;
		}
		FKSIEcologyRouteRequest Q;
		const FVector Base=Terrain->GetActorLocation()/100.;
		Q.FromMeters=Base+FVector(-6.,3.,.35);
		Q.TargetMeters=Base+FVector(6.,3.,.35);
		Q.HabitatMeters=FBox(Terrain->GetWorldTerrainBounds().Min/100.,Terrain->GetWorldTerrainBounds().Max/100.);
		Q.WorldSeconds=Runtime->GetClock().GetWorldSeconds();
		TArray<FVector> Route;
		if (!Space.FindRoute(Q,&Runtime->GetSurfaceSimulation(),Route,Error))
		{
			UE_LOG(LogTemp,Display,TEXT("CCL_ECOLOGY_SPACE WAIT query=%s"),*Error);
			Next=Now+5.;
			return;
		}

		if (!Check(!Route.IsEmpty(),TEXT("real terrain navigation route")))
		{
			return;
		}

		UE_LOG(LogTemp,Display,TEXT("CCL_ECOLOGY_SPACE GROUND_READY points=%d"),Route.Num());
		const FKSIEcologyRouteRequest Ground=Q;
		auto* Wall=W->SpawnActor<AActor>();
		auto* Box=NewObject<UBoxComponent>(Wall);
		Wall->SetRootComponent(Box);
		Wall->AddInstanceComponent(Box);
		Box->SetMobility(EComponentMobility::Movable);
		Box->SetBoxExtent(FVector(20.,150.,180.));
		Box->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Box->SetCollisionResponseToAllChannels(ECR_Block);
		Box->SetCanEverAffectNavigation(false);
		Box->RegisterComponent();
		Wall->SetActorLocation((Origin+FVector(3.,3.,2.))*100.);
		if (bRead)
		{
			if (!Check(FFileHelper::LoadFileToArray(SavedWorld,*(Directory/TEXT("world.bin")))
				&&FFileHelper::LoadFileToArray(SavedEcology,*(Directory/TEXT("ecology.bin")))
				&&Runtime->Restore(Agents->GetSimulation(),SavedWorld,Error),TEXT("new process spatial restore")))
			{
				return;
			}

			TArray<uint8> Actual;
			if (!Check(E.Capture(Actual,Error)&&Actual==SavedEcology&&E.HasSpatialBehavior()
				&&RecreateActors()&&Runtime->ChangeTimeScale(4.,Error),TEXT("restored spatial history and actors")))
			{
				return;
			}

			InitialTime=Runtime->GetClock().GetWorldSeconds();
			Step=5;
			return;
		}
		Q.Medium=EKSIEcologyMovement::Air;
		Q.FromMeters=Origin+FVector(1.,3.,2.);
		Q.TargetMeters=Origin+FVector(5.,3.,2.);
		Q.HabitatMeters=FBox(Origin,Origin+FVector(6.,6.,4.));
		if (!Check(Space.FindRoute(Q,nullptr,Route,Error)&&Route.Num()>2,TEXT("air sphere routes around actual wall")))
		{
			return;
		}

		double Length = 0.;
		for (int32 N = 1; N < Route.Num(); ++N)
		{
			Length += FVector::Distance(Route[N - 1], Route[N]);
		}

		if (!Check(Length>4.1,TEXT("detour distance exceeds blocked shortcut")))
		{
			return;
		}

		Box->SetBoxExtent(FVector(20.,310.,210.),true);
		if (!Check(Space.FindRoute(Q,nullptr,Route,Error)&&Route.IsEmpty(),TEXT("sealed wall cannot be crossed")))
		{
			return;
		}

		Box->SetBoxExtent(FVector(20.,150.,180.),true);
		FKSISurfaceSimulation Water,Dry;
		if (!Check(Water.Initialize(E.GetWorldId(),0.,Q.WorldSeconds,0,Error)&&Water.AddRegion(WaterGrid(false),Error)
			&&Dry.Initialize(E.GetWorldId(),0.,Q.WorldSeconds,0,Error)&&Dry.AddRegion(WaterGrid(true),Error),TEXT("water fixtures")))
		{
			return;
		}

		Q.Medium=EKSIEcologyMovement::Water;
		if (!Check(Space.FindRoute(Q,&Water,Route,Error)&&Route.Num()>2,TEXT("fish follows liquid around wall")))
		{
			return;
		}

		for (const FVector& Point:Route)
		{
			if (!Check(FCCLEcologyWorldSpace::IsInLiquid(Water,Point,Q.RadiusMeters),TEXT("fish body remains submerged")))
			{
				return;
			}
		}

		if (!Check(Space.FindRoute(Q,&Dry,Route,Error)&&Route.IsEmpty(),TEXT("dry band disconnects water route")))
		{
			return;
		}

		if (!Check(Runtime->AddSurfaceRegion(WaterGrid(false),Error),TEXT("world water input")))
		{
			return;
		}

		for (uint32 N=0;N<4;++N)
		{
			const auto& RouteInput=N<2?Q:Ground;
			FKSIEcologySpecies S;
			S.Id=Id(10+N);
			S.Movement=Media[N];
			FKSIEcologyHabitat H;
			H.Id=Id(20+N);
			H.MinimumMeters=RouteInput.HabitatMeters.Min;
			H.MaximumMeters=RouteInput.HabitatMeters.Max;
			H.MovementMask=1<<uint8(S.Movement);
			FKSIEcologyPopulation P;
			P.Id=Id(30+N);
			P.SpeciesId=S.Id;
			P.HabitatId=H.Id;
			P.Cohorts.Add(Id(40+N),{Id(40+N),Q.WorldSeconds,2});
			FKSIEcologyEvent Event;
			Event.Id=Id(50+N);
			Event.Kind=EKSIEcologyEventKind::Materialize;
			Event.Source=P.Id;
			Event.CohortId=Id(40+N);
			Event.IndividualId=Id(60+N);
			Event.BirthWorldSeconds=Q.WorldSeconds;
			Event.bPersistent=1;
			FKSIEcologyForage F;
			F.Id=Id(70+N);
			F.HabitatId=H.Id;
			F.PositionMeters=RouteInput.TargetMeters;
			FKSIEcologyBehavior B;
			B.Id=Event.IndividualId;
			B.bUseWorldSpace=1;
			B.Energy=.1;
			B.MetabolismPerSecond=.005;
			if (!Check(Runtime->AddEcologySpecies(S,Error)&&Runtime->AddEcologyHabitat(H,Error)&&Runtime->AddEcologyPopulation(P,Error)
				&&Runtime->ApplyEcologyEvent(Event,Error)&&R->SetRepresentation(B.Id,EKSIEcologyRepresentation::Actor,Error)
				&&R->SetPosition(B.Id,RouteInput.FromMeters,Error)&&Runtime->AddEcologyForage(F,Error)&&Runtime->AddEcologyBehavior(B,Error),TEXT("world spatial behavior")))
			{
				return;
			}
		}
		InitialTime=Runtime->GetClock().GetWorldSeconds();
		if (!Check(R->Flush(Error)&&WritePoses(TEXT("initial.txt")),TEXT("initial position oracle")))
		{
			return;
		}

		UE_LOG(LogTemp,Display,TEXT("CCL_ECOLOGY_SPACE BASE_READY"));
		Step=2;
		return;
	}
	if (Step==2)
	{
		if (W->GetNetMode()!=NM_Standalone&&!FPaths::FileExists(Directory/TEXT("first-ready.txt")))
		{
			return;
		}

		if (!Check(Runtime->ChangeTimeScale(4.,Error),TEXT("start spatial movement")))
		{
			return;
		}

		Step=3;
		return;
	}
	if (Step==3)
	{
		if (Runtime->GetClock().GetWorldSeconds()<InitialTime+20.)
		{
			if (!AdvanceBoundary())
			{
				return;
			}

			return;
		}

		if (!Check(Runtime->ChangeTimeScale(0.,Error),TEXT("freeze after movement")))
		{
			return;
		}

		Step=4;
		Next=Now+1.;
		return;
	}
	if (Step==4)
	{
		if (!Check(R->Flush(Error),TEXT("pose publication")))
		{
			return;
		}

		for (uint32 N=0;N<4;++N)
		{
			const auto& B=E.GetBehaviors()[Id(60+N)];
			const auto* I=E.FindIndividual(B.Id);
			UE_LOG(LogTemp,Display,TEXT("CCL_ECOLOGY_SPACE ACTOR medium=%u consumed=%.6f position=%s activity=%u"),N,B.ConsumedUnits,*I->PositionMeters.ToString(),uint8(B.Activity));
			if (!Check(B.ConsumedUnits>0.
				&&R->Find(B.Id)->Actor->GetActorLocation().Equals(I->PositionMeters*100.,.1),TEXT("real actor followed spatial route and ate")))
			{
				return;
			}
		}
		if (!Check(Runtime->Save(Agents->GetSimulation(),SavedWorld,Error)&&E.Capture(SavedEcology,Error)
			&&FFileHelper::SaveArrayToFile(SavedWorld,*(Directory/TEXT("world.bin")))
			&&FFileHelper::SaveArrayToFile(SavedEcology,*(Directory/TEXT("ecology.bin")))
			&&Runtime->ChangeTimeScale(4.,Error),TEXT("save spatial policy and history")))
		{
			return;
		}

		InitialTime=Runtime->GetClock().GetWorldSeconds();
		Step=5;
		return;
	}
	if (Step==5)
	{
		if (Runtime->GetClock().GetWorldSeconds()<InitialTime+8.)
		{
			if (!AdvanceBoundary())
			{
				return;
			}

			return;
		}
		TArray<uint8> Actual,Expected;
		if (!Check(Runtime->ChangeTimeScale(0.,Error)&&R->Flush(Error)&&E.Capture(Actual,Error),TEXT("capture continued spatial state")))
		{
			return;
		}

		if (bRead)
		{
			if (!Check(FFileHelper::LoadFileToArray(Expected,*(Directory/TEXT("continued.bin")))&&Actual==Expected,
				TEXT("new process continuation matches uninterrupted spatial state")))
			{
				return;
			}

			UE_LOG(LogTemp,Display,TEXT("CCL_ECOLOGY_SPACE CONTINUATION_MATCH"));
		}
		else if (!Check(FFileHelper::SaveArrayToFile(Actual,*(Directory/TEXT("continued.bin"))),TEXT("continued spatial oracle")))
		{
			return;
		}

		if (!Check(Runtime->Restore(Agents->GetSimulation(),SavedWorld,Error)&&E.Capture(Actual,Error)
			&&Actual==SavedEcology&&E.HasSpatialBehavior()&&RecreateActors()&&WritePoses(TEXT("poses.txt")),
			TEXT("spatial restore after further movement and actor publication")))
		{
			return;
		}

		UE_LOG(LogTemp,Display,TEXT("CCL_ECOLOGY_SPACE RESTORE_READY"));
		Step=6;
		Next=Now+1.;
		return;
	}
	if (Step==6)
	{
		if (W->GetNetMode()!=NM_Standalone&&!FPaths::FileExists(Directory/TEXT("clients-ready.txt")))
		{
			return;
		}

		Finish(true,bRead?TEXT("new process spatial continuation and exact restore"):
			TEXT("ground navigation air detour aquatic volume feeding save restore and observed poses"));
	}
}

void UCCLEcologySpaceSmokeSubsystem::Finish(bool Success,const FString& Message)
{
	bComplete=1;
	UE_LOG(LogTemp,Display,TEXT("CCL_ECOLOGY_SPACE %s %s"),Success?TEXT("PASS"):TEXT("FAIL"),*Message);
	if (!Success||!GetWorld()||GetWorld()->GetNetMode()==NM_Standalone||GetWorld()->GetNetMode()==NM_Client)
	{
		FPlatformMisc::RequestExitWithStatus(false,Success?0:1,TEXT("CCLEcologySpaceSmoke"));
	}
}

TStatId UCCLEcologySpaceSmokeSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UCCLEcologySpaceSmokeSubsystem,STATGROUP_Tickables);
}
