#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LayoutSample.generated.h"

UCLASS()
class ALayoutSample : public AActor
{
	GENERATED_BODY()

	// Parent interface
public:
	ALayoutSample();

	virtual void BeginPlay() override;

	// Own functions
public:
	bool IsReady() const { return bReady != 0; }

#if WITH_EDITORONLY_DATA
private:
	UPROPERTY()
	TObjectPtr<UObject> EditorPreview;
#endif

	// Properties
private:
	UPROPERTY(EditAnywhere)
	uint8 bReady = 0;
};
