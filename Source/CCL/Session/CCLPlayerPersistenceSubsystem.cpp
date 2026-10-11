#include "CCLPlayerPersistenceSubsystem.h"

#include "CCLPlayerState.h"
#include "Agents/CCLAccountComponent.h"
#include "Agents/CCLAgentWorldSubsystem.h"
#include "AbilitySystem/CCLAbilitySystemComponent.h"
#include "AbilitySystem/CCLHealthSet.h"
#include "AbilitySystem/CCLStaminaSet.h"
#include "Campaign/CCLExpeditionComponent.h"
#include "Environment/CCLWorldSimulationSubsystem.h"
#include "Environment/CCLSurfaceSimulation.h"
#include "Items/CCLLoadoutComponent.h"
#include "Items/CCLSkillDefinition.h"
#include "Items/CCLItemDefinition.h"
#include "Items/CCLInventorySnapshot.h"
#include "Items/CCLInventoryComponent.h"
#include "Session/CCLSessionRecord.h"
#include "Engine/World.h"
#include "Misc/ScopeExit.h"

namespace
{
bool Reject(FString& Error, const TCHAR* Message)
{
	Error = Message;
	return false;
}

bool ResolveEntries(const FCCLPlayerSnapshot& Profile, TArray<FCCLInventoryEntry>& Entries, FString& Error)
{
	for (const auto& Item : Profile.Items)
	{
		auto& Entry = Entries.AddDefaulted_GetRef();
		Entry.Id = Item.Id;
		Entry.Definition = FCCLSessionCodec::Item(Item.Definition);
		Entry.Quantity = Item.Quantity;
		Entry.Slot = Item.Slot;
		Entry.LoadedAmmo = Item.LoadedAmmo;
		if (!Entry.Definition || !CCLFood::Decode(Item.Food, Entry.Food, Error))
		{
			return Reject(Error, TEXT("Player item definition or food could not be resolved."));
		}
	}
	return UCCLInventoryComponent::ValidateEntries(Entries, Profile.Capacity) ||
		   Reject(Error, TEXT("Resolved player items are invalid."));
}
} // namespace

bool UCCLPlayerPersistenceSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	const auto* World = Cast<UWorld>(Outer);
	return World && World->IsGameWorld();
}

void UCCLPlayerPersistenceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	if (auto* World = Collection.InitializeDependency<UCCLWorldSimulationSubsystem>())
	{
		World->RegisterParticipant(*this);
	}
}

void UCCLPlayerPersistenceSubsystem::Deinitialize()
{
	if (auto* World = GetWorld()->GetSubsystem<UCCLWorldSimulationSubsystem>())
	{
		World->UnregisterParticipant(*this);
	}
	Super::Deinitialize();
}

bool UCCLPlayerPersistenceSubsystem::CapturePlayer(const FString& Key, ACCLPlayerState& Player,
												   FCCLPlayerSnapshot& Output, FString& Error) const
{
	auto* Inventory = Player.GetInventory();
	auto* Loadout = Player.GetLoadout();
	auto* ASC = Player.GetCCLAbilitySystem();
	auto* Account = Player.FindComponentByClass<UCCLAccountComponent>();
	if (!Player.HasAuthority() || !Inventory || !Loadout || !ASC || !Account || !Player.GetExpedition() ||
		Inventory->GetPersistenceId().IsValid() || Inventory->IsTransactionOpen() ||
		!Inventory->CanRestore(Inventory->GetEntries()))
	{
		return Reject(Error, TEXT("Player capture requires a valid, unbound, idle inventory and profile components."));
	}
	FCCLPlayerSnapshot Candidate;
	Candidate.IdentityKey = Key;
	Candidate.AccountId = Account->GetAccountId();
	Candidate.Capacity = Inventory->Capacity;
	Candidate.Environment = Inventory->GetFoodEnvironment();
	Candidate.EquipmentSlots = Loadout->GetEquipment();
	Candidate.Points = Loadout->GetPoints();
	Candidate.Quest = static_cast<uint8>(Player.GetExpedition()->GetQuest());
	const float MaxHealth = ASC->GetNumericAttribute(UCCLHealthSet::GetMaxHealthAttribute());
	const float MaxStamina = ASC->GetNumericAttribute(UCCLStaminaSet::GetMaxStaminaAttribute());
	if (!FMath::IsFinite(MaxHealth) || !FMath::IsFinite(MaxStamina) || MaxHealth <= 0.f || MaxStamina <= 0.f)
	{
		return Reject(Error, TEXT("Player maximum attributes must be finite and positive."));
	}
	Candidate.HealthFraction = ASC->GetNumericAttribute(UCCLHealthSet::GetHealthAttribute()) / MaxHealth;
	Candidate.StaminaFraction = ASC->GetNumericAttribute(UCCLStaminaSet::GetStaminaAttribute()) / MaxStamina;
	for (auto Skill : Loadout->GetSkills())
	{
		if (Loadout->IsLearned(Skill))
		{
			Candidate.Skills.Add(Skill->GetFName());
		}
	}
	for (const auto& Entry : Inventory->GetEntries())
	{
		auto& Item = Candidate.Items.AddDefaulted_GetRef();
		Item.Id = Entry.Id;
		Item.Definition = Entry.Definition->GetFName();
		Item.Quantity = Entry.Quantity;
		Item.Slot = Entry.Slot;
		Item.LoadedAmmo = Entry.LoadedAmmo;
		if (!CCLFood::Encode(Entry.Food, Item.Food, Error))
		{
			return false;
		}
	}
	Output = MoveTemp(Candidate);
	return true;
}

bool UCCLPlayerPersistenceSubsystem::Capture(FGuid WorldId, double WorldSeconds, TArray<FCCLPlayerSnapshot>& Output,
											 TArray<UCCLInventoryComponent*>& Participants, FString& Error,
											 const UCCLInventoryComponent* SessionInventory) const
{
	if (bPreparing || bCaptureFailed || GetWorld()->GetNetMode() == NM_Client)
	{
		return Reject(Error, TEXT("Player capture is unavailable."));
	}
	auto Candidate = Records;
	TArray<UCCLInventoryComponent*> Owners;
	for (const auto& Pair : Live)
	{
		auto* Player = Pair.Value.Get();
		if (!Player)
		{
			return Reject(Error, TEXT("A registered player disappeared without a captured profile."));
		}
		if (Player->GetInventory() == SessionInventory)
		{
			Candidate.Remove(Pair.Key);
			continue;
		}
		FCCLPlayerSnapshot Profile;
		if (!CapturePlayer(Pair.Key, *Player, Profile, Error))
		{
			return false;
		}
		Candidate.Add(Pair.Key, MoveTemp(Profile));
		Owners.Add(Player->GetInventory());
	}
	TArray<FCCLPlayerSnapshot> Values;
	Candidate.GenerateValueArray(Values);
	Values.Sort([](const auto& A, const auto& B) { return A.IdentityKey < B.IdentityKey; });
	if (!CCLPlayerSnapshot::Validate(Values, WorldId, WorldSeconds, Error))
	{
		return false;
	}
	Output = MoveTemp(Values);
	Participants = MoveTemp(Owners);
	return true;
}

bool UCCLPlayerPersistenceSubsystem::Resolve(const FCCLPlayerSnapshot& Profile, ACCLPlayerState& Player,
											 FCCLLoadoutRestoreRequest& Request, FString& Error) const
{
	if (!Player.HasAuthority() || Player.GetWorld() != GetWorld() || !Player.GetExpedition() ||
		!Player.FindComponentByClass<UCCLAccountComponent>() || !Player.GetInventory() ||
		Player.GetInventory()->Capacity != Profile.Capacity || Player.GetInventory()->GetPersistenceId().IsValid())
	{
		return Reject(Error, TEXT("Player components, capacity or inventory ownership are incompatible."));
	}
	Request.Loadout = Player.GetLoadout();
	Request.Points = Profile.Points;
	Request.Equipment = Profile.EquipmentSlots;
	Request.bResetHealth = 0;
	if (!ResolveEntries(Profile, Request.Entries, Error))
	{
		return false;
	}
	for (const auto Name : Profile.Skills)
	{
		Request.Skills.Add(FCCLSessionCodec::Skill(Name));
	}
	return Request.Loadout &&
				   Request.Loadout->CanRestoreState(Request.Entries, Request.Equipment, Request.Skills, Request.Points)
			   ? true
			   : Reject(Error, TEXT("Player loadout is unavailable for restore."));
}

void UCCLPlayerPersistenceSubsystem::Finalize(ACCLPlayerState& Player, const FCCLPlayerSnapshot& Profile)
{
	Player.GetInventory()->FoodEnvironment = Profile.Environment;
	verify(Player.FindComponentByClass<UCCLAccountComponent>()->BindAccount(Profile.AccountId));
	verify(Player.GetExpedition()->Restore(0, static_cast<ECCLQuestStatus>(Profile.Quest), false));
	auto* ASC = Player.GetCCLAbilitySystem();
	ASC->SetNumericAttributeBase(UCCLHealthSet::GetHealthAttribute(),
								 Profile.HealthFraction *
									 ASC->GetNumericAttribute(UCCLHealthSet::GetMaxHealthAttribute()));
	ASC->SetNumericAttributeBase(UCCLStaminaSet::GetStaminaAttribute(),
								 Profile.StaminaFraction *
									 ASC->GetNumericAttribute(UCCLStaminaSet::GetMaxStaminaAttribute()));
	Player.ForceNetUpdate();
}

bool UCCLPlayerPersistenceSubsystem::Bind(ACCLPlayerState& Player, const FString& Key, FString& Error)
{
	auto* WorldState = GetWorld()->GetSubsystem<UCCLWorldSimulationSubsystem>();
	auto* Agents = GetWorld()->GetSubsystem<UCCLAgentWorldSubsystem>();
	if (bPreparing || bCaptureFailed || !Player.HasAuthority() || Player.GetWorld() != GetWorld() || !WorldState ||
		!WorldState->IsRunning() || !Agents || !Agents->IsRunning())
	{
		return Reject(Error, TEXT("Player binding needs an idle running server world."));
	}
	for (const auto& Pair : Live)
	{
		if (Pair.Key == Key || Pair.Value.Get() == &Player)
		{
			return Pair.Key == Key && Pair.Value.Get() == &Player
					   ? true
					   : Reject(Error, TEXT("Player or identity key is already bound."));
		}
	}
	FCCLPlayerSnapshot Profile;
	const bool bSaved = Records.Contains(Key);
	if (bSaved)
	{
		Profile = Records[Key];
	}
	else if (!CapturePlayer(Key, Player, Profile, Error))
	{
		return false;
	}
	TArray<FCCLPlayerSnapshot> Values;
	TArray<UCCLInventoryComponent*> Owners;
	if (!Capture(WorldState->GetIdentity().WorldId, WorldState->GetClock().GetWorldSeconds(), Values, Owners, Error))
	{
		return false;
	}
	Values.RemoveAll([&](const auto& Value) { return Value.IdentityKey == Key; });
	Values.Add(Profile);
	if (!CCLPlayerSnapshot::Validate(Values, WorldState->GetIdentity().WorldId,
									 WorldState->GetClock().GetWorldSeconds(), Error) ||
		!Agents->GetSimulation().GetEconomy().Accounts.Contains(Profile.AccountId))
	{
		return Reject(Error, TEXT("Player identity, item ownership or world account is invalid."));
	}
	if (bSaved)
	{
		FCCLLoadoutRestoreRequest Request;
		if (!Resolve(Profile, Player, Request, Error))
		{
			return false;
		}
		TGuardValue<uint8> Guard(bPreparing, 1);
		if (!FCCLLoadoutRestoreBatch::Execute(
				MakeArrayView(&Request, 1), [] { return true; }, [&] { Finalize(Player, Profile); }))
		{
			return Reject(Error, TEXT("Saved player effect preparation was rejected."));
		}
	}
	Live.Add(Key, &Player);
	Player.GetInventory()->bExternalPersistence = 1;
	Records.Add(Key, MoveTemp(Profile));
	return true;
}

void UCCLPlayerPersistenceSubsystem::Unbind(ACCLPlayerState& Player)
{
	FString Key;
	for (const auto& Pair : Live)
	{
		if (Pair.Value.Get() == &Player)
		{
			Key = Pair.Key;
			break;
		}
	}
	if (Key.IsEmpty())
	{
		return;
	}
	FCCLPlayerSnapshot Profile;
	FString Error;
	if (bPreparing || !CapturePlayer(Key, Player, Profile, Error))
	{
		bCaptureFailed = 1;
		UE_LOG(LogTemp, Error, TEXT("CCL_PLAYER unload capture failed: %s"), *Error);
	}
	else
	{
		Records.Add(Key, MoveTemp(Profile));
	}
	Live.Remove(Key);
	Player.GetInventory()->bExternalPersistence = 0;
}

bool UCCLPlayerPersistenceSubsystem::StageSessionRestore(const FCCLLoadoutRestoreRequest& Request, TFunction<void()> InFinalize,
														 FString& Error)
{
	if (bPreparing || bRestoreStaged || GetWorld()->GetNetMode() == NM_Client || !IsValid(Request.Loadout) ||
		!IsValid(Request.Loadout->GetOwner()) || Request.Loadout->GetWorld() != GetWorld())
	{
		return Reject(Error, TEXT("Session restore participant must belong to this idle server world."));
	}
	SessionRequest = Request;
	SessionFinalize = MoveTemp(InFinalize);
	return true;
}

void UCCLPlayerPersistenceSubsystem::ClearSessionRestore()
{
	if (!bRestoreStaged)
	{
		SessionRequest.Reset();
		SessionFinalize = {};
	}
}

bool UCCLPlayerPersistenceSubsystem::OwnsInventory(const UCCLInventoryComponent& Inventory) const
{
	if (bPreparing)
	{
		return false;
	}
	for (const auto& Pair : Live)
	{
		if (const auto* Player = Pair.Value.Get(); Player && Player->GetInventory() == &Inventory)
		{
			return true;
		}
	}
	return false;
}

bool UCCLPlayerPersistenceSubsystem::IsBound(const ACCLPlayerState& Player) const
{
	for (const auto& Pair : Live)
	{
		if (Pair.Value.Get() == &Player)
		{
			return true;
		}
	}
	return false;
}

FName UCCLPlayerPersistenceSubsystem::GetWorldSection() const
{
	return CCLPlayerSnapshot::WorldSection();
}

bool UCCLPlayerPersistenceSubsystem::CaptureWorldSection(FCCLWorldCaptureContext& Context, TArray<uint8>& OutBytes,
														 FString& Error)
{
	TArray<FCCLPlayerSnapshot> Profiles;
	TArray<UCCLInventoryComponent*> Owners;
	if (!Capture(Context.WorldId, Context.WorldSeconds, Profiles, Owners, Error,
				 Cast<UCCLInventoryComponent>(Context.SessionContainer)) ||
		!CCLPlayerSnapshot::Encode(Profiles, Context.WorldId, Context.WorldSeconds, OutBytes, Error))
	{
		return false;
	}
	for (const auto* Owner : Owners)
	{
		Context.ClaimedContainers.Add(Owner);
	}
	return true;
}

bool UCCLPlayerPersistenceSubsystem::PrepareWorldRestore(FCCLWorldRestoreContext& Context, const TArray<uint8>* Section,
														 FString& Error)
{
	TArray<FCCLPlayerSnapshot> Input;
	if (bPreparing || bRestoreStaged || GetWorld()->GetNetMode() == NM_Client ||
		(Section && !CCLPlayerSnapshot::Decode(*Section, Context.WorldId, Context.WorldSeconds, Input, Error)) ||
		!CCLPlayerSnapshot::Validate(Input, Context.WorldId, Context.WorldSeconds, Error))
	{
		return Reject(Error, Error.IsEmpty() ? TEXT("Player restore is unavailable or invalid.") : *FString(Error));
	}
	RestoreCandidate.Reset();
	for (const auto& Profile : Input)
	{
		RestoreCandidate.Add(Profile.IdentityKey, Profile);
	}
	RestoreRequests.Reset();
	RestoreResolved.Reset();
	bRestoreStarting = Context.bStarting;

	// A starting world has no bound players yet; it only adopts the stored profiles.
	if (Context.bStarting)
	{
		if (!Live.IsEmpty() || SessionRequest.IsSet())
		{
			ResetRestore();
			return Reject(Error, TEXT("Players are already bound to this world."));
		}
		bRestoreStaged = 1;
		return true;
	}

	for (const auto& Pair : Live)
	{
		auto* Player = Pair.Value.Get();
		if (!Player)
		{
			ResetRestore();
			return Reject(Error, TEXT("Registered player no longer exists."));
		}
		if (SessionRequest && Player->GetLoadout() == SessionRequest->Loadout)
		{
			if (RestoreCandidate.Contains(Pair.Key))
			{
				ResetRestore();
				return Reject(Error, TEXT("Session and world cannot both restore one player."));
			}
			continue;
		}

		// A player who joined after the snapshot keeps the current profile instead of being rewound.
		const auto* Profile = RestoreCandidate.Find(Pair.Key);
		if (!Profile)
		{
			FCCLPlayerSnapshot Current;
			if (!CapturePlayer(Pair.Key, *Player, Current, Error))
			{
				ResetRestore();
				return false;
			}
			for (const auto& Item : Current.Items)
			{
				Context.KeptItems.Add(Item.Id);
			}
			Context.ClaimedContainers.Add(Player->GetInventory());
			RestoreCandidate.Add(Pair.Key, MoveTemp(Current));
			continue;
		}
		FCCLLoadoutRestoreRequest Request;
		if (!Resolve(*Profile, *Player, Request, Error))
		{
			ResetRestore();
			return Reject(Error, TEXT("A live player is incompatible with the saved profile."));
		}
		Context.ClaimedContainers.Add(Player->GetInventory());
		RestoreRequests.Add(MoveTemp(Request));
		RestoreResolved.Emplace(Player, *Profile);
	}
	if (SessionRequest)
	{
		if (SessionRequest->Entries.ContainsByPredicate([&](const FCCLInventoryEntry& Entry) { return Context.KeptItems.Contains(Entry.Id); }))
		{
			ResetRestore();
			return Reject(Error, TEXT("The session restores an item that a kept player holds."));
		}
		Context.ClaimedContainers.Add(SessionRequest->Loadout->GetOwner()->FindComponentByClass<UCCLInventoryComponent>());
		RestoreRequests.Add(*SessionRequest);
	}

	// Kept profiles join the restored ones, so item ownership and food times must agree with the saved world.
	TArray<FCCLPlayerSnapshot> Merged;
	RestoreCandidate.GenerateValueArray(Merged);
	if (!CCLPlayerSnapshot::Validate(Merged, Context.WorldId, Context.WorldSeconds, Error))
	{
		ResetRestore();
		return Reject(Error, TEXT("A player who joined after the saved world holds items or food that conflict with it."));
	}
	bPreparing = 1;
	bRestoreStaged = 1;
	return true;
}

bool UCCLPlayerPersistenceSubsystem::CommitWorldRestore(TFunctionRef<bool()> CommitInner)
{
	check(bRestoreStaged);
	auto FinalizeAll = [this]
	{
		Records = MoveTemp(RestoreCandidate);
		bCaptureFailed = 0;
		for (auto& Pair : RestoreResolved)
		{
			if (auto* Player = Pair.Key.Get())
			{
				Finalize(*Player, Pair.Value);
			}
		}

		// Kept players may have opened their accounts after the saved world; reopen them with the current balance.
		for (const auto& Pair : Live)
		{
			if (auto* Player = Pair.Value.Get())
			{
				if (auto* Account = Player->FindComponentByClass<UCCLAccountComponent>())
				{
					Account->EnsureOpen();
				}
			}
		}
		if (SessionFinalize)
		{
			SessionFinalize();
		}
	};

	// Loadouts, the session and every inner participant commit inside one gameplay effect batch.
	if (RestoreRequests.IsEmpty())
	{
		if (!CommitInner())
		{
			return false;
		}
		FinalizeAll();
		return true;
	}
	return FCCLLoadoutRestoreBatch::Execute(RestoreRequests, CommitInner, FinalizeAll);
}

void UCCLPlayerPersistenceSubsystem::PublishWorldRestore()
{
	ResetRestore();
}

void UCCLPlayerPersistenceSubsystem::CancelWorldRestore()
{
	ResetRestore();
}

void UCCLPlayerPersistenceSubsystem::ResetRestore()
{
	RestoreCandidate.Reset();
	RestoreRequests.Reset();
	RestoreResolved.Reset();
	SessionRequest.Reset();
	SessionFinalize = {};
	bRestoreStaged = 0;
	bRestoreStarting = 0;
	bPreparing = 0;
}

bool UCCLPlayerPersistenceSubsystem::NeedsWorldAdvance() const
{
	for (const auto& Pair : Live)
	{
		if (const auto* Player = Pair.Value.Get(); Player && Player->GetInventory()->GetFoodEnvironment().IsDynamic())
		{
			return true;
		}
	}
	for (const auto& Pair : Records)
	{
		if (!Live.Contains(Pair.Key) && Pair.Value.Environment.IsDynamic())
		{
			return true;
		}
	}
	return false;
}

bool UCCLPlayerPersistenceSubsystem::PrepareWorldAdvance(const FCCLWorldStep& Step, const FCCLSurfaceSimulation& Surface,
														 FString& Error)
{
	TArray<FCCLPlayerSnapshot> Values;
	TArray<UCCLInventoryComponent*> Owners;
	if (!Capture(Surface.GetWorldId(), Step.WorldFromSeconds, Values, Owners, Error))
	{
		return false;
	}
	TMap<FString, FCCLPlayerSnapshot> Candidate;
	TMap<TWeakObjectPtr<UCCLInventoryComponent>, TArray<FCCLInventoryEntry>> Entries;
	for (auto& Profile : Values)
	{
		if (Profile.Environment.IsDynamic())
		{
			FCCLInventorySnapshot FoodRecord;
			FoodRecord.Environment = Profile.Environment;
			for (const auto& Item : Profile.Items)
			{
				FoodRecord.Items.AddDefaulted_GetRef().Food = Item.Food;
			}
			if (!CCLFoodEnvironment::Advance(FoodRecord, Surface, Step.WorldFromSeconds, Error))
			{
				return false;
			}
			Profile.Environment = FoodRecord.Environment;
			for (int32 Index = 0; Index < Profile.Items.Num(); ++Index)
			{
				Profile.Items[Index].Food = FoodRecord.Items[Index].Food;
			}
			if (auto* Player = Live.FindRef(Profile.IdentityKey).Get())
			{
				if (!ResolveEntries(Profile, Entries.FindOrAdd(Player->GetInventory()), Error))
				{
					return false;
				}
			}
		}
		Candidate.Add(Profile.IdentityKey, MoveTemp(Profile));
	}
	TArray<FCCLPlayerSnapshot> Check;
	Candidate.GenerateValueArray(Check);
	if (!CCLPlayerSnapshot::Validate(Check, Surface.GetWorldId(), Step.WorldToSeconds, Error))
	{
		return false;
	}
	bPreparing = 1;
	Prepared = MoveTemp(Candidate);
	Pending = MoveTemp(Entries);
	for (auto& Pair : Pending)
	{
		if (!Pair.Key.IsValid() || !Pair.Key->BeginTransaction())
		{
			CancelWorldAdvance();
			return Reject(Error, TEXT("Player food inventory is busy."));
		}
		Locked.Add(Pair.Key);
	}
	return true;
}

void UCCLPlayerPersistenceSubsystem::CancelWorldAdvance()
{
	for (auto& Owner : Locked)
	{
		if (Owner.IsValid())
		{
			Owner->EndTransaction(false);
		}
	}
	Locked.Reset();
	Pending.Reset();
	Prepared.Reset();
	bPreparing = 0;
}

void UCCLPlayerPersistenceSubsystem::CommitWorldAdvance()
{
	check(bPreparing);
	Records = MoveTemp(Prepared);
	for (const auto& Pair : Live)
	{
		auto* Player = Pair.Value.Get();
		if (Player && Pending.Contains(Player->GetInventory()))
		{
			Player->GetInventory()->FoodEnvironment = Records[Pair.Key].Environment;
			Player->GetInventory()->RestoreValidated(Pending[Player->GetInventory()]);
		}
	}
	Pending.Reset();
}

void UCCLPlayerPersistenceSubsystem::PublishWorldAdvance()
{
	// Inventory transactions held every notification until all participants committed.
	auto Notify = MoveTemp(Locked);
	for (auto& Owner : Notify)
	{
		if (Owner.IsValid())
		{
			Owner->EndTransaction(true);
		}
	}
	bPreparing = 0;
}
