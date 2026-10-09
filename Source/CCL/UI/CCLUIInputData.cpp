#include "CCLUIInputData.h"

#include "Engine/World.h"
#include "InputAction.h"
#include "InputMappingContext.h"

UCCLUIInputData::UCCLUIInputData()
{
	EnhancedInputBackAction = CreateDefaultSubobject<UInputAction>(TEXT("Back"));
	EnhancedInputClickAction = CreateDefaultSubobject<UInputAction>(TEXT("Accept"));
	EnhancedInputBackAction->bConsumeInput = false;
	EnhancedInputClickAction->bConsumeInput = false;
	Mapping = CreateDefaultSubobject<UInputMappingContext>(TEXT("Mapping"));
	PIEMapping = CreateDefaultSubobject<UInputMappingContext>(TEXT("PIEMapping"));
	Mapping->MapKey(EnhancedInputBackAction, EKeys::Escape);
	PIEMapping->MapKey(EnhancedInputBackAction, EKeys::F6);
	for (auto* Context : {Mapping.Get(), PIEMapping.Get()})
	{
		Context->MapKey(EnhancedInputBackAction, EKeys::Gamepad_FaceButton_Right);
		Context->MapKey(EnhancedInputClickAction, EKeys::Enter);
		Context->MapKey(EnhancedInputClickAction, EKeys::Gamepad_FaceButton_Bottom);
	}
}

FKey UCCLUIInputData::GetBackKey(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	return World && World->WorldType == EWorldType::PIE ? EKeys::F6 : EKeys::Escape;
}

const TCHAR* UCCLUIInputData::GetBackKeyLabel(const UObject* WorldContext)
{
	return GetBackKey(WorldContext) == EKeys::F6 ? TEXT("F6") : TEXT("Esc");
}

UInputMappingContext* UCCLUIInputData::GetMapping(const UObject* WorldContext) const
{
	return GetBackKey(WorldContext) == EKeys::F6 ? PIEMapping : Mapping;
}
