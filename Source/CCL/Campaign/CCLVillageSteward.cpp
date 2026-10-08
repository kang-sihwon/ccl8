#include "CCLVillageSteward.h"

#include "Map/CCLMapSystem.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimInstance.h"
#include "UObject/ConstructorHelpers.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
ACCLVillageSteward::ACCLVillageSteward()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.1f;
	auto* MapMarker = CreateDefaultSubobject<UCCLMapMarkerComponent>(TEXT("MapMarker"));
	MapMarker->Kind = ECCLMapKind::NPC;

	bReplicates = true;
	GetCharacterMovement()->SetMovementMode(MOVE_None);
	GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, -88.f));
	GetMesh()->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> StewardMesh(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));
	if (StewardMesh.Succeeded()) { GetMesh()->SetSkeletalMesh(StewardMesh.Object); }
	static ConstructorHelpers::FClassFinder<UAnimInstance> Anim(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed"));
	if (Anim.Succeeded()) { GetMesh()->SetAnimInstanceClass(Anim.Class); }
	auto* Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
	Label->SetupAttachment(GetRootComponent());
	Label->SetRelativeLocation(FVector(0.f, 0.f, 130.f));
	Label->SetText(FText::FromString(TEXT("VILLAGE STEWARD")));
	Label->SetWorldSize(20.f);
	Label->SetHorizontalAlignment(EHTA_Center);
	Label->SetTextRenderColor(FColor::Yellow);
}
void ACCLVillageSteward::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	auto* PC = GetWorld()->GetFirstPlayerController();
	auto* Label = FindComponentByClass<UTextRenderComponent>();
	if (PC && PC->PlayerCameraManager && PC->GetPawn() && Label)
	{
		const FVector ToLabel = Label->GetComponentLocation() - PC->PlayerCameraManager->GetCameraLocation();
		Label->SetWorldRotation((-ToLabel).Rotation());
		Label->SetVisibility(FVector::DistSquared(PC->GetPawn()->GetActorLocation(), GetActorLocation()) < FMath::Square(450.f) &&
			FVector::DotProduct(ToLabel.GetSafeNormal(), PC->PlayerCameraManager->GetCameraRotation().Vector()) > 0.98f);
	}
}
bool ACCLVillageSteward::CanReach(const APawn* Visitor) const
{
	if (!IsValid(Visitor) || Visitor->GetWorld() != GetWorld() || FVector::DistSquared(GetActorLocation(), Visitor->GetActorLocation()) > FMath::Square(275.f)) { return false; }
	FHitResult Hit;
	FCollisionQueryParams Query(SCENE_QUERY_STAT(VillageInteraction), false, Visitor);
	Query.AddIgnoredActor(this);
	return !GetWorld()->LineTraceSingleByChannel(Hit, Visitor->GetActorLocation(), GetActorLocation(), ECC_Visibility, Query);
}
