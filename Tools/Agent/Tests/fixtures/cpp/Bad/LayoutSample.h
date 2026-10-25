#include "GameFramework/Actor.h"
#include "CoreMinimal.h"
#include "LayoutSample.generated.h"
#include "Engine/World.h"

UCLASS()
class ALayoutSample : public AActor
{
	GENERATED_BODY()
public:

	ALayoutSample();

#if WITH_EDITOR
	UPROPERTY()
	TObjectPtr<UObject> EditorPreview;
#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TObjectPtr<UObject> Nested;
#endif
#endif


private:
	bool bReady = false;

};
