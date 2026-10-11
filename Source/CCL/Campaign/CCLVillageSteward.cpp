#include "CCLVillageSteward.h"

#include "KSIMapSystem.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/Font.h"
#include "Misc/CoreMisc.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimInstance.h"
#include "UObject/ConstructorHelpers.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
#include "Items/CCLItemDefinition.h"
#include "NativeGameplayTags.h"
#include "Net/UnrealNetwork.h"

namespace
{
	UE_DEFINE_GAMEPLAY_TAG_STATIC(RecoveryPotionStock, "Agent.Resource.RecoveryPotion");
}

ACCLVillageSteward::ACCLVillageSteward()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.1f;
	auto* MapMarker = CreateDefaultSubobject<UKSIMapMarkerComponent>(TEXT("MapMarker"));
	MapMarker->Kind = EKSIMapKind::NPC;

	bReplicates = true;
	Offer.Item = TSoftObjectPtr<UKSIItemDefinition>(FSoftObjectPath(TEXT("/Game/Progression/DA_RecoveryPotion.DA_RecoveryPotion")));
	Offer.Price = 10;
	Offer.StockResource = RecoveryPotionStock;
	GetCharacterMovement()->SetMovementMode(MOVE_None);
	GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, -88.f));
	GetMesh()->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> StewardMesh(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));
	if (StewardMesh.Succeeded())
	{
		GetMesh()->SetSkeletalMesh(StewardMesh.Object);
	}

	static ConstructorHelpers::FClassFinder<UAnimInstance> Anim(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed"));
	if (Anim.Succeeded())
	{
		GetMesh()->SetAnimInstanceClass(Anim.Class);
	}

	auto* Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
	Label->SetupAttachment(GetRootComponent());
	if (!IsRunningDedicatedServer())
	{
		static ConstructorHelpers::FObjectFinder<UFont> KoreanFont(TEXT("/Game/UI/Fonts/F_KoreanLabel.F_KoreanLabel"));
		if (KoreanFont.Succeeded())
		{
			Label->SetFont(KoreanFont.Object);
		}
	}

	Label->SetRelativeLocation(FVector(0.f, 0.f, 130.f));
	Label->SetText(FText::FromString(TEXT("마을 관리인")));
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

void ACCLVillageSteward::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ACCLVillageSteward, Offer);
}

bool ACCLVillageSteward::CanReach(const APawn* Visitor) const
{
	if (!IsValid(Visitor) || Visitor->GetWorld() != GetWorld() || FVector::DistSquared(GetActorLocation(), Visitor->GetActorLocation()) > FMath::Square(275.f))
	{
		return false;
	}

	FHitResult Hit;
	FCollisionQueryParams Query(SCENE_QUERY_STAT(VillageInteraction), false, Visitor);
	Query.AddIgnoredActor(this);
	return !GetWorld()->LineTraceSingleByChannel(Hit, Visitor->GetActorLocation(), GetActorLocation(), ECC_Visibility, Query);
}
