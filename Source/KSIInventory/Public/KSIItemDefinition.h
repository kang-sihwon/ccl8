#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "StructUtils/InstancedStruct.h"
#include "KSIItemDefinition.generated.h"

class UKSIItemDefinition;
class UTexture2D;

// Fragment 검증에 정의와 오류 목록을 함께 넘긴다.
struct KSIINVENTORY_API FKSIItemValidation
{
	const UKSIItemDefinition& Item;
	TArray<FText>& Errors;

	void Error(const FText& Message) const { Errors.Add(Message); }
};

// 아이템 정의에 붙는 선택 기능의 공용 루트다. 파생 구조체가 자기 검증과 인스턴스 상태 타입을 선언한다.
USTRUCT(BlueprintType)
struct KSIINVENTORY_API FKSIItemFragmentData
{
	GENERATED_BODY()

	virtual ~FKSIItemFragmentData() = default;

	virtual void Validate(const FKSIItemValidation& Context) const {}

	// 아이템마다 따로 바뀌는 상태가 필요하면 그 구조체 타입을, 필요 없으면 null을 돌려준다.
	virtual const UScriptStruct* GetInstanceStateType() const { return nullptr; }
};

UCLASS(BlueprintType)
class KSIINVENTORY_API UKSIItemDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

	// 부모 인터페이스 함수
public:
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

	// 내 클래스 함수
public:
	// 정의 필드를 검사하고 각 Fragment의 Validate를 부른다.
	virtual bool ValidateDefinition(TArray<FText>& Errors) const;

	// 파생 Fragment도 찾는다. 같은 계열의 Fragment가 둘이면 먼저 넣은 것을 돌려준다.
	template <class T>
	const T* FindFragment() const
	{
		for (const FInstancedStruct& Fragment : ItemFragments)
		{
			if (const T* Value = Fragment.GetPtr<T>())
			{
				return Value;
			}
		}
		return nullptr;
	}

	FText GetLabel() const;
	int32 GetMaxStack() const;

	// 프로퍼티
public:
	UPROPERTY(EditAnywhere, Category = "Item")
	FText ItemName;

	UPROPERTY(EditAnywhere, Category = "Item", meta = (MultiLine = true))
	FText Description;

	UPROPERTY(EditAnywhere, Category = "Item")
	TObjectPtr<UTexture2D> IconTexture;

	// 한 칸에 쌓을 수 있는 최대 개수다. 실제 개수는 인벤토리 항목의 Quantity다.
	UPROPERTY(EditAnywhere, Category = "Item", meta = (ClampMin = "1", ClampMax = "1000"))
	int32 MaxStackCount = 1;

	UPROPERTY(EditAnywhere, Category = "Item", meta = (BaseStruct = "/Script/KSIInventory.KSIItemFragmentData", ExcludeBaseStruct))
	TArray<FInstancedStruct> ItemFragments;
};
