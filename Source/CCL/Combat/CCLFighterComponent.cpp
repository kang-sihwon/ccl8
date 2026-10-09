#include "CCLFighterComponent.h"
#include "Items/CCLAttachmentProfile.h"
#include "GameFramework/Character.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"

#include "CCLCombatDefinition.h"
#include "CCLCombatComponent.h"
#include "CCLHitRule.h"
#include "AbilitySystem/CCLAbilitySystemComponent.h"
#include "AbilitySystem/CCLAbilitySet.h"
#include "AbilitySystem/CCLEffects.h"
#include "AbilitySystem/CCLGameplayTags.h"
#include "AbilitySystem/CCLHealthSet.h"
#include "AbilitySystem/CCLStaminaSet.h"
#include "Items/CCLItemDefinition.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "DrawDebugHelpers.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimInstance.h"
#include "Net/UnrealNetwork.h"

UCCLFighterComponent::UCCLFighterComponent()
{
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = true;
}

void UCCLFighterComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!Item)
	{
		Item = LoadObject<UCCLItemDefinition>(nullptr, Team == 0 ? TEXT("/Game/Combat/DA_Unarmed.DA_Unarmed") : TEXT("/Game/Combat/DA_EnemyUnarmed.DA_EnemyUnarmed"));
	}
}

void UCCLFighterComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ASC)
	{
		ASC->GetGameplayAttributeValueChangeDelegate(UCCLStaminaSet::GetStaminaAttribute()).Remove(StaminaChanged);

		if (ASC->GetAvatarActor() == GetOwner())
		{
			ASC->ReleaseAllInputs();

			if (GetOwner()->HasAuthority())
			{
				ASC->CancelAllAbilities();
			}

			ASC->ClearActorInfo();
		}
	}

	Super::EndPlay(EndPlayReason);
}

void UCCLFighterComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	auto* Character = Cast<ACharacter>(GetOwner());

	if (!Character || !ASC)
	{
		return;
	}

	const bool bStaggered = ASC->HasMatchingGameplayTag(CCLTags::State_Stagger);
	const bool bFacing = Action == ECCLCombatAction::Guard || Action == ECCLCombatAction::Parry || Action == ECCLCombatAction::Attack;
	Character->GetCharacterMovement()->bOrientRotationToMovement = !bFacing && !bStaggered;

	if (bFacing && !bAttackDirectionLocked && !IsDead() && Character->GetController() && (Character->HasAuthority() || Character->IsLocallyControlled()))
	{
		Character->SetActorRotation(FRotator(0.f, Character->GetControlRotation().Yaw, 0.f));
	}

	if (bStaggered)
	{
		Character->GetCharacterMovement()->StopMovementImmediately();
	}

	if (GetWorld()->GetTimeSeconds() > FeedbackExpires)
	{
		Feedback.Empty();
	}

	if (GetNetMode() != NM_DedicatedServer && (Action == ECCLCombatAction::Guard || Action == ECCLCombatAction::Parry || bStaggered))
	{
		const FColor Color = bStaggered ? FColor::Red : (Action == ECCLCombatAction::Guard ? FColor::Blue : FColor::Yellow);
		DrawDebugSphere(GetWorld(), Character->GetActorLocation() + Character->GetActorForwardVector() * 55.f, 45.f, 16, Color, false, 0.f, 0, 3.f);
	}
}

void UCCLFighterComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UCCLFighterComponent, Action);
	DOREPLIFETIME(UCCLFighterComponent, Item);
	DOREPLIFETIME(UCCLFighterComponent, LeftHandItem);
	DOREPLIFETIME(UCCLFighterComponent, RightHandItem);
	DOREPLIFETIME(UCCLFighterComponent, AttackOverride);
}

void UCCLFighterComponent::Initialize(UCCLAbilitySystemComponent* InASC)
{
	if (!InASC)
	{
		return;
	}

	if (ASC)
	{
		ASC->GetGameplayAttributeValueChangeDelegate(UCCLStaminaSet::GetStaminaAttribute()).Remove(StaminaChanged);
	}

	ASC = InASC;

	if (!DodgeMontage)
	{
		DodgeMontage = LoadObject<UAnimMontage>(nullptr, TEXT("/Game/Combat/AM_Dodge.AM_Dodge"));
	}

	if (!DefenseMontage)
	{
		DefenseMontage = LoadObject<UAnimMontage>(nullptr, TEXT("/Game/Combat/AM_DefensePose.AM_DefensePose"));
	}

	if (auto* Character = Cast<ACharacter>(GetOwner()))
	{
		if (auto* AnimClass = LoadClass<UAnimInstance>(nullptr, TEXT("/Game/Combat/ABP_Combat.ABP_Combat_C")))
		{
			Character->GetMesh()->SetAnimInstanceClass(AnimClass);
		}
	}

	if (!Item)
	{
		Item = LoadObject<UCCLItemDefinition>(nullptr, Team == 0 ? TEXT("/Game/Combat/DA_Unarmed.DA_Unarmed") : TEXT("/Game/Combat/DA_EnemyUnarmed.DA_EnemyUnarmed"));
	}

	if (GetOwner()->HasAuthority())
	{
		ASC->ReleaseAllInputs();
		ASC->CancelAllAbilities();
		ASC->RemoveActiveEffectsWithGrantedTags(FGameplayTagContainer(CCLTags::Effect_Life));
		ASC->InitAbilityActorInfo(ASC->GetOwner(), GetOwner());

		if (Item)
		{
			const auto* Fragment = Item->FindFragment<FCCLItemFragment_Weapon>();

			if (Fragment && Fragment->Abilities)
			{
				Fragment->Abilities->GrantTo(*ASC, Item);
			}
		}

		UGameplayEffect* Initial = NewObject<UGameplayEffect>(GetTransientPackage());
		Initial->DurationPolicy = EGameplayEffectDurationType::Instant;
		auto Add = [Initial](FGameplayAttribute Attribute, float Value)
		{
			FGameplayModifierInfo& Modifier = Initial->Modifiers.AddDefaulted_GetRef();
			Modifier.Attribute = Attribute;
			Modifier.ModifierOp = EGameplayModOp::Override;
			Modifier.ModifierMagnitude = FScalableFloat(Value);
		};
		Add(UCCLHealthSet::GetMaxHealthAttribute(), InitialHealth);
		Add(UCCLHealthSet::GetHealthAttribute(), InitialHealth);

		if (ASC->GetSet<UCCLStaminaSet>())
		{
			Add(UCCLStaminaSet::GetMaxStaminaAttribute(), InitialStamina);
			Add(UCCLStaminaSet::GetStaminaAttribute(), InitialStamina);
		}

		ASC->ApplyGameplayEffectToSelf(Initial, 1.f, ASC->MakeEffectContext());

		if (ASC->GetSet<UCCLStaminaSet>())
		{
			ASC->ApplyEffect(UCCLStaminaRegenEffect::StaticClass(), RegenRate * 0.1f);
		}
	}

	StaminaChanged = ASC->GetGameplayAttributeValueChangeDelegate(UCCLStaminaSet::GetStaminaAttribute()).AddUObject(this, &ThisClass::OnStaminaChanged);
}

void UCCLFighterComponent::EndLife()
{
	if (!ASC || ASC->GetAvatarActor() != GetOwner())
	{
		return;
	}

	ASC->ReleaseAllInputs();
	ASC->CancelAllAbilities();

	if (GetOwner()->HasAuthority())
	{
		ASC->RemoveActiveEffectsWithGrantedTags(FGameplayTagContainer(CCLTags::Effect_Life));
		ASC->ApplyEffect(UCCLDeadEffect::StaticClass());
	}

	SetAction(ECCLCombatAction::None);
}

void UCCLFighterComponent::SetAction(ECCLCombatAction NewAction)
{
	Action = NewAction;
	bAttackDirectionLocked = 0;
	OnRep_Action();
}

void UCCLFighterComponent::LockAttackDirection()
{
	bAttackDirectionLocked = 1;
}

void UCCLFighterComponent::NotifyHit(FGameplayTag Outcome)
{
	if (GetOwner()->HasAuthority())
	{
		MulticastFeedback(Outcome);
	}
}

const UCCLCombatDefinition* UCCLFighterComponent::GetAttack() const
{
	if (AttackOverride) { return AttackOverride; }
	const auto* Fragment = Item ? Item->FindFragment<FCCLItemFragment_Weapon>() : nullptr;
	return Fragment ? Fragment->Combat.Get() : nullptr;
}

bool UCCLFighterComponent::IsDead() const
{
	return ASC && ASC->HasMatchingGameplayTag(CCLTags::State_Dead);
}

void UCCLFighterComponent::OnStaminaChanged(const FOnAttributeChangeData& Data)
{
	if (GetOwner()->HasAuthority() && ASC && Data.NewValue < Data.OldValue)
	{
		ASC->RemoveActiveEffectsWithGrantedTags(FGameplayTagContainer(CCLTags::State_RecoveryDelay));
		ASC->ApplyEffect(UCCLRecoveryDelayEffect::StaticClass(), 0.f, RegenDelay);
	}
}

void UCCLFighterComponent::OnRep_Action()
{
	// Animation is replicated through GAS montage state; this state also drives temporary defense poses.
}

void UCCLFighterComponent::MulticastFeedback_Implementation(FGameplayTag Outcome)
{
	if (Outcome == CCLTags::Outcome_Parried)
	{
		Feedback = TEXT("패링 성공");
	}
	else if (Outcome == CCLTags::Outcome_Guarded)
	{
		Feedback = TEXT("방어");
	}
	else if (Outcome == CCLTags::Outcome_GuardBroken)
	{
		Feedback = TEXT("방어 붕괴");
	}
	else if (Outcome == CCLTags::Outcome_Damage)
	{
		Feedback = TEXT("피격");
	}
	else
	{
		Feedback = TEXT("회피");
	}

	FeedbackExpires = GetWorld()->GetTimeSeconds() + 0.8;
}

void UCCLFighterComponent::RefreshEquipmentVisuals()
{
	for (UMeshComponent* Mesh : EquipmentVisuals)
	{
		if (Mesh)
		{
			Mesh->DestroyComponent();
		}
	}

	EquipmentVisuals.Reset();
	const auto* Profile = AttachmentProfile.LoadSynchronous();
	auto* Character = Cast<ACharacter>(GetOwner());
	if (!Character || !Character->GetMesh())
	{
		return;
	}

	for (int32 Hand = 0; Hand < 2; ++Hand)
	{
		const auto* Definition = Hand == 0 ? LeftHandItem.Get() : RightHandItem.Get();
		if (!Definition)
		{
			continue;
		}

		if (Hand == 0 && CCLEquipment::IsTwoHanded(Definition))
		{
			continue;
		}

		const auto* Visual = Definition->FindFragment<FCCLItemFragment_Visual>();
		if (!Visual)
		{
			continue;
		}

		const auto* Binding = Visual->FindAttachment(CCLEquipment::SlotAt(Hand));
		FName Socket;
		FString Error;
		if (!Binding || !Profile ||
		    !Profile->Resolve(Character->GetMesh()->GetSkeletalMeshAsset(), Binding->Point, Socket, Error))
		{
			UE_LOG(LogTemp, Warning, TEXT("CCL attachment rejected for %s: %s"), *Definition->GetName(), *Error);
			continue;
		}

		UMeshComponent* Mesh = nullptr;
		if (const auto* Skeletal = Definition->FindFragment<FCCLItemFragment_SkeletalVisual>(); Skeletal && Skeletal->EquippedMesh)
		{
			auto* Component = NewObject<USkeletalMeshComponent>(Character);
			Component->SetSkeletalMesh(Skeletal->EquippedMesh);
			if (Skeletal->AnimClass)
			{
				Component->SetAnimInstanceClass(Skeletal->AnimClass);
			}

			Mesh = Component;
		}
		else if (Visual->DroppedMesh)
		{
			auto* Component = NewObject<UStaticMeshComponent>(Character);
			Component->SetStaticMesh(Visual->DroppedMesh);
			Mesh = Component;
		}

		if (Mesh)
		{
			Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Mesh->SetupAttachment(Character->GetMesh(), Socket);
			Mesh->SetRelativeTransform(Binding->Offset);
			Mesh->RegisterComponent();
			EquipmentVisuals.Add(Mesh);
		}
	}
}
