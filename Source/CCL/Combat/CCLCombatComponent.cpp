#include "CCLCombatComponent.h"

#include "CCLCombatDefinition.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Engine/World.h"

UCCLCombatComponent::UCCLCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UCCLCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!GetOwner()->HasAuthority() || !ActiveDefinition)
	{
		return;
	}

	const FVector Start = GetOwner()->GetActorLocation();
	TArray<FHitResult> Hits;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(CCLAttack), false, GetOwner());
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

void UCCLCombatComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	EndAttack();
	Super::EndPlay(EndPlayReason);
}

uint32 UCCLCombatComponent::BeginAttack(const UCCLCombatDefinition* Definition, const FVector& Direction)
{
	if (!GetOwner()->HasAuthority() || !Definition)
	{
		return 0;
	}

	ActiveDefinition = Definition;
	AttackDirection = Direction.GetSafeNormal2D();
	HitActors.Reset();
	++CurrentAttackId;
	SetComponentTickEnabled(true);
	return CurrentAttackId;
}

void UCCLCombatComponent::EndAttack()
{
	ActiveDefinition = nullptr;
	HitActors.Reset();
	SetComponentTickEnabled(false);
}

FGameplayTag UCCLCombatComponent::ResolveHit(AActor* Target, const FHitResult& Hit, uint32 AttackId)
{
	AActor* Source = GetOwner();

	if (!Source->HasAuthority() || !ActiveDefinition || AttackId != CurrentAttackId || !IsValid(Target) || Target == Source || HitActors.Contains(Target))
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
	FCollisionQueryParams VisibilityParams(SCENE_QUERY_STAT(CCLHitVisibility), false, Source);

	if (GetWorld()->LineTraceSingleByChannel(Obstruction, Source->GetActorLocation(), Target->GetActorLocation(), ECC_Visibility, VisibilityParams) && Obstruction.GetActor() != Target)
	{
		return FGameplayTag();
	}

	const UCCLCombatDefinition* Definition = ActiveDefinition;

	if (!Definition->HitRule)
	{
		return FGameplayTag();
	}

	FCCLHitContext Context{Source, Target, SourceASC, TargetASC, Definition, Hit, AttackId};
	// Reserve before policy side effects: cancellation can synchronously re-enter the resolver.
	HitActors.Add(Target);
	const FGameplayTag Outcome = CCLHit::Apply(Context);
	OnHitResolved.Broadcast(Target, Outcome);
	return Outcome;
}
