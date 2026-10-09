#include "CCLExperimentStation.h"

#include "CCLExperimentDefinition.h"
#include "CCLExperimentDirector.h"
#include "Components/SceneComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/Font.h"

ACCLExperimentStation::ACCLExperimentStation()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bAllowTickOnDedicatedServer = false;
	PrimaryActorTick.TickInterval = 0.25f;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
	Label->SetupAttachment(RootComponent);
	Label->SetRelativeLocation(FVector(0, 0, 200));
	Label->SetWorldSize(55);
	Label->SetHorizontalAlignment(EHTA_Center);
	Label->SetTextRenderColor(FColor(175, 225, 255));
	State = CreateDefaultSubobject<UTextRenderComponent>(TEXT("State"));
	State->SetupAttachment(RootComponent);
	State->SetRelativeLocation(FVector(0, 0, 90));
	State->SetWorldSize(35);
	State->SetHorizontalAlignment(EHTA_Center);

}

void ACCLExperimentStation::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	if (!Label || !State || IsRunningDedicatedServer())
	{
		return;
	}

	if (auto* Font = LoadObject<UFont>(nullptr, TEXT("/Game/Environment/Fonts/F_EnvironmentLabel.F_EnvironmentLabel")))
	{
		Label->SetFont(Font);
		State->SetFont(Font);
	}
	if (Definition)
	{
		Label->SetText(Definition->Title);
		State->SetText(FText::FromString(Definition->IsImplemented() ? TEXT("시험 준비 · F7 조작 화면") : TEXT("미구현 · 후속 단계에서 연결")));
	}
}

void ACCLExperimentStation::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (Definition && State)
	{
		const auto* Director = ACCLExperimentDirector::Find(GetWorld());
		if (const auto* Result = Director ? Director->FindResult(Definition->CaseId) : nullptr)
		{
			State->SetText(FText::FromString(UCCLExperimentDefinition::StatusText(Result->Status) + TEXT(" · F7 조작 화면")));
		}
	}
}

void ACCLExperimentStation::RefreshLabel()
{
	OnConstruction(GetActorTransform());
}
