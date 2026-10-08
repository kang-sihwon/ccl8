#include "CCLUIInputData.h"

#include "InputAction.h"
#include "InputMappingContext.h"

UCCLUIInputData::UCCLUIInputData()
{
	EnhancedInputBackAction = CreateDefaultSubobject<UInputAction>(TEXT("Back"));
	EnhancedInputClickAction = CreateDefaultSubobject<UInputAction>(TEXT("Accept"));
	EnhancedInputBackAction->bConsumeInput = false;
	EnhancedInputClickAction->bConsumeInput = false;
	Mapping = CreateDefaultSubobject<UInputMappingContext>(TEXT("Mapping"));
	Mapping->MapKey(EnhancedInputBackAction, EKeys::Escape);
	Mapping->MapKey(EnhancedInputBackAction, EKeys::Gamepad_FaceButton_Right);
	Mapping->MapKey(EnhancedInputClickAction, EKeys::Enter);
	Mapping->MapKey(EnhancedInputClickAction, EKeys::Gamepad_FaceButton_Bottom);
}
