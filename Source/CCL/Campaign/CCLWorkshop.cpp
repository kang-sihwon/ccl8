#include "CCLWorkshop.h"

#include "Agents/CCLAgentWorldSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/Font.h"
#include "Engine/StaticMesh.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"

ACCLWorkshop::ACCLWorkshop()
{
	bReplicates = true;
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 1;
	Store = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Storage"));
	RootComponent = Store;
	Store->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Mesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	Store->SetStaticMesh(Mesh.Object);
	Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
	Label->SetupAttachment(Store);
	static ConstructorHelpers::FObjectFinder<UFont> KoreanFont(TEXT("/Game/UI/Fonts/F_KoreanLabel.F_KoreanLabel"));
	if (KoreanFont.Succeeded()) { Label->SetFont(KoreanFont.Object); }
	Label->SetRelativeLocation(FVector(0, 0, 110));
	Label->SetWorldSize(16);
	Label->SetAbsolute(false, false, true);
	Label->SetHorizontalAlignment(EHTA_Center);
	Refresh();
}
void ACCLWorkshop::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (auto* PC = GetWorld()->GetFirstPlayerController(); PC && PC->PlayerCameraManager && PC->GetPawn())
	{
		Label->SetWorldRotation((PC->PlayerCameraManager->GetCameraLocation() - Label->GetComponentLocation()).Rotation());
		Label->SetVisibility(FVector::DistSquared(PC->GetPawn()->GetActorLocation(), GetActorLocation()) < FMath::Square(300.f));
	}
	const auto* World = GetWorld()->GetSubsystem<UCCLAgentWorldSubsystem>();
	const auto* Ownership = World && World->IsRunning() ? World->GetSimulation().GetEconomy().Ownerships.Find(OwnershipId) : nullptr;
	if (HasAuthority() && Ownership && Level != Ownership->ImprovementLevel)
	{
		Level = Ownership->ImprovementLevel;
		Refresh();
		ForceNetUpdate();
	}
}
void ACCLWorkshop::Refresh()
{
	Store->SetRelativeScale3D(FVector(0.55f + Level * 0.18f, 0.55f, 0.4f + Level * 0.15f));
	Label->SetText(FText::FromString(FString::Printf(TEXT("창고 %d단계"), Level)));
}
void ACCLWorkshop::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ACCLWorkshop, Level);
}
