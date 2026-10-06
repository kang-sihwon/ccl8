#include "CCLVillageSteward.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimInstance.h"
#include "UObject/ConstructorHelpers.h"
ACCLVillageSteward::ACCLVillageSteward()
{
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
bool ACCLVillageSteward::CanReach(const APawn* Visitor) const
{
	if (!IsValid(Visitor) || Visitor->GetWorld() != GetWorld() || FVector::DistSquared(GetActorLocation(), Visitor->GetActorLocation()) > FMath::Square(275.f)) { return false; }
	FHitResult Hit;
	FCollisionQueryParams Query(SCENE_QUERY_STAT(VillageInteraction), false, Visitor);
	Query.AddIgnoredActor(this);
	return !GetWorld()->LineTraceSingleByChannel(Hit, Visitor->GetActorLocation(), GetActorLocation(), ECC_Visibility, Query);
}

