#include "KSICombatProbe.h"

#include "KSIAbilitySystemComponent.h"
#include "KSIAbilitySet.h"
#include "KSIActionComponent.h"
#include "KSICombatComponent.h"
#include "KSICombatDefinition.h"
#include "Components/SphereComponent.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "NativeGameplayTags.h"
#include "Modules/ModuleManager.h"

UE_DEFINE_GAMEPLAY_TAG_STATIC(ProbeDamage, "Probe.Outcome.Damage");
UE_DEFINE_GAMEPLAY_TAG_STATIC(ProbeMagnitude, "Probe.Data.Magnitude");
UE_DEFINE_GAMEPLAY_TAG_STATIC(ProbeAction, "Probe.Action.Trigger");

namespace
{
    int32 Commits = 0;
    int32 Activations = 0;
    FGameplayAttribute HealthAttribute()
    {
        return FGameplayAttribute(FindFProperty<FProperty>(UKSIProbeHealth::StaticClass(), GET_MEMBER_NAME_CHECKED(UKSIProbeHealth, Health)));
    }
}

UKSIProbeDamage::UKSIProbeDamage()
{
    DurationPolicy = EGameplayEffectDurationType::Instant;
    FGameplayModifierInfo Modifier;
    Modifier.Attribute = HealthAttribute();
    Modifier.ModifierOp = EGameplayModOp::Additive;
    FSetByCallerFloat Caller;
    Caller.DataTag = ProbeMagnitude;
    Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(Caller);
    Modifiers.Add(Modifier);
}

FKSIHitResolution UKSIProbeHitRule::Resolve(const FKSIHitContext& Context) const
{
    return {ProbeDamage, UKSIProbeDamage::StaticClass(), ProbeMagnitude, -10.f};
}

void UKSIProbeHitRule::Commit(const FKSIHitContext& Context, const FKSIHitResolution& Resolution) const
{
    ++Commits;
}

void UKSIProbeReceiver::ReceiveHit(const FKSIHitContext& Context, FGameplayTag Outcome)
{
    ++Hits;
}

UKSIProbeAbility::UKSIProbeAbility()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
}

void UKSIProbeAbility::ActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* Info,
    FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* EventData)
{
    ++Activations;
    // Remain active until the source releases, cancels or is removed.
}

AKSIProbeTarget::AKSIProbeTarget()
{
    auto* Sphere = CreateDefaultSubobject<USphereComponent>(TEXT("Body"));
    Sphere->InitSphereRadius(20.f);
    Sphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Sphere->SetCollisionResponseToAllChannels(ECR_Ignore);
    Sphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
    Sphere->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    RootComponent = Sphere;
    ASC = CreateDefaultSubobject<UKSIAbilitySystemComponent>(TEXT("ASC"));
    Combat = CreateDefaultSubobject<UKSICombatComponent>(TEXT("Combat"));
    Actions = CreateDefaultSubobject<UKSIActionComponent>(TEXT("Actions"));
    Health = CreateDefaultSubobject<UKSIProbeHealth>(TEXT("Health"));
    Receiver = CreateDefaultSubobject<UKSIProbeReceiver>(TEXT("Receiver"));
}

void AKSIProbeTarget::BeginPlay()
{
    Super::BeginPlay();
    ASC->InitAbilityActorInfo(this, this);
    ASC->AddAttributeSetSubobject(Health.Get());
}

UAbilitySystemComponent* AKSIProbeTarget::GetAbilitySystemComponent() const
{
    return ASC;
}

bool RunKSICombatProbe(UWorld* World)
{
    bool bPassed = true;
    auto Check = [&bPassed](bool Condition, const TCHAR* Label)
    {
        bPassed &= Condition;
        UE_LOG(LogTemp, Display, TEXT("KSI_PROBE %s %s"), Condition ? TEXT("CHECK") : TEXT("FAIL"), Label);
    };
    Check(!FModuleManager::Get().IsModuleLoaded("CCL") && !FModuleManager::Get().IsModuleLoaded("KSIInventory") &&
        !FModuleManager::Get().IsModuleLoaded("KSIUI"), TEXT("combat without game, inventory, UI or character modules"));
    const FVector Origin(10000, 10000, 1000);
    auto* Source = World->SpawnActor<AKSIProbeTarget>(Origin, FRotator::ZeroRotator);
    auto* Target = World->SpawnActor<AKSIProbeTarget>(Origin + FVector(100, 0, 0), FRotator::ZeroRotator);
    auto* Definition = NewObject<UKSICombatDefinition>(Source);
    Definition->HitRule = UKSIProbeHitRule::StaticClass();
    FHitResult Hit(Target, Cast<UPrimitiveComponent>(Target->GetRootComponent()), Target->GetActorLocation(), -FVector::ForwardVector);
    auto Begin = [&]() { return Source->Combat->BeginAttack(Definition, FVector::ForwardVector); };
    auto Resolve = [&](uint32 Id) { return Source->Combat->ResolveHit(Target, Hit, Id); };
    Commits = 0;
    const uint32 First = Begin();
    Check(First != 0 && Resolve(First) == ProbeDamage && Target->Health->Health.GetCurrentValue() == 90.f,
        TEXT("trap damages destructible Actor using only GAS health"));
    Check(!Resolve(First).IsValid() && Commits == 1 && Target->Receiver->Hits == 1, TEXT("one commit and receiver per target and attack"));
    Source->Combat->EndAttack();
    Check(!Resolve(First).IsValid(), TEXT("ended attack rejected"));
    const uint32 Next = Begin();
    Check(Next != First && !Resolve(First).IsValid(), TEXT("stale attack id rejected"));
    Source->ASC->InitAbilityActorInfo(Source, Target);
    Check(!Resolve(Next).IsValid(), TEXT("stale source avatar rejected"));
    FKSIHitContext Context{Source, Target, Source->ASC, Target->ASC, Definition, Hit, Next};
    Check(!KSIHit::Apply(Context).IsValid(), TEXT("direct resolver also rejects stale source avatar"));
    Source->ASC->InitAbilityActorInfo(Source, Source);
    Target->ASC->InitAbilityActorInfo(Target, Source);
    Check(!Resolve(Next).IsValid(), TEXT("stale target avatar rejected"));
    Target->ASC->InitAbilityActorInfo(Target, Target);
    FHitResult Far = Hit;
    Far.ImpactPoint += FVector(1000, 0, 0);
    Check(!Source->Combat->ResolveHit(Target, Far, Next).IsValid(), TEXT("out of range hit rejected"));
    auto* Wall = World->SpawnActor<AActor>();
    auto* Box = NewObject<UBoxComponent>(Wall);
    Wall->SetRootComponent(Box);
    Wall->AddInstanceComponent(Box);
    Box->SetBoxExtent(FVector(5, 100, 100));
    Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Box->SetCollisionResponseToAllChannels(ECR_Ignore);
    Box->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    Box->RegisterComponent();
    Wall->SetActorLocation(Origin + FVector(50, 0, 0));
    Check(!Resolve(Next).IsValid(), TEXT("visibility obstruction rejected"));
    Wall->Destroy();
    const auto Reject = FGameplayEffectApplicationQuery::CreateLambda([](const FActiveGameplayEffectsContainer&, const FGameplayEffectSpec&) { return false; });
    Target->ASC->GameplayEffectApplicationQueries.Add(Reject);
    Check(!Resolve(Next).IsValid() && Commits == 1 && Target->Receiver->Hits == 1 && Target->Health->Health.GetCurrentValue() == 90.f,
        TEXT("GAS rejected effect leaves health, commit and receiver untouched"));
    Target->ASC->GameplayEffectApplicationQueries.Reset();
    Check(!Resolve(Next).IsValid(), TEXT("rejected effect still consumes target reservation"));
    const uint32 Reentrant = Begin();
    bool bReentryBlocked = false;
    auto Listener = Source->Combat->OnHitResolved.AddLambda([&](AActor*, FGameplayTag)
    {
        Source->Combat->EndAttack();
        bReentryBlocked = Begin() == 0 && !Resolve(Reentrant).IsValid();
    });
    Check(Resolve(Reentrant) == ProbeDamage && bReentryBlocked && Commits == 2, TEXT("synchronous cancel and reentry cannot duplicate damage"));
    Source->Combat->OnHitResolved.Remove(Listener);
    Definition->Radius = -1;
    Check(Begin() == 0, TEXT("invalid query radius rejected"));
    Definition->Radius = 35;
    Check(Source->Combat->BeginAttack(Definition, FVector::ZeroVector) == 0, TEXT("empty direction rejected"));
    Begin();
    Source->Combat->TickComponent(0.016f, LEVELTICK_All, nullptr);
    Check(Commits == 3 && Target->Health->Health.GetCurrentValue() == 70.f, TEXT("actual sphere sweep reaches destructible target"));
    Source->Combat->EndAttack();
    // Detached effects preserve attribution data without requiring the old source avatar.
    Context.bDetachedSource = 1;
    Context.CapturedEffect = Source->ASC->MakeOutgoingSpec(UKSIProbeDamage::StaticClass(), 1.f, Source->ASC->MakeEffectContext());
    Context.SourceASC = nullptr;
    Check(KSIHit::Apply(Context) == ProbeDamage && Target->Health->Health.GetCurrentValue() == 60.f,
        TEXT("captured detached effect applies after source ASC is unavailable"));
    auto* Abilities = NewObject<UKSIAbilitySet>(Source);
    FKSIAbilityGrant Grant;
    Grant.Ability = UKSIProbeAbility::StaticClass();
    Grant.InputTag = ProbeAction;
    Abilities->Abilities.Add(Grant);
    Abilities->GrantTo(*Source->ASC, Definition);
    Abilities->GrantTo(*Source->ASC, Definition);
    Check(Source->ASC->GetActivatableAbilities().Num() == 1, TEXT("ability set avoids duplicate grants"));
    Activations = 0;
    Source->Actions->RequestInput(ProbeAction, true);
    auto* Spec = Source->ASC->FindAbilitySpecFromClass(UKSIProbeAbility::StaticClass());
    Check(Spec && Spec->IsActive() && Spec->InputPressed && Activations == 1, TEXT("generic action input activates tagged ability"));
    Source->ASC->ReleaseAllInputs();
    Check(Spec && !Spec->InputPressed, TEXT("release all clears pressed input"));
    Source->ASC->CancelAllAbilities();
    Source->ASC->ClearAllAbilities();
    FKSIActionGrant Action;
    Action.Action = ProbeAction;
    Action.Ability = UKSIProbeAbility::StaticClass();
    const FGuid Id = FGuid::NewGuid();
    Check(!Source->Actions->RegisterSource(Id, Definition, {Action, Action}), TEXT("duplicate action tags rejected"));
    Check(Source->Actions->RegisterSource(Id, Definition, {Action}) && !Source->Actions->RegisterSource(Id, Definition, {Action}),
        TEXT("generic source registers once without inventory"));
    Check(Source->Actions->Execute(Id, ProbeAction) && Activations == 2, TEXT("source action executes"));
    Spec = Source->ASC->FindAbilitySpecFromClass(UKSIProbeAbility::StaticClass());
    Check(Spec && Source->Actions->FindSource(Spec->Handle) && Source->Actions->FindSource(Spec->Handle)->Id == Id, TEXT("ability handle resolves source identity"));
    Source->Actions->Release(Id, ProbeAction);
    Check(Spec && !Spec->IsActive(), TEXT("release cancels source action"));
    Source->Actions->Execute(Id, ProbeAction);
    Source->Actions->RemoveSource(Id);
    Check(!Source->Actions->HasSource(Id) && Source->ASC->GetActivatableAbilities().IsEmpty(), TEXT("remove cancels and clears granted abilities"));
    Source->Actions->RegisterSource(Id, Definition, {Action});
    Source->Actions->Execute(Id, ProbeAction);
    Source->Actions->DestroyComponent();
    Check(Source->ASC->GetActivatableAbilities().IsEmpty(), TEXT("component end play clears source lifetime"));
    Source->Destroy();
    Target->Destroy();
    return bPassed;
}
