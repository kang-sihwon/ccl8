#include "CCLUISubsystem.h"

#include "CCLScreen.h"
#include "CCLUIContext.h"
#include "CCLUIRoot.h"
#include "Engine/AssetManager.h"
#include "Engine/LocalPlayer.h"
#include "Engine/StreamableManager.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Misc/ScopeExit.h"

void UCCLUISubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	WorldCleanupHandle = FWorldDelegates::OnWorldCleanup.AddUObject(this, &ThisClass::OnWorldCleanup);
}

void UCCLUISubsystem::Deinitialize()
{
	bShuttingDown = 1;
	FWorldDelegates::OnWorldCleanup.Remove(WorldCleanupHandle);
	TArray<FGuid> Requests;
	Pending.GetKeys(Requests);
	for (FGuid Id : Requests)
	{
		CancelRequest({Id});
	}

	CloseAllViews();
	ResetRoot();
	Registrations.Reset();
	Registry = nullptr;
	OnRequestFinished.Clear();
	OnViewClosed.Clear();
	Super::Deinitialize();
}

void UCCLUISubsystem::PlayerControllerChanged(APlayerController* NewPlayerController)
{
	Super::PlayerControllerChanged(NewPlayerController);
	if (Root && Root->GetOwningPlayer() != NewPlayerController)
	{
		ResetRoot();
	}
}

void UCCLUISubsystem::Tick(float DeltaTime)
{
	TArray<FGuid> Expired;
	for (const auto& Pair : Registrations)
	{
		if (!Pair.Value.Owner.IsValid())
		{
			Expired.Add(Pair.Key);
		}
	}

	for (FGuid Id : Expired)
	{
		UnregisterView({Id});
	}

	TArray<FGuid> OpenIds;
	Views.GetKeys(OpenIds);
	OpenIds.Sort([this](const FGuid& A, const FGuid& B)
	{
		return Views.FindChecked(A).OpenOrder < Views.FindChecked(B).OpenOrder;
	});
	for (FGuid Id : OpenIds)
	{
		auto* View = Views.Find(Id);
		if (!View)
		{
			continue;
		}

		if (!View->Owner.IsValid() || (View->Definition.Scope == ECCLUIScope::World && View->World != GetWorld()) ||
			(View->Screen && (!Root || !Root->ContainsScreen(View->Screen) || !View->Screen->GetViewHandle().IsValid())))
		{
			CloseView({Id});
		}
		else if (!View->Screen && EnsureRoot())
		{
			AttachScreen({Id});
		}
	}

	TArray<FGuid> RequestIds;
	Pending.GetKeys(RequestIds);
	for (FGuid Id : RequestIds)
	{
		const auto* Request = Pending.Find(Id);
		const auto* Registration = Request ? Registrations.Find(Request->Registration.Id) : nullptr;
		if (!Request)
		{
			continue;
		}

		if (!Registration || !Request->Owner.IsValid() || !Registration->Owner.IsValid() ||
			(Registration->Definition.Scope == ECCLUIScope::World && Request->World != GetWorld()))
		{
			CancelRequest({Id});
		}
		else if (Request->bReady)
		{
			FinishRequest({Id});
		}
	}
}

TStatId UCCLUISubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UCCLUISubsystem, STATGROUP_Tickables);
}

UWorld* UCCLUISubsystem::GetTickableGameObjectWorld() const
{
	return GetWorld();
}

bool UCCLUISubsystem::IsTickable() const
{
	return !HasAnyFlags(RF_ClassDefaultObject) && !bShuttingDown &&
		(!Registrations.IsEmpty() || !Views.IsEmpty() || !Pending.IsEmpty());
}

bool UCCLUISubsystem::ConfigureRegistry(UCCLUIRegistry* InRegistry)
{
	TArray<FText> Errors;
	if (bShuttingDown || bCleaningWorld || bClosingAll || bRemovingScreen || !OpeningRegistrations.IsEmpty() ||
		!InRegistry || !Views.IsEmpty() || !Pending.IsEmpty() ||
		!InRegistry->ValidateRegistry(Errors))
	{
		return false;
	}

	ResetRoot();
	Registrations.Reset();
	Registry = DuplicateObject<UCCLUIRegistry>(InRegistry, this);
	for (const auto& Definition : Registry->Views)
	{
		RegisterView(Definition, this);
	}

	return true;
}

FCCLUIRegistrationHandle UCCLUISubsystem::RegisterView(const FCCLUIViewDefinition& Definition, UObject* Owner)
{
	TArray<FText> Errors;
	if (bShuttingDown || bCleaningWorld || !Registry || !IsValid(Owner) || FindRegistration(Definition.Tag).IsValid() ||
		!Registry->ValidateView(Definition, Errors))
	{
		return {};
	}

	FCCLUIRegistrationHandle Handle{FGuid::NewGuid()};
	auto& Registration = Registrations.Add(Handle.Id);
	Registration.Definition = Definition;
	Registration.Owner = Owner;
	return Handle;
}

void UCCLUISubsystem::UnregisterView(FCCLUIRegistrationHandle Registration)
{
	if (!Registrations.Remove(Registration.Id))
	{
		return;
	}

	TArray<FGuid> RequestIds;
	for (const auto& Pair : Pending)
	{
		if (Pair.Value.Registration == Registration)
		{
			RequestIds.Add(Pair.Key);
		}
	}

	for (FGuid Id : RequestIds)
	{
		CancelRequest({Id});
	}

	TArray<FGuid> ViewIds;
	for (const auto& Pair : Views)
	{
		if (Pair.Value.Registration == Registration)
		{
			ViewIds.Add(Pair.Key);
		}
	}

	for (FGuid Id : ViewIds)
	{
		CloseView({Id});
	}
}

FCCLUIViewHandle UCCLUISubsystem::OpenView(FGameplayTag ViewTag, UCCLUIContext* Context, UObject* Owner)
{
	const FCCLUIRegistrationHandle Registration = FindRegistration(ViewTag);
	const auto* Entry = Registrations.Find(Registration.Id);
	if (!CanOpen(Entry, Context, Owner) || !Entry->Definition.WidgetClass.Get() || OpeningRegistrations.Contains(Registration.Id))
	{
		return {};
	}

	// Retain a copy across close callbacks, which may modify registrations.
	const FCCLUIViewDefinition Definition = Entry->Definition;
	OpeningRegistrations.Add(Registration.Id);
	ON_SCOPE_EXIT { OpeningRegistrations.Remove(Registration.Id); };
	if (!EnsureRoot())
	{
		return {};
	}

	TArray<FGuid> Replaced;
	for (const auto& Pair : Views)
	{
		if (Pair.Value.Registration == Registration && Definition.InstancePolicy != ECCLUIInstancePolicy::Multiple &&
			(Definition.InstancePolicy == ECCLUIInstancePolicy::Single || Pair.Value.Context == Context))
		{
			if (Pair.Value.Owner == Owner && Pair.Value.Context == Context)
			{
				return {Pair.Key};
			}

			Replaced.Add(Pair.Key);
		}
	}

	for (FGuid Id : Replaced)
	{
		CloseView({Id});
	}

	if (!CanOpen(Registrations.Find(Registration.Id), Context, Owner))
	{
		return {};
	}

	FCCLUIViewHandle Handle{FGuid::NewGuid()};
	auto& View = Views.Add(Handle.Id);
	View.Definition = Definition;
	View.Registration = Registration;
	View.Context = Context;
	View.Owner = Owner;
	View.World = GetWorld();
	View.OpenOrder = ++NextOpenOrder;
	if (!AttachScreen(Handle))
	{
		Views.Remove(Handle.Id);
		return {};
	}

	return Handle;
}

FCCLUIRequestHandle UCCLUISubsystem::RequestOpenView(FGameplayTag ViewTag, UCCLUIContext* Context, UObject* Owner)
{
	const FCCLUIRegistrationHandle Registration = FindRegistration(ViewTag);
	const auto* Entry = Registrations.Find(Registration.Id);
	if (!CanOpen(Entry, Context, Owner))
	{
		return {};
	}

	const FSoftObjectPath Path = Entry->Definition.WidgetClass.ToSoftObjectPath();
	FCCLUIRequestHandle Handle{FGuid::NewGuid()};
	auto& Request = Pending.Add(Handle.Id);
	Request.Registration = Registration;
	Request.Context = Context;
	Request.Owner = Owner;
	Request.World = GetWorld();
	Request.bReady = Entry->Definition.WidgetClass.Get() != nullptr;
	if (!Request.bReady)
	{
		auto Load = UAssetManager::GetStreamableManager().RequestAsyncLoad(Path,
			FStreamableDelegate::CreateUObject(this, &ThisClass::MarkRequestReady, Handle.Id));
		if (Load.IsValid())
		{
			Loads.Add(Handle.Id, MoveTemp(Load));
		}
		else
		{
			Request.bReady = 1;
		}
	}

	return Handle;
}

void UCCLUISubsystem::CancelRequest(FCCLUIRequestHandle Request)
{
	if (!Pending.Remove(Request.Id))
	{
		return;
	}

	TSharedPtr<FStreamableHandle> Load;
	if (Loads.RemoveAndCopyValue(Request.Id, Load) && Load.IsValid())
	{
		Load->CancelHandle();
	}

	OnRequestFinished.Broadcast(Request, {}, false);
}

void UCCLUISubsystem::CloseView(FCCLUIViewHandle Handle)
{
	FCCLUIOpenView View;
	if (!Views.RemoveAndCopyValue(Handle.Id, View))
	{
		return;
	}

	if (Root && View.Screen)
	{
		TGuardValue<uint8> RemovingGuard(bRemovingScreen, 1);
		Root->RemoveScreen(View.Screen);
	}

	OnViewClosed.Broadcast(Handle);
}

void UCCLUISubsystem::CloseAllViews()
{
	TGuardValue<uint8> ClosingGuard(bClosingAll, 1);
	TArray<FGuid> ViewIds;
	Views.GetKeys(ViewIds);
	for (FGuid Id : ViewIds)
	{
		CloseView({Id});
	}
}

UCCLScreen* UCCLUISubsystem::FindScreen(FCCLUIViewHandle View) const
{
	const auto* Entry = Views.Find(View.Id);
	return Entry ? Entry->Screen.Get() : nullptr;
}

FCCLUIRegistrationHandle UCCLUISubsystem::FindRegistration(FGameplayTag View) const
{
	for (const auto& Pair : Registrations)
	{
		if (Pair.Value.Definition.Tag == View)
		{
			return {Pair.Key};
		}
	}

	return {};
}

bool UCCLUISubsystem::EnsureRoot()
{
	if (Root)
	{
		return true;
	}

	auto* PC = GetLocalPlayer()->GetPlayerController(GetWorld());
	if (!Registry || !IsValid(PC) || !PC->IsLocalController() || bShuttingDown || bCleaningWorld)
	{
		return false;
	}

	Root = CreateWidget<UCCLUIRoot>(PC);
	if (!Root || !Root->Configure(Registry) || !Root->AddToPlayerScreen())
	{
		Root = nullptr;
		return false;
	}

	Root->TakeWidget();
	Root->ActivateWidget();
	return true;
}

bool UCCLUISubsystem::AttachScreen(FCCLUIViewHandle Handle)
{
	const auto* Entry = Views.Find(Handle.Id);
	if (!Entry || !Root)
	{
		return false;
	}

	const FCCLUIOpenView Snapshot = *Entry;
	auto* Screen = Root->AddScreen(Snapshot.Definition, [this, Snapshot, Handle](UCCLScreen& Instance)
	{
		Instance.BindContext(Snapshot.Context, Handle, Snapshot.Definition.InputPolicy);
		Instance.OnCloseRequested.AddUObject(this, &ThisClass::CloseView);
	});

	if (auto* Current = Views.Find(Handle.Id); Current && Screen && Screen->GetViewHandle() == Handle)
	{
		Current->Screen = Screen;
		Current->World = GetWorld();
		return true;
	}

	if (Root && Screen)
	{
		Root->RemoveScreen(Screen);
	}

	return false;
}

void UCCLUISubsystem::ResetRoot()
{
	TGuardValue<uint8> CleanupGuard(bCleaningWorld, 1);
	if (Root)
	{
		Root->ClearScreens();
		Root->DeactivateWidget();
		Root->RemoveFromParent();
		Root = nullptr;
	}

	for (auto& Pair : Views)
	{
		Pair.Value.Screen = nullptr;
	}
}

void UCCLUISubsystem::OnWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources)
{
	TGuardValue<uint8> CleanupGuard(bCleaningWorld, 1);
	TArray<FGuid> ViewIds;
	for (const auto& Pair : Views)
	{
		if (Pair.Value.World == World && Pair.Value.Definition.Scope == ECCLUIScope::World)
		{
			ViewIds.Add(Pair.Key);
		}
	}

	for (FGuid Id : ViewIds)
	{
		CloseView({Id});
	}

	TArray<FGuid> RequestIds;
	for (const auto& Pair : Pending)
	{
		const auto* Registration = Registrations.Find(Pair.Value.Registration.Id);
		if (Pair.Value.World == World && (!Registration || Registration->Definition.Scope == ECCLUIScope::World))
		{
			RequestIds.Add(Pair.Key);
		}
	}

	for (FGuid Id : RequestIds)
	{
		CancelRequest({Id});
	}

	if (Root && Root->GetWorld() == World)
	{
		ResetRoot();
	}
}

void UCCLUISubsystem::FinishRequest(FCCLUIRequestHandle Request)
{
	FCCLUIPendingView PendingView;
	if (!Pending.RemoveAndCopyValue(Request.Id, PendingView))
	{
		return;
	}

	const auto* Entry = Registrations.Find(PendingView.Registration.Id);
	const FCCLUIViewHandle View = Entry ? OpenView(Entry->Definition.Tag, PendingView.Context, PendingView.Owner.Get()) : FCCLUIViewHandle();
	Loads.Remove(Request.Id);
	OnRequestFinished.Broadcast(Request, View, View.IsValid());
}

void UCCLUISubsystem::MarkRequestReady(FGuid Id)
{
	if (auto* Request = Pending.Find(Id))
	{
		Request->bReady = 1;
	}
}

bool UCCLUISubsystem::CanOpen(const FCCLUIRegistration* Registration, UCCLUIContext* Context, UObject* Owner) const
{
	TArray<FText> Errors;
	return !bShuttingDown && !bCleaningWorld && !bClosingAll && !bRemovingScreen && Registry && Registration &&
		Registry->ValidateView(Registration->Definition, Errors) &&
		Registration->Owner.IsValid() && IsValid(Owner) &&
		Registration->Definition.AcceptsContext(Context) && GetWorld() &&
		(!Owner->GetWorld() || Owner->GetWorld() == GetWorld());
}
