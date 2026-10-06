#pragma once
#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Engine/EngineBaseTypes.h"
#include "CCLSessionRecord.h"
#include "CCLGameInstance.generated.h"
class SWidget;
class SEditableTextBox;
UCLASS()
class CCL_API UCCLGameInstance : public UGameInstance
{
	GENERATED_BODY()
public:
	virtual void Init() override;
	virtual void Shutdown() override;
	void ShowMenu();
	void HideMenu();
	void ToggleMenu();
	bool IsMenuVisible() const { return Menu.IsValid(); }
	void StartNew(bool bHost);
	bool Join(const FString& Address);
	bool SaveSession();
	bool LoadSession(bool bHost);
	void ReturnToMenu();
	void Quit();
	void CycleQuality();
	void ToggleWindowMode();
	const FString& GetStatus() const { return Status; }
	bool IsRestoring() const { return bPendingRestore != 0; }
	static bool ValidateAddress(const FString& Address);
	static FString SaveSlot();
private:
	void BeforeMap(const FString& MapName);
	void AfterMap(UWorld* World);
	void TryRestore();
	void NetworkFailed(UWorld* World, UNetDriver* Driver, ENetworkFailure::Type Type, const FString& Error);
	void TravelFailed(UWorld* World, ETravelFailure::Type Type, const FString& Error);
	bool ApplyRecord(const FCCLSessionRecord& Record);
	TSharedPtr<SWidget> Menu;
	TSharedPtr<SEditableTextBox> AddressBox;
	FString Status = TEXT("Choose a mode. Direct connection requires a reachable host address.");
	UPROPERTY() FCCLSessionRecord Pending;
	uint8 bPendingRestore = 0;
	FTimerHandle RestoreTimer;
	double RestoreStarted = 0.;
};
