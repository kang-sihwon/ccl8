#include "KSIUISubsystem.h"

#include "KSIScreen.h"
#include "KSIUIContext.h"
#include "KSIUIRoot.h"
#include "KSIUIActionRouter.h"
#include "Engine/AssetManager.h"
#include "Engine/LocalPlayer.h"
#include "Engine/StreamableManager.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Misc/ScopeExit.h"

void UKSIUISubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	WorldCleanupHandle = FWorldDelegates::OnWorldCleanup.AddUObject(this, &ThisClass::OnWorldCleanup);
}

void UKSIUISubsystem::Deinitialize()
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
	Presentations.Reset();
	ResetRoot();
	Registrations.Reset();
	Registry = nullptr;
	OnRequestFinished.Clear();
	OnViewClosed.Clear();
	Super::Deinitialize();
}

void UKSIUISubsystem::PlayerControllerChanged(APlayerController* NewPlayerController)
{
	Super::PlayerControllerChanged(NewPlayerController);
	if (Root && Root->GetOwningPlayer() != NewPlayerController)
	{
		ResetRoot();
	}
}

void UKSIUISubsystem::Tick(float DeltaTime)
{
	TArray<FGuid> ExpiredPresentation;
	float ExitFade = 0.f;
	for (const auto& Pair : Presentations)
	{
		if (!Pair.Value.Owner.IsValid() || (Pair.Value.Definition.Scope == EKSIUIScope::World && Pair.Value.World != GetWorld()))
		{
			ExpiredPresentation.Add(Pair.Key);
			ExitFade = FMath::Max(ExitFade, Pair.Value.Definition.FadeSeconds);
		}
	}

	for (const auto& Id : ExpiredPresentation)
	{
		Presentations.Remove(Id);
	}

	if (!ExpiredPresentation.IsEmpty() || bPresentationDirty)
	{
		RefreshPresentation(ExitFade);
	}

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

		if (!View->Owner.IsValid() || (View->Definition.Scope == EKSIUIScope::World && View->World != GetWorld()) ||
			(View->Screen && (!Root || !Root->ContainsScreen(View->Screen) || !View->Screen->GetViewHandle().IsValid())))
		{
			CloseView({Id});
		}
		else if (!View->Screen && EnsureRoot())
		{
			AttachScreen({Id});
		}

		if (auto* Screen = FindScreen({Id}))
		{
			Screen->TickManagedPresentation(DeltaTime);
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
			(Registration->Definition.Scope == EKSIUIScope::World && Request->World != GetWorld()))
		{
			CancelRequest({Id});
		}
		else if (Request->bReady)
		{
			FinishRequest({Id});
		}
	}

	if (bPresentationRoutingDirty)
	{
		bPresentationRoutingDirty = 0;
		if (auto* Router = GetLocalPlayer()->GetSubsystem<UKSIUIActionRouter>())
		{
			Router->RefreshPresentationRouting();
		}
	}
}

TStatId UKSIUISubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UKSIUISubsystem, STATGROUP_Tickables);
}

UWorld* UKSIUISubsystem::GetTickableGameObjectWorld() const
{
	return GetWorld();
}

bool UKSIUISubsystem::IsTickable() const
{
	return !HasAnyFlags(RF_ClassDefaultObject) && !bShuttingDown &&
		(!Registrations.IsEmpty() || !Views.IsEmpty() || !Pending.IsEmpty() || !Presentations.IsEmpty());
}

bool UKSIUISubsystem::ConfigureRegistry(UKSIUIRegistry* InRegistry)
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
	Registry = DuplicateObject<UKSIUIRegistry>(InRegistry, this);
	for (const auto& Definition : Registry->Views)
	{
		RegisterView(Definition, this);
	}

	return true;
}

FKSIUIRegistrationHandle UKSIUISubsystem::RegisterView(const FKSIUIViewDefinition& Definition, UObject* Owner)
{
	TArray<FText> Errors;
	if (bShuttingDown || bCleaningWorld || !Registry || !IsValid(Owner) || FindRegistration(Definition.Tag).IsValid() ||
		!Registry->ValidateView(Definition, Errors))
	{
		return {};
	}

	FKSIUIRegistrationHandle Handle{FGuid::NewGuid()};
	auto& Registration = Registrations.Add(Handle.Id);
	Registration.Definition = Definition;
	Registration.Owner = Owner;
	return Handle;
}

void UKSIUISubsystem::UnregisterView(FKSIUIRegistrationHandle Registration)
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

FKSIUIViewHandle UKSIUISubsystem::OpenView(FGameplayTag ViewTag, UKSIUIContext* Context, UObject* Owner)
{
	const FKSIUIRegistrationHandle Registration = FindRegistration(ViewTag);
	const auto* Entry = Registrations.Find(Registration.Id);
	if (!CanOpen(Entry, Context, Owner) || !Entry->Definition.WidgetClass.Get() || OpeningRegistrations.Contains(Registration.Id))
	{
		return {};
	}

	// Retain a copy across close callbacks, which may modify registrations.
	const FKSIUIViewDefinition Definition = Entry->Definition;
	OpeningRegistrations.Add(Registration.Id);
	ON_SCOPE_EXIT { OpeningRegistrations.Remove(Registration.Id); };
	if (!EnsureRoot())
	{
		return {};
	}

	TArray<FGuid> Replaced;
	for (const auto& Pair : Views)
	{
		if (Pair.Value.Registration == Registration && Definition.InstancePolicy != EKSIUIInstancePolicy::Multiple &&
			(Definition.InstancePolicy == EKSIUIInstancePolicy::Single || Pair.Value.Context == Context))
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

	FKSIUIViewHandle Handle{FGuid::NewGuid()};
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

FKSIUIRequestHandle UKSIUISubsystem::RequestOpenView(FGameplayTag ViewTag, UKSIUIContext* Context, UObject* Owner)
{
	const FKSIUIRegistrationHandle Registration = FindRegistration(ViewTag);
	const auto* Entry = Registrations.Find(Registration.Id);
	if (!CanOpen(Entry, Context, Owner))
	{
		return {};
	}

	const FSoftObjectPath Path = Entry->Definition.WidgetClass.ToSoftObjectPath();
	FKSIUIRequestHandle Handle{FGuid::NewGuid()};
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

void UKSIUISubsystem::CancelRequest(FKSIUIRequestHandle Request)
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

void UKSIUISubsystem::CloseView(FKSIUIViewHandle Handle)
{
	FKSIUIOpenView View;
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

void UKSIUISubsystem::CloseAllViews()
{
	TGuardValue<uint8> ClosingGuard(bClosingAll, 1);
	TArray<FGuid> ViewIds;
	Views.GetKeys(ViewIds);
	for (FGuid Id : ViewIds)
	{
		CloseView({Id});
	}
}

UKSIScreen* UKSIUISubsystem::FindScreen(FKSIUIViewHandle View) const
{
	const auto* Entry = Views.Find(View.Id);
	return Entry ? Entry->Screen.Get() : nullptr;
}

FKSIUIRegistrationHandle UKSIUISubsystem::FindRegistration(FGameplayTag View) const
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

bool UKSIUISubsystem::EnsureRoot()
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

	Root = CreateWidget<UKSIUIRoot>(PC);
	if (!Root || !Root->Configure(Registry) || !Root->AddToPlayerScreen())
	{
		Root = nullptr;
		return false;
	}

	Root->TakeWidget();
	Root->ActivateWidget();
	return true;
}

bool UKSIUISubsystem::AttachScreen(FKSIUIViewHandle Handle)
{
	const auto* Entry = Views.Find(Handle.Id);
	if (!Entry || !Root)
	{
		return false;
	}

	const FKSIUIOpenView Snapshot = *Entry;
	auto* Screen = Root->AddScreen(Snapshot.Definition, [this, Snapshot, Handle](UKSIScreen& Instance)
	{
		Instance.BindContext(Snapshot.Context, Handle, Snapshot.Definition.InputPolicy);
		Instance.OnCloseRequested.AddUObject(this, &ThisClass::CloseView);
	});

	if (auto* Current = Views.Find(Handle.Id); Current && Screen && Screen->GetViewHandle() == Handle)
	{
		Current->Screen = Screen;
		Current->World = GetWorld();
		ApplyPresentation(Handle, 0.f);
		// A feature hook may close the new view while applying an existing request.
		return IsViewOpen(Handle);
	}

	if (Root && Screen)
	{
		Root->RemoveScreen(Screen);
	}

	return false;
}

void UKSIUISubsystem::ResetRoot()
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

void UKSIUISubsystem::OnWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources)
{
	TGuardValue<uint8> CleanupGuard(bCleaningWorld, 1);
	for (auto It = Presentations.CreateIterator(); It; ++It)
	{
		if (It.Value().World == World && It.Value().Definition.Scope == EKSIUIScope::World)
		{
			It.RemoveCurrent();
		}
	}

	bPresentationDirty = 1;
	TArray<FGuid> ViewIds;
	for (const auto& Pair : Views)
	{
		if (Pair.Value.World == World && Pair.Value.Definition.Scope == EKSIUIScope::World)
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
		if (Pair.Value.World == World && (!Registration || Registration->Definition.Scope == EKSIUIScope::World))
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

void UKSIUISubsystem::FinishRequest(FKSIUIRequestHandle Request)
{
	FKSIUIPendingView PendingView;
	if (!Pending.RemoveAndCopyValue(Request.Id, PendingView))
	{
		return;
	}

	const auto* Entry = Registrations.Find(PendingView.Registration.Id);
	const FKSIUIViewHandle View = Entry ? OpenView(Entry->Definition.Tag, PendingView.Context, PendingView.Owner.Get()) : FKSIUIViewHandle();
	Loads.Remove(Request.Id);
	OnRequestFinished.Broadcast(Request, View, View.IsValid());
}

void UKSIUISubsystem::MarkRequestReady(FGuid Id)
{
	if (auto* Request = Pending.Find(Id))
	{
		Request->bReady = 1;
	}
}

FKSIUIPresentationHandle UKSIUISubsystem::PushPresentation(const FKSIUIPresentationDefinition& Definition, UObject* Owner)
{
	if (bShuttingDown || bCleaningWorld || !IsValid(Owner) || !GetWorld() ||
		(Owner->GetWorld() && Owner->GetWorld() != GetWorld()) || !Definition.IsValid())
	{
		return {};
	}

	FKSIUIPresentationHandle Handle{FGuid::NewGuid()};
	auto& Request = Presentations.Add(Handle.Id);
	Request.Definition = Definition;
	Request.Owner = Owner;
	Request.World = GetWorld();
	if (Definition.bBlockGameplay)
	{
		if (auto* PC = GetLocalPlayer()->GetPlayerController(GetWorld()))
		{
			PC->FlushPressedKeys();
		}
	}

	RefreshPresentation(Definition.FadeSeconds);
	return Handle;
}

void UKSIUISubsystem::ReleasePresentation(FKSIUIPresentationHandle Handle)
{
	FKSIUIPresentationRequest Removed;
	if (Presentations.RemoveAndCopyValue(Handle.Id, Removed))
	{
		RefreshPresentation(Removed.Definition.FadeSeconds);
	}
}

bool UKSIUISubsystem::SetBasePresentation(FKSIUIViewHandle Handle, const FKSIUIViewPresentation& State)
{
	auto* View = Views.Find(Handle.Id);
	if (!View || !State.IsValid())
	{
		return false;
	}

	View->BasePresentation = State;
	RefreshPresentation();
	return true;
}

bool UKSIUISubsystem::IsGameplayInputBlocked() const
{
	for (const auto& Pair : Presentations)
	{
		if (Pair.Value.Owner.IsValid() && Pair.Value.Definition.bBlockGameplay &&
			(Pair.Value.Definition.Scope != EKSIUIScope::World || Pair.Value.World == GetWorld()))
		{
			return true;
		}
	}

	return false;
}

void UKSIUISubsystem::RefreshPresentation(float FadeSeconds)
{
	bPresentationDirty = 1;
	if (bRefreshingPresentation || bShuttingDown)
	{
		return;
	}

	TGuardValue<uint8> Guard(bRefreshingPresentation, 1);
	bPresentationDirty = 0;
	TArray<FGuid> Ids;
	Views.GetKeys(Ids);
	for (const auto& Id : Ids)
	{
		ApplyPresentation({Id}, FadeSeconds);
	}

	InvalidatePresentationRouting();
}

void UKSIUISubsystem::ApplyPresentation(FKSIUIViewHandle Handle, float FadeSeconds)
{
	const auto* View = Views.Find(Handle.Id);
	if (!View || !View->Screen)
	{
		return;
	}

	FKSIUIViewPresentation State = View->BasePresentation;
	for (const auto& Pair : Presentations)
	{
		const auto& Request = Pair.Value.Definition;
		if (Pair.Value.Owner.IsValid() && (Request.Scope != EKSIUIScope::World || Pair.Value.World == GetWorld()) &&
			Request.Matches(View->Definition.Groups))
		{
			State.bVisible = State.bVisible && !Request.bHide;
			State.bInputEnabled = State.bInputEnabled && !Request.bDisableInput;
			State.bUpdatesEnabled = State.bUpdatesEnabled && !Request.bSuspendUpdates;
			State.Opacity = FMath::Min(State.Opacity, Request.Opacity);
			FadeSeconds = FMath::Max(FadeSeconds, Request.FadeSeconds);
		}
	}

	View->Screen->ApplyPresentation(State, FadeSeconds);
}

bool UKSIUISubsystem::CanOpen(const FKSIUIRegistration* Registration, UKSIUIContext* Context, UObject* Owner) const
{
	TArray<FText> Errors;
	return !bShuttingDown && !bCleaningWorld && !bClosingAll && !bRemovingScreen && Registry && Registration &&
		Registry->ValidateView(Registration->Definition, Errors) &&
		Registration->Owner.IsValid() && IsValid(Owner) &&
		Registration->Definition.AcceptsContext(Context) && GetWorld() &&
		(!Owner->GetWorld() || Owner->GetWorld() == GetWorld());
}
