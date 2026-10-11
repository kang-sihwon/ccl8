#include "KSICombatComponent.h"

#include "KSICombatDefinition.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Engine/World.h"

UKSICombatComponent::UKSICombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UKSICombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!GetOwner()->HasAuthority() || !ActiveDefinition)
	{
		return;
	}

	const FVector Start = GetOwner()->GetActorLocation();
	TArray<FHitResult> Hits;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(KSIAttack), false, GetOwner());
	GetWorld()->SweepMultiByChannel(Hits, Start, Start + AttackDirection * ActiveDefinition->Reach,
	    FQuat::Identity, ECC_Pawn, FCollisionShape::MakeSphere(ActiveDefinition->Radius), Params);
	const uint32 Id = CurrentAttackId;

	for (const FHitResult& Hit : Hits)
	{
		if (!ActiveDefinition || Id != CurrentAttackId)
		{
			break;
		}

		ResolveHit(Hit.GetActor(), Hit, Id);
	}
}

void UKSICombatComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	EndAttack();
	Super::EndPlay(EndPlayReason);
}

uint32 UKSICombatComponent::BeginAttack(const UKSICombatDefinition* Definition, const FVector& Direction)
{
	if (!IsValid(GetOwner()) || !GetOwner()->HasAuthority() || bResolvingHit || !IsValid(Definition) || !Definition->HitRule ||
		!FMath::IsFinite(Definition->Reach) || Definition->Reach < 0.f ||
		!FMath::IsFinite(Definition->Radius) || Definition->Radius <= 0.f ||
		Direction.ContainsNaN() || Direction.GetSafeNormal2D().IsNearlyZero())
	{
		return 0;
	}

	ActiveDefinition = Definition;
	AttackDirection = Direction.GetSafeNormal2D();
	HitActors.Reset();
	if (++CurrentAttackId == 0)
	{
		++CurrentAttackId;
	}
	SetComponentTickEnabled(true);
	return CurrentAttackId;
}

void UKSICombatComponent::EndAttack()
{
	ActiveDefinition = nullptr;
	HitActors.Reset();
	SetComponentTickEnabled(false);
}

FGameplayTag UKSICombatComponent::ResolveHit(AActor* Target, const FHitResult& Hit, uint32 AttackId)
{
	AActor* Source = GetOwner();

	if (!IsValid(Source) || !Source->HasAuthority() || bResolvingHit || Hit.ImpactPoint.ContainsNaN() || !ActiveDefinition || AttackId != CurrentAttackId || !IsValid(Target) || Target == Source || HitActors.Contains(Target))
	{
		return FGameplayTag();
	}

	UAbilitySystemComponent* SourceASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Source);
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);

	if (!SourceASC || !TargetASC || SourceASC->GetAvatarActor() != Source || TargetASC->GetAvatarActor() != Target)
	{
		return FGameplayTag();
	}

	if (FVector::DistSquared(Source->GetActorLocation(), Hit.ImpactPoint) > FMath::Square(ActiveDefinition->Reach + ActiveDefinition->Radius + 1.f))
	{
		return FGameplayTag();
	}

	FHitResult Obstruction;
	FCollisionQueryParams VisibilityParams(SCENE_QUERY_STAT(KSIHitVisibility), false, Source);

	if (GetWorld()->LineTraceSingleByChannel(Obstruction, Source->GetActorLocation(), Target->GetActorLocation(), ECC_Visibility, VisibilityParams) && Obstruction.GetActor() != Target)
	{
		return FGameplayTag();
	}

	const UKSICombatDefinition* Definition = ActiveDefinition;

	if (!Definition->HitRule)
	{
		return FGameplayTag();
	}

	TGuardValue<uint8> Resolving(bResolvingHit, 1);
	FKSIHitContext Context{Source, Target, SourceASC, TargetASC, Definition, Hit, AttackId};
	// Reserve before policy side effects: cancellation can synchronously re-enter the resolver.
	HitActors.Add(Target);
	const FGameplayTag Outcome = KSIHit::Apply(Context);
	OnHitResolved.Broadcast(Target, Outcome);
	return Outcome;
}
